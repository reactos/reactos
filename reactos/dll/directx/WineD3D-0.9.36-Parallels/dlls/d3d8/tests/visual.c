/*
 * Copyright (C) 2005 Henri Verbeet
 * Copyright (C) 2007 Stefan Dösinger(for CodeWeavers)
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

#define COBJMACROS
#include <d3d8.h>
#include <dxerr8.h>
#include "wine/test.h"

static HMODULE d3d8_handle = 0;

static HWND create_window(void)
{
    WNDCLASS wc = {0};
    HWND ret;
    wc.lpfnWndProc = &DefWindowProc;
    wc.lpszClassName = "d3d8_test_wc";
    RegisterClass(&wc);

    ret = CreateWindow("d3d8_test_wc", "d3d8_test",
                        WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION , 0, 0, 640, 480, 0, 0, 0, 0);
    return ret;
}

static DWORD getPixelColor(IDirect3DDevice8 *device, UINT x, UINT y)
{
    DWORD ret;
    IDirect3DSurface8 *surf;
    IDirect3DTexture8 *tex;
    HRESULT hr;
    D3DLOCKED_RECT lockedRect;
    RECT rectToLock = {x, y, x+1, y+1};

    hr = IDirect3DDevice8_CreateTexture(device, 640, 480, 1 /* Levels */, 0 /* usage */, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &tex);
    if(FAILED(hr) || !tex )  /* This is not a test */
    {
        trace("Can't create an offscreen plain surface to read the render target data, hr=%s\n", DXGetErrorString8(hr));
        return 0xdeadbeef;
    }
    hr = IDirect3DTexture8_GetSurfaceLevel(tex, 0, &surf);
    if(FAILED(hr) || !tex )  /* This is not a test */
    {
        trace("Can't get surface from texture, hr=%s\n", DXGetErrorString8(hr));
        ret = 0xdeadbeee;
        goto out;
    }

    hr = IDirect3DDevice8_GetFrontBuffer(device, surf);
    if(FAILED(hr))
    {
        trace("Can't read the front buffer data, hr=%s\n", DXGetErrorString8(hr));
        ret = 0xdeadbeed;
        goto out;
    }

    hr = IDirect3DSurface8_LockRect(surf, &lockedRect, &rectToLock, D3DLOCK_READONLY);
    if(FAILED(hr))
    {
        trace("Can't lock the offscreen surface, hr=%s\n", DXGetErrorString8(hr));
        ret = 0xdeadbeec;
        goto out;
    }
    /* Remove the X channel for now. DirectX and OpenGL have different ideas how to treat it apparently, and it isn't
     * really important for these tests
     */
    ret = ((DWORD *) lockedRect.pBits)[0] & 0x00ffffff;
    hr = IDirect3DSurface8_UnlockRect(surf);
    if(FAILED(hr))
    {
        trace("Can't unlock the offscreen surface, hr=%s\n", DXGetErrorString8(hr));
    }

out:
    if(surf) IDirect3DSurface8_Release(surf);
    if(tex) IDirect3DTexture8_Release(tex);
    return ret;
}

static IDirect3DDevice8 *init_d3d8(void)
{
    IDirect3D8 * (__stdcall * d3d8_create)(UINT SDKVersion) = 0;
    IDirect3D8 *d3d8_ptr = 0;
    IDirect3DDevice8 *device_ptr = 0;
    D3DPRESENT_PARAMETERS present_parameters;
    HRESULT hr;

    d3d8_create = (void *)GetProcAddress(d3d8_handle, "Direct3DCreate8");
    ok(d3d8_create != NULL, "Failed to get address of Direct3DCreate8\n");
    if (!d3d8_create) return NULL;

    d3d8_ptr = d3d8_create(D3D_SDK_VERSION);
    ok(d3d8_ptr != NULL, "Failed to create IDirect3D8 object\n");
    if (!d3d8_ptr) return NULL;

    ZeroMemory(&present_parameters, sizeof(present_parameters));
    present_parameters.Windowed = FALSE;
    present_parameters.hDeviceWindow = create_window();
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.BackBufferWidth = 640;
    present_parameters.BackBufferHeight = 480;
    present_parameters.BackBufferFormat = D3DFMT_X8R8G8B8;

    hr = IDirect3D8_CreateDevice(d3d8_ptr, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, present_parameters.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device_ptr);
    ok(hr == D3D_OK, "IDirect3D_CreateDevice returned: %s\n", DXGetErrorString8(hr));

    return device_ptr;
}

struct vertex
{
    float x, y, z;
    DWORD diffuse;
};

