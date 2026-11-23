#include <windows.h>
#include <vector>
#include "Model.h"
#include "Controller.h"
#include "GuiHelper.h"

#define ID_TAB_OVERVIEW    101
#define ID_TAB_CATALOGUE   102
#define ID_TAB_CIRCULATION 103
#define ID_TAB_MEMBERS     104
#define ID_TAB_REPORTS     105
#define ID_BTN_LOGOUT      106

#define ID_BTN_LOGIN       201
#define ID_BTN_SEARCH      202
#define ID_BTN_ADD_BOOK    203
#define ID_BTN_UPD_BOOK    204
#define ID_BTN_DEL_BOOK    205
#define ID_BTN_BORROW      206
#define ID_BTN_RETURN      207
#define ID_BTN_RENEW       208
#define ID_BTN_HOLD        209
#define ID_BTN_PAY_FINE    210
#define ID_BTN_ADD_ACC     211
#define ID_BTN_POLICY      212
#define ID_BTN_GO_REGISTER 213
#define ID_BTN_REGISTER    214
#define ID_BTN_BACK_LOGIN  215

LibraryDatabase g_db;
LibraryController g_ctrl(g_db);
User* g_user = nullptr;
int g_activeTab = ID_TAB_OVERVIEW;

vector<HWND> g_loginScreen;
vector<HWND> g_registerScreen;
vector<HWND> g_appShell;
vector<HWND> g_tabOverview;
vector<HWND> g_tabCatalogue;
vector<HWND> g_tabCirculation;
vector<HWND> g_tabMembers;
vector<HWND> g_tabReports;

HWND hLoginEmail, hLoginPass, hLoginStatus;
HWND hRegName, hRegEmail, hRegPass, hRegConfirm, hRegStatus;
HWND hHeaderPath, hUserFooter, hDateLabel;
HWND hStatLoans, hStatAvail, hStatOverdue, hStatMembers;
HWND hBoxRecent, hBoxAttention;
HWND hCatBookId, hCatTitle, hCatAuthor, hCatCopyId, hCatBarcode, hCatOutput;
HWND hCirMemberId, hCirCopyId, hCirBookId, hCirLoanId, hCirDay, hCirFineId, hCirOutput;
HWND hMemName, hMemEmail, hMemRole, hMemOutput;
HWND hRepMaxDays, hRepFineRate, hRepOutput;

string GetStr(HWND hw) {
    char buf[256];
    GetWindowTextA(hw, buf, sizeof(buf));
    return string(buf);
}

void SetGroupVisible(const vector<HWND>& list, bool show) {
    for (HWND h : list) ShowWindow(h, show ? SW_SHOW : SW_HIDE);
}

void RefreshOverviewData() {
    SetWindowTextA(hDateLabel, DateUtils::today().c_str());

    int avail = 0, borrowed = 0;
    for (const auto& c : g_db.bookCopies) {
        if (c.copyStatus == "Available") avail++;
        else borrowed++;
    }
    int overdue = 0;
    for (const auto& l : g_db.loans) {
        if (l.status == "Overdue" || l.isOverdue()) overdue++;
    }

    SetWindowTextA(hStatLoans,   (to_string(borrowed) + " ban sao dang muon").c_str());
    SetWindowTextA(hStatAvail,   (to_string(avail) + " ban sao san sang").c_str());
    SetWindowTextA(hStatOverdue, (to_string(overdue) + " phieu qua han").c_str());
    SetWindowTextA(hStatMembers, (to_string(g_db.members.size()) + " doc gia").c_str());

    string rec = "LOAN ID | COPY ID | MEMBER ID | HAN TRA    | TRANG THAI\r\n"
                 "---------------------------------------------------------\r\n";
    for (const auto& l : g_db.loans) {
        rec += l.loanId + "     | " + l.copyId + "     | " + l.memberId + "       | " + l.dueDate + " | " + l.status + "\r\n";
    }
    SetWindowTextA(hBoxRecent, rec.c_str());

    string att = "FINE ID | LOAN ID | TRE HAN | SO TIEN   | TRANG THAI\r\n"
                 "----------------------------------------------------\r\n";
    for (const auto& f : g_db.fines) {
        att += f.fineId + "     | " + f.loanId + "     | " + to_string(f.daysOverdue) + " ngay | " + to_string((int)f.amount) + " d | " + f.paidStatus + "\r\n";
    }
    SetWindowTextA(hBoxAttention, att.c_str());
}

