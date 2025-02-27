/*
 * Copyright (C) 2007 Stefan Dösinger(for CodeWeavers)
 * Copyright (C) 2008 Alexander Dorofeyev
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

/* See comment in dlls/d3d9/tests/visual.c for general guidelines */

#include "wine/test.h"
#include "ddraw.h"
#include "d3d.h"

struct vec3
{
    float x, y, z;
};

struct vec4
{
    float x, y, z, w;
};

static HWND window;
static IDirectDraw7        *DirectDraw;
static IDirectDrawSurface7 *Surface;
static IDirectDrawSurface7 *depth_buffer;
static IDirect3D7          *Direct3D;
static IDirect3DDevice7    *Direct3DDevice;

static BOOL refdevice = FALSE;

static HRESULT (WINAPI *pDirectDrawCreateEx)(GUID *driver_guid,
        void **ddraw, REFIID interface_iid, IUnknown *outer);

static BOOL compare_uint(unsigned int x, unsigned int y, unsigned int max_diff)
{
    unsigned int diff = x > y ? x - y : y - x;

    return diff <= max_diff;
}

static BOOL color_match(D3DCOLOR c1, D3DCOLOR c2, BYTE max_diff)
{
    return compare_uint(c1 & 0xff, c2 & 0xff, max_diff)
            && compare_uint((c1 >> 8) & 0xff, (c2 >> 8) & 0xff, max_diff)
            && compare_uint((c1 >> 16) & 0xff, (c2 >> 16) & 0xff, max_diff)
            && compare_uint((c1 >> 24) & 0xff, (c2 >> 24) & 0xff, max_diff);
}

static HRESULT WINAPI enum_z_fmt(DDPIXELFORMAT *fmt, void *ctx)
{
    DDPIXELFORMAT *zfmt = ctx;

    if(fmt->dwZBufferBitDepth > zfmt->dwZBufferBitDepth)
    {
        *zfmt = *fmt;
    }
    return DDENUMRET_OK;
}

static HRESULT WINAPI enum_devtype_cb(char *desc_str, char *name, D3DDEVICEDESC7 *desc, void *ctx)
{
    BOOL *hal_ok = ctx;
    if (IsEqualGUID(&desc->deviceGUID, &IID_IDirect3DTnLHalDevice))
    {
        *hal_ok = TRUE;
        return DDENUMRET_CANCEL;
    }
    return DDENUMRET_OK;
}

