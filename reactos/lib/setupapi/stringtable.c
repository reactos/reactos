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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "setupapi.h"

#include "wine/debug.h"


#define TABLE_DEFAULT_SIZE 256

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

typedef struct _STRING_TABLE
{
    LPWSTR *pSlots;
    DWORD dwUsedSlots;
    DWORD dwMaxSlots;
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

    pStringTable->pSlots = MyMalloc(sizeof(LPWSTR) * TABLE_DEFAULT_SIZE);
    if (pStringTable->pSlots == NULL)
    {
        MyFree(pStringTable->pSlots);
        return NULL;
    }

    memset(pStringTable->pSlots, 0, sizeof(LPWSTR) * TABLE_DEFAULT_SIZE);

    pStringTable->dwUsedSlots = 0;
    pStringTable->dwMaxSlots = TABLE_DEFAULT_SIZE;

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

    TRACE("%p\n", (PVOID)hStringTable);

    pStringTable = (PSTRING_TABLE)hStringTable;
    if (pStringTable == NULL)
        return;

    if (pStringTable->pSlots != NULL)
    {
        for (i = 0; i < pStringTable->dwMaxSlots; i++)
        {
            if (pStringTable->pSlots[i] != NULL)
            {
                MyFree(pStringTable->pSlots[i]);
                pStringTable->pSlots[i] = NULL;
            }
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
        if (pStringTable->pSlots[i] != NULL)
        {
            if (dwFlags & 1)
            {
                if (!lstrcmpW(pStringTable->pSlots[i], lpString))
                {
                    return i;
                }
            }
            else
            {
                if (!lstrcmpiW(pStringTable->pSlots[i], lpString))
                {
                    return i;
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
        if (pStringTable->pSlots[i] == NULL)
        {
            pStringTable->pSlots[i] = MyMalloc((lstrlenW(lpString) + 1) * sizeof(WCHAR));
            if (pStringTable->pSlots[i] == NULL)
            {
                TRACE("Couldn't allocate memory for a new string!\n");
                return (DWORD)-1;
            }

            lstrcpyW(pStringTable->pSlots[i], lpString);

            pStringTable->dwUsedSlots++;

            return i;
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

    TRACE("%p\n", (PVOID)hStringTable);

    pSourceTable = (PSTRING_TABLE)hStringTable;
    if (pSourceTable == NULL)
    {
        ERR("Invalid hStringTable!\n");
        return (HSTRING_TABLE)NULL;
    }

    pDestinationTable = MyMalloc(sizeof(STRING_TABLE));
    if (pDestinationTable == NULL)
    {
        ERR("Cound not allocate a new string table!\n");
        return (HSTRING_TABLE)NULL;
    }

    memset(pDestinationTable, 0, sizeof(STRING_TABLE));

    pDestinationTable->pSlots = MyMalloc(sizeof(LPWSTR) * pSourceTable->dwMaxSlots);
    if (pDestinationTable->pSlots == NULL)
    {
        MyFree(pDestinationTable);
        return (HSTRING_TABLE)NULL;
    }

    memset(pDestinationTable->pSlots, 0, sizeof(LPWSTR) * pSourceTable->dwMaxSlots);

    pDestinationTable->dwUsedSlots = 0;
    pDestinationTable->dwMaxSlots = pSourceTable->dwMaxSlots;

    for (i = 0; i < pSourceTable->dwMaxSlots; i++)
    {
        if (pSourceTable->pSlots[i] != NULL)
        {
            length = (lstrlenW(pSourceTable->pSlots[i]) + 1) * sizeof(WCHAR);
            pDestinationTable->pSlots[i] = MyMalloc(length);
            if (pDestinationTable->pSlots[i] != NULL)
            {
                memcpy(pDestinationTable->pSlots[i],
                       pSourceTable->pSlots[i],
                       length);
                pDestinationTable->dwUsedSlots++;
            }
        }
    }

    return (HSTRING_TABLE)pDestinationTable;
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
        if (pStringTable->pSlots[i] != NULL)
        {
            if (dwFlags & 1)
            {
                if (!lstrcmpW(pStringTable->pSlots[i], lpString))
                {
                    return i;
                }
            }
            else
            {
                if (!lstrcmpiW(pStringTable->pSlots[i], lpString))
                {
                    return i;
                }
            }
        }
    }

    return (DWORD)-1;
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

    TRACE("%p %lx\n", (PVOID)hStringTable, dwId);

    pStringTable = (PSTRING_TABLE)hStringTable;
    if (pStringTable == NULL)
    {
        ERR("Invalid hStringTable!\n");
        return NULL;
    }

    if (dwId >= pStringTable->dwMaxSlots)
        return NULL;

    return pStringTable->pSlots[dwId];
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

    TRACE("%p %lx %p %p\n",
          (PVOID)hStringTable, dwId, lpBuffer, lpBufferLength);

    pStringTable = (PSTRING_TABLE)hStringTable;
    if (pStringTable == NULL)
    {
        ERR("Invalid hStringTable!\n");
        *lpBufferLength = 0;
        return FALSE;
    }

    if (dwId >= pStringTable->dwMaxSlots ||
        pStringTable->pSlots[dwId] == NULL)
    {
        WARN("Invalid string ID!\n");
        *lpBufferLength = 0;
        return FALSE;
    }

    dwLength = (lstrlenW(pStringTable->pSlots[dwId]) + 1) * sizeof(WCHAR);
    if (dwLength <= *lpBufferLength)
    {
        lstrcpyW(lpBuffer, pStringTable->pSlots[dwId]);
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
    FIXME("%p\n", (PVOID)hStringTable);
}