string GetMembersListText() {
    string res = "DANH SACH TAI KHOAN TRONG HE THONG (Database: User / Member / Librarian / Admin)\r\n"
                 "==================================================================================\r\n";
    for (const auto& a : g_db.admins)
        res += "[" + a.getUserId() + "] " + a.getFullName() + " | Email: " + a.getEmail() + " | Role: Administrator\r\n";
    for (const auto& l : g_db.librarians)
        res += "[" + l.getUserId() + "] " + l.getFullName() + " | Email: " + l.getEmail() + " | Role: Librarian (Ca: " + l.deskShift + ")\r\n";
    for (const auto& m : g_db.members)
        res += "[" + m.getUserId() + "] " + m.getFullName() + " | Email: " + m.getEmail() + " | The: " + m.membershipCardNo +
               " | Dang muon: " + to_string(m.activeLoansCount) + "/" + to_string(m.maxQuota) + " | No phat: " + to_string((int)m.totalUnpaidFines) + " d\r\n";
    return res;
}

void ShowLoginScreen(HWND hwnd) {
    SetGroupVisible(g_loginScreen, true);
    SetGroupVisible(g_registerScreen, false);
    SetGroupVisible(g_appShell, false);
    SetGroupVisible(g_tabOverview, false);
    SetGroupVisible(g_tabCatalogue, false);
    SetGroupVisible(g_tabCirculation, false);
    SetGroupVisible(g_tabMembers, false);
    SetGroupVisible(g_tabReports, false);
    InvalidateRect(hwnd, NULL, TRUE);
}

void ShowRegisterScreen(HWND hwnd) {
    SetGroupVisible(g_loginScreen, false);
    SetGroupVisible(g_registerScreen, true);
    SetGroupVisible(g_appShell, false);
    SetGroupVisible(g_tabOverview, false);
    SetGroupVisible(g_tabCatalogue, false);
    SetGroupVisible(g_tabCirculation, false);
    SetGroupVisible(g_tabMembers, false);
    SetGroupVisible(g_tabReports, false);
    SetWindowTextA(hRegStatus, "Dang ky tai khoan Doc gia (Member) moi.");
    InvalidateRect(hwnd, NULL, TRUE);
}

void SwitchTab(HWND hwnd, int tabId) {
    g_activeTab = tabId;
    SetGroupVisible(g_loginScreen, false);
    SetGroupVisible(g_registerScreen, false);
    SetGroupVisible(g_appShell, true);

    SetGroupVisible(g_tabOverview,    tabId == ID_TAB_OVERVIEW);
    SetGroupVisible(g_tabCatalogue,   tabId == ID_TAB_CATALOGUE);
    SetGroupVisible(g_tabCirculation, tabId == ID_TAB_CIRCULATION);
    SetGroupVisible(g_tabMembers,     tabId == ID_TAB_MEMBERS);
    SetGroupVisible(g_tabReports,     tabId == ID_TAB_REPORTS);

    SetWindowTextA(hDateLabel, DateUtils::today().c_str());

    if (tabId == ID_TAB_OVERVIEW) {
        SetWindowTextA(hHeaderPath, "Diamond Library  >  Overview (Tong quan he thong)");
        RefreshOverviewData();
    } else if (tabId == ID_TAB_CATALOGUE) {
        SetWindowTextA(hHeaderPath, "Diamond Library  >  Catalogue (Quan ly Kho sach & Ban sao - Process 2.0)");
        SetWindowTextA(hCatOutput, g_ctrl.searchBook("").c_str());
    } else if (tabId == ID_TAB_CIRCULATION) {
        SetWindowTextA(hHeaderPath, "Diamond Library  >  Circulation (Muon, Tra, Gia han, Dat cho & Thu phat - Process 3.0 & 4.0)");
        SetWindowTextA(hCirDay, DateUtils::today().c_str());
        SetWindowTextA(hCirOutput, g_ctrl.searchBook("").c_str());
    } else if (tabId == ID_TAB_MEMBERS) {
        SetWindowTextA(hHeaderPath, "Diamond Library  >  Members & Staff (Quan ly Tai khoan - Process 1.0)");
        SetWindowTextA(hMemOutput, GetMembersListText().c_str());
    } else if (tabId == ID_TAB_REPORTS) {
        SetWindowTextA(hHeaderPath, "Diamond Library  >  Reports & Policy (Bao cao & Cau hinh quy dinh - Process 5.0)");
        SetWindowTextA(hRepOutput, g_ctrl.exportAuditReport().c_str());
    }
    InvalidateRect(hwnd, NULL, TRUE);
}
