/*
 * Copyright 2005 Antoine Chavasse (a.chavasse@gmail.com)
 * Copyright 2008, 2011, 2012-2014 Stefan Dösinger for CodeWeavers
 * Copyright 2011-2014 Henri Verbeet for CodeWeavers
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

#ifdef __REACTOS__
#define _USE_MATH_DEFINES 1
#endif

#include "wine/test.h"
#include <limits.h>
#include <math.h>
#include "d3d.h"

HRESULT WINAPI GetSurfaceFromDC(HDC dc, struct IDirectDrawSurface **surface, HDC *device_dc);

static BOOL is_ddraw64 = sizeof(DWORD) != sizeof(DWORD *);
static DEVMODEW registry_mode;

static HRESULT (WINAPI *pDwmIsCompositionEnabled)(BOOL *);

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#endif

struct vec2
{
    float x, y;
};

struct vec3
{
    float x, y, z;
};

struct vec4
{
    float x, y, z, w;
};

struct create_window_thread_param
{
    HWND window;
    HANDLE window_created;
    HANDLE destroy_window;
    HANDLE thread;
};

static BOOL compare_float(float f, float g, unsigned int ulps)
{
    int x = *(int *)&f;
    int y = *(int *)&g;

    if (x < 0)
        x = INT_MIN - x;
    if (y < 0)
        y = INT_MIN - y;

    if (abs(x - y) > ulps)
        return FALSE;

    return TRUE;
}

static BOOL compare_vec4(const struct vec4 *vec, float x, float y, float z, float w, unsigned int ulps)
{
    return compare_float(vec->x, x, ulps)
            && compare_float(vec->y, y, ulps)
            && compare_float(vec->z, z, ulps)
            && compare_float(vec->w, w, ulps);
}

static BOOL compare_color(D3DCOLOR c1, D3DCOLOR c2, BYTE max_diff)
{
    if (abs((c1 & 0xff) - (c2 & 0xff)) > max_diff) return FALSE;
    c1 >>= 8; c2 >>= 8;
    if (abs((c1 & 0xff) - (c2 & 0xff)) > max_diff) return FALSE;
    c1 >>= 8; c2 >>= 8;
    if (abs((c1 & 0xff) - (c2 & 0xff)) > max_diff) return FALSE;
    c1 >>= 8; c2 >>= 8;
    if (abs((c1 & 0xff) - (c2 & 0xff)) > max_diff) return FALSE;
    return TRUE;
}

static BOOL ddraw_get_identifier(IDirectDraw4 *ddraw, DDDEVICEIDENTIFIER *identifier)
{
    HRESULT hr;

    hr = IDirectDraw4_GetDeviceIdentifier(ddraw, identifier, 0);
    ok(SUCCEEDED(hr), "Failed to get device identifier, hr %#x.\n", hr);

    return SUCCEEDED(hr);
}

static BOOL ddraw_is_warp(IDirectDraw4 *ddraw)
{
    DDDEVICEIDENTIFIER identifier;

    return strcmp(winetest_platform, "wine")
            && ddraw_get_identifier(ddraw, &identifier)
            && strstr(identifier.szDriver, "warp");
}

static BOOL ddraw_is_vendor(IDirectDraw4 *ddraw, DWORD vendor)
{
    DDDEVICEIDENTIFIER identifier;

    return strcmp(winetest_platform, "wine")
            && ddraw_get_identifier(ddraw, &identifier)
            && identifier.dwVendorId == vendor;
}

static BOOL ddraw_is_intel(IDirectDraw4 *ddraw)
{
    return ddraw_is_vendor(ddraw, 0x8086);
}

static BOOL ddraw_is_nvidia(IDirectDraw4 *ddraw)
{
    return ddraw_is_vendor(ddraw, 0x10de);
}

static BOOL ddraw_is_vmware(IDirectDraw4 *ddraw)
{
    return ddraw_is_vendor(ddraw, 0x15ad);
}

static IDirectDrawSurface4 *create_overlay(IDirectDraw4 *ddraw,
        unsigned int width, unsigned int height, DWORD format)
{
    IDirectDrawSurface4 *surface;
    DDSURFACEDESC2 desc;

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    desc.dwWidth = width;
    desc.dwHeight = height;
    desc.ddsCaps.dwCaps = DDSCAPS_OVERLAY;
    U4(desc).ddpfPixelFormat.dwSize = sizeof(U4(desc).ddpfPixelFormat);
    U4(desc).ddpfPixelFormat.dwFlags = DDPF_FOURCC;
    U4(desc).ddpfPixelFormat.dwFourCC = format;

    if (FAILED(IDirectDraw4_CreateSurface(ddraw, &desc, &surface, NULL)))
        return NULL;
    return surface;
}

static HWND create_window(void)
{
    RECT r = {0, 0, 640, 480};

    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW | WS_VISIBLE, FALSE);

    return CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top, NULL, NULL, NULL, NULL);
}

static DWORD WINAPI create_window_thread_proc(void *param)
{
    struct create_window_thread_param *p = param;
    DWORD res;
    BOOL ret;

    p->window = create_window();
    ret = SetEvent(p->window_created);
    ok(ret, "SetEvent failed, last error %#x.\n", GetLastError());

    for (;;)
    {
        MSG msg;

        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
            DispatchMessageA(&msg);
        res = WaitForSingleObject(p->destroy_window, 100);
        if (res == WAIT_OBJECT_0)
            break;
        if (res != WAIT_TIMEOUT)
        {
            ok(0, "Wait failed (%#x), last error %#x.\n", res, GetLastError());
            break;
        }
    }

    DestroyWindow(p->window);

    return 0;
}

static void create_window_thread(struct create_window_thread_param *p)
{
    DWORD res, tid;

    p->window_created = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(!!p->window_created, "CreateEvent failed, last error %#x.\n", GetLastError());
    p->destroy_window = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(!!p->destroy_window, "CreateEvent failed, last error %#x.\n", GetLastError());
    p->thread = CreateThread(NULL, 0, create_window_thread_proc, p, 0, &tid);
    ok(!!p->thread, "Failed to create thread, last error %#x.\n", GetLastError());
    res = WaitForSingleObject(p->window_created, INFINITE);
    ok(res == WAIT_OBJECT_0, "Wait failed (%#x), last error %#x.\n", res, GetLastError());
}

static void destroy_window_thread(struct create_window_thread_param *p)
{
    SetEvent(p->destroy_window);
    WaitForSingleObject(p->thread, INFINITE);
    CloseHandle(p->destroy_window);
    CloseHandle(p->window_created);
    CloseHandle(p->thread);
}

static IDirectDrawSurface4 *get_depth_stencil(IDirect3DDevice3 *device)
{
    IDirectDrawSurface4 *rt, *ret;
    DDSCAPS2 caps = {DDSCAPS_ZBUFFER, 0, 0, {0}};
    HRESULT hr;

    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get the render target, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_GetAttachedSurface(rt, &caps, &ret);
    ok(SUCCEEDED(hr) || hr == DDERR_NOTFOUND, "Failed to get the z buffer, hr %#x.\n", hr);
    IDirectDrawSurface4_Release(rt);
    return ret;
}

static HRESULT set_display_mode(IDirectDraw4 *ddraw, DWORD width, DWORD height)
{
    if (SUCCEEDED(IDirectDraw4_SetDisplayMode(ddraw, width, height, 32, 0, 0)))
        return DD_OK;
    return IDirectDraw4_SetDisplayMode(ddraw, width, height, 24, 0, 0);
}

static D3DCOLOR get_surface_color(IDirectDrawSurface4 *surface, UINT x, UINT y)
{
    RECT rect = {x, y, x + 1, y + 1};
    DDSURFACEDESC2 surface_desc;
    D3DCOLOR color;
    HRESULT hr;

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);

    hr = IDirectDrawSurface4_Lock(surface, &rect, &surface_desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    if (FAILED(hr))
        return 0xdeadbeef;

    color = *((DWORD *)surface_desc.lpSurface) & 0x00ffffff;

    hr = IDirectDrawSurface4_Unlock(surface, &rect);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    return color;
}

static HRESULT CALLBACK enum_z_fmt(DDPIXELFORMAT *format, void *ctx)
{
    DDPIXELFORMAT *z_fmt = ctx;

    if (U1(*format).dwZBufferBitDepth > U1(*z_fmt).dwZBufferBitDepth)
        *z_fmt = *format;

    return DDENUMRET_OK;
}

static IDirectDraw4 *create_ddraw(void)
{
    IDirectDraw4 *ddraw4;
    IDirectDraw *ddraw1;
    HRESULT hr;

    if (FAILED(DirectDrawCreate(NULL, &ddraw1, NULL)))
        return NULL;

    hr = IDirectDraw_QueryInterface(ddraw1, &IID_IDirectDraw4, (void **)&ddraw4);
    IDirectDraw_Release(ddraw1);
    if (FAILED(hr))
        return NULL;

    return ddraw4;
}

static IDirect3DDevice3 *create_device(HWND window, DWORD coop_level)
{
    IDirectDrawSurface4 *surface, *ds;
    IDirect3DDevice3 *device = NULL;
    DDSURFACEDESC2 surface_desc;
    IDirectDraw4 *ddraw4;
    DDPIXELFORMAT z_fmt;
    IDirect3D3 *d3d3;
    HRESULT hr;

    if (!(ddraw4 = create_ddraw()))
        return NULL;

    hr = IDirectDraw4_SetCooperativeLevel(ddraw4, window, coop_level);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    surface_desc.dwWidth = 640;
    surface_desc.dwHeight = 480;

    hr = IDirectDraw4_CreateSurface(ddraw4, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    if (coop_level & DDSCL_NORMAL)
    {
        IDirectDrawClipper *clipper;

        hr = IDirectDraw4_CreateClipper(ddraw4, 0, &clipper, NULL);
        ok(SUCCEEDED(hr), "Failed to create clipper, hr %#x.\n", hr);
        hr = IDirectDrawClipper_SetHWnd(clipper, 0, window);
        ok(SUCCEEDED(hr), "Failed to set clipper window, hr %#x.\n", hr);
        hr = IDirectDrawSurface4_SetClipper(surface, clipper);
        ok(SUCCEEDED(hr), "Failed to set surface clipper, hr %#x.\n", hr);
        IDirectDrawClipper_Release(clipper);
    }

    hr = IDirectDraw4_QueryInterface(ddraw4, &IID_IDirect3D3, (void **)&d3d3);
    IDirectDraw4_Release(ddraw4);
    if (FAILED(hr))
    {
        IDirectDrawSurface4_Release(surface);
        return NULL;
    }

    memset(&z_fmt, 0, sizeof(z_fmt));
    hr = IDirect3D3_EnumZBufferFormats(d3d3, &IID_IDirect3DHALDevice, enum_z_fmt, &z_fmt);
    if (FAILED(hr) || !z_fmt.dwSize)
    {
        IDirect3D3_Release(d3d3);
        IDirectDrawSurface4_Release(surface);
        return NULL;
    }

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    U4(surface_desc).ddpfPixelFormat = z_fmt;
    surface_desc.dwWidth = 640;
    surface_desc.dwHeight = 480;
    hr = IDirectDraw4_CreateSurface(ddraw4, &surface_desc, &ds, NULL);
    ok(SUCCEEDED(hr), "Failed to create depth buffer, hr %#x.\n", hr);
    if (FAILED(hr))
    {
        IDirect3D3_Release(d3d3);
        IDirectDrawSurface4_Release(surface);
        return NULL;
    }

    hr = IDirectDrawSurface_AddAttachedSurface(surface, ds);
    ok(SUCCEEDED(hr), "Failed to attach depth buffer, hr %#x.\n", hr);
    IDirectDrawSurface4_Release(ds);
    if (FAILED(hr))
    {
        IDirect3D3_Release(d3d3);
        IDirectDrawSurface4_Release(surface);
        return NULL;
    }

    hr = IDirect3D3_CreateDevice(d3d3, &IID_IDirect3DHALDevice, surface, &device, NULL);
    IDirect3D3_Release(d3d3);
    IDirectDrawSurface4_Release(surface);
    if (FAILED(hr))
        return NULL;

    return device;
}

static IDirect3DViewport3 *create_viewport(IDirect3DDevice3 *device, UINT x, UINT y, UINT w, UINT h)
{
    IDirect3DViewport3 *viewport;
    D3DVIEWPORT2 vp;
    IDirect3D3 *d3d;
    HRESULT hr;

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get d3d interface, hr %#x.\n", hr);
    hr = IDirect3D3_CreateViewport(d3d, &viewport, NULL);
    ok(SUCCEEDED(hr), "Failed to create viewport, hr %#x.\n", hr);
    hr = IDirect3DDevice3_AddViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to add viewport, hr %#x.\n", hr);
    memset(&vp, 0, sizeof(vp));
    vp.dwSize = sizeof(vp);
    vp.dwX = x;
    vp.dwY = y;
    vp.dwWidth = w;
    vp.dwHeight = h;
    vp.dvClipX = -1.0f;
    vp.dvClipY =  1.0f;
    vp.dvClipWidth = 2.0f;
    vp.dvClipHeight = 2.0f;
    vp.dvMinZ = 0.0f;
    vp.dvMaxZ = 1.0f;
    hr = IDirect3DViewport3_SetViewport2(viewport, &vp);
    ok(SUCCEEDED(hr), "Failed to set viewport data, hr %#x.\n", hr);
    IDirect3D3_Release(d3d);

    return viewport;
}

static void destroy_viewport(IDirect3DDevice3 *device, IDirect3DViewport3 *viewport)
{
    HRESULT hr;

    hr = IDirect3DDevice3_DeleteViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to delete viewport, hr %#x.\n", hr);
    IDirect3DViewport3_Release(viewport);
}

static IDirect3DMaterial3 *create_material(IDirect3DDevice3 *device, D3DMATERIAL *mat)
{
    IDirect3DMaterial3 *material;
    IDirect3D3 *d3d;
    HRESULT hr;

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get d3d interface, hr %#x.\n", hr);
    hr = IDirect3D3_CreateMaterial(d3d, &material, NULL);
    ok(SUCCEEDED(hr), "Failed to create material, hr %#x.\n", hr);
    hr = IDirect3DMaterial3_SetMaterial(material, mat);
    ok(SUCCEEDED(hr), "Failed to set material data, hr %#x.\n", hr);
    IDirect3D3_Release(d3d);

    return material;
}

static IDirect3DMaterial3 *create_diffuse_material(IDirect3DDevice3 *device, float r, float g, float b, float a)
{
    D3DMATERIAL mat;

    memset(&mat, 0, sizeof(mat));
    mat.dwSize = sizeof(mat);
    U1(U(mat).diffuse).r = r;
    U2(U(mat).diffuse).g = g;
    U3(U(mat).diffuse).b = b;
    U4(U(mat).diffuse).a = a;

    return create_material(device, &mat);
}

static IDirect3DMaterial3 *create_specular_material(IDirect3DDevice3 *device,
        float r, float g, float b, float a, float power)
{
    D3DMATERIAL mat;

    memset(&mat, 0, sizeof(mat));
    mat.dwSize = sizeof(mat);
    U1(U2(mat).specular).r = r;
    U2(U2(mat).specular).g = g;
    U3(U2(mat).specular).b = b;
    U4(U2(mat).specular).a = a;
    U4(mat).power = power;

    return create_material(device, &mat);
}

static IDirect3DMaterial3 *create_emissive_material(IDirect3DDevice3 *device, float r, float g, float b, float a)
{
    D3DMATERIAL mat;

    memset(&mat, 0, sizeof(mat));
    mat.dwSize = sizeof(mat);
    U1(U3(mat).emissive).r = r;
    U2(U3(mat).emissive).g = g;
    U3(U3(mat).emissive).b = b;
    U4(U3(mat).emissive).a = a;

    return create_material(device, &mat);
}

static void destroy_material(IDirect3DMaterial3 *material)
{
    IDirect3DMaterial3_Release(material);
}

struct message
{
    UINT message;
    BOOL check_wparam;
    WPARAM expect_wparam;
};

static const struct message *expect_messages;

static LRESULT CALLBACK test_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (expect_messages && message == expect_messages->message)
    {
        if (expect_messages->check_wparam)
            ok (wparam == expect_messages->expect_wparam,
                    "Got unexpected wparam %lx for message %x, expected %lx.\n",
                    wparam, message, expect_messages->expect_wparam);

        ++expect_messages;
    }

    return DefWindowProcA(hwnd, message, wparam, lparam);
}

/* Set the wndproc back to what ddraw expects it to be, and release the ddraw
 * interface. This prevents subsequent SetCooperativeLevel() calls on a
 * different window from failing with DDERR_HWNDALREADYSET. */
static void fix_wndproc(HWND window, LONG_PTR proc)
{
    IDirectDraw4 *ddraw;
    HRESULT hr;

    if (!(ddraw = create_ddraw()))
        return;

    SetWindowLongPtrA(window, GWLP_WNDPROC, proc);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    IDirectDraw4_Release(ddraw);
}

static void test_process_vertices(void)
{
    IDirect3DVertexBuffer *src_vb, *dst_vb;
    IDirect3DViewport3 *viewport;
    D3DVERTEXBUFFERDESC vb_desc;
    IDirect3DDevice3 *device;
    struct vec3 *src_data;
    struct vec4 *dst_data;
    IDirect3D3 *d3d3;
    D3DVIEWPORT2 vp2;
    D3DVIEWPORT vp1;
    HWND window;
    HRESULT hr;

    static D3DMATRIX identity =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    static D3DMATRIX projection =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        6.0f, 7.0f, 8.0f, 1.0f,
    };

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d3);
    ok(SUCCEEDED(hr), "Failed to get Direct3D3 interface, hr %#x.\n", hr);

    memset(&vb_desc, 0, sizeof(vb_desc));
    vb_desc.dwSize = sizeof(vb_desc);
    vb_desc.dwFVF = D3DFVF_XYZ;
    vb_desc.dwNumVertices = 3;
    hr = IDirect3D3_CreateVertexBuffer(d3d3, &vb_desc, &src_vb, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to create source vertex buffer, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer_Lock(src_vb, DDLOCK_WRITEONLY, (void **)&src_data, NULL);
    ok(SUCCEEDED(hr), "Failed to lock source vertex buffer, hr %#x.\n", hr);
    src_data[0].x = -1.0f;
    src_data[0].y = -1.0f;
    src_data[0].z = -1.0f;
    src_data[1].x = 0.0f;
    src_data[1].y = 0.0f;
    src_data[1].z = 0.0f;
    src_data[2].x = 1.0f;
    src_data[2].y = 1.0f;
    src_data[2].z = 1.0f;
    hr = IDirect3DVertexBuffer_Unlock(src_vb);
    ok(SUCCEEDED(hr), "Failed to unlock source vertex buffer, hr %#x.\n", hr);

    memset(&vb_desc, 0, sizeof(vb_desc));
    vb_desc.dwSize = sizeof(vb_desc);
    vb_desc.dwFVF = D3DFVF_XYZRHW;
    vb_desc.dwNumVertices = 3;
    hr = IDirect3D3_CreateVertexBuffer(d3d3, &vb_desc, &dst_vb, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to create destination vertex buffer, hr %#x.\n", hr);

    hr = IDirect3D3_CreateViewport(d3d3, &viewport, NULL);
    ok(SUCCEEDED(hr), "Failed to create viewport, hr %#x.\n", hr);
    hr = IDirect3DDevice3_AddViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to add viewport, hr %#x.\n", hr);
    vp2.dwSize = sizeof(vp2);
    vp2.dwX = 10;
    vp2.dwY = 20;
    vp2.dwWidth = 100;
    vp2.dwHeight = 200;
    vp2.dvClipX = 2.0f;
    vp2.dvClipY = 3.0f;
    vp2.dvClipWidth = 4.0f;
    vp2.dvClipHeight = 5.0f;
    vp2.dvMinZ = -2.0f;
    vp2.dvMaxZ = 3.0f;
    hr = IDirect3DViewport3_SetViewport2(viewport, &vp2);
    ok(SUCCEEDED(hr), "Failed to set viewport data, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);

    hr = IDirect3DDevice3_SetTransform(device, D3DTRANSFORMSTATE_WORLD, &identity);
    ok(SUCCEEDED(hr), "Failed to set world transformation, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTransform(device, D3DTRANSFORMSTATE_VIEW, &identity);
    ok(SUCCEEDED(hr), "Failed to set view transformation, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTransform(device, D3DTRANSFORMSTATE_PROJECTION, &identity);
    ok(SUCCEEDED(hr), "Failed to set projection transformation, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer_ProcessVertices(dst_vb, D3DVOP_TRANSFORM, 0, 3, src_vb, 0, device, 0);
    ok(SUCCEEDED(hr), "Failed to process vertices, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer_Lock(dst_vb, DDLOCK_READONLY, (void **)&dst_data, NULL);
    ok(SUCCEEDED(hr), "Failed to lock destination vertex buffer, hr %#x.\n", hr);
    ok(compare_vec4(&dst_data[0], -6.500e+1f, +1.800e+2f, +2.000e-1f, +1.000e+0f, 4096),
            "Got unexpected vertex 0 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[0].x, dst_data[0].y, dst_data[0].z, dst_data[0].w);
    ok(compare_vec4(&dst_data[1], -4.000e+1f, +1.400e+2f, +4.000e-1f, +1.000e+0f, 4096),
            "Got unexpected vertex 1 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[1].x, dst_data[1].y, dst_data[1].z, dst_data[1].w);
    ok(compare_vec4(&dst_data[2], -1.500e+1f, +1.000e+2f, +6.000e-1f, +1.000e+0f, 4096),
            "Got unexpected vertex 2 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[2].x, dst_data[2].y, dst_data[2].z, dst_data[2].w);
    hr = IDirect3DVertexBuffer_Unlock(dst_vb);
    ok(SUCCEEDED(hr), "Failed to unlock destination vertex buffer, hr %#x.\n", hr);

    hr = IDirect3DDevice3_MultiplyTransform(device, D3DTRANSFORMSTATE_PROJECTION, &projection);
    ok(SUCCEEDED(hr), "Failed to set projection transformation, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer_ProcessVertices(dst_vb, D3DVOP_TRANSFORM, 0, 3, src_vb, 0, device, 0);
    ok(SUCCEEDED(hr), "Failed to process vertices, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer_Lock(dst_vb, DDLOCK_READONLY, (void **)&dst_data, NULL);
    ok(SUCCEEDED(hr), "Failed to lock destination vertex buffer, hr %#x.\n", hr);
    ok(compare_vec4(&dst_data[0], +8.500e+1f, -1.000e+2f, +1.800e+0f, +1.000e+0f, 4096),
            "Got unexpected vertex 0 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[0].x, dst_data[0].y, dst_data[0].z, dst_data[0].w);
    ok(compare_vec4(&dst_data[1], +1.100e+2f, -1.400e+2f, +2.000e+0f, +1.000e+0f, 4096),
            "Got unexpected vertex 1 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[1].x, dst_data[1].y, dst_data[1].z, dst_data[1].w);
    ok(compare_vec4(&dst_data[2], +1.350e+2f, -1.800e+2f, +2.200e+0f, +1.000e+0f, 4096),
            "Got unexpected vertex 2 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[2].x, dst_data[2].y, dst_data[2].z, dst_data[2].w);
    hr = IDirect3DVertexBuffer_Unlock(dst_vb);
    ok(SUCCEEDED(hr), "Failed to unlock destination vertex buffer, hr %#x.\n", hr);

    vp2.dwSize = sizeof(vp2);
    vp2.dwX = 30;
    vp2.dwY = 40;
    vp2.dwWidth = 90;
    vp2.dwHeight = 80;
    vp2.dvClipX = 4.0f;
    vp2.dvClipY = 6.0f;
    vp2.dvClipWidth = 2.0f;
    vp2.dvClipHeight = 4.0f;
    vp2.dvMinZ = 3.0f;
    vp2.dvMaxZ = -2.0f;
    hr = IDirect3DViewport3_SetViewport2(viewport, &vp2);
    ok(SUCCEEDED(hr), "Failed to set viewport data, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer_ProcessVertices(dst_vb, D3DVOP_TRANSFORM, 0, 3, src_vb, 0, device, 0);
    ok(SUCCEEDED(hr), "Failed to process vertices, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer_Lock(dst_vb, DDLOCK_READONLY, (void **)&dst_data, NULL);
    ok(SUCCEEDED(hr), "Failed to lock destination vertex buffer, hr %#x.\n", hr);
    ok(compare_vec4(&dst_data[0], +7.500e+1f, +4.000e+1f, -8.000e-1f, +1.000e+0f, 4096),
            "Got unexpected vertex 0 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[0].x, dst_data[0].y, dst_data[0].z, dst_data[0].w);
    ok(compare_vec4(&dst_data[1], +1.200e+2f, +2.000e+1f, -1.000e+0f, +1.000e+0f, 4096),
            "Got unexpected vertex 1 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[1].x, dst_data[1].y, dst_data[1].z, dst_data[1].w);
    ok(compare_vec4(&dst_data[2], +1.650e+2f, +0.000e+0f, -1.200e+0f, +1.000e+0f, 4096),
            "Got unexpected vertex 2 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[2].x, dst_data[2].y, dst_data[2].z, dst_data[2].w);
    hr = IDirect3DVertexBuffer_Unlock(dst_vb);
    ok(SUCCEEDED(hr), "Failed to unlock destination vertex buffer, hr %#x.\n", hr);

    vp1.dwSize = sizeof(vp1);
    vp1.dwX = 30;
    vp1.dwY = 40;
    vp1.dwWidth = 90;
    vp1.dwHeight = 80;
    vp1.dvScaleX = 7.0f;
    vp1.dvScaleY = 2.0f;
    vp1.dvMaxX = 6.0f;
    vp1.dvMaxY = 10.0f;
    vp1.dvMinZ = -2.0f;
    vp1.dvMaxZ = 3.0f;
    hr = IDirect3DViewport3_SetViewport(viewport, &vp1);
    ok(SUCCEEDED(hr), "Failed to set viewport data, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer_ProcessVertices(dst_vb, D3DVOP_TRANSFORM, 0, 3, src_vb, 0, device, 0);
    ok(SUCCEEDED(hr), "Failed to process vertices, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer_Lock(dst_vb, DDLOCK_READONLY, (void **)&dst_data, NULL);
    ok(SUCCEEDED(hr), "Failed to lock destination vertex buffer, hr %#x.\n", hr);
    ok(compare_vec4(&dst_data[0], +1.100e+2f, +6.800e+1f, +7.000e+0f, +1.000e+0f, 4096),
            "Got unexpected vertex 0 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[0].x, dst_data[0].y, dst_data[0].z, dst_data[0].w);
    ok(compare_vec4(&dst_data[1], +1.170e+2f, +6.600e+1f, +8.000e+0f, +1.000e+0f, 4096),
            "Got unexpected vertex 1 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[1].x, dst_data[1].y, dst_data[1].z, dst_data[1].w);
    ok(compare_vec4(&dst_data[2], +1.240e+2f, +6.400e+1f, +9.000e+0f, +1.000e+0f, 4096),
            "Got unexpected vertex 2 {%.8e, %.8e, %.8e, %.8e}.\n",
            dst_data[2].x, dst_data[2].y, dst_data[2].z, dst_data[2].w);
    hr = IDirect3DVertexBuffer_Unlock(dst_vb);
    ok(SUCCEEDED(hr), "Failed to unlock destination vertex buffer, hr %#x.\n", hr);

    hr = IDirect3DDevice3_DeleteViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to delete viewport, hr %#x.\n", hr);

    IDirect3DVertexBuffer_Release(dst_vb);
    IDirect3DVertexBuffer_Release(src_vb);
    IDirect3DViewport3_Release(viewport);
    IDirect3D3_Release(d3d3);
    IDirect3DDevice3_Release(device);
    DestroyWindow(window);
}

static void test_coop_level_create_device_window(void)
{
    HWND focus_window, device_window;
    IDirectDraw4 *ddraw;
    HRESULT hr;

    focus_window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW | DDSCL_NORMAL);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW | DDSCL_NORMAL | DDSCL_FULLSCREEN);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DDERR_NOFOCUSWINDOW || broken(hr == DDERR_INVALIDPARAMS), "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");

    /* Windows versions before 98 / NT5 don't support DDSCL_CREATEDEVICEWINDOW. */
    if (broken(hr == DDERR_INVALIDPARAMS))
    {
        win_skip("DDSCL_CREATEDEVICEWINDOW not supported, skipping test.\n");
        IDirectDraw4_Release(ddraw);
        DestroyWindow(focus_window);
        return;
    }

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, focus_window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_SETFOCUSWINDOW
            | DDSCL_CREATEDEVICEWINDOW | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DDERR_NOHWND, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!!device_window, "Device window not found.\n");

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, focus_window, DDSCL_SETFOCUSWINDOW
            | DDSCL_CREATEDEVICEWINDOW | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!!device_window, "Device window not found.\n");

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DDERR_NOFOCUSWINDOW, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, focus_window, DDSCL_SETFOCUSWINDOW);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!!device_window, "Device window not found.\n");

    IDirectDraw4_Release(ddraw);
    DestroyWindow(focus_window);
}

static void test_clipper_blt(void)
{
    IDirectDrawSurface4 *src_surface, *dst_surface;
    RECT client_rect, src_rect;
    IDirectDrawClipper *clipper;
    DDSURFACEDESC2 surface_desc;
    unsigned int i, j, x, y;
    IDirectDraw4 *ddraw;
    RGNDATA *rgn_data;
    D3DCOLOR color;
    ULONG refcount;
    HRGN r1, r2;
    HWND window;
    DDBLTFX fx;
    HRESULT hr;
    DWORD *ptr;
    DWORD ret;

    static const DWORD src_data[] =
    {
        0xff0000ff, 0xff0000ff, 0xff00ff00, 0xffff0000, 0xffffffff, 0xffffffff,
        0xff0000ff, 0xff0000ff, 0xff00ff00, 0xffff0000, 0xffffffff, 0xffffffff,
        0xff0000ff, 0xff0000ff, 0xff00ff00, 0xffff0000, 0xffffffff, 0xffffffff,
    };
    static const D3DCOLOR expected1[] =
    {
        0x000000ff, 0x0000ff00, 0x00000000, 0x00000000,
        0x000000ff, 0x0000ff00, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00ff0000, 0x00ffffff,
        0x00000000, 0x00000000, 0x00ff0000, 0x00ffffff,
    };
    /* Nvidia on Windows seems to have an off-by-one error
     * when processing source rectangles. Our left = 1 and
     * right = 5 input reads from x = {1, 2, 3}. x = 4 is
     * read as well, but only for the edge pixels on the
     * output image. The bug happens on the y axis as well,
     * but we only read one row there, and all source rows
     * contain the same data. This bug is not dependent on
     * the presence of a clipper. */
    static const D3DCOLOR expected1_broken[] =
    {
        0x000000ff, 0x000000ff, 0x00000000, 0x00000000,
        0x000000ff, 0x000000ff, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00ff0000, 0x00ff0000,
        0x00000000, 0x00000000, 0x0000ff00, 0x00ff0000,
    };
    static const D3DCOLOR expected2[] =
    {
        0x000000ff, 0x000000ff, 0x00000000, 0x00000000,
        0x000000ff, 0x000000ff, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x000000ff, 0x000000ff,
        0x00000000, 0x00000000, 0x000000ff, 0x000000ff,
    };

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            10, 10, 640, 480, 0, 0, 0, 0);
    ShowWindow(window, SW_SHOW);
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    ret = GetClientRect(window, &client_rect);
    ok(ret, "Failed to get client rect.\n");
    ret = MapWindowPoints(window, NULL, (POINT *)&client_rect, 2);
    ok(ret, "Failed to map client rect.\n");

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    hr = IDirectDraw4_CreateClipper(ddraw, 0, &clipper, NULL);
    ok(SUCCEEDED(hr), "Failed to create clipper, hr %#x.\n", hr);
    hr = IDirectDrawClipper_GetClipList(clipper, NULL, NULL, &ret);
    ok(hr == DDERR_NOCLIPLIST, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawClipper_SetHWnd(clipper, 0, window);
    ok(SUCCEEDED(hr), "Failed to set clipper window, hr %#x.\n", hr);
    hr = IDirectDrawClipper_GetClipList(clipper, NULL, NULL, &ret);
    ok(SUCCEEDED(hr), "Failed to get clip list size, hr %#x.\n", hr);
    rgn_data = HeapAlloc(GetProcessHeap(), 0, ret);
    hr = IDirectDrawClipper_GetClipList(clipper, NULL, rgn_data, &ret);
    ok(SUCCEEDED(hr), "Failed to get clip list, hr %#x.\n", hr);
    ok(rgn_data->rdh.dwSize == sizeof(rgn_data->rdh), "Got unexpected structure size %#x.\n", rgn_data->rdh.dwSize);
    ok(rgn_data->rdh.iType == RDH_RECTANGLES, "Got unexpected type %#x.\n", rgn_data->rdh.iType);
    ok(rgn_data->rdh.nCount >= 1, "Got unexpected count %u.\n", rgn_data->rdh.nCount);
    ok(EqualRect(&rgn_data->rdh.rcBound, &client_rect),
            "Got unexpected bounding rect %s, expected %s.\n",
            wine_dbgstr_rect(&rgn_data->rdh.rcBound), wine_dbgstr_rect(&client_rect));
    HeapFree(GetProcessHeap(), 0, rgn_data);

    r1 = CreateRectRgn(0, 0, 320, 240);
    ok(!!r1, "Failed to create region.\n");
    r2 = CreateRectRgn(320, 240, 640, 480);
    ok(!!r2, "Failed to create region.\n");
    CombineRgn(r1, r1, r2, RGN_OR);
    ret = GetRegionData(r1, 0, NULL);
    rgn_data = HeapAlloc(GetProcessHeap(), 0, ret);
    ret = GetRegionData(r1, ret, rgn_data);
    ok(!!ret, "Failed to get region data.\n");

    DeleteObject(r2);
    DeleteObject(r1);

    hr = IDirectDrawClipper_SetClipList(clipper, rgn_data, 0);
    ok(hr == DDERR_CLIPPERISUSINGHWND, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawClipper_SetHWnd(clipper, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to set clipper window, hr %#x.\n", hr);
    hr = IDirectDrawClipper_SetClipList(clipper, rgn_data, 0);
    ok(SUCCEEDED(hr), "Failed to set clip list, hr %#x.\n", hr);

    HeapFree(GetProcessHeap(), 0, rgn_data);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    surface_desc.dwWidth = 640;
    surface_desc.dwHeight = 480;
    U4(surface_desc).ddpfPixelFormat.dwSize = sizeof(U4(surface_desc).ddpfPixelFormat);
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount = 32;
    U2(U4(surface_desc).ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(U4(surface_desc).ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(U4(surface_desc).ddpfPixelFormat).dwBBitMask = 0x000000ff;

    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &src_surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create source surface, hr %#x.\n", hr);
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &dst_surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create destination surface, hr %#x.\n", hr);

    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    hr = IDirectDrawSurface4_Blt(src_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear source surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Blt(dst_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear destination surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_Lock(src_surface, NULL, &surface_desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock source surface, hr %#x.\n", hr);
    ok(U1(surface_desc).lPitch == 2560, "Got unexpected surface pitch %u.\n", U1(surface_desc).lPitch);
    ptr = surface_desc.lpSurface;
    memcpy(&ptr[   0], &src_data[ 0], 6 * sizeof(DWORD));
    memcpy(&ptr[ 640], &src_data[ 6], 6 * sizeof(DWORD));
    memcpy(&ptr[1280], &src_data[12], 6 * sizeof(DWORD));
    hr = IDirectDrawSurface4_Unlock(src_surface, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock source surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_SetClipper(dst_surface, clipper);
    ok(SUCCEEDED(hr), "Failed to set clipper, hr %#x.\n", hr);

    SetRect(&src_rect, 1, 1, 5, 2);
    hr = IDirectDrawSurface4_Blt(dst_surface, NULL, src_surface, &src_rect, DDBLT_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);
    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            x = 80 * ((2 * j) + 1);
            y = 60 * ((2 * i) + 1);
            color = get_surface_color(dst_surface, x, y);
            ok(compare_color(color, expected1[i * 4 + j], 1)
                    || broken(compare_color(color, expected1_broken[i * 4 + j], 1)),
                    "Expected color 0x%08x at %u,%u, got 0x%08x.\n", expected1[i * 4 + j], x, y, color);
        }
    }

    U5(fx).dwFillColor = 0xff0000ff;
    hr = IDirectDrawSurface4_Blt(dst_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear destination surface, hr %#x.\n", hr);
    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            x = 80 * ((2 * j) + 1);
            y = 60 * ((2 * i) + 1);
            color = get_surface_color(dst_surface, x, y);
            ok(compare_color(color, expected2[i * 4 + j], 1),
                    "Expected color 0x%08x at %u,%u, got 0x%08x.\n", expected2[i * 4 + j], x, y, color);
        }
    }

    hr = IDirectDrawSurface4_BltFast(dst_surface, 0, 0, src_surface, NULL, DDBLTFAST_WAIT);
    ok(hr == DDERR_BLTFASTCANTCLIP, "Got unexpected hr %#x.\n", hr);

    hr = IDirectDrawClipper_SetHWnd(clipper, 0, window);
    ok(SUCCEEDED(hr), "Failed to set clipper window, hr %#x.\n", hr);
    hr = IDirectDrawClipper_GetClipList(clipper, NULL, NULL, &ret);
    ok(SUCCEEDED(hr), "Failed to get clip list size, hr %#x.\n", hr);
    DestroyWindow(window);
    hr = IDirectDrawClipper_GetClipList(clipper, NULL, NULL, &ret);
    ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawClipper_SetHWnd(clipper, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to set clipper window, hr %#x.\n", hr);
    hr = IDirectDrawClipper_GetClipList(clipper, NULL, NULL, &ret);
    ok(SUCCEEDED(hr), "Failed to get clip list size, hr %#x.\n", hr);
    hr = IDirectDrawClipper_SetClipList(clipper, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to set clip list, hr %#x.\n", hr);
    hr = IDirectDrawClipper_GetClipList(clipper, NULL, NULL, &ret);
    ok(hr == DDERR_NOCLIPLIST, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Blt(dst_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_NOCLIPLIST, "Got unexpected hr %#x.\n", hr);

    IDirectDrawSurface4_Release(dst_surface);
    IDirectDrawSurface4_Release(src_surface);
    refcount = IDirectDrawClipper_Release(clipper);
    ok(!refcount, "Clipper has %u references left.\n", refcount);
    IDirectDraw4_Release(ddraw);
}

static void test_coop_level_d3d_state(void)
{
    D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    IDirectDrawSurface4 *rt, *surface;
    IDirect3DViewport3 *viewport;
    IDirect3DDevice3 *device;
    IDirectDraw4 *ddraw;
    IDirect3D3 *d3d;
    D3DCOLOR color;
    DWORD value;
    HWND window;
    HRESULT hr;

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    viewport = create_viewport(device, 0, 0, 640, 480);

    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);
    hr = IDirect3DDevice3_GetRenderState(device, D3DRENDERSTATE_ZENABLE, &value);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#x.\n", hr);
    ok(!!value, "Got unexpected z-enable state %#x.\n", value);
    hr = IDirect3DDevice3_GetRenderState(device, D3DRENDERSTATE_ALPHABLENDENABLE, &value);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#x.\n", hr);
    ok(!value, "Got unexpected alpha blend enable state %#x.\n", value);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0xffff0000, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x00ff0000, 1), "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get d3d interface, hr %#x.\n", hr);
    hr = IDirect3D3_QueryInterface(d3d, &IID_IDirectDraw4, (void **)&ddraw);
    ok(SUCCEEDED(hr), "Failed to get ddraw interface, hr %#x.\n", hr);
    IDirect3D3_Release(d3d);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_IsLost(rt);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDraw4_RestoreAllSurfaces(ddraw);
    ok(SUCCEEDED(hr), "Failed to restore surfaces, hr %#x.\n", hr);
    IDirectDraw4_Release(ddraw);

    hr = IDirect3DDevice3_GetRenderTarget(device, &surface);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);
    ok(surface == rt, "Got unexpected surface %p.\n", surface);
    hr = IDirect3DDevice3_GetRenderState(device, D3DRENDERSTATE_ZENABLE, &value);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#x.\n", hr);
    ok(!!value, "Got unexpected z-enable state %#x.\n", value);
    hr = IDirect3DDevice3_GetRenderState(device, D3DRENDERSTATE_ALPHABLENDENABLE, &value);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#x.\n", hr);
    ok(!!value, "Got unexpected alpha blend enable state %#x.\n", value);
    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0xff00ff00, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x0000ff00, 1), "Got unexpected color 0x%08x.\n", color);

    destroy_viewport(device, viewport);
    IDirectDrawSurface4_Release(surface);
    IDirectDrawSurface4_Release(rt);
    IDirect3DDevice3_Release(device);
    DestroyWindow(window);
}

static void test_surface_interface_mismatch(void)
{
    IDirectDraw4 *ddraw = NULL;
    IDirect3D3 *d3d = NULL;
    IDirectDrawSurface4 *surface = NULL, *ds;
    IDirectDrawSurface3 *surface3 = NULL;
    IDirect3DDevice3 *device = NULL;
    IDirect3DViewport3 *viewport = NULL;
    DDSURFACEDESC2 surface_desc;
    DDPIXELFORMAT z_fmt;
    ULONG refcount;
    HRESULT hr;
    D3DCOLOR color;
    HWND window;
    D3DRECT clear_rect = {{0}, {0}, {640}, {480}};

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    surface_desc.dwWidth = 640;
    surface_desc.dwHeight = 480;

    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_QueryInterface(surface, &IID_IDirectDrawSurface3, (void **)&surface3);
    ok(SUCCEEDED(hr), "Failed to QI IDirectDrawSurface3, hr %#x.\n", hr);

    if (FAILED(IDirectDraw4_QueryInterface(ddraw, &IID_IDirect3D3, (void **)&d3d)))
    {
        skip("D3D interface is not available, skipping test.\n");
        goto cleanup;
    }

    memset(&z_fmt, 0, sizeof(z_fmt));
    hr = IDirect3D3_EnumZBufferFormats(d3d, &IID_IDirect3DHALDevice, enum_z_fmt, &z_fmt);
    if (FAILED(hr) || !z_fmt.dwSize)
    {
        skip("No depth buffer formats available, skipping test.\n");
        goto cleanup;
    }

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    U4(surface_desc).ddpfPixelFormat = z_fmt;
    surface_desc.dwWidth = 640;
    surface_desc.dwHeight = 480;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &ds, NULL);
    ok(SUCCEEDED(hr), "Failed to create depth buffer, hr %#x.\n", hr);
    if (FAILED(hr))
        goto cleanup;

    /* Using a different surface interface version still works */
    hr = IDirectDrawSurface3_AddAttachedSurface(surface3, (IDirectDrawSurface3 *)ds);
    ok(SUCCEEDED(hr), "Failed to attach depth buffer, hr %#x.\n", hr);
    refcount = IDirectDrawSurface4_Release(ds);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);
    if (FAILED(hr))
        goto cleanup;

    /* Here too */
    hr = IDirect3D3_CreateDevice(d3d, &IID_IDirect3DHALDevice, (IDirectDrawSurface4 *)surface3, &device, NULL);
    ok(SUCCEEDED(hr), "Failed to create d3d device.\n");
    if (FAILED(hr))
        goto cleanup;

    viewport = create_viewport(device, 0, 0, 640, 480);

    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0xffff0000, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    color = get_surface_color(surface, 320, 240);
    ok(compare_color(color, 0x00ff0000, 1), "Got unexpected color 0x%08x.\n", color);

cleanup:
    if (viewport)
        destroy_viewport(device, viewport);
    if (surface3) IDirectDrawSurface3_Release(surface3);
    if (surface) IDirectDrawSurface4_Release(surface);
    if (device) IDirect3DDevice3_Release(device);
    if (d3d) IDirect3D3_Release(d3d);
    if (ddraw) IDirectDraw4_Release(ddraw);
    DestroyWindow(window);
}

static void test_coop_level_threaded(void)
{
    struct create_window_thread_param p;
    IDirectDraw4 *ddraw;
    HRESULT hr;

    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    create_window_thread(&p);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, p.window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    IDirectDraw4_Release(ddraw);
    destroy_window_thread(&p);
}

static void test_depth_blit(void)
{
    static struct
    {
        float x, y, z;
        DWORD color;
    }
    quad1[] =
    {
        { -1.0,  1.0, 0.50f, 0xff00ff00},
        {  1.0,  1.0, 0.50f, 0xff00ff00},
        { -1.0, -1.0, 0.50f, 0xff00ff00},
        {  1.0, -1.0, 0.50f, 0xff00ff00},
    };
    static const D3DCOLOR expected_colors[4][4] =
    {
        {0x00ff0000, 0x00ff0000, 0x0000ff00, 0x0000ff00},
        {0x00ff0000, 0x00ff0000, 0x0000ff00, 0x0000ff00},
        {0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00},
        {0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00},
    };
    DDSURFACEDESC2 ddsd_new, ddsd_existing;

    IDirect3DDevice3 *device;
    IDirectDrawSurface4 *ds1, *ds2, *ds3, *rt;
    IDirect3DViewport3 *viewport;
    RECT src_rect, dst_rect;
    unsigned int i, j;
    D3DCOLOR color;
    HRESULT hr;
    IDirect3D3 *d3d;
    IDirectDraw4 *ddraw;
    DDBLTFX fx;
    HWND window;
    D3DRECT d3drect;

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get Direct3D3 interface, hr %#x.\n", hr);
    hr = IDirect3D3_QueryInterface(d3d, &IID_IDirectDraw4, (void **)&ddraw);
    ok(SUCCEEDED(hr), "Failed to get DirectDraw4 interface, hr %#x.\n", hr);
    IDirect3D3_Release(d3d);

    ds1 = get_depth_stencil(device);

    memset(&ddsd_new, 0, sizeof(ddsd_new));
    ddsd_new.dwSize = sizeof(ddsd_new);
    memset(&ddsd_existing, 0, sizeof(ddsd_existing));
    ddsd_existing.dwSize = sizeof(ddsd_existing);
    hr = IDirectDrawSurface4_GetSurfaceDesc(ds1, &ddsd_existing);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ddsd_new.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    ddsd_new.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    ddsd_new.dwWidth = ddsd_existing.dwWidth;
    ddsd_new.dwHeight = ddsd_existing.dwHeight;
    U4(ddsd_new).ddpfPixelFormat = U4(ddsd_existing).ddpfPixelFormat;
    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd_new, &ds2, NULL);
    ok(SUCCEEDED(hr), "Failed to create a surface, hr %#x.\n", hr);
    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd_new, &ds3, NULL);
    ok(SUCCEEDED(hr), "Failed to create a surface, hr %#x.\n", hr);
    IDirectDraw4_Release(ddraw);

    viewport = create_viewport(device, 0, 0, ddsd_existing.dwWidth, ddsd_existing.dwHeight);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to activate the viewport, hr %#x.\n", hr);

    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_ZENABLE, D3DZB_TRUE);
    ok(SUCCEEDED(hr), "Failed to enable z testing, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL);
    ok(SUCCEEDED(hr), "Failed to set the z function, hr %#x.\n", hr);

    U1(d3drect).x1 = U2(d3drect).y1 = 0;
    U3(d3drect).x2 = ddsd_existing.dwWidth; U4(d3drect).y2 = ddsd_existing.dwHeight;
    hr = IDirect3DViewport3_Clear2(viewport, 1, &d3drect, D3DCLEAR_ZBUFFER, 0, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear the z buffer, hr %#x.\n", hr);

    /* Partial blit. */
    SetRect(&src_rect, 0, 0, 320, 240);
    SetRect(&dst_rect, 0, 0, 320, 240);
    hr = IDirectDrawSurface4_Blt(ds2, &dst_rect, ds1, &src_rect, DDBLT_WAIT, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    /* Different locations. */
    SetRect(&src_rect, 0, 0, 320, 240);
    SetRect(&dst_rect, 320, 240, 640, 480);
    hr = IDirectDrawSurface4_Blt(ds2, &dst_rect, ds1, &src_rect, DDBLT_WAIT, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    /* Stretched. */
    SetRect(&src_rect, 0, 0, 320, 240);
    SetRect(&dst_rect, 0, 0, 640, 480);
    hr = IDirectDrawSurface4_Blt(ds2, &dst_rect, ds1, &src_rect, DDBLT_WAIT, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    /* Flipped. */
    SetRect(&src_rect, 0, 480, 640, 0);
    SetRect(&dst_rect, 0, 0, 640, 480);
    hr = IDirectDrawSurface4_Blt(ds2, &dst_rect, ds1, &src_rect, DDBLT_WAIT, NULL);
    ok(hr == DDERR_INVALIDRECT, "Got unexpected hr %#x.\n", hr);
    SetRect(&src_rect, 0, 0, 640, 480);
    SetRect(&dst_rect, 0, 480, 640, 0);
    hr = IDirectDrawSurface4_Blt(ds2, &dst_rect, ds1, &src_rect, DDBLT_WAIT, NULL);
    ok(hr == DDERR_INVALIDRECT, "Got unexpected hr %#x.\n", hr);
    /* Full, explicit. */
    SetRect(&src_rect, 0, 0, 640, 480);
    SetRect(&dst_rect, 0, 0, 640, 480);
    hr = IDirectDrawSurface4_Blt(ds2, &dst_rect, ds1, &src_rect, DDBLT_WAIT, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    /* Depth -> color blit: Succeeds on Win7 + Radeon HD 5700, fails on WinXP + Radeon X1600 */

    /* Depth blit inside a BeginScene / EndScene pair */
    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to start a scene, hr %#x.\n", hr);
    /* From the current depth stencil */
    hr = IDirectDrawSurface4_Blt(ds2, NULL, ds1, NULL, DDBLT_WAIT, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    /* To the current depth stencil */
    hr = IDirectDrawSurface4_Blt(ds1, NULL, ds2, NULL, DDBLT_WAIT, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    /* Between unbound surfaces */
    hr = IDirectDrawSurface4_Blt(ds3, NULL, ds2, NULL, DDBLT_WAIT, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end a scene, hr %#x.\n", hr);

    /* Avoid changing the depth stencil, it doesn't work properly on Windows.
     * Instead use DDBLT_DEPTHFILL to clear the depth stencil. Unfortunately
     * drivers disagree on the meaning of dwFillDepth. Only 0 seems to produce
     * a reliable result(z = 0.0) */
    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    hr = IDirectDrawSurface4_Blt(ds2, NULL, NULL, NULL, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear the source z buffer, hr %#x.\n", hr);

    hr = IDirect3DViewport3_Clear2(viewport, 1, &d3drect, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, 0xffff0000, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear the color and z buffers, hr %#x.\n", hr);
    SetRect(&dst_rect, 0, 0, 320, 240);
    hr = IDirectDrawSurface4_Blt(ds1, &dst_rect, ds2, NULL, DDBLT_WAIT, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    IDirectDrawSurface4_Release(ds3);
    IDirectDrawSurface4_Release(ds2);
    IDirectDrawSurface4_Release(ds1);

    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to start a scene, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_DIFFUSE,
            quad1, 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end a scene, hr %#x.\n", hr);

    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);
    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            unsigned int x = 80 * ((2 * j) + 1);
            unsigned int y = 60 * ((2 * i) + 1);
            color = get_surface_color(rt, x, y);
            ok(compare_color(color, expected_colors[i][j], 1),
                    "Expected color 0x%08x at %u,%u, got 0x%08x.\n", expected_colors[i][j], x, y, color);
        }
    }
    IDirectDrawSurface4_Release(rt);

    destroy_viewport(device, viewport);
    IDirect3DDevice3_Release(device);
    DestroyWindow(window);
}

static void test_texture_load_ckey(void)
{
    IDirectDraw4 *ddraw;
    IDirectDrawSurface4 *src;
    IDirectDrawSurface4 *dst;
    IDirect3DTexture2 *src_tex;
    IDirect3DTexture2 *dst_tex;
    DDSURFACEDESC2 ddsd;
    HRESULT hr;
    DDCOLORKEY ckey;

    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    ddsd.dwHeight = 128;
    ddsd.dwWidth = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &src, NULL);
    ok(SUCCEEDED(hr), "Failed to create source texture, hr %#x.\n", hr);
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &dst, NULL);
    ok(SUCCEEDED(hr), "Failed to create destination texture, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_QueryInterface(src, &IID_IDirect3DTexture2, (void **)&src_tex);
    ok(SUCCEEDED(hr) || hr == E_NOINTERFACE, "Failed to get Direct3DTexture2 interface, hr %#x.\n", hr);
    if (FAILED(hr))
    {
        /* 64 bit ddraw does not support d3d */
        skip("Could not get Direct3DTexture2 interface, skipping texture::Load color keying tests.\n");
        IDirectDrawSurface4_Release(dst);
        IDirectDrawSurface4_Release(src);
        IDirectDraw4_Release(ddraw);
        return;
    }
    hr = IDirectDrawSurface4_QueryInterface(dst, &IID_IDirect3DTexture2, (void **)&dst_tex);
    ok(SUCCEEDED(hr), "Failed to get Direct3DTexture2 interface, hr %#x.\n", hr);

    /* No surface has a color key */
    hr = IDirect3DTexture2_Load(dst_tex, src_tex);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    ckey.dwColorSpaceLowValue = ckey.dwColorSpaceHighValue = 0xdeadbeef;
    hr = IDirectDrawSurface4_GetColorKey(dst, DDCKEY_SRCBLT, &ckey);
    ok(hr == DDERR_NOCOLORKEY, "Got unexpected hr %#x.\n", hr);
    ok(ckey.dwColorSpaceLowValue == 0xdeadbeef, "dwColorSpaceLowValue is %#x.\n", ckey.dwColorSpaceLowValue);
    ok(ckey.dwColorSpaceHighValue == 0xdeadbeef, "dwColorSpaceHighValue is %#x.\n", ckey.dwColorSpaceHighValue);

    /* Source surface has a color key */
    ckey.dwColorSpaceLowValue = ckey.dwColorSpaceHighValue = 0x0000ff00;
    hr = IDirectDrawSurface4_SetColorKey(src, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);
    hr = IDirect3DTexture2_Load(dst_tex, src_tex);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_GetColorKey(dst, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    ok(ckey.dwColorSpaceLowValue == 0x0000ff00, "dwColorSpaceLowValue is %#x.\n", ckey.dwColorSpaceLowValue);
    ok(ckey.dwColorSpaceHighValue == 0x0000ff00, "dwColorSpaceHighValue is %#x.\n", ckey.dwColorSpaceHighValue);

    /* Both surfaces have a color key: Dest ckey is overwritten */
    ckey.dwColorSpaceLowValue = ckey.dwColorSpaceHighValue = 0x000000ff;
    hr = IDirectDrawSurface4_SetColorKey(dst, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);
    hr = IDirect3DTexture2_Load(dst_tex, src_tex);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_GetColorKey(dst, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    ok(ckey.dwColorSpaceLowValue == 0x0000ff00, "dwColorSpaceLowValue is %#x.\n", ckey.dwColorSpaceLowValue);
    ok(ckey.dwColorSpaceHighValue == 0x0000ff00, "dwColorSpaceHighValue is %#x.\n", ckey.dwColorSpaceHighValue);

    /* Only the destination has a color key: It is not deleted */
    hr = IDirectDrawSurface4_SetColorKey(src, DDCKEY_SRCBLT, NULL);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_GetColorKey(src, DDCKEY_SRCBLT, &ckey);
    ok(hr == DDERR_NOCOLORKEY, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DTexture2_Load(dst_tex, src_tex);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_GetColorKey(dst, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    ok(ckey.dwColorSpaceLowValue == 0x0000ff00, "dwColorSpaceLowValue is %#x.\n", ckey.dwColorSpaceLowValue);
    ok(ckey.dwColorSpaceHighValue == 0x0000ff00, "dwColorSpaceHighValue is %#x.\n", ckey.dwColorSpaceHighValue);

    IDirect3DTexture2_Release(dst_tex);
    IDirect3DTexture2_Release(src_tex);
    IDirectDrawSurface4_Release(dst);
    IDirectDrawSurface4_Release(src);
    IDirectDraw4_Release(ddraw);
}

static ULONG get_refcount(IUnknown *test_iface)
{
    IUnknown_AddRef(test_iface);
    return IUnknown_Release(test_iface);
}

static void test_viewport(void)
{
    IDirectDraw4 *ddraw;
    IDirect3D3 *d3d;
    HRESULT hr, old_d3d_ref;
    ULONG ref;
    IDirect3DViewport *viewport;
    IDirect3DViewport2 *viewport2;
    IDirect3DViewport3 *viewport3, *another_vp, *test_vp;
    IDirectDrawGammaControl *gamma;
    IUnknown *unknown;
    HWND window;
    IDirect3DDevice3 *device;

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }
    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get Direct3D3 interface, hr %#x.\n", hr);
    hr = IDirect3D3_QueryInterface(d3d, &IID_IDirectDraw4, (void **)&ddraw);
    ok(SUCCEEDED(hr), "Failed to get DirectDraw4 interface, hr %#x.\n", hr);
    old_d3d_ref = get_refcount((IUnknown *) d3d);

    hr = IDirect3D3_CreateViewport(d3d, &viewport3, NULL);
    ok(SUCCEEDED(hr), "Failed to create viewport, hr %#x.\n", hr);
    ref = get_refcount((IUnknown *)viewport3);
    ok(ref == 1, "Initial IDirect3DViewport3 refcount is %u\n", ref);
    ref = get_refcount((IUnknown *)d3d);
    ok(ref == old_d3d_ref, "IDirect3D3 refcount is %u\n", ref);

    gamma = (IDirectDrawGammaControl *)0xdeadbeef;
    hr = IDirect3DViewport2_QueryInterface(viewport3, &IID_IDirectDrawGammaControl, (void **)&gamma);
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#x.\n", hr);
    ok(gamma == NULL, "Interface not set to NULL by failed QI call: %p\n", gamma);
    if (SUCCEEDED(hr)) IDirectDrawGammaControl_Release(gamma);
    /* NULL iid: Segfaults */

    hr = IDirect3DViewport3_QueryInterface(viewport3, &IID_IDirect3DViewport, (void **)&viewport);
    ok(SUCCEEDED(hr), "Failed to QI IDirect3DViewport, hr %#x.\n", hr);
    if (viewport)
    {
        ref = get_refcount((IUnknown *)viewport);
        ok(ref == 2, "IDirect3DViewport refcount is %u\n", ref);
        ref = get_refcount((IUnknown *)viewport3);
        ok(ref == 2, "IDirect3DViewport3 refcount is %u\n", ref);
        IDirect3DViewport_Release(viewport);
        viewport = NULL;
    }

    hr = IDirect3DViewport3_QueryInterface(viewport3, &IID_IDirect3DViewport3, (void **)&viewport2);
    ok(SUCCEEDED(hr), "Failed to QI IDirect3DViewport3, hr %#x.\n", hr);
    if (viewport2)
    {
        ref = get_refcount((IUnknown *)viewport2);
        ok(ref == 2, "IDirect3DViewport2 refcount is %u\n", ref);
        ref = get_refcount((IUnknown *)viewport3);
        ok(ref == 2, "IDirect3DViewport3 refcount is %u\n", ref);
        IDirect3DViewport3_Release(viewport2);
    }

    hr = IDirect3DViewport3_QueryInterface(viewport3, &IID_IUnknown, (void **)&unknown);
    ok(SUCCEEDED(hr), "Failed to QI IUnknown, hr %#x.\n", hr);
    if (unknown)
    {
        ref = get_refcount((IUnknown *)viewport3);
        ok(ref == 2, "IDirect3DViewport3 refcount is %u\n", ref);
        ref = get_refcount(unknown);
        ok(ref == 2, "IUnknown refcount is %u\n", ref);
        IUnknown_Release(unknown);
    }

    /* AddViewport(NULL): Segfault */
    hr = IDirect3DDevice3_DeleteViewport(device, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice3_GetCurrentViewport(device, NULL);
    ok(hr == D3DERR_NOCURRENTVIEWPORT, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3D3_CreateViewport(d3d, &another_vp, NULL);
    ok(SUCCEEDED(hr), "Failed to create viewport, hr %#x.\n", hr);

    /* Setting a viewport not in the viewport list fails */
    hr = IDirect3DDevice3_SetCurrentViewport(device, another_vp);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice3_AddViewport(device, viewport3);
    ok(SUCCEEDED(hr), "Failed to add viewport to device, hr %#x.\n", hr);
    ref = get_refcount((IUnknown *) viewport3);
    ok(ref == 2, "viewport3 refcount is %d\n", ref);
    hr = IDirect3DDevice3_AddViewport(device, another_vp);
    ok(SUCCEEDED(hr), "Failed to add viewport to device, hr %#x.\n", hr);
    ref = get_refcount((IUnknown *) another_vp);
    ok(ref == 2, "another_vp refcount is %d\n", ref);

    test_vp = (IDirect3DViewport3 *) 0xbaadc0de;
    hr = IDirect3DDevice3_GetCurrentViewport(device, &test_vp);
    ok(hr == D3DERR_NOCURRENTVIEWPORT, "Got unexpected hr %#x.\n", hr);
    ok(test_vp == (IDirect3DViewport3 *) 0xbaadc0de, "Got unexpected pointer %p\n", test_vp);

    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport3);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);
    ref = get_refcount((IUnknown *) viewport3);
    ok(ref == 3, "viewport3 refcount is %d\n", ref);
    ref = get_refcount((IUnknown *) device);
    ok(ref == 1, "device refcount is %d\n", ref);

    test_vp = NULL;
    hr = IDirect3DDevice3_GetCurrentViewport(device, &test_vp);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(test_vp == viewport3, "Got unexpected viewport %p\n", test_vp);
    ref = get_refcount((IUnknown *) viewport3);
    ok(ref == 4, "viewport3 refcount is %d\n", ref);
    if(test_vp) IDirect3DViewport3_Release(test_vp);

    /* GetCurrentViewport with a viewport set and NULL input param: Segfault */

    /* Cannot set the viewport to NULL */
    hr = IDirect3DDevice3_SetCurrentViewport(device, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "Failed to set viewport to NULL, hr %#x.\n", hr);
    test_vp = NULL;
    hr = IDirect3DDevice3_GetCurrentViewport(device, &test_vp);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(test_vp == viewport3, "Got unexpected viewport %p\n", test_vp);
    if(test_vp) IDirect3DViewport3_Release(test_vp);

    /* SetCurrentViewport properly releases the old viewport's reference */
    hr = IDirect3DDevice3_SetCurrentViewport(device, another_vp);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);
    ref = get_refcount((IUnknown *) viewport3);
    ok(ref == 2, "viewport3 refcount is %d\n", ref);
    ref = get_refcount((IUnknown *) another_vp);
    ok(ref == 3, "another_vp refcount is %d\n", ref);

    /* Unlike device2::DeleteViewport, device3::DeleteViewport releases the
     * reference held by SetCurrentViewport */
    hr = IDirect3DDevice3_DeleteViewport(device, another_vp);
    ok(SUCCEEDED(hr), "Failed to delete viewport from device, hr %#x.\n", hr);
    ref = get_refcount((IUnknown *) another_vp);
    ok(ref == 1, "another_vp refcount is %d\n", ref);

    /* GetCurrentViewport still fails */
    test_vp = NULL;
    hr = IDirect3DDevice3_GetCurrentViewport(device, &test_vp);
    ok(hr == D3DERR_NOCURRENTVIEWPORT, "Got unexpected hr %#x.\n", hr);
    ok(test_vp == NULL, "Got unexpected viewport %p\n", test_vp);
    if(test_vp) IDirect3DViewport3_Release(test_vp);

    /* Setting a different viewport doesn't have any surprises now */
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport3);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);
    ref = get_refcount((IUnknown *) viewport3);
    ok(ref == 3, "viewport3 refcount is %d\n", ref);
    ref = get_refcount((IUnknown *) another_vp);
    ok(ref == 1, "another_vp refcount is %d\n", ref);

    /* Destroying the device removes the viewport and releases the reference */
    IDirect3DDevice3_Release(device);
    ref = get_refcount((IUnknown *) viewport3);
    ok(ref == 1, "viewport3 refcount is %d\n", ref);

    ref = IDirect3DViewport3_Release(another_vp);
    ok(ref == 0, "Got unexpected ref %d\n", ref);
    ref = IDirect3DViewport3_Release(viewport3);
    ok(ref == 0, "Got unexpected ref %d\n", ref);
    IDirect3D3_Release(d3d);
    DestroyWindow(window);
    IDirectDraw4_Release(ddraw);
}

static void test_zenable(void)
{
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    static struct
    {
        struct vec4 position;
        D3DCOLOR diffuse;
    }
    tquad[] =
    {
        {{  0.0f, 480.0f, -0.5f, 1.0f}, 0xff00ff00},
        {{  0.0f,   0.0f, -0.5f, 1.0f}, 0xff00ff00},
        {{640.0f, 480.0f,  1.5f, 1.0f}, 0xff00ff00},
        {{640.0f,   0.0f,  1.5f, 1.0f}, 0xff00ff00},
    };
    IDirect3DViewport3 *viewport;
    IDirect3DDevice3 *device;
    IDirectDrawSurface4 *rt;
    D3DCOLOR color;
    HWND window;
    HRESULT hr;
    UINT x, y;
    UINT i, j;

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    viewport = create_viewport(device, 0, 0, 640, 480);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);

    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_ZENABLE, D3DZB_FALSE);
    ok(SUCCEEDED(hr), "Failed to disable z-buffering, hr %#x.\n", hr);

    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffff0000, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, tquad, 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);
    for (i = 0; i < 4; ++i)
    {
        for (j = 0; j < 4; ++j)
        {
            x = 80 * ((2 * j) + 1);
            y = 60 * ((2 * i) + 1);
            color = get_surface_color(rt, x, y);
            ok(compare_color(color, 0x0000ff00, 1),
                    "Expected color 0x0000ff00 at %u, %u, got 0x%08x.\n", x, y, color);
        }
    }
    IDirectDrawSurface4_Release(rt);

    destroy_viewport(device, viewport);
    IDirect3DDevice3_Release(device);
    DestroyWindow(window);
}

static void test_ck_rgba(void)
{
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    static struct
    {
        struct vec4 position;
        struct vec2 texcoord;
    }
    tquad[] =
    {
        {{  0.0f, 480.0f, 0.25f, 1.0f}, {0.0f, 0.0f}},
        {{  0.0f,   0.0f, 0.25f, 1.0f}, {0.0f, 1.0f}},
        {{640.0f, 480.0f, 0.25f, 1.0f}, {1.0f, 0.0f}},
        {{640.0f,   0.0f, 0.25f, 1.0f}, {1.0f, 1.0f}},
        {{  0.0f, 480.0f, 0.75f, 1.0f}, {0.0f, 0.0f}},
        {{  0.0f,   0.0f, 0.75f, 1.0f}, {0.0f, 1.0f}},
        {{640.0f, 480.0f, 0.75f, 1.0f}, {1.0f, 0.0f}},
        {{640.0f,   0.0f, 0.75f, 1.0f}, {1.0f, 1.0f}},
    };
    static const struct
    {
        D3DCOLOR fill_color;
        BOOL color_key;
        BOOL blend;
        D3DCOLOR result1, result1_broken;
        D3DCOLOR result2, result2_broken;
    }
    tests[] =
    {
        /* r200 on Windows doesn't check the alpha component when applying the color
         * key, so the key matches on every texel. */
        {0xff00ff00, TRUE,  TRUE,  0x00ff0000, 0x00ff0000, 0x000000ff, 0x000000ff},
        {0xff00ff00, TRUE,  FALSE, 0x00ff0000, 0x00ff0000, 0x000000ff, 0x000000ff},
        {0xff00ff00, FALSE, TRUE,  0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00},
        {0xff00ff00, FALSE, FALSE, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00},
        {0x7f00ff00, TRUE,  TRUE,  0x00807f00, 0x00ff0000, 0x00807f00, 0x000000ff},
        {0x7f00ff00, TRUE,  FALSE, 0x0000ff00, 0x00ff0000, 0x0000ff00, 0x000000ff},
        {0x7f00ff00, FALSE, TRUE,  0x00807f00, 0x00807f00, 0x00807f00, 0x00807f00},
        {0x7f00ff00, FALSE, FALSE, 0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00},
    };

    IDirectDrawSurface4 *surface;
    IDirect3DViewport3 *viewport;
    DDSURFACEDESC2 surface_desc;
    IDirect3DTexture2 *texture;
    IDirect3DDevice3 *device;
    IDirectDrawSurface4 *rt;
    IDirectDraw4 *ddraw;
    IDirect3D3 *d3d;
    D3DCOLOR color;
    HWND window;
    DDBLTFX fx;
    HRESULT hr;
    UINT i;

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    viewport = create_viewport(device, 0, 0, 640, 480);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get d3d interface, hr %#x.\n", hr);
    hr = IDirect3D3_QueryInterface(d3d, &IID_IDirectDraw4, (void **)&ddraw);
    ok(SUCCEEDED(hr), "Failed to get ddraw interface, hr %#x.\n", hr);
    IDirect3D3_Release(d3d);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CKSRCBLT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    surface_desc.dwWidth = 256;
    surface_desc.dwHeight = 256;
    U4(surface_desc).ddpfPixelFormat.dwSize = sizeof(U4(surface_desc).ddpfPixelFormat);
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount = 32;
    U2(U4(surface_desc).ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(U4(surface_desc).ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(U4(surface_desc).ddpfPixelFormat).dwBBitMask = 0x000000ff;
    U5(U4(surface_desc).ddpfPixelFormat).dwRGBAlphaBitMask = 0xff000000;
    surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue = 0xff00ff00;
    surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue = 0xff00ff00;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create destination surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_QueryInterface(surface, &IID_IDirect3DTexture2, (void **)&texture);
    ok(SUCCEEDED(hr), "Failed to get texture interface, hr %#x.\n", hr);

    hr = IDirect3DDevice3_SetTexture(device, 0, texture);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
    ok(SUCCEEDED(hr), "Failed to enable alpha blending, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
    ok(SUCCEEDED(hr), "Failed to enable alpha blending, hr %#x.\n", hr);

    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_COLORKEYENABLE, tests[i].color_key);
        ok(SUCCEEDED(hr), "Failed to enable color keying, hr %#x.\n", hr);
        hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_ALPHABLENDENABLE, tests[i].blend);
        ok(SUCCEEDED(hr), "Failed to enable alpha blending, hr %#x.\n", hr);

        memset(&fx, 0, sizeof(fx));
        fx.dwSize = sizeof(fx);
        U5(fx).dwFillColor = tests[i].fill_color;
        hr = IDirectDrawSurface4_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
        ok(SUCCEEDED(hr), "Failed to fill texture, hr %#x.\n", hr);

        hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect,
                D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffff0000, 1.0f, 0);
        ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
        hr = IDirect3DDevice3_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
        hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZRHW | D3DFVF_TEX1, &tquad[0], 4, 0);
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
        hr = IDirect3DDevice3_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

        color = get_surface_color(rt, 320, 240);
        ok(compare_color(color, tests[i].result1, 1) || compare_color(color, tests[i].result1_broken, 1),
                "Expected color 0x%08x for test %u, got 0x%08x.\n",
                tests[i].result1, i, color);

        U5(fx).dwFillColor = 0xff0000ff;
        hr = IDirectDrawSurface4_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
        ok(SUCCEEDED(hr), "Failed to fill texture, hr %#x.\n", hr);

        hr = IDirect3DDevice3_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
        hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZRHW | D3DFVF_TEX1, &tquad[4], 4, 0);
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
        hr = IDirect3DDevice3_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

        /* This tests that fragments that are masked out by the color key are
         * discarded, instead of just fully transparent. */
        color = get_surface_color(rt, 320, 240);
        ok(compare_color(color, tests[i].result2, 1) || compare_color(color, tests[i].result2_broken, 1),
                "Expected color 0x%08x for test %u, got 0x%08x.\n",
                tests[i].result2, i, color);
    }

    IDirectDrawSurface4_Release(rt);
    IDirect3DTexture2_Release(texture);
    IDirectDrawSurface4_Release(surface);
    destroy_viewport(device, viewport);
    IDirectDraw4_Release(ddraw);
    IDirect3DDevice3_Release(device);
    DestroyWindow(window);
}

static void test_ck_default(void)
{
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    static struct
    {
        struct vec4 position;
        struct vec2 texcoord;
    }
    tquad[] =
    {
        {{  0.0f, 480.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{  0.0f,   0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{640.0f, 480.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{640.0f,   0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    };
    IDirectDrawSurface4 *surface, *rt;
    IDirect3DViewport3 *viewport;
    DDSURFACEDESC2 surface_desc;
    IDirect3DTexture2 *texture;
    IDirect3DDevice3 *device;
    IDirectDraw4 *ddraw;
    IDirect3D3 *d3d;
    D3DCOLOR color;
    DWORD value;
    HWND window;
    DDBLTFX fx;
    HRESULT hr;

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get d3d interface, hr %#x.\n", hr);
    hr = IDirect3D3_QueryInterface(d3d, &IID_IDirectDraw4, (void **)&ddraw);
    ok(SUCCEEDED(hr), "Failed to get ddraw interface, hr %#x.\n", hr);
    IDirect3D3_Release(d3d);

    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    viewport = create_viewport(device, 0, 0, 640, 480);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CKSRCBLT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    surface_desc.dwWidth = 256;
    surface_desc.dwHeight = 256;
    U4(surface_desc).ddpfPixelFormat.dwSize = sizeof(U4(surface_desc).ddpfPixelFormat);
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount = 32;
    U2(U4(surface_desc).ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(U4(surface_desc).ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(U4(surface_desc).ddpfPixelFormat).dwBBitMask = 0x000000ff;
    surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue = 0x000000ff;
    surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue = 0x000000ff;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_QueryInterface(surface, &IID_IDirect3DTexture2, (void **)&texture);
    ok(SUCCEEDED(hr), "Failed to get texture interface, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTexture(device, 0, texture);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);

    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    U5(fx).dwFillColor = 0x000000ff;
    hr = IDirectDrawSurface4_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to fill surface, hr %#x.\n", hr);

    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0xff00ff00, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice3_GetRenderState(device, D3DRENDERSTATE_COLORKEYENABLE, &value);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#x.\n", hr);
    ok(!value, "Got unexpected color keying state %#x.\n", value);
    hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZRHW | D3DFVF_TEX1, &tquad[0], 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);
    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x000000ff, 1), "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0xff00ff00, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_COLORKEYENABLE, TRUE);
    ok(SUCCEEDED(hr), "Failed to enable color keying, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZRHW | D3DFVF_TEX1, &tquad[0], 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_GetRenderState(device, D3DRENDERSTATE_COLORKEYENABLE, &value);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#x.\n", hr);
    ok(!!value, "Got unexpected color keying state %#x.\n", value);
    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);
    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x0000ff00, 1), "Got unexpected color 0x%08x.\n", color);

    IDirect3DTexture_Release(texture);
    IDirectDrawSurface4_Release(surface);
    destroy_viewport(device, viewport);
    IDirectDrawSurface4_Release(rt);
    IDirect3DDevice3_Release(device);
    IDirectDraw4_Release(ddraw);
    DestroyWindow(window);
}

static void test_ck_complex(void)
{
    IDirectDrawSurface4 *surface, *mipmap, *tmp;
    DDSCAPS2 caps = {DDSCAPS_COMPLEX, 0, 0, {0}};
    DDSURFACEDESC2 surface_desc;
    IDirect3DDevice3 *device;
    DDCOLORKEY color_key;
    IDirectDraw4 *ddraw;
    IDirect3D3 *d3d;
    unsigned int i;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = create_window();
    if (!(device = create_device(window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }
    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get d3d interface, hr %#x.\n", hr);
    hr = IDirect3D3_QueryInterface(d3d, &IID_IDirectDraw4, (void **)&ddraw);
    ok(SUCCEEDED(hr), "Failed to get ddraw interface, hr %#x.\n", hr);
    IDirect3D3_Release(d3d);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
    surface_desc.dwWidth = 128;
    surface_desc.dwHeight = 128;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_GetColorKey(surface, DDCKEY_SRCBLT, &color_key);
    ok(hr == DDERR_NOCOLORKEY, "Got unexpected hr %#x.\n", hr);
    color_key.dwColorSpaceLowValue = 0x0000ff00;
    color_key.dwColorSpaceHighValue = 0x0000ff00;
    hr = IDirectDrawSurface4_SetColorKey(surface, DDCKEY_SRCBLT, &color_key);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);
    memset(&color_key, 0, sizeof(color_key));
    hr = IDirectDrawSurface4_GetColorKey(surface, DDCKEY_SRCBLT, &color_key);
    ok(SUCCEEDED(hr), "Failed to get color key, hr %#x.\n", hr);
    ok(color_key.dwColorSpaceLowValue == 0x0000ff00, "Got unexpected value 0x%08x.\n",
            color_key.dwColorSpaceLowValue);
    ok(color_key.dwColorSpaceHighValue == 0x0000ff00, "Got unexpected value 0x%08x.\n",
            color_key.dwColorSpaceHighValue);

    mipmap = surface;
    IDirectDrawSurface_AddRef(mipmap);
    for (i = 0; i < 7; ++i)
    {
        hr = IDirectDrawSurface4_GetAttachedSurface(mipmap, &caps, &tmp);
        ok(SUCCEEDED(hr), "Failed to get attached surface, i %u, hr %#x.\n", i, hr);

        hr = IDirectDrawSurface4_GetColorKey(tmp, DDCKEY_SRCBLT, &color_key);
        ok(hr == DDERR_NOCOLORKEY, "Got unexpected hr %#x, i %u.\n", hr, i);
        color_key.dwColorSpaceLowValue = 0x000000ff;
        color_key.dwColorSpaceHighValue = 0x000000ff;
        hr = IDirectDrawSurface4_SetColorKey(tmp, DDCKEY_SRCBLT, &color_key);
        ok(SUCCEEDED(hr), "Failed to set color key, hr %#x, i %u.\n", hr, i);
        memset(&color_key, 0, sizeof(color_key));
        hr = IDirectDrawSurface4_GetColorKey(tmp, DDCKEY_SRCBLT, &color_key);
        ok(SUCCEEDED(hr), "Failed to get color key, hr %#x, i %u.\n", hr, i);
        ok(color_key.dwColorSpaceLowValue == 0x000000ff, "Got unexpected value 0x%08x, i %u.\n",
                color_key.dwColorSpaceLowValue, i);
        ok(color_key.dwColorSpaceHighValue == 0x000000ff, "Got unexpected value 0x%08x, i %u.\n",
                color_key.dwColorSpaceHighValue, i);

        IDirectDrawSurface_Release(mipmap);
        mipmap = tmp;
    }

    memset(&color_key, 0, sizeof(color_key));
    hr = IDirectDrawSurface4_GetColorKey(surface, DDCKEY_SRCBLT, &color_key);
    ok(SUCCEEDED(hr), "Failed to get color key, hr %#x.\n", hr);
    ok(color_key.dwColorSpaceLowValue == 0x0000ff00, "Got unexpected value 0x%08x.\n",
            color_key.dwColorSpaceLowValue);
    ok(color_key.dwColorSpaceHighValue == 0x0000ff00, "Got unexpected value 0x%08x.\n",
            color_key.dwColorSpaceHighValue);

    hr = IDirectDrawSurface4_GetAttachedSurface(mipmap, &caps, &tmp);
    ok(hr == DDERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);
    IDirectDrawSurface_Release(mipmap);
    refcount = IDirectDrawSurface4_Release(surface);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;
    U5(surface_desc).dwBackBufferCount = 1;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_GetColorKey(surface, DDCKEY_SRCBLT, &color_key);
    ok(hr == DDERR_NOCOLORKEY, "Got unexpected hr %#x.\n", hr);
    color_key.dwColorSpaceLowValue = 0x0000ff00;
    color_key.dwColorSpaceHighValue = 0x0000ff00;
    hr = IDirectDrawSurface4_SetColorKey(surface, DDCKEY_SRCBLT, &color_key);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);
    memset(&color_key, 0, sizeof(color_key));
    hr = IDirectDrawSurface4_GetColorKey(surface, DDCKEY_SRCBLT, &color_key);
    ok(SUCCEEDED(hr), "Failed to get color key, hr %#x.\n", hr);
    ok(color_key.dwColorSpaceLowValue == 0x0000ff00, "Got unexpected value 0x%08x.\n",
            color_key.dwColorSpaceLowValue);
    ok(color_key.dwColorSpaceHighValue == 0x0000ff00, "Got unexpected value 0x%08x.\n",
            color_key.dwColorSpaceHighValue);

    hr = IDirectDrawSurface4_GetAttachedSurface(surface, &caps, &tmp);
    ok(SUCCEEDED(hr), "Failed to get attached surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_GetColorKey(tmp, DDCKEY_SRCBLT, &color_key);
    ok(hr == DDERR_NOCOLORKEY, "Got unexpected hr %#x, i %u.\n", hr, i);
    color_key.dwColorSpaceLowValue = 0x0000ff00;
    color_key.dwColorSpaceHighValue = 0x0000ff00;
    hr = IDirectDrawSurface4_SetColorKey(tmp, DDCKEY_SRCBLT, &color_key);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);
    memset(&color_key, 0, sizeof(color_key));
    hr = IDirectDrawSurface4_GetColorKey(tmp, DDCKEY_SRCBLT, &color_key);
    ok(SUCCEEDED(hr), "Failed to get color key, hr %#x.\n", hr);
    ok(color_key.dwColorSpaceLowValue == 0x0000ff00, "Got unexpected value 0x%08x.\n",
            color_key.dwColorSpaceLowValue);
    ok(color_key.dwColorSpaceHighValue == 0x0000ff00, "Got unexpected value 0x%08x.\n",
            color_key.dwColorSpaceHighValue);

    IDirectDrawSurface_Release(tmp);

    refcount = IDirectDrawSurface4_Release(surface);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    IDirectDraw4_Release(ddraw);
    refcount = IDirect3DDevice3_Release(device);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

struct qi_test
{
    REFIID iid;
    REFIID refcount_iid;
    HRESULT hr;
};

static void test_qi(const char *test_name, IUnknown *base_iface,
        REFIID refcount_iid, const struct qi_test *tests, UINT entry_count)
{
    ULONG refcount, expected_refcount;
    IUnknown *iface1, *iface2;
    HRESULT hr;
    UINT i, j;

    for (i = 0; i < entry_count; ++i)
    {
        hr = IUnknown_QueryInterface(base_iface, tests[i].iid, (void **)&iface1);
        ok(hr == tests[i].hr, "Got hr %#x for test \"%s\" %u.\n", hr, test_name, i);
        if (SUCCEEDED(hr))
        {
            for (j = 0; j < entry_count; ++j)
            {
                hr = IUnknown_QueryInterface(iface1, tests[j].iid, (void **)&iface2);
                ok(hr == tests[j].hr, "Got hr %#x for test \"%s\" %u, %u.\n", hr, test_name, i, j);
                if (SUCCEEDED(hr))
                {
                    expected_refcount = 0;
                    if (IsEqualGUID(refcount_iid, tests[j].refcount_iid))
                        ++expected_refcount;
                    if (IsEqualGUID(tests[i].refcount_iid, tests[j].refcount_iid))
                        ++expected_refcount;
                    refcount = IUnknown_Release(iface2);
                    ok(refcount == expected_refcount, "Got refcount %u for test \"%s\" %u, %u, expected %u.\n",
                            refcount, test_name, i, j, expected_refcount);
                }
            }

            expected_refcount = 0;
            if (IsEqualGUID(refcount_iid, tests[i].refcount_iid))
                ++expected_refcount;
            refcount = IUnknown_Release(iface1);
            ok(refcount == expected_refcount, "Got refcount %u for test \"%s\" %u, expected %u.\n",
                    refcount, test_name, i, expected_refcount);
        }
    }
}

static void test_surface_qi(void)
{
    static const struct qi_test tests[] =
    {
        {&IID_IDirect3DTexture2,        &IID_IDirectDrawSurface4,       S_OK         },
        {&IID_IDirect3DTexture,         &IID_IDirectDrawSurface4,       S_OK         },
        {&IID_IDirectDrawGammaControl,  &IID_IDirectDrawGammaControl,   S_OK         },
        {&IID_IDirectDrawColorControl,  NULL,                           E_NOINTERFACE},
        {&IID_IDirectDrawSurface7,      &IID_IDirectDrawSurface7,       S_OK         },
        {&IID_IDirectDrawSurface4,      &IID_IDirectDrawSurface4,       S_OK         },
        {&IID_IDirectDrawSurface3,      &IID_IDirectDrawSurface3,       S_OK         },
        {&IID_IDirectDrawSurface2,      &IID_IDirectDrawSurface2,       S_OK         },
        {&IID_IDirectDrawSurface,       &IID_IDirectDrawSurface,        S_OK         },
        {&IID_IDirect3DDevice7,         NULL,                           E_INVALIDARG },
        {&IID_IDirect3DDevice3,         NULL,                           E_INVALIDARG },
        {&IID_IDirect3DDevice2,         NULL,                           E_INVALIDARG },
        {&IID_IDirect3DDevice,          NULL,                           E_INVALIDARG },
        {&IID_IDirect3D7,               NULL,                           E_INVALIDARG },
        {&IID_IDirect3D3,               NULL,                           E_INVALIDARG },
        {&IID_IDirect3D2,               NULL,                           E_INVALIDARG },
        {&IID_IDirect3D,                NULL,                           E_INVALIDARG },
        {&IID_IDirectDraw7,             NULL,                           E_INVALIDARG },
        {&IID_IDirectDraw4,             NULL,                           E_INVALIDARG },
        {&IID_IDirectDraw3,             NULL,                           E_INVALIDARG },
        {&IID_IDirectDraw2,             NULL,                           E_INVALIDARG },
        {&IID_IDirectDraw,              NULL,                           E_INVALIDARG },
        {&IID_IDirect3DLight,           NULL,                           E_INVALIDARG },
        {&IID_IDirect3DMaterial,        NULL,                           E_INVALIDARG },
        {&IID_IDirect3DMaterial2,       NULL,                           E_INVALIDARG },
        {&IID_IDirect3DMaterial3,       NULL,                           E_INVALIDARG },
        {&IID_IDirect3DExecuteBuffer,   NULL,                           E_INVALIDARG },
        {&IID_IDirect3DViewport,        NULL,                           E_INVALIDARG },
        {&IID_IDirect3DViewport2,       NULL,                           E_INVALIDARG },
        {&IID_IDirect3DViewport3,       NULL,                           E_INVALIDARG },
        {&IID_IDirect3DVertexBuffer,    NULL,                           E_INVALIDARG },
        {&IID_IDirect3DVertexBuffer7,   NULL,                           E_INVALIDARG },
        {&IID_IDirectDrawPalette,       NULL,                           E_INVALIDARG },
        {&IID_IDirectDrawClipper,       NULL,                           E_INVALIDARG },
        {&IID_IUnknown,                 &IID_IDirectDrawSurface,        S_OK         },
        {NULL,                          NULL,                           E_INVALIDARG },
    };

    IDirectDrawSurface4 *surface;
    DDSURFACEDESC2 surface_desc;
    IDirect3DDevice3 *device;
    IDirectDraw4 *ddraw;
    HWND window;
    HRESULT hr;

    if (!GetProcAddress(GetModuleHandleA("ddraw.dll"), "DirectDrawCreateEx"))
    {
        win_skip("DirectDrawCreateEx not available, skipping test.\n");
        return;
    }

    window = create_window();
    /* Try to create a D3D device to see if the ddraw implementation supports
     * D3D. 64-bit ddraw in particular doesn't seem to support D3D, and
     * doesn't support e.g. the IDirect3DTexture interfaces. */
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }
    IDirect3DDevice_Release(device);
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    surface_desc.dwWidth = 512;
    surface_desc.dwHeight = 512;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, (IDirectDrawSurface4 **)0xdeadbeef, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    test_qi("surface_qi", (IUnknown *)surface, &IID_IDirectDrawSurface4, tests, ARRAY_SIZE(tests));

    IDirectDrawSurface4_Release(surface);
    IDirectDraw4_Release(ddraw);
    DestroyWindow(window);
}

static void test_device_qi(void)
{
    static const struct qi_test tests[] =
    {
        {&IID_IDirect3DTexture2,        NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DTexture,         NULL,                           E_NOINTERFACE},
        {&IID_IDirectDrawGammaControl,  NULL,                           E_NOINTERFACE},
        {&IID_IDirectDrawColorControl,  NULL,                           E_NOINTERFACE},
        {&IID_IDirectDrawSurface7,      NULL,                           E_NOINTERFACE},
        {&IID_IDirectDrawSurface4,      NULL,                           E_NOINTERFACE},
        {&IID_IDirectDrawSurface3,      NULL,                           E_NOINTERFACE},
        {&IID_IDirectDrawSurface2,      NULL,                           E_NOINTERFACE},
        {&IID_IDirectDrawSurface,       NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DDevice7,         NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DDevice3,         &IID_IDirect3DDevice3,          S_OK         },
        {&IID_IDirect3DDevice2,         &IID_IDirect3DDevice3,          S_OK         },
        {&IID_IDirect3DDevice,          &IID_IDirect3DDevice3,          S_OK         },
        {&IID_IDirect3DRampDevice,      NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DRGBDevice,       NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DHALDevice,       NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DMMXDevice,       NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DRefDevice,       NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DTnLHalDevice,    NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DNullDevice,      NULL,                           E_NOINTERFACE},
        {&IID_IDirect3D7,               NULL,                           E_NOINTERFACE},
        {&IID_IDirect3D3,               NULL,                           E_NOINTERFACE},
        {&IID_IDirect3D2,               NULL,                           E_NOINTERFACE},
        {&IID_IDirect3D,                NULL,                           E_NOINTERFACE},
        {&IID_IDirectDraw7,             NULL,                           E_NOINTERFACE},
        {&IID_IDirectDraw4,             NULL,                           E_NOINTERFACE},
        {&IID_IDirectDraw3,             NULL,                           E_NOINTERFACE},
        {&IID_IDirectDraw2,             NULL,                           E_NOINTERFACE},
        {&IID_IDirectDraw,              NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DLight,           NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DMaterial,        NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DMaterial2,       NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DMaterial3,       NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DExecuteBuffer,   NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DViewport,        NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DViewport2,       NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DViewport3,       NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DVertexBuffer,    NULL,                           E_NOINTERFACE},
        {&IID_IDirect3DVertexBuffer7,   NULL,                           E_NOINTERFACE},
        {&IID_IDirectDrawPalette,       NULL,                           E_NOINTERFACE},
        {&IID_IDirectDrawClipper,       NULL,                           E_NOINTERFACE},
        {&IID_IUnknown,                 &IID_IDirect3DDevice3,          S_OK         },
    };

    IDirect3DDevice3 *device;
    HWND window;

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    test_qi("device_qi", (IUnknown *)device, &IID_IDirect3DDevice3, tests, ARRAY_SIZE(tests));

    IDirect3DDevice3_Release(device);
    DestroyWindow(window);
}

static void test_wndproc(void)
{
    LONG_PTR proc, ddraw_proc;
    IDirectDraw4 *ddraw;
    WNDCLASSA wc = {0};
    HWND window;
    HRESULT hr;
    ULONG ref;

    static struct message messages[] =
    {
        {WM_WINDOWPOSCHANGING,  FALSE,  0},
        {WM_MOVE,               FALSE,  0},
        {WM_SIZE,               FALSE,  0},
        {WM_WINDOWPOSCHANGING,  FALSE,  0},
        {WM_ACTIVATE,           FALSE,  0},
        {WM_SETFOCUS,           FALSE,  0},
        {0,                     FALSE,  0},
    };

    /* DDSCL_EXCLUSIVE replaces the window's window proc. */
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    wc.lpfnWndProc = test_proc;
    wc.lpszClassName = "ddraw_test_wndproc_wc";
    ok(RegisterClassA(&wc), "Failed to register window class.\n");

    window = CreateWindowA("ddraw_test_wndproc_wc", "ddraw_test",
            WS_MAXIMIZE | WS_CAPTION , 0, 0, 640, 480, 0, 0, 0, 0);

    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    expect_messages = messages;
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    ok(!expect_messages->message, "Expected message %#x, but didn't receive it.\n", expect_messages->message);
    expect_messages = NULL;
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    ref = IDirectDraw4_Release(ddraw);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    /* DDSCL_NORMAL doesn't. */
    ddraw = create_ddraw();
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    ref = IDirectDraw4_Release(ddraw);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    /* The original window proc is only restored by ddraw if the current
     * window proc matches the one ddraw set. This also affects switching
     * from DDSCL_NORMAL to DDSCL_EXCLUSIVE. */
    ddraw = create_ddraw();
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    ddraw_proc = proc;
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    proc = SetWindowLongPtrA(window, GWLP_WNDPROC, (LONG_PTR)DefWindowProcA);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)DefWindowProcA, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)DefWindowProcA, proc);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    proc = SetWindowLongPtrA(window, GWLP_WNDPROC, (LONG_PTR)ddraw_proc);
    ok(proc == (LONG_PTR)DefWindowProcA, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)DefWindowProcA, proc);
    ref = IDirectDraw4_Release(ddraw);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    ddraw = create_ddraw();
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    proc = SetWindowLongPtrA(window, GWLP_WNDPROC, (LONG_PTR)DefWindowProcA);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    ref = IDirectDraw4_Release(ddraw);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)DefWindowProcA, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)DefWindowProcA, proc);

    fix_wndproc(window, (LONG_PTR)test_proc);
    expect_messages = NULL;
    DestroyWindow(window);
    UnregisterClassA("ddraw_test_wndproc_wc", GetModuleHandleA(NULL));
}

static void test_window_style(void)
{
    LONG style, exstyle, tmp, expected_style;
    RECT fullscreen_rect, r;
    IDirectDraw4 *ddraw;
    HWND window;
    HRESULT hr;
    ULONG ref;
    BOOL ret;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 100, 100, 0, 0, 0, 0);
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    style = GetWindowLongA(window, GWL_STYLE);
    exstyle = GetWindowLongA(window, GWL_EXSTYLE);
    SetRect(&fullscreen_rect, 0, 0, registry_mode.dmPelsWidth, registry_mode.dmPelsHeight);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    tmp = GetWindowLongA(window, GWL_STYLE);
    todo_wine ok(tmp == style, "Expected window style %#x, got %#x.\n", style, tmp);
    tmp = GetWindowLongA(window, GWL_EXSTYLE);
    todo_wine ok(tmp == exstyle, "Expected window extended style %#x, got %#x.\n", exstyle, tmp);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &fullscreen_rect), "Expected %s, got %s.\n",
            wine_dbgstr_rect(&fullscreen_rect), wine_dbgstr_rect(&r));
    GetClientRect(window, &r);
    todo_wine ok(!EqualRect(&r, &fullscreen_rect), "Client rect and window rect are equal.\n");

    ret = SetForegroundWindow(GetDesktopWindow());
    ok(ret, "Failed to set foreground window.\n");

    tmp = GetWindowLongA(window, GWL_STYLE);
    todo_wine ok(tmp == style, "Expected window style %#x, got %#x.\n", style, tmp);
    tmp = GetWindowLongA(window, GWL_EXSTYLE);
    todo_wine ok(tmp == exstyle, "Expected window extended style %#x, got %#x.\n", exstyle, tmp);

    ret = SetForegroundWindow(window);
    ok(ret, "Failed to set foreground window.\n");
    /* Windows 7 (but not Vista and XP) shows the window when it receives focus. Hide it again,
     * the next tests expect this. */
    ShowWindow(window, SW_HIDE);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    tmp = GetWindowLongA(window, GWL_STYLE);
    todo_wine ok(tmp == style, "Expected window style %#x, got %#x.\n", style, tmp);
    tmp = GetWindowLongA(window, GWL_EXSTYLE);
    todo_wine ok(tmp == exstyle, "Expected window extended style %#x, got %#x.\n", exstyle, tmp);

    ShowWindow(window, SW_SHOW);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    tmp = GetWindowLongA(window, GWL_STYLE);
    expected_style = style | WS_VISIBLE;
    todo_wine ok(tmp == expected_style, "Expected window style %#x, got %#x.\n", expected_style, tmp);
    tmp = GetWindowLongA(window, GWL_EXSTYLE);
    expected_style = exstyle | WS_EX_TOPMOST;
    todo_wine ok(tmp == expected_style, "Expected window extended style %#x, got %#x.\n", expected_style, tmp);

    ret = SetForegroundWindow(GetDesktopWindow());
    ok(ret, "Failed to set foreground window.\n");
    tmp = GetWindowLongA(window, GWL_STYLE);
    expected_style = style | WS_VISIBLE | WS_MINIMIZE;
    todo_wine ok(tmp == expected_style, "Expected window style %#x, got %#x.\n", expected_style, tmp);
    tmp = GetWindowLongA(window, GWL_EXSTYLE);
    expected_style = exstyle | WS_EX_TOPMOST;
    todo_wine ok(tmp == expected_style, "Expected window extended style %#x, got %#x.\n", expected_style, tmp);

    ref = IDirectDraw4_Release(ddraw);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);

    DestroyWindow(window);
}

static void test_redundant_mode_set(void)
{
    DDSURFACEDESC2 surface_desc = {0};
    IDirectDraw4 *ddraw;
    RECT q, r, s;
    HWND window;
    HRESULT hr;
    ULONG ref;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 100, 100, 0, 0, 0, 0);
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDraw4_GetDisplayMode(ddraw, &surface_desc);
    ok(SUCCEEDED(hr), "GetDisplayMode failed, hr %#x.\n", hr);

    hr = IDirectDraw4_SetDisplayMode(ddraw, surface_desc.dwWidth, surface_desc.dwHeight,
            U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount, 0, 0);
    ok(SUCCEEDED(hr), "SetDisplayMode failed, hr %#x.\n", hr);

    GetWindowRect(window, &q);
    r = q;
    r.right /= 2;
    r.bottom /= 2;
    SetWindowPos(window, HWND_TOP, r.left, r.top, r.right, r.bottom, 0);
    GetWindowRect(window, &s);
    ok(EqualRect(&r, &s), "Expected %s, got %s.\n", wine_dbgstr_rect(&r), wine_dbgstr_rect(&s));

    hr = IDirectDraw4_SetDisplayMode(ddraw, surface_desc.dwWidth, surface_desc.dwHeight,
            U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount, 0, 0);
    ok(SUCCEEDED(hr), "SetDisplayMode failed, hr %#x.\n", hr);

    GetWindowRect(window, &s);
    ok(EqualRect(&r, &s) || broken(EqualRect(&q, &s) /* Windows 10 */),
            "Expected %s, got %s.\n", wine_dbgstr_rect(&r), wine_dbgstr_rect(&s));

    ref = IDirectDraw4_Release(ddraw);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);

    DestroyWindow(window);
}

static SIZE screen_size, screen_size2;

static LRESULT CALLBACK mode_set_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (message == WM_SIZE)
    {
        screen_size.cx = GetSystemMetrics(SM_CXSCREEN);
        screen_size.cy = GetSystemMetrics(SM_CYSCREEN);
    }

    return test_proc(hwnd, message, wparam, lparam);
}

static LRESULT CALLBACK mode_set_proc2(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (message == WM_SIZE)
    {
        screen_size2.cx = GetSystemMetrics(SM_CXSCREEN);
        screen_size2.cy = GetSystemMetrics(SM_CYSCREEN);
    }

    return test_proc(hwnd, message, wparam, lparam);
}

struct test_coop_level_mode_set_enum_param
{
    DWORD ddraw_width, ddraw_height, user32_width, user32_height;
};

static HRESULT CALLBACK test_coop_level_mode_set_enum_cb(DDSURFACEDESC2 *surface_desc, void *context)
{
    struct test_coop_level_mode_set_enum_param *param = context;

    if (U1(U4(*surface_desc).ddpfPixelFormat).dwRGBBitCount != registry_mode.dmBitsPerPel)
        return DDENUMRET_OK;
    if (surface_desc->dwWidth == registry_mode.dmPelsWidth
            && surface_desc->dwHeight == registry_mode.dmPelsHeight)
        return DDENUMRET_OK;

    if (!param->ddraw_width)
    {
        param->ddraw_width = surface_desc->dwWidth;
        param->ddraw_height = surface_desc->dwHeight;
        return DDENUMRET_OK;
    }
    if (surface_desc->dwWidth == param->ddraw_width && surface_desc->dwHeight == param->ddraw_height)
        return DDENUMRET_OK;

    param->user32_width = surface_desc->dwWidth;
    param->user32_height = surface_desc->dwHeight;
    return DDENUMRET_CANCEL;
}

static void test_coop_level_mode_set(void)
{
    IDirectDrawSurface4 *primary;
    RECT registry_rect, ddraw_rect, user32_rect, r;
    IDirectDraw4 *ddraw;
    DDSURFACEDESC2 ddsd;
    WNDCLASSA wc = {0};
    HWND window, window2;
    HRESULT hr;
    ULONG ref;
    MSG msg;
    struct test_coop_level_mode_set_enum_param param;
    DEVMODEW devmode;
    BOOL ret;
    LONG change_ret;

    static const struct message exclusive_messages[] =
    {
        {WM_WINDOWPOSCHANGING,  FALSE,  0},
        {WM_WINDOWPOSCHANGED,   FALSE,  0},
        {WM_SIZE,               FALSE,  0},
        {WM_DISPLAYCHANGE,      FALSE,  0},
        {0,                     FALSE,  0},
    };
    static const struct message exclusive_focus_loss_messages[] =
    {
        {WM_ACTIVATE,           TRUE,   WA_INACTIVE},
        {WM_DISPLAYCHANGE,      FALSE,  0},
        {WM_WINDOWPOSCHANGING,  FALSE,  0},
        /* Like d3d8 and d3d9 ddraw seems to use SW_SHOWMINIMIZED instead of
         * SW_MINIMIZED, causing a recursive window activation that does not
         * produce the same result in Wine yet. Ignore the difference for now.
         * {WM_ACTIVATE,           TRUE,   0x200000 | WA_ACTIVE}, */
        {WM_WINDOWPOSCHANGED,   FALSE,  0},
        {WM_MOVE,               FALSE,  0},
        {WM_SIZE,               TRUE,   SIZE_MINIMIZED},
        {WM_ACTIVATEAPP,        TRUE,   FALSE},
        {0,                     FALSE,  0},
    };
    static const struct message exclusive_focus_restore_messages[] =
    {
        {WM_WINDOWPOSCHANGING,  FALSE,  0}, /* From the ShowWindow(SW_RESTORE). */
        {WM_WINDOWPOSCHANGING,  FALSE,  0}, /* Generated by ddraw, matches d3d9 behavior. */
        {WM_WINDOWPOSCHANGED,   FALSE,  0}, /* Matching previous message. */
        {WM_SIZE,               FALSE,  0}, /* DefWindowProc. */
        {WM_DISPLAYCHANGE,      FALSE,  0}, /* Ddraw restores mode. */
        /* Native redundantly sets the window size here. */
        {WM_ACTIVATEAPP,        TRUE,   TRUE}, /* End of ddraw's hooks. */
        {WM_WINDOWPOSCHANGED,   FALSE,  0}, /* Matching the one from ShowWindow. */
        {WM_MOVE,               FALSE,  0}, /* DefWindowProc. */
        {WM_SIZE,               TRUE,   SIZE_RESTORED}, /* DefWindowProc. */
        {0,                     FALSE,  0},
    };
    static const struct message sc_restore_messages[] =
    {
        {WM_SYSCOMMAND,         TRUE,   SC_RESTORE},
        {WM_WINDOWPOSCHANGING,  FALSE,  0},
        {WM_WINDOWPOSCHANGED,   FALSE,  0},
        {WM_SIZE,               TRUE,   SIZE_RESTORED},
        {0,                     FALSE,  0},
    };
    static const struct message sc_minimize_messages[] =
    {
        {WM_SYSCOMMAND,         TRUE,   SC_MINIMIZE},
        {WM_WINDOWPOSCHANGING,  FALSE,  0},
        {WM_WINDOWPOSCHANGED,   FALSE,  0},
        {WM_SIZE,               TRUE,   SIZE_MINIMIZED},
        {0,                     FALSE,  0},
    };
    static const struct message sc_maximize_messages[] =
    {
        {WM_SYSCOMMAND,         TRUE,   SC_MAXIMIZE},
        {WM_WINDOWPOSCHANGING,  FALSE,  0},
        {WM_WINDOWPOSCHANGED,   FALSE,  0},
        {WM_SIZE,               TRUE,   SIZE_MAXIMIZED},
        {0,                     FALSE,  0},
    };

    static const struct message normal_messages[] =
    {
        {WM_DISPLAYCHANGE,      FALSE,  0},
        {0,                     FALSE,  0},
    };

    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    memset(&param, 0, sizeof(param));
    hr = IDirectDraw4_EnumDisplayModes(ddraw, 0, NULL, &param, test_coop_level_mode_set_enum_cb);
    ok(SUCCEEDED(hr), "Failed to enumerate display mode, hr %#x.\n", hr);
    ref = IDirectDraw4_Release(ddraw);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);

    if (!param.user32_height)
    {
        skip("Fewer than 3 different modes supported, skipping mode restore test.\n");
        return;
    }

    SetRect(&registry_rect, 0, 0, registry_mode.dmPelsWidth, registry_mode.dmPelsHeight);
    SetRect(&ddraw_rect, 0, 0, param.ddraw_width, param.ddraw_height);
    SetRect(&user32_rect, 0, 0, param.user32_width, param.user32_height);

    memset(&devmode, 0, sizeof(devmode));
    devmode.dmSize = sizeof(devmode);
    devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
    devmode.dmPelsWidth = param.user32_width;
    devmode.dmPelsHeight = param.user32_height;
    change_ret = ChangeDisplaySettingsW(&devmode, CDS_FULLSCREEN);
    ok(change_ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#x.\n", change_ret);

    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    wc.lpfnWndProc = mode_set_proc;
    wc.lpszClassName = "ddraw_test_wndproc_wc";
    ok(RegisterClassA(&wc), "Failed to register window class.\n");
    wc.lpfnWndProc = mode_set_proc2;
    wc.lpszClassName = "ddraw_test_wndproc_wc2";
    ok(RegisterClassA(&wc), "Failed to register window class.\n");

    window = CreateWindowA("ddraw_test_wndproc_wc", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 100, 100, 0, 0, 0, 0);
    window2 = CreateWindowA("ddraw_test_wndproc_wc2", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 100, 100, 0, 0, 0, 0);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &user32_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&user32_rect),
            wine_dbgstr_rect(&r));

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == param.user32_width, "Expected surface width %u, got %u.\n",
            param.user32_width, ddsd.dwWidth);
    ok(ddsd.dwHeight == param.user32_height, "Expected surface height %u, got %u.\n",
            param.user32_height, ddsd.dwHeight);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &user32_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&user32_rect),
            wine_dbgstr_rect(&r));

    PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE);
    expect_messages = exclusive_messages;
    screen_size.cx = 0;
    screen_size.cy = 0;

    hr = IDirectDrawSurface4_IsLost(primary);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = set_display_mode(ddraw, param.ddraw_width, param.ddraw_height);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_IsLost(primary);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);

    ok(!expect_messages->message, "Expected message %#x, but didn't receive it.\n", expect_messages->message);
    expect_messages = NULL;
    ok(screen_size.cx == param.ddraw_width && screen_size.cy == param.ddraw_height,
            "Expected screen size %ux%u, got %ux%u.\n",
            param.ddraw_width, param.ddraw_height, screen_size.cx, screen_size.cy);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &ddraw_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&ddraw_rect),
            wine_dbgstr_rect(&r));

    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == param.user32_width, "Expected surface width %u, got %u.\n",
            param.user32_width, ddsd.dwWidth);
    ok(ddsd.dwHeight == param.user32_height, "Expected surface height %u, got %u.\n",
            param.user32_height, ddsd.dwHeight);
    IDirectDrawSurface4_Release(primary);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == param.ddraw_width, "Expected surface width %u, got %u.\n",
            param.ddraw_width, ddsd.dwWidth);
    ok(ddsd.dwHeight == param.ddraw_height, "Expected surface height %u, got %u.\n",
            param.ddraw_height, ddsd.dwHeight);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &ddraw_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&ddraw_rect),
            wine_dbgstr_rect(&r));

    PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE);
    expect_messages = exclusive_messages;
    screen_size.cx = 0;
    screen_size.cy = 0;

    hr = IDirectDrawSurface4_IsLost(primary);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    change_ret = ChangeDisplaySettingsW(&devmode, CDS_FULLSCREEN);
    ok(change_ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#x.\n", change_ret);
    hr = IDirectDrawSurface4_IsLost(primary);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);

    ok(!expect_messages->message, "Expected message %#x, but didn't receive it.\n", expect_messages->message);
    expect_messages = NULL;
    ok(screen_size.cx == param.user32_width && screen_size.cy == param.user32_height,
            "Expected screen size %ux%u, got %ux%u.\n",
            param.user32_width, param.user32_height, screen_size.cx, screen_size.cy);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &user32_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&user32_rect),
            wine_dbgstr_rect(&r));

    expect_messages = exclusive_focus_loss_messages;
    ret = SetForegroundWindow(GetDesktopWindow());
    ok(ret, "Failed to set foreground window.\n");
    ok(!expect_messages->message, "Expected message %#x, but didn't receive it.\n", expect_messages->message);
    memset(&devmode, 0, sizeof(devmode));
    devmode.dmSize = sizeof(devmode);
    ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devmode);
    ok(ret, "Failed to get display mode.\n");
    ok(devmode.dmPelsWidth == registry_mode.dmPelsWidth
            && devmode.dmPelsHeight == registry_mode.dmPelsHeight, "Got unexpect screen size %ux%u.\n",
            devmode.dmPelsWidth, devmode.dmPelsHeight);

    expect_messages = exclusive_focus_restore_messages;
    ShowWindow(window, SW_RESTORE);
    ok(!expect_messages->message, "Expected message %#x, but didn't receive it.\n", expect_messages->message);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &ddraw_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&ddraw_rect),
            wine_dbgstr_rect(&r));
    ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devmode);
    ok(ret, "Failed to get display mode.\n");
    ok(devmode.dmPelsWidth == param.ddraw_width
            && devmode.dmPelsHeight == param.ddraw_height, "Got unexpect screen size %ux%u.\n",
            devmode.dmPelsWidth, devmode.dmPelsHeight);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    /* Normally the primary should be restored here. Unfortunately this causes the
     * GetSurfaceDesc call after the next display mode change to crash on the Windows 8
     * testbot. Another Restore call would presumably avoid the crash, but it also moots
     * the point of the GetSurfaceDesc call. */

    expect_messages = sc_minimize_messages;
    SendMessageA(window, WM_SYSCOMMAND, SC_MINIMIZE, 0);
    ok(!expect_messages->message, "Expected message %#x, but didn't receive it.\n", expect_messages->message);
    expect_messages = NULL;

    expect_messages = sc_restore_messages;
    SendMessageA(window, WM_SYSCOMMAND, SC_RESTORE, 0);
    ok(!expect_messages->message, "Expected message %#x, but didn't receive it.\n", expect_messages->message);
    expect_messages = NULL;

    expect_messages = sc_maximize_messages;
    SendMessageA(window, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
    ok(!expect_messages->message, "Expected message %#x, but didn't receive it.\n", expect_messages->message);
    expect_messages = NULL;

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE);
    expect_messages = exclusive_messages;
    screen_size.cx = 0;
    screen_size.cy = 0;

    hr = IDirectDrawSurface4_IsLost(primary);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDraw4_RestoreDisplayMode(ddraw);
    ok(SUCCEEDED(hr), "RestoreDisplayMode failed, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_IsLost(primary);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);

    ok(!expect_messages->message, "Expected message %#x, but didn't receive it.\n", expect_messages->message);
    expect_messages = NULL;
    ok(screen_size.cx == registry_mode.dmPelsWidth
            && screen_size.cy == registry_mode.dmPelsHeight,
            "Expected screen size %ux%u, got %ux%u.\n",
            registry_mode.dmPelsWidth, registry_mode.dmPelsHeight, screen_size.cx, screen_size.cy);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &registry_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&registry_rect),
            wine_dbgstr_rect(&r));

    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == param.ddraw_width, "Expected surface width %u, got %u.\n",
            param.ddraw_width, ddsd.dwWidth);
    ok(ddsd.dwHeight == param.ddraw_height, "Expected surface height %u, got %u.\n",
            param.ddraw_height, ddsd.dwHeight);
    IDirectDrawSurface4_Release(primary);

    /* For Wine. */
    change_ret = ChangeDisplaySettingsW(NULL, CDS_FULLSCREEN);
    ok(change_ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#x.\n", change_ret);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == registry_mode.dmPelsWidth, "Expected surface width %u, got %u.\n",
            registry_mode.dmPelsWidth, ddsd.dwWidth);
    ok(ddsd.dwHeight == registry_mode.dmPelsHeight, "Expected surface height %u, got %u.\n",
            registry_mode.dmPelsHeight, ddsd.dwHeight);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &registry_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&registry_rect),
            wine_dbgstr_rect(&r));

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &registry_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&registry_rect),
            wine_dbgstr_rect(&r));

    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == registry_mode.dmPelsWidth, "Expected surface width %u, got %u.\n",
            registry_mode.dmPelsWidth, ddsd.dwWidth);
    ok(ddsd.dwHeight == registry_mode.dmPelsHeight, "Expected surface height %u, got %u.\n",
            registry_mode.dmPelsHeight, ddsd.dwHeight);
    IDirectDrawSurface4_Release(primary);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == registry_mode.dmPelsWidth, "Expected surface width %u, got %u.\n",
            registry_mode.dmPelsWidth, ddsd.dwWidth);
    ok(ddsd.dwHeight == registry_mode.dmPelsHeight, "Expected surface height %u, got %u.\n",
            registry_mode.dmPelsHeight, ddsd.dwHeight);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &registry_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&registry_rect),
            wine_dbgstr_rect(&r));

    PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE);
    expect_messages = normal_messages;
    screen_size.cx = 0;
    screen_size.cy = 0;

    hr = IDirectDrawSurface4_IsLost(primary);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
    devmode.dmPelsWidth = param.user32_width;
    devmode.dmPelsHeight = param.user32_height;
    change_ret = ChangeDisplaySettingsW(&devmode, CDS_FULLSCREEN);
    ok(change_ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#x.\n", change_ret);
    hr = IDirectDrawSurface4_IsLost(primary);
    todo_wine ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);

    ok(!expect_messages->message, "Expected message %#x, but didn't receive it.\n", expect_messages->message);
    expect_messages = NULL;
    ok(!screen_size.cx && !screen_size.cy, "Got unexpected screen size %ux%u.\n", screen_size.cx, screen_size.cy);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &registry_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&registry_rect),
            wine_dbgstr_rect(&r));

    PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE);
    expect_messages = normal_messages;
    screen_size.cx = 0;
    screen_size.cy = 0;

    hr = IDirectDrawSurface4_Restore(primary);
    ok(hr == DDERR_WRONGMODE, "Got unexpected hr %#x.\n", hr);
    hr = set_display_mode(ddraw, param.ddraw_width, param.ddraw_height);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Restore(primary);
    ok(hr == DDERR_WRONGMODE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_IsLost(primary);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);

    ok(!expect_messages->message, "Expected message %#x, but didn't receive it.\n", expect_messages->message);
    expect_messages = NULL;
    ok(!screen_size.cx && !screen_size.cy, "Got unexpected screen size %ux%u.\n", screen_size.cx, screen_size.cy);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &registry_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&registry_rect),
            wine_dbgstr_rect(&r));

    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == registry_mode.dmPelsWidth, "Expected surface width %u, got %u.\n",
            registry_mode.dmPelsWidth, ddsd.dwWidth);
    ok(ddsd.dwHeight == registry_mode.dmPelsHeight, "Expected surface height %u, got %u.\n",
            registry_mode.dmPelsHeight, ddsd.dwHeight);
    IDirectDrawSurface4_Release(primary);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == param.ddraw_width, "Expected surface width %u, got %u.\n",
            param.ddraw_width, ddsd.dwWidth);
    ok(ddsd.dwHeight == param.ddraw_height, "Expected surface height %u, got %u.\n",
            param.ddraw_height, ddsd.dwHeight);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &registry_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&registry_rect),
            wine_dbgstr_rect(&r));

    PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE);
    expect_messages = normal_messages;
    screen_size.cx = 0;
    screen_size.cy = 0;

    hr = IDirectDrawSurface4_IsLost(primary);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDraw4_RestoreDisplayMode(ddraw);
    ok(SUCCEEDED(hr), "RestoreDisplayMode failed, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_IsLost(primary);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);

    ok(!expect_messages->message, "Expected message %#x, but didn't receive it.\n", expect_messages->message);
    expect_messages = NULL;
    ok(!screen_size.cx && !screen_size.cy, "Got unexpected screen size %ux%u.\n", screen_size.cx, screen_size.cy);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &registry_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&registry_rect),
            wine_dbgstr_rect(&r));

    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == param.ddraw_width, "Expected surface width %u, got %u.\n",
            param.ddraw_width, ddsd.dwWidth);
    ok(ddsd.dwHeight == param.ddraw_height, "Expected surface height %u, got %u.\n",
            param.ddraw_height, ddsd.dwHeight);
    IDirectDrawSurface4_Release(primary);

    ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devmode);
    ok(ret, "Failed to get display mode.\n");
    ok(devmode.dmPelsWidth == registry_mode.dmPelsWidth
            && devmode.dmPelsHeight == registry_mode.dmPelsHeight,
            "Expected resolution %ux%u, got %ux%u.\n",
            registry_mode.dmPelsWidth, registry_mode.dmPelsHeight,
            devmode.dmPelsWidth, devmode.dmPelsHeight);
    change_ret = ChangeDisplaySettingsW(NULL, CDS_FULLSCREEN);
    ok(change_ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#x.\n", change_ret);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == registry_mode.dmPelsWidth, "Expected surface width %u, got %u.\n",
            registry_mode.dmPelsWidth, ddsd.dwWidth);
    ok(ddsd.dwHeight == registry_mode.dmPelsHeight, "Expected surface height %u, got %u.\n",
            registry_mode.dmPelsHeight, ddsd.dwHeight);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &registry_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&registry_rect),
            wine_dbgstr_rect(&r));

    /* DDSCL_NORMAL | DDSCL_FULLSCREEN behaves the same as just DDSCL_NORMAL.
     * Resizing the window on mode changes is a property of DDSCL_EXCLUSIVE,
     * not DDSCL_FULLSCREEN. */
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &registry_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&registry_rect),
            wine_dbgstr_rect(&r));

    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == registry_mode.dmPelsWidth, "Expected surface width %u, got %u.\n",
            registry_mode.dmPelsWidth, ddsd.dwWidth);
    ok(ddsd.dwHeight == registry_mode.dmPelsHeight, "Expected surface height %u, got %u.\n",
            registry_mode.dmPelsHeight, ddsd.dwHeight);
    IDirectDrawSurface4_Release(primary);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == registry_mode.dmPelsWidth, "Expected surface width %u, got %u.\n",
            registry_mode.dmPelsWidth, ddsd.dwWidth);
    ok(ddsd.dwHeight == registry_mode.dmPelsHeight, "Expected surface height %u, got %u.\n",
            registry_mode.dmPelsHeight, ddsd.dwHeight);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &registry_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&registry_rect),
            wine_dbgstr_rect(&r));

    PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE);
    expect_messages = normal_messages;
    screen_size.cx = 0;
    screen_size.cy = 0;

    hr = IDirectDrawSurface4_IsLost(primary);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
    devmode.dmPelsWidth = param.user32_width;
    devmode.dmPelsHeight = param.user32_height;
    change_ret = ChangeDisplaySettingsW(&devmode, CDS_FULLSCREEN);
    ok(change_ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#x.\n", change_ret);
    hr = IDirectDrawSurface4_IsLost(primary);
    todo_wine ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);

    ok(!expect_messages->message, "Expected message %#x, but didn't receive it.\n", expect_messages->message);
    expect_messages = NULL;
    ok(!screen_size.cx && !screen_size.cy, "Got unexpected screen size %ux%u.\n", screen_size.cx, screen_size.cy);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &registry_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&registry_rect),
            wine_dbgstr_rect(&r));

    PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE);
    expect_messages = normal_messages;
    screen_size.cx = 0;
    screen_size.cy = 0;

    hr = IDirectDrawSurface4_Restore(primary);
    ok(hr == DDERR_WRONGMODE, "Got unexpected hr %#x.\n", hr);
    hr = set_display_mode(ddraw, param.ddraw_width, param.ddraw_height);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Restore(primary);
    ok(hr == DDERR_WRONGMODE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_IsLost(primary);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);

    ok(!expect_messages->message, "Expected message %#x, but didn't receive it.\n", expect_messages->message);
    expect_messages = NULL;
    ok(!screen_size.cx && !screen_size.cy, "Got unexpected screen size %ux%u.\n", screen_size.cx, screen_size.cy);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &registry_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&registry_rect),
            wine_dbgstr_rect(&r));

    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == registry_mode.dmPelsWidth, "Expected surface width %u, got %u.\n",
            registry_mode.dmPelsWidth, ddsd.dwWidth);
    ok(ddsd.dwHeight == registry_mode.dmPelsHeight, "Expected surface height %u, got %u.\n",
            registry_mode.dmPelsHeight, ddsd.dwHeight);
    IDirectDrawSurface4_Release(primary);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == param.ddraw_width, "Expected surface width %u, got %u.\n",
            param.ddraw_width, ddsd.dwWidth);
    ok(ddsd.dwHeight == param.ddraw_height, "Expected surface height %u, got %u.\n",
            param.ddraw_height, ddsd.dwHeight);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &registry_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&registry_rect),
            wine_dbgstr_rect(&r));

    PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE);
    expect_messages = normal_messages;
    screen_size.cx = 0;
    screen_size.cy = 0;

    hr = IDirectDrawSurface4_IsLost(primary);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDraw4_RestoreDisplayMode(ddraw);
    ok(SUCCEEDED(hr), "RestoreDisplayMode failed, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_IsLost(primary);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);

    ok(!expect_messages->message, "Expected message %#x, but didn't receive it.\n", expect_messages->message);
    expect_messages = NULL;
    ok(!screen_size.cx && !screen_size.cy, "Got unexpected screen size %ux%u.\n", screen_size.cx, screen_size.cy);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &registry_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&registry_rect),
            wine_dbgstr_rect(&r));

    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == param.ddraw_width, "Expected surface width %u, got %u.\n",
            param.ddraw_width, ddsd.dwWidth);
    ok(ddsd.dwHeight == param.ddraw_height, "Expected surface height %u, got %u.\n",
            param.ddraw_height, ddsd.dwHeight);
    IDirectDrawSurface4_Release(primary);

    ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devmode);
    ok(ret, "Failed to get display mode.\n");
    ok(devmode.dmPelsWidth == registry_mode.dmPelsWidth
            && devmode.dmPelsHeight == registry_mode.dmPelsHeight,
            "Expected resolution %ux%u, got %ux%u.\n",
            registry_mode.dmPelsWidth, registry_mode.dmPelsHeight,
            devmode.dmPelsWidth, devmode.dmPelsHeight);
    change_ret = ChangeDisplaySettingsW(NULL, CDS_FULLSCREEN);
    ok(change_ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#x.\n", change_ret);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == registry_mode.dmPelsWidth, "Expected surface width %u, got %u.\n",
            registry_mode.dmPelsWidth, ddsd.dwWidth);
    ok(ddsd.dwHeight == registry_mode.dmPelsHeight, "Expected surface height %u, got %u.\n",
            registry_mode.dmPelsHeight, ddsd.dwHeight);
    IDirectDrawSurface4_Release(primary);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &registry_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&registry_rect),
            wine_dbgstr_rect(&r));

    /* Changing the coop level from EXCLUSIVE to NORMAL restores the screen resolution */
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    hr = set_display_mode(ddraw, param.ddraw_width, param.ddraw_height);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);

    PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE);
    expect_messages = exclusive_messages;
    screen_size.cx = 0;
    screen_size.cy = 0;

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    ok(!expect_messages->message, "Expected message %#x, but didn't receive it.\n", expect_messages->message);
    expect_messages = NULL;
    ok(screen_size.cx == registry_mode.dmPelsWidth
            && screen_size.cy == registry_mode.dmPelsHeight,
            "Expected screen size %ux%u, got %ux%u.\n",
            registry_mode.dmPelsWidth, registry_mode.dmPelsHeight,
            screen_size.cx, screen_size.cy);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &registry_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&registry_rect),
            wine_dbgstr_rect(&r));

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == registry_mode.dmPelsWidth, "Expected surface width %u, got %u.\n",
            registry_mode.dmPelsWidth, ddsd.dwWidth);
    ok(ddsd.dwHeight == registry_mode.dmPelsHeight, "Expected surface height %u, got %u.\n",
            registry_mode.dmPelsHeight, ddsd.dwHeight);
    IDirectDrawSurface4_Release(primary);

    /* The screen restore is a property of DDSCL_EXCLUSIVE  */
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    hr = set_display_mode(ddraw, param.ddraw_width, param.ddraw_height);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == param.ddraw_width, "Expected surface width %u, got %u.\n",
            param.ddraw_width, ddsd.dwWidth);
    ok(ddsd.dwHeight == param.ddraw_height, "Expected surface height %u, got %u.\n",
            param.ddraw_height, ddsd.dwHeight);
    IDirectDrawSurface4_Release(primary);

    hr = IDirectDraw4_RestoreDisplayMode(ddraw);
    ok(SUCCEEDED(hr), "RestoreDisplayMode failed, hr %#x.\n", hr);

    /* If the window is changed at the same time, messages are sent to the new window. */
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    hr = set_display_mode(ddraw, param.ddraw_width, param.ddraw_height);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);

    PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE);
    expect_messages = exclusive_messages;
    screen_size.cx = 0;
    screen_size.cy = 0;
    screen_size2.cx = 0;
    screen_size2.cy = 0;

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window2, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    ok(!expect_messages->message, "Expected message %#x, but didn't receive it.\n", expect_messages->message);
    expect_messages = NULL;
    ok(!screen_size.cx && !screen_size.cy, "Got unexpected screen size %ux%u.\n",
            screen_size.cx, screen_size.cy);
    ok(screen_size2.cx == registry_mode.dmPelsWidth && screen_size2.cy == registry_mode.dmPelsHeight,
            "Expected screen size 2 %ux%u, got %ux%u.\n",
            registry_mode.dmPelsWidth, registry_mode.dmPelsHeight, screen_size2.cx, screen_size2.cy);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &ddraw_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&ddraw_rect),
            wine_dbgstr_rect(&r));
    GetWindowRect(window2, &r);
    ok(EqualRect(&r, &registry_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&registry_rect),
            wine_dbgstr_rect(&r));

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == registry_mode.dmPelsWidth, "Expected surface width %u, got %u.\n",
            registry_mode.dmPelsWidth, ddsd.dwWidth);
    ok(ddsd.dwHeight == registry_mode.dmPelsHeight, "Expected surface height %u, got %u.\n",
            registry_mode.dmPelsHeight, ddsd.dwHeight);
    IDirectDrawSurface4_Release(primary);

    ref = IDirectDraw4_Release(ddraw);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &ddraw_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&ddraw_rect),
            wine_dbgstr_rect(&r));

    expect_messages = NULL;
    DestroyWindow(window);
    DestroyWindow(window2);
    UnregisterClassA("ddraw_test_wndproc_wc", GetModuleHandleA(NULL));
    UnregisterClassA("ddraw_test_wndproc_wc2", GetModuleHandleA(NULL));
}

static void test_coop_level_mode_set_multi(void)
{
    IDirectDraw4 *ddraw1, *ddraw2;
    UINT w, h;
    HWND window;
    HRESULT hr;
    ULONG ref;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 100, 100, 0, 0, 0, 0);
    ddraw1 = create_ddraw();
    ok(!!ddraw1, "Failed to create a ddraw object.\n");

    /* With just a single ddraw object, the display mode is restored on
     * release. */
    hr = set_display_mode(ddraw1, 800, 600);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == 800, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == 600, "Got unexpected screen height %u.\n", h);

    ref = IDirectDraw4_Release(ddraw1);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == registry_mode.dmPelsWidth, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == registry_mode.dmPelsHeight, "Got unexpected screen height %u.\n", h);

    /* When there are multiple ddraw objects, the display mode is restored to
     * the initial mode, before the first SetDisplayMode() call. */
    ddraw1 = create_ddraw();
    hr = set_display_mode(ddraw1, 800, 600);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == 800, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == 600, "Got unexpected screen height %u.\n", h);

    ddraw2 = create_ddraw();
    hr = set_display_mode(ddraw2, 640, 480);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == 640, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == 480, "Got unexpected screen height %u.\n", h);

    ref = IDirectDraw4_Release(ddraw2);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == registry_mode.dmPelsWidth, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == registry_mode.dmPelsHeight, "Got unexpected screen height %u.\n", h);

    ref = IDirectDraw4_Release(ddraw1);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == registry_mode.dmPelsWidth, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == registry_mode.dmPelsHeight, "Got unexpected screen height %u.\n", h);

    /* Regardless of release ordering. */
    ddraw1 = create_ddraw();
    hr = set_display_mode(ddraw1, 800, 600);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == 800, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == 600, "Got unexpected screen height %u.\n", h);

    ddraw2 = create_ddraw();
    hr = set_display_mode(ddraw2, 640, 480);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == 640, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == 480, "Got unexpected screen height %u.\n", h);

    ref = IDirectDraw4_Release(ddraw1);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == registry_mode.dmPelsWidth, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == registry_mode.dmPelsHeight, "Got unexpected screen height %u.\n", h);

    ref = IDirectDraw4_Release(ddraw2);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == registry_mode.dmPelsWidth, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == registry_mode.dmPelsHeight, "Got unexpected screen height %u.\n", h);

    /* But only for ddraw objects that called SetDisplayMode(). */
    ddraw1 = create_ddraw();
    ddraw2 = create_ddraw();
    hr = set_display_mode(ddraw2, 640, 480);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == 640, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == 480, "Got unexpected screen height %u.\n", h);

    ref = IDirectDraw4_Release(ddraw1);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == 640, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == 480, "Got unexpected screen height %u.\n", h);

    ref = IDirectDraw4_Release(ddraw2);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == registry_mode.dmPelsWidth, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == registry_mode.dmPelsHeight, "Got unexpected screen height %u.\n", h);

    /* If there's a ddraw object that's currently in exclusive mode, it blocks
     * restoring the display mode. */
    ddraw1 = create_ddraw();
    hr = set_display_mode(ddraw1, 800, 600);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == 800, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == 600, "Got unexpected screen height %u.\n", h);

    ddraw2 = create_ddraw();
    hr = set_display_mode(ddraw2, 640, 480);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == 640, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == 480, "Got unexpected screen height %u.\n", h);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw2, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    ref = IDirectDraw4_Release(ddraw1);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == 640, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == 480, "Got unexpected screen height %u.\n", h);

    ref = IDirectDraw4_Release(ddraw2);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == registry_mode.dmPelsWidth, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == registry_mode.dmPelsHeight, "Got unexpected screen height %u.\n", h);

    /* Exclusive mode blocks mode setting on other ddraw objects in general. */
    ddraw1 = create_ddraw();
    hr = set_display_mode(ddraw1, 800, 600);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == 800, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == 600, "Got unexpected screen height %u.\n", h);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw1, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    ddraw2 = create_ddraw();
    hr = set_display_mode(ddraw2, 640, 480);
    ok(hr == DDERR_NOEXCLUSIVEMODE, "Got unexpected hr %#x.\n", hr);

    ref = IDirectDraw4_Release(ddraw1);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == registry_mode.dmPelsWidth, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == registry_mode.dmPelsHeight, "Got unexpected screen height %u.\n", h);

    ref = IDirectDraw4_Release(ddraw2);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == registry_mode.dmPelsWidth, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == registry_mode.dmPelsHeight, "Got unexpected screen height %u.\n", h);

    DestroyWindow(window);
}

static void test_initialize(void)
{
    IDirectDraw4 *ddraw;
    HRESULT hr;

    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    hr = IDirectDraw4_Initialize(ddraw, NULL);
    ok(hr == DDERR_ALREADYINITIALIZED, "Initialize returned hr %#x.\n", hr);
    IDirectDraw4_Release(ddraw);

    CoInitialize(NULL);
    hr = CoCreateInstance(&CLSID_DirectDraw, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectDraw4, (void **)&ddraw);
    ok(SUCCEEDED(hr), "Failed to create IDirectDraw4 instance, hr %#x.\n", hr);
    hr = IDirectDraw4_Initialize(ddraw, NULL);
    ok(hr == DD_OK, "Initialize returned hr %#x, expected DD_OK.\n", hr);
    hr = IDirectDraw4_Initialize(ddraw, NULL);
    ok(hr == DDERR_ALREADYINITIALIZED, "Initialize returned hr %#x, expected DDERR_ALREADYINITIALIZED.\n", hr);
    IDirectDraw4_Release(ddraw);
    CoUninitialize();
}

static void test_coop_level_surf_create(void)
{
    IDirectDrawSurface4 *surface;
    IDirectDraw4 *ddraw;
    DDSURFACEDESC2 ddsd;
    HRESULT hr;

    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    surface = (void *)0xdeadbeef;
    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &surface, NULL);
    ok(hr == DDERR_NOCOOPERATIVELEVELSET, "Surface creation returned hr %#x.\n", hr);
    ok(surface == (void *)0xdeadbeef, "Got unexpected surface %p.\n", surface);

    surface = (void *)0xdeadbeef;
    hr = IDirectDraw4_CreateSurface(ddraw, NULL, &surface, NULL);
    ok(hr == DDERR_NOCOOPERATIVELEVELSET, "Surface creation returned hr %#x.\n", hr);
    ok(surface == (void *)0xdeadbeef, "Got unexpected surface %p.\n", surface);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    surface = (void *)0xdeadbeef;
    hr = IDirectDraw4_CreateSurface(ddraw, NULL, &surface, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "Unexpected hr %#x.\n", hr);
    ok(surface == (void *)0xdeadbeef, "Got unexpected surface %p.\n", surface);

    IDirectDraw4_Release(ddraw);
}

static void test_vb_discard(void)
{
    static const struct vec4 quad[] =
    {
        {  0.0f, 480.0f, 0.0f, 1.0f},
        {  0.0f,   0.0f, 0.0f, 1.0f},
        {640.0f, 480.0f, 0.0f, 1.0f},
        {640.0f,   0.0f, 0.0f, 1.0f},
    };

    IDirect3DDevice3 *device;
    IDirect3D3 *d3d;
    IDirect3DVertexBuffer *buffer;
    HWND window;
    HRESULT hr;
    D3DVERTEXBUFFERDESC desc;
    BYTE *data;
    static const unsigned int vbsize = 16;
    unsigned int i;

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get d3d interface, hr %#x.\n", hr);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwCaps = D3DVBCAPS_WRITEONLY;
    desc.dwFVF = D3DFVF_XYZRHW;
    desc.dwNumVertices = vbsize;
    hr = IDirect3D3_CreateVertexBuffer(d3d, &desc, &buffer, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to create vertex buffer, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer_Lock(buffer, DDLOCK_DISCARDCONTENTS, (void **)&data, NULL);
    ok(SUCCEEDED(hr), "Failed to lock vertex buffer, hr %#x.\n", hr);
    memcpy(data, quad, sizeof(quad));
    hr = IDirect3DVertexBuffer_Unlock(buffer);
    ok(SUCCEEDED(hr), "Failed to unlock vertex buffer, hr %#x.\n", hr);

    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitiveVB(device, D3DPT_TRIANGLESTRIP, buffer, 0, 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer_Lock(buffer, DDLOCK_DISCARDCONTENTS, (void **)&data, NULL);
    ok(SUCCEEDED(hr), "Failed to lock vertex buffer, hr %#x.\n", hr);
    memset(data, 0xaa, sizeof(struct vec4) * vbsize);
    hr = IDirect3DVertexBuffer_Unlock(buffer);
    ok(SUCCEEDED(hr), "Failed to unlock vertex buffer, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer_Lock(buffer, DDLOCK_DISCARDCONTENTS, (void **)&data, NULL);
    ok(SUCCEEDED(hr), "Failed to lock vertex buffer, hr %#x.\n", hr);
    for (i = 0; i < sizeof(struct vec4) * vbsize; i++)
    {
        if (data[i] != 0xaa)
        {
            ok(FALSE, "Vertex buffer data byte %u is 0x%02x, expected 0xaa\n", i, data[i]);
            break;
        }
    }
    hr = IDirect3DVertexBuffer_Unlock(buffer);
    ok(SUCCEEDED(hr), "Failed to unlock vertex buffer, hr %#x.\n", hr);

    IDirect3DVertexBuffer_Release(buffer);
    IDirect3D3_Release(d3d);
    IDirect3DDevice3_Release(device);
    DestroyWindow(window);
}

static void test_coop_level_multi_window(void)
{
    HWND window1, window2;
    IDirectDraw4 *ddraw;
    HRESULT hr;

    window1 = create_window();
    window2 = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window1, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window2, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    ok(IsWindow(window1), "Window 1 was destroyed.\n");
    ok(IsWindow(window2), "Window 2 was destroyed.\n");

    IDirectDraw4_Release(ddraw);
    DestroyWindow(window2);
    DestroyWindow(window1);
}

static void test_draw_strided(void)
{
    static struct vec3 position[] =
    {
        {-1.0,   -1.0,   0.0},
        {-1.0,    1.0,   0.0},
        { 1.0,    1.0,   0.0},
        { 1.0,   -1.0,   0.0},
    };
    static DWORD diffuse[] =
    {
        0x0000ff00, 0x0000ff00, 0x0000ff00, 0x0000ff00,
    };
    static WORD indices[] =
    {
        0, 1, 2, 2, 3, 0
    };

    IDirectDrawSurface4 *rt;
    IDirect3DDevice3 *device;
    D3DCOLOR color;
    HWND window;
    HRESULT hr;
    D3DDRAWPRIMITIVESTRIDEDDATA strided;
    IDirect3DViewport3 *viewport;
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);
    viewport = create_viewport(device, 0, 0, 640, 480);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to activate the viewport, hr %#x.\n", hr);
    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0x00000000, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear the viewport, hr %#x.\n", hr);

    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);

    memset(&strided, 0x55, sizeof(strided));
    strided.position.lpvData = position;
    strided.position.dwStride = sizeof(*position);
    strided.diffuse.lpvData = diffuse;
    strided.diffuse.dwStride = sizeof(*diffuse);
    hr = IDirect3DDevice3_DrawIndexedPrimitiveStrided(device, D3DPT_TRIANGLELIST, D3DFVF_XYZ | D3DFVF_DIFFUSE,
            &strided, 4, indices, 6, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x0000ff00, 1), "Got unexpected color 0x%08x.\n", color);

    IDirect3DViewport3_Release(viewport);
    IDirectDrawSurface4_Release(rt);
    IDirect3DDevice3_Release(device);
    DestroyWindow(window);
}

static void test_lighting(void)
{
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    static D3DMATRIX mat =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    },
    mat_singular =
    {
        1.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.5f, 1.0f,
    },
    mat_transf =
    {
         0.0f,  0.0f,  1.0f, 0.0f,
         0.0f,  1.0f,  0.0f, 0.0f,
        -1.0f,  0.0f,  0.0f, 0.0f,
         10.f, 10.0f, 10.0f, 1.0f,
    },
    mat_nonaffine =
    {
        1.0f,  0.0f,  0.0f,  0.0f,
        0.0f,  1.0f,  0.0f,  0.0f,
        0.0f,  0.0f,  1.0f, -1.0f,
        10.f, 10.0f, 10.0f,  0.0f,
    };
    static struct
    {
        struct vec3 position;
        DWORD diffuse;
    }
    unlitquad[] =
    {
        {{-1.0f, -1.0f, 0.1f}, 0xffff0000},
        {{-1.0f,  0.0f, 0.1f}, 0xffff0000},
        {{ 0.0f,  0.0f, 0.1f}, 0xffff0000},
        {{ 0.0f, -1.0f, 0.1f}, 0xffff0000},
    },
    litquad[] =
    {
        {{-1.0f,  0.0f, 0.1f}, 0xff00ff00},
        {{-1.0f,  1.0f, 0.1f}, 0xff00ff00},
        {{ 0.0f,  1.0f, 0.1f}, 0xff00ff00},
        {{ 0.0f,  0.0f, 0.1f}, 0xff00ff00},
    };
    static struct
    {
        struct vec3 position;
        struct vec3 normal;
        DWORD diffuse;
    }
    unlitnquad[] =
    {
        {{0.0f, -1.0f, 0.1f}, {1.0f, 1.0f, 1.0f}, 0xff0000ff},
        {{0.0f,  0.0f, 0.1f}, {1.0f, 1.0f, 1.0f}, 0xff0000ff},
        {{1.0f,  0.0f, 0.1f}, {1.0f, 1.0f, 1.0f}, 0xff0000ff},
        {{1.0f, -1.0f, 0.1f}, {1.0f, 1.0f, 1.0f}, 0xff0000ff},
    },
    litnquad[] =
    {
        {{0.0f,  0.0f, 0.1f}, {1.0f, 1.0f, 1.0f}, 0xffffff00},
        {{0.0f,  1.0f, 0.1f}, {1.0f, 1.0f, 1.0f}, 0xffffff00},
        {{1.0f,  1.0f, 0.1f}, {1.0f, 1.0f, 1.0f}, 0xffffff00},
        {{1.0f,  0.0f, 0.1f}, {1.0f, 1.0f, 1.0f}, 0xffffff00},
    },
    nquad[] =
    {
        {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, 0xff0000ff},
        {{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, 0xff0000ff},
        {{ 1.0f,  1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, 0xff0000ff},
        {{ 1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, 0xff0000ff},
    },
    rotatedquad[] =
    {
        {{-10.0f, -11.0f,  11.0f}, {-1.0f, 0.0f, 0.0f}, 0xff0000ff},
        {{-10.0f,  -9.0f,  11.0f}, {-1.0f, 0.0f, 0.0f}, 0xff0000ff},
        {{-10.0f,  -9.0f,   9.0f}, {-1.0f, 0.0f, 0.0f}, 0xff0000ff},
        {{-10.0f, -11.0f,   9.0f}, {-1.0f, 0.0f, 0.0f}, 0xff0000ff},
    },
    translatedquad[] =
    {
        {{-11.0f, -11.0f, -10.0f}, {0.0f, 0.0f, -1.0f}, 0xff0000ff},
        {{-11.0f,  -9.0f, -10.0f}, {0.0f, 0.0f, -1.0f}, 0xff0000ff},
        {{ -9.0f,  -9.0f, -10.0f}, {0.0f, 0.0f, -1.0f}, 0xff0000ff},
        {{ -9.0f, -11.0f, -10.0f}, {0.0f, 0.0f, -1.0f}, 0xff0000ff},
    };
    static WORD indices[] = {0, 1, 2, 2, 3, 0};
    static const struct
    {
        D3DMATRIX *world_matrix;
        void *quad;
        DWORD expected;
        const char *message;
    }
    tests[] =
    {
        {&mat, nquad, 0x000000ff, "Lit quad with light"},
        {&mat_singular, nquad, 0x000000b4, "Lit quad with singular world matrix"},
        {&mat_transf, rotatedquad, 0x000000ff, "Lit quad with transformation matrix"},
        {&mat_nonaffine, translatedquad, 0x000000ff, "Lit quad with non-affine matrix"},
    };

    HWND window;
    IDirect3D3 *d3d;
    IDirect3DDevice3 *device;
    IDirectDrawSurface4 *rt;
    IDirect3DViewport3 *viewport;
    IDirect3DMaterial3 *material;
    IDirect3DLight *light;
    D3DMATERIALHANDLE mat_handle;
    D3DLIGHT2 light_desc;
    HRESULT hr;
    DWORD fvf = D3DFVF_XYZ | D3DFVF_DIFFUSE;
    DWORD nfvf = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_NORMAL;
    D3DCOLOR color;
    ULONG refcount;
    unsigned int i;

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get D3D interface, hr %#x.\n", hr);

    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    viewport = create_viewport(device, 0, 0, 640, 480);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);

    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0xffffffff, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);

    hr = IDirect3DDevice3_SetTransform(device, D3DTRANSFORMSTATE_WORLD, &mat);
    ok(SUCCEEDED(hr), "Failed to set world transformation, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTransform(device, D3DTRANSFORMSTATE_VIEW, &mat);
    ok(SUCCEEDED(hr), "Failed to set view transformation, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTransform(device, D3DTRANSFORMSTATE_PROJECTION, &mat);
    ok(SUCCEEDED(hr), "Failed to set projection transformation, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_CLIPPING, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable clipping, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_ZENABLE, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable zbuffer, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_FOGENABLE, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable fog, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_STENCILENABLE, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable stencil buffer, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
    ok(SUCCEEDED(hr), "Failed to disable culling, hr %#x.\n", hr);

    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);

    /* There is no D3DRENDERSTATE_LIGHTING on ddraw < 7. */
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable lighting, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, fvf, unlitquad, 4,
            indices, 6, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_LIGHTING, TRUE);
    ok(SUCCEEDED(hr), "Failed to enable lighting, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, fvf, litquad, 4,
            indices, 6, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable lighting, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, nfvf, unlitnquad, 4,
            indices, 6, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_LIGHTING, TRUE);
    ok(SUCCEEDED(hr), "Failed to enable lighting, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, nfvf, litnquad, 4,
            indices, 6, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = get_surface_color(rt, 160, 360);
    ok(color == 0x00ff0000, "Unlit quad without normals has color 0x%08x.\n", color);
    color = get_surface_color(rt, 160, 120);
    ok(color == 0x0000ff00, "Lit quad without normals has color 0x%08x.\n", color);
    color = get_surface_color(rt, 480, 360);
    ok(color == 0x000000ff, "Unlit quad with normals has color 0x%08x.\n", color);
    color = get_surface_color(rt, 480, 120);
    ok(color == 0x00ffff00, "Lit quad with normals has color 0x%08x.\n", color);

    material = create_diffuse_material(device, 0.0f, 1.0f, 0.0f, 0.0f);
    hr = IDirect3DMaterial3_GetHandle(material, device, &mat_handle);
    ok(SUCCEEDED(hr), "Failed to set material state, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetLightState(device, D3DLIGHTSTATE_MATERIAL, mat_handle);
    ok(SUCCEEDED(hr), "Failed to set material state, hr %#x.\n", hr);

    hr = IDirect3D3_CreateLight(d3d, &light, NULL);
    ok(SUCCEEDED(hr), "Failed to create a light object, hr %#x.\n", hr);
    memset(&light_desc, 0, sizeof(light_desc));
    light_desc.dwSize = sizeof(light_desc);
    light_desc.dltType = D3DLIGHT_DIRECTIONAL;
    U1(light_desc.dcvColor).r = 1.0f;
    U2(light_desc.dcvColor).g = 1.0f;
    U3(light_desc.dcvColor).b = 1.0f;
    U4(light_desc.dcvColor).a = 1.0f;
    U3(light_desc.dvDirection).z = 1.0f;
    hr = IDirect3DLight_SetLight(light, (D3DLIGHT *)&light_desc);
    ok(SUCCEEDED(hr), "Failed to set light, hr %#x.\n", hr);
    hr = IDirect3DViewport3_AddLight(viewport, light);
    ok(SUCCEEDED(hr), "Failed to add a light to the viewport, hr %#x.\n", hr);

    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0xffffffff, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);

    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);

    hr = IDirect3DDevice3_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, nfvf, nquad,
            4, indices, 6, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = get_surface_color(rt, 320, 240);
    ok(color == 0x00000000, "Lit quad with no light has color 0x%08x.\n", color);

    light_desc.dwFlags = D3DLIGHT_ACTIVE;
    hr = IDirect3DLight_SetLight(light, (D3DLIGHT *)&light_desc);
    ok(SUCCEEDED(hr), "Failed to set light, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        hr = IDirect3DDevice3_SetTransform(device, D3DTRANSFORMSTATE_WORLD, tests[i].world_matrix);
        ok(SUCCEEDED(hr), "Failed to set world transformation, hr %#x.\n", hr);

        hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0xffffffff, 0.0f, 0);
        ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);

        hr = IDirect3DDevice3_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);

        hr = IDirect3DDevice3_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, nfvf, tests[i].quad,
                4, indices, 6, 0);
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

        hr = IDirect3DDevice3_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

        color = get_surface_color(rt, 320, 240);
        ok(color == tests[i].expected, "%s has color 0x%08x.\n", tests[i].message, color);
    }

    hr = IDirect3DViewport3_DeleteLight(viewport, light);
    ok(SUCCEEDED(hr), "Failed to remove a light from the viewport, hr %#x.\n", hr);
    IDirect3DLight_Release(light);
    destroy_material(material);
    IDirect3DViewport3_Release(viewport);
    IDirectDrawSurface4_Release(rt);
    refcount = IDirect3DDevice3_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D3_Release(d3d);
    DestroyWindow(window);
}

static void test_specular_lighting(void)
{
    static const unsigned int vertices_side = 5;
    const unsigned int indices_count = (vertices_side - 1) * (vertices_side - 1) * 2 * 3;
    static const DWORD fvf = D3DFVF_XYZ | D3DFVF_NORMAL;
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    static D3DMATRIX mat =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    static D3DLIGHT2 directional =
    {
        sizeof(D3DLIGHT2),
        D3DLIGHT_DIRECTIONAL,
        {{1.0f}, {1.0f}, {1.0f}, {0.0f}},
        {{0.0f}, {0.0f}, {0.0f}},
        {{0.0f}, {0.0f}, {1.0f}},
    },
    point =
    {
        sizeof(D3DLIGHT2),
        D3DLIGHT_POINT,
        {{1.0f}, {1.0f}, {1.0f}, {0.0f}},
        {{0.0f}, {0.0f}, {0.0f}},
        {{0.0f}, {0.0f}, {0.0f}},
        100.0f,
        0.0f,
        0.0f, 0.0f, 1.0f,
    },
    spot =
    {
        sizeof(D3DLIGHT2),
        D3DLIGHT_SPOT,
        {{1.0f}, {1.0f}, {1.0f}, {0.0f}},
        {{0.0f}, {0.0f}, {0.0f}},
        {{0.0f}, {0.0f}, {1.0f}},
        100.0f,
        1.0f,
        0.0f, 0.0f, 1.0f,
        M_PI / 12.0f, M_PI / 3.0f
    },
    parallelpoint =
    {
        sizeof(D3DLIGHT2),
        D3DLIGHT_PARALLELPOINT,
        {{1.0f}, {1.0f}, {1.0f}, {0.0f}},
        {{0.5f}, {0.0f}, {-1.0f}},
        {{0.0f}, {0.0f}, {0.0f}},
    },
    point_side =
    {
        sizeof(D3DLIGHT2),
        D3DLIGHT_POINT,
        {{1.0f}, {1.0f}, {1.0f}, {0.0f}},
        {{-1.1f}, {0.0f}, {1.1f}},
        {{0.0f}, {0.0f}, {0.0f}},
        100.0f,
        0.0f,
        1.0f, 0.0f, 0.0f,
    },
    point_far =
    {
        sizeof(D3DLIGHT2),
        D3DLIGHT_POINT,
        {{1.0f}, {1.0f}, {1.0f}, {0.0f}},
        {{0.0f}, {0.0f}, {0.1f}},
        {{0.0f}, {0.0f}, {0.0f}},
        1.0f,
        0.0f,
        1.0f, 0.0f, 0.0f,
    };
    static const struct expected_color
    {
        unsigned int x, y;
        D3DCOLOR color;
    }
    expected_directional[] =
    {
        {160, 120, 0x003c3c3c},
        {320, 120, 0x00717171},
        {480, 120, 0x003c3c3c},
        {160, 240, 0x00717171},
        {320, 240, 0x00ffffff},
        {480, 240, 0x00717171},
        {160, 360, 0x003c3c3c},
        {320, 360, 0x00717171},
        {480, 360, 0x003c3c3c},
    },
    expected_point[] =
    {
        {160, 120, 0x00000000},
        {320, 120, 0x00090909},
        {480, 120, 0x00000000},
        {160, 240, 0x00090909},
        {320, 240, 0x00fafafa},
        {480, 240, 0x00090909},
        {160, 360, 0x00000000},
        {320, 360, 0x00090909},
        {480, 360, 0x00000000},
    },
    expected_spot[] =
    {
        {160, 120, 0x00000000},
        {320, 120, 0x00020202},
        {480, 120, 0x00000000},
        {160, 240, 0x00020202},
        {320, 240, 0x00fafafa},
        {480, 240, 0x00020202},
        {160, 360, 0x00000000},
        {320, 360, 0x00020202},
        {480, 360, 0x00000000},
    },
    expected_parallelpoint[] =
    {
        {160, 120, 0x00050505},
        {320, 120, 0x002c2c2c},
        {480, 120, 0x006e6e6e},
        {160, 240, 0x00090909},
        {320, 240, 0x00717171},
        {480, 240, 0x00ffffff},
        {160, 360, 0x00050505},
        {320, 360, 0x002c2c2c},
        {480, 360, 0x006e6e6e},
    },
    expected_point_side[] =
    {
        {160, 120, 0x00000000},
        {320, 120, 0x00000000},
        {480, 120, 0x00000000},
        {160, 240, 0x00000000},
        {320, 240, 0x00000000},
        {480, 240, 0x00000000},
        {160, 360, 0x00000000},
        {320, 360, 0x00000000},
        {480, 360, 0x00000000},
    },
    expected_point_far[] =
    {
        {160, 120, 0x00000000},
        {320, 120, 0x00000000},
        {480, 120, 0x00000000},
        {160, 240, 0x00000000},
        {320, 240, 0x00ffffff},
        {480, 240, 0x00000000},
        {160, 360, 0x00000000},
        {320, 360, 0x00000000},
        {480, 360, 0x00000000},
    };
    static const struct
    {
        D3DLIGHT2 *light;
        BOOL local_viewer;
        float specular_power;
        const struct expected_color *expected;
        unsigned int expected_count;
    }
    tests[] =
    {
        /* D3DRENDERSTATE_LOCALVIEWER does not exist in D3D < 7 (the behavior is
         * the one you get on newer D3D versions with it set as TRUE). */
        {&directional, FALSE, 30.0f, expected_directional, ARRAY_SIZE(expected_directional)},
        {&directional, TRUE, 30.0f, expected_directional, ARRAY_SIZE(expected_directional)},
        {&point, TRUE, 30.0f, expected_point, ARRAY_SIZE(expected_point)},
        {&spot, TRUE, 30.0f, expected_spot, ARRAY_SIZE(expected_spot)},
        {&parallelpoint, TRUE, 30.0f, expected_parallelpoint, ARRAY_SIZE(expected_parallelpoint)},
        {&point_side, TRUE, 0.0f, expected_point_side, ARRAY_SIZE(expected_point_side)},
        {&point_far, TRUE, 1.0f, expected_point_far, ARRAY_SIZE(expected_point_far)},
    };
    IDirect3D3 *d3d;
    IDirect3DDevice3 *device;
    IDirectDrawSurface4 *rt;
    IDirect3DViewport3 *viewport;
    IDirect3DMaterial3 *material;
    IDirect3DLight *light;
    D3DMATERIALHANDLE mat_handle;
    D3DCOLOR color;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    unsigned int i, j, x, y;
    struct
    {
        struct vec3 position;
        struct vec3 normal;
    } *quad;
    WORD *indices;

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    quad = HeapAlloc(GetProcessHeap(), 0, vertices_side * vertices_side * sizeof(*quad));
    indices = HeapAlloc(GetProcessHeap(), 0, indices_count * sizeof(*indices));
    for (i = 0, y = 0; y < vertices_side; ++y)
    {
        for (x = 0; x < vertices_side; ++x)
        {
            quad[i].position.x = x * 2.0f / (vertices_side - 1) - 1.0f;
            quad[i].position.y = y * 2.0f / (vertices_side - 1) - 1.0f;
            quad[i].position.z = 1.0f;
            quad[i].normal.x = 0.0f;
            quad[i].normal.y = 0.0f;
            quad[i++].normal.z = -1.0f;
        }
    }
    for (i = 0, y = 0; y < (vertices_side - 1); ++y)
    {
        for (x = 0; x < (vertices_side - 1); ++x)
        {
            indices[i++] = y * vertices_side + x + 1;
            indices[i++] = y * vertices_side + x;
            indices[i++] = (y + 1) * vertices_side + x;
            indices[i++] = y * vertices_side + x + 1;
            indices[i++] = (y + 1) * vertices_side + x;
            indices[i++] = (y + 1) * vertices_side + x + 1;
        }
    }

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get D3D interface, hr %#x.\n", hr);

    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    viewport = create_viewport(device, 0, 0, 640, 480);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);

    hr = IDirect3DDevice3_SetTransform(device, D3DTRANSFORMSTATE_WORLD, &mat);
    ok(SUCCEEDED(hr), "Failed to set world transform, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTransform(device, D3DTRANSFORMSTATE_VIEW, &mat);
    ok(SUCCEEDED(hr), "Failed to set view transform, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTransform(device, D3DTRANSFORMSTATE_PROJECTION, &mat);
    ok(SUCCEEDED(hr), "Failed to set projection transform, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_CLIPPING, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable clipping, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_ZENABLE, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable z-buffering, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_FOGENABLE, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable fog, hr %#x.\n", hr);

    hr = IDirect3D3_CreateLight(d3d, &light, NULL);
    ok(SUCCEEDED(hr), "Failed to create a light object, hr %#x.\n", hr);
    hr = IDirect3DViewport3_AddLight(viewport, light);
    ok(SUCCEEDED(hr), "Failed to add a light to the viewport, hr %#x.\n", hr);

    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_SPECULARENABLE, TRUE);
    ok(SUCCEEDED(hr), "Failed to enable specular lighting, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        tests[i].light->dwFlags = D3DLIGHT_ACTIVE;
        hr = IDirect3DLight_SetLight(light, (D3DLIGHT *)tests[i].light);
        ok(SUCCEEDED(hr), "Failed to set light, hr %#x.\n", hr);

        hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_LOCALVIEWER, tests[i].local_viewer);
        ok(SUCCEEDED(hr), "Failed to set local viewer state, hr %#x.\n", hr);

        material = create_specular_material(device, 1.0f, 1.0f, 1.0f, 1.0f, tests[i].specular_power);
        hr = IDirect3DMaterial3_GetHandle(material, device, &mat_handle);
        ok(SUCCEEDED(hr), "Failed to get material handle, hr %#x.\n", hr);
        hr = IDirect3DDevice3_SetLightState(device, D3DLIGHTSTATE_MATERIAL, mat_handle);
        ok(SUCCEEDED(hr), "Failed to set material state, hr %#x.\n", hr);

        hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0xffffffff, 0.0f, 0);
        ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);

        hr = IDirect3DDevice3_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);

        hr = IDirect3DDevice3_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, fvf, quad,
                vertices_side * vertices_side, indices, indices_count, 0);
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

        hr = IDirect3DDevice3_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

        for (j = 0; j < tests[i].expected_count; ++j)
        {
            color = get_surface_color(rt, tests[i].expected[j].x, tests[i].expected[j].y);
            ok(compare_color(color, tests[i].expected[j].color, 1),
                    "Expected color 0x%08x at location (%u, %u), got 0x%08x, case %u.\n",
                    tests[i].expected[j].color, tests[i].expected[j].x,
                    tests[i].expected[j].y, color, i);
        }

        destroy_material(material);
    }

    hr = IDirect3DViewport3_DeleteLight(viewport, light);
    ok(SUCCEEDED(hr), "Failed to remove a light from the viewport, hr %#x.\n", hr);
    IDirect3DLight_Release(light);
    IDirect3DViewport3_Release(viewport);
    IDirectDrawSurface4_Release(rt);
    refcount = IDirect3DDevice3_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D3_Release(d3d);
    DestroyWindow(window);
    HeapFree(GetProcessHeap(), 0, indices);
    HeapFree(GetProcessHeap(), 0, quad);
}

static void test_clear_rect_count(void)
{
    IDirectDrawSurface4 *rt;
    IDirect3DDevice3 *device;
    D3DCOLOR color;
    HWND window;
    HRESULT hr;
    IDirect3DViewport3 *viewport;
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    viewport = create_viewport(device, 0, 0, 640, 480);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to activate the viewport, hr %#x.\n", hr);
    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0x00ffffff, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear the viewport, hr %#x.\n", hr);
    hr = IDirect3DViewport3_Clear2(viewport, 0, &clear_rect, D3DCLEAR_TARGET, 0x00ff0000, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear the viewport, hr %#x.\n", hr);
    hr = IDirect3DViewport3_Clear2(viewport, 0, NULL, D3DCLEAR_TARGET, 0x0000ff00, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear the viewport, hr %#x.\n", hr);
    hr = IDirect3DViewport3_Clear2(viewport, 1, NULL, D3DCLEAR_TARGET, 0x000000ff, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear the viewport, hr %#x.\n", hr);

    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x00ffffff, 1) || broken(compare_color(color, 0x000000ff, 1)),
            "Got unexpected color 0x%08x.\n", color);

    IDirect3DViewport3_Release(viewport);
    IDirectDrawSurface4_Release(rt);
    IDirect3DDevice3_Release(device);
    DestroyWindow(window);
}

static BOOL test_mode_restored(IDirectDraw4 *ddraw, HWND window)
{
    DDSURFACEDESC2 ddsd1, ddsd2;
    HRESULT hr;

    memset(&ddsd1, 0, sizeof(ddsd1));
    ddsd1.dwSize = sizeof(ddsd1);
    hr = IDirectDraw4_GetDisplayMode(ddraw, &ddsd1);
    ok(SUCCEEDED(hr), "GetDisplayMode failed, hr %#x.\n", hr);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    hr = set_display_mode(ddraw, 640, 480);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);
    hr = IDirectDraw4_GetDisplayMode(ddraw, &ddsd2);
    ok(SUCCEEDED(hr), "GetDisplayMode failed, hr %#x.\n", hr);
    hr = IDirectDraw4_RestoreDisplayMode(ddraw);
    ok(SUCCEEDED(hr), "RestoreDisplayMode failed, hr %#x.\n", hr);

    return ddsd1.dwWidth == ddsd2.dwWidth && ddsd1.dwHeight == ddsd2.dwHeight;
}

static void test_coop_level_versions(void)
{
    HWND window;
    IDirectDraw *ddraw;
    HRESULT hr;
    BOOL restored;
    IDirectDrawSurface *surface;
    IDirectDraw4 *ddraw4;
    DDSURFACEDESC ddsd;

    window = create_window();
    ddraw4 = create_ddraw();
    ok(!!ddraw4, "Failed to create a ddraw object.\n");
    /* Newly created ddraw objects restore the mode on ddraw2+::SetCooperativeLevel(NORMAL) */
    restored = test_mode_restored(ddraw4, window);
    ok(restored, "Display mode not restored in new ddraw object\n");

    /* A failing ddraw1::SetCooperativeLevel call does not have an effect */
    hr = IDirectDraw4_QueryInterface(ddraw4, &IID_IDirectDraw, (void **)&ddraw);
    ok(SUCCEEDED(hr), "QueryInterface failed, hr %#x.\n", hr);

    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE);
    ok(FAILED(hr), "SetCooperativeLevel returned %#x, expected failure.\n", hr);
    restored = test_mode_restored(ddraw4, window);
    ok(restored, "Display mode not restored after bad ddraw1::SetCooperativeLevel call\n");

    /* A successful one does */
    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    restored = test_mode_restored(ddraw4, window);
    ok(!restored, "Display mode restored after good ddraw1::SetCooperativeLevel call\n");

    IDirectDraw_Release(ddraw);
    IDirectDraw4_Release(ddraw4);

    ddraw4 = create_ddraw();
    ok(!!ddraw4, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_QueryInterface(ddraw4, &IID_IDirectDraw, (void **)&ddraw);
    ok(SUCCEEDED(hr), "QueryInterface failed, hr %#x.\n", hr);

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_SETFOCUSWINDOW);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    restored = test_mode_restored(ddraw4, window);
    ok(!restored, "Display mode restored after ddraw1::SetCooperativeLevel(SETFOCUSWINDOW) call\n");

    IDirectDraw_Release(ddraw);
    IDirectDraw4_Release(ddraw4);

    /* A failing call does not restore the ddraw2+ behavior */
    ddraw4 = create_ddraw();
    ok(!!ddraw4, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_QueryInterface(ddraw4, &IID_IDirectDraw, (void **)&ddraw);
    ok(SUCCEEDED(hr), "QueryInterface failed, hr %#x.\n", hr);

    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE);
    ok(FAILED(hr), "SetCooperativeLevel returned %#x, expected failure.\n", hr);
    restored = test_mode_restored(ddraw4, window);
    ok(!restored, "Display mode restored after good-bad ddraw1::SetCooperativeLevel() call sequence\n");

    IDirectDraw_Release(ddraw);
    IDirectDraw4_Release(ddraw4);

    /* Neither does a sequence of successful calls with the new interface */
    ddraw4 = create_ddraw();
    ok(!!ddraw4, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_QueryInterface(ddraw4, &IID_IDirectDraw, (void **)&ddraw);
    ok(SUCCEEDED(hr), "QueryInterface failed, hr %#x.\n", hr);

    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw4, window, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw4, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    restored = test_mode_restored(ddraw4, window);
    ok(!restored, "Display mode restored after ddraw1-ddraw4 SetCooperativeLevel() call sequence\n");
    IDirectDraw_Release(ddraw);
    IDirectDraw4_Release(ddraw4);

    /* ddraw1::CreateSurface does not triger the ddraw1 behavior */
    ddraw4 = create_ddraw();
    ok(!!ddraw4, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_QueryInterface(ddraw4, &IID_IDirectDraw, (void **)&ddraw);
    ok(SUCCEEDED(hr), "QueryInterface failed, hr %#x.\n", hr);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw4, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth = ddsd.dwHeight = 8;
    hr = IDirectDraw_CreateSurface(ddraw, &ddsd, &surface, NULL);
    ok(SUCCEEDED(hr), "CreateSurface failed, hr %#x.\n", hr);
    IDirectDrawSurface_Release(surface);
    restored = test_mode_restored(ddraw4, window);
    ok(restored, "Display mode not restored after ddraw1::CreateSurface() call\n");

    IDirectDraw_Release(ddraw);
    IDirectDraw4_Release(ddraw4);
    DestroyWindow(window);
}

static void test_lighting_interface_versions(void)
{
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    IDirect3DMaterial3 *emissive;
    IDirect3DViewport3 *viewport;
    IDirect3DDevice3 *device;
    IDirectDrawSurface4 *rt;
    D3DCOLOR color;
    HWND window;
    HRESULT hr;
    D3DMATERIALHANDLE mat_handle;
    DWORD rs;
    unsigned int i;
    ULONG ref;
    static D3DVERTEX quad[] =
    {
        {{-1.0f}, { 1.0f}, {0.0f}, {1.0f}, {0.0f}, {0.0f}},
        {{ 1.0f}, { 1.0f}, {0.0f}, {1.0f}, {0.0f}, {0.0f}},
        {{-1.0f}, {-1.0f}, {0.0f}, {1.0f}, {0.0f}, {0.0f}},
        {{ 1.0f}, {-1.0f}, {0.0f}, {1.0f}, {0.0f}, {0.0f}},
    };

#define FVF_COLORVERTEX (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_SPECULAR)
    static struct
    {
        struct vec3 position;
        struct vec3 normal;
        DWORD diffuse, specular;
    }
    quad2[] =
    {
        {{-1.0f,  1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0xffff0000, 0xff808080},
        {{ 1.0f,  1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0xffff0000, 0xff808080},
        {{-1.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0xffff0000, 0xff808080},
        {{ 1.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0xffff0000, 0xff808080},
    };

    static D3DLVERTEX lquad[] =
    {
        {{-1.0f}, { 1.0f}, {0.0f}, 0, {0xffff0000}, {0xff808080}},
        {{ 1.0f}, { 1.0f}, {0.0f}, 0, {0xffff0000}, {0xff808080}},
        {{-1.0f}, {-1.0f}, {0.0f}, 0, {0xffff0000}, {0xff808080}},
        {{ 1.0f}, {-1.0f}, {0.0f}, 0, {0xffff0000}, {0xff808080}},
    };

#define FVF_LVERTEX2 (D3DFVF_LVERTEX & ~D3DFVF_RESERVED1)
    static struct
    {
        struct vec3 position;
        DWORD diffuse, specular;
        struct vec2 texcoord;
    }
    lquad2[] =
    {
        {{-1.0f,  1.0f, 0.0f}, 0xffff0000, 0xff808080},
        {{ 1.0f,  1.0f, 0.0f}, 0xffff0000, 0xff808080},
        {{-1.0f, -1.0f, 0.0f}, 0xffff0000, 0xff808080},
        {{ 1.0f, -1.0f, 0.0f}, 0xffff0000, 0xff808080},
    };

    static D3DTLVERTEX tlquad[] =
    {
        {{   0.0f}, { 480.0f}, {0.0f}, {1.0f}, {0xff0000ff}, {0xff808080}},
        {{   0.0f}, {   0.0f}, {0.0f}, {1.0f}, {0xff0000ff}, {0xff808080}},
        {{ 640.0f}, { 480.0f}, {0.0f}, {1.0f}, {0xff0000ff}, {0xff808080}},
        {{ 640.0f}, {   0.0f}, {0.0f}, {1.0f}, {0xff0000ff}, {0xff808080}},
    };

    static const struct
    {
        DWORD vertextype;
        void *data;
        DWORD d3drs_lighting, d3drs_specular;
        DWORD draw_flags;
        D3DCOLOR color;
    }
    tests[] =
    {
        /* Lighting is enabled when all of these conditions are met:
         * 1) No pretransformed position(D3DFVF_XYZRHW)
         * 2) Normals are available (D3DFVF_NORMAL)
         * 3) D3DDP_DONOTLIGHT is not set.
         *
         * D3DRENDERSTATE_LIGHTING is ignored, it is not defined
         * in this d3d version */

        /* 0 */
        { D3DFVF_VERTEX,    quad,       FALSE,  FALSE,  0,                  0x0000ff00},
        { D3DFVF_VERTEX,    quad,       TRUE,   FALSE,  0,                  0x0000ff00},
        { D3DFVF_VERTEX,    quad,       FALSE,  FALSE,  D3DDP_DONOTLIGHT,   0x00ffffff},
        { D3DFVF_VERTEX,    quad,       TRUE,   FALSE,  D3DDP_DONOTLIGHT,   0x00ffffff},
        { D3DFVF_VERTEX,    quad,       FALSE,  TRUE,   0,                  0x0000ff00},
        { D3DFVF_VERTEX,    quad,       TRUE,   TRUE,   0,                  0x0000ff00},
        { D3DFVF_VERTEX,    quad,       FALSE,  TRUE,   D3DDP_DONOTLIGHT,   0x00ffffff},
        { D3DFVF_VERTEX,    quad,       TRUE,   TRUE,   D3DDP_DONOTLIGHT,   0x00ffffff},

        /* 8 */
        { FVF_COLORVERTEX,  quad2,      FALSE,  FALSE,  0,                  0x0000ff00},
        { FVF_COLORVERTEX,  quad2,      TRUE,   FALSE,  0,                  0x0000ff00},
        { FVF_COLORVERTEX,  quad2,      FALSE,  FALSE,  D3DDP_DONOTLIGHT,   0x00ff0000},
        { FVF_COLORVERTEX,  quad2,      TRUE,   FALSE,  D3DDP_DONOTLIGHT,   0x00ff0000},
        /* The specular color in the vertex is ignored because
         * D3DRENDERSTATE_COLORVERTEX is not enabled */
        { FVF_COLORVERTEX,  quad2,      FALSE,  TRUE,   0,                  0x0000ff00},
        { FVF_COLORVERTEX,  quad2,      TRUE,   TRUE,   0,                  0x0000ff00},
        { FVF_COLORVERTEX,  quad2,      FALSE,  TRUE,   D3DDP_DONOTLIGHT,   0x00ff8080},
        { FVF_COLORVERTEX,  quad2,      TRUE,   TRUE,   D3DDP_DONOTLIGHT,   0x00ff8080},

        /* 16 */
        { D3DFVF_LVERTEX,   lquad,      FALSE,  FALSE,  0,                  0x00ff0000},
        { D3DFVF_LVERTEX,   lquad,      TRUE,   FALSE,  0,                  0x00ff0000},
        { D3DFVF_LVERTEX,   lquad,      FALSE,  FALSE,  D3DDP_DONOTLIGHT,   0x00ff0000},
        { D3DFVF_LVERTEX,   lquad,      TRUE,   FALSE,  D3DDP_DONOTLIGHT,   0x00ff0000},
        { D3DFVF_LVERTEX,   lquad,      FALSE,  TRUE,   0,                  0x00ff8080},
        { D3DFVF_LVERTEX,   lquad,      TRUE,   TRUE,   0,                  0x00ff8080},
        { D3DFVF_LVERTEX,   lquad,      FALSE,  TRUE,   D3DDP_DONOTLIGHT,   0x00ff8080},
        { D3DFVF_LVERTEX,   lquad,      TRUE,   TRUE,   D3DDP_DONOTLIGHT,   0x00ff8080},

        /* 24 */
        { FVF_LVERTEX2,     lquad2,     FALSE,  FALSE,  0,                  0x00ff0000},
        { FVF_LVERTEX2,     lquad2,     TRUE,   FALSE,  0,                  0x00ff0000},
        { FVF_LVERTEX2,     lquad2,     FALSE,  FALSE,  D3DDP_DONOTLIGHT,   0x00ff0000},
        { FVF_LVERTEX2,     lquad2,     TRUE,   FALSE,  D3DDP_DONOTLIGHT,   0x00ff0000},
        { FVF_LVERTEX2,     lquad2,     FALSE,  TRUE,   0,                  0x00ff8080},
        { FVF_LVERTEX2,     lquad2,     TRUE,   TRUE,   0,                  0x00ff8080},
        { FVF_LVERTEX2,     lquad2,     FALSE,  TRUE,   D3DDP_DONOTLIGHT,   0x00ff8080},
        { FVF_LVERTEX2,     lquad2,     TRUE,   TRUE,   D3DDP_DONOTLIGHT,   0x00ff8080},

        /* 32 */
        { D3DFVF_TLVERTEX,  tlquad,     FALSE,  FALSE,  0,                  0x000000ff},
        { D3DFVF_TLVERTEX,  tlquad,     TRUE,   FALSE,  0,                  0x000000ff},
        { D3DFVF_TLVERTEX,  tlquad,     FALSE,  FALSE,  D3DDP_DONOTLIGHT,   0x000000ff},
        { D3DFVF_TLVERTEX,  tlquad,     TRUE,   FALSE,  D3DDP_DONOTLIGHT,   0x000000ff},
        { D3DFVF_TLVERTEX,  tlquad,     FALSE,  TRUE,   0,                  0x008080ff},
        { D3DFVF_TLVERTEX,  tlquad,     TRUE,   TRUE,   0,                  0x008080ff},
        { D3DFVF_TLVERTEX,  tlquad,     FALSE,  TRUE,   D3DDP_DONOTLIGHT,   0x008080ff},
        { D3DFVF_TLVERTEX,  tlquad,     TRUE,   TRUE,   D3DDP_DONOTLIGHT,   0x008080ff},
    };

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    viewport = create_viewport(device, 0, 0, 640, 480);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);

    emissive = create_emissive_material(device, 0.0f, 1.0f, 0.0f, 0.0f);
    hr = IDirect3DMaterial3_GetHandle(emissive, device, &mat_handle);
    ok(SUCCEEDED(hr), "Failed to get material handle, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetLightState(device, D3DLIGHTSTATE_MATERIAL, mat_handle);
    ok(SUCCEEDED(hr), "Failed to set material state, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_ZENABLE, D3DZB_FALSE);
    ok(SUCCEEDED(hr), "Failed to disable z test, hr %#x.\n", hr);

    hr = IDirect3DDevice3_GetRenderState(device, D3DRENDERSTATE_SPECULARENABLE, &rs);
    ok(SUCCEEDED(hr), "Failed to get specularenable render state, hr %#x.\n", hr);
    ok(rs == FALSE, "Initial D3DRENDERSTATE_SPECULARENABLE is %#x, expected FALSE.\n", rs);

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0xff202020, 0.0f, 0);
        ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);

        hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_LIGHTING, tests[i].d3drs_lighting);
        ok(SUCCEEDED(hr), "Failed to set lighting render state, hr %#x.\n", hr);
        hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_SPECULARENABLE,
                tests[i].d3drs_specular);
        ok(SUCCEEDED(hr), "Failed to set specularenable render state, hr %#x.\n", hr);

        hr = IDirect3DDevice3_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
        hr = IDirect3DDevice2_DrawPrimitive(device, D3DPT_TRIANGLESTRIP,
                tests[i].vertextype, tests[i].data, 4, tests[i].draw_flags | D3DDP_WAIT);
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
        hr = IDirect3DDevice3_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

        color = get_surface_color(rt, 320, 240);
        ok(compare_color(color, tests[i].color, 1),
                "Got unexpected color 0x%08x, expected 0x%08x, test %u.\n",
                color, tests[i].color, i);
    }

    destroy_material(emissive);
    IDirectDrawSurface4_Release(rt);
    ref = IDirect3DDevice3_Release(device);
    ok(ref == 0, "Device not properly released, refcount %u.\n", ref);
    DestroyWindow(window);
}

static struct
{
    BOOL received;
    IDirectDraw4 *ddraw;
    HWND window;
    DWORD coop_level;
} activateapp_testdata;

static LRESULT CALLBACK activateapp_test_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (message == WM_ACTIVATEAPP)
    {
        if (activateapp_testdata.ddraw)
        {
            HRESULT hr;
            activateapp_testdata.received = FALSE;
            hr = IDirectDraw4_SetCooperativeLevel(activateapp_testdata.ddraw,
                    activateapp_testdata.window, activateapp_testdata.coop_level);
            ok(SUCCEEDED(hr), "Recursive SetCooperativeLevel call failed, hr %#x.\n", hr);
            ok(!activateapp_testdata.received, "Received WM_ACTIVATEAPP during recursive SetCooperativeLevel call.\n");
        }
        activateapp_testdata.received = TRUE;
    }

    return DefWindowProcA(hwnd, message, wparam, lparam);
}

static void test_coop_level_activateapp(void)
{
    IDirectDraw4 *ddraw;
    HRESULT hr;
    HWND window;
    WNDCLASSA wc = {0};
    DDSURFACEDESC2 ddsd;
    IDirectDrawSurface4 *surface;

    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    wc.lpfnWndProc = activateapp_test_proc;
    wc.lpszClassName = "ddraw_test_wndproc_wc";
    ok(RegisterClassA(&wc), "Failed to register window class.\n");

    window = CreateWindowA("ddraw_test_wndproc_wc", "ddraw_test",
            WS_MAXIMIZE | WS_CAPTION , 0, 0, 640, 480, 0, 0, 0, 0);

    /* Exclusive with window already active. */
    SetForegroundWindow(window);
    activateapp_testdata.received = FALSE;
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    ok(!activateapp_testdata.received, "Received WM_ACTIVATEAPP although window was already active.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    /* Exclusive with window not active. */
    SetForegroundWindow(GetDesktopWindow());
    activateapp_testdata.received = FALSE;
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    ok(activateapp_testdata.received, "Expected WM_ACTIVATEAPP, but did not receive it.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    /* Normal with window not active, then exclusive with the same window. */
    SetForegroundWindow(GetDesktopWindow());
    activateapp_testdata.received = FALSE;
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    ok(!activateapp_testdata.received, "Received WM_ACTIVATEAPP when setting DDSCL_NORMAL.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    ok(activateapp_testdata.received, "Expected WM_ACTIVATEAPP, but did not receive it.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    /* Recursive set of DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN. */
    SetForegroundWindow(GetDesktopWindow());
    activateapp_testdata.received = FALSE;
    activateapp_testdata.ddraw = ddraw;
    activateapp_testdata.window = window;
    activateapp_testdata.coop_level = DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN;
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    ok(activateapp_testdata.received, "Expected WM_ACTIVATEAPP, but did not receive it.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    /* The recursive call seems to have some bad effect on native ddraw, despite (apparently)
     * succeeding. Another switch to exclusive and back to normal is needed to release the
     * window properly. Without doing this, SetCooperativeLevel(EXCLUSIVE) will not send
     * WM_ACTIVATEAPP messages. */
    activateapp_testdata.ddraw = NULL;
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    /* Setting DDSCL_NORMAL with recursive invocation. */
    SetForegroundWindow(GetDesktopWindow());
    activateapp_testdata.received = FALSE;
    activateapp_testdata.ddraw = ddraw;
    activateapp_testdata.window = window;
    activateapp_testdata.coop_level = DDSCL_NORMAL;
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    ok(activateapp_testdata.received, "Expected WM_ACTIVATEAPP, but did not receive it.\n");

    /* DDraw is in exclusive mode now. */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    U5(ddsd).dwBackBufferCount = 1;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;
    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    IDirectDrawSurface4_Release(surface);

    /* Recover again, just to be sure. */
    activateapp_testdata.ddraw = NULL;
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    DestroyWindow(window);
    UnregisterClassA("ddraw_test_wndproc_wc", GetModuleHandleA(NULL));
    IDirectDraw4_Release(ddraw);
}

static void test_texturemanage(void)
{
    IDirectDraw4 *ddraw;
    HRESULT hr;
    DDSURFACEDESC2 ddsd;
    IDirectDrawSurface4 *surface;
    unsigned int i;
    DDCAPS hal_caps, hel_caps;
    DWORD needed_caps = DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY;
    static const struct
    {
        DWORD caps_in, caps2_in;
        HRESULT hr;
        DWORD caps_out, caps2_out;
    }
    tests[] =
    {
        {DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE, DDSCAPS2_TEXTUREMANAGE, DDERR_INVALIDCAPS,
                ~0U, ~0U},
        {DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE, DDSCAPS2_D3DTEXTUREMANAGE, DDERR_INVALIDCAPS,
                ~0U, ~0U},
        {DDSCAPS_VIDEOMEMORY | DDSCAPS_TEXTURE, DDSCAPS2_TEXTUREMANAGE, DDERR_INVALIDCAPS,
                ~0U, ~0U},
        {DDSCAPS_VIDEOMEMORY | DDSCAPS_TEXTURE, DDSCAPS2_D3DTEXTUREMANAGE, DDERR_INVALIDCAPS,
                ~0U, ~0U},
        {DDSCAPS_TEXTURE, DDSCAPS2_TEXTUREMANAGE, DD_OK,
                DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE, DDSCAPS2_TEXTUREMANAGE},
        {DDSCAPS_TEXTURE, DDSCAPS2_D3DTEXTUREMANAGE, DD_OK,
                DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE, DDSCAPS2_D3DTEXTUREMANAGE},
        {DDSCAPS_VIDEOMEMORY | DDSCAPS_TEXTURE, 0, DD_OK,
                DDSCAPS_VIDEOMEMORY | DDSCAPS_TEXTURE | DDSCAPS_LOCALVIDMEM, 0},
        {DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE, 0, DD_OK,
                DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE, 0},

        {0, DDSCAPS2_TEXTUREMANAGE, DDERR_INVALIDCAPS,
                ~0U, ~0U},
        {0, DDSCAPS2_D3DTEXTUREMANAGE, DDERR_INVALIDCAPS,
                ~0U, ~0U},
        {DDSCAPS_SYSTEMMEMORY, DDSCAPS2_TEXTUREMANAGE, DDERR_INVALIDCAPS,
                ~0U, ~0U},
        {DDSCAPS_SYSTEMMEMORY, DDSCAPS2_D3DTEXTUREMANAGE, DDERR_INVALIDCAPS,
                ~0U, ~0U},
        {DDSCAPS_VIDEOMEMORY, DDSCAPS2_TEXTUREMANAGE, DDERR_INVALIDCAPS,
                ~0U, ~0U},
        {DDSCAPS_VIDEOMEMORY, DDSCAPS2_D3DTEXTUREMANAGE, DDERR_INVALIDCAPS,
                ~0U, ~0U},
        {DDSCAPS_VIDEOMEMORY, 0, DD_OK,
                DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY, 0},
        {DDSCAPS_SYSTEMMEMORY, 0, DD_OK,
                DDSCAPS_SYSTEMMEMORY, 0},
    };

    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&hal_caps, 0, sizeof(hal_caps));
    hal_caps.dwSize = sizeof(hal_caps);
    memset(&hel_caps, 0, sizeof(hel_caps));
    hel_caps.dwSize = sizeof(hel_caps);
    hr = IDirectDraw4_GetCaps(ddraw, &hal_caps, &hel_caps);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);
    if ((hal_caps.ddsCaps.dwCaps & needed_caps) != needed_caps)
    {
        skip("Managed textures not supported, skipping managed texture test.\n");
        IDirectDraw4_Release(ddraw);
        return;
    }

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
        ddsd.ddsCaps.dwCaps = tests[i].caps_in;
        ddsd.ddsCaps.dwCaps2 = tests[i].caps2_in;
        ddsd.dwWidth = 4;
        ddsd.dwHeight = 4;

        hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &surface, NULL);
        ok(hr == tests[i].hr, "Got unexpected, hr %#x, case %u.\n", hr, i);
        if (FAILED(hr))
            continue;

        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        hr = IDirectDrawSurface4_GetSurfaceDesc(surface, &ddsd);
        ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);

        ok(ddsd.ddsCaps.dwCaps == tests[i].caps_out,
                "Input caps %#x, %#x, expected output caps %#x, got %#x, case %u.\n",
                tests[i].caps_in, tests[i].caps2_in, tests[i].caps_out, ddsd.ddsCaps.dwCaps, i);
        ok(ddsd.ddsCaps.dwCaps2 == tests[i].caps2_out,
                "Input caps %#x, %#x, expected output caps %#x, got %#x, case %u.\n",
                tests[i].caps_in, tests[i].caps2_in, tests[i].caps2_out, ddsd.ddsCaps.dwCaps2, i);

        IDirectDrawSurface4_Release(surface);
    }

    IDirectDraw4_Release(ddraw);
}

#define SUPPORT_DXT1    0x01
#define SUPPORT_DXT2    0x02
#define SUPPORT_DXT3    0x04
#define SUPPORT_DXT4    0x08
#define SUPPORT_DXT5    0x10
#define SUPPORT_YUY2    0x20
#define SUPPORT_UYVY    0x40

static HRESULT WINAPI test_block_formats_creation_cb(DDPIXELFORMAT *fmt, void *ctx)
{
    DWORD *supported_fmts = ctx;

    if (!(fmt->dwFlags & DDPF_FOURCC))
        return DDENUMRET_OK;

    switch (fmt->dwFourCC)
    {
        case MAKEFOURCC('D','X','T','1'):
            *supported_fmts |= SUPPORT_DXT1;
            break;
        case MAKEFOURCC('D','X','T','2'):
            *supported_fmts |= SUPPORT_DXT2;
            break;
        case MAKEFOURCC('D','X','T','3'):
            *supported_fmts |= SUPPORT_DXT3;
            break;
        case MAKEFOURCC('D','X','T','4'):
            *supported_fmts |= SUPPORT_DXT4;
            break;
        case MAKEFOURCC('D','X','T','5'):
            *supported_fmts |= SUPPORT_DXT5;
            break;
        case MAKEFOURCC('Y','U','Y','2'):
            *supported_fmts |= SUPPORT_YUY2;
            break;
        case MAKEFOURCC('U','Y','V','Y'):
            *supported_fmts |= SUPPORT_UYVY;
            break;
        default:
            break;
    }

    return DDENUMRET_OK;
}

static void test_block_formats_creation(void)
{
    HRESULT hr, expect_hr;
    unsigned int i, j, w, h;
    HWND window;
    IDirectDraw4 *ddraw;
    IDirect3D3 *d3d;
    IDirect3DDevice3 *device;
    IDirectDrawSurface4 *surface;
    DWORD supported_fmts = 0, supported_overlay_fmts = 0;
    DWORD num_fourcc_codes = 0, *fourcc_codes;
    DDSURFACEDESC2 ddsd;
    DDCAPS hal_caps;
    void *mem;

    static const struct
    {
        DWORD fourcc;
        const char *name;
        DWORD support_flag;
        unsigned int block_width;
        unsigned int block_height;
        unsigned int block_size;
        BOOL create_size_checked, overlay;
    }
    formats[] =
    {
        {MAKEFOURCC('D','X','T','1'), "D3DFMT_DXT1", SUPPORT_DXT1, 4, 4, 8,  TRUE,  FALSE},
        {MAKEFOURCC('D','X','T','2'), "D3DFMT_DXT2", SUPPORT_DXT2, 4, 4, 16, TRUE,  FALSE},
        {MAKEFOURCC('D','X','T','3'), "D3DFMT_DXT3", SUPPORT_DXT3, 4, 4, 16, TRUE,  FALSE},
        {MAKEFOURCC('D','X','T','4'), "D3DFMT_DXT4", SUPPORT_DXT4, 4, 4, 16, TRUE,  FALSE},
        {MAKEFOURCC('D','X','T','5'), "D3DFMT_DXT5", SUPPORT_DXT5, 4, 4, 16, TRUE,  FALSE},
        {MAKEFOURCC('Y','U','Y','2'), "D3DFMT_YUY2", SUPPORT_YUY2, 2, 1, 4,  FALSE, TRUE },
        {MAKEFOURCC('U','Y','V','Y'), "D3DFMT_UYVY", SUPPORT_UYVY, 2, 1, 4,  FALSE, TRUE },
    };
    static const struct
    {
        DWORD caps, caps2;
        const char *name;
        BOOL overlay;
    }
    types[] =
    {
        /* DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY fails to create any fourcc
         * surface with DDERR_INVALIDPIXELFORMAT. Don't care about it for now.
         *
         * Nvidia returns E_FAIL on DXTN DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY.
         * Other hw / drivers successfully create those surfaces. Ignore them, this
         * suggests that no game uses this, otherwise Nvidia would support it. */
        {
            DDSCAPS_VIDEOMEMORY | DDSCAPS_TEXTURE, 0,
            "videomemory texture", FALSE
        },
        {
            DDSCAPS_VIDEOMEMORY | DDSCAPS_OVERLAY, 0,
            "videomemory overlay", TRUE
        },
        {
            DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE, 0,
            "systemmemory texture", FALSE
        },
        {
            DDSCAPS_TEXTURE, DDSCAPS2_TEXTUREMANAGE,
            "managed texture", FALSE
        }
    };
    enum size_type
    {
        SIZE_TYPE_ZERO,
        SIZE_TYPE_PITCH,
        SIZE_TYPE_SIZE,
    };
    static const struct
    {
        DWORD flags;
        enum size_type size_type;
        int rel_size;
        HRESULT hr;
    }
    user_mem_tests[] =
    {
        {DDSD_LINEARSIZE,                               SIZE_TYPE_ZERO,   0, DD_OK},
        {DDSD_LINEARSIZE,                               SIZE_TYPE_SIZE,   0, DD_OK},
        {DDSD_PITCH,                                    SIZE_TYPE_ZERO,   0, DD_OK},
        {DDSD_PITCH,                                    SIZE_TYPE_PITCH,  0, DD_OK},
        {DDSD_LPSURFACE,                                SIZE_TYPE_ZERO,   0, DDERR_INVALIDPARAMS},
        {DDSD_LPSURFACE | DDSD_LINEARSIZE,              SIZE_TYPE_ZERO,   0, DDERR_INVALIDPARAMS},
        {DDSD_LPSURFACE | DDSD_LINEARSIZE,              SIZE_TYPE_PITCH,  0, DDERR_INVALIDPARAMS},
        {DDSD_LPSURFACE | DDSD_LINEARSIZE,              SIZE_TYPE_SIZE,   0, DD_OK},
        {DDSD_LPSURFACE | DDSD_LINEARSIZE,              SIZE_TYPE_SIZE,   1, DD_OK},
        {DDSD_LPSURFACE | DDSD_LINEARSIZE,              SIZE_TYPE_SIZE,  -1, DDERR_INVALIDPARAMS},
        {DDSD_LPSURFACE | DDSD_PITCH,                   SIZE_TYPE_ZERO,   0, DD_OK},
        {DDSD_LPSURFACE | DDSD_PITCH,                   SIZE_TYPE_PITCH,  0, DD_OK},
        {DDSD_LPSURFACE | DDSD_PITCH,                   SIZE_TYPE_SIZE,   0, DD_OK},
        {DDSD_LPSURFACE | DDSD_PITCH | DDSD_LINEARSIZE, SIZE_TYPE_SIZE,   0, DD_OK},
    };

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get d3d interface, hr %#x.\n", hr);
    hr = IDirect3D3_QueryInterface(d3d, &IID_IDirectDraw4, (void **) &ddraw);
    ok(SUCCEEDED(hr), "Failed to get ddraw interface, hr %#x.\n", hr);
    IDirect3D3_Release(d3d);

    hr = IDirect3DDevice3_EnumTextureFormats(device, test_block_formats_creation_cb,
            &supported_fmts);
    ok(SUCCEEDED(hr), "Failed to enumerate texture formats %#x.\n", hr);

    hr = IDirectDraw4_GetFourCCCodes(ddraw, &num_fourcc_codes, NULL);
    ok(SUCCEEDED(hr), "Failed to get fourcc codes %#x.\n", hr);
    fourcc_codes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            num_fourcc_codes * sizeof(*fourcc_codes));
    if (!fourcc_codes)
        goto cleanup;
    hr = IDirectDraw4_GetFourCCCodes(ddraw, &num_fourcc_codes, fourcc_codes);
    ok(SUCCEEDED(hr), "Failed to get fourcc codes %#x.\n", hr);
    for (i = 0; i < num_fourcc_codes; i++)
    {
        for (j = 0; j < ARRAY_SIZE(formats); j++)
        {
            if (fourcc_codes[i] == formats[j].fourcc)
                supported_overlay_fmts |= formats[j].support_flag;
        }
    }
    HeapFree(GetProcessHeap(), 0, fourcc_codes);

    memset(&hal_caps, 0, sizeof(hal_caps));
    hal_caps.dwSize = sizeof(hal_caps);
    hr = IDirectDraw4_GetCaps(ddraw, &hal_caps, NULL);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);

    mem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 2 * 2 * 16 + 1);

    for (i = 0; i < ARRAY_SIZE(formats); i++)
    {
        for (j = 0; j < ARRAY_SIZE(types); j++)
        {
            BOOL support;

            if (formats[i].overlay != types[j].overlay
                    || (types[j].overlay && !(hal_caps.dwCaps & DDCAPS_OVERLAY)))
                continue;

            if (formats[i].overlay)
                support = supported_overlay_fmts & formats[i].support_flag;
            else
                support = supported_fmts & formats[i].support_flag;

            for (w = 1; w <= 8; w++)
            {
                for (h = 1; h <= 8; h++)
                {
                    BOOL block_aligned = TRUE;
                    BOOL todo = FALSE;

                    if (w & (formats[i].block_width - 1) || h & (formats[i].block_height - 1))
                        block_aligned = FALSE;

                    memset(&ddsd, 0, sizeof(ddsd));
                    ddsd.dwSize = sizeof(ddsd);
                    ddsd.dwFlags = DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
                    ddsd.ddsCaps.dwCaps = types[j].caps;
                    ddsd.ddsCaps.dwCaps2 = types[j].caps2;
                    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
                    U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_FOURCC;
                    U4(ddsd).ddpfPixelFormat.dwFourCC = formats[i].fourcc;
                    ddsd.dwWidth = w;
                    ddsd.dwHeight = h;

                    /* TODO: Handle power of two limitations. I cannot test the pow2
                     * behavior on windows because I have no hardware that doesn't at
                     * least support np2_conditional. There's probably no HW that
                     * supports DXTN textures but no conditional np2 textures. */
                    if (!support && !(types[j].caps & DDSCAPS_SYSTEMMEMORY))
                        expect_hr = DDERR_INVALIDPARAMS;
                    else if (formats[i].create_size_checked && !block_aligned)
                    {
                        expect_hr = DDERR_INVALIDPARAMS;
                        if (!(types[j].caps & DDSCAPS_TEXTURE))
                            todo = TRUE;
                    }
                    else
                        expect_hr = D3D_OK;

                    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &surface, NULL);
                    todo_wine_if (todo)
                        ok(hr == expect_hr,
                                "Got unexpected hr %#x for format %s, resource type %s, size %ux%u, expected %#x.\n",
                                hr, formats[i].name, types[j].name, w, h, expect_hr);

                    if (SUCCEEDED(hr))
                        IDirectDrawSurface4_Release(surface);
                }
            }
        }

        if (formats[i].overlay)
            continue;

        for (j = 0; j < ARRAY_SIZE(user_mem_tests); ++j)
        {
            memset(&ddsd, 0, sizeof(ddsd));
            ddsd.dwSize = sizeof(ddsd);
            ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | user_mem_tests[j].flags;
            ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE;

            switch (user_mem_tests[j].size_type)
            {
                case SIZE_TYPE_ZERO:
                    U1(ddsd).dwLinearSize = 0;
                    break;

                case SIZE_TYPE_PITCH:
                    U1(ddsd).dwLinearSize = 2 * formats[i].block_size;
                    break;

                case SIZE_TYPE_SIZE:
                    U1(ddsd).dwLinearSize = 2 * 2 * formats[i].block_size;
                    break;
            }
            U1(ddsd).dwLinearSize += user_mem_tests[j].rel_size;

            ddsd.lpSurface = mem;
            U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
            U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_FOURCC;
            U4(ddsd).ddpfPixelFormat.dwFourCC = formats[i].fourcc;
            ddsd.dwWidth = 8;
            ddsd.dwHeight = 8;

            hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &surface, NULL);
            ok(hr == user_mem_tests[j].hr, "Test %u: Got unexpected hr %#x, format %s.\n", j, hr, formats[i].name);

            if (FAILED(hr))
                continue;

            memset(&ddsd, 0, sizeof(ddsd));
            ddsd.dwSize = sizeof(ddsd);
            hr = IDirectDrawSurface4_GetSurfaceDesc(surface, &ddsd);
            ok(SUCCEEDED(hr), "Test %u: Failed to get surface desc, hr %#x.\n", j, hr);
            ok(ddsd.dwFlags == (DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_LINEARSIZE),
                    "Test %u: Got unexpected flags %#x.\n", j, ddsd.dwFlags);
            if (user_mem_tests[j].flags & DDSD_LPSURFACE)
                ok(U1(ddsd).dwLinearSize == ~0u, "Test %u: Got unexpected linear size %#x.\n",
                        j, U1(ddsd).dwLinearSize);
            else
                ok(U1(ddsd).dwLinearSize == 2 * 2 * formats[i].block_size,
                        "Test %u: Got unexpected linear size %#x, expected %#x.\n",
                        j, U1(ddsd).dwLinearSize, 2 * 2 * formats[i].block_size);
            IDirectDrawSurface4_Release(surface);
        }
    }

    HeapFree(GetProcessHeap(), 0, mem);
cleanup:
    IDirectDraw4_Release(ddraw);
    IDirect3DDevice3_Release(device);
    DestroyWindow(window);
}

struct format_support_check
{
    const DDPIXELFORMAT *format;
    BOOL supported;
};

static HRESULT WINAPI test_unsupported_formats_cb(DDPIXELFORMAT *fmt, void *ctx)
{
    struct format_support_check *format = ctx;

    if (!memcmp(format->format, fmt, sizeof(*fmt)))
    {
        format->supported = TRUE;
        return DDENUMRET_CANCEL;
    }

    return DDENUMRET_OK;
}

static void test_unsupported_formats(void)
{
    HRESULT hr;
    BOOL expect_success;
    HWND window;
    IDirectDraw4 *ddraw;
    IDirect3D3 *d3d;
    IDirect3DDevice3 *device;
    IDirectDrawSurface4 *surface;
    DDSURFACEDESC2 ddsd;
    unsigned int i, j;
    DWORD expected_caps;
    static const struct
    {
        const char *name;
        DDPIXELFORMAT fmt;
    }
    formats[] =
    {
        {
            "D3DFMT_A8R8G8B8",
            {
                sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_ALPHAPIXELS, 0,
                {32}, {0x00ff0000}, {0x0000ff00}, {0x000000ff}, {0xff000000}
            }
        },
        {
            "D3DFMT_P8",
            {
                sizeof(DDPIXELFORMAT), DDPF_PALETTEINDEXED8 | DDPF_RGB, 0,
                {8 }, {0x00000000}, {0x00000000}, {0x00000000}, {0x00000000}
            }
        },
    };
    static const DWORD caps[] = {0, DDSCAPS_SYSTEMMEMORY, DDSCAPS_VIDEOMEMORY};

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get d3d interface, hr %#x.\n", hr);
    hr = IDirect3D3_QueryInterface(d3d, &IID_IDirectDraw4, (void **) &ddraw);
    ok(SUCCEEDED(hr), "Failed to get ddraw interface, hr %#x.\n", hr);
    IDirect3D3_Release(d3d);

    for (i = 0; i < ARRAY_SIZE(formats); i++)
    {
        struct format_support_check check = {&formats[i].fmt, FALSE};
        hr = IDirect3DDevice3_EnumTextureFormats(device, test_unsupported_formats_cb, &check);
        ok(SUCCEEDED(hr), "Failed to enumerate texture formats %#x.\n", hr);

        for (j = 0; j < ARRAY_SIZE(caps); j++)
        {
            memset(&ddsd, 0, sizeof(ddsd));
            ddsd.dwSize = sizeof(ddsd);
            ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
            U4(ddsd).ddpfPixelFormat = formats[i].fmt;
            ddsd.dwWidth = 4;
            ddsd.dwHeight = 4;
            ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | caps[j];

            if (caps[j] & DDSCAPS_VIDEOMEMORY && !check.supported)
                expect_success = FALSE;
            else
                expect_success = TRUE;

            hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &surface, NULL);
            ok(SUCCEEDED(hr) == expect_success,
                    "Got unexpected hr %#x for format %s, caps %#x, expected %s.\n",
                    hr, formats[i].name, caps[j], expect_success ? "success" : "failure");
            if (FAILED(hr))
                continue;

            memset(&ddsd, 0, sizeof(ddsd));
            ddsd.dwSize = sizeof(ddsd);
            hr = IDirectDrawSurface4_GetSurfaceDesc(surface, &ddsd);
            ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);

            if (caps[j] & DDSCAPS_VIDEOMEMORY)
                expected_caps = DDSCAPS_VIDEOMEMORY;
            else if (caps[j] & DDSCAPS_SYSTEMMEMORY)
                expected_caps = DDSCAPS_SYSTEMMEMORY;
            else if (check.supported)
                expected_caps = DDSCAPS_VIDEOMEMORY;
            else
                expected_caps = DDSCAPS_SYSTEMMEMORY;

            ok(ddsd.ddsCaps.dwCaps & expected_caps,
                    "Expected capability %#x, format %s, input cap %#x.\n",
                    expected_caps, formats[i].name, caps[j]);

            IDirectDrawSurface4_Release(surface);
        }
    }

    IDirectDraw4_Release(ddraw);
    IDirect3DDevice3_Release(device);
    DestroyWindow(window);
}

static void test_rt_caps(void)
{
    PALETTEENTRY palette_entries[256];
    IDirectDrawPalette *palette;
    IDirectDraw4 *ddraw;
    DDPIXELFORMAT z_fmt;
    IDirect3D3 *d3d;
    unsigned int i;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    static const DDPIXELFORMAT p8_fmt =
    {
        sizeof(DDPIXELFORMAT), DDPF_PALETTEINDEXED8 | DDPF_RGB, 0,
        {8}, {0x00000000}, {0x00000000}, {0x00000000}, {0x00000000},
    };

    const struct
    {
        const DDPIXELFORMAT *pf;
        DWORD caps_in;
        DWORD caps_out;
        HRESULT create_device_hr;
        HRESULT set_rt_hr, alternative_set_rt_hr;
    }
    test_data[] =
    {
        {
            NULL,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
            D3D_OK,
            D3D_OK,
            D3D_OK,
        },
        {
            NULL,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
            D3D_OK,
            D3D_OK,
            D3D_OK,
        },
        {
            NULL,
            DDSCAPS_OFFSCREENPLAIN,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
        },
        {
            NULL,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY | DDSCAPS_3DDEVICE,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY | DDSCAPS_3DDEVICE,
            D3DERR_SURFACENOTINVIDMEM,
            D3D_OK,
            D3D_OK,
        },
        {
            NULL,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
        },
        {
            NULL,
            DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY,
            DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
            D3D_OK,
            D3D_OK,
            D3D_OK,
        },
        {
            NULL,
            DDSCAPS_3DDEVICE,
            DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
            D3D_OK,
            D3D_OK,
            D3D_OK,
        },
        {
            NULL,
            0,
            DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
        },
        {
            NULL,
            DDSCAPS_SYSTEMMEMORY | DDSCAPS_3DDEVICE,
            DDSCAPS_SYSTEMMEMORY | DDSCAPS_3DDEVICE,
            D3DERR_SURFACENOTINVIDMEM,
            D3D_OK,
            D3D_OK,
        },
        {
            NULL,
            DDSCAPS_SYSTEMMEMORY,
            DDSCAPS_SYSTEMMEMORY,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
        },
        {
            &p8_fmt,
            0,
            DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
        },
        {
            &p8_fmt,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE,
            ~0U /* AMD r200 */,
            DDERR_NOPALETTEATTACHED,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
        },
        {
            &p8_fmt,
            DDSCAPS_OFFSCREENPLAIN,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
        },
        {
            &p8_fmt,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY | DDSCAPS_3DDEVICE,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY | DDSCAPS_3DDEVICE,
            DDERR_NOPALETTEATTACHED,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
        },
        {
            &p8_fmt,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
        },
        {
            &z_fmt,
            DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY | DDSCAPS_ZBUFFER,
            DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY | DDSCAPS_ZBUFFER | DDSCAPS_LOCALVIDMEM,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDPIXELFORMAT,
            D3D_OK /* r200 */,
        },
        {
            &z_fmt,
            DDSCAPS_3DDEVICE | DDSCAPS_ZBUFFER,
            DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY | DDSCAPS_ZBUFFER | DDSCAPS_LOCALVIDMEM,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDPIXELFORMAT,
            D3D_OK /* r200 */,
        },
        {
            &z_fmt,
            DDSCAPS_ZBUFFER,
            DDSCAPS_VIDEOMEMORY | DDSCAPS_ZBUFFER | DDSCAPS_LOCALVIDMEM,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
        },
        {
            &z_fmt,
            DDSCAPS_SYSTEMMEMORY | DDSCAPS_3DDEVICE | DDSCAPS_ZBUFFER,
            DDSCAPS_SYSTEMMEMORY | DDSCAPS_3DDEVICE | DDSCAPS_ZBUFFER,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDPIXELFORMAT,
            D3D_OK /* r200 */,
        },
        {
            &z_fmt,
            DDSCAPS_SYSTEMMEMORY | DDSCAPS_ZBUFFER,
            DDSCAPS_SYSTEMMEMORY | DDSCAPS_ZBUFFER,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
            DDERR_INVALIDCAPS,
        },
    };

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    if (FAILED(IDirectDraw4_QueryInterface(ddraw, &IID_IDirect3D3, (void **)&d3d)))
    {
        skip("D3D interface is not available, skipping test.\n");
        goto done;
    }

    memset(&z_fmt, 0, sizeof(z_fmt));
    hr = IDirect3D3_EnumZBufferFormats(d3d, &IID_IDirect3DHALDevice, enum_z_fmt, &z_fmt);
    if (FAILED(hr) || !z_fmt.dwSize)
    {
        skip("No depth buffer formats available, skipping test.\n");
        IDirect3D3_Release(d3d);
        goto done;
    }

    memset(palette_entries, 0, sizeof(palette_entries));
    hr = IDirectDraw4_CreatePalette(ddraw, DDPCAPS_ALLOW256 | DDPCAPS_8BIT, palette_entries, &palette, NULL);
    ok(SUCCEEDED(hr), "Failed to create palette, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(test_data); ++i)
    {
        IDirectDrawSurface4 *surface, *rt, *expected_rt, *tmp;
        DDSURFACEDESC2 surface_desc;
        IDirect3DDevice3 *device;

        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        surface_desc.ddsCaps.dwCaps = test_data[i].caps_in;
        if (test_data[i].pf)
        {
            surface_desc.dwFlags |= DDSD_PIXELFORMAT;
            U4(surface_desc).ddpfPixelFormat = *test_data[i].pf;
        }
        surface_desc.dwWidth = 640;
        surface_desc.dwHeight = 480;
        hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
        ok(SUCCEEDED(hr), "Test %u: Failed to create surface with caps %#x, hr %#x.\n",
                i, test_data[i].caps_in, hr);

        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        hr = IDirectDrawSurface4_GetSurfaceDesc(surface, &surface_desc);
        ok(SUCCEEDED(hr), "Test %u: Failed to get surface desc, hr %#x.\n", i, hr);
        ok(test_data[i].caps_out == ~0U || surface_desc.ddsCaps.dwCaps == test_data[i].caps_out,
                "Test %u: Got unexpected caps %#x, expected %#x.\n",
                i, surface_desc.ddsCaps.dwCaps, test_data[i].caps_out);

        hr = IDirect3D3_CreateDevice(d3d, &IID_IDirect3DHALDevice, surface, &device, NULL);
        ok(hr == test_data[i].create_device_hr, "Test %u: Got unexpected hr %#x, expected %#x.\n",
                i, hr, test_data[i].create_device_hr);
        if (FAILED(hr))
        {
            if (hr == DDERR_NOPALETTEATTACHED)
            {
                hr = IDirectDrawSurface4_SetPalette(surface, palette);
                ok(SUCCEEDED(hr), "Test %u: Failed to set palette, hr %#x.\n", i, hr);
                hr = IDirect3D3_CreateDevice(d3d, &IID_IDirect3DHALDevice, surface, &device, NULL);
                if (surface_desc.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY)
                    ok(hr == DDERR_INVALIDPIXELFORMAT, "Test %u: Got unexpected hr %#x.\n", i, hr);
                else
                    ok(hr == D3DERR_SURFACENOTINVIDMEM, "Test %u: Got unexpected hr %#x.\n", i, hr);
            }
            IDirectDrawSurface4_Release(surface);

            memset(&surface_desc, 0, sizeof(surface_desc));
            surface_desc.dwSize = sizeof(surface_desc);
            surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
            surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
            surface_desc.dwWidth = 640;
            surface_desc.dwHeight = 480;
            hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
            ok(SUCCEEDED(hr), "Test %u: Failed to create surface, hr %#x.\n", i, hr);

            hr = IDirect3D3_CreateDevice(d3d, &IID_IDirect3DHALDevice, surface, &device, NULL);
            ok(SUCCEEDED(hr), "Test %u: Failed to create device, hr %#x.\n", i, hr);
        }

        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        surface_desc.ddsCaps.dwCaps = test_data[i].caps_in;
        if (test_data[i].pf)
        {
            surface_desc.dwFlags |= DDSD_PIXELFORMAT;
            U4(surface_desc).ddpfPixelFormat = *test_data[i].pf;
        }
        surface_desc.dwWidth = 640;
        surface_desc.dwHeight = 480;
        hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &rt, NULL);
        ok(SUCCEEDED(hr), "Test %u: Failed to create surface with caps %#x, hr %#x.\n",
                i, test_data[i].caps_in, hr);

        hr = IDirect3DDevice3_SetRenderTarget(device, rt, 0);
        ok(hr == test_data[i].set_rt_hr || broken(hr == test_data[i].alternative_set_rt_hr),
                "Test %u: Got unexpected hr %#x, expected %#x.\n",
                i, hr, test_data[i].set_rt_hr);
        if (SUCCEEDED(hr) || hr == DDERR_INVALIDPIXELFORMAT)
            expected_rt = rt;
        else
            expected_rt = surface;

        hr = IDirect3DDevice3_GetRenderTarget(device, &tmp);
        ok(SUCCEEDED(hr), "Test %u: Failed to get render target, hr %#x.\n", i, hr);
        ok(tmp == expected_rt, "Test %u: Got unexpected rt %p.\n", i, tmp);

        IDirectDrawSurface4_Release(tmp);
        IDirectDrawSurface4_Release(rt);
        refcount = IDirect3DDevice3_Release(device);
        ok(refcount == 0, "Test %u: The device was not properly freed, refcount %u.\n", i, refcount);
        refcount = IDirectDrawSurface4_Release(surface);
        ok(refcount == 0, "Test %u: The surface was not properly freed, refcount %u.\n", i, refcount);
    }

    IDirectDrawPalette_Release(palette);
    IDirect3D3_Release(d3d);

done:
    refcount = IDirectDraw4_Release(ddraw);
    ok(refcount == 0, "The ddraw object was not properly freed, refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_primary_caps(void)
{
    const DWORD placement = DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY | DDSCAPS_SYSTEMMEMORY;
    IDirectDrawSurface4 *surface;
    DDSURFACEDESC2 surface_desc;
    IDirectDraw4 *ddraw;
    unsigned int i;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    static const struct
    {
        DWORD coop_level;
        DWORD caps_in;
        DWORD back_buffer_count;
        HRESULT hr;
        DWORD caps_out;
    }
    test_data[] =
    {
        {
            DDSCL_NORMAL,
            DDSCAPS_PRIMARYSURFACE,
            ~0u,
            DD_OK,
            DDSCAPS_VISIBLE | DDSCAPS_PRIMARYSURFACE,
        },
        {
            DDSCL_NORMAL,
            DDSCAPS_PRIMARYSURFACE | DDSCAPS_TEXTURE,
            ~0u,
            DDERR_INVALIDCAPS,
            ~0u,
        },
        {
            DDSCL_NORMAL,
            DDSCAPS_PRIMARYSURFACE | DDSCAPS_FRONTBUFFER,
            ~0u,
            DDERR_INVALIDCAPS,
            ~0u,
        },
        {
            DDSCL_NORMAL,
            DDSCAPS_PRIMARYSURFACE | DDSCAPS_BACKBUFFER,
            ~0u,
            DDERR_INVALIDCAPS,
            ~0u,
        },
        {
            DDSCL_NORMAL,
            DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP,
            ~0u,
            DDERR_INVALIDCAPS,
            ~0u,
        },
        {
            DDSCL_NORMAL,
            DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX,
            ~0u,
            DDERR_INVALIDCAPS,
            ~0u,
        },
        {
            DDSCL_NORMAL,
            DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP,
            ~0u,
            DDERR_INVALIDCAPS,
            ~0u,
        },
        {
            DDSCL_NORMAL,
            DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP,
            0,
            DDERR_INVALIDCAPS,
            ~0u,
        },
        {
            DDSCL_NORMAL,
            DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP,
            1,
            DDERR_NOEXCLUSIVEMODE,
            ~0u,
        },
        {
            DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN,
            DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP,
            0,
            DDERR_INVALIDCAPS,
            ~0u,
        },
        {
            DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN,
            DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP,
            1,
            DD_OK,
            DDSCAPS_VISIBLE | DDSCAPS_PRIMARYSURFACE | DDSCAPS_FRONTBUFFER | DDSCAPS_FLIP | DDSCAPS_COMPLEX,
        },
        {
            DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN,
            DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP | DDSCAPS_FRONTBUFFER,
            1,
            DDERR_INVALIDCAPS,
            ~0u,
        },
        {
            DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN,
            DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP | DDSCAPS_BACKBUFFER,
            1,
            DDERR_INVALIDCAPS,
            ~0u,
        },
    };

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    for (i = 0; i < ARRAY_SIZE(test_data); ++i)
    {
        hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, test_data[i].coop_level);
        ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS;
        if (test_data[i].back_buffer_count != ~0u)
            surface_desc.dwFlags |= DDSD_BACKBUFFERCOUNT;
        surface_desc.ddsCaps.dwCaps = test_data[i].caps_in;
        U5(surface_desc).dwBackBufferCount = test_data[i].back_buffer_count;
        hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
        ok(hr == test_data[i].hr, "Test %u: Got unexpected hr %#x, expected %#x.\n", i, hr, test_data[i].hr);
        if (FAILED(hr))
            continue;

        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        hr = IDirectDrawSurface4_GetSurfaceDesc(surface, &surface_desc);
        ok(SUCCEEDED(hr), "Test %u: Failed to get surface desc, hr %#x.\n", i, hr);
        ok((surface_desc.ddsCaps.dwCaps & ~placement) == test_data[i].caps_out,
                "Test %u: Got unexpected caps %#x, expected %#x.\n",
                i, surface_desc.ddsCaps.dwCaps, test_data[i].caps_out);

        IDirectDrawSurface4_Release(surface);
    }

    refcount = IDirectDraw4_Release(ddraw);
    ok(refcount == 0, "The ddraw object was not properly freed, refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_surface_lock(void)
{
    IDirectDraw4 *ddraw;
    IDirect3D3 *d3d = NULL;
    IDirectDrawSurface4 *surface;
    HRESULT hr;
    HWND window;
    unsigned int i;
    DDSURFACEDESC2 ddsd;
    ULONG refcount;
    DDPIXELFORMAT z_fmt;
    static const struct
    {
        DWORD caps;
        DWORD caps2;
        const char *name;
    }
    tests[] =
    {
        {
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY,
            0,
            "videomemory offscreenplain"
        },
        {
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY,
            0,
            "systemmemory offscreenplain"
        },
        {
            DDSCAPS_PRIMARYSURFACE,
            0,
            "primary"
        },
        {
            DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY,
            0,
            "videomemory texture"
        },
        {
            DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY,
            DDSCAPS2_OPAQUE,
            "opaque videomemory texture"
        },
        {
            DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY,
            0,
            "systemmemory texture"
        },
        {
            DDSCAPS_TEXTURE,
            DDSCAPS2_TEXTUREMANAGE,
            "managed texture"
        },
        {
            DDSCAPS_TEXTURE,
            DDSCAPS2_D3DTEXTUREMANAGE,
            "managed texture"
        },
        {
            DDSCAPS_TEXTURE,
            DDSCAPS2_TEXTUREMANAGE | DDSCAPS2_OPAQUE,
            "opaque managed texture"
        },
        {
            DDSCAPS_TEXTURE,
            DDSCAPS2_D3DTEXTUREMANAGE | DDSCAPS2_OPAQUE,
            "opaque managed texture"
        },
        {
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE,
            0,
            "render target"
        },
        {
            DDSCAPS_ZBUFFER,
            0,
            "Z buffer"
        },
    };

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    if (FAILED(IDirectDraw4_QueryInterface(ddraw, &IID_IDirect3D3, (void **)&d3d)))
    {
        skip("D3D interface is not available, skipping test.\n");
        goto done;
    }

    memset(&z_fmt, 0, sizeof(z_fmt));
    hr = IDirect3D3_EnumZBufferFormats(d3d, &IID_IDirect3DHALDevice, enum_z_fmt, &z_fmt);
    if (FAILED(hr) || !z_fmt.dwSize)
    {
        skip("No depth buffer formats available, skipping test.\n");
        goto done;
    }

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS;
        if (!(tests[i].caps & DDSCAPS_PRIMARYSURFACE))
        {
            ddsd.dwFlags |= DDSD_WIDTH | DDSD_HEIGHT;
            ddsd.dwWidth = 64;
            ddsd.dwHeight = 64;
        }
        if (tests[i].caps & DDSCAPS_ZBUFFER)
        {
            ddsd.dwFlags |= DDSD_PIXELFORMAT;
            U4(ddsd).ddpfPixelFormat = z_fmt;
        }
        ddsd.ddsCaps.dwCaps = tests[i].caps;
        ddsd.ddsCaps.dwCaps2 = tests[i].caps2;

        hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &surface, NULL);
        ok(SUCCEEDED(hr), "Failed to create surface, type %s, hr %#x.\n", tests[i].name, hr);

        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        hr = IDirectDrawSurface4_Lock(surface, NULL, &ddsd, DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Failed to lock surface, type %s, hr %#x.\n", tests[i].name, hr);
        if (SUCCEEDED(hr))
        {
            hr = IDirectDrawSurface4_Unlock(surface, NULL);
            ok(SUCCEEDED(hr), "Failed to unlock surface, type %s, hr %#x.\n", tests[i].name, hr);
        }

        memset(&ddsd, 0, sizeof(ddsd));
        hr = IDirectDrawSurface4_Lock(surface, NULL, &ddsd, DDLOCK_WAIT, NULL);
        ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x, type %s.\n", hr, tests[i].name);

        IDirectDrawSurface4_Release(surface);
    }

done:
    if (d3d)
        IDirect3D3_Release(d3d);
    refcount = IDirectDraw4_Release(ddraw);
    ok(refcount == 0, "The ddraw object was not properly freed, refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_surface_discard(void)
{
    IDirect3DDevice3 *device;
    IDirect3D3 *d3d;
    IDirectDraw4 *ddraw;
    HRESULT hr;
    HWND window;
    DDSURFACEDESC2 ddsd;
    IDirectDrawSurface4 *surface, *target;
    void *addr;
    static const struct
    {
        DWORD caps, caps2;
        BOOL discard;
    }
    tests[] =
    {
        {DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY, 0, TRUE},
        {DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY, 0, FALSE},
        {DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY, 0, TRUE},
        {DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY, 0, FALSE},
        {DDSCAPS_TEXTURE, DDSCAPS2_TEXTUREMANAGE, FALSE},
        {DDSCAPS_TEXTURE, DDSCAPS2_TEXTUREMANAGE | DDSCAPS2_HINTDYNAMIC, FALSE},
        {DDSCAPS_TEXTURE, DDSCAPS2_D3DTEXTUREMANAGE, FALSE},
        {DDSCAPS_TEXTURE, DDSCAPS2_D3DTEXTUREMANAGE | DDSCAPS2_HINTDYNAMIC, FALSE},
    };
    unsigned int i;

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }
    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get d3d interface, hr %#x.\n", hr);
    hr = IDirect3D3_QueryInterface(d3d, &IID_IDirectDraw4, (void **)&ddraw);
    ok(SUCCEEDED(hr), "Failed to get ddraw interface, hr %#x.\n", hr);
    hr = IDirect3DDevice3_GetRenderTarget(device, &target);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        BOOL discarded;

        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        ddsd.ddsCaps.dwCaps = tests[i].caps;
        ddsd.ddsCaps.dwCaps2 = tests[i].caps2;
        ddsd.dwWidth = 64;
        ddsd.dwHeight = 64;
        hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &surface, NULL);
        ok(SUCCEEDED(hr), "Failed to create offscreen surface, hr %#x, case %u.\n", hr, i);

        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        hr = IDirectDrawSurface4_Lock(surface, NULL, &ddsd, 0, NULL);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
        addr = ddsd.lpSurface;
        hr = IDirectDrawSurface4_Unlock(surface, NULL);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        hr = IDirectDrawSurface4_Lock(surface, NULL, &ddsd, DDLOCK_DISCARDCONTENTS, NULL);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
        discarded = ddsd.lpSurface != addr;
        hr = IDirectDrawSurface4_Unlock(surface, NULL);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

        hr = IDirectDrawSurface4_Blt(target, NULL, surface, NULL, DDBLT_WAIT, NULL);
        ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);

        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        hr = IDirectDrawSurface4_Lock(surface, NULL, &ddsd, DDLOCK_DISCARDCONTENTS, NULL);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
        discarded |= ddsd.lpSurface != addr;
        hr = IDirectDrawSurface4_Unlock(surface, NULL);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

        IDirectDrawSurface4_Release(surface);

        /* Windows 7 reliably changes the address of surfaces that are discardable (Nvidia Kepler,
         * AMD r500, evergreen). Windows XP, at least on AMD r200, does not. */
        ok(!discarded || tests[i].discard, "Expected surface not to be discarded, case %u\n", i);
    }

    IDirectDrawSurface4_Release(target);
    IDirectDraw4_Release(ddraw);
    IDirect3D3_Release(d3d);
    IDirect3DDevice3_Release(device);
    DestroyWindow(window);
}

static void fill_surface(IDirectDrawSurface4 *surface, D3DCOLOR color)
{
    DDSURFACEDESC2 surface_desc = {sizeof(surface_desc)};
    HRESULT hr;
    unsigned int x, y;
    DWORD *ptr;

    hr = IDirectDrawSurface4_Lock(surface, NULL, &surface_desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);

    for (y = 0; y < surface_desc.dwHeight; ++y)
    {
        ptr = (DWORD *)((BYTE *)surface_desc.lpSurface + y * surface_desc.lPitch);
        for (x = 0; x < surface_desc.dwWidth; ++x)
        {
            ptr[x] = color;
        }
    }

    hr = IDirectDrawSurface4_Unlock(surface, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
}

static void test_flip(void)
{
    const DWORD placement = DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY | DDSCAPS_SYSTEMMEMORY;
    IDirectDrawSurface4 *frontbuffer, *backbuffer1, *backbuffer2, *backbuffer3, *surface;
    DDSCAPS2 caps = {DDSCAPS_FLIP, 0, 0, {0}};
    DDSURFACEDESC2 surface_desc;
    BOOL sysmem_primary;
    IDirectDraw4 *ddraw;
    DWORD expected_caps;
    unsigned int i;
    D3DCOLOR color;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    static const struct
    {
        const char *name;
        DWORD caps;
    }
    test_data[] =
    {
        {"PRIMARYSURFACE", DDSCAPS_PRIMARYSURFACE},
        {"OFFSCREENPLAIN", DDSCAPS_OFFSCREENPLAIN},
        {"TEXTURE",        DDSCAPS_TEXTURE},
    };

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(test_data); ++i)
    {
        /* Creating a flippable texture induces a BSoD on some versions of the
         * Intel graphics driver. At least Intel GMA 950 with driver version
         * 6.14.10.4926 on Windows XP SP3 is affected. */
        if ((test_data[i].caps & DDSCAPS_TEXTURE) && ddraw_is_intel(ddraw))
        {
            win_skip("Skipping flippable texture test.\n");
            continue;
        }

        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS;
        if (!(test_data[i].caps & DDSCAPS_PRIMARYSURFACE))
            surface_desc.dwFlags |= DDSD_WIDTH | DDSD_HEIGHT;
        surface_desc.ddsCaps.dwCaps = DDSCAPS_COMPLEX | DDSCAPS_FLIP | test_data[i].caps;
        surface_desc.dwWidth = 512;
        surface_desc.dwHeight = 512;
        U5(surface_desc).dwBackBufferCount = 3;
        hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &frontbuffer, NULL);
        ok(hr == DDERR_INVALIDCAPS, "%s: Got unexpected hr %#x.\n", test_data[i].name, hr);

        surface_desc.ddsCaps.dwCaps &= ~DDSCAPS_FLIP;
        surface_desc.dwFlags |= DDSD_BACKBUFFERCOUNT;
        hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &frontbuffer, NULL);
        ok(hr == DDERR_INVALIDCAPS, "%s: Got unexpected hr %#x.\n", test_data[i].name, hr);

        surface_desc.ddsCaps.dwCaps &= ~DDSCAPS_COMPLEX;
        surface_desc.ddsCaps.dwCaps |= DDSCAPS_FLIP;
        hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &frontbuffer, NULL);
        ok(hr == DDERR_INVALIDCAPS, "%s: Got unexpected hr %#x.\n", test_data[i].name, hr);

        surface_desc.ddsCaps.dwCaps |= DDSCAPS_COMPLEX;
        hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &frontbuffer, NULL);
        todo_wine_if(test_data[i].caps & DDSCAPS_TEXTURE)
            ok(SUCCEEDED(hr), "%s: Failed to create surface, hr %#x.\n", test_data[i].name, hr);
        if (FAILED(hr))
            continue;

        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        hr = IDirectDrawSurface4_GetSurfaceDesc(frontbuffer, &surface_desc);
        ok(SUCCEEDED(hr), "%s: Failed to get surface desc, hr %#x.\n", test_data[i].name, hr);
        expected_caps = DDSCAPS_FRONTBUFFER | DDSCAPS_COMPLEX | DDSCAPS_FLIP | test_data[i].caps;
        if (test_data[i].caps & DDSCAPS_PRIMARYSURFACE)
            expected_caps |= DDSCAPS_VISIBLE;
        ok((surface_desc.ddsCaps.dwCaps & ~placement) == expected_caps,
                "%s: Got unexpected caps %#x.\n", test_data[i].name, surface_desc.ddsCaps.dwCaps);
        sysmem_primary = surface_desc.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY;

        hr = IDirectDrawSurface4_GetAttachedSurface(frontbuffer, &caps, &backbuffer1);
        ok(SUCCEEDED(hr), "%s: Failed to get attached surface, hr %#x.\n", test_data[i].name, hr);
        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        hr = IDirectDrawSurface4_GetSurfaceDesc(backbuffer1, &surface_desc);
        ok(SUCCEEDED(hr), "%s: Failed to get surface desc, hr %#x.\n", test_data[i].name, hr);
        ok(!U5(surface_desc).dwBackBufferCount, "%s: Got unexpected back buffer count %u.\n",
                test_data[i].name, U5(surface_desc).dwBackBufferCount);
        expected_caps &= ~(DDSCAPS_VISIBLE | DDSCAPS_PRIMARYSURFACE | DDSCAPS_FRONTBUFFER);
        expected_caps |= DDSCAPS_BACKBUFFER;
        ok((surface_desc.ddsCaps.dwCaps & ~placement) == expected_caps,
                "%s: Got unexpected caps %#x.\n", test_data[i].name, surface_desc.ddsCaps.dwCaps);

        hr = IDirectDrawSurface4_GetAttachedSurface(backbuffer1, &caps, &backbuffer2);
        ok(SUCCEEDED(hr), "%s: Failed to get attached surface, hr %#x.\n", test_data[i].name, hr);
        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        hr = IDirectDrawSurface4_GetSurfaceDesc(backbuffer2, &surface_desc);
        ok(SUCCEEDED(hr), "%s: Failed to get surface desc, hr %#x.\n", test_data[i].name, hr);
        ok(!U5(surface_desc).dwBackBufferCount, "%s: Got unexpected back buffer count %u.\n",
                test_data[i].name, U5(surface_desc).dwBackBufferCount);
        expected_caps &= ~DDSCAPS_BACKBUFFER;
        ok((surface_desc.ddsCaps.dwCaps & ~placement) == expected_caps,
                "%s: Got unexpected caps %#x.\n", test_data[i].name, surface_desc.ddsCaps.dwCaps);

        hr = IDirectDrawSurface4_GetAttachedSurface(backbuffer2, &caps, &backbuffer3);
        ok(SUCCEEDED(hr), "%s: Failed to get attached surface, hr %#x.\n", test_data[i].name, hr);
        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        hr = IDirectDrawSurface4_GetSurfaceDesc(backbuffer3, &surface_desc);
        ok(SUCCEEDED(hr), "%s: Failed to get surface desc, hr %#x.\n", test_data[i].name, hr);
        ok(!U5(surface_desc).dwBackBufferCount, "%s: Got unexpected back buffer count %u.\n",
                test_data[i].name, U5(surface_desc).dwBackBufferCount);
        ok((surface_desc.ddsCaps.dwCaps & ~placement) == expected_caps,
                "%s: Got unexpected caps %#x.\n", test_data[i].name, surface_desc.ddsCaps.dwCaps);

        hr = IDirectDrawSurface4_GetAttachedSurface(backbuffer3, &caps, &surface);
        ok(SUCCEEDED(hr), "%s: Failed to get attached surface, hr %#x.\n", test_data[i].name, hr);
        ok(surface == frontbuffer, "%s: Got unexpected surface %p, expected %p.\n",
                test_data[i].name, surface, frontbuffer);
        IDirectDrawSurface4_Release(surface);

        hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL | DDSCL_FULLSCREEN);
        ok(SUCCEEDED(hr), "%s: Failed to set cooperative level, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_IsLost(frontbuffer);
        ok(hr == DD_OK, "%s: Got unexpected hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_Flip(frontbuffer, NULL, DDFLIP_WAIT);
        if (test_data[i].caps & DDSCAPS_PRIMARYSURFACE)
            ok(hr == DDERR_NOEXCLUSIVEMODE, "%s: Got unexpected hr %#x.\n", test_data[i].name, hr);
        else
            ok(SUCCEEDED(hr), "%s: Failed to flip, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
        ok(SUCCEEDED(hr), "%s: Failed to set cooperative level, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_IsLost(frontbuffer);
        todo_wine ok(hr == DDERR_SURFACELOST, "%s: Got unexpected hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDraw4_RestoreAllSurfaces(ddraw);
        ok(SUCCEEDED(hr), "%s: Failed to restore surfaces, hr %#x.\n", test_data[i].name, hr);

        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        surface_desc.ddsCaps.dwCaps = 0;
        surface_desc.dwWidth = 640;
        surface_desc.dwHeight = 480;
        hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
        ok(SUCCEEDED(hr), "%s: Failed to create surface, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_Flip(frontbuffer, surface, DDFLIP_WAIT);
        ok(hr == DDERR_NOTFLIPPABLE, "%s: Got unexpected hr %#x.\n", test_data[i].name, hr);
        IDirectDrawSurface4_Release(surface);

        hr = IDirectDrawSurface4_Flip(frontbuffer, frontbuffer, DDFLIP_WAIT);
        ok(hr == DDERR_NOTFLIPPABLE, "%s: Got unexpected hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_Flip(backbuffer1, NULL, DDFLIP_WAIT);
        ok(hr == DDERR_NOTFLIPPABLE, "%s: Got unexpected hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_Flip(backbuffer2, NULL, DDFLIP_WAIT);
        ok(hr == DDERR_NOTFLIPPABLE, "%s: Got unexpected hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_Flip(backbuffer3, NULL, DDFLIP_WAIT);
        ok(hr == DDERR_NOTFLIPPABLE, "%s: Got unexpected hr %#x.\n", test_data[i].name, hr);

        /* The Nvidia Geforce 7 driver cannot do a color fill on a texture backbuffer after
         * the backbuffer has been locked. Do it ourselves as a workaround. Unlike ddraw1
         * and 2 GetSurfaceDesc does not cause issues in ddraw4 and ddraw7. */
        fill_surface(backbuffer1, 0xffff0000);
        fill_surface(backbuffer2, 0xff00ff00);
        fill_surface(backbuffer3, 0xff0000ff);

        hr = IDirectDrawSurface4_Flip(frontbuffer, NULL, DDFLIP_WAIT);
        ok(SUCCEEDED(hr), "%s: Failed to flip, hr %#x.\n", test_data[i].name, hr);
        color = get_surface_color(backbuffer1, 320, 240);
        /* The testbot seems to just copy the contents of one surface to all the
         * others, instead of properly flipping. */
        ok(compare_color(color, 0x0000ff00, 1) || broken(sysmem_primary && compare_color(color, 0x000000ff, 1)),
                "%s: Got unexpected color 0x%08x.\n", test_data[i].name, color);
        color = get_surface_color(backbuffer2, 320, 240);
        ok(compare_color(color, 0x000000ff, 1), "%s: Got unexpected color 0x%08x.\n", test_data[i].name, color);
        fill_surface(backbuffer3, 0xffff0000);

        hr = IDirectDrawSurface4_Flip(frontbuffer, NULL, DDFLIP_WAIT);
        ok(SUCCEEDED(hr), "%s: Failed to flip, hr %#x.\n", test_data[i].name, hr);
        color = get_surface_color(backbuffer1, 320, 240);
        ok(compare_color(color, 0x000000ff, 1) || broken(sysmem_primary && compare_color(color, 0x00ff0000, 1)),
                "%s: Got unexpected color 0x%08x.\n", test_data[i].name, color);
        color = get_surface_color(backbuffer2, 320, 240);
        ok(compare_color(color, 0x00ff0000, 1), "%s: Got unexpected color 0x%08x.\n", test_data[i].name, color);
        fill_surface(backbuffer3, 0xff00ff00);

        hr = IDirectDrawSurface4_Flip(frontbuffer, NULL, DDFLIP_WAIT);
        ok(SUCCEEDED(hr), "%s: Failed to flip, hr %#x.\n", test_data[i].name, hr);
        color = get_surface_color(backbuffer1, 320, 240);
        ok(compare_color(color, 0x00ff0000, 1) || broken(sysmem_primary && compare_color(color, 0x0000ff00, 1)),
                "%s: Got unexpected color 0x%08x.\n", test_data[i].name, color);
        color = get_surface_color(backbuffer2, 320, 240);
        ok(compare_color(color, 0x0000ff00, 1), "%s: Got unexpected color 0x%08x.\n", test_data[i].name, color);
        fill_surface(backbuffer3, 0xff0000ff);

        hr = IDirectDrawSurface4_Flip(frontbuffer, backbuffer1, DDFLIP_WAIT);
        ok(SUCCEEDED(hr), "%s: Failed to flip, hr %#x.\n", test_data[i].name, hr);
        color = get_surface_color(backbuffer2, 320, 240);
        ok(compare_color(color, 0x0000ff00, 1) || broken(sysmem_primary && compare_color(color, 0x000000ff, 1)),
                "%s: Got unexpected color 0x%08x.\n", test_data[i].name, color);
        color = get_surface_color(backbuffer3, 320, 240);
        ok(compare_color(color, 0x000000ff, 1), "%s: Got unexpected color 0x%08x.\n", test_data[i].name, color);
        fill_surface(backbuffer1, 0xffff0000);

        hr = IDirectDrawSurface4_Flip(frontbuffer, backbuffer2, DDFLIP_WAIT);
        ok(SUCCEEDED(hr), "%s: Failed to flip, hr %#x.\n", test_data[i].name, hr);
        color = get_surface_color(backbuffer1, 320, 240);
        ok(compare_color(color, 0x00ff0000, 1), "%s: Got unexpected color 0x%08x.\n", test_data[i].name, color);
        color = get_surface_color(backbuffer3, 320, 240);
        ok(compare_color(color, 0x000000ff, 1) || broken(sysmem_primary && compare_color(color, 0x00ff0000, 1)),
                "%s: Got unexpected color 0x%08x.\n", test_data[i].name, color);
        fill_surface(backbuffer2, 0xff00ff00);

        hr = IDirectDrawSurface4_Flip(frontbuffer, backbuffer3, DDFLIP_WAIT);
        ok(SUCCEEDED(hr), "%s: Failed to flip, hr %#x.\n", test_data[i].name, hr);
        color = get_surface_color(backbuffer1, 320, 240);
        ok(compare_color(color, 0x00ff0000, 1) || broken(sysmem_primary && compare_color(color, 0x0000ff00, 1)),
                "%s: Got unexpected color 0x%08x.\n", test_data[i].name, color);
        color = get_surface_color(backbuffer2, 320, 240);
        ok(compare_color(color, 0x0000ff00, 1), "%s: Got unexpected color 0x%08x.\n", test_data[i].name, color);

        IDirectDrawSurface4_Release(backbuffer3);
        IDirectDrawSurface4_Release(backbuffer2);
        IDirectDrawSurface4_Release(backbuffer1);
        IDirectDrawSurface4_Release(frontbuffer);
    }

    refcount = IDirectDraw4_Release(ddraw);
    ok(refcount == 0, "The ddraw object was not properly freed, refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void reset_ddsd(DDSURFACEDESC2 *ddsd)
{
    memset(ddsd, 0, sizeof(*ddsd));
    ddsd->dwSize = sizeof(*ddsd);
}

static void test_set_surface_desc(void)
{
    IDirectDraw4 *ddraw;
    HWND window;
    HRESULT hr;
    DDSURFACEDESC2 ddsd;
    IDirectDrawSurface4 *surface;
    BYTE data[16*16*4];
    ULONG ref;
    unsigned int i;
    static const struct
    {
        DWORD caps, caps2;
        BOOL supported;
        const char *name;
    }
    invalid_caps_tests[] =
    {
        {DDSCAPS_VIDEOMEMORY, 0, FALSE, "videomemory plain"},
        {DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY, 0, TRUE, "systemmemory texture"},
        {DDSCAPS_TEXTURE, DDSCAPS2_D3DTEXTUREMANAGE, FALSE, "managed texture"},
        {DDSCAPS_TEXTURE, DDSCAPS2_TEXTUREMANAGE, FALSE, "managed texture"},
        {DDSCAPS_PRIMARYSURFACE | DDSCAPS_SYSTEMMEMORY, 0, FALSE, "systemmemory primary"},
    };

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT;
    ddsd.dwWidth = 8;
    ddsd.dwHeight = 8;
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(U4(ddsd).ddpfPixelFormat).dwRGBBitCount = 32;
    U2(U4(ddsd).ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(U4(ddsd).ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(U4(ddsd).ddpfPixelFormat).dwBBitMask = 0x000000ff;
    ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;

    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_LPSURFACE;
    ddsd.lpSurface = data;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(SUCCEEDED(hr), "Failed to set surface desc, hr %#x.\n", hr);

    /* Redundantly setting the same lpSurface is not an error. */
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(SUCCEEDED(hr), "Failed to set surface desc, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_GetSurfaceDesc(surface, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(!(ddsd.dwFlags & DDSD_LPSURFACE), "DDSD_LPSURFACE is set.\n");
    ok(ddsd.lpSurface == NULL, "lpSurface is %p, expected NULL.\n", ddsd.lpSurface);

    hr = IDirectDrawSurface4_Lock(surface, NULL, &ddsd, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    ok(!(ddsd.dwFlags & DDSD_LPSURFACE), "DDSD_LPSURFACE is set.\n");
    ok(ddsd.lpSurface == data, "lpSurface is %p, expected %p.\n", data, data);
    hr = IDirectDrawSurface4_Unlock(surface, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_LPSURFACE;
    ddsd.lpSurface = data;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 1);
    ok(hr == DDERR_INVALIDPARAMS, "SetSurfaceDesc with flags=1 returned %#x.\n", hr);

    ddsd.lpSurface = NULL;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting lpSurface=NULL returned %#x.\n", hr);

    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, NULL, 0);
    ok(hr == DDERR_INVALIDPARAMS, "SetSurfaceDesc with NULL desc returned %#x.\n", hr);

    hr = IDirectDrawSurface4_GetSurfaceDesc(surface, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.ddsCaps.dwCaps == (DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN),
            "Got unexpected caps %#x.\n", ddsd.ddsCaps.dwCaps);
    ok(ddsd.ddsCaps.dwCaps2 == 0, "Got unexpected caps2 %#x.\n", 0);

    /* Setting the caps is an error. This also means the original description cannot be reapplied. */
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting the original desc returned %#x.\n", hr);

    ddsd.dwFlags = DDSD_CAPS;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting DDSD_CAPS returned %#x.\n", hr);

    /* dwCaps = 0 is allowed, but ignored. Caps2 can be anything and is ignored too. */
    ddsd.dwFlags = DDSD_CAPS | DDSD_LPSURFACE;
    ddsd.lpSurface = data;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDCAPS, "Setting DDSD_CAPS returned %#x.\n", hr);
    ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDCAPS, "Setting DDSD_CAPS returned %#x.\n", hr);
    ddsd.ddsCaps.dwCaps = 0;
    ddsd.ddsCaps.dwCaps2 = 0xdeadbeef;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(SUCCEEDED(hr), "Failed to set surface desc, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_GetSurfaceDesc(surface, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.ddsCaps.dwCaps == (DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN),
            "Got unexpected caps %#x.\n", ddsd.ddsCaps.dwCaps);
    ok(ddsd.ddsCaps.dwCaps2 == 0, "Got unexpected caps2 %#x.\n", 0);

    /* Setting the height is allowed, but it cannot be set to 0, and only if LPSURFACE is set too. */
    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_HEIGHT;
    ddsd.dwHeight = 16;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting height without lpSurface returned %#x.\n", hr);

    ddsd.lpSurface = data;
    ddsd.dwFlags = DDSD_HEIGHT | DDSD_LPSURFACE;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(SUCCEEDED(hr), "Failed to set surface desc, hr %#x.\n", hr);

    ddsd.dwHeight = 0;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting height=0 returned %#x.\n", hr);

    reset_ddsd(&ddsd);
    hr = IDirectDrawSurface4_GetSurfaceDesc(surface, &ddsd);
    ok(SUCCEEDED(hr), "GetSurfaceDesc failed, hr %#x.\n", hr);
    ok(ddsd.dwWidth == 8, "SetSurfaceDesc: Expected width 8, got %u.\n", ddsd.dwWidth);
    ok(ddsd.dwHeight == 16, "SetSurfaceDesc: Expected height 16, got %u.\n", ddsd.dwHeight);

    /* Pitch and width can be set, but only together, and only with LPSURFACE. They must not be 0 */
    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_PITCH;
    U1(ddsd).lPitch = 8 * 4;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting pitch without lpSurface or width returned %#x.\n", hr);

    ddsd.dwFlags = DDSD_WIDTH;
    ddsd.dwWidth = 16;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting width without lpSurface or pitch returned %#x.\n", hr);

    ddsd.dwFlags = DDSD_PITCH | DDSD_LPSURFACE;
    ddsd.lpSurface = data;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting pitch and lpSurface without width returned %#x.\n", hr);

    ddsd.dwFlags = DDSD_WIDTH | DDSD_LPSURFACE;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting width and lpSurface without pitch returned %#x.\n", hr);

    ddsd.dwFlags = DDSD_WIDTH | DDSD_PITCH | DDSD_LPSURFACE;
    U1(ddsd).lPitch = 16 * 4;
    ddsd.dwWidth = 16;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(SUCCEEDED(hr), "Failed to set surface desc, hr %#x.\n", hr);

    reset_ddsd(&ddsd);
    hr = IDirectDrawSurface4_GetSurfaceDesc(surface, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == 16, "SetSurfaceDesc: Expected width 8, got %u.\n", ddsd.dwWidth);
    ok(ddsd.dwHeight == 16, "SetSurfaceDesc: Expected height 16, got %u.\n", ddsd.dwHeight);
    ok(U1(ddsd).lPitch == 16 * 4, "SetSurfaceDesc: Expected pitch 64, got %u.\n", U1(ddsd).lPitch);

    /* The pitch must be 32 bit aligned and > 0, but is not verified for sanity otherwise.
     *
     * VMware rejects those calls, but all real drivers accept it. Mark the VMware behavior broken. */
    ddsd.dwFlags = DDSD_WIDTH | DDSD_PITCH | DDSD_LPSURFACE;
    U1(ddsd).lPitch = 4 * 4;
    ddsd.lpSurface = data;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(SUCCEEDED(hr) || broken(hr == DDERR_INVALIDPARAMS), "Failed to set surface desc, hr %#x.\n", hr);

    U1(ddsd).lPitch = 4;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(SUCCEEDED(hr) || broken(hr == DDERR_INVALIDPARAMS), "Failed to set surface desc, hr %#x.\n", hr);

    U1(ddsd).lPitch = 16 * 4 + 1;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting misaligned pitch returned %#x.\n", hr);

    U1(ddsd).lPitch = 16 * 4 + 3;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting misaligned pitch returned %#x.\n", hr);

    U1(ddsd).lPitch = -4;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting negative pitch returned %#x.\n", hr);

    U1(ddsd).lPitch = 16 * 4;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(SUCCEEDED(hr), "Failed to set surface desc, hr %#x.\n", hr);

    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_PITCH | DDSD_LPSURFACE;
    U1(ddsd).lPitch = 0;
    ddsd.dwWidth = 16;
    ddsd.lpSurface = data;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting zero pitch returned %#x.\n", hr);

    ddsd.dwFlags = DDSD_WIDTH | DDSD_PITCH | DDSD_LPSURFACE;
    U1(ddsd).lPitch = 16 * 4;
    ddsd.dwWidth = 0;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting zero width returned %#x.\n", hr);

    /* Setting the pixelformat without LPSURFACE is an error, but with LPSURFACE it works. */
    ddsd.dwFlags = DDSD_PIXELFORMAT;
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(U4(ddsd).ddpfPixelFormat).dwRGBBitCount = 32;
    U2(U4(ddsd).ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(U4(ddsd).ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(U4(ddsd).ddpfPixelFormat).dwBBitMask = 0x000000ff;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting the pixel format returned %#x.\n", hr);

    ddsd.dwFlags = DDSD_PIXELFORMAT | DDSD_LPSURFACE;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(SUCCEEDED(hr), "Failed to set surface desc, hr %#x.\n", hr);

    /* Can't set color keys. */
    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_CKSRCBLT;
    ddsd.ddckCKSrcBlt.dwColorSpaceLowValue = 0x00ff0000;
    ddsd.ddckCKSrcBlt.dwColorSpaceHighValue = 0x00ff0000;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting ddckCKSrcBlt returned %#x.\n", hr);

    ddsd.dwFlags = DDSD_CKSRCBLT | DDSD_LPSURFACE;
    ddsd.lpSurface = data;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Setting ddckCKSrcBlt returned %#x.\n", hr);

    IDirectDrawSurface4_Release(surface);

    /* SetSurfaceDesc needs systemmemory surfaces.
     *
     * As a sidenote, fourcc surfaces aren't allowed in sysmem, thus testing DDSD_LINEARSIZE is moot. */
    for (i = 0; i < ARRAY_SIZE(invalid_caps_tests); i++)
    {
        reset_ddsd(&ddsd);
        ddsd.dwFlags = DDSD_CAPS;
        ddsd.ddsCaps.dwCaps = invalid_caps_tests[i].caps;
        ddsd.ddsCaps.dwCaps2 = invalid_caps_tests[i].caps2;
        if (!(invalid_caps_tests[i].caps & DDSCAPS_PRIMARYSURFACE))
        {
            ddsd.dwFlags |= DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
            ddsd.dwWidth = 8;
            ddsd.dwHeight = 8;
            U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
            U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_RGB;
            U1(U4(ddsd).ddpfPixelFormat).dwRGBBitCount = 32;
            U2(U4(ddsd).ddpfPixelFormat).dwRBitMask = 0x00ff0000;
            U3(U4(ddsd).ddpfPixelFormat).dwGBitMask = 0x0000ff00;
            U4(U4(ddsd).ddpfPixelFormat).dwBBitMask = 0x000000ff;
        }

        hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &surface, NULL);
        ok(SUCCEEDED(hr) || hr == DDERR_NODIRECTDRAWHW, "Failed to create surface, hr %#x.\n", hr);
        if (FAILED(hr))
        {
            skip("Cannot create a %s surface, skipping vidmem SetSurfaceDesc test.\n",
                    invalid_caps_tests[i].name);
            goto done;
        }

        reset_ddsd(&ddsd);
        ddsd.dwFlags = DDSD_LPSURFACE;
        ddsd.lpSurface = data;
        hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
        if (invalid_caps_tests[i].supported)
        {
            ok(SUCCEEDED(hr), "Failed to set surface desc, hr %#x.\n", hr);
        }
        else
        {
            ok(hr == DDERR_INVALIDSURFACETYPE, "SetSurfaceDesc on a %s surface returned %#x.\n",
                    invalid_caps_tests[i].name, hr);

            /* Check priority of error conditions. */
            ddsd.dwFlags = DDSD_WIDTH;
            hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
            ok(hr == DDERR_INVALIDSURFACETYPE, "SetSurfaceDesc on a %s surface returned %#x.\n",
                    invalid_caps_tests[i].name, hr);
        }

        IDirectDrawSurface4_Release(surface);
    }

done:
    ref = IDirectDraw4_Release(ddraw);
    ok(ref == 0, "Ddraw object not properly released, refcount %u.\n", ref);
    DestroyWindow(window);
}

static void test_user_memory_getdc(void)
{
    IDirectDraw4 *ddraw;
    HWND window;
    HRESULT hr;
    DDSURFACEDESC2 ddsd;
    IDirectDrawSurface4 *surface;
    DWORD data[16][16];
    HBITMAP bitmap;
    DIBSECTION dib;
    ULONG ref;
    int size;
    HDC dc;
    unsigned int x, y;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT;
    ddsd.dwWidth = 16;
    ddsd.dwHeight = 16;
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(U4(ddsd).ddpfPixelFormat).dwRGBBitCount = 32;
    U2(U4(ddsd).ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(U4(ddsd).ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(U4(ddsd).ddpfPixelFormat).dwBBitMask = 0x000000ff;
    ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;
    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    memset(data, 0xaa, sizeof(data));
    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_LPSURFACE;
    ddsd.lpSurface = data;
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(SUCCEEDED(hr), "Failed to set surface desc, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_GetDC(surface, &dc);
    ok(SUCCEEDED(hr), "Failed to get DC, hr %#x.\n", hr);
    bitmap = GetCurrentObject(dc, OBJ_BITMAP);
    ok(!!bitmap, "Failed to get bitmap.\n");
    size = GetObjectA(bitmap, sizeof(dib), &dib);
    ok(size == sizeof(dib), "Got unexpected size %d.\n", size);
    ok(dib.dsBm.bmBits == data, "Got unexpected bits %p, expected %p.\n", dib.dsBm.bmBits, data);
    BitBlt(dc, 0, 0, 16, 8, NULL, 0, 0, WHITENESS);
    BitBlt(dc, 0, 8, 16, 8, NULL, 0, 0, BLACKNESS);
    hr = IDirectDrawSurface4_ReleaseDC(surface, dc);
    ok(SUCCEEDED(hr), "Failed to release DC, hr %#x.\n", hr);

    ok(data[0][0] == 0xffffffff, "Expected color 0xffffffff, got %#x.\n", data[0][0]);
    ok(data[15][15] == 0x00000000, "Expected color 0x00000000, got %#x.\n", data[15][15]);

    ddsd.dwFlags = DDSD_LPSURFACE | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH;
    ddsd.lpSurface = data;
    ddsd.dwWidth = 4;
    ddsd.dwHeight = 8;
    U1(ddsd).lPitch = sizeof(*data);
    hr = IDirectDrawSurface4_SetSurfaceDesc(surface, &ddsd, 0);
    ok(SUCCEEDED(hr), "Failed to set surface desc, hr %#x.\n", hr);

    memset(data, 0xaa, sizeof(data));
    hr = IDirectDrawSurface4_GetDC(surface, &dc);
    ok(SUCCEEDED(hr), "Failed to get DC, hr %#x.\n", hr);
    BitBlt(dc, 0, 0, 4, 8, NULL, 0, 0, BLACKNESS);
    BitBlt(dc, 1, 1, 2, 2, NULL, 0, 0, WHITENESS);
    hr = IDirectDrawSurface4_ReleaseDC(surface, dc);
    ok(SUCCEEDED(hr), "Failed to release DC, hr %#x.\n", hr);

    for (y = 0; y < 4; y++)
    {
        for (x = 0; x < 4; x++)
        {
            if ((x == 1 || x == 2) && (y == 1 || y == 2))
                ok(data[y][x] == 0xffffffff, "Expected color 0xffffffff on position %ux%u, got %#x.\n",
                        x, y, data[y][x]);
            else
                ok(data[y][x] == 0x00000000, "Expected color 0x00000000 on position %ux%u, got %#x.\n",
                        x, y, data[y][x]);
        }
    }
    ok(data[0][5] == 0xaaaaaaaa, "Expected color 0xaaaaaaaa on position 5x0, got %#x.\n",
            data[0][5]);
    ok(data[7][3] == 0x00000000, "Expected color 0x00000000 on position 3x7, got %#x.\n",
            data[7][3]);
    ok(data[7][4] == 0xaaaaaaaa, "Expected color 0xaaaaaaaa on position 4x7, got %#x.\n",
            data[7][4]);
    ok(data[8][0] == 0xaaaaaaaa, "Expected color 0xaaaaaaaa on position 0x8, got %#x.\n",
            data[8][0]);

    IDirectDrawSurface4_Release(surface);
    ref = IDirectDraw4_Release(ddraw);
    ok(ref == 0, "Ddraw object not properly released, refcount %u.\n", ref);
    DestroyWindow(window);
}

static void test_sysmem_overlay(void)
{
    IDirectDraw4 *ddraw;
    HWND window;
    HRESULT hr;
    DDSURFACEDESC2 ddsd;
    IDirectDrawSurface4 *surface;
    ULONG ref;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    reset_ddsd(&ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.dwWidth = 16;
    ddsd.dwHeight = 16;
    ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OVERLAY;
    U4(ddsd).ddpfPixelFormat.dwSize = sizeof(U4(ddsd).ddpfPixelFormat);
    U4(ddsd).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(U4(ddsd).ddpfPixelFormat).dwRGBBitCount = 32;
    U2(U4(ddsd).ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(U4(ddsd).ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(U4(ddsd).ddpfPixelFormat).dwBBitMask = 0x000000ff;
    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &surface, NULL);
    ok(hr == DDERR_NOOVERLAYHW, "Got unexpected hr %#x.\n", hr);

    ref = IDirectDraw4_Release(ddraw);
    ok(ref == 0, "Ddraw object not properly released, refcount %u.\n", ref);
    DestroyWindow(window);
}

static void test_primary_palette(void)
{
    DDSCAPS2 surface_caps = {DDSCAPS_FLIP, 0, 0, {0}};
    IDirectDrawSurface4 *primary, *backbuffer;
    PALETTEENTRY palette_entries[256];
    IDirectDrawPalette *palette, *tmp;
    DDSURFACEDESC2 surface_desc;
    IDirectDraw4 *ddraw;
    DWORD palette_caps;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    if (FAILED(IDirectDraw4_SetDisplayMode(ddraw, 640, 480, 8, 0, 0)))
    {
        win_skip("Failed to set 8 bpp display mode, skipping test.\n");
        IDirectDraw4_Release(ddraw);
        DestroyWindow(window);
        return;
    }
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;
    U5(surface_desc).dwBackBufferCount = 1;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_GetAttachedSurface(primary, &surface_caps, &backbuffer);
    ok(SUCCEEDED(hr), "Failed to get attached surface, hr %#x.\n", hr);

    memset(palette_entries, 0, sizeof(palette_entries));
    hr = IDirectDraw4_CreatePalette(ddraw, DDPCAPS_8BIT | DDPCAPS_ALLOW256, palette_entries, &palette, NULL);
    ok(SUCCEEDED(hr), "Failed to create palette, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)palette);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);

    hr = IDirectDrawPalette_GetCaps(palette, &palette_caps);
    ok(SUCCEEDED(hr), "Failed to get palette caps, hr %#x.\n", hr);
    ok(palette_caps == (DDPCAPS_8BIT | DDPCAPS_ALLOW256), "Got unexpected palette caps %#x.\n", palette_caps);

    hr = IDirectDrawSurface4_SetPalette(primary, palette);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);

    /* The Windows 8 testbot attaches the palette to the backbuffer as well,
     * and is generally somewhat broken with respect to 8 bpp / palette
     * handling. */
    if (SUCCEEDED(IDirectDrawSurface4_GetPalette(backbuffer, &tmp)))
    {
        win_skip("Broken palette handling detected, skipping tests.\n");
        IDirectDrawPalette_Release(tmp);
        IDirectDrawPalette_Release(palette);
        /* The Windows 8 testbot keeps extra references to the primary and
         * backbuffer while in 8 bpp mode. */
        hr = IDirectDraw4_RestoreDisplayMode(ddraw);
        ok(SUCCEEDED(hr), "Failed to restore display mode, hr %#x.\n", hr);
        goto done;
    }

    refcount = get_refcount((IUnknown *)palette);
    ok(refcount == 2, "Got unexpected refcount %u.\n", refcount);

    hr = IDirectDrawPalette_GetCaps(palette, &palette_caps);
    ok(SUCCEEDED(hr), "Failed to get palette caps, hr %#x.\n", hr);
    ok(palette_caps == (DDPCAPS_8BIT | DDPCAPS_PRIMARYSURFACE | DDPCAPS_ALLOW256),
            "Got unexpected palette caps %#x.\n", palette_caps);

    hr = IDirectDrawSurface4_SetPalette(primary, NULL);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)palette);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);

    hr = IDirectDrawPalette_GetCaps(palette, &palette_caps);
    ok(SUCCEEDED(hr), "Failed to get palette caps, hr %#x.\n", hr);
    ok(palette_caps == (DDPCAPS_8BIT | DDPCAPS_ALLOW256), "Got unexpected palette caps %#x.\n", palette_caps);

    hr = IDirectDrawSurface4_SetPalette(primary, palette);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)palette);
    ok(refcount == 2, "Got unexpected refcount %u.\n", refcount);

    hr = IDirectDrawSurface4_GetPalette(primary, &tmp);
    ok(SUCCEEDED(hr), "Failed to get palette, hr %#x.\n", hr);
    ok(tmp == palette, "Got unexpected palette %p, expected %p.\n", tmp, palette);
    IDirectDrawPalette_Release(tmp);
    hr = IDirectDrawSurface4_GetPalette(backbuffer, &tmp);
    ok(hr == DDERR_NOPALETTEATTACHED, "Got unexpected hr %#x.\n", hr);

    refcount = IDirectDrawPalette_Release(palette);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDrawPalette_Release(palette);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    /* Note that this only seems to work when the palette is attached to the
     * primary surface. When attached to a regular surface, attempting to get
     * the palette here will cause an access violation. */
    hr = IDirectDrawSurface4_GetPalette(primary, &tmp);
    ok(hr == DDERR_NOPALETTEATTACHED, "Got unexpected hr %#x.\n", hr);

    hr = IDirectDrawSurface4_IsLost(primary);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(surface_desc.dwWidth == 640, "Got unexpected surface width %u.\n", surface_desc.dwWidth);
    ok(surface_desc.dwHeight == 480, "Got unexpected surface height %u.\n", surface_desc.dwHeight);
    ok(U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount == 8, "Got unexpected bit count %u.\n",
            U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount);

    hr = set_display_mode(ddraw, 640, 480);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(surface_desc.dwWidth == 640, "Got unexpected surface width %u.\n", surface_desc.dwWidth);
    ok(surface_desc.dwHeight == 480, "Got unexpected surface height %u.\n", surface_desc.dwHeight);
    ok(U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount == 32
            || U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount == 24,
            "Got unexpected bit count %u.\n", U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount);

    hr = IDirectDrawSurface4_IsLost(primary);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Restore(primary);
    ok(hr == DDERR_WRONGMODE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_IsLost(primary);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface4_GetSurfaceDesc(primary, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(surface_desc.dwWidth == 640, "Got unexpected surface width %u.\n", surface_desc.dwWidth);
    ok(surface_desc.dwHeight == 480, "Got unexpected surface height %u.\n", surface_desc.dwHeight);
    ok(U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount == 32
            || U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount == 24,
            "Got unexpected bit count %u.\n", U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount);

done:
    refcount = IDirectDrawSurface4_Release(backbuffer);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDrawSurface4_Release(primary);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDraw4_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

static HRESULT WINAPI surface_counter(IDirectDrawSurface4 *surface, DDSURFACEDESC2 *desc, void *context)
{
    UINT *surface_count = context;

    ++(*surface_count);
    IDirectDrawSurface_Release(surface);

    return DDENUMRET_OK;
}

static void test_surface_attachment(void)
{
    IDirectDrawSurface4 *surface1, *surface2, *surface3, *surface4;
    IDirectDrawSurface *surface1v1, *surface2v1;
    DDSCAPS2 caps = {DDSCAPS_TEXTURE, 0, 0, {0}};
    DDSURFACEDESC2 surface_desc;
    IDirectDraw4 *ddraw;
    UINT surface_count;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_MIPMAPCOUNT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
    U2(surface_desc).dwMipMapCount = 3;
    surface_desc.dwWidth = 128;
    surface_desc.dwHeight = 128;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface1, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_GetAttachedSurface(surface1, &caps, &surface2);
    ok(SUCCEEDED(hr), "Failed to get mip level, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_GetAttachedSurface(surface2, &caps, &surface3);
    ok(SUCCEEDED(hr), "Failed to get mip level, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_GetAttachedSurface(surface3, &caps, &surface4);
    ok(hr == DDERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);

    surface_count = 0;
    IDirectDrawSurface4_EnumAttachedSurfaces(surface1, &surface_count, surface_counter);
    ok(surface_count == 1, "Got unexpected surface_count %u.\n", surface_count);
    surface_count = 0;
    IDirectDrawSurface4_EnumAttachedSurfaces(surface2, &surface_count, surface_counter);
    ok(surface_count == 1, "Got unexpected surface_count %u.\n", surface_count);
    surface_count = 0;
    IDirectDrawSurface4_EnumAttachedSurfaces(surface3, &surface_count, surface_counter);
    ok(!surface_count, "Got unexpected surface_count %u.\n", surface_count);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    surface_desc.dwWidth = 16;
    surface_desc.dwHeight = 16;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface4, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_AddAttachedSurface(surface1, surface4);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_AddAttachedSurface(surface4, surface1);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_AddAttachedSurface(surface3, surface4);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_AddAttachedSurface(surface4, surface3);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_AddAttachedSurface(surface2, surface4);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_AddAttachedSurface(surface4, surface2);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);

    IDirectDrawSurface4_Release(surface4);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;
    surface_desc.dwWidth = 16;
    surface_desc.dwHeight = 16;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface4, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    if (SUCCEEDED(hr = IDirectDrawSurface4_AddAttachedSurface(surface1, surface4)))
    {
        skip("Running on refrast, skipping some tests.\n");
        hr = IDirectDrawSurface4_DeleteAttachedSurface(surface1, 0, surface4);
        ok(SUCCEEDED(hr), "Failed to detach surface, hr %#x.\n", hr);
    }
    else
    {
        ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
        hr = IDirectDrawSurface4_AddAttachedSurface(surface4, surface1);
        ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
        hr = IDirectDrawSurface4_AddAttachedSurface(surface3, surface4);
        ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
        hr = IDirectDrawSurface4_AddAttachedSurface(surface4, surface3);
        ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
        hr = IDirectDrawSurface4_AddAttachedSurface(surface2, surface4);
        ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
        hr = IDirectDrawSurface4_AddAttachedSurface(surface4, surface2);
        ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
    }

    IDirectDrawSurface4_Release(surface4);
    IDirectDrawSurface4_Release(surface3);
    IDirectDrawSurface4_Release(surface2);
    IDirectDrawSurface4_Release(surface1);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    /* Try a single primary and two offscreen plain surfaces. */
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface1, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    surface_desc.dwWidth = registry_mode.dmPelsWidth;
    surface_desc.dwHeight = registry_mode.dmPelsHeight;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface2, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    surface_desc.dwWidth = registry_mode.dmPelsWidth;
    surface_desc.dwHeight = registry_mode.dmPelsHeight;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface3, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    /* This one has a different size. */
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    surface_desc.dwWidth = 128;
    surface_desc.dwHeight = 128;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface4, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_AddAttachedSurface(surface1, surface2);
    ok(SUCCEEDED(hr), "Failed to attach surface, hr %#x.\n", hr);
    /* Try the reverse without detaching first. */
    hr = IDirectDrawSurface4_AddAttachedSurface(surface2, surface1);
    ok(hr == DDERR_SURFACEALREADYATTACHED, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_DeleteAttachedSurface(surface1, 0, surface2);
    ok(SUCCEEDED(hr), "Failed to detach surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_AddAttachedSurface(surface2, surface1);
    ok(SUCCEEDED(hr), "Failed to attach surface, hr %#x.\n", hr);
    /* Try to detach reversed. */
    hr = IDirectDrawSurface4_DeleteAttachedSurface(surface1, 0, surface2);
    ok(hr == DDERR_CANNOTDETACHSURFACE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_DeleteAttachedSurface(surface2, 0, surface1);
    ok(SUCCEEDED(hr), "Failed to detach surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_AddAttachedSurface(surface2, surface3);
    ok(SUCCEEDED(hr), "Failed to attach surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_DeleteAttachedSurface(surface2, 0, surface3);
    ok(SUCCEEDED(hr), "Failed to detach surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_AddAttachedSurface(surface1, surface4);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_AddAttachedSurface(surface4, surface1);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);

    IDirectDrawSurface4_Release(surface4);
    IDirectDrawSurface4_Release(surface3);
    IDirectDrawSurface4_Release(surface2);
    IDirectDrawSurface4_Release(surface1);

    /* Test DeleteAttachedSurface() and automatic detachment of attached surfaces on release. */
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    surface_desc.dwWidth = 64;
    surface_desc.dwHeight = 64;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    U4(surface_desc).ddpfPixelFormat.dwSize = sizeof(U4(surface_desc).ddpfPixelFormat);
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_RGB; /* D3DFMT_R5G6B5 */
    U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount = 16;
    U2(U4(surface_desc).ddpfPixelFormat).dwRBitMask = 0xf800;
    U3(U4(surface_desc).ddpfPixelFormat).dwGBitMask = 0x07e0;
    U4(U4(surface_desc).ddpfPixelFormat).dwBBitMask = 0x001f;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface1, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface3, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    surface_desc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_ZBUFFER;
    U1(U4(surface_desc).ddpfPixelFormat).dwZBufferBitDepth = 16;
    U3(U4(surface_desc).ddpfPixelFormat).dwZBitMask = 0x0000ffff;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface2, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_QueryInterface(surface1, &IID_IDirectDrawSurface, (void **)&surface1v1);
    ok(SUCCEEDED(hr), "Failed to get interface, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_QueryInterface(surface2, &IID_IDirectDrawSurface, (void **)&surface2v1);
    ok(SUCCEEDED(hr), "Failed to get interface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_AddAttachedSurface(surface1, surface2);
    ok(SUCCEEDED(hr), "Failed to attach surface, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)surface2);
    ok(refcount == 2, "Got unexpected refcount %u.\n", refcount);
    refcount = get_refcount((IUnknown *)surface2v1);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);
    hr = IDirectDrawSurface4_AddAttachedSurface(surface1, surface2);
    ok(hr == DDERR_SURFACEALREADYATTACHED, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_AddAttachedSurface(surface1v1, surface2v1);
    todo_wine ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_DeleteAttachedSurface(surface1v1, 0, surface2v1);
    ok(hr == DDERR_SURFACENOTATTACHED, "Got unexpected hr %#x.\n", hr);

    /* Attaching while already attached to other surface. */
    hr = IDirectDrawSurface4_AddAttachedSurface(surface3, surface2);
    todo_wine ok(SUCCEEDED(hr), "Failed to attach surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_DeleteAttachedSurface(surface3, 0, surface2);
    todo_wine ok(SUCCEEDED(hr), "Failed to detach surface, hr %#x.\n", hr);
    IDirectDrawSurface4_Release(surface3);

    hr = IDirectDrawSurface4_DeleteAttachedSurface(surface1, 0, surface2);
    ok(SUCCEEDED(hr), "Failed to detach surface, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)surface2);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);
    refcount = get_refcount((IUnknown *)surface2v1);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);

    /* DeleteAttachedSurface() when attaching via IDirectDrawSurface. */
    hr = IDirectDrawSurface_AddAttachedSurface(surface1v1, surface2v1);
    ok(SUCCEEDED(hr), "Failed to attach surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_DeleteAttachedSurface(surface1, 0, surface2);
    ok(hr == DDERR_SURFACENOTATTACHED, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_DeleteAttachedSurface(surface1v1, 0, surface2v1);
    ok(SUCCEEDED(hr), "Failed to detach surface, hr %#x.\n", hr);
    refcount = IDirectDrawSurface4_Release(surface2);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDrawSurface4_Release(surface1);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    /* Automatic detachment on release. */
    hr = IDirectDrawSurface_AddAttachedSurface(surface1v1, surface2v1);
    ok(SUCCEEDED(hr), "Failed to attach surface, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)surface2v1);
    ok(refcount == 2, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDrawSurface_Release(surface1v1);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDrawSurface_Release(surface2v1);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDraw4_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_private_data(void)
{
    IDirectDraw4 *ddraw;
    IDirectDrawSurface4 *surface, *surface2;
    DDSURFACEDESC2 surface_desc;
    ULONG refcount, refcount2, refcount3;
    IUnknown *ptr;
    DWORD size = sizeof(ptr);
    HRESULT hr;
    HWND window;
    DDSCAPS2 caps = {DDSCAPS_COMPLEX, 0, 0, {0}};
    DWORD data[] = {1, 2, 3, 4};
    DDCAPS hal_caps;
    static const GUID ddraw_private_data_test_guid =
    {
        0xfdb37466,
        0x428f,
        0x4edf,
        {0xa3,0x7f,0x9b,0x1d,0xf4,0x88,0xc5,0xfc}
    };
    static const GUID ddraw_private_data_test_guid2 =
    {
        0x2e5afac2,
        0x87b5,
        0x4c10,
        {0x9b,0x4b,0x89,0xd7,0xd1,0x12,0xe7,0x2b}
    };

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    reset_ddsd(&surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    surface_desc.ddsCaps.dwCaps |= DDSCAPS_OFFSCREENPLAIN;
    surface_desc.dwHeight = 4;
    surface_desc.dwWidth = 4;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    /* NULL pointers are not valid, but don't cause a crash. */
    hr = IDirectDrawSurface4_SetPrivateData(surface, &ddraw_private_data_test_guid, NULL,
            sizeof(IUnknown *), DDSPD_IUNKNOWNPOINTER);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_SetPrivateData(surface, &ddraw_private_data_test_guid, NULL, 0, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_SetPrivateData(surface, &ddraw_private_data_test_guid, NULL, 1, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    /* DDSPD_IUNKNOWNPOINTER needs sizeof(IUnknown *) bytes of data. */
    hr = IDirectDrawSurface4_SetPrivateData(surface, &ddraw_private_data_test_guid, ddraw,
            0, DDSPD_IUNKNOWNPOINTER);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_SetPrivateData(surface, &ddraw_private_data_test_guid, ddraw,
            5, DDSPD_IUNKNOWNPOINTER);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_SetPrivateData(surface, &ddraw_private_data_test_guid, ddraw,
            sizeof(ddraw) * 2, DDSPD_IUNKNOWNPOINTER);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    /* Note that with a size != 0 and size != sizeof(IUnknown *) and
     * DDSPD_IUNKNOWNPOINTER set SetPrivateData in ddraw4 and ddraw7
     * erases the old content and returns an error. This behavior has
     * been fixed in d3d8 and d3d9. Unless an application is found
     * that depends on this we don't care about this behavior. */
    hr = IDirectDrawSurface4_SetPrivateData(surface, &ddraw_private_data_test_guid, ddraw,
            sizeof(ddraw), DDSPD_IUNKNOWNPOINTER);
    ok(SUCCEEDED(hr), "Failed to set private data, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_SetPrivateData(surface, &ddraw_private_data_test_guid, ddraw,
            0, DDSPD_IUNKNOWNPOINTER);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    size = sizeof(ptr);
    hr = IDirectDrawSurface4_GetPrivateData(surface, &ddraw_private_data_test_guid, &ptr, &size);
    ok(SUCCEEDED(hr), "Failed to get private data, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_FreePrivateData(surface, &ddraw_private_data_test_guid);
    ok(SUCCEEDED(hr), "Failed to free private data, hr %#x.\n", hr);

    refcount = get_refcount((IUnknown *)ddraw);
    hr = IDirectDrawSurface4_SetPrivateData(surface, &ddraw_private_data_test_guid, ddraw,
            sizeof(ddraw), DDSPD_IUNKNOWNPOINTER);
    ok(SUCCEEDED(hr), "Failed to set private data, hr %#x.\n", hr);
    refcount2 = get_refcount((IUnknown *)ddraw);
    ok(refcount2 == refcount + 1, "Got unexpected refcount %u.\n", refcount2);

    hr = IDirectDrawSurface4_FreePrivateData(surface, &ddraw_private_data_test_guid);
    ok(SUCCEEDED(hr), "Failed to free private data, hr %#x.\n", hr);
    refcount2 = get_refcount((IUnknown *)ddraw);
    ok(refcount2 == refcount, "Got unexpected refcount %u.\n", refcount2);

    hr = IDirectDrawSurface4_SetPrivateData(surface, &ddraw_private_data_test_guid, ddraw,
            sizeof(ddraw), DDSPD_IUNKNOWNPOINTER);
    ok(SUCCEEDED(hr), "Failed to set private data, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_SetPrivateData(surface, &ddraw_private_data_test_guid, surface,
            sizeof(surface), DDSPD_IUNKNOWNPOINTER);
    ok(SUCCEEDED(hr), "Failed to set private data, hr %#x.\n", hr);
    refcount2 = get_refcount((IUnknown *)ddraw);
    ok(refcount2 == refcount, "Got unexpected refcount %u.\n", refcount2);

    hr = IDirectDrawSurface4_SetPrivateData(surface, &ddraw_private_data_test_guid, ddraw,
            sizeof(ddraw), DDSPD_IUNKNOWNPOINTER);
    ok(SUCCEEDED(hr), "Failed to set private data, hr %#x.\n", hr);
    size = 2 * sizeof(ptr);
    hr = IDirectDrawSurface4_GetPrivateData(surface, &ddraw_private_data_test_guid, &ptr, &size);
    ok(SUCCEEDED(hr), "Failed to get private data, hr %#x.\n", hr);
    ok(size == sizeof(ddraw), "Got unexpected size %u.\n", size);
    refcount2 = get_refcount(ptr);
    /* Object is NOT addref'ed by the getter. */
    ok(ptr == (IUnknown *)ddraw, "Returned interface pointer is %p, expected %p.\n", ptr, ddraw);
    ok(refcount2 == refcount + 1, "Got unexpected refcount %u.\n", refcount2);

    ptr = (IUnknown *)0xdeadbeef;
    size = 1;
    hr = IDirectDrawSurface4_GetPrivateData(surface, &ddraw_private_data_test_guid, NULL, &size);
    ok(hr == DDERR_MOREDATA, "Got unexpected hr %#x.\n", hr);
    ok(size == sizeof(ddraw), "Got unexpected size %u.\n", size);
    size = 2 * sizeof(ptr);
    hr = IDirectDrawSurface4_GetPrivateData(surface, &ddraw_private_data_test_guid, NULL, &size);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    ok(size == 2 * sizeof(ptr), "Got unexpected size %u.\n", size);
    size = 1;
    hr = IDirectDrawSurface4_GetPrivateData(surface, &ddraw_private_data_test_guid, &ptr, &size);
    ok(hr == DDERR_MOREDATA, "Got unexpected hr %#x.\n", hr);
    ok(size == sizeof(ddraw), "Got unexpected size %u.\n", size);
    ok(ptr == (IUnknown *)0xdeadbeef, "Got unexpected pointer %p.\n", ptr);
    hr = IDirectDrawSurface4_GetPrivateData(surface, &ddraw_private_data_test_guid2, NULL, NULL);
    ok(hr == DDERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);
    size = 0xdeadbabe;
    hr = IDirectDrawSurface4_GetPrivateData(surface, &ddraw_private_data_test_guid2, &ptr, &size);
    ok(hr == DDERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);
    ok(ptr == (IUnknown *)0xdeadbeef, "Got unexpected pointer %p.\n", ptr);
    ok(size == 0xdeadbabe, "Got unexpected size %u.\n", size);
    hr = IDirectDrawSurface4_GetPrivateData(surface, &ddraw_private_data_test_guid, NULL, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    refcount3 = IDirectDrawSurface4_Release(surface);
    ok(!refcount3, "Got unexpected refcount %u.\n", refcount3);

    /* Destroying the surface frees the reference held on the private data. It also frees
     * the reference the surface is holding on its creating object. */
    refcount2 = get_refcount((IUnknown *)ddraw);
    ok(refcount2 == refcount - 1, "Got unexpected refcount %u.\n", refcount2);

    memset(&hal_caps, 0, sizeof(hal_caps));
    hal_caps.dwSize = sizeof(hal_caps);
    hr = IDirectDraw7_GetCaps(ddraw, &hal_caps, NULL);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);
    if ((hal_caps.ddsCaps.dwCaps & (DDSCAPS_TEXTURE | DDSCAPS_MIPMAP)) == (DDSCAPS_TEXTURE | DDSCAPS_MIPMAP))
    {
        reset_ddsd(&surface_desc);
        surface_desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_MIPMAPCOUNT;
        surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
        surface_desc.dwHeight = 4;
        surface_desc.dwWidth = 4;
        U2(surface_desc).dwMipMapCount = 2;
        hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
        ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
        hr = IDirectDrawSurface4_GetAttachedSurface(surface, &caps, &surface2);
        ok(SUCCEEDED(hr), "Failed to get attached surface, hr %#x.\n", hr);

        hr = IDirectDrawSurface4_SetPrivateData(surface, &ddraw_private_data_test_guid, data, sizeof(data), 0);
        ok(SUCCEEDED(hr), "Failed to set private data, hr %#x.\n", hr);
        hr = IDirectDrawSurface4_GetPrivateData(surface2, &ddraw_private_data_test_guid, NULL, NULL);
        ok(hr == DDERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);

        IDirectDrawSurface4_Release(surface2);
        IDirectDrawSurface4_Release(surface);
    }
    else
        skip("Mipmapped textures not supported, skipping mipmap private data test.\n");

    refcount = IDirectDraw4_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_pixel_format(void)
{
    HWND window, window2 = NULL;
    HDC hdc, hdc2 = NULL;
    HMODULE gl = NULL;
    int format, test_format;
    PIXELFORMATDESCRIPTOR pfd;
    IDirectDraw4 *ddraw = NULL;
    IDirectDrawClipper *clipper = NULL;
    DDSURFACEDESC2 ddsd;
    IDirectDrawSurface4 *primary = NULL;
    DDBLTFX fx;
    HRESULT hr;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            100, 100, 160, 160, NULL, NULL, NULL, NULL);
    if (!window)
    {
        skip("Failed to create window\n");
        return;
    }

    window2 = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            100, 100, 160, 160, NULL, NULL, NULL, NULL);

    hdc = GetDC(window);
    if (!hdc)
    {
        skip("Failed to get DC\n");
        goto cleanup;
    }

    if (window2)
        hdc2 = GetDC(window2);

    gl = LoadLibraryA("opengl32.dll");
    ok(!!gl, "failed to load opengl32.dll; SetPixelFormat()/GetPixelFormat() may not work right\n");

    format = GetPixelFormat(hdc);
    ok(format == 0, "new window has pixel format %d\n", format);

    ZeroMemory(&pfd, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.iLayerType = PFD_MAIN_PLANE;
    format = ChoosePixelFormat(hdc, &pfd);
    if (format <= 0)
    {
        skip("no pixel format available\n");
        goto cleanup;
    }

    if (!SetPixelFormat(hdc, format, &pfd) || GetPixelFormat(hdc) != format)
    {
        skip("failed to set pixel format\n");
        goto cleanup;
    }

    if (!hdc2 || !SetPixelFormat(hdc2, format, &pfd) || GetPixelFormat(hdc2) != format)
    {
        skip("failed to set pixel format on second window\n");
        if (hdc2)
        {
            ReleaseDC(window2, hdc2);
            hdc2 = NULL;
        }
    }

    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    test_format = GetPixelFormat(hdc);
    ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    if (FAILED(hr))
    {
        skip("Failed to set cooperative level, hr %#x.\n", hr);
        goto cleanup;
    }

    test_format = GetPixelFormat(hdc);
    ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

    if (hdc2)
    {
        hr = IDirectDraw4_CreateClipper(ddraw, 0, &clipper, NULL);
        ok(SUCCEEDED(hr), "Failed to create clipper, hr %#x.\n", hr);
        hr = IDirectDrawClipper_SetHWnd(clipper, 0, window2);
        ok(SUCCEEDED(hr), "Failed to set clipper window, hr %#x.\n", hr);

        test_format = GetPixelFormat(hdc);
        ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

        test_format = GetPixelFormat(hdc2);
        ok(test_format == format, "second window has pixel format %d, expected %d\n", test_format, format);
    }

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw4_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);

    test_format = GetPixelFormat(hdc);
    ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

    if (hdc2)
    {
        test_format = GetPixelFormat(hdc2);
        ok(test_format == format, "second window has pixel format %d, expected %d\n", test_format, format);
    }

    if (clipper)
    {
        hr = IDirectDrawSurface4_SetClipper(primary, clipper);
        ok(SUCCEEDED(hr), "Failed to set clipper, hr %#x.\n", hr);

        test_format = GetPixelFormat(hdc);
        ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

        test_format = GetPixelFormat(hdc2);
        ok(test_format == format, "second window has pixel format %d, expected %d\n", test_format, format);
    }

    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    hr = IDirectDrawSurface4_Blt(primary, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear source surface, hr %#x.\n", hr);

    test_format = GetPixelFormat(hdc);
    ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

    if (hdc2)
    {
        test_format = GetPixelFormat(hdc2);
        ok(test_format == format, "second window has pixel format %d, expected %d\n", test_format, format);
    }

cleanup:
    if (primary) IDirectDrawSurface4_Release(primary);
    if (clipper) IDirectDrawClipper_Release(clipper);
    if (ddraw) IDirectDraw4_Release(ddraw);
    if (gl) FreeLibrary(gl);
    if (hdc) ReleaseDC(window, hdc);
    if (hdc2) ReleaseDC(window2, hdc2);
    if (window) DestroyWindow(window);
    if (window2) DestroyWindow(window2);
}

static void test_create_surface_pitch(void)
{
    IDirectDrawSurface4 *surface;
    DDSURFACEDESC2 surface_desc;
    IDirectDraw4 *ddraw;
    unsigned int i;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    void *mem;

    static const struct
    {
        DWORD caps;
        DWORD flags_in;
        DWORD pitch_in;
        HRESULT hr;
        DWORD flags_out;
        DWORD pitch_out32;
        DWORD pitch_out64;
    }
    test_data[] =
    {
        /* 0 */
        {DDSCAPS_VIDEOMEMORY | DDSCAPS_OFFSCREENPLAIN,
                0,                                              0,      DD_OK,
                DDSD_PITCH,                                     0x100,  0x100},
        {DDSCAPS_VIDEOMEMORY | DDSCAPS_OFFSCREENPLAIN,
                DDSD_PITCH,                                     0x104,  DD_OK,
                DDSD_PITCH,                                     0x100,  0x100},
        {DDSCAPS_VIDEOMEMORY | DDSCAPS_OFFSCREENPLAIN,
                DDSD_PITCH,                                     0x0f8,  DD_OK,
                DDSD_PITCH,                                     0x100,  0x100},
        {DDSCAPS_VIDEOMEMORY | DDSCAPS_OFFSCREENPLAIN,
                DDSD_LPSURFACE | DDSD_PITCH,                    0x100,  DDERR_INVALIDCAPS,
                0,                                              0,      0    },
        {DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN,
                0,                                              0,      DD_OK,
                DDSD_PITCH,                                     0x100,  0x0fc},
        /* 5 */
        {DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN,
                DDSD_PITCH,                                     0x104,  DD_OK,
                DDSD_PITCH,                                     0x100,  0x0fc},
        {DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN,
                DDSD_PITCH,                                     0x0f8,  DD_OK,
                DDSD_PITCH,                                     0x100,  0x0fc},
        {DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN,
                DDSD_PITCH | DDSD_LINEARSIZE,                   0,      DD_OK,
                DDSD_PITCH,                                     0x100,  0x0fc},
        {DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN,
                DDSD_LPSURFACE,                                 0,      DDERR_INVALIDPARAMS,
                0,                                              0,      0    },
        {DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN,
                DDSD_LPSURFACE | DDSD_PITCH,                    0x100,  DD_OK,
                DDSD_PITCH,                                     0x100,  0x100},
        /* 10 */
        {DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN,
                DDSD_LPSURFACE | DDSD_PITCH,                    0x0fe,  DDERR_INVALIDPARAMS,
                0,                                              0,      0    },
        {DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN,
                DDSD_LPSURFACE | DDSD_PITCH,                    0x0fc,  DD_OK,
                DDSD_PITCH,                                     0x0fc,  0x0fc},
        {DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN,
                DDSD_LPSURFACE | DDSD_PITCH,                    0x0f8,  DDERR_INVALIDPARAMS,
                0,                                              0,      0    },
        {DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN,
                DDSD_LPSURFACE | DDSD_LINEARSIZE,               0x100,  DDERR_INVALIDPARAMS,
                0,                                              0,      0    },
        {DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN,
                DDSD_LPSURFACE | DDSD_LINEARSIZE,               0x3f00, DDERR_INVALIDPARAMS,
                0,                                              0,      0    },
        /* 15 */
        {DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN,
                DDSD_LPSURFACE | DDSD_PITCH | DDSD_LINEARSIZE,  0x100,  DD_OK,
                DDSD_PITCH,                                     0x100,  0x100},
        {DDSCAPS_VIDEOMEMORY | DDSCAPS_OFFSCREENPLAIN | DDSCAPS_ALLOCONLOAD,
                0,                                              0,      DDERR_INVALIDCAPS,
                0,                                              0,      0    },
        {DDSCAPS_VIDEOMEMORY | DDSCAPS_TEXTURE | DDSCAPS_ALLOCONLOAD,
                0,                                              0,      DD_OK,
                DDSD_PITCH,                                     0x100,  0    },
        {DDSCAPS_VIDEOMEMORY | DDSCAPS_TEXTURE | DDSCAPS_ALLOCONLOAD,
                DDSD_LPSURFACE | DDSD_PITCH,                    0x100,  DDERR_INVALIDCAPS,
                0,                                              0,      0    },
        {DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN | DDSCAPS_ALLOCONLOAD,
                0,                                              0,      DDERR_INVALIDCAPS,
                0,                                              0,      0    },
        /* 20 */
        {DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE | DDSCAPS_ALLOCONLOAD,
                0,                                              0,      DD_OK,
                DDSD_PITCH,                                     0x100,  0    },
        {DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE | DDSCAPS_ALLOCONLOAD,
                DDSD_LPSURFACE | DDSD_PITCH,                    0x100,  DD_OK,
                DDSD_PITCH,                                     0x100,  0    },
    };
    DWORD flags_mask = DDSD_PITCH | DDSD_LPSURFACE | DDSD_LINEARSIZE;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    mem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ((63 * 4) + 8) * 63);

    for (i = 0; i < ARRAY_SIZE(test_data); ++i)
    {
        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | test_data[i].flags_in;
        surface_desc.ddsCaps.dwCaps = test_data[i].caps;
        surface_desc.dwWidth = 63;
        surface_desc.dwHeight = 63;
        U1(surface_desc).lPitch = test_data[i].pitch_in;
        U4(surface_desc).ddpfPixelFormat.dwSize = sizeof(U4(surface_desc).ddpfPixelFormat);
        U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_RGB;
        U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount = 32;
        U2(U4(surface_desc).ddpfPixelFormat).dwRBitMask = 0x00ff0000;
        U3(U4(surface_desc).ddpfPixelFormat).dwGBitMask = 0x0000ff00;
        U4(U4(surface_desc).ddpfPixelFormat).dwBBitMask = 0x000000ff;
        hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
        if (test_data[i].flags_in & DDSD_LPSURFACE)
        {
            HRESULT expected_hr = SUCCEEDED(test_data[i].hr) ? DDERR_INVALIDPARAMS : test_data[i].hr;
            ok(hr == expected_hr, "Test %u: Got unexpected hr %#x, expected %#x.\n", i, hr, expected_hr);
            surface_desc.lpSurface = mem;
            hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
        }
        if ((test_data[i].caps & DDSCAPS_VIDEOMEMORY) && hr == DDERR_NODIRECTDRAWHW)
            continue;
        ok(hr == test_data[i].hr, "Test %u: Got unexpected hr %#x, expected %#x.\n", i, hr, test_data[i].hr);
        if (FAILED(hr))
            continue;

        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        hr = IDirectDrawSurface4_GetSurfaceDesc(surface, &surface_desc);
        ok(SUCCEEDED(hr), "Test %u: Failed to get surface desc, hr %#x.\n", i, hr);
        ok((surface_desc.dwFlags & flags_mask) == test_data[i].flags_out,
                "Test %u: Got unexpected flags %#x, expected %#x.\n",
                i, surface_desc.dwFlags & flags_mask, test_data[i].flags_out);
        /* The pitch for textures seems to be implementation specific. */
        if (!(test_data[i].caps & DDSCAPS_TEXTURE))
        {
            if (is_ddraw64 && test_data[i].pitch_out32 != test_data[i].pitch_out64)
                todo_wine ok(U1(surface_desc).lPitch == test_data[i].pitch_out64,
                        "Test %u: Got unexpected pitch %u, expected %u.\n",
                        i, U1(surface_desc).lPitch, test_data[i].pitch_out64);
            else
                ok(U1(surface_desc).lPitch == test_data[i].pitch_out32,
                        "Test %u: Got unexpected pitch %u, expected %u.\n",
                        i, U1(surface_desc).lPitch, test_data[i].pitch_out32);
        }
        ok(!surface_desc.lpSurface, "Test %u: Got unexpected lpSurface %p.\n", i, surface_desc.lpSurface);

        IDirectDrawSurface4_Release(surface);
    }

    HeapFree(GetProcessHeap(), 0, mem);
    refcount = IDirectDraw4_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_mipmap(void)
{
    IDirectDrawSurface4 *surface, *surface2;
    DDSURFACEDESC2 surface_desc;
    IDirectDraw4 *ddraw;
    unsigned int i;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    DDSCAPS2 caps = {DDSCAPS_COMPLEX, 0, 0, {0}};
    DDCAPS hal_caps;

    static const struct
    {
        DWORD flags;
        DWORD caps;
        DWORD width;
        DWORD height;
        DWORD mipmap_count_in;
        HRESULT hr;
        DWORD mipmap_count_out;
    }
    tests[] =
    {
        {DDSD_MIPMAPCOUNT, DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP, 128, 32, 3, DD_OK,               3},
        {DDSD_MIPMAPCOUNT, DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP, 128, 32, 0, DDERR_INVALIDPARAMS, 0},
        {0,                DDSCAPS_TEXTURE | DDSCAPS_MIPMAP,                   128, 32, 0, DD_OK,               1},
        {0,                DDSCAPS_MIPMAP,                                     128, 32, 0, DDERR_INVALIDCAPS,   0},
        {0,                DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP, 128, 32, 0, DD_OK,               6},
        {0,                DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP, 32,  64, 0, DD_OK,               6},
    };

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&hal_caps, 0, sizeof(hal_caps));
    hal_caps.dwSize = sizeof(hal_caps);
    hr = IDirectDraw4_GetCaps(ddraw, &hal_caps, NULL);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);
    if ((hal_caps.ddsCaps.dwCaps & (DDSCAPS_TEXTURE | DDSCAPS_MIPMAP)) != (DDSCAPS_TEXTURE | DDSCAPS_MIPMAP))
    {
        skip("Mipmapped textures not supported, skipping tests.\n");
        IDirectDraw4_Release(ddraw);
        DestroyWindow(window);
        return;
    }

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | tests[i].flags;
        surface_desc.ddsCaps.dwCaps = tests[i].caps;
        surface_desc.dwWidth = tests[i].width;
        surface_desc.dwHeight = tests[i].height;
        if (tests[i].flags & DDSD_MIPMAPCOUNT)
            U2(surface_desc).dwMipMapCount = tests[i].mipmap_count_in;
        hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
        ok(hr == tests[i].hr, "Test %u: Got unexpected hr %#x.\n", i, hr);
        if (FAILED(hr))
            continue;

        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        hr = IDirectDrawSurface4_GetSurfaceDesc(surface, &surface_desc);
        ok(SUCCEEDED(hr), "Test %u: Failed to get surface desc, hr %#x.\n", i, hr);
        ok(surface_desc.dwFlags & DDSD_MIPMAPCOUNT,
                "Test %u: Got unexpected flags %#x.\n", i, surface_desc.dwFlags);
        ok(U2(surface_desc).dwMipMapCount == tests[i].mipmap_count_out,
                "Test %u: Got unexpected mipmap count %u.\n", i, U2(surface_desc).dwMipMapCount);

        if (U2(surface_desc).dwMipMapCount > 1)
        {
            hr = IDirectDrawSurface4_GetAttachedSurface(surface, &caps, &surface2);
            ok(SUCCEEDED(hr), "Test %u: Failed to get attached surface, hr %#x.\n", i, hr);

            memset(&surface_desc, 0, sizeof(surface_desc));
            surface_desc.dwSize = sizeof(surface_desc);
            hr = IDirectDrawSurface4_Lock(surface, NULL, &surface_desc, 0, NULL);
            ok(SUCCEEDED(hr), "Test %u: Failed to lock surface, hr %#x.\n", i, hr);
            memset(&surface_desc, 0, sizeof(surface_desc));
            surface_desc.dwSize = sizeof(surface_desc);
            hr = IDirectDrawSurface4_Lock(surface2, NULL, &surface_desc, 0, NULL);
            ok(SUCCEEDED(hr), "Test %u: Failed to lock surface, hr %#x.\n", i, hr);
            IDirectDrawSurface4_Unlock(surface2, NULL);
            IDirectDrawSurface4_Unlock(surface, NULL);

            IDirectDrawSurface4_Release(surface2);
        }

        IDirectDrawSurface4_Release(surface);
    }

    refcount = IDirectDraw4_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_palette_complex(void)
{
    IDirectDrawSurface4 *surface, *mipmap, *tmp;
    DDSURFACEDESC2 surface_desc;
    IDirectDraw4 *ddraw;
    IDirectDrawPalette *palette, *palette2, *palette_mipmap;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    DDSCAPS2 caps = {DDSCAPS_COMPLEX, 0, 0, {0}};
    DDCAPS hal_caps;
    PALETTEENTRY palette_entries[256];
    unsigned int i;
    HDC dc;
    RGBQUAD rgbquad;
    UINT count;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&hal_caps, 0, sizeof(hal_caps));
    hal_caps.dwSize = sizeof(hal_caps);
    hr = IDirectDraw4_GetCaps(ddraw, &hal_caps, NULL);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);
    if ((hal_caps.ddsCaps.dwCaps & (DDSCAPS_TEXTURE | DDSCAPS_MIPMAP)) != (DDSCAPS_TEXTURE | DDSCAPS_MIPMAP))
    {
        skip("Mipmapped textures not supported, skipping mipmap palette test.\n");
        IDirectDraw4_Release(ddraw);
        DestroyWindow(window);
        return;
    }

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    surface_desc.dwWidth = 128;
    surface_desc.dwHeight = 128;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
    U4(surface_desc).ddpfPixelFormat.dwSize = sizeof(U4(surface_desc).ddpfPixelFormat);
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED8 | DDPF_RGB;
    U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount = 8;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    memset(palette_entries, 0, sizeof(palette_entries));
    hr = IDirectDraw4_CreatePalette(ddraw, DDPCAPS_8BIT | DDPCAPS_ALLOW256,
            palette_entries, &palette, NULL);
    ok(SUCCEEDED(hr), "Failed to create palette, hr %#x.\n", hr);

    memset(palette_entries, 0, sizeof(palette_entries));
    palette_entries[1].peRed = 0xff;
    palette_entries[1].peGreen = 0x80;
    hr = IDirectDraw4_CreatePalette(ddraw, DDPCAPS_8BIT | DDPCAPS_ALLOW256,
            palette_entries, &palette_mipmap, NULL);
    ok(SUCCEEDED(hr), "Failed to create palette, hr %#x.\n", hr);

    palette2 = (void *)0xdeadbeef;
    hr = IDirectDrawSurface4_GetPalette(surface, &palette2);
    ok(hr == DDERR_NOPALETTEATTACHED, "Got unexpected hr %#x.\n", hr);
    ok(!palette2, "Got unexpected palette %p.\n", palette2);
    hr = IDirectDrawSurface4_SetPalette(surface, palette);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_GetPalette(surface, &palette2);
    ok(SUCCEEDED(hr), "Failed to get palette, hr %#x.\n", hr);
    ok(palette == palette2, "Got unexpected palette %p.\n", palette2);
    IDirectDrawPalette_Release(palette2);

    mipmap = surface;
    IDirectDrawSurface4_AddRef(mipmap);
    for (i = 0; i < 7; ++i)
    {
        hr = IDirectDrawSurface4_GetAttachedSurface(mipmap, &caps, &tmp);
        ok(SUCCEEDED(hr), "Failed to get attached surface, i %u, hr %#x.\n", i, hr);
        palette2 = (void *)0xdeadbeef;
        hr = IDirectDrawSurface4_GetPalette(tmp, &palette2);
        ok(hr == DDERR_NOPALETTEATTACHED, "Got unexpected hr %#x, i %u.\n", hr, i);
        ok(!palette2, "Got unexpected palette %p, i %u.\n", palette2, i);

        hr = IDirectDrawSurface4_SetPalette(tmp, palette_mipmap);
        ok(SUCCEEDED(hr), "Failed to set palette, i %u, hr %#x.\n", i, hr);

        hr = IDirectDrawSurface4_GetPalette(tmp, &palette2);
        ok(SUCCEEDED(hr), "Failed to get palette, i %u, hr %#x.\n", i, hr);
        ok(palette_mipmap == palette2, "Got unexpected palette %p.\n", palette2);
        IDirectDrawPalette_Release(palette2);

        hr = IDirectDrawSurface4_GetDC(tmp, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC, i %u, hr %#x.\n", i, hr);
        count = GetDIBColorTable(dc, 1, 1, &rgbquad);
        ok(count == 1, "Expected count 1, got %u.\n", count);
        ok(rgbquad.rgbRed == 0xff, "Expected rgbRed = 0xff, got %#x.\n", rgbquad.rgbRed);
        ok(rgbquad.rgbGreen == 0x80, "Expected rgbGreen = 0x80, got %#x.\n", rgbquad.rgbGreen);
        ok(rgbquad.rgbBlue == 0x0, "Expected rgbBlue = 0x0, got %#x.\n", rgbquad.rgbBlue);
        hr = IDirectDrawSurface4_ReleaseDC(tmp, dc);
        ok(SUCCEEDED(hr), "Failed to release DC, i %u, hr %#x.\n", i, hr);

        IDirectDrawSurface4_Release(mipmap);
        mipmap = tmp;
    }

    hr = IDirectDrawSurface4_GetAttachedSurface(mipmap, &caps, &tmp);
    ok(hr == DDERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);
    IDirectDrawSurface4_Release(mipmap);
    refcount = IDirectDrawSurface4_Release(surface);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDrawPalette_Release(palette_mipmap);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDrawPalette_Release(palette);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    refcount = IDirectDraw4_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_p8_blit(void)
{
    IDirectDrawSurface4 *src, *dst, *dst_p8;
    DDSURFACEDESC2 surface_desc;
    IDirectDraw4 *ddraw;
    IDirectDrawPalette *palette, *palette2;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    PALETTEENTRY palette_entries[256];
    unsigned int x;
    DDBLTFX fx;
    BOOL is_warp;
    static const BYTE src_data[] = {0x10, 0x1, 0x2, 0x3, 0x4, 0x5, 0xff, 0x80};
    static const BYTE src_data2[] = {0x10, 0x5, 0x4, 0x3, 0x2, 0x1, 0xff, 0x80};
    static const BYTE expected_p8[] = {0x10, 0x1, 0x4, 0x3, 0x4, 0x5, 0xff, 0x80};
    static const D3DCOLOR expected[] =
    {
        0x00101010, 0x00010101, 0x00020202, 0x00030303,
        0x00040404, 0x00050505, 0x00ffffff, 0x00808080,
    };
    D3DCOLOR color;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    is_warp = ddraw_is_warp(ddraw);

    memset(palette_entries, 0, sizeof(palette_entries));
    palette_entries[1].peGreen = 0xff;
    palette_entries[2].peBlue = 0xff;
    palette_entries[3].peFlags = 0xff;
    palette_entries[4].peRed = 0xff;
    hr = IDirectDraw4_CreatePalette(ddraw, DDPCAPS_8BIT | DDPCAPS_ALLOW256,
            palette_entries, &palette, NULL);
    ok(SUCCEEDED(hr), "Failed to create palette, hr %#x.\n", hr);
    palette_entries[1].peBlue = 0xff;
    palette_entries[2].peGreen = 0xff;
    palette_entries[3].peRed = 0xff;
    palette_entries[4].peFlags = 0x0;
    hr = IDirectDraw4_CreatePalette(ddraw, DDPCAPS_8BIT | DDPCAPS_ALLOW256,
            palette_entries, &palette2, NULL);
    ok(SUCCEEDED(hr), "Failed to create palette, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    surface_desc.dwWidth = 8;
    surface_desc.dwHeight = 1;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    U4(surface_desc).ddpfPixelFormat.dwSize = sizeof(U4(surface_desc).ddpfPixelFormat);
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED8 | DDPF_RGB;
    U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount = 8;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &src, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &dst_p8, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_SetPalette(dst_p8, palette2);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    surface_desc.dwWidth = 8;
    surface_desc.dwHeight = 1;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    U4(surface_desc).ddpfPixelFormat.dwSize = sizeof(U4(surface_desc).ddpfPixelFormat);
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount = 32;
    U2(U4(surface_desc).ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(U4(surface_desc).ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(U4(surface_desc).ddpfPixelFormat).dwBBitMask = 0x000000ff;
    U5(U4(surface_desc).ddpfPixelFormat).dwRGBAlphaBitMask = 0xff000000;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &dst, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface4_Lock(src, NULL, &surface_desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock source surface, hr %#x.\n", hr);
    memcpy(surface_desc.lpSurface, src_data, sizeof(src_data));
    hr = IDirectDrawSurface4_Unlock(src, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock source surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_Lock(dst_p8, NULL, &surface_desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock destination surface, hr %#x.\n", hr);
    memcpy(surface_desc.lpSurface, src_data2, sizeof(src_data2));
    hr = IDirectDrawSurface4_Unlock(dst_p8, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock destination surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_SetPalette(src, palette);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Blt(dst, NULL, src, NULL, DDBLT_WAIT, NULL);
    /* The r500 Windows 7 driver returns E_NOTIMPL. r200 on Windows XP works.
     * The Geforce 7 driver on Windows Vista returns E_FAIL. Newer Nvidia GPUs work. */
    ok(SUCCEEDED(hr) || broken(hr == E_NOTIMPL) || broken(hr == E_FAIL),
            "Failed to blit, hr %#x.\n", hr);

    if (SUCCEEDED(hr))
    {
        for (x = 0; x < ARRAY_SIZE(expected); x++)
        {
            color = get_surface_color(dst, x, 0);
            todo_wine ok(compare_color(color, expected[x], 0),
                    "Pixel %u: Got color %#x, expected %#x.\n",
                    x, color, expected[x]);
        }
    }

    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    fx.ddckSrcColorkey.dwColorSpaceHighValue = 0x2;
    fx.ddckSrcColorkey.dwColorSpaceLowValue = 0x2;
    hr = IDirectDrawSurface4_Blt(dst_p8, NULL, src, NULL, DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE, &fx);
    ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_Lock(dst_p8, NULL, &surface_desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock destination surface, hr %#x.\n", hr);
    /* A color keyed P8 blit doesn't do anything on WARP - it just leaves the data in the destination
     * surface untouched. P8 blits without color keys work. Error checking (DDBLT_KEYSRC without a key
     * for example) also works as expected.
     *
     * Using DDBLT_KEYSRC instead of DDBLT_KEYSRCOVERRIDE doesn't change this. Doing this blit with
     * the display mode set to P8 doesn't help either. */
    ok(!memcmp(surface_desc.lpSurface, expected_p8, sizeof(expected_p8))
            || broken(is_warp && !memcmp(surface_desc.lpSurface, src_data2, sizeof(src_data2))),
            "Got unexpected P8 color key blit result.\n");
    hr = IDirectDrawSurface4_Unlock(dst_p8, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock destination surface, hr %#x.\n", hr);

    IDirectDrawSurface4_Release(src);
    IDirectDrawSurface4_Release(dst);
    IDirectDrawSurface4_Release(dst_p8);
    IDirectDrawPalette_Release(palette);
    IDirectDrawPalette_Release(palette2);

    refcount = IDirectDraw4_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_material(void)
{
    D3DMATERIALHANDLE mat_handle, tmp;
    IDirect3DMaterial3 *material;
    IDirect3DViewport3 *viewport;
    IDirect3DDevice3 *device;
    IDirectDrawSurface4 *rt;
    D3DCOLOR color;
    ULONG refcount;
    unsigned int i;
    HWND window;
    HRESULT hr;
    BOOL valid;

    static struct
    {
        struct vec3 position;
        struct vec3 normal;
        D3DCOLOR diffuse;
    }
    quad1[] =
    {
        {{-1.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0xffffffff},
        {{-1.0f,  1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0xffffffff},
        {{ 1.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0xffffffff},
        {{ 1.0f,  1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0xffffffff},
    },
    quad2[] =
    {
        {{-1.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0xffff0000},
        {{-1.0f,  1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0xffff0000},
        {{ 1.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0xffff0000},
        {{ 1.0f,  1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0xffff0000},
    };
    static const struct
    {
        void *data;
        BOOL material;
        D3DCOLOR expected_color;
    }
    test_data[] =
    {
        {quad1, TRUE,  0x0000ff00},
        {quad2, TRUE,  0x0000ff00},
        {quad1, FALSE, 0x00ffffff},
        {quad2, FALSE, 0x00ff0000},
    };
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    viewport = create_viewport(device, 0, 0, 640, 480);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);

    material = create_emissive_material(device, 0.0f, 1.0f, 0.0f, 0.0f);
    hr = IDirect3DMaterial3_GetHandle(material, device, &mat_handle);
    ok(SUCCEEDED(hr), "Failed to get material handle, hr %#x.\n", hr);

    hr = IDirect3DDevice3_GetLightState(device, D3DLIGHTSTATE_MATERIAL, &tmp);
    ok(SUCCEEDED(hr), "Failed to get light state, hr %#x.\n", hr);
    ok(!tmp, "Got unexpected material handle %#x.\n", tmp);
    hr = IDirect3DDevice3_SetLightState(device, D3DLIGHTSTATE_MATERIAL, mat_handle);
    ok(SUCCEEDED(hr), "Failed to set material state, hr %#x.\n", hr);
    hr = IDirect3DDevice3_GetLightState(device, D3DLIGHTSTATE_MATERIAL, &tmp);
    ok(SUCCEEDED(hr), "Failed to get light state, hr %#x.\n", hr);
    ok(tmp == mat_handle, "Got unexpected material handle %#x, expected %#x.\n", tmp, mat_handle);
    hr = IDirect3DDevice3_SetLightState(device, D3DLIGHTSTATE_MATERIAL, 0);
    ok(SUCCEEDED(hr), "Failed to set material state, hr %#x.\n", hr);
    hr = IDirect3DDevice3_GetLightState(device, D3DLIGHTSTATE_MATERIAL, &tmp);
    ok(SUCCEEDED(hr), "Failed to get light state, hr %#x.\n", hr);
    ok(!tmp, "Got unexpected material handle %#x.\n", tmp);

    for (i = 0; i < ARRAY_SIZE(test_data); ++i)
    {
        hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect,
                D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff0000ff, 1.0f, 0);
        ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);

        hr = IDirect3DDevice3_SetLightState(device, D3DLIGHTSTATE_MATERIAL, test_data[i].material ? mat_handle : 0);
        ok(SUCCEEDED(hr), "Failed to set material state, hr %#x.\n", hr);

        hr = IDirect3DDevice3_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
        hr = IDirect3DDevice2_DrawPrimitive(device, D3DPT_TRIANGLESTRIP,
                D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE, test_data[i].data, 4, 0);
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
        hr = IDirect3DDevice3_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);
        color = get_surface_color(rt, 320, 240);
        ok(compare_color(color, test_data[i].expected_color, 1),
                "Got unexpected color 0x%08x, test %u.\n", color, i);
    }

    destroy_material(material);
    material = create_diffuse_material(device, 1.0f, 0.0f, 0.0f, 1.0f);
    hr = IDirect3DMaterial3_GetHandle(material, device, &mat_handle);
    ok(SUCCEEDED(hr), "Failed to get material handle, hr %#x.\n", hr);

    hr = IDirect3DViewport3_SetBackground(viewport, mat_handle);
    ok(SUCCEEDED(hr), "Failed to set viewport background, hr %#x.\n", hr);
    hr = IDirect3DViewport3_GetBackground(viewport, &tmp, &valid);
    ok(SUCCEEDED(hr), "Failed to get viewport background, hr %#x.\n", hr);
    ok(tmp == mat_handle, "Got unexpected material handle %#x, expected %#x.\n", tmp, mat_handle);
    ok(valid, "Got unexpected valid %#x.\n", valid);
    hr = IDirect3DViewport3_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x00ff0000, 1), "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DViewport3_SetBackground(viewport, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DViewport3_GetBackground(viewport, &tmp, &valid);
    ok(SUCCEEDED(hr), "Failed to get viewport background, hr %#x.\n", hr);
    ok(tmp == mat_handle, "Got unexpected material handle %#x, expected %#x.\n", tmp, mat_handle);
    ok(valid, "Got unexpected valid %#x.\n", valid);
    hr = IDirect3DViewport3_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x00ff0000, 1), "Got unexpected color 0x%08x.\n", color);

    destroy_viewport(device, viewport);
    viewport = create_viewport(device, 0, 0, 640, 480);

    hr = IDirect3DViewport3_GetBackground(viewport, &tmp, &valid);
    ok(SUCCEEDED(hr), "Failed to get viewport background, hr %#x.\n", hr);
    ok(!tmp, "Got unexpected material handle %#x.\n", tmp);
    ok(!valid, "Got unexpected valid %#x.\n", valid);
    hr = IDirect3DViewport3_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x00000000, 1), "Got unexpected color 0x%08x.\n", color);

    destroy_viewport(device, viewport);
    destroy_material(material);
    IDirectDrawSurface4_Release(rt);
    refcount = IDirect3DDevice3_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_palette_gdi(void)
{
    IDirectDrawSurface4 *surface, *primary;
    DDSURFACEDESC2 surface_desc;
    IDirectDraw4 *ddraw;
    IDirectDrawPalette *palette, *palette2;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    PALETTEENTRY palette_entries[256];
    UINT i;
    HDC dc;
    DDBLTFX fx;
    RECT r;
    COLORREF color;
    /* On the Windows 8 testbot palette index 0 of the onscreen palette is forced to
     * r = 0, g = 0, b = 0. Do not attempt to set it to something else as this is
     * not the point of this test. */
    static const RGBQUAD expected1[] =
    {
        {0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x01, 0x00}, {0x00, 0x02, 0x00, 0x00},
        {0x03, 0x00, 0x00, 0x00}, {0x15, 0x14, 0x13, 0x00},
    };
    static const RGBQUAD expected2[] =
    {
        {0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x01, 0x00}, {0x00, 0x02, 0x00, 0x00},
        {0x03, 0x00, 0x00, 0x00}, {0x25, 0x24, 0x23, 0x00},
    };
    static const RGBQUAD expected3[] =
    {
        {0x00, 0x00, 0x00, 0x00}, {0x40, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x40, 0x00},
        {0x00, 0x40, 0x00, 0x00}, {0x56, 0x34, 0x12, 0x00},
    };
    HPALETTE ddraw_palette_handle;
    /* Similar to index 0, index 255 is r = 0xff, g = 0xff, b = 0xff on the Win8 VMs. */
    RGBQUAD rgbquad[255];
    static const RGBQUAD rgb_zero = {0, 0, 0, 0};

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    surface_desc.dwWidth = 16;
    surface_desc.dwHeight = 16;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    U4(surface_desc).ddpfPixelFormat.dwSize = sizeof(U4(surface_desc).ddpfPixelFormat);
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED8 | DDPF_RGB;
    U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount = 8;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    /* Avoid colors from the Windows default palette. */
    memset(palette_entries, 0, sizeof(palette_entries));
    palette_entries[1].peRed = 0x01;
    palette_entries[2].peGreen = 0x02;
    palette_entries[3].peBlue = 0x03;
    palette_entries[4].peRed = 0x13;
    palette_entries[4].peGreen = 0x14;
    palette_entries[4].peBlue = 0x15;
    hr = IDirectDraw4_CreatePalette(ddraw, DDPCAPS_8BIT | DDPCAPS_ALLOW256,
            palette_entries, &palette, NULL);
    ok(SUCCEEDED(hr), "Failed to create palette, hr %#x.\n", hr);

    /* If there is no palette assigned and the display mode is not 8 bpp, some
     * drivers refuse to create a DC while others allow it. If a DC is created,
     * the DIB color table is uninitialized and contains random colors. No error
     * is generated when trying to read pixels and random garbage is returned.
     *
     * The most likely explanation is that if the driver creates a DC, it (or
     * the higher-level runtime) uses GetSystemPaletteEntries to find the
     * palette, but GetSystemPaletteEntries fails when bpp > 8 and the palette
     * contains uninitialized garbage. See comments below for the P8 case. */

    hr = IDirectDrawSurface4_SetPalette(surface, palette);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_GetDC(surface, &dc);
    ok(SUCCEEDED(hr), "Failed to get DC, hr %#x.\n", hr);
    ddraw_palette_handle = SelectPalette(dc, GetStockObject(DEFAULT_PALETTE), FALSE);
    ok(ddraw_palette_handle == GetStockObject(DEFAULT_PALETTE),
            "Got unexpected palette %p, expected %p.\n",
            ddraw_palette_handle, GetStockObject(DEFAULT_PALETTE));

    i = GetDIBColorTable(dc, 0, ARRAY_SIZE(rgbquad), rgbquad);
    ok(i == ARRAY_SIZE(rgbquad), "Expected count 255, got %u.\n", i);
    for (i = 0; i < ARRAY_SIZE(expected1); i++)
    {
        ok(!memcmp(&rgbquad[i], &expected1[i], sizeof(rgbquad[i])),
                "Got color table entry %u r=%#x g=%#x b=%#x, expected r=%#x g=%#x b=%#x.\n",
                i, rgbquad[i].rgbRed, rgbquad[i].rgbGreen, rgbquad[i].rgbBlue,
                expected1[i].rgbRed, expected1[i].rgbGreen, expected1[i].rgbBlue);
    }
    for (; i < ARRAY_SIZE(rgbquad); i++)
    {
        ok(!memcmp(&rgbquad[i], &rgb_zero, sizeof(rgbquad[i])),
                "Got color table entry %u r=%#x g=%#x b=%#x, expected r=0 g=0 b=0.\n",
                i, rgbquad[i].rgbRed, rgbquad[i].rgbGreen, rgbquad[i].rgbBlue);
    }

    /* Update the palette while the DC is in use. This does not modify the DC. */
    palette_entries[4].peRed = 0x23;
    palette_entries[4].peGreen = 0x24;
    palette_entries[4].peBlue = 0x25;
    hr = IDirectDrawPalette_SetEntries(palette, 0, 4, 1, &palette_entries[4]);
    ok(SUCCEEDED(hr), "Failed to set palette entries, hr %#x.\n", hr);

    i = GetDIBColorTable(dc, 4, 1, &rgbquad[4]);
    ok(i == 1, "Expected count 1, got %u.\n", i);
    ok(!memcmp(&rgbquad[4], &expected1[4], sizeof(rgbquad[4])),
            "Got color table entry %u r=%#x g=%#x b=%#x, expected r=%#x g=%#x b=%#x.\n",
            i, rgbquad[4].rgbRed, rgbquad[4].rgbGreen, rgbquad[4].rgbBlue,
            expected1[4].rgbRed, expected1[4].rgbGreen, expected1[4].rgbBlue);

    /* Neither does re-setting the palette. */
    hr = IDirectDrawSurface4_SetPalette(surface, NULL);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_SetPalette(surface, palette);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);

    i = GetDIBColorTable(dc, 4, 1, &rgbquad[4]);
    ok(i == 1, "Expected count 1, got %u.\n", i);
    ok(!memcmp(&rgbquad[4], &expected1[4], sizeof(rgbquad[4])),
            "Got color table entry %u r=%#x g=%#x b=%#x, expected r=%#x g=%#x b=%#x.\n",
            i, rgbquad[4].rgbRed, rgbquad[4].rgbGreen, rgbquad[4].rgbBlue,
            expected1[4].rgbRed, expected1[4].rgbGreen, expected1[4].rgbBlue);

    hr = IDirectDrawSurface4_ReleaseDC(surface, dc);
    ok(SUCCEEDED(hr), "Failed to release DC, hr %#x.\n", hr);

    /* Refresh the DC. This updates the palette. */
    hr = IDirectDrawSurface4_GetDC(surface, &dc);
    ok(SUCCEEDED(hr), "Failed to get DC, hr %#x.\n", hr);
    i = GetDIBColorTable(dc, 0, ARRAY_SIZE(rgbquad), rgbquad);
    ok(i == ARRAY_SIZE(rgbquad), "Expected count 255, got %u.\n", i);
    for (i = 0; i < ARRAY_SIZE(expected2); i++)
    {
        ok(!memcmp(&rgbquad[i], &expected2[i], sizeof(rgbquad[i])),
                "Got color table entry %u r=%#x g=%#x b=%#x, expected r=%#x g=%#x b=%#x.\n",
                i, rgbquad[i].rgbRed, rgbquad[i].rgbGreen, rgbquad[i].rgbBlue,
                expected2[i].rgbRed, expected2[i].rgbGreen, expected2[i].rgbBlue);
    }
    for (; i < ARRAY_SIZE(rgbquad); i++)
    {
        ok(!memcmp(&rgbquad[i], &rgb_zero, sizeof(rgbquad[i])),
                "Got color table entry %u r=%#x g=%#x b=%#x, expected r=0 g=0 b=0.\n",
                i, rgbquad[i].rgbRed, rgbquad[i].rgbGreen, rgbquad[i].rgbBlue);
    }
    hr = IDirectDrawSurface4_ReleaseDC(surface, dc);
    ok(SUCCEEDED(hr), "Failed to release DC, hr %#x.\n", hr);

    refcount = IDirectDrawSurface4_Release(surface);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    if (FAILED(IDirectDraw4_SetDisplayMode(ddraw, 640, 480, 8, 0, 0)))
    {
        win_skip("Failed to set 8 bpp display mode, skipping test.\n");
        IDirectDrawPalette_Release(palette);
        IDirectDraw4_Release(ddraw);
        DestroyWindow(window);
        return;
    }
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    U5(fx).dwFillColor = 3;
    SetRect(&r, 0, 0, 319, 479);
    hr = IDirectDrawSurface4_Blt(primary, &r, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear surface, hr %#x.\n", hr);
    SetRect(&r, 320, 0, 639, 479);
    U5(fx).dwFillColor = 4;
    hr = IDirectDrawSurface4_Blt(primary, &r, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_SetPalette(primary, palette);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_GetDC(primary, &dc);
    ok(SUCCEEDED(hr), "Failed to get DC, hr %#x.\n", hr);

    color = GetPixel(dc, 160, 240);
    ok(color == 0x00030000, "Clear index 3: Got unexpected color 0x%08x.\n", color);
    color = GetPixel(dc, 480, 240);
    ok(color == 0x00252423, "Clear index 4: Got unexpected color 0x%08x.\n", color);

    ddraw_palette_handle = SelectPalette(dc, GetStockObject(DEFAULT_PALETTE), FALSE);
    ok(ddraw_palette_handle == GetStockObject(DEFAULT_PALETTE),
            "Got unexpected palette %p, expected %p.\n",
            ddraw_palette_handle, GetStockObject(DEFAULT_PALETTE));
    SelectPalette(dc, ddraw_palette_handle, FALSE);

    /* The primary uses the system palette. In exclusive mode, the system palette matches
     * the ddraw palette attached to the primary, so the result is what you would expect
     * from a regular surface. Tests for the interaction between the ddraw palette and
     * the system palette are not included pending an application that depends on this.
     * The relation between those causes problems on Windows Vista and newer for games
     * like Age of Empires or StarCraft. Don't emulate it without a real need. */
    i = GetDIBColorTable(dc, 0, ARRAY_SIZE(rgbquad), rgbquad);
    ok(i == ARRAY_SIZE(rgbquad), "Expected count 255, got %u.\n", i);
    for (i = 0; i < ARRAY_SIZE(expected2); i++)
    {
        ok(!memcmp(&rgbquad[i], &expected2[i], sizeof(rgbquad[i])),
                "Got color table entry %u r=%#x g=%#x b=%#x, expected r=%#x g=%#x b=%#x.\n",
                i, rgbquad[i].rgbRed, rgbquad[i].rgbGreen, rgbquad[i].rgbBlue,
                expected2[i].rgbRed, expected2[i].rgbGreen, expected2[i].rgbBlue);
    }
    for (; i < ARRAY_SIZE(rgbquad); i++)
    {
        ok(!memcmp(&rgbquad[i], &rgb_zero, sizeof(rgbquad[i])),
                "Got color table entry %u r=%#x g=%#x b=%#x, expected r=0 g=0 b=0.\n",
                i, rgbquad[i].rgbRed, rgbquad[i].rgbGreen, rgbquad[i].rgbBlue);
    }
    hr = IDirectDrawSurface4_ReleaseDC(primary, dc);
    ok(SUCCEEDED(hr), "Failed to release DC, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.dwWidth = 16;
    surface_desc.dwHeight = 16;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    /* Here the offscreen surface appears to use the primary's palette,
     * but in all likelihood it is actually the system palette. */
    hr = IDirectDrawSurface4_GetDC(surface, &dc);
    ok(SUCCEEDED(hr), "Failed to get DC, hr %#x.\n", hr);
    i = GetDIBColorTable(dc, 0, ARRAY_SIZE(rgbquad), rgbquad);
    ok(i == ARRAY_SIZE(rgbquad), "Expected count 255, got %u.\n", i);
    for (i = 0; i < ARRAY_SIZE(expected2); i++)
    {
        ok(!memcmp(&rgbquad[i], &expected2[i], sizeof(rgbquad[i])),
                "Got color table entry %u r=%#x g=%#x b=%#x, expected r=%#x g=%#x b=%#x.\n",
                i, rgbquad[i].rgbRed, rgbquad[i].rgbGreen, rgbquad[i].rgbBlue,
                expected2[i].rgbRed, expected2[i].rgbGreen, expected2[i].rgbBlue);
    }
    for (; i < ARRAY_SIZE(rgbquad); i++)
    {
        ok(!memcmp(&rgbquad[i], &rgb_zero, sizeof(rgbquad[i])),
                "Got color table entry %u r=%#x g=%#x b=%#x, expected r=0 g=0 b=0.\n",
                i, rgbquad[i].rgbRed, rgbquad[i].rgbGreen, rgbquad[i].rgbBlue);
    }
    hr = IDirectDrawSurface4_ReleaseDC(surface, dc);
    ok(SUCCEEDED(hr), "Failed to release DC, hr %#x.\n", hr);

    /* On real hardware a change to the primary surface's palette applies immediately,
     * even on device contexts from offscreen surfaces that do not have their own
     * palette. On the testbot VMs this is not the case. Don't test this until we
     * know of an application that depends on this. */

    memset(palette_entries, 0, sizeof(palette_entries));
    palette_entries[1].peBlue = 0x40;
    palette_entries[2].peRed = 0x40;
    palette_entries[3].peGreen = 0x40;
    palette_entries[4].peRed = 0x12;
    palette_entries[4].peGreen = 0x34;
    palette_entries[4].peBlue = 0x56;
    hr = IDirectDraw4_CreatePalette(ddraw, DDPCAPS_8BIT | DDPCAPS_ALLOW256,
            palette_entries, &palette2, NULL);
    ok(SUCCEEDED(hr), "Failed to create palette, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_SetPalette(surface, palette2);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);

    /* A palette assigned to the offscreen surface overrides the primary / system
     * palette. */
    hr = IDirectDrawSurface4_GetDC(surface, &dc);
    ok(SUCCEEDED(hr), "Failed to get DC, hr %#x.\n", hr);
    i = GetDIBColorTable(dc, 0, ARRAY_SIZE(rgbquad), rgbquad);
    ok(i == ARRAY_SIZE(rgbquad), "Expected count 255, got %u.\n", i);
    for (i = 0; i < ARRAY_SIZE(expected3); i++)
    {
        ok(!memcmp(&rgbquad[i], &expected3[i], sizeof(rgbquad[i])),
                "Got color table entry %u r=%#x g=%#x b=%#x, expected r=%#x g=%#x b=%#x.\n",
                i, rgbquad[i].rgbRed, rgbquad[i].rgbGreen, rgbquad[i].rgbBlue,
                expected3[i].rgbRed, expected3[i].rgbGreen, expected3[i].rgbBlue);
    }
    for (; i < ARRAY_SIZE(rgbquad); i++)
    {
        ok(!memcmp(&rgbquad[i], &rgb_zero, sizeof(rgbquad[i])),
                "Got color table entry %u r=%#x g=%#x b=%#x, expected r=0 g=0 b=0.\n",
                i, rgbquad[i].rgbRed, rgbquad[i].rgbGreen, rgbquad[i].rgbBlue);
    }
    hr = IDirectDrawSurface4_ReleaseDC(surface, dc);
    ok(SUCCEEDED(hr), "Failed to release DC, hr %#x.\n", hr);

    refcount = IDirectDrawSurface4_Release(surface);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    /* The Windows 8 testbot keeps extra references to the primary and
     * backbuffer while in 8 bpp mode. */
    hr = IDirectDraw4_RestoreDisplayMode(ddraw);
    ok(SUCCEEDED(hr), "Failed to restore display mode, hr %#x.\n", hr);

    refcount = IDirectDrawSurface4_Release(primary);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDrawPalette_Release(palette2);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDrawPalette_Release(palette);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDraw4_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_palette_alpha(void)
{
    IDirectDrawSurface4 *surface;
    DDSURFACEDESC2 surface_desc;
    IDirectDraw4 *ddraw;
    IDirectDrawPalette *palette;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    PALETTEENTRY palette_entries[256];
    unsigned int i;
    static const struct
    {
        DWORD caps, flags;
        BOOL attach_allowed;
        const char *name;
    }
    test_data[] =
    {
        {DDSCAPS_OFFSCREENPLAIN, DDSD_WIDTH | DDSD_HEIGHT, FALSE, "offscreenplain"},
        {DDSCAPS_TEXTURE, DDSD_WIDTH | DDSD_HEIGHT, TRUE, "texture"},
        {DDSCAPS_PRIMARYSURFACE, 0, FALSE, "primary"}
    };

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    if (FAILED(IDirectDraw4_SetDisplayMode(ddraw, 640, 480, 8, 0, 0)))
    {
        win_skip("Failed to set 8 bpp display mode, skipping test.\n");
        IDirectDraw4_Release(ddraw);
        DestroyWindow(window);
        return;
    }
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(palette_entries, 0, sizeof(palette_entries));
    palette_entries[1].peFlags = 0x42;
    palette_entries[2].peFlags = 0xff;
    palette_entries[3].peFlags = 0x80;
    hr = IDirectDraw4_CreatePalette(ddraw, DDPCAPS_ALLOW256 | DDPCAPS_8BIT, palette_entries, &palette, NULL);
    ok(SUCCEEDED(hr), "Failed to create palette, hr %#x.\n", hr);

    memset(palette_entries, 0x66, sizeof(palette_entries));
    hr = IDirectDrawPalette_GetEntries(palette, 0, 1, 4, palette_entries);
    ok(SUCCEEDED(hr), "Failed to get palette entries, hr %#x.\n", hr);
    ok(palette_entries[0].peFlags == 0x42, "Got unexpected peFlags 0x%02x, expected 0xff.\n",
            palette_entries[0].peFlags);
    ok(palette_entries[1].peFlags == 0xff, "Got unexpected peFlags 0x%02x, expected 0xff.\n",
            palette_entries[1].peFlags);
    ok(palette_entries[2].peFlags == 0x80, "Got unexpected peFlags 0x%02x, expected 0x80.\n",
            palette_entries[2].peFlags);
    ok(palette_entries[3].peFlags == 0x00, "Got unexpected peFlags 0x%02x, expected 0x00.\n",
            palette_entries[3].peFlags);

    IDirectDrawPalette_Release(palette);

    memset(palette_entries, 0, sizeof(palette_entries));
    palette_entries[1].peFlags = 0x42;
    palette_entries[1].peRed   = 0xff;
    palette_entries[2].peFlags = 0xff;
    palette_entries[3].peFlags = 0x80;
    hr = IDirectDraw4_CreatePalette(ddraw, DDPCAPS_ALLOW256 | DDPCAPS_8BIT | DDPCAPS_ALPHA,
            palette_entries, &palette, NULL);
    ok(SUCCEEDED(hr), "Failed to create palette, hr %#x.\n", hr);

    memset(palette_entries, 0x66, sizeof(palette_entries));
    hr = IDirectDrawPalette_GetEntries(palette, 0, 1, 4, palette_entries);
    ok(SUCCEEDED(hr), "Failed to get palette entries, hr %#x.\n", hr);
    ok(palette_entries[0].peFlags == 0x42, "Got unexpected peFlags 0x%02x, expected 0xff.\n",
            palette_entries[0].peFlags);
    ok(palette_entries[1].peFlags == 0xff, "Got unexpected peFlags 0x%02x, expected 0xff.\n",
            palette_entries[1].peFlags);
    ok(palette_entries[2].peFlags == 0x80, "Got unexpected peFlags 0x%02x, expected 0x80.\n",
            palette_entries[2].peFlags);
    ok(palette_entries[3].peFlags == 0x00, "Got unexpected peFlags 0x%02x, expected 0x00.\n",
            palette_entries[3].peFlags);

    for (i = 0; i < ARRAY_SIZE(test_data); i++)
    {
        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS | test_data[i].flags;
        surface_desc.dwWidth = 128;
        surface_desc.dwHeight = 128;
        surface_desc.ddsCaps.dwCaps = test_data[i].caps;
        hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
        ok(SUCCEEDED(hr), "Failed to create %s surface, hr %#x.\n", test_data[i].name, hr);

        hr = IDirectDrawSurface4_SetPalette(surface, palette);
        if (test_data[i].attach_allowed)
            ok(SUCCEEDED(hr), "Failed to attach palette to %s surface, hr %#x.\n", test_data[i].name, hr);
        else
            ok(hr == DDERR_INVALIDSURFACETYPE, "Got unexpected hr %#x, %s surface.\n", hr, test_data[i].name);

        if (SUCCEEDED(hr))
        {
            HDC dc;
            RGBQUAD rgbquad;
            UINT retval;

            hr = IDirectDrawSurface4_GetDC(surface, &dc);
            ok(SUCCEEDED(hr), "Failed to get DC, hr %#x, %s surface.\n", hr, test_data[i].name);
            retval = GetDIBColorTable(dc, 1, 1, &rgbquad);
            ok(retval == 1, "GetDIBColorTable returned unexpected result %u.\n", retval);
            ok(rgbquad.rgbRed == 0xff, "Expected rgbRed = 0xff, got %#x, %s surface.\n",
                    rgbquad.rgbRed, test_data[i].name);
            ok(rgbquad.rgbGreen == 0, "Expected rgbGreen = 0, got %#x, %s surface.\n",
                    rgbquad.rgbGreen, test_data[i].name);
            ok(rgbquad.rgbBlue == 0, "Expected rgbBlue = 0, got %#x, %s surface.\n",
                    rgbquad.rgbBlue, test_data[i].name);
            ok(rgbquad.rgbReserved == 0, "Expected rgbReserved = 0, got %u, %s surface.\n",
                    rgbquad.rgbReserved, test_data[i].name);
            hr = IDirectDrawSurface4_ReleaseDC(surface, dc);
            ok(SUCCEEDED(hr), "Failed to release DC, hr %#x.\n", hr);
        }
        IDirectDrawSurface4_Release(surface);
    }

    /* Test INVALIDSURFACETYPE vs INVALIDPIXELFORMAT. */
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    surface_desc.dwWidth = 128;
    surface_desc.dwHeight = 128;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    U4(surface_desc).ddpfPixelFormat.dwSize = sizeof(U4(surface_desc).ddpfPixelFormat);
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount = 32;
    U2(U4(surface_desc).ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(U4(surface_desc).ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(U4(surface_desc).ddpfPixelFormat).dwBBitMask = 0x000000ff;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_SetPalette(surface, palette);
    ok(hr == DDERR_INVALIDSURFACETYPE, "Got unexpected hr %#x.\n", hr);
    IDirectDrawSurface4_Release(surface);

    /* The Windows 8 testbot keeps extra references to the primary
     * while in 8 bpp mode. */
    hr = IDirectDraw4_RestoreDisplayMode(ddraw);
    ok(SUCCEEDED(hr), "Failed to restore display mode, hr %#x.\n", hr);

    refcount = IDirectDrawPalette_Release(palette);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDraw4_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_vb_writeonly(void)
{
    IDirect3DDevice3 *device;
    IDirect3D3 *d3d;
    IDirect3DVertexBuffer *buffer;
    HWND window;
    HRESULT hr;
    D3DVERTEXBUFFERDESC desc;
    void *ptr;
    static const struct vec4 quad[] =
    {
        {  0.0f, 480.0f, 0.0f, 1.0f},
        {  0.0f,   0.0f, 0.0f, 1.0f},
        {640.0f, 480.0f, 0.0f, 1.0f},
        {640.0f,   0.0f, 0.0f, 1.0f},
    };

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get d3d interface, hr %#x.\n", hr);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwCaps = D3DVBCAPS_WRITEONLY;
    desc.dwFVF = D3DFVF_XYZRHW;
    desc.dwNumVertices = ARRAY_SIZE(quad);
    hr = IDirect3D3_CreateVertexBuffer(d3d, &desc, &buffer, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to create vertex buffer, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer_Lock(buffer, DDLOCK_DISCARDCONTENTS, &ptr, NULL);
    ok(SUCCEEDED(hr), "Failed to lock vertex buffer, hr %#x.\n", hr);
    memcpy(ptr, quad, sizeof(quad));
    hr = IDirect3DVertexBuffer_Unlock(buffer);
    ok(SUCCEEDED(hr), "Failed to unlock vertex buffer, hr %#x.\n", hr);

    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitiveVB(device, D3DPT_TRIANGLESTRIP, buffer, 0, 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer_Lock(buffer, 0, &ptr, NULL);
    ok(SUCCEEDED(hr), "Failed to lock vertex buffer, hr %#x.\n", hr);
    ok (!memcmp(ptr, quad, sizeof(quad)), "Got unexpected vertex buffer data.\n");
    hr = IDirect3DVertexBuffer_Unlock(buffer);
    ok(SUCCEEDED(hr), "Failed to unlock vertex buffer, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer_Lock(buffer, DDLOCK_READONLY, &ptr, NULL);
    ok(SUCCEEDED(hr), "Failed to lock vertex buffer, hr %#x.\n", hr);
    ok (!memcmp(ptr, quad, sizeof(quad)), "Got unexpected vertex buffer data.\n");
    hr = IDirect3DVertexBuffer_Unlock(buffer);
    ok(SUCCEEDED(hr), "Failed to unlock vertex buffer, hr %#x.\n", hr);

    IDirect3DVertexBuffer_Release(buffer);
    IDirect3D3_Release(d3d);
    IDirect3DDevice3_Release(device);
    DestroyWindow(window);
}

static void test_lost_device(void)
{
    IDirectDrawSurface4 *surface;
    DDSURFACEDESC2 surface_desc;
    HWND window1, window2;
    IDirectDraw4 *ddraw;
    ULONG refcount;
    HRESULT hr;
    BOOL ret;

    window1 = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    window2 = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window1, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;
    U5(surface_desc).dwBackBufferCount = 1;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDraw4_TestCooperativeLevel(ddraw);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_IsLost(surface);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Flip(surface, NULL, DDFLIP_WAIT);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);

    ret = SetForegroundWindow(GetDesktopWindow());
    ok(ret, "Failed to set foreground window.\n");
    hr = IDirectDraw4_TestCooperativeLevel(ddraw);
    ok(hr == DDERR_NOEXCLUSIVEMODE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_IsLost(surface);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Flip(surface, NULL, DDFLIP_WAIT);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);

    ret = SetForegroundWindow(window1);
    ok(ret, "Failed to set foreground window.\n");
    hr = IDirectDraw4_TestCooperativeLevel(ddraw);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_IsLost(surface);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Flip(surface, NULL, DDFLIP_WAIT);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);

    hr = IDirectDraw4_RestoreAllSurfaces(ddraw);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDraw4_TestCooperativeLevel(ddraw);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_IsLost(surface);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Flip(surface, NULL, DDFLIP_WAIT);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window1, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDraw4_TestCooperativeLevel(ddraw);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_IsLost(surface);
    todo_wine ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Flip(surface, NULL, DDFLIP_WAIT);
    todo_wine ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);

    /* Trying to restore the primary will crash, probably because flippable
     * surfaces can't exist in DDSCL_NORMAL. */
    IDirectDrawSurface4_Release(surface);
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDraw4_TestCooperativeLevel(ddraw);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_IsLost(surface);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);

    ret = SetForegroundWindow(GetDesktopWindow());
    ok(ret, "Failed to set foreground window.\n");
    hr = IDirectDraw4_TestCooperativeLevel(ddraw);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_IsLost(surface);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);

    ret = SetForegroundWindow(window1);
    ok(ret, "Failed to set foreground window.\n");
    hr = IDirectDraw4_TestCooperativeLevel(ddraw);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_IsLost(surface);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window1, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDraw4_TestCooperativeLevel(ddraw);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_IsLost(surface);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);

    hr = IDirectDraw4_RestoreAllSurfaces(ddraw);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDraw4_TestCooperativeLevel(ddraw);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_IsLost(surface);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);

    IDirectDrawSurface4_Release(surface);
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;
    U5(surface_desc).dwBackBufferCount = 1;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window1, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDraw4_TestCooperativeLevel(ddraw);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_IsLost(surface);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Flip(surface, NULL, DDFLIP_WAIT);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window1, DDSCL_NORMAL | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDraw4_TestCooperativeLevel(ddraw);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_IsLost(surface);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Flip(surface, NULL, DDFLIP_WAIT);
    ok(hr == DDERR_NOEXCLUSIVEMODE, "Got unexpected hr %#x.\n", hr);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window1, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDraw4_TestCooperativeLevel(ddraw);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_IsLost(surface);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Flip(surface, NULL, DDFLIP_WAIT);
    ok(hr == DDERR_NOEXCLUSIVEMODE, "Got unexpected hr %#x.\n", hr);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window2, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDraw4_TestCooperativeLevel(ddraw);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_IsLost(surface);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Flip(surface, NULL, DDFLIP_WAIT);
    ok(hr == DDERR_NOEXCLUSIVEMODE, "Got unexpected hr %#x.\n", hr);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window2, DDSCL_NORMAL | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDraw4_TestCooperativeLevel(ddraw);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_IsLost(surface);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Flip(surface, NULL, DDFLIP_WAIT);
    ok(hr == DDERR_NOEXCLUSIVEMODE, "Got unexpected hr %#x.\n", hr);

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window2, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDraw4_TestCooperativeLevel(ddraw);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_IsLost(surface);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Flip(surface, NULL, DDFLIP_WAIT);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);

    IDirectDrawSurface4_Release(surface);
    refcount = IDirectDraw4_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window2);
    DestroyWindow(window1);
}

static void test_surface_desc_lock(void)
{
    IDirectDrawSurface4 *surface;
    DDSURFACEDESC2 surface_desc;
    IDirectDraw4 *ddraw;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.dwWidth = 16;
    surface_desc.dwHeight = 16;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    memset(&surface_desc, 0xaa, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface4_GetSurfaceDesc(surface, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(!surface_desc.lpSurface, "Got unexpected lpSurface %p.\n", surface_desc.lpSurface);

    memset(&surface_desc, 0xaa, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface4_Lock(surface, NULL, &surface_desc, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    ok(surface_desc.lpSurface != NULL, "Got unexpected lpSurface %p.\n", surface_desc.lpSurface);
    memset(&surface_desc, 0xaa, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface4_GetSurfaceDesc(surface, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(!surface_desc.lpSurface, "Got unexpected lpSurface %p.\n", surface_desc.lpSurface);
    hr = IDirectDrawSurface4_Unlock(surface, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    memset(&surface_desc, 0xaa, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface4_GetSurfaceDesc(surface, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(!surface_desc.lpSurface, "Got unexpected lpSurface %p.\n", surface_desc.lpSurface);

    IDirectDrawSurface4_Release(surface);
    refcount = IDirectDraw4_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_texturemapblend(void)
{
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    static RECT rect = {0, 0, 64, 128};
    IDirectDrawSurface4 *surface, *rt;
    IDirect3DViewport3 *viewport;
    DDSURFACEDESC2 surface_desc;
    IDirect3DTexture2 *texture;
    IDirect3DDevice3 *device;
    IDirectDraw4 *ddraw;
    IDirect3D3 *d3d;
    DDCOLORKEY ckey;
    D3DCOLOR color;
    ULONG refcount;
    HWND window;
    DDBLTFX fx;
    HRESULT hr;

    static struct
    {
        struct vec4 position;
        D3DCOLOR diffuse;
        struct vec2 texcoord;
    }
    test1_quads[] =
    {
        {{  0.0f,   0.0f, 0.0f, 1.0f}, 0xffffffff, {0.0f, 0.0f}},
        {{  0.0f, 240.0f, 0.0f, 1.0f}, 0xffffffff, {0.0f, 1.0f}},
        {{640.0f,   0.0f, 0.0f, 1.0f}, 0xffffffff, {1.0f, 0.0f}},
        {{640.0f, 240.0f, 0.0f, 1.0f}, 0xffffffff, {1.0f, 1.0f}},
        {{  0.0f, 240.0f, 0.0f, 1.0f}, 0x80ffffff, {0.0f, 0.0f}},
        {{  0.0f, 480.0f, 0.0f, 1.0f}, 0x80ffffff, {0.0f, 1.0f}},
        {{640.0f, 240.0f, 0.0f, 1.0f}, 0x80ffffff, {1.0f, 0.0f}},
        {{640.0f, 480.0f, 0.0f, 1.0f}, 0x80ffffff, {1.0f, 1.0f}},
    },
    test2_quads[] =
    {
        {{  0.0f,   0.0f, 0.0f, 1.0f}, 0x00ff0080, {0.0f, 0.0f}},
        {{  0.0f, 240.0f, 0.0f, 1.0f}, 0x00ff0080, {0.0f, 1.0f}},
        {{640.0f,   0.0f, 0.0f, 1.0f}, 0x00ff0080, {1.0f, 0.0f}},
        {{640.0f, 240.0f, 0.0f, 1.0f}, 0x00ff0080, {1.0f, 1.0f}},
        {{  0.0f, 240.0f, 0.0f, 1.0f}, 0x008000ff, {0.0f, 0.0f}},
        {{  0.0f, 480.0f, 0.0f, 1.0f}, 0x008000ff, {0.0f, 1.0f}},
        {{640.0f, 240.0f, 0.0f, 1.0f}, 0x008000ff, {1.0f, 0.0f}},
        {{640.0f, 480.0f, 0.0f, 1.0f}, 0x008000ff, {1.0f, 1.0f}},
    };

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get d3d interface, hr %#x.\n", hr);
    hr = IDirect3D3_QueryInterface(d3d, &IID_IDirectDraw4, (void **)&ddraw);
    ok(SUCCEEDED(hr), "Failed to get ddraw interface, hr %#x.\n", hr);
    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    viewport = create_viewport(device, 0, 0, 640, 480);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);

    /* Test alpha with DDPF_ALPHAPIXELS texture - should be taken from texture
     * alpha channel.
     *
     * The vertex alpha is completely ignored in this case, so case 1 and 2
     * combined are not a D3DTOP_MODULATE with texture alpha = 0xff in case 2
     * (no alpha in texture). */
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    surface_desc.dwHeight = 128;
    surface_desc.dwWidth = 128;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    U4(surface_desc).ddpfPixelFormat.dwSize = sizeof(U4(surface_desc).ddpfPixelFormat);
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount = 32;
    U2(U4(surface_desc).ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(U4(surface_desc).ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(U4(surface_desc).ddpfPixelFormat).dwBBitMask = 0x000000ff;
    U5(U4(surface_desc).ddpfPixelFormat).dwRGBAlphaBitMask = 0xff000000;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_QueryInterface(surface, &IID_IDirect3DTexture2, (void **)&texture);
    ok(SUCCEEDED(hr), "Failed to get texture interface, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTexture(device, 0, texture);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);

    hr = IDirect3DViewport3_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear render target, hr %#x.\n", hr);

    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    U5(fx).dwFillColor = 0xff0000ff;
    hr = IDirectDrawSurface4_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear texture, hr %#x.\n", hr);
    U5(fx).dwFillColor = 0x800000ff;
    hr = IDirectDrawSurface4_Blt(surface, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear texture, hr %#x.\n", hr);

    /* Note that the ddraw1 version of this test runs tests 1-3 with
     * D3DRENDERSTATE_COLORKEYENABLE enabled, whereas this version only runs
     * test 4 with color keying on. Because no color key is set on the texture
     * this should not result in different behavior. */
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_ZENABLE, D3DZB_FALSE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_TEXTUREMAPBLEND, D3DTBLEND_MODULATE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);

    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP,
            D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1, &test1_quads[0], 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP,
            D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1, &test1_quads[4], 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = get_surface_color(rt, 5, 5);
    ok(compare_color(color, 0x00000080, 2), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 400, 5);
    ok(compare_color(color, 0x000000ff, 2), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 5, 245);
    ok(compare_color(color, 0x00000080, 2), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 400, 245);
    ok(compare_color(color, 0x000000ff, 2), "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DDevice3_SetTexture(device, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    IDirect3DTexture2_Release(texture);
    refcount = IDirectDrawSurface4_Release(surface);
    ok(!refcount, "Surface not properly released, refcount %u.\n", refcount);

    /* Test alpha with texture that has no alpha channel - alpha should be
     * taken from diffuse vertex color. */
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    surface_desc.dwHeight = 128;
    surface_desc.dwWidth = 128;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    U4(surface_desc).ddpfPixelFormat.dwSize = sizeof(U4(surface_desc).ddpfPixelFormat);
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount = 32;
    U2(U4(surface_desc).ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(U4(surface_desc).ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(U4(surface_desc).ddpfPixelFormat).dwBBitMask = 0x000000ff;

    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_QueryInterface(surface, &IID_IDirect3DTexture2, (void **)&texture);
    ok(SUCCEEDED(hr), "Failed to get texture interface, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTexture(device, 0, texture);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);

    hr = IDirect3DViewport3_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear render target, hr %#x.\n", hr);

    U5(fx).dwFillColor = 0xff0000ff;
    hr = IDirectDrawSurface4_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear texture, hr %#x.\n", hr);
    U5(fx).dwFillColor = 0x800000ff;
    hr = IDirectDrawSurface4_Blt(surface, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear texture, hr %#x.\n", hr);

    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP,
            D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1, &test1_quads[0], 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP,
            D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1, &test1_quads[4], 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = get_surface_color(rt, 5, 5);
    ok(compare_color(color, 0x000000ff, 2), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 400, 5);
    ok(compare_color(color, 0x000000ff, 2), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 5, 245);
    ok(compare_color(color, 0x00000080, 2), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 400, 245);
    ok(compare_color(color, 0x00000080, 2), "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DDevice3_SetTexture(device, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    IDirect3DTexture2_Release(texture);
    refcount = IDirectDrawSurface4_Release(surface);
    ok(!refcount, "Surface not properly released, refcount %u.\n", refcount);

    /* Test RGB - should multiply color components from diffuse vertex color
     * and texture. */
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    surface_desc.dwHeight = 128;
    surface_desc.dwWidth = 128;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    U4(surface_desc).ddpfPixelFormat.dwSize = sizeof(U4(surface_desc).ddpfPixelFormat);
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount = 32;
    U2(U4(surface_desc).ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(U4(surface_desc).ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(U4(surface_desc).ddpfPixelFormat).dwBBitMask = 0x000000ff;
    U5(U4(surface_desc).ddpfPixelFormat).dwRGBAlphaBitMask = 0xff000000;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_QueryInterface(surface, &IID_IDirect3DTexture2, (void **)&texture);
    ok(SUCCEEDED(hr), "Failed to get texture interface, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTexture(device, 0, texture);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);

    hr = IDirect3DViewport3_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear render target, hr %#x.\n", hr);

    U5(fx).dwFillColor = 0x00ffffff;
    hr = IDirectDrawSurface4_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear texture, hr %#x.\n", hr);
    U5(fx).dwFillColor = 0x00ffff80;
    hr = IDirectDrawSurface4_Blt(surface, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear texture, hr %#x.\n", hr);

    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);

    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP,
            D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1, &test2_quads[0], 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP,
            D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1, &test2_quads[4], 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = get_surface_color(rt, 5, 5);
    ok(compare_color(color, 0x00ff0040, 2), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 400, 5);
    ok(compare_color(color, 0x00ff0080, 2), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 5, 245);
    ok(compare_color(color, 0x00800080, 2), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 400, 245);
    ok(compare_color(color, 0x008000ff, 2), "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DDevice3_SetTexture(device, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    IDirect3DTexture2_Release(texture);
    refcount = IDirectDrawSurface4_Release(surface);
    ok(!refcount, "Surface not properly released, refcount %u.\n", refcount);

    /* Test alpha again, now with color keyed texture (colorkey emulation in
     * wine can interfere). */
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    surface_desc.dwHeight = 128;
    surface_desc.dwWidth = 128;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    U4(surface_desc).ddpfPixelFormat.dwSize = sizeof(U4(surface_desc).ddpfPixelFormat);
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount = 16;
    U2(U4(surface_desc).ddpfPixelFormat).dwRBitMask = 0xf800;
    U3(U4(surface_desc).ddpfPixelFormat).dwGBitMask = 0x07e0;
    U4(U4(surface_desc).ddpfPixelFormat).dwBBitMask = 0x001f;

    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_QueryInterface(surface, &IID_IDirect3DTexture2, (void **)&texture);
    ok(SUCCEEDED(hr), "Failed to get texture interface, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTexture(device, 0, texture);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);

    hr = IDirect3DViewport3_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear render target, hr %#x.\n", hr);

    U5(fx).dwFillColor = 0xf800;
    hr = IDirectDrawSurface4_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear texture, hr %#x.\n", hr);
    U5(fx).dwFillColor = 0x001f;
    hr = IDirectDrawSurface4_Blt(surface, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear texture, hr %#x.\n", hr);

    ckey.dwColorSpaceLowValue = 0x001f;
    ckey.dwColorSpaceHighValue = 0x001f;
    hr = IDirectDrawSurface4_SetColorKey(surface, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);

    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_COLORKEYENABLE, TRUE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);

    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP,
            D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1, &test1_quads[0], 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP,
            D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1, &test1_quads[4], 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = get_surface_color(rt, 5, 5);
    ok(compare_color(color, 0x00000000, 2), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 400, 5);
    ok(compare_color(color, 0x00ff0000, 2), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 5, 245);
    ok(compare_color(color, 0x00000000, 2), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 400, 245);
    ok(compare_color(color, 0x00800000, 2), "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DDevice3_SetTexture(device, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    IDirect3DTexture2_Release(texture);
    refcount = IDirectDrawSurface4_Release(surface);
    ok(!refcount, "Surface not properly released, refcount %u.\n", refcount);

    destroy_viewport(device, viewport);
    IDirectDrawSurface4_Release(rt);
    IDirect3DDevice3_Release(device);
    IDirect3D3_Release(d3d);
    refcount = IDirectDraw4_Release(ddraw);
    ok(!refcount, "Ddraw object not properly released, refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_signed_formats(void)
{
    HRESULT hr;
    IDirect3DDevice3 *device;
    IDirect3D3 *d3d;
    IDirectDraw4 *ddraw;
    IDirectDrawSurface4 *surface, *rt;
    IDirect3DTexture2 *texture;
    IDirect3DViewport3 *viewport;
    DDSURFACEDESC2 surface_desc;
    ULONG refcount;
    HWND window;
    D3DCOLOR color, expected_color;
    D3DRECT clear_rect;
    static struct
    {
        struct vec3 position;
        struct vec2 texcoord;
    }
    quad[] =
    {
        {{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
        {{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
        {{ 1.0f,  1.0f, 0.0f}, {1.0f, 0.0f}},
    };
    /* See test_signed_formats() in dlls/d3d9/tests/visual.c for an explanation
     * of these values. */
    static const USHORT content_v8u8[4][4] =
    {
        {0x0000, 0x7f7f, 0x8880, 0x0000},
        {0x0080, 0x8000, 0x7f00, 0x007f},
        {0x193b, 0xe8c8, 0x0808, 0xf8f8},
        {0x4444, 0xc0c0, 0xa066, 0x22e0},
    };
    static const DWORD content_x8l8v8u8[4][4] =
    {
        {0x00000000, 0x00ff7f7f, 0x00008880, 0x00ff0000},
        {0x00000080, 0x00008000, 0x00007f00, 0x0000007f},
        {0x0041193b, 0x0051e8c8, 0x00040808, 0x00fff8f8},
        {0x00824444, 0x0000c0c0, 0x00c2a066, 0x009222e0},
    };
    static const USHORT content_l6v5u5[4][4] =
    {
        {0x0000, 0xfdef, 0x0230, 0xfc00},
        {0x0010, 0x0200, 0x01e0, 0x000f},
        {0x4067, 0x53b9, 0x0421, 0xffff},
        {0x8108, 0x0318, 0xc28c, 0x909c},
    };
    static const struct
    {
        const char *name;
        const void *content;
        SIZE_T pixel_size;
        BOOL blue;
        unsigned int slop, slop_broken;
        DDPIXELFORMAT format;
    }
    formats[] =
    {
        {
            "D3DFMT_V8U8",     content_v8u8,     sizeof(WORD),  FALSE, 1, 0,
            {
                sizeof(DDPIXELFORMAT), DDPF_BUMPDUDV, 0,
                {16}, {0x000000ff}, {0x0000ff00}, {0x00000000}, {0x00000000}
            }
        },
        {
            "D3DFMT_X8L8V8U8", content_x8l8v8u8, sizeof(DWORD), TRUE,  1, 0,
            {
                sizeof(DDPIXELFORMAT), DDPF_BUMPDUDV | DDPF_BUMPLUMINANCE, 0,
                {32}, {0x000000ff}, {0x0000ff00}, {0x00ff0000}, {0x00000000}
            }
        },
        {
            "D3DFMT_L6V5U5",   content_l6v5u5,   sizeof(WORD),  TRUE,  4, 7,
            {
                sizeof(DDPIXELFORMAT), DDPF_BUMPDUDV | DDPF_BUMPLUMINANCE, 0,
                {16}, {0x0000001f}, {0x000003e0}, {0x0000fc00}, {0x00000000}
            }

        },
        /* No V16U16 or Q8W8V8U8 support in ddraw. */
    };
    static const D3DCOLOR expected_colors[4][4] =
    {
        {0x00808080, 0x00fefeff, 0x00010780, 0x008080ff},
        {0x00018080, 0x00800180, 0x0080fe80, 0x00fe8080},
        {0x00ba98a0, 0x004767a8, 0x00888881, 0x007878ff},
        {0x00c3c3c0, 0x003f3f80, 0x00e51fe1, 0x005fa2c8},
    };
    unsigned int i, width, x, y;
    D3DDEVICEDESC device_desc, hel_desc;

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    memset(&device_desc, 0, sizeof(device_desc));
    device_desc.dwSize = sizeof(device_desc);
    memset(&hel_desc, 0, sizeof(hel_desc));
    hel_desc.dwSize = sizeof(hel_desc);
    hr = IDirect3DDevice3_GetCaps(device, &device_desc, &hel_desc);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (!(device_desc.dwTextureOpCaps & D3DTEXOPCAPS_BLENDFACTORALPHA))
    {
        skip("D3DTOP_BLENDFACTORALPHA not supported, skipping bumpmap format tests.\n");
        goto done;
    }

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get d3d interface, hr %#x.\n", hr);
    hr = IDirect3D3_QueryInterface(d3d, &IID_IDirectDraw4, (void **)&ddraw);
    ok(SUCCEEDED(hr), "Failed to get ddraw interface, hr %#x.\n", hr);
    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface4_GetSurfaceDesc(rt, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    viewport = create_viewport(device, 0, 0, surface_desc.dwWidth, surface_desc.dwHeight);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to activate the viewport, hr %#x.\n", hr);
    U1(clear_rect).x1 = 0;
    U2(clear_rect).y1 = 0;
    U3(clear_rect).x2 = surface_desc.dwWidth;
    U4(clear_rect).y2 = surface_desc.dwHeight;

    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_ZENABLE, D3DZB_FALSE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);

    /* dst = tex * 0.5 + 1.0 * (1.0 - 0.5) = tex * 0.5 + 0.5 */
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_TEXTUREFACTOR, 0x80ffffff);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_BLENDFACTORALPHA);
    ok(SUCCEEDED(hr), "Failed to set texture stage state, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(SUCCEEDED(hr), "Failed to set texture stage state, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTextureStageState(device, 0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
    ok(SUCCEEDED(hr), "Failed to set texture stage state, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(formats); i++)
    {
        for (width = 1; width < 5; width += 3)
        {
            hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0x00000000, 0.0f, 0);
            ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);

            memset(&surface_desc, 0, sizeof(surface_desc));
            surface_desc.dwSize = sizeof(surface_desc);
            surface_desc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS;
            surface_desc.dwWidth = width;
            surface_desc.dwHeight = 4;
            U4(surface_desc).ddpfPixelFormat = formats[i].format;
            surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY;
            hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
            if (FAILED(hr))
            {
                skip("%s textures not supported, skipping.\n", formats[i].name);
                continue;
            }
            ok(SUCCEEDED(hr), "Failed to create surface, hr %#x, format %s.\n", hr, formats[i].name);

            hr = IDirectDrawSurface4_QueryInterface(surface, &IID_IDirect3DTexture2, (void **)&texture);
            ok(SUCCEEDED(hr), "Failed to get Direct3DTexture2 interface, hr %#x, format %s.\n",
                    hr, formats[i].name);
            hr = IDirect3DDevice3_SetTexture(device, 0, texture);
            ok(SUCCEEDED(hr), "Failed to set texture, hr %#x, format %s.\n", hr, formats[i].name);
            IDirect3DTexture2_Release(texture);

            memset(&surface_desc, 0, sizeof(surface_desc));
            surface_desc.dwSize = sizeof(surface_desc);
            hr = IDirectDrawSurface4_Lock(surface, NULL, &surface_desc, 0, NULL);
            ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x, format %s.\n", hr, formats[i].name);
            for (y = 0; y < 4; y++)
            {
                memcpy((char *)surface_desc.lpSurface + y * U1(surface_desc).lPitch,
                        (char *)formats[i].content + y * 4 * formats[i].pixel_size,
                        width * formats[i].pixel_size);
            }
            hr = IDirectDrawSurface4_Unlock(surface, NULL);
            ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x, format %s.\n", hr, formats[i].name);

            hr = IDirect3DDevice3_BeginScene(device);
            ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
            hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP,
                    D3DFVF_XYZ | D3DFVF_TEX1, quad, 4, 0);
            ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
            hr = IDirect3DDevice3_EndScene(device);
            ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

            for (y = 0; y < 4; y++)
            {
                for (x = 0; x < width; x++)
                {
                    expected_color = expected_colors[y][x];
                    if (!formats[i].blue)
                        expected_color |= 0x000000ff;

                    color = get_surface_color(rt, 80 + 160 * x, 60 + 120 * y);
                    ok(compare_color(color, expected_color, formats[i].slop)
                            || broken(compare_color(color, expected_color, formats[i].slop_broken)),
                            "Expected color 0x%08x, got 0x%08x, format %s, location %ux%u.\n",
                            expected_color, color, formats[i].name, x, y);
                }
            }

            IDirectDrawSurface4_Release(surface);
        }
    }

    destroy_viewport(device, viewport);
    IDirectDrawSurface4_Release(rt);
    IDirectDraw4_Release(ddraw);
    IDirect3D3_Release(d3d);

done:
    refcount = IDirect3DDevice3_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_color_fill(void)
{
    HRESULT hr;
    IDirect3DDevice3 *device;
    IDirect3D3 *d3d;
    IDirectDraw4 *ddraw;
    IDirectDrawSurface4 *surface, *surface2;
    DDSURFACEDESC2 surface_desc;
    DDPIXELFORMAT z_fmt;
    ULONG refcount;
    HWND window;
    unsigned int i;
    DDBLTFX fx;
    RECT rect = {5, 5, 7, 7};
    DWORD *color;
    DWORD supported_fmts = 0, num_fourcc_codes, *fourcc_codes;
    DDCAPS hal_caps;
    static const struct
    {
        DWORD caps, caps2;
        HRESULT colorfill_hr, depthfill_hr;
        BOOL rop_success;
        const char *name;
        DWORD result;
        BOOL check_result;
        DDPIXELFORMAT format;
    }
    tests[] =
    {
        {
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY, 0,
            DD_OK, DDERR_INVALIDPARAMS, TRUE, "vidmem offscreenplain RGB", 0xdeadbeef, TRUE,
            {
                sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_ALPHAPIXELS, 0,
                {32}, {0x00ff0000}, {0x0000ff00}, {0x000000ff}, {0xff000000}
            }
        },
        {
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY, 0,
            DD_OK, DDERR_INVALIDPARAMS, TRUE, "sysmem offscreenplain RGB", 0xdeadbeef, TRUE,
            {
                sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_ALPHAPIXELS, 0,
                {32}, {0x00ff0000}, {0x0000ff00}, {0x000000ff}, {0xff000000}
            }
        },
        {
            DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY, 0,
            DD_OK, DDERR_INVALIDPARAMS, TRUE, "vidmem texture RGB", 0xdeadbeef, TRUE,
            {
                sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_ALPHAPIXELS, 0,
                {32}, {0x00ff0000}, {0x0000ff00}, {0x000000ff}, {0xff000000}
            }
        },
        {
            DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY, 0,
            DD_OK, DDERR_INVALIDPARAMS, TRUE, "sysmem texture RGB", 0xdeadbeef, TRUE,
            {
                sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_ALPHAPIXELS, 0,
                {32}, {0x00ff0000}, {0x0000ff00}, {0x000000ff}, {0xff000000}
            }
        },
        {
            DDSCAPS_TEXTURE, DDSCAPS2_TEXTUREMANAGE,
            DD_OK, DDERR_INVALIDPARAMS, TRUE, "managed texture RGB", 0xdeadbeef, TRUE,
            {
                sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_ALPHAPIXELS, 0,
                {32}, {0x00ff0000}, {0x0000ff00}, {0x000000ff}, {0xff000000}
            }
        },
        {
            DDSCAPS_ZBUFFER | DDSCAPS_VIDEOMEMORY, 0,
            DDERR_INVALIDPARAMS, DD_OK, TRUE, "vidmem zbuffer", 0xdeadbeef, TRUE,
            {0, 0, 0, {0}, {0}, {0}, {0}, {0}}
        },
        {
            DDSCAPS_ZBUFFER | DDSCAPS_SYSTEMMEMORY, 0,
            DDERR_INVALIDPARAMS, DD_OK, TRUE, "sysmem zbuffer", 0xdeadbeef, TRUE,
            {0, 0, 0, {0}, {0}, {0}, {0}, {0}}
        },
        {
            /* Colorfill on YUV surfaces always returns DD_OK, but the content is
             * different afterwards. DX9+ GPUs set one of the two luminance values
             * in each block, but AMD and Nvidia GPUs disagree on which luminance
             * value they set. r200 (dx8) just sets the entire block to the clear
             * value. */
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY, 0,
            DD_OK, DDERR_INVALIDPARAMS, FALSE, "vidmem offscreenplain YUY2", 0, FALSE,
            {
                sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('Y', 'U', 'Y', '2'),
                {0}, {0}, {0}, {0}, {0}
            }
        },
        {
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY, 0,
            DD_OK, DDERR_INVALIDPARAMS, FALSE, "vidmem offscreenplain UYVY", 0, FALSE,
            {
                sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('U', 'Y', 'V', 'Y'),
                {0}, {0}, {0}, {0}, {0}
            }
        },
        {
            DDSCAPS_OVERLAY | DDSCAPS_VIDEOMEMORY, 0,
            DD_OK, DDERR_INVALIDPARAMS, FALSE, "vidmem overlay YUY2", 0, FALSE,
            {
                sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('Y', 'U', 'Y', '2'),
                {0}, {0}, {0}, {0}, {0}
            }
        },
        {
            DDSCAPS_OVERLAY | DDSCAPS_VIDEOMEMORY, 0,
            DD_OK, DDERR_INVALIDPARAMS, FALSE, "vidmem overlay UYVY", 0, FALSE,
            {
                sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('U', 'Y', 'V', 'Y'),
                {0}, {0}, {0}, {0}, {0}
            }
        },
        {
            DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY, 0,
            E_NOTIMPL, DDERR_INVALIDPARAMS, FALSE, "vidmem texture DXT1", 0, FALSE,
            {
                sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('D', 'X', 'T', '1'),
                {0}, {0}, {0}, {0}, {0}
            }
        },
        {
            DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY, 0,
            E_NOTIMPL, DDERR_INVALIDPARAMS, FALSE, "sysmem texture DXT1", 0, FALSE,
            {
                sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('D', 'X', 'T', '1'),
                {0}, {0}, {0}, {0}, {0}
            }
        },
        {
            /* The testbot fills this with 0x00 instead of the blue channel. The sysmem
             * surface works, presumably because it is handled by the runtime instead of
             * the driver. */
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY, 0,
            DD_OK, DDERR_INVALIDPARAMS, TRUE, "vidmem offscreenplain P8", 0xefefefef, FALSE,
            {
                sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_PALETTEINDEXED8, 0,
                {8}, {0}, {0}, {0}, {0}
            }
        },
        {
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY, 0,
            DD_OK, DDERR_INVALIDPARAMS, TRUE, "sysmem offscreenplain P8", 0xefefefef, TRUE,
            {
                sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_PALETTEINDEXED8, 0,
                {8}, {0}, {0}, {0}, {0}
            }
        },
    };
    static const struct
    {
        DWORD rop;
        const char *name;
        HRESULT hr;
    }
    rops[] =
    {
        {SRCCOPY,       "SRCCOPY",      DD_OK},
        {SRCPAINT,      "SRCPAINT",     DDERR_NORASTEROPHW},
        {SRCAND,        "SRCAND",       DDERR_NORASTEROPHW},
        {SRCINVERT,     "SRCINVERT",    DDERR_NORASTEROPHW},
        {SRCERASE,      "SRCERASE",     DDERR_NORASTEROPHW},
        {NOTSRCCOPY,    "NOTSRCCOPY",   DDERR_NORASTEROPHW},
        {NOTSRCERASE,   "NOTSRCERASE",  DDERR_NORASTEROPHW},
        {MERGECOPY,     "MERGECOPY",    DDERR_NORASTEROPHW},
        {MERGEPAINT,    "MERGEPAINT",   DDERR_NORASTEROPHW},
        {PATCOPY,       "PATCOPY",      DDERR_NORASTEROPHW},
        {PATPAINT,      "PATPAINT",     DDERR_NORASTEROPHW},
        {PATINVERT,     "PATINVERT",    DDERR_NORASTEROPHW},
        {DSTINVERT,     "DSTINVERT",    DDERR_NORASTEROPHW},
        {BLACKNESS,     "BLACKNESS",    DD_OK},
        {WHITENESS,     "WHITENESS",    DD_OK},
        {0xaa0029,      "0xaa0029",     DDERR_NORASTEROPHW} /* noop */
    };

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get d3d interface, hr %#x.\n", hr);
    hr = IDirect3D3_QueryInterface(d3d, &IID_IDirectDraw4, (void **)&ddraw);
    ok(SUCCEEDED(hr), "Failed to get ddraw interface, hr %#x.\n", hr);

    memset(&z_fmt, 0, sizeof(z_fmt));
    IDirect3D3_EnumZBufferFormats(d3d, &IID_IDirect3DHALDevice, enum_z_fmt, &z_fmt);
    if (!z_fmt.dwSize)
        skip("No Z buffer formats supported, skipping Z buffer colorfill test.\n");

    IDirect3DDevice3_EnumTextureFormats(device, test_block_formats_creation_cb, &supported_fmts);
    if (!(supported_fmts & SUPPORT_DXT1))
        skip("DXT1 textures not supported, skipping DXT1 colorfill test.\n");

    IDirect3D3_Release(d3d);

    hr = IDirectDraw4_GetFourCCCodes(ddraw, &num_fourcc_codes, NULL);
    ok(SUCCEEDED(hr), "Failed to get fourcc codes %#x.\n", hr);
    fourcc_codes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            num_fourcc_codes * sizeof(*fourcc_codes));
    if (!fourcc_codes)
        goto done;
    hr = IDirectDraw4_GetFourCCCodes(ddraw, &num_fourcc_codes, fourcc_codes);
    ok(SUCCEEDED(hr), "Failed to get fourcc codes %#x.\n", hr);
    for (i = 0; i < num_fourcc_codes; i++)
    {
        if (fourcc_codes[i] == MAKEFOURCC('Y', 'U', 'Y', '2'))
            supported_fmts |= SUPPORT_YUY2;
        else if (fourcc_codes[i] == MAKEFOURCC('U', 'Y', 'V', 'Y'))
            supported_fmts |= SUPPORT_UYVY;
    }
    HeapFree(GetProcessHeap(), 0, fourcc_codes);

    memset(&hal_caps, 0, sizeof(hal_caps));
    hal_caps.dwSize = sizeof(hal_caps);
    hr = IDirectDraw4_GetCaps(ddraw, &hal_caps, NULL);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);

    if (!(supported_fmts & (SUPPORT_YUY2 | SUPPORT_UYVY)) || !(hal_caps.dwCaps & DDCAPS_OVERLAY))
        skip("Overlays or some YUV formats not supported, skipping YUV colorfill tests.\n");

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        DWORD expected_broken = tests[i].result;

        /* Some Windows drivers modify dwFillColor when it is used on P8 or FourCC formats. */
        memset(&fx, 0, sizeof(fx));
        fx.dwSize = sizeof(fx);
        U5(fx).dwFillColor = 0xdeadbeef;

        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
        surface_desc.dwWidth = 64;
        surface_desc.dwHeight = 64;
        U4(surface_desc).ddpfPixelFormat = tests[i].format;
        surface_desc.ddsCaps.dwCaps = tests[i].caps;
        surface_desc.ddsCaps.dwCaps2 = tests[i].caps2;

        if (tests[i].format.dwFourCC == MAKEFOURCC('D','X','T','1') && !(supported_fmts & SUPPORT_DXT1))
            continue;
        if (tests[i].format.dwFourCC == MAKEFOURCC('Y','U','Y','2') && !(supported_fmts & SUPPORT_YUY2))
            continue;
        if (tests[i].format.dwFourCC == MAKEFOURCC('U','Y','V','Y') && !(supported_fmts & SUPPORT_UYVY))
            continue;
        if (tests[i].caps & DDSCAPS_OVERLAY && !(hal_caps.dwCaps & DDCAPS_OVERLAY))
            continue;

        if (tests[i].caps & DDSCAPS_ZBUFFER)
        {
            if (!z_fmt.dwSize)
                continue;

            U4(surface_desc).ddpfPixelFormat = z_fmt;
            /* Some drivers seem to convert depth values incorrectly or not at
             * all. Affects at least AMD PALM, 8.17.10.1247. */
            if (tests[i].caps & DDSCAPS_VIDEOMEMORY)
            {
                DWORD expected;
                float f, g;

                expected = tests[i].result & U3(z_fmt).dwZBitMask;
                f = ceilf(log2f(expected + 1.0f));
                g = (f + 1.0f) / 2.0f;
                g -= (int)g;
                expected_broken = (expected / exp2f(f) - g) * 256;
                expected_broken *= 0x01010101;
            }
        }

        hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
        ok(SUCCEEDED(hr), "Failed to create surface, hr %#x, surface %s.\n", hr, tests[i].name);

        hr = IDirectDrawSurface4_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
        todo_wine_if (tests[i].format.dwFourCC)
            ok(hr == tests[i].colorfill_hr, "Blt returned %#x, expected %#x, surface %s.\n",
                    hr, tests[i].colorfill_hr, tests[i].name);

        hr = IDirectDrawSurface4_Blt(surface, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
        todo_wine_if (tests[i].format.dwFourCC)
            ok(hr == tests[i].colorfill_hr, "Blt returned %#x, expected %#x, surface %s.\n",
                    hr, tests[i].colorfill_hr, tests[i].name);

        if (SUCCEEDED(hr) && tests[i].check_result)
        {
            memset(&surface_desc, 0, sizeof(surface_desc));
            surface_desc.dwSize = sizeof(surface_desc);
            hr = IDirectDrawSurface4_Lock(surface, NULL, &surface_desc, DDLOCK_READONLY, 0);
            ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x, surface %s.\n", hr, tests[i].name);
            color = surface_desc.lpSurface;
            ok(*color == tests[i].result, "Got clear result 0x%08x, expected 0x%08x, surface %s.\n",
                    *color, tests[i].result, tests[i].name);
            hr = IDirectDrawSurface4_Unlock(surface, NULL);
            ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x, surface %s.\n", hr, tests[i].name);
        }

        hr = IDirectDrawSurface4_Blt(surface, NULL, NULL, NULL, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
        ok(hr == tests[i].depthfill_hr, "Blt returned %#x, expected %#x, surface %s.\n",
                hr, tests[i].depthfill_hr, tests[i].name);
        hr = IDirectDrawSurface4_Blt(surface, &rect, NULL, NULL, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
        ok(hr == tests[i].depthfill_hr, "Blt returned %#x, expected %#x, surface %s.\n",
                hr, tests[i].depthfill_hr, tests[i].name);

        if (SUCCEEDED(hr) && tests[i].check_result)
        {
            memset(&surface_desc, 0, sizeof(surface_desc));
            surface_desc.dwSize = sizeof(surface_desc);
            hr = IDirectDrawSurface4_Lock(surface, NULL, &surface_desc, DDLOCK_READONLY, 0);
            ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x, surface %s.\n", hr, tests[i].name);
            color = surface_desc.lpSurface;
            ok((*color & U3(z_fmt).dwZBitMask) == (tests[i].result & U3(z_fmt).dwZBitMask)
                    || broken((*color & U3(z_fmt).dwZBitMask) == (expected_broken & U3(z_fmt).dwZBitMask)),
                    "Got clear result 0x%08x, expected 0x%08x, surface %s.\n",
                    *color & U3(z_fmt).dwZBitMask, tests[i].result & U3(z_fmt).dwZBitMask, tests[i].name);
            hr = IDirectDrawSurface4_Unlock(surface, NULL);
            ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x, surface %s.\n", hr, tests[i].name);
        }

        U5(fx).dwFillColor = 0xdeadbeef;
        fx.dwROP = BLACKNESS;
        hr = IDirectDrawSurface4_Blt(surface, NULL, NULL, NULL, DDBLT_ROP | DDBLT_WAIT, &fx);
        ok(FAILED(hr) == !tests[i].rop_success, "Blt returned %#x, expected %s, surface %s.\n",
                hr, tests[i].rop_success ? "success" : "failure", tests[i].name);
        ok(U5(fx).dwFillColor == 0xdeadbeef, "dwFillColor was set to 0x%08x, surface %s\n",
                U5(fx).dwFillColor, tests[i].name);

        if (SUCCEEDED(hr) && tests[i].check_result)
        {
            memset(&surface_desc, 0, sizeof(surface_desc));
            surface_desc.dwSize = sizeof(surface_desc);
            hr = IDirectDrawSurface4_Lock(surface, NULL, &surface_desc, DDLOCK_READONLY, 0);
            ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x, surface %s.\n", hr, tests[i].name);
            color = surface_desc.lpSurface;
            ok(*color == 0, "Got clear result 0x%08x, expected 0x00000000, surface %s.\n",
                    *color, tests[i].name);
            hr = IDirectDrawSurface4_Unlock(surface, NULL);
            ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x, surface %s.\n", hr, tests[i].name);
        }

        fx.dwROP = WHITENESS;
        hr = IDirectDrawSurface4_Blt(surface, NULL, NULL, NULL, DDBLT_ROP | DDBLT_WAIT, &fx);
        ok(FAILED(hr) == !tests[i].rop_success, "Blt returned %#x, expected %s, surface %s.\n",
                hr, tests[i].rop_success ? "success" : "failure", tests[i].name);
        ok(U5(fx).dwFillColor == 0xdeadbeef, "dwFillColor was set to 0x%08x, surface %s\n",
                U5(fx).dwFillColor, tests[i].name);

        if (SUCCEEDED(hr) && tests[i].check_result)
        {
            memset(&surface_desc, 0, sizeof(surface_desc));
            surface_desc.dwSize = sizeof(surface_desc);
            hr = IDirectDrawSurface4_Lock(surface, NULL, &surface_desc, DDLOCK_READONLY, 0);
            ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x, surface %s.\n", hr, tests[i].name);
            color = surface_desc.lpSurface;
            /* WHITENESS sets the alpha channel to 0x00. Ignore this for now. */
            ok((*color & 0x00ffffff) == 0x00ffffff, "Got clear result 0x%08x, expected 0xffffffff, surface %s.\n",
                    *color, tests[i].name);
            hr = IDirectDrawSurface4_Unlock(surface, NULL);
            ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x, surface %s.\n", hr, tests[i].name);
        }

        IDirectDrawSurface4_Release(surface);
    }

    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    U5(fx).dwFillColor = 0xdeadbeef;
    fx.dwROP = WHITENESS;

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    surface_desc.dwWidth = 64;
    surface_desc.dwHeight = 64;
    U4(surface_desc).ddpfPixelFormat.dwSize = sizeof(U4(surface_desc).ddpfPixelFormat);
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount = 32;
    U2(U4(surface_desc).ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(U4(surface_desc).ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(U4(surface_desc).ddpfPixelFormat).dwBBitMask = 0x000000ff;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface2, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    /* No DDBLTFX. */
    hr = IDirectDrawSurface4_Blt(surface, NULL, NULL, &rect, DDBLT_COLORFILL | DDBLT_WAIT, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Blt(surface, NULL, NULL, &rect, DDBLT_ROP | DDBLT_WAIT, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    /* Unused source rectangle. */
    hr = IDirectDrawSurface4_Blt(surface, NULL, NULL, &rect, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Blt(surface, NULL, NULL, &rect, DDBLT_ROP | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);

    /* Unused source surface. */
    hr = IDirectDrawSurface4_Blt(surface, NULL, surface2, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Blt(surface, NULL, surface2, NULL, DDBLT_ROP | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Blt(surface, NULL, surface2, &rect, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Blt(surface, NULL, surface2, &rect, DDBLT_ROP | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);

    /* Inverted destination or source rectangle. */
    SetRect(&rect, 5, 7, 7, 5);
    hr = IDirectDrawSurface4_Blt(surface, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDRECT, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Blt(surface, NULL, NULL, &rect, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Blt(surface, &rect, surface2, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Blt(surface, NULL, surface2, &rect, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Blt(surface, NULL, surface2, &rect, DDBLT_ROP | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDRECT, "Got unexpected hr %#x.\n", hr);

    /* Negative rectangle. */
    SetRect(&rect, -1, -1, 5, 5);
    hr = IDirectDrawSurface4_Blt(surface, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDRECT, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Blt(surface, NULL, NULL, &rect, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Blt(surface, &rect, surface2, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Blt(surface, &rect, surface2, &rect, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Blt(surface, NULL, surface2, &rect, DDBLT_ROP | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDRECT, "Got unexpected hr %#x.\n", hr);

    /* Out of bounds rectangle. */
    SetRect(&rect, 0, 0, 65, 65);
    hr = IDirectDrawSurface4_Blt(surface, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDRECT, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Blt(surface, NULL, surface2, &rect, DDBLT_ROP | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDRECT, "Got unexpected hr %#x.\n", hr);

    /* Combine multiple flags. */
    hr = IDirectDrawSurface4_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_ROP | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Blt(surface, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_ROP | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(rops); i++)
    {
        fx.dwROP = rops[i].rop;
        hr = IDirectDrawSurface4_Blt(surface, NULL, surface2, NULL, DDBLT_ROP | DDBLT_WAIT, &fx);
        ok(hr == rops[i].hr, "Got unexpected hr %#x for rop %s.\n", hr, rops[i].name);
    }

    IDirectDrawSurface4_Release(surface2);
    IDirectDrawSurface4_Release(surface);

    if (!z_fmt.dwSize)
        goto done;

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    surface_desc.dwWidth = 64;
    surface_desc.dwHeight = 64;
    U4(surface_desc).ddpfPixelFormat = z_fmt;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface2, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    /* No DDBLTFX. */
    hr = IDirectDrawSurface4_Blt(surface, NULL, NULL, &rect, DDBLT_DEPTHFILL | DDBLT_WAIT, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    /* Unused source rectangle. */
    hr = IDirectDrawSurface4_Blt(surface, NULL, NULL, &rect, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);

    /* Unused source surface. */
    hr = IDirectDrawSurface4_Blt(surface, NULL, surface2, NULL, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Blt(surface, NULL, surface2, &rect, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    /* Inverted destination or source rectangle. */
    SetRect(&rect, 5, 7, 7, 5);
    hr = IDirectDrawSurface4_Blt(surface, &rect, NULL, NULL, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDRECT, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Blt(surface, NULL, NULL, &rect, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Blt(surface, &rect, surface2, NULL, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Blt(surface, NULL, surface2, &rect, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    /* Negative rectangle. */
    SetRect(&rect, -1, -1, 5, 5);
    hr = IDirectDrawSurface4_Blt(surface, &rect, NULL, NULL, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDRECT, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Blt(surface, NULL, NULL, &rect, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Blt(surface, &rect, surface2, NULL, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Blt(surface, &rect, surface2, &rect, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    /* Out of bounds rectangle. */
    SetRect(&rect, 0, 0, 65, 65);
    hr = IDirectDrawSurface4_Blt(surface, &rect, NULL, NULL, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDRECT, "Got unexpected hr %#x.\n", hr);

    /* Combine multiple flags. */
    hr = IDirectDrawSurface4_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    IDirectDrawSurface4_Release(surface2);
    IDirectDrawSurface4_Release(surface);

done:
    IDirectDraw4_Release(ddraw);
    refcount = IDirect3DDevice3_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_texcoordindex(void)
{
    static struct
    {
        struct vec3 pos;
        struct vec2 texcoord1;
        struct vec2 texcoord2;
        struct vec2 texcoord3;
    }
    quad[] =
    {
        {{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f}},
        {{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 1.0f, -1.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}},
        {{ 1.0f,  1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f}},
    };
    static const DWORD fvf = D3DFVF_XYZ | D3DFVF_TEX3;
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    IDirect3DDevice3 *device;
    IDirect3D3 *d3d;
    IDirectDraw4 *ddraw;
    IDirectDrawSurface4 *rt;
    IDirect3DViewport3 *viewport;
    HWND window;
    HRESULT hr;
    IDirectDrawSurface4 *surface1, *surface2;
    IDirect3DTexture2 *texture1, *texture2;
    DDSURFACEDESC2 surface_desc;
    ULONG refcount;
    D3DCOLOR color;
    DWORD *ptr;

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get Direct3D3 interface, hr %#x.\n", hr);
    hr = IDirect3D3_QueryInterface(d3d, &IID_IDirectDraw4, (void **)&ddraw);
    ok(SUCCEEDED(hr), "Failed to get DirectDraw4 interface, hr %#x.\n", hr);
    IDirect3D3_Release(d3d);

    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    surface_desc.dwWidth = 2;
    surface_desc.dwHeight = 2;
    U4(surface_desc).ddpfPixelFormat.dwSize = sizeof(U4(surface_desc).ddpfPixelFormat);
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount = 32;
    U2(U4(surface_desc).ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(U4(surface_desc).ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(U4(surface_desc).ddpfPixelFormat).dwBBitMask = 0x000000ff;
    U5(U4(surface_desc).ddpfPixelFormat).dwRGBAlphaBitMask = 0xff000000;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface1, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface2, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface4_Lock(surface1, 0, &surface_desc, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    ptr = surface_desc.lpSurface;
    ptr[0] = 0xff000000;
    ptr[1] = 0xff00ff00;
    ptr += U1(surface_desc).lPitch / sizeof(*ptr);
    ptr[0] = 0xff0000ff;
    ptr[1] = 0xff00ffff;
    hr = IDirectDrawSurface4_Unlock(surface1, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface4_Lock(surface2, 0, &surface_desc, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    ptr = surface_desc.lpSurface;
    ptr[0] = 0xff000000;
    ptr[1] = 0xff0000ff;
    ptr += U1(surface_desc).lPitch / sizeof(*ptr);
    ptr[0] = 0xffff0000;
    ptr[1] = 0xffff00ff;
    hr = IDirectDrawSurface4_Unlock(surface2, 0);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    viewport = create_viewport(device, 0, 0, 640, 480);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_QueryInterface(surface1, &IID_IDirect3DTexture2, (void **)&texture1);
    ok(SUCCEEDED(hr), "Failed to get texture interface, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_QueryInterface(surface2, &IID_IDirect3DTexture2, (void **)&texture2);
    ok(SUCCEEDED(hr), "Failed to get texture interface, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTexture(device, 0, texture1);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTexture(device, 1, texture2);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(SUCCEEDED(hr), "Failed to set color op, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(SUCCEEDED(hr), "Failed to set color arg, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTextureStageState(device, 1, D3DTSS_COLOROP, D3DTOP_ADD);
    ok(SUCCEEDED(hr), "Failed to set color op, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTextureStageState(device, 1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(SUCCEEDED(hr), "Failed to set color arg, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTextureStageState(device, 1, D3DTSS_COLORARG2, D3DTA_CURRENT);
    ok(SUCCEEDED(hr), "Failed to set color arg, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTextureStageState(device, 2, D3DTSS_COLOROP, D3DTOP_DISABLE);
    ok(SUCCEEDED(hr), "Failed to set color op, hr %#x.\n", hr);

    hr = IDirect3DDevice3_SetTextureStageState(device, 0, D3DTSS_TEXCOORDINDEX, 1);
    ok(SUCCEEDED(hr), "Failed to set texcoord index, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTextureStageState(device, 1, D3DTSS_TEXCOORDINDEX, 0);
    ok(SUCCEEDED(hr), "Failed to set texcoord index, hr %#x.\n", hr);

    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_ZENABLE, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable z-buffering, hr %#x.\n", hr);

    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0xffffff00, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, fvf, quad, 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = get_surface_color(rt, 160, 120);
    ok(compare_color(color, 0x000000ff, 2), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 480, 120);
    ok(compare_color(color, 0x0000ffff, 2), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 160, 360);
    ok(compare_color(color, 0x00ff0000, 2), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 480, 360);
    ok(compare_color(color, 0x00ffffff, 2), "Got unexpected color 0x%08x.\n", color);

    /* D3DTSS_TEXTURETRANSFORMFLAGS was introduced in D3D7, can't test it here. */

    hr = IDirect3DDevice3_SetTextureStageState(device, 1, D3DTSS_TEXCOORDINDEX, 2);
    ok(SUCCEEDED(hr), "Failed to set texcoord index, hr %#x.\n", hr);

    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0xffffff00, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, fvf, quad, 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = get_surface_color(rt, 160, 120);
    ok(compare_color(color, 0x000000ff, 2), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 480, 120);
    ok(compare_color(color, 0x0000ffff, 2), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 160, 360);
    ok(compare_color(color, 0x00ff00ff, 2), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 480, 360);
    ok(compare_color(color, 0x00ffff00, 2), "Got unexpected color 0x%08x.\n", color);

    IDirect3DTexture2_Release(texture2);
    IDirect3DTexture2_Release(texture1);
    IDirectDrawSurface4_Release(surface2);
    IDirectDrawSurface4_Release(surface1);

    destroy_viewport(device, viewport);

    IDirectDrawSurface4_Release(rt);
    IDirectDraw_Release(ddraw);
    refcount = IDirect3DDevice3_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_colorkey_precision(void)
{
    static struct
    {
        struct vec3 pos;
        struct vec2 texcoord;
    }
    quad[] =
    {
        {{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
        {{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
        {{ 1.0f,  1.0f, 0.0f}, {1.0f, 0.0f}},
    };
    IDirect3DDevice3 *device;
    IDirect3D3 *d3d;
    IDirectDraw4 *ddraw;
    IDirectDrawSurface4 *rt;
    IDirect3DViewport3 *viewport;
    HWND window;
    HRESULT hr;
    IDirectDrawSurface4 *src, *dst, *texture;
    IDirect3DTexture2 *d3d_texture;
    DDSURFACEDESC2 surface_desc, lock_desc;
    ULONG refcount;
    D3DCOLOR color;
    unsigned int t, c;
    DDCOLORKEY ckey;
    DDBLTFX fx;
    DWORD data[4] = {0}, color_mask;
    D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    BOOL is_nvidia, is_warp;
    static const struct
    {
        unsigned int max, shift, bpp, clear;
        const char *name;
        BOOL skip_nv;
        DDPIXELFORMAT fmt;
    }
    tests[] =
    {
        {
            255, 0, 4, 0x00345678, "D3DFMT_X8R8G8B8", FALSE,
            {
                sizeof(DDPIXELFORMAT), DDPF_RGB, 0,
                {32}, {0x00ff0000}, {0x0000ff00}, {0x000000ff}, {0x00000000}
            }

        },
        {
            63, 5, 2, 0x5678, "D3DFMT_R5G6B5, G channel", FALSE,
            {
                sizeof(DDPIXELFORMAT), DDPF_RGB, 0,
                {16}, {0xf800}, {0x07e0}, {0x001f}, {0x0000}
            }

        },
        {
            31, 0, 2, 0x5678, "D3DFMT_R5G6B5, B channel", FALSE,
            {
                sizeof(DDPIXELFORMAT), DDPF_RGB, 0,
                {16}, {0xf800}, {0x07e0}, {0x001f}, {0x0000}
            }

        },
        {
            15, 0, 2, 0x0678, "D3DFMT_A4R4G4B4", TRUE,
            {
                sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_ALPHAPIXELS, 0,
                {16}, {0x0f00}, {0x00f0}, {0x000f}, {0xf000}
            }
        },
    };

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get Direct3D3 interface, hr %#x.\n", hr);
    hr = IDirect3D3_QueryInterface(d3d, &IID_IDirectDraw4, (void **)&ddraw);
    ok(SUCCEEDED(hr), "Failed to get DirectDraw4 interface, hr %#x.\n", hr);
    IDirect3D3_Release(d3d);
    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    is_nvidia = ddraw_is_nvidia(ddraw);
    /* The Windows 8 WARP driver has plenty of false negatives in X8R8G8B8
     * (color key doesn't match although the values are equal), and a false
     * positive when the color key is 0 and the texture contains the value 1.
     * I don't want to mark this broken unconditionally since this would
     * essentially disable the test on Windows. Also on random occasions
     * 254 == 255 and 255 != 255.*/
    is_warp = ddraw_is_warp(ddraw);

    viewport = create_viewport(device, 0, 0, 640, 480);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);

    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable lighting, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_ZENABLE, D3DZB_FALSE);
    ok(SUCCEEDED(hr), "Failed to disable z-buffering, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_COLORKEYENABLE, TRUE);
    ok(SUCCEEDED(hr), "Failed to enable color keying, hr %#x.\n", hr);
    /* Multiply the texture read result with 0, that way the result color if the key doesn't
     * match is constant. In theory color keying works without reading the texture result
     * (meaning we could just op=arg1, arg1=tfactor), but the Geforce7 Windows driver begs
     * to differ. */
    hr = IDirect3DDevice3_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    ok(SUCCEEDED(hr), "Failed to set color op, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    ok(SUCCEEDED(hr), "Failed to set color arg, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTextureStageState(device, 0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
    ok(SUCCEEDED(hr), "Failed to set color arg, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_TEXTUREFACTOR, 0x00000000);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);

    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    memset(&lock_desc, 0, sizeof(lock_desc));
    lock_desc.dwSize = sizeof(lock_desc);

    for (t = 0; t < ARRAY_SIZE(tests); ++t)
    {
        if (is_nvidia && tests[t].skip_nv)
        {
            win_skip("Skipping test %s on Nvidia Windows drivers.\n", tests[t].name);
            continue;
        }

        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
        surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
        surface_desc.dwWidth = 4;
        surface_desc.dwHeight = 1;
        U4(surface_desc).ddpfPixelFormat = tests[t].fmt;
        /* Windows XP (at least with the r200 driver, other drivers untested) produces
         * garbage when doing color keyed texture->texture blits. */
        hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &src, NULL);
        ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
        hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &dst, NULL);
        ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

        U5(fx).dwFillColor = tests[t].clear;
        /* On the w8 testbot (WARP driver) the blit result has different values in the
         * X channel. */
        color_mask = U2(tests[t].fmt).dwRBitMask
                | U3(tests[t].fmt).dwGBitMask
                | U4(tests[t].fmt).dwBBitMask;

        for (c = 0; c <= tests[t].max; ++c)
        {
            /* The idiotic Nvidia Windows driver can't change the color key on a d3d
             * texture after it has been set once... */
            surface_desc.dwFlags |= DDSD_CKSRCBLT;
            surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
            surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue = c << tests[t].shift;
            surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue = c << tests[t].shift;
            hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &texture, NULL);
            ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
            hr = IDirectDrawSurface4_QueryInterface(texture, &IID_IDirect3DTexture2, (void **)&d3d_texture);
            ok(SUCCEEDED(hr), "Failed to get texture interface, hr %#x.\n", hr);
            hr = IDirect3DDevice3_SetTexture(device, 0, d3d_texture);
            ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);

            hr = IDirectDrawSurface4_Blt(dst, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
            ok(SUCCEEDED(hr), "Failed to clear destination surface, hr %#x.\n", hr);

            hr = IDirectDrawSurface4_Lock(src, NULL, &lock_desc, DDLOCK_WAIT, NULL);
            ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
            switch (tests[t].bpp)
            {
                case 4:
                    ((DWORD *)lock_desc.lpSurface)[0] = (c ? c - 1 : 0) << tests[t].shift;
                    ((DWORD *)lock_desc.lpSurface)[1] = c << tests[t].shift;
                    ((DWORD *)lock_desc.lpSurface)[2] = min(c + 1, tests[t].max) << tests[t].shift;
                    ((DWORD *)lock_desc.lpSurface)[3] = 0xffffffff;
                    break;

                case 2:
                    ((WORD *)lock_desc.lpSurface)[0] = (c ? c - 1 : 0) << tests[t].shift;
                    ((WORD *)lock_desc.lpSurface)[1] = c << tests[t].shift;
                    ((WORD *)lock_desc.lpSurface)[2] = min(c + 1, tests[t].max) << tests[t].shift;
                    ((WORD *)lock_desc.lpSurface)[3] = 0xffff;
                    break;
            }
            hr = IDirectDrawSurface4_Unlock(src, 0);
            ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
            hr = IDirectDrawSurface4_Blt(texture, NULL, src, NULL, DDBLT_WAIT, NULL);
            ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);

            ckey.dwColorSpaceLowValue = c << tests[t].shift;
            ckey.dwColorSpaceHighValue = c << tests[t].shift;
            hr = IDirectDrawSurface4_SetColorKey(src, DDCKEY_SRCBLT, &ckey);
            ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);

            hr = IDirectDrawSurface4_Blt(dst, NULL, src, NULL, DDBLT_KEYSRC | DDBLT_WAIT, NULL);
            ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);

            /* Don't make this read only, it somehow breaks the detection of the Nvidia bug below. */
            hr = IDirectDrawSurface4_Lock(dst, NULL, &lock_desc, DDLOCK_WAIT, NULL);
            ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
            switch (tests[t].bpp)
            {
                case 4:
                    data[0] = ((DWORD *)lock_desc.lpSurface)[0] & color_mask;
                    data[1] = ((DWORD *)lock_desc.lpSurface)[1] & color_mask;
                    data[2] = ((DWORD *)lock_desc.lpSurface)[2] & color_mask;
                    data[3] = ((DWORD *)lock_desc.lpSurface)[3] & color_mask;
                    break;

                case 2:
                    data[0] = ((WORD *)lock_desc.lpSurface)[0] & color_mask;
                    data[1] = ((WORD *)lock_desc.lpSurface)[1] & color_mask;
                    data[2] = ((WORD *)lock_desc.lpSurface)[2] & color_mask;
                    data[3] = ((WORD *)lock_desc.lpSurface)[3] & color_mask;
                    break;
            }
            hr = IDirectDrawSurface4_Unlock(dst, 0);
            ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

            if (!c)
            {
                ok(data[0] == tests[t].clear, "Expected surface content %#x, got %#x, format %s, c=%u.\n",
                        tests[t].clear, data[0], tests[t].name, c);

                if (data[3] == tests[t].clear)
                {
                    /* My Geforce GTX 460 on Windows 7 misbehaves when A4R4G4B4 is blitted with color
                     * keying: The blit takes ~0.5 seconds, and subsequent color keying draws are broken,
                     * even when a different surface is used. The blit itself doesn't draw anything,
                     * so we can detect the bug by looking at the otherwise unused 4th texel. It should
                     * never be masked out by the key.
                     *
                     * On Windows 10 the problem is worse, Blt just hangs. For this reason the ARGB4444
                     * test is disabled entirely.
                     *
                     * Also appears to affect the testbot in some way with R5G6B5. Color keying is
                     * terrible on WARP. */
                    skip("Nvidia A4R4G4B4 color keying blit bug detected, skipping.\n");
                    IDirect3DTexture2_Release(d3d_texture);
                    IDirectDrawSurface4_Release(texture);
                    IDirectDrawSurface4_Release(src);
                    IDirectDrawSurface4_Release(dst);
                    goto done;
                }
            }
            else
                ok(data[0] == (c - 1) << tests[t].shift, "Expected surface content %#x, got %#x, format %s, c=%u.\n",
                        (c - 1) << tests[t].shift, data[0], tests[t].name, c);

            ok(data[1] == tests[t].clear, "Expected surface content %#x, got %#x, format %s, c=%u.\n",
                    tests[t].clear, data[1], tests[t].name, c);

            if (c == tests[t].max)
                ok(data[2] == tests[t].clear, "Expected surface content %#x, got %#x, format %s, c=%u.\n",
                        tests[t].clear, data[2], tests[t].name, c);
            else
                ok(data[2] == (c + 1) << tests[t].shift, "Expected surface content %#x, got %#x, format %s, c=%u.\n",
                        (c + 1) << tests[t].shift, data[2], tests[t].name, c);

            hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0x0000ff00, 1.0f, 0);
            ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

            hr = IDirect3DDevice3_BeginScene(device);
            ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
            hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_TEX1, quad, 4, 0);
            ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
            hr = IDirect3DDevice3_EndScene(device);
            ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

            color = get_surface_color(rt, 80, 240);
            if (!c)
                ok(compare_color(color, 0x0000ff00, 1) || broken(is_warp && compare_color(color, 0x00000000, 1)),
                        "Got unexpected color 0x%08x, format %s, c=%u.\n",
                        color, tests[t].name, c);
            else
                ok(compare_color(color, 0x00000000, 1) || broken(is_warp && compare_color(color, 0x0000ff00, 1)),
                        "Got unexpected color 0x%08x, format %s, c=%u.\n",
                        color, tests[t].name, c);

            color = get_surface_color(rt, 240, 240);
            ok(compare_color(color, 0x0000ff00, 1) || broken(is_warp && compare_color(color, 0x00000000, 1)),
                    "Got unexpected color 0x%08x, format %s, c=%u.\n",
                    color, tests[t].name, c);

            color = get_surface_color(rt, 400, 240);
            if (c == tests[t].max)
                ok(compare_color(color, 0x0000ff00, 1) || broken(is_warp && compare_color(color, 0x00000000, 1)),
                        "Got unexpected color 0x%08x, format %s, c=%u.\n",
                        color, tests[t].name, c);
            else
                ok(compare_color(color, 0x00000000, 1) || broken(is_warp && compare_color(color, 0x0000ff00, 1)),
                        "Got unexpected color 0x%08x, format %s, c=%u.\n",
                        color, tests[t].name, c);

            IDirect3DTexture2_Release(d3d_texture);
            IDirectDrawSurface4_Release(texture);
        }
        IDirectDrawSurface4_Release(src);
        IDirectDrawSurface4_Release(dst);
    }
    done:

    destroy_viewport(device, viewport);
    IDirectDrawSurface4_Release(rt);
    IDirectDraw4_Release(ddraw);
    refcount = IDirect3DDevice3_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_range_colorkey(void)
{
    IDirectDraw4 *ddraw;
    HWND window;
    HRESULT hr;
    IDirectDrawSurface4 *surface;
    DDSURFACEDESC2 surface_desc;
    ULONG refcount;
    DDCOLORKEY ckey;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CKSRCBLT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    surface_desc.dwWidth = 1;
    surface_desc.dwHeight = 1;
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount = 32;
    U2(U4(surface_desc).ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(U4(surface_desc).ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(U4(surface_desc).ddpfPixelFormat).dwBBitMask = 0x000000ff;
    U5(U4(surface_desc).ddpfPixelFormat).dwRGBAlphaBitMask = 0x00000000;

    /* Creating a surface with a range color key fails with DDERR_NOCOLORKEY. */
    surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue = 0x00000000;
    surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue = 0x00000001;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(hr == DDERR_NOCOLORKEYHW, "Got unexpected hr %#x.\n", hr);

    surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue = 0x00000001;
    surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue = 0x00000000;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(hr == DDERR_NOCOLORKEYHW, "Got unexpected hr %#x.\n", hr);

    /* Same for DDSCAPS_OFFSCREENPLAIN. */
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue = 0x00000000;
    surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue = 0x00000001;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(hr == DDERR_NOCOLORKEYHW, "Got unexpected hr %#x.\n", hr);

    surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue = 0x00000001;
    surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue = 0x00000000;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(hr == DDERR_NOCOLORKEYHW, "Got unexpected hr %#x.\n", hr);

    surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue = 0x00000000;
    surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue = 0x00000000;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    /* Setting a range color key without DDCKEY_COLORSPACE collapses the key. */
    ckey.dwColorSpaceLowValue = 0x00000000;
    ckey.dwColorSpaceHighValue = 0x00000001;
    hr = IDirectDrawSurface4_SetColorKey(surface, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_GetColorKey(surface, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to get color key, hr %#x.\n", hr);
    ok(!ckey.dwColorSpaceLowValue, "Got unexpected value 0x%08x.\n", ckey.dwColorSpaceLowValue);
    ok(!ckey.dwColorSpaceHighValue, "Got unexpected value 0x%08x.\n", ckey.dwColorSpaceHighValue);

    ckey.dwColorSpaceLowValue = 0x00000001;
    ckey.dwColorSpaceHighValue = 0x00000000;
    hr = IDirectDrawSurface4_SetColorKey(surface, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_GetColorKey(surface, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to get color key, hr %#x.\n", hr);
    ok(ckey.dwColorSpaceLowValue == 0x00000001, "Got unexpected value 0x%08x.\n", ckey.dwColorSpaceLowValue);
    ok(ckey.dwColorSpaceHighValue == 0x00000001, "Got unexpected value 0x%08x.\n", ckey.dwColorSpaceHighValue);

    /* DDCKEY_COLORSPACE is ignored if the key is a single value. */
    ckey.dwColorSpaceLowValue = 0x00000000;
    ckey.dwColorSpaceHighValue = 0x00000000;
    hr = IDirectDrawSurface4_SetColorKey(surface, DDCKEY_SRCBLT | DDCKEY_COLORSPACE, &ckey);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);

    /* Using it with a range key results in DDERR_NOCOLORKEYHW. */
    ckey.dwColorSpaceLowValue = 0x00000001;
    ckey.dwColorSpaceHighValue = 0x00000000;
    hr = IDirectDrawSurface4_SetColorKey(surface, DDCKEY_SRCBLT | DDCKEY_COLORSPACE, &ckey);
    ok(hr == DDERR_NOCOLORKEYHW, "Got unexpected hr %#x.\n", hr);
    ckey.dwColorSpaceLowValue = 0x00000000;
    ckey.dwColorSpaceHighValue = 0x00000001;
    hr = IDirectDrawSurface4_SetColorKey(surface, DDCKEY_SRCBLT | DDCKEY_COLORSPACE, &ckey);
    ok(hr == DDERR_NOCOLORKEYHW, "Got unexpected hr %#x.\n", hr);
    /* Range destination keys don't work either. */
    hr = IDirectDrawSurface4_SetColorKey(surface, DDCKEY_DESTBLT | DDCKEY_COLORSPACE, &ckey);
    ok(hr == DDERR_NOCOLORKEYHW, "Got unexpected hr %#x.\n", hr);

    /* Just to show it's not because of A, R, and G having equal values. */
    ckey.dwColorSpaceLowValue = 0x00000000;
    ckey.dwColorSpaceHighValue = 0x01010101;
    hr = IDirectDrawSurface4_SetColorKey(surface, DDCKEY_SRCBLT | DDCKEY_COLORSPACE, &ckey);
    ok(hr == DDERR_NOCOLORKEYHW, "Got unexpected hr %#x.\n", hr);

    /* None of these operations modified the key. */
    hr = IDirectDrawSurface4_GetColorKey(surface, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to get color key, hr %#x.\n", hr);
    ok(!ckey.dwColorSpaceLowValue, "Got unexpected value 0x%08x.\n", ckey.dwColorSpaceLowValue);
    ok(!ckey.dwColorSpaceHighValue, "Got unexpected value 0x%08x.\n", ckey.dwColorSpaceHighValue);

    IDirectDrawSurface4_Release(surface),
    refcount = IDirectDraw4_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_shademode(void)
{
    IDirect3DVertexBuffer *vb_strip, *vb_list, *buffer;
    IDirect3DViewport3 *viewport;
    IDirect3DDevice3 *device;
    D3DVERTEXBUFFERDESC desc;
    IDirectDrawSurface4 *rt;
    DWORD color0, color1;
    void *data = NULL;
    IDirect3D3 *d3d;
    ULONG refcount;
    UINT i, count;
    HWND window;
    HRESULT hr;
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    static const struct
    {
        struct vec3 position;
        DWORD diffuse;
    }
    quad_strip[] =
    {
        {{-1.0f, -1.0f, 0.0f}, 0xffff0000},
        {{-1.0f,  1.0f, 0.0f}, 0xff00ff00},
        {{ 1.0f, -1.0f, 0.0f}, 0xff0000ff},
        {{ 1.0f,  1.0f, 0.0f}, 0xffffffff},
    },
    quad_list[] =
    {
        {{-1.0f, -1.0f, 0.0f}, 0xffff0000},
        {{-1.0f,  1.0f, 0.0f}, 0xff00ff00},
        {{ 1.0f, -1.0f, 0.0f}, 0xff0000ff},

        {{ 1.0f, -1.0f, 0.0f}, 0xff0000ff},
        {{-1.0f,  1.0f, 0.0f}, 0xff00ff00},
        {{ 1.0f,  1.0f, 0.0f}, 0xffffffff},
    };
    static const struct
    {
        DWORD primtype;
        DWORD shademode;
        DWORD color0, color1;
    }
    tests[] =
    {
        {D3DPT_TRIANGLESTRIP, D3DSHADE_FLAT,    0x00ff0000, 0x0000ff00},
        {D3DPT_TRIANGLESTRIP, D3DSHADE_PHONG,   0x000dca28, 0x000d45c7},
        {D3DPT_TRIANGLESTRIP, D3DSHADE_GOURAUD, 0x000dca28, 0x000d45c7},
        {D3DPT_TRIANGLESTRIP, D3DSHADE_PHONG,   0x000dca28, 0x000d45c7},
        {D3DPT_TRIANGLELIST,  D3DSHADE_FLAT,    0x00ff0000, 0x000000ff},
        {D3DPT_TRIANGLELIST,  D3DSHADE_GOURAUD, 0x000dca28, 0x000d45c7},
    };

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get d3d interface, hr %#x.\n", hr);
    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    viewport = create_viewport(device, 0, 0, 640, 480);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to activate the viewport, hr %#x.\n", hr);

    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_FOGENABLE, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable fog, hr %#x.\n", hr);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwCaps = D3DVBCAPS_WRITEONLY;
    desc.dwFVF = D3DFVF_XYZ | D3DFVF_DIFFUSE;
    desc.dwNumVertices = ARRAY_SIZE(quad_strip);
    hr = IDirect3D3_CreateVertexBuffer(d3d, &desc, &vb_strip, 0, NULL);
    ok(hr == D3D_OK, "Failed to create vertex buffer, hr %#x.\n", hr);
    hr = IDirect3DVertexBuffer_Lock(vb_strip, 0, &data, NULL);
    ok(hr == D3D_OK, "Failed to lock vertex buffer, hr %#x.\n", hr);
    memcpy(data, quad_strip, sizeof(quad_strip));
    hr = IDirect3DVertexBuffer_Unlock(vb_strip);
    ok(hr == D3D_OK, "Failed to unlock vertex buffer, hr %#x.\n", hr);

    desc.dwNumVertices = ARRAY_SIZE(quad_list);
    hr = IDirect3D3_CreateVertexBuffer(d3d, &desc, &vb_list, 0, NULL);
    ok(hr == D3D_OK, "Failed to create vertex buffer, hr %#x.\n", hr);
    hr = IDirect3DVertexBuffer_Lock(vb_list, 0, &data, NULL);
    ok(hr == D3D_OK, "Failed to lock vertex buffer, hr %#x.\n", hr);
    memcpy(data, quad_list, sizeof(quad_list));
    hr = IDirect3DVertexBuffer_Unlock(vb_list);
    ok(hr == D3D_OK, "Failed to unlock vertex buffer, hr %#x.\n", hr);

    /* Try it first with a TRIANGLESTRIP.  Do it with different geometry because
     * the color fixups we have to do for FLAT shading will be dependent on that. */

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0xffffffff, 0.0f, 0);
        ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);

        hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_SHADEMODE, tests[i].shademode);
        ok(hr == D3D_OK, "Failed to set shade mode, hr %#x.\n", hr);

        hr = IDirect3DDevice3_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
        buffer = tests[i].primtype == D3DPT_TRIANGLESTRIP ? vb_strip : vb_list;
        count = tests[i].primtype == D3DPT_TRIANGLESTRIP ? 4 : 6;
        hr = IDirect3DDevice3_DrawPrimitiveVB(device, tests[i].primtype, buffer, 0, count, 0);
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
        hr = IDirect3DDevice3_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

        color0 = get_surface_color(rt, 100, 100); /* Inside first triangle */
        color1 = get_surface_color(rt, 500, 350); /* Inside second triangle */

        /* For D3DSHADE_FLAT it should take the color of the first vertex of
         * each triangle. This requires EXT_provoking_vertex or similar
         * functionality being available. */
        /* PHONG should be the same as GOURAUD, since no hardware implements
         * this. */
        ok(compare_color(color0, tests[i].color0, 1), "Test %u shading has color0 %08x, expected %08x.\n",
                i, color0, tests[i].color0);
        ok(compare_color(color1, tests[i].color1, 1), "Test %u shading has color1 %08x, expected %08x.\n",
                i, color1, tests[i].color1);
    }

    IDirect3DVertexBuffer_Release(vb_strip);
    IDirect3DVertexBuffer_Release(vb_list);
    destroy_viewport(device, viewport);
    IDirectDrawSurface4_Release(rt);
    IDirect3D3_Release(d3d);
    refcount = IDirect3DDevice3_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_lockrect_invalid(void)
{
    unsigned int i, r;
    IDirectDraw4 *ddraw;
    IDirectDrawSurface4 *surface;
    HWND window;
    HRESULT hr;
    DDSURFACEDESC2 surface_desc;
    DDCAPS hal_caps;
    DWORD needed_caps = DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY;
    static RECT valid[] =
    {
        {60, 60, 68, 68},
        {60, 60, 60, 68},
        {60, 60, 68, 60},
        {120, 60, 128, 68},
        {60, 120, 68, 128},
    };
    static RECT invalid[] =
    {
        {68, 60, 60, 68},       /* left > right */
        {60, 68, 68, 60},       /* top > bottom */
        {-8, 60,  0, 68},       /* left < surface */
        {60, -8, 68,  0},       /* top < surface */
        {-16, 60, -8, 68},      /* right < surface */
        {60, -16, 68, -8},      /* bottom < surface */
        {60, 60, 136, 68},      /* right > surface */
        {60, 60, 68, 136},      /* bottom > surface */
        {136, 60, 144, 68},     /* left > surface */
        {60, 136, 68, 144},     /* top > surface */
    };
    static const struct
    {
        DWORD caps, caps2;
        const char *name;
        HRESULT hr;
    }
    resources[] =
    {
        {DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY, 0, "sysmem offscreenplain", DDERR_INVALIDPARAMS},
        {DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY, 0, "vidmem offscreenplain", DDERR_INVALIDPARAMS},
        {DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY, 0, "sysmem texture", DDERR_INVALIDPARAMS},
        {DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY, 0, "vidmem texture", DDERR_INVALIDPARAMS},
        {DDSCAPS_TEXTURE, DDSCAPS2_TEXTUREMANAGE, "managed texture", DDERR_INVALIDPARAMS},
    };

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&hal_caps, 0, sizeof(hal_caps));
    hal_caps.dwSize = sizeof(hal_caps);
    hr = IDirectDraw4_GetCaps(ddraw, &hal_caps, NULL);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);
    if ((hal_caps.ddsCaps.dwCaps & needed_caps) != needed_caps
            || !(hal_caps.ddsCaps.dwCaps & DDSCAPS2_TEXTUREMANAGE))
    {
        skip("Required surface types not supported, skipping test.\n");
        goto done;
    }

    for (r = 0; r < ARRAY_SIZE(resources); ++r)
    {
        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
        surface_desc.ddsCaps.dwCaps = resources[r].caps;
        surface_desc.ddsCaps.dwCaps2 = resources[r].caps2;
        surface_desc.dwWidth = 128;
        surface_desc.dwHeight = 128;
        U4(surface_desc).ddpfPixelFormat.dwSize = sizeof(U4(surface_desc).ddpfPixelFormat);
        U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_RGB;
        U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount = 32;
        U2(U4(surface_desc).ddpfPixelFormat).dwRBitMask = 0xff0000;
        U3(U4(surface_desc).ddpfPixelFormat).dwGBitMask = 0x00ff00;
        U4(U4(surface_desc).ddpfPixelFormat).dwBBitMask = 0x0000ff;

        hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
        ok(SUCCEEDED(hr), "Failed to create surface, hr %#x, type %s.\n", hr, resources[r].name);

        hr = IDirectDrawSurface4_Lock(surface, NULL, NULL, DDLOCK_WAIT, NULL);
        ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x, type %s.\n", hr, resources[r].name);

        for (i = 0; i < ARRAY_SIZE(valid); ++i)
        {
            RECT *rect = &valid[i];

            memset(&surface_desc, 0, sizeof(surface_desc));
            surface_desc.dwSize = sizeof(surface_desc);

            hr = IDirectDrawSurface4_Lock(surface, rect, &surface_desc, DDLOCK_WAIT, NULL);
            ok(SUCCEEDED(hr), "Lock failed (%#x) for rect %s, type %s.\n",
                    hr, wine_dbgstr_rect(rect), resources[r].name);

            hr = IDirectDrawSurface4_Unlock(surface, NULL);
            ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x, type %s.\n", hr, resources[r].name);
        }

        for (i = 0; i < ARRAY_SIZE(invalid); ++i)
        {
            RECT *rect = &invalid[i];

            memset(&surface_desc, 1, sizeof(surface_desc));
            surface_desc.dwSize = sizeof(surface_desc);

            hr = IDirectDrawSurface4_Lock(surface, rect, &surface_desc, DDLOCK_WAIT, NULL);
            ok(hr == resources[r].hr, "Lock returned %#x for rect %s, type %s.\n",
                    hr, wine_dbgstr_rect(rect), resources[r].name);
            if (SUCCEEDED(hr))
            {
                hr = IDirectDrawSurface4_Unlock(surface, NULL);
                ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x, type %s.\n", hr, resources[r].name);
            }
            else
                ok(!surface_desc.lpSurface, "Got unexpected lpSurface %p.\n", surface_desc.lpSurface);
        }

        hr = IDirectDrawSurface4_Lock(surface, NULL, &surface_desc, DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Lock(rect = NULL) failed, hr %#x, type %s.\n",
                hr, resources[r].name);
        hr = IDirectDrawSurface4_Lock(surface, NULL, &surface_desc, DDLOCK_WAIT, NULL);
        ok(hr == DDERR_SURFACEBUSY, "Double lock(rect = NULL) returned %#x, type %s.\n",
                hr, resources[r].name);
        hr = IDirectDrawSurface4_Unlock(surface, NULL);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x, type %s.\n", hr, resources[r].name);

        hr = IDirectDrawSurface4_Lock(surface, &valid[0], &surface_desc, DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Lock(rect = %s) failed (%#x).\n", wine_dbgstr_rect(&valid[0]), hr);
        hr = IDirectDrawSurface4_Lock(surface, &valid[0], &surface_desc, DDLOCK_WAIT, NULL);
        ok(hr == DDERR_SURFACEBUSY, "Double lock(rect = %s) failed (%#x).\n",
                wine_dbgstr_rect(&valid[0]), hr);

        /* Locking a different rectangle returns DD_OK, but it seems to break the surface.
         * Afterwards unlocking the surface fails(NULL rectangle or both locked rectangles) */

        hr = IDirectDrawSurface4_Unlock(surface, NULL);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x, type %s.\n", hr, resources[r].name);

        IDirectDrawSurface4_Release(surface);
    }

done:
    IDirectDraw4_Release(ddraw);
    DestroyWindow(window);
}

static void test_yv12_overlay(void)
{
    IDirectDrawSurface4 *src_surface, *dst_surface;
    RECT rect = {13, 17, 14, 18};
    unsigned int offset, y;
    DDSURFACEDESC2 desc;
    unsigned char *base;
    IDirectDraw4 *ddraw;
    HWND window;
    HRESULT hr;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    if (!(src_surface = create_overlay(ddraw, 256, 256, MAKEFOURCC('Y','V','1','2'))))
    {
        skip("Failed to create a YV12 overlay, skipping test.\n");
        goto done;
    }

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    hr = IDirectDrawSurface4_Lock(src_surface, NULL, &desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);

    ok(desc.dwFlags == (DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CAPS | DDSD_PITCH),
            "Got unexpected flags %#x.\n", desc.dwFlags);
    ok(desc.ddsCaps.dwCaps == (DDSCAPS_OVERLAY | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM | DDSCAPS_HWCODEC)
            || desc.ddsCaps.dwCaps == (DDSCAPS_OVERLAY | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM),
            "Got unexpected caps %#x.\n", desc.ddsCaps.dwCaps);
    ok(desc.dwWidth == 256, "Got unexpected width %u.\n", desc.dwWidth);
    ok(desc.dwHeight == 256, "Got unexpected height %u.\n", desc.dwHeight);
    /* The overlay pitch seems to have 256 byte alignment. */
    ok(!(U1(desc).lPitch & 0xff), "Got unexpected pitch %u.\n", U1(desc).lPitch);

    /* Fill the surface with some data for the blit test. */
    base = desc.lpSurface;
    /* Luminance */
    for (y = 0; y < desc.dwHeight; ++y)
    {
        memset(base + U1(desc).lPitch * y, 0x10, desc.dwWidth);
    }
    /* V */
    for (; y < desc.dwHeight + desc.dwHeight / 4; ++y)
    {
        memset(base + U1(desc).lPitch * y, 0x20, desc.dwWidth);
    }
    /* U */
    for (; y < desc.dwHeight + desc.dwHeight / 2; ++y)
    {
        memset(base + U1(desc).lPitch * y, 0x30, desc.dwWidth);
    }

    hr = IDirectDrawSurface4_Unlock(src_surface, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    /* YV12 uses 2x2 blocks with 6 bytes per block (4*Y, 1*U, 1*V). Unlike
     * other block-based formats like DXT the entire Y channel is stored in
     * one big chunk of memory, followed by the chroma channels. So partial
     * locks do not really make sense. Show that they are allowed nevertheless
     * and the offset points into the luminance data. */
    hr = IDirectDrawSurface4_Lock(src_surface, &rect, &desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    offset = ((const unsigned char *)desc.lpSurface - base);
    ok(offset == rect.top * U1(desc).lPitch + rect.left, "Got unexpected offset %u, expected %u.\n",
            offset, rect.top * U1(desc).lPitch + rect.left);
    hr = IDirectDrawSurface4_Unlock(src_surface, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    if (!(dst_surface = create_overlay(ddraw, 256, 256, MAKEFOURCC('Y','V','1','2'))))
    {
        /* Windows XP with a Radeon X1600 GPU refuses to create a second
         * overlay surface, DDERR_NOOVERLAYHW, making the blit tests moot. */
        skip("Failed to create a second YV12 surface, skipping blit test.\n");
        IDirectDrawSurface4_Release(src_surface);
        goto done;
    }

    hr = IDirectDrawSurface4_Blt(dst_surface, NULL, src_surface, NULL, DDBLT_WAIT, NULL);
    /* VMware rejects YV12 blits. This behavior has not been seen on real
     * hardware yet, so mark it broken. */
    ok(SUCCEEDED(hr) || broken(hr == E_NOTIMPL), "Failed to blit, hr %#x.\n", hr);

    if (SUCCEEDED(hr))
    {
        memset(&desc, 0, sizeof(desc));
        desc.dwSize = sizeof(desc);
        hr = IDirectDrawSurface4_Lock(dst_surface, NULL, &desc, DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);

        base = desc.lpSurface;
        ok(base[0] == 0x10, "Got unexpected Y data 0x%02x.\n", base[0]);
        base += desc.dwHeight * U1(desc).lPitch;
        todo_wine ok(base[0] == 0x20, "Got unexpected V data 0x%02x.\n", base[0]);
        base += desc.dwHeight / 4 * U1(desc).lPitch;
        todo_wine ok(base[0] == 0x30, "Got unexpected U data 0x%02x.\n", base[0]);

        hr = IDirectDrawSurface4_Unlock(dst_surface, NULL);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
    }

    IDirectDrawSurface4_Release(dst_surface);
    IDirectDrawSurface4_Release(src_surface);
done:
    IDirectDraw4_Release(ddraw);
    DestroyWindow(window);
}

static BOOL dwm_enabled(void)
{
    BOOL ret = FALSE;

    if (!strcmp(winetest_platform, "wine"))
        return FALSE;
    if (!pDwmIsCompositionEnabled)
        return FALSE;
    if (FAILED(pDwmIsCompositionEnabled(&ret)))
        return FALSE;
    return ret;
}

static void test_offscreen_overlay(void)
{
    IDirectDrawSurface4 *overlay, *offscreen, *primary;
    DDSURFACEDESC2 surface_desc;
    IDirectDraw4 *ddraw;
    HWND window;
    HRESULT hr;
    HDC dc;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    if (!(overlay = create_overlay(ddraw, 64, 64, MAKEFOURCC('U','Y','V','Y'))))
    {
        skip("Failed to create a UYVY overlay, skipping test.\n");
        goto done;
    }

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);

    /* On Windows 7, and probably Vista, UpdateOverlay() will return
     * DDERR_OUTOFCAPS if the dwm is active. Calling GetDC() on the primary
     * surface prevents this by disabling the dwm. */
    hr = IDirectDrawSurface4_GetDC(primary, &dc);
    ok(SUCCEEDED(hr), "Failed to get DC, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_ReleaseDC(primary, dc);
    ok(SUCCEEDED(hr), "Failed to release DC, hr %#x.\n", hr);

    /* Try to overlay a NULL surface. */
    hr = IDirectDrawSurface4_UpdateOverlay(overlay, NULL, NULL, NULL, DDOVER_SHOW, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_UpdateOverlay(overlay, NULL, NULL, NULL, DDOVER_HIDE, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    /* Try to overlay an offscreen surface. */
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    surface_desc.dwWidth = 64;
    surface_desc.dwHeight = 64;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    U4(surface_desc).ddpfPixelFormat.dwSize = sizeof(U4(surface_desc).ddpfPixelFormat);
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U4(surface_desc).ddpfPixelFormat.dwFourCC = 0;
    U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount = 16;
    U2(U4(surface_desc).ddpfPixelFormat).dwRBitMask = 0xf800;
    U3(U4(surface_desc).ddpfPixelFormat).dwGBitMask = 0x07e0;
    U4(U4(surface_desc).ddpfPixelFormat).dwBBitMask = 0x001f;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &offscreen, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);

    hr = IDirectDrawSurface4_UpdateOverlay(overlay, NULL, offscreen, NULL, DDOVER_SHOW, NULL);
    ok(SUCCEEDED(hr) || broken(hr == DDERR_OUTOFCAPS && dwm_enabled())
            || broken(hr == E_NOTIMPL && ddraw_is_vmware(ddraw)),
            "Failed to update overlay, hr %#x.\n", hr);

    /* Try to overlay the primary with a non-overlay surface. */
    hr = IDirectDrawSurface4_UpdateOverlay(offscreen, NULL, primary, NULL, DDOVER_SHOW, NULL);
    ok(hr == DDERR_NOTAOVERLAYSURFACE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_UpdateOverlay(offscreen, NULL, primary, NULL, DDOVER_HIDE, NULL);
    ok(hr == DDERR_NOTAOVERLAYSURFACE, "Got unexpected hr %#x.\n", hr);

    IDirectDrawSurface4_Release(offscreen);
    IDirectDrawSurface4_Release(primary);
    IDirectDrawSurface4_Release(overlay);
done:
    IDirectDraw4_Release(ddraw);
    DestroyWindow(window);
}

static void test_overlay_rect(void)
{
    IDirectDrawSurface4 *overlay, *primary = NULL;
    DDSURFACEDESC2 surface_desc;
    RECT rect = {0, 0, 64, 64};
    IDirectDraw4 *ddraw;
    LONG pos_x, pos_y;
    HRESULT hr, hr2;
    HWND window;
    HDC dc;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    if (!(overlay = create_overlay(ddraw, 64, 64, MAKEFOURCC('U','Y','V','Y'))))
    {
        skip("Failed to create a UYVY overlay, skipping test.\n");
        goto done;
    }

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);

    /* On Windows 7, and probably Vista, UpdateOverlay() will return
     * DDERR_OUTOFCAPS if the dwm is active. Calling GetDC() on the primary
     * surface prevents this by disabling the dwm. */
    hr = IDirectDrawSurface4_GetDC(primary, &dc);
    ok(SUCCEEDED(hr), "Failed to get DC, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_ReleaseDC(primary, dc);
    ok(SUCCEEDED(hr), "Failed to release DC, hr %#x.\n", hr);

    /* On Windows 8 and newer DWM can't be turned off, making overlays unusable. */
    if (dwm_enabled())
    {
        win_skip("Cannot disable DWM, skipping overlay test.\n");
        goto done;
    }

    /* The dx sdk sort of implies that rect must be set when DDOVER_SHOW is
     * used. This is not true in Windows Vista and earlier, but changed in
     * Windows 7. */
    hr = IDirectDrawSurface4_UpdateOverlay(overlay, NULL, primary, &rect, DDOVER_SHOW, NULL);
    ok(SUCCEEDED(hr), "Failed to update overlay, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_UpdateOverlay(overlay, NULL, primary, NULL, DDOVER_HIDE, NULL);
    ok(SUCCEEDED(hr), "Failed to update overlay, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_UpdateOverlay(overlay, NULL, primary, NULL, DDOVER_SHOW, NULL);
    ok(hr == DD_OK || hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    /* Show that the overlay position is the (top, left) coordinate of the
     * destination rectangle. */
    OffsetRect(&rect, 32, 16);
    hr = IDirectDrawSurface4_UpdateOverlay(overlay, NULL, primary, &rect, DDOVER_SHOW, NULL);
    ok(SUCCEEDED(hr), "Failed to update overlay, hr %#x.\n", hr);
    pos_x = -1; pos_y = -1;
    hr = IDirectDrawSurface4_GetOverlayPosition(overlay, &pos_x, &pos_y);
    ok(SUCCEEDED(hr), "Failed to get overlay position, hr %#x.\n", hr);
    ok(pos_x == rect.left, "Got unexpected pos_x %d, expected %d.\n", pos_x, rect.left);
    ok(pos_y == rect.top, "Got unexpected pos_y %d, expected %d.\n", pos_y, rect.top);

    /* Passing a NULL dest rect sets the position to 0/0. Visually it can be
     * seen that the overlay overlays the whole primary(==screen). */
    hr2 = IDirectDrawSurface4_UpdateOverlay(overlay, NULL, primary, NULL, 0, NULL);
    ok(hr2 == DD_OK || hr2 == DDERR_INVALIDPARAMS || hr2 == DDERR_OUTOFCAPS, "Got unexpected hr %#x.\n", hr2);
    hr = IDirectDrawSurface4_GetOverlayPosition(overlay, &pos_x, &pos_y);
    ok(SUCCEEDED(hr), "Failed to get overlay position, hr %#x.\n", hr);
    if (SUCCEEDED(hr2))
    {
        ok(!pos_x, "Got unexpected pos_x %d.\n", pos_x);
        ok(!pos_y, "Got unexpected pos_y %d.\n", pos_y);
    }
    else
    {
        ok(pos_x == 32, "Got unexpected pos_x %d.\n", pos_x);
        ok(pos_y == 16, "Got unexpected pos_y %d.\n", pos_y);
    }

    /* The position cannot be retrieved when the overlay is not shown. */
    hr = IDirectDrawSurface4_UpdateOverlay(overlay, NULL, primary, &rect, DDOVER_HIDE, NULL);
    ok(SUCCEEDED(hr), "Failed to update overlay, hr %#x.\n", hr);
    pos_x = -1; pos_y = -1;
    hr = IDirectDrawSurface4_GetOverlayPosition(overlay, &pos_x, &pos_y);
    ok(hr == DDERR_OVERLAYNOTVISIBLE, "Got unexpected hr %#x.\n", hr);
    ok(!pos_x, "Got unexpected pos_x %d.\n", pos_x);
    ok(!pos_y, "Got unexpected pos_y %d.\n", pos_y);

    IDirectDrawSurface4_Release(overlay);
done:
    if (primary)
        IDirectDrawSurface4_Release(primary);
    IDirectDraw4_Release(ddraw);
    DestroyWindow(window);
}

static void test_blt(void)
{
    IDirectDrawSurface4 *surface, *rt;
    DDSURFACEDESC2 surface_desc;
    IDirect3DDevice3 *device;
    IDirectDraw4 *ddraw;
    IDirect3D3 *d3d;
    unsigned int i;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    static struct
    {
        RECT src_rect;
        RECT dst_rect;
        HRESULT hr;
    }
    test_data[] =
    {
        {{160,   0, 640, 480}, {  0,   0, 480, 480}, DD_OK},             /* Overlapped blit. */
        {{160, 480, 640,   0}, {  0,   0, 480, 480}, DDERR_INVALIDRECT}, /* Overlapped blit, flipped source. */
        {{640,   0, 160, 480}, {  0,   0, 480, 480}, DDERR_INVALIDRECT}, /* Overlapped blit, mirrored source. */
        {{160,   0, 480, 480}, {  0,   0, 480, 480}, DD_OK},             /* Overlapped blit, stretched x. */
        {{160, 160, 640, 480}, {  0,   0, 480, 480}, DD_OK},             /* Overlapped blit, stretched y. */
        {{  0,   0, 640, 480}, {  0,   0, 640, 480}, DD_OK},             /* Full surface blit. */
        {{  0,   0, 640, 480}, {  0, 480, 640,   0}, DDERR_INVALIDRECT}, /* Full surface, flipped destination. */
        {{  0,   0, 640, 480}, {640,   0,   0, 480}, DDERR_INVALIDRECT}, /* Full surface, mirrored destination. */
        {{  0, 480, 640,   0}, {  0,   0, 640, 480}, DDERR_INVALIDRECT}, /* Full surface, flipped source. */
        {{640,   0,   0, 480}, {  0,   0, 640, 480}, DDERR_INVALIDRECT}, /* Full surface, mirrored source. */
    };

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get Direct3D3 interface, hr %#x.\n", hr);
    hr = IDirect3D3_QueryInterface(d3d, &IID_IDirectDraw4, (void **)&ddraw);
    ok(SUCCEEDED(hr), "Failed to get DirectDraw4 interface, hr %#x.\n", hr);
    IDirect3D3_Release(d3d);
    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
    surface_desc.dwWidth = 640;
    surface_desc.dwHeight = 480;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_Blt(surface, NULL, surface, NULL, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_Blt(surface, NULL, rt, NULL, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(test_data); ++i)
    {
        hr = IDirectDrawSurface4_Blt(surface, &test_data[i].dst_rect,
                surface, &test_data[i].src_rect, DDBLT_WAIT, NULL);
        ok(hr == test_data[i].hr, "Test %u: Got unexpected hr %#x, expected %#x.\n", i, hr, test_data[i].hr);

        hr = IDirectDrawSurface4_Blt(surface, &test_data[i].dst_rect,
                rt, &test_data[i].src_rect, DDBLT_WAIT, NULL);
        ok(hr == test_data[i].hr, "Test %u: Got unexpected hr %#x, expected %#x.\n", i, hr, test_data[i].hr);

        hr = IDirectDrawSurface4_Blt(surface, &test_data[i].dst_rect,
                NULL, &test_data[i].src_rect, DDBLT_WAIT, NULL);
        ok(hr == DDERR_INVALIDPARAMS, "Test %u: Got unexpected hr %#x.\n", i, hr);

        hr = IDirectDrawSurface4_Blt(surface, &test_data[i].dst_rect, NULL, NULL, DDBLT_WAIT, NULL);
        ok(hr == DDERR_INVALIDPARAMS, "Test %u: Got unexpected hr %#x.\n", i, hr);
    }

    IDirectDrawSurface4_Release(surface);
    IDirectDrawSurface4_Release(rt);
    IDirectDraw4_Release(ddraw);
    refcount = IDirect3DDevice3_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_blt_z_alpha(void)
{
    DWORD blt_flags[] =
    {
        /* 0 */
        DDBLT_ALPHADEST,
        DDBLT_ALPHADESTCONSTOVERRIDE,
        DDBLT_ALPHADESTNEG,
        DDBLT_ALPHADESTSURFACEOVERRIDE,
        DDBLT_ALPHAEDGEBLEND,
        /* 5 */
        DDBLT_ALPHASRC,
        DDBLT_ALPHASRCCONSTOVERRIDE,
        DDBLT_ALPHASRCNEG,
        DDBLT_ALPHASRCSURFACEOVERRIDE,
        DDBLT_ZBUFFER,
        /* 10 */
        DDBLT_ZBUFFERDESTCONSTOVERRIDE,
        DDBLT_ZBUFFERDESTOVERRIDE,
        DDBLT_ZBUFFERSRCCONSTOVERRIDE,
        DDBLT_ZBUFFERSRCOVERRIDE,
    };
    IDirectDrawSurface4 *src_surface, *dst_surface;
    DDSURFACEDESC2 surface_desc;
    IDirectDraw4 *ddraw;
    DDPIXELFORMAT pf;
    ULONG refcount;
    unsigned int i;
    D3DCOLOR color;
    HWND window;
    HRESULT hr;
    DDBLTFX fx;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&pf, 0, sizeof(pf));
    pf.dwSize = sizeof(pf);
    pf.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    U1(pf).dwRGBBitCount = 32;
    U2(pf).dwRBitMask = 0x00ff0000;
    U3(pf).dwGBitMask = 0x0000ff00;
    U4(pf).dwBBitMask = 0x000000ff;
    U5(pf).dwRGBAlphaBitMask = 0xff000000;

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT;
    surface_desc.dwWidth = 64;
    surface_desc.dwHeight = 64;
    U4(surface_desc).ddpfPixelFormat = pf;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &src_surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create source surface, hr %#x.\n", hr);
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &dst_surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create destination surface, hr %#x.\n", hr);

    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    fx.dwZBufferOpCode = D3DCMP_NEVER;
    fx.dwZDestConstBitDepth = 32;
    U1(fx).dwZDestConst = 0x11111111;
    fx.dwZSrcConstBitDepth = 32;
    U2(fx).dwZSrcConst = 0xeeeeeeee;
    fx.dwAlphaEdgeBlendBitDepth = 8;
    fx.dwAlphaEdgeBlend = 0x7f;
    fx.dwAlphaDestConstBitDepth = 8;
    U3(fx).dwAlphaDestConst = 0xdd;
    fx.dwAlphaSrcConstBitDepth = 8;
    U4(fx).dwAlphaSrcConst = 0x22;

    for (i = 0; i < ARRAY_SIZE(blt_flags); ++i)
    {
        fx.dwFillColor = 0x3300ff00;
        hr = IDirectDrawSurface4_Blt(src_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
        ok(SUCCEEDED(hr), "Test %u: Got unexpected hr %#x.\n", i, hr);

        fx.dwFillColor = 0xccff0000;
        hr = IDirectDrawSurface4_Blt(dst_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
        ok(SUCCEEDED(hr), "Test %u: Got unexpected hr %#x.\n", i, hr);

        hr = IDirectDrawSurface4_Blt(dst_surface, NULL, src_surface, NULL, blt_flags[i] | DDBLT_WAIT, &fx);
        ok(SUCCEEDED(hr), "Test %u: Got unexpected hr %#x.\n", i, hr);

        color = get_surface_color(dst_surface, 32, 32);
        ok(compare_color(color, 0x0000ff00, 0), "Test %u: Got unexpected color 0x%08x.\n", i, color);
    }

    IDirectDrawSurface4_Release(dst_surface);
    IDirectDrawSurface4_Release(src_surface);
    refcount = IDirectDraw4_Release(ddraw);
    ok(!refcount, "DirectDraw has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_color_clamping(void)
{
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    static D3DMATRIX mat =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    static struct vec3 quad[] =
    {
        {-1.0f, -1.0f, 0.1f},
        {-1.0f,  1.0f, 0.1f},
        { 1.0f, -1.0f, 0.1f},
        { 1.0f,  1.0f, 0.1f},
    };
    IDirect3DViewport3 *viewport;
    IDirect3DDevice3 *device;
    IDirectDrawSurface4 *rt;
    ULONG refcount;
    D3DCOLOR color;
    HWND window;
    HRESULT hr;

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    viewport = create_viewport(device, 0, 0, 640, 480);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to activate the viewport, hr %#x.\n", hr);

    hr = IDirect3DDevice3_SetTransform(device, D3DTRANSFORMSTATE_WORLD, &mat);
    ok(SUCCEEDED(hr), "Failed to set world transform, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTransform(device, D3DTRANSFORMSTATE_VIEW, &mat);
    ok(SUCCEEDED(hr), "Failed to set view transform, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTransform(device, D3DTRANSFORMSTATE_PROJECTION, &mat);
    ok(SUCCEEDED(hr), "Failed to set projection transform, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_CLIPPING, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable clipping, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_ZENABLE, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable Z test, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_FOGENABLE, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable fog, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_STENCILENABLE, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable stencil test, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
    ok(SUCCEEDED(hr), "Failed to disable culling, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable lighting, hr %#x.\n", hr);

    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_TEXTUREFACTOR, 0xff404040);
    ok(SUCCEEDED(hr), "Failed to set texture factor, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_ADD);
    ok(SUCCEEDED(hr), "Failed to set color op, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    ok(SUCCEEDED(hr), "Failed to set color arg, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTextureStageState(device, 0, D3DTSS_COLORARG2, D3DTA_SPECULAR);
    ok(SUCCEEDED(hr), "Failed to set color arg, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTextureStageState(device, 1, D3DTSS_COLOROP, D3DTOP_MODULATE);
    ok(SUCCEEDED(hr), "Failed to set color op, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTextureStageState(device, 1, D3DTSS_COLORARG1, D3DTA_TFACTOR);
    ok(SUCCEEDED(hr), "Failed to set color arg, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTextureStageState(device, 1, D3DTSS_COLORARG2, D3DTA_CURRENT);
    ok(SUCCEEDED(hr), "Failed to set color arg, hr %#x.\n", hr);

    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0xff00ff00, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);

    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);

    hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ, quad, 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x00404040, 1), "Got unexpected color 0x%08x.\n", color);

    destroy_viewport(device, viewport);
    IDirectDrawSurface4_Release(rt);
    refcount = IDirect3DDevice3_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_getdc(void)
{
    DDSCAPS2 caps = {DDSCAPS_COMPLEX, 0, 0, {0}};
    IDirectDrawSurface4 *surface, *surface2, *tmp;
    DDSURFACEDESC2 surface_desc, map_desc;
    IDirectDraw4 *ddraw;
    unsigned int i;
    HWND window;
    HDC dc, dc2;
    HRESULT hr;

    static const struct
    {
        const char *name;
        DDPIXELFORMAT format;
        BOOL getdc_supported;
        HRESULT alt_result;
    }
    test_data[] =
    {
        {"D3DFMT_A8R8G8B8",    {sizeof(test_data->format), DDPF_RGB | DDPF_ALPHAPIXELS, 0, {32},
                {0x00ff0000}, {0x0000ff00}, {0x000000ff}, {0xff000000}}, TRUE},
        {"D3DFMT_X8R8G8B8",    {sizeof(test_data->format), DDPF_RGB, 0, {32},
                {0x00ff0000}, {0x0000ff00}, {0x000000ff}, {0x00000000}}, TRUE},
        {"D3DFMT_R5G6B5",      {sizeof(test_data->format), DDPF_RGB, 0, {16},
                {0x0000f800}, {0x000007e0}, {0x0000001f}, {0x00000000}}, TRUE},
        {"D3DFMT_X1R5G5B5",    {sizeof(test_data->format), DDPF_RGB, 0, {16},
                {0x00007c00}, {0x000003e0}, {0x0000001f}, {0x00000000}}, TRUE},
        {"D3DFMT_A1R5G5B5",    {sizeof(test_data->format), DDPF_RGB | DDPF_ALPHAPIXELS, 0, {16},
                {0x00007c00}, {0x000003e0}, {0x0000001f}, {0x00008000}}, TRUE},
        {"D3DFMT_A4R4G4B4",    {sizeof(test_data->format), DDPF_RGB | DDPF_ALPHAPIXELS, 0, {16},
                {0x00000f00}, {0x000000f0}, {0x0000000f}, {0x0000f000}}, TRUE, DDERR_CANTCREATEDC /* Vista+ */},
        {"D3DFMT_X4R4G4B4",    {sizeof(test_data->format), DDPF_RGB, 0, {16},
                {0x00000f00}, {0x000000f0}, {0x0000000f}, {0x00000000}}, TRUE, DDERR_CANTCREATEDC /* Vista+ */},
        {"D3DFMT_A2R10G10B10", {sizeof(test_data->format), DDPF_RGB | DDPF_ALPHAPIXELS, 0, {32},
                {0xc0000000}, {0x3ff00000}, {0x000ffc00}, {0x000003ff}}, TRUE},
        {"D3DFMT_A8B8G8R8",    {sizeof(test_data->format), DDPF_RGB | DDPF_ALPHAPIXELS, 0, {32},
                {0x000000ff}, {0x0000ff00}, {0x00ff0000}, {0xff000000}}, TRUE, DDERR_CANTCREATEDC /* Vista+ */},
        {"D3DFMT_X8B8G8R8",    {sizeof(test_data->format), DDPF_RGB, 0, {32},
                {0x000000ff}, {0x0000ff00}, {0x00ff0000}, {0x00000000}}, TRUE, DDERR_CANTCREATEDC /* Vista+ */},
        {"D3DFMT_R3G3B2",      {sizeof(test_data->format), DDPF_RGB, 0, {8},
                {0x000000e0}, {0x0000001c}, {0x00000003}, {0x00000000}}, FALSE},
        /* GetDC() on a P8 surface fails unless the display mode is 8 bpp.
         * This is not implemented in wine yet, so disable the test for now.
         * Succeeding P8 GetDC() calls are tested in the ddraw:visual test.
        {"D3DFMT_P8", {sizeof(test_data->format), DDPF_PALETTEINDEXED8 | DDPF_RGB, 0, {8 },
                {0x00000000}, {0x00000000}, {0x00000000}, {0x00000000}}, FALSE},
         */
        {"D3DFMT_L8",          {sizeof(test_data->format), DDPF_LUMINANCE, 0, {8},
                {0x000000ff}, {0x00000000}, {0x00000000}, {0x00000000}}, FALSE},
        {"D3DFMT_A8L8",        {sizeof(test_data->format), DDPF_ALPHAPIXELS | DDPF_LUMINANCE, 0, {16},
                {0x000000ff}, {0x00000000}, {0x00000000}, {0x0000ff00}}, FALSE},
        {"D3DFMT_DXT1",        {sizeof(test_data->format), DDPF_FOURCC, MAKEFOURCC('D','X','T','1'), {0},
                {0x00000000}, {0x00000000}, {0x00000000}, {0x00000000}}, FALSE},
        {"D3DFMT_DXT2",        {sizeof(test_data->format), DDPF_FOURCC, MAKEFOURCC('D','X','T','2'), {0},
                {0x00000000}, {0x00000000}, {0x00000000}, {0x00000000}}, FALSE},
        {"D3DFMT_DXT3",        {sizeof(test_data->format), DDPF_FOURCC, MAKEFOURCC('D','X','T','3'), {0},
                {0x00000000}, {0x00000000}, {0x00000000}, {0x00000000}}, FALSE},
        {"D3DFMT_DXT4",        {sizeof(test_data->format), DDPF_FOURCC, MAKEFOURCC('D','X','T','4'), {0},
                {0x00000000}, {0x00000000}, {0x00000000}, {0x00000000}}, FALSE},
        {"D3DFMT_DXT5",        {sizeof(test_data->format), DDPF_FOURCC, MAKEFOURCC('D','X','T','5'), {0},
                {0x00000000}, {0x00000000}, {0x00000000}, {0x00000000}}, FALSE},
    };

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(test_data); ++i)
    {
        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
        surface_desc.dwWidth = 64;
        surface_desc.dwHeight = 64;
        U4(surface_desc).ddpfPixelFormat = test_data[i].format;
        surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

        if (FAILED(IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL)))
        {
            surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
            surface_desc.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
            if (FAILED(hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL)))
            {
                skip("Failed to create surface for format %s (hr %#x), skipping tests.\n", test_data[i].name, hr);
                continue;
            }
        }

        dc = (void *)0x1234;
        hr = IDirectDrawSurface4_GetDC(surface, &dc);
        if (test_data[i].getdc_supported)
            ok(SUCCEEDED(hr) || broken(hr == test_data[i].alt_result),
                    "Got unexpected hr %#x for format %s.\n", hr, test_data[i].name);
        else
            ok(FAILED(hr), "Got unexpected hr %#x for format %s.\n", hr, test_data[i].name);

        if (SUCCEEDED(hr))
        {
            unsigned int width_bytes;
            DIBSECTION dib;
            HBITMAP bitmap;
            DWORD type;
            int size;

            type = GetObjectType(dc);
            ok(type == OBJ_MEMDC, "Got unexpected object type %#x for format %s.\n", type, test_data[i].name);
            bitmap = GetCurrentObject(dc, OBJ_BITMAP);
            type = GetObjectType(bitmap);
            ok(type == OBJ_BITMAP, "Got unexpected object type %#x for format %s.\n", type, test_data[i].name);

            size = GetObjectA(bitmap, sizeof(dib), &dib);
            ok(size == sizeof(dib), "Got unexpected size %d for format %s.\n", size, test_data[i].name);
            ok(!dib.dsBm.bmType, "Got unexpected type %#x for format %s.\n",
                    dib.dsBm.bmType, test_data[i].name);
            ok(dib.dsBm.bmWidth == surface_desc.dwWidth, "Got unexpected width %d for format %s.\n",
                    dib.dsBm.bmWidth, test_data[i].name);
            ok(dib.dsBm.bmHeight == surface_desc.dwHeight, "Got unexpected height %d for format %s.\n",
                    dib.dsBm.bmHeight, test_data[i].name);
            width_bytes = ((dib.dsBm.bmWidth * U1(test_data[i].format).dwRGBBitCount + 31) >> 3) & ~3;
            ok(dib.dsBm.bmWidthBytes == width_bytes, "Got unexpected width bytes %d for format %s.\n",
                    dib.dsBm.bmWidthBytes, test_data[i].name);
            ok(dib.dsBm.bmPlanes == 1, "Got unexpected plane count %d for format %s.\n",
                    dib.dsBm.bmPlanes, test_data[i].name);
            ok(dib.dsBm.bmBitsPixel == U1(test_data[i].format).dwRGBBitCount,
                    "Got unexpected bit count %d for format %s.\n",
                    dib.dsBm.bmBitsPixel, test_data[i].name);
            ok(!!dib.dsBm.bmBits, "Got unexpected bits %p for format %s.\n",
                    dib.dsBm.bmBits, test_data[i].name);

            ok(dib.dsBmih.biSize == sizeof(dib.dsBmih), "Got unexpected size %u for format %s.\n",
                    dib.dsBmih.biSize, test_data[i].name);
            ok(dib.dsBmih.biWidth == surface_desc.dwWidth, "Got unexpected width %d for format %s.\n",
                    dib.dsBmih.biHeight, test_data[i].name);
            ok(dib.dsBmih.biHeight == surface_desc.dwHeight, "Got unexpected height %d for format %s.\n",
                    dib.dsBmih.biHeight, test_data[i].name);
            ok(dib.dsBmih.biPlanes == 1, "Got unexpected plane count %u for format %s.\n",
                    dib.dsBmih.biPlanes, test_data[i].name);
            ok(dib.dsBmih.biBitCount == U1(test_data[i].format).dwRGBBitCount,
                    "Got unexpected bit count %u for format %s.\n",
                    dib.dsBmih.biBitCount, test_data[i].name);
            ok(dib.dsBmih.biCompression == (U1(test_data[i].format).dwRGBBitCount == 16 ? BI_BITFIELDS : BI_RGB)
                    || broken(U1(test_data[i].format).dwRGBBitCount == 32 && dib.dsBmih.biCompression == BI_BITFIELDS),
                    "Got unexpected compression %#x for format %s.\n",
                    dib.dsBmih.biCompression, test_data[i].name);
            ok(!dib.dsBmih.biSizeImage, "Got unexpected image size %u for format %s.\n",
                    dib.dsBmih.biSizeImage, test_data[i].name);
            ok(!dib.dsBmih.biXPelsPerMeter, "Got unexpected horizontal resolution %d for format %s.\n",
                    dib.dsBmih.biXPelsPerMeter, test_data[i].name);
            ok(!dib.dsBmih.biYPelsPerMeter, "Got unexpected vertical resolution %d for format %s.\n",
                    dib.dsBmih.biYPelsPerMeter, test_data[i].name);
            ok(!dib.dsBmih.biClrUsed, "Got unexpected used colour count %u for format %s.\n",
                    dib.dsBmih.biClrUsed, test_data[i].name);
            ok(!dib.dsBmih.biClrImportant, "Got unexpected important colour count %u for format %s.\n",
                    dib.dsBmih.biClrImportant, test_data[i].name);

            if (dib.dsBmih.biCompression == BI_BITFIELDS)
            {
                ok((dib.dsBitfields[0] == U2(test_data[i].format).dwRBitMask
                        && dib.dsBitfields[1] == U3(test_data[i].format).dwGBitMask
                        && dib.dsBitfields[2] == U4(test_data[i].format).dwBBitMask)
                        || broken(!dib.dsBitfields[0] && !dib.dsBitfields[1] && !dib.dsBitfields[2]),
                        "Got unexpected colour masks 0x%08x 0x%08x 0x%08x for format %s.\n",
                        dib.dsBitfields[0], dib.dsBitfields[1], dib.dsBitfields[2], test_data[i].name);
            }
            else
            {
                ok(!dib.dsBitfields[0] && !dib.dsBitfields[1] && !dib.dsBitfields[2],
                        "Got unexpected colour masks 0x%08x 0x%08x 0x%08x for format %s.\n",
                        dib.dsBitfields[0], dib.dsBitfields[1], dib.dsBitfields[2], test_data[i].name);
            }
            ok(!dib.dshSection, "Got unexpected section %p for format %s.\n", dib.dshSection, test_data[i].name);
            ok(!dib.dsOffset, "Got unexpected offset %u for format %s.\n", dib.dsOffset, test_data[i].name);

            hr = IDirectDrawSurface4_ReleaseDC(surface, dc);
            ok(hr == DD_OK, "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);
        }
        else
        {
            ok(!dc, "Got unexpected dc %p for format %s.\n", dc, test_data[i].name);
        }

        IDirectDrawSurface4_Release(surface);

        if (FAILED(hr))
            continue;

        surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
        surface_desc.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
        if (FAILED(hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL)))
        {
            skip("Failed to create mip-mapped texture for format %s (hr %#x), skipping tests.\n",
                    test_data[i].name, hr);
            continue;
        }

        hr = IDirectDrawSurface4_GetAttachedSurface(surface, &caps, &tmp);
        ok(SUCCEEDED(hr), "Failed to get attached surface for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_GetAttachedSurface(tmp, &caps, &surface2);
        ok(SUCCEEDED(hr), "Failed to get attached surface for format %s, hr %#x.\n", test_data[i].name, hr);
        IDirectDrawSurface4_Release(tmp);

        hr = IDirectDrawSurface4_GetDC(surface, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_ReleaseDC(surface, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_GetDC(surface2, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_ReleaseDC(surface2, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);

        hr = IDirectDrawSurface4_GetDC(surface, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", test_data[i].name, hr);
        dc2 = (void *)0x1234;
        hr = IDirectDrawSurface4_GetDC(surface, &dc2);
        ok(hr == DDERR_DCALREADYCREATED, "Got unexpected hr %#x for format %s.\n", hr, test_data[i].name);
        ok(dc2 == (void *)0x1234, "Got unexpected dc %p for format %s.\n", dc, test_data[i].name);
        hr = IDirectDrawSurface4_ReleaseDC(surface, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_ReleaseDC(surface, dc);
        ok(hr == DDERR_NODC, "Got unexpected hr %#x for format %s.\n", hr, test_data[i].name);

        map_desc.dwSize = sizeof(map_desc);
        hr = IDirectDrawSurface4_Lock(surface, NULL, &map_desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Failed to map surface for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_Lock(surface, NULL, &map_desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
        ok(hr == DDERR_SURFACEBUSY, "Got unexpected hr %#x for format %s.\n", hr, test_data[i].name);
        hr = IDirectDrawSurface4_Unlock(surface, NULL);
        ok(SUCCEEDED(hr), "Failed to unmap surface for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_Unlock(surface, NULL);
        ok(hr == DDERR_NOTLOCKED, "Got unexpected hr %#x for format %s.\n", hr, test_data[i].name);

        hr = IDirectDrawSurface4_GetDC(surface, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_Lock(surface, NULL, &map_desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
        ok(hr == DDERR_SURFACEBUSY, "Got unexpected hr %#x for format %s.\n", hr, test_data[i].name);
        hr = IDirectDrawSurface4_ReleaseDC(surface, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);

        hr = IDirectDrawSurface4_Lock(surface, NULL, &map_desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Failed to map surface for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_GetDC(surface, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_ReleaseDC(surface, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_Unlock(surface, NULL);
        ok(SUCCEEDED(hr), "Failed to unmap surface for format %s, hr %#x.\n", test_data[i].name, hr);

        hr = IDirectDrawSurface4_GetDC(surface, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_GetDC(surface2, &dc2);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_ReleaseDC(surface2, dc2);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_ReleaseDC(surface, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);

        hr = IDirectDrawSurface4_GetDC(surface2, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_GetDC(surface, &dc2);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_ReleaseDC(surface, dc2);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_ReleaseDC(surface2, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);

        hr = IDirectDrawSurface4_Lock(surface, NULL, &map_desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Failed to map surface for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_Lock(surface2, NULL, &map_desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Failed to map surface for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_Unlock(surface2, NULL);
        ok(SUCCEEDED(hr), "Failed to unmap surface for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_Unlock(surface, NULL);
        ok(SUCCEEDED(hr), "Failed to unmap surface for format %s, hr %#x.\n", test_data[i].name, hr);

        hr = IDirectDrawSurface4_Lock(surface, NULL, &map_desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Failed to map surface for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_GetDC(surface, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_ReleaseDC(surface, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_Unlock(surface, NULL);
        ok(SUCCEEDED(hr), "Failed to unmap surface for format %s, hr %#x.\n", test_data[i].name, hr);

        hr = IDirectDrawSurface4_Lock(surface2, NULL, &map_desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Failed to map surface for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_GetDC(surface, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_ReleaseDC(surface, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_Unlock(surface2, NULL);
        ok(SUCCEEDED(hr), "Failed to unmap surface for format %s, hr %#x.\n", test_data[i].name, hr);

        hr = IDirectDrawSurface4_GetDC(surface, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_Lock(surface2, NULL, &map_desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Failed to map surface for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_Unlock(surface2, NULL);
        ok(SUCCEEDED(hr), "Failed to unmap surface for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_ReleaseDC(surface, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);

        hr = IDirectDrawSurface4_GetDC(surface2, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_Lock(surface, NULL, &map_desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Failed to map surface for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_Unlock(surface, NULL);
        ok(SUCCEEDED(hr), "Failed to unmap surface for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_ReleaseDC(surface2, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);

        hr = IDirectDrawSurface4_Unlock(surface, NULL);
        ok(hr == DDERR_NOTLOCKED, "Got unexpected hr %#x for format %s.\n", hr, test_data[i].name);
        hr = IDirectDrawSurface4_GetDC(surface2, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_Unlock(surface, NULL);
        ok(hr == DDERR_NOTLOCKED, "Got unexpected hr %#x for format %s.\n", hr, test_data[i].name);
        hr = IDirectDrawSurface4_ReleaseDC(surface2, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_Unlock(surface, NULL);
        ok(hr == DDERR_NOTLOCKED, "Got unexpected hr %#x for format %s.\n", hr, test_data[i].name);

        hr = IDirectDrawSurface4_Unlock(surface2, NULL);
        ok(hr == DDERR_NOTLOCKED, "Got unexpected hr %#x for format %s.\n", hr, test_data[i].name);
        hr = IDirectDrawSurface4_GetDC(surface, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_Unlock(surface2, NULL);
        ok(hr == DDERR_NOTLOCKED, "Got unexpected hr %#x for format %s.\n", hr, test_data[i].name);
        hr = IDirectDrawSurface4_ReleaseDC(surface, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface4_Unlock(surface2, NULL);
        ok(hr == DDERR_NOTLOCKED, "Got unexpected hr %#x for format %s.\n", hr, test_data[i].name);

        IDirectDrawSurface4_Release(surface2);
        IDirectDrawSurface4_Release(surface);
    }

    IDirectDraw4_Release(ddraw);
    DestroyWindow(window);
}

static void test_draw_primitive(void)
{
    static WORD indices[] = {0, 1, 2, 3};
    static struct vec3 quad[] =
    {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f,  1.0f, 0.0f},
        { 1.0f, -1.0f, 0.0f},
        { 1.0f,  1.0f, 0.0f},
    };
    D3DDRAWPRIMITIVESTRIDEDDATA strided;
    IDirect3DViewport3 *viewport;
    D3DVERTEXBUFFERDESC vb_desc;
    IDirect3DVertexBuffer *vb;
    IDirect3DDevice3 *device;
    IDirect3D3 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    void *data;

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    viewport = create_viewport(device, 0, 0, 640, 480);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to activate the viewport, hr %#x.\n", hr);

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get D3D interface, hr %#x.\n", hr);

    memset(&vb_desc, 0, sizeof(vb_desc));
    vb_desc.dwSize = sizeof(vb_desc);
    vb_desc.dwFVF = D3DFVF_XYZ;
    vb_desc.dwNumVertices = 4;
    hr = IDirect3D3_CreateVertexBuffer(d3d, &vb_desc, &vb, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to create vertex buffer, hr %#x.\n", hr);

    IDirect3D3_Release(d3d);

    memset(&strided, 0, sizeof(strided));

    hr = IDirect3DDevice3_DrawIndexedPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ, NULL, 0, NULL, 0, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawIndexedPrimitiveStrided(device,
            D3DPT_TRIANGLESTRIP, D3DFVF_XYZ, &strided, 0, NULL, 0, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawIndexedPrimitiveVB(device, D3DPT_TRIANGLESTRIP, vb, NULL, 0, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ, NULL, 0, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitiveStrided(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ, &strided, 0, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitiveVB(device, D3DPT_TRIANGLESTRIP, vb, 0, 0, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    hr = IDirect3DDevice3_DrawIndexedPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ, NULL, 0, indices, 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawIndexedPrimitiveStrided(device,
            D3DPT_TRIANGLESTRIP, D3DFVF_XYZ, &strided, 0, indices, 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawIndexedPrimitiveVB(device, D3DPT_TRIANGLESTRIP, vb, indices, 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    strided.position.lpvData = quad;
    strided.position.dwStride = sizeof(*quad);
    hr = IDirect3DVertexBuffer_Lock(vb, 0, &data, NULL);
    ok(SUCCEEDED(hr), "Failed to lock vertex buffer, hr %#x.\n", hr);
    memcpy(data, quad, sizeof(quad));
    hr = IDirect3DVertexBuffer_Unlock(vb);
    ok(SUCCEEDED(hr), "Failed to unlock vertex buffer, hr %#x.\n", hr);

    hr = IDirect3DDevice3_DrawIndexedPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ, quad, 4, NULL, 0, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawIndexedPrimitiveStrided(device,
            D3DPT_TRIANGLESTRIP, D3DFVF_XYZ, &strided, 4, NULL, 0, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawIndexedPrimitiveVB(device, D3DPT_TRIANGLESTRIP, vb, NULL, 0, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ, quad, 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitiveStrided(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ, &strided, 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitiveVB(device, D3DPT_TRIANGLESTRIP, vb, 0, 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawIndexedPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ, quad, 4, indices, 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawIndexedPrimitiveStrided(device,
            D3DPT_TRIANGLESTRIP, D3DFVF_XYZ, &strided, 4, indices, 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawIndexedPrimitiveVB(device, D3DPT_TRIANGLESTRIP, vb, indices, 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    IDirect3DVertexBuffer_Release(vb);
    destroy_viewport(device, viewport);
    refcount = IDirect3DDevice3_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_edge_antialiasing_blending(void)
{
    D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    IDirectDrawSurface4 *offscreen, *ds;
    D3DDEVICEDESC hal_desc, hel_desc;
    IDirect3DViewport3 *viewport;
    DDSURFACEDESC2 surface_desc;
    IDirect3DDevice3 *device;
    IDirectDraw4 *ddraw;
    IDirect3D3 *d3d;
    ULONG refcount;
    D3DCOLOR color;
    HWND window;
    HRESULT hr;

    static D3DMATRIX mat =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    static struct
    {
        struct vec3 position;
        DWORD diffuse;
    }
    green_quad[] =
    {
        {{-1.0f, -1.0f, 0.1f}, 0x7f00ff00},
        {{-1.0f,  1.0f, 0.1f}, 0x7f00ff00},
        {{ 1.0f, -1.0f, 0.1f}, 0x7f00ff00},
        {{ 1.0f,  1.0f, 0.1f}, 0x7f00ff00},
    };
    static struct
    {
        struct vec3 position;
        DWORD diffuse;
    }
    red_quad[] =
    {
        {{-1.0f, -1.0f, 0.1f}, 0xccff0000},
        {{-1.0f,  1.0f, 0.1f}, 0xccff0000},
        {{ 1.0f, -1.0f, 0.1f}, 0xccff0000},
        {{ 1.0f,  1.0f, 0.1f}, 0xccff0000},
    };

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device.\n");
        DestroyWindow(window);
        return;
    }

    memset(&hal_desc, 0, sizeof(hal_desc));
    hal_desc.dwSize = sizeof(hal_desc);
    memset(&hel_desc, 0, sizeof(hel_desc));
    hel_desc.dwSize = sizeof(hel_desc);
    hr = IDirect3DDevice3_GetCaps(device, &hal_desc, &hel_desc);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    trace("HAL line edge antialiasing support: %#x.\n",
            hal_desc.dpcLineCaps.dwRasterCaps & D3DPRASTERCAPS_ANTIALIASEDGES);
    trace("HAL triangle edge antialiasing support: %#x.\n",
            hal_desc.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_ANTIALIASEDGES);
    trace("HEL line edge antialiasing support: %#x.\n",
            hel_desc.dpcLineCaps.dwRasterCaps & D3DPRASTERCAPS_ANTIALIASEDGES);
    trace("HEL triangle edge antialiasing support: %#x.\n",
            hel_desc.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_ANTIALIASEDGES);

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get D3D interface, hr %#x.\n", hr);
    hr = IDirect3D3_QueryInterface(d3d, &IID_IDirectDraw4, (void **)&ddraw);
    ok(SUCCEEDED(hr), "Failed to get DirectDraw4 interface, hr %#x.\n", hr);
    IDirect3D3_Release(d3d);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT;
    surface_desc.dwWidth = 640;
    surface_desc.dwHeight = 480;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_3DDEVICE;
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount = 32;
    U2(U4(surface_desc).ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(U4(surface_desc).ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(U4(surface_desc).ddpfPixelFormat).dwBBitMask = 0x000000ff;
    U5(U4(surface_desc).ddpfPixelFormat).dwRGBAlphaBitMask = 0xff000000;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &offscreen, NULL);
    ok(hr == D3D_OK, "Creating the offscreen render target failed, hr %#x.\n", hr);

    ds = get_depth_stencil(device);
    hr = IDirectDrawSurface_AddAttachedSurface(offscreen, ds);
    todo_wine ok(SUCCEEDED(hr), "Failed to attach depth buffer, hr %#x.\n", hr);
    IDirectDrawSurface_Release(ds);

    hr = IDirect3DDevice3_SetRenderTarget(device, offscreen, 0);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);

    viewport = create_viewport(device, 0, 0, 640, 480);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);

    hr = IDirect3DDevice3_SetTransform(device, D3DTRANSFORMSTATE_WORLD, &mat);
    ok(SUCCEEDED(hr), "Failed to set world transform, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTransform(device, D3DTRANSFORMSTATE_VIEW, &mat);
    ok(SUCCEEDED(hr), "Failed to set view transform, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTransform(device, D3DTRANSFORMSTATE_PROJECTION, &mat);
    ok(SUCCEEDED(hr), "Failed to set projection transform, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_CLIPPING, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable clipping, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_ZENABLE, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable Z test, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_FOGENABLE, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable fog, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_STENCILENABLE, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable stencil test, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
    ok(SUCCEEDED(hr), "Failed to disable culling, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable lighting, hr %#x.\n", hr);

    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
    ok(SUCCEEDED(hr), "Failed to enable blending, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
    ok(SUCCEEDED(hr), "Failed to set src blend, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_DESTBLEND, D3DBLEND_DESTALPHA);
    ok(SUCCEEDED(hr), "Failed to set dest blend, hr %#x.\n", hr);

    hr = IDirect3DDevice3_SetTextureStageState(device, 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    ok(SUCCEEDED(hr), "Failed to set color op, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTextureStageState(device, 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    ok(SUCCEEDED(hr), "Failed to set color arg, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTextureStageState(device, 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    ok(SUCCEEDED(hr), "Failed to set alpha op, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTextureStageState(device, 0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
    ok(SUCCEEDED(hr), "Failed to set alpha arg, hr %#x.\n", hr);

    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0xccff0000, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_DIFFUSE,
            green_quad, 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);
    color = get_surface_color(offscreen, 320, 240);
    ok(compare_color(color, 0x00cc7f00, 1), "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0x7f00ff00, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_DIFFUSE,
            red_quad, 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);
    color = get_surface_color(offscreen, 320, 240);
    ok(compare_color(color, 0x00cc7f00, 1), "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable blending, hr %#x.\n", hr);

    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0xccff0000, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_DIFFUSE,
            green_quad, 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);
    color = get_surface_color(offscreen, 320, 240);
    ok(compare_color(color, 0x0000ff00, 1), "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0x7f00ff00, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_DIFFUSE,
            red_quad, 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);
    color = get_surface_color(offscreen, 320, 240);
    ok(compare_color(color, 0x00ff0000, 1), "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_EDGEANTIALIAS, TRUE);
    ok(SUCCEEDED(hr), "Failed to enable edge antialiasing, hr %#x.\n", hr);

    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0xccff0000, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_DIFFUSE,
            green_quad, 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);
    color = get_surface_color(offscreen, 320, 240);
    ok(compare_color(color, 0x0000ff00, 1), "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0x7f00ff00, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_DIFFUSE,
            red_quad, 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);
    color = get_surface_color(offscreen, 320, 240);
    ok(compare_color(color, 0x00ff0000, 1), "Got unexpected color 0x%08x.\n", color);

    IDirectDrawSurface4_Release(offscreen);
    IDirectDraw3_Release(ddraw);
    destroy_viewport(device, viewport);
    refcount = IDirect3DDevice3_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    DestroyWindow(window);
}

/* TransformVertices always writes 32 bytes regardless of the input / output stride.
 * The stride is honored for navigating to the next vertex. 3 floats input position
 * are read, and 16 bytes extra vertex data are copied around. */
struct transform_input
{
    float x, y, z, unused1; /* Position data, transformed. */
    DWORD v1, v2, v3, v4; /* Extra data, e.g. color and texture coords, copied. */
    DWORD unused2;
};

struct transform_output
{
    float x, y, z, w;
    DWORD v1, v2, v3, v4;
    DWORD unused3, unused4;
};

static void test_transform_vertices(void)
{
    IDirect3DDevice3 *device;
    IDirectDrawSurface4 *rt;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    D3DCOLOR color;
    IDirect3DViewport3 *viewport;
    static struct transform_input position_tests[] =
    {
        { 0.0f,  0.0f,  0.0f, 0.0f,   1,   2,   3,   4,   5},
        { 1.0f,  1.0f,  1.0f, 8.0f,   6,   7,   8,   9,  10},
        {-1.0f, -1.0f, -1.0f, 4.0f,  11,  12,  13,  14,  15},
        { 0.5f,  0.5f,  0.5f, 2.0f,  16,  17,  18,  19,  20},
        {-0.5f, -0.5f, -0.5f, 1.0f, ~1U, ~2U, ~3U, ~4U, ~5U},
        {-0.5f, -0.5f,  0.0f, 0.0f, ~6U, ~7U, ~8U, ~9U, ~0U},
    };
    static struct transform_input cliptest[] =
    {
        { 25.59f,  25.59f,  1.0f,  0.0f, 1, 2, 3, 4, 5},
        { 25.61f,  25.61f,  1.01f, 0.0f, 1, 2, 3, 4, 5},
        {-25.59f, -25.59f,  0.0f,  0.0f, 1, 2, 3, 4, 5},
        {-25.61f, -25.61f, -0.01f, 0.0f, 1, 2, 3, 4, 5},
    };
    static struct transform_input offscreentest[] =
    {
        {128.1f, 0.0f, 0.0f, 0.0f, 1, 2, 3, 4, 5},
    };
    struct transform_output out[ARRAY_SIZE(position_tests)];
    D3DHVERTEX out_h[ARRAY_SIZE(position_tests)];
    D3DTRANSFORMDATA transformdata;
    static const D3DVIEWPORT vp_template =
    {
        sizeof(vp_template), 0, 0, 256, 256, 5.0f, 5.0f, 256.0f, 256.0f, -25.0f, 60.0f
    };
    D3DVIEWPORT vp_data =
    {
        sizeof(vp_data), 0, 0, 256, 256, 1.0f, 1.0f, 256.0f, 256.0f, 0.0f, 1.0f
    };
    D3DVIEWPORT2 vp2_data;
    unsigned int i;
    DWORD offscreen;
    static D3DMATRIX mat_scale =
    {
        2.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 2.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    },
    mat_translate1 =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 1.0f,
    },
    mat_translate2 =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
    },
    mat_transform3 =
    {
        1.0f,  0.0f, 0.0f, 0.0f,
        0.0f,  1.0f, 0.0f, 0.0f,
        0.0f,  0.0f, 1.0f, 0.0f,
        0.0f, 19.2f, 0.0f, 2.0f,
    },
    mat_identity =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    static struct
    {
        struct vec3 position;
        DWORD color;
    }
    quad[] =
    {
        {{-0.75f, -0.5f , 0.0f}, 0xffff0000},
        {{-0.75f,  0.25f, 0.0f}, 0xffff0000},
        {{ 0.5f,  -0.5f , 0.0f}, 0xffff0000},
        {{ 0.5f,   0.25f, 0.0f}, 0xffff0000},
    };
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};

    for (i = 0; i < ARRAY_SIZE(out); ++i)
    {
        out[i].unused3 = 0xdeadbeef;
        out[i].unused4 = 0xcafecafe;
    }

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device.\n");
        DestroyWindow(window);
        return;
    }
    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    viewport = create_viewport(device, 0, 0, 256, 256);
    hr = IDirect3DViewport2_SetViewport(viewport, &vp_data);
    ok(SUCCEEDED(hr), "Failed to set viewport, hr %#x.\n", hr);

    memset(&transformdata, 0, sizeof(transformdata));
    transformdata.dwSize = sizeof(transformdata);
    transformdata.lpIn = position_tests;
    transformdata.dwInSize = sizeof(position_tests[0]);
    transformdata.lpOut = out;
    transformdata.dwOutSize = sizeof(out[0]);
    transformdata.lpHOut = NULL;

    hr = IDirect3DViewport2_TransformVertices(viewport, ARRAY_SIZE(position_tests),
            &transformdata, D3DTRANSFORM_UNCLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(!offscreen, "Offscreen is %x.\n", offscreen);

    for (i = 0; i < ARRAY_SIZE(position_tests); ++i)
    {
        static const struct vec4 cmp[] =
        {
            {128.0f, 128.0f, 0.0f, 1.0f}, {129.0f, 127.0f,  1.0f, 1.0f}, {127.0f, 129.0f, -1.0f, 1.0f},
            {128.5f, 127.5f, 0.5f, 1.0f}, {127.5f, 128.5f, -0.5f, 1.0f}, {127.5f, 128.5f,  0.0f, 1.0f}
        };

        ok(compare_vec4(&cmp[i], out[i].x, out[i].y, out[i].z, out[i].w, 4096),
                "Vertex %u differs. Got %f %f %f %f.\n", i,
                out[i].x, out[i].y, out[i].z, out[i].w);
        ok(out[i].v1 == position_tests[i].v1 && out[i].v2 == position_tests[i].v2
                && out[i].v3 == position_tests[i].v3 && out[i].v4 == position_tests[i].v4,
                "Vertex %u payload is %u %u %u %u.\n", i, out[i].v1, out[i].v2, out[i].v3, out[i].v4);
        ok(out[i].unused3 == 0xdeadbeef && out[i].unused4 == 0xcafecafe,
                "Vertex %u unused data is %#x, %#x.\n", i, out[i].unused3, out[i].unused4);
    }

    vp_data = vp_template;
    hr = IDirect3DViewport2_SetViewport(viewport, &vp_data);
    ok(SUCCEEDED(hr), "Failed to set viewport, hr %#x.\n", hr);
    offscreen = 0xdeadbeef;
    hr = IDirect3DViewport2_TransformVertices(viewport, ARRAY_SIZE(position_tests),
            &transformdata, D3DTRANSFORM_UNCLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(!offscreen, "Offscreen is %x.\n", offscreen);

    for (i = 0; i < ARRAY_SIZE(position_tests); ++i)
    {
        static const struct vec4 cmp[] =
        {
            {128.0f, 128.0f, 0.0f, 1.0f}, {133.0f, 123.0f,  1.0f, 1.0f}, {123.0f, 133.0f, -1.0f, 1.0f},
            {130.5f, 125.5f, 0.5f, 1.0f}, {125.5f, 130.5f, -0.5f, 1.0f}, {125.5f, 130.5f,  0.0f, 1.0f}
        };
        ok(compare_vec4(&cmp[i], out[i].x, out[i].y, out[i].z, out[i].w, 4096),
                "Vertex %u differs. Got %f %f %f %f.\n", i,
                out[i].x, out[i].y, out[i].z, out[i].w);
    }

    vp_data.dwX = 10;
    vp_data.dwY = 20;
    hr = IDirect3DViewport2_SetViewport(viewport, &vp_data);
    ok(SUCCEEDED(hr), "Failed to set viewport, hr %#x.\n", hr);
    offscreen = 0xdeadbeef;
    hr = IDirect3DViewport2_TransformVertices(viewport, ARRAY_SIZE(position_tests),
            &transformdata, D3DTRANSFORM_UNCLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(!offscreen, "Offscreen is %x.\n", offscreen);
    for (i = 0; i < ARRAY_SIZE(position_tests); ++i)
    {
        static const struct vec4 cmp[] =
        {
            {138.0f, 148.0f, 0.0f, 1.0f}, {143.0f, 143.0f,  1.0f, 1.0f}, {133.0f, 153.0f, -1.0f, 1.0f},
            {140.5f, 145.5f, 0.5f, 1.0f}, {135.5f, 150.5f, -0.5f, 1.0f}, {135.5f, 150.5f,  0.0f, 1.0f}
        };
        ok(compare_vec4(&cmp[i], out[i].x, out[i].y, out[i].z, out[i].w, 4096),
                "Vertex %u differs. Got %f %f %f %f.\n", i,
                out[i].x, out[i].y, out[i].z, out[i].w);
    }

    transformdata.lpHOut = out_h;
    offscreen = 0xdeadbeef;
    hr = IDirect3DViewport2_TransformVertices(viewport, ARRAY_SIZE(position_tests),
            &transformdata, D3DTRANSFORM_CLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(!offscreen, "Offscreen is %x.\n", offscreen);
    for (i = 0; i < ARRAY_SIZE(position_tests); ++i)
    {
        static const D3DHVERTEX cmp_h[] =
        {
            {0,             { 0.0f}, { 0.0f}, { 0.0f}}, {0, { 1.0f}, { 1.0f}, {1.0f}},
            {D3DCLIP_FRONT, {-1.0f}, {-1.0f}, {-1.0f}}, {0, { 0.5f}, { 0.5f}, {0.5f}},
            {D3DCLIP_FRONT, {-0.5f}, {-0.5f}, {-0.5f}}, {0, {-0.5f}, {-0.5f}, {0.0f}}
        };
        ok(compare_float(U1(cmp_h[i]).hx, U1(out_h[i]).hx, 4096)
                && compare_float(U2(cmp_h[i]).hy, U2(out_h[i]).hy, 4096)
                && compare_float(U3(cmp_h[i]).hz, U3(out_h[i]).hz, 4096)
                && cmp_h[i].dwFlags == out_h[i].dwFlags,
                "HVertex %u differs. Got %#x %f %f %f.\n", i,
                out_h[i].dwFlags, U1(out_h[i]).hx, U2(out_h[i]).hy, U3(out_h[i]).hz);

        /* No scheme has been found behind those return values. It seems to be
         * whatever data windows has when throwing the vertex away. Modify the
         * input test vertices to test this more. Depending on the input data
         * it can happen that the z coord gets written into y, or similar things. */
        if (0)
        {
            static const struct vec4 cmp[] =
            {
                {138.0f, 148.0f, 0.0f, 1.0f}, {143.0f, 143.0f,  1.0f, 1.0f}, { -1.0f,  -1.0f, 0.5f, 1.0f},
                {140.5f, 145.5f, 0.5f, 1.0f}, { -0.5f,  -0.5f, -0.5f, 1.0f}, {135.5f, 150.5f, 0.0f, 1.0f}
            };
            ok(compare_vec4(&cmp[i], out[i].x, out[i].y, out[i].z, out[i].w, 4096),
                    "Vertex %u differs. Got %f %f %f %f.\n", i,
                    out[i].x, out[i].y, out[i].z, out[i].w);
        }
    }

    transformdata.lpIn = cliptest;
    transformdata.dwInSize = sizeof(cliptest[0]);
    offscreen = 0xdeadbeef;
    hr = IDirect3DViewport2_TransformVertices(viewport, ARRAY_SIZE(cliptest),
            &transformdata, D3DTRANSFORM_CLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(!offscreen, "Offscreen is %x.\n", offscreen);
    for (i = 0; i < ARRAY_SIZE(cliptest); ++i)
    {
        static const DWORD flags[] =
        {
            0,
            D3DCLIP_RIGHT | D3DCLIP_BACK   | D3DCLIP_TOP,
            0,
            D3DCLIP_LEFT  | D3DCLIP_BOTTOM | D3DCLIP_FRONT,
        };
        ok(flags[i] == out_h[i].dwFlags, "Cliptest %u returned %#x.\n", i, out_h[i].dwFlags);
    }

    vp_data = vp_template;
    vp_data.dwWidth = 10;
    vp_data.dwHeight = 480;
    hr = IDirect3DViewport2_SetViewport(viewport, &vp_data);
    ok(SUCCEEDED(hr), "Failed to set viewport, hr %#x.\n", hr);
    offscreen = 0xdeadbeef;
    hr = IDirect3DViewport2_TransformVertices(viewport, ARRAY_SIZE(cliptest),
            &transformdata, D3DTRANSFORM_CLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(!offscreen, "Offscreen is %x.\n", offscreen);
    for (i = 0; i < ARRAY_SIZE(cliptest); ++i)
    {
        static const DWORD flags[] =
        {
            D3DCLIP_RIGHT,
            D3DCLIP_RIGHT | D3DCLIP_BACK,
            D3DCLIP_LEFT,
            D3DCLIP_LEFT  | D3DCLIP_FRONT,
        };
        ok(flags[i] == out_h[i].dwFlags, "Cliptest %u returned %#x.\n", i, out_h[i].dwFlags);
    }

    vp_data = vp_template;
    vp_data.dwWidth = 256;
    vp_data.dwHeight = 256;
    vp_data.dvScaleX = 1;
    vp_data.dvScaleY = 1;
    hr = IDirect3DViewport2_SetViewport(viewport, &vp_data);
    ok(SUCCEEDED(hr), "Failed to set viewport, hr %#x.\n", hr);
    hr = IDirect3DViewport2_TransformVertices(viewport, ARRAY_SIZE(cliptest),
            &transformdata, D3DTRANSFORM_CLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(!offscreen, "Offscreen is %x.\n", offscreen);
    for (i = 0; i < ARRAY_SIZE(cliptest); ++i)
    {
        static const DWORD flags[] =
        {
            0,
            D3DCLIP_BACK,
            0,
            D3DCLIP_FRONT,
        };
        ok(flags[i] == out_h[i].dwFlags, "Cliptest %u returned %#x.\n", i, out_h[i].dwFlags);
    }

    /* Finally try to figure out how the DWORD dwOffscreen works.
     * It is a logical AND of the vertices' dwFlags members. */
    vp_data = vp_template;
    vp_data.dwWidth = 5;
    vp_data.dwHeight = 5;
    vp_data.dvScaleX = 10000.0f;
    vp_data.dvScaleY = 10000.0f;
    hr = IDirect3DViewport2_SetViewport(viewport, &vp_data);
    ok(SUCCEEDED(hr), "Failed to set viewport, hr %#x.\n", hr);
    transformdata.lpIn = cliptest;
    offscreen = 0xdeadbeef;
    hr = IDirect3DViewport2_TransformVertices(viewport, 1,
            &transformdata, D3DTRANSFORM_UNCLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(!offscreen, "Offscreen is %x.\n", offscreen);

    offscreen = 0xdeadbeef;
    hr = IDirect3DViewport2_TransformVertices(viewport, 1,
            &transformdata, D3DTRANSFORM_CLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(offscreen == (D3DCLIP_RIGHT | D3DCLIP_TOP), "Offscreen is %x.\n", offscreen);
    offscreen = 0xdeadbeef;
    hr = IDirect3DViewport2_TransformVertices(viewport, 2,
            &transformdata, D3DTRANSFORM_CLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(offscreen == (D3DCLIP_RIGHT | D3DCLIP_TOP), "Offscreen is %x.\n", offscreen);
    hr = IDirect3DViewport2_TransformVertices(viewport, 3,
            &transformdata, D3DTRANSFORM_CLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(!offscreen, "Offscreen is %x.\n", offscreen);

    transformdata.lpIn = cliptest + 1;
    hr = IDirect3DViewport2_TransformVertices(viewport, 1,
            &transformdata, D3DTRANSFORM_CLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(offscreen == (D3DCLIP_BACK | D3DCLIP_RIGHT | D3DCLIP_TOP), "Offscreen is %x.\n", offscreen);

    transformdata.lpIn = cliptest + 2;
    hr = IDirect3DViewport2_TransformVertices(viewport, 1,
            &transformdata, D3DTRANSFORM_CLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(offscreen == (D3DCLIP_BOTTOM | D3DCLIP_LEFT), "Offscreen is %x.\n", offscreen);
    offscreen = 0xdeadbeef;
    hr = IDirect3DViewport2_TransformVertices(viewport, 2,
            &transformdata, D3DTRANSFORM_CLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(offscreen == (D3DCLIP_BOTTOM | D3DCLIP_LEFT), "Offscreen is %x.\n", offscreen);

    transformdata.lpIn = cliptest + 3;
    hr = IDirect3DViewport2_TransformVertices(viewport, 1,
            &transformdata, D3DTRANSFORM_CLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(offscreen == (D3DCLIP_FRONT | D3DCLIP_BOTTOM | D3DCLIP_LEFT), "Offscreen is %x.\n", offscreen);

    transformdata.lpIn = offscreentest;
    transformdata.dwInSize = sizeof(offscreentest[0]);
    vp_data = vp_template;
    vp_data.dwWidth = 257;
    vp_data.dwHeight = 257;
    vp_data.dvScaleX = 1.0f;
    vp_data.dvScaleY = 1.0f;
    hr = IDirect3DViewport2_SetViewport(viewport, &vp_data);
    ok(SUCCEEDED(hr), "Failed to set viewport, hr %#x.\n", hr);
    offscreen = 0xdeadbeef;
    hr = IDirect3DViewport2_TransformVertices(viewport, 1,
            &transformdata, D3DTRANSFORM_CLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(!offscreen, "Offscreen is %x.\n", offscreen);

    vp_data.dwWidth = 256;
    vp_data.dwHeight = 256;
    hr = IDirect3DViewport2_SetViewport(viewport, &vp_data);
    ok(SUCCEEDED(hr), "Failed to set viewport, hr %#x.\n", hr);
    hr = IDirect3DViewport2_TransformVertices(viewport, 1,
            &transformdata, D3DTRANSFORM_CLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(offscreen == D3DCLIP_RIGHT, "Offscreen is %x.\n", offscreen);

    /* Test the effect of Matrices.
     *
     * Basically the x coordinate ends up as ((x + 1) * 2 + 0) * 5 and
     * y as ((y + 0) * 2 + 1) * 5. The 5 comes from dvScaleX/Y, 2 from
     * the view matrix and the +1's from the world and projection matrix. */
    vp_data.dwX = 0;
    vp_data.dwY = 0;
    vp_data.dwWidth = 256;
    vp_data.dwHeight = 256;
    vp_data.dvScaleX = 5.0f;
    vp_data.dvScaleY = 5.0f;
    vp_data.dvMinZ = 0.0f;
    vp_data.dvMaxZ = 1.0f;
    hr = IDirect3DViewport2_SetViewport(viewport, &vp_data);
    ok(SUCCEEDED(hr), "Failed to set viewport, hr %#x.\n", hr);

    hr = IDirect3DDevice3_SetTransform(device, D3DTRANSFORMSTATE_WORLD, &mat_translate1);
    ok(SUCCEEDED(hr), "Failed to set world transform, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTransform(device, D3DTRANSFORMSTATE_VIEW, &mat_scale);
    ok(SUCCEEDED(hr), "Failed to set world transform, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTransform(device, D3DTRANSFORMSTATE_PROJECTION, &mat_translate2);
    ok(SUCCEEDED(hr), "Failed to set world transform, hr %#x.\n", hr);

    transformdata.lpIn = position_tests;
    transformdata.dwInSize = sizeof(position_tests[0]);
    hr = IDirect3DViewport2_TransformVertices(viewport, ARRAY_SIZE(position_tests),
            &transformdata, D3DTRANSFORM_UNCLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(position_tests); ++i)
    {
        static const struct vec4 cmp[] =
        {
            {138.0f, 123.0f, 0.0f, 1.0f}, {148.0f, 113.0f,  2.0f, 1.0f}, {128.0f, 133.0f, -2.0f, 1.0f},
            {143.0f, 118.0f, 1.0f, 1.0f}, {133.0f, 128.0f, -1.0f, 1.0f}, {133.0f, 128.0f,  0.0f, 1.0f}
        };

        ok(compare_vec4(&cmp[i], out[i].x, out[i].y, out[i].z, out[i].w, 4096),
                "Vertex %u differs. Got %f %f %f %f.\n", i,
                out[i].x, out[i].y, out[i].z, out[i].w);
    }

    /* Invalid flags. */
    offscreen = 0xdeadbeef;
    hr = IDirect3DViewport2_TransformVertices(viewport, ARRAY_SIZE(position_tests),
            &transformdata, 0, &offscreen);
    ok(hr == DDERR_INVALIDPARAMS, "TransformVertices returned %#x.\n", hr);
    ok(offscreen == 0xdeadbeef, "Offscreen is %x.\n", offscreen);

    /* NULL transform data. */
    hr = IDirect3DViewport2_TransformVertices(viewport, 1,
            NULL, D3DTRANSFORM_UNCLIPPED, &offscreen);
    ok(hr == DDERR_INVALIDPARAMS, "TransformVertices returned %#x.\n", hr);
    ok(offscreen == 0xdeadbeef, "Offscreen is %x.\n", offscreen);
    hr = IDirect3DViewport2_TransformVertices(viewport, 0,
            NULL, D3DTRANSFORM_UNCLIPPED, &offscreen);
    ok(hr == DDERR_INVALIDPARAMS, "TransformVertices returned %#x.\n", hr);
    ok(offscreen == 0xdeadbeef, "Offscreen is %x.\n", offscreen);

    /* NULL transform data and NULL dwOffscreen.
     *
     * Valid transform data + NULL dwOffscreen -> crash. */
    hr = IDirect3DViewport2_TransformVertices(viewport, 1,
            NULL, D3DTRANSFORM_UNCLIPPED, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "TransformVertices returned %#x.\n", hr);

    /* No vertices. */
    hr = IDirect3DViewport2_TransformVertices(viewport, 0,
            &transformdata, D3DTRANSFORM_UNCLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(!offscreen, "Offscreen is %x.\n", offscreen);
    hr = IDirect3DViewport2_TransformVertices(viewport, 0,
            &transformdata, D3DTRANSFORM_CLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(offscreen == ~0U, "Offscreen is %x.\n", offscreen);

    /* Invalid sizes. */
    offscreen = 0xdeadbeef;
    transformdata.dwSize = sizeof(transformdata) - 1;
    hr = IDirect3DViewport2_TransformVertices(viewport, 1,
            &transformdata, D3DTRANSFORM_UNCLIPPED, &offscreen);
    ok(hr == DDERR_INVALIDPARAMS, "TransformVertices returned %#x.\n", hr);
    ok(offscreen == 0xdeadbeef, "Offscreen is %x.\n", offscreen);
    transformdata.dwSize = sizeof(transformdata) + 1;
    hr = IDirect3DViewport2_TransformVertices(viewport, 1,
            &transformdata, D3DTRANSFORM_UNCLIPPED, &offscreen);
    ok(hr == DDERR_INVALIDPARAMS, "TransformVertices returned %#x.\n", hr);
    ok(offscreen == 0xdeadbeef, "Offscreen is %x.\n", offscreen);

    /* NULL lpIn or lpOut -> crash, except when transforming 0 vertices. */
    transformdata.dwSize = sizeof(transformdata);
    transformdata.lpIn = NULL;
    transformdata.lpOut = NULL;
    offscreen = 0xdeadbeef;
    hr = IDirect3DViewport2_TransformVertices(viewport, 0,
            &transformdata, D3DTRANSFORM_CLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(offscreen == ~0U, "Offscreen is %x.\n", offscreen);

    /* Test how vertices are transformed during draws. */
    vp_data.dwX = 20;
    vp_data.dwY = 20;
    vp_data.dwWidth = 200;
    vp_data.dwHeight = 400;
    vp_data.dvScaleX = 20.0f;
    vp_data.dvScaleY = 50.0f;
    vp_data.dvMinZ = 0.0f;
    vp_data.dvMaxZ = 1.0f;
    hr = IDirect3DViewport3_SetViewport(viewport, &vp_data);
    ok(SUCCEEDED(hr), "Failed to set viewport, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to activate the viewport, hr %#x.\n", hr);

    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0x000000ff, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);

    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_DIFFUSE,
            quad, 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = get_surface_color(rt, 128, 143);
    ok(compare_color(color, 0x000000ff, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 132, 143);
    ok(compare_color(color, 0x000000ff, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 128, 147);
    ok(compare_color(color, 0x000000ff, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 132, 147);
    ok(compare_color(color, 0x00ff0000, 1), "Got unexpected color 0x%08x.\n", color);

    color = get_surface_color(rt, 177, 217);
    ok(compare_color(color, 0x00ff0000, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 181, 217);
    ok(compare_color(color, 0x000000ff, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 177, 221);
    ok(compare_color(color, 0x000000ff, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 181, 221);
    ok(compare_color(color, 0x000000ff, 1), "Got unexpected color 0x%08x.\n", color);

    /* Test D3DVIEWPORT2 behavior. */
    vp2_data.dwSize = sizeof(vp2_data);
    vp2_data.dwX = 20;
    vp2_data.dwY = 20;
    vp2_data.dwWidth = 200;
    vp2_data.dwHeight = 400;
    vp2_data.dvClipX = -0.5f;
    vp2_data.dvClipY = 4.0f;
    vp2_data.dvClipWidth = 5.0f;
    vp2_data.dvClipHeight = 10.0f;
    vp2_data.dvMinZ = 0.0f;
    vp2_data.dvMaxZ = 2.0f;
    hr = IDirect3DViewport3_SetViewport2(viewport, &vp2_data);
    ok(SUCCEEDED(hr), "Failed to set viewport data, hr %#x.\n", hr);
    transformdata.lpIn = position_tests;
    transformdata.lpOut = out;
    hr = IDirect3DViewport3_TransformVertices(viewport, ARRAY_SIZE(position_tests),
            &transformdata, D3DTRANSFORM_UNCLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    for (i = 0; i < ARRAY_SIZE(position_tests); ++i)
    {
        static const struct vec4 cmp[] =
        {
            {120.0f, 140.0f, 0.0f, 1.0f}, {200.0f,  60.0f,  1.0f, 1.0f}, {40.0f, 220.0f, -1.0f, 1.0f},
            {160.0f, 100.0f, 0.5f, 1.0f}, { 80.0f, 180.0f, -0.5f, 1.0f}, {80.0f, 180.0f,  0.0f, 1.0f}
        };

        ok(compare_vec4(&cmp[i], out[i].x, out[i].y, out[i].z, out[i].w, 4096),
                "Vertex %u differs. Got %f %f %f %f.\n", i,
                out[i].x, out[i].y, out[i].z, out[i].w);
    }

    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0x0000ff00, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);

    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_DIFFUSE,
            quad, 4, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = get_surface_color(rt, 58, 118);
    ok(compare_color(color, 0x0000ff00, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 62, 118);
    ok(compare_color(color, 0x0000ff00, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 58, 122);
    ok(compare_color(color, 0x0000ff00, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 62, 122);
    ok(compare_color(color, 0x00ff0000, 1), "Got unexpected color 0x%08x.\n", color);

    color = get_surface_color(rt, 157, 177);
    ok(compare_color(color, 0x00ff0000, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 161, 177);
    ok(compare_color(color, 0x0000ff00, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 157, 181);
    ok(compare_color(color, 0x0000ff00, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 161, 181);
    ok(compare_color(color, 0x0000ff00, 1), "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DDevice3_SetTransform(device, D3DTRANSFORMSTATE_WORLD, &mat_identity);
    ok(SUCCEEDED(hr), "Failed to set world transform, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTransform(device, D3DTRANSFORMSTATE_VIEW, &mat_identity);
    ok(SUCCEEDED(hr), "Failed to set world transform, hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetTransform(device, D3DTRANSFORMSTATE_PROJECTION, &mat_transform3);
    ok(SUCCEEDED(hr), "Failed to set world transform, hr %#x.\n", hr);

    vp2_data.dwX = 0.0;
    vp2_data.dwY = 0.0;
    vp2_data.dwWidth = 1;
    vp2_data.dwHeight = 1;
    vp2_data.dvClipX = -12.8f;
    vp2_data.dvClipY = 12.8f + mat_transform3._42 / mat_transform3._44;
    vp2_data.dvClipWidth = 25.6f;
    vp2_data.dvClipHeight = 25.6f;
    vp2_data.dvMinZ = 0.0f;
    vp2_data.dvMaxZ = 0.5f;
    hr = IDirect3DViewport3_SetViewport2(viewport, &vp2_data);
    ok(SUCCEEDED(hr), "Failed to set viewport data, hr %#x.\n", hr);
    transformdata.lpIn = cliptest;
    transformdata.dwInSize = sizeof(cliptest[0]);
    offscreen = 0xdeadbeef;
    hr = IDirect3DViewport3_TransformVertices(viewport, ARRAY_SIZE(cliptest),
            &transformdata, D3DTRANSFORM_CLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(!offscreen, "Offscreen is %x.\n", offscreen);
    for (i = 0; i < ARRAY_SIZE(cliptest); ++i)
    {
        static const D3DHVERTEX cmp_h[] =
        {
            {0,                                            { 25.59f}, { 44.79f}, { 1.0f }},
            {D3DCLIP_RIGHT | D3DCLIP_TOP | D3DCLIP_BACK,   { 25.61f}, { 44.81f}, { 1.01f}},
            {0,                                            {-25.59f}, {-6.39f }, { 0.0f }},
            {D3DCLIP_LEFT | D3DCLIP_BOTTOM | D3DCLIP_FRONT,{-25.61f}, {-6.41f }, {-0.01f}},
        };
        ok(compare_float(U1(cmp_h[i]).hx, U1(out_h[i]).hx, 4096)
                && compare_float(U2(cmp_h[i]).hy, U2(out_h[i]).hy, 4096)
                && compare_float(U3(cmp_h[i]).hz, U3(out_h[i]).hz, 4096)
                && cmp_h[i].dwFlags == out_h[i].dwFlags,
                "HVertex %u differs. Got %#x %f %f %f.\n", i,
                out_h[i].dwFlags, U1(out_h[i]).hx, U2(out_h[i]).hy, U3(out_h[i]).hz);
    }

    destroy_viewport(device, viewport);
    IDirectDrawSurface4_Release(rt);
    refcount = IDirect3DDevice3_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_display_mode_surface_pixel_format(void)
{
    unsigned int width, height, bpp;
    IDirectDrawSurface4 *surface;
    DDSURFACEDESC2 surface_desc;
    IDirectDraw4 *ddraw;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    if (!(ddraw = create_ddraw()))
    {
        skip("Failed to create ddraw.\n");
        return;
    }

    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDraw4_GetDisplayMode(ddraw, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get display mode, hr %#x.\n", hr);
    width = surface_desc.dwWidth;
    height = surface_desc.dwHeight;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, width, height, NULL, NULL, NULL, NULL);
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    bpp = 0;
    if (SUCCEEDED(IDirectDraw4_SetDisplayMode(ddraw, width, height, 16, 0, 0)))
        bpp = 16;
    if (SUCCEEDED(IDirectDraw4_SetDisplayMode(ddraw, width, height, 24, 0, 0)))
        bpp = 24;
    if (SUCCEEDED(IDirectDraw4_SetDisplayMode(ddraw, width, height, 32, 0, 0)))
        bpp = 32;
    ok(bpp, "Set display mode failed.\n");

    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDraw4_GetDisplayMode(ddraw, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get display mode, hr %#x.\n", hr);
    ok(surface_desc.dwWidth == width, "Got width %u, expected %u.\n", surface_desc.dwWidth, width);
    ok(surface_desc.dwHeight == height, "Got height %u, expected %u.\n", surface_desc.dwHeight, height);
    ok(U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount == bpp, "Got bpp %u, expected %u.\n",
            U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount, bpp);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    U5(surface_desc).dwBackBufferCount = 1;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_COMPLEX | DDSCAPS_FLIP | DDSCAPS_PRIMARYSURFACE;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(hr == D3D_OK, "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_GetSurfaceDesc(surface, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(surface_desc.dwWidth == width, "Got width %u, expected %u.\n", surface_desc.dwWidth, width);
    ok(surface_desc.dwHeight == height, "Got height %u, expected %u.\n", surface_desc.dwHeight, height);
    ok(U4(surface_desc).ddpfPixelFormat.dwFlags == DDPF_RGB, "Got unexpected pixel format flags %#x.\n",
            U4(surface_desc).ddpfPixelFormat.dwFlags);
    ok(U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount == bpp, "Got bpp %u, expected %u.\n",
            U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount, bpp);
    IDirectDrawSurface4_Release(surface);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    surface_desc.dwWidth = width;
    surface_desc.dwHeight = height;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(hr == D3D_OK, "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_GetSurfaceDesc(surface, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(U4(surface_desc).ddpfPixelFormat.dwFlags == DDPF_RGB, "Got unexpected pixel format flags %#x.\n",
            U4(surface_desc).ddpfPixelFormat.dwFlags);
    ok(U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount == bpp, "Got bpp %u, expected %u.\n",
            U1(U4(surface_desc).ddpfPixelFormat).dwRGBBitCount, bpp);
    IDirectDrawSurface4_Release(surface);

    refcount = IDirectDraw4_Release(ddraw);
    ok(!refcount, "DirectDraw has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_surface_desc_size(void)
{
    union
    {
        DWORD dwSize;
        DDSURFACEDESC desc1;
        DDSURFACEDESC2 desc2;
        BYTE blob[1024];
    } desc;
    IDirectDrawSurface4 *surface4;
    IDirectDrawSurface3 *surface3;
    IDirectDrawSurface *surface;
    DDSURFACEDESC2 surface_desc;
    HRESULT expected_hr, hr;
    IDirectDraw4 *ddraw;
    unsigned int i, j;
    ULONG refcount;

    static const struct
    {
        unsigned int caps;
        const char *name;
    }
    surface_caps[] =
    {
        {DDSCAPS_OFFSCREENPLAIN, "offscreenplain"},
        {DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY, "systemmemory texture"},
        {DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY, "videomemory texture"},
    };
    static const unsigned int desc_sizes[] =
    {
        sizeof(DDSURFACEDESC),
        sizeof(DDSURFACEDESC2),
        sizeof(DDSURFACEDESC) + 1,
        sizeof(DDSURFACEDESC2) + 1,
        2 * sizeof(DDSURFACEDESC),
        2 * sizeof(DDSURFACEDESC2),
        sizeof(DDSURFACEDESC) - 1,
        sizeof(DDSURFACEDESC2) - 1,
        sizeof(DDSURFACEDESC) / 2,
        sizeof(DDSURFACEDESC2) / 2,
        0,
        1,
        12,
        sizeof(desc) / 2,
        sizeof(desc) - 100,
    };

    if (!(ddraw = create_ddraw()))
    {
        skip("Failed to create ddraw.\n");
        return;
    }
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(surface_caps); ++i)
    {
        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
        surface_desc.ddsCaps.dwCaps = surface_caps[i].caps;
        surface_desc.dwHeight = 128;
        surface_desc.dwWidth = 128;
        if (FAILED(IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface4, NULL)))
        {
            skip("Failed to create surface, type %s.\n", surface_caps[i].name);
            continue;
        }
        hr = IDirectDrawSurface_QueryInterface(surface4, &IID_IDirectDrawSurface, (void **)&surface);
        ok(hr == DD_OK, "Failed to query IDirectDrawSurface, hr %#x, type %s.\n", hr, surface_caps[i].name);
        hr = IDirectDrawSurface_QueryInterface(surface4, &IID_IDirectDrawSurface3, (void **)&surface3);
        ok(hr == DD_OK, "Failed to query IDirectDrawSurface3, hr %#x, type %s.\n", hr, surface_caps[i].name);

        /* GetSurfaceDesc() */
        for (j = 0; j < ARRAY_SIZE(desc_sizes); ++j)
        {
            memset(&desc, 0, sizeof(desc));
            desc.dwSize = desc_sizes[j];
            expected_hr = desc.dwSize == sizeof(DDSURFACEDESC) ? DD_OK : DDERR_INVALIDPARAMS;
            hr = IDirectDrawSurface_GetSurfaceDesc(surface, &desc.desc1);
            ok(hr == expected_hr, "Got hr %#x, expected %#x, dwSize %u, type %s.\n",
                    hr, expected_hr, desc_sizes[j], surface_caps[i].name);

            memset(&desc, 0, sizeof(desc));
            desc.dwSize = desc_sizes[j];
            expected_hr = desc.dwSize == sizeof(DDSURFACEDESC) ? DD_OK : DDERR_INVALIDPARAMS;
            hr = IDirectDrawSurface3_GetSurfaceDesc(surface3, &desc.desc1);
            ok(hr == expected_hr, "Got hr %#x, expected %#x, dwSize %u, type %s.\n",
                    hr, expected_hr, desc_sizes[j], surface_caps[i].name);

            memset(&desc, 0, sizeof(desc));
            desc.dwSize = desc_sizes[j];
            expected_hr = desc.dwSize == sizeof(DDSURFACEDESC2) ? DD_OK : DDERR_INVALIDPARAMS;
            hr = IDirectDrawSurface4_GetSurfaceDesc(surface4, &desc.desc2);
            ok(hr == expected_hr, "Got hr %#x, expected %#x, dwSize %u, type %s.\n",
                    hr, expected_hr, desc_sizes[j], surface_caps[i].name);
        }

        /* Lock() */
        for (j = 0; j < ARRAY_SIZE(desc_sizes); ++j)
        {
            const BOOL valid_size = desc_sizes[j] == sizeof(DDSURFACEDESC)
                    || desc_sizes[j] == sizeof(DDSURFACEDESC2);
            DWORD expected_texture_stage;

            memset(&desc, 0, sizeof(desc));
            desc.dwSize = desc_sizes[j];
            desc.desc2.dwTextureStage = 0xdeadbeef;
            desc.blob[sizeof(DDSURFACEDESC2)] = 0xef;
            hr = IDirectDrawSurface_Lock(surface, NULL, &desc.desc1, 0, 0);
            expected_hr = valid_size ? DD_OK : DDERR_INVALIDPARAMS;
            ok(hr == expected_hr, "Got hr %#x, expected %#x, dwSize %u, type %s.\n",
                    hr, expected_hr, desc_sizes[j], surface_caps[i].name);
            ok(desc.dwSize == desc_sizes[j], "dwSize was changed from %u to %u, type %s.\n",
                    desc_sizes[j], desc.dwSize, surface_caps[i].name);
            ok(desc.blob[sizeof(DDSURFACEDESC2)] == 0xef, "Got unexpected byte %02x, dwSize %u, type %s.\n",
                    desc.blob[sizeof(DDSURFACEDESC2)], desc_sizes[j], surface_caps[i].name);
            if (SUCCEEDED(hr))
            {
                ok(desc.desc1.dwWidth == 128, "Got unexpected width %u, dwSize %u, type %s.\n",
                        desc.desc1.dwWidth, desc_sizes[j], surface_caps[i].name);
                ok(desc.desc1.dwHeight == 128, "Got unexpected height %u, dwSize %u, type %s.\n",
                        desc.desc1.dwHeight, desc_sizes[j], surface_caps[i].name);
                expected_texture_stage = desc_sizes[j] >= sizeof(DDSURFACEDESC2) ? 0 : 0xdeadbeef;
                todo_wine_if(!expected_texture_stage)
                ok(desc.desc2.dwTextureStage == expected_texture_stage,
                        "Got unexpected texture stage %#x, dwSize %u, type %s.\n",
                        desc.desc2.dwTextureStage, desc_sizes[j], surface_caps[i].name);
                IDirectDrawSurface_Unlock(surface, NULL);
            }

            memset(&desc, 0, sizeof(desc));
            desc.dwSize = desc_sizes[j];
            desc.desc2.dwTextureStage = 0xdeadbeef;
            desc.blob[sizeof(DDSURFACEDESC2)] = 0xef;
            hr = IDirectDrawSurface3_Lock(surface3, NULL, &desc.desc1, 0, 0);
            expected_hr = valid_size ? DD_OK : DDERR_INVALIDPARAMS;
            ok(hr == expected_hr, "Got hr %#x, expected %#x, dwSize %u, type %s.\n",
                    hr, expected_hr, desc_sizes[j], surface_caps[i].name);
            ok(desc.dwSize == desc_sizes[j], "dwSize was changed from %u to %u, type %s.\n",
                    desc_sizes[j], desc.dwSize, surface_caps[i].name);
            ok(desc.blob[sizeof(DDSURFACEDESC2)] == 0xef, "Got unexpected byte %02x, dwSize %u, type %s.\n",
                    desc.blob[sizeof(DDSURFACEDESC2)], desc_sizes[j], surface_caps[i].name);
            if (SUCCEEDED(hr))
            {
                ok(desc.desc1.dwWidth == 128, "Got unexpected width %u, dwSize %u, type %s.\n",
                        desc.desc1.dwWidth, desc_sizes[j], surface_caps[i].name);
                ok(desc.desc1.dwHeight == 128, "Got unexpected height %u, dwSize %u, type %s.\n",
                        desc.desc1.dwHeight, desc_sizes[j], surface_caps[i].name);
                expected_texture_stage = desc_sizes[j] >= sizeof(DDSURFACEDESC2) ? 0 : 0xdeadbeef;
                todo_wine_if(!expected_texture_stage)
                ok(desc.desc2.dwTextureStage == expected_texture_stage,
                        "Got unexpected texture stage %#x, dwSize %u, type %s.\n",
                        desc.desc2.dwTextureStage, desc_sizes[j], surface_caps[i].name);
                IDirectDrawSurface3_Unlock(surface3, NULL);
            }

            memset(&desc, 0, sizeof(desc));
            desc.dwSize = desc_sizes[j];
            desc.desc2.dwTextureStage = 0xdeadbeef;
            desc.blob[sizeof(DDSURFACEDESC2)] = 0xef;
            hr = IDirectDrawSurface4_Lock(surface4, NULL, &desc.desc2, 0, 0);
            expected_hr = valid_size ? DD_OK : DDERR_INVALIDPARAMS;
            ok(hr == expected_hr, "Got hr %#x, expected %#x, dwSize %u, type %s.\n",
                    hr, expected_hr, desc_sizes[j], surface_caps[i].name);
            ok(desc.dwSize == desc_sizes[j], "dwSize was changed from %u to %u, type %s.\n",
                    desc_sizes[j], desc.dwSize, surface_caps[i].name);
            ok(desc.blob[sizeof(DDSURFACEDESC2)] == 0xef, "Got unexpected byte %02x, dwSize %u, type %s.\n",
                    desc.blob[sizeof(DDSURFACEDESC2)], desc_sizes[j], surface_caps[i].name);
            if (SUCCEEDED(hr))
            {
                ok(desc.desc2.dwWidth == 128, "Got unexpected width %u, dwSize %u, type %s.\n",
                        desc.desc2.dwWidth, desc_sizes[j], surface_caps[i].name);
                ok(desc.desc2.dwHeight == 128, "Got unexpected height %u, dwSize %u, type %s.\n",
                        desc.desc2.dwHeight, desc_sizes[j], surface_caps[i].name);
                expected_texture_stage = desc_sizes[j] >= sizeof(DDSURFACEDESC2) ? 0 : 0xdeadbeef;
                ok(desc.desc2.dwTextureStage == expected_texture_stage,
                        "Got unexpected texture stage %#x, dwSize %u, type %s.\n",
                        desc.desc2.dwTextureStage, desc_sizes[j], surface_caps[i].name);
                IDirectDrawSurface4_Unlock(surface4, NULL);
            }
        }

        IDirectDrawSurface4_Release(surface4);
        IDirectDrawSurface3_Release(surface3);
        IDirectDrawSurface_Release(surface);
    }

    /* GetDisplayMode() */
    for (j = 0; j < ARRAY_SIZE(desc_sizes); ++j)
    {
        memset(&desc, 0xcc, sizeof(desc));
        desc.dwSize = desc_sizes[j];
        expected_hr = (desc.dwSize == sizeof(DDSURFACEDESC) || desc.dwSize == sizeof(DDSURFACEDESC2))
                ? DD_OK : DDERR_INVALIDPARAMS;
        hr = IDirectDraw4_GetDisplayMode(ddraw, &desc.desc2);
        ok(hr == expected_hr, "Got hr %#x, expected %#x, size %u.\n", hr, expected_hr, desc_sizes[j]);
        if (SUCCEEDED(hr))
        {
            ok(desc.dwSize == sizeof(DDSURFACEDESC2), "Wrong size %u for %u.\n", desc.dwSize, desc_sizes[j]);
            ok(desc.blob[desc_sizes[j]] == 0xcc, "Overflow for size %u.\n", desc_sizes[j]);
            ok(desc.blob[desc_sizes[j] - 1] != 0xcc, "Struct not cleared for size %u.\n", desc_sizes[j]);
        }
    }

    refcount = IDirectDraw4_Release(ddraw);
    ok(!refcount, "DirectDraw has %u references left.\n", refcount);
}

static void test_get_surface_from_dc(void)
{
    IDirectDrawSurface *surface1, *tmp;
    IDirectDrawSurface4 *surface;
    DDSURFACEDESC2 surface_desc;
    IDirectDraw4 *ddraw;
    HDC dc, device_dc;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    DWORD ret;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.dwWidth = 64;
    surface_desc.dwHeight = 64;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_QueryInterface(surface, &IID_IDirectDrawSurface, (void **)&surface1);
    ok(SUCCEEDED(hr), "Failed to query IDirectDrawSurface interface, hr %#x.\n", hr);

    refcount = get_refcount((IUnknown *)surface1);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);
    refcount = get_refcount((IUnknown *)surface);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);

    hr = IDirectDrawSurface4_GetDC(surface, &dc);
    ok(SUCCEEDED(hr), "Failed to get DC, hr %#x.\n", hr);

    tmp = (void *)0xdeadbeef;
    device_dc = (void *)0xdeadbeef;
    hr = GetSurfaceFromDC(NULL, &tmp, &device_dc);
    ok(hr == DDERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);
    ok(!tmp, "Got unexpected surface %p.\n", tmp);
    ok(!device_dc, "Got unexpected device_dc %p.\n", device_dc);

    device_dc = (void *)0xdeadbeef;
    hr = GetSurfaceFromDC(dc, NULL, &device_dc);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    ok(device_dc == (void *)0xdeadbeef, "Got unexpected device_dc %p.\n", device_dc);

    tmp = (void *)0xdeadbeef;
    hr = GetSurfaceFromDC(dc, &tmp, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    ok(!tmp, "Got unexpected surface %p.\n", tmp);

    hr = GetSurfaceFromDC(dc, &tmp, &device_dc);
    ok(SUCCEEDED(hr), "GetSurfaceFromDC failed, hr %#x.\n", hr);
    ok(tmp == surface1, "Got unexpected surface %p, expected %p.\n", tmp, surface1);
    IDirectDrawSurface_Release(tmp);

    ret = GetObjectType(device_dc);
    todo_wine ok(ret == OBJ_DC, "Got unexpected object type %#x.\n", ret);
    ret = GetDeviceCaps(device_dc, TECHNOLOGY);
    todo_wine ok(ret == DT_RASDISPLAY, "Got unexpected technology %#x.\n", ret);

    hr = IDirectDraw4_GetSurfaceFromDC(ddraw, dc, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);

    hr = IDirectDraw4_GetSurfaceFromDC(ddraw, dc, (IDirectDrawSurface4 **)&tmp);
    ok(SUCCEEDED(hr), "GetSurfaceFromDC failed, hr %#x.\n", hr);
    ok(tmp == surface1, "Got unexpected surface %p, expected %p.\n", tmp, surface1);

    refcount = get_refcount((IUnknown *)surface1);
    ok(refcount == 2, "Got unexpected refcount %u.\n", refcount);
    refcount = get_refcount((IUnknown *)surface);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);

    hr = IDirectDrawSurface4_ReleaseDC(surface, dc);
    ok(SUCCEEDED(hr), "ReleaseDC failed, hr %#x.\n", hr);

    IDirectDrawSurface_Release(tmp);

    dc = CreateCompatibleDC(NULL);
    ok(!!dc, "CreateCompatibleDC failed.\n");

    tmp = (void *)0xdeadbeef;
    device_dc = (void *)0xdeadbeef;
    hr = GetSurfaceFromDC(dc, &tmp, &device_dc);
    ok(hr == DDERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);
    ok(!tmp, "Got unexpected surface %p.\n", tmp);
    ok(!device_dc, "Got unexpected device_dc %p.\n", device_dc);

    tmp = (void *)0xdeadbeef;
    hr = IDirectDraw4_GetSurfaceFromDC(ddraw, dc, (IDirectDrawSurface4 **)&tmp);
    ok(hr == DDERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);
    ok(!tmp, "Got unexpected surface %p.\n", tmp);

    ok(DeleteDC(dc), "DeleteDC failed.\n");

    tmp = (void *)0xdeadbeef;
    hr = IDirectDraw4_GetSurfaceFromDC(ddraw, NULL, (IDirectDrawSurface4 **)&tmp);
    ok(hr == DDERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);
    ok(!tmp, "Got unexpected surface %p.\n", tmp);

    IDirectDrawSurface4_Release(surface);
    IDirectDrawSurface_Release(surface1);
    IDirectDraw4_Release(ddraw);
    DestroyWindow(window);
}

static void test_ck_operation(void)
{
    IDirectDrawSurface4 *src, *dst;
    IDirectDrawSurface *src1, *dst1;
    DDSURFACEDESC2 surface_desc;
    IDirectDraw4 *ddraw;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    D3DCOLOR *color;
    unsigned int i;
    DDCOLORKEY ckey;
    DDBLTFX fx;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw4_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    surface_desc.dwWidth = 4;
    surface_desc.dwHeight = 1;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(U4(surface_desc.ddpfPixelFormat)).dwRGBBitCount = 32;
    U2(U4(surface_desc.ddpfPixelFormat)).dwRBitMask = 0x00ff0000;
    U3(U4(surface_desc.ddpfPixelFormat)).dwGBitMask = 0x0000ff00;
    U4(U4(surface_desc.ddpfPixelFormat)).dwBBitMask = 0x000000ff;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &dst, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    surface_desc.dwFlags |= DDSD_CKSRCBLT;
    surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue = 0x00ff00ff;
    surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue = 0x00ff00ff;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &src, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_Lock(src, NULL, &surface_desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    ok(!(surface_desc.dwFlags & DDSD_LPSURFACE), "Surface desc has LPSURFACE Flags set.\n");
    color = surface_desc.lpSurface;
    color[0] = 0x77010203;
    color[1] = 0x00010203;
    color[2] = 0x77ff00ff;
    color[3] = 0x00ff00ff;
    hr = IDirectDrawSurface4_Unlock(src, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    for (i = 0; i < 2; ++i)
    {
        hr = IDirectDrawSurface4_Lock(dst, NULL, &surface_desc, DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
        color = surface_desc.lpSurface;
        color[0] = 0xcccccccc;
        color[1] = 0xcccccccc;
        color[2] = 0xcccccccc;
        color[3] = 0xcccccccc;
        hr = IDirectDrawSurface4_Unlock(dst, NULL);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

        if (i)
        {
            hr = IDirectDrawSurface4_BltFast(dst, 0, 0, src, NULL, DDBLTFAST_SRCCOLORKEY);
            ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);
        }
        else
        {
            hr = IDirectDrawSurface4_Blt(dst, NULL, src, NULL, DDBLT_KEYSRC, NULL);
            ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);
        }

        hr = IDirectDrawSurface4_Lock(dst, NULL, &surface_desc, DDLOCK_WAIT | DDLOCK_READONLY, NULL);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
        ok(!(surface_desc.dwFlags & DDSD_LPSURFACE), "Surface desc has LPSURFACE Flags set.\n");
        color = surface_desc.lpSurface;
        /* Different behavior on some drivers / windows versions. Some versions ignore the X channel when
         * color keying, but copy it to the destination surface. Others (sysmem surfaces) apply it for
         * color keying, but do not copy it into the destination surface. Nvidia neither uses it for
         * color keying nor copies it. */
        ok((color[0] == 0x77010203 && color[1] == 0x00010203
                && color[2] == 0xcccccccc && color[3] == 0xcccccccc) /* AMD, Wine */
                || broken(color[0] == 0x00010203 && color[1] == 0x00010203
                && color[2] == 0x00ff00ff && color[3] == 0xcccccccc) /* Sysmem surfaces? */
                || broken(color[0] == 0x00010203 && color[1] == 0x00010203
                && color[2] == 0xcccccccc && color[3] == 0xcccccccc) /* Nvidia */
                || broken(color[0] == 0xff010203 && color[1] == 0xff010203
                && color[2] == 0xcccccccc && color[3] == 0xcccccccc) /* Testbot */,
                "Destination data after blitting is %08x %08x %08x %08x, i=%u.\n",
                color[0], color[1], color[2], color[3], i);
        hr = IDirectDrawSurface4_Unlock(dst, NULL);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
    }

    hr = IDirectDrawSurface4_GetColorKey(src, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to get color key, hr %#x.\n", hr);
    ok(ckey.dwColorSpaceLowValue == 0x00ff00ff && ckey.dwColorSpaceHighValue == 0x00ff00ff,
            "Got unexpected color key low=%08x high=%08x.\n", ckey.dwColorSpaceLowValue, ckey.dwColorSpaceHighValue);

    ckey.dwColorSpaceLowValue = ckey.dwColorSpaceHighValue = 0x0000ff00;
    hr = IDirectDrawSurface4_SetColorKey(src, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);

    ckey.dwColorSpaceLowValue = ckey.dwColorSpaceHighValue = 0;
    hr = IDirectDrawSurface4_GetColorKey(src, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to get color key, hr %#x.\n", hr);
    ok(ckey.dwColorSpaceLowValue == 0x0000ff00 && ckey.dwColorSpaceHighValue == 0x0000ff00,
            "Got unexpected color key low=%08x high=%08x.\n", ckey.dwColorSpaceLowValue, ckey.dwColorSpaceHighValue);

    surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue = 0;
    surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue = 0;
    hr = IDirectDrawSurface4_GetSurfaceDesc(src, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue == 0x0000ff00
            && surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue == 0x0000ff00,
            "Got unexpected color key low=%08x high=%08x.\n", surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue,
            surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue);

    /* Test SetColorKey with dwColorSpaceHighValue < dwColorSpaceLowValue */
    ckey.dwColorSpaceLowValue = 0x000000ff;
    ckey.dwColorSpaceHighValue = 0x00000000;
    hr = IDirectDrawSurface4_SetColorKey(src, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);

    ckey.dwColorSpaceLowValue = ckey.dwColorSpaceHighValue = 0;
    hr = IDirectDrawSurface4_GetColorKey(src, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to get color key, hr %#x.\n", hr);
    ok(ckey.dwColorSpaceLowValue == 0x000000ff && ckey.dwColorSpaceHighValue == 0x000000ff,
            "Got unexpected color key low=%08x high=%08x.\n", ckey.dwColorSpaceLowValue, ckey.dwColorSpaceHighValue);

    ckey.dwColorSpaceLowValue = 0x000000ff;
    ckey.dwColorSpaceHighValue = 0x00000001;
    hr = IDirectDrawSurface4_SetColorKey(src, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);

    ckey.dwColorSpaceLowValue = ckey.dwColorSpaceHighValue = 0;
    hr = IDirectDrawSurface4_GetColorKey(src, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to get color key, hr %#x.\n", hr);
    ok(ckey.dwColorSpaceLowValue == 0x000000ff && ckey.dwColorSpaceHighValue == 0x000000ff,
            "Got unexpected color key low=%08x high=%08x.\n", ckey.dwColorSpaceLowValue, ckey.dwColorSpaceHighValue);

    ckey.dwColorSpaceLowValue = 0x000000fe;
    ckey.dwColorSpaceHighValue = 0x000000fd;
    hr = IDirectDrawSurface4_SetColorKey(src, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);

    ckey.dwColorSpaceLowValue = ckey.dwColorSpaceHighValue = 0;
    hr = IDirectDrawSurface4_GetColorKey(src, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to get color key, hr %#x.\n", hr);
    ok(ckey.dwColorSpaceLowValue == 0x000000fe && ckey.dwColorSpaceHighValue == 0x000000fe,
            "Got unexpected color key low=%08x high=%08x.\n", ckey.dwColorSpaceLowValue, ckey.dwColorSpaceHighValue);

    IDirectDrawSurface4_Release(src);
    IDirectDrawSurface4_Release(dst);

    /* Test source and destination keys and where they are read from. Use a surface with alpha
     * to avoid driver-dependent content in the X channel. */
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    surface_desc.dwWidth = 6;
    surface_desc.dwHeight = 1;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    U1(U4(surface_desc.ddpfPixelFormat)).dwRGBBitCount = 32;
    U2(U4(surface_desc.ddpfPixelFormat)).dwRBitMask = 0x00ff0000;
    U3(U4(surface_desc.ddpfPixelFormat)).dwGBitMask = 0x0000ff00;
    U4(U4(surface_desc.ddpfPixelFormat)).dwBBitMask = 0x000000ff;
    U5(U4(surface_desc.ddpfPixelFormat)).dwRGBAlphaBitMask = 0xff000000;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &dst, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &src, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    ckey.dwColorSpaceLowValue = 0x0000ff00;
    ckey.dwColorSpaceHighValue = 0x0000ff00;
    hr = IDirectDrawSurface4_SetColorKey(dst, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);
    ckey.dwColorSpaceLowValue = 0x00ff0000;
    ckey.dwColorSpaceHighValue = 0x00ff0000;
    hr = IDirectDrawSurface4_SetColorKey(dst, DDCKEY_DESTBLT, &ckey);
    ok(SUCCEEDED(hr) || hr == DDERR_NOCOLORKEYHW, "Failed to set color key, hr %#x.\n", hr);
    if (FAILED(hr))
    {
        /* Nvidia reject dest keys, AMD allows them. This applies to vidmem and sysmem surfaces. */
        skip("Failed to set destination color key, skipping related tests.\n");
        goto done;
    }

    ckey.dwColorSpaceLowValue = 0x000000ff;
    ckey.dwColorSpaceHighValue = 0x000000ff;
    hr = IDirectDrawSurface4_SetColorKey(src, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);
    ckey.dwColorSpaceLowValue = 0x000000aa;
    ckey.dwColorSpaceHighValue = 0x000000aa;
    hr = IDirectDrawSurface4_SetColorKey(src, DDCKEY_DESTBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);

    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    fx.ddckSrcColorkey.dwColorSpaceHighValue = 0x00110000;
    fx.ddckSrcColorkey.dwColorSpaceLowValue = 0x00110000;
    fx.ddckDestColorkey.dwColorSpaceHighValue = 0x00001100;
    fx.ddckDestColorkey.dwColorSpaceLowValue = 0x00001100;

    hr = IDirectDrawSurface4_Lock(src, NULL, &surface_desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    color = surface_desc.lpSurface;
    color[0] = 0x000000ff; /* Applies to src blt key in src surface. */
    color[1] = 0x000000aa; /* Applies to dst blt key in src surface. */
    color[2] = 0x00ff0000; /* Dst color key in dst surface. */
    color[3] = 0x0000ff00; /* Src color key in dst surface. */
    color[4] = 0x00001100; /* Src color key in ddbltfx. */
    color[5] = 0x00110000; /* Dst color key in ddbltfx. */
    hr = IDirectDrawSurface4_Unlock(src, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_Lock(dst, NULL, &surface_desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    color = surface_desc.lpSurface;
    color[0] = color[1] = color[2] = color[3] = color[4] = color[5] = 0x55555555;
    hr = IDirectDrawSurface4_Unlock(dst, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    /* Test a blit without keying. */
    hr = IDirectDrawSurface4_Blt(dst, NULL, src, NULL, 0, &fx);
    ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_Lock(dst, NULL, &surface_desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    color = surface_desc.lpSurface;
    /* Should have copied src data unmodified to dst. */
    ok(color[0] == 0x000000ff && color[1] == 0x000000aa && color[2] == 0x00ff0000 &&
            color[3] == 0x0000ff00 && color[4] == 0x00001100 && color[5] == 0x00110000,
            "Got unexpected content %08x %08x %08x %08x %08x %08x.\n",
            color[0], color[1], color[2], color[3], color[4], color[5]);

    color[0] = color[1] = color[2] = color[3] = color[4] = color[5] = 0x55555555;
    hr = IDirectDrawSurface4_Unlock(dst, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    /* Src key. */
    hr = IDirectDrawSurface4_Blt(dst, NULL, src, NULL, DDBLT_KEYSRC, &fx);
    ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_Lock(dst, NULL, &surface_desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    color = surface_desc.lpSurface;
    /* Src key applied to color[0]. It is unmodified, the others are copied. */
    ok(color[0] == 0x55555555 && color[1] == 0x000000aa && color[2] == 0x00ff0000 &&
            color[3] == 0x0000ff00 && color[4] == 0x00001100 && color[5] == 0x00110000,
            "Got unexpected content %08x %08x %08x %08x %08x %08x.\n",
            color[0], color[1], color[2], color[3], color[4], color[5]);

    color[0] = color[1] = color[2] = color[3] = color[4] = color[5] = 0x55555555;
    hr = IDirectDrawSurface4_Unlock(dst, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    /* Src override. */
    hr = IDirectDrawSurface4_Blt(dst, NULL, src, NULL, DDBLT_KEYSRCOVERRIDE, &fx);
    ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_Lock(dst, NULL, &surface_desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    color = surface_desc.lpSurface;
    /* Override key applied to color[5]. It is unmodified, the others are copied. */
    ok(color[0] == 0x000000ff && color[1] == 0x000000aa && color[2] == 0x00ff0000 &&
            color[3] == 0x0000ff00 && color[4] == 0x00001100 && color[5] == 0x55555555,
            "Got unexpected content %08x %08x %08x %08x %08x %08x.\n",
            color[0], color[1], color[2], color[3], color[4], color[5]);

    color[0] = color[1] = color[2] = color[3] = color[4] = color[5] = 0x55555555;
    hr = IDirectDrawSurface4_Unlock(dst, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    /* Src override AND src key. That is not supposed to work. */
    hr = IDirectDrawSurface4_Blt(dst, NULL, src, NULL, DDBLT_KEYSRC | DDBLT_KEYSRCOVERRIDE, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    hr = IDirectDrawSurface4_Lock(dst, NULL, &surface_desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    color = surface_desc.lpSurface;
    /* Ensure the destination was not changed. */
    ok(color[0] == 0x55555555 && color[1] == 0x55555555 && color[2] == 0x55555555 &&
            color[3] == 0x55555555 && color[4] == 0x55555555 && color[5] == 0x55555555,
            "Got unexpected content %08x %08x %08x %08x %08x %08x.\n",
            color[0], color[1], color[2], color[3], color[4], color[5]);

    /* Use different dst colors for the dst key test. */
    color[0] = 0x00ff0000; /* Dest key in dst surface. */
    color[1] = 0x00ff0000; /* Dest key in dst surface. */
    color[2] = 0x00001100; /* Dest key in override. */
    color[3] = 0x00001100; /* Dest key in override. */
    color[4] = 0x000000aa; /* Dest key in src surface. */
    color[5] = 0x000000aa; /* Dest key in src surface. */
    hr = IDirectDrawSurface4_Unlock(dst, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    /* Dest key blit. The key is taken from the DESTINATION surface in v4! */
    hr = IDirectDrawSurface4_Blt(dst, NULL, src, NULL, DDBLT_KEYDEST, &fx);
    ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_Lock(dst, NULL, &surface_desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    color = surface_desc.lpSurface;
    /* Dst key applied to color[0,1], they are the only changed pixels. */
    todo_wine ok(color[0] == 0x000000ff && color[1] == 0x000000aa && color[2] == 0x00001100 &&
            color[3] == 0x00001100 && color[4] == 0x000000aa && color[5] == 0x000000aa,
            "Got unexpected content %08x %08x %08x %08x %08x %08x.\n",
            color[0], color[1], color[2], color[3], color[4], color[5]);

    color[0] = 0x00ff0000; /* Dest key in dst surface. */
    color[1] = 0x00ff0000; /* Dest key in dst surface. */
    color[2] = 0x00001100; /* Dest key in override. */
    color[3] = 0x00001100; /* Dest key in override. */
    color[4] = 0x000000aa; /* Dest key in src surface. */
    color[5] = 0x000000aa; /* Dest key in src surface. */
    hr = IDirectDrawSurface4_Unlock(dst, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    /* What happens with a QI'd older version of the interface? It takes the key
     * from the source surface. */
    hr = IDirectDrawSurface4_QueryInterface(src, &IID_IDirectDrawSurface, (void **)&src1);
    ok(SUCCEEDED(hr), "Failed to query IDirectDrawSurface interface, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_QueryInterface(dst, &IID_IDirectDrawSurface, (void **)&dst1);
    ok(SUCCEEDED(hr), "Failed to query IDirectDrawSurface interface, hr %#x.\n", hr);

    hr = IDirectDrawSurface_Blt(dst1, NULL, src1, NULL, DDBLT_KEYDEST, &fx);
    ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);

    IDirectDrawSurface_Release(dst1);
    IDirectDrawSurface_Release(src1);

    hr = IDirectDrawSurface7_Lock(dst, NULL, &surface_desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    color = surface_desc.lpSurface;
    /* Dst key applied to color[4,5], they are the only changed pixels. */
    ok(color[0] == 0x00ff0000 && color[1] == 0x00ff0000 && color[2] == 0x00001100 &&
            color[3] == 0x00001100 && color[4] == 0x00001100 && color[5] == 0x00110000,
            "Got unexpected content %08x %08x %08x %08x %08x %08x.\n",
            color[0], color[1], color[2], color[3], color[4], color[5]);

    color[0] = 0x00ff0000; /* Dest key in dst surface. */
    color[1] = 0x00ff0000; /* Dest key in dst surface. */
    color[2] = 0x00001100; /* Dest key in override. */
    color[3] = 0x00001100; /* Dest key in override. */
    color[4] = 0x000000aa; /* Dest key in src surface. */
    color[5] = 0x000000aa; /* Dest key in src surface. */
    hr = IDirectDrawSurface7_Unlock(dst, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    /* Dest override key blit. */
    hr = IDirectDrawSurface4_Blt(dst, NULL, src, NULL, DDBLT_KEYDESTOVERRIDE, &fx);
    ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_Lock(dst, NULL, &surface_desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    color = surface_desc.lpSurface;
    /* Dst key applied to color[2,3], they are the only changed pixels. */
    ok(color[0] == 0x00ff0000 && color[1] == 0x00ff0000 && color[2] == 0x00ff0000 &&
            color[3] == 0x0000ff00 && color[4] == 0x000000aa && color[5] == 0x000000aa,
            "Got unexpected content %08x %08x %08x %08x %08x %08x.\n",
            color[0], color[1], color[2], color[3], color[4], color[5]);

    color[0] = 0x00ff0000; /* Dest key in dst surface. */
    color[1] = 0x00ff0000; /* Dest key in dst surface. */
    color[2] = 0x00001100; /* Dest key in override. */
    color[3] = 0x00001100; /* Dest key in override. */
    color[4] = 0x000000aa; /* Dest key in src surface. */
    color[5] = 0x000000aa; /* Dest key in src surface. */
    hr = IDirectDrawSurface4_Unlock(dst, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    /* Dest override together with surface key. Supposed to fail. */
    hr = IDirectDrawSurface4_Blt(dst, NULL, src, NULL, DDBLT_KEYDEST | DDBLT_KEYDESTOVERRIDE, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    hr = IDirectDrawSurface4_Lock(dst, NULL, &surface_desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    color = surface_desc.lpSurface;
    /* Destination is unchanged. */
    ok(color[0] == 0x00ff0000 && color[1] == 0x00ff0000 && color[2] == 0x00001100 &&
            color[3] == 0x00001100 && color[4] == 0x000000aa && color[5] == 0x000000aa,
            "Got unexpected content %08x %08x %08x %08x %08x %08x.\n",
            color[0], color[1], color[2], color[3], color[4], color[5]);
    hr = IDirectDrawSurface4_Unlock(dst, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    /* Source and destination key. This is driver dependent. New HW treats it like
     * DDBLT_KEYSRC. Older HW and some software renderers apply both keys. */
    if (0)
    {
        hr = IDirectDrawSurface4_Blt(dst, NULL, src, NULL, DDBLT_KEYDEST | DDBLT_KEYSRC, &fx);
        ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);

        hr = IDirectDrawSurface4_Lock(dst, NULL, &surface_desc, DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
        color = surface_desc.lpSurface;
        /* Color[0] is filtered by the src key, 2-5 are filtered by the dst key, if
         * the driver applies it. */
        ok(color[0] == 0x00ff0000 && color[1] == 0x000000aa && color[2] == 0x00ff0000 &&
                color[3] == 0x0000ff00 && color[4] == 0x00001100 && color[5] == 0x00110000,
                "Got unexpected content %08x %08x %08x %08x %08x %08x.\n",
                color[0], color[1], color[2], color[3], color[4], color[5]);

        color[0] = 0x00ff0000; /* Dest key in dst surface. */
        color[1] = 0x00ff0000; /* Dest key in dst surface. */
        color[2] = 0x00001100; /* Dest key in override. */
        color[3] = 0x00001100; /* Dest key in override. */
        color[4] = 0x000000aa; /* Dest key in src surface. */
        color[5] = 0x000000aa; /* Dest key in src surface. */
        hr = IDirectDrawSurface4_Unlock(dst, NULL);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
    }

    /* Override keys without ddbltfx parameter fail */
    hr = IDirectDrawSurface4_Blt(dst, NULL, src, NULL, DDBLT_KEYDESTOVERRIDE, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface4_Blt(dst, NULL, src, NULL, DDBLT_KEYSRCOVERRIDE, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    /* Try blitting without keys in the source surface. */
    hr = IDirectDrawSurface4_SetColorKey(src, DDCKEY_SRCBLT, NULL);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_SetColorKey(src, DDCKEY_DESTBLT, NULL);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);

    /* That fails now. Do not bother to check that the data is unmodified. */
    hr = IDirectDrawSurface4_Blt(dst, NULL, src, NULL, DDBLT_KEYSRC, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    /* Dest key blit still works, the destination surface key is used in v4. */
    hr = IDirectDrawSurface4_Blt(dst, NULL, src, NULL, DDBLT_KEYDEST, &fx);
    ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_Lock(dst, NULL, &surface_desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    color = surface_desc.lpSurface;
    /* Dst key applied to color[0,1], they are the only changed pixels. */
    todo_wine ok(color[0] == 0x000000ff && color[1] == 0x000000aa && color[2] == 0x00001100 &&
            color[3] == 0x00001100 && color[4] == 0x000000aa && color[5] == 0x000000aa,
            "Got unexpected content %08x %08x %08x %08x %08x %08x.\n",
            color[0], color[1], color[2], color[3], color[4], color[5]);
    hr = IDirectDrawSurface4_Unlock(dst, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    /* Try blitting without keys in the destination surface. */
    hr = IDirectDrawSurface4_SetColorKey(dst, DDCKEY_SRCBLT, NULL);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_SetColorKey(dst, DDCKEY_DESTBLT, NULL);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);

    /* This fails, as sanity would dictate. */
    hr = IDirectDrawSurface4_Blt(dst, NULL, src, NULL, DDBLT_KEYDEST, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

done:
    IDirectDrawSurface4_Release(src);
    IDirectDrawSurface4_Release(dst);
    refcount = IDirectDraw4_Release(ddraw);
    ok(!refcount, "DirectDraw has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_vb_refcount(void)
{
    ULONG prev_d3d_refcount, prev_device_refcount;
    ULONG cur_d3d_refcount, cur_device_refcount;
    IDirect3DVertexBuffer *vb, *vb1;
    IDirect3DVertexBuffer7 *vb7;
    D3DVERTEXBUFFERDESC vb_desc;
    IDirect3DDevice3 *device;
    IDirect3D3 *d3d;
    ULONG refcount;
    IUnknown *unk;
    HWND window;
    HRESULT hr;

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get Direct3D3 interface, hr %#x.\n", hr);

    prev_d3d_refcount = get_refcount((IUnknown *)d3d);
    prev_device_refcount = get_refcount((IUnknown *)device);

    memset(&vb_desc, 0, sizeof(vb_desc));
    vb_desc.dwSize = sizeof(vb_desc);
    vb_desc.dwFVF = D3DFVF_XYZ;
    vb_desc.dwNumVertices = 4;
    hr = IDirect3D3_CreateVertexBuffer(d3d, &vb_desc, &vb, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to create vertex buffer, hr %#x.\n", hr);

    cur_d3d_refcount = get_refcount((IUnknown *)d3d);
    cur_device_refcount = get_refcount((IUnknown *)device);
    ok(cur_d3d_refcount == prev_d3d_refcount, "D3D object refcount changed from %u to %u.\n",
            prev_d3d_refcount, cur_d3d_refcount);
    ok(cur_device_refcount == prev_device_refcount, "Device refcount changed from %u to %u.\n",
            prev_device_refcount, cur_device_refcount);

    hr = IDirect3DVertexBuffer_QueryInterface(vb, &IID_IDirect3DVertexBuffer, (void **)&vb1);
    ok(hr == DD_OK, "Failed to query IDirect3DVertexBuffer, hr %#x.\n", hr);
    IDirect3DVertexBuffer_Release(vb1);

    hr = IDirect3DVertexBuffer_QueryInterface(vb, &IID_IDirect3DVertexBuffer7, (void **)&vb7);
    ok(hr == E_NOINTERFACE, "Querying IDirect3DVertexBuffer7 returned unexpected hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer_QueryInterface(vb, &IID_IUnknown, (void **)&unk);
    ok(hr == DD_OK, "Failed to query IUnknown, hr %#x.\n", hr);
    ok((IUnknown *)vb == unk,
            "IDirect3DVertexBuffer and IUnknown interface pointers don't match, %p != %p.\n", vb, unk);
    IUnknown_Release(unk);

    refcount = IDirect3DVertexBuffer_Release(vb);
    ok(!refcount, "Vertex buffer has %u references left.\n", refcount);
    IDirect3D3_Release(d3d);
    refcount = IDirect3DDevice3_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_compute_sphere_visibility(void)
{
    static D3DMATRIX proj_1 =
    {
        1.810660f, 0.000000f,  0.000000f, 0.000000f,
        0.000000f, 2.414213f,  0.000000f, 0.000000f,
        0.000000f, 0.000000f,  1.020408f, 1.000000f,
        0.000000f, 0.000000f, -0.102041f, 0.000000f,
    };
    static D3DMATRIX proj_2 =
    {
        10.0f,  0.0f,  0.0f, 0.0f,
         0.0f, 10.0f,  0.0f, 0.0f,
         0.0f,  0.0f, 10.0f, 0.0f,
         0.0f,  0.0f,  0.0f, 1.0f,
    };
    static D3DMATRIX view_1 =
    {
          1.000000f, 0.000000f,  0.000000f, 0.000000f,
          0.000000f, 0.768221f, -0.640185f, 0.000000f,
         -0.000000f, 0.640185f,  0.768221f, 0.000000f,
        -14.852037f, 9.857489f, 11.600972f, 1.000000f,
    };
    static D3DMATRIX identity =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    static struct
    {
        D3DMATRIX *view, *proj;
        unsigned int sphere_count;
        D3DVECTOR center[3];
        D3DVALUE radius[3];
        const DWORD expected[3];
        BOOL todo;
    }
    tests[] =
    {
        {&view_1, &proj_1, 1, {{{11.461533f}, {-4.761727f}, {-1.171646f}}}, {38.252632f}, {0x1555}},
        {&view_1, &proj_1, 3, {{{-3.515620f}, {-1.560661f}, {-12.464638f}},
                 {{14.290396f}, {-2.981143f}, {-24.311312f}},
                 {{1.461626f}, {-6.093709f}, {-13.901010f}}},
                 {4.354097f, 12.500704f, 17.251318f}, {0x154a, 0x1555, 0x1555}},
        {&identity, &proj_2, 1, {{{0.0f}, {0.0f}, {0.05f}}}, {0.04f}, {0x1555}, TRUE},
        {&identity, &identity, 1, {{{0.0f}, {0.0f}, {0.5f}}}, {0.5f}, {0x1401}},
        {&identity, &identity, 1, {{{0.0f}, {0.0f}, {0.0f}}}, {0.0f}, {0x401}},
        {&identity, &identity, 1, {{{-1.0f}, {-1.0f}, {0.5f}}}, {0.25f}, {0x1505}, TRUE}, /* 5 */
        {&identity, &identity, 1, {{{-20.0f}, {0.0f}, {0.5f}}}, {3.0f}, {0x154a}},
        {&identity, &identity, 1, {{{20.0f}, {0.0f}, {0.5f}}}, {3.0f}, {0x1562}},
        {&identity, &identity, 1, {{{0.0f}, {-20.0f}, {0.5f}}}, {3.0f}, {0x1616}},
        {&identity, &identity, 1, {{{0.0f}, {20.0f}, {0.5f}}}, {3.0f}, {0x1496}},
        {&identity, &identity, 1, {{{0.0f}, {0.0f}, {-20.0f}}}, {3.0f}, {0x956}}, /* 10 */
        {&identity, &identity, 1, {{{0.0f}, {0.0f}, {20.0f}}}, {3.0f}, {0x2156}},
    };
    IDirect3DViewport3 *viewport;
    IDirect3DDevice3 *device;
    unsigned int i, j;
    DWORD result[3];
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    viewport = create_viewport(device, 0, 0, 640, 480);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);

    IDirect3DDevice3_SetTransform(device, D3DTRANSFORMSTATE_WORLD, &identity);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        IDirect3DDevice3_SetTransform(device, D3DTRANSFORMSTATE_VIEW, tests[i].view);
        IDirect3DDevice3_SetTransform(device, D3DTRANSFORMSTATE_PROJECTION, tests[i].proj);

        hr = IDirect3DDevice3_ComputeSphereVisibility(device, tests[i].center, tests[i].radius,
                tests[i].sphere_count, 0, result);
        ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

        for (j = 0; j < tests[i].sphere_count; ++j)
            todo_wine_if(tests[i].todo)
                ok(result[j] == tests[i].expected[j], "Test %u sphere %u: expected %#x, got %#x.\n",
                        i, j, tests[i].expected[j], result[j]);
    }

    destroy_viewport(device, viewport);
    refcount = IDirect3DDevice3_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_texture_stages_limits(void)
{
    IDirectDrawSurface4 *surface;
    DDSURFACEDESC2 surface_desc;
    IDirect3DTexture2 *texture;
    IDirect3DDevice3 *device;
    IDirectDraw4 *ddraw;
    IDirect3D3 *d3d;
    unsigned int i;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create 3D device.\n");
        DestroyWindow(window);
        return;
    }
    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get Direct3D interface, hr %#x.\n", hr);
    hr = IDirect3D3_QueryInterface(d3d, &IID_IDirectDraw4, (void **)&ddraw);
    ok(SUCCEEDED(hr), "Failed to get DirectDraw interface, hr %#x.\n", hr);
    IDirect3D3_Release(d3d);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    surface_desc.dwWidth = 16;
    surface_desc.dwHeight = 16;
    hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(hr == DD_OK, "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_QueryInterface(surface, &IID_IDirect3DTexture2, (void **)&texture);
    ok(SUCCEEDED(hr), "Failed to get texture interface, hr %#x.\n", hr);
    IDirectDrawSurface4_Release(surface);

    for (i = 0; i < 8; ++i)
    {
        hr = IDirect3DDevice3_SetTexture(device, i, texture);
        ok(hr == D3D_OK, "Failed to set texture %u, hr %#x.\n", i, hr);
        hr = IDirect3DDevice3_SetTexture(device, i, NULL);
        ok(hr == D3D_OK, "Failed to set texture %u, hr %#x.\n", i, hr);
        hr = IDirect3DDevice3_SetTextureStageState(device, i, D3DTSS_COLOROP, D3DTOP_ADD);
        ok(hr == D3D_OK, "Failed to set texture stage state %u, hr %#x.\n", i, hr);
    }

    IDirectDraw4_Release(ddraw);
    IDirect3DTexture2_Release(texture);
    refcount = IDirect3DDevice3_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_set_render_state(void)
{
    IDirect3DDevice3 *device;
    ULONG refcount;
    HWND window;
    DWORD state;
    HRESULT hr;

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create 3D device.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_ZVISIBLE, TRUE);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_ZVISIBLE, FALSE);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_TEXTUREHANDLE, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    state = 0xdeadbeef;
    hr = IDirect3DDevice3_GetRenderState(device, D3DRENDERSTATE_TEXTUREHANDLE, &state);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(!state, "Got unexpected render state %#x.\n", state);
    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_TEXTUREMAPBLEND, D3DTBLEND_MODULATE);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice3_GetRenderState(device, D3DRENDERSTATE_TEXTUREMAPBLEND, &state);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(state == D3DTBLEND_MODULATE, "Got unexpected render state %#x.\n", state);

    refcount = IDirect3DDevice3_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_map_synchronisation(void)
{
    LARGE_INTEGER frequency, diff, ts[3];
    IDirect3DVertexBuffer *buffer;
    IDirect3DViewport3 *viewport;
    unsigned int i, j, tri_count;
    D3DVERTEXBUFFERDESC vb_desc;
    IDirect3DDevice3 *device;
    BOOL unsynchronised, ret;
    IDirectDrawSurface4 *rt;
    IDirect3D3 *d3d;
    D3DCOLOR colour;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};

    /* DDLOCK_DISCARDCONTENTS and DDLOCK_NOOVERWRITE were introduced with
     * ddraw7 and are ignored in earlier versions. */
    static const struct
    {
        unsigned int flags;
        BOOL unsynchronised;
    }
    tests[] =
    {
        {0,                                           FALSE},
        {DDLOCK_NOOVERWRITE,                          FALSE},
        {DDLOCK_DISCARDCONTENTS,                      FALSE},
        {DDLOCK_NOOVERWRITE | DDLOCK_DISCARDCONTENTS, FALSE},
    };

    static const struct quad
    {
        struct
        {
            struct vec3 position;
            DWORD diffuse;
        } strip[4];
    }
    quad1 =
    {
        {
            {{-1.0f, -1.0f, 0.0f}, 0xffff0000},
            {{-1.0f,  1.0f, 0.0f}, 0xff00ff00},
            {{ 1.0f, -1.0f, 0.0f}, 0xff0000ff},
            {{ 1.0f,  1.0f, 0.0f}, 0xffffffff},
        }
    },
    quad2 =
    {
        {
            {{-1.0f, -1.0f, 0.0f}, 0xffffff00},
            {{-1.0f,  1.0f, 0.0f}, 0xffffff00},
            {{ 1.0f, -1.0f, 0.0f}, 0xffffff00},
            {{ 1.0f,  1.0f, 0.0f}, 0xffffff00},
        }
    };
    struct quad *quads;

    window = create_window();
    ok(!!window, "Failed to create a window.\n");

    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get Direct3D3 interface, hr %#x.\n", hr);

    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);
    viewport = create_viewport(device, 0, 0, 640, 480);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);

    tri_count = 0x1000;

    ret = QueryPerformanceFrequency(&frequency);
    ok(ret, "Failed to get performance counter frequency.\n");

    vb_desc.dwSize = sizeof(vb_desc);
    vb_desc.dwCaps = D3DVBCAPS_WRITEONLY;
    vb_desc.dwFVF = D3DFVF_XYZ | D3DFVF_DIFFUSE;
    vb_desc.dwNumVertices = tri_count + 2;
    hr = IDirect3D3_CreateVertexBuffer(d3d, &vb_desc, &buffer, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to create vertex buffer, hr %#x.\n", hr);
    hr = IDirect3DVertexBuffer_Lock(buffer, DDLOCK_DISCARDCONTENTS, (void **)&quads, NULL);
    ok(SUCCEEDED(hr), "Failed to lock vertex buffer, hr %#x.\n", hr);
    for (j = 0; j < vb_desc.dwNumVertices / 4; ++j)
    {
        quads[j] = quad1;
    }
    hr = IDirect3DVertexBuffer_Unlock(buffer);
    ok(SUCCEEDED(hr), "Failed to unlock vertex buffer, hr %#x.\n", hr);

    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);

    /* Initial draw to initialise states, compile shaders, etc. */
    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0xff0000ff, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);
    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitiveVB(device, D3DPT_TRIANGLESTRIP, buffer, 0, vb_desc.dwNumVertices, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);
    /* Read the result to ensure the GPU has finished drawing. */
    colour = get_surface_color(rt, 320, 240);

    /* Time drawing tri_count triangles. */
    ret = QueryPerformanceCounter(&ts[0]);
    ok(ret, "Failed to read performance counter.\n");
    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0xff0000ff, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);
    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitiveVB(device, D3DPT_TRIANGLESTRIP, buffer, 0, vb_desc.dwNumVertices, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);
    colour = get_surface_color(rt, 320, 240);
    /* Time drawing a single triangle. */
    ret = QueryPerformanceCounter(&ts[1]);
    ok(ret, "Failed to read performance counter.\n");
    hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0xff0000ff, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);
    hr = IDirect3DDevice3_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice3_DrawPrimitiveVB(device, D3DPT_TRIANGLESTRIP, buffer, 0, 3, 0);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice3_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);
    colour = get_surface_color(rt, 320, 240);
    ret = QueryPerformanceCounter(&ts[2]);
    ok(ret, "Failed to read performance counter.\n");

    IDirect3DVertexBuffer_Release(buffer);

    /* Estimate the number of triangles we can draw in 100ms. */
    diff.QuadPart = ts[1].QuadPart - ts[0].QuadPart + ts[1].QuadPart - ts[2].QuadPart;
    tri_count = (tri_count * frequency.QuadPart) / (diff.QuadPart * 10);
    tri_count = ((tri_count + 2 + 3) & ~3) - 2;
    vb_desc.dwNumVertices = tri_count + 2;

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        hr = IDirect3D3_CreateVertexBuffer(d3d, &vb_desc, &buffer, 0, NULL);
        ok(SUCCEEDED(hr), "Failed to create vertex buffer, hr %#x.\n", hr);
        hr = IDirect3DVertexBuffer_Lock(buffer, DDLOCK_DISCARDCONTENTS, (void **)&quads, NULL);
        ok(SUCCEEDED(hr), "Failed to lock vertex buffer, hr %#x.\n", hr);
        for (j = 0; j < vb_desc.dwNumVertices / 4; ++j)
        {
            quads[j] = quad1;
        }
        hr = IDirect3DVertexBuffer_Unlock(buffer);
        ok(SUCCEEDED(hr), "Failed to unlock vertex buffer, hr %#x.\n", hr);

        /* Start a draw operation. */
        hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect, D3DCLEAR_TARGET, 0xff0000ff, 0.0f, 0);
        ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);
        hr = IDirect3DDevice3_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
        hr = IDirect3DDevice3_DrawPrimitiveVB(device, D3DPT_TRIANGLESTRIP, buffer, 0, vb_desc.dwNumVertices, 0);
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
        hr = IDirect3DDevice3_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

        /* Map the last quad while the draw is in progress. */
        hr = IDirect3DVertexBuffer_Lock(buffer, DDLOCK_WAIT | tests[i].flags, (void **)&quads, NULL);
        ok(SUCCEEDED(hr), "Failed to lock vertex buffer, hr %#x.\n", hr);
        quads[(vb_desc.dwNumVertices / 4) - 1] = quad2;
        hr = IDirect3DVertexBuffer_Unlock(buffer);
        ok(SUCCEEDED(hr), "Failed to unlock vertex buffer, hr %#x.\n", hr);

        colour = get_surface_color(rt, 320, 240);
        unsynchronised = compare_color(colour, 0x00ffff00, 1);
        ok(tests[i].unsynchronised == unsynchronised, "Expected %s map for flags %#x.\n",
                tests[i].unsynchronised ? "unsynchronised" : "synchronised", tests[i].flags);

        IDirect3DVertexBuffer_Release(buffer);
    }

    destroy_viewport(device, viewport);
    IDirectDrawSurface4_Release(rt);
    IDirect3D3_Release(d3d);
    refcount = IDirect3DDevice3_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_depth_readback(void)
{
    DWORD depth, expected_depth, max_diff, passed_fmts = 0;
    IDirectDrawSurface4 *rt, *ds;
    IDirect3DViewport3 *viewport;
    DDSURFACEDESC2 surface_desc;
    IDirect3DDevice3 *device;
    unsigned int i, x, y;
    IDirectDraw4 *ddraw;
    IDirect3D3 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    RECT r;
    BOOL all_pass;

    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    static struct
    {
        struct vec3 position;
        DWORD diffuse;
    }
    quad[] =
    {
        {{-1.0f, -1.0f, 0.1f}, 0xff00ff00},
        {{-1.0f,  1.0f, 0.0f}, 0xff00ff00},
        {{ 1.0f, -1.0f, 1.0f}, 0xff00ff00},
        {{ 1.0f,  1.0f, 0.9f}, 0xff00ff00},
    };

    static const struct
    {
        unsigned int z_depth, s_depth, z_mask, s_mask;
        BOOL todo;
    }
    tests[] =
    {
        {16, 0, 0x0000ffff, 0x00000000},
        {24, 0, 0x00ffffff, 0x00000000},
        {32, 0, 0x00ffffff, 0x00000000},
        {32, 8, 0x00ffffff, 0xff000000, TRUE},
        {32, 0, 0xffffffff, 0x00000000},
    };

    window = create_window();
    ok(!!window, "Failed to create a window.\n");

    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get Direct3D3 interface, hr %#x.\n", hr);
    hr = IDirect3D3_QueryInterface(d3d, &IID_IDirectDraw4, (void **)&ddraw);
    ok(SUCCEEDED(hr), "Failed to get ddraw interface, hr %#x.\n", hr);

    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);
    viewport = create_viewport(device, 0, 0, 640, 480);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);

    hr = IDirect3DDevice3_SetRenderState(device, D3DRENDERSTATE_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);

    ds = get_depth_stencil(device);
    hr = IDirectDrawSurface4_DeleteAttachedSurface(rt, 0, ds);
    ok(SUCCEEDED(hr), "Failed to detach depth buffer, hr %#x.\n", hr);
    IDirectDrawSurface4_Release(ds);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT;
        surface_desc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER | DDSCAPS_VIDEOMEMORY;
        U4(surface_desc).ddpfPixelFormat.dwSize = sizeof(U4(surface_desc).ddpfPixelFormat);
        U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_ZBUFFER;
        if (tests[i].s_depth)
            U4(surface_desc).ddpfPixelFormat.dwFlags |= DDPF_STENCILBUFFER;
        U1(U4(surface_desc).ddpfPixelFormat).dwZBufferBitDepth = tests[i].z_depth;
        U2(U4(surface_desc).ddpfPixelFormat).dwStencilBitDepth = tests[i].s_depth;
        U3(U4(surface_desc).ddpfPixelFormat).dwZBitMask = tests[i].z_mask;
        U4(U4(surface_desc).ddpfPixelFormat).dwStencilBitMask = tests[i].s_mask;
        surface_desc.dwWidth = 640;
        surface_desc.dwHeight = 480;
        hr = IDirectDraw4_CreateSurface(ddraw, &surface_desc, &ds, NULL);
        if (FAILED(hr))
        {
            skip("Format %u not supported, skipping test.\n", i);
            continue;
        }

        hr = IDirectDrawSurface_AddAttachedSurface(rt, ds);
        ok(SUCCEEDED(hr), "Failed to attach depth buffer, hr %#x.\n", hr);
        hr = IDirect3DDevice3_SetRenderTarget(device, rt, 0);
        ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);

        hr = IDirect3DViewport3_Clear2(viewport, 1, &clear_rect,
                D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff0000ff, 1.0f, 0);
        ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);
        hr = IDirect3DDevice3_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
        hr = IDirect3DDevice3_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, D3DFVF_XYZ | D3DFVF_DIFFUSE, quad, 4, 0);
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
        hr = IDirect3DDevice3_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

        all_pass = TRUE;
        for (y = 60; y < 480; y += 120)
        {
            for (x = 80; x < 640; x += 160)
            {
                SetRect(&r, x, y, x + 1, y + 1);
                memset(&surface_desc, 0, sizeof(surface_desc));
                surface_desc.dwSize = sizeof(surface_desc);
                hr = IDirectDrawSurface4_Lock(ds, &r, &surface_desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
                ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);

                depth = *((DWORD *)surface_desc.lpSurface) & tests[i].z_mask;
                expected_depth = (x * (0.9 / 640.0) + y * (0.1 / 480.0)) * tests[i].z_mask;
                max_diff = ((0.5f * 0.9f) / 640.0f) * tests[i].z_mask;
                /* The ddraw4 version of this test behaves similarly to the ddraw7 version on Nvidia GPUs,
                 * except that Geforce 7 also returns garbage data in D24S8, whereas the ddraw7 version
                 * returns 0 for that format. Give up on pre-filtering formats, accept Nvidia as generally
                 * broken here, but still expect at least one format (D16 or D24X8 in practise) to pass. */
                todo_wine_if(tests[i].todo)
                    ok(abs(expected_depth - depth) <= max_diff || ddraw_is_nvidia(ddraw),
                             "Test %u: Got depth 0x%08x (diff %d), expected 0x%08x+/-%u, at %u, %u.\n",
                             i, depth, expected_depth - depth, expected_depth, max_diff, x, y);
                if (abs(expected_depth - depth) > max_diff)
                    all_pass = FALSE;

                hr = IDirectDrawSurface4_Unlock(ds, &r);
                ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
            }
        }

        if (all_pass)
            passed_fmts++;

        hr = IDirectDrawSurface4_DeleteAttachedSurface(rt, 0, ds);
        ok(SUCCEEDED(hr), "Failed to detach depth buffer, hr %#x.\n", hr);
        IDirectDrawSurface4_Release(ds);
    }

    ok(passed_fmts, "Not a single format passed the tests, this is bad even by Nvidia's standards.\n");

    destroy_viewport(device, viewport);
    IDirectDrawSurface4_Release(rt);
    IDirectDraw4_Release(ddraw);
    IDirect3D3_Release(d3d);
    refcount = IDirect3DDevice3_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_clear(void)
{
    D3DRECT rect_negneg, rect_full = {{0}, {0}, {640}, {480}};
    IDirect3DViewport3 *viewport, *viewport2, *viewport3;
    IDirect3DDevice3 *device;
    IDirectDrawSurface4 *rt;
    IDirectDraw4 *ddraw;
    IDirect3D3 *d3d;
    D3DRECT rect[2];
    D3DCOLOR color;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = create_window();
    if (!(device = create_device(window, DDSCL_NORMAL)))
    {
        skip("Failed to create 3D device.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice3_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get Direct3D3 interface, hr %#x.\n", hr);
    hr = IDirect3D3_QueryInterface(d3d, &IID_IDirectDraw4, (void **)&ddraw);
    ok(SUCCEEDED(hr), "Failed to get ddraw interface, hr %#x.\n", hr);

    hr = IDirect3DDevice3_GetRenderTarget(device, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    viewport = create_viewport(device, 0, 0, 640, 480);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);

    hr = IDirect3DViewport3_Clear2(viewport, 1, &rect_full, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

    /* Positive x, negative y. */
    U1(rect[0]).x1 = 0;
    U2(rect[0]).y1 = 480;
    U3(rect[0]).x2 = 320;
    U4(rect[0]).y2 = 240;

    /* Positive x, positive y. */
    U1(rect[1]).x1 = 0;
    U2(rect[1]).y1 = 0;
    U3(rect[1]).x2 = 320;
    U4(rect[1]).y2 = 240;

    /* Clear 2 rectangles with one call. Unlike d3d8/9, the refrast does not
     * refuse negative rectangles, but it will not clear them either. */
    hr = IDirect3DViewport3_Clear2(viewport, 2, rect, D3DCLEAR_TARGET, 0xffff0000, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

    color = get_surface_color(rt, 160, 360);
    ok(compare_color(color, 0x00ffffff, 0), "Clear rectangle 3 (pos, neg) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 160, 120);
    ok(compare_color(color, 0x00ff0000, 0), "Clear rectangle 1 (pos, pos) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 480, 360);
    ok(compare_color(color, 0x00ffffff, 0), "Clear rectangle 4 (NULL) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 480, 120);
    ok(compare_color(color, 0x00ffffff, 0), "Clear rectangle 4 (neg, neg) has color 0x%08x.\n", color);

    hr = IDirect3DViewport3_Clear2(viewport, 1, &rect_full, D3DCLEAR_TARGET, 0xffffffff, 0.0, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);
    /* negative x, negative y.
     * Also ignored, except on WARP, which clears the entire screen. */
    rect_negneg.x1 = 640;
    rect_negneg.y1 = 240;
    rect_negneg.x2 = 320;
    rect_negneg.y2 = 0;
    hr = IDirect3DViewport3_Clear2(viewport, 1, &rect_negneg, D3DCLEAR_TARGET, 0xff00ff00, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

    color = get_surface_color(rt, 160, 360);
    ok(compare_color(color, 0x00ffffff, 0)
            || broken(ddraw_is_warp(ddraw) && compare_color(color, 0x0000ff00, 0)),
            "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 160, 120);
    ok(compare_color(color, 0x00ffffff, 0)
            || broken(ddraw_is_warp(ddraw) && compare_color(color, 0x0000ff00, 0)),
            "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 480, 360);
    ok(compare_color(color, 0x00ffffff, 0)
            || broken(ddraw_is_warp(ddraw) && compare_color(color, 0x0000ff00, 0)),
            "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 480, 120);
    ok(compare_color(color, 0x00ffffff, 0)
            || broken(ddraw_is_warp(ddraw) && compare_color(color, 0x0000ff00, 0)),
            "Got unexpected color 0x%08x.\n", color);

    /* Test how the viewport affects clears. */
    hr = IDirect3DViewport3_Clear2(viewport, 1, &rect_full, D3DCLEAR_TARGET, 0xffffffff, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

    viewport2 = create_viewport(device, 160, 120, 160, 120);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport2);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);

    hr = IDirect3DViewport3_Clear2(viewport2, 1, &rect_full, D3DCLEAR_TARGET, 0xff0000ff, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

    viewport3 = create_viewport(device, 320, 240, 320, 240);
    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport3);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);

    U1(rect[0]).x1 = 160;
    U2(rect[0]).y1 = 120;
    U3(rect[0]).x2 = 480;
    U4(rect[0]).y2 = 360;
    hr = IDirect3DViewport3_Clear2(viewport3, 1, &rect[0], D3DCLEAR_TARGET, 0xff00ff00, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

    hr = IDirect3DDevice3_SetCurrentViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to set current viewport, hr %#x.\n", hr);

    color = get_surface_color(rt, 158, 118);
    ok(compare_color(color, 0x00ffffff, 0), "(158, 118) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 162, 118);
    ok(compare_color(color, 0x00ffffff, 0), "(162, 118) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 158, 122);
    ok(compare_color(color, 0x00ffffff, 0), "(158, 122) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 162, 122);
    ok(compare_color(color, 0x000000ff, 0), "(162, 122) has color 0x%08x.\n", color);

    color = get_surface_color(rt, 318, 238);
    ok(compare_color(color, 0x000000ff, 0), "(318, 238) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 322, 238);
    ok(compare_color(color, 0x00ffffff, 0), "(322, 328) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 318, 242);
    ok(compare_color(color, 0x00ffffff, 0), "(318, 242) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 322, 242);
    ok(compare_color(color, 0x0000ff00, 0), "(322, 242) has color 0x%08x.\n", color);

    color = get_surface_color(rt, 478, 358);
    ok(compare_color(color, 0x0000ff00, 0), "(478, 358) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 482, 358);
    ok(compare_color(color, 0x00ffffff, 0), "(482, 358) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 478, 362);
    ok(compare_color(color, 0x00ffffff, 0), "(478, 362) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 482, 362);
    ok(compare_color(color, 0x00ffffff, 0), "(482, 362) has color 0x%08x.\n", color);

    /* The clear rectangle is rendertarget absolute, not relative to the
     * viewport. */
    hr = IDirect3DViewport3_Clear2(viewport, 1, &rect_full, D3DCLEAR_TARGET, 0xffffffff, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);
    U1(rect[0]).x1 = 330;
    U2(rect[0]).y1 = 250;
    U3(rect[0]).x2 = 340;
    U4(rect[0]).y2 = 260;
    hr = IDirect3DViewport3_Clear2(viewport3, 1, &rect[0], D3DCLEAR_TARGET, 0xff00ff00, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

    color = get_surface_color(rt, 328, 248);
    ok(compare_color(color, 0x00ffffff, 0), "(328, 248) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 332, 248);
    ok(compare_color(color, 0x00ffffff, 0), "(332, 248) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 328, 252);
    ok(compare_color(color, 0x00ffffff, 0), "(328, 252) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 332, 252);
    ok(compare_color(color, 0x0000ff00, 0), "(332, 252) has color 0x%08x.\n", color);

    color = get_surface_color(rt, 338, 248);
    ok(compare_color(color, 0x00ffffff, 0), "(338, 248) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 342, 248);
    ok(compare_color(color, 0x00ffffff, 0), "(342, 248) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 338, 252);
    ok(compare_color(color, 0x0000ff00, 0), "(338, 252) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 342, 252);
    ok(compare_color(color, 0x00ffffff, 0), "(342, 252) has color 0x%08x.\n", color);

    color = get_surface_color(rt, 328, 258);
    ok(compare_color(color, 0x00ffffff, 0), "(328, 258) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 332, 258);
    ok(compare_color(color, 0x0000ff00, 0), "(332, 258) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 328, 262);
    ok(compare_color(color, 0x00ffffff, 0), "(328, 262) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 332, 262);
    ok(compare_color(color, 0x00ffffff, 0), "(332, 262) has color 0x%08x.\n", color);

    color = get_surface_color(rt, 338, 258);
    ok(compare_color(color, 0x0000ff00, 0), "(338, 258) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 342, 258);
    ok(compare_color(color, 0x00ffffff, 0), "(342, 258) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 338, 262);
    ok(compare_color(color, 0x00ffffff, 0), "(338, 262) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 342, 262);
    ok(compare_color(color, 0x00ffffff, 0), "(342, 262) has color 0x%08x.\n", color);

    /* COLORWRITEENABLE, SRGBWRITEENABLE and scissor rectangles do not exist
     * in d3d3. */

    IDirect3DViewport3_Release(viewport3);
    IDirect3DViewport3_Release(viewport2);
    IDirect3DViewport3_Release(viewport);
    IDirectDrawSurface4_Release(rt);
    IDirectDraw4_Release(ddraw);
    IDirect3D3_Release(d3d);
    refcount = IDirect3DDevice3_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    DestroyWindow(window);
}

struct enum_surfaces_param
{
    IDirectDrawSurface4 *surfaces[8];
    unsigned int count;
};

static HRESULT WINAPI enum_surfaces_cb(IDirectDrawSurface4 *surface, DDSURFACEDESC2 *desc, void *context)
{
    struct enum_surfaces_param *param = context;
    BOOL found = FALSE;
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(param->surfaces); ++i)
    {
        if (param->surfaces[i] == surface)
        {
            found = TRUE;
            break;
        }
    }

    ok(found, "Unexpected surface %p enumerated.\n", surface);
    IDirectDrawSurface4_Release(surface);
    ++param->count;

    return DDENUMRET_OK;
}

static void test_enum_surfaces(void)
{
    struct enum_surfaces_param param = {{0}};
    DDSURFACEDESC2 desc;
    IDirectDraw4 *ddraw;
    HRESULT hr;

    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    hr = IDirectDraw4_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_MIPMAPCOUNT;
    desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
    U2(desc).dwMipMapCount = 3;
    desc.dwWidth = 32;
    desc.dwHeight = 32;
    hr = IDirectDraw4_CreateSurface(ddraw, &desc, &param.surfaces[0], NULL);
    ok(SUCCEEDED(hr), "Failed to create a surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface4_GetAttachedSurface(param.surfaces[0], &desc.ddsCaps, &param.surfaces[1]);
    ok(SUCCEEDED(hr), "Failed to get attached surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_GetAttachedSurface(param.surfaces[1], &desc.ddsCaps, &param.surfaces[2]);
    ok(SUCCEEDED(hr), "Failed to get attached surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface4_GetAttachedSurface(param.surfaces[2], &desc.ddsCaps, &param.surfaces[3]);
    ok(hr == DDERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);
    ok(!param.surfaces[3], "Got unexpected pointer %p.\n", param.surfaces[3]);

    hr = IDirectDraw4_EnumSurfaces(ddraw, DDENUMSURFACES_DOESEXIST | DDENUMSURFACES_ALL,
            &desc, &param, enum_surfaces_cb);
    ok(SUCCEEDED(hr), "Failed to enumerate surfaces, hr %#x.\n", hr);
    ok(param.count == 3, "Got unexpected number of enumerated surfaces %u.\n", param.count);

    param.count = 0;
    hr = IDirectDraw4_EnumSurfaces(ddraw, DDENUMSURFACES_DOESEXIST | DDENUMSURFACES_ALL,
            NULL, &param, enum_surfaces_cb);
    ok(SUCCEEDED(hr), "Failed to enumerate surfaces, hr %#x.\n", hr);
    ok(param.count == 3, "Got unexpected number of enumerated surfaces %u.\n", param.count);

    IDirectDrawSurface4_Release(param.surfaces[2]);
    IDirectDrawSurface4_Release(param.surfaces[1]);
    IDirectDrawSurface4_Release(param.surfaces[0]);
    IDirectDraw4_Release(ddraw);
}

START_TEST(ddraw4)
{
    DDDEVICEIDENTIFIER identifier;
    DEVMODEW current_mode;
    IDirectDraw4 *ddraw;
    HMODULE dwmapi;

    if (!(ddraw = create_ddraw()))
    {
        skip("Failed to create a ddraw object, skipping tests.\n");
        return;
    }

    if (ddraw_get_identifier(ddraw, &identifier))
    {
        trace("Driver string: \"%s\"\n", identifier.szDriver);
        trace("Description string: \"%s\"\n", identifier.szDescription);
        trace("Driver version %d.%d.%d.%d\n",
                HIWORD(U(identifier.liDriverVersion).HighPart), LOWORD(U(identifier.liDriverVersion).HighPart),
                HIWORD(U(identifier.liDriverVersion).LowPart), LOWORD(U(identifier.liDriverVersion).LowPart));
    }
    IDirectDraw4_Release(ddraw);

    memset(&current_mode, 0, sizeof(current_mode));
    current_mode.dmSize = sizeof(current_mode);
    ok(EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &current_mode), "Failed to get display mode.\n");
    registry_mode.dmSize = sizeof(registry_mode);
    ok(EnumDisplaySettingsW(NULL, ENUM_REGISTRY_SETTINGS, &registry_mode), "Failed to get display mode.\n");
    if (registry_mode.dmPelsWidth != current_mode.dmPelsWidth
            || registry_mode.dmPelsHeight != current_mode.dmPelsHeight)
    {
        skip("Current mode does not match registry mode, skipping test.\n");
        return;
    }

    if ((dwmapi = LoadLibraryA("dwmapi.dll")))
        pDwmIsCompositionEnabled = (void *)GetProcAddress(dwmapi, "DwmIsCompositionEnabled");

    test_process_vertices();
    test_coop_level_create_device_window();
    test_clipper_blt();
    test_coop_level_d3d_state();
    test_surface_interface_mismatch();
    test_coop_level_threaded();
    test_depth_blit();
    test_texture_load_ckey();
    test_viewport();
    test_zenable();
    test_ck_rgba();
    test_ck_default();
    test_ck_complex();
    test_surface_qi();
    test_device_qi();
    test_wndproc();
    test_window_style();
    test_redundant_mode_set();
    test_coop_level_mode_set();
    test_coop_level_mode_set_multi();
    test_initialize();
    test_coop_level_surf_create();
    test_vb_discard();
    test_coop_level_multi_window();
    test_draw_strided();
    test_lighting();
    test_specular_lighting();
    test_clear_rect_count();
    test_coop_level_versions();
    test_lighting_interface_versions();
    test_coop_level_activateapp();
    test_texturemanage();
    test_block_formats_creation();
    test_unsupported_formats();
    test_rt_caps();
    test_primary_caps();
    test_surface_lock();
    test_surface_discard();
    test_flip();
    test_set_surface_desc();
    test_user_memory_getdc();
    test_sysmem_overlay();
    test_primary_palette();
    test_surface_attachment();
    test_private_data();
    test_pixel_format();
    test_create_surface_pitch();
    test_mipmap();
    test_palette_complex();
    test_p8_blit();
    test_material();
    test_palette_gdi();
    test_palette_alpha();
    test_vb_writeonly();
    test_lost_device();
    test_surface_desc_lock();
    test_texturemapblend();
    test_signed_formats();
    test_color_fill();
    test_texcoordindex();
    test_colorkey_precision();
    test_range_colorkey();
    test_shademode();
    test_lockrect_invalid();
    test_yv12_overlay();
    test_offscreen_overlay();
    test_overlay_rect();
    test_blt();
    test_blt_z_alpha();
    test_color_clamping();
    test_getdc();
    test_draw_primitive();
    test_edge_antialiasing_blending();
    test_transform_vertices();
    test_display_mode_surface_pixel_format();
    test_surface_desc_size();
    test_get_surface_from_dc();
    test_ck_operation();
    test_vb_refcount();
    test_compute_sphere_visibility();
    test_texture_stages_limits();
    test_set_render_state();
    test_map_synchronisation();
    test_depth_readback();
    test_clear();
    test_enum_surfaces();
}
