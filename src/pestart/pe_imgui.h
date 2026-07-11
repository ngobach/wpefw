#ifndef PE_IMGUI_H
#define PE_IMGUI_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* Segoe MDL2 Assets glyph codepoints */
#define GLYPH_POWER   0xE7E8   /* PowerButton */
#define GLYPH_RESTART 0xE72C   /* Refresh / circular arrow */

/* UI Context structure */
typedef struct {
    int mx, my;
    int mouseDown;
    int mouseClicked;
    int mouseReleased;
    int mouseScroll;
    int lastChar;
    int backspace;
    unsigned int hot;
    unsigned int active;
    HDC hdc;
    int dy;
    int animating;
} UIContext;

#ifdef PE_IMGUI_IMPLEMENTATION
UIContext g_ui;
double g_hoverStates[1024];
#else
extern UIContext g_ui;
extern double g_hoverStates[1024];
#endif

/* External dependencies expected from the main application */
extern HFONT g_fNorm, g_fBold, g_fSmall, g_fTitle, g_fTile, g_fMDL2;
extern COLORREF g_accent;
extern double g_scroll;
extern int g_dirty;
extern int g_w, g_h;
extern void *g_bits;
extern double g_flyoutAnim;

/* GDI Drawing Helper Declarations */
double easeOutCubic(double t);
COLORREF Mix(COLORREF a, COLORREF b, double t);
void FillRound(HDC hdc, int x, int y, int w, int h, int r, COLORREF c);
void FillRoundAA(void *bits, int bw, int bh, int x, int y, int w, int h, int r, COLORREF c);
void FillRoundGrad(HDC hdc, int x, int y, int w, int h, int r, COLORREF c0, COLORREF c1);
void FillRoundGradTop(HDC hdc, int x, int y, int w, int h, int r, COLORREF c0, COLORREF c1);
void FillRoundGradTopAA(void *bits, int bw, int bh, int x, int y, int w, int h, int r, COLORREF c0, COLORREF c1);
void DrawTextC(HDC hdc, const char *s, RECT r, UINT fmt, HFONT f, COLORREF c);
void DrawWinLogo(HDC hdc, int x, int y, int s, COLORREF col);
void DrawGlyph(HDC hdc, unsigned int cp, int cx, int cy, COLORREF col, HFONT f);

/* IMGUI Widget Declarations */
int UI_ListItem(unsigned int id, RECT r, const char *name, HICON hIcon);
int UI_Tile(unsigned int id, RECT r, const char *name, HICON hIcon, COLORREF baseColor);
void UI_InputText(unsigned int id, RECT r, char *buf, int maxLen, const char *placeholder);
int UI_StripButton(unsigned int id, RECT r, unsigned int glyph, int selected);
int UI_FlyoutItem(unsigned int id, RECT r, unsigned int glyph, COLORREF glyphColor, const char *title, const char *subtitle, int pos);

#endif /* PE_IMGUI_H */

/* ================================================================= */
/*  Implementation                                                   */
/* ================================================================= */
#ifdef PE_IMGUI_IMPLEMENTATION

#include <math.h>
#include <string.h>

double easeOutCubic(double t) {
    return 1.0 - pow(1.0 - t, 3.0);
}

COLORREF Mix(COLORREF a, COLORREF b, double t) {
    int r = (int)(GetRValue(a) + (GetRValue(b) - GetRValue(a)) * t);
    int g = (int)(GetGValue(a) + (GetGValue(b) - GetGValue(a)) * t);
    int bl = (int)(GetBValue(a) + (GetBValue(b) - GetBValue(a)) * t);
    return RGB((BYTE)r, (BYTE)g, (BYTE)bl);
}

void FillRound(HDC hdc, int x, int y, int w, int h, int r, COLORREF c) {
    HRGN rgn = CreateRoundRectRgn(x, y, x + w, y + h, r, r);
    SelectClipRgn(hdc, rgn);
    HBRUSH b = CreateSolidBrush(c);
    RECT rc = {x, y, x + w, y + h};
    FillRect(hdc, &rc, b);
    DeleteObject(b);
    SelectClipRgn(hdc, NULL);
    DeleteObject(rgn);
}

