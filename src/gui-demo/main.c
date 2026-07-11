#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include "resource.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")

static HWND hEditInput, hEditMulti;
static HWND hListBox, hComboBox;
static HWND hChkEnable, hChkBold;
static HWND hSlider, hProgress;
static HWND hTree, hTab, hStatusBar;
static HWND hStaticValue;
static HWND hRadRed;
static HFONT hFontNormal, hFontBold;

static BOOL CALLBACK SetFontProc(HWND hwnd, LPARAM lParam) {
    SendMessage(hwnd, WM_SETFONT, (WPARAM)lParam, TRUE);
    return TRUE;
}

static INT_PTR CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_INITDIALOG:
        hEditInput = GetDlgItem(hwnd, IDC_EDIT_INPUT);
        hEditMulti = GetDlgItem(hwnd, IDC_EDIT_MULTI);
        hListBox = GetDlgItem(hwnd, IDC_LIST_BOX);
        hComboBox = GetDlgItem(hwnd, IDC_COMBO_BOX);
        hChkEnable = GetDlgItem(hwnd, ID_CHK_ENABLE);
        hChkBold = GetDlgItem(hwnd, ID_CHK_BOLD);
        hSlider = GetDlgItem(hwnd, IDC_SLIDER);
        hProgress = GetDlgItem(hwnd, IDC_PROGRESS);
        hTree = GetDlgItem(hwnd, IDC_TREE);
        hTab = GetDlgItem(hwnd, IDC_TAB);
        hStaticValue = GetDlgItem(hwnd, IDC_STATIC_VALUE);
        hRadRed = GetDlgItem(hwnd, ID_RAD_RED);

        SendMessage(hChkEnable, BM_SETCHECK, BST_CHECKED, 0);
        SendMessage(hRadRed, BM_SETCHECK, BST_CHECKED, 0);
        SendMessage(hSlider, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
        SendMessage(hSlider, TBM_SETPOS, TRUE, 50);
        SendMessage(hSlider, TBM_SETTICFREQ, 10, 0);
        SendMessage(hProgress, PBM_SETRANGE, 0, MAKELONG(0, 100));
        SendMessage(hProgress, PBM_SETPOS, 50, 0);
        SendMessage(hProgress, PBM_SETBARCOLOR, 0, RGB(0, 180, 0));

        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)"Apple");
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)"Banana");
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)"Cherry");
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)"Date");
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)"Elderberry");
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)"Fig");
        SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)"Grape");

        SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)"Small");
        SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)"Medium");
        SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)"Large");
        SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)"Extra Large");
        SendMessage(hComboBox, CB_SETCURSEL, 1, 0);

        {
            TVINSERTSTRUCTA tvi = {0};
            tvi.hInsertAfter = TVI_LAST;
            tvi.item.mask = TVIF_TEXT;

            tvi.hParent = TVI_ROOT;
            tvi.item.pszText = "Documents";
            HTREEITEM hDocs = (HTREEITEM)TreeView_InsertItem(hTree, &tvi);
            tvi.hParent = hDocs;
            tvi.item.pszText = "report.doc";
            TreeView_InsertItem(hTree, &tvi);
            tvi.item.pszText = "notes.txt";
            TreeView_InsertItem(hTree, &tvi);

            tvi.hParent = TVI_ROOT;
            tvi.item.pszText = "Pictures";
            HTREEITEM hPics = (HTREEITEM)TreeView_InsertItem(hTree, &tvi);
            tvi.hParent = hPics;
            tvi.item.pszText = "photo.jpg";
            TreeView_InsertItem(hTree, &tvi);
            tvi.item.pszText = "screenshot.png";
            TreeView_InsertItem(hTree, &tvi);

            tvi.hParent = TVI_ROOT;
            tvi.item.pszText = "Music";
            TreeView_InsertItem(hTree, &tvi);
        }

        {
            TCITEMA tie = {0};
            tie.mask = TCIF_TEXT;
            tie.pszText = "General";
            TabCtrl_InsertItem(hTab, 0, &tie);
            tie.pszText = "Advanced";
            TabCtrl_InsertItem(hTab, 1, &tie);
            tie.pszText = "About";
            TabCtrl_InsertItem(hTab, 2, &tie);
        }

        hStatusBar = CreateWindowExA(0, STATUSCLASSNAMEA, "",
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0, hwnd, (HMENU)1802, GetModuleHandle(NULL), NULL);
        SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)"Ready");
        SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM)"Controls Demo - WinPE");
        {
            int parts[] = {200, 400, -1};
            SendMessage(hStatusBar, SB_SETPARTS, 3, (LPARAM)parts);
        }
        SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM)"Win32 API");

        EnumChildWindows(hwnd, SetFontProc, (LPARAM)hFontNormal);
        return TRUE;

    case WM_CTLCOLORDLG:
        return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
        return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
    }

    case WM_NOTIFY: {
        NMHDR *nm = (NMHDR *)lParam;
        if (nm->hwndFrom == hTab && nm->code == NM_CUSTOMDRAW) {
            NMTBCUSTOMDRAW *ntb = (NMTBCUSTOMDRAW *)lParam;
            if (ntb->nmcd.dwDrawStage == CDDS_PREPAINT)
                return CDRF_NOTIFYITEMDRAW;
            if (ntb->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
                ntb->clrText = RGB(0, 0, 0);
                ntb->clrBtnFace = GetSysColor(COLOR_BTNFACE);
                ntb->clrBtnHighlight = GetSysColor(COLOR_BTNFACE);
                return CDRF_NEWFONT;
            }
        }
        return 0;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);

        switch (id) {
        case ID_BTN_CLICK: {
            char text[256];
            GetWindowTextA(hEditInput, text, sizeof(text));
            if (text[0] == '\0')
                lstrcpyA(text, "(empty)");
            char buf[512];
            wsprintfA(buf, "You typed: %s", text);
            MessageBoxA(hwnd, buf, "Button Clicked", MB_ICONINFORMATION);
            SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)"Button clicked!");
            break;
        }
        case ID_BTN_CLEAR:
            SetWindowTextA(hEditInput, "");
            SetWindowTextA(hEditMulti, "");
            SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
            SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)"All cleared.");
            break;

        case ID_BTN_ADD: {
            char text[256];
            GetWindowTextA(hEditInput, text, sizeof(text));
            if (text[0]) {
                SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)text);
                int count = (int)SendMessage(hListBox, LB_GETCOUNT, 0, 0);
                char buf[64];
                wsprintfA(buf, "Added item. Total: %d", count);
                SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)buf);
            }
            break;
        }
        case ID_BTN_REMOVE: {
            int sel = (int)SendMessage(hListBox, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
                SendMessage(hListBox, LB_DELETESTRING, sel, 0);
                SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)"Item removed.");
            }
            break;
        }

        case ID_CHK_ENABLE:
            EnableWindow(hListBox, SendMessage(hChkEnable, BM_GETCHECK, 0, 0) == BST_CHECKED);
            break;

        case ID_CHK_BOLD:
            if (SendMessage(hChkBold, BM_GETCHECK, 0, 0) == BST_CHECKED)
                SendMessage(hEditMulti, WM_SETFONT, (WPARAM)hFontBold, TRUE);
            else
                SendMessage(hEditMulti, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
            break;

        case ID_RAD_RED:
        case ID_RAD_GREEN:
        case ID_RAD_BLUE:
            SendMessage(hStatusBar, SB_SETTEXT, 1,
                (LPARAM)(id == ID_RAD_RED ? "Red selected" :
                         id == ID_RAD_GREEN ? "Green selected" : "Blue selected"));
            break;

        case IDC_LIST_BOX:
            if (code == LBN_SELCHANGE) {
                int sel = (int)SendMessage(hListBox, LB_GETCURSEL, 0, 0);
                if (sel != LB_ERR) {
                    char buf[128];
                    SendMessage(hListBox, LB_GETTEXT, sel, (LPARAM)buf);
                    char msg[256];
                    wsprintfA(msg, "Selected: %s", buf);
                    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)msg);
                }
            }
            break;

        case IDC_COMBO_BOX:
            if (code == CBN_SELCHANGE) {
                int sel = (int)SendMessage(hComboBox, CB_GETCURSEL, 0, 0);
                char buf[64];
                SendMessage(hComboBox, CB_GETLBTEXT, sel, (LPARAM)buf);
                char msg[128];
                wsprintfA(msg, "Combo: %s", buf);
                SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)msg);
            }
            break;
        }
        return 0;
    }

    case WM_HSCROLL: {
        if ((HWND)lParam == hSlider) {
            int pos = (int)SendMessage(hSlider, TBM_GETPOS, 0, 0);
            char buf[32];
            wsprintfA(buf, "Value: %d", pos);
            SetWindowTextA(hStaticValue, buf);
            SendMessage(hProgress, PBM_SETPOS, pos, 0);
        }
        return 0;
    }

    case WM_SIZE: {
        if (hStatusBar) {
            SendMessage(hStatusBar, WM_SIZE, 0, 0);
            int parts[] = {200, 400, -1};
            SendMessage(hStatusBar, SB_SETPARTS, 3, (LPARAM)parts);
        }
        return 0;
    }

    case WM_CLOSE:
        EndDialog(hwnd, 0);
        return TRUE;
    }
    return FALSE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    (void)hPrev; (void)lpCmd; (void)nShow;

    INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_BAR_CLASSES | ICC_TREEVIEW_CLASSES | ICC_TAB_CLASSES};
    InitCommonControlsEx(&icc);

    hFontNormal = CreateFontA(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, 0, 0, 0, 0, "Segoe UI");
    hFontBold = CreateFontA(13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, 0, 0, 0, 0, "Segoe UI");

    HWND hDlg = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_GUIDIALOG), NULL, DlgProc);
    ShowWindow(hDlg, nShow);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        if (!IsDialogMessageA(hDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

    DeleteObject(hFontNormal);
    DeleteObject(hFontBold);
    return (int)msg.wParam;
}
