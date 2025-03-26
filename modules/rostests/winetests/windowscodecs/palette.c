/*
 * Copyright 2009 Vincent Povirk for CodeWeavers
 * Copyright 2012,2016 Dmitry Timoshkov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <assert.h>

#define COBJMACROS

#include "windef.h"
#include "objbase.h"
#include "wincodec.h"
#include "wine/test.h"

static IWICImagingFactory *factory;

static void test_custom_palette(void)
{
    IWICPalette *palette, *palette2;
    HRESULT hr;
    WICBitmapPaletteType type=0xffffffff;
    UINT count=1;
    const WICColor initcolors[4]={0xff000000,0xff0000ff,0xffffff00,0xffffffff};
    WICColor colors[4];
    BOOL boolresult;

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(SUCCEEDED(hr), "CreatePalette failed, hr=%lx\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IWICPalette_GetType(palette, &type);
        ok(SUCCEEDED(hr), "GetType failed, hr=%lx\n", hr);
        ok(type == WICBitmapPaletteTypeCustom, "expected WICBitmapPaletteTypeCustom, got %x\n", type);

        hr = IWICPalette_GetColorCount(palette, &count);
        ok(SUCCEEDED(hr), "GetColorCount failed, hr=%lx\n", hr);
        ok(count == 0, "expected 0, got %u\n", count);

        hr = IWICPalette_GetColors(palette, 0, colors, &count);
        ok(SUCCEEDED(hr), "GetColors failed, hr=%lx\n", hr);
        ok(count == 0, "expected 0, got %u\n", count);

        hr = IWICPalette_GetColors(palette, 4, colors, &count);
        ok(SUCCEEDED(hr), "GetColors failed, hr=%lx\n", hr);
        ok(count == 0, "expected 0, got %u\n", count);

        memcpy(colors, initcolors, sizeof(initcolors));
        hr = IWICPalette_InitializeCustom(palette, colors, 4);
        ok(SUCCEEDED(hr), "InitializeCustom failed, hr=%lx\n", hr);

        hr = IWICPalette_GetType(palette, &type);
        ok(SUCCEEDED(hr), "GetType failed, hr=%lx\n", hr);
        ok(type == WICBitmapPaletteTypeCustom, "expected WICBitmapPaletteTypeCustom, got %x\n", type);

        hr = IWICPalette_GetColorCount(palette, &count);
        ok(SUCCEEDED(hr), "GetColorCount failed, hr=%lx\n", hr);
        ok(count == 4, "expected 4, got %u\n", count);

        memset(colors, 0, sizeof(colors));
        count = 0;
        hr = IWICPalette_GetColors(palette, 4, colors, &count);
        ok(SUCCEEDED(hr), "GetColors failed, hr=%lx\n", hr);
        ok(count == 4, "expected 4, got %u\n", count);
        ok(!memcmp(colors, initcolors, sizeof(colors)), "got unexpected palette data\n");

        memset(colors, 0, sizeof(colors));
        count = 0;
        hr = IWICPalette_GetColors(palette, 2, colors, &count);
        ok(SUCCEEDED(hr), "GetColors failed, hr=%lx\n", hr);
        ok(count == 2, "expected 2, got %u\n", count);
        ok(!memcmp(colors, initcolors, sizeof(WICColor)*2), "got unexpected palette data\n");

        count = 0;
        hr = IWICPalette_GetColors(palette, 6, colors, &count);
        ok(SUCCEEDED(hr), "GetColors failed, hr=%lx\n", hr);
        ok(count == 4, "expected 4, got %u\n", count);

        hr = IWICPalette_HasAlpha(palette, &boolresult);
        ok(SUCCEEDED(hr), "HasAlpha failed, hr=%lx\n", hr);
        ok(!boolresult, "expected FALSE, got TRUE\n");

        hr = IWICPalette_IsBlackWhite(palette, &boolresult);
        ok(SUCCEEDED(hr), "IsBlackWhite failed, hr=%lx\n", hr);
        ok(!boolresult, "expected FALSE, got TRUE\n");

        hr = IWICPalette_IsGrayscale(palette, &boolresult);
        ok(SUCCEEDED(hr), "IsGrayscale failed, hr=%lx\n", hr);
        ok(!boolresult, "expected FALSE, got TRUE\n");

        hr = IWICImagingFactory_CreatePalette(factory, &palette2);
        ok(SUCCEEDED(hr), "CreatePalette failed, hr=%lx\n", hr);

        hr = IWICPalette_InitializeFromPalette(palette2, palette);
        ok(SUCCEEDED(hr), "InitializeFromPalette failed, hr=%lx\n", hr);

        type = 0xdeadbeef;
        hr = IWICPalette_GetType(palette2, &type);
        ok(SUCCEEDED(hr), "GetType failed, hr=%lx\n", hr);
        ok(type == WICBitmapPaletteTypeCustom, "expected WICBitmapPaletteTypeCustom, got %x\n", type);

        count = 0xdeadbeef;
        hr = IWICPalette_GetColorCount(palette2, &count);
        ok(SUCCEEDED(hr), "GetColorCount failed, hr=%lx\n", hr);
        ok(count == 4, "expected 4, got %u\n", count);

        memset(colors, 0, sizeof(colors));
        count = 0xdeadbeef;
        hr = IWICPalette_GetColors(palette2, 4, colors, &count);
        ok(SUCCEEDED(hr), "GetColors failed, hr=%lx\n", hr);
        ok(count == 4, "expected 4, got %u\n", count);
        ok(!memcmp(colors, initcolors, sizeof(colors)), "got unexpected palette data\n");

        /* try a palette with some alpha in it */
        colors[2] = 0x80ffffff;
        hr = IWICPalette_InitializeCustom(palette, colors, 4);
        ok(SUCCEEDED(hr), "InitializeCustom failed, hr=%lx\n", hr);

        hr = IWICPalette_HasAlpha(palette, &boolresult);
        ok(SUCCEEDED(hr), "HasAlpha failed, hr=%lx\n", hr);
        ok(boolresult, "expected TRUE, got FALSE\n");

        /* setting to a 0-color palette is acceptable */
        hr = IWICPalette_InitializeCustom(palette, NULL, 0);
        ok(SUCCEEDED(hr), "InitializeCustom failed, hr=%lx\n", hr);

        type = 0xdeadbeef;
        hr = IWICPalette_GetType(palette, &type);
        ok(SUCCEEDED(hr), "GetType failed, hr=%lx\n", hr);
        ok(type == WICBitmapPaletteTypeCustom, "expected WICBitmapPaletteTypeCustom, got %x\n", type);

        count = 0xdeadbeef;
        hr = IWICPalette_GetColorCount(palette, &count);
        ok(SUCCEEDED(hr), "GetColorCount failed, hr=%lx\n", hr);
        ok(count == 0, "expected 0, got %u\n", count);

        count = 0xdeadbeef;
        hr = IWICPalette_GetColors(palette, 4, colors, &count);
        ok(SUCCEEDED(hr), "GetColors failed, hr=%lx\n", hr);
        ok(count == 0, "expected 0, got %u\n", count);

        hr = IWICPalette_InitializeFromPalette(palette2, palette);
        ok(SUCCEEDED(hr), "InitializeFromPalette failed, hr=%lx\n", hr);

        type = 0xdeadbeef;
        hr = IWICPalette_GetType(palette2, &type);
        ok(SUCCEEDED(hr), "GetType failed, hr=%lx\n", hr);
        ok(type == WICBitmapPaletteTypeCustom, "expected WICBitmapPaletteTypeCustom, got %x\n", type);

        count = 0xdeadbeef;
        hr = IWICPalette_GetColorCount(palette2, &count);
        ok(SUCCEEDED(hr), "GetColorCount failed, hr=%lx\n", hr);
        ok(count == 0, "expected 0, got %u\n", count);

        memset(colors, 0, sizeof(colors));
        count = 0xdeadbeef;
        hr = IWICPalette_GetColors(palette2, 4, colors, &count);
        ok(SUCCEEDED(hr), "GetColors failed, hr=%lx\n", hr);
        ok(count == 0, "expected 0, got %u\n", count);

        /* IWICPalette is paranoid about NULL pointers */
        hr = IWICPalette_GetType(palette, NULL);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %lx\n", hr);

        hr = IWICPalette_GetColorCount(palette, NULL);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %lx\n", hr);

        hr = IWICPalette_InitializeCustom(palette, NULL, 4);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %lx\n", hr);

        hr = IWICPalette_GetColors(palette, 4, NULL, &count);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %lx\n", hr);

        hr = IWICPalette_GetColors(palette, 4, colors, NULL);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %lx\n", hr);

        hr = IWICPalette_HasAlpha(palette, NULL);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %lx\n", hr);

        hr = IWICPalette_IsBlackWhite(palette, NULL);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %lx\n", hr);

        hr = IWICPalette_IsGrayscale(palette, NULL);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %lx\n", hr);

        hr = IWICPalette_InitializeFromPalette(palette, NULL);
        ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %lx\n", hr);

        IWICPalette_Release(palette2);
        IWICPalette_Release(palette);
    }
}

