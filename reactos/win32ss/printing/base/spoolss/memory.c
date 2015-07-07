/*
 * PROJECT:     ReactOS Spooler Router
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions for allocating and freeing memory
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"


/**
 * @name AllocSplStr
 *
 * Allocates memory for a Unicode string and copies the input string into it.
 * Equivalent of wcsdup, but the returned buffer is allocated from the spooler heap and must be freed with DllFreeSplStr.
 *
 * @param pwszInput
 * The input string to copy
 *
 * @return
 * Pointer to the copied string or NULL if no memory could be allocated.
 */
PWSTR WINAPI
AllocSplStr(PCWSTR pwszInput)
{
    DWORD cbInput;
    PWSTR pwszOutput;

    // Sanity check
    if (!pwszInput)
        return NULL;

    // Get the length of the input string.
    cbInput = (wcslen(pwszInput) + 1) * sizeof(WCHAR);

    // Allocate it. We don't use DllAllocSplMem here, because it unnecessarily zeroes the memory.
    pwszOutput = HeapAlloc(hProcessHeap, 0, cbInput);
    if (!pwszOutput)
    {
        ERR("HeapAlloc failed with error %lu!\n", GetLastError());
        return NULL;
    }

    // Copy the string and return it.
    CopyMemory(pwszOutput, pwszInput, cbInput);
    return pwszOutput;
}

/**
 * @name DllAllocSplMem
 *
 * Allocate a block of zeroed memory.
 * Windows allocates from a separate spooler heap here while we just use the process heap.
 *
 * @param dwBytes
 * Number of bytes to allocate.
 *
 * @return
 * A pointer to the allocated memory or NULL in case of an error.
 * You have to free this memory using DllFreeSplMem.
 */
PVOID WINAPI
DllAllocSplMem(DWORD dwBytes)
{
    return HeapAlloc(hProcessHeap, HEAP_ZERO_MEMORY, dwBytes);
}

/**
 * @name DllFreeSplMem
 *
 * Frees the memory allocated with DllAllocSplMem.
 *
 * @param pMem
 * Pointer to the allocated memory.
 *
 * @return
 * TRUE in case of success, FALSE otherwise.
 */
BOOL WINAPI
DllFreeSplMem(PVOID pMem)
{
    return HeapFree(hProcessHeap, 0, pMem);
}

/**
 * @name DllFreeSplStr
 *
 * Frees the string allocated with AllocSplStr.
 *
 * @param pwszString
 * Pointer to the allocated string.
 *
 * @return
 * TRUE in case of success, FALSE otherwise.
 */
BOOL WINAPI
DllFreeSplStr(PWSTR pwszString)
{
    return HeapFree(hProcessHeap, 0, pwszString);
}

/**
 * @name ReallocSplMem
 *
 * Allocates a new block of memory and copies the contents of the old block into the new one.
 *
 * @param pOldMem
 * Pointer to the old block of memory.
 * If this parameter is NULL, ReallocSplMem behaves exactly like DllAllocSplMem.
 *
 * @param cbOld
 * Number of bytes to copy from the old block into the new one.
 *
 * @param cbNew
 * Number of bytes to allocate for the new block.
 *
 * @return
 * A pointer to the allocated new block or NULL in case of an error.
 * You have to free this memory using DllFreeSplMem.
 */
PVOID WINAPI
ReallocSplMem(PVOID pOldMem, DWORD cbOld, DWORD cbNew)
{
    PVOID pNewMem;

    // Always allocate the new block of memory.
    pNewMem = DllAllocSplMem(cbNew);
    if (!pNewMem)
    {
        ERR("DllAllocSplMem failed with error %lu!\n", GetLastError());
        return NULL;
    }

    // Copy the old memory into the new block and free it.
    if (pOldMem)
    {
        CopyMemory(pNewMem, pOldMem, min(cbOld, cbNew));
        DllFreeSplMem(pOldMem);
    }

    return pNewMem;
}

/**
 * @name ReallocSplStr
 *
 * Frees a string allocated by AllocSplStr and copies the given Unicode string into a newly allocated block of memory.
 *
 * @param ppwszString
 * Pointer to the string pointer allocated by AllocSplStr.
 * When the function returns, the variable receives the pointer to the copied string.
 *
 * @param pwszInput
 * The Unicode string to copy into the new block of memory.
 *
 * @return
 * Returns TRUE in any case.
*/
BOOL WINAPI
ReallocSplStr(PWSTR* ppwszString, PCWSTR pwszInput)
{
    if (*ppwszString)
        DllFreeSplStr(*ppwszString);

    *ppwszString = AllocSplStr(pwszInput);
    
    return TRUE;
}
