#define NOMINMAX
#include "cards.h"

#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

namespace
{
const int kCardWidth = 71;
const int kCardHeight = 96;
ULONG_PTR g_gdiplusToken = 0;
int g_initCount = 0;

Color ToColor(COLORREF cr)
{
    return Color(255, GetRValue(cr), GetGValue(cr), GetBValue(cr));
}

bool EnsureGdiplus()
{
    if (g_gdiplusToken != 0)
    {
        return true;
    }

    GdiplusStartupInput input;
    return GdiplusStartup(&g_gdiplusToken, &input, NULL) == Ok;
}

void AddRoundedRect(GraphicsPath &path, float x, float y, float w, float h, float r)
{
    const float d = r * 2.0f;

    path.AddArc(x, y, d, d, 180.0f, 90.0f);
    path.AddArc(x + w - d, y, d, d, 270.0f, 90.0f);
    path.AddArc(x + w - d, y + h - d, d, d, 0.0f, 90.0f);
    path.AddArc(x, y + h - d, d, d, 90.0f, 90.0f);
    path.CloseFigure();
}

void DrawSuit(Graphics &g, int suit, float cx, float cy, float size, Brush &brush)
{
    const float s = size;
    GraphicsPath path;

    switch (suit)
    {
    case ecsHEARTS:
        path.AddBezier(cx, cy + s * 0.42f, cx - s * 0.75f, cy - s * 0.05f, cx - s * 0.50f, cy - s * 0.62f, cx, cy - s * 0.30f);
        path.AddBezier(cx, cy - s * 0.30f, cx + s * 0.50f, cy - s * 0.62f, cx + s * 0.75f, cy - s * 0.05f, cx, cy + s * 0.42f);
        path.CloseFigure();
        g.FillPath(&brush, &path);
        break;

    case ecsDIAMONDS:
    {
        PointF pts[] = {
            PointF(cx, cy - s * 0.62f),
            PointF(cx + s * 0.48f, cy),
            PointF(cx, cy + s * 0.62f),
            PointF(cx - s * 0.48f, cy),
        };
        g.FillPolygon(&brush, pts, 4);
        break;
    }

    case ecsSPADES:
        path.AddBezier(cx, cy - s * 0.56f, cx - s * 0.76f, cy - s * 0.03f, cx - s * 0.45f, cy + s * 0.42f, cx, cy + s * 0.18f);
        path.AddBezier(cx, cy + s * 0.18f, cx + s * 0.45f, cy + s * 0.42f, cx + s * 0.76f, cy - s * 0.03f, cx, cy - s * 0.56f);
        path.CloseFigure();
        g.FillPath(&brush, &path);
        {
            PointF stem[] = {
                PointF(cx, cy + s * 0.02f),
                PointF(cx + s * 0.23f, cy + s * 0.58f),
                PointF(cx - s * 0.23f, cy + s * 0.58f),
            };
            g.FillPolygon(&brush, stem, 3);
        }
        break;

    default:
        g.FillEllipse(&brush, cx - s * 0.28f, cy - s * 0.58f, s * 0.56f, s * 0.56f);
        g.FillEllipse(&brush, cx - s * 0.58f, cy - s * 0.08f, s * 0.56f, s * 0.56f);
        g.FillEllipse(&brush, cx + s * 0.02f, cy - s * 0.08f, s * 0.56f, s * 0.56f);
        {
            PointF stem[] = {
                PointF(cx, cy + s * 0.10f),
                PointF(cx + s * 0.20f, cy + s * 0.58f),
                PointF(cx - s * 0.20f, cy + s * 0.58f),
            };
            g.FillPolygon(&brush, stem, 3);
        }
        break;
    }
}

void DrawFrame(Graphics &g, float w, float h, bool face)
{
    GraphicsPath outer;
    SolidBrush fill(face ? Color(255, 252, 252, 248) : Color(255, 238, 241, 247));
    Pen border(Color(255, 25, 25, 25), 1.0f);

    AddRoundedRect(outer, 0.5f, 0.5f, w - 1.0f, h - 1.0f, 7.0f);
    g.FillPath(&fill, &outer);
    g.DrawPath(&border, &outer);
}

const WCHAR *RankText(int rank)
{
    static const WCHAR *kRanks[] = {
        L"", L"A", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9", L"10", L"J", L"Q", L"K",
    };
    return kRanks[rank < 1 || rank > 13 ? 1 : rank];
}

void DrawCorner(Graphics &g, int rank, int suit, float x, float y, bool rotate)
{
    const float cornerW = 20.0f;
    const float cornerH = 30.0f;
    const float rankH = 13.0f;
    const float suitY = 20.0f;

    GraphicsState state = g.Save();
    g.TranslateTransform(x, y);
    if (rotate)
    {
        g.TranslateTransform(cornerW * 0.5f, cornerH * 0.5f);
        g.RotateTransform(180.0f);
        g.TranslateTransform(-cornerW * 0.5f, -cornerH * 0.5f);
    }

    SolidBrush brush((suit == ecsDIAMONDS || suit == ecsHEARTS) ? Color(255, 176, 28, 42) : Color(255, 22, 28, 34));
    FontFamily family(L"Arial Narrow");
    REAL fontSize = 12.0f;
    StringFormat format;
    format.SetAlignment(StringAlignmentCenter);
    format.SetLineAlignment(StringAlignmentCenter);
    format.SetFormatFlags(StringFormatFlagsNoWrap | StringFormatFlagsNoClip);

    RectF rankRect(0.0f, 0.0f, cornerW, rankH);
    RectF measured;

    for (;;)
    {
        Font rankFont(&family, fontSize, FontStyleBold, UnitPixel);
        g.MeasureString(RankText(rank), -1, &rankFont, PointF(0.0f, 0.0f), &format, &measured);
        if (measured.Width <= rankRect.Width || fontSize <= 10.0f)
        {
            g.DrawString(RankText(rank), -1, &rankFont, rankRect, &format, &brush);
            break;
        }

        fontSize -= 0.25f;
    }

    DrawSuit(g, suit, cornerW * 0.5f, suitY, 6.6f, brush);
    g.Restore(state);
}

void DrawPips(Graphics &g, int rank, int suit, float w, float h)
{
    SolidBrush brush((suit == ecsDIAMONDS || suit == ecsHEARTS) ? Color(255, 180, 23, 38) : Color(255, 20, 26, 32));

    struct Pip
    {
        float x;
        float y;
        bool flip;
    };

    Pip pips[10];
    int count = 0;
    const float lx = w * 0.32f;
    const float rx = w * 0.68f;
    const float cx = w * 0.50f;
    const float y1 = h * 0.18f;
    const float y2 = h * 0.34f;
    const float y3 = h * 0.50f;
    const float y4 = h * 0.66f;
    const float y5 = h * 0.82f;
    float size = rank == 1 ? 22.0f : 12.6f;

    switch (rank)
    {
    case 1:
        pips[count++] = {cx, y3, false};
        break;
    case 2:
        pips[count++] = {cx, y1, false};
        pips[count++] = {cx, y5, true};
        break;
    case 3:
        pips[count++] = {cx, y1, false};
        pips[count++] = {cx, y3, false};
        pips[count++] = {cx, y5, true};
        break;
    case 4:
    case 5:
        pips[count++] = {lx, y1, false};
        pips[count++] = {rx, y1, false};
        if (rank == 5)
            pips[count++] = {cx, y3, false};
        pips[count++] = {lx, y5, true};
        pips[count++] = {rx, y5, true};
        break;
    case 6:
    case 7:
    case 8:
        pips[count++] = {lx, y1, false};
        pips[count++] = {rx, y1, false};
        pips[count++] = {lx, y3, false};
        pips[count++] = {rx, y3, false};
        pips[count++] = {lx, y5, true};
        pips[count++] = {rx, y5, true};
        if (rank >= 7)
            pips[count++] = {cx, y2, false};
        if (rank >= 8)
            pips[count++] = {cx, y4, true};
        break;
    case 9:
        pips[count++] = {lx, y1, false};
        pips[count++] = {rx, y1, false};
        pips[count++] = {lx, h * 0.335f, false};
        pips[count++] = {rx, h * 0.335f, false};
        pips[count++] = {cx, y3, false};
        pips[count++] = {lx, h * 0.665f, true};
        pips[count++] = {rx, h * 0.665f, true};
        pips[count++] = {lx, y5, true};
        pips[count++] = {rx, y5, true};
        size = suit == ecsDIAMONDS ? 11.2f : 13.4f;
        break;
    case 10:
        pips[count++] = {lx, h * 0.17f, false};
        pips[count++] = {rx, h * 0.17f, false};
        pips[count++] = {cx, h * 0.31f, false};
        pips[count++] = {lx, h * 0.415f, false};
        pips[count++] = {rx, h * 0.415f, false};
        pips[count++] = {lx, h * 0.585f, true};
        pips[count++] = {rx, h * 0.585f, true};
        pips[count++] = {cx, h * 0.69f, true};
        pips[count++] = {lx, h * 0.83f, true};
        pips[count++] = {rx, h * 0.83f, true};
        size = suit == ecsDIAMONDS ? 10.8f : 12.4f;
        break;
    }

    for (int i = 0; i < count; ++i)
    {
        GraphicsState state = g.Save();
        if (pips[i].flip)
        {
            g.TranslateTransform(pips[i].x, pips[i].y);
            g.RotateTransform(180.0f);
            g.TranslateTransform(-pips[i].x, -pips[i].y);
        }
        DrawSuit(g, suit, pips[i].x, pips[i].y, size, brush);
        g.Restore(state);
    }
}

void DrawCourt(Graphics &g, int rank, int suit, float w, float h)
{
    SolidBrush ink((suit == ecsDIAMONDS || suit == ecsHEARTS) ? Color(255, 176, 28, 42) : Color(255, 22, 28, 34));
    SolidBrush panel(Color(255, 244, 238, 218));
    Pen line(Color(255, 90, 90, 86), 1.0f);
    GraphicsPath panelPath;
    AddRoundedRect(panelPath, w * 0.27f, h * 0.27f, w * 0.46f, h * 0.46f, 4.0f);

    g.FillPath(&panel, &panelPath);
    g.DrawPath(&line, &panelPath);

    FontFamily family(L"Georgia");
    Font font(&family, 25.0f, FontStyleBold, UnitPixel);
    StringFormat format;
    format.SetAlignment(StringAlignmentCenter);
    format.SetLineAlignment(StringAlignmentCenter);
    RectF textRect(w * 0.28f, h * 0.29f, w * 0.44f, h * 0.24f);

    g.DrawString(RankText(rank), -1, &font, textRect, &format, &ink);
    DrawSuit(g, suit, w * 0.50f, h * 0.62f, 15.0f, ink);
}

void DrawFace(Graphics &g, int card, float w, float h)
{
    if (card < 0 || card > 51)
        card = 0;

    const int rank = (card / 4) + 1;
    const int suit = card % 4;

    DrawFrame(g, w, h, true);
    DrawCorner(g, rank, suit, 1.0f, 4.0f, false);
    DrawCorner(g, rank, suit, w - 21.0f, h - 34.0f, true);

    if (rank <= 10)
        DrawPips(g, rank, suit, w, h);
    else
        DrawCourt(g, rank, suit, w, h);
}

Color BackColor(int cardback, DWORD color)
{
    static const Color colors[] = {
        Color(255, 40, 104, 154), Color(255, 72, 89, 162), Color(255, 36, 128, 96), Color(255, 112, 72, 148),
        Color(255, 168, 74, 58),  Color(255, 56, 124, 142), Color(255, 104, 112, 52), Color(255, 128, 62, 98),
        Color(255, 65, 92, 150),  Color(255, 42, 124, 72),  Color(255, 134, 85, 48), Color(255, 46, 104, 130),
        Color(255, 112, 68, 128), Color(255, 92, 92, 92),   Color(255, 42, 42, 42),   Color(255, 235, 238, 246),
    };

    if (cardback == ecbCROSSHATCH && color != 0)
        return ToColor((COLORREF)color);

    int index = cardback - ecbCROSSHATCH;
    if (index < 0 || index >= 16)
        index = 0;

    return colors[index];
}

void DrawBackPattern(Graphics &g, int cardback, float w, float h, const Color &base)
{
    SolidBrush baseBrush(base);
    Pen light(Color(160, 255, 255, 255), 1.0f);
    Pen dark(Color(100, 0, 0, 0), 1.0f);
    GraphicsPath inner;

    AddRoundedRect(inner, 7.0f, 7.0f, w - 14.0f, h - 14.0f, 5.0f);
    g.FillPath(&baseBrush, &inner);

    const int style = cardback - ecbCROSSHATCH;
    if (style == 14)
    {
        g.DrawLine(&light, 18.0f, 18.0f, w - 18.0f, h - 18.0f);
        g.DrawLine(&light, w - 18.0f, 18.0f, 18.0f, h - 18.0f);
        return;
    }
    if (style == 15)
    {
        g.DrawEllipse(&light, 18.0f, 28.0f, w - 36.0f, h - 56.0f);
        g.DrawEllipse(&dark, 21.0f, 31.0f, w - 42.0f, h - 62.0f);
        return;
    }

    for (float x = -h; x < w; x += 9.0f)
        g.DrawLine(&light, x, h - 7.0f, x + h, 7.0f);

    if ((style % 3) == 0)
    {
        for (float x = 7.0f; x < w; x += 8.0f)
            g.DrawLine(&dark, x, 8.0f, x, h - 8.0f);
    }
    else if ((style % 3) == 1)
    {
        SolidBrush mark(Color(120, 255, 255, 255));
        for (float y = 22.0f; y < h - 10.0f; y += 20.0f)
            DrawSuit(g, style % 4, w * 0.50f, y, 8.0f, mark);
    }
    else
    {
        g.DrawEllipse(&light, 18.0f, 24.0f, w - 36.0f, h - 48.0f);
        g.DrawRectangle(&dark, 23.0f, 29.0f, w - 46.0f, h - 58.0f);
    }
}

void DrawBack(Graphics &g, int cardback, DWORD color, float w, float h)
{
    DrawFrame(g, w, h, false);
    Color base = BackColor(cardback, color);
    DrawBackPattern(g, cardback, w, h, base);
}
}

