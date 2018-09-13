/**************************** Module Header ********************************\
* Module Name: mnchange.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Change Menu Routine
*
* History:
* 10-10-90 JimA       Cleanup.
* 03-18-91 IanJa      Window revalidation added (none required)
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 * Allocation/deallocation increments.  Make them
 * different to avoid possible thrashing when an item
 * is repeatedly added and removed.
 */
#define CMENUITEMALLOC 8
#define CMENUITEMDEALLOC 10

BOOL xxxSetLPITEMInfo(PMENU pMenu, PITEM pItem, LPMENUITEMINFOW lpmii, PUNICODE_STRING pstr);
typedef BOOL (*MENUAPIFN)(PMENU, UINT, BOOL, LPMENUITEMINFOW);


#if DBG
VOID RelocateMenuLockRecords(
    PITEM pItem,
    int cItem,
    LONG_PTR cbMove)
{
    while (cItem > 0) {
        if (pItem->spSubMenu != NULL) {
            HMRelocateLockRecord(&(pItem->spSubMenu), cbMove);
        }
        pItem++;
        cItem--;
    }
}
#endif

/***************************************************************************\
* UnlockSubMenu
*
* Unlocks the pSubMenu and removes the MENULIST element corresponding to pMenu
*
* History:
*  Nov-20-98    MCostea
\***************************************************************************/
PMENU UnlockSubMenu(
    PMENU pMenu,
    PMENU* ppSubMenu)
{
    PMENULIST* pp;
    PMENULIST pMLFound;

    if (*ppSubMenu == NULL) {
        return NULL;
    }
    /*
     * Remove the item from pMenu's pParentsList
     */
    for (pp = &(*ppSubMenu)->pParentMenus; *pp != NULL; pp = &(*pp)->pNext) {
        if ((*pp)->pMenu == pMenu) {
            pMLFound = *pp;
            *pp = (*pp)->pNext;
            DesktopFree(pMenu->head.rpdesk, pMLFound);
            break;
        }
    }
    return Unlock(ppSubMenu);
}

#define NESTED_MENU_LIMIT 25
/***************************************************************************\
* GetMenuDepth
*
* Returns the menu depth (how many nested submenus this menu has).
* This helps catch loops in the menu hierarchy or deep evil apps.
*
* History:
*  Sept-22-98    MCostea
\***************************************************************************/
CHAR GetMenuDepth(PMENU pMenu, UINT uMaxAllowedDepth)
{
    UINT uItems, uMaxDepth = 0, uSubMenuDepth;
    PITEM pItem;

    /*
     * This will prevent us from getting trapped in loops
     */
    if (uMaxAllowedDepth == 0) {
        return NESTED_MENU_LIMIT;
    }
    pItem = pMenu->rgItems;
    for (uItems = pMenu->cItems; uItems--; pItem++) {
        if (pItem->spSubMenu != NULL) {
            uSubMenuDepth = GetMenuDepth(pItem->spSubMenu, uMaxAllowedDepth-1);
            if (uSubMenuDepth > uMaxDepth) {
                /*
                 * Don't walk the other submenus if a deep branch was found
                 */
                if (uSubMenuDepth >= NESTED_MENU_LIMIT) {
                    return NESTED_MENU_LIMIT;
                }
                uMaxDepth = uSubMenuDepth;
            }
        }
    }
    return uMaxDepth + 1;
}

/***************************************************************************\
* GetMenuAncestors
*
* Returns the maximum number of levels above pMenu in the menu hierarchy.
* Walking the "parent" tree should not be expensive as it is pretty unusual
* for menus to appear in different places in the hierarchy.  The tree is
* usualy a simple linked list.
*
* History:
*  Nov-10-98    MCostea
\***************************************************************************/
CHAR GetMenuAncestors(PMENU pMenu)
{
    PMENULIST pParentMenu;
    CHAR uParentAncestors;
    CHAR retVal = 0;

    for (pParentMenu = pMenu->pParentMenus; pParentMenu; pParentMenu = pParentMenu->pNext) {
        uParentAncestors = GetMenuAncestors(pParentMenu->pMenu);
        if (uParentAncestors > retVal) {
            retVal = uParentAncestors;
        }
    }
    return retVal+1;
}

