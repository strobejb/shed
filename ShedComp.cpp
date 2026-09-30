#include <windows.h>
#include <tchar.h>
#include "..\CardLib\CardLib.h"
#include "Shed.h"
#include "..\Catch22Lib\trace.h"

//#define CARD_COUNT_START  2
//#define CARD_COUNT_END    14

const int AceHigh = 14;


HANDLE hCompThread = 0;
int		nMinCompStartCard = 3;
int		nSkillLevel	= SKILL_NORMAL;

CardCount playercount;
CardCount deadlist;			// list of cards that we KNOW have been discarded

bool fCompChooseFinished = false;

#define PAUSE_BEFORE_COMP_GO_MS 100 //250

bool PLAY_AGAIN(int val)
{
	return (val == 10) ? true : false;
}

void RefillCompHand()
{
	int i;
	int nNumDeckCards = pDeck->NumCards();
	int nNumInHand    = pCompHand->NumCards();

	if(nNumInHand < 3 && nNumDeckCards != 0)
	{
		CardLib_SetZoomSpeed(16);
		
		for(i = 0; i < 3 - nNumInHand; i++)
		{
			pDeck->MoveCards(pCompHand, 1, true);
		}
		
		ShedBoard.Update();
		pDeckTextLabel->SetText(_T("%d cards"), pDeck->NumCards());
		
		CardLib_SetZoomSpeed(32);
	}
}

void CompPickup()
{
	pPickup->MoveCards(pCompHand, pPickup->NumCards(), true);
	pPickupTextLabel->SetText(_T("0 cards"));
	
	ShedBoard.Update();
	ShedBoard.Redraw();
}

//
//	find minimum card VALUE (aces high) to play
//	fAllCards - (true) -returns -1 for special cards
//		       (false) -returns special card values
//
int GetMinCard(CardCount &cardcount, int compareto, bool fAllCards)
{
	int i;

	for(i = 2; i <= AceHigh; i++)
	{
		if(i == 2 || i == 10 || i == AceHigh)	 //2's, 10's or Aces
			continue;

		//find smallest value which we can play
		if(cardcount[i] > 0 && i >= compareto)
			return i;
	}

	//At this point, there are either NO other lower cards,
	//or we have a 2,10 or Ace
	if(fAllCards)
	{
		//rank in order : 2, Ace, 10
		if(cardcount[2])       return 2;
		if(cardcount[AceHigh]) return AceHigh;
		if(cardcount[10])      return 10;
	}
	else
	{
		//don't return values for 2's or 10's
		if(cardcount[AceHigh])	   return AceHigh;
	}

	return -1;
}

//
//	find maximum card VALUE to play
//
int GetMaxCard(CardCount &cardcount)
{
	int i;

	for(i = AceHigh; i >= 2; i--)
	{
		if(i == 10 || i == 2) continue;	//2's or 10's

		if(cardcount[i] > 0)
			return i;	
	}

	return -1;
}

//
//	Get BEST card
//
int GetBestCard(CardCount &cardcount)
{
	if(cardcount[2])	return 2;		//prefer 2's over aces
	if(cardcount[10])	return 10;

	return GetMaxCard(cardcount);
}

//
//	build two lists for the cards in the 3 stacks
//  this list is an un-ordered "normal" list (i.e. not
//  an array with values for the number of cards for each array element)
//
int GetStackCards(CardRegion *pStack[], CardStack &bottom, CardStack &top)
{
	int i, j, num;

	bottom.Clear();
	top.Clear();

	for(i = 0; i < 3; i++)
	{
		CardStack &cardstack = pStack[i]->GetCardStack();
		
		num = cardstack.NumCards();
		
		if(num >= 1)
		{
			bottom.Push(cardstack[num - 1]);
		}
		
		if(num >= 2) 
		{
			
			for(j = 1; j < num; j++)
			{
				top.Push(cardstack[num - j - 1]);
			}
		}
	}

	return 0;
}

void MakeLegal(CardCount &cardcount, int nMinCardVal)
{
	if(nMinCardVal == 1)		nMinCardVal = AceHigh;
	if(nMinCardVal <= 2)		return;
	if(nMinCardVal >  AceHigh)	return;

	for(int i = 2; i < nMinCardVal; i++)
	{
		//Never adjust 2s or 10s
		if(i == 2 || i == 10)
			continue;

		while(cardcount[i] != 0)
				cardcount.Dec(i);
	}
}

