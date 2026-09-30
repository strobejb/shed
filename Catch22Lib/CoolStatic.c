#include "CoolStatic.h"

void MakeStaticLine(HWND hwndParent, int controlId)
{
    HWND hwndStatic = GetDlgItem(hwndParent, controlId);
    LONG_PTR style;

    if (hwndStatic == NULL)
    {
        return;
    }

    style = GetWindowLongPtr(hwndStatic, GWL_STYLE);
    style &= ~(SS_TYPEMASK);
    style |= SS_ETCHEDHORZ;
    SetWindowLongPtr(hwndStatic, GWL_STYLE, style);
    InvalidateRect(hwndStatic, NULL, TRUE);
}

void StaticToURLControl(HWND hwndParent, int controlId, LPCTSTR text, COLORREF colour)
{
    HWND hwndStatic = GetDlgItem(hwndParent, controlId);
    HFONT hFont;
    LONG_PTR style;

    UNREFERENCED_PARAMETER(colour);

    if (hwndStatic == NULL)
    {
        return;
    }

    SetWindowText(hwndStatic, text);
    style = GetWindowLongPtr(hwndStatic, GWL_STYLE);
    SetWindowLongPtr(hwndStatic, GWL_STYLE, style | SS_NOTIFY);

    hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    if (hFont != NULL)
    {
        SendMessage(hwndStatic, WM_SETFONT, (WPARAM)hFont, TRUE);
    }
}

void CenterWindow(HWND hwnd)
{
    HWND hwndParent = GetParent(hwnd);
    RECT rcWindow;
    RECT rcParent;
    int width;
    int height;
    int x;
    int y;

    if (hwndParent == NULL)
    {
        hwndParent = GetDesktopWindow();
    }

    GetWindowRect(hwnd, &rcWindow);
    GetWindowRect(hwndParent, &rcParent);

    width = rcWindow.right - rcWindow.left;
    height = rcWindow.bottom - rcWindow.top;
    x = rcParent.left + ((rcParent.right - rcParent.left) - width) / 2;
    y = rcParent.top + ((rcParent.bottom - rcParent.top) - height) / 2;

    SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}