/**********************************************
*   Global Insert/Append/Set client/server interface
*
*   01-13-94  FritzS  Created
***********************************************/
BOOL xxxSetMenuItemInfo(
    PMENU pMenu,
    UINT wIndex,
    BOOL fByPosition,
    LPMENUITEMINFOW lpmii,
    PUNICODE_STRING pstrItem)
{

    PITEM pItem;

    CheckLock(pMenu);

    pItem = MNLookUpItem(pMenu, wIndex, fByPosition,NULL);
    if (pItem == NULL) {
        /*
         * Word doesn't like not finding SC_TASKLIST -- so it that's what
         * they're looking for, let's pretend we changed it.
         */
        if (!fByPosition && (wIndex == SC_TASKLIST))
            return TRUE;

        /*
         * Item not found.  Return false.
         */
        RIPERR0(ERROR_MENU_ITEM_NOT_FOUND, RIP_WARNING, "ModifyMenu: Menu item not found");
        return FALSE;
    }
    /*
     * we need to treat MFT_RIGHTORDER separately as this is propogated down
     * to the entire menu not just to this item so that we stay in ssync. This
     * is pretty similar to the use of MFT_RIGHTJUST, we actually do the
     * propogation because we need the flag in all sorts of places, not just
     * in MBC_RightJustifyMenu()
     */

    /*
     * See ValidateMENUITEMINFO in client\clmenu.c will add more flags to fMask if it use to be MIIM_TYPE
     * Then fMask will not be any more == MIIM_TYPE.
     */

    if (lpmii->fMask & MIIM_TYPE) {
        BOOL bRtoL = (lpmii->fType & MFT_RIGHTORDER) ? TRUE : FALSE;

        if (bRtoL || TestMF(pMenu, MFRTL)) {
            MakeMenuRtoL(pMenu, bRtoL);
        }
    }
    return xxxSetLPITEMInfo(pMenu, pItem, lpmii, pstrItem);
}

