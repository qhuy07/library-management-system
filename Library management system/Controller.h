#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "Model.h"

class LibraryController {
private:
    LibraryDatabase& db;

public:
    LibraryController(LibraryDatabase& database) : db(database) {}

    User* login(const string& email, const string& pwd) {
        for (auto& a : db.admins)     if (a.login(email, pwd)) return &a;
        for (auto& l : db.librarians) if (l.login(email, pwd)) return &l;
        for (auto& m : db.members)    if (m.login(email, pwd)) return &m;
        return nullptr;
    }

    bool emailExists(const string& email) const {
        for (const auto& a : db.admins)     if (a.getEmail() == email) return true;
        for (const auto& l : db.librarians) if (l.getEmail() == email) return true;
        for (const auto& m : db.members)    if (m.getEmail() == email) return true;
        return false;
    }

    string generateNextUserId() const {
        int maxNum = 0;
        auto checkId = [&](const string& id) {
            if (id.size() >= 2 && (id[0] == 'U' || id[0] == 'u')) {
                try {
                    int num = stoi(id.substr(1));
                    if (num > maxNum) maxNum = num;
                } catch (...) {}
            }
        };
        for (const auto& a : db.admins)     checkId(a.getUserId());
        for (const auto& l : db.librarians) checkId(l.getUserId());
        for (const auto& m : db.members)    checkId(m.getUserId());
        int nextNum = maxNum + 1;
        return (nextNum < 10 ? "U0" : "U") + to_string(nextNum);
    }

    bool registerMember(const string& name, const string& email, const string& password, string& outMessage, string& outUserId) {
        if (name.empty() || email.empty() || password.empty()) {
            outMessage = "Loi: Vui long nhap day du Ho ten, Email va Mat khau!";
            return false;
        }
        if (name.find(',') != string::npos || email.find(',') != string::npos || password.find(',') != string::npos) {
            outMessage = "Error: The information must not contain commas (,).";
            return false;
        }
        if (email.find('@') == string::npos || email.find('.') == string::npos) {
            outMessage = "Loi: Dinh dang Email khong hop le!";
            return false;
        }
        if (password.size() < 3) {
            outMessage = "Error: Password at least 3 characters";
            return false;
        }
        if (emailExists(email)) {
            outMessage = "Error: Email '" + email + "' already register in the system!";
            return false;
        }

        string newId = generateNextUserId();
        string cardNo = "CARD-" + newId;
        db.members.push_back(Member(newId, name, email, password, cardNo, 3, 0, 0.0));
        db.saveAll();

        outUserId = newId;
        outMessage = "[UC: Register] register successful! ID member: " + newId + " (" + email + ")";
        return true;
    }

    string createStaffOrMember(const string& role, const string& id, const string& name, const string& email) {
        if (id.empty() || name.empty() || email.empty()) return "Loi: Nhap day du ID, Ho ten va Email!";
        if (emailExists(email)) return "Loi: Email '" + email + "' da ton tai trong he thong!";
        if (role == "Librarian") {
            db.librarians.push_back(Librarian(id, name, email, "123", "STF-" + id, "Afternoon"));
        } else {
            db.members.push_back(Member(id, name, email, "123", "CARD-" + id, 3, 0, 0.0));
        }
        db.saveAll();
        return "[Use Case: Manage Accounts] Da tao tai khoan " + role + ": " + name + " (" + email + ")";
    }

