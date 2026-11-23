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
