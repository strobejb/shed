#include <windows.h>
#include <tchar.h>
#include "..\CardLib\CardLib.h"
#include "Shed.h"
#include "resource.h"

//
//	Game rules
//
bool fGameStarted			= false;
bool fAutoRefill			= true;		//refill everytime play a card
bool fSevensSeeThrough		= false;
int  SEE_THROUGH_CARD_VALUE = 7;		//sevens are see-through

//
//	Internal game state
//
bool fClearPickupActive		= false;
bool fZoomUserCardsBack		= false;

int CountUserCards()
{
	int count = 0;

	for(int i = 0; i < 13; i++)
		count += pUserHand[i]->NumCards();

	return count;
}

void AutoClearPickup(DWORD dwParam)
{
	fClearPickupActive = false;

	//keep track of the cards that are no longer in play
	deadlist.Add(pPickup->GetCardStack());

	pPickup->Clear();
	pPickup->StopFlash();
	
	ShedBoard.Redraw();
}

//
//	Start the pickup pile flashing, and register an event
//	to clear the pickup pile, once the flashing has passed
//	(note: the two functions are independent, it is up to us
//	to make sure that the time-outs are such that the clear event
//	will always occur after the flashing stops
//
void ClearPickupPile()
{
	pPickupTextLabel->SetText(_T("0 cards"));
	pPickupTextLabel->Redraw();

	pPickup->Flash(6, 100);
	
	fClearPickupActive = true;
	
	//set timer to go off in 6 * 100ms...
	SetTimer(hwndMain, SHED_CLEAR_STACK_TIMER, 600, 0);
	//uClearStackEventId = RegisterEvent(AutoClearPickup, 0, 600);
}

int CountStackCards(CardRegion *pThreeStacks[])
{
	return pThreeStacks[0]->NumCards() + 
		   pThreeStacks[1]->NumCards() + 
		   pThreeStacks[2]->NumCards();
}

//	return the value of the card at the top of the pickup pile
//	apply the see-through card rule here
//
int GetTopOfPickupPile()
{
	int nNumPickups, i;
	
	CardStack &cardstack = pPickup->GetCardStack();

	nNumPickups = pPickup->NumCards();

	if(nNumPickups <= 0)
		return 0;

	if(fSevensSeeThrough == true)
	{
		for(i = 0; i < nNumPickups; i++)
		{
			if(cardstack[i].HiVal() != SEE_THROUGH_CARD_VALUE)	
				return cardstack[i].HiVal();
		}
	
		return (nNumPickups != 0) ? (SEE_THROUGH_CARD_VALUE) : 0;
	}
	else
	{
		if(nNumPickups > 0)
			return cardstack[0].HiVal();
	}

	return 0;
}

//
//	a dropzone callback must return which stack id
//	to allow the drop to occur on,
//	or -1 for fail
//
int  CARDLIBPROC DropZoneProc(int dzid, const CardStack &cards)
{
	int foundstack = -1;
	int val;

	if(cards.NumCards() < 1 || cards.NumCards() > 1) return CS_DROPZONE_NODROP;
	
	//see what value the card we are dropping is
	//make it range 0-12
	val = cards[0].LoVal() - 1;
	
	if(fGameStarted == true)
	{
		return CS_DROPZONE_NODROP;
	}

	//if a stack already exists, drop the card onto that one
	if(pUserHand[val]->NumCards() > 0)
	{
		return pUserHand[val]->Id();
	}
	
	//otherwise, need to "create" a new stack, by
	//showing the appropriate user-stack which is hidden
	else
	{
		pUserHand[val]->Show(true);
		
		ShedBoard.DistributeStacks(USER_HAND_ID, 13, CS_XJUST_CENTER, 4, ShedBoard.GetWidth() / 2);
		ShedBoard.Update();

		return pUserHand[val]->Id();
	}
	
	//never allow drop to take place - we will do this
	//ourselves
	return CS_DROPZONE_NODROP;
}

