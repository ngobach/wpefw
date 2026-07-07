#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")

#define ID_BTN_CLICK     1001
#define ID_BTN_CLEAR     1002
#define ID_BTN_ADD       1003
#define ID_BTN_REMOVE    1004
#define ID_EDIT_INPUT    1101
#define ID_EDIT_MULTI    1102
#define ID_LIST_BOX      1201
#define ID_COMBO_BOX     1202
#define ID_CHK_ENABLE    1301
#define ID_CHK_BOLD      1302
#define ID_RAD_RED       1401
#define ID_RAD_GREEN     1402
#define ID_RAD_BLUE      1403
#define ID_SLIDER        1501
#define ID_PROGRESS      1502
#define ID_TREE          1601
#define ID_TAB           1701
#define ID_STATUSBAR     1801

static HWND hBtnClick, hBtnClear, hBtnAdd, hBtnRemove;
static HWND hEditInput, hEditMulti;
static HWND hListBox, hComboBox;
static HWND hChkEnable, hChkBold;
static HWND hRadRed, hRadGreen, hRadBlue;
static HWND hSlider, hProgress;
static HWND hTree, hTab, hStatusBar;
static HWND hStaticGroup, hStaticLabel, hStaticValue;
static HWND hMainWnd;
static HFONT hFontNormal, hFontBold;

static void CreateControls(HWND hwnd) {
    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);

    /* Static group box */
    hStaticGroup = CreateWindowExA(0, "STATIC", "Input Controls",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10, 5, 300, 18, hwnd, NULL, hInst, NULL);

    /* Label */
    hStaticLabel = CreateWindowExA(0, "STATIC", "Type something:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 25, 120, 20, hwnd, NULL, hInst, NULL);

    /* Single-line edit */
    hEditInput = CreateWindowExA(0, "EDIT", "",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        20, 48, 280, 24, hwnd, (HMENU)ID_EDIT_INPUT, hInst, NULL);

    /* Buttons row */
    hBtnClick = CreateWindowExA(0, "BUTTON", "Click Me",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        20, 80, 85, 28, hwnd, (HMENU)ID_BTN_CLICK, hInst, NULL);

    hBtnAdd = CreateWindowExA(0, "BUTTON", "Add Item",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        110, 80, 85, 28, hwnd, (HMENU)ID_BTN_ADD, hInst, NULL);

    hBtnRemove = CreateWindowExA(0, "BUTTON", "Remove",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        200, 80, 85, 28, hwnd, (HMENU)ID_BTN_REMOVE, hInst, NULL);

    hBtnClear = CreateWindowExA(0, "BUTTON", "Clear All",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        20, 115, 85, 28, hwnd, (HMENU)ID_BTN_CLEAR, hInst, NULL);

    /* Checkboxes */
    hChkEnable = CreateWindowExA(0, "BUTTON", "Enable List",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        110, 118, 100, 22, hwnd, (HMENU)ID_CHK_ENABLE, hInst, NULL);
    SendMessage(hChkEnable, BM_SETCHECK, BST_CHECKED, 0);

    hChkBold = CreateWindowExA(0, "BUTTON", "Bold Text",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        220, 118, 85, 22, hwnd, (HMENU)ID_CHK_BOLD, hInst, NULL);

    /* Radio buttons */
    CreateWindowExA(0, "STATIC", "Color:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 148, 270, 18, hwnd, NULL, hInst, NULL);

    hRadRed = CreateWindowExA(0, "BUTTON", "Red",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        30, 168, 60, 20, hwnd, (HMENU)ID_RAD_RED, hInst, NULL);
    hRadGreen = CreateWindowExA(0, "BUTTON", "Green",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        100, 168, 70, 20, hwnd, (HMENU)ID_RAD_GREEN, hInst, NULL);
    hRadBlue = CreateWindowExA(0, "BUTTON", "Blue",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        180, 168, 60, 20, hwnd, (HMENU)ID_RAD_BLUE, hInst, NULL);
    SendMessage(hRadRed, BM_SETCHECK, BST_CHECKED, 0);

    /* Static value display */
    hStaticValue = CreateWindowExA(0, "STATIC", "Value: 50",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        320, 25, 200, 20, hwnd, NULL, hInst, NULL);

    /* Slider (trackbar) */
    CreateWindowExA(0, "STATIC", "Slider:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        320, 48, 50, 20, hwnd, NULL, hInst, NULL);

    hSlider = CreateWindowExA(0, TRACKBAR_CLASSA, "",
        WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_TOOLTIPS,
        380, 45, 180, 30, hwnd, (HMENU)ID_SLIDER, hInst, NULL);
    SendMessage(hSlider, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
    SendMessage(hSlider, TBM_SETPOS, TRUE, 50);
    SendMessage(hSlider, TBM_SETTICFREQ, 10, 0);

    /* Progress bar */
    CreateWindowExA(0, "STATIC", "Progress:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        320, 80, 60, 20, hwnd, NULL, hInst, NULL);

    hProgress = CreateWindowExA(0, PROGRESS_CLASSA, "",
        WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
        385, 78, 175, 22, hwnd, (HMENU)ID_PROGRESS, hInst, NULL);
    SendMessage(hProgress, PBM_SETRANGE, 0, MAKELONG(0, 100));
    SendMessage(hProgress, PBM_SETPOS, 30, 0);
    SendMessage(hProgress, PBM_SETBARCOLOR, 0, RGB(0, 180, 0));

    /* Listbox */
    CreateWindowExA(0, "STATIC", "ListBox:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        320, 108, 60, 20, hwnd, NULL, hInst, NULL);

    hListBox = CreateWindowExA(0, "LISTBOX", "",
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_HASSTRINGS,
        320, 128, 240, 80, hwnd, (HMENU)ID_LIST_BOX, hInst, NULL);
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)"Apple");
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)"Banana");
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)"Cherry");
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)"Date");
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)"Elderberry");
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)"Fig");
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)"Grape");

    /* ComboBox */
    CreateWindowExA(0, "STATIC", "ComboBox:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 215, 70, 20, hwnd, NULL, hInst, NULL);

    hComboBox = CreateWindowExA(0, "COMBOBOX", "",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS,
        95, 212, 200, 120, hwnd, (HMENU)ID_COMBO_BOX, hInst, NULL);
    SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)"Small");
    SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)"Medium");
    SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)"Large");
    SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)"Extra Large");
    SendMessage(hComboBox, CB_SETCURSEL, 1, 0);

    /* Multi-line edit */
    CreateWindowExA(0, "STATIC", "Multi-line Edit:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        20, 240, 100, 20, hwnd, NULL, hInst, NULL);

    hEditMulti = CreateWindowExA(0, "EDIT", "This is a multi-line edit control.\r\nYou can type multiple lines here.\r\nTry scrolling!",
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL |
        ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
        20, 260, 275, 100, hwnd, (HMENU)ID_EDIT_MULTI, hInst, NULL);

    /* TreeView */
    CreateWindowExA(0, "STATIC", "TreeView:",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        320, 215, 60, 20, hwnd, NULL, hInst, NULL);

    hTree = CreateWindowExA(0, WC_TREEVIEWA, "",
        WS_CHILD | WS_VISIBLE | WS_BORDER | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT,
        320, 235, 240, 130, hwnd, (HMENU)ID_TREE, hInst, NULL);

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

    /* Tab control */
    hTab = CreateWindowExA(0, WC_TABCONTROLA, "",
        WS_CHILD | WS_VISIBLE,
        10, 370, 550, 110, hwnd, (HMENU)ID_TAB, hInst, NULL);

    TCITEMA tie = {0};
    tie.mask = TCIF_TEXT;
    tie.pszText = "General";
    TabCtrl_InsertItem(hTab, 0, &tie);
    tie.pszText = "Advanced";
    TabCtrl_InsertItem(hTab, 1, &tie);
    tie.pszText = "About";
    TabCtrl_InsertItem(hTab, 2, &tie);

    /* Status bar */
    hStatusBar = CreateWindowExA(0, STATUSCLASSNAMEA, "",
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0, hwnd, (HMENU)ID_STATUSBAR, hInst, NULL);
    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)"Ready");
    SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM)"Controls Demo - WinPE");
    int parts[] = {200, 400, -1};
    SendMessage(hStatusBar, SB_SETPARTS, 3, (LPARAM)parts);
    SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM)"Win32 API");
}

