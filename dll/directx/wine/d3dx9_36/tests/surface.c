/*
 * Tests for the D3DX9 surface functions
 *
 * Copyright 2009 Tony Wasserka
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
#include <assert.h>
#include "wine/test.h"
#include "d3dx9tex.h"
#include "resources.h"
#include <stdint.h>
#include "d3dx9_test_images.h"

/*
 * MAKE_DDHRESULT is first defined in d3dx9.h, with the same definition as the
 * one in ddraw.h.
 */
#ifdef MAKE_DDHRESULT
#undef MAKE_DDHRESULT
#endif
#include "ddraw.h"

static BOOL compare_uint(uint32_t x, uint32_t y, uint32_t max_diff)
{
    uint32_t diff = x > y ? x - y : y - x;

    return diff <= max_diff;
}

static BOOL compare_float(float f, float g, uint32_t ulps)
{
    int32_t x = *(int32_t *)&f;
    int32_t y = *(int32_t *)&g;

    if (x < 0)
        x = INT_MIN - x;
    if (y < 0)
        y = INT_MIN - y;

    return compare_uint(x, y, ulps);
}

#define check_release(obj, exp) _check_release(__LINE__, obj, exp)
static inline void _check_release(unsigned int line, IUnknown *obj, int exp)
{
    int ref = IUnknown_Release(obj);
    ok_(__FILE__, line)(ref == exp, "Invalid refcount. Expected %d, got %d\n", exp, ref);
}

struct surface_readback
{
    IDirect3DSurface9 *surface;
    D3DLOCKED_RECT locked_rect;
};

static uint32_t get_readback_color(struct surface_readback *rb, uint32_t x, uint32_t y)
{
    return rb->locked_rect.pBits
            ? ((uint32_t *)rb->locked_rect.pBits)[y * rb->locked_rect.Pitch / sizeof(uint32_t) + x] : 0xdeadbeef;
}

static void release_surface_readback(struct surface_readback *rb)
{
    HRESULT hr;

    if (!rb->surface)
        return;
    if (rb->locked_rect.pBits && FAILED(hr = IDirect3DSurface9_UnlockRect(rb->surface)))
        trace("Can't unlock the offscreen surface, hr %#lx.\n", hr);
    IDirect3DSurface9_Release(rb->surface);
}

static void get_surface_readback(IDirect3DDevice9 *device, IDirect3DSurface9 *src_surface, D3DFORMAT rb_format,
        struct surface_readback *rb)
{
    D3DSURFACE_DESC desc;
    HRESULT hr;

    memset(rb, 0, sizeof(*rb));
    hr = IDirect3DSurface9_GetDesc(src_surface, &desc);
    if (FAILED(hr))
    {
        trace("Failed to get source surface description, hr %#lx.\n", hr);
        return;
    }

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, desc.Width, desc.Height, rb_format, D3DPOOL_SYSTEMMEM,
            &rb->surface, NULL);
    if (FAILED(hr))
    {
        trace("Can't create the readback surface, hr %#lx.\n", hr);
        return;
    }

    hr = D3DXLoadSurfaceFromSurface(rb->surface, NULL, NULL, src_surface, NULL, NULL, D3DX_FILTER_NONE, 0);
    if (FAILED(hr))
    {
        trace("Can't load the readback surface, hr %#lx.\n", hr);
        IDirect3DSurface9_Release(rb->surface);
        rb->surface = NULL;
        return;
    }

    hr = IDirect3DSurface9_LockRect(rb->surface, &rb->locked_rect, NULL, D3DLOCK_READONLY);
    if (FAILED(hr))
    {
        trace("Can't lock the offscreen surface, hr %#lx.\n", hr);
        IDirect3DSurface9_Release(rb->surface);
        rb->surface = NULL;
    }
}

static void get_surface_decompressed_readback(IDirect3DDevice9 *device, IDirect3DSurface9 *compressed_surface,
        struct surface_readback *rb)
{
    return get_surface_readback(device, compressed_surface, D3DFMT_A8R8G8B8, rb);
}

static HRESULT create_file(const char *filename, const unsigned char *data, const unsigned int size)
{
    DWORD received;
    HANDLE hfile;

    hfile = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if(hfile == INVALID_HANDLE_VALUE) return HRESULT_FROM_WIN32(GetLastError());

    if(WriteFile(hfile, data, size, &received, NULL))
    {
        CloseHandle(hfile);
        return D3D_OK;
    }

    CloseHandle(hfile);
    return D3DERR_INVALIDCALL;
}

/* dds_pixel_format.flags */
#define DDS_PF_ALPHA 0x00000001
#define DDS_PF_ALPHA_ONLY 0x00000002
#define DDS_PF_FOURCC 0x00000004
#define DDS_PF_INDEXED 0x00000020
#define DDS_PF_RGB 0x00000040
#define DDS_PF_LUMINANCE 0x00020000
#define DDS_PF_BUMPLUMINANCE 0x00040000
#define DDS_PF_BUMPDUDV 0x00080000

struct dds_pixel_format
{
    DWORD size;
    DWORD flags;
    DWORD fourcc;
    DWORD bpp;
    DWORD rmask;
    DWORD gmask;
    DWORD bmask;
    DWORD amask;
};

struct dds_header
{
    DWORD size;
    DWORD flags;
    DWORD height;
    DWORD width;
    DWORD pitch_or_linear_size;
    DWORD depth;
    DWORD miplevels;
    DWORD reserved[11];
    struct dds_pixel_format pixel_format;
    DWORD caps;
    DWORD caps2;
    DWORD caps3;
    DWORD caps4;
    DWORD reserved2;
};

/* fills dds_header with reasonable default values */
static void fill_dds_header(struct dds_header *header)
{
    memset(header, 0, sizeof(*header));

    header->size = sizeof(*header);
    header->flags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    header->height = 4;
    header->width = 4;
    header->pixel_format.size = sizeof(header->pixel_format);
    /* X8R8G8B8 */
    header->pixel_format.flags = DDS_PF_RGB;
    header->pixel_format.fourcc = 0;
    header->pixel_format.bpp = 32;
    header->pixel_format.rmask = 0xff0000;
    header->pixel_format.gmask = 0x00ff00;
    header->pixel_format.bmask = 0x0000ff;
    header->pixel_format.amask = 0;
    header->caps = DDSCAPS_TEXTURE;
}

#define check_dds_pixel_format(flags, fourcc, bpp, rmask, gmask, bmask, amask, format) \
        check_dds_pixel_format_(__LINE__, flags, fourcc, bpp, rmask, gmask, bmask, amask, format)
