/*
 * PROJECT:     ReactOS Spooler Router
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to switching between security contexts
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

/**
 * @see RevertToPrinterSelf
 */
BOOL WINAPI
ImpersonatePrinterClient(HANDLE hToken)
{
    if (!hToken)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (!SetThreadToken(NULL, hToken))
    {
        ERR("SetThreadToken failed with error %u!\n", GetLastError());
        CloseHandle(hToken);
        return FALSE;
    }

    CloseHandle(hToken);
    return TRUE;
}

/**
 * RevertToPrinterSelf reverts the security context from the current user's context back to the process context.
 * As spoolss.dll is used by spoolsv.exe, this is usually the SYSTEM security context.
 *
 * Unlike the traditional ImpersonateClient and then RevertToSelf approach, we do it the other way round here,
 * because spoolss.dll is delay-loaded by spoolsv.exe in the current user's context. Use RevertToPrinterSelf then to
 * return to the SYSTEM context for specific tasks.
 */
HANDLE WINAPI
RevertToPrinterSelf()
{
    HANDLE hToken;

    // Retrieve our current impersonation token
    if (!OpenThreadToken(GetCurrentThread(), TOKEN_IMPERSONATE, TRUE, &hToken))
    {
        ERR("OpenThreadToken failed with error %u!\n", GetLastError());
        return NULL;
    }

    // Tell the thread to stop impersonating
    if (!SetThreadToken(NULL, NULL))
    {
        ERR("SetThreadToken failed with error %u!\n", GetLastError());
        return NULL;
    }

    // Return the token required for reverting back to impersonation in ImpersonatePrinterClient
    return hToken;
}
