/****************************** Module Header ******************************\
* Module Name: dragdrop.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Stuff for object-oriented direct manipulation, designed first for the shell.
*
* History:
* 08-06-91 darrinm    Ported from Win 3.1.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

PCURSOR xxxQueryDropObject(PWND pwnd, LPDROPSTRUCT lpds);

/***************************************************************************\
* DragObject (API)
*
* Contains the main dragging loop.
*
* History:
* 08-06-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

DWORD xxxDragObject(
    PWND pwndParent,
    PWND pwndFrom,          // NULL is valid
    UINT wFmt,
    ULONG_PTR dwData,
    PCURSOR pcur)
{
    MSG msg, msgKey;
    DWORD result = 0;
    BOOL fDrag = TRUE;
    LPDROPSTRUCT lpds;
    PWND pwndDragging = NULL;
    PWND pwndTop;
    PCURSOR pcurOld, pcurT;
    PWND pwndT;
    TL tlpwndT;
    TL tlpwndTop;
    TL tlpwndDragging;
    TL tlPool;
    PTHREADINFO pti = PtiCurrent();

    CheckLock(pwndParent);
    CheckLock(pwndFrom);
    CheckLock(pcur);
    UserAssert(IsWinEventNotifyDeferredOK());

    lpds = (LPDROPSTRUCT)UserAllocPoolWithQuota(2 * sizeof(DROPSTRUCT), TAG_DRAGDROP);
    if (lpds == NULL)
        return 0;

    ThreadLockPool(pti, lpds, &tlPool);
    lpds->hwndSource = HW(pwndFrom);
    lpds->wFmt = wFmt;
    lpds->dwData = dwData;

    if (pcur != NULL) {
        /*
         * No need to DeferWinEventNotify() - pwndFrom is locked
         */
        pcurOld = zzzSetCursor(pcur);
    } else {
        pcurOld = pti->pq->spcurCurrent;
    }

    if (pwndFrom) {
        for (pwndTop = pwndFrom; TestwndChild(pwndTop);
                pwndTop = pwndTop->spwndParent) ;

        ThreadLockWithPti(pti, pwndTop, &tlpwndTop);
        xxxUpdateWindow(pwndTop);
        ThreadUnlock(&tlpwndTop);
    }

    if (FWINABLE()) {
        xxxWindowEvent(EVENT_SYSTEM_DRAGDROPSTART, pwndFrom, OBJID_WINDOW, INDEXID_CONTAINER, 0);
    }

    xxxSetCapture(pwndFrom);
    zzzShowCursor(TRUE);

    ThreadLockWithPti(pti, pwndDragging, &tlpwndDragging);

    while (fDrag && pti->pq->spwndCapture == pwndFrom) {
        while (!(xxxPeekMessage(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE) ||
                 xxxPeekMessage(&msg, NULL, WM_QUEUESYNC, WM_QUEUESYNC, PM_REMOVE) ||
                 xxxPeekMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))) {
            if (!xxxSleepThread(QS_MOUSE | QS_KEY, 0, TRUE)) {
                ThreadUnlock(&tlpwndDragging);
                ThreadUnlockAndFreePool(pti, &tlPool);
                return 0;
            }
        }

        /*
         * Be sure to eliminate any extra keydown messages that are
         * being queued up by MOUSE message processing.
         */

        while (xxxPeekMessage(&msgKey, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))
           ;

        if  ( (pti->pq->spwndCapture != pwndFrom) ||
              (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) )
        {
            if (pcurT = SYSCUR(NO))
                zzzSetCursor(pcurT);
            break;
        }

        RtlCopyMemory(lpds + 1, lpds, sizeof(DROPSTRUCT));

        /*
         * in screen coordinates
         */
        lpds->ptDrop = msg.pt;

        pcurT = xxxQueryDropObject(pwndParent, lpds);

        /*
         * Returning FALSE to a WM_QUERYDROPOBJECT message means drops
         * aren't supported and the 'illegal drop target' cursor should be
         * displayed.  Returning TRUE means the target is valid and the
         * regular drag cursor should be displayed.  Also, through a bit
         * of polymorphic magic one can return a cursor handle to override
         * the normal drag cursor.
         */
        if (pcurT == (PCURSOR)FALSE) {
            pcurT = SYSCUR(NO);
            lpds->hwndSink = NULL;
        } else if (pcurT == (PCURSOR)TRUE) {
            pcurT = pcur;
        }

        if (pcurT != NULL)
            zzzSetCursor(pcurT);

        /*
         * send the WM_DRAGLOOP after the above zzzSetCursor() to allow the
         * receiver to change the cursor at WM_DRAGLOOP time with a zzzSetCursor()
         */
        if (pwndFrom) {
            xxxSendMessage(pwndFrom, WM_DRAGLOOP, (pcurT != SYSCUR(NO)),
                    (LPARAM)lpds);
        }

        /*
         * send these messages internally only
         */
        if (pwndDragging != RevalidateHwnd(lpds->hwndSink)) {
            if (pwndDragging != NULL) {
                xxxSendMessage(pwndDragging, WM_DRAGSELECT, FALSE,
                        (LPARAM)(lpds + 1));
            }
            pwndDragging = RevalidateHwnd(lpds->hwndSink);
            ThreadUnlock(&tlpwndDragging);
            ThreadLockWithPti(pti, pwndDragging, &tlpwndDragging);

            if (pwndDragging != NULL) {
                xxxSendMessage(pwndDragging, WM_DRAGSELECT, TRUE, (LPARAM)lpds);
            }
        } else {
            if (pwndDragging != NULL) {
                xxxSendMessage(pwndDragging, WM_DRAGMOVE, 0, (LPARAM)lpds);
            }
        }

        switch (msg.message) {
        case WM_LBUTTONUP:
        case WM_NCLBUTTONUP:
            fDrag = FALSE;
            break;
        }
    }

    ThreadUnlock(&tlpwndDragging);

    /*
     * If the capture has been lost (i.e. fDrag == TRUE), don't do the drop.
     */
    if (fDrag)
        pcurT = SYSCUR(NO);

    /*
     * before the actual drop, clean up the cursor, as the app may do
     * stuff here...
     */
    xxxReleaseCapture();
    zzzShowCursor(FALSE);

    zzzSetCursor(pcurOld);

    /*
     * we either got lbuttonup or enter
     */
    if (pcurT != SYSCUR(NO)) {

        /*
         * object allows drop...  send drop message
         */
        pwndT = ValidateHwnd(lpds->hwndSink);
        if (pwndT != NULL) {

            ThreadLockAlwaysWithPti(pti, pwndT, &tlpwndT);

            /*
             * Allow this guy to activate.
             */
            GETPTI(pwndT)->TIF_flags |= TIF_ALLOWFOREGROUNDACTIVATE;
            TAGMSG1(DBGTAG_FOREGROUND, "xxxDragObject set TIF %#p", GETPTI(pwndT));
            result = (DWORD)xxxSendMessage(pwndT, WM_DROPOBJECT,
                    (WPARAM)HW(pwndFrom), (LPARAM)lpds);

            ThreadUnlock(&tlpwndT);
        }
    }

    if (FWINABLE()) {
        xxxWindowEvent(EVENT_SYSTEM_DRAGDROPEND, pwndFrom, OBJID_WINDOW, INDEXID_CONTAINER, 0);
    }

    ThreadUnlockAndFreePool(pti, &tlPool);
    return result;
}


