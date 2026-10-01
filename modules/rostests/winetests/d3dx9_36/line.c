/*
 * Copyright 2010 Christian Costa
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
#include "d3dx9.h"

#define admitted_error 0.0001f

#define relative_error(exp, out) ((exp == 0.0f) ? fabs(exp - out) : (fabs(1.0f - out/ exp) ))

static inline BOOL compare_matrix(const D3DXMATRIX *m1, const D3DXMATRIX *m2)
{
    int i, j;

    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            if (relative_error(m1->m[i][j], m2->m[i][j]) > admitted_error)
                return FALSE;
        }
    }

    return TRUE;
}

#define expect_mat(expectedmat, gotmat) \
do { \
    const D3DXMATRIX *__m1 = (expectedmat); \
    const D3DXMATRIX *__m2 = (gotmat); \
    ok(compare_matrix(__m1, __m2), "Expected matrix=\n(%f,%f,%f,%f\n %f,%f,%f,%f\n %f,%f,%f,%f\n %f,%f,%f,%f\n)\n\n" \
            "Got matrix=\n(%f,%f,%f,%f\n %f,%f,%f,%f\n %f,%f,%f,%f\n %f,%f,%f,%f)\n", \
            __m1->m[0][0], __m1->m[0][1], __m1->m[0][2], __m1->m[0][3], \
            __m1->m[1][0], __m1->m[1][1], __m1->m[1][2], __m1->m[1][3], \
            __m1->m[2][0], __m1->m[2][1], __m1->m[2][2], __m1->m[2][3], \
            __m1->m[3][0], __m1->m[3][1], __m1->m[3][2], __m1->m[3][3], \
            __m2->m[0][0], __m2->m[0][1], __m2->m[0][2], __m2->m[0][3], \
            __m2->m[1][0], __m2->m[1][1], __m2->m[1][2], __m2->m[1][3], \
            __m2->m[2][0], __m2->m[2][1], __m2->m[2][2], __m2->m[2][3], \
            __m2->m[3][0], __m2->m[3][1], __m2->m[3][2], __m2->m[3][3]); \
} while(0)

static void test_create_line(IDirect3DDevice9* device)
{
    HRESULT hr;
    ID3DXLine *line = NULL;
    struct IDirect3DDevice9 *return_device;
    D3DXMATRIX world, identity, result;
    FLOAT r11, r12, r13, r14;
    ULONG ref;

    /* Arbitrary values for matrix tests. */
    r11 = 0.1421; r12 = 0.2114; r13 = 0.8027; r14 = 0.4587;

    hr = D3DXCreateLine(NULL, &line);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3D_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateLine(device, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3D_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = D3DXCreateLine(device, &line);
    ok(hr == D3D_OK, "Got result %lx, expected 0 (D3D_OK)\n", hr);

    if (FAILED(hr))
    {
        return;
    }

    hr = ID3DXLine_GetDevice(line, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got result %lx, expected %lx (D3D_INVALIDCALL)\n", hr, D3DERR_INVALIDCALL);

    hr = ID3DXLine_GetDevice(line, &return_device);
    ok(hr == D3D_OK, "Got result %lx, expected %lx (D3D_OK)\n", hr, D3D_OK);
    ok(return_device == device, "Expected line device %p, got %p\n", device, return_device);

    D3DXMatrixIdentity(&world);
    D3DXMatrixIdentity(&identity);
    world._11 = r11; world._12 = r12; world._13 = r13; world._14 = r14;

    hr = IDirect3DDevice9_SetTransform(device, D3DTS_WORLD, &world);
    ok(hr == D3D_OK, "Got result %lx, expected %lx (D3D_OK)\n", hr, D3D_OK);

    hr = ID3DXLine_Begin(line);
    ok(hr == D3D_OK, "Got result %lx, expected %lx (D3D_OK)\n", hr, D3D_OK);

    hr = IDirect3DDevice9_GetTransform(device, D3DTS_WORLD, &result);
    ok(hr == D3D_OK, "Got result %lx, expected %lx (D3D_OK)\n", hr, D3D_OK);
    expect_mat(&identity, &result);

    hr = ID3DXLine_End(line);
    ok(hr == D3D_OK, "Got result %lx, expected %lx (D3D_OK)\n", hr, D3D_OK);

    hr = IDirect3DDevice9_GetTransform(device, D3DTS_WORLD, &result);
    ok(hr == D3D_OK, "Got result %lx, expected %lx (D3D_OK)\n", hr, D3D_OK);
    expect_mat(&world, &result);

    IDirect3DDevice9_Release(return_device);

    ref = ID3DXLine_Release(line);
    ok(ref == 0, "Got %lx references to line %p, expected 0\n", ref, line);
}

static void test_line_width(IDirect3DDevice9* device)
{
    ID3DXLine *line = NULL;
    ULONG refcount;
    float width;
    HRESULT hr;

    hr = D3DXCreateLine(device, &line);
    ok(hr == D3D_OK, "Failed to create a line, hr %#lx.\n", hr);

    width = ID3DXLine_GetWidth(line);
    ok(width == 1.0f, "Unexpected line width %.8e.\n", width);

    hr = ID3DXLine_SetWidth(line, 0.0f);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);
    width = ID3DXLine_GetWidth(line);
    ok(width == 1.0f, "Unexpected line width %.8e.\n", width);

    hr = ID3DXLine_SetWidth(line, -1.0f);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);
    width = ID3DXLine_GetWidth(line);
    ok(width == 1.0f, "Unexpected line width %.8e.\n", width);

    hr = ID3DXLine_SetWidth(line, 10.0f);
    ok(hr == D3D_OK, "Unexpected hr %#lx.\n", hr);
    width = ID3DXLine_GetWidth(line);
    ok(width == 10.0f, "Unexpected line width %.8e.\n", width);

    refcount = ID3DXLine_Release(line);
    ok(!refcount, "Got %lu references to line.\n", refcount);
}

START_TEST(line)
{
    HWND wnd;
    IDirect3D9* d3d;
    IDirect3DDevice9* device;
    D3DPRESENT_PARAMETERS d3dpp;
    HRESULT hr;

    if (!(wnd = CreateWindowA("static", "d3dx9_test", WS_OVERLAPPEDWINDOW, 0, 0,
            640, 480, NULL, NULL, NULL, NULL)))
    {
        skip("Couldn't create application window\n");
        return;
    }
    if (!(d3d = Direct3DCreate9(D3D_SDK_VERSION)))
    {
        skip("Couldn't create IDirect3D9 object\n");
        DestroyWindow(wnd);
        return;
    }

    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &device);
    if (FAILED(hr)) {
        skip("Failed to create IDirect3DDevice9 object %#lx\n", hr);
        IDirect3D9_Release(d3d);
        DestroyWindow(wnd);
        return;
    }

    test_create_line(device);
    test_line_width(device);

    IDirect3DDevice9_Release(device);
    IDirect3D9_Release(d3d);
    DestroyWindow(wnd);
}
