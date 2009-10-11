/*
 * PROJECT:      Spider Solitaire
 * LICENSE:      See COPYING in top level directory
 * FILE:         base/applications/games/spider/lang/spigame.cpp
 * PURPOSE:      Spider Solitaire game functions
 * PROGRAMMER:   Gregor Schneider
 */

#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include <stdio.h>

#include "resource.h"
#include "cardlib.h"
#include "spider.h"

#define NUM_DECK_CARDS     5
#define NUM_SMALLER_STACKS 4
#define NUM_CARD_COLORS    4
#define NUM_ONECOLOR_CARDS 13
#define NUM_STD_CARDS      52
#define NUM_SPIDER_CARDS   104

CardStack deck;
CardRegion *from;
CardRegion *pDeck;
CardRegion *pStack[NUM_STACKS];
bool fGameStarted = false;
int  yRowStackCardOffset;
int cardsFinished;
extern TCHAR MsgDeal[];
extern TCHAR MsgWin[];

CardStack CreatePlayDeck()
{
    CardStack newStack;
    int i, colors = 1, num = 0;
    
    switch (dwDifficulty)
    {
        case IDC_DIF_ONECOLOR:
            colors = 1;
            break;
        case IDC_DIF_TWOCOLORS:
            colors = 2;
            break;
        case IDC_DIF_FOURCOLORS:
            colors = 4;
            break;
    }
    for (i = 0; i < NUM_SPIDER_CARDS; i++)
    {
        num += NUM_CARD_COLORS / colors;
        Card newCard(num % NUM_STD_CARDS);
        newStack.Push(newCard);
    }
    return newStack;
}

void NewGame(void)
{
    int i, j;
    /* First four stack with five, all other with 4 */
    int covCards = 5; 
    CardStack fakeDeck, temp;

    SpiderWnd.EmptyStacks();

    /* Create a new card-deck, fake deck */
    deck = CreatePlayDeck();
    deck.Shuffle();
    fakeDeck.NewDeck();
    fakeDeck.Shuffle();
    
    /* Reset progress value */
    cardsFinished = 0;

    /* Deal to each stack */
    for (i = 0; i < NUM_STACKS; i++)
    {
        temp.Clear();       
        if (i == NUM_SMALLER_STACKS)
        {
            covCards--;
        }
        for (j = 0; j <= covCards; j++)
        {
            temp.Push(deck.Pop(1));
        }
        pStack[i]->SetFaceDirection(CS_FACE_DOWNUP, covCards);
        pStack[i]->SetCardStack(temp);
    }
    /* Deal five fake cards to the deck */
    pDeck->SetCardStack(fakeDeck.Pop(5));  

    SpiderWnd.Redraw();
    fGameStarted = false;
}

/* Click on the deck */
void CARDLIBPROC DeckClickProc(CardRegion &stackobj, int NumDragCards)
{
    CardStack temp, fakeDeck = pDeck->GetCardStack();
    fGameStarted = true;
    int i;
    
    if (fakeDeck.NumCards() != 0 && deck.NumCards() != 0)
    {
        /* Add one card to every stack */
        for (i = 0; i < NUM_STACKS; i++)
        {
            temp = pStack[i]->GetCardStack();
            temp.Push(deck.Pop());
            pStack[i]->SetCardStack(temp);
        }
        /* Remove one card from the fake ones */
        pDeck->SetCardStack(fakeDeck.Pop(fakeDeck.NumCards() - 1));
    }
    pDeck->Update();
    SpiderWnd.Redraw();
}

/* Cards dragged friom a stack */
bool CARDLIBPROC StackDragProc(CardRegion &stackobj, int numDragCards)
{
    int numfacedown, numcards, i;

    stackobj.GetFaceDirection(&numfacedown);
    numcards = stackobj.NumCards();
 
    /* Only cards facing up */   
    if (numDragCards <= numcards - numfacedown)
    {
        const CardStack &mystack = stackobj.GetCardStack();
        /* Don't allow to drag unsuited cards */
        for (i = 0; i < numDragCards - 1; i++)
        {            
            if (mystack[i].LoVal() != mystack[i + 1].LoVal() - 1)
            {
                return false;
            }
            switch (dwDifficulty)
            {
                case IDC_DIF_ONECOLOR:
                    break;
                case IDC_DIF_TWOCOLORS:
                    if (mystack[i].IsBlack() && mystack[i + 1].IsBlack() ||
                        mystack[i].IsRed() && mystack[i + 1].IsRed())
                    {
                       return false;
                    }
                    break;
                case IDC_DIF_FOURCOLORS:
                    if (mystack[i].Suit() != mystack[i + 1].Suit())
                    {
                       return false;
                    }
                    break;
            }
        }
        /* Remember where the cards come from */
        from = &stackobj;
        return true;
    }
    else
    {
        return false;
    }
}

/* Game finished successfully */
void GameFinished()
{
    MessageBox(SpiderWnd, MsgWin, szAppName, MB_OK | MB_ICONINFORMATION);
    if( IDYES == MessageBox(SpiderWnd, MsgDeal, szAppName, MB_YESNO | MB_ICONQUESTION) )
    {
        NewGame();
    }
    else
    {
        SpiderWnd.EmptyStacks();
        fGameStarted = false;
    }
}

