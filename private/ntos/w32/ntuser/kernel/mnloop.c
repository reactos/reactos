/**************************** Module Header ********************************\
* Module Name: mnloop.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Menu Modal Loop Routines
*
* History:
* 10-10-90 JimA       Cleanup.
* 03-18-91 IanJa      Window revalidation added
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* xxxMNRemoveMessage
*
* History
* 11/23/96  GerardoB  Created
\***************************************************************************/
BOOL xxxMNRemoveMessage (UINT message1, UINT message2)
{
    MSG msg;
    if (!xxxPeekMessage(&msg, NULL, 0, 0, PM_NOYIELD | PM_NOREMOVE)) {
        return FALSE;
    }

    if ((msg.message == message1) || (msg.message == message2)) {
        UserAssert(msg.message != 0);
        xxxPeekMessage(&msg, NULL, msg.message, msg.message, PM_REMOVE);
        return TRUE;
    } else {
        return FALSE;
    }
}
/***************************************************************************\
* xxxHandleMenuMessages
*
* History:
\***************************************************************************/

BOOL xxxHandleMenuMessages(
    LPMSG lpmsg,
    PMENUSTATE pMenuState,
    PPOPUPMENU ppopupmenu)
{
    DWORD ch;
    ULONG_PTR cmdHitArea;
    UINT cmdItem;
    LPARAM lParam;
    BOOL fThreadLock = FALSE;
    TL tlpwndHitArea;
    TL tlpwndT;
    POINT pt;
    PWND pwnd;
    RECT rc;

    /*
     * Paranoia. Let's bail up front if we don't have a menu.
     * Some code checks for NULL spmenu, other parts assume it's always not NULL
     * Use RIP_ERROR for a while to make sure this is OK
     */
    if (ppopupmenu->spmenu == NULL) {
        RIPMSG2(RIP_ERROR, "xxxHandleMenuMessages NULL spmenu. pMenuSate:%p ppopupmenu:%p",
                pMenuState, ppopupmenu);
        return FALSE;
    }
    /*
     * Get things out of the structure so that we can access them quicker.
     */
    ch = (DWORD)lpmsg->wParam;
    lParam = lpmsg->lParam;

    /*
     * In this switch statement, we only look at messages we want to handle and
     * swallow.  Messages we don't understand will get translated and
     * dispatched.
     */
    switch (lpmsg->message) {
    case WM_RBUTTONDOWN:
    case WM_NCRBUTTONDOWN:

        if (ppopupmenu->fRightButton) {
            goto HandleButtonDown;
        }
        /*
         * Fall through
         */
    case WM_RBUTTONDBLCLK:
    case WM_NCRBUTTONDBLCLK:
        /*
         * Right click outside the menu dismisses the menu
         * (we didn't use to do this for single right clicks on 4.0)
         */
        pMenuState->mnFocus = MOUSEHOLD;
        cmdHitArea = xxxMNFindWindowFromPoint(ppopupmenu, &cmdItem, MAKEPOINTS(lParam));
        if (cmdHitArea == MFMWFP_OFFMENU) {
            xxxMNDismiss(pMenuState);
            return TRUE;
        }
        /*
         * Do nothing on right clicks on the menu
         */
        if (!pMenuState->fModelessMenu) {
            xxxMNRemoveMessage(lpmsg->message, 0);
        }
        return TRUE;

    case WM_LBUTTONDOWN:
    case WM_NCLBUTTONDOWN:
// Commented out due to TandyT whinings...
// if ((ppopupmenu->trackPopupMenuFlags & TPM_RIGHTBUTTON))
// break;

HandleButtonDown:

        /*
         * Find out where this mouse down occurred.
         */
        pMenuState->mnFocus = MOUSEHOLD;
        pMenuState->ptMouseLast.x = GET_X_LPARAM(lParam);
        pMenuState->ptMouseLast.y =  GET_Y_LPARAM(lParam);
        cmdHitArea = xxxMNFindWindowFromPoint(ppopupmenu, &cmdItem, MAKEPOINTS(lParam));


        /*
         * Thread lock this if it is a pwnd.  This certainly isn't the way
         * you'd implement this if you had locking to begin with.
         */
        fThreadLock = IsMFMWFPWindow(cmdHitArea);
        if (fThreadLock) {
            ThreadLock((PWND)cmdHitArea, &tlpwndHitArea);
        }

        /*
         * If this is a drag and drop menu, remember the mouse
         *  position and the hit test results.
         */
        if (pMenuState->fDragAndDrop) {
            pMenuState->ptButtonDown = pMenuState->ptMouseLast;
            pMenuState->uButtonDownIndex = cmdItem;
            LockMFMWFPWindow(&pMenuState->uButtonDownHitArea, cmdHitArea);
        }

        /*
         * Modeless menus don't capture the mouse so we might not see
         *  the button up. We also release capture when sending the
         *  WM_MENUDODRAGDROP message. So we want to remember what
         *  mouse button went down.
         */
        if (pMenuState->fDragAndDrop || pMenuState->fModelessMenu) {
            if (ch & MK_RBUTTON) {
                pMenuState->vkButtonDown = VK_RBUTTON;
            } else {
                pMenuState->vkButtonDown = VK_LBUTTON;
            }
        }


        if ((cmdHitArea == MFMWFP_OFFMENU) && (cmdItem == 0)) {
            //
            // Clicked in middle of nowhere, so terminate menus, and
            // let button pass through.
CancelOut:
            xxxMNDismiss(pMenuState);
            goto Unlock;
        } else if (ppopupmenu->fHasMenuBar && (cmdHitArea == MFMWFP_ALTMENU)) {
            //
            // Switching between menu bar & popup
            //
            xxxMNSwitchToAlternateMenu(ppopupmenu);
            cmdHitArea = MFMWFP_NOITEM;
        }

        if (cmdHitArea == MFMWFP_NOITEM) {
            //
            // On menu bar (system or main)
            //
            xxxMNButtonDown(ppopupmenu, pMenuState, cmdItem, TRUE);
        } else {
            // On popup window menu
            UserAssert(cmdHitArea);
            xxxSendMessage((PWND)cmdHitArea, MN_BUTTONDOWN, cmdItem, 0L);
        }

        /*
         * Swallow the message since we handled it.
         */
            /*
             * The excel guys change a wm_rbuttondown to a wm_lbuttondown message
             * in their message filter hook.  Remove the message here or we'll
             * get in a nasty loop.
             *
             * We need to swallow msg32.message ONLY.  It is possible for
             * the LBUTTONDOWN to not be at the head of the input queue.
             * If not, we will swallow a WM_MOUSEMOVE or something else like
             * that.  The reason Peek() doesn't need to check the range
             * is because we've already Peek(PM_NOYIELD'ed) before, which
             * locked the sys queue.
             */
        if (!pMenuState->fModelessMenu) {
            xxxMNRemoveMessage(lpmsg->message, WM_RBUTTONDOWN);
        }
        goto Unlock;

    case WM_MOUSEMOVE:
    case WM_NCMOUSEMOVE:

        /*
         * Is the user starting to drag?
         */
        if (pMenuState->fDragAndDrop
                && pMenuState->fButtonDown
                && !pMenuState->fDragging
                && !pMenuState->fButtonAlwaysDown
                && (pMenuState->uButtonDownHitArea != MFMWFP_OFFMENU)) {

            /*
             * We expect the mouse to go down on a menu item before a drag can start
             */
            UserAssert(!ppopupmenu->fFirstClick);

            /*
             * Calculate drag detect rectangle using the position the mouse went
             *  down on
             */
            *(LPPOINT)&rc.left = pMenuState->ptButtonDown;
            *(LPPOINT)&rc.right = pMenuState->ptButtonDown;
            InflateRect(&rc, SYSMET(CXDRAG), SYSMET(CYDRAG));

            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);

            /*
             * If we've moved outside the drag rect, then the user is dragging
             */
            if (!PtInRect(&rc, pt)) {
                /*
                 * Post a message so we'll finish processing this message
                 *  and get out of here before telling the app that the user
                 *  is dragging
                 */
                pwnd = GetMenuStateWindow(pMenuState);
                if (pwnd != NULL) {
                    pMenuState->fDragging = TRUE;
                    _PostMessage(pwnd, MN_DODRAGDROP, 0, 0);
                } else {
                    RIPMSG0(RIP_ERROR, "xxxMNMouseMove. Unble to post MN_DODGRAGDROP");
                }
             }
        } /* if (pMenuState->fDragAndDrop */

        xxxMNMouseMove(ppopupmenu, pMenuState, MAKEPOINTS(lParam));
        return TRUE;

    case WM_RBUTTONUP:
    case WM_NCRBUTTONUP:
        if (ppopupmenu->fRightButton) {
            goto HandleButtonUp;
        }
        /*
         * If the button is down, simply swallow this message
         */
        if (pMenuState->fButtonDown) {
            if (!pMenuState->fModelessMenu) {
                xxxMNRemoveMessage(lpmsg->message, 0);
            }
            return TRUE;
        }
        // New feature for shell start menu -- notify when a right click
        // occurs on a menu item, and open a window of opportunity for
        // menus to recurse, allowing them to popup a context-sensitive
        // menu for that item.      (jeffbog 9/28/95)
        //
        // BUGBUG: need to add check for Nashville+ app
        if ((lpmsg->message == WM_RBUTTONUP) && !ppopupmenu->fNoNotify) {
                PPOPUPMENU ppopupActive;

                if ((ppopupmenu->spwndActivePopup != NULL)
                        && (ppopupActive = ((PMENUWND)(ppopupmenu->spwndActivePopup))->ppopupmenu)
                        && MNIsItemSelected(ppopupActive))
                {
                    TL tlpwndNotify;
                    ThreadLock( ppopupActive->spwndNotify, &tlpwndNotify );
                    xxxSendMessage(ppopupActive->spwndNotify, WM_MENURBUTTONUP,
                            ppopupActive->posSelectedItem, (LPARAM)PtoH(ppopupActive->spmenu));
                    ThreadUnlock( &tlpwndNotify );
                }
            }
        break;

    case WM_LBUTTONUP:
    case WM_NCLBUTTONUP:
// Commented out due to TandyT whinings...
// if ((ppopupmenu->trackPopupMenuFlags & TPM_RIGHTBUTTON))
// break;

HandleButtonUp:
        if (!pMenuState->fButtonDown) {

            /*
             * Don't care about this mouse up since we never saw the button
             * down for some reason.
             */
            return TRUE;
        }

        /*
         * Cancel the dragging state, if any.
         */
        if (pMenuState->fDragAndDrop) {

            UnlockMFMWFPWindow(&pMenuState->uButtonDownHitArea);
            pMenuState->fDragging = FALSE;

            if (pMenuState->fIgnoreButtonUp) {
                pMenuState->fButtonDown =
                pMenuState->fIgnoreButtonUp = FALSE;
                return TRUE;
            }
        }

        /*
         * Find out where this mouse up occurred.
         */
        pMenuState->ptMouseLast.x = GET_X_LPARAM(lParam);
        pMenuState->ptMouseLast.y = GET_Y_LPARAM(lParam);
        cmdHitArea = xxxMNFindWindowFromPoint(ppopupmenu, &cmdItem, MAKEPOINTS(lParam));


        /*
         * If this is not true, some the code below won't work right.
         */
        UserAssert((cmdHitArea != MFMWFP_OFFMENU) || (cmdItem == 0));
        UserAssert(cmdHitArea != 0x0000FFFF);

        /*
         * Thread lock this if it is a pwnd.  This certainly isn't the way
         * you'd implement this if you had locking to begin with.
         */
        fThreadLock = IsMFMWFPWindow(cmdHitArea);
        if (fThreadLock) {
            ThreadLock((PWND)cmdHitArea, &tlpwndHitArea);
        }


        if (ppopupmenu->fHasMenuBar) {
            if (((cmdHitArea == MFMWFP_OFFMENU) && (cmdItem == 0)) ||
                    ((cmdHitArea == MFMWFP_NOITEM) && ppopupmenu->fIsSysMenu && ppopupmenu->fToggle))
                    // Button up occurred in some random spot.  Terminate
                    // menus and swallow the message.
                    goto CancelOut;
        } else {
            if ((cmdHitArea == MFMWFP_OFFMENU) && (cmdItem == 0)) {
                if (!ppopupmenu->fFirstClick) {
                    //
                    // User upclicked in some random spot. Terminate
                    // menus and don't swallow the message.
                    //

                    //
                    // Don't do anything with HWND here cuz the window is
                    // destroyed after this SendMessage().
                    //
//                    DONTREVALIDATE();
                    ThreadLock(ppopupmenu->spwndPopupMenu, &tlpwndT);
                    xxxSendMessage(ppopupmenu->spwndPopupMenu, MN_CANCELMENUS, 0, 0);
                    ThreadUnlock(&tlpwndT);
                    goto Unlock;
                }
            }

            ppopupmenu->fFirstClick = FALSE;
        }

        if (cmdHitArea == MFMWFP_NOITEM) {
            //
            // This is a system menu or a menu bar and the button up
            // occurred on the system menu or on a menu bar item.
            //
            xxxMNButtonUp(ppopupmenu, pMenuState, cmdItem, 0);
        } else if ((cmdHitArea != MFMWFP_OFFMENU) && (cmdHitArea != MFMWFP_ALTMENU)) {
            //
            // Warning:  It's common for the popup to go away during the
            // processing of this message, so don't add any code that
            // messes with hwnd after this call!
            //
//            DONTREVALIDATE();

            //
            // We send lParam (that has the mouse co-ords ) for the app
            // to get it in its SC_RESTORE/SC_MINIMIZE messages 3.0
            // compat
            //
            xxxSendMessage((PWND)cmdHitArea, MN_BUTTONUP, (DWORD)cmdItem, lParam);
        } else {
            pMenuState->fButtonDown =
            pMenuState->fButtonAlwaysDown = FALSE;
        }
Unlock:
        if (fThreadLock)
            ThreadUnlock(&tlpwndHitArea);
        return TRUE;


    case WM_LBUTTONDBLCLK:
    case WM_NCLBUTTONDBLCLK:

        // Commented out due to TandyT whinings...
        //        if (ppopup->fRightButton)
        //            break;
        pMenuState->mnFocus = MOUSEHOLD;
        cmdHitArea = xxxMNFindWindowFromPoint(ppopupmenu, &cmdItem, MAKEPOINTS(lParam));
        if ((cmdHitArea == MFMWFP_OFFMENU) && (cmdItem == 0)) {
                // Dbl-clicked in middle of nowhere, so terminate menus, and
                // let button pass through.
                xxxMNDismiss(pMenuState);
                return TRUE;
        } else if (ppopupmenu->fHasMenuBar && (cmdHitArea == MFMWFP_ALTMENU)) {
            //
            // BOGUS
            // TREAT LIKE BUTTON DOWN since we didn't dblclk on same item.
            //
            xxxMNSwitchToAlternateMenu(ppopupmenu);
            cmdHitArea =  MFMWFP_NOITEM;
        }

        if (cmdHitArea == MFMWFP_NOITEM)
            xxxMNDoubleClick(pMenuState, ppopupmenu, cmdItem);
        else {
            UserAssert(cmdHitArea);

            ThreadLock((PWND)cmdHitArea, &tlpwndHitArea);
            xxxSendMessage((PWND)cmdHitArea, MN_DBLCLK,
                    (DWORD)cmdItem, 0L);
            ThreadUnlock(&tlpwndHitArea);
        }
        return TRUE;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:

        /*
         * If mouse button is down, ignore keyboard input (fix #3899, IanJa)
         */
        if (pMenuState->fButtonDown && (ch != VK_F1)) {

            /*
             * Check if the user wants to cancel dragging.
             */
            if (pMenuState->fDragging && (ch == VK_ESCAPE)) {
                RIPMSG0(RIP_WARNING, "xxxHandleMenuMessages: ESC while dragging");
                pMenuState->fIgnoreButtonUp = TRUE;
            }

            return TRUE;
        }
        pMenuState->mnFocus = KEYBDHOLD;
        switch (ch) {
        case VK_UP:
        case VK_DOWN:
        case VK_LEFT:
        case VK_RIGHT:
        case VK_RETURN:
        case VK_CANCEL:
        case VK_ESCAPE:
        case VK_MENU:
        case VK_F10:
        case VK_F1:
            if (ppopupmenu->spwndActivePopup) {
                ThreadLockAlways(ppopupmenu->spwndActivePopup, &tlpwndT);
                xxxSendMessage(ppopupmenu->spwndActivePopup, lpmsg->message,
                        ch, 0L);
                ThreadUnlock(&tlpwndT);
            } else {
                xxxMNKeyDown(ppopupmenu, pMenuState, (UINT)ch);
            }
            break;

        case VK_TAB:
            /*
             * People hit the ALT key now just to turn underlines ON in dialogs.
             * This throws them into "invisible menu mode". If they hit any char
             * at that point, we'll bail in xxxMNChar. But not so if they hit ctrl-tab,
             * which is used to navigate property sheets. So let's help them out.
             */
            if (ppopupmenu->fIsMenuBar && (ppopupmenu->spwndActivePopup == NULL)) {
                xxxMNDismiss(pMenuState);
                return TRUE;
            }
            /*
             * Fall through
             */

        default:
TranslateKey:
            if (!pMenuState->fModelessMenu) {
                xxxTranslateMessage(lpmsg, 0);
            }
            break;
        }
        return TRUE;

    case WM_CHAR:
    case WM_SYSCHAR:
        if (ppopupmenu->spwndActivePopup) {
            ThreadLockAlways(ppopupmenu->spwndActivePopup, &tlpwndT);
            xxxSendMessage(ppopupmenu->spwndActivePopup, lpmsg->message,
                        ch, 0L);
            ThreadUnlock(&tlpwndT);
        } else {
            xxxMNChar(ppopupmenu, pMenuState, (UINT)ch);
        }
        return TRUE;

    case WM_SYSKEYUP:

        /*
         * Ignore ALT and F10 keyup messages since they are handled on
         * the KEYDOWN message.
         */
        if (ch == VK_MENU || ch == VK_F10) {
            if (gwinOldAppHackoMaticFlags & WOAHACK_CHECKALTKEYSTATE) {
                if (gwinOldAppHackoMaticFlags & WOAHACK_IGNOREALTKEYDOWN) {
                    gwinOldAppHackoMaticFlags &= ~WOAHACK_IGNOREALTKEYDOWN;
                    gwinOldAppHackoMaticFlags &= ~WOAHACK_CHECKALTKEYSTATE;
                } else
                    gwinOldAppHackoMaticFlags |= WOAHACK_IGNOREALTKEYDOWN;
            }

            return TRUE;
        }

        /*
         ** fall thru **
         */

    case WM_KEYUP:

        /*
         * Do RETURNs on the up transition only
         */
        goto TranslateKey;

      case WM_SYSTIMER:

        /*
         * Prevent the caret from flashing by eating all WM_SYSTIMER messages.
         */
        return TRUE;

      default:
        break;
    }

