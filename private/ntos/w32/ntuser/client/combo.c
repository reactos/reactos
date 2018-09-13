/**************************** Module Header ********************************\
* Module Name: combo.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* The WndProc for combo boxes and other often used combo routines
*
* History:
* ??-???-???? ??????    Ported from Win 3.0 sources
* 01-Feb-1991 mikeke    Added Revalidation code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

LOOKASIDE ComboboxLookaside;

BOOL NtUserTrackMouseEvent(TRACKMOUSEEVENT *ptme);
LONG xxxCBGetTextLengthHelper(PCBOX pcbox, BOOL fAnsi);
LONG xxxCBGetTextHelper(PCBOX pcbox, int len, LPWSTR lpstr, BOOL fAnsi);

/***************************************************************************\
*
*  PressButton()
*
*  Pops combobox button back up.
*
\***************************************************************************/
void xxxPressButton(PCBOX pcbox, BOOL fPress)
{
    //
    // Publisher relies on getting a WM_PAINT message after the combo list
    // pops back up.  On a WM_PAINT they change the focus, which causes
    // toolbar combos to send CBN_SELENDCANCEL notifications.  On this
    // notification they apply the font/pt size change you made to the
    // selection.
    //
    // This happened in 3.1 because the dropdown list overlapped the button
    // on the bottom or top by a pixel.  Since we'd end up painting under
    // the list SPB, when it went away USER would reinvalidate the dirty
    // area.  This would cause a paint message.
    //
    // In 4.0, this doesn't happen because the dropdown doesn't overlap.  So
    // we need to make sure Publisher gets a WM_PAINT anyway.  We do this
    // by changing where the dropdown shows up for 3.x apps
    //
    //

    if ((pcbox->fButtonPressed != 0) != (fPress != 0)) {

        HWND hwnd = HWq(pcbox->spwnd);

        pcbox->fButtonPressed = (fPress != 0);
        if (pcbox->f3DCombo)
            NtUserInvalidateRect(hwnd, &pcbox->buttonrc, TRUE);
        else
        {
            RECT    rc;

            CopyRect(&rc, &pcbox->buttonrc);
            InflateRect(&rc, 0, SYSMET(CYEDGE));
            NtUserInvalidateRect(hwnd, &rc, TRUE);
        }
        UpdateWindow(hwnd);

        if (FWINABLE()) {
            NotifyWinEvent(EVENT_OBJECT_STATECHANGE, hwnd, OBJID_CLIENT, INDEX_COMBOBOX_BUTTON);
        }
    }
}

/***************************************************************************\
* HotTrack
*
* If we're not already hot-tracking and the mouse is over the combobox,
* turn on hot-tracking and invalidate the drop-down button.
*
\***************************************************************************/

#ifdef COLOR_HOTTRACKING

void HotTrack(PCBOX pcbox)
{
    if (!pcbox->fButtonHotTracked && !pcbox->fMouseDown) {
        HWND hwnd = HWq(pcbox->spwnd);
        TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd, 0};
        if (NtUserTrackMouseEvent(&tme)) {
            pcbox->fButtonHotTracked = TRUE;
            NtUserInvalidateRect(hwnd, &pcbox->buttonrc, TRUE);
        }
    }
}

#endif // COLOR_HOTTRACKING

/***************************************************************************\
* xxxComboBoxDBCharHandler
*
* Double Byte character handler for ANSI ComboBox
*
* History:
\***************************************************************************/

LRESULT ComboBoxDBCharHandler(
    PCBOX pcbox,
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    WORD w;
    PWND pwndSend;

    w = DbcsCombine(hwnd, (BYTE)wParam);
    if (w == 0) {
        return CB_ERR;  // Failed to assemble DBCS
    }

    UserAssert(pcbox->spwndList);
    if (pcbox->fNoEdit) {
        pwndSend = pcbox->spwndList;
    } else if (pcbox->spwndEdit) {
        RIPMSG1(RIP_WARNING, "ComboBoxWndProcWorker: WM_CHAR is posted to Combobox itself(%08x).",
                hwnd);
        pwndSend = pcbox->spwndEdit;
    } else {
        return CB_ERR;
    }

    RIPMSG1(RIP_VERBOSE, "ComboBoxWndProcWorker: sending WM_CHAR %04x", w);

    if (!TestWF(pwndSend, WFANSIPROC)) {
        //
        // If receiver is not ANSI WndProc (may be subclassed?),
        // send a UNICODE message.
        //
        WCHAR wChar;
        LPWSTR lpwstr = &wChar;

        if (MBToWCSEx(THREAD_CODEPAGE(), (LPCSTR)&w, 2, &lpwstr, 1, FALSE) == 0) {
            RIPMSG1(RIP_WARNING, "ComboBoxWndProcWorker: cannot convert 0x%04x to UNICODE.", w);
            return CB_ERR;
        }
        return SendMessageWorker(pwndSend, message, wChar, lParam, FALSE);
    }

    /*
     * Post the Trailing byte to the target
     * so that they can peek the second WM_CHAR
     * message later.
     * Note: it's safe since sender is A and receiver is A,
     * translation layer does not perform any DBCS combining and cracking.
     */
    PostMessageA(HWq(pwndSend), message, CrackCombinedDbcsTB(w), lParam);
    return SendMessageWorker(pwndSend, message, wParam, lParam, TRUE);
}

BOOL ComboBoxMsgOKInInit(UINT message, LRESULT* plRet)
{
    switch (message) {
    default:
        break;
    case WM_SIZE:
        *plRet = 0;
        return FALSE;
    case WM_STYLECHANGED:
    case WM_GETTEXT:
    case WM_GETTEXTLENGTH:
    case WM_PRINT:
    case WM_COMMAND:
    case CBEC_KILLCOMBOFOCUS:
    case WM_PRINTCLIENT:
    case WM_SETFONT:
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
    case WM_CHAR:
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
    case WM_MOUSEWHEEL:
    case WM_CAPTURECHANGED:
    case WM_LBUTTONUP:
    case WM_MOUSEMOVE:
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
    case WM_SETREDRAW:
    case WM_ENABLE:
    case CB_SETDROPPEDWIDTH:
    case CB_DIR:
    case CB_ADDSTRING:
        /*
         * Cannot handle those messages yet. Bail out.
         */
        *plRet = CB_ERR;
        return FALSE;
    }
    return TRUE;
}

/***************************************************************************\
* xxxComboBoxCtlWndProc
*
* Class procedure for all combo boxes
*
* History:
\***************************************************************************/

