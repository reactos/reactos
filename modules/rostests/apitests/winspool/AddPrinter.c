/*
 * PROJECT:     ReactOS Print Spooler DLL API Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for AddPrinter, SetPrinter and DeletePrinter
 * COPYRIGHT:   Copyright 2026 Ahmed Arif <arif.ing@outlook.com>
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winspool.h>
#include <stdio.h>
#include <strsafe.h>

#define TEST_PRINTER_NAME   "ReactOS AddPrinter Test"
#define TEST_PRINTER_NAMEW  L"ReactOS AddPrinter Test"

/**
 * The ANSI entries convert the strings of the caller's structure for the
 * Unicode call underneath. That conversion must never happen in place: the
 * caller keeps owning the structure and reads its strings again afterwards.
 * Getting this wrong leaves freed Unicode pointers in the caller's ANSI
 * structure, which is what made Visual Basic 6 applications crash in
 * MultiByteToWideChar while adding a printer (CORE-19134).
 */
static void
Test_AddPrinterA_DoesNotModifyCallersStructure(void)
{
    CHAR szComment[] = "Test comment";
    CHAR szDatatype[] = "RAW";
    CHAR szDriverName[] = "ReactOS Test Driver";
    CHAR szLocation[] = "Test location";
    CHAR szParameters[] = "Test parameters";
    CHAR szPortName[] = "FILE:";
    CHAR szPrinterName[] = TEST_PRINTER_NAME;
    CHAR szPrintProcessor[] = "WinPrint";
    CHAR szSepFile[] = "Test sepfile";
    CHAR szShareName[] = "Test share";
    HANDLE hPrinter;
    PRINTER_INFO_2A pi2;
    PRINTER_INFO_2A pi2Saved;

    ZeroMemory(&pi2, sizeof(pi2));
    pi2.pPrinterName = szPrinterName;
    pi2.pShareName = szShareName;
    pi2.pPortName = szPortName;
    pi2.pDriverName = szDriverName;
    pi2.pComment = szComment;
    pi2.pLocation = szLocation;
    pi2.pSepFile = szSepFile;
    pi2.pPrintProcessor = szPrintProcessor;
    pi2.pDatatype = szDatatype;
    pi2.pParameters = szParameters;

    CopyMemory(&pi2Saved, &pi2, sizeof(pi2));

    // This is expected to fail, because "ReactOS Test Driver" is not installed.
    // What it must not do is touch our structure.
    SetLastError(0xDEADBEEF);
    hPrinter = AddPrinterA(NULL, 2, (PBYTE)&pi2);

    if (hPrinter)
    {
        DeletePrinter(hPrinter);
        ClosePrinter(hPrinter);
    }

    ok(pi2.pServerName == pi2Saved.pServerName, "pServerName was changed to %p!\n", pi2.pServerName);
    ok(pi2.pPrinterName == pi2Saved.pPrinterName, "pPrinterName was changed to %p!\n", pi2.pPrinterName);
    ok(pi2.pShareName == pi2Saved.pShareName, "pShareName was changed to %p!\n", pi2.pShareName);
    ok(pi2.pPortName == pi2Saved.pPortName, "pPortName was changed to %p!\n", pi2.pPortName);
    ok(pi2.pDriverName == pi2Saved.pDriverName, "pDriverName was changed to %p!\n", pi2.pDriverName);
    ok(pi2.pComment == pi2Saved.pComment, "pComment was changed to %p!\n", pi2.pComment);
    ok(pi2.pLocation == pi2Saved.pLocation, "pLocation was changed to %p!\n", pi2.pLocation);
    ok(pi2.pDevMode == pi2Saved.pDevMode, "pDevMode was changed to %p!\n", pi2.pDevMode);
    ok(pi2.pSepFile == pi2Saved.pSepFile, "pSepFile was changed to %p!\n", pi2.pSepFile);
    ok(pi2.pPrintProcessor == pi2Saved.pPrintProcessor, "pPrintProcessor was changed to %p!\n", pi2.pPrintProcessor);
    ok(pi2.pDatatype == pi2Saved.pDatatype, "pDatatype was changed to %p!\n", pi2.pDatatype);
    ok(pi2.pParameters == pi2Saved.pParameters, "pParameters was changed to %p!\n", pi2.pParameters);

    // The strings themselves have to be intact as well.
    ok(!strcmp(szPrinterName, TEST_PRINTER_NAME), "szPrinterName is \"%s\"!\n", szPrinterName);
    ok(!strcmp(szPortName, "FILE:"), "szPortName is \"%s\"!\n", szPortName);
    ok(!strcmp(szDriverName, "ReactOS Test Driver"), "szDriverName is \"%s\"!\n", szDriverName);
    ok(!strcmp(szPrintProcessor, "WinPrint"), "szPrintProcessor is \"%s\"!\n", szPrintProcessor);
}

