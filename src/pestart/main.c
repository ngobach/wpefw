#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <objbase.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* ---------------------------------------------------------------- */
/*  pestart - a custom Windows 10 style start menu for Windows PE   */
/*  Pure GDI, layered window, dark theme, smooth animations.        */
/* ---------------------------------------------------------------- */

#define MAX_ITEMS 64
#define MAX_PATH_LEN 256
#define OPEN_MS   220
#define CLOSE_MS  160
#define FLYOUT_MS 100
#define WM_PESTART_TOGGLE (WM_APP + 1)

typedef enum { IT_PROGRAM, IT_FOLDER } ItemType;

typedef struct {
    ItemType type;
    char     name[64];
    char     target[MAX_PATH_LEN];
    int      tile;          /* 1 if shown as a live tile */
    HICON    hIcon;
    COLORREF tileColor;
} Item;

#define PE_IMGUI_IMPLEMENTATION
#include "pe_imgui.h"

static Item    g_items[MAX_ITEMS];
static int     g_nitems = 0;

static int     g_filt[MAX_ITEMS];
static int     g_nfilt = 0;

static char    g_userName[64] = {0};
COLORREF g_accent = RGB(0, 120, 215);

/* fonts */
HFONT g_fNorm, g_fBold, g_fTitle, g_fTile, g_fMDL2;

/* surface (layered) */
static HDC    g_hdc = NULL;
static HBITMAP g_hBmp = NULL;
void  *g_bits = NULL;
int    g_w = 0, g_h = 0;
static int    g_winX = 0, g_winY = 0;

/* cached panel (drawn once, blitted each frame) */
static HDC    g_panelDC = NULL;
static HBITMAP g_panelBmp = NULL;
static void  *g_panelBits = NULL;

/* state */
static HWND   g_hwnd = NULL;
static DWORD  g_openStart = 0;
static int    g_closing = 0;
static DWORD  g_closeStart = 0;
static int    g_flyout = 0;          /* target open state */
double g_flyoutAnim = 0;      /* 0..1 animated */
static int    g_flyoutDy = 0;        /* current slide offset (shared by draw + hit) */
double g_scroll = 0;
double g_targetScroll = 0;
static char   g_search[128] = {0};
int    g_dirty = 1;
static int    g_everAnimating = 0;


/* ---------------------------------------------------------------- */
/*  Icon helpers                                                    */
/* ---------------------------------------------------------------- */

static HICON GetIcon(const char *path, int folder) {
    HICON h = NULL;
    if (folder) {
        h = ExtractIconA(NULL, "shell32.dll", 4);
    } else {
        h = ExtractIconA(NULL, path, 0);
        if ((int)(INT_PTR)h <= 1)
            h = ExtractIconA(NULL, "shell32.dll", 1);
    }
    if (!h || (int)(INT_PTR)h <= 1)
        h = ExtractIconA(NULL, "shell32.dll", 1);
    return h;
}

/* ---------------------------------------------------------------- */
/*  Item management                                                 */
/* ---------------------------------------------------------------- */

static COLORREF g_palette[] = {
    RGB(0, 120, 215), RGB(16, 137, 62), RGB(106, 56, 180),
    RGB(197, 49, 60), RGB(0, 138, 138), RGB(200, 92, 0)
};

static void AddItem(ItemType t, const char *name, const char *target, int tile) {
    if (g_nitems >= MAX_ITEMS) return;
    Item *it = &g_items[g_nitems];
    it->type = t;
    lstrcpynA(it->name, name, sizeof(it->name));
    lstrcpynA(it->target, target, sizeof(it->target));
    it->tile = tile;
    it->hIcon = GetIcon(target, t == IT_FOLDER);
    it->tileColor = g_palette[g_nitems % (sizeof(g_palette)/sizeof(g_palette[0]))];
    g_nitems++;
}