static void check_dds_pixel_format_(unsigned int line,
                                    DWORD flags, DWORD fourcc, DWORD bpp,
                                    DWORD rmask, DWORD gmask, DWORD bmask, DWORD amask,
                                    D3DFORMAT expected_format)
{
    HRESULT hr;
    D3DXIMAGE_INFO info;
    struct
    {
        DWORD magic;
        struct dds_header header;
        PALETTEENTRY palette[256];
        BYTE data[256];
    } dds;

    dds.magic = MAKEFOURCC('D','D','S',' ');
    fill_dds_header(&dds.header);
    dds.header.pixel_format.flags = flags;
    dds.header.pixel_format.fourcc = fourcc;
    dds.header.pixel_format.bpp = bpp;
    dds.header.pixel_format.rmask = rmask;
    dds.header.pixel_format.gmask = gmask;
    dds.header.pixel_format.bmask = bmask;
    dds.header.pixel_format.amask = amask;
    memset(dds.data, 0, sizeof(dds.data));

    hr = D3DXGetImageInfoFromFileInMemory(&dds, sizeof(dds), &info);
    ok_(__FILE__, line)(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#lx for pixel format %#x, expected %#lx\n",
            hr, expected_format, D3D_OK);
    if (SUCCEEDED(hr))
    {
        ok_(__FILE__, line)(info.Format == expected_format, "D3DXGetImageInfoFromFileInMemory returned format %#x, expected %#x\n",
                info.Format, expected_format);
    }

    /* Test again with unused fields set. */
    if (flags & DDS_PF_FOURCC)
        rmask = gmask = bmask = amask = bpp = ~0u;
    else if ((flags & (DDS_PF_INDEXED | DDS_PF_ALPHA)) == (DDS_PF_INDEXED | DDS_PF_ALPHA))
        rmask = gmask = bmask = fourcc = ~0u;
    else if (flags & DDS_PF_INDEXED)
        rmask = gmask = bmask = amask = fourcc = ~0u;
    else if ((flags & (DDS_PF_RGB | DDS_PF_ALPHA)) == (DDS_PF_RGB | DDS_PF_ALPHA))
        fourcc = ~0u;
    else if (flags & DDS_PF_RGB)
        fourcc = amask = ~0u;
    else if ((flags & (DDS_PF_LUMINANCE | DDS_PF_ALPHA)) == (DDS_PF_LUMINANCE | DDS_PF_ALPHA))
        gmask = bmask = fourcc = ~0u;
    else if (flags & DDS_PF_LUMINANCE)
        gmask = bmask = amask = fourcc = ~0u;
    else if (flags & DDS_PF_ALPHA_ONLY)
        rmask = gmask = bmask = fourcc = ~0u;
    else if (flags & DDS_PF_BUMPDUDV)
        fourcc = ~0u;
    else if (flags & DDS_PF_BUMPLUMINANCE)
        fourcc = amask = ~0u;

    dds.header.pixel_format.fourcc = fourcc;
    dds.header.pixel_format.bpp = bpp;
    dds.header.pixel_format.rmask = rmask;
    dds.header.pixel_format.gmask = gmask;
    dds.header.pixel_format.bmask = bmask;
    dds.header.pixel_format.amask = amask;
    hr = D3DXGetImageInfoFromFileInMemory(&dds, sizeof(dds), &info);
    ok_(__FILE__, line)(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
        ok_(__FILE__, line)(info.Format == expected_format, "Unexpected format %#x.\n", info.Format);
}

static void test_dds_header_handling(void)
{
    int i;
    HRESULT hr;
    D3DXIMAGE_INFO info;
    struct
    {
        DWORD magic;
        struct dds_header header;
        BYTE data[4096 * 1024];
    } *dds;

    static const struct
    {
        struct dds_pixel_format pixel_format;
        DWORD flags;
        DWORD width;
        DWORD height;
        DWORD pitch;
        DWORD miplevels;
        DWORD pixel_data_size;
        struct
        {
            HRESULT hr;
            UINT miplevels;
            BOOL todo;
        }
        expected;
    } tests[] = {
        /* pitch is ignored */
        { { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }, 0, 4, 4, 0, 0,
          63 /* pixel data size */, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }, DDSD_PITCH, 4, 4, 0 /* pitch */, 0,
          64, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }, DDSD_PITCH, 4, 4, 1 /* pitch */, 0,
          64, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }, DDSD_PITCH, 4, 4, 2 /* pitch */, 0,
          64, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }, DDSD_PITCH, 4, 4, 3 /* pitch */, 0,
          64, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }, DDSD_PITCH, 4, 4, 4 /* pitch */, 0,
          64, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }, DDSD_PITCH, 4, 4, 16 /* pitch */, 0,
          64, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }, DDSD_PITCH, 4, 4, 1024 /* pitch */, 0,
          64, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 }, DDSD_PITCH, 4, 4, -1 /* pitch */, 0,
          64, { D3D_OK, 1 } },
        /* linear size is ignored */
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 }, 0, 4, 4, 0, 0,
          7 /* pixel data size */, { D3DXERR_INVALIDDATA, 1 } },
        /* 10. */
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 }, DDSD_LINEARSIZE, 4, 4, 0 /* linear size */, 0,
          8, { D3D_OK, 1 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 }, DDSD_LINEARSIZE, 4, 4, 1 /* linear size */, 0,
          8, { D3D_OK, 1 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 }, DDSD_LINEARSIZE, 4, 4, 2 /* linear size */, 0,
          8, { D3D_OK, 1 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 }, DDSD_LINEARSIZE, 4, 4, 9 /* linear size */, 0,
          8, { D3D_OK, 1 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 }, DDSD_LINEARSIZE, 4, 4, 16 /* linear size */, 0,
          8, { D3D_OK, 1 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 }, DDSD_LINEARSIZE, 4, 4, -1 /* linear size */, 0,
          8, { D3D_OK, 1 } },
        /* integer overflows */
        { { 32, DDS_PF_RGB, 0, 32, 0xff0000, 0x00ff00, 0x0000ff, 0 }, 0, 0x80000000, 0x80000000 /* 0x80000000 * 0x80000000 * 4 = 0 */, 0, 0,
          64, { D3D_OK, 1, .todo = TRUE } },
        { { 32, DDS_PF_RGB, 0, 32, 0xff0000, 0x00ff00, 0x0000ff, 0 }, 0, 0x8000100, 0x800100 /* 0x8000100 * 0x800100 * 4 = 262144 */, 0, 0,
          64, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_RGB, 0, 32, 0xff0000, 0x00ff00, 0x0000ff, 0 }, 0, 0x80000001, 0x80000001 /* 0x80000001 * 0x80000001 * 4 = 4 */, 0, 0,
          4, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB, 0, 32, 0xff0000, 0x00ff00, 0x0000ff, 0 }, 0, 0x80000001, 0x80000001 /* 0x80000001 * 0x80000001 * 4 = 4 */, 0, 0,
          3 /* pixel data size */, { D3DXERR_INVALIDDATA, 0 } },
        /* 20. File size is validated. */
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, 0, 256, 64, 0, 0, 49151, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, 0, 256, 64, 0, 0, 49152, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, DDSD_MIPMAPCOUNT, 256, 64, 0, 4, 65279, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, DDSD_MIPMAPCOUNT, 256, 64, 0, 4, 65280, { D3D_OK, 4 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, DDSD_MIPMAPCOUNT, 256, 64, 0, 9, 65540, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, DDSD_MIPMAPCOUNT, 256, 64, 0, 9, 65541, { D3D_OK, 9 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, 0, 256, 256, 0, 0, 196607, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, 0, 256, 256, 0, 0, 196608, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, 0, 256, 256, 0, 0, 196609, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, DDSD_MIPMAPCOUNT, 256, 256, 0, 1, 196607, { D3DXERR_INVALIDDATA, 0 } },
        /* 30. */
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, DDSD_MIPMAPCOUNT, 256, 256, 0, 1, 196608, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, DDSD_MIPMAPCOUNT, 256, 256, 0, 0, 196607, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, DDSD_MIPMAPCOUNT, 256, 256, 0, 0, 196608, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, DDSD_MIPMAPCOUNT, 256, 256, 0, 0, 400000, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, DDSD_MIPMAPCOUNT, 256, 256, 0, 9, 262142, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, DDSD_MIPMAPCOUNT, 256, 256, 0, 9, 262143, { D3D_OK, 9 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, DDSD_MIPMAPCOUNT, 256, 256, 0, 10, 262145, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, DDSD_MIPMAPCOUNT, 256, 256, 0, 10, 262146, { D3D_OK, 10 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, DDSD_MIPMAPCOUNT, 256, 256, 0, 20, 262175, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, DDSD_MIPMAPCOUNT, 256, 256, 0, 20, 262176, { D3D_OK, 20 } },
        /* 40. */
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 }, 0, 256, 256, 0, 0, 32767, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 }, 0, 256, 256, 0, 0, 32768, { D3D_OK, 1 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 }, DDSD_MIPMAPCOUNT, 256, 256, 0, 0, 32767, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 }, DDSD_MIPMAPCOUNT, 256, 256, 0, 0, 32768, { D3D_OK, 1 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 }, DDSD_MIPMAPCOUNT, 256, 256, 0, 9, 43703, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 }, DDSD_MIPMAPCOUNT, 256, 256, 0, 9, 43704, { D3D_OK, 9 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 }, DDSD_MIPMAPCOUNT, 256, 256, 0, 20, 43791, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 }, DDSD_MIPMAPCOUNT, 256, 256, 0, 20, 43792, { D3D_OK, 20 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT4, 0, 0, 0, 0, 0 }, 0, 256, 256, 0, 0, 65535, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT4, 0, 0, 0, 0, 0 }, 0, 256, 256, 0, 0, 65536, { D3D_OK, 1 } },
        /* 50. */
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT4, 0, 0, 0, 0, 0 }, DDSD_MIPMAPCOUNT, 256, 256, 0, 0, 65535, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT4, 0, 0, 0, 0, 0 }, DDSD_MIPMAPCOUNT, 256, 256, 0, 0, 65536, { D3D_OK, 1 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT4, 0, 0, 0, 0, 0 }, DDSD_MIPMAPCOUNT, 256, 256, 0, 9, 87407, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT4, 0, 0, 0, 0, 0 }, DDSD_MIPMAPCOUNT, 256, 256, 0, 9, 87408, { D3D_OK, 9 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT4, 0, 0, 0, 0, 0 }, DDSD_MIPMAPCOUNT, 256, 256, 0, 20, 87583, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT4, 0, 0, 0, 0, 0 }, DDSD_MIPMAPCOUNT, 256, 256, 0, 20, 87584, { D3D_OK, 20 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT4, 0, 0, 0, 0, 0 }, DDSD_MIPMAPCOUNT, 256, 64, 0, 4, 21759, { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_DXT4, 0, 0, 0, 0, 0 }, DDSD_MIPMAPCOUNT, 256, 64, 0, 4, 21760, { D3D_OK, 4 } },
        /* DDSD_MIPMAPCOUNT is ignored */
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, 0, 256, 256, 0, 0, 262146, { D3D_OK, 1 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, 0, 256, 256, 0, 2, 262146, { D3D_OK, 2 } },
        /* 60. */
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, 0, 256, 256, 0, 9, 262146, { D3D_OK, 9 } },
        { { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0x000000 }, 0, 256, 256, 0, 10, 262146, { D3D_OK, 10 } },
        /* Packed formats. */
        { { 32, DDS_PF_FOURCC, D3DFMT_R8G8_B8G8, 0, 0, 0, 0, 0 }, 0, 3, 3, 0, 0, (4 * 3 * 2), { D3D_OK, 1 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_G8R8_G8B8, 0, 0, 0, 0, 0 }, 0, 3, 3, 0, 0, (4 * 3 * 2), { D3D_OK, 1 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_UYVY, 0, 0, 0, 0, 0 }, 0, 3, 3, 0, 0, (4 * 3 * 2), { D3D_OK, 1 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_YUY2, 0, 0, 0, 0, 0 }, 0, 3, 3, 0, 0, (4 * 3 * 2), { D3D_OK, 1 } },
        /* Uneven height/width is supported, but pixel size must align. */
        { { 32, DDS_PF_FOURCC, D3DFMT_R8G8_B8G8, 0, 0, 0, 0, 0 }, 0, 3, 3, 0, 0, (3 * 3 * 2), { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_G8R8_G8B8, 0, 0, 0, 0, 0 }, 0, 3, 3, 0, 0, (3 * 3 * 2), { D3DXERR_INVALIDDATA, 0 } },
        /* 70. */
        { { 32, DDS_PF_FOURCC, D3DFMT_R8G8_B8G8, 0, 0, 0, 0, 0 }, 0, 4, 3, 4, 1, (4 * 3 * 2), { D3D_OK, 1 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_G8R8_G8B8, 0, 0, 0, 0, 0 }, 0, 4, 3, 4, 1, (4 * 3 * 2), { D3D_OK, 1 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_UYVY, 0, 0, 0, 0, 0 }, 0, 3, 3, 0, 0, (3 * 3 * 2), { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_YUY2, 0, 0, 0, 0, 0 }, 0, 3, 3, 0, 0, (3 * 3 * 2), { D3DXERR_INVALIDDATA, 0 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_UYVY, 0, 0, 0, 0, 0 }, 0, 4, 3, 4, 1, (4 * 3 * 2), { D3D_OK, 1 } },
        { { 32, DDS_PF_FOURCC, D3DFMT_YUY2, 0, 0, 0, 0, 0 }, 0, 4, 3, 4, 1, (4 * 3 * 2), { D3D_OK, 1 } },
    };
    static const struct
    {
        uint32_t flags;
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint32_t row_pitch;
        uint32_t mip_levels;
        uint32_t caps;
        uint32_t caps2;
        struct
        {
            HRESULT hr;
            uint32_t width;
            uint32_t height;
            uint32_t depth;
            uint32_t mip_levels;
            D3DRESOURCETYPE resource_type;
        }
        expected;
        uint32_t pixel_data_size;
        BOOL todo_hr;
        BOOL todo_info;
    } info_tests[] = {
        /* Depth value set to 4, but no caps bits are set. Depth is ignored. */
        { (DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT), 4, 4, 4, (4 * 4), 3, 0, 0,
          { D3D_OK, 4, 4, 1, 3, D3DRTYPE_TEXTURE, }, 292 },
        /* The volume texture caps2 field is ignored. */
        { (DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT), 4, 4, 4, (4 * 4), 3,
          (DDSCAPS_TEXTURE | DDSCAPS_COMPLEX), DDSCAPS2_VOLUME,
          { D3D_OK, 4, 4, 1, 3, D3DRTYPE_TEXTURE, }, 292 },
        /*
         * The DDSD_DEPTH flag is the only thing checked to determine if a DDS
         * file represents a volume texture.
         */
        { (DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_DEPTH), 4, 4, 4, (4 * 4), 3,
          0, 0,
          { D3D_OK, 4, 4, 4, 3, D3DRTYPE_VOLUMETEXTURE, }, 292 },
        /* Even if the depth field is set to 0, it's still a volume texture. */
        { (DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_DEPTH), 4, 4, 0, (4 * 4), 3,
          0, 0,
          { D3D_OK, 4, 4, 1, 3, D3DRTYPE_VOLUMETEXTURE, }, 292 },
        /* The DDSD_DEPTH flag overrides cubemap caps. */
        { (DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_DEPTH), 4, 4, 4, (4 * 4), 3,
          (DDSCAPS_TEXTURE | DDSCAPS_COMPLEX), (DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_ALLFACES),
          { D3D_OK, 4, 4, 4, 3, D3DRTYPE_VOLUMETEXTURE, }, (292 * 6) },
        /* Cubemap where width field does not equal height. */
        { (DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT), 4, 5, 1, (4 * 4), 1,
          (DDSCAPS_TEXTURE | DDSCAPS_COMPLEX), (DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_ALLFACES),
          { D3D_OK, 4, 5, 1, 1, D3DRTYPE_CUBETEXTURE, }, (80 * 6) },
        /* Partial cubemaps are not supported. */
        { (DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT), 4, 4, 1, (4 * 4), 1,
          (DDSCAPS_TEXTURE | DDSCAPS_COMPLEX), (DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX),
          { D3DXERR_INVALIDDATA, }, (64 * 6) },
    };

    dds = calloc(1, sizeof(*dds));
    if (!dds)
    {
        skip("Failed to allocate memory.\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        DWORD file_size = sizeof(dds->magic) + sizeof(dds->header) + tests[i].pixel_data_size;
        assert(file_size <= sizeof(*dds));

        winetest_push_context("Test %u", i);
        dds->magic = MAKEFOURCC('D','D','S',' ');
        fill_dds_header(&dds->header);
        dds->header.flags |= tests[i].flags;
        dds->header.width = tests[i].width;
        dds->header.height = tests[i].height;
        dds->header.pitch_or_linear_size = tests[i].pitch;
        dds->header.miplevels = tests[i].miplevels;
        dds->header.pixel_format = tests[i].pixel_format;

        hr = D3DXGetImageInfoFromFileInMemory(dds, file_size, &info);
        todo_wine_if(tests[i].expected.todo) ok(hr == tests[i].expected.hr, "Unexpected hr %#lx, expected %#lx.\n",
                hr, tests[i].expected.hr);
        if (SUCCEEDED(hr))
        {
            ok(info.MipLevels == tests[i].expected.miplevels, "Unexpected MipLevels %u, expected %u.\n",
                    info.MipLevels, tests[i].expected.miplevels);
        }
        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(info_tests); i++)
    {
        uint32_t file_size = sizeof(dds->magic) + sizeof(dds->header) + info_tests[i].pixel_data_size;
        assert(file_size <= sizeof(*dds));

        winetest_push_context("Test %u", i);
        dds->magic = MAKEFOURCC('D','D','S',' ');
        fill_dds_header(&dds->header);
        dds->header.flags = info_tests[i].flags;
        dds->header.width = info_tests[i].width;
        dds->header.height = info_tests[i].height;
        dds->header.depth = info_tests[i].depth;
        dds->header.pitch_or_linear_size = info_tests[i].row_pitch;
        dds->header.miplevels = info_tests[i].mip_levels;
        dds->header.caps = info_tests[i].caps;
        dds->header.caps2 = info_tests[i].caps2;

        memset(&info, 0, sizeof(info));
        hr = D3DXGetImageInfoFromFileInMemory(dds, file_size, &info);
        todo_wine_if(info_tests[i].todo_hr) ok(hr == info_tests[i].expected.hr, "Unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(info_tests[i].expected.hr))
        {
            check_image_info(&info, info_tests[i].expected.width, info_tests[i].expected.height,
                    info_tests[i].expected.depth, info_tests[i].expected.mip_levels, D3DFMT_X8R8G8B8,
                    info_tests[i].expected.resource_type, D3DXIFF_DDS, info_tests[i].todo_info);
        }
        winetest_pop_context();
    }

    free(dds);
}

#define COLORMAP_TYPE_NONE 0
#define COLORMAP_TYPE_ONE  1

#define IMAGETYPE_COLORMAPPED 1
#define IMAGETYPE_TRUECOLOR 2
#define IMAGETYPE_GRAYSCALE 3
#define IMAGETYPE_RLE 8

#define IMAGE_RIGHTTOLEFT 0x10
#define IMAGE_TOPTOBOTTOM 0x20

#include "pshpack1.h"
struct tga_header
{
    uint8_t  id_length;
    uint8_t  color_map_type;
    uint8_t  image_type;
    uint16_t color_map_firstentry;
    uint16_t color_map_length;
    uint8_t  color_map_entrysize;
    uint16_t xorigin;
    uint16_t yorigin;
    uint16_t width;
    uint16_t height;
    uint8_t  depth;
    uint8_t  image_descriptor;
};

struct tga_footer
{
    uint32_t extension_area_offset;
    uint32_t developer_directory_offset;
    uint8_t magic[18];
};
#include "poppack.h"

static const struct tga_footer default_tga_footer = {
    0, 0,
    { 'T', 'R', 'U', 'E', 'V', 'I', 'S', 'I', 'O', 'N', '-', 'X', 'F', 'I', 'L', 'E', '.', 0 }
};

#define check_tga_image_info(tga, tga_size, expected_width, expected_height, expected_format, expected_hr) \
    check_tga_image_info_(__LINE__, tga, tga_size, expected_width, expected_height, expected_format, expected_hr)
static void check_tga_image_info_(uint32_t line, const void *tga, uint32_t tga_size, uint32_t expected_width,
        uint32_t expected_height, D3DFORMAT expected_format, HRESULT expected_hr)
{
    D3DXIMAGE_INFO info = { 0 };
    HRESULT hr;

    hr = D3DXGetImageInfoFromFileInMemory(tga, tga_size, &info);
    ok_(__FILE__, line)(hr == expected_hr, "Unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(expected_hr) && SUCCEEDED(hr))
    {
        check_image_info_(__FILE__, line, &info, expected_width, expected_height, 1, 1, expected_format,
                D3DRTYPE_TEXTURE, D3DXIFF_TGA, FALSE);
    }
}

static void test_tga_header_handling(void)
{
    static const struct
    {
        struct tga_header header;
        struct
        {
            HRESULT hr;
            uint32_t width;
            uint32_t height;
            D3DFORMAT format;
        } expected;
        uint32_t extra_header_size;
    } info_tests[] =
    {
        /* 15 bpp true color. */
        { { 0, COLORMAP_TYPE_NONE, IMAGETYPE_TRUECOLOR, 0, 0, 0, 0, 0, 4, 4, 15, 0 },
          { D3D_OK, 4, 4, D3DFMT_X1R5G5B5 }
        },
        /* 16 bpp true color. */
        { { 0, COLORMAP_TYPE_NONE, IMAGETYPE_TRUECOLOR, 0, 0, 0, 0, 0, 4, 4, 16, 0 },
          { D3D_OK, 4, 4, D3DFMT_A1R5G5B5 }
        },
        /* 24 bpp true color. */
        { { 0, COLORMAP_TYPE_NONE, IMAGETYPE_TRUECOLOR, 0, 0, 0, 0, 0, 4, 4, 24, 0 },
          { D3D_OK, 4, 4, D3DFMT_R8G8B8 }
        },
        /* 32 bpp true color. */
        { { 0, COLORMAP_TYPE_NONE, IMAGETYPE_TRUECOLOR, 0, 0, 0, 0, 0, 4, 4, 32, 0 },
          { D3D_OK, 4, 4, D3DFMT_A8R8G8B8 }
        },
        /* 8 bit paletted, 15 bpp palette. */
        { { 0, COLORMAP_TYPE_ONE, IMAGETYPE_COLORMAPPED, 0, 256, 15, 0, 0, 4, 4, 8, 0 },
          { D3D_OK, 4, 4, D3DFMT_P8 }, (256 * 2)
        },
        /* 8 bit paletted, 16 bpp palette. */
        { { 0, COLORMAP_TYPE_ONE, IMAGETYPE_COLORMAPPED, 0, 256, 16, 0, 0, 4, 4, 8, 0 },
          { D3D_OK, 4, 4, D3DFMT_P8 }, (256 * 2)
        },
        /* 8 bit paletted, 24 bpp palette. */
        { { 0, COLORMAP_TYPE_ONE, IMAGETYPE_COLORMAPPED, 0, 256, 24, 0, 0, 4, 4, 8, 0 },
          { D3D_OK, 4, 4, D3DFMT_P8 }, (256 * 3)
        },
        /* 8 bit paletted, 32 bpp palette. */
        { { 0, COLORMAP_TYPE_ONE, IMAGETYPE_COLORMAPPED, 0, 256, 32, 0, 0, 4, 4, 8, 0 },
          { D3D_OK, 4, 4, D3DFMT_P8 }, (256 * 4)
        },
        /* Grayscale, 8bpp. */
        { { 0, COLORMAP_TYPE_NONE, IMAGETYPE_GRAYSCALE, 0, 0, 0, 0, 0, 4, 4, 8, 0 },
          { D3D_OK, 4, 4, D3DFMT_L8 }
        },
        /* No 16-bit grayscale. */
        { { 0, COLORMAP_TYPE_NONE, IMAGETYPE_GRAYSCALE, 0, 0, 0, 0, 0, 4, 4, 16, 0 },
          { D3DXERR_INVALIDDATA }
        },
    };
    struct
    {
        struct tga_header header;
        uint8_t data[4096 * 1024];
    } *tga;
    struct tga_footer tmp_footer;
    uint32_t i;

    tga = calloc(1, sizeof(*tga));
    if (!tga)
    {
        skip("Failed to allocate memory.\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(info_tests); i++)
    {
        uint32_t file_size = sizeof(tga->header) + info_tests[i].extra_header_size;
        assert(file_size <= sizeof(*tga));

        winetest_push_context("Test %u", i);

        tga->header = info_tests[i].header;
        check_tga_image_info(tga, file_size, info_tests[i].expected.width, info_tests[i].expected.height,
                info_tests[i].expected.format, info_tests[i].expected.hr);

        /* X/Y origin fields are ignored. */
        tga->header.xorigin = tga->header.width + 1;
        tga->header.yorigin = tga->header.height + 1;
        check_tga_image_info(tga, file_size, info_tests[i].expected.width, info_tests[i].expected.height,
                info_tests[i].expected.format, info_tests[i].expected.hr);

        /* Image descriptor field is ignored. */
        tga->header.image_descriptor = 0xcf;
        check_tga_image_info(tga, file_size, info_tests[i].expected.width, info_tests[i].expected.height,
                info_tests[i].expected.format, info_tests[i].expected.hr);

        if (FAILED(info_tests[i].expected.hr))
            goto next;

        /*
         * Footer offsets do not seem to be validated. Possible that footer
         * isn't even checked for.
         */
        tmp_footer = default_tga_footer;
        tmp_footer.extension_area_offset = 65536;
        memcpy(&tga->data[info_tests[i].extra_header_size], &tmp_footer, sizeof(tmp_footer));
        check_tga_image_info(tga, file_size + sizeof(tmp_footer), info_tests[i].expected.width, info_tests[i].expected.height,
                info_tests[i].expected.format, info_tests[i].expected.hr);

        /* Check RLE type. */
        tga->header.image_type |= IMAGETYPE_RLE;
        check_tga_image_info(tga, file_size, info_tests[i].expected.width, info_tests[i].expected.height,
                info_tests[i].expected.format, info_tests[i].expected.hr);
        tga->header.image_type &= ~IMAGETYPE_RLE;

        if (tga->header.image_type == IMAGETYPE_COLORMAPPED)
            goto next;

        /*
         * Even if the image isn't color mapped, the color map fields are used
         * to validate header size.
         */
        tga->header.color_map_length = 1;
        check_tga_image_info(tga, file_size, info_tests[i].expected.width, info_tests[i].expected.height,
                info_tests[i].expected.format, info_tests[i].expected.hr);

        tga->header.color_map_entrysize = 8;
        check_tga_image_info(tga, file_size, 0, 0, D3DFMT_UNKNOWN, D3DXERR_INVALIDDATA);

        /* Add a byte to file size to account for color map. */
        check_tga_image_info(tga, file_size + 1, info_tests[i].expected.width, info_tests[i].expected.height,
                info_tests[i].expected.format, info_tests[i].expected.hr);

        /* ID length field is also considered. */
        tga->header.id_length = 1;
        check_tga_image_info(tga, file_size + 1, 0, 0, D3DFMT_UNKNOWN, D3DXERR_INVALIDDATA);

        /* Add another byte to file size to account for id length. */
        check_tga_image_info(tga, file_size + 2, info_tests[i].expected.width, info_tests[i].expected.height,
                info_tests[i].expected.format, info_tests[i].expected.hr);

        /*
         * If the color map type field is set but the color map fields
         * result in a color map size of 0, header is considered invalid.
         * Also, the entrysize value is now validated, e.g it must be 15,
         * 16, 24, or 32.
         */
        tga->header.id_length = tga->header.color_map_entrysize = tga->header.color_map_length = 0;
        tga->header.color_map_type = COLORMAP_TYPE_ONE;
        check_tga_image_info(tga, file_size + 2, 0, 0, D3DFMT_UNKNOWN, D3DXERR_INVALIDDATA);

        /* 8 isn't a valid entry size. */
        tga->header.color_map_entrysize = 8;
        tga->header.color_map_length = 1;
        check_tga_image_info(tga, file_size + 1, 0, 0, D3DFMT_UNKNOWN, D3DXERR_INVALIDDATA);

        /* 16 is a valid entry size. */
        tga->header.color_map_entrysize = 16;
        check_tga_image_info(tga, file_size + 2, info_tests[i].expected.width, info_tests[i].expected.height,
                info_tests[i].expected.format, info_tests[i].expected.hr);

        /* First entry doesn't factor into validation. */
        tga->header.color_map_firstentry = 512;
        check_tga_image_info(tga, file_size + 2, info_tests[i].expected.width, info_tests[i].expected.height,
                info_tests[i].expected.format, info_tests[i].expected.hr);
next:
        winetest_pop_context();
    }

    free(tga);
}

static void test_D3DXGetImageInfo(void)
{
    HRESULT hr;
    D3DXIMAGE_INFO info;
    BOOL testdummy_ok, testbitmap_ok;

    hr = create_file("testdummy.bmp", noimage, sizeof(noimage));  /* invalid image */
    testdummy_ok = SUCCEEDED(hr);

    hr = create_file("testbitmap.bmp", bmp_1bpp, sizeof(bmp_1bpp));  /* valid image */
    testbitmap_ok = SUCCEEDED(hr);

    /* D3DXGetImageInfoFromFile */
    if(testbitmap_ok) {
        hr = D3DXGetImageInfoFromFileA("testbitmap.bmp", &info);
        ok(hr == D3D_OK, "D3DXGetImageInfoFromFile returned %#lx, expected %#lx\n", hr, D3D_OK);

        hr = D3DXGetImageInfoFromFileA("testbitmap.bmp", NULL); /* valid image, second parameter is NULL */
        ok(hr == D3D_OK, "D3DXGetImageInfoFromFile returned %#lx, expected %#lx\n", hr, D3D_OK);
    } else skip("Couldn't create \"testbitmap.bmp\"\n");

    if(testdummy_ok) {
        hr = D3DXGetImageInfoFromFileA("testdummy.bmp", NULL); /* invalid image, second parameter is NULL */
        ok(hr == D3D_OK, "D3DXGetImageInfoFromFile returned %#lx, expected %#lx\n", hr, D3D_OK);

        hr = D3DXGetImageInfoFromFileA("testdummy.bmp", &info);
        ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFile returned %#lx, expected %#x\n", hr, D3DXERR_INVALIDDATA);
    } else skip("Couldn't create \"testdummy.bmp\"\n");

    hr = D3DXGetImageInfoFromFileA("filedoesnotexist.bmp", &info);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFile returned %#lx, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXGetImageInfoFromFileA("filedoesnotexist.bmp", NULL);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFile returned %#lx, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXGetImageInfoFromFileA("", &info);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFile returned %#lx, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXGetImageInfoFromFileA(NULL, &info);
    ok(hr == D3DERR_INVALIDCALL, "D3DXGetImageInfoFromFile returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXGetImageInfoFromFileA(NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXGetImageInfoFromFile returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);


    /* D3DXGetImageInfoFromResource */
    hr = D3DXGetImageInfoFromResourceA(NULL, MAKEINTRESOURCEA(IDB_BITMAP_1x1), &info); /* RT_BITMAP */
    ok(hr == D3D_OK, "D3DXGetImageInfoFromResource returned %#lx, expected %#lx\n", hr, D3D_OK);

    hr = D3DXGetImageInfoFromResourceA(NULL, MAKEINTRESOURCEA(IDB_BITMAP_1x1), NULL);
    ok(hr == D3D_OK, "D3DXGetImageInfoFromResource returned %#lx, expected %#lx\n", hr, D3D_OK);

    hr = D3DXGetImageInfoFromResourceA(NULL, MAKEINTRESOURCEA(IDD_BITMAPDATA_1x1), &info); /* RT_RCDATA */
    ok(hr == D3D_OK, "D3DXGetImageInfoFromResource returned %#lx, expected %#lx\n", hr, D3D_OK);

    hr = D3DXGetImageInfoFromResourceA(NULL, MAKEINTRESOURCEA(IDS_STRING), &info);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromResource returned %#lx, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXGetImageInfoFromResourceA(NULL, MAKEINTRESOURCEA(IDS_STRING), NULL);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromResource returned %#lx, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXGetImageInfoFromResourceA(NULL, "resourcedoesnotexist", &info);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromResource returned %#lx, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXGetImageInfoFromResourceA(NULL, "resourcedoesnotexist", NULL);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromResource returned %#lx, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXGetImageInfoFromResourceA(NULL, NULL, NULL);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromResource returned %#lx, expected %#x\n", hr, D3DXERR_INVALIDDATA);


    /* D3DXGetImageInfoFromFileInMemory */
    hr = D3DXGetImageInfoFromFileInMemory(bmp_1bpp, sizeof(bmp_1bpp), &info);
    ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);

    hr = D3DXGetImageInfoFromFileInMemory(bmp_1bpp, sizeof(bmp_1bpp)+5, &info); /* too large size */
    ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);

    hr = D3DXGetImageInfoFromFileInMemory(bmp_1bpp, sizeof(bmp_1bpp), NULL);
    ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);

    hr = D3DXGetImageInfoFromFileInMemory(noimage, sizeof(noimage), NULL);
    ok(hr == D3D_OK, "D3DXGetImageInfoFromResource returned %#lx, expected %#lx\n", hr, D3D_OK);

    hr = D3DXGetImageInfoFromFileInMemory(noimage, sizeof(noimage), &info);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    todo_wine {
        hr = D3DXGetImageInfoFromFileInMemory(bmp_1bpp, sizeof(bmp_1bpp)-1, &info);
        ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#x\n", hr, D3DXERR_INVALIDDATA);
    }

    hr = D3DXGetImageInfoFromFileInMemory(bmp_1bpp+1, sizeof(bmp_1bpp)-1, &info);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXGetImageInfoFromFileInMemory(bmp_1bpp, 0, &info);
    ok(hr == D3DERR_INVALIDCALL, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXGetImageInfoFromFileInMemory(bmp_1bpp, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXGetImageInfoFromFileInMemory(noimage, 0, &info);
    ok(hr == D3DERR_INVALIDCALL, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXGetImageInfoFromFileInMemory(noimage, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXGetImageInfoFromFileInMemory(NULL, 0, &info);
    ok(hr == D3DERR_INVALIDCALL, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXGetImageInfoFromFileInMemory(NULL, 4, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXGetImageInfoFromFileInMemory(NULL, 4, &info);
    ok(hr == D3DERR_INVALIDCALL, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXGetImageInfoFromFileInMemory(NULL, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    /* test BMP support */
    hr = D3DXGetImageInfoFromFileInMemory(bmp_1bpp, sizeof(bmp_1bpp), &info);
    ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(info.Depth == 1, "Got depth %u, expected 1\n", info.Depth);
    ok(info.Format == D3DFMT_P8, "Got format %u, expected %u\n", info.Format, D3DFMT_P8);
    hr = D3DXGetImageInfoFromFileInMemory(bmp_2bpp, sizeof(bmp_2bpp), &info);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#x\n", hr, D3DXERR_INVALIDDATA);
    hr = D3DXGetImageInfoFromFileInMemory(bmp_4bpp, sizeof(bmp_4bpp), &info);
    ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(info.Depth == 1, "Got depth %u, expected 1\n", info.Depth);
    ok(info.Format == D3DFMT_P8, "Got format %u, expected %u\n", info.Format, D3DFMT_P8);
    hr = D3DXGetImageInfoFromFileInMemory(bmp_8bpp, sizeof(bmp_8bpp), &info);
    ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(info.Depth == 1, "Got depth %u, expected 1\n", info.Depth);
    ok(info.Format == D3DFMT_P8, "Got format %u, expected %u\n", info.Format, D3DFMT_P8);

    hr = D3DXGetImageInfoFromFileInMemory(bmp_32bpp_xrgb, sizeof(bmp_32bpp_xrgb), &info);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    ok(info.Format == D3DFMT_X8R8G8B8, "Got unexpected format %u.\n", info.Format);
    hr = D3DXGetImageInfoFromFileInMemory(bmp_32bpp_argb, sizeof(bmp_32bpp_argb), &info);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    ok(info.Format == D3DFMT_A8R8G8B8, "Got unexpected format %u.\n", info.Format);

    /* Test JPG support. */
    hr = D3DXGetImageInfoFromFileInMemory(jpg_rgb_2_2, sizeof(jpg_rgb_2_2), &info);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
    check_image_info(&info, 2, 2, 1, 1, D3DFMT_X8R8G8B8, D3DRTYPE_TEXTURE, D3DXIFF_JPG, FALSE);

    hr = D3DXGetImageInfoFromFileInMemory(jpg_grayscale_2_2, sizeof(jpg_grayscale_2_2), &info);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
    check_image_info(&info, 2, 2, 1, 1, D3DFMT_L8, D3DRTYPE_TEXTURE, D3DXIFF_JPG, FALSE);

    /* Test PNG support. */
    hr = D3DXGetImageInfoFromFileInMemory(png_2_2_48bpp_rgb, sizeof(png_2_2_48bpp_rgb), &info);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
    check_image_info(&info, 2, 2, 1, 1, D3DFMT_A16B16G16R16, D3DRTYPE_TEXTURE, D3DXIFF_PNG, FALSE);

    hr = D3DXGetImageInfoFromFileInMemory(png_2_2_64bpp_rgba, sizeof(png_2_2_64bpp_rgba), &info);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
    check_image_info(&info, 2, 2, 1, 1, D3DFMT_A16B16G16R16, D3DRTYPE_TEXTURE, D3DXIFF_PNG, FALSE);

    hr = D3DXGetImageInfoFromFileInMemory(png_2_2_24bpp_bgr, sizeof(png_2_2_24bpp_bgr), &info);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
    check_image_info(&info, 2, 2, 1, 1, D3DFMT_X8R8G8B8, D3DRTYPE_TEXTURE, D3DXIFF_PNG, FALSE);

    /* Grayscale PNG */
    hr = D3DXGetImageInfoFromFileInMemory(png_grayscale, sizeof(png_grayscale), &info);
    ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(info.Depth == 1, "Got depth %u, expected 1\n", info.Depth);
    ok(info.Format == D3DFMT_L8, "Got format %u, expected %u\n", info.Format, D3DFMT_L8);

    /* test DDS support */
    hr = D3DXGetImageInfoFromFileInMemory(dds_24bit, sizeof(dds_24bit), &info);
    ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);
    if (hr == D3D_OK) {
        ok(info.Width == 2, "Got width %u, expected 2\n", info.Width);
        ok(info.Height == 2, "Got height %u, expected 2\n", info.Height);
        ok(info.Depth == 1, "Got depth %u, expected 1\n", info.Depth);
        ok(info.MipLevels == 2, "Got miplevels %u, expected 2\n", info.MipLevels);
        ok(info.Format == D3DFMT_R8G8B8, "Got format %#x, expected %#x\n", info.Format, D3DFMT_R8G8B8);
        ok(info.ResourceType == D3DRTYPE_TEXTURE, "Got resource type %#x, expected %#x\n", info.ResourceType, D3DRTYPE_TEXTURE);
        ok(info.ImageFileFormat == D3DXIFF_DDS, "Got image file format %#x, expected %#x\n", info.ImageFileFormat, D3DXIFF_DDS);
    } else skip("Couldn't get image info from 24-bit DDS file in memory\n");

    hr = D3DXGetImageInfoFromFileInMemory(dds_16bit, sizeof(dds_16bit), &info);
    ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);
    if (hr == D3D_OK) {
        ok(info.Width == 2, "Got width %u, expected 2\n", info.Width);
        ok(info.Height == 2, "Got height %u, expected 2\n", info.Height);
        ok(info.Depth == 1, "Got depth %u, expected 1\n", info.Depth);
        ok(info.MipLevels == 1, "Got miplevels %u, expected 1\n", info.MipLevels);
        ok(info.Format == D3DFMT_X1R5G5B5, "Got format %#x, expected %#x\n", info.Format, D3DFMT_X1R5G5B5);
        ok(info.ResourceType == D3DRTYPE_TEXTURE, "Got resource type %#x, expected %#x\n", info.ResourceType, D3DRTYPE_TEXTURE);
        ok(info.ImageFileFormat == D3DXIFF_DDS, "Got image file format %#x, expected %#x\n", info.ImageFileFormat, D3DXIFF_DDS);
    } else skip("Couldn't get image info from 16-bit DDS file in memory\n");

    memset(&info, 0, sizeof(info));
    hr = D3DXGetImageInfoFromFileInMemory(dds_8bit, sizeof(dds_8bit), &info);
    ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#lx\n", hr);
    ok(info.Width == 16, "Got width %u.\n", info.Width);
    ok(info.Height == 4, "Got height %u.\n", info.Height);
    ok(info.Depth == 1, "Got depth %u.\n", info.Depth);
    ok(info.MipLevels == 1, "Got miplevels %u.\n", info.MipLevels);
    ok(info.Format == D3DFMT_P8, "Got format %#x.\n", info.Format);
    ok(info.ResourceType == D3DRTYPE_TEXTURE, "Got resource type %#x.\n", info.ResourceType);
    ok(info.ImageFileFormat == D3DXIFF_DDS, "Got image file format %#x.\n", info.ImageFileFormat);

    hr = D3DXGetImageInfoFromFileInMemory(dds_cube_map, sizeof(dds_cube_map), &info);
    ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);
    if (hr == D3D_OK) {
        ok(info.Width == 4, "Got width %u, expected 4\n", info.Width);
        ok(info.Height == 4, "Got height %u, expected 4\n", info.Height);
        ok(info.Depth == 1, "Got depth %u, expected 1\n", info.Depth);
        ok(info.MipLevels == 1, "Got miplevels %u, expected 1\n", info.MipLevels);
        ok(info.Format == D3DFMT_DXT5, "Got format %#x, expected %#x\n", info.Format, D3DFMT_DXT5);
        ok(info.ResourceType == D3DRTYPE_CUBETEXTURE, "Got resource type %#x, expected %#x\n", info.ResourceType, D3DRTYPE_CUBETEXTURE);
        ok(info.ImageFileFormat == D3DXIFF_DDS, "Got image file format %#x, expected %#x\n", info.ImageFileFormat, D3DXIFF_DDS);
    } else skip("Couldn't get image info from cube map in memory\n");

    hr = D3DXGetImageInfoFromFileInMemory(dds_volume_map, sizeof(dds_volume_map), &info);
    ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);
    if (hr == D3D_OK) {
        ok(info.Width == 4, "Got width %u, expected 4\n", info.Width);
        ok(info.Height == 4, "Got height %u, expected 4\n", info.Height);
        ok(info.Depth == 2, "Got depth %u, expected 2\n", info.Depth);
        ok(info.MipLevels == 3, "Got miplevels %u, expected 3\n", info.MipLevels);
        ok(info.Format == D3DFMT_DXT3, "Got format %#x, expected %#x\n", info.Format, D3DFMT_DXT3);
        ok(info.ResourceType == D3DRTYPE_VOLUMETEXTURE, "Got resource type %#x, expected %#x\n", info.ResourceType, D3DRTYPE_VOLUMETEXTURE);
        ok(info.ImageFileFormat == D3DXIFF_DDS, "Got image file format %#x, expected %#x\n", info.ImageFileFormat, D3DXIFF_DDS);
    } else skip("Couldn't get image info from volume map in memory\n");

    check_dds_pixel_format(DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0, D3DFMT_DXT1);
    check_dds_pixel_format(DDS_PF_FOURCC, D3DFMT_DXT2, 0, 0, 0, 0, 0, D3DFMT_DXT2);
    check_dds_pixel_format(DDS_PF_FOURCC, D3DFMT_DXT3, 0, 0, 0, 0, 0, D3DFMT_DXT3);
    check_dds_pixel_format(DDS_PF_FOURCC, D3DFMT_DXT4, 0, 0, 0, 0, 0, D3DFMT_DXT4);
    check_dds_pixel_format(DDS_PF_FOURCC, D3DFMT_DXT5, 0, 0, 0, 0, 0, D3DFMT_DXT5);
    check_dds_pixel_format(DDS_PF_FOURCC, D3DFMT_R8G8_B8G8, 0, 0, 0, 0, 0, D3DFMT_R8G8_B8G8);
    check_dds_pixel_format(DDS_PF_FOURCC, D3DFMT_G8R8_G8B8, 0, 0, 0, 0, 0, D3DFMT_G8R8_G8B8);
    check_dds_pixel_format(DDS_PF_FOURCC, D3DFMT_UYVY, 0, 0, 0, 0, 0, D3DFMT_UYVY);
    check_dds_pixel_format(DDS_PF_FOURCC, D3DFMT_YUY2, 0, 0, 0, 0, 0, D3DFMT_YUY2);
    /*
     * D3DFMTs that aren't fourCC values, but are supported in the fourCC
     * field.
     */
    check_dds_pixel_format(DDS_PF_FOURCC, D3DFMT_A16B16G16R16, 0, 0, 0, 0, 0, D3DFMT_A16B16G16R16);
    check_dds_pixel_format(DDS_PF_FOURCC, D3DFMT_Q16W16V16U16, 0, 0, 0, 0, 0, D3DFMT_Q16W16V16U16);
    check_dds_pixel_format(DDS_PF_FOURCC, D3DFMT_R16F, 0, 0, 0, 0, 0, D3DFMT_R16F);
    check_dds_pixel_format(DDS_PF_FOURCC, D3DFMT_G16R16F, 0, 0, 0, 0, 0, D3DFMT_G16R16F);
    check_dds_pixel_format(DDS_PF_FOURCC, D3DFMT_A16B16G16R16F, 0, 0, 0, 0, 0, D3DFMT_A16B16G16R16F);
    check_dds_pixel_format(DDS_PF_FOURCC, D3DFMT_R32F, 0, 0, 0, 0, 0, D3DFMT_R32F);
    check_dds_pixel_format(DDS_PF_FOURCC, D3DFMT_G32R32F, 0, 0, 0, 0, 0, D3DFMT_G32R32F);
    check_dds_pixel_format(DDS_PF_FOURCC, D3DFMT_A32B32G32R32F, 0, 0, 0, 0, 0, D3DFMT_A32B32G32R32F);
    check_dds_pixel_format(DDS_PF_RGB, 0, 16, 0xf800, 0x07e0, 0x001f, 0, D3DFMT_R5G6B5);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 16, 0x7c00, 0x03e0, 0x001f, 0x8000, D3DFMT_A1R5G5B5);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 16, 0x0f00, 0x00f0, 0x000f, 0xf000, D3DFMT_A4R4G4B4);
    check_dds_pixel_format(DDS_PF_RGB, 0, 8, 0xe0, 0x1c, 0x03, 0, D3DFMT_R3G3B2);
    check_dds_pixel_format(DDS_PF_ALPHA_ONLY, 0, 8, 0, 0, 0, 0xff, D3DFMT_A8);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 16, 0x00e0, 0x001c, 0x0003, 0xff00, D3DFMT_A8R3G3B2);
    check_dds_pixel_format(DDS_PF_RGB, 0, 16, 0xf00, 0x0f0, 0x00f, 0, D3DFMT_X4R4G4B4);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000, D3DFMT_A2B10G10R10);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x000003ff, 0x000ffc00, 0x3ff00000, 0xc0000000, D3DFMT_A2R10G10B10);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000, D3DFMT_A8R8G8B8);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000, D3DFMT_A8B8G8R8);
    check_dds_pixel_format(DDS_PF_RGB, 0, 32, 0xff0000, 0x00ff00, 0x0000ff, 0, D3DFMT_X8R8G8B8);
    check_dds_pixel_format(DDS_PF_RGB, 0, 32, 0x0000ff, 0x00ff00, 0xff0000, 0, D3DFMT_X8B8G8R8);
    check_dds_pixel_format(DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0, D3DFMT_R8G8B8);
    check_dds_pixel_format(DDS_PF_RGB, 0, 32, 0x0000ffff, 0xffff0000, 0, 0, D3DFMT_G16R16);
    check_dds_pixel_format(DDS_PF_LUMINANCE, 0, 8, 0xff, 0, 0, 0, D3DFMT_L8);
    check_dds_pixel_format(DDS_PF_LUMINANCE, 0, 16, 0xffff, 0, 0, 0, D3DFMT_L16);
    check_dds_pixel_format(DDS_PF_LUMINANCE | DDS_PF_ALPHA, 0, 16, 0x00ff, 0, 0, 0xff00, D3DFMT_A8L8);
    /* 8bpp works too for D3DFMT_A8L8. */
    check_dds_pixel_format(DDS_PF_LUMINANCE | DDS_PF_ALPHA, 0, 8, 0x00ff, 0, 0, 0xff00, D3DFMT_A8L8);
    check_dds_pixel_format(DDS_PF_LUMINANCE | DDS_PF_ALPHA, 0, 8, 0x0f, 0, 0, 0xf0, D3DFMT_A4L4);
    check_dds_pixel_format(DDS_PF_BUMPDUDV, 0, 16, 0x00ff, 0xff00, 0, 0, D3DFMT_V8U8);
    check_dds_pixel_format(DDS_PF_BUMPDUDV, 0, 32, 0x0000ffff, 0xffff0000, 0, 0, D3DFMT_V16U16);
    check_dds_pixel_format(DDS_PF_BUMPDUDV, 0, 32, 0xff, 0xff00, 0x00ff0000, 0xff000000, D3DFMT_Q8W8V8U8);
    check_dds_pixel_format(DDS_PF_BUMPDUDV | DDS_PF_ALPHA, 0, 32, 0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000, D3DFMT_A2W10V10U10);
    check_dds_pixel_format(DDS_PF_BUMPLUMINANCE, 0, 32, 0x0000ff, 0x00ff00, 0xff0000, 0, D3DFMT_X8L8V8U8);
    todo_wine check_dds_pixel_format(DDS_PF_BUMPLUMINANCE, 0, 16, 0x001f, 0x03e0, 0xfc00, 0, D3DFMT_L6V5U5);
    check_dds_pixel_format(DDS_PF_INDEXED, 0, 8, 0, 0, 0, 0, D3DFMT_P8);
    check_dds_pixel_format(DDS_PF_INDEXED | DDS_PF_ALPHA, 0, 16, 0, 0, 0, 0xff00, D3DFMT_A8P8);

    test_dds_header_handling();
    test_tga_header_handling();

    hr = D3DXGetImageInfoFromFileInMemory(dds_16bit, sizeof(dds_16bit) - 1, &info);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXGetImageInfoFromFileInMemory(dds_24bit, sizeof(dds_24bit) - 1, &info);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXGetImageInfoFromFileInMemory(dds_cube_map, sizeof(dds_cube_map) - 1, &info);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXGetImageInfoFromFileInMemory(dds_volume_map, sizeof(dds_volume_map) - 1, &info);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    /* Size includes the size of the color palette. */
    hr = D3DXGetImageInfoFromFileInMemory(dds_8bit, (sizeof(dds_8bit) - (sizeof(PALETTEENTRY) * 256)), &info);
    ok(hr == D3DXERR_INVALIDDATA, "Unexpected hr %#lx.\n", hr);

    /* cleanup */
    if(testdummy_ok) DeleteFileA("testdummy.bmp");
    if(testbitmap_ok) DeleteFileA("testbitmap.bmp");
}

#define check_pixel_2bpp(lockrect, x, y, color) _check_pixel_2bpp(__LINE__, lockrect, x, y, color)
static inline void _check_pixel_2bpp(unsigned int line, const D3DLOCKED_RECT *lockrect, int x, int y, WORD expected_color)
{
    WORD color = ((WORD*)lockrect->pBits)[x + y * lockrect->Pitch / 2];
    ok_(__FILE__, line)(color == expected_color, "Got color 0x%04x, expected 0x%04x\n", color, expected_color);
}

#define check_pixel_4bpp(lockrect, x, y, color) _check_pixel_4bpp(__LINE__, lockrect, x, y, color)
static inline void _check_pixel_4bpp(unsigned int line, const D3DLOCKED_RECT *lockrect, int x, int y, DWORD expected_color)
{
   DWORD color = ((DWORD*)lockrect->pBits)[x + y * lockrect->Pitch / 4];
   ok_(__FILE__, line)(color == expected_color, "Got color 0x%08lx, expected 0x%08lx\n", color, expected_color);
}

#define check_pixel_8bpp(lockrect, x, y, color) _check_pixel_8bpp(__LINE__, lockrect, x, y, color)
static inline void _check_pixel_8bpp(unsigned int line, const D3DLOCKED_RECT *lockrect, int x, int y, uint64_t expected_color)
{
   uint64_t color = ((uint64_t *)lockrect->pBits)[x + y * (lockrect->Pitch / 8)];
   ok_(__FILE__, line)(color == expected_color, "Got color %#I64x, expected %#I64x.\n", color, expected_color);
}

#define check_pixel_float4(lockrect, x, y, fx, fy, fz, fw, ulps, todo) \
    _check_pixel_float4(__LINE__, lockrect, x, y, fx, fy, fz, fw, ulps, todo)
static inline void _check_pixel_float4(uint32_t line, const D3DLOCKED_RECT *lockrect, uint32_t x, uint32_t y,
        float fx, float fy, float fz, float fw, uint32_t ulps, BOOL todo)
{
    float *ptr = (float *)(((BYTE *)lockrect->pBits) + (y * lockrect->Pitch) + (x * sizeof(float) * 4));

    todo_wine_if(todo) ok_(__FILE__, line)(compare_float(fx, ptr[0], ulps)
        && compare_float(fy, ptr[1], ulps)
        && compare_float(fz, ptr[2], ulps)
        && compare_float(fw, ptr[3], ulps),
        "Expected (%.8e, %.8e, %.8e, %.8e), got (%.8e, %.8e, %.8e, %.8e)\n",
        fx, fy, fz, fw,
        ptr[0], ptr[1], ptr[2], ptr[3]);
}

#define check_readback_pixel_4bpp(rb, x, y, color, todo) _check_readback_pixel_4bpp(__LINE__, rb, x, y, color, todo)
static inline void _check_readback_pixel_4bpp(unsigned int line, struct surface_readback *rb, uint32_t x,
        uint32_t y, uint32_t expected_color, BOOL todo)
{
   uint32_t color = get_readback_color(rb, x, y);
   todo_wine_if(todo) ok_(__FILE__, line)(color == expected_color, "Got color 0x%08x, expected 0x%08x.\n", color, expected_color);
}

