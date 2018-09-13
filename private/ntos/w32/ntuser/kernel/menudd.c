/****************************** Module Header ******************************\
* Module Name: menudd.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Menu drag and drop - kernel
*
* History:
* 10/29/96  GerardoB    Created
\***************************************************************************/
#include "precomp.h"
#pragma hdrstop

#include "callback.h"
/*
 * xxxClient* are callbacks from the kernel to call/load OLE functions
 * The other functions in this file are client calls into the kernel
 */
/**************************************************************************\
* xxxClientLoadOLE
*
* 11/06/96 GerardoB     Created
\**************************************************************************/
NTSTATUS xxxClientLoadOLE (void)
{
    NTSTATUS Status;
    PPROCESSINFO ppiCurrent = PpiCurrent();

    if (ppiCurrent->W32PF_Flags & W32PF_OLELOADED) {
        return STATUS_SUCCESS;
    }

    Status = xxxUserModeCallback(FI_CLIENTLOADOLE, NULL, 0, NULL, 0);
    if (NT_SUCCESS(Status)) {
        ppiCurrent->W32PF_Flags |= W32PF_OLELOADED;
    }
    return Status;
}
/**************************************************************************\
* xxxClientRegisterDragDrop
*
* 10/28/96 GerardoB     Created
\**************************************************************************/
NTSTATUS xxxClientRegisterDragDrop (HWND hwnd)
{
    return xxxUserModeCallback(FI_CLIENTREGISTERDRAGDROP, &hwnd, sizeof(&hwnd), NULL, 0);
}
/**************************************************************************\
* xxxClientRevokeDragDrop
*
* 10/28/96 GerardoB     Created
\**************************************************************************/
NTSTATUS xxxClientRevokeDragDrop (HWND hwnd)
{
    return xxxUserModeCallback(FI_CLIENTREVOKEDRAGDROP, &hwnd, sizeof(&hwnd), NULL, 0);

}
/**************************************************************************\
* xxxMNSetGapState
*
* 11/15/96 GerardoB     Created
\**************************************************************************/
void xxxMNSetGapState (ULONG_PTR uHitArea, UINT uIndex, UINT uFlags, BOOL fSet)
{
    int yTop;
    PITEM pItem, pItemGap;
    PPOPUPMENU ppopup;
    RECT rc;
    TL tlHitArea;

    /*
     * Bail if there is nothing to do.
     */
    if (!(uFlags & MNGOF_GAP) || !IsMFMWFPWindow(uHitArea)) {
        return;
    }

    ppopup = ((PMENUWND)uHitArea)->ppopupmenu;
    pItem = MNGetpItem(ppopup, uIndex);

    /*
     * The menu window might be destroyed by now so pItem could be NULL.
     */
    if (pItem == NULL) {
        return;
    }

    /*
     * Mark the item and set the rectangle we need to redraw.
     * Drawing/erasing the insertion bar unhilites/hiltes the
     *  item, so pItem needs to be redrawn completely. In additon,
     *  we need to draw the insertion bar in the next/previous item.
     */
    rc.left = pItem->xItem;
    rc.right = pItem->xItem + pItem->cxItem;
    rc.top = pItem->yItem;
    rc.bottom = pItem->yItem + pItem->cyItem;

    if (uFlags & MNGOF_TOPGAP) {
        pItemGap = MNGetpItem(ppopup, uIndex - 1);
        if (fSet) {
            SetMFS(pItem, MFS_TOPGAPDROP);
            if (pItemGap != NULL) {
                SetMFS(pItemGap, MFS_BOTTOMGAPDROP);
            }
        } else {
            ClearMFS(pItem, MFS_TOPGAPDROP);
            if (pItemGap != NULL) {
                ClearMFS(pItemGap, MFS_BOTTOMGAPDROP);
            }
        }
        if (pItemGap != NULL) {
            rc.top -= SYSMET(CYDRAG);
        }
    } else {
        pItemGap = MNGetpItem(ppopup, uIndex + 1);
        if (fSet) {
            SetMFS(pItem, MFS_BOTTOMGAPDROP);
            if (pItemGap != NULL) {
                SetMFS(pItemGap, MFS_TOPGAPDROP);
            }
        } else {
            ClearMFS(pItem, MFS_BOTTOMGAPDROP);
            if (pItemGap != NULL) {
                ClearMFS(pItemGap, MFS_TOPGAPDROP);
            }
        }
        if (pItemGap != NULL) {
            rc.bottom += SYSMET(CYDRAG);
        }
    }

    /*
     * Adjust to "menu" coordinates (for scrollable menus)
     */
    yTop = MNGetToppItem(ppopup->spmenu)->yItem;
    rc.top -= yTop;
    rc.bottom -= yTop;

    /*
     * Invalidate this rect to repaint it later
     */
    ThreadLockAlways((PWND)uHitArea, &tlHitArea);
    xxxInvalidateRect((PWND)uHitArea, &rc, TRUE);
    ThreadUnlock(&tlHitArea);
}
/**************************************************************************\
* xxxMNDragOver
*
* Menu windows involved in drag drop are registered as targets. This function
*  is called from the client side IDropTarget functions so the menu code can
*  update the selection given the mouse position
*
* 10/28/96 GerardoB     Created
\**************************************************************************/
BOOL xxxMNDragOver(POINT * ppt, PMNDRAGOVERINFO pmndoi)
{
    BOOL fRet;
    PMENUSTATE pMenuState;
    PWND pwnd;
    PPOPUPMENU ppopup;
    TL tlpwnd;

    /*
     * OLE always calls us in context (proxy/marshall stuff). So the
     *  current thread must be in menu mode
     */
    pMenuState = PtiCurrent()->pMenuState;
    if (pMenuState == NULL) {
        RIPMSG0(RIP_WARNING, "xxxMNDragOver: Not in menu mode");
        return FALSE;
    }

    /*
     * This must be a drag and drop menu
     */
    UserAssert(pMenuState->fDragAndDrop);

    /*
     * We might have not initiated this DoDragDrop so make sure
     *  the internal flag is set.
     */
    pMenuState->fInDoDragDrop = TRUE;

    /*
     * Get a window to call xxxCallHandleMenuMessages
     */
    pwnd = GetMenuStateWindow(pMenuState);
    if (pwnd == NULL) {
        RIPMSG0(RIP_WARNING, "xxxMNDragOver: Failed to get MenuStateWindow");
        return FALSE;
    }

    /*
     * We need this after calling back, so lock it
     */
    LockMenuState(pMenuState);

    /*
     * Update the selection and the dragging info
     * Use WM_NCMOUSEMOVE because the point is in screen coordinates already.
     */
    ThreadLockAlways(pwnd, &tlpwnd);
    xxxCallHandleMenuMessages(pMenuState, pwnd, WM_NCMOUSEMOVE, 0, MAKELONG(ppt->x, ppt->y));
    ThreadUnlock(&tlpwnd);

    /*
     * If we're on a popup, propagate the hit test info
     */
    if (pMenuState->uDraggingHitArea != MFMWFP_OFFMENU) {
        ppopup = ((PMENUWND)pMenuState->uDraggingHitArea)->ppopupmenu;
        pmndoi->hmenu = PtoH(ppopup->spmenu);
        pmndoi->uItemIndex = pMenuState->uDraggingIndex;
        pmndoi->hwndNotify = PtoH(ppopup->spwndNotify);
        pmndoi->dwFlags = pMenuState->uDraggingFlags;
        /*
         * Bottom gap of item N corresponds to N+1 gap
         */
        if (pmndoi->dwFlags & MNGOF_BOTTOMGAP) {
            UserAssert(pmndoi->uItemIndex != MFMWFP_NOITEM);
            (pmndoi->uItemIndex)++;
        }
        fRet = TRUE;
    } else {
        fRet = FALSE;
    }

    xxxUnlockMenuState(pMenuState);
    return fRet;;

}
/**************************************************************************\
* xxxMNDragLeave
*
* 11/15/96 GerardoB     Created
\**************************************************************************/
BOOL xxxMNDragLeave (VOID)
{
    PMENUSTATE pMenuState;

    pMenuState = PtiCurrent()->pMenuState;
    if (pMenuState == NULL) {
        RIPMSG0(RIP_WARNING, "xxxMNDragLeave: Not in menu mode");
        return FALSE;
    }

    LockMenuState(pMenuState);

    /*
     * Clean up any present insertion bar state
     */
    xxxMNSetGapState(pMenuState->uDraggingHitArea,
                  pMenuState->uDraggingIndex,
                  pMenuState->uDraggingFlags,
                  FALSE);

    /*
     * Forget the last dragging area
     */
    UnlockMFMWFPWindow(&pMenuState->uDraggingHitArea);
    pMenuState->uDraggingIndex = MFMWFP_NOITEM;
    pMenuState->uDraggingFlags = 0;


    /*
     * The DoDragDrop loop has left our window.
     */
    pMenuState->fInDoDragDrop = FALSE;

    xxxUnlockMenuState(pMenuState);

    return TRUE;
}
/**************************************************************************\
* xxxMNUpdateDraggingInfo
*
* 10/28/96 GerardoB     Created
\**************************************************************************/
void xxxMNUpdateDraggingInfo (PMENUSTATE pMenuState, ULONG_PTR uHitArea, UINT uIndex)
{
    BOOL fCross;
    int y, iIndexDelta;
    PITEM pItem;
    PPOPUPMENU ppopup;
    TL tlLastHitArea;
    ULONG_PTR uLastHitArea;
    UINT uLastIndex, uLastFlags;

    /*
     * Remember current dragging area so we can detected when
     *  crossing item/gap boundries.
     */
    UserAssert((pMenuState->uDraggingHitArea == 0) || IsMFMWFPWindow(pMenuState->uDraggingHitArea));
    ThreadLock((PWND)pMenuState->uDraggingHitArea, &tlLastHitArea);
    uLastHitArea = pMenuState->uDraggingHitArea;
    uLastIndex = pMenuState->uDraggingIndex;
    uLastFlags = pMenuState->uDraggingFlags & MNGOF_GAP;

    /*
     * Store new dragging area.
     */
    LockMFMWFPWindow(&pMenuState->uDraggingHitArea, uHitArea);
    pMenuState->uDraggingIndex = uIndex;

    /*
     * If we're not on a popup, done.
     */
    if (!IsMFMWFPWindow(pMenuState->uDraggingHitArea)) {
        pMenuState->uDraggingHitArea = MFMWFP_OFFMENU;
        pMenuState->uDraggingIndex = MFMWFP_NOITEM;
        ThreadUnlock(&tlLastHitArea);
        return;
    }

    /*
     * Get the popup and item we're on
     */
    ppopup = ((PMENUWND)pMenuState->uDraggingHitArea)->ppopupmenu;
    pItem = MNGetpItem(ppopup, pMenuState->uDraggingIndex);

    /*
     * Find out if we're on the gap, that is, the "virtual" space
     *  between items. Some apps want to distinguish between a drop
     *  ON the item and a drop BEFORE/AFTER the item; there is no
     *  actual space between items so we define a virtual gap
     *
     */
    pMenuState->uDraggingFlags = 0;
    if (pItem != NULL) {
        /*
         * Map the point to client coordinates and then to "menu"
         *  coordinates (to take care of scrollable menus)
         */
        y = pMenuState->ptMouseLast.y;
        y -= ((PWND)pMenuState->uDraggingHitArea)->rcClient.top;
        y += MNGetToppItem(ppopup->spmenu)->yItem;
#if DBG
        if ((y < (int)pItem->yItem)
                || (y > (int)(pItem->yItem + pItem->cyItem))) {
            RIPMSG4(RIP_ERROR, "xxxMNUpdateDraggingInfo: y Point not in selected item. "
                               "pwnd:%#lx ppopup:%#lx Index:%#lx pItem:%#lx",
                               pMenuState->uDraggingHitArea, ppopup, pMenuState->uDraggingIndex, pItem);
        }
#endif

        /*
         * Top/bottom gap check
         */
        if (y <= (int)(pItem->yItem + SYSMET(CYDRAG))) {
            pMenuState->uDraggingFlags = MNGOF_TOPGAP;
        } else if (y >= (int)(pItem->yItem + pItem->cyItem - SYSMET(CYDRAG))) {
            pMenuState->uDraggingFlags = MNGOF_BOTTOMGAP;
        }
    }

    /*
     * Have we crossed an item/gap boundary?
     * We don't cross a boundary when we move from the bottom
     *  of an item to the top of the next, or, from the top
     *  of an item to the bottom of the previous.
     *  (Item N is on top of and previous to item N+1).
     */
    fCross = (uLastHitArea != pMenuState->uDraggingHitArea);
    if (!fCross) {
        iIndexDelta = (int)pMenuState->uDraggingIndex - (int)uLastIndex;
        switch (iIndexDelta) {
            case 0:
                /*
                 * We're on the same item.
                 */
                fCross = (uLastFlags != pMenuState->uDraggingFlags);
                break;

            case 1:
                /*
                 * We've moved to the next item
                 */
                fCross = !((pMenuState->uDraggingFlags == MNGOF_TOPGAP)
                          && (uLastFlags == MNGOF_BOTTOMGAP));
                break;

            case -1:
                /*
                 * We've moved to the previous item
                 */
                fCross = !((pMenuState->uDraggingFlags == MNGOF_BOTTOMGAP)
                          && (uLastFlags == MNGOF_TOPGAP));
                break;

            default:
                /*
                 * We've skipped more than one item.
                 */
                fCross = TRUE;
        }
    }

    if (fCross) {
        pMenuState->uDraggingFlags |= MNGOF_CROSSBOUNDARY;

        /*
         * Update the insertion bar state.
         */
        xxxMNSetGapState(uLastHitArea, uLastIndex, uLastFlags, FALSE);
        xxxMNSetGapState(pMenuState->uDraggingHitArea,
                      pMenuState->uDraggingIndex,
                      pMenuState->uDraggingFlags,
                      TRUE);
    }

    ThreadUnlock(&tlLastHitArea);
}

