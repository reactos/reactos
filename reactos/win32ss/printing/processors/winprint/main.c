/*
 * PROJECT:     ReactOS Standard Print Processor
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Main functions
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

PCWSTR pwszDatatypes[] = {
    L"RAW",
    0
};

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
EnumPrintProcessorDatatypesW(LPWSTR pName, LPWSTR pPrintProcessorName, DWORD Level, LPBYTE pDatatypes, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    DWORD cbDatatype;
    DWORD dwOffsets[_countof(pwszDatatypes)];
    PCWSTR* pCurrentDatatype;
    PDWORD pCurrentOffset = dwOffsets;

    // Sanity checks
    if (Level != 1 || !pcbNeeded || !pcReturned)
        return FALSE;

    // Count the required buffer size and the number of datatypes.
    *pcbNeeded = 0;
    *pcReturned = 0;

    for (pCurrentDatatype = pwszDatatypes; *pCurrentDatatype; pCurrentDatatype++)
    {
        cbDatatype = (wcslen(*pCurrentDatatype) + 1) * sizeof(WCHAR);
        *pcbNeeded += sizeof(DATATYPES_INFO_1W) + cbDatatype;

        // Also calculate the offset in the output buffer of the pointer to this datatype string.
        *pCurrentOffset = *pcReturned * sizeof(DATATYPES_INFO_1W) + FIELD_OFFSET(DATATYPES_INFO_1W, pName);

        *pcReturned++;
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
    PackStrings(pwszDatatypes, pDatatypes, dwOffsets, &pDatatypes[*pcbNeeded]);

    return TRUE;
}