static void generate_gray16_palette(WICColor *entries, UINT count)
{
    UINT i;

    assert(count == 16);

    for (i = 0; i < 16; i++)
    {
        entries[i] = 0xff000000;
        entries[i] |= (i << 20) | (i << 16) | (i << 12) | (i << 8) | (i << 4) | i;
    }
}

static void generate_gray256_palette(WICColor *entries, UINT count)
{
    UINT i;

    assert(count == 256);

    for (i = 0; i < 256; i++)
    {
        entries[i] = 0xff000000;
        entries[i] |= (i << 16) | (i << 8) | i;
    }
}

static void generate_halftone8_palette(WICColor *entries, UINT count, BOOL add_transparent)
{
    UINT i;

    if (add_transparent)
        ok(count == 17, "expected 17, got %u\n", count);
    else
        ok(count == 16, "expected 16, got %u\n", count);

    for (i = 0; i < 8; i++)
    {
        entries[i] = 0xff000000;
        if (i & 1) entries[i] |= 0xff;
        if (i & 2) entries[i] |= 0xff00;
        if (i & 4) entries[i] |= 0xff0000;
    }

    for (i = 8; i < 16; i++)
    {
        static const DWORD halftone[8] = { 0xc0c0c0, 0x808080, 0x800000, 0x008000,
                                           0x000080, 0x808000, 0x800080, 0x008080 };
        entries[i] = 0xff000000;
        entries[i] |= halftone[i-8];
    }

    if (add_transparent)
        entries[i] = 0;
}

