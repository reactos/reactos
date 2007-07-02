/*
 * Copyright 2006-2007 Henri Verbeet
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
#include <d3d9.h>
#include <dxerr9.h>
#include "wine/test.h"

static HWND create_window(void)
{
    WNDCLASS wc = {0};
    wc.lpfnWndProc = &DefWindowProc;
    wc.lpszClassName = "d3d9_test_wc";
    RegisterClass(&wc);

    return CreateWindow("d3d9_test_wc", "d3d9_test",
            0, 0, 0, 0, 0, 0, 0, 0, 0);
}

static IDirect3DDevice9 *init_d3d9(HMODULE d3d9_handle)
{
    IDirect3D9 * (__stdcall * d3d9_create)(UINT SDKVersion) = 0;
    IDirect3D9 *d3d9_ptr = 0;
    IDirect3DDevice9 *device_ptr = 0;
    D3DPRESENT_PARAMETERS present_parameters;
    HRESULT hr;

    d3d9_create = (void *)GetProcAddress(d3d9_handle, "Direct3DCreate9");
    ok(d3d9_create != NULL, "Failed to get address of Direct3DCreate9\n");
    if (!d3d9_create) return NULL;

    d3d9_ptr = d3d9_create(D3D_SDK_VERSION);
    ok(d3d9_ptr != NULL, "Failed to create IDirect3D9 object\n");
    if (!d3d9_ptr) return NULL;

    ZeroMemory(&present_parameters, sizeof(present_parameters));
    present_parameters.Windowed = TRUE;
    present_parameters.hDeviceWindow = create_window();
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;

    hr = IDirect3D9_CreateDevice(d3d9_ptr, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            NULL, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device_ptr);

    if(FAILED(hr))
    {
        trace("could not create device, IDirect3D9_CreateDevice returned %#x\n", hr);
        return NULL;
    }

    return device_ptr;
}

static void test_surface_get_container(IDirect3DDevice9 *device_ptr)
{
    IDirect3DTexture9 *texture_ptr = 0;
    IDirect3DSurface9 *surface_ptr = 0;
    void *container_ptr;
    HRESULT hr;

    hr = IDirect3DDevice9_CreateTexture(device_ptr, 128, 128, 1, 0,
            D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture_ptr, 0);
    ok(SUCCEEDED(hr) && texture_ptr != NULL, "CreateTexture returned: hr %#x, texture_ptr %p. "
        "Expected hr %#x, texture_ptr != %p\n", hr, texture_ptr, D3D_OK, NULL);
    if (!texture_ptr || FAILED(hr)) goto cleanup;

    hr = IDirect3DTexture9_GetSurfaceLevel(texture_ptr, 0, &surface_ptr);
    ok(SUCCEEDED(hr) && surface_ptr != NULL, "GetSurfaceLevel returned: hr %#x, surface_ptr %p. "
        "Expected hr %#x, surface_ptr != %p\n", hr, surface_ptr, D3D_OK, NULL);
    if (!surface_ptr || FAILED(hr)) goto cleanup;

    /* These should work... */
    container_ptr = (void *)0x1337c0d3;
    hr = IDirect3DSurface9_GetContainer(surface_ptr, &IID_IUnknown, &container_ptr);
    ok(SUCCEEDED(hr) && container_ptr == texture_ptr, "GetContainer returned: hr %#x, container_ptr %p. "
        "Expected hr %#x, container_ptr %p\n", hr, container_ptr, S_OK, texture_ptr);
    if (container_ptr && container_ptr != (void *)0x1337c0d3) IUnknown_Release((IUnknown *)container_ptr);

    container_ptr = (void *)0x1337c0d3;
    hr = IDirect3DSurface9_GetContainer(surface_ptr, &IID_IDirect3DResource9, &container_ptr);
    ok(SUCCEEDED(hr) && container_ptr == texture_ptr, "GetContainer returned: hr %#x, container_ptr %p. "
        "Expected hr %#x, container_ptr %p\n", hr, container_ptr, S_OK, texture_ptr);
    if (container_ptr && container_ptr != (void *)0x1337c0d3) IUnknown_Release((IUnknown *)container_ptr);

    container_ptr = (void *)0x1337c0d3;
    hr = IDirect3DSurface9_GetContainer(surface_ptr, &IID_IDirect3DBaseTexture9, &container_ptr);
    ok(SUCCEEDED(hr) && container_ptr == texture_ptr, "GetContainer returned: hr %#x, container_ptr %p. "
        "Expected hr %#x, container_ptr %p\n", hr, container_ptr, S_OK, texture_ptr);
    if (container_ptr && container_ptr != (void *)0x1337c0d3) IUnknown_Release((IUnknown *)container_ptr);

    container_ptr = (void *)0x1337c0d3;
    hr = IDirect3DSurface9_GetContainer(surface_ptr, &IID_IDirect3DTexture9, &container_ptr);
    ok(SUCCEEDED(hr) && container_ptr == texture_ptr, "GetContainer returned: hr %#x, container_ptr %p. "
        "Expected hr %#x, container_ptr %p\n", hr, container_ptr, S_OK, texture_ptr);
    if (container_ptr && container_ptr != (void *)0x1337c0d3) IUnknown_Release((IUnknown *)container_ptr);

    /* ...and this one shouldn't. This should return E_NOINTERFACE and set container_ptr to NULL */
    container_ptr = (void *)0x1337c0d3;
    hr = IDirect3DSurface9_GetContainer(surface_ptr, &IID_IDirect3DSurface9, &container_ptr);
    ok(hr == E_NOINTERFACE && container_ptr == NULL, "GetContainer returned: hr %#x, container_ptr %p. "
        "Expected hr %#x, container_ptr %p\n", hr, container_ptr, E_NOINTERFACE, NULL);
    if (container_ptr && container_ptr != (void *)0x1337c0d3) IUnknown_Release((IUnknown *)container_ptr);