static const PALETTEENTRY test_palette[256] =
{
    {0x00,0x00,0x00,0x00}, {0x00,0x00,0x80,0x01}, {0x00,0x80,0x00,0x02}, {0x00,0x80,0x80,0x03},
    {0x80,0x00,0x00,0x04}, {0x80,0x00,0x80,0x05}, {0x80,0x80,0x00,0x06}, {0xc0,0xc0,0xc0,0x07},
    {0xc0,0xdc,0xc0,0x08}, {0xf0,0xca,0xa6,0x09}, {0x00,0x20,0x40,0x0a}, {0x00,0x20,0x60,0x0b},
    {0x00,0x20,0x80,0x0c}, {0x00,0x20,0xa0,0x0d}, {0x00,0x20,0xc0,0x0e}, {0x00,0x20,0xe0,0x0f},

    {0x00,0x40,0x00,0x10}, {0x00,0x40,0x20,0x11}, {0x00,0x40,0x40,0x12}, {0x00,0x40,0x60,0x13},
    {0x00,0x40,0x80,0x14}, {0x00,0x40,0xa0,0x15}, {0x00,0x40,0xc0,0x16}, {0x00,0x40,0xe0,0x17},
    {0x00,0x60,0x00,0x18}, {0x00,0x60,0x20,0x19}, {0x00,0x60,0x40,0x1a}, {0x00,0x60,0x60,0x1b},
    {0x00,0x60,0x80,0x1c}, {0x00,0x60,0xa0,0x1d}, {0x00,0x60,0xc0,0x1e}, {0x00,0x60,0xe0,0x1f},

    {0x00,0x80,0x00,0x20}, {0x00,0x80,0x20,0x21}, {0x00,0x80,0x40,0x22}, {0x00,0x80,0x60,0x23},
    {0x00,0x80,0x80,0x24}, {0x00,0x80,0xa0,0x25}, {0x00,0x80,0xc0,0x26}, {0x00,0x80,0xe0,0x27},
    {0x00,0xa0,0x00,0x28}, {0x00,0xa0,0x20,0x29}, {0x00,0xa0,0x40,0x2a}, {0x00,0xa0,0x60,0x2b},
    {0x00,0xa0,0x80,0x2c}, {0x00,0xa0,0xa0,0x2d}, {0x00,0xa0,0xc0,0x2e}, {0x00,0xa0,0xe0,0x2f},

    {0x00,0xc0,0x00,0x30}, {0x00,0xc0,0x20,0x31}, {0x00,0xc0,0x40,0x32}, {0x00,0xc0,0x60,0x33},
    {0x00,0xc0,0x80,0x34}, {0x00,0xc0,0xa0,0x35}, {0x00,0xc0,0xc0,0x36}, {0x00,0xc0,0xe0,0x37},
    {0x00,0xe0,0x00,0x38}, {0x00,0xe0,0x20,0x39}, {0x00,0xe0,0x40,0x3a}, {0x00,0xe0,0x60,0x3b},
    {0x00,0xe0,0x80,0x3c}, {0x00,0xe0,0xa0,0x3d}, {0x00,0xe0,0xc0,0x3e}, {0x00,0xe0,0xe0,0x3f},

    {0x40,0x00,0x00,0x40}, {0x40,0x00,0x20,0x41}, {0x40,0x00,0x40,0x42}, {0x40,0x00,0x60,0x43},
    {0x40,0x00,0x80,0x44}, {0x40,0x00,0xa0,0x45}, {0x40,0x00,0xc0,0x46}, {0x40,0x00,0xe0,0x47},
    {0x40,0x20,0x00,0x48}, {0x40,0x20,0x20,0x49}, {0x40,0x20,0x40,0x4a}, {0x40,0x20,0x60,0x4b},
    {0x40,0x20,0x80,0x4c}, {0x40,0x20,0xa0,0x4d}, {0x40,0x20,0xc0,0x4e}, {0x40,0x20,0xe0,0x4f},

    {0x40,0x40,0x00,0x50}, {0x40,0x40,0x20,0x51}, {0x40,0x40,0x40,0x52}, {0x40,0x40,0x60,0x53},
    {0x40,0x40,0x80,0x54}, {0x40,0x40,0xa0,0x55}, {0x40,0x40,0xc0,0x56}, {0x40,0x40,0xe0,0x57},
    {0x40,0x60,0x00,0x58}, {0x40,0x60,0x20,0x59}, {0x40,0x60,0x40,0x5a}, {0x40,0x60,0x60,0x5b},
    {0x40,0x60,0x80,0x5c}, {0x40,0x60,0xa0,0x5d}, {0x40,0x60,0xc0,0x5e}, {0x40,0x60,0xe0,0x5f},

    {0x40,0x80,0x00,0x60}, {0x40,0x80,0x20,0x61}, {0x40,0x80,0x40,0x62}, {0x40,0x80,0x60,0x63},
    {0x40,0x80,0x80,0x64}, {0x40,0x80,0xa0,0x65}, {0x40,0x80,0xc0,0x66}, {0x40,0x80,0xe0,0x67},
    {0x40,0xa0,0x00,0x68}, {0x40,0xa0,0x20,0x69}, {0x40,0xa0,0x40,0x6a}, {0x40,0xa0,0x60,0x6b},
    {0x40,0xa0,0x80,0x6c}, {0x40,0xa0,0xa0,0x6d}, {0x40,0xa0,0xc0,0x6e}, {0x40,0xa0,0xe0,0x6f},

    {0x40,0xc0,0x00,0x70}, {0x40,0xc0,0x20,0x71}, {0x40,0xc0,0x40,0x72}, {0x40,0xc0,0x60,0x73},
    {0x40,0xc0,0x80,0x74}, {0x40,0xc0,0xa0,0x75}, {0x40,0xc0,0xc0,0x76}, {0x40,0xc0,0xe0,0x77},
    {0x40,0xe0,0x00,0x78}, {0x40,0xe0,0x20,0x79}, {0x40,0xe0,0x40,0x7a}, {0x40,0xe0,0x60,0x7b},
    {0x40,0xe0,0x80,0x7c}, {0x40,0xe0,0xa0,0x7d}, {0x40,0xe0,0xc0,0x7e}, {0x40,0xe0,0xe0,0x7f},

    {0x80,0x00,0x00,0x80}, {0x80,0x00,0x20,0x81}, {0x80,0x00,0x40,0x82}, {0x80,0x00,0x60,0x83},
    {0x80,0x00,0x80,0x84}, {0x80,0x00,0xa0,0x85}, {0x80,0x00,0xc0,0x86}, {0x80,0x00,0xe0,0x87},
    {0x80,0x20,0x00,0x88}, {0x80,0x20,0x20,0x89}, {0x80,0x20,0x40,0x8a}, {0x80,0x20,0x60,0x8b},
    {0x80,0x20,0x80,0x8c}, {0x80,0x20,0xa0,0x8d}, {0x80,0x20,0xc0,0x8e}, {0x80,0x20,0xe0,0x8f},

    {0x80,0x40,0x00,0x90}, {0x80,0x40,0x20,0x91}, {0x80,0x40,0x40,0x92}, {0x80,0x40,0x60,0x93},
    {0x80,0x40,0x80,0x94}, {0x80,0x40,0xa0,0x95}, {0x80,0x40,0xc0,0x96}, {0x80,0x40,0xe0,0x97},
    {0x80,0x60,0x00,0x98}, {0x80,0x60,0x20,0x99}, {0x80,0x60,0x40,0x9a}, {0x80,0x60,0x60,0x9b},
    {0x80,0x60,0x80,0x9c}, {0x80,0x60,0xa0,0x9d}, {0x80,0x60,0xc0,0x9e}, {0x80,0x60,0xe0,0x9f},

    {0x80,0x80,0x00,0xa0}, {0x80,0x80,0x20,0xa1}, {0x80,0x80,0x40,0xa2}, {0x80,0x80,0x60,0xa3},
    {0x80,0x80,0x80,0xa4}, {0x80,0x80,0xa0,0xa5}, {0x80,0x80,0xc0,0xa6}, {0x80,0x80,0xe0,0xa7},
    {0x80,0xa0,0x00,0xa8}, {0x80,0xa0,0x20,0xa9}, {0x80,0xa0,0x40,0xaa}, {0x80,0xa0,0x60,0xab},
    {0x80,0xa0,0x80,0xac}, {0x80,0xa0,0xa0,0xad}, {0x80,0xa0,0xc0,0xae}, {0x80,0xa0,0xe0,0xaf},

    {0x80,0xc0,0x00,0xb0}, {0x80,0xc0,0x20,0xb1}, {0x80,0xc0,0x40,0xb2}, {0x80,0xc0,0x60,0xb3},
    {0x80,0xc0,0x80,0xb4}, {0x80,0xc0,0xa0,0xb5}, {0x80,0xc0,0xc0,0xb6}, {0x80,0xc0,0xe0,0xb7},
    {0x80,0xe0,0x00,0xb8}, {0x80,0xe0,0x20,0xb9}, {0x80,0xe0,0x40,0xba}, {0x80,0xe0,0x60,0xbb},
    {0x80,0xe0,0x80,0xbc}, {0x80,0xe0,0xa0,0xbd}, {0x80,0xe0,0xc0,0xbe}, {0x80,0xe0,0xe0,0xbf},

    {0xc0,0x00,0x00,0xc0}, {0xc0,0x00,0x20,0xc1}, {0xc0,0x00,0x40,0xc2}, {0xc0,0x00,0x60,0xc3},
    {0xc0,0x00,0x80,0xc4}, {0xc0,0x00,0xa0,0xc5}, {0xc0,0x00,0xc0,0xc6}, {0xc0,0x00,0xe0,0xc7},
    {0xc0,0x20,0x00,0xc8}, {0xc0,0x20,0x20,0xc9}, {0xc0,0x20,0x40,0xca}, {0xc0,0x20,0x60,0xcb},
    {0xc0,0x20,0x80,0xcc}, {0xc0,0x20,0xa0,0xcd}, {0xc0,0x20,0xc0,0xce}, {0xc0,0x20,0xe0,0xcf},

    {0xc0,0x40,0x00,0xd0}, {0xc0,0x40,0x20,0xd1}, {0xc0,0x40,0x40,0xd2}, {0xc0,0x40,0x60,0xd3},
    {0xc0,0x40,0x80,0xd4}, {0xc0,0x40,0xa0,0xd5}, {0xc0,0x40,0xc0,0xd6}, {0xc0,0x40,0xe0,0xd7},
    {0xc0,0x60,0x00,0xd8}, {0xc0,0x60,0x20,0xd9}, {0xc0,0x60,0x40,0xda}, {0xc0,0x60,0x60,0xdb},
    {0xc0,0x60,0x80,0xdc}, {0xc0,0x60,0xa0,0xdd}, {0xc0,0x60,0xc0,0xde}, {0xc0,0x60,0xe0,0xdf},

    {0xc0,0x80,0x00,0xe0}, {0xc0,0x80,0x20,0xe1}, {0xc0,0x80,0x40,0xe2}, {0xc0,0x80,0x60,0xe3},
    {0xc0,0x80,0x80,0xe4}, {0xc0,0x80,0xa0,0xe5}, {0xc0,0x80,0xc0,0xe6}, {0xc0,0x80,0xe0,0xe7},
    {0xc0,0xa0,0x00,0xe8}, {0xc0,0xa0,0x20,0xe9}, {0xc0,0xa0,0x40,0xea}, {0xc0,0xa0,0x60,0xeb},
    {0xc0,0xa0,0x80,0xec}, {0xc0,0xa0,0xa0,0xed}, {0xc0,0xa0,0xc0,0xee}, {0xc0,0xa0,0xe0,0xef},

    {0xc0,0xc0,0x00,0xf0}, {0xc0,0xc0,0x20,0xf1}, {0xc0,0xc0,0x40,0xf2}, {0xc0,0xc0,0x60,0xf3},
    {0xc0,0xc0,0x80,0xf4}, {0xc0,0xc0,0xa0,0xf5}, {0xf0,0xfb,0xff,0xf6}, {0xa4,0xa0,0xa0,0xf7},
    {0x80,0x80,0x80,0xf8}, {0x00,0x00,0xff,0xf9}, {0x00,0xff,0x00,0xfa}, {0x00,0xff,0xff,0xfb},
    {0xff,0x00,0x00,0xfc}, {0xff,0x00,0xff,0xfd}, {0xff,0xff,0x00,0xfe}, {0xff,0xff,0xff,0xff},
};

static const uint16_t v16u16_2_2[] =
{
    0x0000, 0x3000, 0x4000, 0x7fff, 0x8000, 0x8001, 0xc000, 0xffff,
};

static const uint8_t v16u16_2_2_expected[] =
{
    0x00,0x80,0x00,0xb0,0x00,0xc0,0xff,0xff,0x00,0x00,0x00,0x00,0xff,0x3f,0xff,0x7f,
};

static const uint8_t v16u16_2_2_expected2[] =
{
    0x00,0x00,0x00,0x00,0x80,0x01,0xc0,0x3e,0x00,0x00,0x80,0x3f,0x00,0x00,0x80,0x3f,
    0x00,0x01,0x00,0x3f,0x00,0x00,0x80,0x3f,0x00,0x00,0x80,0x3f,0x00,0x00,0x80,0x3f,
    0x00,0x00,0x80,0xbf,0x00,0x00,0x80,0xbf,0x00,0x00,0x80,0x3f,0x00,0x00,0x80,0x3f,
    0x00,0x01,0x00,0xbf,0x00,0x01,0x00,0xb8,0x00,0x00,0x80,0x3f,0x00,0x00,0x80,0x3f,
};

static const uint8_t v8u8_2_2[] =
{
    0x00,0x30,0x40,0x7f,0x80,0x81,0xc0,0xff,
};

static const uint8_t v8u8_2_2_expected[] =
{
    0x00,0x00,0x00,0x00,0x06,0x83,0xc1,0x3e,0x00,0x00,0x80,0x3f,0x00,0x00,0x80,0x3f,
    0x04,0x02,0x01,0x3f,0x00,0x00,0x80,0x3f,0x00,0x00,0x80,0x3f,0x00,0x00,0x80,0x3f,
    0x00,0x00,0x80,0xbf,0x00,0x00,0x80,0xbf,0x00,0x00,0x80,0x3f,0x00,0x00,0x80,0x3f,
    0x04,0x02,0x01,0xbf,0x04,0x02,0x01,0xbc,0x00,0x00,0x80,0x3f,0x00,0x00,0x80,0x3f,
};

static const uint16_t a16b16g16r16_2_2[] =
{
    0x0000,0x1000,0x2000,0x3000,0x4000,0x5000,0x6000,0x7000,
    0x8000,0x9000,0xa000,0xb000,0xc000,0xd000,0xe000,0xffff,
};

static const uint8_t a16b16g16r16_2_2_expected[] =
{
    0x00,0x00,0x00,0x00,0x80,0x00,0x80,0x3d,0x80,0x00,0x00,0x3e,0xc0,0x00,0x40,0x3e,
    0x80,0x00,0x80,0x3e,0xa0,0x00,0xa0,0x3e,0xc0,0x00,0xc0,0x3e,0xe0,0x00,0xe0,0x3e,
    0x80,0x00,0x00,0x3f,0x90,0x00,0x10,0x3f,0xa0,0x00,0x20,0x3f,0xb0,0x00,0x30,0x3f,
    0xc0,0x00,0x40,0x3f,0xd0,0x00,0x50,0x3f,0xe0,0x00,0x60,0x3f,0x00,0x00,0x80,0x3f,
};

static const uint16_t q16w16v16u16_2_2[] =
{
    0x0000,0x1000,0x2000,0x3000,0x4000,0x5000,0x6000,0x7fff,
    0x8000,0x8001,0xa000,0xb000,0xc000,0xd000,0xe000,0xffff,
};

static const uint8_t q16w16v16u16_2_2_expected[] =
{
    0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x3e,0x00,0x01,0x80,0x3e,0x80,0x01,0xc0,0x3e,
    0x00,0x01,0x00,0x3f,0x40,0x01,0x20,0x3f,0x80,0x01,0x40,0x3f,0x00,0x00,0x80,0x3f,
    0x00,0x00,0x80,0xbf,0x00,0x00,0x80,0xbf,0x80,0x01,0x40,0xbf,0x40,0x01,0x20,0xbf,
    0x00,0x01,0x00,0xbf,0x80,0x01,0xc0,0xbe,0x00,0x01,0x80,0xbe,0x00,0x01,0x00,0xb8,
};

static const uint8_t p8_2_2[] =
{
    0x00,0x40,0x80,0xff,
};

static const uint8_t p8_2_2_expected[] =
{
    0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x40,0x00,0x00,0x80,0x80,0xff,0xff,0xff,0xff,
};

static const uint8_t a8p8_2_2[] =
{
    0x00,0x10,0x40,0x20,0x80,0x30,0xff,0x40
};

static const uint8_t a8p8_2_2_expected[] =
{
    0x00,0x00,0x00,0x10,0x00,0x00,0x40,0x20,0x00,0x00,0x80,0x30,0xff,0xff,0xff,0x40,
};

static uint32_t get_bpp_for_d3dformat(D3DFORMAT format)
{
    switch (format)
    {
    case D3DFMT_A32B32G32R32F:
        return 16;

    case D3DFMT_A16B16G16R16:
    case D3DFMT_Q16W16V16U16:
        return 8;

    case D3DFMT_A8B8G8R8:
    case D3DFMT_A8R8G8B8:
    case D3DFMT_V16U16:
    case D3DFMT_G16R16:
        return 4;

    case D3DFMT_R8G8B8:
        return 3;

    case D3DFMT_X1R5G5B5:
    case D3DFMT_A1R5G5B5:
    case D3DFMT_V8U8:
    case D3DFMT_A8P8:
        return 2;

    case D3DFMT_L8:
    case D3DFMT_P8:
        return 1;

    default:
        assert(0 && "Need to add format to get_bpp_for_d3dformat().");
        return 0;
    }
}

static void test_format_conversion(IDirect3DDevice9 *device)
{
    struct
    {
        D3DFORMAT src_format;
        const PALETTEENTRY *src_palette;
        const RECT src_rect;
        const void *src_data;

        D3DFORMAT dst_format;
        const void *expected_dst_data;
        BOOL todo;
    } tests[] = {
        { D3DFMT_P8,            test_palette, { 0, 0, 2, 2 }, p8_2_2,           D3DFMT_A8R8G8B8,      p8_2_2_expected },
        { D3DFMT_A16B16G16R16,  NULL,         { 0, 0, 2, 2 }, a16b16g16r16_2_2, D3DFMT_A32B32G32R32F, a16b16g16r16_2_2_expected },
        { D3DFMT_V16U16,        NULL,         { 0, 0, 2, 2 }, v16u16_2_2,       D3DFMT_G16R16,        v16u16_2_2_expected },
        { D3DFMT_V16U16,        NULL,         { 0, 0, 2, 2 }, v16u16_2_2,       D3DFMT_A32B32G32R32F, v16u16_2_2_expected2 },
        { D3DFMT_V8U8,          NULL,         { 0, 0, 2, 2 }, v8u8_2_2,         D3DFMT_A32B32G32R32F, v8u8_2_2_expected },
        { D3DFMT_Q16W16V16U16,  NULL,         { 0, 0, 2, 2 }, q16w16v16u16_2_2, D3DFMT_A32B32G32R32F, q16w16v16u16_2_2_expected },
        { D3DFMT_A8P8,          test_palette, { 0, 0, 2, 2 }, a8p8_2_2,         D3DFMT_A8R8G8B8,      a8p8_2_2_expected },
    };
    uint32_t i;

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        IDirect3DTexture9 *tex;
        HRESULT hr;

        winetest_push_context("Test %u", i);
        hr = IDirect3DDevice9_CreateTexture(device, tests[i].src_rect.right, tests[i].src_rect.bottom, 1, 0,
                tests[i].dst_format, D3DPOOL_MANAGED, &tex, NULL);
        if (SUCCEEDED(hr))
        {
            const uint32_t src_pitch = get_bpp_for_d3dformat(tests[i].src_format) * tests[i].src_rect.right;
            const uint32_t dst_pitch = get_bpp_for_d3dformat(tests[i].dst_format) * tests[i].src_rect.right;
            IDirect3DSurface9 *surf;

            hr = IDirect3DTexture9_GetSurfaceLevel(tex, 0, &surf);
            ok(hr == D3D_OK, "Failed to get the surface, hr %#lx.\n", hr);

            hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, tests[i].src_data, tests[i].src_format,
                    src_pitch, tests[i].src_palette, &tests[i].src_rect, D3DX_FILTER_NONE, 0);
            todo_wine_if(tests[i].todo) ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
            if (SUCCEEDED(hr))
            {
                const uint32_t dst_fmt_bpp = get_bpp_for_d3dformat(tests[i].dst_format);
                D3DLOCKED_RECT lock_rect;
                uint32_t x, y;

                IDirect3DSurface9_LockRect(surf, &lock_rect, NULL, D3DLOCK_READONLY);
                for (y = 0; y < tests[i].src_rect.bottom; ++y)
                {
                    const uint8_t *dst_expected_row = ((uint8_t *)tests[i].expected_dst_data) + (dst_pitch * y);
                    const uint8_t *dst_row = ((uint8_t *)lock_rect.pBits) + (lock_rect.Pitch * y);

                    for (x = 0; x < tests[i].src_rect.right; ++x)
                    {
                        const uint8_t *dst_expected_pixel = dst_expected_row + (dst_fmt_bpp * x);
                        const uint8_t *dst_pixel = dst_row + (dst_fmt_bpp * x);
                        BOOL pixel_match = !memcmp(dst_pixel, dst_expected_pixel, dst_fmt_bpp);

                        todo_wine_if(tests[i].todo) ok(pixel_match, "Pixel mismatch at (%u,%u).\n", x, y);
                    }
                }

                IDirect3DSurface9_UnlockRect(surf);
            }

            check_release((IUnknown *)surf, 1);
            check_release((IUnknown *)tex, 0);
        }
        else
        {
            skip("Failed to create texture for format %d, hr %#lx.\n", tests[i].dst_format, hr);
        }
        winetest_pop_context();
    }
}

