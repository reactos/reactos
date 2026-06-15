/*
 * PROJECT:     ReactOS Hostname Command
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Retrieves the current DNS host name of the computer.
 * COPYRIGHT:   Copyright 2005 Emanuele Aliberti <ea@reactos.com>
 *              Copyright 2019-2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include <stdlib.h>
#include <stdio.h>

#include <windef.h>
#include <winbase.h>

#include <conutils.h>

#include "resource.h"

static VOID
PrintError(
    _In_opt_ PCWSTR Message,
    _In_ DWORD dwError)
{
    INT Len;

    if (dwError == ERROR_SUCCESS)
        return;

    if (IS_INTRESOURCE(Message))
        ConResPuts(StdErr, PtrToUlong(Message));
    else // if (Message)
        ConPuts(StdErr, Message);
    Len = ConMsgPuts(StdErr, FORMAT_MESSAGE_FROM_SYSTEM,
                     NULL, dwError, LANG_USER_DEFAULT);
    if (Len <= 0) /* Fall back in case the error is not defined */
        ConPrintf(StdErr, L"%lu\n", dwError);
}

int wmain(int argc, WCHAR* argv[])
{
    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    if (argc == 1)
    {
        BOOL bSuccess;
        WCHAR LocalHostName[256] = L""; // MAX_COMPUTERNAME_LENGTH + 1 for NetBIOS name.
        DWORD HostNameSize = _countof(LocalHostName);
        PWSTR HostName = LocalHostName;

        /* Try to retrieve the host name using the local buffer */
        bSuccess = GetComputerNameExW(ComputerNameDnsHostname, HostName, &HostNameSize);
        if (!bSuccess && (GetLastError() == ERROR_MORE_DATA))
        {
            /* Retry with a larger buffer since the local buffer was too small */
            HostName = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, HostNameSize * sizeof(WCHAR));
            if (HostName)
                bSuccess = GetComputerNameExW(ComputerNameDnsHostname, HostName, &HostNameSize);
        }

        /* Print out the host name */
        if (bSuccess)
            ConPrintf(StdOut, L"%s\n", HostName);

        /* If a larger buffer has been allocated, free it */
        if (HostName && (HostName != LocalHostName))
            HeapFree(GetProcessHeap(), 0, HostName);

        if (!bSuccess)
        {
            /* Fail in case of error */
            PrintError(MAKEINTRESOURCEW(IDS_ERROR), GetLastError());
            return 1;
        }
    }
    else
    {
        if ((_wcsicmp(argv[1], L"-s") == 0) || (_wcsicmp(argv[1], L"/s") == 0))
        {
            /* The program doesn't allow the user to set the host name */
            ConResPuts(StdErr, IDS_NOSET);
            return 1;
        }
        else
        {
            /* Let the user know what the program does */
            ConResPuts(StdOut, IDS_USAGE);
        }
    }

    return 0;
}

/* EOF */