static int StrIStr(const char *hay, const char *ndl) {
    if (!*ndl) return 1;
    int hl = (int)strlen(hay), nl = (int)strlen(ndl);
    for (int i = 0; i + nl <= hl; i++) {
        int ok = 1;
        for (int j = 0; j < nl; j++) {
            char a = hay[i + j]; if (a >= 'A' && a <= 'Z') a += 32;
            char b = ndl[j];     if (b >= 'A' && b <= 'Z') b += 32;
            if (a != b) { ok = 0; break; }
        }
        if (ok) return 1;
    }
    return 0;
}

static char g_logPath[MAX_PATH] = {0};
static int  g_logEnabled = 0;
static void LogInit(void) {
    g_logEnabled = 1;

    char drive[32] = {0};
    GetEnvironmentVariableA("SystemDrive", drive, sizeof(drive));
    if (!drive[0]) lstrcpyA(drive, "X:");
    wsprintfA(g_logPath, "%s\\pestart.log", drive);
    
    FILE *f = fopen(g_logPath, "w");
    if (f) {
        fprintf(f, "=== pestart log initialized ===\n");
        fclose(f);
    }
}

static void WriteLog(const char *fmt, ...) {
    if (!g_logEnabled || !g_logPath[0]) return;
    FILE *f = fopen(g_logPath, "a");
    if (f) {
        va_list args;
        va_start(args, fmt);
        vfprintf(f, fmt, args);
        va_end(args);
        fclose(f);
    }
}

static BOOL ResolveShortcut(const char *lnkPath, char *targetPath, HICON *phIcon) {
    IShellLinkA *pLink = NULL;
    IPersistFile *pPersist = NULL;
    BOOL success = FALSE;

    if (SUCCEEDED(CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkA, (void**)&pLink))) {
        if (SUCCEEDED(pLink->lpVtbl->QueryInterface(pLink, &IID_IPersistFile, (void**)&pPersist))) {
            wchar_t wPath[MAX_PATH];
            MultiByteToWideChar(CP_ACP, 0, lnkPath, -1, wPath, MAX_PATH);

            if (SUCCEEDED(pPersist->lpVtbl->Load(pPersist, wPath, STGM_READ))) {
                if (SUCCEEDED(pLink->lpVtbl->Resolve(pLink, NULL, SLR_NO_UI))) {
                    pLink->lpVtbl->GetPath(pLink, targetPath, MAX_PATH, NULL, 0);
                    
                    char iconPath[MAX_PATH] = {0};
                    int iconIdx = 0;
                    pLink->lpVtbl->GetIconLocation(pLink, iconPath, MAX_PATH, &iconIdx);

                    if (phIcon) {
                        *phIcon = NULL;
                        if (iconPath[0]) {
                            ExtractIconExA(iconPath, iconIdx, phIcon, NULL, 1);
                        }
                        if (!*phIcon && targetPath[0]) {
                            ExtractIconExA(targetPath, 0, phIcon, NULL, 1);
                        }
                    }
                    success = TRUE;
                }
            }
            pPersist->lpVtbl->Release(pPersist);
        }
        pLink->lpVtbl->Release(pLink);
    }
    return success;
}

static void ScanProgramsDir(const char *dirPath, int isPinnedFolder) {
    WriteLog("[SCAN] Directory: %s (isPinned=%d)\n", dirPath, isPinnedFolder);
    char searchPath[MAX_PATH];
    wsprintfA(searchPath, "%s\\*", dirPath);
    
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(searchPath, &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;
    
    do {
        if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, "..")) continue;
        
        char fullPath[MAX_PATH];
        wsprintfA(fullPath, "%s\\%s", dirPath, fd.cFileName);
        
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            int subIsPinned = isPinnedFolder || (lstrcmpiA(fd.cFileName, "Pinned") == 0);
            ScanProgramsDir(fullPath, subIsPinned);
        } else {
            char *ext = strrchr(fd.cFileName, '.');
            if (ext && lstrcmpiA(ext, ".lnk") == 0) {
                char target[MAX_PATH] = {0};
                HICON hIcon = NULL;
                if (ResolveShortcut(fullPath, target, &hIcon)) {
                    char name[64];
                    int len = (int)(ext - fd.cFileName);
                    if (len >= 64) len = 63;
                    lstrcpynA(name, fd.cFileName, len + 1);
                    
                    WriteLog("[FOUND] %s -> %s (Pinned=%d)\n", name, target, isPinnedFolder);
                    AddItem(IT_PROGRAM, name, target, isPinnedFolder);
                    if (g_nitems > 0 && hIcon) {
                        g_items[g_nitems - 1].hIcon = hIcon;
                    }
                } else {
                    WriteLog("[FAILED] To resolve shortcut: %s\n", fullPath);
                }
            }
        }
    } while (FindNextFileA(hFind, &fd));
    
    FindClose(hFind);
}




