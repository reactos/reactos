/*
 * PROJECT:     ReactOS Printing Stack Marshalling Functions
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Marshalling functions
 * COPYRIGHT:   Copyright 2015-2018 Colin Finck (colin@reactos.org)
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>

#include <marshalling/marshalling.h>

/**
 * @name MarshallDownStructure
 *
 * Prepare a structure for marshalling/serialization by replacing absolute pointer addresses in its fields by relative offsets.
 *
 * @param pStructure
 * Pointer to the structure to operate on.
 *
 * @param pInfo
 * Array of MARSHALLING_INFO elements containing information about the fields of the structure as well as how to modify them.
 * See the documentation on MARSHALLING_INFO for more information.
 * You have to indicate the end of the array by setting the dwOffset field to MAXDWORD.
 *
 * @param cbStructureSize
 * Size in bytes of the structure.
 * This parameter is unused in my implementation.
 *
 * @param bSomeBoolean
 * Unknown boolean value, set to TRUE.
 *
 * @return
 * TRUE if the structure was successfully adjusted, FALSE otherwise.
 */
BOOL WINAPI
MarshallDownStructure(PVOID pStructure, const MARSHALLING_INFO* pInfo, DWORD cbStructureSize, BOOL bSomeBoolean)
{
    // Sanity checks
    if (!pStructure || !pInfo)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Loop until we reach an element with offset set to MAXDWORD.
    while (pInfo->dwOffset != MAXDWORD)
    {
        PULONG_PTR pCurrentField = (PULONG_PTR)((PBYTE)pStructure + pInfo->dwOffset);

        if (pInfo->bAdjustAddress && *pCurrentField)
        {
            // Make a relative offset out of the absolute pointer address.
            *pCurrentField -= (ULONG_PTR)pStructure;
        }

        // Advance to the next field description.
        pInfo++;
    }

    return TRUE;
}

/**
 * @name MarshallDownStructuresArray
 *
 * Prepare an array of structures for marshalling/serialization by replacing absolute pointer addresses in its fields by relative offsets.
 *
 * @param pStructuresArray
 * Pointer to the array of structures to operate on.
 *
 * @param cElements
 * Number of array elements.
 *
 * @param pInfo
 * Array of MARSHALLING_INFO elements containing information about the fields of the structure as well as how to modify them.
 * See the documentation on MARSHALLING_INFO for more information.
 * You have to indicate the end of the array by setting the dwOffset field to MAXDWORD.
 *
 * @param cbStructureSize
 * Size in bytes of each structure array element.
 *
 * @param bSomeBoolean
 * Unknown boolean value, set to TRUE.
 *
 * @return
 * TRUE if the array was successfully adjusted, FALSE otherwise.
 */
BOOL WINAPI
MarshallDownStructuresArray(PVOID pStructuresArray, DWORD cElements, const MARSHALLING_INFO* pInfo, DWORD cbStructureSize, BOOL bSomeBoolean)
{
    PBYTE pCurrentElement = pStructuresArray;

    // Call MarshallDownStructure on all array elements given by cElements of cbStructureSize.
    while (cElements--)
    {
        if (!MarshallDownStructure(pCurrentElement, pInfo, cbStructureSize, bSomeBoolean))
            return FALSE;

        // Advance to the next array element.
        pCurrentElement += cbStructureSize;
    }

    return TRUE;
}

/**
 * @name MarshallUpStructure
 *
 * Unmarshall/deserialize a structure previuosly marshalled by MarshallDownStructure by replacing relative offsets in its fields
 * by absolute pointer addresses again.
 *
 * @param cbSize
 * Size in bytes of the memory allocated for both the structure and its data.
 * The function will check if all relative offsets are within the bounds given by this size.
 *
 * @param pStructure
 * Pointer to the structure to operate on.
 *
 * @param pInfo
 * Array of MARSHALLING_INFO elements containing information about the fields of the structure as well as how to modify them.
 * See the documentation on MARSHALLING_INFO for more information.
 * You have to indicate the end of the array by setting the dwOffset field to MAXDWORD.
 *
 * @param cbStructureSize
 * Size in bytes of the structure.
 * This parameter is unused in my implementation.
 *
 * @param bSomeBoolean
 * Unknown boolean value, set to TRUE.
 *
 * @return
 * TRUE if the structure was successfully adjusted, FALSE otherwise.
 */
BOOL WINAPI
MarshallUpStructure(DWORD cbSize, PVOID pStructure, const MARSHALLING_INFO* pInfo, DWORD cbStructureSize, BOOL bSomeBoolean)
{
    // Sanity checks
    if (!pStructure || !pInfo)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Loop until we reach an element with offset set to MAXDWORD.
    while (pInfo->dwOffset != MAXDWORD)
    {
        PULONG_PTR pCurrentField = (PULONG_PTR)((PBYTE)pStructure + pInfo->dwOffset);

        if (pInfo->bAdjustAddress && *pCurrentField)
        {
            // Verify that the offset in the current field is within the bounds given by cbSize.
            if (cbSize <= *pCurrentField)
            {
                SetLastError(ERROR_INVALID_DATA);
                return FALSE;
            }

            // Make an absolute pointer address out of the relative offset.
            *pCurrentField += (ULONG_PTR)pStructure;
        }

        // Advance to the next field description.
        pInfo++;
    }

    return TRUE;
}

/**
 * @name MarshallUpStructuresArray
 *
 * Unmarshall/deserialize an array of structures previuosly marshalled by MarshallDownStructuresArray by replacing relative offsets
 * in its fields by absolute pointer addresses again.
 *
 * @param cbSize
 * Size in bytes of the memory allocated for the entire structure array and its data.
 * The function will check if all relative offsets are within the bounds given by this size.
 *
 * @param pStructuresArray
 * Pointer to the array of structures to operate on.
 *
 * @param cElements
 * Number of array elements.
 *
 * @param pInfo
 * Array of MARSHALLING_INFO elements containing information about the fields of the structure as well as how to modify them.
 * See the documentation on MARSHALLING_INFO for more information.
 * You have to indicate the end of the array by setting the dwOffset field to MAXDWORD.
 *
 * @param cbStructureSize
 * Size in bytes of each structure array element.
 *
 * @param bSomeBoolean
 * Unknown boolean value, set to TRUE.
 *
 * @return
 * TRUE if the array was successfully adjusted, FALSE otherwise.
 */
BOOL WINAPI
MarshallUpStructuresArray(DWORD cbSize, PVOID pStructuresArray, DWORD cElements, const MARSHALLING_INFO* pInfo, DWORD cbStructureSize, BOOL bSomeBoolean)
{
    PBYTE pCurrentElement = pStructuresArray;

    // Call MarshallUpStructure on all array elements given by cElements of cbStructureSize.
    while (cElements--)
    {
        if (!MarshallUpStructure(cbSize, pCurrentElement, pInfo, cbStructureSize, bSomeBoolean))
            return FALSE;

        // Advance to the next array element.
        pCurrentElement += cbStructureSize;
    }

    return TRUE;
}
