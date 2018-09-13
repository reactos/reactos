/****************************** Module Header ******************************\
* Module Name: cursor.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains code for dealing with cursors.
*
* History:
* 03-Dec-1990 DavidPe   Created.
* 01-Feb-1991 MikeKe    Added Revalidation code (None)
* 12-Feb-1991 JimA      Added access checks
* 21-Jan-1992 IanJa     ANSI/Unicode neutralized (null op)
* 02-Aug-1992 DarrinM   Added animated cursor code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* zzzSetCursor (API)
*
* This API sets the cursor image for the current thread.
*
* History:
* 12-03-90 DavidPe      Created.
\***************************************************************************/

PCURSOR zzzSetCursor(
    PCURSOR pcur)
{
    PQ      pq;
    PCURSOR pcurPrev;
    PTHREADINFO  ptiCurrent = PtiCurrent();

    pq = ptiCurrent->pq;

    pcurPrev = pq->spcurCurrent;

    if (pq->spcurCurrent != pcur) {

        /*
         * Lock() returns pobjOld - if it is still valid.  Don't want to
         * return a pcurPrev that is an invalid pointer.
         */
        pcurPrev = LockQCursor(pq, pcur);

        /*
         * If no thread 'owns' the cursor, we must be in initialization
         * so go ahead and assign it to ourself.
         */
        if (gpqCursor == NULL)
            gpqCursor = pq;

        /*
         * If we're changing the local-cursor for the thread currently
         * representing the global-cursor, update the cursor image now.
         */
        if (pq == gpqCursor) {
            TL tlpcur;
            ThreadLockWithPti(ptiCurrent, pcurPrev, &tlpcur);
            zzzUpdateCursorImage();
            pcurPrev = ThreadUnlock(&tlpcur);
        }
    }

    return pcurPrev;
}

/***************************************************************************\
* zzzSetCursorPos (API)
*
* This API sets the cursor position.
*
* History:
* 03-Dec-1990 DavidPe  Created.
* 12-Feb-1991 JimA     Added access check
* 16-May-1991 mikeke   Changed to return BOOL
\***************************************************************************/

BOOL zzzSetCursorPos(
    int x,
    int y)
{
    /*
     * Blow it off if the caller doesn't have the proper access rights
     */
    if (!CheckWinstaWriteAttributesAccess()) {
        return FALSE;
    }

    zzzInternalSetCursorPos(x, y);

    /*
     * Save the absolute coordinates in the global array
     * for GetMouseMovePointsEx.
     */
    SAVEPOINT(x, y,
              SYSMET(CXVIRTUALSCREEN) - 1,
              SYSMET(CYVIRTUALSCREEN) - 1,
              NtGetTickCount(), 0);

    return TRUE;
}

/***************************************************************************\
* zzzInternalSetCursorPos
*
* This function is used whenever the server needs to set the cursor
* position, regardless of the caller's access rights.
*
* History:
* 12-Feb-1991 JimA      Created.
\***************************************************************************/
VOID zzzInternalSetCursorPos(
    int      x,
    int      y
    )
{

    gptCursorAsync.x = x;
    gptCursorAsync.y = y;

    BoundCursor(&gptCursorAsync);
    gpsi->ptCursor = gptCursorAsync;
    GreMovePointer(gpDispInfo->hDev, gpsi->ptCursor.x, gpsi->ptCursor.y);

    /*
     * Cursor has changed position, so generate a mouse event so the
     * window underneath the new location knows it's there and sets the
     * shape accordingly.
     */
    zzzSetFMouseMoved();
}

/***************************************************************************\
* IncCursorLevel
* DecCursorLevel
*
* Keeps track of this thread show/hide cursor level as well as the queue
* it is associated with. Thread levels are done so that when
* AttachThreadInput() is called we can do exact level calculations in the
* new queue.
*
* 15-Jan-1993 ScottLu   Created.
\***************************************************************************/

VOID IncCursorLevel(
    PTHREADINFO pti)
{
    pti->iCursorLevel++;
    pti->pq->iCursorLevel++;
}

