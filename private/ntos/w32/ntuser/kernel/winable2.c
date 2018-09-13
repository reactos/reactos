/**************************** Module Header ********************************\
* Module Name: winable2.c
*
* This has the following Active Accesibility API
*     GetGUIThreadInfo
*
* The Win Event Hooks are handled in winable.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* History:
* 08-30-96 IanJa  Ported from Windows '95
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*****************************************************************************\
*
*  GetGUIThreadInfo()
*
*  This gets GUI information out of context.  If you pass in a NULL thread ID,
*  we will get the 'global' information, using the foreground thread.  This
*  is guaranteed to be the real active window, focus window, etc.  Yes, you
*  could do it yourself by calling GetForegroundWindow, getting the thread ID
*  of that window via GetWindowThreadProcessId, then passing the ID into
*  GetGUIThreadInfo().  However, that takes three calls and aside from being
*  a pain, anything could happen in the middle.  So passing in NULL gets
*  you stuff in one call and hence also works right.
*
*  This function returns FALSE if the thread doesn't have a queue or the
*  thread ID is bogus.
*
\*****************************************************************************/
BOOL WINAPI
_GetGUIThreadInfo(PTHREADINFO pti, PGUITHREADINFO pgui)
{
    PQ pq;

    /*
     * Validate threadinfo structure
     */
    if (pgui->cbSize != sizeof(GUITHREADINFO)) {
        RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "GUITHREADINFO.cbSize %d is wrong", pgui->cbSize);
        return FALSE;
    }

    /*
     * Is this a valid initialized GUI thread?
     */
    if (pti != NULL) {
        if ((pq = pti->pq) == NULL) {
            // does this ever happen?
            RIPMSG1(RIP_ERROR, "GetGUIThreadInfo: No queue for pti %lx", pti);
            return FALSE;
        }
    } else {
        /*
         * Use the foreground queue. To get menu state information we must also
         * figure out the right pti.  This matches _GetForegroundWindow() logic.
         */
        if ((pq = gpqForeground) == NULL) {
            // this does sometimes happen...
            RIPMSG0(RIP_WARNING, "GetGUIThreadInfo: No foreground queue");
            return FALSE;
        }

        if (pq->spwndActive && (GETPTI(pq->spwndActive)->pq == pq)) {
            pti = GETPTI(pq->spwndActive);
            if (PtiCurrentShared()->rpdesk != pti->rpdesk) {
                RIPERR0(ERROR_ACCESS_DENIED, RIP_VERBOSE, "Foreground window on different desktop");
                return FALSE;
            }
        }
    }

    UserAssert(pq != NULL);

    /*
     * For C2 security, verify that pq and pti are on the current thread's desktop.
     * We can't directly determine which desktop pq belongs to, but we can at
     * least ensure that any caret info we return is not from another desktop
     */
    if (pq->caret.spwnd &&
            (GETPTI(pq->caret.spwnd)->rpdesk != PtiCurrentShared()->rpdesk)) {
        RIPERR0(ERROR_ACCESS_DENIED, RIP_VERBOSE, "Foreground caret on different desktop");
        return FALSE;
    }
    if (pti && (pti->rpdesk != PtiCurrentShared()->rpdesk)) {
        RIPERR0(ERROR_ACCESS_DENIED, RIP_VERBOSE, "Foreground thread on different desktop");
        return FALSE;
    }

    pgui->flags        = 0;
    pgui->hwndMoveSize = NULL;
    pgui->hwndMenuOwner = NULL;

    /*
     * Get Menu information from the THREADINFO
     */
    if (pti != NULL) {
        if (pti->pmsd && !pti->pmsd->fTrackCancelled && pti->pmsd->spwnd) {
            pgui->flags |= GUI_INMOVESIZE;
            pgui->hwndMoveSize = HWq(pti->pmsd->spwnd);
        }

        if (pti->pMenuState && pti->pMenuState->pGlobalPopupMenu) {
            pgui->flags |= GUI_INMENUMODE;

            if (pti->pMenuState->pGlobalPopupMenu->fHasMenuBar) {
                if (pti->pMenuState->pGlobalPopupMenu->fIsSysMenu) {
                    pgui->flags |= GUI_SYSTEMMENUMODE;
                }
            } else {
                pgui->flags |= GUI_POPUPMENUMODE;
            }

            if (pti->pMenuState->pGlobalPopupMenu->spwndNotify)
                pgui->hwndMenuOwner = HWq(pti->pMenuState->pGlobalPopupMenu->spwndNotify);
        }
    }

    /*
     * Get the rest of the information from the queue
     */
    pgui->hwndActive   = HW(pq->spwndActive);
    pgui->hwndFocus    = HW(pq->spwndFocus);
    pgui->hwndCapture  = HW(pq->spwndCapture);

    pgui->hwndCaret    = NULL;

    if (pq->caret.spwnd) {
        pgui->hwndCaret = HWq(pq->caret.spwnd);

        /*
         * These coords are always relative to the client of hwndCaret
         * of course.
         */
        pgui->rcCaret.left   = pq->caret.x;
        pgui->rcCaret.right  = pgui->rcCaret.left + pq->caret.cx;
        pgui->rcCaret.top    = pq->caret.y;
        pgui->rcCaret.bottom = pgui->rcCaret.top + pq->caret.cy;

        if (pq->caret.iHideLevel == 0)
            pgui->flags |= GUI_CARETBLINKING;
    } else if (pti && (pti->ppi->W32PF_Flags & W32PF_CONSOLEHASFOCUS)) {
        /*
         * The thread is running in the console window with focus. Pull
         * out the info from the console pseudo caret.
         */
        pgui->hwndCaret = pti->rpdesk->cciConsole.hwnd;
        pgui->rcCaret = pti->rpdesk->cciConsole.rc;
    } else {
        SetRectEmpty(&pgui->rcCaret);
    }

    return TRUE;
}


