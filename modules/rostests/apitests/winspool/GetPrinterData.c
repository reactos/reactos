/*
 * PROJECT:     ReactOS Print Spooler DLL API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for GetPrinterData(Ex)A/GetPrinterData(Ex)W/SetPrinterData(Ex)A/SetPrinterData(Ex)W
 * COPYRIGHT:   Copyright 2017 Colin Finck (colin@reactos.org)
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winspool.h>
#include <winreg.h>

/* From printing/include/spoolss.h */
#define MAX_PRINTER_NAME        220

typedef struct _SPLREG_VALUE
{
    PSTR pszName;
    PWSTR pwszName;
    DWORD dwType;
    DWORD cbNeededA;
    BOOL bSettable;
}
SPLREG_VALUE, *PSPLREG_VALUE;

SPLREG_VALUE SplRegValues[] = {
    { "DefaultSpoolDirectory", L"DefaultSpoolDirectory", REG_SZ, 0xFFFFFFFF, TRUE },
    { "PortThreadPriorityDefault", L"PortThreadPriorityDefault", REG_NONE, 4, FALSE },
    { "PortThreadPriority", L"PortThreadPriority", REG_DWORD, 4, TRUE },
    { "SchedulerThreadPriorityDefault", L"SchedulerThreadPriorityDefault", REG_NONE, 4, FALSE },
    { "SchedulerThreadPriority", L"SchedulerThreadPriority", REG_DWORD, 4, TRUE },
    { "BeepEnabled", L"BeepEnabled", REG_DWORD, 4, TRUE },

    /* These fail in Win8, probably removed since NT6:

    { "NetPopup", L"NetPopup", REG_DWORD, 4, TRUE },
    { "RetryPopup", L"RetryPopup", REG_DWORD, 4, TRUE },
    { "NetPopupToComputer", L"NetPopupToComputer", REG_DWORD, 4, TRUE },

    */

    { "EventLog", L"EventLog", REG_DWORD, 4, TRUE },
    { "MajorVersion", L"MajorVersion", REG_NONE, 4, FALSE },
    { "MinorVersion", L"MinorVersion", REG_NONE, 4, FALSE },
    { "Architecture", L"Architecture", REG_NONE, 0xFFFFFFFF, FALSE },
    { "OSVersion", L"OSVersion", REG_NONE, sizeof(OSVERSIONINFOA), FALSE },
    { "OSVersionEx", L"OSVersionEx", REG_NONE, sizeof(OSVERSIONINFOEXA), FALSE },
#if 0
    { "DsPresent", L"DsPresent", REG_DWORD, 4, FALSE },
    { "DsPresentForUser", L"DsPresentForUser", REG_DWORD, 4, FALSE },
#endif
    { "RemoteFax", L"RemoteFax", REG_NONE, 4, FALSE },
    { "RestartJobOnPoolError", L"RestartJobOnPoolError", REG_DWORD, 4, TRUE },
    { "RestartJobOnPoolEnabled", L"RestartJobOnPoolEnabled", REG_DWORD, 4, TRUE },
#if 0 // FIXME: fails on WHS testbot with ERROR_INVALID_PARAMETER
    { "DNSMachineName", L"DNSMachineName", REG_SZ, 0xFFFFFFFF, FALSE },
#endif
    { "AllowUserManageForms", L"AllowUserManageForms", REG_DWORD, 4, TRUE },
    { NULL, NULL, 0, 0, FALSE }
};