/***************************************************************************\
* xxxSetMenuInfo (API)
*
*
* History:
* 12-Feb-1996 JudeJ     Ported from Memphis
* 23-Jun-1996 GerardoB  Fixed up for 5.0
\***************************************************************************/
BOOL xxxSetMenuInfo(PMENU pMenu, LPCMENUINFO lpmi)
{
    PPOPUPMENU  ppopup;
    BOOL        fRecompute = FALSE;
    BOOL        fRedraw    = FALSE;
    UINT        uFlags     = MNUS_DEFAULT;
    PITEM       pItem;
    UINT        uItems;
    TL          tlSubMenu;

    CheckLock(pMenu);

    if (lpmi->fMask & MIM_STYLE) {
        pMenu->fFlags ^= (pMenu->fFlags ^ lpmi->dwStyle) & MNS_VALID;
        fRecompute = TRUE;
    }

    if (lpmi->fMask & MIM_MAXHEIGHT) {
        pMenu->cyMax = lpmi->cyMax;
        fRecompute = TRUE;
    }

    if (lpmi->fMask & MIM_BACKGROUND) {
        pMenu->hbrBack = lpmi->hbrBack;
        fRedraw = TRUE;
        if (pMenu->dwArrowsOn != MSA_OFF) {
            uFlags |= MNUS_DRAWFRAME;
        }
    }

    if (lpmi->fMask & MIM_HELPID) {
        pMenu->dwContextHelpId = lpmi->dwContextHelpID;
    }

    if (lpmi->fMask & MIM_MENUDATA) {
        pMenu->dwMenuData = lpmi->dwMenuData;
    }

    /*
     * Do we need to set this for all submenus?
     */
    if (lpmi->fMask & MIM_APPLYTOSUBMENUS) {
        pItem = pMenu->rgItems;
        for (uItems = pMenu->cItems; uItems--; pItem++) {
            if (pItem->spSubMenu != NULL) {
                ThreadLock(pItem->spSubMenu, &tlSubMenu);
                xxxSetMenuInfo(pItem->spSubMenu, lpmi);
                ThreadUnlock(&tlSubMenu);
            }
        }
    }


    if (fRecompute) {
        // Set the size of this menu to be 0 so that it gets recomputed with this
        // new item...
        pMenu->cyMenu = pMenu->cxMenu = 0;
    }

    if (fRecompute || fRedraw) {
        if (ppopup = MNGetPopupFromMenu(pMenu, NULL)) {
            // this menu is currently being displayed -- redisplay the menu,
            // recomputing if necessary
            xxxMNUpdateShownMenu(ppopup, NULL, uFlags);
        }
    }

    return TRUE;
}
/***************************************************************************\
* MNDeleteAdjustIndex
*
* History:
* 11/19/96 GerardoB  Created
\***************************************************************************/
void NNDeleteAdjustIndex (UINT * puAdjustIndex, UINT uDelIndex)
{
    if (*puAdjustIndex == uDelIndex) {
        *puAdjustIndex = MFMWFP_NOITEM;
    } else if ((int)*puAdjustIndex > (int)uDelIndex) {
        (*puAdjustIndex)--;
    }
}
/***************************************************************************\
* MNDeleteAdjustIndexes
*
* This function is called when an item on an active menu is about
*  to be deleted. It makes sure that other indexes like posSelectedItem,
*  uButtonDownIndex and uDraggingIndex are adjusted to reflect the change
* It "clears" the index if it is AT the point of deletion or
*  decrements it if it is after the point of deletion
*
* History:
* 01/16/97 GerardoB  Created
\***************************************************************************/
void MNDeleteAdjustIndexes (PMENUSTATE pMenuState, PPOPUPMENU ppopup, UINT uiPos)
{
    /*
     * Adjust the index of the selected item and the dropped popup, if needed.
     */
    NNDeleteAdjustIndex(&ppopup->posSelectedItem, uiPos);
    if (ppopup->fHierarchyDropped) {
        NNDeleteAdjustIndex(&ppopup->posDropped, uiPos);
    }

    /*
     * Adjust uButtonDownIndex and uDraggingIndex if needed
     */
    if (pMenuState->uButtonDownHitArea == (ULONG_PTR)ppopup->spwndPopupMenu) {
        NNDeleteAdjustIndex(&pMenuState->uButtonDownIndex, uiPos);
    }
    if (pMenuState->uDraggingHitArea == (ULONG_PTR)ppopup->spwndPopupMenu) {
        NNDeleteAdjustIndex(&pMenuState->uDraggingIndex, uiPos);
    }
}
/***************************************************************************\
* xxxInsertMenuItem
*
\***************************************************************************/
BOOL xxxInsertMenuItem(
    PMENU pMenu,
    UINT wIndex,
    BOOL fByPosition,
    LPMENUITEMINFOW lpmii,
    PUNICODE_STRING pstrItem)
{
    BOOL            fRet = TRUE;
    PITEM           pItem;
    PMENU           pMenuItemIsOn;
    PMENUSTATE      pMenuState;
    PITEM           pNewItems;
    PPOPUPMENU      ppopup = NULL;
    TL              tlMenu;
    UINT            uiPos;

    CheckLock(pMenu);

// Find out where the item we are inserting should go.
    if (wIndex != MFMWFP_NOITEM) {
        pItem = MNLookUpItem(pMenu, wIndex, fByPosition, &pMenuItemIsOn);

        if (pItem != NULL) {
            pMenu = pMenuItemIsOn;
        } else {
            wIndex = MFMWFP_NOITEM;
        }
    } else {
        pItem = NULL;
    }
    /*
     * keep normal menu items between the MDI system bitmap items
     */
    if (!TestMF(pMenu, MFISPOPUP)
            && (pMenu->cItems != 0)
            && (!(lpmii->fMask & MIIM_BITMAP)
                || (lpmii->hbmpItem > HBMMENU_MBARLAST)
                || (lpmii->hbmpItem == 0)
                    )) {

        UINT wSave, w;
        PITEM  pItemWalk;
        wSave = w = wIndex;

        if (pItem && !fByPosition) {
            w = MNGetpItemIndex(pMenu, pItem);
            w = (UINT)((PBYTE)pItem - (PBYTE)(pMenu->rgItems)) / sizeof(ITEM);
        }

        if (!w) {
            pItemWalk = pMenu->rgItems;
            if ((pItemWalk->hbmp == HBMMENU_SYSTEM)) {
                wIndex = 1;
            }
        } else {
            if (w == MFMWFP_NOITEM) {
                w = pMenu->cItems;
            }

            w--;
            pItemWalk = pMenu->rgItems + w;
            while (w && (pItemWalk->hbmp) && (pItemWalk->hbmp < HBMMENU_MBARLAST)) {
                wIndex = w--;
                pItemWalk--;
            }
        }

        if (wIndex != wSave) {
            pItem = pMenu->rgItems + wIndex;
        }
    }

    // LATER -- we currently realloc every 10 items.  investigate the
    // performance hit/gain we get from this and adjust accordingly.
    if (pMenu->cItems >= pMenu->cAlloced) {
        if (pMenu->rgItems) {
            pNewItems = (PITEM)DesktopAlloc(pMenu->head.rpdesk,
                    (pMenu->cAlloced + CMENUITEMALLOC) * sizeof(ITEM),
                                            DTAG_MENUITEM);
            if (pNewItems) {
                RtlCopyMemory(pNewItems, pMenu->rgItems,
                        pMenu->cAlloced * sizeof(ITEM));
#if DEBUGTAGS
                if (IsDbgTagEnabled(DBGTAG_TrackLocks)) {
                    RelocateMenuLockRecords(pNewItems, pMenu->cItems,
                        ((PBYTE)pNewItems) - (PBYTE)(pMenu->rgItems));
                }
#endif
                DesktopFree(pMenu->head.rpdesk, pMenu->rgItems);
            }
        } else {
            pNewItems = (PITEM)DesktopAlloc(pMenu->head.rpdesk,
                    sizeof(ITEM) * CMENUITEMALLOC, DTAG_MENUITEM);
        }

        if (pNewItems == NULL)
            return(FALSE);

        pMenu->cAlloced += CMENUITEMALLOC;
        pMenu->rgItems = pNewItems;

        /*
         * Now look up the item again since it probably moved when we realloced the
         * memory.
         */
        if (wIndex != MFMWFP_NOITEM)
            pItem = MNLookUpItem(pMenu, wIndex, fByPosition, &pMenuItemIsOn);

    }


    /*
     * If this menu is being displayed right now and we're not appending
     *  an item, then we need to adjust the positions we keep track of.
     * We want to do this before moving the items to accomodate the
     *  new one, in case we need to clear the insertion bar
     */
    if ((pItem != NULL)
        && (ppopup = MNGetPopupFromMenu(pMenu, &pMenuState))) {
        /*
         * This menu is active. Adjust the index the selected
         *  item and the dropped popup, if needed
         */
        uiPos = MNGetpItemIndex(pMenu, pItem);
        if (ppopup->posSelectedItem >= (int)uiPos) {
            ppopup->posSelectedItem++;
        }
        if (ppopup->fHierarchyDropped && (ppopup->posDropped >= (int)uiPos)) {
            ppopup->posDropped++;
        }

        /*
         * Adjust uButtonDownIndex and uDraggingIndex if needed
         */
        if (pMenuState->uButtonDownHitArea == (ULONG_PTR)ppopup->spwndPopupMenu) {
            if ((int)pMenuState->uButtonDownIndex >= (int)uiPos) {
                pMenuState->uButtonDownIndex++;
            }
        }
        if (pMenuState->uDraggingHitArea == (ULONG_PTR)ppopup->spwndPopupMenu) {
            /*
             * Check to see if an item is inserted right on the insertion
             *  bar. If so, clean up any present insertion bar state
             */
            if (((int)pMenuState->uDraggingIndex == (int)uiPos)
                    && (pMenuState->uDraggingFlags & MNGOF_TOPGAP)) {

                xxxMNSetGapState(pMenuState->uDraggingHitArea,
                              pMenuState->uDraggingIndex,
                              pMenuState->uDraggingFlags,
                              FALSE);
            }

            if ((int)pMenuState->uDraggingIndex >= (int)uiPos) {
                pMenuState->uDraggingIndex++;
            }
        }
    }

    pMenu->cItems++;
    if (pItem != NULL) {
        // Move this item up to make room for the one we want to insert.
        RtlMoveMemory(pItem + 1, pItem, (pMenu->cItems - 1) *
                sizeof(ITEM) - ((char *)pItem - (char *)pMenu->rgItems));
#if DEBUGTAGS
        if (IsDbgTagEnabled(DBGTAG_TrackLocks)) {
            RelocateMenuLockRecords(pItem + 1,
                    (int)(&(pMenu->rgItems[pMenu->cItems]) - (pItem + 1)),
                    sizeof(ITEM));
        }
#endif
    } else {

        // If lpItem is null, we will be inserting the item at the end of the
        // menu.
        pItem = pMenu->rgItems + pMenu->cItems - 1;
    }

    // Need to zero these fields in case we are inserting this item in the
    // middle of the item list.
    pItem->fType           = 0;
    pItem->fState          = 0;
    pItem->wID             = 0;
    pItem->spSubMenu       = NULL;
    pItem->hbmpChecked     = NULL;
    pItem->hbmpUnchecked   = NULL;
    pItem->cch             = 0;
    pItem->dwItemData      = 0;
    pItem->xItem           = 0;
    pItem->yItem           = 0;
    pItem->cxItem          = 0;
    pItem->cyItem          = 0;
    pItem->hbmp            = NULL;
    pItem->cxBmp           = MNIS_MEASUREBMP;
    pItem->lpstr           = NULL;

    /*
     * We might have reassigned pMenu above, so lock it
     */
    ThreadLock(pMenu, &tlMenu);
    if (!xxxSetLPITEMInfo(pMenu, pItem, lpmii, pstrItem)) {

        /*
         * Reset any of the indexes we might have adjusted above
         */
        if (ppopup != NULL) {
            MNDeleteAdjustIndexes(pMenuState, ppopup, uiPos);
        }

        MNFreeItem(pMenu, pItem, TRUE);


        // Move things up since we removed/deleted the item
        RtlMoveMemory(pItem, pItem + 1, pMenu->cItems * (UINT)sizeof(ITEM) +
            (UINT)((char *)&pMenu->rgItems[0] - (char *)(pItem + 1)));
#if DEBUGTAGS
        if (IsDbgTagEnabled(DBGTAG_TrackLocks)) {
            RelocateMenuLockRecords(pItem,
                    (int)(&(pMenu->rgItems[pMenu->cItems - 1]) - pItem),
                    -(int)sizeof(ITEM));
        }
#endif
        pMenu->cItems--;
        fRet = FALSE;
    } else {
       /*
        * Like MFT_RIGHTJUSTIFY, this staggers down the menu,
        *    (but we inherit, to make localisation etc MUCH easier).
        *
        * MFT_RIGHTORDER is the same value as MFT_SYSMENU.  We distinguish
        * between the two by also looking for MFT_BITMAP.
        */
        if (TestMF(pMenu, MFRTL) ||
            (pItem && TestMFT(pItem, MFT_RIGHTORDER) && !TestMFT(pItem, MFT_BITMAP))) {
            pItem->fType |= (MFT_RIGHTORDER | MFT_RIGHTJUSTIFY);
            if (pItem->spSubMenu) {
                MakeMenuRtoL(pItem->spSubMenu, TRUE);
            }
        }
    }

    ThreadUnlock(&tlMenu);
    return fRet;

}

