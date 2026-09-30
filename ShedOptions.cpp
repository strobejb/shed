#include <windows.h>
#include <tchar.h>
#include "resource.h"
#include "..\Catch22Lib\RegHelper.h"
#include "..\CardLib\CardLib.h"
#include "Shed.h"
//#include "Shed.h"

#define MAX_USER_NAME 40

static TCHAR szRegLoc[] = _T("Software\\Catch22\\Shed");

extern COLORREF crCustColours[];


//
//	Shed persistent settings
//
COLORREF g_crBackColour;
int      g_nBackCardIdx = 53;
TCHAR    szUserName[MAX_USER_NAME] = _T("Player 1");

void LoadSettings()
{
	HKEY hkey;

	RegCreateKeyEx(HKEY_CURRENT_USER, szRegLoc, 0, _T(""), 0, KEY_READ, NULL, &hkey, NULL);

	g_crBackColour = GetSettingInt(hkey, _T("BackColour"), RGB(0, 64, 100));
	g_nBackCardIdx = GetSettingInt(hkey, _T("BackCardIdx"), 53);
	nSkillLevel    = GetSettingInt(hkey, _T("SkillLevel"), SKILL_NORMAL);

	GetSettingBinary(hkey, _T("CustColours"), crCustColours, sizeof(COLORREF) * 16);

	GetSettingStr(hkey, _T("PlayerName"), szUserName, szUserName, MAX_USER_NAME);

	RegCloseKey(hkey);
}

void SaveSettings()
{
	HKEY hkey;

	RegCreateKeyEx(HKEY_CURRENT_USER, szRegLoc, 0, _T(""), 0, KEY_WRITE, NULL, &hkey, NULL);

	WriteSettingInt(hkey, _T("BackColour"), g_crBackColour);
	WriteSettingInt(hkey, _T("BackCardIdx"), g_nBackCardIdx);
	WriteSettingStr(hkey, _T("PlayerName"), szUserName);
	WriteSettingInt(hkey, _T("SkillLevel"), nSkillLevel);
	
	WriteSettingBinary(hkey, _T("CustColours"), crCustColours, sizeof(COLORREF) * 16);

	RegCloseKey(hkey);
}

BOOL CALLBACK GameOptionsProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_INITDIALOG:

		SetDlgItemText(hwnd, IDC_EDIT2, szUserName);
		
		CheckDlgButton(hwnd, IDC_CHECK2, fSevensSeeThrough ? TRUE : FALSE);

		return TRUE;

	case WM_COMMAND:

		switch(LOWORD(wParam))
		{
		case IDCANCEL:
			EndDialog(hwnd, 0);
			return TRUE;

		case IDOK:
			GetDlgItemText(hwnd, IDC_EDIT2, szUserName, 40);

			fSevensSeeThrough = IsDlgButtonChecked(hwnd, IDC_CHECK2) ? true : false;

			EndDialog(hwnd, TRUE);
			return TRUE;

		}
		return FALSE;
		

	case WM_CLOSE:
		EndDialog(hwnd, 0);
		return TRUE;
	}

	return FALSE;
}

void ShowGameOptionsDlg(HWND hwndParent)
{
	if(DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_DIALOG3), hwndParent, GameOptionsProc))
	{
		pUserNameLabel->SetText(_T("%s: "), szUserName);
		pUserNameLabel->Redraw();
	}
}

