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

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        // MAN HINH 0: DANG NHAP
        HWND lgBox    = MakeGroup(hwnd, " LOG IN TO DIAMOND LIBRARY SYSTEM  ", 310, 120, 480, 410);
        HWND lgTitle  = MakeLabel(hwnd, "DIAMOND LIBRARY", 425, 165, 280, 28, 22, true);
        HWND lgSub    = MakeLabel(hwnd, "Please log in with account @ut.edu.vn", 395, 198, 320, 20);

        HWND lgL1     = MakeLabel(hwnd, "Email login:", 350, 240, 115, 20, 16, true);
        hLoginEmail   = MakeInput(hwnd, "long@ut.edu.vn", 470, 237, 270, 28);

        HWND lgL2     = MakeLabel(hwnd, "Password:", 350, 282, 115, 20, 16, true);
        hLoginPass    = MakeInput(hwnd, "123", 470, 279, 270, 28, true);

        HWND lgBtn    = MakeColorButton(hwnd, "LOG IN TO SYSTEM", ID_BTN_LOGIN, 350, 326, 390, 38);
        HWND lgRegBtn = MakeColorButton(hwnd, "No account? Register account now", ID_BTN_GO_REGISTER, 350, 374, 390, 34);
        hLoginStatus  = MakeLabel(hwnd, "Test account (Pass: 123):\n- librarian: long@ut.edu.vn\n- Admin: admin@ut.edu.vn  |  Member: truong@ut.edu.vn", 350, 422, 390, 75);

        g_loginScreen = {lgBox, lgTitle, lgSub, lgL1, hLoginEmail, lgL2, hLoginPass, lgBtn, lgRegBtn, hLoginStatus};

        // MAN HINH 0B: DANG KY TAI KHOAN MOI
        HWND rgBox     = MakeGroup(hwnd, " REGISTER NEW MEMBER ACCOUNT ", 310, 120, 480, 430);
        HWND rgTitle   = MakeLabel(hwnd, "CREATE ACCOUNT DIAMOND", 420, 165, 290, 28, 22, true);
        HWND rgSub     = MakeLabel(hwnd, "Register an account to search & Reserve and hold book", 385, 198, 340, 20);

        HWND rgL1      = MakeLabel(hwnd, "Full name:", 345, 236, 120, 20, 16, true);
        hRegName       = MakeInput(hwnd, "", 470, 233, 275, 28);

        HWND rgL2      = MakeLabel(hwnd, "Email:", 345, 276, 120, 20, 16, true);
        hRegEmail      = MakeInput(hwnd, "", 470, 273, 275, 28);

        HWND rgL3      = MakeLabel(hwnd, "Pass:", 345, 316, 120, 20, 16, true);
        hRegPass       = MakeInput(hwnd, "", 470, 313, 275, 28, true);

        HWND rgL4      = MakeLabel(hwnd, "Password confirm:", 345, 356, 120, 20, 16, true);
        hRegConfirm    = MakeInput(hwnd, "", 470, 353, 275, 28, true);

        HWND rgBtn     = MakeColorButton(hwnd, "REGISTER ACCOUNT", ID_BTN_REGISTER, 345, 398, 400, 38);
        HWND rgBackBtn = MakeColorButton(hwnd, "Have an account? Go to login page", ID_BTN_BACK_LOGIN, 345, 446, 400, 34);
        hRegStatus     = MakeLabel(hwnd, "Register new member account.", 345, 492, 400, 45);

        g_registerScreen = {rgBox, rgTitle, rgSub, rgL1, hRegName, rgL2, hRegEmail, rgL3, hRegPass, rgL4, hRegConfirm, rgBtn, rgBackBtn, hRegStatus};

        // KHUNG CHUNG SAU KHI DANG NHAP
        HWND sbBox = MakeGroup(hwnd, " Diamond LMS ", 15, 10, 195, 640);
        HWND bOv   = MakeColorButton(hwnd, "1. Overview",    ID_TAB_OVERVIEW,    30, 45,  165, 36);
        HWND bCat  = MakeColorButton(hwnd, "2. Catalogue",   ID_TAB_CATALOGUE,   30, 90,  165, 36);
        HWND bCir  = MakeColorButton(hwnd, "3. Circulation", ID_TAB_CIRCULATION, 30, 135, 165, 36);
        HWND bMem  = MakeColorButton(hwnd, "4. Members",     ID_TAB_MEMBERS,     30, 180, 165, 36);
        HWND bRep  = MakeColorButton(hwnd, "5. Reports",     ID_TAB_REPORTS,     30, 225, 165, 36);

        HWND uBox  = MakeGroup(hwnd, " Tai khoan online ", 25, 505, 175, 78);
        hUserFooter = MakeLabel(hwnd, "", 35, 528, 155, 45, 16, true);
        HWND bOut  = MakeColorButton(hwnd, "Dang xuat (Logout)", ID_BTN_LOGOUT, 25, 595, 175, 36);

        hHeaderPath = MakeLabel(hwnd, "Diamond Library > Overview", 225, 18, 720, 22, 16, true);
        hDateLabel  = MakeLabel(hwnd, DateUtils::today().c_str(), 965, 18, 105, 22);

        g_appShell = {sbBox, bOv, bCat, bCir, bMem, bRep, uBox, hUserFooter, bOut, hHeaderPath, hDateLabel};

        // MAN HINH 1: OVERVIEW
        HWND ovB1 = MakeGroup(hwnd, " LIBRARY BRIEFING ", 225, 45, 845, 75);
        HWND ovB2 = MakeLabel(hwnd, "Welcome back to Diamond Library!", 245, 72, 750, 26, 20, true);

        HWND ovK1 = MakeGroup(hwnd, " Loans active ",    225, 130, 202, 70);
        hStatLoans   = MakeLabel(hwnd, "0", 240, 160, 175, 25, 16, true);
        HWND ovK2 = MakeGroup(hwnd, " Books available ", 439, 130, 202, 70);
        hStatAvail   = MakeLabel(hwnd, "0", 454, 160, 175, 25, 16, true);
        HWND ovK3 = MakeGroup(hwnd, " Overdue loans ",   653, 130, 202, 70);
        hStatOverdue = MakeLabel(hwnd, "0", 668, 160, 175, 25, 16, true);
        HWND ovK4 = MakeGroup(hwnd, " Active members ",  867, 130, 203, 70);
        hStatMembers = MakeLabel(hwnd, "0", 882, 160, 175, 25, 16, true);

        HWND ovRec = MakeGroup(hwnd, " Recent circulation (Lich su muon tra) ", 225, 215, 480, 435);
        hBoxRecent = MakeBox(hwnd, "", 240, 242, 450, 392);

        HWND ovAtt = MakeGroup(hwnd, " Needs your attention (Cac khoan phat tre han) ", 720, 215, 350, 435);
        hBoxAttention = MakeBox(hwnd, "", 735, 242, 320, 392);

        g_tabOverview = {ovB1, ovB2, ovK1, hStatLoans, ovK2, hStatAvail, ovK3, hStatOverdue, ovK4, hStatMembers, ovRec, hBoxRecent, ovAtt, hBoxAttention};

        // MAN HINH 2: CATALOGUE
        HWND catG1 = MakeGroup(hwnd, " Nhap thong tin Dau sach (Book) & Ban sao vat ly (BookCopy) ", 225, 45, 845, 155);
        HWND catL1 = MakeLabel(hwnd, "Book ID:", 245, 78, 60, 20);
        hCatBookId = MakeInput(hwnd, "B05", 310, 75, 80, 24);
        HWND catL2 = MakeLabel(hwnd, "Ten sach / Tu khoa:", 410, 78, 125, 20);
        hCatTitle  = MakeInput(hwnd, "", 540, 75, 240, 24);
        HWND catB1 = MakeColorButton(hwnd, "Tim kiem sach", ID_BTN_SEARCH, 800, 73, 250, 28);

        HWND catL3 = MakeLabel(hwnd, "Tac gia:", 245, 118, 60, 20);
        hCatAuthor = MakeInput(hwnd, "Nam Cao", 310, 115, 170, 24);
        HWND catL4 = MakeLabel(hwnd, "Copy ID:", 500, 118, 60, 20);
        hCatCopyId = MakeInput(hwnd, "C05", 565, 115, 80, 24);
        HWND catL5 = MakeLabel(hwnd, "Barcode:", 665, 118, 60, 20);
        hCatBarcode = MakeInput(hwnd, "NF-0200-01", 730, 115, 120, 24);

        HWND catB2 = MakeColorButton(hwnd, "Them Sach & Ban sao", ID_BTN_ADD_BOOK, 245, 155, 255, 32);
        HWND catB3 = MakeColorButton(hwnd, "Cap nhat thong tin Sach", ID_BTN_UPD_BOOK, 520, 155, 255, 32);
        HWND catB4 = MakeColorButton(hwnd, "Xoa Dau sach (Delete)", ID_BTN_DEL_BOOK, 795, 155, 255, 32);

        HWND catG2 = MakeGroup(hwnd, " Danh sach Dau sach & Ban sao trong kho ", 225, 210, 845, 440);
        hCatOutput = MakeBox(hwnd, "", 240, 235, 815, 400);

        g_tabCatalogue = {catG1, catL1, hCatBookId, catL2, hCatTitle, catB1, catL3, hCatAuthor, catL4, hCatCopyId, catL5, hCatBarcode, catB2, catB3, catB4, catG2, hCatOutput};
    // MAN HINH 3: CIRCULATION
        HWND cirG1 = MakeGroup(hwnd, " A. Muon sach (Borrow) & Dat giu cho (Hold) ", 225, 45, 415, 215);
        HWND cirL1 = MakeLabel(hwnd, "Ma Doc gia (MemberID):", 240, 78, 150, 20);
        hCirMemberId = MakeInput(hwnd, "U03", 400, 75, 220, 24);

        HWND cirL2 = MakeLabel(hwnd, "Ma Ban sao (CopyID):", 240, 113, 150, 20);
        hCirCopyId = MakeInput(hwnd, "C01", 400, 110, 220, 24);

        HWND cirL3 = MakeLabel(hwnd, "Ma Dau sach (BookID):", 240, 148, 150, 20);
        hCirBookId = MakeInput(hwnd, "B02", 400, 145, 220, 24);

        HWND cirB1 = MakeColorButton(hwnd, "Cho Muon Sach (Borrow)", ID_BTN_BORROW, 240, 185, 185, 34);
        HWND cirB2 = MakeColorButton(hwnd, "Dat Giu Cho (Hold)",     ID_BTN_HOLD,   435, 185, 185, 34);

        HWND cirG2 = MakeGroup(hwnd, " B. Tra sach (Return), Gia han (Renew) & Thu Phat ", 655, 45, 415, 215);
        HWND cirL4 = MakeLabel(hwnd, "Ma Phieu muon (LoanID):", 670, 78, 155, 20);
        hCirLoanId = MakeInput(hwnd, "L01", 835, 75, 215, 24);

        HWND cirL5 = MakeLabel(hwnd, "Ngay GD (YYYY-MM-DD):", 670, 113, 160, 20);
        hCirDay = MakeInput(hwnd, DateUtils::today().c_str(), 835, 110, 215, 24);

        HWND cirL6 = MakeLabel(hwnd, "Ma Phieu phat (FineID):", 670, 148, 155, 20);
        hCirFineId = MakeInput(hwnd, "F01", 835, 145, 215, 24);

        HWND cirB3 = MakeColorButton(hwnd, "Nhan Tra & Tinh Phat", ID_BTN_RETURN,   670, 185, 145, 34);
        HWND cirB4 = MakeColorButton(hwnd, "Gia Han (+7d)",       ID_BTN_RENEW,    822, 185, 110, 34);
        HWND cirB5 = MakeColorButton(hwnd, "Thu Tien Phat",       ID_BTN_PAY_FINE, 939, 185, 111, 34);

        HWND cirG3 = MakeGroup(hwnd, " Ket qua giao dich & Trang thai ban sao sach ", 225, 270, 845, 380);
        hCirOutput = MakeBox(hwnd, "", 240, 295, 815, 340);

        g_tabCirculation = {
            cirG1, cirL1, hCirMemberId, cirL2, hCirCopyId, cirL3, hCirBookId, cirB1, cirB2,
            cirG2, cirL4, hCirLoanId, cirL5, hCirDay, cirL6, hCirFineId, cirB3, cirB4, cirB5,
            cirG3, hCirOutput
        };


        // MAN HINH 4: MEMBERS
        HWND memG1 = MakeGroup(hwnd, " Dang ky & Tao tai khoan moi (Member / Librarian) ", 225, 45, 845, 130);
        HWND memL1 = MakeLabel(hwnd, "Ho va Ten:", 245, 80, 75, 20);
        hMemName   = MakeInput(hwnd, "Tran Van B", 325, 77, 180, 24);
        HWND memL2 = MakeLabel(hwnd, "Email (@ut.edu.vn):", 520, 80, 125, 20);
        hMemEmail  = MakeInput(hwnd, "vanb@ut.edu.vn", 650, 77, 180, 24);
        HWND memL3 = MakeLabel(hwnd, "Vai tro:", 845, 80, 50, 20);
        hMemRole   = MakeInput(hwnd, "Member", 900, 77, 150, 24);

        HWND memB1 = MakeColorButton(hwnd, "Tao Tai Khoan Moi (Mac dinh Pass: 123)", ID_BTN_ADD_ACC, 245, 120, 805, 34);

        HWND memG2 = MakeGroup(hwnd, " Danh sach Doc gia, Thu thu & Quan tri vien ", 225, 185, 845, 465);
        hMemOutput = MakeBox(hwnd, "", 240, 210, 815, 425);

        g_tabMembers = {memG1, memL1, hMemName, memL2, hMemEmail, memL3, hMemRole, memB1, memG2, hMemOutput};

        // MAN HINH 5: REPORTS
        HWND repG1 = MakeGroup(hwnd, " Cau hinh chinh sach Thu vien (Admin Only - Process 5.0) ", 225, 45, 845, 120);
        HWND repL1 = MakeLabel(hwnd, "So ngay muon toi da:", 245, 80, 140, 20);
        hRepMaxDays = MakeInput(hwnd, "14", 390, 77, 80, 24);
        HWND repL2 = MakeLabel(hwnd, "Muc phat tre han (VND/ngay):", 495, 80, 190, 20);
        hRepFineRate = MakeInput(hwnd, "5000", 690, 77, 110, 24);
        HWND repB1 = MakeColorButton(hwnd, "Luu Quy Dinh & Xuat Bao Cao", ID_BTN_POLICY, 820, 73, 230, 34);

        HWND repG2 = MakeGroup(hwnd, " Bao cao kiem toan & Thong ke hoat dong (Audit Report) ", 225, 175, 845, 475);
        hRepOutput = MakeBox(hwnd, "", 240, 200, 815, 435);

        g_tabReports = {repG1, repL1, hRepMaxDays, repL2, hRepFineRate, repB1, repG2, hRepOutput};

        ShowLoginScreen(hwnd);
        break;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        HWND hCtrl = (HWND)lParam;
        SetBkMode(hdcStatic, TRANSPARENT);
        if (hCtrl == hLoginStatus || hCtrl == hRegStatus) {
            SetTextColor(hdcStatic, RGB(160, 45, 45));
        } else if (hCtrl == hHeaderPath) {
            SetTextColor(hdcStatic, COL_TEXT_MUTED);
        } else {
            SetTextColor(hdcStatic, COL_TEXT_DARK);
        }
        return (INT_PTR)GetCreamBrush();
    }
    case WM_CTLCOLOREDIT: {
        HDC hdcEdit = (HDC)wParam;
        SetTextColor(hdcEdit, COL_TEXT_DARK);
        SetBkColor(hdcEdit, COL_SIDEBAR);
        return (INT_PTR)GetWhiteBrush();
    }
    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
        if (dis->CtlType == ODT_BUTTON) {
            DrawStyledButton(dis, g_activeTab);
            return TRUE;
        }
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        if (!g_user) {
            RECT banner = {312, 128, 788, 154};
            FillRect(hdc, &banner, GetDarkGreenBrush());
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == ID_BTN_LOGIN) {
            User* u = g_ctrl.login(GetStr(hLoginEmail), GetStr(hLoginPass));
            if (u) {
                g_user = u;
                SetWindowTextA(hUserFooter, (u->getFullName() + "\n[" + u->getRole() + "]").c_str());
                SwitchTab(hwnd, ID_TAB_OVERVIEW);
            } else {
                SetWindowTextA(hLoginStatus, "LOI: Sai Email hoac Mat khau! Vui long kiem tra lai.");
            }
        }
        else if (id == ID_BTN_GO_REGISTER) {
            ShowRegisterScreen(hwnd);
        }
        else if (id == ID_BTN_BACK_LOGIN) {
            ShowLoginScreen(hwnd);
        }
        else if (id == ID_BTN_REGISTER) {
            string name = GetStr(hRegName);
            string email = GetStr(hRegEmail);
            string pass = GetStr(hRegPass);
            string confirm = GetStr(hRegConfirm);
            if (pass != confirm) {
                SetWindowTextA(hRegStatus, "Error: Confirm password not match!");
            } else {
                string msg, newId;
                if (g_ctrl.registerMember(name, email, pass, msg, newId)) {
                    SetWindowTextA(hLoginEmail, email.c_str());
                    SetWindowTextA(hLoginPass, "");
                    SetWindowTextA(hLoginStatus, (msg + "\nHay dang nhap bang tai khoan vua tao!").c_str());
                    SetWindowTextA(hRegName, "");
                    SetWindowTextA(hRegEmail, "");
                    SetWindowTextA(hRegPass, "");
                    SetWindowTextA(hRegConfirm, "");
                    ShowLoginScreen(hwnd);
                } else {
                    SetWindowTextA(hRegStatus, msg.c_str());
                }
            }
        }
        else if (id == ID_BTN_LOGOUT) {
            g_user = nullptr;
            SetWindowTextA(hLoginStatus, "Log out of the system.");
            ShowLoginScreen(hwnd);
        }
        else if (id >= ID_TAB_OVERVIEW && id <= ID_TAB_REPORTS) {
            SwitchTab(hwnd, id);
        }
        else if (id == ID_BTN_SEARCH) {
            SetWindowTextA(hCatOutput, g_ctrl.searchBook(GetStr(hCatTitle)).c_str());
        }
        else if (id == ID_BTN_ADD_BOOK) {
            string msg = g_ctrl.addBookAndCopy(GetStr(hCatBookId), GetStr(hCatTitle), GetStr(hCatAuthor), GetStr(hCatCopyId), GetStr(hCatBarcode));
            SetWindowTextA(hCatOutput, (msg + "\r\n\r\n" + g_ctrl.searchBook("")).c_str());
        }
        else if (id == ID_BTN_UPD_BOOK) {
            string msg = g_ctrl.updateBook(GetStr(hCatBookId), GetStr(hCatTitle), GetStr(hCatAuthor));
            SetWindowTextA(hCatOutput, (msg + "\r\n\r\n" + g_ctrl.searchBook("")).c_str());
        }
        else if (id == ID_BTN_DEL_BOOK) {
            string msg = g_ctrl.deleteBook(GetStr(hCatBookId));
            SetWindowTextA(hCatOutput, (msg + "\r\n\r\n" + g_ctrl.searchBook("")).c_str());
        }