struct nvertex
{
    float x, y, z;
    float nx, ny, nz;
    DWORD diffuse;
};

static void lighting_test(IDirect3DDevice8 *device)
{
    HRESULT hr;
    DWORD fvf = D3DFVF_XYZ | D3DFVF_DIFFUSE;
    DWORD nfvf = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_NORMAL;
    DWORD color;

    float mat[16] = { 1.0, 0.0, 0.0, 0.0,
                      0.0, 1.0, 0.0, 0.0,
                      0.0, 0.0, 1.0, 0.0,
                      0.0, 0.0, 0.0, 1.0 };

    struct vertex unlitquad[] =
    {
        {-1.0,  -1.0,    0.1,                           0xffff0000},
        {-1.0,   0.0,    0.1,                           0xffff0000},
        { 0.0,   0.0,    0.1,                           0xffff0000},
        { 0.0,  -1.0,    0.1,                           0xffff0000},
    };
    struct vertex litquad[] =
    {
        {-1.0,   0.0,    0.1,                           0xff00ff00},
        {-1.0,   1.0,    0.1,                           0xff00ff00},
        { 0.0,   1.0,    0.1,                           0xff00ff00},
        { 0.0,   0.0,    0.1,                           0xff00ff00},
    };
    struct nvertex unlitnquad[] =
    {
        { 0.0,  -1.0,    0.1,   1.0,    1.0,    1.0,    0xff0000ff},
        { 0.0,   0.0,    0.1,   1.0,    1.0,    1.0,    0xff0000ff},
        { 1.0,   0.0,    0.1,   1.0,    1.0,    1.0,    0xff0000ff},
        { 1.0,  -1.0,    0.1,   1.0,    1.0,    1.0,    0xff0000ff},
    };
    struct nvertex litnquad[] =
    {
        { 0.0,   0.0,    0.1,   1.0,    1.0,    1.0,    0xffffff00},
        { 0.0,   1.0,    0.1,   1.0,    1.0,    1.0,    0xffffff00},
        { 1.0,   1.0,    0.1,   1.0,    1.0,    1.0,    0xffffff00},
        { 1.0,   0.0,    0.1,   1.0,    1.0,    1.0,    0xffffff00},
    };
    WORD Indices[] = {0, 1, 2, 2, 3, 0};

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice8_Clear failed with %s\n", DXGetErrorString8(hr));

    /* Setup some states that may cause issues */
    hr = IDirect3DDevice8_SetTransform(device, D3DTS_WORLDMATRIX(0), (D3DMATRIX *) mat);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetTransform returned %s\n", DXGetErrorString8(hr));
    hr = IDirect3DDevice8_SetTransform(device, D3DTS_VIEW, (D3DMATRIX *)mat);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetTransform returned %s\n", DXGetErrorString8(hr));
    hr = IDirect3DDevice8_SetTransform(device, D3DTS_PROJECTION, (D3DMATRIX *) mat);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetTransform returned %s\n", DXGetErrorString8(hr));
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_CLIPPING, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetRenderState returned %s\n", DXGetErrorString8(hr));
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ZENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetRenderState returned %s\n", DXGetErrorString8(hr));
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetRenderState returned %s\n", DXGetErrorString8(hr));
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_STENCILENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetRenderState returned %s\n", DXGetErrorString8(hr));
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ALPHATESTENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetRenderState returned %s\n", DXGetErrorString8(hr));
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_ALPHABLENDENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetRenderState returned %s\n", DXGetErrorString8(hr));
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_CULLMODE, D3DCULL_NONE);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetRenderState failed with %s\n", DXGetErrorString8(hr));
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetRenderState failed with %s\n", DXGetErrorString8(hr));

    hr = IDirect3DDevice8_SetVertexShader(device, fvf);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetVertexShader returned %s\n", DXGetErrorString8(hr));

    hr = IDirect3DDevice8_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice8_BeginScene failed with %s\n", DXGetErrorString8(hr));
    if(hr == D3D_OK)
    {
        /* No lights are defined... That means, lit vertices should be entirely black */
        hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
        ok(hr == D3D_OK, "IDirect3DDevice8_SetRenderState returned %s\n", DXGetErrorString8(hr));
        hr = IDirect3DDevice8_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                    2 /*PrimCount */, Indices, D3DFMT_INDEX16, unlitquad, sizeof(unlitquad[0]));
        ok(hr == D3D_OK, "IDirect3DDevice8_DrawIndexedPrimitiveUP failed with %s\n", DXGetErrorString8(hr));

        hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, TRUE);
        ok(hr == D3D_OK, "IDirect3DDevice8_SetRenderState returned %s\n", DXGetErrorString8(hr));
        hr = IDirect3DDevice8_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                    2 /*PrimCount */, Indices, D3DFMT_INDEX16, litquad, sizeof(litquad[0]));
        ok(hr == D3D_OK, "IDirect3DDevice8_DrawIndexedPrimitiveUP failed with %s\n", DXGetErrorString8(hr));

        hr = IDirect3DDevice8_SetVertexShader(device, nfvf);
        ok(hr == D3D_OK, "IDirect3DDevice8_SetVertexShader failed with %s\n", DXGetErrorString8(hr));

        hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
        ok(hr == D3D_OK, "IDirect3DDevice8_SetRenderState returned %s\n", DXGetErrorString8(hr));
        hr = IDirect3DDevice8_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                    2 /*PrimCount */, Indices, D3DFMT_INDEX16, unlitnquad, sizeof(unlitnquad[0]));
        ok(hr == D3D_OK, "IDirect3DDevice8_DrawIndexedPrimitiveUP failed with %s\n", DXGetErrorString8(hr));

        hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, TRUE);
        ok(hr == D3D_OK, "IDirect3DDevice8_SetRenderState returned %s\n", DXGetErrorString8(hr));
        hr = IDirect3DDevice8_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                    2 /*PrimCount */, Indices, D3DFMT_INDEX16, litnquad, sizeof(litnquad[0]));
        ok(hr == D3D_OK, "IDirect3DDevice8_DrawIndexedPrimitiveUP failed with %s\n", DXGetErrorString8(hr));

        IDirect3DDevice8_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice8_EndScene failed with %s\n", DXGetErrorString8(hr));
    }

    IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);

    color = getPixelColor(device, 160, 360); /* lower left quad - unlit without normals */
    ok(color == 0x00ff0000, "Unlit quad without normals has color %08x\n", color);
    color = getPixelColor(device, 160, 120); /* upper left quad - lit without normals */
    ok(color == 0x00000000, "Lit quad without normals has color %08x\n", color);
    color = getPixelColor(device, 480, 360); /* lower left quad - unlit width normals */
    ok(color == 0x000000ff, "Unlit quad width normals has color %08x\n", color);
    color = getPixelColor(device, 480, 120); /* upper left quad - lit width normals */
    ok(color == 0x00000000, "Lit quad width normals has color %08x\n", color);

    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice8_SetRenderState returned %s\n", DXGetErrorString8(hr));
}