static void test_dxt_premultiplied_alpha(IDirect3DDevice9 *device)
{
    static const uint32_t dxt_pma_decompressed_expected[] =
    {
        0x00000000, 0x22ffffff, 0x44ffffff, 0x66ffffff, 0x88f7f3f7, 0xaac5c2c5, 0xcca5a2a5, 0xff848284,
        0x00000000, 0x22ffffff, 0x44ffffff, 0x66ffffff, 0x88f7f3f7, 0xaac5c2c5, 0xcca5a2a5, 0xff848284,
    };
    static const uint32_t dxt_decompressed_expected[] =
    {
        0x00848284, 0x22848284, 0x44848284, 0x66848284, 0x88848284, 0xaa848284, 0xcc848284, 0xff848284,
        0x00848284, 0x22848284, 0x44848284, 0x66848284, 0x88848284, 0xaa848284, 0xcc848284, 0xff848284,
    };
    static const uint8_t dxt3_block[] =
    {
        0x20,0x64,0xa8,0xfc,0x20,0x64,0xa8,0xfc,0x10,0x84,0x10,0x84,0x00,0x00,0x00,0x00,
    };
    static const uint8_t dxt5_block[] =
    {
        0x22,0xcc,0x86,0xc6,0xe6,0x86,0xc6,0xe6,0x10,0x84,0x10,0x84,0x00,0x00,0x00,0x00,
    };
    static const uint32_t test_compress_pixels[] =
    {
        0xffffffff, 0x00ffffff, 0xffffffff, 0x00ffffff, 0xffffffff, 0x00ffffff, 0xffffffff, 0x00ffffff,
        0xffffffff, 0x00ffffff, 0xffffffff, 0x00ffffff, 0xffffffff, 0x00ffffff, 0xffffffff, 0x00ffffff,
    };
    static const struct test
    {
        D3DFORMAT pma_fmt;
        D3DFORMAT nonpma_fmt;
        const uint8_t *dxt_block;
        const char *name;
    } tests[] =
    {
        { D3DFMT_DXT2, D3DFMT_DXT3, dxt3_block, "DXT2 / DXT3" },
        { D3DFMT_DXT4, D3DFMT_DXT5, dxt5_block, "DXT4 / DXT5" },
    };
    static const RECT src_rect = { 0, 0, 4, 4 };
    IDirect3DSurface9 *decomp_surf;
    D3DLOCKED_RECT lock_rect;
    uint32_t i, x, y;
    HRESULT hr;

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 4, 4, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &decomp_surf, NULL);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        struct surface_readback surface_rb;
        IDirect3DSurface9 *pma_surf, *surf;
        IDirect3DTexture9 *pma_tex, *tex;

        winetest_push_context("Test %s", tests[i].name);
        hr = IDirect3DDevice9_CreateTexture(device, 4, 4, 1, 0, tests[i].pma_fmt, D3DPOOL_SYSTEMMEM, &pma_tex, NULL);
        if (FAILED(hr))
        {
            skip("Failed to create texture for format %#x, hr %#lx.\n", tests[i].pma_fmt, hr);
            winetest_pop_context();
            continue;
        }

        hr = IDirect3DDevice9_CreateTexture(device, 4, 4, 1, 0, tests[i].nonpma_fmt, D3DPOOL_SYSTEMMEM, &tex, NULL);
        if (FAILED(hr))
        {
            skip("Failed to create texture for format %#x, hr %#lx.\n", tests[i].nonpma_fmt, hr);
            IDirect3DTexture9_Release(pma_tex);
            winetest_pop_context();
            continue;
        }

        hr = IDirect3DTexture9_GetSurfaceLevel(pma_tex, 0, &pma_surf);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

        hr = IDirect3DTexture9_GetSurfaceLevel(tex, 0, &surf);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

        /* Compress and load the same image onto each DXT surface. */
        hr = D3DXLoadSurfaceFromMemory(pma_surf, NULL, NULL, test_compress_pixels, D3DFMT_A8B8G8R8, 16, NULL, &src_rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, test_compress_pixels, D3DFMT_A8B8G8R8, 16, NULL, &src_rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

        /*
         * For DXT2/DXT4, the source image has all of its color channels
         * premultiplied by the alpha value prior to compression. If the
         * alpha channel's value is 0, then the color channel values are
         * lost.
         */
        get_surface_decompressed_readback(device, pma_surf, &surface_rb);
        for (y = 0; y < 4; ++y)
        {
            for (x = 0; x < 4; ++x)
            {
                const uint32_t expected_pixel = !(x & 0x01) ? 0xffffffff : 0x00000000;

                check_readback_pixel_4bpp(&surface_rb, x, y, expected_pixel, !expected_pixel);
            }
        }
        release_surface_readback(&surface_rb);

        /* For DXT3/DXT5, no premultiplication by the alpha channel value is done. */
        get_surface_decompressed_readback(device, surf, &surface_rb);
        for (y = 0; y < 4; ++y)
        {
            for (x = 0; x < 4; ++x)
                check_readback_pixel_4bpp(&surface_rb, x, y, test_compress_pixels[(y * 4) + x], FALSE);
        }
        release_surface_readback(&surface_rb);

        /*
         * Load our test DXT block with the premultiplied alpha DXT format.
         * The block is decompressed, and then the premultiplied alpha
         * operation is undone prior to being copied to the destination
         * surface.
         */
        hr = D3DXLoadSurfaceFromMemory(decomp_surf, NULL, NULL, tests[i].dxt_block, tests[i].pma_fmt, 16, NULL, &src_rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
        IDirect3DSurface9_LockRect(decomp_surf, &lock_rect, NULL, D3DLOCK_READONLY);
        for (y = 0; y < 4; ++y)
        {
            for (x = 0; x < 4; ++x)
            {
                const uint32_t expected_pixel = dxt_pma_decompressed_expected[(y * 4) + x];
                const BOOL todo = ((expected_pixel >> 24) & 0xff) != 0xff;

                todo_wine_if(todo) check_pixel_4bpp(&lock_rect, x, y, expected_pixel);
            }
        }
        IDirect3DSurface9_UnlockRect(decomp_surf);

        /*
         * Load our test DXT block as a non-premultiplied alpha DXT format.
         * The block is decompressed, and the data is copied over directly.
         */
        hr = D3DXLoadSurfaceFromMemory(decomp_surf, NULL, NULL, tests[i].dxt_block, tests[i].nonpma_fmt, 16, NULL, &src_rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
        IDirect3DSurface9_LockRect(decomp_surf, &lock_rect, NULL, D3DLOCK_READONLY);
        for (y = 0; y < 4; ++y)
        {
            for (x = 0; x < 4; ++x)
                check_pixel_4bpp(&lock_rect, x, y, dxt_decompressed_expected[(y * 4) + x]);
        }
        IDirect3DSurface9_UnlockRect(decomp_surf);

        IDirect3DSurface9_Release(pma_surf);
        IDirect3DTexture9_Release(pma_tex);
        IDirect3DSurface9_Release(surf);
        IDirect3DTexture9_Release(tex);
        winetest_pop_context();
    }
    IDirect3DSurface9_Release(decomp_surf);
}

static const uint8_t test_tga_color_map_15bpp[] =
{
    0x00,0x00,0x10,0x00,0x00,0x02,0x10,0x02,0x00,0x40,0x10,0x40,0x00,0x42,0x18,0x63,
    0x78,0x63,0x34,0x7b,0x88,0x00,0x8c,0x00,0x90,0x00,0x94,0x00,0x98,0x00,0x9c,0x00,
    0x00,0x01,0x04,0x01,0x08,0x01,0x0c,0x01,0x10,0x01,0x14,0x01,0x18,0x01,0x1c,0x01,
    0x80,0x01,0x84,0x01,0x88,0x01,0x8c,0x01,0x90,0x01,0x94,0x01,0x98,0x01,0x9c,0x01,
    0x00,0x02,0x04,0x02,0x08,0x02,0x0c,0x02,0x10,0x02,0x14,0x02,0x18,0x02,0x1c,0x02,
    0x80,0x02,0x84,0x02,0x88,0x02,0x8c,0x02,0x90,0x02,0x94,0x02,0x98,0x02,0x9c,0x02,
    0x00,0x03,0x04,0x03,0x08,0x03,0x0c,0x03,0x10,0x03,0x14,0x03,0x18,0x03,0x1c,0x03,
    0x80,0x03,0x84,0x03,0x88,0x03,0x8c,0x03,0x90,0x03,0x94,0x03,0x98,0x03,0x9c,0x03,
    0x00,0x20,0x04,0x20,0x08,0x20,0x0c,0x20,0x10,0x20,0x14,0x20,0x18,0x20,0x1c,0x20,
    0x80,0x20,0x84,0x20,0x88,0x20,0x8c,0x20,0x90,0x20,0x94,0x20,0x98,0x20,0x9c,0x20,
    0x00,0x21,0x04,0x21,0x08,0x21,0x0c,0x21,0x10,0x21,0x14,0x21,0x18,0x21,0x1c,0x21,
    0x80,0x21,0x84,0x21,0x88,0x21,0x8c,0x21,0x90,0x21,0x94,0x21,0x98,0x21,0x9c,0x21,
    0x00,0x22,0x04,0x22,0x08,0x22,0x0c,0x22,0x10,0x22,0x14,0x22,0x18,0x22,0x1c,0x22,
    0x80,0x22,0x84,0x22,0x88,0x22,0x8c,0x22,0x90,0x22,0x94,0x22,0x98,0x22,0x9c,0x22,
    0x00,0x23,0x04,0x23,0x08,0x23,0x0c,0x23,0x10,0x23,0x14,0x23,0x18,0x23,0x1c,0x23,
    0x80,0x23,0x84,0x23,0x88,0x23,0x8c,0x23,0x90,0x23,0x94,0x23,0x98,0x23,0x9c,0x23,
    0x00,0x40,0x04,0x40,0x08,0x40,0x0c,0x40,0x10,0x40,0x14,0x40,0x18,0x40,0x1c,0x40,
    0x80,0x40,0x84,0x40,0x88,0x40,0x8c,0x40,0x90,0x40,0x94,0x40,0x98,0x40,0x9c,0x40,
    0x00,0x41,0x04,0x41,0x08,0x41,0x0c,0x41,0x10,0x41,0x14,0x41,0x18,0x41,0x1c,0x41,
    0x80,0x41,0x84,0x41,0x88,0x41,0x8c,0x41,0x90,0x41,0x94,0x41,0x98,0x41,0x9c,0x41,
    0x00,0x42,0x04,0x42,0x08,0x42,0x0c,0x42,0x10,0x42,0x14,0x42,0x18,0x42,0x1c,0x42,
    0x80,0x42,0x84,0x42,0x88,0x42,0x8c,0x42,0x90,0x42,0x94,0x42,0x98,0x42,0x9c,0x42,
    0x00,0x43,0x04,0x43,0x08,0x43,0x0c,0x43,0x10,0x43,0x14,0x43,0x18,0x43,0x1c,0x43,
    0x80,0x43,0x84,0x43,0x88,0x43,0x8c,0x43,0x90,0x43,0x94,0x43,0x98,0x43,0x9c,0x43,
    0x00,0x60,0x04,0x60,0x08,0x60,0x0c,0x60,0x10,0x60,0x14,0x60,0x18,0x60,0x1c,0x60,
    0x80,0x60,0x84,0x60,0x88,0x60,0x8c,0x60,0x90,0x60,0x94,0x60,0x98,0x60,0x9c,0x60,
    0x00,0x61,0x04,0x61,0x08,0x61,0x0c,0x61,0x10,0x61,0x14,0x61,0x18,0x61,0x1c,0x61,
    0x80,0x61,0x84,0x61,0x88,0x61,0x8c,0x61,0x90,0x61,0x94,0x61,0x98,0x61,0x9c,0x61,
    0x00,0x62,0x04,0x62,0x08,0x62,0x0c,0x62,0x10,0x62,0x14,0x62,0x18,0x62,0x1c,0x62,
    0x80,0x62,0x84,0x62,0x88,0x62,0x8c,0x62,0x90,0x62,0x94,0x62,0x98,0x62,0x9c,0x62,
    0x00,0x63,0x04,0x63,0x08,0x63,0x0c,0x63,0x10,0x63,0x14,0x63,0xff,0x7b,0x94,0x52,
    0x10,0x42,0x1f,0x00,0xe0,0x03,0xff,0x03,0x00,0x7c,0x1f,0x7c,0xe0,0x7f,0xff,0x7f,
};

static const uint8_t test_tga_color_map_16bpp[] =
{
    0x00,0x00,0x10,0x00,0x00,0x02,0x10,0x02,0x00,0x40,0x10,0x40,0x00,0x42,0x18,0x63,
    0x78,0x63,0x34,0x7b,0x88,0x00,0x8c,0x00,0x90,0x00,0x94,0x00,0x98,0x00,0x9c,0x00,
    0x00,0x01,0x04,0x01,0x08,0x01,0x0c,0x01,0x10,0x01,0x14,0x01,0x18,0x01,0x1c,0x01,
    0x80,0x01,0x84,0x01,0x88,0x01,0x8c,0x01,0x90,0x01,0x94,0x01,0x98,0x01,0x9c,0x01,
    0x00,0x02,0x04,0x02,0x08,0x02,0x0c,0x02,0x10,0x02,0x14,0x02,0x18,0x02,0x1c,0x02,
    0x80,0x02,0x84,0x02,0x88,0x02,0x8c,0x02,0x90,0x02,0x94,0x02,0x98,0x02,0x9c,0x02,
    0x00,0x03,0x04,0x03,0x08,0x03,0x0c,0x03,0x10,0x03,0x14,0x03,0x18,0x03,0x1c,0x03,
    0x80,0x03,0x84,0x03,0x88,0x03,0x8c,0x03,0x90,0x03,0x94,0x03,0x98,0x03,0x9c,0x03,
    0x00,0x20,0x04,0x20,0x08,0x20,0x0c,0x20,0x10,0x20,0x14,0x20,0x18,0x20,0x1c,0x20,
    0x80,0x20,0x84,0x20,0x88,0x20,0x8c,0x20,0x90,0x20,0x94,0x20,0x98,0x20,0x9c,0x20,
    0x00,0x21,0x04,0x21,0x08,0x21,0x0c,0x21,0x10,0x21,0x14,0x21,0x18,0x21,0x1c,0x21,
    0x80,0x21,0x84,0x21,0x88,0x21,0x8c,0x21,0x90,0x21,0x94,0x21,0x98,0x21,0x9c,0x21,
    0x00,0x22,0x04,0x22,0x08,0x22,0x0c,0x22,0x10,0x22,0x14,0x22,0x18,0x22,0x1c,0x22,
    0x80,0x22,0x84,0x22,0x88,0x22,0x8c,0x22,0x90,0x22,0x94,0x22,0x98,0x22,0x9c,0x22,
    0x00,0x23,0x04,0x23,0x08,0x23,0x0c,0x23,0x10,0x23,0x14,0x23,0x18,0x23,0x1c,0x23,
    0x80,0x23,0x84,0x23,0x88,0x23,0x8c,0x23,0x90,0x23,0x94,0x23,0x98,0x23,0x9c,0x23,
    0x00,0xc0,0x04,0xc0,0x08,0xc0,0x0c,0xc0,0x10,0xc0,0x14,0xc0,0x18,0xc0,0x1c,0xc0,
    0x80,0xc0,0x84,0xc0,0x88,0xc0,0x8c,0xc0,0x90,0xc0,0x94,0xc0,0x98,0xc0,0x9c,0xc0,
    0x00,0xc1,0x04,0xc1,0x08,0xc1,0x0c,0xc1,0x10,0xc1,0x14,0xc1,0x18,0xc1,0x1c,0xc1,
    0x80,0xc1,0x84,0xc1,0x88,0xc1,0x8c,0xc1,0x90,0xc1,0x94,0xc1,0x98,0xc1,0x9c,0xc1,
    0x00,0xc2,0x04,0xc2,0x08,0xc2,0x0c,0xc2,0x10,0xc2,0x14,0xc2,0x18,0xc2,0x1c,0xc2,
    0x80,0xc2,0x84,0xc2,0x88,0xc2,0x8c,0xc2,0x90,0xc2,0x94,0xc2,0x98,0xc2,0x9c,0xc2,
    0x00,0xc3,0x04,0xc3,0x08,0xc3,0x0c,0xc3,0x10,0xc3,0x14,0xc3,0x18,0xc3,0x1c,0xc3,
    0x80,0xc3,0x84,0xc3,0x88,0xc3,0x8c,0xc3,0x90,0xc3,0x94,0xc3,0x98,0xc3,0x9c,0xc3,
    0x00,0xe0,0x04,0xe0,0x08,0xe0,0x0c,0xe0,0x10,0xe0,0x14,0xe0,0x18,0xe0,0x1c,0xe0,
    0x80,0xe0,0x84,0xe0,0x88,0xe0,0x8c,0xe0,0x90,0xe0,0x94,0xe0,0x98,0xe0,0x9c,0xe0,
    0x00,0xe1,0x04,0xe1,0x08,0xe1,0x0c,0xe1,0x10,0xe1,0x14,0xe1,0x18,0xe1,0x1c,0xe1,
    0x80,0xe1,0x84,0xe1,0x88,0xe1,0x8c,0xe1,0x90,0xe1,0x94,0xe1,0x98,0xe1,0x9c,0xe1,
    0x00,0xe2,0x04,0xe2,0x08,0xe2,0x0c,0xe2,0x10,0xe2,0x14,0xe2,0x18,0xe2,0x1c,0xe2,
    0x80,0xe2,0x84,0xe2,0x88,0xe2,0x8c,0xe2,0x90,0xe2,0x94,0xe2,0x98,0xe2,0x9c,0xe2,
    0x00,0xe3,0x04,0xe3,0x08,0xe3,0x0c,0xe3,0x10,0xe3,0x14,0xe3,0xff,0xfb,0x94,0xd2,
    0x10,0xc2,0x1f,0x80,0xe0,0x83,0xff,0x83,0x00,0xfc,0x1f,0xfc,0xe0,0xff,0xff,0xff,
};

static const uint8_t test_tga_color_map_24bpp[] =
{
    0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x80,0x00,0x80,0x80,0x00,0x00,0x00,0x80,0x80,
    0x00,0x80,0x00,0x80,0x80,0xc0,0xc0,0xc0,0xc0,0xdc,0xc0,0xa6,0xca,0xf0,0x40,0x20,
    0x00,0x60,0x20,0x00,0x80,0x20,0x00,0xa0,0x20,0x00,0xc0,0x20,0x00,0xe0,0x20,0x00,
    0x00,0x40,0x00,0x20,0x40,0x00,0x40,0x40,0x00,0x60,0x40,0x00,0x80,0x40,0x00,0xa0,
    0x40,0x00,0xc0,0x40,0x00,0xe0,0x40,0x00,0x00,0x60,0x00,0x20,0x60,0x00,0x40,0x60,
    0x00,0x60,0x60,0x00,0x80,0x60,0x00,0xa0,0x60,0x00,0xc0,0x60,0x00,0xe0,0x60,0x00,
    0x00,0x80,0x00,0x20,0x80,0x00,0x40,0x80,0x00,0x60,0x80,0x00,0x80,0x80,0x00,0xa0,
    0x80,0x00,0xc0,0x80,0x00,0xe0,0x80,0x00,0x00,0xa0,0x00,0x20,0xa0,0x00,0x40,0xa0,
    0x00,0x60,0xa0,0x00,0x80,0xa0,0x00,0xa0,0xa0,0x00,0xc0,0xa0,0x00,0xe0,0xa0,0x00,
    0x00,0xc0,0x00,0x20,0xc0,0x00,0x40,0xc0,0x00,0x60,0xc0,0x00,0x80,0xc0,0x00,0xa0,
    0xc0,0x00,0xc0,0xc0,0x00,0xe0,0xc0,0x00,0x00,0xe0,0x00,0x20,0xe0,0x00,0x40,0xe0,
    0x00,0x60,0xe0,0x00,0x80,0xe0,0x00,0xa0,0xe0,0x00,0xc0,0xe0,0x00,0xe0,0xe0,0x00,
    0x00,0x00,0x40,0x20,0x00,0x40,0x40,0x00,0x40,0x60,0x00,0x40,0x80,0x00,0x40,0xa0,
    0x00,0x40,0xc0,0x00,0x40,0xe0,0x00,0x40,0x00,0x20,0x40,0x20,0x20,0x40,0x40,0x20,
    0x40,0x60,0x20,0x40,0x80,0x20,0x40,0xa0,0x20,0x40,0xc0,0x20,0x40,0xe0,0x20,0x40,
    0x00,0x40,0x40,0x20,0x40,0x40,0x40,0x40,0x40,0x60,0x40,0x40,0x80,0x40,0x40,0xa0,
    0x40,0x40,0xc0,0x40,0x40,0xe0,0x40,0x40,0x00,0x60,0x40,0x20,0x60,0x40,0x40,0x60,
    0x40,0x60,0x60,0x40,0x80,0x60,0x40,0xa0,0x60,0x40,0xc0,0x60,0x40,0xe0,0x60,0x40,
    0x00,0x80,0x40,0x20,0x80,0x40,0x40,0x80,0x40,0x60,0x80,0x40,0x80,0x80,0x40,0xa0,
    0x80,0x40,0xc0,0x80,0x40,0xe0,0x80,0x40,0x00,0xa0,0x40,0x20,0xa0,0x40,0x40,0xa0,
    0x40,0x60,0xa0,0x40,0x80,0xa0,0x40,0xa0,0xa0,0x40,0xc0,0xa0,0x40,0xe0,0xa0,0x40,
    0x00,0xc0,0x40,0x20,0xc0,0x40,0x40,0xc0,0x40,0x60,0xc0,0x40,0x80,0xc0,0x40,0xa0,
    0xc0,0x40,0xc0,0xc0,0x40,0xe0,0xc0,0x40,0x00,0xe0,0x40,0x20,0xe0,0x40,0x40,0xe0,
    0x40,0x60,0xe0,0x40,0x80,0xe0,0x40,0xa0,0xe0,0x40,0xc0,0xe0,0x40,0xe0,0xe0,0x40,
    0x00,0x00,0x80,0x20,0x00,0x80,0x40,0x00,0x80,0x60,0x00,0x80,0x80,0x00,0x80,0xa0,
    0x00,0x80,0xc0,0x00,0x80,0xe0,0x00,0x80,0x00,0x20,0x80,0x20,0x20,0x80,0x40,0x20,
    0x80,0x60,0x20,0x80,0x80,0x20,0x80,0xa0,0x20,0x80,0xc0,0x20,0x80,0xe0,0x20,0x80,
    0x00,0x40,0x80,0x20,0x40,0x80,0x40,0x40,0x80,0x60,0x40,0x80,0x80,0x40,0x80,0xa0,
    0x40,0x80,0xc0,0x40,0x80,0xe0,0x40,0x80,0x00,0x60,0x80,0x20,0x60,0x80,0x40,0x60,
    0x80,0x60,0x60,0x80,0x80,0x60,0x80,0xa0,0x60,0x80,0xc0,0x60,0x80,0xe0,0x60,0x80,
    0x00,0x80,0x80,0x20,0x80,0x80,0x40,0x80,0x80,0x60,0x80,0x80,0x80,0x80,0x80,0xa0,
    0x80,0x80,0xc0,0x80,0x80,0xe0,0x80,0x80,0x00,0xa0,0x80,0x20,0xa0,0x80,0x40,0xa0,
    0x80,0x60,0xa0,0x80,0x80,0xa0,0x80,0xa0,0xa0,0x80,0xc0,0xa0,0x80,0xe0,0xa0,0x80,
    0x00,0xc0,0x80,0x20,0xc0,0x80,0x40,0xc0,0x80,0x60,0xc0,0x80,0x80,0xc0,0x80,0xa0,
    0xc0,0x80,0xc0,0xc0,0x80,0xe0,0xc0,0x80,0x00,0xe0,0x80,0x20,0xe0,0x80,0x40,0xe0,
    0x80,0x60,0xe0,0x80,0x80,0xe0,0x80,0xa0,0xe0,0x80,0xc0,0xe0,0x80,0xe0,0xe0,0x80,
    0x00,0x00,0xc0,0x20,0x00,0xc0,0x40,0x00,0xc0,0x60,0x00,0xc0,0x80,0x00,0xc0,0xa0,
    0x00,0xc0,0xc0,0x00,0xc0,0xe0,0x00,0xc0,0x00,0x20,0xc0,0x20,0x20,0xc0,0x40,0x20,
    0xc0,0x60,0x20,0xc0,0x80,0x20,0xc0,0xa0,0x20,0xc0,0xc0,0x20,0xc0,0xe0,0x20,0xc0,
    0x00,0x40,0xc0,0x20,0x40,0xc0,0x40,0x40,0xc0,0x60,0x40,0xc0,0x80,0x40,0xc0,0xa0,
    0x40,0xc0,0xc0,0x40,0xc0,0xe0,0x40,0xc0,0x00,0x60,0xc0,0x20,0x60,0xc0,0x40,0x60,
    0xc0,0x60,0x60,0xc0,0x80,0x60,0xc0,0xa0,0x60,0xc0,0xc0,0x60,0xc0,0xe0,0x60,0xc0,
    0x00,0x80,0xc0,0x20,0x80,0xc0,0x40,0x80,0xc0,0x60,0x80,0xc0,0x80,0x80,0xc0,0xa0,
    0x80,0xc0,0xc0,0x80,0xc0,0xe0,0x80,0xc0,0x00,0xa0,0xc0,0x20,0xa0,0xc0,0x40,0xa0,
    0xc0,0x60,0xa0,0xc0,0x80,0xa0,0xc0,0xa0,0xa0,0xc0,0xc0,0xa0,0xc0,0xe0,0xa0,0xc0,
    0x00,0xc0,0xc0,0x20,0xc0,0xc0,0x40,0xc0,0xc0,0x60,0xc0,0xc0,0x80,0xc0,0xc0,0xa0,
    0xc0,0xc0,0xff,0xfb,0xf0,0xa0,0xa0,0xa4,0x80,0x80,0x80,0xff,0x00,0x00,0x00,0xff,
    0x00,0xff,0xff,0x00,0x00,0x00,0xff,0xff,0x00,0xff,0x00,0xff,0xff,0xff,0xff,0xff,
};

static const uint8_t test_tga_color_map_32bpp[] =
{
    0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x01,0x00,0x80,0x00,0x02,0x80,0x80,0x00,0x03,
    0x00,0x00,0x80,0x04,0x80,0x00,0x80,0x05,0x00,0x80,0x80,0x06,0xc0,0xc0,0xc0,0x07,
    0xc0,0xdc,0xc0,0x08,0xa6,0xca,0xf0,0x09,0x40,0x20,0x00,0x0a,0x60,0x20,0x00,0x0b,
    0x80,0x20,0x00,0x0c,0xa0,0x20,0x00,0x0d,0xc0,0x20,0x00,0x0e,0xe0,0x20,0x00,0x0f,
    0x00,0x40,0x00,0x10,0x20,0x40,0x00,0x11,0x40,0x40,0x00,0x12,0x60,0x40,0x00,0x13,
    0x80,0x40,0x00,0x14,0xa0,0x40,0x00,0x15,0xc0,0x40,0x00,0x16,0xe0,0x40,0x00,0x17,
    0x00,0x60,0x00,0x18,0x20,0x60,0x00,0x19,0x40,0x60,0x00,0x1a,0x60,0x60,0x00,0x1b,
    0x80,0x60,0x00,0x1c,0xa0,0x60,0x00,0x1d,0xc0,0x60,0x00,0x1e,0xe0,0x60,0x00,0x1f,
    0x00,0x80,0x00,0x20,0x20,0x80,0x00,0x21,0x40,0x80,0x00,0x22,0x60,0x80,0x00,0x23,
    0x80,0x80,0x00,0x24,0xa0,0x80,0x00,0x25,0xc0,0x80,0x00,0x26,0xe0,0x80,0x00,0x27,
    0x00,0xa0,0x00,0x28,0x20,0xa0,0x00,0x29,0x40,0xa0,0x00,0x2a,0x60,0xa0,0x00,0x2b,
    0x80,0xa0,0x00,0x2c,0xa0,0xa0,0x00,0x2d,0xc0,0xa0,0x00,0x2e,0xe0,0xa0,0x00,0x2f,
    0x00,0xc0,0x00,0x30,0x20,0xc0,0x00,0x31,0x40,0xc0,0x00,0x32,0x60,0xc0,0x00,0x33,
    0x80,0xc0,0x00,0x34,0xa0,0xc0,0x00,0x35,0xc0,0xc0,0x00,0x36,0xe0,0xc0,0x00,0x37,
    0x00,0xe0,0x00,0x38,0x20,0xe0,0x00,0x39,0x40,0xe0,0x00,0x3a,0x60,0xe0,0x00,0x3b,
    0x80,0xe0,0x00,0x3c,0xa0,0xe0,0x00,0x3d,0xc0,0xe0,0x00,0x3e,0xe0,0xe0,0x00,0x3f,
    0x00,0x00,0x40,0x40,0x20,0x00,0x40,0x41,0x40,0x00,0x40,0x42,0x60,0x00,0x40,0x43,
    0x80,0x00,0x40,0x44,0xa0,0x00,0x40,0x45,0xc0,0x00,0x40,0x46,0xe0,0x00,0x40,0x47,
    0x00,0x20,0x40,0x48,0x20,0x20,0x40,0x49,0x40,0x20,0x40,0x4a,0x60,0x20,0x40,0x4b,
    0x80,0x20,0x40,0x4c,0xa0,0x20,0x40,0x4d,0xc0,0x20,0x40,0x4e,0xe0,0x20,0x40,0x4f,
    0x00,0x40,0x40,0x50,0x20,0x40,0x40,0x51,0x40,0x40,0x40,0x52,0x60,0x40,0x40,0x53,
    0x80,0x40,0x40,0x54,0xa0,0x40,0x40,0x55,0xc0,0x40,0x40,0x56,0xe0,0x40,0x40,0x57,
    0x00,0x60,0x40,0x58,0x20,0x60,0x40,0x59,0x40,0x60,0x40,0x5a,0x60,0x60,0x40,0x5b,
    0x80,0x60,0x40,0x5c,0xa0,0x60,0x40,0x5d,0xc0,0x60,0x40,0x5e,0xe0,0x60,0x40,0x5f,
    0x00,0x80,0x40,0x60,0x20,0x80,0x40,0x61,0x40,0x80,0x40,0x62,0x60,0x80,0x40,0x63,
    0x80,0x80,0x40,0x64,0xa0,0x80,0x40,0x65,0xc0,0x80,0x40,0x66,0xe0,0x80,0x40,0x67,
    0x00,0xa0,0x40,0x68,0x20,0xa0,0x40,0x69,0x40,0xa0,0x40,0x6a,0x60,0xa0,0x40,0x6b,
    0x80,0xa0,0x40,0x6c,0xa0,0xa0,0x40,0x6d,0xc0,0xa0,0x40,0x6e,0xe0,0xa0,0x40,0x6f,
    0x00,0xc0,0x40,0x70,0x20,0xc0,0x40,0x71,0x40,0xc0,0x40,0x72,0x60,0xc0,0x40,0x73,
    0x80,0xc0,0x40,0x74,0xa0,0xc0,0x40,0x75,0xc0,0xc0,0x40,0x76,0xe0,0xc0,0x40,0x77,
    0x00,0xe0,0x40,0x78,0x20,0xe0,0x40,0x79,0x40,0xe0,0x40,0x7a,0x60,0xe0,0x40,0x7b,
    0x80,0xe0,0x40,0x7c,0xa0,0xe0,0x40,0x7d,0xc0,0xe0,0x40,0x7e,0xe0,0xe0,0x40,0x7f,
    0x00,0x00,0x80,0x80,0x20,0x00,0x80,0x81,0x40,0x00,0x80,0x82,0x60,0x00,0x80,0x83,
    0x80,0x00,0x80,0x84,0xa0,0x00,0x80,0x85,0xc0,0x00,0x80,0x86,0xe0,0x00,0x80,0x87,
    0x00,0x20,0x80,0x88,0x20,0x20,0x80,0x89,0x40,0x20,0x80,0x8a,0x60,0x20,0x80,0x8b,
    0x80,0x20,0x80,0x8c,0xa0,0x20,0x80,0x8d,0xc0,0x20,0x80,0x8e,0xe0,0x20,0x80,0x8f,
    0x00,0x40,0x80,0x90,0x20,0x40,0x80,0x91,0x40,0x40,0x80,0x92,0x60,0x40,0x80,0x93,
    0x80,0x40,0x80,0x94,0xa0,0x40,0x80,0x95,0xc0,0x40,0x80,0x96,0xe0,0x40,0x80,0x97,
    0x00,0x60,0x80,0x98,0x20,0x60,0x80,0x99,0x40,0x60,0x80,0x9a,0x60,0x60,0x80,0x9b,
    0x80,0x60,0x80,0x9c,0xa0,0x60,0x80,0x9d,0xc0,0x60,0x80,0x9e,0xe0,0x60,0x80,0x9f,
    0x00,0x80,0x80,0xa0,0x20,0x80,0x80,0xa1,0x40,0x80,0x80,0xa2,0x60,0x80,0x80,0xa3,
    0x80,0x80,0x80,0xa4,0xa0,0x80,0x80,0xa5,0xc0,0x80,0x80,0xa6,0xe0,0x80,0x80,0xa7,
    0x00,0xa0,0x80,0xa8,0x20,0xa0,0x80,0xa9,0x40,0xa0,0x80,0xaa,0x60,0xa0,0x80,0xab,
    0x80,0xa0,0x80,0xac,0xa0,0xa0,0x80,0xad,0xc0,0xa0,0x80,0xae,0xe0,0xa0,0x80,0xaf,
    0x00,0xc0,0x80,0xb0,0x20,0xc0,0x80,0xb1,0x40,0xc0,0x80,0xb2,0x60,0xc0,0x80,0xb3,
    0x80,0xc0,0x80,0xb4,0xa0,0xc0,0x80,0xb5,0xc0,0xc0,0x80,0xb6,0xe0,0xc0,0x80,0xb7,
    0x00,0xe0,0x80,0xb8,0x20,0xe0,0x80,0xb9,0x40,0xe0,0x80,0xba,0x60,0xe0,0x80,0xbb,
    0x80,0xe0,0x80,0xbc,0xa0,0xe0,0x80,0xbd,0xc0,0xe0,0x80,0xbe,0xe0,0xe0,0x80,0xbf,
    0x00,0x00,0xc0,0xc0,0x20,0x00,0xc0,0xc1,0x40,0x00,0xc0,0xc2,0x60,0x00,0xc0,0xc3,
    0x80,0x00,0xc0,0xc4,0xa0,0x00,0xc0,0xc5,0xc0,0x00,0xc0,0xc6,0xe0,0x00,0xc0,0xc7,
    0x00,0x20,0xc0,0xc8,0x20,0x20,0xc0,0xc9,0x40,0x20,0xc0,0xca,0x60,0x20,0xc0,0xcb,
    0x80,0x20,0xc0,0xcc,0xa0,0x20,0xc0,0xcd,0xc0,0x20,0xc0,0xce,0xe0,0x20,0xc0,0xcf,
    0x00,0x40,0xc0,0xd0,0x20,0x40,0xc0,0xd1,0x40,0x40,0xc0,0xd2,0x60,0x40,0xc0,0xd3,
    0x80,0x40,0xc0,0xd4,0xa0,0x40,0xc0,0xd5,0xc0,0x40,0xc0,0xd6,0xe0,0x40,0xc0,0xd7,
    0x00,0x60,0xc0,0xd8,0x20,0x60,0xc0,0xd9,0x40,0x60,0xc0,0xda,0x60,0x60,0xc0,0xdb,
    0x80,0x60,0xc0,0xdc,0xa0,0x60,0xc0,0xdd,0xc0,0x60,0xc0,0xde,0xe0,0x60,0xc0,0xdf,
    0x00,0x80,0xc0,0xe0,0x20,0x80,0xc0,0xe1,0x40,0x80,0xc0,0xe2,0x60,0x80,0xc0,0xe3,
    0x80,0x80,0xc0,0xe4,0xa0,0x80,0xc0,0xe5,0xc0,0x80,0xc0,0xe6,0xe0,0x80,0xc0,0xe7,
    0x00,0xa0,0xc0,0xe8,0x20,0xa0,0xc0,0xe9,0x40,0xa0,0xc0,0xea,0x60,0xa0,0xc0,0xeb,
    0x80,0xa0,0xc0,0xec,0xa0,0xa0,0xc0,0xed,0xc0,0xa0,0xc0,0xee,0xe0,0xa0,0xc0,0xef,
    0x00,0xc0,0xc0,0xf0,0x20,0xc0,0xc0,0xf1,0x40,0xc0,0xc0,0xf2,0x60,0xc0,0xc0,0xf3,
    0x80,0xc0,0xc0,0xf4,0xa0,0xc0,0xc0,0xf5,0xff,0xfb,0xf0,0xf6,0xa0,0xa0,0xa4,0xf7,
    0x80,0x80,0x80,0xf8,0xff,0x00,0x00,0xf9,0x00,0xff,0x00,0xfa,0xff,0xff,0x00,0xfb,
    0x00,0x00,0xff,0xfc,0xff,0x00,0xff,0xfd,0x00,0xff,0xff,0xfe,0xff,0xff,0xff,0xff,
};

static const uint8_t test_tga_color_map_index_4_4[] =
{
    0x00,0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80,0x90,0xa0,0xb0,0xc0,0xd0,0xe0,0xff,
};

static const uint8_t test_tga_color_map_index_4_4_rle[] =
{
    0x83,0x10,0x83,0x20,0x03,0x80,0x90,0xa0,0xb0,0x03,0xc0,0xd0,0xe0,0xff,
};

static const uint8_t test_tga_true_color_15bpp_4_4[] =
{
    0x00,0x04,0x42,0x0c,0x84,0x14,0xc6,0x1c,0x08,0x25,0x4a,0x2d,0x8c,0x35,0xce,0x3d,
    0x10,0x46,0x52,0x4e,0x94,0x56,0xd6,0x5e,0x18,0x67,0x5a,0x6f,0x9c,0x77,0xde,0x7f,
};

static const uint8_t test_tga_true_color_15bpp_4_4_rle[] =
{
    0x03,0x00,0x04,0x42,0x0c,0x84,0x14,0xc6,0x1c,0x81,0x08,0x25,0x00,0x8c,0x35,0x00,
    0xce,0x3d,0x03,0x10,0x46,0x52,0x4e,0x94,0x56,0xd6,0x5e,0x03,0x18,0x67,0x5a,0x6f,
    0x9c,0x77,0xde,0x7f,
};

static const uint8_t test_tga_true_color_16bpp_4_4[] =
{
    0x00,0x04,0x42,0x0c,0x84,0x14,0xc6,0x1c,0x08,0x25,0x4a,0x2d,0x8c,0x35,0xce,0x3d,
    0x10,0xc6,0x52,0xce,0x94,0xd6,0xd6,0xde,0x18,0xe7,0x5a,0xef,0x9c,0xf7,0xde,0xff,
};

static const uint8_t test_tga_true_color_16bpp_4_4_rle[] =
{
    0x03,0x00,0x04,0x42,0x0c,0x84,0x14,0xc6,0x1c,0x81,0x08,0x25,0x00,0x8c,0x35,0x00,
    0xce,0x3d,0x03,0x10,0xc6,0x52,0xce,0x94,0xd6,0xd6,0xde,0x03,0x18,0xe7,0x5a,0xef,
    0x9c,0xf7,0xde,0xff,
};

static const uint8_t test_tga_true_color_24bpp_4_4[] =
{
    0x00,0x04,0x08,0x10,0x14,0x18,0x20,0x24,0x28,0x30,0x34,0x38,0x40,0x44,0x48,0x50,
    0x54,0x58,0x60,0x64,0x68,0x70,0x74,0x78,0x80,0x84,0x88,0x90,0x94,0x98,0xa0,0xa4,
    0xa8,0xb0,0xb4,0xb8,0xc0,0xc4,0xc8,0xd0,0xd4,0xd8,0xe0,0xe4,0xe8,0xf0,0xf4,0xf8,
};

static const uint8_t test_tga_true_color_24bpp_4_4_rle[] =
{
    0x03,0x00,0x04,0x08,0x10,0x14,0x18,0x20,0x24,0x28,0x30,0x34,0x38,0x81,0x40,0x44,
    0x48,0x00,0x60,0x64,0x68,0x00,0x70,0x74,0x78,0x03,0x80,0x84,0x88,0x90,0x94,0x98,
    0xa0,0xa4,0xa8,0xb0,0xb4,0xb8,0x03,0xc0,0xc4,0xc8,0xd0,0xd4,0xd8,0xe0,0xe4,0xe8,
    0xf0,0xf4,0xf8,
};

static const uint8_t test_tga_true_color_32bpp_4_4[] =
{
    0x08,0x04,0x00,0x0c,0x18,0x14,0x10,0x1c,0x28,0x24,0x20,0x2c,0x38,0x34,0x30,0x3c,
    0x48,0x44,0x40,0x4c,0x58,0x54,0x50,0x5c,0x68,0x64,0x60,0x6c,0x78,0x74,0x70,0x7c,
    0x88,0x84,0x80,0x8c,0x98,0x94,0x90,0x9c,0xa8,0xa4,0xa0,0xac,0xb8,0xb4,0xb0,0xbc,
    0xc8,0xc4,0xc0,0xcc,0xd8,0xd4,0xd0,0xdc,0xe8,0xe4,0xe0,0xec,0xf8,0xf4,0xf0,0xfc,
};

static const uint8_t test_tga_true_color_32bpp_4_4_rle[] =
{
    0x03,0x08,0x04,0x00,0x0c,0x18,0x14,0x10,0x1c,0x28,0x24,0x20,0x2c,0x38,0x34,0x30,
    0x3c,0x81,0x48,0x44,0x40,0x4c,0x00,0x68,0x64,0x60,0x6c,0x00,0x78,0x74,0x70,0x7c,
    0x03,0x88,0x84,0x80,0x8c,0x98,0x94,0x90,0x9c,0xa8,0xa4,0xa0,0xac,0xb8,0xb4,0xb0,
    0xbc,0x03,0xc8,0xc4,0xc0,0xcc,0xd8,0xd4,0xd0,0xdc,0xe8,0xe4,0xe0,0xec,0xf8,0xf4,
    0xf0,0xfc,
};

static const uint8_t test_tga_grayscale_8bpp_4_4[] =
{
    0x00,0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80,0x90,0xa0,0xb0,0xc0,0xd0,0xe0,0xf0,
};

static const uint8_t test_tga_grayscale_8bpp_4_4_rle[] =
{
    0x03,0x00,0x10,0x20,0x30,0x82,0x40,0x00,0x70,0x03,0x80,0x90,0xa0,0xb0,0x03,0xc0,
    0xd0,0xe0,0xf0,
};

/* Expected is stored as bottom to top. */
static const uint8_t test_tga_color_map_15bpp_expected[] =
{
    0x00,0x00,0x00,0xff,0x00,0x42,0x00,0xff,0x00,0x84,0x00,0xff,0x00,0xc6,0x00,0xff,
    0x42,0x00,0x00,0xff,0x42,0x42,0x00,0xff,0x42,0x84,0x00,0xff,0x42,0xc6,0x00,0xff,
    0x84,0x00,0x00,0xff,0x84,0x42,0x00,0xff,0x84,0x84,0x00,0xff,0x84,0xc6,0x00,0xff,
    0xc6,0x00,0x00,0xff,0xc6,0x42,0x00,0xff,0xc6,0x84,0x00,0xff,0xff,0xff,0xff,0xff,
};

static const uint8_t test_tga_color_map_15bpp_rle_expected[] =
{
    0x00,0x42,0x00,0xff,0x00,0x42,0x00,0xff,0x00,0x42,0x00,0xff,0x00,0x42,0x00,0xff,
    0x00,0x84,0x00,0xff,0x00,0x84,0x00,0xff,0x00,0x84,0x00,0xff,0x00,0x84,0x00,0xff,
    0x84,0x00,0x00,0xff,0x84,0x42,0x00,0xff,0x84,0x84,0x00,0xff,0x84,0xc6,0x00,0xff,
    0xc6,0x00,0x00,0xff,0xc6,0x42,0x00,0xff,0xc6,0x84,0x00,0xff,0xff,0xff,0xff,0xff,
};

static const uint8_t test_tga_color_map_16bpp_expected[] =
{
    0x00,0x00,0x00,0x00,0x00,0x42,0x00,0x00,0x00,0x84,0x00,0x00,0x00,0xc6,0x00,0x00,
    0x42,0x00,0x00,0x00,0x42,0x42,0x00,0x00,0x42,0x84,0x00,0x00,0x42,0xc6,0x00,0x00,
    0x84,0x00,0x00,0xff,0x84,0x42,0x00,0xff,0x84,0x84,0x00,0xff,0x84,0xc6,0x00,0xff,
    0xc6,0x00,0x00,0xff,0xc6,0x42,0x00,0xff,0xc6,0x84,0x00,0xff,0xff,0xff,0xff,0xff,
};

static const uint8_t test_tga_color_map_24bpp_expected[] =
{
    0x00,0x00,0x00,0xff,0x00,0x40,0x00,0xff,0x00,0x80,0x00,0xff,0x00,0xc0,0x00,0xff,
    0x40,0x00,0x00,0xff,0x40,0x40,0x00,0xff,0x40,0x80,0x00,0xff,0x40,0xc0,0x00,0xff,
    0x80,0x00,0x00,0xff,0x80,0x40,0x00,0xff,0x80,0x80,0x00,0xff,0x80,0xc0,0x00,0xff,
    0xc0,0x00,0x00,0xff,0xc0,0x40,0x00,0xff,0xc0,0x80,0x00,0xff,0xff,0xff,0xff,0xff,
};

static const uint8_t test_tga_color_map_32bpp_expected[] =
{
    0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x10,0x00,0x80,0x00,0x20,0x00,0xc0,0x00,0x30,
    0x40,0x00,0x00,0x40,0x40,0x40,0x00,0x50,0x40,0x80,0x00,0x60,0x40,0xc0,0x00,0x70,
    0x80,0x00,0x00,0x80,0x80,0x40,0x00,0x90,0x80,0x80,0x00,0xa0,0x80,0xc0,0x00,0xb0,
    0xc0,0x00,0x00,0xc0,0xc0,0x40,0x00,0xd0,0xc0,0x80,0x00,0xe0,0xff,0xff,0xff,0xff,
};

static const uint8_t test_tga_color_map_half_32bpp_expected[] =
{
    0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x10,0x00,0x80,0x00,0x20,0x00,0xc0,0x00,0x30,
    0x40,0x00,0x00,0x40,0x40,0x40,0x00,0x50,0x40,0x80,0x00,0x60,0x40,0xc0,0x00,0x70,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
};

static const uint8_t test_tga_true_color_15bpp_rle_expected[] =
{
    0x00,0x04,0x42,0x0c,0x84,0x14,0xc6,0x1c,0x08,0x25,0x08,0x25,0x8c,0x35,0xce,0x3d,
    0x10,0x46,0x52,0x4e,0x94,0x56,0xd6,0x5e,0x18,0x67,0x5a,0x6f,0x9c,0x77,0xde,0x7f,
};

static const uint8_t test_tga_true_color_16bpp_rle_expected[] =
{
    0x00,0x04,0x42,0x0c,0x84,0x14,0xc6,0x1c,0x08,0x25,0x08,0x25,0x8c,0x35,0xce,0x3d,
    0x10,0xc6,0x52,0xce,0x94,0xd6,0xd6,0xde,0x18,0xe7,0x5a,0xef,0x9c,0xf7,0xde,0xff,
};

static const uint8_t test_tga_true_color_24bpp_rle_expected[] =
{
    0x00,0x04,0x08,0x10,0x14,0x18,0x20,0x24,0x28,0x30,0x34,0x38,0x40,0x44,0x48,0x40,
    0x44,0x48,0x60,0x64,0x68,0x70,0x74,0x78,0x80,0x84,0x88,0x90,0x94,0x98,0xa0,0xa4,
    0xa8,0xb0,0xb4,0xb8,0xc0,0xc4,0xc8,0xd0,0xd4,0xd8,0xe0,0xe4,0xe8,0xf0,0xf4,0xf8,
};

static const uint8_t test_tga_true_color_32bpp_rle_expected[] =
{
    0x08,0x04,0x00,0x0c,0x18,0x14,0x10,0x1c,0x28,0x24,0x20,0x2c,0x38,0x34,0x30,0x3c,
    0x48,0x44,0x40,0x4c,0x48,0x44,0x40,0x4c,0x68,0x64,0x60,0x6c,0x78,0x74,0x70,0x7c,
    0x88,0x84,0x80,0x8c,0x98,0x94,0x90,0x9c,0xa8,0xa4,0xa0,0xac,0xb8,0xb4,0xb0,0xbc,
    0xc8,0xc4,0xc0,0xcc,0xd8,0xd4,0xd0,0xdc,0xe8,0xe4,0xe0,0xec,0xf8,0xf4,0xf0,0xfc,
};

static const uint8_t test_tga_grayscale_8bpp_rle_expected[] =
{
    0x00,0x10,0x20,0x30,0x40,0x40,0x40,0x70,0x80,0x90,0xa0,0xb0,0xc0,0xd0,0xe0,0xf0,
};

#define check_tga_surface_load(surface, expected, right_to_left, bottom_to_top) \
    check_tga_surface_load_(__LINE__, surface, expected, right_to_left, bottom_to_top)
static void check_tga_surface_load_(uint32_t line, IDirect3DSurface9 *surface, const uint8_t *expected, BOOL right_to_left,
        BOOL bottom_to_top)
{
    uint32_t x, y, fmt_bpp, fmt_pitch;
    D3DLOCKED_RECT lock_rect;
    D3DSURFACE_DESC desc;
    HRESULT hr;

    hr = IDirect3DSurface9_GetDesc(surface, &desc);
    ok_(__FILE__, line)(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    fmt_bpp = get_bpp_for_d3dformat(desc.Format);
    fmt_pitch = fmt_bpp * desc.Width;
    IDirect3DSurface9_LockRect(surface, &lock_rect, NULL, D3DLOCK_READONLY);
    for (y = 0; y < desc.Height; ++y)
    {
        const uint32_t expected_row_idx = bottom_to_top ? (desc.Height - y - 1) : y;
        const uint8_t *row = ((uint8_t *)lock_rect.pBits) + (lock_rect.Pitch * y);
        const uint8_t *expected_row = expected + (fmt_pitch * expected_row_idx);

        for (x = 0; x < desc.Width; ++x)
        {
            const uint32_t expected_pixel_idx = right_to_left ? (desc.Width - x - 1) : x;
            const uint8_t *expected_pixel = expected_row + (fmt_bpp * expected_pixel_idx);
            const uint8_t *pixel = row + (fmt_bpp * x);
            BOOL pixel_match = !memcmp(pixel, expected_pixel, fmt_bpp);

            ok_(__FILE__, line)(pixel_match, "Pixel mismatch at (%u,%u).\n", x, y);
        }
    }
    IDirect3DSurface9_UnlockRect(surface);
}

static void test_load_surface_from_tga(IDirect3DDevice9 *device)
{
    static const struct
    {
        struct tga_header header;
        const uint8_t *color_map;
        uint32_t color_map_size;
        const uint8_t *pixels;
        uint32_t pixels_size;

        const uint8_t *expected;
    } tga_tests[] =
    {
        { { 0, COLORMAP_TYPE_ONE, IMAGETYPE_COLORMAPPED, 0, 256, 15, 0, 0, 4, 4, 8, 0 },
          test_tga_color_map_15bpp, sizeof(test_tga_color_map_15bpp),
          test_tga_color_map_index_4_4, sizeof(test_tga_color_map_index_4_4),
          test_tga_color_map_15bpp_expected,
        },
        { { 0, COLORMAP_TYPE_ONE, IMAGETYPE_COLORMAPPED | IMAGETYPE_RLE, 0, 256, 15, 0, 0, 4, 4, 8, 0 },
          test_tga_color_map_15bpp, sizeof(test_tga_color_map_15bpp),
          test_tga_color_map_index_4_4_rle, sizeof(test_tga_color_map_index_4_4_rle),
          test_tga_color_map_15bpp_rle_expected
        },
        { { 0, COLORMAP_TYPE_ONE, IMAGETYPE_COLORMAPPED, 0, 256, 16, 0, 0, 4, 4, 8, 0 },
          test_tga_color_map_16bpp, sizeof(test_tga_color_map_16bpp),
          test_tga_color_map_index_4_4, sizeof(test_tga_color_map_index_4_4),
          test_tga_color_map_16bpp_expected
        },
        { { 0, COLORMAP_TYPE_ONE, IMAGETYPE_COLORMAPPED, 0, 256, 24, 0, 0, 4, 4, 8, 0 },
          test_tga_color_map_24bpp, sizeof(test_tga_color_map_24bpp),
          test_tga_color_map_index_4_4, sizeof(test_tga_color_map_index_4_4),
          test_tga_color_map_24bpp_expected
        },
        { { 0, COLORMAP_TYPE_ONE, IMAGETYPE_COLORMAPPED, 0, 256, 32, 0, 0, 4, 4, 8, 0 },
          test_tga_color_map_32bpp, sizeof(test_tga_color_map_32bpp),
          test_tga_color_map_index_4_4, sizeof(test_tga_color_map_index_4_4),
          test_tga_color_map_32bpp_expected
        },
        { { 0, COLORMAP_TYPE_ONE, IMAGETYPE_COLORMAPPED, 0, 128, 32, 0, 0, 4, 4, 8, 0 },
          test_tga_color_map_32bpp, sizeof(test_tga_color_map_32bpp) / 2,
          test_tga_color_map_index_4_4, sizeof(test_tga_color_map_index_4_4),
          test_tga_color_map_half_32bpp_expected
        },
        { { 0, COLORMAP_TYPE_NONE, IMAGETYPE_TRUECOLOR, 0, 0, 0, 0, 0, 4, 4, 15, 0 },
          NULL, 0,
          test_tga_true_color_15bpp_4_4, sizeof(test_tga_true_color_15bpp_4_4),
          test_tga_true_color_15bpp_4_4
        },
        { { 0, COLORMAP_TYPE_NONE, IMAGETYPE_TRUECOLOR | IMAGETYPE_RLE, 0, 0, 0, 0, 0, 4, 4, 15, 0 },
          NULL, 0,
          test_tga_true_color_15bpp_4_4_rle, sizeof(test_tga_true_color_15bpp_4_4_rle),
          test_tga_true_color_15bpp_rle_expected
        },
        { { 0, COLORMAP_TYPE_NONE, IMAGETYPE_TRUECOLOR, 0, 0, 0, 0, 0, 4, 4, 16, 0 },
          NULL, 0,
          test_tga_true_color_16bpp_4_4, sizeof(test_tga_true_color_16bpp_4_4),
          test_tga_true_color_16bpp_4_4
        },
        { { 0, COLORMAP_TYPE_NONE, IMAGETYPE_TRUECOLOR | IMAGETYPE_RLE, 0, 0, 0, 0, 0, 4, 4, 16, 0 },
          NULL, 0,
          test_tga_true_color_16bpp_4_4_rle, sizeof(test_tga_true_color_16bpp_4_4_rle),
          test_tga_true_color_16bpp_rle_expected
        },
        { { 0, COLORMAP_TYPE_NONE, IMAGETYPE_TRUECOLOR, 0, 0, 0, 0, 0, 4, 4, 24, 0 },
          NULL, 0,
          test_tga_true_color_24bpp_4_4, sizeof(test_tga_true_color_24bpp_4_4),
          test_tga_true_color_24bpp_4_4,
        },
        { { 0, COLORMAP_TYPE_NONE, IMAGETYPE_TRUECOLOR | IMAGETYPE_RLE, 0, 0, 0, 0, 0, 4, 4, 24, 0 },
          NULL, 0,
          test_tga_true_color_24bpp_4_4_rle, sizeof(test_tga_true_color_24bpp_4_4_rle),
          test_tga_true_color_24bpp_rle_expected
        },
        { { 0, COLORMAP_TYPE_NONE, IMAGETYPE_TRUECOLOR, 0, 0, 0, 0, 0, 4, 4, 32, 0 },
          NULL, 0,
          test_tga_true_color_32bpp_4_4, sizeof(test_tga_true_color_32bpp_4_4),
          test_tga_true_color_32bpp_4_4,
        },
        { { 0, COLORMAP_TYPE_NONE, IMAGETYPE_TRUECOLOR | IMAGETYPE_RLE, 0, 0, 0, 0, 0, 4, 4, 32, 0 },
          NULL, 0,
          test_tga_true_color_32bpp_4_4_rle, sizeof(test_tga_true_color_32bpp_4_4_rle),
          test_tga_true_color_32bpp_rle_expected
        },
        { { 0, COLORMAP_TYPE_NONE, IMAGETYPE_GRAYSCALE, 0, 0, 0, 0, 0, 4, 4, 8, 0 },
          NULL, 0,
          test_tga_grayscale_8bpp_4_4, sizeof(test_tga_grayscale_8bpp_4_4),
          test_tga_grayscale_8bpp_4_4
        },
        { { 0, COLORMAP_TYPE_NONE, IMAGETYPE_GRAYSCALE | IMAGETYPE_RLE, 0, 0, 0, 0, 0, 4, 4, 8, 0 },
          NULL, 0,
          test_tga_grayscale_8bpp_4_4_rle, sizeof(test_tga_grayscale_8bpp_4_4_rle),
          test_tga_grayscale_8bpp_rle_expected
        },
    };
    static const uint8_t rle_test_bits[] = { 15, 16, 24, 32 };
    struct
    {
        struct tga_header header;
        uint8_t data[4096 * 1024];
    } *tga;
    IDirect3DSurface9 *surface;
    unsigned int i;
    HRESULT hr;

    if (!(tga = calloc(1, sizeof(*tga))))
    {
        skip("Failed to allocate memory.\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(tga_tests); ++i)
    {
        const uint32_t file_size = sizeof(tga->header) + tga_tests[i].color_map_size + tga_tests[i].pixels_size;
        D3DFORMAT surface_fmt;
        D3DXIMAGE_INFO info;

        winetest_push_context("Test %u", i);

        tga->header = tga_tests[i].header;
        if (tga_tests[i].color_map)
            memcpy(tga->data, tga_tests[i].color_map, tga_tests[i].color_map_size);
        memcpy(tga->data + tga_tests[i].color_map_size, tga_tests[i].pixels, tga_tests[i].pixels_size);

        hr = D3DXGetImageInfoFromFileInMemory(tga, file_size, &info);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
        surface_fmt = (info.Format == D3DFMT_P8) ? D3DFMT_A8B8G8R8 : info.Format;

        hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, info.Width, info.Height, surface_fmt, D3DPOOL_SCRATCH,
                &surface, NULL);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

        /*
         * Unlike D3DXGetImageInfo*() which only validates header size, the
         * size of the image data is also validated when loading into a surface.
         */
        hr = D3DXGetImageInfoFromFileInMemory(tga, file_size - 1, &info);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

        hr = D3DXLoadSurfaceFromFileInMemory(surface, NULL, NULL, tga, file_size - 1, NULL, D3DX_FILTER_NONE, 0, NULL);
        ok(hr == D3DXERR_INVALIDDATA, "Unexpected hr %#lx.\n", hr);

        /* Read as default, bottom to top, left to right. */
        hr = D3DXLoadSurfaceFromFileInMemory(surface, NULL, NULL, tga, file_size, NULL, D3DX_FILTER_NONE, 0, NULL);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
        check_tga_surface_load(surface, tga_tests[i].expected, FALSE, TRUE);

        /* Read as top to bottom, left to right. */
        tga->header.image_descriptor = IMAGE_TOPTOBOTTOM;
        hr = D3DXLoadSurfaceFromFileInMemory(surface, NULL, NULL, tga, file_size, NULL, D3DX_FILTER_NONE, 0, NULL);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
        check_tga_surface_load(surface, tga_tests[i].expected, FALSE, FALSE);

        /* Read as bottom to top, right to left. */
        tga->header.image_descriptor = IMAGE_RIGHTTOLEFT;
        hr = D3DXLoadSurfaceFromFileInMemory(surface, NULL, NULL, tga, file_size, NULL, D3DX_FILTER_NONE, 0, NULL);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
        check_tga_surface_load(surface, tga_tests[i].expected, TRUE, TRUE);

        /* Read as top to bottom, right to left. */
        tga->header.image_descriptor = IMAGE_TOPTOBOTTOM | IMAGE_RIGHTTOLEFT;
        hr = D3DXLoadSurfaceFromFileInMemory(surface, NULL, NULL, tga, file_size, NULL, D3DX_FILTER_NONE, 0, NULL);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
        check_tga_surface_load(surface, tga_tests[i].expected, TRUE, FALSE);

        check_release((IUnknown *)surface, 0);
        winetest_pop_context();
    }

    /*
     * Test RLE behavior. RLE packets cannot cross row boundaries.
     */
    for (i = 0; i < ARRAY_SIZE(rle_test_bits); ++i)
    {
        const struct tga_header rle_hdr = { 0, COLORMAP_TYPE_NONE, IMAGETYPE_TRUECOLOR | IMAGETYPE_RLE, 0, 0, 0, 0, 0,
                                            4, 2, rle_test_bits[i], IMAGE_TOPTOBOTTOM };
        const uint32_t packet_size = ((rle_test_bits[i] + 7) / 8) + 1;
        uint32_t file_size = sizeof(tga->header);
        D3DFORMAT surface_fmt;
        D3DXIMAGE_INFO info;

        winetest_push_context("Test %u", i);

        memset(tga->data, 0, sizeof(tga->data));
        tga->header = rle_hdr;

        hr = D3DXGetImageInfoFromFileInMemory(tga, file_size, &info);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
        surface_fmt = info.Format;

        hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, info.Width, info.Height, surface_fmt, D3DPOOL_SCRATCH,
                &surface, NULL);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

        /*
         * This packet encodes 8 pixels, but the image has a width of 4. This
         * should fail.
         */
        tga->data[0] = 0x87;
        file_size += packet_size;
        hr = D3DXLoadSurfaceFromFileInMemory(surface, NULL, NULL, tga, file_size, NULL, D3DX_FILTER_NONE, 0, NULL);
        ok(hr == D3DXERR_INVALIDDATA, "Unexpected hr %#lx.\n", hr);

        /* Two packets, each containing 4 pixels. This succeeds. */
        tga->data[0] = 0x83;
        tga->data[packet_size] = 0x83;
        file_size += packet_size;

        hr = D3DXLoadSurfaceFromFileInMemory(surface, NULL, NULL, tga, file_size, NULL, D3DX_FILTER_NONE, 0, NULL);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

        /* Second packet with only 2 pixels, doesn't finish the final row. */
        tga->data[packet_size] = 0x82;
        hr = D3DXLoadSurfaceFromFileInMemory(surface, NULL, NULL, tga, file_size, NULL, D3DX_FILTER_NONE, 0, NULL);
        ok(hr == D3DXERR_INVALIDDATA, "Unexpected hr %#lx.\n", hr);

        check_release((IUnknown *)surface, 0);
        winetest_pop_context();
    }

    free(tga);
}

static void test_D3DXLoadSurface(IDirect3DDevice9 *device)
{
    HRESULT hr;
    BOOL testdummy_ok, testbitmap_ok;
    IDirect3DTexture9 *tex;
    IDirect3DSurface9 *surf, *newsurf;
    RECT rect, destrect;
    D3DLOCKED_RECT lockrect;
    static const WORD pixdata_a8r3g3b2[] = { 0x57df, 0x98fc, 0xacdd, 0xc891 };
    static const WORD pixdata_a1r5g5b5[] = { 0x46b5, 0x99c8, 0x06a2, 0x9431 };
    static const WORD pixdata_r5g6b5[] = { 0x9ef6, 0x658d, 0x0aee, 0x42ee };
    static const WORD pixdata_a8l8[] = { 0xff00, 0x00ff, 0xff30, 0x7f7f };
    static const DWORD pixdata_g16r16[] = { 0x07d23fbe, 0xdc7f44a4, 0xe4d8976b, 0x9a84fe89 };
    static const DWORD pixdata_a8b8g8r8[] = { 0xc3394cf0, 0x235ae892, 0x09b197fd, 0x8dc32bf6 };
    static const DWORD pixdata_a2r10g10b10[] = { 0x57395aff, 0x5b7668fd, 0xb0d856b5, 0xff2c61d6 };
    static const uint32_t pixdata_a8r8g8b8[] = { 0x00102030, 0x40506070, 0x8090a0b0, 0xc0d0e0ff };
    static const uint32_t pixdata_a8b8g8r8_2[] = { 0x30201000, 0x70605040, 0xb0a09080, 0xffe0d0c0 };
    static const uint32_t pixdata_x8l8v8u8[] = { 0x00003000, 0x00557f40, 0x00aa8180, 0x00ffffc0 };
    static const uint32_t pixdata_a2w10v10u10[] = { 0x0ba17400, 0x5ff5d117, 0xae880600, 0xfffe8b45 };
    static const uint32_t pixdata_q8w8v8u8[] = { 0x30201000, 0x7f605040, 0xb0a08180, 0xffe0d0c0 };
    static const float pixdata_a32b32g32r32f[] = {  0.0f,  0.1f,  NAN,  INFINITY,  1.0f,  1.1f,  1.2f,  1.3f,
                                                   -0.1f, -0.2f, -NAN, -INFINITY, -1.0f, -1.1f, -1.2f, -1.3f };
    static const uint16_t pixdata_v8u8[] = { 0x3000, 0x7f40, 0x8180, 0xffc0 };
    BYTE buffer[4 * 8 * 4];
    uint32_t i;

    hr = create_file("testdummy.bmp", noimage, sizeof(noimage));  /* invalid image */
    testdummy_ok = SUCCEEDED(hr);

    hr = create_file("testbitmap.bmp", bmp_1bpp, sizeof(bmp_1bpp));  /* valid image */
    testbitmap_ok = SUCCEEDED(hr);

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 256, 256, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &surf, NULL);
    if(FAILED(hr)) {
        skip("Failed to create a surface (%#lx)\n", hr);
        if(testdummy_ok) DeleteFileA("testdummy.bmp");
        if(testbitmap_ok) DeleteFileA("testbitmap.bmp");
        return;
    }

    /* D3DXLoadSurfaceFromFile */
    if(testbitmap_ok) {
        hr = D3DXLoadSurfaceFromFileA(surf, NULL, NULL, "testbitmap.bmp", NULL, D3DX_DEFAULT, 0, NULL);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromFile returned %#lx, expected %#lx\n", hr, D3D_OK);

        hr = D3DXLoadSurfaceFromFileA(NULL, NULL, NULL, "testbitmap.bmp", NULL, D3DX_DEFAULT, 0, NULL);
        ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromFile returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);
    } else skip("Couldn't create \"testbitmap.bmp\"\n");

    if(testdummy_ok) {
        hr = D3DXLoadSurfaceFromFileA(surf, NULL, NULL, "testdummy.bmp", NULL, D3DX_DEFAULT, 0, NULL);
        ok(hr == D3DXERR_INVALIDDATA, "D3DXLoadSurfaceFromFile returned %#lx, expected %#x\n", hr, D3DXERR_INVALIDDATA);
    } else skip("Couldn't create \"testdummy.bmp\"\n");

    hr = D3DXLoadSurfaceFromFileA(surf, NULL, NULL, NULL, NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromFile returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXLoadSurfaceFromFileA(surf, NULL, NULL, "", NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXLoadSurfaceFromFile returned %#lx, expected %#x\n", hr, D3DXERR_INVALIDDATA);


    /* D3DXLoadSurfaceFromResource */
    hr = D3DXLoadSurfaceFromResourceA(surf, NULL, NULL, NULL,
            MAKEINTRESOURCEA(IDB_BITMAP_1x1), NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3D_OK, "D3DXLoadSurfaceFromResource returned %#lx, expected %#lx\n", hr, D3D_OK);

    hr = D3DXLoadSurfaceFromResourceA(surf, NULL, NULL, NULL,
            MAKEINTRESOURCEA(IDD_BITMAPDATA_1x1), NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3D_OK, "D3DXLoadSurfaceFromResource returned %#lx, expected %#lx\n", hr, D3D_OK);

    hr = D3DXLoadSurfaceFromResourceA(surf, NULL, NULL, NULL, NULL, NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXLoadSurfaceFromResource returned %#lx, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXLoadSurfaceFromResourceA(NULL, NULL, NULL, NULL,
            MAKEINTRESOURCEA(IDB_BITMAP_1x1), NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromResource returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXLoadSurfaceFromResourceA(surf, NULL, NULL, NULL,
            MAKEINTRESOURCEA(IDS_STRING), NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXLoadSurfaceFromResource returned %#lx, expected %#x\n", hr, D3DXERR_INVALIDDATA);


    /* D3DXLoadSurfaceFromFileInMemory */
    hr = D3DXLoadSurfaceFromFileInMemory(surf, NULL, NULL, bmp_1bpp, sizeof(bmp_1bpp), NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3D_OK, "D3DXLoadSurfaceFromFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);

    hr = D3DXLoadSurfaceFromFileInMemory(surf, NULL, NULL, noimage, sizeof(noimage), NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXLoadSurfaceFromFileInMemory returned %#lx, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXLoadSurfaceFromFileInMemory(surf, NULL, NULL, bmp_1bpp, 0, NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromFileInMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXLoadSurfaceFromFileInMemory(NULL, NULL, NULL, bmp_1bpp, sizeof(bmp_1bpp), NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromFileInMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXLoadSurfaceFromFileInMemory(surf, NULL, NULL, NULL, 8, NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromFileInMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXLoadSurfaceFromFileInMemory(surf, NULL, NULL, NULL, 0, NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromFileInMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXLoadSurfaceFromFileInMemory(NULL, NULL, NULL, NULL, 0, NULL, D3DX_DEFAULT, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromFileInMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);


    /* D3DXLoadSurfaceFromMemory */
    SetRect(&rect, 0, 0, 2, 2);

    hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata, D3DFMT_A8R8G8B8, sizeof(pixdata), NULL, &rect, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "D3DXLoadSurfaceFromMemory returned %#lx, expected %#lx\n", hr, D3D_OK);

    hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata, D3DFMT_A8R8G8B8, 0, NULL, &rect, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "D3DXLoadSurfaceFromMemory returned %#lx, expected %#lx\n", hr, D3D_OK);

    hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, NULL, D3DFMT_A8R8G8B8, sizeof(pixdata), NULL, &rect, D3DX_DEFAULT, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXLoadSurfaceFromMemory(NULL, NULL, NULL, pixdata, D3DFMT_A8R8G8B8, sizeof(pixdata), NULL, &rect, D3DX_DEFAULT, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata, D3DFMT_A8R8G8B8, sizeof(pixdata), NULL, NULL, D3DX_DEFAULT, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata, D3DFMT_UNKNOWN, sizeof(pixdata), NULL, &rect, D3DX_DEFAULT, 0);
    ok(hr == E_FAIL, "D3DXLoadSurfaceFromMemory returned %#lx, expected %#lx\n", hr, E_FAIL);

    SetRect(&destrect, -1, -1, 1, 1); /* destination rect is partially outside texture boundaries */
    hr = D3DXLoadSurfaceFromMemory(surf, NULL, &destrect, pixdata, D3DFMT_A8R8G8B8, sizeof(pixdata), NULL, &rect, D3DX_FILTER_NONE, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    SetRect(&destrect, 255, 255, 257, 257); /* destination rect is partially outside texture boundaries */
    hr = D3DXLoadSurfaceFromMemory(surf, NULL, &destrect, pixdata, D3DFMT_A8R8G8B8, sizeof(pixdata), NULL, &rect, D3DX_FILTER_NONE, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    SetRect(&destrect, 1, 1, 0, 0); /* left > right, top > bottom */
    hr = D3DXLoadSurfaceFromMemory(surf, NULL, &destrect, pixdata, D3DFMT_A8R8G8B8, sizeof(pixdata), NULL, &rect, D3DX_FILTER_NONE, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    SetRect(&destrect, 1, 2, 1, 2); /* left = right, top = bottom */
    hr = D3DXLoadSurfaceFromMemory(surf, NULL, &destrect, pixdata, D3DFMT_A8R8G8B8, sizeof(pixdata), NULL, &rect, D3DX_FILTER_NONE, 0);
    /* fails when debug version of d3d9 is used */
    ok(hr == D3D_OK || broken(hr == D3DERR_INVALIDCALL), "D3DXLoadSurfaceFromMemory returned %#lx, expected %#lx\n", hr, D3D_OK);

    SetRect(&destrect, 257, 257, 257, 257); /* left = right, top = bottom, but invalid values */
    hr = D3DXLoadSurfaceFromMemory(surf, NULL, &destrect, pixdata, D3DFMT_A8R8G8B8, sizeof(pixdata), NULL, &rect, D3DX_FILTER_NONE, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);


    /* D3DXLoadSurfaceFromSurface */
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 256, 256, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &newsurf, NULL);
    if(SUCCEEDED(hr)) {
        hr = D3DXLoadSurfaceFromSurface(newsurf, NULL, NULL, surf, NULL, NULL, D3DX_DEFAULT, 0);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromSurface returned %#lx, expected %#lx\n", hr, D3D_OK);

        hr = D3DXLoadSurfaceFromSurface(NULL, NULL, NULL, surf, NULL, NULL, D3DX_DEFAULT, 0);
        ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromSurface returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

        hr = D3DXLoadSurfaceFromSurface(newsurf, NULL, NULL, NULL, NULL, NULL, D3DX_DEFAULT, 0);
        ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromSurface returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

        check_release((IUnknown*)newsurf, 0);
    } else skip("Failed to create a second surface\n");

    hr = IDirect3DDevice9_CreateTexture(device, 256, 256, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex, NULL);
    if (SUCCEEDED(hr))
    {
        IDirect3DTexture9_GetSurfaceLevel(tex, 0, &newsurf);

        hr = D3DXLoadSurfaceFromSurface(newsurf, NULL, NULL, surf, NULL, NULL, D3DX_DEFAULT, 0);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromSurface returned %#lx, expected %#lx\n", hr, D3D_OK);

        IDirect3DSurface9_Release(newsurf);
        IDirect3DTexture9_Release(tex);
    } else skip("Failed to create texture\n");

    /* non-lockable render target */
    hr = IDirect3DDevice9_CreateRenderTarget(device, 256, 256, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, FALSE, &newsurf, NULL);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, NULL, newsurf, NULL, NULL, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
    IDirect3DSurface9_Release(newsurf);

    /* non-lockable multisampled render target */
    hr = IDirect3DDevice9_CreateRenderTarget(device, 256, 256, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_2_SAMPLES, 0, FALSE, &newsurf, NULL);
    if (SUCCEEDED(hr))
    {
       struct surface_readback surface_rb;

       /* D3DXLoadSurfaceFromMemory should return success with a multisampled render target. */
       SetRect(&rect, 0, 0, 2, 2);
       hr = D3DXLoadSurfaceFromMemory(newsurf, NULL, &rect, pixdata_a8r8g8b8, D3DFMT_A8R8G8B8, 8, NULL, &rect, D3DX_FILTER_NONE, 0);
       ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

       /* The call succeeds, but the surface isn't actually written to. */
       get_surface_readback(device, newsurf, D3DFMT_A8R8G8B8, &surface_rb);
       check_readback_pixel_4bpp(&surface_rb, 0, 0, 0x00000000, FALSE);
       check_readback_pixel_4bpp(&surface_rb, 1, 0, 0x00000000, FALSE);
       check_readback_pixel_4bpp(&surface_rb, 0, 1, 0x00000000, FALSE);
       check_readback_pixel_4bpp(&surface_rb, 1, 1, 0x00000000, FALSE);
       release_surface_readback(&surface_rb);

       /*
        * Load the data into our non-multisampled render target, then load
        * that into the multisampled render target.
        */
       SetRect(&rect, 0, 0, 2, 2);
       hr = D3DXLoadSurfaceFromMemory(surf, NULL, &rect, pixdata_a8r8g8b8, D3DFMT_A8R8G8B8, 8, NULL, &rect, D3DX_FILTER_NONE, 0);
       ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

       get_surface_readback(device, surf, D3DFMT_A8R8G8B8, &surface_rb);
       check_readback_pixel_4bpp(&surface_rb, 0, 0, pixdata_a8r8g8b8[0], FALSE);
       check_readback_pixel_4bpp(&surface_rb, 1, 0, pixdata_a8r8g8b8[1], FALSE);
       check_readback_pixel_4bpp(&surface_rb, 0, 1, pixdata_a8r8g8b8[2], FALSE);
       check_readback_pixel_4bpp(&surface_rb, 1, 1, pixdata_a8r8g8b8[3], FALSE);
       release_surface_readback(&surface_rb);

       /*
        * Loading from a non-multisampled surface into a multisampled surface
        * does change the surface contents.
        */
       hr = D3DXLoadSurfaceFromSurface(newsurf, NULL, &rect, surf, NULL, &rect, D3DX_FILTER_NONE, 0);
       ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

       get_surface_readback(device, newsurf, D3DFMT_A8R8G8B8, &surface_rb);
       check_readback_pixel_4bpp(&surface_rb, 0, 0, pixdata_a8r8g8b8[0], TRUE);
       check_readback_pixel_4bpp(&surface_rb, 1, 0, pixdata_a8r8g8b8[1], TRUE);
       check_readback_pixel_4bpp(&surface_rb, 0, 1, pixdata_a8r8g8b8[2], TRUE);
       check_readback_pixel_4bpp(&surface_rb, 1, 1, pixdata_a8r8g8b8[3], TRUE);
       release_surface_readback(&surface_rb);

       /* Contents of the multisampled surface are preserved. */
       hr = D3DXLoadSurfaceFromMemory(newsurf, NULL, NULL, pixdata, D3DFMT_A8R8G8B8, 8, NULL, &rect, D3DX_FILTER_POINT, 0);
       ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

       get_surface_readback(device, newsurf, D3DFMT_A8R8G8B8, &surface_rb);
       check_readback_pixel_4bpp(&surface_rb, 0, 0, pixdata_a8r8g8b8[0], TRUE);
       check_readback_pixel_4bpp(&surface_rb, 1, 0, pixdata_a8r8g8b8[1], TRUE);
       check_readback_pixel_4bpp(&surface_rb, 0, 1, pixdata_a8r8g8b8[2], TRUE);
       check_readback_pixel_4bpp(&surface_rb, 1, 1, pixdata_a8r8g8b8[3], TRUE);
       release_surface_readback(&surface_rb);

       IDirect3DSurface9_Release(newsurf);
    }
    else
    {
        skip("Failed to create multisampled render target.\n");
    }

    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &newsurf);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetRenderTarget returned %#lx, expected %#lx.\n", hr, D3D_OK);

    hr = D3DXLoadSurfaceFromSurface(newsurf, NULL, NULL, surf, NULL, NULL, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "D3DXLoadSurfaceFromSurface returned %#lx, expected %#lx.\n", hr, D3D_OK);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, NULL, newsurf, NULL, NULL, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "D3DXLoadSurfaceFromSurface returned %#lx, expected %#lx.\n", hr, D3D_OK);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, NULL, newsurf, NULL, NULL, D3DX_FILTER_NONE, 0xff000000);
    ok(hr == D3D_OK, "D3DXLoadSurfaceFromSurface returned %#lx, expected %#lx.\n", hr, D3D_OK);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, NULL, newsurf, NULL, NULL, D3DX_FILTER_TRIANGLE | D3DX_FILTER_MIRROR, 0);
    ok(hr == D3D_OK, "D3DXLoadSurfaceFromSurface returned %#lx, expected %#lx.\n", hr, D3D_OK);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, NULL, newsurf, NULL, NULL, D3DX_FILTER_LINEAR, 0);
    ok(hr == D3D_OK, "D3DXLoadSurfaceFromSurface returned %#lx, expected %#lx.\n", hr, D3D_OK);

    /* rects */
    SetRect(&rect, 2, 2, 1, 1);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, NULL, newsurf, NULL, &rect, D3DX_FILTER_NONE, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromSurface returned %#lx, expected %#lx.\n", hr, D3DERR_INVALIDCALL);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, NULL, newsurf, NULL, &rect, D3DX_DEFAULT, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromSurface returned %#lx, expected %#lx.\n", hr, D3DERR_INVALIDCALL);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, &rect, newsurf, NULL, NULL, D3DX_FILTER_NONE, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromSurface returned %#lx, expected %#lx.\n", hr, D3DERR_INVALIDCALL);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, &rect, newsurf, NULL, NULL, D3DX_DEFAULT, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromSurface returned %#lx, expected %#lx.\n", hr, D3DERR_INVALIDCALL);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, &rect, newsurf, NULL, NULL, D3DX_FILTER_POINT, 0);
    ok(hr == D3DERR_INVALIDCALL, "D3DXLoadSurfaceFromSurface returned %#lx, expected %#lx.\n", hr, D3DERR_INVALIDCALL);
    SetRect(&rect, 1, 1, 1, 1);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, NULL, newsurf, NULL, &rect, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "D3DXLoadSurfaceFromSurface returned %#lx, expected %#lx.\n", hr, D3D_OK);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, &rect, newsurf, NULL, NULL, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "D3DXLoadSurfaceFromSurface returned %#lx, expected %#lx.\n", hr, D3D_OK);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, NULL, newsurf, NULL, &rect, D3DX_DEFAULT, 0);
    ok(hr == E_FAIL, "D3DXLoadSurfaceFromSurface returned %#lx, expected %#lx.\n", hr, E_FAIL);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, &rect, newsurf, NULL, NULL, D3DX_DEFAULT, 0);
    ok(hr == E_FAIL, "D3DXLoadSurfaceFromSurface returned %#lx, expected %#lx.\n", hr, E_FAIL);
    if (0)
    {
        /* Somehow it crashes with a STATUS_INTEGER_DIVIDE_BY_ZERO exception
         * on Windows. */
        hr = D3DXLoadSurfaceFromSurface(surf, NULL, &rect, newsurf, NULL, NULL, D3DX_FILTER_POINT, 0);
        ok(hr == E_FAIL, "D3DXLoadSurfaceFromSurface returned %#lx, expected %#lx.\n", hr, E_FAIL);
    }
    SetRect(&rect, 1, 1, 2, 2);
    SetRect(&destrect, 1, 1, 2, 2);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, &rect, newsurf, NULL, &destrect, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "D3DXLoadSurfaceFromSurface returned %#lx, expected %#lx.\n", hr, D3D_OK);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, &rect, newsurf, NULL, NULL, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "D3DXLoadSurfaceFromSurface returned %#lx, expected %#lx.\n", hr, D3D_OK);
    hr = D3DXLoadSurfaceFromSurface(surf, NULL, NULL, newsurf, NULL, &destrect, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "D3DXLoadSurfaceFromSurface returned %#lx, expected %#lx.\n", hr, D3D_OK);

    for (i = 0; i < ARRAY_SIZE(test_filter_values); ++i)
    {
        winetest_push_context("Filter %d (%#x)", i, test_filter_values[i].filter);

        hr = D3DXLoadSurfaceFromSurface(surf, NULL, NULL, newsurf, NULL, NULL, test_filter_values[i].filter, 0);
        ok(hr == test_filter_values[i].expected_hr, "Unexpected hr %#lx.\n", hr);

        winetest_pop_context();
    }

    IDirect3DSurface9_Release(newsurf);

    check_release((IUnknown*)surf, 0);

    SetRect(&rect, 1, 1, 2, 2);
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 1, 1, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &surf, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a8b8g8r8,
            D3DFMT_A8R8G8B8, 8, NULL, &rect, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
    check_pixel_4bpp(&lockrect, 0, 0, 0x8dc32bf6);
    IDirect3DSurface9_UnlockRect(surf);

    /*
     * Test negative offsets in the source rectangle. Causes an access
     * violation when run on 64-bit Windows.
     */
    if (sizeof(void *) != 8)
    {
        SetRect(&rect, 0, -1, 1, 0);
        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, &pixdata_a8b8g8r8[2],
                D3DFMT_A8R8G8B8, 8, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
        IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        check_pixel_4bpp(&lockrect, 0, 0, pixdata_a8b8g8r8[0]);
        IDirect3DSurface9_UnlockRect(surf);
    }
    else
        skip("Skipping test for negative source rectangle values on 64-bit.\n");

    check_release((IUnknown *)surf, 0);

    /* test color conversion */
    SetRect(&rect, 0, 0, 2, 2);
    /* A8R8G8B8 */
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 2, 2, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &surf, NULL);
    if(FAILED(hr)) skip("Failed to create a surface (%#lx)\n", hr);
    else {
        PALETTEENTRY palette;

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a8r3g3b2,
                D3DFMT_A8R3G3B2, 4, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromMemory returned %#lx, expected %#lx\n", hr, D3D_OK);
        IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        check_pixel_4bpp(&lockrect, 0, 0, 0x57dbffff);
        check_pixel_4bpp(&lockrect, 1, 0, 0x98ffff00);
        check_pixel_4bpp(&lockrect, 0, 1, 0xacdbff55);
        check_pixel_4bpp(&lockrect, 1, 1, 0xc8929255);
        IDirect3DSurface9_UnlockRect(surf);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a1r5g5b5,
                D3DFMT_A1R5G5B5, 4, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromMemory returned %#lx, expected %#lx\n", hr, D3D_OK);
        IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        check_pixel_4bpp(&lockrect, 0, 0, 0x008cadad);
        check_pixel_4bpp(&lockrect, 1, 0, 0xff317342);
        check_pixel_4bpp(&lockrect, 0, 1, 0x0008ad10);
        check_pixel_4bpp(&lockrect, 1, 1, 0xff29088c);
        IDirect3DSurface9_UnlockRect(surf);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_r5g6b5,
                D3DFMT_R5G6B5, 4, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromMemory returned %#lx, expected %#lx\n", hr, D3D_OK);
        IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        check_pixel_4bpp(&lockrect, 0, 0, 0xff9cdfb5);
        check_pixel_4bpp(&lockrect, 1, 0, 0xff63b26b);
        check_pixel_4bpp(&lockrect, 0, 1, 0xff085d73);
        check_pixel_4bpp(&lockrect, 1, 1, 0xff425d73);
        IDirect3DSurface9_UnlockRect(surf);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_g16r16,
                D3DFMT_G16R16, 8, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromMemory returned %#lx, expected %#lx\n", hr, D3D_OK);
        IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        todo_wine {
            check_pixel_4bpp(&lockrect, 0, 0, 0xff3f08ff);
        }
        check_pixel_4bpp(&lockrect, 1, 0, 0xff44dcff);
        check_pixel_4bpp(&lockrect, 0, 1, 0xff97e4ff);
        check_pixel_4bpp(&lockrect, 1, 1, 0xfffe9aff);
        IDirect3DSurface9_UnlockRect(surf);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a8b8g8r8,
                D3DFMT_A8B8G8R8, 8, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
        check_pixel_4bpp(&lockrect, 0, 0, 0xc3f04c39);
        check_pixel_4bpp(&lockrect, 1, 0, 0x2392e85a);
        check_pixel_4bpp(&lockrect, 0, 1, 0x09fd97b1);
        check_pixel_4bpp(&lockrect, 1, 1, 0x8df62bc3);
        IDirect3DSurface9_UnlockRect(surf);

        SetRect(&rect, 0, 0, 1, 1);
        SetRect(&destrect, 1, 1, 2, 2);
        hr = D3DXLoadSurfaceFromMemory(surf, NULL, &destrect, pixdata_a8b8g8r8,
                D3DFMT_A8B8G8R8, 8, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
        IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        check_pixel_4bpp(&lockrect, 0, 0, 0xc3f04c39);
        check_pixel_4bpp(&lockrect, 1, 0, 0x2392e85a);
        check_pixel_4bpp(&lockrect, 0, 1, 0x09fd97b1);
        check_pixel_4bpp(&lockrect, 1, 1, 0xc3f04c39);
        IDirect3DSurface9_UnlockRect(surf);

        SetRect(&rect, 0, 0, 2, 2);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a2r10g10b10,
                D3DFMT_A2R10G10B10, 8, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
        IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        check_pixel_4bpp(&lockrect, 0, 0, 0x555c95bf);
        check_pixel_4bpp(&lockrect, 1, 0, 0x556d663f);
        check_pixel_4bpp(&lockrect, 0, 1, 0xaac385ad);
        todo_wine {
            check_pixel_4bpp(&lockrect, 1, 1, 0xfffcc575);
        }
        IDirect3DSurface9_UnlockRect(surf);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a8l8,
                D3DFMT_A8L8, 4, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(SUCCEEDED(hr), "Failed to load surface, hr %#lx.\n", hr);
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#lx.\n", hr);
        check_pixel_4bpp(&lockrect, 0, 0, 0xff000000);
        check_pixel_4bpp(&lockrect, 1, 0, 0x00ffffff);
        check_pixel_4bpp(&lockrect, 0, 1, 0xff303030);
        check_pixel_4bpp(&lockrect, 1, 1, 0x7f7f7f7f);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#lx.\n", hr);

        /* From a signed normalized format to an unsigned normalized format. */
        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_q8w8v8u8, D3DFMT_Q8W8V8U8, 8, NULL, &rect,
                D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "Failed to lock surface, hr %#lx.\n", hr);
        check_pixel_4bpp(&lockrect, 0, 0, 0xb08090a0);
        check_pixel_4bpp(&lockrect, 1, 0, 0xffc0d0e0);
        check_pixel_4bpp(&lockrect, 0, 1, 0x2f00001f);
        check_pixel_4bpp(&lockrect, 1, 1, 0x7e3f4f5f);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(hr == D3D_OK, "Failed to unlock surface, hr %#lx.\n", hr);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a32b32g32r32f, D3DFMT_A32B32G32R32F, 32, NULL, &rect,
                D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "Failed to lock surface, hr %#lx.\n", hr);
        check_pixel_4bpp(&lockrect, 0, 0, 0xff001aff);
        check_pixel_4bpp(&lockrect, 1, 0, 0xffffffff);
        check_pixel_4bpp(&lockrect, 0, 1, 0x000000ff);
        check_pixel_4bpp(&lockrect, 1, 1, 0x00000000);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(hr == D3D_OK, "Failed to unlock surface, hr %#lx.\n", hr);

        /* Test D3DXLoadSurfaceFromMemory with indexed color image */
        if (0)
        {
        /* Crashes on Nvidia Win10. */
        palette.peRed   = bmp_1bpp[56];
        palette.peGreen = bmp_1bpp[55];
        palette.peBlue  = bmp_1bpp[54];
        palette.peFlags = bmp_1bpp[57]; /* peFlags is the alpha component in DX8 and higher */
        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, &bmp_1bpp[62],
                D3DFMT_P8, 1, (const PALETTEENTRY *)&palette, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#lx\n", hr);
        ok(*(DWORD*)lockrect.pBits == 0x80f3f2f1,
                "Pixel color mismatch: got %#lx, expected 0x80f3f2f1\n", *(DWORD*)lockrect.pBits);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#lx\n", hr);
        }

        /* Test D3DXLoadSurfaceFromFileInMemory with indexed color image (alpha is not taken into account for bmp file) */
        hr = D3DXLoadSurfaceFromFileInMemory(surf, NULL, NULL, bmp_1bpp, sizeof(bmp_1bpp), NULL, D3DX_FILTER_NONE, 0, NULL);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#lx\n", hr);
        ok(*(DWORD*)lockrect.pBits == 0xfff3f2f1, "Pixel color mismatch: got %#lx, expected 0xfff3f2f1\n", *(DWORD*)lockrect.pBits);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#lx\n", hr);

        SetRect(&rect, 0, 0, 2, 2);
        for (i = 0; i < ARRAY_SIZE(test_filter_values); ++i)
        {
            winetest_push_context("Filter %d (%#x)", i, test_filter_values[i].filter);

            hr = D3DXLoadSurfaceFromFileInMemory(surf, NULL, NULL, dds_24bit, sizeof(dds_24bit), NULL,
                    test_filter_values[i].filter, 0, NULL);
            ok(hr == test_filter_values[i].expected_hr, "Unexpected hr %#lx.\n", hr);

            hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a8b8g8r8, D3DFMT_A8B8G8R8, 8, NULL, &rect,
                    test_filter_values[i].filter, 0);
            ok(hr == test_filter_values[i].expected_hr, "Unexpected hr %#lx.\n", hr);

            winetest_pop_context();
        }

        /* Test D3DXLoadSurfaceFromFileInMemory with indexed pixel format DDS files. */
        hr = D3DXLoadSurfaceFromFileInMemory(surf, NULL, NULL, dds_8bit, sizeof(dds_8bit), &rect, D3DX_FILTER_NONE, 0, NULL);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "Failed to lock surface, hr %#lx.\n", hr);
        check_pixel_4bpp(&lockrect, 0, 0, 0xffec2700);
        check_pixel_4bpp(&lockrect, 1, 0, 0xffec2700);
        check_pixel_4bpp(&lockrect, 0, 1, 0xffec2700);
        check_pixel_4bpp(&lockrect, 1, 1, 0xffec2700);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(hr == D3D_OK, "Failed to unlock surface, hr %#lx.\n", hr);

        hr = D3DXLoadSurfaceFromFileInMemory(surf, NULL, NULL, dds_a8p8, sizeof(dds_a8p8), &rect, D3DX_FILTER_NONE, 0, NULL);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "Failed to lock surface, hr %#lx.\n", hr);
        check_pixel_4bpp(&lockrect, 0, 0, 0xf0000000);
        check_pixel_4bpp(&lockrect, 1, 0, 0xe0004000);
        check_pixel_4bpp(&lockrect, 0, 1, 0xb0400000);
        check_pixel_4bpp(&lockrect, 1, 1, 0xa0404000);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(hr == D3D_OK, "Failed to unlock surface, hr %#lx.\n", hr);

        check_release((IUnknown*)surf, 0);
    }

    /* A1R5G5B5 */
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 2, 2, D3DFMT_A1R5G5B5, D3DPOOL_DEFAULT, &surf, NULL);
    if(FAILED(hr)) skip("Failed to create a surface (%#lx)\n", hr);
    else {
        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a8r3g3b2,
                D3DFMT_A8R3G3B2, 4, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromMemory returned %#lx, expected %#lx\n", hr, D3D_OK);
        IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        check_pixel_2bpp(&lockrect, 0, 0, 0x6fff);
        check_pixel_2bpp(&lockrect, 1, 0, 0xffe0);
        check_pixel_2bpp(&lockrect, 0, 1, 0xefea);
        check_pixel_2bpp(&lockrect, 1, 1, 0xca4a);
        IDirect3DSurface9_UnlockRect(surf);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a1r5g5b5,
                D3DFMT_A1R5G5B5, 4, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromMemory returned %#lx, expected %#lx\n", hr, D3D_OK);
        IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        check_pixel_2bpp(&lockrect, 0, 0, 0x46b5);
        check_pixel_2bpp(&lockrect, 1, 0, 0x99c8);
        check_pixel_2bpp(&lockrect, 0, 1, 0x06a2);
        check_pixel_2bpp(&lockrect, 1, 1, 0x9431);
        IDirect3DSurface9_UnlockRect(surf);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_r5g6b5,
                D3DFMT_R5G6B5, 4, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromMemory returned %#lx, expected %#lx\n", hr, D3D_OK);
        IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        check_pixel_2bpp(&lockrect, 0, 0, 0xcf76);
        check_pixel_2bpp(&lockrect, 1, 0, 0xb2cd);
        check_pixel_2bpp(&lockrect, 0, 1, 0x856e);
        check_pixel_2bpp(&lockrect, 1, 1, 0xa16e);
        IDirect3DSurface9_UnlockRect(surf);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_g16r16,
                D3DFMT_G16R16, 8, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromMemory returned %#lx, expected %#lx\n", hr, D3D_OK);
        IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        todo_wine {
            check_pixel_2bpp(&lockrect, 0, 0, 0xa03f);
        }
        check_pixel_2bpp(&lockrect, 1, 0, 0xa37f);
        check_pixel_2bpp(&lockrect, 0, 1, 0xcb9f);
        check_pixel_2bpp(&lockrect, 1, 1, 0xfe7f);
        IDirect3DSurface9_UnlockRect(surf);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a8b8g8r8,
                D3DFMT_A8B8G8R8, 8, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromMemory returned %#lx, expected %#lx\n", hr, D3D_OK);
        IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        todo_wine {
            check_pixel_2bpp(&lockrect, 0, 0, 0xf527);
            check_pixel_2bpp(&lockrect, 1, 0, 0x4b8b);
        }
        check_pixel_2bpp(&lockrect, 0, 1, 0x7e56);
        check_pixel_2bpp(&lockrect, 1, 1, 0xf8b8);
        IDirect3DSurface9_UnlockRect(surf);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a2r10g10b10,
                D3DFMT_A2R10G10B10, 8, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromMemory returned %#lx, expected %#lx\n", hr, D3D_OK);
        IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        check_pixel_2bpp(&lockrect, 0, 0, 0x2e57);
        todo_wine {
            check_pixel_2bpp(&lockrect, 1, 0, 0x3588);
        }
        check_pixel_2bpp(&lockrect, 0, 1, 0xe215);
        check_pixel_2bpp(&lockrect, 1, 1, 0xff0e);
        IDirect3DSurface9_UnlockRect(surf);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a8l8,
                D3DFMT_A8L8, 4, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(SUCCEEDED(hr), "Failed to load surface, hr %#lx.\n", hr);
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#lx.\n", hr);
        check_pixel_2bpp(&lockrect, 0, 0, 0x8000);
        check_pixel_2bpp(&lockrect, 1, 0, 0x7fff);
        check_pixel_2bpp(&lockrect, 0, 1, 0x98c6);
        check_pixel_2bpp(&lockrect, 1, 1, 0x3def);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#lx.\n", hr);

        check_release((IUnknown*)surf, 0);
    }

    /* A8L8 */
    hr = IDirect3DDevice9_CreateTexture(device, 2, 2, 1, 0, D3DFMT_A8L8, D3DPOOL_MANAGED, &tex, NULL);
    if (FAILED(hr))
        skip("Failed to create A8L8 texture, hr %#lx.\n", hr);
    else
    {
        hr = IDirect3DTexture9_GetSurfaceLevel(tex, 0, &surf);
        ok(SUCCEEDED(hr), "Failed to get the surface, hr %#lx.\n", hr);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a8r3g3b2,
                D3DFMT_A8R3G3B2, 4, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(SUCCEEDED(hr), "Failed to load surface, hr %#lx.\n", hr);
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#lx.\n", hr);
        check_pixel_2bpp(&lockrect, 0, 0, 0x57f7);
        check_pixel_2bpp(&lockrect, 1, 0, 0x98ed);
        check_pixel_2bpp(&lockrect, 0, 1, 0xaceb);
        check_pixel_2bpp(&lockrect, 1, 1, 0xc88d);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#lx.\n", hr);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a1r5g5b5,
                D3DFMT_A1R5G5B5, 4, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "D3DXLoadSurfaceFromMemory returned %#lx, expected %#lx\n", hr, D3D_OK);
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#lx.\n", hr);
        check_pixel_2bpp(&lockrect, 0, 0, 0x00a6);
        check_pixel_2bpp(&lockrect, 1, 0, 0xff62);
        check_pixel_2bpp(&lockrect, 0, 1, 0x007f);
        check_pixel_2bpp(&lockrect, 1, 1, 0xff19);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#lx.\n", hr);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_r5g6b5,
                D3DFMT_R5G6B5, 4, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(SUCCEEDED(hr), "Failed to load surface, hr %#lx.\n", hr);
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#lx.\n", hr);
        check_pixel_2bpp(&lockrect, 0, 0, 0xffce);
        check_pixel_2bpp(&lockrect, 1, 0, 0xff9c);
        check_pixel_2bpp(&lockrect, 0, 1, 0xff4d);
        check_pixel_2bpp(&lockrect, 1, 1, 0xff59);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#lx.\n", hr);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_g16r16,
                D3DFMT_G16R16, 8, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(SUCCEEDED(hr), "Failed to load surface, hr %#lx.\n", hr);
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#lx.\n", hr);
        check_pixel_2bpp(&lockrect, 0, 0, 0xff25);
        check_pixel_2bpp(&lockrect, 1, 0, 0xffbe);
        check_pixel_2bpp(&lockrect, 0, 1, 0xffd6);
        check_pixel_2bpp(&lockrect, 1, 1, 0xffb6);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#lx.\n", hr);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a8b8g8r8,
                D3DFMT_A8B8G8R8, 8, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(SUCCEEDED(hr), "Failed to load surface, hr %#lx.\n", hr);
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#lx.\n", hr);
        check_pixel_2bpp(&lockrect, 0, 0, 0xc36d);
        check_pixel_2bpp(&lockrect, 1, 0, 0x23cb);
        check_pixel_2bpp(&lockrect, 0, 1, 0x09af);
        check_pixel_2bpp(&lockrect, 1, 1, 0x8d61);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#lx.\n", hr);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a2r10g10b10,
                D3DFMT_A2R10G10B10, 8, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(SUCCEEDED(hr), "Failed to load surface, hr %#lx.\n", hr);
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#lx.\n", hr);
        check_pixel_2bpp(&lockrect, 0, 0, 0x558c);
        check_pixel_2bpp(&lockrect, 1, 0, 0x5565);
        check_pixel_2bpp(&lockrect, 0, 1, 0xaa95);
        check_pixel_2bpp(&lockrect, 1, 1, 0xffcb);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#lx.\n", hr);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a8l8,
                D3DFMT_A8L8, 4, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(SUCCEEDED(hr), "Failed to load surface, hr %#lx.\n", hr);
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#lx.\n", hr);
        check_pixel_2bpp(&lockrect, 0, 0, 0xff00);
        check_pixel_2bpp(&lockrect, 1, 0, 0x00ff);
        check_pixel_2bpp(&lockrect, 0, 1, 0xff30);
        check_pixel_2bpp(&lockrect, 1, 1, 0x7f7f);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#lx.\n", hr);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_q8w8v8u8, D3DFMT_Q8W8V8U8, 8, NULL, &rect,
                D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "Failed to lock surface, hr %#lx.\n", hr);
        check_pixel_2bpp(&lockrect, 0, 0, 0xb08d);
        check_pixel_2bpp(&lockrect, 1, 0, 0xffce);
        check_pixel_2bpp(&lockrect, 0, 1, 0x2f02);
        check_pixel_2bpp(&lockrect, 1, 1, 0x7e4d);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(hr == D3D_OK, "Failed to unlock surface, hr %#lx.\n", hr);

        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a32b32g32r32f, D3DFMT_A32B32G32R32F, 32, NULL, &rect,
                D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "Failed to lock surface, hr %#lx.\n", hr);
        todo_wine check_pixel_2bpp(&lockrect, 0, 0, 0xff25);
        check_pixel_2bpp(&lockrect, 1, 0, 0xffff);
        todo_wine check_pixel_2bpp(&lockrect, 0, 1, 0x0012);
        check_pixel_2bpp(&lockrect, 1, 1, 0x0000);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(hr == D3D_OK, "Failed to unlock surface, hr %#lx.\n", hr);

        check_release((IUnknown*)surf, 1);
        check_release((IUnknown*)tex, 0);
    }

    /* DXT1, DXT2, DXT3, DXT4, DXT5 */
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 4, 4, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &surf, NULL);
    if (FAILED(hr))
        skip("Failed to create A8R8G8B8 surface, hr %#lx.\n", hr);
    else
    {
        hr = D3DXLoadSurfaceFromFileInMemory(surf, NULL, NULL, dds_24bit, sizeof(dds_24bit), NULL, D3DX_FILTER_NONE, 0, NULL);
        ok(SUCCEEDED(hr), "Failed to load surface, hr %#lx.\n", hr);

        hr = IDirect3DDevice9_CreateTexture(device, 4, 4, 1, 0, D3DFMT_DXT2, D3DPOOL_SYSTEMMEM, &tex, NULL);
        if (FAILED(hr))
            skip("Failed to create DXT2 texture, hr %#lx.\n", hr);
        else
        {
            hr = IDirect3DTexture9_GetSurfaceLevel(tex, 0, &newsurf);
            ok(SUCCEEDED(hr), "Failed to get the surface, hr %#lx.\n", hr);
            hr = D3DXLoadSurfaceFromSurface(newsurf, NULL, NULL, surf, NULL, NULL, D3DX_FILTER_NONE, 0);
            ok(SUCCEEDED(hr), "Failed to convert pixels to DXT2 format.\n");
            check_release((IUnknown*)newsurf, 1);
            check_release((IUnknown*)tex, 0);
        }

        hr = IDirect3DDevice9_CreateTexture(device, 4, 4, 1, 0, D3DFMT_DXT3, D3DPOOL_SYSTEMMEM, &tex, NULL);
        if (FAILED(hr))
            skip("Failed to create DXT3 texture, hr %#lx.\n", hr);
        else
        {
            hr = IDirect3DTexture9_GetSurfaceLevel(tex, 0, &newsurf);
            ok(SUCCEEDED(hr), "Failed to get the surface, hr %#lx.\n", hr);
            hr = D3DXLoadSurfaceFromSurface(newsurf, NULL, NULL, surf, NULL, NULL, D3DX_FILTER_NONE, 0);
            ok(SUCCEEDED(hr), "Failed to convert pixels to DXT3 format.\n");
            check_release((IUnknown*)newsurf, 1);
            check_release((IUnknown*)tex, 0);
        }

        hr = IDirect3DDevice9_CreateTexture(device, 4, 4, 1, 0, D3DFMT_DXT4, D3DPOOL_SYSTEMMEM, &tex, NULL);
        if (FAILED(hr))
            skip("Failed to create DXT4 texture, hr %#lx.\n", hr);
        else
        {
            hr = IDirect3DTexture9_GetSurfaceLevel(tex, 0, &newsurf);
            ok(SUCCEEDED(hr), "Failed to get the surface, hr %#lx.\n", hr);
            hr = D3DXLoadSurfaceFromSurface(newsurf, NULL, NULL, surf, NULL, NULL, D3DX_FILTER_NONE, 0);
            ok(SUCCEEDED(hr), "Failed to convert pixels to DXT4 format.\n");
            check_release((IUnknown*)newsurf, 1);
            check_release((IUnknown*)tex, 0);
        }

        hr = IDirect3DDevice9_CreateTexture(device, 4, 4, 1, 0, D3DFMT_DXT5, D3DPOOL_SYSTEMMEM, &tex, NULL);
        if (FAILED(hr))
            skip("Failed to create DXT5 texture, hr %#lx.\n", hr);
        else
        {
            struct surface_readback surface_rb;

            hr = IDirect3DTexture9_GetSurfaceLevel(tex, 0, &newsurf);
            ok(SUCCEEDED(hr), "Failed to get the surface, hr %#lx.\n", hr);
            hr = D3DXLoadSurfaceFromSurface(newsurf, NULL, NULL, surf, NULL, NULL, D3DX_FILTER_NONE, 0);
            ok(SUCCEEDED(hr), "Failed to convert pixels to DXT5 format.\n");

            SetRect(&rect, 0, 0, 4, 2);
            hr = D3DXLoadSurfaceFromSurface(newsurf, NULL, &rect, surf, NULL, &rect, D3DX_FILTER_NONE, 0);
            ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
            hr = D3DXLoadSurfaceFromMemory(newsurf, NULL, &rect, &dds_dxt5[128],
                    D3DFMT_DXT5, 16, NULL, &rect, D3DX_FILTER_NONE, 0);
            ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
            check_release((IUnknown *)newsurf, 1);
            check_release((IUnknown *)tex, 0);

            /* Test updating subarea of compressed texture. */
            hr = IDirect3DDevice9_CreateTexture(device, 32, 16, 1, 0, D3DFMT_DXT5, D3DPOOL_SYSTEMMEM, &tex, NULL);
            ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
            hr = IDirect3DTexture9_GetSurfaceLevel(tex, 0, &newsurf);
            ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
            SetRect(&destrect, 0, 0, 4, 8);
            SetRect(&rect, 0, 0, 4, 8);
            memset(buffer, 0x40, sizeof(buffer));
            hr = D3DXLoadSurfaceFromMemory(newsurf, NULL, &destrect, buffer,
                    D3DFMT_A8B8G8R8, 4 * 4, NULL, &rect, D3DX_FILTER_NONE, 0);
            ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

            hr = IDirect3DSurface9_LockRect(newsurf, &lockrect, &destrect, D3DLOCK_READONLY);
            ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
            /* 2 identical 16 bytes DXT5 blocks. The exact values in blocks may differ from Windows due to
             * different compression algorithms. */
            ok(!memcmp(lockrect.pBits, (char *)lockrect.pBits + lockrect.Pitch, 16), "data mismatch.\n");
            hr = IDirect3DSurface9_UnlockRect(newsurf);
            ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

            check_release((IUnknown *)newsurf, 1);
            check_release((IUnknown *)tex, 0);

            /* Test a rect larger than but not an integer multiple of the block size. */
            hr = IDirect3DDevice9_CreateTexture(device, 4, 8, 1, 0, D3DFMT_DXT5, D3DPOOL_SYSTEMMEM, &tex, NULL);
            ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
            hr = IDirect3DTexture9_GetSurfaceLevel(tex, 0, &newsurf);
            ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
            SetRect(&rect, 0, 0, 4, 6);
            hr = D3DXLoadSurfaceFromMemory(newsurf, NULL, &rect, &dds_dxt5[112],
                    D3DFMT_DXT5, 16, NULL, &rect, D3DX_FILTER_POINT, 0);
            ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

            check_release((IUnknown *)newsurf, 1);
            check_release((IUnknown *)tex, 0);

            /* More misalignment tests. */
            hr = IDirect3DDevice9_CreateTexture(device, 8, 8, 1, 0, D3DFMT_DXT5, D3DPOOL_SYSTEMMEM, &tex, NULL);
            ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
            hr = IDirect3DTexture9_GetSurfaceLevel(tex, 0, &newsurf);
            ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

            SetRect(&rect, 2, 2, 6, 6);
            hr = D3DXLoadSurfaceFromMemory(newsurf, NULL, NULL, &dds_dxt5_8_8[128],
                    D3DFMT_DXT5, 16 * 2, NULL, &rect, D3DX_FILTER_POINT, 0);
            ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

            hr = D3DXLoadSurfaceFromMemory(newsurf, NULL, &rect, &dds_dxt5_8_8[128],
                    D3DFMT_DXT5, 16 * 2, NULL, NULL, D3DX_FILTER_POINT, 0);
            ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

            hr = D3DXLoadSurfaceFromMemory(newsurf, NULL, &rect, &dds_dxt5_8_8[128],
                    D3DFMT_DXT5, 16 * 2, NULL, &rect, D3DX_FILTER_POINT, 0);
            ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

            check_release((IUnknown *)newsurf, 1);
            check_release((IUnknown *)tex, 0);

            /* Misalignment tests but check the resulting image. */
            hr = IDirect3DDevice9_CreateTexture(device, 8, 8, 1, 0, D3DFMT_DXT5, D3DPOOL_SYSTEMMEM, &tex, NULL);
            ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
            hr = IDirect3DTexture9_GetSurfaceLevel(tex, 0, &newsurf);
            ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

            SetRect(&rect, 0, 0, 8, 8);
            hr = D3DXLoadSurfaceFromMemory(newsurf, NULL, NULL, dxt5_8_8,
                    D3DFMT_DXT5, 16 * 2, NULL, &rect, D3DX_FILTER_NONE, 0);
            ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

            get_surface_decompressed_readback(device, newsurf, &surface_rb);

            check_readback_pixel_4bpp(&surface_rb, 0, 0, 0xff0000ff, FALSE); /* Blue block, top left. */
            check_readback_pixel_4bpp(&surface_rb, 3, 3, 0xff0000ff, FALSE); /* Blue block, bottom right. */
            check_readback_pixel_4bpp(&surface_rb, 7, 0, 0xff00ff00, FALSE); /* Green block, top right. */
            check_readback_pixel_4bpp(&surface_rb, 4, 3, 0xff00ff00, FALSE); /* Green block, bottom left. */
            check_readback_pixel_4bpp(&surface_rb, 3, 4, 0xffff0000, FALSE); /* Red block, top right. */
            check_readback_pixel_4bpp(&surface_rb, 0, 7, 0xffff0000, FALSE); /* Red block, bottom left. */
            check_readback_pixel_4bpp(&surface_rb, 4, 4, 0xff000000, FALSE); /* Black block, top left. */
            check_readback_pixel_4bpp(&surface_rb, 7, 7, 0xff000000, FALSE); /* Black block, bottom right. */

            release_surface_readback(&surface_rb);

            /*
             * Load our surface into a destination rectangle that overlaps
             * multiple blocks. Original data in the blocks should be
             * preserved.
             */
            SetRect(&rect, 4, 4, 8, 8);
            SetRect(&destrect, 2, 2, 6, 6);
            hr = D3DXLoadSurfaceFromMemory(newsurf, NULL, &destrect, dxt5_8_8,
                    D3DFMT_DXT5, 16 * 2, NULL, &rect, D3DX_FILTER_NONE, 0);
            ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

            get_surface_decompressed_readback(device, newsurf, &surface_rb);

            check_readback_pixel_4bpp(&surface_rb, 0, 0, 0xff0000ff, FALSE); /* Blue block, top left. */
            check_readback_pixel_4bpp(&surface_rb, 3, 3, 0xff000000, FALSE); /* Blue block, bottom right. */
            check_readback_pixel_4bpp(&surface_rb, 7, 0, 0xff00ff00, FALSE); /* Green block, top right. */
            check_readback_pixel_4bpp(&surface_rb, 4, 3, 0xff000000, FALSE); /* Green block, bottom left. */
            check_readback_pixel_4bpp(&surface_rb, 3, 4, 0xff000000, FALSE); /* Red block, top right. */
            check_readback_pixel_4bpp(&surface_rb, 0, 7, 0xffff0000, FALSE); /* Red block, bottom left. */
            check_readback_pixel_4bpp(&surface_rb, 4, 4, 0xff000000, FALSE); /* Black block, top left. */
            check_readback_pixel_4bpp(&surface_rb, 7, 7, 0xff000000, FALSE); /* Black block, bottom right. */

            release_surface_readback(&surface_rb);

            /*
             * Our source and destination rectangles start on aligned
             * boundaries, but the size is not the entire block.
             */
            SetRect(&rect, 4, 0, 6, 2);
            SetRect(&destrect, 4, 0, 6, 2);
            hr = D3DXLoadSurfaceFromMemory(newsurf, NULL, &destrect, dxt5_8_8,
                    D3DFMT_DXT5, 16 * 2, NULL, &rect, D3DX_FILTER_NONE, 0);
            ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

            get_surface_decompressed_readback(device, newsurf, &surface_rb);

            check_readback_pixel_4bpp(&surface_rb, 4, 0, 0xff00ff00, FALSE); /* Green block, top left. */
            /*
             * Bottom left of green block, should still be black from prior
             * operation.
             */
            check_readback_pixel_4bpp(&surface_rb, 4, 3, 0xff000000, FALSE);

            release_surface_readback(&surface_rb);

            check_release((IUnknown *)newsurf, 1);
            check_release((IUnknown *)tex, 0);
        }

        hr = IDirect3DDevice9_CreateTexture(device, 4, 4, 1, 0, D3DFMT_DXT1, D3DPOOL_SYSTEMMEM, &tex, NULL);
        if (FAILED(hr))
            skip("Failed to create DXT1 texture, hr %#lx.\n", hr);
        else
        {
            hr = IDirect3DTexture9_GetSurfaceLevel(tex, 0, &newsurf);
            ok(SUCCEEDED(hr), "Failed to get the surface, hr %#lx.\n", hr);
            hr = D3DXLoadSurfaceFromSurface(newsurf, NULL, NULL, surf, NULL, NULL, D3DX_FILTER_NONE, 0);
            ok(SUCCEEDED(hr), "Failed to convert pixels to DXT1 format.\n");

            hr = D3DXLoadSurfaceFromSurface(surf, NULL, NULL, newsurf, NULL, NULL, D3DX_FILTER_NONE, 0);
            ok(SUCCEEDED(hr), "Failed to convert pixels from DXT1 format.\n");

            check_release((IUnknown*)newsurf, 1);
            check_release((IUnknown*)tex, 0);
        }

        check_release((IUnknown*)surf, 0);
    }

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 2, 2, D3DFMT_A32B32G32R32F, D3DPOOL_DEFAULT, &surf, NULL);
    if (FAILED(hr))
        skip("Failed to create a D3DFMT_A32B32G32R32F surface, hr %#lx.\n", hr);
    else
    {
        /* Direct copy. */
        SetRect(&rect, 0, 0, 2, 2);
        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a32b32g32r32f, D3DFMT_A32B32G32R32F, 32, NULL, &rect,
                D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "Failed to lock surface, hr %#lx.\n", hr);
        check_pixel_float4(&lockrect, 0, 0, pixdata_a32b32g32r32f[0], pixdata_a32b32g32r32f[1],
                pixdata_a32b32g32r32f[2], pixdata_a32b32g32r32f[3], 0, FALSE);
        check_pixel_float4(&lockrect, 1, 0, pixdata_a32b32g32r32f[4], pixdata_a32b32g32r32f[5],
                pixdata_a32b32g32r32f[6], pixdata_a32b32g32r32f[7], 0, FALSE);
        check_pixel_float4(&lockrect, 0, 1, pixdata_a32b32g32r32f[8], pixdata_a32b32g32r32f[9],
                pixdata_a32b32g32r32f[10], pixdata_a32b32g32r32f[11], 0, FALSE);
        check_pixel_float4(&lockrect, 1, 1, pixdata_a32b32g32r32f[12], pixdata_a32b32g32r32f[13],
                pixdata_a32b32g32r32f[14], pixdata_a32b32g32r32f[15], 0, FALSE);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(hr == D3D_OK, "Failed to unlock surface, hr %#lx.\n", hr);

        /* Signed normalized value to full range float. */
        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_q8w8v8u8, D3DFMT_Q8W8V8U8, 8, NULL, &rect,
                D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "Failed to lock surface, hr %#lx.\n", hr);
        check_pixel_float4(&lockrect, 0, 0,  0.00000000e+000f,  1.25984251e-001f,  2.51968503e-001f,  3.77952754e-001f, 0, FALSE);
        check_pixel_float4(&lockrect, 1, 0,  5.03937006e-001f,  6.29921257e-001f,  7.55905509e-001f,  1.00000000e+000f, 0, FALSE);
        check_pixel_float4(&lockrect, 0, 1, -1.00000000e+000f, -1.00000000e+000f, -7.55905509e-001f, -6.29921257e-001f, 0, FALSE);
        check_pixel_float4(&lockrect, 1, 1, -5.03937006e-001f, -3.77952754e-001f, -2.51968503e-001f, -7.87401572e-003f, 0, FALSE);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(hr == D3D_OK, "Failed to unlock surface, hr %#lx.\n", hr);

        /* Unsigned normalized value to full range float. */
        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a8b8g8r8_2, D3DFMT_A8B8G8R8, 8, NULL, &rect,
                D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "Failed to lock surface, hr %#lx.\n", hr);
        check_pixel_float4(&lockrect, 0, 0, 0.00000000e+000f, 6.27451017e-002f, 1.25490203e-001f, 1.88235313e-001f, 1, FALSE);
        check_pixel_float4(&lockrect, 1, 0, 2.50980407e-001f, 3.13725501e-001f, 3.76470625e-001f, 4.39215720e-001f, 1, FALSE);
        check_pixel_float4(&lockrect, 0, 1, 5.01960814e-001f, 5.64705908e-001f, 6.27451003e-001f, 6.90196097e-001f, 0, FALSE);
        check_pixel_float4(&lockrect, 1, 1, 7.52941251e-001f, 8.15686345e-001f, 8.78431439e-001f, 1.00000000e+000f, 1, FALSE);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(hr == D3D_OK, "Failed to unlock surface, hr %#lx.\n", hr);

        /* V8U8 snorm. */
        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_v8u8, D3DFMT_V8U8, 4, NULL, &rect,
                D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "Failed to lock surface, hr %#lx.\n", hr);
        check_pixel_float4(&lockrect, 0, 0,  0.00000000e+000f,  3.77952754e-001f, 1.0f, 1.0f, 0, FALSE);
        check_pixel_float4(&lockrect, 1, 0,  0.503937f,         1.0f,             1.0f, 1.0f, 0, FALSE);
        check_pixel_float4(&lockrect, 0, 1, -1.0f,             -1.0f,             1.0f, 1.0f, 0, FALSE);
        check_pixel_float4(&lockrect, 1, 1, -5.03937006e-001f, -7.87401572e-003f, 1.0f, 1.0f, 0, FALSE);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(hr == D3D_OK, "Failed to unlock surface, hr %#lx.\n", hr);

        /* A2W10V10U10 unorm/snorm. */
        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a2w10v10u10, D3DFMT_A2W10V10U10, 8, NULL, &rect,
                D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "Failed to lock surface, hr %#lx.\n", hr);
        check_pixel_float4(&lockrect, 0, 0,  0.0f,              1.81996077e-001f,  3.63992155e-001f, 0.0f,             1, FALSE);
        check_pixel_float4(&lockrect, 1, 0,  5.45988262e-001f,  7.27984309e-001f,  1.0f,             3.33333343e-001f, 1, FALSE);
        check_pixel_float4(&lockrect, 0, 1, -1.0f,             -1.0f,             -5.47945201e-001f, 6.66666687e-001f, 0, FALSE);
        check_pixel_float4(&lockrect, 1, 1, -3.65949124e-001f, -1.83953032e-001f, -1.95694715e-003f, 1.0f,             0, FALSE);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(hr == D3D_OK, "Failed to unlock surface, hr %#lx.\n", hr);

        /* X8L8V8U8 unorm/snorm. */
        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_x8l8v8u8, D3DFMT_X8L8V8U8, 8, NULL, &rect,
                D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

        /* The luma value goes into the alpha channel. */
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "Failed to lock surface, hr %#lx.\n", hr);
        check_pixel_float4(&lockrect, 0, 0,  0.0,               3.77952754e-001f, 1.0f, 0.0f,            0, FALSE);
        check_pixel_float4(&lockrect, 1, 0,  5.03937006e-001f,  1.0f,             1.0f, 3.33333343e-001, 0, FALSE);
        check_pixel_float4(&lockrect, 0, 1, -1.0f,             -1.0f,             1.0f, 6.66666687e-001, 0, FALSE);
        check_pixel_float4(&lockrect, 1, 1, -5.03937006e-001f, -7.87401572e-003f, 1.0f, 1.0f,            0, FALSE);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(hr == D3D_OK, "Failed to unlock surface, hr %#lx.\n", hr);

        check_release((IUnknown*)surf, 0);
    }

    hr = IDirect3DDevice9_CreateTexture(device, 2, 2, 1, 0, D3DFMT_Q8W8V8U8, D3DPOOL_MANAGED, &tex, NULL);
    if (FAILED(hr))
        skip("Failed to create D3DFMT_Q8W8V8U8 texture, hr %#lx.\n", hr);
    else
    {
        hr = IDirect3DTexture9_GetSurfaceLevel(tex, 0, &surf);
        ok(hr == D3D_OK, "Failed to get the surface, hr %#lx.\n", hr);

        /* Snorm to snorm, direct copy. */
        SetRect(&rect, 0, 0, 2, 2);
        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_q8w8v8u8, D3DFMT_Q8W8V8U8, 8, NULL, &rect,
                D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "Failed to lock surface, hr %#lx.\n", hr);
        check_pixel_4bpp(&lockrect, 0, 0, pixdata_q8w8v8u8[0]);
        check_pixel_4bpp(&lockrect, 1, 0, pixdata_q8w8v8u8[1]);
        check_pixel_4bpp(&lockrect, 0, 1, pixdata_q8w8v8u8[2]);
        check_pixel_4bpp(&lockrect, 1, 1, pixdata_q8w8v8u8[3]);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(hr == D3D_OK, "Failed to unlock surface, hr %#lx.\n", hr);

        /* Unorm to snorm. */
        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a8b8g8r8,
                D3DFMT_A8B8G8R8, 8, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "Failed to lock surface, hr %#lx.\n", hr);
        check_pixel_4bpp(&lockrect, 0, 0, 0x43bbce70);
        check_pixel_4bpp(&lockrect, 1, 0, 0xa5dc6812);
        check_pixel_4bpp(&lockrect, 0, 1, 0x8b31177d);
        check_pixel_4bpp(&lockrect, 1, 1, 0x0d43ad76);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(hr == D3D_OK, "Failed to unlock surface, hr %#lx.\n", hr);

        /* Full range float to snorm. */
        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a32b32g32r32f, D3DFMT_A32B32G32R32F, 32, NULL, &rect,
                D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "Failed to lock surface, hr %#lx.\n", hr);
        check_pixel_4bpp(&lockrect, 0, 0, 0x7f7f0d00);
        check_pixel_4bpp(&lockrect, 1, 0, 0x7f7f7f7f);
        check_pixel_4bpp(&lockrect, 0, 1, 0x827fe8f4);
        check_pixel_4bpp(&lockrect, 1, 1, 0x82828282);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(hr == D3D_OK, "Failed to unlock surface, hr %#lx.\n", hr);

        /* Unorm alpha and unorm luma to snorm. */
        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a8l8, D3DFMT_A8L8, 4, NULL, &rect, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "Failed to lock surface, hr %#lx.\n", hr);
        check_pixel_4bpp(&lockrect, 0, 0, 0x7f828282);
        check_pixel_4bpp(&lockrect, 1, 0, 0x827f7f7f);
        check_pixel_4bpp(&lockrect, 0, 1, 0x7fb2b2b2);
        check_pixel_4bpp(&lockrect, 1, 1, 0x00000000);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(hr == D3D_OK, "Failed to unlock surface, hr %#lx.\n", hr);

        check_release((IUnknown*)surf, 1);
        check_release((IUnknown*)tex, 0);
    }

    hr = IDirect3DDevice9_CreateTexture(device, 2, 2, 1, 0, D3DFMT_X8L8V8U8, D3DPOOL_MANAGED, &tex, NULL);
    if (FAILED(hr))
        skip("Failed to create D3DFMT_X8L8V8U8 texture, hr %#lx.\n", hr);
    else
    {
        hr = IDirect3DTexture9_GetSurfaceLevel(tex, 0, &surf);
        ok(hr == D3D_OK, "Failed to get the surface, hr %#lx.\n", hr);

        SetRect(&rect, 0, 0, 2, 2);
        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a8b8g8r8_2, D3DFMT_A8B8G8R8, 8, NULL, &rect,
                D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

        /*
         * The L8 channel here doesn't do RGB->Luma conversion, it just
         * copies the alpha channel directly as UNORM. V8 and U8 are snorm,
         * pulled from r/g respectively. The X8 (b) channel is set to 0.
         */
        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "Failed to lock surface, hr %#lx.\n", hr);
        check_pixel_4bpp(&lockrect, 0, 0, 0x00309282);
        check_pixel_4bpp(&lockrect, 1, 0, 0x0070d2c2);
        check_pixel_4bpp(&lockrect, 0, 1, 0x00b01000);
        check_pixel_4bpp(&lockrect, 1, 1, 0x00ff5040);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(hr == D3D_OK, "Failed to unlock surface, hr %#lx.\n", hr);

        /*
         * Q8 will get converted to unorm range, v8u8, despite being in the
         * same range, will not be directly copied. 0x80/0x81 are clamped to
         * 0x82, and 0xd0/0xc0 end up as 0xd1/0xc1.
         */
        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_q8w8v8u8, D3DFMT_Q8W8V8U8, 8, NULL, &rect,
                D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "Failed to lock surface, hr %#lx.\n", hr);
        check_pixel_4bpp(&lockrect, 0, 0, 0x00b01000);
        check_pixel_4bpp(&lockrect, 1, 0, 0x00ff5040);
        check_pixel_4bpp(&lockrect, 0, 1, 0x002f8282);
        check_pixel_4bpp(&lockrect, 1, 1, 0x007ed1c1);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(hr == D3D_OK, "Failed to unlock surface, hr %#lx.\n", hr);

        check_release((IUnknown*)surf, 1);
        check_release((IUnknown*)tex, 0);
    }

    hr = IDirect3DDevice9_CreateTexture(device, 2, 2, 1, 0, D3DFMT_V8U8, D3DPOOL_MANAGED, &tex, NULL);
    if (FAILED(hr))
        skip("Failed to create D3DFMT_V8U8 texture, hr %#lx.\n", hr);
    else
    {
        hr = IDirect3DTexture9_GetSurfaceLevel(tex, 0, &surf);
        ok(hr == D3D_OK, "Failed to get the surface, hr %#lx.\n", hr);

        /* R and G channels converted to SNORM range. */
        SetRect(&rect, 0, 0, 2, 2);
        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_a8b8g8r8_2, D3DFMT_A8B8G8R8, 8, NULL, &rect,
                D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "Failed to lock surface, hr %#lx.\n", hr);
        check_pixel_2bpp(&lockrect, 0, 0, 0x9282);
        check_pixel_2bpp(&lockrect, 1, 0, 0xd2c2);
        check_pixel_2bpp(&lockrect, 0, 1, 0x1000);
        check_pixel_2bpp(&lockrect, 1, 1, 0x5040);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(hr == D3D_OK, "Failed to unlock surface, hr %#lx.\n", hr);

        /*
         * Pull the V8U8 channels. Even though they're the same range, they're
         * not directly copied over. 0x80/0x81 are clamped to 0x82, and
         * 0xd0/0xc0 end up as 0xd1/0xc1.
         */
        hr = D3DXLoadSurfaceFromMemory(surf, NULL, NULL, pixdata_q8w8v8u8, D3DFMT_Q8W8V8U8, 8, NULL, &rect,
                D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

        hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "Failed to lock surface, hr %#lx.\n", hr);
        check_pixel_2bpp(&lockrect, 0, 0, 0x1000);
        check_pixel_2bpp(&lockrect, 1, 0, 0x5040);
        /*
         * Wine does direct copies from the source pixels here, but if we
         * force conversion to float on the source and then convert back to
         * snorm we'd match these values.
         */
        todo_wine check_pixel_2bpp(&lockrect, 0, 1, 0x8282);
        todo_wine check_pixel_2bpp(&lockrect, 1, 1, 0xd1c1);
        hr = IDirect3DSurface9_UnlockRect(surf);
        ok(hr == D3D_OK, "Failed to unlock surface, hr %#lx.\n", hr);

        check_release((IUnknown*)surf, 1);
        check_release((IUnknown*)tex, 0);
    }

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 2, 2, D3DFMT_A16B16G16R16, D3DPOOL_SCRATCH, &surf, NULL);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

    hr = D3DXLoadSurfaceFromFileInMemory(surf, NULL, NULL, png_2_2_48bpp_rgb, sizeof(png_2_2_48bpp_rgb), NULL,
            D3DX_FILTER_NONE, 0, NULL);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
    ok(hr == D3D_OK, "Failed to lock surface, hr %#lx.\n", hr);

    check_pixel_8bpp(&lockrect, 0, 0, 0xffff202010100000);
    check_pixel_8bpp(&lockrect, 1, 0, 0xffff505040403030);
    check_pixel_8bpp(&lockrect, 0, 1, 0xffff808070706060);
    check_pixel_8bpp(&lockrect, 1, 1, 0xffffb0b0a0a09090);

    hr = IDirect3DSurface9_UnlockRect(surf);
    ok(hr == D3D_OK, "Failed to unlock surface, hr %#lx.\n", hr);

    hr = D3DXLoadSurfaceFromFileInMemory(surf, NULL, NULL, png_2_2_64bpp_rgba, sizeof(png_2_2_64bpp_rgba), NULL,
            D3DX_FILTER_NONE, 0, NULL);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DSurface9_LockRect(surf, &lockrect, NULL, D3DLOCK_READONLY);
    ok(hr == D3D_OK, "Failed to lock surface, hr %#lx.\n", hr);

    check_pixel_8bpp(&lockrect, 0, 0, 0x3030202010100000);
    check_pixel_8bpp(&lockrect, 1, 0, 0x7070606050504040);
    check_pixel_8bpp(&lockrect, 0, 1, 0xb0b0a0a090908080);
    check_pixel_8bpp(&lockrect, 1, 1, 0xf0f0e0e0d0d0c0c0);

    hr = IDirect3DSurface9_UnlockRect(surf);
    ok(hr == D3D_OK, "Failed to unlock surface, hr %#lx.\n", hr);
    check_release((IUnknown *)surf, 0);

    test_format_conversion(device);
    test_dxt_premultiplied_alpha(device);
    test_load_surface_from_tga(device);

    /* cleanup */
    if(testdummy_ok) DeleteFileA("testdummy.bmp");
    if(testbitmap_ok) DeleteFileA("testbitmap.bmp");
}

