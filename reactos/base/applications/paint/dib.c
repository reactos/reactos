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
    BITMAPINFO bitmapinfo;
    bitmapinfo.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
    bitmapinfo.bmiHeader.biWidth = width;
    bitmapinfo.bmiHeader.biHeight = height;
    bitmapinfo.bmiHeader.biPlanes = 1;
    bitmapinfo.bmiHeader.biBitCount = 24;
    bitmapinfo.bmiHeader.biCompression = BI_RGB;
    bitmapinfo.bmiHeader.biSizeImage = 0;
    bitmapinfo.bmiHeader.biXPelsPerMeter = 0;
    bitmapinfo.bmiHeader.biYPelsPerMeter = 0;
    bitmapinfo.bmiHeader.biClrUsed = 0;
    bitmapinfo.bmiHeader.biClrImportant = 0;
    return CreateDIBSection(NULL, &bitmapinfo, DIB_RGB_COLORS, NULL, NULL, 0);
}

int GetDIBWidth(HBITMAP hbm)
{
    BITMAP bm;
    GetObject(hbm, sizeof(BITMAP), &bm);
    return bm.bmWidth;
}

int GetDIBHeight(HBITMAP hbm)
{
    BITMAP bm;
    GetObject(hbm, sizeof(BITMAP), &bm);
    return bm.bmHeight;
}

void SaveDIBToFile(HBITMAP hbm, LPTSTR name, HDC hdc)
{
    BITMAP bm;
    HANDLE hFile;
    BITMAPFILEHEADER bf;
    BITMAPINFOHEADER bi;
    int imgDataSize;
    int bytesWritten;

    GetObject(hbm, sizeof(BITMAP), &bm);

    imgDataSize = bm.bmWidthBytes*bm.bmHeight;
    bf.bfType = 0x4d42;
    bf.bfSize = imgDataSize+52;
    bf.bfReserved1 = 0;
    bf.bfReserved2 = 0;
    bf.bfOffBits = 54;
    bi.biSize = 40;
    bi.biWidth = bm.bmWidth;
    bi.biHeight = bm.bmHeight;
    bi.biPlanes = bm.bmPlanes;
    bi.biBitCount = bm.bmBitsPixel;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;
    int *buffer = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, imgDataSize);
    GetDIBits(hdc, hbm, 0, bm.bmHeight, buffer, (LPBITMAPINFO)&bi, DIB_RGB_COLORS);
    hFile = CreateFile(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    WriteFile(hFile, &bf, 14, (LPDWORD)&bytesWritten, NULL);
    WriteFile(hFile, &bi, 40, (LPDWORD)&bytesWritten, NULL);
    WriteFile(hFile, buffer, imgDataSize, (LPDWORD)&bytesWritten, NULL);
    CloseHandle(hFile);
    HeapFree(GetProcessHeap(), 0, buffer);
}

HBITMAP LoadDIBFromFile(LPTSTR name)
{
    HBITMAP bm;
    BITMAPFILEHEADER bfh;
    BITMAPINFO *bi;
    VOID *data;
    int bytesRead;
    HANDLE f = CreateFile(name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    ReadFile(f, &bfh, 14, (LPDWORD)&bytesRead, NULL);
    if (bfh.bfType!=0x4d42)
    {
        CloseHandle(f);
        return NULL;
    }
    bi = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, bfh.bfOffBits-14);
    ReadFile(f, bi, bfh.bfOffBits-14, (LPDWORD)&bytesRead, NULL);
    bm = CreateDIBSection(NULL, bi, DIB_RGB_COLORS, &data, NULL, 0);
    ReadFile(f, data, bfh.bfSize-bfh.bfOffBits, (LPDWORD)&bytesRead, NULL);
    CloseHandle(f);
    HeapFree(GetProcessHeap(), 0, bi);
    return bm;
}
