/**************************** Module Header ********************************\
* Module Name: mnstate.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Menu State Routines
*
* History:
*  10-10-90 JimA      Cleanup.
*  03-18-91 IanJa     Windowrevalidation added
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

PMENU xxxGetInitMenuParam(PWND pwndMenu, BOOL * lpfSystem);
/***************************************************************************\
* void PositionSysMenu(pwnd, hSysMenu)
*
*
* History:
* 4-25-91 Mikehar Port for 3.1 merge
\***************************************************************************/

void MNPositionSysMenu(
    PWND pwnd,
    PMENU pmenusys)
{
    RECT rc;
    PITEM pItem;

    if (pmenusys == NULL) {
        RIPERR0(ERROR_INVALID_HANDLE,
                RIP_WARNING,
                "Invalid menu handle pmenusys (NULL) to MNPositionSysMenu");

        return;
    }

    /*
     * Whoever positions the menu becomes the owner
     */
    if (pwnd != pmenusys->spwndNotify) {
        Lock(&pmenusys->spwndNotify, pwnd);
    }

    /*
     * Setup the SysMenu hit rectangle.
     */
    rc.top = rc.left = 0;

    if (TestWF(pwnd, WEFTOOLWINDOW)) {
        rc.right = SYSMET(CXSMSIZE);
        rc.bottom = SYSMET(CYSMSIZE);
    } else {
        rc.right = SYSMET(CXSIZE);
        rc.bottom = SYSMET(CYSIZE);
    }

    if (!TestWF(pwnd, WFMINIMIZED)) {
        int cBorders;

        cBorders = GetWindowBorders(pwnd->style, pwnd->ExStyle, TRUE, FALSE);
        OffsetRect(&rc, cBorders*SYSMET(CXBORDER), cBorders*SYSMET(CYBORDER));
    }

    /*
     * Offset the System popup menu.
     */
    if (!TestMF(pmenusys, MF_POPUP) && (pmenusys->cItems > 0)) {
        pItem = pmenusys->rgItems;
        if (pItem) {
            pItem->yItem = rc.top;
            pItem->xItem = rc.left;
            pItem->cyItem = rc.bottom - rc.top;
            pItem->cxItem = rc.right - rc.left;
        }
    }
    else
        // BOGUS -- MF_POPUP should never be set on a MENU -- only a MENU ITEM
        UserAssert(FALSE);
}

/***************************************************************************\
* MNFlushDestroyedPopups
*
* Walk the ppmDelayedFree list freeing those marked as destroyed.
*
* 05-14-96 GerardoB  Created
\***************************************************************************/
void MNFlushDestroyedPopups (PPOPUPMENU ppopupmenu, BOOL fUnlock)
{
    PPOPUPMENU ppmDestroyed, ppmFree;

    UserAssert(IsRootPopupMenu(ppopupmenu));

    /*
     * Walk ppmDelayedFree
     */
    ppmDestroyed = ppopupmenu;
    while (ppmDestroyed->ppmDelayedFree != NULL) {
        /*
         * If it's marked as destroyed, unlink it and free it
         */
        if (ppmDestroyed->ppmDelayedFree->fDestroyed) {
            ppmFree = ppmDestroyed->ppmDelayedFree;
            ppmDestroyed->ppmDelayedFree = ppmFree->ppmDelayedFree;
            UserAssert(ppmFree != ppmFree->ppopupmenuRoot);
            MNFreePopup(ppmFree);
        } else {
            /*
             * fUnlock is TRUE if the root popup is being destroyed; if
             *  so, reset fDelayedFree and unlink it
             */
            if (fUnlock) {
                /*
                 * This means that the root popup is going away before
                 *  some of the hierarchical popups have been destroyed.
                 * This can happen if someone destroys one of the menu
                 *  windows breaking the spwndNextPopup chain.
                 */
                ppmDestroyed->ppmDelayedFree->fDelayedFree = FALSE;
                /*
                 * Stop here so we can figure how this happened.
                 */
                UserAssert(ppmDestroyed->ppmDelayedFree->fDelayedFree);
                ppmDestroyed->ppmDelayedFree = ppmDestroyed->ppmDelayedFree->ppmDelayedFree;
            } else {
                /*
                 * Not fDestroyed so move to the next one.
                 */
                ppmDestroyed = ppmDestroyed->ppmDelayedFree;
            } /* fUnlock */
        } /* fDestroyed */
    } /* while ppmDelayedFree */

}

