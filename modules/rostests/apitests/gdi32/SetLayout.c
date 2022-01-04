/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Tests for SetLayout and its effects on other gdi functions
 *                  such as StretchBlt, BitBlt, LPtoDP, DPtoLP
 * PROGRAMMERS:     Baruch Rutman
 *                  Inspired by the StretchBlt test
 */

#include "precomp.h"

static void copy(PUINT32 buffer, UINT32 value, int width, int start_x, int start_y, int end_x, int end_y)
{
    for (int y = start_y; y < end_y; y++)
    {
        for (int x = start_x; x < end_x; x++)
            buffer[y * width + x] = value;
    }
}

#define BLACK_PIXEL 0x000000
#define BLUE_PIXEL  0x0000FF
#define GREEN_PIXEL 0x00FF00
#define RED_PIXEL   0xFF0000
#define WHITE_PIXEL 0xFFFFFF

#if 0
#include "wincon.h"

/* Draw the bitmap as colored letters on white background */
static void
dump(PUINT32 buffer, int width, LPCSTR title)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;

    GetConsoleScreenBufferInfo(hConsole, &info);

    if (title)
        printf("%s", title);

    for (int i = 0; i < width * width; i++)
    {
        char c;
        WORD attributes = 0;
        UINT32 pixel_value = buffer[i];

        if (i % width == 0)
        {
            SetConsoleTextAttribute(hConsole, info.wAttributes);
            putchar('\n');
        }

        switch (pixel_value)
        {
            case WHITE_PIXEL:
                c = 'W';
                break;
            case BLUE_PIXEL:
                c = 'B';
                break;
            case GREEN_PIXEL:
                c = 'G';
                break;
            case RED_PIXEL:
                c = 'R';
                break;
            case BLACK_PIXEL:
                c = 'E'; /* Use 'E' for 'Empty' because 'B' is taken */
                break;
            default:
                c = '?';
        }

        if (pixel_value != WHITE_PIXEL && c != '?')
        {
            attributes = (pixel_value & RED_PIXEL) ? FOREGROUND_RED : 0 |
                         (pixel_value & GREEN_PIXEL) ? FOREGROUND_GREEN : 0 |
                         (pixel_value & BLUE_PIXEL) ? FOREGROUND_BLUE : 0;
        }

        SetConsoleTextAttribute(hConsole, attributes | BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED);
        putchar(c);
    }
    SetConsoleTextAttribute(hConsole, info.wAttributes);
    putchar('\n');
}
#endif

static void nomirror_test(PUINT32 dstBuffer, PUINT32 srcBuffer, int width, int line)
{
    for (int y = 0; y < width; y++)
    {
        for (int x = 0; x < width; x++)
        {
            if (x == width - 1)
            {
                ok(dstBuffer[y * width + x] == BLACK_PIXEL,
                   "Expected blank (black) pixel (0x0), got (%06X), coordinates (%d, %d). line: %d\n",
                   dstBuffer[y * width + x], x, y, line);
            }
            else
            {
                ok(dstBuffer[y * width + x] == srcBuffer[y * width + x + 1],
                   "Coordinates: (%d, %d), expected (%06X), got (%06X). line: %d\n",
                   x, y, srcBuffer[y * width + x + 1], dstBuffer[y * width + x], line);
            }
        }
    }
}