/***************************************************************************\
* FreeItemBitmap
*
* History:
*  07-23-96 GerardoB - Added header and Fixed up for 5.0
\***************************************************************************/
void FreeItemBitmap(PITEM pItem)
{
    // Free up hItem unless it's a bitmap handle or nonexistent.
    // Apps are responsible for freeing their bitmaps.
     if ((pItem->hbmp != NULL) && !TestMFS(pItem, MFS_CACHEDBMP)) {
            /*
             * Assign ownership of the bitmap to the process that is
             * destroying the menu to ensure that bitmap will
             * eventually be destroyed.
             */
        GreSetBitmapOwner((HBITMAP)(pItem->hbmp), OBJECT_OWNER_CURRENT);
    }

    // Zap this pointer in case we try to free or reference it again
    pItem->hbmp  = NULL;
}
/***************************************************************************\
* FreeItemString
*
* History:
*  07-23-96 GerardoB - Added header and Fixed up for 5.0
\***************************************************************************/

void FreeItemString(PMENU pMenu, PITEM pItem)
{
    // Free up Item's string
    if ((pItem->lpstr != NULL)) {
        DesktopFree(pMenu->head.rpdesk, pItem->lpstr);
    }
    // Zap this pointer in case we try to free or reference it again
    pItem->lpstr  = NULL;
}