static void generate_halftone27_palette(WICColor *entries, UINT count, BOOL add_transparent)
{
    static const BYTE halftone_values[4] = { 0x00,0x80,0xff };
    UINT i;

    if (add_transparent)
        ok(count == 29, "expected 29, got %u\n", count);
    else
        ok(count == 28, "expected 28, got %u\n", count);

    for (i = 0; i < 27; i++)
    {
        entries[i] = 0xff000000;
        entries[i] |= halftone_values[i%3];
        entries[i] |= halftone_values[(i/3)%3] << 8;
        entries[i] |= halftone_values[(i/9)%3] << 16;
    }

    entries[i++] = 0xffc0c0c0;
    if (add_transparent)
        entries[i] = 0;
}

static void generate_halftone64_palette(WICColor *entries, UINT count, BOOL add_transparent)
{
    static const BYTE halftone_values[4] = { 0x00,0x55,0xaa,0xff };
    UINT i;

    if (add_transparent)
        ok(count == 73, "expected 73, got %u\n", count);
    else
        ok(count == 72, "expected 72, got %u\n", count);

    for (i = 0; i < 64; i++)
    {
        entries[i] = 0xff000000;
        entries[i] |= halftone_values[i%4];
        entries[i] |= halftone_values[(i/4)%4] << 8;
        entries[i] |= halftone_values[(i/16)%4] << 16;
    }

    for (i = 64; i < 72; i++)
    {
        static const DWORD halftone[8] = { 0xc0c0c0, 0x808080, 0x800000, 0x008000,
                                           0x000080, 0x808000, 0x800080, 0x008080 };
        entries[i] = 0xff000000;
        entries[i] |= halftone[i-64];
    }

    if (add_transparent)
        entries[i] = 0;
}

