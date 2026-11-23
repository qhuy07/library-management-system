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
// Dat giu cho: tat ca role (Member chi duoc dat cho chinh minh)
    svr.Post("/api/circulation/hold", [](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lock(g_mtx);
        User* actor = getActor(req);
        if (!actor) return sendForbidden(res, "Chua dang nhap!");
        string targetMember = (actor->getRole() == "Member")
            ? actor->getUserId()
            : req.get_param_value("memberId");
        string msg = g_ctrl.reserveBook(targetMember, req.get_param_value("bookId"));
        res.set_content("{\"ok\":true,\"message\":\"" + jsonEscape(msg) + "\"}", "application/json; charset=utf-8");
    });

    // Librarian/Admin duyet dat cho (Pending -> Ready, giu ban sao sang Reserved)
    svr.Post("/api/circulation/hold/approve", [](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lock(g_mtx);
        User* actor = getActor(req);
        if (!actor || (actor->getRole() != "Librarian" && actor->getRole() != "Administrator")) {
            return sendForbidden(res, "Tu choi: Chi Thu thu hoac Admin moi co quyen duyet dat cho!");
        }
        string msg = g_ctrl.approveReservation(req.get_param_value("reservationId"));
        res.set_content("{\"ok\":true,\"message\":\"" + jsonEscape(msg) + "\"}", "application/json; charset=utf-8");
    });

    // Librarian/Admin giao sach tu phieu dat cho (Ready/Pending -> Fulfilled + tao Loan)
    svr.Post("/api/circulation/hold/fulfill", [](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lock(g_mtx);
        User* actor = getActor(req);
        if (!actor || (actor->getRole() != "Librarian" && actor->getRole() != "Administrator")) {
            return sendForbidden(res, "Tu choi: Chi Thu thu hoac Admin moi co quyen giao sach dat cho!");
        }
        string dateStr = req.has_param("date") ? req.get_param_value("date") : "";
        string msg = g_ctrl.fulfillReservation(req.get_param_value("reservationId"), actor->getUserId(), dateStr);
        res.set_content("{\"ok\":true,\"message\":\"" + jsonEscape(msg) + "\"}", "application/json; charset=utf-8");
    });

    // Member tu huy phieu cua minh HOAC Librarian/Admin huy bat ky phieu nao
    svr.Post("/api/circulation/hold/cancel", [](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lock(g_mtx);
        User* actor = getActor(req);
        if (!actor) return sendForbidden(res, "Chua dang nhap!");
        string msg = g_ctrl.cancelReservation(
            req.get_param_value("reservationId"),
            actor->getUserId(),
            actor->getRole()
        );
        res.set_content("{\"ok\":true,\"message\":\"" + jsonEscape(msg) + "\"}", "application/json; charset=utf-8");
    });
// Nhan tra sach: chi Librarian & Administrator
    svr.Post("/api/circulation/return", [](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lock(g_mtx);
        User* actor = getActor(req);
        if (!actor || (actor->getRole() != "Librarian" && actor->getRole() != "Administrator")) {
            return sendForbidden(res, "Tu choi: Chi Thu thu hoac Admin moi co quyen nhan tra sach!");
        }
        string dateStr = req.has_param("date") ? req.get_param_value("date") : req.get_param_value("day");
        string msg = g_ctrl.processReturn(req.get_param_value("loanId"), dateStr);
        res.set_content("{\"ok\":true,\"message\":\"" + jsonEscape(msg) + "\"}", "application/json; charset=utf-8");
    });
 // Gia han: tat ca role (Member chi duoc gia han phieu cua chinh minh)
    svr.Post("/api/circulation/renew", [](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lock(g_mtx);
        User* actor = getActor(req);
        if (!actor) return sendForbidden(res, "Chua dang nhap!");
        string loanId = req.get_param_value("loanId");
        if (actor->getRole() == "Member") {
            bool ownsLoan = false;
            for (const auto& l : g_db.loans) {
                if (l.loanId == loanId && l.memberId == actor->getUserId()) { ownsLoan = true; break; }
            }
            if (!ownsLoan) {
                return sendForbidden(res, "Tu choi: Ban chi duoc gia han phieu muon cua chinh minh!");
            }
        }
        string msg = g_ctrl.renewLoan(loanId);
        res.set_content("{\"ok\":true,\"message\":\"" + jsonEscape(msg) + "\"}", "application/json; charset=utf-8");
    });