/***************************************************************************\
* FreeItem
*
* Free a menu item and its associated resources.
*
* History:
* 10-11-90 JimA       Translated from ASM
\***************************************************************************/

void MNFreeItem(
    PMENU pMenu,
    PITEM pItem,
    BOOL fFreeItemPopup)
{
    PMENU pSubMenu;

    FreeItemBitmap(pItem);
    FreeItemString(pMenu, pItem);

    pSubMenu = UnlockSubMenu(pMenu, &(pItem->spSubMenu));
    if (pSubMenu) {
        if (fFreeItemPopup) {
            _DestroyMenu(pSubMenu);
        }
    }
}
/***************************************************************************\
* RemoveDeleteMenuHelper
*
* This removes the menu item from the given menu.  If
* fDeleteMenuItem, the memory associted with the popup menu associated with
* the item being removed is freed and recovered.
*
* History:
\***************************************************************************/

BOOL xxxRemoveDeleteMenuHelper(
    PMENU pMenu,
    UINT nPosition,
    DWORD wFlags,
    BOOL fDeleteMenu)
{
    PITEM  pItem;
    PITEM  pNewItems;
    PMENU  pMenuSave;
    PMENUSTATE pMenuState;
    PPOPUPMENU ppopup;
    UINT       uiPos;

    CheckLock(pMenu);

    pMenuSave = pMenu;

    pItem = MNLookUpItem(pMenu, nPosition, (BOOL) (wFlags & MF_BYPOSITION), &pMenu);
    if (pItem == NULL) {

        /*
         * Hack for apps written for Win95. In Win95 the prototype for
         * this function was with 'WORD nPosition' and because of this
         * the HIWORD(nPosition) got set to 0.
         * We are doing this just for system menu commands.
         */
        if (nPosition >= 0xFFFFF000 && !(wFlags & MF_BYPOSITION)) {
            nPosition &= 0x0000FFFF;
            pMenu = pMenuSave;
            pItem = MNLookUpItem(pMenu, nPosition, FALSE, &pMenu);

            if (pItem == NULL)
                return FALSE;
        } else
            return FALSE;
    }

    if (ppopup = MNGetPopupFromMenu(pMenu, &pMenuState)) {
        /*
         * This menu is active; since we're about to insert an item,
         *  make sure that any of the positions we've stored are
         *  adjusted properly
         */
        uiPos = MNGetpItemIndex(pMenu, pItem);
        MNDeleteAdjustIndexes(pMenuState, ppopup, uiPos);
    }
    MNFreeItem(pMenu, pItem, fDeleteMenu);

    /*
     * Reset the menu size so that it gets recomputed next time.
     */
    pMenu->cyMenu = pMenu->cxMenu = 0;

    if (pMenu->cItems == 1) {
        DesktopFree(pMenu->head.rpdesk, pMenu->rgItems);
        pMenu->cAlloced = 0;
        pNewItems = NULL;
    } else {

        /*
         * Move things up since we removed/deleted the item
         */

        RtlMoveMemory(pItem, pItem + 1, pMenu->cItems * (UINT)sizeof(ITEM) +
                (UINT)((char *)&pMenu->rgItems[0] - (char *)(pItem + 1)));
#if DEBUGTAGS
        if (IsDbgTagEnabled(DBGTAG_TrackLocks)) {
            RelocateMenuLockRecords(pItem,
                    (int)(&(pMenu->rgItems[pMenu->cItems - 1]) - pItem),
                    -(int)sizeof(ITEM));
        }
#endif

        /*
         * We're shrinking so if localalloc fails, just leave the mem as is.
         */
        UserAssert(pMenu->cAlloced >= pMenu->cItems);
        if ((pMenu->cAlloced - pMenu->cItems) >= CMENUITEMDEALLOC - 1) {
            pNewItems = (PITEM)DesktopAlloc(pMenu->head.rpdesk,
                    (pMenu->cAlloced - CMENUITEMDEALLOC) * sizeof(ITEM),
                                            DTAG_MENUITEM);
            if (pNewItems != NULL) {

                RtlCopyMemory(pNewItems, pMenu->rgItems,
                        (pMenu->cAlloced - CMENUITEMDEALLOC) * sizeof(ITEM));
#if DEBUGTAGS
                if (IsDbgTagEnabled(DBGTAG_TrackLocks)) {
                    RelocateMenuLockRecords(pNewItems, pMenu->cItems - 1,
                        ((PBYTE)pNewItems) - (PBYTE)(pMenu->rgItems));
                }
#endif
                DesktopFree(pMenu->head.rpdesk, pMenu->rgItems);
                pMenu->cAlloced -= CMENUITEMDEALLOC;
            } else
                pNewItems = pMenu->rgItems;
        } else
            pNewItems = pMenu->rgItems;
    }

    pMenu->rgItems = pNewItems;
    pMenu->cItems--;

    if (ppopup != NULL) {
        /*
         * this menu is currently being displayed -- redisplay the menu with
         * this item removed
         */
        xxxMNUpdateShownMenu(ppopup, pMenu->rgItems + uiPos, MNUS_DELETE);
    }
    return TRUE;
}

