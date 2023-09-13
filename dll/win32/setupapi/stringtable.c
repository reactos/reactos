/*
 * Setupapi string table functions
 *
 * Copyright 2005 Eric Kohl
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "setupapi_private.h"


#define _PSETUP(func)   pSetup ## func

#define StringTableInitialize       _PSETUP(StringTableInitialize)
#define StringTableInitializeEx     _PSETUP(StringTableInitializeEx)
#define StringTableDestroy          _PSETUP(StringTableDestroy)
#define StringTableAddString        _PSETUP(StringTableAddString)
#define StringTableAddStringEx      _PSETUP(StringTableAddStringEx)
#define StringTableDuplicate        _PSETUP(StringTableDuplicate)
#define StringTableGetExtraData     _PSETUP(StringTableGetExtraData)
#define StringTableLookUpString     _PSETUP(StringTableLookUpString)
#define StringTableLookUpStringEx   _PSETUP(StringTableLookUpStringEx)
#define StringTableSetExtraData     _PSETUP(StringTableSetExtraData)
#define StringTableStringFromId     _PSETUP(StringTableStringFromId)
#define StringTableStringFromIdEx   _PSETUP(StringTableStringFromIdEx)
#define StringTableTrim             _PSETUP(StringTableTrim)


#define TABLE_DEFAULT_SIZE 256

typedef struct _TABLE_SLOT
{
    LPWSTR pString;
    LPVOID pData;
    DWORD dwSize;
} TABLE_SLOT, *PTABLE_SLOT;

typedef struct _STRING_TABLE
{
    PTABLE_SLOT pSlots;
    DWORD dwUsedSlots;
    DWORD dwMaxSlots;
    DWORD dwMaxDataSize;
} STRING_TABLE, *PSTRING_TABLE;


/**************************************************************************
 * StringTableInitialize [SETUPAPI.@]
 *
 * Creates a new string table and initializes it.
 *
 * PARAMS
 *     None
 *
 * RETURNS
 *     Success: Handle to the string table
 *     Failure: NULL
 */
HSTRING_TABLE WINAPI
StringTableInitialize(VOID)
{
    PSTRING_TABLE pStringTable;

    TRACE("\n");

    pStringTable = MyMalloc(sizeof(STRING_TABLE));
    if (pStringTable == NULL)
    {
        ERR("Invalid hStringTable!\n");
        return NULL;
    }

    memset(pStringTable, 0, sizeof(STRING_TABLE));

    pStringTable->pSlots = MyMalloc(sizeof(TABLE_SLOT) * TABLE_DEFAULT_SIZE);
    if (pStringTable->pSlots == NULL)
    {
        MyFree(pStringTable);
        return NULL;
    }

    memset(pStringTable->pSlots, 0, sizeof(TABLE_SLOT) * TABLE_DEFAULT_SIZE);

    pStringTable->dwUsedSlots = 0;
    pStringTable->dwMaxSlots = TABLE_DEFAULT_SIZE;
    pStringTable->dwMaxDataSize = 0;

    TRACE("Done\n");

    return (HSTRING_TABLE)pStringTable;
}

/**************************************************************************
 * StringTableInitializeEx [SETUPAPI.@]
 *
 * Creates a new string table and initializes it.
 *
 * PARAMS
 *     dwMaxExtraDataSize [I] Maximum extra data size
 *     dwReserved         [I] Unused
 *
 * RETURNS
 *     Success: Handle to the string table
 *     Failure: NULL
 */
HSTRING_TABLE WINAPI
StringTableInitializeEx(DWORD dwMaxExtraDataSize,
                        DWORD dwReserved)
{
    PSTRING_TABLE pStringTable;

    TRACE("\n");

    pStringTable = MyMalloc(sizeof(STRING_TABLE));
    if (pStringTable == NULL) return NULL;

    memset(pStringTable, 0, sizeof(STRING_TABLE));

    pStringTable->pSlots = MyMalloc(sizeof(TABLE_SLOT) * TABLE_DEFAULT_SIZE);
    if (pStringTable->pSlots == NULL)
    {
        MyFree(pStringTable);
        return NULL;
    }

    memset(pStringTable->pSlots, 0, sizeof(TABLE_SLOT) * TABLE_DEFAULT_SIZE);

    pStringTable->dwUsedSlots = 0;
    pStringTable->dwMaxSlots = TABLE_DEFAULT_SIZE;
    pStringTable->dwMaxDataSize = dwMaxExtraDataSize;

    TRACE("Done\n");

    return (HSTRING_TABLE)pStringTable;
}