VOID DecCursorLevel(
    PTHREADINFO pti)
{
    pti->iCursorLevel--;
    pti->pq->iCursorLevel--;
}

/***************************************************************************\
* zzzShowCursor (API)
*
* This API allows the application to hide or show the cursor image.
*
* History:
* 03-Dec-1990 JimA      Implemented for fake cursor stuff
\***************************************************************************/

int zzzShowCursor(
    BOOL fShow)
{
    PTHREADINFO pti = PtiCurrent();
    PQ          pq;
    int         iCursorLevel;

    pq = pti->pq;
    /*
     * To preserve pq
     */
    DeferWinEventNotify();

    if (fShow) {

        IncCursorLevel(pti);

        if ((pq == gpqCursor) && (pq->iCursorLevel == 0))
            zzzUpdateCursorImage();

    } else {

        DecCursorLevel(pti);

        if ((pq == gpqCursor) && (pq->iCursorLevel == -1))
            zzzUpdateCursorImage();
    }

    iCursorLevel = pq->iCursorLevel;
    zzzEndDeferWinEventNotify();

    return iCursorLevel;
}

/***************************************************************************\
* zzzClipCursor (API)
*
* This API sets the cursor clipping rectangle which restricts where the
* cursor can go.  If prcClip is NULL, the clipping rectangle will be the
* screen.
*
* History:
* 03-Dec-1990 DavidPe   Created.
* 16-May-1991 MikeKe    Changed to return BOOL
\***************************************************************************/

BOOL zzzClipCursor(
    LPCRECT prcClip)
{
    PEPROCESS Process = PsGetCurrentProcess();

    /*
     * Don't let this happen if it doesn't have access.
     */
    if (Process != gpepCSRSS && !CheckWinstaWriteAttributesAccess()) {
        return FALSE;
    }

    /*
     * The comment from NT 3.51:
     *     Non-foreground threads can only set the clipping rectangle
     *     if it was empty, or if they are restoring it to the whole screen.
     *
     * But the code from NT 3.51 says "IsRectEmpty" instead of
     * "!IsRectEmpty", as would follow from the comment. We leave
     * the code as it was, as following the comment appears to
     * break apps.
     *
     * CONSIDER: Removing this test altogether after NT4.0 ships.
     */
    if (    PtiCurrent()->pq != gpqForeground &&
            prcClip != NULL &&
            IsRectEmpty(&grcCursorClip)) {
        RIPERR0(ERROR_ACCESS_DENIED, RIP_WARNING, "Access denied in _ClipCursor");
        return FALSE;
    }

    if (prcClip == NULL) {

        grcCursorClip = gpDispInfo->rcScreen;

    } else {

        /*
         * Never let our cursor leave the screen. Can't use IntersectRect()
         * because it doesn't allow rects with 0 width or height.
         */
        grcCursorClip.left   = max(gpDispInfo->rcScreen.left  , prcClip->left);
        grcCursorClip.right  = min(gpDispInfo->rcScreen.right , prcClip->right);
        grcCursorClip.top    = max(gpDispInfo->rcScreen.top   , prcClip->top);
        grcCursorClip.bottom = min(gpDispInfo->rcScreen.bottom, prcClip->bottom);

        /*
         * Check for invalid clip rect.
         */
        if (grcCursorClip.left > grcCursorClip.right ||
            grcCursorClip.top > grcCursorClip.bottom) {

            grcCursorClip = gpDispInfo->rcScreen;
        }
    }

    /*
     * Update the cursor position if it's currently outside the
     * cursor clip-rect.
     */
    if (!PtInRect(&grcCursorClip, gpsi->ptCursor)) {
        zzzInternalSetCursorPos(gpsi->ptCursor.x, gpsi->ptCursor.y);
    }

    return TRUE;
}

/***************************************************************************\
* BoundCursor
*
* This rountine 'clips' gptCursorAsync to be within rcCursorClip.  This
* routine treats rcCursorClip as non-inclusive so the bottom and right sides
* get bound to rcCursorClip.bottom/right - 1.
*
* Is called in OR out of the USER critical section!! IANJA
*
* History:
* 03-Dec-1990 DavidPe   Created.
\***************************************************************************/
#ifdef LOCK_MOUSE_CODE
#pragma alloc_text(MOUSE, BoundCursor)
#endif

