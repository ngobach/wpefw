#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

/* ---------------------------------------------------------------- */
/*  pestart - a custom Windows 10 style start menu for Windows PE   */
/*  Pure GDI, layered window, dark theme, smooth animations.        */
/* ---------------------------------------------------------------- */

#define MAX_ITEMS 64
#define MAX_PATH_LEN 256
#define OPEN_MS   220
#define CLOSE_MS  160
#define FLYOUT_MS 160

typedef enum { IT_PROGRAM, IT_FOLDER } ItemType;

typedef struct {
    ItemType type;
    char     name[64];
    char     target[MAX_PATH_LEN];
    int      tile;          /* 1 if shown as a live tile */
    HICON    hIcon;
    COLORREF tileColor;
    double   hoverList;     /* 0..1 animated hover (left list) */
    double   hoverTile;     /* 0..1 animated hover (right tile) */
} Item;

static Item    g_items[MAX_ITEMS];
static int     g_nitems = 0;

static int     g_filt[MAX_ITEMS];
static int     g_nfilt = 0;

static char    g_userName[64] = {0};
static COLORREF g_accent = RGB(0, 120, 215);

/* layout / hit testing */
typedef struct { RECT r; int kind; int idx; } Hit;
#define HIT_LIST 0
#define HIT_TILE 1
#define HIT_USER 2
#define HIT_POWER 3
#define HIT_FLY_SHUT 4
#define HIT_FLY_RESTART 5
#define HIT_SEARCH 6

static Hit   g_hits[160];
static int   g_nhits = 0;
static RECT  g_searchR, g_userR, g_powerR, g_flyoutR;

/* fonts */
static HFONT g_fNorm, g_fBold, g_fSmall, g_fTitle, g_fTile, g_fMDL2;

/* Segoe MDL2 Assets glyph codepoints */
#define GLYPH_POWER   0xE7E8   /* PowerButton */
#define GLYPH_RESTART 0xE72C   /* Refresh / circular arrow */

/* surface (layered) */
static HDC    g_hdc = NULL;
static HBITMAP g_hBmp = NULL;
static void  *g_bits = NULL;
static int    g_w = 0, g_h = 0;
static int    g_winX = 0, g_winY = 0;

/* cached panel (drawn once, blitted each frame) */
static HDC    g_panelDC = NULL;
static HBITMAP g_panelBmp = NULL;

/* state */
static HWND   g_hwnd = NULL;
static DWORD  g_openStart = 0;
static int    g_closing = 0;
static DWORD  g_closeStart = 0;
static int    g_flyout = 0;          /* target open state */
static double g_flyoutAnim = 0;      /* 0..1 animated */
static int    g_flyoutDy = 0;        /* current slide offset (shared by draw + hit) */
static int    g_scroll = 0;
static char   g_search[128] = {0};
static int    g_dirty = 1;
static int    g_everAnimating = 0;

/* hover animation for power button + flyout items */
static double g_powerHover = 0;
static double g_flyShutHover = 0;
static double g_flyRestHover = 0;

static double easeOutCubic(double t) { return 1.0 - pow(1.0 - t, 3.0); }

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
    it->hoverList = 0;
    it->hoverTile = 0;
    it->hIcon = GetIcon(target, t == IT_FOLDER);
    it->tileColor = g_palette[g_nitems % (sizeof(g_palette)/sizeof(g_palette[0]))];
    g_nitems++;
}

