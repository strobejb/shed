#include <windows.h>
#include <tchar.h>
#include "resource.h"
#include "..\Catch22Lib\CoolStatic.h"

BOOL CALLBACK AboutDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:

		MakeStaticLine(hwnd, IDC_STATICLINE);
		//MakeStaticLine(hwnd, IDC_STATICLINE2);

		StaticToURLControl(hwnd, IDC_URL, "www.catch22.uk.net", RGB(0,0,255));
		//StaticToURLControl(hwnd, IDC_UPX, "http://upx.sourceforge.net/", RGB(0,0,0));

		//CenterWindow(hwnd);

		return 0;

	case WM_CLOSE:
		EndDialog(hwnd, 0);
		return TRUE;

	case WM_COMMAND:
		switch(wParam)
		{
		case IDOK:
			EndDialog(hwnd, 0);
			return 0;
		}

		return 0;
	}

	return 0;
}

void ShowAboutDlg(HWND hwndParent)
{
	DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_DIALOG4), hwndParent, AboutDlgProc);
}