static void
Test_AddPrinter_InvalidParameters(void)
{
    PRINTER_INFO_2W pi2;

    ZeroMemory(&pi2, sizeof(pi2));

    SetLastError(0xDEADBEEF);
    ok(!AddPrinterW(NULL, 1, (PBYTE)&pi2), "AddPrinterW succeeded for level 1!\n");
    ok(GetLastError() == ERROR_INVALID_LEVEL, "AddPrinterW returns error %lu!\n", GetLastError());

    SetLastError(0xDEADBEEF);
    ok(!AddPrinterW(NULL, 2, NULL), "AddPrinterW succeeded for a NULL structure!\n");

    SetLastError(0xDEADBEEF);
    ok(!DeletePrinter(NULL), "DeletePrinter succeeded for a NULL handle!\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "DeletePrinter returns error %lu!\n", GetLastError());
}

#define TEST_DRIVER_NAMEW   L"ReactOS AddPrinter Test Driver"

/**
 * Installs a throwaway printer driver, then runs the sequence a printer setup
 * performs: AddPrinterDriverEx, AddPrinter, EnumPrinters, SetPrinter and
 * DeletePrinter (CORE-19134, CORE-20750).
 */
static void
Test_AddPrinter_Cycle(void)
{
    BOOL bFound;
    DRIVER_INFO_3W di3;
    DWORD cbNeeded;
    DWORD dwReturned;
    DWORD i;
    HANDLE hPrinter = NULL;
    PPRINTER_INFO_2W pPrinterInfo = NULL;
    PRINTER_INFO_2W pi2;
    WCHAR wszConfigFile[MAX_PATH];
    WCHAR wszDataFile[MAX_PATH];
    WCHAR wszDriverPath[MAX_PATH];
    WCHAR wszSystemDir[MAX_PATH];

    if (!GetSystemDirectoryW(wszSystemDir, _countof(wszSystemDir)))
    {
        skip("GetSystemDirectoryW failed with error %lu!\n", GetLastError());
        return;
    }

    // Any existing DLL will do; the driver is never actually used for printing here.
    StringCchPrintfW(wszDriverPath, _countof(wszDriverPath), L"%s\\localspl.dll", wszSystemDir);
    StringCchPrintfW(wszDataFile, _countof(wszDataFile), L"%s\\localspl.dll", wszSystemDir);
    StringCchPrintfW(wszConfigFile, _countof(wszConfigFile), L"%s\\printui.dll", wszSystemDir);

    ZeroMemory(&di3, sizeof(di3));
    di3.cVersion = 3;
    di3.pName = TEST_DRIVER_NAMEW;
    di3.pEnvironment = NULL;
    di3.pDriverPath = wszDriverPath;
    di3.pDataFile = wszDataFile;
    di3.pConfigFile = wszConfigFile;
    di3.pDefaultDataType = L"RAW";

    SetLastError(0xDEADBEEF);
    if (!AddPrinterDriverExW(NULL, 3, (PBYTE)&di3, APD_COPY_ALL_FILES))
    {
        skip("AddPrinterDriverExW failed with error %lu!\n", GetLastError());
        return;
    }

    ZeroMemory(&pi2, sizeof(pi2));
    pi2.pPrinterName = TEST_PRINTER_NAMEW;
    pi2.pPortName = L"FILE:";
    pi2.pDriverName = TEST_DRIVER_NAMEW;
    pi2.pPrintProcessor = L"WinPrint";
    pi2.pDatatype = L"RAW";
    pi2.pComment = L"Added by winspool_apitest";
    pi2.pLocation = L"";
    pi2.pShareName = L"";
    pi2.pSepFile = L"";
    pi2.pParameters = L"";

    SetLastError(0xDEADBEEF);
    hPrinter = AddPrinterW(NULL, 2, (PBYTE)&pi2);
    ok(hPrinter != NULL, "AddPrinterW failed with error %lu!\n", GetLastError());

    if (!hPrinter)
        return;

    // The new printer has to show up in the enumeration.
    bFound = FALSE;
    cbNeeded = 0;
    dwReturned = 0;
    EnumPrintersW(PRINTER_ENUM_LOCAL, NULL, 2, NULL, 0, &cbNeeded, &dwReturned);

    if (cbNeeded)
    {
        pPrinterInfo = HeapAlloc(GetProcessHeap(), 0, cbNeeded);

        if (pPrinterInfo && EnumPrintersW(PRINTER_ENUM_LOCAL, NULL, 2, (PBYTE)pPrinterInfo, cbNeeded, &cbNeeded, &dwReturned))
        {
            for (i = 0; i < dwReturned; i++)
            {
                if (!wcscmp(pPrinterInfo[i].pPrinterName, TEST_PRINTER_NAMEW))
                {
                    bFound = TRUE;
                    break;
                }
            }
        }

        if (pPrinterInfo)
            HeapFree(GetProcessHeap(), 0, pPrinterInfo);
    }

    ok(bFound, "The added printer was not returned by EnumPrintersW!\n");

    // Changing the printer has to work as well.
    pi2.pComment = L"Changed by winspool_apitest";
    SetLastError(0xDEADBEEF);
    ok(SetPrinterW(hPrinter, 2, (PBYTE)&pi2, 0), "SetPrinterW failed with error %lu!\n", GetLastError());

    SetLastError(0xDEADBEEF);
    ok(SetPrinterW(hPrinter, 0, NULL, PRINTER_CONTROL_PAUSE), "SetPrinterW(PAUSE) failed with error %lu!\n", GetLastError());

    SetLastError(0xDEADBEEF);
    ok(SetPrinterW(hPrinter, 0, NULL, PRINTER_CONTROL_RESUME), "SetPrinterW(RESUME) failed with error %lu!\n", GetLastError());

    // Now delete it again.
    SetLastError(0xDEADBEEF);
    ok(DeletePrinter(hPrinter), "DeletePrinter failed with error %lu!\n", GetLastError());
    ClosePrinter(hPrinter);

    // ...and it must be gone.
    bFound = FALSE;
    cbNeeded = 0;
    dwReturned = 0;
    EnumPrintersW(PRINTER_ENUM_LOCAL, NULL, 2, NULL, 0, &cbNeeded, &dwReturned);

    if (cbNeeded)
    {
        pPrinterInfo = HeapAlloc(GetProcessHeap(), 0, cbNeeded);

        if (pPrinterInfo && EnumPrintersW(PRINTER_ENUM_LOCAL, NULL, 2, (PBYTE)pPrinterInfo, cbNeeded, &cbNeeded, &dwReturned))
        {
            for (i = 0; i < dwReturned; i++)
            {
                if (!wcscmp(pPrinterInfo[i].pPrinterName, TEST_PRINTER_NAMEW))
                {
                    bFound = TRUE;
                    break;
                }
            }
        }

        if (pPrinterInfo)
            HeapFree(GetProcessHeap(), 0, pPrinterInfo);
    }

    ok(!bFound, "The deleted printer is still returned by EnumPrintersW!\n");
}

START_TEST(AddPrinter)
{
    Test_AddPrinter_InvalidParameters();
    Test_AddPrinterA_DoesNotModifyCallersStructure();
    Test_AddPrinter_Cycle();
}