LRESULT APIENTRY ComboBoxWndProcWorker(
    PWND pwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    DWORD fAnsi)
{
    HWND hwnd = HWq(pwnd);
    PCBOX pcbox;
    POINT pt;
    TL tlpwndEdit;
    TL tlpwndList;
    PAINTSTRUCT ps;
    LPWSTR lpwsz = NULL;
    LRESULT lReturn;
    static BOOL fInit = TRUE;
    int  i;

    CheckLock(pwnd);

    VALIDATECLASSANDSIZE(pwnd, FNID_COMBOBOX);
    INITCONTROLLOOKASIDE(&ComboboxLookaside, CBOX, spwnd, 8);

    /*
     * Get the pcbox for the given window now since we will use it a lot in
     * various handlers.  This is stored by NtUserSetWindowLongPtr() in the
     * INITCONTROLLOOKASIDE macro above.
     */
    pcbox = ((PCOMBOWND)pwnd)->pcbox;

    /*
     * Protect the combobox during the initialization.
     */
    if (pcbox->spwndList == NULL) {
        LRESULT lRet;

        if (!ComboBoxMsgOKInInit(message, &lRet)) {
            RIPMSG2(RIP_WARNING, "ComboBoxWndProcWorker: msg=%04x is sent to hwnd=%08x in the middle of initialization.",
                    message, hwnd);
            return lRet;
        }
    }

    /*
     * Dispatch the various messages we can receive
     */
    switch (message) {
    case CBEC_KILLCOMBOFOCUS:

        /*
         * Private message coming from editcontrol informing us that the combo
         * box is losing the focus to a window which isn't in this combo box.
         */
        xxxCBKillFocusHelper(pcbox);
        break;

    case WM_COMMAND:

        /*
         * So that we can handle notification messages from the listbox and
         * edit control.
         */
        return xxxCBCommandHandler(pcbox, (DWORD)wParam, (HWND)lParam);

    case WM_STYLECHANGED:
        UserAssert(pcbox->spwndList != NULL);
        {
            LONG OldStyle;
            LONG NewStyle = 0;

            pcbox->fRtoLReading = (TestWF(pwnd, WEFRTLREADING) != 0);
            pcbox->fRightAlign  = (TestWF(pwnd, WEFRIGHT) != 0);
            if (pcbox->fRtoLReading)
                NewStyle |= (WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR);
            if (pcbox->fRightAlign)
                NewStyle |= WS_EX_RIGHT;

            ThreadLock(pcbox->spwndList, &tlpwndList);
            OldStyle = GetWindowLong(HWq(pcbox->spwndList), GWL_EXSTYLE) & ~(WS_EX_RIGHT|WS_EX_RTLREADING|WS_EX_LEFTSCROLLBAR);
            SetWindowLong(HWq(pcbox->spwndList), GWL_EXSTYLE, OldStyle|NewStyle);
            ThreadUnlock(&tlpwndList);

            if (!pcbox->fNoEdit && pcbox->spwndEdit) {
                ThreadLock(pcbox->spwndEdit, &tlpwndEdit);
                OldStyle = GetWindowLong(HWq(pcbox->spwndEdit), GWL_EXSTYLE) & ~(WS_EX_RIGHT|WS_EX_RTLREADING|WS_EX_LEFTSCROLLBAR);
                SetWindowLong(HWq(pcbox->spwndEdit), GWL_EXSTYLE, OldStyle|NewStyle);
                ThreadUnlock(&tlpwndEdit);
            }
            xxxCBPosition(pcbox);
            NtUserInvalidateRect(hwnd, NULL, FALSE);
        }
        break;

    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOR:
        //
        // Causes compatibility problems for 3.X apps.  Forward only
        // for 4.0
        //
        if (TestWF(pwnd, WFWIN40COMPAT)) {
            TL tlpwndParent;
            LRESULT ret;
            PWND pwndParent;

            pwndParent = REBASEPWND(pwnd, spwndParent);
            ThreadLock(pwndParent, &tlpwndParent);
            ret = SendMessage(HW(pwndParent), message, wParam, lParam);
            ThreadUnlock(tlpwndParent);
            return ret;
        } else
            return(DefWindowProcWorker(pwnd, message, wParam, lParam, fAnsi));
        break;

    case WM_GETTEXT:
        if (pcbox->fNoEdit) {
            return xxxCBGetTextHelper(pcbox, (int)wParam, (LPWSTR)lParam, fAnsi);
        }
        goto CallEditSendMessage;
        break;

    case WM_GETTEXTLENGTH:

        /*
         * If the is not edit control, CBS_DROPDOWNLIST, then we have to
         * ask the list box for the size
         */

        if (pcbox->fNoEdit) {
            return xxxCBGetTextLengthHelper(pcbox, fAnsi);
        }

        // FALL THROUGH

    case WM_CLEAR:
    case WM_CUT:
    case WM_PASTE:
    case WM_COPY:
    case WM_SETTEXT:
        goto CallEditSendMessage;
        break;

    case WM_CREATE:

        /*
         * wParam - not used
         * lParam - Points to the CREATESTRUCT data structure for the window.
         */
        return xxxCBCreateHandler(pcbox, pwnd);

    case WM_ERASEBKGND:

        /*
         * Just return 1L so that the background isn't erased
         */
        return 1L;

    case WM_GETFONT:
        return (LRESULT)pcbox->hFont;

    case WM_PRINT:
        if (!DefWindowProcWorker(pwnd, message, wParam, lParam, fAnsi))
            return(FALSE);

        if ((lParam & PRF_OWNED) && (pcbox->CBoxStyle & SDROPPABLE) &&
            TestWF(pcbox->spwndList, WFVISIBLE)) {
            TL tpwndList;
            int iDC = SaveDC((HDC) wParam);
            OffsetWindowOrgEx((HDC) wParam, 0, pwnd->rcWindow.top - pcbox->spwndList->rcWindow.top, NULL);
            lParam &= ~PRF_CHECKVISIBLE;
            ThreadLock(pcbox->spwndList, &tpwndList);
            SendMessageWorker(pcbox->spwndList, WM_PRINT, wParam, lParam, FALSE);
            RestoreDC((HDC) wParam, iDC);
        }
        return TRUE;

    case WM_PRINTCLIENT:
        xxxCBPaint(pcbox, (HDC) wParam);
        break;

    case WM_PAINT: {
        HDC hdc;

        /*
         * wParam - perhaps a hdc
         */
        hdc = (wParam) ? (HDC) wParam : NtUserBeginPaint(hwnd, &ps);

        if (IsComboVisible(pcbox))
            xxxCBPaint(pcbox, hdc);

        if (!wParam)
            NtUserEndPaint(hwnd, &ps);
        break;
    }
    case WM_GETDLGCODE:

        /*
         * wParam - not used
         * lParam - not used
         */
        {
            LRESULT code = DLGC_WANTCHARS | DLGC_WANTARROWS;

            // If the listbox is dropped and the ENTER key is pressed,
            // we want this message so we can close up the listbox
            if ((lParam != 0) &&
                (((LPMSG)lParam)->message == WM_KEYDOWN) &&
                pcbox->fLBoxVisible &&
                ((wParam == VK_RETURN) || (wParam == VK_ESCAPE)))
            {
                code |= DLGC_WANTMESSAGE;
            }
            return code;
        }
        /*
         * No fall through
         */

    case WM_SETFONT:
        xxxCBSetFontHandler(pcbox, (HANDLE)wParam, LOWORD(lParam));
        break;

    case WM_SYSKEYDOWN:
        if (lParam & 0x20000000L)  /* Check if the alt key is down */ {

            /*
             * Handle Combobox support.  We want alt up or down arrow to behave
             * like F4 key which completes the combo box selection
             */
            if (lParam & 0x1000000) {

                /*
                 * This is an extended key such as the arrow keys not on the
                 * numeric keypad so just drop the combobox.
                 */
                if (wParam == VK_DOWN || wParam == VK_UP)
                    goto DropCombo;

                goto CallDWP;
            }

            if (GetKeyState(VK_NUMLOCK) & 0x1) {
                /*
                 * If numlock down, just send all system keys to dwp
                 */
                goto CallDWP;
            } else {

                /*
                 * We just want to ignore keys on the number pad...
                 */
                if (!(wParam == VK_DOWN || wParam == VK_UP))
                    goto CallDWP;
            }
DropCombo:
            if (!pcbox->fLBoxVisible) {

                /*
                 * If the listbox isn't visible, just show it
                 */
                xxxCBShowListBoxWindow(pcbox, TRUE);
            } else {

                /*
                 * Ok, the listbox is visible.  So hide the listbox window.
                 */
                if (!xxxCBHideListBoxWindow(pcbox, TRUE, TRUE))
                    return(0L);
            }
        }
        goto CallDWP;
        break;

    case WM_KEYDOWN:
        /*
         * If the listbox is dropped and the ENTER key is pressed,
         * close up the listbox successfully.  If ESCAPE is pressed,
         * close it up like cancel.
         */
        if (pcbox->fLBoxVisible) {
            if ((wParam == VK_RETURN) || (wParam == VK_ESCAPE)) {
                xxxCBHideListBoxWindow(pcbox, TRUE, (wParam != VK_ESCAPE));
                break;
            }
        }
        // FALL THROUGH

    case WM_CHAR:
        if (fAnsi && IS_DBCS_ENABLED() && IsDBCSLeadByteEx(THREAD_CODEPAGE(), (BYTE)wParam)) {
            return ComboBoxDBCharHandler(pcbox, hwnd, message, wParam, lParam);
        }

        if (pcbox->fNoEdit) {
            goto CallListSendMessage;
        }
        else
            goto CallEditSendMessage;
        break;

    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:

#ifdef COLOR_HOTTRACKING
        pcbox->fButtonHotTracked = FALSE;
#endif // COLOR_HOTTRACKING

        /*
         * Set the focus to the combo box if we get a mouse click on it.
         */
        if (!pcbox->fFocus) {
            NtUserSetFocus(hwnd);
            if (!pcbox->fFocus) {

                /*
                 * Don't do anything if we still don't have the focus.
                 */
                break;
            }
        }

        /*
         * If user clicked in button rect and we are a combobox with edit, then
         * drop the listbox.  (The button rect is 0 if there is no button so the
         * ptinrect will return false.) If a drop down list (no edit), clicking
         * anywhere on the face causes the list to drop.
         */

        POINTSTOPOINT(pt, lParam);
        if ((pcbox->CBoxStyle == SDROPDOWN &&
                PtInRect(&pcbox->buttonrc, pt)) ||
                pcbox->CBoxStyle == SDROPDOWNLIST) {

            /*
             * Set the fMouseDown flag so that we can handle clicking on
             * the popdown button and dragging into the listbox (when it just
             * dropped down) to make a selection.
             */
            pcbox->fButtonPressed = TRUE;
            if (pcbox->fLBoxVisible) {
                if (pcbox->fMouseDown) {
                    pcbox->fMouseDown = FALSE;
                    NtUserReleaseCapture();
                }
                xxxPressButton(pcbox, FALSE);

                if (!xxxCBHideListBoxWindow(pcbox, TRUE, TRUE))
                    return(0L);
            } else {
                xxxCBShowListBoxWindow(pcbox, FALSE);

                // Setting and resetting this flag must always be followed
                // imediately by SetCapture or ReleaseCapture
                //
                pcbox->fMouseDown = TRUE;
                NtUserSetCapture(hwnd);
                if (FWINABLE()) {
                    NotifyWinEvent(EVENT_OBJECT_STATECHANGE, hwnd, OBJID_CLIENT, INDEX_COMBOBOX_BUTTON);
                }
            }
        }
        break;

    case WM_MOUSEWHEEL:
        /*
         * Handle only scrolling.
         */
        if (wParam & (MK_CONTROL | MK_SHIFT))
            goto CallDWP;

        /*
         * If the listbox is visible, send it the message to scroll.
         */
        if (pcbox->fLBoxVisible)
            goto CallListSendMessage;

        /*
         * If we're in extended UI mode or the edit control isn't yet created,
         * bail.
         */
        if (pcbox->fExtendedUI || pcbox->spwndEdit == NULL)
            return TRUE;

        /*
         * Emulate arrow up/down messages to the edit control.
         */
        i = abs(((short)HIWORD(wParam))/WHEEL_DELTA);
        wParam = ((short)HIWORD(wParam) > 0) ? VK_UP : VK_DOWN;

        ThreadLock(pcbox->spwndEdit, &tlpwndEdit);
        while (i-- > 0) {
            SendMessageWorker(
                    pcbox->spwndEdit, WM_KEYDOWN, wParam, 0, fAnsi);
        }
        ThreadUnlock(&tlpwndEdit);
        return TRUE;

    case WM_CAPTURECHANGED:
        if (!(TestWF(pwnd, WFWIN40COMPAT)))
            return 0;

        if ((pcbox->fMouseDown)) {
            pcbox->fMouseDown = FALSE;
            xxxPressButton(pcbox, FALSE);

            //
            // Pop combo listbox back up, canceling.
            //
            if (pcbox->fLBoxVisible)
                xxxCBHideListBoxWindow(pcbox, TRUE, FALSE);
        }
        break;

    case WM_LBUTTONUP:
        xxxPressButton(pcbox, FALSE);

        /*
         * Clear this flag so that mouse moves aren't sent to the listbox
         */
        if (pcbox->fMouseDown) {
            pcbox->fMouseDown = FALSE;

            if (pcbox->CBoxStyle == SDROPDOWN) {
                // If an item in the listbox matches the text in the edit
                // control, scroll it to the top of the listbox. Select the
                // item only if the mouse button isn't down otherwise we
                // will select the item when the mouse button goes up.
                xxxCBUpdateListBoxWindow(pcbox, TRUE);
                xxxCBCompleteEditWindow(pcbox);
            }
            NtUserReleaseCapture();

            // Now, we want listbox to track mouse moves while mouse up
            // until mouse down, and select items as though they were
            // clicked on.
            if (TestWF(pwnd, WFWIN40COMPAT)) {

                ThreadLock(pcbox->spwndList, &tlpwndList);
                SendMessageWorker(pcbox->spwndList, LBCB_STARTTRACK, FALSE, 0, FALSE);
                ThreadUnlock(&tlpwndList);
            }
        }
#ifdef COLOR_HOTTRACKING
        HotTrack(pcbox);
        break;

    case WM_MOUSELEAVE:
        pcbox->fButtonHotTracked = FALSE;
        NtUserInvalidateRect(hwnd, &pcbox->buttonrc, TRUE);
#endif // COLOR_HOTTRACKING
        break;

    case WM_MOUSEMOVE:
        if (pcbox->fMouseDown) {
            POINTSTOPOINT(pt, lParam);

            // Note conversion of INT bit field to BOOL (1 or 0)

            if (PtInRect(&pcbox->buttonrc, pt) != !!pcbox->fButtonPressed) {
                xxxPressButton(pcbox, (pcbox->fButtonPressed == 0));
            }

            _ClientToScreen(pwnd, &pt);
            if (PtInRect(&pcbox->spwndList->rcClient, pt)) {

                /*
                 * This handles dropdown comboboxes/listboxes so that clicking
                 * on the dropdown button and dragging into the listbox window
                 * will let the user make a listbox selection.
                 */
                pcbox->fMouseDown = FALSE;
                NtUserReleaseCapture();

                if (pcbox->CBoxStyle & SEDITABLE) {

                    /*
                     * If an item in the listbox matches the text in the edit
                     * control, scroll it to the top of the listbox.  Select the
                     * item only if the mouse button isn't down otherwise we
                     * will select the item when the mouse button goes up.
                     */

                    /*
                     * We need to select the item which matches the editcontrol
                     * so that if the user drags out of the listbox, we don't
                     * cancel back to his origonal selection
                     */
                    xxxCBUpdateListBoxWindow(pcbox, TRUE);
                }

                /*
                 * Convert point to listbox coordinates and send a buttondown
                 * message to the listbox window.
                 */
                _ScreenToClient(pcbox->spwndList, &pt);
                lParam = POINTTOPOINTS(pt);
                message = WM_LBUTTONDOWN;
                goto CallListSendMessage;
            }
        }
#ifdef COLOR_HOTTRACKING
        HotTrack(pcbox);
#endif // COLOR_HOTTRACKING
        break;

    case WM_NCDESTROY:
    case WM_FINALDESTROY:
        xxxCBNcDestroyHandler(pwnd, pcbox);
        break;

    case WM_SETFOCUS:
        if (pcbox->fNoEdit) {

            /*
             * There is no editcontrol so set the focus to the combo box itself.
             */
            xxxCBGetFocusHelper(pcbox);
        } else if (pcbox->spwndEdit) {
            /*
             * Set the focus to the edit control window if there is one
             */
            ThreadLock(pcbox->spwndEdit, &tlpwndEdit);
            NtUserSetFocus(HWq(pcbox->spwndEdit));
            ThreadUnlock(&tlpwndEdit);
        }
        break;

    case WM_KILLFOCUS:

        /*
         * wParam has the new focus hwnd
         */
        if (wParam != 0)
            wParam = (WPARAM)ValidateHwnd((HWND)wParam);
        if ((wParam == 0) || !_IsChild(pwnd, (PWND)wParam)) {

            /*
             * We only give up the focus if the new window getting the focus
             * doesn't belong to the combo box.
             */
            xxxCBKillFocusHelper(pcbox);
        }

        UserAssert(pcbox->spwndList);
        {
            PLBIV plb = ((PLBWND)pcbox->spwndList)->pLBIV;

            if ((plb != NULL) && (plb != (PLBIV)-1)) {
                plb->iTypeSearch = 0;
                if (plb->pszTypeSearch) {
                    UserLocalFree(plb->pszTypeSearch);
                    plb->pszTypeSearch = NULL;
                }
            }
        }
        break;

    case WM_SETREDRAW:

        /*
         * wParam - specifies state of the redraw flag.  nonzero = redraw
         * lParam - not used
         */

        /*
         * effects: Sets the state of the redraw flag for this combo box
         * and its children.
         */
        pcbox->fNoRedraw = (UINT)!((BOOL)wParam);

        /*
         * Must check pcbox->spwnEdit in case we get this message before
         * WM_CREATE - PCBOX won't be initialized yet. (Eudora does this)
         */
        if (!pcbox->fNoEdit && pcbox->spwndEdit) {
            ThreadLock(pcbox->spwndEdit, &tlpwndEdit);
            SendMessageWorker(pcbox->spwndEdit, message, wParam, lParam, FALSE);
            ThreadUnlock(&tlpwndEdit);
        }
        goto CallListSendMessage;
        break;

    case WM_ENABLE:

        /*
         * Invalidate the rect to cause it to be drawn in grey for its
         * disabled view or ungreyed for non-disabled view.
         */
        NtUserInvalidateRect(hwnd, NULL, FALSE);
        if ((pcbox->CBoxStyle & SEDITABLE) && pcbox->spwndEdit) {

            /*
             * Enable/disable the edit control window
             */
            ThreadLock(pcbox->spwndEdit, &tlpwndEdit);
            NtUserEnableWindow(HWq(pcbox->spwndEdit), (TestWF(pwnd, WFDISABLED) == 0));
            ThreadUnlock(&tlpwndEdit);
        }

        /*
         * Enable/disable the listbox window
         */
        UserAssert(pcbox->spwndList);
        ThreadLock(pcbox->spwndList, &tlpwndList);
        NtUserEnableWindow(HWq(pcbox->spwndList), (TestWF(pwnd, WFDISABLED) == 0));
        ThreadUnlock(&tlpwndList);
      break;

    case WM_SIZE:

        /*
         * wParam - defines the type of resizing fullscreen, sizeiconic,
         *          sizenormal etc.
         * lParam - new width in LOWORD, new height in HIGHUINT of client area
         */
        UserAssert(pcbox->spwndList);
        if (LOWORD(lParam) == 0 || HIWORD(lParam) == 0) {

            /*
             * If being sized to a zero width or to a zero height or we aren't
             * fully initialized, just return.
             */
            return 0;
        }

        // OPTIMIZATIONS -- first check if old and new widths are the same
        if (pcbox->cxCombo == pwnd->rcWindow.right - pwnd->rcWindow.left) {
            int iNewHeight = pwnd->rcWindow.bottom - pwnd->rcWindow.top;

            // now check if new height is the dropped down height
            if (pcbox->fLBoxVisible) {
                // Check if new height is the full size height
                if (pcbox->cyDrop + pcbox->cyCombo == iNewHeight)
                    return(0L);
            } else {
                // Check if new height is the closed up height
                if (pcbox->cyCombo == iNewHeight)
                    return(0L);
            }
        }

        xxxCBSizeHandler(pcbox);
        break;

    case CB_GETDROPPEDSTATE:

        /*
         * returns 1 if combo is dropped down else 0
         * wParam - not used
         * lParam - not used
         */
        return pcbox->fLBoxVisible;

    case CB_GETDROPPEDCONTROLRECT:

        /*
         * wParam - not used
         * lParam - lpRect which will get the dropped down window rect in
         *          screen coordinates.
         */
        ((LPRECT)lParam)->left      = pwnd->rcWindow.left;
        ((LPRECT)lParam)->top       = pwnd->rcWindow.top;
        ((LPRECT)lParam)->right     = pwnd->rcWindow.left + max(pcbox->cxDrop, pcbox->cxCombo);
        ((LPRECT)lParam)->bottom    = pwnd->rcWindow.top + pcbox->cyCombo + pcbox->cyDrop;
        break;

    case CB_SETDROPPEDWIDTH:
        if (pcbox->CBoxStyle & SDROPPABLE) {
            if (wParam) {
                wParam = max(wParam, (UINT)pcbox->cxCombo);

                if (wParam != (UINT) pcbox->cxDrop)
                {
                    pcbox->cxDrop = (int)wParam;
                    xxxCBPosition(pcbox);
                }
            }
        }
        // fall thru

    case CB_GETDROPPEDWIDTH:
        if (pcbox->CBoxStyle & SDROPPABLE)
            return((LRESULT) max(pcbox->cxDrop, pcbox->cxCombo));
        else
            return(CB_ERR);
        break;

    case CB_DIR:
        /*
         * wParam - Dos attribute value.
         * lParam - Points to a file specification string
         */
        if (fAnsi && lParam != 0) {
            if (MBToWCS((LPSTR)lParam, -1, &lpwsz, -1, TRUE) == 0)
                return CB_ERR;
            lParam = (LPARAM)lpwsz;
        }
        lReturn = xxxCBDir(pcbox, LOWORD(wParam), (LPWSTR)lParam);
        if (fAnsi && lParam != 0) {
            UserLocalFree(lpwsz);
        }
        return lReturn;

    case CB_SETEXTENDEDUI:

        /*
         * wParam - specifies state to set extendui flag to.
         * Currently only 1 is allowed.  Return CB_ERR (-1) if
         * failure else 0 if success.
         */
        if (pcbox->CBoxStyle & SDROPPABLE) {
            if (!wParam) {
                pcbox->fExtendedUI = 0;
                return 0;
            }

            if (wParam == 1) {
              pcbox->fExtendedUI = 1;
              return 0;
            }

            RIPERR1(ERROR_INVALID_PARAMETER,
                    RIP_WARNING,
                    "Invalid parameter \"wParam\" (%ld) to ComboBoxWndProcWorker",
                    wParam);

        } else {
            RIPERR1(ERROR_INVALID_MESSAGE,
                    RIP_WARNING,
                    "Invalid message (%ld) sent to ComboBoxWndProcWorker",
                    message);
        }

        return CB_ERR;

    case CB_GETEXTENDEDUI:
        if (pcbox->CBoxStyle & SDROPPABLE) {
            if (pcbox->fExtendedUI)
                return TRUE;
        }
        return FALSE;

    case CB_GETEDITSEL:

        /*
         * wParam - not used
         * lParam - not used
         * effects: Gets the selection range for the given edit control.  The
         * starting BYTE-position is in the low order word.  It contains the
         * the BYTE-position of the first nonselected character after the end
         * of the selection in the high order word.  Returns CB_ERR if no
         * editcontrol.
         */
        message = EM_GETSEL;
        goto CallEditSendMessage;
        break;

    case CB_LIMITTEXT:

        /*
         * wParam - max number of bytes that can be entered
         * lParam - not used
         * effects: Specifies the maximum number of bytes of text the user may
         * enter.  If maxLength is 0, we may enter MAXINT number of BYTES.
         */
        message = EM_LIMITTEXT;
        goto CallEditSendMessage;
        break;

    case CB_SETEDITSEL:

        /*
         * wParam - ichStart
         * lParam - ichEnd
         *
         */
        message = EM_SETSEL;

        wParam = (int)(SHORT)LOWORD(lParam);
        lParam = (int)(SHORT)HIWORD(lParam);
        goto CallEditSendMessage;
        break;

    case CB_ADDSTRING:

        /*
         * wParam - not used
         * lParam - Points to null terminated string to be added to listbox
         */
        if (!pcbox->fCase)
            message = LB_ADDSTRING;
        else
            message = (pcbox->fCase & UPPERCASE) ? LB_ADDSTRINGUPPER : LB_ADDSTRINGLOWER;
        goto CallListSendMessage;
        break;

    case CB_DELETESTRING:

        /*
         * wParam - index to string to be deleted
         * lParam - not used
         */
        message = LB_DELETESTRING;
        goto CallListSendMessage;
        break;

    case CB_INITSTORAGE:
        // wParamLo - number of items
        // lParam - number of bytes of string space
        message = LB_INITSTORAGE;
        goto CallListSendMessage;

    case CB_SETTOPINDEX:
        // wParamLo - index to make top
        // lParam - not used
        message = LB_SETTOPINDEX;
        goto CallListSendMessage;

    case CB_GETTOPINDEX:
        // wParamLo / lParam - not used
        message = LB_GETTOPINDEX;
        goto CallListSendMessage;

    case CB_GETCOUNT:

        /*
         * wParam - not used
         * lParam - not used
         */
        message = LB_GETCOUNT;
        goto CallListSendMessage;
        break;

    case CB_GETCURSEL:

        /*
         * wParam - not used
         * lParam - not used
         */
        message = LB_GETCURSEL;
        goto CallListSendMessage;
        break;

    case CB_GETLBTEXT:

        /*
         * wParam - index of string to be copied
         * lParam - buffer that is to receive the string
         */
        message = LB_GETTEXT;
        goto CallListSendMessage;
        break;

    case CB_GETLBTEXTLEN:

        /*
         * wParam - index to string
         * lParam - now used for cbANSI
         */
        message = LB_GETTEXTLEN;
        goto CallListSendMessage;
        break;

    case CB_INSERTSTRING:

        /*
         * wParam - position to receive the string
         * lParam - points to the string
         */
        if (!pcbox->fCase)
            message = LB_INSERTSTRING;
        else
            message = (pcbox->fCase & UPPERCASE) ? LB_INSERTSTRINGUPPER : LB_INSERTSTRINGLOWER;
        goto CallListSendMessage;
        break;

    case CB_RESETCONTENT:

        /*
         * wParam - not used
         * lParam - not used
         * If we come here before WM_CREATE has been processed,
         * pcbox->spwndList will be NULL.
         */
        UserAssert(pcbox->spwndList);
        ThreadLock(pcbox->spwndList, &tlpwndList);
        SendMessageWorker(pcbox->spwndList, LB_RESETCONTENT, 0, 0, FALSE);
        ThreadUnlock(&tlpwndList);
        xxxCBInternalUpdateEditWindow(pcbox, NULL);
        break;

    case CB_GETHORIZONTALEXTENT:
        message = LB_GETHORIZONTALEXTENT;
        goto CallListSendMessage;

    case CB_SETHORIZONTALEXTENT:
        message = LB_SETHORIZONTALEXTENT;
        goto CallListSendMessage;

    case CB_FINDSTRING:

        /*
         * wParam - index of starting point for search
         * lParam - points to prefix string
         */
        message = LB_FINDSTRING;
        goto CallListSendMessage;
        break;

    case CB_FINDSTRINGEXACT:

        /*
         * wParam - index of starting point for search
         * lParam - points to a exact string
         */
        message = LB_FINDSTRINGEXACT;
        goto CallListSendMessage;
        break;

    case CB_SELECTSTRING:

        /*
         * wParam - index of starting point for search
         * lParam - points to prefix string
         */
        UserAssert(pcbox->spwndList);
        ThreadLock(pcbox->spwndList, &tlpwndList);
        lParam = SendMessageWorker(pcbox->spwndList, LB_SELECTSTRING,
                wParam, lParam, fAnsi);
        ThreadUnlock(&tlpwndList);
        xxxCBInternalUpdateEditWindow(pcbox, NULL);
        return lParam;

    case CB_SETCURSEL:

        /*
         * wParam - Contains index to be selected
         * lParam - not used
         * If we come here before WM_CREATE has been processed,
         * pcbox->spwndList will be NULL.
         */

        UserAssert(pcbox->spwndList);

        ThreadLock(pcbox->spwndList, &tlpwndList);
        lParam = SendMessageWorker(pcbox->spwndList, LB_SETCURSEL, wParam, lParam, FALSE);
        if (lParam != -1) {
            SendMessageWorker(pcbox->spwndList, LB_SETTOPINDEX, wParam, 0, FALSE);
        }
        ThreadUnlock(&tlpwndList);
        xxxCBInternalUpdateEditWindow(pcbox, NULL);
        return lParam;

    case CB_GETITEMDATA:
        message = LB_GETITEMDATA;
        goto CallListSendMessage;
        break;

    case CB_SETITEMDATA:
        message = LB_SETITEMDATA;
        goto CallListSendMessage;
        break;

    case CB_SETITEMHEIGHT:
        if (wParam == -1) {
            if (HIWORD(lParam) != 0)
                return CB_ERR;
            return xxxCBSetEditItemHeight(pcbox, LOWORD(lParam));
        }

        message = LB_SETITEMHEIGHT;
        goto CallListSendMessage;
        break;

    case CB_GETITEMHEIGHT:
        if (wParam == -1)
            return pcbox->editrc.bottom - pcbox->editrc.top;

        message = LB_GETITEMHEIGHT;
        goto CallListSendMessage;
        break;

    case CB_SHOWDROPDOWN:

        /*
         * wParam - True then drop down the listbox if possible else hide it
         * lParam - not used
         */
        if (wParam && !pcbox->fLBoxVisible) {
            xxxCBShowListBoxWindow(pcbox, TRUE);
        } else {
            if (!wParam && pcbox->fLBoxVisible) {
                xxxCBHideListBoxWindow(pcbox, TRUE, FALSE);
            }
        }
        break;

    case CB_SETLOCALE:

        /*
         * wParam - locale id
         * lParam - not used
         */
        message = LB_SETLOCALE;
        goto CallListSendMessage;
        break;

    case CB_GETLOCALE:

        /*
         * wParam - not used
         * lParam - not used
         */
        message = LB_GETLOCALE;
        goto CallListSendMessage;
        break;

    case WM_MEASUREITEM:
    case WM_DELETEITEM:
    case WM_DRAWITEM:
    case WM_COMPAREITEM:
        return xxxCBMessageItemHandler(pcbox, message, (LPVOID)lParam);

    case WM_NCCREATE:

        /*
         * wParam - Contains a handle to the window being created
         * lParam - Points to the CREATESTRUCT data structure for the window.
         */
        return CBNcCreateHandler(pcbox, pwnd);

    case WM_PARENTNOTIFY:
        if (LOWORD(wParam) == WM_DESTROY) {
            if ((HWND)lParam == HW(pcbox->spwndEdit)) {
                pcbox->CBoxStyle &= ~SEDITABLE;
                pcbox->fNoEdit = TRUE;
                pcbox->spwndEdit = pwnd;
            } else if ((HWND)lParam == HW(pcbox->spwndList)) {
                pcbox->CBoxStyle &= ~SDROPPABLE;
                pcbox->spwndList = NULL;
            }
        }
        break;

    case WM_UPDATEUISTATE:
        /*
         * Propagate the change to the list control, if any
         */
        UserAssert(pcbox->spwndList);
        ThreadLock(pcbox->spwndList, &tlpwndList);
        SendMessageWorker(pcbox->spwndList, WM_UPDATEUISTATE,
                          wParam, lParam, fAnsi);
        ThreadUnlock(&tlpwndList);
        goto CallDWP;

    case WM_HELP:
        {
            LPHELPINFO lpHelpInfo;

            /* Check if this message is form a child of this combo
             */
            if ((lpHelpInfo = (LPHELPINFO)lParam) != NULL &&
                ((pcbox->spwndEdit && lpHelpInfo->iCtrlId == (SHORT)(PTR_TO_ID(pcbox->spwndEdit->spmenu))) ||
                 lpHelpInfo->iCtrlId == (SHORT)(PTR_TO_ID(pcbox->spwndList->spmenu)) )) {

                // BUGBUG - What to do here?
                lpHelpInfo->iCtrlId = (SHORT)(PTR_TO_ID(pwnd->spmenu));
                lpHelpInfo->hItemHandle = hwnd;
                lpHelpInfo->dwContextId = GetContextHelpId(pwnd);
            }
        }
        /*
         * Fall through to DefWindowProc
         */

    default:

        if (SYSMET(PENWINDOWS) &&
                (message >= WM_PENWINFIRST && message <= WM_PENWINLAST))
            goto CallEditSendMessage;

CallDWP:
        return DefWindowProcWorker(pwnd, message, wParam, lParam, fAnsi);
    }  /* switch (message) */

    return TRUE;

/*
 * The following forward messages off to the child controls.
 */
CallEditSendMessage:
    if (!pcbox->fNoEdit && pcbox->spwndEdit) {
        /*
         * pcbox->spwndEdit will be NULL if we haven't done WM_CREATE yet!
         */
        ThreadLock(pcbox->spwndEdit, &tlpwndEdit);
        lReturn = SendMessageWorker(pcbox->spwndEdit, message,
                wParam, lParam, fAnsi);
        ThreadUnlock(&tlpwndEdit);
    }
    else {
        RIPERR0(ERROR_INVALID_COMBOBOX_MESSAGE, RIP_VERBOSE, "");
        lReturn = CB_ERR;
    }
    return lReturn;

CallListSendMessage:
    /*
     * pcbox->spwndList will be NULL if we haven't done WM_CREATE yet!
     */
    UserAssert(pcbox->spwndList);
    ThreadLock(pcbox->spwndList, &tlpwndList);
    lReturn = SendMessageWorker(pcbox->spwndList, message,
            wParam, lParam, fAnsi);
    ThreadUnlock(&tlpwndList);
    return lReturn;

}  /* ComboBoxWndProcWorker */


