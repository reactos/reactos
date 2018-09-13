/**************************** Module Header ********************************\
* Module Name: lboxmult.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Multi column list box routines
*
* History:
* ??-???-???? ianja    Ported from Win 3.0 sources
* 14-Feb-1991 mikeke   Added Revalidation code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* LBCalcItemRowsAndColumns
*
* Calculates the number of columns (including partially visible)
* in the listbox and calculates the number of items per column
*
* History:
\***************************************************************************/

void LBCalcItemRowsAndColumns(
    PLBIV plb)
{
    RECT rc;

    _GetClientRect(plb->spwnd, &rc);

    //
    // B#4155
    // We need to check if plb->cyChar has been initialized.  This is because
    // we remove WS_BORDER from old listboxes and add on WS_EX_CLIENTEDGE.
    // Since listboxes are always inflated by CXBORDER and CYBORDER, a
    // listbox that was created empty always ends up 2 x 2.  Since this isn't
    // big enough to fit the entire client border, we don't mark it as
    // present.  Thus the client isn't empty in VER40, although it was in
    // VER31 and before.  It is possible to get to this spot without
    // plb->cyChar having been initialized yet if the listbox  is
    // multicolumn && ownerdraw variable.
    //

    if (rc.bottom && rc.right && plb->cyChar) {

        /*
         * Only make these calculations if the width & height are positive
         */
        plb->itemsPerColumn = (INT)max(rc.bottom / plb->cyChar, 1);
        plb->numberOfColumns = (INT)max(rc.right / plb->cxColumn, 1);

        plb->cItemFullMax = plb->itemsPerColumn * plb->numberOfColumns;

        /*
         * Adjust iTop so it's at the top of a column
         */
        xxxNewITop(plb, plb->iTop);
    }
}


/***************************************************************************\
* xxxLBoxCtlHScrollMultiColumn
*
* Supports horizontal scrolling of multicolumn listboxes
*
* History:
\***************************************************************************/

void xxxLBoxCtlHScrollMultiColumn(
    PLBIV plb,
    INT cmd,
    INT xAmt)
{
    INT iTop = plb->iTop;

    CheckLock(plb->spwnd);

    if (!plb->cMac)  return;

    switch (cmd) {
    case SB_LINEUP:
        if (plb->fRightAlign)
            goto ReallyLineDown;
ReallyLineUp:
        iTop -= plb->itemsPerColumn;
        break;
    case SB_LINEDOWN:
        if (plb->fRightAlign)
            goto ReallyLineUp;
ReallyLineDown:
        iTop += plb->itemsPerColumn;
        break;
    case SB_PAGEUP:
        if (plb->fRightAlign)
            goto ReallyPageDown;
ReallyPageUp:
        iTop -= plb->itemsPerColumn * plb->numberOfColumns;
        break;
    case SB_PAGEDOWN:
        if (plb->fRightAlign)
            goto ReallyPageUp;
ReallyPageDown:
        iTop += plb->itemsPerColumn * plb->numberOfColumns;
        break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
        if (plb->fRightAlign) {
            int  iCols = plb->cMac ? ((plb->cMac-1) / plb->itemsPerColumn) + 1 : 0;

            xAmt = iCols - (xAmt + plb->numberOfColumns);
            if (xAmt<0)
                xAmt=0;
        }
        iTop = xAmt * plb->itemsPerColumn;
        break;
    case SB_TOP:
        if (plb->fRightAlign)
            goto ReallyBottom;
ReallyTop:
        iTop = 0;
        break;
    case SB_BOTTOM:
        if (plb->fRightAlign)
            goto ReallyTop;
ReallyBottom:
        iTop = plb->cMac - 1 - ((plb->cMac - 1) % plb->itemsPerColumn);
        break;
    case SB_ENDSCROLL:
        plb->fSmoothScroll = TRUE;
        xxxLBShowHideScrollBars(plb);
        break;
    }

    xxxNewITop(plb, iTop);
}