static void AddDefaults(void) {
    AddItem(IT_PROGRAM, "BootICE",    "X:\\apps\\bootice\\BOOTICE.exe",        1);
    AddItem(IT_PROGRAM, "DiskGenius", "X:\\apps\\diskgenius\\DiskGenius.exe",  1);
    AddItem(IT_PROGRAM, "Ghost",      "X:\\apps\\ghost\\GHOST.exe",            1);
    AddItem(IT_PROGRAM, "HWiNFO",     "X:\\apps\\hwinfo\\HWiNFO.exe",          1);
    AddItem(IT_PROGRAM, "WinNTSetup", "X:\\apps\\winntsetup\\WinNTSetup_x64.exe", 1);
    AddItem(IT_FOLDER,  "System (X:)", "X:\\",           0);
    AddItem(IT_FOLDER,  "Applications", "X:\\apps",      0);
    AddItem(IT_FOLDER,  "Program Files", "X:\\Program Files", 0);

    /* dummy entries for scroll testing */
    static const char *dummy[] = {
        "Notepad", "Calculator", "Paint", "Command Prompt", "Registry Editor",
        "Task Manager", "File Explorer", "Control Panel", "Device Manager",
        "Disk Management", "Event Viewer", "Services", "Group Policy",
        "Resource Monitor", "Performance Monitor", "System Information",
        "Remote Desktop", "Windows Terminal"
    };
    for (int i = 0; i < (int)(sizeof(dummy)/sizeof(dummy[0])); i++) {
        char tgt[64];
        wsprintfA(tgt, "X:\\apps\\dummy\\app%d.exe", i + 1);
        AddItem(IT_PROGRAM, dummy[i], tgt, 0);
    }
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

static void LoadConfig(void) {
    char path[MAX_PATH_LEN];
    if (!GetModuleFileNameA(NULL, path, MAX_PATH_LEN)) { AddDefaults(); return; }
    char *p = strrchr(path, '\\');
    if (p) lstrcpyA(p + 1, "pestart.cfg");
    else   lstrcpyA(path, "pestart.cfg");

    FILE *f = fopen(path, "r");
    if (!f) { AddDefaults(); return; }

    char line[512];
    int haveApp = 0;
    while (fgets(line, sizeof(line), f)) {
        int n = (int)strlen(line);
        while (n > 0 && (line[n-1] == '\n' || line[n-1] == '\r')) line[--n] = 0;
        if (n == 0 || line[0] == '#') continue;

        char *bar = strchr(line, '|');
        if (!bar) continue;
        *bar = 0;
        char *kind = line;
        char *rest = bar + 1;

        if (!strcmp(kind, "app") || !strcmp(kind, "tile")) {
            char *b2 = strchr(rest, '|');
            if (!b2) continue;
            *b2 = 0;
            AddItem(IT_PROGRAM, rest, b2 + 1, !strcmp(kind, "tile"));
            haveApp = 1;
        } else if (!strcmp(kind, "folder")) {
            char *b2 = strchr(rest, '|');
            if (!b2) continue;
            *b2 = 0;
            AddItem(IT_FOLDER, rest, b2 + 1, 0);
        } else if (!strcmp(kind, "name")) {
            lstrcpynA(g_userName, rest, sizeof(g_userName));
        } else if (!strcmp(kind, "accent")) {
            int r = 0, g = 0, b = 0;
            if (sscanf(rest, "%d,%d,%d", &r, &g, &b) == 3)
                g_accent = RGB((BYTE)r, (BYTE)g, (BYTE)b);
        }
    }
    fclose(f);
    if (!haveApp && g_nitems == 0) AddDefaults();
}

/* ---------------------------------------------------------------- */
/*  Drawing helpers                                                 */
/* ---------------------------------------------------------------- */

static void FillRoundGrad(HDC hdc, int x, int y, int w, int h, int r,
                          COLORREF c0, COLORREF c1) {
    HRGN rgn = CreateRoundRectRgn(x, y, x + w, y + h, r, r);
    SelectClipRgn(hdc, rgn);
    for (int i = 0; i < h; i++) {
        double t = (double)i / (double)h;
        int rr = (int)(GetRValue(c0) + (GetRValue(c1) - GetRValue(c0)) * t);
        int gg = (int)(GetGValue(c0) + (GetGValue(c1) - GetGValue(c0)) * t);
        int bb = (int)(GetBValue(c0) + (GetBValue(c1) - GetBValue(c0)) * t);
        HPEN p = CreatePen(PS_SOLID, 1, RGB(rr, gg, bb));
        HGDIOBJ o = SelectObject(hdc, p);
        MoveToEx(hdc, x, y + i, NULL);
        LineTo(hdc, x + w, y + i);
        SelectObject(hdc, o);
        DeleteObject(p);
    }
    SelectClipRgn(hdc, NULL);
    DeleteObject(rgn);
}

/* Like FillRoundGrad but with square bottom corners (flush to taskbar). */
static void FillRoundGradTop(HDC hdc, int x, int y, int w, int h, int r,
                             COLORREF c0, COLORREF c1) {
    HRGN rgn = CreateRoundRectRgn(x, y, x + w, y + h, r, r);
    HRGN bot = CreateRectRgn(x, y + h - r, x + w, y + h);
    HRGN full = CreateRectRgn(0, 0, 0, 0);
    CombineRgn(full, rgn, bot, RGN_OR);
    SelectClipRgn(hdc, full);
    for (int i = 0; i < h; i++) {
        double t = (double)i / (double)h;
        int rr = (int)(GetRValue(c0) + (GetRValue(c1) - GetRValue(c0)) * t);
        int gg = (int)(GetGValue(c0) + (GetGValue(c1) - GetGValue(c0)) * t);
        int bb = (int)(GetBValue(c0) + (GetBValue(c1) - GetBValue(c0)) * t);
        HPEN p = CreatePen(PS_SOLID, 1, RGB(rr, gg, bb));
        HGDIOBJ o = SelectObject(hdc, p);
        MoveToEx(hdc, x, y + i, NULL);
        LineTo(hdc, x + w, y + i);
        SelectObject(hdc, o);
        DeleteObject(p);
    }
    SelectClipRgn(hdc, NULL);
    DeleteObject(rgn);
    DeleteObject(bot);
    DeleteObject(full);
}

static void FillRound(HDC hdc, int x, int y, int w, int h, int r, COLORREF c) {
    HRGN rgn = CreateRoundRectRgn(x, y, x + w, y + h, r, r);
    SelectClipRgn(hdc, rgn);
    HBRUSH b = CreateSolidBrush(c);
    RECT rc = {x, y, x + w, y + h};
    FillRect(hdc, &rc, b);
    DeleteObject(b);
    SelectClipRgn(hdc, NULL);
    DeleteObject(rgn);
}

static void DrawTextC(HDC hdc, const char *s, RECT r, UINT fmt, HFONT f, COLORREF c) {
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, c);
    HGDIOBJ o = SelectObject(hdc, f);
    DrawTextA(hdc, s, -1, &r, fmt);
    SelectObject(hdc, o);
}