/***************************************************************************\
\***************************************************************************/

LRESULT WINAPI ComboBoxWndProcA(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    PWND pwnd;

    if ((pwnd = ValidateHwnd(hwnd)) == NULL) {
        return (0L);
    }

    /*
     * If the control is not interested in this message,
     * pass it to DefWindowProc.
     */
    if (!FWINDOWMSG(message, FNID_COMBOBOX) &&
            !(SYSMET(PENWINDOWS) &&
                    (message >= WM_PENWINFIRST && message <= WM_PENWINLAST)))
        return DefWindowProcWorker(pwnd, message, wParam, lParam, TRUE);

    return ComboBoxWndProcWorker(pwnd, message, wParam, lParam, TRUE);
}

LRESULT WINAPI ComboBoxWndProcW(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    PWND pwnd;

    if ((pwnd = ValidateHwnd(hwnd)) == NULL) {
        return (0L);
    }

    /*
     * If the control is not interested in this message,
     * pass it to DefWindowProc.
     */
    if (!FWINDOWMSG(message, FNID_COMBOBOX) &&
            !(SYSMET(PENWINDOWS) &&
                    (message >= WM_PENWINFIRST && message <= WM_PENWINLAST)))
        return DefWindowProcWorker(pwnd, message, wParam, lParam, FALSE);

    return ComboBoxWndProcWorker(pwnd, message, wParam, lParam, FALSE);
}