static BOOL createObjects(void)
{
    HRESULT hr;
    HMODULE hmod = GetModuleHandleA("ddraw.dll");
    WNDCLASSA wc = {0};
    DDSURFACEDESC2 ddsd;
    DDPIXELFORMAT zfmt;
    BOOL hal_ok = FALSE;
    const GUID *devtype = &IID_IDirect3DHALDevice;

    if(!hmod) return FALSE;
    pDirectDrawCreateEx = (void*)GetProcAddress(hmod, "DirectDrawCreateEx");
    if(!pDirectDrawCreateEx) return FALSE;

    hr = pDirectDrawCreateEx(NULL, (void **) &DirectDraw, &IID_IDirectDraw7, NULL);
    ok(hr == DD_OK || hr == DDERR_NODIRECTDRAWSUPPORT, "Got hr %#lx.\n", hr);
    if(!DirectDraw) goto err;

    wc.lpfnWndProc = DefWindowProcA;
    wc.lpszClassName = "d3d7_test_wc";
    RegisterClassA(&wc);
    window = CreateWindowA("d3d7_test_wc", "d3d7_test", WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION,
            0, 0, 640, 480, 0, 0, 0, 0);

    hr = IDirectDraw7_SetCooperativeLevel(DirectDraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    if(FAILED(hr)) goto err;
    hr = IDirectDraw7_SetDisplayMode(DirectDraw, 640, 480, 32, 0, 0);
    if(FAILED(hr)) {
        /* 24 bit is fine too */
        hr = IDirectDraw7_SetDisplayMode(DirectDraw, 640, 480, 24, 0, 0);

    }
    ok(hr == DD_OK || hr == DDERR_UNSUPPORTED, "Got hr %#lx.\n", hr);
    if(FAILED(hr)) {
        /* use trace, the caller calls skip() */
        trace("SetDisplayMode failed\n");
        goto err;
    }

    hr = IDirectDraw7_QueryInterface(DirectDraw, &IID_IDirect3D7, (void**) &Direct3D);
    if (hr == E_NOINTERFACE) goto err;
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    /* DirectDraw Flipping behavior doesn't seem that well-defined. The reference rasterizer behaves differently
     * than hardware implementations. Request single buffering, that seems to work everywhere
     */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE;
    ddsd.dwBackBufferCount = 1;
    hr = IDirectDraw7_CreateSurface(DirectDraw, &ddsd, &Surface, NULL);
    if(FAILED(hr)) goto err;

    hr = IDirect3D7_EnumDevices(Direct3D, enum_devtype_cb, &hal_ok);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
    if (hal_ok) devtype = &IID_IDirect3DTnLHalDevice;

    memset(&zfmt, 0, sizeof(zfmt));
    hr = IDirect3D7_EnumZBufferFormats(Direct3D, devtype, enum_z_fmt, &zfmt);
    if (FAILED(hr)) goto err;
    if (zfmt.dwSize == 0) goto err;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    ddsd.ddpfPixelFormat = zfmt;
    ddsd.dwWidth = 640;
    ddsd.dwHeight = 480;
    hr = IDirectDraw7_CreateSurface(DirectDraw, &ddsd, &depth_buffer, NULL);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
    if (FAILED(hr)) goto err;

    hr = IDirectDrawSurface_AddAttachedSurface(Surface, depth_buffer);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
    if (FAILED(hr)) goto err;

    hr = IDirect3D7_CreateDevice(Direct3D, devtype, Surface, &Direct3DDevice);
    if (FAILED(hr) || !Direct3DDevice) goto err;
    return TRUE;

    err:
    if(DirectDraw) IDirectDraw7_Release(DirectDraw);
    if (depth_buffer) IDirectDrawSurface7_Release(depth_buffer);
    if(Surface) IDirectDrawSurface7_Release(Surface);
    if(Direct3D) IDirect3D7_Release(Direct3D);
    if(Direct3DDevice) IDirect3DDevice7_Release(Direct3DDevice);
    if(window) DestroyWindow(window);
    return FALSE;
}

static void releaseObjects(void)
{
    IDirect3DDevice7_Release(Direct3DDevice);
    IDirect3D7_Release(Direct3D);
    IDirectDrawSurface7_Release(depth_buffer);
    IDirectDrawSurface7_Release(Surface);
    IDirectDraw7_Release(DirectDraw);
    DestroyWindow(window);
}

static DWORD getPixelColor(IDirect3DDevice7 *device, UINT x, UINT y)
{
    DWORD ret;
    HRESULT hr;
    DDSURFACEDESC2 ddsd;
    RECT rectToLock = {x, y, x+1, y+1};
    IDirectDrawSurface7 *surf = NULL;

    /* Some implementations seem to dislike direct locking on the front buffer. Thus copy the front buffer
     * to an offscreen surface and lock it instead of the front buffer
     */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
    ddsd.dwWidth = 640;
    ddsd.dwHeight = 480;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    hr = IDirectDraw7_CreateSurface(DirectDraw, &ddsd, &surf, NULL);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    if(!surf)
    {
        trace("cannot create helper surface\n");
        return 0xdeadbeef;
    }

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);

    hr = IDirectDrawSurface_BltFast(surf, 0, 0, Surface, NULL, 0);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    if(FAILED(hr))
    {
        trace("Cannot blit\n");
        ret = 0xdeadbee;
        goto out;
    }

    hr = IDirectDrawSurface7_Lock(surf, &rectToLock, &ddsd, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
    if(FAILED(hr))
    {
        trace("Can't lock the offscreen surface, hr %#lx\n", hr);
        ret = 0xdeadbeec;
        goto out;
    }

    /* Remove the X channel for now. DirectX and OpenGL have different ideas how to treat it apparently, and it isn't
     * really important for these tests
     */
    ret = ((DWORD *) ddsd.lpSurface)[0] & 0x00ffffff;
    hr = IDirectDrawSurface7_Unlock(surf, NULL);
    if(FAILED(hr))
    {
        trace("Can't unlock the offscreen surface, hr %#lx\n", hr);
    }

out:
    IDirectDrawSurface7_Release(surf);
    return ret;
}

static void set_viewport_size(IDirect3DDevice7 *device)
{
    D3DVIEWPORT7 vp = {0};
    DDSURFACEDESC2 ddsd;
    HRESULT hr;
    IDirectDrawSurface7 *target;

    hr = IDirect3DDevice7_GetRenderTarget(device, &target);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    hr = IDirectDrawSurface7_GetSurfaceDesc(target, &ddsd);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    IDirectDrawSurface7_Release(target);

    vp.dwWidth = ddsd.dwWidth;
    vp.dwHeight = ddsd.dwHeight;
    hr = IDirect3DDevice7_SetViewport(device, &vp);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    return;
}

static void fog_test(IDirect3DDevice7 *device)
{
    HRESULT hr;
    unsigned int color;
    float start = 0.0, end = 1.0;
    D3DDEVICEDESC7 caps;

    struct
    {
        struct vec3 position;
        DWORD diffuse;
        DWORD specular;
    }
    /* Gets full z based fog with linear fog, no fog with specular color */
    untransformed_1[] =
    {
        {{-1.0f, -1.0f, 0.1f}, 0xffff0000, 0xff000000},
        {{-1.0f,  0.0f, 0.1f}, 0xffff0000, 0xff000000},
        {{ 0.0f,  0.0f, 0.1f}, 0xffff0000, 0xff000000},
        {{ 0.0f, -1.0f, 0.1f}, 0xffff0000, 0xff000000},
    },
    /* Ok, I am too lazy to deal with transform matrices */
    untransformed_2[] =
    {
        {{-1.0f,  0.0f, 1.0f}, 0xffff0000, 0xff000000},
        {{-1.0f,  1.0f, 1.0f}, 0xffff0000, 0xff000000},
        {{ 0.0f,  1.0f, 1.0f}, 0xffff0000, 0xff000000},
        {{ 0.0f,  0.0f, 1.0f}, 0xffff0000, 0xff000000},
    },
    far_quad1[] =
    {
        {{-1.0f, -1.0f, 0.5f}, 0xffff0000, 0xff000000},
        {{-1.0f,  0.0f, 0.5f}, 0xffff0000, 0xff000000},
        {{ 0.0f,  0.0f, 0.5f}, 0xffff0000, 0xff000000},
        {{ 0.0f, -1.0f, 0.5f}, 0xffff0000, 0xff000000},
    },
    far_quad2[] =
    {
        {{-1.0f,  0.0f, 1.5f}, 0xffff0000, 0xff000000},
        {{-1.0f,  1.0f, 1.5f}, 0xffff0000, 0xff000000},
        {{ 0.0f,  1.0f, 1.5f}, 0xffff0000, 0xff000000},
        {{ 0.0f,  0.0f, 1.5f}, 0xffff0000, 0xff000000},
    };
    /* Untransformed ones. Give them a different diffuse color to make the
     * test look nicer. It also helps making sure that they are drawn
     * correctly. */
    struct
    {
        struct vec4 position;
        DWORD diffuse;
        DWORD specular;
    }
    transformed_1[] =
    {
        {{320.0f,   0.0f, 1.0f, 1.0f}, 0xffffff00, 0xff000000},
        {{640.0f,   0.0f, 1.0f, 1.0f}, 0xffffff00, 0xff000000},
        {{640.0f, 240.0f, 1.0f, 1.0f}, 0xffffff00, 0xff000000},
        {{320.0f, 240.0f, 1.0f, 1.0f}, 0xffffff00, 0xff000000},
    },
    transformed_2[] =
    {
        {{320.0f, 240.0f, 1.0f, 1.0f}, 0xffffff00, 0xff000000},
        {{640.0f, 240.0f, 1.0f, 1.0f}, 0xffffff00, 0xff000000},
        {{640.0f, 480.0f, 1.0f, 1.0f}, 0xffffff00, 0xff000000},
        {{320.0f, 480.0f, 1.0f, 1.0f}, 0xffffff00, 0xff000000},
    };
    WORD Indices[] = {0, 1, 2, 2, 3, 0};
    D3DMATRIX ident_mat =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    D3DMATRIX world_mat1 =
    {
        1.0f, 0.0f,  0.0f, 0.0f,
        0.0f, 1.0f,  0.0f, 0.0f,
        0.0f, 0.0f,  1.0f, 0.0f,
        0.0f, 0.0f, -0.5f, 1.0f,
    };
    D3DMATRIX world_mat2 =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 1.0f,
    };
    D3DMATRIX proj_mat =
    {
        1.0f, 0.0f,  0.0f, 0.0f,
        0.0f, 1.0f,  0.0f, 0.0f,
        0.0f, 0.0f,  1.0f, 0.0f,
        0.0f, 0.0f, -1.0f, 1.0f,
    };

    memset(&caps, 0, sizeof(caps));
    hr = IDirect3DDevice7_GetCaps(device, &caps);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice7_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff00ff, 0.0, 0);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_ZENABLE, FALSE);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_LIGHTING, FALSE);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_FOGENABLE, TRUE);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_FOGCOLOR, 0xFF00FF00 /* A nice green */);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    /* First test: Both table fog and vertex fog off */
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_NONE);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_FOGVERTEXMODE, D3DFOG_NONE);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    /* Start = 0, end = 1. Should be default, but set them */
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_FOGSTART, *((DWORD *) &start));
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_FOGEND, *((DWORD *) &end));
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    if(IDirect3DDevice7_BeginScene(device) == D3D_OK)
    {
        /* Untransformed, vertex fog = NONE, table fog = NONE: Read the fog weighting from the specular color */
        hr = IDirect3DDevice7_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR,
                                                   untransformed_1, 4, Indices, 6, 0);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

        /* That makes it use the Z value */
        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_FOGVERTEXMODE, D3DFOG_LINEAR);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
        /* Untransformed, vertex fog != none (or table fog != none):
         * Use the Z value as input into the equation
         */
        hr = IDirect3DDevice7_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR,
                                                   untransformed_2, 4, Indices, 6, 0);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

        /* Transformed, vertex fog != NONE, pixel fog == NONE: Use specular color alpha component */
        hr = IDirect3DDevice7_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR,
                                                   transformed_1, 4, Indices, 6, 0);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_LINEAR);
        ok( hr == D3D_OK, "Got hr %#lx.\n", hr);
        /* Transformed, table fog != none, vertex anything: Use Z value as input to the fog
         * equation
         */
        hr = IDirect3DDevice7_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR,
                                                   transformed_2, 4, Indices, 6, 0);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

        hr = IDirect3DDevice7_EndScene(device);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    }
    else
    {
        ok(FALSE, "BeginScene failed\n");
    }

    color = getPixelColor(device, 160, 360);
    ok(color_match(color, 0x00FF0000, 1), "Untransformed vertex with no table or vertex fog has color %08x\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color_match(color, 0x0000FF00, 1), "Untransformed vertex with linear vertex fog has color %08x\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color_match(color, 0x00FFFF00, 1), "Transformed vertex with linear vertex fog has color %08x\n", color);
    if(caps.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_FOGTABLE)
    {
        color = getPixelColor(device, 480, 360);
        ok(color_match(color, 0x0000FF00, 1), "Transformed vertex with linear table fog has color %08x\n", color);
    }
    else
    {
        /* Without fog table support the vertex fog is still applied, even though table fog is turned on.
         * The settings above result in no fogging with vertex fog
         */
        color = getPixelColor(device, 480, 120);
        ok(color == 0x00FFFF00, "Transformed vertex with linear vertex fog has color %08x\n", color);
        trace("Info: Table fog not supported by this device\n");
    }

    if (caps.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_FOGTABLE)
    {
        /* A simple fog + non-identity world matrix test */
        hr = IDirect3DDevice7_SetTransform(device, D3DTRANSFORMSTATE_WORLD, &world_mat1);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_FOGTABLEMODE, D3DFOG_LINEAR);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_FOGVERTEXMODE, D3DFOG_NONE);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

        hr = IDirect3DDevice7_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff00ff, 0.0, 0);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

        if (IDirect3DDevice7_BeginScene(device) == D3D_OK)
        {
            hr = IDirect3DDevice7_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST,
                    D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR, far_quad1, 4, Indices, 6, 0);
            ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

            hr = IDirect3DDevice7_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST,
                    D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR, far_quad2, 4, Indices, 6, 0);
            ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

            hr = IDirect3DDevice7_EndScene(device);
            ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
        }
        else
        {
            ok(FALSE, "BeginScene failed\n");
        }

        color = getPixelColor(device, 160, 360);
        ok(color_match(color, 0x00ff0000, 4), "Unfogged quad has color %08x\n", color);
        color = getPixelColor(device, 160, 120);
        ok(color_match(color, 0x0000ff00, 1), "Fogged out quad has color %08x\n", color);

        /* Test fog behavior with an orthogonal (but not identity) projection matrix */
        hr = IDirect3DDevice7_SetTransform(device, D3DTRANSFORMSTATE_WORLD, &world_mat2);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
        hr = IDirect3DDevice7_SetTransform(device, D3DTRANSFORMSTATE_PROJECTION, &proj_mat);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

        hr = IDirect3DDevice7_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff00ff, 0.0, 0);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

        if (IDirect3DDevice7_BeginScene(device) == D3D_OK)
        {
            hr = IDirect3DDevice7_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST,
                    D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR, untransformed_1, 4, Indices, 6, 0);
            ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

            hr = IDirect3DDevice7_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST,
                    D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR, untransformed_2, 4, Indices, 6, 0);
            ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

            hr = IDirect3DDevice7_EndScene(device);
            ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
        }
        else
        {
            ok(FALSE, "BeginScene failed\n");
        }

        color = getPixelColor(device, 160, 360);
        ok(color_match(color, 0x00e51900, 4), "Partially fogged quad has color %08x\n", color);
        color = getPixelColor(device, 160, 120);
        ok(color_match(color, 0x0000ff00, 1), "Fogged out quad has color %08x\n", color);

        hr = IDirect3DDevice7_SetTransform(device, D3DTRANSFORMSTATE_WORLD, &ident_mat);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
        hr = IDirect3DDevice7_SetTransform(device, D3DTRANSFORMSTATE_PROJECTION, &ident_mat);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    }
    else
    {
        skip("D3DPRASTERCAPS_FOGTABLE not supported, skipping some fog tests\n");
    }

    /* Turn off the fog master switch to avoid confusing other tests */
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_FOGENABLE, FALSE);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
}

