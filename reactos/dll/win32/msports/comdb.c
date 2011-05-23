/*
 * PROJECT:     Ports installer library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll\win32\msports\comdb.c
 * PURPOSE:     COM port database
 * COPYRIGHT:   Copyright 2011 Eric Kohl
 */

#include <windows.h>
#include <msports.h>

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(msports);

#define BITS_PER_BYTE 8

typedef struct _COMDB
{
    HKEY hKey;
    DWORD dwSize;
    PBYTE pBitmap;
    PBYTE pData;
} COMDB, *PCOMDB;


LONG
WINAPI
ComDBClaimPort(IN HCOMDB hComDB,
               IN DWORD ComNumber,
               IN BOOL ForceClaim,
               OUT PBOOL Forced)
{
    PCOMDB pComDB;
    PBYTE pByte;
    BYTE cMask;
    DWORD dwBitIndex;
    DWORD dwType;
    DWORD dwSize;
    LONG lError;

    if (hComDB == INVALID_HANDLE_VALUE ||
        hComDB == NULL ||
        ComNumber == 0 ||
        ComNumber > COMDB_MAX_PORTS_ARBITRATED)
        return ERROR_INVALID_PARAMETER;

    pComDB = (PCOMDB)hComDB;

    /* Update the bitmap */
    dwSize = pComDB->dwSize;
    lError = RegQueryValueExW(pComDB->hKey,
                              L"ComDB",
                              NULL,
                              &dwType,
                              pComDB->pBitmap,
                              &dwSize);
    if (lError != ERROR_SUCCESS)
        return lError;

    /* Get the bit index */
    dwBitIndex = ComNumber - 1;

    /* Check if the bit to set fits into the bitmap */
    if (dwBitIndex >= (pComDB->dwSize * BITS_PER_BYTE))
    {
        /* FIXME: Resize the bitmap */
        return ERROR_INVALID_PARAMETER;
    }

    /* Get a pointer to the affected byte and calculate a mask for the affected bit */
    pByte = &(pComDB->pBitmap[dwBitIndex / BITS_PER_BYTE]);
    cMask = 1 << (dwBitIndex % BITS_PER_BYTE);

    /* Check if the bit is not set */
    if ((*pByte & cMask) == 0)
    {
        /* Set the bit */
        *pByte |= cMask;
        lError = ERROR_SUCCESS;
    }
    else
    {
        /* The bit is already set */
        lError = ERROR_SHARING_VIOLATION;
    }

    /* Save the bitmap if it was modified */
    if (lError == ERROR_SUCCESS)
    {
        lError = RegSetValueExW(pComDB->hKey,
                                L"ComDB",
                                0,
                                REG_BINARY,
                                pComDB->pData,
                                pComDB->dwSize);
    }

    return lError;
}


LONG
WINAPI
ComDBClose(IN HCOMDB hComDB)
{
    PCOMDB pComDB;

    if (hComDB == HCOMDB_INVALID_HANDLE_VALUE || hComDB == NULL)
        return ERROR_INVALID_PARAMETER;

    pComDB = (PCOMDB)hComDB;

    /* Close the registry key */
    if (pComDB->hKey != NULL)
        RegCloseKey(pComDB->hKey);

    /* Release the bitmap */
    if (pComDB->pBitmap != NULL)
        HeapFree(GetProcessHeap(), 0, pComDB->pBitmap);

    /* Release the database */
    HeapFree(GetProcessHeap(), 0, pComDB);

    return ERROR_SUCCESS;
}


LONG
WINAPI
ComDBOpen(OUT HCOMDB *phComDB)
{
    PCOMDB pComDB;
    DWORD dwDisposition;
    DWORD dwType;
    LONG lError;

    TRACE("ComDBOpen(%p)\n", phComDB);

    /* Allocate a new database */
    pComDB = HeapAlloc(GetProcessHeap(),
                       HEAP_ZERO_MEMORY,
                       sizeof(COMDB));
    if (pComDB == NULL)
    {
        ERR("Failed to allocaete the database!\n");
        return ERROR_ACCESS_DENIED;
    }

    /* Create or open the database key */
    lError = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                             L"System\\CurrentControlSet\\Control\\COM Name Arbiter",
                             0,
                             NULL,
                             0,
                             KEY_ALL_ACCESS,
                             NULL,
                             &pComDB->hKey,
                             &dwDisposition);
    if (lError != ERROR_SUCCESS)
        goto done;

    /* Get the required bitmap size */
    lError = RegQueryValueExW(pComDB->hKey,
                              L"ComDB",
                              NULL,
                              &dwType,
                              NULL,
                              &pComDB->dwSize);
    if (lError == ERROR_FILE_NOT_FOUND)
    {
        /* Allocate a new bitmap */
        pComDB->dwSize = COMDB_MIN_PORTS_ARBITRATED / BITS_PER_BYTE;
        pComDB->pData = HeapAlloc(GetProcessHeap(),
                                  HEAP_ZERO_MEMORY,
                                  pComDB->dwSize);
        if (pComDB->pData == NULL)
        {
            ERR("Failed to allocaete the bitmap!\n");
            lError = ERROR_ACCESS_DENIED;
            goto done;
        }

        /* Read the bitmap from the registry */
        lError = RegSetValueExW(pComDB->hKey,
                                L"ComDB",
                                0,
                                REG_BINARY,
                                pComDB->pData,
                                pComDB->dwSize);
    }

done:;
    if (lError != ERROR_SUCCESS)
    {
        /* Clean up in case of failure */
        if (pComDB->hKey != NULL)
            RegCloseKey(pComDB->hKey);

        if (pComDB->pData != NULL)
            HeapFree(GetProcessHeap(), 0, pComDB->pData);

        HeapFree(GetProcessHeap(), 0, pComDB);

        *phComDB = HCOMDB_INVALID_HANDLE_VALUE;
    }
    else
    {
        /* Return the database handle */
        *phComDB = (HCOMDB)pComDB;
    }

    TRACE("done (Error %lu)\n", lError);

    return lError;
}

/* EOF */