VOID BoundCursor(
    LPPOINT lppt)
{
    if (TEST_PUDF(PUDF_VDMBOUNDSACTIVE) && gspwndFullScreen != NULL) {

        if (lppt->x < grcVDMCursorBounds.left) {
            lppt->x = grcVDMCursorBounds.left;
        } else if (lppt->x >= grcVDMCursorBounds.right) {
            lppt->x = grcVDMCursorBounds.right - 1;
        }

        if (lppt->y < grcVDMCursorBounds.top) {
            lppt->y = grcVDMCursorBounds.top;
        } else if (lppt->y >= grcVDMCursorBounds.bottom) {
            lppt->y = grcVDMCursorBounds.bottom - 1;
        }

    } else {

        if (lppt->x < grcCursorClip.left) {
            lppt->x = grcCursorClip.left;
        } else if (lppt->x >= grcCursorClip.right) {
            lppt->x = grcCursorClip.right - 1;
        }

        if (lppt->y < grcCursorClip.top) {
            lppt->y = grcCursorClip.top;
        } else if (lppt->y >= grcCursorClip.bottom) {
            lppt->y = grcCursorClip.bottom - 1;
        }
    }

    /*
     * If we have more than one monitor, then we need to clip the
     * cursor to a point on the desktop.
     */
    if (!gpDispInfo->fDesktopIsRect) {
        ClipPointToDesktop(lppt);
    }
}

/***************************************************************************\
* SetVDMCursorBounds
*
* This routine is needed so when a vdm is running, the mouse is not bounded
* by the screen. This is so the vdm can correctly virtualize the DOS mouse
* device driver. It can't deal with user always bounding to the screen,
* so it sets wide open bounds.
*
* 20-May-1993 ScottLu       Created.
\***************************************************************************/

VOID SetVDMCursorBounds(
    LPRECT lprc)
{
    if (lprc != NULL) {

        /*
         * Set grcVDMCursorBounds before TEST_PUDF(PUDF_VDMBOUNDSACTIVE), because
         * MoveEvent() calls BoundCursor() from outside the USER CritSect!
         */
        grcVDMCursorBounds = *lprc;
        SET_PUDF(PUDF_VDMBOUNDSACTIVE);

    } else {

        /*
         * Turn vdm bounds off.
         */
        CLEAR_PUDF(PUDF_VDMBOUNDSACTIVE);
    }
}

/***************************************************************************\
* zzzAnimateCursor
*
* When an animated cursor is loaded and the wait cursor is up this routine
* gets called to maintain the cursor animation.
*
* Should only be called by the cursor animation timer.
*
* History:
* 02-Oct-1991 DarrinM      Created.
* 03-Aug-1994 SanfordS     Calibrated.
\***************************************************************************/

#if defined (_M_IX86) && (_MSC_VER <= 1100)
#pragma optimize("s", off)
#endif

VOID zzzAnimateCursor(
    PWND  pwndDummy,
    UINT  message,
    UINT_PTR nID,
    LPARAM lParam)
{
    int   iicur;
    PACON pacon;
    TL    tlpacon;
    int   LostTime;
    int   tTime;

    pacon = (PACON)gpcurLogCurrent;

    if (pacon == NULL || !(pacon->CURSORF_flags & CURSORF_ACON)) {
        gdwLastAniTick = 0;
        return;
    }

    /*
     * Find out actual time loss since last update.
     */
    if (gdwLastAniTick) {

        LostTime = NtGetTickCount() - gdwLastAniTick -
                (pacon->ajifRate[pacon->iicur] * 100 / 6);

        if (LostTime < 0)
            LostTime = 0;

    } else {

        LostTime = 0;
    }

    /*
     * Increment the animation index.
     */
    iicur = pacon->iicur + 1;
    if (iicur >= pacon->cicur)
        iicur = 0;

    pacon->iicur = iicur;

    /*
     * This forces the new cursor to be drawn.
     */
    ThreadLockAlways(pacon, &tlpacon);
    zzzUpdateCursorImage();

    tTime = pacon->ajifRate[iicur] * 100 / 6;

    while (tTime < LostTime) {

        /*
         * Animation is outrunning our ability to render it - skip frames
         * to catch up.
         */
        LostTime -= tTime;

        /*
         * Increment the animation index.
         */
        iicur = pacon->iicur + 1;
        if (iicur >= pacon->cicur)
            iicur = 0;

        pacon->iicur = iicur;

        tTime = pacon->ajifRate[iicur] * 100 / 6;
    }
    ThreadUnlock(&tlpacon);

    gdwLastAniTick = NtGetTickCount() - LostTime;
    gidCursorTimer = InternalSetTimer(NULL, gidCursorTimer, tTime - LostTime, zzzAnimateCursor, TMRF_RIT | TMRF_ONESHOT);

    return;


    DBG_UNREFERENCED_PARAMETER(pwndDummy);
    DBG_UNREFERENCED_PARAMETER(message);
    DBG_UNREFERENCED_PARAMETER(nID);
    DBG_UNREFERENCED_PARAMETER(lParam);
}

