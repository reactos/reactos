/*
 * Tests for the D3DX9 texture functions
 *
 * Copyright 2009 Tony Wasserka
 * Copyright 2010 Owen Rudge for CodeWeavers
 * Copyright 2010 Matteo Bruni for CodeWeavers
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

#define COBJMACROS
#include "wine/test.h"
#include "d3dx9tex.h"
#include "resources.h"

static int has_2d_dxt1, has_2d_dxt3, has_2d_dxt5, has_cube_dxt5, has_3d_dxt3;

/* 2x2 16-bit dds, no mipmaps */
static const unsigned char dds_16bit[] = {
0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x08,0x00,0x02,0x00,0x00,0x00,
0x02,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x7c,0x00,0x00,
0xe0,0x03,0x00,0x00,0x1f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0xff,0x7f,0xff,0x7f,0xff,0x7f,0xff,0x7f
};

/* 2x2 24-bit dds, 2 mipmaps */
static const unsigned char dds_24bit[] = {
0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x0a,0x00,0x02,0x00,0x00,0x00,
0x02,0x00,0x00,0x00,0x0c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0xff,0x00,
0x00,0xff,0x00,0x00,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x40,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
};

/* 4x4 cube map dds */
static const unsigned char dds_cube_map[] = {
0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x08,0x00,0x04,0x00,0x00,0x00,
0x04,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
0x04,0x00,0x00,0x00,0x44,0x58,0x54,0x35,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x00,0x00,
0x00,0xfe,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50,
0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x51,
0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x52,
0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x53,
0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x54,
0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x55
};

/* 4x4x2 volume map dds, 2 mipmaps */
static const unsigned char dds_volume_map[] = {
0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x8a,0x00,0x04,0x00,0x00,0x00,
0x04,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
0x04,0x00,0x00,0x00,0x44,0x58,0x54,0x33,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x40,0x00,
0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50,
0xff,0x00,0xff,0x00,0x00,0x00,0x00,0x00,0x2f,0x7e,0xcf,0x79,0x01,0x54,0x5c,0x5c,
0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x84,0xef,0x7b,0xaa,0xab,0xab,0xab
};

/* 4x2 dxt5 */
static const BYTE dds_dxt5[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x08,0x00,0x02,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x44,0x58,0x54,0x35,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50,
};

static const BYTE dds_dxt5_8_8[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x08,0x00,0x08,0x00,0x00,0x00,
    0x08,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x44,0x58,0x54,0x35,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x1f,0x00,0xe0,0x07,0x05,0x05,0x50,0x50,
    0x3f,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xf8,0xff,0x07,0x05,0x05,0x50,0x50,
    0x7f,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x1f,0xf8,0xe0,0xff,0x05,0x05,0x50,0x50,
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x00,0x00,0x05,0x05,0x50,0x50,
};

static const unsigned char png_grayscale[] =
{
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49,
    0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x00,
    0x00, 0x00, 0x00, 0x3a, 0x7e, 0x9b, 0x55, 0x00, 0x00, 0x00, 0x0a, 0x49, 0x44,
    0x41, 0x54, 0x08, 0xd7, 0x63, 0xf8, 0x0f, 0x00, 0x01, 0x01, 0x01, 0x00, 0x1b,
    0xb6, 0xee, 0x56, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42,
    0x60, 0x82
};

#define ADMITTED_ERROR 0.0001f

static inline float relative_error(float expected, float got)
{
    return expected == 0.0f ? fabs(expected - got) : fabs(1.0f - got / expected);
}

#define expect_vec4(expected, got) expect_vec4_(__LINE__, expected, got)
static inline void expect_vec4_(unsigned int line, const D3DXVECTOR4 *expected, const D3DXVECTOR4 *got)
{
    ok_(__FILE__, line)(relative_error(expected->x, got->x) < ADMITTED_ERROR
        && relative_error(expected->y, got->y) < ADMITTED_ERROR
        && relative_error(expected->z, got->z) < ADMITTED_ERROR
        && relative_error(expected->w, got->w) < ADMITTED_ERROR,
        "Expected (%f, %f, %f, %f), got (%f, %f, %f, %f)\n",
        expected->x, expected->y, expected->z, expected->w,
        got->x, got->y, got->z, got->w);
}

static BOOL compare_uint(unsigned int x, unsigned int y, unsigned int max_diff)
{
    unsigned int diff = x > y ? x - y : y - x;

    return diff <= max_diff;
}

static BOOL compare_color(DWORD c1, DWORD c2, BYTE max_diff)
{
    return compare_uint(c1 & 0xff, c2 & 0xff, max_diff)
            && compare_uint((c1 >> 8) & 0xff, (c2 >> 8) & 0xff, max_diff)
            && compare_uint((c1 >> 16) & 0xff, (c2 >> 16) & 0xff, max_diff)
            && compare_uint((c1 >> 24) & 0xff, (c2 >> 24) & 0xff, max_diff);
}

static BOOL is_autogenmipmap_supported(IDirect3DDevice9 *device, D3DRESOURCETYPE resource_type)
{
    HRESULT hr;
    D3DCAPS9 caps;
    IDirect3D9 *d3d9;
    D3DDISPLAYMODE mode;
    D3DDEVICE_CREATION_PARAMETERS params;

    IDirect3DDevice9_GetDeviceCaps(device, &caps);
    IDirect3DDevice9_GetDirect3D(device, &d3d9);
    IDirect3DDevice9_GetCreationParameters(device, &params);
    IDirect3DDevice9_GetDisplayMode(device, 0, &mode);

    if (!(caps.Caps2 & D3DCAPS2_CANAUTOGENMIPMAP))
        return FALSE;

    hr = IDirect3D9_CheckDeviceFormat(d3d9, params.AdapterOrdinal, params.DeviceType,
        mode.Format, D3DUSAGE_AUTOGENMIPMAP, resource_type, D3DFMT_A8R8G8B8);

    IDirect3D9_Release(d3d9);
    return hr == D3D_OK;
}

