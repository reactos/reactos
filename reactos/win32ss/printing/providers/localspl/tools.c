/*
 * PROJECT:     ReactOS Local Spooler
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Various tools
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

/**
 * @name AllocAndRegQueryWSZ
 *
 * Queries a REG_SZ value in the registry, allocates memory for it and returns a buffer containing the value.
 * You have to free this buffer using HeapFree.
 *
 * @param hKey
 * HKEY variable of the key opened with RegOpenKeyExW.
 *
 * @param pwszValueName
 * Name of the REG_SZ value to query.
 *
 * @return
 * Pointer to the buffer containing the value or NULL in case of failure.
 */
PWSTR
AllocAndRegQueryWSZ(HKEY hKey, PCWSTR pwszValueName)
{
    DWORD cbNeeded;
    LONG lStatus;
    PWSTR pwszValue;

    // Determine the size of the required buffer.
    lStatus = RegQueryValueExW(hKey, pwszValueName, NULL, NULL, NULL, &cbNeeded);
    if (lStatus != ERROR_SUCCESS)
    {
        ERR("RegQueryValueExW failed with status %ld!\n", lStatus);
        return NULL;
    }

    // Allocate it.
    pwszValue = HeapAlloc(hProcessHeap, 0, cbNeeded);
    if (!pwszValue)
    {
        ERR("HeapAlloc failed with error %lu!\n", GetLastError());
        return NULL;
    }

    // Now get the actual value.
    lStatus = RegQueryValueExW(hKey, pwszValueName, NULL, NULL, (PBYTE)pwszValue, &cbNeeded);
    if (lStatus != ERROR_SUCCESS)
    {
        ERR("RegQueryValueExW failed with status %ld!\n", lStatus);
        HeapFree(hProcessHeap, 0, pwszValue);
        return NULL;
    }

    return pwszValue;
}

/**
 * @name DuplicateStringW
 *
 * Allocates memory for a Unicode string and copies the input string into it.
 * Equivalent of wcsdup, but the returned buffer must be freed with HeapFree instead of free.
 *
 * @param pwszInput
 * The input string to copy
 *
 * @return
 * Pointer to the copied string or NULL if no memory could be allocated.
 */
PWSTR
DuplicateStringW(PCWSTR pwszInput)
{
    DWORD cchInput;
    PWSTR pwszOutput;

    cchInput = wcslen(pwszInput);

    pwszOutput = HeapAlloc(hProcessHeap, 0, (cchInput + 1) * sizeof(WCHAR));
    if (!pwszOutput)
    {
        ERR("HeapAlloc failed with error %lu!\n", GetLastError());
        return NULL;
    }

    CopyMemory(pwszOutput, pwszInput, (cchInput + 1) * sizeof(WCHAR));

    return pwszOutput;
}

/**
 * @name GenericTableAllocateRoutine
 *
 * RTL_GENERIC_ALLOCATE_ROUTINE for all our RTL_GENERIC_TABLEs, using HeapAlloc.
 */
PVOID NTAPI
GenericTableAllocateRoutine(PRTL_GENERIC_TABLE Table, CLONG ByteSize)
{
    return HeapAlloc(hProcessHeap, 0, ByteSize);
}

/**
 * @name GenericTableFreeRoutine
 *
 * RTL_GENERIC_FREE_ROUTINE for all our RTL_GENERIC_TABLEs, using HeapFree.
 */
VOID NTAPI
GenericTableFreeRoutine(PRTL_GENERIC_TABLE Table, PVOID Buffer)
{
    HeapFree(hProcessHeap, 0, Buffer);
}