static void offscreen_test(IDirect3DDevice7 *device)
{
    HRESULT hr;
    IDirectDrawSurface7 *backbuffer = NULL, *offscreen = NULL;
    unsigned int color;
    DDSURFACEDESC2 ddsd;

    static float quad[][5] = {
        {-0.5f, -0.5f, 0.1f, 0.0f, 0.0f},
        {-0.5f,  0.5f, 0.1f, 0.0f, 1.0f},
        { 0.5f, -0.5f, 0.1f, 1.0f, 0.0f},
        { 0.5f,  0.5f, 0.1f, 1.0f, 1.0f},
    };

    hr = IDirect3DDevice7_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff0000, 0.0, 0);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_3DDEVICE;
    hr = IDirectDraw7_CreateSurface(DirectDraw, &ddsd, &offscreen, NULL);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    if(!offscreen) {
        goto out;
    }

    hr = IDirect3DDevice7_GetRenderTarget(device, &backbuffer);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    if(!backbuffer) {
        goto out;
    }

    hr = IDirect3DDevice7_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice7_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice7_SetTextureStageState(device, 0, D3DTSS_MINFILTER, D3DFILTER_NEAREST);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice7_SetTextureStageState(device, 0, D3DTSS_MAGFILTER, D3DFILTER_NEAREST);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_LIGHTING, FALSE);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    if (refdevice) {
        win_skip("Tests would crash on W2K with a refdevice\n");
        goto out;
    }

    if(IDirect3DDevice7_BeginScene(device) == D3D_OK) {
        hr = IDirect3DDevice7_SetRenderTarget(device, offscreen, 0);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
        set_viewport_size(device);
        hr = IDirect3DDevice7_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff00ff, 0.0, 0);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

        /* Draw without textures - Should result in a white quad */
        hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_TEX1, quad, 4, 0);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

        hr = IDirect3DDevice7_SetRenderTarget(device, backbuffer, 0);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
        set_viewport_size(device);

        hr = IDirect3DDevice7_SetTexture(device, 0, offscreen);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

        /* This time with the texture */
        hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_TEX1, quad, 4, 0);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

        IDirect3DDevice7_EndScene(device);
    }

    /* Center quad - should be white */
    color = getPixelColor(device, 320, 240);
    ok(color == 0x00ffffff, "Offscreen failed: Got color 0x%08x, expected 0x00ffffff.\n", color);
    /* Some quad in the cleared part of the texture */
    color = getPixelColor(device, 170, 240);
    ok(color == 0x00ff00ff, "Offscreen failed: Got color 0x%08x, expected 0x00ff00ff.\n", color);
    /* Part of the originally cleared back buffer */
    color = getPixelColor(device, 10, 10);
    ok(color == 0x00ff0000, "Offscreen failed: Got color 0x%08x, expected 0x00ff0000.\n", color);
    if(0) {
        /* Lower left corner of the screen, where back buffer offscreen rendering draws the offscreen texture.
         * It should be red, but the offscreen texture may leave some junk there. Not tested yet. Depending on
         * the offscreen rendering mode this test would succeed or fail
         */
        color = getPixelColor(device, 10, 470);
        ok(color == 0x00ff0000, "Offscreen failed: Got color 0x%08x, expected 0x00ff0000.\n", color);
    }

out:
    hr = IDirect3DDevice7_SetTexture(device, 0, NULL);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);

    /* restore things */
    if(backbuffer) {
        hr = IDirect3DDevice7_SetRenderTarget(device, backbuffer, 0);
        ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
        IDirectDrawSurface7_Release(backbuffer);
    }
    if(offscreen) {
        IDirectDrawSurface7_Release(offscreen);
    }
}

