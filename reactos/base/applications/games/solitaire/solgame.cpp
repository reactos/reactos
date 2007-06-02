#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include <stdio.h>
#include "resource.h"
#include "cardlib/cardlib.h"
//#include "../catch22lib/trace.h"
#include "solitaire.h"

#if 1
#define TRACE(s)
#else
#define TRACE(s) printf("%s(%i): %s",__FILE__,__LINE__,s)
#endif

CardStack activepile;
bool fGameStarted = false;

void NewGame(void)
{
    TRACE("ENTER NewGame()\n");
    int i, j;

    SolWnd.EmptyStacks();
    
    //create a new card-stack
    CardStack deck;
    deck.NewDeck();
    deck.Shuffle();
    activepile.Clear();

    //deal to each row stack..
    for(i = 0; i < NUM_ROW_STACKS; i++)
    {
        CardStack temp;
        temp.Clear();

        pRowStack[i]->SetFaceDirection(CS_FACE_DOWNUP, i);

        for(j = 0; j <= i; j++)
        {
            temp.Push(deck.Pop());
        }

        pRowStack[i]->SetCardStack(temp);
    }

    //put the other cards onto the deck
    pDeck->SetCardStack(deck);
    pDeck->Update();
    
    SolWnd.Redraw();

    fGameStarted = false;
    TRACE("EXIT NewGame()\n");
}

//
//    Now follow the stack callback functions. This is where we
//  provide the game functionality and rules
//

//
//    Can only drag face-up cards
//
bool CARDLIBPROC RowStackDragProc(CardRegion &stackobj, int iNumDragCards)
{
    TRACE("ENTER RowStackDragProc()\n");
    int numfacedown;
    int numcards;

    stackobj.GetFaceDirection(&numfacedown);

    numcards = stackobj.NumCards();

    TRACE("EXIT RowStackDragProc()\n");
    if(iNumDragCards <= numcards-numfacedown)
        return true;
    else
        return false;

}

//
//    Row a row-stack, we can only drop cards 
//    that are lower / different colour
//
bool CARDLIBPROC RowStackDropProc(CardRegion &stackobj,  const CardStack &dragcards)
{
    TRACE("ENTER RowStackDropProc()\n");
    Card dragcard = dragcards[dragcards.NumCards() - 1];

    //if we are empty, can only drop a stack with a King at bottom
    if(stackobj.NumCards() == 0)
    {
        if(dragcard.LoVal() != 13)
        {
            TRACE("EXIT RowStackDropProc(false)\n");
            return false;
        }
    }
    else
    {
        const CardStack &mystack = stackobj.GetCardStack();
        
        //can only drop if card is 1 less
        if(mystack[0].LoVal() != dragcard.LoVal() + 1)
        {
            TRACE("EXIT RowStackDropProc(false)\n");
            return false;
        }

        //can only drop if card is different colour
        if( mystack[0].IsBlack() && !dragcard.IsRed() ||
           !mystack[0].IsBlack() &&  dragcard.IsRed() )
        {
            TRACE("EXIT RowStackDropProc(false)\n");
            return false;
        }
    }

    TRACE("EXIT RowStackDropProc(true)\n");
    return true;
}

//
//    Can only drop a card onto a suit-stack if the
//  card is 1 higher, and is the same suit
//
bool CanDrop(CardRegion &stackobj, Card card)
{
    TRACE("ENTER CanDrop()\n");
    int topval;

    const CardStack &cardstack = stackobj.GetCardStack();

    if(cardstack.NumCards() > 0)
    {
        if(card.Suit() != cardstack[0].Suit())
        {
            TRACE("EXIT CanDrop()\n");
            return false;
        }

        topval = cardstack[0].LoVal();
    }
    else
    {
        topval = 0;
    }

    //make sure 1 higher
    if(card.LoVal() != (topval + 1))
    {
        TRACE("EXIT CanDrop()\n");
        return false;
    }

    TRACE("EXIT CanDrop()\n");
    return true;
}

//
//    Can only drop a card onto suit stack if it is same suit, and 1 higher
//
bool CARDLIBPROC SuitStackDropProc(CardRegion &stackobj, const CardStack &dragcards)
{
    TRACE("ENTER SuitStackDropProc()\n");
    //only drop 1 card at a time
    if(dragcards.NumCards() != 1)
    {
        TRACE("EXIT SuitStackDropProc()\n");
        return false;
    }

    bool b = CanDrop(stackobj, dragcards[0]);
    TRACE("EXIT SuitStackDropProc()\n");
    return b;
}

//
//    Single-click on one of the row-stacks
//    Turn the top-card over if they are all face-down
//
void CARDLIBPROC RowStackClickProc(CardRegion &stackobj, int iNumClicked)
{
    TRACE("ENTER RowStackClickProc()\n");
    int numfacedown;
    
    stackobj.GetFaceDirection(&numfacedown);

    //if all face-down, then make top card face-up
    if(stackobj.NumCards() == numfacedown)
    {
        if(numfacedown > 0) numfacedown--;
        stackobj.SetFaceDirection(CS_FACE_DOWNUP, numfacedown);
        stackobj.Redraw();
    }
    TRACE("EXIT RowStackClickProc()\n");
}

