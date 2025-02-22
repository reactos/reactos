/*
 * PROJECT:     ReactOS Standard Print Processor
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Main functions
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

// Local Constants
static PCWSTR _pwszDatatypes[] = {
    L"RAW",
    0
};


/**
 * @name ClosePrintProcessor
 *
 * Closes a Print Processor Handle that has previously been opened through OpenPrintProcessor.
 *
 * @param hPrintProcessor
 * The return value of a previous successful OpenPrintProcessor call.
 *
 * @return
 * TRUE if the Print Processor Handle was successfully closed, FALSE otherwise.
 * A more specific error code can be obtained through GetLastError.
 */
BOOL WINAPI
ClosePrintProcessor(HANDLE hPrintProcessor)
{
    DWORD dwErrorCode;
    PWINPRINT_HANDLE pHandle;

    TRACE("ClosePrintProcessor(%p)\n", hPrintProcessor);

    // Sanity checks
    if (!hPrintProcessor)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    pHandle = (PWINPRINT_HANDLE)hPrintProcessor;

    // Free all structure fields for which memory has been allocated.
    if (pHandle->pwszDatatype)
        DllFreeSplStr(pHandle->pwszDatatype);

    if (pHandle->pwszDocumentName)
        DllFreeSplStr(pHandle->pwszDocumentName);

    if (pHandle->pwszOutputFile)
        DllFreeSplStr(pHandle->pwszOutputFile);

    if (pHandle->pwszPrinterPort)
        DllFreeSplStr(pHandle->pwszPrinterPort);

    // Finally free the WINSPOOL_HANDLE structure itself.
    DllFreeSplMem(pHandle);
    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
ControlPrintProcessor(HANDLE hPrintProcessor, DWORD Command)
{
    TRACE("ControlPrintProcessor(%p, %lu)\n", hPrintProcessor, Command);

    UNIMPLEMENTED;
    return FALSE;
}

/**
 * @name EnumPrintProcessorDatatypesW
 *
 * Obtains an array of all datatypes supported by this Print Processor.
 *
 * @param pName
 * Server Name. Ignored here, because every caller of EnumPrintProcessorDatatypesW is interested in this Print Processor's information.
 *
 * @param pPrintProcessorName
 * Print Processor Name. Ignored here, because every caller of EnumPrintProcessorDatatypesW is interested in this Print Processor's information.
 *
 * @param Level
 * The level of the structure supplied through pDatatypes. This must be 1.
 *
 * @param pDatatypes
 * Pointer to the buffer that receives an array of DATATYPES_INFO_1W structures.
 * Can be NULL if you just want to know the required size of the buffer.
 *
 * @param cbBuf
 * Size of the buffer you supplied for pDatatypes, in bytes.
 *
 * @param pcbNeeded
 * Pointer to a variable that receives the required size of the buffer for pDatatypes, in bytes.
 * This parameter mustn't be NULL!
 *
 * @param pcReturned
 * Pointer to a variable that receives the number of elements of the DATATYPES_INFO_1W array.
 * This parameter mustn't be NULL!
 *
 * @return
 * TRUE if we successfully copied the array into pDatatypes, FALSE otherwise.
 * A more specific error code can be obtained through GetLastError.
 */
BOOL WINAPI
EnumPrintProcessorDatatypesW(PWSTR pName, PWSTR pPrintProcessorName, DWORD Level, PBYTE pDatatypes, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    DWORD cbDatatype;
    DWORD dwDatatypeCount = 0;
    DWORD dwOffsets[_countof(_pwszDatatypes)];
    PCWSTR* pCurrentDatatype;
    PDWORD pCurrentOffset = dwOffsets;

    TRACE("EnumPrintProcessorDatatypesW(%S, %S, %lu, %p, %lu, %p, %p)\n", pName, pPrintProcessorName, Level, pDatatypes, cbBuf, pcbNeeded, pcReturned);

    // Sanity checks
    if (Level != 1 || !pcbNeeded || !pcReturned)
        return FALSE;

    // Count the required buffer size and the number of datatypes.
    *pcbNeeded = 0;
    *pcReturned = 0;

    for (pCurrentDatatype = _pwszDatatypes; *pCurrentDatatype; pCurrentDatatype++)
    {
        cbDatatype = (wcslen(*pCurrentDatatype) + 1) * sizeof(WCHAR);
        *pcbNeeded += sizeof(DATATYPES_INFO_1W) + cbDatatype;

        // Also calculate the offset in the output buffer of the pointer to this datatype string.
        *pCurrentOffset = dwDatatypeCount * sizeof(DATATYPES_INFO_1W) + FIELD_OFFSET(DATATYPES_INFO_1W, pName);

        dwDatatypeCount++;
        pCurrentOffset++;
    }

    // Check if the supplied buffer is large enough.
    if (cbBuf < *pcbNeeded)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    // Check if a buffer was supplied at all.
    if (!pDatatypes)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Copy over all datatypes.
    *pCurrentOffset = MAXDWORD;
    PackStrings(_pwszDatatypes, pDatatypes, dwOffsets, &pDatatypes[*pcbNeeded]);

    *pcReturned = dwDatatypeCount;
    return TRUE;
}


DWORD WINAPI
GetPrintProcessorCapabilities(PWSTR pValueName, DWORD dwAttributes, PBYTE pData, DWORD nSize, PDWORD pcbNeeded)
{
    TRACE("GetPrintProcessorCapabilities(%S, %lu, %p, %lu, %p)\n", pValueName, dwAttributes, pData, nSize, pcbNeeded);

    UNIMPLEMENTED;
    return 0;
}

/**
 * @name OpenPrintProcessor
 *
 * Prepares this Print Processor for processing a document.
 *
 * @param pPrinterName
 * String in the format "\\COMPUTERNAME\Port:, Port" that is passed to OpenPrinterW for writing to the Print Monitor on the specified port.
 *
 * @param pPrintProcessorOpenData
 * Pointer to a PRINTPROCESSOROPENDATA structure containing details about the print job to be processed.
 *
 * @return
 * A Print Processor handle on success or NULL in case of a failure. This handle has to be passed to PrintDocumentOnPrintProcessor to do the actual processing.
 * A more specific error code can be obtained through GetLastError.
 */
HANDLE WINAPI
OpenPrintProcessor(PWSTR pPrinterName, PPRINTPROCESSOROPENDATA pPrintProcessorOpenData)
{
    DWORD dwErrorCode;
    HANDLE hReturnValue = NULL;
    PWINPRINT_HANDLE pHandle = NULL;

    TRACE("OpenPrintProcessor(%S, %p)\n", pPrinterName, pPrintProcessorOpenData);

    // Sanity checks
    // This time a datatype needs to be given. We can't fall back to a default here.
    if (!pPrintProcessorOpenData || !pPrintProcessorOpenData->pDatatype || !*pPrintProcessorOpenData->pDatatype)
    {
        dwErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    // Create a new WINPRINT_HANDLE structure and fill the relevant fields.
    pHandle = DllAllocSplMem(sizeof(WINPRINT_HANDLE));

    // Check what datatype was given.
    if (_wcsicmp(pPrintProcessorOpenData->pDatatype, L"RAW") == 0)
    {
        pHandle->Datatype = RAW;
    }
    else
    {
        dwErrorCode = ERROR_INVALID_DATATYPE;
        goto Cleanup;
    }

    // Fill the relevant fields.
    pHandle->dwJobID = pPrintProcessorOpenData->JobId;
    pHandle->pwszDatatype = AllocSplStr(pPrintProcessorOpenData->pDatatype);
    pHandle->pwszDocumentName = AllocSplStr(pPrintProcessorOpenData->pDocumentName);
    pHandle->pwszOutputFile = AllocSplStr(pPrintProcessorOpenData->pOutputFile);
    pHandle->pwszPrinterPort = AllocSplStr(pPrinterName);

    // We were successful! Return the handle and don't let the cleanup routine free it.
    dwErrorCode = ERROR_SUCCESS;
    hReturnValue = pHandle;
    pHandle = NULL;

Cleanup:
    if (pHandle)
        DllFreeSplMem(pHandle);

    SetLastError(dwErrorCode);
    return hReturnValue;
}

/**
 * @name PrintDocumentOnPrintProcessor
 *
 * Prints a document on this Print Processor after a handle for the document has been opened through OpenPrintProcessor.
 *
 * @param hPrintProcessor
 * The return value of a previous successful OpenPrintProcessor call.
 *
 * @param pDocumentName
 * String in the format "Printer, Job N" describing the spooled job that is to be processed.
 *
 * @return
 * TRUE if the document was successfully processed by this Print Processor, FALSE otherwise.
 * A more specific error code can be obtained through GetLastError.
 */
BOOL WINAPI
PrintDocumentOnPrintProcessor(HANDLE hPrintProcessor, PWSTR pDocumentName)
{
    DWORD dwErrorCode;
    PWINPRINT_HANDLE pHandle;

    TRACE("PrintDocumentOnPrintProcessor(%p, %S)\n", hPrintProcessor, pDocumentName);

    // Sanity checks
    if (!hPrintProcessor)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    pHandle = (PWINPRINT_HANDLE)hPrintProcessor;

    // Call the corresponding Print function for the datatype.
    if (pHandle->Datatype == RAW)
        dwErrorCode = PrintRawJob(pHandle, pDocumentName);

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}