static void test_blend(IDirect3DDevice7 *device)
{
    HRESULT hr;
    IDirectDrawSurface7 *backbuffer = NULL, *offscreen = NULL;
    unsigned int color, red, green, blue;
    DDSURFACEDESC2 ddsd;

    struct
    {
        struct vec3 position;
        DWORD diffuse;
    }
    quad1[] =
    {
        {{-1.0f, -1.0f, 0.1f}, 0x4000ff00},
        {{-1.0f,  0.0f, 0.1f}, 0x4000ff00},
        {{ 1.0f, -1.0f, 0.1f}, 0x4000ff00},
        {{ 1.0f,  0.0f, 0.1f}, 0x4000ff00},
    },
    quad2[] =
    {
        {{-1.0f,  0.0f, 0.1f}, 0xc00000ff},
        {{-1.0f,  1.0f, 0.1f}, 0xc00000ff},
        {{ 1.0f,  0.0f, 0.1f}, 0xc00000ff},
        {{ 1.0f,  1.0f, 0.1f}, 0xc00000ff},
    };
    static float composite_quad[][5] = {
        { 0.0f, -1.0f, 0.1f, 0.0f, 1.0f},
        { 0.0f,  1.0f, 0.1f, 0.0f, 0.0f},
        { 1.0f, -1.0f, 0.1f, 1.0f, 1.0f},
        { 1.0f,  1.0f, 0.1f, 1.0f, 0.0f},
    };

    /* Clear the render target with alpha = 0.5 */
    hr = IDirect3DDevice7_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x80ff0000, 0.0, 0);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT;
    ddsd.dwWidth = 128;
    ddsd.dwHeight = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_3DDEVICE;
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    ddsd.ddpfPixelFormat.dwRGBBitCount      = 32;
    ddsd.ddpfPixelFormat.dwRBitMask         = 0x00ff0000;
    ddsd.ddpfPixelFormat.dwGBitMask         = 0x0000ff00;
    ddsd.ddpfPixelFormat.dwBBitMask         = 0x000000ff;
    ddsd.ddpfPixelFormat.dwRGBAlphaBitMask  = 0xff000000;
    hr = IDirectDraw7_CreateSurface(DirectDraw, &ddsd, &offscreen, NULL);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    if(!offscreen) {
        goto out;
    }
    hr = IDirect3DDevice7_GetRenderTarget(device, &backbuffer);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    if(!backbuffer) {
        goto out;
    }

    hr = IDirect3DDevice7_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice7_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice7_SetTextureStageState(device, 0, D3DTSS_MINFILTER, D3DFILTER_NEAREST);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice7_SetTextureStageState(device, 0, D3DTSS_MAGFILTER, D3DFILTER_NEAREST);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    if (refdevice) {
        win_skip("Tests would crash on W2K with a refdevice\n");
        goto out;
    }

    if(IDirect3DDevice7_BeginScene(device) == D3D_OK) {

        /* Draw two quads, one with src alpha blending, one with dest alpha blending. The
         * SRCALPHA / INVSRCALPHA blend doesn't give any surprises. Colors are blended based on
         * the input alpha
         *
         * The DESTALPHA / INVDESTALPHA do not "work" on the regular buffer because there is no alpha.
         * They give essentially ZERO and ONE blend factors
         */
        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
        hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_DIFFUSE, quad1, 4, 0);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_SRCBLEND, D3DBLEND_DESTALPHA);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVDESTALPHA);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
        hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_DIFFUSE, quad2, 4, 0);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

        /* Switch to the offscreen buffer, and redo the testing. SRCALPHA and DESTALPHA. The offscreen buffer
         * has an alpha channel on its own. Clear the offscreen buffer with alpha = 0.5 again, then draw the
         * quads again. The SRCALPHA/INVSRCALPHA doesn't give any surprises, but the DESTALPHA/INVDESTALPHA
         * blending works as supposed now - blend factor is 0.5 in both cases, not 0.75 as from the input
         * vertices
         */
        hr = IDirect3DDevice7_SetRenderTarget(device, offscreen, 0);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
        set_viewport_size(device);
        hr = IDirect3DDevice7_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x80ff0000, 0.0, 0);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
        hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_DIFFUSE, quad1, 4, 0);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_SRCBLEND, D3DBLEND_DESTALPHA);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVDESTALPHA);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
        hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_DIFFUSE, quad2, 4, 0);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

        hr = IDirect3DDevice7_SetRenderTarget(device, backbuffer, 0);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
        set_viewport_size(device);

        /* Render the offscreen texture onto the frame buffer to be able to compare it regularly.
         * Disable alpha blending for the final composition
         */
        hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

        hr = IDirect3DDevice7_SetTexture(device, 0, offscreen);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
        hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_TEX1, composite_quad, 4, 0);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
        hr = IDirect3DDevice7_SetTexture(device, 0, NULL);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

        hr = IDirect3DDevice7_EndScene(device);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    }

    color = getPixelColor(device, 160, 360);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red >= 0xbe && red <= 0xc0 && green >= 0x39 && green <= 0x41 && blue == 0x00,
       "SRCALPHA on frame buffer returned color 0x%08x, expected 0x00bf4000\n", color);

    color = getPixelColor(device, 160, 120);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red == 0x00 && green == 0x00 && blue >= 0xfe && blue <= 0xff ,
       "DSTALPHA on frame buffer returned color 0x%08x, expected 0x000000ff\n", color);

    color = getPixelColor(device, 480, 360);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red >= 0xbe && red <= 0xc0 && green >= 0x39 && green <= 0x41 && blue == 0x00,
       "SRCALPHA on texture returned color 0x%08x, expected 0x00bf4000\n", color);

    color = getPixelColor(device, 480, 120);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red >= 0x7e && red <= 0x81 && green == 0x00 && blue >= 0x7e && blue <= 0x81,
       "DSTALPHA on texture returned color 0x%08x, expected 0x00800080\n", color);

    out:
    if(offscreen) IDirectDrawSurface7_Release(offscreen);
    if(backbuffer) IDirectDrawSurface7_Release(backbuffer);
}

static void rhw_zero_test(IDirect3DDevice7 *device)
{
/* Test if it will render a quad correctly when vertex rhw = 0 */
    unsigned int color;
    HRESULT hr;

    struct {
        float x, y, z;
        float rhw;
        DWORD diffuse;
        } quad1[] =
    {
        {0, 100, 0, 0, 0xffffffff},
        {0, 0, 0, 0, 0xffffffff},
        {100, 100, 0, 0, 0xffffffff},
        {100, 0, 0, 0, 0xffffffff},
    };

    /* Clear to black */
    hr = IDirect3DDevice7_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0, 0.0, 0);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice7_BeginScene(device);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    if (SUCCEEDED(hr)) {
        hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, quad1, 4, 0);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

        hr = IDirect3DDevice7_EndScene(device);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    }

    color = getPixelColor(device, 5, 5);
    ok(color == 0xffffff ||
       broken(color == 0), /* VMware */
       "Got color %08x, expected 00ffffff\n", color);

    color = getPixelColor(device, 105, 105);
    ok(color == 0, "Got color %08x, expected 00000000\n", color);
}

