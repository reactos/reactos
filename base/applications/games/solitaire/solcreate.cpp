#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include "resource.h"
#include "cardlib/cardlib.h"

#include "solitaire.h"

CardRegion *pDeck;
CardRegion *pPile;
CardRegion *pSuitStack[4];
CardRegion *pRowStack[NUM_ROW_STACKS];

extern CardStack activepile;

HBITMAP hbmBitmap;
HDC     hdcBitmap;
int     yRowStackCardOffset;

void CreateSol()
{
    int i;

//    hbmBitmap = (HBITMAP)LoadImage(0,"test.bmp",IMAGE_BITMAP,0,0,LR_LOADFROMFILE);
//    SolWnd.SetBackImage(hbmBitmap);

    activepile.Clear();

    // Compute the value for yRowStackCardOffset based on the height of the card, so the card number isn't hidden on larger cards
    yRowStackCardOffset = (int)(__cardheight / 6.7);

    pDeck = SolWnd.CreateRegion(DECK_ID, true, X_BORDER, Y_BORDER, 2, 1);
    pDeck->SetEmptyImage(CS_EI_CIRC);
    pDeck->SetThreedCount(6);
    pDeck->SetDragRule(CS_DRAG_NONE, 0);
    pDeck->SetDropRule(CS_DROP_NONE, 0);
    pDeck->SetClickProc(DeckClickProc);
    pDeck->SetDblClickProc(DeckClickProc);
    pDeck->SetFaceDirection(CS_FACE_DOWN, 0);

    pPile = SolWnd.CreateRegion(PILE_ID, true, X_BORDER + __cardwidth + X_PILE_BORDER, Y_BORDER, CS_DEFXOFF, 1);
    pPile->SetEmptyImage(CS_EI_NONE);
    pPile->SetDragRule(CS_DRAG_TOP, 0);
    pPile->SetDropRule(CS_DROP_NONE, 0);
    pPile->SetDblClickProc(PileDblClickProc);
    pPile->SetRemoveCardProc(PileRemoveProc);

    //
    //    Create the suit stacks
    //
    for(i = 0; i < 4; i++)
    {
        pSuitStack[i] = SolWnd.CreateRegion(SUIT_ID+i, true, 0, Y_BORDER, 0, 0);
        pSuitStack[i]->SetEmptyImage(CS_EI_SUNK);
        pSuitStack[i]->SetPlacement(CS_XJUST_CENTER, 0, i * (__cardwidth + X_SUITSTACK_BORDER) , 0);

        pSuitStack[i]->SetDropRule(CS_DROP_CALLBACK, SuitStackDropProc);
        pSuitStack[i]->SetDragRule(CS_DRAG_TOP);

        pSuitStack[i]->SetAddCardProc(SuitStackAddProc);
    }

    //
    //    Create the row stacks
    //
    for(i = 0; i < NUM_ROW_STACKS; i++)
    {
        pRowStack[i] = SolWnd.CreateRegion(ROW_ID+i, true, 0, Y_BORDER + __cardheight + Y_ROWSTACK_BORDER, 0, yRowStackCardOffset);
        pRowStack[i]->SetEmptyImage(CS_EI_SUNK);
        pRowStack[i]->SetFaceDirection(CS_FACE_DOWNUP, i);

        pRowStack[i]->SetPlacement(CS_XJUST_CENTER, 0,
            (i - NUM_ROW_STACKS/2) * (__cardwidth + X_ROWSTACK_BORDER),     0);

        pRowStack[i]->SetEmptyImage(CS_EI_NONE);

        pRowStack[i]->SetDragRule(CS_DRAG_CALLBACK, RowStackDragProc);
        pRowStack[i]->SetDropRule(CS_DROP_CALLBACK, RowStackDropProc);
        pRowStack[i]->SetClickProc(RowStackClickProc);
        pRowStack[i]->SetDblClickProc(RowStackDblClickProc);
    }
}
