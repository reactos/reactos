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
#include <stdint.h>
#include "d3dx9_test_images.h"

static int has_2d_dxt1, has_2d_dxt3, has_2d_dxt5, has_cube_dxt5, has_3d_dxt3;

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

static inline void check_surface_desc(uint32_t line, D3DFORMAT format, uint32_t usage, D3DPOOL pool,
        D3DMULTISAMPLE_TYPE multi_sample_type, uint32_t multi_sample_quality, uint32_t width, uint32_t height,
        const D3DSURFACE_DESC *desc, BOOL wine_todo)
{
    const D3DSURFACE_DESC expected_desc = { format, D3DRTYPE_SURFACE, usage, pool, multi_sample_type,
                                            multi_sample_quality, width, height };
    BOOL matched;

    matched = !memcmp(&expected_desc, desc, sizeof(*desc));
    todo_wine_if(wine_todo) ok_(__FILE__, line)(matched, "Got unexpected surface desc values.\n");
    if (matched)
        return;

    todo_wine_if(wine_todo && desc->Format != format)
        ok_(__FILE__, line)(desc->Format == format, "Expected surface format %d, got %d.\n", format, desc->Format);
    ok_(__FILE__, line)(desc->Type == D3DRTYPE_SURFACE, "Expected D3DRTYPE_SURFACE, got %d.\n", desc->Type);
    todo_wine_if(wine_todo && desc->Usage != usage)
        ok_(__FILE__, line)(desc->Usage == usage, "Expected usage %u, got %lu.\n", usage, desc->Usage);
    todo_wine_if(wine_todo && desc->Pool != pool)
        ok_(__FILE__, line)(desc->Pool == pool, "Expected pool %d, got %d.\n", pool, desc->Pool);
    todo_wine_if(wine_todo && desc->MultiSampleType != multi_sample_type)
        ok_(__FILE__, line)(desc->MultiSampleType == multi_sample_type, "Expected multi sample type %d, got %d.\n",
                multi_sample_type, desc->MultiSampleType);
    todo_wine_if(wine_todo && desc->MultiSampleQuality != multi_sample_quality)
        ok_(__FILE__, line)(desc->MultiSampleQuality == multi_sample_quality, "Expected multi sample quality %u, got %lu.\n",
                multi_sample_quality, desc->MultiSampleQuality);
    todo_wine_if(wine_todo && desc->Width != width)
        ok_(__FILE__, line)(desc->Width == width, "Expected width %d, got %d.\n", width, desc->Width);
    todo_wine_if(wine_todo && desc->Height != height)
        ok_(__FILE__, line)(desc->Height == height, "Expected height %d, got %d.\n", height, desc->Height);
}

#define check_texture_level_desc(tex, level, format, usage, pool, multi_sample_type, multi_sample_quality, width, \
                                 height, wine_todo) \
    check_texture_level_desc_(__LINE__, tex, level, format, usage, pool, multi_sample_type, multi_sample_quality, \
            width, height, wine_todo)
static inline void check_texture_level_desc_(uint32_t line, IDirect3DTexture9 *tex, uint32_t level, D3DFORMAT format,
        uint32_t usage, D3DPOOL pool, D3DMULTISAMPLE_TYPE multi_sample_type,
        uint32_t multi_sample_quality, uint32_t width, uint32_t height, BOOL wine_todo)
{
    D3DSURFACE_DESC desc;
    HRESULT hr;

    hr = IDirect3DTexture9_GetLevelDesc(tex, level, &desc);
    todo_wine_if(wine_todo && FAILED(hr))
        ok_(__FILE__, line)(hr == S_OK, "Failed to get texture level desc with hr %#lx.\n", hr);
    if (FAILED(hr))
        return;

    check_surface_desc(line, format, usage, pool, multi_sample_type, multi_sample_quality, width, height, &desc,
            wine_todo);
}

#define check_cube_texture_level_desc(tex, level, format, usage, pool, multi_sample_type, multi_sample_quality, size, \
                                 wine_todo) \
    check_cube_texture_level_desc_(__LINE__, tex, level, format, usage, pool, multi_sample_type, multi_sample_quality, \
            size, wine_todo)
static inline void check_cube_texture_level_desc_(uint32_t line, IDirect3DCubeTexture9 *tex, uint32_t level,
        D3DFORMAT format, uint32_t usage, D3DPOOL pool, D3DMULTISAMPLE_TYPE multi_sample_type,
        uint32_t multi_sample_quality, uint32_t size, BOOL wine_todo)
{
    D3DSURFACE_DESC desc;
    HRESULT hr;

    hr = IDirect3DCubeTexture9_GetLevelDesc(tex, level, &desc);
    todo_wine_if(wine_todo && FAILED(hr))
        ok_(__FILE__, line)(hr == S_OK, "Failed to get cube texture level desc with hr %#lx.\n", hr);
    if (FAILED(hr))
        return;

    check_surface_desc(line, format, usage, pool, multi_sample_type, multi_sample_quality, size, size, &desc, wine_todo);
}

#define check_volume_texture_level_desc(tex, level, format, usage, pool, width, height, depth, wine_todo) \
    check_volume_texture_level_desc_(__LINE__, tex, level, format, usage, pool, width, height, depth, wine_todo)
static inline void check_volume_texture_level_desc_(uint32_t line, IDirect3DVolumeTexture9 *tex, uint32_t level,
        D3DFORMAT format, uint32_t usage, D3DPOOL pool, uint32_t width, uint32_t height, uint32_t depth, BOOL wine_todo)
{
    const D3DVOLUME_DESC expected_desc = { format, D3DRTYPE_VOLUME, usage, pool, width, height, depth };
    D3DVOLUME_DESC desc;
    BOOL matched;
    HRESULT hr;

    hr = IDirect3DVolumeTexture9_GetLevelDesc(tex, level, &desc);
    todo_wine_if(wine_todo && FAILED(hr))
        ok_(__FILE__, line)(hr == S_OK, "Failed to get texture level desc with hr %#lx.\n", hr);
    if (FAILED(hr))
        return;

    matched = !memcmp(&expected_desc, &desc, sizeof(desc));
    todo_wine_if(wine_todo) ok_(__FILE__, line)(matched, "Got unexpected volume desc values.\n");
    if (matched)
        return;

    todo_wine_if(wine_todo && desc.Format != format)
        ok_(__FILE__, line)(desc.Format == format, "Expected volume format %d, got %d.\n", format, desc.Format);
    ok_(__FILE__, line)(desc.Type == D3DRTYPE_VOLUME, "Expected D3DRTYPE_VOLUME, got %d.\n", desc.Type);
    todo_wine_if(wine_todo && desc.Usage != usage)
        ok_(__FILE__, line)(desc.Usage == usage, "Expected usage %u, got %lu.\n", usage, desc.Usage);
    todo_wine_if(wine_todo && desc.Pool != pool)
        ok_(__FILE__, line)(desc.Pool == pool, "Expected pool %d, got %d.\n", pool, desc.Pool);
    todo_wine_if(wine_todo && desc.Width != width)
        ok_(__FILE__, line)(desc.Width == width, "Expected width %u, got %u.\n", width, desc.Width);
    todo_wine_if(wine_todo && desc.Height != height)
        ok_(__FILE__, line)(desc.Height == height, "Expected height %u, got %u.\n", height, desc.Height);
    todo_wine_if(wine_todo && desc.Depth != depth)
        ok_(__FILE__, line)(desc.Depth == depth, "Expected depth %u, got %u.\n", depth, desc.Depth);
}

#define check_texture_mip_levels(tex, expected_mip_levels, wine_todo) \
    check_texture_mip_levels_(__LINE__, ((IDirect3DBaseTexture9 *)tex), expected_mip_levels, wine_todo)
static inline void check_texture_mip_levels_(uint32_t line, IDirect3DBaseTexture9 *tex, uint32_t expected_mip_levels,
        BOOL wine_todo)
{
    uint32_t mip_levels = IDirect3DBaseTexture9_GetLevelCount(tex);

    todo_wine_if(wine_todo) ok_(__FILE__, line)(mip_levels == expected_mip_levels, "Got miplevels %u, expected %u.\n",
            mip_levels, expected_mip_levels);
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
        trace("Can't unlock the readback surface, hr %#lx.\n", hr);
    IDirect3DSurface9_Release(rb->surface);
}

static void get_surface_readback(IDirect3DDevice9 *device, IDirect3DSurface9 *surface, struct surface_readback *rb)
{
    D3DSURFACE_DESC desc;
    HRESULT hr;

    hr = IDirect3DSurface9_GetDesc(surface, &desc);
    if (FAILED(hr))
    {
        trace("Failed to get surface description, hr %#lx.\n", hr);
        goto exit;
    }

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, desc.Width, desc.Height, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM,
            &rb->surface, NULL);
    if (FAILED(hr))
    {
        trace("Can't create the readback surface, hr %#lx.\n", hr);
        goto exit;
    }

    hr = D3DXLoadSurfaceFromSurface(rb->surface, NULL, NULL, surface, NULL, NULL, D3DX_FILTER_NONE, 0);
    if (FAILED(hr))
    {
        trace("Can't load the readback surface, hr %#lx.\n", hr);
        goto exit;
    }

    hr = IDirect3DSurface9_LockRect(rb->surface, &rb->locked_rect, NULL, D3DLOCK_READONLY);
    if (FAILED(hr))
        trace("Can't lock the readback surface, hr %#lx.\n", hr);

exit:
    if (FAILED(hr))
    {
        if (rb->surface)
            IDirect3DSurface9_Release(rb->surface);
        rb->surface = NULL;
    }
}

static void get_texture_surface_readback(IDirect3DDevice9 *device, IDirect3DTexture9 *texture, uint32_t mip_level,
        struct surface_readback *rb)
{
    IDirect3DSurface9 *surface;
    HRESULT hr;

    memset(rb, 0, sizeof(*rb));
    hr = IDirect3DTexture9_GetSurfaceLevel(texture, mip_level, &surface);
    if (FAILED(hr))
    {
        trace("Failed to get surface for mip level %d, hr %#lx.\n", mip_level, hr);
        return;
    }

    get_surface_readback(device, surface, rb);
    IDirect3DSurface9_Release(surface);
}

static void get_cube_texture_surface_readback(IDirect3DDevice9 *device, IDirect3DCubeTexture9 *texture, uint32_t face,
        uint32_t mip_level, struct surface_readback *rb)
{
    IDirect3DSurface9 *surface;
    HRESULT hr;

    memset(rb, 0, sizeof(*rb));
    hr = IDirect3DCubeTexture9_GetCubeMapSurface(texture, face, mip_level, &surface);
    if (FAILED(hr))
    {
        trace("Failed to get surface for face %d mip level %d, hr %#lx.\n", face, mip_level, hr);
        return;
    }

    get_surface_readback(device, surface, rb);
    IDirect3DSurface9_Release(surface);
}

