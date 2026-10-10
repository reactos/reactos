/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for ExitThread in the last running thread
 * COPYRIGHT:   Copyright 2026 Tomas Srnka <tomas.srnka@e2b.dev>
 */

#include "precomp.h"

#define CHILD_EXIT_CODE 42

static
DWORD
WINAPI
ReturnThread(
    _In_ PVOID Parameter)
{
    return 0;
}

static
VOID
RunChild(VOID)
{
    HANDLE hThread;

    /* The exited thread stays referenced through this handle */
    hThread = CreateThread(NULL, 0, ReturnThread, NULL, 0, NULL);
    if (hThread)
        WaitForSingleObject(hThread, INFINITE);

    /* This is the last running thread, so this must end the process */
    ExitThread(CHILD_EXIT_CODE);
}

START_TEST(ExitThread)
{
    int argc;
    char **argv;
    WCHAR FileName[MAX_PATH];
    WCHAR CommandLine[MAX_PATH + 32];
    STARTUPINFOW StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    DWORD Wait, ExitCode;
    BOOL Success;

    argc = winetest_get_mainargs(&argv);
    if (argc >= 3 && !strcmp(argv[2], "child"))
    {
        RunChild();
        return;
    }

    GetModuleFileNameW(NULL, FileName, _countof(FileName));
    StringCbPrintfW(CommandLine, sizeof(CommandLine), L"\"%ls\" ExitThread child", FileName);

    RtlZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.dwFlags = STARTF_USESTDHANDLES;

    Success = CreateProcessW(FileName,
                             CommandLine,
                             NULL,
                             NULL,
                             FALSE,
                             0,
                             NULL,
                             NULL,
                             &StartupInfo,
                             &ProcessInfo);
    if (!Success)
    {
        skip("CreateProcess failed with %lu\n", GetLastError());
        return;
    }
    CloseHandle(ProcessInfo.hThread);

    Wait = WaitForSingleObject(ProcessInfo.hProcess, 30000);
    ok(Wait == WAIT_OBJECT_0, "The child process did not exit: %lu\n", Wait);
    if (Wait == WAIT_OBJECT_0)
    {
        Success = GetExitCodeProcess(ProcessInfo.hProcess, &ExitCode);
        ok(Success, "GetExitCodeProcess failed with %lu\n", GetLastError());
        ok_hex(ExitCode, CHILD_EXIT_CODE);
    }
    else
    {
        TerminateProcess(ProcessInfo.hProcess, 1);
    }
    CloseHandle(ProcessInfo.hProcess);
}