/**************************************************************************
 * StringTableDestroy [SETUPAPI.@]
 *
 * Destroys a string table.
 *
 * PARAMS
 *     hStringTable [I] Handle to the string table to be destroyed
 *
 * RETURNS
 *     None
 */
VOID WINAPI
StringTableDestroy(HSTRING_TABLE hStringTable)
{
    PSTRING_TABLE pStringTable;
    DWORD i;

    TRACE("%p\n", hStringTable);

    pStringTable = (PSTRING_TABLE)hStringTable;
    if (pStringTable == NULL)
        return;

    if (pStringTable->pSlots != NULL)
    {
        for (i = 0; i < pStringTable->dwMaxSlots; i++)
        {
            MyFree(pStringTable->pSlots[i].pString);
            pStringTable->pSlots[i].pString = NULL;

            MyFree(pStringTable->pSlots[i].pData);
            pStringTable->pSlots[i].pData = NULL;
            pStringTable->pSlots[i].dwSize = 0;
        }

        MyFree(pStringTable->pSlots);
    }

    MyFree(pStringTable);
}

/**************************************************************************
 * StringTableAddString [SETUPAPI.@]
 *
 * Adds a new string to the string table.
 *
 * PARAMS
 *     hStringTable [I] Handle to the string table
 *     lpString     [I] String to be added to the string table
 *     dwFlags      [I] Flags
 *                        1: case sensitive compare
 *
 * RETURNS
 *     Success: String ID
 *     Failure: -1
 *
 * NOTES
 *     If the given string already exists in the string table it will not
 *     be added again. The ID of the existing string will be returned in
 *     this case.
 */
DWORD WINAPI
StringTableAddString(HSTRING_TABLE hStringTable,
                     LPWSTR lpString,
                     DWORD dwFlags)
{
    PSTRING_TABLE pStringTable;
    DWORD i;

    TRACE("%p %s %x\n", hStringTable, debugstr_w(lpString), dwFlags);

    pStringTable = (PSTRING_TABLE)hStringTable;
    if (pStringTable == NULL)
    {
        ERR("Invalid hStringTable!\n");
        return (DWORD)-1;
    }

    /* Search for existing string in the string table */
    for (i = 0; i < pStringTable->dwMaxSlots; i++)
    {
        if (pStringTable->pSlots[i].pString != NULL)
        {
            if (dwFlags & 1)
            {
                if (!lstrcmpW(pStringTable->pSlots[i].pString, lpString))
                {
                    return i + 1;
                }
            }
            else
            {
                if (!lstrcmpiW(pStringTable->pSlots[i].pString, lpString))
                {
                    return i + 1;
                }
            }
        }
    }

    /* Check for filled slot table */
    if (pStringTable->dwUsedSlots == pStringTable->dwMaxSlots)
    {
        PTABLE_SLOT pNewSlots;
        DWORD dwNewMaxSlots;

        /* FIXME: not thread safe */
        dwNewMaxSlots = pStringTable->dwMaxSlots * 2;
        pNewSlots = MyMalloc(sizeof(TABLE_SLOT) * dwNewMaxSlots);
        if (pNewSlots == NULL)
            return (DWORD)-1;
        memset(&pNewSlots[pStringTable->dwMaxSlots], 0, sizeof(TABLE_SLOT) * (dwNewMaxSlots - pStringTable->dwMaxSlots));
        memcpy(pNewSlots, pStringTable->pSlots, sizeof(TABLE_SLOT) * pStringTable->dwMaxSlots);
        pNewSlots = InterlockedExchangePointer((PVOID*)&pStringTable->pSlots, pNewSlots);
        MyFree(pNewSlots);
        pStringTable->dwMaxSlots = dwNewMaxSlots;

        return StringTableAddString(hStringTable, lpString, dwFlags);
    }

    /* Search for an empty slot */
    for (i = 0; i < pStringTable->dwMaxSlots; i++)
    {
        if (pStringTable->pSlots[i].pString == NULL)
        {
            pStringTable->pSlots[i].pString = MyMalloc((lstrlenW(lpString) + 1) * sizeof(WCHAR));
            if (pStringTable->pSlots[i].pString == NULL)
            {
                TRACE("Couldn't allocate memory for a new string!\n");
                return (DWORD)-1;
            }

            lstrcpyW(pStringTable->pSlots[i].pString, lpString);

            pStringTable->dwUsedSlots++;

            return i + 1;
        }
    }

    TRACE("Couldn't find an empty slot!\n");

    return (DWORD)-1;
}