#if DBG
    /*
     * Nobody should be able to steal capture from modal menus.
     */
    if (!pMenuState->fModelessMenu
            && !pMenuState->fInDoDragDrop
            && !ExitMenuLoop (pMenuState, ppopupmenu) ) {

        UserAssert(PtiCurrent()->pq->QF_flags & QF_CAPTURELOCKED);
        UserAssert(PtiCurrent()->pq->spwndCapture == ppopupmenu->spwndNotify);
    }
#endif

    /*
     * We didn't handle this message
     */
    return FALSE;
}

/***************************************************************************\
* xxxEndMenuLoop
*
* Makes sure that the menu has been ended/canceled
*
* History:
* 10/25/96 GerardoB    Extracted from xxxMNLoop
\***************************************************************************/
void xxxEndMenuLoop (PMENUSTATE pMenuState, PPOPUPMENU ppopupmenu)
{

    UserAssert(IsRootPopupMenu(ppopupmenu));

    if (ppopupmenu->fIsTrackPopup) {
        if (!ppopupmenu->fInCancel) {
            xxxMNDismiss(pMenuState);
        }
    } else {
        if (pMenuState->fUnderline) {
            TL tlpwnd;
            ThreadLock(ppopupmenu->spwndNotify, &tlpwnd);
            xxxDrawMenuBarUnderlines(ppopupmenu->spwndNotify, FALSE);
            ThreadUnlock(&tlpwnd);
        }
        if (!pMenuState->fInEndMenu) {
            xxxEndMenu(pMenuState);
        }
    }
    /*
     * If this is a modeless menu, make sure that the notification
     *  window caption is drawn in the proper state
     */
    if (pMenuState->fModelessMenu && (ppopupmenu->spwndNotify != NULL)) {
        PWND pwndNotify = ppopupmenu->spwndNotify;
        PTHREADINFO pti = GETPTI(pwndNotify);
        BOOL fFrameOn = (pti->pq == gpqForeground)
                            && (pti->pq->spwndActive == pwndNotify);
        TL tlpwndNotify;

        if (fFrameOn ^ !!TestWF(pwndNotify, WFFRAMEON)) {
            ThreadLockAlways(pwndNotify, &tlpwndNotify);
            xxxDWP_DoNCActivate(pwndNotify,
                                (fFrameOn ? NCA_ACTIVE : NCA_FORCEFRAMEOFF),
                                HRGN_FULL);
            ThreadUnlock(&tlpwndNotify);

        }
    }
}
/***************************************************************************\
* xxxMenuLoop
*
* The menu processing entry point.
* assumes: pMenuState->spwndMenu is the window which is the owner of the menu
* we are processing.
*
* History:
\***************************************************************************/