static DWORD D3D3_getPixelColor(IDirectDraw4 *DirectDraw, IDirectDrawSurface4 *Surface, UINT x, UINT y)
{
    DWORD ret;
    HRESULT hr;
    DDSURFACEDESC2 ddsd;
    RECT rectToLock = {x, y, x+1, y+1};
    IDirectDrawSurface4 *surf = NULL;

    /* Some implementations seem to dislike direct locking on the front buffer. Thus copy the front buffer
     * to an offscreen surface and lock it instead of the front buffer
     */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
    ddsd.dwWidth = 640;
    ddsd.dwHeight = 480;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    hr = IDirectDraw4_CreateSurface(DirectDraw, &ddsd, &surf, NULL);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    if(!surf)
    {
        trace("cannot create helper surface\n");
        return 0xdeadbeef;
    }

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);

    hr = IDirectDrawSurface4_BltFast(surf, 0, 0, Surface, NULL, 0);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    if(FAILED(hr))
    {
        trace("Cannot blit\n");
        ret = 0xdeadbee;
        goto out;
    }

    hr = IDirectDrawSurface4_Lock(surf, &rectToLock, &ddsd, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
    if(FAILED(hr))
    {
        trace("Can't lock the offscreen surface, hr=%#lx\n", hr);
        ret = 0xdeadbeec;
        goto out;
    }

    /* Remove the X channel for now. DirectX and OpenGL have different ideas how to treat it apparently, and it isn't
     * really important for these tests
     */
    ret = ((DWORD *) ddsd.lpSurface)[0] & 0x00ffffff;
    hr = IDirectDrawSurface4_Unlock(surf, NULL);
    if(FAILED(hr))
    {
        trace("Can't unlock the offscreen surface, hr=%#lx\n", hr);
    }

out:
    IDirectDrawSurface4_Release(surf);
    return ret;
}

static void D3D3_ViewportClearTest(void)
{
    HRESULT hr;
    IDirectDraw *DirectDraw1 = NULL;
    IDirectDraw4 *DirectDraw4 = NULL;
    IDirectDrawSurface4 *Primary = NULL;
    IDirect3D3 *Direct3D3 = NULL;
    IDirect3DViewport3 *Viewport3 = NULL;
    IDirect3DViewport3 *SmallViewport3 = NULL;
    IDirect3DDevice3 *Direct3DDevice3 = NULL;
    unsigned int color, red, green, blue;
    WNDCLASSA wc = {0};
    DDSURFACEDESC2 ddsd;
    D3DVIEWPORT2 vp_data;
    D3DRECT rect;
    D3DMATRIX mat =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    struct
    {
        struct vec3 position;
        DWORD diffuse;
    }
    quad[] =
    {
        {{-1.0f, -1.0f, 0.1f}, 0xffffffff},
        {{-1.0f,  1.0f, 0.1f}, 0xffffffff},
        {{ 1.0f,  1.0f, 0.1f}, 0xffffffff},
        {{ 1.0f, -1.0f, 0.1f}, 0xffffffff},
    };

    WORD Indices[] = {0, 1, 2, 2, 3, 0};
    DWORD fvf = D3DFVF_XYZ | D3DFVF_DIFFUSE;

    wc.lpfnWndProc = DefWindowProcA;
    wc.lpszClassName = "D3D3_ViewportClearTest_wc";
    RegisterClassA(&wc);
    window = CreateWindowA("D3D3_ViewportClearTest_wc", "D3D3_ViewportClearTest",
            WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION, 0, 0, 640, 480, 0, 0, 0, 0);

    hr = DirectDrawCreate( NULL, &DirectDraw1, NULL );
    ok(hr == DD_OK || hr == DDERR_NODIRECTDRAWSUPPORT, "Got hr %#lx.\n", hr);
    if(FAILED(hr)) goto out;

    hr = IDirectDraw_SetCooperativeLevel(DirectDraw1, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    if(FAILED(hr)) goto out;

    hr = IDirectDraw_SetDisplayMode(DirectDraw1, 640, 480, 32);
    if(FAILED(hr)) {
        /* 24 bit is fine too */
        hr = IDirectDraw_SetDisplayMode(DirectDraw1, 640, 480, 24);
    }
    ok(hr == DD_OK || hr == DDERR_UNSUPPORTED, "Got hr %#lx.\n", hr);
    if (FAILED(hr)) goto out;

    hr = IDirectDraw_QueryInterface(DirectDraw1, &IID_IDirectDraw4, (void**)&DirectDraw4);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    if(FAILED(hr)) goto out;

    memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
    ddsd.dwSize = sizeof(DDSURFACEDESC2);
    ddsd.dwFlags    = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE;

    hr = IDirectDraw4_CreateSurface(DirectDraw4, &ddsd, &Primary, NULL);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    if(FAILED(hr)) goto out;

    hr = IDirectDraw4_QueryInterface(DirectDraw4, &IID_IDirect3D3, (void**)&Direct3D3);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    if(FAILED(hr)) goto out;

    hr = IDirect3D3_CreateDevice(Direct3D3, &IID_IDirect3DHALDevice, Primary, &Direct3DDevice3, NULL);
    if(FAILED(hr)) {
        trace("Creating a HAL device failed, trying Ref\n");
        hr = IDirect3D3_CreateDevice(Direct3D3, &IID_IDirect3DRefDevice, Primary, &Direct3DDevice3, NULL);
    }
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    if(FAILED(hr)) goto out;

    hr = IDirect3D3_CreateViewport(Direct3D3, &Viewport3, NULL);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    if(FAILED(hr)) goto out;

    hr = IDirect3DDevice3_AddViewport(Direct3DDevice3, Viewport3);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    memset(&vp_data, 0, sizeof(D3DVIEWPORT2));
    vp_data.dwSize = sizeof(D3DVIEWPORT2);
    vp_data.dwWidth = 640;
    vp_data.dwHeight = 480;
    vp_data.dvClipX = -1.0f;
    vp_data.dvClipWidth = 2.0f;
    vp_data.dvClipY = 1.0f;
    vp_data.dvClipHeight = 2.0f;
    vp_data.dvMaxZ = 1.0f;
    hr = IDirect3DViewport3_SetViewport2(Viewport3, &vp_data);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    hr = IDirect3D3_CreateViewport(Direct3D3, &SmallViewport3, NULL);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    if(FAILED(hr)) goto out;

    hr = IDirect3DDevice3_AddViewport(Direct3DDevice3, SmallViewport3);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    memset(&vp_data, 0, sizeof(D3DVIEWPORT2));
    vp_data.dwSize = sizeof(D3DVIEWPORT2);
    vp_data.dwX = 400;
    vp_data.dwY = 100;
    vp_data.dwWidth = 100;
    vp_data.dwHeight = 100;
    vp_data.dvClipX = -1.0f;
    vp_data.dvClipWidth = 2.0f;
    vp_data.dvClipY = 1.0f;
    vp_data.dvClipHeight = 2.0f;
    vp_data.dvMaxZ = 1.0f;
    hr = IDirect3DViewport3_SetViewport2(SmallViewport3, &vp_data);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice3_BeginScene(Direct3DDevice3);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice3_SetTransform(Direct3DDevice3, D3DTRANSFORMSTATE_WORLD, &mat);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice3_SetTransform(Direct3DDevice3, D3DTRANSFORMSTATE_VIEW, &mat);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice3_SetTransform(Direct3DDevice3, D3DTRANSFORMSTATE_PROJECTION, &mat);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(Direct3DDevice3, D3DRENDERSTATE_CLIPPING, FALSE);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(Direct3DDevice3, D3DRENDERSTATE_ZENABLE, FALSE);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(Direct3DDevice3, D3DRENDERSTATE_FOGENABLE, FALSE);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(Direct3DDevice3, D3DRENDERSTATE_STENCILENABLE, FALSE);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(Direct3DDevice3, D3DRENDERSTATE_ALPHATESTENABLE, FALSE);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(Direct3DDevice3, D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(Direct3DDevice3, D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(Direct3DDevice3, D3DRENDERSTATE_LIGHTING, FALSE);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    if (SUCCEEDED(hr)) {
        rect.x1 = rect.y1 = 0;
        rect.x2 = 640;
        rect.y2 = 480;

        hr = IDirect3DViewport3_Clear2(Viewport3, 1, &rect, D3DCLEAR_TARGET, 0x00ff00, 0.0f, 0);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

        hr = IDirect3DViewport3_Clear2(SmallViewport3, 1, &rect, D3DCLEAR_TARGET, 0xff0000, 0.0f, 0);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

        hr = IDirect3DDevice3_EndScene(Direct3DDevice3);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
        }

    color = D3D3_getPixelColor(DirectDraw4, Primary, 5, 5);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red == 0 && green == 0xff && blue == 0, "Got color %08x, expected 0000ff00\n", color);

    color = D3D3_getPixelColor(DirectDraw4, Primary, 405, 105);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red == 0xff && green == 0 && blue == 0, "Got color %08x, expected 00ff0000\n", color);

    /* Test that clearing viewport doesn't interfere with rendering to previously active viewport. */
    hr = IDirect3DDevice3_BeginScene(Direct3DDevice3);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    if (SUCCEEDED(hr)) {
        hr = IDirect3DDevice3_SetCurrentViewport(Direct3DDevice3, SmallViewport3);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

        hr = IDirect3DViewport3_Clear2(Viewport3, 1, &rect, D3DCLEAR_TARGET, 0x000000, 0.0f, 0);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

        hr = IDirect3DDevice3_DrawIndexedPrimitive(Direct3DDevice3, D3DPT_TRIANGLELIST, fvf, quad, 4 /* NumVerts */,
                                                    Indices, 6 /* Indexcount */, 0 /* flags */);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

        hr = IDirect3DDevice3_EndScene(Direct3DDevice3);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
        }

    color = D3D3_getPixelColor(DirectDraw4, Primary, 5, 5);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red == 0 && green == 0 && blue == 0, "Got color %08x, expected 00000000\n", color);

    color = D3D3_getPixelColor(DirectDraw4, Primary, 405, 105);
    red =   (color & 0x00ff0000) >> 16;
    green = (color & 0x0000ff00) >>  8;
    blue =  (color & 0x000000ff);
    ok(red == 0xff && green == 0xff && blue == 0xff, "Got color %08x, expected 00ffffff\n", color);

    out:

    if (SmallViewport3) IDirect3DViewport3_Release(SmallViewport3);
    if (Viewport3) IDirect3DViewport3_Release(Viewport3);
    if (Direct3DDevice3) IDirect3DDevice3_Release(Direct3DDevice3);
    if (Direct3D3) IDirect3D3_Release(Direct3D3);
    if (Primary) IDirectDrawSurface4_Release(Primary);
    if (DirectDraw1) IDirectDraw_Release(DirectDraw1);
    if (DirectDraw4) IDirectDraw4_Release(DirectDraw4);
    if(window) DestroyWindow(window);
}