/* ---------------------------------------------------------------- */
/*  Layout                                                          */
/* ---------------------------------------------------------------- */

#define PAD 14
#define SEARCH_H 38
#define BOTTOM_H 56
#define ROW_H 44
#define RAD 12
#define HEADER_H 22



static void LaunchItem(int idx);
static void PowerAction(int restart);
static void LaunchShellFolder(const char *folder);

static void RenderScene(double openE) {
    /* clear to transparent */
    memset(g_bits, 0, (size_t)g_w * g_h * 4);

    int dy = (int)((1.0 - openE) * 46);
    g_ui.hdc = g_hdc;
    g_ui.dy = dy;

    HDC hdc = g_hdc;

    /* panel background: blit cached gradient (slides up with dy) */
    BitBlt(hdc, 0, dy, g_w, g_h - dy, g_panelDC, 0, 0, SRCCOPY);

    if (dy < g_h) {
        int contentTop = PAD + SEARCH_H + PAD;
        int bottomTop = g_h - BOTTOM_H;
        int leftW = 270;
        int listX = PAD;
        int tilesX = listX + leftW + PAD;
        int tilesW = g_w - tilesX - PAD;

        /* search box */
        RECT searchR = {listX, PAD, listX + leftW + PAD + tilesW, PAD + SEARCH_H};
        UI_InputText(1, searchR, g_search, sizeof(g_search), "Search apps and files");

        int listTop = contentTop + HEADER_H;

        /* left list header (sticky) */
        {
            RECT tr = {PAD, contentTop + dy, PAD + 200, listTop + dy};
            DrawTextC(hdc, "All Apps", tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE,
                      g_fBold, RGB(210, 210, 220));
        }

        /* filtered list count and items */
        g_nfilt = 0;
        for (int i = 0; i < g_nitems; i++) {
            if (g_search[0] == 0 || StrIStr(g_items[i].name, g_search))
                g_filt[g_nfilt++] = i;
        }

        /* list items (clipped to area below the header) */
        if (g_nfilt > 0) {
            HRGN cl = CreateRectRgn(PAD, listTop + dy, PAD + 270, bottomTop + dy);
            SelectClipRgn(hdc, cl);
            int startY = contentTop + HEADER_H - g_scroll;
            for (int i = 0; i < g_nfilt; i++) {
                int y = startY + i * ROW_H;
                if (y + ROW_H <= contentTop + HEADER_H || y >= bottomTop) continue;
                RECT itemR = {listX, y, listX + leftW, y + ROW_H};
                int idx = g_filt[i];
                if (UI_ListItem(100 + idx, itemR, g_items[idx].name, g_items[idx].hIcon)) {
                    LaunchItem(idx);
                }
            }
            SelectClipRgn(hdc, NULL);
            DeleteObject(cl);
        } else {
            /* Draw list empty state */
            int cx = listX + leftW / 2;
            int cy = listTop + 80 + dy;
            DrawGlyph(hdc, 0xE721, cx, cy, RGB(90, 90, 100), g_fMDL2);
            
            RECT rTitle = { listX + 10, cy + 20, listX + leftW - 10, cy + 40 };
            DrawTextC(hdc, "No apps found", rTitle, DT_CENTER | DT_SINGLELINE, g_fBold, RGB(160, 160, 170));
            
            RECT rSub = { listX + 20, cy + 42, listX + leftW - 20, cy + 100 };
            DrawTextC(hdc, "Check spelling or search terms.", rSub, DT_CENTER | DT_WORDBREAK, g_fNorm, RGB(110, 110, 120));
        }

        /* Pinned list header */
        {
            RECT trh = {tilesX, contentTop + dy, g_w - PAD, listTop + dy};
            DrawTextC(hdc, "Pinned", trh, DT_LEFT | DT_VCENTER | DT_SINGLELINE,
                      g_fBold, RGB(210, 210, 220));
        }

        int npinned = 0;
        for (int i = 0; i < g_nitems; i++) {
            if (g_items[i].type == IT_PROGRAM && g_items[i].tile) npinned++;
        }

        if (npinned > 0) {
            int tileGap = 12, tileH = 74;
            int tileW = (tilesW - tileGap) / 2;
            int col = 0, row = 0, drawn = 0;
            for (int i = 0; i < g_nitems && drawn < 8; i++) {
                if (g_items[i].type != IT_PROGRAM || !g_items[i].tile) continue;
                int tx = tilesX + col * (tileW + tileGap);
                int ty = contentTop + HEADER_H + row * (tileH + tileGap);
                if (ty + tileH > bottomTop) break;
                RECT tileR = {tx, ty, tx + tileW, ty + tileH};
                if (UI_Tile(200 + i, tileR, g_items[i].name, g_items[i].hIcon, g_items[i].tileColor)) {
                    LaunchItem(i);
                }
                col++; if (col == 2) { col = 0; row++; }
                drawn++;
            }
        } else {
            /* Draw tiles empty state */
            int cx = tilesX + tilesW / 2;
            int cy = listTop + 80 + dy;
            DrawGlyph(hdc, 0xE718, cx, cy, RGB(90, 90, 100), g_fMDL2);
            
            RECT rTitle = { tilesX + 10, cy + 20, tilesX + tilesW - 10, cy + 40 };
            DrawTextC(hdc, "No pinned apps", rTitle, DT_CENTER | DT_SINGLELINE, g_fBold, RGB(160, 160, 170));
            
            RECT rSub = { tilesX + 20, cy + 42, tilesX + tilesW - 20, cy + 120 };
            DrawTextC(hdc, "Put shortcuts in the Programs\\Pinned folder to pin them here.", rSub, DT_CENTER | DT_WORDBREAK, g_fNorm, RGB(110, 110, 120));
        }

        /* bottom bar */
        FillRound(hdc, 0, bottomTop + dy, g_w, BOTTOM_H, 0, RGB(16, 16, 20));
        
        /* bottom bar divider line */
        {
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(45, 45, 52));
            HPEN hOld = SelectObject(hdc, hPen);
            MoveToEx(hdc, 0, bottomTop + dy, NULL);
            LineTo(hdc, g_w, bottomTop + dy);
            SelectObject(hdc, hOld);
            DeleteObject(hPen);
        }
        
        /* user */
        RECT userR = {PAD, bottomTop + 8, PAD + 240, g_h - 8};
        {
            int ux = userR.left;
            int uy = bottomTop + 10 + dy;
            int rad = 36;
            FillRoundAA(g_bits, g_w, g_h, ux, uy, rad, rad, rad / 2, g_accent);
            /* initials */
            char ini[3] = {0};
            if (g_userName[0]) {
                ini[0] = g_userName[0];
                char *sp = strchr(g_userName, ' ');
                if (sp && *(sp+1)) ini[1] = *(sp+1);
            } else lstrcpyA(ini, "A");
            RECT tr = {ux, uy, ux + rad, uy + rad};
            DrawTextC(hdc, ini, tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE, g_fTitle, RGB(255,255,255));
            RECT trn = {ux + rad + 12, uy, userR.right, uy + rad};
            DrawTextC(hdc, g_userName[0] ? g_userName : "Administrator", trn,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE, g_fTitle, RGB(225,225,232));
        }

        /* button strip */
        int stripW = 44;
        int stripGap = 8;
        int stripY = bottomTop + 8;
        int stripH = g_h - 8 - stripY;
        int startX = g_w - PAD - (stripW * 4 + stripGap * 3);
        
        /* Button 1: My PC */
        RECT pcR = { startX, stripY, startX + stripW, stripY + stripH };
        if (UI_StripButton(30, pcR, 0xE7F4, 0)) {
            LaunchShellFolder("shell:MyComputerFolder");
        }
        
        /* Button 2: Downloads */
        startX += stripW + stripGap;
        RECT dlR = { startX, stripY, startX + stripW, stripY + stripH };
        if (UI_StripButton(31, dlR, 0xE896, 0)) {
            LaunchShellFolder("shell:Downloads");
        }
        
        /* Button 3: Desktop */
        startX += stripW + stripGap;
        RECT dtR = { startX, stripY, startX + stripW, stripY + stripH };
        if (UI_StripButton(32, dtR, 0xE80F, 0)) {
            LaunchShellFolder("shell:Desktop");
        }
        
        /* Button 4: Power */
        startX += stripW + stripGap;
        RECT powerR = { startX, stripY, startX + stripW, stripY + stripH };
        if (UI_StripButton(10, powerR, 0xE7E8, g_flyout)) {
            g_flyout = !g_flyout;
            g_dirty = 1;
        }

        /* flyout */
        if (g_flyoutAnim > 0.01) {
            int fw = 240, fh = 120;
            int fy0 = powerR.top - 16 - fh - g_flyoutDy;
            RECT flyoutR = {powerR.right - fw, fy0, powerR.right, fy0 + fh};
            int fx = flyoutR.left, fy = flyoutR.top, midY = fy + fh / 2;
            
            COLORREF bg = Mix(RGB(34, 34, 40), RGB(36, 36, 44), g_flyoutAnim);
            FillRoundAA(g_bits, g_w, g_h, fx, fy, fw, fh, 12, bg);
            
            RECT rShut = {fx, fy, fx + fw, midY};
            if (UI_FlyoutItem(11, rShut, GLYPH_POWER, RGB(225, 225, 232), "Shut down", "Closes all apps and turns off the PC", 1)) {
                PowerAction(0);
            }
            RECT rRest = {fx, midY, fx + fw, fy + fh};
            if (UI_FlyoutItem(12, rRest, GLYPH_RESTART, RGB(225, 225, 232), "Restart", "Closes apps, turns off, and boots again", 2)) {
                PowerAction(1);
            }
        }
    }

    /* opaque pass: any non-zero RGB -> opaque */
    unsigned char *b = (unsigned char *)g_bits;
    for (int i = 0; i < g_w * g_h; i++) {
        if (b[0] | b[1] | b[2]) b[3] = 255;
        b += 4;
    }
}