/***************************************************************************\
* MNAllocPopup
*
\***************************************************************************/
PPOPUPMENU MNAllocPopup(BOOL fForceAlloc)
{
    PPOPUPMENU ppm;
    if (!fForceAlloc && !TEST_PUDF(PUDF_POPUPINUSE)) {
        SET_PUDF(PUDF_POPUPINUSE);

        ppm = &gpopupMenu;
    } else {
        ppm = (PPOPUPMENU)UserAllocPoolWithQuota(sizeof(POPUPMENU), TAG_POPUPMENU);
    }

    if (ppm) {
        RtlZeroMemory(ppm, sizeof(POPUPMENU));
    }

    return (ppm);
}

/***************************************************************************\
* MNFreePopup
*
\***************************************************************************/
VOID MNFreePopup(
    PPOPUPMENU ppopupmenu)
{

    Validateppopupmenu(ppopupmenu);

    if (IsRootPopupMenu(ppopupmenu)) {
        MNFlushDestroyedPopups (ppopupmenu, TRUE);
    }
#if DBG
    /*
     * If this is not a cached menu and it corresponds to a menu window,
     *  then the reference to the popup must be cleared at this point since
     *  we're going to free this popup
     */
    if (ppopupmenu->spwndPopupMenu != NULL) {
        if (!ppopupmenu->fDesktopMenu && !ppopupmenu->fIsMenuBar) {
            UserAssert(GETFNID(ppopupmenu->spwndPopupMenu) == FNID_MENU);
            UserAssert(((PMENUWND)ppopupmenu->spwndPopupMenu)->ppopupmenu == NULL);
        }
    }
#endif    
    Unlock(&ppopupmenu->spwndPopupMenu);
    /*
     * if spwndNextPopup is not NULL, we're breaking the chain and spwndNext won't
     *  get closed. I won't remove the unlock since it has
     *  always been there.
     */
    UserAssert(ppopupmenu->spwndNextPopup == NULL);
    Unlock(&ppopupmenu->spwndNextPopup);

    Unlock(&ppopupmenu->spwndPrevPopup);
    UnlockPopupMenu(ppopupmenu, &ppopupmenu->spmenu);
    UnlockPopupMenu(ppopupmenu, &ppopupmenu->spmenuAlternate);
    Unlock(&ppopupmenu->spwndNotify);
    Unlock(&ppopupmenu->spwndActivePopup);

#if DBG
   if (!ppopupmenu->fDesktopMenu) {
       ppopupmenu->fFreed = TRUE;
   } else {
       PDESKTOP pdesk = PtiCurrent()->rpdesk;
       UserAssert(pdesk->dwDTFlags & DF_MENUINUSE);
       UserAssert(((PMENUWND)pdesk->spwndMenu)->ppopupmenu == ppopupmenu);
   }
#endif

    if (ppopupmenu->fDesktopMenu) {
        PtiCurrent()->rpdesk->dwDTFlags &= ~DF_MENUINUSE;
        /* The desktop menu window points to this popup so don't leave bogus stuff in it */
        ppopupmenu->ppopupmenuRoot = NULL;
    } else if (ppopupmenu == &gpopupMenu) {
        UserAssert(TEST_PUDF(PUDF_POPUPINUSE));
        CLEAR_PUDF(PUDF_POPUPINUSE);
    } else {
        UserFreePool(ppopupmenu);
    }
}
/***************************************************************************\
* MNEndMenuStateNotify
*
* spwndNotify might have been created by a thread other than the one
*  the menu mode is running on. If this is the case, this function
*  NULLs out pMenuState for the thread that owns spwndNotify.
* It returns TRUE if the menu state owner doesn't own the notification
*  window (multiple threads involved)
*
* 05-21-96 GerardoB Created
\***************************************************************************/
BOOL MNEndMenuStateNotify (PMENUSTATE pMenuState)
{
    PTHREADINFO ptiNotify;

    if (pMenuState->pGlobalPopupMenu->spwndNotify != NULL) {
        ptiNotify = GETPTI(pMenuState->pGlobalPopupMenu->spwndNotify);
        if (ptiNotify != pMenuState->ptiMenuStateOwner) {
            /*
             * Later5.0 GerardoB. xxxMNStartMenuState no longer allows this.
             *  This is dead code that I'll remove eventually
             */
            UserAssert(ptiNotify == pMenuState->ptiMenuStateOwner);

            UserAssert(ptiNotify->pMenuState == pMenuState);
            UserAssert(pMenuState->pmnsPrev == NULL);
            ptiNotify->pMenuState = NULL;
            return TRUE;
        }
    }

    return FALSE;
}
/***************************************************************************\
* xxxMNEndMenuState
*
* This funtion must be called to clean up pMenuState after getting out
*  of menu mode. It must be called by the same thread that initialized
*  pMenuState either manually or by calling xxxMNStartMenuState.
*
* 05-20-96 GerardoB Created
\***************************************************************************/
void xxxMNEndMenuState (BOOL fFreePopup)
{
    PTHREADINFO ptiCurrent = PtiCurrent();
    PMENUSTATE pMenuState;
    pMenuState = ptiCurrent->pMenuState;
    UserAssert(ptiCurrent->pMenuState != NULL);
    UserAssert(ptiCurrent == pMenuState->ptiMenuStateOwner);

    /*
     * If the menu is locked, someone doesn't want it to go just yet.
     */
    if (pMenuState->dwLockCount != 0) {
        RIPMSG1(RIP_WARNING, "xxxMNEndMenuState Locked:%#p", pMenuState);
        return;
    }

    MNEndMenuStateNotify(pMenuState);

    /*
     * pMenuState->pGlobalPopupMenu could be NULL if xxxMNAllocMenuState failed
     */
    if (pMenuState->pGlobalPopupMenu != NULL) {
        if (fFreePopup) {
            UserAssert(pMenuState->pGlobalPopupMenu->fIsMenuBar || pMenuState->pGlobalPopupMenu->fDestroyed);

            MNFreePopup(pMenuState->pGlobalPopupMenu);
        } else {
            /*
             * This means that we're ending the menustate but the popup menu
             *  window is still around. This can happen when called from
             *  xxxDestroyThreadInfo.
             */
            UserAssert(pMenuState->pGlobalPopupMenu->fIsTrackPopup);
            pMenuState->pGlobalPopupMenu->fDelayedFree = FALSE;
        }
    }

    /*
     * Unlock MFMWFP windows.
     */
    UnlockMFMWFPWindow(&pMenuState->uButtonDownHitArea);
    UnlockMFMWFPWindow(&pMenuState->uDraggingHitArea);

    /*
     * Restore the previous state, if any
     */
    ptiCurrent->pMenuState = pMenuState->pmnsPrev;

   /*
    * This (modal) menu mode is off
    */
   if (!pMenuState->fModelessMenu) {
       DecSFWLockCount();
       DBGDecModalMenuCount();
   }

    if (pMenuState->hbmAni != NULL) {
        MNDestroyAnimationBitmap(pMenuState);
    }

    /*
     * Free the menu state
     */
    if (pMenuState == &gMenuState) {
        UserAssert(TEST_PUDF(PUDF_MENUSTATEINUSE));
        CLEAR_PUDF(PUDF_MENUSTATEINUSE);
        GreSetDCOwner(gMenuState.hdcAni, OBJECT_OWNER_PUBLIC);
    } else {
        if (pMenuState->hdcAni != NULL) {
            GreDeleteDC(pMenuState->hdcAni);
        }
        UserFreePool(pMenuState);
    }

    /*
     * If returning to a modeless menu, make sure have activation
     * If returning to a modal menu, make sure we have capture
     */
   if (ptiCurrent->pMenuState != NULL) {
       if (ptiCurrent->pMenuState->fModelessMenu) {
           xxxActivateThisWindow(ptiCurrent->pMenuState->pGlobalPopupMenu->spwndActivePopup,
                                 0, 0);
       } else {
           xxxMNSetCapture(ptiCurrent->pMenuState->pGlobalPopupMenu);
       }
   }

#if DBG
    /*
     * If this thread is not in menu mode anymore, it must not be using
     *  the desktop menu
     */
    if ((ptiCurrent->pMenuState == NULL) && (ptiCurrent->rpdesk->spwndMenu != NULL)) {
        UserAssert(ptiCurrent != GETPTI(ptiCurrent->rpdesk->spwndMenu));
    }
    /*
     * If someone is using the menu window, it must be in menu mode
     */
    if (ptiCurrent->rpdesk->dwDTFlags & DF_MENUINUSE) {
        UserAssert(GetpMenuState(ptiCurrent->rpdesk->spwndMenu) != NULL);
    }
    /*
     * No pti should point to this pMenuState anymore
     * If guModalMenuStateCount is zero, all pMenuState must be NULL or modeless
     */
{
    PLIST_ENTRY pHead, pEntry;
    PTHREADINFO ptiT;

    pHead = &(ptiCurrent->rpdesk->PtiList);
    for (pEntry = pHead->Flink; pEntry != pHead; pEntry = pEntry->Flink) {
       ptiT = CONTAINING_RECORD(pEntry, THREADINFO, PtiLink);
       UserAssert(ptiT->pMenuState != pMenuState);
       if (guModalMenuStateCount == 0) {
           UserAssert(ptiT->pMenuState == NULL || ptiT->pMenuState->fModelessMenu);
       }
   }
}
#endif
}
/***************************************************************************\
* MNCreateAnimationBitmap
*
\***************************************************************************/
BOOL MNCreateAnimationBitmap(PMENUSTATE pMenuState, UINT cx, UINT cy)
{
    HBITMAP hbm = GreCreateCompatibleBitmap(gpDispInfo->hdcScreen, cx, cy);
    if (hbm == NULL) {
        RIPMSG0(RIP_WARNING, "MNSetupAnimationBitmap: Failed to create hbmAni");
        return FALSE;
    }

#if DBG
    if (pMenuState->hdcAni == NULL) {
        RIPMSG0(RIP_WARNING, "MNCreateAnimationBitmap: hdcAni is NULL");
    }
    if (pMenuState->hbmAni != NULL) {
        RIPMSG0(RIP_WARNING, "MNCreateAnimationBitmap: hbmAni already exists");
    }
#endif

    GreSelectBitmap(pMenuState->hdcAni, hbm);
    pMenuState->hbmAni = hbm;
    return TRUE;
}
/***************************************************************************\
* MNDestroyAnimationBitmap
*
\***************************************************************************/
void MNDestroyAnimationBitmap(PMENUSTATE pMenuState)
{
    GreSelectBitmap(pMenuState->hdcAni, GreGetStockObject(PRIV_STOCK_BITMAP));
    UserVerify(GreDeleteObject(pMenuState->hbmAni));
    pMenuState->hbmAni = NULL;
}
/***************************************************************************\
* MNSetupAnimationDC
*
* 9/20/96 GerardoB      Created
\***************************************************************************/
BOOL MNSetupAnimationDC (PMENUSTATE pMenuState)
{
    pMenuState->hdcAni = GreCreateCompatibleDC(gpDispInfo->hdcScreen);
    if (pMenuState->hdcAni == NULL) {
        RIPMSG0(RIP_WARNING, "MNSetupAnimationDC: Failed to create hdcAnimate");
        UserAssert(pMenuState != &gMenuState);
        return FALSE;
    }
    GreSelectFont(pMenuState->hdcAni, ghMenuFont);
    return TRUE;
}
/***************************************************************************\
* xxxUnlockMenuState
*
* 11/24/96 GerardoB      Created
\***************************************************************************/
BOOL xxxUnlockMenuState (PMENUSTATE pMenuState)
{
    UserAssert(pMenuState->dwLockCount != 0);
    (pMenuState->dwLockCount)--;
    if ((pMenuState->dwLockCount == 0) && ExitMenuLoop(pMenuState, pMenuState->pGlobalPopupMenu)) {
        xxxMNEndMenuState(TRUE);
        return TRUE;
    }
    return FALSE;
}
/***************************************************************************\
* xxxMNAllocMenuState
*
* Allocates and initializes a pMenuState
*
* 5-21-96 GerardoB      Created
\***************************************************************************/
PMENUSTATE xxxMNAllocMenuState(PTHREADINFO ptiCurrent, PTHREADINFO ptiNotify, PPOPUPMENU ppopupmenuRoot)
{
    BOOL fAllocate;
    PMENUSTATE pMenuState;

    UserAssert(PtiCurrent() == ptiCurrent);
    UserAssert(ptiCurrent->rpdesk == ptiNotify->rpdesk);

    /*
     * If gMenuState is already taken, allocate one.
     */
    fAllocate = TEST_PUDF(PUDF_MENUSTATEINUSE);
    if (fAllocate) {
        pMenuState = (PMENUSTATE)UserAllocPoolWithQuota(sizeof(MENUSTATE), TAG_MENUSTATE);
        if (pMenuState == NULL) {
            return NULL;
        }
    } else {
        /*
         * Use chache global which already has the animation DC setup
         */
        SET_PUDF(PUDF_MENUSTATEINUSE);
        pMenuState = &gMenuState;
        UserAssert(gMenuState.hdcAni != NULL);
        GreSetDCOwner(gMenuState.hdcAni, OBJECT_OWNER_CURRENT);
    }

    /*
     * Prevent anyone from changing the foreground while this menu is active
     */
    IncSFWLockCount();
    DBGIncModalMenuCount();

    /*
     * Initialize pMenuState.
     * Animation DC stuff is already setup so don't zero init it.
     */
    RtlZeroMemory(pMenuState, sizeof(MENUSTATE) - sizeof(MENUANIDC));
    pMenuState->pGlobalPopupMenu = ppopupmenuRoot;
    pMenuState->ptiMenuStateOwner = ptiCurrent;

    /*
     * Save previous state, if any. Then set new state
     */
    pMenuState->pmnsPrev = ptiCurrent->pMenuState;
    ptiCurrent->pMenuState = pMenuState;

    if (ptiNotify != ptiCurrent) {
        UserAssert(ptiNotify->pMenuState == NULL);
        ptiNotify->pMenuState = pMenuState;
    }

    /*
     * If the menustate was allocated, set up animation stuff.
     * This is done here because in case of failure, MNEndMenuState
     * will find ptiCurrent->pMenuState properly.
     */
    if (fAllocate) {
        RtlZeroMemory((PBYTE)pMenuState + sizeof(MENUSTATE) -
                sizeof(MENUANIDC), sizeof(MENUANIDC));
        if (!MNSetupAnimationDC(pMenuState)) {
            xxxMNEndMenuState(TRUE);
            return NULL;
        }
    }

    return pMenuState;
}
/***************************************************************************\
* xxxMNStartMenuState
*
* This function is called when the menu bar is about to be activated (the
*  app's main menu). It makes sure that the threads involved are not in
*  menu mode already, finds the owner/notification window, initializes
*  pMenuState and sends the WM_ENTERMENULOOP message.
* It successful, it returns a pointer to a pMenuState. If so, the caller
*  must call MNEndMenuState when done.
*
* History:
* 4-25-91 Mikehar       Port for 3.1 merge
* 5-20-96 GerardoB      Renamed and changed (Old name: xxxMNGetPopup)
\***************************************************************************/
PMENUSTATE xxxMNStartMenuState(PWND pwnd, DWORD cmd, LPARAM lParam)
{
    PPOPUPMENU ppopupmenu;
    PTHREADINFO ptiCurrent, ptiNotify;
    PMENUSTATE pMenuState;
    TL tlpwnd;
    PWND pwndT;

    CheckLock(pwnd);

    /*
     * Bail if the current thread is already in menu mode
     */
    ptiCurrent = PtiCurrent();
    if (ptiCurrent->pMenuState != NULL) {
        return NULL;
    }

    /*
     * If pwnd is not owned by ptiCurrent, the _PostMessage call below might
     *  send us in a loop
     */
    UserAssert(ptiCurrent == GETPTI(pwnd));

    /*
     * If this is not a child window, use the active window on its queue
     */
    if (!TestwndChild(pwnd)) {
        pwnd = GETPTI(pwnd)->pq->spwndActive;
    } else {
        /*
         * Search up the parents for a window with a System Menu.
         */
        while (TestwndChild(pwnd)) {
            if (TestWF(pwnd, WFSYSMENU)) {
                break;
            }
            pwnd = pwnd->spwndParent;
        }
    }

    if (pwnd == NULL) {
        return NULL;
    }

    if (!TestwndChild(pwnd) && (pwnd->spmenu != NULL)) {
        goto hasmenu;
    }

    if (!TestWF(pwnd, WFSYSMENU)) {
        return NULL;
    }

hasmenu:

    /*
     * If the owner/notification window was created by another thread,
     *  make sure that it's not in menu mode already
     * This can happen if PtiCurrent() is attached to other threads, one of
     *  which created pwnd.
     */
    ptiNotify = GETPTI(pwnd);
    if (ptiNotify->pMenuState != NULL) {
        return NULL;
    }

    /*
     * If the notification window is owned by another thread,
     *   then the menu loop wouldn't get any keyboard or mouse
     *   messages because we set capture to the notification window.
     * So we pass the WM_SYSCOMMAND to that thread and bail
     */
    if (ptiNotify != ptiCurrent) {
        RIPMSG2(RIP_WARNING, "Passing WM_SYSCOMMAND SC_*MENU from thread %#p to %#p", ptiCurrent, ptiNotify);
        _PostMessage(pwnd, WM_SYSCOMMAND, cmd, lParam);
        return NULL;
    }

    /*
     * Allocate ppoupmenu and pMenuState
     */
    ppopupmenu = MNAllocPopup(FALSE);
    if (ppopupmenu == NULL) {
        return NULL;
    }

    pMenuState = xxxMNAllocMenuState(ptiCurrent, ptiNotify, ppopupmenu);
    if (pMenuState == NULL) {
        MNFreePopup(ppopupmenu);
        return NULL;
    }

    ppopupmenu->fIsMenuBar = TRUE;
    ppopupmenu->fHasMenuBar = TRUE;
    Lock(&(ppopupmenu->spwndNotify), pwnd);
    ppopupmenu->posSelectedItem = MFMWFP_NOITEM;
    Lock(&(ppopupmenu->spwndPopupMenu), pwnd);
    ppopupmenu->ppopupmenuRoot = ppopupmenu;

    pwndT = pwnd;
    while( TestwndChild(pwndT) )
        pwndT = pwndT->spwndParent;

    if (pwndT->spmenu)
        ppopupmenu->fRtoL = TestMF(pwndT->spmenu, MFRTL) ?TRUE:FALSE;
    else {
        //
        // no way to know, no menu, but there is a system menu. Thus arrow
        // keys are really not important. however lets take the next best
        // thing just to be safe
        //
        ppopupmenu->fRtoL = TestWF(pwnd, WEFRTLREADING) ?TRUE :FALSE;
    }
    /*
     * Notify the app we are entering menu mode.  wParam is always 0 since this
     * procedure will only be called for menu bar menus not TrackPopupMenu
     * menus.
     */
    ThreadLockAlways(pwnd, &tlpwnd);
    xxxSendMessage(pwnd, WM_ENTERMENULOOP, 0, 0L);
    ThreadUnlock(&tlpwnd);

    return pMenuState;
}