#define check_dds_pixel_format_struct(pixel_format, expected_pixel_format, wine_todo) \
    check_dds_pixel_format_struct_(__FILE__, __LINE__, pixel_format, expected_pixel_format, wine_todo)
static void check_dds_pixel_format_struct_(const char *file, uint32_t line, const struct dds_pixel_format *pixel_format,
        const struct dds_pixel_format *expected_pixel_format, BOOL wine_todo)
{
    BOOL matched;

    matched = !memcmp(expected_pixel_format, pixel_format, sizeof(*pixel_format));
    todo_wine_if(wine_todo) ok_(file, line)(matched, "Got unexpected dds pixel format values.\n");
    if (matched)
        return;

    todo_wine_if(wine_todo && pixel_format->flags != expected_pixel_format->flags)
        ok_(file, line)(pixel_format->flags == expected_pixel_format->flags, "Unexpected DDS pixel format flags %#lx.\n",
                pixel_format->flags);
    todo_wine_if(wine_todo && pixel_format->fourcc != expected_pixel_format->fourcc)
        ok_(file, line)(pixel_format->fourcc == expected_pixel_format->fourcc, "Unexpected DDS pixel format fourcc %#lx.\n",
                pixel_format->fourcc);
    todo_wine_if(wine_todo && pixel_format->bpp != expected_pixel_format->bpp)
        ok_(file, line)(pixel_format->bpp == expected_pixel_format->bpp, "Unexpected DDS pixel format bpp %#lx.\n",
                pixel_format->bpp);
    todo_wine_if(wine_todo && pixel_format->rmask != expected_pixel_format->rmask)
        ok_(file, line)(pixel_format->rmask == expected_pixel_format->rmask, "Unexpected DDS pixel format rmask %#lx.\n",
                pixel_format->rmask);
    todo_wine_if(wine_todo && pixel_format->gmask != expected_pixel_format->gmask)
        ok_(file, line)(pixel_format->gmask == expected_pixel_format->gmask, "Unexpected DDS pixel format gmask %#lx.\n",
                pixel_format->gmask);
    todo_wine_if(wine_todo && pixel_format->bmask != expected_pixel_format->bmask)
        ok_(file, line)(pixel_format->bmask == expected_pixel_format->bmask, "Unexpected DDS pixel format bmask %#lx.\n",
                pixel_format->bmask);
    todo_wine_if(wine_todo && pixel_format->amask != expected_pixel_format->amask)
        ok_(file, line)(pixel_format->amask == expected_pixel_format->amask, "Unexpected DDS pixel format amask %#lx.\n",
                pixel_format->amask);
}