cleanup:
    if (texture_ptr) IDirect3DTexture9_Release(texture_ptr);
    if (surface_ptr) IDirect3DSurface9_Release(surface_ptr);
}

static void test_surface_alignment(IDirect3DDevice9 *device_ptr)
{
    IDirect3DSurface9 *surface_ptr = 0;
    HRESULT hr;
    int i;

    /* Test a sysmem surface as those aren't affected by the hardware's np2 restrictions */
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device_ptr, 5, 5, D3DFMT_R5G6B5, D3DPOOL_SYSTEMMEM, &surface_ptr, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateOffscreenPlainSurface returned %08x\n", hr);

    if(surface_ptr)
    {
        D3DLOCKED_RECT lockedRect;
        hr = IDirect3DSurface9_LockRect(surface_ptr, &lockedRect, NULL, 0);
        ok(hr == D3D_OK, "IDirect3DSurface9_LockRect returned %08x\n", hr);
        ok(!(lockedRect.Pitch & 3), "Surface pitch %d is not 32-bit aligned\n", lockedRect.Pitch);
        /* Some applications also depend on the exact pitch, rather than just
         * the alignment.
         */
        ok(lockedRect.Pitch == 12, "Got pitch %d, expected 12\n", lockedRect.Pitch);
        hr = IDirect3DSurface9_UnlockRect(surface_ptr);
        IDirect3DSurface9_Release(surface_ptr);
    }

    for (i = 0; i < 5; i++)
    {
        IDirect3DTexture9 *pTexture;
        int j, pitch;

        hr = IDirect3DDevice9_CreateTexture(device_ptr, 64, 64, 0, 0, MAKEFOURCC('D', 'X', 'T', '1'+i),
                                            D3DPOOL_MANAGED, &pTexture, NULL);
        ok(SUCCEEDED(hr), "IDirect3DDevice9_CreateTexture: %s\n", DXGetErrorString9(hr));
        if (FAILED(hr)) continue;

        for (j = IDirect3DBaseTexture9_GetLevelCount(pTexture) - 1; j >= 0; j--)
        {
            D3DLOCKED_RECT rc;
            D3DSURFACE_DESC descr;

            IDirect3DTexture9_GetLevelDesc(pTexture, j, &descr);
            hr = IDirect3DTexture9_LockRect(pTexture, j, &rc, NULL, 0);
            ok(SUCCEEDED(hr), "IDirect3DTexture9_LockRect: %s\n", DXGetErrorString9(hr));
            IDirect3DTexture9_UnlockRect(pTexture, j);

            pitch = ((descr.Width + 3) >> 2) << 3;
            if (i > 0) pitch <<= 1;
            ok(rc.Pitch == pitch, "Wrong pitch for DXT%d lvl[%d (%dx%d)]: expected %d got %d\n",
               i + 1, j, descr.Width, descr.Height, pitch, rc.Pitch);
        }
        IUnknown_Release( pTexture );
    }
}

