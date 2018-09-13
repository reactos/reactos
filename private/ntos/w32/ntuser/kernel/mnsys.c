/**************************** Module Header ********************************\
* Module Name: mnsys.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* System Menu Routines
*
* History:
*  10-10-90 JimA    Cleanup.
*  03-18-91 IanJa   Window revalidation added (none required)
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

void _SetCloseDefault(PMENU pSubMenu);
PWND FindFakeMDIChild(PWND pwndParent);

/***************************************************************************\
* LoadSysDesktopMenu
*
* Loads and locks a desktop system menu. Since we have to call the client
*  to load the menu, while thread 1 is loading the menu, thread 2
*  might grab the critical section, check pdesk->spmenu* and decide that
*  the menu needs to be loaded. Hence we could load the menu more than once.
*  this function handles that case to avoid leaking menus.
*
* 10/24/97 Gerardob     Created
\***************************************************************************/
PMENU xxxLoadSysDesktopMenu (PMENU * ppmenu, UINT uMenuId)
{
    PMENU pmenu;
    /*
     * This should only be called when the menu hasn't been loaded
     */
    UserAssert(*ppmenu == NULL);

    pmenu = xxxLoadSysMenu(uMenuId);
    if (pmenu == NULL) {
        return NULL;
    }
    /*
     * If someone beat us loading the menu, destroy this one
     *  and return the one already loaded
     */
    if (*ppmenu != NULL) {
        UserAssert(TestMF(*ppmenu, MFSYSMENU));
        RIPMSG1(RIP_WARNING,
                "LoadSysDesktopMenu: Menu loaded during callback. ppmenu:%#p",
                ppmenu);
        _DestroyMenu(pmenu);
        return *ppmenu;
    }
    /*
     * Mark it, lock it and done
     */
    SetMF(pmenu, MFSYSMENU);
    LockDesktopMenu(ppmenu, pmenu);
    return pmenu;
}
/***************************************************************************\
* Lock/UnlockDesktopMenu
*
* These functions lock/unlock a pmenu into a desktop structure (spmenuSys or
*  spmenuDialogSys) and mark/clear it as such.
* We mark these menus so we can identify them quickly on single bit test.
* We also don't want any one to modify these menus or any submenu.
*
* Note that this assumes that there is only one submenu. If more are added,
*  these functions have to be fixed accordingly.
*
* 08/18/97 Gerardob     Created
\***************************************************************************/
PVOID LockDesktopMenu(PMENU * ppmenu, PMENU pmenu)
{
    PMENU pSubMenu;
    PTHREADINFO ptiDesktop;
    /*
     * We only load desktop sys menus once.
     */
    UserAssert(*ppmenu == NULL);

    if (pmenu == NULL) {
        return NULL;
    }

    SetMF(pmenu, MFDESKTOP);
    /*
     * This is awful but this is the real owner of this object. We used to set it
     *  to NULL but that was forcing us to handle the NULL owner all over the place
     */
    ptiDesktop = PtiCurrent()->rpdesk->rpwinstaParent->pTerm->ptiDesktop;
    HMChangeOwnerProcess(pmenu, ptiDesktop);

    pSubMenu = pmenu->rgItems->spSubMenu;
    UserAssert(pSubMenu != NULL);

    SetMF(pSubMenu, MFDESKTOP);
    HMChangeOwnerProcess(pSubMenu, ptiDesktop);

#if DBG
    {
        /*
         * Assert that there are no other submenus that would need to be
         *  marked as MFDESKTOP.
         */
        PITEM pitem;
        UINT uItems;

        UserAssert(pmenu->cItems == 1);

        pitem = pSubMenu->rgItems;
        uItems = pSubMenu->cItems;
        while (uItems--) {
            UserAssert(pitem->spSubMenu == NULL);
            pitem++;
        }
    }
#endif

    return Lock(ppmenu, pmenu);
}