static void generate_halftone125_palette(WICColor *entries, UINT count, BOOL add_transparent)
{
    static const BYTE halftone_values[5] = { 0x00, 0x40, 0x80, 0xbf, 0xff };
    UINT i;

    if (add_transparent)
        ok(count == 127, "expected 127, got %u\n", count);
    else
        ok(count == 126, "expected 126, got %u\n", count);

    for (i = 0; i < 125; i++)
    {
        entries[i] = 0xff000000;
        entries[i] |= halftone_values[i%5];
        entries[i] |= halftone_values[(i/5)%5] << 8;
        entries[i] |= halftone_values[(i/25)%5] << 16;
    }

    entries[i++] = 0xffc0c0c0;
    if (add_transparent)
        entries[i] = 0;
}

static void generate_halftone216_palette(WICColor *entries, UINT count, BOOL add_transparent)
{
    static const BYTE halftone_values[6] = { 0x00,0x33,0x66,0x99,0xcc,0xff };
    UINT i;

    if (add_transparent)
        ok(count == 225, "expected 225, got %u\n", count);
    else
        ok(count == 224, "expected 224, got %u\n", count);

    for (i = 0; i < 216; i++)
    {
        entries[i] = 0xff000000;
        entries[i] |= halftone_values[i%6];
        entries[i] |= halftone_values[(i/6)%6] << 8;
        entries[i] |= halftone_values[(i/36)%6] << 16;
    }

    for (i = 216; i < 224; i++)
    {
        static const DWORD halftone[8] = { 0xc0c0c0, 0x808080, 0x800000, 0x008000,
                                           0x000080, 0x808000, 0x800080, 0x008080 };
        entries[i] = 0xff000000;
        entries[i] |= halftone[i-216];
    }

    if (add_transparent)
        entries[i] = 0;
}

static void generate_halftone252_palette(WICColor *entries, UINT count, BOOL add_transparent)
{
    static const BYTE halftone_values_rb[6] = { 0x00,0x33,0x66,0x99,0xcc,0xff };
    static const BYTE halftone_values_g[7] = { 0x00,0x2b,0x55,0x80,0xaa,0xd5,0xff };
    UINT i;

    if (add_transparent)
        ok(count == 253, "expected 253, got %u\n", count);
    else
        ok(count == 252, "expected 252, got %u\n", count);

    for (i = 0; i < 252; i++)
    {
        entries[i] = 0xff000000;
        entries[i] |= halftone_values_rb[i%6];
        entries[i] |= halftone_values_g[(i/6)%7] << 8;
        entries[i] |= halftone_values_rb[(i/42)%6] << 16;
    }

    if (add_transparent)
        entries[i] = 0;
}

static void generate_halftone256_palette(WICColor *entries, UINT count, BOOL add_transparent)
{
    static const BYTE halftone_values_b[4] = { 0x00,0x55,0xaa,0xff };
    static const BYTE halftone_values_gr[8] = { 0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff };
    UINT i;

    assert(count == 256);

    for (i = 0; i < 256; i++)
    {
        entries[i] = 0xff000000;
        entries[i] |= halftone_values_b[i%4];
        entries[i] |= halftone_values_gr[(i/4)%8] << 8;
        entries[i] |= halftone_values_gr[(i/32)%8] << 16;
    }

    if (add_transparent)
        entries[255] = 0;
}

