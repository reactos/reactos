/*
 * PROJECT:     ReactOS Spooler Router
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Miscellaneous tool functions
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"


/**
 * @name PackStrings
 *
 * Takes an array of Unicode strings and fills an output buffer with these strings at the end and pointers to each string at specific offsets.
 * Useful helper for functions that copy an information structure including strings into a given buffer (like PRINTER_INFO_1).
 *
 * @param pSource
 * The array of Unicode strings to copy. Needs to have at least as many elements as the DestOffsets array.
 *
 * @param pDest
 * Pointer to the beginning of the output buffer.
 * The caller is responsible for verifying that this buffer is large enough to hold all strings and pointers.
 *
 * @param DestOffsets
 * Array of byte offsets in the output buffer. For each element of DestOffsets, the function will copy the address of the corresponding copied
 * string of pSource to this location in the output buffer. If a string in pSource is NULL, the function will set the pointer address to NULL
 * in the output buffer.
 * Use macros like FIELD_OFFSET to calculate the offsets for this array.
 * The last element of the array must have the value MAXDWORD to let the function detect the end of the array.
 *
 * @param pEnd
 * Pointer to the end of the output buffer. That means the first element outside of the buffer given in pDest.
 *
 * @return
 * Returns a pointer to the beginning of the strings in pDest.
 * The strings are copied in reverse order, so this pointer will point to the last copied string of pSource.
 */
PBYTE WINAPI
PackStrings(PCWSTR* pSource, PBYTE pDest, const DWORD* DestOffsets, PBYTE pEnd)
{
    DWORD cbString;
    ULONG_PTR StringAddress;

    // Loop until we reach an element with offset set to MAXDWORD.
    while (*DestOffsets != MAXDWORD)
    {
        StringAddress = 0;

        if (*pSource)
        {
            // Determine the length of the source string.
            cbString = (wcslen(*pSource) + 1) * sizeof(WCHAR);

            // Copy it before the last string.
            pEnd -= cbString;
            StringAddress = (ULONG_PTR)pEnd;
            CopyMemory(pEnd, *pSource, cbString);
        }

        // Copy the address of the copied string to the location given by the offset.
        CopyMemory(&pDest[*DestOffsets], &StringAddress, sizeof(ULONG_PTR));

        // Advance to the next source string and destination offset.
        pSource++;
        DestOffsets++;
    }

    // pEnd is now at the last string we copied. Return this value as a pointer to the beginning of all strings in the output buffer.
    return pEnd;
}