START_TEST(GetPrinterData)
{
    DWORD cbNeeded;
    DWORD cchDefaultPrinter;
    DWORD dwReturnCode;
    DWORD dwType;
    HANDLE hPrinter;
    PBYTE pDataA;
    PBYTE pDataW;
    PSPLREG_VALUE p;
    WCHAR wszDefaultPrinter[MAX_PRINTER_NAME + 1];

    // Don't supply any parameters, this has to fail with ERROR_INVALID_HANDLE!
    dwReturnCode = GetPrinterDataExW(NULL, NULL, NULL, NULL, NULL, 0, NULL);
    ok(dwReturnCode == ERROR_INVALID_HANDLE, "GetPrinterDataExW returns error %lu!\n", dwReturnCode);

    // Open a handle to the local print server.
    if (!OpenPrinterW(NULL, &hPrinter, NULL))
    {
        skip("Could not retrieve a handle to the local print server!\n");
        return;
    }

    // Now try with valid handle, but leave remaining parameters NULL.
    dwReturnCode = GetPrinterDataExW(hPrinter, NULL, NULL, NULL, NULL, 0, NULL);
    ok(dwReturnCode == RPC_X_NULL_REF_POINTER, "GetPrinterDataExW returns error %lu!\n", dwReturnCode);

    // Try all valid Print Server data values.
    for (p = SplRegValues; p->pszName; p++)
    {
        // Try the ANSI version of the function.
        dwType = 0xDEADBEEF;
        dwReturnCode = GetPrinterDataExA(hPrinter, NULL, p->pszName, &dwType, NULL, 0, &cbNeeded);
        ok(dwReturnCode == ERROR_MORE_DATA || dwReturnCode == ERROR_FILE_NOT_FOUND, "GetPrinterDataExA returns %lu for \"%s\"!\n", dwReturnCode, p->pszName);
        if (dwReturnCode != ERROR_MORE_DATA)
            continue;

        ok(dwType == p->dwType, "dwType is %lu for \"%s\"!\n", dwType, p->pszName);

        if (p->cbNeededA < 0xFFFFFFFF)
            ok(cbNeeded == p->cbNeededA, "cbNeeded is %lu for \"%s\", but expected %lu!\n", cbNeeded, p->pszName, p->cbNeededA);
        else
            ok(cbNeeded > 0, "cbNeeded is 0 for \"%s\"!\n", p->pszName);

        pDataA = HeapAlloc(GetProcessHeap(), 0, cbNeeded);
        dwReturnCode = GetPrinterDataExA(hPrinter, NULL, p->pszName, NULL, pDataA, cbNeeded, &cbNeeded);
        ok(dwReturnCode == ERROR_SUCCESS, "GetPrinterDataExA returns %lu for \"%s\"!\n", dwReturnCode, p->pszName);

        // Try the Unicode version of the function too.
        dwType = 0xDEADBEEF;
        dwReturnCode = GetPrinterDataExW(hPrinter, NULL, p->pwszName, &dwType, NULL, 0, &cbNeeded);
        ok(dwReturnCode == ERROR_MORE_DATA, "GetPrinterDataExW returns %lu for \"%s\"!\n", dwReturnCode, p->pszName);
        ok(dwType == p->dwType, "dwType is %lu for \"%s\"!\n", dwType, p->pszName);

        pDataW = HeapAlloc(GetProcessHeap(), 0, cbNeeded);
        dwReturnCode = GetPrinterDataExW(hPrinter, NULL, p->pwszName, NULL, pDataW, cbNeeded, &cbNeeded);
        ok(dwReturnCode == ERROR_SUCCESS, "GetPrinterDataExW returns %lu for \"%s\"!\n", dwReturnCode, p->pszName);

        // Verify that OSVERSIONINFO structures are correctly returned.
        if (strcmp(p->pszName, "OSVersion") == 0)
        {
            POSVERSIONINFOA pOSVersionInfoA = (POSVERSIONINFOA)pDataA;
            POSVERSIONINFOW pOSVersionInfoW = (POSVERSIONINFOW)pDataW;
            ok(pOSVersionInfoA->dwOSVersionInfoSize == sizeof(OSVERSIONINFOA), "dwOSVersionInfoSize is %lu!\n", pOSVersionInfoA->dwOSVersionInfoSize);
            ok(pOSVersionInfoW->dwOSVersionInfoSize == sizeof(OSVERSIONINFOW), "dwOSVersionInfoSize is %lu!\n", pOSVersionInfoW->dwOSVersionInfoSize);
        }
        else if (strcmp(p->pszName, "OSVersionEx") == 0)
        {
            POSVERSIONINFOEXA pOSVersionInfoA = (POSVERSIONINFOEXA)pDataA;
            POSVERSIONINFOEXW pOSVersionInfoW = (POSVERSIONINFOEXW)pDataW;
            ok(pOSVersionInfoA->dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXA), "dwOSVersionInfoSize is %lu!\n", pOSVersionInfoA->dwOSVersionInfoSize);
            ok(pOSVersionInfoW->dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXW), "dwOSVersionInfoSize is %lu!\n", pOSVersionInfoW->dwOSVersionInfoSize);
        }

        // Shortly test SetPrinterDataExW by setting the same data we just retrieved.
        if (p->bSettable)
        {
            dwReturnCode = SetPrinterDataExW(hPrinter, NULL, p->pwszName, dwType, pDataW, cbNeeded);
            ok(dwReturnCode == ERROR_SUCCESS, "SetPrinterDataExW returns %lu for \"%s\"!\n", dwReturnCode, p->pszName);
        }

        HeapFree(GetProcessHeap(), 0, pDataA);
        HeapFree(GetProcessHeap(), 0, pDataW);
    }

    // Try an invalid one.
    dwReturnCode = GetPrinterDataExW(hPrinter, NULL, L"Invalid", NULL, NULL, 0, &cbNeeded);
    ok(dwReturnCode == ERROR_INVALID_PARAMETER, "GetPrinterDataExW returns %lu!\n", dwReturnCode);

    ClosePrinter(hPrinter);

    // Open a handle to the default printer.
    cchDefaultPrinter = _countof(wszDefaultPrinter);
    ok(GetDefaultPrinterW(wszDefaultPrinter, &cchDefaultPrinter), "GetDefaultPrinterW returns FALSE and requires %lu characters!\n", cchDefaultPrinter);
    if (!OpenPrinterW(wszDefaultPrinter, &hPrinter, NULL))
    {
        skip("Could not retrieve a handle to the default printer!\n");
        return;
    }

    // Using NULL or L"" for pKeyName on a Printer handle yields ERROR_INVALID_PARAMETER.
    dwReturnCode = GetPrinterDataExW(hPrinter, NULL, L"Name", NULL, NULL, 0, &cbNeeded);
    ok(dwReturnCode == ERROR_INVALID_PARAMETER, "GetPrinterDataExW returns %lu!\n", dwReturnCode);
    dwReturnCode = GetPrinterDataExW(hPrinter, L"", L"Name", NULL, NULL, 0, &cbNeeded);
    ok(dwReturnCode == ERROR_INVALID_PARAMETER, "GetPrinterDataExW returns %lu!\n", dwReturnCode);

    // Using L"\\" allows us to examine the contents of the main printer key anyway.
    dwReturnCode = GetPrinterDataExW(hPrinter, L"\\", L"Name", &dwType, NULL, 0, &cbNeeded);
    ok(dwReturnCode == ERROR_MORE_DATA, "GetPrinterDataExW returns %lu!\n", dwReturnCode);
    ok(dwType == REG_SZ, "dwType is %lu!\n", dwType);
    ok(cbNeeded > 0, "cbNeeded is 0!\n");

    pDataW = HeapAlloc(GetProcessHeap(), 0, cbNeeded);
    dwReturnCode = GetPrinterDataExW(hPrinter, L"\\", L"Name", NULL, pDataW, cbNeeded, &cbNeeded);
    ok(dwReturnCode == ERROR_SUCCESS, "GetPrinterDataExW returns %lu!\n", dwReturnCode);

    // The following test fails if the default printer is a remote printer.
    ok(wcscmp((PWSTR)pDataW, wszDefaultPrinter) == 0, "pDataW is \"%S\", default printer is \"%S\"!\n", (PWSTR)pDataW, wszDefaultPrinter);

    // SetPrinterDataExW should return ERROR_ACCESS_DENIED when attempting to set the Name.
    dwReturnCode = SetPrinterDataExW(hPrinter, L"\\", L"Name", REG_SZ, pDataW, cbNeeded);
    ok(dwReturnCode == ERROR_ACCESS_DENIED, "SetPrinterDataExW returns %lu!\n", dwReturnCode);

    HeapFree(GetProcessHeap(), 0, pDataW);
}