/***************************************************************************\
* RemoveMenu
*
* Removes and item but doesn't delete it. Only useful for items with
* an associated popup since this will remove the item from the menu with
* destroying the popup menu handle.
*
* History:
\***************************************************************************/

BOOL xxxRemoveMenu(
    PMENU pMenu,
    UINT nPosition,
    UINT wFlags)
{
    return xxxRemoveDeleteMenuHelper(pMenu, nPosition, wFlags, FALSE);
}

/***************************************************************************\
* DeleteMenu
*
* Deletes an item. ie. Removes it and recovers the memory used by it.
*
* History:
\***************************************************************************/

BOOL xxxDeleteMenu(
    PMENU pMenu,
    UINT nPosition,
    UINT wFlags)
{
    return xxxRemoveDeleteMenuHelper(pMenu, nPosition, wFlags, TRUE);
}

/***************************************************************************\
* xxxSetLPITEMInfo
*
* History:
*  07-23-96 GerardoB - Added header and Fixed up for 5.0
\***************************************************************************/
BOOL NEAR xxxSetLPITEMInfo(
    PMENU pMenu,
    PITEM pItem,
    LPMENUITEMINFOW lpmii,
    PUNICODE_STRING pstrItem)
{

    HANDLE hstr;
    UINT cch;
    BOOL fRecompute = FALSE;
    BOOL fRedraw = FALSE;
    PPOPUPMENU ppopup;

    CheckLock(pMenu);

    if (lpmii->fMask & MIIM_FTYPE) {
        pItem->fType &= ~MFT_MASK;
        pItem->fType |= lpmii->fType;
        if (lpmii->fType & MFT_SEPARATOR ) {
            pItem->fState |= MFS_DISABLED ;
        }
        fRecompute = TRUE;
        fRedraw = (lpmii->fType & MFT_OWNERDRAW);
    }

    if (lpmii->fMask & MIIM_STRING) {
        if (pstrItem->Buffer != NULL) {
            hstr = (HANDLE)DesktopAlloc(pMenu->head.rpdesk,
                    pstrItem->Length + sizeof(UNICODE_NULL), DTAG_MENUTEXT);

            if (hstr == NULL) {
                return FALSE;
            }

            try {
                RtlCopyMemory(hstr, pstrItem->Buffer, pstrItem->Length);
            } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
                DesktopFree(pMenu->head.rpdesk, hstr);
                return FALSE;
            }
            cch = pstrItem->Length / sizeof(WCHAR);
            /*
             * We don't need to null terminate the string, since DesktopAlloc
             * zero-fills for us.
             */
        } else {
            cch = 0;
            hstr = NULL;
        }
        FreeItemString(pMenu,pItem);
        pItem->cch = cch;
        pItem->lpstr = hstr;
        fRecompute = TRUE;
        fRedraw = TRUE;
    }

    if (lpmii->fMask & MIIM_BITMAP) {
        FreeItemBitmap(pItem);
        pItem->hbmp = lpmii->hbmpItem;
        fRecompute = TRUE;
        fRedraw = TRUE;
        pItem->cxBmp = MNIS_MEASUREBMP;
        /*
         * If this is one of the special bitmaps, mark it as such
         */
        if ((pItem->hbmp > HBMMENU_MIN) && (pItem->hbmp < HBMMENU_MAX)) {
            SetMFS(pItem, MFS_CACHEDBMP);
        } else {
            ClearMFS(pItem, MFS_CACHEDBMP);
        }
    }

    if (lpmii->fMask & MIIM_ID) {
        pItem->wID = lpmii->wID;
    }

    if (lpmii->fMask & MIIM_DATA) {
        pItem->dwItemData = lpmii->dwItemData;
    }

    if (lpmii->fMask & MIIM_STATE) {
        /*
         * Preserve private bits (~MFS_MASK).
         * Also preserve MFS_HILITE | MFS_DEFAULT if already set; if not set,
         *  let the caller turn them on.
         */
        UserAssert(!(lpmii->fState & ~MFS_MASK));
        pItem->fState &= ~MFS_MASK | MFS_HILITE | MFS_DEFAULT;
        pItem->fState |= lpmii->fState;
        if (pItem->fType & MFT_SEPARATOR)
            pItem->fState |= MFS_DISABLED;
        fRedraw = TRUE;
    }

    if (lpmii->fMask & MIIM_CHECKMARKS) {
        pItem->hbmpChecked     = lpmii->hbmpChecked;
        pItem->hbmpUnchecked   = lpmii->hbmpUnchecked;
        fRedraw = TRUE;
    }

    if (lpmii->fMask & MIIM_SUBMENU) {
        PMENU pSubMenu = NULL;

        if (lpmii->hSubMenu != NULL) {
            pSubMenu = ValidateHmenu(lpmii->hSubMenu);
        }

        // Free the popup associated with this item, if any and if needed.
        if (pItem->spSubMenu != pSubMenu) {
            if (pItem->spSubMenu != NULL) {
                _DestroyMenu(pItem->spSubMenu);
            }
            if (pSubMenu != NULL) {

                BOOL bMenuCreated = FALSE;
                /*
                 * Fix MSTest that sets a submenu to itself by giving it a different handle.
                 * So the loop is broken and we won't fail their call later
                 * MCostea #243374
                 */
                if (pSubMenu == pMenu) {
                    pSubMenu = _CreateMenu();
                    if (!pSubMenu) {
                        return FALSE;
                    }
                    bMenuCreated = TRUE;
                }
                /*
                 * Link the submenu and then check for loops
                 */
                Lock(&(pItem->spSubMenu), pSubMenu);
                SetMF(pItem->spSubMenu, MFISPOPUP);
                /*
                 * We just added a submenu.  Check to see if the menu tree is not
                 * unreasonable deep and there is no loop forming.
                 * This will prevent us from running out of stack
                 * MCostea #226460
                 */
                if (GetMenuDepth(pSubMenu, NESTED_MENU_LIMIT) + GetMenuAncestors(pMenu) >= NESTED_MENU_LIMIT) {
FailInsertion:
                    RIPMSG1(RIP_WARNING, "The menu hierarchy is very deep or has a loop %#p", pMenu);
                    ClearMF(pItem->spSubMenu, MFISPOPUP);
                    Unlock(&(pItem->spSubMenu));
                    if (bMenuCreated) {
                        _DestroyMenu(pSubMenu);
                    }
                    return FALSE;
                }
                /*
                 * Add pMenu to the pSubMenu->pParentMenus list
                 */
                {
                    PMENULIST pMenuList = DesktopAlloc(pMenu->head.rpdesk,
                                            sizeof(MENULIST),
                                            DTAG_MENUITEM);
                    if (!pMenuList) {
                        goto FailInsertion;
                    }
                    pMenuList->pMenu = pMenu;
                    pMenuList->pNext = pSubMenu->pParentMenus;
                    pSubMenu->pParentMenus = pMenuList;
                }
            } else {
                UnlockSubMenu(pMenu, &(pItem->spSubMenu));
            }
            fRedraw = TRUE;
        }
    }

    // For support of the old way of defining a separator i.e. if it is not a string
    // or a bitmap or a ownerdraw, then it must be a separator.
    // This should prpbably be moved to MIIOneWayConvert -jjk
    if (!(pItem->fType & (MFT_OWNERDRAW | MFT_SEPARATOR))
         && (pItem->lpstr == NULL)
         && (pItem->hbmp == NULL)) {

        pItem->fType = MFT_SEPARATOR;
        pItem->fState|=MFS_DISABLED;
    }

    if (fRecompute) {
        pItem->dxTab   = 0;
        pItem->ulX     = UNDERLINE_RECALC;
        pItem->ulWidth = 0;

        // Set the size of this menu to be 0 so that it gets recomputed with this
        // new item...
        pMenu->cyMenu = pMenu->cxMenu = 0;


        if (fRedraw) {
            if (ppopup = MNGetPopupFromMenu(pMenu, NULL)) {
                // this menu is currently being displayed -- redisplay the menu,
                // recomputing if necessary
                xxxMNUpdateShownMenu(ppopup, pItem, MNUS_DEFAULT);
            }
        }

    }

    return TRUE;
}

