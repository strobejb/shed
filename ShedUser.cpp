#include <windows.h>
#include <tchar.h>
#include "..\CardLib\CardLib.h"
#include "Shed.h"
#include "resource.h"

int nHScrollMax = 0;
int nHScrollPos = 0;
int nHScrollVis = 0;

int nNumUserWins = 0;
int nNumCompWins = 0;

void Fudge();


DWORD WINAPI GameOverThreadProc(void *pParam)
{
	Sleep(2000);
	fGameStarted = false;
	NewGameButtonProc(*pNewButton);

	return 0;
}

void GameOver(bool fUserWin)
{
	if(fUserWin == true)
		nNumUserWins++;
	else
		nNumCompWins++;

	for(int i = 0; i < 3; i++)
	{
		if(pUserStack[i]->NumCards() > 1)
		{
			CardStack &cs = pUserStack[i]->GetCardStack();
			cs.Pop(cs.NumCards() - 1);
		}

		if(pCompStack[i]->NumCards() > 1)
		{
			CardStack &cs = pCompStack[i]->GetCardStack();
			cs.Pop(cs.NumCards() - 1);
		}
	}

	for(i = 0; i < 13; i++)
		pUserHand[i]->Clear();

	//keep track of the cards that are no longer in play
	deadlist.Add(pPickup->GetCardStack());
	
	pPickup->Clear();
	pDeck->Clear();
	pCompHand->Clear();
	
	for(i = 0; i < 3; i++)
	{
		pUserStack[i]->SetFaceDirection(CS_FACE_FACEUP, 0);
		pCompStack[i]->SetFaceDirection(CS_FACE_FACEUP, 0);

		pUserStack[i]->Flash(6, 100);
		pCompStack[i]->Flash(6, 100);
	}

	fGameStarted = false;

	//HANDLE hThread = CreateThread(0, 0, GameOverThreadProc, 0, 0, 0);
	//CloseHandle(hThread);

	pUserScoreLabel->SetText(_T("%d"), nNumUserWins);
	pUserScoreLabel->Redraw();

	pCompScoreLabel->SetText(_T("%d"), nNumCompWins);
	pCompScoreLabel->Redraw();

}

void DealToUser(int nMaxCards)
{
	int nNumUserCards = CountUserCards();

	while(nNumUserCards < nMaxCards && pDeck->NumCards() > 0)
	{
		//see top card
		CardStack &cardstack = pDeck->GetCardStack();
		
		int val = cardstack[0].LoVal() - 1;
		
		CardRegion *pUser = ShedBoard.CardRegionFromId(USER_HAND_ID + val);
		
		//if a stack already exists, drop the card onto that one
		if(pUser->IsVisible())
		{
			pDeck->MoveCards(pUser, 1, true);
		}
		
		//otherwise, need to "create" a new stack, by
		//showing the appropriate user-stack which is hidden
		else
		{
			pUser->Show(true);
			
			ShedBoard.DistributeStacks(USER_HAND_ID, 13, CS_XJUST_CENTER, 4, 0);
			ShedBoard.Update();
			ShedBoard.Redraw();

			pDeck->MoveCards(pUser, 1, true);
			
			//UpdateStackPositions();
			//MoveCards(DECK_ID, iUserStackIds[numericvalue], 1, 1);//i < 2 ? 1 : 0);
			//return iUserStackIds[numericvalue];
		}

		nNumUserCards++;

		pDeckTextLabel->SetText(_T("%d cards"), pDeck->NumCards());
		pDeckTextLabel->Redraw();
	}
}