static COLORREF getPixelColor_GDI(IDirectDrawSurface *Surface, UINT x, UINT y)
{
    COLORREF clr = CLR_INVALID;
    HDC hdc;
    HRESULT hr;

    hr = IDirectDrawSurface_GetDC(Surface, &hdc);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    if (SUCCEEDED(hr)) {
        clr = GetPixel(hdc, x, y);

        hr = IDirectDrawSurface_ReleaseDC(Surface, hdc);
        ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    }

    return clr;
}

static void cubemap_test(IDirect3DDevice7 *device)
{
    IDirect3D7 *d3d;
    IDirectDraw7 *ddraw;
    IDirectDrawSurface7 *cubemap, *surface;
    D3DDEVICEDESC7 d3dcaps;
    HRESULT hr;
    unsigned int color;
    DDSURFACEDESC2 ddsd;
    DDBLTFX DDBltFx;
    DDSCAPS2 caps;
    static float quad[] = {
      -1.0,   -1.0,    0.1,    1.0,    0.0,    0.0, /* Lower left */
       0.0,   -1.0,    0.1,    1.0,    0.0,    0.0,
      -1.0,    0.0,    0.1,    1.0,    0.0,    0.0,
       0.0,    0.0,    0.1,    1.0,    0.0,    0.0,

       0.0,   -1.0,    0.1,    0.0,    1.0,    0.0, /* Lower right */
       1.0,   -1.0,    0.1,    0.0,    1.0,    0.0,
       0.0,    0.0,    0.1,    0.0,    1.0,    0.0,
       1.0,    0.0,    0.1,    0.0,    1.0,    0.0,

       0.0,    0.0,    0.1,    0.0,    0.0,    1.0, /* upper right */
       1.0,    0.0,    0.1,    0.0,    0.0,    1.0,
       0.0,    1.0,    0.1,    0.0,    0.0,    1.0,
       1.0,    1.0,    0.1,    0.0,    0.0,    1.0,

      -1.0,    0.0,    0.1,   -1.0,    0.0,    0.0, /* Upper left */
       0.0,    0.0,    0.1,   -1.0,    0.0,    0.0,
      -1.0,    1.0,    0.1,   -1.0,    0.0,    0.0,
       0.0,    1.0,    0.1,   -1.0,    0.0,    0.0,
    };

    memset(&DDBltFx, 0, sizeof(DDBltFx));
    DDBltFx.dwSize = sizeof(DDBltFx);

    memset(&d3dcaps, 0, sizeof(d3dcaps));
    hr = IDirect3DDevice7_GetCaps(device, &d3dcaps);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    if(!(d3dcaps.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_CUBEMAP))
    {
        skip("No cubemap support\n");
        return;
    }

    hr = IDirect3DDevice7_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xff000000, 0.0, 0);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice7_GetDirect3D(device, &d3d);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3D7_QueryInterface(d3d, &IID_IDirectDraw7, (void **) &ddraw);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    IDirect3D7_Release(d3d);


    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS;
    ddsd.dwWidth = 16;
    ddsd.dwHeight = 16;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX;
    ddsd.ddsCaps.dwCaps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_ALLFACES | DDSCAPS2_TEXTUREMANAGE;
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
    ddsd.ddpfPixelFormat.dwRGBBitCount = 32;
    ddsd.ddpfPixelFormat.dwRBitMask = 0x00FF0000;
    ddsd.ddpfPixelFormat.dwGBitMask = 0x0000FF00;
    ddsd.ddpfPixelFormat.dwBBitMask = 0x000000FF;

    hr = IDirectDraw7_CreateSurface(ddraw, &ddsd, &cubemap, NULL);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    IDirectDraw7_Release(ddraw);

    /* Positive X */
    DDBltFx.dwFillColor = 0x00ff0000;
    hr = IDirectDrawSurface7_Blt(cubemap, NULL, NULL, NULL, DDBLT_COLORFILL, &DDBltFx);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    memset(&caps, 0, sizeof(caps));
    caps.dwCaps = DDSCAPS_TEXTURE;
    caps.dwCaps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX;
    hr = IDirectDrawSurface_GetAttachedSurface(cubemap, &caps, &surface);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    DDBltFx.dwFillColor = 0x0000ffff;
    hr = IDirectDrawSurface7_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL, &DDBltFx);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    caps.dwCaps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ;
    hr = IDirectDrawSurface_GetAttachedSurface(cubemap, &caps, &surface);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    DDBltFx.dwFillColor = 0x0000ff00;
    hr = IDirectDrawSurface7_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL, &DDBltFx);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    caps.dwCaps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ;
    hr = IDirectDrawSurface_GetAttachedSurface(cubemap, &caps, &surface);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    DDBltFx.dwFillColor = 0x000000ff;
    hr = IDirectDrawSurface7_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL, &DDBltFx);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    caps.dwCaps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY;
    hr = IDirectDrawSurface_GetAttachedSurface(cubemap, &caps, &surface);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    DDBltFx.dwFillColor = 0x00ffff00;
    hr = IDirectDrawSurface7_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL, &DDBltFx);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    caps.dwCaps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY;
    hr = IDirectDrawSurface_GetAttachedSurface(cubemap, &caps, &surface);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    DDBltFx.dwFillColor = 0x00ff00ff;
    hr = IDirectDrawSurface7_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL, &DDBltFx);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice7_SetTexture(device, 0, cubemap);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice7_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice7_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice7_BeginScene(device);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    if(SUCCEEDED(hr))
    {
        hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_TEXCOORDSIZE3(0) | D3DFVF_TEX1, quad + 0 * 6, 4, 0);
        if (hr == DDERR_UNSUPPORTED || hr == DDERR_NODIRECTDRAWHW)
        {
            /* VMware */
            win_skip("IDirect3DDevice7_DrawPrimitive is not completely implemented, colors won't be tested\n");
            hr = IDirect3DDevice7_EndScene(device);
            ok(hr == DD_OK, "Got hr %#lx.\n", hr);
            goto out;
        }
        ok(hr == DD_OK, "Got hr %#lx.\n", hr);
        hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_TEXCOORDSIZE3(0) | D3DFVF_TEX1, quad + 4 * 6, 4, 0);
        ok(hr == DD_OK, "Got hr %#lx.\n", hr);
        hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_TEXCOORDSIZE3(0) | D3DFVF_TEX1, quad + 8 * 6, 4, 0);
        ok(hr == DD_OK, "Got hr %#lx.\n", hr);
        hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_TEXCOORDSIZE3(0) | D3DFVF_TEX1, quad + 12* 6, 4, 0);
        ok(hr == DD_OK, "Got hr %#lx.\n", hr);

        hr = IDirect3DDevice7_EndScene(device);
        ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    }
    hr = IDirect3DDevice7_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_DISABLE);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    color = getPixelColor(device, 160, 360); /* lower left quad - positivex */
    ok(color == 0x00ff0000, "DDSCAPS2_CUBEMAP_POSITIVEX has color 0x%08x, expected 0x00ff0000\n", color);
    color = getPixelColor(device, 160, 120); /* upper left quad - negativex */
    ok(color == 0x0000ffff, "DDSCAPS2_CUBEMAP_NEGATIVEX has color 0x%08x, expected 0x0000ffff\n", color);
    color = getPixelColor(device, 480, 360); /* lower right quad - positivey */
    ok(color == 0x00ff00ff, "DDSCAPS2_CUBEMAP_POSITIVEY has color 0x%08x, expected 0x00ff00ff\n", color);
    color = getPixelColor(device, 480, 120); /* upper right quad - positivez */
    ok(color == 0x000000ff, "DDSCAPS2_CUBEMAP_POSITIVEZ has color 0x%08x, expected 0x000000ff\n", color);