static void test_D3DXCheckTextureRequirements(IDirect3DDevice9 *device)
{
    UINT width, height, mipmaps;
    D3DFORMAT format, expected;
    D3DCAPS9 caps;
    HRESULT hr;
    IDirect3D9 *d3d;
    D3DDEVICE_CREATION_PARAMETERS params;
    D3DDISPLAYMODE mode;

    IDirect3DDevice9_GetDeviceCaps(device, &caps);

    /* general tests */
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);

    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, D3DX_DEFAULT, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);

    hr = D3DXCheckTextureRequirements(NULL, NULL, NULL, NULL, D3DX_DEFAULT, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    /* width & height */
    width = height = D3DX_DEFAULT;
    hr = D3DXCheckTextureRequirements(device, &width, &height, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(width == 256, "Returned width %d, expected %d\n", width, 256);
    ok(height == 256, "Returned height %d, expected %d\n", height, 256);

    width = D3DX_DEFAULT;
    hr = D3DXCheckTextureRequirements(device, &width, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(width == 256, "Returned width %d, expected %d\n", width, 256);

    if (caps.TextureCaps & D3DPTEXTURECAPS_POW2)
        skip("Hardware only supports pow2 textures\n");
    else
    {
        width = 62;
        hr = D3DXCheckTextureRequirements(device, &width, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
        ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
        ok(width == 62, "Returned width %d, expected %d\n", width, 62);

        width = D3DX_DEFAULT; height = 63;
        hr = D3DXCheckTextureRequirements(device, &width, &height, NULL, 0, NULL, D3DPOOL_DEFAULT);
        ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
        ok(width == height, "Returned width %d, expected %d\n", width, height);
        ok(height == 63, "Returned height %d, expected %d\n", height, 63);
    }

    width = D3DX_DEFAULT; height = 0;
    hr = D3DXCheckTextureRequirements(device, &width, &height, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(width == 1, "Returned width %d, expected %d\n", width, 1);
    ok(height == 1, "Returned height %d, expected %d\n", height, 1);

    width = 0; height = 0;
    hr = D3DXCheckTextureRequirements(device, &width, &height, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(width == 1, "Returned width %d, expected %d\n", width, 1);
    ok(height == 1, "Returned height %d, expected %d\n", height, 1);

    width = 0;
    hr = D3DXCheckTextureRequirements(device, &width, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(width == 1, "Returned width %d, expected %d\n", width, 1);

    width = 0xFFFFFFFE;
    hr = D3DXCheckTextureRequirements(device, &width, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(width == caps.MaxTextureWidth, "Returned width %d, expected %d\n", width, caps.MaxTextureWidth);

    width = caps.MaxTextureWidth-1;
    hr = D3DXCheckTextureRequirements(device, &width, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    if (caps.TextureCaps & D3DPTEXTURECAPS_POW2)
        ok(width == caps.MaxTextureWidth, "Returned width %d, expected %d\n", width, caps.MaxTextureWidth);
    else
        ok(width == caps.MaxTextureWidth-1, "Returned width %d, expected %d\n", width, caps.MaxTextureWidth-1);

    /* mipmaps */
    width = 64; height = 63;
    mipmaps = 9;
    hr = D3DXCheckTextureRequirements(device, &width, &height, &mipmaps, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(mipmaps == 7, "Returned mipmaps %d, expected %d\n", mipmaps, 7);

    if (!(caps.TextureCaps & D3DPTEXTURECAPS_POW2))
    {
        width = 284; height = 137;
        mipmaps = 20;
        hr = D3DXCheckTextureRequirements(device, &width, &height, &mipmaps, 0, NULL, D3DPOOL_DEFAULT);
        ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
        ok(mipmaps == 9, "Returned mipmaps %d, expected %d\n", mipmaps, 9);

        width = height = 63;
        mipmaps = 9;
        hr = D3DXCheckTextureRequirements(device, &width, &height, &mipmaps, 0, NULL, D3DPOOL_DEFAULT);
        ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
        ok(mipmaps == 6, "Returned mipmaps %d, expected %d\n", mipmaps, 6);
    }
    else
        skip("Skipping some tests, npot2 textures unsupported\n");

    mipmaps = 20;
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, &mipmaps, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(mipmaps == 9, "Returned mipmaps %d, expected %d\n", mipmaps, 9);

    mipmaps = 0;
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, &mipmaps, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(mipmaps == 9, "Returned mipmaps %d, expected %d\n", mipmaps, 9);

    /* mipmaps when D3DUSAGE_AUTOGENMIPMAP is set */
    if (is_autogenmipmap_supported(device, D3DRTYPE_TEXTURE))
    {
        mipmaps = 0;
        hr = D3DXCheckTextureRequirements(device, NULL, NULL, &mipmaps, D3DUSAGE_AUTOGENMIPMAP, NULL, D3DPOOL_DEFAULT);
        ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
        ok(mipmaps == 0, "Returned mipmaps %d, expected %d\n", mipmaps, 0);
        mipmaps = 1;
        hr = D3DXCheckTextureRequirements(device, NULL, NULL, &mipmaps, D3DUSAGE_AUTOGENMIPMAP, NULL, D3DPOOL_DEFAULT);
        ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
        ok(mipmaps == 1, "Returned mipmaps %d, expected %d\n", mipmaps, 1);
        mipmaps = 2;
        hr = D3DXCheckTextureRequirements(device, NULL, NULL, &mipmaps, D3DUSAGE_AUTOGENMIPMAP, NULL, D3DPOOL_DEFAULT);
        ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
        ok(mipmaps == 0, "Returned mipmaps %d, expected %d\n", mipmaps, 0);
        mipmaps = 6;
        hr = D3DXCheckTextureRequirements(device, NULL, NULL, &mipmaps, D3DUSAGE_AUTOGENMIPMAP, NULL, D3DPOOL_DEFAULT);
        ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
        ok(mipmaps == 0, "Returned mipmaps %d, expected %d\n", mipmaps, 0);
    }
    else
        skip("No D3DUSAGE_AUTOGENMIPMAP support for textures\n");

    /* usage */
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, D3DUSAGE_WRITEONLY, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCheckTextureRequirements succeeded, but should've failed.\n");
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, D3DUSAGE_DONOTCLIP, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCheckTextureRequirements succeeded, but should've failed.\n");
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, D3DUSAGE_POINTS, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCheckTextureRequirements succeeded, but should've failed.\n");
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, D3DUSAGE_RTPATCHES, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCheckTextureRequirements succeeded, but should've failed.\n");
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, D3DUSAGE_NPATCHES, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCheckTextureRequirements succeeded, but should've failed.\n");

    /* format */
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);

    format = D3DFMT_UNKNOWN;
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(format == D3DFMT_A8R8G8B8, "Returned format %u, expected %u\n", format, D3DFMT_A8R8G8B8);

    format = D3DX_DEFAULT;
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(format == D3DFMT_A8R8G8B8, "Returned format %u, expected %u\n", format, D3DFMT_A8R8G8B8);

    format = D3DFMT_R8G8B8;
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(format == D3DFMT_X8R8G8B8, "Returned format %u, expected %u\n", format, D3DFMT_X8R8G8B8);

    IDirect3DDevice9_GetDirect3D(device, &d3d);
    IDirect3DDevice9_GetCreationParameters(device, &params);
    IDirect3DDevice9_GetDisplayMode(device, 0, &mode);

    if (SUCCEEDED(IDirect3D9_CheckDeviceFormat(d3d, params.AdapterOrdinal, params.DeviceType,
                                               mode.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_R3G3B2)))
        expected = D3DFMT_R3G3B2;
    else if (SUCCEEDED(IDirect3D9_CheckDeviceFormat(d3d, params.AdapterOrdinal, params.DeviceType,
                                                    mode.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_X4R4G4B4)))
        expected = D3DFMT_X4R4G4B4;
    else if (SUCCEEDED(IDirect3D9_CheckDeviceFormat(d3d, params.AdapterOrdinal, params.DeviceType,
                                                    mode.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_X1R5G5B5)))
        expected = D3DFMT_X1R5G5B5;
    else
        expected = D3DFMT_R5G6B5;

    format = D3DFMT_R3G3B2;
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(format == expected, "Returned format %u, expected %u\n", format, expected);

    if(SUCCEEDED(IDirect3D9_CheckDeviceFormat(d3d, params.AdapterOrdinal, params.DeviceType,
                                              mode.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_A8R3G3B2)))
        expected = D3DFMT_A8R3G3B2;
    else
        expected = D3DFMT_A8R8G8B8;

    format = D3DFMT_A8R3G3B2;
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(format == expected, "Returned format %u, expected %u\n", format, expected);

    if(SUCCEEDED(IDirect3D9_CheckDeviceFormat(d3d, params.AdapterOrdinal, params.DeviceType,
                                              mode.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_P8)))
        expected = D3DFMT_P8;
    else
        expected = D3DFMT_A8R8G8B8;

    format = D3DFMT_P8;
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(format == expected, "Returned format %u, expected %u\n", format, expected);

    if(SUCCEEDED(IDirect3D9_CheckDeviceFormat(d3d, params.AdapterOrdinal, params.DeviceType,
            mode.Format, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, D3DFMT_L8)))
        expected = D3DFMT_L8;
    else
        expected = D3DFMT_X8R8G8B8;

    format = D3DFMT_L8;
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, D3DUSAGE_RENDERTARGET, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(format == expected, "Returned format %u, expected %u\n", format, expected);

    if(SUCCEEDED(IDirect3D9_CheckDeviceFormat(d3d, params.AdapterOrdinal, params.DeviceType,
            mode.Format, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, D3DFMT_L16)))
        expected = D3DFMT_L16;
    else
        expected = D3DFMT_A16B16G16R16;

    format = D3DFMT_L16;
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, D3DUSAGE_RENDERTARGET, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(format == expected, "Returned format %u, expected %u\n", format, expected);

    /* Block-based texture formats and size < block size. */
    format = D3DFMT_DXT1;
    width = 2; height = 2;
    mipmaps = 1;
    hr = D3DXCheckTextureRequirements(device, &width, &height, &mipmaps, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(mipmaps == 1, "Got unexpected level count %u.\n", mipmaps);
    if (has_2d_dxt1)
    {
        ok(width == 4, "Got unexpected width %d.\n", width);
        ok(height == 4, "Got unexpected height %d.\n", height);
        ok(format == D3DFMT_DXT1, "Got unexpected format %u.\n", format);
    }
    else
    {
        ok(width == 2, "Got unexpected width %d.\n", width);
        ok(height == 2, "Got unexpected height %d.\n", height);
        ok(format == D3DFMT_A8R8G8B8, "Got unexpected format %u.\n", format);
    }

    format = D3DFMT_DXT5;
    width = 2; height = 2;
    hr = D3DXCheckTextureRequirements(device, &width, &height, &mipmaps, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(mipmaps == 1, "Got unexpected level count %u.\n", mipmaps);
    if (has_2d_dxt5)
    {
        ok(width == 4, "Got unexpected width %d.\n", width);
        ok(height == 4, "Got unexpected height %d.\n", height);
        ok(format == D3DFMT_DXT5, "Got unexpected format %u.\n", format);

        width = 9;
        height = 9;
        hr = D3DXCheckTextureRequirements(device, &width, &height, &mipmaps, 0, &format, D3DPOOL_DEFAULT);
        ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
        ok(width == 12, "Got unexpected width %u.\n", width);
        ok(height == 12, "Got unexpected height %u.\n", height);
        ok(mipmaps == 1, "Got unexpected level count %u.\n", mipmaps);
        ok(format == D3DFMT_DXT5, "Got unexpected format %u.\n", format);
    }
    else
    {
        ok(width == 2, "Got unexpected width %d.\n", width);
        ok(height == 2, "Got unexpected height %d.\n", height);
        ok(format == D3DFMT_A8R8G8B8, "Got unexpected format %u.\n", format);
    }
    width = 4;
    height = 2;
    hr = D3DXCheckTextureRequirements(device, &width, &height, &mipmaps, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(width == 4, "Got unexpected width %u.\n", width);
    ok(mipmaps == 1, "Got unexpected level count %u.\n", mipmaps);
    if (has_2d_dxt5)
    {
        ok(height == 4, "Got unexpected height %u.\n", height);
        ok(format == D3DFMT_DXT5, "Got unexpected format %u.\n", format);
    }
    else
    {
        ok(height == 2, "Got unexpected height %u.\n", height);
        ok(format == D3DFMT_A8R8G8B8, "Got unexpected format %u.\n", format);
    }

    IDirect3D9_Release(d3d);
}

static void test_D3DXCheckCubeTextureRequirements(IDirect3DDevice9 *device)
{
    UINT size, mipmaps, expected;
    D3DFORMAT format;
    D3DCAPS9 caps;
    HRESULT hr;

    IDirect3DDevice9_GetDeviceCaps(device, &caps);

    if (!(caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP))
    {
        skip("No cube textures support\n");
        return;
    }

    /* general tests */
    hr = D3DXCheckCubeTextureRequirements(device, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckCubeTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);

    hr = D3DXCheckCubeTextureRequirements(device, NULL, NULL, D3DX_DEFAULT, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckCubeTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);

    hr = D3DXCheckCubeTextureRequirements(NULL, NULL, NULL, D3DX_DEFAULT, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCheckCubeTextureRequirements returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    /* size */
    size = D3DX_DEFAULT;
    hr = D3DXCheckCubeTextureRequirements(device, &size, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckCubeTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(size == 256, "Returned size %d, expected %d\n", size, 256);

    /* mipmaps */
    size = 64;
    mipmaps = 9;
    hr = D3DXCheckCubeTextureRequirements(device, &size, &mipmaps, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckCubeTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(mipmaps == 7, "Returned mipmaps %d, expected %d\n", mipmaps, 7);

    size = 284;
    mipmaps = 20;
    expected = caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP_POW2 ? 10 : 9;
    expected = caps.TextureCaps & D3DPTEXTURECAPS_MIPCUBEMAP ? expected : 1;
    hr = D3DXCheckCubeTextureRequirements(device, &size, &mipmaps, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckCubeTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(mipmaps == expected, "Returned mipmaps %d, expected %d\n", mipmaps, expected);

    size = 63;
    mipmaps = 9;
    expected = caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP_POW2 ? 7 : 6;
    expected = caps.TextureCaps & D3DPTEXTURECAPS_MIPCUBEMAP ? expected : 1;
    hr = D3DXCheckCubeTextureRequirements(device, &size, &mipmaps, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckCubeTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(mipmaps == expected, "Returned mipmaps %d, expected %d\n", mipmaps, expected);

    mipmaps = 0;
    hr = D3DXCheckCubeTextureRequirements(device, NULL, &mipmaps, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckCubeTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(mipmaps == 9, "Returned mipmaps %d, expected %d\n", mipmaps, 9);

    if (is_autogenmipmap_supported(device, D3DRTYPE_CUBETEXTURE))
    {
        mipmaps = 3;
        hr = D3DXCheckCubeTextureRequirements(device, NULL,  &mipmaps, D3DUSAGE_AUTOGENMIPMAP, NULL, D3DPOOL_DEFAULT);
        ok(hr == D3D_OK, "D3DXCheckCubeTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
        ok(mipmaps == 0, "Returned mipmaps %d, expected %d\n", mipmaps, 0);
    }
    else
        skip("No D3DUSAGE_AUTOGENMIPMAP support for cube textures\n");

    /* usage */
    hr = D3DXCheckCubeTextureRequirements(device, NULL, NULL, D3DUSAGE_WRITEONLY, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCheckCubeTextureRequirements succeeded, but should've failed.\n");
    hr = D3DXCheckCubeTextureRequirements(device, NULL, NULL, D3DUSAGE_DONOTCLIP, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCheckCubeTextureRequirements succeeded, but should've failed.\n");
    hr = D3DXCheckCubeTextureRequirements(device, NULL, NULL, D3DUSAGE_POINTS, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCheckCubeTextureRequirements succeeded, but should've failed.\n");
    hr = D3DXCheckCubeTextureRequirements(device, NULL, NULL, D3DUSAGE_RTPATCHES, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCheckCubeTextureRequirements succeeded, but should've failed.\n");
    hr = D3DXCheckCubeTextureRequirements(device, NULL, NULL, D3DUSAGE_NPATCHES, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCheckCubeTextureRequirements succeeded, but should've failed.\n");

    /* format */
    hr = D3DXCheckCubeTextureRequirements(device, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckCubeTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);

    format = D3DFMT_UNKNOWN;
    hr = D3DXCheckCubeTextureRequirements(device, NULL, NULL, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckCubeTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(format == D3DFMT_A8R8G8B8, "Returned format %u, expected %u\n", format, D3DFMT_A8R8G8B8);

    format = D3DX_DEFAULT;
    hr = D3DXCheckCubeTextureRequirements(device, NULL, NULL, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckCubeTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(format == D3DFMT_A8R8G8B8, "Returned format %u, expected %u\n", format, D3DFMT_A8R8G8B8);

    format = D3DFMT_R8G8B8;
    hr = D3DXCheckCubeTextureRequirements(device, NULL, NULL, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckCubeTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(format == D3DFMT_X8R8G8B8, "Returned format %u, expected %u\n", format, D3DFMT_X8R8G8B8);
}

static void test_D3DXCheckVolumeTextureRequirements(IDirect3DDevice9 *device)
{
    UINT width, height, depth, mipmaps, expected;
    D3DFORMAT format;
    D3DCAPS9 caps;
    HRESULT hr;

    IDirect3DDevice9_GetDeviceCaps(device, &caps);

    if (!(caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP) || (caps.MaxVolumeExtent < 256))
    {
        skip("Limited or no volume textures support.\n");
        return;
    }

    /* general tests */
    hr = D3DXCheckVolumeTextureRequirements(device, NULL, NULL, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);

    hr = D3DXCheckVolumeTextureRequirements(device, NULL, NULL, NULL, NULL, D3DX_DEFAULT, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);

    hr = D3DXCheckVolumeTextureRequirements(NULL, NULL, NULL, NULL, NULL, D3DX_DEFAULT, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCheckVolumeTextureRequirements returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    /* width, height, depth */
    width = height = depth = D3DX_DEFAULT;
    hr = D3DXCheckVolumeTextureRequirements(device, &width, &height, &depth, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(width == 256, "Returned width %d, expected %d\n", width, 256);
    ok(height == 256, "Returned height %d, expected %d\n", height, 256);
    ok(depth == 1, "Returned depth %d, expected %d\n", depth, 1);

    width = D3DX_DEFAULT;
    hr = D3DXCheckVolumeTextureRequirements(device, &width, NULL, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(width == 256, "Returned width %d, expected %d\n", width, 256);

    width = D3DX_DEFAULT; height = 0; depth = 0;
    hr = D3DXCheckVolumeTextureRequirements(device, &width, &height, &depth, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(width == 1, "Returned width %d, expected %d\n", width, 1);
    ok(height == 1, "Returned height %d, expected %d\n", height, 1);
    ok(depth == 1, "Returned height %d, expected %d\n", depth, 1);

    width = 0; height = 0; depth = 0;
    hr = D3DXCheckVolumeTextureRequirements(device, &width, &height, &depth, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(width == 1, "Returned width %d, expected %d\n", width, 1);
    ok(height == 1, "Returned height %d, expected %d\n", height, 1);
    ok(depth == 1, "Returned height %d, expected %d\n", depth, 1);

    width = 0;
    hr = D3DXCheckVolumeTextureRequirements(device, &width, NULL, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(width == 1, "Returned width %d, expected %d\n", width, 1);

    width = 0xFFFFFFFE;
    hr = D3DXCheckVolumeTextureRequirements(device, &width, NULL, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(width == caps.MaxVolumeExtent, "Returned width %d, expected %d\n", width, caps.MaxVolumeExtent);

    /* format */
    hr = D3DXCheckVolumeTextureRequirements(device, NULL, NULL, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);

    format = D3DFMT_UNKNOWN;
    hr = D3DXCheckVolumeTextureRequirements(device, NULL, NULL, NULL, NULL, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(format == D3DFMT_A8R8G8B8, "Returned format %u, expected %u\n", format, D3DFMT_A8R8G8B8);

    format = D3DX_DEFAULT;
    hr = D3DXCheckVolumeTextureRequirements(device, NULL, NULL, NULL, NULL, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(format == D3DFMT_A8R8G8B8, "Returned format %u, expected %u\n", format, D3DFMT_A8R8G8B8);

    format = D3DFMT_R8G8B8;
    hr = D3DXCheckVolumeTextureRequirements(device, NULL, NULL, NULL, NULL, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(format == D3DFMT_X8R8G8B8, "Returned format %u, expected %u\n", format, D3DFMT_X8R8G8B8);

    format = D3DFMT_DXT3;
    hr = D3DXCheckVolumeTextureRequirements(device, NULL, NULL, NULL, NULL, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    if (has_3d_dxt3)
        ok(format == D3DFMT_DXT3, "Returned format %u, expected %u\n", format, D3DFMT_DXT3);
    else
        ok(format == D3DFMT_A8R8G8B8, "Returned format %u, expected %u\n", format, D3DFMT_A8R8G8B8);

    /* mipmaps */
    if (!(caps.TextureCaps & D3DPTEXTURECAPS_MIPVOLUMEMAP))
    {
        skip("No volume textures mipmapping support\n");
        return;
    }

    width = height = depth = 64;
    mipmaps = 9;
    hr = D3DXCheckVolumeTextureRequirements(device, &width, &height, &depth, &mipmaps, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(mipmaps == 7, "Returned mipmaps %d, expected %d\n", mipmaps, 7);

    width = 284;
    height = 143;
    depth = 55;
    mipmaps = 20;
    expected = (caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP_POW2 && caps.MaxVolumeExtent >= 512) ? 10 : 9;
    hr = D3DXCheckVolumeTextureRequirements(device, &width, &height, &depth, &mipmaps, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(mipmaps == expected, "Returned mipmaps %d, expected %d\n", mipmaps, expected);

    mipmaps = 0;
    hr = D3DXCheckVolumeTextureRequirements(device, NULL, NULL, NULL, &mipmaps, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#x, expected %#x\n", hr, D3D_OK);
    ok(mipmaps == 9, "Returned mipmaps %d, expected %d\n", mipmaps, 9);

    /* D3DUSAGE_AUTOGENMIPMAP is never supported for volume textures. */
    ok(!is_autogenmipmap_supported(device, D3DRTYPE_VOLUMETEXTURE),
            "D3DUSAGE_AUTOGENMIPMAP is unexpectedly supported on volume textures.\n");
}

static void test_D3DXCreateTexture(IDirect3DDevice9 *device)
{
    IDirect3DTexture9 *texture;
    D3DSURFACE_DESC desc;
    D3DCAPS9 caps;
    UINT mipmaps;
    HRESULT hr;

    IDirect3DDevice9_GetDeviceCaps(device, &caps);

    hr = D3DXCreateTexture(NULL, 0, 0, 0, 0, D3DX_DEFAULT, D3DPOOL_DEFAULT, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateTexture returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    /* width and height tests */

    hr = D3DXCreateTexture(device, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, 0, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3D_OK, "D3DXCreateTexture returned %#x, expected %#x\n", hr, D3D_OK);

    if (texture)
    {
        hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &desc);
        ok(hr == D3D_OK, "GetLevelDesc returned %#x, expected %#x\n", hr, D3D_OK);
        ok(desc.Format == D3DFMT_A8R8G8B8, "Returned format %u, expected %u\n", desc.Format, D3DFMT_A8R8G8B8);

        ok(desc.Width == 256, "Returned width %d, expected %d\n", desc.Width, 256);
        ok(desc.Height == 256, "Returned height %d, expected %d\n", desc.Height, 256);

        IDirect3DTexture9_Release(texture);
    }


    hr = D3DXCreateTexture(device, 0, 0, 0, 0, 0, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3D_OK, "D3DXCreateTexture returned %#x, expected %#x\n", hr, D3D_OK);

    if (texture)
    {
        hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &desc);
        ok(hr == D3D_OK, "GetLevelDesc returned %#x, expected %#x\n", hr, D3D_OK);
        ok(desc.Format == D3DFMT_A8R8G8B8, "Returned format %u, expected %u\n", desc.Format, D3DFMT_A8R8G8B8);

        ok(desc.Width == 1, "Returned width %d, expected %d\n", desc.Width, 1);
        ok(desc.Height == 1, "Returned height %d, expected %d\n", desc.Height, 1);

        IDirect3DTexture9_Release(texture);
    }


    if (caps.TextureCaps & D3DPTEXTURECAPS_POW2)
        skip("Hardware only supports pow2 textures\n");
    else
    {
        hr = D3DXCreateTexture(device, D3DX_DEFAULT, 63, 0, 0, 0, D3DPOOL_DEFAULT, &texture);
        ok((hr == D3D_OK) ||
           /* may not work with conditional NPOT */
           ((hr != D3D_OK) && (caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL)),
           "D3DXCreateTexture returned %#x, expected %#x\n", hr, D3D_OK);

        if (texture)
        {
            hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &desc);
            ok(hr == D3D_OK, "GetLevelDesc returned %#x, expected %#x\n", hr, D3D_OK);
            ok(desc.Format == D3DFMT_A8R8G8B8, "Returned format %u, expected %u\n", desc.Format, D3DFMT_A8R8G8B8);

            /* Conditional NPOT may create a texture with different dimensions, so allow those
               situations instead of returning a fail */

            ok(desc.Width == 63 ||
               (caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL),
               "Returned width %d, expected %d\n", desc.Width, 63);

            ok(desc.Height == 63 ||
               (caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL),
               "Returned height %d, expected %d\n", desc.Height, 63);

            IDirect3DTexture9_Release(texture);
        }
    }

    /* mipmaps */

    hr = D3DXCreateTexture(device, 64, 63, 9, 0, 0, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3D_OK, "D3DXCreateTexture returned %#x, expected %#x\n", hr, D3D_OK);

    if (texture)
    {
        mipmaps = IDirect3DTexture9_GetLevelCount(texture);
        ok(mipmaps == 7, "Returned mipmaps %d, expected %d\n", mipmaps, 7);

        IDirect3DTexture9_Release(texture);
    }


    hr = D3DXCreateTexture(device, 284, 137, 9, 0, 0, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3D_OK, "D3DXCreateTexture returned %#x, expected %#x\n", hr, D3D_OK);

    if (texture)
    {
        mipmaps = IDirect3DTexture9_GetLevelCount(texture);
        ok(mipmaps == 9, "Returned mipmaps %d, expected %d\n", mipmaps, 9);

        IDirect3DTexture9_Release(texture);
    }


    hr = D3DXCreateTexture(device, 0, 0, 20, 0, 0, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3D_OK, "D3DXCreateTexture returned %#x, expected %#x\n", hr, D3D_OK);

    if (texture)
    {
        mipmaps = IDirect3DTexture9_GetLevelCount(texture);
        ok(mipmaps == 1, "Returned mipmaps %d, expected %d\n", mipmaps, 1);

        IDirect3DTexture9_Release(texture);
    }


    hr = D3DXCreateTexture(device, 64, 64, 1, 0, 0, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3D_OK, "D3DXCreateTexture returned %#x, expected %#x\n", hr, D3D_OK);

    if (texture)
    {
        mipmaps = IDirect3DTexture9_GetLevelCount(texture);
        ok(mipmaps == 1, "Returned mipmaps %d, expected %d\n", mipmaps, 1);

        IDirect3DTexture9_Release(texture);
    }

    /* usage */

    hr = D3DXCreateTexture(device, 0, 0, 0, D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateTexture succeeded, but should have failed.\n");
    hr = D3DXCreateTexture(device, 0, 0, 0, D3DUSAGE_DONOTCLIP, 0, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateTexture succeeded, but should have failed.\n");
    hr = D3DXCreateTexture(device, 0, 0, 0, D3DUSAGE_POINTS, 0, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateTexture succeeded, but should have failed.\n");
    hr = D3DXCreateTexture(device, 0, 0, 0, D3DUSAGE_RTPATCHES, 0, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateTexture succeeded, but should have failed.\n");
    hr = D3DXCreateTexture(device, 0, 0, 0, D3DUSAGE_NPATCHES, 0, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateTexture succeeded, but should have failed.\n");

    /* format */

    hr = D3DXCreateTexture(device, 0, 0, 0, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3D_OK, "D3DXCreateTexture returned %#x, expected %#x\n", hr, D3D_OK);

    if (texture)
    {
        hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &desc);
        ok(hr == D3D_OK, "GetLevelDesc returned %#x, expected %#x\n", hr, D3D_OK);
        ok(desc.Format == D3DFMT_A8R8G8B8, "Returned format %u, expected %u\n", desc.Format, D3DFMT_A8R8G8B8);

        IDirect3DTexture9_Release(texture);
    }


    hr = D3DXCreateTexture(device, 0, 0, 0, 0, 0, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3D_OK, "D3DXCreateTexture returned %#x, expected %#x\n", hr, D3D_OK);

    if (texture)
    {
        hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &desc);
        ok(hr == D3D_OK, "GetLevelDesc returned %#x, expected %#x\n", hr, D3D_OK);
        ok(desc.Format == D3DFMT_A8R8G8B8, "Returned format %u, expected %u\n", desc.Format, D3DFMT_A8R8G8B8);

        IDirect3DTexture9_Release(texture);
    }

    /* D3DXCreateTextureFromResource */
    hr = D3DXCreateTextureFromResourceA(device, NULL, MAKEINTRESOURCEA(IDB_BITMAP_1x1), &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromResource returned %#x, expected %#x\n", hr, D3D_OK);
    if (SUCCEEDED(hr)) IDirect3DTexture9_Release(texture);

    hr = D3DXCreateTextureFromResourceA(device, NULL, MAKEINTRESOURCEA(IDD_BITMAPDATA_1x1), &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromResource returned %#x, expected %#x\n", hr, D3D_OK);
    if (SUCCEEDED(hr)) IDirect3DTexture9_Release(texture);

    hr = D3DXCreateTextureFromResourceA(device, NULL, MAKEINTRESOURCEA(IDS_STRING), &texture);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXCreateTextureFromResource returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXCreateTextureFromResourceA(NULL, NULL, MAKEINTRESOURCEA(IDD_BITMAPDATA_1x1), &texture);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateTextureFromResource returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateTextureFromResourceA(device, NULL, NULL, &texture);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXCreateTextureFromResource returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXCreateTextureFromResourceA(device, NULL, MAKEINTRESOURCEA(IDD_BITMAPDATA_1x1), NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateTextureFromResource returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);


    /* D3DXCreateTextureFromResourceEx */
    hr = D3DXCreateTextureFromResourceExA(device, NULL, MAKEINTRESOURCEA(IDD_BITMAPDATA_1x1), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromResourceEx returned %#x, expected %#x\n", hr, D3D_OK);
    if (SUCCEEDED(hr)) IDirect3DTexture9_Release(texture);

    hr = D3DXCreateTextureFromResourceExA(device, NULL, MAKEINTRESOURCEA(IDS_STRING), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXCreateTextureFromResourceEx returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXCreateTextureFromResourceExA(NULL, NULL, MAKEINTRESOURCEA(IDD_BITMAPDATA_1x1), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateTextureFromResourceEx returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateTextureFromResourceExA(device, NULL, NULL, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXCreateTextureFromResourceEx returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXCreateTextureFromResourceExA(device, NULL, MAKEINTRESOURCEA(IDD_BITMAPDATA_1x1), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateTextureFromResourceEx returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);
}

static void test_D3DXFilterTexture(IDirect3DDevice9 *device)
{
    IDirect3DTexture9 *tex;
    IDirect3DCubeTexture9 *cubetex;
    IDirect3DVolumeTexture9 *voltex;
    HRESULT hr;

    hr = IDirect3DDevice9_CreateTexture(device, 256, 256, 5, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex, NULL);

    if (SUCCEEDED(hr))
    {
        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) tex, NULL, D3DX_DEFAULT, D3DX_FILTER_NONE);
        ok(hr == D3D_OK, "D3DXFilterTexture returned %#x, expected %#x\n", hr, D3D_OK);

        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) tex, NULL, 0, D3DX_FILTER_NONE);
        ok(hr == D3D_OK, "D3DXFilterTexture returned %#x, expected %#x\n", hr, D3D_OK);

        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) tex, NULL, 0, D3DX_FILTER_BOX + 1); /* Invalid filter */
        ok(hr == D3DERR_INVALIDCALL, "D3DXFilterTexture returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) tex, NULL, 5, D3DX_FILTER_NONE); /* Invalid miplevel */
        ok(hr == D3DERR_INVALIDCALL, "D3DXFilterTexture returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

        IDirect3DTexture9_Release(tex);
    }
    else
        skip("Failed to create texture\n");

    hr = D3DXFilterTexture(NULL, NULL, 0, D3DX_FILTER_NONE);
    ok(hr == D3DERR_INVALIDCALL, "D3DXFilterTexture returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    /* Test different pools */
    hr = IDirect3DDevice9_CreateTexture(device, 256, 256, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &tex, NULL);

    if (SUCCEEDED(hr))
    {
        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) tex, NULL, 0, D3DX_FILTER_NONE);
        ok(hr == D3D_OK, "D3DXFilterTexture returned %#x, expected %#x\n", hr, D3D_OK);
        IDirect3DTexture9_Release(tex);
    }
    else
        skip("Failed to create texture\n");

    hr = IDirect3DDevice9_CreateTexture(device, 256, 256, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &tex, NULL);

    if (SUCCEEDED(hr))
    {
        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) tex, NULL, 0, D3DX_FILTER_NONE);
        ok(hr == D3D_OK, "D3DXFilterTexture returned %#x, expected %#x\n", hr, D3D_OK);
        IDirect3DTexture9_Release(tex);
    }
    else
        skip("Failed to create texture\n");

    hr = IDirect3DDevice9_CreateTexture(device, 256, 256, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex, NULL);
    if (SUCCEEDED(hr))
    {
        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) tex, NULL, 0, D3DX_FILTER_POINT);
        ok(hr == D3D_OK, "D3DXFilterTexture returned %#x, expected %#x\n", hr, D3D_OK);
        IDirect3DTexture9_Release(tex);
    }
    else
        skip("Failed to create texture\n");

    hr = IDirect3DDevice9_CreateTexture(device, 256, 256, 0, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex, NULL);
    if (SUCCEEDED(hr))
    {
        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) tex, NULL, 0, D3DX_FILTER_POINT);
        ok(hr == D3D_OK, "D3DXFilterTexture returned %#x, expected %#x\n", hr, D3D_OK);
        IDirect3DTexture9_Release(tex);
    }
    else
        skip("Failed to create texture\n");

    /* Cube texture test */
    hr = IDirect3DDevice9_CreateCubeTexture(device, 256, 5, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &cubetex, NULL);

    if (SUCCEEDED(hr))
    {
        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) cubetex, NULL, 0, D3DX_FILTER_NONE);
        ok(hr == D3D_OK, "D3DXFilterTexture returned %#x, expected %#x\n", hr, D3D_OK);

        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) cubetex, NULL, 0, D3DX_FILTER_BOX + 1); /* Invalid filter */
        ok(hr == D3DERR_INVALIDCALL, "D3DXFilterTexture returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) cubetex, NULL, 5, D3DX_FILTER_NONE); /* Invalid miplevel */
        ok(hr == D3DERR_INVALIDCALL, "D3DXFilterTexture returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);
        IDirect3DCubeTexture9_Release(cubetex);
    }
    else
        skip("Failed to create texture\n");

    /* Volume texture test */
    hr = IDirect3DDevice9_CreateVolumeTexture(device, 256, 256, 4, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &voltex, NULL);
    if (SUCCEEDED(hr))
    {
        DWORD level_count = IDirect3DVolumeTexture9_GetLevelCount(voltex);

        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) voltex, NULL, 0, D3DX_FILTER_NONE);
        ok(hr == D3D_OK, "D3DXFilterTexture returned %#x, expected %#x\n", hr, D3D_OK);

        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) voltex, NULL, 0, D3DX_DEFAULT);
        ok(hr == D3D_OK, "D3DXFilterTexture returned %#x, expected %#x\n", hr, D3D_OK);

        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) voltex, NULL, 0, D3DX_FILTER_BOX);
        ok(hr == D3D_OK, "D3DXFilterTexture returned %#x, expected %#x\n", hr, D3D_OK);

        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) voltex, NULL, level_count - 1, D3DX_DEFAULT);
        ok(hr == D3D_OK, "D3DXFilterTexture returned %#x, expected %#x\n", hr, D3D_OK);

        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) voltex, NULL, level_count, D3DX_DEFAULT);
        ok(hr == D3DERR_INVALIDCALL, "D3DXFilterTexture returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

        IDirect3DVolumeTexture9_Release(voltex);
    }
    else
        skip("Failed to create volume texture\n");

    /* Test textures with D3DUSAGE_AUTOGENMIPMAP usage */
    if (!is_autogenmipmap_supported(device, D3DRTYPE_TEXTURE))
    {
        skip("No D3DUSAGE_AUTOGENMIPMAP supported for textures\n");
        return;
    }

    hr = IDirect3DDevice9_CreateTexture(device, 256, 256, 0, D3DUSAGE_DYNAMIC | D3DUSAGE_AUTOGENMIPMAP, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex, NULL);
    if (SUCCEEDED(hr))
    {
        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) tex, NULL, 0, D3DX_FILTER_NONE);
        ok(hr == D3D_OK, "D3dXFilteTexture returned %#x, expected %#x\n", hr, D3D_OK);
        IDirect3DTexture9_Release(tex);
    }
    else
        skip("Failed to create texture\n");

    hr = IDirect3DDevice9_CreateTexture(device, 256, 256, 1, D3DUSAGE_DYNAMIC | D3DUSAGE_AUTOGENMIPMAP, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex, NULL);
    if (SUCCEEDED(hr))
    {
        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) tex, NULL, 0, D3DX_FILTER_NONE);
        ok(hr == D3D_OK, "D3dXFilteTexture returned %#x, expected %#x\n", hr, D3D_OK);
        IDirect3DTexture9_Release(tex);
    }
    else
        skip("Failed to create texture\n");
}

static BOOL color_match(const DWORD *value, const DWORD *expected)
{
    int i;

    for (i = 0; i < 4; i++)
    {
        DWORD diff = value[i] > expected[i] ? value[i] - expected[i] : expected[i] - value[i];
        if (diff > 1) return FALSE;
    }
    return TRUE;
}

static void WINAPI fillfunc(D3DXVECTOR4 *value, const D3DXVECTOR2 *texcoord,
                            const D3DXVECTOR2 *texelsize, void *data)
{
    value->x = texcoord->x;
    value->y = texcoord->y;
    value->z = texelsize->x;
    value->w = 1.0f;
}

static void test_D3DXFillTexture(IDirect3DDevice9 *device)
{
    static const struct
    {
        DWORD usage;
        D3DPOOL pool;
    }
    test_access_types[] =
    {
        {0,  D3DPOOL_MANAGED},
        {0,  D3DPOOL_DEFAULT},
        {D3DUSAGE_RENDERTARGET, D3DPOOL_DEFAULT},
    };

    IDirect3DTexture9 *tex;
    HRESULT hr;
    D3DLOCKED_RECT lock_rect;
    DWORD x, y, m;
    DWORD v[4], e[4];
    DWORD value, expected, size, pitch;
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(test_access_types); ++i)
    {
        size = 4;
        hr = IDirect3DDevice9_CreateTexture(device, size, size, 0, test_access_types[i].usage,
                D3DFMT_A8R8G8B8, test_access_types[i].pool, &tex, NULL);
        ok(hr == D3D_OK, "Unexpected hr %#x, i %u.\n", hr, i);

        hr = D3DXFillTexture(tex, fillfunc, NULL);
        ok(hr == D3D_OK, "Unexpected hr %#x, i %u.\n", hr, i);

        for (m = 0; m < 3; m++)
        {
            IDirect3DSurface9 *src_surface, *temp_surface;

            hr = IDirect3DTexture9_GetSurfaceLevel(tex, m, &src_surface);
            ok(hr == D3D_OK, "Unexpected hr %#x, i %u, m %u.\n", hr, i, m);
            temp_surface = src_surface;

            if (FAILED(hr = IDirect3DSurface9_LockRect(src_surface, &lock_rect, NULL, D3DLOCK_READONLY)))
            {
                hr = IDirect3DDevice9_CreateRenderTarget(device, size, size,
                        D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &temp_surface, NULL);
                ok(hr == D3D_OK, "Unexpected hr %#x, i %u, m %u.\n", hr, i, m);
                hr = IDirect3DDevice9_StretchRect(device, src_surface, NULL, temp_surface, NULL, D3DTEXF_NONE);
                ok(hr == D3D_OK, "Unexpected hr %#x, i %u, m %u.\n", hr, i, m);
                hr = IDirect3DSurface9_LockRect(temp_surface, &lock_rect, NULL, D3DLOCK_READONLY);
                ok(hr == D3D_OK, "Unexpected hr %#x, i %u, m %u.\n", hr, i, m);
            }

            pitch = lock_rect.Pitch / sizeof(DWORD);
            for (y = 0; y < size; y++)
            {
                for (x = 0; x < size; x++)
                {
                    value = ((DWORD *)lock_rect.pBits)[y * pitch + x];
                    v[0] = (value >> 24) & 0xff;
                    v[1] = (value >> 16) & 0xff;
                    v[2] = (value >> 8) & 0xff;
                    v[3] = value & 0xff;

                    e[0] = 0xff;
                    e[1] = (x + 0.5f) / size * 255.0f + 0.5f;
                    e[2] = (y + 0.5f) / size * 255.0f + 0.5f;
                    e[3] = 255.0f / size + 0.5f;
                    expected = e[0] << 24 | e[1] << 16 | e[2] << 8 | e[3];

                    ok(color_match(v, e),
                            "Texel at (%u, %u) doesn't match: %#x, expected %#x, i %u, m %u.\n",
                            x, y, value, expected, i, m);
                }
            }
            IDirect3DSurface9_UnlockRect(temp_surface);
            if (temp_surface != src_surface)
                IDirect3DSurface9_Release(temp_surface);
            IDirect3DSurface9_Release(src_surface);
            size >>= 1;
        }
        IDirect3DTexture9_Release(tex);
    }

    hr = IDirect3DDevice9_CreateTexture(device, 256, 256, 1, D3DUSAGE_DEPTHSTENCIL,
            D3DFMT_D16_LOCKABLE, D3DPOOL_DEFAULT, &tex, NULL);
    if (hr == D3D_OK)
    {
        hr = D3DXFillTexture(tex, fillfunc, NULL);
        todo_wine ok(hr == D3D_OK, "Unexpected hr %#x.\n", hr);
        IDirect3DTexture9_Release(tex);
    }

    hr = IDirect3DDevice9_CreateTexture(device, 256, 256, 1, D3DUSAGE_DEPTHSTENCIL,
            D3DFMT_D16, D3DPOOL_DEFAULT, &tex, NULL);
    if (hr == D3D_OK)
    {
        hr = D3DXFillTexture(tex, fillfunc, NULL);
        ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#x.\n", hr);
        IDirect3DTexture9_Release(tex);
    }

    hr = IDirect3DDevice9_CreateTexture(device, 4, 4, 1, 0, D3DFMT_A1R5G5B5,
                                        D3DPOOL_MANAGED, &tex, NULL);

    if (SUCCEEDED(hr))
    {
        hr = D3DXFillTexture(tex, fillfunc, NULL);
        ok(hr == D3D_OK, "D3DXFillTexture returned %#x, expected %#x\n", hr, D3D_OK);

        hr = IDirect3DTexture9_LockRect(tex, 0, &lock_rect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "Couldn't lock the texture, error %#x\n", hr);
        if (SUCCEEDED(hr))
        {
            pitch = lock_rect.Pitch / sizeof(WORD);
            for (y = 0; y < 4; y++)
            {
                for (x = 0; x < 4; x++)
                {
                    value = ((WORD *)lock_rect.pBits)[y * pitch + x];
                    v[0] = value >> 15;
                    v[1] = value >> 10 & 0x1f;
                    v[2] = value >> 5 & 0x1f;
                    v[3] = value & 0x1f;

                    e[0] = 1;
                    e[1] = (x + 0.5f) / 4.0f * 31.0f + 0.5f;
                    e[2] = (y + 0.5f) / 4.0f * 31.0f + 0.5f;
                    e[3] = 8;
                    expected = e[0] << 15 | e[1] << 10 | e[2] << 5 | e[3];

                    ok(color_match(v, e),
                       "Texel at (%u, %u) doesn't match: %#x, expected %#x\n",
                       x, y, value, expected);
                }
            }
            IDirect3DTexture9_UnlockRect(tex, 0);
        }

        IDirect3DTexture9_Release(tex);
    }
    else
        skip("Failed to create texture\n");

    /* test floating-point textures */
    hr = IDirect3DDevice9_CreateTexture(device, 4, 4, 1, 0, D3DFMT_A16B16G16R16F,
                                        D3DPOOL_MANAGED, &tex, NULL);

    if (SUCCEEDED(hr))
    {
        hr = D3DXFillTexture(tex, fillfunc, NULL);
        ok(hr == D3D_OK, "D3DXFillTexture returned %#x, expected %#x\n", hr, D3D_OK);

        hr = IDirect3DTexture9_LockRect(tex, 0, &lock_rect, NULL, D3DLOCK_READONLY);
        if (SUCCEEDED(hr))
        {
            pitch = lock_rect.Pitch / sizeof(WORD);
            for (y = 0; y < 4; y++)
            {
                WORD *ptr = (WORD *)lock_rect.pBits + y * pitch;
                for (x = 0; x < 4; x++)
                {
                    D3DXVECTOR4 got, expected;

                    D3DXFloat16To32Array((FLOAT *)&got, (D3DXFLOAT16 *)ptr, 4);
                    ptr += 4;

                    expected.x = (x + 0.5f) / 4.0f;
                    expected.y = (y + 0.5f) / 4.0f;
                    expected.z = 1.0f / 4.0f;
                    expected.w = 1.0f;

                    expect_vec4(&expected, &got);
                }
            }

            IDirect3DTexture9_UnlockRect(tex, 0);
        }
        else
            skip("Failed to lock texture\n");

        IDirect3DTexture9_Release(tex);
    }
    else
        skip("Failed to create D3DFMT_A16B16G16R16F texture\n");

    hr = IDirect3DDevice9_CreateTexture(device, 4, 4, 1, 0, D3DFMT_A32B32G32R32F,
                                        D3DPOOL_MANAGED, &tex, NULL);

    if (SUCCEEDED(hr))
    {
        hr = D3DXFillTexture(tex, fillfunc, NULL);
        ok(hr == D3D_OK, "D3DXFillTexture returned %#x, expected %#x\n", hr, D3D_OK);

        hr = IDirect3DTexture9_LockRect(tex, 0, &lock_rect, NULL, D3DLOCK_READONLY);
        if (SUCCEEDED(hr))
        {
            pitch = lock_rect.Pitch / sizeof(float);
            for (y = 0; y < 4; y++)
            {
                float *ptr = (float *)lock_rect.pBits + y * pitch;
                for (x = 0; x < 4; x++)
                {
                    D3DXVECTOR4 got, expected;

                    got.x = *ptr++;
                    got.y = *ptr++;
                    got.z = *ptr++;
                    got.w = *ptr++;

                    expected.x = (x + 0.5f) / 4.0f;
                    expected.y = (y + 0.5f) / 4.0f;
                    expected.z = 1.0f / 4.0f;
                    expected.w = 1.0f;

                    expect_vec4(&expected, &got);
                }
            }

            IDirect3DTexture9_UnlockRect(tex, 0);
        }
        else
            skip("Failed to lock texture\n");

        IDirect3DTexture9_Release(tex);
    }
    else
        skip("Failed to create D3DFMT_A32B32G32R32F texture\n");

    /* test a compressed texture */
    hr = IDirect3DDevice9_CreateTexture(device, 4, 4, 1, 0, D3DFMT_DXT1,
                                        D3DPOOL_MANAGED, &tex, NULL);

    if (SUCCEEDED(hr))
    {
        hr = D3DXFillTexture(tex, fillfunc, NULL);
        todo_wine ok(hr == D3D_OK, "D3DXFillTexture returned %#x, expected %#x\n", hr, D3D_OK);

        IDirect3DTexture9_Release(tex);
    }
    else
        skip("Failed to create D3DFMT_DXT1 texture\n");
}

static void WINAPI fillfunc_cube(D3DXVECTOR4 *value, const D3DXVECTOR3 *texcoord,
                                 const D3DXVECTOR3 *texelsize, void *data)
{
    value->x = (texcoord->x + 1.0f) / 2.0f;
    value->y = (texcoord->y + 1.0f) / 2.0f;
    value->z = (texcoord->z + 1.0f) / 2.0f;
    value->w = texelsize->x;
}

enum cube_coord
{
    XCOORD = 0,
    XCOORDINV = 1,
    YCOORD = 2,
    YCOORDINV = 3,
    ZERO = 4,
    ONE = 5
};

static float get_cube_coord(enum cube_coord coord, unsigned int x, unsigned int y, unsigned int size)
{
    switch (coord)
    {
        case XCOORD:
            return x + 0.5f;
        case XCOORDINV:
            return size - x - 0.5f;
        case YCOORD:
            return y + 0.5f;
        case YCOORDINV:
            return size - y - 0.5f;
        case ZERO:
            return 0.0f;
        case ONE:
            return size;
        default:
           trace("Unexpected coordinate value\n");
           return 0.0f;
    }
}

static void test_D3DXFillCubeTexture(IDirect3DDevice9 *device)
{
    IDirect3DCubeTexture9 *tex;
    HRESULT hr;
    D3DLOCKED_RECT lock_rect;
    DWORD x, y, f, m;
    DWORD v[4], e[4];
    DWORD value, expected, size, pitch;
    enum cube_coord coordmap[6][3] =
        {
            {ONE, YCOORDINV, XCOORDINV},
            {ZERO, YCOORDINV, XCOORD},
            {XCOORD, ONE, YCOORD},
            {XCOORD, ZERO, YCOORDINV},
            {XCOORD, YCOORDINV, ONE},
            {XCOORDINV, YCOORDINV, ZERO}
        };

    size = 4;
    hr = IDirect3DDevice9_CreateCubeTexture(device, size, 0, 0, D3DFMT_A8R8G8B8,
                                            D3DPOOL_MANAGED, &tex, NULL);

    if (SUCCEEDED(hr))
    {
        hr = D3DXFillCubeTexture(tex, fillfunc_cube, NULL);
        ok(hr == D3D_OK, "D3DXFillCubeTexture returned %#x, expected %#x\n", hr, D3D_OK);

        for (m = 0; m < 3; m++)
        {
            for (f = 0; f < 6; f++)
            {
                hr = IDirect3DCubeTexture9_LockRect(tex, f, m, &lock_rect, NULL, D3DLOCK_READONLY);
                ok(hr == D3D_OK, "Couldn't lock the texture, error %#x\n", hr);
                if (SUCCEEDED(hr))
                {
                    pitch = lock_rect.Pitch / sizeof(DWORD);
                    for (y = 0; y < size; y++)
                    {
                        for (x = 0; x < size; x++)
                        {
                            value = ((DWORD *)lock_rect.pBits)[y * pitch + x];
                            v[0] = (value >> 24) & 0xff;
                            v[1] = (value >> 16) & 0xff;
                            v[2] = (value >> 8) & 0xff;
                            v[3] = value & 0xff;

                            e[0] = (f == 0) || (f == 1) ?
                                0 : (BYTE)(255.0f / size * 2.0f + 0.5f);
                            e[1] = get_cube_coord(coordmap[f][0], x, y, size) / size * 255.0f + 0.5f;
                            e[2] = get_cube_coord(coordmap[f][1], x, y, size) / size * 255.0f + 0.5f;
                            e[3] = get_cube_coord(coordmap[f][2], x, y, size) / size * 255.0f + 0.5f;
                            expected = e[0] << 24 | e[1] << 16 | e[2] << 8 | e[3];

                            ok(color_match(v, e),
                               "Texel at face %u (%u, %u) doesn't match: %#x, expected %#x\n",
                               f, x, y, value, expected);
                        }
                    }
                    IDirect3DCubeTexture9_UnlockRect(tex, f, m);
                }
            }
            size >>= 1;
        }

        IDirect3DCubeTexture9_Release(tex);
    }
    else
        skip("Failed to create texture\n");

    hr = IDirect3DDevice9_CreateCubeTexture(device, 4, 1, 0, D3DFMT_A1R5G5B5,
                                            D3DPOOL_MANAGED, &tex, NULL);

    if (SUCCEEDED(hr))
    {
        hr = D3DXFillCubeTexture(tex, fillfunc_cube, NULL);
        ok(hr == D3D_OK, "D3DXFillTexture returned %#x, expected %#x\n", hr, D3D_OK);
        for (f = 0; f < 6; f++)
        {
            hr = IDirect3DCubeTexture9_LockRect(tex, f, 0, &lock_rect, NULL, D3DLOCK_READONLY);
            ok(hr == D3D_OK, "Couldn't lock the texture, error %#x\n", hr);
            if (SUCCEEDED(hr))
            {
                pitch = lock_rect.Pitch / sizeof(WORD);
                for (y = 0; y < 4; y++)
                {
                    for (x = 0; x < 4; x++)
                    {
                        value = ((WORD *)lock_rect.pBits)[y * pitch + x];
                        v[0] = value >> 15;
                        v[1] = value >> 10 & 0x1f;
                        v[2] = value >> 5 & 0x1f;
                        v[3] = value & 0x1f;

                        e[0] = (f == 0) || (f == 1) ?
                            0 : (BYTE)(1.0f / size * 2.0f + 0.5f);
                        e[1] = get_cube_coord(coordmap[f][0], x, y, 4) / 4 * 31.0f + 0.5f;
                        e[2] = get_cube_coord(coordmap[f][1], x, y, 4) / 4 * 31.0f + 0.5f;
                        e[3] = get_cube_coord(coordmap[f][2], x, y, 4) / 4 * 31.0f + 0.5f;
                        expected = e[0] << 15 | e[1] << 10 | e[2] << 5 | e[3];

                        ok(color_match(v, e),
                           "Texel at face %u (%u, %u) doesn't match: %#x, expected %#x\n",
                           f, x, y, value, expected);
                    }
                }
                IDirect3DCubeTexture9_UnlockRect(tex, f, 0);
            }
        }

        IDirect3DCubeTexture9_Release(tex);
    }
    else
        skip("Failed to create texture\n");
}

static void WINAPI fillfunc_volume(D3DXVECTOR4 *value, const D3DXVECTOR3 *texcoord,
                                   const D3DXVECTOR3 *texelsize, void *data)
{
    value->x = texcoord->x;
    value->y = texcoord->y;
    value->z = texcoord->z;
    value->w = texelsize->x;
}

static void test_D3DXFillVolumeTexture(IDirect3DDevice9 *device)
{
    IDirect3DVolumeTexture9 *tex;
    HRESULT hr;
    D3DLOCKED_BOX lock_box;
    DWORD x, y, z, m;
    DWORD v[4], e[4];
    DWORD value, expected, size, row_pitch, slice_pitch;

    size = 4;
    hr = IDirect3DDevice9_CreateVolumeTexture(device, size, size, size, 0, 0, D3DFMT_A8R8G8B8,
            D3DPOOL_DEFAULT, &tex, NULL);
    ok(hr == D3D_OK, "Unexpected hr %#x.\n", hr);
    hr = D3DXFillVolumeTexture(tex, fillfunc_volume, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#x.\n", hr);
    IDirect3DVolumeTexture9_Release(tex);

    hr = IDirect3DDevice9_CreateVolumeTexture(device, size, size, size, 0, 0, D3DFMT_A8R8G8B8,
            D3DPOOL_MANAGED, &tex, NULL);
    if (SUCCEEDED(hr))
    {
        hr = D3DXFillVolumeTexture(tex, fillfunc_volume, NULL);
        ok(hr == D3D_OK, "D3DXFillVolumeTexture returned %#x, expected %#x\n", hr, D3D_OK);

        for (m = 0; m < 3; m++)
        {
            hr = IDirect3DVolumeTexture9_LockBox(tex, m, &lock_box, NULL, D3DLOCK_READONLY);
            ok(hr == D3D_OK, "Couldn't lock the texture, error %#x\n", hr);
            if (SUCCEEDED(hr))
            {
                row_pitch = lock_box.RowPitch / sizeof(DWORD);
                slice_pitch = lock_box.SlicePitch / sizeof(DWORD);
                for (z = 0; z < size; z++)
                {
                    for (y = 0; y < size; y++)
                    {
                        for (x = 0; x < size; x++)
                        {
                            value = ((DWORD *)lock_box.pBits)[z * slice_pitch + y * row_pitch + x];
                            v[0] = (value >> 24) & 0xff;
                            v[1] = (value >> 16) & 0xff;
                            v[2] = (value >> 8) & 0xff;
                            v[3] = value & 0xff;

                            e[0] = 255.0f / size + 0.5f;
                            e[1] = (x + 0.5f) / size * 255.0f + 0.5f;
                            e[2] = (y + 0.5f) / size * 255.0f + 0.5f;
                            e[3] = (z + 0.5f) / size * 255.0f + 0.5f;
                            expected = e[0] << 24 | e[1] << 16 | e[2] << 8 | e[3];

                            ok(color_match(v, e),
                               "Texel at (%u, %u, %u) doesn't match: %#x, expected %#x\n",
                               x, y, z, value, expected);
                        }
                    }
                }
                IDirect3DVolumeTexture9_UnlockBox(tex, m);
            }
            size >>= 1;
        }

        IDirect3DVolumeTexture9_Release(tex);
    }
    else
        skip("Failed to create texture\n");

    hr = IDirect3DDevice9_CreateVolumeTexture(device, 4, 4, 4, 1, 0, D3DFMT_A1R5G5B5,
                                              D3DPOOL_MANAGED, &tex, NULL);

    if (SUCCEEDED(hr))
    {
        hr = D3DXFillVolumeTexture(tex, fillfunc_volume, NULL);
        ok(hr == D3D_OK, "D3DXFillTexture returned %#x, expected %#x\n", hr, D3D_OK);
        hr = IDirect3DVolumeTexture9_LockBox(tex, 0, &lock_box, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "Couldn't lock the texture, error %#x\n", hr);
        if (SUCCEEDED(hr))
        {
            row_pitch = lock_box.RowPitch / sizeof(WORD);
            slice_pitch = lock_box.SlicePitch / sizeof(WORD);
            for (z = 0; z < 4; z++)
            {
                for (y = 0; y < 4; y++)
                {
                    for (x = 0; x < 4; x++)
                    {
                        value = ((WORD *)lock_box.pBits)[z * slice_pitch + y * row_pitch + x];
                        v[0] = value >> 15;
                        v[1] = value >> 10 & 0x1f;
                        v[2] = value >> 5 & 0x1f;
                        v[3] = value & 0x1f;

                        e[0] = 1;
                        e[1] = (x + 0.5f) / 4 * 31.0f + 0.5f;
                        e[2] = (y + 0.5f) / 4 * 31.0f + 0.5f;
                        e[3] = (z + 0.5f) / 4 * 31.0f + 0.5f;
                        expected = e[0] << 15 | e[1] << 10 | e[2] << 5 | e[3];

                        ok(color_match(v, e),
                           "Texel at (%u, %u, %u) doesn't match: %#x, expected %#x\n",
                           x, y, z, value, expected);
                    }
                }
            }
            IDirect3DVolumeTexture9_UnlockBox(tex, 0);
        }

        IDirect3DVolumeTexture9_Release(tex);
    }
    else
        skip("Failed to create texture\n");
}

static void test_D3DXCreateTextureFromFileInMemory(IDirect3DDevice9 *device)
{
    static const DWORD dds_dxt5_expected[] =
    {
        0xff7b207b, 0xff7b207b, 0xff84df7b, 0xff84df7b,
        0xff7b207b, 0xff7b207b, 0xff84df7b, 0xff84df7b,
        0xff7b207b, 0xff7b207b, 0xff84df7b, 0xff84df7b,
        0xff7b207b, 0xff7b207b, 0xff84df7b, 0xff84df7b,
    };
    static const DWORD dds_dxt5_8_8_expected[] =
    {
        0x0000ff00, 0x0000ff00, 0x000000ff, 0x000000ff, 0x3f00ffff, 0x3f00ffff, 0x3fff0000, 0x3fff0000,
        0x0000ff00, 0x0000ff00, 0x000000ff, 0x000000ff, 0x3f00ffff, 0x3f00ffff, 0x3fff0000, 0x3fff0000,
        0x000000ff, 0x000000ff, 0x0000ff00, 0x0000ff00, 0x3fff0000, 0x3fff0000, 0x3f00ffff, 0x3f00ffff,
        0x000000ff, 0x000000ff, 0x0000ff00, 0x0000ff00, 0x3fff0000, 0x3fff0000, 0x3f00ffff, 0x3f00ffff,
        0x7fffff00, 0x7fffff00, 0x7fff00ff, 0x7fff00ff, 0xff000000, 0xff000000, 0xffffffff, 0xffffffff,
        0x7fffff00, 0x7fffff00, 0x7fff00ff, 0x7fff00ff, 0xff000000, 0xff000000, 0xffffffff, 0xffffffff,
        0x7fff00ff, 0x7fff00ff, 0x7fffff00, 0x7fffff00, 0xffffffff, 0xffffffff, 0xff000000, 0xff000000,
        0x7fff00ff, 0x7fff00ff, 0x7fffff00, 0x7fffff00, 0xffffffff, 0xffffffff, 0xff000000, 0xff000000,
    };
    static const DWORD dds_dxt5_8_8_expected_misaligned_1[] =
    {
        0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x3fff0000, 0x3fff0000, 0x3fff0000, 0x3fff0000,
        0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x3fff0000, 0x3fff0000, 0x3fff0000, 0x3fff0000,
        0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x3fff0000, 0x3fff0000, 0x3fff0000, 0x3fff0000,
        0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x3fff0000, 0x3fff0000, 0x3fff0000, 0x3fff0000,
        0x7fff00ff, 0x7fff00ff, 0x7fff00ff, 0x7fff00ff, 0xff000000, 0xff000000, 0xff000000, 0xff000000,
        0x7fff00ff, 0x7fff00ff, 0x7fff00ff, 0x7fff00ff, 0xff000000, 0xff000000, 0xff000000, 0xff000000,
        0x7fff00ff, 0x7fff00ff, 0x7fff00ff, 0x7fff00ff, 0xff000000, 0xff000000, 0xff000000, 0xff000000,
        0x7fff00ff, 0x7fff00ff, 0x7fff00ff, 0x7fff00ff, 0xff000000, 0xff000000, 0xff000000, 0xff000000,
    };
    static const DWORD dds_dxt5_8_8_expected_misaligned_3[] =
    {
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x0000ff00, 0x0000ff00, 0x3fff0000, 0x3fff0000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x0000ff00, 0x0000ff00, 0x3fff0000, 0x3fff0000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x7fff00ff, 0x7fff00ff, 0xff000000, 0xff000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x7fff00ff, 0x7fff00ff, 0xff000000, 0xff000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    };
    IDirect3DSurface9 *surface, *uncompressed_surface;
    IDirect3DTexture9 *texture;
    D3DLOCKED_RECT lock_rect;
    D3DRESOURCETYPE type;
    D3DSURFACE_DESC desc;
    unsigned int i, x, y;
    DWORD level_count;
    HRESULT hr;
    RECT rect;

    hr = D3DXCreateTextureFromFileInMemory(device, dds_16bit, sizeof(dds_16bit), &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);
    if (SUCCEEDED(hr)) IDirect3DTexture9_Release(texture);

    hr = D3DXCreateTextureFromFileInMemory(device, dds_24bit, sizeof(dds_24bit), &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);
    if (SUCCEEDED(hr)) IDirect3DTexture9_Release(texture);

    hr = D3DXCreateTextureFromFileInMemory(device, dds_24bit, sizeof(dds_24bit) - 1, &texture);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXCreateTextureFromFileInMemory returned %#x, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    /* Check that D3DXCreateTextureFromFileInMemory accepts cube texture dds file (only first face texture is loaded) */
    hr = D3DXCreateTextureFromFileInMemory(device, dds_cube_map, sizeof(dds_cube_map), &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemory returned %#x, expected %#x.\n", hr, D3D_OK);
    type = IDirect3DTexture9_GetType(texture);
    ok(type == D3DRTYPE_TEXTURE, "IDirect3DTexture9_GetType returned %u, expected %u.\n", type, D3DRTYPE_TEXTURE);
    hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &desc);
    ok(hr == D3D_OK, "IDirect3DTexture9_GetLevelDesc returned %#x, expected %#x.\n", hr, D3D_OK);
    ok(desc.Width == 4, "Width is %u, expected 4.\n", desc.Width);
    ok(desc.Height == 4, "Height is %u, expected 4.\n", desc.Height);
    if (has_cube_dxt5)
    {
        ok(desc.Format == D3DFMT_DXT5, "Unexpected texture format %#x.\n", desc.Format);
        hr = IDirect3DTexture9_LockRect(texture, 0, &lock_rect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "IDirect3DTexture9_LockRect returned %#x, expected %#x\n", hr, D3D_OK);
        if (SUCCEEDED(hr))
        {
            for (i = 0; i < 16; i++)
                ok(((BYTE *)lock_rect.pBits)[i] == dds_cube_map[128 + i],
                        "Byte at index %u is 0x%02x, expected 0x%02x.\n",
                        i, ((BYTE *)lock_rect.pBits)[i], dds_cube_map[128 + i]);
            IDirect3DTexture9_UnlockRect(texture, 0);
        }
    }
    else
    {
        ok(desc.Format == D3DFMT_A8R8G8B8, "Unexpected texture format %#x.\n", desc.Format);
        skip("D3DFMT_DXT5 textures are not supported, skipping a test.\n");
    }
    IDirect3DTexture9_Release(texture);

    /* Test with a DXT5 texture smaller than the block size. */
    hr = D3DXCreateTextureFromFileInMemory(device, dds_dxt5, sizeof(dds_dxt5), &texture);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    if (SUCCEEDED(hr) && has_2d_dxt5)
    {
        type = IDirect3DTexture9_GetType(texture);
        ok(type == D3DRTYPE_TEXTURE, "Got unexpected type %u.\n", type);
        hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &desc);
        ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
        ok(desc.Width == 4, "Got unexpected width %u.\n", desc.Width);
        ok(desc.Height == 4, "Got unexpected height %u.\n", desc.Height);

        IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
        hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 4, 4, D3DFMT_A8R8G8B8,
                D3DPOOL_DEFAULT, &uncompressed_surface, NULL);
        ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
        hr = D3DXLoadSurfaceFromSurface(uncompressed_surface, NULL, NULL, surface, NULL, NULL, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
        hr = IDirect3DSurface9_LockRect(uncompressed_surface, &lock_rect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
        for (y = 0; y < 4; ++y)
        {
            for (x = 0; x < 4; ++x)
            {
                /* Use a large tolerance, decompression + stretching +
                 * compression + decompression again introduce quite a bit of
                 * precision loss. */
                ok(compare_color(((DWORD *)lock_rect.pBits)[lock_rect.Pitch / 4 * y + x],
                        dds_dxt5_expected[y * 4 + x], 32),
                        "Color at position %u, %u is 0x%08x, expected 0x%08x.\n",
                        x, y, ((DWORD *)lock_rect.pBits)[lock_rect.Pitch / 4 * y + x],
                        dds_dxt5_expected[y * 4 + x]);
            }
        }
        hr = IDirect3DSurface9_UnlockRect(uncompressed_surface);
        ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

        IDirect3DSurface9_Release(uncompressed_surface);
        IDirect3DSurface9_Release(surface);
    }
    if (SUCCEEDED(hr))
        IDirect3DTexture9_Release(texture);

    /* Test with a larger DXT5 texture. */
    hr = D3DXCreateTextureFromFileInMemory(device, dds_dxt5_8_8, sizeof(dds_dxt5_8_8), &texture);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    type = IDirect3DTexture9_GetType(texture);
    ok(type == D3DRTYPE_TEXTURE, "Got unexpected type %u.\n", type);
    hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &desc);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(desc.Width == 8, "Got unexpected width %u.\n", desc.Width);
    ok(desc.Height == 8, "Got unexpected height %u.\n", desc.Height);
    IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 8, 8, D3DFMT_A8R8G8B8,
            D3DPOOL_DEFAULT, &uncompressed_surface, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = D3DXLoadSurfaceFromSurface(uncompressed_surface, NULL, NULL, surface, NULL, NULL, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IDirect3DSurface9_LockRect(uncompressed_surface, &lock_rect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
        for (y = 0; y < 8; ++y)
        {
            for (x = 0; x < 8; ++x)
            {
                ok(compare_color(((DWORD *)lock_rect.pBits)[lock_rect.Pitch / 4 * y + x],
                        dds_dxt5_8_8_expected[y * 8 + x], 0),
                        "Color at position %u, %u is 0x%08x, expected 0x%08x.\n",
                        x, y, ((DWORD *)lock_rect.pBits)[lock_rect.Pitch / 4 * y + x],
                        dds_dxt5_8_8_expected[y * 8 + x]);
            }
        }
        hr = IDirect3DSurface9_UnlockRect(uncompressed_surface);
        ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    }

    hr = IDirect3DSurface9_LockRect(surface, &lock_rect, NULL, D3DLOCK_READONLY);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    for (y = 0; y < 2; ++y)
        memset(&((BYTE *)lock_rect.pBits)[y * lock_rect.Pitch], 0, 16 * 2);
    hr = IDirect3DSurface9_UnlockRect(surface);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    SetRect(&rect, 2, 2, 6, 6);
    hr = D3DXLoadSurfaceFromMemory(surface, NULL, NULL, &dds_dxt5_8_8[128],
            D3DFMT_DXT5, 16 * 2, NULL, &rect, D3DX_FILTER_POINT, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = D3DXLoadSurfaceFromSurface(uncompressed_surface, NULL, NULL, surface, NULL, NULL, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DSurface9_LockRect(uncompressed_surface, &lock_rect, NULL, D3DLOCK_READONLY);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    for (y = 0; y < 8; ++y)
    {
        for (x = 0; x < 8; ++x)
        {
            ok(compare_color(((DWORD *)lock_rect.pBits)[lock_rect.Pitch / 4 * y + x],
                    dds_dxt5_8_8_expected_misaligned_1[y * 8 + x], 0),
                    "Color at position %u, %u is 0x%08x, expected 0x%08x.\n",
                    x, y, ((DWORD *)lock_rect.pBits)[lock_rect.Pitch / 4 * y + x],
                    dds_dxt5_8_8_expected_misaligned_1[y * 8 + x]);
        }
    }
    hr = IDirect3DSurface9_UnlockRect(uncompressed_surface);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DSurface9_LockRect(surface, &lock_rect, NULL, D3DLOCK_READONLY);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    for (y = 0; y < 2; ++y)
        memset(&((BYTE *)lock_rect.pBits)[y * lock_rect.Pitch], 0, 16 * 2);
    hr = IDirect3DSurface9_UnlockRect(surface);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = D3DXLoadSurfaceFromMemory(surface, NULL, &rect, &dds_dxt5_8_8[128],
            D3DFMT_DXT5, 16 * 2, NULL, NULL, D3DX_FILTER_POINT, 0);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = D3DXLoadSurfaceFromMemory(surface, NULL, &rect, &dds_dxt5_8_8[128],
            D3DFMT_DXT5, 16 * 2, NULL, &rect, D3DX_FILTER_POINT, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = D3DXLoadSurfaceFromSurface(uncompressed_surface, NULL, NULL, surface, NULL, NULL, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DSurface9_LockRect(uncompressed_surface, &lock_rect, NULL, D3DLOCK_READONLY);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    for (y = 0; y < 8; ++y)
    {
        for (x = 0; x < 8; ++x)
        {
            ok(compare_color(((DWORD *)lock_rect.pBits)[lock_rect.Pitch / 4 * y + x],
                    dds_dxt5_8_8_expected_misaligned_3[y * 8 + x], 0),
                    "Color at position %u, %u is 0x%08x, expected 0x%08x.\n",
                    x, y, ((DWORD *)lock_rect.pBits)[lock_rect.Pitch / 4 * y + x],
                    dds_dxt5_8_8_expected_misaligned_3[y * 8 + x]);
        }
    }
    hr = IDirect3DSurface9_UnlockRect(uncompressed_surface);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = D3DXLoadSurfaceFromFileInMemory(surface, NULL, &rect, dds_dxt5_8_8,
            sizeof(dds_dxt5_8_8), &rect, D3DX_FILTER_POINT, 0, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = D3DXLoadSurfaceFromSurface(uncompressed_surface, NULL, NULL, surface, NULL, NULL, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DSurface9_LockRect(uncompressed_surface, &lock_rect, NULL, D3DLOCK_READONLY);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    for (y = 0; y < 8; ++y)
    {
        for (x = 0; x < 8; ++x)
        {
            ok(compare_color(((DWORD *)lock_rect.pBits)[lock_rect.Pitch / 4 * y + x],
                    dds_dxt5_8_8_expected_misaligned_3[y * 8 + x], 0),
                    "Color at position %u, %u is 0x%08x, expected 0x%08x.\n",
                    x, y, ((DWORD *)lock_rect.pBits)[lock_rect.Pitch / 4 * y + x],
                    dds_dxt5_8_8_expected_misaligned_3[y * 8 + x]);
        }
    }
    hr = IDirect3DSurface9_UnlockRect(uncompressed_surface);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    IDirect3DSurface9_Release(uncompressed_surface);
    IDirect3DSurface9_Release(surface);
    IDirect3DTexture9_Release(texture);

    /* Volume textures work too. */
    hr = D3DXCreateTextureFromFileInMemory(device, dds_volume_map, sizeof(dds_volume_map), &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemory returned %#x, expected %#x.\n", hr, D3D_OK);
    type = IDirect3DTexture9_GetType(texture);
    ok(type == D3DRTYPE_TEXTURE, "IDirect3DTexture9_GetType returned %u, expected %u.\n", type, D3DRTYPE_TEXTURE);
    level_count = IDirect3DBaseTexture9_GetLevelCount((IDirect3DBaseTexture9 *)texture);
    todo_wine ok(level_count == 3, "Texture has %u mip levels, 3 expected.\n", level_count);
    hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &desc);
    ok(hr == D3D_OK, "IDirect3DTexture9_GetLevelDesc returned %#x, expected %#x.\n", hr, D3D_OK);
    ok(desc.Width == 4, "Width is %u, expected 4.\n", desc.Width);
    ok(desc.Height == 4, "Height is %u, expected 4.\n", desc.Height);

    if (has_2d_dxt3)
    {
        ok(desc.Format == D3DFMT_DXT3, "Unexpected texture format %#x.\n", desc.Format);
        hr = IDirect3DTexture9_LockRect(texture, 0, &lock_rect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "IDirect3DTexture9_LockRect returned %#x, expected %#x.\n", hr, D3D_OK);
        if (SUCCEEDED(hr))
        {
            for (i = 0; i < 16; ++i)
                ok(((BYTE *)lock_rect.pBits)[i] == dds_volume_map[128 + i],
                        "Byte at index %u is 0x%02x, expected 0x%02x.\n",
                        i, ((BYTE *)lock_rect.pBits)[i], dds_volume_map[128 + i]);
            IDirect3DTexture9_UnlockRect(texture, 0);
        }
    }
    else
    {
        ok(desc.Format == D3DFMT_A8R8G8B8, "Unexpected texture format %#x.\n", desc.Format);
        skip("D3DFMT_DXT3 volume textures are not supported, skipping a test.\n");
    }
    /* The lower texture levels are apparently generated by filtering the level 0 surface
     * I.e. following levels from the file are ignored. */
    IDirect3DTexture9_Release(texture);
}

static void test_D3DXCreateTextureFromFileInMemoryEx(IDirect3DDevice9 *device)
{
    HRESULT hr;
    IDirect3DTexture9 *texture;
    unsigned int miplevels;
    IDirect3DSurface9 *surface;
    D3DSURFACE_DESC desc;

    hr = D3DXCreateTextureFromFileInMemoryEx(device, dds_16bit, sizeof(dds_16bit), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT,
        0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemoryEx returned %#x, expected %#x\n", hr, D3D_OK);
    IDirect3DTexture9_Release(texture);

    hr = D3DXCreateTextureFromFileInMemoryEx(device, dds_16bit, sizeof(dds_16bit), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT,
        D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemoryEx returned %#x, expected %#x\n", hr, D3D_OK);
    IDirect3DTexture9_Release(texture);

    hr = D3DXCreateTextureFromFileInMemoryEx(device, dds_24bit, sizeof(dds_24bit), D3DX_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_SKIP_DDS_MIP_LEVELS(1, D3DX_FILTER_POINT), 0, NULL, NULL, &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemoryEx returned %#x, expected %#x\n", hr, D3D_OK);
    miplevels = IDirect3DTexture9_GetLevelCount(texture);
    ok(miplevels == 1, "Got miplevels %u, expected %u.\n", miplevels, 1);
    IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    IDirect3DSurface9_GetDesc(surface, &desc);
    ok(desc.Width == 1 && desc.Height == 1,
            "Surface dimensions are %ux%u, expected 1x1.\n", desc.Width, desc.Height);
    IDirect3DSurface9_Release(surface);
    IDirect3DTexture9_Release(texture);

    if (!is_autogenmipmap_supported(device, D3DRTYPE_TEXTURE))
    {
        skip("No D3DUSAGE_AUTOGENMIPMAP support for textures\n");
        return;
    }

    hr = D3DXCreateTextureFromFileInMemoryEx(device, dds_16bit, sizeof(dds_16bit), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT,
        D3DUSAGE_DYNAMIC | D3DUSAGE_AUTOGENMIPMAP, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemoryEx returned %#x, expected %#x\n", hr, D3D_OK);
    IDirect3DTexture9_Release(texture);

    /* Checking for color key format overrides. */
    hr = D3DXCreateTextureFromFileInMemoryEx(device, dds_16bit, sizeof(dds_16bit),
            D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemoryEx returned %#x, expected %#x.\n", hr, D3D_OK);
    IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    IDirect3DSurface9_GetDesc(surface, &desc);
    ok(desc.Format == D3DFMT_X1R5G5B5, "Returned format %u, expected %u.\n", desc.Format, D3DFMT_X1R5G5B5);
    IDirect3DSurface9_Release(surface);
    IDirect3DTexture9_Release(texture);
    hr = D3DXCreateTextureFromFileInMemoryEx(device, dds_16bit, sizeof(dds_16bit),
            D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, 0xff000000, NULL, NULL, &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemoryEx returned %#x, expected %#x.\n", hr, D3D_OK);
    IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    IDirect3DSurface9_GetDesc(surface, &desc);
    ok(desc.Format == D3DFMT_A1R5G5B5, "Returned format %u, expected %u.\n", desc.Format, D3DFMT_A1R5G5B5);
    IDirect3DSurface9_Release(surface);
    IDirect3DTexture9_Release(texture);
    hr = D3DXCreateTextureFromFileInMemoryEx(device, dds_16bit, sizeof(dds_16bit),
            D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_X1R5G5B5, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, 0xff000000, NULL, NULL, &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemoryEx returned %#x, expected %#x.\n", hr, D3D_OK);
    IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    IDirect3DSurface9_GetDesc(surface, &desc);
    ok(desc.Format == D3DFMT_X1R5G5B5, "Returned format %u, expected %u.\n", desc.Format, D3DFMT_X1R5G5B5);
    IDirect3DSurface9_Release(surface);
    IDirect3DTexture9_Release(texture);

    hr = D3DXCreateTextureFromFileInMemoryEx(device, dds_24bit, sizeof(dds_24bit),
            D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemoryEx returned %#x, expected %#x.\n", hr, D3D_OK);
    IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    IDirect3DSurface9_GetDesc(surface, &desc);
    ok(desc.Format == D3DFMT_X8R8G8B8, "Returned format %u, expected %u.\n", desc.Format, D3DFMT_X8R8G8B8);
    IDirect3DSurface9_Release(surface);
    IDirect3DTexture9_Release(texture);
    hr = D3DXCreateTextureFromFileInMemoryEx(device, dds_24bit, sizeof(dds_24bit),
            D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, 0xff000000, NULL, NULL, &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemoryEx returned %#x, expected %#x.\n", hr, D3D_OK);
    IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    IDirect3DSurface9_GetDesc(surface, &desc);
    ok(desc.Format == D3DFMT_A8R8G8B8, "Returned format %u, expected %u.\n", desc.Format, D3DFMT_A8R8G8B8);
    IDirect3DSurface9_Release(surface);
    IDirect3DTexture9_Release(texture);
    hr = D3DXCreateTextureFromFileInMemoryEx(device, dds_24bit, sizeof(dds_24bit),
            D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, 0xff000000, NULL, NULL, &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemoryEx returned %#x, expected %#x.\n", hr, D3D_OK);
    IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    IDirect3DSurface9_GetDesc(surface, &desc);
    ok(desc.Format == D3DFMT_X8R8G8B8, "Returned format %u, expected %u.\n", desc.Format, D3DFMT_X8R8G8B8);
    IDirect3DSurface9_Release(surface);
    IDirect3DTexture9_Release(texture);

    hr = D3DXCreateTextureFromFileInMemoryEx(device, png_grayscale, sizeof(png_grayscale),
            D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemoryEx returned %#x, expected %#x.\n", hr, D3D_OK);
    IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    IDirect3DSurface9_GetDesc(surface, &desc);
    ok(desc.Format == D3DFMT_L8, "Returned format %u, expected %u.\n", desc.Format, D3DFMT_L8);
    IDirect3DSurface9_Release(surface);
    IDirect3DTexture9_Release(texture);
    hr = D3DXCreateTextureFromFileInMemoryEx(device, png_grayscale, sizeof(png_grayscale),
            D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, 0xff000000, NULL, NULL, &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemoryEx returned %#x, expected %#x.\n", hr, D3D_OK);
    IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    IDirect3DSurface9_GetDesc(surface, &desc);
    ok(desc.Format == D3DFMT_A8L8, "Returned format %u, expected %u.\n", desc.Format, D3DFMT_A8L8);
    IDirect3DSurface9_Release(surface);
    IDirect3DTexture9_Release(texture);
    hr = D3DXCreateTextureFromFileInMemoryEx(device, png_grayscale, sizeof(png_grayscale),
            D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_L8, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, 0xff000000, NULL, NULL, &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemoryEx returned %#x, expected %#x.\n", hr, D3D_OK);
    IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    IDirect3DSurface9_GetDesc(surface, &desc);
    ok(desc.Format == D3DFMT_L8, "Returned format %u, expected %u.\n", desc.Format, D3DFMT_L8);
    IDirect3DSurface9_Release(surface);
    IDirect3DTexture9_Release(texture);
}

static void test_D3DXCreateCubeTextureFromFileInMemory(IDirect3DDevice9 *device)
{
    HRESULT hr;
    ULONG ref;
    DWORD levelcount;
    IDirect3DCubeTexture9 *cube_texture;
    D3DSURFACE_DESC surface_desc;

    hr = D3DXCreateCubeTextureFromFileInMemory(NULL, dds_cube_map, sizeof(dds_cube_map), &cube_texture);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateCubeTextureFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateCubeTextureFromFileInMemory(device, NULL, sizeof(dds_cube_map), &cube_texture);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateCubeTextureFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateCubeTextureFromFileInMemory(device, dds_cube_map, 0, &cube_texture);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateCubeTextureFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateCubeTextureFromFileInMemory(device, dds_cube_map, sizeof(dds_cube_map), NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateCubeTextureFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateCubeTextureFromFileInMemory(device, dds_cube_map, sizeof(dds_cube_map), &cube_texture);
    if (SUCCEEDED(hr))
    {
        levelcount = IDirect3DCubeTexture9_GetLevelCount(cube_texture);
        ok(levelcount == 3, "GetLevelCount returned %u, expected 3\n", levelcount);

        hr = IDirect3DCubeTexture9_GetLevelDesc(cube_texture, 0, &surface_desc);
        ok(hr == D3D_OK, "GetLevelDesc returned %#x, expected %#x\n", hr, D3D_OK);
        ok(surface_desc.Width == 4, "Got width %u, expected 4\n", surface_desc.Width);
        ok(surface_desc.Height == 4, "Got height %u, expected 4\n", surface_desc.Height);

        ref = IDirect3DCubeTexture9_Release(cube_texture);
        ok(ref == 0, "Invalid reference count. Got %u, expected 0\n", ref);
    } else skip("Couldn't create cube texture\n");
}

static void test_D3DXCreateCubeTextureFromFileInMemoryEx(IDirect3DDevice9 *device)
{
    IDirect3DCubeTexture9 *cube_texture;
    HRESULT hr;

    hr = D3DXCreateCubeTextureFromFileInMemoryEx(device, dds_cube_map, sizeof(dds_cube_map), D3DX_DEFAULT,
            D3DX_DEFAULT, D3DUSAGE_RENDERTARGET, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT,
            D3DX_DEFAULT, 0, NULL, NULL, &cube_texture);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    IDirect3DCubeTexture9_Release(cube_texture);

    if (!is_autogenmipmap_supported(device, D3DRTYPE_CUBETEXTURE))
    {
        skip("No D3DUSAGE_AUTOGENMIPMAP support for cube textures\n");
        return;
    }

    hr = D3DXCreateCubeTextureFromFileInMemoryEx(device, dds_cube_map, sizeof(dds_cube_map), D3DX_DEFAULT,
            D3DX_DEFAULT, D3DUSAGE_DYNAMIC | D3DUSAGE_AUTOGENMIPMAP, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &cube_texture);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    IDirect3DCubeTexture9_Release(cube_texture);
}

static void test_D3DXCreateVolumeTextureFromFileInMemory(IDirect3DDevice9 *device)
{
    HRESULT hr;
    ULONG ref;
    DWORD levelcount;
    IDirect3DVolumeTexture9 *volume_texture;
    D3DVOLUME_DESC volume_desc;

    hr = D3DXCreateVolumeTextureFromFileInMemory(NULL, dds_volume_map, sizeof(dds_volume_map), &volume_texture);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateVolumeTextureFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateVolumeTextureFromFileInMemory(device, NULL, sizeof(dds_volume_map), &volume_texture);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateVolumeTextureFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateVolumeTextureFromFileInMemory(device, dds_volume_map, 0, &volume_texture);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateVolumeTextureFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateVolumeTextureFromFileInMemory(device, dds_volume_map, sizeof(dds_volume_map), NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateVolumeTextureFromFileInMemory returned %#x, expected %#x\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateVolumeTextureFromFileInMemory(device, dds_volume_map, sizeof(dds_volume_map), &volume_texture);
    ok(hr == D3D_OK, "D3DXCreateVolumeTextureFromFileInMemory returned %#x, expected %#x.\n", hr, D3D_OK);
    levelcount = IDirect3DVolumeTexture9_GetLevelCount(volume_texture);
    ok(levelcount == 3, "GetLevelCount returned %u, expected 3.\n", levelcount);

    hr = IDirect3DVolumeTexture9_GetLevelDesc(volume_texture, 0, &volume_desc);
    ok(hr == D3D_OK, "GetLevelDesc returned %#x, expected %#x.\n", hr, D3D_OK);
    ok(volume_desc.Width == 4, "Got width %u, expected 4.\n", volume_desc.Width);
    ok(volume_desc.Height == 4, "Got height %u, expected 4.\n", volume_desc.Height);
    ok(volume_desc.Depth == 2, "Got depth %u, expected 2.\n", volume_desc.Depth);
    ok(volume_desc.Pool == D3DPOOL_MANAGED, "Got pool %u, expected D3DPOOL_MANAGED.\n", volume_desc.Pool);

    hr = IDirect3DVolumeTexture9_GetLevelDesc(volume_texture, 1, &volume_desc);
    ok(hr == D3D_OK, "GetLevelDesc returned %#x, expected %#x.\n", hr, D3D_OK);
    ok(volume_desc.Width == 2, "Got width %u, expected 2.\n", volume_desc.Width);
    ok(volume_desc.Height == 2, "Got height %u, expected 2.\n", volume_desc.Height);
    ok(volume_desc.Depth == 1, "Got depth %u, expected 1.\n", volume_desc.Depth);

    ref = IDirect3DVolumeTexture9_Release(volume_texture);
    ok(ref == 0, "Invalid reference count. Got %u, expected 0.\n", ref);
}

static void test_D3DXCreateVolumeTextureFromFileInMemoryEx(IDirect3DDevice9 *device)
{
    IDirect3DVolumeTexture9 *volume_texture;
    HRESULT hr;

    hr = D3DXCreateVolumeTextureFromFileInMemoryEx(device, dds_volume_map, sizeof(dds_volume_map), D3DX_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, 1, D3DUSAGE_RENDERTARGET, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT,
            D3DX_DEFAULT, 0, NULL, NULL, &volume_texture);
    ok(hr == D3DERR_NOTAVAILABLE, "Got unexpected hr %#x.\n", hr);

    hr = D3DXCreateVolumeTextureFromFileInMemoryEx(device, dds_volume_map, sizeof(dds_volume_map), D3DX_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, 1, D3DUSAGE_DEPTHSTENCIL, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT,
            D3DX_DEFAULT, 0, NULL, NULL, &volume_texture);
    ok(hr == D3DERR_NOTAVAILABLE, "Got unexpected hr %#x.\n", hr);
}

/* fills positive x face with red color */
static void WINAPI fill_cube_positive_x(D3DXVECTOR4 *out, const D3DXVECTOR3 *tex_coord, const D3DXVECTOR3 *texel_size, void *data)
{
    memset(out, 0, sizeof(*out));
    if (tex_coord->x > 0 && fabs(tex_coord->x) > fabs(tex_coord->y) && fabs(tex_coord->x) > fabs(tex_coord->z))
        out->x = 1;
}

static void test_D3DXSaveTextureToFileInMemory(IDirect3DDevice9 *device)
{
    HRESULT hr;
    IDirect3DTexture9 *texture;
    IDirect3DCubeTexture9 *cube_texture;
    IDirect3DVolumeTexture9 *volume_texture;
    ID3DXBuffer *buffer;
    void *buffer_pointer;
    DWORD buffer_size;
    D3DXIMAGE_INFO info;
    D3DXIMAGE_FILEFORMAT file_format;

    /* textures */
    hr = IDirect3DDevice9_CreateTexture(device, 256, 256, 0, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &texture, NULL);
    if (FAILED(hr))
    {
        skip("Failed to create texture\n");
        return;
    }

    for (file_format = D3DXIFF_BMP; file_format <= D3DXIFF_JPG; file_format++)
    {
        hr = D3DXSaveTextureToFileInMemory(&buffer, file_format, (IDirect3DBaseTexture9 *)texture, NULL);
        ok(hr == D3D_OK, "D3DXSaveTextureToFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);
        if (SUCCEEDED(hr))
        {
            buffer_pointer = ID3DXBuffer_GetBufferPointer(buffer);
            buffer_size = ID3DXBuffer_GetBufferSize(buffer);
            hr = D3DXGetImageInfoFromFileInMemory(buffer_pointer, buffer_size, &info);
            ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);

            ok(info.Width == 256, "Got width %u, expected %u\n", info.Width, 256);
            ok(info.Height == 256, "Got height %u, expected %u\n", info.Height, 256);
            ok(info.MipLevels == 1, "Got miplevels %u, expected %u\n", info.MipLevels, 1);
            ok(info.ResourceType == D3DRTYPE_TEXTURE, "Got resource type %#x, expected %#x\n", info.ResourceType, D3DRTYPE_TEXTURE);
            ok(info.ImageFileFormat == file_format, "Got file format %#x, expected %#x\n", info.ImageFileFormat, file_format);
            ID3DXBuffer_Release(buffer);
        }
    }

    todo_wine {
    hr = D3DXSaveTextureToFileInMemory(&buffer, D3DXIFF_DDS, (IDirect3DBaseTexture9 *)texture, NULL);
    ok(hr == D3D_OK, "D3DXSaveTextureToFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);
    if (SUCCEEDED(hr))
    {
        buffer_pointer = ID3DXBuffer_GetBufferPointer(buffer);
        buffer_size = ID3DXBuffer_GetBufferSize(buffer);
        hr = D3DXGetImageInfoFromFileInMemory(buffer_pointer, buffer_size, &info);
        ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);

        ok(info.Width == 256, "Got width %u, expected %u\n", info.Width, 256);
        ok(info.Height == 256, "Got height %u, expected %u\n", info.Height, 256);
        ok(info.MipLevels == 9, "Got miplevels %u, expected %u\n", info.MipLevels, 9);
        ok(info.ResourceType == D3DRTYPE_TEXTURE, "Got resource type %#x, expected %#x\n", info.ResourceType, D3DRTYPE_TEXTURE);
        ok(info.ImageFileFormat == D3DXIFF_DDS, "Got file format %#x, expected %#x\n", info.ImageFileFormat, D3DXIFF_DDS);
        ID3DXBuffer_Release(buffer);
    }
    }

    IDirect3DTexture9_Release(texture);

    /* cube textures */
    hr = IDirect3DDevice9_CreateCubeTexture(device, 256, 0, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &cube_texture, NULL);
    if (FAILED(hr))
    {
        skip("Failed to create cube texture\n");
        return;
    }

    hr = D3DXFillCubeTexture(cube_texture, fill_cube_positive_x, NULL);
    ok(hr == D3D_OK, "D3DXFillCubeTexture returned %#x, expected %#x\n", hr, D3D_OK);

    hr = D3DXSaveTextureToFileInMemory(&buffer, D3DXIFF_BMP, (IDirect3DBaseTexture9 *)cube_texture, NULL);
    ok(hr == D3D_OK, "D3DXSaveTextureToFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);
    if (SUCCEEDED(hr))
    {
        IDirect3DSurface9 *surface;

        buffer_pointer = ID3DXBuffer_GetBufferPointer(buffer);
        buffer_size = ID3DXBuffer_GetBufferSize(buffer);
        hr = D3DXGetImageInfoFromFileInMemory(buffer_pointer, buffer_size, &info);
        ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);

        ok(info.Width == 256, "Got width %u, expected %u\n", info.Width, 256);
        ok(info.Height == 256, "Got height %u, expected %u\n", info.Height, 256);
        ok(info.MipLevels == 1, "Got miplevels %u, expected %u\n", info.MipLevels, 1);
        ok(info.ResourceType == D3DRTYPE_TEXTURE, "Got resource type %#x, expected %#x\n", info.ResourceType, D3DRTYPE_TEXTURE);
        ok(info.ImageFileFormat == D3DXIFF_BMP, "Got file format %#x, expected %#x\n", info.ImageFileFormat, D3DXIFF_BMP);

        /* positive x face is saved */
        hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 256, 256, D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, &surface, NULL);
        if (SUCCEEDED(hr))
        {
            D3DLOCKED_RECT locked_rect;

            hr = D3DXLoadSurfaceFromFileInMemory(surface, NULL, NULL, buffer_pointer, buffer_size, NULL, D3DX_FILTER_NONE, 0, NULL);
            ok(hr == D3D_OK, "D3DXLoadSurfaceFromFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);

            hr = IDirect3DSurface9_LockRect(surface, &locked_rect, NULL, D3DLOCK_READONLY);
            if (SUCCEEDED(hr))
            {
                DWORD *color = locked_rect.pBits;
                ok(*color == 0x00ff0000, "Got color %#x, expected %#x\n", *color, 0x00ff0000);
                IDirect3DSurface9_UnlockRect(surface);
            }

            IDirect3DSurface9_Release(surface);
        } else skip("Failed to create surface\n");

        ID3DXBuffer_Release(buffer);
    }

    todo_wine {
    hr = D3DXSaveTextureToFileInMemory(&buffer, D3DXIFF_DDS, (IDirect3DBaseTexture9 *)cube_texture, NULL);
    ok(hr == D3D_OK, "D3DXSaveTextureToFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);
    if (SUCCEEDED(hr))
    {
        buffer_pointer = ID3DXBuffer_GetBufferPointer(buffer);
        buffer_size = ID3DXBuffer_GetBufferSize(buffer);
        hr = D3DXGetImageInfoFromFileInMemory(buffer_pointer, buffer_size, &info);
        ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);

        ok(info.Width == 256, "Got width %u, expected %u\n", info.Width, 256);
        ok(info.Height == 256, "Got height %u, expected %u\n", info.Height, 256);
        ok(info.MipLevels == 9, "Got miplevels %u, expected %u\n", info.MipLevels, 9);
        ok(info.ResourceType == D3DRTYPE_CUBETEXTURE, "Got resource type %#x, expected %#x\n", info.ResourceType, D3DRTYPE_CUBETEXTURE);
        ok(info.ImageFileFormat == D3DXIFF_DDS, "Got file format %#x, expected %#x\n", info.ImageFileFormat, D3DXIFF_DDS);
        ID3DXBuffer_Release(buffer);
    }
    }

    IDirect3DCubeTexture9_Release(cube_texture);

    /* volume textures */
    hr = IDirect3DDevice9_CreateVolumeTexture(device, 256, 256, 256, 0, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &volume_texture, NULL);
    if (FAILED(hr))
    {
        skip("Failed to create volume texture\n");
        return;
    }

    todo_wine {
    hr = D3DXSaveTextureToFileInMemory(&buffer, D3DXIFF_BMP, (IDirect3DBaseTexture9 *)volume_texture, NULL);
    ok(hr == D3D_OK, "D3DXSaveTextureToFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);
    if (SUCCEEDED(hr))
    {
        buffer_pointer = ID3DXBuffer_GetBufferPointer(buffer);
        buffer_size = ID3DXBuffer_GetBufferSize(buffer);
        hr = D3DXGetImageInfoFromFileInMemory(buffer_pointer, buffer_size, &info);
        ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);

        ok(info.Width == 256, "Got width %u, expected %u\n", info.Width, 256);
        ok(info.Height == 256, "Got height %u, expected %u\n", info.Height, 256);
        ok(info.Depth == 1, "Got depth %u, expected %u\n", info.Depth, 1);
        ok(info.MipLevels == 1, "Got miplevels %u, expected %u\n", info.MipLevels, 1);
        ok(info.ResourceType == D3DRTYPE_TEXTURE, "Got resource type %#x, expected %#x\n", info.ResourceType, D3DRTYPE_TEXTURE);
        ok(info.ImageFileFormat == D3DXIFF_BMP, "Got file format %#x, expected %#x\n", info.ImageFileFormat, D3DXIFF_BMP);
        ID3DXBuffer_Release(buffer);
    }

    hr = D3DXSaveTextureToFileInMemory(&buffer, D3DXIFF_DDS, (IDirect3DBaseTexture9 *)volume_texture, NULL);
    ok(hr == D3D_OK, "D3DXSaveTextureToFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);
    if (SUCCEEDED(hr))
    {
        buffer_pointer = ID3DXBuffer_GetBufferPointer(buffer);
        buffer_size = ID3DXBuffer_GetBufferSize(buffer);
        hr = D3DXGetImageInfoFromFileInMemory(buffer_pointer, buffer_size, &info);
        ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#x, expected %#x\n", hr, D3D_OK);

        ok(info.Width == 256, "Got width %u, expected %u\n", info.Width, 256);
        ok(info.Height == 256, "Got height %u, expected %u\n", info.Height, 256);
        ok(info.Depth == 256, "Got depth %u, expected %u\n", info.Depth, 256);
        ok(info.MipLevels == 9, "Got miplevels %u, expected %u\n", info.MipLevels, 9);
        ok(info.ResourceType == D3DRTYPE_VOLUMETEXTURE, "Got resource type %#x, expected %#x\n", info.ResourceType, D3DRTYPE_VOLUMETEXTURE);
        ok(info.ImageFileFormat == D3DXIFF_DDS, "Got file format %#x, expected %#x\n", info.ImageFileFormat, D3DXIFF_DDS);
        ID3DXBuffer_Release(buffer);
    }
    }

    IDirect3DVolumeTexture9_Release(volume_texture);
}

static void test_texture_shader(void)
{
    static const DWORD shader_zero[] = {0x0};
    static const DWORD shader_invalid[] = {0xeeee0100};
    static const DWORD shader_empty[] = {0xfffe0200, 0x0000ffff};
#if 0
float4 main(float3 pos : POSITION, float3 size : PSIZE) : COLOR
{
    return float4(pos, 1.0);
}
#endif
    static const DWORD shader_code[] =
    {
        0x54580100, 0x0015fffe, 0x42415443, 0x0000001c, 0x0000001f, 0x54580100, 0x00000000, 0x00000000,
        0x00000100, 0x0000001c, 0x4d007874, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820, 0x6853204c,
        0x72656461, 0x6d6f4320, 0x656c6970, 0x2e392072, 0x392e3932, 0x332e3235, 0x00313131, 0x000afffe,
        0x54494c43, 0x00000004, 0x00000000, 0x3ff00000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x0014fffe, 0x434c5846, 0x00000002, 0x10000003, 0x00000001, 0x00000000,
        0x00000003, 0x00000000, 0x00000000, 0x00000004, 0x00000000, 0x10000001, 0x00000001, 0x00000000,
        0x00000001, 0x00000000, 0x00000000, 0x00000004, 0x00000003, 0xf0f0f0f0, 0x0f0f0f0f, 0x0000ffff,
    };
    IDirect3DVolumeTexture9 *volume_texture;
    IDirect3DCubeTexture9 *cube_texture;
    D3DPRESENT_PARAMETERS d3dpp;
    IDirect3DTexture9 *texture;
    IDirect3DDevice9 *device;
    ID3DXTextureShader *tx;
    unsigned int x, y, z;
    ID3DXBuffer *buffer;
    unsigned int *data;
    D3DLOCKED_RECT lr;
    D3DLOCKED_BOX lb;
    IDirect3D9 *d3d;
    D3DCAPS9 caps;
    HRESULT hr;
    HWND wnd;

    hr = D3DXCreateTextureShader(NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = D3DXCreateTextureShader(NULL, &tx);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = D3DXCreateTextureShader(shader_invalid, &tx);
    todo_wine ok(hr == D3DXERR_INVALIDDATA, "Got unexpected hr %#x.\n", hr);

    hr = D3DXCreateTextureShader(shader_zero, &tx);
    todo_wine ok(hr == D3DXERR_INVALIDDATA, "Got unexpected hr %#x.\n", hr);

    hr = D3DXCreateTextureShader(shader_empty, &tx);
    todo_wine ok(hr == D3DXERR_INVALIDDATA, "Got unexpected hr %#x.\n", hr);

    hr = D3DXCreateTextureShader(shader_code, &tx);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);

    hr = tx->lpVtbl->GetFunction(tx, &buffer);
    todo_wine ok(SUCCEEDED(hr), "Failed to get texture shader bytecode.\n");
    if (FAILED(hr))
    {
        skip("Texture shaders not supported, skipping further tests.\n");
        IUnknown_Release(tx);
        return;
    }
    ID3DXBuffer_Release(buffer);

    if (!(wnd = CreateWindowA("static", "d3dx9_test", WS_OVERLAPPEDWINDOW, 0, 0,
            640, 480, NULL, NULL, NULL, NULL)))
    {
        skip("Couldn't create application window.\n");
        IUnknown_Release(tx);
        return;
    }
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d)
    {
        skip("Couldn't create IDirect3D9 object.\n");
        DestroyWindow(wnd);
        IUnknown_Release(tx);
        return;
    }

    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &device);
    if (FAILED(hr))
    {
        skip("Failed to create IDirect3DDevice9 object, hr %#x.\n", hr);
        IDirect3D9_Release(d3d);
        DestroyWindow(wnd);
        IUnknown_Release(tx);
        return;
    }

    IDirect3DDevice9_GetDeviceCaps(device, &caps);

    hr = IDirect3DDevice9_CreateTexture(device, 256, 256, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM,
            &texture, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);

    hr = D3DXFillTextureTX(texture, tx);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DTexture9_LockRect(texture, 0, &lr, NULL, D3DLOCK_READONLY);
    ok(SUCCEEDED(hr), "Locking texture failed, hr %#x.\n", hr);
    data = lr.pBits;
    for (y = 0; y < 256; ++y)
    {
        for (x = 0; x < 256; ++x)
        {
            unsigned int expected = 0xff000000 | x << 16 | y << 8;
            /* The third position coordinate is apparently undefined for 2D textures. */
            unsigned int color = data[y * lr.Pitch / sizeof(*data) + x] & 0xffffff00;

            ok(compare_color(color, expected, 1), "Unexpected color %08x at (%u, %u).\n", color, x, y);
        }
    }
    hr = IDirect3DTexture9_UnlockRect(texture, 0);
    ok(SUCCEEDED(hr), "Unlocking texture failed, hr %#x.\n", hr);

    IDirect3DTexture9_Release(texture);

    if (!(caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP))
    {
        skip("Cube textures not supported, skipping tests.\n");
        goto cleanup;
    }

    hr = IDirect3DDevice9_CreateCubeTexture(device, 256, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM,
            &cube_texture, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);

    hr = D3DXFillCubeTextureTX(cube_texture, tx);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);

    for (z = 0; z < 6; ++z)
    {
        static const char * const mapping[6][3] =
        {
            {"-x", "-y", "1"},
            {"+x", "-y", "0"},
            {"+y", "1", "+x"},
            {"-y", "0", "+x"},
            {"1", "-y", "+x"},
            {"0", "-y", "-x"},
        };

        hr = IDirect3DCubeTexture9_LockRect(cube_texture, z, 0, &lr, NULL, D3DLOCK_READONLY);
        ok(SUCCEEDED(hr), "Locking texture failed, hr %#x.\n", hr);
        data = lr.pBits;
        for (y = 0; y < 256; ++y)
        {
            for (x = 0; x < 256; ++x)
            {
                unsigned int color = data[y * lr.Pitch / sizeof(*data) + x];
                unsigned int expected = 0xff000000;
                unsigned int i;

                for (i = 0; i < 3; ++i)
                {
                    int component;

                    if (mapping[z][i][0] == '0')
                        component = 0;
                    else if (mapping[z][i][0] == '1')
                        component = 255;
                    else
                        component = mapping[z][i][1] == 'x' ? x * 2 - 255 : y * 2 - 255;
                    if (mapping[z][i][0] == '-')
                        component = -component;
                    expected |= max(component, 0) << i * 8;
                }
                ok(compare_color(color, expected, 1), "Unexpected color %08x at (%u, %u, %u).\n",
                        color, x, y, z);
            }
        }
        hr = IDirect3DCubeTexture9_UnlockRect(cube_texture, z, 0);
        ok(SUCCEEDED(hr), "Unlocking texture failed, hr %#x.\n", hr);
    }

    IDirect3DCubeTexture9_Release(cube_texture);

    if (!(caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP) || caps.MaxVolumeExtent < 64)
    {
        skip("Volume textures not supported, skipping test.\n");
        goto cleanup;
    }
    hr = IDirect3DDevice9_CreateVolumeTexture(device, 64, 64, 64, 1, 0, D3DFMT_A8R8G8B8,
            D3DPOOL_SYSTEMMEM, &volume_texture, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);

    hr = D3DXFillVolumeTextureTX(volume_texture, tx);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DVolumeTexture9_LockBox(volume_texture, 0, &lb, NULL, D3DLOCK_READONLY);
    ok(SUCCEEDED(hr), "Locking texture failed, hr %#x.\n", hr);
    data = lb.pBits;
    for (z = 0; z < 64; ++z)
    {
        for (y = 0; y < 64; ++y)
        {
            for (x = 0; x < 64; ++x)
            {
                unsigned int expected = 0xff000000 | ((x * 4 + 2) << 16) | ((y * 4 + 2) << 8) | (z * 4 + 2);
                unsigned int color = data[z * lb.SlicePitch / sizeof(*data) + y * lb.RowPitch / sizeof(*data) + x];

                ok(compare_color(color, expected, 1), "Unexpected color %08x at (%u, %u, %u).\n",
                        color, x, y, z);
            }
        }
    }
    hr = IDirect3DVolumeTexture9_UnlockBox(volume_texture, 0);
    ok(SUCCEEDED(hr), "Unlocking texture failed, hr %#x.\n", hr);

    IDirect3DVolumeTexture9_Release(volume_texture);

 cleanup:
    IDirect3DDevice9_Release(device);
    IDirect3D9_Release(d3d);
    DestroyWindow(wnd);
    IUnknown_Release(tx);
}

START_TEST(texture)
{
    HWND wnd;
    IDirect3D9 *d3d;
    IDirect3DDevice9 *device;
    D3DPRESENT_PARAMETERS d3dpp;
    HRESULT hr;
    ULONG ref;

    if (!(wnd = CreateWindowA("static", "d3dx9_test", WS_OVERLAPPEDWINDOW, 0, 0,
            640, 480, NULL, NULL, NULL, NULL)))
    {
        skip("Couldn't create application window\n");
        return;
    }
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d) {
        skip("Couldn't create IDirect3D9 object\n");
        DestroyWindow(wnd);
        return;
    }

    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed   = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &device);
    if (FAILED(hr)) {
        skip("Failed to create IDirect3DDevice9 object %#x\n", hr);
        IDirect3D9_Release(d3d);
        DestroyWindow(wnd);
        return;
    }

    /* Check whether DXTn textures are supported. */
    has_2d_dxt1 = SUCCEEDED(IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, 0, D3DRTYPE_TEXTURE, D3DFMT_DXT1));
    has_2d_dxt3 = SUCCEEDED(IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, 0, D3DRTYPE_TEXTURE, D3DFMT_DXT3));
    has_2d_dxt5 = SUCCEEDED(IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, 0, D3DRTYPE_TEXTURE, D3DFMT_DXT5));
    has_cube_dxt5 = SUCCEEDED(IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, 0, D3DRTYPE_CUBETEXTURE, D3DFMT_DXT5));
    has_3d_dxt3 = SUCCEEDED(IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, 0, D3DRTYPE_VOLUMETEXTURE, D3DFMT_DXT3));
    trace("DXTn texture support: 2D DXT1 %#x, 2D DXT3 %#x, 2D DXT5 %#x, cube DXT5 %#x, 3D dxt3 %#x.\n",
            has_2d_dxt1, has_2d_dxt3, has_2d_dxt5, has_cube_dxt5, has_3d_dxt3);

    test_D3DXCheckTextureRequirements(device);
    test_D3DXCheckCubeTextureRequirements(device);
    test_D3DXCheckVolumeTextureRequirements(device);
    test_D3DXCreateTexture(device);
    test_D3DXFilterTexture(device);
    test_D3DXFillTexture(device);
    test_D3DXFillCubeTexture(device);
    test_D3DXFillVolumeTexture(device);
    test_D3DXCreateTextureFromFileInMemory(device);
    test_D3DXCreateTextureFromFileInMemoryEx(device);
    test_D3DXCreateCubeTextureFromFileInMemory(device);
    test_D3DXCreateCubeTextureFromFileInMemoryEx(device);
    test_D3DXCreateVolumeTextureFromFileInMemory(device);
    test_D3DXCreateVolumeTextureFromFileInMemoryEx(device);
    test_D3DXSaveTextureToFileInMemory(device);

    ref = IDirect3DDevice9_Release(device);
    ok(!ref, "Device has %u references left.\n", ref);

    IDirect3D9_Release(d3d);
    DestroyWindow(wnd);

    test_texture_shader();
}
