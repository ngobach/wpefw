#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <shellapi.h>
#include <math.h>

#define WM_TRAYICON (WM_USER + 1)
#define IDI_TRAY    1
#define IDM_DEMO    1001
#define IDM_EXIT    1002
#define TIMER_ID    1

static HINSTANCE hInst;
static HWND hWnd;
static NOTIFYICONDATAA nid = {0};
static int frame = 0;

static void ShowMenu(HWND hwnd);

static void DrawScene(HDC hdc, int w, int h, int t) {
    /* Background gradient */
    for (int y = 0; y < h; y++) {
        int r = 20 + y / 8;
        int g = 20 + y / 6;
        int b = 40 + y / 4;
        HPEN hpen = CreatePen(PS_SOLID, 1, RGB(r, g, b));
        HPEN old = (HPEN)SelectObject(hdc, hpen);
        MoveToEx(hdc, 0, y, NULL);
        LineTo(hdc, w, y);
        SelectObject(hdc, old);
        DeleteObject(hpen);
    }

    /* Grid */
    HPEN hgrid = CreatePen(PS_DOT, 1, RGB(60, 60, 80));
    HPEN old = (HPEN)SelectObject(hdc, hgrid);
    for (int x = 0; x < w; x += 40) {
        MoveToEx(hdc, x, 0, NULL);
        LineTo(hdc, x, h);
    }
    for (int y = 0; y < h; y += 40) {
        MoveToEx(hdc, 0, y, NULL);
        LineTo(hdc, w, y);
    }
    SelectObject(hdc, old);
    DeleteObject(hgrid);

    int cx = w / 2, cy = h / 2;

    /* Orbiting circles */
    for (int i = 0; i < 12; i++) {
        double angle = (t * 3 + i * 30) * 3.14159 / 180.0;
        int orbit_r = 80 + i * 10;
        int px = cx + (int)(orbit_r * cos(angle));
        int py = cy + (int)(orbit_r * sin(angle));
        int sz = 8 + i * 2;
        HPEN hpen = CreatePen(PS_SOLID, 2, RGB(100 + i * 12, 200 - i * 10, 255 - i * 8));
        HBRUSH hbr = CreateSolidBrush(RGB(100 + i * 12, 200 - i * 10, 255 - i * 8));
        HPEN old2 = (HPEN)SelectObject(hdc, hpen);
        HBRUSH oldb = (HBRUSH)SelectObject(hdc, hbr);
        Ellipse(hdc, px - sz, py - sz, px + sz, py + sz);
        SelectObject(hdc, oldb);
        SelectObject(hdc, old2);
        DeleteObject(hbr);
        DeleteObject(hpen);
    }

    /* Rotating star */
    {
        double a = t * 5 * 3.14159 / 180.0;
        POINT pts[10];
        for (int i = 0; i < 10; i++) {
            double ang = a + i * 3.14159 / 5.0;
            double r = (i % 2 == 0) ? 60.0 : 25.0;
            pts[i].x = cx + (int)(r * cos(ang));
            pts[i].y = cy + (int)(r * sin(ang));
        }
        HPEN hpen = CreatePen(PS_SOLID, 2, RGB(255, 255, 0));
        HBRUSH hbr = CreateSolidBrush(RGB(255, 255, 0));
        HPEN old2 = (HPEN)SelectObject(hdc, hpen);
        HBRUSH oldb = (HBRUSH)SelectObject(hdc, hbr);
        Polygon(hdc, pts, 10);
        SelectObject(hdc, oldb);
        SelectObject(hdc, old2);
        DeleteObject(hbr);
        DeleteObject(hpen);
    }

    /* Sine wave */
    {
        HPEN hpen = CreatePen(PS_SOLID, 3, RGB(0, 255, 150));
        HPEN old2 = (HPEN)SelectObject(hdc, hpen);
        MoveToEx(hdc, 0, h - 80, NULL);
        for (int x = 0; x < w; x += 2) {
            double y = (h - 80) + 30 * sin((x + t * 5) * 0.03);
            LineTo(hdc, x, (int)y);
        }
        SelectObject(hdc, old2);
        DeleteObject(hpen);
    }

    /* Cosine wave */
    {
        HPEN hpen = CreatePen(PS_SOLID, 2, RGB(255, 100, 100));
        HPEN old2 = (HPEN)SelectObject(hdc, hpen);
        MoveToEx(hdc, 0, h - 80, NULL);
        for (int x = 0; x < w; x += 2) {
            double y = (h - 80) + 20 * cos((x + t * 3) * 0.05);
            LineTo(hdc, x, (int)y);
        }
        SelectObject(hdc, old2);
        DeleteObject(hpen);
    }

    /* Arcs and pies */
    for (int i = 0; i < 5; i++) {
        int ax = 30 + i * 55;
        int ay = h - 50;
        int startAngle = (t * 4 + i * 72) % 360;
        int sweepAngle = 90;
        HPEN hpen = CreatePen(PS_SOLID, 2, RGB(255, 200 - i * 30, i * 50));
        HBRUSH hbr = CreateSolidBrush(RGB(255, 200 - i * 30, i * 50));
        HPEN old2 = (HPEN)SelectObject(hdc, hpen);
        HBRUSH oldb = (HBRUSH)SelectObject(hdc, hbr);
        Pie(hdc, ax, ay - 30, ax + 40, ay + 10,
            ax + 20 + (int)(20 * cos(startAngle * 3.14159 / 180)),
            ay - 10 + (int)(20 * sin(startAngle * 3.14159 / 180)),
            ax + 20 + (int)(20 * cos((startAngle + sweepAngle) * 3.14159 / 180)),
            ay - 10 + (int)(20 * sin((startAngle + sweepAngle) * 3.14159 / 180)));
        SelectObject(hdc, oldb);
        SelectObject(hdc, old2);
        DeleteObject(hbr);
        DeleteObject(hpen);
    }

    /* Fractal-like recursive squares */
    {
        HPEN hpen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
        HPEN old2 = (HPEN)SelectObject(hdc, hpen);
        int size = 60;
        int x = w - size - 20;
        int y = 20;
        for (int i = 0; i < 6; i++) {
            Rectangle(hdc, x, y, x + size, y + size);
            int inset = size / 4;
            double rot = (t * 2 + i * 30) * 3.14159 / 180.0;
            int nx = x + inset + (int)(inset * cos(rot));
            int ny = y + inset + (int)(inset * sin(rot));
            x = nx; y = ny;
            size = size * 3 / 4;
            HPEN hpen2 = CreatePen(PS_SOLID, 1, RGB(100 + i * 25, 150 + i * 15, 200));
            SelectObject(hdc, hpen2);
            DeleteObject(hpen);
            hpen = hpen2;
        }
        SelectObject(hdc, old2);
        DeleteObject(hpen);
    }

    /* Moving particles */
    for (int i = 0; i < 30; i++) {
        int seed = i * 137 + 42;
        int px = ((seed * 7 + t * (2 + i % 3)) % w);
        int py = ((seed * 13 + t * (1 + i % 2)) % h);
        int brightness = (int)(128 + 127 * sin((t + i) * 0.1));
        HBRUSH hbr = CreateSolidBrush(RGB(brightness, brightness / 2, 255 - brightness));
        RECT rc = {px - 2, py - 2, px + 2, py + 2};
        FillRect(hdc, &rc, hbr);
        DeleteObject(hbr);
    }

    /* Title text */
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
    HFONT hFont = CreateFontA(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, 0, 0, 0, 0, "Arial");
    HFONT oldf = (HFONT)SelectObject(hdc, hFont);
    TextOutA(hdc, 10, 10, "GDI Stress Test - WinPE", 23);
    SelectObject(hdc, oldf);
    DeleteObject(hFont);

    /* Frame counter */
    HFONT hFontSm = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, 0, 0, 0, 0, "Consolas");
    oldf = (HFONT)SelectObject(hdc, hFontSm);
    char buf[64];
    wsprintfA(buf, "Frame: %d  |  Shapes: 12 orbits + star + waves + arcs + fractals + particles", t);
    SetTextColor(hdc, RGB(180, 180, 180));
    TextOutA(hdc, 10, h - 25, buf, lstrlenA(buf));
    SelectObject(hdc, oldf);
    DeleteObject(hFontSm);

    /* Status badge */
    {
        int bx = w - 120, by = 10;
        HBRUSH hbr = CreateSolidBrush(RGB(0, 0, 0));
        RECT rcb = {bx - 5, by - 5, bx + 115, by + 25};
        FillRect(hdc, &rcb, hbr);
        DeleteObject(hbr);
        HPEN hpen = CreatePen(PS_SOLID, 1, RGB(0, 255, 0));
        HPEN old2 = (HPEN)SelectObject(hdc, hpen);
        Rectangle(hdc, bx - 5, by - 5, bx + 115, by + 25);
        SelectObject(hdc, old2);
        DeleteObject(hpen);
        HFONT hFps = CreateFontA(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, 0, 0, 0, 0, "Consolas");
        oldf = (HFONT)SelectObject(hdc, hFps);
        SetTextColor(hdc, RGB(0, 255, 0));
        wsprintfA(buf, "GDI Active");
        TextOutA(hdc, bx, by, buf, lstrlenA(buf));
        SelectObject(hdc, oldf);
        DeleteObject(hFps);
    }
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP)
            ShowMenu(hwnd);
        return 0;

    case WM_TIMER:
        if (wParam == TIMER_ID) {
            frame++;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        int w = rc.right, h = rc.bottom;

        HDC hMem = CreateCompatibleDC(hdc);
        HBITMAP hBmp = CreateCompatibleBitmap(hdc, w, h);
        HBITMAP hOld = (HBITMAP)SelectObject(hMem, hBmp);

        DrawScene(hMem, w, h, frame);

        BitBlt(hdc, 0, 0, w, h, hMem, 0, 0, SRCCOPY);
        SelectObject(hMem, hOld);
        DeleteObject(hBmp);
        DeleteDC(hMem);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_SIZE:
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;

    case WM_MEASUREITEM: {
        MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT *)lParam;
        if (mis->CtlType == ODT_MENU) {
            mis->itemWidth = 100;
            mis->itemHeight = 22;
            return TRUE;
        }
        return FALSE;
    }

    case WM_DRAWITEM: {
        DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *)lParam;
        if (dis->CtlType == ODT_MENU) {
            HBRUSH hbr;
            if (dis->itemState & ODS_SELECTED)
                hbr = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
            else
                hbr = CreateSolidBrush(GetSysColor(COLOR_MENU));
            FillRect(dis->hDC, &dis->rcItem, hbr);
            DeleteObject(hbr);

            HICON hIcon = (HICON)dis->itemData;
            if (hIcon)
                DrawIconEx(dis->hDC, dis->rcItem.left + 4,
                    dis->rcItem.top + (dis->rcItem.bottom - dis->rcItem.top - 16) / 2,
                    hIcon, 16, 16, 0, NULL, DI_NORMAL);

            SetBkMode(dis->hDC, TRANSPARENT);
            SetTextColor(dis->hDC, (dis->itemState & ODS_SELECTED) ?
                GetSysColor(COLOR_HIGHLIGHTTEXT) : GetSysColor(COLOR_MENUTEXT));

            const char *text = (dis->itemID == IDM_DEMO) ? "GDI Demo" : "Exit";
            RECT rc = dis->rcItem;
            rc.left += 24;
            DrawTextA(dis->hDC, text, -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            return TRUE;
        }
        return FALSE;
    }

    case WM_DESTROY:
        KillTimer(hwnd, TIMER_ID);
        Shell_NotifyIconA(NIM_DELETE, &nid);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static void ShowMenu(HWND hwnd) {
    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd);

    HMENU hMenu = CreatePopupMenu();
    HICON hIconDemo = ExtractIconA(hInst, "shell32.dll", 13);
    HICON hIconExit = ExtractIconA(hInst, "shell32.dll", 280);
    AppendMenuA(hMenu, MF_OWNERDRAW, IDM_DEMO, (LPCSTR)(intptr_t)hIconDemo);
    AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(hMenu, MF_OWNERDRAW, IDM_EXIT, (LPCSTR)(intptr_t)hIconExit);
    SetMenuDefaultItem(hMenu, 0, TRUE);

    int cmd = TrackPopupMenu(hMenu,
        TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY,
        pt.x, pt.y, 0, hwnd, NULL);
    PostMessage(hwnd, WM_NULL, 0, 0);
    DestroyMenu(hMenu);

    if (cmd == IDM_DEMO) {
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
    } else if (cmd == IDM_EXIT) {
        DestroyWindow(hwnd);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    (void)hPrev; (void)lpCmd; (void)nShow;
    hInst = hInstance;

    WNDCLASSEXA wc = {0};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "GdiDemoClass";
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClassExA(&wc);

    hWnd = CreateWindowExA(0, "GdiDemoClass", "GDI Stress Test",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        640, 480, NULL, NULL, hInstance, NULL);

    nid.cbSize = sizeof(nid);
    nid.hWnd = hWnd;
    nid.uID = IDI_TRAY;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = ExtractIconA(hInstance, "shell32.dll", 1);
    lstrcpyA(nid.szTip, "GDI Demo");
    Shell_NotifyIconA(NIM_ADD, &nid);

    nid.uFlags |= NIF_INFO;
    lstrcpyA(nid.szInfoTitle, "GDI Demo");
    lstrcpyA(nid.szInfo, "Right-click tray > GDI Demo to start animation.");
    nid.dwInfoFlags = NIIF_INFO;
    Shell_NotifyIconA(NIM_MODIFY, &nid);
    nid.uFlags &= ~NIF_INFO;

    SetTimer(hWnd, TIMER_ID, 33, NULL);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return (int)msg.wParam;
}
