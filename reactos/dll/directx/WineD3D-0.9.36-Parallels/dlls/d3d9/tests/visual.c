/*
 * Copyright 2005, 2007 Henri Verbeet
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

/* This test framework allows limited testing of rendering results. Things are rendered, shown on
 * the framebuffer, read back from there and compared to expected colors.
 *
 * However, neither d3d nor opengl is guaranteed to be pixel exact, and thus the capability of this test
 * is rather limited. As a general guideline for adding tests, do not rely on corner pixels. Draw a big enough
 * area which shows specific behavior(like a quad on the whole screen), and try to get resulting colos with
 * all bits set or unset in all channels(like pure red, green, blue, white, black). Hopefully everything that
 * causes visible results in games can be tested in a way that does not depend on pixel exactness
 */

#define COBJMACROS
#include <d3d9.h>
#include <dxerr9.h>
#include "wine/test.h"

static HMODULE d3d9_handle = 0;

static HWND create_window(void)
{
    WNDCLASS wc = {0};
    HWND ret;
    wc.lpfnWndProc = &DefWindowProc;
    wc.lpszClassName = "d3d9_test_wc";
    RegisterClass(&wc);

    ret = CreateWindow("d3d9_test_wc", "d3d9_test",
                        WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION , 0, 0, 640, 480, 0, 0, 0, 0);
    return ret;
}

static DWORD getPixelColor(IDirect3DDevice9 *device, UINT x, UINT y)
{
    DWORD ret;
    IDirect3DSurface9 *surf;
    HRESULT hr;
    D3DLOCKED_RECT lockedRect;
    RECT rectToLock = {x, y, x+1, y+1};

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 640, 480, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &surf, NULL);
    if(FAILED(hr) || !surf )  /* This is not a test */
    {
        trace("Can't create an offscreen plain surface to read the render target data, hr=%s\n", DXGetErrorString9(hr));
        return 0xdeadbeef;
    }

    hr = IDirect3DDevice9_GetFrontBufferData(device, 0, surf);
    if(FAILED(hr))
    {
        trace("Can't read the front buffer data, hr=%s\n", DXGetErrorString9(hr));
        ret = 0xdeadbeed;
        goto out;
    }

    hr = IDirect3DSurface9_LockRect(surf, &lockedRect, &rectToLock, D3DLOCK_READONLY);
    if(FAILED(hr))
    {
        trace("Can't lock the offscreen surface, hr=%s\n", DXGetErrorString9(hr));
        ret = 0xdeadbeec;
        goto out;
    }

    /* Remove the X channel for now. DirectX and OpenGL have different ideas how to treat it apparently, and it isn't
     * really important for these tests
     */
    ret = ((DWORD *) lockedRect.pBits)[0] & 0x00ffffff;
    hr = IDirect3DSurface9_UnlockRect(surf);
    if(FAILED(hr))
    {
        trace("Can't unlock the offscreen surface, hr=%s\n", DXGetErrorString9(hr));
    }

out:
    if(surf) IDirect3DSurface9_Release(surf);
    return ret;
}

static IDirect3DDevice9 *init_d3d9(void)
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
    present_parameters.Windowed = FALSE;
    present_parameters.hDeviceWindow = create_window();
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.BackBufferWidth = 640;
    present_parameters.BackBufferHeight = 480;
    present_parameters.BackBufferFormat = D3DFMT_X8R8G8B8;

    hr = IDirect3D9_CreateDevice(d3d9_ptr, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, present_parameters.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device_ptr);
    ok(hr == D3D_OK, "IDirect3D_CreateDevice returned: %s\n", DXGetErrorString9(hr));

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

static void lighting_test(IDirect3DDevice9 *device)
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

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %s\n", DXGetErrorString9(hr));

    /* Setup some states that may cause issues */
    hr = IDirect3DDevice9_SetTransform(device, D3DTS_WORLDMATRIX(0), (D3DMATRIX *) mat);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetTransform(device, D3DTS_VIEW, (D3DMATRIX *)mat);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetTransform(device, D3DTS_PROJECTION, (D3DMATRIX *) mat);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetTransform returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_CLIPPING, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_STENCILENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHATESTENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ALPHABLENDENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_SCISSORTESTENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_CULLMODE, D3DCULL_NONE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed with %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLED_RED | D3DCOLORWRITEENABLED_GREEN | D3DCOLORWRITEENABLED_BLUE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed with %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_SetFVF(device, fvf);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF returned %s\n", DXGetErrorString9(hr));

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %s\n", DXGetErrorString9(hr));
    if(hr == D3D_OK)
    {
        /* No lights are defined... That means, lit vertices should be entirely black */
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                    2 /*PrimCount */, Indices, D3DFMT_INDEX16, unlitquad, sizeof(unlitquad[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawIndexedPrimitiveUP failed with %s\n", DXGetErrorString9(hr));

        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, TRUE);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                    2 /*PrimCount */, Indices, D3DFMT_INDEX16, litquad, sizeof(litquad[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawIndexedPrimitiveUP failed with %s\n", DXGetErrorString9(hr));

        hr = IDirect3DDevice9_SetFVF(device, nfvf);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetFVF failed with %s\n", DXGetErrorString9(hr));

        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                    2 /*PrimCount */, Indices, D3DFMT_INDEX16, unlitnquad, sizeof(unlitnquad[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawIndexedPrimitiveUP failed with %s\n", DXGetErrorString9(hr));

        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, TRUE);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                    2 /*PrimCount */, Indices, D3DFMT_INDEX16, litnquad, sizeof(litnquad[0]));
        ok(hr == D3D_OK, "IDirect3DDevice9_DrawIndexedPrimitiveUP failed with %s\n", DXGetErrorString9(hr));

        IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %s\n", DXGetErrorString9(hr));
    }

    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

    color = getPixelColor(device, 160, 360); /* lower left quad - unlit without normals */
    ok(color == 0x00ff0000, "Unlit quad without normals has color %08x\n", color);
    color = getPixelColor(device, 160, 120); /* upper left quad - lit without normals */
    ok(color == 0x00000000, "Lit quad without normals has color %08x\n", color);
    color = getPixelColor(device, 480, 360); /* lower left quad - unlit width normals */
    ok(color == 0x000000ff, "Unlit quad width normals has color %08x\n", color);
    color = getPixelColor(device, 480, 120); /* upper left quad - lit width normals */
    ok(color == 0x00000000, "Lit quad width normals has color %08x\n", color);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));
}