/***************************************************************************\
* xxxCBMessageItemHandler
*
* Handles WM_DRAWITEM,WM_MEASUREITEM,WM_DELETEITEM,WM_COMPAREITEM
* messages from the listbox.
*
* History:
\***************************************************************************/

LRESULT xxxCBMessageItemHandler(
    PCBOX pcbox,
    UINT message,
    LPVOID lpfoo)  /* Actually can be any of the structs below */
{
    LRESULT lRet;
    TL tlpwndParent;

    CheckLock(pcbox->spwnd);

    /*
     * Send the <foo>item message back to the application after changing some
     * parameters to their combo box specific versions.
     */
    ((LPMEASUREITEMSTRUCT)lpfoo)->CtlType = ODT_COMBOBOX;
    ((LPMEASUREITEMSTRUCT)lpfoo)->CtlID = PtrToUlong(pcbox->spwnd->spmenu);
    if (message == WM_DRAWITEM)
        ((LPDRAWITEMSTRUCT)lpfoo)->hwndItem = HWq(pcbox->spwnd);
    else if (message == WM_DELETEITEM)
        ((LPDELETEITEMSTRUCT)lpfoo)->hwndItem = HWq(pcbox->spwnd);
    else if (message == WM_COMPAREITEM)
        ((LPCOMPAREITEMSTRUCT)lpfoo)->hwndItem = HWq(pcbox->spwnd);

    ThreadLock(pcbox->spwndParent, &tlpwndParent);
    lRet = SendMessage(HW(pcbox->spwndParent), message,
            (WPARAM)pcbox->spwnd->spmenu, (LPARAM)lpfoo);
    ThreadUnlock(&tlpwndParent);

    return lRet;
}