// Thu tien phat: chi Librarian & Administrator
    svr.Post("/api/circulation/pay-fine", [](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lock(g_mtx);
        User* actor = getActor(req);
        if (!actor || (actor->getRole() != "Librarian" && actor->getRole() != "Administrator")) {
            return sendForbidden(res, "Tu choi: Chi Thu thu hoac Admin moi co quyen thu tien phat!");
        }
        string msg = g_ctrl.collectFine(req.get_param_value("fineId"));
        res.set_content("{\"ok\":true,\"message\":\"" + jsonEscape(msg) + "\"}", "application/json; charset=utf-8");
    });
 // 5. Quan ly tai khoan
    // Librarian chi duoc tao Member; Administrator tao duoc ca Member & Librarian
    svr.Post("/api/members/add", [](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lock(g_mtx);
        User* actor = getActor(req);
        if (!actor || actor->getRole() == "Member") {
            return sendForbidden(res, "Tu choi: Doc gia khong co quyen tao tai khoan!");
        }
        string targetRole = req.get_param_value("role");
        if (actor->getRole() == "Librarian" && targetRole != "Member") {
            return sendForbidden(res, "Tu choi: Thu thu chi duoc tao tai khoan Member. Chi Admin moi tao duoc Librarian!");
        }
        string newId = g_ctrl.generateNextUserId();
        string msg = g_ctrl.createStaffOrMember(
            targetRole,
            newId,
            req.get_param_value("name"),
            req.get_param_value("email")
        );
        res.set_content("{\"ok\":true,\"message\":\"" + jsonEscape(msg) + "\"}", "application/json; charset=utf-8");
    });
    // 6. Cap nhat chinh sach he thong (CHI Administrator)
    svr.Post("/api/policy/update", [](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lock(g_mtx);
        User* actor = getActor(req);
        if (!actor || actor->getRole() != "Administrator") {
            return sendForbidden(res, "Tu choi: Chi Administrator moi co quyen cau hinh chinh sach thu vien!");
        }
        int days = safeInt(req.get_param_value("maxDays"), 14);
        double rate = safeDouble(req.get_param_value("fineRate"), 5000.0);
        string msg = g_ctrl.updateSystemPolicy(days, rate);
        res.set_content("{\"ok\":true,\"message\":\"" + jsonEscape(msg) + "\"}", "application/json; charset=utf-8");
    });
   // 7. Xem thong tin nguoi dung dang dang nhap
    svr.Get("/api/profile", [](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lock(g_mtx);
        User* actor = getActor(req);
        if (!actor) {
            res.status = 401;
            res.set_content("{\"ok\":false,\"message\":\"Chua dang nhap!\"}", "application/json; charset=utf-8");
            return;
        }

        ostringstream os;
        os << "{\"ok\":true,\"user\":{"
           << "\"userId\":\"" << jsonEscape(actor->getUserId()) << "\","
           << "\"fullName\":\"" << jsonEscape(actor->getFullName()) << "\","
           << "\"email\":\"" << jsonEscape(actor->getEmail()) << "\","
           << "\"role\":\"" << jsonEscape(actor->getRole()) << "\","
           << "\"status\":\"" << jsonEscape(actor->getStatus()) << "\"";

        if (actor->getRole() == "Member") {
            for (const auto& m : g_db.members) {
                if (m.getUserId() == actor->getUserId()) {
                    os << ",\"membershipCardNo\":\"" << jsonEscape(m.membershipCardNo) << "\""
                       << ",\"maxQuota\":" << m.maxQuota
                       << ",\"activeLoansCount\":" << m.activeLoansCount
                       << ",\"totalUnpaidFines\":" << (long long)m.totalUnpaidFines;
                    break;
                }
            }
        } else if (actor->getRole() == "Librarian") {
            for (const auto& l : g_db.librarians) {
                if (l.getUserId() == actor->getUserId()) {
                    os << ",\"staffCode\":\"" << jsonEscape(l.staffCode) << "\""
                       << ",\"deskShift\":\"" << jsonEscape(l.deskShift) << "\"";
                    break;
                }
            }
        } else if (actor->getRole() == "Administrator") {
            for (const auto& a : g_db.admins) {
                if (a.getUserId() == actor->getUserId()) {
                    os << ",\"adminLevel\":" << a.adminLevel
                       << ",\"lastAuditDate\":\"" << jsonEscape(a.lastAuditDate) << "\"";
                    break;
                }
            }
        }
        os << "}}";
        res.set_content(os.str(), "application/json; charset=utf-8");
    });

    // 8. Sua thong tin nguoi dung (Tu sua thong tin cua minh, hoac Admin sua cho userId khac)
    svr.Post("/api/profile/update", [](const httplib::Request& req, httplib::Response& res) {
        lock_guard<mutex> lock(g_mtx);
        User* actor = getActor(req);
        if (!actor) return sendForbidden(res, "Chua dang nhap!");

        string targetId = req.has_param("targetUserId") && !req.get_param_value("targetUserId").empty()
            ? req.get_param_value("targetUserId")
            : actor->getUserId();

        if (targetId != actor->getUserId() && actor->getRole() != "Administrator") {
            return sendForbidden(res, "Tu choi: Ban chi co the sua thong tin cua chinh minh!");
        }

        string newName = req.get_param_value("fullName");
        string newEmail = req.get_param_value("email");
        string newPass = req.get_param_value("password");

        string msg;
        bool ok = g_ctrl.updateUserProfile(targetId, newName, newEmail, newPass, msg);
        if (!ok) {
            res.set_content("{\"ok\":false,\"message\":\"" + jsonEscape(msg) + "\"}", "application/json; charset=utf-8");
            return;
        }

        User* updated = findUserById(targetId);
        ostringstream os;
        os << "{\"ok\":true,\"message\":\"" << jsonEscape(msg) << "\",\"user\":{"
           << "\"userId\":\"" << jsonEscape(updated->getUserId()) << "\","
           << "\"fullName\":\"" << jsonEscape(updated->getFullName()) << "\","
           << "\"email\":\"" << jsonEscape(updated->getEmail()) << "\","
           << "\"role\":\"" << jsonEscape(updated->getRole()) << "\""
           << "}}";
        res.set_content(os.str(), "application/json; charset=utf-8");
    });

    cout << "Diamond Library Web Server (RBAC Enabled) running at http://0.0.0.0:8080\n";
    cout << "Server Time: " << DateUtils::nowDateTime() << "\n";
    svr.listen("0.0.0.0", 8080);
    return 0;
}