BOOL _SetMenuContextHelpId(PMENU pMenu, DWORD dwContextHelpId)
{

    // Set the new context help Id;
    pMenu->dwContextHelpId = dwContextHelpId;

    return TRUE;
}

BOOL _SetMenuFlagRtoL(PMENU pMenu)
{

    // This is a right-to-left menu being created;
    SetMF(pMenu, MFRTL);

    return TRUE;
}

/***************************************************************************\
* MNGetPopupFromMenu
*
*  checks to see if the given hMenu is currently being shown in a popup.
*  returns the PPOPUPMENU associated with this hMenu if it is being shown;
*  NULL if the hMenu is not currently being shown
*
* History:
*  07-23-96 GerardoB - Added header & fixed up for 5.0
\***************************************************************************/
PPOPUPMENU MNGetPopupFromMenu(PMENU pMenu, PMENUSTATE *ppMenuState)
{
    PPOPUPMENU  ppopup;
    PMENUSTATE pMenuState;

    /*
     * If this menu doesn't have a notification window, then
     *  it cannot be in menu mode
     */
   if (pMenu->spwndNotify == NULL) {
       return NULL;
   }

   /*
    * If no pMenuState, no menu mode
    */
   pMenuState = GetpMenuState(pMenu->spwndNotify);
   if (pMenuState == NULL) {
       return NULL;
   }

   /*
    * If not in the menu loop, not yet or no longer in menu mode
    */
  if (!pMenuState->fInsideMenuLoop) {
      return NULL;
  }

  /*
   * return pMenuState if requested
   */
  if (ppMenuState != NULL) {
      *ppMenuState = pMenuState;
  }


    /*
     * Starting from the root popup, find the popup associated to this menu
     */
    ppopup = pMenuState->pGlobalPopupMenu;
    while (ppopup != NULL) {
        /*
         * found?
         */
        if (ppopup->spmenu == pMenu) {
            if (ppopup->fIsMenuBar) {
                return NULL;
            }
            /*
             * Since the menu is being modified, let's kill any animation.
             */
            MNAnimate(pMenuState, FALSE);
            return ppopup;
        }
        /*
         * If no more popups, bail
         */
        if (ppopup->spwndNextPopup == NULL) {
            return NULL;
        }

        /*
         * Next popup
         */
        ppopup = ((PMENUWND)ppopup->spwndNextPopup)->ppopupmenu;
    }

    return NULL;
}