/****************************************************************************\
*
* _GetTitleBarInfo()
*
* Gets info about a window's title bar.  If the window is bogus or
* doesn't have a titlebar, this will fail.
*
\****************************************************************************/
BOOL WINAPI
xxxGetTitleBarInfo(PWND pwnd, PTITLEBARINFO ptbi)
{
    int     cxB;

    CheckLock(pwnd);

    /*
     * Validate titlebarinfo structure
     */
    if (ptbi->cbSize != sizeof(TITLEBARINFO)) {
        RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "TITLEBARINFO.cbSize %d is wrong", ptbi->cbSize);
        return FALSE;
    }

    RtlZeroMemory(&ptbi->rgstate, sizeof(ptbi->rgstate));

    ptbi->rgstate[INDEX_TITLEBAR_SELF] |= STATE_SYSTEM_FOCUSABLE;
    if (TestWF(pwnd, WFBORDERMASK) != LOBYTE(WFCAPTION))
    {
        // No titlebar.
        ptbi->rgstate[INDEX_TITLEBAR_SELF] |= STATE_SYSTEM_INVISIBLE;
        return TRUE;
    }

    if (!TestWF(pwnd, WFMINIMIZED) && !TestWF(pwnd, WFCPRESENT))
    {
        // Off screen (didn't fit)
        ptbi->rgstate[INDEX_TITLEBAR_SELF] |= STATE_SYSTEM_OFFSCREEN;
        SetRectEmpty(&ptbi->rcTitleBar);
        return TRUE;
    }

    /*
     * Get titlebar rect
     */
    ptbi->rcTitleBar = pwnd->rcWindow;
    cxB = GetWindowBorders(pwnd->style, pwnd->ExStyle, TRUE, FALSE);
    InflateRect(&ptbi->rcTitleBar, -cxB * SYSMET(CXBORDER), -cxB * SYSMET(CYBORDER));
    if (TestWF(pwnd, WEFTOOLWINDOW)) {
        ptbi->rcTitleBar.bottom = ptbi->rcTitleBar.top + SYSMET(CYSMCAPTION);
    } else {
        ptbi->rcTitleBar.bottom = ptbi->rcTitleBar.top + SYSMET(CYCAPTION);
    }

    /*
     * Don't include the system menu area!
     */
    if (TestWF(pwnd, WFSYSMENU) && _HasCaptionIcon(pwnd))
        ptbi->rcTitleBar.left += (ptbi->rcTitleBar.bottom - ptbi->rcTitleBar.top - SYSMET(CYBORDER));

    /*
     * Close button
     */
    if (!TestWF(pwnd, WFSYSMENU) && TestWF(pwnd, WFWIN40COMPAT)) {
        ptbi->rgstate[INDEX_TITLEBAR_CLOSEBUTTON] |= STATE_SYSTEM_INVISIBLE;
    } else {
        if (!xxxMNCanClose(pwnd))
            ptbi->rgstate[INDEX_TITLEBAR_CLOSEBUTTON] |= STATE_SYSTEM_UNAVAILABLE;

        if (TestWF(pwnd, WFCLOSEBUTTONDOWN))
            ptbi->rgstate[INDEX_TITLEBAR_CLOSEBUTTON] |= STATE_SYSTEM_PRESSED;
    }


    /*
     * Max button
     */
    if (! TestWF(pwnd, WFSYSMENU) && TestWF(pwnd, WFWIN40COMPAT)) {
        ptbi->rgstate[INDEX_TITLEBAR_MAXBUTTON] |= STATE_SYSTEM_INVISIBLE;
    } else {
        if (! TestWF(pwnd, WFMAXBOX)) {
            if (! TestWF(pwnd, WFMINBOX)) {
                ptbi->rgstate[INDEX_TITLEBAR_MAXBUTTON] |= STATE_SYSTEM_INVISIBLE;
            } else {
                ptbi->rgstate[INDEX_TITLEBAR_MAXBUTTON] |= STATE_SYSTEM_UNAVAILABLE;
            }
        }

        if (TestWF(pwnd, WFZOOMBUTTONDOWN))
            ptbi->rgstate[INDEX_TITLEBAR_MAXBUTTON] |= STATE_SYSTEM_PRESSED;
    }


    /*
     * Min button
     */
    if (! TestWF(pwnd, WFSYSMENU) && TestWF(pwnd, WFWIN40COMPAT)) {
        ptbi->rgstate[INDEX_TITLEBAR_MINBUTTON] |= STATE_SYSTEM_INVISIBLE;
    } else {
        if (! TestWF(pwnd, WFMINBOX)) {
            if (! TestWF(pwnd, WFMAXBOX)) {
                ptbi->rgstate[INDEX_TITLEBAR_MINBUTTON] |= STATE_SYSTEM_INVISIBLE;
            } else {
                ptbi->rgstate[INDEX_TITLEBAR_MINBUTTON] |= STATE_SYSTEM_UNAVAILABLE;
            }
        }

        if (TestWF(pwnd, WFREDUCEBUTTONDOWN))
            ptbi->rgstate[INDEX_TITLEBAR_MINBUTTON] |= STATE_SYSTEM_PRESSED;
    }


    /*
     * Help button
     */
    if (!TestWF(pwnd, WEFCONTEXTHELP) || TestWF(pwnd, WFMINBOX) ||
            TestWF(pwnd, WFMAXBOX)) {
        ptbi->rgstate[INDEX_TITLEBAR_HELPBUTTON] |= STATE_SYSTEM_INVISIBLE;
    } else {
        if (TestWF(pwnd, WFHELPBUTTONDOWN))
            ptbi->rgstate[INDEX_TITLEBAR_HELPBUTTON] |= STATE_SYSTEM_PRESSED;
    }

    // IME button BOGUS!
    ptbi->rgstate[INDEX_TITLEBAR_IMEBUTTON] = STATE_SYSTEM_INVISIBLE;