PVOID UnlockDesktopMenu(PMENU * ppmenu)
{
    UserAssert(*ppmenu != NULL);
    UserAssert(TestMF(*ppmenu, MFDESKTOP));
    ClearMF(*ppmenu, MFDESKTOP);
    UserAssert(TestMF((*ppmenu)->rgItems->spSubMenu, MFDESKTOP));
    ClearMF((*ppmenu)->rgItems->spSubMenu, MFDESKTOP);
    return Unlock(ppmenu);
}
/***************************************************************************\
* GetSysMenuHandle
*
* Returns a handle to the system menu of the given window. NULL if
* the window doesn't have a system menu.
*
* History:
\***************************************************************************/

PMENU xxxGetSysMenuHandle(
    PWND pwnd)
{
    PMENU pMenu;

    CheckLock(pwnd);

    if (TestWF(pwnd, WFSYSMENU)) {
        pMenu = pwnd->spmenuSys;

        /*
         * If the window doesn't have a System Menu, use the default one.
         */
        if (pMenu == NULL) {

            /*
             * Grab the menu from the desktop.  If the desktop menu
             * has not been loaded and this is not a system thread,
             * load it now.  Callbacks cannot be made from a system
             * thread or when a thread is in cleanup.
             */
            pMenu = pwnd->head.rpdesk->spmenuSys;

            /*
             * Do not do callbacks if the thread is exiting.  We ran into this when
             * destroying a thread's window and the window it was promoting to
             * foreground was a hard error popup.
             */
            if (pMenu == NULL && !(PtiCurrent()->TIF_flags & (TIF_SYSTEMTHREAD | TIF_INCLEANUP))) {

                pMenu = xxxLoadSysDesktopMenu (&pwnd->head.rpdesk->spmenuSys, ID_SYSMENU);
            }
        }
    } else {
        pMenu = NULL;
    }

    return pMenu;
}

/***************************************************************************\
*
*  GetSysMenu()
*
*  Sets up the system menu first, then returns it.
*
\***************************************************************************/
PMENU xxxGetSysMenu(PWND pwnd, BOOL fSubMenu)
{
    PMENU   pMenu;

    CheckLock(pwnd);
    xxxSetSysMenu(pwnd);
    if ((pMenu = xxxGetSysMenuHandle(pwnd)) != NULL) {
        if (fSubMenu)
            pMenu = _GetSubMenu(pMenu, 0);
    }

    return(pMenu);
}

/***************************************************************************\
* IsSmallerThanScreen
*
\***************************************************************************/

BOOL IsSmallerThanScreen(PWND pwnd)
{
    int dxMax, dyMax;
    PMONITOR pMonitor;

    pMonitor = _MonitorFromWindow(pwnd, MONITOR_DEFAULTTOPRIMARY);
    dxMax = pMonitor->rcWork.right - pMonitor->rcWork.left;
    dyMax = pMonitor->rcWork.bottom - pMonitor->rcWork.top;

    if ((pwnd->rcWindow.right - pwnd->rcWindow.left < dxMax) ||
            (pwnd->rcWindow.bottom - pwnd->rcWindow.top < dyMax)) {
        return TRUE;
    }
    return FALSE;
}

/***************************************************************************\
* SetSysMenu
*
* !
*
* History:
\***************************************************************************/

