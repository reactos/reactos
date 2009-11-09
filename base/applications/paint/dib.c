/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        dib.c
 * PURPOSE:     Some DIB related functions
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include <windows.h>

/* FUNCTIONS ********************************************************/

HBITMAP CreateDIBWithProperties(int width, int height)
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

int GetDIBWidth(HBITMAP hBitmap)
{
    BITMAP bm;
    GetObject(hBitmap, sizeof(BITMAP), &bm);
    return bm.bmWidth;
}

int GetDIBHeight(HBITMAP hBitmap)
{
    BITMAP bm;
    GetObject(hBitmap, sizeof(BITMAP), &bm);
    return bm.bmHeight;
}

void SaveDIBToFile(HBITMAP hBitmap, LPTSTR FileName, HDC hDC)
{
    BITMAP bm;
    HANDLE hFile;
    BITMAPFILEHEADER bf;
    BITMAPINFOHEADER bi;
    int imgDataSize;
    DWORD dwBytesWritten;
    char* buffer;

    GetObject(hBitmap, sizeof(BITMAP), &bm);

    ZeroMemory(&bf, sizeof(BITMAPFILEHEADER));
    ZeroMemory(&bi, sizeof(BITMAPINFOHEADER));

    imgDataSize = bm.bmWidthBytes * bm.bmHeight;
    bf.bfType = 0x4d42; /* BM */
    bf.bfSize = imgDataSize + 52;
    bf.bfOffBits = 54;
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bm.bmWidth;
    bi.biHeight = bm.bmHeight;
    bi.biPlanes = bm.bmPlanes;
    bi.biBitCount = bm.bmBitsPixel;
    bi.biCompression = BI_RGB;

    buffer = HeapAlloc(GetProcessHeap(), 0, imgDataSize);
    GetDIBits(hDC, hBitmap, 0, bm.bmHeight, buffer, (LPBITMAPINFO)&bi, DIB_RGB_COLORS);

    hFile = CreateFile(FileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return;

    WriteFile(hFile, &bf, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, &bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, buffer, imgDataSize, &dwBytesWritten, NULL);

    CloseHandle(hFile);
    HeapFree(GetProcessHeap(), 0, buffer);
}

HBITMAP LoadDIBFromFile(LPTSTR name)
{
    HBITMAP hBitmap;
    BITMAPFILEHEADER bfh;
    BITMAPINFO *bi;
    PVOID pvBits;
    DWORD dwBytesRead;
    HANDLE hFile;

    hFile = CreateFile(name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return NULL;

    /* read header and check for 'BM' magic */
    ReadFile(hFile, &bfh, sizeof(BITMAPFILEHEADER), &dwBytesRead, NULL);
    if (bfh.bfType != 0x4d42)
    {
        CloseHandle(hFile);
        return NULL;
    }

    bi = HeapAlloc(GetProcessHeap(), 0, bfh.bfOffBits - sizeof(BITMAPFILEHEADER));
    if (!bi)
        return NULL;

    ReadFile(hFile, bi, bfh.bfOffBits - sizeof(BITMAPFILEHEADER), &dwBytesRead, NULL);
    hBitmap = CreateDIBSection(NULL, bi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    ReadFile(hFile, pvBits, bfh.bfSize - bfh.bfOffBits, &dwBytesRead, NULL);
    CloseHandle(hFile);
    HeapFree(GetProcessHeap(), 0, bi);
    return hBitmap;
}