static void clear_test(IDirect3DDevice9 *device)
{
    /* Tests the correctness of clearing parameters */
    HRESULT hr;
    D3DRECT rect[2];
    D3DRECT rect_negneg;
    DWORD color;

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %s\n", DXGetErrorString9(hr));

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
    hr = IDirect3DDevice9_Clear(device, 2, rect, D3DCLEAR_TARGET, 0xffff0000, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %s\n", DXGetErrorString9(hr));

    /* negative x, negative y */
    rect_negneg.x1 = 640;
    rect_negneg.x1 = 240;
    rect_negneg.x2 = 320;
    rect_negneg.y2 = 0;
    hr = IDirect3DDevice9_Clear(device, 1, &rect_negneg, D3DCLEAR_TARGET, 0xff00ff00, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %s\n", DXGetErrorString9(hr));

    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

    color = getPixelColor(device, 160, 360); /* lower left quad */
    ok(color == 0x00ffffff, "Clear rectangle 3(pos, neg) has color %08x\n", color);
    color = getPixelColor(device, 160, 120); /* upper left quad */
    ok(color == 0x00ff0000, "Clear rectangle 1(pos, pos) has color %08x\n", color);
    color = getPixelColor(device, 480, 360); /* lower right quad  */
    ok(color == 0x00ffffff, "Clear rectangle 4(NULL) has color %08x\n", color);
    color = getPixelColor(device, 480, 120); /* upper right quad */
    ok(color == 0x00ffffff, "Clear rectangle 4(neg, neg) has color %08x\n", color);
}

typedef struct {
    float in[4];
    DWORD out;
} test_data_t;

/*
 *  c7      rounded     ARGB
 * -2.4     -2          0x00ffff00
 * -1.6     -2          0x00ffff00
 * -0.4      0          0x0000ffff
 *  0.4      0          0x0000ffff
 *  1.6      2          0x00ff00ff
 *  2.4      2          0x00ff00ff
 */
static void test_mova(IDirect3DDevice9 *device)
{
    static const DWORD mova_test[] = {
        0xfffe0200,                                                             /* vs_2_0                       */
        0x0200001f, 0x80000000, 0x900f0000,                                     /* dcl_position v0              */
        0x05000051, 0xa00f0000, 0x3f800000, 0x00000000, 0x00000000, 0x3f800000, /* def c0, 1.0, 0.0, 0.0, 1.0   */
        0x05000051, 0xa00f0001, 0x3f800000, 0x3f800000, 0x00000000, 0x3f800000, /* def c1, 1.0, 1.0, 0.0, 1.0   */
        0x05000051, 0xa00f0002, 0x00000000, 0x3f800000, 0x00000000, 0x3f800000, /* def c2, 0.0, 1.0, 0.0, 1.0   */
        0x05000051, 0xa00f0003, 0x00000000, 0x3f800000, 0x3f800000, 0x3f800000, /* def c3, 0.0, 1.0, 1.0, 1.0   */
        0x05000051, 0xa00f0004, 0x00000000, 0x00000000, 0x3f800000, 0x3f800000, /* def c4, 0.0, 0.0, 1.0, 1.0   */
        0x05000051, 0xa00f0005, 0x3f800000, 0x00000000, 0x3f800000, 0x3f800000, /* def c5, 1.0, 0.0, 1.0, 1.0   */
        0x05000051, 0xa00f0006, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000, /* def c6, 1.0, 1.0, 1.0, 1.0   */
        0x0200002e, 0xb0010000, 0xa0000007,                                     /* mova a0.x, c7.x              */
        0x03000001, 0xd00f0000, 0xa0e42003, 0xb0000000,                         /* mov oD0, c[a0.x + 3]         */
        0x02000001, 0xc00f0000, 0x90e40000,                                     /* mov oPos, v0                 */
        0x0000ffff                                                              /* END                          */
    };

    static const test_data_t test_data[] = {
        {{-2.4f, 0.0f, 0.0f, 0.0f}, 0x00ffff00},
        {{-1.6f, 0.0f, 0.0f, 0.0f}, 0x00ffff00},
        {{-0.4f, 0.0f, 0.0f, 0.0f}, 0x0000ffff},
        {{ 0.4f, 0.0f, 0.0f, 0.0f}, 0x0000ffff},
        {{ 1.6f, 0.0f, 0.0f, 0.0f}, 0x00ff00ff},
        {{ 2.4f, 0.0f, 0.0f, 0.0f}, 0x00ff00ff}
    };

    static const float quad[][3] = {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f,  1.0f, 0.0f},
        { 1.0f, -1.0f, 0.0f},
        { 1.0f,  1.0f, 0.0f},
    };

    static const D3DVERTEXELEMENT9 decl_elements[] = {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        D3DDECL_END()
    };

    IDirect3DVertexDeclaration9 *vertex_declaration = NULL;
    IDirect3DVertexShader9 *mova_shader = NULL;
    HRESULT hr;
    int i;

    hr = IDirect3DDevice9_CreateVertexShader(device, mova_test, &mova_shader);
    ok(SUCCEEDED(hr), "CreateVertexShader failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetVertexShader(device, mova_shader);
    ok(SUCCEEDED(hr), "SetVertexShader failed (%08x)\n", hr);

    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements, &vertex_declaration);
    ok(SUCCEEDED(hr), "CreateVertexDeclaration failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetVertexDeclaration(device, vertex_declaration);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed (%08x)\n", hr);

    for (i = 0; i < (sizeof(test_data) / sizeof(test_data_t)); ++i)
    {
        DWORD color;

        hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 7, test_data[i].in, 1);
        ok(SUCCEEDED(hr), "SetVertexShaderConstantF failed (%08x)\n", hr);

        hr = IDirect3DDevice9_BeginScene(device);
        ok(SUCCEEDED(hr), "BeginScene failed (%08x)\n", hr);

        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quad[0], 3 * sizeof(float));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(SUCCEEDED(hr), "EndScene failed (%08x)\n", hr);

        hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        ok(SUCCEEDED(hr), "Present failed (%08x)\n", hr);

        color = getPixelColor(device, 320, 240);
        ok(color == test_data[i].out, "Expected color %08x, got %08x (for input %f)\n", test_data[i].out, color, test_data[i].in[0]);

        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0, 0.0f, 0);
        ok(SUCCEEDED(hr), "Clear failed (%08x)\n", hr);
    }

    hr = IDirect3DDevice9_SetVertexShader(device, NULL);
    ok(SUCCEEDED(hr), "SetVertexShader failed (%08x)\n", hr);

    IDirect3DVertexDeclaration9_Release(vertex_declaration);
    IDirect3DVertexShader9_Release(mova_shader);
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