//
//	Can drag cards from one of the three user-stacks?
//
bool CARDLIBPROC UserStackCanDragProc (CardRegion &region, int iNumDragging)
{
	CardStack &cardstack = region.GetCardStack();

	int nNumCards = region.NumCards();

	//whilst playing game, can only drag cards
	//from user stacks if there are no cards in the user's
	//hand, and none on the deck either
	if(fGameStarted)
	{
		//Can only drag face-up cards from the bottom layer
		if(nNumCards == 1 && region.GetFaceDirection(0) != CS_FACE_FACEUP)
			return false;

		// Cannot drag the bottom cards from the user stacks if 
		// any other user stack contains face-up cards
		if(nNumCards > 1 && iNumDragging == nNumCards) 
			return false;
		
		if( nNumCards == 1 &&
		  ( pUserStack[0]->NumCards() > 1 ||
			pUserStack[1]->NumCards() > 1 ||
			pUserStack[2]->NumCards() > 1 ))  
			return false;
		
		if(CountUserCards() != 0 || pDeck->NumCards() != 0)
			return false;

		return true;
	}
	//otherwise, if setting up the cards
	else
	{
		//not allowed to remove the bottom-most card
		if(cardstack.NumCards() - iNumDragging >= 1)
			return true;
		else
			return false;
	}
}

//
//	Can drop the specified cards on user-stack?
//
bool CARDLIBPROC UserStackCanDropProc (CardRegion &region, const CardStack &dropcards)
{
	CardStack &cardstack = region.GetCardStack();

	//
	// Cannot drop cards on the stacks, once we have started
	//
	//	The only exception is when we are down to the three stacks,
	//  and we want to drag a card from one stack onto another
	//  so we can play them both at the same time (when same value)
	//
	if(fGameStarted)
	{
		//cannot drop cards on an empty place-holder during game play
		if(region.NumCards() == 0)
			return false;

		//cannot drop a card if we are down to the last face-down layer
		if(region.NumCards() <= 1)
			return false;

		/*if( pUserStack[0]->NumCards() == 0 ||
			pUserStack[1]->NumCards() == 0 ||
			pUserStack[2]->NumCards() == 0 )  
			return false;*/

		if(CountUserCards() != 0 || pDeck->NumCards() != 0)
			return false;
	
		if(CountUserCards() == 0 && pDeck->NumCards() == 0 && region.NumCards() > 1)
		{
			if(dropcards[0].HiVal() == cardstack[0].HiVal())
				return true;
		}

		//can only drop same valued cards, unless there is just
		//a face-down card on the bottom
		if(cardstack.NumCards() > 1 && dropcards[0].HiVal() != cardstack[0].HiVal())
			return false;

		return false;
	}
	//
	// otherwise, if setting up the cards
	//
	else
	{
		if(cardstack.NumCards() + dropcards.NumCards() > 2)
			return false;
		else
			return true;
	}

	return false;
}

//
//	Post-add notification for one of the three user-stacks
//
void CARDLIBPROC UserStackAddProc (CardRegion &region, const CardStack &cards)
{
	//if we are setting up the game prior to
	//playing, then show the start-button once all
	//3 stacks have their top card positioned
	if(fGameStarted == false)
	{
		if( pUserStack[0]->NumCards() == 2 &&
			pUserStack[1]->NumCards() == 2 &&
			pUserStack[2]->NumCards() == 2 )
		{
			pStartButton->Show(true);
		}
		else
		{
			pStartButton->Show(false);
		}
	}

	//we rely on the other stack-procs to let the window get redrawn
}

//
//	Post-remove notification from one of the three user-stacks
//
void CARDLIBPROC UserStackRemoveProc (CardRegion &region, int nNumRemoved)
{
	//if we are setting up the game prior to
	//playing, then show the start-button once all
	//3 stacks have their top card positioned
	if(fGameStarted == false)
	{
		if( pUserStack[0]->NumCards() == 2 &&
			pUserStack[1]->NumCards() == 2 &&
			pUserStack[2]->NumCards() == 2 )
		{
			pStartButton->Show(true);
		}
		else
		{
			pStartButton->Show(false);
		}
	}

	//we rely on the other stack-procs to let the window get redrawn
}

