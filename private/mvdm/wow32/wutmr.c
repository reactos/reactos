/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WUTMR.C
 *  WOW32 16-bit User Timer API support
 *
 *  History:
 *  Created 07-Mar-1991 by Jeff Parsons (jeffpar)
 *          24-Feb-1993 reworked to use array of timer functions - barryb
--*/


#include "precomp.h"
#pragma hdrstop

MODNAME(wutmr.c);

LIST_ENTRY TimerList;

// Element Zero is unused.

STATIC PTMR aptmrWOWTimers[] = {
                                 NULL, NULL, NULL, NULL,
                                 NULL, NULL, NULL, NULL,
                                 NULL, NULL, NULL, NULL,
                                 NULL, NULL, NULL, NULL,
                                 NULL, NULL, NULL, NULL,
                                 NULL, NULL, NULL, NULL,
                                 NULL, NULL, NULL, NULL,
                                 NULL, NULL, NULL, NULL,
                                 NULL, NULL, NULL
                               };


STATIC TIMERPROC afnTimerFuncs[] = {
                        NULL,       W32Timer1,  W32Timer2,  W32Timer3,
                        W32Timer4,  W32Timer5,  W32Timer6,  W32Timer7,
                        W32Timer8,  W32Timer9,  W32Timer10, W32Timer11,
                        W32Timer12, W32Timer13, W32Timer14, W32Timer15,
                        W32Timer16, W32Timer17, W32Timer18, W32Timer19,
                        W32Timer20, W32Timer21, W32Timer22, W32Timer23,
                        W32Timer24, W32Timer25, W32Timer26, W32Timer27,
                        W32Timer28, W32Timer29, W32Timer30, W32Timer31,
                        W32Timer32, W32Timer33, W32Timer34
                        };


/* Timer mapping functions
 *
 * The basic 16-bit timer mapping operations are Add, Find and Free.  When
 * a 16-bit app calls SetTimer, we call Win32's SetTimer with W32TimerProc
 * in place of the 16-bit proc address.  Assuming the timer is successfully
 * allocated, we add the timer to our own table, recording the 16-bit proc
 * address.
 */


//
// Search for a timer by its 16-bit information.  Looks in the list of
// active timers.  If the timer is found by this routine, then SetTimer()
// has been called and KillTimer() has not yet been called.
//
PTMR IsDuplicateTimer16(HWND16 hwnd16, HTASK16 htask16, WORD wIDEvent)
{
    register PTMR ptmr;
    register INT iTimer;

    //
    // Excel calls SetTimer with hwnd==NULL but dispatches the
    // WM_TIMER messages with hwnd!=NULL.  so call it a match if
    // hwnd16!=NULL and ptmr->hwnd16==NULL
    //

    for (iTimer=1; iTimer<NUMEL(aptmrWOWTimers); iTimer++) {

        ptmr = aptmrWOWTimers[iTimer];

        if (ptmr) {
            if (LOWORD(ptmr->dwEventID) == wIDEvent &&
                ptmr->htask16 == htask16 &&
                (ptmr->hwnd16 == hwnd16 || !ptmr->hwnd16)) {

                return ptmr;
            }
        }
    }

    return NULL;
}



//
// This is called to free *ALL* timers created with a given hwnd16
// ie. All timers created by SetTimer(hwnd != NULL, id, duration)
// This should only be called when the hwnd is being destroyed: DestroyWindow()
//
VOID FreeWindowTimers16(HWND hwnd32)
{
    register PTMR ptmr;
    register INT iTimer;
    HAND16 htask16;

    htask16 = CURRENTPTD()->htask16;

    for (iTimer=1; iTimer<NUMEL(aptmrWOWTimers); iTimer++) {

        ptmr = aptmrWOWTimers[iTimer];

        if (ptmr) {
            if (ptmr->htask16 == htask16 && GETHWND16(hwnd32) == ptmr->hwnd16) {

                // we can't wait for Win32 to kill the timer for us during its
                // normal DestroyWindow() handling because it might send another
                // WM_TIMER message which we are now not ready to handle.
                KillTimer(ptmr->hwnd32, ptmr->dwEventID);

                // now free our WOW structures supporting this timer
                FreeTimer16(ptmr);
            }
        }
    }
}