/***************************************************************************\
* xxxCBPaint
*
* History:
\***************************************************************************/

void xxxCBPaint(
    PCBOX pcbox,
    HDC hdc)
{
    RECT rc;
    UINT msg;
    HBRUSH hbr;

    CheckLock(pcbox->spwnd);

    rc.left = rc.top = 0;
    rc.right = pcbox->cxCombo;
    rc.bottom = pcbox->cyCombo;
    if (pcbox->f3DCombo)
        DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
    else
        DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_RECT | BF_ADJUST | BF_FLAT | BF_MONO);

    if (pcbox->buttonrc.left != 0) {
    // Draw in the dropdown arrow button
        DrawFrameControl(hdc, &pcbox->buttonrc, DFC_SCROLL,
            DFCS_SCROLLCOMBOBOX |
            (pcbox->fButtonPressed ? DFCS_PUSHED | DFCS_FLAT : 0) |
            (TestWF(pcbox->spwnd, WFDISABLED) ? DFCS_INACTIVE : 0));
#ifdef COLOR_HOTTRACKING
            (pcbox->fButtonHotTracked ? DFCS_HOT: 0)));
#endif // COLOR_HOTTRACKING
        if (pcbox->fRightAlign )
            rc.left = pcbox->buttonrc.right;
        else
            rc.right = pcbox->buttonrc.left;
    }

    // Erase the background behind the edit/static item.  Since a combo
    // is an edit field/list box hybrid, we use the same coloring
    // conventions.
    msg = WM_CTLCOLOREDIT;
    if (TestWF(pcbox->spwnd, WFWIN40COMPAT)) {
        if (TestWF(pcbox->spwnd, WFDISABLED) ||
            (!pcbox->fNoEdit && pcbox->spwndEdit && TestWF(pcbox->spwndEdit, EFREADONLY)))
            msg = WM_CTLCOLORSTATIC;
    } else
        msg = WM_CTLCOLORLISTBOX;

    hbr = GetControlBrush(HWq(pcbox->spwnd), hdc, msg);

    if (pcbox->fNoEdit)
        xxxCBInternalUpdateEditWindow(pcbox, hdc);
    else
        FillRect(hdc, &rc, hbr);
}


/***************************************************************************\
* xxxCBCommandHandler
*
* Check the various notification codes from the controls and do the
* proper thing.
* always returns 0L
*
* History:
\***************************************************************************/

long xxxCBCommandHandler(
    PCBOX pcbox,
    DWORD wParam,
    HWND hwndControl)
{

    CheckLock(pcbox->spwnd);

    /*
     * Check the edit control notification codes.  Note that currently, edit
     * controls don't send EN_KILLFOCUS messages to the parent.
     */
    if (!pcbox->fNoEdit &&
            SAMEWOWHANDLE(hwndControl, HWq(pcbox->spwndEdit))) {

        /*
         * Edit control notification codes
         */
        switch (HIWORD(wParam)) {
        case EN_SETFOCUS:
            if (!pcbox->fFocus) {

                /*
                 * The edit control has the focus for the first time which means
                 * this is the first time the combo box has received the focus
                 * and the parent must be notified that we have the focus.
                 */
                xxxCBGetFocusHelper(pcbox);
            }
            break;

        case EN_CHANGE:
            xxxCBNotifyParent(pcbox, CBN_EDITCHANGE);
            xxxCBUpdateListBoxWindow(pcbox, FALSE);
            break;

        case EN_UPDATE:
            xxxCBNotifyParent(pcbox, CBN_EDITUPDATE);
            break;

        case EN_ERRSPACE:
            xxxCBNotifyParent(pcbox, CBN_ERRSPACE);
            break;
        }
    }

    /*
     * Check listbox control notification codes
     */
    if (SAMEWOWHANDLE(hwndControl, HWq(pcbox->spwndList))) {

        /*
         * Listbox control notification codes
         */
        switch ((int)HIWORD(wParam)) {
        case LBN_DBLCLK:
            xxxCBNotifyParent(pcbox, CBN_DBLCLK);
            break;

        case LBN_ERRSPACE:
            xxxCBNotifyParent(pcbox, CBN_ERRSPACE);
            break;

        case LBN_SELCHANGE:
        case LBN_SELCANCEL:
            if (!pcbox->fKeyboardSelInListBox) {

                /*
                 * If the selchange is caused by the user keyboarding through,
                 * we don't want to hide the listbox.
                 */
                if (!xxxCBHideListBoxWindow(pcbox, TRUE, TRUE))
                    return(0L);
            } else {
                pcbox->fKeyboardSelInListBox = FALSE;
            }

            xxxCBNotifyParent(pcbox, CBN_SELCHANGE);
            xxxCBInternalUpdateEditWindow(pcbox, NULL);
            break;
        }
    }

    return 0L;
}


/***************************************************************************\
* xxxCBNotifyParent
*
* Sends the notification code to the parent of the combo box control
*
* History:
\***************************************************************************/

void xxxCBNotifyParent(
    PCBOX pcbox,
    short notificationCode)
{
    PWND pwndParent;            // Parent if it exists
    TL tlpwndParent;

    CheckLock(pcbox->spwnd);

    if (pcbox->spwndParent)
        pwndParent = pcbox->spwndParent;
    else
        pwndParent = pcbox->spwnd;

    /*
     * wParam contains Control ID and notification code.
     * lParam contains Handle to window
     */
    ThreadLock(pwndParent, &tlpwndParent);
    SendMessageWorker(pwndParent, WM_COMMAND,
            MAKELONG(PTR_TO_ID(pcbox->spwnd->spmenu), notificationCode),
            (LPARAM)HWq(pcbox->spwnd), FALSE);
    ThreadUnlock(&tlpwndParent);
}

/***************************************************************************\
*
*
* Completes the text in the edit box with the closest match from the
* listbox.  If a prefix match can't be found, the edit control text isn't
* updated. Assume a DROPDOWN style combo box.
*
*
* History:
\***************************************************************************/
void xxxCBCompleteEditWindow(
    PCBOX pcbox)
{
    int cchText;
    int cchItemText;
    int itemNumber;
    LPWSTR pText;
    TL tlpwndEdit;
    TL tlpwndList;

    CheckLock(pcbox->spwnd);

    /*
     * Firstly check the edit control.
     */
    if (pcbox->spwndEdit == NULL) {
        return;
    }

    ThreadLock(pcbox->spwndEdit, &tlpwndEdit);
    ThreadLock(pcbox->spwndList, &tlpwndList);

    /*
     * +1 for null terminator
     */
    cchText = (int)SendMessageWorker(pcbox->spwndEdit, WM_GETTEXTLENGTH, 0, 0, FALSE);

    if (cchText) {
        cchText++;
        if (!(pText = (LPWSTR)UserLocalAlloc(HEAP_ZERO_MEMORY, cchText*sizeof(WCHAR))))
            goto Unlock;

        /*
         * We want to be sure to free the above allocated memory even if
         * the client dies during callback (xxx) or some of the following
         * window revalidation fails.
         */
        try {
            SendMessageWorker(pcbox->spwndEdit, WM_GETTEXT, cchText, (LPARAM)pText, FALSE);
            itemNumber = (int)SendMessageWorker(pcbox->spwndList,
                    LB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)pText, FALSE);
            if (itemNumber == -1)
                itemNumber = (int)SendMessageWorker(pcbox->spwndList,
                        LB_FINDSTRING, (WPARAM)-1, (LPARAM)pText, FALSE);
        } finally {
            UserLocalFree((HANDLE)pText);
        }

        if (itemNumber == -1) {

            /*
             * No close match.  Blow off.
             */
            goto Unlock;
        }

        cchItemText = (int)SendMessageWorker(pcbox->spwndList, LB_GETTEXTLEN,
                itemNumber, 0, FALSE);
        if (cchItemText) {
            cchItemText++;
            if (!(pText = (LPWSTR)UserLocalAlloc(HEAP_ZERO_MEMORY, cchItemText*sizeof(WCHAR))))
                goto Unlock;

            /*
             * We want to be sure to free the above allocated memory even if
             * the client dies during callback (xxx) or some of the following
             * window revalidation fails.
             */
            try {
                SendMessageWorker(pcbox->spwndList, LB_GETTEXT,
                        itemNumber, (LPARAM)pText, FALSE);
                SendMessageWorker(pcbox->spwndEdit, WM_SETTEXT,
                        0, (LPARAM)pText, FALSE);
            } finally {
                UserLocalFree((HANDLE)pText);
            }

            SendMessageWorker(pcbox->spwndEdit, EM_SETSEL, 0, MAXLONG, !!TestWF(pcbox->spwnd, WFANSIPROC));
        }
    }