static void clear_test(IDirect3DDevice8 *device)
{
    /* Tests the correctness of clearing parameters */
    HRESULT hr;
    D3DRECT rect[2];
    D3DRECT rect_negneg;
    DWORD color;

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice8_Clear failed with %s\n", DXGetErrorString8(hr));

    /* Positive x, negative y */
    rect[0].x1 = 0;
    rect[0].y1 = 480;
    rect[0].x2 = 320;
    rect[0].y2 = 240;

    /* Positive x, positive y */
    rect[1].x1 = 0;
    rect[1].y1 = 0;
    rect[1].x2 = 320;
    rect[1].y2 = 240;
    /* Clear 2 rectangles with one call. Shows that a positive value is returned, but the negative rectangle
     * is ignored, the positive is still cleared afterwards
     */
    hr = IDirect3DDevice8_Clear(device, 2, rect, D3DCLEAR_TARGET, 0xffff0000, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice8_Clear failed with %s\n", DXGetErrorString8(hr));

    /* negative x, negative y */
    rect_negneg.x1 = 640;
    rect_negneg.x1 = 240;
    rect_negneg.x2 = 320;
    rect_negneg.y2 = 0;
    hr = IDirect3DDevice8_Clear(device, 1, &rect_negneg, D3DCLEAR_TARGET, 0xff00ff00, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice8_Clear failed with %s\n", DXGetErrorString8(hr));

    IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);

    color = getPixelColor(device, 160, 360); /* lower left quad */
    ok(color == 0x00ffffff, "Clear rectangle 3(pos, neg) has color %08x\n", color);
    color = getPixelColor(device, 160, 120); /* upper left quad */
    ok(color == 0x00ff0000, "Clear rectangle 1(pos, pos) has color %08x\n", color);
    color = getPixelColor(device, 480, 360); /* lower right quad  */
    ok(color == 0x00ffffff, "Clear rectangle 4(NULL) has color %08x\n", color);
    color = getPixelColor(device, 480, 120); /* upper right quad */
    ok(color == 0x00ffffff, "Clear rectangle 4(neg, neg) has color %08x\n", color);
}

struct sVertex {
    float x, y, z;
    DWORD diffuse;
    DWORD specular;
};

struct sVertexT {
    float x, y, z, rhw;
    DWORD diffuse;
    DWORD specular;
};

static void fog_test(IDirect3DDevice8 *device)
{
    HRESULT hr;
    DWORD color;
    float start = 0.0, end = 1.0;

    /* Gets full z based fog with linear fog, no fog with specular color */
    struct sVertex unstransformed_1[] = {
        {-1,    -1,   0.1,          0xFFFF0000,     0xFF000000  },
        {-1,     0,   0.1,          0xFFFF0000,     0xFF000000  },
        { 0,     0,   0.1,          0xFFFF0000,     0xFF000000  },
        { 0,    -1,   0.1,          0xFFFF0000,     0xFF000000  },
    };
    /* Ok, I am too lazy to deal with transform matrices */
    struct sVertex unstransformed_2[] = {
        {-1,     0,   1.0,          0xFFFF0000,     0xFF000000  },
        {-1,     1,   1.0,          0xFFFF0000,     0xFF000000  },
        { 0,     1,   1.0,          0xFFFF0000,     0xFF000000  },
        { 0,     0,   1.0,          0xFFFF0000,     0xFF000000  },
    };
    /* Untransformed ones. Give them a different diffuse color to make the test look
     * nicer. It also makes making sure that they are drawn correctly easier.
     */
    struct sVertexT transformed_1[] = {
        {320,    0,   1.0,  1.0,    0xFFFFFF00,     0xFF000000  },
        {640,    0,   1.0,  1.0,    0xFFFFFF00,     0xFF000000  },
        {640,  240,   1.0,  1.0,    0xFFFFFF00,     0xFF000000  },
        {320,  240,   1.0,  1.0,    0xFFFFFF00,     0xFF000000  },
    };
    struct sVertexT transformed_2[] = {
        {320,  240,   1.0,  1.0,    0xFFFFFF00,     0xFF000000  },
        {640,  240,   1.0,  1.0,    0xFFFFFF00,     0xFF000000  },
        {640,  480,   1.0,  1.0,    0xFFFFFF00,     0xFF000000  },
        {320,  480,   1.0,  1.0,    0xFFFFFF00,     0xFF000000  },
    };
    WORD Indices[] = {0, 1, 2, 2, 3, 0};

    hr = IDirect3DDevice8_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff00ff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice8_Clear returned %s\n", DXGetErrorString8(hr));

    /* Setup initial states: No lighting, fog on, fog color */
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(hr == D3D_OK, "Turning off lighting returned %s\n", DXGetErrorString8(hr));
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGENABLE, TRUE);
    ok(hr == D3D_OK, "Turning on fog calculations returned %s\n", DXGetErrorString8(hr));
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGCOLOR, 0xFF00FF00 /* A nice green */);
    ok(hr == D3D_OK, "Turning on fog calculations returned %s\n", DXGetErrorString8(hr));

    /* First test: Both table fog and vertex fog off */
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGTABLEMODE, D3DFOG_NONE);
    ok(hr == D3D_OK, "Turning off table fog returned %s\n", DXGetErrorString8(hr));
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGVERTEXMODE, D3DFOG_NONE);
    ok(hr == D3D_OK, "Turning off table fog returned %s\n", DXGetErrorString8(hr));

    /* Start = 0, end = 1. Should be default, but set them */
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGSTART, *((DWORD *) &start));
    ok(hr == D3D_OK, "Setting fog start returned %s\n", DXGetErrorString8(hr));
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGEND, *((DWORD *) &end));
    ok(hr == D3D_OK, "Setting fog start returned %s\n", DXGetErrorString8(hr));

    if(IDirect3DDevice8_BeginScene(device) == D3D_OK)
    {
        hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR);
        ok( hr == D3D_OK, "SetVertexShader returned %s\n", DXGetErrorString8(hr));
        /* Untransformed, vertex fog = NONE, table fog = NONE: Read the fog weighting from the specular color */
        hr = IDirect3DDevice8_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                     2 /*PrimCount */, Indices, D3DFMT_INDEX16, unstransformed_1,
                                                     sizeof(unstransformed_1[0]));
        ok(hr == D3D_OK, "DrawIndexedPrimitiveUP returned %s\n", DXGetErrorString8(hr));

        /* That makes it use the Z value */
        hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
        ok(hr == D3D_OK, "Turning off table fog returned %s\n", DXGetErrorString8(hr));
        /* Untransformed, vertex fog != none (or table fog != none):
         * Use the Z value as input into the equation
         */
        hr = IDirect3DDevice8_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                     2 /*PrimCount */, Indices, D3DFMT_INDEX16, unstransformed_2,
                                                     sizeof(unstransformed_1[0]));
        ok(hr == D3D_OK, "DrawIndexedPrimitiveUP returned %s\n", DXGetErrorString8(hr));

        /* transformed verts */
        hr = IDirect3DDevice8_SetVertexShader(device, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR);
        ok( hr == D3D_OK, "SetVertexShader returned %s\n", DXGetErrorString8(hr));
        /* Transformed, vertex fog != NONE, pixel fog == NONE: Use specular color alpha component */
        hr = IDirect3DDevice8_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                     2 /*PrimCount */, Indices, D3DFMT_INDEX16, transformed_1,
                                                     sizeof(transformed_1[0]));
        ok(hr == D3D_OK, "DrawIndexedPrimitiveUP returned %s\n", DXGetErrorString8(hr));

        hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGTABLEMODE, D3DFOG_LINEAR);
        ok( hr == D3D_OK, "Setting fog table mode to D3DFOG_LINEAR returned %s\n", DXGetErrorString8(hr));
        /* Transformed, table fog != none, vertex anything: Use Z value as input to the fog
         * equation
         */
        hr = IDirect3DDevice8_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                     2 /*PrimCount */, Indices, D3DFMT_INDEX16, transformed_2,
                                                     sizeof(transformed_2[0]));

        hr = IDirect3DDevice8_EndScene(device);
        ok(hr == D3D_OK, "EndScene returned %s\n", DXGetErrorString8(hr));
    }
    else
    {
        ok(FALSE, "BeginScene failed\n");
    }

    IDirect3DDevice8_Present(device, NULL, NULL, NULL, NULL);
    color = getPixelColor(device, 160, 360);
    ok(color == 0x00FF0000, "Untransformed vertex with no table or vertex fog has color %08x\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color == 0x0000FF00, "Untransformed vertex with linear vertex fog has color %08x\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color == 0x00FFFF00, "Transformed vertex with linear vertex fog has color %08x\n", color);
    color = getPixelColor(device, 480, 360);
    ok(color == 0x0000FF00, "Transformed vertex with linear table fog has color %08x\n", color);

    /* Turn off the fog master switch to avoid confusing other tests */
    hr = IDirect3DDevice8_SetRenderState(device, D3DRS_FOGENABLE, FALSE);
    ok(hr == D3D_OK, "Turning off fog calculations returned %s\n", DXGetErrorString8(hr));
}