    string getUserProfile(const string& userId) const {
        for (const auto& a : db.admins) {
            if (a.getUserId() == userId) {
                return "THONG TIN TAI KHOAN (Administrator)\r\n"
                       "------------------------------------\r\n"
                       "- Ma TK (ID)   : " + a.getUserId() + "\r\n"
                       "- Ho va ten    : " + a.getFullName() + "\r\n"
                       "- Email        : " + a.getEmail() + "\r\n"
                       "- Vai tro      : " + a.getRole() + "\r\n"
                       "- Trang thai   : " + a.getStatus() + "\r\n"
                       "- Cap do Admin : " + to_string(a.adminLevel) + "\r\n";
            }
        }
        for (const auto& l : db.librarians) {
            if (l.getUserId() == userId) {
                return "THONG TIN TAI KHOAN (Librarian)\r\n"
                       "------------------------------------\r\n"
                       "- Ma TK (ID)   : " + l.getUserId() + "\r\n"
                       "- Ho va ten    : " + l.getFullName() + "\r\n"
                       "- Email        : " + l.getEmail() + "\r\n"
                       "- Vai tro      : " + l.getRole() + "\r\n"
                       "- Trang thai   : " + l.getStatus() + "\r\n"
                       "- Ma nhan vien : " + l.staffCode + "\r\n"
                       "- Ca truc      : " + l.deskShift + "\r\n";
            }
        }
        for (const auto& m : db.members) {
            if (m.getUserId() == userId) {
                return "THONG TIN TAI KHOAN (Member)\r\n"
                       "------------------------------------\r\n"
                       "- Ma TK (ID)   : " + m.getUserId() + "\r\n"
                       "- Ho va ten    : " + m.getFullName() + "\r\n"
                       "- Email        : " + m.getEmail() + "\r\n"
                       "- Vai tro      : " + m.getRole() + "\r\n"
                       "- Trang thai   : " + m.getStatus() + "\r\n"
                       "- So the       : " + m.membershipCardNo + "\r\n"
                       "- Dang muon    : " + to_string(m.activeLoansCount) + "/" + to_string(m.maxQuota) + "\r\n"
                       "- No phat      : " + to_string((int)m.totalUnpaidFines) + " VND\r\n";
            }
        }
        return "Khong tim thay nguoi dung co ID: " + userId;
    }

    bool updateUserProfile(const string& userId, const string& newName, const string& newEmail, const string& newPassword, string& outMessage) {
        if (newName.find(',') != string::npos || newEmail.find(',') != string::npos || newPassword.find(',') != string::npos) {
            outMessage = "Error: The information must not contain commas (,).";
            return false;
        }
        if (!newEmail.empty() && (newEmail.find('@') == string::npos || newEmail.find('.') == string::npos)) {
            outMessage = "Loi: Dinh dang Email khong hop le!";
            return false;
        }
        if (!newPassword.empty() && newPassword.size() < 3) {
            outMessage = "Loi: Mat khau moi phai co it nhat 3 ky tu!";
            return false;
        }

        auto applyUpdate = [&](User& u) -> bool {
            if (!newEmail.empty() && newEmail != u.getEmail() && emailExists(newEmail)) {
                outMessage = "Loi: Email '" + newEmail + "' da duoc su dung boi tai khoan khac!";
                return false;
            }
            if (!newName.empty())     u.updateProfile(newName);
            if (!newEmail.empty())    u.updateEmail(newEmail);
            if (!newPassword.empty()) u.updatePassword(newPassword);
            db.saveAll();
            outMessage = "[UC: Update Profile] Cap nhat thong tin thanh cong cho " + u.getUserId() + " (" + u.getFullName() + ")";
            return true;
        };

        for (auto& a : db.admins)     if (a.getUserId() == userId) return applyUpdate(a);
        for (auto& l : db.librarians) if (l.getUserId() == userId) return applyUpdate(l);
        for (auto& m : db.members)    if (m.getUserId() == userId) return applyUpdate(m);

        outMessage = "Loi: Khong tim thay tai khoan " + userId;
        return false;
    }

    string searchBook(const string& kw) {
        string res = "DANH MUC DAU SACH & BAN SAO VAT LY (Book & BookCopy):\r\n"
                     "========================================================================\r\n";
        for (const auto& b : db.books) {
            if (kw.empty() || b.title.find(kw) != string::npos || b.bookId == kw || b.author.find(kw) != string::npos) {
                res += "[" + b.bookId + "] " + b.title + " - Tac gia: " + b.author + " (ISBN: " + b.isbn + ")\r\n";
                for (const auto& c : db.bookCopies) {
                    if (c.bookId == b.bookId) {
                        res += "    -> Ban sao: " + c.copyId + " | Barcode: " + c.barcode + " | Ke: " + c.rackLocation + " | Trang thai: " + c.copyStatus + "\r\n";
                    }
                }
            }
        }
        return res;
    }