void CARDLIBPROC NewGameButtonProc(CardButton &Button)
{
	int i;

	//
	//	Interrupt the computer's card choosing...
	//
	SetEvent(hCompChooseEvent);

	//
	//	Reset AI state
	//
	playercount.Clear();
	deadlist.Clear();
	
	CardWindow &shed = Button.GetCardWindow();

	shed.EmptyStacks();
		
	CardRegion *pDeck = shed.CardRegionFromId(DECK_ID);

	CardStack &cardstack = pDeck->GetCardStack();
	cardstack.NewDeck();
	
	cardstack.Shuffle();
	cardstack.Shuffle(); 
	cardstack.Shuffle();
	cardstack.Shuffle();

	for(i = 0; i < 13; i++)
	{
		CardRegion *pUser = shed.CardRegionFromId(USER_HAND_ID+i);
		pUser->Show(false);
	}
	
	for(i = 0; i < 3; i++)
	{
		pUserStack[i]->SetFaceDirection(CS_FACE_DOWNUP, 1);
		pCompStack[i]->SetFaceDirection(CS_FACE_DOWNUP, 1);
	}

	//
	//	Show/hide buttons
	//
	pStartButton->Show(false);
	pDealButton->Show(true);
	pPickupButton->Show(false);
	
	pPickupTextLabel->SetText(_T("0 cards"));
	pDeckTextLabel->SetText(_T("52 cards"));
	
	pDeck->Update();

	shed.Update();
	shed.Redraw();

	fGameStarted = false;
	fCompChooseFinished = false;
}

void CARDLIBPROC DealButtonProc(CardButton &Button)
{
	int i;
	CardWindow &shed = Button.GetCardWindow();

	CardLib_SetZoomSpeed(64);

	for(i = 0; i < 6; i++)
		pDeck->MoveCards(COMP_HAND_ID, 1, true);

	pDeck->MoveCards(COMP_STACK_ID + 0, 1, true);
	pDeck->MoveCards(COMP_STACK_ID + 1, 1, true);
	pDeck->MoveCards(COMP_STACK_ID + 2, 1, true);

	pDeck->MoveCards(USER_STACK_ID + 0, 1, true);
	pDeck->MoveCards(USER_STACK_ID + 1, 1, true);
	pDeck->MoveCards(USER_STACK_ID + 2, 1, true);

	//CardButton *pDeckText = shed.GetButton(

	//
	//	Now deal 6 cards into user's hand...
	//
	DealToUser(6);

	Button.Show(false);

	shed.DistributeStacks(USER_HAND_ID, 13, CS_XJUST_CENTER, 4, 0);

	pDeckTextLabel->SetText(_T("%d cards"), pDeck->NumCards());

	//Don't show start button until all stacks full
	//pStartButton->Show(true);

	shed.Update();
	shed.Redraw();

	CardLib_SetZoomSpeed(32);

	fCompChooseFinished = false;
	ComputerChooseCards();
}

//
//	Start the game - true to let player go first,
//  false to make us wait for the computer..
//
bool StartGame(CardButton &Button)
{
	int nMinUserStartCard;
	CardWindow &shed = Button.GetCardWindow();
	CardCount usercount;

	if(fCompChooseFinished == false)
		return false;

	if( pUserStack[0]->NumCards() == 2 &&
		pUserStack[1]->NumCards() == 2 &&	
		pUserStack[2]->NumCards() == 2 )	
	{
		pStartButton->Show(false);
		pPickupButton->Show(true);
	
		//Fudge();

		shed.Redraw();

		fGameStarted = true;

		//Find what the user's lowest card is in his hand
		for(int n = 0; n < 13; n++)
		{
			usercount.Add( pUserHand[n]->GetCardStack() );
		}

		nMinUserStartCard = GetMinCard(usercount, 0, false);

		if(nMinUserStartCard == -1) nMinUserStartCard = 3;
		if(nMinCompStartCard == -1) nMinCompStartCard = 3;

		//if the computer has a lower card, let him go first
		if(nMinCompStartCard < nMinUserStartCard)
		{
			ComputerGo();
			return false;
		}
		else
		{
			return true;
		}

	}
	else
	{
		return false;
	}
}

void CARDLIBPROC StartButtonProc(CardButton &Button)
{
	(void)StartGame(Button);	
}