#define check_dds_header(header, flags, height, width, pitch, depth, mip_levels, pixel_format, caps, caps2, wine_todo) \
    check_dds_header_(__FILE__, __LINE__, header, flags, height, width, pitch, depth, mip_levels, pixel_format, \
                      caps, caps2, wine_todo)
static void check_dds_header_(const char *file, uint32_t line, const struct dds_header *header, uint32_t flags,
        uint32_t height, uint32_t width, uint32_t pitch, uint32_t depth, uint32_t mip_levels,
        const struct dds_pixel_format *pixel_format, uint32_t caps, uint32_t caps2, BOOL wine_todo)
{
    const struct dds_header expected_header = { sizeof(*header), flags, height, width, pitch, depth, mip_levels, { 0 },
                                                *pixel_format, caps, caps2, 0, 0, 0 };
    BOOL matched;

    matched = !memcmp(&expected_header, header, sizeof(*header));
    todo_wine_if(wine_todo) ok_(file, line)(matched, "Got unexpected dds header values.\n");
    if (matched)
        return;

    todo_wine_if(wine_todo && header->flags != flags)
        ok_(file, line)(header->flags == flags, "Unexpected DDS header flags %#lx.\n", header->flags);
    todo_wine_if(wine_todo && header->width != width)
        ok_(file, line)(header->width == width, "Unexpected DDS header width %#lx.\n", header->width);
    todo_wine_if(wine_todo && header->height != height)
        ok_(file, line)(header->height == height, "Unexpected DDS header height %#lx.\n", header->height);
    todo_wine_if(wine_todo && header->pitch_or_linear_size != pitch)
        ok_(file, line)(header->pitch_or_linear_size == pitch, "Unexpected DDS header pitch %#lx.\n",
                header->pitch_or_linear_size);
    todo_wine_if(wine_todo && header->depth != depth)
        ok_(file, line)(header->depth == depth, "Unexpected DDS header depth %#lx.\n", header->depth);
    todo_wine_if(wine_todo && header->miplevels != mip_levels)
        ok_(file, line)(header->miplevels == mip_levels, "Unexpected DDS header mip levels %#lx.\n", header->miplevels);
    ok_(file, line)(!memcmp(header->reserved, expected_header.reserved, sizeof(header->reserved)),
            "Unexpected values in DDS header reserved field.");
    check_dds_pixel_format_struct(&header->pixel_format, pixel_format, FALSE);
    todo_wine_if(wine_todo && header->caps != caps)
        ok_(file, line)(header->caps == caps, "Unexpected DDS header caps %#lx.\n", header->caps);
    todo_wine_if(wine_todo && header->caps2 != caps2)
        ok_(file, line)(header->caps2 == caps2, "Unexpected DDS header caps2 %#lx.\n", header->caps2);
    ok_(file, line)(!header->caps3, "Unexpected DDS header caps3 %#lx.\n", header->caps3);
    ok_(file, line)(!header->caps4, "Unexpected DDS header caps4 %#lx.\n", header->caps4);
    ok_(file, line)(!header->reserved2, "Unexpected DDS header reserved2 %#lx.\n", header->reserved2);
}

