#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include "httplib.h"
#include "Model.h"
#include "Controller.h"

using namespace std;

LibraryDatabase g_db;
LibraryController g_ctrl(g_db);
mutex g_mtx;

string jsonEscape(const string& s) {
    string out;
    out.reserve(s.size() + 16);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;      break;
        }
    }
    return out;
}

User* findUserById(const string& userId) {
    for (auto& a : g_db.admins)     if (a.getUserId() == userId) return &a;
    for (auto& l : g_db.librarians) if (l.getUserId() == userId) return &l;
    for (auto& m : g_db.members)    if (m.getUserId() == userId) return &m;
    return nullptr;
}
User* getActor(const httplib::Request& req) {
    string uid = req.get_header_value("X-User-Id");
    if (uid.empty() && req.has_param("actorId")) {
        uid = req.get_param_value("actorId");
    }
    return findUserById(uid);
}

void sendForbidden(httplib::Response& res, const string& msg) {
    res.status = 403;
    res.set_content("{\"ok\":false,\"message\":\"" + jsonEscape(msg) + "\"}", "application/json; charset=utf-8");
}

string buildStateJson(User* actor) {
    string role = actor ? actor->getRole() : "Guest";
    string actorId = actor ? actor->getUserId() : "";
    bool isMember = (role == "Member");
    bool isStaff = (role == "Librarian" || role == "Administrator");

    ostringstream os;
    os << "{";

    os << "\"serverDate\":\"" << jsonEscape(DateUtils::today()) << "\",";
    os << "\"serverDateTime\":\"" << jsonEscape(DateUtils::nowDateTime()) << "\",";

    os << "\"policy\":{"
       << "\"defaultLoanDays\":" << g_db.defaultLoanDays << ","
       << "\"fineRatePerDay\":" << (long long)g_db.fineRatePerDay
       << "},";

    os << "\"books\":[";
    for (size_t i = 0; i < g_db.books.size(); ++i) {
        const auto& b = g_db.books[i];
        if (i) os << ",";
        os << "{"
           << "\"bookId\":\"" << jsonEscape(b.bookId) << "\","
           << "\"isbn\":\"" << jsonEscape(b.isbn) << "\","
           << "\"title\":\"" << jsonEscape(b.title) << "\","
           << "\"author\":\"" << jsonEscape(b.author) << "\","
           << "\"publisher\":\"" << jsonEscape(b.publisher) << "\","
           << "\"pubYear\":" << b.pubYear << ","
           << "\"categoryId\":\"" << jsonEscape(b.categoryId) << "\""
           << "}";
    }
    os << "],";

    os << "\"bookCopies\":[";
    for (size_t i = 0; i < g_db.bookCopies.size(); ++i) {
        const auto& c = g_db.bookCopies[i];
        if (i) os << ",";
        os << "{"
           << "\"copyId\":\"" << jsonEscape(c.copyId) << "\","
           << "\"barcode\":\"" << jsonEscape(c.barcode) << "\","
           << "\"rackLocation\":\"" << jsonEscape(c.rackLocation) << "\","
           << "\"copyStatus\":\"" << jsonEscape(c.copyStatus) << "\","
           << "\"bookId\":\"" << jsonEscape(c.bookId) << "\""
           << "}";
    }
    os << "],";

    os << "\"loans\":[";
    bool first = true;
    for (const auto& l : g_db.loans) {
        if (isMember && l.memberId != actorId) continue;
        if (!first) os << ",";
        first = false;
        os << "{"
           << "\"loanId\":\"" << jsonEscape(l.loanId) << "\","
           << "\"memberId\":\"" << jsonEscape(l.memberId) << "\","
           << "\"librarianId\":\"" << jsonEscape(l.librarianId) << "\","
           << "\"copyId\":\"" << jsonEscape(l.copyId) << "\","
           << "\"issueDate\":\"" << jsonEscape(l.issueDate) << "\","
           << "\"dueDate\":\"" << jsonEscape(l.dueDate) << "\","
           << "\"returnDate\":\"" << jsonEscape(l.returnDate) << "\","
           << "\"renewalCount\":" << l.renewalCount << ","
           << "\"status\":\"" << jsonEscape(l.status) << "\""
           << "}";
    }
    os << "],";

    os << "\"fines\":[";
    first = true;
    for (const auto& f : g_db.fines) {
        if (isMember) {
            bool mine = false;
            for (const auto& l : g_db.loans) {
                if (l.loanId == f.loanId && l.memberId == actorId) { mine = true; break; }
            }
            if (!mine) continue;
        }
        if (!first) os << ",";
        first = false;
        os << "{"
           << "\"fineId\":\"" << jsonEscape(f.fineId) << "\","
           << "\"daysOverdue\":" << f.daysOverdue << ","
           << "\"amount\":" << (long long)f.amount << ","
           << "\"paidStatus\":\"" << jsonEscape(f.paidStatus) << "\","
           << "\"paymentDate\":\"" << jsonEscape(f.paymentDate) << "\","
           << "\"loanId\":\"" << jsonEscape(f.loanId) << "\""
           << "}";
    }
    os << "],";

    os << "\"reservations\":[";
    first = true;
    for (const auto& r : g_db.reservations) {
        if (isMember && r.memberId != actorId) continue;
        if (!first) os << ",";
        first = false;
        os << "{"
           << "\"reservationId\":\"" << jsonEscape(r.reservationId) << "\","
           << "\"requestDate\":\"" << jsonEscape(r.requestDate) << "\","
           << "\"expiryDate\":\"" << jsonEscape(r.expiryDate) << "\","
           << "\"status\":\"" << jsonEscape(r.status) << "\","
           << "\"memberId\":\"" << jsonEscape(r.memberId) << "\","
           << "\"bookId\":\"" << jsonEscape(r.bookId) << "\","
           << "\"copyId\":\"" << jsonEscape(r.copyId) << "\""
           << "}";
    }
    os << "],";

    os << "\"admins\":[";
    if (isStaff) {
        for (size_t i = 0; i < g_db.admins.size(); ++i) {
            const auto& a = g_db.admins[i];
            if (i) os << ",";
            os << "{"
               << "\"userId\":\"" << jsonEscape(a.getUserId()) << "\","
               << "\"fullName\":\"" << jsonEscape(a.getFullName()) << "\","
               << "\"email\":\"" << jsonEscape(a.getEmail()) << "\","
               << "\"role\":\"" << jsonEscape(a.getRole()) << "\""
               << "}";
        }
    }
    os << "],";

    os << "\"librarians\":[";
    if (isStaff) {
        for (size_t i = 0; i < g_db.librarians.size(); ++i) {
            const auto& l = g_db.librarians[i];
            if (i) os << ",";
            os << "{"
               << "\"userId\":\"" << jsonEscape(l.getUserId()) << "\","
               << "\"fullName\":\"" << jsonEscape(l.getFullName()) << "\","
               << "\"email\":\"" << jsonEscape(l.getEmail()) << "\","
               << "\"role\":\"" << jsonEscape(l.getRole()) << "\","
               << "\"staffCode\":\"" << jsonEscape(l.staffCode) << "\","
               << "\"deskShift\":\"" << jsonEscape(l.deskShift) << "\""
               << "}";
        }
    }
    os << "],";

    os << "\"members\":[";
    first = true;
    for (const auto& m : g_db.members) {
        if (isMember && m.getUserId() != actorId) continue;
        if (!first) os << ",";
        first = false;
        os << "{"
           << "\"userId\":\"" << jsonEscape(m.getUserId()) << "\","
           << "\"fullName\":\"" << jsonEscape(m.getFullName()) << "\","
           << "\"email\":\"" << jsonEscape(m.getEmail()) << "\","
           << "\"role\":\"" << jsonEscape(m.getRole()) << "\","
           << "\"membershipCardNo\":\"" << jsonEscape(m.membershipCardNo) << "\","
           << "\"maxQuota\":" << m.maxQuota << ","
           << "\"activeLoansCount\":" << m.activeLoansCount << ","
           << "\"totalUnpaidFines\":" << (long long)m.totalUnpaidFines
           << "}";
    }
    os << "],";

    os << "\"auditReport\":\"" << (isStaff ? jsonEscape(g_ctrl.exportAuditReport()) : "") << "\"";
    os << "}";
    return os.str();
}