#if 0
    /*
     * System menu
     */
    if (!TestWF(pwnd, WFSYSMENU) || !_HasCaptionIcon(pwnd))
        ptbi->stateSysMenu |= STATE_SYSTEM_INVISIBLE;
#endif

    return TRUE;
}

/*****************************************************************************\
*
*  _GetScrollBarInfo()
*
*  Gets state & location information about a scrollbar.
*
*  Note we fill in the minimal amount of useful info.  OLEACC is responsible
*  for extrapolation.  I.E., if both the line up and line down buttons are
*  disabled, the whole scrollbar is, and the thumb is invisible.
*
\*****************************************************************************/
BOOL WINAPI
_GetScrollBarInfo(PWND pwnd, LONG idObject, PSCROLLBARINFO psbi)
{
    UINT   wDisable;
    BOOL   fVertical;
    SBCALC SBCalc;

    /*
     * Validate scrollbarinfo structure
     */
    if (psbi->cbSize != sizeof(SCROLLBARINFO)) {
        RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "SCROLLBARINFO.cbSize %d is wrong", psbi->cbSize);
        return FALSE;
    }

    RtlZeroMemory(&psbi->rgstate, sizeof(psbi->rgstate));

    /*
     * Calculate where everything is.
     */
    if (idObject == OBJID_CLIENT) {
        RECT rc;
        wDisable = ((PSBWND)pwnd)->wDisableFlags;
        fVertical = ((PSBWND)pwnd)->fVert;
        GetRect(pwnd, &rc, GRECT_CLIENT | GRECT_CLIENTCOORDS);
        CalcSBStuff2(&SBCalc, &rc, (PSBDATA)&((PSBWND)pwnd)->SBCalc, ((PSBWND)pwnd)->fVert);
    } else {
        /*
         * Is this window scrollbar here?
         */
        if (idObject == OBJID_VSCROLL) {
            fVertical = TRUE;
            if (! TestWF(pwnd, WFVSCROLL)) {
                // No scrollbar.
                psbi->rgstate[INDEX_SCROLLBAR_SELF] |= STATE_SYSTEM_INVISIBLE;
            } else if (! TestWF(pwnd, WFVPRESENT)) {
                // Window too short to display it.
                psbi->rgstate[INDEX_SCROLLBAR_SELF] |= STATE_SYSTEM_OFFSCREEN;
            }
        } else if (idObject == OBJID_HSCROLL) {
            fVertical = FALSE;
            if (! TestWF(pwnd, WFHSCROLL)) {
                // No scrollbar.
                psbi->rgstate[INDEX_SCROLLBAR_SELF] |= STATE_SYSTEM_INVISIBLE;
            } else if (! TestWF(pwnd, WFHPRESENT)) {
                psbi->rgstate[INDEX_SCROLLBAR_SELF] |= STATE_SYSTEM_OFFSCREEN;
            }
        } else {
            RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "invalid idObject %d", idObject);
            return FALSE;
        }

        if (psbi->rgstate[INDEX_SCROLLBAR_SELF] & STATE_SYSTEM_INVISIBLE)
            return TRUE;

        wDisable = GetWndSBDisableFlags(pwnd, fVertical);

        if (!(psbi->rgstate[INDEX_SCROLLBAR_SELF] & STATE_SYSTEM_OFFSCREEN))
            CalcSBStuff(pwnd, &SBCalc, fVertical);
    }

    /*
     * Setup button states.
     */
    if (wDisable & LTUPFLAG) {
        psbi->rgstate[INDEX_SCROLLBAR_UP] |= STATE_SYSTEM_UNAVAILABLE;
        psbi->rgstate[INDEX_SCROLLBAR_UPPAGE] |= STATE_SYSTEM_UNAVAILABLE;
    }

    if (wDisable & RTDNFLAG) {
        psbi->rgstate[INDEX_SCROLLBAR_DOWN] |= STATE_SYSTEM_UNAVAILABLE;
        psbi->rgstate[INDEX_SCROLLBAR_DOWNPAGE] |= STATE_SYSTEM_UNAVAILABLE;
    }

    if ((wDisable & (LTUPFLAG | RTDNFLAG)) == (LTUPFLAG | RTDNFLAG))
        psbi->rgstate[INDEX_SCROLLBAR_SELF] |= STATE_SYSTEM_UNAVAILABLE;

    /*
     * Button pressed?
     */
    if (TestWF(pwnd, WFSCROLLBUTTONDOWN) &&
            ((idObject != OBJID_VSCROLL) || TestWF(pwnd, WFVERTSCROLLTRACK))) {
        if (TestWF(pwnd, WFLINEUPBUTTONDOWN))
            psbi->rgstate[INDEX_SCROLLBAR_UP] |= STATE_SYSTEM_PRESSED;

        if (TestWF(pwnd, WFPAGEUPBUTTONDOWN))
            psbi->rgstate[INDEX_SCROLLBAR_UPPAGE] |= STATE_SYSTEM_PRESSED;

        if (TestWF(pwnd, WFPAGEDNBUTTONDOWN))
            psbi->rgstate[INDEX_SCROLLBAR_DOWNPAGE] |= STATE_SYSTEM_PRESSED;

        if (TestWF(pwnd, WFLINEDNBUTTONDOWN))
            psbi->rgstate[INDEX_SCROLLBAR_DOWN] |= STATE_SYSTEM_PRESSED;
    }

    /*
     * Fill in area locations.
     */
    if (!(psbi->rgstate[INDEX_SCROLLBAR_SELF] & STATE_SYSTEM_OFFSCREEN)) {
        if (fVertical) {
            psbi->rcScrollBar.left = SBCalc.pxLeft;
            psbi->rcScrollBar.top = SBCalc.pxTop;
            psbi->rcScrollBar.right = SBCalc.pxRight;
            psbi->rcScrollBar.bottom = SBCalc.pxBottom;
        } else {
            psbi->rcScrollBar.left = SBCalc.pxTop;
            psbi->rcScrollBar.top = SBCalc.pxLeft;
            psbi->rcScrollBar.right = SBCalc.pxBottom;
            psbi->rcScrollBar.bottom = SBCalc.pxRight;
        }

        if (idObject == OBJID_CLIENT) {
            OffsetRect(&psbi->rcScrollBar, pwnd->rcClient.left, pwnd->rcClient.top);
        } else {
            OffsetRect(&psbi->rcScrollBar, pwnd->rcWindow.left, pwnd->rcWindow.top);
        }

        psbi->dxyLineButton = (SBCalc.pxUpArrow - SBCalc.pxTop);
        psbi->xyThumbTop = (SBCalc.pxThumbTop - SBCalc.pxTop);
        psbi->xyThumbBottom = (SBCalc.pxThumbBottom - SBCalc.pxTop);

        /*
         * Is the thumb all the way to the left/top?  If so, page up is
         * not visible.
         */
        if (SBCalc.pxThumbTop == SBCalc.pxUpArrow)
            psbi->rgstate[INDEX_SCROLLBAR_UPPAGE] |= STATE_SYSTEM_INVISIBLE;

        /*
         * Is the thumb all the way to the right/down?  If so, page down
         * is not visible.
         */
        if (SBCalc.pxThumbBottom == SBCalc.pxDownArrow)
            psbi->rgstate[INDEX_SCROLLBAR_DOWNPAGE] |= STATE_SYSTEM_INVISIBLE;
    }

    return TRUE;
}