static COLORREF Mix(COLORREF a, COLORREF b, double t) {
    int r = (int)(GetRValue(a) + (GetRValue(b) - GetRValue(a)) * t);
    int g = (int)(GetGValue(a) + (GetGValue(b) - GetGValue(a)) * t);
    int bl = (int)(GetBValue(a) + (GetBValue(b) - GetBValue(a)) * t);
    return RGB((BYTE)r, (BYTE)g, (BYTE)bl);
}

/* Windows-style 4-pane logo */
static void DrawWinLogo(HDC hdc, int x, int y, int s, COLORREF col) {
    int q = s / 2 - 1;
    HBRUSH b = CreateSolidBrush(col);
    RECT r;
    r.left = x;        r.top = y;        r.right = x + q;     r.bottom = y + q;     FillRect(hdc, &r, b);
    r.left = x + q + 2;r.top = y;        r.right = x + s;     r.bottom = y + q;     FillRect(hdc, &r, b);
    r.left = x;        r.top = y + q + 2;r.right = x + q;     r.bottom = y + s;     FillRect(hdc, &r, b);
    r.left = x + q + 2;r.top = y + q + 2;r.right = x + s;     r.bottom = y + s;     FillRect(hdc, &r, b);
    DeleteObject(b);
}

static void DrawGlyph(HDC hdc, unsigned int cp, int cx, int cy, COLORREF col, HFONT f) {
    wchar_t w[2] = { (wchar_t)cp, 0 };
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, col);
    HGDIOBJ o = SelectObject(hdc, f);
    RECT r = { cx - 28, cy - 28, cx + 28, cy + 28 };
    DrawTextW(hdc, w, 1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, o);
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

