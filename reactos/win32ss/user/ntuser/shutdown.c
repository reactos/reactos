/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Shutdown routines
 * FILE:             subsystems/win32/win32k/ntuser/shutdown.c
 * PROGRAMER:        Hermes Belusca
 */

#include <win32k.h>
// DBG_DEFAULT_CHANNEL(UserShutdown);

// Client Shutdown messages
#define MCS_SHUTDOWNTIMERS  1
#define MCS_QUERYENDSESSION 2
// Client Shutdown returns
#define MCSR_GOODFORSHUTDOWN  1
#define MCSR_SHUTDOWNFINISHED 2
#define MCSR_DONOTSHUTDOWN    3

/*
 * Based on CSRSS and described in pages 1115 - 1118 "Windows Internals, Fifth Edition".
 * CSRSS sends WM_CLIENTSHUTDOWN messages to top-level windows, and it is our job
 * to send WM_QUERYENDSESSION / WM_ENDSESSION messages in response.
 */
LRESULT
IntClientShutdown(IN PWND pWindow,
                  IN WPARAM wParam,
                  IN LPARAM lParam)
{
    LPARAM lParams;
    BOOL KillTimers;
    INT i;
    LRESULT lResult = MCSR_GOODFORSHUTDOWN;
    HWND *List;

    lParams = wParam & (ENDSESSION_LOGOFF|ENDSESSION_CRITICAL|ENDSESSION_CLOSEAPP);
    KillTimers = wParam & MCS_SHUTDOWNTIMERS ? TRUE : FALSE;

    /* First, send end sessions to children */
    List = IntWinListChildren(pWindow);

    if (List)
    {
        for (i = 0; List[i]; i++)
        {
            PWND WndChild;

            if (!(WndChild = UserGetWindowObject(List[i])))
                continue;

            if (wParam & MCS_QUERYENDSESSION)
            {
                if (!co_IntSendMessage(WndChild->head.h, WM_QUERYENDSESSION, 0, lParams))
                {
                    lResult = MCSR_DONOTSHUTDOWN;
                    break;
                }
            }
            else
            {
                co_IntSendMessage(WndChild->head.h, WM_ENDSESSION, KillTimers, lParams);
                if (KillTimers)
                {
                    DestroyTimersForWindow(WndChild->head.pti, WndChild);
                }
                lResult = MCSR_SHUTDOWNFINISHED;
            }
        }
        ExFreePoolWithTag(List, USERTAG_WINDOWLIST);
    }

    if (List && (lResult == MCSR_DONOTSHUTDOWN))
        return lResult;

    /* Send to the caller */
    if (wParam & MCS_QUERYENDSESSION)
    {
        if (!co_IntSendMessage(pWindow->head.h, WM_QUERYENDSESSION, 0, lParams))
        {
            lResult = MCSR_DONOTSHUTDOWN;
        }
    }
    else
    {
        co_IntSendMessage(pWindow->head.h, WM_ENDSESSION, KillTimers, lParams);
        if (KillTimers)
        {
            DestroyTimersForWindow(pWindow->head.pti, pWindow);
        }
        lResult = MCSR_SHUTDOWNFINISHED;
    }

    return lResult;
}

/* EOF */