#define check_readback_pixel_4bpp(rb, x, y, color, todo) _check_readback_pixel_4bpp(__LINE__, rb, x, y, color, todo)
static inline void _check_readback_pixel_4bpp(uint32_t line, struct surface_readback *rb, uint32_t x,
        uint32_t y, uint32_t expected_color, BOOL todo)
{
   uint32_t color = get_readback_color(rb, x, y);
   todo_wine_if(todo) ok_(__FILE__, line)(color == expected_color, "Got color 0x%08x, expected 0x%08x.\n", color, expected_color);
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
    unsigned int expected_size;
    D3DCAPS9 caps;
    HRESULT hr;
    IDirect3D9 *d3d;
    D3DDEVICE_CREATION_PARAMETERS params;
    D3DDISPLAYMODE mode;

    IDirect3DDevice9_GetDeviceCaps(device, &caps);

    /* general tests */
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);

    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, D3DX_DEFAULT, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);

    hr = D3DXCheckTextureRequirements(NULL, NULL, NULL, NULL, D3DX_DEFAULT, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCheckTextureRequirements returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    /* width & height */
    width = height = D3DX_DEFAULT;
    hr = D3DXCheckTextureRequirements(device, &width, &height, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(width == 256, "Returned width %d, expected %d\n", width, 256);
    ok(height == 256, "Returned height %d, expected %d\n", height, 256);

    width = D3DX_DEFAULT;
    hr = D3DXCheckTextureRequirements(device, &width, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(width == 256, "Returned width %d, expected %d\n", width, 256);

    if (caps.TextureCaps & D3DPTEXTURECAPS_POW2)
        skip("Hardware only supports pow2 textures\n");
    else
    {
        width = 62;
        hr = D3DXCheckTextureRequirements(device, &width, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
        ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
        ok(width == 62, "Returned width %d, expected %d\n", width, 62);

        width = D3DX_DEFAULT; height = 63;
        hr = D3DXCheckTextureRequirements(device, &width, &height, NULL, 0, NULL, D3DPOOL_DEFAULT);
        ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
        ok(width == height, "Returned width %d, expected %d\n", width, height);
        ok(height == 63, "Returned height %d, expected %d\n", height, 63);
    }

    width = D3DX_DEFAULT; height = 0;
    hr = D3DXCheckTextureRequirements(device, &width, &height, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(width == 1, "Returned width %d, expected %d\n", width, 1);
    ok(height == 1, "Returned height %d, expected %d\n", height, 1);

    width = 0; height = 0;
    hr = D3DXCheckTextureRequirements(device, &width, &height, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(width == 1, "Returned width %d, expected %d\n", width, 1);
    ok(height == 1, "Returned height %d, expected %d\n", height, 1);

    width = 0;
    hr = D3DXCheckTextureRequirements(device, &width, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(width == 1, "Returned width %d, expected %d\n", width, 1);

    width = 0xFFFFFFFE;
    hr = D3DXCheckTextureRequirements(device, &width, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(width == caps.MaxTextureWidth, "Returned width %d, expected %ld\n", width, caps.MaxTextureWidth);

    width = caps.MaxTextureWidth-1;
    hr = D3DXCheckTextureRequirements(device, &width, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    if (caps.TextureCaps & D3DPTEXTURECAPS_POW2)
        ok(width == caps.MaxTextureWidth, "Returned width %d, expected %ld\n", width, caps.MaxTextureWidth);
    else
        ok(width == caps.MaxTextureWidth-1, "Returned width %d, expected %ld\n", width, caps.MaxTextureWidth-1);

    /* mipmaps */
    width = 64; height = 63;
    mipmaps = 9;
    hr = D3DXCheckTextureRequirements(device, &width, &height, &mipmaps, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(mipmaps == 7, "Returned mipmaps %d, expected %d\n", mipmaps, 7);

    if (!(caps.TextureCaps & D3DPTEXTURECAPS_POW2))
    {
        width = 284; height = 137;
        mipmaps = 20;
        hr = D3DXCheckTextureRequirements(device, &width, &height, &mipmaps, 0, NULL, D3DPOOL_DEFAULT);
        ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
        ok(mipmaps == 9, "Returned mipmaps %d, expected %d\n", mipmaps, 9);

        width = height = 63;
        mipmaps = 9;
        hr = D3DXCheckTextureRequirements(device, &width, &height, &mipmaps, 0, NULL, D3DPOOL_DEFAULT);
        ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
        ok(mipmaps == 6, "Returned mipmaps %d, expected %d\n", mipmaps, 6);
    }
    else
        skip("Skipping some tests, npot2 textures unsupported\n");

    mipmaps = 20;
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, &mipmaps, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(mipmaps == 9, "Returned mipmaps %d, expected %d\n", mipmaps, 9);

    mipmaps = 0;
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, &mipmaps, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(mipmaps == 9, "Returned mipmaps %d, expected %d\n", mipmaps, 9);

    /* mipmaps when D3DUSAGE_AUTOGENMIPMAP is set */
    if (is_autogenmipmap_supported(device, D3DRTYPE_TEXTURE))
    {
        mipmaps = 0;
        hr = D3DXCheckTextureRequirements(device, NULL, NULL, &mipmaps, D3DUSAGE_AUTOGENMIPMAP, NULL, D3DPOOL_DEFAULT);
        ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
        ok(mipmaps == 0, "Returned mipmaps %d, expected %d\n", mipmaps, 0);
        mipmaps = 1;
        hr = D3DXCheckTextureRequirements(device, NULL, NULL, &mipmaps, D3DUSAGE_AUTOGENMIPMAP, NULL, D3DPOOL_DEFAULT);
        ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
        ok(mipmaps == 1, "Returned mipmaps %d, expected %d\n", mipmaps, 1);
        mipmaps = 2;
        hr = D3DXCheckTextureRequirements(device, NULL, NULL, &mipmaps, D3DUSAGE_AUTOGENMIPMAP, NULL, D3DPOOL_DEFAULT);
        ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
        ok(mipmaps == 0, "Returned mipmaps %d, expected %d\n", mipmaps, 0);
        mipmaps = 6;
        hr = D3DXCheckTextureRequirements(device, NULL, NULL, &mipmaps, D3DUSAGE_AUTOGENMIPMAP, NULL, D3DPOOL_DEFAULT);
        ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
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
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);

    format = D3DFMT_UNKNOWN;
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(format == D3DFMT_A8R8G8B8, "Returned format %u, expected %u\n", format, D3DFMT_A8R8G8B8);

    format = D3DX_DEFAULT;
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(format == D3DFMT_A8R8G8B8, "Returned format %u, expected %u\n", format, D3DFMT_A8R8G8B8);

    format = D3DFMT_R8G8B8;
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
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
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(format == expected, "Returned format %u, expected %u\n", format, expected);

    if(SUCCEEDED(IDirect3D9_CheckDeviceFormat(d3d, params.AdapterOrdinal, params.DeviceType,
                                              mode.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_A8R3G3B2)))
        expected = D3DFMT_A8R3G3B2;
    else
        expected = D3DFMT_A8R8G8B8;

    format = D3DFMT_A8R3G3B2;
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(format == expected, "Returned format %u, expected %u\n", format, expected);

    if(SUCCEEDED(IDirect3D9_CheckDeviceFormat(d3d, params.AdapterOrdinal, params.DeviceType,
                                              mode.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_P8)))
        expected = D3DFMT_P8;
    else
        expected = D3DFMT_A8R8G8B8;

    format = D3DFMT_P8;
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(format == expected, "Returned format %u, expected %u\n", format, expected);

    if(SUCCEEDED(IDirect3D9_CheckDeviceFormat(d3d, params.AdapterOrdinal, params.DeviceType,
            mode.Format, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, D3DFMT_L8)))
        expected = D3DFMT_L8;
    else
        expected = D3DFMT_X8R8G8B8;

    format = D3DFMT_L8;
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, D3DUSAGE_RENDERTARGET, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(format == expected, "Returned format %u, expected %u\n", format, expected);

    if(SUCCEEDED(IDirect3D9_CheckDeviceFormat(d3d, params.AdapterOrdinal, params.DeviceType,
            mode.Format, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, D3DFMT_L16)))
        expected = D3DFMT_L16;
    else if (SUCCEEDED(IDirect3D9_CheckDeviceFormat(d3d, params.AdapterOrdinal, params.DeviceType,
            mode.Format, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, D3DFMT_A16B16G16R16)))
        expected = D3DFMT_A16B16G16R16;
    else
        expected = D3DFMT_A2R10G10B10;

    format = D3DFMT_L16;
    hr = D3DXCheckTextureRequirements(device, NULL, NULL, NULL, D3DUSAGE_RENDERTARGET, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(format == expected, "Returned format %u, expected %u\n", format, expected);

    /* Block-based texture formats and size < block size. */
    format = D3DFMT_DXT1;
    width = 2; height = 2;
    mipmaps = 1;
    hr = D3DXCheckTextureRequirements(device, &width, &height, &mipmaps, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
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
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    ok(mipmaps == 1, "Got unexpected level count %u.\n", mipmaps);
    if (has_2d_dxt5)
    {
        ok(width == 4, "Got unexpected width %d.\n", width);
        ok(height == 4, "Got unexpected height %d.\n", height);
        ok(format == D3DFMT_DXT5, "Got unexpected format %u.\n", format);

        width = 9;
        height = 9;
        hr = D3DXCheckTextureRequirements(device, &width, &height, &mipmaps, 0, &format, D3DPOOL_DEFAULT);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
        if (caps.TextureCaps & D3DPTEXTURECAPS_POW2)
            expected_size = 16;
        else
            expected_size = 12;
        ok(width == expected_size, "Unexpected width %u, expected %u.\n", width, expected_size);
        ok(height == expected_size, "Unexpected height %u, expected %u.\n", height, expected_size);
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
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
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
    ok(hr == D3D_OK, "D3DXCheckCubeTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);

    hr = D3DXCheckCubeTextureRequirements(device, NULL, NULL, D3DX_DEFAULT, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckCubeTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);

    hr = D3DXCheckCubeTextureRequirements(NULL, NULL, NULL, D3DX_DEFAULT, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCheckCubeTextureRequirements returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    /* size */
    size = D3DX_DEFAULT;
    hr = D3DXCheckCubeTextureRequirements(device, &size, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckCubeTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(size == 256, "Returned size %d, expected %d\n", size, 256);

    /* mipmaps */
    size = 64;
    mipmaps = 9;
    hr = D3DXCheckCubeTextureRequirements(device, &size, &mipmaps, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckCubeTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(mipmaps == 7, "Returned mipmaps %d, expected %d\n", mipmaps, 7);

    size = 284;
    mipmaps = 20;
    expected = caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP_POW2 ? 10 : 9;
    expected = caps.TextureCaps & D3DPTEXTURECAPS_MIPCUBEMAP ? expected : 1;
    hr = D3DXCheckCubeTextureRequirements(device, &size, &mipmaps, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckCubeTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(mipmaps == expected, "Returned mipmaps %d, expected %d\n", mipmaps, expected);

    size = 63;
    mipmaps = 9;
    expected = caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP_POW2 ? 7 : 6;
    expected = caps.TextureCaps & D3DPTEXTURECAPS_MIPCUBEMAP ? expected : 1;
    hr = D3DXCheckCubeTextureRequirements(device, &size, &mipmaps, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckCubeTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(mipmaps == expected, "Returned mipmaps %d, expected %d\n", mipmaps, expected);

    mipmaps = 0;
    hr = D3DXCheckCubeTextureRequirements(device, NULL, &mipmaps, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckCubeTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(mipmaps == 9, "Returned mipmaps %d, expected %d\n", mipmaps, 9);

    if (is_autogenmipmap_supported(device, D3DRTYPE_CUBETEXTURE))
    {
        mipmaps = 3;
        hr = D3DXCheckCubeTextureRequirements(device, NULL,  &mipmaps, D3DUSAGE_AUTOGENMIPMAP, NULL, D3DPOOL_DEFAULT);
        ok(hr == D3D_OK, "D3DXCheckCubeTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
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
    ok(hr == D3D_OK, "D3DXCheckCubeTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);

    format = D3DFMT_UNKNOWN;
    hr = D3DXCheckCubeTextureRequirements(device, NULL, NULL, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckCubeTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(format == D3DFMT_A8R8G8B8, "Returned format %u, expected %u\n", format, D3DFMT_A8R8G8B8);

    format = D3DX_DEFAULT;
    hr = D3DXCheckCubeTextureRequirements(device, NULL, NULL, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckCubeTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(format == D3DFMT_A8R8G8B8, "Returned format %u, expected %u\n", format, D3DFMT_A8R8G8B8);

    format = D3DFMT_R8G8B8;
    hr = D3DXCheckCubeTextureRequirements(device, NULL, NULL, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckCubeTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
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
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);

    hr = D3DXCheckVolumeTextureRequirements(device, NULL, NULL, NULL, NULL, D3DX_DEFAULT, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);

    hr = D3DXCheckVolumeTextureRequirements(NULL, NULL, NULL, NULL, NULL, D3DX_DEFAULT, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCheckVolumeTextureRequirements returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    /* width, height, depth */
    width = height = depth = D3DX_DEFAULT;
    hr = D3DXCheckVolumeTextureRequirements(device, &width, &height, &depth, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(width == 256, "Returned width %d, expected %d\n", width, 256);
    ok(height == 256, "Returned height %d, expected %d\n", height, 256);
    ok(depth == 1, "Returned depth %d, expected %d\n", depth, 1);

    width = D3DX_DEFAULT;
    hr = D3DXCheckVolumeTextureRequirements(device, &width, NULL, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(width == 256, "Returned width %d, expected %d\n", width, 256);

    width = D3DX_DEFAULT; height = 0; depth = 0;
    hr = D3DXCheckVolumeTextureRequirements(device, &width, &height, &depth, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(width == 1, "Returned width %d, expected %d\n", width, 1);
    ok(height == 1, "Returned height %d, expected %d\n", height, 1);
    ok(depth == 1, "Returned height %d, expected %d\n", depth, 1);

    width = 0; height = 0; depth = 0;
    hr = D3DXCheckVolumeTextureRequirements(device, &width, &height, &depth, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(width == 1, "Returned width %d, expected %d\n", width, 1);
    ok(height == 1, "Returned height %d, expected %d\n", height, 1);
    ok(depth == 1, "Returned height %d, expected %d\n", depth, 1);

    width = 0;
    hr = D3DXCheckVolumeTextureRequirements(device, &width, NULL, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(width == 1, "Returned width %d, expected %d\n", width, 1);

    width = 0xFFFFFFFE;
    hr = D3DXCheckVolumeTextureRequirements(device, &width, NULL, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(width == caps.MaxVolumeExtent, "Returned width %d, expected %ld\n", width, caps.MaxVolumeExtent);

    /* format */
    hr = D3DXCheckVolumeTextureRequirements(device, NULL, NULL, NULL, NULL, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);

    format = D3DFMT_UNKNOWN;
    hr = D3DXCheckVolumeTextureRequirements(device, NULL, NULL, NULL, NULL, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(format == D3DFMT_A8R8G8B8, "Returned format %u, expected %u\n", format, D3DFMT_A8R8G8B8);

    format = D3DX_DEFAULT;
    hr = D3DXCheckVolumeTextureRequirements(device, NULL, NULL, NULL, NULL, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(format == D3DFMT_A8R8G8B8, "Returned format %u, expected %u\n", format, D3DFMT_A8R8G8B8);

    format = D3DFMT_R8G8B8;
    hr = D3DXCheckVolumeTextureRequirements(device, NULL, NULL, NULL, NULL, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(format == D3DFMT_X8R8G8B8, "Returned format %u, expected %u\n", format, D3DFMT_X8R8G8B8);

    format = D3DFMT_DXT3;
    hr = D3DXCheckVolumeTextureRequirements(device, NULL, NULL, NULL, NULL, 0, &format, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
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
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(mipmaps == 7, "Returned mipmaps %d, expected %d\n", mipmaps, 7);

    width = 284;
    height = 143;
    depth = 55;
    mipmaps = 20;
    expected = (caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP_POW2 && caps.MaxVolumeExtent >= 512) ? 10 : 9;
    hr = D3DXCheckVolumeTextureRequirements(device, &width, &height, &depth, &mipmaps, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
    ok(mipmaps == expected, "Returned mipmaps %d, expected %d\n", mipmaps, expected);

    mipmaps = 0;
    hr = D3DXCheckVolumeTextureRequirements(device, NULL, NULL, NULL, &mipmaps, 0, NULL, D3DPOOL_DEFAULT);
    ok(hr == D3D_OK, "D3DXCheckVolumeTextureRequirements returned %#lx, expected %#lx\n", hr, D3D_OK);
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
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateTexture returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    /* width and height tests */

    hr = D3DXCreateTexture(device, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, 0, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3D_OK, "D3DXCreateTexture returned %#lx, expected %#lx\n", hr, D3D_OK);

    if (texture)
    {
        hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &desc);
        ok(hr == D3D_OK, "GetLevelDesc returned %#lx, expected %#lx\n", hr, D3D_OK);
        ok(desc.Format == D3DFMT_A8R8G8B8, "Returned format %u, expected %u\n", desc.Format, D3DFMT_A8R8G8B8);

        ok(desc.Width == 256, "Returned width %d, expected %d\n", desc.Width, 256);
        ok(desc.Height == 256, "Returned height %d, expected %d\n", desc.Height, 256);

        IDirect3DTexture9_Release(texture);
    }


    hr = D3DXCreateTexture(device, 0, 0, 0, 0, 0, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3D_OK, "D3DXCreateTexture returned %#lx, expected %#lx\n", hr, D3D_OK);

    if (texture)
    {
        hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &desc);
        ok(hr == D3D_OK, "GetLevelDesc returned %#lx, expected %#lx\n", hr, D3D_OK);
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
           "D3DXCreateTexture returned %#lx, expected %#lx\n", hr, D3D_OK);

        if (texture)
        {
            hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &desc);
            ok(hr == D3D_OK, "GetLevelDesc returned %#lx, expected %#lx\n", hr, D3D_OK);
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
    ok(hr == D3D_OK, "D3DXCreateTexture returned %#lx, expected %#lx\n", hr, D3D_OK);

    if (texture)
    {
        mipmaps = IDirect3DTexture9_GetLevelCount(texture);
        ok(mipmaps == 7, "Returned mipmaps %d, expected %d\n", mipmaps, 7);

        IDirect3DTexture9_Release(texture);
    }


    hr = D3DXCreateTexture(device, 284, 137, 9, 0, 0, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3D_OK, "D3DXCreateTexture returned %#lx, expected %#lx\n", hr, D3D_OK);

    if (texture)
    {
        mipmaps = IDirect3DTexture9_GetLevelCount(texture);
        ok(mipmaps == 9, "Returned mipmaps %d, expected %d\n", mipmaps, 9);

        IDirect3DTexture9_Release(texture);
    }


    hr = D3DXCreateTexture(device, 0, 0, 20, 0, 0, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3D_OK, "D3DXCreateTexture returned %#lx, expected %#lx\n", hr, D3D_OK);

    if (texture)
    {
        mipmaps = IDirect3DTexture9_GetLevelCount(texture);
        ok(mipmaps == 1, "Returned mipmaps %d, expected %d\n", mipmaps, 1);

        IDirect3DTexture9_Release(texture);
    }


    hr = D3DXCreateTexture(device, 64, 64, 1, 0, 0, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3D_OK, "D3DXCreateTexture returned %#lx, expected %#lx\n", hr, D3D_OK);

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
    ok(hr == D3D_OK, "D3DXCreateTexture returned %#lx, expected %#lx\n", hr, D3D_OK);

    if (texture)
    {
        hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &desc);
        ok(hr == D3D_OK, "GetLevelDesc returned %#lx, expected %#lx\n", hr, D3D_OK);
        ok(desc.Format == D3DFMT_A8R8G8B8, "Returned format %u, expected %u\n", desc.Format, D3DFMT_A8R8G8B8);

        IDirect3DTexture9_Release(texture);
    }


    hr = D3DXCreateTexture(device, 0, 0, 0, 0, 0, D3DPOOL_DEFAULT, &texture);
    ok(hr == D3D_OK, "D3DXCreateTexture returned %#lx, expected %#lx\n", hr, D3D_OK);

    if (texture)
    {
        hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &desc);
        ok(hr == D3D_OK, "GetLevelDesc returned %#lx, expected %#lx\n", hr, D3D_OK);
        ok(desc.Format == D3DFMT_A8R8G8B8, "Returned format %u, expected %u\n", desc.Format, D3DFMT_A8R8G8B8);

        IDirect3DTexture9_Release(texture);
    }

    /* D3DXCreateTextureFromResource */
    hr = D3DXCreateTextureFromResourceA(device, NULL, MAKEINTRESOURCEA(IDB_BITMAP_1x1), &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromResource returned %#lx, expected %#lx\n", hr, D3D_OK);
    if (SUCCEEDED(hr)) IDirect3DTexture9_Release(texture);

    hr = D3DXCreateTextureFromResourceA(device, NULL, MAKEINTRESOURCEA(IDD_BITMAPDATA_1x1), &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromResource returned %#lx, expected %#lx\n", hr, D3D_OK);
    if (SUCCEEDED(hr)) IDirect3DTexture9_Release(texture);

    hr = D3DXCreateTextureFromResourceA(device, NULL, MAKEINTRESOURCEA(IDS_STRING), &texture);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXCreateTextureFromResource returned %#lx, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXCreateTextureFromResourceA(NULL, NULL, MAKEINTRESOURCEA(IDD_BITMAPDATA_1x1), &texture);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateTextureFromResource returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateTextureFromResourceA(device, NULL, NULL, &texture);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXCreateTextureFromResource returned %#lx, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXCreateTextureFromResourceA(device, NULL, MAKEINTRESOURCEA(IDD_BITMAPDATA_1x1), NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateTextureFromResource returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);


    /* D3DXCreateTextureFromResourceEx */
    hr = D3DXCreateTextureFromResourceExA(device, NULL, MAKEINTRESOURCEA(IDD_BITMAPDATA_1x1), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromResourceEx returned %#lx, expected %#lx\n", hr, D3D_OK);
    if (SUCCEEDED(hr)) IDirect3DTexture9_Release(texture);

    hr = D3DXCreateTextureFromResourceExA(device, NULL, MAKEINTRESOURCEA(IDS_STRING), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXCreateTextureFromResourceEx returned %#lx, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXCreateTextureFromResourceExA(NULL, NULL, MAKEINTRESOURCEA(IDD_BITMAPDATA_1x1), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateTextureFromResourceEx returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateTextureFromResourceExA(device, NULL, NULL, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXCreateTextureFromResourceEx returned %#lx, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    hr = D3DXCreateTextureFromResourceExA(device, NULL, MAKEINTRESOURCEA(IDD_BITMAPDATA_1x1), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateTextureFromResourceEx returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);
}

static void test_D3DXFilterTexture(IDirect3DDevice9 *device)
{
    IDirect3DTexture9 *tex;
    IDirect3DCubeTexture9 *cubetex;
    IDirect3DVolumeTexture9 *voltex;
    HRESULT hr;
    uint32_t i;

    hr = IDirect3DDevice9_CreateTexture(device, 256, 256, 5, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex, NULL);

    if (SUCCEEDED(hr))
    {
        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) tex, NULL, D3DX_DEFAULT, D3DX_FILTER_NONE);
        ok(hr == D3D_OK, "D3DXFilterTexture returned %#lx, expected %#lx\n", hr, D3D_OK);

        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) tex, NULL, 0, D3DX_FILTER_NONE);
        ok(hr == D3D_OK, "D3DXFilterTexture returned %#lx, expected %#lx\n", hr, D3D_OK);

        for (i = 0; i < ARRAY_SIZE(test_filter_values); ++i)
        {
            winetest_push_context("Filter %d (%#x)", i, test_filter_values[i].filter);

            hr = D3DXFilterTexture((IDirect3DBaseTexture9 *)tex, NULL, 0, test_filter_values[i].filter);
            ok(hr == test_filter_values[i].expected_hr, "Unexpected hr %#lx.\n", hr);

            winetest_pop_context();
        }

        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) tex, NULL, 0, D3DX_FILTER_BOX + 1); /* Invalid filter */
        ok(hr == D3DERR_INVALIDCALL, "D3DXFilterTexture returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) tex, NULL, 5, D3DX_FILTER_NONE); /* Invalid miplevel */
        ok(hr == D3DERR_INVALIDCALL, "D3DXFilterTexture returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

        IDirect3DTexture9_Release(tex);
    }
    else
        skip("Failed to create texture\n");

    hr = D3DXFilterTexture(NULL, NULL, 0, D3DX_FILTER_NONE);
    ok(hr == D3DERR_INVALIDCALL, "D3DXFilterTexture returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    /* Test different pools */
    hr = IDirect3DDevice9_CreateTexture(device, 256, 256, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &tex, NULL);

    if (SUCCEEDED(hr))
    {
        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) tex, NULL, 0, D3DX_FILTER_NONE);
        ok(hr == D3D_OK, "D3DXFilterTexture returned %#lx, expected %#lx\n", hr, D3D_OK);
        IDirect3DTexture9_Release(tex);
    }
    else
        skip("Failed to create texture\n");

    hr = IDirect3DDevice9_CreateTexture(device, 256, 256, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &tex, NULL);

    if (SUCCEEDED(hr))
    {
        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) tex, NULL, 0, D3DX_FILTER_NONE);
        ok(hr == D3D_OK, "D3DXFilterTexture returned %#lx, expected %#lx\n", hr, D3D_OK);
        IDirect3DTexture9_Release(tex);
    }
    else
        skip("Failed to create texture\n");

    hr = IDirect3DDevice9_CreateTexture(device, 256, 256, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex, NULL);
    if (SUCCEEDED(hr))
    {
        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) tex, NULL, 0, D3DX_FILTER_POINT);
        ok(hr == D3D_OK, "D3DXFilterTexture returned %#lx, expected %#lx\n", hr, D3D_OK);
        IDirect3DTexture9_Release(tex);
    }
    else
        skip("Failed to create texture\n");

    hr = IDirect3DDevice9_CreateTexture(device, 256, 256, 0, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex, NULL);
    if (SUCCEEDED(hr))
    {
        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) tex, NULL, 0, D3DX_FILTER_POINT);
        ok(hr == D3D_OK, "D3DXFilterTexture returned %#lx, expected %#lx\n", hr, D3D_OK);
        IDirect3DTexture9_Release(tex);
    }
    else
        skip("Failed to create texture\n");

    /* Cube texture test */
    hr = IDirect3DDevice9_CreateCubeTexture(device, 256, 5, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &cubetex, NULL);

    if (SUCCEEDED(hr))
    {
        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) cubetex, NULL, 0, D3DX_FILTER_NONE);
        ok(hr == D3D_OK, "D3DXFilterTexture returned %#lx, expected %#lx\n", hr, D3D_OK);

        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) cubetex, NULL, 0, D3DX_FILTER_BOX + 1); /* Invalid filter */
        ok(hr == D3DERR_INVALIDCALL, "D3DXFilterTexture returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

        for (i = 0; i < ARRAY_SIZE(test_filter_values); ++i)
        {
            winetest_push_context("Filter %d (%#x)", i, test_filter_values[i].filter);

            hr = D3DXFilterTexture((IDirect3DBaseTexture9 *)cubetex, NULL, 0, test_filter_values[i].filter);
            ok(hr == test_filter_values[i].expected_hr, "Unexpected hr %#lx.\n", hr);

            winetest_pop_context();
        }

        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) cubetex, NULL, 5, D3DX_FILTER_NONE); /* Invalid miplevel */
        ok(hr == D3DERR_INVALIDCALL, "D3DXFilterTexture returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);
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
        ok(hr == D3D_OK, "D3DXFilterTexture returned %#lx, expected %#lx\n", hr, D3D_OK);

        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) voltex, NULL, 0, D3DX_DEFAULT);
        ok(hr == D3D_OK, "D3DXFilterTexture returned %#lx, expected %#lx\n", hr, D3D_OK);

        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) voltex, NULL, 0, D3DX_FILTER_BOX);
        ok(hr == D3D_OK, "D3DXFilterTexture returned %#lx, expected %#lx\n", hr, D3D_OK);

        for (i = 0; i < ARRAY_SIZE(test_filter_values); ++i)
        {
            winetest_push_context("Filter %d (%#x)", i, test_filter_values[i].filter);

            hr = D3DXFilterTexture((IDirect3DBaseTexture9 *)voltex, NULL, 0, test_filter_values[i].filter);
            ok(hr == test_filter_values[i].expected_hr, "Unexpected hr %#lx.\n", hr);

            winetest_pop_context();
        }

        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) voltex, NULL, level_count - 1, D3DX_DEFAULT);
        ok(hr == D3D_OK, "D3DXFilterTexture returned %#lx, expected %#lx\n", hr, D3D_OK);

        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) voltex, NULL, level_count, D3DX_DEFAULT);
        ok(hr == D3DERR_INVALIDCALL, "D3DXFilterTexture returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

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
        ok(hr == D3D_OK, "D3dXFilteTexture returned %#lx, expected %#lx\n", hr, D3D_OK);
        IDirect3DTexture9_Release(tex);
    }
    else
        skip("Failed to create texture\n");

    hr = IDirect3DDevice9_CreateTexture(device, 256, 256, 1, D3DUSAGE_DYNAMIC | D3DUSAGE_AUTOGENMIPMAP, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex, NULL);
    if (SUCCEEDED(hr))
    {
        hr = D3DXFilterTexture((IDirect3DBaseTexture9*) tex, NULL, 0, D3DX_FILTER_NONE);
        ok(hr == D3D_OK, "D3dXFilteTexture returned %#lx, expected %#lx\n", hr, D3D_OK);
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
        ok(hr == D3D_OK, "Unexpected hr %#lx, i %u.\n", hr, i);

        hr = D3DXFillTexture(tex, fillfunc, NULL);
        ok(hr == D3D_OK, "Unexpected hr %#lx, i %u.\n", hr, i);

        for (m = 0; m < 3; m++)
        {
            IDirect3DSurface9 *src_surface, *temp_surface;

            hr = IDirect3DTexture9_GetSurfaceLevel(tex, m, &src_surface);
            ok(hr == D3D_OK, "Unexpected hr %#lx, i %u, m %lu.\n", hr, i, m);
            temp_surface = src_surface;

            if (FAILED(hr = IDirect3DSurface9_LockRect(src_surface, &lock_rect, NULL, D3DLOCK_READONLY)))
            {
                hr = IDirect3DDevice9_CreateRenderTarget(device, size, size,
                        D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &temp_surface, NULL);
                ok(hr == D3D_OK, "Unexpected hr %#lx, i %u, m %lu.\n", hr, i, m);
                hr = IDirect3DDevice9_StretchRect(device, src_surface, NULL, temp_surface, NULL, D3DTEXF_NONE);
                ok(hr == D3D_OK, "Unexpected hr %#lx, i %u, m %lu.\n", hr, i, m);
                hr = IDirect3DSurface9_LockRect(temp_surface, &lock_rect, NULL, D3DLOCK_READONLY);
                ok(hr == D3D_OK, "Unexpected hr %#lx, i %u, m %lu.\n", hr, i, m);
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
                            "Texel at (%lu, %lu) doesn't match: %#lx, expected %#lx, i %u, m %lu.\n",
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
        todo_wine ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
        IDirect3DTexture9_Release(tex);
    }

    hr = IDirect3DDevice9_CreateTexture(device, 256, 256, 1, D3DUSAGE_DEPTHSTENCIL,
            D3DFMT_D16, D3DPOOL_DEFAULT, &tex, NULL);
    if (hr == D3D_OK)
    {
        hr = D3DXFillTexture(tex, fillfunc, NULL);
        ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);
        IDirect3DTexture9_Release(tex);
    }

    hr = IDirect3DDevice9_CreateTexture(device, 4, 4, 1, 0, D3DFMT_A1R5G5B5,
                                        D3DPOOL_MANAGED, &tex, NULL);

    if (SUCCEEDED(hr))
    {
        hr = D3DXFillTexture(tex, fillfunc, NULL);
        ok(hr == D3D_OK, "D3DXFillTexture returned %#lx, expected %#lx\n", hr, D3D_OK);

        hr = IDirect3DTexture9_LockRect(tex, 0, &lock_rect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "Couldn't lock the texture, error %#lx\n", hr);
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
                       "Texel at (%lu, %lu) doesn't match: %#lx, expected %#lx\n",
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
        ok(hr == D3D_OK, "D3DXFillTexture returned %#lx, expected %#lx\n", hr, D3D_OK);

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
        ok(hr == D3D_OK, "D3DXFillTexture returned %#lx, expected %#lx\n", hr, D3D_OK);

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
        todo_wine ok(hr == D3D_OK, "D3DXFillTexture returned %#lx, expected %#lx\n", hr, D3D_OK);

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

static void WINAPI fillfunc_cube_coord(D3DXVECTOR4 *value, const D3DXVECTOR3 *texcoord,
        const D3DXVECTOR3 *texelsize, void *data)
{
    value->x = texcoord->x;
    value->y = texcoord->y;
    value->z = texcoord->z;
    value->w = 1.0f;
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

static DWORD get_argb_color(D3DFORMAT format, DWORD x, DWORD y, const D3DLOCKED_RECT *lock_rect)
{
    DWORD value, ret;
    int pitch;

    switch (format)
    {
    case D3DFMT_A8R8G8B8:
        pitch = lock_rect->Pitch / sizeof(DWORD);
        return ((DWORD *)lock_rect->pBits)[y * pitch + x];
    case D3DFMT_A1R5G5B5:
        pitch = lock_rect->Pitch / sizeof(WORD);
        value = ((WORD *)lock_rect->pBits)[y * pitch + x];

        ret = (value >> 15 & 0x1) << 24
                | (value >> 10 & 0x1f) << 16
                | (value >> 5 & 0x1f) << 8
                | (value & 0x1f);

        return ret;

    default:
        return 0;
    }
}

static BYTE get_s8_clipped(float v)
{
    return (BYTE)(v >= 0.0f ? v * 255 + 0.5f : 0.0f);
}

static DWORD get_expected_argb_color(D3DFORMAT format, const D3DXVECTOR4 *v)
{
    switch (format)
    {
    case D3DFMT_A8R8G8B8:
        return get_s8_clipped(v->w) << 24
                | get_s8_clipped(v->x) << 16
                | get_s8_clipped(v->y) << 8
                | get_s8_clipped(v->z);

    case D3DFMT_A1R5G5B5:
        return (BYTE)(v->w + 0.5f) << 24
                | (BYTE)(v->x * 31 + 0.5f) << 16
                | (BYTE)(v->y * 31 + 0.5f) << 8
                | (BYTE)(v->z * 31 + 0.5f);
    default:
        return 0;
    }
}

#define compare_cube_texture(t,f,d) compare_cube_texture_(t,f,d,__LINE__)
static void compare_cube_texture_(IDirect3DCubeTexture9 *texture,
        LPD3DXFILL3D func, BYTE diff, unsigned int line)
{
    static const enum cube_coord coordmap[6][3] =
    {
        {ONE, YCOORDINV, XCOORDINV},
        {ZERO, YCOORDINV, XCOORD},
        {XCOORD, ONE, YCOORD},
        {XCOORD, ZERO, YCOORDINV},
        {XCOORD, YCOORDINV, ONE},
        {XCOORDINV, YCOORDINV, ZERO}
    };

    DWORD x, y, m, f, levels, size, value, expected;
    D3DXVECTOR3 coord, texelsize;
    D3DLOCKED_RECT lock_rect;
    D3DSURFACE_DESC desc;
    D3DXVECTOR4 out;
    HRESULT hr;

    levels = IDirect3DCubeTexture9_GetLevelCount(texture);

    for (m = 0; m < levels; ++m)
    {
        hr = IDirect3DCubeTexture9_GetLevelDesc(texture, m, &desc);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

        size = desc.Width;

        for (f = 0; f < 6; f++)
        {
            texelsize.x = (f == 0) || (f == 1) ? 0.0f : 2.0f / size;
            texelsize.y = (f == 2) || (f == 3) ? 0.0f : 2.0f / size;
            texelsize.z = (f == 4) || (f == 5) ? 0.0f : 2.0f / size;

            hr = IDirect3DCubeTexture9_LockRect(texture, f, m, &lock_rect, NULL, D3DLOCK_READONLY);
            ok(hr == D3D_OK, "Couldn't lock the texture, error %#lx.\n", hr);

            for (y = 0; y < size; y++)
            {
                for (x = 0; x < size; x++)
                {
                    coord.x = get_cube_coord(coordmap[f][0], x, y, size) / size * 2.0f - 1.0f;
                    coord.y = get_cube_coord(coordmap[f][1], x, y, size) / size * 2.0f - 1.0f;
                    coord.z = get_cube_coord(coordmap[f][2], x, y, size) / size * 2.0f - 1.0f;

                    func(&out, &coord, &texelsize, NULL);

                    value = get_argb_color(desc.Format, x, y, &lock_rect);
                    expected = get_expected_argb_color(desc.Format, &out);

                    ok_(__FILE__, line)(compare_color(value, expected, diff),
                            "Texel at face %lu (%lu, %lu) doesn't match: %08lx, expected %08lx.\n",
                            f, x, y, value, expected);
                }
            }
            IDirect3DCubeTexture9_UnlockRect(texture, f, m);
        }
    }
}

static void test_D3DXFillCubeTexture(IDirect3DDevice9 *device)
{
    IDirect3DCubeTexture9 *tex;
    HRESULT hr;

    /* A8R8G8B8 */
    hr = IDirect3DDevice9_CreateCubeTexture(device, 4, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex, NULL);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

    hr = D3DXFillCubeTexture(tex, fillfunc_cube, NULL);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
    compare_cube_texture(tex, fillfunc_cube, 1);

    hr = D3DXFillCubeTexture(tex, fillfunc_cube_coord, NULL);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
    compare_cube_texture(tex, fillfunc_cube_coord, 1);

    IDirect3DCubeTexture9_Release(tex);

    /* A1R5G5B5 */
    hr = IDirect3DDevice9_CreateCubeTexture(device, 4, 1, 0, D3DFMT_A1R5G5B5,
            D3DPOOL_MANAGED, &tex, NULL);
    if (SUCCEEDED(hr))
    {
        hr = D3DXFillCubeTexture(tex, fillfunc_cube, NULL);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
        compare_cube_texture(tex, fillfunc_cube, 2);
        IDirect3DCubeTexture9_Release(tex);
    }
    else
    {
        skip("Texture format D3DFMT_A1R5G5B5 unsupported.\n");
    }
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
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
    hr = D3DXFillVolumeTexture(tex, fillfunc_volume, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);
    IDirect3DVolumeTexture9_Release(tex);

    hr = IDirect3DDevice9_CreateVolumeTexture(device, size, size, size, 0, 0, D3DFMT_A8R8G8B8,
            D3DPOOL_MANAGED, &tex, NULL);
    if (SUCCEEDED(hr))
    {
        hr = D3DXFillVolumeTexture(tex, fillfunc_volume, NULL);
        ok(hr == D3D_OK, "D3DXFillVolumeTexture returned %#lx, expected %#lx\n", hr, D3D_OK);

        for (m = 0; m < 3; m++)
        {
            hr = IDirect3DVolumeTexture9_LockBox(tex, m, &lock_box, NULL, D3DLOCK_READONLY);
            ok(hr == D3D_OK, "Couldn't lock the texture, error %#lx\n", hr);
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
                               "Texel at (%lu, %lu, %lu) doesn't match: %#lx, expected %#lx\n",
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
        ok(hr == D3D_OK, "D3DXFillTexture returned %#lx, expected %#lx\n", hr, D3D_OK);
        hr = IDirect3DVolumeTexture9_LockBox(tex, 0, &lock_box, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "Couldn't lock the texture, error %#lx\n", hr);
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
                           "Texel at (%lu, %lu, %lu) doesn't match: %#lx, expected %#lx\n",
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
    static const uint32_t dds_volume_dxt3_4_4_4_expected_uncompressed[] =
    {
        0xffff0000, 0xff00ff00, 0xff0000ff,
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
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);
    if (SUCCEEDED(hr)) IDirect3DTexture9_Release(texture);

    hr = D3DXCreateTextureFromFileInMemory(device, dds_24bit, sizeof(dds_24bit), &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);
    if (SUCCEEDED(hr)) IDirect3DTexture9_Release(texture);

    hr = D3DXCreateTextureFromFileInMemory(device, dds_24bit, sizeof(dds_24bit) - 1, &texture);
    ok(hr == D3DXERR_INVALIDDATA, "D3DXCreateTextureFromFileInMemory returned %#lx, expected %#x\n", hr, D3DXERR_INVALIDDATA);

    /* Check that D3DXCreateTextureFromFileInMemory accepts cube texture dds file (only first face texture is loaded) */
    hr = D3DXCreateTextureFromFileInMemory(device, dds_cube_map, sizeof(dds_cube_map), &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemory returned %#lx, expected %#lx.\n", hr, D3D_OK);
    type = IDirect3DTexture9_GetType(texture);
    ok(type == D3DRTYPE_TEXTURE, "IDirect3DTexture9_GetType returned %u, expected %u.\n", type, D3DRTYPE_TEXTURE);
    hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &desc);
    ok(hr == D3D_OK, "IDirect3DTexture9_GetLevelDesc returned %#lx, expected %#lx.\n", hr, D3D_OK);
    ok(desc.Width == 4, "Width is %u, expected 4.\n", desc.Width);
    ok(desc.Height == 4, "Height is %u, expected 4.\n", desc.Height);
    if (has_cube_dxt5)
    {
        ok(desc.Format == D3DFMT_DXT5, "Unexpected texture format %#x.\n", desc.Format);
        hr = IDirect3DTexture9_LockRect(texture, 0, &lock_rect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "IDirect3DTexture9_LockRect returned %#lx, expected %#lx\n", hr, D3D_OK);
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
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr) && has_2d_dxt5)
    {
        type = IDirect3DTexture9_GetType(texture);
        ok(type == D3DRTYPE_TEXTURE, "Got unexpected type %u.\n", type);
        hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &desc);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
        ok(desc.Width == 4, "Got unexpected width %u.\n", desc.Width);
        ok(desc.Height == 4, "Got unexpected height %u.\n", desc.Height);

        IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
        hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 4, 4, D3DFMT_A8R8G8B8,
                D3DPOOL_DEFAULT, &uncompressed_surface, NULL);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
        hr = D3DXLoadSurfaceFromSurface(uncompressed_surface, NULL, NULL, surface, NULL, NULL, D3DX_FILTER_NONE, 0);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
        hr = IDirect3DSurface9_LockRect(uncompressed_surface, &lock_rect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
        for (y = 0; y < 4; ++y)
        {
            for (x = 0; x < 4; ++x)
            {
                /* Use a large tolerance, decompression + stretching +
                 * compression + decompression again introduce quite a bit of
                 * precision loss. */
                ok(compare_color(((DWORD *)lock_rect.pBits)[lock_rect.Pitch / 4 * y + x],
                        dds_dxt5_expected[y * 4 + x], 32),
                        "Color at position %u, %u is 0x%08lx, expected 0x%08lx.\n",
                        x, y, ((DWORD *)lock_rect.pBits)[lock_rect.Pitch / 4 * y + x],
                        dds_dxt5_expected[y * 4 + x]);
            }
        }
        hr = IDirect3DSurface9_UnlockRect(uncompressed_surface);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

        IDirect3DSurface9_Release(uncompressed_surface);
        IDirect3DSurface9_Release(surface);
    }
    if (SUCCEEDED(hr))
        IDirect3DTexture9_Release(texture);

    /* Test with a larger DXT5 texture. */
    hr = D3DXCreateTextureFromFileInMemory(device, dds_dxt5_8_8, sizeof(dds_dxt5_8_8), &texture);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    type = IDirect3DTexture9_GetType(texture);
    ok(type == D3DRTYPE_TEXTURE, "Got unexpected type %u.\n", type);
    hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &desc);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    ok(desc.Width == 8, "Got unexpected width %u.\n", desc.Width);
    ok(desc.Height == 8, "Got unexpected height %u.\n", desc.Height);
    IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 8, 8, D3DFMT_A8R8G8B8,
            D3DPOOL_DEFAULT, &uncompressed_surface, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = D3DXLoadSurfaceFromSurface(uncompressed_surface, NULL, NULL, surface, NULL, NULL, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IDirect3DSurface9_LockRect(uncompressed_surface, &lock_rect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
        for (y = 0; y < 8; ++y)
        {
            for (x = 0; x < 8; ++x)
            {
                ok(compare_color(((DWORD *)lock_rect.pBits)[lock_rect.Pitch / 4 * y + x],
                        dds_dxt5_8_8_expected[y * 8 + x], 0),
                        "Color at position %u, %u is 0x%08lx, expected 0x%08lx.\n",
                        x, y, ((DWORD *)lock_rect.pBits)[lock_rect.Pitch / 4 * y + x],
                        dds_dxt5_8_8_expected[y * 8 + x]);
            }
        }
        hr = IDirect3DSurface9_UnlockRect(uncompressed_surface);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    }

    hr = IDirect3DSurface9_LockRect(surface, &lock_rect, NULL, D3DLOCK_READONLY);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    for (y = 0; y < 2; ++y)
        memset(&((BYTE *)lock_rect.pBits)[y * lock_rect.Pitch], 0, 16 * 2);
    hr = IDirect3DSurface9_UnlockRect(surface);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    SetRect(&rect, 2, 2, 6, 6);
    hr = D3DXLoadSurfaceFromMemory(surface, NULL, NULL, &dds_dxt5_8_8[128],
            D3DFMT_DXT5, 16 * 2, NULL, &rect, D3DX_FILTER_POINT, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = D3DXLoadSurfaceFromSurface(uncompressed_surface, NULL, NULL, surface, NULL, NULL, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DSurface9_LockRect(uncompressed_surface, &lock_rect, NULL, D3DLOCK_READONLY);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    for (y = 0; y < 8; ++y)
    {
        for (x = 0; x < 8; ++x)
        {
            ok(compare_color(((DWORD *)lock_rect.pBits)[lock_rect.Pitch / 4 * y + x],
                    dds_dxt5_8_8_expected_misaligned_1[y * 8 + x], 0),
                    "Color at position %u, %u is 0x%08lx, expected 0x%08lx.\n",
                    x, y, ((DWORD *)lock_rect.pBits)[lock_rect.Pitch / 4 * y + x],
                    dds_dxt5_8_8_expected_misaligned_1[y * 8 + x]);
        }
    }
    hr = IDirect3DSurface9_UnlockRect(uncompressed_surface);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DSurface9_LockRect(surface, &lock_rect, NULL, D3DLOCK_READONLY);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    for (y = 0; y < 2; ++y)
        memset(&((BYTE *)lock_rect.pBits)[y * lock_rect.Pitch], 0, 16 * 2);
    hr = IDirect3DSurface9_UnlockRect(surface);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = D3DXLoadSurfaceFromMemory(surface, NULL, &rect, &dds_dxt5_8_8[128],
            D3DFMT_DXT5, 16 * 2, NULL, NULL, D3DX_FILTER_POINT, 0);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DXLoadSurfaceFromMemory(surface, NULL, &rect, &dds_dxt5_8_8[128],
            D3DFMT_DXT5, 16 * 2, NULL, &rect, D3DX_FILTER_POINT, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = D3DXLoadSurfaceFromSurface(uncompressed_surface, NULL, NULL, surface, NULL, NULL, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DSurface9_LockRect(uncompressed_surface, &lock_rect, NULL, D3DLOCK_READONLY);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    for (y = 0; y < 8; ++y)
    {
        for (x = 0; x < 8; ++x)
        {
            ok(compare_color(((DWORD *)lock_rect.pBits)[lock_rect.Pitch / 4 * y + x],
                    dds_dxt5_8_8_expected_misaligned_3[y * 8 + x], 0),
                    "Color at position %u, %u is 0x%08lx, expected 0x%08lx.\n",
                    x, y, ((DWORD *)lock_rect.pBits)[lock_rect.Pitch / 4 * y + x],
                    dds_dxt5_8_8_expected_misaligned_3[y * 8 + x]);
        }
    }
    hr = IDirect3DSurface9_UnlockRect(uncompressed_surface);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = D3DXLoadSurfaceFromFileInMemory(surface, NULL, &rect, dds_dxt5_8_8,
            sizeof(dds_dxt5_8_8), &rect, D3DX_FILTER_POINT, 0, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = D3DXLoadSurfaceFromSurface(uncompressed_surface, NULL, NULL, surface, NULL, NULL, D3DX_FILTER_NONE, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DSurface9_LockRect(uncompressed_surface, &lock_rect, NULL, D3DLOCK_READONLY);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    for (y = 0; y < 8; ++y)
    {
        for (x = 0; x < 8; ++x)
        {
            ok(compare_color(((DWORD *)lock_rect.pBits)[lock_rect.Pitch / 4 * y + x],
                    dds_dxt5_8_8_expected_misaligned_3[y * 8 + x], 0),
                    "Color at position %u, %u is 0x%08lx, expected 0x%08lx.\n",
                    x, y, ((DWORD *)lock_rect.pBits)[lock_rect.Pitch / 4 * y + x],
                    dds_dxt5_8_8_expected_misaligned_3[y * 8 + x]);
        }
    }
    hr = IDirect3DSurface9_UnlockRect(uncompressed_surface);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    IDirect3DSurface9_Release(uncompressed_surface);
    IDirect3DSurface9_Release(surface);
    IDirect3DTexture9_Release(texture);

    /* Volume textures work too. */
    hr = D3DXCreateTextureFromFileInMemory(device, dds_volume_map, sizeof(dds_volume_map), &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemory returned %#lx, expected %#lx.\n", hr, D3D_OK);
    type = IDirect3DTexture9_GetType(texture);
    ok(type == D3DRTYPE_TEXTURE, "IDirect3DTexture9_GetType returned %u, expected %u.\n", type, D3DRTYPE_TEXTURE);
    level_count = IDirect3DBaseTexture9_GetLevelCount((IDirect3DBaseTexture9 *)texture);
    ok(level_count == 3, "Texture has %lu mip levels, 3 expected.\n", level_count);
    hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &desc);
    ok(hr == D3D_OK, "IDirect3DTexture9_GetLevelDesc returned %#lx, expected %#lx.\n", hr, D3D_OK);
    ok(desc.Width == 4, "Width is %u, expected 4.\n", desc.Width);
    ok(desc.Height == 4, "Height is %u, expected 4.\n", desc.Height);

    if (has_2d_dxt3)
    {
        ok(desc.Format == D3DFMT_DXT3, "Unexpected texture format %#x.\n", desc.Format);
        hr = IDirect3DTexture9_LockRect(texture, 0, &lock_rect, NULL, D3DLOCK_READONLY);
        ok(hr == D3D_OK, "IDirect3DTexture9_LockRect returned %#lx, expected %#lx.\n", hr, D3D_OK);
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

    IDirect3DTexture9_Release(texture);

    /*
     * All mip levels are pulled from the texture file, even in the case of a
     * volume texture file.
     */
    hr = D3DXCreateTextureFromFileInMemory(device, dds_volume_dxt3_4_4_4, sizeof(dds_volume_dxt3_4_4_4), &texture);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
    check_texture_mip_levels(texture, 3, FALSE);
    if (has_2d_dxt3)
    {
        struct surface_readback surface_rb;
        uint32_t mip_level;

        check_texture_level_desc(texture, 0, D3DFMT_DXT3, 0, D3DPOOL_MANAGED, 0, 0, 4, 4, FALSE);
        check_texture_level_desc(texture, 1, D3DFMT_DXT3, 0, D3DPOOL_MANAGED, 0, 0, 2, 2, FALSE);
        check_texture_level_desc(texture, 2, D3DFMT_DXT3, 0, D3DPOOL_MANAGED, 0, 0, 1, 1, FALSE);
        for (mip_level = 0; mip_level < ARRAY_SIZE(dds_volume_dxt3_4_4_4_expected_uncompressed); ++mip_level)
        {
            const uint32_t expected_color = dds_volume_dxt3_4_4_4_expected_uncompressed[mip_level];
            uint32_t x, y;

            IDirect3DTexture9_GetLevelDesc(texture, mip_level, &desc);
            get_texture_surface_readback(device, texture, mip_level, &surface_rb);
            for (y = 0; y < desc.Height; ++y)
            {
                for (x = 0; x < desc.Width; ++x)
                {
                    check_readback_pixel_4bpp(&surface_rb, x, y, expected_color, FALSE);
                }
            }
            release_surface_readback(&surface_rb);
        }
    }
    else
    {
        check_texture_level_desc(texture, 0, D3DFMT_A8R8G8B8, 0, D3DPOOL_MANAGED, 0, 0, 4, 4, FALSE);
        skip("D3DFMT_DXT3 textures are not supported, skipping a test.\n");
    }

    IDirect3DTexture9_Release(texture);
}

static void test_D3DXCreateTextureFromFileInMemoryEx(IDirect3DDevice9 *device)
{
    static const uint32_t dds_24bit_8_8_expected[] =
    {
        0xffff0000, 0xff00ff00, 0xff0000ff, 0xff000000,
    };
    static const uint32_t dds_volume_24bit_4_4_4_expected[] =
    {
        0xffff0000, 0xff00ff00, 0xff0000ff,
    };
    static const uint32_t dds_volume_dxt3_4_4_4_expected_uncompressed[] =
    {
        0xffff0000, 0xff00ff00, 0xff0000ff,
    };
    HRESULT hr;
    struct surface_readback surface_rb;
    uint32_t miplevels, mip_level, i;
    IDirect3DTexture9 *texture;
    IDirect3DSurface9 *surface;
    D3DXIMAGE_INFO img_info;
    D3DSURFACE_DESC desc;

    hr = D3DXCreateTextureFromFileInMemoryEx(device, dds_16bit, sizeof(dds_16bit), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT,
        0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemoryEx returned %#lx, expected %#lx\n", hr, D3D_OK);
    IDirect3DTexture9_Release(texture);

    for (i = 0; i < ARRAY_SIZE(test_filter_values); ++i)
    {
        winetest_push_context("Filter %d (%#x)", i, test_filter_values[i].filter);

        texture = NULL;
        hr = D3DXCreateTextureFromFileInMemoryEx(device, dds_16bit, sizeof(dds_16bit), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT,
            0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, test_filter_values[i].filter, D3DX_DEFAULT, 0, NULL, NULL, &texture);
        ok(hr == test_filter_values[i].expected_hr, "Unexpected hr %#lx.\n", hr);
        if (texture)
            IDirect3DTexture9_Release(texture);

        winetest_pop_context();
    }

    /* Mip filter argument values never cause failure. */
    for (i = 0; i < ARRAY_SIZE(test_filter_values); ++i)
    {
        winetest_push_context("Mip filter %d (%#x)", i, test_filter_values[i].filter);

        texture = NULL;
        hr = D3DXCreateTextureFromFileInMemoryEx(device, dds_24bit, sizeof(dds_24bit), 32, 32, 6,
            0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, test_filter_values[i].filter, 0, NULL, NULL, &texture);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
        if (texture)
            IDirect3DTexture9_Release(texture);

        winetest_pop_context();
    }

    /* Mip skip bits are 30-26, 25-23 are unused, setting them does nothing. */
    texture = NULL;
    hr = D3DXCreateTextureFromFileInMemoryEx(device, dds_24bit, sizeof(dds_24bit), 32, 32, 6,
        0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, 0x03800001, 0, NULL, NULL, &texture);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
    if (texture)
        IDirect3DTexture9_Release(texture);

    hr = D3DXCreateTextureFromFileInMemoryEx(device, dds_16bit, sizeof(dds_16bit), D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT,
        D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemoryEx returned %#lx, expected %#lx\n", hr, D3D_OK);
    IDirect3DTexture9_Release(texture);

    hr = D3DXCreateTextureFromFileInMemoryEx(device, dds_24bit, sizeof(dds_24bit), D3DX_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_SKIP_DDS_MIP_LEVELS(1, D3DX_FILTER_POINT), 0, NULL, NULL, &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemoryEx returned %#lx, expected %#lx\n", hr, D3D_OK);
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
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemoryEx returned %#lx, expected %#lx\n", hr, D3D_OK);
    IDirect3DTexture9_Release(texture);

    /* Checking for color key format overrides. */
    hr = D3DXCreateTextureFromFileInMemoryEx(device, dds_16bit, sizeof(dds_16bit),
            D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemoryEx returned %#lx, expected %#lx.\n", hr, D3D_OK);
    IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    IDirect3DSurface9_GetDesc(surface, &desc);
    ok(desc.Format == D3DFMT_X1R5G5B5, "Returned format %u, expected %u.\n", desc.Format, D3DFMT_X1R5G5B5);
    IDirect3DSurface9_Release(surface);
    IDirect3DTexture9_Release(texture);
    hr = D3DXCreateTextureFromFileInMemoryEx(device, dds_16bit, sizeof(dds_16bit),
            D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, 0xff000000, NULL, NULL, &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemoryEx returned %#lx, expected %#lx.\n", hr, D3D_OK);
    IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    IDirect3DSurface9_GetDesc(surface, &desc);
    ok(desc.Format == D3DFMT_A1R5G5B5, "Returned format %u, expected %u.\n", desc.Format, D3DFMT_A1R5G5B5);
    IDirect3DSurface9_Release(surface);
    IDirect3DTexture9_Release(texture);
    hr = D3DXCreateTextureFromFileInMemoryEx(device, dds_16bit, sizeof(dds_16bit),
            D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_X1R5G5B5, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, 0xff000000, NULL, NULL, &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemoryEx returned %#lx, expected %#lx.\n", hr, D3D_OK);
    IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    IDirect3DSurface9_GetDesc(surface, &desc);
    ok(desc.Format == D3DFMT_X1R5G5B5, "Returned format %u, expected %u.\n", desc.Format, D3DFMT_X1R5G5B5);
    IDirect3DSurface9_Release(surface);
    IDirect3DTexture9_Release(texture);

    hr = D3DXCreateTextureFromFileInMemoryEx(device, dds_24bit, sizeof(dds_24bit),
            D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemoryEx returned %#lx, expected %#lx.\n", hr, D3D_OK);
    IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    IDirect3DSurface9_GetDesc(surface, &desc);
    ok(desc.Format == D3DFMT_X8R8G8B8, "Returned format %u, expected %u.\n", desc.Format, D3DFMT_X8R8G8B8);
    IDirect3DSurface9_Release(surface);
    IDirect3DTexture9_Release(texture);
    hr = D3DXCreateTextureFromFileInMemoryEx(device, dds_24bit, sizeof(dds_24bit),
            D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, 0xff000000, NULL, NULL, &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemoryEx returned %#lx, expected %#lx.\n", hr, D3D_OK);
    IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    IDirect3DSurface9_GetDesc(surface, &desc);
    ok(desc.Format == D3DFMT_A8R8G8B8, "Returned format %u, expected %u.\n", desc.Format, D3DFMT_A8R8G8B8);
    IDirect3DSurface9_Release(surface);
    IDirect3DTexture9_Release(texture);
    hr = D3DXCreateTextureFromFileInMemoryEx(device, dds_24bit, sizeof(dds_24bit),
            D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, 0xff000000, NULL, NULL, &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemoryEx returned %#lx, expected %#lx.\n", hr, D3D_OK);
    IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    IDirect3DSurface9_GetDesc(surface, &desc);
    ok(desc.Format == D3DFMT_X8R8G8B8, "Returned format %u, expected %u.\n", desc.Format, D3DFMT_X8R8G8B8);
    IDirect3DSurface9_Release(surface);
    IDirect3DTexture9_Release(texture);

    hr = D3DXCreateTextureFromFileInMemoryEx(device, png_grayscale, sizeof(png_grayscale),
            D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemoryEx returned %#lx, expected %#lx.\n", hr, D3D_OK);
    IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    IDirect3DSurface9_GetDesc(surface, &desc);
    ok(desc.Format == D3DFMT_L8, "Returned format %u, expected %u.\n", desc.Format, D3DFMT_L8);
    IDirect3DSurface9_Release(surface);
    IDirect3DTexture9_Release(texture);
    hr = D3DXCreateTextureFromFileInMemoryEx(device, png_grayscale, sizeof(png_grayscale),
            D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, 0xff000000, NULL, NULL, &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemoryEx returned %#lx, expected %#lx.\n", hr, D3D_OK);
    IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    IDirect3DSurface9_GetDesc(surface, &desc);
    ok(desc.Format == D3DFMT_A8L8, "Returned format %u, expected %u.\n", desc.Format, D3DFMT_A8L8);
    IDirect3DSurface9_Release(surface);
    IDirect3DTexture9_Release(texture);
    hr = D3DXCreateTextureFromFileInMemoryEx(device, png_grayscale, sizeof(png_grayscale),
            D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, D3DFMT_L8, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, 0xff000000, NULL, NULL, &texture);
    ok(hr == D3D_OK, "D3DXCreateTextureFromFileInMemoryEx returned %#lx, expected %#lx.\n", hr, D3D_OK);
    IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    IDirect3DSurface9_GetDesc(surface, &desc);
    ok(desc.Format == D3DFMT_L8, "Returned format %u, expected %u.\n", desc.Format, D3DFMT_L8);
    IDirect3DSurface9_Release(surface);
    IDirect3DTexture9_Release(texture);

    /* Test values returned in the D3DXIMAGE_INFO structure. */
    hr = D3DXCreateTextureFromFileInMemoryEx(device, dds_24bit, sizeof(dds_24bit), D3DX_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_SKIP_DDS_MIP_LEVELS(0, D3DX_FILTER_POINT), 0, &img_info, NULL, &texture);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

    check_texture_mip_levels(texture, 2, FALSE);
    check_image_info(&img_info, 2, 2, 1, 2, D3DFMT_R8G8B8, D3DRTYPE_TEXTURE, D3DXIFF_DDS, FALSE);
    check_texture_level_desc(texture, 0, D3DFMT_X8R8G8B8, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 0, 0, 2, 2, FALSE);
    check_texture_level_desc(texture, 1, D3DFMT_X8R8G8B8, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 0, 0, 1, 1, FALSE);

    IDirect3DTexture9_Release(texture);

    /* Skip level bits are bits 30-26. Bit 31 needs to be ignored. */
    hr = D3DXCreateTextureFromFileInMemoryEx(device, dds_24bit, sizeof(dds_24bit), D3DX_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_FILTER_POINT | (0x20u << D3DX_SKIP_DDS_MIP_LEVELS_SHIFT), 0, &img_info, NULL, &texture);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

    check_image_info(&img_info, 2, 2, 1, 2, D3DFMT_R8G8B8, D3DRTYPE_TEXTURE, D3DXIFF_DDS, FALSE);
    IDirect3DTexture9_Release(texture);

    /*
     * The values returned in the D3DXIMAGE_INFO structure represent the mip
     * level the texture data was retrieved from, i.e if we skip the first mip
     * level, we will get the values of the second mip level.
     */
    hr = D3DXCreateTextureFromFileInMemoryEx(device, dds_24bit, sizeof(dds_24bit), D3DX_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_SKIP_DDS_MIP_LEVELS(1, D3DX_FILTER_POINT), 0, &img_info, NULL, &texture);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

    check_texture_mip_levels(texture, 1, FALSE);
    check_image_info(&img_info, 1, 1, 1, 1, D3DFMT_R8G8B8, D3DRTYPE_TEXTURE, D3DXIFF_DDS, FALSE);
    check_texture_level_desc(texture, 0, D3DFMT_X8R8G8B8, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 0, 0, 1, 1, FALSE);

    IDirect3DTexture9_Release(texture);

    /*
     * Request skipping 3 mip levels in a file that only has 2 mip levels. In this
     * case, it stops at the final mip level.
     */
    hr = D3DXCreateTextureFromFileInMemoryEx(device, dds_24bit, sizeof(dds_24bit), D3DX_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_SKIP_DDS_MIP_LEVELS(3, D3DX_FILTER_POINT), 0, &img_info, NULL, &texture);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

    check_texture_mip_levels(texture, 1, FALSE);
    check_image_info(&img_info, 1, 1, 1, 1, D3DFMT_R8G8B8, D3DRTYPE_TEXTURE, D3DXIFF_DDS, FALSE);
    check_texture_level_desc(texture, 0, D3DFMT_X8R8G8B8, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 0, 0, 1, 1, FALSE);

    IDirect3DTexture9_Release(texture);

    /*
     * Load multiple mip levels from a file and check the resulting pixel
     * values.
     */
    hr = D3DXCreateTextureFromFileInMemoryEx(device, dds_24bit_8_8, sizeof(dds_24bit_8_8),
            D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, 0, &img_info, NULL, &texture);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

    check_texture_mip_levels(texture, 4, FALSE);
    check_image_info(&img_info, 8, 8, 1, 4, D3DFMT_R8G8B8, D3DRTYPE_TEXTURE, D3DXIFF_DDS, FALSE);
    check_texture_level_desc(texture, 0, D3DFMT_X8R8G8B8, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 0, 0, 8, 8, FALSE);
    check_texture_level_desc(texture, 1, D3DFMT_X8R8G8B8, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 0, 0, 4, 4, FALSE);
    check_texture_level_desc(texture, 2, D3DFMT_X8R8G8B8, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 0, 0, 2, 2, FALSE);
    check_texture_level_desc(texture, 3, D3DFMT_X8R8G8B8, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 0, 0, 1, 1, FALSE);

    for (mip_level = 0; mip_level < ARRAY_SIZE(dds_24bit_8_8_expected); ++mip_level)
    {
        const uint32_t expected_color = dds_24bit_8_8_expected[mip_level];
        uint32_t x, y;

        IDirect3DTexture9_GetLevelDesc(texture, mip_level, &desc);
        get_texture_surface_readback(device, texture, mip_level, &surface_rb);
        for (y = 0; y < desc.Height; ++y)
        {
            for (x = 0; x < desc.Width; ++x)
            {
                check_readback_pixel_4bpp(&surface_rb, x, y, expected_color, FALSE);
            }
        }
        release_surface_readback(&surface_rb);
    }

    IDirect3DTexture9_Release(texture);

    /* Volume DDS with mips into regular texture tests. */
    hr = D3DXCreateTextureFromFileInMemoryEx(device, dds_volume_24bit_4_4_4, sizeof(dds_volume_24bit_4_4_4),
            D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, 0, &img_info, NULL, &texture);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

    check_texture_mip_levels(texture, 3, FALSE);
    check_image_info(&img_info, 4, 4, 4, 3, D3DFMT_R8G8B8, D3DRTYPE_VOLUMETEXTURE, D3DXIFF_DDS, FALSE);
    check_texture_level_desc(texture, 0, D3DFMT_X8R8G8B8, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 0, 0, 4, 4, FALSE);
    check_texture_level_desc(texture, 1, D3DFMT_X8R8G8B8, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 0, 0, 2, 2, FALSE);
    check_texture_level_desc(texture, 2, D3DFMT_X8R8G8B8, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 0, 0, 1, 1, FALSE);

    for (mip_level = 0; mip_level < ARRAY_SIZE(dds_volume_24bit_4_4_4_expected); ++mip_level)
    {
        const uint32_t expected_color = dds_volume_24bit_4_4_4_expected[mip_level];
        uint32_t x, y;

        IDirect3DTexture9_GetLevelDesc(texture, mip_level, &desc);
        get_texture_surface_readback(device, texture, mip_level, &surface_rb);
        for (y = 0; y < desc.Height; ++y)
        {
            for (x = 0; x < desc.Width; ++x)
            {
                check_readback_pixel_4bpp(&surface_rb, x, y, expected_color, FALSE);
            }
        }
        release_surface_readback(&surface_rb);
    }

    IDirect3DTexture9_Release(texture);

    /* DXT3 volume DDS with mips into a regular texture. */
    if (has_2d_dxt3)
    {
        hr = D3DXCreateTextureFromFileInMemoryEx(device, dds_volume_dxt3_4_4_4, sizeof(dds_volume_dxt3_4_4_4),
                D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
                D3DX_DEFAULT, D3DX_DEFAULT, 0, &img_info, NULL, &texture);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

        check_texture_mip_levels(texture, 3, FALSE);
        check_image_info(&img_info, 4, 4, 4, 3, D3DFMT_DXT3, D3DRTYPE_VOLUMETEXTURE, D3DXIFF_DDS, FALSE);
        check_texture_level_desc(texture, 0, D3DFMT_DXT3, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 0, 0, 4, 4, FALSE);
        check_texture_level_desc(texture, 1, D3DFMT_DXT3, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 0, 0, 2, 2, FALSE);
        check_texture_level_desc(texture, 2, D3DFMT_DXT3, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 0, 0, 1, 1, FALSE);

        for (mip_level = 0; mip_level < ARRAY_SIZE(dds_volume_dxt3_4_4_4_expected_uncompressed); ++mip_level)
        {
            const uint32_t expected_color = dds_volume_dxt3_4_4_4_expected_uncompressed[mip_level];
            uint32_t x, y;

            IDirect3DTexture9_GetLevelDesc(texture, mip_level, &desc);
            get_texture_surface_readback(device, texture, mip_level, &surface_rb);
            for (y = 0; y < desc.Height; ++y)
            {
                for (x = 0; x < desc.Width; ++x)
                {
                    check_readback_pixel_4bpp(&surface_rb, x, y, expected_color, FALSE);
                }
            }
            release_surface_readback(&surface_rb);
        }

        IDirect3DTexture9_Release(texture);
    }
    else
    {
        skip("D3DFMT_DXT3 textures are not supported, skipping tests.\n");
    }

    /* Create texture from JPG. */
    hr = D3DXCreateTextureFromFileInMemoryEx(device, jpg_rgb_2_2, sizeof(jpg_rgb_2_2), D3DX_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, 0, &img_info, NULL, &texture);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

    check_texture_mip_levels(texture, 2, FALSE);
    check_image_info(&img_info, 2, 2, 1, 1, D3DFMT_X8R8G8B8, D3DRTYPE_TEXTURE, D3DXIFF_JPG, FALSE);
    check_texture_level_desc(texture, 0, D3DFMT_X8R8G8B8, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 0, 0, 2, 2, FALSE);
    check_texture_level_desc(texture, 1, D3DFMT_X8R8G8B8, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 0, 0, 1, 1, FALSE);

    IDirect3DTexture9_Release(texture);

    /* Create texture from PNG. */
    hr = D3DXCreateTextureFromFileInMemoryEx(device, png_2_2_48bpp_rgb, sizeof(png_2_2_48bpp_rgb), D3DX_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, 0, &img_info, NULL, &texture);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
    check_texture_mip_levels(texture, 2, FALSE);
    check_image_info(&img_info, 2, 2, 1, 1, D3DFMT_A16B16G16R16, D3DRTYPE_TEXTURE, D3DXIFF_PNG, FALSE);
    check_texture_level_desc(texture, 0, D3DFMT_A16B16G16R16, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 0, 0, 2, 2, FALSE);
    check_texture_level_desc(texture, 1, D3DFMT_A16B16G16R16, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 0, 0, 1, 1, FALSE);
    IDirect3DTexture9_Release(texture);

    hr = D3DXCreateTextureFromFileInMemoryEx(device, png_2_2_64bpp_rgba, sizeof(png_2_2_64bpp_rgba), D3DX_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, 0, &img_info, NULL, &texture);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

    check_texture_mip_levels(texture, 2, FALSE);
    check_image_info(&img_info, 2, 2, 1, 1, D3DFMT_A16B16G16R16, D3DRTYPE_TEXTURE, D3DXIFF_PNG, FALSE);
    check_texture_level_desc(texture, 0, D3DFMT_A16B16G16R16, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 0, 0, 2, 2, FALSE);
    check_texture_level_desc(texture, 1, D3DFMT_A16B16G16R16, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 0, 0, 1, 1, FALSE);
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
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateCubeTextureFromFileInMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateCubeTextureFromFileInMemory(device, NULL, sizeof(dds_cube_map), &cube_texture);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateCubeTextureFromFileInMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateCubeTextureFromFileInMemory(device, dds_cube_map, 0, &cube_texture);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateCubeTextureFromFileInMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateCubeTextureFromFileInMemory(device, dds_cube_map, sizeof(dds_cube_map), NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateCubeTextureFromFileInMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateCubeTextureFromFileInMemory(device, dds_24bit, sizeof(dds_24bit), &cube_texture);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    hr = D3DXCreateCubeTextureFromFileInMemory(device, bmp_32bpp_4_4_argb, sizeof(bmp_32bpp_4_4_argb), &cube_texture);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    hr = D3DXCreateCubeTextureFromFileInMemory(device, dds_cube_map, sizeof(dds_cube_map), &cube_texture);
    if (SUCCEEDED(hr))
    {
        levelcount = IDirect3DCubeTexture9_GetLevelCount(cube_texture);
        ok(levelcount == 3, "GetLevelCount returned %lu, expected 3\n", levelcount);

        hr = IDirect3DCubeTexture9_GetLevelDesc(cube_texture, 0, &surface_desc);
        ok(hr == D3D_OK, "GetLevelDesc returned %#lx, expected %#lx\n", hr, D3D_OK);
        ok(surface_desc.Width == 4, "Got width %u, expected 4\n", surface_desc.Width);
        ok(surface_desc.Height == 4, "Got height %u, expected 4\n", surface_desc.Height);

        ref = IDirect3DCubeTexture9_Release(cube_texture);
        ok(ref == 0, "Invalid reference count. Got %lu, expected 0\n", ref);
    } else skip("Couldn't create cube texture\n");
}

static void test_D3DXCreateCubeTextureFromFileInMemoryEx(IDirect3DDevice9 *device)
{
    static const uint32_t dds_cube_map_non_square_expected[] =
    {
        0xffff0000, 0xff00ff00, 0xff0000ff, 0xffffff00, 0xffff00ff, 0xff000000,
    };
    static const uint32_t dds_cube_map_4_4_expected[] =
    {
        0xffff0000, 0xff00ff00, 0xff0000ff, 0xffffff00, 0xffff00ff, 0xff00ffff,
        0xffffffff, 0xff000000, 0xff800000, 0xff008000, 0xff000080, 0xff808000
    };
    IDirect3DCubeTexture9 *cube_texture;
    struct surface_readback surface_rb;
    D3DSURFACE_DESC desc;
    D3DXIMAGE_INFO info;
    uint32_t i, x, y;
    HRESULT hr;

    hr = D3DXCreateCubeTextureFromFileInMemoryEx(device, dds_cube_map, sizeof(dds_cube_map), D3DX_DEFAULT,
            D3DX_DEFAULT, D3DUSAGE_RENDERTARGET, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT,
            D3DX_DEFAULT, 0, NULL, NULL, &cube_texture);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    IDirect3DCubeTexture9_Release(cube_texture);

    hr = D3DXCreateCubeTextureFromFileInMemoryEx(device, dds_cube_map, sizeof(dds_cube_map), D3DX_DEFAULT,
            D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &cube_texture);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    IDirect3DCubeTexture9_Release(cube_texture);
    for (i = 0; i < ARRAY_SIZE(test_filter_values); ++i)
    {
        winetest_push_context("Filter %d (%#x)", i, test_filter_values[i].filter);

        cube_texture = NULL;
        hr = D3DXCreateCubeTextureFromFileInMemoryEx(device, dds_cube_map, sizeof(dds_cube_map), D3DX_DEFAULT,
                D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, test_filter_values[i].filter, D3DX_DEFAULT,
                0, NULL, NULL, &cube_texture);
        ok(hr == test_filter_values[i].expected_hr, "Unexpected hr %#lx.\n", hr);
        if (cube_texture)
            IDirect3DCubeTexture9_Release(cube_texture);

        winetest_pop_context();
    }

    /* Mip filter argument values never cause failure. */
    for (i = 0; i < ARRAY_SIZE(test_filter_values); ++i)
    {
        winetest_push_context("Mip filter %d (%#x)", i, test_filter_values[i].filter);

        cube_texture = NULL;
        hr = D3DXCreateCubeTextureFromFileInMemoryEx(device, dds_cube_map, sizeof(dds_cube_map), D3DX_DEFAULT,
                D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, test_filter_values[i].filter,
                0, NULL, NULL, &cube_texture);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
        if (cube_texture)
            IDirect3DCubeTexture9_Release(cube_texture);

        winetest_pop_context();
    }

    /* Mip skip bits are 30-26, 25-23 are unused, setting them does nothing. */
    cube_texture = NULL;
    hr = D3DXCreateCubeTextureFromFileInMemoryEx(device, dds_cube_map, sizeof(dds_cube_map), D3DX_DEFAULT,
            D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, 0x03800001,
            0, NULL, NULL, &cube_texture);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
    if (cube_texture)
        IDirect3DCubeTexture9_Release(cube_texture);

    if (!is_autogenmipmap_supported(device, D3DRTYPE_CUBETEXTURE))
    {
        skip("No D3DUSAGE_AUTOGENMIPMAP support for cube textures\n");
        return;
    }

    hr = D3DXCreateCubeTextureFromFileInMemoryEx(device, dds_cube_map, sizeof(dds_cube_map), D3DX_DEFAULT,
            D3DX_DEFAULT, D3DUSAGE_DYNAMIC | D3DUSAGE_AUTOGENMIPMAP, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &cube_texture);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    IDirect3DCubeTexture9_Release(cube_texture);

    /*
     * Cubemap file with a width of 4 and a height of 2. The largest value is
     * used for the size of the created cubemap.
     */
    hr = D3DXCreateCubeTextureFromFileInMemoryEx(device, dds_cube_map_4_2, sizeof(dds_cube_map_4_2), D3DX_DEFAULT, 1,
            D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_FILTER_NONE, D3DX_DEFAULT, 0, &info, NULL, &cube_texture);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

    check_texture_mip_levels(cube_texture, 1, FALSE);
    check_image_info(&info, 4, 2, 1, 1, D3DFMT_X8R8G8B8, D3DRTYPE_CUBETEXTURE, D3DXIFF_DDS, FALSE);
    check_cube_texture_level_desc(cube_texture, 0, D3DFMT_X8R8G8B8, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 0, 0, 4, FALSE);

    IDirect3DCubeTexture9_GetLevelDesc(cube_texture, 0, &desc);
    for (i = 0; i < 6; ++i)
    {
        const uint32_t expected_color = dds_cube_map_non_square_expected[i];

        winetest_push_context("Face %u", i);
        get_cube_texture_surface_readback(device, cube_texture, i, 0, &surface_rb);
        for (y = 0; y < desc.Height; ++y)
        {
            for (x = 0; x < desc.Width; ++x)
            {
                if (y < info.Height)
                    check_readback_pixel_4bpp(&surface_rb, x, y, expected_color, FALSE);
                else
                    check_readback_pixel_4bpp(&surface_rb, x, y, 0xff000000, FALSE);
            }
        }
        release_surface_readback(&surface_rb);
        winetest_pop_context();
    }
    IDirect3DCubeTexture9_Release(cube_texture);

    /*
     * Load the same cubemap, but this time with a point filter. Source image
     * is scaled to cover the entire 4x4 cubemap texture faces.
     */
    hr = D3DXCreateCubeTextureFromFileInMemoryEx(device, dds_cube_map_4_2, sizeof(dds_cube_map_4_2), D3DX_DEFAULT, 1,
            D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_FILTER_POINT, D3DX_DEFAULT, 0, &info, NULL, &cube_texture);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

    check_texture_mip_levels(cube_texture, 1, FALSE);
    check_image_info(&info, 4, 2, 1, 1, D3DFMT_X8R8G8B8, D3DRTYPE_CUBETEXTURE, D3DXIFF_DDS, FALSE);
    check_cube_texture_level_desc(cube_texture, 0, D3DFMT_X8R8G8B8, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 0, 0, 4, FALSE);

    IDirect3DCubeTexture9_GetLevelDesc(cube_texture, 0, &desc);
    for (i = 0; i < 6; ++i)
    {
        const uint32_t expected_color = dds_cube_map_non_square_expected[i];

        winetest_push_context("Face %u", i);
        get_cube_texture_surface_readback(device, cube_texture, i, 0, &surface_rb);
        for (y = 0; y < desc.Height; ++y)
        {
            for (x = 0; x < desc.Width; ++x)
            {
                check_readback_pixel_4bpp(&surface_rb, x, y, expected_color, FALSE);
            }
        }
        release_surface_readback(&surface_rb);
        winetest_pop_context();
    }
    IDirect3DCubeTexture9_Release(cube_texture);

    /*
     * Cubemap file with a width of 2 and a height of 4.
     */
    hr = D3DXCreateCubeTextureFromFileInMemoryEx(device, dds_cube_map_2_4, sizeof(dds_cube_map_2_4), D3DX_DEFAULT, 1,
            D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_FILTER_NONE, D3DX_DEFAULT, 0, &info, NULL, &cube_texture);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

    check_texture_mip_levels(cube_texture, 1, FALSE);
    check_image_info(&info, 2, 4, 1, 1, D3DFMT_X8R8G8B8, D3DRTYPE_CUBETEXTURE, D3DXIFF_DDS, FALSE);
    check_cube_texture_level_desc(cube_texture, 0, D3DFMT_X8R8G8B8, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 0, 0, 4, FALSE);

    IDirect3DCubeTexture9_GetLevelDesc(cube_texture, 0, &desc);
    for (i = 0; i < 6; ++i)
    {
        const uint32_t expected_color = dds_cube_map_non_square_expected[i];

        winetest_push_context("Face %u", i);
        get_cube_texture_surface_readback(device, cube_texture, i, 0, &surface_rb);
        for (y = 0; y < desc.Height; ++y)
        {
            for (x = 0; x < desc.Width; ++x)
            {
                if (x < info.Width)
                    check_readback_pixel_4bpp(&surface_rb, x, y, expected_color, FALSE);
                else
                    check_readback_pixel_4bpp(&surface_rb, x, y, 0xff000000, FALSE);
            }
        }
        release_surface_readback(&surface_rb);
        winetest_pop_context();
    }
    IDirect3DCubeTexture9_Release(cube_texture);

    /* Multi-mip cubemap DDS file. */
    hr = D3DXCreateCubeTextureFromFileInMemoryEx(device, dds_cube_map_4_4, sizeof(dds_cube_map_4_4), D3DX_DEFAULT, D3DX_FROM_FILE,
            D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, &info, NULL, &cube_texture);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

    check_texture_mip_levels(cube_texture, 2, FALSE);
    check_image_info(&info, 4, 4, 1, 2, D3DFMT_X8R8G8B8, D3DRTYPE_CUBETEXTURE, D3DXIFF_DDS, FALSE);
    check_cube_texture_level_desc(cube_texture, 0, D3DFMT_X8R8G8B8, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 0, 0, 4, FALSE);
    check_cube_texture_level_desc(cube_texture, 1, D3DFMT_X8R8G8B8, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 0, 0, 2, FALSE);

    for (i = 0; i < 6; ++i)
    {
        uint32_t mip_level;

        for (mip_level = 0; mip_level < 2; ++mip_level)
        {
            const uint32_t expected_color = dds_cube_map_4_4_expected[(i * 2) + mip_level];

            winetest_push_context("Face %u, mip level %u", i, mip_level);
            IDirect3DCubeTexture9_GetLevelDesc(cube_texture, mip_level, &desc);
            get_cube_texture_surface_readback(device, cube_texture, i, mip_level, &surface_rb);
            for (y = 0; y < desc.Height; ++y)
            {
                for (x = 0; x < desc.Width; ++x)
                {
                    check_readback_pixel_4bpp(&surface_rb, x, y, expected_color, FALSE);
                }
            }
            release_surface_readback(&surface_rb);
            winetest_pop_context();
        }
    }
    IDirect3DCubeTexture9_Release(cube_texture);

    /* Skip level bits are bits 30-26. Bit 31 needs to be ignored. */
    hr = D3DXCreateCubeTextureFromFileInMemoryEx(device, dds_cube_map_4_4, sizeof(dds_cube_map_4_4), D3DX_DEFAULT,
            D3DX_FROM_FILE, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT,
            D3DX_FILTER_POINT | (0x20u << D3DX_SKIP_DDS_MIP_LEVELS_SHIFT), 0, &info, NULL, &cube_texture);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

    check_image_info(&info, 4, 4, 1, 2, D3DFMT_X8R8G8B8, D3DRTYPE_CUBETEXTURE, D3DXIFF_DDS, FALSE);
    IDirect3DCubeTexture9_Release(cube_texture);

    /* Multi-mip cubemap DDS file with mip skipping. */
    hr = D3DXCreateCubeTextureFromFileInMemoryEx(device, dds_cube_map_4_4, sizeof(dds_cube_map_4_4), D3DX_DEFAULT,
            D3DX_FROM_FILE, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT,
            D3DX_SKIP_DDS_MIP_LEVELS(1, D3DX_DEFAULT), 0, &info, NULL, &cube_texture);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    check_texture_mip_levels(cube_texture, 1, FALSE);
    check_image_info(&info, 2, 2, 1, 1, D3DFMT_X8R8G8B8, D3DRTYPE_CUBETEXTURE, D3DXIFF_DDS, FALSE);
    check_cube_texture_level_desc(cube_texture, 0, D3DFMT_X8R8G8B8, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 0, 0, 2, FALSE);

    for (i = 0; i < 6; ++i)
    {
        const uint32_t expected_color = dds_cube_map_4_4_expected[(i * 2) + 1];

        winetest_push_context("Face %u", i);
        IDirect3DCubeTexture9_GetLevelDesc(cube_texture, 0, &desc);
        get_cube_texture_surface_readback(device, cube_texture, i, 0, &surface_rb);
        for (y = 0; y < desc.Height; ++y)
        {
            for (x = 0; x < desc.Width; ++x)
            {
                check_readback_pixel_4bpp(&surface_rb, x, y, expected_color, FALSE);
            }
        }
        release_surface_readback(&surface_rb);
        winetest_pop_context();
    }
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
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateVolumeTextureFromFileInMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateVolumeTextureFromFileInMemory(device, NULL, sizeof(dds_volume_map), &volume_texture);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateVolumeTextureFromFileInMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateVolumeTextureFromFileInMemory(device, dds_volume_map, 0, &volume_texture);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateVolumeTextureFromFileInMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateVolumeTextureFromFileInMemory(device, dds_volume_map, sizeof(dds_volume_map), NULL);
    ok(hr == D3DERR_INVALIDCALL, "D3DXCreateVolumeTextureFromFileInMemory returned %#lx, expected %#lx\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateVolumeTextureFromFileInMemory(device, dds_volume_map, sizeof(dds_volume_map), &volume_texture);
    ok(hr == D3D_OK, "D3DXCreateVolumeTextureFromFileInMemory returned %#lx, expected %#lx.\n", hr, D3D_OK);
    levelcount = IDirect3DVolumeTexture9_GetLevelCount(volume_texture);
    ok(levelcount == 3, "GetLevelCount returned %lu, expected 3.\n", levelcount);

    hr = IDirect3DVolumeTexture9_GetLevelDesc(volume_texture, 0, &volume_desc);
    ok(hr == D3D_OK, "GetLevelDesc returned %#lx, expected %#lx.\n", hr, D3D_OK);
    ok(volume_desc.Width == 4, "Got width %u, expected 4.\n", volume_desc.Width);
    ok(volume_desc.Height == 4, "Got height %u, expected 4.\n", volume_desc.Height);
    ok(volume_desc.Depth == 2, "Got depth %u, expected 2.\n", volume_desc.Depth);
    ok(volume_desc.Pool == D3DPOOL_MANAGED, "Got pool %u, expected D3DPOOL_MANAGED.\n", volume_desc.Pool);

    hr = IDirect3DVolumeTexture9_GetLevelDesc(volume_texture, 1, &volume_desc);
    ok(hr == D3D_OK, "GetLevelDesc returned %#lx, expected %#lx.\n", hr, D3D_OK);
    ok(volume_desc.Width == 2, "Got width %u, expected 2.\n", volume_desc.Width);
    ok(volume_desc.Height == 2, "Got height %u, expected 2.\n", volume_desc.Height);
    ok(volume_desc.Depth == 1, "Got depth %u, expected 1.\n", volume_desc.Depth);

    ref = IDirect3DVolumeTexture9_Release(volume_texture);
    ok(ref == 0, "Invalid reference count. Got %lu, expected 0.\n", ref);
}

static void test_D3DXCreateVolumeTextureFromFileInMemoryEx(IDirect3DDevice9 *device)
{
    static const uint32_t dds_volume_dxt3_4_4_4_expected_uncompressed[] =
    {
        0xffff0000, 0xff00ff00, 0xff0000ff,
    };
    static const uint32_t dds_24bit_8_8_expected[] =
    {
        0xffff0000, 0xff00ff00, 0xff0000ff, 0xff000000,
    };
    static const uint32_t bmp_32bpp_4_4_argb_expected[] =
    {
        0xffff0000, 0xff00ff00, 0xff0000ff, 0xff000000,
    };
    IDirect3DVolumeTexture9 *volume_texture, *texture;
    struct volume_readback volume_rb;
    uint32_t mip_level, x, y, z, i;
    D3DXIMAGE_INFO img_info;
    D3DVOLUME_DESC desc;
    HRESULT hr;

    hr = D3DXCreateVolumeTextureFromFileInMemoryEx(device, dds_volume_map, sizeof(dds_volume_map), D3DX_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, 1, D3DUSAGE_RENDERTARGET, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT,
            D3DX_DEFAULT, 0, NULL, NULL, &volume_texture);
    ok(hr == D3DERR_NOTAVAILABLE, "Got unexpected hr %#lx.\n", hr);

    hr = D3DXCreateVolumeTextureFromFileInMemoryEx(device, dds_volume_map, sizeof(dds_volume_map), D3DX_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, 1, D3DUSAGE_DEPTHSTENCIL, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, D3DX_DEFAULT,
            D3DX_DEFAULT, 0, NULL, NULL, &volume_texture);
    ok(hr == D3DERR_NOTAVAILABLE, "Got unexpected hr %#lx.\n", hr);

    /* Skip level bits are bits 30-26. Bit 31 needs to be ignored. */
    hr = D3DXCreateVolumeTextureFromFileInMemoryEx(device, dds_24bit_8_8, sizeof(dds_24bit_8_8),
            D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_FILTER_POINT | (0x20u << D3DX_SKIP_DDS_MIP_LEVELS_SHIFT), 0, &img_info, NULL, &texture);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

    check_image_info(&img_info, 8, 8, 1, 4, D3DFMT_R8G8B8, D3DRTYPE_TEXTURE, D3DXIFF_DDS, FALSE);
    IDirect3DVolumeTexture9_Release(texture);

    for (i = 0; i < ARRAY_SIZE(test_filter_values); ++i)
    {
        winetest_push_context("Filter %d (%#x)", i, test_filter_values[i].filter);

        texture = NULL;
        hr = D3DXCreateVolumeTextureFromFileInMemoryEx(device, dds_24bit_8_8, sizeof(dds_24bit_8_8),
                D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
                test_filter_values[i].filter, D3DX_DEFAULT, 0, NULL, NULL, &texture);
        ok(hr == test_filter_values[i].expected_hr, "Unexpected hr %#lx.\n", hr);
        if (texture)
            IDirect3DVolumeTexture9_Release(texture);

        winetest_pop_context();
    }

    /* Mip filter argument values never cause failure. */
    for (i = 0; i < ARRAY_SIZE(test_filter_values); ++i)
    {
        winetest_push_context("Mip filter %d (%#x)", i, test_filter_values[i].filter);

        texture = NULL;
        hr = D3DXCreateVolumeTextureFromFileInMemoryEx(device, dds_24bit_8_8, sizeof(dds_24bit_8_8),
                D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
                D3DX_DEFAULT, test_filter_values[i].filter, 0, NULL, NULL, &texture);
        ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
        if (texture)
            IDirect3DVolumeTexture9_Release(texture);

        winetest_pop_context();
    }

    /* Mip skip bits are 30-26, 25-23 are unused, setting them does nothing. */
    texture = NULL;
    hr = D3DXCreateVolumeTextureFromFileInMemoryEx(device, dds_24bit_8_8, sizeof(dds_24bit_8_8),
            D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, 0x03800001, 0, NULL, NULL, &texture);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
    if (texture)
        IDirect3DVolumeTexture9_Release(texture);

    hr = D3DXCreateVolumeTextureFromFileInMemoryEx(device, dds_24bit_8_8, sizeof(dds_24bit_8_8),
            D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_SKIP_DDS_MIP_LEVELS(1, D3DX_FILTER_POINT), 0, &img_info, NULL, &texture);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    check_image_info(&img_info, 4, 4, 1, 3, D3DFMT_R8G8B8, D3DRTYPE_TEXTURE, D3DXIFF_DDS, FALSE);
    IDirect3DVolumeTexture9_Release(texture);

    /* Load a 2D texture DDS file with multiple mips into a volume texture. */
    hr = D3DXCreateVolumeTextureFromFileInMemoryEx(device, dds_24bit_8_8, sizeof(dds_24bit_8_8),
            D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_DEFAULT, 0, &img_info, NULL, &texture);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    check_texture_mip_levels(texture, 4, FALSE);
    check_image_info(&img_info, 8, 8, 1, 4, D3DFMT_R8G8B8, D3DRTYPE_TEXTURE, D3DXIFF_DDS, FALSE);
    check_volume_texture_level_desc(texture, 0, D3DFMT_X8R8G8B8, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 8, 8, 1, FALSE);
    check_volume_texture_level_desc(texture, 1, D3DFMT_X8R8G8B8, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 4, 4, 1, FALSE);
    check_volume_texture_level_desc(texture, 2, D3DFMT_X8R8G8B8, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 2, 2, 1, FALSE);
    check_volume_texture_level_desc(texture, 3, D3DFMT_X8R8G8B8, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 1, 1, 1, FALSE);

    for (mip_level = 0; mip_level < 4; ++mip_level)
    {
        const uint32_t expected_color = dds_24bit_8_8_expected[mip_level];
        uint32_t x, y, z;

        IDirect3DVolumeTexture9_GetLevelDesc(texture, mip_level, &desc);
        get_texture_volume_readback(device, texture, mip_level, &volume_rb);
        for (z = 0; z < desc.Depth; ++z)
        {
            for (y = 0; y < desc.Height; ++y)
            {
                for (x = 0; x < desc.Width; ++x)
                {
                    check_volume_readback_pixel_4bpp(&volume_rb, x, y, z, expected_color, FALSE);
                }
            }
        }
        release_volume_readback(&volume_rb);
    }
    IDirect3DVolumeTexture9_Release(texture);

    /* Multi-mip compressed volume texture file. */
    if (has_3d_dxt3)
    {
        hr = D3DXCreateVolumeTextureFromFileInMemoryEx(device, dds_volume_dxt3_4_4_4, sizeof(dds_volume_dxt3_4_4_4),
                D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN,
                D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, &img_info, NULL, &texture);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
        check_texture_mip_levels(texture, 3, FALSE);
        check_image_info(&img_info, 4, 4, 4, 3, D3DFMT_DXT3, D3DRTYPE_VOLUMETEXTURE, D3DXIFF_DDS, FALSE);
        check_volume_texture_level_desc(texture, 0, D3DFMT_DXT3, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 4, 4, 4, FALSE);
        check_volume_texture_level_desc(texture, 1, D3DFMT_DXT3, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 2, 2, 2, FALSE);
        check_volume_texture_level_desc(texture, 2, D3DFMT_DXT3, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 1, 1, 1, FALSE);

        for (mip_level = 0; mip_level < 3; ++mip_level)
        {
            const uint32_t expected_color = dds_volume_dxt3_4_4_4_expected_uncompressed[mip_level];
            uint32_t x, y, z;

            IDirect3DVolumeTexture9_GetLevelDesc(texture, mip_level, &desc);
            get_texture_volume_readback(device, texture, mip_level, &volume_rb);
            for (z = 0; z < desc.Depth; ++z)
            {
                for (y = 0; y < desc.Height; ++y)
                {
                    for (x = 0; x < desc.Width; ++x)
                    {
                        check_volume_readback_pixel_4bpp(&volume_rb, x, y, z, expected_color, FALSE);
                    }
                }
            }
            release_volume_readback(&volume_rb);
        }
        IDirect3DVolumeTexture9_Release(texture);
    }
    else
    {
        skip("D3DFMT_DXT3 volume textures are not supported, skipping tests.\n");
    }

    /* Load a BMP file into a volume texture. */
    hr = D3DXCreateVolumeTextureFromFileInMemoryEx(device, bmp_32bpp_4_4_argb, sizeof(bmp_32bpp_4_4_argb),
            D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_FILTER_POINT, 0, &img_info, NULL, &texture);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    check_texture_mip_levels(texture, 3, FALSE);
    check_image_info(&img_info, 4, 4, 1, 1, D3DFMT_A8R8G8B8, D3DRTYPE_TEXTURE, D3DXIFF_BMP, FALSE);
    check_volume_texture_level_desc(texture, 0, D3DFMT_A8R8G8B8, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 4, 4, 1, FALSE);
    check_volume_texture_level_desc(texture, 1, D3DFMT_A8R8G8B8, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 2, 2, 1, FALSE);
    check_volume_texture_level_desc(texture, 2, D3DFMT_A8R8G8B8, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 1, 1, 1, FALSE);

    for (mip_level = 0; mip_level < 3; ++mip_level)
    {
        IDirect3DVolumeTexture9_GetLevelDesc(texture, mip_level, &desc);
        get_texture_volume_readback(device, texture, mip_level, &volume_rb);
        for (z = 0; z < desc.Depth; ++z)
        {
            for (y = 0; y < desc.Height; ++y)
            {
                const uint32_t *expected_color = &bmp_32bpp_4_4_argb_expected[(desc.Height == 1 || y < (desc.Height / 2)) ? 0 : 2];

                for (x = 0; x < desc.Width; ++x)
                {
                    if (desc.Width == 1 || x < (desc.Width / 2))
                        check_volume_readback_pixel_4bpp(&volume_rb, x, y, z, expected_color[0], FALSE);
                    else
                        check_volume_readback_pixel_4bpp(&volume_rb, x, y, z, expected_color[1], FALSE);
                }
            }
        }
        release_volume_readback(&volume_rb);
    }
    IDirect3DVolumeTexture9_Release(texture);

    /* Create texture from JPG. */
    hr = D3DXCreateVolumeTextureFromFileInMemoryEx(device, jpg_rgb_2_2, sizeof(jpg_rgb_2_2),
            D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_FILTER_POINT, 0, &img_info, NULL, &texture);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    check_texture_mip_levels(texture, 2, FALSE);
    check_image_info(&img_info, 2, 2, 1, 1, D3DFMT_X8R8G8B8, D3DRTYPE_TEXTURE, D3DXIFF_JPG, FALSE);
    check_volume_texture_level_desc(texture, 0, D3DFMT_X8R8G8B8, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 2, 2, 1, FALSE);
    check_volume_texture_level_desc(texture, 1, D3DFMT_X8R8G8B8, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 1, 1, 1, FALSE);

    IDirect3DVolumeTexture9_Release(texture);

    /* Create texture from PNG. */
    hr = D3DXCreateVolumeTextureFromFileInMemoryEx(device, png_2_2_48bpp_rgb, sizeof(png_2_2_48bpp_rgb),
            D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_FILTER_POINT, 0, &img_info, NULL, &texture);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

    check_texture_mip_levels(texture, 2, FALSE);
    check_image_info(&img_info, 2, 2, 1, 1, D3DFMT_A16B16G16R16, D3DRTYPE_TEXTURE, D3DXIFF_PNG, FALSE);
    check_volume_texture_level_desc(texture, 0, D3DFMT_A16B16G16R16, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 2, 2, 1, FALSE);
    check_volume_texture_level_desc(texture, 1, D3DFMT_A16B16G16R16, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 1, 1, 1, FALSE);

    IDirect3DVolumeTexture9_Release(texture);

    hr = D3DXCreateVolumeTextureFromFileInMemoryEx(device, png_2_2_64bpp_rgba, sizeof(png_2_2_64bpp_rgba),
            D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, D3DUSAGE_DYNAMIC, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT,
            D3DX_DEFAULT, D3DX_FILTER_POINT, 0, &img_info, NULL, &texture);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

    check_texture_mip_levels(texture, 2, FALSE);
    check_image_info(&img_info, 2, 2, 1, 1, D3DFMT_A16B16G16R16, D3DRTYPE_TEXTURE, D3DXIFF_PNG, FALSE);
    check_volume_texture_level_desc(texture, 0, D3DFMT_A16B16G16R16, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 2, 2, 1, FALSE);
    check_volume_texture_level_desc(texture, 1, D3DFMT_A16B16G16R16, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT, 1, 1, 1, FALSE);

    IDirect3DVolumeTexture9_Release(texture);
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
        ok(hr == D3D_OK, "D3DXSaveTextureToFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);
        if (SUCCEEDED(hr))
        {
            buffer_pointer = ID3DXBuffer_GetBufferPointer(buffer);
            buffer_size = ID3DXBuffer_GetBufferSize(buffer);
            hr = D3DXGetImageInfoFromFileInMemory(buffer_pointer, buffer_size, &info);
            ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);

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
    ok(hr == D3D_OK, "D3DXSaveTextureToFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);
    if (SUCCEEDED(hr))
    {
        buffer_pointer = ID3DXBuffer_GetBufferPointer(buffer);
        buffer_size = ID3DXBuffer_GetBufferSize(buffer);
        hr = D3DXGetImageInfoFromFileInMemory(buffer_pointer, buffer_size, &info);
        ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);

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
    ok(hr == D3D_OK, "D3DXFillCubeTexture returned %#lx, expected %#lx\n", hr, D3D_OK);

    hr = D3DXSaveTextureToFileInMemory(&buffer, D3DXIFF_BMP, (IDirect3DBaseTexture9 *)cube_texture, NULL);
    ok(hr == D3D_OK, "D3DXSaveTextureToFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);
    if (SUCCEEDED(hr))
    {
        IDirect3DSurface9 *surface;

        buffer_pointer = ID3DXBuffer_GetBufferPointer(buffer);
        buffer_size = ID3DXBuffer_GetBufferSize(buffer);
        hr = D3DXGetImageInfoFromFileInMemory(buffer_pointer, buffer_size, &info);
        ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);

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
            ok(hr == D3D_OK, "D3DXLoadSurfaceFromFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);

            hr = IDirect3DSurface9_LockRect(surface, &locked_rect, NULL, D3DLOCK_READONLY);
            if (SUCCEEDED(hr))
            {
                DWORD *color = locked_rect.pBits;
                ok(*color == 0x00ff0000, "Got color %#lx, expected %#x\n", *color, 0x00ff0000);
                IDirect3DSurface9_UnlockRect(surface);
            }

            IDirect3DSurface9_Release(surface);
        } else skip("Failed to create surface\n");

        ID3DXBuffer_Release(buffer);
    }

    todo_wine {
    hr = D3DXSaveTextureToFileInMemory(&buffer, D3DXIFF_DDS, (IDirect3DBaseTexture9 *)cube_texture, NULL);
    ok(hr == D3D_OK, "D3DXSaveTextureToFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);
    if (SUCCEEDED(hr))
    {
        buffer_pointer = ID3DXBuffer_GetBufferPointer(buffer);
        buffer_size = ID3DXBuffer_GetBufferSize(buffer);
        hr = D3DXGetImageInfoFromFileInMemory(buffer_pointer, buffer_size, &info);
        ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);

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
    ok(hr == D3D_OK, "D3DXSaveTextureToFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);
    if (SUCCEEDED(hr))
    {
        buffer_pointer = ID3DXBuffer_GetBufferPointer(buffer);
        buffer_size = ID3DXBuffer_GetBufferSize(buffer);
        hr = D3DXGetImageInfoFromFileInMemory(buffer_pointer, buffer_size, &info);
        ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);

        ok(info.Width == 256, "Got width %u, expected %u\n", info.Width, 256);
        ok(info.Height == 256, "Got height %u, expected %u\n", info.Height, 256);
        ok(info.Depth == 1, "Got depth %u, expected %u\n", info.Depth, 1);
        ok(info.MipLevels == 1, "Got miplevels %u, expected %u\n", info.MipLevels, 1);
        ok(info.ResourceType == D3DRTYPE_TEXTURE, "Got resource type %#x, expected %#x\n", info.ResourceType, D3DRTYPE_TEXTURE);
        ok(info.ImageFileFormat == D3DXIFF_BMP, "Got file format %#x, expected %#x\n", info.ImageFileFormat, D3DXIFF_BMP);
        ID3DXBuffer_Release(buffer);
    }

    hr = D3DXSaveTextureToFileInMemory(&buffer, D3DXIFF_DDS, (IDirect3DBaseTexture9 *)volume_texture, NULL);
    ok(hr == D3D_OK, "D3DXSaveTextureToFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);
    if (SUCCEEDED(hr))
    {
        buffer_pointer = ID3DXBuffer_GetBufferPointer(buffer);
        buffer_size = ID3DXBuffer_GetBufferSize(buffer);
        hr = D3DXGetImageInfoFromFileInMemory(buffer_pointer, buffer_size, &info);
        ok(hr == D3D_OK, "D3DXGetImageInfoFromFileInMemory returned %#lx, expected %#lx\n", hr, D3D_OK);

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
#if 0
float f1[2] = { 0.4f, 0.1f };
bool b1 = false;
float4 f2;
bool b2[2];

struct s
{
    bool b3;
    float f3;
    float4 f4;
    uint i1;
    int i2;
};

struct s s1;

float4 main(float3 pos : POSITION, float3 size : PSIZE) : COLOR
{
    float t = b2[0] * b2[1] * f2.x + s1.i2;
    return float4(pos, f1[0] + f1[1] + b1 * 0.6f + t);
}
#endif
    static const DWORD shader_code2[] =
    {
        0x54580100, 0x0074fffe, 0x42415443, 0x0000001c, 0x000001a3, 0x54580100, 0x00000005, 0x0000001c,
        0x00000100, 0x000001a0, 0x00000080, 0x00090002, 0x00000001, 0x00000084, 0x00000094, 0x000000a4,
        0x00070002, 0x00000002, 0x000000a8, 0x00000000, 0x000000b8, 0x00050002, 0x00000002, 0x000000bc,
        0x000000cc, 0x000000ec, 0x000a0002, 0x00000001, 0x000000f0, 0x00000000, 0x00000100, 0x00000002,
        0x00000005, 0x00000190, 0x00000000, 0xab003162, 0x00010000, 0x00010001, 0x00000001, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xab003262, 0x00010000, 0x00010001, 0x00000002,
        0x00000000, 0xab003166, 0x00030000, 0x00010001, 0x00000002, 0x00000000, 0x3ecccccd, 0x00000000,
        0x00000000, 0x00000000, 0x3dcccccd, 0x00000000, 0x00000000, 0x00000000, 0xab003266, 0x00030001,
        0x00040001, 0x00000001, 0x00000000, 0x62003173, 0xabab0033, 0x00010000, 0x00010001, 0x00000001,
        0x00000000, 0xab003366, 0x00030000, 0x00010001, 0x00000001, 0x00000000, 0xab003466, 0x00030001,
        0x00040001, 0x00000001, 0x00000000, 0xab003169, 0x00020000, 0x00010001, 0x00000001, 0x00000000,
        0xab003269, 0x00020000, 0x00010001, 0x00000001, 0x00000000, 0x00000103, 0x00000108, 0x00000118,
        0x0000011c, 0x0000012c, 0x00000130, 0x00000140, 0x00000144, 0x00000154, 0x00000158, 0x00000005,
        0x00080001, 0x00050001, 0x00000168, 0x4d007874, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820,
        0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x30312072, 0xab00312e, 0x0062fffe, 0x54494c43,
        0x00000030, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x40000000, 0x3fe33333, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x0059fffe, 0x434c5846, 0x00000008, 0xa0500001, 0x00000002, 0x00000000, 0x00000002,
        0x0000001c, 0x00000000, 0x00000002, 0x00000020, 0x00000000, 0x00000007, 0x00000000, 0xa0500001,
        0x00000002, 0x00000000, 0x00000007, 0x00000000, 0x00000000, 0x00000002, 0x00000028, 0x00000000,
        0x00000007, 0x00000004, 0xa0400001, 0x00000002, 0x00000000, 0x00000007, 0x00000004, 0x00000000,
        0x00000002, 0x00000010, 0x00000000, 0x00000007, 0x00000000, 0xa0400001, 0x00000002, 0x00000000,
        0x00000002, 0x00000014, 0x00000000, 0x00000002, 0x00000018, 0x00000000, 0x00000007, 0x00000001,
        0xa0500001, 0x00000002, 0x00000000, 0x00000002, 0x00000024, 0x00000000, 0x00000001, 0x0000002c,
        0x00000000, 0x00000007, 0x00000002, 0xa0400001, 0x00000002, 0x00000000, 0x00000007, 0x00000002,
        0x00000000, 0x00000007, 0x00000001, 0x00000000, 0x00000007, 0x00000004, 0xa0400001, 0x00000002,
        0x00000000, 0x00000007, 0x00000000, 0x00000000, 0x00000007, 0x00000004, 0x00000000, 0x00000004,
        0x00000003, 0x10000003, 0x00000001, 0x00000000, 0x00000003, 0x00000000, 0x00000000, 0x00000004,
        0x00000000, 0xf0f0f0f0, 0x0f0f0f0f, 0x0000ffff,
    };
    IDirect3DVolumeTexture9 *volume_texture;
    IDirect3DCubeTexture9 *cube_texture;
    D3DXCONSTANTTABLE_DESC ctab_desc;
    ID3DXBuffer *buffer, *buffer2;
    D3DXCONSTANT_DESC const_desc;
    unsigned int x, y, z, count;
    D3DPRESENT_PARAMETERS d3dpp;
    IDirect3DTexture9 *texture;
    IDirect3DDevice9 *device;
    ID3DXTextureShader *tx;
    unsigned int *data;
    D3DLOCKED_RECT lr;
    D3DLOCKED_BOX lb;
    D3DXHANDLE h, h2;
    IDirect3D9 *d3d;
    D3DCAPS9 caps;
    DWORD size;
    HRESULT hr;
    HWND wnd;

    hr = D3DXCreateTextureShader(NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

    tx = (void *)0xdeadbeef;
    hr = D3DXCreateTextureShader(NULL, &tx);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    ok(tx == (void *)0xdeadbeef, "Unexpected pointer %p.\n", tx);

    tx = (void *)0xdeadbeef;
    hr = D3DXCreateTextureShader(shader_invalid, &tx);
    ok(hr == D3DXERR_INVALIDDATA, "Got unexpected hr %#lx.\n", hr);
    ok(tx == (void *)0xdeadbeef, "Unexpected pointer %p.\n", tx);

    tx = (void *)0xdeadbeef;
    hr = D3DXCreateTextureShader(shader_zero, &tx);
    ok(hr == D3DXERR_INVALIDDATA, "Got unexpected hr %#lx.\n", hr);
    ok(tx == (void *)0xdeadbeef, "Unexpected pointer %p.\n", tx);

    tx = (void *)0xdeadbeef;
    hr = D3DXCreateTextureShader(shader_empty, &tx);
    ok(hr == D3DXERR_INVALIDDATA, "Got unexpected hr %#lx.\n", hr);
    ok(tx == (void *)0xdeadbeef, "Unexpected pointer %p.\n", tx);

    hr = D3DXCreateTextureShader(shader_code, &tx);
    ok(SUCCEEDED(hr), "Got unexpected hr %#lx.\n", hr);

    hr = tx->lpVtbl->GetFunction(tx, &buffer);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);

    hr = tx->lpVtbl->GetFunction(tx, &buffer2);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
    ok(buffer2 == buffer, "Unexpected buffer object.\n");
    ID3DXBuffer_Release(buffer2);

    size = ID3DXBuffer_GetBufferSize(buffer);
    ok(size == 224, "Unexpected buffer size %lu.\n", size);

    ID3DXBuffer_Release(buffer);

    /* Constant buffer */
    hr = tx->lpVtbl->GetConstantBuffer(tx, &buffer);
    todo_wine
    ok(SUCCEEDED(hr), "Failed to get texture shader constant buffer.\n");

    if (SUCCEEDED(hr))
    {
        size = ID3DXBuffer_GetBufferSize(buffer);
        ok(!size, "Unexpected buffer size %lu.\n", size);

        ID3DXBuffer_Release(buffer);
    }

    hr = tx->lpVtbl->GetDesc(tx, &ctab_desc);
    todo_wine
    ok(hr == S_OK, "Failed to get constant description, hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
        ok(!ctab_desc.Constants, "Unexpected number of constants %u.\n", ctab_desc.Constants);

    /* Constant table access calls, without constant table. */
    h = tx->lpVtbl->GetConstant(tx, NULL, 0);
    ok(!h, "Unexpected handle %p.\n", h);

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
        skip("Failed to create IDirect3DDevice9 object, hr %#lx.\n", hr);
        IDirect3D9_Release(d3d);
        DestroyWindow(wnd);
        IUnknown_Release(tx);
        return;
    }

    IDirect3DDevice9_GetDeviceCaps(device, &caps);

    hr = IDirect3DDevice9_CreateTexture(device, 256, 256, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM,
            &texture, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#lx.\n", hr);

    hr = D3DXFillTextureTX(texture, tx);
    ok(SUCCEEDED(hr), "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DTexture9_LockRect(texture, 0, &lr, NULL, D3DLOCK_READONLY);
    ok(SUCCEEDED(hr), "Locking texture failed, hr %#lx.\n", hr);
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
    ok(SUCCEEDED(hr), "Unlocking texture failed, hr %#lx.\n", hr);

    IDirect3DTexture9_Release(texture);

    if (!(caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP))
    {
        skip("Cube textures not supported, skipping tests.\n");
        goto cleanup;
    }

    hr = IDirect3DDevice9_CreateCubeTexture(device, 256, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM,
            &cube_texture, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#lx.\n", hr);

    hr = D3DXFillCubeTextureTX(cube_texture, tx);
    ok(SUCCEEDED(hr), "Got unexpected hr %#lx.\n", hr);
    compare_cube_texture(cube_texture, fillfunc_cube_coord, 1);
    IDirect3DCubeTexture9_Release(cube_texture);

    if (!(caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP) || caps.MaxVolumeExtent < 64)
    {
        skip("Volume textures not supported, skipping test.\n");
        goto cleanup;
    }
    hr = IDirect3DDevice9_CreateVolumeTexture(device, 64, 64, 64, 1, 0, D3DFMT_A8R8G8B8,
            D3DPOOL_SYSTEMMEM, &volume_texture, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#lx.\n", hr);

    hr = D3DXFillVolumeTextureTX(volume_texture, tx);
    todo_wine
    ok(SUCCEEDED(hr), "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DVolumeTexture9_LockBox(volume_texture, 0, &lb, NULL, D3DLOCK_READONLY);
    ok(SUCCEEDED(hr), "Locking texture failed, hr %#lx.\n", hr);
    data = lb.pBits;
    for (z = 0; z < 64; ++z)
    {
        for (y = 0; y < 64; ++y)
        {
            for (x = 0; x < 64; ++x)
            {
                unsigned int expected = 0xff000000 | ((x * 4 + 2) << 16) | ((y * 4 + 2) << 8) | (z * 4 + 2);
                unsigned int color = data[z * lb.SlicePitch / sizeof(*data) + y * lb.RowPitch / sizeof(*data) + x];

                todo_wine
                ok(compare_color(color, expected, 1), "Unexpected color %08x at (%u, %u, %u).\n",
                        color, x, y, z);
            }
        }
    }
    hr = IDirect3DVolumeTexture9_UnlockBox(volume_texture, 0);
    ok(SUCCEEDED(hr), "Unlocking texture failed, hr %#lx.\n", hr);

    IDirect3DVolumeTexture9_Release(volume_texture);

    IUnknown_Release(tx);

    /* With constant table */
    tx = NULL;
    hr = D3DXCreateTextureShader(shader_code2, &tx);
    todo_wine
    ok(SUCCEEDED(hr), "Got unexpected hr %#lx.\n", hr);
    if (FAILED(hr))
        goto cleanup;

    hr = tx->lpVtbl->GetConstantBuffer(tx, &buffer);
    todo_wine
    ok(SUCCEEDED(hr), "Failed to get texture shader constant buffer.\n");
    if (FAILED(hr))
    {
        skip("Texture shaders not supported, skipping further tests.\n");
        IUnknown_Release(tx);
        return;
    }

    size = ID3DXBuffer_GetBufferSize(buffer);
    ok(size == 176, "Unexpected buffer size %lu.\n", size);

    hr = tx->lpVtbl->GetDesc(tx, &ctab_desc);
    ok(hr == S_OK, "Failed to get constant description, hr %#lx.\n", hr);
    ok(ctab_desc.Constants == 5, "Unexpected number of constants %u.\n", ctab_desc.Constants);

    h = tx->lpVtbl->GetConstant(tx, NULL, 0);
    ok(!!h, "Unexpected handle %p.\n", h);
    hr = tx->lpVtbl->GetConstantDesc(tx, h, &const_desc, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!strcmp(const_desc.Name, "b1"), "Unexpected name %s.\n", const_desc.Name);
    ok(const_desc.RegisterSet == D3DXRS_FLOAT4, "Unexpected register set %u.\n", const_desc.RegisterSet);
    ok(const_desc.RegisterIndex == 9, "Unexpected register index %u.\n", const_desc.RegisterIndex);
    ok(const_desc.RegisterCount == 1, "Unexpected register count %u.\n", const_desc.RegisterCount);

    h = tx->lpVtbl->GetConstant(tx, NULL, 1);
    ok(!!h, "Unexpected handle %p.\n", h);
    hr = tx->lpVtbl->GetConstantDesc(tx, h, &const_desc, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!strcmp(const_desc.Name, "b2"), "Unexpected name %s.\n", const_desc.Name);
    ok(const_desc.RegisterSet == D3DXRS_FLOAT4, "Unexpected register set %u.\n", const_desc.RegisterSet);
    ok(const_desc.RegisterIndex == 7, "Unexpected register index %u.\n", const_desc.RegisterIndex);
    ok(const_desc.RegisterCount == 2, "Unexpected register count %u.\n", const_desc.RegisterCount);

    h = tx->lpVtbl->GetConstant(tx, NULL, 2);
    ok(!!h, "Unexpected handle %p.\n", h);
    hr = tx->lpVtbl->GetConstantDesc(tx, h, &const_desc, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!strcmp(const_desc.Name, "f1"), "Unexpected name %s.\n", const_desc.Name);
    ok(const_desc.RegisterSet == D3DXRS_FLOAT4, "Unexpected register set %u.\n", const_desc.RegisterSet);
    ok(const_desc.RegisterIndex == 5, "Unexpected register index %u.\n", const_desc.RegisterIndex);
    ok(const_desc.RegisterCount == 2, "Unexpected register count %u.\n", const_desc.RegisterCount);
    ok(const_desc.Elements == 2, "Unexpected elements count %u.\n", const_desc.Elements);

    /* Array */
    h2 = tx->lpVtbl->GetConstantElement(tx, h, 0);
    ok(!!h2, "Unexpected handle %p.\n", h);
    hr = tx->lpVtbl->GetConstantDesc(tx, h2, &const_desc, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!strcmp(const_desc.Name, "f1"), "Unexpected name %s.\n", const_desc.Name);
    ok(const_desc.RegisterSet == D3DXRS_FLOAT4, "Unexpected register set %u.\n", const_desc.RegisterSet);
    ok(const_desc.RegisterIndex == 5, "Unexpected register index %u.\n", const_desc.RegisterIndex);
    ok(const_desc.RegisterCount == 1, "Unexpected register count %u.\n", const_desc.RegisterCount);
    ok(const_desc.Elements == 1, "Unexpected elements count %u.\n", const_desc.Elements);

    h2 = tx->lpVtbl->GetConstantElement(tx, h, 1);
    ok(!!h2, "Unexpected handle %p.\n", h);
    hr = tx->lpVtbl->GetConstantDesc(tx, h2, &const_desc, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!strcmp(const_desc.Name, "f1"), "Unexpected name %s.\n", const_desc.Name);
    ok(const_desc.RegisterSet == D3DXRS_FLOAT4, "Unexpected register set %u.\n", const_desc.RegisterSet);
    ok(const_desc.RegisterIndex == 6, "Unexpected register index %u.\n", const_desc.RegisterIndex);
    ok(const_desc.RegisterCount == 1, "Unexpected register count %u.\n", const_desc.RegisterCount);
    ok(const_desc.Elements == 1, "Unexpected elements count %u.\n", const_desc.Elements);

    h2 = tx->lpVtbl->GetConstantElement(tx, h, 2);
    ok(!h2, "Unexpected handle %p.\n", h);

    h = tx->lpVtbl->GetConstant(tx, NULL, 3);
    ok(!!h, "Unexpected handle %p.\n", h);
    hr = tx->lpVtbl->GetConstantDesc(tx, h, &const_desc, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!strcmp(const_desc.Name, "f2"), "Unexpected name %s.\n", const_desc.Name);
    ok(const_desc.RegisterSet == D3DXRS_FLOAT4, "Unexpected register set %u.\n", const_desc.RegisterSet);
    ok(const_desc.RegisterIndex == 10, "Unexpected register index %u.\n", const_desc.RegisterIndex);
    ok(const_desc.RegisterCount == 1, "Unexpected register count %u.\n", const_desc.RegisterCount);

    /* Structure */
    h = tx->lpVtbl->GetConstant(tx, NULL, 4);
    ok(!!h, "Unexpected handle %p.\n", h);
    hr = tx->lpVtbl->GetConstantDesc(tx, h, &const_desc, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!strcmp(const_desc.Name, "s1"), "Unexpected name %s.\n", const_desc.Name);
    ok(const_desc.RegisterSet == D3DXRS_FLOAT4, "Unexpected register set %u.\n", const_desc.RegisterSet);
    ok(const_desc.RegisterIndex == 0, "Unexpected register index %u.\n", const_desc.RegisterIndex);
    ok(const_desc.RegisterCount == 5, "Unexpected register count %u.\n", const_desc.RegisterCount);
    ok(const_desc.Class == D3DXPC_STRUCT, "Unexpected class %u.\n", const_desc.Class);
    ok(const_desc.StructMembers == 5, "Unexpected member count %u.\n", const_desc.StructMembers);

    h2 = tx->lpVtbl->GetConstant(tx, h, 0);
    ok(!!h2, "Unexpected handle %p.\n", h);
    hr = tx->lpVtbl->GetConstantDesc(tx, h2, &const_desc, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!strcmp(const_desc.Name, "b3"), "Unexpected name %s.\n", const_desc.Name);
    ok(const_desc.RegisterSet == D3DXRS_FLOAT4, "Unexpected register set %u.\n", const_desc.RegisterSet);
    ok(const_desc.RegisterIndex == 0, "Unexpected register index %u.\n", const_desc.RegisterIndex);
    ok(const_desc.RegisterCount == 1, "Unexpected register count %u.\n", const_desc.RegisterCount);
    ok(const_desc.Elements == 1, "Unexpected elements count %u.\n", const_desc.Elements);

    h2 = tx->lpVtbl->GetConstant(tx, h, 1);
    ok(!!h2, "Unexpected handle %p.\n", h);
    hr = tx->lpVtbl->GetConstantDesc(tx, h2, &const_desc, &count);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!strcmp(const_desc.Name, "f3"), "Unexpected name %s.\n", const_desc.Name);
    ok(const_desc.RegisterSet == D3DXRS_FLOAT4, "Unexpected register set %u.\n", const_desc.RegisterSet);
    ok(const_desc.RegisterIndex == 1, "Unexpected register index %u.\n", const_desc.RegisterIndex);
    ok(const_desc.RegisterCount == 1, "Unexpected register count %u.\n", const_desc.RegisterCount);
    ok(const_desc.Elements == 1, "Unexpected elements count %u.\n", const_desc.Elements);

    h2 = tx->lpVtbl->GetConstant(tx, h, 10);
    ok(!h2, "Unexpected handle %p.\n", h);

    ID3DXBuffer_Release(buffer);

 cleanup:
    IDirect3DDevice9_Release(device);
    IDirect3D9_Release(d3d);
    DestroyWindow(wnd);
    if (tx)
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
        skip("Failed to create IDirect3DDevice9 object %#lx\n", hr);
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
    ok(!ref, "Device has %lu references left.\n", ref);

    IDirect3D9_Release(d3d);
    DestroyWindow(wnd);

    test_texture_shader();
}