static void test_predefined_palette(void)
{
    static struct test_data
    {
        WICBitmapPaletteType type;
        BOOL is_bw, is_gray;
        UINT count;
        WICColor color[256];
        BOOL add_transparent;
    } td[] =
    {
        { WICBitmapPaletteTypeFixedBW, 1, 1, 2, { 0xff000000, 0xffffffff } },
        { WICBitmapPaletteTypeFixedBW, 1, 1, 2, { 0xff000000, 0xffffffff }, 1 },
        { WICBitmapPaletteTypeFixedGray4, 0, 1, 4,
          { 0xff000000, 0xff555555, 0xffaaaaaa, 0xffffffff } },
        { WICBitmapPaletteTypeFixedGray4, 0, 1, 4,
          { 0xff000000, 0xff555555, 0xffaaaaaa, 0xffffffff }, 1 },
        { WICBitmapPaletteTypeFixedGray16, 0, 1, 16, { 0 } },
        { WICBitmapPaletteTypeFixedGray16, 0, 1, 16, { 0 }, 1 },
        { WICBitmapPaletteTypeFixedGray256, 0, 1, 256, { 0 } },
        { WICBitmapPaletteTypeFixedGray256, 0, 1, 256, { 0 }, 1 },
        { WICBitmapPaletteTypeFixedHalftone8, 0, 0, 16, { 0 } },
        { WICBitmapPaletteTypeFixedHalftone8, 0, 0, 17, { 0 }, 1 },
        { WICBitmapPaletteTypeFixedHalftone27, 0, 0, 28, { 0 } },
        { WICBitmapPaletteTypeFixedHalftone27, 0, 0, 29, { 0 }, 1 },
        { WICBitmapPaletteTypeFixedHalftone64, 0, 0, 72, { 0 } },
        { WICBitmapPaletteTypeFixedHalftone64, 0, 0, 73, { 0 }, 1 },
        { WICBitmapPaletteTypeFixedHalftone125, 0, 0, 126, { 0 } },
        { WICBitmapPaletteTypeFixedHalftone125, 0, 0, 127, { 0 }, 1 },
        { WICBitmapPaletteTypeFixedHalftone216, 0, 0, 224, { 0 } },
        { WICBitmapPaletteTypeFixedHalftone216, 0, 0, 225, { 0 }, 1 },
        { WICBitmapPaletteTypeFixedHalftone252, 0, 0, 252, { 0 } },
        { WICBitmapPaletteTypeFixedHalftone252, 0, 0, 253, { 0 }, 1 },
        { WICBitmapPaletteTypeFixedHalftone256, 0, 0, 256, { 0 } },
        { WICBitmapPaletteTypeFixedHalftone256, 0, 0, 256, { 0 }, 1 }
    };
    IWICPalette *palette;
    HRESULT hr;
    WICBitmapPaletteType type;
    UINT count, i, ret;
    BOOL bret;
    WICColor color[256];

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "CreatePalette error %#lx\n", hr);
    hr = IWICPalette_InitializePredefined(palette, WICBitmapPaletteTypeCustom, FALSE);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);
    hr = IWICPalette_InitializePredefined(palette, WICBitmapPaletteTypeMedianCut, FALSE);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);
    hr = IWICPalette_InitializePredefined(palette, 0x0f, FALSE);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);
    IWICPalette_Release(palette);

    for (i = 0; i < ARRAY_SIZE(td); i++)
    {
        hr = IWICImagingFactory_CreatePalette(factory, &palette);
        ok(hr == S_OK, "%u: CreatePalette error %#lx\n", i, hr);

        hr = IWICPalette_InitializePredefined(palette, td[i].type, td[i].add_transparent);
        ok(hr == S_OK, "%u: InitializePredefined error %#lx\n", i, hr);

        bret = -1;
        hr = IWICPalette_IsBlackWhite(palette, &bret);
        ok(hr == S_OK, "%u: IsBlackWhite error %#lx\n", i, hr);
        ok(bret == td[i].is_bw ||
           broken(td[i].type == WICBitmapPaletteTypeFixedBW && bret != td[i].is_bw), /* XP */
           "%u: expected %d, got %d\n",i, td[i].is_bw, bret);

        bret = -1;
        hr = IWICPalette_IsGrayscale(palette, &bret);
        ok(hr == S_OK, "%u: IsGrayscale error %#lx\n", i, hr);
        ok(bret == td[i].is_gray, "%u: expected %d, got %d\n", i, td[i].is_gray, bret);

        type = -1;
        hr = IWICPalette_GetType(palette, &type);
        ok(hr == S_OK, "%u: GetType error %#lx\n", i, hr);
        ok(type == td[i].type, "%u: expected %#x, got %#x\n", i, td[i].type, type);

        count = 0xdeadbeef;
        hr = IWICPalette_GetColorCount(palette, &count);
        ok(hr == S_OK, "%u: GetColorCount error %#lx\n", i, hr);
        ok(count == td[i].count, "%u: expected %u, got %u\n", i, td[i].count, count);

        hr = IWICPalette_GetColors(palette, count, color, &ret);
        ok(hr == S_OK, "%u: GetColors error %#lx\n", i, hr);
        ok(ret == count, "%u: expected %u, got %u\n", i, count, ret);
        if (ret == td[i].count)
        {
            UINT j;

            if (td[i].type == WICBitmapPaletteTypeFixedGray16)
                generate_gray16_palette(td[i].color, td[i].count);
            else if (td[i].type == WICBitmapPaletteTypeFixedGray256)
                generate_gray256_palette(td[i].color, td[i].count);
            else if (td[i].type == WICBitmapPaletteTypeFixedHalftone8)
                generate_halftone8_palette(td[i].color, td[i].count, td[i].add_transparent);
            else if (td[i].type == WICBitmapPaletteTypeFixedHalftone27)
                generate_halftone27_palette(td[i].color, td[i].count, td[i].add_transparent);
            else if (td[i].type == WICBitmapPaletteTypeFixedHalftone64)
                generate_halftone64_palette(td[i].color, td[i].count, td[i].add_transparent);
            else if (td[i].type == WICBitmapPaletteTypeFixedHalftone125)
                generate_halftone125_palette(td[i].color, td[i].count, td[i].add_transparent);
            else if (td[i].type == WICBitmapPaletteTypeFixedHalftone216)
                generate_halftone216_palette(td[i].color, td[i].count, td[i].add_transparent);
            else if (td[i].type == WICBitmapPaletteTypeFixedHalftone252)
                generate_halftone252_palette(td[i].color, td[i].count, td[i].add_transparent);
            else if (td[i].type == WICBitmapPaletteTypeFixedHalftone256)
                generate_halftone256_palette(td[i].color, td[i].count, td[i].add_transparent);

            for (j = 0; j < count; j++)
            {
                ok(color[j] == td[i].color[j], "%u:[%u]: expected %#x, got %#x\n",
                   i, j, td[i].color[j], color[j]);
            }
        }

        IWICPalette_Release(palette);
    }
}

