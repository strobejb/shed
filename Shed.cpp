#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include "resource.h"
#include "..\CardLib\CardLib.h"
#include "..\catch22lib\trace.h"

#include "Shed.h"

TCHAR szHelpPath[MAX_PATH];

DWORD dwAppStartTime;
HWND hwndMain;
HWND hwndStatus;
HINSTANCE hInstance;

TCHAR szAppName[] = _T("Shed");
TCHAR szWinFmt[] =  _T("WINS: Computer [ %d ]   You [ %d ]");

LRESULT CALLBACK WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

void SetStatusBarText(HWND hwndSB, int part, unsigned style, char *fmt, ...)
{
	char tmpbuf[128];
	
	if(part < 0 || part >= 4) return;
	
	va_list argp;
	va_start(argp, fmt);
	
	wvsprintf(tmpbuf, fmt, argp);
	va_end(argp);

	if(style == 1) style = SBT_NOBORDERS;
	else		   style = 0;

	SendMessage(hwndSB, SB_SETTEXT, (WPARAM)part | style, (LPARAM)(LPSTR)tmpbuf);
}

void MakePath(TCHAR *szDest, UINT nDestLen, TCHAR *szExt)
{
	TCHAR *ptr;
	
	ptr = szDest + GetModuleFileName(GetModuleHandle(0), szDest, nDestLen) - 1;
	while(*ptr-- != '.');
	lstrcpy(ptr + 1, szExt);
}

