/*
 * PROJECT:     ReactOS Service Host
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        base/services/svchost/netbios.c
 * PURPOSE:     NetBIOS Service Support
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES ******************************************************************/

#include "svchost.h"

#include <lmerr.h>
#include <nb30.h>

/* GLOBALS *******************************************************************/

LONG GlobalNetBiosUseCount;
DWORD LanaFlags[8];
CRITICAL_SECTION SvcNetBiosCritSec;

/* FUNCTIONS *****************************************************************/

DWORD
WINAPI
SvcNetBiosStatusToApiStatus (
    _In_ DWORD NetBiosError
    )
{
    /* Convert from one status to another */
    switch (NetBiosError)
    {
        case NRC_GOODRET:
            return NERR_Success;
        case NRC_INUSE:
        case NRC_NAMCONF:
            return NERR_DuplicateName;
        case NRC_NOWILD:
        case NRC_NAMERR:
            return ERROR_INVALID_PARAMETER;
        case NRC_NOCALL:
            return NERR_NameNotFound;
        case NRC_NORES:
            return NERR_NoNetworkResource;
        case NRC_DUPNAME:
            return NERR_AlreadyExists;
        case NRC_NAMTFUL:
            return NERR_TooManyNames;
        case NRC_ACTSES:
            return NERR_DeleteLater;
        case NRC_REMTFUL:
            return ERROR_REM_NOT_LIST;
        default:
            return NERR_NetworkError;
    }
}

BOOL
WINAPI
LanaFlagIsSet (
    _In_ UCHAR Lana
    )
{
    DWORD i = Lana / 32;

    /* Clear the flag for this LANA */
    return (i <= 7) ? LanaFlags[i] & (1 << (Lana - 32 * i)) : FALSE;
}

VOID
WINAPI
SetLanaFlag (
    _In_ UCHAR Lana
    )
{
    DWORD i = Lana / 32;

    /* Set the flag for this LANA */
    if (i <= 7) LanaFlags[i] |= 1 << (Lana - 32 * i);
}

VOID
WINAPI
SvcNetBiosInit(
    VOID
    )
{
    /* Initialize NetBIOS-related structures and variables */
    InitializeCriticalSection(&SvcNetBiosCritSec);
    GlobalNetBiosUseCount = 0;
    ZeroMemory(LanaFlags, sizeof(LanaFlags));
}

VOID
WINAPI
SvcNetBiosClose (
    VOID
    )
{
    /* While holding the lock, drop a reference*/
    EnterCriticalSection(&SvcNetBiosCritSec);
    if ((GlobalNetBiosUseCount != 0) && (--GlobalNetBiosUseCount == 0))
    {
        /* All references are gone, clear all LANA's */
        ZeroMemory(LanaFlags, sizeof(LanaFlags));
    }
    LeaveCriticalSection(&SvcNetBiosCritSec);
}

VOID
WINAPI
SvcNetBiosOpen (
    VOID
    )
{
    /* Increment the reference counter while holding the lock */
    EnterCriticalSection(&SvcNetBiosCritSec);
    GlobalNetBiosUseCount++;
    LeaveCriticalSection(&SvcNetBiosCritSec);
}

DWORD
WINAPI
SvcNetBiosReset (
    _In_ UCHAR LanaNum
    )
{
    DWORD dwError = ERROR_SUCCESS;
    UCHAR nbtError;
    NCB ncb;

    /* Block all other NetBIOS operations */
    EnterCriticalSection(&SvcNetBiosCritSec);

    /* Is this LANA enabled? */
    if (!LanaFlagIsSet(LanaNum))
    {
        /* Yep, build a reset packet */
        ZeroMemory(&ncb, sizeof(ncb));
        ncb.ncb_lsn = 0;
        ncb.ncb_command = NCBRESET;
        ncb.ncb_callname[0] = 0xFE; // Max Sessions
        ncb.ncb_callname[1] = 0;
        ncb.ncb_callname[2] = 0xFD; // Max Names
        ncb.ncb_callname[3] = 0;
        ncb.ncb_lana_num = LanaNum;

        /* Send it */
        nbtError = Netbios(&ncb);

        /* Convert the status to Win32 format */
        dwError = SvcNetBiosStatusToApiStatus(nbtError);

        /* Enable the LANA if the reset worked */
        if (dwError == ERROR_SUCCESS) SetLanaFlag(LanaNum);
    }

    /* Drop the lock and return */
    LeaveCriticalSection(&SvcNetBiosCritSec);
    return dwError;
}