/***************************************************************************\
* QueryDropObject
*
* Determines where in the window heirarchy the "drop" takes place, and
* sends a message to the deepest child window first.  If that window does
* not respond, we go up the heirarchy (recursively, for the moment) until
* we either get a window that does respond or the parent doesn't respond.
*
* History:
* 08-06-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

PCURSOR xxxQueryDropObject(
    PWND pwnd,
    LPDROPSTRUCT lpds)
{
    PWND pwndT;
    PCURSOR pcurT = NULL;
    POINT pt;
    BOOL fNC;
    TL tlpwndT;
    CheckLock(pwnd);

    /*
     *  pt is in screen coordinates
     */
    pt = lpds->ptDrop;

    /*
     * reject points outside this window or if the window is disabled
     */
    if (!PtInRect(&pwnd->rcWindow, pt) || TestWF(pwnd, WFDISABLED))
        return NULL;

    /*
     * Check to see if in window region (if it has one)
     */
    if (pwnd->hrgnClip != NULL) {
        if (!GrePtInRegion(pwnd->hrgnClip, pt.x, pt.y))
            return NULL;
    }

    /*
     * are we dropping in the nonclient area of the window or on an iconic
     * window?
     */
    if (fNC = (TestWF(pwnd, WFMINIMIZED) || !PtInRect(&pwnd->rcClient, pt))) {
        goto SendQueryDrop;
    }

    /*
     * dropping in client area
     */
    _ScreenToClient(pwnd, &pt);
    pwndT = _ChildWindowFromPointEx(pwnd, pt, CWP_SKIPDISABLED | CWP_SKIPINVISIBLE);
    _ClientToScreen(pwnd, &pt);

    pcurT = NULL;
    if (pwndT && pwndT != pwnd) {
        ThreadLock(pwndT, &tlpwndT);
        pcurT = xxxQueryDropObject(pwndT, lpds);
        ThreadUnlock(&tlpwndT);
    }

    if (pcurT == NULL) {

        /*
         * there are no children who are in the right place or who want
         * drops...  convert the point into client coordinates of the
         * current window.  Because of the recursion, this is already
         * done if a child window grabbed the drop.
         */
SendQueryDrop:
        _ScreenToClient(pwnd, &lpds->ptDrop);
        lpds->hwndSink = HWq(pwnd);

        /*
         * To avoid hanging dropper (sender) app we do a SendMessageTimeout to
         * the droppee (receiver)
         */
        if ((PCURSOR)xxxSendMessageTimeout(pwnd, WM_QUERYDROPOBJECT, fNC,
                (LPARAM)lpds, SMTO_ABORTIFHUNG, 3*1000, (PLONG_PTR)&pcurT) == FALSE)
            pcurT = (PCURSOR)FALSE;

        if (pcurT != (PCURSOR)FALSE && pcurT != (PCURSOR)TRUE)
            pcurT = HMValidateHandle((HCURSOR)pcurT, TYPE_CURSOR);

        /*
         * restore drop point to screen coordinates if this window won't
         * take drops
         */
        if (pcurT == NULL)
            lpds->ptDrop = pt;
    }
    return pcurT;
}