//
// Search for a timer by its 32-bit information.  Looks in the list of
// all timers (including those that have already been killed by KillTimer().
//
//
PTMR FindTimer32(HWND16 hwnd16, DWORD dwIDEvent)
{
    register PTMR ptmr;
    HAND16 htask16;

    htask16 = CURRENTPTD()->htask16;

    //
    // Excel calls SetTimer with hwnd==NULL but dispatches the
    // WM_TIMER messages with hwnd!=NULL.  so call it a match if
    // hwnd16!=NULL and ptmr->hwnd16==NULL
    //

    for (ptmr = (PTMR)TimerList.Flink; ptmr != (PTMR)&TimerList; ptmr = (PTMR)ptmr->TmrList.Flink) {

        if (ptmr->dwEventID == dwIDEvent &&
            ptmr->htask16 == htask16 &&
            (ptmr->hwnd16 == hwnd16 || (hwnd16 && !ptmr->hwnd16))) {

            return ptmr;
        }
    }

    return (PTMR)NULL;
}


//
// Search for a timer by its 16-bit information.  Looks in the list of
// all timers (including those that have already been killed by KillTimer().
//
//
PTMR FindTimer16(HWND16 hwnd16, HTASK16 htask16, WORD wIDEvent)
{
    register PTMR ptmr;

    //
    // Excel calls SetTimer with hwnd==NULL but dispatches the
    // WM_TIMER messages with hwnd!=NULL.  so call it a match if
    // hwnd16!=NULL and ptmr->hwnd16==NULL
    //

    for (ptmr = (PTMR)TimerList.Flink; ptmr != (PTMR)&TimerList; ptmr = (PTMR)ptmr->TmrList.Flink) {

        if (LOWORD(ptmr->dwEventID) == wIDEvent &&
            ptmr->htask16 == htask16 &&
            (ptmr->hwnd16 == hwnd16 || (hwnd16 && !ptmr->hwnd16))) {

            return ptmr;
        }
    }

    return (PTMR)NULL;
}


//
// Search for a killed timer by its 16-bit information.
//
//
PTMR FindKilledTimer16(HWND16 hwnd16, HTASK16 htask16, WORD wIDEvent)
{
    register PTMR ptmr;

    for (ptmr = (PTMR)TimerList.Flink; ptmr != (PTMR)&TimerList; ptmr = (PTMR)ptmr->TmrList.Flink) {

        if (ptmr->wIndex == 0 &&
            ptmr->htask16 == htask16 &&
            ptmr->hwnd16 == hwnd16 &&
            (LOWORD(ptmr->dwEventID) == wIDEvent || !hwnd16)) {
            // 1. the timer has been killed and
            // 2. the timer is in this task and
            // 3. the hwnds match (both might be 0) and
            // 4. the IDs match, or the hwnds are both 0 (in that case,
            //    IDs are ignored)

            return ptmr;
        }
    }

    return (PTMR)NULL;
}


VOID FreeTimer16(PTMR ptmr)
{
    WOW32ASSERT(ptmr->wIndex == 0 || ptmr == aptmrWOWTimers[ptmr->wIndex]);
    aptmrWOWTimers[ptmr->wIndex] = NULL;
    RemoveEntryList(&ptmr->TmrList);
    free_w(ptmr);
}