/***************************************************************************\
* xxxStartMenu
*
* Note that this function calls back many times so we might be forced
*  out of menu mode at any time. We don't want to check this after
*  each callback so we lock what we need and go on. Be careful.
*
* History:
* 4-25-91 Mikehar Port for 3.1 merge
\***************************************************************************/

BOOL xxxMNStartMenu(
    PPOPUPMENU ppopupmenu,
    int mn)
{
    PWND pwndMenu;
    PMENU pMenu;
    PMENUSTATE pMenuState;
    TL tlpwndMenu;
    TL tlpmenu;

    UserAssert(IsRootPopupMenu(ppopupmenu));

    if (ppopupmenu->fDestroyed) {
        return FALSE;
    }

    pwndMenu = ppopupmenu->spwndNotify;
    ThreadLock(pwndMenu, &tlpwndMenu);

    pMenuState = GetpMenuState(pwndMenu);
    if (pMenuState == NULL) {
        RIPMSG0(RIP_ERROR, "xxxMNStartMenu: pMenuState == NULL");
        ThreadUnlock(&tlpwndMenu);
        return FALSE;
    }
    pMenuState->mnFocus = mn;
    pMenuState->fMenuStarted = TRUE;
    pMenuState->fButtonDown =
    pMenuState->fButtonAlwaysDown = ((_GetKeyState(VK_LBUTTON) & 0x8000) != 0);

    xxxMNSetCapture(ppopupmenu);

    xxxSendMessage(pwndMenu, WM_SETCURSOR, (WPARAM)HWq(pwndMenu),
            MAKELONG(MSGF_MENU, 0));

    if (ppopupmenu->fIsMenuBar) {
        BOOL    fSystemMenu;


        pMenu = xxxGetInitMenuParam(pwndMenu, &fSystemMenu);

        if (pMenu == NULL) {
            pMenuState->fMenuStarted = FALSE;
            xxxMNReleaseCapture();
            ThreadUnlock(&tlpwndMenu);
            return(FALSE);
        }

        LockPopupMenu(ppopupmenu, &ppopupmenu->spmenu, pMenu);

        ppopupmenu->fIsSysMenu = (fSystemMenu != 0);
        if (!fSystemMenu) {
            pMenu = xxxGetSysMenu(pwndMenu, FALSE);
            LockPopupMenu(ppopupmenu, &ppopupmenu->spmenuAlternate, pMenu);
        }
    }

    pMenuState->fIsSysMenu = (ppopupmenu->fIsSysMenu != 0);

    if (!ppopupmenu->fNoNotify) {

        if (ppopupmenu->fIsTrackPopup && ppopupmenu->fIsSysMenu) {
            pMenu = xxxGetInitMenuParam(pwndMenu, NULL);
        } else {
            pMenu = ppopupmenu->spmenu;
        }

        xxxSendMessage(pwndMenu, WM_INITMENU, (WPARAM)PtoH(pMenu), 0L);
    }

    if (!ppopupmenu->fIsTrackPopup) {
        if (ppopupmenu->fIsSysMenu) {
            MNPositionSysMenu(pwndMenu, ppopupmenu->spmenu);
        } else if (ppopupmenu->fIsMenuBar) {
            ThreadLock(ppopupmenu->spmenu, &tlpmenu);
            xxxMNRecomputeBarIfNeeded(pwndMenu, ppopupmenu->spmenu);
            ThreadUnlock(&tlpmenu);
            MNPositionSysMenu(pwndMenu, ppopupmenu->spmenuAlternate);
        }
    }

    /*
     * If returning TRUE, set menu style in pMenuState
     */
    if (!ppopupmenu->fDestroyed) {
        if (TestMF(ppopupmenu->spmenu, MNS_MODELESS)) {
            pMenuState->fModelessMenu = TRUE;
        }

        if (TestMF(ppopupmenu->spmenu, MNS_DRAGDROP)) {
            if (NT_SUCCESS(xxxClientLoadOLE())) {
                pMenuState->fDragAndDrop = TRUE;
            }
        }

        if (TestMF(ppopupmenu->spmenu, MNS_AUTODISMISS)) {
            pMenuState->fAutoDismiss = TRUE;
        }

        if (TestMF(ppopupmenu->spmenu, MNS_NOTIFYBYPOS)) {
            pMenuState->fNotifyByPos = TRUE;
        }

    }

    /*
     * Bogus!  We don't always know that this is the system menu.  We
     * will frequently pass on an OBJID_MENU even when you hit Alt+Space.
     *
     * Hence, MNSwitchToAlternate will send a EVENT_SYSTEM_MENUEND for the
     * menu bar and an EVENT_SYSTEM_MENUSTART for the sysmenu when we "switch".
     */
    if (FWINABLE()) {
        xxxWindowEvent(EVENT_SYSTEM_MENUSTART, pwndMenu,
                (ppopupmenu->fIsSysMenu ? OBJID_SYSMENU : (ppopupmenu->fIsMenuBar ? OBJID_MENU : OBJID_WINDOW)),
                INDEXID_CONTAINER, 0);
    }

    ThreadUnlock(&tlpwndMenu);

    return !ppopupmenu->fDestroyed;
}

// --------------------------------------------------------------------------
//
//  GetInitMenuParam()
//
//  Gets the HMENU sent as the wParam of WM_INITMENU, and for menu bars, is
//  the actual menu to be interacted with.
//
// --------------------------------------------------------------------------
PMENU xxxGetInitMenuParam(PWND pwndMenu, BOOL * lpfSystem)
{
    //
    // Find out what menu we should be sending in WM_INITMENU:
    //      If minimized/child/empty menubar, use system menu
    //
    CheckLock(pwndMenu);

    if (TestWF(pwndMenu, WFMINIMIZED) ||
        TestwndChild(pwndMenu) ||
        (pwndMenu->spmenu == NULL) ||
        !pwndMenu->spmenu->cItems)
    {
        if (lpfSystem != NULL)
            *lpfSystem = TRUE;

        return(xxxGetSysMenu(pwndMenu, FALSE));
    }
    else
    {
        if (lpfSystem != NULL)
            *lpfSystem = FALSE;

        return(pwndMenu->spmenu);
    }
}
