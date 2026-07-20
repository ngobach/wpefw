#ifndef WIDGETS_H
#define WIDGETS_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Extern font and color resources from main.c / pe_imgui.h
extern HFONT g_fWidgetTime;
extern HFONT g_fWidgetDesc;
extern HFONT g_fMDL2;
extern COLORREF g_accent;
extern void *g_bits;
extern int g_w;
extern int g_h;

static double UI_HoverAnimation(unsigned int id, int hovered);

static void DrawWallclockWidget(HDC hdc, int x, int y, int w, int h) {
    int id = 900;
    int drawY = y + g_ui.dy;
    int hovered = (g_ui.mx >= x && g_ui.mx < x + w && g_ui.my >= drawY && g_ui.my < drawY + h);
    
    if (hovered) {
        g_ui.hot = id;
    }
    
    double hover = UI_HoverAnimation(id, g_ui.hot == id);
    
    COLORREF bgCol = Mix(RGB(28, 28, 34), RGB(38, 38, 45), hover);
    FillRoundAA(g_bits, g_w, g_h, x, drawY, w, h, 8, bgCol);
    
    // Draw left accent bar
    FillRoundAA(g_bits, g_w, g_h, x + 2, drawY + 8, 4, h - 16, 2, g_accent);
    
    // Get local time
    SYSTEMTIME st;
    GetLocalTime(&st);
    
    // Format time
    char timeStr[16];
    wsprintfA(timeStr, "%02d:%02d", st.wHour, st.wMinute);
    
    char secStr[8];
    wsprintfA(secStr, ":%02d", st.wSecond);
    
    // Format date
    static const char *DAYS[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
    static const char *MONTHS[] = { "", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    
    char dateStr[64];
    wsprintfA(dateStr, "%s, %s %d, %d", DAYS[st.wDayOfWeek], MONTHS[st.wMonth], st.wDay, st.wYear);
    
    // Draw clock glyph on the right side (U+E823)
    DrawGlyph(hdc, 0xE823, x + w - 28, drawY + h / 2, Mix(RGB(70, 70, 80), g_accent, 0.3 + hover * 0.2), g_fMDL2);
    
    // Draw time
    SIZE sz;
    HGDIOBJ oldFont = SelectObject(hdc, g_fWidgetTime);
    GetTextExtentPoint32A(hdc, timeStr, (int)strlen(timeStr), &sz);
    SelectObject(hdc, oldFont);
    
    RECT rTime = { x + 16, drawY + 6, x + 16 + sz.cx, drawY + 38 };
    DrawTextC(hdc, timeStr, rTime, DT_LEFT | DT_TOP | DT_SINGLELINE, g_fWidgetTime, RGB(255, 255, 255));
    
    // Draw seconds
    RECT rSec = { x + 16 + sz.cx + 2, drawY + 20, x + w - 48, drawY + 38 };
    DrawTextC(hdc, secStr, rSec, DT_LEFT | DT_TOP | DT_SINGLELINE, g_fWidgetDesc, RGB(160, 160, 170));
    
    // Draw date
    RECT rDate = { x + 16, drawY + 38, x + w - 48, drawY + 54 };
    DrawTextC(hdc, dateStr, rDate, DT_LEFT | DT_TOP | DT_SINGLELINE, g_fWidgetDesc, RGB(160, 160, 170));
}

#endif // WIDGETS_H
