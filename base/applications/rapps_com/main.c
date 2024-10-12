/*
 * PROJECT:     ReactOS Applications Manager Command-Line Launcher (rapps.com)
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Allow explorer / cmd to wait for rapps.exe when passing commandline arguments
 * COPYRIGHT:   Copyright 2020 Mark Jansen (mark.jansen@reactos.org)
 */

#include <windef.h>
#include <winbase.h>
#include <strsafe.h>

int run_rapps(LPWSTR cmdline)
{
    DWORD dwExit;
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };

    SetLastError(0);
    if (!CreateProcessW(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        fprintf(stderr, "Unable to create rapps.exe process...\n");
        return -1;
    }
    CloseHandle(pi.hThread);
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &dwExit);
    CloseHandle(pi.hProcess);
    return (int)dwExit;
}

int wmain(int argc, wchar_t* argv[])
{
    int iRet;
    int n;
    size_t arglen;
    WCHAR RappsExe[MAX_PATH] = { 0 };
    wchar_t* cmdline;

    GetModuleFileNameW(NULL, RappsExe, ARRAYSIZE(RappsExe));
    arglen = wcslen(RappsExe);
    if (arglen > 4 && !_wcsicmp(RappsExe + arglen - 4, L".com"))
    {
        wcscpy(RappsExe + arglen - 4, L".exe");
    }
    else
    {
        fprintf(stderr, "Unable to build rapps.exe path...\n");
        return -1;
    }

    arglen += (1 + 2);  // NULL terminator + 2 quotes

    for (n = 1; n < argc; ++n)
    {
        arglen += wcslen(argv[n]);
        arglen += 3;    // Surrounding quotes + space
    }

    cmdline = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, arglen * sizeof(WCHAR));
    if (cmdline)
    {
        wchar_t* ptr = cmdline;
        size_t cchRemaining = arglen;

        StringCchPrintfExW(ptr, cchRemaining, &ptr, &cchRemaining, 0, L"\"%s\"", RappsExe);

        for (n = 1; n < argc; ++n)
        {
            StringCchPrintfExW(ptr, cchRemaining, &ptr, &cchRemaining, 0, L" \"%s\"", argv[n]);
        }
    }

    iRet = run_rapps(cmdline);
    if (cmdline)
        HeapFree(GetProcessHeap(), 0, cmdline);
    return iRet;
}