//
//	Return the number of cards in the cardcount object
//
int CountCardCount(CardCount &cardcount)
{
	int i, total = 0;
	
	for(i = 2; i <= AceHigh; i++)
	{
		total += cardcount[i];
	}

	return total;
}

//
//	Find a card to play from the supplied list
//  of cards. Use the acquired knowledge of the pickup pile
//	and the user's cards which we know to help decide
//	which card to play
//
int FindCardToPlay(CardCount &cardcount, int *pNumToPlay, 
				   int nNumTopStack, int nNumBotStack, int nNumCompHand)
{
	int index = 0;
	int nTopCardVal = -1;
	int nNumToPlay = 0;
	int nPlayCardVal = -1;
	
	int nNumUserHand   = CountUserCards();
	int nNumUserStacks, nNumUserBot, nNumUserTop;

	int nNumOnStacks = nNumTopStack + nNumBotStack;

	bool fKnowAll = false;		//do we know exactly what player has??
	
	// Player card details
	CardStack usertop, userbottom;
	CardCount usertopcount;

	int nMaxPlayerCard  = GetMaxCard(playercount);

	GetStackCards(pUserStack, userbottom, usertop);

	usertopcount.Init(usertop);

	nNumUserBot = userbottom.NumCards();
	nNumUserTop = usertop.NumCards();

	nNumUserStacks = nNumUserBot + nNumUserTop;
	
	if(pNumToPlay == 0) 
	{
		return -1;
	}

	//if the we don't have any cards!
	if(nNumOnStacks == 0 && nNumCompHand == 0)
	{
		*pNumToPlay = 0;
		return -1;
	}

	//
	//	Work out if we know exactly what player cards are...
	//
	if(pDeck->NumCards() == 0 && nNumUserHand == CountCardCount(playercount))
	{

	}

	//
	// Find the value of the top-most card in the pickup pile
	// APPLY THE SEE-THROUGH RULE HERE (don't ignore the rule)
	//
	nTopCardVal = GetTopOfPickupPile();	
	MakeLegal(cardcount, nTopCardVal);

	//
	//  Find the VALUE of the lowest card (2-14)???
	//	This function will return -1 if we don't have a card to play
	//  (even if our hand is full)
	//
	nPlayCardVal = GetMinCard(cardcount, nTopCardVal, true);
	
	//Didn't find a card, can't go!!
	if(nPlayCardVal == -1)
	{
		//DAMN! Can't go, so must pickup
		*pNumToPlay = 0;
		return -1;
	}

	if(nSkillLevel == SKILL_EASY)
	{
		goto exit_point;
	}
		

	//
	//	EXPERT RULE #1
	//	If we know what the player's hand is, then base
	//	our decision on that!
	//

	//
	//	SPECIAL RULE #1
	//	If we have n cards left of same value (and none in stacks) then
	//	play them all to win!
	//
	if(nNumOnStacks == 0 && cardcount[nPlayCardVal] == nNumCompHand)
	{
		TRACE("Special Rule #1\n");
		*pNumToPlay = nNumCompHand;
		return nPlayCardVal;
	}
		
	//
	//	SPECIAL RULE #2
	//	If we have a 10, and any other single card, then play the 10 first
	//  to get auto win!
	//
	if(nNumOnStacks == 0 && cardcount[10] > 0 && nPlayCardVal != 10
		&& nNumCompHand == (cardcount[10] + cardcount[nPlayCardVal]))
	{
		TRACE("Special Rule #2\n");
		*pNumToPlay = 1;
		return 10;
	}
		
	//
	//	SPECIAL RULE #3
	//	If player has 1 card left, then play our highest card
	//
	if(nNumUserStacks == 1 && nNumUserHand == 0 ||
		nNumUserHand == 1  && nNumUserStacks == 0)
	{
		/*if(fKnowAll)
		{
			
		}
		else*/
		{
			TRACE("Special Rule #3\n");
			*pNumToPlay = 1;

			nPlayCardVal = GetMaxCard(cardcount);

			if(nPlayCardVal == -1)
			{
				*pNumToPlay = 0;	// MUST set this back to zero!!
				return -1;
			}
		}

		return nPlayCardVal;
	}

	//
	//	SPECIAL RULE #4
	//	If there are hardly any cards on pickup pile, and we know
	//  there is an ace in it, then pick it up...!!
	//
	if(nNumCompHand != 0 && pPickup->NumCards() <= 2 && pDeck->NumCards() > 5)
	{
		//ONLY do this if we don't already have an ace,
		//and we have a 10, but not a 2
		if(cardcount[1] == 0 && cardcount[2] == 0 && cardcount[10] > 0)
		{
			
			CardStack &pickup = pPickup->GetCardStack();
			for(int i = 0; i < pickup.NumCards(); i++)
				if(pickup[i].HiVal() == AceHigh)
				{
					*pNumToPlay = 0;
					return -1;
				}
		}
	}
	
	//
	//	SPECIAL RULE #5
	//	Play ace's one-at-a-time
	//
	if(nPlayCardVal == AceHigh && nNumUserHand != 0)
	{
		TRACE("Special Rule #4\n");
		*pNumToPlay = 1;
		return nPlayCardVal;
	}
	
	//
	//	SPECIAL RULE #6
	//  If there are alot of cards in pickup pile, and we have
	//	a ten at our disposal, then clear it
	//
	if(pPickup->NumCards() > (10 + rand() % 10) && cardcount[10] > 0)
	{
		TRACE("Special Rule #5\n");
		*pNumToPlay = 1;
		return 10;
	}

	//
	//	SPECIAL RULE #7
	//	If we can see the player's top cards then play 1 higher
	//	so he can't go!
	//
	if(nNumUserHand == 0 && nNumUserTop != 0)
	{
		//no point if the user has a 2 or 10
		if(usertopcount[2] == 0 && usertopcount[10] == 0)
		{
			int pc = GetMaxCard(usertopcount);
			pc = max(pc, nTopCardVal);
		
			if(-1 != pc)
			{
				MakeLegal(cardcount, nTopCardVal);
				pc = GetMinCard(cardcount, pc, false);
			}

			if(-1 != pc)
			{	
				TRACE("Special Rule #6!!\n");
				*pNumToPlay = 1;
				return pc;
			}
		}
	}

	//
	//	SPECIAL RULE #7
	//	Prefer 2's over Ace's!
	//
	if(nPlayCardVal == AceHigh && cardcount[2] > 0)
	{
		TRACE("Special Rule #7\n");
		nPlayCardVal = 2;
	}

	//
	//	General rules follow
	//	Play minimum card
	//
exit_point:

	nNumToPlay = cardcount[nPlayCardVal];

	//
	//	Restrict the flow of our special cards
	//
	if(nPlayCardVal == 2)		nNumToPlay = 1;	//Two's
	if(nPlayCardVal == 10)		nNumToPlay = 1;	//Ten's
	
	if(nSkillLevel > 0)
		if(nPlayCardVal == AceHigh)	nNumToPlay = 1;	//Ace's

	*pNumToPlay = nNumToPlay;
	return nPlayCardVal;
}

