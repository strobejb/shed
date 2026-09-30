#include <windows.h>
#include <tchar.h>
#include "..\CardLib\CardLib.h"
#include "Shed.h"
#include "resource.h"

static bool fDebug = false;

#ifdef _DEBUG

BOOL CALLBACK DebugProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static CardRegion *pStack;
	int i;
	char ach[400];

	switch(msg)
	{
	case WM_INITDIALOG:
		pStack = (CardRegion *)lParam;

		ach[0] = 0;

		for(i = pStack->NumCards() - 1; i >= 0; i--)
		{
			CardStack &cards = pStack->GetCardStack();

			char temp[8];
			wsprintf(temp, "%d, ", cards[i].LoVal());
			lstrcat(ach, temp);
		}

		SetDlgItemText(hwnd, IDC_EDIT1, ach);
		return TRUE;

	case WM_COMMAND:

		switch(LOWORD(wParam))
		{
		case IDCANCEL:
			EndDialog(hwnd, 0);
			return TRUE;

		case IDOK:
			
			{
				char *ptr;
				int num;
				int ch;
				char *t;
				char temp[80];

				GetDlgItemText(hwnd, IDC_EDIT1, ach, 400);

				CardStack &cards = pStack->GetCardStack();
				cards.Clear();

				ptr = ach;
				ch = *ptr++;
				
				while(ch != '\0')
				{
					while(ch == ' ' || ch == ',') 
						ch = *ptr++;

					switch(ch)
					{
					case '0': case '1': case '2': case '3': case '4':
					case '5': case '6': case '7': case '8': case '9':
						
						t = temp;
						while(isdigit(ch))
						{
							*t++ = ch;
							ch = *ptr++;
						}
						
						*t = '\0';
						num = atoi(temp);

						if(num <= 0) num = 1;
						if(num > 13) num = 13;

						num = (num - 1) * 4 + (rand() % 4);		//pick a random suit..

						cards.InsertCard(cards.NumCards(), Card(num));
						//cards.Push(Card(num, true));
						break;

					default:
						ch = *ptr++;
					}
				}

				pStack->Update();
				ShedBoard.Update();
				ShedBoard.Redraw();
				EndDialog(hwnd, 0);

			}
			return TRUE;
		}

		return FALSE;

	case WM_CLOSE:
		EndDialog(hwnd, 0);
		return TRUE;
	}
	return FALSE;
}

bool CARDLIBPROC DebugClickProc(CardRegion &stack)
{
	if(fDebug)
	{
		fDebug = false;
		DialogBoxParam(GetModuleHandle(0), MAKEINTRESOURCE(IDD_DIALOG1), ShedBoard, DebugProc, (DWORD)&stack);
		return false;
	}
	else
		return true;
}

void CARDLIBPROC DebugButtonProc(CardButton &button)
{
	fDebug = true;
	CardLib_SetStackClickProc(DebugClickProc);
}
#endif