Unlock:
    ThreadUnlock(&tlpwndList);
    ThreadUnlock(&tlpwndEdit);
}


/***************************************************************************\
* xxxCBHideListBoxWindow
*
* Hides the dropdown listbox window if it is a dropdown style.
*
* History:
\***************************************************************************/

BOOL xxxCBHideListBoxWindow(
    PCBOX pcbox,
    BOOL fNotifyParent,
    BOOL fSelEndOK)
{
    HWND hwnd = HWq(pcbox->spwnd);
    HWND hwndList = HWq(pcbox->spwndList);
    TL tlpwndList;


    CheckLock(pcbox->spwnd);

    // For 3.1+ apps, send CBN_SELENDOK to all types of comboboxes but only
    // allow CBN_SELENDCANCEL to be sent for droppable comboboxes
    if (fNotifyParent && TestWF(pcbox->spwnd, WFWIN31COMPAT) &&
        ((pcbox->CBoxStyle & SDROPPABLE) || fSelEndOK)) {
        if (fSelEndOK)
        {
            xxxCBNotifyParent(pcbox, CBN_SELENDOK);
        }
        else
        {
            xxxCBNotifyParent(pcbox, CBN_SELENDCANCEL);
        }
        if (!IsWindow(hwnd))
            return(FALSE);
    }

    /*
     * return, we don't hide simple combo boxes.
     */
    if (!(pcbox->CBoxStyle & SDROPPABLE)) {
        return TRUE;
    }

    /*
     * Send a faked buttonup message to the listbox so that it can release
     * the capture and all.
     */
    ThreadLock(pcbox->spwndList, &tlpwndList);

    SendMessageWorker(pcbox->spwndList, LBCB_ENDTRACK, fSelEndOK, 0, FALSE);

    if (pcbox->fLBoxVisible) {
        WORD swpFlags = SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE;

        if (!TestWF(pcbox->spwnd, WFWIN31COMPAT))
            swpFlags |= SWP_FRAMECHANGED;

        pcbox->fLBoxVisible = FALSE;

        /*
         * Hide the listbox window
         */
        NtUserShowWindow(hwndList, SW_HIDE);

        //
        // Invalidate the item area now since SWP() might update stuff.
        // Since the combo is CS_VREDRAW/CS_HREDRAW, a size change will
        // redraw the whole thing, including the item rect.  But if it
        // isn't changing size, we still want to redraw the item anyway
        // to show focus/selection.
        //
        if (!(pcbox->CBoxStyle & SEDITABLE))
            NtUserInvalidateRect(hwnd, &pcbox->editrc, TRUE);

        NtUserSetWindowPos(hwnd, HWND_TOP, 0, 0,
                pcbox->cxCombo, pcbox->cyCombo, swpFlags);

        // In case size didn't change
        UpdateWindow(hwnd);

        if (pcbox->CBoxStyle & SEDITABLE) {
            xxxCBCompleteEditWindow(pcbox);
        }

        if (fNotifyParent) {

            /*
             * Notify parent we will be popping up the combo box.
             */
            xxxCBNotifyParent(pcbox, CBN_CLOSEUP);
            if (!IsWindow(hwnd))
                return(FALSE);
        }
    }

    ThreadUnlock(&tlpwndList);

    return(TRUE);
}

/***************************************************************************\
* xxxCBShowListBoxWindow
*
* Lowers the dropdown listbox window.
*
* History:
\***************************************************************************/

void xxxCBShowListBoxWindow(
    PCBOX pcbox, BOOL fTrack)
{
    RECT        editrc;
    int         itemNumber;
    int         iHeight;
    int         yTop;
    DWORD       dwMult;
    int         cyItem;
    HWND        hwnd = HWq(pcbox->spwnd);
    HWND        hwndList = HWq(pcbox->spwndList);
    BOOL        fAnimPos;
    TL          tlpwndList;
    PMONITOR    pMonitor;

    //
    // THIS FUNCTION IS ONLY CALLED FOR DROPPABLE LIST COMBOBOXES
    //
    UserAssert(pcbox->CBoxStyle & SDROPPABLE);

    CheckLock(pcbox->spwnd);

    ThreadLock(pcbox->spwndList, &tlpwndList);

    /*
     * Notify parent we will be dropping down the combo box.
     */

    xxxCBNotifyParent(pcbox, CBN_DROPDOWN);
    /*
     * Invalidate the button rect so that the depressed arrow is drawn.
     */
    NtUserInvalidateRect(hwnd, &pcbox->buttonrc, TRUE);

    pcbox->fLBoxVisible = TRUE;

    if (pcbox->CBoxStyle == SDROPDOWN) {

        /*
         * If an item in the listbox matches the text in the edit control,
         * scroll it to the top of the listbox.  Select the item only if the
         * mouse button isn't down otherwise we will select the item when the
         * mouse button goes up.
         */
        xxxCBUpdateListBoxWindow(pcbox, !pcbox->fMouseDown);
        if (!pcbox->fMouseDown)
            xxxCBCompleteEditWindow(pcbox);
    } else {

        /*
         * Scroll the currently selected item to the top of the listbox.
         */
        itemNumber = (int)SendMessageWorker(pcbox->spwndList, LB_GETCURSEL,
                0, 0, FALSE);
        if (itemNumber == -1) {
            itemNumber = 0;
        }
        SendMessageWorker(pcbox->spwndList, LB_SETTOPINDEX, itemNumber, 0, FALSE);
        SendMessageWorker(pcbox->spwndList, LBCB_CARETON, 0, 0, FALSE);

        /*
         * We need to invalidate the edit rect so that the focus frame/invert
         * will be turned off when the listbox is visible.  Tandy wants this for
         * his typical reasons...
         */
        NtUserInvalidateRect(hwnd, &pcbox->editrc, TRUE);
    }

    //
    // Figure out where to position the dropdown listbox.  We want it just
    // touching the edge around the edit rectangle.  Note that since the
    // listbox is a popup, we need the position in screen coordinates.
    //

    // We want the dropdown to pop below or above the combo

    // Get screen coords
    editrc.left   = pcbox->spwnd->rcWindow.left;
    editrc.top    = pcbox->spwnd->rcWindow.top;
    editrc.right  = pcbox->spwnd->rcWindow.left + pcbox->cxCombo;
    editrc.bottom = pcbox->spwnd->rcWindow.top  + pcbox->cyCombo;

    // List area
    cyItem = (int)SendMessageWorker(pcbox->spwndList, LB_GETITEMHEIGHT, 0, 0, FALSE);

    if (cyItem == 0) {
        // Make sure that it's not 0
        RIPMSG0( RIP_WARNING, "LB_GETITEMHEIGHT is returning 0\n" );

        cyItem = gpsi->cySysFontChar;
    }

    //  we shoulda' just been able to use cyDrop here, but thanks to VB's need
    //  to do things their OWN SPECIAL WAY, we have to keep monitoring the size
    //  of the listbox 'cause VB changes it directly (jeffbog 03/21/94)
    iHeight = max(pcbox->cyDrop, pcbox->spwndList->rcWindow.bottom -
                                 pcbox->spwndList->rcWindow.top);

    if (dwMult = (DWORD)SendMessageWorker(pcbox->spwndList, LB_GETCOUNT, 0, 0, FALSE)) {
        dwMult = (DWORD)(LOWORD(dwMult) * cyItem);
        dwMult += SYSMET(CYEDGE);

        if (dwMult < 0x7FFF)
            iHeight = min(LOWORD(dwMult), iHeight);
    }

    if (!TestWF(pcbox->spwnd, CBFNOINTEGRALHEIGHT)) {
        UserAssert(cyItem);
        iHeight = ((iHeight - SYSMET(CYEDGE)) / cyItem) * cyItem + SYSMET(CYEDGE);
    }

    //
    // Other 1/2 of old app combo fix.  Make dropdown overlap combo window
    // a little.  That way we can have a chance of invalidating the overlap
    // and causing a repaint to help out Publisher 2.0's toolbar combos.
    // See comments for PressButton() above.
    //
    pMonitor = _MonitorFromWindow(pcbox->spwnd, MONITOR_DEFAULTTOPRIMARY);
    if (editrc.bottom + iHeight <= pMonitor->rcMonitor.bottom) {
        yTop = editrc.bottom;
        if (!pcbox->f3DCombo)
            yTop -= SYSMET(CYBORDER);

        fAnimPos = TRUE;
    } else {
        yTop = max(editrc.top - iHeight, pMonitor->rcMonitor.top);
        if (!pcbox->f3DCombo)
            yTop += SYSMET(CYBORDER);

        fAnimPos = FALSE;
    }

    if ( ! TestWF( pcbox->spwnd, WFWIN40COMPAT) )
    {
      // fix for Winword B#7504, Combo-ListBox text gets
      // truncated by a small width, this is do to us
      // now setting size here in SetWindowPos, rather than
      // earlier where we did this in Win3.1

      if ( (pcbox->spwndList->rcWindow.right - pcbox->spwndList->rcWindow.left ) >
            pcbox->cxDrop )

            pcbox->cxDrop = pcbox->spwndList->rcWindow.right - pcbox->spwndList->rcWindow.left;
    }

    NtUserSetWindowPos(hwndList, HWND_TOPMOST, editrc.left,
        yTop, max(pcbox->cxDrop, pcbox->cxCombo), iHeight, SWP_NOACTIVATE);

    /*
     * Get any drawing in the combo box window out of the way so it doesn't
     * invalidate any of the SPB underneath the list window.
     */
    UpdateWindow(hwnd);

    if (!(TEST_EffectPUSIF(PUSIF_COMBOBOXANIMATION))
        || (GetAppCompatFlags2(VER40) & GACF2_ANIMATIONOFF)) {
        NtUserShowWindow(hwndList, SW_SHOWNA);
    } else {
        AnimateWindow(hwndList, CMS_QANIMATION, (fAnimPos ? AW_VER_POSITIVE :
                AW_VER_NEGATIVE) | AW_SLIDE);
    }

#ifdef LATER
//
// we don't have sys modal windows.
//
    if (pwndSysModal) {

        /*
         * If this combo is in a system modal dialog box, we need to explicitly
         * call update window otherwise we won't automatically send paint
         * messages to the toplevel listbox window.  This is especially
         * noticeable in the File Open/Save sys modal dlgs which are put up at
         * ExitWindows time.
         */
        UpdateWindow(hwndList);
    }
#endif

    /*
     * Restart search buffer from first char
     */
    {
    PLBIV plb = ((PLBWND)pcbox->spwndList)->pLBIV;

        if ((plb != NULL) && (plb != (PLBIV)-1)) {
            plb->iTypeSearch = 0;
        }
    }

    if (fTrack && TestWF(pcbox->spwnd, WFWIN40COMPAT))
        SendMessageWorker(pcbox->spwndList, LBCB_STARTTRACK, FALSE, 0, FALSE);

    ThreadUnlock(&tlpwndList);
}