/*****************************************************************************\
* GetAncestor()
*
* This gets one of:
*     * The _real_ parent.  This does NOT include the owner, unlike
*         GetParent().  Stops at a top level window unless we start with
*         the desktop.  In which case, we return the desktop.
*     * The _real_ root, caused by walking up the chain getting the
*         ancestor.
*     * The _real_ owned root, caused by GetParent()ing up.
\*****************************************************************************/
PWND WINAPI
_GetAncestor(PWND pwnd, UINT gaFlags)
{
    PWND pwndParent;

    /*
     * If we start with the desktop return NULL.
     */
    if (pwnd == PWNDDESKTOP(pwnd))
        return NULL;

    switch (gaFlags) {
    case GA_PARENT:
        pwnd = pwnd->spwndParent;
        break;

    case GA_ROOT:
        while ((pwnd->spwndParent != PWNDDESKTOP(pwnd)) &&
               (pwnd->spwndParent != PWNDMESSAGE(pwnd))) {
            pwnd = pwnd->spwndParent;
        }
        break;

    case GA_ROOTOWNER:
        while (pwndParent = _GetParent(pwnd)) {
            pwnd = pwndParent;
        }
        break;
    }

    return pwnd;
}


/*****************************************************************************\
*
* RealChildWindowFromPoint()
*
* This returns the REAL child window at a point.  The problem is that
* ChildWindowFromPoint() doesn't deal with HTTRANSPARENT areas of
* standard controls.  We want to return a child behind a groupbox if it
* is in the "clear" area.  But we want to return a static field always
* even though it too returns HTTRANSPARENT.
*
\*****************************************************************************/
PWND WINAPI
_RealChildWindowFromPoint(PWND pwndParent, POINT pt)
{
    PWND    pwndChild;
    PWND    pwndSave;

    if (pwndParent != PWNDDESKTOP(pwndParent)) {
        pt.x += pwndParent->rcClient.left;
        pt.y += pwndParent->rcClient.top;
    }

    /*
     * Is this point even in the parent?
     */
    if (!PtInRect(&pwndParent->rcClient, pt)  ||
        (pwndParent->hrgnClip && !GrePtInRegion(pwndParent->hrgnClip, pt.x, pt.y))) {
        // Nope
        return NULL;
    }

    pwndSave = NULL;

    /*
     * Loop through the children.
     */
    for (pwndChild = pwndParent->spwndChild; pwndChild; pwndChild = pwndChild->spwndNext) {
        if (!TestWF(pwndChild, WFVISIBLE))
            continue;

        /*
         * Is this point in the child's window?
         */
        if (!PtInRect(&pwndChild->rcWindow, pt) ||
                (pwndChild->hrgnClip && !GrePtInRegion(pwndChild->hrgnClip, pt.x, pt.y)))
            continue;

        /*
         * OK, we are in somebody's window.  Is this by chance a group box?
         */
        if ((pwndChild->pcls->atomClassName == gpsi->atomSysClass[ICLS_BUTTON]) ||
                (GETFNID(pwndChild) == FNID_BUTTON)) {
            if (TestWF(pwndChild, BFTYPEMASK) == LOBYTE(BS_GROUPBOX)) {
               pwndSave = pwndChild;
               continue;
            }
        }

        return pwndChild;
    }

    /*
     * Did we save a groupbox which turned out to have nothing behind it
     * at that point?
     */
    if (pwndSave) {
        return pwndSave;
    } else {
        return pwndParent;
    }
}