void FillRoundAA(void *bits, int bw, int bh, int x, int y, int w, int h, int r, COLORREF c) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > bw) w = bw - x;
    if (y + h > bh) h = bh - y;
    if (w <= 0 || h <= 0) return;

    unsigned char br = GetRValue(c);
    unsigned char bg = GetGValue(c);
    unsigned char bb = GetBValue(c);

    unsigned int *pixels = (unsigned int *)bits;

    for (int py = y; py < y + h; py++) {
        unsigned int *row = pixels + py * bw;
        int qy = (py < y + r) ? (y + r - py) : ((py >= y + h - r) ? (py - (y + h - r) + 1) : 0);
        
        for (int px = x; px < x + w; px++) {
            int qx = (px < x + r) ? (x + r - px) : ((px >= x + w - r) ? (px - (x + w - r) + 1) : 0);
            
            double opacity = 1.0;
            if (qx > 0 && qy > 0) {
                double dist = sqrt((qx - 0.5) * (qx - 0.5) + (qy - 0.5) * (qy - 0.5));
                if (dist > r + 0.5) {
                    opacity = 0.0;
                } else if (dist > r - 0.5) {
                    opacity = (r + 0.5 - dist);
                }
            }
            
            if (opacity > 0.0) {
                unsigned int *pixel = row + px;
                if (opacity >= 0.99) {
                    *pixel = bb | (bg << 8) | (br << 16);
                } else {
                    unsigned int bgPixel = *pixel;
                    unsigned char b_orig = bgPixel & 0xFF;
                    unsigned char g_orig = (bgPixel >> 8) & 0xFF;
                    unsigned char r_orig = (bgPixel >> 16) & 0xFF;
                    
                    unsigned char b_new = (unsigned char)(b_orig + (bb - b_orig) * opacity);
                    unsigned char g_new = (unsigned char)(g_orig + (bg - g_orig) * opacity);
                    unsigned char r_new = (unsigned char)(r_orig + (br - r_orig) * opacity);
                    
                    *pixel = b_new | (g_new << 8) | (r_new << 16);
                }
            }
        }
    }
}

void FillRoundGrad(HDC hdc, int x, int y, int w, int h, int r, COLORREF c0, COLORREF c1) {
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

void FillRoundGradTop(HDC hdc, int x, int y, int w, int h, int r, COLORREF c0, COLORREF c1) {
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

void FillRoundGradTopAA(void *bits, int bw, int bh, int x, int y, int w, int h, int r, COLORREF c0, COLORREF c1) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > bw) w = bw - x;
    if (y + h > bh) h = bh - y;
    if (w <= 0 || h <= 0) return;

    unsigned int *pixels = (unsigned int *)bits;

    for (int py = y; py < y + h; py++) {
        double t = (double)(py - y) / (double)h;
        int rr = (int)(GetRValue(c0) + (GetRValue(c1) - GetRValue(c0)) * t);
        int gg = (int)(GetGValue(c0) + (GetGValue(c1) - GetGValue(c0)) * t);
        int bb = (int)(GetBValue(c0) + (GetBValue(c1) - GetBValue(c0)) * t);

        unsigned int *row = pixels + py * bw;
        int qy = (py < y + r) ? (y + r - py) : 0; // Only round the top corners
        
        for (int px = x; px < x + w; px++) {
            int qx = (px < x + r) ? (x + r - px) : ((px >= x + w - r) ? (px - (x + w - r) + 1) : 0);
            
            double opacity = 1.0;
            if (qx > 0 && qy > 0) {
                double dist = sqrt((qx - 0.5) * (qx - 0.5) + (qy - 0.5) * (qy - 0.5));
                if (dist > r + 0.5) {
                    opacity = 0.0;
                } else if (dist > r - 0.5) {
                    opacity = (r + 0.5 - dist);
                }
            }
            
            if (opacity > 0.0) {
                unsigned int *pixel = row + px;
                if (opacity >= 0.99) {
                    *pixel = bb | (gg << 8) | (rr << 16);
                } else {
                    unsigned int bgPixel = *pixel;
                    unsigned char b_orig = bgPixel & 0xFF;
                    unsigned char g_orig = (bgPixel >> 8) & 0xFF;
                    unsigned char r_orig = (bgPixel >> 16) & 0xFF;
                    
                    unsigned char b_new = (unsigned char)(b_orig + (bb - b_orig) * opacity);
                    unsigned char g_new = (unsigned char)(g_orig + (gg - g_orig) * opacity);
                    unsigned char r_new = (unsigned char)(r_orig + (rr - r_orig) * opacity);
                    
                    *pixel = b_new | (g_new << 8) | (r_new << 16);
                }
            }
        }
    }
}

