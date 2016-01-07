/*
 * PROJECT:     ReactOS BootVid Font Generator Utility
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Generates the FontData array for the bootdata.c file of bootvid.dll
 * COPYRIGHT:   Copyright 2016 Colin Finck <colin@reactos.org>
 */

#include <stdio.h>
#include <conio.h>
#include <windows.h>

/*
 * Enable this #define if you want to dump the generated character on screen
 */
// #define DUMP_CHAR_ON_SCREEN

// Windows original Blue Screen font is "Lucida Console" at FONT_SIZE 10 with no offsets.
#define FONT_NAME_DEF   "Lucida Console" // "DejaVu Sans Mono" // "Anonymous Pro"
#define FONT_SIZE_DEF   10
#define X_OFFSET_DEF    0                // 0                  // 1
#define Y_OFFSET_DEF    0

#define HEIGHT          13  // Must be == BOOTCHAR_HEIGHT (see reactos/drivers/base/bootvid/precomp.h)
#define WIDTH           8   //  8 bits == 1 byte

#ifdef DUMP_CHAR_ON_SCREEN
/**
 * Sketch the character on the console screen using ASCII characters.
 * Allows you to easily check if the font fits properly into the 8x13 box.
 */
static void DumpCharacterOnScreen(DWORD BmpBits[])
{
    int i, j;

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

#else

/**
 * Dump the FontData for the bootvid/i386/bootdata.c array.
 */
static void DumpCharacterFontData(DWORD BmpBits[])
{
    static int iBegin = 0;
    int i;

    fprintf(stdout, "    ");

    for (i = 0; i < HEIGHT; i++)
        fprintf(stdout, "0x%02lX, ", BmpBits[i]);

    fprintf(stdout, " // %d\n", iBegin);
    iBegin += HEIGHT;
}
#endif

/**
 * Use GDI APIs to load a monospace font and plot a single character into a bitmap.
 */
static BOOL PlotCharacter(HDC hDC, HFONT hFont, INT XOffset, INT YOffset, CHAR Character, DWORD BmpBits[])
{
    BOOL bReturnValue = FALSE;
    HBITMAP hOldBmp;
    HFONT hOldFont;
    HBITMAP hBmp = NULL;
    BYTE BmpInfo[sizeof(BITMAPINFO) + sizeof(RGBQUAD)];
    PBITMAPINFO pBmpInfo = (PBITMAPINFO)&BmpInfo;

    hBmp = CreateCompatibleBitmap(hDC, WIDTH, HEIGHT);
    if (!hBmp)
    {
        fprintf(stderr, "CreateCompatibleBitmap failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }

    hOldBmp  = SelectObject(hDC, hBmp);
    hOldFont = SelectObject(hDC, hFont);
    SetBkColor(hDC, RGB(0, 0, 0));
    SetTextColor(hDC, RGB(255, 255, 255));
    TextOutA(hDC, XOffset, YOffset, &Character, 1);

    /*
     * Use enough memory for BITMAPINFO and one additional color in the color table.
     * BITMAPINFO already contains a color table entry for a single color and
     * GetDIBits needs space for two colors (black and white).
     */
    ZeroMemory(&BmpInfo, sizeof(BmpInfo));
    pBmpInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pBmpInfo->bmiHeader.biHeight = -HEIGHT;
    pBmpInfo->bmiHeader.biWidth = WIDTH;
    pBmpInfo->bmiHeader.biBitCount = 1;
    pBmpInfo->bmiHeader.biPlanes = 1;

    bReturnValue = TRUE;

    if (!GetDIBits(hDC, hBmp, 0, HEIGHT, BmpBits, pBmpInfo, 0))
    {
        fprintf(stderr, "GetDIBits failed with error %lu!\n", GetLastError());
        bReturnValue = FALSE;
    }

    SelectObject(hDC, hOldBmp);
    SelectObject(hDC, hOldFont);

Cleanup:
    if (hBmp)
        DeleteObject(hBmp);

    return bReturnValue;
}

static void DumpFont(LPSTR FontName, INT FontSize, INT XOffset, INT YOffset)
{
    int iHeight;
    HDC hDC = NULL;
    HFONT hFont = NULL;

    DWORD BmpBits[HEIGHT];
    USHORT c;

    hDC = CreateCompatibleDC(NULL);
    if (!hDC)
    {
        fprintf(stderr, "CreateCompatibleDC failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }

    iHeight = -MulDiv(FontSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
    hFont = CreateFontA(iHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        NONANTIALIASED_QUALITY, FIXED_PITCH, FontName);
    if (!hFont)
    {
        fprintf(stderr, "CreateFont failed with error %lu!\n", GetLastError());
        goto Cleanup;
    }

    for (c = 0; c < 256; c++)
    {
        PlotCharacter(hDC, hFont, XOffset, YOffset, (CHAR)c, BmpBits);

#ifdef DUMP_CHAR_ON_SCREEN
        DumpCharacterOnScreen(BmpBits);
        fprintf(stdout, "\nPress any key to continue...\n");
        _getch();
        system("cls");
#else
        DumpCharacterFontData(BmpBits);
#endif
    }

Cleanup:
    if (hFont)
        DeleteObject(hFont);

    if (hDC)
        DeleteDC(hDC);
}

int main(int argc, char** argv)
{
    /* Validate the arguments */
    if (argc > 5 || (argc >= 2 && strncmp(argv[1], "/?", 2) == 0))
    {
        fprintf(stdout,
                "Usage: %s \"font name\" [font size] [X-offset] [Y-offset]\n"
                "Default font name is: \"%s\"\n"
                "Default font size is: %i\n"
                "Default X-offset  is: %i\n"
                "Default Y-offset  is: %i\n",
                argv[0],
                FONT_NAME_DEF, FONT_SIZE_DEF, X_OFFSET_DEF, Y_OFFSET_DEF);

        return -1;
    }

    DumpFont((argc <= 1) ? FONT_NAME_DEF : argv[1],
             (argc <= 2) ? FONT_SIZE_DEF : atoi(argv[2]),
             (argc <= 3) ?  X_OFFSET_DEF : atoi(argv[3]),
             (argc <= 4) ?  Y_OFFSET_DEF : atoi(argv[4]));
    return 0;
}