//
//    Find the suit-stack that can accept the specified card
//
CardRegion *FindSuitStackFromCard(Card card)
{
    TRACE("ENTER FindSuitStackFromCard()\n");
    for(int i = 0; i < 4; i++)
    {
        if(CanDrop(*pSuitStack[i], card))
        {
            TRACE("EXIT FindSuitStackFromCard()\n");
            return pSuitStack[i];
        }
    }

    TRACE("EXIT FindSuitStackFromCard()\n");
    return 0;
}

//
//    What happens when we add a card to one of the suit stacks?
//  Well, nothing (it is already added), but we need to
//  check all four stacks (not just this one) to see if
//  the game has finished.
//
void CARDLIBPROC SuitStackAddProc(CardRegion &stackobj, const CardStack &added)
{
    TRACE("ENTER SuitStackAddProc()\n");
    bool fGameOver = true;

    for(int i = 0; i < 4; i++)
    {
        if(pSuitStack[i]->NumCards() != 13)
        {
            fGameOver = false;
            break;
        }
    }

    if(fGameOver)
    {
        MessageBox(SolWnd, _T("Congratulations, you win!!"), szAppName, MB_OK | MB_ICONINFORMATION);
    
        for(int i = 0; i < 4; i++)
        {
            pSuitStack[i]->Flash(11, 100);
        }
    }
    TRACE("EXIT SuitStackAddProc()\n");
}

//
//    Double-click on one of the row stacks
//    The aim is to find a suit-stack to move the
//  double-clicked card to.
//
void CARDLIBPROC RowStackDblClickProc(CardRegion &stackobj, int iNumClicked)
{
    TRACE("ENTER RowStackDblClickProc()\n");
    //can only move 1 card at a time
    if(iNumClicked != 1)
    {
        TRACE("EXIT RowStackDblClickProc()\n");
        return;
    }

    //find a suit-stack to move the card to...
    const CardStack &cardstack = stackobj.GetCardStack();
    CardRegion *pDest = FindSuitStackFromCard(cardstack[0]);
    
    if(pDest != 0)
    {
        //stackobj.MoveCards(pDest, 1, true);
        //use the SimulateDrag funcion, because we get the
        //AddProc callbacks called for us on the destination stacks...
        stackobj.SimulateDrag(pDest, 1, true);
    }
    TRACE("EXIT RowStackDblClickProc()\n");
}

//
//    Face-up pile double-click
//
void CARDLIBPROC PileDblClickProc(CardRegion &stackobj, int iNumClicked)
{
    TRACE("ENTER PileDblClickProc()\n");
    RowStackDblClickProc(stackobj, iNumClicked);
    TRACE("EXIT PileDblClickProc()\n");
}

//
//    What happens when a card is removed from face-up pile?
//
void CARDLIBPROC PileRemoveProc(CardRegion &stackobj, int iItems)
{
    TRACE("ENTER PileRemoveProc()\n");
    //modify our "virtual" pile by removing the same card
    //that was removed from the physical card stack
    activepile.Pop(iItems);

    //if there is just 1 card left, then modify the
    //stack to contain ALL the face-up cards..the effect
    //will be, the next time a card is dragged, all the
    //previous card-triplets will be available underneath
    if(stackobj.NumCards() == 1)
    {
        stackobj.SetOffsets(0,0);
        stackobj.SetCardStack(activepile);
    }
    TRACE("EXIT PileRemoveProc()\n");
}

//
//    Double-click on the deck
//    Move 3 cards to the face-up pile
//
void CARDLIBPROC DeckClickProc(CardRegion &stackobj, int iNumClicked)
{
    TRACE("ENTER DeckClickProc()\n");
    CardStack cardstack = stackobj.GetCardStack();
    CardStack pile      = pPile->GetCardStack();

    fGameStarted = true;

    //reset the face-up pile to represent 3 cards
    pPile->SetOffsets(CS_DEFXOFF, 1);

    if(cardstack.NumCards() == 0)
    {
        pile.Clear();

        activepile.Reverse();
        cardstack.Push(activepile);
        activepile.Clear();
    }
    else
    {
        int numcards = min((nOptions & OPTION_THREE_CARDS) ? 3 : 1, cardstack.NumCards());

        //make a "visible" copy of these cards
        CardStack temp;
        temp = cardstack.Pop(numcards);
        temp.Reverse();

        pile.Clear();
        pile.Push(temp);

        //remove the top 3 from deck
        activepile.Push(temp);
    }

    activepile.Print();

    pDeck->SetCardStack(cardstack);
    pPile->SetCardStack(pile);

    SolWnd.Redraw();
    TRACE("EXIT DeckClickProc()\n");
}