/*****************************************************************************\
* xxxGetMenuBarInfo()
*
* This succeeds if the menu/menu item exists.
*
* Parameters:
*     pwnd        window
*     idObject    this can be OBJID_MENU, OBJID_SYSMENU, or OBJID_CLIENT
*     idItem      which thing do we need info on? 0..cItems. 0 indicates
*                 the menu itself, 1 is the first item on the menu...
*     pmbi        Pointer to a MENUBARINFO structure that gets filled in
*
\*****************************************************************************/
BOOL WINAPI
xxxGetMenuBarInfo(PWND pwnd, long idObject, long idItem, PMENUBARINFO pmbi)
{
    PMENU       pMenu;
    int         cBorders;
    PITEM       pItem;
    PPOPUPMENU  ppopup;

    CheckLock(pwnd);

    /*
     * Validate menubarinfo structure
     */
    if (pmbi->cbSize != sizeof(MENUBARINFO)) {
        RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "MENUBARINFO.cbSize %d is wrong", pmbi->cbSize);
        return FALSE;
    }

    /*
     * Initialize the fields
     */
    SetRectEmpty(&pmbi->rcBar);
    pmbi->hMenu = NULL;
    pmbi->hwndMenu = NULL;
    pmbi->fBarFocused = FALSE;
    pmbi->fFocused = FALSE;

    /*
     * Get the menu handle we will deal with.
     */
    if (idObject == OBJID_MENU) {
        int cBorders;

        if (TestWF(pwnd, WFCHILD) || !pwnd->spmenu)
            return FALSE;

        pMenu = pwnd->spmenu;
        if (!pMenu)
            return FALSE;

        // If we have an item, is it in the valid range?
        if ((idItem < 0) || ((DWORD)idItem > pMenu->cItems))
            return FALSE;

        /*
         * Menu handle
         */
        pmbi->hMenu = PtoHq(pMenu);

        /*
         * Menu rect
         */
        if (pMenu->cxMenu && pMenu->cyMenu) {
            if (!idItem) {
                cBorders = GetWindowBorders(pwnd->style, pwnd->ExStyle, TRUE, FALSE);
                pmbi->rcBar.left = pwnd->rcWindow.left + cBorders * SYSMET(CXBORDER);
                pmbi->rcBar.top = pwnd->rcWindow.top + cBorders * SYSMET(CYBORDER);

                if (TestWF(pwnd, WFCPRESENT)) {
                    pmbi->rcBar.top += (TestWF(pwnd, WEFTOOLWINDOW) ? SYSMET(CYSMCAPTION) : SYSMET(CYCAPTION));
                }

                pmbi->rcBar.right = pmbi->rcBar.left + pMenu->cxMenu;
                pmbi->rcBar.bottom = pmbi->rcBar.top + pMenu->cyMenu;
            } else {
                pItem = pMenu->rgItems + idItem - 1;

                pmbi->rcBar.left = pwnd->rcWindow.left + pItem->xItem;
                pmbi->rcBar.top = pwnd->rcWindow.top + pItem->yItem;
                pmbi->rcBar.right = pmbi->rcBar.left + pItem->cxItem;
                pmbi->rcBar.bottom = pmbi->rcBar.top + pItem->cyItem;
            }
        }

        /*
         * Are we currently in app menu bar mode?
         */
        ppopup = GetpGlobalPopupMenu(pwnd);
        if (ppopup && ppopup->fHasMenuBar && !ppopup->fIsSysMenu &&
            (ppopup->spwndNotify == pwnd))
        {
            // Yes, we are.
            pmbi->fBarFocused = TRUE;

            if (!idItem) {
                pmbi->fFocused = TRUE;
            } else if (ppopup->ppopupmenuRoot->posSelectedItem == (UINT)idItem-1) {
                pmbi->fFocused = TRUE;
                UserAssert(ppopup->ppopupmenuRoot);
                pmbi->hwndMenu = HW(ppopup->ppopupmenuRoot->spwndNextPopup);
            }
        }

    } else if (idObject == OBJID_SYSMENU) {
        if (!TestWF(pwnd, WFSYSMENU))
            return FALSE;

        pMenu = xxxGetSysMenu(pwnd, FALSE);
        if (!pMenu)
            return FALSE;

        // If we have an item, is it in the valid range?
        if ((idItem < 0) || ((DWORD)idItem > pMenu->cItems))
            return FALSE;

        pmbi->hMenu = PtoHq(pMenu);

        /*
         * Menu rect
         */
        if (_HasCaptionIcon(pwnd)) {
            // The menu and single item take up the same space
            cBorders = GetWindowBorders(pwnd->style, pwnd->ExStyle, TRUE, FALSE);
            pmbi->rcBar.left = pwnd->rcWindow.left + cBorders * SYSMET(CXBORDER);
            pmbi->rcBar.top = pwnd->rcWindow.top + cBorders * SYSMET(CYBORDER);

            pmbi->rcBar.right = pmbi->rcBar.left +
                (TestWF(pwnd, WEFTOOLWINDOW) ? SYSMET(CXSMSIZE) : SYSMET(CXSIZE));

            pmbi->rcBar.bottom = pmbi->rcBar.top +
                (TestWF(pwnd, WEFTOOLWINDOW) ? SYSMET(CYSMSIZE) : SYSMET(CYSIZE));
        }

        /*
         * Are we currently in system menu bar mode?
         */
        ppopup = GetpGlobalPopupMenu(pwnd);
        if (ppopup && ppopup->fHasMenuBar && ppopup->fIsSysMenu &&
            (ppopup->spwndNotify == pwnd))
        {
            // Yes, we are.
            pmbi->fBarFocused = TRUE;

            if (!idItem) {
                pmbi->fFocused = TRUE;
            } else if (ppopup->ppopupmenuRoot->posSelectedItem == (UINT)idItem - 1) {
                pmbi->fFocused = TRUE;
                UserAssert(ppopup->ppopupmenuRoot);
                pmbi->hwndMenu = HW(ppopup->ppopupmenuRoot->spwndNextPopup);
            }
        }
    }
    else if (idObject == OBJID_CLIENT)
    {
        HMENU hMenu;
        hMenu = (HMENU)xxxSendMessage(pwnd, MN_GETHMENU, 0, 0);
        pMenu = HtoP(hMenu);
        if (!pMenu)
            return FALSE;

        // If we have an item, is it in the valid range?
        if ((idItem < 0) || ((DWORD)idItem > pMenu->cItems))
            return FALSE;

        pmbi->hMenu = hMenu;

        if (!idItem) {
            pmbi->rcBar = pwnd->rcClient;
        } else {
            pItem = pMenu->rgItems + idItem - 1;

            pmbi->rcBar.left = pwnd->rcClient.left + pItem->xItem;
            pmbi->rcBar.top = pwnd->rcClient.top + pItem->yItem;
            pmbi->rcBar.right = pmbi->rcBar.left + pItem->cxItem;
            pmbi->rcBar.bottom = pmbi->rcBar.top + pItem->cyItem;
        }

        /*
         * Are we currently in popup mode with us as one of the popups
         * showing?
         */
        ppopup = ((PMENUWND)pwnd)->ppopupmenu;
        if (ppopup && (ppopup->ppopupmenuRoot == GetpGlobalPopupMenu(pwnd))) {
            pmbi->fBarFocused = TRUE;

            if (!idItem) {
                pmbi->fFocused = TRUE;
            } else if ((UINT)idItem == ppopup->posSelectedItem + 1) {
                pmbi->fFocused = TRUE;
                pmbi->hwndMenu = HW(ppopup->spwndNextPopup);
            }
        }
    } else {
        return FALSE;
    }

    return TRUE;
}


