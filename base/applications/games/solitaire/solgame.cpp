#include "solitaire.h"

#if 1
#define TRACE(s)
#else
#define TRACE(s) printf("%s(%i): %s",__FILE__,__LINE__,s)
#endif

extern TCHAR MsgWin[128];
extern TCHAR MsgDeal[128];

CardStack activepile;
int VisiblePileCards;
int LastId;
bool fGameStarted = false;
bool bAutoroute = false;

void NewGame(void)
{
    TRACE("ENTER NewGame()\n");
    int i, j;

    if (GetScoreMode() == SCORE_VEGAS)
    {
        if ((dwOptions & OPTION_KEEP_SCORE) && (dwPrevMode == SCORE_VEGAS))
            lScore = lScore - 52;
        else
            lScore = -52;

        if (dwOptions & OPTION_THREE_CARDS)
            dwWasteTreshold = 2;
        else
            dwWasteTreshold = 0;

    }
    else
    {
        if (dwOptions & OPTION_THREE_CARDS)
            dwWasteTreshold = 3;
        else
            dwWasteTreshold = 0;

        lScore = 0;
    }

    dwTime = 0;
    dwWasteCount = 0;
    LastId = 0;

    SolWnd.EmptyStacks();

    //create a new card-stack
    CardStack deck;
    deck.NewDeck();
    deck.Shuffle();
    activepile.Clear();
    VisiblePileCards = 0;

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

    // For the 1-card-mode, all cards need to be completely overlapped
    if(!(dwOptions & OPTION_THREE_CARDS))
        pPile->SetOffsets(0, 0);

    SolWnd.Redraw();

    fGameStarted = false;

    dwPrevMode = GetScoreMode();

    UpdateStatusBar();
    ClearUndo();

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

    SetPlayTimer();

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
bool CARDLIBPROC RowStackDropProc(CardRegion &stackobj, CardStack &dragcards)
{
    TRACE("ENTER RowStackDropProc()\n");
    Card dragcard = dragcards[dragcards.NumCards() - 1];

    SetPlayTimer();

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
        if( (mystack[0].IsBlack() && !dragcard.IsRed()) ||
           (!mystack[0].IsBlack() &&  dragcard.IsRed()) )
        {
            TRACE("EXIT RowStackDropProc(false)\n");
            return false;
        }
    }

    fGameStarted = true;

    SetUndo(LastId, stackobj.Id(), dragcards.NumCards(), lScore, VisiblePileCards);

    if (LastId == PILE_ID)
    {
        if (GetScoreMode() == SCORE_STD)
        {
            lScore = lScore + 5;
        }
    }
    else if ((LastId >= SUIT_ID) && (LastId <= SUIT_ID + 3))
    {
        if (GetScoreMode() == SCORE_STD)
        {
            lScore = lScore >= 15 ? lScore - 15 : 0;
        }
        else if (GetScoreMode() == SCORE_VEGAS)
        {
            lScore = lScore >= -47 ? lScore - 5 : -52;
        }
    }

    UpdateStatusBar();

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

    SetPlayTimer();

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
bool CARDLIBPROC SuitStackDropProc(CardRegion &stackobj, CardStack &dragcards)
{
    TRACE("ENTER SuitStackDropProc()\n");

    SetPlayTimer();

    //only drop 1 card at a time
    if (!bAutoroute && dragcards.NumCards() != 1)
    {
        TRACE("EXIT SuitStackDropProc()\n");
        return false;
    }

    bool b = CanDrop(stackobj, dragcards[0]);
    TRACE("EXIT SuitStackDropProc()\n");

    if (b)
    {
        SetUndo(LastId, stackobj.Id(), 1, lScore, VisiblePileCards);

        if ((LastId == PILE_ID) || (LastId >= ROW_ID))
        {
            if (GetScoreMode() == SCORE_VEGAS)
            {
                lScore = lScore + 5;
            }
            else if (GetScoreMode() == SCORE_STD)
            {
                lScore = lScore + 10;
            }

            UpdateStatusBar();
        }
    }

    return b;
}

//
//    Single-click on one of the suit-stacks
//
void CARDLIBPROC SuitStackClickProc(CardRegion &stackobj, int iNumClicked)
{
    TRACE("ENTER SuitStackClickProc()\n");

    fGameStarted = true;

    LastId = stackobj.Id();

    TRACE("EXIT SuitStackClickProc()\n");
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

        if (GetScoreMode() == SCORE_STD)
        {
            lScore = lScore + 5;
            UpdateStatusBar();
        }
        ClearUndo();
    }

    LastId = stackobj.Id();

    fGameStarted = true;

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

    SetPlayTimer();

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
        KillTimer(hwndMain, IDT_PLAYTIMER);
        PlayTimer = 0;

        if ((dwOptions & OPTION_SHOW_TIME) && (GetScoreMode() == SCORE_STD))
        {
            lScore = lScore + (700000 / dwTime);
        }

        UpdateStatusBar();
        ClearUndo();

        MessageBox(SolWnd, MsgWin, szAppName, MB_OK | MB_ICONINFORMATION);

        for(int i = 0; i < 4; i++)
        {
            pSuitStack[i]->Flash(11, 100);
        }

        if( IDYES == MessageBox(SolWnd, MsgDeal, szAppName, MB_YESNO | MB_ICONQUESTION) )
        {
            NewGame();
        }
        else
        {
            SolWnd.EmptyStacks();

            fGameStarted = false;
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

    SetPlayTimer();

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
        fGameStarted = true;
        SetPlayTimer();

        //stackobj.MoveCards(pDest, 1, true);
        //use the SimulateDrag function, because we get the
        //AddProc callbacks called for us on the destination stacks...
        bAutoroute = true;
        stackobj.SimulateDrag(pDest, 1, true);
        bAutoroute = false;
    }
    TRACE("EXIT RowStackDblClickProc()\n");
}