#define WIDTH 10
START_TEST(SetLayout)
{
    HBITMAP bmpDst, bmpSrc, oldDst, oldSrc;
    HDC hdc, hdcDst, hdcSrc;
    PUINT32 dstBuffer, srcBuffer;
    BITMAPINFO info = { 0 };
    size_t nBuf = WIDTH * WIDTH * sizeof(UINT32);

    hdc = CreateCompatibleDC(NULL);
    hdcDst = CreateCompatibleDC(hdc);
    hdcSrc = CreateCompatibleDC(hdc);

    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = WIDTH;
    info.bmiHeader.biHeight = -WIDTH;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    /* Create bitmaps to test with */
    bmpSrc = CreateDIBSection(hdcSrc, &info, DIB_RGB_COLORS, (PVOID*)&srcBuffer, NULL, 0);
    bmpDst = CreateDIBSection(hdcDst, &info, DIB_RGB_COLORS, (PVOID*)&dstBuffer, NULL, 0);

    if (!bmpSrc || !bmpDst)
    {
        skip("Failed to create bitmaps");
        goto cleanup;
    }

    oldSrc = SelectObject(hdcSrc, bmpSrc);
    oldDst = SelectObject(hdcDst, bmpDst);

    /* Create base "image" for use in the tests */
    copy(srcBuffer, WHITE_PIXEL, WIDTH, 0, 0, WIDTH / 2, WIDTH / 2);
    copy(srcBuffer, BLUE_PIXEL, WIDTH, 0, WIDTH / 2, WIDTH / 2, WIDTH);
    copy(srcBuffer, GREEN_PIXEL, WIDTH, WIDTH / 2, 0, WIDTH, WIDTH / 2);
    copy(srcBuffer, RED_PIXEL, WIDTH, WIDTH / 2, WIDTH / 2, WIDTH, WIDTH);

    /* Mirror destination DC */
    SetLayout(hdcDst, LAYOUT_RTL);
    ok(GetLayout(hdcDst) == LAYOUT_RTL, "DC layout is not RTL\n");
    ok(GetMapMode(hdcDst) == MM_ANISOTROPIC, "DC Map mode is not MM_ANISOTROPIC\n");

    /* Test RTL transform (using LPtoDP) and the inverse transform (DPtoLP) */
    for (int y = 0; y < WIDTH; y++)
    {
        for (int x = 0; x < WIDTH; x++)
        {
            POINT pt = { x, y };
            POINT mirrored = { WIDTH - 1 - x, y }; /* Expected results */

            LPtoDP(hdcDst, &pt, 1);
            /* Test LPtoDP */
            ok(pt.x == mirrored.x && pt.y == mirrored.y,
              "Coodinates: (%d, %d), expected (%ld, %ld), got (%ld, %ld)\n",
              x, y, mirrored.x, mirrored.y, pt.x, pt.y);

            pt = mirrored;

            /* Test DPtoLP */
            DPtoLP(hdcDst, &pt, 1);
            ok(pt.x == x && pt.y == y,
               "Mirrored Coodinates: (%ld, %ld), expected (%d, %d), got (%ld, %ld)\n",
               mirrored.x, mirrored.y, x, y, pt.x, pt.y);
        }
    }

    ZeroMemory(dstBuffer, nBuf);
    StretchBlt(hdcDst, 0, 0, WIDTH, WIDTH, hdcSrc, 0, 0, WIDTH, WIDTH, SRCCOPY);
    for (int y = 0; y < WIDTH; y++)
    {
        for (int x = 0; x < WIDTH; x++)
        {
            /* Test if the image is mirrored using the assumed RTL transform results */
            ok(dstBuffer[y * WIDTH + (WIDTH - 1 - x)] == srcBuffer[y * WIDTH + x],
               "Coordinates: (%d, %d), expected (%06X), got (%06X)\n",
               x, y, srcBuffer[y * WIDTH + x], dstBuffer[y * WIDTH + (WIDTH - 1 - x)]);
        }
    }

    ZeroMemory(dstBuffer, nBuf);
    StretchBlt(hdcDst, 0, 0, WIDTH, WIDTH, hdcSrc, 0, 0, WIDTH, WIDTH, SRCCOPY | NOMIRRORBITMAP);
    nomirror_test(dstBuffer, srcBuffer, WIDTH, __LINE__);

    ZeroMemory(dstBuffer, nBuf);
    BitBlt(hdcDst, 0, 0, WIDTH, WIDTH, hdcSrc, 0, 0, SRCCOPY);
    for (int y = 0; y < WIDTH; y++)
    {
        for (int x = 0; x < WIDTH; x++)
        {
            /* Test if the image is mirrored using the assumed RTL transform results */
            ok(dstBuffer[y * WIDTH + (WIDTH - 1 - x)] == srcBuffer[y * WIDTH + x],
               "Coordinates: (%d, %d), expected (%06X), got (%06X)\n",
               x, y, srcBuffer[y * WIDTH + x], dstBuffer[y * WIDTH + (WIDTH - 1 - x)]);
        }
    }

    ZeroMemory(dstBuffer, nBuf);
    BitBlt(hdcDst, 0, 0, WIDTH, WIDTH, hdcSrc, 0, 0, SRCCOPY | NOMIRRORBITMAP);
    nomirror_test(dstBuffer, srcBuffer, WIDTH, __LINE__);

    SetLayout(hdcDst, LAYOUT_RTL | LAYOUT_BITMAPORIENTATIONPRESERVED);

    ok(GetLayout(hdcDst) == (LAYOUT_RTL | LAYOUT_BITMAPORIENTATIONPRESERVED),
       "DC Layout is not LAYOUT_RTL | LAYOUT_BITMAPORIENTATIONPRESERVED\n");
    ok(GetMapMode(hdcDst) == MM_ANISOTROPIC, "DC Map mode is not MM_ANISOTROPIC\n");

    ZeroMemory(dstBuffer, nBuf);
    StretchBlt(hdcDst, 0, 0, WIDTH, WIDTH, hdcSrc, 0, 0, WIDTH, WIDTH, SRCCOPY);
    nomirror_test(dstBuffer, srcBuffer, WIDTH, __LINE__);

    ZeroMemory(dstBuffer, nBuf);
    StretchBlt(hdcDst, 0, 0, WIDTH, WIDTH, hdcSrc, 0, 0, WIDTH, WIDTH, SRCCOPY | NOMIRRORBITMAP);
    nomirror_test(dstBuffer, srcBuffer, WIDTH, __LINE__);

    ZeroMemory(dstBuffer, nBuf);
    BitBlt(hdcDst, 0, 0, WIDTH, WIDTH, hdcSrc, 0, 0, SRCCOPY);
    nomirror_test(dstBuffer, srcBuffer, WIDTH, __LINE__);

    ZeroMemory(dstBuffer, nBuf);
    BitBlt(hdcDst, 0, 0, WIDTH, WIDTH, hdcSrc, 0, 0, SRCCOPY | NOMIRRORBITMAP);
    nomirror_test(dstBuffer, srcBuffer, WIDTH, __LINE__);

    /* Reset DC layout to default (LTR) */
    SetLayout(hdcDst, LAYOUT_LTR);
    ok(GetLayout(hdcDst) == LAYOUT_LTR, "DC layout is not LAYOUT_LTR");

    for (int y = 0; y < WIDTH; y++)
    {
        for (int x = 0; x < WIDTH; x++)
        {
            POINT pt = { x, y };

            LPtoDP(hdcDst, &pt, 1);
            /* Confirm that RTL transform is not the current one */
            ok(pt.x == x && pt.y == y,
               "Expected (%d, %d) got (%ld, %ld)\n", x, y, pt.x, pt.y);
        }
    }

    ZeroMemory(dstBuffer, nBuf);
    StretchBlt(hdcDst, 0, 0, WIDTH, WIDTH, hdcSrc, 0, 0, WIDTH, WIDTH, SRCCOPY);
    ok(memcmp(dstBuffer, srcBuffer, nBuf) == 0, "Bitmaps are not identical\n");

    SetLayout(hdcDst, LAYOUT_BITMAPORIENTATIONPRESERVED);
    ok(GetLayout(hdcDst) == LAYOUT_BITMAPORIENTATIONPRESERVED, "DC Layout is not LAYOUT_BITMAPORIENTATIONPRESERVED");

    SelectObject(hdcSrc, oldSrc);
    SelectObject(hdcDst, oldDst);
    DeleteObject(bmpSrc);
    DeleteObject(bmpDst);
cleanup:
    DeleteDC(hdcSrc);
    DeleteDC(hdcDst);
    DeleteDC(hdc);
}
