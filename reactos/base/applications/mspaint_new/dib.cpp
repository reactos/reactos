/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/dib.cpp
 * PURPOSE:     Some DIB related functions
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

/* FUNCTIONS ********************************************************/

HBITMAP
CreateDIBWithProperties(int width, int height)
{
    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;
    return CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, NULL, NULL, 0);
}

int
GetDIBWidth(HBITMAP hBitmap)
{
    BITMAP bm;
    GetObject(hBitmap, sizeof(BITMAP), &bm);
    return bm.bmWidth;
}

int
GetDIBHeight(HBITMAP hBitmap)
{
    BITMAP bm;
    GetObject(hBitmap, sizeof(BITMAP), &bm);
    return bm.bmHeight;
}

void
SaveDIBToFile(HBITMAP hBitmap, LPTSTR FileName, HDC hDC, LPSYSTEMTIME time, int *size, int hRes, int vRes)
{
    BITMAP bm;
    HANDLE hFile;
    BITMAPFILEHEADER bf;
    BITMAPINFOHEADER bi;
    int imgDataSize;
    DWORD dwBytesWritten;
    char *buffer;

    GetObject(hBitmap, sizeof(BITMAP), &bm);

    ZeroMemory(&bf, sizeof(BITMAPFILEHEADER));
    ZeroMemory(&bi, sizeof(BITMAPINFOHEADER));

    imgDataSize = bm.bmWidthBytes * bm.bmHeight;
    bf.bfType = 0x4d42;         /* BM */
    bf.bfSize = imgDataSize + 52;
    bf.bfOffBits = 54;
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bm.bmWidth;
    bi.biHeight = bm.bmHeight;
    bi.biPlanes = bm.bmPlanes;
    bi.biBitCount = bm.bmBitsPixel;
    bi.biCompression = BI_RGB;
    bi.biXPelsPerMeter = hRes;
    bi.biYPelsPerMeter = vRes;

    buffer = (char*) HeapAlloc(GetProcessHeap(), 0, imgDataSize);
    if (!buffer)
        return;

    GetDIBits(hDC, hBitmap, 0, bm.bmHeight, buffer, (LPBITMAPINFO) & bi, DIB_RGB_COLORS);

    hFile = CreateFile(FileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        HeapFree(GetProcessHeap(), 0, buffer);
        return;
    }

    WriteFile(hFile, &bf, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, &bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, buffer, imgDataSize, &dwBytesWritten, NULL);

    if (time)
    {
        FILETIME ft;
        GetFileTime(hFile, NULL, NULL, &ft);
        FileTimeToSystemTime(&ft, time);
    }
    if (size)
        *size = GetFileSize(hFile, NULL);

    CloseHandle(hFile);
    HeapFree(GetProcessHeap(), 0, buffer);
}

void ShowFileLoadError(LPTSTR name)
{
    TCHAR programname[20];
    TCHAR loaderrortext[100];
    TCHAR temptext[500];
    LoadString(hProgInstance, IDS_PROGRAMNAME, programname, SIZEOF(programname));
    LoadString(hProgInstance, IDS_LOADERRORTEXT, loaderrortext, SIZEOF(loaderrortext));
    _stprintf(temptext, loaderrortext, name);
    MessageBox(hMainWnd, temptext, programname, MB_OK | MB_ICONEXCLAMATION);
}

void
LoadDIBFromFile(HBITMAP * hBitmap, LPTSTR name, LPSYSTEMTIME time, int *size, int *hRes, int *vRes)
{
    BITMAPFILEHEADER bfh;
    BITMAPINFO *bi;
    PVOID pvBits;
    DWORD dwBytesRead;
    HANDLE hFile;

    if (!hBitmap)
    {
        ShowFileLoadError(name);
        return;
    }

    hFile =
        CreateFile(name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        ShowFileLoadError(name);
        return;
    }

    /* read header and check for 'BM' magic */
    ReadFile(hFile, &bfh, sizeof(BITMAPFILEHEADER), &dwBytesRead, NULL);
    if (bfh.bfType != 0x4d42)
    {
        CloseHandle(hFile);
        ShowFileLoadError(name);
        return;
    }

    if (time)
    {
        FILETIME ft;
        GetFileTime(hFile, NULL, NULL, &ft);
        FileTimeToSystemTime(&ft, time);
    }
    if (size)
        *size = GetFileSize(hFile, NULL);

    bi = (BITMAPINFO*) HeapAlloc(GetProcessHeap(), 0, bfh.bfOffBits - sizeof(BITMAPFILEHEADER));
    if (!bi)
    {
        CloseHandle(hFile);
        ShowFileLoadError(name);
        return;
    }

    ReadFile(hFile, bi, bfh.bfOffBits - sizeof(BITMAPFILEHEADER), &dwBytesRead, NULL);
    *hBitmap = CreateDIBSection(NULL, bi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    ReadFile(hFile, pvBits, bfh.bfSize - bfh.bfOffBits, &dwBytesRead, NULL);

    if (hRes)
        *hRes = (*bi).bmiHeader.biXPelsPerMeter;
    if (vRes)
        *vRes = (*bi).bmiHeader.biYPelsPerMeter;

    CloseHandle(hFile);
    HeapFree(GetProcessHeap(), 0, bi);
}