//
//	Main entry point
//
int WINAPI WinMain (HINSTANCE hInst, HINSTANCE hPrev, PSTR szCmdLine, int iCmdShow)
{
	HWND		hwnd;
	MSG			msg;
	WNDCLASSEX	wndclass;
	INITCOMMONCONTROLSEX ice;
	HACCEL		hAccelTable;			

	hInstance = hInst;

	//Window class for the main application parent window
	wndclass.cbSize			= sizeof(wndclass);
	wndclass.style			= 0;//CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc	= WndProc;
	wndclass.cbClsExtra		= 0;
	wndclass.cbWndExtra		= 0;
	wndclass.hInstance		= hInst;
	wndclass.hIcon			= LoadIcon (hInst, MAKEINTRESOURCE(IDI_ICON1));
	wndclass.hCursor		= LoadCursor (NULL, IDC_ARROW);
	wndclass.hbrBackground	= (HBRUSH)NULL;
	wndclass.lpszMenuName	= MAKEINTRESOURCE(IDR_MENU1);
	wndclass.lpszClassName	= szAppName;
	wndclass.hIconSm		= (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 16, 16, 0);

	RegisterClassEx(&wndclass);

	ice.dwSize = sizeof(ice);
	ice.dwICC = ICC_BAR_CLASSES;
	InitCommonControlsEx(&ice);

	srand((unsigned)GetTickCount());//timeGetTime());

	LoadSettings();

	//Construct the path to our help file
	MakePath(szHelpPath, MAX_PATH, _T(".hlp"));
	
	hwnd = CreateWindow(szAppName,		// window class name
				szAppName,				// window caption
				WS_OVERLAPPEDWINDOW
				,//|WS_CLIPCHILDREN,	// window style
				CW_USEDEFAULT,			// initial x position
				CW_USEDEFAULT,			// initial y position
				CW_USEDEFAULT,			// initial x size
				CW_USEDEFAULT,			// initial y size
				NULL,					// parent window handle
				NULL,					// use window class menu
				hInst,					// program instance handle
				NULL);					// creation parameters

	hwndMain = hwnd;

	ShowWindow(hwnd, iCmdShow);
	UpdateWindow(hwnd);

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));

	while(GetMessage(&msg, NULL,0,0))
	{
		if(!TranslateAccelerator(hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	SaveSettings();

	return msg.wParam;
}

void MenuCheckMark(HMENU hmenu, UINT uCmdId, BOOL fChecked)
{
	if(fChecked)
		CheckMenuItem(hmenu, uCmdId, MF_CHECKED | MF_BYCOMMAND);
	else
		CheckMenuItem(hmenu, uCmdId, MF_UNCHECKED | MF_BYCOMMAND);
}

//-----------------------------------------------------------------------------
LRESULT CALLBACK WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	static int nWidth, nHeight;
	int nStatusHeight = 0;//20;
	int parts[] = { 100, -1 };
	UINT ret;

	MINMAXINFO *mmi;

	switch(iMsg)
	{
	case WM_CREATE:
		hwndStatus = CreateStatusWindow(WS_CHILD | WS_VISIBLE | CCS_BOTTOM | SBARS_SIZEGRIP, "Ready", hwnd, 0);
		
		//SendMessage(hwndStatus, SB_SIMPLE, (WPARAM)TRUE, 0);

		SendMessage(hwndStatus, SB_SETPARTS, 2, (LPARAM)parts); 
		SendMessage(hwndStatus, SB_SETTEXT, 0 | SBT_NOBORDERS, (LPARAM)"");

		ShowWindow(hwndStatus, SW_HIDE);

		ShedBoard.Create(hwnd, WS_EX_CLIENTEDGE, WS_CHILD|WS_VISIBLE, 0, 0, 0, 0);

		CreateShed(ShedBoard);

		dwAppStartTime = GetTickCount();

		SetTimer(hwnd, SHED_CLOCK_TIMER, 1000, 0);

		//
		//	Create an event so we can interrupt the computer's choose
		//  thread when he is setting up his cards
		//
		hCompChooseEvent = CreateEvent(0, FALSE, FALSE, 0);

		MenuCheckMark(GetMenu(hwnd), IDM_GAME_EASY,		nSkillLevel == SKILL_EASY);
		MenuCheckMark(GetMenu(hwnd), IDM_GAME_NORMAL,   nSkillLevel == SKILL_NORMAL);
		return 0;

	case WM_DESTROY:
		DeleteObject(hCompChooseEvent);
		PostQuitMessage(0);
		return 0;

	case WM_SIZE:
		nWidth  = LOWORD(lParam);
		nHeight = HIWORD(lParam);

		MoveWindow(ShedBoard, 0, 0, nWidth, nHeight-nStatusHeight, TRUE);
		MoveWindow(hwndStatus, 0, nHeight-nStatusHeight, nWidth, nHeight, TRUE);
		parts[0] = nWidth - 256;
		SendMessage(hwndStatus, SB_SETPARTS, 2, (LPARAM)parts); 

		//size the help window if we are minimizing etc
		if(wParam == SIZE_MINIMIZED)
		{
			/*BYTE mem[512];
			HELPWININFO *phwi = (HELPWININFO *)mem;
			phwi->wStructSize = sizeof HELPWININFO + lstrlen(_T("main"));
			phwi->wMax = SW_MINIMIZE; //SW_SHOWNORMAL;
			lstrcpy(phwi->rgchMember, _T("main"));
			LONG l = WinHelp(hwnd, szHelpPath, HELP_SETWINPOS, (DWORD)phwi);
			TRACE("Minimize\n");*/
		}
		else if(wParam == SIZE_RESTORED)
		{
			/*HELPWININFO hwi;
			hwi.wStructSize = sizeof hwi ;
			hwi.wMax = SW_RESTORE;//SHOWNORMAL;
			WinHelp(hwnd, szHelpPath, HELP_SETWINPOS, (DWORD)&hwi);
			TRACE("Restore\n");*/
		}
		
		return 0;

	case WM_TIMER:

		if(wParam == SHED_CLEAR_STACK_TIMER)
		{
			KillTimer(hwnd, wParam);
			AutoClearPickup(0);
		}
		
		if(wParam == SHED_CLOCK_TIMER)
		{
			DWORD dwAppCurTime =  (GetTickCount() - dwAppStartTime) / 1000;
			char ach[80];
			
			wsprintf(ach, "%02d:%02d:%02d", (dwAppCurTime / (60*60)) % 12, (dwAppCurTime / 60) % 60, dwAppCurTime % 60);
			//SendMessage(hwndStatus, SB_SETTEXT, 1, (LPARAM)ach);
		}
		return 0;

	case WM_GETMINMAXINFO:
		mmi = (MINMAXINFO *)lParam;		
		mmi->ptMinTrackSize.x = 580;
		mmi->ptMinTrackSize.y = 440;

		return 0;

	case WM_COMMAND:
	
		switch(LOWORD(wParam))
		{
		case IDM_GAME_NEW:
			//simulate a button click on the new button..
			NewGameButtonProc(*pNewButton);
			return 0;

		case IDM_GAME_DECK:
			ShowDeckOptionsDlg(hwnd);
			return 0;

		case IDM_GAME_OPTIONS:
			ShowGameOptionsDlg(hwnd);
			return 0;

		case IDM_HELP_CONTENTS:

			WinHelp(hwnd, szHelpPath, HELP_CONTENTS, 0);//HELP_KEY, (DWORD)"How to play");
			
			return 0;

		case IDM_HELP_ABOUT:
			ShowAboutDlg(hwnd);
			return 0;

		case IDM_GAME_EXIT:
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			return 0;

		case IDM_GAME_EASY:
			MenuCheckMark(GetMenu(hwnd), IDM_GAME_EASY,		TRUE);
			MenuCheckMark(GetMenu(hwnd), IDM_GAME_NORMAL,	FALSE);
			nSkillLevel = SKILL_EASY;
			break;

		case IDM_GAME_NORMAL:
			MenuCheckMark(GetMenu(hwnd), IDM_GAME_EASY,		FALSE);
			MenuCheckMark(GetMenu(hwnd), IDM_GAME_NORMAL,	TRUE);
			nSkillLevel = SKILL_NORMAL;
			break;
		}

		return 0;

	case WM_CLOSE:
		
		ret = IDOK;

		if(fGameStarted)
		{
			ret = MessageBox(hwnd, _T("Quit the current game?"), szAppName, MB_OKCANCEL|MB_ICONQUESTION);
		}

		if(ret == IDOK)
		{
			WinHelp(hwnd, szHelpPath, HELP_QUIT, 0);
			DestroyWindow(hwnd);
		}

		return 0;
	}

	return DefWindowProc (hwnd, iMsg, wParam, lParam);
}