//
//	Post-add notification of card added to user's hand
//
void CARDLIBPROC UserHandAddProc (CardRegion &region, const CardStack &cards)
{
/*	ResizeProc(ShedBoard.GetWidth(), ShedBoard.GetHeight());

	ShedBoard.DistributeStacks(USER_HAND_ID, 13, CS_XJUST_CENTER, 4, ShedBoard.GetWidth() / 2);
	ShedBoard.Update();*/
}

//
//	Post-removal of a card from the user's hand
//
void CARDLIBPROC UserHandRemoveProc (CardRegion &region, int nNumRemoved)
{
	CardStack &cardstack = region.GetCardStack();
	
	ResizeProc(ShedBoard.GetWidth(), ShedBoard.GetHeight());

	//if the stack has no cards left in it, then hide
	//the stack, and realign the others
	if(cardstack.NumCards() == 0)
	{
		region.Show(false);
		ShedBoard.DistributeStacks(USER_HAND_ID, 13, CS_XJUST_CENTER, 4, ShedBoard.GetWidth() / 2);
		ShedBoard.Update();
	}
}

//
//	Single-click on one of the three user-stacks
//
void CARDLIBPROC UserStackClickProc		(CardRegion &region, int iNumCards)
{
	if(fGameStarted)
	{
		//when we get down to the last card (face-down),
		//then turn it face-up when we click on it..
		//ONLY do this if there are no other face-up cards!!
		
		if(	CountUserCards()  == 0 &&
			pDeck->NumCards() == 0 &&
			
			pUserStack[0]->NumCards() <= 1 && 
			pUserStack[1]->NumCards() <= 1 &&
			pUserStack[2]->NumCards() <= 1 )
		{
			int id = region.Id();

			for(int i = 0; i < 3; i++)
			{
				//if one of the other two stacks already has
				//a face-up card on the bottom...
				if( pUserStack[i]->Id() != id && 
					pUserStack[i]->NumCards() > 0 &&
					pUserStack[i]->GetFaceDirection(0) == CS_FACE_FACEUP)
				{
					return;
				}
			}
			
			region.SetFaceDirection(CS_FACE_FACEUP, 0);
			region.Redraw();
		}
	}
}

//
//	Double-click on pne of the three user-stacks
//
void CARDLIBPROC UserStackDblClickProc	(CardRegion &region, int iNumCards)
{
	if(fGameStarted)
	{
		//play a card to the pickup pile
		region.SimulateDrag(pPickup, iNumCards, true);
	}
	else
	{
		//remove the card from this stack, put it back on the
		//user-stack...
		//need to simulate drop zones in...
	}
}

//
//	Double-click on one of the user's hands
//
void CARDLIBPROC UserHandDblClickProc	(CardRegion &region, int iNumCards)
{
	if(fGameStarted)
	{
		//play a card to the pickup pile
		region.SimulateDrag(pPickup, iNumCards, true);
	}
	else
	{
		//if we have set the three stacks off, then start!
		if( pUserStack[0]->NumCards() == 2 && 
			pUserStack[1]->NumCards() == 2 &&
			pUserStack[2]->NumCards() == 2 )
		{
			if(StartGame(*pStartButton))
				region.SimulateDrag(pPickup, iNumCards, true);
	
			return;
		}

		//otherwise, place a card on one of the three stacks
		for(int i = 0; i < 3; i++)
		{
			if(pUserStack[i]->NumCards() == 1)
			{
				region.SimulateDrag(pUserStack[i], iNumCards, true);
				break;
			}
		}
	}
}