/***************************************************************************\
*
* _GetComboBoxInfo()
*
* This returns combobox information for either a combo or its dropdown
* list.
*
\***************************************************************************/
BOOL WINAPI
_GetComboBoxInfo(PWND pwnd, PCOMBOBOXINFO pcbi)
{
    PCLS    pcls;
    COMBOBOXINFO cbi = {
        sizeof cbi,
    };
    BOOL fOtherProcess;
    BOOL bRetval = FALSE;
    WORD wWindowType = 0;

    /*
     * Make sure it is a combobox or a dropdown
     */
    pcls = pwnd->pcls;


    if ((GETFNID(pwnd) == FNID_COMBOBOX) ||
            (pcls->atomClassName == gpsi->atomSysClass[ICLS_COMBOBOX])) {
        wWindowType = FNID_COMBOBOX;
    } else if ((GETFNID(pwnd) == FNID_COMBOLISTBOX) ||
            (pcls->atomClassName == gpsi->atomSysClass[ICLS_COMBOLISTBOX])) {
        wWindowType = FNID_COMBOLISTBOX;
    } else {
        RIPERR1(ERROR_WINDOW_NOT_COMBOBOX, RIP_WARNING,
                "pwnd %#p not a combobox or dropdown", pwnd);
        return FALSE;
    }

    /*
     * Validate combo structure
     */
    if (pcbi->cbSize != sizeof(COMBOBOXINFO)) {
        RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "COMBOBOXINFO.cbSize %d is wrong", pcbi->cbSize);
        return FALSE;
    }

    if (fOtherProcess = (GETPTI(pwnd)->ppi != PpiCurrent())) {
        KeAttachProcess(&GETPTI(pwnd)->ppi->Process->Pcb);
    }

    try {
        PCBOX ccxPcboxSnap;
        PWND  ccxPwndSnap;
        HWND  ccxHwndSnap;

        /*
         * Snap and probe the CBOX structure, since it is client side.
         */
        if (wWindowType == FNID_COMBOBOX) {
            ccxPcboxSnap = ((PCOMBOWND)pwnd)->pcbox;
        } else {
            PLBIV ccxPlbSnap;
            /*
             * If this is a listbox, we must snap and probe the LBIV structure
             * in order to get to the CBOX structure.
             */
            ccxPlbSnap = ((PLBWND)pwnd)->pLBIV;
            if (!ccxPlbSnap) {
                goto errorexit;
            }
            ProbeForRead(ccxPlbSnap, sizeof(LBIV), DATAALIGN);
            ccxPcboxSnap = ccxPlbSnap->pcbox;
        }
        if (!ccxPcboxSnap) {
            goto errorexit;
        }
        ProbeForRead(ccxPcboxSnap, sizeof(CBOX), DATAALIGN);

        /*
         * Get the combo information now
         */

        /*
         * Snap and probe the client side pointer to the Combo window
         */
        ccxPwndSnap = ccxPcboxSnap->spwnd;
        ProbeForRead(ccxPwndSnap, sizeof(HEAD), DATAALIGN);
        cbi.hwndCombo = HWCCX(ccxPwndSnap);

        /*
         * Snap & probe the client side pointer to the Edit window.
         * To compare spwndEdit and pwnd, we should compare handles
         * since spwndEdit is a client-side address and pwnd is a
         * kernel-mode address,
         */

        ccxPwndSnap = ccxPcboxSnap->spwndEdit;
        /*
         * If combobox is not fully initialized and spwndEdit is NULL,
         * we should fail.
         */
        ProbeForRead(ccxPwndSnap, sizeof(HEAD), DATAALIGN);
        ccxHwndSnap = HWCCX(ccxPwndSnap);
        if (ccxHwndSnap == HW(pwnd)) {
            /*
             * ComboBox doesn't have Edit control.
             */
            cbi.hwndItem = NULL;
        } else {
            cbi.hwndItem = HWCCX(ccxPwndSnap);
        }

        /*
         * Snap and probe the client side pointer to the List window
         */
        ccxPwndSnap = ccxPcboxSnap->spwndList;
        /*
         * If combobox is not fully initialized and spwndList is NULL,
         * we should fail.
         */
        ProbeForRead(ccxPwndSnap, sizeof(HEAD), DATAALIGN);
        cbi.hwndList = HWCCX(ccxPwndSnap);

        /*
         * Snap the rest of the combo information.
         * We don't need to probe any of these, since there are no more indirections.
         */
        cbi.rcItem = ccxPcboxSnap->editrc;
        cbi.rcButton = ccxPcboxSnap->buttonrc;

        /*
         * Button state
         */
        cbi.stateButton = 0;
        if (ccxPcboxSnap->CBoxStyle == CBS_SIMPLE) {
            cbi.stateButton |= STATE_SYSTEM_INVISIBLE;
        }
        if (ccxPcboxSnap->fButtonPressed) {
            cbi.stateButton |= STATE_SYSTEM_PRESSED;
        }
    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        goto errorexit;
    }

    *pcbi = cbi;
    bRetval = TRUE;