#define DDS_FILE_HEADER_SIZE (sizeof(uint32_t) + sizeof(struct dds_header))
#define PALETTED_DDS_FILE_HEADER_SIZE (DDS_FILE_HEADER_SIZE + (sizeof(PALETTEENTRY) * 256))
static void test_save_surface_to_dds(IDirect3DDevice9 *device)
{
    struct expected
    {
        HRESULT hr;
        struct dds_pixel_format pixel_format;
        uint32_t flags;
        uint32_t width;
        uint32_t height;
        uint32_t pitch;
        uint32_t depth;
        uint32_t mip_levels;
        uint32_t caps;
        uint32_t caps2;
        uint32_t buffer_size;
    };
    static const struct
    {
        D3DFORMAT format;
        uint32_t width;
        uint32_t height;
        const PALETTEENTRY *palette;
        struct expected expected_vals;
        BOOL todo_hr;
        BOOL todo_expected;
    } dds_tests[] =
    {
        { D3DFMT_P8, 4, 4, test_palette,
          { D3D_OK, { 32, DDS_PF_INDEXED, 0, 8, 0, 0, 0, 0 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE | DDSCAPS_PALETTE, 0,
            PALETTED_DDS_FILE_HEADER_SIZE + (4 * 4)
          }
        },
        { D3DFMT_A8P8, 4, 4, test_palette,
          { D3D_OK, { 32, DDS_PF_INDEXED | DDS_PF_ALPHA, 0, 16, 0, 0, 0, 0xff00 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0,
            DDSCAPS_TEXTURE | DDSCAPS_PALETTE | DDSCAPS_ALPHA, 0,
            PALETTED_DDS_FILE_HEADER_SIZE + (4 * 2 * 4)
          }
        },
        /* If a palette isn't provided, d3dx converts to D3DFMT_A8R8G8B8. */
        { D3DFMT_P8, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE | DDSCAPS_ALPHA, 0,
            DDS_FILE_HEADER_SIZE + (4 * 4 * 4)
          }, .todo_hr = TRUE
        },
        { D3DFMT_A8P8, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE | DDSCAPS_ALPHA, 0,
            DDS_FILE_HEADER_SIZE + (4 * 4 * 4)
          }, .todo_hr = TRUE
        },
        { D3DFMT_V8U8, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_BUMPDUDV, 0, 16, 0x000000ff, 0x0000ff00, 0x00000000, 0x00000000 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE, 0,
            DDS_FILE_HEADER_SIZE + (4 * 2 * 4)
          }
        },
        { D3DFMT_V16U16, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_BUMPDUDV, 0, 32, 0x0000ffff, 0xffff0000, 0x00000000, 0x00000000 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE, 0,
            DDS_FILE_HEADER_SIZE + (4 * 4 * 4)
          }
        },
        { D3DFMT_Q8W8V8U8, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_BUMPDUDV, 0, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE, 0,
            DDS_FILE_HEADER_SIZE + (4 * 4 * 4)
          }
        },
        { D3DFMT_A2W10V10U10, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_BUMPDUDV | DDS_PF_ALPHA, 0, 32, 0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE | DDSCAPS_ALPHA, 0,
            DDS_FILE_HEADER_SIZE + (4 * 4 * 4)
          }
        },
        { D3DFMT_X8L8V8U8, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_BUMPLUMINANCE, 0, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE, 0,
            DDS_FILE_HEADER_SIZE + (4 * 4 * 4)
          }
        },
        { D3DFMT_R5G6B5, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_RGB, 0, 16, 0xf800, 0x07e0, 0x001f, 0 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE, 0,
            DDS_FILE_HEADER_SIZE + (4 * 2 * 4)
          }
        },
        /* 10. */
        { D3DFMT_A1R5G5B5, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 16, 0x7c00, 0x03e0, 0x001f, 0x8000 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE | DDSCAPS_ALPHA, 0,
            DDS_FILE_HEADER_SIZE + (4 * 2 * 4)
          }
        },
        { D3DFMT_A4R4G4B4, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 16, 0x0f00, 0x00f0, 0x000f, 0xf000 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE | DDSCAPS_ALPHA, 0,
            DDS_FILE_HEADER_SIZE + (4 * 2 * 4)
          }
        },
        { D3DFMT_R3G3B2, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_RGB, 0, 8, 0xe0, 0x1c, 0x03, 0 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE, 0,
            DDS_FILE_HEADER_SIZE + (4 * 4)
          }
        },
        { D3DFMT_A8R3G3B2, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 16, 0x00e0, 0x001c, 0x0003, 0xff00 },
              DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE | DDSCAPS_ALPHA, 0,
              DDS_FILE_HEADER_SIZE + (4 * 2 * 4)
          }
        },
        { D3DFMT_X4R4G4B4, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_RGB, 0, 16, 0xf00, 0x0f0, 0x00f, 0 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE, 0,
            DDS_FILE_HEADER_SIZE + (4 * 2 * 4)
          }
        },
        { D3DFMT_A2B10G10R10, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE | DDSCAPS_ALPHA, 0,
            DDS_FILE_HEADER_SIZE + (4 * 4 * 4)
          }
        },
        { D3DFMT_A2R10G10B10, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x000003ff, 0x000ffc00, 0x3ff00000, 0xc0000000 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE | DDSCAPS_ALPHA, 0,
            DDS_FILE_HEADER_SIZE + (4 * 4 * 4)
          }
        },
        { D3DFMT_A8R8G8B8, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE | DDSCAPS_ALPHA, 0,
            DDS_FILE_HEADER_SIZE + (4 * 4 * 4)
          }
        },
        { D3DFMT_A8B8G8R8, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE | DDSCAPS_ALPHA, 0,
            DDS_FILE_HEADER_SIZE + (4 * 4 * 4)
          }
        },
        { D3DFMT_X8R8G8B8, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_RGB, 0, 32, 0xff0000, 0x00ff00, 0x0000ff, 0 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE, 0,
            DDS_FILE_HEADER_SIZE + (4 * 4 * 4)
          }
        },
        /* 20. */
        { D3DFMT_X8B8G8R8, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_RGB, 0, 32, 0x0000ff, 0x00ff00, 0xff0000, 0 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE, 0,
            DDS_FILE_HEADER_SIZE + (4 * 4 * 4)
          }
        },
        { D3DFMT_R8G8B8, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE, 0,
            DDS_FILE_HEADER_SIZE + (4 * 3 * 4)
          }
        },
        { D3DFMT_G16R16, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_RGB, 0, 32, 0x0000ffff, 0xffff0000, 0, 0 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE, 0,
            DDS_FILE_HEADER_SIZE + (4 * 4 * 4)
          }
        },
        { D3DFMT_A8, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_ALPHA_ONLY, 0, 8, 0, 0, 0, 0xff },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE | DDSCAPS_ALPHA, 0,
            DDS_FILE_HEADER_SIZE + (4 * 4)
          }
        },
        { D3DFMT_DXT1, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_FOURCC, D3DFMT_DXT1, 0, 0, 0, 0, 0 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE, 0,
            DDS_FILE_HEADER_SIZE + (8)
          }
        },
        { D3DFMT_DXT2, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_FOURCC, D3DFMT_DXT2, 0, 0, 0, 0, 0 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE, 0,
            DDS_FILE_HEADER_SIZE + (16)
          }
        },
        { D3DFMT_DXT3, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_FOURCC, D3DFMT_DXT3, 0, 0, 0, 0, 0 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE, 0,
            DDS_FILE_HEADER_SIZE + (16)
          }
        },
        { D3DFMT_DXT4, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_FOURCC, D3DFMT_DXT4, 0, 0, 0, 0, 0 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE, 0,
            DDS_FILE_HEADER_SIZE + (16)
          }
        },
        { D3DFMT_DXT5, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_FOURCC, D3DFMT_DXT5, 0, 0, 0, 0, 0 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE, 0,
            DDS_FILE_HEADER_SIZE + (16)
          }
        },
        { D3DFMT_A16B16G16R16, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_FOURCC, D3DFMT_A16B16G16R16, 0, 0, 0, 0, 0 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE, 0,
            DDS_FILE_HEADER_SIZE + (4 * 8 * 4)
          }
        },
        /* 30. */
        { D3DFMT_Q16W16V16U16, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_FOURCC, D3DFMT_Q16W16V16U16, 0, 0, 0, 0, 0 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE, 0,
            DDS_FILE_HEADER_SIZE + (4 * 8 * 4)
          }
        },
        { D3DFMT_R16F, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_FOURCC, D3DFMT_R16F, 0, 0, 0, 0, 0 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE, 0,
            DDS_FILE_HEADER_SIZE + (4 * 2 * 4)
          }
        },
        { D3DFMT_G16R16F, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_FOURCC, D3DFMT_G16R16F, 0, 0, 0, 0, 0 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE, 0,
            DDS_FILE_HEADER_SIZE + (4 * 4 * 4)
          }
        },
        { D3DFMT_A16B16G16R16F, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_FOURCC, D3DFMT_A16B16G16R16F, 0, 0, 0, 0, 0 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE, 0,
            DDS_FILE_HEADER_SIZE + (4 * 8 * 4)
          }
        },
        { D3DFMT_R32F, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_FOURCC, D3DFMT_R32F, 0, 0, 0, 0, 0 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE, 0,
            DDS_FILE_HEADER_SIZE + (4 * 4 * 4)
          }
        },
        { D3DFMT_G32R32F, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_FOURCC, D3DFMT_G32R32F, 0, 0, 0, 0, 0 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE, 0,
            DDS_FILE_HEADER_SIZE + (4 * 8 * 4)
          }
        },
        { D3DFMT_A32B32G32R32F, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_FOURCC, D3DFMT_A32B32G32R32F, 0, 0, 0, 0, 0 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE, 0,
            DDS_FILE_HEADER_SIZE + (4 * 16 * 4)
          }
        },
        { D3DFMT_G8R8_G8B8, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_FOURCC, D3DFMT_G8R8_G8B8, 0, 0, 0, 0, 0 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE, 0,
            DDS_FILE_HEADER_SIZE + (4 * 2 * 4)
          }
        },
        { D3DFMT_R8G8_B8G8, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_FOURCC, D3DFMT_R8G8_B8G8, 0, 0, 0, 0, 0 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE, 0,
            DDS_FILE_HEADER_SIZE + (4 * 2 * 4)
          }
        },
        { D3DFMT_UYVY, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_FOURCC, D3DFMT_UYVY, 0, 0, 0, 0, 0 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE, 0,
            DDS_FILE_HEADER_SIZE + (4 * 2 * 4)
          }
        },
        /* 40. */
        { D3DFMT_YUY2, 4, 4, NULL,
          { D3D_OK, { 32, DDS_PF_FOURCC, D3DFMT_YUY2, 0, 0, 0, 0, 0 },
            DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0, DDSCAPS_TEXTURE, 0,
            DDS_FILE_HEADER_SIZE + (4 * 2 * 4)
          }
        },
    };
    struct
    {
         DWORD magic;
         struct dds_header header;
         BYTE *data;
    } *dds;
    IDirect3DSurface9 *surface;
    ID3DXBuffer *buffer;
    unsigned int i;
    HRESULT hr;

    for (i = 0; i < ARRAY_SIZE(dds_tests); ++i)
    {
        const struct expected *expected = &dds_tests[i].expected_vals;

        hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, dds_tests[i].width, dds_tests[i].height, dds_tests[i].format,
                D3DPOOL_SCRATCH, &surface, NULL);
        if (FAILED(hr))
        {
            skip("Couldn't create surface for format %#x.\n", dds_tests[i].format);
            continue;
        }

        winetest_push_context("Test %u", i);
        hr = D3DXSaveSurfaceToFileInMemory(&buffer, D3DXIFF_DDS, surface, dds_tests[i].palette, NULL);
        todo_wine_if(dds_tests[i].todo_hr) ok(hr == expected->hr, "Unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
        {
            ok(ID3DXBuffer_GetBufferSize(buffer) == expected->buffer_size, "Unexpected buffer size %lu.\n",
                    ID3DXBuffer_GetBufferSize(buffer));

            dds = ID3DXBuffer_GetBufferPointer(buffer);
            check_dds_header(&dds->header, expected->flags, expected->height, expected->width, expected->pitch,
                    expected->depth, expected->mip_levels, &expected->pixel_format, expected->caps, expected->caps2,
                    dds_tests[i].todo_expected);
            ID3DXBuffer_Release(buffer);
        }

        IDirect3DSurface9_Release(surface);
        winetest_pop_context();
    }
}