/***************************************************************************\
* xxxDragDetect (API)
*
*
*
* History:
* 08-06-91 darrinm      Ported from Win 3.1 sources.
\***************************************************************************/

BOOL xxxDragDetect(
    PWND pwnd,
    POINT pt)
{
    return xxxIsDragging(pwnd, pt, WM_LBUTTONUP);
}

/***************************************************************************\
* xxxIsDragging
*
*
*
* History:
* 05-17-94 johnl        Ported from Chicago sources
\***************************************************************************/

BOOL xxxIsDragging(PWND pwnd, POINT ptScreen, UINT uMsg)
{
    RECT rc;
    MSG  msg;
    BOOL fDragging;
    BOOL fCheck;
    TL   tlpwndDragging;
    PTHREADINFO pti = PtiCurrent();

    /*
     * Check synchronous mouse state, and punt if the mouse isn't down
     * according to the queue.
     */
    if (!(_GetKeyState((uMsg == WM_LBUTTONUP ? VK_LBUTTON : VK_RBUTTON)) & 0x8000))
        return FALSE;

    xxxSetCapture(pwnd);

    *(LPPOINT)&rc.left = ptScreen;
    *(LPPOINT)&rc.right = ptScreen;
    InflateRect(&rc, SYSMET(CXDRAG), SYSMET(CYDRAG));

    fDragging = FALSE;
    fCheck    = TRUE;

    ThreadLockWithPti(pti, pwnd, &tlpwndDragging);
    while (fCheck) {
        while ( !(
                  xxxPeekMessage(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST,PM_REMOVE) ||
                  xxxPeekMessage(&msg, NULL, WM_QUEUESYNC, WM_QUEUESYNC,PM_REMOVE) ||
                  xxxPeekMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST,PM_REMOVE)
                 )
               && (pti->pq->spwndCapture == pwnd)) {
            /*
             * If there is no input for half a second (500ms) consider that
             * we are dragging. If we don't specify a timeout value, the
             * thread may sleep here forever and wouldn't repaint, etc.
             */
            if (!xxxSleepThread(QS_MOUSE | QS_KEY, 500, TRUE)) {
                fDragging = TRUE;
                goto Cleanup;
            }
        }

        /*
         * Cancel if the button was released or we no longer have the capture.
         */
        if ( pti->pq->spwndCapture != pwnd || msg.message == uMsg) {
            fCheck = FALSE;
        } else {
            switch (msg.message) {

            case WM_MOUSEMOVE:
                if (!PtInRect(&rc, msg.pt)) {
                    fDragging = TRUE;
                    fCheck    = FALSE;
                }
                break;

            case WM_QUEUESYNC:
                /*
                 * CBT Hook needs to know
                 */
                xxxCallHook(HCBT_QS, 0, 0, WH_CBT);
                break;

            case WM_KEYDOWN:
                /*
                 * <Esc> cancels drag detection
                 */
                if (msg.wParam == VK_ESCAPE)
                    fCheck = FALSE;
                break;

            }
        }
    }

Cleanup:
    if (pti->pq->spwndCapture == pwnd)
        xxxReleaseCapture();

    ThreadUnlock(&tlpwndDragging);
    return fDragging ;
}
