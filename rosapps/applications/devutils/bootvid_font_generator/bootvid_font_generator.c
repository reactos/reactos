/*
 * PROJECT:     ReactOS bootvid Font Generator Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Generates the FontData array for the bootdata.c file of bootvid
 * COPYRIGHT:   Copyright 2016 Colin Finck <colin@reactos.org>
 */

#include <windows.h>
#include <stdio.h>

// Windows original Blue Screen font is "Lucida Console" at FONT_SIZE 10 with no offsets.
#define FONT_NAME       L"Anonymous Pro"
#define FONT_SIZE       10
#define X_OFFSET        1
#define Y_OFFSET        0
#define HEIGHT          13
#define WIDTH           8

/**
 * Sketch the character on the console screen using ASCII characters.
 * Allows you to easily check if the font fits properly into the 8x13 box.
 */
void DumpCharacterOnScreen(DWORD BmpBits[])
{
    int i;
    int j;

    for (i = 0; i < HEIGHT; i++)
    {
        for (j = WIDTH; --j >= 0;)
        {
            if (BmpBits[i] >> j & 0x1)
                putchar(' ');
            else
                putchar('#');
        }

        putchar('\n');
    }
}

/**
 * Dump the FontData for the bootvid/i386/bootdata.c array.
 */
void DumpCharacterFontData(DWORD BmpBits[])
{
    static int iBegin = 0;
    int i;

    printf("    ");

    for (i = 0; i < HEIGHT; i++)
        printf("0x%02lX, ", BmpBits[i]);

    printf(" // %d\n", iBegin);
    iBegin += HEIGHT;
}

/**
 * Use GDI APIs to load a monospace font and plot a single character into a bitmap.
 */
BOOL PlotCharacter(WCHAR Character, DWORD BmpBits[])
{
    BOOL bReturnValue = FALSE;
    int iHeight;
    HBITMAP hBmp = NULL;
    HDC hDC = NULL;
    HFONT hFont = NULL;
    PBITMAPINFO pBmpInfo;

    hDC = CreateCompatibleDC(NULL);
    if (!hDC)
    {
        printf("CreateCompatibleDC failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }

    hBmp = CreateCompatibleBitmap(hDC, WIDTH, HEIGHT);
    if (!hBmp)
    {
        printf("CreateCompatibleBitmap failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }

    iHeight = -MulDiv(FONT_SIZE, GetDeviceCaps(hDC, LOGPIXELSY), 72);
    hFont = CreateFontW(iHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, 0, 0, NONANTIALIASED_QUALITY, FIXED_PITCH, FONT_NAME);
    if (!hFont)
    {
        printf("CreateFontW failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }

    SelectObject(hDC, hBmp);
    SelectObject(hDC, hFont);
    SetBkColor(hDC, RGB(0, 0, 0));
    SetTextColor(hDC, RGB(255, 255, 255));
    TextOutW(hDC, X_OFFSET, Y_OFFSET, &Character, 1);

    // Allocate enough memory for BITMAPINFO and one additional color in the color table.
    // BITMAPINFO already contains a color table entry for a single color and GetDIBits needs space for two colors (black and white).
    pBmpInfo = HeapAlloc(GetProcessHeap(), 0, sizeof(BITMAPINFO) + sizeof(RGBQUAD));
    ZeroMemory(pBmpInfo, sizeof(BITMAPINFO) + sizeof(RGBQUAD));
    pBmpInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pBmpInfo->bmiHeader.biHeight = -HEIGHT;
    pBmpInfo->bmiHeader.biWidth = WIDTH;
    pBmpInfo->bmiHeader.biBitCount = 1;
    pBmpInfo->bmiHeader.biPlanes = 1;

    if (!GetDIBits(hDC, hBmp, 0, HEIGHT, BmpBits, pBmpInfo, 0))
    {
        printf("GetDIBits failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }

    bReturnValue = TRUE;

Cleanup:
    if (hFont)
        DeleteObject(hFont);

    if (hBmp)
        DeleteObject(hBmp);

    if (hDC)
        DeleteDC(hDC);

    return bReturnValue;
}

int main()
{
    DWORD BmpBits[HEIGHT];
    WCHAR c;

    for (c = 0; c < 256; c++)
    {
        PlotCharacter(c, BmpBits);

#if 0
        DumpCharacterOnScreen(BmpBits);
        system("pause");
        system("cls");
#else
        DumpCharacterFontData(BmpBits);
#endif
    }

    return 0;
}
