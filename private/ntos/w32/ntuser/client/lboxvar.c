/**************************** Module Header ********************************\
* Module Name: lboxvar.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* List Box variable height owner draw routines
*
* History:
* ??-???-???? ianja    Ported from Win 3.0 sources
* 14-Feb-1991 mikeke   Added Revalidation code (None)
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* LBGetVariableHeightItemHeight
*
* Returns the height of the given item number. Assumes variable
* height owner draw.
*
* History:
\***************************************************************************/

INT LBGetVariableHeightItemHeight(
    PLBIV plb,
    INT itemNumber)
{
    BYTE itemHeight;
    int offsetHeight;

    if (plb->cMac) {
        if (plb->fHasStrings)
            offsetHeight = plb->cMac * sizeof(LBItem);
        else
            offsetHeight = plb->cMac * sizeof(LBODItem);

        if (plb->wMultiple)
            offsetHeight += plb->cMac;

        offsetHeight += itemNumber;

        itemHeight = *(plb->rgpch+(UINT)offsetHeight);

        return (INT)itemHeight;

    }

    /*
     *Default, we return the height of the system font.  This is so we can draw
     * the focus rect even though there are no items in the listbox.
     */
    return gpsi->cySysFontChar;
}


/***************************************************************************\
* LBSetVariableHeightItemHeight
*
* Sets the height of the given item number. Assumes variable height
* owner draw, a valid item number and valid height.
*
*
* History:
\***************************************************************************/

void LBSetVariableHeightItemHeight(
    PLBIV plb,
    INT itemNumber,
    INT itemHeight)
{
    int offsetHeight;

    if (plb->fHasStrings)
        offsetHeight = plb->cMac * sizeof(LBItem);
    else
        offsetHeight = plb->cMac * sizeof(LBODItem);

    if (plb->wMultiple)
        offsetHeight += plb->cMac;

    offsetHeight += itemNumber;

    *(plb->rgpch + (UINT)offsetHeight) = (BYTE)itemHeight;

}


/***************************************************************************\
* CItemInWindowVarOwnerDraw
*
* Returns the number of items which can fit in a variable height OWNERDRAW
* list box. If fDirection, then we return the number of items which
* fit starting at sTop and going forward (for page down), otherwise, we are
* going backwards (for page up). (Assumes var height ownerdraw) If fPartial,
* then include the partially visible item at the bottom of the listbox.
*
* History:
\***************************************************************************/

INT CItemInWindowVarOwnerDraw(
    PLBIV plb,
    BOOL fPartial)
{
    RECT rect;
    INT sItem;
    INT clientbottom;

    _GetClientRect(plb->spwnd, (LPRECT)&rect);
    clientbottom = rect.bottom;

    /*
     * Find the number of var height ownerdraw items which are visible starting
     * from plb->iTop.
     */
    for (sItem = plb->iTop; sItem < plb->cMac; sItem++) {

        /*
         * Find out if the item is visible or not
         */
        if (!LBGetItemRect(plb, sItem, (LPRECT)&rect)) {

            /*
             * This is the first item which is completely invisible, so return
             * how many items are visible.
             */
            return (sItem - plb->iTop);
        }

        if (!fPartial && rect.bottom > clientbottom) {

            /*
             * If we only want fully visible items, then if this item is
             * visible, we check if the bottom of the item is below the client
             * rect, so we return how many are fully visible.
             */
            return (sItem - plb->iTop - 1);
        }
    }

    /*
     * All the items are visible
     */
    return (plb->cMac - plb->iTop);
}


/***************************************************************************\
* LBPage
*
* For variable height ownerdraw listboxes, calaculates the new iTop we must
* move to when paging (page up/down) through variable height listboxes.
*
* History:
\***************************************************************************/

INT LBPage(
    PLBIV plb,
    INT startItem,
    BOOL fPageForwardDirection)
{
    INT     i;
    INT height;
    RECT    rc;

    if (plb->cMac == 1)
        return(0);

    _GetClientRect(plb->spwnd, &rc);
    height = rc.bottom;
    i = startItem;

    if (fPageForwardDirection) {
        while ((height >= 0) && (i < plb->cMac))
            height -= LBGetVariableHeightItemHeight(plb, i++);

        return((height >= 0) ? plb->cMac - 1 : max(i - 2, startItem + 1));
    } else {
        while ((height >= 0) && (i >= 0))
            height -= LBGetVariableHeightItemHeight(plb, i--);

        return((height >= 0) ? 0 : min(i + 2, startItem - 1));
    }

}


/***************************************************************************\
* LBCalcVarITopScrollAmt
*
* Changing the top most item in the listbox from iTopOld to iTopNew we
* want to calculate the number of pixels to scroll so that we minimize the
* number of items we will redraw.
*
* History:
\***************************************************************************/

INT LBCalcVarITopScrollAmt(
    PLBIV plb,
    INT iTopOld,
    INT iTopNew)
{
    RECT rc;
    RECT rcClient;

    _GetClientRect(plb->spwnd, (LPRECT)&rcClient);

    /*
     * Just optimize redrawing when move +/- 1 item.  We will redraw all items
     * if moving more than 1 item ahead or back.  This is good enough for now.
     */
    if (iTopOld + 1 == iTopNew) {

        /*
         * We are scrolling the current iTop up off the top off the listbox so
         * return a negative number.
         */
        LBGetItemRect(plb, iTopOld, (LPRECT)&rc);
        return (rcClient.top - rc.bottom);
    }

    if (iTopOld - 1 == iTopNew) {

        /*
         * We are scrolling the current iTop down and the previous item is
         * becoming the new iTop so return a positive number.
         */
        LBGetItemRect(plb, iTopNew, (LPRECT)&rc);
        return -rc.top;
    }

    return rcClient.bottom - rcClient.top;
}