void DrawTextC(HDC hdc, const char *s, RECT r, UINT fmt, HFONT f, COLORREF c) {
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, c);
    HGDIOBJ o = SelectObject(hdc, f);
    DrawTextA(hdc, s, -1, &r, fmt);
    SelectObject(hdc, o);
}

void DrawWinLogo(HDC hdc, int x, int y, int s, COLORREF col) {
    int q = s / 2 - 1;
    HBRUSH b = CreateSolidBrush(col);
    RECT r;
    r.left = x;        r.top = y;        r.right = x + q;     r.bottom = y + q;     FillRect(hdc, &r, b);
    r.left = x + q + 2;r.top = y;        r.right = x + s;     r.bottom = y + q;     FillRect(hdc, &r, b);
    r.left = x;        r.top = y + q + 2;r.right = x + q;     r.bottom = y + s;     FillRect(hdc, &r, b);
    r.left = x + q + 2;r.top = y + q + 2;r.right = x + s;     r.bottom = y + s;     FillRect(hdc, &r, b);
    DeleteObject(b);
}

void DrawGlyph(HDC hdc, unsigned int cp, int cx, int cy, COLORREF col, HFONT f) {
    wchar_t w[2] = { (wchar_t)cp, 0 };
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, col);
    HGDIOBJ o = SelectObject(hdc, f);
    RECT r = { cx - 28, cy - 28, cx + 28, cy + 28 };
    DrawTextW(hdc, w, 1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, o);
}

static double UI_HoverAnimation(unsigned int id, int hovered) {
    double target = hovered ? 1.0 : 0.0;
    double current = g_hoverStates[id];
    if (current < target) {
        current = min(target, current + 0.12);
        g_ui.animating = 1;
    } else if (current > target) {
        current = max(target, current - 0.12);
        g_ui.animating = 1;
    }
    g_hoverStates[id] = current;
    return current;
}

int UI_ListItem(unsigned int id, RECT r, const char *name, HICON hIcon) {
    RECT drawR = { r.left, r.top + g_ui.dy, r.right, r.bottom + g_ui.dy };
    int listTop = 88;
    int bottomTop = g_h - 56;
    int hovered = (g_ui.mx >= drawR.left && g_ui.mx < drawR.right &&
                   g_ui.my >= drawR.top && g_ui.my < drawR.bottom &&
                   g_ui.my >= listTop + g_ui.dy && g_ui.my < bottomTop + g_ui.dy);

    if (hovered) {
        g_ui.hot = id;
        if (g_ui.active == 0 && g_ui.mouseClicked) {
            g_ui.active = id;
        }
    }

    int clicked = 0;
    if (g_ui.active == id && g_ui.mouseReleased) {
        if (hovered) clicked = 1;
        g_ui.active = 0;
    }

    double hover = UI_HoverAnimation(id, g_ui.hot == id);

    int x = drawR.left, y = drawR.top, w = drawR.right - drawR.left, h = drawR.bottom - drawR.top;
    if (hover > 0.01) {
        FillRoundAA(g_bits, g_w, g_h, x + 2, y + 2, w - 4, h - 4, 6,
                    Mix(RGB(40, 40, 46), RGB(70, 70, 82), hover));
    }
    if (hIcon) {
        DrawIconEx(g_ui.hdc, x + 8, y + (h - 24) / 2, hIcon, 24, 24, 0, NULL, DI_NORMAL);
    }
    RECT tr = { x + 40, y, x + w - 8, y + h };
    DrawTextC(g_ui.hdc, name, tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE, g_fNorm, RGB(225, 225, 232));

    return clicked;
}

