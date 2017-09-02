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
    DWORD cbReturned;
    DWORD dwErrorCode;
    TOKEN_TYPE Type;

    // Sanity check
    if (!hToken)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    // Get the type of the supplied token.
    if (!GetTokenInformation(hToken, TokenType, &Type, sizeof(TOKEN_TYPE), &cbReturned))
    {
        dwErrorCode = GetLastError();
        ERR("GetTokenInformation failed with error %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // Check if this is an impersonation token and only set it as the thread token in this case.
    // This is not always an impersonation token, see RevertToPrinterSelf.
    if (Type == TokenImpersonation)
    {
        if (!SetThreadToken(NULL, hToken))
        {
            dwErrorCode = GetLastError();
            ERR("SetThreadToken failed with error %lu!\n", dwErrorCode);
            goto Cleanup;
        }
    }

Cleanup:
    if (hToken)
        CloseHandle(hToken);

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
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
    DWORD dwErrorCode;
    HANDLE hReturnValue = NULL;
    HANDLE hToken = NULL;

    // All spoolss code is usually called after impersonating the client. In this case, we can retrieve our current thread impersonation token using OpenThreadToken.
    // But in rare occasions, spoolss code is also called from a higher-privileged thread that doesn't impersonate the client. Then we don't get an impersonation token.
    // Anyway, we can't just return nothing in this case, because this is being treated as failure by the caller. So we return the token of the current process.
    // This behaviour is verified with Windows!
    if (OpenThreadToken(GetCurrentThread(), TOKEN_IMPERSONATE, TRUE, &hToken))
    {
        // Tell the thread to stop impersonating.
        if (!SetThreadToken(NULL, NULL))
        {
            dwErrorCode = GetLastError();
            ERR("SetThreadToken failed with error %lu!\n", dwErrorCode);
            goto Cleanup;
        }
    }
    else if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        dwErrorCode = GetLastError();
        ERR("OpenProcessToken failed with error %lu!\n", dwErrorCode);
        goto Cleanup;
    }

    // We were successful, return a token!
    dwErrorCode = ERROR_SUCCESS;
    hReturnValue = hToken;

    // Don't let the cleanup routine close this.
    hToken = NULL;

Cleanup:
    if (hToken)
        CloseHandle(hToken);

    SetLastError(dwErrorCode);
    return hReturnValue;
}
