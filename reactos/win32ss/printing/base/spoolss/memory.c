/*
 * PROJECT:     ReactOS Spooler Router
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions for allocating and freeing memory
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"


/**
 * @name AlignRpcPtr
 *
 * Checks if the input buffer and buffer size are 4-byte aligned.
 * If the buffer size is not 4-byte aligned, it is aligned down.
 * If the input buffer is not 4-byte aligned, a 4-byte aligned buffer of the aligned down buffer size is allocated and returned.
 *
 * @param pBuffer
 * The buffer to check.
 *
 * @param pcbBuffer
 * Pointer to the buffer size to check. Its value is aligned down if needed.
 *
 * @return
 * pBuffer if pBuffer is already 4-byte aligned, or a newly allocated 4-byte aligned buffer of the aligned down buffer size otherwise.
 * If a buffer was allocated, you have to free it using UndoAlignRpcPtr.
 */
PVOID WINAPI
AlignRpcPtr(PVOID pBuffer, PDWORD pcbBuffer)
{
    ASSERT(pcbBuffer);

    // Align down the buffer size in pcbBuffer to a 4-byte boundary.
    *pcbBuffer -= *pcbBuffer % sizeof(DWORD);

    // Check if pBuffer is 4-byte aligned. If not, allocate a 4-byte aligned buffer.
    if ((ULONG_PTR)pBuffer % sizeof(DWORD))
        pBuffer = DllAllocSplMem(*pcbBuffer);

    return pBuffer;
}

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

/**
 * @name UndoAlignRpcPtr
 *
 * Copies the data from the aligned buffer previously allocated by AlignRpcPtr back to the original unaligned buffer.
 * The aligned buffer is freed.
 *
 * Also aligns up the returned required buffer size of a function to a 4-byte boundary.
 *
 * @param pDestinationBuffer
 * The original unaligned buffer, which you input as pBuffer to AlignRpcPtr.
 * The data from pSourceBuffer is copied into this buffer before pSourceBuffer is freed.
 * If AlignRpcPtr did not allocate a buffer, pDestinationBuffer equals pSourceBuffer and no memory is copied or freed.
 * This parameter may be NULL if pSourceBuffer is NULL or cbBuffer is 0.
 *
 * @param pSourceBuffer
 * The aligned buffer, which is returned by AlignRpcPtr.
 * Its data is copied into pDestinationBuffer and then pSourceBuffer is freed.
 * If AlignRpcPtr did not allocate a buffer, pDestinationBuffer equals pSourceBuffer and no memory is copied or freed.
 * This parameter may be NULL.
 *
 * @param cbBuffer
 * Number of bytes to copy.
 * Set this to the size returned by AlignRpcPtr's pcbBuffer or less.
 *
 * @param pcbNeeded
 * Let this parameter point to your variable calculating the needed bytes for a buffer and returning this value to the user.
 * It is then aligned up to a 4-byte boundary, so that the user supplies a large enough buffer in the next call.
 * Otherwise, AlignRpcPtr would align down the buffer size in the next call and your buffer would be smaller than intended.
 * This parameter may be NULL.
 *
 * @return
 * pcbNeeded
 */
PDWORD WINAPI
UndoAlignRpcPtr(PVOID pDestinationBuffer, PVOID pSourceBuffer, DWORD cbBuffer, PDWORD pcbNeeded)
{
    // pDestinationBuffer is accessed unless pSourceBuffer equals pDestinationBuffer or cbBuffer is 0.
    ASSERT(pDestinationBuffer || pSourceBuffer == pDestinationBuffer || cbBuffer == 0);

    // If pSourceBuffer is given, and source and destination pointers don't match,
    // we assume that pSourceBuffer is the buffer allocated by AlignRpcPtr.
    if (pSourceBuffer && pSourceBuffer != pDestinationBuffer)
    {
        // Copy back the buffer data to the (usually unaligned) destination buffer
        // and free the buffer allocated by AlignRpcPtr.
        CopyMemory(pDestinationBuffer, pSourceBuffer, cbBuffer);
        DllFreeSplMem(pSourceBuffer);
    }

    // If pcbNeeded is given, align it up to a 4-byte boundary.
    if (pcbNeeded && *pcbNeeded % sizeof(DWORD))
        *pcbNeeded += sizeof(DWORD) - *pcbNeeded % sizeof(DWORD);

    return pcbNeeded;
}