static void Present(int fade) {
    BLENDFUNCTION bf;
    bf.BlendOp = AC_SRC_OVER;
    bf.BlendFlags = 0;
    bf.SourceConstantAlpha = (BYTE)(fade < 0 ? 0 : (fade > 255 ? 255 : fade));
    bf.AlphaFormat = AC_SRC_ALPHA;
    POINT ptSrc = {0, 0};
    POINT ptDst = {g_winX, g_winY};
    SIZE sz = {g_w, g_h};
    UpdateLayeredWindow(g_hwnd, NULL, &ptDst, &sz, g_hdc, &ptSrc, 0, &bf, ULW_ALPHA);
}

/* ---------------------------------------------------------------- */
/*  Actions                                                         */
/* ---------------------------------------------------------------- */

static void LaunchItem(int idx) {
    Item *it = &g_items[idx];
    ShellExecuteA(g_hwnd, "open", it->target, NULL, NULL, SW_SHOWNORMAL);
    ShowWindow(g_hwnd, SW_HIDE);
}

static void LaunchShellFolder(const char *folder) {
    ShellExecuteA(g_hwnd, "open", "explorer.exe", folder, NULL, SW_SHOWNORMAL);
    ShowWindow(g_hwnd, SW_HIDE);
}

static void PowerAction(int restart) {
    const char *verb = restart ? "reboot" : "shutdown";
    HINSTANCE r = ShellExecuteA(g_hwnd, "open", "wpeutil", verb, NULL, SW_HIDE);
    if ((int)(INT_PTR)r <= 32) {
        ShellExecuteA(g_hwnd, "open", "PECMD", restart ? "REBOOT" : "SHUT", NULL, SW_HIDE);
    }
    ShowWindow(g_hwnd, SW_HIDE);
}