out:
    hr = IDirect3DDevice7_SetTexture(device, 0, NULL);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    IDirectDrawSurface7_Release(cubemap);
}

/* This test tests depth clamping / clipping behaviour:
 *   - With software vertex processing, depth values are clamped to the
 *     minimum / maximum z value when D3DRS_CLIPPING is disabled, and clipped
 *     when D3DRS_CLIPPING is enabled. Pretransformed vertices behave the
 *     same as regular vertices here.
 *   - With hardware vertex processing, D3DRS_CLIPPING seems to be ignored.
 *     Normal vertices are always clipped. Pretransformed vertices are
 *     clipped when D3DPMISCCAPS_CLIPTLVERTS is set, clamped when it isn't.
 *   - The viewport's MinZ/MaxZ is irrelevant for this.
 */
static void depth_clamp_test(IDirect3DDevice7 *device)
{
    struct
    {
        struct vec4 position;
        DWORD diffuse;
    }
    quad1[] =
    {
        {{  0.0f,   0.0f,  5.0f, 1.0f}, 0xff002b7f},
        {{640.0f,   0.0f,  5.0f, 1.0f}, 0xff002b7f},
        {{  0.0f, 480.0f,  5.0f, 1.0f}, 0xff002b7f},
        {{640.0f, 480.0f,  5.0f, 1.0f}, 0xff002b7f},
    },
    quad2[] =
    {
        {{  0.0f, 300.0f, 10.0f, 1.0f}, 0xfff9e814},
        {{640.0f, 300.0f, 10.0f, 1.0f}, 0xfff9e814},
        {{  0.0f, 360.0f, 10.0f, 1.0f}, 0xfff9e814},
        {{640.0f, 360.0f, 10.0f, 1.0f}, 0xfff9e814},
    },
    quad3[] =
    {
        {{112.0f, 108.0f,  5.0f, 1.0f}, 0xffffffff},
        {{208.0f, 108.0f,  5.0f, 1.0f}, 0xffffffff},
        {{112.0f, 204.0f,  5.0f, 1.0f}, 0xffffffff},
        {{208.0f, 204.0f,  5.0f, 1.0f}, 0xffffffff},
    },
    quad4[] =
    {
        {{ 42.0f,  41.0f, 10.0f, 1.0f}, 0xffffffff},
        {{112.0f,  41.0f, 10.0f, 1.0f}, 0xffffffff},
        {{ 42.0f, 108.0f, 10.0f, 1.0f}, 0xffffffff},
        {{112.0f, 108.0f, 10.0f, 1.0f}, 0xffffffff},
    };
    struct
    {
        struct vec3 position;
        DWORD diffuse;
    }
    quad5[] =
    {
        {{-0.5f,  0.5f, 10.0f}, 0xff14f914},
        {{ 0.5f,  0.5f, 10.0f}, 0xff14f914},
        {{-0.5f, -0.5f, 10.0f}, 0xff14f914},
        {{ 0.5f, -0.5f, 10.0f}, 0xff14f914},
    },
    quad6[] =
    {
        {{-1.0f, 0.5f,  10.0f}, 0xfff91414},
        {{ 1.0f, 0.5f,  10.0f}, 0xfff91414},
        {{-1.0f, 0.25f, 10.0f}, 0xfff91414},
        {{ 1.0f, 0.25f, 10.0f}, 0xfff91414},
    };

    unsigned int color;
    D3DVIEWPORT7 vp;
    HRESULT hr;

    vp.dwX = 0;
    vp.dwY = 0;
    vp.dwWidth = 640;
    vp.dwHeight = 480;
    vp.dvMinZ = 0.0;
    vp.dvMaxZ = 7.5;

    hr = IDirect3DDevice7_SetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice7_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff00ff00, 1.0, 0);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_CLIPPING, FALSE);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_ZWRITEENABLE, TRUE);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice7_BeginScene(device);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, quad1, 4, 0);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, quad2, 4, 0);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_CLIPPING, TRUE);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, quad3, 4, 0);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, quad4, 4, 0);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_CLIPPING, FALSE);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_DIFFUSE, quad5, 4, 0);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice7_SetRenderState(device, D3DRENDERSTATE_CLIPPING, TRUE);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice7_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_DIFFUSE, quad6, 4, 0);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice7_EndScene(device);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);

    color = getPixelColor(device, 75, 75);
    ok(color_match(color, 0x00ffffff, 1) || color_match(color, 0x0000ff00, 1), "color 0x%08x.\n", color);
    color = getPixelColor(device, 150, 150);
    ok(color_match(color, 0x00ffffff, 1) || color_match(color, 0x0000ff00, 1), "color 0x%08x.\n", color);
    color = getPixelColor(device, 320, 240);
    ok(color_match(color, 0x00002b7f, 1) || color_match(color, 0x0000ff00, 1), "color 0x%08x.\n", color);
    color = getPixelColor(device, 320, 330);
    ok(color_match(color, 0x00f9e814, 1) || color_match(color, 0x0000ff00, 1), "color 0x%08x.\n", color);
    color = getPixelColor(device, 320, 330);
    ok(color_match(color, 0x00f9e814, 1) || color_match(color, 0x0000ff00, 1), "color 0x%08x.\n", color);

    vp.dvMinZ = 0.0;
    vp.dvMaxZ = 1.0;
    hr = IDirect3DDevice7_SetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
}

