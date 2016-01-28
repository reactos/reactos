/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Clipboard Viewer
 * FILE:            base/applications/clipbrd/fileutils.c
 * PURPOSE:         Clipboard file format helper functions.
 * PROGRAMMERS:     Ricardo Hanke
 */

#include "precomp.h"

static HGLOBAL ClipboardReadMemoryBlock(HANDLE hFile, DWORD dwOffset, DWORD dwLength)
{
    HGLOBAL hData;
    LPVOID lpData;
    DWORD dwBytesRead;

    hData = GlobalAlloc(GHND, dwLength);
    if (!hData)
    {
        return NULL;
    }

    lpData = GlobalLock(hData);
    if (!lpData)
    {
        GlobalFree(hData);
        return NULL;
    }

    if (SetFilePointer(hFile, dwOffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        GlobalUnlock(hData);
        GlobalFree(hData);
        return NULL;
    }

    if (!ReadFile(hFile, lpData, dwLength, &dwBytesRead, NULL))
    {
        GlobalUnlock(hData);
        GlobalFree(hData);
        return NULL;
    }

    GlobalUnlock(hData);

    return hData;
}

static BOOL ClipboardReadMemory(HANDLE hFile, DWORD dwFormat, DWORD dwOffset, DWORD dwLength, LPCWSTR lpFormatName)
{
    HGLOBAL hData;
    DWORD dwTemp;

    hData = ClipboardReadMemoryBlock(hFile, dwOffset, dwLength);
    if (!hData)
    {
        return FALSE;
    }

    if ((dwFormat >= 0xC000) && (dwFormat <= 0xFFFF))
    {
        dwTemp = RegisterClipboardFormatW(lpFormatName);
        if (!dwTemp)
        {
            GlobalFree(hData);
            return FALSE;
        }
    }
    else
    {
        dwTemp = dwFormat;
    }

    if (!SetClipboardData(dwTemp, hData))
    {
        GlobalFree(hData);
        return FALSE;
    }

    return TRUE;
}

static BOOL ClipboardReadPalette(HANDLE hFile, DWORD dwOffset, DWORD dwLength)
{
    LPLOGPALETTE lpPalette;
    HPALETTE hPalette;
    HGLOBAL hData;

    hData = ClipboardReadMemoryBlock(hFile, dwOffset, dwLength);
    if (!hData)
    {
        return FALSE;
    }

    lpPalette = GlobalLock(hData);
    if (!lpPalette)
    {
        GlobalFree(hData);
        return FALSE;
    }

    hPalette = CreatePalette(lpPalette);
    if (!hPalette)
    {
        GlobalUnlock(hData);
        GlobalFree(hData);
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    GlobalUnlock(hData);
    GlobalFree(hData);

    if (!SetClipboardData(CF_PALETTE, hPalette))
    {
        DeleteObject(hPalette);
        return FALSE;
    }

    return TRUE;
}

static BOOL ClipboardReadEnhMetafile(HANDLE hFile, DWORD dwOffset, DWORD dwLength)
{
    HENHMETAFILE hEmf;
    HGLOBAL hData;
    LPVOID lpData;

    hData = ClipboardReadMemoryBlock(hFile, dwOffset, dwLength);
    if (!hData)
    {
        return FALSE;
    }

    lpData = GlobalLock(hData);
    if (!lpData)
    {
        GlobalFree(hData);
        return FALSE;
    }

    hEmf = SetEnhMetaFileBits(dwLength, lpData);

    GlobalUnlock(hData);
    GlobalFree(hData);

    if (!hEmf)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    if (!SetClipboardData(CF_ENHMETAFILE, hEmf))
    {
        DeleteEnhMetaFile(hEmf);
        return FALSE;
    }

    return TRUE;
}

static BOOL ClipboardReadBitmap(HANDLE hFile, DWORD dwOffset, DWORD dwLength)
{
    HGLOBAL hData;
    HBITMAP hBitmap;
    LPBITMAP lpBitmap;

    hData = ClipboardReadMemoryBlock(hFile, dwOffset, dwLength);
    if (!hData)
    {
        return FALSE;
    }

    lpBitmap = GlobalLock(hData);
    if (!lpBitmap)
    {
        GlobalFree(hData);
        return FALSE;
    }

    lpBitmap->bmBits = lpBitmap + sizeof(BITMAP) + 1;

    hBitmap = CreateBitmapIndirect(lpBitmap);

    GlobalUnlock(hData);
    GlobalFree(hData);

    if (!hBitmap)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    if (!SetClipboardData(CF_BITMAP, hBitmap))
    {
        DeleteObject(hBitmap);
        return FALSE;
    }

    return TRUE;
}

void ReadClipboardFile(LPCWSTR lpFileName)
{
    CLIPBOARDFILEHEADER cfhFileHeader;
    CLIPBOARDFORMATHEADER *cfhFormatArray = NULL;
    HANDLE hFile;
    DWORD dwBytesRead;
    BOOL bResult;
    int i;

    hFile = CreateFileW(lpFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        ShowLastWin32Error(Globals.hMainWnd);
        goto done;
    }

    if (!ReadFile(hFile, &cfhFileHeader, sizeof(cfhFileHeader), &dwBytesRead, NULL))
    {
        ShowLastWin32Error(Globals.hMainWnd);
        goto done;
    }

    if ((cfhFileHeader.wFileIdentifier != CLIPBOARD_FORMAT_NT) && (cfhFileHeader.wFileIdentifier != CLIPBOARD_FORMAT_BK))
    {
        MessageBoxRes(Globals.hMainWnd, Globals.hInstance, ERROR_INVALID_FILE_FORMAT, 0, MB_ICONSTOP | MB_OK);
        goto done;
    }

    cfhFormatArray = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cfhFileHeader.wFormatCount * sizeof(CLIPBOARDFORMATHEADER));
    if (!cfhFormatArray)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        ShowLastWin32Error(Globals.hMainWnd);
        goto done;
    }

    if (!ReadFile(hFile, cfhFormatArray, cfhFileHeader.wFormatCount * sizeof(CLIPBOARDFORMATHEADER), &dwBytesRead, NULL))
    {
        ShowLastWin32Error(Globals.hMainWnd);
        goto done;
    }

    for (i = 0; i < cfhFileHeader.wFormatCount; i++)
    {
        switch (cfhFormatArray[i].dwFormatID)
        {
            case CF_OWNERDISPLAY:
            case CF_DSPMETAFILEPICT:
            case CF_METAFILEPICT:
            {
                break;
            }

            case CF_BITMAP:
            case CF_DSPBITMAP:
            {
                bResult = ClipboardReadBitmap(hFile, cfhFormatArray[i].dwOffData, cfhFormatArray[i].dwLenData);
                break;
            }

            case CF_DSPENHMETAFILE:
            case CF_ENHMETAFILE:
            {
                bResult = ClipboardReadEnhMetafile(hFile, cfhFormatArray[i].dwOffData, cfhFormatArray[i].dwLenData);
                break;
            }

            case CF_PALETTE:
            {
                bResult = ClipboardReadPalette(hFile, cfhFormatArray[i].dwOffData, cfhFormatArray[i].dwLenData);
                break;
            }

            default:
            {
                if ((cfhFormatArray[i].dwFormatID < CF_PRIVATEFIRST) || (cfhFormatArray[i].dwFormatID > CF_PRIVATELAST))
                {
                    bResult = ClipboardReadMemory(hFile, cfhFormatArray[i].dwFormatID, cfhFormatArray[i].dwOffData, cfhFormatArray[i].dwLenData, cfhFormatArray[i].szName);
                }
                break;
            }
        }

        if (!bResult)
        {
            ShowLastWin32Error(Globals.hMainWnd);
        }
    }

done:
    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
    }

    if (cfhFormatArray)
    {
        HeapFree(GetProcessHeap(), 0, cfhFormatArray);
    }

    return;
}

void WriteClipboardFile(LPCWSTR lpFileName)
{
    MessageBoxW(Globals.hMainWnd, L"This function is currently not implemented.", L"Clipboard", MB_OK | MB_ICONINFORMATION);
    return;
}
