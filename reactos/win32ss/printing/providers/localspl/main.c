/*
 * PROJECT:     ReactOS Local Spooler
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Main functions
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

// Global Variables
WCHAR wszSpoolDirectory[MAX_PATH];
DWORD cchSpoolDirectory;

// Global Constants
#include <prtprocenv.h>

const WCHAR wszDefaultDocumentName[] = L"Local Downlevel Document";

PWSTR wszPrintProviderInfo[3] = {
    L"Windows NT Local Print Providor",     // Name
    L"Locally connected Printers",          // Comment
    L"Windows NT Local Printers"            // Description
};

// Local Constants
static const PRINTPROVIDOR _PrintProviderFunctions = {
    LocalOpenPrinter,                           // fpOpenPrinter
    LocalSetJob,                                // fpSetJob
    LocalGetJob,                                // fpGetJob
    LocalEnumJobs,                              // fpEnumJobs
    NULL,                                       // fpAddPrinter
    NULL,                                       // fpDeletePrinter
    NULL,                                       // fpSetPrinter
    NULL,                                       // fpGetPrinter
    LocalEnumPrinters,                          // fpEnumPrinters
    NULL,                                       // fpAddPrinterDriver
    NULL,                                       // fpEnumPrinterDrivers
    NULL,                                       // fpGetPrinterDriver
    NULL,                                       // fpGetPrinterDriverDirectory
    NULL,                                       // fpDeletePrinterDriver
    NULL,                                       // fpAddPrintProcessor
    LocalEnumPrintProcessors,                   // fpEnumPrintProcessors
    LocalGetPrintProcessorDirectory,            // fpGetPrintProcessorDirectory
    NULL,                                       // fpDeletePrintProcessor
    LocalEnumPrintProcessorDatatypes,           // fpEnumPrintProcessorDatatypes
    LocalStartDocPrinter,                       // fpStartDocPrinter
    LocalStartPagePrinter,                      // fpStartPagePrinter
    LocalWritePrinter,                          // fpWritePrinter
    LocalEndPagePrinter,                        // fpEndPagePrinter
    NULL,                                       // fpAbortPrinter
    LocalReadPrinter,                           // fpReadPrinter
    LocalEndDocPrinter,                         // fpEndDocPrinter
    LocalAddJob,                                // fpAddJob
    LocalScheduleJob,                           // fpScheduleJob
    NULL,                                       // fpGetPrinterData
    NULL,                                       // fpSetPrinterData
    NULL,                                       // fpWaitForPrinterChange
    LocalClosePrinter,                          // fpClosePrinter
    NULL,                                       // fpAddForm
    NULL,                                       // fpDeleteForm
    NULL,                                       // fpGetForm
    NULL,                                       // fpSetForm
    NULL,                                       // fpEnumForms
    LocalEnumMonitors,                          // fpEnumMonitors
    LocalEnumPorts,                             // fpEnumPorts
    NULL,                                       // fpAddPort
    NULL,                                       // fpConfigurePort
    NULL,                                       // fpDeletePort
    NULL,                                       // fpCreatePrinterIC
    NULL,                                       // fpPlayGdiScriptOnPrinterIC
    NULL,                                       // fpDeletePrinterIC
    NULL,                                       // fpAddPrinterConnection
    NULL,                                       // fpDeletePrinterConnection
    NULL,                                       // fpPrinterMessageBox
    NULL,                                       // fpAddMonitor
    NULL,                                       // fpDeleteMonitor
    NULL,                                       // fpResetPrinter
    NULL,                                       // fpGetPrinterDriverEx
    NULL,                                       // fpFindFirstPrinterChangeNotification
    NULL,                                       // fpFindClosePrinterChangeNotification
    NULL,                                       // fpAddPortEx
    NULL,                                       // fpShutDown
    NULL,                                       // fpRefreshPrinterChangeNotification
    NULL,                                       // fpOpenPrinterEx
    NULL,                                       // fpAddPrinterEx
    NULL,                                       // fpSetPort
    NULL,                                       // fpEnumPrinterData
    NULL,                                       // fpDeletePrinterData
    NULL,                                       // fpClusterSplOpen
    NULL,                                       // fpClusterSplClose
    NULL,                                       // fpClusterSplIsAlive
    NULL,                                       // fpSetPrinterDataEx
    NULL,                                       // fpGetPrinterDataEx
    NULL,                                       // fpEnumPrinterDataEx
    NULL,                                       // fpEnumPrinterKey
    NULL,                                       // fpDeletePrinterDataEx
    NULL,                                       // fpDeletePrinterKey
    NULL,                                       // fpSeekPrinter
    NULL,                                       // fpDeletePrinterDriverEx
    NULL,                                       // fpAddPerMachineConnection
    NULL,                                       // fpDeletePerMachineConnection
    NULL,                                       // fpEnumPerMachineConnections
    NULL,                                       // fpXcvData
    NULL,                                       // fpAddPrinterDriverEx
    NULL,                                       // fpSplReadPrinter
    NULL,                                       // fpDriverUnloadComplete
    NULL,                                       // fpGetSpoolFileInfo
    NULL,                                       // fpCommitSpoolData
    NULL,                                       // fpCloseSpoolFileHandle
    NULL,                                       // fpFlushPrinter
    NULL,                                       // fpSendRecvBidiData
    NULL,                                       // fpAddDriverCatalog
};

static void
_GetSpoolDirectory()
{
    const WCHAR wszSpoolPath[] = L"\\spool";
    const DWORD cchSpoolPath = _countof(wszSpoolPath) - 1;

    // Get the system directory and append the "spool" subdirectory.
    // Forget about length checks here. If this doesn't fit into MAX_PATH, our OS has more serious problems...
    cchSpoolDirectory = GetSystemDirectoryW(wszSpoolDirectory, MAX_PATH);
    CopyMemory(&wszSpoolDirectory[cchSpoolDirectory], wszSpoolPath, (cchSpoolPath + 1) * sizeof(WCHAR));
    cchSpoolDirectory += cchSpoolPath;
}

BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            _GetSpoolDirectory();

            return InitializePrintMonitorList() &&
                   InitializePortList() &&
                   InitializePrintProcessorList() &&
                   InitializePrinterList() &&
                   InitializeGlobalJobList();

        default:
            return TRUE;
    }
}

BOOL WINAPI
InitializePrintProvidor(LPPRINTPROVIDOR pPrintProvidor, DWORD cbPrintProvidor, LPWSTR pFullRegistryPath)
{
    CopyMemory(pPrintProvidor, &_PrintProviderFunctions, min(cbPrintProvidor, sizeof(PRINTPROVIDOR)));

    SetLastError(ERROR_SUCCESS);
    return TRUE;
}
