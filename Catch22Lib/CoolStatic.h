#ifndef CATCH22_COOLSTATIC_H
#define CATCH22_COOLSTATIC_H

#include <windows.h>
#include <tchar.h>

#ifdef __cplusplus
extern "C" {
#endif

void MakeStaticLine(HWND hwndParent, int controlId);
void StaticToURLControl(HWND hwndParent, int controlId, LPCTSTR text, COLORREF colour);
void CenterWindow(HWND hwnd);

#ifdef __cplusplus
}
#endif

#endif
