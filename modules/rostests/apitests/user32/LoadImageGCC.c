/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for LoadImageW using DLL compiled with MSVC
 * COPYRIGHT:   Copyright 2024 Doug Lyons <douglyons@douglyons.com>
 *
 * NOTES:
 * Works on ReactOS, but not on Windows 2003 Server SP2.
 */

#include "precomp.h"
#include "resource.h"
#include <stdio.h>
#include <versionhelpers.h>

BOOL FileExistsW(PCWSTR FileName)
{
    DWORD Attribute = GetFileAttributesW(FileName);

    return (Attribute != INVALID_FILE_ATTRIBUTES &&
            !(Attribute & FILE_ATTRIBUTE_DIRECTORY));
}

BOOL ResourceToFileW(INT i, PCWSTR FileName)
{
    FILE *fout;
    HGLOBAL hData;
    HRSRC hRes;
    PVOID pResLock;
    UINT iSize;

    if (FileExistsW(FileName))
    {
        /* We should only be using %temp% paths, so deleting here should be OK */
        printf("Deleting '%S' that already exists.\n", FileName);
        DeleteFileW(FileName);
    }

    hRes = FindResourceW(NULL, MAKEINTRESOURCEW(i), MAKEINTRESOURCEW(RT_RCDATA));
    if (hRes == NULL)
    {
        skip("Could not locate resource (%d). Exiting now\n", i);
        return FALSE;
    }

    iSize = SizeofResource(NULL, hRes);

    hData = LoadResource(NULL, hRes);
    if (hData == NULL)
    {
        skip("Could not load resource (%d). Exiting now\n", i);
        return FALSE;
    }

    // Lock the resource into global memory.
    pResLock = LockResource(hData);
    if (pResLock == NULL)
    {
        skip("Could not lock resource (%d). Exiting now\n", i);
        return FALSE;
    }

    fout = _wfopen(FileName, L"wb");
    fwrite(pResLock, iSize, 1, fout);
    fclose(fout);
    return TRUE;
}

static struct
{
    PCWSTR FileName;
    INT ResourceId;
} DataFiles[] =
{
    {L"%SystemRoot%\\bin\\image.dll", IDR_DLL_NORMAL},
};


START_TEST(LoadImageGCC)
{
    UINT i;
    WCHAR PathBuffer[MAX_PATH];
    static HBITMAP hBmp;
    HANDLE handle;
    HDC hdcMem;
    BITMAP bitmap;
    BITMAPINFO bmi;
    HGLOBAL hMem;
    LPVOID lpBits;
    CHAR img[8] = { 0 };
    UINT size;

    /* Windows 2003 cannot run this test. Testman shows CRASH, so skip it. */
    if (!IsReactOS())
        return;

    /* Extract Data Files */
    for (i = 0; i < _countof(DataFiles); ++i)
    {
        ExpandEnvironmentStringsW(DataFiles[i].FileName, PathBuffer, _countof(PathBuffer));

        if (!ResourceToFileW(DataFiles[i].ResourceId, PathBuffer))
        {
            printf("ResourceToFile Failed. Exiting now\n");
            goto Cleanup;
        }
    }

    handle = LoadLibraryExW(PathBuffer, NULL, LOAD_LIBRARY_AS_DATAFILE);
    hBmp = (HBITMAP)LoadImage(handle, MAKEINTRESOURCE(130), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);

    hdcMem = CreateCompatibleDC(NULL);
    SelectObject(hdcMem, hBmp);
    GetObject(hBmp, sizeof(BITMAP), &bitmap);

    memset(&bmi, 0, sizeof(bmi));
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = bitmap.bmWidth;
    bmi.bmiHeader.biHeight      = bitmap.bmHeight;
    bmi.bmiHeader.biPlanes      = bitmap.bmPlanes;
    bmi.bmiHeader.biBitCount    = bitmap.bmBitsPixel;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage   = 0;

    size = ((bitmap.bmWidth * bmi.bmiHeader.biBitCount + 31) / 32) * 4 * bitmap.bmHeight;

    hMem = GlobalAlloc(GMEM_MOVEABLE, size);
    lpBits = GlobalLock(hMem);
    GetDIBits(hdcMem, hBmp, 0, bitmap.bmHeight, lpBits, &bmi, DIB_RGB_COLORS);

    /* Get first 8 bytes of second row of bits from bitmap */
    memcpy(img, (VOID *)((INT_PTR)lpBits + 4 * bitmap.bmWidth), 8);

    ok(img[0] == 0, "Byte 0 Bad. Got 0x%02x, expected 0\n", img[0] & 0xff);
    ok(img[1] == 0, "Byte 1 Bad. Got 0x%02x, expected 0\n", img[1] & 0xff);
    ok(img[2] == 0, "Byte 2 Bad. Got 0x%02x, expected 0\n", img[2] & 0xff);
    ok(img[3] == 0, "Byte 3 Bad. Got 0x%02x, expected 0\n", img[3] & 0xff);

    GlobalUnlock(hMem);
    GlobalFree(hMem);

    DeleteDC(hdcMem);

Cleanup:
    for (i = 0; i < _countof(DataFiles); ++i)
    {
        ExpandEnvironmentStringsW(DataFiles[i].FileName, PathBuffer, _countof(PathBuffer));
        DeleteFileW(PathBuffer);
    }
}
