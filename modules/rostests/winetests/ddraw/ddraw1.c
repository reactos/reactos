/*
 * Copyright 2005 Antoine Chavasse (a.chavasse@gmail.com)
 * Copyright 2008, 2011, 2012-2013 Stefan Dösinger for CodeWeavers
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

#include "wine/test.h"
#include <limits.h>
#include <math.h>
#include "d3d.h"

static BOOL is_ddraw64 = sizeof(DWORD) != sizeof(DWORD *);
static DEVMODEW registry_mode;

static HRESULT (WINAPI *pDwmIsCompositionEnabled)(BOOL *);

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#endif

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

static BOOL ddraw_get_identifier(IDirectDraw *ddraw, DDDEVICEIDENTIFIER *identifier)
{
    IDirectDraw4 *ddraw4;
    HRESULT hr;

    hr = IDirectDraw_QueryInterface(ddraw, &IID_IDirectDraw4, (void **)&ddraw4);
    ok(SUCCEEDED(hr), "Failed to get IDirectDraw4 interface, hr %#x.\n", hr);
    hr = IDirectDraw4_GetDeviceIdentifier(ddraw4, identifier, 0);
    ok(SUCCEEDED(hr), "Failed to get device identifier, hr %#x.\n", hr);
    IDirectDraw4_Release(ddraw4);

    return SUCCEEDED(hr);
}

static BOOL ddraw_is_warp(IDirectDraw *ddraw)
{
    DDDEVICEIDENTIFIER identifier;

    return strcmp(winetest_platform, "wine")
            && ddraw_get_identifier(ddraw, &identifier)
            && strstr(identifier.szDriver, "warp");
}

static BOOL ddraw_is_vendor(IDirectDraw *ddraw, DWORD vendor)
{
    DDDEVICEIDENTIFIER identifier;

    return strcmp(winetest_platform, "wine")
            && ddraw_get_identifier(ddraw, &identifier)
            && identifier.dwVendorId == vendor;
}

static BOOL ddraw_is_amd(IDirectDraw *ddraw)
{
    return ddraw_is_vendor(ddraw, 0x1002);
}

static BOOL ddraw_is_intel(IDirectDraw *ddraw)
{
    return ddraw_is_vendor(ddraw, 0x8086);
}

static BOOL ddraw_is_nvidia(IDirectDraw *ddraw)
{
    return ddraw_is_vendor(ddraw, 0x10de);
}

static BOOL ddraw_is_vmware(IDirectDraw *ddraw)
{
    return ddraw_is_vendor(ddraw, 0x15ad);
}

static IDirectDrawSurface *create_overlay(IDirectDraw *ddraw,
        unsigned int width, unsigned int height, DWORD format)
{
    IDirectDrawSurface *surface;
    DDSURFACEDESC desc;

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    desc.dwWidth = width;
    desc.dwHeight = height;
    desc.ddsCaps.dwCaps = DDSCAPS_OVERLAY;
    desc.ddpfPixelFormat.dwSize = sizeof(desc.ddpfPixelFormat);
    desc.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
    desc.ddpfPixelFormat.dwFourCC = format;

    if (FAILED(IDirectDraw_CreateSurface(ddraw, &desc, &surface, NULL)))
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

static IDirectDrawSurface *get_depth_stencil(IDirect3DDevice *device)
{
    IDirectDrawSurface *rt, *ret;
    DDSCAPS caps = {DDSCAPS_ZBUFFER};
    HRESULT hr;

    hr = IDirect3DDevice_QueryInterface(device, &IID_IDirectDrawSurface, (void **)&rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);
    hr = IDirectDrawSurface_GetAttachedSurface(rt, &caps, &ret);
    ok(SUCCEEDED(hr) || hr == DDERR_NOTFOUND, "Failed to get the z buffer, hr %#x.\n", hr);
    IDirectDrawSurface_Release(rt);
    return ret;
}

static HRESULT set_display_mode(IDirectDraw *ddraw, DWORD width, DWORD height)
{
    if (SUCCEEDED(IDirectDraw_SetDisplayMode(ddraw, width, height, 32)))
        return DD_OK;
    return IDirectDraw_SetDisplayMode(ddraw, width, height, 24);
}

static D3DCOLOR get_surface_color(IDirectDrawSurface *surface, UINT x, UINT y)
{
    RECT rect = {x, y, x + 1, y + 1};
    DDSURFACEDESC surface_desc;
    D3DCOLOR color;
    HRESULT hr;

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);

    hr = IDirectDrawSurface_Lock(surface, &rect, &surface_desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    if (FAILED(hr))
        return 0xdeadbeef;

    color = *((DWORD *)surface_desc.lpSurface) & 0x00ffffff;

    hr = IDirectDrawSurface_Unlock(surface, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    return color;
}

static void emit_process_vertices(void **ptr, DWORD flags, WORD base_idx, DWORD vertex_count)
{
    D3DINSTRUCTION *inst = *ptr;
    D3DPROCESSVERTICES *pv = (D3DPROCESSVERTICES *)(inst + 1);

    inst->bOpcode = D3DOP_PROCESSVERTICES;
    inst->bSize = sizeof(*pv);
    inst->wCount = 1;

    pv->dwFlags = flags;
    pv->wStart = base_idx;
    pv->wDest = 0;
    pv->dwCount = vertex_count;
    pv->dwReserved = 0;

    *ptr = pv + 1;
}

static void emit_set_ts(void **ptr, D3DTRANSFORMSTATETYPE state, DWORD value)
{
    D3DINSTRUCTION *inst = *ptr;
    D3DSTATE *ts = (D3DSTATE *)(inst + 1);

    inst->bOpcode = D3DOP_STATETRANSFORM;
    inst->bSize = sizeof(*ts);
    inst->wCount = 1;

    U1(*ts).dtstTransformStateType = state;
    U2(*ts).dwArg[0] = value;

    *ptr = ts + 1;
}

static void emit_set_ls(void **ptr, D3DLIGHTSTATETYPE state, DWORD value)
{
    D3DINSTRUCTION *inst = *ptr;
    D3DSTATE *ls = (D3DSTATE *)(inst + 1);

    inst->bOpcode = D3DOP_STATELIGHT;
    inst->bSize = sizeof(*ls);
    inst->wCount = 1;

    U1(*ls).dlstLightStateType = state;
    U2(*ls).dwArg[0] = value;

    *ptr = ls + 1;
}

static void emit_set_rs(void **ptr, D3DRENDERSTATETYPE state, DWORD value)
{
    D3DINSTRUCTION *inst = *ptr;
    D3DSTATE *rs = (D3DSTATE *)(inst + 1);

    inst->bOpcode = D3DOP_STATERENDER;
    inst->bSize = sizeof(*rs);
    inst->wCount = 1;

    U1(*rs).drstRenderStateType = state;
    U2(*rs).dwArg[0] = value;

    *ptr = rs + 1;
}

static void emit_tquad(void **ptr, WORD base_idx)
{
    D3DINSTRUCTION *inst = *ptr;
    D3DTRIANGLE *tri = (D3DTRIANGLE *)(inst + 1);

    inst->bOpcode = D3DOP_TRIANGLE;
    inst->bSize = sizeof(*tri);
    inst->wCount = 2;

    U1(*tri).v1 = base_idx;
    U2(*tri).v2 = base_idx + 1;
    U3(*tri).v3 = base_idx + 2;
    tri->wFlags = D3DTRIFLAG_START;
    ++tri;

    U1(*tri).v1 = base_idx + 2;
    U2(*tri).v2 = base_idx + 1;
    U3(*tri).v3 = base_idx + 3;
    tri->wFlags = D3DTRIFLAG_ODD;
    ++tri;

    *ptr = tri;
}

static void emit_tquad_tlist(void **ptr, WORD base_idx)
{
    D3DINSTRUCTION *inst = *ptr;
    D3DTRIANGLE *tri = (D3DTRIANGLE *)(inst + 1);

    inst->bOpcode = D3DOP_TRIANGLE;
    inst->bSize = sizeof(*tri);
    inst->wCount = 2;

    U1(*tri).v1 = base_idx;
    U2(*tri).v2 = base_idx + 1;
    U3(*tri).v3 = base_idx + 2;
    tri->wFlags = D3DTRIFLAG_START;
    ++tri;

    U1(*tri).v1 = base_idx + 2;
    U2(*tri).v2 = base_idx + 3;
    U3(*tri).v3 = base_idx;
    tri->wFlags = D3DTRIFLAG_START;
    ++tri;

    *ptr = tri;
}

static void emit_texture_load(void **ptr, D3DTEXTUREHANDLE dst_texture,
        D3DTEXTUREHANDLE src_texture)
{
    D3DINSTRUCTION *instruction = *ptr;
    D3DTEXTURELOAD *texture_load = (D3DTEXTURELOAD *)(instruction + 1);

    instruction->bOpcode = D3DOP_TEXTURELOAD;
    instruction->bSize = sizeof(*texture_load);
    instruction->wCount = 1;

    texture_load->hDestTexture = dst_texture;
    texture_load->hSrcTexture = src_texture;
    ++texture_load;

    *ptr = texture_load;
}

static void emit_end(void **ptr)
{
    D3DINSTRUCTION *inst = *ptr;

    inst->bOpcode = D3DOP_EXIT;
    inst->bSize = 0;
    inst->wCount = 0;

    *ptr = inst + 1;
}

static void set_execute_data(IDirect3DExecuteBuffer *execute_buffer, UINT vertex_count, UINT offset, UINT len)
{
    D3DEXECUTEDATA exec_data;
    HRESULT hr;

    memset(&exec_data, 0, sizeof(exec_data));
    exec_data.dwSize = sizeof(exec_data);
    exec_data.dwVertexCount = vertex_count;
    exec_data.dwInstructionOffset = offset;
    exec_data.dwInstructionLength = len;
    hr = IDirect3DExecuteBuffer_SetExecuteData(execute_buffer, &exec_data);
    ok(SUCCEEDED(hr), "Failed to set execute data, hr %#x.\n", hr);
}

static DWORD get_device_z_depth(IDirect3DDevice *device)
{
    DDSCAPS caps = {DDSCAPS_ZBUFFER};
    IDirectDrawSurface *ds, *rt;
    DDSURFACEDESC desc;
    HRESULT hr;

    if (FAILED(IDirect3DDevice_QueryInterface(device, &IID_IDirectDrawSurface, (void **)&rt)))
        return 0;

    hr = IDirectDrawSurface_GetAttachedSurface(rt, &caps, &ds);
    IDirectDrawSurface_Release(rt);
    if (FAILED(hr))
        return 0;

    desc.dwSize = sizeof(desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(ds, &desc);
    IDirectDrawSurface_Release(ds);
    if (FAILED(hr))
        return 0;

    return U2(desc).dwZBufferBitDepth;
}

static IDirectDraw *create_ddraw(void)
{
    IDirectDraw *ddraw;

    if (FAILED(DirectDrawCreate(NULL, &ddraw, NULL)))
        return NULL;

    return ddraw;
}

static IDirect3DDevice *create_device(IDirectDraw *ddraw, HWND window, DWORD coop_level)
{
    /* Prefer 16 bit depth buffers because Nvidia gives us an unpadded D24 buffer if we ask
     * for 24 bit and handles such buffers incorrectly in DDBLT_DEPTHFILL. AMD only supports
     * 16 bit buffers in ddraw1/2. Stencil was added in ddraw4, so we cannot create a D24S8
     * buffer here. */
    static const DWORD z_depths[] = {16, 32, 24};
    IDirectDrawSurface *surface, *ds;
    IDirect3DDevice *device = NULL;
    DDSURFACEDESC surface_desc;
    unsigned int i;
    HRESULT hr;

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, coop_level);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    surface_desc.dwWidth = 640;
    surface_desc.dwHeight = 480;

    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    if (coop_level & DDSCL_NORMAL)
    {
        IDirectDrawClipper *clipper;

        hr = IDirectDraw_CreateClipper(ddraw, 0, &clipper, NULL);
        ok(SUCCEEDED(hr), "Failed to create clipper, hr %#x.\n", hr);
        hr = IDirectDrawClipper_SetHWnd(clipper, 0, window);
        ok(SUCCEEDED(hr), "Failed to set clipper window, hr %#x.\n", hr);
        hr = IDirectDrawSurface_SetClipper(surface, clipper);
        ok(SUCCEEDED(hr), "Failed to set surface clipper, hr %#x.\n", hr);
        IDirectDrawClipper_Release(clipper);
    }

    /* We used to use EnumDevices() for this, but it seems
     * D3DDEVICEDESC.dwDeviceZBufferBitDepth only has a very casual
     * relationship with reality. */
    for (i = 0; i < ARRAY_SIZE(z_depths); ++i)
    {
        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS | DDSD_ZBUFFERBITDEPTH | DDSD_WIDTH | DDSD_HEIGHT;
        surface_desc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
        U2(surface_desc).dwZBufferBitDepth = z_depths[i];
        surface_desc.dwWidth = 640;
        surface_desc.dwHeight = 480;
        if (FAILED(IDirectDraw_CreateSurface(ddraw, &surface_desc, &ds, NULL)))
            continue;

        hr = IDirectDrawSurface_AddAttachedSurface(surface, ds);
        ok(SUCCEEDED(hr), "Failed to attach depth buffer, hr %#x.\n", hr);
        IDirectDrawSurface_Release(ds);
        if (FAILED(hr))
            continue;

        if (SUCCEEDED(IDirectDrawSurface_QueryInterface(surface, &IID_IDirect3DHALDevice, (void **)&device)))
            break;

        IDirectDrawSurface_DeleteAttachedSurface(surface, 0, ds);
    }

    IDirectDrawSurface_Release(surface);
    return device;
}

static IDirect3DViewport *create_viewport(IDirect3DDevice *device, UINT x, UINT y, UINT w, UINT h)
{
    IDirect3DViewport *viewport;
    D3DVIEWPORT vp;
    IDirect3D *d3d;
    HRESULT hr;

    hr = IDirect3DDevice_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get d3d interface, hr %#x.\n", hr);
    hr = IDirect3D_CreateViewport(d3d, &viewport, NULL);
    ok(SUCCEEDED(hr), "Failed to create viewport, hr %#x.\n", hr);
    hr = IDirect3DDevice_AddViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to add viewport, hr %#x.\n", hr);
    memset(&vp, 0, sizeof(vp));
    vp.dwSize = sizeof(vp);
    vp.dwX = x;
    vp.dwY = y;
    vp.dwWidth = w;
    vp.dwHeight = h;
    vp.dvScaleX = (float)w / 2.0f;
    vp.dvScaleY = (float)h / 2.0f;
    vp.dvMaxX = 1.0f;
    vp.dvMaxY = 1.0f;
    vp.dvMinZ = 0.0f;
    vp.dvMaxZ = 1.0f;
    hr = IDirect3DViewport_SetViewport(viewport, &vp);
    ok(SUCCEEDED(hr), "Failed to set viewport data, hr %#x.\n", hr);
    IDirect3D_Release(d3d);

    return viewport;
}

static void viewport_set_background(IDirect3DDevice *device, IDirect3DViewport *viewport,
        IDirect3DMaterial *material)
{
    D3DMATERIALHANDLE material_handle;
    HRESULT hr;

    hr = IDirect3DMaterial2_GetHandle(material, device, &material_handle);
    ok(SUCCEEDED(hr), "Failed to get material handle, hr %#x.\n", hr);
    hr = IDirect3DViewport2_SetBackground(viewport, material_handle);
    ok(SUCCEEDED(hr), "Failed to set viewport background, hr %#x.\n", hr);
}

static void destroy_viewport(IDirect3DDevice *device, IDirect3DViewport *viewport)
{
    HRESULT hr;

    hr = IDirect3DDevice_DeleteViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to delete viewport, hr %#x.\n", hr);
    IDirect3DViewport_Release(viewport);
}

static IDirect3DMaterial *create_material(IDirect3DDevice *device, D3DMATERIAL *mat)
{
    IDirect3DMaterial *material;
    IDirect3D *d3d;
    HRESULT hr;

    hr = IDirect3DDevice_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get d3d interface, hr %#x.\n", hr);
    hr = IDirect3D_CreateMaterial(d3d, &material, NULL);
    ok(SUCCEEDED(hr), "Failed to create material, hr %#x.\n", hr);
    hr = IDirect3DMaterial_SetMaterial(material, mat);
    ok(SUCCEEDED(hr), "Failed to set material data, hr %#x.\n", hr);
    IDirect3D_Release(d3d);

    return material;
}

static IDirect3DMaterial *create_diffuse_material(IDirect3DDevice *device, float r, float g, float b, float a)
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

static IDirect3DMaterial *create_emissive_material(IDirect3DDevice *device, float r, float g, float b, float a)
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

static void destroy_material(IDirect3DMaterial *material)
{
    IDirect3DMaterial_Release(material);
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
    IDirectDraw *ddraw;
    HRESULT hr;

    if (!(ddraw = create_ddraw()))
        return;

    SetWindowLongPtrA(window, GWLP_WNDPROC, proc);
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    IDirectDraw_Release(ddraw);
}

static HRESULT CALLBACK restore_callback(IDirectDrawSurface *surface, DDSURFACEDESC *desc, void *context)
{
    HRESULT hr = IDirectDrawSurface_Restore(surface);
    ok(SUCCEEDED(hr) || hr == DDERR_IMPLICITLYCREATED, "Failed to restore surface, hr %#x.\n", hr);
    IDirectDrawSurface_Release(surface);

    return DDENUMRET_OK;
}

static HRESULT restore_surfaces(IDirectDraw *ddraw)
{
    return IDirectDraw_EnumSurfaces(ddraw, DDENUMSURFACES_ALL | DDENUMSURFACES_DOESEXIST,
            NULL, NULL, restore_callback);
}

static void test_coop_level_create_device_window(void)
{
    HWND focus_window, device_window;
    IDirectDraw *ddraw;
    HRESULT hr;

    focus_window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW | DDSCL_NORMAL);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW | DDSCL_NORMAL | DDSCL_FULLSCREEN);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DDERR_NOFOCUSWINDOW || broken(hr == DDERR_INVALIDPARAMS), "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");

    /* Windows versions before 98 / NT5 don't support DDSCL_CREATEDEVICEWINDOW. */
    if (broken(hr == DDERR_INVALIDPARAMS))
    {
        win_skip("DDSCL_CREATEDEVICEWINDOW not supported, skipping test.\n");
        IDirectDraw_Release(ddraw);
        DestroyWindow(focus_window);
        return;
    }

    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw_SetCooperativeLevel(ddraw, focus_window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");

    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_SETFOCUSWINDOW
            | DDSCL_CREATEDEVICEWINDOW | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DDERR_NOHWND, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!!device_window, "Device window not found.\n");

    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw_SetCooperativeLevel(ddraw, focus_window, DDSCL_SETFOCUSWINDOW
            | DDSCL_CREATEDEVICEWINDOW | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!!device_window, "Device window not found.\n");

    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DDERR_NOFOCUSWINDOW, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw_SetCooperativeLevel(ddraw, focus_window, DDSCL_SETFOCUSWINDOW);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!device_window, "Unexpected device window found.\n");
    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_CREATEDEVICEWINDOW | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    device_window = FindWindowA("DirectDrawDeviceWnd", "DirectDrawDeviceWnd");
    ok(!!device_window, "Device window not found.\n");

    IDirectDraw_Release(ddraw);
    DestroyWindow(focus_window);
}

static void test_clipper_blt(void)
{
    IDirectDrawSurface *src_surface, *dst_surface;
    RECT client_rect, src_rect;
    IDirectDrawClipper *clipper;
    DDSURFACEDESC surface_desc;
    unsigned int i, j, x, y;
    IDirectDraw *ddraw;
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

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    hr = IDirectDraw_CreateClipper(ddraw, 0, &clipper, NULL);
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
    surface_desc.ddpfPixelFormat.dwSize = sizeof(surface_desc.ddpfPixelFormat);
    surface_desc.ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(surface_desc.ddpfPixelFormat).dwRGBBitCount = 32;
    U2(surface_desc.ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(surface_desc.ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(surface_desc.ddpfPixelFormat).dwBBitMask = 0x000000ff;

    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &src_surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create source surface, hr %#x.\n", hr);
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &dst_surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create destination surface, hr %#x.\n", hr);

    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    hr = IDirectDrawSurface_Blt(src_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear source surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface_Blt(dst_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear destination surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface_Lock(src_surface, NULL, &surface_desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock source surface, hr %#x.\n", hr);
    ok(U1(surface_desc).lPitch == 2560, "Got unexpected surface pitch %u.\n", U1(surface_desc).lPitch);
    ptr = surface_desc.lpSurface;
    memcpy(&ptr[   0], &src_data[ 0], 6 * sizeof(DWORD));
    memcpy(&ptr[ 640], &src_data[ 6], 6 * sizeof(DWORD));
    memcpy(&ptr[1280], &src_data[12], 6 * sizeof(DWORD));
    hr = IDirectDrawSurface_Unlock(src_surface, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock source surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface_SetClipper(dst_surface, clipper);
    ok(SUCCEEDED(hr), "Failed to set clipper, hr %#x.\n", hr);

    SetRect(&src_rect, 1, 1, 5, 2);
    hr = IDirectDrawSurface_Blt(dst_surface, NULL, src_surface, &src_rect, DDBLT_WAIT, NULL);
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
    hr = IDirectDrawSurface_Blt(dst_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
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

    hr = IDirectDrawSurface_BltFast(dst_surface, 0, 0, src_surface, NULL, DDBLTFAST_WAIT);
    ok(hr == DDERR_BLTFASTCANTCLIP || broken(hr == E_NOTIMPL /* NT4 */), "Got unexpected hr %#x.\n", hr);

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
    hr = IDirectDrawSurface_Blt(dst_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_NOCLIPLIST, "Got unexpected hr %#x.\n", hr);

    IDirectDrawSurface_Release(dst_surface);
    IDirectDrawSurface_Release(src_surface);
    refcount = IDirectDrawClipper_Release(clipper);
    ok(!refcount, "Clipper has %u references left.\n", refcount);
    IDirectDraw_Release(ddraw);
}

static void test_coop_level_d3d_state(void)
{
    D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    IDirectDrawSurface *rt, *surface;
    IDirect3DMaterial *background;
    IDirect3DViewport *viewport;
    IDirect3DDevice *device;
    D3DMATERIAL material;
    IDirectDraw *ddraw;
    D3DCOLOR color;
    HWND window;
    HRESULT hr;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    if (!(device = create_device(ddraw, window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        IDirectDraw_Release(ddraw);
        DestroyWindow(window);
        return;
    }

    background = create_diffuse_material(device, 1.0f, 0.0f, 0.0f, 1.0f);
    viewport = create_viewport(device, 0, 0, 640, 480);
    viewport_set_background(device, viewport, background);

    hr = IDirect3DDevice_QueryInterface(device, &IID_IDirectDrawSurface, (void **)&rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);
    hr = IDirect3DViewport_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x00ff0000, 1), "Got unexpected color 0x%08x.\n", color);

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    hr = IDirectDrawSurface_IsLost(rt);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);
    hr = restore_surfaces(ddraw);
    ok(SUCCEEDED(hr), "Failed to restore surfaces, hr %#x.\n", hr);

    memset(&material, 0, sizeof(material));
    material.dwSize = sizeof(material);
    U1(U(material).diffuse).r = 0.0f;
    U2(U(material).diffuse).g = 1.0f;
    U3(U(material).diffuse).b = 0.0f;
    U4(U(material).diffuse).a = 1.0f;
    hr = IDirect3DMaterial_SetMaterial(background, &material);
    ok(SUCCEEDED(hr), "Failed to set material data, hr %#x.\n", hr);

    hr = IDirect3DDevice_QueryInterface(device, &IID_IDirectDrawSurface, (void **)&surface);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);
    ok(surface == rt, "Got unexpected surface %p.\n", surface);
    hr = IDirect3DViewport_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x0000ff00, 1) || broken(compare_color(color, 0x00000000, 1)),
            "Got unexpected color 0x%08x.\n", color);

    destroy_viewport(device, viewport);
    destroy_material(background);
    IDirectDrawSurface_Release(surface);
    IDirectDrawSurface_Release(rt);
    IDirect3DDevice_Release(device);
    IDirectDraw_Release(ddraw);
    DestroyWindow(window);
}

static void test_surface_interface_mismatch(void)
{
    IDirectDraw *ddraw = NULL;
    IDirectDrawSurface *surface = NULL, *ds;
    IDirectDrawSurface3 *surface3 = NULL;
    IDirect3DDevice *device = NULL;
    IDirect3DViewport *viewport = NULL;
    IDirect3DMaterial *background = NULL;
    DDSURFACEDESC surface_desc;
    DWORD z_depth = 0;
    ULONG refcount;
    HRESULT hr;
    D3DCOLOR color;
    HWND window;
    D3DRECT clear_rect = {{0}, {0}, {640}, {480}};

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    if (!(device = create_device(ddraw, window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        IDirectDraw_Release(ddraw);
        DestroyWindow(window);
        return;
    }
    z_depth = get_device_z_depth(device);
    ok(!!z_depth, "Failed to get device z depth.\n");
    IDirect3DDevice_Release(device);
    device = NULL;

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    surface_desc.dwWidth = 640;
    surface_desc.dwHeight = 480;

    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface_QueryInterface(surface, &IID_IDirectDrawSurface3, (void **)&surface3);
    if (FAILED(hr))
    {
        skip("Failed to get the IDirectDrawSurface3 interface, skipping test.\n");
        goto cleanup;
    }

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_ZBUFFERBITDEPTH | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    U2(surface_desc).dwZBufferBitDepth = z_depth;
    surface_desc.dwWidth = 640;
    surface_desc.dwHeight = 480;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &ds, NULL);
    ok(SUCCEEDED(hr), "Failed to create depth buffer, hr %#x.\n", hr);
    if (FAILED(hr))
        goto cleanup;

    /* Using a different surface interface version still works */
    hr = IDirectDrawSurface3_AddAttachedSurface(surface3, (IDirectDrawSurface3 *)ds);
    ok(SUCCEEDED(hr), "Failed to attach depth buffer, hr %#x.\n", hr);
    refcount = IDirectDrawSurface_Release(ds);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);
    if (FAILED(hr))
        goto cleanup;

    /* Here too */
    hr = IDirectDrawSurface3_QueryInterface(surface3, &IID_IDirect3DHALDevice, (void **)&device);
    ok(SUCCEEDED(hr), "Failed to create d3d device.\n");
    if (FAILED(hr))
        goto cleanup;

    background = create_diffuse_material(device, 1.0f, 0.0f, 0.0f, 1.0f);
    viewport = create_viewport(device, 0, 0, 640, 480);
    viewport_set_background(device, viewport, background);

    hr = IDirect3DViewport_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear render target, hr %#x.\n", hr);
    color = get_surface_color(surface, 320, 240);
    ok(compare_color(color, 0x00ff0000, 1), "Got unexpected color 0x%08x.\n", color);

cleanup:
    if (viewport)
        destroy_viewport(device, viewport);
    if (background)
        destroy_material(background);
    if (surface3) IDirectDrawSurface3_Release(surface3);
    if (surface) IDirectDrawSurface_Release(surface);
    if (device) IDirect3DDevice_Release(device);
    if (ddraw) IDirectDraw_Release(ddraw);
    DestroyWindow(window);
}

static void test_coop_level_threaded(void)
{
    struct create_window_thread_param p;
    IDirectDraw *ddraw;
    HRESULT hr;

    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    create_window_thread(&p);

    hr = IDirectDraw_SetCooperativeLevel(ddraw, p.window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    IDirectDraw_Release(ddraw);
    destroy_window_thread(&p);
}

static ULONG get_refcount(IUnknown *test_iface)
{
    IUnknown_AddRef(test_iface);
    return IUnknown_Release(test_iface);
}

static void test_viewport(void)
{
    IDirectDraw *ddraw;
    IDirect3D *d3d;
    HRESULT hr;
    ULONG ref;
    IDirect3DViewport *viewport, *another_vp;
    IDirect3DViewport2 *viewport2;
    IDirect3DViewport3 *viewport3;
    IDirectDrawGammaControl *gamma;
    IUnknown *unknown;
    IDirect3DDevice *device;
    HWND window;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    if (!(device = create_device(ddraw, window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        IDirectDraw_Release(ddraw);
        DestroyWindow(window);
        return;
    }

    hr = IDirectDraw_QueryInterface(ddraw, &IID_IDirect3D, (void **)&d3d);
    ok(SUCCEEDED(hr), "Failed to get d3d interface, hr %#x.\n", hr);
    ref = get_refcount((IUnknown *) d3d);
    ok(ref == 2, "IDirect3D refcount is %d\n", ref);

    hr = IDirect3D_CreateViewport(d3d, &viewport, NULL);
    ok(SUCCEEDED(hr), "Failed to create viewport, hr %#x.\n", hr);
    ref = get_refcount((IUnknown *)viewport);
    ok(ref == 1, "Initial IDirect3DViewport refcount is %u\n", ref);
    ref = get_refcount((IUnknown *)d3d);
    ok(ref == 2, "IDirect3D refcount is %u\n", ref);

    /* E_FAIL return values are returned by Winetestbot Windows NT machines. While not supporting
     * newer interfaces is legitimate for old ddraw versions, E_FAIL violates Microsoft's rules
     * for QueryInterface, hence the broken() */
    gamma = (IDirectDrawGammaControl *)0xdeadbeef;
    hr = IDirect3DViewport_QueryInterface(viewport, &IID_IDirectDrawGammaControl, (void **)&gamma);
    ok(hr == E_NOINTERFACE || broken(hr == E_FAIL), "Got unexpected hr %#x.\n", hr);
    ok(gamma == NULL, "Interface not set to NULL by failed QI call: %p\n", gamma);
    if (SUCCEEDED(hr)) IDirectDrawGammaControl_Release(gamma);
    /* NULL iid: Segfaults */

    hr = IDirect3DViewport_QueryInterface(viewport, &IID_IDirect3DViewport2, (void **)&viewport2);
    ok(SUCCEEDED(hr) || hr == E_NOINTERFACE || broken(hr == E_FAIL),
            "Failed to QI IDirect3DViewport2, hr %#x.\n", hr);
    if (viewport2)
    {
        ref = get_refcount((IUnknown *)viewport);
        ok(ref == 2, "IDirect3DViewport refcount is %u\n", ref);
        ref = get_refcount((IUnknown *)viewport2);
        ok(ref == 2, "IDirect3DViewport2 refcount is %u\n", ref);
        IDirect3DViewport2_Release(viewport2);
        viewport2 = NULL;
    }

    hr = IDirect3DViewport_QueryInterface(viewport, &IID_IDirect3DViewport3, (void **)&viewport3);
    ok(SUCCEEDED(hr) || hr == E_NOINTERFACE || broken(hr == E_FAIL),
            "Failed to QI IDirect3DViewport3, hr %#x.\n", hr);
    if (viewport3)
    {
        ref = get_refcount((IUnknown *)viewport);
        ok(ref == 2, "IDirect3DViewport refcount is %u\n", ref);
        ref = get_refcount((IUnknown *)viewport3);
        ok(ref == 2, "IDirect3DViewport3 refcount is %u\n", ref);
        IDirect3DViewport3_Release(viewport3);
    }

    hr = IDirect3DViewport_QueryInterface(viewport, &IID_IUnknown, (void **)&unknown);
    ok(SUCCEEDED(hr), "Failed to QI IUnknown, hr %#x.\n", hr);
    if (unknown)
    {
        ref = get_refcount((IUnknown *)viewport);
        ok(ref == 2, "IDirect3DViewport refcount is %u\n", ref);
        ref = get_refcount(unknown);
        ok(ref == 2, "IUnknown refcount is %u\n", ref);
        IUnknown_Release(unknown);
    }

    /* AddViewport(NULL): Segfault */
    hr = IDirect3DDevice_DeleteViewport(device, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3D_CreateViewport(d3d, &another_vp, NULL);
    ok(SUCCEEDED(hr), "Failed to create viewport, hr %#x.\n", hr);

    hr = IDirect3DDevice_AddViewport(device, viewport);
    ok(SUCCEEDED(hr), "Failed to add viewport to device, hr %#x.\n", hr);
    ref = get_refcount((IUnknown *) viewport);
    ok(ref == 2, "IDirect3DViewport refcount is %d\n", ref);
    hr = IDirect3DDevice_AddViewport(device, another_vp);
    ok(SUCCEEDED(hr), "Failed to add viewport to device, hr %#x.\n", hr);
    ref = get_refcount((IUnknown *) another_vp);
    ok(ref == 2, "IDirect3DViewport refcount is %d\n", ref);

    hr = IDirect3DDevice_DeleteViewport(device, another_vp);
    ok(SUCCEEDED(hr), "Failed to delete viewport from device, hr %#x.\n", hr);
    ref = get_refcount((IUnknown *) another_vp);
    ok(ref == 1, "IDirect3DViewport refcount is %d\n", ref);

    IDirect3DDevice_Release(device);
    ref = get_refcount((IUnknown *) viewport);
    ok(ref == 1, "IDirect3DViewport refcount is %d\n", ref);

    IDirect3DViewport_Release(another_vp);
    IDirect3D_Release(d3d);
    IDirect3DViewport_Release(viewport);
    DestroyWindow(window);
    IDirectDraw_Release(ddraw);
}

static void test_zenable(void)
{
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    static D3DTLVERTEX tquad[] =
    {
        {{  0.0f}, {480.0f}, {-0.5f}, {1.0f}, {0xff00ff00}, {0x00000000}, {0.0f}, {0.0f}},
        {{  0.0f}, {  0.0f}, {-0.5f}, {1.0f}, {0xff00ff00}, {0x00000000}, {0.0f}, {0.0f}},
        {{640.0f}, {480.0f}, { 1.5f}, {1.0f}, {0xff00ff00}, {0x00000000}, {0.0f}, {0.0f}},
        {{640.0f}, {  0.0f}, { 1.5f}, {1.0f}, {0xff00ff00}, {0x00000000}, {0.0f}, {0.0f}},
    };
    IDirect3DExecuteBuffer *execute_buffer;
    D3DEXECUTEBUFFERDESC exec_desc;
    IDirect3DMaterial *background;
    IDirect3DViewport *viewport;
    IDirect3DDevice *device;
    IDirectDrawSurface *rt;
    IDirectDraw *ddraw;
    UINT inst_length;
    D3DCOLOR color;
    HWND window;
    HRESULT hr;
    UINT x, y;
    UINT i, j;
    void *ptr;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    if (!(device = create_device(ddraw, window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        IDirectDraw_Release(ddraw);
        DestroyWindow(window);
        return;
    }

    background = create_diffuse_material(device, 1.0f, 0.0f, 0.0f, 1.0f);
    viewport = create_viewport(device, 0, 0, 640, 480);
    viewport_set_background(device, viewport, background);

    memset(&exec_desc, 0, sizeof(exec_desc));
    exec_desc.dwSize = sizeof(exec_desc);
    exec_desc.dwFlags = D3DDEB_BUFSIZE | D3DDEB_CAPS;
    exec_desc.dwBufferSize = 1024;
    exec_desc.dwCaps = D3DDEBCAPS_SYSTEMMEMORY;

    hr = IDirect3DDevice_CreateExecuteBuffer(device, &exec_desc, &execute_buffer, NULL);
    ok(SUCCEEDED(hr), "Failed to create execute buffer, hr %#x.\n", hr);
    hr = IDirect3DExecuteBuffer_Lock(execute_buffer, &exec_desc);
    ok(SUCCEEDED(hr), "Failed to lock execute buffer, hr %#x.\n", hr);
    memcpy(exec_desc.lpData, tquad, sizeof(tquad));
    ptr = ((BYTE *)exec_desc.lpData) + sizeof(tquad);
    emit_process_vertices(&ptr, D3DPROCESSVERTICES_COPY, 0, 4);
    emit_set_rs(&ptr, D3DRENDERSTATE_ZENABLE, D3DZB_FALSE);
    emit_tquad(&ptr, 0);
    emit_end(&ptr);
    inst_length = (BYTE *)ptr - (BYTE *)exec_desc.lpData;
    inst_length -= sizeof(tquad);
    hr = IDirect3DExecuteBuffer_Unlock(execute_buffer);
    ok(SUCCEEDED(hr), "Failed to unlock execute buffer, hr %#x.\n", hr);

    hr = IDirect3DViewport_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    hr = IDirect3DDevice_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    set_execute_data(execute_buffer, 4, sizeof(tquad), inst_length);
    hr = IDirect3DDevice_Execute(device, execute_buffer, viewport, D3DEXECUTE_CLIPPED);
    ok(SUCCEEDED(hr), "Failed to execute exec buffer, hr %#x.\n", hr);
    hr = IDirect3DDevice_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    hr = IDirect3DDevice_QueryInterface(device, &IID_IDirectDrawSurface, (void **)&rt);
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
    IDirectDrawSurface_Release(rt);

    destroy_viewport(device, viewport);
    IDirect3DExecuteBuffer_Release(execute_buffer);
    destroy_material(background);
    IDirect3DDevice_Release(device);
    IDirectDraw_Release(ddraw);
    DestroyWindow(window);
}

static void test_ck_rgba(void)
{
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    static D3DTLVERTEX tquad[] =
    {
        {{  0.0f}, {480.0f}, {0.25f}, {1.0f}, {0xffffffff}, {0x00000000}, {0.0f}, {0.0f}},
        {{  0.0f}, {  0.0f}, {0.25f}, {1.0f}, {0xffffffff}, {0x00000000}, {0.0f}, {1.0f}},
        {{640.0f}, {480.0f}, {0.25f}, {1.0f}, {0xffffffff}, {0x00000000}, {1.0f}, {0.0f}},
        {{640.0f}, {  0.0f}, {0.25f}, {1.0f}, {0xffffffff}, {0x00000000}, {1.0f}, {1.0f}},
        {{  0.0f}, {480.0f}, {0.75f}, {1.0f}, {0xffffffff}, {0x00000000}, {0.0f}, {0.0f}},
        {{  0.0f}, {  0.0f}, {0.75f}, {1.0f}, {0xffffffff}, {0x00000000}, {0.0f}, {1.0f}},
        {{640.0f}, {480.0f}, {0.75f}, {1.0f}, {0xffffffff}, {0x00000000}, {1.0f}, {0.0f}},
        {{640.0f}, {  0.0f}, {0.75f}, {1.0f}, {0xffffffff}, {0x00000000}, {1.0f}, {1.0f}},
    };
    /* Supposedly there was no D3DRENDERSTATE_COLORKEYENABLE in D3D < 5.
     * Maybe the WARP driver on Windows 8 ignores setting it via the older
     * device interface but it's buggy in that the internal state is not
     * initialized, or possibly toggling D3DRENDERSTATE_COLORKEYENABLE /
     * D3DRENDERSTATE_ALPHABLENDENABLE has unintended side effects.
     * Checking the W8 test results it seems like test 1 fails most of the time
     * and test 0 fails very rarely. */
    static const struct
    {
        D3DCOLOR fill_color;
        BOOL color_key;
        BOOL blend;
        D3DCOLOR result1, result1_r200, result1_warp;
        D3DCOLOR result2, result2_r200, result2_warp;
    }
    tests[] =
    {
        /* r200 on Windows doesn't check the alpha component when applying the color
         * key, so the key matches on every texel. */
        {0xff00ff00, TRUE,  TRUE,  0x00ff0000, 0x00ff0000, 0x0000ff00,
                0x000000ff, 0x000000ff, 0x0000ff00},
        {0xff00ff00, TRUE,  FALSE, 0x00ff0000, 0x00ff0000, 0x0000ff00,
                0x000000ff, 0x000000ff, 0x0000ff00},
        {0xff00ff00, FALSE, TRUE,  0x0000ff00, 0x0000ff00, 0x0000ff00,
                0x0000ff00, 0x0000ff00, 0x0000ff00},
        {0xff00ff00, FALSE, FALSE, 0x0000ff00, 0x0000ff00, 0x0000ff00,
                0x0000ff00, 0x0000ff00, 0x0000ff00},
        {0x7f00ff00, TRUE,  TRUE,  0x00807f00, 0x00ff0000, 0x00807f00,
                0x00807f00, 0x000000ff, 0x00807f00},
        {0x7f00ff00, TRUE,  FALSE, 0x0000ff00, 0x00ff0000, 0x0000ff00,
                0x0000ff00, 0x000000ff, 0x0000ff00},
        {0x7f00ff00, FALSE, TRUE,  0x00807f00, 0x00807f00, 0x00807f00,
                0x00807f00, 0x00807f00, 0x00807f00},
        {0x7f00ff00, FALSE, FALSE, 0x0000ff00, 0x0000ff00, 0x0000ff00,
                0x0000ff00, 0x0000ff00, 0x0000ff00},
    };

    IDirect3DExecuteBuffer *execute_buffer;
    D3DTEXTUREHANDLE texture_handle;
    D3DEXECUTEBUFFERDESC exec_desc;
    IDirect3DMaterial *background;
    IDirectDrawSurface *surface;
    IDirect3DViewport *viewport;
    DDSURFACEDESC surface_desc;
    IDirect3DTexture *texture;
    IDirect3DDevice *device;
    IDirectDrawSurface *rt;
    IDirectDraw *ddraw;
    D3DCOLOR color;
    HWND window;
    DDBLTFX fx;
    HRESULT hr;
    UINT i;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    if (!(device = create_device(ddraw, window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        IDirectDraw_Release(ddraw);
        DestroyWindow(window);
        return;
    }

    background = create_diffuse_material(device, 1.0, 0.0f, 0.0f, 1.0f);
    viewport = create_viewport(device, 0, 0, 640, 480);
    viewport_set_background(device, viewport, background);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CKSRCBLT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    surface_desc.dwWidth = 256;
    surface_desc.dwHeight = 256;
    surface_desc.ddpfPixelFormat.dwSize = sizeof(surface_desc.ddpfPixelFormat);
    surface_desc.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    U1(surface_desc.ddpfPixelFormat).dwRGBBitCount = 32;
    U2(surface_desc.ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(surface_desc.ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(surface_desc.ddpfPixelFormat).dwBBitMask = 0x000000ff;
    U5(surface_desc.ddpfPixelFormat).dwRGBAlphaBitMask = 0xff000000;
    surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue = 0xff00ff00;
    surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue = 0xff00ff00;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create destination surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface_QueryInterface(surface, &IID_IDirect3DTexture, (void **)&texture);
    ok(SUCCEEDED(hr), "Failed to get texture interface, hr %#x.\n", hr);
    hr = IDirect3DTexture_GetHandle(texture, device, &texture_handle);
    ok(SUCCEEDED(hr), "Failed to get texture handle, hr %#x.\n", hr);
    IDirect3DTexture_Release(texture);

    memset(&exec_desc, 0, sizeof(exec_desc));
    exec_desc.dwSize = sizeof(exec_desc);
    exec_desc.dwFlags = D3DDEB_BUFSIZE | D3DDEB_CAPS;
    exec_desc.dwBufferSize = 1024;
    exec_desc.dwCaps = D3DDEBCAPS_SYSTEMMEMORY;
    hr = IDirect3DDevice_CreateExecuteBuffer(device, &exec_desc, &execute_buffer, NULL);
    ok(SUCCEEDED(hr), "Failed to create execute buffer, hr %#x.\n", hr);

    hr = IDirect3DDevice_QueryInterface(device, &IID_IDirectDrawSurface, (void **)&rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        UINT draw1_len, draw2_len;
        void *ptr;

        hr = IDirect3DExecuteBuffer_Lock(execute_buffer, &exec_desc);
        ok(SUCCEEDED(hr), "Failed to lock execute buffer, hr %#x.\n", hr);
        memcpy(exec_desc.lpData, tquad, sizeof(tquad));
        ptr = ((BYTE *)exec_desc.lpData) + sizeof(tquad);
        emit_process_vertices(&ptr, D3DPROCESSVERTICES_COPY, 0, 4);
        emit_set_rs(&ptr, D3DRENDERSTATE_TEXTUREHANDLE, texture_handle);
        emit_set_rs(&ptr, D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
        emit_set_rs(&ptr, D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
        emit_set_rs(&ptr, D3DRENDERSTATE_COLORKEYENABLE, tests[i].color_key);
        emit_set_rs(&ptr, D3DRENDERSTATE_ALPHABLENDENABLE, tests[i].blend);
        emit_tquad(&ptr, 0);
        emit_end(&ptr);
        draw1_len = (BYTE *)ptr - (BYTE *)exec_desc.lpData - sizeof(tquad);
        emit_process_vertices(&ptr, D3DPROCESSVERTICES_COPY, 4, 4);
        emit_tquad(&ptr, 0);
        emit_set_rs(&ptr, D3DRENDERSTATE_TEXTUREHANDLE, 0);
        emit_end(&ptr);
        draw2_len = (BYTE *)ptr - (BYTE *)exec_desc.lpData - draw1_len;
        hr = IDirect3DExecuteBuffer_Unlock(execute_buffer);
        ok(SUCCEEDED(hr), "Failed to unlock execute buffer, hr %#x.\n", hr);

        memset(&fx, 0, sizeof(fx));
        fx.dwSize = sizeof(fx);
        U5(fx).dwFillColor = tests[i].fill_color;
        hr = IDirectDrawSurface_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
        ok(SUCCEEDED(hr), "Failed to fill texture, hr %#x.\n", hr);

        hr = IDirect3DViewport_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);
        ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
        hr = IDirect3DDevice_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
        set_execute_data(execute_buffer, 8, sizeof(tquad), draw1_len);
        hr = IDirect3DDevice_Execute(device, execute_buffer, viewport, D3DEXECUTE_CLIPPED);
        ok(SUCCEEDED(hr), "Failed to execute exec buffer, hr %#x.\n", hr);
        hr = IDirect3DDevice_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

        color = get_surface_color(rt, 320, 240);
        ok(compare_color(color, tests[i].result1, 1)
                || broken(compare_color(color, tests[i].result1_r200, 1))
                || broken(compare_color(color, tests[i].result1_warp, 1)),
                "Got unexpected color 0x%08x for test %u.\n", color, i);

        U5(fx).dwFillColor = 0xff0000ff;
        hr = IDirectDrawSurface_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
        ok(SUCCEEDED(hr), "Failed to fill texture, hr %#x.\n", hr);

        hr = IDirect3DDevice_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
        set_execute_data(execute_buffer, 8, sizeof(tquad) + draw1_len, draw2_len);
        hr = IDirect3DDevice_Execute(device, execute_buffer, viewport, D3DEXECUTE_CLIPPED);
        ok(SUCCEEDED(hr), "Failed to execute exec buffer, hr %#x.\n", hr);
        hr = IDirect3DDevice_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

        /* This tests that fragments that are masked out by the color key are
         * discarded, instead of just fully transparent. */
        color = get_surface_color(rt, 320, 240);
        ok(compare_color(color, tests[i].result2, 1)
                || broken(compare_color(color, tests[i].result2_r200, 1))
                || broken(compare_color(color, tests[i].result2_warp, 1)),
                "Got unexpected color 0x%08x for test %u.\n", color, i);
    }

    IDirectDrawSurface_Release(rt);
    IDirect3DExecuteBuffer_Release(execute_buffer);
    IDirectDrawSurface_Release(surface);
    destroy_viewport(device, viewport);
    destroy_material(background);
    IDirect3DDevice_Release(device);
    IDirectDraw_Release(ddraw);
    DestroyWindow(window);
}

static void test_ck_default(void)
{
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    static D3DTLVERTEX tquad[] =
    {
        {{  0.0f}, {480.0f}, {0.0f}, {1.0f}, {0xffffffff}, {0x00000000}, {0.0f}, {0.0f}},
        {{  0.0f}, {  0.0f}, {0.0f}, {1.0f}, {0xffffffff}, {0x00000000}, {0.0f}, {1.0f}},
        {{640.0f}, {480.0f}, {0.0f}, {1.0f}, {0xffffffff}, {0x00000000}, {1.0f}, {0.0f}},
        {{640.0f}, {  0.0f}, {0.0f}, {1.0f}, {0xffffffff}, {0x00000000}, {1.0f}, {1.0f}},
    };
    IDirect3DExecuteBuffer *execute_buffer;
    IDirectDrawSurface *surface, *rt;
    D3DTEXTUREHANDLE texture_handle;
    D3DEXECUTEBUFFERDESC exec_desc;
    IDirect3DMaterial *background;
    UINT draw1_offset, draw1_len;
    UINT draw2_offset, draw2_len;
    UINT draw3_offset, draw3_len;
    UINT draw4_offset, draw4_len;
    IDirect3DViewport *viewport;
    DDSURFACEDESC surface_desc;
    IDirect3DTexture *texture;
    IDirect3DDevice *device;
    IDirectDraw *ddraw;
    D3DCOLOR color;
    HWND window;
    DDBLTFX fx;
    HRESULT hr;
    void *ptr;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    if (!(device = create_device(ddraw, window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        IDirectDraw_Release(ddraw);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice_QueryInterface(device, &IID_IDirectDrawSurface, (void **)&rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    background = create_diffuse_material(device, 0.0, 1.0f, 0.0f, 1.0f);
    viewport = create_viewport(device, 0, 0, 640, 480);
    viewport_set_background(device, viewport, background);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CKSRCBLT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    surface_desc.dwWidth = 256;
    surface_desc.dwHeight = 256;
    surface_desc.ddpfPixelFormat.dwSize = sizeof(surface_desc.ddpfPixelFormat);
    surface_desc.ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(surface_desc.ddpfPixelFormat).dwRGBBitCount = 32;
    U2(surface_desc.ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(surface_desc.ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(surface_desc.ddpfPixelFormat).dwBBitMask = 0x000000ff;
    surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue = 0x000000ff;
    surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue = 0x000000ff;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface_QueryInterface(surface, &IID_IDirect3DTexture, (void **)&texture);
    ok(SUCCEEDED(hr), "Failed to get texture interface, hr %#x.\n", hr);
    hr = IDirect3DTexture_GetHandle(texture, device, &texture_handle);
    ok(SUCCEEDED(hr), "Failed to get texture handle, hr %#x.\n", hr);
    IDirect3DTexture_Release(texture);

    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    U5(fx).dwFillColor = 0x000000ff;
    hr = IDirectDrawSurface_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to fill surface, hr %#x.\n", hr);

    memset(&exec_desc, 0, sizeof(exec_desc));
    exec_desc.dwSize = sizeof(exec_desc);
    exec_desc.dwFlags = D3DDEB_BUFSIZE | D3DDEB_CAPS;
    exec_desc.dwBufferSize = 1024;
    exec_desc.dwCaps = D3DDEBCAPS_SYSTEMMEMORY;
    hr = IDirect3DDevice_CreateExecuteBuffer(device, &exec_desc, &execute_buffer, NULL);
    ok(SUCCEEDED(hr), "Failed to create execute buffer, hr %#x.\n", hr);

    hr = IDirect3DExecuteBuffer_Lock(execute_buffer, &exec_desc);
    ok(SUCCEEDED(hr), "Failed to lock execute buffer, hr %#x.\n", hr);
    memcpy(exec_desc.lpData, tquad, sizeof(tquad));
    ptr = (BYTE *)exec_desc.lpData + sizeof(tquad);
    emit_process_vertices(&ptr, D3DPROCESSVERTICES_COPY, 0, 4);
    emit_set_rs(&ptr, D3DRENDERSTATE_TEXTUREHANDLE, texture_handle);
    emit_tquad(&ptr, 0);
    emit_end(&ptr);
    draw1_offset = sizeof(tquad);
    draw1_len = (BYTE *)ptr - (BYTE *)exec_desc.lpData - draw1_offset;
    emit_process_vertices(&ptr, D3DPROCESSVERTICES_COPY, 0, 4);
    emit_set_rs(&ptr, D3DRENDERSTATE_COLORKEYENABLE, FALSE);
    emit_tquad(&ptr, 0);
    emit_end(&ptr);
    draw2_offset = draw1_offset + draw1_len;
    draw2_len = (BYTE *)ptr - (BYTE *)exec_desc.lpData - draw2_offset;
    emit_process_vertices(&ptr, D3DPROCESSVERTICES_COPY, 0, 4);
    emit_tquad(&ptr, 0);
    emit_end(&ptr);
    draw3_offset = draw2_offset + draw2_len;
    draw3_len = (BYTE *)ptr - (BYTE *)exec_desc.lpData - draw3_offset;
    emit_process_vertices(&ptr, D3DPROCESSVERTICES_COPY, 0, 4);
    emit_set_rs(&ptr, D3DRENDERSTATE_COLORKEYENABLE, TRUE);
    emit_tquad(&ptr, 0);
    emit_set_rs(&ptr, D3DRENDERSTATE_TEXTUREHANDLE, 0);
    emit_end(&ptr);
    draw4_offset = draw3_offset + draw3_len;
    draw4_len = (BYTE *)ptr - (BYTE *)exec_desc.lpData - draw4_offset;
    hr = IDirect3DExecuteBuffer_Unlock(execute_buffer);
    ok(SUCCEEDED(hr), "Failed to unlock execute buffer, hr %#x.\n", hr);

    hr = IDirect3DViewport_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    hr = IDirect3DDevice_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    set_execute_data(execute_buffer, 4, draw1_offset, draw1_len);
    hr = IDirect3DDevice_Execute(device, execute_buffer, viewport, D3DEXECUTE_CLIPPED);
    ok(SUCCEEDED(hr), "Failed to execute exec buffer, hr %#x.\n", hr);
    hr = IDirect3DDevice_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);
    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x0000ff00, 1), "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DViewport_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    hr = IDirect3DDevice_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    set_execute_data(execute_buffer, 4, draw2_offset, draw2_len);
    hr = IDirect3DDevice_Execute(device, execute_buffer, viewport, D3DEXECUTE_CLIPPED);
    ok(SUCCEEDED(hr), "Failed to execute exec buffer, hr %#x.\n", hr);
    hr = IDirect3DDevice_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);
    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x000000ff, 1), "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DViewport_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    hr = IDirect3DDevice_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    set_execute_data(execute_buffer, 4, draw3_offset, draw3_len);
    hr = IDirect3DDevice_Execute(device, execute_buffer, viewport, D3DEXECUTE_CLIPPED);
    ok(SUCCEEDED(hr), "Failed to execute exec buffer, hr %#x.\n", hr);
    hr = IDirect3DDevice_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);
    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x000000ff, 1), "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DViewport_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    hr = IDirect3DDevice_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    set_execute_data(execute_buffer, 4, draw4_offset, draw4_len);
    hr = IDirect3DDevice_Execute(device, execute_buffer, viewport, D3DEXECUTE_CLIPPED);
    ok(SUCCEEDED(hr), "Failed to execute exec buffer, hr %#x.\n", hr);
    hr = IDirect3DDevice_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);
    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x0000ff00, 1), "Got unexpected color 0x%08x.\n", color);

    IDirect3DExecuteBuffer_Release(execute_buffer);
    IDirectDrawSurface_Release(surface);
    destroy_viewport(device, viewport);
    destroy_material(background);
    IDirectDrawSurface_Release(rt);
    IDirect3DDevice_Release(device);
    IDirectDraw_Release(ddraw);
    DestroyWindow(window);
}

static void test_ck_complex(void)
{
    IDirectDrawSurface *surface, *mipmap, *tmp;
    DDSCAPS caps = {DDSCAPS_COMPLEX};
    DDSURFACEDESC surface_desc;
    IDirect3DDevice *device;
    DDCOLORKEY color_key;
    IDirectDraw *ddraw;
    unsigned int i;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    if (!(device = create_device(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        IDirectDraw2_Release(ddraw);
        return;
    }
    IDirect3DDevice_Release(device);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
    surface_desc.dwWidth = 128;
    surface_desc.dwHeight = 128;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface_GetColorKey(surface, DDCKEY_SRCBLT, &color_key);
    ok(hr == DDERR_NOCOLORKEY, "Got unexpected hr %#x.\n", hr);
    color_key.dwColorSpaceLowValue = 0x0000ff00;
    color_key.dwColorSpaceHighValue = 0x0000ff00;
    hr = IDirectDrawSurface_SetColorKey(surface, DDCKEY_SRCBLT, &color_key);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);
    memset(&color_key, 0, sizeof(color_key));
    hr = IDirectDrawSurface_GetColorKey(surface, DDCKEY_SRCBLT, &color_key);
    ok(SUCCEEDED(hr), "Failed to get color key, hr %#x.\n", hr);
    ok(color_key.dwColorSpaceLowValue == 0x0000ff00, "Got unexpected value 0x%08x.\n",
            color_key.dwColorSpaceLowValue);
    ok(color_key.dwColorSpaceHighValue == 0x0000ff00, "Got unexpected value 0x%08x.\n",
            color_key.dwColorSpaceHighValue);

    mipmap = surface;
    IDirectDrawSurface_AddRef(mipmap);
    for (i = 0; i < 7; ++i)
    {
        hr = IDirectDrawSurface_GetAttachedSurface(mipmap, &caps, &tmp);
        ok(SUCCEEDED(hr), "Failed to get attached surface, i %u, hr %#x.\n", i, hr);

        hr = IDirectDrawSurface_GetColorKey(tmp, DDCKEY_SRCBLT, &color_key);
        ok(hr == DDERR_NOCOLORKEY, "Got unexpected hr %#x, i %u.\n", hr, i);
        color_key.dwColorSpaceLowValue = 0x000000ff;
        color_key.dwColorSpaceHighValue = 0x000000ff;
        hr = IDirectDrawSurface_SetColorKey(tmp, DDCKEY_SRCBLT, &color_key);
        ok(SUCCEEDED(hr), "Failed to set color key, hr %#x, i %u.\n", hr, i);
        memset(&color_key, 0, sizeof(color_key));
        hr = IDirectDrawSurface_GetColorKey(tmp, DDCKEY_SRCBLT, &color_key);
        ok(SUCCEEDED(hr), "Failed to get color key, hr %#x, i %u.\n", hr, i);
        ok(color_key.dwColorSpaceLowValue == 0x000000ff, "Got unexpected value 0x%08x, i %u.\n",
                color_key.dwColorSpaceLowValue, i);
        ok(color_key.dwColorSpaceHighValue == 0x000000ff, "Got unexpected value 0x%08x, i %u.\n",
                color_key.dwColorSpaceHighValue, i);

        IDirectDrawSurface_Release(mipmap);
        mipmap = tmp;
    }

    memset(&color_key, 0, sizeof(color_key));
    hr = IDirectDrawSurface_GetColorKey(surface, DDCKEY_SRCBLT, &color_key);
    ok(SUCCEEDED(hr), "Failed to get color key, hr %#x.\n", hr);
    ok(color_key.dwColorSpaceLowValue == 0x0000ff00, "Got unexpected value 0x%08x.\n",
            color_key.dwColorSpaceLowValue);
    ok(color_key.dwColorSpaceHighValue == 0x0000ff00, "Got unexpected value 0x%08x.\n",
            color_key.dwColorSpaceHighValue);

    hr = IDirectDrawSurface_GetAttachedSurface(mipmap, &caps, &tmp);
    ok(hr == DDERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);
    IDirectDrawSurface_Release(mipmap);
    refcount = IDirectDrawSurface_Release(surface);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;
    surface_desc.dwBackBufferCount = 1;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface_GetColorKey(surface, DDCKEY_SRCBLT, &color_key);
    ok(hr == DDERR_NOCOLORKEY, "Got unexpected hr %#x.\n", hr);
    color_key.dwColorSpaceLowValue = 0x0000ff00;
    color_key.dwColorSpaceHighValue = 0x0000ff00;
    hr = IDirectDrawSurface_SetColorKey(surface, DDCKEY_SRCBLT, &color_key);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);
    memset(&color_key, 0, sizeof(color_key));
    hr = IDirectDrawSurface_GetColorKey(surface, DDCKEY_SRCBLT, &color_key);
    ok(SUCCEEDED(hr), "Failed to get color key, hr %#x.\n", hr);
    ok(color_key.dwColorSpaceLowValue == 0x0000ff00, "Got unexpected value 0x%08x.\n",
            color_key.dwColorSpaceLowValue);
    ok(color_key.dwColorSpaceHighValue == 0x0000ff00, "Got unexpected value 0x%08x.\n",
            color_key.dwColorSpaceHighValue);

    hr = IDirectDrawSurface_GetAttachedSurface(surface, &caps, &tmp);
    ok(SUCCEEDED(hr), "Failed to get attached surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface_GetColorKey(tmp, DDCKEY_SRCBLT, &color_key);
    ok(hr == DDERR_NOCOLORKEY, "Got unexpected hr %#x, i %u.\n", hr, i);
    color_key.dwColorSpaceLowValue = 0x0000ff00;
    color_key.dwColorSpaceHighValue = 0x0000ff00;
    hr = IDirectDrawSurface_SetColorKey(tmp, DDCKEY_SRCBLT, &color_key);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);
    memset(&color_key, 0, sizeof(color_key));
    hr = IDirectDrawSurface_GetColorKey(tmp, DDCKEY_SRCBLT, &color_key);
    ok(SUCCEEDED(hr), "Failed to get color key, hr %#x.\n", hr);
    ok(color_key.dwColorSpaceLowValue == 0x0000ff00, "Got unexpected value 0x%08x.\n",
            color_key.dwColorSpaceLowValue);
    ok(color_key.dwColorSpaceHighValue == 0x0000ff00, "Got unexpected value 0x%08x.\n",
            color_key.dwColorSpaceHighValue);

    IDirectDrawSurface_Release(tmp);

    refcount = IDirectDrawSurface_Release(surface);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDraw_Release(ddraw);
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
        {&IID_IDirect3DTexture2,        &IID_IDirectDrawSurface,        S_OK         },
        {&IID_IDirect3DTexture,         &IID_IDirectDrawSurface,        S_OK         },
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

    IDirectDrawSurface *surface;
    DDSURFACEDESC surface_desc;
    IDirect3DDevice *device;
    IDirectDraw *ddraw;
    HWND window;
    HRESULT hr;

    if (!GetProcAddress(GetModuleHandleA("ddraw.dll"), "DirectDrawCreateEx"))
    {
        win_skip("DirectDrawCreateEx not available, skipping test.\n");
        return;
    }

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    /* Try to create a D3D device to see if the ddraw implementation supports
     * D3D. 64-bit ddraw in particular doesn't seem to support D3D, and
     * doesn't support e.g. the IDirect3DTexture interfaces. */
    if (!(device = create_device(ddraw, window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        IDirectDraw_Release(ddraw);
        DestroyWindow(window);
        return;
    }
    IDirect3DDevice_Release(device);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    surface_desc.dwWidth = 512;
    surface_desc.dwHeight = 512;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, (IDirectDrawSurface **)0xdeadbeef, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    test_qi("surface_qi", (IUnknown *)surface, &IID_IDirectDrawSurface, tests, ARRAY_SIZE(tests));

    IDirectDrawSurface_Release(surface);
    IDirectDraw_Release(ddraw);
    DestroyWindow(window);
}

static void test_device_qi(void)
{
    static const struct qi_test tests[] =
    {
        {&IID_IDirect3DTexture2,        &IID_IDirectDrawSurface,        S_OK         },
        {&IID_IDirect3DTexture,         &IID_IDirectDrawSurface,        S_OK         },
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
        {&IID_IDirect3DHALDevice,       &IID_IDirectDrawSurface,        S_OK         },
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
    };


    IDirect3DDevice *device;
    IDirectDraw *ddraw;
    HWND window;

    if (!GetProcAddress(GetModuleHandleA("ddraw.dll"), "DirectDrawCreateEx"))
    {
        win_skip("DirectDrawCreateEx not available, skipping test.\n");
        return;
    }

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    if (!(device = create_device(ddraw, window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        IDirectDraw_Release(ddraw);
        DestroyWindow(window);
        return;
    }

    test_qi("device_qi", (IUnknown *)device, &IID_IDirectDrawSurface, tests, ARRAY_SIZE(tests));

    IDirect3DDevice_Release(device);
    IDirectDraw_Release(ddraw);
    DestroyWindow(window);
}

static void test_wndproc(void)
{
    LONG_PTR proc, ddraw_proc;
    IDirectDraw *ddraw;
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
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    ok(!expect_messages->message, "Expected message %#x, but didn't receive it.\n", expect_messages->message);
    expect_messages = NULL;
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    ref = IDirectDraw_Release(ddraw);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    /* DDSCL_NORMAL doesn't. */
    ddraw = create_ddraw();
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    ref = IDirectDraw_Release(ddraw);
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
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    ddraw_proc = proc;
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    proc = SetWindowLongPtrA(window, GWLP_WNDPROC, (LONG_PTR)DefWindowProcA);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)DefWindowProcA, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)DefWindowProcA, proc);
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    proc = SetWindowLongPtrA(window, GWLP_WNDPROC, (LONG_PTR)ddraw_proc);
    ok(proc == (LONG_PTR)DefWindowProcA, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)DefWindowProcA, proc);
    ref = IDirectDraw_Release(ddraw);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    ddraw = create_ddraw();
    proc = GetWindowLongPtrA(window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    proc = SetWindowLongPtrA(window, GWLP_WNDPROC, (LONG_PTR)DefWindowProcA);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    ref = IDirectDraw_Release(ddraw);
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
    IDirectDraw *ddraw;
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

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
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

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    tmp = GetWindowLongA(window, GWL_STYLE);
    todo_wine ok(tmp == style, "Expected window style %#x, got %#x.\n", style, tmp);
    tmp = GetWindowLongA(window, GWL_EXSTYLE);
    todo_wine ok(tmp == exstyle, "Expected window extended style %#x, got %#x.\n", exstyle, tmp);

    ShowWindow(window, SW_SHOW);
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
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

    ref = IDirectDraw_Release(ddraw);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);

    DestroyWindow(window);
}

static void test_redundant_mode_set(void)
{
    DDSURFACEDESC surface_desc = {0};
    IDirectDraw *ddraw;
    RECT q, r, s;
    HWND window;
    HRESULT hr;
    ULONG ref;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 100, 100, 0, 0, 0, 0);
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDraw_GetDisplayMode(ddraw, &surface_desc);
    ok(SUCCEEDED(hr), "GetDisplayMode failed, hr %#x.\n", hr);

    hr = IDirectDraw_SetDisplayMode(ddraw, surface_desc.dwWidth, surface_desc.dwHeight,
            U1(surface_desc.ddpfPixelFormat).dwRGBBitCount);
    ok(SUCCEEDED(hr), "SetDisplayMode failed, hr %#x.\n", hr);

    GetWindowRect(window, &q);
    r = q;
    r.right /= 2;
    r.bottom /= 2;
    SetWindowPos(window, HWND_TOP, r.left, r.top, r.right, r.bottom, 0);
    GetWindowRect(window, &s);
    ok(EqualRect(&r, &s), "Expected %s, got %s.\n", wine_dbgstr_rect(&r), wine_dbgstr_rect(&s));

    hr = IDirectDraw_SetDisplayMode(ddraw, surface_desc.dwWidth, surface_desc.dwHeight,
            U1(surface_desc.ddpfPixelFormat).dwRGBBitCount);
    ok(SUCCEEDED(hr), "SetDisplayMode failed, hr %#x.\n", hr);

    GetWindowRect(window, &s);
    ok(EqualRect(&r, &s) || broken(EqualRect(&q, &s) /* Windows 10 */),
            "Expected %s, got %s.\n", wine_dbgstr_rect(&r), wine_dbgstr_rect(&s));

    ref = IDirectDraw_Release(ddraw);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);

    DestroyWindow(window);
}

static SIZE screen_size;

static LRESULT CALLBACK mode_set_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (message == WM_SIZE)
    {
        screen_size.cx = GetSystemMetrics(SM_CXSCREEN);
        screen_size.cy = GetSystemMetrics(SM_CYSCREEN);
    }

    return test_proc(hwnd, message, wparam, lparam);
}

struct test_coop_level_mode_set_enum_param
{
    DWORD ddraw_width, ddraw_height, user32_width, user32_height;
};

static HRESULT CALLBACK test_coop_level_mode_set_enum_cb(DDSURFACEDESC *surface_desc, void *context)
{
    struct test_coop_level_mode_set_enum_param *param = context;

    if (U1(surface_desc->ddpfPixelFormat).dwRGBBitCount != registry_mode.dmBitsPerPel)
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
    IDirectDrawSurface *primary;
    RECT registry_rect, ddraw_rect, user32_rect, r;
    IDirectDraw *ddraw;
    DDSURFACEDESC ddsd;
    WNDCLASSA wc = {0};
    HWND window;
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
    hr = IDirectDraw_EnumDisplayModes(ddraw, 0, NULL, &param, test_coop_level_mode_set_enum_cb);
    ok(SUCCEEDED(hr), "Failed to enumerate display mode, hr %#x.\n", hr);
    ref = IDirectDraw_Release(ddraw);
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

    window = CreateWindowA("ddraw_test_wndproc_wc", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 100, 100, 0, 0, 0, 0);

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &user32_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&user32_rect),
            wine_dbgstr_rect(&r));

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface_GetSurfaceDesc(primary, &ddsd);
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

    hr = IDirectDrawSurface_IsLost(primary);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = set_display_mode(ddraw, param.ddraw_width, param.ddraw_height);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    hr = IDirectDrawSurface_IsLost(primary);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);

    ok(!expect_messages->message, "Expected message %#x, but didn't receive it.\n", expect_messages->message);
    expect_messages = NULL;
    ok(screen_size.cx == param.ddraw_width && screen_size.cy == param.ddraw_height,
            "Expected screen size %ux%u, got %ux%u.\n",
            param.ddraw_width, param.ddraw_height, screen_size.cx, screen_size.cy);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &ddraw_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&ddraw_rect),
            wine_dbgstr_rect(&r));

    hr = IDirectDrawSurface_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == param.user32_width, "Expected surface width %u, got %u.\n",
            param.user32_width, ddsd.dwWidth);
    ok(ddsd.dwHeight == param.user32_height, "Expected surface height %u, got %u.\n",
            param.user32_height, ddsd.dwHeight);
    IDirectDrawSurface_Release(primary);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface_GetSurfaceDesc(primary, &ddsd);
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

    hr = IDirectDrawSurface_IsLost(primary);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    change_ret = ChangeDisplaySettingsW(&devmode, CDS_FULLSCREEN);
    ok(change_ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#x.\n", change_ret);
    hr = IDirectDrawSurface_IsLost(primary);
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

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
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

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE);
    expect_messages = exclusive_messages;
    screen_size.cx = 0;
    screen_size.cy = 0;

    hr = IDirectDrawSurface_IsLost(primary);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDraw_RestoreDisplayMode(ddraw);
    ok(SUCCEEDED(hr), "RestoreDisplayMode failed, hr %#x.\n", hr);
    hr = IDirectDrawSurface_IsLost(primary);
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

    hr = IDirectDrawSurface_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == param.ddraw_width, "Expected surface width %u, got %u.\n",
            param.ddraw_width, ddsd.dwWidth);
    ok(ddsd.dwHeight == param.ddraw_height, "Expected surface height %u, got %u.\n",
            param.ddraw_height, ddsd.dwHeight);
    IDirectDrawSurface_Release(primary);

    /* For Wine. */
    change_ret = ChangeDisplaySettingsW(NULL, CDS_FULLSCREEN);
    ok(change_ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#x.\n", change_ret);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == registry_mode.dmPelsWidth, "Expected surface width %u, got %u.\n",
            registry_mode.dmPelsWidth, ddsd.dwWidth);
    ok(ddsd.dwHeight == registry_mode.dmPelsHeight, "Expected surface height %u, got %u.\n",
            registry_mode.dmPelsHeight, ddsd.dwHeight);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &registry_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&registry_rect),
            wine_dbgstr_rect(&r));

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &registry_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&registry_rect),
            wine_dbgstr_rect(&r));

    hr = IDirectDrawSurface_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == registry_mode.dmPelsWidth, "Expected surface width %u, got %u.\n",
            registry_mode.dmPelsWidth, ddsd.dwWidth);
    ok(ddsd.dwHeight == registry_mode.dmPelsHeight, "Expected surface height %u, got %u.\n",
            registry_mode.dmPelsHeight, ddsd.dwHeight);
    IDirectDrawSurface_Release(primary);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface_GetSurfaceDesc(primary, &ddsd);
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

    hr = IDirectDrawSurface_IsLost(primary);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
    devmode.dmPelsWidth = param.user32_width;
    devmode.dmPelsHeight = param.user32_height;
    change_ret = ChangeDisplaySettingsW(&devmode, CDS_FULLSCREEN);
    ok(change_ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#x.\n", change_ret);
    hr = IDirectDrawSurface_IsLost(primary);
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

    hr = IDirectDrawSurface_Restore(primary);
    ok(hr == DDERR_WRONGMODE, "Got unexpected hr %#x.\n", hr);
    hr = set_display_mode(ddraw, param.ddraw_width, param.ddraw_height);
    if (hr == DDERR_NOEXCLUSIVEMODE /* NT4 testbot */)
    {
        win_skip("Broken SetDisplayMode(), skipping remaining tests.\n");
        IDirectDrawSurface_Release(primary);
        IDirectDraw_Release(ddraw);
        goto done;
    }
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    hr = IDirectDrawSurface_Restore(primary);
    ok(hr == DDERR_WRONGMODE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_IsLost(primary);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);

    ok(!expect_messages->message, "Expected message %#x, but didn't receive it.\n", expect_messages->message);
    expect_messages = NULL;
    ok(!screen_size.cx && !screen_size.cy, "Got unexpected screen size %ux%u.\n", screen_size.cx, screen_size.cy);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &registry_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&registry_rect),
            wine_dbgstr_rect(&r));

    hr = IDirectDrawSurface_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == registry_mode.dmPelsWidth, "Expected surface width %u, got %u.\n",
            registry_mode.dmPelsWidth, ddsd.dwWidth);
    ok(ddsd.dwHeight == registry_mode.dmPelsHeight, "Expected surface height %u, got %u.\n",
            registry_mode.dmPelsHeight, ddsd.dwHeight);
    IDirectDrawSurface_Release(primary);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface_GetSurfaceDesc(primary, &ddsd);
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

    hr = IDirectDrawSurface_IsLost(primary);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDraw_RestoreDisplayMode(ddraw);
    ok(SUCCEEDED(hr), "RestoreDisplayMode failed, hr %#x.\n", hr);
    hr = IDirectDrawSurface_IsLost(primary);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);

    ok(!expect_messages->message, "Expected message %#x, but didn't receive it.\n", expect_messages->message);
    expect_messages = NULL;
    ok(!screen_size.cx && !screen_size.cy, "Got unexpected screen size %ux%u.\n", screen_size.cx, screen_size.cy);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &registry_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&registry_rect),
            wine_dbgstr_rect(&r));

    hr = IDirectDrawSurface_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == param.ddraw_width, "Expected surface width %u, got %u.\n",
            param.ddraw_width, ddsd.dwWidth);
    ok(ddsd.dwHeight == param.ddraw_height, "Expected surface height %u, got %u.\n",
            param.ddraw_height, ddsd.dwHeight);
    IDirectDrawSurface_Release(primary);

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

    hr = IDirectDraw_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface_GetSurfaceDesc(primary, &ddsd);
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
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &registry_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&registry_rect),
            wine_dbgstr_rect(&r));

    hr = IDirectDrawSurface_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == registry_mode.dmPelsWidth, "Expected surface width %u, got %u.\n",
            registry_mode.dmPelsWidth, ddsd.dwWidth);
    ok(ddsd.dwHeight == registry_mode.dmPelsHeight, "Expected surface height %u, got %u.\n",
            registry_mode.dmPelsHeight, ddsd.dwHeight);
    IDirectDrawSurface_Release(primary);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface_GetSurfaceDesc(primary, &ddsd);
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

    hr = IDirectDrawSurface_IsLost(primary);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
    devmode.dmPelsWidth = param.user32_width;
    devmode.dmPelsHeight = param.user32_height;
    change_ret = ChangeDisplaySettingsW(&devmode, CDS_FULLSCREEN);
    ok(change_ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#x.\n", change_ret);
    hr = IDirectDrawSurface_IsLost(primary);
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

    hr = IDirectDrawSurface_Restore(primary);
    ok(hr == DDERR_WRONGMODE, "Got unexpected hr %#x.\n", hr);
    hr = set_display_mode(ddraw, param.ddraw_width, param.ddraw_height);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    hr = IDirectDrawSurface_Restore(primary);
    ok(hr == DDERR_WRONGMODE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_IsLost(primary);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);

    ok(!expect_messages->message, "Expected message %#x, but didn't receive it.\n", expect_messages->message);
    expect_messages = NULL;
    ok(!screen_size.cx && !screen_size.cy, "Got unexpected screen size %ux%u.\n", screen_size.cx, screen_size.cy);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &registry_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&registry_rect),
            wine_dbgstr_rect(&r));

    hr = IDirectDrawSurface_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == registry_mode.dmPelsWidth, "Expected surface width %u, got %u.\n",
            registry_mode.dmPelsWidth, ddsd.dwWidth);
    ok(ddsd.dwHeight == registry_mode.dmPelsHeight, "Expected surface height %u, got %u.\n",
            registry_mode.dmPelsHeight, ddsd.dwHeight);
    IDirectDrawSurface_Release(primary);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface_GetSurfaceDesc(primary, &ddsd);
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

    hr = IDirectDrawSurface_IsLost(primary);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDraw_RestoreDisplayMode(ddraw);
    ok(SUCCEEDED(hr), "RestoreDisplayMode failed, hr %#x.\n", hr);
    hr = IDirectDrawSurface_IsLost(primary);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);

    ok(!expect_messages->message, "Expected message %#x, but didn't receive it.\n", expect_messages->message);
    expect_messages = NULL;
    ok(!screen_size.cx && !screen_size.cy, "Got unexpected screen size %ux%u.\n", screen_size.cx, screen_size.cy);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &registry_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&registry_rect),
            wine_dbgstr_rect(&r));

    hr = IDirectDrawSurface_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == param.ddraw_width, "Expected surface width %u, got %u.\n",
            param.ddraw_width, ddsd.dwWidth);
    ok(ddsd.dwHeight == param.ddraw_height, "Expected surface height %u, got %u.\n",
            param.ddraw_height, ddsd.dwHeight);
    IDirectDrawSurface_Release(primary);

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

    hr = IDirectDraw_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == registry_mode.dmPelsWidth, "Expected surface width %u, got %u.\n",
            registry_mode.dmPelsWidth, ddsd.dwWidth);
    ok(ddsd.dwHeight == registry_mode.dmPelsHeight, "Expected surface height %u, got %u.\n",
            registry_mode.dmPelsHeight, ddsd.dwHeight);
    IDirectDrawSurface_Release(primary);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &registry_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&registry_rect),
            wine_dbgstr_rect(&r));

    /* Unlike ddraw2-7, changing from EXCLUSIVE to NORMAL does not restore the resolution */
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);
    hr = set_display_mode(ddraw, param.ddraw_width, param.ddraw_height);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    hr = IDirectDraw_CreateSurface(ddraw, &ddsd, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);
    hr = IDirectDrawSurface_GetSurfaceDesc(primary, &ddsd);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(ddsd.dwWidth == param.ddraw_width, "Expected surface width %u, got %u.\n",
            param.ddraw_width, ddsd.dwWidth);
    ok(ddsd.dwHeight == param.ddraw_height, "Expected surface height %u, got %u.\n",
            param.ddraw_height, ddsd.dwHeight);
    IDirectDrawSurface_Release(primary);
    hr = IDirectDraw_RestoreDisplayMode(ddraw);
    ok(SUCCEEDED(hr), "RestoreDisplayMode failed, hr %#x.\n", hr);

    ref = IDirectDraw_Release(ddraw);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);

    GetWindowRect(window, &r);
    ok(EqualRect(&r, &ddraw_rect), "Expected %s, got %s.\n", wine_dbgstr_rect(&ddraw_rect),
            wine_dbgstr_rect(&r));

done:
    expect_messages = NULL;
    DestroyWindow(window);
    UnregisterClassA("ddraw_test_wndproc_wc", GetModuleHandleA(NULL));
}

static void test_coop_level_mode_set_multi(void)
{
    IDirectDraw *ddraw1, *ddraw2;
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
    if (hr == DDERR_NOEXCLUSIVEMODE /* NT4 testbot */)
    {
        win_skip("Broken SetDisplayMode(), skipping test.\n");
        IDirectDraw_Release(ddraw1);
        DestroyWindow(window);
        return;
    }
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == 800, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == 600, "Got unexpected screen height %u.\n", h);

    ref = IDirectDraw_Release(ddraw1);
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

    ref = IDirectDraw_Release(ddraw2);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == registry_mode.dmPelsWidth, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == registry_mode.dmPelsHeight, "Got unexpected screen height %u.\n", h);

    ref = IDirectDraw_Release(ddraw1);
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

    ref = IDirectDraw_Release(ddraw1);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == registry_mode.dmPelsWidth, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == registry_mode.dmPelsHeight, "Got unexpected screen height %u.\n", h);

    ref = IDirectDraw_Release(ddraw2);
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

    ref = IDirectDraw_Release(ddraw1);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == 640, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == 480, "Got unexpected screen height %u.\n", h);

    ref = IDirectDraw_Release(ddraw2);
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

    hr = IDirectDraw_SetCooperativeLevel(ddraw2, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    ref = IDirectDraw_Release(ddraw1);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == 640, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == 480, "Got unexpected screen height %u.\n", h);

    ref = IDirectDraw_Release(ddraw2);
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

    hr = IDirectDraw_SetCooperativeLevel(ddraw1, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "SetCooperativeLevel failed, hr %#x.\n", hr);

    ddraw2 = create_ddraw();
    hr = set_display_mode(ddraw2, 640, 480);
    ok(hr == DDERR_NOEXCLUSIVEMODE, "Got unexpected hr %#x.\n", hr);

    ref = IDirectDraw_Release(ddraw1);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == registry_mode.dmPelsWidth, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == registry_mode.dmPelsHeight, "Got unexpected screen height %u.\n", h);

    ref = IDirectDraw_Release(ddraw2);
    ok(ref == 0, "The ddraw object was not properly freed: refcount %u.\n", ref);
    w = GetSystemMetrics(SM_CXSCREEN);
    ok(w == registry_mode.dmPelsWidth, "Got unexpected screen width %u.\n", w);
    h = GetSystemMetrics(SM_CYSCREEN);
    ok(h == registry_mode.dmPelsHeight, "Got unexpected screen height %u.\n", h);

    DestroyWindow(window);
}

static void test_initialize(void)
{
    IDirectDraw *ddraw;
    IDirect3D *d3d;
    HRESULT hr;

    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    hr = IDirectDraw_Initialize(ddraw, NULL);
    ok(hr == DDERR_ALREADYINITIALIZED, "Initialize returned hr %#x.\n", hr);
    IDirectDraw_Release(ddraw);

    CoInitialize(NULL);
    hr = CoCreateInstance(&CLSID_DirectDraw, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectDraw, (void **)&ddraw);
    ok(SUCCEEDED(hr), "Failed to create IDirectDraw instance, hr %#x.\n", hr);
    hr = IDirectDraw_QueryInterface(ddraw, &IID_IDirect3D, (void **)&d3d);
    if (SUCCEEDED(hr))
    {
        /* IDirect3D_Initialize() just returns DDERR_ALREADYINITIALIZED. */
        hr = IDirect3D_Initialize(d3d, NULL);
        ok(hr == DDERR_ALREADYINITIALIZED, "Initialize returned hr %#x, expected DDERR_ALREADYINITIALIZED.\n", hr);
        IDirect3D_Release(d3d);
    }
    else
        skip("D3D interface is not available, skipping test.\n");
    hr = IDirectDraw_Initialize(ddraw, NULL);
    ok(hr == DD_OK, "Initialize returned hr %#x, expected DD_OK.\n", hr);
    hr = IDirectDraw_Initialize(ddraw, NULL);
    ok(hr == DDERR_ALREADYINITIALIZED, "Initialize returned hr %#x, expected DDERR_ALREADYINITIALIZED.\n", hr);
    IDirectDraw_Release(ddraw);
    CoUninitialize();

    if (0) /* This crashes on the W2KPROSP4 testbot. */
    {
        CoInitialize(NULL);
        hr = CoCreateInstance(&CLSID_DirectDraw, NULL, CLSCTX_INPROC_SERVER, &IID_IDirect3D, (void **)&d3d);
        ok(hr == E_NOINTERFACE, "CoCreateInstance returned hr %#x, expected E_NOINTERFACE.\n", hr);
        CoUninitialize();
    }
}

static void test_coop_level_surf_create(void)
{
    IDirectDrawSurface *surface;
    IDirectDraw *ddraw;
    DDSURFACEDESC ddsd;
    HRESULT hr;

    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    surface = (void *)0xdeadbeef;
    hr = IDirectDraw_CreateSurface(ddraw, &ddsd, &surface, NULL);
    ok(hr == DDERR_NOCOOPERATIVELEVELSET, "Surface creation returned hr %#x.\n", hr);
    ok(surface == (void *)0xdeadbeef, "Got unexpected surface %p.\n", surface);

    surface = (void *)0xdeadbeef;
    hr = IDirectDraw_CreateSurface(ddraw, NULL, &surface, NULL);
    ok(hr == DDERR_NOCOOPERATIVELEVELSET, "Surface creation returned hr %#x.\n", hr);
    ok(surface == (void *)0xdeadbeef, "Got unexpected surface %p.\n", surface);

    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    surface = (void *)0xdeadbeef;
    hr = IDirectDraw_CreateSurface(ddraw, NULL, &surface, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "Unexpected hr %#x.\n", hr);
    ok(surface == (void *)0xdeadbeef, "Got unexpected surface %p.\n", surface);

    IDirectDraw_Release(ddraw);
}

static void test_coop_level_multi_window(void)
{
    HWND window1, window2;
    IDirectDraw *ddraw;
    HRESULT hr;

    window1 = create_window();
    window2 = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window1, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window2, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    ok(IsWindow(window1), "Window 1 was destroyed.\n");
    ok(IsWindow(window2), "Window 2 was destroyed.\n");

    IDirectDraw_Release(ddraw);
    DestroyWindow(window2);
    DestroyWindow(window1);
}

static void test_clear_rect_count(void)
{
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    IDirect3DMaterial *white, *red, *green, *blue;
    IDirect3DViewport *viewport;
    IDirect3DDevice *device;
    IDirectDrawSurface *rt;
    IDirectDraw *ddraw;
    D3DCOLOR color;
    HWND window;
    HRESULT hr;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    if (!(device = create_device(ddraw, window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        IDirectDraw_Release(ddraw);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice_QueryInterface(device, &IID_IDirectDrawSurface, (void **)&rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    white = create_diffuse_material(device, 1.0f, 1.0f, 1.0f, 1.0f);
    red   = create_diffuse_material(device, 1.0f, 0.0f, 0.0f, 1.0f);
    green = create_diffuse_material(device, 0.0f, 1.0f, 0.0f, 1.0f);
    blue  = create_diffuse_material(device, 0.0f, 0.0f, 1.0f, 1.0f);
    viewport = create_viewport(device, 0, 0, 640, 480);

    viewport_set_background(device, viewport, white);
    hr = IDirect3DViewport_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    viewport_set_background(device, viewport, red);
    hr = IDirect3DViewport_Clear(viewport, 0, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    viewport_set_background(device, viewport, green);
    hr = IDirect3DViewport_Clear(viewport, 0, NULL, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    viewport_set_background(device, viewport, blue);
    hr = IDirect3DViewport_Clear(viewport, 1, NULL, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);

    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x00ffffff, 1) || broken(compare_color(color, 0x000000ff, 1)),
            "Got unexpected color 0x%08x.\n", color);

    IDirectDrawSurface_Release(rt);
    destroy_viewport(device, viewport);
    destroy_material(white);
    destroy_material(red);
    destroy_material(green);
    destroy_material(blue);
    IDirect3DDevice_Release(device);
    IDirectDraw_Release(ddraw);
    DestroyWindow(window);
}

static struct
{
    BOOL received;
    IDirectDraw *ddraw;
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
            hr = IDirectDraw_SetCooperativeLevel(activateapp_testdata.ddraw,
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
    IDirectDraw *ddraw;
    HRESULT hr;
    HWND window;
    WNDCLASSA wc = {0};
    DDSURFACEDESC ddsd;
    IDirectDrawSurface *surface;

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
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    ok(!activateapp_testdata.received, "Received WM_ACTIVATEAPP although window was already active.\n");
    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    /* Exclusive with window not active. */
    SetForegroundWindow(GetDesktopWindow());
    activateapp_testdata.received = FALSE;
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    ok(activateapp_testdata.received, "Expected WM_ACTIVATEAPP, but did not receive it.\n");
    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    /* Normal with window not active, then exclusive with the same window. */
    SetForegroundWindow(GetDesktopWindow());
    activateapp_testdata.received = FALSE;
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    ok(!activateapp_testdata.received, "Received WM_ACTIVATEAPP when setting DDSCL_NORMAL.\n");
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    ok(activateapp_testdata.received, "Expected WM_ACTIVATEAPP, but did not receive it.\n");
    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    /* Recursive set of DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN. */
    SetForegroundWindow(GetDesktopWindow());
    activateapp_testdata.received = FALSE;
    activateapp_testdata.ddraw = ddraw;
    activateapp_testdata.window = window;
    activateapp_testdata.coop_level = DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN;
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    ok(activateapp_testdata.received, "Expected WM_ACTIVATEAPP, but did not receive it.\n");
    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    /* The recursive call seems to have some bad effect on native ddraw, despite (apparently)
     * succeeding. Another switch to exclusive and back to normal is needed to release the
     * window properly. Without doing this, SetCooperativeLevel(EXCLUSIVE) will not send
     * WM_ACTIVATEAPP messages. */
    activateapp_testdata.ddraw = NULL;
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    /* Setting DDSCL_NORMAL with recursive invocation. */
    SetForegroundWindow(GetDesktopWindow());
    activateapp_testdata.received = FALSE;
    activateapp_testdata.ddraw = ddraw;
    activateapp_testdata.window = window;
    activateapp_testdata.coop_level = DDSCL_NORMAL;
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    ok(activateapp_testdata.received, "Expected WM_ACTIVATEAPP, but did not receive it.\n");

    /* DDraw is in exclusive mode now. */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    ddsd.dwBackBufferCount = 1;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;
    hr = IDirectDraw_CreateSurface(ddraw, &ddsd, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    IDirectDrawSurface_Release(surface);

    /* Recover again, just to be sure. */
    activateapp_testdata.ddraw = NULL;
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    DestroyWindow(window);
    UnregisterClassA("ddraw_test_wndproc_wc", GetModuleHandleA(NULL));
    IDirectDraw_Release(ddraw);
}

struct format_support_check
{
    const DDPIXELFORMAT *format;
    BOOL supported;
};

static HRESULT WINAPI test_unsupported_formats_cb(DDSURFACEDESC *desc, void *ctx)
{
    struct format_support_check *format = ctx;

    if (!memcmp(format->format, &desc->ddpfPixelFormat, sizeof(*format->format)))
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
    IDirectDraw *ddraw;
    IDirect3DDevice *device;
    IDirectDrawSurface *surface;
    DDSURFACEDESC ddsd;
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
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    if (!(device = create_device(ddraw, window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        IDirectDraw_Release(ddraw);
        DestroyWindow(window);
        return;
    }

    for (i = 0; i < ARRAY_SIZE(formats); i++)
    {
        struct format_support_check check = {&formats[i].fmt, FALSE};
        hr = IDirect3DDevice_EnumTextureFormats(device, test_unsupported_formats_cb, &check);
        ok(SUCCEEDED(hr), "Failed to enumerate texture formats %#x.\n", hr);

        for (j = 0; j < ARRAY_SIZE(caps); j++)
        {
            memset(&ddsd, 0, sizeof(ddsd));
            ddsd.dwSize = sizeof(ddsd);
            ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
            ddsd.ddpfPixelFormat = formats[i].fmt;
            ddsd.dwWidth = 4;
            ddsd.dwHeight = 4;
            ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE | caps[j];

            if (caps[j] & DDSCAPS_VIDEOMEMORY && !check.supported)
                expect_success = FALSE;
            else
                expect_success = TRUE;

            hr = IDirectDraw_CreateSurface(ddraw, &ddsd, &surface, NULL);
            ok(SUCCEEDED(hr) == expect_success,
                    "Got unexpected hr %#x for format %s, caps %#x, expected %s.\n",
                    hr, formats[i].name, caps[j], expect_success ? "success" : "failure");
            if (FAILED(hr))
                continue;

            memset(&ddsd, 0, sizeof(ddsd));
            ddsd.dwSize = sizeof(ddsd);
            hr = IDirectDrawSurface_GetSurfaceDesc(surface, &ddsd);
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

            IDirectDrawSurface_Release(surface);
        }
    }

    IDirect3DDevice_Release(device);
    IDirectDraw_Release(ddraw);
    DestroyWindow(window);
}

static void test_rt_caps(void)
{
    PALETTEENTRY palette_entries[256];
    IDirectDrawPalette *palette;
    IDirect3DDevice *device;
    IDirectDraw *ddraw;
    DWORD z_depth = 0;
    unsigned int i;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    static const DDPIXELFORMAT p8_fmt =
    {
        sizeof(DDPIXELFORMAT), DDPF_PALETTEINDEXED8 | DDPF_RGB, 0,
        {8}, {0x00000000}, {0x00000000}, {0x00000000}, {0x00000000},
    };

    static const struct
    {
        const DDPIXELFORMAT *pf;
        DWORD caps_in;
        DWORD caps_out;
        HRESULT create_device_hr;
        BOOL create_may_fail;
    }
    test_data[] =
    {
        {
            NULL,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
            D3D_OK,
            FALSE,
        },
        {
            NULL,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
            D3D_OK,
            FALSE,
        },
        {
            NULL,
            DDSCAPS_OFFSCREENPLAIN,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
            DDERR_INVALIDCAPS,
            FALSE,
        },
        {
            NULL,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY | DDSCAPS_3DDEVICE,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY | DDSCAPS_3DDEVICE,
            D3DERR_SURFACENOTINVIDMEM,
            FALSE,
        },
        {
            NULL,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY,
            DDERR_INVALIDCAPS,
            FALSE,
        },
        {
            NULL,
            DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY,
            DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
            D3D_OK,
            FALSE,
        },
        {
            NULL,
            DDSCAPS_3DDEVICE,
            DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
            D3D_OK,
            FALSE,
        },
        {
            NULL,
            0,
            DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
            DDERR_INVALIDCAPS,
            FALSE,
        },
        {
            NULL,
            DDSCAPS_SYSTEMMEMORY | DDSCAPS_3DDEVICE,
            DDSCAPS_SYSTEMMEMORY | DDSCAPS_3DDEVICE,
            D3DERR_SURFACENOTINVIDMEM,
            FALSE,
        },
        {
            NULL,
            DDSCAPS_SYSTEMMEMORY,
            DDSCAPS_SYSTEMMEMORY,
            DDERR_INVALIDCAPS,
            FALSE,
        },
        {
            &p8_fmt,
            0,
            DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
            DDERR_INVALIDCAPS,
            FALSE,
        },
        {
            &p8_fmt,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE,
            ~0U /* AMD r200 */ ,
            DDERR_NOPALETTEATTACHED,
            FALSE,
        },
        {
            &p8_fmt,
            DDSCAPS_OFFSCREENPLAIN,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM,
            DDERR_INVALIDCAPS,
            FALSE,
        },
        {
            &p8_fmt,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY | DDSCAPS_3DDEVICE,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY | DDSCAPS_3DDEVICE,
            DDERR_NOPALETTEATTACHED,
            FALSE,
        },
        {
            &p8_fmt,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY,
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY,
            DDERR_INVALIDCAPS,
            FALSE,
        },
        {
            NULL,
            DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY | DDSCAPS_ZBUFFER,
            DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY | DDSCAPS_ZBUFFER | DDSCAPS_LOCALVIDMEM,
            DDERR_INVALIDCAPS,
            TRUE /* AMD Evergreen */,
        },
        {
            NULL,
            DDSCAPS_3DDEVICE | DDSCAPS_ZBUFFER,
            ~0U /* AMD Evergreen */,
            DDERR_INVALIDCAPS,
            FALSE,
        },
        {
            NULL,
            DDSCAPS_ZBUFFER,
            ~0U /* AMD Evergreen */,
            DDERR_INVALIDCAPS,
            FALSE,
        },
        {
            NULL,
            DDSCAPS_SYSTEMMEMORY | DDSCAPS_3DDEVICE | DDSCAPS_ZBUFFER,
            DDSCAPS_SYSTEMMEMORY | DDSCAPS_3DDEVICE | DDSCAPS_ZBUFFER,
            DDERR_INVALIDCAPS,
            TRUE /* Nvidia Kepler */,
        },
        {
            NULL,
            DDSCAPS_SYSTEMMEMORY | DDSCAPS_ZBUFFER,
            DDSCAPS_SYSTEMMEMORY | DDSCAPS_ZBUFFER,
            DDERR_INVALIDCAPS,
            TRUE /* Nvidia Kepler */,
        },
    };

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    if (!(device = create_device(ddraw, window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        IDirectDraw_Release(ddraw);
        DestroyWindow(window);
        return;
    }
    z_depth = get_device_z_depth(device);
    ok(!!z_depth, "Failed to get device z depth.\n");
    IDirect3DDevice_Release(device);

    memset(palette_entries, 0, sizeof(palette_entries));
    hr = IDirectDraw_CreatePalette(ddraw, DDPCAPS_ALLOW256 | DDPCAPS_8BIT, palette_entries, &palette, NULL);
    ok(SUCCEEDED(hr), "Failed to create palette, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(test_data); ++i)
    {
        IDirectDrawSurface *surface;
        DDSURFACEDESC surface_desc;
        IDirect3DDevice *device;

        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        surface_desc.ddsCaps.dwCaps = test_data[i].caps_in;
        if (test_data[i].pf)
        {
            surface_desc.dwFlags |= DDSD_PIXELFORMAT;
            surface_desc.ddpfPixelFormat = *test_data[i].pf;
        }
        if (test_data[i].caps_in & DDSCAPS_ZBUFFER)
        {
            surface_desc.dwFlags |= DDSD_ZBUFFERBITDEPTH;
            U2(surface_desc).dwZBufferBitDepth = z_depth;
        }
        surface_desc.dwWidth = 640;
        surface_desc.dwHeight = 480;
        hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
        ok(SUCCEEDED(hr) || broken(test_data[i].create_may_fail),
                "Test %u: Failed to create surface with caps %#x, hr %#x.\n",
                i, test_data[i].caps_in, hr);
        if (FAILED(hr))
            continue;

        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        hr = IDirectDrawSurface_GetSurfaceDesc(surface, &surface_desc);
        ok(SUCCEEDED(hr), "Test %u: Failed to get surface desc, hr %#x.\n", i, hr);
        ok(test_data[i].caps_out == ~0U || surface_desc.ddsCaps.dwCaps == test_data[i].caps_out,
                "Test %u: Got unexpected caps %#x, expected %#x.\n",
                i, surface_desc.ddsCaps.dwCaps, test_data[i].caps_out);

        hr = IDirectDrawSurface_QueryInterface(surface, &IID_IDirect3DHALDevice, (void **)&device);
        ok(hr == test_data[i].create_device_hr, "Test %u: Got unexpected hr %#x, expected %#x.\n",
                i, hr, test_data[i].create_device_hr);
        if (hr == DDERR_NOPALETTEATTACHED)
        {
            hr = IDirectDrawSurface_SetPalette(surface, palette);
            ok(SUCCEEDED(hr), "Test %u: Failed to set palette, hr %#x.\n", i, hr);
            hr = IDirectDrawSurface_QueryInterface(surface, &IID_IDirect3DHALDevice, (void **)&device);
            if (surface_desc.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY)
                ok(hr == DDERR_INVALIDPIXELFORMAT, "Test %u: Got unexpected hr %#x.\n", i, hr);
            else
                ok(hr == D3DERR_SURFACENOTINVIDMEM, "Test %u: Got unexpected hr %#x.\n", i, hr);
        }
        if (SUCCEEDED(hr))
        {
            refcount = IDirect3DDevice_Release(device);
            ok(refcount == 1, "Test %u: Got unexpected refcount %u.\n", i, refcount);
        }

        refcount = IDirectDrawSurface_Release(surface);
        ok(refcount == 0, "Test %u: The surface was not properly freed, refcount %u.\n", i, refcount);
    }

    IDirectDrawPalette_Release(palette);
    refcount = IDirectDraw_Release(ddraw);
    ok(refcount == 0, "The ddraw object was not properly freed, refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_primary_caps(void)
{
    const DWORD placement = DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY | DDSCAPS_SYSTEMMEMORY;
    IDirectDrawSurface *surface;
    DDSURFACEDESC surface_desc;
    IDirectDraw *ddraw;
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
            DD_OK,
            DDSCAPS_VISIBLE | DDSCAPS_PRIMARYSURFACE | DDSCAPS_FRONTBUFFER,
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
        hr = IDirectDraw_SetCooperativeLevel(ddraw, window, test_data[i].coop_level);
        ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS;
        if (test_data[i].back_buffer_count != ~0u)
            surface_desc.dwFlags |= DDSD_BACKBUFFERCOUNT;
        surface_desc.ddsCaps.dwCaps = test_data[i].caps_in;
        surface_desc.dwBackBufferCount = test_data[i].back_buffer_count;
        hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
        ok(hr == test_data[i].hr, "Test %u: Got unexpected hr %#x, expected %#x.\n", i, hr, test_data[i].hr);
        if (FAILED(hr))
            continue;

        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        hr = IDirectDrawSurface_GetSurfaceDesc(surface, &surface_desc);
        ok(SUCCEEDED(hr), "Test %u: Failed to get surface desc, hr %#x.\n", i, hr);
        ok((surface_desc.ddsCaps.dwCaps & ~placement) == test_data[i].caps_out,
                "Test %u: Got unexpected caps %#x, expected %#x.\n",
                i, surface_desc.ddsCaps.dwCaps, test_data[i].caps_out);

        IDirectDrawSurface_Release(surface);
    }

    refcount = IDirectDraw_Release(ddraw);
    ok(refcount == 0, "The ddraw object was not properly freed, refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_surface_lock(void)
{
    IDirectDraw *ddraw;
    IDirectDrawSurface *surface;
    IDirect3DDevice *device;
    HRESULT hr;
    HWND window;
    unsigned int i;
    DDSURFACEDESC ddsd;
    ULONG refcount;
    DWORD z_depth = 0;
    static const struct
    {
        DWORD caps;
        const char *name;
    }
    tests[] =
    {
        {
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY,
            "videomemory offscreenplain"
        },
        {
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY,
            "systemmemory offscreenplain"
        },
        {
            DDSCAPS_PRIMARYSURFACE,
            "primary"
        },
        {
            DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY,
            "videomemory texture"
        },
        {
            DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY,
            "systemmemory texture"
        },
        {
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE,
            "render target"
        },
        {
            DDSCAPS_ZBUFFER,
            "Z buffer"
        },
    };

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    if (!(device = create_device(ddraw, window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        IDirectDraw_Release(ddraw);
        DestroyWindow(window);
        return;
    }
    z_depth = get_device_z_depth(device);
    ok(!!z_depth, "Failed to get device z depth.\n");
    IDirect3DDevice_Release(device);

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
            ddsd.dwFlags |= DDSD_ZBUFFERBITDEPTH;
            U2(ddsd).dwZBufferBitDepth = z_depth;
        }
        ddsd.ddsCaps.dwCaps = tests[i].caps;

        hr = IDirectDraw_CreateSurface(ddraw, &ddsd, &surface, NULL);
        ok(SUCCEEDED(hr), "Failed to create surface, type %s, hr %#x.\n", tests[i].name, hr);

        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        hr = IDirectDrawSurface_Lock(surface, NULL, &ddsd, DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Failed to lock surface, type %s, hr %#x.\n", tests[i].name, hr);
        if (SUCCEEDED(hr))
        {
            hr = IDirectDrawSurface_Unlock(surface, NULL);
            ok(SUCCEEDED(hr), "Failed to unlock surface, type %s, hr %#x.\n", tests[i].name, hr);
        }

        memset(&ddsd, 0, sizeof(ddsd));
        hr = IDirectDrawSurface_Lock(surface, NULL, &ddsd, DDLOCK_WAIT, NULL);
        ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x, type %s.\n", hr, tests[i].name);

        IDirectDrawSurface_Release(surface);
    }

    refcount = IDirectDraw_Release(ddraw);
    ok(refcount == 0, "The ddraw object was not properly freed, refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_surface_discard(void)
{
    IDirectDraw *ddraw;
    IDirect3DDevice *device;
    HRESULT hr;
    HWND window;
    DDSURFACEDESC ddsd;
    IDirectDrawSurface *surface, *target;
    void *addr;
    static const struct
    {
        DWORD caps;
        BOOL discard;
    }
    tests[] =
    {
        {DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY, TRUE},
        {DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY, FALSE},
        {DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY, TRUE},
        {DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY, FALSE},
    };
    unsigned int i;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    if (!(device = create_device(ddraw, window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        IDirectDraw_Release(ddraw);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice_QueryInterface(device, &IID_IDirectDrawSurface, (void **)&target);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        BOOL discarded;

        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        ddsd.ddsCaps.dwCaps = tests[i].caps;
        ddsd.dwWidth = 64;
        ddsd.dwHeight = 64;
        hr = IDirectDraw_CreateSurface(ddraw, &ddsd, &surface, NULL);
        if (FAILED(hr))
        {
            skip("Failed to create surface, skipping.\n");
            continue;
        }

        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        hr = IDirectDrawSurface_Lock(surface, NULL, &ddsd, DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
        addr = ddsd.lpSurface;
        hr = IDirectDrawSurface_Unlock(surface, NULL);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        hr = IDirectDrawSurface_Lock(surface, NULL, &ddsd, DDLOCK_DISCARDCONTENTS | DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
        discarded = ddsd.lpSurface != addr;
        hr = IDirectDrawSurface_Unlock(surface, NULL);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

        hr = IDirectDrawSurface_Blt(target, NULL, surface, NULL, DDBLT_WAIT, NULL);
        ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);

        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        hr = IDirectDrawSurface_Lock(surface, NULL, &ddsd, DDLOCK_DISCARDCONTENTS | DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
        discarded |= ddsd.lpSurface != addr;
        hr = IDirectDrawSurface_Unlock(surface, NULL);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

        IDirectDrawSurface_Release(surface);

        /* Windows 7 reliably changes the address of surfaces that are discardable (Nvidia Kepler,
         * AMD r500, evergreen). Windows XP, at least on AMD r200, never changes the pointer. */
        ok(!discarded || tests[i].discard, "Expected surface not to be discarded, case %u\n", i);
    }

    IDirectDrawSurface_Release(target);
    IDirect3DDevice_Release(device);
    IDirectDraw_Release(ddraw);
    DestroyWindow(window);
}

static void fill_surface(IDirectDrawSurface *surface, D3DCOLOR color)
{
    DDSURFACEDESC surface_desc = {sizeof(surface_desc)};
    HRESULT hr;
    unsigned int x, y;
    DWORD *ptr;

    hr = IDirectDrawSurface_Lock(surface, NULL, &surface_desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);

    for (y = 0; y < surface_desc.dwHeight; ++y)
    {
        ptr = (DWORD *)((BYTE *)surface_desc.lpSurface + y * surface_desc.lPitch);
        for (x = 0; x < surface_desc.dwWidth; ++x)
        {
            ptr[x] = color;
        }
    }

    hr = IDirectDrawSurface_Unlock(surface, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
}

static void test_flip(void)
{
    const DWORD placement = DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY | DDSCAPS_SYSTEMMEMORY;
    IDirectDrawSurface *frontbuffer, *backbuffer1, *backbuffer2, *backbuffer3, *surface;
    DDSCAPS caps = {DDSCAPS_FLIP};
    DDSURFACEDESC surface_desc;
    BOOL sysmem_primary;
    IDirectDraw *ddraw;
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

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
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
        surface_desc.dwBackBufferCount = 3;
        hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &frontbuffer, NULL);
        ok(hr == DDERR_INVALIDCAPS, "%s: Got unexpected hr %#x.\n", test_data[i].name, hr);

        surface_desc.ddsCaps.dwCaps &= ~DDSCAPS_FLIP;
        surface_desc.dwFlags |= DDSD_BACKBUFFERCOUNT;
        hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &frontbuffer, NULL);
        ok(hr == DDERR_INVALIDCAPS, "%s: Got unexpected hr %#x.\n", test_data[i].name, hr);

        surface_desc.ddsCaps.dwCaps &= ~DDSCAPS_COMPLEX;
        surface_desc.ddsCaps.dwCaps |= DDSCAPS_FLIP;
        hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &frontbuffer, NULL);
        ok(hr == DDERR_INVALIDCAPS, "%s: Got unexpected hr %#x.\n", test_data[i].name, hr);

        surface_desc.ddsCaps.dwCaps |= DDSCAPS_COMPLEX;
        hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &frontbuffer, NULL);
        todo_wine_if(test_data[i].caps & DDSCAPS_TEXTURE)
            ok(SUCCEEDED(hr), "%s: Failed to create surface, hr %#x.\n", test_data[i].name, hr);
        if (FAILED(hr))
            continue;

        hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL | DDSCL_FULLSCREEN);
        ok(SUCCEEDED(hr), "%s: Failed to set cooperative level, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_IsLost(frontbuffer);
        ok(hr == DD_OK, "%s: Got unexpected hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_Flip(frontbuffer, NULL, DDFLIP_WAIT);
        if (test_data[i].caps & DDSCAPS_PRIMARYSURFACE)
            ok(hr == DDERR_NOEXCLUSIVEMODE, "%s: Got unexpected hr %#x.\n", test_data[i].name, hr);
        else
            ok(SUCCEEDED(hr), "%s: Failed to flip, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
        ok(SUCCEEDED(hr), "%s: Failed to set cooperative level, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_IsLost(frontbuffer);
        todo_wine ok(hr == DDERR_SURFACELOST, "%s: Got unexpected hr %#x.\n", test_data[i].name, hr);
        hr = restore_surfaces(ddraw);
        ok(SUCCEEDED(hr), "%s: Failed to restore surfaces, hr %#x.\n", test_data[i].name, hr);

        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        hr = IDirectDrawSurface_GetSurfaceDesc(frontbuffer, &surface_desc);
        ok(SUCCEEDED(hr), "%s: Failed to get surface desc, hr %#x.\n", test_data[i].name, hr);
        expected_caps = DDSCAPS_FRONTBUFFER | DDSCAPS_COMPLEX | DDSCAPS_FLIP | test_data[i].caps;
        if (test_data[i].caps & DDSCAPS_PRIMARYSURFACE)
            expected_caps |= DDSCAPS_VISIBLE;
        ok((surface_desc.ddsCaps.dwCaps & ~placement) == expected_caps,
                "%s: Got unexpected caps %#x.\n", test_data[i].name, surface_desc.ddsCaps.dwCaps);
        sysmem_primary = surface_desc.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY;

        hr = IDirectDrawSurface_GetAttachedSurface(frontbuffer, &caps, &backbuffer1);
        ok(SUCCEEDED(hr), "%s: Failed to get attached surface, hr %#x.\n", test_data[i].name, hr);
        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        hr = IDirectDrawSurface_GetSurfaceDesc(backbuffer1, &surface_desc);
        ok(SUCCEEDED(hr), "%s: Failed to get surface desc, hr %#x.\n", test_data[i].name, hr);
        ok(!surface_desc.dwBackBufferCount, "%s: Got unexpected back buffer count %u.\n",
                test_data[i].name, surface_desc.dwBackBufferCount);
        expected_caps &= ~(DDSCAPS_VISIBLE | DDSCAPS_PRIMARYSURFACE | DDSCAPS_FRONTBUFFER);
        expected_caps |= DDSCAPS_BACKBUFFER;
        ok((surface_desc.ddsCaps.dwCaps & ~placement) == expected_caps,
                "%s: Got unexpected caps %#x.\n", test_data[i].name, surface_desc.ddsCaps.dwCaps);

        hr = IDirectDrawSurface_GetAttachedSurface(backbuffer1, &caps, &backbuffer2);
        ok(SUCCEEDED(hr), "%s: Failed to get attached surface, hr %#x.\n", test_data[i].name, hr);
        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        hr = IDirectDrawSurface_GetSurfaceDesc(backbuffer2, &surface_desc);
        ok(SUCCEEDED(hr), "%s: Failed to get surface desc, hr %#x.\n", test_data[i].name, hr);
        ok(!surface_desc.dwBackBufferCount, "%s: Got unexpected back buffer count %u.\n",
                test_data[i].name, surface_desc.dwBackBufferCount);
        expected_caps &= ~DDSCAPS_BACKBUFFER;
        ok((surface_desc.ddsCaps.dwCaps & ~placement) == expected_caps,
                "%s: Got unexpected caps %#x.\n", test_data[i].name, surface_desc.ddsCaps.dwCaps);

        hr = IDirectDrawSurface_GetAttachedSurface(backbuffer2, &caps, &backbuffer3);
        ok(SUCCEEDED(hr), "%s: Failed to get attached surface, hr %#x.\n", test_data[i].name, hr);
        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        hr = IDirectDrawSurface_GetSurfaceDesc(backbuffer3, &surface_desc);
        ok(SUCCEEDED(hr), "%s: Failed to get surface desc, hr %#x.\n", test_data[i].name, hr);
        ok(!surface_desc.dwBackBufferCount, "%s: Got unexpected back buffer count %u.\n",
                test_data[i].name, surface_desc.dwBackBufferCount);
        ok((surface_desc.ddsCaps.dwCaps & ~placement) == expected_caps,
                "%s: Got unexpected caps %#x.\n", test_data[i].name, surface_desc.ddsCaps.dwCaps);

        hr = IDirectDrawSurface_GetAttachedSurface(backbuffer3, &caps, &surface);
        ok(SUCCEEDED(hr), "%s: Failed to get attached surface, hr %#x.\n", test_data[i].name, hr);
        ok(surface == frontbuffer, "%s: Got unexpected surface %p, expected %p.\n",
                test_data[i].name, surface, frontbuffer);
        IDirectDrawSurface_Release(surface);

        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        surface_desc.ddsCaps.dwCaps = 0;
        surface_desc.dwWidth = 640;
        surface_desc.dwHeight = 480;
        hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
        ok(SUCCEEDED(hr), "%s: Failed to create surface, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_Flip(frontbuffer, surface, DDFLIP_WAIT);
        ok(hr == DDERR_NOTFLIPPABLE, "%s: Got unexpected hr %#x.\n", test_data[i].name, hr);
        IDirectDrawSurface_Release(surface);

        hr = IDirectDrawSurface_Flip(frontbuffer, frontbuffer, DDFLIP_WAIT);
        ok(hr == DDERR_NOTFLIPPABLE, "%s: Got unexpected hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_Flip(backbuffer1, NULL, DDFLIP_WAIT);
        ok(hr == DDERR_NOTFLIPPABLE, "%s: Got unexpected hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_Flip(backbuffer2, NULL, DDFLIP_WAIT);
        ok(hr == DDERR_NOTFLIPPABLE, "%s: Got unexpected hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_Flip(backbuffer3, NULL, DDFLIP_WAIT);
        ok(hr == DDERR_NOTFLIPPABLE, "%s: Got unexpected hr %#x.\n", test_data[i].name, hr);

        /* The Nvidia Geforce 7 driver cannot do a color fill on a texture backbuffer after
         * the backbuffer has been locked or GetSurfaceDesc has been called. Do it ourselves
         * as a workaround. */
        fill_surface(backbuffer1, 0xffff0000);
        fill_surface(backbuffer2, 0xff00ff00);
        fill_surface(backbuffer3, 0xff0000ff);

        hr = IDirectDrawSurface_Flip(frontbuffer, NULL, DDFLIP_WAIT);
        ok(SUCCEEDED(hr), "%s: Failed to flip, hr %#x.\n", test_data[i].name, hr);
        color = get_surface_color(backbuffer1, 320, 240);
        /* The testbot seems to just copy the contents of one surface to all the
         * others, instead of properly flipping. */
        ok(compare_color(color, 0x0000ff00, 1) || broken(sysmem_primary && compare_color(color, 0x000000ff, 1)),
                "%s: Got unexpected color 0x%08x.\n", test_data[i].name, color);
        color = get_surface_color(backbuffer2, 320, 240);
        ok(compare_color(color, 0x000000ff, 1), "%s: Got unexpected color 0x%08x.\n", test_data[i].name, color);
        fill_surface(backbuffer3, 0xffff0000);

        hr = IDirectDrawSurface_Flip(frontbuffer, NULL, DDFLIP_WAIT);
        ok(SUCCEEDED(hr), "%s: Failed to flip, hr %#x.\n", test_data[i].name, hr);
        color = get_surface_color(backbuffer1, 320, 240);
        ok(compare_color(color, 0x000000ff, 1) || broken(sysmem_primary && compare_color(color, 0x00ff0000, 1)),
                "%s: Got unexpected color 0x%08x.\n", test_data[i].name, color);
        color = get_surface_color(backbuffer2, 320, 240);
        ok(compare_color(color, 0x00ff0000, 1), "%s: Got unexpected color 0x%08x.\n", test_data[i].name, color);
        fill_surface(backbuffer3, 0xff00ff00);

        hr = IDirectDrawSurface_Flip(frontbuffer, NULL, DDFLIP_WAIT);
        ok(SUCCEEDED(hr), "%s: Failed to flip, hr %#x.\n", test_data[i].name, hr);
        color = get_surface_color(backbuffer1, 320, 240);
        ok(compare_color(color, 0x00ff0000, 1) || broken(sysmem_primary && compare_color(color, 0x0000ff00, 1)),
                "%s: Got unexpected color 0x%08x.\n", test_data[i].name, color);
        color = get_surface_color(backbuffer2, 320, 240);
        ok(compare_color(color, 0x0000ff00, 1), "%s: Got unexpected color 0x%08x.\n", test_data[i].name, color);
        fill_surface(backbuffer3, 0xff0000ff);

        hr = IDirectDrawSurface_Flip(frontbuffer, backbuffer1, DDFLIP_WAIT);
        ok(SUCCEEDED(hr), "%s: Failed to flip, hr %#x.\n", test_data[i].name, hr);
        color = get_surface_color(backbuffer2, 320, 240);
        ok(compare_color(color, 0x0000ff00, 1) || broken(sysmem_primary && compare_color(color, 0x000000ff, 1)),
                "%s: Got unexpected color 0x%08x.\n", test_data[i].name, color);
        color = get_surface_color(backbuffer3, 320, 240);
        ok(compare_color(color, 0x000000ff, 1), "%s: Got unexpected color 0x%08x.\n", test_data[i].name, color);
        fill_surface(backbuffer1, 0xffff0000);

        hr = IDirectDrawSurface_Flip(frontbuffer, backbuffer2, DDFLIP_WAIT);
        ok(SUCCEEDED(hr), "%s: Failed to flip, hr %#x.\n", test_data[i].name, hr);
        color = get_surface_color(backbuffer1, 320, 240);
        ok(compare_color(color, 0x00ff0000, 1), "%s: Got unexpected color 0x%08x.\n", test_data[i].name, color);
        color = get_surface_color(backbuffer3, 320, 240);
        ok(compare_color(color, 0x000000ff, 1) || broken(sysmem_primary && compare_color(color, 0x00ff0000, 1)),
                "%s: Got unexpected color 0x%08x.\n", test_data[i].name, color);
        fill_surface(backbuffer2, 0xff00ff00);

        hr = IDirectDrawSurface_Flip(frontbuffer, backbuffer3, DDFLIP_WAIT);
        ok(SUCCEEDED(hr), "%s: Failed to flip, hr %#x.\n", test_data[i].name, hr);
        color = get_surface_color(backbuffer1, 320, 240);
        ok(compare_color(color, 0x00ff0000, 1) || broken(sysmem_primary && compare_color(color, 0x0000ff00, 1)),
                "%s: Got unexpected color 0x%08x.\n", test_data[i].name, color);
        color = get_surface_color(backbuffer2, 320, 240);
        ok(compare_color(color, 0x0000ff00, 1), "%s: Got unexpected color 0x%08x.\n", test_data[i].name, color);

        IDirectDrawSurface_Release(backbuffer3);
        IDirectDrawSurface_Release(backbuffer2);
        IDirectDrawSurface_Release(backbuffer1);
        IDirectDrawSurface_Release(frontbuffer);
    }

    refcount = IDirectDraw_Release(ddraw);
    ok(refcount == 0, "The ddraw object was not properly freed, refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_sysmem_overlay(void)
{
    IDirectDraw *ddraw;
    HWND window;
    HRESULT hr;
    DDSURFACEDESC ddsd;
    IDirectDrawSurface *surface;
    ULONG ref;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.dwWidth = 16;
    ddsd.dwHeight = 16;
    ddsd.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OVERLAY;
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(ddsd.ddpfPixelFormat).dwRGBBitCount = 32;
    U2(ddsd.ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(ddsd.ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(ddsd.ddpfPixelFormat).dwBBitMask = 0x000000ff;
    hr = IDirectDraw_CreateSurface(ddraw, &ddsd, &surface, NULL);
    ok(hr == DDERR_NOOVERLAYHW, "Got unexpected hr %#x.\n", hr);

    ref = IDirectDraw_Release(ddraw);
    ok(ref == 0, "Ddraw object not properly released, refcount %u.\n", ref);
    DestroyWindow(window);
}

static void test_primary_palette(void)
{
    DDSCAPS surface_caps = {DDSCAPS_FLIP};
    IDirectDrawSurface *primary, *backbuffer;
    PALETTEENTRY palette_entries[256];
    IDirectDrawPalette *palette, *tmp;
    DDSURFACEDESC surface_desc;
    IDirectDraw *ddraw;
    DWORD palette_caps;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    if (FAILED(IDirectDraw_SetDisplayMode(ddraw, 640, 480, 8)))
    {
        win_skip("Failed to set 8 bpp display mode, skipping test.\n");
        IDirectDraw_Release(ddraw);
        DestroyWindow(window);
        return;
    }
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;
    surface_desc.dwBackBufferCount = 1;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface_GetAttachedSurface(primary, &surface_caps, &backbuffer);
    ok(SUCCEEDED(hr), "Failed to get attached surface, hr %#x.\n", hr);

    memset(palette_entries, 0, sizeof(palette_entries));
    hr = IDirectDraw_CreatePalette(ddraw, DDPCAPS_8BIT | DDPCAPS_ALLOW256, palette_entries, &palette, NULL);
    ok(SUCCEEDED(hr), "Failed to create palette, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)palette);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);

    hr = IDirectDrawPalette_GetCaps(palette, &palette_caps);
    ok(SUCCEEDED(hr), "Failed to get palette caps, hr %#x.\n", hr);
    ok(palette_caps == (DDPCAPS_8BIT | DDPCAPS_ALLOW256), "Got unexpected palette caps %#x.\n", palette_caps);

    hr = IDirectDrawSurface_SetPalette(primary, palette);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);

    /* The Windows 8 testbot attaches the palette to the backbuffer as well,
     * and is generally somewhat broken with respect to 8 bpp / palette
     * handling. */
    if (SUCCEEDED(IDirectDrawSurface_GetPalette(backbuffer, &tmp)))
    {
        win_skip("Broken palette handling detected, skipping tests.\n");
        IDirectDrawPalette_Release(tmp);
        IDirectDrawPalette_Release(palette);
        /* The Windows 8 testbot keeps extra references to the primary and
         * backbuffer while in 8 bpp mode. */
        hr = IDirectDraw_RestoreDisplayMode(ddraw);
        ok(SUCCEEDED(hr), "Failed to restore display mode, hr %#x.\n", hr);
        goto done;
    }

    refcount = get_refcount((IUnknown *)palette);
    ok(refcount == 2, "Got unexpected refcount %u.\n", refcount);

    hr = IDirectDrawPalette_GetCaps(palette, &palette_caps);
    ok(SUCCEEDED(hr), "Failed to get palette caps, hr %#x.\n", hr);
    ok(palette_caps == (DDPCAPS_8BIT | DDPCAPS_PRIMARYSURFACE | DDPCAPS_ALLOW256),
            "Got unexpected palette caps %#x.\n", palette_caps);

    hr = IDirectDrawSurface_SetPalette(primary, NULL);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)palette);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);

    hr = IDirectDrawPalette_GetCaps(palette, &palette_caps);
    ok(SUCCEEDED(hr), "Failed to get palette caps, hr %#x.\n", hr);
    ok(palette_caps == (DDPCAPS_8BIT | DDPCAPS_ALLOW256), "Got unexpected palette caps %#x.\n", palette_caps);

    hr = IDirectDrawSurface_SetPalette(primary, palette);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)palette);
    ok(refcount == 2, "Got unexpected refcount %u.\n", refcount);

    hr = IDirectDrawSurface_GetPalette(primary, &tmp);
    ok(SUCCEEDED(hr), "Failed to get palette, hr %#x.\n", hr);
    ok(tmp == palette, "Got unexpected palette %p, expected %p.\n", tmp, palette);
    IDirectDrawPalette_Release(tmp);
    hr = IDirectDrawSurface_GetPalette(backbuffer, &tmp);
    ok(hr == DDERR_NOPALETTEATTACHED, "Got unexpected hr %#x.\n", hr);

    refcount = IDirectDrawPalette_Release(palette);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDrawPalette_Release(palette);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    /* Note that this only seems to work when the palette is attached to the
     * primary surface. When attached to a regular surface, attempting to get
     * the palette here will cause an access violation. */
    hr = IDirectDrawSurface_GetPalette(primary, &tmp);
    ok(hr == DDERR_NOPALETTEATTACHED, "Got unexpected hr %#x.\n", hr);

    hr = IDirectDrawSurface_IsLost(primary);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(primary, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(surface_desc.dwWidth == 640, "Got unexpected surface width %u.\n", surface_desc.dwWidth);
    ok(surface_desc.dwHeight == 480, "Got unexpected surface height %u.\n", surface_desc.dwHeight);
    ok(U1(surface_desc.ddpfPixelFormat).dwRGBBitCount == 8, "Got unexpected bit count %u.\n",
            U1(surface_desc.ddpfPixelFormat).dwRGBBitCount);

    hr = set_display_mode(ddraw, 640, 480);
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(primary, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(surface_desc.dwWidth == 640, "Got unexpected surface width %u.\n", surface_desc.dwWidth);
    ok(surface_desc.dwHeight == 480, "Got unexpected surface height %u.\n", surface_desc.dwHeight);
    ok(U1(surface_desc.ddpfPixelFormat).dwRGBBitCount == 32
            || U1(surface_desc.ddpfPixelFormat).dwRGBBitCount == 24,
            "Got unexpected bit count %u.\n", U1(surface_desc.ddpfPixelFormat).dwRGBBitCount);

    hr = IDirectDrawSurface_IsLost(primary);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Restore(primary);
    ok(hr == DDERR_WRONGMODE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_IsLost(primary);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(primary, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(surface_desc.dwWidth == 640, "Got unexpected surface width %u.\n", surface_desc.dwWidth);
    ok(surface_desc.dwHeight == 480, "Got unexpected surface height %u.\n", surface_desc.dwHeight);
    ok(U1(surface_desc.ddpfPixelFormat).dwRGBBitCount == 32
            || U1(surface_desc.ddpfPixelFormat).dwRGBBitCount == 24,
            "Got unexpected bit count %u.\n", U1(surface_desc.ddpfPixelFormat).dwRGBBitCount);

done:
    refcount = IDirectDrawSurface_Release(backbuffer);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDrawSurface_Release(primary);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDraw_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

static HRESULT WINAPI surface_counter(IDirectDrawSurface *surface, DDSURFACEDESC *desc, void *context)
{
    UINT *surface_count = context;

    ++(*surface_count);
    IDirectDrawSurface_Release(surface);

    return DDENUMRET_OK;
}

static void test_surface_attachment(void)
{
    IDirectDrawSurface *surface1, *surface2, *surface3, *surface4;
    DDSCAPS caps = {DDSCAPS_TEXTURE};
    DDSURFACEDESC surface_desc;
    IDirectDraw *ddraw;
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
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface1, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface_GetAttachedSurface(surface1, &caps, &surface2);
    ok(SUCCEEDED(hr), "Failed to get mip level, hr %#x.\n", hr);
    hr = IDirectDrawSurface_GetAttachedSurface(surface2, &caps, &surface3);
    ok(SUCCEEDED(hr), "Failed to get mip level, hr %#x.\n", hr);
    hr = IDirectDrawSurface_GetAttachedSurface(surface3, &caps, &surface4);
    ok(hr == DDERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);

    surface_count = 0;
    IDirectDrawSurface_EnumAttachedSurfaces(surface1, &surface_count, surface_counter);
    ok(surface_count == 1, "Got unexpected surface_count %u.\n", surface_count);
    surface_count = 0;
    IDirectDrawSurface_EnumAttachedSurfaces(surface2, &surface_count, surface_counter);
    ok(surface_count == 1, "Got unexpected surface_count %u.\n", surface_count);
    surface_count = 0;
    IDirectDrawSurface_EnumAttachedSurfaces(surface3, &surface_count, surface_counter);
    ok(!surface_count, "Got unexpected surface_count %u.\n", surface_count);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    surface_desc.dwWidth = 16;
    surface_desc.dwHeight = 16;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface4, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface_AddAttachedSurface(surface1, surface4);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_AddAttachedSurface(surface4, surface1);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_AddAttachedSurface(surface3, surface4);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_AddAttachedSurface(surface4, surface3);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_AddAttachedSurface(surface2, surface4);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_AddAttachedSurface(surface4, surface2);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);

    IDirectDrawSurface_Release(surface4);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;
    surface_desc.dwWidth = 16;
    surface_desc.dwHeight = 16;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface4, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    if (SUCCEEDED(hr = IDirectDrawSurface_AddAttachedSurface(surface1, surface4)))
    {
        skip("Running on refrast, skipping some tests.\n");
        hr = IDirectDrawSurface_DeleteAttachedSurface(surface1, 0, surface4);
        ok(SUCCEEDED(hr), "Failed to detach surface, hr %#x.\n", hr);
    }
    else
    {
        ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
        hr = IDirectDrawSurface_AddAttachedSurface(surface4, surface1);
        ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
        hr = IDirectDrawSurface_AddAttachedSurface(surface3, surface4);
        ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
        hr = IDirectDrawSurface_AddAttachedSurface(surface4, surface3);
        ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
        hr = IDirectDrawSurface_AddAttachedSurface(surface2, surface4);
        ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
        hr = IDirectDrawSurface_AddAttachedSurface(surface4, surface2);
        ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
    }

    IDirectDrawSurface_Release(surface4);
    IDirectDrawSurface_Release(surface3);
    IDirectDrawSurface_Release(surface2);
    IDirectDrawSurface_Release(surface1);

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    /* Try a single primary and two offscreen plain surfaces. */
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface1, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    surface_desc.dwWidth = registry_mode.dmPelsWidth;
    surface_desc.dwHeight = registry_mode.dmPelsHeight;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface2, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    surface_desc.dwWidth = registry_mode.dmPelsWidth;
    surface_desc.dwHeight = registry_mode.dmPelsHeight;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface3, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    /* This one has a different size. */
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    surface_desc.dwWidth = 128;
    surface_desc.dwHeight = 128;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface4, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface_AddAttachedSurface(surface1, surface2);
    ok(SUCCEEDED(hr), "Failed to attach surface, hr %#x.\n", hr);
    /* Try the reverse without detaching first. */
    hr = IDirectDrawSurface_AddAttachedSurface(surface2, surface1);
    ok(hr == DDERR_SURFACEALREADYATTACHED, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_DeleteAttachedSurface(surface1, 0, surface2);
    ok(SUCCEEDED(hr), "Failed to detach surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface_AddAttachedSurface(surface2, surface1);
    ok(SUCCEEDED(hr), "Failed to attach surface, hr %#x.\n", hr);
    /* Try to detach reversed. */
    hr = IDirectDrawSurface_DeleteAttachedSurface(surface1, 0, surface2);
    ok(hr == DDERR_CANNOTDETACHSURFACE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_DeleteAttachedSurface(surface2, 0, surface1);
    ok(SUCCEEDED(hr), "Failed to detach surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface_AddAttachedSurface(surface2, surface3);
    ok(SUCCEEDED(hr), "Failed to attach surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface_DeleteAttachedSurface(surface2, 0, surface3);
    ok(SUCCEEDED(hr), "Failed to detach surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface_AddAttachedSurface(surface1, surface4);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_AddAttachedSurface(surface4, surface1);
    ok(hr == DDERR_CANNOTATTACHSURFACE, "Got unexpected hr %#x.\n", hr);

    IDirectDrawSurface_Release(surface4);
    IDirectDrawSurface_Release(surface3);
    IDirectDrawSurface_Release(surface2);
    IDirectDrawSurface_Release(surface1);

    /* Test DeleteAttachedSurface() and automatic detachment of attached surfaces on release. */
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    surface_desc.dwWidth = 64;
    surface_desc.dwHeight = 64;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE;
    surface_desc.ddpfPixelFormat.dwSize = sizeof(surface_desc.ddpfPixelFormat);
    surface_desc.ddpfPixelFormat.dwFlags = DDPF_RGB; /* D3DFMT_R5G6B5 */
    U1(surface_desc.ddpfPixelFormat).dwRGBBitCount = 16;
    U2(surface_desc.ddpfPixelFormat).dwRBitMask = 0xf800;
    U3(surface_desc.ddpfPixelFormat).dwGBitMask = 0x07e0;
    U4(surface_desc.ddpfPixelFormat).dwBBitMask = 0x001f;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface1, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface3, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    surface_desc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    surface_desc.ddpfPixelFormat.dwFlags = DDPF_ZBUFFER;
    U1(surface_desc.ddpfPixelFormat).dwZBufferBitDepth = 16;
    U3(surface_desc.ddpfPixelFormat).dwZBitMask = 0x0000ffff;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface2, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface_AddAttachedSurface(surface1, surface2);
    ok(SUCCEEDED(hr), "Failed to attach surface, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)surface2);
    ok(refcount == 2, "Got unexpected refcount %u.\n", refcount);
    hr = IDirectDrawSurface_AddAttachedSurface(surface1, surface2);
    ok(hr == DDERR_SURFACEALREADYATTACHED, "Got unexpected hr %#x.\n", hr);

    /* Attaching while already attached to other surface. */
    hr = IDirectDrawSurface_AddAttachedSurface(surface3, surface2);
    todo_wine ok(SUCCEEDED(hr), "Failed to attach surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface_DeleteAttachedSurface(surface3, 0, surface2);
    todo_wine ok(SUCCEEDED(hr), "Failed to detach surface, hr %#x.\n", hr);
    IDirectDrawSurface_Release(surface3);

    hr = IDirectDrawSurface_DeleteAttachedSurface(surface1, 0, surface2);
    ok(SUCCEEDED(hr), "Failed to detach surface, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)surface2);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);

    /* Automatic detachment on release. */
    hr = IDirectDrawSurface_AddAttachedSurface(surface1, surface2);
    ok(SUCCEEDED(hr), "Failed to attach surface, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)surface2);
    ok(refcount == 2, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDrawSurface_Release(surface1);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDrawSurface_Release(surface2);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDraw_Release(ddraw);
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
    IDirectDraw *ddraw = NULL;
    IDirectDrawClipper *clipper = NULL;
    DDSURFACEDESC ddsd;
    IDirectDrawSurface *primary = NULL;
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

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    if (FAILED(hr))
    {
        skip("Failed to set cooperative level, hr %#x.\n", hr);
        goto cleanup;
    }

    test_format = GetPixelFormat(hdc);
    ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

    if (hdc2)
    {
        hr = IDirectDraw_CreateClipper(ddraw, 0, &clipper, NULL);
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

    hr = IDirectDraw_CreateSurface(ddraw, &ddsd, &primary, NULL);
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
        hr = IDirectDrawSurface_SetClipper(primary, clipper);
        ok(SUCCEEDED(hr), "Failed to set clipper, hr %#x.\n", hr);

        test_format = GetPixelFormat(hdc);
        ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

        test_format = GetPixelFormat(hdc2);
        ok(test_format == format, "second window has pixel format %d, expected %d\n", test_format, format);
    }

    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    hr = IDirectDrawSurface_Blt(primary, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear source surface, hr %#x.\n", hr);

    test_format = GetPixelFormat(hdc);
    ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

    if (hdc2)
    {
        test_format = GetPixelFormat(hdc2);
        ok(test_format == format, "second window has pixel format %d, expected %d\n", test_format, format);
    }

cleanup:
    if (primary) IDirectDrawSurface_Release(primary);
    if (clipper) IDirectDrawClipper_Release(clipper);
    if (ddraw) IDirectDraw_Release(ddraw);
    if (gl) FreeLibrary(gl);
    if (hdc) ReleaseDC(window, hdc);
    if (hdc2) ReleaseDC(window2, hdc2);
    if (window) DestroyWindow(window);
    if (window2) DestroyWindow(window2);
}

static void test_create_surface_pitch(void)
{
    IDirectDrawSurface *surface;
    DDSURFACEDESC surface_desc;
    IDirectDraw *ddraw;
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
                0,                              0,      DD_OK,
                DDSD_PITCH,                     0x100,  0x100},
        {DDSCAPS_VIDEOMEMORY | DDSCAPS_OFFSCREENPLAIN,
                DDSD_PITCH,                     0x104,  DD_OK,
                DDSD_PITCH,                     0x100,  0x100},
        {DDSCAPS_VIDEOMEMORY | DDSCAPS_OFFSCREENPLAIN,
                DDSD_PITCH,                     0x0f8,  DD_OK,
                DDSD_PITCH,                     0x100,  0x100},
        {DDSCAPS_VIDEOMEMORY | DDSCAPS_OFFSCREENPLAIN,
                DDSD_LPSURFACE | DDSD_PITCH,    0x100,  DDERR_INVALIDCAPS,
                0,                              0,      0    },
        {DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN,
                0,                              0,      DD_OK,
                DDSD_PITCH,                     0x100,  0x0fc},
        /* 5 */
        {DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN,
                DDSD_PITCH,                     0x104,  DD_OK,
                DDSD_PITCH,                     0x100,  0x0fc},
        {DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN,
                DDSD_PITCH,                     0x0f8,  DD_OK,
                DDSD_PITCH,                     0x100,  0x0fc},
        {DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN,
                DDSD_PITCH | DDSD_LINEARSIZE,   0,      DD_OK,
                DDSD_PITCH,                     0x100,  0x0fc},
        {DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN,
                DDSD_LPSURFACE,                 0,      DDERR_INVALIDPARAMS,
                0,                              0,      0    },
        {DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN,
                DDSD_LPSURFACE | DDSD_PITCH,    0x100,  DDERR_INVALIDPARAMS,
                0,                              0,      0    },
        /* 10 */
        {DDSCAPS_VIDEOMEMORY | DDSCAPS_OFFSCREENPLAIN | DDSCAPS_ALLOCONLOAD,
                0,                              0,      DDERR_INVALIDCAPS,
                0,                              0,      0    },
        {DDSCAPS_VIDEOMEMORY | DDSCAPS_TEXTURE | DDSCAPS_ALLOCONLOAD,
                0,                              0,      DD_OK,
                DDSD_PITCH,                     0x100,  0    },
        {DDSCAPS_VIDEOMEMORY | DDSCAPS_TEXTURE | DDSCAPS_ALLOCONLOAD,
                DDSD_LPSURFACE | DDSD_PITCH,    0x100,  DDERR_INVALIDCAPS,
                0,                              0,      0    },
        {DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN | DDSCAPS_ALLOCONLOAD,
                0,                              0,      DDERR_INVALIDCAPS,
                0,                              0,      0    },
        {DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE | DDSCAPS_ALLOCONLOAD,
                0,                              0,      DD_OK,
                DDSD_PITCH,                     0x100,  0    },
        /* 15 */
        {DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE | DDSCAPS_ALLOCONLOAD,
                DDSD_LPSURFACE | DDSD_PITCH,    0x100,  DDERR_INVALIDPARAMS,
                0,                              0,      0    },
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
        surface_desc.ddpfPixelFormat.dwSize = sizeof(surface_desc.ddpfPixelFormat);
        surface_desc.ddpfPixelFormat.dwFlags = DDPF_RGB;
        U1(surface_desc.ddpfPixelFormat).dwRGBBitCount = 32;
        U2(surface_desc.ddpfPixelFormat).dwRBitMask = 0x00ff0000;
        U3(surface_desc.ddpfPixelFormat).dwGBitMask = 0x0000ff00;
        U4(surface_desc.ddpfPixelFormat).dwBBitMask = 0x000000ff;
        hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
        if (test_data[i].flags_in & DDSD_LPSURFACE)
        {
            HRESULT expected_hr = SUCCEEDED(test_data[i].hr) ? DDERR_INVALIDPARAMS : test_data[i].hr;
            ok(hr == expected_hr, "Test %u: Got unexpected hr %#x, expected %#x.\n", i, hr, expected_hr);
            surface_desc.lpSurface = mem;
            hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
        }
        if ((test_data[i].caps & DDSCAPS_VIDEOMEMORY) && hr == DDERR_NODIRECTDRAWHW)
            continue;
        ok(hr == test_data[i].hr, "Test %u: Got unexpected hr %#x, expected %#x.\n", i, hr, test_data[i].hr);
        if (FAILED(hr))
            continue;

        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        hr = IDirectDrawSurface_GetSurfaceDesc(surface, &surface_desc);
        ok(SUCCEEDED(hr), "Test %u: Failed to get surface desc, hr %#x.\n", i, hr);
        ok((surface_desc.dwFlags & flags_mask) == test_data[i].flags_out,
                "Test %u: Got unexpected flags %#x, expected %#x.\n",
                i, surface_desc.dwFlags & flags_mask, test_data[i].flags_out);
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

        IDirectDrawSurface_Release(surface);
    }

    HeapFree(GetProcessHeap(), 0, mem);
    refcount = IDirectDraw_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_mipmap(void)
{
    IDirectDrawSurface *surface, *surface2;
    DDSURFACEDESC surface_desc;
    IDirectDraw *ddraw;
    unsigned int i;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    DDSCAPS caps = {DDSCAPS_COMPLEX};
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
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&hal_caps, 0, sizeof(hal_caps));
    hal_caps.dwSize = sizeof(hal_caps);
    hr = IDirectDraw_GetCaps(ddraw, &hal_caps, NULL);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);
    if ((hal_caps.ddsCaps.dwCaps & (DDSCAPS_TEXTURE | DDSCAPS_MIPMAP)) != (DDSCAPS_TEXTURE | DDSCAPS_MIPMAP))
    {
        skip("Mipmapped textures not supported, skipping tests.\n");
        IDirectDraw_Release(ddraw);
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
        hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
        ok(hr == tests[i].hr, "Test %u: Got unexpected hr %#x.\n", i, hr);
        if (FAILED(hr))
            continue;

        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        hr = IDirectDrawSurface_GetSurfaceDesc(surface, &surface_desc);
        ok(SUCCEEDED(hr), "Test %u: Failed to get surface desc, hr %#x.\n", i, hr);
        ok(surface_desc.dwFlags & DDSD_MIPMAPCOUNT,
                "Test %u: Got unexpected flags %#x.\n", i, surface_desc.dwFlags);
        ok(U2(surface_desc).dwMipMapCount == tests[i].mipmap_count_out,
                "Test %u: Got unexpected mipmap count %u.\n", i, U2(surface_desc).dwMipMapCount);

        if (U2(surface_desc).dwMipMapCount > 1)
        {
            hr = IDirectDrawSurface_GetAttachedSurface(surface, &caps, &surface2);
            ok(SUCCEEDED(hr), "Test %u: Failed to get attached surface, hr %#x.\n", i, hr);

            memset(&surface_desc, 0, sizeof(surface_desc));
            surface_desc.dwSize = sizeof(surface_desc);
            hr = IDirectDrawSurface_Lock(surface, NULL, &surface_desc, 0, NULL);
            ok(SUCCEEDED(hr), "Test %u: Failed to lock surface, hr %#x.\n", i, hr);
            memset(&surface_desc, 0, sizeof(surface_desc));
            surface_desc.dwSize = sizeof(surface_desc);
            hr = IDirectDrawSurface_Lock(surface2, NULL, &surface_desc, 0, NULL);
            ok(SUCCEEDED(hr), "Test %u: Failed to lock surface, hr %#x.\n", i, hr);
            IDirectDrawSurface_Unlock(surface2, NULL);
            IDirectDrawSurface_Unlock(surface, NULL);

            IDirectDrawSurface_Release(surface2);
        }

        IDirectDrawSurface_Release(surface);
    }

    refcount = IDirectDraw_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_palette_complex(void)
{
    IDirectDrawSurface *surface, *mipmap, *tmp;
    DDSURFACEDESC surface_desc;
    IDirectDraw *ddraw;
    IDirectDrawPalette *palette, *palette2, *palette_mipmap;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    DDSCAPS caps = {DDSCAPS_COMPLEX};
    DDCAPS hal_caps;
    PALETTEENTRY palette_entries[256];
    unsigned int i;
    HDC dc;
    RGBQUAD rgbquad;
    UINT count;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&hal_caps, 0, sizeof(hal_caps));
    hal_caps.dwSize = sizeof(hal_caps);
    hr = IDirectDraw_GetCaps(ddraw, &hal_caps, NULL);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);
    if ((hal_caps.ddsCaps.dwCaps & (DDSCAPS_TEXTURE | DDSCAPS_MIPMAP)) != (DDSCAPS_TEXTURE | DDSCAPS_MIPMAP))
    {
        skip("Mipmapped textures not supported, skipping mipmap palette test.\n");
        IDirectDraw_Release(ddraw);
        DestroyWindow(window);
        return;
    }

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    surface_desc.dwWidth = 128;
    surface_desc.dwHeight = 128;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
    surface_desc.ddpfPixelFormat.dwSize = sizeof(surface_desc.ddpfPixelFormat);
    surface_desc.ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED8 | DDPF_RGB;
    U1(surface_desc.ddpfPixelFormat).dwRGBBitCount = 8;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    memset(palette_entries, 0, sizeof(palette_entries));
    hr = IDirectDraw_CreatePalette(ddraw, DDPCAPS_8BIT | DDPCAPS_ALLOW256,
            palette_entries, &palette, NULL);
    ok(SUCCEEDED(hr), "Failed to create palette, hr %#x.\n", hr);

    memset(palette_entries, 0, sizeof(palette_entries));
    palette_entries[1].peRed = 0xff;
    palette_entries[1].peGreen = 0x80;
    hr = IDirectDraw_CreatePalette(ddraw, DDPCAPS_8BIT | DDPCAPS_ALLOW256,
            palette_entries, &palette_mipmap, NULL);
    ok(SUCCEEDED(hr), "Failed to create palette, hr %#x.\n", hr);

    palette2 = (void *)0xdeadbeef;
    hr = IDirectDrawSurface_GetPalette(surface, &palette2);
    ok(hr == DDERR_NOPALETTEATTACHED, "Got unexpected hr %#x.\n", hr);
    ok(!palette2, "Got unexpected palette %p.\n", palette2);
    hr = IDirectDrawSurface_SetPalette(surface, palette);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);
    hr = IDirectDrawSurface_GetPalette(surface, &palette2);
    ok(SUCCEEDED(hr), "Failed to get palette, hr %#x.\n", hr);
    ok(palette == palette2, "Got unexpected palette %p.\n", palette2);
    IDirectDrawPalette_Release(palette2);

    mipmap = surface;
    IDirectDrawSurface_AddRef(mipmap);
    for (i = 0; i < 7; ++i)
    {
        hr = IDirectDrawSurface_GetAttachedSurface(mipmap, &caps, &tmp);
        ok(SUCCEEDED(hr), "Failed to get attached surface, i %u, hr %#x.\n", i, hr);
        palette2 = (void *)0xdeadbeef;
        hr = IDirectDrawSurface_GetPalette(tmp, &palette2);
        ok(hr == DDERR_NOPALETTEATTACHED, "Got unexpected hr %#x, i %u.\n", hr, i);
        ok(!palette2, "Got unexpected palette %p, i %u.\n", palette2, i);

        hr = IDirectDrawSurface_SetPalette(tmp, palette_mipmap);
        ok(SUCCEEDED(hr), "Failed to set palette, i %u, hr %#x.\n", i, hr);

        hr = IDirectDrawSurface_GetPalette(tmp, &palette2);
        ok(SUCCEEDED(hr), "Failed to get palette, i %u, hr %#x.\n", i, hr);
        ok(palette_mipmap == palette2, "Got unexpected palette %p.\n", palette2);
        IDirectDrawPalette_Release(palette2);

        hr = IDirectDrawSurface_GetDC(tmp, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC, i %u, hr %#x.\n", i, hr);
        count = GetDIBColorTable(dc, 1, 1, &rgbquad);
        ok(count == 1, "Expected count 1, got %u.\n", count);
        ok(rgbquad.rgbRed == 0xff, "Expected rgbRed = 0xff, got %#x.\n", rgbquad.rgbRed);
        ok(rgbquad.rgbGreen == 0x80, "Expected rgbGreen = 0x80, got %#x.\n", rgbquad.rgbGreen);
        ok(rgbquad.rgbBlue == 0x0, "Expected rgbBlue = 0x0, got %#x.\n", rgbquad.rgbBlue);
        hr = IDirectDrawSurface_ReleaseDC(tmp, dc);
        ok(SUCCEEDED(hr), "Failed to release DC, i %u, hr %#x.\n", i, hr);

        IDirectDrawSurface_Release(mipmap);
        mipmap = tmp;
    }

    hr = IDirectDrawSurface_GetAttachedSurface(mipmap, &caps, &tmp);
    ok(hr == DDERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);
    IDirectDrawSurface_Release(mipmap);
    refcount = IDirectDrawSurface_Release(surface);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDrawPalette_Release(palette_mipmap);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDrawPalette_Release(palette);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    refcount = IDirectDraw_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_p8_blit(void)
{
    IDirectDrawSurface *src, *dst, *dst_p8;
    DDSURFACEDESC surface_desc;
    IDirectDraw *ddraw;
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
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    is_warp = ddraw_is_warp(ddraw);

    memset(palette_entries, 0, sizeof(palette_entries));
    palette_entries[1].peGreen = 0xff;
    palette_entries[2].peBlue = 0xff;
    palette_entries[3].peFlags = 0xff;
    palette_entries[4].peRed = 0xff;
    hr = IDirectDraw_CreatePalette(ddraw, DDPCAPS_8BIT | DDPCAPS_ALLOW256,
            palette_entries, &palette, NULL);
    ok(SUCCEEDED(hr), "Failed to create palette, hr %#x.\n", hr);
    palette_entries[1].peBlue = 0xff;
    palette_entries[2].peGreen = 0xff;
    palette_entries[3].peRed = 0xff;
    palette_entries[4].peFlags = 0x0;
    hr = IDirectDraw_CreatePalette(ddraw, DDPCAPS_8BIT | DDPCAPS_ALLOW256,
            palette_entries, &palette2, NULL);
    ok(SUCCEEDED(hr), "Failed to create palette, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    surface_desc.dwWidth = 8;
    surface_desc.dwHeight = 1;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    surface_desc.ddpfPixelFormat.dwSize = sizeof(surface_desc.ddpfPixelFormat);
    surface_desc.ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED8 | DDPF_RGB;
    U1(surface_desc.ddpfPixelFormat).dwRGBBitCount = 8;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &src, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &dst_p8, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface_SetPalette(dst_p8, palette2);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    surface_desc.dwWidth = 8;
    surface_desc.dwHeight = 1;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    surface_desc.ddpfPixelFormat.dwSize = sizeof(surface_desc.ddpfPixelFormat);
    surface_desc.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    U1(surface_desc.ddpfPixelFormat).dwRGBBitCount = 32;
    U2(surface_desc.ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(surface_desc.ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(surface_desc.ddpfPixelFormat).dwBBitMask = 0x000000ff;
    U5(surface_desc.ddpfPixelFormat).dwRGBAlphaBitMask = 0xff000000;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &dst, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface_Lock(src, NULL, &surface_desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock source surface, hr %#x.\n", hr);
    memcpy(surface_desc.lpSurface, src_data, sizeof(src_data));
    hr = IDirectDrawSurface_Unlock(src, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock source surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface_Lock(dst_p8, NULL, &surface_desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock destination surface, hr %#x.\n", hr);
    memcpy(surface_desc.lpSurface, src_data2, sizeof(src_data2));
    hr = IDirectDrawSurface_Unlock(dst_p8, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock destination surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface_SetPalette(src, palette);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);
    hr = IDirectDrawSurface_Blt(dst, NULL, src, NULL, DDBLT_WAIT, NULL);
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
    hr = IDirectDrawSurface_Blt(dst_p8, NULL, src, NULL, DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE, &fx);
    ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);

    hr = IDirectDrawSurface_Lock(dst_p8, NULL, &surface_desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
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
    hr = IDirectDrawSurface_Unlock(dst_p8, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock destination surface, hr %#x.\n", hr);

    IDirectDrawSurface_Release(src);
    IDirectDrawSurface_Release(dst);
    IDirectDrawSurface_Release(dst_p8);
    IDirectDrawPalette_Release(palette);
    IDirectDrawPalette_Release(palette2);

    refcount = IDirectDraw_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_material(void)
{
    IDirect3DMaterial *background, *material;
    IDirect3DExecuteBuffer *execute_buffer;
    D3DMATERIALHANDLE mat_handle, tmp;
    D3DEXECUTEBUFFERDESC exec_desc;
    IDirect3DViewport *viewport;
    IDirect3DDevice *device;
    IDirectDrawSurface *rt;
    IDirectDraw *ddraw;
    UINT inst_length;
    D3DCOLOR color;
    ULONG refcount;
    unsigned int i;
    HWND window;
    HRESULT hr;
    BOOL valid;
    void *ptr;

    static D3DVERTEX quad[] =
    {
        {{-1.0f}, {-1.0f}, {0.0f}, {1.0f}, {0.0f}, {0.0f}},
        {{-1.0f}, { 1.0f}, {0.0f}, {1.0f}, {0.0f}, {0.0f}},
        {{ 1.0f}, {-1.0f}, {0.0f}, {1.0f}, {0.0f}, {0.0f}},
        {{ 1.0f}, { 1.0f}, {0.0f}, {1.0f}, {0.0f}, {0.0f}},
    };
    static const struct
    {
        BOOL material;
        D3DCOLOR expected_color;
    }
    test_data[] =
    {
        {TRUE,  0x0000ff00},
        {FALSE, 0x00ffffff},
    };
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    if (!(device = create_device(ddraw, window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice_QueryInterface(device, &IID_IDirectDrawSurface, (void **)&rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    background = create_diffuse_material(device, 0.0f, 0.0f, 1.0f, 1.0f);
    viewport = create_viewport(device, 0, 0, 640, 480);
    viewport_set_background(device, viewport, background);

    material = create_emissive_material(device, 0.0f, 1.0f, 0.0f, 0.0f);
    hr = IDirect3DMaterial_GetHandle(material, device, &mat_handle);
    ok(SUCCEEDED(hr), "Failed to get material handle, hr %#x.\n", hr);

    memset(&exec_desc, 0, sizeof(exec_desc));
    exec_desc.dwSize = sizeof(exec_desc);
    exec_desc.dwFlags = D3DDEB_BUFSIZE | D3DDEB_CAPS;
    exec_desc.dwBufferSize = 1024;
    exec_desc.dwCaps = D3DDEBCAPS_SYSTEMMEMORY;

    hr = IDirect3DDevice_CreateExecuteBuffer(device, &exec_desc, &execute_buffer, NULL);
    ok(SUCCEEDED(hr), "Failed to create execute buffer, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(test_data); ++i)
    {
        hr = IDirect3DExecuteBuffer_Lock(execute_buffer, &exec_desc);
        ok(SUCCEEDED(hr), "Failed to lock execute buffer, hr %#x.\n", hr);

        memcpy(exec_desc.lpData, quad, sizeof(quad));
        ptr = ((BYTE *)exec_desc.lpData) + sizeof(quad);
        emit_set_ls(&ptr, D3DLIGHTSTATE_MATERIAL, test_data[i].material ? mat_handle : 0);
        emit_process_vertices(&ptr, D3DPROCESSVERTICES_TRANSFORMLIGHT, 0, 4);
        emit_tquad(&ptr, 0);
        emit_end(&ptr);
        inst_length = (BYTE *)ptr - (BYTE *)exec_desc.lpData;
        inst_length -= sizeof(quad);

        hr = IDirect3DExecuteBuffer_Unlock(execute_buffer);
        ok(SUCCEEDED(hr), "Failed to unlock execute buffer, hr %#x.\n", hr);

        hr = IDirect3DViewport_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);
        ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);

        hr = IDirect3DDevice_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
        set_execute_data(execute_buffer, 4, sizeof(quad), inst_length);
        hr = IDirect3DDevice_Execute(device, execute_buffer, viewport, D3DEXECUTE_CLIPPED);
        ok(SUCCEEDED(hr), "Failed to execute exec buffer, hr %#x.\n", hr);
        hr = IDirect3DDevice_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);
        color = get_surface_color(rt, 320, 240);
        if (test_data[i].material)
            todo_wine ok(compare_color(color, test_data[i].expected_color, 1)
                    /* The Windows 8 testbot appears to return undefined results. */
                    || broken(TRUE),
                    "Got unexpected color 0x%08x, test %u.\n", color, i);
        else
            ok(compare_color(color, test_data[i].expected_color, 1),
                    "Got unexpected color 0x%08x, test %u.\n", color, i);
    }

    destroy_material(material);
    material = create_diffuse_material(device, 1.0f, 0.0f, 0.0f, 1.0f);
    hr = IDirect3DMaterial_GetHandle(material, device, &mat_handle);
    ok(SUCCEEDED(hr), "Failed to get material handle, hr %#x.\n", hr);

    hr = IDirect3DViewport_SetBackground(viewport, mat_handle);
    ok(SUCCEEDED(hr), "Failed to set viewport background, hr %#x.\n", hr);
    hr = IDirect3DViewport_GetBackground(viewport, &tmp, &valid);
    ok(SUCCEEDED(hr), "Failed to get viewport background, hr %#x.\n", hr);
    ok(tmp == mat_handle, "Got unexpected material handle %#x, expected %#x.\n", tmp, mat_handle);
    ok(valid, "Got unexpected valid %#x.\n", valid);
    hr = IDirect3DViewport_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x00ff0000, 1), "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DViewport_SetBackground(viewport, 0);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DViewport_GetBackground(viewport, &tmp, &valid);
    ok(SUCCEEDED(hr), "Failed to get viewport background, hr %#x.\n", hr);
    ok(tmp == mat_handle, "Got unexpected material handle %#x, expected %#x.\n", tmp, mat_handle);
    ok(valid, "Got unexpected valid %#x.\n", valid);
    hr = IDirect3DViewport_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x00ff0000, 1), "Got unexpected color 0x%08x.\n", color);

    destroy_viewport(device, viewport);
    viewport = create_viewport(device, 0, 0, 640, 480);

    hr = IDirect3DViewport_GetBackground(viewport, &tmp, &valid);
    ok(SUCCEEDED(hr), "Failed to get viewport background, hr %#x.\n", hr);
    ok(!tmp, "Got unexpected material handle %#x.\n", tmp);
    ok(!valid, "Got unexpected valid %#x.\n", valid);
    hr = IDirect3DViewport_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x00000000, 1), "Got unexpected color 0x%08x.\n", color);

    IDirect3DExecuteBuffer_Release(execute_buffer);
    destroy_viewport(device, viewport);
    destroy_material(background);
    destroy_material(material);
    IDirectDrawSurface_Release(rt);
    refcount = IDirect3DDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    refcount = IDirectDraw_Release(ddraw);
    ok(!refcount, "Ddraw object has %u references left.\n", refcount);
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
    static D3DLVERTEX unlitquad[] =
    {
        {{-1.0f}, {-1.0f}, {0.1f}, 0, {0xffff0000}, {0}, {0.0f}, {0.0f}},
        {{-1.0f}, { 0.0f}, {0.1f}, 0, {0xffff0000}, {0}, {0.0f}, {0.0f}},
        {{ 0.0f}, { 0.0f}, {0.1f}, 0, {0xffff0000}, {0}, {0.0f}, {0.0f}},
        {{ 0.0f}, {-1.0f}, {0.1f}, 0, {0xffff0000}, {0}, {0.0f}, {0.0f}},
    },
    litquad[] =
    {
        {{-1.0f}, { 0.0f}, {0.1f}, 0, {0xff00ff00}, {0}, {0.0f}, {0.0f}},
        {{-1.0f}, { 1.0f}, {0.1f}, 0, {0xff00ff00}, {0}, {0.0f}, {0.0f}},
        {{ 0.0f}, { 1.0f}, {0.1f}, 0, {0xff00ff00}, {0}, {0.0f}, {0.0f}},
        {{ 0.0f}, { 0.0f}, {0.1f}, 0, {0xff00ff00}, {0}, {0.0f}, {0.0f}},
    };
    static D3DVERTEX unlitnquad[] =
    {
        {{0.0f}, {-1.0f}, {0.1f}, {1.0f}, {1.0f}, {1.0f}, {0.0f}, {0.0f}},
        {{0.0f}, { 0.0f}, {0.1f}, {1.0f}, {1.0f}, {1.0f}, {0.0f}, {0.0f}},
        {{1.0f}, { 0.0f}, {0.1f}, {1.0f}, {1.0f}, {1.0f}, {0.0f}, {0.0f}},
        {{1.0f}, {-1.0f}, {0.1f}, {1.0f}, {1.0f}, {1.0f}, {0.0f}, {0.0f}},
    },
    litnquad[] =
    {
        {{0.0f}, { 0.0f}, {0.1f}, {1.0f}, {1.0f}, {1.0f}, {0.0f}, {0.0f}},
        {{0.0f}, { 1.0f}, {0.1f}, {1.0f}, {1.0f}, {1.0f}, {0.0f}, {0.0f}},
        {{1.0f}, { 1.0f}, {0.1f}, {1.0f}, {1.0f}, {1.0f}, {0.0f}, {0.0f}},
        {{1.0f}, { 0.0f}, {0.1f}, {1.0f}, {1.0f}, {1.0f}, {0.0f}, {0.0f}},
    },
    nquad[] =
    {
        {{-1.0f}, {-1.0f}, {0.0f}, {0.0f}, {0.0f}, {-1.0f}, {0.0f}, {0.0f}},
        {{-1.0f}, { 1.0f}, {0.0f}, {0.0f}, {0.0f}, {-1.0f}, {0.0f}, {0.0f}},
        {{ 1.0f}, { 1.0f}, {0.0f}, {0.0f}, {0.0f}, {-1.0f}, {0.0f}, {0.0f}},
        {{ 1.0f}, {-1.0f}, {0.0f}, {0.0f}, {0.0f}, {-1.0f}, {0.0f}, {0.0f}},
    },
    rotatedquad[] =
    {
        {{-10.0f}, {-11.0f}, {11.0f}, {-1.0f}, {0.0f}, {0.0f}, {0.0f}, {0.0f}},
        {{-10.0f}, { -9.0f}, {11.0f}, {-1.0f}, {0.0f}, {0.0f}, {0.0f}, {0.0f}},
        {{-10.0f}, { -9.0f}, { 9.0f}, {-1.0f}, {0.0f}, {0.0f}, {0.0f}, {0.0f}},
        {{-10.0f}, {-11.0f}, { 9.0f}, {-1.0f}, {0.0f}, {0.0f}, {0.0f}, {0.0f}},
    },
    translatedquad[] =
    {
        {{-11.0f}, {-11.0f}, {-10.0f}, {0.0f}, {0.0f}, {-1.0f}, {0.0f}, {0.0f}},
        {{-11.0f}, { -9.0f}, {-10.0f}, {0.0f}, {0.0f}, {-1.0f}, {0.0f}, {0.0f}},
        {{ -9.0f}, { -9.0f}, {-10.0f}, {0.0f}, {0.0f}, {-1.0f}, {0.0f}, {0.0f}},
        {{ -9.0f}, {-11.0f}, {-10.0f}, {0.0f}, {0.0f}, {-1.0f}, {0.0f}, {0.0f}},
    };
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
    IDirect3D *d3d;
    IDirect3DDevice *device;
    IDirectDraw *ddraw;
    IDirectDrawSurface *rt;
    IDirect3DViewport *viewport;
    IDirect3DMaterial *material;
    IDirect3DLight *light;
    IDirect3DExecuteBuffer *execute_buffer;
    D3DEXECUTEBUFFERDESC exec_desc;
    D3DMATERIALHANDLE mat_handle;
    D3DMATRIXHANDLE world_handle, view_handle, proj_handle;
    D3DLIGHT light_desc;
    HRESULT hr;
    D3DCOLOR color;
    void *ptr;
    UINT inst_length;
    ULONG refcount;
    unsigned int i;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    if (!(device = create_device(ddraw, window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        IDirectDraw_Release(ddraw);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get D3D interface, hr %#x.\n", hr);

    hr = IDirect3DDevice_QueryInterface(device, &IID_IDirectDrawSurface, (void **)&rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    viewport = create_viewport(device, 0, 0, 640, 480);
    material = create_diffuse_material(device, 1.0f, 1.0f, 1.0f, 1.0f);
    viewport_set_background(device, viewport, material);

    hr = IDirect3DViewport_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);

    hr = IDirect3DDevice_CreateMatrix(device, &world_handle);
    ok(hr == D3D_OK, "Creating a matrix object failed, hr %#x.\n", hr);
    hr = IDirect3DDevice_SetMatrix(device, world_handle, &mat);
    ok(hr == D3D_OK, "Setting a matrix object failed, hr %#x.\n", hr);
    hr = IDirect3DDevice_CreateMatrix(device, &view_handle);
    ok(hr == D3D_OK, "Creating a matrix object failed, hr %#x.\n", hr);
    hr = IDirect3DDevice_SetMatrix(device, view_handle, &mat);
    ok(hr == D3D_OK, "Setting a matrix object failed, hr %#x.\n", hr);
    hr = IDirect3DDevice_CreateMatrix(device, &proj_handle);
    ok(hr == D3D_OK, "Creating a matrix object failed, hr %#x.\n", hr);
    hr = IDirect3DDevice_SetMatrix(device, proj_handle, &mat);
    ok(hr == D3D_OK, "Setting a matrix object failed, hr %#x.\n", hr);

    memset(&exec_desc, 0, sizeof(exec_desc));
    exec_desc.dwSize = sizeof(exec_desc);
    exec_desc.dwFlags = D3DDEB_BUFSIZE | D3DDEB_CAPS;
    exec_desc.dwBufferSize = 1024;
    exec_desc.dwCaps = D3DDEBCAPS_SYSTEMMEMORY;

    hr = IDirect3DDevice_CreateExecuteBuffer(device, &exec_desc, &execute_buffer, NULL);
    ok(SUCCEEDED(hr), "Failed to create execute buffer, hr %#x.\n", hr);

    hr = IDirect3DExecuteBuffer_Lock(execute_buffer, &exec_desc);
    ok(SUCCEEDED(hr), "Failed to lock execute buffer, hr %#x.\n", hr);

    memcpy(exec_desc.lpData, unlitquad, sizeof(unlitquad));
    ptr = ((BYTE *)exec_desc.lpData) + sizeof(unlitquad);
    emit_set_ts(&ptr, D3DTRANSFORMSTATE_WORLD, world_handle);
    emit_set_ts(&ptr, D3DTRANSFORMSTATE_VIEW, view_handle);
    emit_set_ts(&ptr, D3DTRANSFORMSTATE_PROJECTION, proj_handle);
    emit_set_rs(&ptr, D3DRENDERSTATE_CLIPPING, FALSE);
    emit_set_rs(&ptr, D3DRENDERSTATE_ZENABLE, FALSE);
    emit_set_rs(&ptr, D3DRENDERSTATE_FOGENABLE, FALSE);
    emit_set_rs(&ptr, D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
    emit_process_vertices(&ptr, D3DPROCESSVERTICES_TRANSFORM, 0, 4);
    emit_tquad_tlist(&ptr, 0);
    emit_end(&ptr);
    inst_length = (BYTE *)ptr - (BYTE *)exec_desc.lpData;
    inst_length -= sizeof(unlitquad);

    hr = IDirect3DExecuteBuffer_Unlock(execute_buffer);
    ok(SUCCEEDED(hr), "Failed to unlock execute buffer, hr %#x.\n", hr);

    hr = IDirect3DDevice_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);

    set_execute_data(execute_buffer, 4, sizeof(unlitquad), inst_length);
    hr = IDirect3DDevice_Execute(device, execute_buffer, viewport, D3DEXECUTE_CLIPPED);
    ok(SUCCEEDED(hr), "Failed to execute exec buffer, hr %#x.\n", hr);

    hr = IDirect3DExecuteBuffer_Lock(execute_buffer, &exec_desc);
    ok(SUCCEEDED(hr), "Failed to lock execute buffer, hr %#x.\n", hr);

    memcpy(exec_desc.lpData, litquad, sizeof(litquad));
    ptr = ((BYTE *)exec_desc.lpData) + sizeof(litquad);
    emit_process_vertices(&ptr, D3DPROCESSVERTICES_TRANSFORM, 0, 4);
    emit_tquad_tlist(&ptr, 0);
    emit_end(&ptr);
    inst_length = (BYTE *)ptr - (BYTE *)exec_desc.lpData;
    inst_length -= sizeof(litquad);

    hr = IDirect3DExecuteBuffer_Unlock(execute_buffer);
    ok(SUCCEEDED(hr), "Failed to unlock execute buffer, hr %#x.\n", hr);

    set_execute_data(execute_buffer, 4, sizeof(litquad), inst_length);
    hr = IDirect3DDevice_Execute(device, execute_buffer, viewport, D3DEXECUTE_CLIPPED);
    ok(SUCCEEDED(hr), "Failed to execute exec buffer, hr %#x.\n", hr);

    hr = IDirect3DExecuteBuffer_Lock(execute_buffer, &exec_desc);
    ok(SUCCEEDED(hr), "Failed to lock execute buffer, hr %#x.\n", hr);

    memcpy(exec_desc.lpData, unlitnquad, sizeof(unlitnquad));
    ptr = ((BYTE *)exec_desc.lpData) + sizeof(unlitnquad);
    emit_process_vertices(&ptr, D3DPROCESSVERTICES_TRANSFORMLIGHT, 0, 4);
    emit_tquad_tlist(&ptr, 0);
    emit_end(&ptr);
    inst_length = (BYTE *)ptr - (BYTE *)exec_desc.lpData;
    inst_length -= sizeof(unlitnquad);

    hr = IDirect3DExecuteBuffer_Unlock(execute_buffer);
    ok(SUCCEEDED(hr), "Failed to unlock execute buffer, hr %#x.\n", hr);

    set_execute_data(execute_buffer, 4, sizeof(unlitnquad), inst_length);
    hr = IDirect3DDevice_Execute(device, execute_buffer, viewport, D3DEXECUTE_CLIPPED);
    ok(SUCCEEDED(hr), "Failed to execute exec buffer, hr %#x.\n", hr);

    hr = IDirect3DExecuteBuffer_Lock(execute_buffer, &exec_desc);
    ok(SUCCEEDED(hr), "Failed to lock execute buffer, hr %#x.\n", hr);

    memcpy(exec_desc.lpData, litnquad, sizeof(litnquad));
    ptr = ((BYTE *)exec_desc.lpData) + sizeof(litnquad);
    emit_process_vertices(&ptr, D3DPROCESSVERTICES_TRANSFORMLIGHT, 0, 4);
    emit_tquad_tlist(&ptr, 0);
    emit_end(&ptr);
    inst_length = (BYTE *)ptr - (BYTE *)exec_desc.lpData;
    inst_length -= sizeof(litnquad);

    hr = IDirect3DExecuteBuffer_Unlock(execute_buffer);
    ok(SUCCEEDED(hr), "Failed to unlock execute buffer, hr %#x.\n", hr);

    set_execute_data(execute_buffer, 4, sizeof(litnquad), inst_length);
    hr = IDirect3DDevice_Execute(device, execute_buffer, viewport, D3DEXECUTE_CLIPPED);
    ok(SUCCEEDED(hr), "Failed to execute exec buffer, hr %#x.\n", hr);

    hr = IDirect3DDevice_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = get_surface_color(rt, 160, 360);
    ok(color == 0x00ff0000, "Unlit quad without normals has color 0x%08x.\n", color);
    color = get_surface_color(rt, 160, 120);
    ok(color == 0x0000ff00, "Lit quad without normals has color 0x%08x.\n", color);
    color = get_surface_color(rt, 480, 360);
    ok(color == 0x00ffffff, "Unlit quad with normals has color 0x%08x.\n", color);
    color = get_surface_color(rt, 480, 120);
    ok(color == 0x00ffffff, "Lit quad with normals has color 0x%08x.\n", color);

    hr = IDirect3DMaterial_GetHandle(material, device, &mat_handle);
    ok(SUCCEEDED(hr), "Failed to get material handle, hr %#x.\n", hr);

    hr = IDirect3D_CreateLight(d3d, &light, NULL);
    ok(SUCCEEDED(hr), "Failed to create a light object, hr %#x.\n", hr);
    memset(&light_desc, 0, sizeof(light_desc));
    light_desc.dwSize = sizeof(light_desc);
    light_desc.dltType = D3DLIGHT_DIRECTIONAL;
    U1(light_desc.dcvColor).r = 0.0f;
    U2(light_desc.dcvColor).g = 0.0f;
    U3(light_desc.dcvColor).b = 1.0f;
    U4(light_desc.dcvColor).a = 1.0f;
    U3(light_desc.dvDirection).z = 1.0f;
    hr = IDirect3DLight_SetLight(light, &light_desc);
    ok(SUCCEEDED(hr), "Failed to set light, hr %#x.\n", hr);
    hr = IDirect3DViewport_AddLight(viewport, light);
    ok(SUCCEEDED(hr), "Failed to add a light to the viewport, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        hr = IDirect3DDevice_SetMatrix(device, world_handle, tests[i].world_matrix);
        ok(hr == D3D_OK, "Setting a matrix object failed, hr %#x.\n", hr);

        hr = IDirect3DViewport_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
        ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);

        hr = IDirect3DDevice_BeginScene(device);
        ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);

        hr = IDirect3DExecuteBuffer_Lock(execute_buffer, &exec_desc);
        ok(SUCCEEDED(hr), "Failed to lock execute buffer, hr %#x.\n", hr);

        memcpy(exec_desc.lpData, tests[i].quad, sizeof(nquad));
        ptr = ((BYTE *)exec_desc.lpData) + sizeof(nquad);
        emit_set_ls(&ptr, D3DLIGHTSTATE_MATERIAL, mat_handle);
        emit_process_vertices(&ptr, D3DPROCESSVERTICES_TRANSFORMLIGHT, 0, 4);
        emit_tquad_tlist(&ptr, 0);
        emit_end(&ptr);
        inst_length = (BYTE *)ptr - (BYTE *)exec_desc.lpData;
        inst_length -= sizeof(nquad);

        hr = IDirect3DExecuteBuffer_Unlock(execute_buffer);
        ok(SUCCEEDED(hr), "Failed to unlock execute buffer, hr %#x.\n", hr);

        set_execute_data(execute_buffer, 4, sizeof(nquad), inst_length);
        hr = IDirect3DDevice_Execute(device, execute_buffer, viewport, D3DEXECUTE_CLIPPED);
        ok(SUCCEEDED(hr), "Failed to execute exec buffer, hr %#x.\n", hr);

        hr = IDirect3DDevice_EndScene(device);
        ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

        color = get_surface_color(rt, 320, 240);
        todo_wine ok(color == tests[i].expected, "%s has color 0x%08x.\n", tests[i].message, color);
    }

    IDirect3DExecuteBuffer_Release(execute_buffer);
    IDirect3DDevice_DeleteMatrix(device, world_handle);
    IDirect3DDevice_DeleteMatrix(device, view_handle);
    IDirect3DDevice_DeleteMatrix(device, proj_handle);
    hr = IDirect3DViewport_DeleteLight(viewport, light);
    ok(SUCCEEDED(hr), "Failed to remove a light from the viewport, hr %#x.\n", hr);
    IDirect3DLight_Release(light);
    destroy_material(material);
    destroy_viewport(device, viewport);
    IDirectDrawSurface_Release(rt);
    refcount = IDirect3DDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D_Release(d3d);
    refcount = IDirectDraw_Release(ddraw);
    ok(!refcount, "Ddraw object has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_palette_gdi(void)
{
    IDirectDrawSurface *surface, *primary;
    DDSURFACEDESC surface_desc;
    IDirectDraw *ddraw;
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
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    surface_desc.dwWidth = 16;
    surface_desc.dwHeight = 16;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    surface_desc.ddpfPixelFormat.dwSize = sizeof(surface_desc.ddpfPixelFormat);
    surface_desc.ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED8 | DDPF_RGB;
    U1(surface_desc.ddpfPixelFormat).dwRGBBitCount = 8;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    /* Avoid colors from the Windows default palette. */
    memset(palette_entries, 0, sizeof(palette_entries));
    palette_entries[1].peRed = 0x01;
    palette_entries[2].peGreen = 0x02;
    palette_entries[3].peBlue = 0x03;
    palette_entries[4].peRed = 0x13;
    palette_entries[4].peGreen = 0x14;
    palette_entries[4].peBlue = 0x15;
    hr = IDirectDraw_CreatePalette(ddraw, DDPCAPS_8BIT | DDPCAPS_ALLOW256,
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

    hr = IDirectDrawSurface_SetPalette(surface, palette);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);
    hr = IDirectDrawSurface_GetDC(surface, &dc);
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
    hr = IDirectDrawSurface_SetPalette(surface, NULL);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);
    hr = IDirectDrawSurface_SetPalette(surface, palette);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);

    i = GetDIBColorTable(dc, 4, 1, &rgbquad[4]);
    ok(i == 1, "Expected count 1, got %u.\n", i);
    ok(!memcmp(&rgbquad[4], &expected1[4], sizeof(rgbquad[4])),
            "Got color table entry %u r=%#x g=%#x b=%#x, expected r=%#x g=%#x b=%#x.\n",
            i, rgbquad[4].rgbRed, rgbquad[4].rgbGreen, rgbquad[4].rgbBlue,
            expected1[4].rgbRed, expected1[4].rgbGreen, expected1[4].rgbBlue);

    hr = IDirectDrawSurface_ReleaseDC(surface, dc);
    ok(SUCCEEDED(hr), "Failed to release DC, hr %#x.\n", hr);

    /* Refresh the DC. This updates the palette. */
    hr = IDirectDrawSurface_GetDC(surface, &dc);
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
    hr = IDirectDrawSurface_ReleaseDC(surface, dc);
    ok(SUCCEEDED(hr), "Failed to release DC, hr %#x.\n", hr);

    refcount = IDirectDrawSurface_Release(surface);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);
    if (FAILED(IDirectDraw_SetDisplayMode(ddraw, 640, 480, 8)))
    {
        win_skip("Failed to set 8 bpp display mode, skipping test.\n");
        IDirectDrawPalette_Release(palette);
        IDirectDraw_Release(ddraw);
        DestroyWindow(window);
        return;
    }
    ok(SUCCEEDED(hr), "Failed to set display mode, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    U5(fx).dwFillColor = 3;
    SetRect(&r, 0, 0, 319, 479);
    hr = IDirectDrawSurface_Blt(primary, &r, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear surface, hr %#x.\n", hr);
    SetRect(&r, 320, 0, 639, 479);
    U5(fx).dwFillColor = 4;
    hr = IDirectDrawSurface_Blt(primary, &r, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface_SetPalette(primary, palette);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);
    hr = IDirectDrawSurface_GetDC(primary, &dc);
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
    hr = IDirectDrawSurface_ReleaseDC(primary, dc);
    ok(SUCCEEDED(hr), "Failed to release DC, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.dwWidth = 16;
    surface_desc.dwHeight = 16;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    /* Here the offscreen surface appears to use the primary's palette,
     * but in all likelihood it is actually the system palette. */
    hr = IDirectDrawSurface_GetDC(surface, &dc);
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
    hr = IDirectDrawSurface_ReleaseDC(surface, dc);
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
    hr = IDirectDraw_CreatePalette(ddraw, DDPCAPS_8BIT | DDPCAPS_ALLOW256,
            palette_entries, &palette2, NULL);
    ok(SUCCEEDED(hr), "Failed to create palette, hr %#x.\n", hr);
    hr = IDirectDrawSurface_SetPalette(surface, palette2);
    ok(SUCCEEDED(hr), "Failed to set palette, hr %#x.\n", hr);

    /* A palette assigned to the offscreen surface overrides the primary / system
     * palette. */
    hr = IDirectDrawSurface_GetDC(surface, &dc);
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
    hr = IDirectDrawSurface_ReleaseDC(surface, dc);
    ok(SUCCEEDED(hr), "Failed to release DC, hr %#x.\n", hr);

    refcount = IDirectDrawSurface_Release(surface);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);

    /* The Windows 8 testbot keeps extra references to the primary and
     * backbuffer while in 8 bpp mode. */
    hr = IDirectDraw_RestoreDisplayMode(ddraw);
    ok(SUCCEEDED(hr), "Failed to restore display mode, hr %#x.\n", hr);

    refcount = IDirectDrawSurface_Release(primary);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDrawPalette_Release(palette2);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDrawPalette_Release(palette);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDraw_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_palette_alpha(void)
{
    IDirectDrawSurface *surface;
    DDSURFACEDESC surface_desc;
    IDirectDraw *ddraw;
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
    if (FAILED(IDirectDraw_SetDisplayMode(ddraw, 640, 480, 8)))
    {
        win_skip("Failed to set 8 bpp display mode, skipping test.\n");
        IDirectDraw_Release(ddraw);
        DestroyWindow(window);
        return;
    }
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(palette_entries, 0, sizeof(palette_entries));
    palette_entries[1].peFlags = 0x42;
    palette_entries[2].peFlags = 0xff;
    palette_entries[3].peFlags = 0x80;
    hr = IDirectDraw_CreatePalette(ddraw, DDPCAPS_ALLOW256 | DDPCAPS_8BIT, palette_entries, &palette, NULL);
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
    hr = IDirectDraw_CreatePalette(ddraw, DDPCAPS_ALLOW256 | DDPCAPS_8BIT | DDPCAPS_ALPHA,
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
        hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
        ok(SUCCEEDED(hr), "Failed to create %s surface, hr %#x.\n", test_data[i].name, hr);

        hr = IDirectDrawSurface_SetPalette(surface, palette);
        if (test_data[i].attach_allowed)
            ok(SUCCEEDED(hr), "Failed to attach palette to %s surface, hr %#x.\n", test_data[i].name, hr);
        else
            ok(hr == DDERR_INVALIDSURFACETYPE, "Got unexpected hr %#x, %s surface.\n", hr, test_data[i].name);

        if (SUCCEEDED(hr))
        {
            HDC dc;
            RGBQUAD rgbquad;
            UINT retval;

            hr = IDirectDrawSurface_GetDC(surface, &dc);
            ok(SUCCEEDED(hr) || broken(hr == DDERR_CANTCREATEDC) /* Win2k testbot */,
                    "Failed to get DC, hr %#x, %s surface.\n", hr, test_data[i].name);
            if (SUCCEEDED(hr))
            {
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
                hr = IDirectDrawSurface_ReleaseDC(surface, dc);
                ok(SUCCEEDED(hr), "Failed to release DC, hr %#x.\n", hr);
            }
        }
        IDirectDrawSurface_Release(surface);
    }

    /* Test INVALIDSURFACETYPE vs INVALIDPIXELFORMAT. */
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    surface_desc.dwWidth = 128;
    surface_desc.dwHeight = 128;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    surface_desc.ddpfPixelFormat.dwSize = sizeof(surface_desc.ddpfPixelFormat);
    surface_desc.ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(surface_desc.ddpfPixelFormat).dwRGBBitCount = 32;
    U2(surface_desc.ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(surface_desc.ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(surface_desc.ddpfPixelFormat).dwBBitMask = 0x000000ff;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface_SetPalette(surface, palette);
    ok(hr == DDERR_INVALIDSURFACETYPE, "Got unexpected hr %#x.\n", hr);
    IDirectDrawSurface_Release(surface);

    /* The Windows 8 testbot keeps extra references to the primary
     * while in 8 bpp mode. */
    hr = IDirectDraw_RestoreDisplayMode(ddraw);
    ok(SUCCEEDED(hr), "Failed to restore display mode, hr %#x.\n", hr);

    refcount = IDirectDrawPalette_Release(palette);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    refcount = IDirectDraw_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_lost_device(void)
{
    IDirectDrawSurface *surface;
    DDSURFACEDESC surface_desc;
    HWND window1, window2;
    IDirectDraw *ddraw;
    ULONG refcount;
    HRESULT hr;
    BOOL ret;

    window1 = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    window2 = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window1, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;
    surface_desc.dwBackBufferCount = 1;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface_IsLost(surface);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Flip(surface, NULL, DDFLIP_WAIT);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);

    ret = SetForegroundWindow(GetDesktopWindow());
    ok(ret, "Failed to set foreground window.\n");
    hr = IDirectDrawSurface_IsLost(surface);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Flip(surface, NULL, DDFLIP_WAIT);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);

    ret = SetForegroundWindow(window1);
    ok(ret, "Failed to set foreground window.\n");
    hr = IDirectDrawSurface_IsLost(surface);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Flip(surface, NULL, DDFLIP_WAIT);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);

    hr = restore_surfaces(ddraw);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_IsLost(surface);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Flip(surface, NULL, DDFLIP_WAIT);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window1, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_IsLost(surface);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Flip(surface, NULL, DDFLIP_WAIT);
    ok(hr == DDERR_NOEXCLUSIVEMODE, "Got unexpected hr %#x.\n", hr);

    /* Trying to restore the primary will crash, probably because flippable
     * surfaces can't exist in DDSCL_NORMAL. */
    IDirectDrawSurface_Release(surface);
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface_IsLost(surface);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);

    ret = SetForegroundWindow(GetDesktopWindow());
    ok(ret, "Failed to set foreground window.\n");
    hr = IDirectDrawSurface_IsLost(surface);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);

    ret = SetForegroundWindow(window1);
    ok(ret, "Failed to set foreground window.\n");
    hr = IDirectDrawSurface_IsLost(surface);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window1, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_IsLost(surface);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);

    hr = restore_surfaces(ddraw);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_IsLost(surface);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);

    IDirectDrawSurface_Release(surface);
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;
    surface_desc.dwBackBufferCount = 1;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window1, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_IsLost(surface);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Flip(surface, NULL, DDFLIP_WAIT);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window1, DDSCL_NORMAL | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_IsLost(surface);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Flip(surface, NULL, DDFLIP_WAIT);
    ok(hr == DDERR_NOEXCLUSIVEMODE, "Got unexpected hr %#x.\n", hr);

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window1, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_IsLost(surface);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Flip(surface, NULL, DDFLIP_WAIT);
    ok(hr == DDERR_NOEXCLUSIVEMODE, "Got unexpected hr %#x.\n", hr);

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window2, DDSCL_NORMAL);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_IsLost(surface);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Flip(surface, NULL, DDFLIP_WAIT);
    ok(hr == DDERR_NOEXCLUSIVEMODE, "Got unexpected hr %#x.\n", hr);

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window2, DDSCL_NORMAL | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_IsLost(surface);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Flip(surface, NULL, DDFLIP_WAIT);
    ok(hr == DDERR_NOEXCLUSIVEMODE, "Got unexpected hr %#x.\n", hr);

    hr = IDirectDraw_SetCooperativeLevel(ddraw, window2, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(hr == DD_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_IsLost(surface);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Flip(surface, NULL, DDFLIP_WAIT);
    ok(hr == DDERR_SURFACELOST, "Got unexpected hr %#x.\n", hr);

    IDirectDrawSurface_Release(surface);
    refcount = IDirectDraw_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window2);
    DestroyWindow(window1);
}

static void test_surface_desc_lock(void)
{
    IDirectDrawSurface *surface;
    DDSURFACEDESC surface_desc;
    IDirectDraw *ddraw;
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
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    surface_desc.dwWidth = 16;
    surface_desc.dwHeight = 16;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    memset(&surface_desc, 0xaa, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(!surface_desc.lpSurface, "Got unexpected lpSurface %p.\n", surface_desc.lpSurface);

    memset(&surface_desc, 0xaa, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface_Lock(surface, NULL, &surface_desc, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    ok(surface_desc.lpSurface != NULL, "Got unexpected lpSurface %p.\n", surface_desc.lpSurface);
    memset(&surface_desc, 0xaa, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(!surface_desc.lpSurface, "Got unexpected lpSurface %p.\n", surface_desc.lpSurface);
    hr = IDirectDrawSurface_Unlock(surface, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    memset(&surface_desc, 0xaa, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(!surface_desc.lpSurface, "Got unexpected lpSurface %p.\n", surface_desc.lpSurface);

    IDirectDrawSurface_Release(surface);
    refcount = IDirectDraw_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_texturemapblend(void)
{
    HRESULT hr;
    DDSURFACEDESC ddsd;
    D3DEXECUTEBUFFERDESC exec_desc;
    DDBLTFX fx;
    static RECT rect = {0, 0, 64, 128};
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    DDCOLORKEY ckey;
    IDirectDrawSurface *surface, *rt;
    IDirect3DTexture *texture;
    D3DTEXTUREHANDLE texture_handle;
    HWND window;
    IDirectDraw *ddraw;
    IDirect3DDevice *device;
    IDirect3DMaterial *material;
    IDirect3DViewport *viewport;
    IDirect3DExecuteBuffer *execute_buffer;
    UINT inst_length;
    void *ptr;
    ULONG ref;
    D3DCOLOR color;

    static const D3DTLVERTEX test1_quads[] =
    {
        {{0.0f},   {0.0f},   {0.0f}, {1.0f}, {0xffffffff}, {0}, {0.0f}, {0.0f}},
        {{0.0f},   {240.0f}, {0.0f}, {1.0f}, {0xffffffff}, {0}, {0.0f}, {1.0f}},
        {{640.0f}, {0.0f},   {0.0f}, {1.0f}, {0xffffffff}, {0}, {1.0f}, {0.0f}},
        {{640.0f}, {240.0f}, {0.0f}, {1.0f}, {0xffffffff}, {0}, {1.0f}, {1.0f}},
        {{0.0f},   {240.0f}, {0.0f}, {1.0f}, {0x80ffffff}, {0}, {0.0f}, {0.0f}},
        {{0.0f},   {480.0f}, {0.0f}, {1.0f}, {0x80ffffff}, {0}, {0.0f}, {1.0f}},
        {{640.0f}, {240.0f}, {0.0f}, {1.0f}, {0x80ffffff}, {0}, {1.0f}, {0.0f}},
        {{640.0f}, {480.0f}, {0.0f}, {1.0f}, {0x80ffffff}, {0}, {1.0f}, {1.0f}},
    },
    test2_quads[] =
    {
        {{0.0f},   {0.0f},   {0.0f}, {1.0f}, {0x00ff0080}, {0}, {0.0f}, {0.0f}},
        {{0.0f},   {240.0f}, {0.0f}, {1.0f}, {0x00ff0080}, {0}, {0.0f}, {1.0f}},
        {{640.0f}, {0.0f},   {0.0f}, {1.0f}, {0x00ff0080}, {0}, {1.0f}, {0.0f}},
        {{640.0f}, {240.0f}, {0.0f}, {1.0f}, {0x00ff0080}, {0}, {1.0f}, {1.0f}},
        {{0.0f},   {240.0f}, {0.0f}, {1.0f}, {0x008000ff}, {0}, {0.0f}, {0.0f}},
        {{0.0f},   {480.0f}, {0.0f}, {1.0f}, {0x008000ff}, {0}, {0.0f}, {1.0f}},
        {{640.0f}, {240.0f}, {0.0f}, {1.0f}, {0x008000ff}, {0}, {1.0f}, {0.0f}},
        {{640.0f}, {480.0f}, {0.0f}, {1.0f}, {0x008000ff}, {0}, {1.0f}, {1.0f}},
    };

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    if (!(device = create_device(ddraw, window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        IDirectDraw_Release(ddraw);
        return;
    }

    hr = IDirect3DDevice_QueryInterface(device, &IID_IDirectDrawSurface, (void **)&rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    material = create_diffuse_material(device, 0.0f, 0.0f, 0.0f, 1.0f);
    viewport = create_viewport(device, 0, 0, 640, 480);
    viewport_set_background(device, viewport, material);

    memset(&exec_desc, 0, sizeof(exec_desc));
    exec_desc.dwSize = sizeof(exec_desc);
    exec_desc.dwFlags = D3DDEB_BUFSIZE | D3DDEB_CAPS;
    exec_desc.dwBufferSize = 1024;
    exec_desc.dwCaps = D3DDEBCAPS_SYSTEMMEMORY;
    hr = IDirect3DDevice_CreateExecuteBuffer(device, &exec_desc, &execute_buffer, NULL);
    ok(SUCCEEDED(hr), "Failed to create execute buffer, hr %#x.\n", hr);

    /* Test alpha with DDPF_ALPHAPIXELS texture - should be taken from texture alpha channel.
     *
     * The vertex alpha is completely ignored in this case, so case 1 and 2 combined are not
     * a D3DTOP_MODULATE with texture alpha = 0xff in case 2 (no alpha in texture). */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.dwHeight = 128;
    ddsd.dwWidth = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    U1(ddsd.ddpfPixelFormat).dwRGBBitCount = 32;
    U2(ddsd.ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(ddsd.ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(ddsd.ddpfPixelFormat).dwBBitMask = 0x000000ff;
    U5(ddsd.ddpfPixelFormat).dwRGBAlphaBitMask = 0xff000000;
    hr = IDirectDraw_CreateSurface(ddraw, &ddsd, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface_QueryInterface(surface, &IID_IDirect3DTexture, (void **)&texture);
    ok(SUCCEEDED(hr), "Failed to get texture interface, hr %#x.\n", hr);
    hr = IDirect3DTexture_GetHandle(texture, device, &texture_handle);
    ok(SUCCEEDED(hr), "Failed to get texture handle, hr %#x.\n", hr);

    hr = IDirect3DViewport_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear render target, hr %#x.\n", hr);

    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    U5(fx).dwFillColor = 0xff0000ff;
    hr = IDirectDrawSurface_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear texture, hr %#x.\n", hr);
    U5(fx).dwFillColor = 0x800000ff;
    hr = IDirectDrawSurface_Blt(surface, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear texture, hr %#x.\n", hr);

    hr = IDirect3DExecuteBuffer_Lock(execute_buffer, &exec_desc);
    ok(SUCCEEDED(hr), "Failed to lock execute buffer, hr %#x.\n", hr);

    memcpy(exec_desc.lpData, test1_quads, sizeof(test1_quads));

    ptr = ((BYTE *)exec_desc.lpData) + sizeof(test1_quads);
    emit_process_vertices(&ptr, D3DPROCESSVERTICES_COPY, 0, 8);
    emit_set_rs(&ptr, D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
    emit_set_rs(&ptr, D3DRENDERSTATE_ZENABLE, D3DZB_FALSE);
    emit_set_rs(&ptr, D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
    emit_set_rs(&ptr, D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
    /* The history of D3DRENDERSTATE_ALPHABLENDENABLE is quite a mess. In the
     * first D3D release there was a D3DRENDERSTATE_BLENDENABLE (enum value 27).
     * D3D5 introduced a new and separate D3DRENDERSTATE_ALPHABLENDENABLE (42)
     * together with D3DRENDERSTATE_COLORKEYENABLE (41). The docs aren't all
     * that clear but they mention that D3DRENDERSTATE_BLENDENABLE overrides the
     * two new states.
     * Then D3D6 came and got rid of the new D3DRENDERSTATE_ALPHABLENDENABLE
     * state (42), renaming the older D3DRENDERSTATE_BLENDENABLE enum (27)
     * as D3DRENDERSTATE_ALPHABLENDENABLE.
     * There is a comment in the D3D6 docs which mentions that hardware
     * rasterizers always used D3DRENDERSTATE_BLENDENABLE to just toggle alpha
     * blending while prior to D3D5 software rasterizers toggled both color
     * keying and alpha blending according to it. What I gather is that, from
     * D3D6 onwards, D3DRENDERSTATE_ALPHABLENDENABLE always only toggles the
     * alpha blending state.
     * These tests seem to show that actual, current hardware follows the D3D6
     * behavior even when using the original D3D interfaces, for the HAL device
     * at least. */
    emit_set_rs(&ptr, D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
    emit_set_rs(&ptr, D3DRENDERSTATE_TEXTUREMAPBLEND, D3DTBLEND_MODULATE);
    emit_set_rs(&ptr, D3DRENDERSTATE_TEXTUREHANDLE, texture_handle);

    emit_tquad(&ptr, 0);
    emit_tquad(&ptr, 4);
    emit_end(&ptr);

    inst_length = (BYTE *)ptr - (BYTE *)exec_desc.lpData;
    inst_length -= sizeof(test1_quads);
    hr = IDirect3DExecuteBuffer_Unlock(execute_buffer);
    ok(SUCCEEDED(hr), "Failed to unlock execute buffer, hr %#x.\n", hr);
    set_execute_data(execute_buffer, 8, sizeof(test1_quads), inst_length);

    hr = IDirect3DDevice_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice_Execute(device, execute_buffer, viewport, D3DEXECUTE_UNCLIPPED);
    ok(SUCCEEDED(hr), "Failed to execute exec buffer, hr %#x.\n", hr);
    hr = IDirect3DDevice_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = get_surface_color(rt, 5, 5);
    ok(compare_color(color, 0x00000080, 2), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 400, 5);
    ok(compare_color(color, 0x000000ff, 2), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 5, 245);
    ok(compare_color(color, 0x00000080, 2), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 400, 245);
    ok(compare_color(color, 0x000000ff, 2), "Got unexpected color 0x%08x.\n", color);

    IDirect3DTexture_Release(texture);
    ref = IDirectDrawSurface_Release(surface);
    ok(ref == 0, "Surface not properly released, refcount %u.\n", ref);

    /* Test alpha with texture that has no alpha channel - alpha should be taken from diffuse vertex color. */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.dwHeight = 128;
    ddsd.dwWidth = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(ddsd.ddpfPixelFormat).dwRGBBitCount = 32;
    U2(ddsd.ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(ddsd.ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(ddsd.ddpfPixelFormat).dwBBitMask = 0x000000ff;

    hr = IDirectDraw_CreateSurface(ddraw, &ddsd, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface_QueryInterface(surface, &IID_IDirect3DTexture, (void **)&texture);
    ok(SUCCEEDED(hr), "Failed to get texture interface, hr %#x.\n", hr);
    hr = IDirect3DTexture_GetHandle(texture, device, &texture_handle);
    ok(SUCCEEDED(hr), "Failed to get texture handle, hr %#x.\n", hr);

    hr = IDirect3DViewport_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear render target, hr %#x.\n", hr);

    U5(fx).dwFillColor = 0xff0000ff;
    hr = IDirectDrawSurface_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear texture, hr %#x.\n", hr);
    U5(fx).dwFillColor = 0x800000ff;
    hr = IDirectDrawSurface_Blt(surface, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear texture, hr %#x.\n", hr);

    hr = IDirect3DExecuteBuffer_Lock(execute_buffer, &exec_desc);
    ok(SUCCEEDED(hr), "Failed to lock execute buffer, hr %#x.\n", hr);

    ptr = ((BYTE *)exec_desc.lpData) + sizeof(test1_quads);
    emit_process_vertices(&ptr, D3DPROCESSVERTICES_COPY, 0, 8);
    emit_set_rs(&ptr, D3DRENDERSTATE_TEXTUREHANDLE, texture_handle);

    emit_tquad(&ptr, 0);
    emit_tquad(&ptr, 4);
    emit_end(&ptr);

    inst_length = (BYTE *)ptr - (BYTE *)exec_desc.lpData;
    inst_length -= sizeof(test1_quads);
    hr = IDirect3DExecuteBuffer_Unlock(execute_buffer);
    ok(SUCCEEDED(hr), "Failed to unlock execute buffer, hr %#x.\n", hr);
    set_execute_data(execute_buffer, 8, sizeof(test1_quads), inst_length);

    hr = IDirect3DDevice_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice_Execute(device, execute_buffer, viewport, D3DEXECUTE_UNCLIPPED);
    ok(SUCCEEDED(hr), "Failed to execute exec buffer, hr %#x.\n", hr);
    hr = IDirect3DDevice_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    color = get_surface_color(rt, 5, 5);
    ok(compare_color(color, 0x000000ff, 2), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 400, 5);
    ok(compare_color(color, 0x000000ff, 2), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 5, 245);
    ok(compare_color(color, 0x00000080, 2), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 400, 245);
    ok(compare_color(color, 0x00000080, 2), "Got unexpected color 0x%08x.\n", color);

    IDirect3DTexture_Release(texture);
    ref = IDirectDrawSurface_Release(surface);
    ok(ref == 0, "Surface not properly released, refcount %u.\n", ref);

    /* Test RGB - should multiply color components from diffuse vertex color and texture. */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.dwHeight = 128;
    ddsd.dwWidth = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    U1(ddsd.ddpfPixelFormat).dwRGBBitCount = 32;
    U2(ddsd.ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(ddsd.ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(ddsd.ddpfPixelFormat).dwBBitMask = 0x000000ff;
    U5(ddsd.ddpfPixelFormat).dwRGBAlphaBitMask = 0xff000000;
    hr = IDirectDraw_CreateSurface(ddraw, &ddsd, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface_QueryInterface(surface, &IID_IDirect3DTexture, (void **)&texture);
    ok(SUCCEEDED(hr), "Failed to get texture interface, hr %#x.\n", hr);
    hr = IDirect3DTexture_GetHandle(texture, device, &texture_handle);
    ok(SUCCEEDED(hr), "Failed to get texture handle, hr %#x.\n", hr);

    hr = IDirect3DViewport_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear render target, hr %#x.\n", hr);

    U5(fx).dwFillColor = 0x00ffffff;
    hr = IDirectDrawSurface_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear texture, hr %#x.\n", hr);
    U5(fx).dwFillColor = 0x00ffff80;
    hr = IDirectDrawSurface_Blt(surface, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear texture, hr %#x.\n", hr);

    hr = IDirect3DExecuteBuffer_Lock(execute_buffer, &exec_desc);
    ok(SUCCEEDED(hr), "Failed to lock execute buffer, hr %#x.\n", hr);

    memcpy(exec_desc.lpData, test2_quads, sizeof(test2_quads));

    ptr = ((BYTE *)exec_desc.lpData) + sizeof(test2_quads);
    emit_process_vertices(&ptr, D3DPROCESSVERTICES_COPY, 0, 8);
    emit_set_rs(&ptr, D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
    emit_set_rs(&ptr, D3DRENDERSTATE_TEXTUREHANDLE, texture_handle);

    emit_tquad(&ptr, 0);
    emit_tquad(&ptr, 4);
    emit_end(&ptr);

    inst_length = (BYTE *)ptr - (BYTE *)exec_desc.lpData;
    inst_length -= sizeof(test2_quads);
    hr = IDirect3DExecuteBuffer_Unlock(execute_buffer);
    ok(SUCCEEDED(hr), "Failed to unlock execute buffer, hr %#x.\n", hr);
    set_execute_data(execute_buffer, 8, sizeof(test2_quads), inst_length);

    hr = IDirect3DDevice_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice_Execute(device, execute_buffer, viewport, D3DEXECUTE_UNCLIPPED);
    ok(SUCCEEDED(hr), "Failed to execute exec buffer, hr %#x.\n", hr);
    hr = IDirect3DDevice_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    /* WARP (Win8 testbot) emulates color keying with the alpha channel like Wine does,
     * but even applies it when there's no color key assigned. The surface alpha is zero
     * here, so nothing gets drawn.
     *
     * The ddraw2 version of this test draws these quads with color keying off due to
     * different defaults in ddraw1 and ddraw2. */
    color = get_surface_color(rt, 5, 5);
    ok(compare_color(color, 0x00ff0040, 2) || broken(compare_color(color, 0x00000000, 1)),
            "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 400, 5);
    ok(compare_color(color, 0x00ff0080, 2) || broken(compare_color(color, 0x00000000, 1)),
            "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 5, 245);
    ok(compare_color(color, 0x00800080, 2) || broken(compare_color(color, 0x00000000, 1)),
            "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 400, 245);
    ok(compare_color(color, 0x008000ff, 2) || broken(compare_color(color, 0x00000000, 1)),
            "Got unexpected color 0x%08x.\n", color);

    IDirect3DTexture_Release(texture);
    ref = IDirectDrawSurface_Release(surface);
    ok(ref == 0, "Surface not properly released, refcount %u.\n", ref);

    /* Test alpha again, now with color keyed texture (colorkey emulation in wine can interfere). */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.dwHeight = 128;
    ddsd.dwWidth = 128;
    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(ddsd.ddpfPixelFormat).dwRGBBitCount = 16;
    U2(ddsd.ddpfPixelFormat).dwRBitMask = 0xf800;
    U3(ddsd.ddpfPixelFormat).dwGBitMask = 0x07e0;
    U4(ddsd.ddpfPixelFormat).dwBBitMask = 0x001f;

    hr = IDirectDraw_CreateSurface(ddraw, &ddsd, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface_QueryInterface(surface, &IID_IDirect3DTexture, (void **)&texture);
    ok(SUCCEEDED(hr), "Failed to get texture interface, hr %#x.\n", hr);
    hr = IDirect3DTexture_GetHandle(texture, device, &texture_handle);
    ok(SUCCEEDED(hr), "Failed to get texture handle, hr %#x.\n", hr);

    hr = IDirect3DViewport_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear render target, hr %#x.\n", hr);

    U5(fx).dwFillColor = 0xf800;
    hr = IDirectDrawSurface_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear texture, hr %#x.\n", hr);
    U5(fx).dwFillColor = 0x001f;
    hr = IDirectDrawSurface_Blt(surface, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to clear texture, hr %#x.\n", hr);

    ckey.dwColorSpaceLowValue = 0x001f;
    ckey.dwColorSpaceHighValue = 0x001f;
    hr = IDirectDrawSurface_SetColorKey(surface, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);

    hr = IDirect3DExecuteBuffer_Lock(execute_buffer, &exec_desc);
    ok(SUCCEEDED(hr), "Failed to lock execute buffer, hr %#x.\n", hr);

    memcpy(exec_desc.lpData, test1_quads, sizeof(test1_quads));

    ptr = ((BYTE *)exec_desc.lpData) + sizeof(test1_quads);
    emit_process_vertices(&ptr, D3DPROCESSVERTICES_COPY, 0, 8);
    emit_set_rs(&ptr, D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
    emit_set_rs(&ptr, D3DRENDERSTATE_TEXTUREHANDLE, texture_handle);
    /* D3DRENDERSTATE_COLORKEYENABLE is supposed to be on by default on version
     * 1 devices, but for some reason it randomly defaults to FALSE on the W8
     * testbot. This is either the fault of Windows 8 or the WARP driver.
     * Also D3DRENDERSTATE_COLORKEYENABLE was introduced in D3D 5 aka version 2
     * devices only, which might imply this doesn't actually do anything on
     * WARP. */
    emit_set_rs(&ptr, D3DRENDERSTATE_COLORKEYENABLE, TRUE);

    emit_tquad(&ptr, 0);
    emit_tquad(&ptr, 4);
    emit_end(&ptr);

    inst_length = (BYTE *)ptr - (BYTE *)exec_desc.lpData;
    inst_length -= sizeof(test1_quads);
    hr = IDirect3DExecuteBuffer_Unlock(execute_buffer);
    ok(SUCCEEDED(hr), "Failed to unlock execute buffer, hr %#x.\n", hr);
    set_execute_data(execute_buffer, 8, sizeof(test1_quads), inst_length);

    hr = IDirect3DDevice_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice_Execute(device, execute_buffer, viewport, D3DEXECUTE_UNCLIPPED);
    ok(SUCCEEDED(hr), "Failed to execute exec buffer, hr %#x.\n", hr);
    hr = IDirect3DDevice_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    /* Allow broken WARP results (colorkey disabled). */
    color = get_surface_color(rt, 5, 5);
    ok(compare_color(color, 0x00000000, 2) || broken(compare_color(color, 0x000000ff, 2)),
            "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 400, 5);
    ok(compare_color(color, 0x00ff0000, 2), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 5, 245);
    ok(compare_color(color, 0x00000000, 2) || broken(compare_color(color, 0x00000080, 2)),
            "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 400, 245);
    ok(compare_color(color, 0x00800000, 2), "Got unexpected color 0x%08x.\n", color);

    IDirect3DTexture_Release(texture);
    ref = IDirectDrawSurface_Release(surface);
    ok(ref == 0, "Surface not properly released, refcount %u.\n", ref);

    ref = IDirect3DExecuteBuffer_Release(execute_buffer);
    ok(ref == 0, "Execute buffer not properly released, refcount %u.\n", ref);
    destroy_viewport(device, viewport);
    ref = IDirect3DMaterial_Release(material);
    ok(ref == 0, "Material not properly released, refcount %u.\n", ref);
    IDirectDrawSurface_Release(rt);
    IDirect3DDevice_Release(device);
    ref = IDirectDraw_Release(ddraw);
    ok(ref == 0, "Ddraw object not properly released, refcount %u.\n", ref);
    DestroyWindow(window);
}

static void test_viewport_clear_rect(void)
{
    HRESULT hr;
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    static D3DRECT clear_rect2 = {{90}, {90}, {110}, {110}};
    IDirectDrawSurface *rt;
    HWND window;
    IDirectDraw *ddraw;
    IDirect3DDevice *device;
    IDirect3DMaterial *red, *green;
    IDirect3DViewport *viewport, *viewport2;
    ULONG ref;
    D3DCOLOR color;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    if (!(device = create_device(ddraw, window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        IDirectDraw_Release(ddraw);
        return;
    }

    hr = IDirect3DDevice_QueryInterface(device, &IID_IDirectDrawSurface, (void **)&rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    red = create_diffuse_material(device, 1.0f, 0.0f, 0.0f, 1.0f);
    viewport = create_viewport(device, 0, 0, 640, 480);
    viewport_set_background(device, viewport, red);
    hr = IDirect3DViewport_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);

    green = create_diffuse_material(device, 0.0f, 1.0f, 0.0f, 1.0f);
    viewport2 = create_viewport(device, 100, 100, 20, 20);
    viewport_set_background(device, viewport2, green);
    hr = IDirect3DViewport_Clear(viewport2, 1, &clear_rect2, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);

    color = get_surface_color(rt, 85, 85); /* Outside both. */
    ok(compare_color(color, 0x00ff0000, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 95, 95); /* Outside vp, inside rect. */
    /* AMD GPUs ignore the viewport dimensions and only care about the rectangle. */
    ok(compare_color(color, 0x00ff0000, 1) || broken(compare_color(color, 0x0000ff00, 1)),
            "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 105, 105); /* Inside both. */
    ok(compare_color(color, 0x0000ff00, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 115, 115); /* Inside vp, outside rect. */
    ok(compare_color(color, 0x00ff0000, 1), "Got unexpected color 0x%08x.\n", color);
    color = get_surface_color(rt, 125, 125); /* Outside both. */
    ok(compare_color(color, 0x00ff0000, 1), "Got unexpected color 0x%08x.\n", color);

    destroy_viewport(device, viewport2);
    destroy_material(green);
    destroy_viewport(device, viewport);
    destroy_material(red);
    IDirectDrawSurface_Release(rt);
    IDirect3DDevice_Release(device);
    ref = IDirectDraw_Release(ddraw);
    ok(ref == 0, "Ddraw object not properly released, refcount %u.\n", ref);
    DestroyWindow(window);
}

static void test_color_fill(void)
{
    HRESULT hr;
    IDirect3DDevice *device;
    IDirectDraw *ddraw;
    IDirectDrawSurface *surface, *surface2;
    DDSURFACEDESC surface_desc;
    ULONG refcount;
    HWND window;
    unsigned int i;
    BOOL is_warp;
    DDBLTFX fx;
    RECT rect = {5, 5, 7, 7};
    DWORD *color;
    DWORD num_fourcc_codes, *fourcc_codes;
    DDCAPS hal_caps;
    BOOL support_uyvy = FALSE, support_yuy2 = FALSE;
    static const struct
    {
        DWORD caps;
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
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY,
            DD_OK, DDERR_INVALIDPARAMS, TRUE, "vidmem offscreenplain RGB", 0xdeadbeef, TRUE,
            {
                sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_ALPHAPIXELS, 0,
                {32}, {0x00ff0000}, {0x0000ff00}, {0x000000ff}, {0xff000000}
            }
        },
        {
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY,
            DD_OK, DDERR_INVALIDPARAMS, TRUE, "sysmem offscreenplain RGB", 0xdeadbeef, TRUE,
            {
                sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_ALPHAPIXELS, 0,
                {32}, {0x00ff0000}, {0x0000ff00}, {0x000000ff}, {0xff000000}
            }
        },
        {
            DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY,
            DD_OK, DDERR_INVALIDPARAMS, TRUE, "vidmem texture RGB", 0xdeadbeef, TRUE,
            {
                sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_ALPHAPIXELS, 0,
                {32}, {0x00ff0000}, {0x0000ff00}, {0x000000ff}, {0xff000000}
            }
        },
        {
            DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY,
            DD_OK, DDERR_INVALIDPARAMS, TRUE, "sysmem texture RGB", 0xdeadbeef, TRUE,
            {
                sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_ALPHAPIXELS, 0,
                {32}, {0x00ff0000}, {0x0000ff00}, {0x000000ff}, {0xff000000}
            }
        },
        {
            DDSCAPS_ZBUFFER | DDSCAPS_VIDEOMEMORY,
            DDERR_INVALIDPARAMS, DD_OK, TRUE, "vidmem zbuffer", 0xdeadbeef, TRUE,
            {0, 0, 0, {0}, {0}, {0}, {0}, {0}}
        },
        {
            /* Colorfill on YUV surfaces always returns DD_OK, but the content is
             * different afterwards. DX9+ GPUs set one of the two luminance values
             * in each block, but AMD and Nvidia GPUs disagree on which luminance
             * value they set. r200 (dx8) just sets the entire block to the clear
             * value. */
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY,
            DD_OK, DDERR_INVALIDPARAMS, FALSE, "vidmem offscreenplain YUY2", 0, FALSE,
            {
                sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('Y', 'U', 'Y', '2'),
                {0}, {0}, {0}, {0}, {0}
            }
        },
        {
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY,
            DD_OK, DDERR_INVALIDPARAMS, FALSE, "vidmem offscreenplain UYVY", 0, FALSE,
            {
                sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('U', 'Y', 'V', 'Y'),
                {0}, {0}, {0}, {0}, {0}
            }
        },
        {
            DDSCAPS_OVERLAY | DDSCAPS_VIDEOMEMORY,
            DD_OK, DDERR_INVALIDPARAMS, FALSE, "vidmem overlay YUY2", 0, FALSE,
            {
                sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('Y', 'U', 'Y', '2'),
                {0}, {0}, {0}, {0}, {0}
            }
        },
        {
            DDSCAPS_OVERLAY | DDSCAPS_VIDEOMEMORY,
            DD_OK, DDERR_INVALIDPARAMS, FALSE, "vidmem overlay UYVY", 0, FALSE,
            {
                sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('U', 'Y', 'V', 'Y'),
                {0}, {0}, {0}, {0}, {0}
            }
        },
        {
            DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY,
            E_NOTIMPL, DDERR_INVALIDPARAMS, FALSE, "vidmem texture DXT1", 0, FALSE,
            {
                sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('D', 'X', 'T', '1'),
                {0}, {0}, {0}, {0}, {0}
            }
        },
        {
            DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY,
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
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY,
            DD_OK, DDERR_INVALIDPARAMS, TRUE, "vidmem offscreenplain P8", 0xefefefef, FALSE,
            {
                sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_PALETTEINDEXED8, 0,
                {8}, {0}, {0}, {0}, {0}
            }
        },
        {
            DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY,
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
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    is_warp = ddraw_is_warp(ddraw);
    if (!(device = create_device(ddraw, window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        IDirectDraw_Release(ddraw);
        return;
    }

    hr = IDirectDraw_GetFourCCCodes(ddraw, &num_fourcc_codes, NULL);
    ok(SUCCEEDED(hr), "Failed to get fourcc codes %#x.\n", hr);
    fourcc_codes = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            num_fourcc_codes * sizeof(*fourcc_codes));
    if (!fourcc_codes)
        goto done;
    hr = IDirectDraw_GetFourCCCodes(ddraw, &num_fourcc_codes, fourcc_codes);
    ok(SUCCEEDED(hr), "Failed to get fourcc codes %#x.\n", hr);
    for (i = 0; i < num_fourcc_codes; i++)
    {
        if (fourcc_codes[i] == MAKEFOURCC('Y', 'U', 'Y', '2'))
            support_yuy2 = TRUE;
        else if (fourcc_codes[i] == MAKEFOURCC('U', 'Y', 'V', 'Y'))
            support_uyvy = TRUE;
    }
    HeapFree(GetProcessHeap(), 0, fourcc_codes);

    memset(&hal_caps, 0, sizeof(hal_caps));
    hal_caps.dwSize = sizeof(hal_caps);
    hr = IDirectDraw_GetCaps(ddraw, &hal_caps, NULL);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);

    if ((!support_yuy2 && !support_uyvy) || !(hal_caps.dwCaps & DDCAPS_OVERLAY))
        skip("Overlays or some YUV formats not supported, skipping YUV colorfill tests.\n");

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        DWORD expected_broken = tests[i].result;
        DWORD mask = 0xffffffffu;

        /* Some Windows drivers modify dwFillColor when it is used on P8 or FourCC formats. */
        memset(&fx, 0, sizeof(fx));
        fx.dwSize = sizeof(fx);
        U5(fx).dwFillColor = 0xdeadbeef;

        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
        surface_desc.dwWidth = 64;
        surface_desc.dwHeight = 64;
        surface_desc.ddpfPixelFormat = tests[i].format;
        surface_desc.ddsCaps.dwCaps = tests[i].caps;

        if (tests[i].caps & DDSCAPS_TEXTURE)
        {
            struct format_support_check check = {&tests[i].format, FALSE};
            hr = IDirect3DDevice_EnumTextureFormats(device, test_unsupported_formats_cb, &check);
            ok(SUCCEEDED(hr), "Failed to enumerate texture formats %#x.\n", hr);
            if (!check.supported)
                continue;
        }

        if (tests[i].format.dwFourCC == MAKEFOURCC('Y','U','Y','2') && !support_yuy2)
            continue;
        if (tests[i].format.dwFourCC == MAKEFOURCC('U','Y','V','Y') && !support_uyvy)
            continue;
        if (tests[i].caps & DDSCAPS_OVERLAY && !(hal_caps.dwCaps & DDCAPS_OVERLAY))
            continue;

        if (tests[i].caps & DDSCAPS_ZBUFFER)
        {
            surface_desc.dwFlags &= ~DDSD_PIXELFORMAT;
            surface_desc.dwFlags |= DDSD_ZBUFFERBITDEPTH;
            U2(surface_desc).dwZBufferBitDepth = get_device_z_depth(device);
            mask >>= (32 - U2(surface_desc).dwZBufferBitDepth);
            /* Some drivers seem to convert depth values incorrectly or not at
             * all. Affects at least AMD PALM, 8.17.10.1247. */
            if (tests[i].caps & DDSCAPS_VIDEOMEMORY)
            {
                DWORD expected;
                float f, g;

                expected = tests[i].result & mask;
                f = ceilf(log2f(expected + 1.0f));
                g = (f + 1.0f) / 2.0f;
                g -= (int)g;
                expected_broken = (expected / exp2f(f) - g) * 256;
                expected_broken *= 0x01010101;
            }
        }

        hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
        ok(SUCCEEDED(hr), "Failed to create surface, hr %#x, surface %s.\n", hr, tests[i].name);

        hr = IDirectDrawSurface_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
        todo_wine_if (tests[i].format.dwFourCC)
            ok(hr == tests[i].colorfill_hr, "Blt returned %#x, expected %#x, surface %s.\n",
                    hr, tests[i].colorfill_hr, tests[i].name);

        hr = IDirectDrawSurface_Blt(surface, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
        todo_wine_if (tests[i].format.dwFourCC)
            ok(hr == tests[i].colorfill_hr, "Blt returned %#x, expected %#x, surface %s.\n",
                    hr, tests[i].colorfill_hr, tests[i].name);

        if (SUCCEEDED(hr) && tests[i].check_result)
        {
            memset(&surface_desc, 0, sizeof(surface_desc));
            surface_desc.dwSize = sizeof(surface_desc);
            hr = IDirectDrawSurface_Lock(surface, NULL, &surface_desc, DDLOCK_READONLY, 0);
            ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x, surface %s.\n", hr, tests[i].name);
            color = surface_desc.lpSurface;
            ok(*color == tests[i].result, "Got clear result 0x%08x, expected 0x%08x, surface %s.\n",
                    *color, tests[i].result, tests[i].name);
            hr = IDirectDrawSurface_Unlock(surface, NULL);
            ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x, surface %s.\n", hr, tests[i].name);
        }

        hr = IDirectDrawSurface_Blt(surface, NULL, NULL, NULL, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
        ok(hr == tests[i].depthfill_hr, "Blt returned %#x, expected %#x, surface %s.\n",
                hr, tests[i].depthfill_hr, tests[i].name);
        hr = IDirectDrawSurface_Blt(surface, &rect, NULL, NULL, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
        ok(hr == tests[i].depthfill_hr, "Blt returned %#x, expected %#x, surface %s.\n",
                hr, tests[i].depthfill_hr, tests[i].name);

        if (SUCCEEDED(hr) && tests[i].check_result)
        {
            memset(&surface_desc, 0, sizeof(surface_desc));
            surface_desc.dwSize = sizeof(surface_desc);
            hr = IDirectDrawSurface_Lock(surface, NULL, &surface_desc, DDLOCK_READONLY, 0);
            ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x, surface %s.\n", hr, tests[i].name);
            color = surface_desc.lpSurface;
            todo_wine_if(tests[i].caps & DDSCAPS_VIDEOMEMORY && U2(surface_desc).dwZBufferBitDepth != 16)
                ok((*color & mask) == (tests[i].result & mask) || broken((*color & mask) == (expected_broken & mask))
                        || broken(is_warp && (*color & mask) == (~0u & mask)) /* Windows 8+ testbot. */,
                        "Got clear result 0x%08x, expected 0x%08x, surface %s.\n",
                        *color & mask, tests[i].result & mask, tests[i].name);
            hr = IDirectDrawSurface_Unlock(surface, NULL);
            ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x, surface %s.\n", hr, tests[i].name);
        }

        U5(fx).dwFillColor = 0xdeadbeef;
        fx.dwROP = BLACKNESS;
        hr = IDirectDrawSurface_Blt(surface, NULL, NULL, NULL, DDBLT_ROP | DDBLT_WAIT, &fx);
        ok(FAILED(hr) == !tests[i].rop_success, "Blt returned %#x, expected %s, surface %s.\n",
                hr, tests[i].rop_success ? "success" : "failure", tests[i].name);
        ok(U5(fx).dwFillColor == 0xdeadbeef, "dwFillColor was set to 0x%08x, surface %s\n",
                U5(fx).dwFillColor, tests[i].name);

        if (SUCCEEDED(hr) && tests[i].check_result)
        {
            memset(&surface_desc, 0, sizeof(surface_desc));
            surface_desc.dwSize = sizeof(surface_desc);
            hr = IDirectDrawSurface_Lock(surface, NULL, &surface_desc, DDLOCK_READONLY, 0);
            ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x, surface %s.\n", hr, tests[i].name);
            color = surface_desc.lpSurface;
            ok(*color == 0, "Got clear result 0x%08x, expected 0x00000000, surface %s.\n",
                    *color, tests[i].name);
            hr = IDirectDrawSurface_Unlock(surface, NULL);
            ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x, surface %s.\n", hr, tests[i].name);
        }

        fx.dwROP = WHITENESS;
        hr = IDirectDrawSurface_Blt(surface, NULL, NULL, NULL, DDBLT_ROP | DDBLT_WAIT, &fx);
        ok(FAILED(hr) == !tests[i].rop_success, "Blt returned %#x, expected %s, surface %s.\n",
                hr, tests[i].rop_success ? "success" : "failure", tests[i].name);
        ok(U5(fx).dwFillColor == 0xdeadbeef, "dwFillColor was set to 0x%08x, surface %s\n",
                U5(fx).dwFillColor, tests[i].name);

        if (SUCCEEDED(hr) && tests[i].check_result)
        {
            memset(&surface_desc, 0, sizeof(surface_desc));
            surface_desc.dwSize = sizeof(surface_desc);
            hr = IDirectDrawSurface_Lock(surface, NULL, &surface_desc, DDLOCK_READONLY, 0);
            ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x, surface %s.\n", hr, tests[i].name);
            color = surface_desc.lpSurface;
            /* WHITENESS sets the alpha channel to 0x00. Ignore this for now. */
            ok((*color & 0x00ffffff) == 0x00ffffff, "Got clear result 0x%08x, expected 0xffffffff, surface %s.\n",
                    *color, tests[i].name);
            hr = IDirectDrawSurface_Unlock(surface, NULL);
            ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x, surface %s.\n", hr, tests[i].name);
        }

        IDirectDrawSurface_Release(surface);
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
    surface_desc.ddpfPixelFormat.dwSize = sizeof(surface_desc.ddpfPixelFormat);
    surface_desc.ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(surface_desc.ddpfPixelFormat).dwRGBBitCount = 32;
    U2(surface_desc.ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(surface_desc.ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(surface_desc.ddpfPixelFormat).dwBBitMask = 0x000000ff;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface2, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    /* No DDBLTFX. */
    hr = IDirectDrawSurface_Blt(surface, NULL, NULL, &rect, DDBLT_COLORFILL | DDBLT_WAIT, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Blt(surface, NULL, NULL, &rect, DDBLT_ROP | DDBLT_WAIT, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    /* Unused source rectangle. */
    hr = IDirectDrawSurface_Blt(surface, NULL, NULL, &rect, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Blt(surface, NULL, NULL, &rect, DDBLT_ROP | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);

    /* Unused source surface. */
    hr = IDirectDrawSurface_Blt(surface, NULL, surface2, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Blt(surface, NULL, surface2, NULL, DDBLT_ROP | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Blt(surface, NULL, surface2, &rect, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Blt(surface, NULL, surface2, &rect, DDBLT_ROP | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);

    /* Inverted destination or source rectangle. */
    SetRect(&rect, 5, 7, 7, 5);
    hr = IDirectDrawSurface_Blt(surface, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDRECT, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Blt(surface, NULL, NULL, &rect, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Blt(surface, &rect, surface2, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Blt(surface, NULL, surface2, &rect, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Blt(surface, NULL, surface2, &rect, DDBLT_ROP | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDRECT, "Got unexpected hr %#x.\n", hr);

    /* Negative rectangle. */
    SetRect(&rect, -1, -1, 5, 5);
    hr = IDirectDrawSurface_Blt(surface, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDRECT, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Blt(surface, NULL, NULL, &rect, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Blt(surface, &rect, surface2, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Blt(surface, &rect, surface2, &rect, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Blt(surface, NULL, surface2, &rect, DDBLT_ROP | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDRECT, "Got unexpected hr %#x.\n", hr);

    /* Out of bounds rectangle. */
    SetRect(&rect, 0, 0, 65, 65);
    hr = IDirectDrawSurface_Blt(surface, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDRECT, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Blt(surface, NULL, surface2, &rect, DDBLT_ROP | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDRECT, "Got unexpected hr %#x.\n", hr);

    /* Combine multiple flags. */
    hr = IDirectDrawSurface_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_ROP | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Blt(surface, &rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_ROP | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(rops); i++)
    {
        fx.dwROP = rops[i].rop;
        hr = IDirectDrawSurface_Blt(surface, NULL, surface2, NULL, DDBLT_ROP | DDBLT_WAIT, &fx);
        ok(hr == rops[i].hr, "Got unexpected hr %#x for rop %s.\n", hr, rops[i].name);
    }

    IDirectDrawSurface_Release(surface2);
    IDirectDrawSurface_Release(surface);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_ZBUFFERBITDEPTH;
    surface_desc.dwWidth = 64;
    surface_desc.dwHeight = 64;
    U2(surface_desc).dwZBufferBitDepth = get_device_z_depth(device);
    surface_desc.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface2, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    /* No DDBLTFX. */
    hr = IDirectDrawSurface_Blt(surface, NULL, NULL, &rect, DDBLT_DEPTHFILL | DDBLT_WAIT, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    /* Unused source rectangle. */
    hr = IDirectDrawSurface_Blt(surface, NULL, NULL, &rect, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);

    /* Unused source surface. */
    hr = IDirectDrawSurface_Blt(surface, NULL, surface2, NULL, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Blt(surface, NULL, surface2, &rect, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    /* Inverted destination or source rectangle. */
    SetRect(&rect, 5, 7, 7, 5);
    hr = IDirectDrawSurface_Blt(surface, &rect, NULL, NULL, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDRECT, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Blt(surface, NULL, NULL, &rect, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Blt(surface, &rect, surface2, NULL, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Blt(surface, NULL, surface2, &rect, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    /* Negative rectangle. */
    SetRect(&rect, -1, -1, 5, 5);
    hr = IDirectDrawSurface_Blt(surface, &rect, NULL, NULL, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDRECT, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Blt(surface, NULL, NULL, &rect, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Blt(surface, &rect, surface2, NULL, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Blt(surface, &rect, surface2, &rect, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    /* Out of bounds rectangle. */
    SetRect(&rect, 0, 0, 65, 65);
    hr = IDirectDrawSurface_Blt(surface, &rect, NULL, NULL, DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDRECT, "Got unexpected hr %#x.\n", hr);

    /* Combine multiple flags. */
    hr = IDirectDrawSurface_Blt(surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_DEPTHFILL | DDBLT_WAIT, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    IDirectDrawSurface_Release(surface2);
    IDirectDrawSurface_Release(surface);

done:
    IDirect3DDevice_Release(device);
    refcount = IDirectDraw_Release(ddraw);
    ok(refcount == 0, "Ddraw object not properly released, refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_colorkey_precision(void)
{
    static D3DTLVERTEX quad[] =
    {
        {{  0.0f}, {480.0f}, {0.0f}, {1.0f}, {0x00000000}, {0x00000000}, {0.0f}, {1.0f}},
        {{  0.0f}, {  0.0f}, {0.0f}, {1.0f}, {0x00000000}, {0x00000000}, {0.0f}, {0.0f}},
        {{640.0f}, {480.0f}, {0.0f}, {1.0f}, {0x00000000}, {0x00000000}, {1.0f}, {1.0f}},
        {{640.0f}, {  0.0f}, {0.0f}, {1.0f}, {0x00000000}, {0x00000000}, {1.0f}, {0.0f}},
    };
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    IDirect3DDevice *device;
    IDirectDraw *ddraw;
    IDirectDrawSurface *rt;
    IDirect3DViewport *viewport;
    IDirect3DExecuteBuffer *execute_buffer;
    D3DEXECUTEBUFFERDESC exec_desc;
    UINT inst_length;
    void *ptr;
    HWND window;
    HRESULT hr;
    IDirectDrawSurface *src, *dst, *texture;
    D3DTEXTUREHANDLE handle;
    IDirect3DTexture *d3d_texture;
    IDirect3DMaterial *green;
    DDSURFACEDESC surface_desc, lock_desc;
    ULONG refcount;
    D3DCOLOR color;
    unsigned int t, c;
    DDCOLORKEY ckey;
    DDBLTFX fx;
    DWORD data[4] = {0}, color_mask;
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
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    if (!(device = create_device(ddraw, window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        DestroyWindow(window);
        IDirectDraw_Release(ddraw);
        return;
    }
    hr = IDirect3DDevice_QueryInterface(device, &IID_IDirectDrawSurface, (void **)&rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    is_nvidia = ddraw_is_nvidia(ddraw);
    /* The Windows 8 WARP driver has plenty of false negatives in X8R8G8B8
     * (color key doesn't match although the values are equal), and a false
     * positive when the color key is 0 and the texture contains the value 1.
     * I don't want to mark this broken unconditionally since this would
     * essentially disable the test on Windows. Also on random occasions
     * 254 == 255 and 255 != 255.*/
    is_warp = ddraw_is_warp(ddraw);

    green = create_diffuse_material(device, 0.0f, 1.0f, 0.0f, 0.0f);
    viewport = create_viewport(device, 0, 0, 640, 480);
    viewport_set_background(device, viewport, green);

    memset(&exec_desc, 0, sizeof(exec_desc));
    exec_desc.dwSize = sizeof(exec_desc);
    exec_desc.dwFlags = D3DDEB_BUFSIZE | D3DDEB_CAPS;
    exec_desc.dwBufferSize = 1024;
    exec_desc.dwCaps = D3DDEBCAPS_SYSTEMMEMORY;
    hr = IDirect3DDevice_CreateExecuteBuffer(device, &exec_desc, &execute_buffer, NULL);
    ok(SUCCEEDED(hr), "Failed to create execute buffer, hr %#x.\n", hr);

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
        surface_desc.ddpfPixelFormat = tests[t].fmt;
        /* Windows XP (at least with the r200 driver, other drivers untested) produces
         * garbage when doing color keyed texture->texture blits. */
        hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &src, NULL);
        ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
        hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &dst, NULL);
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
            hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &texture, NULL);
            ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

            hr = IDirectDrawSurface_QueryInterface(texture, &IID_IDirect3DTexture, (void **)&d3d_texture);
            ok(SUCCEEDED(hr), "Failed to get texture interface, hr %#x.\n", hr);
            hr = IDirect3DTexture_GetHandle(d3d_texture, device, &handle);
            ok(SUCCEEDED(hr), "Failed to get texture handle, hr %#x.\n", hr);
            IDirect3DTexture_Release(d3d_texture);

            hr = IDirect3DExecuteBuffer_Lock(execute_buffer, &exec_desc);
            ok(SUCCEEDED(hr), "Failed to lock execute buffer, hr %#x.\n", hr);

            memcpy(exec_desc.lpData, quad, sizeof(quad));

            ptr = ((BYTE *)exec_desc.lpData) + sizeof(quad);
            emit_process_vertices(&ptr, D3DPROCESSVERTICES_COPY, 0, 8);
            emit_set_rs(&ptr, D3DRENDERSTATE_ZENABLE, D3DZB_FALSE);
            emit_set_rs(&ptr, D3DRENDERSTATE_TEXTUREHANDLE, handle);
            emit_set_rs(&ptr, D3DRENDERSTATE_TEXTUREMAPBLEND, D3DTBLEND_MODULATEALPHA);
            /* D3DRENDERSTATE_COLORKEYENABLE is supposed to be on by default on version
             * 1 devices, but for some reason it randomly defaults to FALSE on the W8
             * testbot. This is either the fault of Windows 8 or the WARP driver.
             * Also D3DRENDERSTATE_COLORKEYENABLE was introduced in D3D 5 aka version 2
             * devices only, which might imply this doesn't actually do anything on
             * WARP. */
            emit_set_rs(&ptr, D3DRENDERSTATE_COLORKEYENABLE, TRUE);

            emit_tquad(&ptr, 0);
            emit_tquad(&ptr, 4);
            emit_set_rs(&ptr, D3DRENDERSTATE_TEXTUREHANDLE, 0);
            emit_end(&ptr);

            inst_length = (BYTE *)ptr - (BYTE *)exec_desc.lpData;
            inst_length -= sizeof(quad);
            hr = IDirect3DExecuteBuffer_Unlock(execute_buffer);
            ok(SUCCEEDED(hr), "Failed to unlock execute buffer, hr %#x.\n", hr);
            set_execute_data(execute_buffer, 8, sizeof(quad), inst_length);

            hr = IDirectDrawSurface_Blt(dst, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
            ok(SUCCEEDED(hr), "Failed to clear destination surface, hr %#x.\n", hr);

            hr = IDirectDrawSurface_Lock(src, NULL, &lock_desc, DDLOCK_WAIT, NULL);
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
            hr = IDirectDrawSurface_Unlock(src, 0);
            ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
            hr = IDirectDrawSurface_Blt(texture, NULL, src, NULL, DDBLT_WAIT, NULL);
            ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);

            ckey.dwColorSpaceLowValue = c << tests[t].shift;
            ckey.dwColorSpaceHighValue = c << tests[t].shift;
            hr = IDirectDrawSurface_SetColorKey(src, DDCKEY_SRCBLT, &ckey);
            ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);

            hr = IDirectDrawSurface_Blt(dst, NULL, src, NULL, DDBLT_KEYSRC | DDBLT_WAIT, NULL);
            ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);

            /* Don't make this read only, it somehow breaks the detection of the Nvidia bug below. */
            hr = IDirectDrawSurface_Lock(dst, NULL, &lock_desc, DDLOCK_WAIT, NULL);
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
            hr = IDirectDrawSurface_Unlock(dst, 0);
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
                    IDirectDrawSurface_Release(texture);
                    IDirectDrawSurface_Release(src);
                    IDirectDrawSurface_Release(dst);
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

            hr = IDirect3DViewport_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
            ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

            hr = IDirect3DDevice_BeginScene(device);
            ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
            hr = IDirect3DDevice_Execute(device, execute_buffer, viewport, D3DEXECUTE_UNCLIPPED);
            ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
            hr = IDirect3DDevice_EndScene(device);
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

            IDirectDrawSurface_Release(texture);
        }
        IDirectDrawSurface_Release(src);
        IDirectDrawSurface_Release(dst);
    }
done:

    destroy_viewport(device, viewport);
    destroy_material(green);
    IDirectDrawSurface_Release(rt);
    IDirect3DExecuteBuffer_Release(execute_buffer);
    IDirect3DDevice_Release(device);
    refcount = IDirectDraw_Release(ddraw);
    ok(refcount == 0, "Ddraw object not properly released, refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_range_colorkey(void)
{
    IDirectDraw *ddraw;
    HWND window;
    HRESULT hr;
    IDirectDrawSurface *surface;
    DDSURFACEDESC surface_desc;
    ULONG refcount;
    DDCOLORKEY ckey;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_CKSRCBLT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    surface_desc.dwWidth = 1;
    surface_desc.dwHeight = 1;
    surface_desc.ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(surface_desc.ddpfPixelFormat).dwRGBBitCount = 32;
    U2(surface_desc.ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(surface_desc.ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(surface_desc.ddpfPixelFormat).dwBBitMask = 0x000000ff;
    U5(surface_desc.ddpfPixelFormat).dwRGBAlphaBitMask = 0x00000000;

    /* Creating a surface with a range color key fails with DDERR_NOCOLORKEY. */
    surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue = 0x00000000;
    surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue = 0x00000001;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(hr == DDERR_NOCOLORKEYHW, "Got unexpected hr %#x.\n", hr);

    surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue = 0x00000001;
    surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue = 0x00000000;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(hr == DDERR_NOCOLORKEYHW, "Got unexpected hr %#x.\n", hr);

    /* Same for DDSCAPS_OFFSCREENPLAIN. */
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue = 0x00000000;
    surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue = 0x00000001;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(hr == DDERR_NOCOLORKEYHW, "Got unexpected hr %#x.\n", hr);

    surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue = 0x00000001;
    surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue = 0x00000000;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(hr == DDERR_NOCOLORKEYHW, "Got unexpected hr %#x.\n", hr);

    surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue = 0x00000000;
    surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue = 0x00000000;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    /* Setting a range color key without DDCKEY_COLORSPACE collapses the key. */
    ckey.dwColorSpaceLowValue = 0x00000000;
    ckey.dwColorSpaceHighValue = 0x00000001;
    hr = IDirectDrawSurface_SetColorKey(surface, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);

    hr = IDirectDrawSurface_GetColorKey(surface, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to get color key, hr %#x.\n", hr);
    ok(!ckey.dwColorSpaceLowValue, "Got unexpected value 0x%08x.\n", ckey.dwColorSpaceLowValue);
    ok(!ckey.dwColorSpaceHighValue, "Got unexpected value 0x%08x.\n", ckey.dwColorSpaceHighValue);

    ckey.dwColorSpaceLowValue = 0x00000001;
    ckey.dwColorSpaceHighValue = 0x00000000;
    hr = IDirectDrawSurface_SetColorKey(surface, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);

    hr = IDirectDrawSurface_GetColorKey(surface, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to get color key, hr %#x.\n", hr);
    ok(ckey.dwColorSpaceLowValue == 0x00000001, "Got unexpected value 0x%08x.\n", ckey.dwColorSpaceLowValue);
    ok(ckey.dwColorSpaceHighValue == 0x00000001, "Got unexpected value 0x%08x.\n", ckey.dwColorSpaceHighValue);

    /* DDCKEY_COLORSPACE is ignored if the key is a single value. */
    ckey.dwColorSpaceLowValue = 0x00000000;
    ckey.dwColorSpaceHighValue = 0x00000000;
    hr = IDirectDrawSurface_SetColorKey(surface, DDCKEY_SRCBLT | DDCKEY_COLORSPACE, &ckey);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);

    /* Using it with a range key results in DDERR_NOCOLORKEYHW. */
    ckey.dwColorSpaceLowValue = 0x00000001;
    ckey.dwColorSpaceHighValue = 0x00000000;
    hr = IDirectDrawSurface_SetColorKey(surface, DDCKEY_SRCBLT | DDCKEY_COLORSPACE, &ckey);
    ok(hr == DDERR_NOCOLORKEYHW, "Got unexpected hr %#x.\n", hr);
    ckey.dwColorSpaceLowValue = 0x00000000;
    ckey.dwColorSpaceHighValue = 0x00000001;
    hr = IDirectDrawSurface_SetColorKey(surface, DDCKEY_SRCBLT | DDCKEY_COLORSPACE, &ckey);
    ok(hr == DDERR_NOCOLORKEYHW, "Got unexpected hr %#x.\n", hr);
    /* Range destination keys don't work either. */
    hr = IDirectDrawSurface_SetColorKey(surface, DDCKEY_DESTBLT | DDCKEY_COLORSPACE, &ckey);
    ok(hr == DDERR_NOCOLORKEYHW, "Got unexpected hr %#x.\n", hr);

    /* Just to show it's not because of A, R, and G having equal values. */
    ckey.dwColorSpaceLowValue = 0x00000000;
    ckey.dwColorSpaceHighValue = 0x01010101;
    hr = IDirectDrawSurface_SetColorKey(surface, DDCKEY_SRCBLT | DDCKEY_COLORSPACE, &ckey);
    ok(hr == DDERR_NOCOLORKEYHW, "Got unexpected hr %#x.\n", hr);

    /* None of these operations modified the key. */
    hr = IDirectDrawSurface_GetColorKey(surface, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to get color key, hr %#x.\n", hr);
    ok(!ckey.dwColorSpaceLowValue, "Got unexpected value 0x%08x.\n", ckey.dwColorSpaceLowValue);
    ok(!ckey.dwColorSpaceHighValue, "Got unexpected value 0x%08x.\n", ckey.dwColorSpaceHighValue);

    IDirectDrawSurface_Release(surface),
    refcount = IDirectDraw_Release(ddraw);
    ok(!refcount, "Got unexpected refcount %u.\n", refcount);
    DestroyWindow(window);
}

static void test_shademode(void)
{
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    IDirect3DExecuteBuffer *execute_buffer;
    D3DEXECUTEBUFFERDESC exec_desc;
    IDirect3DMaterial *background;
    IDirect3DViewport *viewport;
    IDirect3DDevice *device;
    IDirectDrawSurface *rt;
    const D3DLVERTEX *quad;
    DWORD color0, color1;
    UINT i, inst_length;
    IDirectDraw *ddraw;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    void *ptr;
    static const D3DLVERTEX quad_strip[] =
    {
        {{-1.0f}, {-1.0f}, {0.0f}, 0, {0xffff0000}},
        {{-1.0f}, { 1.0f}, {0.0f}, 0, {0xff00ff00}},
        {{ 1.0f}, {-1.0f}, {0.0f}, 0, {0xff0000ff}},
        {{ 1.0f}, { 1.0f}, {0.0f}, 0, {0xffffffff}},
    },
    quad_list[] =
    {
        {{ 1.0f}, {-1.0f}, {0.0f}, 0, {0xff0000ff}},
        {{-1.0f}, {-1.0f}, {0.0f}, 0, {0xffff0000}},
        {{-1.0f}, { 1.0f}, {0.0f}, 0, {0xff00ff00}},
        {{ 1.0f}, { 1.0f}, {0.0f}, 0, {0xffffffff}},
    };
    static const struct
    {
        DWORD primtype;
        DWORD shademode;
        DWORD color0, color1;
    }
    tests[] =
    {
        {D3DPT_TRIANGLESTRIP, D3DSHADE_FLAT,    0x00ff0000, 0x000000ff},
        {D3DPT_TRIANGLESTRIP, D3DSHADE_PHONG,   0x000dca28, 0x000d45c7},
        {D3DPT_TRIANGLESTRIP, D3DSHADE_GOURAUD, 0x000dca28, 0x000d45c7},
        {D3DPT_TRIANGLESTRIP, D3DSHADE_PHONG,   0x000dca28, 0x000d45c7},
        {D3DPT_TRIANGLELIST,  D3DSHADE_FLAT,    0x000000ff, 0x0000ff00},
        {D3DPT_TRIANGLELIST,  D3DSHADE_GOURAUD, 0x000dca28, 0x000d45c7},
    };

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    if (!(device = create_device(ddraw, window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        IDirectDraw_Release(ddraw);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice_QueryInterface(device, &IID_IDirectDrawSurface, (void **)&rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    background = create_diffuse_material(device, 1.0f, 1.0f, 1.0f, 1.0f);
    viewport = create_viewport(device, 0, 0, 640, 480);
    viewport_set_background(device, viewport, background);

    memset(&exec_desc, 0, sizeof(exec_desc));
    exec_desc.dwSize = sizeof(exec_desc);
    exec_desc.dwFlags = D3DDEB_BUFSIZE | D3DDEB_CAPS;
    exec_desc.dwBufferSize = 1024;
    exec_desc.dwCaps = D3DDEBCAPS_SYSTEMMEMORY;

    hr = IDirect3DDevice_CreateExecuteBuffer(device, &exec_desc, &execute_buffer, NULL);
    ok(SUCCEEDED(hr), "Failed to create execute buffer, hr %#x.\n", hr);

    /* Try it first with a TRIANGLESTRIP.  Do it with different geometry because
     * the color fixups we have to do for FLAT shading will be dependent on that. */

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        hr = IDirect3DViewport_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
        ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);

        hr = IDirect3DExecuteBuffer_Lock(execute_buffer, &exec_desc);
        ok(SUCCEEDED(hr), "Failed to lock execute buffer, hr %#x.\n", hr);

        quad = tests[i].primtype == D3DPT_TRIANGLESTRIP ? quad_strip : quad_list;
        memcpy(exec_desc.lpData, quad, sizeof(quad_strip));
        ptr = ((BYTE *)exec_desc.lpData) + sizeof(quad_strip);
        emit_set_rs(&ptr, D3DRENDERSTATE_CLIPPING, FALSE);
        emit_set_rs(&ptr, D3DRENDERSTATE_ZENABLE, FALSE);
        emit_set_rs(&ptr, D3DRENDERSTATE_FOGENABLE, FALSE);
        emit_set_rs(&ptr, D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
        emit_set_rs(&ptr, D3DRENDERSTATE_SHADEMODE, tests[i].shademode);

        emit_process_vertices(&ptr, D3DPROCESSVERTICES_TRANSFORM, 0, 4);
        if (tests[i].primtype == D3DPT_TRIANGLESTRIP)
            emit_tquad(&ptr, 0);
        else
            emit_tquad_tlist(&ptr, 0);
        emit_end(&ptr);
        inst_length = (BYTE *)ptr - (BYTE *)exec_desc.lpData;
        inst_length -= sizeof(quad_strip);

        hr = IDirect3DExecuteBuffer_Unlock(execute_buffer);
        ok(SUCCEEDED(hr), "Failed to unlock execute buffer, hr %#x.\n", hr);

        hr = IDirect3DDevice_BeginScene(device);
        set_execute_data(execute_buffer, 4, sizeof(quad_strip), inst_length);
        hr = IDirect3DDevice_Execute(device, execute_buffer, viewport, D3DEXECUTE_CLIPPED);
        ok(SUCCEEDED(hr), "Failed to execute exec buffer, hr %#x.\n", hr);
        hr = IDirect3DDevice_EndScene(device);
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

    IDirect3DExecuteBuffer_Release(execute_buffer);
    destroy_viewport(device, viewport);
    destroy_material(background);
    IDirectDrawSurface_Release(rt);
    refcount = IDirect3DDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirectDraw_Release(ddraw);
    DestroyWindow(window);
}

static void test_lockrect_invalid(void)
{
    unsigned int i, r;
    IDirectDraw *ddraw;
    IDirectDrawSurface *surface;
    HWND window;
    HRESULT hr;
    DDSURFACEDESC surface_desc;
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
        DWORD caps;
        const char *name;
        HRESULT hr;
    }
    resources[] =
    {
        {DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY, "sysmem offscreenplain", DDERR_INVALIDPARAMS},
        {DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY, "vidmem offscreenplain", DDERR_INVALIDPARAMS},
        {DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY, "sysmem texture", DDERR_INVALIDPARAMS},
        {DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY, "vidmem texture", DDERR_INVALIDPARAMS},
    };

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&hal_caps, 0, sizeof(hal_caps));
    hal_caps.dwSize = sizeof(hal_caps);
    hr = IDirectDraw_GetCaps(ddraw, &hal_caps, NULL);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);
    if ((hal_caps.ddsCaps.dwCaps & needed_caps) != needed_caps)
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
        surface_desc.dwWidth = 128;
        surface_desc.dwHeight = 128;
        surface_desc.ddpfPixelFormat.dwSize = sizeof(surface_desc.ddpfPixelFormat);
        surface_desc.ddpfPixelFormat.dwFlags = DDPF_RGB;
        U1(surface_desc.ddpfPixelFormat).dwRGBBitCount = 32;
        U2(surface_desc.ddpfPixelFormat).dwRBitMask = 0xff0000;
        U3(surface_desc.ddpfPixelFormat).dwGBitMask = 0x00ff00;
        U4(surface_desc.ddpfPixelFormat).dwBBitMask = 0x0000ff;

        hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
        ok(SUCCEEDED(hr), "Failed to create surface, hr %#x, type %s.\n", hr, resources[r].name);

        hr = IDirectDrawSurface_Lock(surface, NULL, NULL, DDLOCK_WAIT, NULL);
        ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x, type %s.\n", hr, resources[r].name);

        for (i = 0; i < ARRAY_SIZE(valid); ++i)
        {
            RECT *rect = &valid[i];

            memset(&surface_desc, 0, sizeof(surface_desc));
            surface_desc.dwSize = sizeof(surface_desc);

            hr = IDirectDrawSurface_Lock(surface, rect, &surface_desc, DDLOCK_WAIT, NULL);
            ok(SUCCEEDED(hr), "Lock failed (%#x) for rect %s, type %s.\n",
                    hr, wine_dbgstr_rect(rect), resources[r].name);

            hr = IDirectDrawSurface_Unlock(surface, NULL);
            ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x, type %s.\n", hr, resources[r].name);
        }

        for (i = 0; i < ARRAY_SIZE(invalid); ++i)
        {
            RECT *rect = &invalid[i];

            memset(&surface_desc, 1, sizeof(surface_desc));
            surface_desc.dwSize = sizeof(surface_desc);

            hr = IDirectDrawSurface_Lock(surface, rect, &surface_desc, DDLOCK_WAIT, NULL);
            ok(hr == resources[r].hr, "Lock returned %#x for rect %s, type %s.\n",
                    hr, wine_dbgstr_rect(rect), resources[r].name);
            if (SUCCEEDED(hr))
            {
                hr = IDirectDrawSurface_Unlock(surface, NULL);
                ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x, type %s.\n", hr, resources[r].name);
            }
            else
                ok(!surface_desc.lpSurface, "Got unexpected lpSurface %p.\n", surface_desc.lpSurface);
        }

        hr = IDirectDrawSurface_Lock(surface, NULL, &surface_desc, DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Lock(rect = NULL) failed, hr %#x, type %s.\n",
                hr, resources[r].name);
        hr = IDirectDrawSurface_Lock(surface, NULL, &surface_desc, DDLOCK_WAIT, NULL);
        ok(hr == DDERR_SURFACEBUSY, "Double lock(rect = NULL) returned %#x, type %s.\n",
                hr, resources[r].name);
        hr = IDirectDrawSurface_Unlock(surface, NULL);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x, type %s.\n", hr, resources[r].name);

        hr = IDirectDrawSurface_Lock(surface, &valid[0], &surface_desc, DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Lock(rect = %s) failed (%#x).\n", wine_dbgstr_rect(&valid[0]), hr);
        hr = IDirectDrawSurface_Lock(surface, &valid[0], &surface_desc, DDLOCK_WAIT, NULL);
        ok(hr == DDERR_SURFACEBUSY, "Double lock(rect = %s) failed (%#x).\n",
                wine_dbgstr_rect(&valid[0]), hr);

        /* Locking a different rectangle returns DD_OK, but it seems to break the surface.
         * Afterwards unlocking the surface fails(NULL rectangle or both locked rectangles) */

        hr = IDirectDrawSurface_Unlock(surface, NULL);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x, type %s.\n", hr, resources[r].name);

        IDirectDrawSurface_Release(surface);
    }

done:
    IDirectDraw_Release(ddraw);
    DestroyWindow(window);
}

static void test_yv12_overlay(void)
{
    IDirectDrawSurface *src_surface, *dst_surface;
    RECT rect = {13, 17, 14, 18};
    unsigned int offset, y;
    unsigned char *base;
    DDSURFACEDESC desc;
    IDirectDraw *ddraw;
    HWND window;
    HRESULT hr;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    if (!(src_surface = create_overlay(ddraw, 256, 256, MAKEFOURCC('Y','V','1','2'))))
    {
        skip("Failed to create a YV12 overlay, skipping test.\n");
        goto done;
    }

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    hr = IDirectDrawSurface_Lock(src_surface, NULL, &desc, DDLOCK_WAIT, NULL);
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

    hr = IDirectDrawSurface_Unlock(src_surface, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    /* YV12 uses 2x2 blocks with 6 bytes per block (4*Y, 1*U, 1*V). Unlike
     * other block-based formats like DXT the entire Y channel is stored in
     * one big chunk of memory, followed by the chroma channels. So partial
     * locks do not really make sense. Show that they are allowed nevertheless
     * and the offset points into the luminance data. */
    hr = IDirectDrawSurface_Lock(src_surface, &rect, &desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    offset = ((const unsigned char *)desc.lpSurface - base);
    ok(offset == rect.top * U1(desc).lPitch + rect.left, "Got unexpected offset %u, expected %u.\n",
            offset, rect.top * U1(desc).lPitch + rect.left);
    hr = IDirectDrawSurface_Unlock(src_surface, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    if (!(dst_surface = create_overlay(ddraw, 256, 256, MAKEFOURCC('Y','V','1','2'))))
    {
        /* Windows XP with a Radeon X1600 GPU refuses to create a second
         * overlay surface, DDERR_NOOVERLAYHW, making the blit tests moot. */
        skip("Failed to create a second YV12 surface, skipping blit test.\n");
        IDirectDrawSurface_Release(src_surface);
        goto done;
    }

    hr = IDirectDrawSurface_Blt(dst_surface, NULL, src_surface, NULL, DDBLT_WAIT, NULL);
    /* VMware rejects YV12 blits. This behavior has not been seen on real
     * hardware yet, so mark it broken. */
    ok(SUCCEEDED(hr) || broken(hr == E_NOTIMPL), "Failed to blit, hr %#x.\n", hr);

    if (SUCCEEDED(hr))
    {
        memset(&desc, 0, sizeof(desc));
        desc.dwSize = sizeof(desc);
        hr = IDirectDrawSurface_Lock(dst_surface, NULL, &desc, DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);

        base = desc.lpSurface;
        ok(base[0] == 0x10, "Got unexpected Y data 0x%02x.\n", base[0]);
        base += desc.dwHeight * U1(desc).lPitch;
        todo_wine ok(base[0] == 0x20, "Got unexpected V data 0x%02x.\n", base[0]);
        base += desc.dwHeight / 4 * U1(desc).lPitch;
        todo_wine ok(base[0] == 0x30, "Got unexpected U data 0x%02x.\n", base[0]);

        hr = IDirectDrawSurface_Unlock(dst_surface, NULL);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
    }

    IDirectDrawSurface_Release(dst_surface);
    IDirectDrawSurface_Release(src_surface);
done:
    IDirectDraw_Release(ddraw);
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
    IDirectDrawSurface *overlay, *offscreen, *primary;
    DDSURFACEDESC surface_desc;
    IDirectDraw *ddraw;
    HWND window;
    HRESULT hr;
    HDC dc;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
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
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);

    /* On Windows 7, and probably Vista, UpdateOverlay() will return
     * DDERR_OUTOFCAPS if the dwm is active. Calling GetDC() on the primary
     * surface prevents this by disabling the dwm. */
    hr = IDirectDrawSurface_GetDC(primary, &dc);
    ok(SUCCEEDED(hr), "Failed to get DC, hr %#x.\n", hr);
    hr = IDirectDrawSurface_ReleaseDC(primary, dc);
    ok(SUCCEEDED(hr), "Failed to release DC, hr %#x.\n", hr);

    /* Try to overlay a NULL surface. */
    hr = IDirectDrawSurface_UpdateOverlay(overlay, NULL, NULL, NULL, DDOVER_SHOW, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_UpdateOverlay(overlay, NULL, NULL, NULL, DDOVER_HIDE, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    /* Try to overlay an offscreen surface. */
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    surface_desc.dwWidth = 64;
    surface_desc.dwHeight = 64;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    surface_desc.ddpfPixelFormat.dwSize = sizeof(surface_desc.ddpfPixelFormat);
    surface_desc.ddpfPixelFormat.dwFlags = DDPF_RGB;
    surface_desc.ddpfPixelFormat.dwFourCC = 0;
    U1(surface_desc.ddpfPixelFormat).dwRGBBitCount = 16;
    U2(surface_desc.ddpfPixelFormat).dwRBitMask = 0xf800;
    U3(surface_desc.ddpfPixelFormat).dwGBitMask = 0x07e0;
    U4(surface_desc.ddpfPixelFormat).dwBBitMask = 0x001f;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &offscreen, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);

    hr = IDirectDrawSurface_UpdateOverlay(overlay, NULL, offscreen, NULL, DDOVER_SHOW, NULL);
    ok(SUCCEEDED(hr) || broken(hr == DDERR_OUTOFCAPS && dwm_enabled())
            || broken(hr == E_NOTIMPL && ddraw_is_vmware(ddraw)),
            "Failed to update overlay, hr %#x.\n", hr);

    /* Try to overlay the primary with a non-overlay surface. */
    hr = IDirectDrawSurface_UpdateOverlay(offscreen, NULL, primary, NULL, DDOVER_SHOW, NULL);
    ok(hr == DDERR_NOTAOVERLAYSURFACE, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_UpdateOverlay(offscreen, NULL, primary, NULL, DDOVER_HIDE, NULL);
    ok(hr == DDERR_NOTAOVERLAYSURFACE, "Got unexpected hr %#x.\n", hr);

    IDirectDrawSurface_Release(offscreen);
    IDirectDrawSurface_Release(primary);
    IDirectDrawSurface_Release(overlay);
done:
    IDirectDraw_Release(ddraw);
    DestroyWindow(window);
}

static void test_overlay_rect(void)
{
    IDirectDrawSurface *overlay, *primary = NULL;
    DDSURFACEDESC surface_desc;
    RECT rect = {0, 0, 64, 64};
    IDirectDraw *ddraw;
    LONG pos_x, pos_y;
    HRESULT hr, hr2;
    HWND window;
    HDC dc;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
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
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &primary, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n",hr);

    /* On Windows 7, and probably Vista, UpdateOverlay() will return
     * DDERR_OUTOFCAPS if the dwm is active. Calling GetDC() on the primary
     * surface prevents this by disabling the dwm. */
    hr = IDirectDrawSurface_GetDC(primary, &dc);
    ok(SUCCEEDED(hr), "Failed to get DC, hr %#x.\n", hr);
    hr = IDirectDrawSurface_ReleaseDC(primary, dc);
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
    hr = IDirectDrawSurface_UpdateOverlay(overlay, NULL, primary, &rect, DDOVER_SHOW, NULL);
    ok(SUCCEEDED(hr), "Failed to update overlay, hr %#x.\n", hr);
    hr = IDirectDrawSurface_UpdateOverlay(overlay, NULL, primary, NULL, DDOVER_HIDE, NULL);
    ok(SUCCEEDED(hr), "Failed to update overlay, hr %#x.\n", hr);
    hr = IDirectDrawSurface_UpdateOverlay(overlay, NULL, primary, NULL, DDOVER_SHOW, NULL);
    ok(hr == DD_OK || hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    /* Show that the overlay position is the (top, left) coordinate of the
     * destination rectangle. */
    OffsetRect(&rect, 32, 16);
    hr = IDirectDrawSurface_UpdateOverlay(overlay, NULL, primary, &rect, DDOVER_SHOW, NULL);
    ok(SUCCEEDED(hr), "Failed to update overlay, hr %#x.\n", hr);
    pos_x = -1; pos_y = -1;
    hr = IDirectDrawSurface_GetOverlayPosition(overlay, &pos_x, &pos_y);
    ok(SUCCEEDED(hr), "Failed to get overlay position, hr %#x.\n", hr);
    ok(pos_x == rect.left, "Got unexpected pos_x %d, expected %d.\n", pos_x, rect.left);
    ok(pos_y == rect.top, "Got unexpected pos_y %d, expected %d.\n", pos_y, rect.top);

    /* Passing a NULL dest rect sets the position to 0/0. Visually it can be
     * seen that the overlay overlays the whole primary(==screen). */
    hr2 = IDirectDrawSurface_UpdateOverlay(overlay, NULL, primary, NULL, 0, NULL);
    ok(hr2 == DD_OK || hr2 == DDERR_INVALIDPARAMS || hr2 == DDERR_OUTOFCAPS, "Got unexpected hr %#x.\n", hr2);
    hr = IDirectDrawSurface_GetOverlayPosition(overlay, &pos_x, &pos_y);
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
    hr = IDirectDrawSurface_UpdateOverlay(overlay, NULL, primary, &rect, DDOVER_HIDE, NULL);
    ok(SUCCEEDED(hr), "Failed to update overlay, hr %#x.\n", hr);
    pos_x = -1; pos_y = -1;
    hr = IDirectDrawSurface_GetOverlayPosition(overlay, &pos_x, &pos_y);
    ok(hr == DDERR_OVERLAYNOTVISIBLE, "Got unexpected hr %#x.\n", hr);
    ok(!pos_x, "Got unexpected pos_x %d.\n", pos_x);
    ok(!pos_y, "Got unexpected pos_y %d.\n", pos_y);

    IDirectDrawSurface_Release(overlay);
done:
    if (primary)
        IDirectDrawSurface_Release(primary);
    IDirectDraw_Release(ddraw);
    DestroyWindow(window);
}

static void test_blt(void)
{
    IDirectDrawSurface *surface, *rt;
    DDSURFACEDESC surface_desc;
    IDirect3DDevice *device;
    IDirectDraw *ddraw;
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
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    if (!(device = create_device(ddraw, window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        IDirectDraw_Release(ddraw);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice_QueryInterface(device, &IID_IDirectDrawSurface, (void **)&rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
    surface_desc.dwWidth = 640;
    surface_desc.dwHeight = 480;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface_Blt(surface, NULL, surface, NULL, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);

    hr = IDirectDrawSurface_Blt(surface, NULL, rt, NULL, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(test_data); ++i)
    {
        hr = IDirectDrawSurface_Blt(surface, &test_data[i].dst_rect,
                surface, &test_data[i].src_rect, DDBLT_WAIT, NULL);
        ok(hr == test_data[i].hr, "Test %u: Got unexpected hr %#x, expected %#x.\n", i, hr, test_data[i].hr);

        hr = IDirectDrawSurface_Blt(surface, &test_data[i].dst_rect,
                rt, &test_data[i].src_rect, DDBLT_WAIT, NULL);
        ok(hr == test_data[i].hr, "Test %u: Got unexpected hr %#x, expected %#x.\n", i, hr, test_data[i].hr);

        hr = IDirectDrawSurface_Blt(surface, &test_data[i].dst_rect,
                NULL, &test_data[i].src_rect, DDBLT_WAIT, NULL);
        ok(hr == DDERR_INVALIDPARAMS, "Test %u: Got unexpected hr %#x.\n", i, hr);

        hr = IDirectDrawSurface_Blt(surface, &test_data[i].dst_rect, NULL, NULL, DDBLT_WAIT, NULL);
        ok(hr == DDERR_INVALIDPARAMS, "Test %u: Got unexpected hr %#x.\n", i, hr);
    }

    IDirectDrawSurface_Release(surface);
    IDirectDrawSurface_Release(rt);
    refcount = IDirect3DDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirectDraw_Release(ddraw);
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
    IDirectDrawSurface *src_surface, *dst_surface;
    DDSURFACEDESC surface_desc;
    IDirectDraw *ddraw;
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
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
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
    surface_desc.ddpfPixelFormat = pf;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &src_surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create source surface, hr %#x.\n", hr);
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &dst_surface, NULL);
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
        hr = IDirectDrawSurface_Blt(src_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
        ok(SUCCEEDED(hr), "Test %u: Got unexpected hr %#x.\n", i, hr);

        fx.dwFillColor = 0xccff0000;
        hr = IDirectDrawSurface_Blt(dst_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
        ok(SUCCEEDED(hr), "Test %u: Got unexpected hr %#x.\n", i, hr);

        hr = IDirectDrawSurface_Blt(dst_surface, NULL, src_surface, NULL, blt_flags[i] | DDBLT_WAIT, &fx);
        ok(SUCCEEDED(hr), "Test %u: Got unexpected hr %#x.\n", i, hr);

        color = get_surface_color(dst_surface, 32, 32);
        ok(compare_color(color, 0x0000ff00, 0), "Test %u: Got unexpected color 0x%08x.\n", i, color);
    }

    IDirectDrawSurface_Release(dst_surface);
    IDirectDrawSurface_Release(src_surface);
    refcount = IDirectDraw_Release(ddraw);
    ok(!refcount, "DirectDraw has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_getdc(void)
{
    IDirectDrawSurface *surface, *surface2, *tmp;
    DDSURFACEDESC surface_desc, map_desc;
    DDSCAPS caps = {DDSCAPS_COMPLEX};
    IDirectDraw *ddraw;
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
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(test_data); ++i)
    {
        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
        surface_desc.dwWidth = 64;
        surface_desc.dwHeight = 64;
        surface_desc.ddpfPixelFormat = test_data[i].format;
        surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

        if (FAILED(IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL)))
        {
            surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
            if (FAILED(hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL)))
            {
                skip("Failed to create surface for format %s (hr %#x), skipping tests.\n", test_data[i].name, hr);
                continue;
            }
        }

        dc = (void *)0x1234;
        hr = IDirectDrawSurface_GetDC(surface, &dc);
        if (test_data[i].getdc_supported)
            ok(SUCCEEDED(hr) || broken(hr == test_data[i].alt_result || ddraw_is_vmware(ddraw)),
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

            hr = IDirectDrawSurface_ReleaseDC(surface, dc);
            ok(hr == DD_OK, "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);
        }
        else
        {
            ok(!dc, "Got unexpected dc %p for format %s.\n", dc, test_data[i].name);
        }

        IDirectDrawSurface_Release(surface);

        if (FAILED(hr))
            continue;

        surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
        if (FAILED(hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL)))
        {
            skip("Failed to create mip-mapped texture for format %s (hr %#x), skipping tests.\n",
                    test_data[i].name, hr);
            continue;
        }

        hr = IDirectDrawSurface_GetAttachedSurface(surface, &caps, &tmp);
        ok(SUCCEEDED(hr), "Failed to get attached surface for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_GetAttachedSurface(tmp, &caps, &surface2);
        ok(SUCCEEDED(hr), "Failed to get attached surface for format %s, hr %#x.\n", test_data[i].name, hr);
        IDirectDrawSurface_Release(tmp);

        hr = IDirectDrawSurface_GetDC(surface, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_ReleaseDC(surface, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_GetDC(surface2, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_ReleaseDC(surface2, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);

        hr = IDirectDrawSurface_GetDC(surface, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", test_data[i].name, hr);
        dc2 = (void *)0x1234;
        hr = IDirectDrawSurface_GetDC(surface, &dc2);
        ok(hr == DDERR_DCALREADYCREATED, "Got unexpected hr %#x for format %s.\n", hr, test_data[i].name);
        ok(dc2 == (void *)0x1234, "Got unexpected dc %p for format %s.\n", dc, test_data[i].name);
        hr = IDirectDrawSurface_ReleaseDC(surface, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_ReleaseDC(surface, dc);
        ok(hr == DDERR_NODC, "Got unexpected hr %#x for format %s.\n", hr, test_data[i].name);

        map_desc.dwSize = sizeof(map_desc);
        hr = IDirectDrawSurface_Lock(surface, NULL, &map_desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Failed to map surface for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_Lock(surface, NULL, &map_desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
        ok(hr == DDERR_SURFACEBUSY, "Got unexpected hr %#x for format %s.\n", hr, test_data[i].name);
        hr = IDirectDrawSurface_Unlock(surface, NULL);
        ok(SUCCEEDED(hr), "Failed to unmap surface for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_Unlock(surface, NULL);
        ok(hr == DDERR_NOTLOCKED, "Got unexpected hr %#x for format %s.\n", hr, test_data[i].name);

        hr = IDirectDrawSurface_GetDC(surface, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_Lock(surface, NULL, &map_desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
        ok(hr == DDERR_SURFACEBUSY, "Got unexpected hr %#x for format %s.\n", hr, test_data[i].name);
        hr = IDirectDrawSurface_ReleaseDC(surface, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);

        hr = IDirectDrawSurface_Lock(surface, NULL, &map_desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Failed to map surface for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_GetDC(surface, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_ReleaseDC(surface, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_Unlock(surface, NULL);
        ok(SUCCEEDED(hr), "Failed to unmap surface for format %s, hr %#x.\n", test_data[i].name, hr);

        hr = IDirectDrawSurface_GetDC(surface, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_GetDC(surface2, &dc2);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_ReleaseDC(surface2, dc2);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_ReleaseDC(surface, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);

        hr = IDirectDrawSurface_GetDC(surface2, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_GetDC(surface, &dc2);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_ReleaseDC(surface, dc2);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_ReleaseDC(surface2, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);

        hr = IDirectDrawSurface_Lock(surface, NULL, &map_desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Failed to map surface for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_Lock(surface2, NULL, &map_desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Failed to map surface for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_Unlock(surface2, NULL);
        ok(SUCCEEDED(hr), "Failed to unmap surface for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_Unlock(surface, NULL);
        ok(SUCCEEDED(hr), "Failed to unmap surface for format %s, hr %#x.\n", test_data[i].name, hr);

        hr = IDirectDrawSurface_Lock(surface, NULL, &map_desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Failed to map surface for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_GetDC(surface, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_ReleaseDC(surface, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_Unlock(surface, NULL);
        ok(SUCCEEDED(hr), "Failed to unmap surface for format %s, hr %#x.\n", test_data[i].name, hr);

        hr = IDirectDrawSurface_Lock(surface2, NULL, &map_desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Failed to map surface for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_GetDC(surface, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_ReleaseDC(surface, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_Unlock(surface2, NULL);
        ok(SUCCEEDED(hr), "Failed to unmap surface for format %s, hr %#x.\n", test_data[i].name, hr);

        hr = IDirectDrawSurface_GetDC(surface, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_Lock(surface2, NULL, &map_desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Failed to map surface for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_Unlock(surface2, NULL);
        ok(SUCCEEDED(hr), "Failed to unmap surface for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_ReleaseDC(surface, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);

        hr = IDirectDrawSurface_GetDC(surface2, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_Lock(surface, NULL, &map_desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Failed to map surface for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_Unlock(surface, NULL);
        ok(SUCCEEDED(hr), "Failed to unmap surface for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_ReleaseDC(surface2, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);

        hr = IDirectDrawSurface_Unlock(surface, NULL);
        ok(hr == DDERR_NOTLOCKED, "Got unexpected hr %#x for format %s.\n", hr, test_data[i].name);
        hr = IDirectDrawSurface_GetDC(surface2, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_Unlock(surface, NULL);
        ok(hr == DDERR_NOTLOCKED, "Got unexpected hr %#x for format %s.\n", hr, test_data[i].name);
        hr = IDirectDrawSurface_ReleaseDC(surface2, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_Unlock(surface, NULL);
        ok(hr == DDERR_NOTLOCKED, "Got unexpected hr %#x for format %s.\n", hr, test_data[i].name);

        hr = IDirectDrawSurface_Unlock(surface2, NULL);
        ok(hr == DDERR_NOTLOCKED, "Got unexpected hr %#x for format %s.\n", hr, test_data[i].name);
        hr = IDirectDrawSurface_GetDC(surface, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_Unlock(surface2, NULL);
        ok(hr == DDERR_NOTLOCKED, "Got unexpected hr %#x for format %s.\n", hr, test_data[i].name);
        hr = IDirectDrawSurface_ReleaseDC(surface, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", test_data[i].name, hr);
        hr = IDirectDrawSurface_Unlock(surface2, NULL);
        ok(hr == DDERR_NOTLOCKED, "Got unexpected hr %#x for format %s.\n", hr, test_data[i].name);

        IDirectDrawSurface_Release(surface2);
        IDirectDrawSurface_Release(surface);
    }

    IDirectDraw_Release(ddraw);
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
    IDirect3DDevice *device;
    IDirectDrawSurface *rt;
    IDirectDraw *ddraw;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    D3DCOLOR color;
    IDirect3DViewport *viewport;
    IDirect3DExecuteBuffer *execute_buffer;
    IDirect3DMaterial *background;
    D3DEXECUTEBUFFERDESC exec_desc;
    UINT inst_length;
    void *ptr;
    D3DMATRIXHANDLE world_handle, view_handle, proj_handle;
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
    };
    static const D3DLVERTEX quad[] =
    {
        {{-0.75f},{-0.5f }, {0.0f}, 0, {0xffff0000}},
        {{-0.75f},{ 0.25f}, {0.0f}, 0, {0xffff0000}},
        {{ 0.5f}, {-0.5f }, {0.0f}, 0, {0xffff0000}},
        {{ 0.5f}, { 0.25f}, {0.0f}, 0, {0xffff0000}},
    };
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};


    for (i = 0; i < ARRAY_SIZE(out); ++i)
    {
        out[i].unused3 = 0xdeadbeef;
        out[i].unused4 = 0xcafecafe;
    }

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    if (!(device = create_device(ddraw, window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        IDirectDraw_Release(ddraw);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice_QueryInterface(device, &IID_IDirectDrawSurface, (void **)&rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    viewport = create_viewport(device, 0, 0, 256, 256);
    hr = IDirect3DViewport_SetViewport(viewport, &vp_data);
    ok(SUCCEEDED(hr), "Failed to set viewport, hr %#x.\n", hr);

    memset(&transformdata, 0, sizeof(transformdata));
    transformdata.dwSize = sizeof(transformdata);
    transformdata.lpIn = position_tests;
    transformdata.dwInSize = sizeof(position_tests[0]);
    transformdata.lpOut = out;
    transformdata.dwOutSize = sizeof(out[0]);
    transformdata.lpHOut = NULL;

    hr = IDirect3DViewport_TransformVertices(viewport, ARRAY_SIZE(position_tests),
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
    hr = IDirect3DViewport_SetViewport(viewport, &vp_data);
    ok(SUCCEEDED(hr), "Failed to set viewport, hr %#x.\n", hr);
    offscreen = 0xdeadbeef;
    hr = IDirect3DViewport_TransformVertices(viewport, ARRAY_SIZE(position_tests),
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
    hr = IDirect3DViewport_SetViewport(viewport, &vp_data);
    ok(SUCCEEDED(hr), "Failed to set viewport, hr %#x.\n", hr);
    offscreen = 0xdeadbeef;
    hr = IDirect3DViewport_TransformVertices(viewport, ARRAY_SIZE(position_tests),
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
    hr = IDirect3DViewport_TransformVertices(viewport, ARRAY_SIZE(position_tests),
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
    hr = IDirect3DViewport_TransformVertices(viewport, ARRAY_SIZE(cliptest),
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
    vp_data.dwHeight = 1000;
    hr = IDirect3DViewport_SetViewport(viewport, &vp_data);
    ok(SUCCEEDED(hr), "Failed to set viewport, hr %#x.\n", hr);
    offscreen = 0xdeadbeef;
    hr = IDirect3DViewport_TransformVertices(viewport, ARRAY_SIZE(cliptest),
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
    hr = IDirect3DViewport_SetViewport(viewport, &vp_data);
    ok(SUCCEEDED(hr), "Failed to set viewport, hr %#x.\n", hr);
    hr = IDirect3DViewport_TransformVertices(viewport, ARRAY_SIZE(cliptest),
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
    hr = IDirect3DViewport_SetViewport(viewport, &vp_data);
    ok(SUCCEEDED(hr), "Failed to set viewport, hr %#x.\n", hr);
    transformdata.lpIn = cliptest;
    offscreen = 0xdeadbeef;
    hr = IDirect3DViewport_TransformVertices(viewport, 1,
            &transformdata, D3DTRANSFORM_UNCLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(!offscreen, "Offscreen is %x.\n", offscreen);

    offscreen = 0xdeadbeef;
    hr = IDirect3DViewport_TransformVertices(viewport, 1,
            &transformdata, D3DTRANSFORM_CLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(offscreen == (D3DCLIP_RIGHT | D3DCLIP_TOP), "Offscreen is %x.\n", offscreen);
    offscreen = 0xdeadbeef;
    hr = IDirect3DViewport_TransformVertices(viewport, 2,
            &transformdata, D3DTRANSFORM_CLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(offscreen == (D3DCLIP_RIGHT | D3DCLIP_TOP), "Offscreen is %x.\n", offscreen);
    hr = IDirect3DViewport_TransformVertices(viewport, 3,
            &transformdata, D3DTRANSFORM_CLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(!offscreen, "Offscreen is %x.\n", offscreen);

    transformdata.lpIn = cliptest + 1;
    hr = IDirect3DViewport_TransformVertices(viewport, 1,
            &transformdata, D3DTRANSFORM_CLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(offscreen == (D3DCLIP_BACK | D3DCLIP_RIGHT | D3DCLIP_TOP), "Offscreen is %x.\n", offscreen);

    transformdata.lpIn = cliptest + 2;
    hr = IDirect3DViewport_TransformVertices(viewport, 1,
            &transformdata, D3DTRANSFORM_CLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(offscreen == (D3DCLIP_BOTTOM | D3DCLIP_LEFT), "Offscreen is %x.\n", offscreen);
    offscreen = 0xdeadbeef;
    hr = IDirect3DViewport_TransformVertices(viewport, 2,
            &transformdata, D3DTRANSFORM_CLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(offscreen == (D3DCLIP_BOTTOM | D3DCLIP_LEFT), "Offscreen is %x.\n", offscreen);

    transformdata.lpIn = cliptest + 3;
    hr = IDirect3DViewport_TransformVertices(viewport, 1,
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
    hr = IDirect3DViewport_SetViewport(viewport, &vp_data);
    ok(SUCCEEDED(hr), "Failed to set viewport, hr %#x.\n", hr);
    offscreen = 0xdeadbeef;
    hr = IDirect3DViewport_TransformVertices(viewport, 1,
            &transformdata, D3DTRANSFORM_CLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(!offscreen, "Offscreen is %x.\n", offscreen);

    vp_data.dwWidth = 256;
    vp_data.dwHeight = 256;
    hr = IDirect3DViewport_SetViewport(viewport, &vp_data);
    ok(SUCCEEDED(hr), "Failed to set viewport, hr %#x.\n", hr);
    hr = IDirect3DViewport_TransformVertices(viewport, 1,
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
    hr = IDirect3DViewport_SetViewport(viewport, &vp_data);
    ok(SUCCEEDED(hr), "Failed to set viewport, hr %#x.\n", hr);

    hr = IDirect3DDevice_CreateMatrix(device, &world_handle);
    ok(hr == D3D_OK, "Creating a matrix object failed, hr %#x.\n", hr);
    hr = IDirect3DDevice_SetMatrix(device, world_handle, &mat_translate1);
    ok(hr == D3D_OK, "Setting a matrix object failed, hr %#x.\n", hr);

    hr = IDirect3DDevice_CreateMatrix(device, &view_handle);
    ok(hr == D3D_OK, "Creating a matrix object failed, hr %#x.\n", hr);
    hr = IDirect3DDevice_SetMatrix(device, view_handle, &mat_scale);
    ok(hr == D3D_OK, "Setting a matrix object failed, hr %#x.\n", hr);

    hr = IDirect3DDevice_CreateMatrix(device, &proj_handle);
    ok(hr == D3D_OK, "Creating a matrix object failed, hr %#x.\n", hr);
    hr = IDirect3DDevice_SetMatrix(device, proj_handle, &mat_translate2);
    ok(hr == D3D_OK, "Setting a matrix object failed, hr %#x.\n", hr);

    memset(&exec_desc, 0, sizeof(exec_desc));
    exec_desc.dwSize = sizeof(exec_desc);
    exec_desc.dwFlags = D3DDEB_BUFSIZE | D3DDEB_CAPS;
    exec_desc.dwBufferSize = 1024;
    exec_desc.dwCaps = D3DDEBCAPS_SYSTEMMEMORY;
    hr = IDirect3DDevice_CreateExecuteBuffer(device, &exec_desc, &execute_buffer, NULL);
    ok(SUCCEEDED(hr), "Failed to create execute buffer, hr %#x.\n", hr);

    hr = IDirect3DExecuteBuffer_Lock(execute_buffer, &exec_desc);
    ok(SUCCEEDED(hr), "Failed to lock execute buffer, hr %#x.\n", hr);
    ptr = (BYTE *)exec_desc.lpData;
    emit_set_ts(&ptr, D3DTRANSFORMSTATE_WORLD, world_handle);
    emit_set_ts(&ptr, D3DTRANSFORMSTATE_VIEW, view_handle);
    emit_set_ts(&ptr, D3DTRANSFORMSTATE_PROJECTION, proj_handle);
    emit_end(&ptr);
    inst_length = (BYTE *)ptr - (BYTE *)exec_desc.lpData;
    hr = IDirect3DExecuteBuffer_Unlock(execute_buffer);
    ok(SUCCEEDED(hr), "Failed to unlock execute buffer, hr %#x.\n", hr);

    set_execute_data(execute_buffer, 0, 0, inst_length);
    hr = IDirect3DDevice_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice_Execute(device, execute_buffer, viewport, D3DEXECUTE_CLIPPED);
    ok(SUCCEEDED(hr), "Failed to execute exec buffer, hr %#x.\n", hr);
    hr = IDirect3DDevice_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    transformdata.lpIn = position_tests;
    transformdata.dwInSize = sizeof(position_tests[0]);
    hr = IDirect3DViewport_TransformVertices(viewport, ARRAY_SIZE(position_tests),
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
    hr = IDirect3DViewport_TransformVertices(viewport, ARRAY_SIZE(position_tests),
            &transformdata, 0, &offscreen);
    ok(hr == DDERR_INVALIDPARAMS, "TransformVertices returned %#x.\n", hr);
    ok(offscreen == 0xdeadbeef, "Offscreen is %x.\n", offscreen);

    /* NULL transform data. */
    hr = IDirect3DViewport_TransformVertices(viewport, 1,
            NULL, D3DTRANSFORM_UNCLIPPED, &offscreen);
    ok(hr == DDERR_INVALIDPARAMS, "TransformVertices returned %#x.\n", hr);
    ok(offscreen == 0xdeadbeef, "Offscreen is %x.\n", offscreen);
    hr = IDirect3DViewport_TransformVertices(viewport, 0,
            NULL, D3DTRANSFORM_UNCLIPPED, &offscreen);
    ok(hr == DDERR_INVALIDPARAMS, "TransformVertices returned %#x.\n", hr);
    ok(offscreen == 0xdeadbeef, "Offscreen is %x.\n", offscreen);

    /* NULL transform data and NULL dwOffscreen.
     *
     * Valid transform data + NULL dwOffscreen -> crash. */
    hr = IDirect3DViewport_TransformVertices(viewport, 1,
            NULL, D3DTRANSFORM_UNCLIPPED, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "TransformVertices returned %#x.\n", hr);

    /* No vertices. */
    hr = IDirect3DViewport_TransformVertices(viewport, 0,
            &transformdata, D3DTRANSFORM_UNCLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(!offscreen, "Offscreen is %x.\n", offscreen);
    hr = IDirect3DViewport_TransformVertices(viewport, 0,
            &transformdata, D3DTRANSFORM_CLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(offscreen == ~0U, "Offscreen is %x.\n", offscreen);

    /* Invalid sizes. */
    offscreen = 0xdeadbeef;
    transformdata.dwSize = sizeof(transformdata) - 1;
    hr = IDirect3DViewport_TransformVertices(viewport, 1,
            &transformdata, D3DTRANSFORM_UNCLIPPED, &offscreen);
    ok(hr == DDERR_INVALIDPARAMS, "TransformVertices returned %#x.\n", hr);
    ok(offscreen == 0xdeadbeef, "Offscreen is %x.\n", offscreen);
    transformdata.dwSize = sizeof(transformdata) + 1;
    hr = IDirect3DViewport_TransformVertices(viewport, 1,
            &transformdata, D3DTRANSFORM_UNCLIPPED, &offscreen);
    ok(hr == DDERR_INVALIDPARAMS, "TransformVertices returned %#x.\n", hr);
    ok(offscreen == 0xdeadbeef, "Offscreen is %x.\n", offscreen);

    /* NULL lpIn or lpOut -> crash, except when transforming 0 vertices. */
    transformdata.dwSize = sizeof(transformdata);
    transformdata.lpIn = NULL;
    transformdata.lpOut = NULL;
    offscreen = 0xdeadbeef;
    hr = IDirect3DViewport_TransformVertices(viewport, 0,
            &transformdata, D3DTRANSFORM_CLIPPED, &offscreen);
    ok(SUCCEEDED(hr), "Failed to transform vertices, hr %#x.\n", hr);
    ok(offscreen == ~0U, "Offscreen is %x.\n", offscreen);

    /* Test how vertices are transformed by execute buffers. */
    vp_data.dwX = 20;
    vp_data.dwY = 20;
    vp_data.dwWidth = 200;
    vp_data.dwHeight = 400;
    vp_data.dvScaleX = 20.0f;
    vp_data.dvScaleY = 50.0f;
    vp_data.dvMinZ = 0.0f;
    vp_data.dvMaxZ = 1.0f;
    hr = IDirect3DViewport_SetViewport(viewport, &vp_data);
    ok(SUCCEEDED(hr), "Failed to set viewport, hr %#x.\n", hr);

    background = create_diffuse_material(device, 0.0f, 0.0f, 1.0f, 0.0f);
    viewport_set_background(device, viewport, background);
    hr = IDirect3DViewport_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);

    hr = IDirect3DExecuteBuffer_Lock(execute_buffer, &exec_desc);
    ok(SUCCEEDED(hr), "Failed to lock execute buffer, hr %#x.\n", hr);
    memcpy(exec_desc.lpData, quad, sizeof(quad));
    ptr = ((BYTE *)exec_desc.lpData) + sizeof(quad);
    emit_process_vertices(&ptr, D3DPROCESSVERTICES_TRANSFORM, 0, 4);
    emit_tquad(&ptr, 0);
    emit_end(&ptr);
    inst_length = (BYTE *)ptr - (BYTE *)exec_desc.lpData;
    hr = IDirect3DExecuteBuffer_Unlock(execute_buffer);
    ok(SUCCEEDED(hr), "Failed to unlock execute buffer, hr %#x.\n", hr);

    set_execute_data(execute_buffer, 4, sizeof(quad), inst_length);
    hr = IDirect3DDevice_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice_Execute(device, execute_buffer, viewport, D3DEXECUTE_CLIPPED);
    ok(SUCCEEDED(hr), "Failed to execute exec buffer, hr %#x.\n", hr);
    hr = IDirect3DDevice_EndScene(device);
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

    IDirect3DDevice_DeleteMatrix(device, world_handle);
    IDirect3DDevice_DeleteMatrix(device, view_handle);
    IDirect3DDevice_DeleteMatrix(device, proj_handle);
    IDirect3DExecuteBuffer_Release(execute_buffer);

    IDirectDrawSurface_Release(rt);
    destroy_viewport(device, viewport);
    IDirect3DMaterial_Release(background);
    refcount = IDirect3DDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirectDraw_Release(ddraw);
    DestroyWindow(window);
}

static void test_display_mode_surface_pixel_format(void)
{
    unsigned int width, height, bpp;
    IDirectDrawSurface *surface;
    DDSURFACEDESC surface_desc;
    IDirectDraw *ddraw;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    if (!(ddraw = create_ddraw()))
    {
        skip("Failed to create ddraw.\n");
        return;
    }

    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDraw_GetDisplayMode(ddraw, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get display mode, hr %#x.\n", hr);
    width = surface_desc.dwWidth;
    height = surface_desc.dwHeight;

    window = CreateWindowA("static", "ddraw_test", WS_OVERLAPPEDWINDOW,
            0, 0, width, height, NULL, NULL, NULL, NULL);
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    bpp = 0;
    if (SUCCEEDED(IDirectDraw_SetDisplayMode(ddraw, width, height, 16)))
        bpp = 16;
    if (SUCCEEDED(IDirectDraw_SetDisplayMode(ddraw, width, height, 24)))
        bpp = 24;
    if (SUCCEEDED(IDirectDraw_SetDisplayMode(ddraw, width, height, 32)))
        bpp = 32;
    ok(bpp, "Set display mode failed.\n");

    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDraw_GetDisplayMode(ddraw, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get display mode, hr %#x.\n", hr);
    ok(surface_desc.dwWidth == width, "Got width %u, expected %u.\n", surface_desc.dwWidth, width);
    ok(surface_desc.dwHeight == height, "Got height %u, expected %u.\n", surface_desc.dwHeight, height);
    ok(U1(surface_desc.ddpfPixelFormat).dwRGBBitCount == bpp, "Got bpp %u, expected %u.\n",
            U1(surface_desc.ddpfPixelFormat).dwRGBBitCount, bpp);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    surface_desc.dwBackBufferCount = 1;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_COMPLEX | DDSCAPS_FLIP | DDSCAPS_PRIMARYSURFACE;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(hr == D3D_OK, "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(surface_desc.dwWidth == width, "Got width %u, expected %u.\n", surface_desc.dwWidth, width);
    ok(surface_desc.dwHeight == height, "Got height %u, expected %u.\n", surface_desc.dwHeight, height);
    ok(surface_desc.ddpfPixelFormat.dwFlags == DDPF_RGB, "Got unexpected pixel format flags %#x.\n",
            surface_desc.ddpfPixelFormat.dwFlags);
    ok(U1(surface_desc.ddpfPixelFormat).dwRGBBitCount == bpp, "Got bpp %u, expected %u.\n",
            U1(surface_desc.ddpfPixelFormat).dwRGBBitCount, bpp);
    IDirectDrawSurface_Release(surface);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    surface_desc.dwWidth = width;
    surface_desc.dwHeight = height;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL);
    ok(hr == D3D_OK, "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface_GetSurfaceDesc(surface, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(surface_desc.ddpfPixelFormat.dwFlags == DDPF_RGB, "Got unexpected pixel format flags %#x.\n",
            surface_desc.ddpfPixelFormat.dwFlags);
    ok(U1(surface_desc.ddpfPixelFormat).dwRGBBitCount == bpp, "Got bpp %u, expected %u.\n",
            U1(surface_desc.ddpfPixelFormat).dwRGBBitCount, bpp);
    IDirectDrawSurface_Release(surface);

    refcount = IDirectDraw_Release(ddraw);
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
    IDirectDrawSurface7 *surface7;
    IDirectDrawSurface *surface;
    DDSURFACEDESC surface_desc;
    HRESULT expected_hr, hr;
    IDirectDraw *ddraw;
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
    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(surface_caps); ++i)
    {
        memset(&surface_desc, 0, sizeof(surface_desc));
        surface_desc.dwSize = sizeof(surface_desc);
        surface_desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
        surface_desc.ddsCaps.dwCaps = surface_caps[i].caps;
        surface_desc.dwHeight = 128;
        surface_desc.dwWidth = 128;
        if (FAILED(IDirectDraw_CreateSurface(ddraw, &surface_desc, &surface, NULL)))
        {
            skip("Failed to create surface, type %s.\n", surface_caps[i].name);
            continue;
        }
        hr = IDirectDrawSurface_QueryInterface(surface, &IID_IDirectDrawSurface7, (void **)&surface7);
        ok(hr == DD_OK, "Failed to query IDirectDrawSurface7, hr %#x, type %s.\n", hr, surface_caps[i].name);

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
            expected_hr = desc.dwSize == sizeof(DDSURFACEDESC2) ? DD_OK : DDERR_INVALIDPARAMS;
            hr = IDirectDrawSurface7_GetSurfaceDesc(surface7, &desc.desc2);
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
            hr = IDirectDrawSurface7_Lock(surface7, NULL, &desc.desc2, 0, 0);
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
                IDirectDrawSurface7_Unlock(surface7, NULL);
            }
        }

        IDirectDrawSurface7_Release(surface7);
        IDirectDrawSurface_Release(surface);
    }

    /* GetDisplayMode() */
    for (j = 0; j < ARRAY_SIZE(desc_sizes); ++j)
    {
        memset(&desc, 0xcc, sizeof(desc));
        desc.dwSize = desc_sizes[j];
        expected_hr = (desc.dwSize == sizeof(DDSURFACEDESC) || desc.dwSize == sizeof(DDSURFACEDESC2))
                ? DD_OK : DDERR_INVALIDPARAMS;
        hr = IDirectDraw_GetDisplayMode(ddraw, &desc.desc1);
        ok(hr == expected_hr, "Got hr %#x, expected %#x, size %u.\n", hr, expected_hr, desc_sizes[j]);
        if (SUCCEEDED(hr))
        {
            ok(desc.dwSize == sizeof(DDSURFACEDESC), "Wrong size %u for %u.\n", desc.dwSize, desc_sizes[j]);
            ok(desc.blob[desc_sizes[j]] == 0xcc, "Overflow for size %u.\n", desc_sizes[j]);
            ok(desc.blob[desc_sizes[j] - 1] != 0xcc, "Struct not cleared for size %u.\n", desc_sizes[j]);
        }
    }

    refcount = IDirectDraw7_Release(ddraw);
    ok(!refcount, "DirectDraw has %u references left.\n", refcount);
}

static void test_texture_load(void)
{
    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    static D3DTLVERTEX tquad[] =
    {
        {{  0.0f}, {480.0f}, {0.0f}, {1.0f}, {0xffffffff}, {0x00000000}, {0.0f}, {0.0f}},
        {{  0.0f}, {  0.0f}, {0.0f}, {1.0f}, {0xffffffff}, {0x00000000}, {0.0f}, {1.0f}},
        {{640.0f}, {480.0f}, {0.0f}, {1.0f}, {0xffffffff}, {0x00000000}, {1.0f}, {0.0f}},
        {{640.0f}, {  0.0f}, {0.0f}, {1.0f}, {0xffffffff}, {0x00000000}, {1.0f}, {1.0f}},
    };
    D3DTEXTUREHANDLE dst_texture_handle, src_texture_handle;
    IDirectDrawSurface *dst_surface, *src_surface;
    IDirect3DExecuteBuffer *execute_buffer;
    D3DEXECUTEBUFFERDESC exec_desc;
    IDirect3DMaterial *background;
    IDirect3DViewport *viewport;
    DDSURFACEDESC surface_desc;
    IDirect3DTexture *texture;
    IDirect3DDevice *device;
    IDirectDrawSurface *rt;
    IDirectDraw *ddraw;
    UINT inst_length;
    D3DCOLOR color;
    ULONG refcount;
    HWND window;
    DDBLTFX fx;
    HRESULT hr;
    void *ptr;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    if (!(device = create_device(ddraw, window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        IDirectDraw_Release(ddraw);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice_QueryInterface(device, &IID_IDirectDrawSurface, (void **)&rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    background = create_diffuse_material(device, 1.0f, 1.0f, 1.0f, 1.0f);
    viewport = create_viewport(device, 0, 0, 640, 480);
    viewport_set_background(device, viewport, background);

    memset(&exec_desc, 0, sizeof(exec_desc));
    exec_desc.dwSize = sizeof(exec_desc);
    exec_desc.dwFlags = D3DDEB_BUFSIZE | D3DDEB_CAPS;
    exec_desc.dwBufferSize = 1024;
    exec_desc.dwCaps = D3DDEBCAPS_SYSTEMMEMORY;
    hr = IDirect3DDevice_CreateExecuteBuffer(device, &exec_desc, &execute_buffer, NULL);
    ok(SUCCEEDED(hr), "Failed to create execute buffer, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    surface_desc.dwWidth = 256;
    surface_desc.dwHeight = 256;
    surface_desc.ddpfPixelFormat.dwSize = sizeof(surface_desc.ddpfPixelFormat);
    surface_desc.ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(surface_desc.ddpfPixelFormat).dwRGBBitCount = 32;
    U2(surface_desc.ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(surface_desc.ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(surface_desc.ddpfPixelFormat).dwBBitMask = 0x000000ff;

    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &src_surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface_QueryInterface(src_surface, &IID_IDirect3DTexture, (void **)&texture);
    ok(SUCCEEDED(hr), "Failed to get texture interface, hr %#x.\n", hr);
    hr = IDirect3DTexture_GetHandle(texture, device, &src_texture_handle);
    ok(SUCCEEDED(hr), "Failed to get texture handle, hr %#x.\n", hr);
    IDirect3DTexture_Release(texture);

    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &dst_surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface_QueryInterface(dst_surface, &IID_IDirect3DTexture, (void **)&texture);
    ok(SUCCEEDED(hr), "Failed to get texture interface, hr %#x.\n", hr);
    hr = IDirect3DTexture_GetHandle(texture, device, &dst_texture_handle);
    ok(SUCCEEDED(hr), "Failed to get texture handle, hr %#x.\n", hr);
    IDirect3DTexture_Release(texture);

    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    U5(fx).dwFillColor = 0x0000ffff;
    hr = IDirectDrawSurface_Blt(src_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to fill surface, hr %#x.\n", hr);

    hr = IDirect3DExecuteBuffer_Lock(execute_buffer, &exec_desc);
    ok(SUCCEEDED(hr), "Failed to lock execute buffer, hr %#x.\n", hr);
    memcpy(exec_desc.lpData, tquad, sizeof(tquad));
    ptr = (BYTE *)exec_desc.lpData + sizeof(tquad);
    emit_process_vertices(&ptr, D3DPROCESSVERTICES_COPY, 0, 4);
    emit_texture_load(&ptr, dst_texture_handle, src_texture_handle);
    emit_set_rs(&ptr, D3DRENDERSTATE_TEXTUREHANDLE, dst_texture_handle);
    emit_tquad(&ptr, 0);
    emit_end(&ptr);
    inst_length = (BYTE *)ptr - (BYTE *)exec_desc.lpData - sizeof(tquad);
    hr = IDirect3DExecuteBuffer_Unlock(execute_buffer);
    ok(SUCCEEDED(hr), "Failed to unlock execute buffer, hr %#x.\n", hr);

    hr = IDirect3DViewport_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x00ffffff, 1), "Got unexpected color 0x%08x.\n", color);
    set_execute_data(execute_buffer, 4, sizeof(tquad), inst_length);
    hr = IDirect3DDevice_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice_Execute(device, execute_buffer, viewport, D3DEXECUTE_CLIPPED);
    ok(SUCCEEDED(hr), "Failed to execute exec buffer, hr %#x.\n", hr);
    hr = IDirect3DDevice_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);
    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x0000ffff, 1), "Got unexpected color 0x%08x.\n", color);

    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    U5(fx).dwFillColor = 0x000000ff;
    hr = IDirectDrawSurface_Blt(src_surface, NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    ok(SUCCEEDED(hr), "Failed to fill surface, hr %#x.\n", hr);

    hr = IDirect3DViewport_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear viewport, hr %#x.\n", hr);
    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x00ffffff, 1), "Got unexpected color 0x%08x.\n", color);
    set_execute_data(execute_buffer, 4, sizeof(tquad), inst_length);
    hr = IDirect3DDevice_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice_Execute(device, execute_buffer, viewport, D3DEXECUTE_CLIPPED);
    ok(SUCCEEDED(hr), "Failed to execute exec buffer, hr %#x.\n", hr);
    hr = IDirect3DDevice_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);
    color = get_surface_color(rt, 320, 240);
    ok(compare_color(color, 0x000000ff, 1), "Got unexpected color 0x%08x.\n", color);

    IDirectDrawSurface_Release(dst_surface);
    IDirectDrawSurface_Release(src_surface);
    IDirectDrawSurface_Release(rt);
    IDirect3DExecuteBuffer_Release(execute_buffer);
    IDirect3DMaterial_Release(background);
    destroy_viewport(device, viewport);
    IDirect3DDevice_Release(device);
    refcount = IDirectDraw_Release(ddraw);
    ok(!refcount, "DirectDraw has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_ck_operation(void)
{
    IDirectDrawSurface *src, *dst;
    IDirectDrawSurface7 *src7, *dst7;
    DDSURFACEDESC surface_desc;
    IDirectDraw *ddraw;
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
    hr = IDirectDraw_SetCooperativeLevel(ddraw, window, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    surface_desc.dwWidth = 4;
    surface_desc.dwHeight = 1;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    U4(surface_desc).ddpfPixelFormat.dwFlags = DDPF_RGB;
    U1(surface_desc.ddpfPixelFormat).dwRGBBitCount = 32;
    U2(surface_desc.ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(surface_desc.ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(surface_desc.ddpfPixelFormat).dwBBitMask = 0x000000ff;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &dst, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    surface_desc.dwFlags |= DDSD_CKSRCBLT;
    surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue = 0x00ff00ff;
    surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue = 0x00ff00ff;
    hr = IDirectDraw_CreateSurface(ddraw, &surface_desc, &src, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface_Lock(src, NULL, &surface_desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    ok(!(surface_desc.dwFlags & DDSD_LPSURFACE), "Surface desc has LPSURFACE Flags set.\n");
    color = surface_desc.lpSurface;
    color[0] = 0x77010203;
    color[1] = 0x00010203;
    color[2] = 0x77ff00ff;
    color[3] = 0x00ff00ff;
    hr = IDirectDrawSurface_Unlock(src, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    for (i = 0; i < 2; ++i)
    {
        hr = IDirectDrawSurface_Lock(dst, NULL, &surface_desc, DDLOCK_WAIT, NULL);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
        color = surface_desc.lpSurface;
        color[0] = 0xcccccccc;
        color[1] = 0xcccccccc;
        color[2] = 0xcccccccc;
        color[3] = 0xcccccccc;
        hr = IDirectDrawSurface_Unlock(dst, NULL);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

        if (i)
        {
            hr = IDirectDrawSurface_BltFast(dst, 0, 0, src, NULL, DDBLTFAST_SRCCOLORKEY);
            ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);
        }
        else
        {
            hr = IDirectDrawSurface_Blt(dst, NULL, src, NULL, DDBLT_KEYSRC, NULL);
            ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);
        }

        hr = IDirectDrawSurface_Lock(dst, NULL, &surface_desc, DDLOCK_WAIT | DDLOCK_READONLY, NULL);
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
        hr = IDirectDrawSurface_Unlock(dst, NULL);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
    }

    hr = IDirectDrawSurface_GetColorKey(src, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to get color key, hr %#x.\n", hr);
    ok(ckey.dwColorSpaceLowValue == 0x00ff00ff && ckey.dwColorSpaceHighValue == 0x00ff00ff,
            "Got unexpected color key low=%08x high=%08x.\n", ckey.dwColorSpaceLowValue, ckey.dwColorSpaceHighValue);

    ckey.dwColorSpaceLowValue = ckey.dwColorSpaceHighValue = 0x0000ff00;
    hr = IDirectDrawSurface_SetColorKey(src, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);

    ckey.dwColorSpaceLowValue = ckey.dwColorSpaceHighValue = 0;
    hr = IDirectDrawSurface_GetColorKey(src, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to get color key, hr %#x.\n", hr);
    ok(ckey.dwColorSpaceLowValue == 0x0000ff00 && ckey.dwColorSpaceHighValue == 0x0000ff00,
            "Got unexpected color key low=%08x high=%08x.\n", ckey.dwColorSpaceLowValue, ckey.dwColorSpaceHighValue);

    surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue = 0;
    surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue = 0;
    hr = IDirectDrawSurface_GetSurfaceDesc(src, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue == 0x0000ff00
            && surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue == 0x0000ff00,
            "Got unexpected color key low=%08x high=%08x.\n", surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue,
            surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue);

    /* Test SetColorKey with dwColorSpaceHighValue < dwColorSpaceLowValue */
    ckey.dwColorSpaceLowValue = 0x000000ff;
    ckey.dwColorSpaceHighValue = 0x00000000;
    hr = IDirectDrawSurface_SetColorKey(src, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);

    ckey.dwColorSpaceLowValue = ckey.dwColorSpaceHighValue = 0;
    hr = IDirectDrawSurface_GetColorKey(src, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to get color key, hr %#x.\n", hr);
    ok(ckey.dwColorSpaceLowValue == 0x000000ff && ckey.dwColorSpaceHighValue == 0x000000ff,
            "Got unexpected color key low=%08x high=%08x.\n", ckey.dwColorSpaceLowValue, ckey.dwColorSpaceHighValue);

    ckey.dwColorSpaceLowValue = 0x000000ff;
    ckey.dwColorSpaceHighValue = 0x00000001;
    hr = IDirectDrawSurface_SetColorKey(src, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);

    ckey.dwColorSpaceLowValue = ckey.dwColorSpaceHighValue = 0;
    hr = IDirectDrawSurface_GetColorKey(src, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to get color key, hr %#x.\n", hr);
    ok(ckey.dwColorSpaceLowValue == 0x000000ff && ckey.dwColorSpaceHighValue == 0x000000ff,
            "Got unexpected color key low=%08x high=%08x.\n", ckey.dwColorSpaceLowValue, ckey.dwColorSpaceHighValue);

    ckey.dwColorSpaceLowValue = 0x000000fe;
    ckey.dwColorSpaceHighValue = 0x000000fd;
    hr = IDirectDrawSurface_SetColorKey(src, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);

    ckey.dwColorSpaceLowValue = ckey.dwColorSpaceHighValue = 0;
    hr = IDirectDrawSurface_GetColorKey(src, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to get color key, hr %#x.\n", hr);
    ok(ckey.dwColorSpaceLowValue == 0x000000fe && ckey.dwColorSpaceHighValue == 0x000000fe,
            "Got unexpected color key low=%08x high=%08x.\n", ckey.dwColorSpaceLowValue, ckey.dwColorSpaceHighValue);

    IDirectDrawSurface_Release(src);
    IDirectDrawSurface_Release(dst);

    /* Test source and destination keys and where they are read from. Use a surface with alpha
     * to avoid driver-dependent content in the X channel. */
    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    surface_desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    surface_desc.dwWidth = 6;
    surface_desc.dwHeight = 1;
    surface_desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    surface_desc.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    U1(surface_desc.ddpfPixelFormat).dwRGBBitCount = 32;
    U2(surface_desc.ddpfPixelFormat).dwRBitMask = 0x00ff0000;
    U3(surface_desc.ddpfPixelFormat).dwGBitMask = 0x0000ff00;
    U4(surface_desc.ddpfPixelFormat).dwBBitMask = 0x000000ff;
    U5(surface_desc.ddpfPixelFormat).dwRGBAlphaBitMask = 0xff000000;
    hr = IDirectDraw7_CreateSurface(ddraw, &surface_desc, &dst, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    hr = IDirectDraw7_CreateSurface(ddraw, &surface_desc, &src, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    ckey.dwColorSpaceLowValue = 0x0000ff00;
    ckey.dwColorSpaceHighValue = 0x0000ff00;
    hr = IDirectDrawSurface_SetColorKey(dst, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);
    ckey.dwColorSpaceLowValue = 0x00ff0000;
    ckey.dwColorSpaceHighValue = 0x00ff0000;
    hr = IDirectDrawSurface_SetColorKey(dst, DDCKEY_DESTBLT, &ckey);
    ok(SUCCEEDED(hr) || hr == DDERR_NOCOLORKEYHW, "Failed to set color key, hr %#x.\n", hr);
    if (FAILED(hr))
    {
        /* Nvidia reject dest keys, AMD allows them. This applies to vidmem and sysmem surfaces. */
        skip("Failed to set destination color key, skipping related tests.\n");
        goto done;
    }

    ckey.dwColorSpaceLowValue = 0x000000ff;
    ckey.dwColorSpaceHighValue = 0x000000ff;
    hr = IDirectDrawSurface_SetColorKey(src, DDCKEY_SRCBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);
    ckey.dwColorSpaceLowValue = 0x000000aa;
    ckey.dwColorSpaceHighValue = 0x000000aa;
    hr = IDirectDrawSurface_SetColorKey(src, DDCKEY_DESTBLT, &ckey);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);

    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    fx.ddckSrcColorkey.dwColorSpaceHighValue = 0x00110000;
    fx.ddckSrcColorkey.dwColorSpaceLowValue = 0x00110000;
    fx.ddckDestColorkey.dwColorSpaceHighValue = 0x00001100;
    fx.ddckDestColorkey.dwColorSpaceLowValue = 0x00001100;

    hr = IDirectDrawSurface_Lock(src, NULL, &surface_desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    color = surface_desc.lpSurface;
    color[0] = 0x000000ff; /* Applies to src blt key in src surface. */
    color[1] = 0x000000aa; /* Applies to dst blt key in src surface. */
    color[2] = 0x00ff0000; /* Dst color key in dst surface. */
    color[3] = 0x0000ff00; /* Src color key in dst surface. */
    color[4] = 0x00001100; /* Src color key in ddbltfx. */
    color[5] = 0x00110000; /* Dst color key in ddbltfx. */
    hr = IDirectDrawSurface_Unlock(src, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface_Lock(dst, NULL, &surface_desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    color = surface_desc.lpSurface;
    color[0] = color[1] = color[2] = color[3] = color[4] = color[5] = 0x55555555;
    hr = IDirectDrawSurface_Unlock(dst, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    /* Test a blit without keying. */
    hr = IDirectDrawSurface_Blt(dst, NULL, src, NULL, 0, &fx);
    ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);

    hr = IDirectDrawSurface_Lock(dst, NULL, &surface_desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    color = surface_desc.lpSurface;
    /* Should have copied src data unmodified to dst. */
    ok(color[0] == 0x000000ff && color[1] == 0x000000aa && color[2] == 0x00ff0000 &&
            color[3] == 0x0000ff00 && color[4] == 0x00001100 && color[5] == 0x00110000,
            "Got unexpected content %08x %08x %08x %08x %08x %08x.\n",
            color[0], color[1], color[2], color[3], color[4], color[5]);

    color[0] = color[1] = color[2] = color[3] = color[4] = color[5] = 0x55555555;
    hr = IDirectDrawSurface_Unlock(dst, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    /* Src key. */
    hr = IDirectDrawSurface_Blt(dst, NULL, src, NULL, DDBLT_KEYSRC, &fx);
    ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);

    hr = IDirectDrawSurface_Lock(dst, NULL, &surface_desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    color = surface_desc.lpSurface;
    /* Src key applied to color[0]. It is unmodified, the others are copied. */
    ok(color[0] == 0x55555555 && color[1] == 0x000000aa && color[2] == 0x00ff0000 &&
            color[3] == 0x0000ff00 && color[4] == 0x00001100 && color[5] == 0x00110000,
            "Got unexpected content %08x %08x %08x %08x %08x %08x.\n",
            color[0], color[1], color[2], color[3], color[4], color[5]);

    color[0] = color[1] = color[2] = color[3] = color[4] = color[5] = 0x55555555;
    hr = IDirectDrawSurface_Unlock(dst, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    /* Src override. */
    hr = IDirectDrawSurface_Blt(dst, NULL, src, NULL, DDBLT_KEYSRCOVERRIDE, &fx);
    ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);

    hr = IDirectDrawSurface_Lock(dst, NULL, &surface_desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    color = surface_desc.lpSurface;
    /* Override key applied to color[5]. It is unmodified, the others are copied. */
    ok(color[0] == 0x000000ff && color[1] == 0x000000aa && color[2] == 0x00ff0000 &&
            color[3] == 0x0000ff00 && color[4] == 0x00001100 && color[5] == 0x55555555,
            "Got unexpected content %08x %08x %08x %08x %08x %08x.\n",
            color[0], color[1], color[2], color[3], color[4], color[5]);

    color[0] = color[1] = color[2] = color[3] = color[4] = color[5] = 0x55555555;
    hr = IDirectDrawSurface_Unlock(dst, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    /* Src override AND src key. That is not supposed to work. */
    hr = IDirectDrawSurface_Blt(dst, NULL, src, NULL, DDBLT_KEYSRC | DDBLT_KEYSRCOVERRIDE, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    hr = IDirectDrawSurface_Lock(dst, NULL, &surface_desc, DDLOCK_WAIT, NULL);
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
    hr = IDirectDrawSurface_Unlock(dst, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface_Blt(dst, NULL, src, NULL, DDBLT_KEYDEST, &fx);
    ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);

    hr = IDirectDrawSurface_Lock(dst, NULL, &surface_desc, DDLOCK_WAIT, NULL);
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
    hr = IDirectDrawSurface_Unlock(dst, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    /* What happens with a QI'd newer version of the interface? It takes the key
     * from the destination surface. */
    hr = IDirectDrawSurface_QueryInterface(src, &IID_IDirectDrawSurface7, (void **)&src7);
    ok(SUCCEEDED(hr), "Failed to query IDirectDrawSurface interface, hr %#x.\n", hr);
    hr = IDirectDrawSurface_QueryInterface(dst, &IID_IDirectDrawSurface7, (void **)&dst7);
    ok(SUCCEEDED(hr), "Failed to query IDirectDrawSurface interface, hr %#x.\n", hr);

    hr = IDirectDrawSurface7_Blt(dst7, NULL, src7, NULL, DDBLT_KEYDEST, &fx);
    ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);

    IDirectDrawSurface7_Release(dst7);
    IDirectDrawSurface7_Release(src7);

    hr = IDirectDrawSurface_Lock(dst, NULL, &surface_desc, DDLOCK_WAIT, NULL);
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
    hr = IDirectDrawSurface_Unlock(dst, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    /* Dest override key blit. */
    hr = IDirectDrawSurface_Blt(dst, NULL, src, NULL, DDBLT_KEYDESTOVERRIDE, &fx);
    ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);

    hr = IDirectDrawSurface_Lock(dst, NULL, &surface_desc, DDLOCK_WAIT, NULL);
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
    hr = IDirectDrawSurface_Unlock(dst, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    /* Dest override together with surface key. Supposed to fail. */
    hr = IDirectDrawSurface_Blt(dst, NULL, src, NULL, DDBLT_KEYDEST | DDBLT_KEYDESTOVERRIDE, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    hr = IDirectDrawSurface_Lock(dst, NULL, &surface_desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    color = surface_desc.lpSurface;
    /* Destination is unchanged. */
    ok(color[0] == 0x00ff0000 && color[1] == 0x00ff0000 && color[2] == 0x00001100 &&
            color[3] == 0x00001100 && color[4] == 0x000000aa && color[5] == 0x000000aa,
            "Got unexpected content %08x %08x %08x %08x %08x %08x.\n",
            color[0], color[1], color[2], color[3], color[4], color[5]);
    hr = IDirectDrawSurface_Unlock(dst, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    /* Source and destination key. This is driver dependent. New HW treats it like
     * DDBLT_KEYSRC. Older HW and some software renderers apply both keys. */
    if (0)
    {
        hr = IDirectDrawSurface_Blt(dst, NULL, src, NULL, DDBLT_KEYDEST | DDBLT_KEYSRC, &fx);
        ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);

        hr = IDirectDrawSurface_Lock(dst, NULL, &surface_desc, DDLOCK_WAIT, NULL);
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
        hr = IDirectDrawSurface_Unlock(dst, NULL);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
    }

    /* Override keys without ddbltfx parameter fail */
    hr = IDirectDrawSurface_Blt(dst, NULL, src, NULL, DDBLT_KEYDESTOVERRIDE, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);
    hr = IDirectDrawSurface_Blt(dst, NULL, src, NULL, DDBLT_KEYSRCOVERRIDE, NULL);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    /* Try blitting without keys in the source surface. */
    hr = IDirectDrawSurface_SetColorKey(src, DDCKEY_SRCBLT, NULL);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);
    hr = IDirectDrawSurface_SetColorKey(src, DDCKEY_DESTBLT, NULL);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);

    /* That fails now. Do not bother to check that the data is unmodified. */
    hr = IDirectDrawSurface_Blt(dst, NULL, src, NULL, DDBLT_KEYSRC, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

    /* Surprisingly this still works. It uses the old key from the src surface. */
    hr = IDirectDrawSurface_Blt(dst, NULL, src, NULL, DDBLT_KEYDEST, &fx);
    ok(SUCCEEDED(hr), "Failed to blit, hr %#x.\n", hr);

    hr = IDirectDrawSurface_Lock(dst, NULL, &surface_desc, DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    color = surface_desc.lpSurface;
    /* Dst key applied to color[4,5], they are the only changed pixels. */
    ok(color[0] == 0x00ff0000 && color[1] == 0x00ff0000 && color[2] == 0x00001100 &&
            color[3] == 0x00001100 && color[4] == 0x00001100 && color[5] == 0x00110000,
            "Got unexpected content %08x %08x %08x %08x %08x %08x.\n",
            color[0], color[1], color[2], color[3], color[4], color[5]);
    hr = IDirectDrawSurface_Unlock(dst, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    /* This returns DDERR_NOCOLORKEY as expected. */
    hr = IDirectDrawSurface_GetColorKey(src, DDCKEY_DESTBLT, &ckey);
    ok(hr == DDERR_NOCOLORKEY, "Got unexpected hr %#x.\n", hr);

    /* GetSurfaceDesc returns a zeroed key as expected. */
    surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue = 0x12345678;
    surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue = 0x12345678;
    hr = IDirectDrawSurface_GetSurfaceDesc(src, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(!surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue
            && !surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue,
            "Got unexpected color key low=%08x high=%08x.\n", surface_desc.ddckCKSrcBlt.dwColorSpaceLowValue,
            surface_desc.ddckCKSrcBlt.dwColorSpaceHighValue);

    /* Try blitting without keys in the destination surface. */
    hr = IDirectDrawSurface_SetColorKey(dst, DDCKEY_SRCBLT, NULL);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);
    hr = IDirectDrawSurface_SetColorKey(dst, DDCKEY_DESTBLT, NULL);
    ok(SUCCEEDED(hr), "Failed to set color key, hr %#x.\n", hr);

    /* This is weird. It makes sense in v4 and v7, but because v1
     * uses the key from the src surface it makes no sense here. */
    hr = IDirectDrawSurface_Blt(dst, NULL, src, NULL, DDBLT_KEYDEST, &fx);
    ok(hr == DDERR_INVALIDPARAMS, "Got unexpected hr %#x.\n", hr);

done:
    IDirectDrawSurface_Release(src);
    IDirectDrawSurface_Release(dst);
    refcount = IDirectDraw_Release(ddraw);
    ok(!refcount, "DirectDraw has %u references left.\n", refcount);
    DestroyWindow(window);
}

static void test_depth_readback(void)
{
    DWORD depth, expected_depth, max_diff, z_depth, z_mask;
    IDirect3DExecuteBuffer *execute_buffer;
    IDirect3DMaterial *blue_background;
    D3DEXECUTEBUFFERDESC exec_desc;
    IDirectDrawSurface *rt, *ds;
    IDirect3DViewport *viewport;
    DDSURFACEDESC surface_desc;
    IDirect3DDevice *device;
    IDirectDraw *ddraw;
    unsigned int x, y;
    UINT inst_length;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    void *ptr;

    static D3DRECT clear_rect = {{0}, {0}, {640}, {480}};
    static D3DLVERTEX quad[] =
    {
        {{-1.0f}, {-1.0f}, {0.1f}, 0, {0xff00ff00}},
        {{-1.0f}, { 1.0f}, {0.0f}, 0, {0xff00ff00}},
        {{ 1.0f}, {-1.0f}, {1.0f}, 0, {0xff00ff00}},
        {{ 1.0f}, { 1.0f}, {0.9f}, 0, {0xff00ff00}},
    };

    window = create_window();
    ok(!!window, "Failed to create a window.\n");
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    if (ddraw_is_nvidia(ddraw))
    {
        /* ddraw1 only has access to D16 Z buffers (and D24 ones, which are even more
         * broken on Nvidia), so don't even attempt to run this test on Nvidia cards
         * because some of them have broken D16 readback. See the ddraw7 version of
         * this test for a more detailed comment. */
        skip("Some Nvidia GPUs have broken D16 readback, skipping.\n");
        IDirectDraw_Release(ddraw);
        DestroyWindow(window);
        return;
    }

    if (!(device = create_device(ddraw, window, DDSCL_NORMAL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirectDraw_Release(ddraw);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice_QueryInterface(device, &IID_IDirectDrawSurface, (void **)&rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);
    z_depth = get_device_z_depth(device);
    z_mask = 0xffffffff >> (32 - z_depth);
    ds = get_depth_stencil(device);

    /* Changing depth buffers is hard in d3d1, so we only test with the
     * initial depth buffer here. */

    blue_background = create_diffuse_material(device, 0.0f, 0.0f, 1.0f, 1.0f);
    viewport = create_viewport(device, 0, 0, 640, 480);
    viewport_set_background(device, viewport, blue_background);

    memset(&exec_desc, 0, sizeof(exec_desc));
    exec_desc.dwSize = sizeof(exec_desc);
    exec_desc.dwFlags = D3DDEB_BUFSIZE | D3DDEB_CAPS;
    exec_desc.dwBufferSize = 1024;
    exec_desc.dwCaps = D3DDEBCAPS_SYSTEMMEMORY;
    hr = IDirect3DDevice_CreateExecuteBuffer(device, &exec_desc, &execute_buffer, NULL);
    ok(SUCCEEDED(hr), "Failed to create execute buffer, hr %#x.\n", hr);

    hr = IDirect3DExecuteBuffer_Lock(execute_buffer, &exec_desc);
    ok(SUCCEEDED(hr), "Failed to lock execute buffer, hr %#x.\n", hr);

    memcpy(exec_desc.lpData, quad, sizeof(quad));
    ptr = (BYTE *)exec_desc.lpData + sizeof(quad);
    emit_process_vertices(&ptr, D3DPROCESSVERTICES_TRANSFORM, 0, 4);
    emit_tquad(&ptr, 0);
    emit_end(&ptr);

    inst_length = ((BYTE *)ptr - sizeof(quad)) - (BYTE *)exec_desc.lpData;

    hr = IDirect3DExecuteBuffer_Unlock(execute_buffer);
    ok(SUCCEEDED(hr), "Failed to unlock execute buffer, hr %#x.\n", hr);

    hr = IDirect3DViewport_Clear(viewport, 1, &clear_rect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);
    hr = IDirect3DDevice_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    set_execute_data(execute_buffer, 4, sizeof(quad), inst_length);
    hr = IDirect3DDevice_Execute(device, execute_buffer, viewport, D3DEXECUTE_UNCLIPPED);
    ok(SUCCEEDED(hr), "Failed to execute exec buffer, hr %#x.\n", hr);
    hr = IDirect3DDevice_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    memset(&surface_desc, 0, sizeof(surface_desc));
    surface_desc.dwSize = sizeof(surface_desc);
    hr = IDirectDrawSurface_Lock(ds, NULL, &surface_desc, DDLOCK_READONLY | DDLOCK_WAIT, NULL);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);

    for (y = 60; y < 480; y += 120)
    {
        for (x = 80; x < 640; x += 160)
        {
            ptr = (BYTE *)surface_desc.lpSurface
                    + y * U1(surface_desc).lPitch
                    + x * (z_depth == 16 ? 2 : 4);
            depth = *((DWORD *)ptr) & z_mask;
            expected_depth = (x * (0.9 / 640.0) + y * (0.1 / 480.0)) * z_mask;
            max_diff = ((0.5f * 0.9f) / 640.0f) * z_mask;
            ok(abs(expected_depth - depth) <= max_diff,
                    "z_depth %u: Got depth 0x%08x (diff %d), expected 0x%08x+/-%u, at %u, %u.\n",
                    z_depth, depth, expected_depth - depth, expected_depth, max_diff, x, y);
        }
    }

    hr = IDirectDrawSurface_Unlock(ds, NULL);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    IDirect3DExecuteBuffer_Release(execute_buffer);
    destroy_viewport(device, viewport);
    destroy_material(blue_background);
    IDirectDrawSurface_Release(ds);
    IDirect3DDevice_Release(device);
    refcount = IDirectDrawSurface_Release(rt);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirectDraw_Release(ddraw);
    DestroyWindow(window);
}

static void test_clear(void)
{
    D3DRECT rect_negneg, rect_full = {{0}, {0}, {640}, {480}};
    IDirect3DViewport *viewport, *viewport2, *viewport3;
    IDirect3DMaterial *white, *red, *green, *blue;
    IDirect3DDevice *device;
    IDirectDrawSurface *rt;
    IDirectDraw *ddraw;
    D3DRECT rect[2];
    D3DCOLOR color;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = create_window();
    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");
    if (!(device = create_device(ddraw, window, DDSCL_NORMAL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        IDirectDraw_Release(ddraw);
        DestroyWindow(window);
        return;
    }
    hr = IDirect3DDevice_QueryInterface(device, &IID_IDirectDrawSurface, (void **)&rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    viewport = create_viewport(device, 0, 0, 640, 480);

    white = create_diffuse_material(device, 1.0f, 1.0f, 1.0f, 1.0f);
    red = create_diffuse_material(device, 1.0f, 0.0f, 0.0f, 1.0f);
    green = create_diffuse_material(device, 0.0f, 1.0f, 0.0f, 1.0f);
    blue = create_diffuse_material(device, 0.0f, 0.0f, 1.0f, 1.0f);

    viewport_set_background(device, viewport, white);
    hr = IDirect3DViewport_Clear(viewport, 1, &rect_full, D3DCLEAR_TARGET);
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
    viewport_set_background(device, viewport, red);
    hr = IDirect3DViewport_Clear(viewport, 2, rect, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

    color = get_surface_color(rt, 160, 360);
    ok(compare_color(color, 0x00ffffff, 0), "Clear rectangle 3 (pos, neg) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 160, 120);
    ok(compare_color(color, 0x00ff0000, 0), "Clear rectangle 1 (pos, pos) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 480, 360);
    ok(compare_color(color, 0x00ffffff, 0), "Clear rectangle 4 (NULL) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 480, 120);
    ok(compare_color(color, 0x00ffffff, 0), "Clear rectangle 4 (neg, neg) has color 0x%08x.\n", color);

    viewport_set_background(device, viewport, white);
    hr = IDirect3DViewport_Clear(viewport, 1, &rect_full, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

    /* negative x, negative y.
     * Also ignored, except on WARP, which clears the entire screen. */
    rect_negneg.x1 = 640;
    rect_negneg.y1 = 240;
    rect_negneg.x2 = 320;
    rect_negneg.y2 = 0;
    viewport_set_background(device, viewport, green);
    hr = IDirect3DViewport_Clear(viewport, 1, &rect_negneg, D3DCLEAR_TARGET);
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
    viewport_set_background(device, viewport, white);
    hr = IDirect3DViewport_Clear(viewport, 1, &rect_full, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

    viewport2 = create_viewport(device, 160, 120, 160, 120);
    viewport_set_background(device, viewport2, blue);
    hr = IDirect3DViewport_Clear(viewport2, 1, &rect_full, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

    viewport3 = create_viewport(device, 320, 240, 320, 240);
    viewport_set_background(device, viewport3, green);

    U1(rect[0]).x1 = 160;
    U2(rect[0]).y1 = 120;
    U3(rect[0]).x2 = 480;
    U4(rect[0]).y2 = 360;
    hr = IDirect3DViewport_Clear(viewport3, 1, &rect[0], D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);

    /* AMD drivers do not limit the clear area to the viewport rectangle in
     * d3d1. It works as intended on other drivers and on d3d2 and newer on
     * AMD cards. */
    color = get_surface_color(rt, 158, 118);
    ok(compare_color(color, 0x00ffffff, 0)
            || broken(ddraw_is_amd(ddraw) && compare_color(color, 0x000000ff, 0)),
            "(158, 118) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 162, 118);
    ok(compare_color(color, 0x00ffffff, 0)
            || broken(ddraw_is_amd(ddraw) && compare_color(color, 0x000000ff, 0)),
            "(162, 118) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 158, 122);
    ok(compare_color(color, 0x00ffffff, 0)
            || broken(ddraw_is_amd(ddraw) && compare_color(color, 0x000000ff, 0)),
            "(158, 122) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 162, 122);
    ok(compare_color(color, 0x000000ff, 0)
            || broken(ddraw_is_amd(ddraw) && compare_color(color, 0x0000ff00, 0)),
            "(162, 122) has color 0x%08x.\n", color);

    color = get_surface_color(rt, 318, 238);
    ok(compare_color(color, 0x000000ff, 0)
            || broken(ddraw_is_amd(ddraw) && compare_color(color, 0x0000ff00, 0)),
            "(318, 238) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 322, 238);
    ok(compare_color(color, 0x00ffffff, 0)
            || broken(ddraw_is_amd(ddraw) && compare_color(color, 0x0000ff00, 0)),
            "(322, 328) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 318, 242);
    ok(compare_color(color, 0x00ffffff, 0)
            || broken(ddraw_is_amd(ddraw) && compare_color(color, 0x0000ff00, 0)),
            "(318, 242) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 322, 242);
    ok(compare_color(color, 0x0000ff00, 0), "(322, 242) has color 0x%08x.\n", color);

    color = get_surface_color(rt, 478, 358);
    ok(compare_color(color, 0x0000ff00, 0), "(478, 358) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 482, 358);
    ok(compare_color(color, 0x00ffffff, 0)
            || broken(ddraw_is_amd(ddraw) && compare_color(color, 0x000000ff, 0)),
            "(482, 358) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 478, 362);
    ok(compare_color(color, 0x00ffffff, 0)
            || broken(ddraw_is_amd(ddraw) && compare_color(color, 0x000000ff, 0)),
            "(478, 362) has color 0x%08x.\n", color);
    color = get_surface_color(rt, 482, 362);
    ok(compare_color(color, 0x00ffffff, 0)
            || broken(ddraw_is_amd(ddraw) && compare_color(color, 0x000000ff, 0)),
            "(482, 362) has color 0x%08x.\n", color);

    /* The clear rectangle is rendertarget absolute, not relative to the
     * viewport. */
    hr = IDirect3DViewport_Clear(viewport, 1, &rect_full, D3DCLEAR_TARGET);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);
    U1(rect[0]).x1 = 330;
    U2(rect[0]).y1 = 250;
    U3(rect[0]).x2 = 340;
    U4(rect[0]).y2 = 260;
    hr = IDirect3DViewport_Clear(viewport3, 1, &rect[0], D3DCLEAR_TARGET);
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
     * in d3d1. */

    IDirect3DViewport_Release(viewport3);
    IDirect3DViewport_Release(viewport2);
    IDirect3DViewport_Release(viewport);
    IDirect3DMaterial_Release(white);
    IDirect3DMaterial_Release(red);
    IDirect3DMaterial_Release(green);
    IDirect3DMaterial_Release(blue);
    IDirectDrawSurface_Release(rt);
    refcount = IDirect3DDevice_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    refcount = IDirectDraw_Release(ddraw);
    ok(!refcount, "Ddraw object has %u references left.\n", refcount);
    DestroyWindow(window);
}

struct enum_surfaces_param
{
    IDirectDrawSurface *surfaces[8];
    unsigned int count;
};

static HRESULT WINAPI enum_surfaces_cb(IDirectDrawSurface *surface, DDSURFACEDESC *desc, void *context)
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
    IDirectDrawSurface_Release(surface);
    ++param->count;

    return DDENUMRET_OK;
}

static void test_enum_surfaces(void)
{
    struct enum_surfaces_param param = {{0}};
    IDirectDraw *ddraw;
    DDSURFACEDESC desc;
    HRESULT hr;

    ddraw = create_ddraw();
    ok(!!ddraw, "Failed to create a ddraw object.\n");

    hr = IDirectDraw_SetCooperativeLevel(ddraw, NULL, DDSCL_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set cooperative level, hr %#x.\n", hr);

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_MIPMAPCOUNT;
    desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
    U2(desc).dwMipMapCount = 3;
    desc.dwWidth = 32;
    desc.dwHeight = 32;
    hr = IDirectDraw_CreateSurface(ddraw, &desc, &param.surfaces[0], NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirectDrawSurface_GetAttachedSurface(param.surfaces[0], &desc.ddsCaps, &param.surfaces[1]);
    ok(SUCCEEDED(hr), "Failed to get attached surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface_GetAttachedSurface(param.surfaces[1], &desc.ddsCaps, &param.surfaces[2]);
    ok(SUCCEEDED(hr), "Failed to get attached surface, hr %#x.\n", hr);
    hr = IDirectDrawSurface_GetAttachedSurface(param.surfaces[2], &desc.ddsCaps, &param.surfaces[3]);
    ok(hr == DDERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);
    ok(!param.surfaces[3], "Got unexpected pointer %p.\n", param.surfaces[3]);

    hr = IDirectDraw_EnumSurfaces(ddraw, DDENUMSURFACES_DOESEXIST | DDENUMSURFACES_ALL,
            &desc, &param, enum_surfaces_cb);
    ok(SUCCEEDED(hr), "Failed to enumerate surfaces, hr %#x.\n", hr);
    ok(param.count == 3, "Got unexpected number of enumerated surfaces %u.\n", param.count);

    param.count = 0;
    hr = IDirectDraw_EnumSurfaces(ddraw, DDENUMSURFACES_DOESEXIST | DDENUMSURFACES_ALL,
            NULL, &param, enum_surfaces_cb);
    ok(SUCCEEDED(hr), "Failed to enumerate surfaces, hr %#x.\n", hr);
    ok(param.count == 3, "Got unexpected number of enumerated surfaces %u.\n", param.count);

    IDirectDrawSurface_Release(param.surfaces[2]);
    IDirectDrawSurface_Release(param.surfaces[1]);
    IDirectDrawSurface_Release(param.surfaces[0]);
    IDirectDraw_Release(ddraw);
}

START_TEST(ddraw1)
{
    DDDEVICEIDENTIFIER identifier;
    DEVMODEW current_mode;
    IDirectDraw *ddraw;
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
    IDirectDraw_Release(ddraw);

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

    test_coop_level_create_device_window();
    test_clipper_blt();
    test_coop_level_d3d_state();
    test_surface_interface_mismatch();
    test_coop_level_threaded();
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
    test_coop_level_multi_window();
    test_clear_rect_count();
    test_coop_level_activateapp();
    test_unsupported_formats();
    test_rt_caps();
    test_primary_caps();
    test_surface_lock();
    test_surface_discard();
    test_flip();
    test_sysmem_overlay();
    test_primary_palette();
    test_surface_attachment();
    test_pixel_format();
    test_create_surface_pitch();
    test_mipmap();
    test_palette_complex();
    test_p8_blit();
    test_material();
    test_lighting();
    test_palette_gdi();
    test_palette_alpha();
    test_lost_device();
    test_surface_desc_lock();
    test_texturemapblend();
    test_viewport_clear_rect();
    test_color_fill();
    test_colorkey_precision();
    test_range_colorkey();
    test_shademode();
    test_lockrect_invalid();
    test_yv12_overlay();
    test_offscreen_overlay();
    test_overlay_rect();
    test_blt();
    test_blt_z_alpha();
    test_getdc();
    test_transform_vertices();
    test_display_mode_surface_pixel_format();
    test_surface_desc_size();
    test_texture_load();
    test_ck_operation();
    test_depth_readback();
    test_clear();
    test_enum_surfaces();
}