//
//	Can drop specified cards on pickup pile?
//
bool CARDLIBPROC PickupStackCanDropProc (CardRegion &region, const CardStack &dropcards)
{
	if(!fGameStarted)
		return false;

	int topcard;
	int dropval;

	//NUM_PUT_DOWN = numdropping;
	
	//if there is an event registered which will delete the contents of the pickup stack,
	//(i.e. allowing the stack to be flashed on/off before deleting it)
	//then we must stop the event before adding these cards
	if(fClearPickupActive == true)
	{
		fClearPickupActive = false;

		KillTimer(hwndMain, SHED_CLEAR_STACK_TIMER);
		
		//keep track of the cards that are no longer in play
		deadlist.Add(pPickup->GetCardStack());

		pPickup->StopFlash();
		pPickup->Clear();
		return true;
	}
	
	if(!fGameStarted) 
		return false;

	if(pPickup->NumCards() == 0) 
		return true;

	dropval = dropcards[0].HiVal();

	//can always drop 2's and aces in shed
	if(dropval == 2 || dropval == 14)
	{
		return true;
	}

	//if drop a 10, then clear the pickup deck
	if(dropval == 10)
	{
		//let the PickupStackAddProc(..) function do this after the
		//cards have dropped
		return true;
	}
	
	//aces high
	topcard = GetTopOfPickupPile();

	//check drop cards are all same/higher than top card
	if(dropval < topcard) 
	{
		//
		//	** CARD IS INVALID **
		//
		// However, if have just dropped a user-stack card, and our hand is
		// empty, then let the card drop - in pickup_onadd, we will
		// ZOOM the cards back to the user
		//
		if(CountUserCards() == 0 && pDeck->NumCards() == 0)
		{
			if( pUserStack[0]->NumCards() <= 1 && 
				pUserStack[1]->NumCards() <= 1 && 
				pUserStack[2]->NumCards() <= 1    )
			{
				fZoomUserCardsBack = true;
				return true;
			}
		}

		return false;
	}
	else
	{
		return true;
	}
}

//
//	Post-addition notification of pickup pile
//
void CARDLIBPROC PickupStackAddProc(CardRegion &Pickup, const CardStack &cards)
{
	int nNumUserCards = CountUserCards();
	
	int val = cards[0].HiVal();	//all cards will be same value

	CardLib_SetZoomSpeed(16);

	//
	// Only the USER will drop cards onto the pickup pile...
	// So, any cards he drops, remove them from our "spy" pile
	//
	playercount.Sub(cards);


	//refill the user's hand before the computer can
	if(fAutoRefill == false)
	{
		if(nNumUserCards == 0 || val != 10)
		{
			DealToUser(3);
		}
	}
	else
	{
		DealToUser(3);
	}

	CardLib_SetZoomSpeed(32);

	//spy_playerputdown(iCardValues[iNumCards-1], NUM_PUT_DOWN);

	pPickupTextLabel->SetText(_T("%d cards"), Pickup.NumCards());
	pPickupTextLabel->Redraw();

	//if user dropped cards but they were invalid, then need
	//to pick them up again...
	if(fZoomUserCardsBack == true)
	{
		//send the cards to the user's hand, by "simulating"
		//a click on the pickup button
		Sleep(500);

		pPickupTextLabel->SetText(_T("0 cards"));
		
		//The button proc calls ComputerGo() for us..
		PickupButtonProc(*pPickupButton);
		fZoomUserCardsBack = false;

		return;
	}

	fZoomUserCardsBack = false;

	//
	//	Keep track of what the user doesn't have---
	//
	//spy_playerstack += cards;

	if(cards[0].HiVal() == 10)
	{
		//show the 10
		ClearPickupPile();
		
		if(CountUserCards() == 0 && CountStackCards(pUserStack) == 0)
		{
			ShedBoard.Redraw();

			MessageBox(ShedBoard, _T("You Win!!!"), szAppName, MB_ICONINFORMATION|MB_OK);
			GameOver(true);
		}

		//Don't let the computer go!
		return;
	}

	pPickupTextLabel->SetText(_T("%d cards"), cards.NumCards());

	if(CountUserCards() == 0 && CountStackCards(pUserStack) == 0)
	{
		ShedBoard.Redraw();

		MessageBox(ShedBoard, _T("You Win!!!"), szAppName, MB_ICONINFORMATION|MB_OK);
		GameOver(true);
	}
	else
	{
		ComputerGo();
	}
}
