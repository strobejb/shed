#include <windows.h>
#include <tchar.h>
#include "..\CardLib\CardLib.h"
#include "Shed.h"
#include "resource.h"

#ifndef ODS_NOFOCUSRECT
#define ODS_NOFOCUSRECT     0x0200
#endif

#define SEL_BORDER			3

COLORREF crCustColours[16];

void RightAlign(HWND hwndCtrl, int border)
{
	HWND hwndParent = GetParent(hwndCtrl);

	RECT rect1, rect2;

	border -= GetSystemMetrics(SM_CXDLGFRAME);

	GetClientRect(hwndParent, &rect1);
	ClientToScreen(hwndParent, (POINT *)&rect1.left);
	ClientToScreen(hwndParent, (POINT *)&rect1.right);

	//GetWindowRect(hwndParent, &rect1);
	GetWindowRect(hwndCtrl, &rect2);

	OffsetRect(&rect2, -rect1.left, -rect1.top);
	OffsetRect(&rect1, -rect1.left, -rect1.top);

	SetWindowPos(hwndCtrl, 0, 
		rect1.right - (rect2.right - rect2.left) - border,
		rect2.top - rect1.top,
		0, 0, 
		SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
}

BOOL DrawCardList(DRAWITEMSTRUCT *dis)
{
	switch(dis->itemAction)
	{
	case ODA_FOCUS:
		
		//Windows 2000 doesn't display a control's focus in 
		//a dialog box until the ALT key is pressed.
		if(!(dis->itemState & ODS_NOFOCUSRECT))
			DrawFocusRect(dis->hDC, &dis->rcItem);
		
		break;
		
	case ODA_SELECT:
		
	case ODA_DRAWENTIRE:
		
		InflateRect(&dis->rcItem, -1, -1);
		
		if(dis->itemState & ODS_SELECTED)
		{
			FillRect(dis->hDC, &dis->rcItem, (HBRUSH)GetSysColorBrush(COLOR_HIGHLIGHT));
		}
		else
		{
			FillRect(dis->hDC, &dis->rcItem, (HBRUSH)GetSysColorBrush(COLOR_WINDOW));
		}
		
		InflateRect(&dis->rcItem, 1, 1);
		
		CardBlt(dis->hDC, 
			dis->rcItem.left + SEL_BORDER + 1, 
			dis->rcItem.top + SEL_BORDER + 1, 
			(UINT)dis->itemData);
		
		break;
	}
	
	
	return TRUE;
	
}

BOOL DrawColourButton(DRAWITEMSTRUCT *dis, COLORREF col)
{
	RECT rect;
	COLORREF oldcol;

	switch(dis->itemAction)
	{
	case ODA_FOCUS:

		//if(!(dis->itemState & ODS_NOFOCUSRECT))
		{
			CopyRect(&rect, &dis->rcItem);
			InflateRect(&rect, -5, -5);
			DrawFocusRect(dis->hDC, &rect);
		}
		
		break;

	case ODA_SELECT:

	case ODA_DRAWENTIRE:
	
		CopyRect(&rect, &dis->rcItem);
		InflateRect(&rect, -6, -6);

		if(dis->itemState & ODS_SELECTED)
		{
			DrawFrameControl(dis->hDC, &dis->rcItem, DFC_BUTTON, DFCS_BUTTONPUSH | DFCS_PUSHED);
			OffsetRect(&rect, 1, 1);
		}
		else
		{
			DrawFrameControl(dis->hDC, &dis->rcItem, DFC_BUTTON, DFCS_BUTTONPUSH);
		}

		oldcol = SetBkColor(dis->hDC, col);
		ExtTextOut(dis->hDC, 0, 0, ETO_OPAQUE, &rect, "", 0, 0);
		SetBkColor(dis->hDC, oldcol);

		FrameRect(dis->hDC, &rect, GetSysColorBrush(COLOR_BTNSHADOW));

		if(dis->itemState & ODS_FOCUS)
		{
			InflateRect(&rect, 1, 1);
			DrawFocusRect(dis->hDC, &rect);
		}

		break;
	}
	return TRUE;
}

BOOL CALLBACK DeckOptionsProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT *)lParam;
	DRAWITEMSTRUCT    *dis = (DRAWITEMSTRUCT    *)lParam;
	int i;
	static HBITMAP hBmp;

	int width, height;
	CHOOSECOLOR cc;
	HWND hwndList;
	RECT rect1, rect2;
	int border;

	static COLORREF crBackGnd;

	switch(msg)
	{
	case WM_INITDIALOG:

		hwndList = GetDlgItem(hwnd, IDC_LIST1);

		//only 12 standard card backs
		for(i = 0; i < 12; i++)
		{
			char str[10];
			int idx;
			wsprintf(str, "%d", i);
			
			idx = SendMessage(hwndList, LB_ADDSTRING, 0, (LONG)str);
			SendMessage(hwndList, LB_SETITEMDATA, idx, i + 53);
		}

		SendDlgItemMessage(hwnd, IDC_LIST1, LB_SETCURSEL, g_nBackCardIdx - 53, 0);
		
		//resize the list so it contains exactly 3 images horizontally,
		//and 1 image vertically
		width  = (__cardwidth + SEL_BORDER * 2 + 2) * 3 + 
					GetSystemMetrics(SM_CXEDGE) * 2;

		height = (__cardheight + SEL_BORDER * 2 + 2)  +
					GetSystemMetrics(SM_CYEDGE) * 2 +
					GetSystemMetrics(SM_CYHSCROLL);

		GetWindowRect(hwnd, &rect1);
		GetWindowRect(hwndList, &rect2);

		OffsetRect(&rect2, -rect1.left, -rect1.top);
		OffsetRect(&rect1, -rect1.left, -rect1.top);
		
		//border between dialog edge and list edge
		border = (rect2.left - rect1.left);

		//resize THIS window
		SetWindowPos(hwnd, 0, 0, 0, 
			width + border * 2,
			rect1.bottom - rect1.top, 
			SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);


		SetWindowPos(hwndList, 0,0,0, width, height, SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
	
		//right-align any buttons
		RightAlign(GetDlgItem(hwnd, IDC_BUTTON2), border);
		RightAlign(GetDlgItem(hwnd, IDCANCEL), border);
		
		crBackGnd = g_crBackColour;

		return TRUE;

	case WM_MEASUREITEM:
		
		if(mis->CtlID == IDC_LIST1)
		{
			mis->itemHeight = __cardheight + SEL_BORDER*2 + 2;
			mis->itemWidth  = __cardwidth +  SEL_BORDER*2 + 2;
			return TRUE;
		}
		
		if(mis->CtlID == IDC_BUTTON1)
		{
			return FALSE;
		}

		return FALSE;

	case WM_DRAWITEM:

		if(dis->CtlID == IDC_LIST1)
			return DrawCardList(dis);

		if(dis->CtlID == IDC_BUTTON1)
			return DrawColourButton(dis, crBackGnd);


	case WM_COMMAND:

		switch(LOWORD(wParam))
		{
		case IDCANCEL:
			EndDialog(hwnd, 0);
			return TRUE;

		case IDOK:
			
			EndDialog(hwnd, 0);

			g_crBackColour = crBackGnd;
			g_nBackCardIdx = SendDlgItemMessage(hwnd, IDC_LIST1, LB_GETCURSEL, 0, 0) + 53;

			ShedBoard.SetBackColor(crBackGnd);
			ShedBoard.SetBackCardIdx(g_nBackCardIdx);
			
			ShedBoard.Redraw();

			return TRUE;

		case IDC_LIST1:
			if(HIWORD(wParam) == LBN_DBLCLK)
			{
				PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), 0);
			}
			return TRUE;

		case IDC_BUTTON1:

			ZeroMemory(&cc, sizeof(cc));
			cc.lStructSize = sizeof(cc);
			cc.hwndOwner = hwnd;
			cc.hInstance = 0;
			cc.lpCustColors = crCustColours;
			cc.Flags = CC_RGBINIT;// | CC_SOLIDCOLOR;
			cc.rgbResult = crBackGnd;

			if(ChooseColor(&cc))
			{
				crBackGnd = cc.rgbResult;
				InvalidateRect(hwnd, 0, TRUE);
			}

			break;

		case IDC_BUTTON2:
			crBackGnd = RGB(0, 64, 100);
			InvalidateRect(hwnd, 0, TRUE);
			break;
		}

		return FALSE;
		

	case WM_CLOSE:
		EndDialog(hwnd, 0);
		return TRUE;
	}
	return FALSE;
}

void ShowDeckOptionsDlg(HWND hwndParent)
{
	DialogBoxParam(GetModuleHandle(0), MAKEINTRESOURCE(IDD_DIALOG2), hwndParent, DeckOptionsProc, (DWORD)0);
}

