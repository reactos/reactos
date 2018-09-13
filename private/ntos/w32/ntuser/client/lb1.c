/****************************** Module Header ********************************\
* Module Name: lb1.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* ListBox routines
*
* History:
* ??-???-???? ianja    Ported from Win 3.0 sources
* 14-Feb-1991 mikeke   Added Revalidation code
\*****************************************************************************/

#include "precomp.h"
#pragma hdrstop

LOOKASIDE ListboxLookaside;


/***************************************************************************\
* xxxLBoxCtlWndProc
*
* Window Procedure for ListBox AND ComboLBox control.
* NOTE: All window procedures are APIENTRY
* WARNING: This listbox code contains some internal messages and styles which
* are defined in combcom.h and in combcom.inc.  They may be redefined
* (or renumbered) as needed to extend the windows API.
*
* History:
* 16-Apr-1992 beng      Added LB_SETCOUNT
\***************************************************************************/

LRESULT APIENTRY ListBoxWndProcWorker(
    PWND pwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    DWORD fAnsi)
{
    HWND hwnd = HWq(pwnd);
    PAINTSTRUCT ps;
    HDC         hdc;
    LPRECT      lprc;
    PLBIV plb;    /* List Box Instance Variable */
    INT iSel;     /* Index of selected item */
    DWORD dw;
    TL tlpwndParent;
    UINT wFlags;
    LPWSTR lpwsz = NULL;
    LRESULT lReturn = 0;
    static BOOL fInit = TRUE;

    CheckLock(pwnd);

    VALIDATECLASSANDSIZE(pwnd, FNID_LISTBOX);
    INITCONTROLLOOKASIDE(&ListboxLookaside, LBIV, spwnd, 4);

    /*
     * Get the plb for the given window now since we will use it a lot in
     * various handlers. This was stored using SetWindowLong(hwnd,0,plb)
     * when the listbox was first created (by INITCONTROLLOOKASIDE above)
     */
    plb = ((PLBWND)pwnd)->pLBIV;

    /*
     * Handle ANSI translations of input parameters
     */
    if (fAnsi) {
        switch (message) {
        case LB_ADDSTRING:
        case LB_ADDSTRINGUPPER:
        case LB_ADDSTRINGLOWER:
        case LB_FINDSTRING:
        case LB_FINDSTRINGEXACT:
        case LB_INSERTSTRING:
        case LB_INSERTSTRINGUPPER:
        case LB_INSERTSTRINGLOWER:
        case LB_SELECTSTRING:
            if (!plb->fHasStrings) {
                break;
            }
            // Fall through...
        case LB_ADDFILE:
        case LB_DIR:
            if (lParam) {
                if (!MBToWCS((LPSTR)lParam, -1, &lpwsz, -1, TRUE))
                    return LB_ERR;
            }
            break;
        default:
            break;
        }
        if (lpwsz) {
            lParam = (LPARAM)lpwsz;
        }
    }

    switch (message) {

    case LB_GETTOPINDEX:        // Return index of top item displayed.
        return plb->iTop;

    case LB_SETTOPINDEX:
        if (wParam && ((INT)wParam < 0 || (INT)wParam >= plb->cMac)) {
            RIPERR0(ERROR_INVALID_INDEX, RIP_VERBOSE, "");
            return LB_ERR;
        }
        if (plb->cMac) {
            xxxNewITop(plb, (INT)wParam);
        }
        break;

    case WM_STYLECHANGED:
        plb->fRtoLReading = (TestWF(pwnd, WEFRTLREADING) != 0);
        plb->fRightAlign  = (TestWF(pwnd, WEFRIGHT) != 0);
        xxxCheckRedraw(plb, FALSE, 0);
        break;

    case WM_WINDOWPOSCHANGED:

        /*
         * If we are in the middle of creation, ignore this
         * message because it will generate a WM_SIZE message.
         * See xxxLBCreate().
         */
        if (!plb->fIgnoreSizeMsg)
            goto CallDWP;
        break;

    case WM_SIZE:

        /*
         * If we are in the middle of creation, ignore size
         * messages.  See xxxLBCreate().
         */
        if (!plb->fIgnoreSizeMsg)
            xxxLBSize(plb, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        break;

    case WM_ERASEBKGND:
        ThreadLock(plb->spwndParent, &tlpwndParent);
        FillWindow(HW(plb->spwndParent), hwnd, (HDC)wParam,
                (HBRUSH)CTLCOLOR_LISTBOX);
        ThreadUnlock(&tlpwndParent);
        return TRUE;

    case LB_RESETCONTENT:
        xxxLBResetContent(plb);
        break;

    case WM_TIMER:
        if (wParam == IDSYS_LBSEARCH) {
            plb->iTypeSearch = 0;
            NtUserKillTimer(hwnd, IDSYS_LBSEARCH);
            xxxInvertLBItem(plb, plb->iSel, TRUE);
            break;
        }

        message = WM_MOUSEMOVE;
        xxxTrackMouse(plb, message, plb->ptPrev);
        break;

        /*
         * Fall through
         */
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
        {
            POINT pt;

            POINTSTOPOINT(pt, lParam);
            xxxTrackMouse(plb, message, pt);
        }
        break;

    case WM_MBUTTONDOWN:
        EnterReaderModeHelper(hwnd);
        break;

    case WM_CAPTURECHANGED:
            //
            // Note that this message should be handled only on unexpected
            // capture changes currently.
            //
        UserAssert(TestWF(pwnd, WFWIN40COMPAT));
        if (plb->fCaptured)
            xxxLBButtonUp(plb, LBUP_NOTIFY);
        break;

    case LBCB_STARTTRACK:
        //
        // Start tracking mouse moves in the listbox, setting capture
        //
        if (!plb->pcbox)
            break;

        plb->fCaptured = FALSE;
        if (wParam) {
            POINT pt;

            POINTSTOPOINT(pt, lParam);

            _ScreenToClient(pwnd, &pt);
            xxxTrackMouse(plb, WM_LBUTTONDOWN, pt);
        } else {
            NtUserSetCapture(hwnd);
            plb->fCaptured = TRUE;
            plb->iLastSelection = plb->iSel;
        }
        break;

    case LBCB_ENDTRACK:
        // Kill capture, tracking, etc.
        if (plb->fCaptured)
            xxxLBButtonUp(plb, LBUP_RELEASECAPTURE | (wParam ? LBUP_SELCHANGE :
                LBUP_RESETSELECTION));
        break;

    case WM_PRINTCLIENT:
        xxxLBPaint(plb, (HDC) wParam, NULL);
        break;

    case WM_PAINT:
        if (wParam) {
            hdc = (HDC) wParam;
            lprc = NULL;
        } else {
            hdc = NtUserBeginPaint(hwnd, &ps);
            lprc = &(ps.rcPaint);
        }

        if (IsLBoxVisible(plb))
            xxxLBPaint(plb, hdc, lprc);

        if (!wParam)
            NtUserEndPaint(hwnd, &ps);
        break;

    case WM_NCDESTROY:
    case WM_FINALDESTROY:
        xxxDestroyLBox(plb, pwnd);
        break;

    case WM_SETFOCUS:
// DISABLED in Win 3.1        xxxUpdateWindow(pwnd);
        CaretCreate(plb);
        xxxLBSetCaret(plb, TRUE);
        xxxNotifyOwner(plb, LBN_SETFOCUS);
        if (FWINABLE()) {
            if (_IsWindowVisible(pwnd)) {
                LBEvent(plb, EVENT_OBJECT_FOCUS, plb->iSelBase);
            }
        }
        break;

    case WM_KILLFOCUS:
        /*
         * Reset the wheel delta count.
         */
        gcWheelDelta = 0;

        xxxLBSetCaret(plb, FALSE);
        xxxCaretDestroy(plb);
        xxxNotifyOwner(plb, LBN_KILLFOCUS);
        if (plb->iTypeSearch) {
            plb->iTypeSearch = 0;
            NtUserKillTimer(hwnd, IDSYS_LBSEARCH);
        }
        if (plb->pszTypeSearch) {
            UserLocalFree(plb->pszTypeSearch);
            plb->pszTypeSearch = NULL;
        }
        break;

    case WM_MOUSEWHEEL:
        {
            int     cDetants;
            int     cPage;
            int     cLines;
            RECT    rc;
            int     windowWidth;
            int     cPos;

            /*
             * Don't handle zoom and datazoom.
             */
            if (wParam & (MK_SHIFT | MK_CONTROL)) {
                goto CallDWP;
            }

            lReturn = 1;
            gcWheelDelta -= (short) HIWORD(wParam);
            cDetants = gcWheelDelta / WHEEL_DELTA;
            if (    cDetants != 0 &&
                    gpsi->ucWheelScrollLines > 0 &&
                    (pwnd->style & (WS_VSCROLL | WS_HSCROLL))) {

                gcWheelDelta = gcWheelDelta % WHEEL_DELTA;

                if (pwnd->style & WS_VSCROLL) {
                    cPage = max(1, (plb->cItemFullMax - 1));
                    cLines = cDetants *
                            (int) min((UINT) cPage, gpsi->ucWheelScrollLines);

                    cPos = max(0, min(plb->iTop + cLines, plb->cMac - 1));
                    if (cPos != plb->iTop) {
                        xxxLBoxCtlScroll(plb, SB_THUMBPOSITION, cPos);
                        xxxLBoxCtlScroll(plb, SB_ENDSCROLL, 0);
                    }
                } else if (plb->fMultiColumn) {
                    cPage = max(1, plb->numberOfColumns);
                    cLines = cDetants * (int) min((UINT) cPage, gpsi->ucWheelScrollLines);
                    cPos = max(
                            0,
                            min((plb->iTop / plb->itemsPerColumn) + cLines,
                                plb->cMac - 1 - ((plb->cMac - 1) % plb->itemsPerColumn)));

                    if (cPos != plb->iTop) {
                        xxxLBoxCtlHScrollMultiColumn(plb, SB_THUMBPOSITION, cPos);
                        xxxLBoxCtlHScrollMultiColumn(plb, SB_ENDSCROLL, 0);
                    }
                } else {
                    _GetClientRect(plb->spwnd, &rc);
                    windowWidth = rc.right;
                    cPage = max(plb->cxChar, (windowWidth / 3) * 2) /
                            plb->cxChar;

                    cLines = cDetants *
                            (int) min((UINT) cPage, gpsi->ucWheelScrollLines);

                    cPos = max(
                            0,
                            min(plb->xOrigin + (cLines * plb->cxChar),
                                plb->maxWidth));

                    if (cPos != plb->xOrigin) {
                        xxxLBoxCtlHScroll(plb, SB_THUMBPOSITION, cPos);
                        xxxLBoxCtlHScroll(plb, SB_ENDSCROLL, 0);
                    }
                }
            }
        }
        break;

    case WM_VSCROLL:
        xxxLBoxCtlScroll(plb, LOWORD(wParam), HIWORD(wParam));
        break;

    case WM_HSCROLL:
        xxxLBoxCtlHScroll(plb, LOWORD(wParam), HIWORD(wParam));
        break;

    case WM_GETDLGCODE:
        return DLGC_WANTARROWS | DLGC_WANTCHARS;

    case WM_CREATE:
        return xxxLBCreate(plb, pwnd, (LPCREATESTRUCT) lParam);

    case WM_SETREDRAW:

        /*
         * If wParam is nonzero, the redraw flag is set
         * If wParam is zero, the flag is cleared
         */
        xxxLBSetRedraw(plb, (wParam != 0));
        break;

    case WM_ENABLE:
        xxxLBInvalidateRect(plb, NULL, !plb->OwnerDraw);
        break;

    case WM_SETFONT:
        xxxLBSetFont(plb, (HANDLE)wParam, LOWORD(lParam));
        break;

    case WM_GETFONT:
        return (LRESULT)plb->hFont;

    case WM_DRAGSELECT:
    case WM_DRAGLOOP:
    case WM_DRAGMOVE:
    case WM_DROPFILES:
        ThreadLock(plb->spwndParent, &tlpwndParent);
        lReturn = SendMessage(HW(plb->spwndParent), message, wParam, lParam);
        ThreadUnlock(&tlpwndParent);
        return lReturn;


    case WM_QUERYDROPOBJECT:
    case WM_DROPOBJECT:

        /*
         * fix up control data, then pass message to parent
         */
        LBDropObjectHandler(plb, (PDROPSTRUCT)lParam);
        ThreadLock(plb->spwndParent, &tlpwndParent);
        lReturn = SendMessage(HW(plb->spwndParent), message, wParam, lParam);
        ThreadUnlock(&tlpwndParent);
        return lReturn;

    case LB_GETITEMRECT:
        return LBGetItemRect(plb, (INT)wParam, (LPRECT)lParam);

    case LB_GETITEMDATA:
        return LBGetItemData(plb, (INT)wParam);  // wParam = item index

    case LB_SETITEMDATA:

        /*
         * wParam is item index
         */
        return LBSetItemData(plb, (INT)wParam, lParam);

    case LB_ADDSTRINGUPPER:
        wFlags = UPPERCASE | LBI_ADD;
        goto CallInsertItem;

    case LB_ADDSTRINGLOWER:
        wFlags = LOWERCASE | LBI_ADD;
        goto CallInsertItem;

    case LB_ADDSTRING:
        wFlags = LBI_ADD;
        goto CallInsertItem;

    case LB_INSERTSTRINGUPPER:
        wFlags = UPPERCASE;
        goto CallInsertItem;

    case LB_INSERTSTRINGLOWER:
        wFlags = LOWERCASE;
        goto CallInsertItem;

    case LB_INSERTSTRING:
        wFlags = 0;
CallInsertItem:
        lReturn = ((LRESULT) xxxLBInsertItem(plb, (LPWSTR) lParam, (int) wParam, wFlags));
        break;

    case LB_INITSTORAGE:
        return xxxLBInitStorage(plb, fAnsi, (INT)wParam, (INT)lParam);

    case LB_DELETESTRING:
        return xxxLBoxCtlDelete(plb, (INT)wParam);

    case LB_DIR:
        /*
         * wParam - Dos attribute value.
         * lParam - Points to a file specification string
         */
        lReturn = xxxLbDir(plb, (INT)wParam, (LPWSTR)lParam);
        break;

    case LB_ADDFILE:
        lReturn = xxxLbInsertFile(plb, (LPWSTR)lParam);
        break;

    case LB_SETSEL:
        return xxxLBSetSel(plb, (wParam != 0), (INT)lParam);

    case LB_SETCURSEL:
        /*
         * If window obscured, update so invert will work correctly
         */
// DISABLED in Win 3.1        xxxUpdateWindow(pwnd);
        return xxxLBSetCurSel(plb, (INT)wParam);

    case LB_GETSEL:
        if (wParam >= (UINT) plb->cMac)
                return((LRESULT) LB_ERR);

        return IsSelected(plb, (INT)wParam, SELONLY);

    case LB_GETCURSEL:
        if (plb->wMultiple == SINGLESEL) {
            return plb->iSel;
        }
        return plb->iSelBase;

    case LB_SELITEMRANGE:
        if (plb->wMultiple == SINGLESEL) {
            /*
             * Can't select a range if only single selections are enabled
             */
            RIPERR0(ERROR_INVALID_INDEX, RIP_VERBOSE,"Invalid index passed to LB_SELITEMRANGE");
            return LB_ERR;
        }

        xxxLBSelRange(plb, LOWORD(lParam), HIWORD(lParam), (wParam != 0));
        break;

    case LB_SELITEMRANGEEX:
        if (plb->wMultiple == SINGLESEL) {
            /*
             * Can't select a range if only single selections are enabled
             */
            RIPERR0(ERROR_INVALID_LB_MESSAGE, RIP_VERBOSE,"LB_SELITEMRANGEEX:Can't select a range if only single selections are enabled");
            return LB_ERR;
        } else {
            BOOL fHighlight = ((DWORD)lParam > (DWORD)wParam);
            if (fHighlight == FALSE) {
                ULONG_PTR temp = lParam;
                lParam = wParam;
                wParam = temp;
            }
            xxxLBSelRange(plb, (INT)wParam, (INT)lParam, fHighlight);
        }
        break;

    case LB_GETTEXTLEN:
        if (lParam != 0) {
            RIPMSG1(RIP_WARNING, "LB_GETTEXTLEN with lParam = %lx\n", lParam);
        }
        lReturn = LBGetText(plb, TRUE, fAnsi, (INT)wParam, NULL);
        break;

    case LB_GETTEXT:
        lReturn = LBGetText(plb, FALSE, fAnsi, (INT)wParam, (LPWSTR)lParam);
        break;

    case LB_GETCOUNT:
        // Lotus Approach calls CallWndProc(ListWndProc, LB_GETCOUNT,...)
        // on a window that doesn't have a plb yet. So, we need to make
        // this check. Bug #6675 - 11/7/94 --
        if(plb)
            return((LRESULT) plb->cMac);
        else
            return(0);

    case LB_SETCOUNT:
        return xxxLBSetCount(plb, (INT)wParam);

    case LB_SELECTSTRING:
    case LB_FINDSTRING:
        iSel = xxxFindString(plb, (LPWSTR)lParam, (INT)wParam, PREFIX, TRUE);
        if (message == LB_FINDSTRING || iSel == LB_ERR) {
            lReturn = iSel;
        } else {
            lReturn = xxxLBSetCurSel(plb, iSel);
        }
        break;

    case LB_GETLOCALE:
        return plb->dwLocaleId;

    case LB_SETLOCALE:

        /*
         * Validate locale
         */
        wParam = ConvertDefaultLocale((LCID)wParam);
        if (!IsValidLocale((LCID)wParam, LCID_INSTALLED))
            return LB_ERR;

        dw = plb->dwLocaleId;
        plb->dwLocaleId = (DWORD)wParam;
        return dw;

    case WM_KEYDOWN:

        /*
         * IanJa: Use LOWORD() to get low 16-bits of wParam - this should
         * work for Win16 & Win32.  The value obtained is the virtual key
         */
        xxxLBoxCtlKeyInput(plb, message, LOWORD(wParam));
        break;

    case WM_CHAR:
        xxxLBoxCtlCharInput(plb, LOWORD(wParam), fAnsi);
        break;

    case LB_GETSELITEMS:
    case LB_GETSELCOUNT:

        /*
         * IanJa/Win32 should this be LPWORD now?
         */
        return LBoxGetSelItems(plb, (message == LB_GETSELCOUNT), (INT)wParam,
                (LPINT)lParam);

    case LB_SETTABSTOPS:

        /*
         * IanJa/Win32: Tabs given by array of INT for backwards compatability
         */
        return LBSetTabStops(plb, (INT)wParam, (LPINT)lParam);

    case LB_GETHORIZONTALEXTENT:

        /*
         * Return the max width of the listbox used for horizontal scrolling
         */
        return plb->maxWidth;

    case LB_SETHORIZONTALEXTENT:

        /*
         * Set the max width of the listbox used for horizontal scrolling
         */
        if (plb->maxWidth != (INT)wParam) {
            plb->maxWidth = (INT)wParam;

            /*
             * When horizontal extent is set, Show/hide the scroll bars.
             * NOTE: LBShowHideScrollBars() takes care if Redraw is OFF.
             * Fix for Bug #2477 -- 01/14/91 -- SANKAR --
             */
            xxxLBShowHideScrollBars(plb); //Try to show or hide scroll bars
            if (plb->fHorzBar && plb->fRightAlign && !(plb->fMultiColumn || plb->OwnerDraw)) {
                /*
                 * origin to right
                 */
                xxxLBoxCtlHScroll(plb, SB_BOTTOM, 0);
            }
        }
        break;    /* originally returned register ax (message) ! */

    case LB_SETCOLUMNWIDTH:

        /*
         * Set the width of a column in a multicolumn listbox
         */
        plb->cxColumn = (INT)wParam;
        LBCalcItemRowsAndColumns(plb);
        if (IsLBoxVisible(plb))
            NtUserInvalidateRect(hwnd, NULL, TRUE);
        xxxLBShowHideScrollBars(plb);
        break;

    case LB_SETANCHORINDEX:
        if ((INT)wParam >= plb->cMac) {
            RIPERR0(ERROR_INVALID_INDEX, RIP_VERBOSE,"Invalid index passed to LB_SETANCHORINDEX");
            return LB_ERR;
        }
        plb->iMouseDown = (INT)wParam;
        plb->iLastMouseMove = (INT)wParam;
        xxxInsureVisible(plb, (int) wParam, (BOOL)(lParam != 0));
        break;

    case LB_GETANCHORINDEX:
        return plb->iMouseDown;

    case LB_SETCARETINDEX:
        if ( (plb->iSel == -1) || ((plb->wMultiple != SINGLESEL) &&
                    (plb->cMac > (INT)wParam))) {

            /*
             * Set's the iSelBase to the wParam
             * if lParam, then don't scroll if partially visible
             * else scroll into view if not fully visible
             */
            xxxInsureVisible(plb, (INT)wParam, (BOOL)LOWORD(lParam));
            xxxSetISelBase(plb, (INT)wParam);
            break;
        } else {
            if ((INT)wParam >= plb->cMac) {
                RIPERR0(ERROR_INVALID_INDEX, RIP_VERBOSE,"Invalid index passed to LB_SETCARETINDEX");
            }
            return LB_ERR;
        }
        break;

    case LB_GETCARETINDEX:
        return plb->iSelBase;

    case LB_SETITEMHEIGHT:
    case LB_GETITEMHEIGHT:
        return LBGetSetItemHeightHandler(plb, message, (INT)wParam, LOWORD(lParam));
        break;

    case LB_FINDSTRINGEXACT:
        lReturn = xxxFindString(plb, (LPWSTR)lParam, (INT)wParam, EQ, TRUE);
        break;

    case LB_ITEMFROMPOINT: {
        POINT pt;
        BOOL bOutside;
        DWORD dwItem;

        POINTSTOPOINT(pt, lParam);
        bOutside = ISelFromPt(plb, pt, &dwItem);
        UserAssert(bOutside == 1 || bOutside == 0);
        return (LRESULT)MAKELONG(dwItem, bOutside);
    }

    case LBCB_CARETON:

        /*
         * Internal message for combo box support
         */
        CaretCreate(plb);
        // Set up the caret in the proper location for drop downs.
        plb->iSelBase = plb->iSel;
        xxxLBSetCaret(plb, TRUE);
        if (FWINABLE()) {
            if (_IsWindowVisible(pwnd)) {
                LBEvent(plb, EVENT_OBJECT_FOCUS, plb->iSelBase);
            }
        }
        return(plb->iSel);

    case LBCB_CARETOFF:

        /*
         * Internal message for combo box support
         */
        xxxLBSetCaret(plb, FALSE);
        xxxCaretDestroy(plb);
        break;

    case WM_NCCREATE:
        if ((pwnd->style & LBS_MULTICOLUMN) && (pwnd->style & WS_VSCROLL))
        {
            DWORD mask = WS_VSCROLL;
            DWORD flags = 0;
            if (!TestWF(pwnd, WFWIN40COMPAT)) {
                mask |= WS_HSCROLL;
                flags = WS_HSCROLL;
            }
            NtUserAlterWindowStyle(hwnd, mask, flags);
        }
        goto CallDWP;

     default:
CallDWP:
        return DefWindowProcWorker(pwnd, message, wParam, lParam, fAnsi);
    }

    /*
     * Handle translation of ANSI output data and free buffer
     */
    if (lpwsz) {
        UserLocalFree(lpwsz);
    }

    return lReturn;
}


/***************************************************************************\
\***************************************************************************/

LRESULT WINAPI ListBoxWndProcA(
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
    if (!FWINDOWMSG(message, FNID_LISTBOX))
        return DefWindowProcWorker(pwnd, message, wParam, lParam, TRUE);

    return ListBoxWndProcWorker(pwnd, message, wParam, lParam, TRUE);
}

LRESULT WINAPI ListBoxWndProcW(
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
    if (!FWINDOWMSG(message, FNID_LISTBOX))
        return DefWindowProcWorker(pwnd, message, wParam, lParam, FALSE);

    return ListBoxWndProcWorker(pwnd, message, wParam, lParam, FALSE);
}

LRESULT WINAPI ComboListBoxWndProcA(
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
    if (!FWINDOWMSG(message, FNID_LISTBOX))
        return DefWindowProcWorker(pwnd, message, wParam, lParam, TRUE);

    return ListBoxWndProcWorker(pwnd, message, wParam, lParam, TRUE);
}

LRESULT WINAPI ComboListBoxWndProcW(
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
    if (!FWINDOWMSG(message, FNID_LISTBOX))
        return DefWindowProcWorker(pwnd, message, wParam, lParam, FALSE);

    return ListBoxWndProcWorker(pwnd, message, wParam, lParam, FALSE);
}


/***************************************************************************\
* GetLpszItem
*
* Returns a far pointer to the string belonging to item sItem
* ONLY for Listboxes maintaining their own strings (pLBIV->fHasStrings == TRUE)
*
* History:
\***************************************************************************/

LPWSTR GetLpszItem(
    PLBIV pLBIV,
    INT sItem)
{
    LONG offsz;
    lpLBItem plbi;

    if (sItem < 0 || sItem >= pLBIV->cMac) {
        RIPERR1(ERROR_INVALID_PARAMETER,
                RIP_WARNING,
                "Invalid parameter \"sItem\" (%ld) to GetLpszItem",
                sItem);

        return NULL;
    }

    /*
     * get pointer to item index array
     * NOTE: NOT OWNERDRAW
     */
    plbi = (lpLBItem)(pLBIV->rgpch);
    offsz = plbi[sItem].offsz;
    return (LPWSTR)((PBYTE)(pLBIV->hStrings) + offsz);
}