START_TEST(visual)
{
    IDirect3DDevice8 *device_ptr;
    HRESULT hr;
    DWORD color;

    d3d8_handle = LoadLibraryA("d3d8.dll");
    if (!d3d8_handle)
    {
        skip("Could not load d3d8.dll\n");
        return;
    }

    device_ptr = init_d3d8();
    if (!device_ptr) return;

    /* Check for the reliability of the returned data */
    hr = IDirect3DDevice8_Clear(device_ptr, 0, NULL, D3DCLEAR_TARGET, 0xffff0000, 0.0, 0);
    if(FAILED(hr))
    {
        trace("Clear failed, can't assure correctness of the test results, skipping\n");
        goto cleanup;
    }
    IDirect3DDevice8_Present(device_ptr, NULL, NULL, NULL, NULL);

    color = getPixelColor(device_ptr, 1, 1);
    if(color !=0x00ff0000)
    {
        trace("Sanity check returned an incorrect color(%08x), can't assure the correctness of the tests, skipping\n", color);
        goto cleanup;
    }

    hr = IDirect3DDevice8_Clear(device_ptr, 0, NULL, D3DCLEAR_TARGET, 0xff00ddee, 0.0, 0);
    if(FAILED(hr))
    {
        trace("Clear failed, can't assure correctness of the test results, skipping\n");
        goto cleanup;
    }
    IDirect3DDevice8_Present(device_ptr, NULL, NULL, NULL, NULL);

    color = getPixelColor(device_ptr, 639, 479);
    if(color != 0x0000ddee)
    {
        trace("Sanity check returned an incorrect color(%08x), can't assure the correctness of the tests, skipping\n", color);
        goto cleanup;
    }

    /* Now run the real test */
    lighting_test(device_ptr);
    clear_test(device_ptr);
    fog_test(device_ptr);

cleanup:
    if(device_ptr) IDirect3DDevice8_Release(device_ptr);
}
