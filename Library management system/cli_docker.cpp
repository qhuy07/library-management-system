#include <iostream>
#include <limits>
#include "Model.h"
#include "Controller.h"

using namespace std;

int main() {
    LibraryDatabase db;
    LibraryController ctrl(db);

    cout << "====================================================\n";
    cout << "  DIAMOND LIBRARY (DOCKER LINUX CONTAINER)\n";
    cout << "  Thoi gian he thong: " << DateUtils::nowDateTime() << "\n";
    cout << "====================================================\n";

    User* currentUser = nullptr;
    while (!currentUser) {
        cout << "\n1. Dang nhap (Login)\n";
        cout << "2. Dang ky tai khoan moi (Register Member)\n";
        cout << "0. Thoat\n";
        cout << "Chon: ";
        int authChoice;
        if (!(cin >> authChoice) || authChoice == 0) return 0;

        if (authChoice == 2) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            string fullName, email, pass, confirm;
            cout << "\n--- DANG KY TAI KHOAN DOC GIA MOI ---\n";
            cout << "Ho va ten: ";
            getline(cin, fullName);
            cout << "Email (@ut.edu.vn): ";
            getline(cin, email);
            cout << "Mat khau (it nhat 3 ky tu): ";
            getline(cin, pass);
            cout << "Xac nhan mat khau: ";
            getline(cin, confirm);

            if (pass != confirm) {
                cout << "Error: Confirm password not match!\n";
                continue;
            }
            string msg, newId;
            if (ctrl.registerMember(fullName, email, pass, msg, newId)) {
                cout << msg << "\nBan co the dang nhap ngay bay gio.\n";
            } else {
                cout << msg << "\n";
            }
        } else if (authChoice == 1) {
            string email, pass;
            cout << "\nEmail (long@ut.edu.vn / admin@ut.edu.vn): ";
            cin >> email;
            cout << "Password (123): ";
            cin >> pass;

            currentUser = ctrl.login(email, pass);
            if (!currentUser) {
                cout << "Log in fail! Wrong email or password.\n";
            }
        }
    }

    cout << "\nXin chao " << currentUser->getFullName() << " [" << currentUser->getRole() << "]\n";

    while (true) {
        cout << "\n--- MENU CHUC NANG (" << DateUtils::today() << ") ---\n";
        cout << "1. Xem danh muc sach (Catalogue)\n";
        cout << "2. Cho muon sach (Borrow)\n";
        cout << "3. Nhan tra & Tinh phat (Return & Fine)\n";
        cout << "4. Xem bao cao kiem toan (Reports)\n";
        cout << "5. Xem & Sua thong tin ca nhan (Profile)\n";
        cout << "0. Thoat\nChon: ";
        int choice;
        if (!(cin >> choice) || choice == 0) break;

        if (choice == 1) cout << "\n" << ctrl.searchBook("");
        else if (choice == 2) {
            string m, c;
            cout << "MemberID (VD: U03): "; cin >> m;
            cout << "CopyID (VD: C01): "; cin >> c;
            cout << ctrl.issueLoan(m, c, currentUser->getUserId(), DateUtils::today()) << "\n";
        } else if (choice == 3) {
            string l, d;
            cout << "LoanID (VD: L01): "; cin >> l;
            cout << "Ngay tra (YYYY-MM-DD, nhap 'today' de lay hom nay): "; cin >> d;
            if (d == "today") d = DateUtils::today();
            cout << ctrl.processReturn(l, d) << "\n";
        } else if (choice == 4) cout << "\n" << ctrl.exportAuditReport();
        else if (choice == 5) {
            cout << "\n" << ctrl.getUserProfile(currentUser->getUserId()) << "\n";
            cout << "Ban co muon sua thong tin khong? (y/n): ";
            char ans;
            cin >> ans;
            if (ans == 'y' || ans == 'Y') {
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                string newName, newEmail, newPass, msg;
                cout << "Ho va ten moi (de trong neu giu nguyen): ";
                getline(cin, newName);
                cout << "Email moi (de trong neu giu nguyen): ";
                getline(cin, newEmail);
                cout << "Mat khau moi (de trong neu giu nguyen): ";
                getline(cin, newPass);

                ctrl.updateUserProfile(currentUser->getUserId(), newName, newEmail, newPass, msg);
                cout << msg << "\n";
            }
        }
    }
    return 0;
}