/* ---------------------------------------------------------------- */
/*  Window procedure                                                */
/* ---------------------------------------------------------------- */

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        g_hwnd = hwnd;
        g_openStart = GetTickCount();
        SetTimer(hwnd, 1, 16, NULL);
        return 0;

    case WM_TIMER: {
        DWORD now = GetTickCount();
        if (!IsWindowVisible(hwnd) && !g_closing) return 0;

        double openT = (double)(now - g_openStart) / OPEN_MS;
        if (openT > 1) openT = 1;
        double openE = easeOutCubic(openT);

        /* flyout animation */
        double fTarget = g_flyout ? 1.0 : 0.0;
        double fStep = 0.016 / (FLYOUT_MS / 1000.0);
        if (g_flyoutAnim < fTarget) g_flyoutAnim = min(1.0, g_flyoutAnim + fStep);
        else if (g_flyoutAnim > fTarget) g_flyoutAnim = max(0.0, g_flyoutAnim - fStep);
        g_flyoutDy = (int)((1.0 - easeOutCubic(g_flyoutAnim)) * 12);

        /* Update cursor position for IMGUI */
        POINT pt; GetCursorPos(&pt); ScreenToClient(hwnd, &pt);
        g_ui.mx = pt.x; g_ui.my = pt.y;

        /* smooth scroll interpolation */
        if (g_scroll != g_targetScroll) {
            double diff = g_targetScroll - g_scroll;
            double absDiff = diff < 0 ? -diff : diff;
            if (absDiff < 0.2) {
                g_scroll = g_targetScroll;
            } else {
                double step = diff * 0.18;
                double maxStep = 18.0;
                if (step > maxStep) step = maxStep;
                else if (step < -maxStep) step = -maxStep;
                g_scroll += step;
            }
            g_dirty = 1;
            g_ui.animating = 1;
        }

        g_ui.animating = 0;
        g_ui.hot = 0;

        int animating = (openT < 1) || g_closing || (g_flyoutAnim > 0.01 && g_flyoutAnim < 0.99) || (g_scroll != g_targetScroll);

        int fade;
        if (g_closing) {
            double ct = (double)(now - g_closeStart) / CLOSE_MS;
            if (ct >= 1) { ShowWindow(hwnd, SW_HIDE); g_closing = 0; return 0; }
            fade = (int)((1.0 - easeOutCubic(ct)) * 255);
        } else {
            fade = (int)(openE * 255);
        }

        if (g_dirty || animating || g_ui.animating) {
            RenderScene(openE);
            Present(fade);
            g_dirty = 0;
        } else if (g_everAnimating && !(animating || g_ui.animating)) {
            RenderScene(openE);
            Present(fade);
        }
        g_everAnimating = (animating || g_ui.animating);

        // Reset click transients at the end of the frame
        g_ui.mouseClicked = 0;
        g_ui.mouseReleased = 0;
        return 0;
    }

    case WM_MOUSEMOVE:
        g_dirty = 1;
        return 0;

    case WM_MOUSEWHEEL: {
         int listTop = PAD + SEARCH_H + PAD + HEADER_H;
         int bottomTop = g_h - BOTTOM_H;
         int leftW = 270;
         if (g_ui.mx >= PAD && g_ui.mx < PAD + leftW && g_ui.my >= listTop && g_ui.my < bottomTop) {
             int notches = (int)((signed short)HIWORD(wParam)) / 120;
             int visible = (g_h - BOTTOM_H) - (PAD + SEARCH_H + PAD + HEADER_H);
             int maxs = max(0, g_nfilt * ROW_H - visible);
             g_targetScroll = max(0, min(maxs, g_targetScroll - notches * 44));
             double maxDiff = 120.0;
             if (g_targetScroll - g_scroll > maxDiff) g_targetScroll = g_scroll + maxDiff;
             else if (g_targetScroll - g_scroll < -maxDiff) g_targetScroll = g_scroll - maxDiff;
             g_dirty = 1;
         }
         return 0;
     }

    case WM_LBUTTONDOWN: {
        g_ui.mouseDown = 1;
        g_ui.mouseClicked = 1;
        g_dirty = 1;
        return 0;
    }

    case WM_LBUTTONUP: {
        g_ui.mouseDown = 0;
        g_ui.mouseReleased = 1;
        g_dirty = 1;
        return 0;
    }

    case WM_CHAR: {
        if (wParam == 8) { /* backspace */
            int l = (int)strlen(g_search);
            if (l > 0) g_search[l - 1] = 0;
            g_scroll = 0;
            g_targetScroll = 0;
            g_dirty = 1;
        } else if (wParam >= 32 && wParam < 127) {
            int l = (int)strlen(g_search);
            if (l < (int)sizeof(g_search) - 1) {
                g_search[l] = (char)wParam;
                g_search[l + 1] = 0;
            }
            g_scroll = 0;
            g_targetScroll = 0;
            g_dirty = 1;
        }
        return 0;
    }

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            if (g_flyout) { g_flyout = 0; g_dirty = 1; }
            else { ShowWindow(hwnd, SW_HIDE); }
        }
        return 0;

    case WM_ACTIVATE:
        WriteLog("[WndProc] WM_ACTIVATE: wParam=%u, IsVisible=%d, now=%u\n",
                 (unsigned int)wParam, IsWindowVisible(hwnd), GetTickCount());
        if (LOWORD(wParam) == WA_INACTIVE && !g_closing) {
            ShowWindow(hwnd, SW_HIDE);
            SetPropA(hwnd, "LastHideTime", (HANDLE)(UINT_PTR)GetTickCount());
            WriteLog("[WndProc] WM_ACTIVATE: Set LastHideTime=%u\n", GetTickCount());
        }
        return 0;

    case WM_PESTART_TOGGLE: {
        DWORD now = GetTickCount();
        WriteLog("[WndProc] WM_PESTART_TOGGLE: IsVisible=%d, now=%u\n",
                 IsWindowVisible(hwnd), now);
        if (IsWindowVisible(hwnd)) {
            ShowWindow(hwnd, SW_HIDE);
            SetPropA(hwnd, "LastHideTime", (HANDLE)(UINT_PTR)now);
            WriteLog("[WndProc] WM_PESTART_TOGGLE: Set LastHideTime=%u\n", now);
        } else {
            DWORD lastHide = (DWORD)(UINT_PTR)GetPropA(hwnd, "LastHideTime");
            if (lastHide != 0 && now - lastHide < 500) {
                WriteLog("[WndProc] WM_PESTART_TOGGLE: Debounced show request! diff=%u\n", now - lastHide);
                return 0;
            }
            RECT wa;
            SystemParametersInfoA(SPI_GETWORKAREA, 0, &wa, 0);
            g_winX = wa.left;
            g_winY = wa.bottom - g_h;
            if (g_winY < wa.top) g_winY = wa.top;
            g_openStart = GetTickCount();
            g_closing = 0;
            g_flyout = 0;
            g_flyoutAnim = 0;
            g_dirty = 1;
            SetWindowPos(hwnd, HWND_TOPMOST, g_winX, g_winY, 0, 0, SWP_NOSIZE);
            RenderScene(0);
            Present(0);
            ShowWindow(hwnd, SW_SHOW);
            SetForegroundWindow(hwnd);
        }
        return 0;
    }

    case WM_ENDSESSION:
        PostQuitMessage(0);
        return 0;

    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;

    case WM_SETCURSOR:
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return TRUE;

    case WM_DESTROY:
        RemovePropA(hwnd, "LastHideTime");
        KillTimer(hwnd, 1);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