/***************************************************************************\
* xxxCBInternalUpdateEditWindow
*
* Updates the editcontrol/statictext window so that it contains the text
* given by the current selection in the listbox.  If the listbox has no
* selection (ie. -1), then we erase all the text in the editcontrol.
*
* hdcPaint is from WM_PAINT messages Begin/End Paint hdc. If null, we should
* get our own dc.
*
* History:
\***************************************************************************/

void xxxCBInternalUpdateEditWindow(
    PCBOX pcbox,
    HDC hdcPaint)
{
    int cchText = 0;
    LPWSTR pText = NULL;
    int sItem;
    HDC hdc;
    UINT msg;
    HBRUSH hbrSave;
    HBRUSH hbrControl;
    HANDLE hOldFont;
    DRAWITEMSTRUCT dis;
    RECT rc;
    HWND hwnd = HWq(pcbox->spwnd);
    TL tlpwndList;
    TL tlpwndEdit;
    TL tlpwndParent;

    CheckLock(pcbox->spwnd);

    /* This check is also commented out in Win3.1 and Win95 */
    // if (!TestWF(pcbox->spwnd, WFVISIBLE)) {
    //    return;
    // }

    ThreadLock(pcbox->spwndParent, &tlpwndParent);
    ThreadLock(pcbox->spwndList, &tlpwndList);
    ThreadLock(pcbox->spwndEdit, &tlpwndEdit);

    sItem = (int)SendMessageWorker(pcbox->spwndList, LB_GETCURSEL, 0, 0, FALSE);

    /*
     * This 'try-finally' block ensures that the allocated 'pText' will
     * be freed no matter how this routine is exited.
     */
    try {
        if (sItem != -1) {
            cchText = (int)SendMessageWorker(pcbox->spwndList, LB_GETTEXTLEN,
                    (DWORD)sItem, 0, FALSE);
            if ((pText = (LPWSTR)UserLocalAlloc(HEAP_ZERO_MEMORY, (cchText+1) * sizeof(WCHAR)))) {
                cchText = (int)SendMessageWorker(pcbox->spwndList, LB_GETTEXT,
                        (DWORD)sItem, (LPARAM)pText, FALSE);
            }
        }

        if (!pcbox->fNoEdit) {

            if (pcbox->spwndEdit) {
                if (TestWF(pcbox->spwnd, CBFHASSTRINGS))
                    SetWindowText(HWq(pcbox->spwndEdit), pText ? pText : TEXT(""));

                if (pcbox->fFocus) {
                    /*
                     * Only hilite the text if we have the focus.
                     */
                    SendMessageWorker(pcbox->spwndEdit, EM_SETSEL, 0, MAXLONG, !!TestWF(pcbox->spwnd, WFANSIPROC));
                }
            }
        } else if (IsComboVisible(pcbox)) {
            if (hdcPaint) {
                hdc = hdcPaint;
            } else {
                hdc = NtUserGetDC(hwnd);
            }

            SetBkMode(hdc, OPAQUE);
            if (TestWF(pcbox->spwnd, WFWIN40COMPAT)) {
                if (TestWF(pcbox->spwnd, WFDISABLED))
                    msg = WM_CTLCOLORSTATIC;
                else
                    msg = WM_CTLCOLOREDIT;
            } else
                msg = WM_CTLCOLORLISTBOX;

            hbrControl = GetControlBrush(hwnd, hdc, msg);
            hbrSave = SelectObject(hdc, hbrControl);

            CopyInflateRect(&rc, &pcbox->editrc, SYSMET(CXBORDER), SYSMET(CYBORDER));
            PatBlt(hdc, rc.left, rc.top, rc.right - rc.left,
                rc.bottom - rc.top, PATCOPY);
            InflateRect(&rc, -SYSMET(CXBORDER), -SYSMET(CYBORDER));

            if (pcbox->fFocus && !pcbox->fLBoxVisible) {
                //
                // Fill in the selected area
                //


                // only do the FillRect if we know its not
                // ownerdraw item, otherwise we mess up people up
                // BUT: for Compat's sake we still do this for Win 3.1 guys

                if (!TestWF( pcbox->spwnd, WFWIN40COMPAT) || !pcbox->OwnerDraw)
                    FillRect(hdc, &rc, SYSHBR(HIGHLIGHT));

                SetBkColor(hdc, SYSRGB(HIGHLIGHT));
                SetTextColor(hdc, SYSRGB(HIGHLIGHTTEXT));
            } else if (TestWF(pcbox->spwnd, WFDISABLED) && !pcbox->OwnerDraw) {
                if ((COLORREF)SYSRGB(GRAYTEXT) != GetBkColor(hdc))
                    SetTextColor(hdc, SYSRGB(GRAYTEXT));
            }

            if (pcbox->hFont != NULL)
                hOldFont = SelectObject(hdc, pcbox->hFont);

            if (pcbox->OwnerDraw) {

                /*
                 * Let the app draw the stuff in the static text box.
                 */
                dis.CtlType = ODT_COMBOBOX;
                dis.CtlID = PtrToUlong(pcbox->spwnd->spmenu);
                dis.itemID = sItem;
                dis.itemAction = ODA_DRAWENTIRE;
                dis.itemState = (UINT)
                    ((pcbox->fFocus && !pcbox->fLBoxVisible ? ODS_SELECTED : 0) |
                    (TestWF(pcbox->spwnd, WFDISABLED) ? ODS_DISABLED : 0) |
                    (pcbox->fFocus && !pcbox->fLBoxVisible ? ODS_FOCUS : 0) |
                    (TestWF(pcbox->spwnd, WFWIN40COMPAT) ? ODS_COMBOBOXEDIT : 0) |
                    (TestWF(pcbox->spwnd, WEFPUIFOCUSHIDDEN) ? ODS_NOFOCUSRECT : 0) |
                    (TestWF(pcbox->spwnd, WEFPUIACCELHIDDEN) ? ODS_NOACCEL : 0));

                dis.hwndItem = hwnd;
                dis.hDC = hdc;
                CopyRect(&dis.rcItem, &rc);

                // Don't let ownerdraw dudes draw outside of the combo client
                // bounds.
                IntersectClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);

                dis.itemData = (ULONG_PTR)SendMessageWorker(pcbox->spwndList,
                        LB_GETITEMDATA, (UINT)sItem, 0, FALSE);

                SendMessage(HW(pcbox->spwndParent), WM_DRAWITEM, dis.CtlID,
                        (LPARAM)&dis);
            } else {

                /*
                 * Start the text one pixel within the rect so that we leave a
                 * nice hilite border around the text.
                 */

                int x ;
                UINT align ;

                if (pcbox->fRightAlign ) {
                    align = TA_RIGHT;
                    x = rc.right - SYSMET(CXBORDER);
                } else {
                    x = rc.left + SYSMET(CXBORDER);
                    align = 0;
                }

                if (pcbox->fRtoLReading )
                    align |= TA_RTLREADING;

                if (align)
                    SetTextAlign(hdc, GetTextAlign(hdc) | align);

                // Draw the text, leaving a gap on the left & top for selection.
                ExtTextOut(hdc, x, rc.top + SYSMET(CYBORDER), ETO_CLIPPED | ETO_OPAQUE,
                       &rc, pText ? pText : TEXT(""), cchText, NULL);
                if (pcbox->fFocus && !pcbox->fLBoxVisible) {
                    if (!TestWF(pcbox->spwnd, WEFPUIFOCUSHIDDEN)) {
                        DrawFocusRect(hdc, &rc);
                    }
                }
            }

            if (pcbox->hFont && hOldFont) {
                SelectObject(hdc, hOldFont);
            }

            if (hbrSave) {
                SelectObject(hdc, hbrSave);
            }

            if (!hdcPaint) {
                NtUserReleaseDC(hwnd, hdc);
            }
        }

    } finally {
        if (pText != NULL)
            UserLocalFree((HANDLE)pText);
    }

    ThreadUnlock(&tlpwndEdit);
    ThreadUnlock(&tlpwndList);
    ThreadUnlock(&tlpwndParent);
}

/***************************************************************************\
* xxxCBInvertStaticWindow
*
* Inverts the static text/picture window associated with the combo
* box.  Gets its own hdc, if the one given is null.
*
* History:
\***************************************************************************/