int safeInt(const string& s, int fallback) {
    try { return s.empty() ? fallback : stoi(s); } catch (...) { return fallback; }
}

double safeDouble(const string& s, double fallback) {
    try { return s.empty() ? fallback : stod(s); } catch (...) { return fallback; }
}

int main() {
    httplib::Server svr;
    svr.set_mount_point("/", "./public");

    // 1. Dang nhap
    svr.Post("/api/login", [](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lock(g_mtx);
        string email = req.get_param_value("email");
        string pass  = req.get_param_value("password");
        User* u = g_ctrl.login(email, pass);
        if (!u) {
            res.set_content("{\"ok\":false,\"message\":\"Wrong email or password!\"}", "application/json; charset=utf-8");
            return;
        }
        ostringstream os;
        os << "{\"ok\":true,\"user\":{"
           << "\"userId\":\"" << jsonEscape(u->getUserId()) << "\","
           << "\"fullName\":\"" << jsonEscape(u->getFullName()) << "\","
           << "\"email\":\"" << jsonEscape(u->getEmail()) << "\","
           << "\"role\":\"" << jsonEscape(u->getRole()) << "\""
           << "}}";
        res.set_content(os.str(), "application/json; charset=utf-8");
    });

    // Dang ky tai khoan Doc gia (Member) moi ngay tai trang Dang nhap
    svr.Post("/api/register", [](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lock(g_mtx);
        string name = req.get_param_value("name");
        string email = req.get_param_value("email");
        string pass = req.get_param_value("password");
        string confirm = req.get_param_value("confirmPassword");

        if (!confirm.empty() && pass != confirm) {
            res.set_content("{\"ok\":false,\"message\":\"Error: Confirm password not match!\"}", "application/json; charset=utf-8");
            return;
        }

        string msg, newId;
        bool ok = g_ctrl.registerMember(name, email, pass, msg, newId);
        if (!ok) {
            res.set_content("{\"ok\":false,\"message\":\"" + jsonEscape(msg) + "\"}", "application/json; charset=utf-8");
            return;
        }

        ostringstream os;
        os << "{\"ok\":true,\"message\":\"" << jsonEscape(msg) << "\",\"user\":{"
           << "\"userId\":\"" << jsonEscape(newId) << "\","
           << "\"fullName\":\"" << jsonEscape(name) << "\","
           << "\"email\":\"" << jsonEscape(email) << "\","
           << "\"role\":\"Member\""
           << "}}";
        res.set_content(os.str(), "application/json; charset=utf-8");
    });
    // 2. Lay du lieu theo dung quyen cua tai khoan dang dang nhap
    svr.Get("/api/state", [](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lock(g_mtx);
        User* actor = getActor(req);
        if (!actor) {
            res.status = 401;
            res.set_content("{\"ok\":false,\"message\":\"Chua dang nhap!\"}", "application/json; charset=utf-8");
            return;
        }
        g_ctrl.syncReservations();
        res.set_content(buildStateJson(actor), "application/json; charset=utf-8");
    });

    // 3. Quan ly kho sach (Chi Librarian & Administrator)
    svr.Post("/api/books/add", [](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lock(g_mtx);
        User* actor = getActor(req);
        if (!actor || (actor->getRole() != "Librarian" && actor->getRole() != "Administrator")) {
            return sendForbidden(res, "Tu choi: Chi Librarian hoac Administrator moi co quyen them sach!");
        }
        string msg = g_ctrl.addBookAndCopy(
            req.get_param_value("bookId"),
            req.get_param_value("title"),
            req.get_param_value("author"),
            req.get_param_value("copyId"),
            req.get_param_value("barcode")
        );
        res.set_content("{\"ok\":true,\"message\":\"" + jsonEscape(msg) + "\"}", "application/json; charset=utf-8");
    });

    svr.Post("/api/books/update", [](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lock(g_mtx);
        User* actor = getActor(req);
        if (!actor || (actor->getRole() != "Librarian" && actor->getRole() != "Administrator")) {
            return sendForbidden(res, "Tu choi: Chi Librarian hoac Administrator moi co quyen sua sach!");
        }
        string msg = g_ctrl.updateBook(
            req.get_param_value("bookId"),
            req.get_param_value("title"),
            req.get_param_value("author")
        );
        res.set_content("{\"ok\":true,\"message\":\"" + jsonEscape(msg) + "\"}", "application/json; charset=utf-8");
    });

    svr.Post("/api/books/delete", [](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lock(g_mtx);
        User* actor = getActor(req);
        if (!actor || (actor->getRole() != "Librarian" && actor->getRole() != "Administrator")) {
            return sendForbidden(res, "Tu choi: Chi Librarian hoac Administrator moi co quyen xoa sach!");
        }
        string msg = g_ctrl.deleteBook(req.get_param_value("bookId"));
        res.set_content("{\"ok\":true,\"message\":\"" + jsonEscape(msg) + "\"}", "application/json; charset=utf-8");
    });

    svr.Post("/api/copies/delete", [](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lock(g_mtx);
        User* actor = getActor(req);
        if (!actor || (actor->getRole() != "Librarian" && actor->getRole() != "Administrator")) {
            return sendForbidden(res, "Tu choi: Chi Librarian hoac Administrator moi co quyen xoa ban sao sach!");
        }
        string msg = g_ctrl.deleteCopy(req.get_param_value("copyId"));
        res.set_content("{\"ok\":true,\"message\":\"" + jsonEscape(msg) + "\"}", "application/json; charset=utf-8");
    });
// 4. Muon - Tra - Gia han - Dat cho - Thu phat
    // Cho muon: chi Librarian & Administrator
    svr.Post("/api/circulation/borrow", [](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lock(g_mtx);
        User* actor = getActor(req);
        if (!actor || (actor->getRole() != "Librarian" && actor->getRole() != "Administrator")) {
            return sendForbidden(res, "Tu choi: Doc gia khong the tu tao phieu muon. Vui long lien he Thu thu!");
        }
        string dateStr = req.has_param("date") ? req.get_param_value("date") : req.get_param_value("day");
        string msg = g_ctrl.issueLoan(
            req.get_param_value("memberId"),
            req.get_param_value("copyId"),
            actor->getUserId(),
            dateStr
        );
        res.set_content("{\"ok\":true,\"message\":\"" + jsonEscape(msg) + "\"}", "application/json; charset=utf-8");
    });