void xxxSetSysMenu(
    PWND pwnd)
{
    PMENU pMenu;
    UINT wSize;
    UINT wMinimize;
    UINT wMaximize;
    UINT wMove;
    UINT wRestore;
    UINT wDefault;
    BOOL fFramedDialogBox;
    TL tlmenu;

    CheckLock(pwnd);
    /*
     * Get the handle of the current system menu.
     */
    if ((pMenu = xxxGetSysMenuHandle(pwnd)) != NULL) {

        pMenu = _GetSubMenu(pMenu, 0);
        if (!pMenu)
            return;

        ThreadLockAlways(pMenu, &tlmenu);

        /*
         * System modal window: no size, icon, zoom, or move.
         */

// No system modal windows on NT.
//        wSize = wMaximize = wMinimize = wMove =
//            (UINT)((_GetSysModalWindow() == NULL) || hTaskLockInput ? 0: MFS_GRAYED);
        wSize = wMaximize = wMinimize = wMove =  0;
        wRestore = MFS_GRAYED;

        //
        // Default menu command is close.
        //
        wDefault = SC_CLOSE;

        /*
         * Minimized exceptions: no minimize, restore.
         */

        // we need to reverse these because VB has a "special" window
        // that is both minimized but without a minbox.
        if (TestWF(pwnd, WFMINIMIZED))
        {
            wRestore  = 0;
            wMinimize = MFS_GRAYED;
            wSize     = MFS_GRAYED;
            wDefault  = SC_RESTORE;

            if (IsTrayWindow(pwnd))
              wMove = MFS_GRAYED;
        }
        else if (!TestWF(pwnd, WFMINBOX))
            wMinimize = MFS_GRAYED;

        /*
         * Maximized exceptions: no maximize, restore.
         */
        if (!TestWF(pwnd, WFMAXBOX))
            wMaximize = MFS_GRAYED;
        else if (TestWF(pwnd, WFMAXIMIZED)) {
            wRestore = 0;

            /*
             * If the window is maximized but it isn't larger than the
             * screen, we allow the user to move the window around the
             * desktop (but we don't allow resizing).
             */
            wMove = MFS_GRAYED;
            if (!TestWF(pwnd, WFCHILD)) {
                if (IsSmallerThanScreen(pwnd)) {
                    wMove = 0;
                }
            }

            wSize     = MFS_GRAYED;
            wMaximize = MFS_GRAYED;
        }

        if (!TestWF(pwnd, WFSIZEBOX))
            wSize = MFS_GRAYED;

        /*
         * Are we dealing with a framed dialog box with a sys menu?
         * Dialogs with min/max/size boxes get a regular system menu
         *  (as opposed to the dialog menu)
         */
        fFramedDialogBox =
                (((TestWF(pwnd, WFBORDERMASK) == (BYTE)LOBYTE(WFDLGFRAME))
                        || (TestWF(pwnd, WEFDLGMODALFRAME)))
                    && !TestWF(pwnd, WFSIZEBOX | WFMINBOX | WFMAXBOX));

        if (!fFramedDialogBox) {
            xxxEnableMenuItem(pMenu, (UINT)SC_SIZE, wSize);
            if (!TestWF(pwnd, WEFTOOLWINDOW))
            {
                xxxEnableMenuItem(pMenu, (UINT)SC_MINIMIZE, wMinimize);
                xxxEnableMenuItem(pMenu, (UINT)SC_MAXIMIZE, wMaximize);
                xxxEnableMenuItem(pMenu, (UINT)SC_RESTORE, wRestore);
            }
        }

        xxxEnableMenuItem(pMenu, (UINT)SC_MOVE, wMove);

#if DBG
        /*
         * Assert that nobody managed to change the desktop menus.
         */
        if (TestMF(pMenu, MFSYSMENU)) {
            PITEM pItem = MNLookUpItem(pMenu, SC_CLOSE, FALSE, NULL);
            UserAssert((pItem != NULL) && !TestMFS(pItem, MFS_GRAYED));
        }
#endif

        if (wDefault == SC_CLOSE)
            _SetCloseDefault(pMenu);
        else
            _SetMenuDefaultItem(pMenu, wDefault, MF_BYCOMMAND);

        ThreadUnlock(&tlmenu);
    }
}


/***************************************************************************\
* GetSystemMenu
*
* !
*
* History:
\***************************************************************************/