/**************************************************************************
 * StringTableAddStringEx [SETUPAPI.@]
 *
 * Adds a new string plus extra data to the string table.
 *
 * PARAMS
 *     hStringTable    [I] Handle to the string table
 *     lpString        [I] String to be added to the string table
 *     dwFlags         [I] Flags
 *                           1: case sensitive compare
 *     lpExtraData     [I] Pointer to the extra data
 *     dwExtraDataSize [I] Size of the extra data
 *
 * RETURNS
 *     Success: String ID
 *     Failure: -1
 *
 * NOTES
 *     If the given string already exists in the string table it will not
 *     be added again. The ID of the existing string will be returned in
 *     this case.
 */
DWORD WINAPI
StringTableAddStringEx(HSTRING_TABLE hStringTable,
                       LPWSTR lpString,
                       DWORD dwFlags,
                       LPVOID lpExtraData,
                       DWORD dwExtraDataSize)
{
    PSTRING_TABLE pStringTable;
    DWORD i;

    TRACE("%p %s %lx\n", (PVOID)hStringTable, debugstr_w(lpString), dwFlags);

    pStringTable = (PSTRING_TABLE)hStringTable;
    if (pStringTable == NULL)
    {
        ERR("Invalid hStringTable!\n");
        return (DWORD)-1;
    }

    /* Search for existing string in the string table */
    for (i = 0; i < pStringTable->dwMaxSlots; i++)
    {
        if (pStringTable->pSlots[i].pString != NULL)
        {
            if (dwFlags & 1)
            {
                if (!lstrcmpW(pStringTable->pSlots[i].pString, lpString))
                {
                    return i + 1;
                }
            }
            else
            {
                if (!lstrcmpiW(pStringTable->pSlots[i].pString, lpString))
                {
                    return i + 1;
                }
            }
        }
    }

    /* Check for filled slot table */
    if (pStringTable->dwUsedSlots == pStringTable->dwMaxSlots)
    {
        FIXME("Resize the string table!\n");
        return (DWORD)-1;
    }

    /* Search for an empty slot */
    for (i = 0; i < pStringTable->dwMaxSlots; i++)
    {
        if (pStringTable->pSlots[i].pString == NULL)
        {
            pStringTable->pSlots[i].pString = MyMalloc((lstrlenW(lpString) + 1) * sizeof(WCHAR));
            if (pStringTable->pSlots[i].pString == NULL)
            {
                TRACE("Couldn't allocate memory for a new string!\n");
                return (DWORD)-1;
            }

            lstrcpyW(pStringTable->pSlots[i].pString, lpString);

            pStringTable->pSlots[i].pData = MyMalloc(dwExtraDataSize);
            if (pStringTable->pSlots[i].pData == NULL)
            {
                TRACE("Couldn't allocate memory for a new extra data!\n");
                MyFree(pStringTable->pSlots[i].pString);
                pStringTable->pSlots[i].pString = NULL;
                return (DWORD)-1;
            }

            memcpy(pStringTable->pSlots[i].pData,
                   lpExtraData,
                   dwExtraDataSize);
            pStringTable->pSlots[i].dwSize = dwExtraDataSize;

            pStringTable->dwUsedSlots++;

            return i + 1;
        }
    }

    TRACE("Couldn't find an empty slot!\n");

    return (DWORD)-1;
}

