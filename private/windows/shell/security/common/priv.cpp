//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       priv.cpp
//
//  Provides support for enabling/disabling privileges
//
//--------------------------------------------------------------------------

#include "pch.h"


/*******************************************************************

    NAME:       EnablePrivileges

    SYNOPSIS:   Enables the given privileges in the current token

    ENTRY:      pdwPrivileges - list of privileges to enable

    RETURNS:    On success, the previous thread handle (if present) or NULL
                On failure, INVALID_HANDLE_VALUE

    NOTES:      The returned handle should be passed to ReleasePrivileges
                to ensure proper cleanup.  Otherwise, if not NULL or
                INVALID_HANDLE_VALUE it should be closed with CloseHandle.

    HISTORY:
        JeffreyS    08-Oct-1996     Created

********************************************************************/
HANDLE EnablePrivileges(PDWORD pdwPrivileges, ULONG cPrivileges)
{
    BOOL                fResult;
    HANDLE              hToken;
    HANDLE              hOriginalThreadToken;
    PTOKEN_PRIVILEGES   ptp;
    ULONG               nBufferSize;

    if (!pdwPrivileges || !cPrivileges)
        return INVALID_HANDLE_VALUE;

    // Note that TOKEN_PRIVILEGES includes a single LUID_AND_ATTRIBUTES
    nBufferSize = sizeof(TOKEN_PRIVILEGES) + (cPrivileges - 1)*sizeof(LUID_AND_ATTRIBUTES);
    ptp = (PTOKEN_PRIVILEGES)LocalAlloc(LPTR, nBufferSize);
    if (!ptp)
        return INVALID_HANDLE_VALUE;

    //
    // Initialize the Privileges Structure
    //
    ptp->PrivilegeCount = cPrivileges;
    for (ULONG i = 0; i < cPrivileges; i++)
    {
        //ptp->Privileges[i].Luid = RtlConvertUlongToLuid(*pdwPrivileges++);
        ptp->Privileges[i].Luid.LowPart = *pdwPrivileges++;
        ptp->Privileges[i].Luid.HighPart = 0;
        ptp->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED;
    }

    //
    // Open the Token
    //
    hToken = hOriginalThreadToken = INVALID_HANDLE_VALUE;
    fResult = OpenThreadToken(GetCurrentThread(), TOKEN_DUPLICATE, FALSE, &hToken);
    if (fResult)
        hOriginalThreadToken = hToken;  // Remember the thread token
    else
        fResult = OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE, &hToken);

    if (fResult)
    {
        HANDLE hNewToken;

        //
        // Duplicate that Token
        //
        fResult = DuplicateTokenEx(hToken,
                                   TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                   NULL,                   // PSECURITY_ATTRIBUTES
                                   SecurityImpersonation,  // SECURITY_IMPERSONATION_LEVEL
                                   TokenImpersonation,     // TokenType
                                   &hNewToken);            // Duplicate token
        if (fResult)
        {
            //
            // Add new privileges
            //
            fResult = AdjustTokenPrivileges(hNewToken,  // TokenHandle
                                            FALSE,      // DisableAllPrivileges
                                            ptp,        // NewState
                                            0,          // BufferLength
                                            NULL,       // PreviousState
                                            NULL);      // ReturnLength
            if (fResult)
            {
                //
                // Begin impersonating with the new token
                //
                fResult = SetThreadToken(NULL, hNewToken);
            }

            CloseHandle(hNewToken);
        }
    }

    // If something failed, don't return a token
    if (!fResult)
        hOriginalThreadToken = INVALID_HANDLE_VALUE;

    // Close the original token if we aren't returning it
    if (hOriginalThreadToken == INVALID_HANDLE_VALUE && hToken != INVALID_HANDLE_VALUE)
        CloseHandle(hToken);

    // If we succeeded, but there was no original thread token,
    // return NULL to indicate we need to do SetThreadToken(NULL, NULL)
    // to release privs.
    if (fResult && hOriginalThreadToken == INVALID_HANDLE_VALUE)
        hOriginalThreadToken = NULL;

    LocalFree(ptp);

    return hOriginalThreadToken;
}


/*******************************************************************

    NAME:       ReleasePrivileges

    SYNOPSIS:   Resets privileges to the state prior to the corresponding
                EnablePrivileges call.

    ENTRY:      hToken - result of call to EnablePrivileges

    RETURNS:    nothing

    HISTORY:
        JeffreyS    08-Oct-1996     Created

********************************************************************/
void ReleasePrivileges(HANDLE hToken)
{
    if (INVALID_HANDLE_VALUE != hToken)
    {
        SetThreadToken(NULL, hToken);
        if (hToken)
            CloseHandle(hToken);
    }
}
