/*
 * PROJECT:     ReactOS Local Spooler API Tests Injected DLL
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Main functions
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck (colin@reactos.org)
 */

#define STANDALONE
#include <apitest.h>

#include <stdio.h>

#define WIN32_NO_STATUS
#include <io.h>
#include <windef.h>
#include <winbase.h>

#include "../services_apitest.h"

//#define NDEBUG
#include <debug.h>

// include definition for wine-testlist
#include "../testlist.h"

// func_service is not needed for our dll.
// but we have only one testlist for dll and exe.
// so we need here this dummy
extern void func_service(void) {}

// Running the tests from the injected DLL and redirecting their output to the pipe.
BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    char szTestName[150];
    DWORD cbRead;
    FILE* fpStdout;
    HANDLE hCommandPipe;
    int iOldStdout;

    // We only want to run our test once when the DLL is injected to the process.
    if (fdwReason != DLL_PROCESS_ATTACH)
        return TRUE;

    // Read the test to run from the command pipe.
    hCommandPipe = CreateFileW(COMMAND_PIPE_NAME, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hCommandPipe == INVALID_HANDLE_VALUE)
    {
        DPRINT("DLL: CreateFileW failed for the command pipe with error %lu!\n", GetLastError());
        return FALSE;
    }

    if (!ReadFile(hCommandPipe, szTestName, sizeof(szTestName), &cbRead, NULL))
    {
        DPRINT("DLL: ReadFile failed for the command pipe with error %lu!\n", GetLastError());
        return FALSE;
    }

    CloseHandle(hCommandPipe);

    // Check if the test name is valid.
    if (!find_test(szTestName))
    {
        DPRINT("DLL: Got invalid test name \"%s\"!\n", szTestName);
        return FALSE;
    }

    // Backup our current stdout and set it to the output pipe.
    iOldStdout = _dup(_fileno(stdout));
    fpStdout = _wfreopen(OUTPUT_PIPE_NAME, L"w", stdout);
    setbuf(stdout, NULL);

    // Run the test.
    run_test(szTestName);

    fflush(fpStdout);

    // Restore stdout to the previous value.
    fclose(fpStdout);
    _dup2(iOldStdout, _fileno(stdout));

    // Return FALSE so that our DLL is immediately unloaded.
    return FALSE;
}
