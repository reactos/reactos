#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include "resource.h"
#include "cardlib/cardlib.h"

#include "solitaire.h"

const int yBorder = 20;
const int xBorder = 20;
const int yRowStacks = yBorder + 128;

CardRegion *pDeck;
CardRegion *pPile;
CardRegion *pSuitStack[4];
CardRegion *pRowStack[NUM_ROW_STACKS];

extern CardStack activepile;

HBITMAP hbmBitmap;
HDC        hdcBitmap;

void CreateSol()
{
    int i;

//    hbmBitmap = (HBITMAP)LoadImage(0,"test.bmp",IMAGE_BITMAP,0,0,LR_LOADFROMFILE);
//    SolWnd.SetBackImage(hbmBitmap);

    activepile.Clear();


    pDeck = SolWnd.CreateRegion(DECK_ID, true, xBorder, yBorder, 2, 1);
    pDeck->SetEmptyImage(CS_EI_SUNK);
    pDeck->SetThreedCount(6);
    pDeck->SetDragRule(CS_DRAG_NONE, 0);
    pDeck->SetDropRule(CS_DROP_NONE, 0);
    pDeck->SetClickProc(DeckClickProc);
    pDeck->SetDblClickProc(DeckClickProc);
    pDeck->SetFaceDirection(CS_FACE_DOWN, 0);

    pPile = SolWnd.CreateRegion(PILE_ID, true, 110, yBorder, CS_DEFXOFF, 1);
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
        pSuitStack[i] = SolWnd.CreateRegion(SUIT_ID+i, true, 0, yBorder, 0, 0);
        pSuitStack[i]->SetEmptyImage(CS_EI_SUNK);
        //pSuitStack[i]->SetPlacement(CS_XJUST_RIGHT, 0, -i * (__cardwidth + 4) - xBorder, 0);
        pSuitStack[i]->SetPlacement(CS_XJUST_CENTER, 0, i * (__cardwidth + 10) , 0);

        pSuitStack[i]->SetDropRule(CS_DROP_CALLBACK, SuitStackDropProc);
        pSuitStack[i]->SetDragRule(CS_DRAG_TOP);

        pSuitStack[i]->SetAddCardProc(SuitStackAddProc);
    }

    //
    //    Create the row stacks
    //
    for(i = 0; i < NUM_ROW_STACKS; i++)
    {
        pRowStack[i] = SolWnd.CreateRegion(ROW_ID+i, true, 0, yRowStacks, 0, 14);
        pRowStack[i]->SetEmptyImage(CS_EI_SUNK);
        pRowStack[i]->SetFaceDirection(CS_FACE_DOWNUP, i);
        
        pRowStack[i]->SetPlacement(CS_XJUST_CENTER, 0, 
            (i - NUM_ROW_STACKS/2) * (__cardwidth + 10),     0);

        pRowStack[i]->SetEmptyImage(CS_EI_NONE);

        pRowStack[i]->SetDragRule(CS_DRAG_CALLBACK, RowStackDragProc);
        pRowStack[i]->SetDropRule(CS_DROP_CALLBACK, RowStackDropProc);
        pRowStack[i]->SetClickProc(RowStackClickProc);
        pRowStack[i]->SetDblClickProc(RowStackDblClickProc);
    }
}