    string addBookAndCopy(const string& bookId, const string& title, const string& author, const string& copyId, const string& barcode) {
        if (bookId.empty() || title.empty() || copyId.empty()) return "Loi: Nhap BookID, Ten sach va CopyID!";
        bool bookExists = false;
        for (const auto& b : db.books) if (b.bookId == bookId) bookExists = true;
        if (!bookExists) {
            db.books.push_back({bookId, "978-NEW", title, author, "NXB Tre", 2026, "CAT01"});
        }
        db.bookCopies.push_back({copyId, barcode.empty() ? "NF-NEW" : barcode, "A2-01", "Available", bookId});
        db.saveAll();
        return "[UC: Add Book] Da them sach '" + title + "' va ban sao '" + copyId + "'.";
    }

    string updateBook(const string& bookId, const string& newTitle, const string& newAuthor) {
        for (auto& b : db.books) {
            if (b.bookId == bookId) {
                if (!newTitle.empty()) b.title = newTitle;
                if (!newAuthor.empty()) b.author = newAuthor;
                db.saveAll();
                return "[UC: Update Book] Da cap nhat thong tin cho BookID: " + bookId;
            }
        }
        return "[UC: Update Book] Khong tim thay BookID: " + bookId;
    }

    string deleteBook(const string& bookId) {
        for (size_t i = 0; i < db.books.size(); i++) {
            if (db.books[i].bookId == bookId) {
                db.books.erase(db.books.begin() + i);
                db.saveAll();
                return "[UC: Delete Book] Da xoa dau sach BookID: " + bookId;
            }
        }
        return "[UC: Delete Book] Khong tim thay BookID: " + bookId;
    }

    string deleteCopy(const string& copyId) {
        if (copyId.empty()) return "[UC: Delete Copy] Loi: Vui long nhap CopyID can xoa!";
        bool foundAny = false;
        string busyStatus = "";
        for (size_t i = 0; i < db.bookCopies.size(); i++) {
            if (db.bookCopies[i].copyId == copyId) {
                foundAny = true;
                if (!db.bookCopies[i].isAvailable()) {
                    busyStatus = db.bookCopies[i].copyStatus;
                    continue;
                }
                string bId = db.bookCopies[i].bookId;
                db.bookCopies.erase(db.bookCopies.begin() + i);
                db.saveAll();
                return "[UC: Delete Copy] Da xoa ban sao CopyID: " + copyId + " (thuoc sach " + bId + ").";
            }
        }
        if (foundAny) {
            return "[UC: Delete Copy] Khong the xoa ban sao " + copyId + " vi dang o trang thai " + busyStatus + "!";
        }
        return "[UC: Delete Copy] Khong tim thay ban sao CopyID: " + copyId;
    }
// Ham noi bo: tu dong chuyen ban sao cho nguoi Pending som nhat cua dau sach do (FIFO)
    bool assignCopyToNextPending(const string& bookId, const string& copyId, const string& baseDate, string& assignedInfo) {
        for (auto& r : db.reservations) {
            if (r.bookId == bookId && r.status == "Pending") {
                string newExpiry = DateUtils::addDays(baseDate, 3);
                r.markReady(copyId, newExpiry);
                for (auto& c : db.bookCopies) {
                    if (c.copyId == copyId) { c.markReserved(); break; }
                }
                assignedInfo = r.reservationId + " (Member: " + r.memberId + ", Han giu: " + newExpiry + ")";
                return true;
            }
        }
        return false;
    }