//
//	Search the card stacks for the specified card, and 
//	return which stack contains it
//
CardRegion *FindTopStackFromCardValue(CardRegion *pStack[], int cardvalue)
{
	int nNumCards;
	
	//loop through every registered card stack until we find
	//the one containg the search value
	for(int i = 0; i < 3; i++)
	{
		CardStack &cardstack = pStack[i]->GetCardStack();

		nNumCards = cardstack.NumCards();

		//only return the stack if there is a face-up card on offer
		for(int j = 0; j < nNumCards - 1; j++)
		{
			if(cardstack[j].HiVal() == cardvalue)
				return pStack[i];
		}
	}

	return 0;
}

//
//	Computer's go. Just play 1 card (or cards of same value)
//	return false to stop, or true to play again (i.e. if we played a 10)
//  return the value of the card to play..
//
bool ComputerTurn()
{
	//int topstackcards[52], bottomstackcards[52];
	CardStack topstackcards, botstackcards;
	CardCount cardstoplay;

	CardStack &comphand = pCompHand->GetCardStack();

	int nNumToPlay = 0;
	int nPlayCardVal = 0;
	int nNumUserHand = CountUserCards();

	int nNumCompHand = comphand.NumCards();	//store a list of cards in compcards[]
	int nNumTopStack = 0;
	int nNumBotStack = 0;
	
	//
	// Count the number of cards in each stack and store 
	// them as a list in the arrays for the face-up and face-down rows.
	// NOTE we must NEVER look in the bottomstackcards array. This is cheating!!
	//
	GetStackCards(pCompStack, botstackcards, topstackcards);

	nNumTopStack = topstackcards.NumCards();
	nNumBotStack = botstackcards.NumCards();

	static int counter;
	//
	// If there is a card in the computer's hand, then must play
	// that first of all.
	//
	if(nNumCompHand != 0)
	{
		//count the computer's cards nicely and fill the cardstoplay count array up
		cardstoplay.Init(comphand);

		//find the best card to play from this list
		nPlayCardVal = FindCardToPlay(cardstoplay, &nNumToPlay, nNumTopStack, nNumBotStack, nNumCompHand);

		//find the cards to play in the user's hand
		if(nNumToPlay != 0)
		{
			pCompHand->PlayCard(pPickup, nPlayCardVal, nNumToPlay);
			//TRACE("[%d] Playing %d x %d\n", counter++, nPlayCardVal, nNumToPlay);
			return PLAY_AGAIN(nPlayCardVal);
		}
		//otherwise, pickup!!
		else
		{
			CompPickup();
			TRACE("Failed1\n");
			return false;
		}
	}
	//
	// Otherwise, select a card to play from the 3 card stacks
	//
	else
	{
		//if there are still face-up cards on the computer's stacks,
		//play them before the bottom ones
		if(nNumTopStack != 0)
		{
			cardstoplay.Init(topstackcards);

			nPlayCardVal = FindCardToPlay(cardstoplay, &nNumToPlay, nNumTopStack, nNumBotStack, nNumCompHand);

			if(nNumToPlay != 0)
			{
				TRACE("[%d] Playing %d x %d\n", counter++, nPlayCardVal, nNumToPlay);

				//ONLY PLAY THE CARDS FROM THE TOP!!!!!!!
				while(nNumToPlay--)
				{
					CardRegion *pStack = FindTopStackFromCardValue(pCompStack, nPlayCardVal);

					if(pStack == 0)
					{
						char ach[80];
						wsprintf(ach, "Couldn't play a %d", nPlayCardVal);
						MessageBox(ShedBoard, ach, "Internal Error", MB_OK|MB_ICONEXCLAMATION);
						break;
					}
					
					//only play a card if it is face-up!!
					if(pStack->NumCards() > 1)
						pStack->PlayCard(pPickup, nPlayCardVal, 1);
				}
			
				return PLAY_AGAIN(nPlayCardVal);
			}
			else
			{
				//move the pickup pile to our hand
				CompPickup();
				return false;
			}
		}

		//
		// Down to the bottom face-down cards. Select one at random
		// pickup the pile if we failed
		//
		else if(nNumBotStack != 0)
		{
			bool fFoundCard = false;
			int toppickupval = GetTopOfPickupPile();
			CardRegion *pSource = 0;

			//if there is a stack with a face-up card on the bottom, then
			//we must play that one....
			/*for(n = 0; n < 3; n++)
			{
				if(pCompStack[n]->NumCards() > 0)// && GetNumFaceDown(COMP_STACK_ID+n) == 0)
				{
					fFoundCard = true;
					pSource = pCompStack[n];
				}
			}*/

			//choose a random stack that has a card in
			while(!fFoundCard)
			{
				pSource = pCompStack[rand() % 3];
				if(pSource->NumCards() > 0) 
					fFoundCard = true;
			}

			//SetNumFaceDown(stackid, 0);
			//RedrawCardWindow();

			CardStack &cardstack = pSource->GetCardStack();
			nPlayCardVal = cardstack[0].HiVal();

			//play the card if it is a 2,10, or greater than the pickup pile
			if(nPlayCardVal == 2 || nPlayCardVal == 10 || nPlayCardVal >= toppickupval)
			{
				TRACE("[%d] Playing %d x %d\n", counter++, nPlayCardVal, 1);

				pSource->PlayCard(pPickup, nPlayCardVal, 1);
				return PLAY_AGAIN(nPlayCardVal);
			}
			//otherwise, better pickup..
			else
			{
				pSource->PlayCard(pPickup, nPlayCardVal, 1);
				Sleep(400);			
				pPickup->Flash(6, 100);
				Sleep(700);			//second pause before emptying the stack
				CompPickup();
				return false;
			}
		}
		//no cards to play...maybe we need to refill our hand?
		else
		{
			return false;
		}
	}

	TRACE("Failed..\n");
	return false;
}


