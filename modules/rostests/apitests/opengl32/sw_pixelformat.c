/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * PURPOSE:         Tests pixel formats exposed by the software implementation
 * PROGRAMMERS:     Jérôme Gardou
 */

#include <windows.h>
#include <wingdi.h>
#include <GL/gl.h>
#include <stdio.h>

#include "wine/test.h"

static char* str_dbg_pfd_flags(DWORD flags)
{
    static char buffer[1000];
    static char *p = buffer;
    char* ret;

    BOOL first = TRUE;

    if (p > &buffer[800])
        p = buffer;

    *p = 0;

#define FLAGS_TO_STR(__x__)                 \
    do {                                    \
        if (flags & __x__)                  \
        {                                   \
            if (!first) strcat(p, " | ");   \
            strcat(p, #__x__);              \
            first = FALSE;                  \
            flags &= ~((DWORD)__x__);       \
        }                                   \
    } while(0)

    FLAGS_TO_STR(PFD_DOUBLEBUFFER);
    FLAGS_TO_STR(PFD_STEREO);
    FLAGS_TO_STR(PFD_DRAW_TO_WINDOW);
    FLAGS_TO_STR(PFD_DRAW_TO_BITMAP);
    FLAGS_TO_STR(PFD_SUPPORT_GDI);
    FLAGS_TO_STR(PFD_SUPPORT_OPENGL);
    FLAGS_TO_STR(PFD_GENERIC_FORMAT);
    FLAGS_TO_STR(PFD_NEED_PALETTE);
    FLAGS_TO_STR(PFD_NEED_SYSTEM_PALETTE);
    FLAGS_TO_STR(PFD_SWAP_EXCHANGE);
    FLAGS_TO_STR(PFD_SWAP_COPY);
    FLAGS_TO_STR(PFD_SWAP_LAYER_BUFFERS);
    FLAGS_TO_STR(PFD_GENERIC_ACCELERATED);
    FLAGS_TO_STR(PFD_SUPPORT_COMPOSITION);
#undef FLAGS_TO_STR

    if (flags)
    {
        char tmp[16];
        sprintf(tmp, " | 0x%08x", (UINT)flags);
        strcat(p, tmp);
    }

    ret = p;

    p += strlen(p) + 1;

    return ret;
}

struct test_color
{
    BYTE iPixelType;
    BYTE cColorBits;
    BYTE cRedBits; BYTE cRedShift;
    BYTE cGreenBits; BYTE cGreenShift;
    BYTE cBlueBits; BYTE cBlueShift;
    BYTE cAlphaBits; BYTE cAlphaShift;
    BYTE cAccumBits;
    BYTE cAccumRedBits; BYTE cAccumGreenBits; BYTE cAccumBlueBits; BYTE cAccumAlphaBits;
};

static const struct test_color test_colors_32[] =
{
    {PFD_TYPE_RGBA,       32, 8, 16, 8, 8, 8, 0, 0, 0, 64, 16, 16, 16, 0},
    {PFD_TYPE_RGBA,       32, 8, 16, 8, 8, 8, 0, 8, 0, 64, 16, 16, 16, 16},
    {PFD_TYPE_COLORINDEX, 32, 8, 16, 8, 8, 8, 0, 0, 0, 0,  0,  0,  0,  0},
};

static const struct test_color test_colors_24[] =
{
    {PFD_TYPE_RGBA,       24, 8, 16, 8, 8, 8, 0, 0, 0, 64, 16, 16, 16, 0},
    {PFD_TYPE_RGBA,       24, 8, 16, 8, 8, 8, 0, 8, 0, 64, 16, 16, 16, 16},
    {PFD_TYPE_COLORINDEX, 24, 8, 16, 8, 8, 8, 0, 0, 0, 0,  0,  0,  0,  0},
};

static const struct test_color test_colors_16[] =
{
    {PFD_TYPE_RGBA,       16, 5, 10, 5, 5, 5, 0, 0, 0, 32, 11, 11, 10, 0},
    {PFD_TYPE_RGBA,       16, 5, 10, 5, 5, 5, 0, 8, 0, 32, 8,  8,  8,  8},
    {PFD_TYPE_COLORINDEX, 16, 5, 10, 5, 5, 5, 0, 0, 0, 0,  0,  0,  0,  0},
};

static const struct test_color test_colors_16_565[] =
{
    {PFD_TYPE_RGBA,       16, 5, 11, 6, 5, 5, 0, 0, 0, 32, 11, 11, 10, 0},
    {PFD_TYPE_RGBA,       16, 5, 11, 6, 5, 5, 0, 8, 0, 32, 8,  8,  8,  8},
    {PFD_TYPE_COLORINDEX, 16, 5, 11, 6, 5, 5, 0, 0, 0, 0,  0,  0,  0,  0},
};

static const struct test_color test_colors_8[] =
{
    {PFD_TYPE_RGBA,       8,  3, 0,  3, 3, 2, 6, 0, 0, 32, 11, 11, 10, 0},
    {PFD_TYPE_RGBA,       8,  3, 0,  3, 3, 2, 6, 8, 0, 32, 8,  8,  8,  8},
    {PFD_TYPE_COLORINDEX, 8,  3, 0,  3, 3, 2, 6, 0, 0, 0,  0,  0,  0,  0},
};

static const struct test_color test_colors_4[] =
{
    {PFD_TYPE_RGBA,       4,  1, 0,  1, 1, 1, 2, 0, 0, 16, 5,  6,  5,  0},
    {PFD_TYPE_RGBA,       4,  1, 0,  1, 1, 1, 2, 8, 0, 16, 4,  4,  4,  4},
    {PFD_TYPE_COLORINDEX, 4,  1, 0,  1, 1, 1, 2, 0, 0, 0,  0,  0,  0,  0},
};

static const struct test_color* test_colors_32bpp[] = {test_colors_32,     test_colors_24, test_colors_16, test_colors_8,  test_colors_4, NULL};
static const struct test_color* test_colors_24bpp[] = {test_colors_24,     test_colors_32, test_colors_16, test_colors_8,  test_colors_4, NULL};
static const struct test_color* test_colors_16bpp[] = {test_colors_16_565, test_colors_24, test_colors_32, test_colors_8,  test_colors_4, NULL};
static const struct test_color* test_colors_8bpp[]  = {test_colors_8,      test_colors_24, test_colors_32, test_colors_16, test_colors_4, NULL};

static const BYTE test_depths[] = { 32, 16 };

static void test_format(HDC hdc, int num_pf, int pf, const struct test_color *colors, BYTE depth, BOOL is_window, BOOL is_double_buffered)
{
    PIXELFORMATDESCRIPTOR pfd;
    int ret;

    ret = DescribePixelFormat(hdc, pf, sizeof(pfd), &pfd);
    ok(ret == num_pf, "Number of pixel formats changed!\n");

    ok(pfd.nSize == sizeof(pfd), "Wrong nSize for format %i, expected %u, got %u\n", pf, (WORD)sizeof(pfd), pfd.nSize);
    ok(pfd.nVersion == 1, "Wrong nVersion for format %i, expected 1, got %u\n", pf, pfd.nVersion);
    if (is_window)
    {
        DWORD flags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_GENERIC_FORMAT;

        if ((pfd.cColorBits <= 8) && (pfd.iPixelType == PFD_TYPE_RGBA))
            flags |= PFD_NEED_PALETTE;

        if (is_double_buffered)
        {
            flags |= PFD_DOUBLEBUFFER | PFD_SWAP_COPY;
        }
        else
        {
            flags |= PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI;
        }
        ok((pfd.dwFlags == flags) || (pfd.dwFlags == (flags | PFD_SUPPORT_COMPOSITION)),
                    "Wrong dwFlags for format %i, expected %s (0x%08x), got %s(0x%08x)\n",
                    pf, str_dbg_pfd_flags(flags), (UINT)flags, str_dbg_pfd_flags(pfd.dwFlags), (UINT)pfd.dwFlags);
    }
    else
    {
        ok(pfd.dwFlags == (PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI | PFD_SUPPORT_OPENGL | PFD_GENERIC_FORMAT),
                "Wrong dwFlags for format %i, expected PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI | PFD_SUPPORT_OPENGL | PFD_GENERIC_FORMAT, got %s\n",
                pf, str_dbg_pfd_flags(pfd.dwFlags));
    }

#define TEST_FIELD(__x__) ok(pfd.__x__ == colors->__x__, "Wrong " #__x__ " for format %i, expected %u, got %u\n", pf, colors->__x__, pfd.__x__)
    TEST_FIELD(iPixelType);
    TEST_FIELD(cColorBits);
    TEST_FIELD(cRedBits);
    TEST_FIELD(cRedShift);
    TEST_FIELD(cGreenBits);
    TEST_FIELD(cGreenShift);
    TEST_FIELD(cBlueBits);
    TEST_FIELD(cBlueShift);
    TEST_FIELD(cAlphaBits);
    TEST_FIELD(cAlphaShift);
    TEST_FIELD(cAccumBits);
    TEST_FIELD(cAccumRedBits);
    TEST_FIELD(cAccumGreenBits);
    TEST_FIELD(cAccumBlueBits);
    TEST_FIELD(cAccumAlphaBits);
#undef TEST_FIELD
    ok(pfd.cDepthBits == depth, "Wrong cDepthBit for format %i, expected %u, got %u\n", pf, depth, pfd.cDepthBits);

    /* the rest is constant */
#define TEST_FIELD(__x__, __y__) ok(pfd.__x__ == __y__, "Wrong " #__x__ " for format %i, expected " #__y__ ", got %u\n", pf, (UINT)pfd.__x__)
    TEST_FIELD(cStencilBits, 8);
    TEST_FIELD(cAuxBuffers, 0);
    TEST_FIELD(iLayerType, 0);
    TEST_FIELD(bReserved, 0);
    TEST_FIELD(dwLayerMask, 0);
    TEST_FIELD(dwVisibleMask, 0);
    TEST_FIELD(dwDamageMask, 0);
#undef TEST_FIELD
}

static void test_screen_colors(const struct test_color **colors)
{
    int pf, num_pf, ret;
    int i, j;
    HDC hdc;
    HWND hwnd;
    PIXELFORMATDESCRIPTOR pfd;

    hwnd = CreateWindowA("static", "Title", WS_OVERLAPPEDWINDOW, 10, 10, 200, 200, NULL, NULL,
            NULL, NULL);
    if (!hwnd)
    {
        ok(hwnd != NULL, "Failed to create a window.\n");
        return;
    }

    hdc = GetDC( hwnd );
    num_pf = DescribePixelFormat(hdc, 0, 0, NULL);
    ok(num_pf > 0, "DescribePixelFormat failed.\n");

    for (pf = 1; pf <= num_pf; pf++)
    {
        ret = DescribePixelFormat(hdc, pf, sizeof(pfd), &pfd);
        ok(ret == num_pf, "Number of pixel formats changed!\n");

        if (pfd.dwFlags & PFD_GENERIC_FORMAT)
            break;
    }

    ok(pf < num_pf, "No generic pixel format!\n");

    /* First the formats compatible with the DC */
    for (i = 0; i < 3; i++)
    {
        /* Single buffered first */
        for (j = 0; j < ARRAY_SIZE(test_depths); j++)
        {
            test_format(hdc, num_pf, pf++, &colors[0][i], test_depths[j], TRUE, FALSE);
        }

        /* Then double buffered */
        for (j = 0; j < ARRAY_SIZE(test_depths); j++)
        {
            test_format(hdc, num_pf, pf++, &colors[0][i], test_depths[j], TRUE, TRUE);
        }
    }

    colors++;

    /* Then the rest */
    while (*colors)
    {
        for (i = 0; i < 3; i++)
        {
            for (j = 0; j < ARRAY_SIZE(test_depths); j++)
            {
                test_format(hdc, num_pf, pf++, &colors[0][i], test_depths[j], FALSE, FALSE);
            }
        }
        colors++;
    }

    /* We must have tested all generic formats. */
    ok(pf - 1 == num_pf, "We didn't test all generic formats : tested %u, total %u.\n", pf, num_pf);

    ReleaseDC( hwnd, hdc );
    DestroyWindow( hwnd );
}

static HBITMAP create_dib_section(WORD bpp, void** dstBuffer)
{
    BITMAPINFO bi;

    memset(&bi, 0, sizeof(BITMAPINFO));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = 4;
    bi.bmiHeader.biHeight = -4;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = bpp;
    bi.bmiHeader.biCompression = BI_RGB;

    return CreateDIBSection(0, &bi, DIB_RGB_COLORS, dstBuffer, NULL, 0);
}

static void test_bitmap_colors(HDC hdc, const struct test_color** colors)
{
    INT num_pf, pf, ret;
    PIXELFORMATDESCRIPTOR pfd;
    INT i, j;

    num_pf = DescribePixelFormat(hdc, 0, 0, NULL);
    ok(num_pf > 0, "DescribePixelFormat failed.\n");

    for (pf = 1; pf <= num_pf; pf++)
    {
        ret = DescribePixelFormat(hdc, pf, sizeof(pfd), &pfd);
        ok(ret == num_pf, "Number of pixel formats changed!\n");

        if (pfd.dwFlags & PFD_GENERIC_FORMAT)
            break;
    }

    ok(pf < num_pf, "No generic pixel format!\n");

    /* First the formats compatible with the DC */
    for (i = 0; i < 3; i++)
    {
        /* Single buffered first */
        for (j = 0; j < ARRAY_SIZE(test_depths); j++)
        {
            test_format(hdc, num_pf, pf++, &colors[0][i], test_depths[j], TRUE, FALSE);
        }

        /* Then double buffered */
        for (j = 0; j < ARRAY_SIZE(test_depths); j++)
        {
            test_format(hdc, num_pf, pf++, &colors[0][i], test_depths[j], TRUE, TRUE);
        }
    }

    colors++;

    /* Then the rest */
    while (*colors)
    {
        for (i = 0; i < 3; i++)
        {
            for (j = 0; j < ARRAY_SIZE(test_depths); j++)
            {
                test_format(hdc, num_pf, pf++, &colors[0][i], test_depths[j], FALSE, FALSE);
            }
        }
        colors++;
    }

    /* We must have tested all generic formats. */
    ok(pf - 1 == num_pf, "We didn't test all generic formats : tested %u, total %u.\n", pf, num_pf);
}

static void test_bitmap_formats(const struct test_color ** colors)
{
    HBITMAP oldBmp, dib;
    HDC hdc;
    static const INT bpps [] = {4, 8, 16, 24, 32};
    INT i;
    void* dib_buffer;

    hdc = CreateCompatibleDC(NULL);
    ok (hdc != NULL, "Failed creating a memory DC.\n");

    for (i = 0; i < ARRAY_SIZE(bpps); i++)
    {
        dib = create_dib_section(bpps[i], &dib_buffer);
        ok(dib != NULL, "Failed to create DIB for %u bpp\n", bpps[i]);

        oldBmp = SelectObject(hdc, dib);
        ok (oldBmp != NULL, "Failed to select the DIB\n");

        trace("testing DIB %ubpp\n", bpps[i]);
        /* For mem DC, this is always the depth of the screen which is taken into account */
        test_bitmap_colors(hdc, colors);

        SelectObject(hdc, oldBmp);
        DeleteObject(dib);
    }
}

START_TEST(sw_pixelformat)
{
    DEVMODEW devMode;
    INT ret;
    DWORD orig_bpp;
    static const WORD bit_depths[] = {8, 16, 24, 32};
    static const struct test_color** colors[] = {test_colors_8bpp, test_colors_16bpp, test_colors_24bpp, test_colors_32bpp};
    INT i;

    ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devMode);
    ok (ret != 0, "EnumDisplaySettings failed.\n");
    orig_bpp = devMode.dmBitsPerPel;

    for (i = 0; i < ARRAY_SIZE(bit_depths); i++)
    {
        devMode.dmBitsPerPel = bit_depths[i];
        ret = ChangeDisplaySettingsExW(NULL, &devMode, NULL, 0, NULL);
        if (ret == DISP_CHANGE_SUCCESSFUL)
        {
            trace("Testing %ubpp\n", bit_depths[i]);
            test_screen_colors(colors[i]);
            test_bitmap_formats(colors[i]);
        }
        else
        {
            skip("Unable to switch to %ubpp\n", bit_depths[i]);
        }
    }

    devMode.dmBitsPerPel = orig_bpp;
    ret = ChangeDisplaySettingsExW(NULL, &devMode, NULL, 0, NULL);
}