int UI_Tile(unsigned int id, RECT r, const char *name, HICON hIcon, COLORREF baseColor) {
    RECT drawR = { r.left, r.top + g_ui.dy, r.right, r.bottom + g_ui.dy };
    int hovered = (g_ui.mx >= drawR.left && g_ui.mx < drawR.right &&
                   g_ui.my >= drawR.top && g_ui.my < drawR.bottom);

    if (hovered) {
        g_ui.hot = id;
        if (g_ui.active == 0 && g_ui.mouseClicked) {
            g_ui.active = id;
        }
    }

    int clicked = 0;
    if (g_ui.active == id && g_ui.mouseReleased) {
        if (hovered) clicked = 1;
        g_ui.active = 0;
    }

    double hover = UI_HoverAnimation(id, g_ui.hot == id);

    int x = drawR.left, y = drawR.top, w = drawR.right - drawR.left, h = drawR.bottom - drawR.top;
    COLORREF hot = Mix(baseColor, RGB(255, 255, 255), 0.18);
    FillRoundAA(g_bits, g_w, g_h, x, y, w, h, 8, Mix(baseColor, hot, hover));
    if (hIcon) {
        DrawIconEx(g_ui.hdc, x + 10, y + 10, hIcon, 28, 28, 0, NULL, DI_NORMAL);
    }
    RECT tr = { x + 8, y + h - 24, x + w - 8, y + h - 6 };
    DrawTextC(g_ui.hdc, name, tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE, g_fTile, RGB(255, 255, 255));

    return clicked;
}

void UI_InputText(unsigned int id, RECT r, char *buf, int maxLen, const char *placeholder) {
    RECT drawR = { r.left, r.top + g_ui.dy, r.right, r.bottom + g_ui.dy };

    int x = drawR.left, y = drawR.top, w = drawR.right - drawR.left, h = drawR.bottom - drawR.top;
    FillRoundAA(g_bits, g_w, g_h, x, y, w, h, 8, RGB(48, 48, 56));
    DrawWinLogo(g_ui.hdc, x + 14, y + 11, 16, g_accent);

    RECT tr = { x + 40, y, drawR.right - 12, y + h };
    if (buf[0]) {
        DrawTextC(g_ui.hdc, buf, tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE, g_fNorm, RGB(235, 235, 240));
    } else {
        DrawTextC(g_ui.hdc, placeholder, tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE, g_fNorm, RGB(150, 150, 160));
    }
}

int UI_StripButton(unsigned int id, RECT r, unsigned int glyph, int selected) {
    RECT drawR = { r.left, r.top + g_ui.dy, r.right, r.bottom + g_ui.dy };
    int hovered = (g_ui.mx >= drawR.left && g_ui.mx < drawR.right &&
                   g_ui.my >= drawR.top && g_ui.my < drawR.bottom);

    if (hovered) {
        g_ui.hot = id;
        if (g_ui.active == 0 && g_ui.mouseClicked) {
            g_ui.active = id;
        }
    }

    int clicked = 0;
    if (g_ui.active == id && g_ui.mouseReleased) {
        if (hovered) clicked = 1;
        g_ui.active = 0;
    }

    double hover = UI_HoverAnimation(id, g_ui.hot == id);

    int x = drawR.left, y = drawR.top, w = drawR.right - drawR.left, h = drawR.bottom - drawR.top;
    
    COLORREF bg;
    if (selected) {
        bg = Mix(RGB(48, 48, 56), g_accent, 0.7);
    } else {
        bg = Mix(RGB(48, 48, 56), RGB(80, 80, 92), hover);
    }
    
    FillRoundAA(g_bits, g_w, g_h, x, y, w, h, 8, bg);
    
    COLORREF glyphCol;
    if (selected) {
        glyphCol = RGB(255, 255, 255);
    } else {
        glyphCol = Mix(RGB(220, 220, 228), RGB(255, 255, 255), hover);
    }
    
    DrawGlyph(g_ui.hdc, glyph, x + w / 2, y + h / 2, glyphCol, g_fMDL2);

    return clicked;
}