static void ComputeLayout(void) {
    g_nhits = 0;
    int contentTop = PAD + SEARCH_H + PAD;
    int bottomTop = g_h - BOTTOM_H;
    int leftW = 270;
    int listX = PAD;
    int tilesX = listX + leftW + PAD;
    int tilesW = g_w - tilesX - PAD;

    g_searchR = (RECT){listX, PAD, listX + leftW + PAD + tilesW, PAD + SEARCH_H};

    /* filtered list */
    g_nfilt = 0;
    for (int i = 0; i < g_nitems; i++) {
        if (g_search[0] == 0 || StrIStr(g_items[i].name, g_search))
            g_filt[g_nfilt++] = i;
    }

    int startY = contentTop + HEADER_H - g_scroll;
    for (int i = 0; i < g_nfilt; i++) {
        int y = startY + i * ROW_H;
        if (y + ROW_H <= contentTop + HEADER_H || y >= bottomTop) continue;
        Hit h = {{listX, y, listX + leftW, y + ROW_H}, HIT_LIST, g_filt[i]};
        if (g_nhits < 160) g_hits[g_nhits++] = h;
    }

    /* tiles (2 columns) */
    int tileGap = 12, tileH = 74;
    int tileW = (tilesW - tileGap) / 2;
    int col = 0, row = 0, drawn = 0;
    for (int i = 0; i < g_nitems && drawn < 8; i++) {
        if (g_items[i].type != IT_PROGRAM || !g_items[i].tile) continue;
        int tx = tilesX + col * (tileW + tileGap);
        int ty = contentTop + HEADER_H + row * (tileH + tileGap);
        if (ty + tileH > bottomTop) break;
        Hit h = {{tx, ty, tx + tileW, ty + tileH}, HIT_TILE, i};
        if (g_nhits < 160) g_hits[g_nhits++] = h;
        col++; if (col == 2) { col = 0; row++; }
        drawn++;
    }

    /* bottom bar */
    g_userR  = (RECT){PAD, bottomTop + 8, PAD + 240, g_h - 8};
    g_powerR = (RECT){g_w - PAD - 44, bottomTop + 8, g_w - PAD, g_h - 8};

    /* search hit */
    Hit hs = {g_searchR, HIT_SEARCH, -1};
    if (g_nhits < 160) g_hits[g_nhits++] = hs;

    /* flyout */
    if (g_flyout) {
        int fw = 210, fh = 104;
        int fy0 = g_powerR.top - 8 - fh + g_flyoutDy;
        g_flyoutR = (RECT){g_powerR.right - fw, fy0, g_powerR.right, fy0 + fh};
        Hit hs1 = {g_flyoutR, HIT_FLY_SHUT, -1};
        if (g_nhits < 160) g_hits[g_nhits++] = hs1;
        Hit hs2 = {{g_flyoutR.left, g_flyoutR.top + fh/2, g_flyoutR.right, g_flyoutR.bottom}, HIT_FLY_RESTART, -1};
        if (g_nhits < 160) g_hits[g_nhits++] = hs2;
    }
}

static Hit *HitTest(int x, int y) {
    for (int i = g_nhits - 1; i >= 0; i--) {
        Hit *h = &g_hits[i];
        if (x >= h->r.left && x <= h->r.right && y >= h->r.top && y <= h->r.bottom)
            return h;
    }
    return NULL;
}

/* ---------------------------------------------------------------- */
/*  Render                                                          */
/* ---------------------------------------------------------------- */