/* Card added, check for win situation */
void CARDLIBPROC StackAddProc(CardRegion &stackobj, const CardStack &added)
{
    if (cardsFinished == NUM_SPIDER_CARDS)
    {
        GameFinished();
    }
}

/* Card to be turned from a stack */
void TurnStackCard(CardRegion &stackobj)
{
    int numfacedown;

    stackobj.GetFaceDirection(&numfacedown);
    if (stackobj.NumCards() <= numfacedown)
    {
        if (numfacedown > 0) numfacedown--;
        stackobj.SetFaceDirection(CS_FACE_DOWNUP, numfacedown);
        stackobj.Redraw();
    }
}

/* Cards dropped to a stack */
bool CARDLIBPROC StackDropProc(CardRegion &stackobj, CardStack &dragcards)
{
    Card dragcard = dragcards[dragcards.NumCards() - 1];
    int faceup, facedown;

    /* Only drop our cards on other stacks */
    if (stackobj.Id() == from->Id())
    {
        return false;
    }
    
    /* If stack is empty, everything can be dropped */
    if (stackobj.NumCards() != 0)
    {
        const CardStack &mystack = stackobj.GetCardStack();

        /* Can only drop if card is 1 less */
        if (mystack[0].LoVal() != dragcard.LoVal() + 1)
        {
            return false;
        }

        /* Can only drop if card matches colour rule */
        switch (dwDifficulty)
        {
            case IDC_DIF_ONECOLOR:
                break;
            case IDC_DIF_TWOCOLORS:
                if (mystack[0].IsBlack() && dragcard.IsRed() ||
                    mystack[0].IsRed() && dragcard.IsBlack())
                {
                   return false;
                }
                break;
            case IDC_DIF_FOURCOLORS:
                if (mystack[0].Suit() != dragcard.Suit())
                {
                   return false;
                }
                break;
        }
        /* Check if stack complete */
        stackobj.GetFaceDirection(&facedown);
        faceup = stackobj.NumCards() - facedown;
        
        if (faceup + dragcards.NumCards() >= NUM_ONECOLOR_CARDS)
        {            
            int i, max = NUM_ONECOLOR_CARDS - dragcards.NumCards() - 1;
            BOOL remove = true;
            
            /* Dragged cards have been checked to be in order, start with the first stack card */
            for (i = 0; i < max; i++)
            {
                if (mystack[i].Suit() != mystack[i + 1].Suit() ||
                    mystack[i].LoVal() != mystack[i + 1].LoVal() - 1)
                {
                    remove = false;
                    break;
                }
            }            
            if (remove)
            {
                CardStack s = stackobj.GetCardStack();
                CardStack f;
                
                /* Remove from card stack */
                for (i = 0; i < max + 1; i++)
                {
                    s.RemoveCard(0);
                }
                /* Remove dragged cards */
                dragcards = f;
                stackobj.SetCardStack(s);
                cardsFinished += NUM_ONECOLOR_CARDS;
                /* Flip top card of the dest stack */
                TurnStackCard(stackobj);
            }
        }
    }
    /* Flip the top card of the source stack */
    TurnStackCard(*from);
    fGameStarted = true;
    return true;
}

/* Create card regions */
void CreateSpider()
{
    int i, pos;

    /* Compute the value for yRowStackCardOffset based on the height of the card, so the card number isn't hidden on larger cards */
    yRowStackCardOffset = (int)(__cardheight / 6.7);

    pDeck = SpiderWnd.CreateRegion(0, true, 0, 0, -15, 0);
    pDeck->SetFaceDirection(CS_FACE_DOWN, 0);
    pDeck->SetEmptyImage(CS_EI_CIRC);
    pDeck->SetPlacement(CS_XJUST_RIGHT, CS_YJUST_BOTTOM, - X_BORDER, - Y_BORDER);
    pDeck->SetDragRule(CS_DRAG_NONE, 0);
    pDeck->SetDropRule(CS_DROP_NONE, 0);
    pDeck->SetClickProc(DeckClickProc);
    pDeck->SetDblClickProc(DeckClickProc);

    /* Create the row stacks */
    for (i = 0; i < NUM_STACKS; i++)
    {
        pStack[i] = SpiderWnd.CreateRegion(NUM_STACKS+i, true, 0, Y_BORDER, 0, yRowStackCardOffset);
        pStack[i]->SetFaceDirection(CS_FACE_DOWN, 0);
        pos = i - NUM_STACKS/2;
        pStack[i]->SetPlacement(CS_XJUST_CENTER, 0,
            pos * (__cardwidth + X_BORDER) + 6 * X_BORDER, 0);
        pStack[i]->SetEmptyImage(CS_EI_SUNK);
        pStack[i]->SetDragRule(CS_DRAG_CALLBACK, StackDragProc);
        pStack[i]->SetDropRule(CS_DROP_CALLBACK, StackDropProc);
        pStack[i]->SetAddCardProc(StackAddProc);
    }
}