    // Tu dong quet va dong bo cac phieu dat cho da qua han
    void syncReservations() {
        string todayStr = DateUtils::today();
        bool changed = false;

        for (auto& r : db.reservations) {
            if ((r.status == "Pending" || r.status == "Ready") &&
                DateUtils::diffDays(r.expiryDate, todayStr) > 0) {
                string releasedCopy = r.copyId;
                r.markExpired();
                changed = true;

                if (releasedCopy != "NONE") {
                    string nextInfo;
                    if (!assignCopyToNextPending(r.bookId, releasedCopy, todayStr, nextInfo)) {
                        for (auto& c : db.bookCopies) {
                            if (c.copyId == releasedCopy && c.isReserved()) {
                                c.markReturned();
                                break;
                            }
                        }
                    }
                }
            }
        }
        if (changed) db.saveAll();
    }
    string issueLoan(const string& memberId, const string& copyId, const string& librarianId, const string& issueDateInput = "") {
        syncReservations();

        Member* mem = nullptr;
        for (auto& m : db.members) if (m.getUserId() == memberId) mem = &m;
        if (!mem) return "[UC: Borrow Book] Loi: Khong tim thay Member ID " + memberId;

        if (!mem->canBorrow()) {
            return "[Include: Validate Quota] Tu choi! Doc gia vuot han muc (" + to_string(mem->activeLoansCount) +
                   "/" + to_string(mem->maxQuota) + ") hoac dang no phat (" + to_string((int)mem->totalUnpaidFines) + " d).";
        }

        string issueDate = DateUtils::isValidDate(issueDateInput) ? issueDateInput : DateUtils::today();
        string dueDate   = DateUtils::addDays(issueDate, db.defaultLoanDays);

        for (auto& c : db.bookCopies) {
            if (c.copyId == copyId) {
                if (c.isReserved()) {
                    bool reservedForMe = false;
                    for (const auto& r : db.reservations) {
                        if (r.status == "Ready" && r.copyId == c.copyId && r.memberId == memberId) {
                            reservedForMe = true;
                            break;
                        }
                    }
                    if (!reservedForMe) {
                        return "[UC: Borrow Book] Ban sao " + copyId + " dang duoc giu (Reserved) cho doc gia khac!";
                    }
                } else if (!c.isAvailable()) {
                    return "[UC: Borrow Book] Ban sao " + copyId + " dang ban (" + c.copyStatus + ").";
                }

                c.markBorrowed();
                mem->activeLoansCount++;

                // Tu dong hoan tat phieu dat cho (neu Member dang dat cuon sach nay)
                string holdSyncMsg = "";
                for (auto& r : db.reservations) {
                    if (r.memberId == memberId && r.bookId == c.bookId &&
                        (r.status == "Ready" || r.status == "Pending") &&
                        (r.copyId == c.copyId || r.copyId == "NONE")) {
                        r.copyId = c.copyId;
                        r.fulfillHold();
                        holdSyncMsg = " | Da dong bo phieu dat cho " + r.reservationId + " -> Fulfilled";
                        break;
                    }
                }

                int nextLoanNum = (int)db.loans.size() + 1;
                string loanId = (nextLoanNum < 10 ? "L0" : "L") + to_string(nextLoanNum);
                db.loans.push_back({loanId, memberId, librarianId, copyId, issueDate, dueDate, "NONE", 0, "Active"});
                db.saveAll();

                return "[UC: Borrow Book] Thanh cong! Ma phieu: " + loanId + " | Ngay muon: " + issueDate +
                       " | Han tra: " + dueDate + holdSyncMsg;
            }
        }
        return "[UC: Borrow Book] Khong tim thay ban sao CopyID: " + copyId;
    }

    string processReturn(const string& loanId, const string& returnDateInput = "") {
        syncReservations();
        string returnDate = DateUtils::isValidDate(returnDateInput) ? returnDateInput : DateUtils::today();

        for (auto& l : db.loans) {
            if (l.loanId == loanId && l.status != "Returned") {
                l.returnDate = returnDate;
                l.status = "Returned";

                string holdNotice = "";
                for (auto& c : db.bookCopies) {
                    if (c.copyId == l.copyId) {
                        string assignedInfo;
                        if (assignCopyToNextPending(c.bookId, c.copyId, returnDate, assignedInfo)) {
                            holdNotice = " | [GIU TAI QUAY] Chuyen ban sao " + c.copyId + " sang Reserved cho phieu " + assignedInfo;
                        } else {
                            c.markReturned();
                        }
                        break;
                    }
                }

                Member* mem = nullptr;
                for (auto& m : db.members) if (m.getUserId() == l.memberId) { m.activeLoansCount--; mem = &m; }

                int overdueDays = l.calculateOverdueDays(returnDate);
                if (overdueDays > 0) {
                    int nextFineNum = (int)db.fines.size() + 1;
                    string fineId = (nextFineNum < 10 ? "F0" : "F") + to_string(nextFineNum);
                    Fine f{fineId, overdueDays, 0.0, "Unpaid", "NONE", l.loanId};
                    f.computeFee(db.fineRatePerDay);
                    db.fines.push_back(f);
                    if (mem) mem->totalUnpaidFines += f.amount;
                    db.saveAll();
                    return "[Extend: Calculate Fine] Tra ngay " + returnDate + " (tre " + to_string(overdueDays) +
                           " ngay)! Phat sinh phieu phat " + fineId + ": " + to_string((int)f.amount) + " VND." + holdNotice;
                }

                db.saveAll();
                return "[UC: Return Book] Tra sach dung han thanh cong ngay " + returnDate + " cho phieu " + loanId + holdNotice;
            }
        }
        return "[UC: Return Book] Khong tim thay phieu muon dang mo: " + loanId;
    }