void xxxCBInvertStaticWindow(
    PCBOX pcbox,
    BOOL fNewSelectionState,  /* True if inverted else false */
    HDC hdc)
{
    BOOL focusSave = pcbox->fFocus;

    CheckLock(pcbox->spwnd);

    pcbox->fFocus = (UINT)fNewSelectionState;
    xxxCBInternalUpdateEditWindow(pcbox, hdc);

    pcbox->fFocus = (UINT)focusSave;
}

/***************************************************************************\
* xxxCBUpdateListBoxWindow
*
* matches the text in the editcontrol. If fSelectionAlso is false, then we
* unselect the current listbox selection and just move the caret to the item
* which is the closest match to the text in the editcontrol.
*
* History:
\***************************************************************************/

void xxxCBUpdateListBoxWindow(
    PCBOX pcbox,
    BOOL fSelectionAlso)
{
    int cchText;
    int sItem, sSel;
    LPWSTR pText = NULL;
    TL tlpwndEdit;
    TL tlpwndList;

    if (pcbox->spwndEdit == NULL) {
        return;
    }

    CheckLock(pcbox->spwnd);

    ThreadLock(pcbox->spwndList, &tlpwndList);
    ThreadLock(pcbox->spwndEdit, &tlpwndEdit);

    /*
     * +1 for null terminator
     */

    cchText = (int)SendMessageWorker(pcbox->spwndEdit, WM_GETTEXTLENGTH, 0, 0, FALSE);

    if (cchText) {
        cchText++;
        pText = (LPWSTR)UserLocalAlloc(HEAP_ZERO_MEMORY, cchText*sizeof(WCHAR));
        if (pText != NULL) {
            try {
                SendMessageWorker(pcbox->spwndEdit, WM_GETTEXT, cchText, (LPARAM)pText, FALSE);
                sItem = (int)SendMessageWorker(pcbox->spwndList, LB_FINDSTRING,
                        (WPARAM)-1L, (LPARAM)pText, FALSE);
            } finally {
                UserLocalFree((HANDLE)pText);
            }
        }
    }
    else
        sItem = -1;

    if (fSelectionAlso) {
        sSel = sItem;
    } else {
        sSel = -1;
    }

    if (sItem == -1)
    {
        sItem = 0;

        //
        // Old apps:  w/ editable combos, selected 1st item in list even if
        // it didn't match text in edit field.  This is not desirable
        // behavior for 4.0 dudes esp. with cancel allowed.  Reason:
        //      (1) User types in text that doesn't match list choices
        //      (2) User drops combo
        //      (3) User pops combo back up
        //      (4) User presses OK in dialog that does stuff w/ combo
        //          contents.
        // In 3.1, when the combo dropped, we'd select the 1st item anyway.
        // So the last CBN_SELCHANGE the owner got would be 0--which is
        // bogus because it really should be -1.  In fact if you type anything
        // into the combo afterwards it will reset itself to -1.
        //
        // 4.0 dudes won't get this bogus 0 selection.
        //
        if (fSelectionAlso && !TestWF(pcbox->spwnd, WFWIN40COMPAT))
            sSel = 0;
    }


    SendMessageWorker(pcbox->spwndList, LB_SETCURSEL, (DWORD)sSel, 0, FALSE);
    SendMessageWorker(pcbox->spwndList, LB_SETCARETINDEX, (DWORD)sItem, 0, FALSE);
    SendMessageWorker(pcbox->spwndList, LB_SETTOPINDEX, (DWORD)sItem, 0, FALSE);

    ThreadUnlock(&tlpwndEdit);
    ThreadUnlock(&tlpwndList);
}

/***************************************************************************\
* xxxCBGetFocusHelper
*
* Handles getting the focus for the combo box
*
* History:
\***************************************************************************/

void xxxCBGetFocusHelper(
    PCBOX pcbox)
{
    TL tlpwndList;
    TL tlpwndEdit;

    CheckLock(pcbox->spwnd);

    if (pcbox->fFocus)
        return;

    ThreadLock(pcbox->spwndList, &tlpwndList);
    ThreadLock(pcbox->spwndEdit, &tlpwndEdit);

    /*
     * The combo box has gotten the focus for the first time.
     */

    /*
     * First turn on the listbox caret
     */

    if (pcbox->CBoxStyle == SDROPDOWNLIST)
       SendMessageWorker(pcbox->spwndList, LBCB_CARETON, 0, 0, FALSE);

    /*
     * and select all the text in the editcontrol or static text rectangle.
     */

    if (pcbox->fNoEdit) {

        /*
         * Invert the static text rectangle
         */
        xxxCBInvertStaticWindow(pcbox, TRUE, (HDC)NULL);
    } else if (pcbox->spwndEdit) {
        UserAssert(pcbox->spwnd);
        SendMessageWorker(pcbox->spwndEdit, EM_SETSEL, 0, MAXLONG, !!TestWF(pcbox->spwnd, WFANSIPROC));
    }

    pcbox->fFocus = TRUE;

    /*
     * Notify the parent we have the focus
     */
    xxxCBNotifyParent(pcbox, CBN_SETFOCUS);

    ThreadUnlock(&tlpwndEdit);
    ThreadUnlock(&tlpwndList);
}

/***************************************************************************\
* xxxCBKillFocusHelper
*
* Handles losing the focus for the combo box.
*
* History:
\***************************************************************************/

void xxxCBKillFocusHelper(
    PCBOX pcbox)
{
    TL tlpwndList;
    TL tlpwndEdit;

    CheckLock(pcbox->spwnd);

    if (!pcbox->fFocus || pcbox->spwndList == NULL)
        return;

    ThreadLock(pcbox->spwndList, &tlpwndList);
    ThreadLock(pcbox->spwndEdit, &tlpwndEdit);

    /*
     * The combo box is losing the focus.  Send buttonup clicks so that
     * things release the mouse capture if they have it...  If the
     * pwndListBox is null, don't do anything.  This occurs if the combo box
     * is destroyed while it has the focus.
     */
    SendMessageWorker(pcbox->spwnd, WM_LBUTTONUP, 0L, 0xFFFFFFFFL, FALSE);
     if (!xxxCBHideListBoxWindow(pcbox, TRUE, FALSE))
         return;

    /*
     * Turn off the listbox caret
     */

    if (pcbox->CBoxStyle == SDROPDOWNLIST)
       SendMessageWorker(pcbox->spwndList, LBCB_CARETOFF, 0, 0, FALSE);

    if (pcbox->fNoEdit) {

        /*
         * Invert the static text rectangle
         */
        xxxCBInvertStaticWindow(pcbox, FALSE, (HDC)NULL);
    } else if (pcbox->spwndEdit) {
        SendMessageWorker(pcbox->spwndEdit, EM_SETSEL, 0, 0, !!TestWF(pcbox->spwnd, WFANSIPROC));
    }

    pcbox->fFocus = FALSE;
    xxxCBNotifyParent(pcbox, CBN_KILLFOCUS);

    ThreadUnlock(&tlpwndEdit);
    ThreadUnlock(&tlpwndList);
}


/***************************************************************************\
* xxxCBGetTextLengthHelper
*
* For the combo box without an edit control, returns size of current selected
* item
*
* History:
\***************************************************************************/

LONG xxxCBGetTextLengthHelper(
    PCBOX pcbox,
    BOOL fAnsi)
{
    int item;
    int cchText;
    TL tlpwndList;

    ThreadLock(pcbox->spwndList, &tlpwndList);
    item = (int)SendMessageWorker(pcbox->spwndList, LB_GETCURSEL, 0, 0, fAnsi);

    if (item == LB_ERR) {

        /*
         * No selection so no text.
         */
        cchText = 0;
    } else {
        cchText = (int)SendMessageWorker(pcbox->spwndList, LB_GETTEXTLEN,
                item, 0, fAnsi);
    }

    ThreadUnlock(&tlpwndList);

    return cchText;
}

/***************************************************************************\
* xxxCBGetTextHelper
*
* For the combo box without an edit control, copies cbString bytes of the
* string in the static text box to the buffer given by pString.
*
* History:
\***************************************************************************/

LONG xxxCBGetTextHelper(
    PCBOX pcbox,
    int cchString,
    LPWSTR pString,
    BOOL fAnsi)
{
    int item;
    int cchText;
    LPWSTR pText;
    DWORD dw;
    TL tlpwndList;

    CheckLock(pcbox->spwnd);

    if (!cchString || !pString)
        return 0;

    /*
     * Null the buffer to be nice.
     */
    if (fAnsi) {
        *((LPSTR)pString) = 0;
    } else {
        *((LPWSTR)pString) = 0;
    }

    ThreadLock(pcbox->spwndList, &tlpwndList);
    item = (int)SendMessageWorker(pcbox->spwndList, LB_GETCURSEL, 0, 0, fAnsi);

    if (item == LB_ERR) {

        /*
         * No selection so no text.
         */
        ThreadUnlock(&tlpwndList);
        return 0;
    }

    cchText = (int)SendMessageWorker(pcbox->spwndList, LB_GETTEXTLEN, item, 0, fAnsi);

    cchText++;
    if ((cchText <= cchString) ||
            (!TestWF(pcbox->spwnd, WFWIN31COMPAT) && cchString == 2)) {
        /*
         * Just do the copy if the given buffer size is large enough to hold
         * everything.  Or if old 3.0 app.  (Norton used to pass 2 & expect 3
         * chars including the \0 in 3.0; Bug #7018 win31: vatsanp)
         */
        dw = (int)SendMessageWorker(pcbox->spwndList, LB_GETTEXT, item,
                (LPARAM)pString, fAnsi);
        ThreadUnlock(&tlpwndList);
        return dw;
    }

    if (!(pText = (LPWSTR)UserLocalAlloc(HEAP_ZERO_MEMORY, cchText*sizeof(WCHAR)))) {

        /*
         * Bail.  Not enough memory to chop up the text.
         */
        ThreadUnlock(&tlpwndList);
        return 0;
    }

    try {
        SendMessageWorker(pcbox->spwndList, LB_GETTEXT, item, (LPARAM)pText, fAnsi);
        if (fAnsi) {
            RtlCopyMemory((PBYTE)pString, (PBYTE)pText, cchString);
            ((LPSTR)pString)[cchString - 1] = 0;
        } else {
            RtlCopyMemory((PBYTE)pString, (PBYTE)pText, cchString * sizeof(WCHAR));
            ((LPWSTR)pString)[cchString - 1] = 0;
        }
    } finally {
        UserLocalFree((HANDLE)pText);
    }

    ThreadUnlock(&tlpwndList);
    return cchString;
}