PMENU xxxGetSystemMenu(
    PWND pwnd,
    BOOL fRevert)
{
    PMENU pmenu;
    CheckLock(pwnd);

    /*
     * Should we start with a fresh copy?
     */

    pmenu = pwnd->spmenuSys;
    if (fRevert) {

        /*
         * Destroy the old system menu.
         */
        if ((pmenu != NULL) && !TestMF(pmenu, MFSYSMENU)) {

            if (UnlockWndMenu(pwnd, &pwnd->spmenuSys)) {
                _DestroyMenu(pmenu);
            }
        }
    } else {

        /*
         * Do we need to load a new system menu?
         */
        if (((pmenu == NULL) || TestMF(pmenu, MFSYSMENU))
                && TestWF(pwnd, WFSYSMENU)) {

            PPOPUPMENU pGlobalPopupMenu;
            UINT uMenuId = (pwnd->spmenuSys == NULL ? ID_SYSMENU : ID_DIALOGSYSMENU);
            pmenu = xxxLoadSysMenu(uMenuId);
            if (pmenu == NULL) {
                RIPMSG1(RIP_WARNING, "_GetSystemMenu: xxxLoadSysMenu Failed. pwnd:%#p", pwnd);
            }
            LockWndMenu(pwnd, &pwnd->spmenuSys, pmenu);

            pmenu = pwnd->spmenuSys;
            pGlobalPopupMenu = GetpGlobalPopupMenu(pwnd);
            if ((pGlobalPopupMenu != NULL)
                    && !pGlobalPopupMenu->fIsTrackPopup
                    && (pGlobalPopupMenu->spwndPopupMenu == pwnd)) {

                UserAssert(pGlobalPopupMenu->spwndNotify == pwnd);
                if (pGlobalPopupMenu->fIsSysMenu) {
                    Lock(&pGlobalPopupMenu->spmenu, pmenu);
                } else {
                    Lock(&pGlobalPopupMenu->spmenuAlternate, pmenu);
                }
            }
        }
    }

    /*
     * Return the handle to the system menu.
     */
    if (pwnd->spmenuSys != NULL) {
        /*
         * The app is probably going to modify this menu and then we'll need to
         *  redraw the caption buttons. Hence we need to store the window pointer
         *  in this pmenu or we won't be able to know what window to repaint.
         * The bogus thing is that we cannot call LockWndMenu here because this is
         *  not the actual pmenuSys.
         */
        pmenu = _GetSubMenu(pwnd->spmenuSys, 0);
        if (pmenu) {
            SetMF(pmenu, MFAPPSYSMENU);
            Lock(&pmenu->spwndNotify, pwnd);
        }
        return pmenu;
    }

    return NULL;
}

/***************************************************************************\
* MenuItemState
*
* Sets the menu item flags identified by wMask to the states identified
* by wFlags.
*
* History:
* 10-11-90 JimA       Translated from ASM
\***************************************************************************/

DWORD MenuItemState(
    PMENU pMenu,
    UINT wCmd,
    DWORD wFlags,
    DWORD wMask,
    PMENU *ppMenu)
{
    PITEM pItem;
    DWORD wRet;

    /*
     * Get a pointer the the menu item
     */
    if ((pItem = MNLookUpItem(pMenu, wCmd, (BOOL) (wFlags & MF_BYPOSITION), ppMenu)) == NULL)
        return (DWORD)-1;

    /*
     * Return previous state
     */
    wRet = pItem->fState & wMask;

    /*
     * Set new state
     */
    pItem->fState ^= ((wRet ^ wFlags) & wMask);

    return wRet;
}


/***************************************************************************\
* EnableMenuItem
*
* Enable, disable or gray a menu item.
*
* History:
* 10-11-90 JimA       Translated from ASM
\***************************************************************************/