static void RenderScene(double openE) {
    ComputeLayout();

    /* clear to transparent */
    memset(g_bits, 0, (size_t)g_w * g_h * 4);

    int dy = (int)((1.0 - openE) * 46);

    HDC hdc = g_hdc;

    /* panel background: blit cached gradient (slides up with dy) */
    BitBlt(hdc, 0, dy, g_w, g_h - dy, g_panelDC, 0, 0, SRCCOPY);

    if (dy < g_h) {
        /* search box */
        FillRound(hdc, g_searchR.left, g_searchR.top + dy,
                  g_searchR.right - g_searchR.left, SEARCH_H, 8, RGB(48, 48, 56));
        /* magnifier */
        DrawWinLogo(hdc, g_searchR.left + 14, g_searchR.top + dy + 11, 16, g_accent);
        {
            RECT tr = {g_searchR.left + 40, g_searchR.top + dy,
                       g_searchR.right - 12, g_searchR.top + dy + SEARCH_H};
            if (g_search[0]) {
                char buf[160];
                lstrcpynA(buf, g_search, sizeof(buf));
                DrawTextC(hdc, buf, tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE,
                          g_fNorm, RGB(235, 235, 240));
            } else {
                DrawTextC(hdc, "Search apps and files", tr,
                          DT_LEFT | DT_VCENTER | DT_SINGLELINE, g_fNorm,
                          RGB(150, 150, 160));
            }
        }

        int contentTop = PAD + SEARCH_H + PAD;
        int bottomTop = g_h - BOTTOM_H;
        int listTop = contentTop + HEADER_H;

        /* left list header (sticky) */
        {
            RECT tr = {PAD, contentTop + dy, PAD + 200, listTop + dy};
            DrawTextC(hdc, "All Apps", tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE,
                      g_fBold, RGB(210, 210, 220));
        }

        /* list items (clipped to area below the header) */
        {
            HRGN cl = CreateRectRgn(PAD, listTop + dy, PAD + 270, bottomTop + dy);
            SelectClipRgn(hdc, cl);
            for (int i = 0; i < g_nhits; i++) {
                Hit *h = &g_hits[i];
                if (h->kind != HIT_LIST) continue;
                Item *it = &g_items[h->idx];
                int x = h->r.left, y = h->r.top + dy, w = h->r.right - h->r.left, hh = h->r.bottom - h->r.top;
                if (it->hoverList > 0.01)
                    FillRound(hdc, x + 2, y + 2, w - 4, hh - 4, 6,
                              Mix(RGB(40, 40, 46), RGB(70, 70, 82), it->hoverList));
                if (it->hIcon)
                    DrawIconEx(hdc, x + 8, y + (hh - 24) / 2, it->hIcon, 24, 24, 0, NULL, DI_NORMAL);
                RECT tr = {x + 40, y, x + w - 8, y + hh};
                DrawTextC(hdc, it->name, tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE,
                          g_fNorm, RGB(225, 225, 232));
            }
            SelectClipRgn(hdc, NULL);
            DeleteObject(cl);
        }

        /* tiles */
        int tilesX = PAD + 270 + PAD;
        RECT trh = {tilesX, contentTop + dy, g_w - PAD, listTop + dy};
        DrawTextC(hdc, "Pinned", trh, DT_LEFT | DT_VCENTER | DT_SINGLELINE,
                  g_fBold, RGB(210, 210, 220));

        for (int i = 0; i < g_nhits; i++) {
            Hit *h = &g_hits[i];
            if (h->kind != HIT_TILE) continue;
            Item *it = &g_items[h->idx];
            int x = h->r.left, y = h->r.top + dy, w = h->r.right - h->r.left, hh = h->r.bottom - h->r.top;
            COLORREF base = it->tileColor;
            COLORREF hot = Mix(base, RGB(255, 255, 255), 0.18);
            FillRound(hdc, x, y, w, hh, 8, Mix(base, hot, it->hoverTile));
            if (it->hIcon)
                DrawIconEx(hdc, x + 10, y + 10, it->hIcon, 28, 28, 0, NULL, DI_NORMAL);
            RECT tr = {x + 8, y + hh - 24, x + w - 8, y + hh - 6};
            DrawTextC(hdc, it->name, tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE,
                      g_fTile, RGB(255, 255, 255));
        }

        /* bottom bar */
        FillRound(hdc, 0, bottomTop + dy, g_w, BOTTOM_H, 0, RGB(26, 26, 31));
        /* user */
        {
            int ux = g_userR.left, uy = g_userR.top + dy, uh = g_userR.bottom - g_userR.top;
            int rad = uh - 8;
            HRGN rgn = CreateEllipticRgn(ux, uy, ux + rad, uy + rad);
            SelectClipRgn(hdc, rgn);
            FillRoundGrad(hdc, ux, uy, rad, rad, rad, g_accent, Mix(g_accent, RGB(0,0,0), 0.45));
            SelectClipRgn(hdc, NULL);
            DeleteObject(rgn);
            /* initials */
            char ini[3] = {0};
            if (g_userName[0]) {
                ini[0] = g_userName[0];
                char *sp = strchr(g_userName, ' ');
                if (sp && *(sp+1)) ini[1] = *(sp+1);
            } else lstrcpyA(ini, "U");
            RECT tr = {ux, uy, ux + rad, uy + rad};
            DrawTextC(hdc, ini, tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE, g_fBold, RGB(255,255,255));
            RECT trn = {ux + rad + 10, uy, g_userR.right, uy + rad};
            DrawTextC(hdc, g_userName[0] ? g_userName : "User", trn,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE, g_fNorm, RGB(225,225,232));
        }
        /* power button */
        {
            int px = g_powerR.left, py = g_powerR.top + dy, pw = g_powerR.right - g_powerR.left, ph = g_powerR.bottom - g_powerR.top;
            FillRound(hdc, px, py, pw, ph, 8, Mix(RGB(48,48,56), RGB(80,80,92), g_powerHover));
            DrawGlyph(hdc, GLYPH_POWER, px + pw/2, py + ph/2, Mix(RGB(220,220,228), RGB(255,255,255), g_powerHover), g_fMDL2);
        }

        /* flyout */
        if (g_flyoutAnim > 0.01) {
            int fx = g_flyoutR.left, fy = g_flyoutR.top, fw = g_flyoutR.right - g_flyoutR.left, fh = g_flyoutR.bottom - g_flyoutR.top;
            int midY = fy + fh / 2;
            FillRound(hdc, fx, fy, fw, fh, 8, RGB(44, 44, 52));
            if (g_flyShutHover > 0.01)
                FillRound(hdc, fx + 4, fy + 4, fw - 8, fh/2 - 6, 6,
                          Mix(RGB(44,44,52), RGB(70,70,82), g_flyShutHover));
            if (g_flyRestHover > 0.01)
                FillRound(hdc, fx + 4, midY + 2, fw - 8, fh/2 - 6, 6,
                          Mix(RGB(44,44,52), RGB(70,70,82), g_flyRestHover));
            DrawGlyph(hdc, GLYPH_POWER, fx + 26, fy + fh/4, RGB(230,90,90), g_fMDL2);
            RECT trs = {fx + 48, fy, fx + fw - 8, midY};
            DrawTextC(hdc, "Shut down", trs, DT_LEFT | DT_VCENTER | DT_SINGLELINE, g_fNorm, RGB(235,235,240));
            DrawGlyph(hdc, GLYPH_RESTART, fx + 26, fy + fh*3/4, RGB(120,200,120), g_fMDL2);
            RECT trr = {fx + 48, midY, fx + fw - 8, fy + fh};
            DrawTextC(hdc, "Restart", trr, DT_LEFT | DT_VCENTER | DT_SINGLELINE, g_fNorm, RGB(235,235,240));
            HPEN p = CreatePen(PS_SOLID, 1, RGB(60,60,68));
            HGDIOBJ o = SelectObject(hdc, p);
            MoveToEx(hdc, fx + 8, midY, NULL);
            LineTo(hdc, fx + fw - 8, midY);
            SelectObject(hdc, o);
            DeleteObject(p);
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
    g_closing = 1;
    g_closeStart = GetTickCount();
}

static void PowerAction(int restart) {
    const char *verb = restart ? "reboot" : "shutdown";
    HINSTANCE r = ShellExecuteA(g_hwnd, "open", "wpeutil", verb, NULL, SW_HIDE);
    if ((int)(INT_PTR)r <= 32) {
        /* fall back to PECMD */
        ShellExecuteA(g_hwnd, "open", "PECMD", restart ? "REBOOT" : "SHUT", NULL, SW_HIDE);
    }
    g_closing = 1;
    g_closeStart = GetTickCount();
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
        double openT = (double)(now - g_openStart) / OPEN_MS;
        if (openT > 1) openT = 1;
        double openE = easeOutCubic(openT);

        /* flyout animation */
        double fTarget = g_flyout ? 1.0 : 0.0;
        double fStep = 0.016 / (FLYOUT_MS / 1000.0);
        if (g_flyoutAnim < fTarget) g_flyoutAnim = min(1.0, g_flyoutAnim + fStep);
        else if (g_flyoutAnim > fTarget) g_flyoutAnim = max(0.0, g_flyoutAnim - fStep);
        g_flyoutDy = (int)((1.0 - g_flyoutAnim) * 30);

        /* hover transitions */
        int mx = -1, my = -1;
        POINT pt; GetCursorPos(&pt); ScreenToClient(hwnd, &pt); mx = pt.x; my = pt.y;
        Hit *hover = HitTest(mx, my);
        int animating = (openT < 1) || g_closing || (g_flyoutAnim > 0.01 && g_flyoutAnim < 0.99);
        for (int i = 0; i < g_nitems; i++) {
            double tList = (hover && hover->kind == HIT_LIST && hover->idx == i) ? 1.0 : 0.0;
            double tTile = (hover && hover->kind == HIT_TILE && hover->idx == i) ? 1.0 : 0.0;
            double cl = g_items[i].hoverList, ct = g_items[i].hoverTile;
            if (cl < tList) { cl = min(tList, cl + 0.12); animating = 1; }
            else if (cl > tList) { cl = max(tList, cl - 0.12); animating = 1; }
            if (ct < tTile) { ct = min(tTile, ct + 0.12); animating = 1; }
            else if (ct > tTile) { ct = max(tTile, ct - 0.12); animating = 1; }
            g_items[i].hoverList = cl;
            g_items[i].hoverTile = ct;
        }
        double tPow  = (hover && hover->kind == HIT_POWER) ? 1.0 : 0.0;
        double tShut = (hover && hover->kind == HIT_FLY_SHUT) ? 1.0 : 0.0;
        double tRest = (hover && hover->kind == HIT_FLY_RESTART) ? 1.0 : 0.0;
        g_powerHover   = (g_powerHover   < tPow)  ? min(tPow,  g_powerHover   + 0.12) : max(tPow,  g_powerHover   - 0.12);
        g_flyShutHover = (g_flyShutHover < tShut) ? min(tShut, g_flyShutHover + 0.12) : max(tShut, g_flyShutHover - 0.12);
        g_flyRestHover = (g_flyRestHover < tRest) ? min(tRest, g_flyRestHover + 0.12) : max(tRest, g_flyRestHover - 0.12);
        if (g_powerHover > 0.001 && g_powerHover < 0.999) animating = 1;
        if (g_flyShutHover > 0.001 && g_flyShutHover < 0.999) animating = 1;
        if (g_flyRestHover > 0.001 && g_flyRestHover < 0.999) animating = 1;

        int fade;
        if (g_closing) {
            double ct = (double)(now - g_closeStart) / CLOSE_MS;
            if (ct >= 1) { PostQuitMessage(0); return 0; }
            fade = (int)((1.0 - easeOutCubic(ct)) * 255);
        } else {
            fade = (int)(openE * 255);
        }

        if (g_dirty || animating) {
            RenderScene(openE);
            Present(fade);
            g_dirty = 0;
        } else if (g_everAnimating && !animating) {
            RenderScene(openE);
            Present(fade);
        }
        g_everAnimating = animating;
        return 0;
    }

    case WM_MOUSEMOVE:
        g_dirty = 1;
        return 0;

    case WM_MOUSEWHEEL: {
         int notches = (int)((signed short)HIWORD(wParam)) / 120;
         int visible = (g_h - BOTTOM_H) - (PAD + SEARCH_H + PAD + HEADER_H);
         int maxs = max(0, g_nfilt * ROW_H - visible);
         g_scroll = max(0, min(maxs, g_scroll - notches * 22));
         g_dirty = 1;
         return 0;
     }

    case WM_LBUTTONDOWN: {
        int x = (int)(short)LOWORD(lParam), y = (int)(short)HIWORD(lParam);
        Hit *h = HitTest(x, y);
        if (!h) return 0;
        switch (h->kind) {
        case HIT_LIST:
        case HIT_TILE:
            LaunchItem(h->idx);
            break;
        case HIT_POWER:
            g_flyout = !g_flyout;
            g_dirty = 1;
            break;
        case HIT_FLY_SHUT:
            PowerAction(0);
            break;
        case HIT_FLY_RESTART:
            PowerAction(1);
            break;
        case HIT_SEARCH:
            /* typing handled globally */
            break;
        }
        return 0;
    }

    case WM_CHAR: {
        if (wParam == 8) { /* backspace */
            int l = (int)strlen(g_search);
            if (l > 0) g_search[l - 1] = 0;
            g_scroll = 0;
            g_dirty = 1;
        } else if (wParam >= 32 && wParam < 127) {
            int l = (int)strlen(g_search);
            if (l < (int)sizeof(g_search) - 1) {
                g_search[l] = (char)wParam;
                g_search[l + 1] = 0;
            }
            g_scroll = 0;
            g_dirty = 1;
        }
        return 0;
    }

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            if (g_flyout) { g_flyout = 0; g_dirty = 1; }
            else { g_closing = 1; g_closeStart = GetTickCount(); }
        }
        return 0;

    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE && !g_closing) {
            g_closing = 1;
            g_closeStart = GetTickCount();
        }
        return 0;

    case WM_SETCURSOR:
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return TRUE;

    case WM_DESTROY:
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
    g_panelBmp = CreateDIBSection(g_panelDC, &bmi, DIB_RGB_COLORS, NULL, NULL, 0);
    SelectObject(g_panelDC, g_panelBmp);
    FillRoundGradTop(g_panelDC, 0, 0, g_w, g_h, RAD, RGB(34, 34, 40), RGB(22, 22, 27));
}

/* ---------------------------------------------------------------- */
/*  Entry point                                                     */
/* ---------------------------------------------------------------- */

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    (void)hPrev; (void)lpCmd; (void)nShow;

    LoadConfig();

    if (g_userName[0] == 0) {
        DWORD n = sizeof(g_userName);
        if (!GetUserNameA(g_userName, &n) || g_userName[0] == 0)
            lstrcpyA(g_userName, "User");
    }

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
    g_fSmall = CreateFontA(11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, "Segoe UI");
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
    DeleteObject(g_fNorm); DeleteObject(g_fBold); DeleteObject(g_fSmall);
    DeleteObject(g_fTitle); DeleteObject(g_fTile); DeleteObject(g_fMDL2);
    DeleteObject(g_hBmp); DeleteObject(g_panelBmp);
    DeleteDC(g_hdc); DeleteDC(g_panelDC);
    return (int)msg.wParam;
}