errorexit:
    if (fOtherProcess) {
        KeDetachProcess();
    }
    return bRetval;
}


/***************************************************************************\
*
* _GetListBoxInfo()
*
* Currently returns back the # of items per column.  There is no way to get
* or calculate this info any other way in a multicolumn list.
*
* For now, no structure is returned. If we ever need one more thing, make one.
*
* Since I have to run on multiple platforms, I can't define a message.
* To do so would require that
*     * I make changes to the thunk table
*     * I make sure the 32-bit define doesn't collide with some NT new msg
*     * I use a different value on Win '95 vs Memphis due to additions
*     * I test apps extensively since many of them pass on bogus valued
*         messages to the listbox handler which checks to see if they
*         are in range.  In other words, any value I pick is probably
*         going to flake out MSVC++ 4.0.
*
*  Ergo an API instead.
*
\***************************************************************************/
DWORD WINAPI
_GetListBoxInfo(PWND pwnd)
{
    PCLS    pcls;
    DWORD   dwRet = 0;
    BOOL    fOtherProcess;

    /*
     * Make sure it is a combobox or a dropdown
     */
    pcls = pwnd->pcls;
    if ((pcls->atomClassName != gpsi->atomSysClass[ICLS_LISTBOX]) &&
            (GETFNID(pwnd) != FNID_LISTBOX)) {
        RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "pwnd %#p is not a listbox", pwnd);
        return 0;
    }

    if (fOtherProcess = (GETPTI(pwnd)->ppi != PpiCurrent())) {
        KeAttachProcess(&GETPTI(pwnd)->ppi->Process->Pcb);
    }

    try {
        PLBIV   ccxPlbSnap;

        /*
         * Snap and probe the pointer to the LBIV, since it is client-side.
         */
        ccxPlbSnap = ((PLBWND)pwnd)->pLBIV;
        if (!ccxPlbSnap) {
            goto errorexit;
        }
        ProbeForRead(ccxPlbSnap, sizeof(LBIV), DATAALIGN);

        if (ccxPlbSnap->fMultiColumn) {
            dwRet = ccxPlbSnap->itemsPerColumn;
        } else {
            dwRet = ccxPlbSnap->cMac;
        }
    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        dwRet = 0;
    }

errorexit:
    if (fOtherProcess) {
        KeDetachProcess();
    }

    return dwRet;
}