DWORD xxxEnableMenuItem(
    PMENU pMenu,
    UINT wIDEnableItem,
    UINT wEnable)
{
    DWORD dres;
    PMENU pRealMenu;
    PPOPUPMENU ppopup;

    CheckLock(pMenu);

    dres = MenuItemState(pMenu, wIDEnableItem, wEnable,
            MFS_GRAYED, &pRealMenu);

    /*
     * If enabling/disabling a system menu item, redraw the caption buttons
     */
    if (TestMF(pMenu, MFAPPSYSMENU) && (pMenu->spwndNotify != NULL)) {

        TL tlpwnd;

        switch (wIDEnableItem) {
        case SC_SIZE:
        case SC_MOVE:
        case SC_MINIMIZE:
        case SC_MAXIMIZE:
        case SC_CLOSE:
        case SC_RESTORE:
            ThreadLock(pMenu->spwndNotify, &tlpwnd);
            xxxRedrawTitle(pMenu->spwndNotify, DC_BUTTONS);
            ThreadUnlock(&tlpwnd);
        }
    }

    /* 367162: If the menu is already being displayed we need to redraw it */
    if(pRealMenu && (ppopup = MNGetPopupFromMenu(pRealMenu, NULL))){
        xxxMNUpdateShownMenu(ppopup, NULL, MNUS_DEFAULT);
    }

    return dres;
}


/***************************************************************************\
* CheckMenuItem (API)
*
* Check or un-check a popup menu item.
*
* History:
* 10-11-90 JimA       Translated from ASM
\***************************************************************************/

DWORD _CheckMenuItem(
    PMENU pMenu,
    UINT wIDCheckItem,
    UINT wCheck)
{
    return MenuItemState(pMenu, wIDCheckItem, wCheck, (UINT)MF_CHECKED, NULL);
}


/***************************************************************************\
*
*  SetMenuDefaultItem() -
*
*  Sets the default item in the menu, by command or by position based on the
*  fByPosition flag.
*  We unset all the other items as the default, then set the given one.
*
*  The return value is TRUE if the given item was set as default, FALSE
*  if not.
*
\***************************************************************************/
BOOL _SetMenuDefaultItem(PMENU pMenu, UINT wID, BOOL fByPosition)
{
    UINT  iItem;
    UINT  cItems;
    PITEM pItem;
    PITEM pItemFound;
    PMENU   pMenuFound;

    //
    // We need to check if wId actually exists on this menu.  0xFFFF means
    // clear all default items.
    //

    if (wID != MFMWFP_NOITEM)
    {
        pItemFound = MNLookUpItem(pMenu, wID, fByPosition, &pMenuFound);

        // item must be on same menu and can't be a separator
        if ((pItemFound == NULL) || (pMenuFound != pMenu) || TestMFT(pItemFound, MFT_SEPARATOR))
            return(FALSE);

    }
    else
        pItemFound = NULL;

    pItem = pMenu->rgItems;
    cItems = pMenu->cItems;

    // Walk the menu list, clearing MFS_DEFAULT from all other items, and
    // setting MFS_DEFAULT on the requested one.
    for (iItem = 0; iItem < cItems; iItem++, pItem++) {
        //
        // Note we don't change the state of lpItemFound if it exists.  This
        // is so that below, where we try to set the default, we can tell
        // if we need to recalculate the underline.
        //

        if (TestMFS(pItem, MFS_DEFAULT) && (pItem != pItemFound))
        {
            //
            // We are changing the default item.  As such, it will be drawn
            // with a different font than the one used to calculate it, if
            // the menu has already been drawn once.  We need to ensure
            // that the underline gets drawn in the right place the next
            // time the menu comes up.  Cause it to recalculate.
            //
            // We do NOT do this if the item
            //      (a) isn't default--otherwise we'll recalculate the
            //  underline for every system menu item every time we go into
            //  menu mode because sysmenu init will call SetMenuDefaultItem.
            //      (b) isn't the item we're going to set as the default.
            //  That way we don't recalculate the underline when the item
            //  isn't changing state.
            //
            ClearMFS(pItem, MFS_DEFAULT);
            pItem->ulX = UNDERLINE_RECALC;
            pItem->ulWidth = 0;
        }
    }

    if (wID != MFMWFP_NOITEM)
    {
        if (!TestMFS(pItemFound, MFS_DEFAULT))
        {
            //
            // We are changing from non-default to default.  Clear out
            // the underline info.  If the menu has never painted, this
            // won't do anything.  But it matters a lot if it has.
            //
            SetMFS(pItemFound, MFS_DEFAULT);
            pItemFound->ulX = UNDERLINE_RECALC;
            pItemFound->ulWidth = 0;
        }
    }

    return(TRUE);
}

