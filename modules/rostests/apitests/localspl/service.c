/*
 * PROJECT:     ReactOS Local Spooler API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions needed to run our code as a service. This is needed to run in SYSTEM security context.
 * COPYRIGHT:   Copyright 2015 Colin Finck (colin@reactos.org)
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winreg.h>
#include <winsvc.h>
#include <winspool.h>
#include <winsplp.h>
#include <tlhelp32.h>

#include "localspl_apitest.h"

//#define NDEBUG
#include <debug.h>


static void
_DoDLLInjection()
{
    DWORD cbDLLPath;
    HANDLE hProcess;
    HANDLE hSnapshot;
    HANDLE hThread;
    PROCESSENTRY32W pe;
    PVOID pLoadLibraryAddress;
    PVOID pLoadLibraryArgument;
    PWSTR p;
    WCHAR wszFilePath[MAX_PATH];

    // Get the full path to our EXE file.
    if (!GetModuleFileNameW(NULL, wszFilePath, _countof(wszFilePath)))
    {
        DPRINT("GetModuleFileNameW failed with error %lu!\n", GetLastError());
        return;
    }

    // Replace the extension.
    p = wcsrchr(wszFilePath, L'.');
    if (!p)
    {
        DPRINT("File path has no file extension: %S\n", wszFilePath);
        return;
    }

    wcscpy(p, L".dll");
    cbDLLPath = (lstrlenW(wszFilePath) + 1) * sizeof(WCHAR);

    // Create a snapshot of the currently running processes.
    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        DPRINT("CreateToolhelp32Snapshot failed with error %lu!\n", GetLastError());
        return;
    }

    // Enumerate through all running processes.
    pe.dwSize = sizeof(pe);
    if (!Process32FirstW(hSnapshot, &pe))
    {
        DPRINT("Process32FirstW failed with error %lu!\n", GetLastError());
        return;
    }

    do
    {
        // Check if this is the spooler server process.
        if (_wcsicmp(pe.szExeFile, L"spoolsv.exe") != 0)
            continue;

        // Open a handle to the process.
        hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);
        if (!hProcess)
        {
            DPRINT("OpenProcess failed with error %lu!\n", GetLastError());
            return;
        }

        // Get the address of LoadLibraryW.
        pLoadLibraryAddress = (PVOID)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
        if (!pLoadLibraryAddress)
        {
            DPRINT("GetProcAddress failed with error %lu!\n", GetLastError());
            return;
        }

        // Allocate memory for the DLL path in the spooler process.
        pLoadLibraryArgument = VirtualAllocEx(hProcess, NULL, cbDLLPath, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!pLoadLibraryArgument)
        {
            DPRINT("VirtualAllocEx failed with error %lu!\n", GetLastError());
            return;
        }

        // Write the DLL path to the process memory.
        if (!WriteProcessMemory(hProcess, pLoadLibraryArgument, wszFilePath, cbDLLPath, NULL))
        {
            DPRINT("WriteProcessMemory failed with error %lu!\n", GetLastError());
            return;
        }

        // Create a new thread in the spooler process that calls LoadLibraryW as the start routine with our DLL as the argument.
        // This effectively injects our DLL into the spooler process and we can inspect localspl.dll there just like the spooler.
        hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pLoadLibraryAddress, pLoadLibraryArgument, 0, NULL);
        if (!hThread)
        {
            DPRINT("CreateRemoteThread failed with error %lu!\n", GetLastError());
            return;
        }

        CloseHandle(hThread);
        break;
    }
    while (Process32NextW(hSnapshot, &pe));
}

static DWORD WINAPI
_ServiceControlHandlerEx(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
    return NO_ERROR;
}

static void WINAPI
_ServiceMain(DWORD dwArgc, LPWSTR* lpszArgv)
{
    SERVICE_STATUS_HANDLE hServiceStatus;
    SERVICE_STATUS ServiceStatus;

    UNREFERENCED_PARAMETER(dwArgc);
    UNREFERENCED_PARAMETER(lpszArgv);

    // Register our service for control.
    hServiceStatus = RegisterServiceCtrlHandlerExW(SERVICE_NAME, _ServiceControlHandlerEx, NULL);

    // Report SERVICE_RUNNING status.
    ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    ServiceStatus.dwServiceSpecificExitCode = 0;
    ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ServiceStatus.dwWaitHint = 4000;
    ServiceStatus.dwWin32ExitCode = NO_ERROR;
    ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(hServiceStatus, &ServiceStatus);

    // Do our funky crazy stuff.
    _DoDLLInjection();

    // Our work is done.
    ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(hServiceStatus, &ServiceStatus);
}

START_TEST(service)
{
    int argc;
    char** argv;

#if defined(_M_AMD64)
    if (!winetest_interactive)
    {
        skip("ROSTESTS-366: Skipping localspl_apitest:service because it hangs on Windows Server 2003 x64-Testbot. Set winetest_interactive to run it anyway.\n");
        return;
    }
#endif

    SERVICE_TABLE_ENTRYW ServiceTable[] =
    {
        { SERVICE_NAME, _ServiceMain },
        { NULL, NULL }
    };

    // This is no real test, but an easy way to integrate the service handler routines into the API-Test executable.
    // Therefore, bail out if someone tries to run "service" as a usual test.
    argc = winetest_get_mainargs(&argv);
    if (argc != 3)
        return;

    // If we have exactly 3 arguments, we're run as a service, so initialize the corresponding service handler functions.
    StartServiceCtrlDispatcherW(ServiceTable);

    // Prevent the testing framework from outputting a "0 tests executed" line here.
    ExitProcess(0);
}