/**************************************************************************
 * StringTableDuplicate [SETUPAPI.@]
 *
 * Duplicates a given string table.
 *
 * PARAMS
 *     hStringTable [I] Handle to the string table
 *
 * RETURNS
 *     Success: Handle to the duplicated string table
 *     Failure: NULL
 *
 */
HSTRING_TABLE WINAPI
StringTableDuplicate(HSTRING_TABLE hStringTable)
{
    PSTRING_TABLE pSourceTable;
    PSTRING_TABLE pDestinationTable;
    DWORD i;
    DWORD length;

    TRACE("%p\n", hStringTable);

    pSourceTable = (PSTRING_TABLE)hStringTable;
    if (pSourceTable == NULL)
    {
        ERR("Invalid hStringTable!\n");
        return (HSTRING_TABLE)NULL;
    }

    pDestinationTable = MyMalloc(sizeof(STRING_TABLE));
    if (pDestinationTable == NULL)
    {
        ERR("Could not allocate a new string table!\n");
        return (HSTRING_TABLE)NULL;
    }

    memset(pDestinationTable, 0, sizeof(STRING_TABLE));

    pDestinationTable->pSlots = MyMalloc(sizeof(TABLE_SLOT) * pSourceTable->dwMaxSlots);
    if (pDestinationTable->pSlots == NULL)
    {
        MyFree(pDestinationTable);
        return (HSTRING_TABLE)NULL;
    }

    memset(pDestinationTable->pSlots, 0, sizeof(TABLE_SLOT) * pSourceTable->dwMaxSlots);

    pDestinationTable->dwUsedSlots = 0;
    pDestinationTable->dwMaxSlots = pSourceTable->dwMaxSlots;

    for (i = 0; i < pSourceTable->dwMaxSlots; i++)
    {
        if (pSourceTable->pSlots[i].pString != NULL)
        {
            length = (lstrlenW(pSourceTable->pSlots[i].pString) + 1) * sizeof(WCHAR);
            pDestinationTable->pSlots[i].pString = MyMalloc(length);
            if (pDestinationTable->pSlots[i].pString != NULL)
            {
                memcpy(pDestinationTable->pSlots[i].pString,
                       pSourceTable->pSlots[i].pString,
                       length);
                pDestinationTable->dwUsedSlots++;
            }

            if (pSourceTable->pSlots[i].pData != NULL)
            {
                length = pSourceTable->pSlots[i].dwSize;
                pDestinationTable->pSlots[i].pData = MyMalloc(length);
                if (pDestinationTable->pSlots[i].pData)
                {
                    memcpy(pDestinationTable->pSlots[i].pData,
                           pSourceTable->pSlots[i].pData,
                           length);
                    pDestinationTable->pSlots[i].dwSize = length;
                }
            }
        }
    }

    return (HSTRING_TABLE)pDestinationTable;
}

/**************************************************************************
 * StringTableGetExtraData [SETUPAPI.@]
 *
 * Retrieves extra data from a given string table entry.
 *
 * PARAMS
 *     hStringTable    [I] Handle to the string table
 *     dwId            [I] String ID
 *     lpExtraData     [I] Pointer a buffer that receives the extra data
 *     dwExtraDataSize [I] Size of the buffer
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI
StringTableGetExtraData(HSTRING_TABLE hStringTable,
                        DWORD dwId,
                        LPVOID lpExtraData,
                        DWORD dwExtraDataSize)
{
    PSTRING_TABLE pStringTable;

    TRACE("%p %x %p %u\n",
          hStringTable, dwId, lpExtraData, dwExtraDataSize);

    pStringTable = (PSTRING_TABLE)hStringTable;
    if (pStringTable == NULL)
    {
        ERR("Invalid hStringTable!\n");
        return FALSE;
    }

    if (dwId == 0 || dwId > pStringTable->dwMaxSlots)
    {
        ERR("Invalid Slot id!\n");
        return FALSE;
    }

    if (pStringTable->pSlots[dwId - 1].dwSize < dwExtraDataSize)
    {
        ERR("Data size is too large!\n");
        return FALSE;
    }

    memcpy(lpExtraData,
           pStringTable->pSlots[dwId - 1].pData,
           dwExtraDataSize);

    return TRUE;
}

/**************************************************************************
 * StringTableLookUpString [SETUPAPI.@]
 *
 * Searches a string table for a given string.
 *
 * PARAMS
 *     hStringTable [I] Handle to the string table
 *     lpString     [I] String to be searched for
 *     dwFlags      [I] Flags
 *                        1: case sensitive compare
 *
 * RETURNS
 *     Success: String ID
 *     Failure: -1
 */