//
//    Face-up pile single-click
//
void CARDLIBPROC PileClickProc(CardRegion &stackobj, int iNumClicked)
{
    TRACE("ENTER SuitStackClickProc()\n");

    fGameStarted = true;

    LastId = stackobj.Id();

    TRACE("EXIT SuitStackClickProc()\n");
}

//
//    Face-up pile double-click
//
void CARDLIBPROC PileDblClickProc(CardRegion &stackobj, int iNumClicked)
{
    TRACE("ENTER PileDblClickProc()\n");

    SetPlayTimer();

    RowStackDblClickProc(stackobj, iNumClicked);
    TRACE("EXIT PileDblClickProc()\n");
}

//
//    Fix for the 3-card play when only 1 card left on the pile.
//
void FixIfOneCardLeft(void)
{
    // If there is just 1 card left, then modify the
    // stack to contain ALL the face-up cards. The effect
    // will be, the next time a card is dragged, all the
    // previous card-triplets will be available underneath.
    if ((dwOptions & OPTION_THREE_CARDS) && pPile->NumCards() == 1)
    {
        pPile->SetOffsets(0, 0);
        pPile->SetCardStack(activepile);
    }
}

//
//    What happens when a card is removed from face-up pile?
//
void CARDLIBPROC PileRemoveProc(CardRegion &stackobj, int iItems)
{
    TRACE("ENTER PileRemoveProc()\n");

    SetPlayTimer();

    //modify our "virtual" pile by removing the same card
    //that was removed from the physical card stack
    activepile.Pop(iItems);
    if ((dwOptions & OPTION_THREE_CARDS) && (VisiblePileCards > 1))
    {
        --VisiblePileCards;
    }

    FixIfOneCardLeft();

    TRACE("EXIT PileRemoveProc()\n");
}

//
//    Double-click on the deck
//    Move 3 cards to the face-up pile
//
void CARDLIBPROC DeckClickProc(CardRegion &stackobj, int iNumClicked)
{
    TRACE("ENTER DeckClickProc()\n");

    SetPlayTimer();

    CardStack cardstack = stackobj.GetCardStack();
    CardStack pile      = pPile->GetCardStack();

    fGameStarted = true;
    SetPlayTimer();

    //reset the face-up pile to represent 3 cards
    if(dwOptions & OPTION_THREE_CARDS)
        pPile->SetOffsets(CS_DEFXOFF, 1);

    if(cardstack.NumCards() == 0)
    {
        if (GetScoreMode() == SCORE_VEGAS)
        {
            if (dwWasteCount < dwWasteTreshold)
            {
                pile.Clear();

                activepile.Reverse();
                cardstack.Push(activepile);
                activepile.Clear();
                SetUndo(PILE_ID, DECK_ID, cardstack.NumCards(), lScore, VisiblePileCards);
                VisiblePileCards = 0;
            }
        }
        else if (GetScoreMode() == SCORE_STD)
        {
            SetUndo(PILE_ID, DECK_ID, activepile.NumCards(), lScore, VisiblePileCards);
            if ((dwWasteCount >= dwWasteTreshold) && (activepile.NumCards() != 0))
            {
                if (dwOptions & OPTION_THREE_CARDS)
                    lScore = lScore >= 20 ? lScore - 20 : 0;
                else
                    lScore = lScore >= 100 ? lScore - 100 : 0;
            }

            pile.Clear();

            activepile.Reverse();
            cardstack.Push(activepile);
            activepile.Clear();
            VisiblePileCards = 0;

            UpdateStatusBar();
        }
        else
        {
            pile.Clear();

            activepile.Reverse();
            cardstack.Push(activepile);
            activepile.Clear();
            SetUndo(PILE_ID, DECK_ID, cardstack.NumCards(), lScore, VisiblePileCards);
            VisiblePileCards = 0;
        }

        dwWasteCount++;
    }
    else
    {
        int numcards = min((dwOptions & OPTION_THREE_CARDS) ? 3 : 1, cardstack.NumCards());

        SetUndo(DECK_ID, PILE_ID, numcards, lScore, VisiblePileCards);

        //make a "visible" copy of these cards
        CardStack temp;
        temp = cardstack.Pop(numcards);
        temp.Reverse();

        if(dwOptions & OPTION_THREE_CARDS)
            pile.Clear();

        pile.Push(temp);

        //remove the top 3 from deck
        activepile.Push(temp);

        VisiblePileCards = numcards;
    }

    activepile.Print();

    pDeck->SetCardStack(cardstack);
    pPile->SetCardStack(pile);

    FixIfOneCardLeft();

    SolWnd.Redraw();
    TRACE("EXIT DeckClickProc()\n");
}