static BOOL CALLBACK SetFontProc(HWND hwnd, LPARAM lParam) {
    SendMessage(hwnd, WM_SETFONT, (WPARAM)lParam, TRUE);
    return TRUE;
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        CreateControls(hwnd);
        EnumChildWindows(hwnd, SetFontProc, (LPARAM)hFontNormal);
        return 0;

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, RGB(255, 255, 255));
        SetTextColor(hdc, RGB(0, 0, 0));
        return (LRESULT)GetStockObject(WHITE_BRUSH);
    }

    case WM_NOTIFY: {
        NMHDR *nm = (NMHDR *)lParam;
        if (nm->hwndFrom == hTab && nm->code == NM_CUSTOMDRAW) {
            NMTBCUSTOMDRAW *ntb = (NMTBCUSTOMDRAW *)lParam;
            if (ntb->nmcd.dwDrawStage == CDDS_PREPAINT)
                return CDRF_NOTIFYITEMDRAW;
            if (ntb->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
                ntb->clrText = RGB(0, 0, 0);
                ntb->clrBtnFace = RGB(255, 255, 255);
                ntb->clrBtnHighlight = RGB(255, 255, 255);
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
            char msg[512];
            wsprintfA(msg, "You typed: %s", text);
            MessageBoxA(hwnd, msg, "Button Clicked", MB_ICONINFORMATION);
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

        case ID_LIST_BOX:
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

        case ID_COMBO_BOX:
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

    case WM_DESTROY:
        DeleteObject(hFontNormal);
        DeleteObject(hFontBold);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    (void)hPrev; (void)lpCmd; (void)nShow;

    INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_BAR_CLASSES | ICC_TREEVIEW_CLASSES | ICC_TAB_CLASSES};
    InitCommonControlsEx(&icc);

    hFontNormal = CreateFontA(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, 0, 0, 0, 0, "Segoe UI");
    hFontBold = CreateFontA(13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, 0, 0, 0, 0, "Segoe UI");

    WNDCLASSEXA wc = {0};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "GuiDemoClass";
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    RegisterClassExA(&wc);

    HWND hwnd = CreateWindowExA(0, "GuiDemoClass", "Win32 GUI Controls Demo",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 530,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return (int)msg.wParam;
}