DWORD WINAPI
StringTableLookUpString(HSTRING_TABLE hStringTable,
                        LPWSTR lpString,
                        DWORD dwFlags)
{
    PSTRING_TABLE pStringTable;
    DWORD i;

    TRACE("%p %s %x\n", hStringTable, debugstr_w(lpString), dwFlags);

    pStringTable = (PSTRING_TABLE)hStringTable;
    if (pStringTable == NULL)
    {
        ERR("Invalid hStringTable!\n");
        return (DWORD)-1;
    }

    /* Search for existing string in the string table */
    for (i = 0; i < pStringTable->dwMaxSlots; i++)
    {
        if (pStringTable->pSlots[i].pString != NULL)
        {
            if (dwFlags & 1)
            {
                if (!lstrcmpW(pStringTable->pSlots[i].pString, lpString))
                    return i + 1;
            }
            else
            {
                if (!lstrcmpiW(pStringTable->pSlots[i].pString, lpString))
                    return i + 1;
            }
        }
    }

    return (DWORD)-1;
}

/**************************************************************************
 * StringTableLookUpStringEx [SETUPAPI.@]
 *
 * Searches a string table and extra data for a given string.
 *
 * PARAMS
 *     hStringTable [I] Handle to the string table
 *     lpString     [I] String to be searched for
 *     dwFlags      [I] Flags
 *                        1: case sensitive compare
 *     lpExtraData  [O] Pointer to the buffer that receives the extra data
 *     lpReserved   [I/O] Unused
 *
 * RETURNS
 *     Success: String ID
 *     Failure: -1
 */
DWORD WINAPI
StringTableLookUpStringEx(HSTRING_TABLE hStringTable,
                          LPWSTR lpString,
                          DWORD dwFlags,
                          LPVOID lpExtraData,
                          DWORD dwReserved)
{
    PSTRING_TABLE pStringTable;
    DWORD i;

    TRACE("%p %s %x %p, %x\n", hStringTable, debugstr_w(lpString), dwFlags,
          lpExtraData, dwReserved);

    pStringTable = (PSTRING_TABLE)hStringTable;
    if (pStringTable == NULL)
    {
        ERR("Invalid hStringTable!\n");
        return ~0u;
    }

    /* Search for existing string in the string table */
    for (i = 0; i < pStringTable->dwMaxSlots; i++)
    {
        if (pStringTable->pSlots[i].pString != NULL)
        {
            if (dwFlags & 1)
            {
                if (!lstrcmpW(pStringTable->pSlots[i].pString, lpString))
                {
                    if (lpExtraData)
                        memcpy(lpExtraData, pStringTable->pSlots[i].pData, dwReserved);
                    return i + 1;
                }
            }
            else
            {
                if (!lstrcmpiW(pStringTable->pSlots[i].pString, lpString))
                {
                    if (lpExtraData)
                        memcpy(lpExtraData, pStringTable->pSlots[i].pData, dwReserved);
                    return i + 1;
                }
            }
        }
    }
    return ~0u;
}