static BYTE *init_bitmap(UINT *width, UINT *height, UINT *stride)
{
    BYTE *src;
    UINT i, j, scale;

    *width = 256;
    *height = 256;
    *stride = (*width * 3 + 3) & ~3;
    trace("width %d, height %d, stride %d\n", *width, *height, *stride);

    src = HeapAlloc(GetProcessHeap(), 0, *stride * *height);

    scale = 256 / *width;
    if (!scale) scale = 1;

    for (i = 0; i < *height; i++)
    {
        for (j = 0; j < *width; j++)
        {
            src[i * *stride + j*3 + 0] = scale * i;
            src[i * *stride + j*3 + 1] = scale * (255 - (i+j)/2);
            src[i * *stride + j*3 + 2] = scale * j;
        }
    }

    return src;
}

static void test_palette_from_bitmap(void)
{
    HRESULT hr;
    BYTE *data;
    IWICBitmap *bitmap;
    IWICPalette *palette;
    WICBitmapPaletteType type;
    UINT width, height, stride, count, ret;
    WICColor color[257];

    data = init_bitmap(&width, &height, &stride);

    hr = IWICImagingFactory_CreateBitmapFromMemory(factory, width, height, &GUID_WICPixelFormat24bppRGB,
                                                   stride, stride * height, data, &bitmap);
    ok(hr == S_OK, "CreateBitmapFromMemory error %#lx\n", hr);

    hr = IWICImagingFactory_CreatePalette(factory, &palette);
    ok(hr == S_OK, "CreatePalette error %#lx\n", hr);

    hr = IWICPalette_InitializeFromBitmap(palette, (IWICBitmapSource *)bitmap, 0, FALSE);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

    hr = IWICPalette_InitializeFromBitmap(palette, (IWICBitmapSource *)bitmap, 1, FALSE);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

    hr = IWICPalette_InitializeFromBitmap(palette, (IWICBitmapSource *)bitmap, 257, FALSE);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

    hr = IWICPalette_InitializeFromBitmap(palette, NULL, 16, FALSE);
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG, got %#lx\n", hr);

    hr = IWICPalette_InitializeFromBitmap(palette, (IWICBitmapSource *)bitmap, 2, FALSE);
    ok(hr == S_OK, "InitializeFromBitmap error %#lx\n", hr);
    count = 0;
    hr = IWICPalette_GetColorCount(palette, &count);
    ok(hr == S_OK, "GetColorCount error %#lx\n", hr);
    ok(count == 2, "expected 2, got %u\n", count);

    hr = IWICPalette_InitializeFromBitmap(palette, (IWICBitmapSource *)bitmap, 2, TRUE);
    ok(hr == S_OK, "InitializeFromBitmap error %#lx\n", hr);
    count = 0;
    hr = IWICPalette_GetColorCount(palette, &count);
    ok(hr == S_OK, "GetColorCount error %#lx\n", hr);
    ok(count == 2, "expected 2, got %u\n", count);

    /* without transparent color */
    hr = IWICPalette_InitializeFromBitmap(palette, (IWICBitmapSource *)bitmap, 16, FALSE);
    ok(hr == S_OK, "InitializeFromBitmap error %#lx\n", hr);
    type = -1;
    hr = IWICPalette_GetType(palette, &type);
    ok(hr == S_OK, "GetType error %#lx\n", hr);
    ok(type == WICBitmapPaletteTypeCustom, "expected WICBitmapPaletteTypeCustom, got %#x\n", type);
    count = 0;
    hr = IWICPalette_GetColorCount(palette, &count);
    ok(hr == S_OK, "GetColorCount error %#lx\n", hr);
    ok(count == 16, "expected 16, got %u\n", count);
    memset(color, 0, sizeof(color));
    hr = IWICPalette_GetColors(palette, count, color, &ret);
    ok(hr == S_OK, "GetColors error %#lx\n", hr);
    ok(ret == count, "expected %u, got %u\n", count, ret);
    ok(color[count - 1] != 0, "expected !0, got %08x\n", color[count - 1]);

    /* with transparent color */
    hr = IWICPalette_InitializeFromBitmap(palette, (IWICBitmapSource *)bitmap, 16, TRUE);
    ok(hr == S_OK, "InitializeFromBitmap error %#lx\n", hr);
    type = -1;
    hr = IWICPalette_GetType(palette, &type);
    ok(hr == S_OK, "GetType error %#lx\n", hr);
    ok(type == WICBitmapPaletteTypeCustom, "expected WICBitmapPaletteTypeCustom, got %#x\n", type);
    count = 0;
    hr = IWICPalette_GetColorCount(palette, &count);
    ok(hr == S_OK, "GetColorCount error %#lx\n", hr);
    ok(count == 16, "expected 16, got %u\n", count);
    memset(color, 0xff, sizeof(color));
    hr = IWICPalette_GetColors(palette, count, color, &ret);
    ok(hr == S_OK, "GetColors error %#lx\n", hr);
    ok(ret == count, "expected %u, got %u\n", count, ret);
    ok(color[count - 1] == 0, "expected 0, got %08x\n", color[count - 1]);

    IWICPalette_Release(palette);
    IWICBitmap_Release(bitmap);

    HeapFree(GetProcessHeap(), 0, data);
}

START_TEST(palette)
{
    HRESULT hr;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICImagingFactory, (void **)&factory);
    ok(hr == S_OK, "CoCreateInstance error %#lx\n", hr);

    test_custom_palette();
    test_predefined_palette();
    test_palette_from_bitmap();

    IWICImagingFactory_Release(factory);

    CoUninitialize();
}