/**************************************************************************\
* FCursorShadowed
*
\**************************************************************************/

__inline FCursorShadowed(PCURSINFO pci)
{
    return (TestALPHA(CURSORSHADOW) && (pci->CURSORF_flags & CURSORF_SYSTEM));
}

#if defined (_M_IX86) && (_MSC_VER <= 1100)
#pragma optimize("", on)
#endif

/**************************************************************************\
* zzzUpdateCursorImage
*
* History:
* 14-Jan-1992 DavidPe   Created.
* 09-Aug-1992 DarrinM   Added animated cursor code.
\**************************************************************************/

VOID zzzUpdateCursorImage()
{
    PCURSOR pcurLogNew;
    PCURSOR pcurPhysNew;
    PACON   pacon;
    PCURSOR pcurPhysOld;

    if (gpqCursor == NULL)
        return;

    if ((gpqCursor->iCursorLevel < 0) || (gpqCursor->spcurCurrent == NULL)) {

        pcurLogNew = NULL;

    } else {

        /*
         * Assume we're using the current cursor.
         */
        pcurLogNew = gpqCursor->spcurCurrent;

        /*
         * Check to see if we should use the "app starting" cursor.
         */
        if (gtimeStartCursorHide != 0) {

            if (gpqCursor->spcurCurrent == SYSCUR(ARROW) ||
                gpqCursor->spcurCurrent == SYSCUR(APPSTARTING)) {

                pcurLogNew = SYSCUR(APPSTARTING);
            }
        }
    }

    /*
     * If the logical cursor is changing then start/stop the cursor
     * animation timer as appropriate.
     */
    if (pcurLogNew != gpcurLogCurrent) {

        /*
         * If the old cursor was animating, shut off the animation timer.
         */
        if (gtmridAniCursor != 0) {
            /*
             * Disable animation.
             */
            KILLRITTIMER(NULL, gtmridAniCursor);
            gtmridAniCursor = 0;
        }

        /*
         * If the new cursor is animated, start the animation timer.
         */
        if ((pcurLogNew != NULL) && (pcurLogNew->CURSORF_flags & CURSORF_ACON)) {

            /*
             * Start the animation over from the beginning.
             */
            pacon = (PACON)pcurLogNew;
            pacon->iicur = 0;

            gdwLastAniTick = NtGetTickCount();

            /*
             * Use the rate table to keep the timer on track.
             * 1 Jiffy = 1/60 sec = 100/6 ms
             */
            gtmridAniCursor = InternalSetTimer(NULL, gtmridAniCursor,
                    pacon->ajifRate[0] * 100 / 6, zzzAnimateCursor, TMRF_RIT | TMRF_ONESHOT);
        }
    }

    /*
     * If this is an animated cursor, find and use the current frame
     * of the animation.  NOTE: this is done AFTER the AppStarting
     * business so the AppStarting cursor itself can be animated.
     */
    if (pcurLogNew != NULL && pcurLogNew->CURSORF_flags & CURSORF_ACON) {

        pcurPhysNew = ((PACON)pcurLogNew)->aspcur[((PACON)pcurLogNew)->
                aicur[((PACON)pcurLogNew)->iicur]];
    } else {

        pcurPhysNew = pcurLogNew;
    }

    /*
     * Remember the new logical cursor.
     */
    gpcurLogCurrent = pcurLogNew;

    /*
     * If the physical cursor is changing then update screen.
     */
    if (pcurPhysNew != gpcurPhysCurrent) {

        pcurPhysOld = gpcurPhysCurrent;

        gpcurPhysCurrent = pcurPhysNew;

        if (pcurPhysNew == NULL) {

            SetPointer(FALSE);

        } else {
            ULONG fl = 0;

            if (pcurLogNew->CURSORF_flags & CURSORF_ACON) {
                fl |= SPS_ANIMATEUPDATE;
            }
            if (FCursorShadowed(GETPCI(pcurLogNew))) {
                fl |= SPS_ALPHA;
            }
            GreSetPointer(gpDispInfo->hDev, GETPCI(pcurPhysNew), fl);
        }

        /*
         * Notify anyone who cares about the change
         * This can happen on the RIT, so we need to pass on the real
         * thread/process ID.  Hence we use hwndCursor.
         * This comment is from WIn'95 so it may not be true - IanJa.
         */
        if (FWINABLE()) {
            DWORD   event;
            /*
             * These are the events we send:
             *      hcurPhys now NULL       ->  EVENT_OBJECT_HIDE
             *      hcurPhys was NULL       ->  EVENT_OBJECT_SHOW
             *      hcurPhys changing       ->  EVENT_OBJECT_NAMECHANGE
             * Since we only go through this code if hcurPhys is actually
             * changing, these checks are simple.
             */
            if (!pcurPhysNew) {
                event = EVENT_OBJECT_HIDE;
            } else if (!pcurPhysOld) {
                event = EVENT_OBJECT_SHOW;
            } else {
                event = EVENT_OBJECT_NAMECHANGE;
            }
            zzzWindowEvent(event, NULL, OBJID_CURSOR, INDEXID_CONTAINER, WEF_USEPWNDTHREAD);
        }
    }
}