int xxxMNLoop(
    PPOPUPMENU ppopupmenu,
    PMENUSTATE pMenuState,
    LPARAM lParam,
    BOOL fDblClk)
{
    int hit;
    MSG msg;
    BOOL fSendIdle = TRUE;
    BOOL fInQueue = FALSE;
    DWORD menuState;
    PTHREADINFO pti;
    TL tlpwndT;

    UserAssert(IsRootPopupMenu(ppopupmenu));

    pMenuState->fInsideMenuLoop = TRUE;
    pMenuState->cmdLast = 0;

    pti = PtiCurrent();

    pMenuState->ptMouseLast.x = pti->ptLast.x;
    pMenuState->ptMouseLast.y = pti->ptLast.y;

    /*
     * Set flag to false, so that we can track if windows have
     * been activated since entering this loop.
     */
    pti->pq->QF_flags &= ~QF_ACTIVATIONCHANGE;

    /*
     * Were we called from xxxMenuKeyFilter? If not, simulate a LBUTTONDOWN
     * message to bring up the popup.
     */
    if (!pMenuState->fMenuStarted) {
        if (_GetKeyState(((ppopupmenu->fRightButton) ?
                        VK_RBUTTON : VK_LBUTTON)) >= 0) {

            /*
             * We think the mouse button should be down but the call to get key
             * state says different so we need to get outta menu mode.  This
             * happens if clicking on the menu causes a sys modal message box to
             * come up before we can enter this stuff.  For example, run
             * winfile, click on drive a: to see its tree.  Activate some other
             * app, then open drive a: and activate winfile by clicking on the
             * menu.  This causes a sys modal msg box to come up just before
             * entering menu mode.  The user may have the mouse button up but
             * menu mode code thinks it is down...
             */

            /*
             * Need to notify the app we are exiting menu mode because we told
             * it we were entering menu mode just before entering this function
             * in xxxSysCommand()...
             */
            if (!ppopupmenu->fNoNotify) {
                ThreadLock(ppopupmenu->spwndNotify, &tlpwndT);
                xxxSendNotifyMessage(ppopupmenu->spwndNotify, WM_EXITMENULOOP,
                    ((ppopupmenu->fIsTrackPopup && !ppopupmenu->fIsSysMenu) ? TRUE : FALSE), 0);
                ThreadUnlock(&tlpwndT);
            }
            goto ExitMenuLoop;
        }

        /*
         * Simulate a WM_LBUTTONDOWN message.
         */
        if (!ppopupmenu->fIsTrackPopup) {

            /*
             * For TrackPopupMenus, we do it in the TrackPopupMenu function
             * itself so we don't want to do it again.
             */
            if (!xxxMNStartMenu(ppopupmenu, MOUSEHOLD)) {
                goto ExitMenuLoop;
            }
        }

        if ((ppopupmenu->fRightButton)) {
            msg.message = (fDblClk ? WM_RBUTTONDBLCLK : WM_RBUTTONDOWN);
            msg.wParam = MK_RBUTTON;
        } else {
            msg.message = (fDblClk ? WM_LBUTTONDBLCLK : WM_LBUTTONDOWN);
            msg.wParam = MK_LBUTTON;
        }
        msg.lParam = lParam;
        msg.hwnd = HW(ppopupmenu->spwndPopupMenu);
        xxxHandleMenuMessages(&msg, pMenuState, ppopupmenu);
     }

    /*
     * If this is a modeless menu, release capture, mark it in the menu state
     *   and return. Decrement foreground lock count.
     */
     if (pMenuState->fModelessMenu) {
         xxxMNReleaseCapture();

         DecSFWLockCount();
         DBGDecModalMenuCount();
         return 0;
     }

    while (pMenuState->fInsideMenuLoop) {

        /*
         * Is a message waiting for us?
         */
        BOOL fPeek = xxxPeekMessage(&msg, NULL, 0, 0, PM_NOYIELD | PM_NOREMOVE);

        Validateppopupmenu(ppopupmenu);

        if (fPeek) {
            /*
             * Bail if we have been forced out of menu loop
             */
            if (ExitMenuLoop (pMenuState, ppopupmenu)) {
                goto ExitMenuLoop;
            }

            /*
             * Since we could have blocked in xxxWaitMessage (see last line
             * of loop) or xxxPeekMessage, reset the cached copy of
             * ptiCurrent()->pq: It could have changed if someone did a
             * DetachThreadInput() while we were away.
             */
            if ((!ppopupmenu->fIsTrackPopup &&
                    pti->pq->spwndActive != ppopupmenu->spwndNotify &&
                    ((pti->pq->spwndActive == NULL) || !_IsChild(pti->pq->spwndActive, ppopupmenu->spwndNotify)))) {

                /*
                 * End menu processing if we are no longer the active window.
                 * This is needed in case a system modal dialog box pops up
                 * while we are tracking the menu code for example.  It also
                 * helps out Tracer if a macro is executed while a menu is down.
                 */

                /*
                 * Also, end menu processing if we think the mouse button is
                 * down but it really isn't.  (Happens if a sys modal dialog int
                 * time dlg box comes up while we are in menu mode.)
                 */

                goto ExitMenuLoop;
            }

            if (ppopupmenu->fIsMenuBar && msg.message == WM_LBUTTONDBLCLK) {

                /*
                 * Was the double click on the system menu or caption?
                 */
                hit = FindNCHit(ppopupmenu->spwndNotify, (LONG)msg.lParam);
                if (hit == HTCAPTION) {
                    PWND pwnd;
                    PMENU pmenu;

                    /*
                     * Get the message out of the queue since we're gonna
                     * process it.
                     */
                    xxxPeekMessage(&msg, NULL, msg.message, msg.message, PM_REMOVE);
                    if (ExitMenuLoop (pMenuState, ppopupmenu)) {
                        goto ExitMenuLoop;
                    } else {
                        pwnd = ppopupmenu->spwndNotify;
                        ThreadLockAlways(pwnd, &tlpwndT);
                        pmenu = xxxGetSysMenuHandle(pwnd);
                        UserAssert(pwnd == ppopupmenu->spwndNotify);

                        menuState = _GetMenuState(pmenu, SC_RESTORE & 0x0000FFF0,
                                MF_BYCOMMAND);

                        /*
                         * Only send the sys command if the item is valid.  If
                         * the item doesn't exist or is disabled, then don't
                         * post the syscommand.  Note that for win2 apps, we
                         * always send the sys command if it is a child window.
                         * This is so hosebag apps can change the sys menu.
                         */
                        if (!(menuState & MFS_GRAYED)) {
                            _PostMessage(pwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
                        }

                        /*
                         * Get out of menu mode.
                         */
                        ThreadUnlock(&tlpwndT);
                        goto ExitMenuLoop;
                    }
                }
            }

            fInQueue = (msg.message == WM_LBUTTONDOWN ||
                        msg.message == WM_RBUTTONDOWN ||
                        msg.message == WM_NCLBUTTONDOWN ||
                        msg.message == WM_NCRBUTTONDOWN);

            if (!fInQueue) {

                /*
                 * Note that we call xxxPeekMessage() with the filter
                 * set to the message we got from xxxPeekMessage() rather
                 * than simply 0, 0.  This prevents problems when
                 * xxxPeekMessage() returns something like a WM_TIMER,
                 * and after we get here to remove it a WM_LBUTTONDOWN,
                 * or some higher-priority input message, gets in the
                 * queue and gets removed accidently.  Basically we want
                 * to be sure we remove the right message in this case.
                 * NT bug 3852 was caused by this problem.
                 * Set the TIF_IGNOREPLAYBACKDELAY bit in case journal playback
                 * is happening: this allows us to proceed even if the hookproc
                 * incorrectly returns a delay now.  The bit will be cleared if
                 * this happens, so we can see why the Peek-Remove below fails.
                 * Lotus' Freelance Graphics tutorial does such bad journalling
                 */

                pti->TIF_flags |= TIF_IGNOREPLAYBACKDELAY;
                if (!xxxPeekMessage(&msg, NULL, msg.message, msg.message, PM_REMOVE)) {
                    if (pti->TIF_flags & TIF_IGNOREPLAYBACKDELAY) {
                        pti->TIF_flags &= ~TIF_IGNOREPLAYBACKDELAY;
                        /*
                         * It wasn't a bad journal playback: something else
                         * made the previously peeked message disappear before
                         * we could peek it again to remove it.
                         */
                        RIPMSG1(RIP_WARNING, "Disappearing msg 0x%08lx", msg.message);
                        goto NoMsg;
                    }
                }
                pti->TIF_flags &= ~TIF_IGNOREPLAYBACKDELAY;
            }

            if (!_CallMsgFilter(&msg, MSGF_MENU)) {
                if (!xxxHandleMenuMessages(&msg, pMenuState, ppopupmenu)) {
                    xxxTranslateMessage(&msg, 0);
                    xxxDispatchMessage(&msg);
                }

                Validateppopupmenu(ppopupmenu);

                if (ExitMenuLoop (pMenuState, ppopupmenu)) {
                    goto ExitMenuLoop;
                }

                if (pti->pq->QF_flags & QF_ACTIVATIONCHANGE) {

                    /*
                     * Run away and exit menu mode if another window has become
                     * active while a menu was up.
                     */
                    RIPMSG0(RIP_WARNING, "Exiting menu mode: another window activated");
                    goto ExitMenuLoop;
                }

#if DBG
                /*
                 * Nobody should be able to still capture from us.
                 */
                if (!pMenuState->fInDoDragDrop) {
                    UserAssert(pti->pq->QF_flags & QF_CAPTURELOCKED);
                    UserAssert(pti->pq->spwndCapture == ppopupmenu->spwndNotify);
                }
#endif

                /*
                 * If we get a system timer, then it's like we're idle
                 */
                if (msg.message == WM_SYSTIMER) {
                    goto NoMsg;
                }

                /*
                 * Don't set fSendIdle if we got these messages
                 */
                if ((msg.message == WM_TIMER) || (msg.message == WM_PAINT)) {
                    continue;
                }

            } else {
                if (fInQueue)
                    xxxPeekMessage(&msg, NULL, msg.message, msg.message,
                            PM_REMOVE);
            }

            /*
             * Reenable WM_ENTERIDLE messages.
             */
            fSendIdle = TRUE;

        } else {
NoMsg:
            /*
             * Bail if we have been forced out of menu loop
             */
            if (ExitMenuLoop (pMenuState, ppopupmenu)) {
                goto ExitMenuLoop;
            }

            UserAssert((ppopupmenu->spwndActivePopup == NULL)
                    || (TestWF(ppopupmenu->spwndActivePopup, WFVISIBLE)));


            /*
             * If a hierarchical popup has been destroyed, this is a
             *  good time to flush ppmDelayedFree
             */
            if (ppopupmenu->fFlushDelayedFree) {
                MNFlushDestroyedPopups (ppopupmenu, FALSE);
                ppopupmenu->fFlushDelayedFree = FALSE;
            }

            /*
             * We need to send the WM_ENTERIDLE message only the first time
             * there are no messages for us to process.  Subsequent times we
             * need to yield via WaitMessage().  This will allow other tasks to
             * get some time while we have a menu down.
             */
            if (fSendIdle) {
                if (ppopupmenu->spwndNotify != NULL) {
                    ThreadLockAlways(ppopupmenu->spwndNotify, &tlpwndT);
                    xxxSendMessage(ppopupmenu->spwndNotify, WM_ENTERIDLE, MSGF_MENU,
                        (LPARAM)HW(ppopupmenu->spwndActivePopup));
                    ThreadUnlock(&tlpwndT);
                }
                fSendIdle = FALSE;
            } else {
                /*
                 * If we're animating, sleep only 1 ms to reduce the chance
                 *  of jerky animation.
                 * When not animating, this is the same as a xxxWaitMessage call
                 */
                xxxSleepThread(QS_ALLINPUT | QS_EVENT, (pMenuState->hdcWndAni != NULL), TRUE);
            }

        } /* if (PeekMessage(&msg, NULL, 0, 0, PM_NOYIELD)) else */

    } /* end while (fInsideMenuLoop) */



ExitMenuLoop:
    pMenuState->fInsideMenuLoop = FALSE;
    pMenuState->fModelessMenu = FALSE;

    /*
     * Make sure that the menu has been ended/canceled
     */
    xxxEndMenuLoop (pMenuState, ppopupmenu);

    xxxMNReleaseCapture();

    // Throw in an extra peek here when we exit the menu loop to ensure that the input queue
    // for this thread gets unlocked if there is no more input left for him.
    xxxPeekMessage(&msg, NULL, WM_MOUSEMOVE, WM_MOUSEMOVE, PM_NOYIELD | PM_NOREMOVE);
    return(pMenuState->cmdLast);
} /* xxxMenuLoop() */
