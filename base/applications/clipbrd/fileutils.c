/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Clipboard Viewer
 * FILE:            base/applications/clipbrd/fileutils.c
 * PURPOSE:         Clipboard file format helper functions.
 * PROGRAMMERS:     Ricardo Hanke
 *                  Hermes Belusca-Maito
 */

#include "precomp.h"

static HGLOBAL ClipboardReadMemoryBlock(HANDLE hFile, DWORD dwOffset, DWORD dwLength)
{
    HGLOBAL hData;
    LPVOID lpData;
    DWORD dwBytesRead;

    hData = GlobalAlloc(GHND, dwLength);
    if (!hData)
        return NULL;

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

static BOOL ClipboardReadMemory(HANDLE hFile, DWORD dwFormat, DWORD dwOffset, DWORD dwLength, WORD FileIdentifier, PVOID lpFormatName)
{
    HGLOBAL hData;
    DWORD dwTemp = 0;

    hData = ClipboardReadMemoryBlock(hFile, dwOffset, dwLength);
    if (!hData)
        return FALSE;

    if ((dwFormat >= 0xC000) && (dwFormat <= 0xFFFF))
    {
        if (FileIdentifier == CLIP_FMT_31)
            dwTemp = RegisterClipboardFormatA((LPCSTR)lpFormatName);
        else if ((FileIdentifier == CLIP_FMT_NT) || (FileIdentifier == CLIP_FMT_BK))
            dwTemp = RegisterClipboardFormatW((LPCWSTR)lpFormatName);

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

static BOOL ClipboardWriteMemory(HANDLE hFile, DWORD dwFormat, DWORD dwOffset, PDWORD pdwLength)
{
    HGLOBAL hData;
    LPVOID lpData;
    DWORD dwBytesWritten;

    hData = GetClipboardData(dwFormat);
    if (!hData)
        return FALSE;

    lpData = GlobalLock(hData);
    if (!lpData)
        return FALSE;

    *pdwLength = GlobalSize(hData);

    if (SetFilePointer(hFile, dwOffset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        GlobalUnlock(hData);
        return FALSE;
    }

    if (!WriteFile(hFile, lpData, *pdwLength, &dwBytesWritten, NULL))
    {
        GlobalUnlock(hData);
        return FALSE;
    }

    GlobalUnlock(hData);

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

static BOOL ClipboardReadMetafile(HANDLE hFile, DWORD dwOffset, DWORD dwLength)
{
    HMETAFILE hMf;
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

    hMf = SetMetaFileBitsEx(dwLength, lpData);

    GlobalUnlock(hData);
    GlobalFree(hData);

    if (!hMf)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    if (!SetClipboardData(CF_METAFILEPICT, hMf))
    {
        DeleteMetaFile(hMf);
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
    CLIPFILEHEADER ClipFileHeader;
    CLIPFORMATHEADER ClipFormatArray;
    NTCLIPFILEHEADER NtClipFileHeader;
    NTCLIPFORMATHEADER NtClipFormatArray;
    PVOID pClipFileHeader;
    PVOID pClipFormatArray;
    DWORD SizeOfFileHeader, SizeOfFormatHeader;

    WORD wFileIdentifier;
    WORD wFormatCount;
    DWORD dwFormatID;
    DWORD dwLenData;
    DWORD dwOffData;
    PVOID szName;

    HANDLE hFile;
    DWORD dwBytesRead;
    BOOL bResult;
    int i;

    /* Open the file for read access */
    hFile = CreateFileW(lpFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        ShowLastWin32Error(Globals.hMainWnd);
        goto done;
    }

    /* Just read enough bytes to get the clipboard file format ID */
    if (!ReadFile(hFile, &wFileIdentifier, sizeof(wFileIdentifier), &dwBytesRead, NULL))
    {
        ShowLastWin32Error(Globals.hMainWnd);
        goto done;
    }

    /* Set data according to the clipboard file format ID */
    switch (wFileIdentifier)
    {
        case CLIP_FMT_31:
            SizeOfFileHeader   = sizeof(CLIPFILEHEADER);
            SizeOfFormatHeader = sizeof(CLIPFORMATHEADER);
            pClipFileHeader    = &ClipFileHeader;
            pClipFormatArray   = &ClipFormatArray;
            break;

        case CLIP_FMT_NT:
        case CLIP_FMT_BK:
            SizeOfFileHeader   = sizeof(NTCLIPFILEHEADER);
            SizeOfFormatHeader = sizeof(NTCLIPFORMATHEADER);
            pClipFileHeader    = &NtClipFileHeader;
            pClipFormatArray   = &NtClipFormatArray;
            break;

        default:
            MessageBoxRes(Globals.hMainWnd, Globals.hInstance, ERROR_INVALID_FILE_FORMAT, 0, MB_ICONSTOP | MB_OK);
            goto done;
    }

    /* Completely read the header */
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    if (!ReadFile(hFile, pClipFileHeader, SizeOfFileHeader, &dwBytesRead, NULL) ||
        dwBytesRead != SizeOfFileHeader)
    {
        ShowLastWin32Error(Globals.hMainWnd);
        goto done;
    }

    /* Get header data */
    switch (wFileIdentifier)
    {
        case CLIP_FMT_31:
            assert(wFileIdentifier == ((CLIPFILEHEADER*)pClipFileHeader)->wFileIdentifier);
            wFormatCount = ((CLIPFILEHEADER*)pClipFileHeader)->wFormatCount;
            break;

        case CLIP_FMT_NT:
        case CLIP_FMT_BK:
            assert(wFileIdentifier == ((NTCLIPFILEHEADER*)pClipFileHeader)->wFileIdentifier);
            wFormatCount = ((NTCLIPFILEHEADER*)pClipFileHeader)->wFormatCount;
            break;
    }

    /* Loop through the format data array */
    for (i = 0; i < wFormatCount; i++)
    {
        if (SetFilePointer(hFile, SizeOfFileHeader + i * SizeOfFormatHeader, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
        {
            ShowLastWin32Error(Globals.hMainWnd);
            goto done;
        }

        if (!ReadFile(hFile, pClipFormatArray, SizeOfFormatHeader, &dwBytesRead, NULL))
        {
            ShowLastWin32Error(Globals.hMainWnd);
            goto done;
        }

        /* Get format data */
        switch (wFileIdentifier)
        {
            case CLIP_FMT_31:
                dwFormatID = ((CLIPFORMATHEADER*)pClipFormatArray)->dwFormatID;
                dwLenData  = ((CLIPFORMATHEADER*)pClipFormatArray)->dwLenData;
                dwOffData  = ((CLIPFORMATHEADER*)pClipFormatArray)->dwOffData;
                szName     = ((CLIPFORMATHEADER*)pClipFormatArray)->szName;
                break;

            case CLIP_FMT_NT:
            case CLIP_FMT_BK:
                dwFormatID = ((NTCLIPFORMATHEADER*)pClipFormatArray)->dwFormatID;
                dwLenData  = ((NTCLIPFORMATHEADER*)pClipFormatArray)->dwLenData;
                dwOffData  = ((NTCLIPFORMATHEADER*)pClipFormatArray)->dwOffData;
                szName     = ((NTCLIPFORMATHEADER*)pClipFormatArray)->szName;
                break;
        }

        switch (dwFormatID)
        {
            case CF_OWNERDISPLAY:
            {
                break;
            }

            case CF_BITMAP:
            case CF_DSPBITMAP:
            {
                bResult = ClipboardReadBitmap(hFile, dwOffData, dwLenData);
                break;
            }

            case CF_METAFILEPICT:
            case CF_DSPMETAFILEPICT:
            {
                bResult = ClipboardReadMetafile(hFile, dwOffData, dwLenData);
                break;
            }

            case CF_ENHMETAFILE:
            case CF_DSPENHMETAFILE:
            {
                bResult = ClipboardReadEnhMetafile(hFile, dwOffData, dwLenData);
                break;
            }

            case CF_PALETTE:
            {
                bResult = ClipboardReadPalette(hFile, dwOffData, dwLenData);
                break;
            }

            default:
            {
                if ((dwFormatID < CF_PRIVATEFIRST) || (dwFormatID > CF_PRIVATELAST))
                {
                    bResult = ClipboardReadMemory(hFile, dwFormatID, dwOffData, dwLenData, wFileIdentifier, szName);
                }
                break;
            }
        }

        if (!bResult)
            ShowLastWin32Error(Globals.hMainWnd);
    }

done:
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    return;
}

void WriteClipboardFile(LPCWSTR lpFileName, WORD wFileIdentifier)
{
    CLIPFILEHEADER ClipFileHeader;
    CLIPFORMATHEADER ClipFormatArray;
    NTCLIPFILEHEADER NtClipFileHeader;
    NTCLIPFORMATHEADER NtClipFormatArray;
    PVOID pClipFileHeader;
    PVOID pClipFormatArray;
    DWORD SizeOfFileHeader, SizeOfFormatHeader;

    WORD wFormatCount;
    DWORD dwFormatID;
    DWORD dwLenData;
    DWORD dwOffData;
    // PVOID szName;

    HANDLE hFile;
    DWORD dwBytesWritten;
    int i;

    /* Create the file for write access */
    hFile = CreateFileW(lpFileName, GENERIC_WRITE, 0, NULL,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        ShowLastWin32Error(Globals.hMainWnd);
        goto done;
    }

    wFormatCount = CountClipboardFormats();

    /* Select the file format and setup the header according to the clipboard file format ID */
    switch (wFileIdentifier)
    {
        case CLIP_FMT_31:
            SizeOfFileHeader   = sizeof(CLIPFILEHEADER);
            SizeOfFormatHeader = sizeof(CLIPFORMATHEADER);
            pClipFileHeader    = &ClipFileHeader;
            pClipFormatArray   = &ClipFormatArray;

            ClipFileHeader.wFileIdentifier = CLIP_FMT_31; // wFileIdentifier
            ClipFileHeader.wFormatCount    = wFormatCount;
            break;

        case CLIP_FMT_NT:
        case CLIP_FMT_BK:
            SizeOfFileHeader   = sizeof(NTCLIPFILEHEADER);
            SizeOfFormatHeader = sizeof(NTCLIPFORMATHEADER);
            pClipFileHeader    = &NtClipFileHeader;
            pClipFormatArray   = &NtClipFormatArray;

            NtClipFileHeader.wFileIdentifier = CLIP_FMT_NT; // wFileIdentifier
            NtClipFileHeader.wFormatCount    = wFormatCount;
            break;

        default:
            MessageBoxRes(Globals.hMainWnd, Globals.hInstance, ERROR_INVALID_FILE_FORMAT, 0, MB_ICONSTOP | MB_OK);
            goto done;
    }

    /* Write the header */
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    if (!WriteFile(hFile, pClipFileHeader, SizeOfFileHeader, &dwBytesWritten, NULL) ||
        dwBytesWritten != SizeOfFileHeader)
    {
        ShowLastWin32Error(Globals.hMainWnd);
        goto done;
    }

    /* Compute where the data should start (after the file header and the format array) */
    dwOffData = SizeOfFileHeader + wFormatCount * SizeOfFormatHeader;

    /* Loop through each format and save the data */
    i = 0;
    dwFormatID = EnumClipboardFormats(0);
    while (dwFormatID)
    {
        if (i >= wFormatCount)
        {
            /* Must never happen! */
            assert(FALSE);
            break;
        }

        /* Write the clipboard data at the specified offset, and retrieve its length */
        if (!ClipboardWriteMemory(hFile, dwFormatID, dwOffData, &dwLenData))
            goto Cont;

        /* Write the format data header */
        switch (wFileIdentifier)
        {
            case CLIP_FMT_31:
                ZeroMemory(pClipFormatArray, sizeof(CLIPFORMATHEADER));
                ((CLIPFORMATHEADER*)pClipFormatArray)->dwFormatID = dwFormatID;
                ((CLIPFORMATHEADER*)pClipFormatArray)->dwLenData  = dwLenData;
                ((CLIPFORMATHEADER*)pClipFormatArray)->dwOffData  = dwOffData;
                RetrieveClipboardFormatName(Globals.hInstance,
                                            dwFormatID,
                                            FALSE,
                                            ((CLIPFORMATHEADER*)pClipFormatArray)->szName,
                                            ARRAYSIZE(((CLIPFORMATHEADER*)pClipFormatArray)->szName));
                break;

            case CLIP_FMT_NT:
            case CLIP_FMT_BK:
                ZeroMemory(pClipFormatArray, sizeof(NTCLIPFORMATHEADER));
                ((NTCLIPFORMATHEADER*)pClipFormatArray)->dwFormatID = dwFormatID;
                ((NTCLIPFORMATHEADER*)pClipFormatArray)->dwLenData  = dwLenData;
                ((NTCLIPFORMATHEADER*)pClipFormatArray)->dwOffData  = dwOffData;
                RetrieveClipboardFormatName(Globals.hInstance,
                                            dwFormatID,
                                            TRUE,
                                            ((NTCLIPFORMATHEADER*)pClipFormatArray)->szName,
                                            ARRAYSIZE(((NTCLIPFORMATHEADER*)pClipFormatArray)->szName));
                break;
        }

        if (SetFilePointer(hFile, SizeOfFileHeader + i * SizeOfFormatHeader, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
        {
            ShowLastWin32Error(Globals.hMainWnd);
            goto done;
        }

        if (!WriteFile(hFile, pClipFormatArray, SizeOfFormatHeader, &dwBytesWritten, NULL))
        {
            ShowLastWin32Error(Globals.hMainWnd);
            goto done;
        }

        /* Adjust the offset for the next data stream */
        dwOffData += dwLenData;

Cont:
        i++;
        dwFormatID = EnumClipboardFormats(dwFormatID);
    }

done:
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    return;
}
