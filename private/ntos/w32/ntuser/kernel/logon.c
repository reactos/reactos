/**************************** Module Header ********************************\
* Module Name: logon.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Logon Support Routines
*
* History:
* 01-14-91 JimA         Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************\
* _RegisterLogonProcess
*
* Register the logon process and set secure mode flag
*
* History:
* 07-01-91 JimA         Created.
\***************************************************************************/

BOOL _RegisterLogonProcess(
    DWORD dwProcessId,
    BOOL fSecure)
{
    UNREFERENCED_PARAMETER(fSecure);

    /*
     * Allow only one logon process and then only if it has TCB
     * privilege.
     */
    if (gpidLogon != 0 || !IsPrivileged(&psTcb)) {
        RIPERR0(ERROR_ACCESS_DENIED,
                RIP_WARNING,
                "Access denied in _RegisterLogonProcess");

        return FALSE;
    }

    gpidLogon = (HANDLE)LongToHandle( dwProcessId );
    return TRUE;
}


/***************************************************************************\
* _LockWindowStation
*
* Locks a windowstation and its desktops and returns the busy status.
*
* History:
* 01-15-91 JimA         Created.
\***************************************************************************/

UINT _LockWindowStation(
    PWINDOWSTATION pwinsta)
{
    PDESKTOP pdesk;
    BOOL fBusy = FALSE;

    /*
     * Make sure the caller is the logon process
     */
    if (GetCurrentProcessId() != gpidLogon) {
        RIPERR0(ERROR_ACCESS_DENIED,
                RIP_WARNING,
                "Access denied in _LockWindowStation");

        return WSS_ERROR;
    }

    /*
     * Prevent desktop switches
     */
    pwinsta->dwWSF_Flags |= WSF_SWITCHLOCK;

    /*
     * Determine whether the station is busy
     */
    pdesk = pwinsta->rpdeskList;
    while (pdesk != NULL) {
        if (pdesk != grpdeskLogon &&
                OBJECT_TO_OBJECT_HEADER(pdesk)->HandleCount != 0) {

            /*
             * This desktop is open, thus the station is busy
             */
            fBusy = TRUE;
            break;
        }
        pdesk = pdesk->rpdeskNext;
    }

    if (pwinsta->dwWSF_Flags & WSF_SHUTDOWN)
        pwinsta->dwWSF_Flags |= WSF_OPENLOCK;

    /*
     * Unlock opens if the station is busy and is not in the middle
     * of shutting down.
     */
    if (fBusy)
        return WSS_BUSY;
    else
        return WSS_IDLE;
}


/***************************************************************************\
* _UnlockWindowStation
*
* Unlocks a windowstation locked by LogonLockWindowStation.
*
* History:
* 01-15-91 JimA         Created.
\***************************************************************************/

BOOL _UnlockWindowStation(
    PWINDOWSTATION pwinsta)
{

    /*
     * Make sure the caller is the logon process
     */
    if (GetCurrentProcessId() != gpidLogon) {
        RIPERR0(ERROR_ACCESS_DENIED,
                RIP_WARNING,
                "Access denied in _UnlockWindowStation");

        return FALSE;
    }

    /*
     * If shutdown is occuring, only remove the switch lock.
     */
    if (pwinsta->dwWSF_Flags & WSF_SHUTDOWN)
        pwinsta->dwWSF_Flags &= ~WSF_SWITCHLOCK;
    else
        pwinsta->dwWSF_Flags &= ~(WSF_OPENLOCK | WSF_SWITCHLOCK);
    return TRUE;
}


/***************************************************************************\
* _SetLogonNotifyWindow
*
* Register the window to notify when logon related events occur.
*
* History:
* 01-13-92 JimA         Created.
\***************************************************************************/
BOOL _SetLogonNotifyWindow(
    PWND pwnd)
{
    /*
     * Make sure the caller is the logon process
     */
    if (GetCurrentProcessId() != gpidLogon) {
        RIPERR0(ERROR_ACCESS_DENIED,
                RIP_WARNING,
                "Access denied in _SetLogonNotifyWindow");

        return FALSE;
    }

    Lock(&gspwndLogonNotify, pwnd);

    return TRUE;
}