/* ---------------------------------------------------------------- */
/*  Cached panel surface                                            */
/* ---------------------------------------------------------------- */

static void BuildPanel(void) {
    g_panelDC = CreateCompatibleDC(NULL);
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = g_w;
    bmi.bmiHeader.biHeight = -g_h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    g_panelBmp = CreateDIBSection(g_panelDC, &bmi, DIB_RGB_COLORS, &g_panelBits, NULL, 0);
    SelectObject(g_panelDC, g_panelBmp);
    FillRoundGradTopAA(g_panelBits, g_w, g_h, 0, 0, g_w, g_h, RAD, RGB(34, 34, 40), RGB(22, 22, 27));
}

/* ---------------------------------------------------------------- */
/*  Entry point                                                     */
/* ---------------------------------------------------------------- */

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    (void)hPrev; (void)nShow;

    HWND hExisting = FindWindowA("PestartClass", NULL);
    if (hExisting) {
        SendMessage(hExisting, WM_PESTART_TOGGLE, 0, 0);
        return 0;
    }

    LogInit();
    CoInitialize(NULL);

    char pathUser[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROGRAMS, NULL, 0, pathUser)))
        ScanProgramsDir(pathUser, 0);

    lstrcpyA(g_userName, "Administrator");

    g_w = 560; g_h = 518;
    int sx = GetSystemMetrics(SM_CXSCREEN);
    int sy = GetSystemMetrics(SM_CYSCREEN);
    RECT wa = {0, 0, sx, sy};
    SystemParametersInfoA(SPI_GETWORKAREA, 0, &wa, 0);
    g_winX = wa.left;
    g_winY = wa.bottom - g_h;
    if (g_winY < wa.top) g_winY = wa.top;

    /* surface */
    g_hdc = CreateCompatibleDC(NULL);
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = g_w;
    bmi.bmiHeader.biHeight = -g_h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    g_hBmp = CreateDIBSection(g_hdc, &bmi, DIB_RGB_COLORS, &g_bits, NULL, 0);
    SelectObject(g_hdc, g_hBmp);
    BuildPanel();

    /* fonts */
    g_fNorm  = CreateFontA(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, "Segoe UI");
    g_fBold  = CreateFontA(13, 0, 0, 0, FW_BOLD,   FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, "Segoe UI");
    g_fTitle = CreateFontA(16, 0, 0, 0, FW_BOLD,   FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, "Segoe UI");
    g_fTile  = CreateFontA(12, 0, 0, 0, FW_BOLD,   FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, "Segoe UI");
    g_fMDL2  = CreateFontA(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, "Segoe MDL2 Assets");

    WNDCLASSEXA wc = {0};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "PestartClass";
    RegisterClassExA(&wc);

    g_hwnd = CreateWindowExA(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        "PestartClass", "pestart",
        WS_POPUP, g_winX, g_winY, g_w, g_h,
        NULL, NULL, hInstance, NULL);

    ShowWindow(g_hwnd, SW_SHOW);
    SetForegroundWindow(g_hwnd);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    for (int i = 0; i < g_nitems; i++)
        if (g_items[i].hIcon) DestroyIcon(g_items[i].hIcon);
    DeleteObject(g_fNorm); DeleteObject(g_fBold);
    DeleteObject(g_fTitle); DeleteObject(g_fTile); DeleteObject(g_fMDL2);
    DeleteObject(g_hBmp); DeleteObject(g_panelBmp);
    DeleteDC(g_hdc); DeleteDC(g_panelDC);
    CoUninitialize();
    return (int)msg.wParam;
}