/**************************************************************************
 * StringTableSetExtraData [SETUPAPI.@]
 *
 * Sets extra data for a given string table entry.
 *
 * PARAMS
 *     hStringTable    [I] Handle to the string table
 *     dwId            [I] String ID
 *     lpExtraData     [I] Pointer to the extra data
 *     dwExtraDataSize [I] Size of the extra data
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI
StringTableSetExtraData(HSTRING_TABLE hStringTable,
                        DWORD dwId,
                        LPVOID lpExtraData,
                        DWORD dwExtraDataSize)
{
    PSTRING_TABLE pStringTable;

    TRACE("%p %x %p %u\n",
          hStringTable, dwId, lpExtraData, dwExtraDataSize);

    pStringTable = (PSTRING_TABLE)hStringTable;
    if (pStringTable == NULL)
    {
        ERR("Invalid hStringTable!\n");
        return FALSE;
    }

    if (dwId == 0 || dwId > pStringTable->dwMaxSlots)
    {
        ERR("Invalid Slot id!\n");
        return FALSE;
    }

    if (pStringTable->dwMaxDataSize < dwExtraDataSize)
    {
        ERR("Data size is too large!\n");
        return FALSE;
    }

    pStringTable->pSlots[dwId - 1].pData = MyMalloc(dwExtraDataSize);
    if (pStringTable->pSlots[dwId - 1].pData == NULL)
    {
        ERR("\n");
        return FALSE;
    }

    memcpy(pStringTable->pSlots[dwId - 1].pData,
           lpExtraData,
           dwExtraDataSize);
    pStringTable->pSlots[dwId - 1].dwSize = dwExtraDataSize;

    return TRUE;
}

/**************************************************************************
 * StringTableStringFromId [SETUPAPI.@]
 *
 * Returns a pointer to a string for the given string ID.
 *
 * PARAMS
 *     hStringTable [I] Handle to the string table.
 *     dwId         [I] String ID
 *
 * RETURNS
 *     Success: Pointer to the string
 *     Failure: NULL
 */
LPWSTR WINAPI
StringTableStringFromId(HSTRING_TABLE hStringTable,
                        DWORD dwId)
{
    PSTRING_TABLE pStringTable;
    static WCHAR empty[] = {0};

    TRACE("%p %x\n", hStringTable, dwId);

    pStringTable = (PSTRING_TABLE)hStringTable;
    if (pStringTable == NULL)
    {
        ERR("Invalid hStringTable!\n");
        return NULL;
    }

    if (dwId == 0 || dwId > pStringTable->dwMaxSlots)
        return empty;

    return pStringTable->pSlots[dwId - 1].pString;
}

/**************************************************************************
 * StringTableStringFromIdEx [SETUPAPI.@]
 *
 * Returns a string for the given string ID.
 *
 * PARAMS
 *     hStringTable [I] Handle to the string table
 *     dwId         [I] String ID
 *     lpBuffer     [I] Pointer to string buffer
 *     lpBufferSize [I/O] Pointer to the size of the string buffer
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */
BOOL WINAPI
StringTableStringFromIdEx(HSTRING_TABLE hStringTable,
                          DWORD dwId,
                          LPWSTR lpBuffer,
                          LPDWORD lpBufferLength)
{
    PSTRING_TABLE pStringTable;
    DWORD dwLength;
    BOOL bResult = FALSE;

    TRACE("%p %x %p %p\n", hStringTable, dwId, lpBuffer, lpBufferLength);

    pStringTable = (PSTRING_TABLE)hStringTable;
    if (pStringTable == NULL)
    {
        ERR("Invalid hStringTable!\n");
        *lpBufferLength = 0;
        return FALSE;
    }

    if (dwId == 0 || dwId > pStringTable->dwMaxSlots ||
        pStringTable->pSlots[dwId - 1].pString == NULL)
    {
        WARN("Invalid string ID!\n");
        *lpBufferLength = 0;
        return FALSE;
    }

    dwLength = (lstrlenW(pStringTable->pSlots[dwId - 1].pString) + 1);
    if (dwLength <= *lpBufferLength)
    {
        lstrcpyW(lpBuffer, pStringTable->pSlots[dwId - 1].pString);
        bResult = TRUE;
    }

    *lpBufferLength = dwLength;

    return bResult;
}

/**************************************************************************
 * StringTableTrim [SETUPAPI.@]
 *
 * ...
 *
 * PARAMS
 *     hStringTable [I] Handle to the string table
 *
 * RETURNS
 *     None
 */
VOID WINAPI
StringTableTrim(HSTRING_TABLE hStringTable)
{
    FIXME("%p\n", hStringTable);
}
