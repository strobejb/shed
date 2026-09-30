#include <windows.h>
#include <tchar.h>
#include "..\CardLib\CardLib.h"
#include "resource.h"
#include "Shed.h"

CardRegion *pCompHand      = 0;
CardRegion *pCompStack[3]  = { 0, 0, 0 };
CardRegion *pUserStack[3]  = { 0, 0, 0 };
CardRegion *pPickup        = 0;
CardRegion *pDeck          = 0;
CardRegion *pUserHand[13]  = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

CardButton  *pNewButton     = 0;
CardButton  *pDealButton    = 0;
CardButton  *pStartButton   = 0;
CardButton  *pPickupButton  = 0;
CardButton  *pMore1Button   = 0;
CardButton  *pMore2Button   = 0;
CardButton  *pDeckTextLabel   = 0;
CardButton  *pPickupTextLabel = 0;

CardButton  *pUserNameLabel  = 0;
CardButton  *pUserScoreLabel = 0;
CardButton  *pCompNameLabel  = 0;
CardButton  *pCompScoreLabel = 0;

CardWindow ShedBoard;

HANDLE hCompChooseEvent;

#if 0
void CreateShed(CardWindow &wnd)
{
	CardRegion *pStack = wnd.CreateStack(0, true, 10, 200, 10, 20);

	CardStack &cards = pStack->GetCardStack();

	cards.Push(Card(1));
	cards.Push(Card(20));
	cards.Push(Card(30));
	cards.Push(Card(40));
	cards.Push(Card(50));
	cards.Push(Card(5));
	cards.Push(Card(9));

	pStack->Update();

	pStack->SetThreedCount(3);

	pStack = wnd.CreateStack(0, true, 200, 200, CS_DEFXOFF, 0);

	pStack->SetFaceDirection(CS_FACE_DOWNUP, 1);
	pStack->SetEmptyImage(0);

	HBITMAP	hbmBitmap = (HBITMAP)LoadImage(0,"test.bmp",IMAGE_BITMAP,0,0,LR_LOADFROMFILE);
	wnd.SetBackImage(hbmBitmap);

}
#else