    string collectFine(const string& fineId) {
        for (auto& f : db.fines) {
            if (f.fineId == fineId && f.paidStatus == "Unpaid") {
                f.recordPayment(DateUtils::today());
                for (const auto& l : db.loans) {
                    if (l.loanId == f.loanId) {
                        for (auto& m : db.members) if (m.getUserId() == l.memberId) m.totalUnpaidFines -= f.amount;
                    }
                }
                db.saveAll();
                return "[UC: Collect Fine] Da thu tien phat & xoa no cho phieu: " + fineId + " (Ngay: " + f.paymentDate + ")";
            }
        }
        return "[UC: Collect Fine] Khong tim thay phieu phat chua thanh toan: " + fineId;
    }
string reserveBook(const string& memberId, const string& bookId) {
        syncReservations();

        bool memberExists = false;
        for (const auto& m : db.members) if (m.getUserId() == memberId) { memberExists = true; break; }
        if (!memberExists) return "[UC: Hold Book] Loi: Khong tim thay MemberID: " + memberId;

        const Book* targetBook = nullptr;
        for (const auto& b : db.books) if (b.bookId == bookId) { targetBook = &b; break; }
        if (!targetBook) return "[UC: Hold Book] Khong tim thay BookID: " + bookId;

        for (const auto& r : db.reservations) {
            if (r.memberId == memberId && r.bookId == bookId &&
                (r.status == "Pending" || r.status == "Ready")) {
                return "[UC: Hold Book] Ban da co phieu dat cho (" + r.reservationId + " - " + r.status + ") cho sach nay!";
            }
        }

        string todayStr  = DateUtils::today();
        string expiryStr = DateUtils::addDays(todayStr, 3);
        int nextNum = (int)db.reservations.size() + 1;
        string resId = (nextNum < 10 ? "R0" : "R") + to_string(nextNum);

        db.reservations.push_back({resId, todayStr, expiryStr, "Pending", memberId, bookId, "NONE"});
        db.saveAll();
        return "[UC: Hold Book] Dat cho thanh cong! Ma: " + resId + " (Sach: " + targetBook->title +
               " | Trang thai: Pending - Cho Thu thu xac nhan | Han: " + expiryStr + ")";
    }

    string approveReservation(const string& reservationId) {
        syncReservations();
        string todayStr = DateUtils::today();

        for (auto& r : db.reservations) {
            if (r.reservationId == reservationId) {
                if (r.status != "Pending") {
                    return "[UC: Approve Hold] Khong the duyet phieu " + reservationId + " (Trang thai hien tai: " + r.status + ").";
                }
                for (auto& c : db.bookCopies) {
                    if (c.bookId == r.bookId && c.isAvailable()) {
                        string newExpiry = DateUtils::addDays(todayStr, 3);
                        c.markReserved();
                        r.markReady(c.copyId, newExpiry);
                        db.saveAll();
                        return "[UC: Approve Hold] Da duyet phieu " + reservationId + "! Giu ban sao " +
                               c.copyId + " cho Member " + r.memberId + " den het ngay " + newExpiry + ".";
                    }
                }
                return "[UC: Approve Hold] Hien chua co ban sao Available nao cho sach " + r.bookId +
                       ". Phieu se tu dong chuyen sang Ready ngay khi co nguoi tra sach.";
            }
        }
        return "[UC: Approve Hold] Khong tim thay ma dat cho: " + reservationId;
    }