int UI_FlyoutItem(unsigned int id, RECT r, unsigned int glyph, COLORREF glyphColor, const char *title, const char *subtitle, int pos) {
    int hovered = (g_ui.mx >= r.left && g_ui.mx < r.right &&
                   g_ui.my >= r.top && g_ui.my < r.bottom);

    if (hovered) {
        g_ui.hot = id;
        if (g_ui.active == 0 && g_ui.mouseClicked) {
            g_ui.active = id;
        }
    }

    int clicked = 0;
    if (g_ui.active == id && g_ui.mouseReleased) {
        if (hovered) clicked = 1;
        g_ui.active = 0;
    }

    double hover = UI_HoverAnimation(id, g_ui.hot == id);

    int x = r.left, y = r.top, w = r.right - r.left, h = r.bottom - r.top;
    
    COLORREF bgBase = RGB(36, 36, 44);
    COLORREF cTitle = Mix(bgBase, RGB(245, 245, 250), g_flyoutAnim);
    COLORREF cSub = Mix(bgBase, RGB(160, 160, 175), g_flyoutAnim);
    COLORREF cGlyph = Mix(bgBase, glyphColor, g_flyoutAnim);

    if (hover > 0.01) {
        COLORREF hoverColor = Mix(RGB(44, 44, 52), RGB(60, 60, 72), hover);
        COLORREF bgBlend = Mix(bgBase, hoverColor, g_flyoutAnim);
        
        int padL = 4, padR = 4;
        int rad = 8;
        int bx = x + padL;
        int bw = w - padL - padR;
        
        if (pos == 1) { // top item: round top, square bottom, top pad 4, bottom pad 2
            int by = y + 4;
            int bh = h - 4 - 2;
            
            HRGN roundRgn = CreateRoundRectRgn(bx, by, bx + bw, by + bh + rad, rad, rad);
            HRGN clipRgn = CreateRectRgn(bx, by, bx + bw, by + bh);
            HRGN finalRgn = CreateRectRgn(0, 0, 0, 0);
            CombineRgn(finalRgn, roundRgn, clipRgn, RGN_AND);
            
            SelectClipRgn(g_ui.hdc, finalRgn);
            HBRUSH brush = CreateSolidBrush(bgBlend);
            RECT rc = { bx, by, bx + bw, by + bh };
            FillRect(g_ui.hdc, &rc, brush);
            DeleteObject(brush);
            
            SelectClipRgn(g_ui.hdc, NULL);
            DeleteObject(roundRgn);
            DeleteObject(clipRgn);
            DeleteObject(finalRgn);
        } else if (pos == 2) { // bottom item: square top, round bottom, top pad 2, bottom pad 4
            int by = y + 2;
            int bh = h - 2 - 4;
            
            HRGN roundRgn = CreateRoundRectRgn(bx, by - rad, bx + bw, by + bh, rad, rad);
            HRGN clipRgn = CreateRectRgn(bx, by, bx + bw, by + bh);
            HRGN finalRgn = CreateRectRgn(0, 0, 0, 0);
            CombineRgn(finalRgn, roundRgn, clipRgn, RGN_AND);
            
            SelectClipRgn(g_ui.hdc, finalRgn);
            HBRUSH brush = CreateSolidBrush(bgBlend);
            RECT rc = { bx, by, bx + bw, by + bh };
            FillRect(g_ui.hdc, &rc, brush);
            DeleteObject(brush);
            
            SelectClipRgn(g_ui.hdc, NULL);
            DeleteObject(roundRgn);
            DeleteObject(clipRgn);
            DeleteObject(finalRgn);
        } else { // middle item: square all corners, top/bottom pad 2
            int by = y + 2;
            int bh = h - 4;
            
            HBRUSH brush = CreateSolidBrush(bgBlend);
            RECT rc = { bx, by, bx + bw, by + bh };
            FillRect(g_ui.hdc, &rc, brush);
            DeleteObject(brush);
        }
    }
    
    DrawGlyph(g_ui.hdc, glyph, x + 24, y + h / 2, cGlyph, g_fMDL2);
    
    RECT trTitle = { x + 48, y + 11, r.right - 8, y + 30 };
    DrawTextC(g_ui.hdc, title, trTitle, DT_LEFT | DT_SINGLELINE, g_fBold, cTitle);
    
    RECT trSub = { x + 48, y + 29, r.right - 8, y + 50 };
    DrawTextC(g_ui.hdc, subtitle, trSub, DT_LEFT | DT_SINGLELINE, g_fNorm, cSub);

    return clicked;
}

#endif /* PE_IMGUI_IMPLEMENTATION */