VOID DestroyTimers16(HTASK16 htask16)
{
    PTMR ptmr, next;

    for (ptmr = (PTMR)TimerList.Flink; ptmr != (PTMR)&TimerList; ptmr = next) {

        next = (PTMR)ptmr->TmrList.Flink;
        if (ptmr->htask16 == htask16) {

            //
            // don't call KillTimer() if the timer was associated with
            // a window and the window is gone, USER has already
            // cleaned it up.
            //

            if (ptmr == aptmrWOWTimers[ptmr->wIndex] && (!ptmr->hwnd32 || IsWindow(ptmr->hwnd32))) {
                if ( KillTimer(ptmr->hwnd32, ptmr->dwEventID) ) {
                    LOGDEBUG(LOG_IMPORTANT,
                       ("DestroyTimers16:Killed %04x\n",ptmr->dwEventID));
                } else {
                    LOGDEBUG(LOG_ERROR,
                       ("DestroyTimers16:FAILED %04x\n",ptmr->dwEventID));
                }
            }
            FreeTimer16(ptmr);
        }

    }
}


VOID W32TimerFunc(UINT index, HWND hwnd, UINT idEvent, DWORD dwTime)
{
    PARM16 Parm16;
    register PTMR ptmr;

    ptmr = aptmrWOWTimers[index];

    if ( !ptmr ) {
        LOGDEBUG(LOG_ALWAYS,("    W32TimerFunc ERROR: cannot find timer %08x\n", idEvent));
        return;
    }

    if (ptmr->dwEventID != idEvent) {
        //
        // This is an extra timer message which was already in the message
        // queue when the app called KillTimer().  The PTMR isn't in the
        // array, but it is still linked into the TimerList.
        //
        LOGDEBUG(LOG_WARNING,("    W32TimerFunc WARNING: Timer %08x called after KillTimer()\n", idEvent));
        for (ptmr = (PTMR)TimerList.Flink; ptmr != (PTMR)&TimerList; ptmr = (PTMR)ptmr->TmrList.Flink) {
            if (ptmr->dwEventID == idEvent) {
                break;
            }
        }

        if ( ptmr == (PTMR)&TimerList ) {
            LOGDEBUG(LOG_ALWAYS,("    W32TimerFunc ERROR: cannot find timer %08x (second case)\n", idEvent));
            return;
        }
    }

    Parm16.WndProc.hwnd   = ptmr->hwnd16;
    Parm16.WndProc.wMsg   = WM_TIMER;
    Parm16.WndProc.wParam = LOWORD(ptmr->dwEventID);
    Parm16.WndProc.lParam = dwTime;
    Parm16.WndProc.hInst  = 0;     // callback16 defaults to ss

    CallBack16(RET_WNDPROC, &Parm16, ptmr->vpfnTimerProc, NULL);
}


/*++
    BOOL KillTimer(<hwnd>, <nIDEvent>)
    HWND <hwnd>;
    INT <nIDEvent>;

    The %KillTimer% function kills the timer event identified by the <hwnd> and
    <nIDEvent> parameters. Any pending WM_TIMER messages associated with the
    timer are removed from the message queue.

    <hwnd>
        Identifies the window associated with the given timer event. This must
        be the same value passed as the hwnd parameter to the SetTimer function
        call that created the timer event.

    <nIDEvent>
        Specifies the timer event to be killed. If the application called
        %SetTimer% with the <hwnd> parameter set to NULL, this must be the event
        identifier returned by %SetTimer%. If the <hwnd> parameter of %SetTimer%
        was a valid window handle, <nIDEvent> must be the value of the
        <nIDEvent> parameter passed to %SetTimer%.

    The return value specifies the outcome of the function. It is TRUE if the
    event was killed. It is FALSE if the %KillTimer% function could not find the
    specified timer event.
--*/