BOOL WINAPI cdtInit(int *width, int *height)
{
    if (!EnsureGdiplus())
        return FALSE;

    ++g_initCount;

    if (width)
        *width = kCardWidth;
    if (height)
        *height = kCardHeight;

    return TRUE;
}

BOOL WINAPI cdtDraw(HDC hdc, int x, int y, int card, int type, DWORD color)
{
    return cdtDrawExt(hdc, x, y, kCardWidth, kCardHeight, card, type, color);
}

BOOL WINAPI cdtDrawExt(HDC hdc, int x, int y, int dx, int dy, int card, int type, DWORD color)
{
    if (hdc == NULL || dx <= 0 || dy <= 0 || !EnsureGdiplus())
        return FALSE;

    int savedDc = SaveDC(hdc);
    IntersectClipRect(hdc, x, y, x + dx, y + dy);

    {
        Graphics g(hdc);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
        g.TranslateTransform((REAL)x, (REAL)y);
        g.ScaleTransform((REAL)dx / (REAL)kCardWidth, (REAL)dy / (REAL)kCardHeight);

        if (type == ectBACKS)
            DrawBack(g, card, color, (REAL)kCardWidth, (REAL)kCardHeight);
        else
            DrawFace(g, card, (REAL)kCardWidth, (REAL)kCardHeight);

        g.Flush();
    }

    if (type == ectINVERT)
        PatBlt(hdc, x, y, dx, dy, DSTINVERT);

    RestoreDC(hdc, savedDc);
    return TRUE;
}

BOOL WINAPI cdtAnimate(HDC hdc, int cardback, int x, int y, int frame)
{
    UNREFERENCED_PARAMETER(hdc);
    UNREFERENCED_PARAMETER(cardback);
    UNREFERENCED_PARAMETER(x);
    UNREFERENCED_PARAMETER(y);
    UNREFERENCED_PARAMETER(frame);
    return FALSE;
}

void WINAPI cdtTerm(void)
{
    if (g_initCount > 0)
        --g_initCount;

    if (g_initCount == 0 && g_gdiplusToken != 0)
    {
        GdiplusShutdown(g_gdiplusToken);
        g_gdiplusToken = 0;
    }
}
