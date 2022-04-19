/*
 * Copyright (C) 2006 Ivan Gyurdiev
 * Copyright (C) 2006 Vitaliy Margolen
 * Copyright (C) 2006 Chris Robinson
 * Copyright 2006-2008, 2010-2011, 2013 Stefan Dösinger for CodeWeavers
 * Copyright 2005, 2006, 2007 Henri Verbeet
 * Copyright 2013-2014 Henri Verbeet for CodeWeavers
 * Copyright (C) 2008 Rico Schüller
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
#include "utils.h"
#include "wine/heap.h"

struct vec3
{
    float x, y, z;
};

#define CREATE_DEVICE_FULLSCREEN                0x01
#define CREATE_DEVICE_NOWINDOWCHANGES           0x02
#define CREATE_DEVICE_FPU_PRESERVE              0x04
#define CREATE_DEVICE_SWVP_ONLY                 0x08
#define CREATE_DEVICE_MIXED_ONLY                0x10
#define CREATE_DEVICE_UNKNOWN_BACKBUFFER_FORMAT 0x20
#define CREATE_DEVICE_LOCKABLE_BACKBUFFER       0x40

struct device_desc
{
    unsigned int adapter_ordinal;
    HWND device_window;
    unsigned int width;
    unsigned int height;
    DWORD flags;
};

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

static DEVMODEW registry_mode;

static void *(WINAPI *Direct3DShaderValidatorCreate9)(void);

static const DWORD simple_vs[] =
{
    0xfffe0101,                                                             /* vs_1_1                       */
    0x0000001f, 0x80000000, 0x900f0000,                                     /* dcl_position0 v0             */
    0x00000009, 0xc0010000, 0x90e40000, 0xa0e40000,                         /* dp4 oPos.x, v0, c0           */
    0x00000009, 0xc0020000, 0x90e40000, 0xa0e40001,                         /* dp4 oPos.y, v0, c1           */
    0x00000009, 0xc0040000, 0x90e40000, 0xa0e40002,                         /* dp4 oPos.z, v0, c2           */
    0x00000009, 0xc0080000, 0x90e40000, 0xa0e40003,                         /* dp4 oPos.w, v0, c3           */
    0x0000ffff,                                                             /* end                          */
};

static const DWORD simple_ps[] =
{
    0xffff0101,                                                             /* ps_1_1                       */
    0x00000051, 0xa00f0001, 0x3f800000, 0x00000000, 0x00000000, 0x00000000, /* def c1 = 1.0, 0.0, 0.0, 0.0  */
    0x00000042, 0xb00f0000,                                                 /* tex t0                       */
    0x00000008, 0x800f0000, 0xa0e40001, 0xa0e40000,                         /* dp3 r0, c1, c0               */
    0x00000005, 0x800f0000, 0x90e40000, 0x80e40000,                         /* mul r0, v0, r0               */
    0x00000005, 0x800f0000, 0xb0e40000, 0x80e40000,                         /* mul r0, t0, r0               */
    0x0000ffff,                                                             /* end                          */
};

static int get_refcount(IUnknown *object)
{
    IUnknown_AddRef( object );
    return IUnknown_Release( object );
}

static BOOL compare_elements(IDirect3DVertexDeclaration9 *declaration, const D3DVERTEXELEMENT9 *expected_elements)
{
    unsigned int element_count, i;
    D3DVERTEXELEMENT9 *elements;
    BOOL equal = TRUE;
    HRESULT hr;

    hr = IDirect3DVertexDeclaration9_GetDeclaration(declaration, NULL, &element_count);
    ok(SUCCEEDED(hr), "Failed to get declaration, hr %#x.\n", hr);
    elements = HeapAlloc(GetProcessHeap(), 0, element_count * sizeof(*elements));
    hr = IDirect3DVertexDeclaration9_GetDeclaration(declaration, elements, &element_count);
    ok(SUCCEEDED(hr), "Failed to get declaration, hr %#x.\n", hr);

    for (i = 0; i < element_count; ++i)
    {
        if (memcmp(&elements[i], &expected_elements[i], sizeof(*elements)))
        {
            equal = FALSE;
            break;
        }
    }

    if (!equal)
    {
        for (i = 0; i < element_count; ++i)
        {
            trace("[Element %u] stream %u, offset %u, type %#x, method %#x, usage %#x, usage index %u.\n",
                    i, elements[i].Stream, elements[i].Offset, elements[i].Type,
                    elements[i].Method, elements[i].Usage, elements[i].UsageIndex);
        }
    }

    HeapFree(GetProcessHeap(), 0, elements);
    return equal;
}

static void get_virtual_rect(RECT *rect)
{
    rect->left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    rect->top = GetSystemMetrics(SM_YVIRTUALSCREEN);
    rect->right = rect->left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
    rect->bottom = rect->top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
}

static HWND create_window(void)
{
    RECT r = {0, 0, 640, 480};

    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW | WS_VISIBLE, FALSE);

    return CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, r.right - r.left, r.bottom - r.top, NULL, NULL, NULL, NULL);
}

/* try to make sure pending X events have been processed before continuing */
static void flush_events(void)
{
    MSG msg;
    int diff = 200;
    int min_timeout = 100;
    DWORD time = GetTickCount() + diff;

    while (diff > 0)
    {
        if (MsgWaitForMultipleObjects( 0, NULL, FALSE, min_timeout, QS_ALLINPUT ) == WAIT_TIMEOUT) break;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
        diff = time - GetTickCount();
    }
}

static BOOL adapter_is_warp(const D3DADAPTER_IDENTIFIER9 *identifier)
{
    return !strcmp(identifier->Driver, "d3d10warp.dll");
}

static BOOL equal_mode_rect(const DEVMODEW *mode1, const DEVMODEW *mode2)
{
    return mode1->dmPosition.x == mode2->dmPosition.x
            && mode1->dmPosition.y == mode2->dmPosition.y
            && mode1->dmPelsWidth == mode2->dmPelsWidth
            && mode1->dmPelsHeight == mode2->dmPelsHeight;
}

/* Free original_modes after finished using it */
static BOOL save_display_modes(DEVMODEW **original_modes, unsigned int *display_count)
{
    unsigned int number, size = 2, count = 0, index = 0;
    DISPLAY_DEVICEW display_device;
    DEVMODEW *modes, *tmp;

    if (!(modes = heap_alloc(size * sizeof(*modes))))
        return FALSE;

    display_device.cb = sizeof(display_device);
    while (EnumDisplayDevicesW(NULL, index++, &display_device, 0))
    {
        /* Skip software devices */
        if (swscanf(display_device.DeviceName, L"\\\\.\\DISPLAY%u", &number) != 1)
            continue;

        if (!(display_device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
            continue;

        if (count >= size)
        {
            size *= 2;
            if (!(tmp = heap_realloc(modes, size * sizeof(*modes))))
            {
                heap_free(modes);
                return FALSE;
            }
            modes = tmp;
        }

        memset(&modes[count], 0, sizeof(modes[count]));
        modes[count].dmSize = sizeof(modes[count]);
        if (!EnumDisplaySettingsW(display_device.DeviceName, ENUM_CURRENT_SETTINGS, &modes[count]))
        {
            heap_free(modes);
            return FALSE;
        }

        lstrcpyW(modes[count++].dmDeviceName, display_device.DeviceName);
    }

    *original_modes = modes;
    *display_count = count;
    return TRUE;
}

static BOOL restore_display_modes(DEVMODEW *modes, unsigned int count)
{
    unsigned int index;
    LONG ret;

    for (index = 0; index < count; ++index)
    {
        ret = ChangeDisplaySettingsExW(modes[index].dmDeviceName, &modes[index], NULL,
                CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
        if (ret != DISP_CHANGE_SUCCESSFUL)
            return FALSE;
    }
    ret = ChangeDisplaySettingsExW(NULL, NULL, NULL, 0, NULL);
    return ret == DISP_CHANGE_SUCCESSFUL;
}

static IDirect3DDevice9 *create_device(IDirect3D9 *d3d9, HWND focus_window, const struct device_desc *desc)
{
    D3DPRESENT_PARAMETERS present_parameters = {0};
    unsigned int adapter_ordinal;
    IDirect3DDevice9 *device;
    DWORD behavior_flags = D3DCREATE_HARDWARE_VERTEXPROCESSING;

    adapter_ordinal = D3DADAPTER_DEFAULT;
    present_parameters.BackBufferWidth = 640;
    present_parameters.BackBufferHeight = 480;
    present_parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.hDeviceWindow = focus_window;
    present_parameters.Windowed = TRUE;
    present_parameters.EnableAutoDepthStencil = TRUE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;

    if (desc)
    {
        adapter_ordinal = desc->adapter_ordinal;
        present_parameters.BackBufferWidth = desc->width;
        present_parameters.BackBufferHeight = desc->height;
        if (desc->flags & CREATE_DEVICE_UNKNOWN_BACKBUFFER_FORMAT)
            present_parameters.BackBufferFormat = D3DFMT_UNKNOWN;
        present_parameters.hDeviceWindow = desc->device_window;
        present_parameters.Windowed = !(desc->flags & CREATE_DEVICE_FULLSCREEN);
        if (desc->flags & CREATE_DEVICE_LOCKABLE_BACKBUFFER)
            present_parameters.Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
        if (desc->flags & CREATE_DEVICE_SWVP_ONLY)
            behavior_flags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        else if (desc->flags & CREATE_DEVICE_MIXED_ONLY)
            behavior_flags = D3DCREATE_MIXED_VERTEXPROCESSING;
        if (desc->flags & CREATE_DEVICE_NOWINDOWCHANGES)
            behavior_flags |= D3DCREATE_NOWINDOWCHANGES;
        if (desc->flags & CREATE_DEVICE_FPU_PRESERVE)
            behavior_flags |= D3DCREATE_FPU_PRESERVE;
    }

    if (SUCCEEDED(IDirect3D9_CreateDevice(d3d9, adapter_ordinal, D3DDEVTYPE_HAL, focus_window,
            behavior_flags, &present_parameters, &device)))
        return device;

    present_parameters.AutoDepthStencilFormat = D3DFMT_D16;
    if (SUCCEEDED(IDirect3D9_CreateDevice(d3d9, adapter_ordinal, D3DDEVTYPE_HAL, focus_window,
            behavior_flags, &present_parameters, &device)))
        return device;

    if (desc && (desc->flags & (CREATE_DEVICE_SWVP_ONLY | CREATE_DEVICE_MIXED_ONLY)))
        return NULL;
    behavior_flags = (behavior_flags
            & ~(D3DCREATE_MIXED_VERTEXPROCESSING | D3DCREATE_SOFTWARE_VERTEXPROCESSING))
            | D3DCREATE_HARDWARE_VERTEXPROCESSING;

    if (SUCCEEDED(IDirect3D9_CreateDevice(d3d9, adapter_ordinal, D3DDEVTYPE_HAL, focus_window,
            behavior_flags, &present_parameters, &device)))
        return device;

    return NULL;
}

static HRESULT reset_device(IDirect3DDevice9 *device, const struct device_desc *desc)
{
    D3DPRESENT_PARAMETERS present_parameters = {0};

    present_parameters.BackBufferWidth = 640;
    present_parameters.BackBufferHeight = 480;
    present_parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.hDeviceWindow = NULL;
    present_parameters.Windowed = TRUE;
    present_parameters.EnableAutoDepthStencil = TRUE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;

    if (desc)
    {
        present_parameters.BackBufferWidth = desc->width;
        present_parameters.BackBufferHeight = desc->height;
        present_parameters.hDeviceWindow = desc->device_window;
        present_parameters.Windowed = !(desc->flags & CREATE_DEVICE_FULLSCREEN);
    }

    return IDirect3DDevice9_Reset(device, &present_parameters);
}

#define CHECK_CALL(r,c,d,rc) \
    if (SUCCEEDED(r)) {\
        int tmp1 = get_refcount( (IUnknown *)d ); \
        int rc_new = rc; \
        ok(tmp1 == rc_new, "Invalid refcount. Expected %d got %d\n", rc_new, tmp1); \
    } else {\
        trace("%s failed: %08x\n", c, r); \
    }

#define CHECK_RELEASE(obj,d,rc) \
    if (obj) { \
        int tmp1, rc_new = rc; \
        IUnknown_Release( (IUnknown*)obj ); \
        tmp1 = get_refcount( (IUnknown *)d ); \
        ok(tmp1 == rc_new, "Invalid refcount. Expected %d got %d\n", rc_new, tmp1); \
    }

#define CHECK_REFCOUNT(obj,rc) \
    { \
        int rc_new = rc; \
        int count = get_refcount( (IUnknown *)obj ); \
        ok(count == rc_new, "Invalid refcount. Expected %d got %d\n", rc_new, count); \
    }

#define CHECK_RELEASE_REFCOUNT(obj,rc) \
    { \
        int rc_new = rc; \
        int count = IUnknown_Release( (IUnknown *)obj ); \
        ok(count == rc_new, "Invalid refcount. Expected %d got %d\n", rc_new, count); \
    }

#define CHECK_ADDREF_REFCOUNT(obj,rc) \
    { \
        int rc_new = rc; \
        int count = IUnknown_AddRef( (IUnknown *)obj ); \
        ok(count == rc_new, "Invalid refcount. Expected %d got %d\n", rc_new, count); \
    }

#define CHECK_SURFACE_CONTAINER(obj,iid,expected) \
    { \
        void *container_ptr = (void *)0x1337c0d3; \
        hr = IDirect3DSurface9_GetContainer(obj, &iid, &container_ptr); \
        ok(SUCCEEDED(hr) && container_ptr == expected, "GetContainer returned: hr %#x, container_ptr %p. " \
            "Expected hr %#x, container_ptr %p\n", hr, container_ptr, S_OK, expected); \
        if (container_ptr && container_ptr != (void *)0x1337c0d3) IUnknown_Release((IUnknown *)container_ptr); \
    }

static void test_get_set_vertex_declaration(void)
{
    IDirect3DVertexDeclaration9 *declaration, *tmp;
    ULONG refcount, expected_refcount;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    HWND window;
    HRESULT hr;

    static const D3DVERTEXELEMENT9 simple_decl[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        D3DDECL_END()
    };

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_CreateVertexDeclaration(device, simple_decl, &declaration);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);

    /* SetVertexDeclaration() should not touch the declaration's refcount. */
    expected_refcount = get_refcount((IUnknown *)declaration);
    hr = IDirect3DDevice9_SetVertexDeclaration(device, declaration);
    ok(SUCCEEDED(hr), "Failed to set vertex declaration, hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)declaration);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);

    /* GetVertexDeclaration() should increase the declaration's refcount by one. */
    tmp = NULL;
    expected_refcount = refcount + 1;
    hr = IDirect3DDevice9_GetVertexDeclaration(device, &tmp);
    ok(SUCCEEDED(hr), "Failed to get vertex declaration, hr %#x.\n", hr);
    ok(tmp == declaration, "Got unexpected declaration %p, expected %p.\n", tmp, declaration);
    refcount = get_refcount((IUnknown *)declaration);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    IDirect3DVertexDeclaration9_Release(tmp);

    IDirect3DVertexDeclaration9_Release(declaration);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_get_declaration(void)
{
    unsigned int element_count, expected_element_count;
    IDirect3DVertexDeclaration9 *declaration;
    D3DVERTEXELEMENT9 *elements;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    static const D3DVERTEXELEMENT9 simple_decl[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        D3DDECL_END()
    };

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_CreateVertexDeclaration(device, simple_decl, &declaration);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);

    /* First test only getting the number of elements. */
    element_count = 0x1337c0de;
    expected_element_count = ARRAY_SIZE(simple_decl);
    hr = IDirect3DVertexDeclaration9_GetDeclaration(declaration, NULL, &element_count);
    ok(SUCCEEDED(hr), "Failed to get declaration, hr %#x.\n", hr);
    ok(element_count == expected_element_count, "Got unexpected element count %u, expected %u.\n",
            element_count, expected_element_count);

    element_count = 0;
    hr = IDirect3DVertexDeclaration9_GetDeclaration(declaration, NULL, &element_count);
    ok(SUCCEEDED(hr), "Failed to get declaration, hr %#x.\n", hr);
    ok(element_count == expected_element_count, "Got unexpected element count %u, expected %u.\n",
            element_count, expected_element_count);

    /* Also test the returned data. */
    elements = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(simple_decl));

    element_count = 0x1337c0de;
    hr = IDirect3DVertexDeclaration9_GetDeclaration(declaration, elements, &element_count);
    ok(SUCCEEDED(hr), "Failed to get declaration, hr %#x.\n", hr);
    ok(element_count == expected_element_count, "Got unexpected element count %u, expected %u.\n",
            element_count, expected_element_count);
    ok(!memcmp(elements, simple_decl, element_count * sizeof(*elements)),
            "Original and returned vertexdeclarations are not the same.\n");

    memset(elements, 0, sizeof(simple_decl));

    element_count = 0;
    hr = IDirect3DVertexDeclaration9_GetDeclaration(declaration, elements, &element_count);
    ok(SUCCEEDED(hr), "Failed to get declaration, hr %#x.\n", hr);
    ok(element_count == expected_element_count, "Got unexpected element count %u, expected %u.\n",
            element_count, expected_element_count);
    ok(!memcmp(elements, simple_decl, element_count * sizeof(*elements)),
            "Original and returned vertexdeclarations are not the same.\n");

    HeapFree(GetProcessHeap(), 0, elements);
    IDirect3DVertexDeclaration9_Release(declaration);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_fvf_decl_conversion(void)
{
    IDirect3DVertexDeclaration9 *default_decl;
    IDirect3DVertexDeclaration9 *declaration;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    unsigned int i;
    HWND window;
    HRESULT hr;

    static const D3DVERTEXELEMENT9 default_elements[] =
    {
        {0, 0, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 0},
        {0, 4, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR, 1},
        D3DDECL_END()
    };
    /* Test conversions from vertex declaration to an FVF. For some reason
     * those seem to occur only for POSITION/POSITIONT, otherwise the FVF is
     * forced to 0 - maybe this is configuration specific. */
    static const struct
    {
        D3DVERTEXELEMENT9 elements[7];
        DWORD fvf;
        BOOL todo;
    }
    decl_to_fvf_tests[] =
    {
        {{{0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_POSITION,     0}, D3DDECL_END()}, D3DFVF_XYZ,    TRUE },
        {{{0, 0,  D3DDECLTYPE_FLOAT4,   0, D3DDECLUSAGE_POSITIONT,    0}, D3DDECL_END()}, D3DFVF_XYZRHW, TRUE },
        {{{0, 0,  D3DDECLTYPE_FLOAT1,   0, D3DDECLUSAGE_BLENDWEIGHT,  0}, D3DDECL_END()}, 0,             FALSE},
        {{{0, 0,  D3DDECLTYPE_FLOAT2,   0, D3DDECLUSAGE_BLENDWEIGHT,  0}, D3DDECL_END()}, 0,             FALSE},
        {{{0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_BLENDWEIGHT,  0}, D3DDECL_END()}, 0,             FALSE},
        {{{0, 0,  D3DDECLTYPE_FLOAT4,   0, D3DDECLUSAGE_BLENDWEIGHT,  0}, D3DDECL_END()}, 0,             FALSE},
        {{{0, 0,  D3DDECLTYPE_UBYTE4,   0, D3DDECLUSAGE_BLENDINDICES, 0}, D3DDECL_END()}, 0,             FALSE},
        {{{0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_NORMAL,       0}, D3DDECL_END()}, 0,             FALSE},
        {{{0, 0,  D3DDECLTYPE_FLOAT1,   0, D3DDECLUSAGE_PSIZE,        0}, D3DDECL_END()}, 0,             FALSE},
        {{{0, 0,  D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR,        0}, D3DDECL_END()}, 0,             FALSE},
        {{{0, 0,  D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR,        1}, D3DDECL_END()}, 0,             FALSE},
        /* No FVF mapping available. */
        {{{0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_POSITION,     1}, D3DDECL_END()}, 0,             FALSE},
        {{{0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_NORMAL,       1}, D3DDECL_END()}, 0,             FALSE},
        /* Try empty declaration. */
        {{                                                                D3DDECL_END()}, 0,             FALSE},
        /* Make sure textures of different sizes work. */
        {{{0, 0,  D3DDECLTYPE_FLOAT1,   0, D3DDECLUSAGE_TEXCOORD,     0}, D3DDECL_END()}, 0,             FALSE},
        {{{0, 0,  D3DDECLTYPE_FLOAT2,   0, D3DDECLUSAGE_TEXCOORD,     0}, D3DDECL_END()}, 0,             FALSE},
        {{{0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_TEXCOORD,     0}, D3DDECL_END()}, 0,             FALSE},
        {{{0, 0,  D3DDECLTYPE_FLOAT4,   0, D3DDECLUSAGE_TEXCOORD,     0}, D3DDECL_END()}, 0,             FALSE},
        /* Make sure the TEXCOORD index works correctly - try several textures. */
        {
            {
                {0, 0,  D3DDECLTYPE_FLOAT1,   0, D3DDECLUSAGE_TEXCOORD,     0},
                {0, 4,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_TEXCOORD,     1},
                {0, 16, D3DDECLTYPE_FLOAT2,   0, D3DDECLUSAGE_TEXCOORD,     2},
                {0, 24, D3DDECLTYPE_FLOAT4,   0, D3DDECLUSAGE_TEXCOORD,     3},
                D3DDECL_END(),
            }, 0, FALSE,
        },
        /* Now try a combination test. */
        {
            {
                {0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_POSITIONT,    0},
                {0, 12, D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_NORMAL,       0},
                {0, 24, D3DDECLTYPE_FLOAT1,   0, D3DDECLUSAGE_PSIZE,        0},
                {0, 28, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR,        1},
                {0, 32, D3DDECLTYPE_FLOAT1,   0, D3DDECLUSAGE_TEXCOORD,     0},
                {0, 44, D3DDECLTYPE_FLOAT4,   0, D3DDECLUSAGE_TEXCOORD,     1},
                D3DDECL_END(),
            }, 0, FALSE,
        },
    };
    /* Test conversions from FVF to a vertex declaration. These seem to always
     * occur internally. A new declaration object is created if necessary. */
    static const struct
    {
        DWORD fvf;
        D3DVERTEXELEMENT9 elements[7];
    }
    fvf_to_decl_tests[] =
    {
        {D3DFVF_XYZ,      {{0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_POSITION,     0}, D3DDECL_END()}},
        {D3DFVF_XYZW,     {{0, 0,  D3DDECLTYPE_FLOAT4,   0, D3DDECLUSAGE_POSITION,     0}, D3DDECL_END()}},
        {D3DFVF_XYZRHW,   {{0, 0,  D3DDECLTYPE_FLOAT4,   0, D3DDECLUSAGE_POSITIONT,    0}, D3DDECL_END()}},
        {
            D3DFVF_XYZB5,
            {
                {0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_POSITION,     0},
                {0, 12, D3DDECLTYPE_FLOAT4,   0, D3DDECLUSAGE_BLENDWEIGHT,  0},
                {0, 28, D3DDECLTYPE_FLOAT1,   0, D3DDECLUSAGE_BLENDINDICES, 0},
                D3DDECL_END(),
            },
        },
        {
            D3DFVF_XYZB5 | D3DFVF_LASTBETA_UBYTE4,
            {
                {0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_POSITION,     0},
                {0, 12, D3DDECLTYPE_FLOAT4,   0, D3DDECLUSAGE_BLENDWEIGHT,  0},
                {0, 28, D3DDECLTYPE_UBYTE4,   0, D3DDECLUSAGE_BLENDINDICES, 0},
                D3DDECL_END(),
            },
        },
        {
            D3DFVF_XYZB5 | D3DFVF_LASTBETA_D3DCOLOR,
            {
                {0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_POSITION,     0},
                {0, 12, D3DDECLTYPE_FLOAT4,   0, D3DDECLUSAGE_BLENDWEIGHT,  0},
                {0, 28, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_BLENDINDICES, 0},
                D3DDECL_END(),
            },
        },
        {
            D3DFVF_XYZB1,
            {
                {0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_POSITION,     0},
                {0, 12, D3DDECLTYPE_FLOAT1,   0, D3DDECLUSAGE_BLENDWEIGHT,  0},
                D3DDECL_END(),
            },
        },
        {
            D3DFVF_XYZB1 | D3DFVF_LASTBETA_UBYTE4,
            {
                {0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_POSITION,     0},
                {0, 12, D3DDECLTYPE_UBYTE4,   0, D3DDECLUSAGE_BLENDINDICES, 0},
                D3DDECL_END(),
            },
        },
        {
            D3DFVF_XYZB1 | D3DFVF_LASTBETA_D3DCOLOR,
            {
                {0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_POSITION,     0},
                {0, 12, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_BLENDINDICES, 0},
                D3DDECL_END(),
            },
        },
        {
            D3DFVF_XYZB2,
            {
                {0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_POSITION,     0},
                {0, 12, D3DDECLTYPE_FLOAT2,   0, D3DDECLUSAGE_BLENDWEIGHT,  0},
                D3DDECL_END(),
            },
        },
        {
            D3DFVF_XYZB2 | D3DFVF_LASTBETA_UBYTE4,
            {
                {0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_POSITION,     0},
                {0, 12, D3DDECLTYPE_FLOAT1,   0, D3DDECLUSAGE_BLENDWEIGHT,  0},
                {0, 16, D3DDECLTYPE_UBYTE4,   0, D3DDECLUSAGE_BLENDINDICES, 0},
                D3DDECL_END(),
            },
        },
        {
            D3DFVF_XYZB2 | D3DFVF_LASTBETA_D3DCOLOR,
            {
                {0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_POSITION,     0},
                {0, 12, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_BLENDWEIGHT,  0},
                {0, 16, D3DDECLTYPE_UBYTE4,   0, D3DDECLUSAGE_BLENDINDICES, 0},
                D3DDECL_END(),
            },
        },
        {
            D3DFVF_XYZB3,
            {
                {0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_POSITION,     0},
                {0, 12, D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_BLENDWEIGHT,  0},
                D3DDECL_END(),
            },
        },
        {
            D3DFVF_XYZB3 | D3DFVF_LASTBETA_UBYTE4,
            {
                {0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_POSITION,     0},
                {0, 12, D3DDECLTYPE_FLOAT2,   0, D3DDECLUSAGE_BLENDWEIGHT,  0},
                {0, 20, D3DDECLTYPE_UBYTE4,   0, D3DDECLUSAGE_BLENDINDICES, 0},
                D3DDECL_END(),
            },
        },
        {
            D3DFVF_XYZB3 | D3DFVF_LASTBETA_D3DCOLOR,
            {
                {0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_POSITION,     0},
                {0, 12, D3DDECLTYPE_FLOAT2,   0, D3DDECLUSAGE_BLENDWEIGHT,  0},
                {0, 20, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_BLENDINDICES, 0},
                D3DDECL_END(),
            },
        },
        {
            D3DFVF_XYZB4,
            {
                {0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_POSITION,     0},
                {0, 12, D3DDECLTYPE_FLOAT4,   0, D3DDECLUSAGE_BLENDWEIGHT,  0},
                D3DDECL_END(),
            },
        },
        {
            D3DFVF_XYZB4 | D3DFVF_LASTBETA_UBYTE4,
            {
                {0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_POSITION,     0},
                {0, 12, D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_BLENDWEIGHT,  0},
                {0, 24, D3DDECLTYPE_UBYTE4,   0, D3DDECLUSAGE_BLENDINDICES, 0},
                D3DDECL_END(),
            },
        },
        {
            D3DFVF_XYZB4 | D3DFVF_LASTBETA_D3DCOLOR,
            {
                {0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_POSITION,     0},
                {0, 12, D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_BLENDWEIGHT,  0},
                {0, 24, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_BLENDINDICES, 0},
                D3DDECL_END(),
            },
        },
        {D3DFVF_NORMAL,   {{0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_NORMAL,       0}, D3DDECL_END()}},
        {D3DFVF_PSIZE,    {{0, 0,  D3DDECLTYPE_FLOAT1,   0, D3DDECLUSAGE_PSIZE,        0}, D3DDECL_END()}},
        {D3DFVF_DIFFUSE,  {{0, 0,  D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR,        0}, D3DDECL_END()}},
        {D3DFVF_SPECULAR, {{0, 0,  D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR,        1}, D3DDECL_END()}},
        /* Make sure textures of different sizes work. */
        {
            D3DFVF_TEXCOORDSIZE1(0) | D3DFVF_TEX1,
            {{0, 0,  D3DDECLTYPE_FLOAT1,   0, D3DDECLUSAGE_TEXCOORD,     0}, D3DDECL_END()},
        },
        {
            D3DFVF_TEXCOORDSIZE2(0) | D3DFVF_TEX1,
            {{0, 0,  D3DDECLTYPE_FLOAT2,   0, D3DDECLUSAGE_TEXCOORD,     0}, D3DDECL_END()},
        },
        {
            D3DFVF_TEXCOORDSIZE3(0) | D3DFVF_TEX1,
            {{0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_TEXCOORD,     0}, D3DDECL_END()},
        },
        {
            D3DFVF_TEXCOORDSIZE4(0) | D3DFVF_TEX1,
            {{0, 0,  D3DDECLTYPE_FLOAT4,   0, D3DDECLUSAGE_TEXCOORD,     0}, D3DDECL_END()},
        },
        /* Make sure the TEXCOORD index works correctly - try several textures. */
        {
            D3DFVF_TEXCOORDSIZE1(0) | D3DFVF_TEXCOORDSIZE3(1) | D3DFVF_TEXCOORDSIZE2(2)
                    | D3DFVF_TEXCOORDSIZE4(3) | D3DFVF_TEX4,
            {
                {0, 0,  D3DDECLTYPE_FLOAT1,   0, D3DDECLUSAGE_TEXCOORD,     0},
                {0, 4,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_TEXCOORD,     1},
                {0, 16, D3DDECLTYPE_FLOAT2,   0, D3DDECLUSAGE_TEXCOORD,     2},
                {0, 24, D3DDECLTYPE_FLOAT4,   0, D3DDECLUSAGE_TEXCOORD,     3},
                D3DDECL_END(),
            },
        },
        /* Now try a combination test. */
        {
            D3DFVF_XYZB4 | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEXCOORDSIZE2(0)
                    | D3DFVF_TEXCOORDSIZE3(1) | D3DFVF_TEX2,
            {
                {0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_POSITION,     0},
                {0, 12, D3DDECLTYPE_FLOAT4,   0, D3DDECLUSAGE_BLENDWEIGHT,  0},
                {0, 28, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR,        0},
                {0, 32, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR,        1},
                {0, 36, D3DDECLTYPE_FLOAT2,   0, D3DDECLUSAGE_TEXCOORD,     0},
                {0, 44, D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_TEXCOORD,     1},
                D3DDECL_END(),
            },
        },
    };

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    for (i = 0; i < ARRAY_SIZE(decl_to_fvf_tests); ++i)
    {
        DWORD fvf = 0xdeadbeef;
        HRESULT hr;

        /* Set a default FVF of SPECULAR and DIFFUSE to make sure it is changed
         * back to 0. */
        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_DIFFUSE | D3DFVF_SPECULAR);
        ok(SUCCEEDED(hr), "Test %u: Failed to set FVF, hr %#x.\n", i, hr);

        hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_to_fvf_tests[i].elements, &declaration);
        ok(SUCCEEDED(hr), "Test %u: Failed to create vertex declaration, hr %#x.\n", i, hr);
        hr = IDirect3DDevice9_SetVertexDeclaration(device, declaration);
        ok(SUCCEEDED(hr), "Test %u: Failed to set vertex declaration, hr %#x.\n", i, hr);

        /* Check the FVF. */
        hr = IDirect3DDevice9_GetFVF(device, &fvf);
        ok(SUCCEEDED(hr), "Test %u: Failed to get FVF, hr %#x.\n", i, hr);

        todo_wine_if (decl_to_fvf_tests[i].todo)
            ok(fvf == decl_to_fvf_tests[i].fvf,
                    "Test %u: Got unexpected FVF %#x, expected %#x.\n",
                    i, fvf, decl_to_fvf_tests[i].fvf);

        IDirect3DDevice9_SetVertexDeclaration(device, NULL);
        IDirect3DVertexDeclaration9_Release(declaration);
    }

    /* Create a default declaration and FVF that does not match any of the
     * tests. */
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, default_elements, &default_decl);
    ok(SUCCEEDED(hr), "Failed to create vertex declaration, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(fvf_to_decl_tests); ++i)
    {
        /* Set a default declaration to make sure it is changed. */
        hr = IDirect3DDevice9_SetVertexDeclaration(device, default_decl);
        ok(SUCCEEDED(hr), "Test %u: Failed to set vertex declaration, hr %#x.\n", i, hr);

        hr = IDirect3DDevice9_SetFVF(device, fvf_to_decl_tests[i].fvf);
        ok(SUCCEEDED(hr), "Test %u: Failed to set FVF, hr %#x.\n", i, hr);

        hr = IDirect3DDevice9_GetVertexDeclaration(device, &declaration);
        ok(SUCCEEDED(hr), "Test %u: Failed to get vertex declaration, hr %#x.\n", i, hr);
        ok(!!declaration && declaration != default_decl,
                "Test %u: Got unexpected declaration %p.\n", i, declaration);
        ok(compare_elements(declaration, fvf_to_decl_tests[i].elements),
                "Test %u: Declaration does not match.\n", i);
        IDirect3DVertexDeclaration9_Release(declaration);
    }

    /* Setting the FVF to 0 should result in no change to the default decl. */
    hr = IDirect3DDevice9_SetVertexDeclaration(device, default_decl);
    ok(SUCCEEDED(hr), "Failed to set vertex declaration, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, 0);
    ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetVertexDeclaration(device, &declaration);
    ok(SUCCEEDED(hr), "Failed to get vertex declaration, hr %#x.\n", hr);
    ok(declaration == default_decl, "Got unexpected declaration %p, expected %p.\n", declaration, default_decl);
    IDirect3DVertexDeclaration9_Release(declaration);

    IDirect3DVertexDeclaration9_Release(default_decl);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

/* Check whether a declaration converted from FVF is shared.
 * Check whether refcounts behave as expected. */
static void test_fvf_decl_management(void)
{
    IDirect3DVertexDeclaration9 *declaration1;
    IDirect3DVertexDeclaration9 *declaration2;
    IDirect3DVertexDeclaration9 *declaration3;
    IDirect3DVertexDeclaration9 *declaration4;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    static const D3DVERTEXELEMENT9 test_elements1[] =
            {{0, 0, D3DDECLTYPE_FLOAT4, 0, D3DDECLUSAGE_POSITIONT, 0}, D3DDECL_END()};
    static const D3DVERTEXELEMENT9 test_elements2[] =
            {{0, 0, D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_NORMAL,    0}, D3DDECL_END()};

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    /* Clear down any current vertex declaration. */
    hr = IDirect3DDevice9_SetVertexDeclaration(device, NULL);
    ok(SUCCEEDED(hr), "Failed to set vertex declaration, hr %#x.\n", hr);
    /* Conversion. */
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZRHW);
    ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);
    /* Get converted decl (#1). */
    hr = IDirect3DDevice9_GetVertexDeclaration(device, &declaration1);
    ok(SUCCEEDED(hr), "Failed to get vertex declaration, hr %#x.\n", hr);
    ok(compare_elements(declaration1, test_elements1), "Declaration does not match.\n");
    /* Get converted decl again (#2). */
    hr = IDirect3DDevice9_GetVertexDeclaration(device, &declaration2);
    ok(SUCCEEDED(hr), "Failed to get vertex declaration, hr %#x.\n", hr);
    ok(declaration2 == declaration1, "Got unexpected declaration2 %p, expected %p.\n", declaration2, declaration1);

    /* Conversion. */
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_NORMAL);
    ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);
    /* Get converted decl (#3). */
    hr = IDirect3DDevice9_GetVertexDeclaration(device, &declaration3);
    ok(SUCCEEDED(hr), "Failed to get vertex declaration, hr %#x.\n", hr);
    ok(declaration3 != declaration2, "Got unexpected declaration3 %p.\n", declaration3);
    /* The contents should correspond to the second conversion. */
    ok(compare_elements(declaration3, test_elements2), "Declaration does not match.\n");
    /* Re-Check if the first decl was overwritten by the new Get(). */
    ok(compare_elements(declaration1, test_elements1), "Declaration does not match.\n");

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZRHW);
    ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetVertexDeclaration(device, &declaration4);
    ok(SUCCEEDED(hr), "Failed to get vertex declaration, hr %#x.\n", hr);
    ok(declaration4 == declaration1, "Got unexpected declaration4 %p, expected %p.\n", declaration4, declaration1);

    refcount = get_refcount((IUnknown*)declaration1);
    ok(refcount == 3, "Got unexpected refcount %u.\n", refcount);
    refcount = get_refcount((IUnknown*)declaration2);
    ok(refcount == 3, "Got unexpected refcount %u.\n", refcount);
    refcount = get_refcount((IUnknown*)declaration3);
    ok(refcount == 1, "Got unexpected refcount %u.\n", refcount);
    refcount = get_refcount((IUnknown*)declaration4);
    ok(refcount == 3, "Got unexpected refcount %u.\n", refcount);

    IDirect3DVertexDeclaration9_Release(declaration4);
    IDirect3DVertexDeclaration9_Release(declaration3);
    IDirect3DVertexDeclaration9_Release(declaration2);
    IDirect3DVertexDeclaration9_Release(declaration1);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_vertex_declaration_alignment(void)
{
    IDirect3DVertexDeclaration9 *declaration;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    unsigned int i;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    static const struct
    {
        D3DVERTEXELEMENT9 elements[3];
        HRESULT hr;
    }
    test_data[] =
    {
        {
            {
                {0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_POSITION, 0},
                {0, 16, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR,    0},
                D3DDECL_END(),
            }, D3D_OK,
        },
        {
            {
                {0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_POSITION, 0},
                {0, 17, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR,    0},
                D3DDECL_END(),
            }, E_FAIL,
        },
        {
            {
                {0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_POSITION, 0},
                {0, 18, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR,    0},
                D3DDECL_END(),
            }, E_FAIL,
        },
        {
            {
                {0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_POSITION, 0},
                {0, 19, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR,    0},
                D3DDECL_END(),
            }, E_FAIL,
        },
        {
            {
                {0, 0,  D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_POSITION, 0},
                {0, 20, D3DDECLTYPE_D3DCOLOR, 0, D3DDECLUSAGE_COLOR,    0},
                D3DDECL_END(),
            }, D3D_OK,
        },
    };

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    for (i = 0; i < ARRAY_SIZE(test_data); ++i)
    {
        hr = IDirect3DDevice9_CreateVertexDeclaration(device, test_data[i].elements, &declaration);
        ok(hr == test_data[i].hr, "Test %u: Got unexpected hr %#x, expected %#x.\n", i, hr, test_data[i].hr);
        if (SUCCEEDED(hr))
            IDirect3DVertexDeclaration9_Release(declaration);
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_unused_declaration_type(void)
{
    IDirect3DVertexDeclaration9 *declaration;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    unsigned int i;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    static const D3DVERTEXELEMENT9 test_elements[][3] =
    {
        {
            {0, 0,  D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0 },
            {0, 16, D3DDECLTYPE_UNUSED, 0, D3DDECLUSAGE_COLOR   , 0 },
            D3DDECL_END(),
        },
        {
            {0, 0,  D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0 },
            {0, 16, D3DDECLTYPE_UNUSED, 0, D3DDECLUSAGE_TEXCOORD, 0 },
            D3DDECL_END(),
        },
        {
            {0, 0,  D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0 },
            {0, 16, D3DDECLTYPE_UNUSED, 0, D3DDECLUSAGE_TEXCOORD, 1 },
            D3DDECL_END(),
        },
        {
            {0, 0,  D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0 },
            {0, 16, D3DDECLTYPE_UNUSED, 0, D3DDECLUSAGE_TEXCOORD, 12},
            D3DDECL_END(),
        },
        {
            {0, 0,  D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0 },
            {1, 16, D3DDECLTYPE_UNUSED, 0, D3DDECLUSAGE_TEXCOORD, 12},
            D3DDECL_END(),
        },
        {
            {0, 0,  D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0 },
            {0, 16, D3DDECLTYPE_UNUSED, 0, D3DDECLUSAGE_NORMAL,   0 },
            D3DDECL_END(),
        },
        {
            {0, 0,  D3DDECLTYPE_FLOAT3, 0, D3DDECLUSAGE_POSITION, 0 },
            {1, 16, D3DDECLTYPE_UNUSED, 0, D3DDECLUSAGE_NORMAL,   0 },
            D3DDECL_END(),
        },
    };

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    for (i = 0; i < ARRAY_SIZE(test_elements); ++i)
    {
        hr = IDirect3DDevice9_CreateVertexDeclaration(device, test_elements[i], &declaration);
        ok(hr == E_FAIL, "Test %u: Got unexpected hr %#x.\n", i, hr);
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void check_mipmap_levels(IDirect3DDevice9 *device, UINT width, UINT height, UINT count)
{
    IDirect3DBaseTexture9* texture = NULL;
    HRESULT hr = IDirect3DDevice9_CreateTexture( device, width, height, 0, 0,
            D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, (IDirect3DTexture9**) &texture, NULL );

    if (SUCCEEDED(hr)) {
        DWORD levels = IDirect3DBaseTexture9_GetLevelCount(texture);
        ok(levels == count, "Invalid level count. Expected %d got %u\n", count, levels);
    } else
        trace("CreateTexture failed: %08x\n", hr);

    if (texture) IDirect3DBaseTexture9_Release( texture );
}

static void test_mipmap_levels(void)
{
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;

    window = create_window();
    ok(!!window, "Failed to create a window.\n");
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        goto cleanup;
    }

    check_mipmap_levels(device, 32, 32, 6);
    check_mipmap_levels(device, 256, 1, 9);
    check_mipmap_levels(device, 1, 256, 9);
    check_mipmap_levels(device, 1, 1, 1);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
cleanup:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_checkdevicemultisampletype(void)
{
    DWORD quality_levels;
    IDirect3D9 *d3d;
    HWND window;
    HRESULT hr;

    window = create_window();
    ok(!!window, "Failed to create a window.\n");
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    if (IDirect3D9_CheckDeviceMultiSampleType(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, TRUE, D3DMULTISAMPLE_2_SAMPLES, NULL) == D3DERR_NOTAVAILABLE)
    {
        skip("Multisampling not supported for D3DFMT_X8R8G8B8, skipping test.\n");
        goto cleanup;
    }

    hr = IDirect3D9_CheckDeviceMultiSampleType(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_UNKNOWN, TRUE, D3DMULTISAMPLE_NONE, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3D9_CheckDeviceMultiSampleType(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            65536, TRUE, D3DMULTISAMPLE_NONE, NULL);
    todo_wine ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);

    hr = IDirect3D9_CheckDeviceMultiSampleType(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, TRUE, D3DMULTISAMPLE_NONE, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);

    quality_levels = 0;
    hr = IDirect3D9_CheckDeviceMultiSampleType(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, TRUE, D3DMULTISAMPLE_NONE, &quality_levels);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    ok(quality_levels == 1, "Got unexpected quality_levels %u.\n", quality_levels);
    quality_levels = 0;
    hr = IDirect3D9_CheckDeviceMultiSampleType(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, FALSE, D3DMULTISAMPLE_NONE, &quality_levels);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    ok(quality_levels == 1, "Got unexpected quality_levels %u.\n", quality_levels);

    quality_levels = 0;
    hr = IDirect3D9_CheckDeviceMultiSampleType(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, TRUE, D3DMULTISAMPLE_NONMASKABLE, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    hr = IDirect3D9_CheckDeviceMultiSampleType(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, TRUE, D3DMULTISAMPLE_NONMASKABLE, &quality_levels);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    ok(quality_levels, "Got unexpected quality_levels %u.\n", quality_levels);

    quality_levels = 0;
    hr = IDirect3D9_CheckDeviceMultiSampleType(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, TRUE, D3DMULTISAMPLE_2_SAMPLES, NULL);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    hr = IDirect3D9_CheckDeviceMultiSampleType(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, TRUE, D3DMULTISAMPLE_2_SAMPLES, &quality_levels);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
    ok(quality_levels, "Got unexpected quality_levels %u.\n", quality_levels);

    /* We assume D3DMULTISAMPLE_15_SAMPLES is never supported in practice. */
    quality_levels = 0;
    hr = IDirect3D9_CheckDeviceMultiSampleType(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, TRUE, D3DMULTISAMPLE_15_SAMPLES, NULL);
    ok(hr == D3DERR_NOTAVAILABLE, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3D9_CheckDeviceMultiSampleType(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, TRUE, D3DMULTISAMPLE_15_SAMPLES, &quality_levels);
    ok(hr == D3DERR_NOTAVAILABLE, "Got unexpected hr %#x.\n", hr);
    ok(quality_levels == 1, "Got unexpected quality_levels %u.\n", quality_levels);

    hr = IDirect3D9_CheckDeviceMultiSampleType(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, TRUE, 65536, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3D9_CheckDeviceMultiSampleType(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_DXT5, TRUE, D3DMULTISAMPLE_2_SAMPLES, NULL);
    ok(hr == D3DERR_NOTAVAILABLE, "Got unexpected hr %#x.\n", hr);

cleanup:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_invalid_multisample(void)
{
    IDirect3DDevice9 *device;
    IDirect3DSurface9 *rt;
    DWORD quality_levels;
    IDirect3D9 *d3d;
    BOOL available;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = create_window();
    ok(!!window, "Failed to create a window.\n");
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        goto cleanup;
    }

    available = SUCCEEDED(IDirect3D9_CheckDeviceMultiSampleType(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                    D3DFMT_X8R8G8B8, TRUE, D3DMULTISAMPLE_NONMASKABLE, &quality_levels));
    hr = IDirect3DDevice9_CreateRenderTarget(device, 128, 128,
            D3DFMT_X8R8G8B8, D3DMULTISAMPLE_NONMASKABLE, 0, FALSE, &rt, NULL);
    if (available)
    {
        ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
        IDirect3DSurface9_Release(rt);
        hr = IDirect3DDevice9_CreateRenderTarget(device, 128, 128,
                D3DFMT_X8R8G8B8, D3DMULTISAMPLE_NONMASKABLE, quality_levels, FALSE, &rt, NULL);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    }
    else
    {
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    }

    available = SUCCEEDED(IDirect3D9_CheckDeviceMultiSampleType(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, TRUE, D3DMULTISAMPLE_2_SAMPLES, &quality_levels));
    hr = IDirect3DDevice9_CreateRenderTarget(device, 128, 128,
            D3DFMT_X8R8G8B8, D3DMULTISAMPLE_2_SAMPLES, 0, FALSE, &rt, NULL);
    if (available)
    {
        ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);
        IDirect3DSurface9_Release(rt);
        hr = IDirect3DDevice9_CreateRenderTarget(device, 128, 128,
                D3DFMT_X8R8G8B8, D3DMULTISAMPLE_2_SAMPLES, quality_levels, FALSE, &rt, NULL);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    }
    else
    {
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    }

    /* We assume D3DMULTISAMPLE_15_SAMPLES is never supported in practice. */
    hr = IDirect3D9_CheckDeviceMultiSampleType(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, TRUE, D3DMULTISAMPLE_15_SAMPLES, NULL);
    ok(hr == D3DERR_NOTAVAILABLE, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateRenderTarget(device, 128, 128,
            D3DFMT_X8R8G8B8, D3DMULTISAMPLE_15_SAMPLES, 0, FALSE, &rt, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
cleanup:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_swapchain(void)
{
    IDirect3DSwapChain9 *swapchain0;
    IDirect3DSwapChain9 *swapchain1;
    IDirect3DSwapChain9 *swapchain2;
    IDirect3DSwapChain9 *swapchain3;
    IDirect3DSwapChain9 *swapchainX;
    IDirect3DSurface9 *backbuffer, *stereo_buffer;
    D3DPRESENT_PARAMETERS d3dpp;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window, window2;
    HRESULT hr;
    struct device_desc device_desc;

    window = create_window();
    ok(!!window, "Failed to create a window.\n");
    window2 = create_window();
    ok(!!window2, "Failed to create a window.\n");
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        goto cleanup;
    }

    /* Get the implicit swapchain */
    hr = IDirect3DDevice9_GetSwapChain(device, 0, &swapchain0);
    ok(SUCCEEDED(hr), "Failed to get the implicit swapchain (%08x)\n", hr);
    /* Check if the back buffer count was modified */
    hr = IDirect3DSwapChain9_GetPresentParameters(swapchain0, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to get present parameters, hr %#x.\n", hr);
    ok(d3dpp.BackBufferCount == 1, "Got unexpected back buffer count %u.\n", d3dpp.BackBufferCount);
    IDirect3DSwapChain9_Release(swapchain0);

    hr = IDirect3DSwapChain9_GetBackBuffer(swapchain0, 0, D3DBACKBUFFER_TYPE_MONO, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    /* IDirect3DDevice9::GetBackBuffer crashes if a NULL output pointer is passed. */
    backbuffer = (void *)0xdeadbeef;
    hr = IDirect3DDevice9_GetBackBuffer(device, 1, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    ok(!backbuffer, "The back buffer pointer is %p, expected NULL.\n", backbuffer);
    backbuffer = (void *)0xdeadbeef;
    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 1, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    ok(!backbuffer, "The back buffer pointer is %p, expected NULL.\n", backbuffer);

    /* Check if there is a back buffer */
    hr = IDirect3DSwapChain9_GetBackBuffer(swapchain0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(SUCCEEDED(hr), "Failed to get the back buffer (%08x)\n", hr);
    ok(backbuffer != NULL, "The back buffer is NULL\n");
    if(backbuffer) IDirect3DSurface9_Release(backbuffer);

    /* The back buffer type value is ignored. */
    hr = IDirect3DSwapChain9_GetBackBuffer(swapchain0, 0, D3DBACKBUFFER_TYPE_LEFT, &stereo_buffer);
    ok(SUCCEEDED(hr), "Failed to get the back buffer, hr %#x.\n", hr);
    ok(stereo_buffer == backbuffer, "Expected left back buffer = %p, got %p.\n", backbuffer, stereo_buffer);
    IDirect3DSurface9_Release(stereo_buffer);
    hr = IDirect3DSwapChain9_GetBackBuffer(swapchain0, 0, D3DBACKBUFFER_TYPE_RIGHT, &stereo_buffer);
    ok(SUCCEEDED(hr), "Failed to get the back buffer, hr %#x.\n", hr);
    ok(stereo_buffer == backbuffer, "Expected right back buffer = %p, got %p.\n", backbuffer, stereo_buffer);
    IDirect3DSurface9_Release(stereo_buffer);
    hr = IDirect3DSwapChain9_GetBackBuffer(swapchain0, 0, (D3DBACKBUFFER_TYPE)0xdeadbeef, &stereo_buffer);
    ok(SUCCEEDED(hr), "Failed to get the back buffer, hr %#x.\n", hr);
    ok(stereo_buffer == backbuffer, "Expected unknown buffer = %p, got %p.\n", backbuffer, stereo_buffer);
    IDirect3DSurface9_Release(stereo_buffer);

    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_LEFT, &stereo_buffer);
    ok(SUCCEEDED(hr), "Failed to get the back buffer, hr %#x.\n", hr);
    ok(stereo_buffer == backbuffer, "Expected left back buffer = %p, got %p.\n", backbuffer, stereo_buffer);
    IDirect3DSurface9_Release(stereo_buffer);
    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_RIGHT, &stereo_buffer);
    ok(SUCCEEDED(hr), "Failed to get the back buffer, hr %#x.\n", hr);
    ok(stereo_buffer == backbuffer, "Expected right back buffer = %p, got %p.\n", backbuffer, stereo_buffer);
    IDirect3DSurface9_Release(stereo_buffer);
    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, (D3DBACKBUFFER_TYPE)0xdeadbeef, &stereo_buffer);
    ok(SUCCEEDED(hr), "Failed to get the back buffer, hr %#x.\n", hr);
    ok(stereo_buffer == backbuffer, "Expected unknown buffer = %p, got %p.\n", backbuffer, stereo_buffer);
    IDirect3DSurface9_Release(stereo_buffer);

    /* Try to get a nonexistent swapchain */
    hr = IDirect3DDevice9_GetSwapChain(device, 1, &swapchainX);
    ok(hr == D3DERR_INVALIDCALL, "GetSwapChain on an nonexistent swapchain returned (%08x)\n", hr);
    ok(swapchainX == NULL, "Swapchain 1 is %p\n", swapchainX);
    if(swapchainX) IDirect3DSwapChain9_Release(swapchainX);

    /* Create a bunch of swapchains */
    d3dpp.BackBufferCount = 0;
    hr = IDirect3DDevice9_CreateAdditionalSwapChain(device, &d3dpp, &swapchain1);
    ok(SUCCEEDED(hr), "Failed to create a swapchain (%08x)\n", hr);
    ok(d3dpp.BackBufferCount == 1, "The back buffer count in the presentparams struct is %d\n", d3dpp.BackBufferCount);

    d3dpp.BackBufferCount  = 1;
    hr = IDirect3DDevice9_CreateAdditionalSwapChain(device, &d3dpp, &swapchain2);
    ok(SUCCEEDED(hr), "Failed to create a swapchain (%08x)\n", hr);

    d3dpp.BackBufferCount  = 2;
    hr = IDirect3DDevice9_CreateAdditionalSwapChain(device, &d3dpp, &swapchain3);
    ok(SUCCEEDED(hr), "Failed to create a swapchain (%08x)\n", hr);
    if(SUCCEEDED(hr)) {
        /* Swapchain 3, created with backbuffercount 2 */
        backbuffer = (void *) 0xdeadbeef;
        hr = IDirect3DSwapChain9_GetBackBuffer(swapchain3, 0, 0, &backbuffer);
        ok(SUCCEEDED(hr), "Failed to get the 1st back buffer (%08x)\n", hr);
        ok(backbuffer != NULL && backbuffer != (void *) 0xdeadbeef, "The back buffer is %p\n", backbuffer);
        if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface9_Release(backbuffer);

        backbuffer = (void *) 0xdeadbeef;
        hr = IDirect3DSwapChain9_GetBackBuffer(swapchain3, 1, 0, &backbuffer);
        ok(SUCCEEDED(hr), "Failed to get the 2nd back buffer (%08x)\n", hr);
        ok(backbuffer != NULL && backbuffer != (void *) 0xdeadbeef, "The back buffer is %p\n", backbuffer);
        if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface9_Release(backbuffer);

        backbuffer = (void *) 0xdeadbeef;
        hr = IDirect3DSwapChain9_GetBackBuffer(swapchain3, 2, 0, &backbuffer);
        ok(hr == D3DERR_INVALIDCALL, "GetBackBuffer returned %08x\n", hr);
        ok(backbuffer == (void *) 0xdeadbeef, "The back buffer pointer was modified (%p)\n", backbuffer);
        if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface9_Release(backbuffer);

        backbuffer = (void *) 0xdeadbeef;
        hr = IDirect3DSwapChain9_GetBackBuffer(swapchain3, 3, 0, &backbuffer);
        ok(FAILED(hr), "Failed to get the back buffer (%08x)\n", hr);
        ok(backbuffer == (void *) 0xdeadbeef, "The back buffer pointer was modified (%p)\n", backbuffer);
        if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface9_Release(backbuffer);
    }

    /* Check the back buffers of the swapchains */
    /* Swapchain 1, created with backbuffercount 0 */
    hr = IDirect3DSwapChain9_GetBackBuffer(swapchain1, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(SUCCEEDED(hr), "Failed to get the back buffer (%08x)\n", hr);
    ok(backbuffer != NULL, "The back buffer is NULL (%08x)\n", hr);
    if(backbuffer) IDirect3DSurface9_Release(backbuffer);

    backbuffer = (void *) 0xdeadbeef;
    hr = IDirect3DSwapChain9_GetBackBuffer(swapchain1, 1, 0, &backbuffer);
    ok(FAILED(hr), "Failed to get the back buffer (%08x)\n", hr);
    ok(backbuffer == (void *) 0xdeadbeef, "The back buffer pointer was modified (%p)\n", backbuffer);
    if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface9_Release(backbuffer);

    /* Swapchain 2 - created with backbuffercount 1 */
    backbuffer = (void *) 0xdeadbeef;
    hr = IDirect3DSwapChain9_GetBackBuffer(swapchain2, 0, 0, &backbuffer);
    ok(SUCCEEDED(hr), "Failed to get the back buffer (%08x)\n", hr);
    ok(backbuffer != NULL && backbuffer != (void *) 0xdeadbeef, "The back buffer is %p\n", backbuffer);
    if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface9_Release(backbuffer);

    backbuffer = (void *) 0xdeadbeef;
    hr = IDirect3DSwapChain9_GetBackBuffer(swapchain2, 1, 0, &backbuffer);
    ok(hr == D3DERR_INVALIDCALL, "GetBackBuffer returned %08x\n", hr);
    ok(backbuffer == (void *) 0xdeadbeef, "The back buffer pointer was modified (%p)\n", backbuffer);
    if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface9_Release(backbuffer);

    backbuffer = (void *) 0xdeadbeef;
    hr = IDirect3DSwapChain9_GetBackBuffer(swapchain2, 2, 0, &backbuffer);
    ok(FAILED(hr), "Failed to get the back buffer (%08x)\n", hr);
    ok(backbuffer == (void *) 0xdeadbeef, "The back buffer pointer was modified (%p)\n", backbuffer);
    if(backbuffer && backbuffer != (void *) 0xdeadbeef) IDirect3DSurface9_Release(backbuffer);

    /* Try getSwapChain on a manually created swapchain
     * it should fail, apparently GetSwapChain only returns implicit swapchains
     */
    swapchainX = (void *) 0xdeadbeef;
    hr = IDirect3DDevice9_GetSwapChain(device, 1, &swapchainX);
    ok(hr == D3DERR_INVALIDCALL, "Failed to get the second swapchain (%08x)\n", hr);
    ok(swapchainX == NULL, "The swapchain pointer is %p\n", swapchainX);
    if(swapchainX && swapchainX != (void *) 0xdeadbeef ) IDirect3DSwapChain9_Release(swapchainX);

    IDirect3DSwapChain9_Release(swapchain3);
    IDirect3DSwapChain9_Release(swapchain2);
    IDirect3DSwapChain9_Release(swapchain1);

    d3dpp.Windowed = FALSE;
    d3dpp.hDeviceWindow = window;
    d3dpp.BackBufferCount = 1;
    hr = IDirect3DDevice9_CreateAdditionalSwapChain(device, &d3dpp, &swapchain1);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x\n", hr);
    d3dpp.hDeviceWindow = window2;
    hr = IDirect3DDevice9_CreateAdditionalSwapChain(device, &d3dpp, &swapchain1);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x\n", hr);

    device_desc.width = registry_mode.dmPelsWidth;
    device_desc.height = registry_mode.dmPelsHeight;
    device_desc.device_window = window;
    device_desc.flags = CREATE_DEVICE_FULLSCREEN;
    hr = reset_device(device, &device_desc);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);

    d3dpp.hDeviceWindow = window;
    hr = IDirect3DDevice9_CreateAdditionalSwapChain(device, &d3dpp, &swapchain1);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x\n", hr);
    d3dpp.hDeviceWindow = window2;
    hr = IDirect3DDevice9_CreateAdditionalSwapChain(device, &d3dpp, &swapchain1);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x\n", hr);
    d3dpp.Windowed = TRUE;
    d3dpp.hDeviceWindow = window;
    hr = IDirect3DDevice9_CreateAdditionalSwapChain(device, &d3dpp, &swapchain1);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x\n", hr);
    d3dpp.hDeviceWindow = window2;
    hr = IDirect3DDevice9_CreateAdditionalSwapChain(device, &d3dpp, &swapchain1);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x\n", hr);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
cleanup:
    IDirect3D9_Release(d3d);
    DestroyWindow(window2);
    DestroyWindow(window);
}

static void test_refcount(void)
{
    IDirect3DVertexBuffer9      *pVertexBuffer      = NULL;
    IDirect3DIndexBuffer9       *pIndexBuffer       = NULL;
    IDirect3DVertexDeclaration9 *pVertexDeclaration = NULL;
    IDirect3DVertexShader9      *pVertexShader      = NULL;
    IDirect3DPixelShader9       *pPixelShader       = NULL;
    IDirect3DCubeTexture9       *pCubeTexture       = NULL;
    IDirect3DTexture9           *pTexture           = NULL;
    IDirect3DVolumeTexture9     *pVolumeTexture     = NULL;
    IDirect3DVolume9            *pVolumeLevel       = NULL;
    IDirect3DSurface9           *pStencilSurface    = NULL;
    IDirect3DSurface9           *pOffscreenSurface  = NULL;
    IDirect3DSurface9           *pRenderTarget      = NULL;
    IDirect3DSurface9           *pRenderTarget2     = NULL;
    IDirect3DSurface9           *pRenderTarget3     = NULL;
    IDirect3DSurface9           *pTextureLevel      = NULL;
    IDirect3DSurface9           *pBackBuffer        = NULL;
    IDirect3DStateBlock9        *pStateBlock        = NULL;
    IDirect3DStateBlock9        *pStateBlock1       = NULL;
    IDirect3DSwapChain9         *pSwapChain         = NULL;
    IDirect3DQuery9             *pQuery             = NULL;
    D3DPRESENT_PARAMETERS        d3dpp;
    IDirect3DDevice9 *device;
    ULONG refcount = 0, tmp;
    IDirect3D9 *d3d, *d3d2;
    HWND window;
    HRESULT hr;

    D3DVERTEXELEMENT9 decl[] =
    {
        D3DDECL_END()
    };

    window = create_window();
    ok(!!window, "Failed to create a window.\n");
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    CHECK_REFCOUNT(d3d, 1);

    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        goto cleanup;
    }

    refcount = get_refcount((IUnknown *)device);
    ok(refcount == 1, "Invalid device RefCount %d\n", refcount);

    CHECK_REFCOUNT(d3d, 2);

    hr = IDirect3DDevice9_GetDirect3D(device, &d3d2);
    CHECK_CALL(hr, "GetDirect3D", device, refcount);

    ok(d3d2 == d3d, "Expected IDirect3D9 pointers to be equal.\n");
    CHECK_REFCOUNT(d3d, 3);
    CHECK_RELEASE_REFCOUNT(d3d, 2);

    /**
     * Check refcount of implicit surfaces and implicit swapchain. Findings:
     *   - the container is the device OR swapchain
     *   - they hold a reference to the device
     *   - they are created with a refcount of 0 (Get/Release returns original refcount)
     *   - they are not freed if refcount reaches 0.
     *   - the refcount is not forwarded to the container.
     */
    hr = IDirect3DDevice9_GetSwapChain(device, 0, &pSwapChain);
    CHECK_CALL(hr, "GetSwapChain", device, ++refcount);
    if (pSwapChain)
    {
        CHECK_REFCOUNT(pSwapChain, 1);

        hr = IDirect3DDevice9_GetRenderTarget(device, 0, &pRenderTarget);
        CHECK_CALL(hr, "GetRenderTarget", device, ++refcount);
        CHECK_REFCOUNT(pSwapChain, 1);
        if (pRenderTarget)
        {
            CHECK_SURFACE_CONTAINER(pRenderTarget, IID_IDirect3DSwapChain9, pSwapChain);
            CHECK_REFCOUNT(pRenderTarget, 1);

            CHECK_ADDREF_REFCOUNT(pRenderTarget, 2);
            CHECK_REFCOUNT(device, refcount);
            CHECK_RELEASE_REFCOUNT(pRenderTarget, 1);
            CHECK_REFCOUNT(device, refcount);

            hr = IDirect3DDevice9_GetRenderTarget(device, 0, &pRenderTarget);
            CHECK_CALL(hr, "GetRenderTarget", device, refcount);
            CHECK_REFCOUNT(pRenderTarget, 2);
            CHECK_RELEASE_REFCOUNT(pRenderTarget, 1);
            CHECK_RELEASE_REFCOUNT(pRenderTarget, 0);
            CHECK_REFCOUNT(device, --refcount);

            /* The render target is released with the device, so AddRef with refcount=0 is fine here. */
            CHECK_ADDREF_REFCOUNT(pRenderTarget, 1);
            CHECK_REFCOUNT(device, ++refcount);
            CHECK_RELEASE_REFCOUNT(pRenderTarget, 0);
            CHECK_REFCOUNT(device, --refcount);
            CHECK_RELEASE_REFCOUNT(pRenderTarget, 0);
            CHECK_RELEASE_REFCOUNT(pRenderTarget, 0);
        }

        /* Render target and back buffer are identical. */
        hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, 0, &pBackBuffer);
        CHECK_CALL(hr, "GetBackBuffer", device, ++refcount);
        if (pBackBuffer)
        {
            CHECK_RELEASE_REFCOUNT(pBackBuffer, 0);
            ok(pRenderTarget == pBackBuffer, "RenderTarget=%p and BackBuffer=%p should be the same.\n",
                    pRenderTarget, pBackBuffer);
            CHECK_RELEASE_REFCOUNT(pBackBuffer, 0);
            CHECK_RELEASE_REFCOUNT(pBackBuffer, 0);
            pBackBuffer = NULL;
        }
        CHECK_REFCOUNT(device, --refcount);

        hr = IDirect3DDevice9_GetDepthStencilSurface(device, &pStencilSurface);
        CHECK_CALL(hr, "GetDepthStencilSurface", device, ++refcount);
        CHECK_REFCOUNT(pSwapChain, 1);
        if (pStencilSurface)
        {
            CHECK_SURFACE_CONTAINER(pStencilSurface, IID_IDirect3DDevice9, device);
            CHECK_REFCOUNT( pStencilSurface, 1);

            CHECK_ADDREF_REFCOUNT(pStencilSurface, 2);
            CHECK_REFCOUNT(device, refcount);
            CHECK_RELEASE_REFCOUNT(pStencilSurface, 1);
            CHECK_REFCOUNT(device, refcount);

            CHECK_RELEASE_REFCOUNT(pStencilSurface, 0);
            CHECK_REFCOUNT(device, --refcount);

            /* The stencil surface is released with the device, so AddRef with refcount=0 is fine here. */
            CHECK_ADDREF_REFCOUNT(pStencilSurface, 1);
            CHECK_REFCOUNT(device, ++refcount);
            CHECK_RELEASE_REFCOUNT(pStencilSurface, 0);
            CHECK_REFCOUNT(device, --refcount);
            CHECK_RELEASE_REFCOUNT(pStencilSurface, 0);
            CHECK_RELEASE_REFCOUNT(pStencilSurface, 0);
            pStencilSurface = NULL;
        }

        CHECK_RELEASE_REFCOUNT(pSwapChain, 0);
        CHECK_REFCOUNT(device, --refcount);
        CHECK_RELEASE_REFCOUNT(pSwapChain, 0);
        CHECK_RELEASE_REFCOUNT(pSwapChain, 0);

        /* The implicit swapchwin is released with the device, so AddRef with refcount=0 is fine here. */
        CHECK_ADDREF_REFCOUNT(pSwapChain, 1);
        CHECK_REFCOUNT(device, ++refcount);
        CHECK_RELEASE_REFCOUNT(pSwapChain, 0);
        CHECK_REFCOUNT(device, --refcount);
        CHECK_RELEASE_REFCOUNT(pSwapChain, 0);
        CHECK_RELEASE_REFCOUNT(pSwapChain, 0);
        pSwapChain = NULL;
    }

    /* Buffers */
    hr = IDirect3DDevice9_CreateIndexBuffer(device, 16, 0, D3DFMT_INDEX32, D3DPOOL_DEFAULT, &pIndexBuffer, NULL);
    CHECK_CALL(hr, "CreateIndexBuffer", device, ++refcount );
    if(pIndexBuffer)
    {
        tmp = get_refcount((IUnknown *)pIndexBuffer);

        hr = IDirect3DDevice9_SetIndices(device, pIndexBuffer);
        CHECK_CALL( hr, "SetIndices", pIndexBuffer, tmp);
        hr = IDirect3DDevice9_SetIndices(device, NULL);
        CHECK_CALL( hr, "SetIndices", pIndexBuffer, tmp);
    }

    hr = IDirect3DDevice9_CreateVertexBuffer(device, 16, 0, D3DFVF_XYZ, D3DPOOL_DEFAULT, &pVertexBuffer, NULL);
    CHECK_CALL(hr, "CreateVertexBuffer", device, ++refcount);
    if(pVertexBuffer)
    {
        IDirect3DVertexBuffer9 *pVBuf = (void*)~0;
        UINT offset = ~0;
        UINT stride = ~0;

        tmp = get_refcount( (IUnknown *)pVertexBuffer );

        hr = IDirect3DDevice9_SetStreamSource(device, 0, pVertexBuffer, 0, 3 * sizeof(float));
        CHECK_CALL( hr, "SetStreamSource", pVertexBuffer, tmp);
        hr = IDirect3DDevice9_SetStreamSource(device, 0, NULL, 0, 0);
        CHECK_CALL( hr, "SetStreamSource", pVertexBuffer, tmp);

        hr = IDirect3DDevice9_GetStreamSource(device, 0, &pVBuf, &offset, &stride);
        ok(SUCCEEDED(hr), "GetStreamSource did not succeed with NULL stream!\n");
        ok(pVBuf==NULL, "pVBuf not NULL (%p)!\n", pVBuf);
        ok(stride==3*sizeof(float), "stride not 3 floats (got %u)!\n", stride);
        ok(offset==0, "offset not 0 (got %u)!\n", offset);
    }
    /* Shaders */
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl, &pVertexDeclaration);
    CHECK_CALL(hr, "CreateVertexDeclaration", device, ++refcount);
    hr = IDirect3DDevice9_CreateVertexShader(device, simple_vs, &pVertexShader);
    CHECK_CALL(hr, "CreateVertexShader", device, ++refcount);
    hr = IDirect3DDevice9_CreatePixelShader(device, simple_ps, &pPixelShader);
    CHECK_CALL(hr, "CreatePixelShader", device, ++refcount);
    /* Textures */
    hr = IDirect3DDevice9_CreateTexture(device, 32, 32, 3, 0, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &pTexture, NULL);
    CHECK_CALL(hr, "CreateTexture", device, ++refcount);
    if (pTexture)
    {
        tmp = get_refcount( (IUnknown *)pTexture );

        /* SetTexture should not increase refcounts */
        hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *)pTexture);
        CHECK_CALL(hr, "SetTexture", pTexture, tmp);
        hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
        CHECK_CALL(hr, "SetTexture", pTexture, tmp);

        /* This should not increment device refcount */
        hr = IDirect3DTexture9_GetSurfaceLevel( pTexture, 1, &pTextureLevel );
        CHECK_CALL(hr, "GetSurfaceLevel", device, refcount);
        /* But should increment texture's refcount */
        CHECK_REFCOUNT( pTexture, tmp+1 );
        /* Because the texture and surface refcount are identical */
        if (pTextureLevel)
        {
            CHECK_REFCOUNT        ( pTextureLevel, tmp+1 );
            CHECK_ADDREF_REFCOUNT ( pTextureLevel, tmp+2 );
            CHECK_REFCOUNT        ( pTexture     , tmp+2 );
            CHECK_RELEASE_REFCOUNT( pTextureLevel, tmp+1 );
            CHECK_REFCOUNT        ( pTexture     , tmp+1 );
            CHECK_RELEASE_REFCOUNT( pTexture     , tmp   );
            CHECK_REFCOUNT        ( pTextureLevel, tmp   );
        }
    }
    hr = IDirect3DDevice9_CreateCubeTexture(device, 32, 0, 0, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &pCubeTexture, NULL);
    CHECK_CALL(hr, "CreateCubeTexture", device, ++refcount);
    hr = IDirect3DDevice9_CreateVolumeTexture(device, 32, 32, 2, 0, 0,
            D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &pVolumeTexture, NULL);
    CHECK_CALL(hr, "CreateVolumeTexture", device, ++refcount);
    if (pVolumeTexture)
    {
        tmp = get_refcount( (IUnknown *)pVolumeTexture );

        /* This should not increment device refcount */
        hr = IDirect3DVolumeTexture9_GetVolumeLevel(pVolumeTexture, 0, &pVolumeLevel);
        CHECK_CALL(hr, "GetVolumeLevel", device, refcount);
        /* But should increment volume texture's refcount */
        CHECK_REFCOUNT( pVolumeTexture, tmp+1 );
        /* Because the volume texture and volume refcount are identical */
        if (pVolumeLevel)
        {
            CHECK_REFCOUNT        ( pVolumeLevel  , tmp+1 );
            CHECK_ADDREF_REFCOUNT ( pVolumeLevel  , tmp+2 );
            CHECK_REFCOUNT        ( pVolumeTexture, tmp+2 );
            CHECK_RELEASE_REFCOUNT( pVolumeLevel  , tmp+1 );
            CHECK_REFCOUNT        ( pVolumeTexture, tmp+1 );
            CHECK_RELEASE_REFCOUNT( pVolumeTexture, tmp   );
            CHECK_REFCOUNT        ( pVolumeLevel  , tmp   );
        }
    }
    /* Surfaces */
    hr = IDirect3DDevice9_CreateDepthStencilSurface(device, 32, 32,
            D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, TRUE, &pStencilSurface, NULL);
    CHECK_CALL(hr, "CreateDepthStencilSurface", device, ++refcount);
    CHECK_REFCOUNT( pStencilSurface, 1 );
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 32, 32,
            D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &pOffscreenSurface, NULL);
    CHECK_CALL(hr, "CreateOffscreenPlainSurface", device, ++refcount);
    CHECK_REFCOUNT( pOffscreenSurface, 1 );
    hr = IDirect3DDevice9_CreateRenderTarget(device, 32, 32,
            D3DFMT_X8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &pRenderTarget3, NULL);
    CHECK_CALL(hr, "CreateRenderTarget", device, ++refcount);
    CHECK_REFCOUNT( pRenderTarget3, 1 );
    /* Misc */
    hr = IDirect3DDevice9_CreateStateBlock(device, D3DSBT_ALL, &pStateBlock);
    CHECK_CALL(hr, "CreateStateBlock", device, ++refcount);

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    hr = IDirect3DDevice9_CreateAdditionalSwapChain(device, &d3dpp, &pSwapChain);
    CHECK_CALL(hr, "CreateAdditionalSwapChain", device, ++refcount);
    if (pSwapChain)
    {
        /* check implicit back buffer */
        hr = IDirect3DSwapChain9_GetBackBuffer(pSwapChain, 0, 0, &pBackBuffer);
        CHECK_CALL(hr, "GetBackBuffer", device, ++refcount);
        CHECK_REFCOUNT(pSwapChain, 1);
        if (pBackBuffer)
        {
            CHECK_SURFACE_CONTAINER(pBackBuffer, IID_IDirect3DSwapChain9, pSwapChain);
            CHECK_REFCOUNT(pBackBuffer, 1);
            CHECK_RELEASE_REFCOUNT(pBackBuffer, 0);
            CHECK_REFCOUNT(device, --refcount);

            /* The back buffer is released with the swapchain, so AddRef with refcount=0 is fine here. */
            CHECK_ADDREF_REFCOUNT(pBackBuffer, 1);
            CHECK_REFCOUNT(device, ++refcount);
            CHECK_RELEASE_REFCOUNT(pBackBuffer, 0);
            CHECK_REFCOUNT(device, --refcount);
            CHECK_RELEASE_REFCOUNT(pBackBuffer, 0);
            CHECK_RELEASE_REFCOUNT(pBackBuffer, 0);
            pBackBuffer = NULL;
        }
        CHECK_REFCOUNT(pSwapChain, 1);
    }
    hr = IDirect3DDevice9_CreateQuery(device, D3DQUERYTYPE_EVENT, &pQuery);
    CHECK_CALL(hr, "CreateQuery", device, ++refcount);

    hr = IDirect3DDevice9_BeginStateBlock(device);
    CHECK_CALL(hr, "BeginStateBlock", device, refcount);
    hr = IDirect3DDevice9_EndStateBlock(device, &pStateBlock1);
    CHECK_CALL(hr, "EndStateBlock", device, ++refcount);

    /* The implicit render target is not freed if refcount reaches 0.
     * Otherwise GetRenderTarget would re-allocate it and the pointer would change.*/
    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &pRenderTarget2);
    CHECK_CALL(hr, "GetRenderTarget", device, ++refcount);
    if (pRenderTarget2)
    {
        CHECK_RELEASE_REFCOUNT(pRenderTarget2, 0);
        ok(pRenderTarget == pRenderTarget2, "RenderTarget=%p and RenderTarget2=%p should be the same.\n",
                pRenderTarget, pRenderTarget2);
        CHECK_REFCOUNT(device, --refcount);
        CHECK_RELEASE_REFCOUNT(pRenderTarget2, 0);
        CHECK_RELEASE_REFCOUNT(pRenderTarget2, 0);
        pRenderTarget2 = NULL;
    }
    pRenderTarget = NULL;

cleanup:
    CHECK_RELEASE(device,               device, --refcount);

    /* Buffers */
    CHECK_RELEASE(pVertexBuffer,        device, --refcount);
    CHECK_RELEASE(pIndexBuffer,         device, --refcount);
    /* Shaders */
    CHECK_RELEASE(pVertexDeclaration,   device, --refcount);
    CHECK_RELEASE(pVertexShader,        device, --refcount);
    CHECK_RELEASE(pPixelShader,         device, --refcount);
    /* Textures */
    CHECK_RELEASE(pTextureLevel,        device, --refcount);
    CHECK_RELEASE(pCubeTexture,         device, --refcount);
    CHECK_RELEASE(pVolumeTexture,       device, --refcount);
    /* Surfaces */
    CHECK_RELEASE(pStencilSurface,      device, --refcount);
    CHECK_RELEASE(pOffscreenSurface,    device, --refcount);
    CHECK_RELEASE(pRenderTarget3,       device, --refcount);
    /* Misc */
    CHECK_RELEASE(pStateBlock,          device, --refcount);
    CHECK_RELEASE(pSwapChain,           device, --refcount);
    CHECK_RELEASE(pQuery,               device, --refcount);
    /* This will destroy device - cannot check the refcount here */
    if (pStateBlock1)         CHECK_RELEASE_REFCOUNT( pStateBlock1, 0);
    CHECK_RELEASE_REFCOUNT(d3d, 0);
    DestroyWindow(window);
}

static void test_cursor(void)
{
    unsigned int adapter_idx, adapter_count, test_idx;
    IDirect3DSurface9 *cursor = NULL;
    struct device_desc device_desc;
    unsigned int width, height;
    IDirect3DDevice9 *device;
    HRESULT expected_hr, hr;
    D3DDISPLAYMODE mode;
    CURSORINFO info;
    IDirect3D9 *d3d;
    ULONG refcount;
    HCURSOR cur;
    HWND window;
    BOOL ret;

    static const DWORD device_flags[] = {0, CREATE_DEVICE_FULLSCREEN};
    static const SIZE cursor_sizes[] =
    {
        {1, 1},
        {2, 4},
        {3, 2},
        {2, 3},
        {6, 6},
    };

    window = create_window();
    ok(!!window, "Failed to create a window.\n");

    ret = SetCursorPos(50, 50);
    ok(ret, "Failed to set cursor position.\n");
    flush_events();
    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    ok(GetCursorInfo(&info), "GetCursorInfo failed\n");
    cur = info.hCursor;

    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        goto cleanup;
    }

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 32, 32,
            D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &cursor, NULL);
    ok(SUCCEEDED(hr), "Failed to create cursor surface, hr %#x.\n", hr);

    /* Initially hidden */
    ret = IDirect3DDevice9_ShowCursor(device, TRUE);
    ok(!ret, "IDirect3DDevice9_ShowCursor returned %d\n", ret);

    /* Not enabled without a surface*/
    ret = IDirect3DDevice9_ShowCursor(device, TRUE);
    ok(!ret, "IDirect3DDevice9_ShowCursor returned %d\n", ret);

    /* Fails */
    hr = IDirect3DDevice9_SetCursorProperties(device, 0, 0, NULL);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_SetCursorProperties returned %08x\n", hr);

    hr = IDirect3DDevice9_SetCursorProperties(device, 0, 0, cursor);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetCursorProperties returned %08x\n", hr);

    IDirect3DSurface9_Release(cursor);

    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    ok(GetCursorInfo(&info), "GetCursorInfo failed\n");
    ok(info.flags & (CURSOR_SHOWING|CURSOR_SUPPRESSED), "The gdi cursor is hidden (%08x)\n", info.flags);
    ok(info.hCursor == cur, "The cursor handle is %p\n", info.hCursor); /* unchanged */

    /* Still hidden */
    ret = IDirect3DDevice9_ShowCursor(device, TRUE);
    ok(!ret, "IDirect3DDevice9_ShowCursor returned %d\n", ret);

    /* Enabled now*/
    ret = IDirect3DDevice9_ShowCursor(device, TRUE);
    ok(ret, "IDirect3DDevice9_ShowCursor returned %d\n", ret);

    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);
    ok(GetCursorInfo(&info), "GetCursorInfo failed\n");
    ok(info.flags & (CURSOR_SHOWING|CURSOR_SUPPRESSED), "The gdi cursor is hidden (%08x)\n", info.flags);
    ok(info.hCursor != cur, "The cursor handle is %p\n", info.hCursor);

    /* Cursor dimensions must all be powers of two */
    for (test_idx = 0; test_idx < ARRAY_SIZE(cursor_sizes); ++test_idx)
    {
        width = cursor_sizes[test_idx].cx;
        height = cursor_sizes[test_idx].cy;
        hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, width, height, D3DFMT_A8R8G8B8,
                D3DPOOL_DEFAULT, &cursor, NULL);
        ok(hr == D3D_OK, "Test %u: CreateOffscreenPlainSurface failed, hr %#x.\n", test_idx, hr);
        hr = IDirect3DDevice9_SetCursorProperties(device, 0, 0, cursor);
        if (width && !(width & (width - 1)) && height && !(height & (height - 1)))
            expected_hr = D3D_OK;
        else
            expected_hr = D3DERR_INVALIDCALL;
        ok(hr == expected_hr, "Test %u: Expect SetCursorProperties return %#x, got %#x.\n",
                test_idx, expected_hr, hr);
        IDirect3DSurface9_Release(cursor);
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);

    /* Cursor dimensions must not exceed adapter display mode */
    device_desc.device_window = window;
    device_desc.width = 640;
    device_desc.height = 480;

    adapter_count = IDirect3D9_GetAdapterCount(d3d);
    for (adapter_idx = 0; adapter_idx < adapter_count; ++adapter_idx)
    {
        for (test_idx = 0; test_idx < ARRAY_SIZE(device_flags); ++test_idx)
        {
            device_desc.adapter_ordinal = adapter_idx;
            device_desc.flags = device_flags[test_idx];
            if (!(device = create_device(d3d, window, &device_desc)))
            {
                skip("Adapter %u test %u: Failed to create a D3D device.\n", adapter_idx, test_idx);
                break;
            }

            hr = IDirect3D9_GetAdapterDisplayMode(d3d, adapter_idx, &mode);
            ok(hr == D3D_OK, "Adapter %u test %u: GetAdapterDisplayMode failed, hr %#x.\n",
                    adapter_idx, test_idx, hr);

            /* Find the largest width and height that are powers of two and less than the display mode */
            width = 1;
            height = 1;
            while (width * 2 <= mode.Width)
                width *= 2;
            while (height * 2 <= mode.Height)
                height *= 2;

            hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, width, height,
                    D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &cursor, NULL);
            ok(hr == D3D_OK, "Adapter %u test %u: CreateOffscreenPlainSurface failed, hr %#x.\n",
                    adapter_idx, test_idx, hr);
            hr = IDirect3DDevice9_SetCursorProperties(device, 0, 0, cursor);
            ok(hr == D3D_OK, "Adapter %u test %u: SetCursorProperties failed, hr %#x.\n",
                    adapter_idx, test_idx, hr);
            IDirect3DSurface9_Release(cursor);

            hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, width * 2, height,
                    D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &cursor, NULL);
            ok(hr == D3D_OK, "Adapter %u test %u: CreateOffscreenPlainSurface failed, hr %#x.\n",
                    adapter_idx, test_idx, hr);
            hr = IDirect3DDevice9_SetCursorProperties(device, 0, 0, cursor);
            ok(hr == D3DERR_INVALIDCALL,
                    "Adapter %u test %u: Expect SetCursorProperties return %#x, got %#x.\n",
                    adapter_idx, test_idx, D3DERR_INVALIDCALL, hr);
            IDirect3DSurface9_Release(cursor);

            hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, width, height * 2,
                    D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &cursor, NULL);
            ok(hr == D3D_OK, "Adapter %u test %u: CreateOffscreenPlainSurface failed, hr %#x.\n",
                    adapter_idx, test_idx, hr);
            hr = IDirect3DDevice9_SetCursorProperties(device, 0, 0, cursor);
            ok(hr == D3DERR_INVALIDCALL,
                    "Adapter %u test %u: Expect SetCursorProperties return %#x, got %#x.\n",
                    adapter_idx, test_idx, D3DERR_INVALIDCALL, hr);
            IDirect3DSurface9_Release(cursor);

            refcount = IDirect3DDevice9_Release(device);
            ok(!refcount, "Adapter %u: Device has %u references left.\n", adapter_idx, refcount);
        }
    }
cleanup:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_reset(void)
{
    HRESULT                      hr;
    RECT                         winrect, client_rect;
    D3DPRESENT_PARAMETERS        d3dpp;
    D3DDISPLAYMODE               d3ddm, d3ddm2;
    D3DVIEWPORT9                 vp;
    DWORD                        width, orig_width = GetSystemMetrics(SM_CXSCREEN);
    DWORD                        height, orig_height = GetSystemMetrics(SM_CYSCREEN);
    IDirect3DSurface9            *surface;
    IDirect3DTexture9            *texture;
    IDirect3DVertexShader9       *shader;
    UINT                         i, adapter_mode_count;
    D3DLOCKED_RECT               lockrect;
    IDirect3DDevice9 *device1 = NULL;
    IDirect3DDevice9 *device2 = NULL;
    IDirect3DSwapChain9 *swapchain;
    struct device_desc device_desc;
    IDirect3DVertexBuffer9 *vb;
    IDirect3DIndexBuffer9 *ib;
    DEVMODEW devmode;
    IDirect3D9 *d3d;
    D3DCAPS9 caps;
    DWORD value;
    HWND hwnd;
    LONG ret;
    struct
    {
        UINT w;
        UINT h;
    } *modes = NULL;
    UINT mode_count = 0;

    hwnd = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            100, 100, 160, 160, NULL, NULL, NULL, NULL);
    ok(!!hwnd, "Failed to create a window.\n");
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    IDirect3D9_GetAdapterDisplayMode(d3d, D3DADAPTER_DEFAULT, &d3ddm);
    adapter_mode_count = IDirect3D9_GetAdapterModeCount(d3d, D3DADAPTER_DEFAULT, d3ddm.Format);
    modes = HeapAlloc(GetProcessHeap(), 0, sizeof(*modes) * adapter_mode_count);
    for(i = 0; i < adapter_mode_count; ++i)
    {
        UINT j;
        ZeroMemory( &d3ddm2, sizeof(d3ddm2) );
        hr = IDirect3D9_EnumAdapterModes(d3d, D3DADAPTER_DEFAULT, d3ddm.Format, i, &d3ddm2);
        ok(hr == D3D_OK, "IDirect3D9_EnumAdapterModes returned %#x\n", hr);

        for (j = 0; j < mode_count; ++j)
        {
            if (modes[j].w == d3ddm2.Width && modes[j].h == d3ddm2.Height)
                break;
        }
        if (j == mode_count)
        {
            modes[j].w = d3ddm2.Width;
            modes[j].h = d3ddm2.Height;
            ++mode_count;
        }

        /* We use them as invalid modes */
        if((d3ddm2.Width == 801 && d3ddm2.Height == 600) ||
           (d3ddm2.Width == 32 && d3ddm2.Height == 32)) {
            skip("This system supports a screen resolution of %dx%d, not running mode tests\n",
                 d3ddm2.Width, d3ddm2.Height);
            goto cleanup;
        }
    }

    if (mode_count < 2)
    {
        skip("Less than 2 modes supported, skipping mode tests\n");
        goto cleanup;
    }

    i = 0;
    if (modes[i].w == orig_width && modes[i].h == orig_height) ++i;

    device_desc.adapter_ordinal = D3DADAPTER_DEFAULT;
    device_desc.width = modes[i].w;
    device_desc.height = modes[i].h;
    device_desc.device_window = hwnd;
    device_desc.flags = CREATE_DEVICE_FULLSCREEN | CREATE_DEVICE_SWVP_ONLY;
    if (!(device1 = create_device(d3d, hwnd, &device_desc)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto cleanup;
    }
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3D_OK, "IDirect3DDevice9_TestCooperativeLevel after creation returned %#x\n", hr);

    hr = IDirect3DDevice9_GetDeviceCaps(device1, &caps);
    ok(SUCCEEDED(hr), "GetDeviceCaps failed, hr %#x.\n", hr);

    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == modes[i].w, "Screen width is %u, expected %u\n", width, modes[i].w);
    ok(height == modes[i].h, "Screen height is %u, expected %u\n", height, modes[i].h);

    hr = IDirect3DDevice9_GetViewport(device1, &vp);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetViewport failed with %08x\n", hr);
    ok(vp.X == 0, "D3DVIEWPORT->X = %d\n", vp.X);
    ok(vp.Y == 0, "D3DVIEWPORT->Y = %d\n", vp.Y);
    ok(vp.Width == modes[i].w, "D3DVIEWPORT->Width = %u, expected %u\n", vp.Width, modes[i].w);
    ok(vp.Height == modes[i].h, "D3DVIEWPORT->Height = %u, expected %u\n", vp.Height, modes[i].h);
    ok(vp.MinZ == 0, "D3DVIEWPORT->MinZ = %f\n", vp.MinZ);
    ok(vp.MaxZ == 1, "D3DVIEWPORT->MaxZ = %f\n", vp.MaxZ);

    i = 1;
    vp.X = 10;
    vp.Y = 20;
    vp.MinZ = 2;
    vp.MaxZ = 3;
    hr = IDirect3DDevice9_SetViewport(device1, &vp);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetViewport failed with %08x\n", hr);

    hr = IDirect3DDevice9_GetRenderState(device1, D3DRS_LIGHTING, &value);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#x.\n", hr);
    ok(!!value, "Got unexpected value %#x for D3DRS_LIGHTING.\n", value);
    hr = IDirect3DDevice9_SetRenderState(device1, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);

    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed         = FALSE;
    d3dpp.BackBufferWidth  = modes[i].w;
    d3dpp.BackBufferHeight = modes[i].h;
    d3dpp.BackBufferFormat = d3ddm.Format;
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed with %08x\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3D_OK, "IDirect3DDevice9_TestCooperativeLevel after a successful reset returned %#x\n", hr);

    hr = IDirect3DDevice9_GetRenderState(device1, D3DRS_LIGHTING, &value);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#x.\n", hr);
    ok(!!value, "Got unexpected value %#x for D3DRS_LIGHTING.\n", value);

    ZeroMemory(&vp, sizeof(vp));
    hr = IDirect3DDevice9_GetViewport(device1, &vp);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetViewport failed with %08x\n", hr);
    ok(vp.X == 0, "D3DVIEWPORT->X = %d\n", vp.X);
    ok(vp.Y == 0, "D3DVIEWPORT->Y = %d\n", vp.Y);
    ok(vp.Width == modes[i].w, "D3DVIEWPORT->Width = %u, expected %u\n", vp.Width, modes[i].w);
    ok(vp.Height == modes[i].h, "D3DVIEWPORT->Height = %u, expected %u\n", vp.Height, modes[i].h);
    ok(vp.MinZ == 0, "D3DVIEWPORT->MinZ = %f\n", vp.MinZ);
    ok(vp.MaxZ == 1, "D3DVIEWPORT->MaxZ = %f\n", vp.MaxZ);

    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == modes[i].w, "Screen width is %u, expected %u\n", width, modes[i].w);
    ok(height == modes[i].h, "Screen height is %u, expected %u\n", height, modes[i].h);

    hr = IDirect3DDevice9_GetSwapChain(device1, 0, &swapchain);
    ok(SUCCEEDED(hr), "Failed to get swapchain, hr %#x.\n", hr);
    memset(&d3dpp, 0, sizeof(d3dpp));
    hr = IDirect3DSwapChain9_GetPresentParameters(swapchain, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to get present parameters, hr %#x.\n", hr);
    ok(d3dpp.BackBufferWidth == modes[i].w, "Got unexpected BackBufferWidth %u, expected %u.\n",
            d3dpp.BackBufferWidth, modes[i].w);
    ok(d3dpp.BackBufferHeight == modes[i].h, "Got unexpected BackBufferHeight %u, expected %u.\n",
            d3dpp.BackBufferHeight, modes[i].h);
    IDirect3DSwapChain9_Release(swapchain);

    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed         = TRUE;
    d3dpp.BackBufferWidth  = 400;
    d3dpp.BackBufferHeight = 300;
    d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed with %08x\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3D_OK, "IDirect3DDevice9_TestCooperativeLevel after a successful reset returned %#x\n", hr);

    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == orig_width, "Screen width is %d\n", width);
    ok(height == orig_height, "Screen height is %d\n", height);

    ZeroMemory(&vp, sizeof(vp));
    hr = IDirect3DDevice9_GetViewport(device1, &vp);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetViewport failed with %08x\n", hr);
    ok(vp.X == 0, "D3DVIEWPORT->X = %d\n", vp.X);
    ok(vp.Y == 0, "D3DVIEWPORT->Y = %d\n", vp.Y);
    ok(vp.Width == 400, "D3DVIEWPORT->Width = %d\n", vp.Width);
    ok(vp.Height == 300, "D3DVIEWPORT->Height = %d\n", vp.Height);
    ok(vp.MinZ == 0, "D3DVIEWPORT->MinZ = %f\n", vp.MinZ);
    ok(vp.MaxZ == 1, "D3DVIEWPORT->MaxZ = %f\n", vp.MaxZ);

    hr = IDirect3DDevice9_GetSwapChain(device1, 0, &swapchain);
    ok(SUCCEEDED(hr), "Failed to get swapchain, hr %#x.\n", hr);
    memset(&d3dpp, 0, sizeof(d3dpp));
    hr = IDirect3DSwapChain9_GetPresentParameters(swapchain, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to get present parameters, hr %#x.\n", hr);
    ok(d3dpp.BackBufferWidth == 400, "Got unexpected BackBufferWidth %u.\n", d3dpp.BackBufferWidth);
    ok(d3dpp.BackBufferHeight == 300, "Got unexpected BackBufferHeight %u.\n", d3dpp.BackBufferHeight);
    IDirect3DSwapChain9_Release(swapchain);

    memset(&devmode, 0, sizeof(devmode));
    devmode.dmSize = sizeof(devmode);
    devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
    devmode.dmPelsWidth = modes[1].w;
    devmode.dmPelsHeight = modes[1].h;
    ret = ChangeDisplaySettingsW(&devmode, CDS_FULLSCREEN);
    ok(ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#x.\n", ret);
    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == modes[1].w, "Screen width is %u, expected %u.\n", width, modes[1].w);
    ok(height == modes[1].h, "Screen height is %u, expected %u.\n", height, modes[1].h);

    d3dpp.BackBufferWidth  = 500;
    d3dpp.BackBufferHeight = 400;
    d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3D_OK, "IDirect3DDevice9_TestCooperativeLevel after a successful reset returned %#x.\n", hr);

    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == modes[1].w, "Screen width is %u, expected %u.\n", width, modes[1].w);
    ok(height == modes[1].h, "Screen height is %u, expected %u.\n", height, modes[1].h);

    ZeroMemory(&vp, sizeof(vp));
    hr = IDirect3DDevice9_GetViewport(device1, &vp);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetViewport failed, hr %#x.\n", hr);
    ok(vp.X == 0, "D3DVIEWPORT->X = %d.\n", vp.X);
    ok(vp.Y == 0, "D3DVIEWPORT->Y = %d.\n", vp.Y);
    ok(vp.Width == 500, "D3DVIEWPORT->Width = %d.\n", vp.Width);
    ok(vp.Height == 400, "D3DVIEWPORT->Height = %d.\n", vp.Height);
    ok(vp.MinZ == 0, "D3DVIEWPORT->MinZ = %f.\n", vp.MinZ);
    ok(vp.MaxZ == 1, "D3DVIEWPORT->MaxZ = %f.\n", vp.MaxZ);

    hr = IDirect3DDevice9_GetSwapChain(device1, 0, &swapchain);
    ok(SUCCEEDED(hr), "Failed to get swapchain, hr %#x.\n", hr);
    memset(&d3dpp, 0, sizeof(d3dpp));
    hr = IDirect3DSwapChain9_GetPresentParameters(swapchain, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to get present parameters, hr %#x.\n", hr);
    ok(d3dpp.BackBufferWidth == 500, "Got unexpected BackBufferWidth %u.\n", d3dpp.BackBufferWidth);
    ok(d3dpp.BackBufferHeight == 400, "Got unexpected BackBufferHeight %u.\n", d3dpp.BackBufferHeight);
    IDirect3DSwapChain9_Release(swapchain);

    devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
    devmode.dmPelsWidth = orig_width;
    devmode.dmPelsHeight = orig_height;
    ret = ChangeDisplaySettingsW(&devmode, CDS_FULLSCREEN);
    ok(ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#x.\n", ret);
    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == orig_width, "Got screen width %u, expected %u.\n", width, orig_width);
    ok(height == orig_height, "Got screen height %u, expected %u.\n", height, orig_height);

    SetRect(&winrect, 0, 0, 200, 150);
    ok(AdjustWindowRect(&winrect, WS_OVERLAPPEDWINDOW, FALSE), "AdjustWindowRect failed\n");
    ok(SetWindowPos(hwnd, NULL, 0, 0,
                    winrect.right-winrect.left,
                    winrect.bottom-winrect.top,
                    SWP_NOMOVE|SWP_NOZORDER),
       "SetWindowPos failed\n");

    /* Windows 10 gives us a different size than we requested with some DPI scaling settings (e.g. 172%). */
    GetClientRect(hwnd, &client_rect);

    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed         = TRUE;
    d3dpp.BackBufferWidth  = 0;
    d3dpp.BackBufferHeight = 0;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed with %08x\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3D_OK, "IDirect3DDevice9_TestCooperativeLevel after a successful reset returned %#x\n", hr);

    ok(d3dpp.BackBufferWidth == client_rect.right, "Got unexpected BackBufferWidth %u, expected %d.\n",
            d3dpp.BackBufferWidth, client_rect.right);
    ok(d3dpp.BackBufferHeight == client_rect.bottom, "Got unexpected BackBufferHeight %u, expected %d.\n",
            d3dpp.BackBufferHeight, client_rect.bottom);
    ok(d3dpp.BackBufferFormat == d3ddm.Format, "Got unexpected BackBufferFormat %#x, expected %#x.\n",
            d3dpp.BackBufferFormat, d3ddm.Format);
    ok(d3dpp.BackBufferCount == 1, "Got unexpected BackBufferCount %u.\n", d3dpp.BackBufferCount);
    ok(!d3dpp.MultiSampleType, "Got unexpected MultiSampleType %u.\n", d3dpp.MultiSampleType);
    ok(!d3dpp.MultiSampleQuality, "Got unexpected MultiSampleQuality %u.\n", d3dpp.MultiSampleQuality);
    ok(d3dpp.SwapEffect == D3DSWAPEFFECT_DISCARD, "Got unexpected SwapEffect %#x.\n", d3dpp.SwapEffect);
    ok(!d3dpp.hDeviceWindow, "Got unexpected hDeviceWindow %p.\n", d3dpp.hDeviceWindow);
    ok(d3dpp.Windowed, "Got unexpected Windowed %#x.\n", d3dpp.Windowed);
    ok(!d3dpp.EnableAutoDepthStencil, "Got unexpected EnableAutoDepthStencil %#x.\n", d3dpp.EnableAutoDepthStencil);
    ok(!d3dpp.AutoDepthStencilFormat, "Got unexpected AutoDepthStencilFormat %#x.\n", d3dpp.AutoDepthStencilFormat);
    ok(!d3dpp.Flags, "Got unexpected Flags %#x.\n", d3dpp.Flags);
    ok(!d3dpp.FullScreen_RefreshRateInHz, "Got unexpected FullScreen_RefreshRateInHz %u.\n",
            d3dpp.FullScreen_RefreshRateInHz);
    ok(!d3dpp.PresentationInterval, "Got unexpected PresentationInterval %#x.\n", d3dpp.PresentationInterval);

    ZeroMemory(&vp, sizeof(vp));
    hr = IDirect3DDevice9_GetViewport(device1, &vp);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetViewport failed with %08x\n", hr);
    ok(vp.X == 0, "D3DVIEWPORT->X = %d\n", vp.X);
    ok(vp.Y == 0, "D3DVIEWPORT->Y = %d\n", vp.Y);
    ok(vp.Width == client_rect.right, "D3DVIEWPORT->Width = %d, expected %d\n",
            vp.Width, client_rect.right);
    ok(vp.Height == client_rect.bottom, "D3DVIEWPORT->Height = %d, expected %d\n",
            vp.Height, client_rect.bottom);
    ok(vp.MinZ == 0, "D3DVIEWPORT->MinZ = %f\n", vp.MinZ);
    ok(vp.MaxZ == 1, "D3DVIEWPORT->MaxZ = %f\n", vp.MaxZ);

    hr = IDirect3DDevice9_GetSwapChain(device1, 0, &swapchain);
    ok(SUCCEEDED(hr), "Failed to get swapchain, hr %#x.\n", hr);
    memset(&d3dpp, 0, sizeof(d3dpp));
    hr = IDirect3DSwapChain9_GetPresentParameters(swapchain, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to get present parameters, hr %#x.\n", hr);
    ok(d3dpp.BackBufferWidth == client_rect.right,
            "Got unexpected BackBufferWidth %u, expected %d.\n", d3dpp.BackBufferWidth, client_rect.right);
    ok(d3dpp.BackBufferHeight == client_rect.bottom,
            "Got unexpected BackBufferHeight %u, expected %d.\n", d3dpp.BackBufferHeight, client_rect.bottom);
    ok(d3dpp.BackBufferFormat == d3ddm.Format, "Got unexpected BackBufferFormat %#x, expected %#x.\n",
            d3dpp.BackBufferFormat, d3ddm.Format);
    ok(d3dpp.BackBufferCount == 1, "Got unexpected BackBufferCount %u.\n", d3dpp.BackBufferCount);
    ok(!d3dpp.MultiSampleType, "Got unexpected MultiSampleType %u.\n", d3dpp.MultiSampleType);
    ok(!d3dpp.MultiSampleQuality, "Got unexpected MultiSampleQuality %u.\n", d3dpp.MultiSampleQuality);
    ok(d3dpp.SwapEffect == D3DSWAPEFFECT_DISCARD, "Got unexpected SwapEffect %#x.\n", d3dpp.SwapEffect);
    ok(d3dpp.hDeviceWindow == hwnd, "Got unexpected hDeviceWindow %p, expected %p.\n", d3dpp.hDeviceWindow, hwnd);
    ok(d3dpp.Windowed, "Got unexpected Windowed %#x.\n", d3dpp.Windowed);
    ok(!d3dpp.EnableAutoDepthStencil, "Got unexpected EnableAutoDepthStencil %#x.\n", d3dpp.EnableAutoDepthStencil);
    ok(!d3dpp.AutoDepthStencilFormat, "Got unexpected AutoDepthStencilFormat %#x.\n", d3dpp.AutoDepthStencilFormat);
    ok(!d3dpp.Flags, "Got unexpected Flags %#x.\n", d3dpp.Flags);
    ok(!d3dpp.FullScreen_RefreshRateInHz, "Got unexpected FullScreen_RefreshRateInHz %u.\n",
            d3dpp.FullScreen_RefreshRateInHz);
    ok(!d3dpp.PresentationInterval, "Got unexpected PresentationInterval %#x.\n", d3dpp.PresentationInterval);
    IDirect3DSwapChain9_Release(swapchain);

    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed         = TRUE;
    d3dpp.BackBufferWidth  = 400;
    d3dpp.BackBufferHeight = 300;

    /* _Reset fails if there is a resource in the default pool */
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device1, 16, 16, D3DFMT_R5G6B5, D3DPOOL_DEFAULT, &surface, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateOffscreenPlainSurface returned %08x\n", hr);
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_Reset failed with %08x\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3DERR_DEVICENOTRESET, "IDirect3DDevice9_TestCooperativeLevel after a failed reset returned %#x\n", hr);
    IDirect3DSurface9_Release(surface);
    /* Reset again to get the device out of the lost state */
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed with %08x\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3D_OK, "IDirect3DDevice9_TestCooperativeLevel after a successful reset returned %#x\n", hr);

    if (caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP)
    {
        IDirect3DVolumeTexture9 *volume_texture;

        hr = IDirect3DDevice9_CreateVolumeTexture(device1, 16, 16, 4, 1, 0,
                D3DFMT_R5G6B5, D3DPOOL_DEFAULT, &volume_texture, NULL);
        ok(SUCCEEDED(hr), "CreateVolumeTexture failed, hr %#x.\n", hr);
        hr = IDirect3DDevice9_Reset(device1, &d3dpp);
        ok(hr == D3DERR_INVALIDCALL, "Reset returned %#x, expected %#x.\n", hr, D3DERR_INVALIDCALL);
        hr = IDirect3DDevice9_TestCooperativeLevel(device1);
        ok(hr == D3DERR_DEVICENOTRESET, "TestCooperativeLevel returned %#x, expected %#x.\n",
                hr, D3DERR_DEVICENOTRESET);
        IDirect3DVolumeTexture9_Release(volume_texture);
        hr = IDirect3DDevice9_Reset(device1, &d3dpp);
        ok(SUCCEEDED(hr), "Reset failed, hr %#x.\n", hr);
        hr = IDirect3DDevice9_TestCooperativeLevel(device1);
        ok(SUCCEEDED(hr), "TestCooperativeLevel failed, hr %#x.\n", hr);
    }
    else
    {
        skip("Volume textures not supported.\n");
    }

    /* Scratch, sysmem and managed pools are fine */
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device1, 16, 16, D3DFMT_R5G6B5, D3DPOOL_SCRATCH, &surface, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateOffscreenPlainSurface returned %08x\n", hr);
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed with %08x\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3D_OK, "IDirect3DDevice9_TestCooperativeLevel after a successful reset returned %#x\n", hr);
    IDirect3DSurface9_Release(surface);

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device1, 16, 16,
            D3DFMT_R5G6B5, D3DPOOL_SYSTEMMEM, &surface, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateOffscreenPlainSurface returned %08x\n", hr);
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed with %08x\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3D_OK, "IDirect3DDevice9_TestCooperativeLevel after a successful reset returned %#x\n", hr);
    IDirect3DSurface9_Release(surface);

    hr = IDirect3DDevice9_CreateVertexBuffer(device1, 16, 0,
            D3DFVF_XYZ, D3DPOOL_SYSTEMMEM, &vb, NULL);
    ok(hr == D3D_OK, "Failed to create vertex buffer, hr %#x.\n", hr);
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3D_OK, "Failed to reset device, hr %#x.\n", hr);
    IDirect3DVertexBuffer9_Release(vb);

    hr = IDirect3DDevice9_CreateIndexBuffer(device1, 16, 0,
            D3DFMT_INDEX16, D3DPOOL_SYSTEMMEM, &ib, NULL);
    ok(hr == D3D_OK, "Failed to create index buffer, hr %#x.\n", hr);
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3D_OK, "Failed to reset device, hr %#x.\n", hr);
    IDirect3DIndexBuffer9_Release(ib);

    /* The depth stencil should get reset to the auto depth stencil when present. */
    hr = IDirect3DDevice9_SetDepthStencilSurface(device1, NULL);
    ok(hr == D3D_OK, "SetDepthStencilSurface failed with 0x%08x\n", hr);

    hr = IDirect3DDevice9_GetDepthStencilSurface(device1, &surface);
    ok(hr == D3DERR_NOTFOUND, "GetDepthStencilSurface returned 0x%08x, expected D3DERR_NOTFOUND\n", hr);
    ok(surface == NULL, "Depth stencil should be NULL\n");

    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed with 0x%08x\n", hr);

    hr = IDirect3DDevice9_GetDepthStencilSurface(device1, &surface);
    ok(hr == D3D_OK, "GetDepthStencilSurface failed with 0x%08x\n", hr);
    ok(surface != NULL, "Depth stencil should not be NULL\n");
    if (surface) IDirect3DSurface9_Release(surface);

    d3dpp.EnableAutoDepthStencil = FALSE;
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed with 0x%08x\n", hr);

    hr = IDirect3DDevice9_GetDepthStencilSurface(device1, &surface);
    ok(hr == D3DERR_NOTFOUND, "GetDepthStencilSurface returned 0x%08x, expected D3DERR_NOTFOUND\n", hr);
    ok(surface == NULL, "Depth stencil should be NULL\n");

    /* Will a sysmem or scratch survive while locked */
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device1, 16, 16,
            D3DFMT_R5G6B5, D3DPOOL_SYSTEMMEM, &surface, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateOffscreenPlainSurface returned %08x\n", hr);
    hr = IDirect3DSurface9_LockRect(surface, &lockrect, NULL, D3DLOCK_DISCARD);
    ok(hr == D3D_OK, "IDirect3DSurface9_LockRect returned %08x\n", hr);
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed with %08x\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3D_OK, "IDirect3DDevice9_TestCooperativeLevel after a successful reset returned %#x\n", hr);
    IDirect3DSurface9_UnlockRect(surface);
    IDirect3DSurface9_Release(surface);

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device1, 16, 16, D3DFMT_R5G6B5, D3DPOOL_SCRATCH, &surface, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateOffscreenPlainSurface returned %08x\n", hr);
    hr = IDirect3DSurface9_LockRect(surface, &lockrect, NULL, D3DLOCK_DISCARD);
    ok(hr == D3D_OK, "IDirect3DSurface9_LockRect returned %08x\n", hr);
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed with %08x\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3D_OK, "IDirect3DDevice9_TestCooperativeLevel after a successful reset returned %#x\n", hr);
    IDirect3DSurface9_UnlockRect(surface);
    IDirect3DSurface9_Release(surface);

    hr = IDirect3DDevice9_CreateTexture(device1, 16, 16, 0, 0, D3DFMT_R5G6B5, D3DPOOL_MANAGED, &texture, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateTexture returned %08x\n", hr);
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed with %08x\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3D_OK, "IDirect3DDevice9_TestCooperativeLevel after a successful reset returned %#x\n", hr);
    IDirect3DTexture9_Release(texture);

    /* A reference held to an implicit surface causes failures as well */
    hr = IDirect3DDevice9_GetBackBuffer(device1, 0, 0, D3DBACKBUFFER_TYPE_MONO, &surface);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetBackBuffer returned %08x\n", hr);
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_Reset failed with %08x\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3DERR_DEVICENOTRESET, "IDirect3DDevice9_TestCooperativeLevel after a failed reset returned %#x\n", hr);
    IDirect3DSurface9_Release(surface);
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed with %08x\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3D_OK, "IDirect3DDevice9_TestCooperativeLevel after a successful reset returned %#x\n", hr);

    /* Shaders are fine as well */
    hr = IDirect3DDevice9_CreateVertexShader(device1, simple_vs, &shader);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateVertexShader returned %08x\n", hr);
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed with %08x\n", hr);
    IDirect3DVertexShader9_Release(shader);

    /* Try setting invalid modes */
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed         = FALSE;
    d3dpp.BackBufferWidth  = 32;
    d3dpp.BackBufferHeight = 32;
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_Reset to w=32, h=32, windowed=FALSE failed with %08x\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3DERR_DEVICENOTRESET, "IDirect3DDevice9_TestCooperativeLevel after a failed reset returned %#x\n", hr);

    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed         = FALSE;
    d3dpp.BackBufferWidth  = 801;
    d3dpp.BackBufferHeight = 600;
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_Reset to w=801, h=600, windowed=FALSE failed with %08x\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3DERR_DEVICENOTRESET, "IDirect3DDevice9_TestCooperativeLevel after a failed reset returned %#x\n", hr);

    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed         = FALSE;
    d3dpp.BackBufferWidth  = 0;
    d3dpp.BackBufferHeight = 0;
    hr = IDirect3DDevice9_Reset(device1, &d3dpp);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_Reset to w=0, h=0, windowed=FALSE failed with %08x\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device1);
    ok(hr == D3DERR_DEVICENOTRESET, "IDirect3DDevice9_TestCooperativeLevel after a failed reset returned %#x\n", hr);

    IDirect3D9_GetAdapterDisplayMode(d3d, D3DADAPTER_DEFAULT, &d3ddm);

    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed         = TRUE;
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.EnableAutoDepthStencil = FALSE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;

    if (FAILED(hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &device2)))
    {
        skip("could not create device, IDirect3D9_CreateDevice returned %#x\n", hr);
        goto cleanup;
    }

    hr = IDirect3DDevice9_TestCooperativeLevel(device2);
    ok(hr == D3D_OK, "IDirect3DDevice9_TestCooperativeLevel after creation returned %#x\n", hr);

    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed         = TRUE;
    d3dpp.BackBufferWidth  = 400;
    d3dpp.BackBufferHeight = 300;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;

    hr = IDirect3DDevice9_Reset(device2, &d3dpp);
    ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed with 0x%08x\n", hr);

    if (FAILED(hr)) goto cleanup;

    hr = IDirect3DDevice9_GetDepthStencilSurface(device2, &surface);
    ok(hr == D3D_OK, "GetDepthStencilSurface failed with 0x%08x\n", hr);
    ok(surface != NULL, "Depth stencil should not be NULL\n");
    if (surface) IDirect3DSurface9_Release(surface);

cleanup:
    HeapFree(GetProcessHeap(), 0, modes);
    if (device2)
    {
        UINT refcount = IDirect3DDevice9_Release(device2);
        ok(!refcount, "Device has %u references left.\n", refcount);
    }
    if (device1)
    {
        UINT refcount = IDirect3DDevice9_Release(device1);
        ok(!refcount, "Device has %u references left.\n", refcount);
    }
    IDirect3D9_Release(d3d);
    DestroyWindow(hwnd);
}

/* Test adapter display modes */
static void test_display_modes(void)
{
    D3DDISPLAYMODE dmode;
    IDirect3D9 *d3d;

    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

#define TEST_FMT(x,r) do { \
    HRESULT res = IDirect3D9_EnumAdapterModes(d3d, 0, (x), 0, &dmode); \
    ok(res==(r), "EnumAdapterModes("#x") did not return "#r" (got %08x)!\n", res); \
} while(0)

    TEST_FMT(D3DFMT_R8G8B8, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_A8R8G8B8, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_X8B8G8R8, D3DERR_INVALIDCALL);
    /* D3DFMT_R5G6B5 */
    TEST_FMT(D3DFMT_X1R5G5B5, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_A1R5G5B5, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_A4R4G4B4, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_R3G3B2, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_A8, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_A8R3G3B2, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_X4R4G4B4, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_A2B10G10R10, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_A8B8G8R8, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_X8B8G8R8, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_G16R16, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_A16B16G16R16, D3DERR_INVALIDCALL);

    TEST_FMT(D3DFMT_A8P8, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_P8, D3DERR_INVALIDCALL);

    TEST_FMT(D3DFMT_L8, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_A8L8, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_A4L4, D3DERR_INVALIDCALL);

    TEST_FMT(D3DFMT_V8U8, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_L6V5U5, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_X8L8V8U8, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_Q8W8V8U8, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_V16U16, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_A2W10V10U10, D3DERR_INVALIDCALL);

    TEST_FMT(D3DFMT_UYVY, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_YUY2, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_DXT1, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_DXT2, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_DXT3, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_DXT4, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_DXT5, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_MULTI2_ARGB8, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_G8R8_G8B8, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_R8G8_B8G8, D3DERR_INVALIDCALL);

    TEST_FMT(D3DFMT_D16_LOCKABLE, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_D32, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_D15S1, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_D24S8, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_D24X8, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_D24X4S4, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_D16, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_L16, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_D32F_LOCKABLE, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_D24FS8, D3DERR_INVALIDCALL);

    TEST_FMT(D3DFMT_VERTEXDATA, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_INDEX16, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_INDEX32, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_Q16W16V16U16, D3DERR_INVALIDCALL);
    /* Floating point formats */
    TEST_FMT(D3DFMT_R16F, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_G16R16F, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_A16B16G16R16F, D3DERR_INVALIDCALL);

    /* IEEE formats */
    TEST_FMT(D3DFMT_R32F, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_G32R32F, D3DERR_INVALIDCALL);
    TEST_FMT(D3DFMT_A32B32G32R32F, D3DERR_INVALIDCALL);

    TEST_FMT(D3DFMT_CxV8U8, D3DERR_INVALIDCALL);

    TEST_FMT(0, D3DERR_INVALIDCALL);

    IDirect3D9_Release(d3d);
}

static void test_scene(void)
{
    IDirect3DSurface9 *surface1, *surface2, *surface3;
    IDirect3DSurface9 *backBuffer, *rt, *ds;
    RECT rect = {0, 0, 128, 128};
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;

    window = create_window();
    ok(!!window, "Failed to create a window.\n");
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        goto cleanup;
    }

    /* Get the caps, they will be needed to tell if an operation is supposed to be valid */
    memset(&caps, 0, sizeof(caps));
    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetCaps failed with %08x\n", hr);

    /* Test an EndScene without BeginScene. Should return an error */
    hr = IDirect3DDevice9_EndScene(device);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_EndScene returned %08x\n", hr);

    /* Test a normal BeginScene / EndScene pair, this should work */
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %08x\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %08x\n", hr);

    /* Test another EndScene without having begun a new scene. Should return an error */
    hr = IDirect3DDevice9_EndScene(device);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_EndScene returned %08x\n", hr);

    /* Two nested BeginScene and EndScene calls */
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %08x\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_BeginScene returned %08x\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %08x\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_EndScene returned %08x\n", hr);

    /* Create some surfaces to test stretchrect between the scenes */
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 128, 128,
            D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &surface1, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateOffscreenPlainSurface failed with %08x\n", hr);
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 128, 128,
            D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &surface2, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateOffscreenPlainSurface failed with %08x\n", hr);
    hr = IDirect3DDevice9_CreateDepthStencilSurface(device, 800, 600,
            D3DFMT_D16, D3DMULTISAMPLE_NONE, 0, FALSE, &surface3, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateDepthStencilSurface failed with %08x\n", hr);
    hr = IDirect3DDevice9_CreateRenderTarget(device, 128, 128,
            D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, FALSE, &rt, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateRenderTarget failed with %08x\n", hr);

    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetBackBuffer failed with %08x\n", hr);
    hr = IDirect3DDevice9_GetDepthStencilSurface(device, &ds);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetBackBuffer failed with %08x\n", hr);

    /* First make sure a simple StretchRect call works */
    hr = IDirect3DDevice9_StretchRect(device, surface1, NULL, surface2, NULL, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);
    hr = IDirect3DDevice9_StretchRect(device, backBuffer, &rect, rt, NULL, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);
    if (0) /* Disabled for now because it crashes in wine */
    {
        HRESULT expected = caps.DevCaps2 & D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES ? D3D_OK : D3DERR_INVALIDCALL;
        hr = IDirect3DDevice9_StretchRect(device, ds, NULL, surface3, NULL, 0);
        ok(hr == expected, "Got unexpected hr %#x, expected %#x.\n", hr, expected);
    }

    /* Now try it in a BeginScene - EndScene pair. Seems to be allowed in a
     * BeginScene - Endscene pair with normal surfaces and render targets, but
     * not depth stencil surfaces. */
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %08x\n", hr);

    hr = IDirect3DDevice9_StretchRect(device, surface1, NULL, surface2, NULL, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);
    hr = IDirect3DDevice9_StretchRect(device, backBuffer, &rect, rt, NULL, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_StretchRect failed with %08x\n", hr);
    /* This is supposed to fail inside a BeginScene - EndScene pair. */
    hr = IDirect3DDevice9_StretchRect(device, ds, NULL, surface3, NULL, 0);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_StretchRect returned %08x, expected D3DERR_INVALIDCALL\n", hr);

    hr = IDirect3DDevice9_EndScene(device);
    ok(hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %08x\n", hr);

    /* Does a SetRenderTarget influence BeginScene / EndScene ?
     * Set a new render target, then see if it started a new scene. Flip the rt back and see if that maybe
     * ended the scene. Expected result is that the scene is not affected by SetRenderTarget
     */
    hr = IDirect3DDevice9_SetRenderTarget(device, 0, rt);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderTarget failed with %08x\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok( hr == D3D_OK, "IDirect3DDevice9_BeginScene failed with %08x\n", hr);
    hr = IDirect3DDevice9_SetRenderTarget(device, 0, backBuffer);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderTarget failed with %08x\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok( hr == D3D_OK, "IDirect3DDevice9_EndScene failed with %08x\n", hr);

    IDirect3DSurface9_Release(rt);
    IDirect3DSurface9_Release(ds);
    IDirect3DSurface9_Release(backBuffer);
    IDirect3DSurface9_Release(surface1);
    IDirect3DSurface9_Release(surface2);
    IDirect3DSurface9_Release(surface3);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
cleanup:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_limits(void)
{
    IDirect3DTexture9 *texture;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    unsigned int i;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = create_window();
    ok(!!window, "Failed to create a window.\n");
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        goto cleanup;
    }

    hr = IDirect3DDevice9_CreateTexture(device, 16, 16, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texture, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_CreateTexture failed with %08x\n", hr);

    /* There are 16 pixel samplers. We should be able to access all of them */
    for (i = 0; i < 16; ++i)
    {
        hr = IDirect3DDevice9_SetTexture(device, i, (IDirect3DBaseTexture9 *)texture);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture for sampler %d failed with %08x\n", i, hr);
        hr = IDirect3DDevice9_SetTexture(device, i, NULL);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTexture for sampler %d failed with %08x\n", i, hr);
        hr = IDirect3DDevice9_SetSamplerState(device, i, D3DSAMP_SRGBTEXTURE, TRUE);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetSamplerState for sampler %d failed with %08x\n", i, hr);
    }

    /* Now test all 8 textures stage states */
    for (i = 0; i < 8; ++i)
    {
        hr = IDirect3DDevice9_SetTextureStageState(device, i, D3DTSS_COLOROP, D3DTOP_ADD);
        ok(hr == D3D_OK, "IDirect3DDevice9_SetTextureStageState for texture %d failed with %08x\n", i, hr);
    }

    /* Investigations show that accessing higher samplers / textures stage
     * states does not return an error either. Writing to too high samplers
     * (approximately sampler 40) causes memory corruption in Windows, so
     * there is no bounds checking. */
    IDirect3DTexture9_Release(texture);
    refcount = IDirect3D9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
cleanup:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_depthstenciltest(void)
{
    HRESULT                      hr;
    IDirect3DDevice9            *pDevice            = NULL;
    D3DPRESENT_PARAMETERS        d3dpp;
    D3DDISPLAYMODE               d3ddm;
    IDirect3DSurface9           *pDepthStencil           = NULL;
    IDirect3DSurface9           *pDepthStencil2          = NULL;
    IDirect3D9 *d3d;
    DWORD state;
    HWND hwnd;

    hwnd = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            100, 100, 160, 160, NULL, NULL, NULL, NULL);
    ok(!!hwnd, "Failed to create a window.\n");
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    IDirect3D9_GetAdapterDisplayMode(d3d, D3DADAPTER_DEFAULT, &d3ddm);
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed         = TRUE;
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferWidth  = 800;
    d3dpp.BackBufferHeight = 600;
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL /* no NULLREF here */,
            hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice);
    ok(hr == D3D_OK || hr == D3DERR_NOTAVAILABLE || broken(hr == D3DERR_INVALIDCALL), "IDirect3D9_CreateDevice failed with %08x\n", hr);
    if(!pDevice)
    {
        skip("Failed to create a d3d device\n");
        goto cleanup;
    }

    hr = IDirect3DDevice9_GetDepthStencilSurface(pDevice, &pDepthStencil);
    ok(hr == D3D_OK && pDepthStencil != NULL, "IDirect3DDevice9_GetDepthStencilSurface failed with %08x\n", hr);

    /* Try to clear */
    hr = IDirect3DDevice9_Clear(pDevice, 0, NULL, D3DCLEAR_ZBUFFER, 0x00000000, 1.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %08x\n", hr);

    hr = IDirect3DDevice9_SetDepthStencilSurface(pDevice, NULL);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetDepthStencilSurface failed with %08x\n", hr);

    /* Check if the set buffer is returned on a get. WineD3D had a bug with that once, prevent it from coming back */
    hr = IDirect3DDevice9_GetDepthStencilSurface(pDevice, &pDepthStencil2);
    ok(hr == D3DERR_NOTFOUND && pDepthStencil2 == NULL, "IDirect3DDevice9_GetDepthStencilSurface failed with %08x\n", hr);
    if(pDepthStencil2) IDirect3DSurface9_Release(pDepthStencil2);

    /* This left the render states untouched! */
    hr = IDirect3DDevice9_GetRenderState(pDevice, D3DRS_ZENABLE, &state);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetRenderState failed with %08x\n", hr);
    ok(state == D3DZB_TRUE, "D3DRS_ZENABLE is %s\n", state == D3DZB_FALSE ? "D3DZB_FALSE" : (state == D3DZB_TRUE ? "D3DZB_TRUE" : "D3DZB_USEW"));
    hr = IDirect3DDevice9_GetRenderState(pDevice, D3DRS_ZWRITEENABLE, &state);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetRenderState failed with %08x\n", hr);
    ok(state == TRUE, "D3DRS_ZWRITEENABLE is %s\n", state ? "TRUE" : "FALSE");
    hr = IDirect3DDevice9_GetRenderState(pDevice, D3DRS_STENCILENABLE, &state);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetRenderState failed with %08x\n", hr);
    ok(state == FALSE, "D3DRS_STENCILENABLE is %s\n", state ? "TRUE" : "FALSE");
    hr = IDirect3DDevice9_GetRenderState(pDevice, D3DRS_STENCILWRITEMASK, &state);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetRenderState failed with %08x\n", hr);
    ok(state == 0xffffffff, "D3DRS_STENCILWRITEMASK is 0x%08x\n", state);

    /* This is supposed to fail now */
    hr = IDirect3DDevice9_Clear(pDevice, 0, NULL, D3DCLEAR_ZBUFFER, 0x00000000, 1.0, 0);
    ok(hr == D3DERR_INVALIDCALL, "IDirect3DDevice9_Clear failed with %08x\n", hr);

    hr = IDirect3DDevice9_SetRenderState(pDevice, D3DRS_ZENABLE, D3DZB_FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState failed with %08x\n", hr);

    hr = IDirect3DDevice9_SetDepthStencilSurface(pDevice, pDepthStencil);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetDepthStencilSurface failed with %08x\n", hr);

    hr = IDirect3DDevice9_GetRenderState(pDevice, D3DRS_ZENABLE, &state);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetRenderState failed with %08x\n", hr);
    ok(state == D3DZB_FALSE, "D3DRS_ZENABLE is %s\n", state == D3DZB_FALSE ? "D3DZB_FALSE" : (state == D3DZB_TRUE ? "D3DZB_TRUE" : "D3DZB_USEW"));

    /* Now it works again */
    hr = IDirect3DDevice9_Clear(pDevice, 0, NULL, D3DCLEAR_ZBUFFER, 0x00000000, 1.0, 0);
    ok(hr == D3D_OK, "IDirect3DDevice9_Clear failed with %08x\n", hr);

    if(pDepthStencil) IDirect3DSurface9_Release(pDepthStencil);
    IDirect3D9_Release(pDevice);

    /* Now see if autodepthstencil disable is honored. First, without a format set */
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed         = TRUE;
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferWidth  = 800;
    d3dpp.BackBufferHeight = 600;
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.EnableAutoDepthStencil = FALSE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_UNKNOWN;

    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL /* no NULLREF here */,
            hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice);
    ok(hr == D3D_OK || hr == D3DERR_NOTAVAILABLE, "IDirect3D9_CreateDevice failed with %08x\n", hr);
    if(!pDevice)
    {
        skip("Failed to create a d3d device\n");
        goto cleanup;
    }

    pDepthStencil = NULL;
    hr = IDirect3DDevice9_GetDepthStencilSurface(pDevice, &pDepthStencil);
    ok(hr == D3DERR_NOTFOUND && pDepthStencil == NULL, "IDirect3DDevice9_GetDepthStencilSurface returned %08x, surface = %p\n", hr, pDepthStencil);
    if(pDepthStencil) {
        IDirect3DSurface9_Release(pDepthStencil);
        pDepthStencil = NULL;
    }

    /* Check the depth test state */
    hr = IDirect3DDevice9_GetRenderState(pDevice, D3DRS_ZENABLE, &state);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetRenderState failed with %08x\n", hr);
    ok(state == D3DZB_FALSE, "D3DRS_ZENABLE is %s\n", state == D3DZB_FALSE ? "D3DZB_FALSE" : (state == D3DZB_TRUE ? "D3DZB_TRUE" : "D3DZB_USEW"));

    IDirect3D9_Release(pDevice);

    /* Next, try EnableAutoDepthStencil FALSE with a depth stencil format set */
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed         = TRUE;
    d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferWidth  = 800;
    d3dpp.BackBufferHeight = 600;
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.EnableAutoDepthStencil = FALSE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

    hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL /* no NULLREF here */,
            hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice);
    ok(hr == D3D_OK || hr == D3DERR_NOTAVAILABLE, "IDirect3D9_CreateDevice failed with %08x\n", hr);
    if(!pDevice)
    {
        skip("Failed to create a d3d device\n");
        goto cleanup;
    }

    pDepthStencil = NULL;
    hr = IDirect3DDevice9_GetDepthStencilSurface(pDevice, &pDepthStencil);
    ok(hr == D3DERR_NOTFOUND && pDepthStencil == NULL, "IDirect3DDevice9_GetDepthStencilSurface returned %08x, surface = %p\n", hr, pDepthStencil);
    if(pDepthStencil) {
        IDirect3DSurface9_Release(pDepthStencil);
        pDepthStencil = NULL;
    }

    hr = IDirect3DDevice9_GetRenderState(pDevice, D3DRS_ZENABLE, &state);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetRenderState failed with %08x\n", hr);
    ok(state == D3DZB_FALSE, "D3DRS_ZENABLE is %s\n", state == D3DZB_FALSE ? "D3DZB_FALSE" : (state == D3DZB_TRUE ? "D3DZB_TRUE" : "D3DZB_USEW"));

cleanup:
    if(pDepthStencil) IDirect3DSurface9_Release(pDepthStencil);
    if (pDevice)
    {
        UINT refcount = IDirect3D9_Release(pDevice);
        ok(!refcount, "Device has %u references left.\n", refcount);
    }
    IDirect3D9_Release(d3d);
    DestroyWindow(hwnd);
}

static void test_get_rt(void)
{
    IDirect3DSurface9 *backbuffer, *rt;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d9;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;
    ULONG ref;
    UINT i;

    window = create_window();
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");
    device = create_device(d3d9, window, NULL);
    if (!device)
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &backbuffer);
    ok(SUCCEEDED(hr), "Failed to get backbuffer, hr %#x.\n", hr);
    ok(!!backbuffer, "Got a NULL backbuffer.\n");

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);

    for (i = 1; i < caps.NumSimultaneousRTs; ++i)
    {
        rt = backbuffer;
        hr = IDirect3DDevice9_GetRenderTarget(device, i, &rt);
        ok(hr == D3DERR_NOTFOUND, "IDirect3DDevice9_GetRenderTarget returned %#x.\n", hr);
        ok(!rt, "Got rt %p.\n", rt);
    }

    IDirect3DSurface9_Release(backbuffer);

    ref = IDirect3DDevice9_Release(device);
    ok(!ref, "The device was not properly freed: refcount %u.\n", ref);
done:
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_draw_primitive(void)
{
    static const struct
    {
        float position[3];
        DWORD color;
    }
    quad[] =
    {
        {{-1.0f, -1.0f, 0.0f}, 0xffff0000},
        {{-1.0f,  1.0f, 0.0f}, 0xffff0000},
        {{ 1.0f,  1.0f, 0.0f}, 0xffff0000},
        {{ 1.0f, -1.0f, 0.0f}, 0xffff0000},
    };
    static const WORD indices[] = {0, 1, 2, 3, 0, 2};
    static const D3DVERTEXELEMENT9 decl_elements[] =
    {
        {0, 0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT,    D3DDECLUSAGE_COLOR, 0},
        D3DDECL_END()
    };

    IDirect3DVertexBuffer9 *vertex_buffer, *current_vb;
    IDirect3DIndexBuffer9 *index_buffer, *current_ib;
    IDirect3DVertexDeclaration9 *vertex_declaration;
    IDirect3DStateBlock9 *stateblock;
    IDirect3DDevice9 *device;
    UINT offset, stride;
    IDirect3D9 *d3d9;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    void *ptr;

    window = create_window();
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d9, window, NULL)))
    {
        skip("Failed to create a D3D device.\n");
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements, &vertex_declaration);
    ok(SUCCEEDED(hr), "CreateVertexDeclaration failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateVertexBuffer(device, sizeof(quad), 0, 0,
            D3DPOOL_DEFAULT, &vertex_buffer, NULL);
    ok(SUCCEEDED(hr), "CreateVertexBuffer failed, hr %#x.\n", hr);
    hr = IDirect3DVertexBuffer9_Lock(vertex_buffer, 0, 0, &ptr, D3DLOCK_DISCARD);
    ok(SUCCEEDED(hr), "Lock failed, hr %#x.\n", hr);
    memcpy(ptr, quad, sizeof(quad));
    hr = IDirect3DVertexBuffer9_Unlock(vertex_buffer);
    ok(SUCCEEDED(hr), "Unlock failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetStreamSource(device, 0, vertex_buffer, 0, sizeof(*quad));
    ok(SUCCEEDED(hr), "SetStreamSource failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateIndexBuffer(device, sizeof(indices), 0, D3DFMT_INDEX16,
            D3DPOOL_DEFAULT, &index_buffer, NULL);
    ok(SUCCEEDED(hr), "CreateIndexBuffer failed, hr %#x.\n", hr);
    hr = IDirect3DIndexBuffer9_Lock(index_buffer, 0, 0, &ptr, D3DLOCK_DISCARD);
    ok(SUCCEEDED(hr), "Lock failed, hr %#x.\n", hr);
    memcpy(ptr, indices, sizeof(indices));
    hr = IDirect3DIndexBuffer9_Unlock(index_buffer);
    ok(SUCCEEDED(hr), "Unlock failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "SetRenderState D3DRS_LIGHTING failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetVertexDeclaration(device, NULL);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_DrawPrimitive(device, D3DPT_TRIANGLELIST, 0, 2);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetStreamSource(device, 0, &current_vb, &offset, &stride);
    ok(SUCCEEDED(hr), "GetStreamSource failed, hr %#x.\n", hr);
    ok(current_vb == vertex_buffer, "Unexpected vb %p.\n", current_vb);
    ok(!offset, "Unexpected offset %u.\n", offset);
    ok(stride == sizeof(*quad), "Unexpected stride %u.\n", stride);
    IDirect3DVertexBuffer9_Release(current_vb);

    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLELIST, 2, quad, sizeof(*quad));
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetStreamSource(device, 0, &current_vb, &offset, &stride);
    ok(SUCCEEDED(hr), "GetStreamSource failed, hr %#x.\n", hr);
    todo_wine ok(!current_vb, "Unexpected vb %p.\n", current_vb);
    ok(!offset, "Unexpected offset %u.\n", offset);
    ok(stride == sizeof(*quad), "Unexpected stride %u.\n", stride);
    if (current_vb)
        IDirect3DVertexBuffer9_Release(current_vb);

    hr = IDirect3DDevice9_SetIndices(device, NULL);
    ok(SUCCEEDED(hr), "SetIndices failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, 0 /* BaseVertexIndex */,
            0 /* MinIndex */, 4 /* NumVerts */, 0 /* StartIndex */, 2 /*PrimCount */);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    /* Valid index buffer, NULL vertex declaration. */
    hr = IDirect3DDevice9_SetIndices(device, index_buffer);
    ok(SUCCEEDED(hr), "SetIndices failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, 0 /* BaseVertexIndex */,
            0 /* MinIndex */, 4 /* NumVerts */, 0 /* StartIndex */, 2 /*PrimCount */);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0, 4, 2,
            indices, D3DFMT_INDEX16, quad, sizeof(*quad));
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetIndices(device, &current_ib);
    ok(SUCCEEDED(hr), "GetIndices failed, hr %#x.\n", hr);
    todo_wine ok(!current_ib, "Unexpected index buffer %p.\n", current_vb);
    if (current_ib)
        IDirect3DIndexBuffer9_Release(current_ib);

    hr = IDirect3DDevice9_SetVertexDeclaration(device, vertex_declaration);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_DrawPrimitive(device, D3DPT_TRIANGLELIST, 0, 2);
    ok(SUCCEEDED(hr), "DrawPrimitive failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLELIST, 2, quad, 0);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLELIST, 0, quad, 0);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLELIST, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawPrimitiveUP failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetStreamSource(device, 0, &current_vb, &offset, &stride);
    ok(SUCCEEDED(hr), "GetStreamSource failed, hr %#x.\n", hr);
    ok(!current_vb, "Unexpected vb %p.\n", current_vb);
    ok(!offset, "Unexpected offset %u.\n", offset);
    ok(!stride, "Unexpected stride %u.\n", stride);

    /* NULL index buffer, valid vertex declaration, NULL stream source. */
    hr = IDirect3DDevice9_SetIndices(device, NULL);
    ok(SUCCEEDED(hr), "SetIndices failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, 0 /* BaseVertexIndex */,
            0 /* MinIndex */, 4 /* NumVerts */, 0 /* StartIndex */, 2 /*PrimCount */);
    todo_wine ok(SUCCEEDED(hr), "DrawIndexedPrimitive failed, hr %#x.\n", hr);

    /* Valid index buffer and vertex declaration, NULL stream source. */
    hr = IDirect3DDevice9_SetIndices(device, index_buffer);
    ok(SUCCEEDED(hr), "SetIndices failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawIndexedPrimitive(device, D3DPT_TRIANGLELIST, 0 /* BaseVertexIndex */,
            0 /* MinIndex */, 4 /* NumVerts */, 0 /* StartIndex */, 2 /*PrimCount */);
    ok(SUCCEEDED(hr), "DrawIndexedPrimitive failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetIndices(device, &current_ib);
    ok(SUCCEEDED(hr), "GetIndices failed, hr %#x.\n", hr);
    ok(current_ib == index_buffer, "Unexpected index buffer %p.\n", current_ib);
    IDirect3DIndexBuffer9_Release(current_ib);

    hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0, 4, 2,
            indices, D3DFMT_INDEX16, quad, 0);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0, 4, 0,
            indices, D3DFMT_INDEX16, quad, 0);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0, 4, 2,
            indices, D3DFMT_INDEX16, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawIndexedPrimitiveUP failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetIndices(device, &current_ib);
    ok(SUCCEEDED(hr), "GetIndices failed, hr %#x.\n", hr);
    ok(!current_ib, "Unexpected index buffer %p.\n", current_ib);

    /* Resetting of stream source and index buffer is not recorded in stateblocks. */

    hr = IDirect3DDevice9_SetStreamSource(device, 0, vertex_buffer, 0, sizeof(*quad));
    ok(SUCCEEDED(hr), "SetStreamSource failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetIndices(device, index_buffer);
    ok(SUCCEEDED(hr), "SetIndices failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginStateBlock(device);
    ok(SUCCEEDED(hr), "BeginStateBlock failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0, 4, 2,
            indices, D3DFMT_INDEX16, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "DrawIndexedPrimitiveUP failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_EndStateBlock(device, &stateblock);
    ok(SUCCEEDED(hr), "BeginStateBlock failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetStreamSource(device, 0, &current_vb, &offset, &stride);
    ok(SUCCEEDED(hr), "GetStreamSource failed, hr %#x.\n", hr);
    ok(!current_vb, "Unexpected vb %p.\n", current_vb);
    ok(!offset, "Unexpected offset %u.\n", offset);
    ok(!stride, "Unexpected stride %u.\n", stride);
    hr = IDirect3DDevice9_GetIndices(device, &current_ib);
    ok(SUCCEEDED(hr), "GetIndices failed, hr %#x.\n", hr);
    ok(!current_ib, "Unexpected index buffer %p.\n", current_ib);

    hr = IDirect3DStateBlock9_Capture(stateblock);
    ok(SUCCEEDED(hr), "Capture failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetStreamSource(device, 0, vertex_buffer, 0, sizeof(*quad));
    ok(SUCCEEDED(hr), "SetStreamSource failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetIndices(device, index_buffer);
    ok(SUCCEEDED(hr), "SetIndices failed, hr %#x.\n", hr);

    hr = IDirect3DStateBlock9_Apply(stateblock);
    ok(SUCCEEDED(hr), "Capture failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetStreamSource(device, 0, &current_vb, &offset, &stride);
    ok(SUCCEEDED(hr), "GetStreamSource failed, hr %#x.\n", hr);
    ok(current_vb == vertex_buffer, "Unexpected vb %p.\n", current_vb);
    ok(!offset, "Unexpected offset %u.\n", offset);
    ok(stride == sizeof(*quad), "Unexpected stride %u.\n", stride);
    IDirect3DVertexBuffer9_Release(current_vb);
    hr = IDirect3DDevice9_GetIndices(device, &current_ib);
    ok(SUCCEEDED(hr), "GetIndices failed, hr %#x.\n", hr);
    ok(current_ib == index_buffer, "Unexpected index buffer %p.\n", current_ib);
    IDirect3DIndexBuffer9_Release(current_ib);

    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed, hr %#x.\n", hr);

    IDirect3DStateBlock9_Release(stateblock);
    IDirect3DVertexBuffer9_Release(vertex_buffer);
    IDirect3DIndexBuffer9_Release(index_buffer);
    IDirect3DVertexDeclaration9_Release(vertex_declaration);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_null_stream(void)
{
    IDirect3DVertexBuffer9 *buffer = NULL;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d9;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    IDirect3DVertexShader9 *shader = NULL;
    IDirect3DVertexDeclaration9 *decl = NULL;
    static const DWORD shader_code[] =
    {
        0xfffe0101,                             /* vs_1_1           */
        0x0000001f, 0x80000000, 0x900f0000,     /* dcl_position v0  */
        0x00000001, 0xc00f0000, 0x90e40000,     /* mov oPos, v0     */
        0x0000ffff                              /* end              */
    };
    static const D3DVERTEXELEMENT9 decl_elements[] = {
        {0, 0,  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {1, 0,  D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT,    D3DDECLUSAGE_COLOR, 0},
        D3DDECL_END()
    };

    window = create_window();
    ok(!!window, "Failed to create a window.\n");
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d9, window, NULL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        goto cleanup;
    }

    hr = IDirect3DDevice9_CreateVertexShader(device, shader_code, &shader);
    if(FAILED(hr)) {
        skip("No vertex shader support\n");
        goto cleanup;
    }
    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl_elements, &decl);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_CreateVertexDeclaration failed (0x%08x)\n", hr);
    if (FAILED(hr)) {
        skip("Vertex declaration handling not possible.\n");
        goto cleanup;
    }
    hr = IDirect3DDevice9_CreateVertexBuffer(device, 12 * sizeof(float), 0, 0, D3DPOOL_MANAGED, &buffer, NULL);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_CreateVertexBuffer failed (0x%08x)\n", hr);
    if (FAILED(hr)) {
        skip("Vertex buffer handling not possible.\n");
        goto cleanup;
    }

    hr = IDirect3DDevice9_SetStreamSource(device, 0, buffer, 0, sizeof(float) * 3);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetStreamSource failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetStreamSource(device, 1, NULL, 0, 0);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetStreamSource failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetVertexShader(device, shader);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetVertexShader failed (0x%08x)\n", hr);
    hr = IDirect3DDevice9_SetVertexDeclaration(device, decl);
    ok(SUCCEEDED(hr), "IDirect3DDevice9_SetVertexDeclaration failed (0x%08x)\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitive(device, D3DPT_POINTLIST, 0, 1);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    IDirect3DDevice9_SetStreamSource(device, 0, NULL, 0, 0);
    IDirect3DDevice9_SetVertexShader(device, NULL);
    IDirect3DDevice9_SetVertexDeclaration(device, NULL);

cleanup:
    if (buffer) IDirect3DVertexBuffer9_Release(buffer);
    if (decl) IDirect3DVertexDeclaration9_Release(decl);
    if (shader) IDirect3DVertexShader9_Release(shader);
    if (device)
    {
        refcount = IDirect3DDevice9_Release(device);
        ok(!refcount, "Device has %u references left.\n", refcount);
    }
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_lights(void)
{
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d9;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    unsigned int i;
    BOOL enabled;
    D3DCAPS9 caps;

    window = create_window();
    ok(!!window, "Failed to create a window.\n");
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d9, window, NULL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        goto cleanup;
    }

    memset(&caps, 0, sizeof(caps));
    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(hr == D3D_OK, "IDirect3DDevice9_GetDeviceCaps failed with %08x\n", hr);

    for(i = 1; i <= caps.MaxActiveLights; i++) {
        hr = IDirect3DDevice9_LightEnable(device, i, TRUE);
        ok(hr == D3D_OK, "Enabling light %u failed with %08x\n", i, hr);
        hr = IDirect3DDevice9_GetLightEnable(device, i, &enabled);
        ok(hr == D3D_OK, "GetLightEnable on light %u failed with %08x\n", i, hr);
        ok(enabled, "Light %d is %s\n", i, enabled ? "enabled" : "disabled");
    }

    /* TODO: Test the rendering results in this situation */
    hr = IDirect3DDevice9_LightEnable(device, i + 1, TRUE);
    ok(hr == D3D_OK, "Enabling one light more than supported returned %08x\n", hr);
    hr = IDirect3DDevice9_GetLightEnable(device, i + 1, &enabled);
    ok(hr == D3D_OK, "GetLightEnable on light %u failed with %08x\n", i + 1, hr);
    ok(enabled, "Light %d is %s\n", i + 1, enabled ? "enabled" : "disabled");
    hr = IDirect3DDevice9_LightEnable(device, i + 1, FALSE);
    ok(hr == D3D_OK, "Disabling the additional returned %08x\n", hr);

    for(i = 1; i <= caps.MaxActiveLights; i++) {
        hr = IDirect3DDevice9_LightEnable(device, i, FALSE);
        ok(hr == D3D_OK, "Disabling light %u failed with %08x\n", i, hr);
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
cleanup:
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_set_stream_source(void)
{
    IDirect3DVertexBuffer9 *vb, *current_vb;
    unsigned int offset, stride;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d9;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = create_window();
    ok(!!window, "Failed to create a window.\n");
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d9, window, NULL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        goto cleanup;
    }

    hr = IDirect3DDevice9_CreateVertexBuffer(device, 512, 0, 0, D3DPOOL_DEFAULT, &vb, NULL);
    ok(SUCCEEDED(hr), "Failed to create a vertex buffer, hr %#x.\n", hr);

    /* Some cards (GeForce 7400 at least) accept non-aligned offsets, others
     * (Radeon 9000 verified) reject them, so accept both results. Wine
     * currently rejects this to be able to optimize the vbo conversion, but
     * writes a WARN. */
    hr = IDirect3DDevice9_SetStreamSource(device, 0, vb, 0, 32);
    ok(SUCCEEDED(hr), "Failed to set the stream source, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetStreamSource(device, 0, vb, 1, 32);
    ok(hr == D3DERR_INVALIDCALL || hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetStreamSource(device, 0, vb, 2, 32);
    ok(hr == D3DERR_INVALIDCALL || hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetStreamSource(device, 0, vb, 3, 32);
    ok(hr == D3DERR_INVALIDCALL || hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetStreamSource(device, 0, vb, 4, 32);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetStreamSource(device, 0, NULL, 0, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetStreamSource(device, 0, &current_vb, &offset, &stride);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(!current_vb, "Got unexpected vb %p.\n", current_vb);
    ok(offset == 4, "Got unexpected offset %u.\n", offset);
    ok(stride == 32, "Got unexpected stride %u.\n", stride);

    hr = IDirect3DDevice9_SetStreamSource(device, 0, vb, 0, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetStreamSource(device, 0, &current_vb, &offset, &stride);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(current_vb == vb, "Got unexpected vb %p.\n", current_vb);
    IDirect3DVertexBuffer9_Release(current_vb);
    ok(!offset, "Got unexpected offset %u.\n", offset);
    ok(!stride, "Got unexpected stride %u.\n", stride);

    /* Try to set the NULL buffer with an offset and stride 0 */
    hr = IDirect3DDevice9_SetStreamSource(device, 0, NULL, 0, 0);
    ok(SUCCEEDED(hr), "Failed to set the stream source, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetStreamSource(device, 0, NULL, 1, 0);
    ok(hr == D3DERR_INVALIDCALL || hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetStreamSource(device, 0, NULL, 2, 0);
    ok(hr == D3DERR_INVALIDCALL || hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetStreamSource(device, 0, NULL, 3, 0);
    ok(hr == D3DERR_INVALIDCALL || hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetStreamSource(device, 0, NULL, 4, 0);
    ok(SUCCEEDED(hr), "Failed to set the stream source, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetStreamSource(device, 0, NULL, 0, 0);
    ok(SUCCEEDED(hr), "Failed to set the stream source, hr %#x.\n", hr);

    IDirect3DVertexBuffer9_Release(vb);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
cleanup:
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

/* Direct3D9 offers 4 display formats: R5G6B5, X1R5G5B5, X8R8G8B8 and
 * A2R10G10B10. Next to these there are 6 different back buffer formats. Only
 * a fixed number of combinations are possible in fullscreen mode. In windowed
 * mode more combinations are allowed due to format conversion and this is
 * likely driver dependent. */
static void test_display_formats(void)
{
    D3DDEVTYPE device_type = D3DDEVTYPE_HAL;
    unsigned int backbuffer, display;
    unsigned int windowed;
    IDirect3D9 *d3d9;
    BOOL should_pass;
    BOOL has_modes;
    HRESULT hr;

    static const struct
    {
        const char *name;
        D3DFORMAT format;
        D3DFORMAT alpha_format;
        BOOL display;
        BOOL windowed;
    }
    formats[] =
    {
        {"D3DFMT_R5G6B5",       D3DFMT_R5G6B5,      0,                  TRUE,   TRUE},
        {"D3DFMT_X1R5G5B5",     D3DFMT_X1R5G5B5,    D3DFMT_A1R5G5B5,    TRUE,   TRUE},
        {"D3DFMT_A1R5G5B5",     D3DFMT_A1R5G5B5,    D3DFMT_A1R5G5B5,    FALSE,  FALSE},
        {"D3DFMT_X8R8G8B8",     D3DFMT_X8R8G8B8,    D3DFMT_A8R8G8B8,    TRUE,   TRUE},
        {"D3DFMT_A8R8G8B8",     D3DFMT_A8R8G8B8,    D3DFMT_A8R8G8B8,    FALSE,  FALSE},
        {"D3DFMT_A2R10G10B10",  D3DFMT_A2R10G10B10, 0,                  TRUE,   FALSE},
        {"D3DFMT_UNKNOWN",      D3DFMT_UNKNOWN,     0,                  FALSE,  FALSE},
    };

    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");

    for (display = 0; display < ARRAY_SIZE(formats); ++display)
    {
        has_modes = IDirect3D9_GetAdapterModeCount(d3d9, D3DADAPTER_DEFAULT, formats[display].format);

        for (windowed = 0; windowed <= 1; ++windowed)
        {
            for (backbuffer = 0; backbuffer < ARRAY_SIZE(formats); ++backbuffer)
            {
                should_pass = FALSE;

                if (formats[display].display && (formats[display].windowed || !windowed) && (has_modes || windowed))
                {
                    D3DFORMAT backbuffer_format;

                    if (windowed && formats[backbuffer].format == D3DFMT_UNKNOWN)
                        backbuffer_format = formats[display].format;
                    else
                        backbuffer_format = formats[backbuffer].format;

                    hr = IDirect3D9_CheckDeviceFormat(d3d9, D3DADAPTER_DEFAULT, device_type, formats[display].format,
                            D3DUSAGE_RENDERTARGET, D3DRTYPE_SURFACE, backbuffer_format);
                    if (hr == D3D_OK)
                    {
                        if (windowed)
                        {
                            hr = IDirect3D9_CheckDeviceFormatConversion(d3d9, D3DADAPTER_DEFAULT, device_type,
                                    backbuffer_format, formats[display].format);
                            should_pass = (hr == D3D_OK);
                        }
                        else
                            should_pass = (formats[display].format == formats[backbuffer].format
                                    || (formats[display].alpha_format
                                    && formats[display].alpha_format == formats[backbuffer].alpha_format));
                    }
                }

                hr = IDirect3D9_CheckDeviceType(d3d9, D3DADAPTER_DEFAULT, device_type,
                        formats[display].format, formats[backbuffer].format, windowed);
                ok(SUCCEEDED(hr) == should_pass || broken(SUCCEEDED(hr) && !has_modes) /* Win8 64-bit */,
                        "Got unexpected hr %#x for %s / %s, windowed %#x, should_pass %#x.\n",
                        hr, formats[display].name, formats[backbuffer].name, windowed, should_pass);
            }
        }
    }

    IDirect3D9_Release(d3d9);
}

static void test_scissor_size(void)
{
    struct device_desc device_desc;
    IDirect3D9 *d3d9_ptr;
    unsigned int i;
    static struct {
        int winx; int winy; int backx; int backy; DWORD flags;
    } scts[] = { /* scissor tests */
        {800, 600, 640, 480, 0},
        {800, 600, 640, 480, CREATE_DEVICE_FULLSCREEN},
        {640, 480, 800, 600, 0},
        {640, 480, 800, 600, CREATE_DEVICE_FULLSCREEN},
    };

    d3d9_ptr = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9_ptr, "Failed to create a D3D object.\n");

    for (i = 0; i < ARRAY_SIZE(scts); i++)
    {
        IDirect3DDevice9 *device_ptr = 0;
        HRESULT hr;
        HWND hwnd = 0;
        RECT scissorrect;

        hwnd = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION,
                0, 0, scts[i].winx, scts[i].winy, 0, 0, 0, 0);

        if (scts[i].flags & CREATE_DEVICE_FULLSCREEN)
        {
            scts[i].backx = registry_mode.dmPelsWidth;
            scts[i].backy = registry_mode.dmPelsHeight;
        }

        device_desc.adapter_ordinal = D3DADAPTER_DEFAULT;
        device_desc.device_window = hwnd;
        device_desc.width = scts[i].backx;
        device_desc.height = scts[i].backy;
        device_desc.flags = scts[i].flags;
        if (!(device_ptr = create_device(d3d9_ptr, hwnd, &device_desc)))
        {
            skip("Failed to create a 3D device, skipping test.\n");
            DestroyWindow(hwnd);
            goto err_out;
        }

        /* Check for the default scissor rect size */
        hr = IDirect3DDevice9_GetScissorRect(device_ptr, &scissorrect);
        ok(hr == D3D_OK, "IDirect3DDevice9_GetScissorRect failed with: %08x\n", hr);
        ok(scissorrect.right == scts[i].backx && scissorrect.bottom == scts[i].backy
                && scissorrect.top == 0 && scissorrect.left == 0,
                "Scissorrect mismatch (%d, %d) should be (%d, %d)\n", scissorrect.right, scissorrect.bottom,
                scts[i].backx, scts[i].backy);

        /* check the scissorrect values after a reset */
        device_desc.width = registry_mode.dmPelsWidth;
        device_desc.height = registry_mode.dmPelsHeight;
        device_desc.flags = scts[i].flags;
        hr = reset_device(device_ptr, &device_desc);
        ok(hr == D3D_OK, "IDirect3DDevice9_Reset failed with %08x\n", hr);
        hr = IDirect3DDevice9_TestCooperativeLevel(device_ptr);
        ok(hr == D3D_OK, "IDirect3DDevice9_TestCooperativeLevel after a successful reset returned %#x\n", hr);

        hr = IDirect3DDevice9_GetScissorRect(device_ptr, &scissorrect);
        ok(hr == D3D_OK, "IDirect3DDevice9_GetScissorRect failed with: %08x\n", hr);
        ok(scissorrect.right == registry_mode.dmPelsWidth && scissorrect.bottom == registry_mode.dmPelsHeight
                && scissorrect.top == 0 && scissorrect.left == 0,
                "Scissorrect mismatch (%d, %d) should be (%u, %u)\n", scissorrect.right, scissorrect.bottom,
                registry_mode.dmPelsWidth, registry_mode.dmPelsHeight);

        if (device_ptr)
        {
            ULONG ref;

            ref = IDirect3DDevice9_Release(device_ptr);
            DestroyWindow(hwnd);
            ok(ref == 0, "The device was not properly freed: refcount %u\n", ref);
        }
    }

err_out:
    IDirect3D9_Release(d3d9_ptr);
}

static void test_multi_device(void)
{
    IDirect3DDevice9 *device1, *device2;
    HWND window1, window2;
    IDirect3D9 *d3d9;
    ULONG refcount;

    window1 = create_window();
    ok(!!window1, "Failed to create a window.\n");
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");
    if (!(device1 = create_device(d3d9, window1, NULL)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        IDirect3D9_Release(d3d9);
        DestroyWindow(window1);
        return;
    }
    IDirect3D9_Release(d3d9);

    window2 = create_window();
    ok(!!window2, "Failed to create a window.\n");
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");
    device2 = create_device(d3d9, window2, NULL);
    IDirect3D9_Release(d3d9);

    refcount = IDirect3DDevice9_Release(device2);
    ok(!refcount, "Device has %u references left.\n", refcount);
    refcount = IDirect3DDevice9_Release(device1);
    ok(!refcount, "Device has %u references left.\n", refcount);
    DestroyWindow(window2);
    DestroyWindow(window1);
}

static HWND filter_messages;

enum message_window
{
    DEVICE_WINDOW,
    FOCUS_WINDOW,
};

struct message
{
    UINT message;
    enum message_window window;
    BOOL check_wparam;
    WPARAM expect_wparam;
    HRESULT device_state;
    WINDOWPOS *store_wp;
};

static const struct message *expect_messages;
static HWND device_window, focus_window;
static LONG windowposchanged_received, syscommand_received, wm_size_received;
static IDirect3DDevice9 *focus_test_device;

struct wndproc_thread_param
{
    HWND dummy_window;
    HANDLE window_created;
    HANDLE test_finished;
    BOOL running_in_foreground;
};

static LRESULT CALLBACK test_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    HRESULT hr;

    if (filter_messages && filter_messages == hwnd)
    {
        if (message != WM_DISPLAYCHANGE && message != WM_IME_NOTIFY)
            todo_wine ok( 0, "Received unexpected message %#x for window %p.\n", message, hwnd);
    }

    if (expect_messages)
    {
        HWND w;

        switch (expect_messages->window)
        {
            case DEVICE_WINDOW:
                w = device_window;
                break;

            case FOCUS_WINDOW:
                w = focus_window;
                break;

            default:
                w = NULL;
                break;
        };

        if (hwnd == w && expect_messages->message == message)
        {
            if (expect_messages->check_wparam)
                ok(wparam == expect_messages->expect_wparam,
                        "Got unexpected wparam %lx for message %x, expected %lx.\n",
                        wparam, message, expect_messages->expect_wparam);

            if (expect_messages->store_wp)
                *expect_messages->store_wp = *(WINDOWPOS *)lparam;

            if (focus_test_device)
            {
                hr = IDirect3DDevice9_TestCooperativeLevel(focus_test_device);
                /* Wined3d marks the device lost earlier than Windows (it follows ddraw
                 * behavior. See test_wndproc before the focus_loss_messages sequence
                 * about the D3DERR_DEVICENOTRESET behavior, */
                todo_wine_if(message != WM_ACTIVATEAPP || hr == D3D_OK)
                        ok(hr == expect_messages->device_state,
                        "Got device state %#x on message %#x, expected %#x.\n",
                        hr, message, expect_messages->device_state);
            }

            ++expect_messages;
        }
    }

    /* KDE randomly does something with the hidden window during the
     * mode change that sometimes generates a WM_WINDOWPOSCHANGING
     * message. A WM_WINDOWPOSCHANGED message is not generated, so
     * just flag WM_WINDOWPOSCHANGED as bad. */
    if (message == WM_WINDOWPOSCHANGED)
        InterlockedIncrement(&windowposchanged_received);
    else if (message == WM_SYSCOMMAND)
        InterlockedIncrement(&syscommand_received);
    else if (message == WM_SIZE)
        InterlockedIncrement(&wm_size_received);

    return DefWindowProcA(hwnd, message, wparam, lparam);
}

static DWORD WINAPI wndproc_thread(void *param)
{
    struct wndproc_thread_param *p = param;
    DWORD res;
    BOOL ret;

    p->dummy_window = CreateWindowA("d3d9_test_wndproc_wc", "d3d9_test",
            WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION, 0, 0, registry_mode.dmPelsWidth,
            registry_mode.dmPelsHeight, 0, 0, 0, 0);
    p->running_in_foreground = SetForegroundWindow(p->dummy_window);

    ret = SetEvent(p->window_created);
    ok(ret, "SetEvent failed, last error %#x.\n", GetLastError());

    for (;;)
    {
        MSG msg;

        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
        res = WaitForSingleObject(p->test_finished, 100);
        if (res == WAIT_OBJECT_0) break;
        if (res != WAIT_TIMEOUT)
        {
            ok(0, "Wait failed (%#x), last error %#x.\n", res, GetLastError());
            break;
        }
    }

    DestroyWindow(p->dummy_window);

    return 0;
}

static void test_wndproc(void)
{
    struct wndproc_thread_param thread_params;
    struct device_desc device_desc;
    static WINDOWPOS windowpos;
    IDirect3DDevice9 *device;
    WNDCLASSA wc = {0};
    IDirect3D9 *d3d9;
    HANDLE thread;
    LONG_PTR proc;
    ULONG ref;
    DWORD res, tid;
    HWND tmp;
    UINT i, adapter_mode_count;
    HRESULT hr;
    D3DDISPLAYMODE d3ddm;
    DWORD d3d_width = 0, d3d_height = 0, user32_width = 0, user32_height = 0;
    DEVMODEW devmode;
    LONG change_ret, device_style;
    BOOL ret;

    static const struct message create_messages[] =
    {
        {WM_WINDOWPOSCHANGING,  FOCUS_WINDOW,   FALSE,  0},
        /* Do not test wparam here. If device creation succeeds,
         * wparam is WA_ACTIVE. If device creation fails (testbot)
         * wparam is set to WA_INACTIVE on some Windows versions. */
        {WM_ACTIVATE,           FOCUS_WINDOW,   FALSE,  0},
        {WM_SETFOCUS,           FOCUS_WINDOW,   FALSE,  0},
        {0,                     0,              FALSE,  0},
    };
    static const struct message focus_loss_messages[] =
    {
        /* WM_ACTIVATE (wparam = WA_INACTIVE) is sent on Windows. It is
         * not reliable on X11 WMs. When the window focus follows the
         * mouse pointer the message is not sent.
         * {WM_ACTIVATE,           FOCUS_WINDOW,   TRUE,   WA_INACTIVE}, */
        {WM_DISPLAYCHANGE,      DEVICE_WINDOW,  FALSE,  0,  D3DERR_DEVICENOTRESET},
        /* WM_DISPLAYCHANGE is sent to the focus window too, but the order is
         * not deterministic. */
        {WM_WINDOWPOSCHANGING,  DEVICE_WINDOW,  FALSE,  0,  D3DERR_DEVICENOTRESET},
        /* Windows sends WM_ACTIVATE to the device window, indicating that
         * SW_SHOWMINIMIZED is used instead of SW_MINIMIZE. Yet afterwards
         * the foreground and focus window are NULL. On Wine SW_SHOWMINIMIZED
         * leaves the device window active, breaking re-activation in the
         * lost device test.
         * {WM_ACTIVATE,           DEVICE_WINDOW,  TRUE,   0x200000 | WA_ACTIVE}, */
        {WM_WINDOWPOSCHANGED,   DEVICE_WINDOW,  FALSE,  0,  D3DERR_DEVICENOTRESET},
        {WM_SIZE,               DEVICE_WINDOW,  TRUE,   SIZE_MINIMIZED,
                D3DERR_DEVICENOTRESET},
        {WM_ACTIVATEAPP,        FOCUS_WINDOW,   TRUE,   FALSE,  D3DERR_DEVICELOST},
        /* WM_ACTIVATEAPP is sent to the device window too, but the order is
         * not deterministic. It may be sent after the focus window handling
         * or before. */
        {0,                     0,              FALSE,  0,      0},
    };
    static const struct message focus_loss_messages_nowc[] =
    {
        /* WM_ACTIVATE (wparam = WA_INACTIVE) is sent on Windows. It is
         * not reliable on X11 WMs. When the window focus follows the
         * mouse pointer the message is not sent.
         * {WM_ACTIVATE,           FOCUS_WINDOW,   TRUE,   WA_INACTIVE}, */
        {WM_DISPLAYCHANGE,      DEVICE_WINDOW,  FALSE,  0,      D3DERR_DEVICENOTRESET},
        {WM_ACTIVATEAPP,        FOCUS_WINDOW,   TRUE,   FALSE,  D3DERR_DEVICELOST},
        {0,                     0,              FALSE,  0,      0},
    };
    static const struct message reactivate_messages[] =
    {
        {WM_WINDOWPOSCHANGING,  DEVICE_WINDOW,  FALSE,  0},
        {WM_WINDOWPOSCHANGED,   DEVICE_WINDOW,  FALSE,  0},
        /* optional WM_MOVE here if size changed */
        {WM_ACTIVATEAPP,        FOCUS_WINDOW,   TRUE,   TRUE},
        {0,                     0,              FALSE,  0},
    };
    static const struct message reactivate_messages_nowc[] =
    {
        /* We're activating the device window before activating the
         * focus window, so no ACTIVATEAPP message is sent. */
        {WM_ACTIVATE,           FOCUS_WINDOW,   TRUE,   WA_ACTIVE},
        {0,                     0,              FALSE,  0},
    };
    static const struct message focus_loss_messages_hidden[] =
    {
        {WM_DISPLAYCHANGE,      DEVICE_WINDOW,  FALSE,  0},
        {WM_ACTIVATEAPP,        FOCUS_WINDOW,   TRUE,   FALSE},
        {0,                     0,              FALSE,  0},
    };
    static const struct message focus_loss_messages_filtered[] =
    {
        /* WM_ACTIVATE is delivered to the window proc because it is
         * generated by SetForegroundWindow before the d3d routine
         * starts it work. Don't check for it due to focus-follows-mouse
         * WMs though. */
        {WM_DISPLAYCHANGE,      FOCUS_WINDOW,   FALSE,  0},
        {WM_ACTIVATEAPP,        FOCUS_WINDOW,   TRUE,   FALSE},
        {0,                     0,              FALSE,  0},
    };
    static const struct message reactivate_messages_filtered[] =
    {
        {WM_ACTIVATEAPP,        FOCUS_WINDOW,   TRUE,   TRUE},
        {0,                     0,              FALSE,  0},
    };
    static const struct message sc_restore_messages[] =
    {
        /* WM_SYSCOMMAND is delivered only once, after d3d has already
         * processed it. Our wndproc has no way to prevent d3d from
         * handling the message. The second DefWindowProc call done by
         * our wndproc doesn't do any changes to the window because it
         * is already restored due to d3d's handling. */
        {WM_WINDOWPOSCHANGING,  FOCUS_WINDOW,   FALSE,  0},
        {WM_WINDOWPOSCHANGED,   FOCUS_WINDOW,   FALSE,  0},
        {WM_SIZE,               FOCUS_WINDOW,   TRUE,   SIZE_RESTORED},
        {WM_SYSCOMMAND,         FOCUS_WINDOW,   TRUE,   SC_RESTORE},
        {0,                     0,              FALSE,  0},
    };
    static const struct message sc_minimize_messages[] =
    {
        {WM_SYSCOMMAND,         FOCUS_WINDOW,   TRUE,   SC_MINIMIZE},
        {WM_WINDOWPOSCHANGING,  FOCUS_WINDOW,   FALSE,  0},
        {WM_WINDOWPOSCHANGED,   FOCUS_WINDOW,   FALSE,  0},
        {WM_MOVE,               FOCUS_WINDOW,   FALSE,  0},
        {WM_SIZE,               FOCUS_WINDOW,   TRUE,   SIZE_MINIMIZED},
        {0,                     0,              FALSE,  0},
    };
    static const struct message sc_maximize_messages[] =
    {
        {WM_SYSCOMMAND,         FOCUS_WINDOW,   TRUE,   SC_MAXIMIZE},
        {WM_WINDOWPOSCHANGING,  FOCUS_WINDOW,   FALSE,  0},
        {WM_WINDOWPOSCHANGED,   FOCUS_WINDOW,   FALSE,  0},
        {WM_MOVE,               FOCUS_WINDOW,   FALSE,  0},
        /* WM_SIZE(SIZE_MAXIMIZED) is unreliable on native. */
        {0,                     0,              FALSE,  0},
    };
    static const struct message mode_change_messages[] =
    {
        {WM_WINDOWPOSCHANGING,  DEVICE_WINDOW,  FALSE,  0},
        {WM_WINDOWPOSCHANGED,   DEVICE_WINDOW,  FALSE,  0},
        {WM_SIZE,               DEVICE_WINDOW,  FALSE,  0},
        /* TODO: WM_DISPLAYCHANGE is sent to the focus window too, but the order is
         * differs between Wine and Windows. */
        /* TODO 2: Windows sends a second WM_WINDOWPOSCHANGING(SWP_NOMOVE | SWP_NOSIZE
         * | SWP_NOACTIVATE) in this situation, suggesting a difference in their ShowWindow
         * implementation. This SetWindowPos call could in theory affect the Z order. Wine's
         * ShowWindow does not send such a message because the window is already visible. */
        {0,                     0,              FALSE,  0},
    };
    static const struct message mode_change_messages_hidden[] =
    {
        {WM_WINDOWPOSCHANGING,  DEVICE_WINDOW,  FALSE,  0},
        {WM_WINDOWPOSCHANGED,   DEVICE_WINDOW,  FALSE,  0},
        {WM_SIZE,               DEVICE_WINDOW,  FALSE,  0},
        {WM_SHOWWINDOW,         DEVICE_WINDOW,  FALSE,  0},
        {WM_WINDOWPOSCHANGING,  DEVICE_WINDOW,  FALSE,  0, ~0U, &windowpos},
        {WM_WINDOWPOSCHANGED,   DEVICE_WINDOW,  FALSE,  0},
        /* TODO: WM_DISPLAYCHANGE is sent to the focus window too, but the order is
         * differs between Wine and Windows. */
        {0,                     0,              FALSE,  0},
    };
    static const struct message mode_change_messages_nowc[] =
    {
        {WM_DISPLAYCHANGE,      FOCUS_WINDOW,   FALSE,  0},
        {0,                     0,              FALSE,  0},
    };
    static const struct
    {
        DWORD create_flags;
        const struct message *focus_loss_messages, *reactivate_messages;
        const struct message *mode_change_messages, *mode_change_messages_hidden;
        BOOL iconic;
    }
    tests[] =
    {
        {
            0,
            focus_loss_messages,
            reactivate_messages,
            mode_change_messages,
            mode_change_messages_hidden,
            TRUE
        },
        {
            CREATE_DEVICE_NOWINDOWCHANGES,
            focus_loss_messages_nowc,
            reactivate_messages_nowc,
            mode_change_messages_nowc,
            mode_change_messages_nowc,
            FALSE
        },
    };

    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");

    adapter_mode_count = IDirect3D9_GetAdapterModeCount(d3d9, D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8);
    for (i = 0; i < adapter_mode_count; ++i)
    {
        hr = IDirect3D9_EnumAdapterModes(d3d9, D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8, i, &d3ddm);
        ok(SUCCEEDED(hr), "Failed to enumerate display mode, hr %#x.\n", hr);

        if (d3ddm.Width == registry_mode.dmPelsWidth && d3ddm.Height == registry_mode.dmPelsHeight)
            continue;
        /* The r200 driver on Windows XP enumerates modes like 320x200 and 320x240 but
         * refuses to create a device at these sizes. */
        if (d3ddm.Width < 640 || d3ddm.Height < 480)
            continue;

        if (!user32_width)
        {
            user32_width = d3ddm.Width;
            user32_height = d3ddm.Height;
            continue;
        }

        /* Make sure the d3d mode is smaller in width or height and at most
         * equal in the other dimension than the mode passed to
         * ChangeDisplaySettings. Otherwise Windows shrinks the window to
         * the ChangeDisplaySettings parameters + 12. */
        if (d3ddm.Width == user32_width && d3ddm.Height == user32_height)
            continue;
        if (d3ddm.Width <= user32_width && d3ddm.Height <= user32_height)
        {
            d3d_width = d3ddm.Width;
            d3d_height = d3ddm.Height;
            break;
        }
        if (user32_width <= d3ddm.Width && user32_height <= d3ddm.Height)
        {
            d3d_width = user32_width;
            d3d_height = user32_height;
            user32_width = d3ddm.Width;
            user32_height = d3ddm.Height;
            break;
        }
    }

    if (!d3d_width)
    {
        skip("Could not find adequate modes, skipping mode tests.\n");
        IDirect3D9_Release(d3d9);
        return;
    }

    filter_messages = NULL;
    expect_messages = NULL;

    wc.lpfnWndProc = test_proc;
    wc.lpszClassName = "d3d9_test_wndproc_wc";
    ok(RegisterClassA(&wc), "Failed to register window class.\n");

    thread_params.window_created = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(!!thread_params.window_created, "CreateEvent failed, last error %#x.\n", GetLastError());
    thread_params.test_finished = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(!!thread_params.test_finished, "CreateEvent failed, last error %#x.\n", GetLastError());

    memset(&devmode, 0, sizeof(devmode));
    devmode.dmSize = sizeof(devmode);
    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
        devmode.dmPelsWidth = user32_width;
        devmode.dmPelsHeight = user32_height;
        change_ret = ChangeDisplaySettingsW(&devmode, CDS_FULLSCREEN);
        ok(change_ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#x.\n", change_ret);

        focus_window = CreateWindowA("d3d9_test_wndproc_wc", "d3d9_test",
                WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION, 0, 0, user32_width, user32_height, 0, 0, 0, 0);
        device_window = CreateWindowA("d3d9_test_wndproc_wc", "d3d9_test",
                WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION, 0, 0, user32_width, user32_height, 0, 0, 0, 0);
        thread = CreateThread(NULL, 0, wndproc_thread, &thread_params, 0, &tid);
        ok(!!thread, "Failed to create thread, last error %#x.\n", GetLastError());

        res = WaitForSingleObject(thread_params.window_created, INFINITE);
        ok(res == WAIT_OBJECT_0, "Wait failed (%#x), last error %#x.\n", res, GetLastError());

        proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
        ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
                (LONG_PTR)test_proc, proc);
        proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
        ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
                (LONG_PTR)test_proc, proc);

        trace("device_window %p, focus_window %p, dummy_window %p.\n",
                device_window, focus_window, thread_params.dummy_window);

        tmp = GetFocus();
        ok(tmp == device_window, "Expected focus %p, got %p.\n", device_window, tmp);
        if (thread_params.running_in_foreground)
        {
            tmp = GetForegroundWindow();
            ok(tmp == thread_params.dummy_window, "Expected foreground window %p, got %p.\n",
                    thread_params.dummy_window, tmp);
        }
        else
            skip("Not running in foreground, skip foreground window test\n");

        flush_events();

        expect_messages = create_messages;

        device_desc.adapter_ordinal = D3DADAPTER_DEFAULT;
        device_desc.device_window = device_window;
        device_desc.width = d3d_width;
        device_desc.height = d3d_height;
        device_desc.flags = CREATE_DEVICE_FULLSCREEN | tests[i].create_flags;
        if (!(device = create_device(d3d9, focus_window, &device_desc)))
        {
            skip("Failed to create a D3D device, skipping tests.\n");
            goto done;
        }

        ok(!expect_messages->message, "Expected message %#x for window %#x, but didn't receive it, i=%u.\n",
                expect_messages->message, expect_messages->window, i);
        expect_messages = NULL;

        if (0) /* Disabled until we can make this work in a reliable way on Wine. */
        {
            tmp = GetFocus();
            ok(tmp == focus_window, "Expected focus %p, got %p.\n", focus_window, tmp);
            tmp = GetForegroundWindow();
            ok(tmp == focus_window, "Expected foreground window %p, got %p.\n", focus_window, tmp);
        }
        SetForegroundWindow(focus_window);
        flush_events();

        proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
        ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx, i=%u.\n",
                (LONG_PTR)test_proc, proc, i);

        proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
        ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#lx, i=%u.\n",
                (LONG_PTR)test_proc, i);

        /* Change the mode while the device is in use and then drop focus. */
        devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
        devmode.dmPelsWidth = user32_width;
        devmode.dmPelsHeight = user32_height;
        change_ret = ChangeDisplaySettingsW(&devmode, CDS_FULLSCREEN);
        ok(change_ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#x, i=%u.\n", change_ret, i);

        /* Wine doesn't (yet) mark the device not reset when the mode is changed, thus the todo_wine.
         * But sometimes focus-follows-mouse WMs also temporarily drop window focus, which makes
         * mark the device lost, then not reset, causing the test to succeed for the wrong reason. */
        hr = IDirect3DDevice9_TestCooperativeLevel(device);
        todo_wine_if (hr != D3DERR_DEVICENOTRESET)
            ok(hr == D3DERR_DEVICENOTRESET, "Got unexpected hr %#x.\n", hr);

        focus_test_device = device;
        expect_messages = tests[i].focus_loss_messages;
        /* SetForegroundWindow is a poor replacement for the user pressing alt-tab or
         * manually changing the focus. It generates the same messages, but the task
         * bar still shows the previous foreground window as active, and the window has
         * an inactive titlebar if reactivated with SetForegroundWindow. Reactivating
         * the device is difficult, see below. */
        SetForegroundWindow(GetDesktopWindow());
        ok(!expect_messages->message, "Expected message %#x for window %#x, but didn't receive it, i=%u.\n",
                expect_messages->message, expect_messages->window, i);
        expect_messages = NULL;
        tmp = GetFocus();
        ok(tmp != device_window, "The device window is active, i=%u.\n", i);
        ok(tmp != focus_window, "The focus window is active, i=%u.\n", i);
        focus_test_device = NULL;

        hr = IDirect3DDevice9_TestCooperativeLevel(device);
        ok(hr == D3DERR_DEVICELOST, "Got unexpected hr %#x.\n", hr);

        ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devmode);
        ok(ret, "Failed to get display mode.\n");
        ok(devmode.dmPelsWidth == registry_mode.dmPelsWidth
                && devmode.dmPelsHeight == registry_mode.dmPelsHeight, "Got unexpected screen size %ux%u.\n",
                devmode.dmPelsWidth, devmode.dmPelsHeight);

        /* This is needed on native with D3DCREATE_NOWINDOWCHANGES, and it needs to be
         * done before the focus window is restored. This makes sense to some extent
         * because minimizing the window on focus loss is the application's job if this
         * flag is set. */
        if (tests[i].create_flags & CREATE_DEVICE_NOWINDOWCHANGES)
        {
            ShowWindow(device_window, SW_MINIMIZE);
            ShowWindow(device_window, SW_RESTORE);
        }
        flush_events();

        /* I have to minimize and restore the focus window, otherwise native d3d9 fails
         * device::reset with D3DERR_DEVICELOST. This does not happen when the window
         * restore is triggered by the user. */
        expect_messages = tests[i].reactivate_messages;
        ShowWindow(focus_window, SW_MINIMIZE);
        ShowWindow(focus_window, SW_RESTORE);
        /* Set focus twice to make KDE and fvwm in focus-follows-mouse mode happy. */
        SetForegroundWindow(focus_window);
        flush_events();
        SetForegroundWindow(focus_window);
        flush_events(); /* WM_WINDOWPOSCHANGING etc arrive after SetForegroundWindow returns. */
        ok(!expect_messages->message, "Expected message %#x for window %#x, but didn't receive it, i=%u.\n",
                expect_messages->message, expect_messages->window, i);
        expect_messages = NULL;

        hr = IDirect3DDevice9_TestCooperativeLevel(device);
        ok(hr == D3DERR_DEVICENOTRESET, "Got unexpected hr %#x.\n", hr);

        ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devmode);
        ok(ret, "Failed to get display mode.\n");
        ok(devmode.dmPelsWidth == registry_mode.dmPelsWidth
                && devmode.dmPelsHeight == registry_mode.dmPelsHeight, "Got unexpected screen size %ux%u.\n",
                devmode.dmPelsWidth, devmode.dmPelsHeight);

        hr = reset_device(device, &device_desc);
        ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);

        /* Remove the WS_VISIBLE flag to test hidden windows. This is enough to trigger d3d's hidden
         * window codepath, but does not actually hide the window without a SetWindowPos(SWP_FRAMECHANGED)
         * call. This way we avoid focus changes and random failures on focus follows mouse WMs. */
        device_style = GetWindowLongA(device_window, GWL_STYLE);
        SetWindowLongA(device_window, GWL_STYLE, device_style & ~WS_VISIBLE);
        flush_events();

        expect_messages = focus_loss_messages_hidden;
        windowposchanged_received = 0;
        SetForegroundWindow(GetDesktopWindow());
        ok(!expect_messages->message, "Expected message %#x for window %#x, but didn't receive it, i=%u.\n",
                expect_messages->message, expect_messages->window, i);
        ok(!windowposchanged_received, "Received WM_WINDOWPOSCHANGED but did not expect it.\n");
        expect_messages = NULL;
        flush_events();

        ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devmode);
        ok(ret, "Failed to get display mode.\n");
        ok(devmode.dmPelsWidth == registry_mode.dmPelsWidth, "Got unexpected width %u.\n", devmode.dmPelsWidth);
        ok(devmode.dmPelsHeight == registry_mode.dmPelsHeight, "Got unexpected height %u.\n", devmode.dmPelsHeight);

        /* SW_SHOWMINNOACTIVE is needed to make FVWM happy. SW_SHOWNOACTIVATE is needed to make windows
         * send SIZE_RESTORED after ShowWindow(SW_SHOWMINNOACTIVE). */
        ShowWindow(focus_window, SW_SHOWNOACTIVATE);
        ShowWindow(focus_window, SW_SHOWMINNOACTIVE);
        flush_events();

        syscommand_received = 0;
        expect_messages = sc_restore_messages;
        SendMessageA(focus_window, WM_SYSCOMMAND, SC_RESTORE, 0);
        ok(!expect_messages->message, "Expected message %#x for window %#x, but didn't receive it, i=%u.\n",
                expect_messages->message, expect_messages->window, i);
        ok(syscommand_received == 1, "Got unexpected number of WM_SYSCOMMAND messages: %d.\n", syscommand_received);
        expect_messages = NULL;
        flush_events();

        expect_messages = sc_minimize_messages;
        SendMessageA(focus_window, WM_SYSCOMMAND, SC_MINIMIZE, 0);
        ok(!expect_messages->message, "Expected message %#x for window %#x, but didn't receive it, i=%u.\n",
                expect_messages->message, expect_messages->window, i);
        expect_messages = NULL;
        flush_events();

        expect_messages = sc_maximize_messages;
        SendMessageA(focus_window, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
        ok(!expect_messages->message, "Expected message %#x for window %#x, but didn't receive it, i=%u.\n",
                expect_messages->message, expect_messages->window, i);
        expect_messages = NULL;
        flush_events();

        SetForegroundWindow(GetDesktopWindow());
        ShowWindow(device_window, SW_MINIMIZE);
        ShowWindow(device_window, SW_RESTORE);
        ShowWindow(focus_window, SW_MINIMIZE);
        ShowWindow(focus_window, SW_RESTORE);
        SetForegroundWindow(focus_window);
        flush_events();

        /* Releasing a device in lost state breaks follow-up tests on native. */
        hr = IDirect3DDevice9_TestCooperativeLevel(device);
        if (hr == D3DERR_DEVICENOTRESET)
        {
            hr = reset_device(device, &device_desc);
            ok(SUCCEEDED(hr), "Failed to reset device, hr %#x, i=%u.\n", hr, i);
        }

        filter_messages = focus_window;

        ref = IDirect3DDevice9_Release(device);
        ok(ref == 0, "The device was not properly freed: refcount %u, i=%u.\n", ref, i);

        /* Fix up the mode until Wine's device release behavior is fixed. */
        change_ret = ChangeDisplaySettingsW(NULL, CDS_FULLSCREEN);
        ok(change_ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#x.\n", change_ret);

        proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
        ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx, i=%u.\n",
                (LONG_PTR)test_proc, proc, i);

        /* Hide the device window. It prevents WM_ACTIVATEAPP messages from being sent
         * on native in the test below. It isn't needed anyways. Creating the third
         * device will show it again. */
        filter_messages = NULL;
        ShowWindow(device_window, SW_HIDE);
        /* Remove the maximized state from the SYSCOMMAND test while we're not
         * interfering with a device. */
        ShowWindow(focus_window, SW_SHOWNORMAL);
        filter_messages = focus_window;

        device_desc.device_window = focus_window;
        if (!(device = create_device(d3d9, focus_window, &device_desc)))
        {
            skip("Failed to create a D3D device, skipping tests.\n");
            goto done;
        }
        filter_messages = NULL;
        SetForegroundWindow(focus_window); /* For KDE. */
        flush_events();

        expect_messages = focus_loss_messages_filtered;
        windowposchanged_received = 0;
        SetForegroundWindow(GetDesktopWindow());
        ok(!expect_messages->message, "Expected message %#x for window %#x, but didn't receive it, i=%u.\n",
                expect_messages->message, expect_messages->window, i);
        ok(!windowposchanged_received, "Received WM_WINDOWPOSCHANGED but did not expect it, i=%u.\n", i);
        expect_messages = NULL;

        /* The window is iconic even though no message was sent. */
        ok(!IsIconic(focus_window) == !tests[i].iconic,
                "Expected IsIconic %u, got %u, i=%u.\n", tests[i].iconic, IsIconic(focus_window), i);

        hr = IDirect3DDevice9_TestCooperativeLevel(device);
        ok(hr == D3DERR_DEVICELOST, "Got unexpected hr %#x.\n", hr);

        if (tests[i].create_flags & CREATE_DEVICE_NOWINDOWCHANGES)
            ShowWindow(focus_window, SW_MINIMIZE);

        syscommand_received = 0;
        expect_messages = sc_restore_messages;
        SendMessageA(focus_window, WM_SYSCOMMAND, SC_RESTORE, 0);
        ok(!expect_messages->message, "Expected message %#x for window %#x, but didn't receive it, i=%u.\n",
                expect_messages->message, expect_messages->window, i);
        ok(syscommand_received == 1, "Got unexpected number of WM_SYSCOMMAND messages: %d.\n", syscommand_received);
        expect_messages = NULL;
        flush_events();

        /* For FVWM. */
        ShowWindow(focus_window, SW_RESTORE);
        flush_events();

        expect_messages = sc_minimize_messages;
        SendMessageA(focus_window, WM_SYSCOMMAND, SC_MINIMIZE, 0);
        ok(!expect_messages->message, "Expected message %#x for window %#x, but didn't receive it, i=%u.\n",
                expect_messages->message, expect_messages->window, i);
        expect_messages = NULL;
        flush_events();

        expect_messages = sc_maximize_messages;
        SendMessageA(focus_window, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
        ok(!expect_messages->message, "Expected message %#x for window %#x, but didn't receive it, i=%u.\n",
                expect_messages->message, expect_messages->window, i);
        expect_messages = NULL;
        flush_events();

        /* Make sure the SetWindowPos call done by d3d9 is not a no-op. */
        SetWindowPos(focus_window, NULL, 10, 10, 100, 100, SWP_NOZORDER | SWP_NOACTIVATE);
        SetForegroundWindow(GetDesktopWindow());
        flush_events();
        SetForegroundWindow(GetDesktopWindow()); /* For FVWM. */
        flush_events();

        expect_messages = reactivate_messages_filtered;
        windowposchanged_received = 0;
        SetForegroundWindow(focus_window);
        flush_events();
        ok(!expect_messages->message, "Expected message %#x for window %#x, but didn't receive it, i=%u.\n",
                expect_messages->message, expect_messages->window, i);
        /* About 1 in 8 test runs receives WM_WINDOWPOSCHANGED on Vista. */
        ok(!windowposchanged_received || broken(1),
                "Received WM_WINDOWPOSCHANGED but did not expect it, i=%u.\n", i);
        expect_messages = NULL;

        /* On Windows 10 style change messages are delivered both on reset and
         * on release. */
        hr = IDirect3DDevice9_TestCooperativeLevel(device);
        ok(hr == D3DERR_DEVICENOTRESET, "Got unexpected hr %#x.\n", hr);

        hr = reset_device(device, &device_desc);
        ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);

        ref = IDirect3DDevice9_Release(device);
        ok(ref == 0, "The device was not properly freed: refcount %u, i=%u.\n", ref, i);

        filter_messages = focus_window;
        device_desc.device_window = device_window;
        if (!(device = create_device(d3d9, focus_window, &device_desc)))
        {
            skip("Failed to create a D3D device, skipping tests.\n");
            goto done;
        }
        filter_messages = NULL;
        flush_events();

        device_desc.width = user32_width;
        device_desc.height = user32_height;

        expect_messages = tests[i].mode_change_messages;
        filter_messages = focus_window;
        hr = reset_device(device, &device_desc);
        ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);
        filter_messages = NULL;

        /* The WINDOWPOS structure passed to the first WM_WINDOWPOSCHANGING differs between windows versions.
         * Prior to Win10 17.03 it is consistent with a MoveWindow(0, 0, width, height) call. Since Windows
         * 10 17.03 it has x = 0, y = 0, width = 0, height = 0, flags = SWP_NOCLIENTMOVE | SWP_NOCLIENTSIZE
         * | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER (0x1837). Visually
         * it is clear that the window has not been resized. In previous Windows version the window is resized. */

        flush_events();
        ok(!expect_messages->message, "Expected message %#x for window %#x, but didn't receive it, i=%u.\n",
                expect_messages->message, expect_messages->window, i);
        expect_messages = NULL;

        /* World of Warplanes hides the window by removing WS_VISIBLE and expects Reset() to show it again. */
        device_style = GetWindowLongA(device_window, GWL_STYLE);
        SetWindowLongA(device_window, GWL_STYLE, device_style & ~WS_VISIBLE);

        flush_events();
        device_desc.width = d3d_width;
        device_desc.height = d3d_height;
        memset(&windowpos, 0, sizeof(windowpos));

        expect_messages = tests[i].mode_change_messages_hidden;
        filter_messages = focus_window;
        hr = reset_device(device, &device_desc);
        ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);
        filter_messages = NULL;

        flush_events();
        ok(!expect_messages->message, "Expected message %#x for window %#x, but didn't receive it, i=%u.\n",
                expect_messages->message, expect_messages->window, i);
        expect_messages = NULL;

        if (!(tests[i].create_flags & CREATE_DEVICE_NOWINDOWCHANGES))
        {
            ok(windowpos.hwnd == device_window && !windowpos.hwndInsertAfter
                    && !windowpos.x && !windowpos.y && !windowpos.cx && !windowpos.cy
                    && windowpos.flags == (SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE),
                    "Got unexpected WINDOWPOS hwnd=%p, insertAfter=%p, x=%d, y=%d, cx=%d, cy=%d, flags=%x\n",
                    windowpos.hwnd, windowpos.hwndInsertAfter, windowpos.x, windowpos.y, windowpos.cx,
                    windowpos.cy, windowpos.flags);
        }

        device_style = GetWindowLongA(device_window, GWL_STYLE);
        if (tests[i].create_flags & CREATE_DEVICE_NOWINDOWCHANGES)
        {
            todo_wine ok(!(device_style & WS_VISIBLE), "Expected the device window to be hidden, i=%u.\n", i);
            ShowWindow(device_window, SW_MINIMIZE);
            ShowWindow(device_window, SW_RESTORE);
        }
        else
        {
            ok(device_style & WS_VISIBLE, "Expected the device window to be visible, i=%u.\n", i);
        }

        proc = SetWindowLongPtrA(focus_window, GWLP_WNDPROC, (LONG_PTR)DefWindowProcA);
        ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#lx, i=%u.\n",
                (LONG_PTR)test_proc, i);

        filter_messages = focus_window;
        ref = IDirect3DDevice9_Release(device);
        ok(ref == 0, "The device was not properly freed: refcount %u, i=%u.\n", ref, i);

        proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
        ok(proc == (LONG_PTR)DefWindowProcA, "Expected wndproc %#lx, got %#lx, i=%u.\n",
                (LONG_PTR)DefWindowProcA, proc, i);

done:
        filter_messages = NULL;
        expect_messages = NULL;
        DestroyWindow(device_window);
        DestroyWindow(focus_window);
        SetEvent(thread_params.test_finished);
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
    }

    IDirect3D9_Release(d3d9);
    CloseHandle(thread_params.test_finished);
    CloseHandle(thread_params.window_created);
    UnregisterClassA("d3d9_test_wndproc_wc", GetModuleHandleA(NULL));
    change_ret = ChangeDisplaySettingsExW(NULL, NULL, NULL, 0, NULL);
    ok(change_ret == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExW failed with %d.\n", change_ret);
}

static void test_wndproc_windowed(void)
{
    struct wndproc_thread_param thread_params;
    struct device_desc device_desc;
    IDirect3DDevice9 *device;
    WNDCLASSA wc = {0};
    IDirect3D9 *d3d9;
    HANDLE thread;
    LONG_PTR proc;
    HRESULT hr;
    ULONG ref;
    DWORD res, tid;
    HWND tmp;

    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");

    filter_messages = NULL;
    expect_messages = NULL;

    wc.lpfnWndProc = test_proc;
    wc.lpszClassName = "d3d9_test_wndproc_wc";
    ok(RegisterClassA(&wc), "Failed to register window class.\n");

    thread_params.window_created = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(!!thread_params.window_created, "CreateEvent failed, last error %#x.\n", GetLastError());
    thread_params.test_finished = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(!!thread_params.test_finished, "CreateEvent failed, last error %#x.\n", GetLastError());

    focus_window = CreateWindowA("d3d9_test_wndproc_wc", "d3d9_test",
            WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION, 0, 0, registry_mode.dmPelsWidth,
            registry_mode.dmPelsHeight, 0, 0, 0, 0);
    device_window = CreateWindowA("d3d9_test_wndproc_wc", "d3d9_test",
            WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION, 0, 0, registry_mode.dmPelsWidth,
            registry_mode.dmPelsHeight, 0, 0, 0, 0);
    thread = CreateThread(NULL, 0, wndproc_thread, &thread_params, 0, &tid);
    ok(!!thread, "Failed to create thread, last error %#x.\n", GetLastError());

    res = WaitForSingleObject(thread_params.window_created, INFINITE);
    ok(res == WAIT_OBJECT_0, "Wait failed (%#x), last error %#x.\n", res, GetLastError());

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    trace("device_window %p, focus_window %p, dummy_window %p.\n",
            device_window, focus_window, thread_params.dummy_window);

    tmp = GetFocus();
    ok(tmp == device_window, "Expected focus %p, got %p.\n", device_window, tmp);
    if (thread_params.running_in_foreground)
    {
        tmp = GetForegroundWindow();
        ok(tmp == thread_params.dummy_window, "Expected foreground window %p, got %p.\n",
                thread_params.dummy_window, tmp);
    }
    else
        skip("Not running in foreground, skip foreground window test\n");

    filter_messages = focus_window;

    device_desc.adapter_ordinal = D3DADAPTER_DEFAULT;
    device_desc.device_window = device_window;
    device_desc.width = 640;
    device_desc.height = 480;
    device_desc.flags = 0;
    if (!(device = create_device(d3d9, focus_window, &device_desc)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    tmp = GetFocus();
    ok(tmp == device_window, "Expected focus %p, got %p.\n", device_window, tmp);
    tmp = GetForegroundWindow();
    ok(tmp == thread_params.dummy_window, "Expected foreground window %p, got %p.\n",
            thread_params.dummy_window, tmp);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    filter_messages = NULL;

    device_desc.width = registry_mode.dmPelsWidth;
    device_desc.height = registry_mode.dmPelsHeight;
    device_desc.flags = CREATE_DEVICE_FULLSCREEN;
    hr = reset_device(device, &device_desc);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#lx.\n", (LONG_PTR)test_proc);

    device_desc.flags = 0;
    hr = reset_device(device, &device_desc);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    filter_messages = focus_window;

    ref = IDirect3DDevice9_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

    filter_messages = device_window;

    device_desc.device_window = focus_window;
    if (!(device = create_device(d3d9, focus_window, &device_desc)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    filter_messages = NULL;

    device_desc.flags = CREATE_DEVICE_FULLSCREEN;
    hr = reset_device(device, &device_desc);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#lx.\n", (LONG_PTR)test_proc);

    device_desc.flags = 0;
    hr = reset_device(device, &device_desc);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    filter_messages = device_window;

    ref = IDirect3DDevice9_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

    device_desc.device_window = device_window;
    if (!(device = create_device(d3d9, focus_window, &device_desc)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    filter_messages = NULL;

    device_desc.device_window = device_window;
    device_desc.flags = CREATE_DEVICE_FULLSCREEN;
    hr = reset_device(device, &device_desc);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#lx.\n", (LONG_PTR)test_proc);

    device_desc.flags = 0;
    hr = reset_device(device, &device_desc);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    filter_messages = device_window;

    ref = IDirect3DDevice9_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

done:
    filter_messages = NULL;
    IDirect3D9_Release(d3d9);

    SetEvent(thread_params.test_finished);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread_params.test_finished);
    CloseHandle(thread_params.window_created);
    CloseHandle(thread);

    DestroyWindow(device_window);
    DestroyWindow(focus_window);
    UnregisterClassA("d3d9_test_wndproc_wc", GetModuleHandleA(NULL));
}

static void test_reset_fullscreen(void)
{
    struct device_desc device_desc;
    D3DDISPLAYMODE d3ddm, d3ddm2;
    unsigned int mode_count, i;
    IDirect3DDevice9 *device;
    WNDCLASSEXA wc = {0};
    IDirect3D9 *d3d;
    HRESULT hr;
    ATOM atom;
    static const struct message messages[] =
    {
        /* Windows usually sends wparam = TRUE, except on the testbot,
         * where it randomly sends FALSE. Ignore it. */
        {WM_ACTIVATEAPP,    FOCUS_WINDOW,   FALSE,  0},
        {0,                 0,              FALSE,  0},
    };

    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    filter_messages = NULL;
    expect_messages = messages;

    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = test_proc;
    wc.lpszClassName = "test_reset_fullscreen";

    atom = RegisterClassExA(&wc);
    ok(atom, "Failed to register a new window class. GetLastError:%d\n", GetLastError());

    device_window = focus_window = CreateWindowExA(0, wc.lpszClassName, "Test Reset Fullscreen", 0,
            0, 0, registry_mode.dmPelsWidth, registry_mode.dmPelsHeight, NULL, NULL, NULL, NULL);
    ok(device_window != NULL, "Failed to create a window. GetLastError:%d\n", GetLastError());

    /*
     * Create a device in windowed mode.
     * Since the device is windowed and we haven't called any methods that
     * could show the window (such as ShowWindow or SetWindowPos) yet,
     * WM_ACTIVATEAPP will not have been sent.
     */
    if (!(device = create_device(d3d, device_window, NULL)))
    {
        skip("Unable to create device. Skipping test.\n");
        goto cleanup;
    }

    /*
     * Switch to fullscreen mode.
     * This will force the window to be shown and will cause the WM_ACTIVATEAPP
     * message to be sent.
     */
    device_desc.width = registry_mode.dmPelsWidth;
    device_desc.height = registry_mode.dmPelsHeight;
    device_desc.device_window = device_window;
    device_desc.flags = CREATE_DEVICE_FULLSCREEN;
    ok(SUCCEEDED(reset_device(device, &device_desc)), "Failed to reset device.\n");

    flush_events();
    ok(expect_messages->message == 0, "Expected to receive message %#x.\n", expect_messages->message);
    expect_messages = NULL;

    IDirect3D9_GetAdapterDisplayMode(d3d, D3DADAPTER_DEFAULT, &d3ddm);
    mode_count = IDirect3D9_GetAdapterModeCount(d3d, D3DADAPTER_DEFAULT, d3ddm.Format);
    for (i = 0; i < mode_count; ++i)
    {
        hr = IDirect3D9_EnumAdapterModes(d3d, D3DADAPTER_DEFAULT, d3ddm.Format, i, &d3ddm2);
        ok(SUCCEEDED(hr), "Failed to enumerate display mode, hr %#x.\n", hr);

        if (d3ddm2.Width != d3ddm.Width || d3ddm2.Height != d3ddm.Height)
            break;
    }
    if (i == mode_count)
    {
        skip("Could not find a suitable display mode.\n");
        goto cleanup;
    }

    wm_size_received = 0;

    /* Fullscreen mode change. */
    device_desc.width = d3ddm2.Width;
    device_desc.height = d3ddm2.Height;
    device_desc.device_window = device_window;
    device_desc.flags = CREATE_DEVICE_FULLSCREEN;
    ok(SUCCEEDED(reset_device(device, &device_desc)), "Failed to reset device.\n");

    flush_events();
    ok(!wm_size_received, "Received unexpected WM_SIZE message.\n");

cleanup:
    if (device) IDirect3DDevice9_Release(device);
    IDirect3D9_Release(d3d);
    DestroyWindow(device_window);
    device_window = focus_window = NULL;
    UnregisterClassA(wc.lpszClassName, GetModuleHandleA(NULL));
}


static inline void set_fpu_cw(WORD cw)
{
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define D3D9_TEST_SET_FPU_CW 1
    __asm__ volatile ("fnclex");
    __asm__ volatile ("fldcw %0" : : "m" (cw));
#elif defined(__i386__) && defined(_MSC_VER)
#define D3D9_TEST_SET_FPU_CW 1
    __asm fnclex;
    __asm fldcw cw;
#endif
}

static inline WORD get_fpu_cw(void)
{
    WORD cw = 0;
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define D3D9_TEST_GET_FPU_CW 1
    __asm__ volatile ("fnstcw %0" : "=m" (cw));
#elif defined(__i386__) && defined(_MSC_VER)
#define D3D9_TEST_GET_FPU_CW 1
    __asm fnstcw cw;
#endif
    return cw;
}

static WORD callback_cw, callback_set_cw;
static DWORD callback_tid;

static HRESULT WINAPI dummy_object_QueryInterface(IUnknown *iface, REFIID riid, void **out)
{
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI dummy_object_AddRef(IUnknown *iface)
{
    callback_cw = get_fpu_cw();
    set_fpu_cw(callback_set_cw);
    callback_tid = GetCurrentThreadId();
    return 2;
}

static ULONG WINAPI dummy_object_Release(IUnknown *iface)
{
    callback_cw = get_fpu_cw();
    set_fpu_cw(callback_set_cw);
    callback_tid = GetCurrentThreadId();
    return 1;
}

static const IUnknownVtbl dummy_object_vtbl =
{
    dummy_object_QueryInterface,
    dummy_object_AddRef,
    dummy_object_Release,
};

static const GUID d3d9_private_data_test_guid =
{
    0xfdb37466,
    0x428f,
    0x4edf,
    {0xa3,0x7f,0x9b,0x1d,0xf4,0x88,0xc5,0xfc}
};

static void test_fpu_setup(void)
{
#if defined(D3D9_TEST_SET_FPU_CW) && defined(D3D9_TEST_GET_FPU_CW)
    static const BOOL is_64bit = sizeof(void *) > sizeof(int);
    IUnknown dummy_object = {&dummy_object_vtbl};
    struct device_desc device_desc;
    IDirect3DSurface9 *surface;
    IDirect3DDevice9 *device;
    WORD cw, expected_cw;
    HWND window = NULL;
    IDirect3D9 *d3d9;
    HRESULT hr;

    window = create_window();
    ok(!!window, "Failed to create a window.\n");
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");

    device_desc.adapter_ordinal = D3DADAPTER_DEFAULT;
    device_desc.device_window = window;
    device_desc.width = 640;
    device_desc.height = 480;
    device_desc.flags = 0;

    set_fpu_cw(0xf60);
    cw = get_fpu_cw();
    ok(cw == 0xf60, "cw is %#x, expected 0xf60.\n", cw);

    if (!(device = create_device(d3d9, window, &device_desc)))
    {
        skip("Failed to create a 3D device, skipping test.\n");
        set_fpu_cw(0x37f);
        goto done;
    }

    expected_cw = is_64bit ? 0xf60 : 0x7f;

    cw = get_fpu_cw();
    todo_wine_if(is_64bit)
    ok(cw == expected_cw, "cw is %#x, expected %#x.\n", cw, expected_cw);

    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &surface);
    ok(SUCCEEDED(hr), "Failed to get render target surface, hr %#x.\n", hr);

    callback_set_cw = 0xf60;
    hr = IDirect3DSurface9_SetPrivateData(surface, &d3d9_private_data_test_guid,
            &dummy_object, sizeof(IUnknown *), D3DSPD_IUNKNOWN);
    ok(SUCCEEDED(hr), "Failed to set private data, hr %#x.\n", hr);
    todo_wine_if(is_64bit)
    ok(callback_cw == expected_cw, "Callback cw is %#x, expected %#x.\n", callback_cw, expected_cw);
    ok(callback_tid == GetCurrentThreadId(), "Got unexpected thread id.\n");
    cw = get_fpu_cw();
    ok(cw == 0xf60, "cw is %#x, expected 0xf60.\n", cw);

    callback_cw = 0;
    hr = IDirect3DSurface9_SetPrivateData(surface, &d3d9_private_data_test_guid,
            &dummy_object, sizeof(IUnknown *), D3DSPD_IUNKNOWN);
    ok(SUCCEEDED(hr), "Failed to set private data, hr %#x.\n", hr);
    ok(callback_cw == 0xf60, "Callback cw is %#x, expected 0xf60.\n", callback_cw);
    ok(callback_tid == GetCurrentThreadId(), "Got unexpected thread id.\n");

    callback_set_cw = 0x7f;
    set_fpu_cw(0x7f);

    IDirect3DSurface9_Release(surface);

    callback_cw = 0;
    IDirect3DDevice9_Release(device);
    ok(callback_cw == 0x7f, "Callback cw is %#x, expected 0x7f.\n", callback_cw);
    ok(callback_tid == GetCurrentThreadId(), "Got unexpected thread id.\n");

    cw = get_fpu_cw();
    ok(cw == 0x7f, "cw is %#x, expected 0x7f.\n", cw);
    set_fpu_cw(0xf60);
    cw = get_fpu_cw();
    ok(cw == 0xf60, "cw is %#x, expected 0xf60.\n", cw);

    device_desc.flags = CREATE_DEVICE_FPU_PRESERVE;
    device = create_device(d3d9, window, &device_desc);
    ok(device != NULL, "CreateDevice failed.\n");

    cw = get_fpu_cw();
    ok(cw == 0xf60, "cw is %#x, expected 0xf60.\n", cw);

    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &surface);
    ok(SUCCEEDED(hr), "Failed to get render target surface, hr %#x.\n", hr);

    callback_cw = 0;
    callback_set_cw = 0x37f;
    hr = IDirect3DSurface9_SetPrivateData(surface, &d3d9_private_data_test_guid,
            &dummy_object, sizeof(IUnknown *), D3DSPD_IUNKNOWN);
    ok(SUCCEEDED(hr), "Failed to set private data, hr %#x.\n", hr);
    ok(callback_cw == 0xf60, "Callback cw is %#x, expected 0xf60.\n", callback_cw);
    ok(callback_tid == GetCurrentThreadId(), "Got unexpected thread id.\n");
    cw = get_fpu_cw();
    ok(cw == 0x37f, "cw is %#x, expected 0x37f.\n", cw);

    IDirect3DSurface9_Release(surface);

    callback_cw = 0;
    IDirect3DDevice9_Release(device);
    ok(callback_cw == 0x37f, "Callback cw is %#x, expected 0xf60.\n", callback_cw);
    ok(callback_tid == GetCurrentThreadId(), "Got unexpected thread id.\n");

done:
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
#endif
}

static void test_window_style(void)
{
    RECT focus_rect, device_rect, fullscreen_rect, r, r2;
    LONG device_style, device_exstyle;
    LONG focus_style, focus_exstyle;
    struct device_desc device_desc;
    LONG style, expected_style;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d9;
    HRESULT hr;
    ULONG ref;
    BOOL ret;
    static const struct
    {
        DWORD device_flags;
        LONG create_style, style, focus_loss_style, exstyle, focus_loss_exstyle;
    }
    tests[] =
    {
        {0,                                 0,          WS_VISIBLE, WS_MINIMIZE,    WS_EX_TOPMOST, WS_EX_TOPMOST},
        {0,                                 WS_VISIBLE, WS_VISIBLE, WS_MINIMIZE,    0,             WS_EX_TOPMOST},
        {CREATE_DEVICE_NOWINDOWCHANGES,     0,          0,          0,              0,             0},
        {CREATE_DEVICE_NOWINDOWCHANGES,     WS_VISIBLE, WS_VISIBLE, 0,              0,             0},
    };
    unsigned int i;

    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");
    SetRect(&fullscreen_rect, 0, 0, registry_mode.dmPelsWidth, registry_mode.dmPelsHeight);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        focus_window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW | tests[i].create_style,
                0, 0, registry_mode.dmPelsWidth / 2, registry_mode.dmPelsHeight / 2, 0, 0, 0, 0);
        device_window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW | tests[i].create_style,
                0, 0, registry_mode.dmPelsWidth / 2, registry_mode.dmPelsHeight / 2, 0, 0, 0, 0);

        device_style = GetWindowLongA(device_window, GWL_STYLE);
        device_exstyle = GetWindowLongA(device_window, GWL_EXSTYLE);
        focus_style = GetWindowLongA(focus_window, GWL_STYLE);
        focus_exstyle = GetWindowLongA(focus_window, GWL_EXSTYLE);

        GetWindowRect(focus_window, &focus_rect);
        GetWindowRect(device_window, &device_rect);

        device_desc.adapter_ordinal = D3DADAPTER_DEFAULT;
        device_desc.device_window = device_window;
        device_desc.width = registry_mode.dmPelsWidth;
        device_desc.height = registry_mode.dmPelsHeight;
        device_desc.flags = CREATE_DEVICE_FULLSCREEN | tests[i].device_flags;
        if (!(device = create_device(d3d9, focus_window, &device_desc)))
        {
            skip("Failed to create a D3D device, skipping tests.\n");
            DestroyWindow(device_window);
            DestroyWindow(focus_window);
            break;
        }

        style = GetWindowLongA(device_window, GWL_STYLE);
        expected_style = device_style | tests[i].style;
        todo_wine ok(style == expected_style || broken(style == (expected_style & ~WS_OVERLAPPEDWINDOW)) /* w1064v1809 */,
                "Expected device window style %#x, got %#x, i=%u.\n",
                expected_style, style, i);
        style = GetWindowLongA(device_window, GWL_EXSTYLE);
        expected_style = device_exstyle | tests[i].exstyle;
        todo_wine ok(style == expected_style || broken(style == (expected_style & ~WS_EX_OVERLAPPEDWINDOW)) /* w1064v1809 */,
                "Expected device window extended style %#x, got %#x, i=%u.\n",
                expected_style, style, i);

        style = GetWindowLongA(focus_window, GWL_STYLE);
        ok(style == focus_style, "Expected focus window style %#x, got %#x, i=%u.\n",
                focus_style, style, i);
        style = GetWindowLongA(focus_window, GWL_EXSTYLE);
        ok(style == focus_exstyle, "Expected focus window extended style %#x, got %#x, i=%u.\n",
                focus_exstyle, style, i);

        GetWindowRect(device_window, &r);
        if (tests[i].device_flags & CREATE_DEVICE_NOWINDOWCHANGES)
            todo_wine ok(EqualRect(&r, &device_rect), "Expected %s, got %s, i=%u.\n",
                    wine_dbgstr_rect(&device_rect), wine_dbgstr_rect(&r), i);
        else
            ok(EqualRect(&r, &fullscreen_rect), "Expected %s, got %s, i=%u.\n",
                    wine_dbgstr_rect(&fullscreen_rect), wine_dbgstr_rect(&r), i);
        GetClientRect(device_window, &r2);
        todo_wine ok(!EqualRect(&r, &r2) || broken(!(style & WS_THICKFRAME)) /* w1064v1809 */,
                "Client rect and window rect are equal, i=%u.\n", i);
        GetWindowRect(focus_window, &r);
        ok(EqualRect(&r, &focus_rect), "Expected %s, got %s, i=%u.\n",
                wine_dbgstr_rect(&focus_rect), wine_dbgstr_rect(&r), i);

        device_desc.flags = 0;
        hr = reset_device(device, &device_desc);
        ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);

        style = GetWindowLongA(device_window, GWL_STYLE);
        expected_style = device_style | tests[i].style;
        ok(style == expected_style, "Expected device window style %#x, got %#x, i=%u.\n",
                expected_style, style, i);
        style = GetWindowLongA(device_window, GWL_EXSTYLE);
        expected_style = device_exstyle | tests[i].exstyle;
        todo_wine_if (!(tests[i].device_flags & CREATE_DEVICE_NOWINDOWCHANGES) && (tests[i].create_style & WS_VISIBLE))
            ok(style == expected_style, "Expected device window extended style %#x, got %#x, i=%u.\n",
                    expected_style, style, i);

        style = GetWindowLongA(focus_window, GWL_STYLE);
        ok(style == focus_style, "Expected focus window style %#x, got %#x, i=%u.\n",
                focus_style, style, i);
        style = GetWindowLongA(focus_window, GWL_EXSTYLE);
        ok(style == focus_exstyle, "Expected focus window extended style %#x, got %#x, i=%u.\n",
                focus_exstyle, style, i);

        device_desc.flags = CREATE_DEVICE_FULLSCREEN;
        hr = reset_device(device, &device_desc);
        ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);
        ret = SetForegroundWindow(GetDesktopWindow());
        ok(ret, "Failed to set foreground window.\n");

        style = GetWindowLongA(device_window, GWL_STYLE);
        expected_style = device_style | tests[i].focus_loss_style | tests[i].style;
        todo_wine ok(style == expected_style, "Expected device window style %#x, got %#x, i=%u.\n",
                expected_style, style, i);
        style = GetWindowLongA(device_window, GWL_EXSTYLE);
        expected_style = device_exstyle | tests[i].focus_loss_exstyle | tests[i].exstyle;
        todo_wine ok(style == expected_style, "Expected device window extended style %#x, got %#x, i=%u.\n",
                expected_style, style, i);

        style = GetWindowLongA(focus_window, GWL_STYLE);
        ok(style == focus_style, "Expected focus window style %#x, got %#x, i=%u.\n",
                focus_style, style, i);
        style = GetWindowLongA(focus_window, GWL_EXSTYLE);
        ok(style == focus_exstyle, "Expected focus window extended style %#x, got %#x, i=%u.\n",
                focus_exstyle, style, i);

        /* In d3d8 follow-up tests fail on native if the device is destroyed while
         * lost. This doesn't happen in d3d9 on my test machine but it still seems
         * like a good idea to reset it first. */
        ShowWindow(focus_window, SW_MINIMIZE);
        ShowWindow(focus_window, SW_RESTORE);
        ret = SetForegroundWindow(focus_window);
        ok(ret, "Failed to set foreground window.\n");
        flush_events();
        hr = reset_device(device, &device_desc);
        ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);

        ref = IDirect3DDevice9_Release(device);
        ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

        DestroyWindow(device_window);
        DestroyWindow(focus_window);
    }
    IDirect3D9_Release(d3d9);
}

static const POINT *expect_pos;

static LRESULT CALLBACK test_cursor_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (message == WM_MOUSEMOVE)
    {
        if (expect_pos && expect_pos->x && expect_pos->y)
        {
            POINT p = {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};

            ClientToScreen(window, &p);
            if (expect_pos->x == p.x && expect_pos->y == p.y)
                ++expect_pos;
        }
    }

    return DefWindowProcA(window, message, wparam, lparam);
}

static void test_cursor_pos(void)
{
    IDirect3DSurface9 *cursor;
    IDirect3DDevice9 *device;
    WNDCLASSA wc = {0};
    IDirect3D9 *d3d9;
    UINT refcount;
    HWND window;
    HRESULT hr;
    BOOL ret;

    /* Note that we don't check for movement we're not supposed to receive.
     * That's because it's hard to distinguish from the user accidentally
     * moving the mouse. */
    static const POINT points[] =
    {
        {50, 50},
        {75, 75},
        {100, 100},
        {125, 125},
        {150, 150},
        {125, 125},
        {150, 150},
        {150, 150},
        {0, 0},
    };

    wc.lpfnWndProc = test_cursor_proc;
    wc.lpszClassName = "d3d9_test_cursor_wc";
    ok(RegisterClassA(&wc), "Failed to register window class.\n");
    window = CreateWindowA("d3d9_test_cursor_wc", "d3d9_test", WS_POPUP | WS_SYSMENU,
            0, 0, 320, 240, NULL, NULL, NULL, NULL);
    ShowWindow(window, SW_SHOW);
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");

    device = create_device(d3d9, window, NULL);
    if (!device)
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 32, 32,
            D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &cursor, NULL);
    ok(SUCCEEDED(hr), "Failed to create cursor surface, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetCursorProperties(device, 0, 0, cursor);
    ok(SUCCEEDED(hr), "Failed to set cursor properties, hr %#x.\n", hr);
    IDirect3DSurface9_Release(cursor);
    ret = IDirect3DDevice9_ShowCursor(device, TRUE);
    ok(!ret, "Failed to show cursor, hr %#x.\n", ret);

    flush_events();
    expect_pos = points;

    ret = SetCursorPos(50, 50);
    ok(ret, "Failed to set cursor position.\n");
    flush_events();

    IDirect3DDevice9_SetCursorPosition(device, 75, 75, 0);
    flush_events();
    /* SetCursorPosition() eats duplicates. */
    IDirect3DDevice9_SetCursorPosition(device, 75, 75, 0);
    flush_events();

    ret = SetCursorPos(100, 100);
    ok(ret, "Failed to set cursor position.\n");
    flush_events();
    /* Even if the position was set with SetCursorPos(). */
    IDirect3DDevice9_SetCursorPosition(device, 100, 100, 0);
    flush_events();

    IDirect3DDevice9_SetCursorPosition(device, 125, 125, 0);
    flush_events();
    ret = SetCursorPos(150, 150);
    ok(ret, "Failed to set cursor position.\n");
    flush_events();
    IDirect3DDevice9_SetCursorPosition(device, 125, 125, 0);
    flush_events();

    IDirect3DDevice9_SetCursorPosition(device, 150, 150, 0);
    flush_events();
    /* SetCursorPos() doesn't. */
    ret = SetCursorPos(150, 150);
    ok(ret, "Failed to set cursor position.\n");
    flush_events();

    ok(!expect_pos->x && !expect_pos->y, "Didn't receive MOUSEMOVE %u (%d, %d).\n",
       (unsigned)(expect_pos - points), expect_pos->x, expect_pos->y);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    DestroyWindow(window);
    UnregisterClassA("d3d9_test_cursor_wc", GetModuleHandleA(NULL));
    IDirect3D9_Release(d3d9);
}

static void test_mode_change(void)
{
    DEVMODEW old_devmode, devmode, devmode2, *original_modes = NULL;
    struct device_desc device_desc, device_desc2;
    WCHAR second_monitor_name[CCHDEVICENAME];
    IDirect3DDevice9 *device, *device2;
    unsigned int display_count = 0;
    RECT d3d_rect, focus_rect, r;
    IDirect3DSurface9 *backbuffer;
    MONITORINFOEXW monitor_info;
    HMONITOR second_monitor;
    D3DSURFACE_DESC desc;
    IDirect3D9 *d3d9;
    ULONG refcount;
    UINT adapter_mode_count, i;
    HRESULT hr;
    BOOL ret;
    LONG change_ret;
    D3DDISPLAYMODE d3ddm;
    DWORD d3d_width = 0, d3d_height = 0, user32_width = 0, user32_height = 0;

    memset(&devmode, 0, sizeof(devmode));
    devmode.dmSize = sizeof(devmode);
    ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devmode);
    ok(ret, "EnumDisplaySettingsW failed, error %#x.\n", GetLastError());
    ok(equal_mode_rect(&devmode, &registry_mode), "Got a different mode.\n");
    ret = EnumDisplaySettingsW(NULL, ENUM_REGISTRY_SETTINGS, &devmode);
    ok(ret, "EnumDisplaySettingsW failed, error %#x.\n", GetLastError());
    ok(equal_mode_rect(&devmode, &registry_mode), "Got a different mode.\n");

    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");

    adapter_mode_count = IDirect3D9_GetAdapterModeCount(d3d9, D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8);
    for (i = 0; i < adapter_mode_count; ++i)
    {
        hr = IDirect3D9_EnumAdapterModes(d3d9, D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8, i, &d3ddm);
        ok(SUCCEEDED(hr), "Failed to enumerate display mode, hr %#x.\n", hr);

        if (d3ddm.Width == registry_mode.dmPelsWidth && d3ddm.Height == registry_mode.dmPelsHeight)
            continue;
        /* The r200 driver on Windows XP enumerates modes like 320x200 and 320x240 but
         * refuses to create a device at these sizes. */
        if (d3ddm.Width < 640 || d3ddm.Height < 480)
            continue;

        if (!user32_width)
        {
            user32_width = d3ddm.Width;
            user32_height = d3ddm.Height;
            continue;
        }

        /* Make sure the d3d mode is smaller in width or height and at most
         * equal in the other dimension than the mode passed to
         * ChangeDisplaySettings. Otherwise Windows shrinks the window to
         * the ChangeDisplaySettings parameters + 12. */
        if (d3ddm.Width == user32_width && d3ddm.Height == user32_height)
            continue;
        if (d3ddm.Width <= user32_width && d3ddm.Height <= user32_height)
        {
            d3d_width = d3ddm.Width;
            d3d_height = d3ddm.Height;
            break;
        }
        if (user32_width <= d3ddm.Width && user32_height <= d3ddm.Height)
        {
            d3d_width = user32_width;
            d3d_height = user32_height;
            user32_width = d3ddm.Width;
            user32_height = d3ddm.Height;
            break;
        }
    }

    if (!d3d_width)
    {
        skip("Could not find adequate modes, skipping mode tests.\n");
        IDirect3D9_Release(d3d9);
        return;
    }

    ret = save_display_modes(&original_modes, &display_count);
    ok(ret, "Failed to save original display modes.\n");

    memset(&devmode, 0, sizeof(devmode));
    devmode.dmSize = sizeof(devmode);
    devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
    devmode.dmPelsWidth = user32_width;
    devmode.dmPelsHeight = user32_height;
    change_ret = ChangeDisplaySettingsW(&devmode, CDS_FULLSCREEN);
    ok(change_ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#x.\n", change_ret);

    focus_window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, user32_width / 2, user32_height / 2, 0, 0, 0, 0);
    device_window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, user32_width / 2, user32_height / 2, 0, 0, 0, 0);

    SetRect(&d3d_rect, 0, 0, d3d_width, d3d_height);
    GetWindowRect(focus_window, &focus_rect);

    device_desc.adapter_ordinal = D3DADAPTER_DEFAULT;
    device_desc.device_window = device_window;
    device_desc.width = d3d_width;
    device_desc.height = d3d_height;
    device_desc.flags = CREATE_DEVICE_FULLSCREEN;
    if (!(device = create_device(d3d9, focus_window, &device_desc)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    devmode.dmPelsWidth = user32_width;
    devmode.dmPelsHeight = user32_height;
    change_ret = ChangeDisplaySettingsW(&devmode, CDS_FULLSCREEN);
    ok(change_ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#x.\n", change_ret);

    ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devmode);
    ok(ret, "Failed to get display mode.\n");
    ok(devmode.dmPelsWidth == user32_width && devmode.dmPelsHeight == user32_height,
            "Expected resolution %ux%u, got %ux%u.\n",
            user32_width, user32_height, devmode.dmPelsWidth, devmode.dmPelsHeight);

    GetWindowRect(device_window, &r);
    ok(EqualRect(&r, &d3d_rect), "Expected %s, got %s.\n",
            wine_dbgstr_rect(&d3d_rect), wine_dbgstr_rect(&r));
    GetWindowRect(focus_window, &r);
    ok(EqualRect(&r, &focus_rect), "Expected %s, got %s.\n",
            wine_dbgstr_rect(&focus_rect), wine_dbgstr_rect(&r));

    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(SUCCEEDED(hr), "Failed to get backbuffer, hr %#x.\n", hr);
    hr = IDirect3DSurface9_GetDesc(backbuffer, &desc);
    ok(SUCCEEDED(hr), "Failed to get backbuffer desc, hr %#x.\n", hr);
    ok(desc.Width == d3d_width, "Got unexpected backbuffer width %u, expected %u.\n",
            desc.Width, d3d_width);
    ok(desc.Height == d3d_height, "Got unexpected backbuffer height %u, expected %u.\n",
            desc.Height, d3d_height);
    IDirect3DSurface9_Release(backbuffer);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);

    ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devmode);
    ok(ret, "Failed to get display mode.\n");
    ok(devmode.dmPelsWidth == registry_mode.dmPelsWidth
            && devmode.dmPelsHeight == registry_mode.dmPelsHeight,
            "Expected resolution %ux%u, got %ux%u.\n",
            registry_mode.dmPelsWidth, registry_mode.dmPelsHeight, devmode.dmPelsWidth, devmode.dmPelsHeight);

    change_ret = ChangeDisplaySettingsW(NULL, CDS_FULLSCREEN);
    ok(change_ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#x.\n", change_ret);

    /* The mode restore also happens when the device was created at the original screen size. */

    device_desc.adapter_ordinal = D3DADAPTER_DEFAULT;
    device_desc.device_window = device_window;
    device_desc.width = registry_mode.dmPelsWidth;
    device_desc.height = registry_mode.dmPelsHeight;
    device_desc.flags = CREATE_DEVICE_FULLSCREEN;
    ok(!!(device = create_device(d3d9, focus_window, &device_desc)), "Failed to create a D3D device.\n");

    devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
    devmode.dmPelsWidth = user32_width;
    devmode.dmPelsHeight = user32_height;
    change_ret = ChangeDisplaySettingsW(&devmode, CDS_FULLSCREEN);
    ok(change_ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#x.\n", change_ret);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);

    memset(&devmode2, 0, sizeof(devmode2));
    devmode2.dmSize = sizeof(devmode2);
    ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devmode2);
    ok(ret, "Failed to get display mode.\n");
    ok(devmode2.dmPelsWidth == registry_mode.dmPelsWidth
            && devmode2.dmPelsHeight == registry_mode.dmPelsHeight,
            "Expected resolution %ux%u, got %ux%u.\n", registry_mode.dmPelsWidth,
            registry_mode.dmPelsHeight, devmode2.dmPelsWidth, devmode2.dmPelsHeight);
    ret = restore_display_modes(original_modes, display_count);
    ok(ret, "Failed to restore display modes.\n");

    /* Test that no mode restorations if no mode changes happened */
    change_ret = ChangeDisplaySettingsW(&devmode, CDS_UPDATEREGISTRY | CDS_NORESET);
    ok(change_ret == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsW failed with %d.\n", change_ret);

    device_desc.adapter_ordinal = D3DADAPTER_DEFAULT;
    device_desc.device_window = device_window;
    device_desc.width = d3d_width;
    device_desc.height = d3d_height;
    device_desc.flags = 0;
    device = create_device(d3d9, device_window, &device_desc);
    ok(!!device, "Failed to create a D3D device.\n");
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);

    ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devmode2);
    ok(ret, "EnumDisplaySettingsW failed, error %#x.\n", GetLastError());
    ok(equal_mode_rect(&devmode2, &registry_mode), "Got a different mode.\n");
    ret = restore_display_modes(original_modes, display_count);
    ok(ret, "Failed to restore display modes.\n");

    /* Test that mode restorations use display settings in the registry with a fullscreen device */
    change_ret = ChangeDisplaySettingsW(&devmode, CDS_UPDATEREGISTRY | CDS_NORESET);
    ok(change_ret == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsW failed with %d.\n", change_ret);

    device_desc.adapter_ordinal = D3DADAPTER_DEFAULT;
    device_desc.device_window = device_window;
    device_desc.width = registry_mode.dmPelsWidth;
    device_desc.height = registry_mode.dmPelsHeight;
    device_desc.flags = CREATE_DEVICE_FULLSCREEN;
    device = create_device(d3d9, device_window, &device_desc);
    ok(!!device, "Failed to create a D3D device.\n");
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);

    ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devmode2);
    ok(ret, "EnumDisplaySettingsW failed, error %#x.\n", GetLastError());
    ok(equal_mode_rect(&devmode2, &devmode), "Got a different mode.\n");
    ret = EnumDisplaySettingsW(NULL, ENUM_REGISTRY_SETTINGS, &devmode2);
    ok(ret, "EnumDisplaySettingsW failed, error %#x.\n", GetLastError());
    ok(equal_mode_rect(&devmode2, &devmode), "Got a different mode.\n");
    ret = restore_display_modes(original_modes, display_count);
    ok(ret, "Failed to restore display modes.\n");

    /* Test that mode restorations use display settings in the registry with a fullscreen device
     * having the same display mode and then reset to a different mode */
    change_ret = ChangeDisplaySettingsW(&devmode, CDS_UPDATEREGISTRY | CDS_NORESET);
    ok(change_ret == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsW failed with %d.\n", change_ret);

    device_desc.adapter_ordinal = D3DADAPTER_DEFAULT;
    device_desc.device_window = device_window;
    device_desc.width = registry_mode.dmPelsWidth;
    device_desc.height = registry_mode.dmPelsHeight;
    device_desc.flags = CREATE_DEVICE_FULLSCREEN;
    device = create_device(d3d9, device_window, &device_desc);
    ok(!!device, "Failed to create a D3D device.\n");

    device_desc.width = d3d_width;
    device_desc.height = d3d_height;
    hr = reset_device(device, &device_desc);
    ok(hr == D3D_OK, "Failed to reset device, hr %#x.\n", hr);
    ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devmode2);
    ok(ret, "EnumDisplaySettingsW failed, error %#x.\n", GetLastError());
    ok(devmode2.dmPelsWidth == d3d_width && devmode2.dmPelsHeight == d3d_height,
            "Expected resolution %ux%u, got %ux%u.\n", d3d_width, d3d_height,
            devmode2.dmPelsWidth, devmode2.dmPelsHeight);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);

    ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devmode2);
    ok(ret, "EnumDisplaySettingsW failed, error %#x.\n", GetLastError());
    ok(equal_mode_rect(&devmode2, &devmode), "Got a different mode.\n");
    ret = EnumDisplaySettingsW(NULL, ENUM_REGISTRY_SETTINGS, &devmode2);
    ok(ret, "EnumDisplaySettingsW failed, error %#x.\n", GetLastError());
    ok(equal_mode_rect(&devmode2, &devmode), "Got a different mode.\n");
    ret = restore_display_modes(original_modes, display_count);
    ok(ret, "Failed to restore display modes.\n");

    if (IDirect3D9_GetAdapterCount(d3d9) < 2)
    {
        skip("Following tests require two adapters.\n");
        goto done;
    }

    second_monitor = IDirect3D9_GetAdapterMonitor(d3d9, 1);
    monitor_info.cbSize = sizeof(monitor_info);
    ret = GetMonitorInfoW(second_monitor, (MONITORINFO *)&monitor_info);
    ok(ret, "GetMonitorInfoW failed, error %#x.\n", GetLastError());
    lstrcpyW(second_monitor_name, monitor_info.szDevice);

    memset(&old_devmode, 0, sizeof(old_devmode));
    old_devmode.dmSize = sizeof(old_devmode);
    ret = EnumDisplaySettingsW(second_monitor_name, ENUM_CURRENT_SETTINGS, &old_devmode);
    ok(ret, "EnumDisplaySettingsW failed, error %#x.\n", GetLastError());

    i = 0;
    d3d_width = 0;
    d3d_height = 0;
    user32_width = 0;
    user32_height = 0;
    while (EnumDisplaySettingsW(second_monitor_name, i++, &devmode))
    {
        if (devmode.dmPelsWidth == old_devmode.dmPelsWidth
                && devmode.dmPelsHeight == old_devmode.dmPelsHeight)
            continue;

        if (!d3d_width && !d3d_height)
        {
            d3d_width = devmode.dmPelsWidth;
            d3d_height = devmode.dmPelsHeight;
            continue;
        }

        if (devmode.dmPelsWidth == d3d_width && devmode.dmPelsHeight == d3d_height)
            continue;

        user32_width = devmode.dmPelsWidth;
        user32_height = devmode.dmPelsHeight;
        break;
    }
    if (!user32_width || !user32_height)
    {
        skip("Failed to find three different display modes for the second monitor.\n");
        goto done;
    }

    /* Test that mode restorations also happen for non-primary monitors on device resets */
    device_desc.adapter_ordinal = D3DADAPTER_DEFAULT;
    device_desc.device_window = device_window;
    device_desc.width = registry_mode.dmPelsWidth;
    device_desc.height = registry_mode.dmPelsHeight;
    device_desc.flags = CREATE_DEVICE_FULLSCREEN;
    device = create_device(d3d9, device_window, &device_desc);
    ok(!!device, "Failed to create a D3D device.\n");

    change_ret = ChangeDisplaySettingsExW(second_monitor_name, &devmode, NULL, CDS_RESET, NULL);
    ok(change_ret == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExW failed with %d.\n", change_ret);
    ret = EnumDisplaySettingsW(second_monitor_name, ENUM_CURRENT_SETTINGS, &devmode2);
    ok(ret, "EnumDisplaySettingsW failed, error %#x.\n", GetLastError());
    if (devmode2.dmPelsWidth == old_devmode.dmPelsWidth
            && devmode2.dmPelsHeight == old_devmode.dmPelsHeight)
    {
        skip("Failed to change display settings of the second monitor.\n");
        refcount = IDirect3DDevice9_Release(device);
        ok(!refcount, "Device has %u references left.\n", refcount);
        goto done;
    }

    device_desc.flags = 0;
    hr = reset_device(device, &device_desc);
    ok(hr == D3D_OK, "Failed to reset device, hr %#x.\n", hr);

    ret = EnumDisplaySettingsW(second_monitor_name, ENUM_CURRENT_SETTINGS, &devmode2);
    ok(ret, "EnumDisplaySettingsW failed, error %#x.\n", GetLastError());
    ok(equal_mode_rect(&devmode2, &old_devmode), "Got a different mode.\n");
    ret = EnumDisplaySettingsW(second_monitor_name, ENUM_REGISTRY_SETTINGS, &devmode2);
    ok(ret, "EnumDisplaySettingsW failed, error %#x.\n", GetLastError());
    ok(equal_mode_rect(&devmode2, &old_devmode), "Got a different mode.\n");
    hr = IDirect3D9_GetAdapterDisplayMode(d3d9, 1, &d3ddm);
    ok(hr == S_OK, "GetAdapterDisplayMode failed, hr %#x.\n", hr);
    ok(d3ddm.Width == old_devmode.dmPelsWidth, "Expected width %u, got %u.\n",
            old_devmode.dmPelsWidth, d3ddm.Width);
    ok(d3ddm.Height == old_devmode.dmPelsHeight, "Expected height %u, got %u.\n",
            old_devmode.dmPelsHeight, d3ddm.Height);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    ret = restore_display_modes(original_modes, display_count);
    ok(ret, "Failed to restore display modes.\n");

    /* Test that mode restorations happen for non-primary monitors on device releases */
    device_desc.adapter_ordinal = D3DADAPTER_DEFAULT;
    device_desc.device_window = device_window;
    device_desc.width = registry_mode.dmPelsWidth;
    device_desc.height = registry_mode.dmPelsHeight;
    device_desc.flags = CREATE_DEVICE_FULLSCREEN;
    device = create_device(d3d9, device_window, &device_desc);
    ok(!!device, "Failed to create a D3D device.\n");

    change_ret = ChangeDisplaySettingsExW(second_monitor_name, &devmode, NULL, CDS_RESET, NULL);
    ok(change_ret == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExW failed with %d.\n", change_ret);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);

    ret = EnumDisplaySettingsW(second_monitor_name, ENUM_CURRENT_SETTINGS, &devmode2);
    ok(ret, "EnumDisplaySettingsW failed, error %#x.\n", GetLastError());
    ok(equal_mode_rect(&devmode2, &old_devmode), "Got a different mode.\n");
    ret = EnumDisplaySettingsW(second_monitor_name, ENUM_REGISTRY_SETTINGS, &devmode2);
    ok(ret, "EnumDisplaySettingsW failed, error %#x.\n", GetLastError());
    ok(equal_mode_rect(&devmode2, &old_devmode), "Got a different mode.\n");
    hr = IDirect3D9_GetAdapterDisplayMode(d3d9, 1, &d3ddm);
    ok(hr == S_OK, "GetAdapterDisplayMode failed, hr %#x.\n", hr);
    ok(d3ddm.Width == old_devmode.dmPelsWidth, "Expected width %u, got %u.\n",
            old_devmode.dmPelsWidth, d3ddm.Width);
    ok(d3ddm.Height == old_devmode.dmPelsHeight, "Expected height %u, got %u.\n",
            old_devmode.dmPelsHeight, d3ddm.Height);
    ret = restore_display_modes(original_modes, display_count);
    ok(ret, "Failed to restore display modes.\n");

    /* Test that mode restorations for non-primary monitors use display settings in the registry */
    device = create_device(d3d9, device_window, &device_desc);
    ok(!!device, "Failed to create a D3D device.\n");

    change_ret = ChangeDisplaySettingsExW(second_monitor_name, &devmode, NULL,
            CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
    ok(change_ret == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExW failed with %d.\n", change_ret);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);

    ret = EnumDisplaySettingsW(second_monitor_name, ENUM_CURRENT_SETTINGS, &devmode2);
    ok(ret, "EnumDisplaySettingsW failed, error %#x.\n", GetLastError());
    ok(devmode2.dmPelsWidth == devmode.dmPelsWidth && devmode2.dmPelsHeight == devmode.dmPelsHeight,
            "Expected resolution %ux%u, got %ux%u.\n", devmode.dmPelsWidth, devmode.dmPelsHeight,
            devmode2.dmPelsWidth, devmode2.dmPelsHeight);
    ret = EnumDisplaySettingsW(second_monitor_name, ENUM_REGISTRY_SETTINGS, &devmode2);
    ok(ret, "EnumDisplaySettingsW failed, error %#x.\n", GetLastError());
    ok(devmode2.dmPelsWidth == devmode.dmPelsWidth && devmode2.dmPelsHeight == devmode.dmPelsHeight,
            "Expected resolution %ux%u, got %ux%u.\n", devmode.dmPelsWidth, devmode.dmPelsHeight,
            devmode2.dmPelsWidth, devmode2.dmPelsHeight);
    hr = IDirect3D9_GetAdapterDisplayMode(d3d9, 1, &d3ddm);
    ok(hr == S_OK, "GetAdapterDisplayMode failed, hr %#x.\n", hr);
    ok(d3ddm.Width == devmode.dmPelsWidth && d3ddm.Height == devmode.dmPelsHeight,
            "Expected resolution %ux%u, got %ux%u.\n", devmode.dmPelsWidth, devmode.dmPelsHeight,
            d3ddm.Width, d3ddm.Height);
    ret = restore_display_modes(original_modes, display_count);
    ok(ret, "Failed to restore display modes.\n");

    /* Test mode restorations when there are two fullscreen devices and one of them got reset */
    device = create_device(d3d9, focus_window, &device_desc);
    ok(!!device, "Failed to create a D3D device.\n");

    device_desc2.adapter_ordinal = 1;
    device_desc2.device_window = device_window;
    device_desc2.width = d3d_width;
    device_desc2.height = d3d_height;
    device_desc2.flags = CREATE_DEVICE_FULLSCREEN;
    device2 = create_device(d3d9, focus_window, &device_desc2);
    ok(!!device2, "Failed to create a D3D device.\n");

    change_ret = ChangeDisplaySettingsExW(second_monitor_name, &devmode, NULL, CDS_RESET, NULL);
    ok(change_ret == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExW failed with %d.\n", change_ret);

    device_desc.flags = 0;
    hr = reset_device(device, &device_desc);
    ok(hr == D3D_OK, "Failed to reset device, hr %#x.\n", hr);

    ret = EnumDisplaySettingsW(second_monitor_name, ENUM_CURRENT_SETTINGS, &devmode2);
    ok(ret, "EnumDisplaySettingsW failed, error %#x.\n", GetLastError());
    ok(equal_mode_rect(&devmode2, &old_devmode), "Got a different mode.\n");
    ret = EnumDisplaySettingsW(second_monitor_name, ENUM_REGISTRY_SETTINGS, &devmode2);
    ok(ret, "EnumDisplaySettingsW failed, error %#x.\n", GetLastError());
    ok(equal_mode_rect(&devmode2, &old_devmode), "Got a different mode.\n");
    hr = IDirect3D9_GetAdapterDisplayMode(d3d9, 1, &d3ddm);
    ok(hr == S_OK, "GetAdapterDisplayMode failed, hr %#x.\n", hr);
    ok(d3ddm.Width == old_devmode.dmPelsWidth && d3ddm.Height == old_devmode.dmPelsHeight,
            "Expected resolution %ux%u, got %ux%u.\n", old_devmode.dmPelsWidth,
            old_devmode.dmPelsHeight, d3ddm.Width, d3ddm.Height);

    refcount = IDirect3DDevice9_Release(device2);
    ok(!refcount, "Device has %u references left.\n", refcount);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    ret = restore_display_modes(original_modes, display_count);
    ok(ret, "Failed to restore display modes.\n");

    /* Test mode restoration when there are two fullscreen devices and one of them got released */
    device_desc.flags = CREATE_DEVICE_FULLSCREEN;
    device = create_device(d3d9, focus_window, &device_desc);
    ok(!!device, "Failed to create a D3D device.\n");
    device2 = create_device(d3d9, focus_window, &device_desc2);
    ok(!!device2, "Failed to create a D3D device.\n");

    change_ret = ChangeDisplaySettingsExW(second_monitor_name, &devmode, NULL, CDS_RESET, NULL);
    ok(change_ret == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsExW failed with %d.\n", change_ret);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);

    ret = EnumDisplaySettingsW(second_monitor_name, ENUM_CURRENT_SETTINGS, &devmode2);
    ok(ret, "EnumDisplaySettingsW failed, error %#x.\n", GetLastError());
    ok(equal_mode_rect(&devmode2, &old_devmode), "Got a different mode.\n");
    ret = EnumDisplaySettingsW(second_monitor_name, ENUM_REGISTRY_SETTINGS, &devmode2);
    ok(ret, "EnumDisplaySettingsW failed, error %#x.\n", GetLastError());
    ok(equal_mode_rect(&devmode2, &old_devmode), "Got a different mode.\n");
    hr = IDirect3D9_GetAdapterDisplayMode(d3d9, 1, &d3ddm);
    ok(hr == S_OK, "GetAdapterDisplayMode failed, hr %#x.\n", hr);
    ok(d3ddm.Width == old_devmode.dmPelsWidth && d3ddm.Height == old_devmode.dmPelsHeight,
            "Expected resolution %ux%u, got %ux%u.\n", old_devmode.dmPelsWidth,
            old_devmode.dmPelsHeight, d3ddm.Width, d3ddm.Height);

    refcount = IDirect3DDevice9_Release(device2);
    ok(!refcount, "Device has %u references left.\n", refcount);

done:
    DestroyWindow(device_window);
    DestroyWindow(focus_window);
    IDirect3D9_Release(d3d9);
    ret = restore_display_modes(original_modes, display_count);
    ok(ret, "Failed to restore display modes.\n");
    heap_free(original_modes);
}

static void test_device_window_reset(void)
{
    RECT fullscreen_rect, device_rect, r;
    struct device_desc device_desc;
    IDirect3DDevice9 *device;
    WNDCLASSA wc = {0};
    IDirect3D9 *d3d9;
    LONG_PTR proc;
    HRESULT hr;
    ULONG ref;

    filter_messages = NULL;
    expect_messages = NULL;

    wc.lpfnWndProc = test_proc;
    wc.lpszClassName = "d3d9_test_wndproc_wc";
    ok(RegisterClassA(&wc), "Failed to register window class.\n");

    focus_window = CreateWindowA("d3d9_test_wndproc_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, registry_mode.dmPelsWidth / 2, registry_mode.dmPelsHeight / 2, 0, 0, 0, 0);
    device_window = CreateWindowA("d3d9_test_wndproc_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, registry_mode.dmPelsWidth / 2, registry_mode.dmPelsHeight / 2, 0, 0, 0, 0);
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");

    SetRect(&fullscreen_rect, 0, 0, registry_mode.dmPelsWidth, registry_mode.dmPelsHeight);
    GetWindowRect(device_window, &device_rect);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);

    device_desc.adapter_ordinal = D3DADAPTER_DEFAULT;
    device_desc.device_window = NULL;
    device_desc.width = registry_mode.dmPelsWidth;
    device_desc.height = registry_mode.dmPelsHeight;
    device_desc.flags = CREATE_DEVICE_FULLSCREEN;
    if (!(device = create_device(d3d9, focus_window, &device_desc)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    GetWindowRect(focus_window, &r);
    ok(EqualRect(&r, &fullscreen_rect), "Expected %s, got %s.\n",
            wine_dbgstr_rect(&fullscreen_rect), wine_dbgstr_rect(&r));
    GetWindowRect(device_window, &r);
    ok(EqualRect(&r, &device_rect), "Expected %s, got %s.\n",
            wine_dbgstr_rect(&device_rect), wine_dbgstr_rect(&r));

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#lx.\n", (LONG_PTR)test_proc);

    device_desc.device_window = device_window;
    hr = reset_device(device, &device_desc);
    ok(SUCCEEDED(hr), "Failed to reset device.\n");

    GetWindowRect(focus_window, &r);
    ok(EqualRect(&r, &fullscreen_rect), "Expected %s, got %s.\n",
            wine_dbgstr_rect(&fullscreen_rect), wine_dbgstr_rect(&r));
    GetWindowRect(device_window, &r);
    ok(EqualRect(&r, &fullscreen_rect), "Expected %s, got %s.\n",
            wine_dbgstr_rect(&fullscreen_rect), wine_dbgstr_rect(&r));

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#lx, got %#lx.\n",
            (LONG_PTR)test_proc, proc);
    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#lx.\n", (LONG_PTR)test_proc);

    ref = IDirect3DDevice9_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

done:
    IDirect3D9_Release(d3d9);
    DestroyWindow(device_window);
    DestroyWindow(focus_window);
    UnregisterClassA("d3d9_test_wndproc_wc", GetModuleHandleA(NULL));
}

static void test_reset_resources(void)
{
    IDirect3DSurface9 *surface, *rt;
    IDirect3DTexture9 *texture;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d9;
    unsigned int i;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;
    ULONG ref;

    window = create_window();
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");

    if (!(device = create_device(d3d9, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateDepthStencilSurface(device, 128, 128,
            D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, TRUE, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create depth/stencil surface, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetDepthStencilSurface(device, surface);
    ok(SUCCEEDED(hr), "Failed to set depth/stencil surface, hr %#x.\n", hr);
    IDirect3DSurface9_Release(surface);

    for (i = 0; i < caps.NumSimultaneousRTs; ++i)
    {
        hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 1, D3DUSAGE_RENDERTARGET,
                D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL);
        ok(SUCCEEDED(hr), "Failed to create render target texture %u, hr %#x.\n", i, hr);
        hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
        ok(SUCCEEDED(hr), "Failed to get surface %u, hr %#x.\n", i, hr);
        IDirect3DTexture9_Release(texture);
        hr = IDirect3DDevice9_SetRenderTarget(device, i, surface);
        ok(SUCCEEDED(hr), "Failed to set render target surface %u, hr %#x.\n", i, hr);
        IDirect3DSurface9_Release(surface);
    }

    hr = reset_device(device, NULL);
    ok(SUCCEEDED(hr), "Failed to reset device.\n");

    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &rt);
    ok(SUCCEEDED(hr), "Failed to get back buffer, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &surface);
    ok(SUCCEEDED(hr), "Failed to get render target surface, hr %#x.\n", hr);
    ok(surface == rt, "Got unexpected surface %p for render target.\n", surface);
    IDirect3DSurface9_Release(surface);
    IDirect3DSurface9_Release(rt);

    for (i = 1; i < caps.NumSimultaneousRTs; ++i)
    {
        hr = IDirect3DDevice9_GetRenderTarget(device, i, &surface);
        ok(hr == D3DERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);
    }

    ref = IDirect3DDevice9_Release(device);
    ok(ref == 0, "The device was not properly freed: refcount %u.\n", ref);

done:
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_set_rt_vp_scissor(void)
{
    IDirect3DStateBlock9 *stateblock;
    IDirect3DDevice9 *device;
    IDirect3DSurface9 *rt;
    IDirect3D9 *d3d9;
    D3DVIEWPORT9 vp;
    UINT refcount;
    HWND window;
    HRESULT hr;
    RECT rect;

    window = create_window();
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d9, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateRenderTarget(device, 128, 128, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_NONE, 0, FALSE, &rt, NULL);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to get viewport, hr %#x.\n", hr);
    ok(!vp.X, "Got unexpected vp.X %u.\n", vp.X);
    ok(!vp.Y, "Got unexpected vp.Y %u.\n", vp.Y);
    ok(vp.Width == 640, "Got unexpected vp.Width %u.\n", vp.Width);
    ok(vp.Height == 480, "Got unexpected vp.Height %u.\n", vp.Height);
    ok(vp.MinZ == 0.0f, "Got unexpected vp.MinZ %.8e.\n", vp.MinZ);
    ok(vp.MaxZ == 1.0f, "Got unexpected vp.MaxZ %.8e.\n", vp.MaxZ);

    hr = IDirect3DDevice9_GetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to get scissor rect, hr %#x.\n", hr);
    ok(rect.left == 0 && rect.top == 0 && rect.right == 640 && rect.bottom == 480,
            "Got unexpected scissor rect %s.\n", wine_dbgstr_rect(&rect));

    hr = IDirect3DDevice9_BeginStateBlock(device);
    ok(SUCCEEDED(hr), "Failed to begin stateblock, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, rt);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);

    hr = IDirect3DDevice9_EndStateBlock(device, &stateblock);
    ok(SUCCEEDED(hr), "Failed to end stateblock, hr %#x.\n", hr);
    IDirect3DStateBlock9_Release(stateblock);

    hr = IDirect3DDevice9_GetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to get viewport, hr %#x.\n", hr);
    ok(!vp.X, "Got unexpected vp.X %u.\n", vp.X);
    ok(!vp.Y, "Got unexpected vp.Y %u.\n", vp.Y);
    ok(vp.Width == 128, "Got unexpected vp.Width %u.\n", vp.Width);
    ok(vp.Height == 128, "Got unexpected vp.Height %u.\n", vp.Height);
    ok(vp.MinZ == 0.0f, "Got unexpected vp.MinZ %.8e.\n", vp.MinZ);
    ok(vp.MaxZ == 1.0f, "Got unexpected vp.MaxZ %.8e.\n", vp.MaxZ);

    hr = IDirect3DDevice9_GetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to get scissor rect, hr %#x.\n", hr);
    ok(rect.left == 0 && rect.top == 0 && rect.right == 128 && rect.bottom == 128,
            "Got unexpected scissor rect %s.\n", wine_dbgstr_rect(&rect));

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, rt);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);

    vp.X = 10;
    vp.Y = 20;
    vp.Width = 30;
    vp.Height = 40;
    vp.MinZ = 0.25f;
    vp.MaxZ = 0.75f;
    hr = IDirect3DDevice9_SetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to set viewport, hr %#x.\n", hr);

    SetRect(&rect, 50, 60, 70, 80);
    hr = IDirect3DDevice9_SetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to set scissor rect, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, rt);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to get viewport, hr %#x.\n", hr);
    ok(!vp.X, "Got unexpected vp.X %u.\n", vp.X);
    ok(!vp.Y, "Got unexpected vp.Y %u.\n", vp.Y);
    ok(vp.Width == 128, "Got unexpected vp.Width %u.\n", vp.Width);
    ok(vp.Height == 128, "Got unexpected vp.Height %u.\n", vp.Height);
    ok(vp.MinZ == 0.0f, "Got unexpected vp.MinZ %.8e.\n", vp.MinZ);
    ok(vp.MaxZ == 1.0f, "Got unexpected vp.MaxZ %.8e.\n", vp.MaxZ);

    hr = IDirect3DDevice9_GetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to get scissor rect, hr %#x.\n", hr);
    ok(rect.left == 0 && rect.top == 0 && rect.right == 128 && rect.bottom == 128,
            "Got unexpected scissor rect %s.\n", wine_dbgstr_rect(&rect));

    IDirect3DSurface9_Release(rt);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_volume_get_container(void)
{
    IDirect3DVolumeTexture9 *texture = NULL;
    IDirect3DVolume9 *volume = NULL;
    IDirect3DDevice9 *device;
    IUnknown *container;
    IDirect3D9 *d3d9;
    ULONG refcount;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;

    window = create_window();
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d9, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (!(caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP))
    {
        skip("No volume texture support, skipping tests.\n");
        IDirect3DDevice9_Release(device);
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateVolumeTexture(device, 128, 128, 128, 1, 0,
            D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture, 0);
    ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x.\n", hr);
    ok(!!texture, "Got unexpected texture %p.\n", texture);

    hr = IDirect3DVolumeTexture9_GetVolumeLevel(texture, 0, &volume);
    ok(SUCCEEDED(hr), "Failed to get volume level, hr %#x.\n", hr);
    ok(!!volume, "Got unexpected volume %p.\n", volume);

    /* These should work... */
    container = NULL;
    hr = IDirect3DVolume9_GetContainer(volume, &IID_IUnknown, (void **)&container);
    ok(SUCCEEDED(hr), "Failed to get volume container, hr %#x.\n", hr);
    ok(container == (IUnknown *)texture, "Got unexpected container %p, expected %p.\n", container, texture);
    IUnknown_Release(container);

    container = NULL;
    hr = IDirect3DVolume9_GetContainer(volume, &IID_IDirect3DResource9, (void **)&container);
    ok(SUCCEEDED(hr), "Failed to get volume container, hr %#x.\n", hr);
    ok(container == (IUnknown *)texture, "Got unexpected container %p, expected %p.\n", container, texture);
    IUnknown_Release(container);

    container = NULL;
    hr = IDirect3DVolume9_GetContainer(volume, &IID_IDirect3DBaseTexture9, (void **)&container);
    ok(SUCCEEDED(hr), "Failed to get volume container, hr %#x.\n", hr);
    ok(container == (IUnknown *)texture, "Got unexpected container %p, expected %p.\n", container, texture);
    IUnknown_Release(container);

    container = NULL;
    hr = IDirect3DVolume9_GetContainer(volume, &IID_IDirect3DVolumeTexture9, (void **)&container);
    ok(SUCCEEDED(hr), "Failed to get volume container, hr %#x.\n", hr);
    ok(container == (IUnknown *)texture, "Got unexpected container %p, expected %p.\n", container, texture);
    IUnknown_Release(container);

    /* ...and this one shouldn't. This should return E_NOINTERFACE and set container to NULL. */
    hr = IDirect3DVolume9_GetContainer(volume, &IID_IDirect3DVolume9, (void **)&container);
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#x.\n", hr);
    ok(!container, "Got unexpected container %p.\n", container);

    IDirect3DVolume9_Release(volume);
    IDirect3DVolumeTexture9_Release(texture);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_volume_resource(void)
{
    IDirect3DVolumeTexture9 *texture;
    IDirect3DResource9 *resource;
    IDirect3DVolume9 *volume;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d9;
    ULONG refcount;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;

    window = create_window();
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d9, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (!(caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP))
    {
        skip("No volume texture support, skipping tests.\n");
        IDirect3DDevice9_Release(device);
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateVolumeTexture(device, 128, 128, 128, 1, 0,
            D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture, 0);
    ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x.\n", hr);
    hr = IDirect3DVolumeTexture9_GetVolumeLevel(texture, 0, &volume);
    ok(SUCCEEDED(hr), "Failed to get volume level, hr %#x.\n", hr);
    IDirect3DVolumeTexture9_Release(texture);

    hr = IDirect3DVolume9_QueryInterface(volume, &IID_IDirect3DResource9, (void **)&resource);
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#x.\n", hr);

    IDirect3DVolume9_Release(volume);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_vb_lock_flags(void)
{
    static const struct
    {
        DWORD flags;
        const char *debug_string;
        HRESULT win7_result;
    }
    test_data[] =
    {
        {D3DLOCK_READONLY,                          "D3DLOCK_READONLY",                         D3D_OK            },
        {D3DLOCK_DISCARD,                           "D3DLOCK_DISCARD",                          D3D_OK            },
        {D3DLOCK_NOOVERWRITE,                       "D3DLOCK_NOOVERWRITE",                      D3D_OK            },
        {D3DLOCK_NOOVERWRITE | D3DLOCK_DISCARD,     "D3DLOCK_NOOVERWRITE | D3DLOCK_DISCARD",    D3D_OK            },
        {D3DLOCK_NOOVERWRITE | D3DLOCK_READONLY,    "D3DLOCK_NOOVERWRITE | D3DLOCK_READONLY",   D3D_OK            },
        {D3DLOCK_READONLY | D3DLOCK_DISCARD,        "D3DLOCK_READONLY | D3DLOCK_DISCARD",       D3DERR_INVALIDCALL},
        /* Completely bogus flags aren't an error. */
        {0xdeadbeef,                                "0xdeadbeef",                               D3DERR_INVALIDCALL},
    };
    IDirect3DVertexBuffer9 *buffer;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d9;
    unsigned int i;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    void *data;

    window = create_window();
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d9, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateVertexBuffer(device, 1024, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
            0, D3DPOOL_DEFAULT, &buffer, NULL);
    ok(SUCCEEDED(hr), "Failed to create vertex buffer, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(test_data); ++i)
    {
        hr = IDirect3DVertexBuffer9_Lock(buffer, 0, 0, &data, test_data[i].flags);
        /* Windows XP always returns D3D_OK even with flags that don't make
         * sense. Windows 7 returns an error. At least one game (Shaiya)
         * depends on the Windows XP result, so mark the Windows 7 behavior as
         * broken. */
        ok(hr == D3D_OK || broken(hr == test_data[i].win7_result), "Got unexpected hr %#x for %s.\n",
                hr, test_data[i].debug_string);
        if (SUCCEEDED(hr))
        {
            ok(!!data, "Got unexpected data %p.\n", data);
            hr = IDirect3DVertexBuffer9_Unlock(buffer);
            ok(SUCCEEDED(hr), "Failed to unlock vertex buffer, hr %#x.\n", hr);
        }
    }

    IDirect3DVertexBuffer9_Release(buffer);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static const char *debug_d3dpool(D3DPOOL pool)
{
    switch (pool)
    {
        case D3DPOOL_DEFAULT:
            return "D3DPOOL_DEFAULT";
        case D3DPOOL_SYSTEMMEM:
            return "D3DPOOL_SYSTEMMEM";
        case D3DPOOL_SCRATCH:
            return "D3DPOOL_SCRATCH";
        case D3DPOOL_MANAGED:
            return "D3DPOOL_MANAGED";
        default:
            return "unknown pool";
    }
}

static void test_vertex_buffer_alignment(void)
{
    static const D3DPOOL pools[] = {D3DPOOL_DEFAULT, D3DPOOL_SYSTEMMEM, D3DPOOL_SCRATCH, D3DPOOL_MANAGED};
    static const DWORD sizes[] = {1, 4, 16, 17, 32, 33, 64, 65, 1024, 1025, 1048576, 1048577};
    IDirect3DVertexBuffer9 *buffer = NULL;
    const unsigned int align = 16;
    IDirect3DDevice9 *device;
    unsigned int i, j;
    IDirect3D9 *d3d9;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    void *data;

    window = create_window();
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d9, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }

    for (i = 0; i < ARRAY_SIZE(sizes); ++i)
    {
        for (j = 0; j < ARRAY_SIZE(pools); ++j)
        {
            hr = IDirect3DDevice9_CreateVertexBuffer(device, sizes[i], 0, 0, pools[j], &buffer, NULL);
            if (pools[j] == D3DPOOL_SCRATCH)
                ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x trying to create a D3DPOOL_SCRATCH buffer.\n", hr);
            else
                ok(SUCCEEDED(hr), "Failed to create vertex buffer in pool %s with size %u, hr %#x.\n",
                        debug_d3dpool(pools[j]), sizes[i], hr);
            if (FAILED(hr))
                continue;

            hr = IDirect3DVertexBuffer9_Lock(buffer, 0, 0, &data, 0);
            ok(SUCCEEDED(hr), "Failed to lock vertex buffer, hr %#x.\n", hr);
            ok(!((DWORD_PTR)data & (align - 1)),
                    "Vertex buffer start address %p is not %u byte aligned (size %u, pool %s).\n",
                    data, align, sizes[i], debug_d3dpool(pools[j]));
            hr = IDirect3DVertexBuffer9_Unlock(buffer);
            ok(SUCCEEDED(hr), "Failed to unlock vertex buffer, hr %#x.\n", hr);
            IDirect3DVertexBuffer9_Release(buffer);
        }
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_query_support(void)
{
    static const D3DQUERYTYPE queries[] =
    {
        D3DQUERYTYPE_VCACHE,
        D3DQUERYTYPE_RESOURCEMANAGER,
        D3DQUERYTYPE_VERTEXSTATS,
        D3DQUERYTYPE_EVENT,
        D3DQUERYTYPE_OCCLUSION,
        D3DQUERYTYPE_TIMESTAMP,
        D3DQUERYTYPE_TIMESTAMPDISJOINT,
        D3DQUERYTYPE_TIMESTAMPFREQ,
        D3DQUERYTYPE_PIPELINETIMINGS,
        D3DQUERYTYPE_INTERFACETIMINGS,
        D3DQUERYTYPE_VERTEXTIMINGS,
        D3DQUERYTYPE_PIXELTIMINGS,
        D3DQUERYTYPE_BANDWIDTHTIMINGS,
        D3DQUERYTYPE_CACHEUTILIZATION,
    };
    IDirect3DQuery9 *query = NULL;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d9;
    unsigned int i;
    ULONG refcount;
    BOOL supported;
    HWND window;
    HRESULT hr;

    window = create_window();
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d9, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }

    for (i = 0; i < ARRAY_SIZE(queries); ++i)
    {
        hr = IDirect3DDevice9_CreateQuery(device, queries[i], NULL);
        ok(hr == D3D_OK || hr == D3DERR_NOTAVAILABLE, "Got unexpected hr %#x for query %#x.\n", hr, queries[i]);

        supported = hr == D3D_OK;

        hr = IDirect3DDevice9_CreateQuery(device, queries[i], &query);
        ok(hr == D3D_OK || hr == D3DERR_NOTAVAILABLE, "Got unexpected hr %#x for query %#x.\n", hr, queries[i]);

        ok(!supported || query, "Query %#x was claimed to be supported, but can't be created.\n", queries[i]);
        ok(supported || !query, "Query %#x was claimed not to be supported, but can be created.\n", queries[i]);

        if (query)
        {
            IDirect3DQuery9_Release(query);
            query = NULL;
        }
    }

    for (i = 0; i < 40; ++i)
    {
        /* Windows 10 17.09 (build 16299.19) added an undocumented query with an enum value of 0x16 (=22).
         * It returns D3D_OK when asking for support and E_FAIL when trying to actually create it. */
        if ((D3DQUERYTYPE_VCACHE <= i && i <= D3DQUERYTYPE_MEMORYPRESSURE) || i == 0x16)
            continue;

        hr = IDirect3DDevice9_CreateQuery(device, i, NULL);
        ok(hr == D3DERR_NOTAVAILABLE, "Got unexpected hr %#x for query %#x.\n", hr, i);

        query = (IDirect3DQuery9 *)0xdeadbeef;
        hr = IDirect3DDevice9_CreateQuery(device, i, &query);
        ok(hr == D3DERR_NOTAVAILABLE, "Got unexpected hr %#x for query %#x.\n", hr, i);
        ok(query == (IDirect3DQuery9 *)0xdeadbeef, "Got unexpected query %p.\n", query);
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_occlusion_query(void)
{
    static const float quad[] =
    {
        -1.0f, -1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
    };
    unsigned int data_size, i, count;
    struct device_desc device_desc;
    IDirect3DQuery9 *query = NULL;
    IDirect3DDevice9 *device;
    IDirect3DSurface9 *rt;
    IDirect3D9 *d3d9;
    D3DVIEWPORT9 vp;
    ULONG refcount;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;
    union
    {
        WORD word[4];
        DWORD dword[2];
        UINT64 uint;
    } data, expected;
    BOOL broken_occlusion = FALSE;
    expected.uint = registry_mode.dmPelsWidth * registry_mode.dmPelsHeight;

    window = create_window();
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");
    device_desc.adapter_ordinal = D3DADAPTER_DEFAULT;
    device_desc.device_window = window;
    device_desc.width = registry_mode.dmPelsWidth;
    device_desc.height = registry_mode.dmPelsHeight;
    device_desc.flags = CREATE_DEVICE_FULLSCREEN;
    if (!(device = create_device(d3d9, window, &device_desc)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffff0000, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
    ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateQuery(device, D3DQUERYTYPE_OCCLUSION, &query);
    ok(hr == D3D_OK || hr == D3DERR_NOTAVAILABLE, "Got unexpected hr %#x.\n", hr);
    if (!query)
    {
        skip("Occlusion queries are not supported, skipping tests.\n");
        IDirect3DDevice9_Release(device);
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }

    data_size = IDirect3DQuery9_GetDataSize(query);
    ok(data_size == sizeof(DWORD), "Unexpected data size %u.\n", data_size);

    memset(&data, 0xff, sizeof(data));
    hr = IDirect3DQuery9_GetData(query, NULL, 0, D3DGETDATA_FLUSH);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DQuery9_GetData(query, &data, data_size, D3DGETDATA_FLUSH);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(data.dword[0] == 0xdddddddd && data.dword[1] == 0xffffffff,
            "Got unexpected query result 0x%08x%08x.\n", data.dword[1], data.dword[0]);

    hr = IDirect3DQuery9_Issue(query, D3DISSUE_END);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DQuery9_Issue(query, D3DISSUE_BEGIN);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DQuery9_Issue(query, D3DISSUE_BEGIN);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    data.dword[0] = 0x12345678;
    hr = IDirect3DQuery9_GetData(query, NULL, 0, D3DGETDATA_FLUSH);
    ok(hr == S_FALSE || hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DQuery9_GetData(query, &data, data_size, D3DGETDATA_FLUSH);
    ok(hr == S_FALSE || hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    if (hr == D3D_OK)
        ok(!data.dword[0], "Got unexpected query result %u.\n", data.dword[0]);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLEFAN, 2, quad, 3 * sizeof(float));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    hr = IDirect3DQuery9_Issue(query, D3DISSUE_END);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    wait_query(query);

    memset(&data, 0xff, sizeof(data));
    hr = IDirect3DQuery9_GetData(query, &data, data_size, D3DGETDATA_FLUSH);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(data.dword[0] == expected.dword[0] || broken(!data.dword[0]),
            "Occlusion query returned an unexpected result (0x%.8x).\n", data.dword[0]);
    if (!data.dword[0])
    {
        win_skip("Occlusion query result looks broken, ignoring returned count.\n");
        broken_occlusion = TRUE;
    }

    memset(&data, 0xff, sizeof(data));
    hr = IDirect3DQuery9_GetData(query, &data, sizeof(WORD), D3DGETDATA_FLUSH);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    if (!broken_occlusion)
        ok(data.word[0] == expected.word[0],
                "Occlusion query returned an unexpected result (0x%.8x).\n", data.dword[0]);
    ok(data.word[1] == 0xffff,
            "Data was modified outside of the expected size (0x%.8x).\n", data.dword[0]);

    memset(&data, 0xf0, sizeof(data));
    hr = IDirect3DQuery9_GetData(query, &data, sizeof(data), D3DGETDATA_FLUSH);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    if (!broken_occlusion)
        ok(data.dword[0] == expected.dword[0],
                "Occlusion query returned an unexpected result (0x%.8x).\n", data.dword[0]);
    /* Different drivers seem to return different data in those high bytes on Windows, but they all
       write something there and the extra data is consistent (I've seen 0x00000000 and 0xdddddddd
       on AMD and Nvidia respectively). */
    if (0)
    {
        ok(data.dword[1] != 0xf0f0f0f0, "high bytes of data were not modified (0x%.8x).\n",
                data.dword[1]);
    }

    memset(&data, 0xff, sizeof(data));
    hr = IDirect3DQuery9_GetData(query, &data, 0, D3DGETDATA_FLUSH);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(data.dword[0] == 0xffffffff, "Occlusion query returned an unexpected result (0x%.8x).\n", data.dword[0]);

    /* This crashes on Windows. */
    if (0)
    {
        hr = IDirect3DQuery9_GetData(query, NULL, data_size, D3DGETDATA_FLUSH);
        ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    }

    hr = IDirect3DQuery9_Issue(query, D3DISSUE_BEGIN);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DQuery9_Issue(query, D3DISSUE_END);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DQuery9_Issue(query, D3DISSUE_END);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    if (broken_occlusion)
        goto done;

    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "Failed to begin scene, hr %#x.\n", hr);
    for (i = 0; i < 50000; ++i)
    {
        hr = IDirect3DQuery9_Issue(query, D3DISSUE_BEGIN);
        ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
        hr = IDirect3DQuery9_Issue(query, D3DISSUE_END);
        ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    }
    hr = IDirect3DDevice9_EndScene(device);
    ok(hr == D3D_OK, "Failed to end scene, hr %#x.\n", hr);

    wait_query(query);

    memset(&data, 0xff, sizeof(data));
    hr = IDirect3DQuery9_GetData(query, &data, sizeof(data), D3DGETDATA_FLUSH);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(data.dword[0] == 0 && data.dword[1] == 0,
            "Got unexpected query result 0x%08x%08x.\n", data.dword[1], data.dword[0]);

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);

    vp.X = 0;
    vp.Y = 0;
    vp.Width = min(caps.MaxTextureWidth, 8192);
    vp.Height = min(caps.MaxTextureHeight, 8192);
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;
    hr = IDirect3DDevice9_SetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to set viewport, hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateRenderTarget(device, vp.Width, vp.Height,
            D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, FALSE, &rt, NULL);
    if (FAILED(hr))
    {
        skip("Failed to create render target (width %u, height %u), hr %#x.\n", vp.Width, vp.Height, hr);
        goto done;
    }
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderTarget(device, 0, rt);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, FALSE);
    ok(hr == D3D_OK, "IDirect3DDevice9_SetRenderState returned %08x\n", hr);

    expected.uint = vp.Width * vp.Height;
    count = ((((UINT64)~0u) + 1) / expected.uint) + 1;
    expected.uint *= count;

    trace("Expects 0x%08x%08x samples.\n", expected.dword[1], expected.dword[0]);

    hr = IDirect3DQuery9_Issue(query, D3DISSUE_BEGIN);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    for (i = 0; i < count; i++)
    {
        hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLEFAN, 2, quad, 3 * sizeof(float));
        ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    }
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);
    hr = IDirect3DQuery9_Issue(query, D3DISSUE_END);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    wait_query(query);

    memset(&data, 0xff, sizeof(data));
    hr = IDirect3DQuery9_GetData(query, &data, sizeof(data), D3DGETDATA_FLUSH);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok((data.dword[0] == expected.dword[0] && data.dword[1] == expected.dword[1])
            || (data.dword[0] == 0xffffffff && !data.dword[1])
            || broken(data.dword[0] < 0xffffffff && !data.dword[1]),
            "Got unexpected query result 0x%08x%08x.\n", data.dword[1], data.dword[0]);

    IDirect3DSurface9_Release(rt);

done:
    IDirect3DQuery9_Release(query);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_timestamp_query(void)
{
    static const float quad[] =
    {
        -1.0f, -1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
    };
    IDirect3DQuery9 *query, *disjoint_query, *freq_query;
    IDirect3DDevice9 *device;
    unsigned int data_size;
    IDirect3D9 *d3d9;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    DWORD timestamp[2], freq[2];
    WORD disjoint[2];

    window = create_window();
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d9, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateQuery(device, D3DQUERYTYPE_TIMESTAMPFREQ, &freq_query);
    ok(hr == D3D_OK || hr == D3DERR_NOTAVAILABLE, "Got unexpected hr %#x.\n", hr);
    if (FAILED(hr))
    {
        skip("Timestamp queries are not supported, skipping tests.\n");
        IDirect3DDevice9_Release(device);
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }
    data_size = IDirect3DQuery9_GetDataSize(freq_query);
    ok(data_size == sizeof(UINT64), "Query data size is %u, 8 expected.\n", data_size);

    memset(freq, 0xff, sizeof(freq));
    hr = IDirect3DQuery9_GetData(freq_query, NULL, 0, D3DGETDATA_FLUSH);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DQuery9_GetData(freq_query, freq, sizeof(DWORD), D3DGETDATA_FLUSH);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(freq[0] == 0xdddddddd && freq[1] == 0xffffffff,
            "Got unexpected query result 0x%08x%08x.\n", freq[1], freq[0]);

    hr = IDirect3DDevice9_CreateQuery(device, D3DQUERYTYPE_TIMESTAMPDISJOINT, &disjoint_query);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    data_size = IDirect3DQuery9_GetDataSize(disjoint_query);
    ok(data_size == sizeof(BOOL), "Query data size is %u, 4 expected.\n", data_size);

    memset(disjoint, 0xff, sizeof(disjoint));
    hr = IDirect3DQuery9_GetData(disjoint_query, NULL, 0, D3DGETDATA_FLUSH);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DQuery9_GetData(disjoint_query, &disjoint, sizeof(WORD), D3DGETDATA_FLUSH);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(disjoint[0] == 0xdddd && disjoint[1] == 0xffff,
            "Got unexpected query result 0x%04x%04x.\n", disjoint[1], disjoint[0]);
    hr = IDirect3DQuery9_GetData(disjoint_query, &disjoint, sizeof(DWORD), D3DGETDATA_FLUSH);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(disjoint[0] == 0xdddd && disjoint[1] == 0xdddd,
            "Got unexpected query result 0x%04x%04x.\n", disjoint[1], disjoint[0]);

    hr = IDirect3DDevice9_CreateQuery(device, D3DQUERYTYPE_TIMESTAMP, &query);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    data_size = IDirect3DQuery9_GetDataSize(query);
    ok(data_size == sizeof(UINT64), "Query data size is %u, 8 expected.\n", data_size);

    hr = IDirect3DQuery9_Issue(freq_query, D3DISSUE_END);

    wait_query(freq_query);

    memset(freq, 0xff, sizeof(freq));
    hr = IDirect3DQuery9_GetData(freq_query, freq, sizeof(DWORD), D3DGETDATA_FLUSH);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(freq[1] == 0xffffffff,
            "Freq was modified outside of the expected size (0x%.8x).\n", freq[1]);
    hr = IDirect3DQuery9_GetData(freq_query, &freq, sizeof(freq), D3DGETDATA_FLUSH);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(freq[1] != 0xffffffff, "High bytes of freq were not modified (0x%.8x).\n",
            freq[1]);

    memset(timestamp, 0xff, sizeof(timestamp));
    hr = IDirect3DQuery9_GetData(query, NULL, 0, D3DGETDATA_FLUSH);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DQuery9_GetData(query, timestamp, sizeof(DWORD), D3DGETDATA_FLUSH);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(timestamp[0] == 0xdddddddd && timestamp[1] == 0xffffffff,
            "Got unexpected query result 0x%08x%08x.\n", timestamp[1], timestamp[0]);
    hr = IDirect3DQuery9_GetData(query, timestamp, sizeof(timestamp), D3DGETDATA_FLUSH);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(timestamp[0] == 0xdddddddd && timestamp[1] == 0xdddddddd,
            "Got unexpected query result 0x%08x%08x.\n", timestamp[1], timestamp[0]);

    hr = IDirect3DQuery9_Issue(disjoint_query, D3DISSUE_END);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DQuery9_Issue(disjoint_query, D3DISSUE_BEGIN);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DQuery9_Issue(disjoint_query, D3DISSUE_BEGIN);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DQuery9_GetData(query, timestamp, sizeof(timestamp), D3DGETDATA_FLUSH);
    ok(hr == S_FALSE || hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
    ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLEFAN, 2, quad, 3 * sizeof(float));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    hr = IDirect3DQuery9_Issue(query, D3DISSUE_END);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    wait_query(query);

    memset(timestamp, 0xff, sizeof(timestamp));
    hr = IDirect3DQuery9_GetData(query, timestamp, sizeof(DWORD), D3DGETDATA_FLUSH);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(timestamp[1] == 0xffffffff,
            "Timestamp was modified outside of the expected size (0x%.8x).\n",
            timestamp[1]);

    hr = IDirect3DQuery9_Issue(query, D3DISSUE_BEGIN);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DQuery9_Issue(query, D3DISSUE_END);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DQuery9_Issue(query, D3DISSUE_END);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DQuery9_Issue(disjoint_query, D3DISSUE_END);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    wait_query(disjoint_query);

    memset(disjoint, 0xff, sizeof(disjoint));
    hr = IDirect3DQuery9_GetData(disjoint_query, disjoint, sizeof(WORD), D3DGETDATA_FLUSH);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(disjoint[1] == 0xffff,
            "Disjoint was modified outside of the expected size (0x%.4hx).\n", disjoint[1]);
    hr = IDirect3DQuery9_GetData(disjoint_query, disjoint, sizeof(disjoint), D3DGETDATA_FLUSH);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    ok(disjoint[1] != 0xffff, "High bytes of disjoint were not modified (0x%.4hx).\n", disjoint[1]);

    /* It's not strictly necessary for the TIMESTAMP query to be inside
     * a TIMESTAMP_DISJOINT query. */
    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLEFAN, 2, quad, 3 * sizeof(float));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);

    hr = IDirect3DQuery9_Issue(query, D3DISSUE_END);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    wait_query(query);

    hr = IDirect3DQuery9_GetData(query, timestamp, sizeof(timestamp), D3DGETDATA_FLUSH);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    IDirect3DQuery9_Release(query);
    IDirect3DQuery9_Release(disjoint_query);
    IDirect3DQuery9_Release(freq_query);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_get_set_vertex_shader(void)
{
    IDirect3DVertexShader9 *current_shader = NULL;
    IDirect3DVertexShader9 *shader = NULL;
    const IDirect3DVertexShader9Vtbl *shader_vtbl;
    IDirect3DDevice9 *device;
    ULONG refcount, i;
    IDirect3D9 *d3d;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (!(caps.VertexShaderVersion & 0xffff))
    {
        skip("No vertex shader support, skipping tests.\n");
        IDirect3DDevice9_Release(device);
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateVertexShader(device, simple_vs, &shader);
    ok(SUCCEEDED(hr), "Failed to create shader, hr %#x.\n", hr);
    ok(!!shader, "Got unexpected shader %p.\n", shader);

    /* SetVertexShader() should not touch the shader's refcount. */
    i = get_refcount((IUnknown *)shader);
    hr = IDirect3DDevice9_SetVertexShader(device, shader);
    refcount = get_refcount((IUnknown *)shader);
    ok(SUCCEEDED(hr), "Failed to set vertex shader, hr %#x.\n", hr);
    ok(refcount == i, "Got unexpected refcount %u, expected %u.\n", refcount, i);

    /* GetVertexShader() should increase the shader's refcount by one. */
    i = refcount + 1;
    hr = IDirect3DDevice9_GetVertexShader(device, &current_shader);
    refcount = get_refcount((IUnknown *)shader);
    ok(SUCCEEDED(hr), "Failed to get vertex shader, hr %#x.\n", hr);
    ok(refcount == i, "Got unexpected refcount %u, expected %u.\n", refcount, i);
    ok(current_shader == shader, "Got unexpected shader %p, expected %p.\n", current_shader, shader);
    IDirect3DVertexShader9_Release(current_shader);

    /* SetVertexShader() with a bogus shader vtbl */
    shader_vtbl = shader->lpVtbl;
    shader->lpVtbl = (IDirect3DVertexShader9Vtbl *)0xdeadbeef;
    hr = IDirect3DDevice9_SetVertexShader(device, shader);
    ok(SUCCEEDED(hr), "Failed to set vertex shader, hr %#x.\n", hr);
    shader->lpVtbl = NULL;
    hr = IDirect3DDevice9_SetVertexShader(device, shader);
    ok(SUCCEEDED(hr), "Failed to set vertex shader, hr %#x.\n", hr);
    shader->lpVtbl = shader_vtbl;

    IDirect3DVertexShader9_Release(shader);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_vertex_shader_constant(void)
{
    static const float d[16] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    static const float c[4] = {0.0, 0.0, 0.0, 0.0};
    IDirect3DDevice9 *device;
    struct device_desc desc;
    DWORD consts_swvp;
    IDirect3D9 *d3d;
    ULONG refcount;
    D3DCAPS9 caps;
    DWORD consts;
    HWND window;
    HRESULT hr;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (!(caps.VertexShaderVersion & 0xffff))
    {
        skip("No vertex shader support, skipping tests.\n");
        IDirect3DDevice9_Release(device);
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }
    consts = caps.MaxVertexShaderConst;

    /* A simple check that the stuff works at all. */
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 0, c, 1);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    /* Test corner cases: Write to const MAX - 1, MAX, MAX + 1, and writing 4
     * consts from MAX - 1. */
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, consts - 1, c, 1);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, consts + 0, c, 1);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, consts + 1, c, 1);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, consts - 1, d, 4);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    /* Constant -1. */
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, -1, c, 1);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);

    desc.adapter_ordinal = D3DADAPTER_DEFAULT;
    desc.device_window = window;
    desc.width = 640;
    desc.height = 480;
    desc.flags = CREATE_DEVICE_SWVP_ONLY;

    if (!(device = create_device(d3d, window, &desc)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }
    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    consts_swvp = caps.MaxVertexShaderConst;
    todo_wine
    ok(consts_swvp == 8192, "Unexpected consts_swvp %u.\n", consts_swvp);

    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, consts + 0, c, 1);
    todo_wine
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, consts + 1, c, 1);
    todo_wine
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, consts - 1, d, 4);
    todo_wine
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, consts_swvp - 1, c, 1);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, consts_swvp, c, 1);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);

    desc.flags = CREATE_DEVICE_MIXED_ONLY;
    if (!(device = create_device(d3d, window, &desc)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }
    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    ok(consts == caps.MaxVertexShaderConst, "Unexpected caps.MaxVertexShaderConst %u, consts %u.\n",
            caps.MaxVertexShaderConst, consts);

    IDirect3DDevice9_SetSoftwareVertexProcessing(device, 0);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, consts + 0, c, 1);
    todo_wine
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, consts_swvp - 1, c, 1);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    IDirect3DDevice9_SetSoftwareVertexProcessing(device, 1);

    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, consts + 0, c, 1);
    todo_wine
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexShaderConstantF(device, consts_swvp - 1, c, 1);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);

    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_get_set_pixel_shader(void)
{
    IDirect3DPixelShader9 *current_shader = NULL;
    IDirect3DPixelShader9 *shader = NULL;
    const IDirect3DPixelShader9Vtbl *shader_vtbl;
    IDirect3DDevice9 *device;
    ULONG refcount, i;
    IDirect3D9 *d3d;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (!(caps.PixelShaderVersion & 0xffff))
    {
        skip("No pixel shader support, skipping tests.\n");
        IDirect3DDevice9_Release(device);
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreatePixelShader(device, simple_ps, &shader);
    ok(SUCCEEDED(hr), "Failed to create shader, hr %#x.\n", hr);
    ok(!!shader, "Got unexpected shader %p.\n", shader);

    /* SetPixelShader() should not touch the shader's refcount. */
    i = get_refcount((IUnknown *)shader);
    hr = IDirect3DDevice9_SetPixelShader(device, shader);
    refcount = get_refcount((IUnknown *)shader);
    ok(SUCCEEDED(hr), "Failed to set pixel shader, hr %#x.\n", hr);
    ok(refcount == i, "Got unexpected refcount %u, expected %u.\n", refcount, i);

    /* GetPixelShader() should increase the shader's refcount by one. */
    i = refcount + 1;
    hr = IDirect3DDevice9_GetPixelShader(device, &current_shader);
    refcount = get_refcount((IUnknown *)shader);
    ok(SUCCEEDED(hr), "Failed to get pixel shader, hr %#x.\n", hr);
    ok(refcount == i, "Got unexpected refcount %u, expected %u.\n", refcount, i);
    ok(current_shader == shader, "Got unexpected shader %p, expected %p.\n", current_shader, shader);
    IDirect3DPixelShader9_Release(current_shader);

    /* SetPixelShader() with a bogus shader vtbl */
    shader_vtbl = shader->lpVtbl;
    shader->lpVtbl = (IDirect3DPixelShader9Vtbl *)0xdeadbeef;
    hr = IDirect3DDevice9_SetPixelShader(device, shader);
    ok(SUCCEEDED(hr), "Failed to set pixel shader, hr %#x.\n", hr);
    shader->lpVtbl = NULL;
    hr = IDirect3DDevice9_SetPixelShader(device, shader);
    ok(SUCCEEDED(hr), "Failed to set pixel shader, hr %#x.\n", hr);
    shader->lpVtbl = shader_vtbl;

    IDirect3DPixelShader9_Release(shader);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_pixel_shader_constant(void)
{
    static const float d[16] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    static const float c[4] = {0.0, 0.0, 0.0, 0.0};
    IDirect3DDevice9 *device;
    DWORD consts = 0;
    IDirect3D9 *d3d;
    ULONG refcount;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (!(caps.PixelShaderVersion & 0xffff))
    {
        skip("No pixel shader support, skipping tests.\n");
        IDirect3DDevice9_Release(device);
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    /* A simple check that the stuff works at all. */
    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 0, c, 1);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    /* Is there really no max pixel shader constant value??? Test how far I can go. */
    while (SUCCEEDED(IDirect3DDevice9_SetPixelShaderConstantF(device, consts++, c, 1)));
    consts = consts - 1;
    trace("SetPixelShaderConstantF was able to set %u shader constants.\n", consts);

    /* Test corner cases: Write 4 consts from MAX - 1, everything else is
     * pointless given the way the constant limit was determined. */
    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, consts - 1, d, 4);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    /* Constant -1. */
    hr = IDirect3DDevice9_SetPixelShaderConstantF(device, -1, c, 1);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_unsupported_shaders(void)
{
    static const DWORD vs_3_0[] =
    {
        0xfffe0300,                             /* vs_3_0               */
        0x0200001f, 0x80000000, 0x900f0000,     /* dcl_position v0      */
        0x0200001f, 0x80000000, 0xe00f0000,     /* dcl_position o0      */
        0x02000001, 0xe00f0000, 0x90e40000,     /* mov o0, v0           */
        0x0000ffff,                             /* end                  */
    };

#if 0
float4 main(const float4 color : COLOR) : SV_TARGET
{
    float4 o;

    o = color;

    return o;
}
#endif
    static const DWORD ps_4_0[] =
    {
        0x43425844, 0x4da9446f, 0xfbe1f259, 0x3fdb3009, 0x517521fa, 0x00000001, 0x000001ac, 0x00000005,
        0x00000034, 0x0000008c, 0x000000bc, 0x000000f0, 0x00000130, 0x46454452, 0x00000050, 0x00000000,
        0x00000000, 0x00000000, 0x0000001c, 0xffff0400, 0x00000100, 0x0000001c, 0x7263694d, 0x666f736f,
        0x52282074, 0x4c482029, 0x53204c53, 0x65646168, 0x6f432072, 0x6c69706d, 0x39207265, 0x2e39322e,
        0x2e323539, 0x31313133, 0xababab00, 0x4e475349, 0x00000028, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000f0f, 0x4f4c4f43, 0xabab0052, 0x4e47534f,
        0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
        0x0000000f, 0x545f5653, 0x45475241, 0xabab0054, 0x52444853, 0x00000038, 0x00000040, 0x0000000e,
        0x03001062, 0x001010f2, 0x00000000, 0x03000065, 0x001020f2, 0x00000000, 0x05000036, 0x001020f2,
        0x00000000, 0x00101e46, 0x00000000, 0x0100003e, 0x54415453, 0x00000074, 0x00000002, 0x00000000,
        0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
        0x00000000, 0x00000000, 0x00000000,
    };
#if 0
    vs_1_1
    dcl_position v0
    def c255, 1.0, 1.0, 1.0, 1.0
    add r0, v0, c255
    mov oPos, r0
#endif
    static const DWORD vs_1_255[] =
    {
        0xfffe0101,
        0x0000001f, 0x80000000, 0x900f0000,
        0x00000051, 0xa00f00ff, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000,
        0x00000002, 0x800f0000, 0x90e40000, 0xa0e400ff,
        0x00000001, 0xc00f0000, 0x80e40000,
        0x0000ffff
    };
#if 0
    vs_1_1
    dcl_position v0
    def c256, 1.0, 1.0, 1.0, 1.0
    add r0, v0, c256
    mov oPos, r0
#endif
    static const DWORD vs_1_256[] =
    {
        0xfffe0101,
        0x0000001f, 0x80000000, 0x900f0000,
        0x00000051, 0xa00f0100, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000,
        0x00000002, 0x800f0000, 0x90e40000, 0xa0e40100,
        0x00000001, 0xc00f0000, 0x80e40000,
        0x0000ffff
    };
#if 0
    vs_3_0
    dcl_position v0
    dcl_position o0
    def c256, 1.0, 1.0, 1.0, 1.0
    add r0, v0, c256
    mov o0, r0
#endif
    static const DWORD vs_3_256[] =
    {
        0xfffe0300,
        0x0200001f, 0x80000000, 0x900f0000,
        0x0200001f, 0x80000000, 0xe00f0000,
        0x05000051, 0xa00f0100, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000,
        0x03000002, 0x800f0000, 0x90e40000, 0xa0e40100,
        0x02000001, 0xe00f0000, 0x80e40000,
        0x0000ffff
    };
#if 0
    /* This shader source generates syntax errors with the native shader assembler
     * due to the constant register index values.
     * The bytecode was modified by hand to use the intended values. */
    vs_3_0
    dcl_position v0
    dcl_position o0
    defi i16, 1, 1, 1, 1
    rep i16
    add r0, r0, v0
    endrep
    mov o0, r0
#endif
    static const DWORD vs_3_i16[] =
    {
        0xfffe0300,
        0x0200001f, 0x80000000, 0x900f0000,
        0x0200001f, 0x80000000, 0xe00f0000,
        0x05000030, 0xf00f0010, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
        0x01000026, 0xf0e40010,
        0x03000002, 0x800f0000, 0x80e40000, 0x90e40000,
        0x00000027,
        0x02000001, 0xe00f0000, 0x80e40000,
        0x0000ffff
    };
#if 0
    /* This shader source generates syntax errors with the native shader assembler
     * due to the constant register index values.
     * The bytecode was modified by hand to use the intended values. */
    vs_3_0
    dcl_position v0
    dcl_position o0
    defb b16, true
    mov r0, v0
    if b16
    add r0, r0, v0
    endif
    mov o0, r0
#endif
    static const DWORD vs_3_b16[] =
    {
        0xfffe0300,
        0x0200001f, 0x80000000, 0x900f0000,
        0x0200001f, 0x80000000, 0xe00f0000,
        0x0200002f, 0xe00f0810, 0x00000001,
        0x02000001, 0x800f0000, 0x90e40000,
        0x01000028, 0xe0e40810,
        0x03000002, 0x800f0000, 0x80e40000, 0x90e40000,
        0x0000002b,
        0x02000001, 0xe00f0000, 0x80e40000,
        0x0000ffff
    };
#if 0
    /* This shader source generates syntax errors with the native shader assembler
     * due to the constant register index values.
     * The bytecode was modified by hand to use the intended values. */
    ps_1_1
    def c8, 1.0, 1.0, 1.0, 1.0
    add r0, v0, c8
#endif
    static const DWORD ps_1_8[] =
    {
        0xffff0101,
        0x00000051, 0xa00f0008, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000,
        0x00000002, 0x800f0000, 0x90e40000, 0xa0e40008,
        0x0000ffff
    };
#if 0
    /* This shader source generates syntax errors with the native shader assembler
     * due to the constant register index values.
     * The bytecode was modified by hand to use the intended values. */
    ps_2_0
    def c32, 1.0, 1.0, 1.0, 1.0
    add oC0, v0, c32
#endif
    static const DWORD ps_2_32[] =
    {
        0xffff0200,
        0x05000051, 0xa00f0020, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000,
        0x03000002, 0x800f0000, 0x90e40000, 0xa0e40020,
        0x0000ffff
    };
#if 0
    /* This shader source generates syntax errors with the native shader assembler
     * due to the constant register index values.
     * The bytecode was modified by hand to use the intended values. */
    ps_3_0
    dcl_color0 v0
    def c224, 1.0, 1.0, 1.0, 1.0
    add oC0, v0, c224
#endif
    static const DWORD ps_3_224[] =
    {
        0xffff0300,
        0x0200001f, 0x8000000a, 0x900f0000,
        0x05000051, 0xa00f00e0, 0x3f800000, 0x3f800000, 0x3f800000, 0x3f800000,
        0x03000002, 0x800f0800, 0x90e40000, 0xa0e400e0,
        0x0000ffff
    };
#if 0
    /* This shader source generates syntax errors with the native shader assembler
     * due to the constant register index values.
     * The bytecode was modified by hand to use the intended values. */
    ps_2_0
    defb b0, true
    defi i0, 1, 1, 1, 1
    rep i0
    if b0
    add r0, r0, v0
    endif
    endrep
    mov oC0, r0
#endif
    static const DWORD ps_2_0_boolint[] =
    {
        0xffff0200,
        0x0200002f, 0xe00f0800, 0x00000001,
        0x05000030, 0xf00f0000, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
        0x01000026, 0xf0e40000,
        0x01000028, 0xe0e40800,
        0x03000002, 0x800f0000, 0x80e40000, 0x90e40000,
        0x0000002b,
        0x00000027,
        0x02000001, 0x800f0800, 0x80e40000,
        0x0000ffff
    };

    IDirect3DVertexShader9 *vs = NULL;
    IDirect3DPixelShader9 *ps = NULL;
    IDirect3DDevice9 *device;
    struct device_desc desc;
    IDirect3D9 * d3d;
    ULONG refcount;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    /* These should always fail, regardless of supported shader version. */
    hr = IDirect3DDevice9_CreateVertexShader(device, simple_ps, &vs);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, simple_vs, &ps);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, ps_4_0, &ps);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (caps.VertexShaderVersion < D3DVS_VERSION(3, 0))
    {
        hr = IDirect3DDevice9_CreateVertexShader(device, vs_3_0, &vs);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
        if (caps.VertexShaderVersion <= D3DVS_VERSION(1, 1) && caps.MaxVertexShaderConst < 256)
        {
            hr = IDirect3DDevice9_CreateVertexShader(device, vs_1_255, &vs);
            ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
        }
        else
        {
            skip("GPU supports SM2+, skipping SM1 test.\n");
        }

        skip("This GPU doesn't support SM3, skipping test with shader using unsupported constants.\n");
    }
    else
    {
        skip("This GPU supports SM3, skipping unsupported shader test.\n");

        hr = IDirect3DDevice9_CreateVertexShader(device, vs_1_255, &vs);
        ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
        IDirect3DVertexShader9_Release(vs);
        hr = IDirect3DDevice9_CreateVertexShader(device, vs_1_256, &vs);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
        hr = IDirect3DDevice9_CreateVertexShader(device, vs_3_256, &vs);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
        hr = IDirect3DDevice9_CreateVertexShader(device, vs_3_i16, &vs);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
        hr = IDirect3DDevice9_CreateVertexShader(device, vs_3_b16, &vs);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    }

    if (caps.PixelShaderVersion < D3DPS_VERSION(3, 0))
    {
        skip("This GPU doesn't support SM3, skipping test with shader using unsupported constants.\n");
        goto cleanup;
    }
    hr = IDirect3DDevice9_CreatePixelShader(device, ps_1_8, &ps);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, ps_2_32, &ps);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, ps_3_224, &ps);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreatePixelShader(device, ps_2_0_boolint, &ps);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    if (ps)
        IDirect3DPixelShader9_Release(ps);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);

    desc.adapter_ordinal = D3DADAPTER_DEFAULT;
    desc.device_window = window;
    desc.width = 640;
    desc.height = 480;
    desc.flags = CREATE_DEVICE_SWVP_ONLY;

    if (!(device = create_device(d3d, window, &desc)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    vs = NULL;
    hr = IDirect3DDevice9_CreateVertexShader(device, vs_1_256, &vs);
    todo_wine
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    if (vs)
        IDirect3DVertexShader9_Release(vs);
    hr = IDirect3DDevice9_CreateVertexShader(device, vs_3_256, &vs);
    todo_wine
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    if (vs)
        IDirect3DVertexShader9_Release(vs);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);

    desc.flags = CREATE_DEVICE_MIXED_ONLY;
    if (!(device = create_device(d3d, window, &desc)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }
    hr = IDirect3DDevice9_SetSoftwareVertexProcessing(device, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateVertexShader(device, vs_1_256, &vs);
    todo_wine
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexShader(device, vs);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    if (vs)
        IDirect3DVertexShader9_Release(vs);

    hr = IDirect3DDevice9_CreateVertexShader(device, vs_3_256, &vs);
    todo_wine
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexShader(device, vs);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    if (vs)
        IDirect3DVertexShader9_Release(vs);

cleanup:
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

/* Test the default texture stage state values */
static void test_texture_stage_states(void)
{
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    unsigned int i;
    ULONG refcount;
    D3DCAPS9 caps;
    DWORD value;
    HWND window;
    HRESULT hr;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);

    for (i = 0; i < caps.MaxTextureBlendStages; ++i)
    {
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_COLOROP, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == (i ? D3DTOP_DISABLE : D3DTOP_MODULATE),
                "Got unexpected value %#x for D3DTSS_COLOROP, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_COLORARG1, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == D3DTA_TEXTURE, "Got unexpected value %#x for D3DTSS_COLORARG1, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_COLORARG2, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == D3DTA_CURRENT, "Got unexpected value %#x for D3DTSS_COLORARG2, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_ALPHAOP, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == (i ? D3DTOP_DISABLE : D3DTOP_SELECTARG1),
                "Got unexpected value %#x for D3DTSS_ALPHAOP, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_ALPHAARG1, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == D3DTA_TEXTURE, "Got unexpected value %#x for D3DTSS_ALPHAARG1, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_ALPHAARG2, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == D3DTA_CURRENT, "Got unexpected value %#x for D3DTSS_ALPHAARG2, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_BUMPENVMAT00, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(!value, "Got unexpected value %#x for D3DTSS_BUMPENVMAT00, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_BUMPENVMAT01, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(!value, "Got unexpected value %#x for D3DTSS_BUMPENVMAT01, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_BUMPENVMAT10, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(!value, "Got unexpected value %#x for D3DTSS_BUMPENVMAT10, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_BUMPENVMAT11, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(!value, "Got unexpected value %#x for D3DTSS_BUMPENVMAT11, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_TEXCOORDINDEX, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == i, "Got unexpected value %#x for D3DTSS_TEXCOORDINDEX, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_BUMPENVLSCALE, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(!value, "Got unexpected value %#x for D3DTSS_BUMPENVLSCALE, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_BUMPENVLOFFSET, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(!value, "Got unexpected value %#x for D3DTSS_BUMPENVLOFFSET, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_TEXTURETRANSFORMFLAGS, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == D3DTTFF_DISABLE,
                "Got unexpected value %#x for D3DTSS_TEXTURETRANSFORMFLAGS, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_COLORARG0, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == D3DTA_CURRENT, "Got unexpected value %#x for D3DTSS_COLORARG0, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_ALPHAARG0, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == D3DTA_CURRENT, "Got unexpected value %#x for D3DTSS_ALPHAARG0, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_RESULTARG, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(value == D3DTA_CURRENT, "Got unexpected value %#x for D3DTSS_RESULTARG, stage %u.\n", value, i);
        hr = IDirect3DDevice9_GetTextureStageState(device, i, D3DTSS_CONSTANT, &value);
        ok(SUCCEEDED(hr), "Failed to get texture stage state, hr %#x.\n", hr);
        ok(!value, "Got unexpected value %#x for D3DTSS_CONSTANT, stage %u.\n", value, i);
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_cube_texture_mipmap_gen(IDirect3DDevice9 *device)
{
    IDirect3DCubeTexture9 *texture;
    IDirect3D9 *d3d;
    HRESULT hr;

    hr = IDirect3DDevice9_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get D3D, hr %#x.\n", hr);
    hr = IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            D3DUSAGE_AUTOGENMIPMAP, D3DRTYPE_CUBETEXTURE, D3DFMT_X8R8G8B8);
    IDirect3D9_Release(d3d);
    if (hr != D3D_OK)
    {
        skip("No cube mipmap generation support, skipping tests.\n");
        return;
    }

    hr = IDirect3DDevice9_CreateCubeTexture(device, 64, 0, (D3DUSAGE_RENDERTARGET | D3DUSAGE_AUTOGENMIPMAP),
            D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    IDirect3DCubeTexture9_Release(texture);

    hr = IDirect3DDevice9_CreateCubeTexture(device, 64, 0, D3DUSAGE_AUTOGENMIPMAP,
            D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &texture, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    IDirect3DCubeTexture9_Release(texture);
}

static void test_cube_texture_levels(IDirect3DDevice9 *device)
{
    IDirect3DCubeTexture9 *texture;
    IDirect3DSurface9 *surface;
    D3DSURFACE_DESC desc;
    DWORD levels;
    HRESULT hr;
    D3DCAPS9 caps;

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    if (FAILED(IDirect3DDevice9_CreateCubeTexture(device, 64, 0, 0,
            D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL)))
    {
        skip("Failed to create cube texture, skipping tests.\n");
        return;
    }

    levels = IDirect3DCubeTexture9_GetLevelCount(texture);
    if (caps.TextureCaps & D3DPTEXTURECAPS_MIPCUBEMAP)
        ok(levels == 7, "Got unexpected levels %u.\n", levels);
    else
        ok(levels == 1, "Got unexpected levels %u.\n", levels);

    hr = IDirect3DCubeTexture9_GetLevelDesc(texture, levels - 1, &desc);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DCubeTexture9_GetLevelDesc(texture, levels, &desc);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DCubeTexture9_GetLevelDesc(texture, levels + 1, &desc);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DCubeTexture9_GetCubeMapSurface(texture, D3DCUBEMAP_FACE_POSITIVE_X, 0, &surface);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    IDirect3DSurface9_Release(surface);
    hr = IDirect3DCubeTexture9_GetCubeMapSurface(texture, D3DCUBEMAP_FACE_NEGATIVE_Z + 1, 0, &surface);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DCubeTexture9_GetCubeMapSurface(texture, D3DCUBEMAP_FACE_POSITIVE_X - 1, 0, &surface);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    IDirect3DCubeTexture9_Release(texture);
}

static void test_cube_textures(void)
{
    IDirect3DCubeTexture9 *texture;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);

    if (caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP)
    {
        hr = IDirect3DDevice9_CreateCubeTexture(device, 512, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL);
        ok(hr == D3D_OK, "Failed to create D3DPOOL_DEFAULT cube texture, hr %#x.\n", hr);
        IDirect3DCubeTexture9_Release(texture);
        hr = IDirect3DDevice9_CreateCubeTexture(device, 512, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &texture, NULL);
        ok(hr == D3D_OK, "Failed to create D3DPOOL_MANAGED cube texture, hr %#x.\n", hr);
        IDirect3DCubeTexture9_Release(texture);
        hr = IDirect3DDevice9_CreateCubeTexture(device, 512, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, &texture, NULL);
        ok(hr == D3D_OK, "Failed to create D3DPOOL_SYSTEMMEM cube texture, hr %#x.\n", hr);
        IDirect3DCubeTexture9_Release(texture);
    }
    else
    {
        hr = IDirect3DDevice9_CreateCubeTexture(device, 512, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for D3DPOOL_DEFAULT cube texture.\n", hr);
        hr = IDirect3DDevice9_CreateCubeTexture(device, 512, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &texture, NULL);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for D3DPOOL_MANAGED cube texture.\n", hr);
        hr = IDirect3DDevice9_CreateCubeTexture(device, 512, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, &texture, NULL);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for D3DPOOL_SYSTEMMEM cube texture.\n", hr);
    }
    hr = IDirect3DDevice9_CreateCubeTexture(device, 512, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_SCRATCH, &texture, NULL);
    ok(hr == D3D_OK, "Failed to create D3DPOOL_SCRATCH cube texture, hr %#x.\n", hr);
    IDirect3DCubeTexture9_Release(texture);

    test_cube_texture_mipmap_gen(device);
    test_cube_texture_levels(device);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_mipmap_gen(void)
{
    static const D3DFORMAT formats[] =
    {
        D3DFMT_A8R8G8B8,
        D3DFMT_X8R8G8B8,
        D3DFMT_A1R5G5B5,
        D3DFMT_A4R4G4B4,
        D3DFMT_A8,
        D3DFMT_L8,
        D3DFMT_A8L8,
        D3DFMT_V8U8,
        D3DFMT_DXT5,
    };
    D3DTEXTUREFILTERTYPE filter_type;
    IDirect3DTexture9 *texture;
    IDirect3DSurface9 *surface;
    IDirect3DDevice9 *device;
    unsigned int i, count;
    D3DSURFACE_DESC desc;
    D3DLOCKED_RECT lr;
    IDirect3D9 *d3d;
    BOOL renderable;
    ULONG refcount;
    DWORD levels;
    HWND window;
    HRESULT hr;

    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    for (i = 0; i < ARRAY_SIZE(formats); ++i)
    {
        hr = IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                0, D3DRTYPE_TEXTURE, formats[i]);
        if (FAILED(hr))
        {
            skip("Skipping unsupported format %#x.\n", formats[i]);
            continue;
        }
        renderable = SUCCEEDED(IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, formats[i]));
        hr = IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                D3DUSAGE_AUTOGENMIPMAP, D3DRTYPE_TEXTURE, formats[i]);
        ok((hr == D3D_OK && renderable) || hr == D3DOK_NOAUTOGEN,
                "Got unexpected hr %#x for %srenderable format %#x.\n",
                hr, renderable ? "" : "non", formats[i]);
    }

    if (IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            D3DUSAGE_AUTOGENMIPMAP, D3DRTYPE_TEXTURE, D3DFMT_X8R8G8B8) != D3D_OK)
    {
        skip("No mipmap generation support, skipping tests.\n");
        IDirect3D9_Release(d3d);
        return;
    }

    window = create_window();
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateTexture(device, 64, 64, 0, (D3DUSAGE_RENDERTARGET | D3DUSAGE_AUTOGENMIPMAP),
            D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    IDirect3DTexture9_Release(texture);

    hr = IDirect3DDevice9_CreateTexture(device, 64, 64, 0, D3DUSAGE_AUTOGENMIPMAP,
            D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &texture, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    filter_type = IDirect3DTexture9_GetAutoGenFilterType(texture);
    ok(filter_type == D3DTEXF_LINEAR, "Got unexpected filter_type %#x.\n", filter_type);
    hr = IDirect3DTexture9_SetAutoGenFilterType(texture, D3DTEXF_NONE);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DTexture9_SetAutoGenFilterType(texture, D3DTEXF_ANISOTROPIC);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    filter_type = IDirect3DTexture9_GetAutoGenFilterType(texture);
    ok(filter_type == D3DTEXF_ANISOTROPIC, "Got unexpected filter_type %#x.\n", filter_type);
    hr = IDirect3DTexture9_SetAutoGenFilterType(texture, D3DTEXF_LINEAR);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    levels = IDirect3DTexture9_GetLevelCount(texture);
    ok(levels == 1, "Got unexpected levels %u.\n", levels);

    for (i = 0; i < 6 /* 64 = 2 ^ 6 */; ++i)
    {
        surface = NULL;
        hr = IDirect3DTexture9_GetSurfaceLevel(texture, i, &surface);
        ok(hr == (i ? D3DERR_INVALIDCALL : D3D_OK), "Got unexpected hr %#x for level %u.\n", hr, i);
        if (surface)
            IDirect3DSurface9_Release(surface);

        hr = IDirect3DTexture9_GetLevelDesc(texture, i, &desc);
        ok(hr == (i ? D3DERR_INVALIDCALL : D3D_OK), "Got unexpected hr %#x for level %u.\n", hr, i);

        hr = IDirect3DTexture9_LockRect(texture, i, &lr, NULL, 0);
        ok(hr == (i ? D3DERR_INVALIDCALL : D3D_OK), "Got unexpected hr %#x for level %u.\n", hr, i);
        if (SUCCEEDED(hr))
        {
            hr = IDirect3DTexture9_UnlockRect(texture, i);
            ok(SUCCEEDED(hr), "Failed to unlock texture, hr %#x.\n", hr);
        }
    }
    IDirect3DTexture9_Release(texture);

    hr = IDirect3DDevice9_CreateTexture(device, 64, 64, 2, D3DUSAGE_AUTOGENMIPMAP,
            D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &texture, 0);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateTexture(device, 64, 64, 6, D3DUSAGE_AUTOGENMIPMAP,
            D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &texture, 0);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateTexture(device, 64, 64, 1, D3DUSAGE_AUTOGENMIPMAP,
            D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &texture, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    levels = IDirect3DTexture9_GetLevelCount(texture);
    ok(levels == 1, "Got unexpected levels %u.\n", levels);
    hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &desc);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(desc.Usage == D3DUSAGE_AUTOGENMIPMAP, "Got unexpected usage %#x.\n", desc.Usage);
    IDirect3DTexture9_Release(texture);

    hr = IDirect3DDevice9_CreateTexture(device, 64, 64, 0, D3DUSAGE_AUTOGENMIPMAP,
            D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, &texture, 0);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(formats); ++i)
    {
        hr = IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_A8R8G8B8,
                D3DUSAGE_DYNAMIC | D3DUSAGE_AUTOGENMIPMAP, D3DRTYPE_TEXTURE, formats[i]);
        ok(hr == D3DERR_NOTAVAILABLE, "Got unexpected hr %#x.\n", hr);

        hr = IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                D3DUSAGE_AUTOGENMIPMAP, D3DRTYPE_TEXTURE, formats[i]);
        if (SUCCEEDED(hr))
        {
            /* i.e. there is no difference between the D3D_OK and the
             * D3DOK_NOAUTOGEN cases. */
            hr = IDirect3DDevice9_CreateTexture(device, 64, 64, 0, D3DUSAGE_AUTOGENMIPMAP,
                    formats[i], D3DPOOL_SYSTEMMEM, &texture, 0);
            ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#x.\n", hr);

            hr = IDirect3DDevice9_CreateTexture(device, 64, 64, 0, D3DUSAGE_AUTOGENMIPMAP,
                    formats[i], D3DPOOL_DEFAULT, &texture, 0);
            ok(hr == D3D_OK, "Unexpected hr %#x.\n", hr);
            count = IDirect3DTexture9_GetLevelCount(texture);
            ok(count == 1, "Unexpected level count %u.\n", count);
            hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &desc);
            ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
            ok(desc.Usage == D3DUSAGE_AUTOGENMIPMAP, "Got unexpected usage %#x.\n", desc.Usage);
            filter_type = IDirect3DTexture9_GetAutoGenFilterType(texture);
            ok(filter_type == D3DTEXF_LINEAR, "Got unexpected filter_type %#x.\n", filter_type);
            hr = IDirect3DTexture9_SetAutoGenFilterType(texture, D3DTEXF_ANISOTROPIC);
            ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
            filter_type = IDirect3DTexture9_GetAutoGenFilterType(texture);
            ok(filter_type == D3DTEXF_ANISOTROPIC, "Got unexpected filter_type %#x.\n", filter_type);
            IDirect3DTexture9_Release(texture);
        }
    }

    hr = IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, D3DUSAGE_QUERY_WRAPANDMIP, D3DRTYPE_TEXTURE, D3DFMT_D16);
    if (hr == D3D_OK)
    {
        hr = IDirect3DDevice9_CreateTexture(device, 64, 64, 0, 0,
                D3DFMT_D16, D3DPOOL_DEFAULT, &texture, 0);
        ok(hr == D3D_OK, "Unexpected hr %#x.\n", hr);
        count = IDirect3DTexture9_GetLevelCount(texture);
        ok(count == 7, "Unexpected level count %u.\n", count);
        IDirect3DTexture9_Release(texture);

        hr = IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL,
                D3DFMT_X8R8G8B8, D3DUSAGE_AUTOGENMIPMAP, D3DRTYPE_TEXTURE, D3DFMT_D16);
        ok(hr == D3DOK_NOAUTOGEN, "Unexpected hr %#x.\n", hr);
        hr = IDirect3DDevice9_CreateTexture(device, 64, 64, 0, D3DUSAGE_AUTOGENMIPMAP,
                D3DFMT_D16, D3DPOOL_DEFAULT, &texture, 0);
        ok(hr == D3D_OK, "Unexpected hr %#x.\n", hr);
        count = IDirect3DTexture9_GetLevelCount(texture);
        ok(count == 1, "Unexpected level count %u.\n", count);
        hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &desc);
        ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
        ok(desc.Usage == D3DUSAGE_AUTOGENMIPMAP, "Got unexpected usage %#x.\n", desc.Usage);
        IDirect3DTexture9_Release(texture);
    }
    else
    {
        skip("Mipmapping not supported for D3DFMT_D16, skipping test.\n");
    }

    hr = IDirect3DDevice9_CreateVolumeTexture(device, 64, 64, 64, 0, D3DUSAGE_AUTOGENMIPMAP,
            D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, (IDirect3DVolumeTexture9 **)&texture, 0);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#x.\n", hr);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_filter(void)
{
    static const struct
    {
        DWORD magfilter, minfilter, mipfilter;
        BOOL has_texture;
        HRESULT result;
    }
    tests[] =
    {
        {D3DTEXF_NONE,   D3DTEXF_NONE,   D3DTEXF_NONE,   FALSE, D3DERR_UNSUPPORTEDTEXTUREFILTER},
        {D3DTEXF_POINT,  D3DTEXF_NONE,   D3DTEXF_NONE,   FALSE, D3DERR_UNSUPPORTEDTEXTUREFILTER},
        {D3DTEXF_NONE,   D3DTEXF_POINT,  D3DTEXF_NONE,   FALSE, D3DERR_UNSUPPORTEDTEXTUREFILTER},
        {D3DTEXF_POINT,  D3DTEXF_POINT,  D3DTEXF_NONE,   FALSE, D3D_OK                         },
        {D3DTEXF_POINT,  D3DTEXF_POINT,  D3DTEXF_POINT,  FALSE, D3D_OK                         },

        {D3DTEXF_NONE,   D3DTEXF_NONE,   D3DTEXF_NONE,   TRUE,  D3DERR_UNSUPPORTEDTEXTUREFILTER},
        {D3DTEXF_POINT,  D3DTEXF_NONE,   D3DTEXF_NONE,   TRUE,  D3DERR_UNSUPPORTEDTEXTUREFILTER},
        {D3DTEXF_POINT,  D3DTEXF_POINT,  D3DTEXF_NONE,   TRUE,  D3D_OK                         },
        {D3DTEXF_POINT,  D3DTEXF_POINT,  D3DTEXF_POINT,  TRUE,  D3D_OK                         },

        {D3DTEXF_NONE,   D3DTEXF_NONE,   D3DTEXF_NONE,   TRUE,  D3DERR_UNSUPPORTEDTEXTUREFILTER},
        {D3DTEXF_LINEAR, D3DTEXF_NONE,   D3DTEXF_NONE,   TRUE,  D3DERR_UNSUPPORTEDTEXTUREFILTER},
        {D3DTEXF_LINEAR, D3DTEXF_POINT,  D3DTEXF_NONE,   TRUE,  E_FAIL                         },
        {D3DTEXF_POINT,  D3DTEXF_LINEAR, D3DTEXF_NONE,   TRUE,  E_FAIL                         },
        {D3DTEXF_POINT,  D3DTEXF_POINT,  D3DTEXF_LINEAR, TRUE,  E_FAIL                         },
    };
    IDirect3DTexture9 *texture;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    unsigned int i;
    ULONG refcount;
    DWORD passes;
    HWND window;
    HRESULT hr;

    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    if (FAILED(IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            0, D3DRTYPE_TEXTURE, D3DFMT_A32B32G32R32F)))
    {
        skip("D3DFMT_A32B32G32R32F not supported, skipping tests.\n");
        IDirect3D9_Release(d3d);
        return;
    }

    if (SUCCEEDED(hr = IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            D3DUSAGE_QUERY_FILTER, D3DRTYPE_TEXTURE, D3DFMT_A32B32G32R32F)))
    {
        skip("D3DFMT_A32B32G32R32F supports filtering, skipping tests.\n");
        IDirect3D9_Release(d3d);
        return;
    }

    window = create_window();
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 0, 0,
            D3DFMT_A32B32G32R32F, D3DPOOL_MANAGED, &texture, NULL);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);

    /* Needed for ValidateDevice(). */
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
    ok(SUCCEEDED(hr), "Failed to set fvf, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        if (tests[i].has_texture)
        {
            hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *)texture);
            ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
        }
        else
        {
            hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
            ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
        }

        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MAGFILTER, tests[i].magfilter);
        ok(SUCCEEDED(hr), "Failed to set sampler state, hr %#x.\n", hr);
        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MINFILTER, tests[i].minfilter);
        ok(SUCCEEDED(hr), "Failed to set sampler state, hr %#x.\n", hr);
        hr = IDirect3DDevice9_SetSamplerState(device, 0, D3DSAMP_MIPFILTER, tests[i].mipfilter);
        ok(SUCCEEDED(hr), "Failed to set sampler state, hr %#x.\n", hr);

        passes = 0xdeadbeef;
        hr = IDirect3DDevice9_ValidateDevice(device, &passes);
        ok(hr == tests[i].result,
                "Got unexpected hr %#x, expected %#x (mag %#x, min %#x, mip %#x, has_texture %#x).\n",
                hr, tests[i].result, tests[i].magfilter, tests[i].minfilter,
                tests[i].mipfilter, tests[i].has_texture);
        if (SUCCEEDED(hr))
            ok(!!passes, "Got unexpected passes %#x.\n", passes);
        else
            ok(passes == 0xdeadbeef, "Got unexpected passes %#x.\n", passes);
    }

    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    IDirect3DTexture9_Release(texture);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_get_set_texture(void)
{
    const IDirect3DBaseTexture9Vtbl *texture_vtbl;
    IDirect3DBaseTexture9 *texture;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    texture = (IDirect3DBaseTexture9 *)0xdeadbeef;
    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetTexture(device, 0, &texture);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(!texture, "Got unexpected texture %p.\n", texture);

    hr = IDirect3DDevice9_CreateTexture(device, 16, 16, 1, 0, D3DFMT_A8R8G8B8,
            D3DPOOL_MANAGED, (IDirect3DTexture9 **)&texture, NULL);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    texture_vtbl = texture->lpVtbl;
    texture->lpVtbl = (IDirect3DBaseTexture9Vtbl *)0xdeadbeef;
    hr = IDirect3DDevice9_SetTexture(device, 0, texture);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    texture->lpVtbl = NULL;
    hr = IDirect3DDevice9_SetTexture(device, 0, texture);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#x.\n", hr);
    texture->lpVtbl = texture_vtbl;
    IDirect3DBaseTexture9_Release(texture);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_lod(void)
{
    IDirect3DTexture9 *texture;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    DWORD ret;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 3, 0,
            D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);

    /* SetLOD() is only supported on D3DPOOL_MANAGED textures, but doesn't
     * return a HRESULT, so it can't return a normal error. Instead, the call
     * is simply ignored. */
    ret = IDirect3DTexture9_SetLOD(texture, 0);
    ok(!ret, "Got unexpected ret %u.\n", ret);
    ret = IDirect3DTexture9_SetLOD(texture, 1);
    ok(!ret, "Got unexpected ret %u.\n", ret);
    ret = IDirect3DTexture9_SetLOD(texture, 2);
    ok(!ret, "Got unexpected ret %u.\n", ret);
    ret = IDirect3DTexture9_GetLOD(texture);
    ok(!ret, "Got unexpected ret %u.\n", ret);

    IDirect3DTexture9_Release(texture);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_surface_get_container(void)
{
    IDirect3DTexture9 *texture = NULL;
    IDirect3DSurface9 *surface = NULL;
    IDirect3DDevice9 *device;
    IUnknown *container;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 1, 0,
            D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    ok(!!texture, "Got unexpected texture %p.\n", texture);

    hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x.\n", hr);
    ok(!!surface, "Got unexpected surface %p.\n", surface);

    /* These should work... */
    container = NULL;
    hr = IDirect3DSurface9_GetContainer(surface, &IID_IUnknown, (void **)&container);
    ok(SUCCEEDED(hr), "Failed to get surface container, hr %#x.\n", hr);
    ok(container == (IUnknown *)texture, "Got unexpected container %p, expected %p.\n", container, texture);
    IUnknown_Release(container);

    container = NULL;
    hr = IDirect3DSurface9_GetContainer(surface, &IID_IDirect3DResource9, (void **)&container);
    ok(SUCCEEDED(hr), "Failed to get surface container, hr %#x.\n", hr);
    ok(container == (IUnknown *)texture, "Got unexpected container %p, expected %p.\n", container, texture);
    IUnknown_Release(container);

    container = NULL;
    hr = IDirect3DSurface9_GetContainer(surface, &IID_IDirect3DBaseTexture9, (void **)&container);
    ok(SUCCEEDED(hr), "Failed to get surface container, hr %#x.\n", hr);
    ok(container == (IUnknown *)texture, "Got unexpected container %p, expected %p.\n", container, texture);
    IUnknown_Release(container);

    container = NULL;
    hr = IDirect3DSurface9_GetContainer(surface, &IID_IDirect3DTexture9, (void **)&container);
    ok(SUCCEEDED(hr), "Failed to get surface container, hr %#x.\n", hr);
    ok(container == (IUnknown *)texture, "Got unexpected container %p, expected %p.\n", container, texture);
    IUnknown_Release(container);

    /* ...and this one shouldn't. This should return E_NOINTERFACE and set container to NULL. */
    hr = IDirect3DSurface9_GetContainer(surface, &IID_IDirect3DSurface9, (void **)&container);
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#x.\n", hr);
    ok(!container, "Got unexpected container %p.\n", container);

    IDirect3DSurface9_Release(surface);
    IDirect3DTexture9_Release(texture);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_surface_alignment(void)
{
    IDirect3DSurface9 *surface;
    IDirect3DDevice9 *device;
    D3DLOCKED_RECT lr;
    unsigned int i, j;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    /* Test a sysmem surface because those aren't affected by the hardware's np2 restrictions. */
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 5, 5,
            D3DFMT_R5G6B5, D3DPOOL_SYSTEMMEM, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirect3DSurface9_LockRect(surface, &lr, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    ok(!(lr.Pitch & 3), "Got misaligned pitch %d.\n", lr.Pitch);
    /* Some applications also depend on the exact pitch, rather than just the
     * alignment. */
    ok(lr.Pitch == 12, "Got unexpected pitch %d.\n", lr.Pitch);
    hr = IDirect3DSurface9_UnlockRect(surface);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
    IDirect3DSurface9_Release(surface);

    for (i = 0; i < 5; ++i)
    {
        IDirect3DTexture9 *texture;
        unsigned int level_count;
        D3DSURFACE_DESC desc;
        int expected_pitch;

        hr = IDirect3DDevice9_CreateTexture(device, 64, 64, 0, 0,
                MAKEFOURCC('D', 'X', 'T', '1' + i), D3DPOOL_MANAGED, &texture, NULL);
        ok(SUCCEEDED(hr) || hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
        if (FAILED(hr))
        {
            skip("DXT%u surfaces are not supported, skipping tests.\n", i + 1);
            continue;
        }

        level_count = IDirect3DBaseTexture9_GetLevelCount(texture);
        for (j = 0; j < level_count; ++j)
        {
            IDirect3DTexture9_GetLevelDesc(texture, j, &desc);
            hr = IDirect3DTexture9_LockRect(texture, j, &lr, NULL, 0);
            ok(SUCCEEDED(hr), "Failed to lock texture, hr %#x.\n", hr);
            hr = IDirect3DTexture9_UnlockRect(texture, j);
            ok(SUCCEEDED(hr), "Failed to unlock texture, hr %#x.\n", hr);

            expected_pitch = ((desc.Width + 3) >> 2) << 3;
            if (i > 0)
                expected_pitch <<= 1;
            ok(lr.Pitch == expected_pitch, "Got unexpected pitch %d for DXT%u level %u (%ux%u), expected %d.\n",
                    lr.Pitch, i + 1, j, desc.Width, desc.Height, expected_pitch);
        }
        IDirect3DTexture9_Release(texture);
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

/* Since the DXT formats are based on 4x4 blocks, locking works slightly
 * different from regular formats. This test verifies we return the correct
 * memory offsets. */
static void test_lockrect_offset(void)
{
    static const struct
    {
        D3DFORMAT format;
        const char *name;
        unsigned int block_width;
        unsigned int block_height;
        unsigned int block_size;
    }
    dxt_formats[] =
    {
        {D3DFMT_DXT1,                 "D3DFMT_DXT1", 4, 4, 8},
        {D3DFMT_DXT2,                 "D3DFMT_DXT2", 4, 4, 16},
        {D3DFMT_DXT3,                 "D3DFMT_DXT3", 4, 4, 16},
        {D3DFMT_DXT4,                 "D3DFMT_DXT4", 4, 4, 16},
        {D3DFMT_DXT5,                 "D3DFMT_DXT5", 4, 4, 16},
        {MAKEFOURCC('A','T','I','1'), "ATI1N",       1, 1,  1},
        {MAKEFOURCC('A','T','I','2'), "ATI2N",       1, 1,  1},
    };
    unsigned int expected_offset, offset, i;
    const RECT rect = {60, 60, 68, 68};
    IDirect3DSurface9 *surface;
    D3DLOCKED_RECT locked_rect;
    IDirect3DDevice9 *device;
    int expected_pitch;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    BYTE *base;
    HRESULT hr;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    for (i = 0; i < ARRAY_SIZE(dxt_formats); ++i)
    {
        if (FAILED(IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                0, D3DRTYPE_TEXTURE, dxt_formats[i].format)))
        {
            skip("Format %s not supported, skipping lockrect offset tests.\n", dxt_formats[i].name);
            continue;
        }

        hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 128, 128,
                dxt_formats[i].format, D3DPOOL_SCRATCH, &surface, NULL);
        ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

        hr = IDirect3DSurface9_LockRect(surface, &locked_rect, NULL, 0);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);

        base = locked_rect.pBits;
        expected_pitch = (128 + dxt_formats[i].block_height - 1) / dxt_formats[i].block_width
                * dxt_formats[i].block_size;
        ok(locked_rect.Pitch == expected_pitch, "Got unexpected pitch %d for format %s, expected %d.\n",
                locked_rect.Pitch, dxt_formats[i].name, expected_pitch);

        hr = IDirect3DSurface9_UnlockRect(surface);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

        hr = IDirect3DSurface9_LockRect(surface, &locked_rect, &rect, 0);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);

        offset = (BYTE *)locked_rect.pBits - base;
        expected_offset = (rect.top / dxt_formats[i].block_height) * expected_pitch
                + (rect.left / dxt_formats[i].block_width) * dxt_formats[i].block_size;
        ok(offset == expected_offset, "Got unexpected offset %u for format %s, expected %u.\n",
                offset, dxt_formats[i].name, expected_offset);

        hr = IDirect3DSurface9_UnlockRect(surface);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

        IDirect3DSurface9_Release(surface);
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_lockrect_invalid(void)
{
    static const struct
    {
        RECT rect;
        HRESULT win7_result;
    }
    test_data[] =
    {
        {{60, 60, 68, 68},      D3D_OK},                /* Valid */
        {{60, 60, 60, 68},      D3DERR_INVALIDCALL},    /* 0 height */
        {{60, 60, 68, 60},      D3DERR_INVALIDCALL},    /* 0 width */
        {{68, 60, 60, 68},      D3DERR_INVALIDCALL},    /* left > right */
        {{60, 68, 68, 60},      D3DERR_INVALIDCALL},    /* top > bottom */
        {{-8, 60,  0, 68},      D3DERR_INVALIDCALL},    /* left < surface */
        {{60, -8, 68,  0},      D3DERR_INVALIDCALL},    /* top < surface */
        {{-16, 60, -8, 68},     D3DERR_INVALIDCALL},    /* right < surface */
        {{60, -16, 68, -8},     D3DERR_INVALIDCALL},    /* bottom < surface */
        {{60, 60, 136, 68},     D3DERR_INVALIDCALL},    /* right > surface */
        {{60, 60, 68, 136},     D3DERR_INVALIDCALL},    /* bottom > surface */
        {{136, 60, 144, 68},    D3DERR_INVALIDCALL},    /* left > surface */
        {{60, 136, 68, 144},    D3DERR_INVALIDCALL},    /* top > surface */
    };
    static const RECT test_rect_2 = {0, 0, 8, 8};
    IDirect3DSurface9 *surface = NULL;
    D3DLOCKED_RECT locked_rect;
    IDirect3DDevice9 *device;
    IDirect3DTexture9 *texture;
    IDirect3DCubeTexture9 *cube_texture;
    HRESULT hr, expected_hr;
    unsigned int i, r;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    BYTE *base;
    static const struct
    {
        D3DRESOURCETYPE type;
        D3DPOOL pool;
        const char *name;
    }
    resources[] =
    {
        {D3DRTYPE_SURFACE, D3DPOOL_SCRATCH, "scratch surface"},
        {D3DRTYPE_TEXTURE, D3DPOOL_MANAGED, "managed texture"},
        {D3DRTYPE_TEXTURE, D3DPOOL_SYSTEMMEM, "sysmem texture"},
        {D3DRTYPE_TEXTURE, D3DPOOL_SCRATCH, "scratch texture"},
        {D3DRTYPE_CUBETEXTURE, D3DPOOL_MANAGED, "default cube texture"},
        {D3DRTYPE_CUBETEXTURE, D3DPOOL_SYSTEMMEM, "sysmem cube texture"},
        {D3DRTYPE_CUBETEXTURE, D3DPOOL_SCRATCH, "scratch cube texture"},
    };

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    for (r = 0; r < ARRAY_SIZE(resources); ++r)
    {
        texture = NULL;
        cube_texture = NULL;
        switch (resources[r].type)
        {
            case D3DRTYPE_SURFACE:
                hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 128, 128,
                        D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &surface, NULL);
                ok(SUCCEEDED(hr), "Failed to create surface, hr %#x, type %s.\n",
                        hr, resources[r].name);
                break;

            case D3DRTYPE_TEXTURE:
                hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 1, 0, D3DFMT_A8R8G8B8,
                        resources[r].pool, &texture, NULL);
                ok(SUCCEEDED(hr), "Failed to create texture, hr %#x, type %s.\n",
                        hr, resources[r].name);
                hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
                ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x, type %s.\n",
                        hr, resources[r].name);
                break;

            case D3DRTYPE_CUBETEXTURE:
                hr = IDirect3DDevice9_CreateCubeTexture(device, 128, 1, 0, D3DFMT_A8R8G8B8,
                        resources[r].pool, &cube_texture, NULL);
                ok(SUCCEEDED(hr), "Failed to create cube texture, hr %#x, type %s.\n",
                        hr, resources[r].name);
                hr = IDirect3DCubeTexture9_GetCubeMapSurface(cube_texture,
                        D3DCUBEMAP_FACE_NEGATIVE_X, 0, &surface);
                ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x, type %s.\n",
                        hr, resources[r].name);
                break;

            default:
                break;
        }

        hr = IDirect3DSurface9_LockRect(surface, &locked_rect, NULL, 0);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x, type %s.\n", hr, resources[r].name);
        base = locked_rect.pBits;
        hr = IDirect3DSurface9_UnlockRect(surface);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x, type %s.\n", hr, resources[r].name);
        expected_hr = resources[r].type == D3DRTYPE_TEXTURE ? D3D_OK : D3DERR_INVALIDCALL;
        hr = IDirect3DSurface9_UnlockRect(surface);
        ok(hr == expected_hr, "Got hr %#x, expected %#x, type %s.\n", hr, expected_hr, resources[r].name);

        for (i = 0; i < ARRAY_SIZE(test_data); ++i)
        {
            unsigned int offset, expected_offset;
            const RECT *rect = &test_data[i].rect;

            locked_rect.pBits = (BYTE *)0xdeadbeef;
            locked_rect.Pitch = 0xdeadbeef;

            hr = IDirect3DSurface9_LockRect(surface, &locked_rect, rect, 0);
            /* Windows XP accepts invalid locking rectangles, windows 7 rejects
             * them. Some games (C&C3) depend on the XP behavior, mark the Win 7
             * one broken. */
            ok(SUCCEEDED(hr) || broken(hr == test_data[i].win7_result),
                    "Failed to lock surface with rect %s, hr %#x, type %s.\n",
                    wine_dbgstr_rect(rect), hr, resources[r].name);
            if (FAILED(hr))
                continue;

            offset = (BYTE *)locked_rect.pBits - base;
            expected_offset = rect->top * locked_rect.Pitch + rect->left * 4;
            ok(offset == expected_offset,
                    "Got unexpected offset %u (expected %u) for rect %s, type %s.\n",
                    offset, expected_offset, wine_dbgstr_rect(rect), resources[r].name);

            hr = IDirect3DSurface9_UnlockRect(surface);
            ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x, type %s.\n", hr, resources[r].name);

            if (texture)
            {
                hr = IDirect3DTexture9_LockRect(texture, 0, &locked_rect, rect, 0);
                ok(SUCCEEDED(hr),
                        "Failed to lock texture with rect %s, hr %#x, type %s.\n",
                        wine_dbgstr_rect(rect), hr, resources[r].name);
                if (FAILED(hr))
                    continue;

                offset = (BYTE *)locked_rect.pBits - base;
                expected_offset = rect->top * locked_rect.Pitch + rect->left * 4;
                ok(offset == expected_offset,
                        "Got unexpected offset %u (expected %u) for rect %s, type %s.\n",
                        offset, expected_offset, wine_dbgstr_rect(rect), resources[r].name);

                hr = IDirect3DTexture9_UnlockRect(texture, 0);
                ok(SUCCEEDED(hr), "Failed to unlock texture, hr %#x, type %s.\n", hr, resources[r].name);
            }
            if (cube_texture)
            {
                hr = IDirect3DCubeTexture9_LockRect(cube_texture, D3DCUBEMAP_FACE_NEGATIVE_X, 0,
                        &locked_rect, rect, 0);
                ok(SUCCEEDED(hr),
                        "Failed to lock texture with rect %s, hr %#x, type %s.\n",
                        wine_dbgstr_rect(rect), hr, resources[r].name);
                if (FAILED(hr))
                    continue;

                offset = (BYTE *)locked_rect.pBits - base;
                expected_offset = rect->top * locked_rect.Pitch + rect->left * 4;
                ok(offset == expected_offset,
                        "Got unexpected offset %u (expected %u) for rect %s, type %s.\n",
                        offset, expected_offset, wine_dbgstr_rect(rect), resources[r].name);

                hr = IDirect3DCubeTexture9_UnlockRect(cube_texture, D3DCUBEMAP_FACE_NEGATIVE_X, 0);
                ok(SUCCEEDED(hr), "Failed to unlock texture, hr %#x, type %s.\n", hr, resources[r].name);
            }
        }

        hr = IDirect3DSurface9_LockRect(surface, &locked_rect, NULL, 0);
        ok(SUCCEEDED(hr), "Failed to lock surface with rect NULL, hr %#x, type %s.\n", hr, resources[r].name);
        locked_rect.pBits = (BYTE *)0xdeadbeef;
        locked_rect.Pitch = 1;
        hr = IDirect3DSurface9_LockRect(surface, &locked_rect, NULL, 0);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x, type %s.\n", hr, resources[r].name);
        ok(locked_rect.pBits == (BYTE *)0xdeadbeef, "Got unexpected pBits: %p, type %s.\n",
                locked_rect.pBits, resources[r].name);
        ok(locked_rect.Pitch == 1, "Got unexpected pitch %d, type %s.\n",
                locked_rect.Pitch, resources[r].name);
        hr = IDirect3DSurface9_UnlockRect(surface);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x, type %s.\n", hr, resources[r].name);

        hr = IDirect3DSurface9_LockRect(surface, &locked_rect, &test_data[0].rect, 0);
        ok(hr == D3D_OK, "Got unexpected hr %#x for rect %s, type %s.\n",
                hr, wine_dbgstr_rect(&test_data[0].rect), resources[r].name);
        hr = IDirect3DSurface9_LockRect(surface, &locked_rect, &test_data[0].rect, 0);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for rect %s, type %s.\n",
                hr, wine_dbgstr_rect(&test_data[0].rect), resources[r].name);
        hr = IDirect3DSurface9_LockRect(surface, &locked_rect, &test_rect_2, 0);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for rect %s, type %s.\n",
                hr, wine_dbgstr_rect(&test_rect_2), resources[r].name);
        hr = IDirect3DSurface9_UnlockRect(surface);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x, type %s.\n", hr, resources[r].name);

        IDirect3DSurface9_Release(surface);

        if (texture)
        {
            hr = IDirect3DTexture9_LockRect(texture, 0, &locked_rect, NULL, 0);
            ok(SUCCEEDED(hr), "Failed to lock texture with rect NULL, hr %#x, type %s.\n",
                    hr, resources[r].name);
            locked_rect.pBits = (BYTE *)0xdeadbeef;
            locked_rect.Pitch = 1;
            hr = IDirect3DTexture9_LockRect(texture, 0, &locked_rect, NULL, 0);
            ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x, type %s.\n", hr, resources[r].name);
            ok(locked_rect.pBits == (BYTE *)0xdeadbeef, "Got unexpected pBits: %p, type %s.\n",
                    locked_rect.pBits, resources[r].name);
            ok(locked_rect.Pitch == 1, "Got unexpected pitch %d, type %s.\n",
                    locked_rect.Pitch, resources[r].name);
            hr = IDirect3DSurface9_LockRect(surface, &locked_rect, NULL, 0);
            ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x, type %s.\n", hr, resources[r].name);
            hr = IDirect3DTexture9_UnlockRect(texture, 0);
            ok(SUCCEEDED(hr), "Failed to unlock texture, hr %#x, type %s.\n", hr, resources[r].name);
            hr = IDirect3DTexture9_UnlockRect(texture, 0);
            ok(hr == D3D_OK, "Got unexpected hr %#x, type %s.\n", hr, resources[r].name);

            hr = IDirect3DTexture9_LockRect(texture, 0, &locked_rect, &test_data[0].rect, 0);
            ok(hr == D3D_OK, "Got unexpected hr %#x for rect %s, type %s.\n",
                    hr, wine_dbgstr_rect(&test_data[0].rect), resources[r].name);
            hr = IDirect3DTexture9_LockRect(texture, 0, &locked_rect, &test_data[0].rect, 0);
            ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for rect %s, type %s.\n",
                    hr, wine_dbgstr_rect(&test_data[0].rect), resources[r].name);
            hr = IDirect3DTexture9_LockRect(texture, 0, &locked_rect, &test_rect_2, 0);
            ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for rect %s, type %s.\n",
                    hr, wine_dbgstr_rect(&test_rect_2), resources[r].name);
            hr = IDirect3DTexture9_UnlockRect(texture, 0);
            ok(SUCCEEDED(hr), "Failed to unlock texture, hr %#x, type %s.\n", hr, resources[r].name);

            IDirect3DTexture9_Release(texture);

            hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 1, D3DUSAGE_WRITEONLY,
                    D3DFMT_A8R8G8B8, resources[r].pool, &texture, NULL);
            ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for type %s.\n",
                    hr, resources[r].name);
        }
        if (cube_texture)
        {
            hr = IDirect3DCubeTexture9_LockRect(cube_texture, D3DCUBEMAP_FACE_NEGATIVE_X, 0,
                    &locked_rect, NULL, 0);
            ok(SUCCEEDED(hr), "Failed to lock texture with rect NULL, hr %#x, type %s.\n",
                    hr, resources[r].name);
            locked_rect.pBits = (BYTE *)0xdeadbeef;
            locked_rect.Pitch = 1;
            hr = IDirect3DCubeTexture9_LockRect(cube_texture, D3DCUBEMAP_FACE_NEGATIVE_X, 0,
                    &locked_rect, NULL, 0);
            ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x, type %s.\n", hr, resources[r].name);
            ok(locked_rect.pBits == (BYTE *)0xdeadbeef, "Got unexpected pBits: %p, type %s.\n",
                    locked_rect.pBits, resources[r].name);
            ok(locked_rect.Pitch == 1, "Got unexpected pitch %d, type %s.\n",
                    locked_rect.Pitch, resources[r].name);
            hr = IDirect3DSurface9_LockRect(surface, &locked_rect, NULL, 0);
            ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x, type %s.\n", hr, resources[r].name);
            hr = IDirect3DCubeTexture9_UnlockRect(cube_texture, D3DCUBEMAP_FACE_NEGATIVE_X, 0);
            ok(SUCCEEDED(hr), "Failed to unlock texture, hr %#x, type %s.\n", hr, resources[r].name);
            hr = IDirect3DCubeTexture9_UnlockRect(cube_texture, D3DCUBEMAP_FACE_NEGATIVE_X, 0);
            todo_wine ok(hr == D3D_OK, "Got unexpected hr %#x, type %s.\n", hr, resources[r].name);

            hr = IDirect3DCubeTexture9_LockRect(cube_texture, D3DCUBEMAP_FACE_NEGATIVE_X, 0,
                    &locked_rect, &test_data[0].rect, 0);
            ok(hr == D3D_OK, "Got unexpected hr %#x for rect %s, type %s.\n",
                    hr, wine_dbgstr_rect(&test_data[0].rect), resources[r].name);
            hr = IDirect3DCubeTexture9_LockRect(cube_texture, D3DCUBEMAP_FACE_NEGATIVE_X, 0,
                    &locked_rect, &test_data[0].rect, 0);
            ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for rect %s, type %s.\n",
                    hr, wine_dbgstr_rect(&test_data[0].rect), resources[r].name);
            hr = IDirect3DCubeTexture9_LockRect(cube_texture, D3DCUBEMAP_FACE_NEGATIVE_X, 0,
                    &locked_rect, &test_rect_2, 0);
            ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for rect %s, type %s.\n",
                    hr, wine_dbgstr_rect(&test_rect_2), resources[r].name);
            hr = IDirect3DCubeTexture9_UnlockRect(cube_texture, D3DCUBEMAP_FACE_NEGATIVE_X, 0);
            ok(SUCCEEDED(hr), "Failed to unlock texture, hr %#x, type %s.\n", hr, resources[r].name);

            IDirect3DCubeTexture9_Release(cube_texture);

            hr = IDirect3DDevice9_CreateCubeTexture(device, 128, 1, D3DUSAGE_WRITEONLY, D3DFMT_A8R8G8B8,
                    resources[r].pool, &cube_texture, NULL);
            ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for type %s.\n",
                    hr, resources[r].name);
        }
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_private_data(void)
{
    ULONG refcount, expected_refcount;
    IDirect3DTexture9 *texture;
    IDirect3DSurface9 *surface, *surface2;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    IUnknown *ptr;
    HWND window;
    HRESULT hr;
    DWORD size;
    DWORD data[4] = {1, 2, 3, 4};
    static const GUID d3d9_private_data_test_guid2 =
    {
        0x2e5afac2,
        0x87b5,
        0x4c10,
        {0x9b,0x4b,0x89,0xd7,0xd1,0x12,0xe7,0x2b}
    };

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 4, 4,
            D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);

    hr = IDirect3DSurface9_SetPrivateData(surface, &d3d9_private_data_test_guid,
            device, 0, D3DSPD_IUNKNOWN);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DSurface9_SetPrivateData(surface, &d3d9_private_data_test_guid,
            device, 5, D3DSPD_IUNKNOWN);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DSurface9_SetPrivateData(surface, &d3d9_private_data_test_guid,
            device, sizeof(IUnknown *) * 2, D3DSPD_IUNKNOWN);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    /* A failing SetPrivateData call does not clear the old data with the same tag. */
    hr = IDirect3DSurface9_SetPrivateData(surface, &d3d9_private_data_test_guid, device,
            sizeof(device), D3DSPD_IUNKNOWN);
    ok(SUCCEEDED(hr), "Failed to set private data, hr %#x.\n", hr);
    hr = IDirect3DSurface9_SetPrivateData(surface, &d3d9_private_data_test_guid, device,
            sizeof(device) * 2, D3DSPD_IUNKNOWN);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    size = sizeof(ptr);
    hr = IDirect3DSurface9_GetPrivateData(surface, &d3d9_private_data_test_guid, &ptr, &size);
    ok(SUCCEEDED(hr), "Failed to get private data, hr %#x.\n", hr);
    IUnknown_Release(ptr);
    hr = IDirect3DSurface9_FreePrivateData(surface, &d3d9_private_data_test_guid);
    ok(SUCCEEDED(hr), "Failed to free private data, hr %#x.\n", hr);

    refcount = get_refcount((IUnknown *)device);
    hr = IDirect3DSurface9_SetPrivateData(surface, &d3d9_private_data_test_guid,
            device, sizeof(IUnknown *), D3DSPD_IUNKNOWN);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    expected_refcount = refcount + 1;
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    hr = IDirect3DSurface9_FreePrivateData(surface, &d3d9_private_data_test_guid);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    expected_refcount = refcount - 1;
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);

    hr = IDirect3DSurface9_SetPrivateData(surface, &d3d9_private_data_test_guid,
            device, sizeof(IUnknown *), D3DSPD_IUNKNOWN);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DSurface9_SetPrivateData(surface, &d3d9_private_data_test_guid,
            surface, sizeof(IUnknown *), D3DSPD_IUNKNOWN);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);

    hr = IDirect3DSurface9_SetPrivateData(surface, &d3d9_private_data_test_guid,
            device, sizeof(IUnknown *), D3DSPD_IUNKNOWN);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    size = 2 * sizeof(ptr);
    hr = IDirect3DSurface9_GetPrivateData(surface, &d3d9_private_data_test_guid, &ptr, &size);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(size == sizeof(device), "Got unexpected size %u.\n", size);
    expected_refcount = refcount + 2;
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    ok(ptr == (IUnknown *)device, "Got unexpected ptr %p, expected %p.\n", ptr, device);
    IUnknown_Release(ptr);
    expected_refcount--;

    ptr = (IUnknown *)0xdeadbeef;
    size = 1;
    hr = IDirect3DSurface9_GetPrivateData(surface, &d3d9_private_data_test_guid, NULL, &size);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(size == sizeof(device), "Got unexpected size %u.\n", size);
    size = 2 * sizeof(ptr);
    hr = IDirect3DSurface9_GetPrivateData(surface, &d3d9_private_data_test_guid, NULL, &size);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(size == sizeof(device), "Got unexpected size %u.\n", size);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);
    size = 1;
    hr = IDirect3DSurface9_GetPrivateData(surface, &d3d9_private_data_test_guid, &ptr, &size);
    ok(hr == D3DERR_MOREDATA, "Got unexpected hr %#x.\n", hr);
    ok(size == sizeof(device), "Got unexpected size %u.\n", size);
    ok(ptr == (IUnknown *)0xdeadbeef, "Got unexpected pointer %p.\n", ptr);
    hr = IDirect3DSurface9_GetPrivateData(surface, &d3d9_private_data_test_guid2, NULL, NULL);
    ok(hr == D3DERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);
    size = 0xdeadbabe;
    hr = IDirect3DSurface9_GetPrivateData(surface, &d3d9_private_data_test_guid2, &ptr, &size);
    ok(hr == D3DERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);
    ok(ptr == (IUnknown *)0xdeadbeef, "Got unexpected pointer %p.\n", ptr);
    ok(size == 0xdeadbabe, "Got unexpected size %u.\n", size);
    /* GetPrivateData with size = NULL causes an access violation on Windows if the
     * requested data exists. */

    /* Destroying the surface frees the held reference. */
    IDirect3DSurface9_Release(surface);
    expected_refcount = refcount - 2;
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %u, expected %u.\n", refcount, expected_refcount);

    hr = IDirect3DDevice9_CreateTexture(device, 4, 4, 2, 0, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH,
            &texture, NULL);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    ok(SUCCEEDED(hr), "Failed to get texture level 0, hr %#x.\n", hr);
    hr = IDirect3DTexture9_GetSurfaceLevel(texture, 1, &surface2);
    ok(SUCCEEDED(hr), "Failed to get texture level 1, hr %#x.\n", hr);

    hr = IDirect3DTexture9_SetPrivateData(texture, &d3d9_private_data_test_guid, data, sizeof(data), 0);
    ok(SUCCEEDED(hr), "Failed to set private data, hr %#x.\n", hr);

    memset(data, 0, sizeof(data));
    size = sizeof(data);
    hr = IDirect3DSurface9_GetPrivateData(surface, &d3d9_private_data_test_guid, data, &size);
    ok(hr == D3DERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DTexture9_GetPrivateData(texture, &d3d9_private_data_test_guid, data, &size);
    ok(SUCCEEDED(hr), "Failed to get private data, hr %#x.\n", hr);
    ok(data[0] == 1 && data[1] == 2 && data[2] == 3 && data[3] == 4,
            "Got unexpected private data: %u, %u, %u, %u.\n", data[0], data[1], data[2], data[3]);

    hr = IDirect3DTexture9_FreePrivateData(texture, &d3d9_private_data_test_guid);
    ok(SUCCEEDED(hr), "Failed to free private data, hr %#x.\n", hr);

    hr = IDirect3DSurface9_SetPrivateData(surface, &d3d9_private_data_test_guid, data, sizeof(data), 0);
    ok(SUCCEEDED(hr), "Failed to set private data, hr %#x.\n", hr);
    hr = IDirect3DSurface9_GetPrivateData(surface2, &d3d9_private_data_test_guid, data, &size);
    ok(hr == D3DERR_NOTFOUND, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DSurface9_FreePrivateData(surface, &d3d9_private_data_test_guid);
    ok(SUCCEEDED(hr), "Failed to free private data, hr %#x.\n", hr);

    IDirect3DSurface9_Release(surface2);
    IDirect3DSurface9_Release(surface);
    IDirect3DTexture9_Release(texture);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_getdc(void)
{
    static const struct
    {
        const char *name;
        D3DFORMAT format;
        unsigned int bit_count;
        DWORD mask_r, mask_g, mask_b;
        BOOL getdc_supported;
    }
    testdata[] =
    {
        {"A8R8G8B8",    D3DFMT_A8R8G8B8,    32, 0x00000000, 0x00000000, 0x00000000, TRUE },
        {"X8R8G8B8",    D3DFMT_X8R8G8B8,    32, 0x00000000, 0x00000000, 0x00000000, TRUE },
        {"R5G6B5",      D3DFMT_R5G6B5,      16, 0x0000f800, 0x000007e0, 0x0000001f, TRUE },
        {"X1R5G5B5",    D3DFMT_X1R5G5B5,    16, 0x00007c00, 0x000003e0, 0x0000001f, TRUE },
        {"A1R5G5B5",    D3DFMT_A1R5G5B5,    16, 0x00007c00, 0x000003e0, 0x0000001f, TRUE },
        {"R8G8B8",      D3DFMT_R8G8B8,      24, 0x00000000, 0x00000000, 0x00000000, TRUE },
        {"A2R10G10B10", D3DFMT_A2R10G10B10, 32, 0x00000000, 0x00000000, 0x00000000, FALSE},
        {"V8U8",        D3DFMT_V8U8,        16, 0x00000000, 0x00000000, 0x00000000, FALSE},
        {"Q8W8V8U8",    D3DFMT_Q8W8V8U8,    32, 0x00000000, 0x00000000, 0x00000000, FALSE},
        {"A8B8G8R8",    D3DFMT_A8B8G8R8,    32, 0x00000000, 0x00000000, 0x00000000, FALSE},
        {"X8B8G8R8",    D3DFMT_A8B8G8R8,    32, 0x00000000, 0x00000000, 0x00000000, FALSE},
        {"R3G3B2",      D3DFMT_R3G3B2,      8,  0x00000000, 0x00000000, 0x00000000, FALSE},
        {"P8",          D3DFMT_P8,          8,  0x00000000, 0x00000000, 0x00000000, FALSE},
        {"L8",          D3DFMT_L8,          8,  0x00000000, 0x00000000, 0x00000000, FALSE},
        {"A8L8",        D3DFMT_A8L8,        16, 0x00000000, 0x00000000, 0x00000000, FALSE},
        {"DXT1",        D3DFMT_DXT1,        4,  0x00000000, 0x00000000, 0x00000000, FALSE},
        {"DXT2",        D3DFMT_DXT2,        8,  0x00000000, 0x00000000, 0x00000000, FALSE},
        {"DXT3",        D3DFMT_DXT3,        8,  0x00000000, 0x00000000, 0x00000000, FALSE},
        {"DXT4",        D3DFMT_DXT4,        8,  0x00000000, 0x00000000, 0x00000000, FALSE},
        {"DXT5",        D3DFMT_DXT5,        8,  0x00000000, 0x00000000, 0x00000000, FALSE},
    };
    IDirect3DSurface9 *surface, *surface2;
    IDirect3DCubeTexture9 *cube_texture;
    struct device_desc device_desc;
    IDirect3DTexture9 *texture;
    IDirect3DDevice9 *device;
    D3DLOCKED_RECT map_desc;
    IDirect3D9 *d3d;
    unsigned int i;
    ULONG refcount;
    HWND window;
    HDC dc, dc2;
    HRESULT hr;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    for (i = 0; i < ARRAY_SIZE(testdata); ++i)
    {
        texture = NULL;
        hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 64, 64,
                testdata[i].format, D3DPOOL_SYSTEMMEM, &surface, NULL);
        if (FAILED(hr))
        {
            hr = IDirect3DDevice9_CreateTexture(device, 64, 64, 1, 0,
                    testdata[i].format, D3DPOOL_MANAGED, &texture, NULL);
            if (FAILED(hr))
            {
                skip("Failed to create surface for format %s (hr %#x), skipping tests.\n", testdata[i].name, hr);
                continue;
            }
            hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
            ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x.\n", hr);
        }

        dc = (void *)0x1234;
        hr = IDirect3DSurface9_GetDC(surface, &dc);
        if (testdata[i].getdc_supported)
            ok(SUCCEEDED(hr), "Got unexpected hr %#x for format %s.\n", hr, testdata[i].name);
        else
            ok(FAILED(hr), "Got unexpected hr %#x for format %s.\n", hr, testdata[i].name);

        if (SUCCEEDED(hr))
        {
            unsigned int width_bytes;
            DIBSECTION dib;
            HBITMAP bitmap;
            DWORD type;
            int size;

            type = GetObjectType(dc);
            ok(type == OBJ_MEMDC, "Got unexpected object type %#x for format %s.\n", type, testdata[i].name);
            bitmap = GetCurrentObject(dc, OBJ_BITMAP);
            type = GetObjectType(bitmap);
            ok(type == OBJ_BITMAP, "Got unexpected object type %#x for format %s.\n", type, testdata[i].name);

            size = GetObjectA(bitmap, sizeof(dib), &dib);
            ok(size == sizeof(dib), "Got unexpected size %d for format %s.\n", size, testdata[i].name);
            ok(!dib.dsBm.bmType, "Got unexpected type %#x for format %s.\n",
                    dib.dsBm.bmType, testdata[i].name);
            ok(dib.dsBm.bmWidth == 64, "Got unexpected width %d for format %s.\n",
                    dib.dsBm.bmWidth, testdata[i].name);
            ok(dib.dsBm.bmHeight == 64, "Got unexpected height %d for format %s.\n",
                    dib.dsBm.bmHeight, testdata[i].name);
            width_bytes = ((dib.dsBm.bmWidth * testdata[i].bit_count + 31) >> 3) & ~3;
            ok(dib.dsBm.bmWidthBytes == width_bytes, "Got unexpected width bytes %d for format %s.\n",
                    dib.dsBm.bmWidthBytes, testdata[i].name);
            ok(dib.dsBm.bmPlanes == 1, "Got unexpected plane count %d for format %s.\n",
                    dib.dsBm.bmPlanes, testdata[i].name);
            ok(dib.dsBm.bmBitsPixel == testdata[i].bit_count,
                    "Got unexpected bit count %d for format %s.\n",
                    dib.dsBm.bmBitsPixel, testdata[i].name);
            ok(!!dib.dsBm.bmBits, "Got unexpected bits %p for format %s.\n",
                    dib.dsBm.bmBits, testdata[i].name);

            ok(dib.dsBmih.biSize == sizeof(dib.dsBmih), "Got unexpected size %u for format %s.\n",
                    dib.dsBmih.biSize, testdata[i].name);
            ok(dib.dsBmih.biWidth == 64, "Got unexpected width %d for format %s.\n",
                    dib.dsBmih.biHeight, testdata[i].name);
            ok(dib.dsBmih.biHeight == 64, "Got unexpected height %d for format %s.\n",
                    dib.dsBmih.biHeight, testdata[i].name);
            ok(dib.dsBmih.biPlanes == 1, "Got unexpected plane count %u for format %s.\n",
                    dib.dsBmih.biPlanes, testdata[i].name);
            ok(dib.dsBmih.biBitCount == testdata[i].bit_count, "Got unexpected bit count %u for format %s.\n",
                    dib.dsBmih.biBitCount, testdata[i].name);
            ok(dib.dsBmih.biCompression == (testdata[i].bit_count == 16 ? BI_BITFIELDS : BI_RGB),
                    "Got unexpected compression %#x for format %s.\n",
                    dib.dsBmih.biCompression, testdata[i].name);
            ok(!dib.dsBmih.biSizeImage, "Got unexpected image size %u for format %s.\n",
                    dib.dsBmih.biSizeImage, testdata[i].name);
            ok(!dib.dsBmih.biXPelsPerMeter, "Got unexpected horizontal resolution %d for format %s.\n",
                    dib.dsBmih.biXPelsPerMeter, testdata[i].name);
            ok(!dib.dsBmih.biYPelsPerMeter, "Got unexpected vertical resolution %d for format %s.\n",
                    dib.dsBmih.biYPelsPerMeter, testdata[i].name);
            ok(!dib.dsBmih.biClrUsed, "Got unexpected used colour count %u for format %s.\n",
                    dib.dsBmih.biClrUsed, testdata[i].name);
            ok(!dib.dsBmih.biClrImportant, "Got unexpected important colour count %u for format %s.\n",
                    dib.dsBmih.biClrImportant, testdata[i].name);

            if (dib.dsBmih.biCompression == BI_BITFIELDS)
            {
                ok(dib.dsBitfields[0] == testdata[i].mask_r && dib.dsBitfields[1] == testdata[i].mask_g
                        && dib.dsBitfields[2] == testdata[i].mask_b,
                        "Got unexpected colour masks 0x%08x 0x%08x 0x%08x for format %s.\n",
                        dib.dsBitfields[0], dib.dsBitfields[1], dib.dsBitfields[2], testdata[i].name);
            }
            else
            {
                ok(!dib.dsBitfields[0] && !dib.dsBitfields[1] && !dib.dsBitfields[2],
                        "Got unexpected colour masks 0x%08x 0x%08x 0x%08x for format %s.\n",
                        dib.dsBitfields[0], dib.dsBitfields[1], dib.dsBitfields[2], testdata[i].name);
            }
            ok(!dib.dshSection, "Got unexpected section %p for format %s.\n", dib.dshSection, testdata[i].name);
            ok(!dib.dsOffset, "Got unexpected offset %u for format %s.\n", dib.dsOffset, testdata[i].name);

            hr = IDirect3DSurface9_ReleaseDC(surface, dc);
            ok(hr == D3D_OK, "Failed to release DC, hr %#x.\n", hr);
        }
        else
        {
            ok(dc == (void *)0x1234, "Got unexpected dc %p.\n", dc);
        }

        IDirect3DSurface9_Release(surface);
        if (texture)
            IDirect3DTexture9_Release(texture);

        if (!testdata[i].getdc_supported)
            continue;

        if (FAILED(hr = IDirect3DDevice9_CreateCubeTexture(device, 64, 0, 0,
                testdata[i].format, D3DPOOL_MANAGED, &cube_texture, NULL)))
        {
            skip("Failed to create cube texture for format %s (hr %#x), skipping tests.\n", testdata[i].name, hr);
            continue;
        }

        hr = IDirect3DCubeTexture9_GetCubeMapSurface(cube_texture, D3DCUBEMAP_FACE_POSITIVE_X, 0, &surface);
        ok(SUCCEEDED(hr), "Failed to get cube surface for format %s, hr %#x.\n", testdata[i].name, hr);
        hr = IDirect3DCubeTexture9_GetCubeMapSurface(cube_texture, D3DCUBEMAP_FACE_NEGATIVE_Y, 2, &surface2);
        ok(SUCCEEDED(hr), "Failed to get cube surface for format %s, hr %#x.\n", testdata[i].name, hr);

        hr = IDirect3DSurface9_GetDC(surface, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", testdata[i].name, hr);
        hr = IDirect3DSurface9_ReleaseDC(surface, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", testdata[i].name, hr);
        hr = IDirect3DSurface9_GetDC(surface2, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", testdata[i].name, hr);
        hr = IDirect3DSurface9_ReleaseDC(surface2, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", testdata[i].name, hr);

        hr = IDirect3DSurface9_GetDC(surface, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", testdata[i].name, hr);
        dc2 = (void *)0x1234;
        hr = IDirect3DSurface9_GetDC(surface, &dc2);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for format %s.\n", hr, testdata[i].name);
        ok(dc2 == (void *)0x1234, "Got unexpected dc %p for format %s.\n", dc, testdata[i].name);
        hr = IDirect3DSurface9_ReleaseDC(surface, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", testdata[i].name, hr);
        hr = IDirect3DSurface9_ReleaseDC(surface, dc);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for format %s.\n", hr, testdata[i].name);

        hr = IDirect3DSurface9_LockRect(surface, &map_desc, NULL, D3DLOCK_READONLY);
        ok(SUCCEEDED(hr), "Failed to map surface for format %s, hr %#x.\n", testdata[i].name, hr);
        hr = IDirect3DSurface9_LockRect(surface, &map_desc, NULL, D3DLOCK_READONLY);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for format %s.\n", hr, testdata[i].name);
        hr = IDirect3DSurface9_UnlockRect(surface);
        ok(SUCCEEDED(hr), "Failed to unmap surface for format %s, hr %#x.\n", testdata[i].name, hr);
        hr = IDirect3DSurface9_UnlockRect(surface);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for format %s.\n", hr, testdata[i].name);

        hr = IDirect3DSurface9_GetDC(surface, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", testdata[i].name, hr);
        hr = IDirect3DSurface9_LockRect(surface, &map_desc, NULL, D3DLOCK_READONLY);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for format %s.\n", hr, testdata[i].name);
        hr = IDirect3DSurface9_ReleaseDC(surface, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", testdata[i].name, hr);

        hr = IDirect3DSurface9_LockRect(surface, &map_desc, NULL, D3DLOCK_READONLY);
        ok(SUCCEEDED(hr), "Failed to map surface for format %s, hr %#x.\n", testdata[i].name, hr);
        hr = IDirect3DSurface9_GetDC(surface, &dc);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for format %s.\n", hr, testdata[i].name);
        hr = IDirect3DSurface9_UnlockRect(surface);
        ok(SUCCEEDED(hr), "Failed to unmap surface for format %s, hr %#x.\n", testdata[i].name, hr);

        hr = IDirect3DSurface9_GetDC(surface, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", testdata[i].name, hr);
        hr = IDirect3DSurface9_GetDC(surface2, &dc2);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for format %s.\n", hr, testdata[i].name);
        hr = IDirect3DSurface9_ReleaseDC(surface2, dc2);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for format %s.\n", hr, testdata[i].name);
        hr = IDirect3DSurface9_ReleaseDC(surface, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", testdata[i].name, hr);

        hr = IDirect3DSurface9_GetDC(surface2, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", testdata[i].name, hr);
        hr = IDirect3DSurface9_GetDC(surface, &dc2);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for format %s.\n", hr, testdata[i].name);
        hr = IDirect3DSurface9_ReleaseDC(surface, dc2);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for format %s.\n", hr, testdata[i].name);
        hr = IDirect3DSurface9_ReleaseDC(surface2, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", testdata[i].name, hr);

        hr = IDirect3DSurface9_LockRect(surface, &map_desc, NULL, D3DLOCK_READONLY);
        ok(SUCCEEDED(hr), "Failed to map surface for format %s, hr %#x.\n", testdata[i].name, hr);
        hr = IDirect3DSurface9_LockRect(surface2, &map_desc, NULL, D3DLOCK_READONLY);
        ok(SUCCEEDED(hr), "Failed to map surface for format %s, hr %#x.\n", testdata[i].name, hr);
        hr = IDirect3DSurface9_UnlockRect(surface2);
        ok(SUCCEEDED(hr), "Failed to unmap surface for format %s, hr %#x.\n", testdata[i].name, hr);
        hr = IDirect3DSurface9_UnlockRect(surface);
        ok(SUCCEEDED(hr), "Failed to unmap surface for format %s, hr %#x.\n", testdata[i].name, hr);

        hr = IDirect3DSurface9_LockRect(surface, &map_desc, NULL, D3DLOCK_READONLY);
        ok(SUCCEEDED(hr), "Failed to map surface for format %s, hr %#x.\n", testdata[i].name, hr);
        hr = IDirect3DSurface9_GetDC(surface, &dc);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for format %s.\n", hr, testdata[i].name);
        hr = IDirect3DSurface9_ReleaseDC(surface, dc);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for format %s.\n", hr, testdata[i].name);
        hr = IDirect3DSurface9_UnlockRect(surface);
        ok(SUCCEEDED(hr), "Failed to unmap surface for format %s, hr %#x.\n", testdata[i].name, hr);

        hr = IDirect3DSurface9_LockRect(surface2, &map_desc, NULL, D3DLOCK_READONLY);
        ok(SUCCEEDED(hr), "Failed to map surface for format %s, hr %#x.\n", testdata[i].name, hr);
        hr = IDirect3DSurface9_GetDC(surface, &dc);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for format %s.\n", hr, testdata[i].name);
        hr = IDirect3DSurface9_ReleaseDC(surface, dc);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for format %s.\n", hr, testdata[i].name);
        hr = IDirect3DSurface9_UnlockRect(surface2);
        ok(SUCCEEDED(hr), "Failed to unmap surface for format %s, hr %#x.\n", testdata[i].name, hr);

        hr = IDirect3DSurface9_GetDC(surface, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", testdata[i].name, hr);
        hr = IDirect3DSurface9_LockRect(surface2, &map_desc, NULL, D3DLOCK_READONLY);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for format %s.\n", hr, testdata[i].name);
        hr = IDirect3DSurface9_UnlockRect(surface2);
        ok(SUCCEEDED(hr), "Failed to unmap surface for format %s, hr %#x.\n", testdata[i].name, hr);
        hr = IDirect3DSurface9_ReleaseDC(surface, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", testdata[i].name, hr);

        hr = IDirect3DSurface9_GetDC(surface2, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", testdata[i].name, hr);
        hr = IDirect3DSurface9_LockRect(surface, &map_desc, NULL, D3DLOCK_READONLY);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for format %s.\n", hr, testdata[i].name);
        hr = IDirect3DSurface9_UnlockRect(surface);
        ok(SUCCEEDED(hr), "Failed to unmap surface for format %s, hr %#x.\n", testdata[i].name, hr);
        hr = IDirect3DSurface9_ReleaseDC(surface2, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", testdata[i].name, hr);

        hr = IDirect3DSurface9_UnlockRect(surface);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for format %s.\n", hr, testdata[i].name);
        hr = IDirect3DSurface9_GetDC(surface2, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", testdata[i].name, hr);
        hr = IDirect3DSurface9_UnlockRect(surface);
        ok(SUCCEEDED(hr), "Failed to unmap surface for format %s, hr %#x.\n", testdata[i].name, hr);
        hr = IDirect3DSurface9_ReleaseDC(surface2, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", testdata[i].name, hr);
        hr = IDirect3DSurface9_UnlockRect(surface);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for format %s.\n", hr, testdata[i].name);

        hr = IDirect3DSurface9_UnlockRect(surface2);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for format %s.\n", hr, testdata[i].name);
        hr = IDirect3DSurface9_GetDC(surface, &dc);
        ok(SUCCEEDED(hr), "Failed to get DC for format %s, hr %#x.\n", testdata[i].name, hr);
        hr = IDirect3DSurface9_UnlockRect(surface2);
        ok(SUCCEEDED(hr), "Failed to unmap surface for format %s, hr %#x.\n", testdata[i].name, hr);
        hr = IDirect3DSurface9_ReleaseDC(surface, dc);
        ok(SUCCEEDED(hr), "Failed to release DC for format %s, hr %#x.\n", testdata[i].name, hr);
        hr = IDirect3DSurface9_UnlockRect(surface2);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for format %s.\n", hr, testdata[i].name);

        IDirect3DSurface9_Release(surface2);
        IDirect3DSurface9_Release(surface);
        IDirect3DCubeTexture9_Release(cube_texture);
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);

    /* Backbuffer created with D3DFMT_UNKNOWN format. */
    device_desc.adapter_ordinal = D3DADAPTER_DEFAULT;
    device_desc.width = 640;
    device_desc.height = 480;
    device_desc.device_window = window;
    device_desc.flags = CREATE_DEVICE_UNKNOWN_BACKBUFFER_FORMAT | CREATE_DEVICE_LOCKABLE_BACKBUFFER;

    device = create_device(d3d, window, &device_desc);
    ok(!!device, "Failed to create device.\n");

    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &surface);
    ok(SUCCEEDED(hr), "Failed to get back buffer, hr %#x.\n", hr);

    dc = NULL;
    hr = IDirect3DSurface9_GetDC(surface, &dc);
    ok(!!dc, "Unexpected DC returned.\n");
    ok(SUCCEEDED(hr), "Failed to get backbuffer DC, hr %#x.\n", hr);
    hr = IDirect3DSurface9_ReleaseDC(surface, dc);
    ok(SUCCEEDED(hr), "Failed to release backbuffer DC, hr %#x.\n", hr);

    IDirect3DSurface9_Release(surface);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);

    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_surface_dimensions(void)
{
    IDirect3DSurface9 *surface;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 0, 1,
            D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &surface, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 1, 0,
            D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &surface, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_surface_format_null(void)
{
    static const D3DFORMAT D3DFMT_NULL = MAKEFOURCC('N','U','L','L');
    IDirect3DTexture9 *texture;
    IDirect3DSurface9 *surface;
    IDirect3DSurface9 *rt, *ds;
    D3DLOCKED_RECT locked_rect;
    IDirect3DDevice9 *device;
    D3DSURFACE_DESC desc;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    hr = IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            D3DUSAGE_RENDERTARGET, D3DRTYPE_SURFACE, D3DFMT_NULL);
    if (hr != D3D_OK)
    {
        skip("No D3DFMT_NULL support, skipping test.\n");
        IDirect3D9_Release(d3d);
        return;
    }

    window = create_window();
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, D3DFMT_NULL);
    ok(hr == D3D_OK, "D3DFMT_NULL should be supported for render target textures, hr %#x.\n", hr);

    hr = IDirect3D9_CheckDepthStencilMatch(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            D3DFMT_NULL, D3DFMT_D24S8);
    ok(SUCCEEDED(hr), "Depth stencil match failed for D3DFMT_NULL, hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateRenderTarget(device, 128, 128, D3DFMT_NULL,
            D3DMULTISAMPLE_NONE, 0, TRUE, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &rt);
    ok(SUCCEEDED(hr), "Failed to get original render target, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetDepthStencilSurface(device, &ds);
    ok(SUCCEEDED(hr), "Failed to get original depth/stencil, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, NULL);
    ok(FAILED(hr), "Succeeded in setting render target 0 to NULL, should fail.\n");

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, surface);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetDepthStencilSurface(device, NULL);
    ok(SUCCEEDED(hr), "Failed to set depth/stencil, hr %#x.\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x00000000, 0.0f, 0);
    ok(SUCCEEDED(hr), "Clear failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, rt);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetDepthStencilSurface(device, ds);
    ok(SUCCEEDED(hr), "Failed to set depth/stencil, hr %#x.\n", hr);

    IDirect3DSurface9_Release(rt);
    IDirect3DSurface9_Release(ds);

    hr = IDirect3DSurface9_GetDesc(surface, &desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#x.\n", hr);
    ok(desc.Width == 128, "Expected width 128, got %u.\n", desc.Width);
    ok(desc.Height == 128, "Expected height 128, got %u.\n", desc.Height);

    hr = IDirect3DSurface9_LockRect(surface, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    ok(locked_rect.Pitch, "Expected non-zero pitch, got %u.\n", locked_rect.Pitch);
    ok(!!locked_rect.pBits, "Expected non-NULL pBits, got %p.\n", locked_rect.pBits);

    hr = IDirect3DSurface9_UnlockRect(surface);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    IDirect3DSurface9_Release(surface);

    hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 0, D3DUSAGE_RENDERTARGET,
            D3DFMT_NULL, D3DPOOL_DEFAULT, &texture, NULL);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    IDirect3DTexture9_Release(texture);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_surface_double_unlock(void)
{
    static const D3DPOOL pools[] =
    {
        D3DPOOL_DEFAULT,
        D3DPOOL_SCRATCH,
        D3DPOOL_SYSTEMMEM,
    };
    IDirect3DSurface9 *surface;
    IDirect3DDevice9 *device;
    D3DSURFACE_DESC desc;
    D3DLOCKED_RECT lr;
    IDirect3D9 *d3d;
    unsigned int i;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    for (i = 0; i < ARRAY_SIZE(pools); ++i)
    {
        hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 64, 64,
                D3DFMT_X8R8G8B8, pools[i], &surface, NULL);
        ok(SUCCEEDED(hr), "Failed to create surface in pool %#x, hr %#x.\n", pools[i], hr);

        hr = IDirect3DSurface9_GetDesc(surface, &desc);
        ok(hr == D3D_OK, "Pool %#x: Got unexpected hr %#x.\n", pools[i], hr);
        ok(!desc.Usage, "Pool %#x: Got unexpected usage %#x.\n", pools[i], desc.Usage);
        ok(desc.Pool == pools[i], "Pool %#x: Got unexpected pool %#x.\n", pools[i], desc.Pool);

        hr = IDirect3DSurface9_UnlockRect(surface);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x, for surface in pool %#x.\n", hr, pools[i]);
        hr = IDirect3DSurface9_LockRect(surface, &lr, NULL, 0);
        ok(SUCCEEDED(hr), "Failed to lock surface in pool %#x, hr %#x.\n", pools[i], hr);
        hr = IDirect3DSurface9_UnlockRect(surface);
        ok(SUCCEEDED(hr), "Failed to unlock surface in pool %#x, hr %#x.\n", pools[i], hr);
        hr = IDirect3DSurface9_UnlockRect(surface);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x, for surface in pool %#x.\n", hr, pools[i]);

        IDirect3DSurface9_Release(surface);
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_surface_blocks(void)
{
    static const struct
    {
        D3DFORMAT fmt;
        const char *name;
        unsigned int block_width;
        unsigned int block_height;
        BOOL broken;
        BOOL create_size_checked, core_fmt;
    }
    formats[] =
    {
        {D3DFMT_DXT1,                 "D3DFMT_DXT1", 4, 4, FALSE, TRUE,  TRUE },
        {D3DFMT_DXT2,                 "D3DFMT_DXT2", 4, 4, FALSE, TRUE,  TRUE },
        {D3DFMT_DXT3,                 "D3DFMT_DXT3", 4, 4, FALSE, TRUE,  TRUE },
        {D3DFMT_DXT4,                 "D3DFMT_DXT4", 4, 4, FALSE, TRUE,  TRUE },
        {D3DFMT_DXT5,                 "D3DFMT_DXT5", 4, 4, FALSE, TRUE,  TRUE },
        /* ATI1N and ATI2N have 2x2 blocks on all AMD cards and Geforce 7 cards,
         * which doesn't match the format spec. On newer Nvidia cards
         * they have the correct 4x4 block size */
        {MAKEFOURCC('A','T','I','1'), "ATI1N",       4, 4, TRUE,  FALSE, FALSE},
        {MAKEFOURCC('A','T','I','2'), "ATI2N",       4, 4, TRUE,  FALSE, FALSE},
        {D3DFMT_YUY2,                 "D3DFMT_YUY2", 2, 1, FALSE, FALSE, TRUE },
        {D3DFMT_UYVY,                 "D3DFMT_UYVY", 2, 1, FALSE, FALSE, TRUE },
    };
    static const struct
    {
        D3DPOOL pool;
        const char *name;
        /* Don't check the return value, Nvidia returns D3DERR_INVALIDCALL for some formats
         * and E_INVALIDARG/DDERR_INVALIDPARAMS for others. */
        BOOL success;
    }
    pools[] =
    {
        {D3DPOOL_DEFAULT,       "D3DPOOL_DEFAULT",  FALSE},
        {D3DPOOL_SCRATCH,       "D3DPOOL_SCRATCH",  TRUE},
        {D3DPOOL_SYSTEMMEM,     "D3DPOOL_SYSTEMMEM",TRUE},
        {D3DPOOL_MANAGED,       "D3DPOOL_MANAGED",  TRUE},
    };
    static struct
    {
        D3DRESOURCETYPE rtype;
        const char *type_name;
        D3DPOOL pool;
        const char *pool_name;
        BOOL need_driver_support, need_runtime_support;
    }
    create_tests[] =
    {
        {D3DRTYPE_SURFACE,     "D3DRTYPE_SURFACE",     D3DPOOL_DEFAULT,   "D3DPOOL_DEFAULT",   TRUE,  FALSE},
        {D3DRTYPE_SURFACE,     "D3DRTYPE_SURFACE",     D3DPOOL_SYSTEMMEM, "D3DPOOL_SYSTEMMEM", TRUE,  TRUE },
        /* Managed offscreen plain surfaces are not supported */
        {D3DRTYPE_SURFACE,     "D3DRTYPE_SURFACE",     D3DPOOL_SCRATCH,   "D3DPOOL_SCRATCH",   FALSE, TRUE },

        {D3DRTYPE_TEXTURE,     "D3DRTYPE_TEXTURE",     D3DPOOL_DEFAULT,   "D3DPOOL_DEFAULT",   TRUE,  FALSE},
        {D3DRTYPE_TEXTURE,     "D3DRTYPE_TEXTURE",     D3DPOOL_SYSTEMMEM, "D3DPOOL_SYSTEMMEM", TRUE,  FALSE},
        {D3DRTYPE_TEXTURE,     "D3DRTYPE_TEXTURE",     D3DPOOL_MANAGED,   "D3DPOOL_MANAGED",   TRUE,  FALSE},
        {D3DRTYPE_TEXTURE,     "D3DRTYPE_TEXTURE",     D3DPOOL_SCRATCH,   "D3DPOOL_SCRATCH",   FALSE, TRUE },

        {D3DRTYPE_CUBETEXTURE, "D3DRTYPE_CUBETEXTURE", D3DPOOL_DEFAULT,   "D3DPOOL_DEFAULT",   TRUE,  FALSE},
        {D3DRTYPE_CUBETEXTURE, "D3DRTYPE_CUBETEXTURE", D3DPOOL_SYSTEMMEM, "D3DPOOL_SYSTEMMEM", TRUE,  FALSE},
        {D3DRTYPE_CUBETEXTURE, "D3DRTYPE_CUBETEXTURE", D3DPOOL_MANAGED,   "D3DPOOL_MANAGED",   TRUE,  FALSE},
        {D3DRTYPE_CUBETEXTURE, "D3DRTYPE_CUBETEXTURE", D3DPOOL_SCRATCH,   "D3DPOOL_SCRATCH",   FALSE, TRUE },
    };
    IDirect3DTexture9 *texture;
    IDirect3DCubeTexture9 *cube_texture;
    IDirect3DSurface9 *surface;
    D3DLOCKED_RECT locked_rect;
    IDirect3DDevice9 *device;
    unsigned int i, j, k, w, h;
    BOOL surface_only;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    RECT rect;
    BOOL tex_pow2, cube_pow2;
    D3DCAPS9 caps;
    static const RECT invalid[] =
    {
        {60, 60, 60, 68},       /* 0 height */
        {60, 60, 68, 60},       /* 0 width */
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

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);
    tex_pow2 = !!(caps.TextureCaps & D3DPTEXTURECAPS_POW2);
    if (tex_pow2)
        tex_pow2 = !(caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL);
    cube_pow2 = !!(caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP_POW2);

    for (i = 0; i < ARRAY_SIZE(formats); ++i)
    {
        BOOL tex_support, cube_support, surface_support, format_known, dynamic_tex_support;

        hr = IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                0, D3DRTYPE_TEXTURE, formats[i].fmt);
        tex_support = SUCCEEDED(hr);
        hr = IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                0, D3DRTYPE_CUBETEXTURE, formats[i].fmt);
        cube_support = SUCCEEDED(hr);
        hr = IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                0, D3DRTYPE_SURFACE, formats[i].fmt);
        surface_support = SUCCEEDED(hr);

        /* Scratch pool in general allows texture creation even if the driver does
         * not support the format. If the format is an extension format that is not
         * known to the runtime, like ATI2N, some driver support is required for
         * this to work.
         *
         * It is also possible that Windows Vista and Windows 7 d3d9 runtimes know
         * about ATI2N. I cannot check this because all my Vista+ machines support
         * ATI2N in hardware, but none of my WinXP machines do. */
        format_known = tex_support || cube_support || surface_support;

        for (w = 1; w <= 8; w++)
        {
            for (h = 1; h <= 8; h++)
            {
                BOOL block_aligned = TRUE;
                BOOL size_is_pow2;

                if (w & (formats[i].block_width - 1) || h & (formats[i].block_height - 1))
                    block_aligned = FALSE;

                size_is_pow2 = !(w & (w - 1) || h & (h - 1));

                for (j = 0; j < ARRAY_SIZE(create_tests); j++)
                {
                    BOOL support, pow2;
                    HRESULT expect_hr;
                    BOOL may_succeed = FALSE;
                    IUnknown **check_null;

                    if (!formats[i].core_fmt)
                    {
                        /* AMD warns against creating ATI2N textures smaller than
                         * the block size because the runtime cannot calculate the
                         * correct texture size. Generalize this for all extension
                         * formats. */
                        if (w < formats[i].block_width || h < formats[i].block_height)
                            continue;
                    }

                    texture = (IDirect3DTexture9 *)0xdeadbeef;
                    cube_texture = (IDirect3DCubeTexture9 *)0xdeadbeef;
                    surface = (IDirect3DSurface9 *)0xdeadbeef;

                    switch (create_tests[j].rtype)
                    {
                        case D3DRTYPE_TEXTURE:
                            check_null = (IUnknown **)&texture;
                            hr = IDirect3DDevice9_CreateTexture(device, w, h, 1, 0,
                                    formats[i].fmt, create_tests[j].pool, &texture, NULL);
                            support = tex_support;
                            pow2 = tex_pow2;
                            break;

                        case D3DRTYPE_CUBETEXTURE:
                            if (w != h)
                                continue;
                            check_null = (IUnknown **)&cube_texture;
                            hr = IDirect3DDevice9_CreateCubeTexture(device, w, 1, 0,
                                    formats[i].fmt, create_tests[j].pool, &cube_texture, NULL);
                            support = cube_support;
                            pow2 = cube_pow2;
                            break;

                        case D3DRTYPE_SURFACE:
                            check_null = (IUnknown **)&surface;
                            hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, w, h,
                                    formats[i].fmt, create_tests[j].pool, &surface, NULL);
                            support = surface_support;
                            pow2 = FALSE;
                            break;

                        default:
                            check_null = NULL;
                            pow2 = FALSE;
                            support = FALSE;
                            break;
                    }

                    if (create_tests[j].need_driver_support && !support)
                        expect_hr = D3DERR_INVALIDCALL;
                    else if (create_tests[j].need_runtime_support && !formats[i].core_fmt && !format_known)
                        expect_hr = D3DERR_INVALIDCALL;
                    else if (formats[i].create_size_checked && !block_aligned)
                        expect_hr = D3DERR_INVALIDCALL;
                    else if (pow2 && !size_is_pow2 && create_tests[j].need_driver_support)
                        expect_hr = D3DERR_INVALIDCALL;
                    else
                        expect_hr = D3D_OK;

                    /* Wine knows about ATI2N and happily creates a scratch resource even if GL
                     * does not support it. Accept scratch creation of extension formats on
                     * Windows as well if it occurs. We don't really care if e.g. a Windows 7
                     * on an r200 GPU creates scratch ATI2N texture even though the card doesn't
                     * support it. */
                    if (!formats[i].core_fmt && !format_known && FAILED(expect_hr))
                        may_succeed = TRUE;

                    ok(hr == expect_hr || ((SUCCEEDED(hr) && may_succeed)),
                            "Got unexpected hr %#x for format %s, pool %s, type %s, size %ux%u.\n",
                            hr, formats[i].name, create_tests[j].pool_name, create_tests[j].type_name, w, h);
                    if (FAILED(hr))
                        ok(*check_null == NULL, "Got object ptr %p, expected NULL.\n", *check_null);
                    else
                        IUnknown_Release(*check_null);
                }
            }
        }

        surface_only = FALSE;
        hr = IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                D3DUSAGE_DYNAMIC, D3DRTYPE_TEXTURE, formats[i].fmt);
        dynamic_tex_support = SUCCEEDED(hr);
        if (!dynamic_tex_support)
        {
            if (!surface_support)
            {
                skip("Format %s not supported, skipping lockrect offset tests.\n", formats[i].name);
                continue;
            }
            surface_only = TRUE;
        }

        for (j = 0; j < ARRAY_SIZE(pools); ++j)
        {
            switch (pools[j].pool)
            {
                case D3DPOOL_SYSTEMMEM:
                case D3DPOOL_MANAGED:
                    if (surface_only)
                        continue;
                    /* Fall through */
                case D3DPOOL_DEFAULT:
                    if (surface_only)
                    {
                        hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 128, 128,
                                formats[i].fmt, pools[j].pool, &surface, NULL);
                        ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
                    }
                    else
                    {
                        hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 1,
                                pools[j].pool == D3DPOOL_DEFAULT ? D3DUSAGE_DYNAMIC : 0,
                                formats[i].fmt, pools[j].pool, &texture, NULL);
                        ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
                        hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
                        ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x.\n", hr);
                        IDirect3DTexture9_Release(texture);
                    }
                    break;

                case D3DPOOL_SCRATCH:
                    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 128, 128,
                            formats[i].fmt, pools[j].pool, &surface, NULL);
                    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
                    break;

                default:
                    break;
            }

            if (formats[i].block_width > 1)
            {
                SetRect(&rect, formats[i].block_width >> 1, 0, formats[i].block_width, formats[i].block_height);
                hr = IDirect3DSurface9_LockRect(surface, &locked_rect, &rect, 0);
                ok(FAILED(hr) == !pools[j].success || broken(formats[i].broken),
                        "Partial block lock %s, expected %s, format %s, pool %s.\n",
                        SUCCEEDED(hr) ? "succeeded" : "failed",
                        pools[j].success ? "success" : "failure", formats[i].name, pools[j].name);
                if (SUCCEEDED(hr))
                {
                    hr = IDirect3DSurface9_UnlockRect(surface);
                    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
                }

                SetRect(&rect, 0, 0, formats[i].block_width >> 1, formats[i].block_height);
                hr = IDirect3DSurface9_LockRect(surface, &locked_rect, &rect, 0);
                ok(FAILED(hr) == !pools[j].success || broken(formats[i].broken),
                        "Partial block lock %s, expected %s, format %s, pool %s.\n",
                        SUCCEEDED(hr) ? "succeeded" : "failed",
                        pools[j].success ? "success" : "failure", formats[i].name, pools[j].name);
                if (SUCCEEDED(hr))
                {
                    hr = IDirect3DSurface9_UnlockRect(surface);
                    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
                }
            }

            if (formats[i].block_height > 1)
            {
                SetRect(&rect, 0, formats[i].block_height >> 1, formats[i].block_width, formats[i].block_height);
                hr = IDirect3DSurface9_LockRect(surface, &locked_rect, &rect, 0);
                ok(FAILED(hr) == !pools[j].success || broken(formats[i].broken),
                        "Partial block lock %s, expected %s, format %s, pool %s.\n",
                        SUCCEEDED(hr) ? "succeeded" : "failed",
                        pools[j].success ? "success" : "failure", formats[i].name, pools[j].name);
                if (SUCCEEDED(hr))
                {
                    hr = IDirect3DSurface9_UnlockRect(surface);
                    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
                }

                SetRect(&rect, 0, 0, formats[i].block_width, formats[i].block_height >> 1);
                hr = IDirect3DSurface9_LockRect(surface, &locked_rect, &rect, 0);
                ok(FAILED(hr) == !pools[j].success || broken(formats[i].broken),
                        "Partial block lock %s, expected %s, format %s, pool %s.\n",
                        SUCCEEDED(hr) ? "succeeded" : "failed",
                        pools[j].success ? "success" : "failure", formats[i].name, pools[j].name);
                if (SUCCEEDED(hr))
                {
                    hr = IDirect3DSurface9_UnlockRect(surface);
                    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
                }
            }

            for (k = 0; k < ARRAY_SIZE(invalid); ++k)
            {
                hr = IDirect3DSurface9_LockRect(surface, &locked_rect, &invalid[k], 0);
                ok(FAILED(hr) == !pools[j].success, "Invalid lock %s(%#x), expected %s, format %s, pool %s, case %u.\n",
                        SUCCEEDED(hr) ? "succeeded" : "failed", hr, pools[j].success ? "success" : "failure",
                        formats[i].name, pools[j].name, k);
                if (SUCCEEDED(hr))
                {
                    hr = IDirect3DSurface9_UnlockRect(surface);
                    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);
                }
            }

            SetRect(&rect, 0, 0, formats[i].block_width, formats[i].block_height);
            hr = IDirect3DSurface9_LockRect(surface, &locked_rect, &rect, 0);
            ok(SUCCEEDED(hr), "Got unexpected hr %#x for format %s, pool %s.\n", hr, formats[i].name, pools[j].name);
            hr = IDirect3DSurface9_UnlockRect(surface);
            ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

            IDirect3DSurface9_Release(surface);
        }

        if (!dynamic_tex_support)
        {
            skip("Dynamic %s textures not supported, skipping mipmap test.\n", formats[i].name);
            continue;
        }

        if (formats[i].block_width == 1 && formats[i].block_height == 1)
            continue;
        if (!formats[i].core_fmt)
            continue;

        hr = IDirect3DDevice9_CreateTexture(device, formats[i].block_width, formats[i].block_height, 2,
                D3DUSAGE_DYNAMIC, formats[i].fmt, D3DPOOL_DEFAULT, &texture, NULL);
        ok(SUCCEEDED(hr), "Failed to create texture, hr %#x, format %s.\n", hr, formats[i].name);

        hr = IDirect3DTexture9_LockRect(texture, 1, &locked_rect, NULL, 0);
        ok(SUCCEEDED(hr), "Failed lock texture, hr %#x.\n", hr);
        hr = IDirect3DTexture9_UnlockRect(texture, 1);
        ok(SUCCEEDED(hr), "Failed lock texture, hr %#x.\n", hr);

        SetRect(&rect, 0, 0, formats[i].block_width == 1 ? 1 : formats[i].block_width >> 1,
                formats[i].block_height == 1 ? 1 : formats[i].block_height >> 1);
        hr = IDirect3DTexture9_LockRect(texture, 1, &locked_rect, &rect, 0);
        ok(SUCCEEDED(hr), "Failed lock texture, hr %#x.\n", hr);
        hr = IDirect3DTexture9_UnlockRect(texture, 1);
        ok(SUCCEEDED(hr), "Failed lock texture, hr %#x.\n", hr);

        SetRect(&rect, 0, 0, formats[i].block_width, formats[i].block_height);
        hr = IDirect3DTexture9_LockRect(texture, 1, &locked_rect, &rect, 0);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
        if (SUCCEEDED(hr))
            IDirect3DTexture9_UnlockRect(texture, 1);

        IDirect3DTexture9_Release(texture);
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_set_palette(void)
{
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d9;
    UINT refcount;
    HWND window;
    HRESULT hr;
    PALETTEENTRY pal[256];
    unsigned int i;
    D3DCAPS9 caps;

    window = create_window();
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d9, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        DestroyWindow(window);
        return;
    }

    for (i = 0; i < ARRAY_SIZE(pal); i++)
    {
        pal[i].peRed = i;
        pal[i].peGreen = i;
        pal[i].peBlue = i;
        pal[i].peFlags = 0xff;
    }
    hr = IDirect3DDevice9_SetPaletteEntries(device, 0, pal);
    ok(SUCCEEDED(hr), "Failed to set palette entries, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    for (i = 0; i < ARRAY_SIZE(pal); i++)
    {
        pal[i].peRed = i;
        pal[i].peGreen = i;
        pal[i].peBlue = i;
        pal[i].peFlags = i;
    }
    if (caps.TextureCaps & D3DPTEXTURECAPS_ALPHAPALETTE)
    {
        hr = IDirect3DDevice9_SetPaletteEntries(device, 0, pal);
        ok(SUCCEEDED(hr), "Failed to set palette entries, hr %#x.\n", hr);
    }
    else
    {
        hr = IDirect3DDevice9_SetPaletteEntries(device, 0, pal);
        ok(hr == D3DERR_INVALIDCALL, "SetPaletteEntries returned %#x, expected D3DERR_INVALIDCALL.\n", hr);
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_pinned_buffers(void)
{
    static const struct
    {
        DWORD device_flags;
        DWORD usage;
        D3DPOOL pool;
    }
    tests[] =
    {
        {CREATE_DEVICE_SWVP_ONLY, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DPOOL_DEFAULT},
        /* This doesn't necessarily return the same address. */
        /* {0, 0, D3DPOOL_DEFAULT}, */
        {0, 0, D3DPOOL_MANAGED},
        {0, 0, D3DPOOL_SYSTEMMEM},
    };
    static const unsigned int vertex_count = 1024;
    struct device_desc device_desc;
    IDirect3DVertexBuffer9 *buffer;
    D3DVERTEXBUFFER_DESC desc;
    IDirect3DDevice9 *device;
    struct vec3 *ptr, *ptr2;
    unsigned int i, test;
    IDirect3D9 *d3d;
    UINT refcount;
    HWND window;
    HRESULT hr;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    for (test = 0; test < ARRAY_SIZE(tests); ++test)
    {
        device_desc.adapter_ordinal = D3DADAPTER_DEFAULT;
        device_desc.device_window = window;
        device_desc.width = 640;
        device_desc.height = 480;
        device_desc.flags = tests[test].device_flags;
        if (!(device = create_device(d3d, window, &device_desc)))
        {
            skip("Test %u: failed to create a D3D device.\n", test);
            continue;
        }

        hr = IDirect3DDevice9_CreateVertexBuffer(device, vertex_count * sizeof(*ptr),
                tests[test].usage, 0, tests[test].pool, &buffer, NULL);
        ok(hr == D3D_OK, "Test %u: got unexpected hr %#x.\n", test, hr);
        hr = IDirect3DVertexBuffer9_GetDesc(buffer, &desc);
        ok(hr == D3D_OK, "Test %u: got unexpected hr %#x.\n", test, hr);
        ok(desc.Pool == tests[test].pool, "Test %u: got unexpected pool %#x.\n", test, desc.Pool);
        ok(desc.Usage == tests[test].usage, "Test %u: got unexpected usage %#x.\n", test, desc.Usage);

        hr = IDirect3DVertexBuffer9_Lock(buffer, 0, vertex_count * sizeof(*ptr), (void **)&ptr, D3DLOCK_DISCARD);
        ok(hr == D3D_OK, "Test %u: got unexpected hr %#x.\n", test, hr);
        for (i = 0; i < vertex_count; ++i)
        {
            ptr[i].x = i * 1.0f;
            ptr[i].y = i * 2.0f;
            ptr[i].z = i * 3.0f;
        }
        hr = IDirect3DVertexBuffer9_Unlock(buffer);
        ok(hr == D3D_OK, "Test %u: got unexpected hr %#x.\n", test, hr);

        hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
        ok(hr == D3D_OK, "Test %u: got unexpected hr %#x.\n", test, hr);
        hr = IDirect3DDevice9_SetStreamSource(device, 0, buffer, 0, sizeof(*ptr));
        ok(hr == D3D_OK, "Test %u: got unexpected hr %#x.\n", test, hr);
        hr = IDirect3DDevice9_BeginScene(device);
        ok(hr == D3D_OK, "Test %u: got unexpected hr %#x.\n", test, hr);
        hr = IDirect3DDevice9_DrawPrimitive(device, D3DPT_TRIANGLELIST, 0, 2);
        ok(hr == D3D_OK, "Test %u: got unexpected hr %#x.\n", test, hr);
        hr = IDirect3DDevice9_EndScene(device);
        ok(hr == D3D_OK, "Test %u: got unexpected hr %#x.\n", test, hr);

        hr = IDirect3DVertexBuffer9_Lock(buffer, 0, vertex_count * sizeof(*ptr2), (void **)&ptr2, D3DLOCK_DISCARD);
        ok(hr == D3D_OK, "Test %u: got unexpected hr %#x.\n", test, hr);
        ok(ptr2 == ptr, "Test %u: got unexpected ptr2 %p, expected %p.\n", test, ptr2, ptr);
        for (i = 0; i < vertex_count; ++i)
        {
            if (ptr2[i].x != i * 1.0f || ptr2[i].y != i * 2.0f || ptr2[i].z != i * 3.0f)
            {
                ok(FALSE, "Test %u: got unexpected vertex %u {%.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e}.\n",
                        test, i, ptr2[i].x, ptr2[i].y, ptr2[i].z, i * 1.0f, i * 2.0f, i * 3.0f);
                break;
            }
        }
        hr = IDirect3DVertexBuffer9_Unlock(buffer);
        ok(hr == D3D_OK, "Test %u: got unexpected hr %#x.\n", test, hr);

        IDirect3DVertexBuffer9_Release(buffer);
        refcount = IDirect3DDevice9_Release(device);
        ok(!refcount, "Test %u: device has %u references left.\n", test, refcount);
    }
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_npot_textures(void)
{
    IDirect3DDevice9 *device = NULL;
    IDirect3D9 *d3d9;
    ULONG refcount;
    HWND window = NULL;
    HRESULT hr;
    D3DCAPS9 caps;
    IDirect3DTexture9 *texture;
    IDirect3DCubeTexture9 *cube_texture;
    IDirect3DVolumeTexture9 *volume_texture;
    struct
    {
        D3DPOOL pool;
        const char *pool_name;
        HRESULT hr;
    }
    pools[] =
    {
        { D3DPOOL_DEFAULT,    "D3DPOOL_DEFAULT",    D3DERR_INVALIDCALL },
        { D3DPOOL_MANAGED,    "D3DPOOL_MANAGED",    D3DERR_INVALIDCALL },
        { D3DPOOL_SYSTEMMEM,  "D3DPOOL_SYSTEMMEM",  D3DERR_INVALIDCALL },
        { D3DPOOL_SCRATCH,    "D3DPOOL_SCRATCH",    D3D_OK             },
    };
    unsigned int i, levels;
    BOOL tex_pow2, cube_pow2, vol_pow2;

    window = create_window();
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d9, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);
    tex_pow2 = !!(caps.TextureCaps & D3DPTEXTURECAPS_POW2);
    cube_pow2 = !!(caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP_POW2);
    vol_pow2 = !!(caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP_POW2);
    ok(cube_pow2 == tex_pow2, "Cube texture and 2d texture pow2 restrictions mismatch.\n");
    ok(vol_pow2 == tex_pow2, "Volume texture and 2d texture pow2 restrictions mismatch.\n");

    for (i = 0; i < ARRAY_SIZE(pools); i++)
    {
        for (levels = 0; levels <= 2; levels++)
        {
            HRESULT expected;

            hr = IDirect3DDevice9_CreateTexture(device, 10, 10, levels, 0, D3DFMT_X8R8G8B8,
                    pools[i].pool, &texture, NULL);
            if (!tex_pow2)
            {
                expected = D3D_OK;
            }
            else if (caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL)
            {
                if (levels == 1)
                    expected = D3D_OK;
                else
                    expected = pools[i].hr;
            }
            else
            {
                expected = pools[i].hr;
            }
            ok(hr == expected, "CreateTexture(w=h=10, %s, levels=%u) returned hr %#x, expected %#x.\n",
                    pools[i].pool_name, levels, hr, expected);

            if (SUCCEEDED(hr))
                IDirect3DTexture9_Release(texture);
        }

        hr = IDirect3DDevice9_CreateCubeTexture(device, 3, 1, 0, D3DFMT_X8R8G8B8, pools[i].pool,
                &cube_texture, NULL);
        if (tex_pow2)
        {
            ok(hr == pools[i].hr, "CreateCubeTexture(EdgeLength=3, %s) returned hr %#x, expected %#x.\n",
                    pools[i].pool_name, hr, pools[i].hr);
        }
        else
        {
            ok(SUCCEEDED(hr), "CreateCubeTexture(EdgeLength=3, %s) returned hr %#x, expected %#x.\n",
                    pools[i].pool_name, hr, D3D_OK);
        }

        if (SUCCEEDED(hr))
            IDirect3DCubeTexture9_Release(cube_texture);

        hr = IDirect3DDevice9_CreateVolumeTexture(device, 2, 2, 3, 1, 0, D3DFMT_X8R8G8B8, pools[i].pool,
                &volume_texture, NULL);
        if (tex_pow2)
        {
            ok(hr == pools[i].hr, "CreateVolumeTextur(Depth=3, %s) returned hr %#x, expected %#x.\n",
                    pools[i].pool_name, hr, pools[i].hr);
        }
        else
        {
            ok(SUCCEEDED(hr), "CreateVolumeTextur(Depth=3, %s) returned hr %#x, expected %#x.\n",
                    pools[i].pool_name, hr, D3D_OK);
        }

        if (SUCCEEDED(hr))
            IDirect3DVolumeTexture9_Release(volume_texture);
    }

done:
    if (device)
    {
        refcount = IDirect3DDevice9_Release(device);
        ok(!refcount, "Device has %u references left.\n", refcount);
    }
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);

}

static void test_vidmem_accounting(void)
{
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d9;
    ULONG refcount;
    HWND window;
    HRESULT hr = D3D_OK;
    IDirect3DTexture9 *textures[20];
    unsigned int i;
    UINT vidmem_start, vidmem_end, diff;

    window = create_window();
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d9, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }

    vidmem_start = IDirect3DDevice9_GetAvailableTextureMem(device);
    memset(textures, 0, sizeof(textures));
    for (i = 0; i < ARRAY_SIZE(textures); i++)
    {
        hr = IDirect3DDevice9_CreateTexture(device, 1024, 1024, 1, D3DUSAGE_RENDERTARGET,
                D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &textures[i], NULL);
        /* D3DERR_OUTOFVIDEOMEMORY is returned when the card runs out of video memory
         * E_FAIL is returned on address space or system memory exhaustion */
        ok(SUCCEEDED(hr) || hr == D3DERR_OUTOFVIDEOMEMORY || hr == E_OUTOFMEMORY,
                "Failed to create texture, hr %#x.\n", hr);
    }
    vidmem_end = IDirect3DDevice9_GetAvailableTextureMem(device);

    ok(vidmem_start > vidmem_end, "Expected available texture memory to decrease during texture creation.\n");
    diff = vidmem_start - vidmem_end;
    ok(diff > 1024 * 1024 * 2 * i, "Expected a video memory difference of at least %u MB, got %u MB.\n",
            2 * i, diff / 1024 / 1024);

    for (i = 0; i < ARRAY_SIZE(textures); i++)
    {
        if (textures[i])
            IDirect3DTexture9_Release(textures[i]);
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_volume_locking(void)
{
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d9;
    HWND window;
    HRESULT hr;
    IDirect3DVolumeTexture9 *texture;
    unsigned int i;
    D3DLOCKED_BOX locked_box;
    ULONG refcount;
    D3DCAPS9 caps;
    static const struct
    {
        D3DPOOL pool;
        DWORD usage;
        HRESULT create_hr, lock_hr;
    }
    tests[] =
    {
        { D3DPOOL_DEFAULT,      0,                  D3D_OK,             D3DERR_INVALIDCALL  },
        { D3DPOOL_DEFAULT,      D3DUSAGE_DYNAMIC,   D3D_OK,             D3D_OK              },
        { D3DPOOL_SYSTEMMEM,    0,                  D3D_OK,             D3D_OK              },
        { D3DPOOL_SYSTEMMEM,    D3DUSAGE_DYNAMIC,   D3D_OK,             D3D_OK              },
        { D3DPOOL_MANAGED,      0,                  D3D_OK,             D3D_OK              },
        { D3DPOOL_MANAGED,      D3DUSAGE_DYNAMIC,   D3DERR_INVALIDCALL, D3D_OK              },
        { D3DPOOL_SCRATCH,      0,                  D3D_OK,             D3D_OK              },
        { D3DPOOL_SCRATCH,      D3DUSAGE_DYNAMIC,   D3DERR_INVALIDCALL, D3D_OK              },
    };

    window = create_window();
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d9, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);
    if (!(caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP))
    {
        skip("Volume textures not supported, skipping test.\n");
        goto out;
    }

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        hr = IDirect3DDevice9_CreateVolumeTexture(device, 4, 4, 4, 1, tests[i].usage,
                D3DFMT_A8R8G8B8, tests[i].pool, &texture, NULL);
        ok(hr == tests[i].create_hr, "Creating volume texture pool=%u, usage=%#x returned %#x, expected %#x.\n",
                tests[i].pool, tests[i].usage, hr, tests[i].create_hr);
        if (FAILED(hr))
            continue;

        locked_box.pBits = (void *)0xdeadbeef;
        hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &locked_box, NULL, 0);
        ok(hr == tests[i].lock_hr, "Lock returned %#x, expected %#x.\n", hr, tests[i].lock_hr);
        if (SUCCEEDED(hr))
        {
            hr = IDirect3DVolumeTexture9_UnlockBox(texture, 0);
            ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);
        }
        else
        {
            ok(locked_box.pBits == NULL, "Failed lock set pBits = %p, expected NULL.\n", locked_box.pBits);
        }
        IDirect3DVolumeTexture9_Release(texture);
    }

out:
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_update_volumetexture(void)
{
    D3DADAPTER_IDENTIFIER9 identifier;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d9;
    HWND window;
    HRESULT hr;
    IDirect3DVolumeTexture9 *src, *dst;
    unsigned int i;
    D3DLOCKED_BOX locked_box;
    ULONG refcount;
    D3DCAPS9 caps;
    BOOL is_warp;
    static const struct
    {
        D3DPOOL src_pool, dst_pool;
        HRESULT hr;
    }
    tests[] =
    {
        { D3DPOOL_DEFAULT,      D3DPOOL_DEFAULT,    D3DERR_INVALIDCALL  },
        { D3DPOOL_MANAGED,      D3DPOOL_DEFAULT,    D3DERR_INVALIDCALL  },
        { D3DPOOL_SYSTEMMEM,    D3DPOOL_DEFAULT,    D3D_OK              },
        { D3DPOOL_SCRATCH,      D3DPOOL_DEFAULT,    D3DERR_INVALIDCALL  },

        { D3DPOOL_DEFAULT,      D3DPOOL_MANAGED,    D3DERR_INVALIDCALL  },
        { D3DPOOL_MANAGED,      D3DPOOL_MANAGED,    D3DERR_INVALIDCALL  },
        { D3DPOOL_SYSTEMMEM,    D3DPOOL_MANAGED,    D3DERR_INVALIDCALL  },
        { D3DPOOL_SCRATCH,      D3DPOOL_MANAGED,    D3DERR_INVALIDCALL  },

        { D3DPOOL_DEFAULT,      D3DPOOL_SYSTEMMEM,  D3DERR_INVALIDCALL  },
        { D3DPOOL_MANAGED,      D3DPOOL_SYSTEMMEM,  D3DERR_INVALIDCALL  },
        { D3DPOOL_SYSTEMMEM,    D3DPOOL_SYSTEMMEM,  D3DERR_INVALIDCALL  },
        { D3DPOOL_SCRATCH,      D3DPOOL_SYSTEMMEM,  D3DERR_INVALIDCALL  },

        { D3DPOOL_DEFAULT,      D3DPOOL_SCRATCH,    D3DERR_INVALIDCALL  },
        { D3DPOOL_MANAGED,      D3DPOOL_SCRATCH,    D3DERR_INVALIDCALL  },
        { D3DPOOL_SYSTEMMEM,    D3DPOOL_SCRATCH,    D3DERR_INVALIDCALL  },
        { D3DPOOL_SCRATCH,      D3DPOOL_SCRATCH,    D3DERR_INVALIDCALL  },
    };
    static const struct
    {
        UINT src_size, dst_size;
        UINT src_lvl, dst_lvl;
        D3DFORMAT src_fmt, dst_fmt;
    }
    tests2[] =
    {
        { 8, 8, 0, 0, D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8 },
        { 8, 8, 4, 4, D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8 },
        { 8, 8, 2, 2, D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8 },
        { 8, 8, 1, 1, D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8 },
        { 8, 8, 4, 0, D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8 },
        { 8, 8, 1, 4, D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8 }, /* Different level count */
        { 4, 8, 1, 1, D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8 }, /* Different size        */
        { 8, 8, 4, 4, D3DFMT_A8R8G8B8, D3DFMT_X8R8G8B8 }, /* Different format      */
    };

    window = create_window();
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");
    hr = IDirect3D9_GetAdapterIdentifier(d3d9, D3DADAPTER_DEFAULT, 0, &identifier);
    ok(SUCCEEDED(hr), "Failed to get adapter identifier, hr %#x.\n", hr);
    is_warp = adapter_is_warp(&identifier);
    if (!(device = create_device(d3d9, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);
    if (!(caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP))
    {
        skip("Volume textures not supported, skipping test.\n");
        goto out;
    }

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        DWORD src_usage = tests[i].src_pool == D3DPOOL_DEFAULT ? D3DUSAGE_DYNAMIC : 0;
        DWORD dst_usage = tests[i].dst_pool == D3DPOOL_DEFAULT ? D3DUSAGE_DYNAMIC : 0;

        hr = IDirect3DDevice9_CreateVolumeTexture(device, 1, 1, 1, 1, src_usage,
                D3DFMT_A8R8G8B8, tests[i].src_pool, &src, NULL);
        ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x.\n", hr);
        hr = IDirect3DDevice9_CreateVolumeTexture(device, 1, 1, 1, 1, dst_usage,
                D3DFMT_A8R8G8B8, tests[i].dst_pool, &dst, NULL);
        ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x.\n", hr);

        hr = IDirect3DVolumeTexture9_LockBox(src, 0, &locked_box, NULL, 0);
        ok(SUCCEEDED(hr), "Failed to lock volume texture, hr %#x.\n", hr);
        *((DWORD *)locked_box.pBits) = 0x11223344;
        hr = IDirect3DVolumeTexture9_UnlockBox(src, 0);
        ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);

        hr = IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9 *)src, (IDirect3DBaseTexture9 *)dst);
        ok(hr == tests[i].hr, "UpdateTexture returned %#x, expected %#x, src pool %x, dst pool %u.\n",
                hr, tests[i].hr, tests[i].src_pool, tests[i].dst_pool);

        if (SUCCEEDED(hr))
        {
            DWORD content = *((DWORD *)locked_box.pBits);
            hr = IDirect3DVolumeTexture9_LockBox(dst, 0, &locked_box, NULL, 0);
            ok(SUCCEEDED(hr), "Failed to lock volume texture, hr %#x.\n", hr);
            ok(content == 0x11223344, "Dest texture contained %#x, expected 0x11223344.\n", content);
            hr = IDirect3DVolumeTexture9_UnlockBox(dst, 0);
            ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);
        }
        IDirect3DVolumeTexture9_Release(src);
        IDirect3DVolumeTexture9_Release(dst);
    }

    if (!(caps.TextureCaps & D3DPTEXTURECAPS_MIPVOLUMEMAP))
    {
        skip("Mipmapped volume maps not supported.\n");
        goto out;
    }

    for (i = 0; i < ARRAY_SIZE(tests2); i++)
    {
        hr = IDirect3DDevice9_CreateVolumeTexture(device,
                tests2[i].src_size, tests2[i].src_size, tests2[i].src_size,
                tests2[i].src_lvl, 0, tests2[i].src_fmt, D3DPOOL_SYSTEMMEM, &src, NULL);
        ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x, case %u.\n", hr, i);
        hr = IDirect3DDevice9_CreateVolumeTexture(device,
                tests2[i].dst_size, tests2[i].dst_size, tests2[i].dst_size,
                tests2[i].dst_lvl, 0, tests2[i].dst_fmt, D3DPOOL_DEFAULT, &dst, NULL);
        ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x, case %u.\n", hr, i);

        hr = IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9 *)src, (IDirect3DBaseTexture9 *)dst);
        todo_wine_if (FAILED(hr))
            ok(SUCCEEDED(hr) || (is_warp && (i == 6 || i == 7)), /* Fails with Win10 WARP driver */
                    "Failed to update texture, hr %#x, case %u.\n", hr, i);

        IDirect3DVolumeTexture9_Release(src);
        IDirect3DVolumeTexture9_Release(dst);
    }

    /* As far as I can see, UpdateTexture on non-matching texture behaves like a memcpy. The raw data
     * stays the same in a format change, a 2x2x1 texture is copied into the first row of a 4x4x1 texture,
     * etc. I could not get it to segfault, but the nonexistent 5th pixel of a 2x2x1 texture is copied into
     * pixel 1x2x1 of a 4x4x1 texture, demonstrating a read beyond the texture's end. I suspect any bad
     * memory access is silently ignored by the runtime, in the kernel or on the GPU.
     *
     * I'm not adding tests for this behavior until an application needs it. */

out:
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_create_rt_ds_fail(void)
{
    IDirect3DDevice9 *device;
    HWND window;
    HRESULT hr;
    ULONG refcount;
    IDirect3D9 *d3d9;
    IDirect3DSurface9 *surface;

    window = create_window();
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d9, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }

    /* Output pointer == NULL segfaults on Windows. */

    surface = (IDirect3DSurface9 *)0xdeadbeef;
    hr = IDirect3DDevice9_CreateRenderTarget(device, 4, 4, D3DFMT_D16,
            D3DMULTISAMPLE_NONE, 0, FALSE, &surface, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Creating a D16 render target returned hr %#x.\n", hr);
    ok(surface == NULL, "Got pointer %p, expected NULL.\n", surface);
    if (SUCCEEDED(hr))
        IDirect3DSurface9_Release(surface);

    surface = (IDirect3DSurface9 *)0xdeadbeef;
    hr = IDirect3DDevice9_CreateDepthStencilSurface(device, 4, 4, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_NONE, 0, TRUE, &surface, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Creating a A8R8G8B8 depth stencil returned hr %#x.\n", hr);
    ok(surface == NULL, "Got pointer %p, expected NULL.\n", surface);
    if (SUCCEEDED(hr))
        IDirect3DSurface9_Release(surface);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_volume_blocks(void)
{
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d9;
    UINT refcount;
    HWND window;
    HRESULT hr;
    D3DCAPS9 caps;
    IDirect3DVolumeTexture9 *texture;
    unsigned int w, h, d, i, j;
    static const struct
    {
        D3DFORMAT fmt;
        const char *name;
        unsigned int block_width;
        unsigned int block_height;
        unsigned int block_depth;
        unsigned int block_size;
        unsigned int broken;
        BOOL create_size_checked, core_fmt;
    }
    formats[] =
    {
        /* Scratch volumes enforce DXTn block locks, unlike their surface counterparts.
         * ATI2N and YUV blocks are not enforced on any tested card (r200, gtx 460). */
        {D3DFMT_DXT1,                 "D3DFMT_DXT1", 4, 4, 1, 8,  0, TRUE,  TRUE },
        {D3DFMT_DXT2,                 "D3DFMT_DXT2", 4, 4, 1, 16, 0, TRUE,  TRUE },
        {D3DFMT_DXT3,                 "D3DFMT_DXT3", 4, 4, 1, 16, 0, TRUE,  TRUE },
        {D3DFMT_DXT4,                 "D3DFMT_DXT4", 4, 4, 1, 16, 0, TRUE,  TRUE },
        {D3DFMT_DXT5,                 "D3DFMT_DXT5", 4, 4, 1, 16, 0, TRUE,  TRUE },
        {D3DFMT_DXT5,                 "D3DFMT_DXT5", 4, 4, 1, 16, 0, TRUE,  TRUE },
        /* ATI2N has 2x2 blocks on all AMD cards and Geforce 7 cards,
         * which doesn't match the format spec. On newer Nvidia cards
         * it has the correct 4x4 block size.
         * ATI1N volume textures are only supported by AMD GPUs right
         * now and locking offsets seem just wrong. */
        {MAKEFOURCC('A','T','I','1'), "ATI1N",       4, 4, 1, 8,  2, FALSE, FALSE},
        {MAKEFOURCC('A','T','I','2'), "ATI2N",       4, 4, 1, 16, 1, FALSE, FALSE},
        {D3DFMT_YUY2,                 "D3DFMT_YUY2", 2, 1, 1, 4,  1, FALSE, TRUE },
        {D3DFMT_UYVY,                 "D3DFMT_UYVY", 2, 1, 1, 4,  1, FALSE, TRUE },
    };
    static const struct
    {
        D3DPOOL pool;
        const char *name;
        BOOL need_driver_support, need_runtime_support;
    }
    create_tests[] =
    {
        {D3DPOOL_DEFAULT,       "D3DPOOL_DEFAULT",  TRUE,  FALSE},
        {D3DPOOL_SCRATCH,       "D3DPOOL_SCRATCH",  FALSE, TRUE },
        {D3DPOOL_SYSTEMMEM,     "D3DPOOL_SYSTEMMEM",TRUE,  FALSE},
        {D3DPOOL_MANAGED,       "D3DPOOL_MANAGED",  TRUE,  FALSE},
    };
    static const struct
    {
        unsigned int x, y, z, x2, y2, z2;
    }
    offset_tests[] =
    {
        {0, 0, 0, 8, 8, 8},
        {0, 0, 3, 8, 8, 8},
        {0, 4, 0, 8, 8, 8},
        {0, 4, 3, 8, 8, 8},
        {4, 0, 0, 8, 8, 8},
        {4, 0, 3, 8, 8, 8},
        {4, 4, 0, 8, 8, 8},
        {4, 4, 3, 8, 8, 8},
    };
    D3DBOX box;
    D3DLOCKED_BOX locked_box;
    BYTE *base;
    INT expected_row_pitch, expected_slice_pitch;
    BOOL support;
    BOOL pow2;
    unsigned int offset, expected_offset;

    window = create_window();
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d9, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d9);
        DestroyWindow(window);
        return;
    }
    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);
    pow2 = !!(caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP_POW2);

    for (i = 0; i < ARRAY_SIZE(formats); i++)
    {
        hr = IDirect3D9_CheckDeviceFormat(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                0, D3DRTYPE_VOLUMETEXTURE, formats[i].fmt);
        support = SUCCEEDED(hr);

        /* Test creation restrictions */
        for (w = 1; w <= 8; w++)
        {
            for (h = 1; h <= 8; h++)
            {
                for (d = 1; d <= 8; d++)
                {
                    HRESULT expect_hr;
                    BOOL size_is_pow2;
                    BOOL block_aligned = TRUE;

                    if (w & (formats[i].block_width - 1) || h & (formats[i].block_height - 1))
                        block_aligned = FALSE;

                    size_is_pow2 = !((w & (w - 1)) || (h & (h - 1)) || (d & (d - 1)));

                    for (j = 0; j < ARRAY_SIZE(create_tests); j++)
                    {
                        BOOL may_succeed = FALSE;

                        if (create_tests[j].need_runtime_support && !formats[i].core_fmt && !support)
                            expect_hr = D3DERR_INVALIDCALL;
                        else if (formats[i].create_size_checked && !block_aligned)
                            expect_hr = D3DERR_INVALIDCALL;
                        else if (pow2 && !size_is_pow2 && create_tests[j].need_driver_support)
                            expect_hr = D3DERR_INVALIDCALL;
                        else if (create_tests[j].need_driver_support && !support)
                            expect_hr = D3DERR_INVALIDCALL;
                        else
                            expect_hr = D3D_OK;

                        texture = (IDirect3DVolumeTexture9 *)0xdeadbeef;
                        hr = IDirect3DDevice9_CreateVolumeTexture(device, w, h, d, 1, 0,
                                formats[i].fmt, create_tests[j].pool, &texture, NULL);

                        /* Wine knows about ATI2N and happily creates a scratch resource even if GL
                         * does not support it. Accept scratch creation of extension formats on
                         * Windows as well if it occurs. We don't really care if e.g. a Windows 7
                         * on an r200 GPU creates scratch ATI2N texture even though the card doesn't
                         * support it. */
                        if (!formats[i].core_fmt && !support && FAILED(expect_hr))
                            may_succeed = TRUE;

                        ok(hr == expect_hr || ((SUCCEEDED(hr) && may_succeed)),
                                "Got unexpected hr %#x for format %s, pool %s, size %ux%ux%u.\n",
                                hr, formats[i].name, create_tests[j].name, w, h, d);

                        if (FAILED(hr))
                            ok(texture == NULL, "Got texture ptr %p, expected NULL.\n", texture);
                        else
                            IDirect3DVolumeTexture9_Release(texture);
                    }
                }
            }
        }

        if (!support && !formats[i].core_fmt)
            continue;

        hr = IDirect3DDevice9_CreateVolumeTexture(device, 24, 8, 8, 1, 0,
                formats[i].fmt, D3DPOOL_SCRATCH, &texture, NULL);
        ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x.\n", hr);

        /* Test lockrect offset */
        for (j = 0; j < ARRAY_SIZE(offset_tests); j++)
        {
            unsigned int bytes_per_pixel;
            bytes_per_pixel = formats[i].block_size / (formats[i].block_width * formats[i].block_height);

            hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &locked_box, NULL, 0);
            ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);

            base = locked_box.pBits;
            if (formats[i].broken == 1)
            {
                expected_row_pitch = bytes_per_pixel * 24;
            }
            else if (formats[i].broken == 2)
            {
                expected_row_pitch = 24;
            }
            else
            {
                expected_row_pitch = (24 /* tex width */ + formats[i].block_height - 1) / formats[i].block_width
                        * formats[i].block_size;
            }
            ok(locked_box.RowPitch == expected_row_pitch, "Got unexpected row pitch %d for format %s, expected %d.\n",
                    locked_box.RowPitch, formats[i].name, expected_row_pitch);

            if (formats[i].broken)
            {
                expected_slice_pitch = expected_row_pitch * 8;
            }
            else
            {
                expected_slice_pitch = (8 /* tex height */ + formats[i].block_depth - 1) / formats[i].block_height
                        * expected_row_pitch;
            }
            ok(locked_box.SlicePitch == expected_slice_pitch,
                    "Got unexpected slice pitch %d for format %s, expected %d.\n",
                    locked_box.SlicePitch, formats[i].name, expected_slice_pitch);

            hr = IDirect3DVolumeTexture9_UnlockBox(texture, 0);
            ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x, j %u.\n", hr, j);

            box.Left = offset_tests[j].x;
            box.Top = offset_tests[j].y;
            box.Front = offset_tests[j].z;
            box.Right = offset_tests[j].x2;
            box.Bottom = offset_tests[j].y2;
            box.Back = offset_tests[j].z2;
            hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &locked_box, &box, 0);
            ok(SUCCEEDED(hr), "Failed to lock volume texture, hr %#x, j %u.\n", hr, j);

            offset = (BYTE *)locked_box.pBits - base;
            if (formats[i].broken == 1)
            {
                expected_offset = box.Front * expected_slice_pitch
                        + box.Top * expected_row_pitch
                        + box.Left * bytes_per_pixel;
            }
            else if (formats[i].broken == 2)
            {
                expected_offset = box.Front * expected_slice_pitch
                        + box.Top * expected_row_pitch
                        + box.Left;
            }
            else
            {
                expected_offset = (box.Front / formats[i].block_depth) * expected_slice_pitch
                        + (box.Top / formats[i].block_height) * expected_row_pitch
                        + (box.Left / formats[i].block_width) * formats[i].block_size;
            }
            ok(offset == expected_offset, "Got unexpected offset %u for format %s, expected %u, box start %ux%ux%u.\n",
                    offset, formats[i].name, expected_offset, box.Left, box.Top, box.Front);

            hr = IDirect3DVolumeTexture9_UnlockBox(texture, 0);
            ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);
        }

        /* Test partial block locks */
        box.Front = 0;
        box.Back = 1;
        if (formats[i].block_width > 1)
        {
            box.Left = formats[i].block_width >> 1;
            box.Top = 0;
            box.Right = formats[i].block_width;
            box.Bottom = formats[i].block_height;
            hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &locked_box, &box, 0);
            ok(FAILED(hr) || broken(formats[i].broken),
                    "Partial block lock succeeded, expected failure, format %s.\n",
                    formats[i].name);
            if (SUCCEEDED(hr))
            {
                hr = IDirect3DVolumeTexture9_UnlockBox(texture, 0);
                ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);
            }

            box.Left = 0;
            box.Top = 0;
            box.Right = formats[i].block_width >> 1;
            box.Bottom = formats[i].block_height;
            hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &locked_box, &box, 0);
            ok(FAILED(hr) || broken(formats[i].broken),
                    "Partial block lock succeeded, expected failure, format %s.\n",
                    formats[i].name);
            if (SUCCEEDED(hr))
            {
                hr = IDirect3DVolumeTexture9_UnlockBox(texture, 0);
                ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);
            }
        }

        if (formats[i].block_height > 1)
        {
            box.Left = 0;
            box.Top = formats[i].block_height >> 1;
            box.Right = formats[i].block_width;
            box.Bottom = formats[i].block_height;
            hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &locked_box, &box, 0);
            ok(FAILED(hr) || broken(formats[i].broken),
                    "Partial block lock succeeded, expected failure, format %s.\n",
                    formats[i].name);
            if (SUCCEEDED(hr))
            {
                hr = IDirect3DVolumeTexture9_UnlockBox(texture, 0);
                ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);
            }

            box.Left = 0;
            box.Top = 0;
            box.Right = formats[i].block_width;
            box.Bottom = formats[i].block_height >> 1;
            hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &locked_box, &box, 0);
            ok(FAILED(hr) || broken(formats[i].broken),
                    "Partial block lock succeeded, expected failure, format %s.\n",
                    formats[i].name);
            if (SUCCEEDED(hr))
            {
                hr = IDirect3DVolumeTexture9_UnlockBox(texture, 0);
                ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);
            }
        }

        /* Test full block lock */
        box.Left = 0;
        box.Top = 0;
        box.Right = formats[i].block_width;
        box.Bottom = formats[i].block_height;
        hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &locked_box, &box, 0);
        ok(SUCCEEDED(hr), "Failed to lock volume texture, hr %#x.\n", hr);
        hr = IDirect3DVolumeTexture9_UnlockBox(texture, 0);
        ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);

        IDirect3DVolumeTexture9_Release(texture);

        /* Test mipmap locks. Don't do this with ATI2N, AMD warns that the runtime
         * does not allocate surfaces smaller than the blocksize properly. */
        if ((formats[i].block_width > 1 || formats[i].block_height > 1) && formats[i].core_fmt)
        {
            hr = IDirect3DDevice9_CreateVolumeTexture(device, formats[i].block_width, formats[i].block_height,
                    2, 2, 0, formats[i].fmt, D3DPOOL_SCRATCH, &texture, NULL);
            ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x.\n", hr);

            hr = IDirect3DVolumeTexture9_LockBox(texture, 1, &locked_box, NULL, 0);
            ok(SUCCEEDED(hr), "Failed to lock volume texture mipmap, hr %#x.\n", hr);
            hr = IDirect3DVolumeTexture9_UnlockBox(texture, 1);
            ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);

            box.Left = box.Top = box.Front = 0;
            box.Right = formats[i].block_width == 1 ? 1 : formats[i].block_width >> 1;
            box.Bottom = formats[i].block_height == 1 ? 1 : formats[i].block_height >> 1;
            box.Back = 1;
            hr = IDirect3DVolumeTexture9_LockBox(texture, 1, &locked_box, &box, 0);
            ok(SUCCEEDED(hr), "Failed to lock volume texture mipmap, hr %#x.\n", hr);
            hr = IDirect3DVolumeTexture9_UnlockBox(texture, 1);
            ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);

            box.Right = formats[i].block_width;
            box.Bottom = formats[i].block_height;
            hr = IDirect3DVolumeTexture9_LockBox(texture, 1, &locked_box, &box, 0);
            ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
            if (SUCCEEDED(hr))
                IDirect3DVolumeTexture9_UnlockBox(texture, 1);

            IDirect3DVolumeTexture9_Release(texture);
        }
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_lockbox_invalid(void)
{
    static const struct
    {
        D3DBOX box;
        HRESULT result;
    }
    test_data[] =
    {
        {{0, 0, 2, 2, 0, 1},    D3D_OK},                /* Valid */
        {{0, 0, 4, 4, 0, 1},    D3D_OK},                /* Valid */
        {{0, 0, 0, 4, 0, 1},    D3DERR_INVALIDCALL},    /* 0 height */
        {{0, 0, 4, 0, 0, 1},    D3DERR_INVALIDCALL},    /* 0 width */
        {{0, 0, 4, 4, 1, 1},    D3DERR_INVALIDCALL},    /* 0 depth */
        {{4, 0, 0, 4, 0, 1},    D3DERR_INVALIDCALL},    /* left > right */
        {{0, 4, 4, 0, 0, 1},    D3DERR_INVALIDCALL},    /* top > bottom */
        {{0, 0, 4, 4, 1, 0},    D3DERR_INVALIDCALL},    /* back > front */
        {{0, 0, 8, 4, 0, 1},    D3DERR_INVALIDCALL},    /* right > surface */
        {{0, 0, 4, 8, 0, 1},    D3DERR_INVALIDCALL},    /* bottom > surface */
        {{0, 0, 4, 4, 0, 3},    D3DERR_INVALIDCALL},    /* back > surface */
        {{8, 0, 16, 4, 0, 1},   D3DERR_INVALIDCALL},    /* left > surface */
        {{0, 8, 4, 16, 0, 1},   D3DERR_INVALIDCALL},    /* top > surface */
        {{0, 0, 4, 4, 2, 4},    D3DERR_INVALIDCALL},    /* top > surface */
    };
    static const D3DBOX test_boxt_2 = {2, 2, 4, 4, 0, 1};
    IDirect3DVolumeTexture9 *texture = NULL;
    D3DLOCKED_BOX locked_box;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    unsigned int i;
    ULONG refcount;
    HWND window;
    BYTE *base;
    HRESULT hr;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateVolumeTexture(device, 4, 4, 2, 1, 0,
            D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &texture, NULL);
    ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x.\n", hr);
    hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &locked_box, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock volume texture, hr %#x.\n", hr);
    base = locked_box.pBits;
    hr = IDirect3DVolumeTexture9_UnlockBox(texture, 0);
    ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(test_data); ++i)
    {
        unsigned int offset, expected_offset;
        const D3DBOX *box = &test_data[i].box;

        locked_box.pBits = (BYTE *)0xdeadbeef;
        locked_box.RowPitch = 0xdeadbeef;
        locked_box.SlicePitch = 0xdeadbeef;

        hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &locked_box, box, 0);
        /* Unlike surfaces, volumes properly check the box even in Windows XP */
        ok(hr == test_data[i].result,
                "Got unexpected hr %#x with box [%u, %u, %u]->[%u, %u, %u], expected %#x.\n",
                hr, box->Left, box->Top, box->Front, box->Right, box->Bottom, box->Back,
                test_data[i].result);
        if (FAILED(hr))
            continue;

        offset = (BYTE *)locked_box.pBits - base;
        expected_offset = box->Front * locked_box.SlicePitch + box->Top * locked_box.RowPitch + box->Left * 4;
        ok(offset == expected_offset,
                "Got unexpected offset %u (expected %u) for rect [%u, %u, %u]->[%u, %u, %u].\n",
                offset, expected_offset, box->Left, box->Top, box->Front, box->Right, box->Bottom, box->Back);

        hr = IDirect3DVolumeTexture9_UnlockBox(texture, 0);
        ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);
    }

    /* locked_box = NULL throws an exception on Windows */
    hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &locked_box, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock volume texture, hr %#x.\n", hr);
    hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &locked_box, NULL, 0);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DVolumeTexture9_UnlockBox(texture, 0);
    ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);
    hr = IDirect3DVolumeTexture9_UnlockBox(texture, 0);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &locked_box, &test_data[0].box, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x for rect [%u, %u, %u]->[%u, %u, %u].\n",
            hr, test_data[0].box.Left, test_data[0].box.Top, test_data[0].box.Front,
            test_data[0].box.Right, test_data[0].box.Bottom, test_data[0].box.Back);
    hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &locked_box, &test_data[0].box, 0);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for rect [%u, %u, %u]->[%u, %u, %u].\n",
            hr, test_data[0].box.Left, test_data[0].box.Top, test_data[0].box.Front,
            test_data[0].box.Right, test_data[0].box.Bottom, test_data[0].box.Back);
    hr = IDirect3DVolumeTexture9_LockBox(texture, 0, &locked_box, &test_boxt_2, 0);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x for rect [%u, %u, %u]->[%u, %u, %u].\n",
            hr, test_boxt_2.Left, test_boxt_2.Top, test_boxt_2.Front,
            test_boxt_2.Right, test_boxt_2.Bottom, test_boxt_2.Back);
    hr = IDirect3DVolumeTexture9_UnlockBox(texture, 0);
    ok(SUCCEEDED(hr), "Failed to unlock volume texture, hr %#x.\n", hr);

    IDirect3DVolumeTexture9_Release(texture);

    hr = IDirect3DDevice9_CreateVolumeTexture(device, 4, 4, 2, 1, D3DUSAGE_WRITEONLY,
            D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &texture, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_shared_handle(void)
{
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    /* Native d3d9ex refuses to create a shared texture if the texture pointer
     * is not initialized to NULL. Make sure this doesn't cause issues here. */
    IDirect3DTexture9 *texture = NULL;
    IDirect3DSurface9 *surface = NULL;
    IDirect3DVertexBuffer9 *vertex_buffer = NULL;
    IDirect3DIndexBuffer9 *index_buffer = NULL;
    HANDLE handle = NULL;
    void *mem;
    D3DCAPS9 caps;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);
    mem = HeapAlloc(GetProcessHeap(), 0, 128 * 128 * 4);

    /* Windows XP returns E_NOTIMPL, Windows 7 returns INVALIDCALL, except for
     * CreateVertexBuffer, where it returns NOTAVAILABLE. */
    hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 1, 0, D3DFMT_A8R8G8B8,
            D3DPOOL_DEFAULT, &texture, &handle);
    ok(hr == E_NOTIMPL || broken(hr == D3DERR_INVALIDCALL), "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 1, 0, D3DFMT_A8R8G8B8,
            D3DPOOL_SYSTEMMEM, &texture, &mem);
    ok(hr == E_NOTIMPL || broken(hr == D3DERR_INVALIDCALL), "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 128, 128,
            D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &surface, &handle);
    ok(hr == E_NOTIMPL || broken(hr == D3DERR_INVALIDCALL), "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 128, 128,
            D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &surface, &mem);
    ok(hr == E_NOTIMPL || broken(hr == D3DERR_INVALIDCALL), "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateVertexBuffer(device, 16, 0, 0, D3DPOOL_DEFAULT,
            &vertex_buffer, &handle);
    ok(hr == E_NOTIMPL || broken(hr == D3DERR_INVALIDCALL), "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateVertexBuffer(device, 16, 0, 0, D3DPOOL_SYSTEMMEM,
            &vertex_buffer, &mem);
    ok(hr == E_NOTIMPL || broken(hr == D3DERR_NOTAVAILABLE), "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateIndexBuffer(device, 16, 0, 0, D3DPOOL_DEFAULT,
            &index_buffer, &handle);
    ok(hr == E_NOTIMPL || broken(hr == D3DERR_INVALIDCALL), "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateIndexBuffer(device, 16, 0, 0, D3DPOOL_SYSTEMMEM,
            &index_buffer, &mem);
    ok(hr == E_NOTIMPL || broken(hr == D3DERR_INVALIDCALL), "Got unexpected hr %#x.\n", hr);

    if (caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP)
    {
        IDirect3DCubeTexture9 *cube_texture = NULL;
        hr = IDirect3DDevice9_CreateCubeTexture(device, 8, 0, 0, D3DFMT_A8R8G8B8,
                D3DPOOL_DEFAULT, &cube_texture, &handle);
        ok(hr == E_NOTIMPL || broken(hr == D3DERR_INVALIDCALL), "Got unexpected hr %#x.\n", hr);
        hr = IDirect3DDevice9_CreateCubeTexture(device, 8, 0, 0, D3DFMT_A8R8G8B8,
                D3DPOOL_SYSTEMMEM, &cube_texture, &mem);
        ok(hr == E_NOTIMPL || broken(hr == D3DERR_INVALIDCALL), "Got unexpected hr %#x.\n", hr);
    }

    if (caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP)
    {
        IDirect3DVolumeTexture9 *volume_texture = NULL;
        hr = IDirect3DDevice9_CreateVolumeTexture(device, 4, 4, 4, 0, 0, D3DFMT_A8R8G8B8,
                D3DPOOL_DEFAULT, &volume_texture, &handle);
        ok(hr == E_NOTIMPL || broken(hr == D3DERR_INVALIDCALL), "Got unexpected hr %#x.\n", hr);
        hr = IDirect3DDevice9_CreateVolumeTexture(device, 4, 4, 4, 0, 0, D3DFMT_A8R8G8B8,
                D3DPOOL_SYSTEMMEM, &volume_texture, &mem);
        ok(hr == E_NOTIMPL || broken(hr == D3DERR_INVALIDCALL), "Got unexpected hr %#x.\n", hr);
    }

    hr = IDirect3DDevice9_CreateRenderTarget(device, 128, 128, D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_NONE, 0, TRUE, &surface, &handle);
    ok(hr == E_NOTIMPL || broken(hr == D3DERR_INVALIDCALL), "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateDepthStencilSurface(device, 128, 128, D3DFMT_D24X8,
            D3DMULTISAMPLE_NONE, 0, TRUE, &surface, &handle);
    ok(hr == E_NOTIMPL || broken(hr == D3DERR_INVALIDCALL), "Got unexpected hr %#x.\n", hr);

    HeapFree(GetProcessHeap(), 0, mem);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_pixel_format(void)
{
    int format, test_format;
    PIXELFORMATDESCRIPTOR pfd;
    IDirect3D9 *d3d9 = NULL;
    IDirect3DDevice9 *device = NULL;
    HWND hwnd, hwnd2;
    HDC hdc, hdc2;
    HMODULE gl;
    HRESULT hr;

    static const float point[] = {0.0f, 0.0f, 0.0f};

    hwnd = create_window();
    ok(!!hwnd, "Failed to create window.\n");
    hwnd2 = create_window();
    ok(!!hwnd2, "Failed to create window.\n");

    hdc = GetDC(hwnd);
    ok(!!hdc, "Failed to get DC.\n");
    hdc2 = GetDC(hwnd2);
    ok(!!hdc2, "Failed to get DC.\n");

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

    if (!SetPixelFormat(hdc2, format, &pfd) || GetPixelFormat(hdc2) != format)
    {
        skip("failed to set pixel format on second window\n");
        goto cleanup;
    }

    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");

    test_format = GetPixelFormat(hdc);
    ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

    if (!(device = create_device(d3d9, hwnd, NULL)))
    {
        skip("Failed to create device\n");
        goto cleanup;
    }

    test_format = GetPixelFormat(hdc);
    ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
    ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);

    test_format = GetPixelFormat(hdc);
    ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "BeginScene failed %#x\n", hr);

    test_format = GetPixelFormat(hdc);
    ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_POINTLIST, 1, point, 3 * sizeof(float));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);

    test_format = GetPixelFormat(hdc);
    ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "EndScene failed %#x\n", hr);

    test_format = GetPixelFormat(hdc);
    ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Present failed %#x\n", hr);

    test_format = GetPixelFormat(hdc);
    ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

    hr = IDirect3DDevice9_Present(device, NULL, NULL, hwnd2, NULL);
    ok(SUCCEEDED(hr), "Present failed %#x\n", hr);

    test_format = GetPixelFormat(hdc);
    ok(test_format == format, "window has pixel format %d, expected %d\n", test_format, format);

    test_format = GetPixelFormat(hdc2);
    ok(test_format == format, "second window has pixel format %d, expected %d\n", test_format, format);

cleanup:
    if (device)
    {
        UINT refcount = IDirect3DDevice9_Release(device);
        ok(!refcount, "Device has %u references left.\n", refcount);
    }
    if (d3d9)
        IDirect3D9_Release(d3d9);
    FreeLibrary(gl);
    ReleaseDC(hwnd2, hdc2);
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd2);
    DestroyWindow(hwnd);
}

static void test_begin_end_state_block(void)
{
    IDirect3DStateBlock9 *stateblock, *stateblock2;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    DWORD value;
    HWND window;
    HRESULT hr;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_BeginStateBlock(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    stateblock = (IDirect3DStateBlock9 *)0xdeadbeef;
    hr = IDirect3DDevice9_EndStateBlock(device, &stateblock);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(!!stateblock && stateblock != (IDirect3DStateBlock9 *)0xdeadbeef,
            "Got unexpected stateblock %p.\n", stateblock);

    stateblock2 = (IDirect3DStateBlock9 *)0xdeadbeef;
    hr = IDirect3DDevice9_EndStateBlock(device, &stateblock2);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    ok(stateblock2 == (IDirect3DStateBlock9 *)0xdeadbeef,
            "Got unexpected stateblock %p.\n", stateblock2);

    hr = IDirect3DDevice9_GetRenderState(device, D3DRS_LIGHTING, &value);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(value == TRUE, "Got unexpected value %#x.\n", value);

    hr = IDirect3DDevice9_BeginStateBlock(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginStateBlock(device);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DStateBlock9_Apply(stateblock);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DStateBlock9_Capture(stateblock);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateStateBlock(device, D3DSBT_ALL, &stateblock2);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetRenderState(device, D3DRS_LIGHTING, &value);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(value == TRUE, "Got unexpected value %#x.\n", value);

    hr = IDirect3DDevice9_EndStateBlock(device, &stateblock2);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DStateBlock9_Apply(stateblock2);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetRenderState(device, D3DRS_LIGHTING, &value);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(value == TRUE, "Got unexpected value %#x.\n", value);

    IDirect3DStateBlock9_Release(stateblock);
    IDirect3DStateBlock9_Release(stateblock2);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_shader_constant_apply(void)
{
    static const float vs_const[] = {1.0f, 2.0f, 3.0f, 4.0f};
    static const float ps_const[] = {5.0f, 6.0f, 7.0f, 8.0f};
    static const float initial[] = {0.0f, 0.0f, 0.0f, 0.0f};
    IDirect3DStateBlock9 *stateblock;
    DWORD vs_version, ps_version;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    D3DCAPS9 caps;
    float ret[4];
    HWND window;
    HRESULT hr;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);
    vs_version = caps.VertexShaderVersion & 0xffff;
    ps_version = caps.PixelShaderVersion & 0xffff;

    if (vs_version)
    {
        hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 0, initial, 1);
        ok(SUCCEEDED(hr), "Failed to set vertex shader constant, hr %#x.\n", hr);
        hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 1, initial, 1);
        ok(SUCCEEDED(hr), "Failed to set vertex shader constant, hr %#x.\n", hr);

        hr = IDirect3DDevice9_GetVertexShaderConstantF(device, 0, ret, 1);
        ok(SUCCEEDED(hr), "Failed to get vertex shader constant, hr %#x.\n", hr);
        ok(!memcmp(ret, initial, sizeof(initial)),
                "Got unexpected vertex shader constant {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                ret[0], ret[1], ret[2], ret[3], initial[0], initial[1], initial[2], initial[3]);
        hr = IDirect3DDevice9_GetVertexShaderConstantF(device, 1, ret, 1);
        ok(SUCCEEDED(hr), "Failed to get vertex shader constant, hr %#x.\n", hr);
        ok(!memcmp(ret, initial, sizeof(initial)),
                "Got unexpected vertex shader constant {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                ret[0], ret[1], ret[2], ret[3], initial[0], initial[1], initial[2], initial[3]);

        hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 0, vs_const, 1);
        ok(SUCCEEDED(hr), "Failed to set vertex shader constant, hr %#x.\n", hr);
    }
    if (ps_version)
    {
        hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 0, initial, 1);
        ok(SUCCEEDED(hr), "Failed to set pixel shader constant, hr %#x.\n", hr);
        hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 1, initial, 1);
        ok(SUCCEEDED(hr), "Failed to set pixel shader constant, hr %#x.\n", hr);

        hr = IDirect3DDevice9_GetPixelShaderConstantF(device, 0, ret, 1);
        ok(SUCCEEDED(hr), "Failed to get pixel shader constant, hr %#x.\n", hr);
        ok(!memcmp(ret, initial, sizeof(initial)),
                "Got unexpected pixel shader constant {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                ret[0], ret[1], ret[2], ret[3], initial[0], initial[1], initial[2], initial[3]);
        hr = IDirect3DDevice9_GetPixelShaderConstantF(device, 1, ret, 1);
        ok(SUCCEEDED(hr), "Failed to get pixel shader constant, hr %#x.\n", hr);
        ok(!memcmp(ret, initial, sizeof(initial)),
                "Got unexpected pixel shader constant {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                ret[0], ret[1], ret[2], ret[3], initial[0], initial[1], initial[2], initial[3]);

        hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 0, ps_const, 1);
        ok(SUCCEEDED(hr), "Failed to set pixel shader constant, hr %#x.\n", hr);
    }

    hr = IDirect3DDevice9_BeginStateBlock(device);
    ok(SUCCEEDED(hr), "Failed to begin stateblock, hr %#x.\n", hr);

    if (vs_version)
    {
        hr = IDirect3DDevice9_SetVertexShaderConstantF(device, 1, vs_const, 1);
        ok(SUCCEEDED(hr), "Failed to set vertex shader constant, hr %#x.\n", hr);
    }
    if (ps_version)
    {
        hr = IDirect3DDevice9_SetPixelShaderConstantF(device, 1, ps_const, 1);
        ok(SUCCEEDED(hr), "Failed to set pixel shader constant, hr %#x.\n", hr);
    }

    hr = IDirect3DDevice9_EndStateBlock(device, &stateblock);
    ok(SUCCEEDED(hr), "Failed to end stateblock, hr %#x.\n", hr);

    if (vs_version)
    {
        hr = IDirect3DDevice9_GetVertexShaderConstantF(device, 0, ret, 1);
        ok(SUCCEEDED(hr), "Failed to get vertex shader constant, hr %#x.\n", hr);
        ok(!memcmp(ret, vs_const, sizeof(vs_const)),
                "Got unexpected vertex shader constant {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                ret[0], ret[1], ret[2], ret[3], vs_const[0], vs_const[1], vs_const[2], vs_const[3]);
        hr = IDirect3DDevice9_GetVertexShaderConstantF(device, 1, ret, 1);
        ok(SUCCEEDED(hr), "Failed to get vertex shader constant, hr %#x.\n", hr);
        ok(!memcmp(ret, initial, sizeof(initial)),
                "Got unexpected vertex shader constant {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                ret[0], ret[1], ret[2], ret[3], initial[0], initial[1], initial[2], initial[3]);
    }
    if (ps_version)
    {
        hr = IDirect3DDevice9_GetPixelShaderConstantF(device, 0, ret, 1);
        ok(SUCCEEDED(hr), "Failed to get pixel shader constant, hr %#x.\n", hr);
        ok(!memcmp(ret, ps_const, sizeof(ps_const)),
                "Got unexpected pixel shader constant {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                ret[0], ret[1], ret[2], ret[3], ps_const[0], ps_const[1], ps_const[2], ps_const[3]);
        hr = IDirect3DDevice9_GetPixelShaderConstantF(device, 1, ret, 1);
        ok(SUCCEEDED(hr), "Failed to get pixel shader constant, hr %#x.\n", hr);
        ok(!memcmp(ret, initial, sizeof(initial)),
                "Got unexpected pixel shader constant {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                ret[0], ret[1], ret[2], ret[3], initial[0], initial[1], initial[2], initial[3]);
    }

    /* Apply doesn't overwrite constants that aren't explicitly set on the
     * source stateblock. */
    hr = IDirect3DStateBlock9_Apply(stateblock);
    ok(SUCCEEDED(hr), "Failed to apply stateblock, hr %#x.\n", hr);

    if (vs_version)
    {
        hr = IDirect3DDevice9_GetVertexShaderConstantF(device, 0, ret, 1);
        ok(SUCCEEDED(hr), "Failed to get vertex shader constant, hr %#x.\n", hr);
        ok(!memcmp(ret, vs_const, sizeof(vs_const)),
                "Got unexpected vertex shader constant {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                ret[0], ret[1], ret[2], ret[3], vs_const[0], vs_const[1], vs_const[2], vs_const[3]);
        hr = IDirect3DDevice9_GetVertexShaderConstantF(device, 1, ret, 1);
        ok(SUCCEEDED(hr), "Failed to get vertex shader constant, hr %#x.\n", hr);
        ok(!memcmp(ret, vs_const, sizeof(vs_const)),
                "Got unexpected vertex shader constant {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                ret[0], ret[1], ret[2], ret[3], vs_const[0], vs_const[1], vs_const[2], vs_const[3]);
    }
    if (ps_version)
    {
        hr = IDirect3DDevice9_GetPixelShaderConstantF(device, 0, ret, 1);
        ok(SUCCEEDED(hr), "Failed to get pixel shader constant, hr %#x.\n", hr);
        ok(!memcmp(ret, ps_const, sizeof(ps_const)),
                "Got unexpected pixel shader constant {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                ret[0], ret[1], ret[2], ret[3], ps_const[0], ps_const[1], ps_const[2], ps_const[3]);
        hr = IDirect3DDevice9_GetPixelShaderConstantF(device, 1, ret, 1);
        ok(SUCCEEDED(hr), "Failed to get pixel shader constant, hr %#x.\n", hr);
        ok(!memcmp(ret, ps_const, sizeof(ps_const)),
                "Got unexpected pixel shader constant {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                ret[0], ret[1], ret[2], ret[3], ps_const[0], ps_const[1], ps_const[2], ps_const[3]);
    }

    IDirect3DStateBlock9_Release(stateblock);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_vdecl_apply(void)
{
    IDirect3DVertexDeclaration9 *declaration, *declaration1, *declaration2;
    IDirect3DStateBlock9 *stateblock;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    static const D3DVERTEXELEMENT9 decl1[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
        D3DDECL_END(),
    };

    static const D3DVERTEXELEMENT9 decl2[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
        {0, 16, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
        D3DDECL_END(),
    };

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl1, &declaration1);
    ok(SUCCEEDED(hr), "CreateVertexDeclaration failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateVertexDeclaration(device, decl2, &declaration2);
    ok(SUCCEEDED(hr), "CreateVertexDeclaration failed, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetVertexDeclaration(device, NULL);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_BeginStateBlock(device);
    ok(SUCCEEDED(hr), "BeginStateBlock failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexDeclaration(device, declaration1);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndStateBlock(device, &stateblock);
    ok(SUCCEEDED(hr), "EndStateBlock failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexDeclaration(device, NULL);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed, hr %#x.\n", hr);
    hr = IDirect3DStateBlock9_Apply(stateblock);
    ok(SUCCEEDED(hr), "Apply failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetVertexDeclaration(device, &declaration);
    ok(SUCCEEDED(hr), "GetVertexDeclaration failed, hr %#x.\n", hr);
    ok(declaration == declaration1, "Got unexpected vertex declaration %p, expected %p.\n",
            declaration, declaration1);
    IDirect3DVertexDeclaration9_Release(declaration);

    hr = IDirect3DDevice9_SetVertexDeclaration(device, NULL);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed, hr %#x.\n", hr);
    hr = IDirect3DStateBlock9_Capture(stateblock);
    ok(SUCCEEDED(hr), "Capture failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexDeclaration(device, declaration2);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed, hr %#x.\n", hr);
    hr = IDirect3DStateBlock9_Apply(stateblock);
    ok(SUCCEEDED(hr), "Apply failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetVertexDeclaration(device, &declaration);
    ok(SUCCEEDED(hr), "GetVertexDeclaration failed, hr %#x.\n", hr);
    ok(declaration == declaration2, "Got unexpected vertex declaration %p, expected %p.\n",
            declaration, declaration2);
    IDirect3DVertexDeclaration9_Release(declaration);

    hr = IDirect3DDevice9_SetVertexDeclaration(device, declaration2);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed, hr %#x.\n", hr);
    hr = IDirect3DStateBlock9_Capture(stateblock);
    ok(SUCCEEDED(hr), "Capture failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexDeclaration(device, NULL);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed, hr %#x.\n", hr);
    hr = IDirect3DStateBlock9_Apply(stateblock);
    ok(SUCCEEDED(hr), "Apply failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetVertexDeclaration(device, &declaration);
    ok(SUCCEEDED(hr), "GetVertexDeclaration failed, hr %#x.\n", hr);
    ok(declaration == declaration2, "Got unexpected vertex declaration %p, expected %p.\n",
            declaration, declaration2);
    IDirect3DVertexDeclaration9_Release(declaration);

    hr = IDirect3DDevice9_SetVertexDeclaration(device, NULL);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed, hr %#x.\n", hr);
    hr = IDirect3DStateBlock9_Capture(stateblock);
    ok(SUCCEEDED(hr), "Capture failed, hr %#x.\n", hr);
    hr = IDirect3DStateBlock9_Apply(stateblock);
    ok(SUCCEEDED(hr), "Apply failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetVertexDeclaration(device, &declaration);
    ok(SUCCEEDED(hr), "GetVertexDeclaration failed, hr %#x.\n", hr);
    ok(!declaration, "Got unexpected vertex declaration %p.\n", declaration);

    hr = IDirect3DDevice9_SetVertexDeclaration(device, declaration2);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed, hr %#x.\n", hr);
    hr = IDirect3DStateBlock9_Capture(stateblock);
    ok(SUCCEEDED(hr), "Capture failed, hr %#x.\n", hr);
    hr = IDirect3DStateBlock9_Apply(stateblock);
    ok(SUCCEEDED(hr), "Apply failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetVertexDeclaration(device, &declaration);
    ok(SUCCEEDED(hr), "GetVertexDeclaration failed, hr %#x.\n", hr);
    ok(declaration == declaration2, "Got unexpected vertex declaration %p, expected %p.\n",
            declaration, declaration2);
    IDirect3DVertexDeclaration9_Release(declaration);

    IDirect3DStateBlock9_Release(stateblock);
    hr = IDirect3DDevice9_SetVertexDeclaration(device, declaration1);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_CreateStateBlock(device, D3DSBT_VERTEXSTATE, &stateblock);
    ok(SUCCEEDED(hr), "CreateStateBlock failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexDeclaration(device, NULL);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed, hr %#x.\n", hr);
    hr = IDirect3DStateBlock9_Apply(stateblock);
    ok(SUCCEEDED(hr), "Apply failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetVertexDeclaration(device, &declaration);
    ok(SUCCEEDED(hr), "GetVertexDeclaration failed, hr %#x.\n", hr);
    ok(declaration == declaration1, "Got unexpected vertex declaration %p, expected %p.\n",
            declaration, declaration1);
    IDirect3DVertexDeclaration9_Release(declaration);

    hr = IDirect3DDevice9_SetVertexDeclaration(device, NULL);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed, hr %#x.\n", hr);
    hr = IDirect3DStateBlock9_Capture(stateblock);
    ok(SUCCEEDED(hr), "Capture failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexDeclaration(device, declaration2);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed, hr %#x.\n", hr);
    hr = IDirect3DStateBlock9_Apply(stateblock);
    ok(SUCCEEDED(hr), "Apply failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetVertexDeclaration(device, &declaration);
    ok(SUCCEEDED(hr), "GetVertexDeclaration failed, hr %#x.\n", hr);
    ok(declaration == declaration2, "Got unexpected vertex declaration %p, expected %p.\n",
            declaration, declaration2);
    IDirect3DVertexDeclaration9_Release(declaration);

    hr = IDirect3DDevice9_SetVertexDeclaration(device, declaration2);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed, hr %#x.\n", hr);
    hr = IDirect3DStateBlock9_Capture(stateblock);
    ok(SUCCEEDED(hr), "Capture failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetVertexDeclaration(device, NULL);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed, hr %#x.\n", hr);
    hr = IDirect3DStateBlock9_Apply(stateblock);
    ok(SUCCEEDED(hr), "Apply failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetVertexDeclaration(device, &declaration);
    ok(SUCCEEDED(hr), "GetVertexDeclaration failed, hr %#x.\n", hr);
    ok(declaration == declaration2, "Got unexpected vertex declaration %p, expected %p.\n",
            declaration, declaration2);
    IDirect3DVertexDeclaration9_Release(declaration);

    hr = IDirect3DDevice9_SetVertexDeclaration(device, NULL);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed, hr %#x.\n", hr);
    hr = IDirect3DStateBlock9_Capture(stateblock);
    ok(SUCCEEDED(hr), "Capture failed, hr %#x.\n", hr);
    hr = IDirect3DStateBlock9_Apply(stateblock);
    ok(SUCCEEDED(hr), "Apply failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetVertexDeclaration(device, &declaration);
    ok(SUCCEEDED(hr), "GetVertexDeclaration failed, hr %#x.\n", hr);
    ok(!declaration, "Got unexpected vertex declaration %p.\n", declaration);

    hr = IDirect3DDevice9_SetVertexDeclaration(device, declaration2);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed, hr %#x.\n", hr);
    hr = IDirect3DStateBlock9_Capture(stateblock);
    ok(SUCCEEDED(hr), "Capture failed, hr %#x.\n", hr);
    hr = IDirect3DStateBlock9_Apply(stateblock);
    ok(SUCCEEDED(hr), "Apply failed, hr %#x.\n", hr);
    hr = IDirect3DDevice9_GetVertexDeclaration(device, &declaration);
    ok(SUCCEEDED(hr), "GetVertexDeclaration failed, hr %#x.\n", hr);
    ok(declaration == declaration2, "Got unexpected vertex declaration %p, expected %p.\n",
            declaration, declaration2);
    IDirect3DVertexDeclaration9_Release(declaration);

    hr = IDirect3DDevice9_SetVertexDeclaration(device, NULL);
    ok(SUCCEEDED(hr), "SetVertexDeclaration failed, hr %#x.\n", hr);
    IDirect3DVertexDeclaration9_Release(declaration1);
    IDirect3DVertexDeclaration9_Release(declaration2);
    IDirect3DStateBlock9_Release(stateblock);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_resource_type(void)
{
    IDirect3DDevice9 *device;
    IDirect3DSurface9 *surface;
    IDirect3DTexture9 *texture;
    IDirect3DCubeTexture9 *cube_texture;
    IDirect3DVolume9 *volume;
    IDirect3DVolumeTexture9 *volume_texture;
    D3DSURFACE_DESC surface_desc;
    D3DVOLUME_DESC volume_desc;
    D3DRESOURCETYPE type;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    D3DCAPS9 caps;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 4, 4, D3DFMT_X8R8G8B8,
            D3DPOOL_SYSTEMMEM, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#x.\n", hr);
    type = IDirect3DSurface9_GetType(surface);
    ok(type == D3DRTYPE_SURFACE, "Expected type D3DRTYPE_SURFACE, got %u.\n", type);
    hr = IDirect3DSurface9_GetDesc(surface, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface description, hr %#x.\n", hr);
    ok(surface_desc.Type == D3DRTYPE_SURFACE, "Expected type D3DRTYPE_SURFACE, got %u.\n",
            surface_desc.Type);
    IDirect3DSurface9_Release(surface);

    hr = IDirect3DDevice9_CreateTexture(device, 2, 8, 4, 0, D3DFMT_X8R8G8B8,
            D3DPOOL_SYSTEMMEM, &texture, NULL);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    type = IDirect3DTexture9_GetType(texture);
    ok(type == D3DRTYPE_TEXTURE, "Expected type D3DRTYPE_TEXTURE, got %u.\n", type);

    hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x.\n", hr);
    /* The following code crashes, for the sake of completeness:
     * type = texture->lpVtbl->GetType((IDirect3DTexture9 *)surface);
     * ok(type == D3DRTYPE_PONIES, "Expected type D3DRTYPE_PONIES, got %u.\n", type);
     *
     * So applications will not depend on getting the "right" resource type - whatever it
     * may be - from the "wrong" vtable. */
    type = IDirect3DSurface9_GetType(surface);
    ok(type == D3DRTYPE_SURFACE, "Expected type D3DRTYPE_SURFACE, got %u.\n", type);
    hr = IDirect3DSurface9_GetDesc(surface, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface description, hr %#x.\n", hr);
    ok(surface_desc.Type == D3DRTYPE_SURFACE, "Expected type D3DRTYPE_SURFACE, got %u.\n",
            surface_desc.Type);
    ok(surface_desc.Width == 2, "Expected width 2, got %u.\n", surface_desc.Width);
    ok(surface_desc.Height == 8, "Expected height 8, got %u.\n", surface_desc.Height);
    hr = IDirect3DTexture9_GetLevelDesc(texture, 0, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get level description, hr %#x.\n", hr);
    ok(surface_desc.Type == D3DRTYPE_SURFACE, "Expected type D3DRTYPE_SURFACE, got %u.\n",
            surface_desc.Type);
    ok(surface_desc.Width == 2, "Expected width 2, got %u.\n", surface_desc.Width);
    ok(surface_desc.Height == 8, "Expected height 8, got %u.\n", surface_desc.Height);
    IDirect3DSurface9_Release(surface);

    hr = IDirect3DTexture9_GetSurfaceLevel(texture, 2, &surface);
    ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x.\n", hr);
    type = IDirect3DSurface9_GetType(surface);
    ok(type == D3DRTYPE_SURFACE, "Expected type D3DRTYPE_SURFACE, got %u.\n", type);
    hr = IDirect3DSurface9_GetDesc(surface, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface description, hr %#x.\n", hr);
    ok(surface_desc.Type == D3DRTYPE_SURFACE, "Expected type D3DRTYPE_SURFACE, got %u.\n",
            surface_desc.Type);
    ok(surface_desc.Width == 1, "Expected width 1, got %u.\n", surface_desc.Width);
    ok(surface_desc.Height == 2, "Expected height 2, got %u.\n", surface_desc.Height);
    hr = IDirect3DTexture9_GetLevelDesc(texture, 2, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get level description, hr %#x.\n", hr);
    ok(surface_desc.Type == D3DRTYPE_SURFACE, "Expected type D3DRTYPE_SURFACE, got %u.\n",
            surface_desc.Type);
    ok(surface_desc.Width == 1, "Expected width 1, got %u.\n", surface_desc.Width);
    ok(surface_desc.Height == 2, "Expected height 2, got %u.\n", surface_desc.Height);
    IDirect3DSurface9_Release(surface);
    IDirect3DTexture9_Release(texture);

    if (caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP)
    {
        hr = IDirect3DDevice9_CreateCubeTexture(device, 1, 1, 0, D3DFMT_X8R8G8B8,
                D3DPOOL_SYSTEMMEM, &cube_texture, NULL);
        ok(SUCCEEDED(hr), "Failed to create cube texture, hr %#x.\n", hr);
        type = IDirect3DCubeTexture9_GetType(cube_texture);
        ok(type == D3DRTYPE_CUBETEXTURE, "Expected type D3DRTYPE_CUBETEXTURE, got %u.\n", type);

        hr = IDirect3DCubeTexture9_GetCubeMapSurface(cube_texture,
                D3DCUBEMAP_FACE_NEGATIVE_X, 0, &surface);
        ok(SUCCEEDED(hr), "Failed to get cube map surface, hr %#x.\n", hr);
        type = IDirect3DSurface9_GetType(surface);
        ok(type == D3DRTYPE_SURFACE, "Expected type D3DRTYPE_SURFACE, got %u.\n", type);
        hr = IDirect3DSurface9_GetDesc(surface, &surface_desc);
        ok(SUCCEEDED(hr), "Failed to get surface description, hr %#x.\n", hr);
        ok(surface_desc.Type == D3DRTYPE_SURFACE, "Expected type D3DRTYPE_SURFACE, got %u.\n",
                surface_desc.Type);
        hr = IDirect3DCubeTexture9_GetLevelDesc(cube_texture, 0, &surface_desc);
        ok(SUCCEEDED(hr), "Failed to get level description, hr %#x.\n", hr);
        ok(surface_desc.Type == D3DRTYPE_SURFACE, "Expected type D3DRTYPE_SURFACE, got %u.\n",
                surface_desc.Type);
        IDirect3DSurface9_Release(surface);
        IDirect3DCubeTexture9_Release(cube_texture);
    }
    else
        skip("Cube maps not supported.\n");

    if (caps.TextureCaps & D3DPTEXTURECAPS_MIPVOLUMEMAP)
    {
        hr = IDirect3DDevice9_CreateVolumeTexture(device, 2, 4, 8, 4, 0, D3DFMT_X8R8G8B8,
                D3DPOOL_SYSTEMMEM, &volume_texture, NULL);
        ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#x.\n", hr);
        type = IDirect3DVolumeTexture9_GetType(volume_texture);
        ok(type == D3DRTYPE_VOLUMETEXTURE, "Expected type D3DRTYPE_VOLUMETEXTURE, got %u.\n", type);

        hr = IDirect3DVolumeTexture9_GetVolumeLevel(volume_texture, 0, &volume);
        ok(SUCCEEDED(hr), "Failed to get volume level, hr %#x.\n", hr);
        /* IDirect3DVolume9 is not an IDirect3DResource9 and has no GetType method. */
        hr = IDirect3DVolume9_GetDesc(volume, &volume_desc);
        ok(SUCCEEDED(hr), "Failed to get volume description, hr %#x.\n", hr);
        ok(volume_desc.Type == D3DRTYPE_VOLUME, "Expected type D3DRTYPE_VOLUME, got %u.\n",
                volume_desc.Type);
        ok(volume_desc.Width == 2, "Expected width 2, got %u.\n", volume_desc.Width);
        ok(volume_desc.Height == 4, "Expected height 4, got %u.\n", volume_desc.Height);
        ok(volume_desc.Depth == 8, "Expected depth 8, got %u.\n", volume_desc.Depth);
        hr = IDirect3DVolumeTexture9_GetLevelDesc(volume_texture, 0, &volume_desc);
        ok(SUCCEEDED(hr), "Failed to get level description, hr %#x.\n", hr);
        ok(volume_desc.Type == D3DRTYPE_VOLUME, "Expected type D3DRTYPE_VOLUME, got %u.\n",
                volume_desc.Type);
        ok(volume_desc.Width == 2, "Expected width 2, got %u.\n", volume_desc.Width);
        ok(volume_desc.Height == 4, "Expected height 4, got %u.\n", volume_desc.Height);
        ok(volume_desc.Depth == 8, "Expected depth 8, got %u.\n", volume_desc.Depth);
        IDirect3DVolume9_Release(volume);

        hr = IDirect3DVolumeTexture9_GetVolumeLevel(volume_texture, 2, &volume);
        ok(SUCCEEDED(hr), "Failed to get volume level, hr %#x.\n", hr);
        /* IDirect3DVolume9 is not an IDirect3DResource9 and has no GetType method. */
        hr = IDirect3DVolume9_GetDesc(volume, &volume_desc);
        ok(SUCCEEDED(hr), "Failed to get volume description, hr %#x.\n", hr);
        ok(volume_desc.Type == D3DRTYPE_VOLUME, "Expected type D3DRTYPE_VOLUME, got %u.\n",
                volume_desc.Type);
        ok(volume_desc.Width == 1, "Expected width 1, got %u.\n", volume_desc.Width);
        ok(volume_desc.Height == 1, "Expected height 1, got %u.\n", volume_desc.Height);
        ok(volume_desc.Depth == 2, "Expected depth 2, got %u.\n", volume_desc.Depth);
        hr = IDirect3DVolumeTexture9_GetLevelDesc(volume_texture, 2, &volume_desc);
        ok(SUCCEEDED(hr), "Failed to get level description, hr %#x.\n", hr);
        ok(volume_desc.Type == D3DRTYPE_VOLUME, "Expected type D3DRTYPE_VOLUME, got %u.\n",
                volume_desc.Type);
        ok(volume_desc.Width == 1, "Expected width 1, got %u.\n", volume_desc.Width);
        ok(volume_desc.Height == 1, "Expected height 1, got %u.\n", volume_desc.Height);
        ok(volume_desc.Depth == 2, "Expected depth 2, got %u.\n", volume_desc.Depth);
        IDirect3DVolume9_Release(volume);
        IDirect3DVolumeTexture9_Release(volume_texture);
    }
    else
        skip("Mipmapped volume maps not supported.\n");

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_mipmap_lock(void)
{
    IDirect3DDevice9 *device;
    IDirect3DSurface9 *surface, *surface2, *surface_dst, *surface_dst2;
    IDirect3DTexture9 *texture, *texture_dst;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    D3DLOCKED_RECT locked_rect;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateTexture(device, 4, 4, 2, 0, D3DFMT_X8R8G8B8,
            D3DPOOL_DEFAULT, &texture_dst, NULL);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    hr = IDirect3DTexture9_GetSurfaceLevel(texture_dst, 0, &surface_dst);
    ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x.\n", hr);
    hr = IDirect3DTexture9_GetSurfaceLevel(texture_dst, 1, &surface_dst2);
    ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateTexture(device, 4, 4, 2, 0, D3DFMT_X8R8G8B8,
            D3DPOOL_SYSTEMMEM, &texture, NULL);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#x.\n", hr);
    hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
    ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x.\n", hr);
    hr = IDirect3DTexture9_GetSurfaceLevel(texture, 1, &surface2);
    ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x.\n", hr);

    hr = IDirect3DSurface9_LockRect(surface, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    hr = IDirect3DSurface9_LockRect(surface2, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#x.\n", hr);
    hr = IDirect3DSurface9_UnlockRect(surface);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    hr = IDirect3DDevice9_UpdateSurface(device, surface, NULL, surface_dst, NULL);
    ok(SUCCEEDED(hr), "Failed to update surface, hr %#x.\n", hr);
    hr = IDirect3DDevice9_UpdateSurface(device, surface2, NULL, surface_dst2, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    /* Apparently there's no validation on the container. */
    hr = IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9 *)texture,
            (IDirect3DBaseTexture9 *)texture_dst);
    ok(SUCCEEDED(hr), "Failed to update texture, hr %#x.\n", hr);

    hr = IDirect3DSurface9_UnlockRect(surface2);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#x.\n", hr);

    IDirect3DSurface9_Release(surface_dst2);
    IDirect3DSurface9_Release(surface_dst);
    IDirect3DSurface9_Release(surface2);
    IDirect3DSurface9_Release(surface);
    IDirect3DTexture9_Release(texture_dst);
    IDirect3DTexture9_Release(texture);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_writeonly_resource(void)
{
    IDirect3D9 *d3d;
    IDirect3DDevice9 *device;
    IDirect3DVertexBuffer9 *buffer;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    void *ptr;
    static const struct
    {
        struct vec3 pos;
    }
    quad[] =
    {
        {{-1.0f, -1.0f, 0.0f}},
        {{-1.0f,  1.0f, 0.0f}},
        {{ 1.0f, -1.0f, 0.0f}},
        {{ 1.0f,  1.0f, 0.0f}}
    };

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateVertexBuffer(device, sizeof(quad),
            D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &buffer, NULL);
    ok(SUCCEEDED(hr), "Failed to create buffer, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer9_Lock(buffer, 0, 0, &ptr, D3DLOCK_DISCARD);
    ok(SUCCEEDED(hr), "Failed to lock vertex buffer, hr %#x.\n", hr);
    memcpy(ptr, quad, sizeof(quad));
    hr = IDirect3DVertexBuffer9_Unlock(buffer);
    ok(SUCCEEDED(hr), "Failed to unlock vertex buffer, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetStreamSource(device, 0, buffer, 0, sizeof(*quad));
    ok(SUCCEEDED(hr), "Failed to set stream source, hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
    ok(SUCCEEDED(hr), "Failed to set FVF, hr %#x.\n", hr);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene %#x\n", hr);
    hr = IDirect3DDevice9_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, 0, 2);
    ok(SUCCEEDED(hr), "Failed to draw, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene %#x\n", hr);

    hr = IDirect3DVertexBuffer9_Lock(buffer, 0, 0, &ptr, 0);
    ok(SUCCEEDED(hr), "Failed to lock vertex buffer, hr %#x.\n", hr);
    ok(!memcmp(ptr, quad, sizeof(quad)), "Got unexpected vertex buffer data.\n");
    hr = IDirect3DVertexBuffer9_Unlock(buffer);
    ok(SUCCEEDED(hr), "Failed to unlock vertex buffer, hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer9_Lock(buffer, 0, 0, &ptr, D3DLOCK_READONLY);
    ok(SUCCEEDED(hr), "Failed to lock vertex buffer, hr %#x.\n", hr);
    ok(!memcmp(ptr, quad, sizeof(quad)), "Got unexpected vertex buffer data.\n");
    hr = IDirect3DVertexBuffer9_Unlock(buffer);
    ok(SUCCEEDED(hr), "Failed to unlock vertex buffer, hr %#x.\n", hr);

    refcount = IDirect3DVertexBuffer9_Release(buffer);
    ok(!refcount, "Vertex buffer has %u references left.\n", refcount);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_lost_device(void)
{
    struct device_desc device_desc;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    BOOL ret;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    device_desc.adapter_ordinal = D3DADAPTER_DEFAULT;
    device_desc.device_window = window;
    device_desc.width = registry_mode.dmPelsWidth;
    device_desc.height = registry_mode.dmPelsHeight;
    device_desc.flags = CREATE_DEVICE_FULLSCREEN;
    if (!(device = create_device(d3d, window, &device_desc)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    ret = SetForegroundWindow(GetDesktopWindow());
    ok(ret, "Failed to set foreground window.\n");
    hr = IDirect3DDevice9_TestCooperativeLevel(device);
    ok(hr == D3DERR_DEVICELOST, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3DERR_DEVICELOST, "Got unexpected hr %#x.\n", hr);

    ret = ShowWindow(window, SW_RESTORE);
    ok(ret, "Failed to restore window.\n");
    ret = SetForegroundWindow(window);
    ok(ret, "Failed to set foreground window.\n");
    hr = IDirect3DDevice9_TestCooperativeLevel(device);
    ok(hr == D3DERR_DEVICENOTRESET, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3DERR_DEVICELOST, "Got unexpected hr %#x.\n", hr);

    hr = reset_device(device, &device_desc);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    device_desc.flags = 0;
    hr = reset_device(device, &device_desc);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    ret = SetForegroundWindow(GetDesktopWindow());
    ok(ret, "Failed to set foreground window.\n");
    hr = IDirect3DDevice9_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    ret = ShowWindow(window, SW_RESTORE);
    ok(ret, "Failed to restore window.\n");
    ret = SetForegroundWindow(window);
    ok(ret, "Failed to set foreground window.\n");
    hr = IDirect3DDevice9_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    device_desc.flags = CREATE_DEVICE_FULLSCREEN;
    hr = reset_device(device, &device_desc);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    ret = SetForegroundWindow(GetDesktopWindow());
    ok(ret, "Failed to set foreground window.\n");
    hr = reset_device(device, &device_desc);
    ok(hr == D3DERR_DEVICELOST, "Got unexpected hr %#x.\n", hr);
    ret = ShowWindow(window, SW_RESTORE);
    ok(ret, "Failed to restore window.\n");
    ret = SetForegroundWindow(window);
    ok(ret, "Failed to set foreground window.\n");
    hr = reset_device(device, &device_desc);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_resource_priority(void)
{
    IDirect3DDevice9 *device;
    IDirect3DSurface9 *surface;
    IDirect3DTexture9 *texture;
    IDirect3DVertexBuffer9 *buffer;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    static const struct
    {
        D3DPOOL pool;
        const char *name;
        BOOL can_set_priority;
    }
    test_data[] =
    {
        {D3DPOOL_DEFAULT, "D3DPOOL_DEFAULT", FALSE},
        {D3DPOOL_SYSTEMMEM, "D3DPOOL_SYSTEMMEM", FALSE},
        {D3DPOOL_MANAGED, "D3DPOOL_MANAGED", TRUE},
        {D3DPOOL_SCRATCH, "D3DPOOL_SCRATCH", FALSE}
    };
    unsigned int i;
    DWORD priority;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    for (i = 0; i < ARRAY_SIZE(test_data); i++)
    {
        hr = IDirect3DDevice9_CreateTexture(device, 16, 16, 0, 0, D3DFMT_X8R8G8B8,
                test_data[i].pool, &texture, NULL);
        ok(SUCCEEDED(hr), "Failed to create texture, hr %#x, pool %s.\n", hr, test_data[i].name);
        hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
        ok(SUCCEEDED(hr), "Failed to get surface level, hr %#x.\n", hr);

        priority = IDirect3DTexture9_GetPriority(texture);
        ok(priority == 0, "Got unexpected priority %u, pool %s.\n", priority, test_data[i].name);
        priority = IDirect3DTexture9_SetPriority(texture, 1);
        ok(priority == 0, "Got unexpected priority %u, pool %s.\n", priority, test_data[i].name);
        priority = IDirect3DTexture9_GetPriority(texture);
        if (test_data[i].can_set_priority)
        {
            ok(priority == 1, "Got unexpected priority %u, pool %s.\n", priority, test_data[i].name);
            priority = IDirect3DTexture9_SetPriority(texture, 2);
            ok(priority == 1, "Got unexpected priority %u, pool %s.\n", priority, test_data[i].name);
        }
        else
            ok(priority == 0, "Got unexpected priority %u, pool %s.\n", priority, test_data[i].name);

        priority = IDirect3DSurface9_GetPriority(surface);
        ok(priority == 0, "Got unexpected priority %u, pool %s.\n", priority, test_data[i].name);
        priority = IDirect3DSurface9_SetPriority(surface, 1);
        ok(priority == 0, "Got unexpected priority %u, pool %s.\n", priority, test_data[i].name);
        priority = IDirect3DSurface9_GetPriority(surface);
        ok(priority == 0, "Got unexpected priority %u, pool %s.\n", priority, test_data[i].name);

        IDirect3DSurface9_Release(surface);
        IDirect3DTexture9_Release(texture);

        if (test_data[i].pool != D3DPOOL_MANAGED)
        {
            hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 16, 16, D3DFMT_X8R8G8B8,
                    test_data[i].pool, &surface, NULL);
            ok(SUCCEEDED(hr), "Failed to create surface, hr %#x, pool %s.\n", hr, test_data[i].name);

            priority = IDirect3DSurface9_GetPriority(surface);
            ok(priority == 0, "Got unexpected priority %u, pool %s.\n", priority, test_data[i].name);
            priority = IDirect3DSurface9_SetPriority(surface, 1);
            ok(priority == 0, "Got unexpected priority %u, pool %s.\n", priority, test_data[i].name);
            priority = IDirect3DSurface9_GetPriority(surface);
            ok(priority == 0, "Got unexpected priority %u, pool %s.\n", priority, test_data[i].name);

            IDirect3DSurface9_Release(surface);
        }

        if (test_data[i].pool != D3DPOOL_SCRATCH)
        {
            hr = IDirect3DDevice9_CreateVertexBuffer(device, 256, 0, 0,
                    test_data[i].pool, &buffer, NULL);
            ok(SUCCEEDED(hr), "Failed to create buffer, hr %#x, pool %s.\n", hr, test_data[i].name);

            priority = IDirect3DVertexBuffer9_GetPriority(buffer);
            ok(priority == 0, "Got unexpected priority %u, pool %s.\n", priority, test_data[i].name);
            priority = IDirect3DVertexBuffer9_SetPriority(buffer, 1);
            ok(priority == 0, "Got unexpected priority %u, pool %s.\n", priority, test_data[i].name);
            priority = IDirect3DVertexBuffer9_GetPriority(buffer);
            if (test_data[i].can_set_priority)
            {
                ok(priority == 1, "Got unexpected priority %u, pool %s.\n", priority, test_data[i].name);
                priority = IDirect3DVertexBuffer9_SetPriority(buffer, 0);
                ok(priority == 1, "Got unexpected priority %u, pool %s.\n", priority, test_data[i].name);
            }
            else
                ok(priority == 0, "Got unexpected priority %u, pool %s.\n", priority, test_data[i].name);

            IDirect3DVertexBuffer9_Release(buffer);
        }
    }

    hr = IDirect3DDevice9_CreateRenderTarget(device, 16, 16, D3DFMT_X8R8G8B8,
            D3DMULTISAMPLE_NONE, 0, FALSE, &surface, NULL);

    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);
    priority = IDirect3DSurface9_GetPriority(surface);
    ok(priority == 0, "Got unexpected priority %u.\n", priority);
    priority = IDirect3DSurface9_SetPriority(surface, 1);
    ok(priority == 0, "Got unexpected priority %u.\n", priority);
    priority = IDirect3DSurface9_GetPriority(surface);
    ok(priority == 0, "Got unexpected priority %u.\n", priority);

    IDirect3DSurface9_Release(surface);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_swapchain_parameters(void)
{
    IDirect3DDevice9 *device;
    HRESULT hr, expected_hr;
    IDirect3D9 *d3d;
    D3DCAPS9 caps;
    HWND window;
    unsigned int i;
    D3DPRESENT_PARAMETERS present_parameters, present_parameters_windowed = {0}, present_parameters2;
    IDirect3DSwapChain9 *swapchain;
    static const struct
    {
        BOOL windowed;
        UINT backbuffer_count;
        D3DSWAPEFFECT swap_effect;
        HRESULT hr;
    }
    tests[] =
    {
        /* Swap effect 0 is not allowed. */
        {TRUE,  1, 0,                        D3DERR_INVALIDCALL},
        {FALSE, 1, 0,                        D3DERR_INVALIDCALL},

        /* All (non-ex) swap effects are allowed in
         * windowed and fullscreen mode. */
        {TRUE,  1, D3DSWAPEFFECT_DISCARD,    D3D_OK},
        {TRUE,  1, D3DSWAPEFFECT_FLIP,       D3D_OK},
        {FALSE, 1, D3DSWAPEFFECT_DISCARD,    D3D_OK},
        {FALSE, 1, D3DSWAPEFFECT_FLIP,       D3D_OK},
        {FALSE, 1, D3DSWAPEFFECT_COPY,       D3D_OK},

        /* Only one backbuffer in copy mode. */
        {TRUE,  0, D3DSWAPEFFECT_COPY,       D3D_OK},
        {TRUE,  1, D3DSWAPEFFECT_COPY,       D3D_OK},
        {TRUE,  2, D3DSWAPEFFECT_COPY,       D3DERR_INVALIDCALL},
        {FALSE, 2, D3DSWAPEFFECT_COPY,       D3DERR_INVALIDCALL},

        /* Ok with the others, in fullscreen and windowed mode. */
        {TRUE,  2, D3DSWAPEFFECT_DISCARD,    D3D_OK},
        {TRUE,  2, D3DSWAPEFFECT_FLIP,       D3D_OK},
        {FALSE, 2, D3DSWAPEFFECT_DISCARD,    D3D_OK},
        {FALSE, 2, D3DSWAPEFFECT_FLIP,       D3D_OK},

        /* D3D9Ex swap effects. */
        {TRUE,  1, D3DSWAPEFFECT_OVERLAY,    D3DERR_INVALIDCALL},
        {TRUE,  1, D3DSWAPEFFECT_FLIPEX,     D3DERR_INVALIDCALL},
        {TRUE,  1, D3DSWAPEFFECT_FLIPEX + 1, D3DERR_INVALIDCALL},
        {FALSE, 1, D3DSWAPEFFECT_OVERLAY,    D3DERR_INVALIDCALL},
        {FALSE, 1, D3DSWAPEFFECT_FLIPEX,     D3DERR_INVALIDCALL},
        {FALSE, 1, D3DSWAPEFFECT_FLIPEX + 1, D3DERR_INVALIDCALL},

        /* 3 is the highest allowed backbuffer count. */
        {TRUE,  3, D3DSWAPEFFECT_DISCARD,    D3D_OK},
        {TRUE,  4, D3DSWAPEFFECT_DISCARD,    D3DERR_INVALIDCALL},
        {TRUE,  4, D3DSWAPEFFECT_FLIP,       D3DERR_INVALIDCALL},
        {FALSE, 4, D3DSWAPEFFECT_DISCARD,    D3DERR_INVALIDCALL},
        {FALSE, 4, D3DSWAPEFFECT_FLIP,       D3DERR_INVALIDCALL},
    };

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }
    memset(&caps, 0, sizeof(caps));
    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(hr == D3D_OK, "Failed to get device caps, hr %#x.\n", hr);
    IDirect3DDevice9_Release(device);

    present_parameters_windowed.BackBufferWidth = registry_mode.dmPelsWidth;
    present_parameters_windowed.BackBufferHeight = registry_mode.dmPelsHeight;
    present_parameters_windowed.hDeviceWindow = window;
    present_parameters_windowed.BackBufferFormat = D3DFMT_X8R8G8B8;
    present_parameters_windowed.SwapEffect = D3DSWAPEFFECT_COPY;
    present_parameters_windowed.Windowed = TRUE;
    present_parameters_windowed.BackBufferCount = 1;

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        memset(&present_parameters, 0, sizeof(present_parameters));
        present_parameters.BackBufferWidth = registry_mode.dmPelsWidth;
        present_parameters.BackBufferHeight = registry_mode.dmPelsHeight;
        present_parameters.hDeviceWindow = window;
        present_parameters.BackBufferFormat = D3DFMT_X8R8G8B8;

        present_parameters.SwapEffect = tests[i].swap_effect;
        present_parameters.Windowed = tests[i].windowed;
        present_parameters.BackBufferCount = tests[i].backbuffer_count;

        hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
                D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device);
        ok(hr == tests[i].hr, "Expected hr %x, got %x, test %u.\n", tests[i].hr, hr, i);
        if (SUCCEEDED(hr))
        {
            UINT bb_count = tests[i].backbuffer_count ? tests[i].backbuffer_count : 1;

            hr = IDirect3DDevice9_GetSwapChain(device, 0, &swapchain);
            ok(SUCCEEDED(hr), "Failed to get swapchain, hr %#x, test %u.\n", hr, i);

            hr = IDirect3DSwapChain9_GetPresentParameters(swapchain, &present_parameters2);
            ok(SUCCEEDED(hr), "Failed to get present parameters, hr %#x, test %u.\n", hr, i);
            ok(present_parameters2.SwapEffect == tests[i].swap_effect, "Swap effect changed from %u to %u, test %u.\n",
                    tests[i].swap_effect, present_parameters2.SwapEffect, i);
            ok(present_parameters2.BackBufferCount == bb_count, "Backbuffer count changed from %u to %u, test %u.\n",
                    bb_count, present_parameters2.BackBufferCount, i);
            ok(present_parameters2.Windowed == tests[i].windowed, "Windowed changed from %u to %u, test %u.\n",
                    tests[i].windowed, present_parameters2.Windowed, i);

            IDirect3DSwapChain9_Release(swapchain);
            IDirect3DDevice9_Release(device);
        }

        hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
                D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters_windowed, &device);
        ok(SUCCEEDED(hr), "Failed to create device, hr %#x, test %u.\n", hr, i);

        memset(&present_parameters, 0, sizeof(present_parameters));
        present_parameters.BackBufferWidth = registry_mode.dmPelsWidth;
        present_parameters.BackBufferHeight = registry_mode.dmPelsHeight;
        present_parameters.hDeviceWindow = window;
        present_parameters.BackBufferFormat = D3DFMT_X8R8G8B8;

        present_parameters.SwapEffect = tests[i].swap_effect;
        present_parameters.Windowed = tests[i].windowed;
        present_parameters.BackBufferCount = tests[i].backbuffer_count;

        hr = IDirect3DDevice9_Reset(device, &present_parameters);
        ok(hr == tests[i].hr, "Expected hr %x, got %x, test %u.\n", tests[i].hr, hr, i);

        if (FAILED(hr))
        {
            hr = IDirect3DDevice9_Reset(device, &present_parameters_windowed);
            ok(SUCCEEDED(hr), "Failed to reset device, hr %#x, test %u.\n", hr, i);
        }
        IDirect3DDevice9_Release(device);
    }

    for (i = 0; i < 10; ++i)
    {
        memset(&present_parameters, 0, sizeof(present_parameters));
        present_parameters.BackBufferWidth = registry_mode.dmPelsWidth;
        present_parameters.BackBufferHeight = registry_mode.dmPelsHeight;
        present_parameters.hDeviceWindow = window;
        present_parameters.BackBufferFormat = D3DFMT_X8R8G8B8;
        present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
        present_parameters.Windowed = FALSE;
        present_parameters.BackBufferCount = 2;

        present_parameters.PresentationInterval = i;
        switch (present_parameters.PresentationInterval)
        {
            case D3DPRESENT_INTERVAL_ONE:
            case D3DPRESENT_INTERVAL_TWO:
            case D3DPRESENT_INTERVAL_THREE:
            case D3DPRESENT_INTERVAL_FOUR:
                if (!(caps.PresentationIntervals & present_parameters.PresentationInterval))
                    continue;
                /* Fall through */
            case D3DPRESENT_INTERVAL_DEFAULT:
                expected_hr = D3D_OK;
                break;
            default:
                expected_hr = D3DERR_INVALIDCALL;
                break;
        }

        hr = IDirect3D9_CreateDevice(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
                D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device);
        ok(hr == expected_hr, "Got unexpected hr %#x, test %u.\n", hr, i);
        if (FAILED(hr))
            continue;

        hr = IDirect3DDevice9_GetSwapChain(device, 0, &swapchain);
        ok(SUCCEEDED(hr), "Failed to get swapchain, hr %#x, test %u.\n", hr, i);

        hr = IDirect3DSwapChain9_GetPresentParameters(swapchain, &present_parameters2);
        ok(SUCCEEDED(hr), "Failed to get present parameters, hr %#x, test %u.\n", hr, i);
        ok(present_parameters2.PresentationInterval == i,
                "Got presentation interval %#x, expected %#x.\n",
                present_parameters2.PresentationInterval, i);

        IDirect3DSwapChain9_Release(swapchain);
        IDirect3DDevice9_Release(device);
    }

    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_check_device_format(void)
{
    static const D3DFORMAT mipmap_autogen_formats[] =
    {
        D3DFMT_R8G8B8,
        D3DFMT_A8R8G8B8,
        D3DFMT_X8R8G8B8,
        D3DFMT_R5G6B5,
        D3DFMT_X1R5G5B5,
        D3DFMT_A8P8,
        D3DFMT_P8,
        D3DFMT_A1R5G5B5,
        D3DFMT_A4R4G4B4,
    };

    BOOL render_target_supported;
    D3DDEVTYPE device_type;
    D3DFORMAT format;
    IDirect3D9 *d3d;
    unsigned int i;
    HRESULT hr;

    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    if (IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, D3DUSAGE_QUERY_SRGBWRITE, D3DRTYPE_TEXTURE, D3DFMT_A8R8G8B8) != D3D_OK)
    {
        skip("D3DFMT_A8R8G8B8 textures with SRGBWRITE not supported.\n");
    }
    else
    {
        hr = IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL,
                D3DFMT_X8R8G8B8, D3DUSAGE_RENDERTARGET | D3DUSAGE_QUERY_SRGBWRITE, D3DRTYPE_SURFACE, D3DFMT_A8R8G8B8);
        ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
        hr = IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL,
                D3DFMT_X8R8G8B8, D3DUSAGE_QUERY_SRGBWRITE, D3DRTYPE_SURFACE, D3DFMT_A8R8G8B8);
        ok(FAILED(hr), "Got unexpected hr %#x.\n", hr);
    }

    for (device_type = D3DDEVTYPE_HAL; device_type <  D3DDEVTYPE_NULLREF; ++device_type)
    {
        hr = IDirect3D9_CheckDeviceFormat(d3d, 0, device_type, D3DFMT_UNKNOWN,
                0, D3DRTYPE_SURFACE, D3DFMT_A8R8G8B8);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x, device type %#x.\n", hr, device_type);
        hr = IDirect3D9_CheckDeviceFormat(d3d, 0, device_type, D3DFMT_UNKNOWN,
                0, D3DRTYPE_TEXTURE, D3DFMT_A8R8G8B8);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x, device type %#x.\n", hr, device_type);
        hr = IDirect3D9_CheckDeviceFormat(d3d, 0, device_type, D3DFMT_UNKNOWN,
                0, D3DRTYPE_SURFACE, D3DFMT_X8R8G8B8);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x, device type %#x.\n", hr, device_type);
        hr = IDirect3D9_CheckDeviceFormat(d3d, 0, device_type, D3DFMT_UNKNOWN,
                0, D3DRTYPE_TEXTURE, D3DFMT_X8R8G8B8);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x, device type %#x.\n", hr, device_type);
    }

    hr = IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            0, D3DRTYPE_VERTEXBUFFER, D3DFMT_VERTEXDATA);
    todo_wine ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            0, D3DRTYPE_INDEXBUFFER, D3DFMT_VERTEXDATA);
    todo_wine ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            0, D3DRTYPE_INDEXBUFFER, D3DFMT_INDEX16);
    todo_wine ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            D3DUSAGE_SOFTWAREPROCESSING, D3DRTYPE_VERTEXBUFFER, D3DFMT_VERTEXDATA);
    todo_wine ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            D3DUSAGE_SOFTWAREPROCESSING, D3DRTYPE_INDEXBUFFER, D3DFMT_VERTEXDATA);
    todo_wine ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            D3DUSAGE_SOFTWAREPROCESSING, D3DRTYPE_INDEXBUFFER, D3DFMT_INDEX16);
    todo_wine ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_A8R8G8B8,
            0, D3DRTYPE_TEXTURE, D3DFMT_X8R8G8B8);
    ok(hr == D3DERR_NOTAVAILABLE, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
            0, D3DRTYPE_TEXTURE, D3DFMT_X8R8G8B8);
    ok(hr == D3D_OK || broken(hr == D3DERR_NOTAVAILABLE) /* Testbot Windows <= 7 */,
            "Got unexpected hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(mipmap_autogen_formats); ++i)
    {
        format = mipmap_autogen_formats[i];

        hr = IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, format);
        ok(hr == D3D_OK || hr == D3DERR_NOTAVAILABLE, "Got unexpected hr %#x.\n", hr);
        render_target_supported = hr == D3D_OK;

        hr = IDirect3D9_CheckDeviceFormat(d3d, 0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
                D3DUSAGE_RENDERTARGET | D3DUSAGE_AUTOGENMIPMAP, D3DRTYPE_TEXTURE, format);
        if (render_target_supported)
        {
            ok(hr == D3D_OK || hr == D3DOK_NOAUTOGEN, "Got unexpected hr %#x.\n", hr);
        }
        else
        {
            ok(hr == D3DERR_NOTAVAILABLE, "Got unexpected hr %#x.\n", hr);
        }
    }

    hr = IDirect3D9_CheckDepthStencilMatch(d3d, D3DADAPTER_DEFAULT,
            D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, D3DFMT_A8R8G8B8, D3DFMT_D32);
    ok(hr == D3DERR_NOTAVAILABLE || broken(hr == D3DERR_INVALIDCALL /* Windows 10 */),
            "Got unexpected hr %#x.\n", hr);

    hr = IDirect3D9_CheckDepthStencilMatch(d3d, D3DADAPTER_DEFAULT,
            D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, D3DFMT_R5G6B5, D3DFMT_D32);
    ok(hr == D3DERR_NOTAVAILABLE || broken(hr == D3DERR_INVALIDCALL /* Windows 10 */),
            "Got unexpected hr %#x.\n", hr);

    IDirect3D9_Release(d3d);
}

static void test_miptree_layout(void)
{
    unsigned int pool_idx, format_idx, base_dimension, level_count, offset, i, j;
    IDirect3DCubeTexture9 *texture_cube;
    IDirect3DTexture9 *texture_2d;
    IDirect3DDevice9 *device;
    D3DLOCKED_RECT map_desc;
    BYTE *base = NULL;
    IDirect3D9 *d3d;
    D3DCAPS9 caps;
    UINT refcount;
    HWND window;
    HRESULT hr;

    static const struct
    {
        D3DFORMAT format;
        const char *name;
    }
    formats[] =
    {
        {D3DFMT_A8R8G8B8,             "D3DFMT_A8R8G8B8"},
        {D3DFMT_A8,                   "D3DFMT_A8"},
        {D3DFMT_L8,                   "D3DFMT_L8"},
        {MAKEFOURCC('A','T','I','1'), "D3DFMT_ATI1"},
        {MAKEFOURCC('A','T','I','2'), "D3DFMT_ATI2"},
    };
    static const struct
    {
        D3DPOOL pool;
        const char *name;
    }
    pools[] =
    {
        {D3DPOOL_MANAGED,   "D3DPOOL_MANAGED"},
        {D3DPOOL_SYSTEMMEM, "D3DPOOL_SYSTEMMEM"},
        {D3DPOOL_SCRATCH,   "D3DPOOL_SCRATCH"},
    };

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#x.\n", hr);

    base_dimension = 257;
    if (caps.TextureCaps & (D3DPTEXTURECAPS_POW2 | D3DPTEXTURECAPS_CUBEMAP_POW2))
    {
        skip("Using power of two base dimension.\n");
        base_dimension = 256;
    }

    for (format_idx = 0; format_idx < ARRAY_SIZE(formats); ++format_idx)
    {
        if (FAILED(hr = IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                D3DFMT_X8R8G8B8, 0, D3DRTYPE_TEXTURE, formats[format_idx].format)))
        {
            skip("%s textures not supported, skipping tests.\n", formats[format_idx].name);
            continue;
        }

        for (pool_idx = 0; pool_idx < ARRAY_SIZE(pools); ++pool_idx)
        {
            hr = IDirect3DDevice9_CreateTexture(device, base_dimension, base_dimension, 0, 0,
                    formats[format_idx].format, pools[pool_idx].pool, &texture_2d, NULL);
            ok(SUCCEEDED(hr), "Failed to create a %s %s texture, hr %#x.\n",
                    pools[pool_idx].name, formats[format_idx].name, hr);

            level_count = IDirect3DTexture9_GetLevelCount(texture_2d);
            for (i = 0, offset = 0; i < level_count; ++i)
            {
                hr = IDirect3DTexture9_LockRect(texture_2d, i, &map_desc, NULL, 0);
                ok(SUCCEEDED(hr), "%s, %s: Failed to lock level %u, hr %#x.\n",
                        pools[pool_idx].name, formats[format_idx].name, i, hr);

                if (!i)
                    base = map_desc.pBits;
                else
                    ok(map_desc.pBits == base + offset,
                            "%s, %s, level %u: Got unexpected pBits %p, expected %p.\n",
                            pools[pool_idx].name, formats[format_idx].name, i, map_desc.pBits, base + offset);
                offset += (base_dimension >> i) * map_desc.Pitch;

                hr = IDirect3DTexture9_UnlockRect(texture_2d, i);
                ok(SUCCEEDED(hr), "%s, %s Failed to unlock level %u, hr %#x.\n",
                        pools[pool_idx].name, formats[format_idx].name, i, hr);
            }

            IDirect3DTexture9_Release(texture_2d);
        }

        if (FAILED(hr = IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                D3DFMT_X8R8G8B8, 0, D3DRTYPE_CUBETEXTURE, formats[format_idx].format)))
        {
            skip("%s cube textures not supported, skipping tests.\n", formats[format_idx].name);
            continue;
        }

        for (pool_idx = 0; pool_idx < ARRAY_SIZE(pools); ++pool_idx)
        {
            hr = IDirect3DDevice9_CreateCubeTexture(device, base_dimension, 0, 0,
                    formats[format_idx].format, pools[pool_idx].pool, &texture_cube, NULL);
            ok(SUCCEEDED(hr), "Failed to create a %s %s cube texture, hr %#x.\n",
                    pools[pool_idx].name, formats[format_idx].name, hr);

            level_count = IDirect3DCubeTexture9_GetLevelCount(texture_cube);
            for (i = 0, offset = 0; i < 6; ++i)
            {
                for (j = 0; j < level_count; ++j)
                {
                    hr = IDirect3DCubeTexture9_LockRect(texture_cube, i, j, &map_desc, NULL, 0);
                    ok(SUCCEEDED(hr), "%s, %s: Failed to lock face %u, level %u, hr %#x.\n",
                            pools[pool_idx].name, formats[format_idx].name, i, j, hr);

                    if (!i && !j)
                        base = map_desc.pBits;
                    else
                        ok(map_desc.pBits == base + offset,
                                "%s, %s, face %u, level %u: Got unexpected pBits %p, expected %p.\n",
                                pools[pool_idx].name, formats[format_idx].name, i, j, map_desc.pBits, base + offset);
                    offset += (base_dimension >> j) * map_desc.Pitch;

                    hr = IDirect3DCubeTexture9_UnlockRect(texture_cube, i, j);
                    ok(SUCCEEDED(hr), "%s, %s: Failed to unlock face %u, level %u, hr %#x.\n",
                            pools[pool_idx].name, formats[format_idx].name, i, j, hr);
                }
                offset = (offset + 15) & ~15;
            }

            IDirect3DCubeTexture9_Release(texture_cube);
        }
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_get_render_target_data(void)
{
    IDirect3DSurface9 *offscreen_surface, *render_target;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    UINT refcount;
    HWND window;
    HRESULT hr;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 32, 32,
            D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &offscreen_surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create offscreen surface, hr %#x.\n", hr);

    hr = IDirect3DDevice9_CreateRenderTarget(device, 32, 32,
            D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &render_target, NULL);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetRenderTargetData(device, NULL, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetRenderTargetData(device, render_target, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetRenderTargetData(device, NULL, offscreen_surface);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetRenderTargetData(device, render_target, offscreen_surface);
    ok(SUCCEEDED(hr), "Got unexpected hr %#x.\n", hr);

    IDirect3DSurface9_Release(render_target);
    IDirect3DSurface9_Release(offscreen_surface);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_render_target_device_mismatch(void)
{
    IDirect3DDevice9 *device, *device2;
    IDirect3DSurface9 *surface, *rt;
    IDirect3D9 *d3d;
    UINT refcount;
    HWND window;
    HRESULT hr;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &rt);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    device2 = create_device(d3d, window, NULL);
    ok(!!device2, "Failed to create a D3D device.\n");

    hr = IDirect3DDevice9_CreateRenderTarget(device2, 640, 480,
            D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, FALSE, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create render target, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, surface);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    IDirect3DSurface9_Release(surface);

    hr = IDirect3DDevice9_GetRenderTarget(device2, 0, &surface);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, surface);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    IDirect3DSurface9_Release(surface);

    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &surface);
    ok(SUCCEEDED(hr), "Failed to get render target, hr %#x.\n", hr);
    ok(surface == rt, "Got unexpected render target %p, expected %p.\n", surface, rt);
    IDirect3DSurface9_Release(surface);
    IDirect3DSurface9_Release(rt);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    refcount = IDirect3DDevice9_Release(device2);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_format_unknown(void)
{
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    UINT refcount;
    HWND window;
    void *iface;
    HRESULT hr;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    iface = (void *)0xdeadbeef;
    hr = IDirect3DDevice9_CreateRenderTarget(device, 64, 64,
            D3DFMT_UNKNOWN, D3DMULTISAMPLE_NONE, 0, FALSE, (IDirect3DSurface9 **)&iface, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    ok(!iface, "Got unexpected iface %p.\n", iface);

    iface = (void *)0xdeadbeef;
    hr = IDirect3DDevice9_CreateDepthStencilSurface(device, 64, 64,
            D3DFMT_UNKNOWN, D3DMULTISAMPLE_NONE, 0, TRUE, (IDirect3DSurface9 **)&iface, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    ok(!iface, "Got unexpected iface %p.\n", iface);

    iface = (void *)0xdeadbeef;
    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 64, 64,
            D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, (IDirect3DSurface9 **)&iface, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    ok(!iface, "Got unexpected iface %p.\n", iface);

    iface = (void *)0xdeadbeef;
    hr = IDirect3DDevice9_CreateTexture(device, 64, 64, 1, 0,
            D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, (IDirect3DTexture9 **)&iface, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    ok(!iface, "Got unexpected iface %p.\n", iface);

    iface = (void *)0xdeadbeef;
    hr = IDirect3DDevice9_CreateCubeTexture(device, 64, 1, 0,
            D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, (IDirect3DCubeTexture9 **)&iface, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    ok(!iface, "Got unexpected iface %p.\n", iface);

    iface = (void *)0xdeadbeef;
    hr = IDirect3DDevice9_CreateVolumeTexture(device, 64, 64, 1, 1, 0,
            D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, (IDirect3DVolumeTexture9 **)&iface, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);
    ok(!iface, "Got unexpected iface %p.\n", iface);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_destroyed_window(void)
{
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d9;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    /* No WS_VISIBLE. */
    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    ok(!!window, "Failed to create a window.\n");

    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d9, "Failed to create a D3D object.\n");
    device = create_device(d3d9, window, NULL);
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
    if (!device)
    {
        skip("Failed to create a 3D device, skipping test.\n");
        return;
    }

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#x.\n", hr);
    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x00000000, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#x.\n", hr);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#x.\n", hr);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
}

static void test_lockable_backbuffer(void)
{
    D3DPRESENT_PARAMETERS present_parameters = {0};
    struct device_desc device_desc;
    IDirect3DSurface9 *surface;
    IDirect3DDevice9 *device;
    D3DLOCKED_RECT lockrect;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    HDC dc;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &surface);
    ok(SUCCEEDED(hr), "Failed to get backbuffer, hr %#x.\n", hr);

    hr = IDirect3DSurface9_LockRect(surface, &lockrect, NULL, D3DLOCK_DISCARD);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    dc = (void *)0xdeadbeef;
    hr = IDirect3DSurface9_GetDC(surface, &dc);
    ok(dc == (void *)0xdeadbeef, "Unexpected DC returned.\n");
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#x.\n", hr);

    IDirect3DSurface9_Release(surface);

    /* Reset with D3DPRESENTFLAG_LOCKABLE_BACKBUFFER. */
    present_parameters.BackBufferWidth = 640;
    present_parameters.BackBufferHeight = 480;
    present_parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.hDeviceWindow = window;
    present_parameters.Windowed = TRUE;
    present_parameters.EnableAutoDepthStencil = TRUE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;
    present_parameters.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

    hr = IDirect3DDevice9_Reset(device, &present_parameters);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &surface);
    ok(SUCCEEDED(hr), "Failed to get backbuffer, hr %#x.\n", hr);

    hr = IDirect3DSurface9_LockRect(surface, &lockrect, NULL, D3DLOCK_DISCARD);
    ok(SUCCEEDED(hr), "Failed to lock rect, hr %#x.\n", hr);
    hr = IDirect3DSurface9_UnlockRect(surface);
    ok(SUCCEEDED(hr), "Failed to unlock rect, hr %#x.\n", hr);

    IDirect3DSurface9_Release(surface);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);

    device_desc.adapter_ordinal = D3DADAPTER_DEFAULT;
    device_desc.width = 640;
    device_desc.height = 480;
    device_desc.device_window = window;
    device_desc.flags = CREATE_DEVICE_LOCKABLE_BACKBUFFER;

    device = create_device(d3d, window, &device_desc);
    ok(!!device, "Failed to create device.\n");

    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &surface);
    ok(SUCCEEDED(hr), "Failed to get backbuffer, hr %#x.\n", hr);

    hr = IDirect3DSurface9_LockRect(surface, &lockrect, NULL, D3DLOCK_DISCARD);
    ok(SUCCEEDED(hr), "Failed to lock rect, hr %#x.\n", hr);
    hr = IDirect3DSurface9_UnlockRect(surface);
    ok(SUCCEEDED(hr), "Failed to unlock rect, hr %#x.\n", hr);

    IDirect3DSurface9_Release(surface);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_clip_planes_limits(void)
{
    static const DWORD device_flags[] = {0, CREATE_DEVICE_SWVP_ONLY};
    IDirect3DDevice9 *device;
    struct device_desc desc;
    unsigned int i, j;
    IDirect3D9 *d3d;
    ULONG refcount;
    float plane[4];
    D3DCAPS9 caps;
    DWORD state;
    HWND window;
    HRESULT hr;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    for (i = 0; i < ARRAY_SIZE(device_flags); ++i)
    {
        desc.adapter_ordinal = D3DADAPTER_DEFAULT;
        desc.device_window = window;
        desc.width = 640;
        desc.height = 480;
        desc.flags = device_flags[i];
        if (!(device = create_device(d3d, window, &desc)))
        {
            skip("Failed to create D3D device, flags %#x.\n", desc.flags);
            continue;
        }

        memset(&caps, 0, sizeof(caps));
        hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
        ok(hr == D3D_OK, "Failed to get caps, hr %#x.\n", hr);

        trace("Max user clip planes: %u.\n", caps.MaxUserClipPlanes);

        for (j = 0; j < 2 * D3DMAXUSERCLIPPLANES; ++j)
        {
            memset(plane, 0xff, sizeof(plane));
            hr = IDirect3DDevice9_GetClipPlane(device, j, plane);
            ok(hr == D3D_OK, "Failed to get clip plane %u, hr %#x.\n", j, hr);
            ok(!plane[0] && !plane[1] && !plane[2] && !plane[3],
                    "Got unexpected plane %u: %.8e, %.8e, %.8e, %.8e.\n",
                    j, plane[0], plane[1], plane[2], plane[3]);
        }

        plane[0] = 2.0f;
        plane[1] = 8.0f;
        plane[2] = 5.0f;
        for (j = 0; j < 2 * D3DMAXUSERCLIPPLANES; ++j)
        {
            plane[3] = j;
            hr = IDirect3DDevice9_SetClipPlane(device, j, plane);
            ok(hr == D3D_OK, "Failed to set clip plane %u, hr %#x.\n", j, hr);
        }
        for (j = 0; j < 2 * D3DMAXUSERCLIPPLANES; ++j)
        {
            float expected_d = j >= caps.MaxUserClipPlanes - 1 ? 2 * D3DMAXUSERCLIPPLANES - 1 : j;
            memset(plane, 0xff, sizeof(plane));
            hr = IDirect3DDevice9_GetClipPlane(device, j, plane);
            ok(hr == D3D_OK, "Failed to get clip plane %u, hr %#x.\n", j, hr);
            ok(plane[0] == 2.0f && plane[1] == 8.0f && plane[2] == 5.0f && plane[3] == expected_d,
                    "Got unexpected plane %u: %.8e, %.8e, %.8e, %.8e.\n",
                    j, plane[0], plane[1], plane[2], plane[3]);
        }

        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_CLIPPLANEENABLE, 0xffffffff);
        ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
        hr = IDirect3DDevice9_GetRenderState(device, D3DRS_CLIPPLANEENABLE, &state);
        ok(SUCCEEDED(hr), "Failed to get render state, hr %#x.\n", hr);
        ok(state == 0xffffffff, "Got unexpected state %#x.\n", state);
        hr = IDirect3DDevice9_SetRenderState(device, D3DRS_CLIPPLANEENABLE, 0x80000000);
        ok(SUCCEEDED(hr), "Failed to set render state, hr %#x.\n", hr);
        hr = IDirect3DDevice9_GetRenderState(device, D3DRS_CLIPPLANEENABLE, &state);
        ok(SUCCEEDED(hr), "Failed to get render state, hr %#x.\n", hr);
        ok(state == 0x80000000, "Got unexpected state %#x.\n", state);

        refcount = IDirect3DDevice9_Release(device);
        ok(!refcount, "Device has %u references left.\n", refcount);
    }

    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_swapchain_multisample_reset(void)
{
    IDirect3DSwapChain9 *swapchain;
    D3DPRESENT_PARAMETERS d3dpp;
    IDirect3DDevice9 *device;
    DWORD quality_levels;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    window = create_window();
    ok(!!window, "Failed to create a window.\n");
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    if (IDirect3D9_CheckDeviceMultiSampleType(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_A8R8G8B8, TRUE, D3DMULTISAMPLE_2_SAMPLES, &quality_levels) == D3DERR_NOTAVAILABLE)
    {
        skip("Multisampling not supported for D3DFMT_A8R8G8B8.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a 3D device.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0);
    ok(hr == D3D_OK, "Failed to clear, hr %#x.\n", hr);

    hr = IDirect3DDevice9_GetSwapChain(device, 0, &swapchain);
    ok(hr == D3D_OK, "Failed to get the implicit swapchain, hr %#x.\n", hr);
    hr = IDirect3DSwapChain9_GetPresentParameters(swapchain, &d3dpp);
    ok(hr == D3D_OK, "Failed to get present parameters, hr %#x.\n", hr);
    ok(d3dpp.MultiSampleType == D3DMULTISAMPLE_NONE,
            "Got unexpected multisample type %#x.\n", d3dpp.MultiSampleType);
    IDirect3DSwapChain9_Release(swapchain);

    d3dpp.MultiSampleType = D3DMULTISAMPLE_2_SAMPLES;
    d3dpp.MultiSampleQuality = quality_levels - 1;
    hr = IDirect3DDevice9_Reset(device, &d3dpp);
    ok(hr == D3D_OK, "Failed to reset device, hr %#x.\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0);
    ok(hr == D3D_OK, "Failed to clear, hr %#x.\n", hr);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_stretch_rect(void)
{
    IDirect3DTexture9 *src_texture, *dst_texture;
    IDirect3DSurface9 *src_surface, *dst_surface;
    IDirect3DSurface9 *src_rt, *dst_rt;
    D3DFORMAT src_format, dst_format;
    IDirect3DSurface9 *src, *dst;
    D3DPOOL src_pool, dst_pool;
    BOOL can_stretch_textures;
    IDirect3DDevice9 *device;
    HRESULT expected_hr;
    unsigned int i, j;
    IDirect3D9 *d3d;
    ULONG refcount;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;

    static const D3DFORMAT formats[] =
    {
        D3DFMT_A8R8G8B8,
        D3DFMT_X8R8G8B8,
        D3DFMT_R5G6B5,
    };

    window = create_window();
    ok(!!window, "Failed to create a window.\n");
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a 3D device.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    memset(&caps, 0, sizeof(caps));
    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(hr == D3D_OK, "Failed to get caps, hr %#x.\n", hr);
    can_stretch_textures = caps.DevCaps2 & D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES;

    for (i = 0; i < ARRAY_SIZE(formats); ++i)
    {
        src_format = formats[i];
        if (FAILED(IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                D3DFMT_X8R8G8B8, D3DUSAGE_RENDERTARGET, D3DRTYPE_SURFACE, src_format))
                || FAILED(IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                D3DFMT_X8R8G8B8, 0, D3DRTYPE_TEXTURE, src_format)))
        {
            skip("Format %#x not supported.\n", src_format);
            continue;
        }

        for (j = 0; j < ARRAY_SIZE(formats); ++j)
        {
            dst_format = formats[j];
            if (FAILED(IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                    D3DFMT_X8R8G8B8, D3DUSAGE_RENDERTARGET, D3DRTYPE_SURFACE, dst_format))
                    || FAILED(IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                    D3DFMT_X8R8G8B8, 0, D3DRTYPE_TEXTURE, dst_format)))
            {
                skip("Format %#x not supported.\n", dst_format);
                continue;
            }

            hr = IDirect3DDevice9_CreateRenderTarget(device, 32, 32, src_format,
                    D3DMULTISAMPLE_NONE, 0, FALSE, &src_rt, NULL);
            ok(hr == D3D_OK, "Failed to create render target, hr %#x.\n", hr);
            hr = IDirect3DDevice9_CreateRenderTarget(device, 32, 32, dst_format,
                    D3DMULTISAMPLE_NONE, 0, FALSE, &dst_rt, NULL);
            ok(hr == D3D_OK, "Failed to create render target, hr %#x.\n", hr);

            hr = IDirect3DDevice9_StretchRect(device, src_rt, NULL, dst_rt, NULL, D3DTEXF_NONE);
            ok(hr == D3D_OK, "Got hr %#x (formats %#x/%#x).\n", hr, src_format, dst_format);

            for (src_pool = D3DPOOL_DEFAULT; src_pool <= D3DPOOL_SCRATCH; ++src_pool)
            {
                for (dst_pool = D3DPOOL_DEFAULT; dst_pool <= D3DPOOL_SCRATCH; ++dst_pool)
                {
                    hr = IDirect3DDevice9_CreateTexture(device, 32, 32, 1, 0,
                            src_format, src_pool, &src_texture, NULL);
                    ok(hr == D3D_OK, "Failed to create texture, hr %#x.\n", hr);
                    hr = IDirect3DDevice9_CreateTexture(device, 32, 32, 1, 0,
                            dst_format, dst_pool, &dst_texture, NULL);
                    ok(hr == D3D_OK, "Failed to create texture, hr %#x.\n", hr);
                    hr = IDirect3DTexture9_GetSurfaceLevel(src_texture, 0, &src);
                    ok(hr == D3D_OK, "Failed to get surface, hr %#x.\n", hr);
                    hr = IDirect3DTexture9_GetSurfaceLevel(dst_texture, 0, &dst);
                    ok(hr == D3D_OK, "Failed to get surface, hr %#x.\n", hr);
                    IDirect3DTexture9_Release(src_texture);
                    IDirect3DTexture9_Release(dst_texture);

                    hr = IDirect3DDevice9_StretchRect(device, src, NULL, dst, NULL, D3DTEXF_NONE);
                    ok(hr == D3DERR_INVALIDCALL, "Got hr %#x (formats %#x/%#x, pools %#x/%#x).\n",
                            hr, src_format, dst_format, src_pool, dst_pool);

                    /* render target <-> texture */
                    if (src_pool == D3DPOOL_DEFAULT && can_stretch_textures)
                        expected_hr = D3D_OK;
                    else
                        expected_hr = D3DERR_INVALIDCALL;
                    hr = IDirect3DDevice9_StretchRect(device, src, NULL, dst_rt, NULL, D3DTEXF_NONE);
                    ok(hr == expected_hr, "Got hr %#x, expected hr %#x (formats %#x/%#x, pool %#x).\n",
                            hr, expected_hr, src_format, dst_format, src_pool);
                    hr = IDirect3DDevice9_StretchRect(device, src_rt, NULL, dst, NULL, D3DTEXF_NONE);
                    ok(hr == D3DERR_INVALIDCALL, "Got hr %#x (formats %#x/%#x, pool %#x).\n",
                            hr, src_format, dst_format, dst_pool);

                    if (src_pool == D3DPOOL_MANAGED || dst_pool == D3DPOOL_MANAGED)
                    {
                        IDirect3DSurface9_Release(src);
                        IDirect3DSurface9_Release(dst);
                        continue;
                    }

                    if (src_pool == D3DPOOL_DEFAULT && dst_pool == D3DPOOL_DEFAULT)
                        expected_hr = D3D_OK;
                    else
                        expected_hr = D3DERR_INVALIDCALL;

                    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 32, 32,
                            src_format, src_pool, &src_surface, NULL);
                    ok(hr == D3D_OK, "Failed to create surface, hr %#x.\n", hr);
                    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device, 32, 32,
                            dst_format, dst_pool, &dst_surface, NULL);
                    ok(hr == D3D_OK, "Failed to create surface, hr %#x.\n", hr);

                    hr = IDirect3DDevice9_StretchRect(device, src_surface, NULL, dst_surface, NULL, D3DTEXF_NONE);
                    ok(hr == expected_hr, "Got hr %#x, expected %#x (formats %#x/%#x, pools %#x/%#x).\n",
                            hr, expected_hr, src_format, dst_format, src_pool, dst_pool);

                    /* offscreen plain <-> texture */
                    hr = IDirect3DDevice9_StretchRect(device, src, NULL, dst_surface, NULL, D3DTEXF_NONE);
                    todo_wine_if(src_pool == D3DPOOL_DEFAULT && dst_pool == D3DPOOL_DEFAULT)
                    ok(hr == D3DERR_INVALIDCALL, "Got hr %#x (formats %#x/%#x, pools %#x/%#x).\n",
                            hr, src_format, dst_format, src_pool, dst_pool);
                    hr = IDirect3DDevice9_StretchRect(device, src_surface, NULL, dst, NULL, D3DTEXF_NONE);
                    ok(hr == D3DERR_INVALIDCALL, "Got hr %#x (formats %#x/%#x, pools %#x/%#x).\n",
                            hr, src_format, dst_format, src_pool, dst_pool);

                    /* offscreen plain <-> render target */
                    expected_hr = src_pool == D3DPOOL_DEFAULT ? D3D_OK : D3DERR_INVALIDCALL;
                    hr = IDirect3DDevice9_StretchRect(device, src_surface, NULL, dst_rt, NULL, D3DTEXF_NONE);
                    ok(hr == expected_hr, "Got hr %#x, expected hr %#x (formats %#x/%#x, pool %#x).\n",
                            hr, expected_hr, src_format, dst_format, src_pool);
                    hr = IDirect3DDevice9_StretchRect(device, src_rt, NULL, dst_surface, NULL, D3DTEXF_NONE);
                    todo_wine_if(dst_pool == D3DPOOL_DEFAULT)
                    ok(hr == D3DERR_INVALIDCALL, "Got hr %#x (formats %#x/%#x, pool %#x).\n",
                            hr, src_format, dst_format, dst_pool);

                    IDirect3DSurface9_Release(src_surface);
                    IDirect3DSurface9_Release(dst_surface);

                    IDirect3DSurface9_Release(src);
                    IDirect3DSurface9_Release(dst);
                }
            }

            IDirect3DSurface9_Release(src_rt);
            IDirect3DSurface9_Release(dst_rt);
        }
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

#define test_device_caps_adapter_group(a, b, c) _test_device_caps_adapter_group(a, b, c, __LINE__)
static void _test_device_caps_adapter_group(const D3DCAPS9 *caps, unsigned int adapter_idx,
        unsigned int adapter_count, int line)
{
    ok_(__FILE__, line)(caps->AdapterOrdinal == adapter_idx,
            "Adapter %u: Got unexpected AdapterOrdinal %u.\n", adapter_idx, caps->AdapterOrdinal);
    /* Single GPU */
    if (caps->NumberOfAdaptersInGroup == adapter_count)
    {
        ok_(__FILE__, line)(caps->MasterAdapterOrdinal == 0,
                "Adapter %u: Expect MasterAdapterOrdinal %u, got %u.\n", adapter_idx, 0,
                caps->MasterAdapterOrdinal);
        ok_(__FILE__, line)(caps->AdapterOrdinalInGroup == caps->AdapterOrdinal,
                "Adapter %u: Expect AdapterOrdinalInGroup %u, got %u.\n", adapter_idx,
                caps->AdapterOrdinal, caps->AdapterOrdinalInGroup);
        ok_(__FILE__, line)(caps->NumberOfAdaptersInGroup ==
                (caps->AdapterOrdinalInGroup ? 0 : adapter_count),
                "Adapter %u: Expect NumberOfAdaptersInGroup %u, got %u.\n", adapter_idx,
                (caps->AdapterOrdinalInGroup ? 0 : adapter_count),
                caps->NumberOfAdaptersInGroup);
    }
    /* Multiple GPUs and each GPU has only one output */
    else if (caps->NumberOfAdaptersInGroup == 1)
    {
        ok_(__FILE__, line)(caps->MasterAdapterOrdinal == caps->AdapterOrdinal,
                "Adapter %u: Expect MasterAdapterOrdinal %u, got %u.\n", adapter_idx,
                caps->AdapterOrdinal, caps->MasterAdapterOrdinal);
        ok_(__FILE__, line)(caps->AdapterOrdinalInGroup == 0,
                "Adapter %u: Expect AdapterOrdinalInGroup %u, got %u.\n", adapter_idx, 0,
                caps->AdapterOrdinalInGroup);
    }
    /* TODO: Test other multi-GPU setup */
}

static void test_device_caps(void)
{
    unsigned int adapter_idx, adapter_count;
    struct device_desc device_desc;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    device_desc.device_window = window;
    device_desc.width = 640;
    device_desc.height = 480;
    device_desc.flags = 0;

    adapter_count = IDirect3D9_GetAdapterCount(d3d);
    for (adapter_idx = 0; adapter_idx < adapter_count; ++adapter_idx)
    {
        /* Test IDirect3D9_GetDeviceCaps */
        hr = IDirect3D9_GetDeviceCaps(d3d, adapter_idx, D3DDEVTYPE_HAL, &caps);
        ok(hr == D3D_OK || hr == D3DERR_NOTAVAILABLE, "Adapter %u: GetDeviceCaps failed, hr %#x.\n",
                adapter_idx, hr);
        if (hr == D3DERR_NOTAVAILABLE)
        {
            skip("Adapter %u: No Direct3D support, skipping test.\n", adapter_idx);
            break;
        }
        test_device_caps_adapter_group(&caps, adapter_idx, adapter_count);

        /* Test IDirect3DDevice9_GetDeviceCaps */
        device_desc.adapter_ordinal = adapter_idx;
        device = create_device(d3d, window, &device_desc);
        if (!device)
        {
            skip("Adapter %u: Failed to create a D3D device, skipping test.\n", adapter_idx);
            break;
        }
        hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
        ok(SUCCEEDED(hr), "Adapter %u: Failed to get caps, hr %#x.\n", adapter_idx, hr);

        test_device_caps_adapter_group(&caps, adapter_idx, adapter_count);
        ok(!(caps.Caps & ~D3DCAPS_READ_SCANLINE),
                "Adapter %u: Caps field has unexpected flags %#x.\n", adapter_idx, caps.Caps);
        ok(!(caps.Caps2 & ~(D3DCAPS2_FULLSCREENGAMMA | D3DCAPS2_CANCALIBRATEGAMMA | D3DCAPS2_RESERVED
                | D3DCAPS2_CANMANAGERESOURCE | D3DCAPS2_DYNAMICTEXTURES | D3DCAPS2_CANAUTOGENMIPMAP
                | D3DCAPS2_CANSHARERESOURCE)),
                "Adapter %u: Caps2 field has unexpected flags %#x.\n", adapter_idx, caps.Caps2);
        /* AMD doesn't filter all the ddraw / d3d9 caps. Consider that behavior
         * broken. */
        ok(!(caps.Caps3 & ~(D3DCAPS3_ALPHA_FULLSCREEN_FLIP_OR_DISCARD
                | D3DCAPS3_LINEAR_TO_SRGB_PRESENTATION | D3DCAPS3_COPY_TO_VIDMEM
                | D3DCAPS3_COPY_TO_SYSTEMMEM | D3DCAPS3_DXVAHD | D3DCAPS3_DXVAHD_LIMITED
                | D3DCAPS3_RESERVED)),
                "Adapter %u: Caps3 field has unexpected flags %#x.\n", adapter_idx, caps.Caps3);
        ok(!(caps.PrimitiveMiscCaps & ~(D3DPMISCCAPS_MASKZ | D3DPMISCCAPS_LINEPATTERNREP
                | D3DPMISCCAPS_CULLNONE | D3DPMISCCAPS_CULLCW | D3DPMISCCAPS_CULLCCW
                | D3DPMISCCAPS_COLORWRITEENABLE | D3DPMISCCAPS_CLIPPLANESCALEDPOINTS
                | D3DPMISCCAPS_CLIPTLVERTS | D3DPMISCCAPS_TSSARGTEMP | D3DPMISCCAPS_BLENDOP
                | D3DPMISCCAPS_NULLREFERENCE | D3DPMISCCAPS_INDEPENDENTWRITEMASKS
                | D3DPMISCCAPS_PERSTAGECONSTANT | D3DPMISCCAPS_FOGANDSPECULARALPHA
                | D3DPMISCCAPS_SEPARATEALPHABLEND | D3DPMISCCAPS_MRTINDEPENDENTBITDEPTHS
                | D3DPMISCCAPS_MRTPOSTPIXELSHADERBLENDING | D3DPMISCCAPS_FOGVERTEXCLAMPED
                | D3DPMISCCAPS_POSTBLENDSRGBCONVERT)),
                "Adapter %u: PrimitiveMiscCaps field has unexpected flags %#x.\n", adapter_idx,
                caps.PrimitiveMiscCaps);
        ok(!(caps.RasterCaps & ~(D3DPRASTERCAPS_DITHER | D3DPRASTERCAPS_ZTEST
                | D3DPRASTERCAPS_FOGVERTEX | D3DPRASTERCAPS_FOGTABLE
                | D3DPRASTERCAPS_MIPMAPLODBIAS | D3DPRASTERCAPS_ZBUFFERLESSHSR
                | D3DPRASTERCAPS_FOGRANGE | D3DPRASTERCAPS_ANISOTROPY | D3DPRASTERCAPS_WBUFFER
                | D3DPRASTERCAPS_WFOG | D3DPRASTERCAPS_ZFOG | D3DPRASTERCAPS_COLORPERSPECTIVE
                | D3DPRASTERCAPS_SCISSORTEST | D3DPRASTERCAPS_SLOPESCALEDEPTHBIAS
                | D3DPRASTERCAPS_DEPTHBIAS | D3DPRASTERCAPS_MULTISAMPLE_TOGGLE))
                || broken(!(caps.RasterCaps & ~0x0f736191)),
                "Adapter %u: RasterCaps field has unexpected flags %#x.\n", adapter_idx,
                caps.RasterCaps);
        /* D3DPBLENDCAPS_SRCCOLOR2 and D3DPBLENDCAPS_INVSRCCOLOR2 are only
         * advertised on the reference rasterizer and WARP. */
        ok(!(caps.SrcBlendCaps & ~(D3DPBLENDCAPS_ZERO | D3DPBLENDCAPS_ONE | D3DPBLENDCAPS_SRCCOLOR
                | D3DPBLENDCAPS_INVSRCCOLOR | D3DPBLENDCAPS_SRCALPHA | D3DPBLENDCAPS_INVSRCALPHA
                | D3DPBLENDCAPS_DESTALPHA | D3DPBLENDCAPS_INVDESTALPHA | D3DPBLENDCAPS_DESTCOLOR
                | D3DPBLENDCAPS_INVDESTCOLOR | D3DPBLENDCAPS_SRCALPHASAT | D3DPBLENDCAPS_BOTHSRCALPHA
                | D3DPBLENDCAPS_BOTHINVSRCALPHA | D3DPBLENDCAPS_BLENDFACTOR))
                || broken(!(caps.SrcBlendCaps & ~(D3DPBLENDCAPS_ZERO | D3DPBLENDCAPS_ONE | D3DPBLENDCAPS_SRCCOLOR
                | D3DPBLENDCAPS_INVSRCCOLOR | D3DPBLENDCAPS_SRCALPHA | D3DPBLENDCAPS_INVSRCALPHA
                | D3DPBLENDCAPS_DESTALPHA | D3DPBLENDCAPS_INVDESTALPHA | D3DPBLENDCAPS_DESTCOLOR
                | D3DPBLENDCAPS_INVDESTCOLOR | D3DPBLENDCAPS_SRCALPHASAT | D3DPBLENDCAPS_BOTHSRCALPHA
                | D3DPBLENDCAPS_BOTHINVSRCALPHA | D3DPBLENDCAPS_BLENDFACTOR
                | D3DPBLENDCAPS_SRCCOLOR2 | D3DPBLENDCAPS_INVSRCCOLOR2))),
                "Adapter %u: SrcBlendCaps field has unexpected flags %#x.\n", adapter_idx,
                caps.SrcBlendCaps);
        ok(!(caps.DestBlendCaps & ~(D3DPBLENDCAPS_ZERO | D3DPBLENDCAPS_ONE | D3DPBLENDCAPS_SRCCOLOR
                | D3DPBLENDCAPS_INVSRCCOLOR | D3DPBLENDCAPS_SRCALPHA | D3DPBLENDCAPS_INVSRCALPHA
                | D3DPBLENDCAPS_DESTALPHA | D3DPBLENDCAPS_INVDESTALPHA | D3DPBLENDCAPS_DESTCOLOR
                | D3DPBLENDCAPS_INVDESTCOLOR | D3DPBLENDCAPS_SRCALPHASAT | D3DPBLENDCAPS_BOTHSRCALPHA
                | D3DPBLENDCAPS_BOTHINVSRCALPHA | D3DPBLENDCAPS_BLENDFACTOR))
                || broken(!(caps.SrcBlendCaps & ~(D3DPBLENDCAPS_ZERO | D3DPBLENDCAPS_ONE | D3DPBLENDCAPS_SRCCOLOR
                | D3DPBLENDCAPS_INVSRCCOLOR | D3DPBLENDCAPS_SRCALPHA | D3DPBLENDCAPS_INVSRCALPHA
                | D3DPBLENDCAPS_DESTALPHA | D3DPBLENDCAPS_INVDESTALPHA | D3DPBLENDCAPS_DESTCOLOR
                | D3DPBLENDCAPS_INVDESTCOLOR | D3DPBLENDCAPS_SRCALPHASAT | D3DPBLENDCAPS_BOTHSRCALPHA
                | D3DPBLENDCAPS_BOTHINVSRCALPHA | D3DPBLENDCAPS_BLENDFACTOR
                | D3DPBLENDCAPS_SRCCOLOR2 | D3DPBLENDCAPS_INVSRCCOLOR2))),
                "Adapter %u: DestBlendCaps field has unexpected flags %#x.\n", adapter_idx,
                caps.DestBlendCaps);
        ok(!(caps.TextureCaps & ~(D3DPTEXTURECAPS_PERSPECTIVE | D3DPTEXTURECAPS_POW2
                | D3DPTEXTURECAPS_ALPHA | D3DPTEXTURECAPS_SQUAREONLY
                | D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE | D3DPTEXTURECAPS_ALPHAPALETTE
                | D3DPTEXTURECAPS_NONPOW2CONDITIONAL | D3DPTEXTURECAPS_PROJECTED
                | D3DPTEXTURECAPS_CUBEMAP | D3DPTEXTURECAPS_VOLUMEMAP | D3DPTEXTURECAPS_MIPMAP
                | D3DPTEXTURECAPS_MIPVOLUMEMAP | D3DPTEXTURECAPS_MIPCUBEMAP
                | D3DPTEXTURECAPS_CUBEMAP_POW2 | D3DPTEXTURECAPS_VOLUMEMAP_POW2
                | D3DPTEXTURECAPS_NOPROJECTEDBUMPENV)),
                "Adapter %u: TextureCaps field has unexpected flags %#x.\n", adapter_idx,
                caps.TextureCaps);
        ok(!(caps.TextureFilterCaps & ~(D3DPTFILTERCAPS_MINFPOINT | D3DPTFILTERCAPS_MINFLINEAR
                | D3DPTFILTERCAPS_MINFANISOTROPIC | D3DPTFILTERCAPS_MINFPYRAMIDALQUAD
                | D3DPTFILTERCAPS_MINFGAUSSIANQUAD | D3DPTFILTERCAPS_MIPFPOINT
                | D3DPTFILTERCAPS_MIPFLINEAR | D3DPTFILTERCAPS_CONVOLUTIONMONO
                | D3DPTFILTERCAPS_MAGFPOINT | D3DPTFILTERCAPS_MAGFLINEAR
                | D3DPTFILTERCAPS_MAGFANISOTROPIC | D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD
                | D3DPTFILTERCAPS_MAGFGAUSSIANQUAD)),
                "Adapter %u: TextureFilterCaps field has unexpected flags %#x.\n", adapter_idx,
                caps.TextureFilterCaps);
        ok(!(caps.CubeTextureFilterCaps & ~(D3DPTFILTERCAPS_MINFPOINT | D3DPTFILTERCAPS_MINFLINEAR
                | D3DPTFILTERCAPS_MINFANISOTROPIC | D3DPTFILTERCAPS_MINFPYRAMIDALQUAD
                | D3DPTFILTERCAPS_MINFGAUSSIANQUAD | D3DPTFILTERCAPS_MIPFPOINT
                | D3DPTFILTERCAPS_MIPFLINEAR | D3DPTFILTERCAPS_MAGFPOINT | D3DPTFILTERCAPS_MAGFLINEAR
                | D3DPTFILTERCAPS_MAGFANISOTROPIC | D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD
                | D3DPTFILTERCAPS_MAGFGAUSSIANQUAD)),
                "Adapter %u: CubeTextureFilterCaps field has unexpected flags %#x.\n", adapter_idx,
                caps.CubeTextureFilterCaps);
        ok(!(caps.VolumeTextureFilterCaps & ~(D3DPTFILTERCAPS_MINFPOINT | D3DPTFILTERCAPS_MINFLINEAR
                | D3DPTFILTERCAPS_MINFANISOTROPIC | D3DPTFILTERCAPS_MINFPYRAMIDALQUAD
                | D3DPTFILTERCAPS_MINFGAUSSIANQUAD | D3DPTFILTERCAPS_MIPFPOINT
                | D3DPTFILTERCAPS_MIPFLINEAR | D3DPTFILTERCAPS_MAGFPOINT | D3DPTFILTERCAPS_MAGFLINEAR
                | D3DPTFILTERCAPS_MAGFANISOTROPIC | D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD
                | D3DPTFILTERCAPS_MAGFGAUSSIANQUAD)),
                "Adapter %u: VolumeTextureFilterCaps field has unexpected flags %#x.\n",
                adapter_idx, caps.VolumeTextureFilterCaps);
        ok(!(caps.LineCaps & ~(D3DLINECAPS_TEXTURE | D3DLINECAPS_ZTEST | D3DLINECAPS_BLEND
                | D3DLINECAPS_ALPHACMP | D3DLINECAPS_FOG | D3DLINECAPS_ANTIALIAS)),
                "Adapter %u: LineCaps field has unexpected flags %#x.\n", adapter_idx,
                caps.LineCaps);
        ok(!(caps.StencilCaps & ~(D3DSTENCILCAPS_KEEP | D3DSTENCILCAPS_ZERO | D3DSTENCILCAPS_REPLACE
                | D3DSTENCILCAPS_INCRSAT | D3DSTENCILCAPS_DECRSAT | D3DSTENCILCAPS_INVERT
                | D3DSTENCILCAPS_INCR | D3DSTENCILCAPS_DECR | D3DSTENCILCAPS_TWOSIDED)),
                "Adapter %u: StencilCaps field has unexpected flags %#x.\n", adapter_idx,
                caps.StencilCaps);
        ok(!(caps.VertexProcessingCaps & ~(D3DVTXPCAPS_TEXGEN | D3DVTXPCAPS_MATERIALSOURCE7
                | D3DVTXPCAPS_DIRECTIONALLIGHTS | D3DVTXPCAPS_POSITIONALLIGHTS | D3DVTXPCAPS_LOCALVIEWER
                | D3DVTXPCAPS_TWEENING | D3DVTXPCAPS_TEXGEN_SPHEREMAP
                | D3DVTXPCAPS_NO_TEXGEN_NONLOCALVIEWER)),
                "Adapter %u: VertexProcessingCaps field has unexpected flags %#x.\n", adapter_idx,
                caps.VertexProcessingCaps);
        /* Both Nvidia and AMD give 10 here. */
        ok(caps.MaxActiveLights <= 10,
                "Adapter %u: MaxActiveLights field has unexpected value %u.\n", adapter_idx,
                caps.MaxActiveLights);
        /* AMD gives 6, Nvidia returns 8. */
        ok(caps.MaxUserClipPlanes <= 8,
                "Adapter %u: MaxUserClipPlanes field has unexpected value %u.\n", adapter_idx,
                caps.MaxUserClipPlanes);
        ok(caps.MaxVertexW == 0.0f || caps.MaxVertexW >= 1e10f,
                "Adapter %u: MaxVertexW field has unexpected value %.8e.\n", adapter_idx,
                caps.MaxVertexW);

        refcount = IDirect3DDevice9_Release(device);
        ok(!refcount, "Adapter %u: Device has %u references left.\n", adapter_idx, refcount);
    }
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_resource_access(void)
{
    IDirect3DSurface9 *backbuffer, *depth_stencil;
    D3DFORMAT colour_format, depth_format, format;
    BOOL depth_2d, depth_cube, depth_plain;
    D3DADAPTER_IDENTIFIER9 identifier;
    struct device_desc device_desc;
    D3DSURFACE_DESC surface_desc;
    IDirect3DDevice9 *device;
    unsigned int i, j;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    BOOL warp;

    enum surface_type
    {
        SURFACE_2D,
        SURFACE_CUBE,
        SURFACE_RT,
        SURFACE_DS,
        SURFACE_PLAIN,
    };

    enum resource_format
    {
        FORMAT_COLOUR,
        FORMAT_ATI2,
        FORMAT_DEPTH,
    };

    static const struct
    {
        D3DPOOL pool;
        enum resource_format format;
        DWORD usage;
        BOOL valid;
    }
    tests[] =
    {
        /* 0 */
        {D3DPOOL_DEFAULT,   FORMAT_COLOUR, 0,                                        TRUE},
        {D3DPOOL_DEFAULT,   FORMAT_ATI2,   0,                                        TRUE},
        {D3DPOOL_DEFAULT,   FORMAT_DEPTH,  0,                                        TRUE},
        {D3DPOOL_DEFAULT,   FORMAT_COLOUR, D3DUSAGE_RENDERTARGET,                    TRUE},
        {D3DPOOL_DEFAULT,   FORMAT_DEPTH,  D3DUSAGE_RENDERTARGET,                    FALSE},
        {D3DPOOL_DEFAULT,   FORMAT_COLOUR, D3DUSAGE_DEPTHSTENCIL,                    FALSE},
        {D3DPOOL_DEFAULT,   FORMAT_DEPTH,  D3DUSAGE_DEPTHSTENCIL,                    TRUE},
        /* 7 */
        {D3DPOOL_DEFAULT,   FORMAT_COLOUR, D3DUSAGE_DYNAMIC,                         TRUE},
        {D3DPOOL_DEFAULT,   FORMAT_ATI2,   D3DUSAGE_DYNAMIC,                         TRUE},
        {D3DPOOL_DEFAULT,   FORMAT_DEPTH,  D3DUSAGE_DYNAMIC,                         TRUE},
        {D3DPOOL_DEFAULT,   FORMAT_COLOUR, D3DUSAGE_DYNAMIC | D3DUSAGE_RENDERTARGET, FALSE},
        {D3DPOOL_DEFAULT,   FORMAT_DEPTH,  D3DUSAGE_DYNAMIC | D3DUSAGE_RENDERTARGET, FALSE},
        {D3DPOOL_DEFAULT,   FORMAT_COLOUR, D3DUSAGE_DYNAMIC | D3DUSAGE_DEPTHSTENCIL, FALSE},
        {D3DPOOL_DEFAULT,   FORMAT_DEPTH,  D3DUSAGE_DYNAMIC | D3DUSAGE_DEPTHSTENCIL, FALSE},
        /* 14 */
        {D3DPOOL_MANAGED,   FORMAT_COLOUR, 0,                                        TRUE},
        {D3DPOOL_MANAGED,   FORMAT_ATI2,   0,                                        TRUE},
        {D3DPOOL_MANAGED,   FORMAT_DEPTH,  0,                                        FALSE},
        {D3DPOOL_MANAGED,   FORMAT_COLOUR, D3DUSAGE_RENDERTARGET,                    FALSE},
        {D3DPOOL_MANAGED,   FORMAT_DEPTH,  D3DUSAGE_RENDERTARGET,                    FALSE},
        {D3DPOOL_MANAGED,   FORMAT_COLOUR, D3DUSAGE_DEPTHSTENCIL,                    FALSE},
        {D3DPOOL_MANAGED,   FORMAT_DEPTH,  D3DUSAGE_DEPTHSTENCIL,                    FALSE},
        /* 21 */
        {D3DPOOL_SYSTEMMEM, FORMAT_COLOUR, 0,                                        TRUE},
        {D3DPOOL_SYSTEMMEM, FORMAT_ATI2,   0,                                        TRUE},
        {D3DPOOL_SYSTEMMEM, FORMAT_DEPTH,  0,                                        FALSE},
        {D3DPOOL_SYSTEMMEM, FORMAT_COLOUR, D3DUSAGE_RENDERTARGET,                    FALSE},
        {D3DPOOL_SYSTEMMEM, FORMAT_DEPTH,  D3DUSAGE_RENDERTARGET,                    FALSE},
        {D3DPOOL_SYSTEMMEM, FORMAT_COLOUR, D3DUSAGE_DEPTHSTENCIL,                    FALSE},
        {D3DPOOL_SYSTEMMEM, FORMAT_DEPTH,  D3DUSAGE_DEPTHSTENCIL,                    FALSE},
        /* 28 */
        {D3DPOOL_SCRATCH,   FORMAT_COLOUR, 0,                                        TRUE},
        {D3DPOOL_SCRATCH,   FORMAT_ATI2,   0,                                        TRUE},
        {D3DPOOL_SCRATCH,   FORMAT_DEPTH,  0,                                        FALSE},
        {D3DPOOL_SCRATCH,   FORMAT_COLOUR, D3DUSAGE_RENDERTARGET,                    FALSE},
        {D3DPOOL_SCRATCH,   FORMAT_DEPTH,  D3DUSAGE_RENDERTARGET,                    FALSE},
        {D3DPOOL_SCRATCH,   FORMAT_COLOUR, D3DUSAGE_DEPTHSTENCIL,                    FALSE},
        {D3DPOOL_SCRATCH,   FORMAT_DEPTH,  D3DUSAGE_DEPTHSTENCIL,                    FALSE},
    };
    static const struct
    {
        const char *name;
        enum surface_type type;
    }
    surface_types[] =
    {
        {"2D", SURFACE_2D},
        {"CUBE", SURFACE_CUBE},
        {"RT", SURFACE_RT},
        {"DS", SURFACE_DS},
        {"PLAIN", SURFACE_PLAIN},
    };

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    device_desc.adapter_ordinal = D3DADAPTER_DEFAULT;
    device_desc.device_window = window;
    device_desc.width = 16;
    device_desc.height = 16;
    device_desc.flags = 0;
    if (!(device = create_device(d3d, window, &device_desc)))
    {
        skip("Failed to create a D3D device.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }
    hr = IDirect3D9_GetAdapterIdentifier(d3d, D3DADAPTER_DEFAULT, 0, &identifier);
    ok(SUCCEEDED(hr), "Failed to get adapter identifier, hr %#x.\n", hr);
    warp = adapter_is_warp(&identifier);

    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DSurface9_GetDesc(backbuffer, &surface_desc);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    colour_format = surface_desc.Format;

    hr = IDirect3DDevice9_GetDepthStencilSurface(device, &depth_stencil);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DSurface9_GetDesc(depth_stencil, &surface_desc);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    depth_format = surface_desc.Format;

    depth_2d = SUCCEEDED(IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, 0, D3DRTYPE_TEXTURE, depth_format));
    depth_cube = SUCCEEDED(IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, 0, D3DRTYPE_CUBETEXTURE, depth_format));
    depth_plain = SUCCEEDED(IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, 0, D3DRTYPE_SURFACE, depth_format));

    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZRHW);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(surface_types); ++i)
    {
        for (j = 0; j < ARRAY_SIZE(tests); ++j)
        {
            IDirect3DCubeTexture9 *texture_cube;
            IDirect3DBaseTexture9 *texture;
            IDirect3DTexture9 *texture_2d;
            IDirect3DSurface9 *surface;
            HRESULT expected_hr;
            D3DLOCKED_RECT lr;

            if (tests[j].format == FORMAT_ATI2)
                format = MAKEFOURCC('A','T','I','2');
            else if (tests[j].format == FORMAT_DEPTH)
                format = depth_format;
            else
                format = colour_format;

            if (tests[j].format == FORMAT_ATI2 && FAILED(IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT,
                    D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, 0, D3DRTYPE_TEXTURE, format)))
            {
                skip("ATI2N texture not supported.\n");
                continue;
            }

            switch (surface_types[i].type)
            {
                case SURFACE_2D:
                    hr = IDirect3DDevice9_CreateTexture(device, 16, 16, 1,
                            tests[j].usage, format, tests[j].pool, &texture_2d, NULL);
                    todo_wine_if(!tests[j].valid && tests[j].format == FORMAT_DEPTH && !tests[j].usage)
                        ok(hr == (tests[j].valid && (tests[j].format != FORMAT_DEPTH || depth_2d)
                                ? D3D_OK : D3DERR_INVALIDCALL),
                                "Test %s %u: Got unexpected hr %#x.\n", surface_types[i].name, j, hr);
                    if (FAILED(hr))
                        continue;

                    hr = IDirect3DTexture9_GetSurfaceLevel(texture_2d, 0, &surface);
                    ok(hr == D3D_OK, "Test %s %u: Got unexpected hr %#x.\n", surface_types[i].name, j, hr);
                    IDirect3DTexture9_Release(texture_2d);
                    break;

                case SURFACE_CUBE:
                    hr = IDirect3DDevice9_CreateCubeTexture(device, 16, 1,
                            tests[j].usage, format, tests[j].pool, &texture_cube, NULL);
                    todo_wine_if(!tests[j].valid && tests[j].format == FORMAT_DEPTH && !tests[j].usage)
                        ok(hr == (tests[j].valid && (tests[j].format != FORMAT_DEPTH || depth_cube)
                                ? D3D_OK : D3DERR_INVALIDCALL),
                                "Test %s %u: Got unexpected hr %#x.\n", surface_types[i].name, j, hr);
                    if (FAILED(hr))
                        continue;

                    hr = IDirect3DCubeTexture9_GetCubeMapSurface(texture_cube,
                            D3DCUBEMAP_FACE_POSITIVE_X, 0, &surface);
                    ok(hr == D3D_OK, "Test %s %u: Got unexpected hr %#x.\n", surface_types[i].name, j, hr);
                    IDirect3DCubeTexture9_Release(texture_cube);
                    break;

                case SURFACE_RT:
                    hr = IDirect3DDevice9_CreateRenderTarget(device, 16, 16, format,
                            D3DMULTISAMPLE_NONE, 0, tests[j].usage & D3DUSAGE_DYNAMIC, &surface, NULL);
                    ok(hr == (tests[j].format == FORMAT_COLOUR ? D3D_OK : D3DERR_INVALIDCALL),
                            "Test %s %u: Got unexpected hr %#x.\n", surface_types[i].name, j, hr);
                    if (FAILED(hr))
                        continue;
                    break;

                case SURFACE_DS:
                    hr = IDirect3DDevice9_CreateDepthStencilSurface(device, 16, 16, format,
                            D3DMULTISAMPLE_NONE, 0, tests[j].usage & D3DUSAGE_DYNAMIC, &surface, NULL);
                    ok(hr == (tests[j].format == FORMAT_DEPTH ? D3D_OK : D3DERR_INVALIDCALL),
                            "Test %s %u: Got unexpected hr %#x.\n", surface_types[i].name, j, hr);
                    if (FAILED(hr))
                        continue;
                    break;

                case SURFACE_PLAIN:
                    hr = IDirect3DDevice9_CreateOffscreenPlainSurface(device,
                            16, 16, format, tests[j].pool, &surface, NULL);
                    todo_wine_if(tests[j].format == FORMAT_ATI2 && tests[j].pool != D3DPOOL_MANAGED
                            && tests[j].pool != D3DPOOL_SCRATCH)
                        ok(hr == (tests[j].pool != D3DPOOL_MANAGED && (tests[j].format != FORMAT_DEPTH || depth_plain)
                                && (tests[j].format != FORMAT_ATI2 || tests[j].pool == D3DPOOL_SCRATCH)
                                ? D3D_OK : D3DERR_INVALIDCALL),
                                "Test %s %u: Got unexpected hr %#x.\n", surface_types[i].name, j, hr);
                    if (FAILED(hr))
                        continue;
                    break;

                default:
                    ok(0, "Invalid surface type %#x.\n", surface_types[i].type);
                    continue;
            }

            hr = IDirect3DSurface9_GetDesc(surface, &surface_desc);
            ok(hr == D3D_OK, "Test %s %u: Got unexpected hr %#x.\n", surface_types[i].name, j, hr);
            if (surface_types[i].type == SURFACE_RT)
            {
                ok(surface_desc.Usage == D3DUSAGE_RENDERTARGET, "Test %s %u: Got unexpected usage %#x.\n",
                        surface_types[i].name, j, surface_desc.Usage);
                ok(surface_desc.Pool == D3DPOOL_DEFAULT, "Test %s %u: Got unexpected pool %#x.\n",
                        surface_types[i].name, j, surface_desc.Pool);
            }
            else if (surface_types[i].type == SURFACE_DS)
            {
                ok(surface_desc.Usage == D3DUSAGE_DEPTHSTENCIL, "Test %s %u: Got unexpected usage %#x.\n",
                        surface_types[i].name, j, surface_desc.Usage);
                ok(surface_desc.Pool == D3DPOOL_DEFAULT, "Test %s %u: Got unexpected pool %#x.\n",
                        surface_types[i].name, j, surface_desc.Pool);
            }
            else if (surface_types[i].type == SURFACE_PLAIN)
            {
                ok(!surface_desc.Usage, "Test %s %u: Got unexpected usage %#x.\n",
                        surface_types[i].name, j, surface_desc.Usage);
                ok(surface_desc.Pool == tests[j].pool, "Test %s %u: Got unexpected pool %#x.\n",
                        surface_types[i].name, j, surface_desc.Pool);
            }
            else
            {
                ok(surface_desc.Usage == tests[j].usage, "Test %s %u: Got unexpected usage %#x.\n",
                        surface_types[i].name, j, surface_desc.Usage);
                ok(surface_desc.Pool == tests[j].pool, "Test %s %u: Got unexpected pool %#x.\n",
                        surface_types[i].name, j, surface_desc.Pool);
            }

            hr = IDirect3DSurface9_LockRect(surface, &lr, NULL, 0);
            if (surface_desc.Pool != D3DPOOL_DEFAULT || surface_desc.Usage & D3DUSAGE_DYNAMIC
                    || (surface_types[i].type == SURFACE_RT && tests[j].usage & D3DUSAGE_DYNAMIC)
                    || surface_types[i].type == SURFACE_PLAIN
                    || tests[j].format == FORMAT_ATI2)
                expected_hr = D3D_OK;
            else
                expected_hr = D3DERR_INVALIDCALL;
            ok(hr == expected_hr, "Test %s %u: Got unexpected hr %#x.\n", surface_types[i].name, j, hr);
            hr = IDirect3DSurface9_UnlockRect(surface);
            todo_wine_if(expected_hr != D3D_OK && surface_types[i].type == SURFACE_2D)
                ok(hr == expected_hr, "Test %s %u: Got unexpected hr %#x.\n", surface_types[i].name, j, hr);

            if (SUCCEEDED(IDirect3DSurface9_GetContainer(surface, &IID_IDirect3DBaseTexture9, (void **)&texture)))
            {
                hr = IDirect3DDevice9_SetTexture(device, 0, texture);
                ok(hr == D3D_OK, "Test %s %u: Got unexpected hr %#x.\n", surface_types[i].name, j, hr);
                hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
                ok(hr == D3D_OK, "Test %s %u: Got unexpected hr %#x.\n", surface_types[i].name, j, hr);
                IDirect3DBaseTexture9_Release(texture);
            }

            hr = IDirect3DDevice9_SetRenderTarget(device, 0, surface);
            ok(hr == (surface_desc.Usage & D3DUSAGE_RENDERTARGET ? D3D_OK : D3DERR_INVALIDCALL),
                    "Test %s %u: Got unexpected hr %#x.\n", surface_types[i].name, j, hr);
            hr = IDirect3DDevice9_SetRenderTarget(device, 0, backbuffer);
            ok(hr == D3D_OK, "Test %s %u: Got unexpected hr %#x.\n", surface_types[i].name, j, hr);

            hr = IDirect3DDevice9_SetDepthStencilSurface(device, surface);
            ok(hr == (surface_desc.Usage & D3DUSAGE_DEPTHSTENCIL ? D3D_OK : D3DERR_INVALIDCALL),
                    "Test %s %u: Got unexpected hr %#x.\n", surface_types[i].name, j, hr);
            hr = IDirect3DDevice9_SetDepthStencilSurface(device, depth_stencil);
            ok(hr == D3D_OK, "Test %s %u: Got unexpected hr %#x.\n", surface_types[i].name, j, hr);

            IDirect3DSurface9_Release(surface);
        }
    }

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        IDirect3DVolumeTexture9 *texture;
        D3DVOLUME_DESC volume_desc;
        IDirect3DVolume9 *volume;
        HRESULT expected_hr;
        D3DLOCKED_BOX lb;

        if (tests[i].format == FORMAT_DEPTH)
            continue;

        if (tests[i].format == FORMAT_ATI2)
            format = MAKEFOURCC('A','T','I','2');
        else
            format = colour_format;

        hr = IDirect3DDevice9_CreateVolumeTexture(device, 16, 16, 1, 1,
                tests[i].usage, format, tests[i].pool, &texture, NULL);
        ok((hr == (!(tests[i].usage & ~D3DUSAGE_DYNAMIC)
                && (tests[i].format != FORMAT_ATI2 || tests[i].pool == D3DPOOL_SCRATCH)
                ? D3D_OK : D3DERR_INVALIDCALL))
                || (tests[i].format == FORMAT_ATI2 && (hr == D3D_OK || warp)),
                "Test %u: Got unexpected hr %#x.\n", i, hr);
        if (FAILED(hr))
            continue;

        hr = IDirect3DVolumeTexture9_GetVolumeLevel(texture, 0, &volume);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);

        hr = IDirect3DVolume9_GetDesc(volume, &volume_desc);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        ok(volume_desc.Usage == tests[i].usage, "Test %u: Got unexpected usage %#x.\n", i, volume_desc.Usage);
        ok(volume_desc.Pool == tests[i].pool, "Test %u: Got unexpected pool %#x.\n", i, volume_desc.Pool);

        hr = IDirect3DVolume9_LockBox(volume, &lb, NULL, 0);
        if (volume_desc.Pool != D3DPOOL_DEFAULT || volume_desc.Usage & D3DUSAGE_DYNAMIC)
            expected_hr = D3D_OK;
        else
            expected_hr = D3DERR_INVALIDCALL;
        ok(hr == expected_hr || (volume_desc.Pool == D3DPOOL_DEFAULT && hr == D3D_OK),
                "Test %u: Got unexpected hr %#x.\n", i, hr);
        hr = IDirect3DVolume9_UnlockBox(volume);
        ok(hr == expected_hr || (volume_desc.Pool == D3DPOOL_DEFAULT && hr == D3D_OK),
                "Test %u: Got unexpected hr %#x.\n", i, hr);

        hr = IDirect3DDevice9_SetTexture(device, 0, (IDirect3DBaseTexture9 *)texture);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);

        IDirect3DVolume9_Release(volume);
        IDirect3DVolumeTexture9_Release(texture);
    }

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        D3DINDEXBUFFER_DESC ib_desc;
        IDirect3DIndexBuffer9 *ib;
        void *data;

        hr = IDirect3DDevice9_CreateIndexBuffer(device, 16, tests[i].usage,
                tests[i].format == FORMAT_COLOUR ? D3DFMT_INDEX32 : D3DFMT_INDEX16, tests[i].pool, &ib, NULL);
        ok(hr == (tests[i].pool == D3DPOOL_SCRATCH || (tests[i].usage & ~D3DUSAGE_DYNAMIC)
                ? D3DERR_INVALIDCALL : D3D_OK), "Test %u: Got unexpected hr %#x.\n", i, hr);
        if (FAILED(hr))
            continue;

        hr = IDirect3DIndexBuffer9_GetDesc(ib, &ib_desc);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        ok(ib_desc.Usage == tests[i].usage, "Test %u: Got unexpected usage %#x.\n", i, ib_desc.Usage);
        ok(ib_desc.Pool == tests[i].pool, "Test %u: Got unexpected pool %#x.\n", i, ib_desc.Pool);

        hr = IDirect3DIndexBuffer9_Lock(ib, 0, 0, &data, 0);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        hr = IDirect3DIndexBuffer9_Unlock(ib);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);

        hr = IDirect3DDevice9_SetIndices(device, ib);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        hr = IDirect3DDevice9_SetIndices(device, NULL);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);

        IDirect3DIndexBuffer9_Release(ib);
    }

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        D3DVERTEXBUFFER_DESC vb_desc;
        IDirect3DVertexBuffer9 *vb;
        void *data;

        hr = IDirect3DDevice9_CreateVertexBuffer(device, 16, tests[i].usage,
                tests[i].format == FORMAT_COLOUR ? 0 : D3DFVF_XYZRHW, tests[i].pool, &vb, NULL);
        ok(hr == (tests[i].pool == D3DPOOL_SCRATCH || (tests[i].usage & ~D3DUSAGE_DYNAMIC)
                ? D3DERR_INVALIDCALL : D3D_OK), "Test %u: Got unexpected hr %#x.\n", i, hr);
        if (FAILED(hr))
            continue;

        hr = IDirect3DVertexBuffer9_GetDesc(vb, &vb_desc);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        ok(vb_desc.Usage == tests[i].usage, "Test %u: Got unexpected usage %#x.\n", i, vb_desc.Usage);
        ok(vb_desc.Pool == tests[i].pool, "Test %u: Got unexpected pool %#x.\n", i, vb_desc.Pool);

        hr = IDirect3DVertexBuffer9_Lock(vb, 0, 0, &data, 0);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        hr = IDirect3DVertexBuffer9_Unlock(vb);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);

        hr = IDirect3DDevice9_SetStreamSource(device, 0, vb, 0, 16);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        hr = IDirect3DDevice9_SetStreamSource(device, 0, NULL, 0, 0);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);

        IDirect3DVertexBuffer9_Release(vb);
    }

    IDirect3DSurface9_Release(depth_stencil);
    IDirect3DSurface9_Release(backbuffer);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_multiply_transform(void)
{
    IDirect3DStateBlock9 *stateblock;
    IDirect3DDevice9 *device;
    D3DMATRIX ret_mat;
    IDirect3D9 *d3d;
    unsigned int i;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    static const D3DTRANSFORMSTATETYPE tests[] =
    {
        D3DTS_VIEW,
        D3DTS_PROJECTION,
        D3DTS_TEXTURE0,
        D3DTS_TEXTURE1,
        D3DTS_TEXTURE2,
        D3DTS_TEXTURE3,
        D3DTS_TEXTURE4,
        D3DTS_TEXTURE5,
        D3DTS_TEXTURE6,
        D3DTS_TEXTURE7,
        D3DTS_WORLDMATRIX(0),
        D3DTS_WORLDMATRIX(1),
        D3DTS_WORLDMATRIX(2),
        D3DTS_WORLDMATRIX(3),
        D3DTS_WORLDMATRIX(255),
    };

    static const D3DMATRIX mat1 =
    {{{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    }}},
    mat2 =
    {{{
        2.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 2.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 2.0f,
    }}};

    window = create_window();
    ok(!!window, "Failed to create a window.\n");
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create D3D object.\n");

    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create 3D device.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        hr = IDirect3DDevice9_GetTransform(device, tests[i], &ret_mat);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        ok(!memcmp(&ret_mat, &mat1, sizeof(mat1)), "Test %u: Got unexpected transform matrix.\n", i);

        hr = IDirect3DDevice9_MultiplyTransform(device, tests[i], &mat2);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);

        hr = IDirect3DDevice9_GetTransform(device, tests[i], &ret_mat);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        ok(!memcmp(&ret_mat, &mat2, sizeof(mat2)), "Test %u: Got unexpected transform matrix.\n", i);

        /* MultiplyTransform() goes directly into the primary stateblock. */

        hr = IDirect3DDevice9_SetTransform(device, tests[i], &mat1);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);

        hr = IDirect3DDevice9_BeginStateBlock(device);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);

        hr = IDirect3DDevice9_MultiplyTransform(device, tests[i], &mat2);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);

        hr = IDirect3DDevice9_EndStateBlock(device, &stateblock);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);

        hr = IDirect3DDevice9_GetTransform(device, tests[i], &ret_mat);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        ok(!memcmp(&ret_mat, &mat2, sizeof(mat2)), "Test %u: Got unexpected transform matrix.\n", i);

        hr = IDirect3DStateBlock9_Capture(stateblock);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);

        hr = IDirect3DDevice9_SetTransform(device, tests[i], &mat1);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);

        hr = IDirect3DStateBlock9_Apply(stateblock);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);

        hr = IDirect3DDevice9_GetTransform(device, tests[i], &ret_mat);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#x.\n", i, hr);
        ok(!memcmp(&ret_mat, &mat1, sizeof(mat1)), "Test %u: Got unexpected transform matrix.\n", i);

        IDirect3DStateBlock9_Release(stateblock);
    }

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_vertex_buffer_read_write(void)
{
    IDirect3DVertexBuffer9 *buffer;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    unsigned int i;
    float *data;
    HWND window;
    HRESULT hr;

    static const struct vec3 tri[] =
    {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f,  1.0f, 0.0f},
        { 1.0f,  1.0f, 0.0f},
    };

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }
    hr = IDirect3DDevice9_CreateVertexBuffer(device, sizeof(tri),
            D3DUSAGE_DYNAMIC, D3DFVF_XYZ, D3DPOOL_DEFAULT, &buffer, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer9_Lock(buffer, 0, sizeof(tri), (void **)&data, D3DLOCK_DISCARD);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    memcpy(data, tri, sizeof(tri));
    hr = IDirect3DVertexBuffer9_Unlock(buffer);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    /* Draw using the buffer to make wined3d create BO. */
    hr = IDirect3DDevice9_SetStreamSource(device, 0, buffer, 0, sizeof(*tri));
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_BeginScene(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitive(device, D3DPT_TRIANGLELIST, 0, 1);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DDevice9_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer9_Lock(buffer, 0, sizeof(tri), (void **)&data, D3DLOCK_NOOVERWRITE);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    for (i = 0; i < 3; ++i)
        data[i] = 3.0f;
    hr = IDirect3DVertexBuffer9_Unlock(buffer);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer9_Lock(buffer, 0, sizeof(tri), (void **)&data, D3DLOCK_NOOVERWRITE);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    for (i = 0; i < 3; ++i)
        ok(data[i] == 3.0f, "Got unexpected value %.8e, i %u.\n", data[i], i);
    hr = IDirect3DVertexBuffer9_Unlock(buffer);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer9_Lock(buffer, 0, sizeof(tri), (void **)&data, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    for (i = 0; i < 3; ++i)
        ok(data[i] == 3.0f, "Got unexpected value %.8e, i %u.\n", data[i], i);
    hr = IDirect3DVertexBuffer9_Unlock(buffer);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer9_Lock(buffer, 0, sizeof(tri), (void **)&data, D3DLOCK_NOOVERWRITE);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    for (i = 0; i < 3; ++i)
        ok(data[i] == 3.0f, "Got unexpected value %.8e, i %u.\n", data[i], i);
    hr = IDirect3DVertexBuffer9_Unlock(buffer);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer9_Lock(buffer, 0, sizeof(tri), (void **)&data, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    for (i = 0; i < 3; ++i)
        data[i] = 4.0f;
    hr = IDirect3DVertexBuffer9_Unlock(buffer);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    hr = IDirect3DVertexBuffer9_Lock(buffer, 0, sizeof(tri), (void **)&data, D3DLOCK_NOOVERWRITE);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    for (i = 0; i < 3; ++i)
        ok(data[i] == 4.0f, "Got unexpected value %.8e, i %u.\n", data[i], i);
    hr = IDirect3DVertexBuffer9_Unlock(buffer);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);

    IDirect3DVertexBuffer9_Release(buffer);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_get_display_mode(void)
{
    static const DWORD creation_flags[] = {0, CREATE_DEVICE_FULLSCREEN};
    unsigned int adapter_idx, adapter_count, mode_idx, test_idx;
    IDirect3DSwapChain9 *swapchain;
    RECT previous_monitor_rect;
    unsigned int width, height;
    IDirect3DDevice9 *device;
    MONITORINFO monitor_info;
    struct device_desc desc;
    D3DDISPLAYMODE mode;
    HMONITOR monitor;
    IDirect3D9 *d3d;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    BOOL ret;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    if (!(device = create_device(d3d, window, NULL)))
    {
        skip("Failed to create a D3D device.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_GetDisplayMode(device, 0, &mode);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(mode.Format == D3DFMT_X8R8G8B8, "Unexpected format %#x.\n", mode.Format);
    hr = IDirect3D9_GetAdapterDisplayMode(d3d, D3DADAPTER_DEFAULT, &mode);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(mode.Format == D3DFMT_X8R8G8B8, "Unexpected format %#x.\n", mode.Format);
    hr = IDirect3DDevice9_GetSwapChain(device, 0, &swapchain);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DSwapChain9_GetDisplayMode(swapchain, &mode);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(mode.Format == D3DFMT_X8R8G8B8, "Unexpected format %#x.\n", mode.Format);
    IDirect3DSwapChain9_Release(swapchain);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);

    desc.adapter_ordinal = D3DADAPTER_DEFAULT;
    desc.device_window = window;
    desc.width = 640;
    desc.height = 480;
    desc.flags = CREATE_DEVICE_FULLSCREEN;
    if (!(device = create_device(d3d, window, &desc)))
    {
        skip("Failed to create a D3D device.\n");
        IDirect3D9_Release(d3d);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_GetDisplayMode(device, 0, &mode);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(mode.Width == 640, "Unexpected width %u.\n", mode.Width);
    ok(mode.Height == 480, "Unexpected width %u.\n", mode.Height);
    ok(mode.Format == D3DFMT_X8R8G8B8, "Unexpected format %#x.\n", mode.Format);
    hr = IDirect3D9_GetAdapterDisplayMode(d3d, D3DADAPTER_DEFAULT, &mode);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(mode.Width == 640, "Unexpected width %u.\n", mode.Width);
    ok(mode.Height == 480, "Unexpected width %u.\n", mode.Height);
    ok(mode.Format == D3DFMT_X8R8G8B8, "Unexpected format %#x.\n", mode.Format);
    hr = IDirect3DDevice9_GetSwapChain(device, 0, &swapchain);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    hr = IDirect3DSwapChain9_GetDisplayMode(swapchain, &mode);
    ok(hr == D3D_OK, "Got unexpected hr %#x.\n", hr);
    ok(mode.Width == 640, "Unexpected width %u.\n", mode.Width);
    ok(mode.Height == 480, "Unexpected width %u.\n", mode.Height);
    ok(mode.Format == D3DFMT_X8R8G8B8, "Unexpected format %#x.\n", mode.Format);
    IDirect3DSwapChain9_Release(swapchain);

    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %u references left.\n", refcount);
    DestroyWindow(window);

    /* D3D9 uses adapter indices to determine which adapter to use to get the display mode */
    adapter_count = IDirect3D9_GetAdapterCount(d3d);
    for (adapter_idx = 0; adapter_idx < adapter_count; ++adapter_idx)
    {
        if (!adapter_idx)
        {
            desc.width = 640;
            desc.height = 480;
        }
        else
        {
            /* Find a mode different than that of the previous adapter, so that tests can be sure
             * that they are comparing to the current adapter display mode */
            monitor = IDirect3D9_GetAdapterMonitor(d3d, adapter_idx - 1);
            ok(!!monitor, "Adapter %u: GetAdapterMonitor failed.\n", adapter_idx - 1);
            monitor_info.cbSize = sizeof(monitor_info);
            ret = GetMonitorInfoW(monitor, &monitor_info);
            ok(ret, "Adapter %u: GetMonitorInfoW failed, error %#x.\n", adapter_idx - 1,
                    GetLastError());
            previous_monitor_rect = monitor_info.rcMonitor;

            desc.width = monitor_info.rcMonitor.right - monitor_info.rcMonitor.left;
            desc.height = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;
            for (mode_idx = 0; SUCCEEDED(IDirect3D9_EnumAdapterModes(d3d, adapter_idx,
                    D3DFMT_X8R8G8B8, mode_idx, &mode)); ++mode_idx)
            {
                if (mode.Width < 640 || mode.Height < 480)
                    continue;
                if (mode.Width != desc.width && mode.Height != desc.height)
                    break;
            }
            ok(mode.Width != desc.width && mode.Height != desc.height,
                    "Adapter %u: Failed to find a different mode than %ux%u.\n", adapter_idx,
                    desc.width, desc.height);
            desc.width = mode.Width;
            desc.height = mode.Height;
        }

        for (test_idx = 0; test_idx < ARRAY_SIZE(creation_flags); ++test_idx)
        {
            window = create_window();
            desc.adapter_ordinal = adapter_idx;
            desc.device_window = window;
            desc.flags = creation_flags[test_idx];
            if (!(device = create_device(d3d, window, &desc)))
            {
                skip("Adapter %u test %u: Failed to create a D3D device.\n", adapter_idx, test_idx);
                DestroyWindow(window);
                continue;
            }

            monitor = IDirect3D9_GetAdapterMonitor(d3d, adapter_idx);
            ok(!!monitor, "Adapter %u test %u: GetAdapterMonitor failed.\n", adapter_idx, test_idx);
            monitor_info.cbSize = sizeof(monitor_info);
            ret = GetMonitorInfoW(monitor, &monitor_info);
            ok(ret, "Adapter %u test %u: GetMonitorInfoW failed, error %#x.\n", adapter_idx,
                    test_idx, GetLastError());
            width = monitor_info.rcMonitor.right - monitor_info.rcMonitor.left;
            height = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;

            if (adapter_idx)
            {
                /* Move the device window to the previous monitor to test that the device window
                 * position doesn't affect which adapter to use to get the display mode */
                ret = SetWindowPos(window, 0, previous_monitor_rect.left, previous_monitor_rect.top,
                        0, 0, SWP_NOZORDER | SWP_NOSIZE);
                ok(ret, "Adapter %u test %u: SetWindowPos failed, error %#x.\n", adapter_idx,
                        test_idx, GetLastError());
            }

            hr = IDirect3D9_GetAdapterDisplayMode(d3d, adapter_idx, &mode);
            ok(hr == D3D_OK, "Adapter %u test %u: GetAdapterDisplayMode failed, hr %#x.\n",
                    adapter_idx, test_idx, hr);
            ok(mode.Width == width, "Adapter %u test %u: Expect width %u, got %u.\n", adapter_idx,
                    test_idx, width, mode.Width);
            ok(mode.Height == height, "Adapter %u test %u: Expect height %u, got %u.\n",
                    adapter_idx, test_idx, height, mode.Height);

            hr = IDirect3DDevice9_GetDisplayMode(device, 0, &mode);
            ok(hr == D3D_OK, "Adapter %u test %u: GetDisplayMode failed, hr %#x.\n", adapter_idx,
                    test_idx, hr);
            ok(mode.Width == width, "Adapter %u test %u: Expect width %u, got %u.\n", adapter_idx,
                    test_idx, width, mode.Width);
            ok(mode.Height == height, "Adapter %u test %u: Expect height %u, got %u.\n",
                    adapter_idx, test_idx, height, mode.Height);

            hr = IDirect3DDevice9_GetSwapChain(device, 0, &swapchain);
            ok(hr == D3D_OK, "Adapter %u test %u: GetSwapChain failed, hr %#x.\n", adapter_idx,
                    test_idx, hr);
            hr = IDirect3DSwapChain9_GetDisplayMode(swapchain, &mode);
            ok(hr == D3D_OK, "Adapter %u test %u: GetDisplayMode failed, hr %#x.\n", adapter_idx,
                    test_idx, hr);
            ok(mode.Width == width, "Adapter %u test %u: Expect width %u, got %u.\n", adapter_idx,
                    test_idx, width, mode.Width);
            ok(mode.Height == height, "Adapter %u test %u: Expect height %u, got %u.\n",
                    adapter_idx, test_idx, height, mode.Height);
            IDirect3DSwapChain9_Release(swapchain);

            refcount = IDirect3DDevice9_Release(device);
            ok(!refcount, "Adapter %u test %u: Device has %u references left.\n", adapter_idx,
                    test_idx, refcount);
            DestroyWindow(window);
        }
    }

    IDirect3D9_Release(d3d);
}

static void test_multi_adapter(void)
{
    unsigned int i, adapter_count, expected_adapter_count = 0;
    DISPLAY_DEVICEA display_device;
    MONITORINFOEXA monitor_info;
    DEVMODEA old_mode, mode;
    HMONITOR monitor;
    IDirect3D9 *d3d;
    LONG ret;

    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    display_device.cb = sizeof(display_device);
    for (i = 0; EnumDisplayDevicesA(NULL, i, &display_device, 0); ++i)
    {
        if (display_device.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
            ++expected_adapter_count;
    }

    adapter_count = IDirect3D9_GetAdapterCount(d3d);
    ok(adapter_count == expected_adapter_count, "Got unexpected adapter count %u, expected %u.\n",
            adapter_count, expected_adapter_count);

    for (i = 0; i < adapter_count; ++i)
    {
        monitor = IDirect3D9_GetAdapterMonitor(d3d, i);
        ok(!!monitor, "Adapter %u: Failed to get monitor.\n", i);

        monitor_info.cbSize = sizeof(monitor_info);
        ret = GetMonitorInfoA(monitor, (MONITORINFO *)&monitor_info);
        ok(ret, "Adapter %u: Failed to get monitor info, error %#x.\n", i, GetLastError());

        if (!i)
            ok(monitor_info.dwFlags == MONITORINFOF_PRIMARY,
                    "Adapter %u: Got unexpected monitor flags %#x.\n", i, monitor_info.dwFlags);
        else
            ok(!monitor_info.dwFlags, "Adapter %u: Got unexpected monitor flags %#x.\n", i,
                    monitor_info.dwFlags);

        /* Test D3D adapters after they got detached */
        if (monitor_info.dwFlags == MONITORINFOF_PRIMARY)
            continue;

        /* Save current display settings */
        memset(&old_mode, 0, sizeof(old_mode));
        old_mode.dmSize = sizeof(old_mode);
        ret = EnumDisplaySettingsA(monitor_info.szDevice, ENUM_CURRENT_SETTINGS, &old_mode);
        /* Win10 TestBots may return FALSE but it's actually successful */
        ok(ret || broken(!ret), "Adapter %u: EnumDisplaySettingsA failed for %s, error %#x.\n", i,
                monitor_info.szDevice, GetLastError());

        /* Detach */
        memset(&mode, 0, sizeof(mode));
        mode.dmSize = sizeof(mode);
        mode.dmFields = DM_POSITION | DM_PELSWIDTH | DM_PELSHEIGHT;
        mode.dmPosition = old_mode.dmPosition;
        ret = ChangeDisplaySettingsExA(monitor_info.szDevice, &mode, NULL,
                CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
        ok(ret == DISP_CHANGE_SUCCESSFUL,
                "Adapter %u: ChangeDisplaySettingsExA %s returned unexpected %d.\n", i,
                monitor_info.szDevice, ret);
        ret = ChangeDisplaySettingsExA(monitor_info.szDevice, NULL, NULL, 0, NULL);
        ok(ret == DISP_CHANGE_SUCCESSFUL,
                "Adapter %u: ChangeDisplaySettingsExA %s returned unexpected %d.\n", i,
                monitor_info.szDevice, ret);

        /* Check if it is really detached */
        memset(&mode, 0, sizeof(mode));
        mode.dmSize = sizeof(mode);
        ret = EnumDisplaySettingsA(monitor_info.szDevice, ENUM_CURRENT_SETTINGS, &mode);
        /* Win10 TestBots may return FALSE but it's actually successful */
        ok(ret || broken(!ret) , "Adapter %u: EnumDisplaySettingsA failed for %s, error %#x.\n", i,
                monitor_info.szDevice, GetLastError());
        if (mode.dmPelsWidth && mode.dmPelsHeight)
        {
            skip("Adapter %u: Failed to detach device %s.\n", i, monitor_info.szDevice);
            continue;
        }

        /* Detaching adapter shouldn't reduce the adapter count */
        expected_adapter_count = adapter_count;
        adapter_count = IDirect3D9_GetAdapterCount(d3d);
        ok(adapter_count == expected_adapter_count,
                "Adapter %u: Got unexpected adapter count %u, expected %u.\n", i, adapter_count,
                expected_adapter_count);

        monitor = IDirect3D9_GetAdapterMonitor(d3d, i);
        ok(!monitor, "Adapter %u: Expect monitor to be NULL.\n", i);

        /* Restore settings */
        ret = ChangeDisplaySettingsExA(monitor_info.szDevice, &old_mode, NULL,
                CDS_UPDATEREGISTRY | CDS_NORESET, NULL);
        ok(ret == DISP_CHANGE_SUCCESSFUL,
                "Adapter %u: ChangeDisplaySettingsExA %s returned unexpected %d.\n", i,
                monitor_info.szDevice, ret);
        ret = ChangeDisplaySettingsExA(monitor_info.szDevice, NULL, NULL, 0, NULL);
        ok(ret == DISP_CHANGE_SUCCESSFUL,
                "Adapter %u: ChangeDisplaySettingsExA %s returned unexpected %d.\n", i,
                monitor_info.szDevice, ret);
    }

    IDirect3D9_Release(d3d);
}

typedef HRESULT (WINAPI *shader_validator_cb)(const char *file, int line,
        DWORD_PTR arg3, DWORD_PTR message_id, const char *message, void *context);

typedef struct IDirect3DShaderValidator9 IDirect3DShaderValidator9;

struct IDirect3DShaderValidator9Vtbl
{
    HRESULT (WINAPI *QueryInterface)(IDirect3DShaderValidator9 *iface, REFIID iid, void **out);
    ULONG (WINAPI *AddRef)(IDirect3DShaderValidator9 *iface);
    ULONG (WINAPI *Release)(IDirect3DShaderValidator9 *iface);
    HRESULT (WINAPI *Begin)(IDirect3DShaderValidator9 *iface,
            shader_validator_cb callback, void *context, DWORD_PTR arg3);
    HRESULT (WINAPI *Instruction)(IDirect3DShaderValidator9 *iface,
            const char *file, int line, const DWORD *tokens, DWORD token_count);
    HRESULT (WINAPI *End)(IDirect3DShaderValidator9 *iface);
};

struct IDirect3DShaderValidator9
{
    const struct IDirect3DShaderValidator9Vtbl *vtbl;
};

#define MAX_VALIDATOR_CB_CALL_COUNT 5

struct test_shader_validator_cb_context
{
    unsigned int call_count;
    const char *file[MAX_VALIDATOR_CB_CALL_COUNT];
    int line[MAX_VALIDATOR_CB_CALL_COUNT];
    DWORD_PTR message_id[MAX_VALIDATOR_CB_CALL_COUNT];
    const char *message[MAX_VALIDATOR_CB_CALL_COUNT];
};

HRESULT WINAPI test_shader_validator_cb(const char *file, int line, DWORD_PTR arg3,
        DWORD_PTR message_id, const char *message, void *context)
{
    if (context)
    {
        struct test_shader_validator_cb_context *c = context;

        c->file[c->call_count] = file;
        c->line[c->call_count] = line;
        c->message_id[c->call_count] = message_id;
        c->message[c->call_count] = message;
        ++c->call_count;
    }
    else
    {
        ok(0, "Unexpected call.\n");
    }
    return S_OK;
}

static void test_shader_validator(void)
{
    static const unsigned int dcl_texcoord_9_9[] = {0x0200001f, 0x80090005, 0x902f0009};   /* dcl_texcoord9_pp v9 */
    static const unsigned int dcl_texcoord_9_10[] = {0x0200001f, 0x80090005, 0x902f000a};  /* dcl_texcoord9_pp v10 */
    static const unsigned int dcl_texcoord_10_9[] = {0x0200001f, 0x800a0005, 0x902f0009};  /* dcl_texcoord10_pp v9 */
    static const unsigned int mov_r2_v9[] = {0x02000001, 0x80220002, 0x90ff0009};          /* mov_pp r2.y, v9.w */
    static const unsigned int mov_r2_v10[] = {0x02000001, 0x80220002, 0x90ff000a};         /* mov_pp r2.y, v10.w */

    static const unsigned int ps_3_0 = D3DPS_VERSION(3, 0);
    static const unsigned int end_token = 0x0000ffff;
    static const char *test_file_name = "test_file";
    static const struct instruction_test
    {
        unsigned int shader_version;
        const unsigned int *instruction;
        unsigned int instruction_length;
        DWORD_PTR message_id;
        const unsigned int *decl;
        unsigned int decl_length;
    }
    instruction_tests[] =
    {
        {D3DPS_VERSION(3, 0), dcl_texcoord_9_9, ARRAY_SIZE(dcl_texcoord_9_9)},
        {D3DPS_VERSION(3, 0), dcl_texcoord_9_10, ARRAY_SIZE(dcl_texcoord_9_10), 0x12c},
        {D3DPS_VERSION(3, 0), dcl_texcoord_10_9, ARRAY_SIZE(dcl_texcoord_10_9)},
        {D3DPS_VERSION(3, 0), mov_r2_v9, ARRAY_SIZE(mov_r2_v9), 0, dcl_texcoord_9_9, ARRAY_SIZE(dcl_texcoord_9_9)},
        {D3DPS_VERSION(3, 0), mov_r2_v10, ARRAY_SIZE(mov_r2_v10), 0x167},
    };

    struct test_shader_validator_cb_context context;
    IDirect3DShaderValidator9 *validator;
    HRESULT expected_hr, hr;
    unsigned int i;
    ULONG refcount;

    validator = Direct3DShaderValidatorCreate9();

    hr = validator->vtbl->Begin(validator, test_shader_validator_cb, NULL, 0);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = validator->vtbl->Instruction(validator, NULL, 0, &simple_vs[0], 1);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = validator->vtbl->Instruction(validator, NULL, 0, &simple_vs[1], 3);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = validator->vtbl->Instruction(validator, NULL, 0, &simple_vs[4], 4);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = validator->vtbl->Instruction(validator, NULL, 0, &simple_vs[8], 4);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = validator->vtbl->Instruction(validator, NULL, 0, &simple_vs[12], 4);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = validator->vtbl->Instruction(validator, NULL, 0, &simple_vs[16], 4);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = validator->vtbl->Instruction(validator, NULL, 0, &simple_vs[20], 1);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = validator->vtbl->End(validator);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    hr = validator->vtbl->Begin(validator, test_shader_validator_cb, &context, 0);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    memset(&context, 0, sizeof(context));
    hr = validator->vtbl->Instruction(validator, test_file_name, 28, &simple_vs[1], 3);
    todo_wine
    {
        ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);
        ok(context.call_count == 2, "Got unexpected call_count %u.\n", context.call_count);
        ok(!!context.message[0], "Got NULL message.\n");
        ok(!!context.message[1], "Got NULL message.\n");
        ok(context.message_id[0] == 0xef, "Got unexpected message_id[0] %p.\n", (void *)context.message_id[0]);
        ok(context.message_id[1] == 0xf0, "Got unexpected message_id[1] %p.\n", (void *)context.message_id[1]);
        ok(context.line[0] == -1, "Got unexpected line %d.\n", context.line[0]);
    }
    ok(!context.file[0], "Got unexpected file[0] %s.\n", context.file[0]);
    ok(!context.file[1], "Got unexpected file[0] %s.\n", context.file[1]);
    ok(!context.line[1], "Got unexpected line %d.\n", context.line[1]);

    memset(&context, 0, sizeof(context));
    hr = validator->vtbl->End(validator);
    todo_wine ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);
    ok(!context.call_count, "Got unexpected call_count %u.\n", context.call_count);

    memset(&context, 0, sizeof(context));
    hr = validator->vtbl->Begin(validator, test_shader_validator_cb, &context, 0);
    todo_wine
    {
        ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);
        ok(context.call_count == 1, "Got unexpected call_count %u.\n", context.call_count);
        ok(context.message_id[0] == 0xeb, "Got unexpected message_id[0] %p.\n", (void *)context.message_id[0]);
        ok(!!context.message[0], "Got NULL message.\n");
    }
    ok(!context.file[0], "Got unexpected file[0] %s.\n", context.file[0]);
    ok(!context.line[0], "Got unexpected line %d.\n", context.line[0]);

    hr = validator->vtbl->Begin(validator, NULL, &context, 0);
    todo_wine ok(hr == E_FAIL, "Got unexpected hr %#x.\n", hr);

    refcount = validator->vtbl->Release(validator);
    todo_wine ok(!refcount, "Validator has %u references left.\n", refcount);
    validator = Direct3DShaderValidatorCreate9();

    hr = validator->vtbl->Begin(validator, NULL, &context, 0);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = validator->vtbl->Instruction(validator, test_file_name, 1, &ps_3_0, 1);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = validator->vtbl->Instruction(validator, test_file_name, 5, &end_token, 1);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);
    hr = validator->vtbl->End(validator);
    ok(hr == S_OK, "Got unexpected hr %#x.\n", hr);

    for (i = 0; i < ARRAY_SIZE(instruction_tests); ++i)
    {
        const struct instruction_test *test = &instruction_tests[i];

        hr = validator->vtbl->Begin(validator, test_shader_validator_cb, &context, 0);
        ok(hr == S_OK, "Got unexpected hr %#x, test %u.\n", hr, i);

        hr = validator->vtbl->Instruction(validator, test_file_name, 1, &test->shader_version, 1);
        ok(hr == S_OK, "Got unexpected hr %#x, test %u.\n", hr, i);

        if (test->decl)
        {
            memset(&context, 0, sizeof(context));
            hr = validator->vtbl->Instruction(validator, test_file_name, 3, test->decl, test->decl_length);
            ok(hr == S_OK, "Got unexpected hr %#x, test %u.\n", hr, i);
            ok(!context.call_count, "Got unexpected call_count %u, test %u.\n", context.call_count, i);
        }

        memset(&context, 0, sizeof(context));
        hr = validator->vtbl->Instruction(validator, test_file_name, 3, test->instruction, test->instruction_length);
        ok(hr == S_OK, "Got unexpected hr %#x, test %u.\n", hr, i);
        if (test->message_id)
        {
            todo_wine
            {
                ok(context.call_count == 1, "Got unexpected call_count %u, test %u.\n", context.call_count, i);
                ok(!!context.message[0], "Got NULL message, test %u.\n", i);
                ok(context.message_id[0] == test->message_id, "Got unexpected message_id[0] %p, test %u.\n",
                        (void *)context.message_id[0], i);
                ok(context.file[0] == test_file_name, "Got unexpected file[0] %s, test %u.\n", context.file[0], i);
                ok(context.line[0] == 3, "Got unexpected line %d, test %u.\n", context.line[0], i);
            }
        }
        else
        {
            ok(!context.call_count, "Got unexpected call_count %u, test %u.\n", context.call_count, i);
        }

        hr = validator->vtbl->Instruction(validator, test_file_name, 5, &end_token, 1);
        ok(hr == S_OK, "Got unexpected hr %#x, test %u.\n", hr, i);

        hr = validator->vtbl->End(validator);
        expected_hr = test->message_id ? E_FAIL : S_OK;
        todo_wine_if(expected_hr) ok(hr == expected_hr, "Got unexpected hr %#x, test %u.\n", hr, i);
    }

    refcount = validator->vtbl->Release(validator);
    todo_wine ok(!refcount, "Validator has %u references left.\n", refcount);
}

static void test_creation_parameters(void)
{
    unsigned int adapter_idx, adapter_count;
    D3DDEVICE_CREATION_PARAMETERS params;
    struct device_desc device_desc;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    HWND window;
    HRESULT hr;

    window = create_window();
    ok(!!window, "Failed to create a window.\n");
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    device_desc.device_window = window;
    device_desc.width = 640;
    device_desc.height = 480;
    device_desc.flags = 0;

    adapter_count = IDirect3D9_GetAdapterCount(d3d);
    for (adapter_idx = 0; adapter_idx < adapter_count; ++adapter_idx)
    {
        device_desc.adapter_ordinal = adapter_idx;
        if (!(device = create_device(d3d, window, &device_desc)))
        {
            skip("Adapter %u: Failed to create a D3D device.\n", adapter_idx);
            break;
        }

        memset(&params, 0, sizeof(params));
        hr = IDirect3DDevice9_GetCreationParameters(device, &params);
        ok(hr == D3D_OK, "Adapter %u: GetCreationParameters failed, hr %#x.\n", adapter_idx, hr);
        ok(params.AdapterOrdinal == adapter_idx, "Adapter %u: Got unexpected adapter ordinal %u.\n",
                adapter_idx, params.AdapterOrdinal);
        ok(params.DeviceType == D3DDEVTYPE_HAL, "Adapter %u: Expect device type %#x, got %#x.\n",
                adapter_idx, D3DDEVTYPE_HAL, params.DeviceType);
        ok(params.hFocusWindow == window, "Adapter %u: Expect focus window %p, got %p.\n",
                adapter_idx, window, params.hFocusWindow);

        IDirect3DDevice9_Release(device);
    }

    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_cursor_clipping(void)
{
    unsigned int adapter_idx, adapter_count, mode_idx;
    D3DDISPLAYMODE mode, current_mode;
    struct device_desc device_desc;
    RECT virtual_rect, clip_rect;
    IDirect3DDevice9 *device;
    IDirect3D9 *d3d;
    HWND window;
    HRESULT hr;
    BOOL ret;

    window = create_window();
    ok(!!window, "Failed to create a window.\n");
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    device_desc.device_window = window;
    device_desc.flags = CREATE_DEVICE_FULLSCREEN;

    adapter_count = IDirect3D9_GetAdapterCount(d3d);
    for (adapter_idx = 0; adapter_idx < adapter_count; ++adapter_idx)
    {
        hr = IDirect3D9_GetAdapterDisplayMode(d3d, adapter_idx, &current_mode);
        ok(hr == D3D_OK, "Adapter %u: GetAdapterDisplayMode failed, hr %#x.\n", adapter_idx, hr);
        for (mode_idx = 0; SUCCEEDED(IDirect3D9_EnumAdapterModes(d3d, adapter_idx, D3DFMT_X8R8G8B8,
                mode_idx, &mode)); ++mode_idx)
        {
            if (mode.Width < 640 || mode.Height < 480)
                continue;
            if (mode.Width != current_mode.Width && mode.Height != current_mode.Height)
                break;
        }
        ok(mode.Width != current_mode.Width && mode.Height != current_mode.Height,
                "Adapter %u: Failed to find a different mode than %ux%u.\n", adapter_idx,
                current_mode.Width, current_mode.Height);

        ret = ClipCursor(NULL);
        ok(ret, "Adapter %u: ClipCursor failed, error %#x.\n", adapter_idx,
                GetLastError());
        get_virtual_rect(&virtual_rect);
        ret = GetClipCursor(&clip_rect);
        ok(ret, "Adapter %u: GetClipCursor failed, error %#x.\n", adapter_idx,
                GetLastError());
        ok(EqualRect(&clip_rect, &virtual_rect), "Adapter %u: Expect clip rect %s, got %s.\n",
                adapter_idx, wine_dbgstr_rect(&virtual_rect), wine_dbgstr_rect(&clip_rect));

        device_desc.adapter_ordinal = adapter_idx;
        device_desc.width = mode.Width;
        device_desc.height = mode.Height;
        if (!(device = create_device(d3d, window, &device_desc)))
        {
            skip("Adapter %u: Failed to create a D3D device.\n", adapter_idx);
            break;
        }
        flush_events();
        get_virtual_rect(&virtual_rect);
        ret = GetClipCursor(&clip_rect);
        ok(ret, "Adapter %u: GetClipCursor failed, error %#x.\n", adapter_idx,
                GetLastError());
        ok(EqualRect(&clip_rect, &virtual_rect), "Adapter %u: Expect clip rect %s, got %s.\n",
                adapter_idx, wine_dbgstr_rect(&virtual_rect), wine_dbgstr_rect(&clip_rect));

        IDirect3DDevice9_Release(device);
        flush_events();
        get_virtual_rect(&virtual_rect);
        ret = GetClipCursor(&clip_rect);
        ok(ret, "Adapter %u: GetClipCursor failed, error %#x.\n", adapter_idx,
                GetLastError());
        ok(EqualRect(&clip_rect, &virtual_rect), "Adapter %u: Expect clip rect %s, got %s.\n",
                adapter_idx, wine_dbgstr_rect(&virtual_rect), wine_dbgstr_rect(&clip_rect));
    }

    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_window_position(void)
{
    unsigned int adapter_idx, adapter_count;
    struct device_desc device_desc;
    IDirect3DDevice9 *device;
    MONITORINFO monitor_info;
    HMONITOR monitor;
    RECT window_rect;
    IDirect3D9 *d3d;
    HWND window;
    HRESULT hr;
    BOOL ret;

    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");

    adapter_count = IDirect3D9_GetAdapterCount(d3d);
    for (adapter_idx = 0; adapter_idx < adapter_count; ++adapter_idx)
    {
        monitor = IDirect3D9_GetAdapterMonitor(d3d, adapter_idx);
        ok(!!monitor, "Adapter %u: GetAdapterMonitor failed.\n", adapter_idx);
        monitor_info.cbSize = sizeof(monitor_info);
        ret = GetMonitorInfoW(monitor, &monitor_info);
        ok(ret, "Adapter %u: GetMonitorInfoW failed, error %#x.\n", adapter_idx, GetLastError());

        window = create_window();
        device_desc.adapter_ordinal = adapter_idx;
        device_desc.device_window = window;
        device_desc.width = monitor_info.rcMonitor.right - monitor_info.rcMonitor.left;
        device_desc.height = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;
        device_desc.flags = CREATE_DEVICE_FULLSCREEN;
        if (!(device = create_device(d3d, window, &device_desc)))
        {
            skip("Adapter %u: Failed to create a D3D device, skipping tests.\n", adapter_idx);
            DestroyWindow(window);
            continue;
        }
        flush_events();
        ret = GetWindowRect(window, &window_rect);
        ok(ret, "Adapter %u: GetWindowRect failed, error %#x.\n", adapter_idx, GetLastError());
        ok(EqualRect(&window_rect, &monitor_info.rcMonitor),
                "Adapter %u: Expect window rect %s, got %s.\n", adapter_idx,
                wine_dbgstr_rect(&monitor_info.rcMonitor), wine_dbgstr_rect(&window_rect));

        /* Device resets should restore the window rectangle to fit the whole monitor */
        ret = SetWindowPos(window, 0, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
        ok(ret, "Adapter %u: SetWindowPos failed, error %#x.\n", adapter_idx, GetLastError());
        hr = reset_device(device, &device_desc);
        ok(hr == D3D_OK, "Adapter %u: Failed to reset device, hr %#x.\n", adapter_idx, hr);
        flush_events();
        ret = GetWindowRect(window, &window_rect);
        ok(ret, "Adapter %u: GetWindowRect failed, error %#x.\n", adapter_idx, GetLastError());
        ok(EqualRect(&window_rect, &monitor_info.rcMonitor),
                "Adapter %u: Expect window rect %s, got %s.\n", adapter_idx,
                wine_dbgstr_rect(&monitor_info.rcMonitor), wine_dbgstr_rect(&window_rect));

        /* Window activation should restore the window rectangle to fit the whole monitor */
        ret = SetWindowPos(window, 0, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
        ok(ret, "Adapter %u: SetWindowPos failed, error %#x.\n", adapter_idx, GetLastError());
        ret = SetForegroundWindow(GetDesktopWindow());
        ok(ret, "Adapter %u: SetForegroundWindow failed, error %#x.\n", adapter_idx, GetLastError());
        flush_events();
        ret = ShowWindow(window, SW_RESTORE);
        ok(ret, "Adapter %u: Failed to restore window, error %#x.\n", adapter_idx, GetLastError());
        flush_events();
        ret = SetForegroundWindow(window);
        ok(ret, "Adapter %u: SetForegroundWindow failed, error %#x.\n", adapter_idx, GetLastError());
        flush_events();
        ret = GetWindowRect(window, &window_rect);
        ok(ret, "Adapter %u: GetWindowRect failed, error %#x.\n", adapter_idx, GetLastError());
        ok(EqualRect(&window_rect, &monitor_info.rcMonitor),
                "Adapter %u: Expect window rect %s, got %s.\n", adapter_idx,
                wine_dbgstr_rect(&monitor_info.rcMonitor), wine_dbgstr_rect(&window_rect));

        IDirect3DDevice9_Release(device);
        DestroyWindow(window);
    }

    IDirect3D9_Release(d3d);
}

START_TEST(device)
{
    HMODULE d3d9_handle = GetModuleHandleA("d3d9.dll");
    WNDCLASSA wc = {0};
    IDirect3D9 *d3d9;
    DEVMODEW current_mode;

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

    if (!(d3d9 = Direct3DCreate9(D3D_SDK_VERSION)))
    {
        skip("could not create D3D9 object\n");
        return;
    }
    IDirect3D9_Release(d3d9);

    wc.lpfnWndProc = DefWindowProcA;
    wc.lpszClassName = "d3d9_test_wc";
    RegisterClassA(&wc);

    Direct3DShaderValidatorCreate9 = (void *)GetProcAddress(d3d9_handle, "Direct3DShaderValidatorCreate9");

    test_get_set_vertex_declaration();
    test_get_declaration();
    test_fvf_decl_conversion();
    test_fvf_decl_management();
    test_vertex_declaration_alignment();
    test_unused_declaration_type();
    test_fpu_setup();
    test_multi_device();
    test_display_formats();
    test_display_modes();
    test_swapchain();
    test_refcount();
    test_mipmap_levels();
    test_checkdevicemultisampletype();
    test_invalid_multisample();
    test_cursor();
    test_cursor_pos();
    test_reset_fullscreen();
    test_reset();
    test_scene();
    test_limits();
    test_depthstenciltest();
    test_get_rt();
    test_draw_primitive();
    test_null_stream();
    test_lights();
    test_set_stream_source();
    test_scissor_size();
    test_wndproc();
    test_wndproc_windowed();
    test_window_style();
    test_mode_change();
    test_device_window_reset();
    test_reset_resources();
    test_set_rt_vp_scissor();
    test_volume_get_container();
    test_volume_resource();
    test_vb_lock_flags();
    test_vertex_buffer_alignment();
    test_query_support();
    test_occlusion_query();
    test_timestamp_query();
    test_get_set_vertex_shader();
    test_vertex_shader_constant();
    test_get_set_pixel_shader();
    test_pixel_shader_constant();
    test_unsupported_shaders();
    test_texture_stage_states();
    test_cube_textures();
    test_mipmap_gen();
    test_filter();
    test_get_set_texture();
    test_lod();
    test_surface_get_container();
    test_surface_alignment();
    test_lockrect_offset();
    test_lockrect_invalid();
    test_private_data();
    test_getdc();
    test_surface_dimensions();
    test_surface_format_null();
    test_surface_double_unlock();
    test_surface_blocks();
    test_set_palette();
    test_pinned_buffers();
    test_npot_textures();
    test_vidmem_accounting();
    test_volume_locking();
    test_update_volumetexture();
    test_create_rt_ds_fail();
    test_volume_blocks();
    test_lockbox_invalid();
    test_shared_handle();
    test_pixel_format();
    test_begin_end_state_block();
    test_shader_constant_apply();
    test_vdecl_apply();
    test_resource_type();
    test_mipmap_lock();
    test_writeonly_resource();
    test_lost_device();
    test_resource_priority();
    test_swapchain_parameters();
    test_check_device_format();
    test_miptree_layout();
    test_get_render_target_data();
    test_render_target_device_mismatch();
    test_format_unknown();
    test_destroyed_window();
    test_lockable_backbuffer();
    test_clip_planes_limits();
    test_swapchain_multisample_reset();
    test_stretch_rect();
    test_device_caps();
    test_resource_access();
    test_multiply_transform();
    test_vertex_buffer_read_write();
    test_get_display_mode();
    test_multi_adapter();
    test_shader_validator();
    test_creation_parameters();
    test_cursor_clipping();
    test_window_position();

    UnregisterClassA("d3d9_test_wc", GetModuleHandleA(NULL));
}
