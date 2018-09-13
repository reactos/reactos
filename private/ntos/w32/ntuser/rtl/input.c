/****************************** Module Header ******************************\
* Module Name: input.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains common input functions.
*
* History:
* 09-12-95 JerrySh      Created.
\***************************************************************************/


/***************************************************************************\
* CheckMsgRange
*
* Checks to see if a message range is within a message filter
*
* History:
* 11-13-90 DavidPe      Created.
* 11-Oct-1993 mikeke    Macroized
\***************************************************************************/

#define CheckMsgRange(wMsgRangeMin, wMsgRangeMax, wMsgFilterMin, wMsgFilterMax) \
    (  ((wMsgFilterMin) > (wMsgFilterMax))      \
     ? (  ((wMsgRangeMax) >  (wMsgFilterMax))   \
        &&((wMsgRangeMin) <  (wMsgFilterMin)))  \
     : (  ((wMsgRangeMax) >= (wMsgFilterMin))   \
        &&((wMsgRangeMin) <= (wMsgFilterMax)))  \
    )

/***************************************************************************\
* CalcWakeMask
*
* Calculates which wakebits to check for based on the message
* range specified by wMsgFilterMin/Max.  This basically means
* if the filter range didn't input WM_KEYUP and WM_KEYDOWN,
* QS_KEY wouldn't be included.
*
* History:
* 10-28-90 DavidPe      Created.
\***************************************************************************/

UINT CalcWakeMask(
    UINT wMsgFilterMin,
    UINT wMsgFilterMax,
    UINT fsWakeMaskFilter)
{
    UINT fsWakeMask;

    /*
     * New for NT5: wake mask filter.
     * In addition to the message filter, the application can also provide
     *      a wake mask directly.
     * If none is provided, default to NT4 mask (plus QS_SENDMESSAGE).
     */
    if (fsWakeMaskFilter == 0) {
        fsWakeMask = (QS_ALLINPUT | QS_EVENT | QS_ALLPOSTMESSAGE);
    } else {
        /*
         * If the caller wants input, we force all input events. The
         *  same goes for posted messages. We do this to keep NT4
         *  compatibility as much as possible.
         */
        if (fsWakeMaskFilter & QS_INPUT) {
            fsWakeMaskFilter |= (QS_INPUT | QS_EVENT);
        }
        if (fsWakeMaskFilter & (QS_POSTMESSAGE | QS_TIMER | QS_HOTKEY)) {
            fsWakeMaskFilter |= (QS_POSTMESSAGE | QS_TIMER | QS_HOTKEY);
        }
        fsWakeMask = fsWakeMaskFilter;
    }

#ifndef _USERK_
    /*
     * The client PeekMessage in client\cltxt.h didn't used to
     *  call CalcWakeMask if wMsgFilterMax was 0. We call it now
     *  to take care of fsWakeMaskFilter but still bail before
     *  messing with the message filter.
     */
    if (wMsgFilterMax == 0) {
        return fsWakeMask;
    }
#endif

    /*
     * Message filter.
     * If the filter doesn't match certain ranges, we take out bits one by one.
     * First check for a 0, 0 filter which means we want all input.
     */
    if (wMsgFilterMin == 0 && wMsgFilterMax == ((UINT)-1)) {
        return fsWakeMask;
    }

    /*
     * We're not looking at all posted messages.
     */
    fsWakeMask &= ~QS_ALLPOSTMESSAGE;

    /*
     * Check for mouse move messages.
     */
    if ((CheckMsgFilter(WM_NCMOUSEMOVE, wMsgFilterMin, wMsgFilterMax) == FALSE) &&
            (CheckMsgFilter(WM_MOUSEMOVE, wMsgFilterMin, wMsgFilterMax) == FALSE)) {
        fsWakeMask &= ~QS_MOUSEMOVE;
    }

    /*
     * First check to see if mouse buttons messages are in the filter range.
     */
    if ((CheckMsgRange(WM_NCLBUTTONDOWN, WM_NCMBUTTONDBLCLK, wMsgFilterMin,
            wMsgFilterMax) == FALSE) && (CheckMsgRange(WM_MOUSEFIRST + 1,
            WM_MOUSELAST, wMsgFilterMin, wMsgFilterMax) == FALSE)) {
        fsWakeMask &= ~QS_MOUSEBUTTON;
    }

    /*
     * Check for key messages.
     */
    if (CheckMsgRange(WM_KEYFIRST, WM_KEYLAST,
            wMsgFilterMin, wMsgFilterMax) == FALSE) {
        fsWakeMask &= ~QS_KEY;
    }

    /*
     * Check for paint messages.
     */
    if (CheckMsgFilter(WM_PAINT, wMsgFilterMin, wMsgFilterMax) == FALSE) {
        fsWakeMask &= ~QS_PAINT;
    }

    /*
     * Check for timer messages.
     */
    if ((CheckMsgFilter(WM_TIMER, wMsgFilterMin, wMsgFilterMax) == FALSE) &&
            (CheckMsgFilter(WM_SYSTIMER,
            wMsgFilterMin, wMsgFilterMax) == FALSE)) {
        fsWakeMask &= ~QS_TIMER;
    }

    /*
     * Check also for WM_QUEUESYNC which maps to all input bits.
     * This was added for CBT/EXCEL processing.  Without it, a
     * xxxPeekMessage(....  WM_QUEUESYNC, WM_QUEUESYNC, FALSE) would
     * not see the message. (bobgu 4/7/87)
     * Since the user can provide a wake mask now (fsWakeMaskFilter),
     *  we also add QS_EVENT in this case (it was always set on NT4).
     */
    if (wMsgFilterMin == WM_QUEUESYNC) {
        fsWakeMask |= (QS_INPUT | QS_EVENT);
    }

    return fsWakeMask;
}