DWORD WINAPI ComputerThreadProc(void *lParam)
{
	//need to loop here, as we might play a series of 10's etc,
	//or put 4 of the same value down etc.
	pPickup->Lock();
	pPickupButton->Lock();
	pNewButton->Lock();

	Sleep(PAUSE_BEFORE_COMP_GO_MS);

	while(ComputerTurn())
	{
		Sleep(100);
		pPickup->Flash(6, 100);
		Sleep(700);			
		
		//keep track of the cards that are no longer in play
		deadlist.Add(pPickup->GetCardStack());

		pPickup->Clear();

		pPickupTextLabel->SetText(_T("%d cards"), pPickup->NumCards());
		ShedBoard.Redraw();

		if(fAutoRefill == true)
		{
			Sleep(200);
			RefillCompHand();
		}
		
	}

	pPickupTextLabel->SetText(_T("%d cards"), pPickup->NumCards());

	ShedBoard.Update();
	ShedBoard.Redraw();

	Sleep(200);
	
	if(CountStackCards(pCompStack) == 0 && pCompHand->NumCards() == 0)
	{
		MessageBox(ShedBoard, _T("I Win!!"), szAppName, MB_OK|MB_ICONINFORMATION);
		GameOver(false);
	}
	else
	{
		RefillCompHand();
	}

	pPickup->UnLock();
	pPickupButton->UnLock();
	pNewButton->UnLock();

	return 0;
}