static void fog_test(IDirect3DDevice9 *device)
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

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff00ff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear returned %s\n", DXGetErrorString9(hr));

    /* Setup initial states: No lighting, fog on, fog color */
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(hr == D3D_OK, "Turning off lighting returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGENABLE, TRUE);
    ok(hr == D3D_OK, "Turning on fog calculations returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGCOLOR, 0xFF00FF00 /* A nice green */);
    ok(hr == D3D_OK, "Turning on fog calculations returned %s\n", DXGetErrorString9(hr));

    /* First test: Both table fog and vertex fog off */
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGTABLEMODE, D3DFOG_NONE);
    ok(hr == D3D_OK, "Turning off table fog returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGVERTEXMODE, D3DFOG_NONE);
    ok(hr == D3D_OK, "Turning off table fog returned %s\n", DXGetErrorString9(hr));

    /* Start = 0, end = 1. Should be default, but set them */
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGSTART, *((DWORD *) &start));
    ok(hr == D3D_OK, "Setting fog start returned %s\n", DXGetErrorString9(hr));
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGEND, *((DWORD *) &end));
    ok(hr == D3D_OK, "Setting fog start returned %s\n", DXGetErrorString9(hr));

    if(IDirect3DDevice9_BeginScene(device) == D3D_OK)
    {
        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR);
        ok( hr == D3D_OK, "SetFVF returned %s\n", DXGetErrorString9(hr));
        /* Untransformed, vertex fog = NONE, table fog = NONE: Read the fog weighting from the specular color */
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                     2 /*PrimCount */, Indices, D3DFMT_INDEX16, unstransformed_1,
                                                     sizeof(unstransformed_1[0]));
        ok(hr == D3D_OK, "DrawIndexedPrimitiveUP returned %s\n", DXGetErrorString9(hr));

        /* That makes it use the Z value */
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
        ok(hr == D3D_OK, "Turning off table fog returned %s\n", DXGetErrorString9(hr));
        /* Untransformed, vertex fog != none (or table fog != none):
         * Use the Z value as input into the equation
         */
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                     2 /*PrimCount */, Indices, D3DFMT_INDEX16, unstransformed_2,
                                                     sizeof(unstransformed_1[0]));
        ok(hr == D3D_OK, "DrawIndexedPrimitiveUP returned %s\n", DXGetErrorString9(hr));

        /* transformed verts */
        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR);
        ok( hr == D3D_OK, "SetFVF returned %s\n", DXGetErrorString9(hr));
        /* Transformed, vertex fog != NONE, pixel fog == NONE: Use specular color alpha component */
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                     2 /*PrimCount */, Indices, D3DFMT_INDEX16, transformed_1,
                                                     sizeof(transformed_1[0]));
        ok(hr == D3D_OK, "DrawIndexedPrimitiveUP returned %s\n", DXGetErrorString9(hr));

        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGTABLEMODE, D3DFOG_LINEAR);
        ok( hr == D3D_OK, "Setting fog table mode to D3DFOG_LINEAR returned %s\n", DXGetErrorString9(hr));
        /* Transformed, table fog != none, vertex anything: Use Z value as input to the fog
         * equation
         */
        hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0 /* MinIndex */, 4 /* NumVerts */,
                                                     2 /*PrimCount */, Indices, D3DFMT_INDEX16, transformed_2,
                                                     sizeof(transformed_2[0]));

        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "EndScene returned %s\n", DXGetErrorString9(hr));
    }
    else
    {
        ok(FALSE, "BeginScene failed\n");
    }

    IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    color = getPixelColor(device, 160, 360);
    ok(color == 0x00FF0000, "Untransformed vertex with no table or vertex fog has color %08x\n", color);
    color = getPixelColor(device, 160, 120);
    ok(color == 0x0000FF00, "Untransformed vertex with linear vertex fog has color %08x\n", color);
    color = getPixelColor(device, 480, 120);
    ok(color == 0x00FFFF00, "Transformed vertex with linear vertex fog has color %08x\n", color);
    color = getPixelColor(device, 480, 360);
    ok(color == 0x0000FF00, "Transformed vertex with linear table fog has color %08x\n", color);

    /* Turn off the fog master switch to avoid confusing other tests */
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGENABLE, FALSE);
    ok(hr == D3D_OK, "Turning off fog calculations returned %s\n", DXGetErrorString9(hr));
}

/* This test verifies the behaviour of cube maps wrt. texture wrapping.
 * D3D cube map wrapping always behaves like GL_CLAMP_TO_EDGE,
 * regardless of the actual addressing mode set. */