static void DX1_BackBufferFlipTest(void)
{
    HRESULT hr;
    IDirectDraw *DirectDraw1 = NULL;
    IDirectDrawSurface *Primary = NULL;
    IDirectDrawSurface *Backbuffer = NULL;
    WNDCLASSA wc = {0};
    DDSURFACEDESC ddsd;
    DDBLTFX ddbltfx;
    COLORREF color;
    const DWORD white = 0xffffff;
    const DWORD red = 0xff0000;
    BOOL attached = FALSE;

    wc.lpfnWndProc = DefWindowProcA;
    wc.lpszClassName = "DX1_BackBufferFlipTest_wc";
    RegisterClassA(&wc);
    window = CreateWindowA("DX1_BackBufferFlipTest_wc", "DX1_BackBufferFlipTest",
            WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION, 0, 0, 640, 480, 0, 0, 0, 0);

    hr = DirectDrawCreate( NULL, &DirectDraw1, NULL );
    ok(hr == DD_OK || hr == DDERR_NODIRECTDRAWSUPPORT, "Got hr %#lx.\n", hr);
    if(FAILED(hr)) goto out;

    hr = IDirectDraw_SetCooperativeLevel(DirectDraw1, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    if(FAILED(hr)) goto out;

    hr = IDirectDraw_SetDisplayMode(DirectDraw1, 640, 480, 32);
    if(FAILED(hr)) {
        /* 24 bit is fine too */
        hr = IDirectDraw_SetDisplayMode(DirectDraw1, 640, 480, 24);
    }
    ok(hr == DD_OK || hr == DDERR_UNSUPPORTED, "Got hr %#lx.\n", hr);
    if (FAILED(hr)) {
        goto out;
    }

    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw_CreateSurface(DirectDraw1, &ddsd, &Primary, NULL);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(DDSURFACEDESC);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;
    ddsd.dwWidth = 640;
    ddsd.dwHeight = 480;
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
    ddsd.ddpfPixelFormat.dwRGBBitCount      = 32;
    ddsd.ddpfPixelFormat.dwRBitMask         = 0x00ff0000;
    ddsd.ddpfPixelFormat.dwGBitMask         = 0x0000ff00;
    ddsd.ddpfPixelFormat.dwBBitMask         = 0x000000ff;

    hr = IDirectDraw_CreateSurface(DirectDraw1, &ddsd, &Backbuffer, NULL);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);
    if(FAILED(hr)) goto out;

    hr = IDirectDrawSurface_AddAttachedSurface(Primary, Backbuffer);
    todo_wine ok(hr == DD_OK || broken(hr == DDERR_CANNOTATTACHSURFACE), "Got hr %#lx.\n", hr);
    if (FAILED(hr)) goto out;

    attached = TRUE;

    memset(&ddbltfx, 0, sizeof(ddbltfx));
    ddbltfx.dwSize = sizeof(ddbltfx);
    ddbltfx.dwFillColor = red;
    hr = IDirectDrawSurface_Blt(Backbuffer, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    ddbltfx.dwFillColor = white;
    hr = IDirectDrawSurface_Blt(Primary, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &ddbltfx);
    ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    /* Check it out */
    color = getPixelColor_GDI(Primary, 5, 5);
    ok(GetRValue(color) == 0xFF && GetGValue(color) == 0xFF && GetBValue(color) == 0xFF,
            "got R %02X G %02X B %02X, expected R FF G FF B FF\n",
            GetRValue(color), GetGValue(color), GetBValue(color));

    color = getPixelColor_GDI(Backbuffer, 5, 5);
    ok(GetRValue(color) == 0xFF && GetGValue(color) == 0 && GetBValue(color) == 0,
            "got R %02X G %02X B %02X, expected R FF G 00 B 00\n",
            GetRValue(color), GetGValue(color), GetBValue(color));

    hr = IDirectDrawSurface_Flip(Primary, NULL, DDFLIP_WAIT);
    todo_wine ok(hr == DD_OK, "Got hr %#lx.\n", hr);

    if (hr == DD_OK)
    {
        color = getPixelColor_GDI(Primary, 5, 5);
        ok(GetRValue(color) == 0xFF && GetGValue(color) == 0 && GetBValue(color) == 0,
                "got R %02X G %02X B %02X, expected R FF G 00 B 00\n",
                GetRValue(color), GetGValue(color), GetBValue(color));

        color = getPixelColor_GDI(Backbuffer, 5, 5);
        ok((GetRValue(color) == 0xFF && GetGValue(color) == 0xFF && GetBValue(color) == 0xFF) ||
           broken(GetRValue(color) == 0xFF && GetGValue(color) == 0 && GetBValue(color) == 0),  /* broken driver */
                "got R %02X G %02X B %02X, expected R FF G FF B FF\n",
                GetRValue(color), GetGValue(color), GetBValue(color));
    }

    out:

    if (Backbuffer)
    {
        if (attached)
            IDirectDrawSurface_DeleteAttachedSurface(Primary, 0, Backbuffer);
        IDirectDrawSurface_Release(Backbuffer);
    }
    if (Primary) IDirectDrawSurface_Release(Primary);
    if (DirectDraw1) IDirectDraw_Release(DirectDraw1);
    if (window) DestroyWindow(window);
}

START_TEST(visual)
{
    unsigned int color;
    HRESULT hr;

    if(!createObjects())
    {
        skip("Cannot initialize DirectDraw and Direct3D, skipping\n");
        return;
    }

    /* Check for the reliability of the returned data */
    hr = IDirect3DDevice7_Clear(Direct3DDevice, 0, NULL, D3DCLEAR_TARGET, 0xffff0000, 0.0, 0);
    if(FAILED(hr))
    {
        skip("Clear failed, can't assure correctness of the test results, skipping\n");
        goto cleanup;
    }

    color = getPixelColor(Direct3DDevice, 1, 1);
    if(color !=0x00ff0000)
    {
        skip("Sanity check returned an incorrect color(%08x), can't assure the correctness of the tests, skipping\n", color);
        goto cleanup;
    }

    hr = IDirect3DDevice7_Clear(Direct3DDevice, 0, NULL, D3DCLEAR_TARGET, 0xff00ddee, 0.0, 0);
    if(FAILED(hr))
    {
        skip("Clear failed, can't assure correctness of the test results, skipping\n");
        goto cleanup;
    }

    color = getPixelColor(Direct3DDevice, 639, 479);
    if(color != 0x0000ddee)
    {
        skip("Sanity check returned an incorrect color(%08x), can't assure the correctness of the tests, skipping\n", color);
        goto cleanup;
    }

    /* Now run the tests */
    depth_clamp_test(Direct3DDevice);
    fog_test(Direct3DDevice);
    offscreen_test(Direct3DDevice);
    test_blend(Direct3DDevice);
    rhw_zero_test(Direct3DDevice);
    cubemap_test(Direct3DDevice);

    releaseObjects(); /* release DX7 interfaces to test D3D1 */

    D3D3_ViewportClearTest();
    DX1_BackBufferFlipTest();

    return ;

cleanup:
    releaseObjects();
}
