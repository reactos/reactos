/*
 * PROJECT:     ReactOS Local Spooler API Tests Injected DLL
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Main functions
 * COPYRIGHT:   Copyright 2015-2016 Colin Finck <colin@reactos.org>
 */

#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

#define WIN32_NO_STATUS
#include <io.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winreg.h>
#include <winspool.h>
#include <winsplp.h>

#include "../localspl_apitest.h"

//#define NDEBUG
#include <debug.h>

// Test list
extern void func_fpEnumPrinters(void);
extern void func_fpGetPrintProcessorDirectory(void);

const struct test winetest_testlist[] =
{
    { "fpEnumPrinters", func_fpEnumPrinters },
    { "fpGetPrintProcessorDirectory", func_fpGetPrintProcessorDirectory },
    { 0, 0 }
};

BOOL
GetLocalsplFuncs(LPPRINTPROVIDOR pp)
{
    HMODULE hLocalspl;
    PInitializePrintProvidor pfnInitializePrintProvidor;

    // Get us a handle to the loaded localspl.dll.
    hLocalspl = GetModuleHandleW(L"localspl");
    if (!hLocalspl)
    {
        skip("GetModuleHandleW failed with error %u!\n", GetLastError());
        return FALSE;
    }

    // Get a pointer to its InitializePrintProvidor function.
    pfnInitializePrintProvidor = (PInitializePrintProvidor)GetProcAddress(hLocalspl, "InitializePrintProvidor");
    if (!pfnInitializePrintProvidor)
    {
        skip("GetProcAddress failed with error %u!\n", GetLastError());
        return FALSE;
    }

    // Get localspl's function pointers.
    if (!pfnInitializePrintProvidor(pp, sizeof(PRINTPROVIDOR), NULL))
    {
        skip("pfnInitializePrintProvidor failed with error %u!\n", GetLastError());
        return FALSE;
    }

    return TRUE;
}

PVOID
GetSpoolssFunc(const char* FunctionName)
{
    HMODULE hSpoolss;

    // Get us a handle to the loaded spoolss.dll.
    hSpoolss = GetModuleHandleW(L"spoolss");
    if (!hSpoolss)
    {
        skip("GetModuleHandleW failed with error %u!\n", GetLastError());
        return FALSE;
    }

    return GetProcAddress(hSpoolss, FunctionName);
}

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

    // Restore stdout to the previous value.
    fclose(fpStdout);
    _dup2(iOldStdout, _fileno(stdout));

    // Return FALSE so that our DLL is immediately unloaded.
    return FALSE;
}