static void test_cube_wrap(IDirect3DDevice9 *device)
{
    static const float quad[][6] = {
        {-1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f},
        {-1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 1.0f},
        { 1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f},
        { 1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 1.0f},
    };

    static const D3DVERTEXELEMENT9 decl_elements[] = {
        {0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        D3DDECL_END()
    };

    static const struct {
        D3DTEXTUREADDRESS mode;
        const char *name;
    } address_modes[] = {
        {D3DTADDRESS_WRAP, "D3DTADDRESS_WRAP"},
        {D3DTADDRESS_MIRROR, "D3DTADDRESS_MIRROR"},
        {D3DTADDRESS_CLAMP, "D3DTADDRESS_CLAMP"},
        {D3DTADDRESS_BORDER, "D3DTADDRESS_BORDER"},
        {D3DTADDRESS_MIRRORONCE, "D3DTADDRESS_MIRRORONCE"},
    };

    IDirect3DVertexDeclaration9 *vertex_declaration = NULL;
    IDirect3DCubeTexture9 *texture = NULL;
    IDirect3DSurface9 *surface = NULL;
    D3DLOCKED_RECT locked_rect;
    HRESULT hr;
    INT x, y, face;

    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements, &vertex_declaration);
    ok(SUCCEEDED(hr), "CreateVertexDeclaration failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetVertexDeclaration(device, vertex_declaration);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed (0x%08x)\n", hr);

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 128, 128,
            D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &surface, NULL);
    ok(SUCCEEDED(hr), "CreateOffscreenPlainSurface failed (0x%08x)\n", hr);

    hr = IDirect3DSurface9_LockRect(surface, &locked_rect, NULL, D3DLOCK_DISCARD);
    ok(SUCCEEDED(hr), "LockRect failed (0x%08x)\n", hr);

    for (y = 0; y < 128; ++y)
    {
        DWORD *ptr = (DWORD *)(((BYTE *)locked_rect.pBits) + (y * locked_rect.Pitch));
        for (x = 0; x < 64; ++x)
        {
            *ptr++ = 0xffff0000;
        }
        for (x = 64; x < 128; ++x)
        {
            *ptr++ = 0xff0000ff;
        }
    }

    hr = IDirect3DSurface9_UnlockRect(surface);
    ok(SUCCEEDED(hr), "UnlockRect failed (0x%08x)\n", hr);

    hr = IDirect3DDevice9_CreateCubeTexture(device, 128, 1, 0, D3DFMT_A8R8G8B8,
            D3DPOOL_DEFAULT, &texture, NULL);
    ok(SUCCEEDED(hr), "CreateCubeTexture failed (0x%08x)\n", hr);

    /* Create cube faces */
    for (face = 0; face < 6; ++face)
    {
        IDirect3DSurface9 *face_surface = NULL;

        hr= IDirect3DCubeTexture9_GetCubeMapSurface(texture, face, 0, &face_surface);
        ok(SUCCEEDED(hr), "GetCubeMapSurface failed (0x%08x)\n", hr);

        hr = IDirect3DDevice9_UpdateSurface(device, surface, NULL, face_surface, NULL);
        ok(SUCCEEDED(hr), "UpdateSurface failed (0x%08x)\n", hr);

        IDirect3DSurface9_Release(face_surface);
    }

    hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *)texture);
    ok(SUCCEEDED(hr), "SetTexture failed (0x%08x)\n", hr);

    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    ok(SUCCEEDED(hr), "SetSamplerState D3DSAMP_MINFILTER failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    ok(SUCCEEDED(hr), "SetSamplerState D3DSAMP_MAGFILTER failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_BORDERCOLOR, 0xff00ff00);
    ok(SUCCEEDED(hr), "SetSamplerState D3DSAMP_BORDERCOLOR failed (0x%08x)\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %s\n", DXGetErrorString9(hr));

    for (x = 0; x < (sizeof(address_modes) / sizeof(*address_modes)); ++x)
    {
        DWORD color;

        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSU, address_modes[x].mode);
        ok(SUCCEEDED(hr), "SetSamplerState D3DSAMP_ADDRESSU (%s) failed (0x%08x)\n", address_modes[x].name, hr);
        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_ADDRESSV, address_modes[x].mode);
        ok(SUCCEEDED(hr), "SetSamplerState D3DSAMP_ADDRESSV (%s) failed (0x%08x)\n", address_modes[x].name, hr);

        hr = IDirect3DDevice9_BeginScene(device);
        ok(SUCCEEDED(hr), "BeginScene failed (0x%08x)\n", hr);

        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quad[0], sizeof(quad[0]));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (0x%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(SUCCEEDED(hr), "EndScene failed (0x%08x)\n", hr);

        hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

        /* Due to the nature of this test, we sample essentially at the edge
         * between two faces. Because of this it's undefined from which face
         * the driver will sample. Furtunately that's not important for this
         * test, since all we care about is that it doesn't sample from the
         * other side of the surface or from the border. */
        color = getPixelColor(device, 320, 240);
        ok(color == 0x00ff0000 || color == 0x000000ff,
                "Got color 0x%08x for addressing mode %s, expected 0x00ff0000 or 0x000000ff.\n",
                color, address_modes[x].name);

        hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0, 0.0f, 0);
        ok(SUCCEEDED(hr), "Clear failed (0x%08x)\n", hr);
    }

    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(SUCCEEDED(hr), "SetTexture failed (0x%08x)\n", hr);

    IDirect3DVertexDeclaration9_Release(vertex_declaration);
    IDirect3DCubeTexture9_Release(texture);
    IDirect3DSurface9_Release(surface);
}

/* This test tests fog in combination with shaders.
 * What's tested: linear fog (vertex and table) with pixel shader
 *                linear table fog with non foggy vertex shader
 *                vertex fog with foggy vertex shader
 * What's not tested: non linear fog with shader
 *                    table fog with foggy vertex shader
 */