void ComputerGo()
{
	//if computer is already going....
	if(hCompThread != 0)
	{
		WaitForSingleObject(hCompThread, INFINITE);
		CloseHandle(hCompThread);
	}

	for(int i = 1; i <= 13; i++)
	{
		if(playercount[i] > 0)
		{
			switch(i)
			{
			case 1:  TRACE("Ace[%d] ",   playercount[i]); break;
			case 11: TRACE("Jack[%d] ",  playercount[i]); break;
			case 12: TRACE("Queen[%d] ", playercount[i]); break;
			case 13: TRACE("King[%d] ",  playercount[i]); break;
			default: TRACE("%d[%d] ", i, playercount[i]); break;

			}
		}
	}
	
	hCompThread = CreateThread(0, 0, ComputerThreadProc, 0, 0, 0);
}

DWORD WINAPI ComputerChooseProc(void *lParam)
{
	int deal[3];		//values of 3 cards to play
	int n = 0;
	DWORD dwFirstAdjust = 500;
	DWORD dwTimeout = 750;

	CardCount cardcount;
	int nCardVal;

	int nNumCompHand = pCompHand->NumCards();

	CardStack &comphand = pCompHand->GetCardStack();

	cardcount.Init(comphand);

	//clip, so that we never place more than 2x 2's or 10's..
	for(n = 2; n <= AceHigh; n++)
	{
		if(n == 2 || n == 10)
		{
			while(cardcount[n] > 2)
				cardcount.Dec(n);
		}
	}

	//choose the best three cards from the hand..
	for(n = 0; n < 3; n++)
	{
		nCardVal = GetBestCard(cardcount);
		
		if(nCardVal == 2 && cardcount[AceHigh] > 0)
			nCardVal = AceHigh;
		
		cardcount.Dec(nCardVal);	//can't use it any more!
		deal[n] = nCardVal;
	}

	//deal them!
	for(n = 0; n < 3; n++)
	{
		if(WAIT_OBJECT_0 == WaitForSingleObject(hCompChooseEvent, dwTimeout - dwFirstAdjust))
				return 0;

		pCompHand->PlayCard(pCompStack[n], deal[n], 1);

		dwFirstAdjust = 0;
	}

	//find what our lowest card is
	nMinCompStartCard = GetMinCard(cardcount, 0, false);

	fCompChooseFinished = true;
	ShedBoard.Redraw();


	return 0;
}


void ComputerChooseCards()
{
	static HANDLE hThread;

	if(hThread != 0)
	{
		//WaitForSingleObject(hThread, INFINITE);
		//CloseHandle(hThread);
	}

	//set to "uninterrupted"
	ResetEvent(hCompChooseEvent);

	hThread = CreateThread(0, 0, ComputerChooseProc, 0, 0, 0);
	CloseHandle(hThread);
}