#if DBG

/***************************************************************************\
* DbgLockQCursor
*
* Special routine to lock cursors into a queue.  Besides a pointer
* to the cursor, the handle is also saved.
* Returns the pointer to the previous current cursor for that queue.
*
* History:
* 26-Jan-1993 JimA      Created.
\***************************************************************************/

PCURSOR DbgLockQCursor(
    PQ      pq,
    PCURSOR pcur)
{
    /*
     * See if the queue is marked for destuction.  If so, we should not
     * be trying to lock a cursor.
     */
    UserAssertMsg0(!(pq->QF_flags & QF_INDESTROY),
                  "LockQCursor: Attempting to lock cursor to freed queue");

    return Lock(&pq->spcurCurrent, pcur);
}

#endif // DBG

/***************************************************************************\
* SetPointer
*
* 29-Mar-1998   vadimg      created
\***************************************************************************/

void SetPointer(BOOL fSet)
{
    if (fSet) {
        if (gpqCursor != NULL && gpqCursor->iCursorLevel >= 0 &&
                gpqCursor->spcurCurrent != NULL &&
                SYSMET(MOUSEPRESENT)) {

            PCURSINFO pci = GETPCI(gpqCursor->spcurCurrent);
            ULONG fl = FCursorShadowed(pci) ? SPS_ALPHA : 0;

            GreSetPointer(gpDispInfo->hDev, pci, fl);
        }
    } else {
        GreSetPointer(gpDispInfo->hDev, NULL, 0);
    }
}

/***************************************************************************\
* HideCursorNoCapture
*
* Set the cursor to NULL if the mouse is not captured
*
* 20-May-1998   MCostea      created
\***************************************************************************/

void zzzHideCursorNoCapture()
{
    PTHREADINFO ptiCurrent = PtiCurrentShared();

    if (!ptiCurrent->pq->spwndCapture && (GetAppCompatFlags2(VER40) & GACF2_EDITNOMOUSEHIDE) == 0) {
        zzzSetCursor(NULL);
    }
}
