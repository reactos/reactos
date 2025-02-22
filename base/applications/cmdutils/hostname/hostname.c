/*
 * PROJECT:     ReactOS Hostname Command
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Retrieves the current DNS host name of the computer.
 * COPYRIGHT:   Copyright 2005-2019 Emanuele Aliberti (ea@reactos.com)
 *              Copyright 2019 Hermes Belusca-Maito
 */

#include <stdlib.h>
#include <conio.h>

#include <windef.h>
#include <winbase.h>
#include <winuser.h>

#include "resource.h"

int wmain(int argc, WCHAR* argv[])
{
    WCHAR Msg[100];

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

        if (bSuccess)
        {
            /* Print out the host name */
            _cwprintf(L"%s\n", HostName);
        }

        /* If a larger buffer has been allocated, free it */
        if (HostName && (HostName != LocalHostName))
            HeapFree(GetProcessHeap(), 0, HostName);

        if (!bSuccess)
        {
            /* Fail in case of error */
            LoadStringW(GetModuleHandle(NULL), IDS_ERROR, Msg, _countof(Msg));
            _cwprintf(L"%s %lu.\n", Msg, GetLastError());
            return 1;
        }
    }
    else
    {
        if ((_wcsicmp(argv[1], L"-s") == 0) || (_wcsicmp(argv[1], L"/s") == 0))
        {
            /* The program doesn't allow the user to set the host name */
            LoadStringW(GetModuleHandle(NULL), IDS_NOSET, Msg, _countof(Msg));
            _cwprintf(L"%s\n", Msg);
            return 1;
        }
        else
        {
            /* Let the user know what the program does */
            LoadStringW(GetModuleHandle(NULL), IDS_USAGE, Msg, _countof(Msg));
            _cwprintf(L"\n%s\n\n", Msg);
        }
    }

    return 0;
}

/* EOF */
