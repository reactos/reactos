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

#define MAX_PASSWORD_LENGTH 64

static
VOID
Usage(VOID)
{
    int i;
    for (i = IDS_USAGE01; i <= IDS_USAGE_MAX; i++)
        ConResPuts(StdOut, i);
}


static
VOID
ConInString(
    _In_ PWSTR pInput,
    _In_ DWORD dwLength)
{
    DWORD dwOldMode;
    DWORD dwRead = 0;
    HANDLE hFile;
    PWSTR p;
    PCHAR pBuf;

    pBuf = (PCHAR)HeapAlloc(GetProcessHeap(), 0, dwLength - 1);
    ZeroMemory(pInput, dwLength * sizeof(WCHAR));
    hFile = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hFile, &dwOldMode);

    SetConsoleMode(hFile, ENABLE_LINE_INPUT /*| ENABLE_ECHO_INPUT*/);

    ReadFile(hFile, (PVOID)pBuf, dwLength - 1, &dwRead, NULL);

    MultiByteToWideChar(GetConsoleCP(), 0, pBuf, dwRead, pInput, dwLength - 1);
    HeapFree(GetProcessHeap(), 0, pBuf);

    for (p = pInput; *p; p++)
    {
        if (*p == L'\x0d')
        {
            *p = UNICODE_NULL;
            break;
        }
    }

    SetConsoleMode(hFile, dwOldMode);
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

    if (pszDomain == NULL)
    {
        DWORD dwLength = MAX_COMPUTERNAME_LENGTH + 1;
        pszDomain = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwLength * sizeof(WCHAR));
        if (pszDomain)
            GetComputerNameW(pszDomain, &dwLength);
    }

    pszPassword = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (MAX_PASSWORD_LENGTH + 1) * sizeof(WCHAR));

    /* Query the password */
    ConResPrintf(StdOut, IDS_PASSWORD, pszDomain, pszUserName);
    ConInString(pszPassword, MAX_PASSWORD_LENGTH + 1);
    ConPuts(StdOut, L"\n");

    ConResPrintf(StdOut, IDS_START, pszCommandLine, pszDomain, pszUserName);

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
        ConResPrintf(StdOut, IDS_RUN_ERROR, pszCommandLine);
        ConPrintf(StdOut, L"Error: %lu\n", GetLastError());
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
