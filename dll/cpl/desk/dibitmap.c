/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            lib/cpl/desk/dibitmap.c
 * PURPOSE:         DIB loading
 *
 * PROGRAMMERS:     Trevor McCort (lycan359@gmail.com)
 */

#include "desk.h"

PDIBITMAP
DibLoadImage(LPTSTR lpFilename)
{
    BOOL     bSuccess;
    DWORD    dwFileSize, dwHighSize, dwBytesRead;
    HANDLE   hFile;
    PDIBITMAP lpBitmap;

    hFile = CreateFile(lpFilename,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_FLAG_SEQUENTIAL_SCAN,
                       NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return NULL;

    dwFileSize = GetFileSize(hFile, &dwHighSize);

    if (dwHighSize)
    {
        CloseHandle(hFile);
        return NULL;
    }

    lpBitmap = HeapAlloc(GetProcessHeap(), 0, sizeof(DIBITMAP));
    if (lpBitmap == NULL)
        return NULL;

    lpBitmap->header = HeapAlloc(GetProcessHeap(), 0, dwFileSize);
    if (lpBitmap->header == NULL)
    {
        CloseHandle(hFile);
        return NULL;
    }

    bSuccess = ReadFile(hFile, lpBitmap->header, dwFileSize, &dwBytesRead, NULL);
    CloseHandle(hFile);

    if (!bSuccess ||
        (dwBytesRead != dwFileSize) ||
        (lpBitmap->header->bfType != * (WORD *) "BM") ||
        (lpBitmap->header->bfSize != dwFileSize))
    {
        HeapFree(GetProcessHeap(), 0, lpBitmap->header);
        return NULL;
    }

    lpBitmap->info = (BITMAPINFO *)(lpBitmap->header + 1);
    lpBitmap->bits = (BYTE *)lpBitmap->header + lpBitmap->header->bfOffBits;

    /* Get the DIB width and height */
    if (lpBitmap->info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        lpBitmap->width  = ((BITMAPCOREHEADER *)lpBitmap->info)->bcWidth;
        lpBitmap->height = ((BITMAPCOREHEADER *)lpBitmap->info)->bcHeight;
    }
    else
    {
        lpBitmap->width   =     lpBitmap->info->bmiHeader.biWidth;
        lpBitmap->height  = abs(lpBitmap->info->bmiHeader.biHeight);
    }

    return lpBitmap;
}


VOID
DibFreeImage(PDIBITMAP lpBitmap)
{
    if (lpBitmap == NULL)
        return;

    /* Free the header */
    if (lpBitmap->header != NULL)
        HeapFree(GetProcessHeap(), 0, lpBitmap->header);

    /* Free the bitmap structure */
    if (lpBitmap != NULL)
        HeapFree(GetProcessHeap(), 0, lpBitmap);
}