static void test_D3DXSaveSurfaceToFileInMemory(IDirect3DDevice9 *device)
{
    static const struct dds_pixel_format d3dfmt_a8r8g8b8_pf = { 32, DDS_PF_RGB | DDS_PF_ALPHA, 0, 32,
                                                                0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 };
    static const uint32_t tmp_pixdata_4_4[] = { 0xff000000, 0xff000000, 0xff000000, 0xff000000,
                                                0x00ff0040, 0x00ff0040, 0x00ff0040, 0x00ff0040,
                                                0x0000ff80, 0x0000ff80, 0x0000ff80, 0x0000ff80,
                                                0x000000c0, 0x000000c0, 0x000000c0, 0x000000c0 };
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

    struct
    {
         DWORD magic;
         struct dds_header header;
         BYTE *data;
    } *dds;
    IDirect3DSurface9 *surface;
    IDirect3DTexture9 *texture;
    unsigned int i, x, y;
    ID3DXBuffer *buffer;
    HRESULT hr;
    RECT rect;

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 4, 4, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &surface, NULL);
    if (FAILED(hr)) {
       skip("Couldn't create surface\n");
       return;
    }

    SetRect(&rect, 0, 0, 4, 4);
    hr = D3DXLoadSurfaceFromMemory(surface, NULL, NULL, tmp_pixdata_4_4, D3DFMT_A8R8G8B8, 16, NULL, &rect, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    SetRectEmpty(&rect);
    hr = D3DXSaveSurfaceToFileInMemory(&buffer, D3DXIFF_BMP, surface, NULL, &rect);
    /* fails with the debug version of d3d9 */
    ok(hr == D3D_OK || broken(hr == D3DERR_INVALIDCALL), "D3DXSaveSurfaceToFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);
    if (SUCCEEDED(hr)) {
        DWORD size = ID3DXBuffer_GetBufferSize(buffer);
        ok(size > 0, "ID3DXBuffer_GetBufferSize returned %lu, expected > 0\n", size);
        ID3DXBuffer_Release(buffer);
    }

    SetRectEmpty(&rect);
    hr = D3DXSaveSurfaceToFileInMemory(&buffer, D3DXIFF_DDS, surface, NULL, &rect);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    dds = ID3DXBuffer_GetBufferPointer(buffer);
    check_dds_header(&dds->header, DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 0, 0, 0, 0, 0,
            &d3dfmt_a8r8g8b8_pf, DDSCAPS_TEXTURE | DDSCAPS_ALPHA, 0, FALSE);
    ID3DXBuffer_Release(buffer);

    /* Test rectangle argument for D3DXIFF_DDS. */
    SetRect(&rect, 0, 0, 0, 2);
    hr = D3DXSaveSurfaceToFileInMemory(&buffer, D3DXIFF_DDS, surface, NULL, &rect);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    dds = ID3DXBuffer_GetBufferPointer(buffer);
    check_dds_header(&dds->header, DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 2, 0, 0, 0, 0,
            &d3dfmt_a8r8g8b8_pf, DDSCAPS_TEXTURE | DDSCAPS_ALPHA, 0, FALSE);
    ID3DXBuffer_Release(buffer);

    SetRect(&rect, 0, 0, 2, 0);
    hr = D3DXSaveSurfaceToFileInMemory(&buffer, D3DXIFF_DDS, surface, NULL, &rect);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    dds = ID3DXBuffer_GetBufferPointer(buffer);
    check_dds_header(&dds->header, DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 0, 2, 0, 0, 0,
            &d3dfmt_a8r8g8b8_pf, DDSCAPS_TEXTURE | DDSCAPS_ALPHA, 0, FALSE);
    ID3DXBuffer_Release(buffer);

    SetRect(&rect, 2, 2, 4, 4);
    hr = D3DXSaveSurfaceToFileInMemory(&buffer, D3DXIFF_DDS, surface, NULL, &rect);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    dds = ID3DXBuffer_GetBufferPointer(buffer);
    check_dds_header(&dds->header, DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 2, 2, 0, 0, 0,
            &d3dfmt_a8r8g8b8_pf, DDSCAPS_TEXTURE | DDSCAPS_ALPHA, 0, FALSE);
    for (y = 0; y < 2; ++y)
    {
        for (x = 0; x < 2; ++x)
        {
            const uint32_t expected_pixel = tmp_pixdata_4_4[((2 + y) * 4) + (x + 2)];
            const uint32_t saved_pixel = ((uint32_t *)&dds->data)[(y * 2) + x];

            ok(expected_pixel == saved_pixel, "Unexpected pixel value %#x.\n", saved_pixel);
        }
    }
    ID3DXBuffer_Release(buffer);

    hr = D3DXSaveSurfaceToFileInMemory(&buffer, D3DXIFF_DDS, surface, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    dds = ID3DXBuffer_GetBufferPointer(buffer);
    check_dds_header(&dds->header, DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT, 4, 4, 0, 0, 0,
            &d3dfmt_a8r8g8b8_pf, DDSCAPS_TEXTURE | DDSCAPS_ALPHA, 0, FALSE);
    ID3DXBuffer_Release(buffer);

    IDirect3DSurface9_Release(surface);

    for (i = 0; i < ARRAY_SIZE(test_access_types); ++i)
    {
        hr = IDirect3DDevice9_CreateTexture(device, 4, 4, 0, test_access_types[i].usage,
                D3DFMT_A8R8G8B8, test_access_types[i].pool, &texture, NULL);
        ok(hr == D3D_OK, "Unexpected hr %#lx, i %u.\n", hr, i);

        hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
        ok(hr == D3D_OK, "Unexpected hr %#lx, i %u.\n", hr, i);

        hr = D3DXSaveSurfaceToFileInMemory(&buffer, D3DXIFF_DDS, surface, NULL, NULL);
        ok(hr == D3D_OK, "Unexpected hr %#lx, i %u.\n", hr, i);
        ID3DXBuffer_Release(buffer);

        hr = D3DXSaveSurfaceToFileInMemory(&buffer, D3DXIFF_BMP, surface, NULL, NULL);
        ok(hr == D3D_OK, "Unexpected hr %#lx, i %u.\n", hr, i);
        ID3DXBuffer_Release(buffer);

        IDirect3DSurface9_Release(surface);
        IDirect3DTexture9_Release(texture);
    }

    test_save_surface_to_dds(device);
}

static void test_D3DXSaveSurfaceToFile(IDirect3DDevice9 *device)
{
    static const BYTE pixels[] =
            {0xff, 0x00, 0x00, 0x00, 0xff, 0x00,
             0x00, 0x00, 0xff, 0x00, 0x00, 0xff,};
    DWORD pitch = sizeof(pixels) / 2;
    IDirect3DSurface9 *surface;
    D3DXIMAGE_INFO image_info;
    D3DLOCKED_RECT lock_rect;
    HRESULT hr;
    RECT rect;

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 2, 2, D3DFMT_R8G8B8, D3DPOOL_SCRATCH, &surface, NULL);
    if (FAILED(hr))
    {
       skip("Couldn't create surface.\n");
       return;
    }

    SetRect(&rect, 0, 0, 2, 2);
    hr = D3DXLoadSurfaceFromMemory(surface, NULL, NULL, pixels, D3DFMT_R8G8B8,
            pitch, NULL, &rect, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = D3DXSaveSurfaceToFileA("saved_surface.bmp", D3DXIFF_BMP, surface, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = D3DXLoadSurfaceFromFileA(surface, NULL, NULL, "saved_surface.bmp",
            NULL, D3DX_FILTER_NONE, 0, &image_info);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    ok(image_info.Width == 2, "Wrong width %u.\n", image_info.Width);
    ok(image_info.Height == 2, "Wrong height %u.\n", image_info.Height);
    ok(image_info.Format == D3DFMT_R8G8B8, "Wrong format %#x.\n", image_info.Format);
    ok(image_info.ImageFileFormat == D3DXIFF_BMP, "Wrong file format %u.\n", image_info.ImageFileFormat);

    hr = IDirect3DSurface9_LockRect(surface, &lock_rect, NULL, D3DLOCK_READONLY);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    ok(!memcmp(lock_rect.pBits, pixels, pitch),
            "Pixel data mismatch in the first row.\n");
    ok(!memcmp((BYTE *)lock_rect.pBits + lock_rect.Pitch, pixels + pitch, pitch),
            "Pixel data mismatch in the second row.\n");

    IDirect3DSurface9_UnlockRect(surface);

    SetRect(&rect, 0, 1, 2, 2);
    hr = D3DXSaveSurfaceToFileA("saved_surface.bmp", D3DXIFF_BMP, surface, NULL, &rect);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    SetRect(&rect, 0, 0, 2, 1);
    hr = D3DXLoadSurfaceFromFileA(surface, NULL, &rect, "saved_surface.bmp", NULL,
            D3DX_FILTER_NONE, 0, &image_info);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DSurface9_LockRect(surface, &lock_rect, NULL, D3DLOCK_READONLY);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!memcmp(lock_rect.pBits, pixels + pitch, pitch),
            "Pixel data mismatch in the first row.\n");
    ok(!memcmp((BYTE *)lock_rect.pBits + lock_rect.Pitch, pixels + pitch, pitch),
            "Pixel data mismatch in the second row.\n");
    IDirect3DSurface9_UnlockRect(surface);

    SetRect(&rect, 0, 0, 2, 2);
    hr = D3DXLoadSurfaceFromMemory(surface, NULL, NULL, pixels, D3DFMT_R8G8B8,
            pitch, NULL, &rect, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = D3DXSaveSurfaceToFileA(NULL, D3DXIFF_BMP, surface, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

    /* PPM and TGA are supported, even though MSDN claims they aren't */
    todo_wine
    {
        hr = D3DXSaveSurfaceToFileA("saved_surface.ppm", D3DXIFF_PPM, surface, NULL, NULL);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
        hr = D3DXSaveSurfaceToFileA("saved_surface.tga", D3DXIFF_TGA, surface, NULL, NULL);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    }

    hr = D3DXSaveSurfaceToFileA("saved_surface.dds", D3DXIFF_DDS, surface, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = D3DXLoadSurfaceFromFileA(surface, NULL, NULL, "saved_surface.dds",
            NULL, D3DX_FILTER_NONE, 0, &image_info);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    ok(image_info.Width == 2, "Wrong width %u.\n", image_info.Width);
    ok(image_info.Format == D3DFMT_R8G8B8, "Wrong format %#x.\n", image_info.Format);
    ok(image_info.ImageFileFormat == D3DXIFF_DDS, "Wrong file format %u.\n", image_info.ImageFileFormat);

    hr = IDirect3DSurface9_LockRect(surface, &lock_rect, NULL, D3DLOCK_READONLY);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!memcmp(lock_rect.pBits, pixels, pitch),
            "Pixel data mismatch in the first row.\n");
    ok(!memcmp((BYTE *)lock_rect.pBits + lock_rect.Pitch, pixels + pitch, pitch),
            "Pixel data mismatch in the second row.\n");
    IDirect3DSurface9_UnlockRect(surface);

    hr = D3DXSaveSurfaceToFileA("saved_surface", D3DXIFF_PFM + 1, surface, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

    SetRect(&rect, 0, 0, 4, 4);
    hr = D3DXSaveSurfaceToFileA("saved_surface.bmp", D3DXIFF_BMP, surface, NULL, &rect);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    SetRect(&rect, 2, 0, 1, 4);
    hr = D3DXSaveSurfaceToFileA("saved_surface.bmp", D3DXIFF_BMP, surface, NULL, &rect);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    SetRect(&rect, 0, 2, 4, 1);
    hr = D3DXSaveSurfaceToFileA("saved_surface.bmp", D3DXIFF_BMP, surface, NULL, &rect);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    SetRect(&rect, -1, -1, 2, 2);
    hr = D3DXSaveSurfaceToFileA("saved_surface.bmp", D3DXIFF_BMP, surface, NULL, &rect);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    SetRectEmpty(&rect);
    hr = D3DXSaveSurfaceToFileA("saved_surface.bmp", D3DXIFF_BMP, surface, NULL, &rect);
    /* fails when debug version of d3d9 is used */
    ok(hr == D3D_OK || broken(hr == D3DERR_INVALIDCALL), "Got unexpected hr %#lx.\n", hr);

    DeleteFileA("saved_surface.bmp");
    DeleteFileA("saved_surface.ppm");
    DeleteFileA("saved_surface.tga");
    DeleteFileA("saved_surface.dds");

    IDirect3DSurface9_Release(surface);
}

START_TEST(surface)
{
    HWND wnd;
    IDirect3D9 *d3d;
    IDirect3DDevice9 *device;
    D3DPRESENT_PARAMETERS d3dpp;
    HRESULT hr;

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
    if(FAILED(hr)) {
        skip("Failed to create IDirect3DDevice9 object %#lx\n", hr);
        IDirect3D9_Release(d3d);
        DestroyWindow(wnd);
        return;
    }

    test_D3DXGetImageInfo();
    test_D3DXLoadSurface(device);
    test_D3DXSaveSurfaceToFileInMemory(device);
    test_D3DXSaveSurfaceToFile(device);

    check_release((IUnknown*)device, 0);
    check_release((IUnknown*)d3d, 0);
    DestroyWindow(wnd);
}