/* Since the DXT formats are based on 4x4 blocks, locking works slightly
 * different than with regular formats. This patch verifies we return the
 * correct memory offsets */
static void test_lockrect_offset(IDirect3DDevice9 *device)
{
    IDirect3DSurface9 *surface = 0;
    const RECT rect = {60, 60, 68, 68};
    D3DLOCKED_RECT locked_rect;
    unsigned int expected_offset;
    unsigned int offset;
    unsigned int i;
    BYTE *base;
    HRESULT hr;

    const struct {
        D3DFORMAT fmt;
        const char *name;
        unsigned int block_width;
        unsigned int block_height;
        unsigned int block_size;
    } dxt_formats[] = {
        {D3DFMT_DXT1, "D3DFMT_DXT1", 4, 4, 8},
        {D3DFMT_DXT2, "D3DFMT_DXT2", 4, 4, 16},
        {D3DFMT_DXT3, "D3DFMT_DXT3", 4, 4, 16},
        {D3DFMT_DXT4, "D3DFMT_DXT4", 4, 4, 16},
        {D3DFMT_DXT5, "D3DFMT_DXT5", 4, 4, 16},
    };

    for (i = 0; i < (sizeof(dxt_formats) / sizeof(*dxt_formats)); ++i) {
        hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 128, 128, dxt_formats[i].fmt, D3DPOOL_SCRATCH, &surface, 0);
        ok(SUCCEEDED(hr), "CreateOffscreenPlainSurface failed (%08x)\n", hr);

        hr = IDirect3DSurface9_LockRect(surface, &locked_rect, NULL, 0);
        ok(SUCCEEDED(hr), "LockRect failed (%08x)\n", hr);

        base = locked_rect.pBits;

        hr = IDirect3DSurface9_UnlockRect(surface);
        ok(SUCCEEDED(hr), "UnlockRect failed (%08x)\n", hr);

        hr = IDirect3DSurface9_LockRect(surface, &locked_rect, &rect, 0);
        ok(SUCCEEDED(hr), "LockRect failed (%08x)\n", hr);

        offset = (BYTE *)locked_rect.pBits - base;
        expected_offset = (rect.top / dxt_formats[i].block_height) * locked_rect.Pitch
                        + (rect.left / dxt_formats[i].block_width) * dxt_formats[i].block_size;
        ok(offset == expected_offset, "Got offset %u, expected offset %u for format %s\n", offset, expected_offset, dxt_formats[i].name);

        hr = IDirect3DSurface9_UnlockRect(surface);
        ok(SUCCEEDED(hr), "UnlockRect failed (%08x)\n", hr);

        IDirect3DSurface9_Release(surface);
    }
}

START_TEST(surface)
{
    HMODULE d3d9_handle;
    IDirect3DDevice9 *device_ptr;

    d3d9_handle = LoadLibraryA("d3d9.dll");
    if (!d3d9_handle)
    {
        skip("Could not load d3d9.dll\n");
        return;
    }

    device_ptr = init_d3d9(d3d9_handle);
    if (!device_ptr) return;

    test_surface_get_container(device_ptr);
    test_surface_alignment(device_ptr);
    test_lockrect_offset(device_ptr);
}