/***************************************************************************\
* xxxMNUpdateShownMenu
*
*  updates a given ppopup menu window to reflect the inserting, deleting,
*  or altering of the given lpItem.
*
* History:
*  07-23-96 GerardoB - Added header & fixed up for 5.0
\***************************************************************************/
void xxxMNUpdateShownMenu(PPOPUPMENU ppopup, PITEM pItem, UINT uFlags)
{
    RECT rc;
    PWND pwnd = ppopup->spwndPopupMenu;
    PMENU pMenu = ppopup->spmenu;
    TL tlpwnd;
    TL tlpmenu;

    /*
     * The popup might get destroyed while we callback, so lock this pwnd.
     */
    ThreadLock(pwnd, &tlpwnd);
    ThreadLock(ppopup->spmenu, &tlpmenu);

    _GetClientRect(pwnd, &rc);

    /*
     * If we need to resize menu window
     */
    if (pMenu->cxMenu == 0) {
        RECT rcScroll = rc;
        int cySave = rc.bottom;
        int cxSave = rc.right;
        DWORD dwSize;
        DWORD dwArrowsOnBefore;

        dwArrowsOnBefore = pMenu->dwArrowsOn;
        UserAssert(uFlags != 0);
        dwSize = (DWORD)xxxSendMessage(pwnd, MN_SIZEWINDOW, uFlags, 0L);
        uFlags &= ~MNUS_DRAWFRAME;
        /*
         * If scroll arrows appeared or disappeared,  redraw entire client
         */
        if (dwArrowsOnBefore ^ pMenu->dwArrowsOn) {
            goto InvalidateAll;
        }

        rc.right = LOWORD(dwSize);
        if (pItem != NULL) {
            if (rc.right != cxSave) {
                // width changed, redraw everything
                // BUGBUG -- this could be tuned to just redraw items with
                // submenus and/or accelerator fields
                goto InvalidateAll;
            } else {
                rc.bottom = pMenu->cyMenu;
                if (pMenu->dwArrowsOn != MSA_OFF) {
                    if (rc.bottom <= cySave) {
                        rc.top = pItem->yItem - MNGetToppItem(pMenu)->yItem;
                        goto InvalidateRest;
                    }

                    _GetClientRect(pwnd, &rcScroll);
                }

                rc.top = rcScroll.top = pItem->yItem - MNGetToppItem(pMenu)->yItem;
                if ((rc.top >= 0) && (rc.top < (int)pMenu->cyMenu)) {
                    xxxScrollWindowEx(pwnd, 0, rc.bottom - cySave, &rcScroll, &rc, NULL, NULL, SW_INVALIDATE | SW_ERASE);
                }
            } /* else of if (rc.right != cxSave) */
        } /* if (pItem != NULL) */
    } /* if (pMenu->cxMenu == 0) */

    if (!(uFlags & MNUS_DELETE)) {
        if (pItem != NULL) {
            rc.top = pItem->yItem - MNGetToppItem(pMenu)->yItem;
            rc.bottom = rc.top + pItem->cyItem;
InvalidateRest:
            if ((rc.top >= 0) && (rc.top < (int)pMenu->cyMenu)) {
                xxxInvalidateRect(pwnd, &rc, TRUE);
            }
        } else {
InvalidateAll:
            xxxInvalidateRect(pwnd, NULL, TRUE);
        }
        if (uFlags & MNUS_DRAWFRAME) {
            xxxSetWindowPos(pwnd, NULL, 0, 0, 0, 0,
             SWP_DRAWFRAME | SWP_NOSIZE | SWP_NOZORDER | SWP_NOMOVE
             | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
        }
    }

    ThreadUnlock(&tlpmenu);
    ThreadUnlock(&tlpwnd);
}
