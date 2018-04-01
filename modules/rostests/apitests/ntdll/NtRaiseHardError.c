/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test for NtRaiseHardError
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <ndk/exfuncs.h>
#include <strsafe.h>
#include <tlhelp32.h>


START_TEST(NtRaiseHardError)
{
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = { 0 };
    char self[MAX_PATH];
    char buffer[MAX_PATH*2];
    int argc;
    char** argv;
    BOOL ret;
    DWORD dwret;
    HANDLE hDup;

    //if (!winetest_interactive)
    //{
    //    skip("Spawns dialogs that need to be manually closed (set WINETEST_INTERACTIVE=1)\n");
    //    return;
    //}

    argc = winetest_get_mainargs(&argv);
    if (argc >= 3)
    {
        if (!strcmp(argv[2], "raise"))
        {
            ULONG HardErrorResponse;
            SetErrorMode(0);
            NtRaiseHardError(STATUS_INVALID_IMAGE_FORMAT, 0, 0, NULL, OptionOk, &HardErrorResponse);
            ok(0, "Child process did not die\n");
        }
        else
        {
            ok(0, "Unexpected command %s\n", argv[2]);
        }

        ExitProcess(winetest_get_failures());   /* Do not print the success / failure line */
    }

    GetModuleFileNameA(NULL, self, sizeof(self));
    StringCchPrintfA(buffer, _countof(buffer), "\"%s\" tests/NtRaiseHardError.c raise", self);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi), "CreateProcess\n");
    /* wait for child to terminate */
    dwret = WaitForSingleObject(pi.hProcess, 100);
    ok(dwret == WAIT_TIMEOUT, "Child process termination\n");

    if (dwret == WAIT_TIMEOUT)
    {
        HANDLE hSnap, hProc;
        PROCESSENTRY32 pe = { 0 };

        TerminateProcess(pi.hProcess, 123);
        dwret = WaitForSingleObject(pi.hProcess, 100);
        ok(dwret == WAIT_OBJECT_0, "Child process termination\n");

        hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnap != INVALID_HANDLE_VALUE)
        {
            pe.dwSize = sizeof(pe);
            if (Process32First(hSnap, &pe))
            {
                do
                {
                    if (pe.th32ProcessID == pi.dwProcessId)
                    {
                        ok(0, "Process 0x%x found!\n", pi.dwProcessId);
                    }
                } while (Process32Next(hSnap, &pe));
            }
            CloseHandle(hSnap);
        }

        hProc = OpenProcess(pi.dwProcessId, FALSE, PROCESS_QUERY_INFORMATION);
        ok_ptr(hProc, NULL);
        if (hProc)
            CloseHandle(hProc);
    }

    ret = GetExitCodeProcess(pi.hProcess, &dwret);
    ok(ret == TRUE, "GetExitCodeProcess returned ret=%u\n", ret);
    ok(dwret == 123, "GetExitCodeProcess returned dwret=%u\n", dwret);

    SetLastError(123456);
    ret = DuplicateHandle(GetCurrentProcess(), pi.hProcess, GetCurrentProcess(), &hDup, 0, FALSE, DUPLICATE_SAME_ACCESS);
    dwret = GetLastError();
    ok(ret == TRUE, "Expected DuplicateHandle to fail\n");
    ok(dwret == 123456, "GetLastError was %u\n", dwret);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}