ULONG FASTCALL WU32KillTimer(PVDMFRAME pFrame)
{
    ULONG ul;
    register PTMR ptmr;
    register PKILLTIMER16 parg16;
    HWND16 hwnd16;
    WORD   wIDEvent;
    HAND16 htask16;

    GETARGPTR(pFrame, sizeof(KILLTIMER16), parg16);

    htask16  = CURRENTPTD()->htask16;
    hwnd16   = (HWND16)parg16->f1;
    wIDEvent = parg16->f2;

    ptmr = IsDuplicateTimer16(hwnd16, htask16, wIDEvent);

    if (ptmr) {
        ul = GETBOOL16(KillTimer(ptmr->hwnd32, ptmr->dwEventID));
        aptmrWOWTimers[ptmr->wIndex] = NULL;
        ptmr->wIndex = 0;
    }
    else {
        ul = 0;
        LOGDEBUG(LOG_IMPORTANT,("    WU32KillTimer ERROR: cannot find timer %04x\n", wIDEvent));
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    WORD SetTimer(<hwnd>, <nIDEvent>, <wElapse>, <lpTimerFunc>)
    HWND <hwnd>;
    int <nIDEvent>;
    WORD <wElapse>;
    FARPROC <lpTimerFunc>;

    The %SetTimer% function creates a system timer event. When a timer event
    occurs, Windows passes a WM_TIMER message to the application-supplied
    function specified by the <lpTimerFunc> parameter. The function can then
    process the event. A NULL value for <lpTimerFunc> causes WM_TIMER messages
    to be placed in the application queue.

    <hwnd>
        Identifies the window to be associated with the timer. If hwnd is NULL,
        no window is associated with the timer.

    <nIDEvent>
        Specifies a nonzero timer-event identifier if the <hwnd> parameter
        is not NULL.

    <wElapse>
        Specifies the elapsed time (in milliseconds) between timer
        events.

    <lpTimerFunc>
        Is the procedure-instance address of the function to be
        notified when the timer event takes place. If <lpTimerFunc> is NULL, the
        WM_TIMER message is placed in the application queue, and the %hwnd%
        member of the %MSG% structure contains the <hwnd> parameter given in the
        %SetTimer% function call. See the following Comments section for
        details.

    The return value specifies the integer identifier for the new timer event.
    If the <hwnd> parameter is NULL, an application passes this value to the
    %KillTimer% function to kill the timer event. The return value is zero if
    the timer was not created.

    Timers are a limited global resource; therefore, it is important that an
    application check the value returned by the %SetTimer% function to verify
    that a timer is actually available.

    To install a timer function, %SetTimer% must receive a procedure-instance
    address of the function, and the function must be exported in the
    application's module-definition file. A procedure-instance address can be
    created using the %MakeProcInstance% function.

    The callback function must use the Pascal calling convention and must be
    declared %FAR%.

    Callback Function:

    WORD FAR PASCAL <TimerFunc>(<hwnd>, <wMsg>, <nIDEvent>, <dwTime>)
    HWND <hwnd>;
    WORD <wMsg>;
    int <nIDEvent>;
    DWORD <dwTime>;

    <TimerFunc> is a placeholder for the application-supplied function name. The
    actual name must be exported by including it in an %EXPORTS% statement in
    the application's module-definition file.

    <hwnd>
        Identifies the window associated with the timer event.

    <wMsg>
        Specifies the WM_TIMER message.

    <nIDEvent>
        Specifies the timer's ID.

    <dwTime>
        Specifies the current system time.
--*/

ULONG FASTCALL WU32SetTimer(PVDMFRAME pFrame)
{
    ULONG ul;
    register PTMR ptmr;
    register PSETTIMER16 parg16;
    HWND16  hwnd16;
    WORD    wIDEvent;
    WORD    wElapse;
    DWORD   vpfnTimerProc;
    DWORD   dwTimerProc32;
    HAND16  htask16;
    INT     iTimer;

    GETARGPTR(pFrame, sizeof(SETTIMER16), parg16);

    ul = 0;

    htask16       = CURRENTPTD()->htask16;
    hwnd16        = (HWND16)parg16->f1;
    wIDEvent      = parg16->f2;
    wElapse       = parg16->f3;

    // Don't allow WOW apps to set a timer with a period of less than
    // 55 ms. Myst and Winstone depend on this.
    if (wElapse < 55) wElapse = 55;

    vpfnTimerProc = VPFN32(parg16->f4);

    ptmr = IsDuplicateTimer16(hwnd16, htask16, wIDEvent);

    if (!ptmr) {

        // Loop through the slots in the timer array

        iTimer = 2;
        while (iTimer < NUMEL(aptmrWOWTimers)) {
            /*
            ** Find a slot in the arrays for which
            ** no pointer has yet been allocated.
            */
            if ( !aptmrWOWTimers[iTimer] ) {

                //
                // See if there is already thunking information for this
                // timer.  If there is, delete it from the list of timer
                // info and re-use its memory because this new timer
                // superceeds the old thunking information.
                //
                ptmr = FindKilledTimer16(hwnd16, htask16, wIDEvent);
                if (ptmr) {

                    RemoveEntryList(&ptmr->TmrList);

                } else {

                    // Allocate a TMR structure for the new timer
                    ptmr = malloc_w(sizeof(TMR));

                }

                aptmrWOWTimers[iTimer] = ptmr;

                if (!ptmr) {
                    LOGDEBUG(LOG_ALWAYS,("    WOW32 ERROR: TMR allocation failure\n"));
                    return 0;
                }

                break;          // Fall out into initialization code
            }
            iTimer++;
        }
        if (iTimer >= NUMEL(aptmrWOWTimers)) {
            LOGDEBUG(LOG_ALWAYS,("    WOW32 ERROR: out of timer slots\n"));
            return 0;
        }

        // Initialize the constant parts of the TMR structure (done on 1st SetTimer)
        InsertHeadList(&TimerList, &ptmr->TmrList);
        ptmr->hwnd16    = hwnd16;
        ptmr->hwnd32    = HWND32(hwnd16);
        ptmr->htask16   = htask16;
        ptmr->wIndex    = (WORD)iTimer;
    }


    // Setup the changeable parts of the TMR structure (done for every SetTimer)

    if (vpfnTimerProc) {
        dwTimerProc32 = (DWORD)afnTimerFuncs[ptmr->wIndex];
    } else {
        dwTimerProc32 = (DWORD)NULL;
    }

    ptmr->vpfnTimerProc = vpfnTimerProc;
    ptmr->dwTimerProc32 = dwTimerProc32;

    ul = SetTimer(
                ptmr->hwnd32,
                (UINT)wIDEvent,
                (UINT)wElapse,
                (TIMERPROC)dwTimerProc32 );

    //
    // USER-generated timerID's are between 0x100 and 0x7fff
    //

    WOW32ASSERT(HIWORD(ul) == 0);

    if (ul) {

        ptmr->dwEventID = ul;

        //
        // when hwnd!=NULL and nEventID==0 the API returns 1 to
        // indicate success but the timer's ID is 0 as requested.
        //

        if (!wIDEvent && ptmr->hwnd32)
            ptmr->dwEventID = 0;

    } else {

        // Since the real SetTimer failed, free
        // our local data using simply our own timer ID

        FreeTimer16(ptmr);
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


VOID CALLBACK W32Timer1(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(1, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer2(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(2, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer3(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(3, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer4(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(4, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer5(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(5, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer6(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(6, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer7(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(7, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer8(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(8, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer9(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(9, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer10(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(10, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer11(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(11, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer12(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(12, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer13(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(13, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer14(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(14, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer15(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(15, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer16(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(16, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer17(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(17, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer18(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(18, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer19(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(19, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer20(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(20, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer21(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(21, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer22(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(22, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer23(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(23, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer24(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(24, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer25(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(25, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer26(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(26, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer27(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(27, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer28(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(28, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer29(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(29, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer30(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(30, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer31(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(31, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer32(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(32, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer33(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(33, hwnd, idEvent, dwTime);
}

VOID CALLBACK W32Timer34(HWND hwnd, UINT msg, UINT idEvent, DWORD dwTime)
{
    WOW32ASSERT(msg == WM_TIMER);
    W32TimerFunc(34, hwnd, idEvent, dwTime);
}