// --------------------------------------------------------------------------
//
//  SetCloseDefault()
//
//  Tries to find a close item in the first level of menu items.  Looks
//  for SC_CLOSE, then a couple other IDs.  We'd rather not do lstrstri's
//  for "Close", which is slow.
//
// --------------------------------------------------------------------------
void _SetCloseDefault(PMENU pSubMenu)
{
    if (!_SetMenuDefaultItem(pSubMenu, SC_CLOSE, MF_BYCOMMAND))
    {
        //
        // Bloody hell.  Let's try a couple other values.
        //      * Project   --  0x7000 less
        //      * FoxPro    --  0xC070
        //
        if (!_SetMenuDefaultItem(pSubMenu, SC_CLOSE - 0x7000, MF_BYCOMMAND))
            _SetMenuDefaultItem(pSubMenu, 0xC070, MF_BYCOMMAND);
    }
}


// --------------------------------------------------------------------------
//
//  FindFakeMDIChild()
//
//  Attempts to find first child visible child window in the zorder that
//  has a system menu or is maxed.  We can't check for an exact system
//  menu match because several apps make their own copy of the sys menu.
//
// --------------------------------------------------------------------------
PWND FindFakeMDIChild(PWND pwnd)
{
    PWND    pwndReturn;

    // Skip invisible windows and their descendants
    if (!TestWF(pwnd, WFVISIBLE))
        return(NULL);

    // Did we hit pay dirt?
    if (TestWF(pwnd, WFCHILD) && (TestWF(pwnd, WFMAXIMIZED) || (pwnd->spmenuSys)))
        return(pwnd);

    // Check our children
    for (pwnd = pwnd->spwndChild; pwnd; pwnd = pwnd->spwndNext)
    {
        pwndReturn = FindFakeMDIChild(pwnd);
        if (pwndReturn)
            return(pwndReturn);
    }

    return(NULL);
}



// --------------------------------------------------------------------------
//
//  SetupFakeMDIAppStuff()
//
//  For apps that dork around with their own MDI (Excel, Word, Project,
//      Quattro Pro), we want to make them a little more Chicago friendly.
//      Namely we:
//
//      (1) Set the default menu item to SC_CLOSE if there isn't one (this
//          won't help FoxPro, but they do so much wrong stuff it doesn't
//          really matter).
//          That way double-clicks will still work.
//
//      (2) Get the right small icon.
//
//  The way we do this is to go find the child window of the menu bar parent
//  who has a system menu that is this one.
//
//  If the system menu is the standard one, then we can't do (2).
//
// --------------------------------------------------------------------------
void SetupFakeMDIAppStuff(PMENU lpMenu, PITEM lpItem)
{
    PMENU   pSubMenu;
    PWND    pwndParent;
    PWND    pwndChild;

    if (!(pSubMenu = lpItem->spSubMenu))
        return;

    pwndParent = lpMenu->spwndNotify;

    //
    // Set up the default menu item.  Project and FoxPro renumber their
    // IDs so we do some special stuff for them, among others.
    //
    if (!TestWF(pwndParent, WFWIN40COMPAT))
    {
        if (_GetMenuDefaultItem(pSubMenu, TRUE, GMDI_USEDISABLED) == -1L)
            _SetCloseDefault(pSubMenu);
    }

    //
    // Don't touch the HIWORD if we don't find an HWND.  That way apps
    // like Excel which have starting-up maxed children can benefit a little.
    // The first time the menu bar is redrawn, the child isn't visible/
    // around (they add the item too early).  But if it redraws later, or
    // you max a child, the icon will kick in.
    //
    if (pwndChild = FindFakeMDIChild(pwndParent)) {
        lpItem->dwItemData = (ULONG_PTR)HWq(pwndChild);
//        lpItem->dwTypeData = MAKELONG(LOWORD(lpItem->dwTypeData), HW16(hwndChild));
    }
}
