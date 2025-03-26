/*
 * PROJECT:     ReactOS Standard Print Processor
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Printing a job with RAW datatype
 * COPYRIGHT:   Copyright 2015 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

/**
 * @name PrintRawJob
 *
 * @param pHandle
 * Pointer to a WINPRINT_HANDLE structure containing information about this job.
 *
 * @param pwszPrinterAndJob
 * String in the format "Printer, Job N" that is passed to OpenPrinterW to read from the spooled print job.
 *
 * @return
 * An error code indicating success or failure.
 */
DWORD
PrintRawJob(PWINPRINT_HANDLE pHandle, PWSTR pwszPrinterAndJob)
{
    // Use a read buffer of 256 KB size like Windows does.
    const DWORD cbReadBuffer = 262144;

    BOOL bStartedDoc = FALSE;
    DOC_INFO_1W DocInfo1;
    DWORD cbRead;
    DWORD cbWritten;
    DWORD dwErrorCode;
    HANDLE hPrintJob;
    HANDLE hPrintMonitor = NULL;
    PBYTE pBuffer = NULL;

    // Open the spooled job to read from it.
    if (!OpenPrinterW(pwszPrinterAndJob, &hPrintJob, NULL))
    {
        dwErrorCode = GetLastError();
        ERR("OpenPrinterW failed for \"%S\" with error %lu!\n", pwszPrinterAndJob, GetLastError());
        goto Cleanup;
    }

    // Open a Print Monitor handle to write to it.
    if (!OpenPrinterW(pHandle->pwszPrinterPort, &hPrintMonitor, NULL))
    {
        dwErrorCode = GetLastError();
        ERR("OpenPrinterW failed for \"%S\" with error %lu!\n", pHandle->pwszPrinterPort, GetLastError());
        goto Cleanup;
    }

    // Fill the Document Information.
    DocInfo1.pDatatype = pHandle->pwszDatatype;
    DocInfo1.pDocName = pHandle->pwszDocumentName;
    DocInfo1.pOutputFile = pHandle->pwszOutputFile;

    // Tell the Print Monitor that we're starting a new document.
    if (!StartDocPrinterW(hPrintMonitor, 1, (PBYTE)&DocInfo1))
    {
        dwErrorCode = GetLastError();
        ERR("StartDocPrinterW failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }

    bStartedDoc = TRUE;

    // Allocate a read buffer on the heap. This would easily exceed the stack size.
    pBuffer = DllAllocSplMem(cbReadBuffer);
    if (!pBuffer)
    {
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        ERR("DllAllocSplMem failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }

    // Loop as long as data is available.
    while (ReadPrinter(hPrintJob, pBuffer, cbReadBuffer, &cbRead) && cbRead)
    {
        // Write it to the Print Monitor.
        WritePrinter(hPrintMonitor, pBuffer, cbRead, &cbWritten);
    }

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    if (pBuffer)
        DllFreeSplMem(pBuffer);

    if (bStartedDoc)
        EndDocPrinter(hPrintMonitor);

    if (hPrintMonitor)
        ClosePrinter(hPrintMonitor);

    if (hPrintJob)
        ClosePrinter(hPrintJob);

    return dwErrorCode;
}