void CreateShed(CardWindow &wnd)
{
	CardButton *pButton;

	int i;

	int nCompHandY   = 12;
	int nComp3Y      = __cardheight + 40;
	int nUser3Y      = __cardheight + 20 + nComp3Y;
	int nUserHandY   = __cardheight + 40 + nUser3Y;
	int nDeckY       = nUser3Y / 2 - nComp3Y/2 + nComp3Y;

	wnd.SetBackColor(g_crBackColour);
	wnd.SetBackCardIdx(g_nBackCardIdx);

	//
	//	Create the stacks for the card game...
	//
	
	//	Computer hand
	//
	pCompHand = wnd.CreateRegion(COMP_HAND_ID, true, 0, nCompHandY, CS_DEFXOFF, 0);

	pCompHand->SetPlacement(CS_XJUST_CENTER, 0, 0, 0);
	pCompHand->SetEmptyImage(CS_EI_NONE);
	
#ifdef _DEBUG	
	pCompHand->SetFaceDirection(CS_FACE_FACEUP, 0);
#else
	pCompHand->SetFaceDirection(CS_FACE_FACEDOWN, 0);
#endif

	pCompHand->SetDragRule(0, 0);
	pCompHand->SetDropRule(0, 0);
	
	//	Computer's 3 stacks, all the same
	//
	for(i = 0; i < 3; i++)
	{
		pCompStack[i] = wnd.CreateRegion(COMP_STACK_ID + i, true, 0, nComp3Y, 0, -10);

		pCompStack[i]->SetPlacement(CS_XJUST_CENTER, 0, (i - 1) * (__cardwidth + 5), 0);
		pCompStack[i]->SetFaceDirection(CS_FACE_DOWNUP, 1);
		pCompStack[i]->SetDragRule(0, 0);
		pCompStack[i]->SetDropRule(0, 0);
	}
	
	//	User's 3 stacks, all the same
	//
	for(i = 0; i < 3; i++)
	{
		pUserStack[i] = wnd.CreateRegion(USER_STACK_ID + i, true, 0, nUser3Y, 0, +10);

		pUserStack[i]->SetPlacement(CS_XJUST_CENTER, 0, (i - 1) * (__cardwidth + 5), 0);
		pUserStack[i]->SetFaceDirection(CS_FACE_DOWNUP, 1);
		pUserStack[i]->SetDragRule(CS_DRAG_CALLBACK, UserStackCanDragProc);
		pUserStack[i]->SetDropRule(CS_DROP_CALLBACK, UserStackCanDropProc);

		pUserStack[i]->SetAddCardProc		(UserStackAddProc);
		pUserStack[i]->SetRemoveCardProc	(UserStackRemoveProc);
		pUserStack[i]->SetClickProc			(UserStackClickProc);
		pUserStack[i]->SetDblClickProc		(UserStackDblClickProc);
	}

	//	Deck
	//
	pDeck = wnd.CreateRegion(DECK_ID, true, 0, nDeckY, 2, 1);

	pDeck->SetPlacement		(CS_XJUST_CENTER, 0,  - __cardwidth * 3, 0);
	pDeck->SetFaceDirection	(CS_FACE_FACEDOWN, 0);
	pDeck->SetDragRule		(CS_DRAG_NONE, 0);
	pDeck->SetDropRule		(CS_DROP_NONE, 0);
	pDeck->SetThreedCount	(CS_DEF3D);

	//	Pickup / discard pile
	//
	pPickup = wnd.CreateRegion(PICKUP_ID, true, 0, nDeckY, 2, 1);

	pPickup->SetPlacement		(CS_XJUST_CENTER, 0,  + __cardwidth * 3, 0);
	pPickup->SetFaceDirection	(CS_FACE_FACEUP, 0);
	pPickup->SetDragRule		(CS_DRAG_NONE, 0);
	pPickup->SetDropRule		(CS_DROP_CALLBACK, PickupStackCanDropProc);
	pPickup->SetThreedCount		(CS_DEF3D);
	pPickup->SetAddCardProc		(PickupStackAddProc);

	//	User's 
	//
	for(i = 0; i < 13; i++)
	{
		pUserHand[i] = wnd.CreateRegion(USER_HAND_ID + i, false, 0, nUserHandY, 0, CS_DEFYOFF);

		//pUserHand[i]->SetPlacement(CS_XJUST_CENTER, 0,  - __cardwidth * 3, 0);
		pUserHand[i]->SetFaceDirection(CS_FACE_FACEUP, 0);
		pUserHand[i]->SetEmptyImage(CS_EI_NONE);
		
		pUserHand[i]->SetDragRule(CS_DRAG_ALL, 0);
		pUserHand[i]->SetRemoveCardProc(UserHandRemoveProc);

		pUserHand[i]->SetDblClickProc(UserHandDblClickProc);
	}

	//
	//	Now create the buttons..
	//

	pNewButton = wnd.CreateButton(NEW_BUT_ID, _T("New Game"), CB_PUSHBUTTON, true, 10,10,70,40);
	pNewButton->SetButtonProc(NewGameButtonProc);
	
	pDealButton = wnd.CreateButton(DEAL_BUT_ID, _T("Deal"), CB_PUSHBUTTON, false, 0, nDeckY + __cardheight+32, 70, 30);
	pDealButton->SetPlacement(CS_XJUST_CENTER, 0, - __cardwidth * 3, 0);
	pDealButton->SetButtonProc(DealButtonProc);

	pStartButton = wnd.CreateButton(START_BUT_ID, _T("Start"), CB_PUSHBUTTON, false, 0, nDeckY + __cardheight+32, 70, 30);
	pStartButton->SetPlacement(CS_XJUST_CENTER, 0,  -__cardwidth * 3, 0);
	pStartButton->SetButtonProc(StartButtonProc);
	
	pPickupButton = wnd.CreateButton(PICKUP_BUT_ID, _T("Pickup"), CB_PUSHBUTTON, false, 0, nDeckY + __cardheight+32, 70, 30);
	pPickupButton->SetPlacement(CS_XJUST_CENTER, 0, __cardwidth * 3, 0);
	pPickupButton->SetButtonProc(PickupButtonProc);

	pMore1Button = wnd.CreateButton(MORE1_BUT_ID, _T("<< More"), CB_PUSHBUTTON, false, 10, nUserHandY + __cardheight/2, 60, 30);
	pMore1Button->SetPlacement(CS_XJUST_NONE, 0, 0, 0);
	pMore1Button->SetButtonProc(ArrowButtonProc);
	
	pMore2Button = wnd.CreateButton(MORE2_BUT_ID, _T("More >>"), CB_PUSHBUTTON, false, 10, nUserHandY + __cardheight/2, 60, 30);
	pMore2Button->SetPlacement(CS_XJUST_RIGHT, 0, -10, 0);
	pMore2Button->SetButtonProc(ArrowButtonProc);

	pButton = wnd.CreateButton(DECK_BUTLABEL1_ID, _T("Deck"),    CB_STATIC, true, 0, nDeckY - 22, 70, 20);
	pButton->SetPlacement(CS_XJUST_CENTER, 0, - __cardwidth * 3, 0);

	pDeckTextLabel = wnd.CreateButton(DECK_BUTLABEL2_ID, _T("0 cards"), CB_STATIC, true, 0, nDeckY + __cardheight + 5, 70, 20);
	pDeckTextLabel->SetPlacement(CS_XJUST_CENTER, 0, - __cardwidth * 3, 0);
	
	pButton = wnd.CreateButton(PICKUP_BUTLABEL1_ID, _T("Pickup"),  CB_STATIC, true, 0, nDeckY - 22, 70, 20);
	pButton->SetPlacement(CS_XJUST_CENTER, 0, __cardwidth * 3, 0);

	pPickupTextLabel = wnd.CreateButton(PICKUP_BUTLABEL2_ID, _T("0 cards"), CB_STATIC, true, 0, nDeckY + __cardheight + 5, 70, 20);
	pPickupTextLabel->SetPlacement(CS_XJUST_CENTER, 0, __cardwidth * 3, 0);

	//winning labels
	HFONT hFont = CreateFont(16,0,0,0,FW_DEMIBOLD,
		0,0,0,0,0,0,0,0,"Arial");

	pUserNameLabel = wnd.CreateButton(PICKUP_BUTLABEL1_ID, _T("Player1: "),  CB_STATIC | CB_ALIGN_RIGHT, true, 0, nCompHandY, 150, 20);
	pUserNameLabel->SetText(_T("%s: "), szUserName);
	pUserNameLabel->SetPlacement(CS_XJUST_RIGHT, 0, -50, 0);
	pUserNameLabel->SetFont(hFont);

	pUserScoreLabel = wnd.CreateButton(PICKUP_BUTLABEL1_ID, _T("0"),  CB_STATIC | CB_ALIGN_LEFT, true, 0, nCompHandY, 20, 20);
	pUserScoreLabel->SetPlacement(CS_XJUST_RIGHT, 0, -30, 0);
	pUserScoreLabel->SetFont(hFont);

	pCompNameLabel = wnd.CreateButton(PICKUP_BUTLABEL1_ID, _T("Computer: "),  CB_STATIC | CB_ALIGN_RIGHT, true, 0, nCompHandY + 20, 150, 20);
	pCompNameLabel->SetPlacement(CS_XJUST_RIGHT, 0, -50, 0);
	pCompNameLabel->SetFont(hFont);
	
	pCompScoreLabel = wnd.CreateButton(PICKUP_BUTLABEL1_ID, _T("0"),  CB_STATIC | CB_ALIGN_LEFT, true, 0, nCompHandY + 20, 20, 20);
	pCompScoreLabel->SetPlacement(CS_XJUST_RIGHT, 0, -30, 0);
	pCompScoreLabel->SetFont(hFont);

	wnd.SetResizeProc(ResizeProc);

	//	User-defined drop-zone
	//
	RECT rect;
	SetRect(&rect, 0, nUserHandY, 1280, nUserHandY + __cardheight*2);
	wnd.RegisterDropZone(1, &rect, DropZoneProc);

	/*hIcon = LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(IDI_ICON1));
	pButton->SetIcon(hIcon, true);*/

#ifdef _DEBUG
	pButton = wnd.CreateButton(666, _T("Debug"), CB_PUSHBUTTON, true, 10, 60, 70, 40);
	pButton->SetButtonProc(DebugButtonProc);

#endif
}


#endif