    string fulfillReservation(const string& reservationId, const string& librarianId, const string& issueDateInput = "") {
        syncReservations();

        for (auto& r : db.reservations) {
            if (r.reservationId == reservationId) {
                if (r.status != "Ready" && r.status != "Pending") {
                    return "[UC: Fulfill Hold] Phieu " + reservationId + " khong con hieu luc (" + r.status + ").";
                }
                string targetCopyId = r.copyId;
                if (targetCopyId == "NONE") {
                    for (const auto& c : db.bookCopies) {
                        if (c.bookId == r.bookId && c.isAvailable()) {
                            targetCopyId = c.copyId;
                            break;
                        }
                    }
                }
                if (targetCopyId == "NONE") {
                    return "[UC: Fulfill Hold] Chua co ban sao nao san sang de giao cho phieu " + reservationId + ".";
                }
                return issueLoan(r.memberId, targetCopyId, librarianId, issueDateInput);
            }
        }
        return "[UC: Fulfill Hold] Khong tim thay ma dat cho: " + reservationId;
    }

    string cancelReservation(const string& reservationId, const string& actorId, const string& actorRole) {
        syncReservations();
        string todayStr = DateUtils::today();

        for (auto& r : db.reservations) {
            if (r.reservationId == reservationId) {
                if (actorRole == "Member" && r.memberId != actorId) {
                    return "[UC: Cancel Hold] Tu choi: Ban chi duoc huy phieu dat cho cua chinh minh!";
                }
                if (r.status != "Pending" && r.status != "Ready") {
                    return "[UC: Cancel Hold] Phieu " + reservationId + " da o trang thai " + r.status + ", khong the huy.";
                }

                string releasedCopy = r.copyId;
                r.cancelHold();

                string extraMsg = "";
                if (releasedCopy != "NONE") {
                    string nextInfo;
                    if (assignCopyToNextPending(r.bookId, releasedCopy, todayStr, nextInfo)) {
                        extraMsg = " | Ban sao " + releasedCopy + " da duoc chuyen sang giu cho phieu ke tiep: " + nextInfo;
                    } else {
                        for (auto& c : db.bookCopies) {
                            if (c.copyId == releasedCopy && c.isReserved()) {
                                c.markReturned();
                                extraMsg = " | Ban sao " + releasedCopy + " da tro lai trang thai Available.";
                                break;
                            }
                        }
                    }
                }

                db.saveAll();
                return "[UC: Cancel Hold] Da huy phieu dat cho " + reservationId + extraMsg;
            }
        }
        return "[UC: Cancel Hold] Khong tim thay ma dat cho: " + reservationId;
    }

    string renewLoan(const string& loanId) {
        for (auto& l : db.loans) {
            if (l.loanId == loanId) {
                if (l.renewLoan(7)) {
                    db.saveAll();
                    return "[UC: Renew Book] Gia han thanh cong them 7 ngay! Han tra moi: " + l.dueDate;
                }
                return "[UC: Renew Book] Khong the gia han (Da qua 2 lan hoac phieu da dong).";
            }
        }
        return "[UC: Renew Book] Khong tim thay LoanID: " + loanId;
    }

    string updateSystemPolicy(int newLoanDays, double newFineRate) {
        if (newLoanDays > 0) db.defaultLoanDays = newLoanDays;
        if (newFineRate > 0) db.fineRatePerDay = newFineRate;
        return "[UC: Update Policy] Da cap nhat quy dinh: Muon toi da " + to_string(db.defaultLoanDays) +
               " ngay | Phat tre: " + to_string((int)db.fineRatePerDay) + " VND/ngay.";
    }

    string exportAuditReport() {
        double collected = 0, unpaid = 0;
        for (const auto& f : db.fines) {
            if (f.paidStatus == "Paid") collected += f.amount;
            else unpaid += f.amount;
        }
        return "BAO CAO KIEM TOAN HE THONG (AUDIT REPORT - DFD 5.0)\r\n"
               "Thoi diem xuat bao cao        : " + DateUtils::nowDateTime() + "\r\n"
               "====================================================\r\n"
               "- Tong dau sach (Book)        : " + to_string(db.books.size()) + "\r\n"
               "- Tong ban sao vat ly (Copy)  : " + to_string(db.bookCopies.size()) + "\r\n"
               "- Tong phieu muon (Loan)      : " + to_string(db.loans.size()) + "\r\n"
               "- Tong phieu dat cho (Hold)   : " + to_string(db.reservations.size()) + "\r\n"
               "- Tien phat da thu            : " + to_string((int)collected) + " VND\r\n"
               "- Tien phat chua thu          : " + to_string((int)unpaid) + " VND\r\n";
    }
};

#endif