void CARDLIBPROC PickupButtonProc(CardButton &Button)
{
	int val;
	int i;
	int nNumCards = pPickup->NumCards(); 

	if(nNumCards == 0) return;

	//Be nice to ourselves - if we have a face-up card on
	//the BOTTOM layer of any of the three stacks, then
	//we need to also pick this up
	for(i = 0; i < 3; i++)
	{
		if(pUserStack[i]->GetFaceDirection(0) == CS_FACE_FACEUP)
			pUserStack[i]->MoveCards(pPickup, 1, false);	
	}

	CardStack &cardstack = pPickup->GetCardStack();

	//Player has picked up some cards...so now we know
	//SOME of the cards he has..
	playercount.Add(cardstack);

	//send all cards to user pile
	for(i = 0; i < nNumCards; i++)
	{
		//see top card
		CardStack &cardstack = pPickup->GetCardStack();

		//get value of top card - put into range 0-12
		val = cardstack[0].LoVal() - 1;
	
		//if a stack already exists, drop the card onto that one
		if(pUserHand[val]->NumCards() > 0)
		{
			pPickup->MoveCards(pUserHand[val], 1, i < 2 ? 1 : 0);
		}
	
		//otherwise, need to "create" a new stack, by
		//showing the appropriate user-stack which is hidden
		else
		{
			pUserHand[val]->Show(true);
			
			ShedBoard.DistributeStacks(USER_HAND_ID, 13, CS_XJUST_CENTER, 4, ShedBoard.GetWidth() / 2);
			ShedBoard.Update();

			pPickup->MoveCards(pUserHand[val], 1, i < 2 ? 1 : 0);
		}
	}

	ResizeProc(ShedBoard.GetWidth(), ShedBoard.GetHeight());
	
	pPickupTextLabel->SetText(_T("%d cards"), 0);

	ShedBoard.Update();
	ShedBoard.Redraw();

	//computer's go now (only if we picked something up, anyway)
	ComputerGo();
	return;
}

void CARDLIBPROC ArrowButtonProc(CardButton &Button)
{
	switch(Button.Id())
	{
	case MORE1_BUT_ID:
		if(--nHScrollPos < 0) 
			nHScrollPos = 0;
		break;

	case MORE2_BUT_ID:
		if(++nHScrollPos >= (nHScrollMax-nHScrollVis)) 
			nHScrollPos = nHScrollMax - nHScrollVis;
		break;
	}

	ResizeProc(ShedBoard.GetWidth(), ShedBoard.GetHeight());
	
	ShedBoard.Update();
	ShedBoard.Redraw();

}

bool CARDLIBPROC stackdragproc(CardRegion &region, int nNumDragging)
{
	return true;
}

void CARDLIBPROC ResizeProc(int width, int height)
{
	bool fVisibleUserId[13];

	int i, numfull = 0;

	int maxvisitems = (width - 140) / (__cardwidth + 4);

	for(i = 0; i < 13; i++)
	{
		if(pUserHand[i]->NumCards() != 0)
		{
			fVisibleUserId[i] = true;
			numfull++;
		}
		else
		{
			fVisibleUserId[i] = false;
		}
	}

	nHScrollMax = numfull;
	nHScrollVis = maxvisitems;

	//if can show all user's hand without need for scrolling
	//buttons, then hide the buttons
	if(((__cardwidth+4) * numfull) < width)
	{
		pMore1Button->Show(false);
		pMore2Button->Show(false);
		
		//show all the user stacks
		for(i = 0; i < 13; i++)
		{
			if(fVisibleUserId[i]) 
				pUserHand[i]->Show(true);
		}

		nHScrollPos = 0;
	}
	//otherwise, hide the necessary outer stacks, and
	//show the buttons
	else
	{
		//find a visible stack to hide
		int j, nummadevis = 0;
		int numhidden = 0;
		
		//hide any which are hidden by scrolling
		for(i = 0; i < 13 && numhidden < nHScrollPos; i++)
		{
			if(fVisibleUserId[i])
			{
				pUserHand[i]->Show(false);
				fVisibleUserId[i] = false;
				numhidden++;
			}
		}

		for(j = 0; j < 13; j++)
		{
			if(fVisibleUserId[j] && nummadevis < maxvisitems)
			{
				nummadevis++;
				pUserHand[j]->Show(true);
			}
			else
				pUserHand[j]->Show(false);
		}

		pMore1Button->Show(nHScrollPos == 0 ? false : true);
		pMore2Button->Show(nHScrollPos == nHScrollMax - nHScrollVis ? false : true);
	}

	ShedBoard.DistributeStacks(USER_HAND_ID, 13, CS_XJUST_CENTER, 4, width / 2);
	ShedBoard.Update();
}