static void fog_with_shader_test(IDirect3DDevice9 *device)
{
    HRESULT hr;
    DWORD color;
    /* NOTE: changing these values will not effect the tests with foggy vertex shader, as the values are hardcoded in the shader*/
    union {float f; DWORD i;} start={.f=0.9}, end={.f=0.1};
    unsigned int i, j;

    /* basic vertex shader without fog computation ("non foggy") */
    static const DWORD vertex_shader_code1[] = {
        0xfffe0101,                                                             /* vs_1_1                       */
        0x0000001f, 0x80000000, 0x900f0000,                                     /* dcl_position v0              */
        0x0000001f, 0x8000000a, 0x900f0001,                                     /* dcl_color0 v1                */
        0x00000001, 0xc00f0000, 0x90e40000,                                     /* mov oPos, v0                 */
        0x00000001, 0xd00f0000, 0x90e40001,                                     /* mov oD0, v1                  */
        0x0000ffff
    };
    /* basic vertex shader with reversed fog computation ("foggy") */
    static const DWORD vertex_shader_code2[] = {
        0xfffe0101,                                                             /* vs_1_1                        */
        0x0000001f, 0x80000000, 0x900f0000,                                     /* dcl_position v0               */
        0x0000001f, 0x8000000a, 0x900f0001,                                     /* dcl_color0 v1                 */
        0x00000051, 0xa00f0000, 0xbfa00000, 0x00000000, 0xbf666666, 0x00000000, /* def c0, -1.25, 0.0, -0.9, 0.0 */
        0x00000001, 0xc00f0000, 0x90e40000,                                     /* mov oPos, v0                  */
        0x00000001, 0xd00f0000, 0x90e40001,                                     /* mov oD0, v1                   */
        0x00000002, 0x800f0000, 0x90aa0000, 0xa0aa0000,                         /* add r0, v0.z, c0.z            */
        0x00000005, 0xc00f0001, 0x80000000, 0xa0000000,                         /* mul oFog, r0.x, c0.x          */
        0x0000ffff
    };
    /* basic pixel shader */
    static const DWORD pixel_shader_code[] = {
        0xffff0101,                                                             /* ps_1_1     */
        0x00000001, 0x800f0000, 0x90e40000,                                     /* mov r0, vo */
        0x0000ffff
    };

    static struct vertex quad[] = {
        {-1.0f, -1.0f,  0.0f,          0xFFFF0000  },
        {-1.0f,  1.0f,  0.0f,          0xFFFF0000  },
        { 1.0f, -1.0f,  0.0f,          0xFFFF0000  },
        { 1.0f,  1.0f,  0.0f,          0xFFFF0000  },
    };

    static const D3DVERTEXELEMENT9 decl_elements[] = {
        {0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT,    D3DDECLUSAGE_COLOR, 0},
        D3DDECL_END()
    };

    IDirect3DVertexDeclaration9 *vertex_declaration = NULL;
    IDirect3DVertexShader9      *vertex_shader[3]   = {NULL, NULL, NULL};
    IDirect3DPixelShader9       *pixel_shader[2]    = {NULL, NULL};

    /* This reference data was collected on a nVidia GeForce 7600GS driver version 84.19 DirectX version 9.0c on Windows XP */
    static const struct test_data_t {
        int vshader;
        int pshader;
        D3DFOGMODE vfog;
        D3DFOGMODE tfog;
        unsigned int color[11];
    } test_data[] = {
        /* only pixel shader: */
        {0, 1, 0, 3,
        {0x0000ff00, 0x0000ff00, 0x0020df00, 0x0040bf00, 0x005fa000, 0x007f8000,
         0x009f6000, 0x00bf4000, 0x00df2000, 0x00ff0000, 0x00ff0000}},
        {0, 1, 1, 3,
        {0x0000ff00, 0x0000ff00, 0x0020df00, 0x0040bf00, 0x005fa000, 0x007f8000,
         0x009f6000, 0x00bf4000, 0x00df2000, 0x00ff0000, 0x00ff0000}},
        {0, 1, 2, 3,
        {0x0000ff00, 0x0000ff00, 0x0020df00, 0x0040bf00, 0x005fa000, 0x007f8000,
         0x009f6000, 0x00bf4000, 0x00df2000, 0x00ff0000, 0x00ff0000}},
        {0, 1, 3, 0,
        {0x0000ff00, 0x0000ff00, 0x0020df00, 0x0040bf00, 0x005fa000, 0x007f8000,
         0x009f6000, 0x00bf4000, 0x00df2000, 0x00ff0000, 0x00ff0000}},
        {0, 1, 3, 3,
        {0x0000ff00, 0x0000ff00, 0x0020df00, 0x0040bf00, 0x005fa000, 0x007f8000,
         0x009f6000, 0x00bf4000, 0x00df2000, 0x00ff0000, 0x00ff0000}},

        /* vertex shader */
        {1, 0, 0, 0,
        {0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00,
         0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00}},
        {1, 0, 0, 3,
        {0x0000ff00, 0x0000ff00, 0x0020df00, 0x0040bf00, 0x005fa000, 0x007f8000,
         0x009f6000, 0x00bf4000, 0x00df2000, 0x00ff0000, 0x00ff0000}},
        {1, 0, 1, 3,
        {0x0000ff00, 0x0000ff00, 0x0020df00, 0x0040bf00, 0x005fa000, 0x007f8000,
         0x009f6000, 0x00bf4000, 0x00df2000, 0x00ff0000, 0x00ff0000}},
        {1, 0, 2, 3,
        {0x0000ff00, 0x0000ff00, 0x0020df00, 0x0040bf00, 0x005fa000, 0x007f8000,
         0x009f6000, 0x00bf4000, 0x00df2000, 0x00ff0000, 0x00ff0000}},
        {1, 0, 3, 3,
        {0x0000ff00, 0x0000ff00, 0x0020df00, 0x0040bf00, 0x005fa000, 0x007f8000,
         0x009f6000, 0x00bf4000, 0x00df2000, 0x00ff0000, 0x00ff0000}},

        /* vertex shader and pixel shader */
        {1, 1, 0, 3,
        {0x0000ff00, 0x0000ff00, 0x0020df00, 0x0040bf00, 0x005fa000, 0x007f8000,
         0x009f6000, 0x00bf4000, 0x00df2000, 0x00ff0000, 0x00ff0000}},
        {1, 1, 1, 3,
        {0x0000ff00, 0x0000ff00, 0x0020df00, 0x0040bf00, 0x005fa000, 0x007f8000,
         0x009f6000, 0x00bf4000, 0x00df2000, 0x00ff0000, 0x00ff0000}},
        {1, 1, 2, 3,
        {0x0000ff00, 0x0000ff00, 0x0020df00, 0x0040bf00, 0x005fa000, 0x007f8000,
         0x009f6000, 0x00bf4000, 0x00df2000, 0x00ff0000, 0x00ff0000}},
        {1, 1, 3, 3,
        {0x0000ff00, 0x0000ff00, 0x0020df00, 0x0040bf00, 0x005fa000, 0x007f8000,
         0x009f6000, 0x00bf4000, 0x00df2000, 0x00ff0000, 0x00ff0000}},

        /* foggy vertex shader */
        {2, 0, 0, 0,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {2, 0, 1, 0,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {2, 0, 2, 0,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {2, 0, 3, 0,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},

        /* foggy vertex shader and pixel shader */
        {2, 1, 0, 0,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {2, 1, 1, 0,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {2, 1, 2, 0,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},
        {2, 1, 3, 0,
        {0x00ff0000, 0x00fe0100, 0x00de2100, 0x00bf4000, 0x009f6000, 0x007f8000,
         0x005fa000, 0x003fc000, 0x001fe000, 0x0000ff00, 0x0000ff00}},

    };

    hr = IDirect3DDevice9_CreateVertexShader(device, vertex_shader_code1, &vertex_shader[1]);
    ok(SUCCEEDED(hr), "CreateVertexShader failed (%08x)\n", hr);
    hr = IDirect3DDevice9_CreateVertexShader(device, vertex_shader_code2, &vertex_shader[2]);
    ok(SUCCEEDED(hr), "CreateVertexShader failed (%08x)\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, pixel_shader_code, &pixel_shader[1]);
    ok(SUCCEEDED(hr), "CreatePixelShader failed (%08x)\n", hr);
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements, &vertex_declaration);
    ok(SUCCEEDED(hr), "CreateVertexDeclaration failed (%08x)\n", hr);

    /* Setup initial states: No lighting, fog on, fog color */
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(hr == D3D_OK, "Turning off lighting failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGENABLE, TRUE);
    ok(hr == D3D_OK, "Turning on fog calculations failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGCOLOR, 0xFF00FF00 /* A nice green */);
    ok(hr == D3D_OK, "Setting fog color failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetVertexDeclaration(device, vertex_declaration);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed (%08x)\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGTABLEMODE, D3DFOG_NONE);
    ok(hr == D3D_OK, "Turning off table fog failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGVERTEXMODE, D3DFOG_NONE);
    ok(hr == D3D_OK, "Turning off vertex fog failed (%08x)\n", hr);

    /* Use fogtart = 0.1 and end = 0.9 to test behavior outside the fog transition phase, too*/
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGSTART, start.i);
    ok(hr == D3D_OK, "Setting fog start failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGEND, end.i);
    ok(hr == D3D_OK, "Setting fog end failed (%08x)\n", hr);

    for (i = 0; i < 22; i++)
    {
        hr = IDirect3DDevice9_SetVertexShader(device, vertex_shader[test_data[i].vshader]);
        ok(SUCCEEDED(hr), "SetVertexShader failed (%08x)\n", hr);
        hr = IDirect3DDevice9_SetPixelShader(device, pixel_shader[test_data[i].pshader]);
        ok(SUCCEEDED(hr), "SetPixelShader failed (%08x)\n", hr);
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGVERTEXMODE, test_data[i].vfog);
        ok( hr == D3D_OK, "Setting fog vertex mode to D3DFOG_LINEAR failed (%08x)\n", hr);
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGTABLEMODE, test_data[i].tfog);
        ok( hr == D3D_OK, "Setting fog table mode to D3DFOG_LINEAR failed (%08x)\n", hr);

        for(j=0; j < 11; j++)
        {
            /* Don't use the whole zrange to prevent rounding errors */
            quad[0].z = 0.001f + (float)j / 10.02f;
            quad[1].z = 0.001f + (float)j / 10.02f;
            quad[2].z = 0.001f + (float)j / 10.02f;
            quad[3].z = 0.001f + (float)j / 10.02f;

            hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff00ff, 0.0, 0);
            ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed (%08x)\n", hr);

            hr = IDirect3DDevice9_BeginScene(device);
            ok( hr == D3D_OK, "BeginScene returned failed (%08x)\n", hr);

            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quad[0], sizeof(quad[0]));
            ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (%08x)\n", hr);

            hr = IDirect3DDevice9_EndScene(device);
            ok(hr == D3D_OK, "EndScene failed (%08x)\n", hr);

            IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);

            /* As the red and green component are the result of blending use 5% tolerance on the expected value */
            color = getPixelColor(device, 128, 240);
            ok((unsigned char)(color) == ((unsigned char)test_data[i].color[j])
                    && abs( ((unsigned char)(color>>8)) - (unsigned char)(test_data[i].color[j]>>8) ) < 13
                    && abs( ((unsigned char)(color>>16)) - (unsigned char)(test_data[i].color[j]>>16) ) < 13,
                    "fog ps%i vs%i fvm%i ftm%i %d: got color %08x, expected %08x +-5%%\n", test_data[i].vshader, test_data[i].pshader, test_data[i].vfog, test_data[i].tfog, j, color, test_data[i].color[j]);
        }
    }

    /* reset states */
    hr = IDirect3DDevice9_SetVertexShader(device, NULL);
    ok(SUCCEEDED(hr), "SetVertexShader failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetPixelShader(device, NULL);
    ok(SUCCEEDED(hr), "SetPixelShader failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetVertexDeclaration(device, NULL);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed (%08x)\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_FOGENABLE, FALSE);
    ok(hr == D3D_OK, "Turning off fog calculations failed (%08x)\n", hr);

    IDirect3DVertexShader9_Release(vertex_shader[1]);
    IDirect3DVertexShader9_Release(vertex_shader[2]);
    IDirect3DPixelShader9_Release(pixel_shader[1]);
    IDirect3DVertexDeclaration9_Release(vertex_declaration);
}

/* test the behavior of the texbem instruction
 * with normal 2D and projective 2D textures
 */
static void texbem_test(IDirect3DDevice9 *device)
{
    HRESULT hr;
    DWORD color;
    unsigned int i, x, y;

    static const DWORD pixel_shader_code[] = {
        0xffff0101,                         /* ps_1_1*/
        0x00000042, 0xb00f0000,             /* tex t0*/
        0x00000043, 0xb00f0001, 0xb0e40000, /* texbem t1, t0*/
        0x00000001, 0x800f0000, 0xb0e40001, /* mov r0, t1*/
        0x0000ffff
    };

    static const float quad[][7] = {
        {-128.0f/640.0f, -128.0f/480.0f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f},
        {-128.0f/640.0f,  128.0f/480.0f, 0.1f, 0.0f, 1.0f, 0.0f, 1.0f},
        { 128.0f/640.0f, -128.0f/480.0f, 0.1f, 1.0f, 0.0f, 1.0f, 0.0f},
        { 128.0f/640.0f,  128.0f/480.0f, 0.1f, 1.0f, 1.0f, 1.0f, 1.0f},
    };
    static const float quad_proj[][9] = {
        {-128.0f/640.0f, -128.0f/480.0f, 0.1f, 0.0f, 0.0f,   0.0f,   0.0f, 0.0f, 128.0f},
        {-128.0f/640.0f,  128.0f/480.0f, 0.1f, 0.0f, 1.0f,   0.0f, 128.0f, 0.0f, 128.0f},
        { 128.0f/640.0f, -128.0f/480.0f, 0.1f, 1.0f, 0.0f, 128.0f,   0.0f, 0.0f, 128.0f},
        { 128.0f/640.0f,  128.0f/480.0f, 0.1f, 1.0f, 1.0f, 128.0f, 128.0f, 0.0f, 128.0f},
    };

    static const D3DVERTEXELEMENT9 decl_elements[][4] = { {
        {0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        {0, 20, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},
        D3DDECL_END()
    },{
        {0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
        {0, 20, D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},
        D3DDECL_END()
    } };

    /* use assymetric matrix to test loading */
    float bumpenvmat[4] = {0.0,0.5,-0.5,0.0};

    IDirect3DVertexDeclaration9 *vertex_declaration = NULL;
    IDirect3DPixelShader9       *pixel_shader       = NULL;
    IDirect3DTexture9           *texture[2]         = {NULL, NULL};
    D3DLOCKED_RECT locked_rect;

    /* Generate the textures */
    for(i=0; i<2; i++)
    {
        hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 1, 0, i?D3DFMT_A8R8G8B8:D3DFMT_V8U8,
                D3DPOOL_MANAGED, &texture[i], NULL);
        ok(SUCCEEDED(hr), "CreateTexture failed (0x%08x)\n", hr);

        hr = IDirect3DTexture9_LockRect(texture[i], 0, &locked_rect, NULL, D3DLOCK_DISCARD);
        ok(SUCCEEDED(hr), "LockRect failed (0x%08x)\n", hr);
        for (y = 0; y < 128; ++y)
        {
            if(i)
            { /* Set up black texture with 2x2 texel white spot in the middle */
                DWORD *ptr = (DWORD *)(((BYTE *)locked_rect.pBits) + (y * locked_rect.Pitch));
                for (x = 0; x < 128; ++x)
                {
                    if(y>62 && y<66 && x>62 && x<66)
                        *ptr++ = 0xffffffff;
                    else
                        *ptr++ = 0xff000000;
                }
            }
            else
            { /* Set up a displacement map which points away from the center parallel to the closest axis.
              * (if multiplied with bumpenvmat)
              */
                WORD *ptr = (WORD *)(((BYTE *)locked_rect.pBits) + (y * locked_rect.Pitch));
                for (x = 0; x < 128; ++x)
                {
                    if(abs(x-64)>abs(y-64))
                    {
                        if(x < 64)
                            *ptr++ = 0xc000;
                        else
                            *ptr++ = 0x4000;
                    }
                    else
                    {
                        if(y < 64)
                            *ptr++ = 0x0040;
                        else
                            *ptr++ = 0x00c0;
                    }
                }
            }
        }
        hr = IDirect3DTexture9_UnlockRect(texture[i], 0);
        ok(SUCCEEDED(hr), "UnlockRect failed (0x%08x)\n", hr);

        hr = IDirect3DDevice9_SetTexture(device, i, (IDirect3DBaseTexture9 *)texture[i]);
        ok(SUCCEEDED(hr), "SetTexture failed (0x%08x)\n", hr);

        /* Disable texture filtering */
        hr = IDirect3DDevice9_SetSamplerState(device, i, D3DSAMP_MINFILTER, D3DTEXF_POINT);
        ok(SUCCEEDED(hr), "SetSamplerState D3DSAMP_MINFILTER failed (0x%08x)\n", hr);
        hr = IDirect3DDevice9_SetSamplerState(device, i, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
        ok(SUCCEEDED(hr), "SetSamplerState D3DSAMP_MAGFILTER failed (0x%08x)\n", hr);

        hr = IDirect3DDevice9_SetSamplerState(device, i, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
        ok(SUCCEEDED(hr), "SetSamplerState D3DSAMP_ADDRESSU failed (0x%08x)\n", hr);
        hr = IDirect3DDevice9_SetSamplerState(device, i, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
        ok(SUCCEEDED(hr), "SetSamplerState D3DSAMP_ADDRESSV failed (0x%08x)\n", hr);
    }

    IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_BUMPENVMAT00, *(LPDWORD)&bumpenvmat[0]);
    IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_BUMPENVMAT01, *(LPDWORD)&bumpenvmat[1]);
    IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_BUMPENVMAT10, *(LPDWORD)&bumpenvmat[2]);
    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_BUMPENVMAT11, *(LPDWORD)&bumpenvmat[3]);
    ok(SUCCEEDED(hr), "SetTextureStageState failed (%08x)\n", hr);

    hr = IDirect3DDevice9_SetVertexShader(device, NULL);
    ok(SUCCEEDED(hr), "SetVertexShader failed (%08x)\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff00ff, 0.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed (%08x)\n", hr);

    for(i=0; i<2; i++)
    {
        if(i)
        {
            hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT4|D3DTTFF_PROJECTED);
            ok(SUCCEEDED(hr), "SetTextureStageState D3DTSS_TEXTURETRANSFORMFLAGS failed (0x%08x)\n", hr);
        }

        hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements[i], &vertex_declaration);
        ok(SUCCEEDED(hr), "CreateVertexDeclaration failed (0x%08x)\n", hr);
        hr = IDirect3DDevice9_SetVertexDeclaration(device, vertex_declaration);
        ok(SUCCEEDED(hr), "SetVertexDeclaration failed (0x%08x)\n", hr);

        hr = IDirect3DDevice9_CreatePixelShader(device, pixel_shader_code, &pixel_shader);
        ok(SUCCEEDED(hr), "CreatePixelShader failed (%08x)\n", hr);
        hr = IDirect3DDevice9_SetPixelShader(device, pixel_shader);
        ok(SUCCEEDED(hr), "SetPixelShader failed (%08x)\n", hr);

        hr = IDirect3DDevice9_BeginScene(device);
        ok(SUCCEEDED(hr), "BeginScene failed (0x%08x)\n", hr);

        if(!i)
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quad[0], sizeof(quad[0]));
        else
            hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, &quad_proj[0], sizeof(quad_proj[0]));
        ok(SUCCEEDED(hr), "DrawPrimitiveUP failed (0x%08x)\n", hr);

        hr = IDirect3DDevice9_EndScene(device);
        ok(SUCCEEDED(hr), "EndScene failed (0x%08x)\n", hr);

        hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
        ok(SUCCEEDED(hr), "Present failed (0x%08x)\n", hr);

        color = getPixelColor(device, 320-32, 240);
        ok(color == 0x00ffffff, "texbem failed: Got color 0x%08x, expected 0x00ffffff.\n", color);
        color = getPixelColor(device, 320+32, 240);
        ok(color == 0x00ffffff, "texbem failed: Got color 0x%08x, expected 0x00ffffff.\n", color);
        color = getPixelColor(device, 320, 240-32);
        ok(color == 0x00ffffff, "texbem failed: Got color 0x%08x, expected 0x00ffffff.\n", color);
        color = getPixelColor(device, 320, 240+32);
        ok(color == 0x00ffffff, "texbem failed: Got color 0x%08x, expected 0x00ffffff.\n", color);

        hr = IDirect3DDevice9_SetPixelShader(device, NULL);
        ok(SUCCEEDED(hr), "SetPixelShader failed (%08x)\n", hr);
        IDirect3DPixelShader9_Release(pixel_shader);

        hr = IDirect3DDevice9_SetVertexDeclaration(device, NULL);
        ok(SUCCEEDED(hr), "SetVertexDeclaration failed (%08x)\n", hr);
        IDirect3DVertexDeclaration9_Release(vertex_declaration);
    }

    /* clean up */
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0, 0.0f, 0);
    ok(SUCCEEDED(hr), "Clear failed (0x%08x)\n", hr);

    hr = IDirect3DDevice9_SetTextureStageState(device, 1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
    ok(SUCCEEDED(hr), "SetTextureStageState D3DTSS_TEXTURETRANSFORMFLAGS failed (0x%08x)\n", hr);

    for(i=0; i<2; i++)
    {
        hr = IDirect3DDevice9_SetTexture(device, i, NULL);
        ok(SUCCEEDED(hr), "SetTexture failed (0x%08x)\n", hr);
        IDirect3DCubeTexture9_Release(texture[i]);
    }
}

START_TEST(visual)
{
    IDirect3DDevice9 *device_ptr;
    D3DCAPS9 caps;
    HRESULT hr;
    DWORD color;

    d3d9_handle = LoadLibraryA("d3d9.dll");
    if (!d3d9_handle)
    {
        skip("Could not load d3d9.dll\n");
        return;
    }

    device_ptr = init_d3d9();
    if (!device_ptr) return;

    IDirect3DDevice9_GetDeviceCaps(device_ptr, &caps);

    /* Check for the reliability of the returned data */
    hr = IDirect3DDevice9_Clear(device_ptr, 0, NULL, D3DCLEAR_TARGET, 0xffff0000, 0.0, 0);
    if(FAILED(hr))
    {
        trace("Clear failed, can't assure correctness of the test results, skipping\n");
        goto cleanup;
    }
    IDirect3DDevice9_Present(device_ptr, NULL, NULL, NULL, NULL);

    color = getPixelColor(device_ptr, 1, 1);
    if(color !=0x00ff0000)
    {
        trace("Sanity check returned an incorrect color(%08x), can't assure the correctness of the tests, skipping\n", color);
        goto cleanup;
    }

    hr = IDirect3DDevice9_Clear(device_ptr, 0, NULL, D3DCLEAR_TARGET, 0xff00ddee, 0.0, 0);
    if(FAILED(hr))
    {
        trace("Clear failed, can't assure correctness of the test results, skipping\n");
        goto cleanup;
    }
    IDirect3DDevice9_Present(device_ptr, NULL, NULL, NULL, NULL);

    color = getPixelColor(device_ptr, 639, 479);
    if(color != 0x0000ddee)
    {
        trace("Sanity check returned an incorrect color(%08x), can't assure the correctness of the tests, skipping\n", color);
        goto cleanup;
    }

    /* Now execute the real tests */
    lighting_test(device_ptr);
    clear_test(device_ptr);
    fog_test(device_ptr);
    test_cube_wrap(device_ptr);

    if (caps.VertexShaderVersion >= D3DVS_VERSION(2, 0))
    {
        test_mova(device_ptr);
    }
    else skip("No vs_2_0 support\n");

    if (caps.VertexShaderVersion >= D3DVS_VERSION(1, 1) && caps.PixelShaderVersion >= D3DVS_VERSION(1, 1))
    {
        fog_with_shader_test(device_ptr);
    }
    else skip("No vs_1_1 and ps_1_1 support\n");

    if (caps.PixelShaderVersion >= D3DVS_VERSION(1, 1))
    {
        texbem_test(device_ptr);
    }
    else skip("No ps_1_1 support\n");

cleanup:
    if(device_ptr) IDirect3DDevice9_Release(device_ptr);
}
