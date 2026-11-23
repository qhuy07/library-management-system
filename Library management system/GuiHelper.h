#ifndef GUI_HELPER_H
#define GUI_HELPER_H

#include <windows.h>
#include <string>

using namespace std;

// Bảng màu chuẩn thiết kế Figma Diamond Library
const COLORREF COL_BG_CREAM   = RGB(244, 246, 243); // Nền kem nhạt
const COLORREF COL_SIDEBAR    = RGB(255, 255, 255); // Sidebar trắng
const COLORREF COL_DARK_GREEN = RGB(30, 77, 59);    // Xanh rêu đậm (#1E4D3B)
const COLORREF COL_MINT_BG    = RGB(230, 242, 236); // Xanh bạc hà nhạt
const COLORREF COL_TEXT_DARK  = RGB(25, 35, 30);
const COLORREF COL_TEXT_MUTED = RGB(100, 115, 108);

inline HBRUSH GetCreamBrush() {
    static HBRUSH br = CreateSolidBrush(COL_BG_CREAM);
    return br;
}

inline HBRUSH GetWhiteBrush() {
    static HBRUSH br = CreateSolidBrush(COL_SIDEBAR);
    return br;
}

inline HBRUSH GetDarkGreenBrush() {
    static HBRUSH br = CreateSolidBrush(COL_DARK_GREEN);
    return br;
}

inline HFONT GetCustomFont(int height, bool bold = false) {
    typedef HFONT (WINAPI *PFN_CreateFontA)(int,int,int,int,int,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,LPCSTR);
    static PFN_CreateFontA pCreateFont = (PFN_CreateFontA)GetProcAddress(LoadLibraryA("gdi32.dll"), "CreateFontA");
    if (pCreateFont) {
        return pCreateFont(height, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_SWISS, "Segoe UI");
    }
    return NULL;
}

inline void SetFont(HWND h, int size = 16, bool bold = false) {
    static HFONT fNormal = GetCustomFont(16, false);
    static HFONT fBold   = GetCustomFont(16, true);
    static HFONT fTitle  = GetCustomFont(22, true);
    HFONT chosen = (size >= 20) ? fTitle : (bold ? fBold : fNormal);
    if (chosen) SendMessageA(h, WM_SETFONT, (WPARAM)chosen, TRUE);
}

inline HWND MakeGroup(HWND p, const char* title, int x, int y, int w, int h) {
    HWND hw = CreateWindowA("BUTTON", title, WS_VISIBLE | WS_CHILD | BS_GROUPBOX, x, y, w, h, p, NULL, NULL, NULL);
    SetFont(hw, 16, true);
    return hw;
}

inline HWND MakeLabel(HWND p, const char* text, int x, int y, int w, int h, int size = 16, bool bold = false) {
    HWND hw = CreateWindowA("STATIC", text, WS_VISIBLE | WS_CHILD, x, y, w, h, p, NULL, NULL, NULL);
    SetFont(hw, size, bold);
    return hw;
}

inline HWND MakeInput(HWND p, const char* def, int x, int y, int w, int h, bool pass = false) {
    DWORD st = WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | (pass ? ES_PASSWORD : 0);
    HWND hw = CreateWindowA("EDIT", def, st, x, y, w, h, p, NULL, NULL, NULL);
    SetFont(hw, 16, false);
    return hw;
}

// Nút bấm hỗ trợ tô màu Owner-Draw (BS_OWNERDRAW)
inline HWND MakeColorButton(HWND p, const char* text, int id, int x, int y, int w, int h) {
    HWND hw = CreateWindowA("BUTTON", text, WS_VISIBLE | WS_CHILD | BS_OWNERDRAW, x, y, w, h, p, (HMENU)(INT_PTR)id, NULL, NULL);
    SetFont(hw, 16, true);
    return hw;
}

inline HWND MakeBox(HWND p, const char* def, int x, int y, int w, int h) {
    HWND hw = CreateWindowA("EDIT", def, WS_VISIBLE | WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY, x, y, w, h, p, NULL, NULL, NULL);
    SetFont(hw, 16, false);
    return hw;
}

// Hàm vẽ màu cho nút bấm (Xanh rêu đậm, Xanh bạc hà active, Đỏ đăng xuất/xóa)
inline void DrawStyledButton(LPDRAWITEMSTRUCT dis, int activeTabId) {
    int id = (int)dis->CtlID;
    HDC hdc = dis->hDC;
    RECT rc = dis->rcItem;

    COLORREF bg = COL_DARK_GREEN;
    COLORREF fg = RGB(255, 255, 255);
    COLORREF border = COL_DARK_GREEN;

    if (id >= 101 && id <= 105) {
        if (id == activeTabId) {
            bg = COL_DARK_GREEN;
            fg = RGB(255, 255, 255);
        } else {
            bg = COL_MINT_BG;
            fg = COL_DARK_GREEN;
            border = RGB(195, 215, 205);
        }
    } else if (id == 106 || id == 205) {
        bg = RGB(175, 50, 50);
        border = RGB(175, 50, 50);
    } else if (id == 207 || id == 210) {
        bg = RGB(24, 95, 110);
        border = RGB(24, 95, 110);
    } else if (id == 213 || id == 215) {
        bg = COL_MINT_BG;
        fg = COL_DARK_GREEN;
        border = COL_DARK_GREEN;
    }

    if (dis->itemState & ODS_SELECTED) {
        bg = RGB(20, 50, 38);
    }

    HBRUSH hBr = CreateSolidBrush(bg);
    FillRect(hdc, &rc, hBr);
    DeleteObject(hBr);

    HBRUSH hBorder = CreateSolidBrush(border);
    FrameRect(hdc, &rc, hBorder);
    DeleteObject(hBorder);

    char text[128];
    GetWindowTextA(dis->hwndItem, text, sizeof(text));
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, fg);
    DrawTextA(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

#endif
