/*
 * PROJECT:     ReactOS runas utility
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/runas/runas.c
 * COPYRIGHT:   Copyright 2022 Eric Kohl <eric.kohl@reactos.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdarg.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <wincon.h>
#include <winsvc.h>
#include <conutils.h>

#include "resource.h"

#define NDEBUG
#include <debug.h>

static
void
Usage(void)
{
    ConResPuts(StdOut, IDS_USAGE01);
    ConResPuts(StdOut, IDS_USAGE02);
    ConResPuts(StdOut, IDS_USAGE03);
    ConResPuts(StdOut, IDS_USAGE04);
    ConResPuts(StdOut, IDS_USAGE05);
    ConResPuts(StdOut, IDS_USAGE06);
    ConResPuts(StdOut, IDS_USAGE07);
}


int
wmain(
    int argc,
    LPCWSTR argv[])
{
    LPCWSTR pszArg;
    int i, result = 0;
    BOOL bProfile = FALSE, bNoProfile = FALSE;
    BOOL bEnv = FALSE;
    PWSTR pszUserName = NULL;
    PWSTR pszDomain = NULL;
    PWSTR pszCommandLine = NULL;
    PWSTR pszPassword = NULL;
    PWSTR ptr;
    STARTUPINFOW StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    DWORD dwLogonFlags = 0;
    BOOL rc;

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    if (argc == 1)
    {
        Usage();
        return 0;
    }

    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    ZeroMemory(&ProcessInfo, sizeof(ProcessInfo));

    for (i = 1; i < argc; i++)
    {
        pszArg = argv[i];
        if (*pszArg == L'-' || *pszArg == L'/')
        {
            pszArg++;
            if (wcscmp(pszArg, L"?") == 0)
            {
                Usage();
            }
            else if (wcsicmp(pszArg, L"profile") == 0)
            {
                bProfile = TRUE;
            }
            else if (wcsicmp(pszArg, L"noprofile") == 0)
            {
                bNoProfile = TRUE;
            }
            else if (wcsicmp(pszArg, L"env") == 0)
            {
                bEnv = TRUE;
            }
            else if (_wcsnicmp(pszArg, L"user:", 5) == 0)
            {
                pszArg += 5;
                ptr = wcschr(pszArg, L'@');
                if (ptr != NULL)
                {
                    /* User@Domain */
                    pszUserName = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ((ptr - pszArg) + 1) * sizeof(WCHAR));
                    if (pszUserName)
                        wcsncpy(pszUserName, pszArg, (ptr - pszArg));

                    ptr++;
                    pszDomain = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (wcslen(ptr) + 1) * sizeof(WCHAR));
                    if (pszDomain)
                        wcscpy(pszDomain, ptr);
                }
                else
                {
                    ptr = wcschr(pszArg, L'\\');
                    if (ptr != NULL)
                    {
                        /* Domain\User */
                        pszUserName = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (wcslen(ptr + 1) + 1)* sizeof(WCHAR));
                        if (pszUserName)
                            wcscpy(pszUserName, (ptr + 1));

                        pszDomain = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ((ptr - pszArg) + 1) * sizeof(WCHAR));
                        if (pszDomain)
                            wcsncpy(pszDomain, pszArg, (ptr - pszArg));
                    }
                    else
                    {
                        /* User */
                        pszUserName = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (wcslen(pszArg) + 1) * sizeof(WCHAR));
                        if (pszUserName)
                            wcscpy(pszUserName, pszArg);
                    }
                }
            }
            else
            {
                Usage();
                result = -1;
            }
        }
        else
        {
            if (pszCommandLine == NULL)
            {
                pszCommandLine = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (wcslen(pszArg) + 1) * sizeof(WCHAR));
                if (pszCommandLine != NULL)
                    wcscpy(pszCommandLine, pszArg);
                break;
            }
        }
    }

    if (bProfile && bNoProfile)
    {
        Usage();
        result = -1;
        goto done;
    }

    if (bProfile)
        dwLogonFlags |= LOGON_WITH_PROFILE;

    if (bNoProfile)
        dwLogonFlags &= ~LOGON_WITH_PROFILE;

    if (bEnv)
    {
        DPRINT("env\n");
    }

    DPRINT("User: %S\n", pszUserName);
    DPRINT("Domain: %S\n", pszDomain);
    DPRINT("CommandLine: %S\n", pszCommandLine);

    /* FIXME: Query the password: */

    rc = CreateProcessWithLogonW(pszUserName,
                                 pszDomain,
                                 pszPassword,
                                 dwLogonFlags,
                                 NULL,      //[in, optional]      LPCWSTR               lpApplicationName,
                                 pszCommandLine,
                                 0,         //[in]                DWORD                 dwCreationFlags,
                                 bEnv ? GetEnvironmentStringsW() : NULL,
                                 NULL,      //[in, optional]      LPCWSTR               lpCurrentDirectory,
                                 &StartupInfo,
                                 &ProcessInfo);
    if (rc == FALSE)
    {
        DPRINT("Error: %lu\n", GetLastError());
    }

done:
    if (ProcessInfo.hThread)
        CloseHandle(ProcessInfo.hThread);

    if (ProcessInfo.hProcess)
        CloseHandle(ProcessInfo.hProcess);

    if (pszPassword)
        HeapFree(GetProcessHeap(), 0, pszPassword);

    if (pszCommandLine)
        HeapFree(GetProcessHeap(), 0, pszCommandLine);

    if (pszUserName)
        HeapFree(GetProcessHeap(), 0, pszUserName);

    if (pszDomain)
        HeapFree(GetProcessHeap(), 0, pszDomain);

    return result;
}
