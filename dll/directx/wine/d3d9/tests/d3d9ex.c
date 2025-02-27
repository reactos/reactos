/*
 * Copyright (C) 2008 Stefan Dösinger(for CodeWeavers)
 * Copyright (C) 2010 Louis Lenders
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

/* This file contains tests specific to IDirect3D9Ex and IDirect3DDevice9Ex, like
 * how to obtain them. For testing rendering with extended functions use visual.c
 */

#define COBJMACROS
#include "wine/test.h"
#include <initguid.h>
#include <d3d9.h>

static HMODULE d3d9_handle = 0;
static DEVMODEW registry_mode;

static HRESULT (WINAPI *pDirect3DCreate9Ex)(UINT SDKVersion, IDirect3D9Ex **d3d9ex);

#define CREATE_DEVICE_FULLSCREEN        0x01
#define CREATE_DEVICE_NOWINDOWCHANGES   0x02
#define CREATE_DEVICE_SWVP_ONLY         0x04

struct device_desc
{
    HWND device_window;
    unsigned int width;
    unsigned int height;
    DWORD flags;
};

static BOOL adapter_is_warp(const D3DADAPTER_IDENTIFIER9 *identifier)
{
    return !strcmp(identifier->Driver, "d3d10warp.dll");
}

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

static DWORD get_pixel_color(IDirect3DDevice9Ex *device, unsigned int x, unsigned int y)
{
    IDirect3DSurface9 *surf = NULL, *target = NULL;
    RECT rect = {x, y, x + 1, y + 1};
    D3DLOCKED_RECT locked_rect;
    D3DSURFACE_DESC desc;
    HRESULT hr;
    DWORD ret;

    hr = IDirect3DDevice9Ex_GetRenderTarget(device, 0, &target);
    if (FAILED(hr))
    {
        trace("Can't get the render target, hr %#lx.\n", hr);
        return 0xdeadbeed;
    }

    hr = IDirect3DSurface9_GetDesc(target, &desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurface(device, desc.Width, desc.Height,
            desc.Format, D3DPOOL_SYSTEMMEM, &surf, NULL);
    if (FAILED(hr) || !surf)
    {
        trace("Can't create an offscreen plain surface to read the render target data, hr %#lx.\n", hr);
        ret = 0xdeadbeef;
        goto out;
    }

    hr = IDirect3DDevice9Ex_GetRenderTargetData(device, target, surf);
    if (FAILED(hr))
    {
        trace("Can't read the render target data, hr %#lx.\n", hr);
        ret = 0xdeadbeec;
        goto out;
    }

    hr = IDirect3DSurface9_LockRect(surf, &locked_rect, &rect, D3DLOCK_READONLY);
    if (FAILED(hr))
    {
        trace("Can't lock the offscreen surface, hr %#lx.\n", hr);
        ret = 0xdeadbeeb;
        goto out;
    }

    /* Remove the X channel for now. DirectX and OpenGL have different
     * ideas how to treat it apparently, and it isn't really important
     * for these tests. */
    ret = ((DWORD *)locked_rect.pBits)[0] & 0x00ffffff;
    hr = IDirect3DSurface9_UnlockRect(surf);
    if (FAILED(hr))
        trace("Can't unlock the offscreen surface, hr %#lx.\n", hr);

out:
    if (target)
        IDirect3DSurface9_Release(target);
    if (surf)
        IDirect3DSurface9_Release(surf);
    return ret;
}

static HWND create_window(void)
{
    return CreateWindowA("d3d9_test_wc", "d3d9_test", WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION,
            0, 0, 640, 480, 0, 0, 0, 0);
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
        if (MsgWaitForMultipleObjects(0, NULL, FALSE, min_timeout, QS_ALLINPUT) == WAIT_TIMEOUT)
            break;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
            DispatchMessageA(&msg);
        diff = time - GetTickCount();
    }
}

static IDirect3DDevice9Ex *create_device(HWND focus_window, const struct device_desc *desc)
{
    D3DPRESENT_PARAMETERS present_parameters = {0};
    IDirect3DDevice9Ex *device;
    D3DDISPLAYMODEEX mode, *m;
    IDirect3D9Ex *d3d9;
    DWORD behavior_flags = D3DCREATE_HARDWARE_VERTEXPROCESSING;

    if (FAILED(pDirect3DCreate9Ex(D3D_SDK_VERSION, &d3d9)))
        return NULL;

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
        present_parameters.BackBufferWidth = desc->width;
        present_parameters.BackBufferHeight = desc->height;
        present_parameters.hDeviceWindow = desc->device_window;
        present_parameters.Windowed = !(desc->flags & CREATE_DEVICE_FULLSCREEN);
        if (desc->flags & CREATE_DEVICE_NOWINDOWCHANGES)
            behavior_flags |= D3DCREATE_NOWINDOWCHANGES;
        if (desc->flags & CREATE_DEVICE_SWVP_ONLY)
            behavior_flags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    }

    mode.Size = sizeof(mode);
    mode.Width = present_parameters.BackBufferWidth;
    mode.Height = present_parameters.BackBufferHeight;
    mode.RefreshRate = 0;
    mode.Format = D3DFMT_A8R8G8B8;
    mode.ScanLineOrdering = 0;

    m = present_parameters.Windowed ? NULL : &mode;
    if (SUCCEEDED(IDirect3D9Ex_CreateDeviceEx(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, focus_window,
            behavior_flags, &present_parameters, m, &device)))
        goto done;

    present_parameters.AutoDepthStencilFormat = D3DFMT_D16;
    if (SUCCEEDED(IDirect3D9Ex_CreateDeviceEx(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, focus_window,
            behavior_flags, &present_parameters, m, &device)))
        goto done;

    behavior_flags ^= (D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_SOFTWARE_VERTEXPROCESSING);

    if (SUCCEEDED(IDirect3D9Ex_CreateDeviceEx(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, focus_window,
            behavior_flags, &present_parameters, m, &device)))
        goto done;

    device = NULL;

done:
    IDirect3D9Ex_Release(d3d9);
    return device;
}

static HRESULT reset_device(IDirect3DDevice9Ex *device, const struct device_desc *desc)
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

static ULONG getref(IUnknown *obj) {
    IUnknown_AddRef(obj);
    return IUnknown_Release(obj);
}

static void test_qi_base_to_ex(void)
{
    IDirect3D9 *d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    IDirect3D9Ex *d3d9ex = (void *) 0xdeadbeef;
    IDirect3DDevice9 *device;
    IDirect3DDevice9Ex *deviceEx = (void *) 0xdeadbeef;
    IDirect3DSwapChain9 *swapchain = NULL;
    IDirect3DSwapChain9Ex *swapchainEx = (void *)0xdeadbeef;
    HRESULT hr;
    HWND window = create_window();
    D3DPRESENT_PARAMETERS present_parameters;

    if (!d3d9)
    {
        skip("Direct3D9 is not available\n");
        return;
    }

    hr = IDirect3D9_QueryInterface(d3d9, &IID_IDirect3D9Ex, (void **) &d3d9ex);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(d3d9ex == NULL, "QueryInterface returned interface %p, expected NULL\n", d3d9ex);
    if(d3d9ex) IDirect3D9Ex_Release(d3d9ex);

    memset(&present_parameters, 0, sizeof(present_parameters));
    present_parameters.Windowed = TRUE;
    present_parameters.hDeviceWindow = window;
    present_parameters.SwapEffect = D3DSWAPEFFECT_COPY;
    present_parameters.BackBufferWidth = 640;
    present_parameters.BackBufferHeight = 480;
    present_parameters.EnableAutoDepthStencil = FALSE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D16;
    hr = IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, present_parameters.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device);
    if(FAILED(hr)) {
        skip("Failed to create a regular Direct3DDevice9, skipping QI tests\n");
        goto out;
    }

    hr = IDirect3DDevice9_QueryInterface(device, &IID_IDirect3DDevice9Ex, (void **) &deviceEx);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    ok(deviceEx == NULL, "QueryInterface returned interface %p, expected NULL\n", deviceEx);
    if(deviceEx) IDirect3DDevice9Ex_Release(deviceEx);

    /* Get the implicit swapchain */
    hr = IDirect3DDevice9_GetSwapChain(device, 0, &swapchain);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IDirect3DSwapChain9_QueryInterface(swapchain, &IID_IDirect3DSwapChain9Ex, (void **)&swapchainEx);
        ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
        ok(swapchainEx == NULL, "QueryInterface returned interface %p, expected NULL.\n", swapchainEx);
        if (swapchainEx)
            IDirect3DSwapChain9Ex_Release(swapchainEx);
    }
    if (swapchain)
        IDirect3DSwapChain9_Release(swapchain);

    IDirect3DDevice9_Release(device);

out:
    IDirect3D9_Release(d3d9);
    DestroyWindow(window);
}

static void test_qi_ex_to_base(void)
{
    IDirect3D9 *d3d9 = (void *) 0xdeadbeef;
    IDirect3D9Ex *d3d9ex;
    IDirect3DDevice9 *device;
    IDirect3DDevice9Ex *deviceEx = (void *) 0xdeadbeef;
    IDirect3DSwapChain9 *swapchain = NULL;
    IDirect3DSwapChain9Ex *swapchainEx = (void *)0xdeadbeef;
    HRESULT hr;
    HWND window = create_window();
    D3DPRESENT_PARAMETERS present_parameters;
    ULONG ref;

    hr = pDirect3DCreate9Ex(D3D_SDK_VERSION, &d3d9ex);
    ok(hr == D3D_OK || hr == D3DERR_NOTAVAILABLE, "Got hr %#lx.\n", hr);
    if(FAILED(hr)) {
        skip("Direct3D9Ex is not available\n");
        goto out;
    }

    hr = IDirect3D9Ex_QueryInterface(d3d9ex, &IID_IDirect3D9, (void **) &d3d9);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(d3d9 != NULL && d3d9 != (void *) 0xdeadbeef,
       "QueryInterface returned interface %p, expected != NULL && != 0xdeadbeef\n", d3d9);
    ref = getref((IUnknown *) d3d9ex);
    ok(ref == 2, "Unexpected refcount %lu.\n", ref);
    ref = getref((IUnknown *) d3d9);
    ok(ref == 2, "Unexpected refcount %lu.\n", ref);

    memset(&present_parameters, 0, sizeof(present_parameters));
    present_parameters.Windowed = TRUE;
    present_parameters.hDeviceWindow = window;
    present_parameters.SwapEffect = D3DSWAPEFFECT_COPY;
    present_parameters.BackBufferWidth = 640;
    present_parameters.BackBufferHeight = 480;
    present_parameters.EnableAutoDepthStencil = FALSE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D16;

    /* First, try to create a normal device with IDirect3D9Ex::CreateDevice and QI it for IDirect3DDevice9Ex */
    hr = IDirect3D9Ex_CreateDevice(d3d9ex, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, present_parameters.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device);
    if(FAILED(hr)) {
        skip("Failed to create a regular Direct3DDevice9, skipping QI tests\n");
        goto out;
    }

    hr = IDirect3DDevice9_QueryInterface(device, &IID_IDirect3DDevice9Ex, (void **) &deviceEx);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    ok(deviceEx != NULL && deviceEx != (void *) 0xdeadbeef,
       "QueryInterface returned interface %p, expected != NULL && != 0xdeadbeef\n", deviceEx);
    ref = getref((IUnknown *) device);
    ok(ref == 2, "Unexpected refcount %lu.\n", ref);
    ref = getref((IUnknown *) deviceEx);
    ok(ref == 2, "Unexpected refcount %lu.\n", ref);
    if(deviceEx) IDirect3DDevice9Ex_Release(deviceEx);
    IDirect3DDevice9_Release(device);

    /* Next, try to create a normal device with IDirect3D9::CreateDevice(non-ex) and QI it */
    hr = IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, present_parameters.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters, &device);
    if(FAILED(hr)) {
        skip("Failed to create a regular Direct3DDevice9, skipping QI tests\n");
        goto out;
    }

    hr = IDirect3DDevice9_QueryInterface(device, &IID_IDirect3DDevice9Ex, (void **) &deviceEx);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    ok(deviceEx != NULL && deviceEx != (void *) 0xdeadbeef,
       "QueryInterface returned interface %p, expected != NULL && != 0xdeadbeef\n", deviceEx);
    ref = getref((IUnknown *) device);
    ok(ref == 2, "Unexpected refcount %lu.\n", ref);
    ref = getref((IUnknown *) deviceEx);
    ok(ref == 2, "Unexpected refcount %lu.\n", ref);

    /* Get the implicit swapchain */
    hr = IDirect3DDevice9_GetSwapChain(device, 0, &swapchain);
    ok(SUCCEEDED(hr), "Got hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IDirect3DSwapChain9_QueryInterface(swapchain, &IID_IDirect3DSwapChain9Ex, (void **)&swapchainEx);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
        ok(swapchainEx != NULL && swapchainEx != (void *)0xdeadbeef,
                "QueryInterface returned interface %p, expected != NULL && != 0xdeadbeef.\n", swapchainEx);
        if (swapchainEx)
            IDirect3DSwapChain9Ex_Release(swapchainEx);
    }
    if (swapchain)
        IDirect3DSwapChain9_Release(swapchain);

    if(deviceEx) IDirect3DDevice9Ex_Release(deviceEx);
    IDirect3DDevice9_Release(device);

    IDirect3D9_Release(d3d9);
    IDirect3D9Ex_Release(d3d9ex);

out:
    DestroyWindow(window);
}

static void test_get_adapter_luid(void)
{
    HWND window = create_window();
    IDirect3D9Ex *d3d9ex;
    UINT count;
    HRESULT hr;
    LUID luid;

    hr = pDirect3DCreate9Ex(D3D_SDK_VERSION, &d3d9ex);
    if (FAILED(hr))
    {
        skip("Direct3D9Ex is not available.\n");
        DestroyWindow(window);
        return;
    }

    count = IDirect3D9Ex_GetAdapterCount(d3d9ex);
    if (!count)
    {
        skip("No adapters available.\n");
        IDirect3D9Ex_Release(d3d9ex);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3D9Ex_GetAdapterLUID(d3d9ex, D3DADAPTER_DEFAULT, &luid);
    ok(SUCCEEDED(hr), "GetAdapterLUID failed, hr %#lx.\n", hr);
    trace("Adapter luid: %08lx:%08lx.\n", luid.HighPart, luid.LowPart);

    IDirect3D9Ex_Release(d3d9ex);
}

static void test_swapchain_get_displaymode_ex(void)
{
    IDirect3DSwapChain9 *swapchain = NULL;
    IDirect3DSwapChain9Ex *swapchainEx = NULL;
    IDirect3DDevice9Ex *device;
    D3DDISPLAYMODE mode;
    D3DDISPLAYMODEEX mode_ex;
    D3DDISPLAYROTATION rotation;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(window, NULL)))
    {
        skip("Failed to create a D3D device, skipping swapchain GetDisplayModeEx tests.\n");
        goto out;
    }

    /* Get the implicit swapchain */
    hr = IDirect3DDevice9Ex_GetSwapChain(device, 0, &swapchain);
    if (FAILED(hr))
    {
        skip("Failed to get the implicit swapchain, skipping swapchain GetDisplayModeEx tests.\n");
        goto out;
    }

    hr = IDirect3DSwapChain9_QueryInterface(swapchain, &IID_IDirect3DSwapChain9Ex, (void **)&swapchainEx);
    IDirect3DSwapChain9_Release(swapchain);
    if (FAILED(hr))
    {
        skip("Failed to QI for IID_IDirect3DSwapChain9Ex, skipping swapchain GetDisplayModeEx tests.\n");
        goto out;
    }

    /* invalid size */
    memset(&mode_ex, 0, sizeof(mode_ex));
    hr = IDirect3DSwapChain9Ex_GetDisplayModeEx(swapchainEx, &mode_ex, &rotation);
    ok(hr == D3DERR_INVALIDCALL, "Got hr %#lx.\n", hr);

    mode_ex.Size = sizeof(D3DDISPLAYMODEEX);
    rotation = (D3DDISPLAYROTATION)0xdeadbeef;
    /* valid count and valid size */
    hr = IDirect3DSwapChain9Ex_GetDisplayModeEx(swapchainEx, &mode_ex, &rotation);
    ok(SUCCEEDED(hr), "GetDisplayModeEx failed, hr %#lx.\n", hr);

    /* compare what GetDisplayMode returns with what GetDisplayModeEx returns */
    hr = IDirect3DSwapChain9Ex_GetDisplayMode(swapchainEx, &mode);
    ok(SUCCEEDED(hr), "GetDisplayMode failed, hr %#lx.\n", hr);

    ok(mode_ex.Size == sizeof(D3DDISPLAYMODEEX), "Size is %d.\n", mode_ex.Size);
    ok(mode_ex.Width == mode.Width, "Width is %d instead of %d.\n", mode_ex.Width, mode.Width);
    ok(mode_ex.Height == mode.Height, "Height is %d instead of %d.\n", mode_ex.Height, mode.Height);
    ok(mode_ex.RefreshRate == mode.RefreshRate, "RefreshRate is %d instead of %d.\n",
            mode_ex.RefreshRate, mode.RefreshRate);
    ok(mode_ex.Format == mode.Format, "Format is %x instead of %x.\n", mode_ex.Format, mode.Format);
    /* Don't know yet how to test for ScanLineOrdering, just testing that it
     * is set to a value by GetDisplayModeEx(). */
    ok(mode_ex.ScanLineOrdering != 0, "ScanLineOrdering returned 0.\n");
    /* Don't know how to compare the rotation in this case, test that it is set */
    ok(rotation != (D3DDISPLAYROTATION)0xdeadbeef, "rotation is %d, expected != 0xdeadbeef.\n", rotation);

    trace("GetDisplayModeEx returned Width = %d, Height = %d, RefreshRate = %d, Format = %x, ScanLineOrdering = %x, rotation = %d.\n",
          mode_ex.Width, mode_ex.Height, mode_ex.RefreshRate, mode_ex.Format, mode_ex.ScanLineOrdering, rotation);

    /* test GetDisplayModeEx with null pointer for D3DDISPLAYROTATION */
    memset(&mode_ex, 0, sizeof(mode_ex));
    mode_ex.Size = sizeof(D3DDISPLAYMODEEX);

    hr = IDirect3DSwapChain9Ex_GetDisplayModeEx(swapchainEx, &mode_ex, NULL);
    ok(SUCCEEDED(hr), "GetDisplayModeEx failed, hr %#lx.\n", hr);

    ok(mode_ex.Size == sizeof(D3DDISPLAYMODEEX), "Size is %d.\n", mode_ex.Size);
    ok(mode_ex.Width == mode.Width, "Width is %d instead of %d.\n", mode_ex.Width, mode.Width);
    ok(mode_ex.Height == mode.Height, "Height is %d instead of %d.\n", mode_ex.Height, mode.Height);
    ok(mode_ex.RefreshRate == mode.RefreshRate, "RefreshRate is %d instead of %d.\n",
            mode_ex.RefreshRate, mode.RefreshRate);
    ok(mode_ex.Format == mode.Format, "Format is %x instead of %x.\n", mode_ex.Format, mode.Format);
    /* Don't know yet how to test for ScanLineOrdering, just testing that it
     * is set to a value by GetDisplayModeEx(). */
    ok(mode_ex.ScanLineOrdering != 0, "ScanLineOrdering returned 0.\n");

    IDirect3DSwapChain9Ex_Release(swapchainEx);

out:
    if (device)
        IDirect3DDevice9Ex_Release(device);
    DestroyWindow(window);
}

static void test_get_adapter_displaymode_ex(void)
{
    HWND window = create_window();
    IDirect3D9 *d3d9 = (void *) 0xdeadbeef;
    IDirect3D9Ex *d3d9ex;
    UINT count;
    HRESULT hr;
    D3DDISPLAYMODE mode;
    D3DDISPLAYMODEEX mode_ex;
    D3DDISPLAYROTATION rotation;
    DEVMODEW startmode, devmode;
    LONG retval;

    hr = pDirect3DCreate9Ex(D3D_SDK_VERSION, &d3d9ex);
    if (FAILED(hr))
    {
        skip("Failed to create IDirect3D9Ex, hr %#lx.\n", hr);
        DestroyWindow(window);
        return;
    }

    count = IDirect3D9Ex_GetAdapterCount(d3d9ex);
    if (!count)
    {
        skip("No adapters available.\n");
        IDirect3D9Ex_Release(d3d9ex);
        DestroyWindow(window);
        return;
    }

    hr = IDirect3D9Ex_QueryInterface(d3d9ex, &IID_IDirect3D9, (void **) &d3d9);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(d3d9 != NULL && d3d9 != (void *) 0xdeadbeef,
       "QueryInterface returned interface %p, expected != NULL && != 0xdeadbeef\n", d3d9);

    memset(&startmode, 0, sizeof(startmode));
    startmode.dmSize = sizeof(startmode);
    retval = EnumDisplaySettingsExW(NULL, ENUM_CURRENT_SETTINGS, &startmode, 0);
    ok(retval, "Failed to retrieve current display mode, retval %ld.\n", retval);
    if (!retval) goto out;

    devmode = startmode;
    devmode.dmFields = DM_DISPLAYORIENTATION | DM_PELSWIDTH | DM_PELSHEIGHT;
    devmode.dmDisplayOrientation = DMDO_180;
    retval = ChangeDisplaySettingsExW(NULL, &devmode, NULL, 0, NULL);
    if (retval == DISP_CHANGE_BADMODE)
    {
        skip("Graphics mode is not supported.\n");
        goto out;
    }

    ok(retval == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsEx failed with %ld.\n", retval);
    /* try retrieve orientation info with EnumDisplaySettingsEx*/
    devmode.dmFields = 0;
    devmode.dmDisplayOrientation = 0;
    ok(EnumDisplaySettingsExW(NULL, ENUM_CURRENT_SETTINGS, &devmode, EDS_ROTATEDMODE),
            "EnumDisplaySettingsEx failed.\n");

    /*now that orientation has changed start tests for GetAdapterDisplayModeEx: invalid Size*/
    memset(&mode_ex, 0, sizeof(mode_ex));
    hr = IDirect3D9Ex_GetAdapterDisplayModeEx(d3d9ex, D3DADAPTER_DEFAULT, &mode_ex, &rotation);
    ok(hr == D3DERR_INVALIDCALL, "Got hr %#lx.\n", hr);

    mode_ex.Size = sizeof(D3DDISPLAYMODEEX);
    /* invalid count*/
    hr = IDirect3D9Ex_GetAdapterDisplayModeEx(d3d9ex, count + 1, &mode_ex, &rotation);
    ok(hr == D3DERR_INVALIDCALL, "Got hr %#lx.\n", hr);
    /*valid count and valid Size*/
    hr = IDirect3D9Ex_GetAdapterDisplayModeEx(d3d9ex, D3DADAPTER_DEFAULT, &mode_ex, &rotation);
    ok(SUCCEEDED(hr), "GetAdapterDisplayModeEx failed, hr %#lx.\n", hr);

    /* Compare what GetAdapterDisplayMode returns with what GetAdapterDisplayModeEx returns*/
    hr = IDirect3D9_GetAdapterDisplayMode(d3d9, D3DADAPTER_DEFAULT, &mode);
    ok(SUCCEEDED(hr), "GetAdapterDisplayMode failed, hr %#lx.\n", hr);

    ok(mode_ex.Size == sizeof(D3DDISPLAYMODEEX), "size is %d\n", mode_ex.Size);
    ok(mode_ex.Width == mode.Width, "width is %d instead of %d\n", mode_ex.Width, mode.Width);
    ok(mode_ex.Height == mode.Height, "height is %d instead of %d\n", mode_ex.Height, mode.Height);
    ok(mode_ex.RefreshRate == mode.RefreshRate, "RefreshRate is %d instead of %d\n",
            mode_ex.RefreshRate, mode.RefreshRate);
    ok(mode_ex.Format == mode.Format, "format is %x instead of %x\n", mode_ex.Format, mode.Format);
    /* Don't know yet how to test for ScanLineOrdering, just testing that it
     * is set to a value by GetAdapterDisplayModeEx(). */
    ok(mode_ex.ScanLineOrdering != 0, "ScanLineOrdering returned 0\n");
    /* Check that orientation is returned correctly by GetAdapterDisplayModeEx
     * and EnumDisplaySettingsEx(). */
    ok(devmode.dmDisplayOrientation == DMDO_180 && rotation == D3DDISPLAYROTATION_180,
            "rotation is %d instead of %ld\n", rotation, devmode.dmDisplayOrientation);

    trace("GetAdapterDisplayModeEx returned Width = %d, Height = %d, RefreshRate = %d, Format = %x, ScanLineOrdering = %x, rotation = %d\n",
          mode_ex.Width, mode_ex.Height, mode_ex.RefreshRate, mode_ex.Format, mode_ex.ScanLineOrdering, rotation);

    /* test GetAdapterDisplayModeEx with null pointer for D3DDISPLAYROTATION */
    memset(&mode_ex, 0, sizeof(mode_ex));
    mode_ex.Size = sizeof(D3DDISPLAYMODEEX);

    hr = IDirect3D9Ex_GetAdapterDisplayModeEx(d3d9ex, D3DADAPTER_DEFAULT, &mode_ex, NULL);
    ok(SUCCEEDED(hr), "GetAdapterDisplayModeEx failed, hr %#lx.\n", hr);

    ok(mode_ex.Size == sizeof(D3DDISPLAYMODEEX), "size is %d\n", mode_ex.Size);
    ok(mode_ex.Width == mode.Width, "width is %d instead of %d\n", mode_ex.Width, mode.Width);
    ok(mode_ex.Height == mode.Height, "height is %d instead of %d\n", mode_ex.Height, mode.Height);
    ok(mode_ex.RefreshRate == mode.RefreshRate, "RefreshRate is %d instead of %d\n",
            mode_ex.RefreshRate, mode.RefreshRate);
    ok(mode_ex.Format == mode.Format, "format is %x instead of %x\n", mode_ex.Format, mode.Format);
    /* Don't know yet how to test for ScanLineOrdering, just testing that it
     * is set to a value by GetAdapterDisplayModeEx(). */
    ok(mode_ex.ScanLineOrdering != 0, "ScanLineOrdering returned 0\n");

    /* return to the default mode */
    retval = ChangeDisplaySettingsExW(NULL, &startmode, NULL, 0, NULL);
    ok(retval == DISP_CHANGE_SUCCESSFUL, "ChangeDisplaySettingsEx failed with %ld.\n", retval);
out:
    IDirect3D9_Release(d3d9);
    IDirect3D9Ex_Release(d3d9ex);
    DestroyWindow(window);
}

static void test_create_depth_stencil_surface_ex(void)
{
    static const struct
    {
        DWORD usage;
        HRESULT hr;
        BOOL broken_warp;
    }
    tests[] =
    {
        {0,                           D3D_OK,             FALSE},
        {D3DUSAGE_DEPTHSTENCIL,       D3DERR_INVALIDCALL, FALSE},
        {D3DUSAGE_RESTRICTED_CONTENT, D3D_OK,             TRUE},
    };

    D3DADAPTER_IDENTIFIER9 identifier;
    D3DSURFACE_DESC surface_desc;
    IDirect3DDevice9Ex *device;
    IDirect3DSurface9 *surface;
    IDirect3D9 *d3d;
    unsigned int i;
    HWND window;
    HRESULT hr;
    ULONG ref;
    BOOL warp;

    window = create_window();

    if (!(device = create_device(window, NULL)))
    {
        skip("Failed to create a D3D device.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9Ex_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get Direct3D9, hr %#lx.\n", hr);
    hr = IDirect3D9_GetAdapterIdentifier(d3d, D3DADAPTER_DEFAULT, 0, &identifier);
    ok(SUCCEEDED(hr), "Failed to get adapter identifier, hr %#lx.\n", hr);
    warp = adapter_is_warp(&identifier);
    IDirect3D9_Release(d3d);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        surface = (IDirect3DSurface9 *)0xdeadbeef;
        hr = IDirect3DDevice9Ex_CreateDepthStencilSurfaceEx(device, 64, 64, D3DFMT_D24S8,
                D3DMULTISAMPLE_NONE, 0, TRUE, &surface, NULL, tests[i].usage);
        ok(hr == tests[i].hr || broken(warp && tests[i].broken_warp),
                "Test %u: Got unexpected hr %#lx.\n", i, hr);
        if (SUCCEEDED(hr))
        {
            hr = IDirect3DSurface9_GetDesc(surface, &surface_desc);
            ok(SUCCEEDED(hr), "Test %u: GetDesc failed, hr %#lx.\n", i, hr);
            ok(surface_desc.Type == D3DRTYPE_SURFACE, "Test %u: Got unexpected type %#x.\n",
                    i, surface_desc.Type);
            ok(surface_desc.Pool == D3DPOOL_DEFAULT, "Test %u: Got unexpected pool %#x.\n",
                    i,  surface_desc.Pool);
            ok(surface_desc.Usage == (tests[i].usage | D3DUSAGE_DEPTHSTENCIL),
                    "Test %u: Got unexpected usage %#lx.\n", i, surface_desc.Usage);

            ref = IDirect3DSurface9_Release(surface);
            ok(!ref, "Test %u: Surface has %lu references left.\n", i, ref);
        }
        else
        {
            ok(surface == (IDirect3DSurface9 *)0xdeadbeef || broken(warp && tests[i].broken_warp),
                    "Test %u: Got unexpected surface pointer %p.\n", i, surface);
        }
    }

    ref = IDirect3DDevice9Ex_Release(device);
    ok(!ref, "Device has %lu references left.\n", ref);
    DestroyWindow(window);
}

static void test_user_memory(void)
{
    static const struct
    {
        float x, y, z;
        float u, v;
    }
    quad[] =
    {
        {-1.0f, -1.0f, 0.0f, 0.0f, 1.0f},
        {-1.0f,  1.0f, 0.0f, 0.0f, 0.0f},
        { 1.0f, -1.0f, 0.0f, 1.0f, 1.0f},
        { 1.0f,  1.0f, 0.0f, 1.0f, 0.0f},
    };
    IDirect3DDevice9Ex *device;
    IDirect3DTexture9 *texture, *texture2;
    IDirect3DCubeTexture9 *cube_texture;
    IDirect3DVolumeTexture9 *volume_texture;
    IDirect3DVertexBuffer9 *vertex_buffer;
    IDirect3DIndexBuffer9 *index_buffer;
    IDirect3DSurface9 *surface;
    D3DLOCKED_RECT locked_rect;
    unsigned int color, x, y;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    void *mem;
    char *ptr;
    D3DCAPS9 caps;

    window = create_window();
    if (!(device = create_device(window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#lx.\n", hr);

    mem = calloc(128 * 128, 4);
    hr = IDirect3DDevice9Ex_CreateTexture(device, 128, 128, 0, 0, D3DFMT_A8R8G8B8,
            D3DPOOL_SYSTEMMEM, &texture, &mem);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CreateTexture(device, 1, 1, 0, 0, D3DFMT_A8R8G8B8,
            D3DPOOL_SYSTEMMEM, &texture, &mem);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CreateTexture(device, 128, 128, 2, 0, D3DFMT_A8R8G8B8,
            D3DPOOL_SYSTEMMEM, &texture, &mem);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CreateTexture(device, 128, 128, 1, 0, D3DFMT_A8R8G8B8,
            D3DPOOL_SCRATCH, &texture, &mem);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_CreateTexture(device, 128, 128, 1, 0, D3DFMT_A8R8G8B8,
            D3DPOOL_SYSTEMMEM, &texture, &mem);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#lx.\n", hr);
    hr = IDirect3DTexture9_LockRect(texture, 0, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock texture, hr %#lx.\n", hr);
    ok(locked_rect.Pitch == 128 * 4, "Got unexpected pitch %d.\n", locked_rect.Pitch);
    ok(locked_rect.pBits == mem, "Got unexpected pBits %p, expected %p.\n", locked_rect.pBits, mem);
    hr = IDirect3DTexture9_UnlockRect(texture, 0);
    ok(SUCCEEDED(hr), "Failed to unlock texture, hr %#lx.\n", hr);
    IDirect3DTexture9_Release(texture);

    if (caps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP)
    {
        hr = IDirect3DDevice9Ex_CreateCubeTexture(device, 2, 1, 0, D3DFMT_A8R8G8B8,
                D3DPOOL_SYSTEMMEM, &cube_texture, &mem);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    }
    if (caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP)
    {
        hr = IDirect3DDevice9Ex_CreateVolumeTexture(device, 2, 2, 2, 1, 0, D3DFMT_A8R8G8B8,
                D3DPOOL_SYSTEMMEM, &volume_texture, &mem);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    }

    hr = IDirect3DDevice9Ex_CreateIndexBuffer(device, 16, 0, D3DFMT_INDEX32, D3DPOOL_SYSTEMMEM,
            &index_buffer, &mem);
    ok(hr == D3DERR_NOTAVAILABLE, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CreateVertexBuffer(device, 16, 0, 0, D3DPOOL_SYSTEMMEM,
            &vertex_buffer, &mem);
    ok(hr == D3DERR_NOTAVAILABLE, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurface(device, 128, 128, D3DFMT_A8R8G8B8,
            D3DPOOL_SYSTEMMEM, &surface, &mem);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#lx.\n", hr);
    hr = IDirect3DSurface9_LockRect(surface, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock surface, hr %#lx.\n", hr);
    ok(locked_rect.Pitch == 128 * 4, "Got unexpected pitch %d.\n", locked_rect.Pitch);
    ok(locked_rect.pBits == mem, "Got unexpected pBits %p, expected %p.\n", locked_rect.pBits, mem);
    hr = IDirect3DSurface9_UnlockRect(surface);
    ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#lx.\n", hr);
    IDirect3DSurface9_Release(surface);

    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurfaceEx(device, 128, 128, D3DFMT_A8R8G8B8,
            D3DPOOL_SYSTEMMEM, &surface, &mem, 0);
    todo_wine ok(SUCCEEDED(hr), "Failed to create surface, hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
    {
        hr = IDirect3DSurface9_LockRect(surface, &locked_rect, NULL, 0);
        ok(SUCCEEDED(hr), "Failed to lock surface, hr %#lx.\n", hr);
        ok(locked_rect.Pitch == 128 * 4, "Got unexpected pitch %d.\n", locked_rect.Pitch);
        ok(locked_rect.pBits == mem, "Got unexpected pBits %p, expected %p.\n", locked_rect.pBits, mem);
        hr = IDirect3DSurface9_UnlockRect(surface);
        ok(SUCCEEDED(hr), "Failed to unlock surface, hr %#lx.\n", hr);
        IDirect3DSurface9_Release(surface);
    }

    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurface(device, 128, 128, D3DFMT_A8R8G8B8,
            D3DPOOL_SCRATCH, &surface, &mem);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurfaceEx(device, 128, 128, D3DFMT_A8R8G8B8,
            D3DPOOL_SCRATCH, &surface, &mem, 0);
    todo_wine ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

    ptr = mem;
    for (y = 0; y < 33; ++y)
        for (x = 0; x < 33; ++x)
            *ptr++ = x * 255 / 32;

    hr = IDirect3DDevice9Ex_CreateTexture(device, 33, 33, 1, 0, D3DFMT_L8,
            D3DPOOL_SYSTEMMEM, &texture, &mem);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#lx.\n", hr);
    hr = IDirect3DTexture9_LockRect(texture, 0, &locked_rect, NULL, 0);
    ok(SUCCEEDED(hr), "Failed to lock texture, hr %#lx.\n", hr);
    ok(locked_rect.Pitch == 33, "Got unexpected pitch %d.\n", locked_rect.Pitch);
    ok(locked_rect.pBits == mem, "Got unexpected pBits %p, expected %p.\n", locked_rect.pBits, mem);
    hr = IDirect3DTexture9_UnlockRect(texture, 0);
    ok(SUCCEEDED(hr), "Failed to unlock texture, hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_CreateTexture(device, 33, 33, 1, 0, D3DFMT_L8,
            D3DPOOL_DEFAULT, &texture2, NULL);
    ok(SUCCEEDED(hr), "Failed to create texture, hr %#lx.\n", hr);
    hr = IDirect3DDevice9_UpdateTexture(device, (IDirect3DBaseTexture9 *)texture,
            (IDirect3DBaseTexture9 *)texture2);
    ok(SUCCEEDED(hr), "Failed to update texture, hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x000000ff, 0.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_SetFVF(device, D3DFVF_XYZ | D3DFVF_TEX1);
    ok(SUCCEEDED(hr), "Failed to set fvf, hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable lighting, hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_SetTexture(device, 0, (IDirect3DBaseTexture9 *)texture2);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(quad[0]));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#lx.\n", hr);

    color = get_pixel_color(device, 320, 240);
    ok(color_match(color, 0x007f7f7f, 2), "Got unexpected color %#x.\n", color);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#lx.\n", hr);

    hr = IDirect3DDevice9_SetTexture(device, 0, NULL);
    ok(SUCCEEDED(hr), "Failed to set texture, hr %#lx.\n", hr);
    IDirect3DTexture9_Release(texture2);
    IDirect3DTexture9_Release(texture);
    free(mem);
    refcount = IDirect3DDevice9Ex_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);

done:
    DestroyWindow(window);
}

static void test_reset(void)
{
    static const DWORD simple_vs[] =
    {
        0xfffe0101,                                     /* vs_1_1             */
        0x0000001f, 0x80000000, 0x900f0000,             /* dcl_position0 v0   */
        0x00000009, 0xc0010000, 0x90e40000, 0xa0e40000, /* dp4 oPos.x, v0, c0 */
        0x00000009, 0xc0020000, 0x90e40000, 0xa0e40001, /* dp4 oPos.y, v0, c1 */
        0x00000009, 0xc0040000, 0x90e40000, 0xa0e40002, /* dp4 oPos.z, v0, c2 */
        0x00000009, 0xc0080000, 0x90e40000, 0xa0e40003, /* dp4 oPos.w, v0, c3 */
        0x0000ffff,                                     /* end                */
    };

    unsigned int width, height, i, adapter_mode_count, offset, stride;
    const DWORD orig_height = GetSystemMetrics(SM_CYSCREEN);
    const DWORD orig_width = GetSystemMetrics(SM_CXSCREEN);
    IDirect3DVertexBuffer9 *vb, *cur_vb;
    IDirect3DIndexBuffer9 *ib, *cur_ib;
    IDirect3DVertexShader9 *shader;
    IDirect3DSwapChain9 *swapchain;
    D3DDISPLAYMODE d3ddm, d3ddm2;
    D3DPRESENT_PARAMETERS d3dpp;
    IDirect3DDevice9Ex *device;
    IDirect3DSurface9 *surface;
    IDirect3DTexture9 *texture;
    DEVMODEW devmode;
    IDirect3D9 *d3d9;
    D3DVIEWPORT9 vp;
    ULONG refcount;
    D3DCAPS9 caps;
    DWORD value;
    HWND window;
    HRESULT hr;
    RECT rect, client_rect;
    LONG ret;
    struct
    {
        UINT w;
        UINT h;
    } *modes = NULL;
    UINT mode_count = 0;

    window = create_window();
    if (!(device = create_device(window, NULL)))
    {
        skip("Failed to create a D3D device, skipping test.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9Ex_GetDirect3D(device, &d3d9);
    ok(SUCCEEDED(hr), "Failed to get d3d9, hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#lx.\n", hr);

    IDirect3D9_GetAdapterDisplayMode(d3d9, D3DADAPTER_DEFAULT, &d3ddm);
    adapter_mode_count = IDirect3D9_GetAdapterModeCount(d3d9, D3DADAPTER_DEFAULT, d3ddm.Format);
    modes = malloc(sizeof(*modes) * adapter_mode_count);
    for (i = 0; i < adapter_mode_count; ++i)
    {
        UINT j;

        hr = IDirect3D9_EnumAdapterModes(d3d9, D3DADAPTER_DEFAULT, d3ddm.Format, i, &d3ddm2);
        ok(SUCCEEDED(hr), "Failed to enumerate display mode, hr %#lx.\n", hr);

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

        /* We use them as invalid modes. */
        if ((d3ddm2.Width == 801 && d3ddm2.Height == 600)
                || (d3ddm2.Width == 32 && d3ddm2.Height == 32))
        {
            skip("This system supports a screen resolution of %dx%d, not running mode tests.\n",
                 d3ddm2.Width, d3ddm2.Height);
            goto cleanup;
        }
    }

    if (mode_count < 2)
    {
        skip("Less than 2 modes supported, skipping mode tests.\n");
        goto cleanup;
    }

    i = 0;
    if (modes[i].w == orig_width && modes[i].h == orig_height)
        ++i;

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.Windowed = FALSE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferWidth = modes[i].w;
    d3dpp.BackBufferHeight = modes[i].h;
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == modes[i].w, "Got screen width %u, expected %u.\n", width, modes[i].w);
    ok(height == modes[i].h, "Got screen height %u, expected %u.\n", height, modes[i].h);

    hr = IDirect3DDevice9Ex_GetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to get scissor rect, hr %#lx.\n", hr);
    ok(rect.left == 0 && rect.top == 0 && rect.right == modes[i].w && rect.bottom == modes[i].h,
            "Got unexpected scissor rect %s.\n", wine_dbgstr_rect(&rect));

    hr = IDirect3DDevice9Ex_GetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to get viewport, hr %#lx.\n", hr);
    ok(vp.X == 0, "Got unexpected vp.X %lu.\n", vp.X);
    ok(vp.Y == 0, "Got unexpected vp.Y %lu.\n", vp.Y);
    ok(vp.Width == modes[i].w, "Got vp.Width %lu, expected %u.\n", vp.Width, modes[i].w);
    ok(vp.Height == modes[i].h, "Got vp.Height %lu, expected %u.\n", vp.Height, modes[i].h);
    ok(vp.MinZ == 0.0f, "Got unexpected vp.MinZ %.8e.\n", vp.MinZ);
    ok(vp.MaxZ == 1.0f, "Got unexpected vp.MaxZ %.8e.\n", vp.MaxZ);

    i = 1;
    vp.X = 10;
    vp.Y = 20;
    vp.MinZ = 2.0f;
    vp.MaxZ = 3.0f;
    hr = IDirect3DDevice9Ex_SetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to set viewport, hr %#lx.\n", hr);

    SetRect(&rect, 10, 20, 30, 40);
    hr = IDirect3DDevice9Ex_SetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to set scissor rect, hr %#lx.\n", hr);

    hr = IDirect3DDevice9_GetRenderState(device, D3DRS_LIGHTING, &value);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#lx.\n", hr);
    ok(!!value, "Got unexpected value %#lx for D3DRS_LIGHTING.\n", value);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#lx.\n", hr);

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = FALSE;
    d3dpp.BackBufferWidth = modes[i].w;
    d3dpp.BackBufferHeight = modes[i].h;
    d3dpp.BackBufferFormat = d3ddm.Format;
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    /* Render states are preserved in d3d9ex. */
    hr = IDirect3DDevice9_GetRenderState(device, D3DRS_LIGHTING, &value);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#lx.\n", hr);
    ok(!value, "Got unexpected value %#lx for D3DRS_LIGHTING.\n", value);

    hr = IDirect3DDevice9Ex_GetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to get scissor rect, hr %#lx.\n", hr);
    ok(rect.left == 0 && rect.top == 0 && rect.right == modes[i].w && rect.bottom == modes[i].h,
            "Got unexpected scissor rect %s.\n", wine_dbgstr_rect(&rect));

    hr = IDirect3DDevice9Ex_GetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to get viewport, hr %#lx.\n", hr);
    ok(vp.X == 0, "Got unexpected vp.X %lu.\n", vp.X);
    ok(vp.Y == 0, "Got unexpected vp.Y %lu.\n", vp.Y);
    ok(vp.Width == modes[i].w, "Got vp.Width %lu, expected %u.\n", vp.Width, modes[i].w);
    ok(vp.Height == modes[i].h, "Got vp.Height %lu, expected %u.\n", vp.Height, modes[i].h);
    ok(vp.MinZ == 2.0f, "Got unexpected vp.MinZ %.8e.\n", vp.MinZ);
    ok(vp.MaxZ == 3.0f, "Got unexpected vp.MaxZ %.8e.\n", vp.MaxZ);

    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == modes[i].w, "Got screen width %u, expected %u.\n", width, modes[i].w);
    ok(height == modes[i].h, "Got screen height %u, expected %u.\n", height, modes[i].h);

    hr = IDirect3DDevice9Ex_GetSwapChain(device, 0, &swapchain);
    ok(SUCCEEDED(hr), "Failed to get swapchain, hr %#lx.\n", hr);
    hr = IDirect3DSwapChain9_GetPresentParameters(swapchain, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to get present parameters, hr %#lx.\n", hr);
    ok(d3dpp.BackBufferWidth == modes[i].w, "Got backbuffer width %u, expected %u.\n",
            d3dpp.BackBufferWidth, modes[i].w);
    ok(d3dpp.BackBufferHeight == modes[i].h, "Got backbuffer height %u, expected %u.\n",
            d3dpp.BackBufferHeight, modes[i].h);
    IDirect3DSwapChain9_Release(swapchain);

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = TRUE;
    d3dpp.BackBufferWidth = 400;
    d3dpp.BackBufferHeight = 300;
    d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == orig_width, "Got screen width %u, expected %lu.\n", width, orig_width);
    ok(height == orig_height, "Got screen height %u, expected %lu.\n", height, orig_height);

    hr = IDirect3DDevice9Ex_GetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to get scissor rect, hr %#lx.\n", hr);
    ok(rect.left == 0 && rect.top == 0 && rect.right == 400 && rect.bottom == 300,
            "Got unexpected scissor rect %s.\n", wine_dbgstr_rect(&rect));

    hr = IDirect3DDevice9Ex_GetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to get viewport, hr %#lx.\n", hr);
    ok(vp.X == 0, "Got unexpected vp.X %lu.\n", vp.X);
    ok(vp.Y == 0, "Got unexpected vp.Y %lu.\n", vp.Y);
    ok(vp.Width == 400, "Got unexpected vp.Width %lu.\n", vp.Width);
    ok(vp.Height == 300, "Got unexpected vp.Height %lu.\n", vp.Height);
    ok(vp.MinZ == 2.0f, "Got unexpected vp.MinZ %.8e.\n", vp.MinZ);
    ok(vp.MaxZ == 3.0f, "Got unexpected vp.MaxZ %.8e.\n", vp.MaxZ);

    hr = IDirect3DDevice9Ex_GetSwapChain(device, 0, &swapchain);
    ok(SUCCEEDED(hr), "Failed to get swapchain, hr %#lx.\n", hr);
    hr = IDirect3DSwapChain9_GetPresentParameters(swapchain, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to get present parameters, hr %#lx.\n", hr);
    ok(d3dpp.BackBufferWidth == 400, "Got unexpected backbuffer width %u.\n", d3dpp.BackBufferWidth);
    ok(d3dpp.BackBufferHeight == 300, "Got unexpected backbuffer height %u.\n", d3dpp.BackBufferHeight);
    IDirect3DSwapChain9_Release(swapchain);

    memset(&devmode, 0, sizeof(devmode));
    devmode.dmSize = sizeof(devmode);
    devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
    devmode.dmPelsWidth = modes[1].w;
    devmode.dmPelsHeight = modes[1].h;
    ret = ChangeDisplaySettingsW(&devmode, CDS_FULLSCREEN);
    ok(ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#lx.\n", ret);
    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == modes[1].w, "Screen width is %u, expected %u.\n", width, modes[1].w);
    ok(height == modes[1].h, "Screen height is %u, expected %u.\n", height, modes[1].h);

    d3dpp.BackBufferWidth  = 500;
    d3dpp.BackBufferHeight = 400;
    d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == modes[1].w, "Screen width is %u, expected %u.\n", width, modes[1].w);
    ok(height == modes[1].h, "Screen height is %u, expected %u.\n", height, modes[1].h);

    hr = IDirect3DDevice9Ex_GetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to get scissor rect, hr %#lx.\n", hr);
    ok(rect.left == 0 && rect.top == 0 && rect.right == 500 && rect.bottom == 400,
            "Got unexpected scissor rect %s.\n", wine_dbgstr_rect(&rect));

    hr = IDirect3DDevice9Ex_GetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to get viewport, hr %#lx.\n", hr);
    ok(vp.X == 0, "Got unexpected vp.X %lu.\n", vp.X);
    ok(vp.Y == 0, "Got unexpected vp.Y %lu.\n", vp.Y);
    ok(vp.Width == 500, "Got unexpected vp.Width %lu.\n", vp.Width);
    ok(vp.Height == 400, "Got unexpected vp.Height %lu.\n", vp.Height);
    ok(vp.MinZ == 2.0f, "Got unexpected vp.MinZ %.8e.\n", vp.MinZ);
    ok(vp.MaxZ == 3.0f, "Got unexpected vp.MaxZ %.8e.\n", vp.MaxZ);

    hr = IDirect3DDevice9Ex_GetSwapChain(device, 0, &swapchain);
    ok(SUCCEEDED(hr), "Failed to get swapchain, hr %#lx.\n", hr);
    memset(&d3dpp, 0, sizeof(d3dpp));
    hr = IDirect3DSwapChain9_GetPresentParameters(swapchain, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to get present parameters, hr %#lx.\n", hr);
    ok(d3dpp.BackBufferWidth == 500, "Got unexpected BackBufferWidth %u.\n", d3dpp.BackBufferWidth);
    ok(d3dpp.BackBufferHeight == 400, "Got unexpected BackBufferHeight %u.\n", d3dpp.BackBufferHeight);
    IDirect3DSwapChain9_Release(swapchain);

    devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
    devmode.dmPelsWidth = orig_width;
    devmode.dmPelsHeight = orig_height;
    ret = ChangeDisplaySettingsW(&devmode, CDS_FULLSCREEN);
    ok(ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#lx.\n", ret);
    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == orig_width, "Got screen width %u, expected %lu.\n", width, orig_width);
    ok(height == orig_height, "Got screen height %u, expected %lu.\n", height, orig_height);

    SetRect(&rect, 0, 0, 200, 150);
    ok(AdjustWindowRect(&rect, GetWindowLongW(window, GWL_STYLE), FALSE), "Failed to adjust window rect.\n");
    ok(SetWindowPos(window, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
            SWP_NOMOVE | SWP_NOZORDER), "Failed to set window position.\n");

    /* Windows 10 gives us a different size than we requested with some DPI scaling settings (e.g. 172%). */
    ok(GetClientRect(window, &client_rect), "Failed to get client rect.\n");

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = TRUE;
    d3dpp.BackBufferWidth = 0;
    d3dpp.BackBufferHeight = 0;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    ok(d3dpp.BackBufferWidth == client_rect.right,
            "Got unexpected BackBufferWidth %u, expected %ld.\n", d3dpp.BackBufferWidth, client_rect.right);
    ok(d3dpp.BackBufferHeight == client_rect.bottom,
            "Got unexpected BackBufferHeight %u, expected %ld.\n", d3dpp.BackBufferHeight, client_rect.bottom);
    ok(d3dpp.BackBufferFormat == d3ddm.Format, "Got unexpected BackBufferFormat %#x, expected %#x.\n",
            d3dpp.BackBufferFormat, d3ddm.Format);
    ok(d3dpp.BackBufferCount == 1, "Got unexpected BackBufferCount %u.\n", d3dpp.BackBufferCount);
    ok(!d3dpp.MultiSampleType, "Got unexpected MultiSampleType %u.\n", d3dpp.MultiSampleType);
    ok(!d3dpp.MultiSampleQuality, "Got unexpected MultiSampleQuality %lu.\n", d3dpp.MultiSampleQuality);
    ok(d3dpp.SwapEffect == D3DSWAPEFFECT_DISCARD, "Got unexpected SwapEffect %#x.\n", d3dpp.SwapEffect);
    ok(!d3dpp.hDeviceWindow, "Got unexpected hDeviceWindow %p.\n", d3dpp.hDeviceWindow);
    ok(d3dpp.Windowed, "Got unexpected Windowed %#x.\n", d3dpp.Windowed);
    ok(!d3dpp.EnableAutoDepthStencil, "Got unexpected EnableAutoDepthStencil %#x.\n", d3dpp.EnableAutoDepthStencil);
    ok(!d3dpp.AutoDepthStencilFormat, "Got unexpected AutoDepthStencilFormat %#x.\n", d3dpp.AutoDepthStencilFormat);
    ok(!d3dpp.Flags, "Got unexpected Flags %#lx.\n", d3dpp.Flags);
    ok(!d3dpp.FullScreen_RefreshRateInHz, "Got unexpected FullScreen_RefreshRateInHz %u.\n",
            d3dpp.FullScreen_RefreshRateInHz);
    ok(!d3dpp.PresentationInterval, "Got unexpected PresentationInterval %#x.\n", d3dpp.PresentationInterval);

    hr = IDirect3DDevice9Ex_GetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to get scissor rect, hr %#lx.\n", hr);
    ok(EqualRect(&rect, &client_rect), "Got unexpected scissor rect %s.\n", wine_dbgstr_rect(&rect));

    hr = IDirect3DDevice9Ex_GetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to get viewport, hr %#lx.\n", hr);
    ok(vp.X == 0, "Got unexpected vp.X %lu.\n", vp.X);
    ok(vp.Y == 0, "Got unexpected vp.Y %lu.\n", vp.Y);
    ok(vp.Width == client_rect.right, "Got unexpected vp.Width %lu, expected %ld.\n",
            vp.Width, client_rect.right);
    ok(vp.Height == client_rect.bottom, "Got unexpected vp.Height %lu, expected %ld.\n",
            vp.Height, client_rect.bottom);
    ok(vp.MinZ == 2.0f, "Got unexpected vp.MinZ %.8e.\n", vp.MinZ);
    ok(vp.MaxZ == 3.0f, "Got unexpected vp.MaxZ %.8e.\n", vp.MaxZ);

    hr = IDirect3DDevice9Ex_GetSwapChain(device, 0, &swapchain);
    ok(SUCCEEDED(hr), "Failed to get swapchain, hr %#lx.\n", hr);
    hr = IDirect3DSwapChain9_GetPresentParameters(swapchain, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to get present parameters, hr %#lx.\n", hr);
    ok(d3dpp.BackBufferWidth == client_rect.right, "Got unexpected backbuffer width %u, expected %ld.\n",
            d3dpp.BackBufferWidth, client_rect.right);
    ok(d3dpp.BackBufferHeight == client_rect.bottom, "Got unexpected backbuffer height %u, expected %ld.\n",
            d3dpp.BackBufferHeight, client_rect.bottom);
    ok(d3dpp.BackBufferFormat == d3ddm.Format, "Got unexpected BackBufferFormat %#x, expected %#x.\n",
            d3dpp.BackBufferFormat, d3ddm.Format);
    ok(d3dpp.BackBufferCount == 1, "Got unexpected BackBufferCount %u.\n", d3dpp.BackBufferCount);
    ok(!d3dpp.MultiSampleType, "Got unexpected MultiSampleType %u.\n", d3dpp.MultiSampleType);
    ok(!d3dpp.MultiSampleQuality, "Got unexpected MultiSampleQuality %lu.\n", d3dpp.MultiSampleQuality);
    ok(d3dpp.SwapEffect == D3DSWAPEFFECT_DISCARD, "Got unexpected SwapEffect %#x.\n", d3dpp.SwapEffect);
    ok(d3dpp.hDeviceWindow == window, "Got unexpected hDeviceWindow %p, expected %p.\n", d3dpp.hDeviceWindow, window);
    ok(d3dpp.Windowed, "Got unexpected Windowed %#x.\n", d3dpp.Windowed);
    ok(!d3dpp.EnableAutoDepthStencil, "Got unexpected EnableAutoDepthStencil %#x.\n", d3dpp.EnableAutoDepthStencil);
    ok(!d3dpp.AutoDepthStencilFormat, "Got unexpected AutoDepthStencilFormat %#x.\n", d3dpp.AutoDepthStencilFormat);
    ok(!d3dpp.Flags, "Got unexpected Flags %#lx.\n", d3dpp.Flags);
    ok(!d3dpp.FullScreen_RefreshRateInHz, "Got unexpected FullScreen_RefreshRateInHz %u.\n",
            d3dpp.FullScreen_RefreshRateInHz);
    ok(!d3dpp.PresentationInterval, "Got unexpected PresentationInterval %#x.\n", d3dpp.PresentationInterval);
    IDirect3DSwapChain9_Release(swapchain);

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = TRUE;
    d3dpp.BackBufferWidth = 400;
    d3dpp.BackBufferHeight = 300;

    /* Reset with resources in the default pool succeeds in d3d9ex. */
    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurface(device, 16, 16,
            D3DFMT_R5G6B5, D3DPOOL_DEFAULT, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    IDirect3DSurface9_Release(surface);

    if (caps.TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP)
    {
        IDirect3DVolumeTexture9 *volume_texture;

        hr = IDirect3DDevice9Ex_CreateVolumeTexture(device, 16, 16, 4, 1, 0,
                D3DFMT_R5G6B5, D3DPOOL_DEFAULT, &volume_texture, NULL);
        ok(SUCCEEDED(hr), "Failed to create volume texture, hr %#lx.\n", hr);
        hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
        ok(SUCCEEDED(hr), "Failed to reset device, hr %#lx.\n", hr);
        hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
        ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
        IDirect3DVolumeTexture9_Release(volume_texture);
    }
    else
    {
        skip("Volume textures not supported.\n");
    }

    /* Test with resources bound but otherwise not referenced. */
    hr = IDirect3DDevice9Ex_CreateVertexBuffer(device, 16, 0,
            D3DFVF_XYZ, D3DPOOL_DEFAULT, &vb, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_SetStreamSource(device, 0, vb, 0, 16);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    refcount = IDirect3DVertexBuffer9_Release(vb);
    ok(!refcount, "Unexpected refcount %lu.\n", refcount);
    hr = IDirect3DDevice9Ex_CreateIndexBuffer(device, 16, 0,
            D3DFMT_INDEX16, D3DPOOL_DEFAULT, &ib, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_SetIndices(device, ib);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    refcount = IDirect3DIndexBuffer9_Release(ib);
    ok(!refcount, "Unexpected refcount %lu.\n", refcount);
    hr = IDirect3DDevice9Ex_CreateTexture(device, 16, 16, 0, 0,
            D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_SetTexture(device, i, (IDirect3DBaseTexture9 *)texture);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_GetIndices(device, &cur_ib);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    ok(cur_ib == ib, "Unexpected index buffer %p.\n", cur_ib);
    refcount = IDirect3DIndexBuffer9_Release(ib);
    ok(!refcount, "Unexpected refcount %lu.\n", refcount);
    hr = IDirect3DDevice9Ex_GetStreamSource(device, 0, &cur_vb, &offset, &stride);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    ok(cur_vb == vb, "Unexpected index buffer %p.\n", cur_ib);
    ok(!offset, "Unexpected offset %u.\n", offset);
    ok(stride == 16, "Unexpected stride %u.\n", stride);
    refcount = IDirect3DVertexBuffer9_Release(vb);
    ok(!refcount, "Unexpected refcount %lu.\n", refcount);
    refcount = IDirect3DTexture9_Release(texture);
    ok(!refcount, "Unexpected refcount %lu.\n", refcount);

    /* Scratch and sysmem pools are fine too. */
    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurface(device, 16, 16,
            D3DFMT_R5G6B5, D3DPOOL_SCRATCH, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    IDirect3DSurface9_Release(surface);

    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurface(device, 16, 16,
            D3DFMT_R5G6B5, D3DPOOL_SYSTEMMEM, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    IDirect3DSurface9_Release(surface);

    /* The depth stencil should get reset to the auto depth stencil when present. */
    hr = IDirect3DDevice9Ex_SetDepthStencilSurface(device, NULL);
    ok(SUCCEEDED(hr), "Failed to set depth/stencil surface, hr %#lx.\n", hr);

    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_GetDepthStencilSurface(device, &surface);
    ok(SUCCEEDED(hr), "Failed to get depth/stencil surface, hr %#lx.\n", hr);
    ok(!!surface, "Depth/stencil surface should not be NULL.\n");
    IDirect3DSurface9_Release(surface);

    d3dpp.EnableAutoDepthStencil = FALSE;
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_GetDepthStencilSurface(device, &surface);
    ok(hr == D3DERR_NOTFOUND, "Got unexpected hr %#lx.\n", hr);
    ok(!surface, "Depth/stencil surface should be NULL.\n");

    /* References to implicit surfaces are allowed in d3d9ex. */
    hr = IDirect3DDevice9Ex_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &surface);
    ok(SUCCEEDED(hr), "Failed to get backbuffer, hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);
    IDirect3DSurface9_Release(surface);

    /* Shaders are fine. */
    hr = IDirect3DDevice9Ex_CreateVertexShader(device, simple_vs, &shader);
    ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#lx.\n", hr);
    IDirect3DVertexShader9_Release(shader);

    /* Try setting invalid modes. */
    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = FALSE;
    d3dpp.BackBufferWidth = 32;
    d3dpp.BackBufferHeight = 32;
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = FALSE;
    d3dpp.BackBufferWidth = 801;
    d3dpp.BackBufferHeight = 600;
    hr = IDirect3DDevice9Ex_Reset(device, &d3dpp);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    hr = IDirect3D9_GetAdapterDisplayMode(d3d9, D3DADAPTER_DEFAULT, &d3ddm);
    ok(SUCCEEDED(hr), "Failed to get display mode, hr %#lx.\n", hr);

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = d3ddm.Format;
    d3dpp.EnableAutoDepthStencil = FALSE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;

cleanup:
    free(modes);
    IDirect3D9_Release(d3d9);
    refcount = IDirect3DDevice9Ex_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
    DestroyWindow(window);
}

static void test_reset_ex(void)
{
    unsigned int height, orig_height = GetSystemMetrics(SM_CYSCREEN);
    unsigned int width, orig_width = GetSystemMetrics(SM_CXSCREEN);
    unsigned int i, adapter_mode_count, mode_count = 0;
    D3DDISPLAYMODEEX mode, mode2, *modes;
    D3DDISPLAYMODEFILTER mode_filter;
    IDirect3DSwapChain9 *swapchain;
    D3DPRESENT_PARAMETERS d3dpp;
    IDirect3DDevice9Ex *device;
    IDirect3D9Ex *d3d9;
    DEVMODEW devmode;
    D3DVIEWPORT9 vp;
    ULONG refcount;
    DWORD value;
    HWND window;
    HRESULT hr;
    RECT rect, client_rect;
    LONG ret;

    window = create_window();
    if (!(device = create_device(window, NULL)))
    {
        skip("Failed to create a D3D device.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9Ex_GetDirect3D(device, (IDirect3D9 **)&d3d9);
    ok(SUCCEEDED(hr), "Failed to get d3d9, hr %#lx.\n", hr);

    memset(&mode, 0, sizeof(mode));
    mode.Size = sizeof(mode);
    hr = IDirect3D9Ex_GetAdapterDisplayModeEx(d3d9, D3DADAPTER_DEFAULT, &mode, NULL);
    ok(SUCCEEDED(hr), "GetAdapterDisplayModeEx failed, hr %#lx.\n", hr);
    memset(&mode_filter, 0, sizeof(mode_filter));
    mode_filter.Size = sizeof(mode_filter);
    mode_filter.Format = mode.Format;
    adapter_mode_count = IDirect3D9Ex_GetAdapterModeCountEx(d3d9, D3DADAPTER_DEFAULT, &mode_filter);
    modes = malloc(sizeof(*modes) * adapter_mode_count);
    for (i = 0; i < adapter_mode_count; ++i)
    {
        unsigned int j;

        memset(&mode2, 0, sizeof(mode));
        mode2.Size = sizeof(mode2);
        hr = IDirect3D9Ex_EnumAdapterModesEx(d3d9, D3DADAPTER_DEFAULT, &mode_filter, i, &mode2);
        ok(SUCCEEDED(hr), "Failed to enumerate display mode, hr %#lx.\n", hr);

        for (j = 0; j < mode_count; ++j)
        {
            if (modes[j].Width == mode2.Width && modes[j].Height == mode2.Height)
                break;
        }
        if (j == mode_count)
        {
            modes[j] = mode2;
            ++mode_count;
        }
    }

    if (mode_count < 2)
    {
        skip("Less than 2 modes supported.\n");
        goto cleanup;
    }

    i = 0;
    if (modes[i].Width == orig_width && modes[i].Height == orig_height)
        ++i;

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.Windowed = FALSE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferWidth = modes[i].Width;
    d3dpp.BackBufferHeight = modes[i].Height;
    d3dpp.BackBufferFormat = modes[i].Format;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    modes[i].RefreshRate = 0;
    modes[i].ScanLineOrdering = 0;
    hr = IDirect3DDevice9Ex_ResetEx(device, &d3dpp, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_ResetEx(device, &d3dpp, &modes[i]);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == modes[i].Width, "Got screen width %u, expected %u.\n", width, modes[i].Width);
    ok(height == modes[i].Height, "Got screen height %u, expected %u.\n", height, modes[i].Height);

    hr = IDirect3DDevice9Ex_GetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to get scissor rect, hr %#lx.\n", hr);
    ok(rect.left == 0 && rect.top == 0 && rect.right == modes[i].Width && rect.bottom == modes[i].Height,
            "Got unexpected scissor rect %s.\n", wine_dbgstr_rect(&rect));

    hr = IDirect3DDevice9Ex_GetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to get viewport, hr %#lx.\n", hr);
    ok(vp.X == 0, "Got unexpected vp.X %lu.\n", vp.X);
    ok(vp.Y == 0, "Got unexpected vp.Y %lu.\n", vp.Y);
    ok(vp.Width == modes[i].Width, "Got vp.Width %lu, expected %u.\n", vp.Width, modes[i].Width);
    ok(vp.Height == modes[i].Height, "Got vp.Height %lu, expected %u.\n", vp.Height, modes[i].Height);
    ok(vp.MinZ == 0.0f, "Got unexpected vp.MinZ %.8e.\n", vp.MinZ);
    ok(vp.MaxZ == 1.0f, "Got unexpected vp.MaxZ %.8e.\n", vp.MaxZ);

    i = 1;
    vp.X = 10;
    vp.Y = 20;
    vp.MinZ = 2.0f;
    vp.MaxZ = 3.0f;
    hr = IDirect3DDevice9Ex_SetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to set viewport, hr %#lx.\n", hr);

    SetRect(&rect, 10, 20, 30, 40);
    hr = IDirect3DDevice9Ex_SetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to set scissor rect, hr %#lx.\n", hr);

    hr = IDirect3DDevice9_GetRenderState(device, D3DRS_LIGHTING, &value);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#lx.\n", hr);
    ok(!!value, "Got unexpected value %#lx for D3DRS_LIGHTING.\n", value);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to set render state, hr %#lx.\n", hr);

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = FALSE;
    d3dpp.BackBufferWidth = modes[i].Width;
    d3dpp.BackBufferHeight = modes[i].Height;
    d3dpp.BackBufferFormat = modes[i].Format;
    modes[i].RefreshRate = 0;
    modes[i].ScanLineOrdering = 0;
    hr = IDirect3DDevice9Ex_ResetEx(device, &d3dpp, &modes[i]);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    /* Render states are preserved in d3d9ex. */
    hr = IDirect3DDevice9_GetRenderState(device, D3DRS_LIGHTING, &value);
    ok(SUCCEEDED(hr), "Failed to get render state, hr %#lx.\n", hr);
    ok(!value, "Got unexpected value %#lx for D3DRS_LIGHTING.\n", value);

    hr = IDirect3DDevice9Ex_GetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to get scissor rect, hr %#lx.\n", hr);
    ok(rect.left == 0 && rect.top == 0 && rect.right == modes[i].Width && rect.bottom == modes[i].Height,
            "Got unexpected scissor rect %s.\n", wine_dbgstr_rect(&rect));

    hr = IDirect3DDevice9Ex_GetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to get viewport, hr %#lx.\n", hr);
    ok(vp.X == 0, "Got unexpected vp.X %lu.\n", vp.X);
    ok(vp.Y == 0, "Got unexpected vp.Y %lu.\n", vp.Y);
    ok(vp.Width == modes[i].Width, "Got vp.Width %lu, expected %u.\n", vp.Width, modes[i].Width);
    ok(vp.Height == modes[i].Height, "Got vp.Height %lu, expected %u.\n", vp.Height, modes[i].Height);
    ok(vp.MinZ == 2.0f, "Got unexpected vp.MinZ %.8e.\n", vp.MinZ);
    ok(vp.MaxZ == 3.0f, "Got unexpected vp.MaxZ %.8e.\n", vp.MaxZ);

    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == modes[i].Width, "Got screen width %u, expected %u.\n", width, modes[i].Width);
    ok(height == modes[i].Height, "Got screen height %u, expected %u.\n", height, modes[i].Height);

    hr = IDirect3DDevice9Ex_GetSwapChain(device, 0, &swapchain);
    ok(SUCCEEDED(hr), "Failed to get swapchain, hr %#lx.\n", hr);
    hr = IDirect3DSwapChain9_GetPresentParameters(swapchain, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to get present parameters, hr %#lx.\n", hr);
    ok(d3dpp.BackBufferWidth == modes[i].Width, "Got backbuffer width %u, expected %u.\n",
            d3dpp.BackBufferWidth, modes[i].Width);
    ok(d3dpp.BackBufferHeight == modes[i].Height, "Got backbuffer height %u, expected %u.\n",
            d3dpp.BackBufferHeight, modes[i].Height);
    IDirect3DSwapChain9_Release(swapchain);

    /* BackBufferWidth and BackBufferHeight have to match display mode. */
    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = FALSE;
    d3dpp.BackBufferFormat = modes[i].Format;
    d3dpp.BackBufferWidth = modes[i].Width - 10;
    d3dpp.BackBufferHeight = modes[i].Height - 10;
    hr = IDirect3DDevice9Ex_ResetEx(device, &d3dpp, &modes[i]);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    d3dpp.BackBufferWidth = modes[i].Width - 1;
    d3dpp.BackBufferHeight = modes[i].Height;
    hr = IDirect3DDevice9Ex_ResetEx(device, &d3dpp, &modes[i]);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    d3dpp.BackBufferWidth = modes[i].Width;
    d3dpp.BackBufferHeight = modes[i].Height - 1;
    hr = IDirect3DDevice9Ex_ResetEx(device, &d3dpp, &modes[i]);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    d3dpp.BackBufferWidth = 0;
    d3dpp.BackBufferHeight = 0;
    hr = IDirect3DDevice9Ex_ResetEx(device, &d3dpp, &modes[i]);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    d3dpp.BackBufferWidth = modes[i].Width;
    d3dpp.BackBufferHeight = modes[i].Height;
    mode2 = modes[i];
    mode2.Width = 0;
    mode2.Height = 0;
    hr = IDirect3DDevice9Ex_ResetEx(device, &d3dpp, &mode2);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    d3dpp.BackBufferWidth = modes[i].Width;
    d3dpp.BackBufferHeight = modes[i].Height;
    hr = IDirect3DDevice9Ex_ResetEx(device, &d3dpp, &modes[i]);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_GetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to get scissor rect, hr %#lx.\n", hr);
    ok(rect.left == 0 && rect.top == 0 && rect.right == modes[i].Width && rect.bottom == modes[i].Height,
            "Got unexpected scissor rect %s.\n", wine_dbgstr_rect(&rect));

    hr = IDirect3DDevice9Ex_GetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to get viewport, hr %#lx.\n", hr);
    ok(vp.X == 0, "Got unexpected vp.X %lu.\n", vp.X);
    ok(vp.Y == 0, "Got unexpected vp.Y %lu.\n", vp.Y);
    ok(vp.Width == modes[i].Width, "Got vp.Width %lu, expected %u.\n", vp.Width, modes[i].Width);
    ok(vp.Height == modes[i].Height, "Got vp.Height %lu, expected %u.\n", vp.Height, modes[i].Height);
    ok(vp.MinZ == 2.0f, "Got unexpected vp.MinZ %.8e.\n", vp.MinZ);
    ok(vp.MaxZ == 3.0f, "Got unexpected vp.MaxZ %.8e.\n", vp.MaxZ);

    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == modes[i].Width, "Got screen width %u, expected %u.\n", width, modes[i].Width);
    ok(height == modes[i].Height, "Got screen height %u, expected %u.\n", height, modes[i].Height);

    hr = IDirect3DDevice9Ex_GetSwapChain(device, 0, &swapchain);
    ok(SUCCEEDED(hr), "Failed to get swapchain, hr %#lx.\n", hr);
    hr = IDirect3DSwapChain9_GetPresentParameters(swapchain, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to get present parameters, hr %#lx.\n", hr);
    ok(d3dpp.BackBufferWidth == modes[i].Width, "Got backbuffer width %u, expected %u.\n",
            d3dpp.BackBufferWidth, modes[i].Width);
    ok(d3dpp.BackBufferHeight == modes[i].Height, "Got backbuffer height %u, expected %u.\n",
            d3dpp.BackBufferHeight, modes[i].Height);
    IDirect3DSwapChain9_Release(swapchain);

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = TRUE;
    d3dpp.BackBufferWidth = 400;
    d3dpp.BackBufferHeight = 300;
    d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    hr = IDirect3DDevice9Ex_ResetEx(device, &d3dpp, NULL);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == orig_width, "Got screen width %u, expected %u.\n", width, orig_width);
    ok(height == orig_height, "Got screen height %u, expected %u.\n", height, orig_height);

    hr = IDirect3DDevice9Ex_GetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to get scissor rect, hr %#lx.\n", hr);
    ok(rect.left == 0 && rect.top == 0 && rect.right == 400 && rect.bottom == 300,
            "Got unexpected scissor rect %s.\n", wine_dbgstr_rect(&rect));

    hr = IDirect3DDevice9Ex_GetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to get viewport, hr %#lx.\n", hr);
    ok(vp.X == 0, "Got unexpected vp.X %lu.\n", vp.X);
    ok(vp.Y == 0, "Got unexpected vp.Y %lu.\n", vp.Y);
    ok(vp.Width == 400, "Got unexpected vp.Width %lu.\n", vp.Width);
    ok(vp.Height == 300, "Got unexpected vp.Height %lu.\n", vp.Height);
    ok(vp.MinZ == 2.0f, "Got unexpected vp.MinZ %.8e.\n", vp.MinZ);
    ok(vp.MaxZ == 3.0f, "Got unexpected vp.MaxZ %.8e.\n", vp.MaxZ);

    hr = IDirect3DDevice9Ex_GetSwapChain(device, 0, &swapchain);
    ok(SUCCEEDED(hr), "Failed to get swapchain, hr %#lx.\n", hr);
    hr = IDirect3DSwapChain9_GetPresentParameters(swapchain, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to get present parameters, hr %#lx.\n", hr);
    ok(d3dpp.BackBufferWidth == 400, "Got unexpected backbuffer width %u.\n", d3dpp.BackBufferWidth);
    ok(d3dpp.BackBufferHeight == 300, "Got unexpected backbuffer height %u.\n", d3dpp.BackBufferHeight);
    IDirect3DSwapChain9_Release(swapchain);

    memset(&devmode, 0, sizeof(devmode));
    devmode.dmSize = sizeof(devmode);
    devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
    devmode.dmPelsWidth = modes[1].Width;
    devmode.dmPelsHeight = modes[1].Height;
    ret = ChangeDisplaySettingsW(&devmode, CDS_FULLSCREEN);
    ok(ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#lx.\n", ret);
    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == modes[1].Width, "Screen width is %u, expected %u.\n", width, modes[1].Width);
    ok(height == modes[1].Height, "Screen height is %u, expected %u.\n", height, modes[1].Height);

    d3dpp.BackBufferWidth  = 500;
    d3dpp.BackBufferHeight = 400;
    d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    hr = IDirect3DDevice9Ex_ResetEx(device, &d3dpp, &mode);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_ResetEx(device, &d3dpp, NULL);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == modes[1].Width, "Screen width is %u, expected %u.\n", width, modes[1].Width);
    ok(height == modes[1].Height, "Screen height is %u, expected %u.\n", height, modes[1].Height);

    hr = IDirect3DDevice9Ex_GetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to get scissor rect, hr %#lx.\n", hr);
    ok(rect.left == 0 && rect.top == 0 && rect.right == 500 && rect.bottom == 400,
            "Got unexpected scissor rect %s.\n", wine_dbgstr_rect(&rect));

    hr = IDirect3DDevice9Ex_GetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to get viewport, hr %#lx.\n", hr);
    ok(vp.X == 0, "Got unexpected vp.X %lu.\n", vp.X);
    ok(vp.Y == 0, "Got unexpected vp.Y %lu.\n", vp.Y);
    ok(vp.Width == 500, "Got unexpected vp.Width %lu.\n", vp.Width);
    ok(vp.Height == 400, "Got unexpected vp.Height %lu.\n", vp.Height);
    ok(vp.MinZ == 2.0f, "Got unexpected vp.MinZ %.8e.\n", vp.MinZ);
    ok(vp.MaxZ == 3.0f, "Got unexpected vp.MaxZ %.8e.\n", vp.MaxZ);

    hr = IDirect3DDevice9Ex_GetSwapChain(device, 0, &swapchain);
    ok(SUCCEEDED(hr), "Failed to get swapchain, hr %#lx.\n", hr);
    memset(&d3dpp, 0, sizeof(d3dpp));
    hr = IDirect3DSwapChain9_GetPresentParameters(swapchain, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to get present parameters, hr %#lx.\n", hr);
    ok(d3dpp.BackBufferWidth == 500, "Got unexpected BackBufferWidth %u.\n", d3dpp.BackBufferWidth);
    ok(d3dpp.BackBufferHeight == 400, "Got unexpected BackBufferHeight %u.\n", d3dpp.BackBufferHeight);
    IDirect3DSwapChain9_Release(swapchain);

    devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
    devmode.dmPelsWidth = orig_width;
    devmode.dmPelsHeight = orig_height;
    ret = ChangeDisplaySettingsW(&devmode, CDS_FULLSCREEN);
    ok(ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#lx.\n", ret);
    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    ok(width == orig_width, "Got screen width %u, expected %u.\n", width, orig_width);
    ok(height == orig_height, "Got screen height %u, expected %u.\n", height, orig_height);

    SetRect(&rect, 0, 0, 200, 150);
    ok(AdjustWindowRect(&rect, GetWindowLongW(window, GWL_STYLE), FALSE), "Failed to adjust window rect.\n");
    ok(SetWindowPos(window, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
            SWP_NOMOVE | SWP_NOZORDER), "Failed to set window position.\n");

    /* Windows 10 gives us a different size than we requested with some DPI scaling settings (e.g. 172%). */
    ok(GetClientRect(window, &client_rect), "Failed to get client rect.\n");

    memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.Windowed = TRUE;
    d3dpp.BackBufferWidth = 0;
    d3dpp.BackBufferHeight = 0;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    hr = IDirect3DDevice9Ex_ResetEx(device, &d3dpp, NULL);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got hr %#lx.\n", hr);

    ok(d3dpp.BackBufferWidth == client_rect.right,
            "Got unexpected BackBufferWidth %u, expected %ld.\n", d3dpp.BackBufferWidth, client_rect.right);
    ok(d3dpp.BackBufferHeight == client_rect.bottom,
            "Got unexpected BackBufferHeight %u, expected %ld.\n", d3dpp.BackBufferHeight, client_rect.bottom);
    ok(d3dpp.BackBufferFormat == mode.Format, "Got unexpected BackBufferFormat %#x, expected %#x.\n",
            d3dpp.BackBufferFormat, mode.Format);
    ok(d3dpp.BackBufferCount == 1, "Got unexpected BackBufferCount %u.\n", d3dpp.BackBufferCount);
    ok(!d3dpp.MultiSampleType, "Got unexpected MultiSampleType %u.\n", d3dpp.MultiSampleType);
    ok(!d3dpp.MultiSampleQuality, "Got unexpected MultiSampleQuality %lu.\n", d3dpp.MultiSampleQuality);
    ok(d3dpp.SwapEffect == D3DSWAPEFFECT_DISCARD, "Got unexpected SwapEffect %#x.\n", d3dpp.SwapEffect);
    ok(!d3dpp.hDeviceWindow, "Got unexpected hDeviceWindow %p.\n", d3dpp.hDeviceWindow);
    ok(d3dpp.Windowed, "Got unexpected Windowed %#x.\n", d3dpp.Windowed);
    ok(!d3dpp.EnableAutoDepthStencil, "Got unexpected EnableAutoDepthStencil %#x.\n", d3dpp.EnableAutoDepthStencil);
    ok(!d3dpp.AutoDepthStencilFormat, "Got unexpected AutoDepthStencilFormat %#x.\n", d3dpp.AutoDepthStencilFormat);
    ok(!d3dpp.Flags, "Got unexpected Flags %#lx.\n", d3dpp.Flags);
    ok(!d3dpp.FullScreen_RefreshRateInHz, "Got unexpected FullScreen_RefreshRateInHz %u.\n",
            d3dpp.FullScreen_RefreshRateInHz);
    ok(!d3dpp.PresentationInterval, "Got unexpected PresentationInterval %#x.\n", d3dpp.PresentationInterval);

    hr = IDirect3DDevice9Ex_GetScissorRect(device, &rect);
    ok(SUCCEEDED(hr), "Failed to get scissor rect, hr %#lx.\n", hr);
    ok(EqualRect(&rect, &client_rect), "Got unexpected scissor rect %s, expected %s.\n",
            wine_dbgstr_rect(&rect), wine_dbgstr_rect(&client_rect));

    hr = IDirect3DDevice9Ex_GetViewport(device, &vp);
    ok(SUCCEEDED(hr), "Failed to get viewport, hr %#lx.\n", hr);
    ok(vp.X == 0, "Got unexpected vp.X %lu.\n", vp.X);
    ok(vp.Y == 0, "Got unexpected vp.Y %lu.\n", vp.Y);
    ok(vp.Width == client_rect.right, "Got unexpected vp.Width %lu, expected %ld.\n",
            vp.Width, client_rect.right);
    ok(vp.Height == client_rect.bottom, "Got unexpected vp.Height %lu, expected %ld.\n",
            vp.Height, client_rect.bottom);
    ok(vp.MinZ == 2.0f, "Got unexpected vp.MinZ %.8e.\n", vp.MinZ);
    ok(vp.MaxZ == 3.0f, "Got unexpected vp.MaxZ %.8e.\n", vp.MaxZ);

    hr = IDirect3DDevice9Ex_GetSwapChain(device, 0, &swapchain);
    ok(SUCCEEDED(hr), "Failed to get swapchain, hr %#lx.\n", hr);
    hr = IDirect3DSwapChain9_GetPresentParameters(swapchain, &d3dpp);
    ok(SUCCEEDED(hr), "Failed to get present parameters, hr %#lx.\n", hr);
    ok(d3dpp.BackBufferWidth == client_rect.right,
            "Got unexpected backbuffer width %u, expected %ld.\n", d3dpp.BackBufferWidth, client_rect.right);
    ok(d3dpp.BackBufferHeight == client_rect.bottom,
            "Got unexpected backbuffer height %u, expected %ld.\n", d3dpp.BackBufferHeight, client_rect.bottom);
    ok(d3dpp.BackBufferFormat == mode.Format, "Got unexpected BackBufferFormat %#x, expected %#x.\n",
            d3dpp.BackBufferFormat, mode.Format);
    ok(d3dpp.BackBufferCount == 1, "Got unexpected BackBufferCount %u.\n", d3dpp.BackBufferCount);
    ok(!d3dpp.MultiSampleType, "Got unexpected MultiSampleType %u.\n", d3dpp.MultiSampleType);
    ok(!d3dpp.MultiSampleQuality, "Got unexpected MultiSampleQuality %lu.\n", d3dpp.MultiSampleQuality);
    ok(d3dpp.SwapEffect == D3DSWAPEFFECT_DISCARD, "Got unexpected SwapEffect %#x.\n", d3dpp.SwapEffect);
    ok(d3dpp.hDeviceWindow == window, "Got unexpected hDeviceWindow %p, expected %p.\n", d3dpp.hDeviceWindow, window);
    ok(d3dpp.Windowed, "Got unexpected Windowed %#x.\n", d3dpp.Windowed);
    ok(!d3dpp.EnableAutoDepthStencil, "Got unexpected EnableAutoDepthStencil %#x.\n", d3dpp.EnableAutoDepthStencil);
    ok(!d3dpp.AutoDepthStencilFormat, "Got unexpected AutoDepthStencilFormat %#x.\n", d3dpp.AutoDepthStencilFormat);
    ok(!d3dpp.Flags, "Got unexpected Flags %#lx.\n", d3dpp.Flags);
    ok(!d3dpp.FullScreen_RefreshRateInHz, "Got unexpected FullScreen_RefreshRateInHz %u.\n",
            d3dpp.FullScreen_RefreshRateInHz);
    ok(!d3dpp.PresentationInterval, "Got unexpected PresentationInterval %#x.\n", d3dpp.PresentationInterval);
    IDirect3DSwapChain9_Release(swapchain);

cleanup:
    free(modes);
    IDirect3D9Ex_Release(d3d9);
    refcount = IDirect3DDevice9Ex_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
    DestroyWindow(window);
}

static void test_reset_resources(void)
{
    IDirect3DSurface9 *surface, *rt;
    IDirect3DTexture9 *texture;
    IDirect3DDevice9Ex *device;
    unsigned int i;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;
    ULONG ref;

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#lx.\n", hr);

    hr = IDirect3DDevice9_CreateDepthStencilSurface(device, 128, 128, D3DFMT_D24S8,
            D3DMULTISAMPLE_NONE, 0, TRUE, &surface, NULL);
    ok(SUCCEEDED(hr), "Failed to create depth/stencil surface, hr %#lx.\n", hr);
    hr = IDirect3DDevice9_SetDepthStencilSurface(device, surface);
    ok(SUCCEEDED(hr), "Failed to set depth/stencil surface, hr %#lx.\n", hr);
    IDirect3DSurface9_Release(surface);

    for (i = 0; i < caps.NumSimultaneousRTs; ++i)
    {
        hr = IDirect3DDevice9_CreateTexture(device, 128, 128, 1, D3DUSAGE_RENDERTARGET,
                D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &texture, NULL);
        ok(SUCCEEDED(hr), "Failed to create render target texture %u, hr %#lx.\n", i, hr);
        hr = IDirect3DTexture9_GetSurfaceLevel(texture, 0, &surface);
        ok(SUCCEEDED(hr), "Failed to get surface %u, hr %#lx.\n", i, hr);
        IDirect3DTexture9_Release(texture);
        hr = IDirect3DDevice9_SetRenderTarget(device, i, surface);
        ok(SUCCEEDED(hr), "Failed to set render target surface %u, hr %#lx.\n", i, hr);
        IDirect3DSurface9_Release(surface);
    }

    hr = reset_device(device, NULL);
    ok(SUCCEEDED(hr), "Failed to reset device.\n");

    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &rt);
    ok(SUCCEEDED(hr), "Failed to get back buffer, hr %#lx.\n", hr);
    hr = IDirect3DDevice9_GetRenderTarget(device, 0, &surface);
    ok(SUCCEEDED(hr), "Failed to get render target surface, hr %#lx.\n", hr);
    ok(surface == rt, "Got unexpected surface %p for render target.\n", surface);
    IDirect3DSurface9_Release(surface);
    IDirect3DSurface9_Release(rt);

    for (i = 1; i < caps.NumSimultaneousRTs; ++i)
    {
        hr = IDirect3DDevice9_GetRenderTarget(device, i, &surface);
        ok(hr == D3DERR_NOTFOUND, "Got unexpected hr %#lx.\n", hr);
    }

    ref = IDirect3DDevice9_Release(device);
    ok(!ref, "Unexpected refcount %lu.\n", ref);

done:
    DestroyWindow(window);
}

static void test_vidmem_accounting(void)
{
    IDirect3DDevice9Ex *device;
    unsigned int i;
    HWND window;
    HRESULT hr = D3D_OK;
    ULONG ref;
    UINT vidmem_start, vidmem_end;
    INT diff;
    IDirect3DTexture9 *textures[20];

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    vidmem_start = IDirect3DDevice9_GetAvailableTextureMem(device);
    memset(textures, 0, sizeof(textures));
    for (i = 0; i < 20 && SUCCEEDED(hr); i++)
    {
        hr = IDirect3DDevice9_CreateTexture(device, 1024, 1024, 1, D3DUSAGE_RENDERTARGET,
                D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &textures[i], NULL);
        /* No D3DERR_OUTOFVIDEOMEMORY in d3d9ex */
        ok(SUCCEEDED(hr) || hr == E_OUTOFMEMORY, "Failed to create texture, hr %#lx.\n", hr);
    }
    vidmem_end = IDirect3DDevice9_GetAvailableTextureMem(device);

    diff = vidmem_start - vidmem_end;
    diff = abs(diff);
    ok(diff < 1024 * 1024, "Expected a video memory difference of less than 1 MB, got %u MB.\n",
            diff / 1024 / 1024);

    for (i = 0; i < 20; i++)
    {
        if (textures[i])
            IDirect3DTexture9_Release(textures[i]);
    }

    ref = IDirect3DDevice9_Release(device);
    ok(!ref, "Unexpected refcount %lu.\n", ref);

done:
    DestroyWindow(window);
}

static void test_user_memory_getdc(void)
{
    IDirect3DDevice9Ex *device;
    unsigned int *data;
    HBITMAP bitmap;
    DIBSECTION dib;
    HWND window;
    HRESULT hr;
    ULONG ref;
    int size;
    IDirect3DSurface9 *surface;
    HDC dc;

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    data = malloc(sizeof(*data) * 16 * 16);
    memset(data, 0xaa, sizeof(*data) * 16 * 16);
    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurface(device, 16, 16,
            D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &surface, (HANDLE *)&data);
    ok(SUCCEEDED(hr), "Failed to create surface, hr %#lx.\n", hr);

    hr = IDirect3DSurface9_GetDC(surface, &dc);
    ok(SUCCEEDED(hr), "Failed to get dc, hr %#lx.\n", hr);
    bitmap = GetCurrentObject(dc, OBJ_BITMAP);
    ok(!!bitmap, "Failed to get bitmap.\n");
    size = GetObjectA(bitmap, sizeof(dib), &dib);
    ok(size == sizeof(dib), "Got unexpected size %d.\n", size);
    ok(dib.dsBm.bmBits == data, "Got unexpected bits %p, expected %p.\n", dib.dsBm.bmBits, data);
    BitBlt(dc, 0, 0, 16, 8, NULL, 0, 0, WHITENESS);
    BitBlt(dc, 0, 8, 16, 8, NULL, 0, 0, BLACKNESS);
    hr = IDirect3DSurface9_ReleaseDC(surface, dc);
    ok(SUCCEEDED(hr), "Failed to release dc, hr %#lx.\n", hr);

    ok(data[0] == 0xffffffff, "Expected color 0xffffffff, got %#x.\n", data[0]);
    ok(data[8 * 16] == 0x00000000, "Expected color 0x00000000, got %#x.\n", data[8 * 16]);

    IDirect3DSurface9_Release(surface);
    free(data);

    ref = IDirect3DDevice9_Release(device);
    ok(!ref, "Unexpected refcount %lu.\n", ref);

done:
    DestroyWindow(window);
}

static void test_lost_device(void)
{
    IDirect3DDevice9Ex *device;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    BOOL ret;
    struct device_desc desc;

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    desc.device_window = window;
    desc.width = 640;
    desc.height = 480;
    desc.flags = CREATE_DEVICE_FULLSCREEN;
    if (!(device = create_device(window, &desc)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_PresentEx(device, NULL, NULL, NULL, NULL, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, window);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, NULL);
    ok(hr == S_PRESENT_OCCLUDED || broken(hr == D3D_OK), "Got unexpected hr %#lx.\n", hr);

    ret = SetForegroundWindow(GetDesktopWindow());
    ok(ret, "Failed to set foreground window.\n");
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == S_PRESENT_OCCLUDED || hr == S_PRESENT_MODE_CHANGED || broken(hr == D3D_OK),
            "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_PresentEx(device, NULL, NULL, NULL, NULL, 0);
    ok(hr == S_PRESENT_OCCLUDED || hr == S_PRESENT_MODE_CHANGED, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, window);
    ok(hr == S_PRESENT_OCCLUDED, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, NULL);
    ok(hr == D3D_OK || hr == S_PRESENT_MODE_CHANGED, "Got unexpected hr %#lx.\n", hr);

    ret = SetForegroundWindow(window);
    ok(ret, "Failed to set foreground window.\n");
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_PresentEx(device, NULL, NULL, NULL, NULL, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, window);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, NULL);
    ok(hr == S_PRESENT_OCCLUDED || hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    desc.width = 1024;
    desc.height = 768;
    hr = reset_device(device, &desc);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_PresentEx(device, NULL, NULL, NULL, NULL, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, window);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, NULL);
    ok(hr == S_PRESENT_OCCLUDED || hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    desc.flags = 0;
    hr = reset_device(device, &desc);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == S_PRESENT_MODE_CHANGED || hr == D3D_OK /* Win10 */, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_PresentEx(device, NULL, NULL, NULL, NULL, 0);
    ok(hr == S_PRESENT_MODE_CHANGED || hr == D3D_OK /* Win10 */, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, window);
    ok(hr == S_PRESENT_MODE_CHANGED || hr == D3D_OK /* Win10 */, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, NULL);
    ok(hr == S_PRESENT_MODE_CHANGED || hr == D3D_OK /* Win10 */, "Got unexpected hr %#lx.\n", hr);

    hr = reset_device(device, &desc);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_PresentEx(device, NULL, NULL, NULL, NULL, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, window);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    ret = SetForegroundWindow(GetDesktopWindow());
    ok(ret, "Failed to set foreground window.\n");
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_PresentEx(device, NULL, NULL, NULL, NULL, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, window);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    ret = SetForegroundWindow(window);
    ok(ret, "Failed to set foreground window.\n");
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_PresentEx(device, NULL, NULL, NULL, NULL, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, window);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    desc.flags = CREATE_DEVICE_FULLSCREEN;
    hr = reset_device(device, &desc);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_PresentEx(device, NULL, NULL, NULL, NULL, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, window);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, NULL);
    ok(hr == S_PRESENT_OCCLUDED || hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    ret = SetForegroundWindow(GetDesktopWindow());
    ok(ret, "Failed to set foreground window.\n");
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == S_PRESENT_OCCLUDED || broken(hr == D3D_OK), "Got unexpected hr %#lx.\n", hr);
    hr = reset_device(device, &desc);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_TestCooperativeLevel(device);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK || broken(hr == S_FALSE), "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_PresentEx(device, NULL, NULL, NULL, NULL, 0);
    ok(hr == D3D_OK || broken(hr == S_FALSE), "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, window);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CheckDeviceState(device, NULL);
    ok(hr == S_PRESENT_OCCLUDED || hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    refcount = IDirect3DDevice9Ex_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
done:
    DestroyWindow(window);
}

static void test_unsupported_shaders(void)
{
    static const DWORD simple_vs[] =
    {
        0xfffe0101,                             /* vs_1_1               */
        0x0000001f, 0x80000000, 0x900f0000,     /* dcl_position0 v0     */
        0x00000001, 0xc00f0000, 0x90e40000,     /* mov oPos, v0         */
        0x0000ffff,                             /* end                  */
    };
    static const DWORD simple_ps[] =
    {
        0xffff0101,                             /* ps_1_1               */
        0x00000001, 0x800f0000, 0x90e40000,     /* mul r0, t0, r0       */
        0x0000ffff,                             /* end                  */
    };
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
    IDirect3DDevice9Ex *device;
    ULONG refcount;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;

    window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, 0, 0, 0, 0);
    if (!(device = create_device(window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9Ex_CreateVertexShader(device, simple_ps, &vs);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CreatePixelShader(device, simple_vs, &ps);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CreatePixelShader(device, ps_4_0, &ps);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get device caps, hr %#lx.\n", hr);

    if (caps.VertexShaderVersion < D3DVS_VERSION(3, 0))
    {
        hr = IDirect3DDevice9Ex_CreateVertexShader(device, vs_3_0, &vs);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
        if (caps.VertexShaderVersion <= D3DVS_VERSION(1, 1) && caps.MaxVertexShaderConst < 256)
        {
            hr = IDirect3DDevice9Ex_CreateVertexShader(device, vs_1_255, &vs);
            ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
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

        hr = IDirect3DDevice9Ex_CreateVertexShader(device, vs_1_255, &vs);
        ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
        IDirect3DVertexShader9_Release(vs);
        hr = IDirect3DDevice9Ex_CreateVertexShader(device, vs_1_256, &vs);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
        hr = IDirect3DDevice9Ex_CreateVertexShader(device, vs_3_256, &vs);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
        hr = IDirect3DDevice9Ex_CreateVertexShader(device, vs_3_i16, &vs);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
        hr = IDirect3DDevice9Ex_CreateVertexShader(device, vs_3_b16, &vs);
        ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    }

    if (caps.PixelShaderVersion < D3DPS_VERSION(3, 0))
    {
        skip("This GPU doesn't support SM3, skipping test with shader using unsupported constants.\n");
        goto cleanup;
    }
    hr = IDirect3DDevice9Ex_CreatePixelShader(device, ps_1_8, &ps);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CreatePixelShader(device, ps_2_32, &ps);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CreatePixelShader(device, ps_3_224, &ps);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CreatePixelShader(device, ps_2_0_boolint, &ps);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    if (ps)
        IDirect3DPixelShader9_Release(ps);

cleanup:
    refcount = IDirect3DDevice9Ex_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
    DestroyWindow(window);
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
static LONG windowposchanged_received, syscommand_received;
static IDirect3DDevice9Ex *focus_test_device;

struct wndproc_thread_param
{
    HWND dummy_window;
    HANDLE window_created;
    HANDLE test_finished;
};

static LRESULT CALLBACK test_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    HRESULT hr;

    if (filter_messages && filter_messages == hwnd)
    {
        if (message != WM_DISPLAYCHANGE && message != WM_IME_NOTIFY)
            todo_wine ok(0, "Received unexpected message %#x for window %p.\n", message, hwnd);
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
                        "Got unexpected wparam %#Ix for message %#x, expected %#Ix.\n",
                        wparam, message, expect_messages->expect_wparam);

            if (expect_messages->store_wp)
                *expect_messages->store_wp = *(WINDOWPOS *)lparam;

            if (focus_test_device)
            {
                hr = IDirect3DDevice9Ex_CheckDeviceState(focus_test_device, device_window);
                todo_wine_if(message != WM_ACTIVATEAPP && message != WM_DISPLAYCHANGE)
                    ok(hr == expect_messages->device_state,
                        "Got device state %#lx on message %#x, expected %#lx.\n",
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

    return DefWindowProcA(hwnd, message, wparam, lparam);
}

static DWORD WINAPI wndproc_thread(void *param)
{
    struct wndproc_thread_param *p = param;
    DWORD res;
    BOOL ret;

    p->dummy_window = CreateWindowA("static", "d3d9_test", WS_VISIBLE | WS_CAPTION,
            100, 100, 200, 200, 0, 0, 0, 0);
    flush_events();

    ret = SetEvent(p->window_created);
    ok(ret, "SetEvent failed, last error %#lx.\n", GetLastError());

    for (;;)
    {
        MSG msg;

        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
            DispatchMessageA(&msg);
        res = WaitForSingleObject(p->test_finished, 100);
        if (res == WAIT_OBJECT_0)
            break;
        if (res != WAIT_TIMEOUT)
        {
            ok(0, "Wait failed (%#lx), last error %#lx.\n", res, GetLastError());
            break;
        }
    }

    DestroyWindow(p->dummy_window);

    return 0;
}

static void test_wndproc(void)
{
    unsigned int adapter_mode_count, i, d3d_width = 0, d3d_height = 0, user32_width = 0, user32_height = 0;
    struct wndproc_thread_param thread_params;
    struct device_desc device_desc;
    static WINDOWPOS windowpos;
    IDirect3DDevice9Ex *device;
    WNDCLASSA wc = {0};
    HANDLE thread;
    LONG_PTR proc;
    ULONG ref;
    DWORD res, tid;
    HWND tmp;
    HRESULT hr;
    D3DDISPLAYMODE d3ddm;
    DEVMODEW devmode;
    LONG change_ret, device_style;
    BOOL ret;
    IDirect3D9Ex *d3d9ex;

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

        /* The S_PRESENT_MODE_CHANGED state is only there because we change the mode
         * before dropping focus. From the base d3d9 tests one might expect d3d9ex to
         * pick up the mode change by the time we receive WM_DISPLAYCHANGE, but as
         * the tests below show, a present call is needed for that to happen. I don't
         * want to call present in a focus loss message handler until we have an app
         * that does that. Without the previous change (+present) the state would be
         * D3D_OK all the way until the WM_ACTIVATEAPP message. */
        {WM_DISPLAYCHANGE,      DEVICE_WINDOW,  FALSE,  0,              S_PRESENT_MODE_CHANGED},
        /* WM_DISPLAYCHANGE is sent to the focus window too, but the order is
         * not deterministic. */
        {WM_WINDOWPOSCHANGING,  DEVICE_WINDOW,  FALSE,  0,              S_PRESENT_MODE_CHANGED},
        /* Windows sends WM_ACTIVATE to the device window, indicating that
         * SW_SHOWMINIMIZED is used instead of SW_MINIMIZE. Yet afterwards
         * the foreground and focus window are NULL. On Wine SW_SHOWMINIMIZED
         * leaves the device window active, breaking re-activation in the
         * lost device test.
         * {WM_ACTIVATE,           DEVICE_WINDOW,  TRUE,   0x200000 | WA_ACTIVE}, */
        {WM_WINDOWPOSCHANGED,   DEVICE_WINDOW,  FALSE,  0,              S_PRESENT_MODE_CHANGED},
        {WM_SIZE,               DEVICE_WINDOW,  TRUE,   SIZE_MINIMIZED, S_PRESENT_MODE_CHANGED},
        {WM_ACTIVATEAPP,        FOCUS_WINDOW,   TRUE,   FALSE,          S_PRESENT_OCCLUDED},
        /* WM_ACTIVATEAPP is sent to the device window too, but the order is
         * not deterministic. It may be sent after the focus window handling
         * or before. */
        {0,                     0,              FALSE,  0,              0},
    };
    static const struct message focus_loss_messages_nowc[] =
    {
        /* WM_ACTIVATE (wparam = WA_INACTIVE) is sent on Windows. It is
         * not reliable on X11 WMs. When the window focus follows the
         * mouse pointer the message is not sent.
         * {WM_ACTIVATE,           FOCUS_WINDOW,   TRUE,   WA_INACTIVE}, */
        {WM_DISPLAYCHANGE,      DEVICE_WINDOW,  FALSE,  0},
        {WM_ACTIVATEAPP,        FOCUS_WINDOW,   TRUE,   FALSE},
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
        const struct message *focus_loss_messages;
        const struct message *mode_change_messages, *mode_change_messages_hidden;
        BOOL iconic;
    }
    tests[] =
    {
        {
            0,
            focus_loss_messages,
            mode_change_messages,
            mode_change_messages_hidden,
            TRUE
        },
        {
            CREATE_DEVICE_NOWINDOWCHANGES,
            focus_loss_messages_nowc,
            mode_change_messages_nowc,
            mode_change_messages_nowc,
            FALSE
        },
    };

    hr = pDirect3DCreate9Ex(D3D_SDK_VERSION, &d3d9ex);
    if (FAILED(hr))
    {
        skip("Failed to create IDirect3D9Ex, hr %#lx.\n", hr);
        return;
    }

    adapter_mode_count = IDirect3D9Ex_GetAdapterModeCount(d3d9ex, D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8);
    for (i = 0; i < adapter_mode_count; ++i)
    {
        hr = IDirect3D9Ex_EnumAdapterModes(d3d9ex, D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8, i, &d3ddm);
        ok(SUCCEEDED(hr), "Failed to enumerate display mode, hr %#lx.\n", hr);

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

    IDirect3D9Ex_Release(d3d9ex);

    if (!d3d_width)
    {
        skip("Could not find adequate modes, skipping mode tests.\n");
        return;
    }

    filter_messages = NULL;
    expect_messages = NULL;

    wc.lpfnWndProc = test_proc;
    wc.lpszClassName = "d3d9_test_wndproc_wc";
    ok(RegisterClassA(&wc), "Failed to register window class.\n");

    thread_params.window_created = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(!!thread_params.window_created, "CreateEvent failed, last error %#lx.\n", GetLastError());
    thread_params.test_finished = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(!!thread_params.test_finished, "CreateEvent failed, last error %#lx.\n", GetLastError());

    memset(&devmode, 0, sizeof(devmode));
    devmode.dmSize = sizeof(devmode);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
        devmode.dmPelsWidth = user32_width;
        devmode.dmPelsHeight = user32_height;
        change_ret = ChangeDisplaySettingsW(&devmode, CDS_FULLSCREEN);
        ok(change_ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#lx.\n", change_ret);

        focus_window = CreateWindowA("d3d9_test_wndproc_wc", "d3d9_test",
                WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION, 0, 0, user32_width, user32_height, 0, 0, 0, 0);
        device_window = CreateWindowA("d3d9_test_wndproc_wc", "d3d9_test",
                WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION, 0, 0, user32_width, user32_height, 0, 0, 0, 0);
        flush_events();

        thread = CreateThread(NULL, 0, wndproc_thread, &thread_params, 0, &tid);
        ok(!!thread, "Failed to create thread, last error %#lx.\n", GetLastError());

        res = WaitForSingleObject(thread_params.window_created, INFINITE);
        ok(res == WAIT_OBJECT_0, "Wait failed (%#lx), last error %#lx.\n", res, GetLastError());
        flush_events();

        proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
        ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#Ix, got %#Ix.\n",
                (LONG_PTR)test_proc, proc);
        proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
        ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#Ix, got %#Ix.\n",
                (LONG_PTR)test_proc, proc);

        trace("device_window %p, focus_window %p, dummy_window %p.\n",
                device_window, focus_window, thread_params.dummy_window);

        tmp = GetFocus();
        ok(tmp == NULL, "Expected focus %p, got %p.\n", NULL, tmp);
        tmp = GetForegroundWindow();
        ok(tmp == thread_params.dummy_window, "Expected foreground window %p, got %p.\n",
                thread_params.dummy_window, tmp);

        flush_events();

        expect_messages = create_messages;

        device_desc.device_window = device_window;
        device_desc.width = d3d_width;
        device_desc.height = d3d_height;
        device_desc.flags = CREATE_DEVICE_FULLSCREEN | tests[i].create_flags;
        if (!(device = create_device(focus_window, &device_desc)))
        {
            skip("Failed to create a D3D device, skipping tests.\n");
            goto done;
        }

        ok(!expect_messages->message, "Expected message %#x for window %#x, but didn't receive it, i=%u.\n",
                expect_messages->message, expect_messages->window, i);
        expect_messages = NULL;

        tmp = GetFocus();
        ok(tmp == focus_window, "Expected focus %p, got %p.\n", focus_window, tmp);
        tmp = GetForegroundWindow();
        ok(tmp == focus_window, "Expected foreground window %p, got %p.\n", focus_window, tmp);
        SetForegroundWindow(focus_window);
        flush_events();

        proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
        ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#Ix, got %#Ix.\n",
                (LONG_PTR)test_proc, proc);

        proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
        ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#Ix.\n",
                (LONG_PTR)test_proc);

        /* Change the mode while the device is in use and then drop focus. */
        devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
        devmode.dmPelsWidth = user32_width;
        devmode.dmPelsHeight = user32_height;
        change_ret = ChangeDisplaySettingsW(&devmode, CDS_FULLSCREEN);
        ok(change_ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#lx, i=%u.\n", change_ret, i);

        /* Native needs a present call to pick up the mode change. Windows 10 15.07 never picks up the mode change
         * in these calls and returns S_OK. This is a regression from Windows 8 and has been fixed in later Win10
         * builds. */
        hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
        todo_wine ok(hr == S_PRESENT_MODE_CHANGED || broken(hr == S_OK), "Got unexpected hr %#lx, i=%u.\n", hr, i);
        hr = IDirect3DDevice9Ex_CheckDeviceState(device, device_window);
        todo_wine ok(hr == S_PRESENT_MODE_CHANGED || broken(hr == S_OK), "Got unexpected hr %#lx, i=%u.\n", hr, i);

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

        hr = IDirect3DDevice9Ex_CheckDeviceState(device, device_window);
        ok(hr == S_PRESENT_OCCLUDED, "Got unexpected hr %#lx, i=%u.\n", hr, i);

        ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devmode);
        ok(ret, "Failed to get display mode.\n");
        ok(devmode.dmPelsWidth == registry_mode.dmPelsWidth
                && devmode.dmPelsHeight == registry_mode.dmPelsHeight, "Got unexpected screen size %lux%lu.\n",
                devmode.dmPelsWidth, devmode.dmPelsHeight);

        /* In d3d9ex the device and focus windows have to be minimized and restored,
         * otherwise native does not notice that focus has been restored. This is
         * independent of D3DCREATE_NOWINDOWCHANGES. */
        ShowWindow(device_window, SW_MINIMIZE);
        ShowWindow(device_window, SW_RESTORE);

        /* Reactivation messages like in d3d8/9 are random in native d3d9ex.
         * Sometimes they are sent, sometimes they are not (tested on Vista
         * and Windows 7). The minimizing and restoring of the device window
         * may have something to do with this, but if the messages are sent,
         * they are generated by the 3 calls below. */
        ShowWindow(focus_window, SW_MINIMIZE);
        ShowWindow(focus_window, SW_RESTORE);
        /* Set focus twice to make KDE and fvwm in focus-follows-mouse mode happy. */
        SetForegroundWindow(focus_window);
        flush_events();
        SetForegroundWindow(focus_window);
        flush_events();

        /* Calling Reset is not necessary in d3d9ex. */
        hr = IDirect3DDevice9Ex_CheckDeviceState(device, device_window);
        ok(hr == S_OK, "Got unexpected hr %#lx, i=%u.\n", hr, i);

        ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devmode);
        ok(ret, "Failed to get display mode.\n");
        ok(devmode.dmPelsWidth == d3d_width
                && devmode.dmPelsHeight == d3d_height, "Got unexpected screen size %lux%lu.\n",
                devmode.dmPelsWidth, devmode.dmPelsHeight);

        hr = reset_device(device, &device_desc);
        ok(SUCCEEDED(hr), "Failed to reset device, hr %#lx.\n", hr);

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
        ok(!windowposchanged_received, "Received WM_WINDOWPOSCHANGED but did not expect it, i=%u.\n", i);

        expect_messages = NULL;

        ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devmode);
        ok(ret, "Failed to get display mode.\n");
        ok(devmode.dmPelsWidth == registry_mode.dmPelsWidth, "Got unexpected width %lu.\n", devmode.dmPelsWidth);
        ok(devmode.dmPelsHeight == registry_mode.dmPelsHeight, "Got unexpected height %lu.\n", devmode.dmPelsHeight);

        flush_events();

        ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &devmode);
        ok(ret, "Failed to get display mode.\n");
        ok(devmode.dmPelsWidth == registry_mode.dmPelsWidth,
                "Got unexpected width %lu.\n", devmode.dmPelsWidth);
        ok(devmode.dmPelsHeight == registry_mode.dmPelsHeight,
                "Got unexpected height %lu.\n", devmode.dmPelsHeight);

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
        ok(syscommand_received == 1, "Got unexpected number of WM_SYSCOMMAND messages: %ld.\n", syscommand_received);
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

        filter_messages = focus_window;
        ref = IDirect3DDevice9Ex_Release(device);
        ok(!ref, "Unexpected refcount %lu, i=%u.\n", ref, i);

        /* Fix up the mode until Wine's device release behavior is fixed. */
        change_ret = ChangeDisplaySettingsW(NULL, CDS_FULLSCREEN);
        ok(change_ret == DISP_CHANGE_SUCCESSFUL, "Failed to change display mode, ret %#lx.\n", change_ret);

        proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
        ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#Ix, got %#Ix, i=%u.\n",
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
        if (!(device = create_device(focus_window, &device_desc)))
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
        flaky_wine
        ok(!expect_messages->message, "Expected message %#x for window %#x, but didn't receive it, i=%u.\n",
                expect_messages->message, expect_messages->window, i);
        ok(!windowposchanged_received, "Received WM_WINDOWPOSCHANGED but did not expect it, i=%u.\n", i);

        expect_messages = NULL;

        /* The window is iconic even though no message was sent. */
        ok(!IsIconic(focus_window) == !tests[i].iconic,
                "Expected IsIconic %u, got %u, i=%u.\n", tests[i].iconic, IsIconic(focus_window), i);

        ShowWindow(focus_window, SW_SHOWNOACTIVATE);
        flush_events();
        ShowWindow(focus_window, SW_SHOWMINNOACTIVE);
        flush_events();

        syscommand_received = 0;
        expect_messages = sc_restore_messages;
        SendMessageA(focus_window, WM_SYSCOMMAND, SC_RESTORE, 0);
        flaky_wine
        ok(!expect_messages->message, "Expected message %#x for window %#x, but didn't receive it, i=%u.\n",
                expect_messages->message, expect_messages->window, i);
        flaky
        ok(syscommand_received == 1, "Got unexpected number of WM_SYSCOMMAND messages: %ld.\n", syscommand_received);
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

        /* This test can't activate, drop focus and restore focus like in plain d3d9 because d3d9ex
         * immediately restores the device on activation. There are plenty of WM_WINDOWPOSCHANGED
         * messages that are generated by ShowWindow, so testing for their absence is pointless. */
        ShowWindow(focus_window, SW_MINIMIZE);
        flush_events();
        ShowWindow(focus_window, SW_RESTORE);
        flush_events();
        SetForegroundWindow(focus_window);
        flush_events();

        filter_messages = focus_window;
        ref = IDirect3DDevice9Ex_Release(device);
        ok(!ref, "Unexpected refcount %lu, i=%u.\n", ref, i);

        device_desc.device_window = device_window;
        if (!(device = create_device(focus_window, &device_desc)))
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
        ok(SUCCEEDED(hr), "Failed to reset device, hr %#lx.\n", hr);
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
        ok(SUCCEEDED(hr), "Failed to reset device, hr %#lx.\n", hr);
        filter_messages = NULL;

        flush_events();
        ok(!expect_messages->message, "Expected message %#x for window %#x, but didn't receive it, i=%u.\n",
                expect_messages->message, expect_messages->window, i);
        expect_messages = NULL;

        if (!(tests[i].create_flags & CREATE_DEVICE_NOWINDOWCHANGES))
        {
            ok(windowpos.hwnd == device_window
                    && !windowpos.x && !windowpos.y && !windowpos.cx && !windowpos.cy
                    && windowpos.flags == (SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE),
                    "Got unexpected WINDOWPOS hwnd=%p, x=%d, y=%d, cx=%d, cy=%d, flags=%x\n",
                    windowpos.hwnd, windowpos.x, windowpos.y, windowpos.cx,
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
        ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#Ix.\n",
                (LONG_PTR)test_proc);

        ref = IDirect3DDevice9Ex_Release(device);
        ok(!ref, "Unexpected refcount %lu.\n", ref);

        proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
        ok(proc == (LONG_PTR)DefWindowProcA, "Expected wndproc %#Ix, got %#Ix.\n",
                (LONG_PTR)DefWindowProcA, proc);

done:
        filter_messages = NULL;
        expect_messages = NULL;
        DestroyWindow(device_window);
        DestroyWindow(focus_window);
        SetEvent(thread_params.test_finished);
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
    }

    CloseHandle(thread_params.test_finished);
    CloseHandle(thread_params.window_created);

    UnregisterClassA("d3d9_test_wndproc_wc", GetModuleHandleA(NULL));
}

static void test_wndproc_windowed(void)
{
    struct wndproc_thread_param thread_params;
    struct device_desc device_desc;
    IDirect3DDevice9Ex *device;
    WNDCLASSA wc = {0};
    HANDLE thread;
    LONG_PTR proc;
    HRESULT hr;
    ULONG ref;
    DWORD res, tid;
    HWND tmp;

    filter_messages = NULL;
    expect_messages = NULL;

    wc.lpfnWndProc = test_proc;
    wc.lpszClassName = "d3d9_test_wndproc_wc";
    ok(RegisterClassA(&wc), "Failed to register window class.\n");

    thread_params.window_created = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(!!thread_params.window_created, "CreateEvent failed, last error %#lx.\n", GetLastError());
    thread_params.test_finished = CreateEventA(NULL, FALSE, FALSE, NULL);
    ok(!!thread_params.test_finished, "CreateEvent failed, last error %#lx.\n", GetLastError());

    focus_window = CreateWindowA("d3d9_test_wndproc_wc", "d3d9_test",
            WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION, 0, 0, registry_mode.dmPelsWidth,
            registry_mode.dmPelsHeight, 0, 0, 0, 0);
    device_window = CreateWindowA("d3d9_test_wndproc_wc", "d3d9_test",
            WS_MAXIMIZE | WS_VISIBLE | WS_CAPTION, 0, 0, registry_mode.dmPelsWidth,
            registry_mode.dmPelsHeight, 0, 0, 0, 0);
    flush_events();

    thread = CreateThread(NULL, 0, wndproc_thread, &thread_params, 0, &tid);
    ok(!!thread, "Failed to create thread, last error %#lx.\n", GetLastError());

    res = WaitForSingleObject(thread_params.window_created, INFINITE);
    ok(res == WAIT_OBJECT_0, "Wait failed (%#lx), last error %#lx.\n", res, GetLastError());
    flush_events();

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#Ix, got %#Ix.\n",
            (LONG_PTR)test_proc, proc);
    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#Ix, got %#Ix.\n",
            (LONG_PTR)test_proc, proc);

    trace("device_window %p, focus_window %p, dummy_window %p.\n",
            device_window, focus_window, thread_params.dummy_window);

    tmp = GetFocus();
    ok(tmp == NULL, "Expected focus %p, got %p.\n", NULL, tmp);
    tmp = GetForegroundWindow();
    ok(tmp == thread_params.dummy_window, "Expected foreground window %p, got %p.\n",
            thread_params.dummy_window, tmp);

    filter_messages = focus_window;

    device_desc.device_window = device_window;
    device_desc.width = registry_mode.dmPelsWidth;
    device_desc.height = registry_mode.dmPelsHeight;
    device_desc.flags = 0;
    if (!(device = create_device(focus_window, &device_desc)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    tmp = GetFocus();
    ok(tmp == NULL, "Expected focus %p, got %p.\n", NULL, tmp);
    tmp = GetForegroundWindow();
    ok(tmp == thread_params.dummy_window, "Expected foreground window %p, got %p.\n",
            thread_params.dummy_window, tmp);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#Ix, got %#Ix.\n",
            (LONG_PTR)test_proc, proc);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#Ix, got %#Ix.\n",
            (LONG_PTR)test_proc, proc);

    filter_messages = NULL;

    device_desc.flags = CREATE_DEVICE_FULLSCREEN;
    hr = reset_device(device, &device_desc);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#lx.\n", hr);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#Ix, got %#Ix.\n",
            (LONG_PTR)test_proc, proc);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#Ix.\n", (LONG_PTR)test_proc);

    device_desc.flags = 0;
    hr = reset_device(device, &device_desc);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#lx.\n", hr);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#Ix, got %#Ix.\n",
            (LONG_PTR)test_proc, proc);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#Ix, got %#Ix.\n",
            (LONG_PTR)test_proc, proc);

    filter_messages = focus_window;

    ref = IDirect3DDevice9Ex_Release(device);
    ok(!ref, "Unexpected refcount %lu.\n", ref);

    filter_messages = device_window;

    device_desc.device_window = focus_window;
    if (!(device = create_device(focus_window, &device_desc)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    filter_messages = NULL;

    device_desc.flags = CREATE_DEVICE_FULLSCREEN;
    hr = reset_device(device, &device_desc);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#lx.\n", hr);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#Ix, got %#Ix.\n",
            (LONG_PTR)test_proc, proc);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#Ix.\n", (LONG_PTR)test_proc);

    device_desc.flags = 0;
    hr = reset_device(device, &device_desc);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#lx.\n", hr);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#Ix, got %#Ix.\n",
            (LONG_PTR)test_proc, proc);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#Ix, got %#Ix.\n",
            (LONG_PTR)test_proc, proc);

    filter_messages = device_window;

    ref = IDirect3DDevice9Ex_Release(device);
    ok(!ref, "Unexpected refcount %lu.\n", ref);

    device_desc.device_window = device_window;
    if (!(device = create_device(focus_window, &device_desc)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    filter_messages = NULL;

    device_desc.flags = CREATE_DEVICE_FULLSCREEN;
    hr = reset_device(device, &device_desc);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#lx.\n", hr);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#Ix, got %#Ix.\n",
            (LONG_PTR)test_proc, proc);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc != (LONG_PTR)test_proc, "Expected wndproc != %#Ix.\n", (LONG_PTR)test_proc);

    device_desc.flags = 0;
    hr = reset_device(device, &device_desc);
    ok(SUCCEEDED(hr), "Failed to reset device, hr %#lx.\n", hr);

    proc = GetWindowLongPtrA(device_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#Ix, got %#Ix.\n",
            (LONG_PTR)test_proc, proc);

    proc = GetWindowLongPtrA(focus_window, GWLP_WNDPROC);
    ok(proc == (LONG_PTR)test_proc, "Expected wndproc %#Ix, got %#Ix.\n",
            (LONG_PTR)test_proc, proc);

    filter_messages = device_window;

    ref = IDirect3DDevice9Ex_Release(device);
    ok(!ref, "Unexpected refcount %lu.\n", ref);

done:
    filter_messages = NULL;

    SetEvent(thread_params.test_finished);
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread_params.test_finished);
    CloseHandle(thread_params.window_created);
    CloseHandle(thread);

    DestroyWindow(device_window);
    DestroyWindow(focus_window);
    UnregisterClassA("d3d9_test_wndproc_wc", GetModuleHandleA(NULL));
}

static void test_window_style(void)
{
    RECT focus_rect, device_rect, fullscreen_rect, r, r2;
    LONG device_style, device_exstyle, expected_style;
    LONG focus_style, focus_exstyle;
    struct device_desc device_desc;
    LONG style;
    IDirect3DDevice9Ex *device;
    HRESULT hr;
    ULONG ref;
    BOOL ret;
    static const struct
    {
        LONG style_flags;
        DWORD device_flags;
        LONG focus_loss_style;
        LONG create2_style, create2_exstyle;
    }
    tests[] =
    {
        {0,             0,                              0,              WS_VISIBLE, WS_EX_TOPMOST},
        {WS_VISIBLE,    0,                              WS_MINIMIZE,    WS_VISIBLE, WS_EX_TOPMOST},
        {0,             CREATE_DEVICE_NOWINDOWCHANGES,  0,              0,          0},
        {WS_VISIBLE,    CREATE_DEVICE_NOWINDOWCHANGES,  0,              0,          0},
    };
    unsigned int i;

    SetRect(&fullscreen_rect, 0, 0, registry_mode.dmPelsWidth, registry_mode.dmPelsHeight);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        focus_window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW | tests[i].style_flags,
                0, 0, registry_mode.dmPelsWidth / 2, registry_mode.dmPelsHeight / 2, 0, 0, 0, 0);
        device_window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW | tests[i].style_flags,
                0, 0, registry_mode.dmPelsWidth / 2, registry_mode.dmPelsHeight / 2, 0, 0, 0, 0);

        device_style = GetWindowLongA(device_window, GWL_STYLE);
        device_exstyle = GetWindowLongA(device_window, GWL_EXSTYLE);
        focus_style = GetWindowLongA(focus_window, GWL_STYLE);
        focus_exstyle = GetWindowLongA(focus_window, GWL_EXSTYLE);

        GetWindowRect(focus_window, &focus_rect);
        GetWindowRect(device_window, &device_rect);

        device_desc.device_window = device_window;
        device_desc.width = registry_mode.dmPelsWidth;
        device_desc.height = registry_mode.dmPelsHeight;
        device_desc.flags = CREATE_DEVICE_FULLSCREEN | tests[i].device_flags;
        if (!(device = create_device(focus_window, &device_desc)))
        {
            skip("Failed to create a D3D device, skipping tests.\n");
            DestroyWindow(device_window);
            DestroyWindow(focus_window);
            return;
        }

        style = GetWindowLongA(device_window, GWL_STYLE);
        expected_style = device_style;
        todo_wine ok(style == expected_style || broken(style == (expected_style & ~WS_OVERLAPPEDWINDOW)) /* w1064v1809 */,
                "Expected device window style %#lx, got %#lx, i=%u.\n",
                expected_style, style, i);
        style = GetWindowLongA(device_window, GWL_EXSTYLE);
        expected_style = device_exstyle;
        todo_wine ok(style == expected_style || broken(style == (expected_style & ~WS_EX_OVERLAPPEDWINDOW)) /* w1064v1809 */,
                "Expected device window extended style %#lx, got %#lx, i=%u.\n",
                expected_style, style, i);

        style = GetWindowLongA(focus_window, GWL_STYLE);
        ok(style == focus_style, "Expected focus window style %#lx, got %#lx, i=%u.\n",
                focus_style, style, i);
        style = GetWindowLongA(focus_window, GWL_EXSTYLE);
        ok(style == focus_exstyle, "Expected focus window extended style %#lx, got %#lx, i=%u.\n",
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
        ok(SUCCEEDED(hr), "Failed to reset device, hr %#lx.\n", hr);

        GetWindowRect(device_window, &r);
        if (tests[i].device_flags & CREATE_DEVICE_NOWINDOWCHANGES)
            todo_wine ok(EqualRect(&r, &device_rect), "Expected %s, got %s, i=%u.\n",
                    wine_dbgstr_rect(&device_rect), wine_dbgstr_rect(&r), i);
        else
            ok(EqualRect(&r, &fullscreen_rect), "Expected %s, got %s, i=%u.\n",
                    wine_dbgstr_rect(&fullscreen_rect), wine_dbgstr_rect(&r), i);

        style = GetWindowLongA(device_window, GWL_STYLE);
        expected_style = device_style;
        ok(style == expected_style, "Expected device window style %#lx, got %#lx, i=%u.\n",
                expected_style, style, i);
        style = GetWindowLongA(device_window, GWL_EXSTYLE);
        expected_style = device_exstyle;
        ok(style == expected_style, "Expected device window extended style %#lx, got %#lx, i=%u.\n",
                expected_style, style, i);

        style = GetWindowLongA(focus_window, GWL_STYLE);
        ok(style == focus_style, "Expected focus window style %#lx, got %#lx, i=%u.\n",
                focus_style, style, i);
        style = GetWindowLongA(focus_window, GWL_EXSTYLE);
        ok(style == focus_exstyle, "Expected focus window extended style %#lx, got %#lx, i=%u.\n",
                focus_exstyle, style, i);

        ref = IDirect3DDevice9Ex_Release(device);
        ok(!ref, "Unexpected refcount %lu.\n", ref);

        style = GetWindowLongA(device_window, GWL_STYLE);
        expected_style = device_style;
        ok(style == expected_style, "Expected device window style %#lx, got %#lx, i=%u.\n",
                expected_style, style, i);
        style = GetWindowLongA(device_window, GWL_EXSTYLE);
        expected_style = device_exstyle;
        ok(style == expected_style, "Expected device window extended style %#lx, got %#lx, i=%u.\n",
                expected_style, style, i);

        style = GetWindowLongA(focus_window, GWL_STYLE);
        ok(style == focus_style, "Expected focus window style %#lx, got %#lx, i=%u.\n",
                focus_style, style, i);
        style = GetWindowLongA(focus_window, GWL_EXSTYLE);
        ok(style == focus_exstyle, "Expected focus window extended style %#lx, got %#lx, i=%u.\n",
                focus_exstyle, style, i);

        /* The second time a device is created on the window the window becomes visible and
         * topmost if D3DCREATE_NOWINDOWCHANGES is not set. */
        device_desc.flags = CREATE_DEVICE_FULLSCREEN | tests[i].device_flags;
        device = create_device(focus_window, &device_desc);
        ok(!!device, "Failed to create a D3D device.\n");
        style = GetWindowLongA(device_window, GWL_STYLE);
        expected_style = device_style | tests[i].create2_style;
        todo_wine ok(style == expected_style || broken(style == (expected_style & ~WS_OVERLAPPEDWINDOW)) /* w1064v1809 */,
                "Expected device window style %#lx, got %#lx, i=%u.\n",
                expected_style, style, i);
        style = GetWindowLongA(device_window, GWL_EXSTYLE);
        expected_style = device_exstyle | tests[i].create2_exstyle;
        todo_wine ok(style == expected_style || broken(style == (expected_style & ~WS_EX_OVERLAPPEDWINDOW)) /* w1064v1809 */,
                "Expected device window extended style %#lx, got %#lx, i=%u.\n",
                expected_style, style, i);

        style = GetWindowLongA(focus_window, GWL_STYLE);
        ok(style == focus_style, "Expected focus window style %#lx, got %#lx, i=%u.\n",
                focus_style, style, i);
        style = GetWindowLongA(focus_window, GWL_EXSTYLE);
        ok(style == focus_exstyle, "Expected focus window extended style %#lx, got %#lx, i=%u.\n",
                focus_exstyle, style, i);
        ref = IDirect3DDevice9Ex_Release(device);
        ok(!ref, "Unexpected refcount %lu.\n", ref);

        DestroyWindow(device_window);
        DestroyWindow(focus_window);
        focus_window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW | tests[i].style_flags,
                0, 0, registry_mode.dmPelsWidth / 2, registry_mode.dmPelsHeight / 2, 0, 0, 0, 0);
        device_window = CreateWindowA("d3d9_test_wc", "d3d9_test", WS_OVERLAPPEDWINDOW | tests[i].style_flags,
                0, 0, registry_mode.dmPelsWidth / 2, registry_mode.dmPelsHeight / 2, 0, 0, 0, 0);

        device_desc.device_window = device_window;
        device = create_device(focus_window, &device_desc);
        ok(!!device, "Failed to create a D3D device.\n");
        ret = SetForegroundWindow(GetDesktopWindow());
        ok(ret, "Failed to set foreground window.\n");

        style = GetWindowLongA(device_window, GWL_STYLE);
        expected_style = device_style | tests[i].focus_loss_style;
        todo_wine ok(style == expected_style, "Expected device window style %#lx, got %#lx, i=%u.\n",
                expected_style, style, i);
        style = GetWindowLongA(device_window, GWL_EXSTYLE);
        expected_style = device_exstyle;
        todo_wine ok(style == expected_style, "Expected device window extended style %#lx, got %#lx, i=%u.\n",
                expected_style, style, i);

        style = GetWindowLongA(focus_window, GWL_STYLE);
        ok(style == focus_style, "Expected focus window style %#lx, got %#lx, i=%u.\n",
                focus_style, style, i);
        style = GetWindowLongA(focus_window, GWL_EXSTYLE);
        ok(style == focus_exstyle, "Expected focus window extended style %#lx, got %#lx, i=%u.\n",
                focus_exstyle, style, i);

        ref = IDirect3DDevice9Ex_Release(device);
        ok(!ref, "Unexpected refcount %lu.\n", ref);

        DestroyWindow(device_window);
        DestroyWindow(focus_window);
    }
}

static void test_swapchain_parameters(void)
{
    IDirect3DDevice9Ex *device;
    IDirect3D9Ex *d3d9ex;
    RECT client_rect;
    HWND window;
    HRESULT hr;
    unsigned int i;
    D3DPRESENT_PARAMETERS present_parameters, present_parameters_windowed = {0}, present_parameters2;
    IDirect3DSwapChain9 *swapchain;
    D3DDISPLAYMODEEX mode = {0};
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
        {TRUE,  1,  0,                        D3DERR_INVALIDCALL},
        {FALSE, 1,  0,                        D3DERR_INVALIDCALL},

        /* All (non-ex) swap effects are allowed in
         * windowed and fullscreen mode. */
        {TRUE,  1,  D3DSWAPEFFECT_DISCARD,    D3D_OK},
        {TRUE,  1,  D3DSWAPEFFECT_FLIP,       D3D_OK},
        {FALSE, 1,  D3DSWAPEFFECT_DISCARD,    D3D_OK},
        {FALSE, 1,  D3DSWAPEFFECT_FLIP,       D3D_OK},
        {FALSE, 1,  D3DSWAPEFFECT_COPY,       D3D_OK},

        /* Only one backbuffer in copy mode. */
        {TRUE,  0,  D3DSWAPEFFECT_COPY,       D3D_OK},
        {TRUE,  1,  D3DSWAPEFFECT_COPY,       D3D_OK},
        {TRUE,  2,  D3DSWAPEFFECT_COPY,       D3DERR_INVALIDCALL},
        {FALSE, 2,  D3DSWAPEFFECT_COPY,       D3DERR_INVALIDCALL},

        /* Ok with the others, in fullscreen and windowed mode. */
        {TRUE,  2,  D3DSWAPEFFECT_DISCARD,    D3D_OK},
        {TRUE,  2,  D3DSWAPEFFECT_FLIP,       D3D_OK},
        {FALSE, 2,  D3DSWAPEFFECT_DISCARD,    D3D_OK},
        {FALSE, 2,  D3DSWAPEFFECT_FLIP,       D3D_OK},

        /* D3D9Ex swap effects. Flipex works, Overlay is complicated
         * and depends on HW features, pixel format, etc. */
        {TRUE,  1,  D3DSWAPEFFECT_FLIPEX,     D3D_OK},
        {TRUE,  1,  D3DSWAPEFFECT_FLIPEX + 1, D3DERR_INVALIDCALL},
        {FALSE, 1,  D3DSWAPEFFECT_FLIPEX,     D3D_OK},
        {FALSE, 1,  D3DSWAPEFFECT_FLIPEX + 1, D3DERR_INVALIDCALL},

        /* 30 is the highest allowed backbuffer count. */
        {TRUE,  30, D3DSWAPEFFECT_DISCARD,    D3D_OK},
        {TRUE,  31, D3DSWAPEFFECT_DISCARD,    D3DERR_INVALIDCALL},
        {TRUE,  30, D3DSWAPEFFECT_FLIP,       D3D_OK},
        {TRUE,  31, D3DSWAPEFFECT_FLIP,       D3DERR_INVALIDCALL},
        {FALSE, 30, D3DSWAPEFFECT_DISCARD,    D3D_OK},
        {FALSE, 31, D3DSWAPEFFECT_DISCARD,    D3DERR_INVALIDCALL},
        {FALSE, 30, D3DSWAPEFFECT_FLIP,       D3D_OK},
        {FALSE, 31, D3DSWAPEFFECT_FLIP,       D3DERR_INVALIDCALL},
    };

    window = CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW,
            0, 0, 640, 480, NULL, NULL, NULL, NULL);
    hr = pDirect3DCreate9Ex(D3D_SDK_VERSION, &d3d9ex);
    if (FAILED(hr))
    {
        skip("Failed to create IDirect3D9Ex, hr %#lx.\n", hr);
        return;
    }

    if (!(device = create_device(window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        IDirect3D9Ex_Release(d3d9ex);
        DestroyWindow(window);
        return;
    }
    IDirect3DDevice9Ex_Release(device);

    present_parameters_windowed.BackBufferWidth = registry_mode.dmPelsWidth;
    present_parameters_windowed.BackBufferHeight = registry_mode.dmPelsHeight;
    present_parameters_windowed.hDeviceWindow = window;
    present_parameters_windowed.BackBufferFormat = D3DFMT_X8R8G8B8;
    present_parameters_windowed.SwapEffect = D3DSWAPEFFECT_COPY;
    present_parameters_windowed.Windowed = TRUE;
    present_parameters_windowed.BackBufferCount = 1;

    mode.Size = sizeof(mode);
    mode.Width = registry_mode.dmPelsWidth;
    mode.Height = registry_mode.dmPelsHeight;
    mode.RefreshRate = 0;
    mode.Format = D3DFMT_X8R8G8B8;
    mode.ScanLineOrdering = 0;

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

        hr = IDirect3D9Ex_CreateDeviceEx(d3d9ex, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
                D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters,
                tests[i].windowed ? NULL : &mode, &device);
        ok(hr == tests[i].hr, "Expected hr %#lx, got %#lx, test %u.\n", tests[i].hr, hr, i);
        if (SUCCEEDED(hr))
        {
            UINT bb_count = tests[i].backbuffer_count ? tests[i].backbuffer_count : 1;

            hr = IDirect3DDevice9Ex_GetSwapChain(device, 0, &swapchain);
            ok(SUCCEEDED(hr), "Failed to get swapchain, hr %#lx, test %u.\n", hr, i);

            hr = IDirect3DSwapChain9_GetPresentParameters(swapchain, &present_parameters2);
            ok(SUCCEEDED(hr), "Failed to get present parameters, hr %#lx, test %u.\n", hr, i);
            ok(present_parameters2.SwapEffect == tests[i].swap_effect, "Swap effect changed from %u to %u, test %u.\n",
                    tests[i].swap_effect, present_parameters2.SwapEffect, i);
            ok(present_parameters2.BackBufferCount == bb_count, "Backbuffer count changed from %u to %u, test %u.\n",
                    bb_count, present_parameters2.BackBufferCount, i);
            ok(present_parameters2.Windowed == tests[i].windowed, "Windowed changed from %u to %u, test %u.\n",
                    tests[i].windowed, present_parameters2.Windowed, i);

            IDirect3DSwapChain9_Release(swapchain);
            IDirect3DDevice9Ex_Release(device);
        }

        hr = IDirect3D9Ex_CreateDeviceEx(d3d9ex, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
                D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present_parameters_windowed, NULL, &device);
        ok(SUCCEEDED(hr), "Failed to create device, hr %#lx, test %u.\n", hr, i);

        memset(&present_parameters, 0, sizeof(present_parameters));
        present_parameters.BackBufferWidth = registry_mode.dmPelsWidth;
        present_parameters.BackBufferHeight = registry_mode.dmPelsHeight;
        present_parameters.hDeviceWindow = window;
        present_parameters.BackBufferFormat = D3DFMT_X8R8G8B8;

        present_parameters.SwapEffect = tests[i].swap_effect;
        present_parameters.Windowed = tests[i].windowed;
        present_parameters.BackBufferCount = tests[i].backbuffer_count;

        hr = IDirect3DDevice9Ex_ResetEx(device, &present_parameters, tests[i].windowed ? NULL : &mode);
        ok(hr == tests[i].hr, "Expected hr %#lx, got %#lx, test %u.\n", tests[i].hr, hr, i);

        if (FAILED(hr))
        {
            hr = IDirect3DDevice9Ex_ResetEx(device, &present_parameters_windowed, NULL);
            ok(SUCCEEDED(hr), "Failed to reset device, hr %#lx, test %u.\n", hr, i);
        }
        IDirect3DDevice9Ex_Release(device);
    }

    memset(&present_parameters, 0, sizeof(present_parameters));
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.Windowed = TRUE;
    present_parameters.BackBufferWidth = 0;
    present_parameters.BackBufferHeight = 0;
    present_parameters.BackBufferFormat = D3DFMT_UNKNOWN;

    GetClientRect(window, &client_rect);

    hr = IDirect3D9Ex_CreateDeviceEx(d3d9ex, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            window, D3DCREATE_SOFTWARE_VERTEXPROCESSING,
            &present_parameters, NULL, &device);

    ok(present_parameters.BackBufferWidth == client_rect.right, "Got unexpected BackBufferWidth %u, expected %ld.\n",
            present_parameters.BackBufferWidth, client_rect.right);
    ok(present_parameters.BackBufferHeight == client_rect.bottom, "Got unexpected BackBufferHeight %u, expected %ld.\n",
            present_parameters.BackBufferHeight, client_rect.bottom);
    ok(present_parameters.BackBufferFormat != D3DFMT_UNKNOWN, "Got unexpected BackBufferFormat %#x.\n",
            present_parameters.BackBufferFormat);
    ok(present_parameters.BackBufferCount == 1, "Got unexpected BackBufferCount %u.\n", present_parameters.BackBufferCount);
    ok(!present_parameters.MultiSampleType, "Got unexpected MultiSampleType %u.\n", present_parameters.MultiSampleType);
    ok(!present_parameters.MultiSampleQuality, "Got unexpected MultiSampleQuality %lu.\n", present_parameters.MultiSampleQuality);
    ok(present_parameters.SwapEffect == D3DSWAPEFFECT_DISCARD, "Got unexpected SwapEffect %#x.\n", present_parameters.SwapEffect);
    ok(!present_parameters.hDeviceWindow, "Got unexpected hDeviceWindow %p.\n", present_parameters.hDeviceWindow);
    ok(present_parameters.Windowed, "Got unexpected Windowed %#x.\n", present_parameters.Windowed);
    ok(!present_parameters.EnableAutoDepthStencil, "Got unexpected EnableAutoDepthStencil %#x.\n", present_parameters.EnableAutoDepthStencil);
    ok(!present_parameters.AutoDepthStencilFormat, "Got unexpected AutoDepthStencilFormat %#x.\n", present_parameters.AutoDepthStencilFormat);
    ok(!present_parameters.Flags, "Got unexpected Flags %#lx.\n", present_parameters.Flags);
    ok(!present_parameters.FullScreen_RefreshRateInHz, "Got unexpected FullScreen_RefreshRateInHz %u.\n",
            present_parameters.FullScreen_RefreshRateInHz);
    ok(!present_parameters.PresentationInterval, "Got unexpected PresentationInterval %#x.\n", present_parameters.PresentationInterval);
    IDirect3DDevice9Ex_Release(device);

    IDirect3D9Ex_Release(d3d9ex);
    DestroyWindow(window);
}

static void test_backbuffer_resize(void)
{
    D3DPRESENT_PARAMETERS present_parameters = {0};
    IDirect3DSwapChain9 *swapchain, *old_swapchain;
    IDirect3DSurface9 *backbuffer, *old_backbuffer;
    IDirect3DDevice9Ex *device, *device2;
    IDirect3DBaseTexture9 *texture;
    D3DSURFACE_DESC surface_desc;
    unsigned int color;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    static const struct
    {
        float position[3];
        DWORD diffuse;
    }
    quad[] =
    {
        {{-1.0f, -1.0f, 0.1f}, D3DCOLOR_ARGB(0xff, 0x00, 0xff, 0x00)},
        {{-1.0f,  1.0f, 0.1f}, D3DCOLOR_ARGB(0xff, 0x00, 0xff, 0x00)},
        {{ 1.0f, -1.0f, 0.1f}, D3DCOLOR_ARGB(0xff, 0x00, 0xff, 0x00)},
        {{ 1.0f,  1.0f, 0.1f}, D3DCOLOR_ARGB(0xff, 0x00, 0xff, 0x00)},
    };

    window = create_window();
    if (!(device = create_device(window, NULL)))
    {
        skip("Failed to create a D3D device.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_CLIPPING, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable clipping, hr %#lx.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_ZENABLE, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable Z test, hr %#lx.\n", hr);
    hr = IDirect3DDevice9_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(SUCCEEDED(hr), "Failed to disable lighting, hr %#lx.\n", hr);
    hr = IDirect3DDevice9_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(SUCCEEDED(hr), "Failed to set FVF, hr %#lx.\n", hr);

    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(SUCCEEDED(hr), "Failed to get backbuffer, hr %#lx.\n", hr);
    hr = IDirect3DDevice9_SetRenderTarget(device, 0, backbuffer);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#lx.\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff0000, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#lx.\n", hr);
    color = get_pixel_color(device, 1, 1);
    ok(color == 0x00ff0000, "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DDevice9_GetSwapChain(device, 0, &old_swapchain);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IDirect3DSurface9_GetContainer(backbuffer, &IID_IDirect3DSwapChain9, (void **)&swapchain);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(swapchain == old_swapchain, "Swapchains didn't match.\n");
    IDirect3DSwapChain9_Release(swapchain);
    hr = IDirect3DSurface9_GetContainer(backbuffer, &IID_IDirect3DDevice9, (void **)&device2);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);

    present_parameters.BackBufferWidth = 800;
    present_parameters.BackBufferHeight = 600;
    present_parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.hDeviceWindow = NULL;
    present_parameters.Windowed = TRUE;
    present_parameters.EnableAutoDepthStencil = TRUE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;
    hr = IDirect3DDevice9_Reset(device, &present_parameters);
    ok(SUCCEEDED(hr), "Failed to reset, hr %#lx.\n", hr);

    old_backbuffer = backbuffer;
    hr = IDirect3DSurface9_GetDesc(old_backbuffer, &surface_desc);
    ok(SUCCEEDED(hr), "Failed to get surface desc, hr %#lx.\n", hr);
    ok(surface_desc.Width == 640, "Got unexpected width %u.\n", surface_desc.Width);
    ok(surface_desc.Height == 480, "Got unexpected height %u.\n", surface_desc.Height);

    hr = IDirect3DDevice9_GetSwapChain(device, 0, &swapchain);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(swapchain == old_swapchain, "Swapchains didn't match.\n");
    IDirect3DSwapChain9_Release(swapchain);
    IDirect3DSwapChain9_Release(old_swapchain);

    hr = IDirect3DSurface9_GetContainer(old_backbuffer, &IID_IDirect3DSwapChain9, (void **)&swapchain);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    hr = IDirect3DSurface9_GetContainer(old_backbuffer, &IID_IDirect3DBaseTexture9, (void **)&texture);
    ok(hr == E_NOINTERFACE, "Got hr %#lx.\n", hr);
    hr = IDirect3DSurface9_GetContainer(old_backbuffer, &IID_IDirect3DDevice9, (void **)&device2);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    ok(device2 == device, "Devices didn't match.\n");
    IDirect3DDevice9_Release(device2);

    refcount = IDirect3DSurface9_Release(old_backbuffer);
    ok(!refcount, "Surface has %lu references left.\n", refcount);

    hr = IDirect3DDevice9_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(SUCCEEDED(hr), "Failed to get backbuffer, hr %#lx.\n", hr);
    ok(backbuffer != old_backbuffer, "Expected new backbuffer surface.\n");

    hr = IDirect3DDevice9_SetRenderTarget(device, 0, backbuffer);
    ok(SUCCEEDED(hr), "Failed to set render target, hr %#lx.\n", hr);

    hr = IDirect3DDevice9_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffffff00, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#lx.\n", hr);
    color = get_pixel_color(device, 1, 1);
    ok(color == 0x00ffff00, "Got unexpected color 0x%08x.\n", color);
    color = get_pixel_color(device, 700, 500);
    ok(color == 0x00ffff00, "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DDevice9_BeginScene(device);
    ok(SUCCEEDED(hr), "Failed to begin scene, hr %#lx.\n", hr);
    hr = IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 2, quad, sizeof(*quad));
    ok(SUCCEEDED(hr), "Failed to draw, hr %#lx.\n", hr);
    hr = IDirect3DDevice9_EndScene(device);
    ok(SUCCEEDED(hr), "Failed to end scene, hr %#lx.\n", hr);
    color = get_pixel_color(device, 1, 1);
    ok(color == 0x0000ff00, "Got unexpected color 0x%08x.\n", color);
    color = get_pixel_color(device, 700, 500);
    ok(color == 0x0000ff00, "Got unexpected color 0x%08x.\n", color);

    IDirect3DSurface9_Release(backbuffer);
    refcount = IDirect3DDevice9_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
    DestroyWindow(window);
}

static void test_format_unknown(void)
{
    IDirect3DDevice9Ex *device;
    ULONG refcount;
    HWND window;
    void *iface;
    HRESULT hr;

    window = create_window();
    if (!(device = create_device(window, NULL)))
    {
        skip("Failed to create a D3D device.\n");
        DestroyWindow(window);
        return;
    }

    iface = (void *)0xdeadbeef;
    hr = IDirect3DDevice9Ex_CreateRenderTarget(device, 64, 64,
            D3DFMT_UNKNOWN, D3DMULTISAMPLE_NONE, 0, FALSE, (IDirect3DSurface9 **)&iface, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    ok(!iface, "Got unexpected iface %p.\n", iface);

    iface = (void *)0xdeadbeef;
    hr = IDirect3DDevice9Ex_CreateRenderTargetEx(device, 64, 64,
            D3DFMT_UNKNOWN, D3DMULTISAMPLE_NONE, 0, FALSE, (IDirect3DSurface9 **)&iface, NULL, 0);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    ok(!iface, "Got unexpected iface %p.\n", iface);

    iface = (void *)0xdeadbeef;
    hr = IDirect3DDevice9Ex_CreateDepthStencilSurface(device, 64, 64,
            D3DFMT_UNKNOWN, D3DMULTISAMPLE_NONE, 0, TRUE, (IDirect3DSurface9 **)&iface, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    ok(!iface, "Got unexpected iface %p.\n", iface);

    iface = (void *)0xdeadbeef;
    hr = IDirect3DDevice9Ex_CreateDepthStencilSurfaceEx(device, 64, 64,
            D3DFMT_UNKNOWN, D3DMULTISAMPLE_NONE, 0, TRUE, (IDirect3DSurface9 **)&iface, NULL, 0);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    ok(!iface, "Got unexpected iface %p.\n", iface);

    iface = (void *)0xdeadbeef;
    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurface(device, 64, 64,
            D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, (IDirect3DSurface9 **)&iface, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    ok(!iface, "Got unexpected iface %p.\n", iface);

    iface = (void *)0xdeadbeef;
    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurfaceEx(device, 64, 64,
            D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, (IDirect3DSurface9 **)&iface, NULL, 9);
    todo_wine ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    todo_wine ok(!iface, "Got unexpected iface %p.\n", iface);

    iface = (void *)0xdeadbeef;
    hr = IDirect3DDevice9Ex_CreateTexture(device, 64, 64, 1, 0,
            D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, (IDirect3DTexture9 **)&iface, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    ok(!iface, "Got unexpected iface %p.\n", iface);

    iface = (void *)0xdeadbeef;
    hr = IDirect3DDevice9Ex_CreateCubeTexture(device, 64, 1, 0,
            D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, (IDirect3DCubeTexture9 **)&iface, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    ok(!iface, "Got unexpected iface %p.\n", iface);

    iface = (void *)0xdeadbeef;
    hr = IDirect3DDevice9Ex_CreateVolumeTexture(device, 64, 64, 1, 1, 0,
            D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, (IDirect3DVolumeTexture9 **)&iface, NULL);
    ok(hr == D3DERR_INVALIDCALL, "Got unexpected hr %#lx.\n", hr);
    ok(!iface, "Got unexpected iface %p.\n", iface);

    refcount = IDirect3DDevice9Ex_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
    DestroyWindow(window);
}

static void test_device_caps(void)
{
    IDirect3DDevice9Ex *device;
    ULONG refcount;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;

    window = create_window();
    if (!(device = create_device(window, NULL)))
    {
        skip("Failed to create a D3D device.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9Ex_GetDeviceCaps(device, &caps);
    ok(SUCCEEDED(hr), "Failed to get caps, hr %#lx.\n", hr);

    ok(!(caps.Caps & ~(D3DCAPS_OVERLAY | D3DCAPS_READ_SCANLINE)),
            "Caps field has unexpected flags %#lx.\n", caps.Caps);
    ok(!(caps.Caps2 & ~(D3DCAPS2_FULLSCREENGAMMA | D3DCAPS2_CANCALIBRATEGAMMA | D3DCAPS2_RESERVED
            | D3DCAPS2_CANMANAGERESOURCE | D3DCAPS2_DYNAMICTEXTURES | D3DCAPS2_CANAUTOGENMIPMAP
            | D3DCAPS2_CANSHARERESOURCE)),
            "Caps2 field has unexpected flags %#lx.\n", caps.Caps2);
    /* AMD doesn't filter all the ddraw / d3d9 caps. Consider that behavior
     * broken. */
    ok(!(caps.Caps3 & ~(D3DCAPS3_ALPHA_FULLSCREEN_FLIP_OR_DISCARD
            | D3DCAPS3_LINEAR_TO_SRGB_PRESENTATION | D3DCAPS3_COPY_TO_VIDMEM
            | D3DCAPS3_COPY_TO_SYSTEMMEM | D3DCAPS3_DXVAHD | D3DCAPS3_DXVAHD_LIMITED
            | D3DCAPS3_RESERVED)),
            "Caps3 field has unexpected flags %#lx.\n", caps.Caps3);
    ok(!(caps.PrimitiveMiscCaps & ~(D3DPMISCCAPS_MASKZ | D3DPMISCCAPS_LINEPATTERNREP
            | D3DPMISCCAPS_CULLNONE | D3DPMISCCAPS_CULLCW | D3DPMISCCAPS_CULLCCW
            | D3DPMISCCAPS_COLORWRITEENABLE | D3DPMISCCAPS_CLIPPLANESCALEDPOINTS
            | D3DPMISCCAPS_CLIPTLVERTS | D3DPMISCCAPS_TSSARGTEMP | D3DPMISCCAPS_BLENDOP
            | D3DPMISCCAPS_NULLREFERENCE | D3DPMISCCAPS_INDEPENDENTWRITEMASKS
            | D3DPMISCCAPS_PERSTAGECONSTANT | D3DPMISCCAPS_FOGANDSPECULARALPHA
            | D3DPMISCCAPS_SEPARATEALPHABLEND | D3DPMISCCAPS_MRTINDEPENDENTBITDEPTHS
            | D3DPMISCCAPS_MRTPOSTPIXELSHADERBLENDING | D3DPMISCCAPS_FOGVERTEXCLAMPED
            | D3DPMISCCAPS_POSTBLENDSRGBCONVERT)),
            "PrimitiveMiscCaps field has unexpected flags %#lx.\n", caps.PrimitiveMiscCaps);
    ok(!(caps.RasterCaps & ~(D3DPRASTERCAPS_DITHER | D3DPRASTERCAPS_ZTEST
            | D3DPRASTERCAPS_FOGVERTEX | D3DPRASTERCAPS_FOGTABLE
            | D3DPRASTERCAPS_MIPMAPLODBIAS | D3DPRASTERCAPS_ZBUFFERLESSHSR
            | D3DPRASTERCAPS_FOGRANGE | D3DPRASTERCAPS_ANISOTROPY | D3DPRASTERCAPS_WBUFFER
            | D3DPRASTERCAPS_WFOG | D3DPRASTERCAPS_ZFOG | D3DPRASTERCAPS_COLORPERSPECTIVE
            | D3DPRASTERCAPS_SCISSORTEST | D3DPRASTERCAPS_SLOPESCALEDEPTHBIAS
            | D3DPRASTERCAPS_DEPTHBIAS | D3DPRASTERCAPS_MULTISAMPLE_TOGGLE))
            || broken(!(caps.RasterCaps & ~0x0f736191)),
            "RasterCaps field has unexpected flags %#lx.\n", caps.RasterCaps);
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
            "SrcBlendCaps field has unexpected flags %#lx.\n", caps.SrcBlendCaps);
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
            "DestBlendCaps field has unexpected flags %#lx.\n", caps.DestBlendCaps);
    ok(!(caps.TextureCaps & ~(D3DPTEXTURECAPS_PERSPECTIVE | D3DPTEXTURECAPS_POW2
            | D3DPTEXTURECAPS_ALPHA | D3DPTEXTURECAPS_SQUAREONLY
            | D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE | D3DPTEXTURECAPS_ALPHAPALETTE
            | D3DPTEXTURECAPS_NONPOW2CONDITIONAL | D3DPTEXTURECAPS_PROJECTED
            | D3DPTEXTURECAPS_CUBEMAP | D3DPTEXTURECAPS_VOLUMEMAP | D3DPTEXTURECAPS_MIPMAP
            | D3DPTEXTURECAPS_MIPVOLUMEMAP | D3DPTEXTURECAPS_MIPCUBEMAP
            | D3DPTEXTURECAPS_CUBEMAP_POW2 | D3DPTEXTURECAPS_VOLUMEMAP_POW2
            | D3DPTEXTURECAPS_NOPROJECTEDBUMPENV)),
            "TextureCaps field has unexpected flags %#lx.\n", caps.TextureCaps);
    ok(!(caps.TextureFilterCaps & ~(D3DPTFILTERCAPS_MINFPOINT | D3DPTFILTERCAPS_MINFLINEAR
            | D3DPTFILTERCAPS_MINFANISOTROPIC | D3DPTFILTERCAPS_MINFPYRAMIDALQUAD
            | D3DPTFILTERCAPS_MINFGAUSSIANQUAD | D3DPTFILTERCAPS_MIPFPOINT
            | D3DPTFILTERCAPS_MIPFLINEAR | D3DPTFILTERCAPS_CONVOLUTIONMONO
            | D3DPTFILTERCAPS_MAGFPOINT | D3DPTFILTERCAPS_MAGFLINEAR
            | D3DPTFILTERCAPS_MAGFANISOTROPIC | D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD
            | D3DPTFILTERCAPS_MAGFGAUSSIANQUAD)),
            "TextureFilterCaps field has unexpected flags %#lx.\n", caps.TextureFilterCaps);
    ok(!(caps.CubeTextureFilterCaps & ~(D3DPTFILTERCAPS_MINFPOINT | D3DPTFILTERCAPS_MINFLINEAR
            | D3DPTFILTERCAPS_MINFANISOTROPIC | D3DPTFILTERCAPS_MINFPYRAMIDALQUAD
            | D3DPTFILTERCAPS_MINFGAUSSIANQUAD | D3DPTFILTERCAPS_MIPFPOINT
            | D3DPTFILTERCAPS_MIPFLINEAR | D3DPTFILTERCAPS_MAGFPOINT | D3DPTFILTERCAPS_MAGFLINEAR
            | D3DPTFILTERCAPS_MAGFANISOTROPIC | D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD
            | D3DPTFILTERCAPS_MAGFGAUSSIANQUAD)),
            "CubeTextureFilterCaps field has unexpected flags %#lx.\n", caps.CubeTextureFilterCaps);
    ok(!(caps.VolumeTextureFilterCaps & ~(D3DPTFILTERCAPS_MINFPOINT | D3DPTFILTERCAPS_MINFLINEAR
            | D3DPTFILTERCAPS_MINFANISOTROPIC | D3DPTFILTERCAPS_MINFPYRAMIDALQUAD
            | D3DPTFILTERCAPS_MINFGAUSSIANQUAD | D3DPTFILTERCAPS_MIPFPOINT
            | D3DPTFILTERCAPS_MIPFLINEAR | D3DPTFILTERCAPS_MAGFPOINT | D3DPTFILTERCAPS_MAGFLINEAR
            | D3DPTFILTERCAPS_MAGFANISOTROPIC | D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD
            | D3DPTFILTERCAPS_MAGFGAUSSIANQUAD)),
            "VolumeTextureFilterCaps field has unexpected flags %#lx.\n", caps.VolumeTextureFilterCaps);
    ok(!(caps.LineCaps & ~(D3DLINECAPS_TEXTURE | D3DLINECAPS_ZTEST | D3DLINECAPS_BLEND
            | D3DLINECAPS_ALPHACMP | D3DLINECAPS_FOG | D3DLINECAPS_ANTIALIAS)),
            "LineCaps field has unexpected flags %#lx.\n", caps.LineCaps);
    ok(!(caps.StencilCaps & ~(D3DSTENCILCAPS_KEEP | D3DSTENCILCAPS_ZERO | D3DSTENCILCAPS_REPLACE
            | D3DSTENCILCAPS_INCRSAT | D3DSTENCILCAPS_DECRSAT | D3DSTENCILCAPS_INVERT
            | D3DSTENCILCAPS_INCR | D3DSTENCILCAPS_DECR | D3DSTENCILCAPS_TWOSIDED)),
            "StencilCaps field has unexpected flags %#lx.\n", caps.StencilCaps);
    ok(!(caps.VertexProcessingCaps & ~(D3DVTXPCAPS_TEXGEN | D3DVTXPCAPS_MATERIALSOURCE7
            | D3DVTXPCAPS_DIRECTIONALLIGHTS | D3DVTXPCAPS_POSITIONALLIGHTS | D3DVTXPCAPS_LOCALVIEWER
            | D3DVTXPCAPS_TWEENING | D3DVTXPCAPS_TEXGEN_SPHEREMAP
            | D3DVTXPCAPS_NO_TEXGEN_NONLOCALVIEWER)),
            "VertexProcessingCaps field has unexpected flags %#lx.\n", caps.VertexProcessingCaps);
    /* Both Nvidia and AMD give 10 here. */
    ok(caps.MaxActiveLights <= 10,
            "MaxActiveLights field has unexpected value %lu.\n", caps.MaxActiveLights);
    /* AMD gives 6, Nvidia returns 8. */
    ok(caps.MaxUserClipPlanes <= 8,
            "MaxUserClipPlanes field has unexpected value %lu.\n", caps.MaxUserClipPlanes);

    refcount = IDirect3DDevice9Ex_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
    DestroyWindow(window);
}

static void test_frame_latency(void)
{
    IDirect3DDevice9Ex *device;
    ULONG refcount;
    UINT latency;
    HWND window;
    HRESULT hr;

    window = create_window();
    if (!(device = create_device(window, NULL)))
    {
        skip("Failed to create a D3D device.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9Ex_GetMaximumFrameLatency(device, &latency);
    ok(SUCCEEDED(hr), "Failed to get max frame latency, hr %#lx.\n", hr);
    ok(latency == 3, "Unexpected default max frame latency %u.\n", latency);

    hr = IDirect3DDevice9Ex_SetMaximumFrameLatency(device, 1);
    ok(SUCCEEDED(hr), "Failed to set max frame latency, hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_GetMaximumFrameLatency(device, &latency);
    ok(SUCCEEDED(hr), "Failed to get max frame latency, hr %#lx.\n", hr);
    ok(latency == 1, "Unexpected max frame latency %u.\n", latency);

    hr = IDirect3DDevice9Ex_SetMaximumFrameLatency(device, 0);
    ok(SUCCEEDED(hr), "Failed to set max frame latency, hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_GetMaximumFrameLatency(device, &latency);
    ok(SUCCEEDED(hr), "Failed to get max frame latency, hr %#lx.\n", hr);
    ok(latency == 3 || !latency, "Unexpected default max frame latency %u.\n", latency);

    hr = IDirect3DDevice9Ex_SetMaximumFrameLatency(device, 30);
    ok(SUCCEEDED(hr), "Failed to set max frame latency, hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_SetMaximumFrameLatency(device, 31);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);

    refcount = IDirect3DDevice9Ex_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
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
    IDirect3DDevice9Ex *device;
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
        SURFACE_RT_EX,
        SURFACE_DS,
        SURFACE_DS_EX,
        SURFACE_PLAIN,
        SURFACE_PLAIN_EX,
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
        {D3DPOOL_MANAGED,   FORMAT_COLOUR, 0,                                        FALSE},
        {D3DPOOL_MANAGED,   FORMAT_ATI2,   0,                                        FALSE},
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
        {"2D",       SURFACE_2D},
        {"CUBE",     SURFACE_CUBE},
        {"RT",       SURFACE_RT},
        {"RT_EX",    SURFACE_RT_EX},
        {"DS",       SURFACE_DS},
        {"DS_EX",    SURFACE_DS_EX},
        {"PLAIN",    SURFACE_PLAIN},
        {"PLAIN_EX", SURFACE_PLAIN_EX},
    };

    window = create_window();
    device_desc.device_window = window;
    device_desc.width = 16;
    device_desc.height = 16;
    device_desc.flags = 0;
    if (!(device = create_device(window, &device_desc)))
    {
        skip("Failed to create a D3D device.\n");
        DestroyWindow(window);
        return;
    }
    hr = IDirect3DDevice9Ex_GetDirect3D(device, &d3d);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3D9_GetAdapterIdentifier(d3d, D3DADAPTER_DEFAULT, 0, &identifier);
    ok(SUCCEEDED(hr), "Failed to get adapter identifier, hr %#lx.\n", hr);
    warp = adapter_is_warp(&identifier);

    hr = IDirect3DDevice9Ex_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DSurface9_GetDesc(backbuffer, &surface_desc);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    colour_format = surface_desc.Format;

    hr = IDirect3DDevice9Ex_GetDepthStencilSurface(device, &depth_stencil);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DSurface9_GetDesc(depth_stencil, &surface_desc);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    depth_format = surface_desc.Format;

    depth_2d = SUCCEEDED(IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, 0, D3DRTYPE_TEXTURE, depth_format));
    depth_cube = SUCCEEDED(IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, 0, D3DRTYPE_CUBETEXTURE, depth_format));
    depth_plain = SUCCEEDED(IDirect3D9_CheckDeviceFormat(d3d, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
            D3DFMT_X8R8G8B8, 0, D3DRTYPE_SURFACE, depth_format));

    hr = IDirect3DDevice9Ex_SetFVF(device, D3DFVF_XYZRHW);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

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
                    hr = IDirect3DDevice9Ex_CreateTexture(device, 16, 16, 1,
                            tests[j].usage, format, tests[j].pool, &texture_2d, NULL);
                    todo_wine_if(!tests[j].valid && tests[j].format == FORMAT_DEPTH
                            && !tests[j].usage && tests[j].pool != D3DPOOL_MANAGED)
                        ok(hr == (tests[j].valid && (tests[j].format != FORMAT_DEPTH || depth_2d)
                                ? D3D_OK : D3DERR_INVALIDCALL),
                                "Test %s %u: Got unexpected hr %#lx.\n", surface_types[i].name, j, hr);
                    if (FAILED(hr))
                        continue;

                    hr = IDirect3DTexture9_GetSurfaceLevel(texture_2d, 0, &surface);
                    ok(hr == D3D_OK, "Test %s %u: Got unexpected hr %#lx.\n", surface_types[i].name, j, hr);
                    IDirect3DTexture9_Release(texture_2d);
                    break;

                case SURFACE_CUBE:
                    hr = IDirect3DDevice9Ex_CreateCubeTexture(device, 16, 1,
                            tests[j].usage, format, tests[j].pool, &texture_cube, NULL);
                    todo_wine_if(!tests[j].valid && tests[j].format == FORMAT_DEPTH
                            && !tests[j].usage && tests[j].pool != D3DPOOL_MANAGED)
                        ok(hr == (tests[j].valid && (tests[j].format != FORMAT_DEPTH || depth_cube)
                                ? D3D_OK : D3DERR_INVALIDCALL),
                                "Test %s %u: Got unexpected hr %#lx.\n", surface_types[i].name, j, hr);
                    if (FAILED(hr))
                        continue;

                    hr = IDirect3DCubeTexture9_GetCubeMapSurface(texture_cube,
                            D3DCUBEMAP_FACE_POSITIVE_X, 0, &surface);
                    ok(hr == D3D_OK, "Test %s %u: Got unexpected hr %#lx.\n", surface_types[i].name, j, hr);
                    IDirect3DCubeTexture9_Release(texture_cube);
                    break;

                case SURFACE_RT:
                    hr = IDirect3DDevice9Ex_CreateRenderTarget(device, 16, 16, format,
                            D3DMULTISAMPLE_NONE, 0, tests[j].usage & D3DUSAGE_DYNAMIC, &surface, NULL);
                    ok(hr == (tests[j].format == FORMAT_COLOUR ? D3D_OK : D3DERR_INVALIDCALL),
                            "Test %s %u: Got unexpected hr %#lx.\n", surface_types[i].name, j, hr);
                    if (FAILED(hr))
                        continue;
                    break;

                case SURFACE_RT_EX:
                    hr = IDirect3DDevice9Ex_CreateRenderTargetEx(device, 16, 16, format, D3DMULTISAMPLE_NONE,
                            0, tests[j].pool != D3DPOOL_DEFAULT, &surface, NULL, tests[j].usage);
                    ok(hr == (tests[j].format == FORMAT_COLOUR && !tests[j].usage ? D3D_OK : D3DERR_INVALIDCALL),
                                "Test %s %u: Got unexpected hr %#lx.\n", surface_types[i].name, j, hr);
                    if (FAILED(hr))
                        continue;
                    break;

                case SURFACE_DS:
                    hr = IDirect3DDevice9Ex_CreateDepthStencilSurface(device, 16, 16, format,
                            D3DMULTISAMPLE_NONE, 0, tests[j].usage & D3DUSAGE_DYNAMIC, &surface, NULL);
                    ok(hr == (tests[j].format == FORMAT_DEPTH ? D3D_OK : D3DERR_INVALIDCALL),
                            "Test %s %u: Got unexpected hr %#lx.\n", surface_types[i].name, j, hr);
                    if (FAILED(hr))
                        continue;
                    break;

                case SURFACE_DS_EX:
                    hr = IDirect3DDevice9Ex_CreateDepthStencilSurfaceEx(device, 16, 16, format, D3DMULTISAMPLE_NONE,
                            0, tests[j].pool != D3DPOOL_DEFAULT, &surface, NULL, tests[j].usage);
                    ok(hr == (tests[j].format == FORMAT_DEPTH && !tests[j].usage ? D3D_OK : D3DERR_INVALIDCALL),
                            "Test %s %u: Got unexpected hr %#lx.\n", surface_types[i].name, j, hr);
                    if (FAILED(hr))
                        continue;
                    break;

                case SURFACE_PLAIN:
                    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurface(device,
                            16, 16, format, tests[j].pool, &surface, NULL);
                    todo_wine_if(tests[j].format == FORMAT_ATI2 && tests[j].pool != D3DPOOL_MANAGED
                            && tests[j].pool != D3DPOOL_SCRATCH)
                        ok(hr == (tests[j].pool != D3DPOOL_MANAGED && (tests[j].format != FORMAT_DEPTH || depth_plain)
                                && (tests[j].format != FORMAT_ATI2 || tests[j].pool == D3DPOOL_SCRATCH)
                                ? D3D_OK : D3DERR_INVALIDCALL),
                                "Test %s %u: Got unexpected hr %#lx.\n", surface_types[i].name, j, hr);
                    if (FAILED(hr))
                        continue;
                    break;

                case SURFACE_PLAIN_EX:
                    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurfaceEx(device,
                            16, 16, format, tests[j].pool, &surface, NULL, tests[j].usage);
                    todo_wine
                        ok(hr == (!tests[j].usage && tests[j].pool != D3DPOOL_MANAGED
                                && (tests[j].format != FORMAT_DEPTH || depth_plain)
                                && (tests[j].format != FORMAT_ATI2 || tests[j].pool == D3DPOOL_SCRATCH)
                                ? D3D_OK : D3DERR_INVALIDCALL),
                                "Test %s %u: Got unexpected hr %#lx.\n", surface_types[i].name, j, hr);
                    if (FAILED(hr))
                        continue;
                    break;

                default:
                    ok(0, "Invalid surface type %#x.\n", surface_types[i].type);
                    continue;
            }

            hr = IDirect3DSurface9_GetDesc(surface, &surface_desc);
            ok(hr == D3D_OK, "Test %s %u: Got unexpected hr %#lx.\n", surface_types[i].name, j, hr);
            if (surface_types[i].type == SURFACE_RT || surface_types[i].type == SURFACE_RT_EX)
            {
                ok(surface_desc.Usage == D3DUSAGE_RENDERTARGET, "Test %s %u: Got unexpected usage %#lx.\n",
                        surface_types[i].name, j, surface_desc.Usage);
                ok(surface_desc.Pool == D3DPOOL_DEFAULT, "Test %s %u: Got unexpected pool %#x.\n",
                        surface_types[i].name, j, surface_desc.Pool);
            }
            else if (surface_types[i].type == SURFACE_DS || surface_types[i].type == SURFACE_DS_EX)
            {
                ok(surface_desc.Usage == D3DUSAGE_DEPTHSTENCIL, "Test %s %u: Got unexpected usage %#lx.\n",
                        surface_types[i].name, j, surface_desc.Usage);
                ok(surface_desc.Pool == D3DPOOL_DEFAULT, "Test %s %u: Got unexpected pool %#x.\n",
                        surface_types[i].name, j, surface_desc.Pool);
            }
            else if (surface_types[i].type == SURFACE_PLAIN || surface_types[i].type == SURFACE_PLAIN_EX)
            {
                ok(!surface_desc.Usage, "Test %s %u: Got unexpected usage %#lx.\n",
                        surface_types[i].name, j, surface_desc.Usage);
                ok(surface_desc.Pool == tests[j].pool, "Test %s %u: Got unexpected pool %#x.\n",
                        surface_types[i].name, j, surface_desc.Pool);
            }
            else
            {
                ok(surface_desc.Usage == tests[j].usage, "Test %s %u: Got unexpected usage %#lx.\n",
                        surface_types[i].name, j, surface_desc.Usage);
                ok(surface_desc.Pool == tests[j].pool, "Test %s %u: Got unexpected pool %#x.\n",
                        surface_types[i].name, j, surface_desc.Pool);
            }

            hr = IDirect3DSurface9_LockRect(surface, &lr, NULL, 0);
            if (surface_desc.Pool != D3DPOOL_DEFAULT || surface_desc.Usage & D3DUSAGE_DYNAMIC
                    || (surface_types[i].type == SURFACE_RT && tests[j].usage & D3DUSAGE_DYNAMIC)
                    || (surface_types[i].type == SURFACE_RT_EX && tests[j].pool != D3DPOOL_DEFAULT)
                    || surface_types[i].type == SURFACE_PLAIN || surface_types[i].type == SURFACE_PLAIN_EX
                    || tests[j].format == FORMAT_ATI2)
                expected_hr = D3D_OK;
            else
                expected_hr = D3DERR_INVALIDCALL;
            ok(hr == expected_hr, "Test %s %u: Got unexpected hr %#lx.\n", surface_types[i].name, j, hr);
            hr = IDirect3DSurface9_UnlockRect(surface);
            todo_wine_if(expected_hr != D3D_OK && surface_types[i].type == SURFACE_2D)
                ok(hr == expected_hr, "Test %s %u: Got unexpected hr %#lx.\n", surface_types[i].name, j, hr);

            if (SUCCEEDED(IDirect3DSurface9_GetContainer(surface, &IID_IDirect3DBaseTexture9, (void **)&texture)))
            {
                hr = IDirect3DDevice9Ex_SetTexture(device, 0, texture);
                ok(hr == D3D_OK, "Test %s %u: Got unexpected hr %#lx.\n", surface_types[i].name, j, hr);
                hr = IDirect3DDevice9Ex_SetTexture(device, 0, NULL);
                ok(hr == D3D_OK, "Test %s %u: Got unexpected hr %#lx.\n", surface_types[i].name, j, hr);
                IDirect3DBaseTexture9_Release(texture);
            }

            hr = IDirect3DDevice9Ex_SetRenderTarget(device, 0, surface);
            ok(hr == (surface_desc.Usage & D3DUSAGE_RENDERTARGET ? D3D_OK : D3DERR_INVALIDCALL),
                    "Test %s %u: Got unexpected hr %#lx.\n", surface_types[i].name, j, hr);
            hr = IDirect3DDevice9Ex_SetRenderTarget(device, 0, backbuffer);
            ok(hr == D3D_OK, "Test %s %u: Got unexpected hr %#lx.\n", surface_types[i].name, j, hr);

            hr = IDirect3DDevice9Ex_SetDepthStencilSurface(device, surface);
            ok(hr == (surface_desc.Usage & D3DUSAGE_DEPTHSTENCIL ? D3D_OK : D3DERR_INVALIDCALL),
                    "Test %s %u: Got unexpected hr %#lx.\n", surface_types[i].name, j, hr);
            hr = IDirect3DDevice9Ex_SetDepthStencilSurface(device, depth_stencil);
            ok(hr == D3D_OK, "Test %s %u: Got unexpected hr %#lx.\n", surface_types[i].name, j, hr);

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

        hr = IDirect3DDevice9Ex_CreateVolumeTexture(device, 16, 16, 1, 1,
                tests[i].usage, format, tests[i].pool, &texture, NULL);
        ok((hr == (!(tests[i].usage & ~D3DUSAGE_DYNAMIC)
                && (tests[i].format != FORMAT_ATI2 || tests[i].pool == D3DPOOL_SCRATCH)
                && tests[i].pool != D3DPOOL_MANAGED ? D3D_OK : D3DERR_INVALIDCALL))
                || (tests[i].format == FORMAT_ATI2 && (hr == D3D_OK || warp)),
                "Test %u: Got unexpected hr %#lx.\n", i, hr);
        if (FAILED(hr))
            continue;

        hr = IDirect3DVolumeTexture9_GetVolumeLevel(texture, 0, &volume);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);

        hr = IDirect3DVolume9_GetDesc(volume, &volume_desc);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        ok(volume_desc.Usage == tests[i].usage, "Test %u: Got unexpected usage %#lx.\n", i, volume_desc.Usage);
        ok(volume_desc.Pool == tests[i].pool, "Test %u: Got unexpected pool %#x.\n", i, volume_desc.Pool);

        hr = IDirect3DVolume9_LockBox(volume, &lb, NULL, 0);
        if (volume_desc.Pool != D3DPOOL_DEFAULT || volume_desc.Usage & D3DUSAGE_DYNAMIC)
            expected_hr = D3D_OK;
        else
            expected_hr = D3DERR_INVALIDCALL;
        ok(hr == expected_hr || (volume_desc.Pool == D3DPOOL_DEFAULT && hr == D3D_OK),
                "Test %u: Got unexpected hr %#lx.\n", i, hr);
        hr = IDirect3DVolume9_UnlockBox(volume);
        ok(hr == expected_hr || (volume_desc.Pool == D3DPOOL_DEFAULT && hr == D3D_OK),
                "Test %u: Got unexpected hr %#lx.\n", i, hr);

        hr = IDirect3DDevice9Ex_SetTexture(device, 0, (IDirect3DBaseTexture9 *)texture);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        hr = IDirect3DDevice9Ex_SetTexture(device, 0, NULL);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);

        IDirect3DVolume9_Release(volume);
        IDirect3DVolumeTexture9_Release(texture);
    }

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        D3DINDEXBUFFER_DESC ib_desc;
        IDirect3DIndexBuffer9 *ib;
        void *data;

        hr = IDirect3DDevice9Ex_CreateIndexBuffer(device, 16, tests[i].usage,
                tests[i].format == FORMAT_COLOUR ? D3DFMT_INDEX32 : D3DFMT_INDEX16, tests[i].pool, &ib, NULL);
        ok(hr == (tests[i].pool == D3DPOOL_SCRATCH || tests[i].pool == D3DPOOL_MANAGED
                || (tests[i].usage & ~D3DUSAGE_DYNAMIC) ? D3DERR_INVALIDCALL : D3D_OK),
                "Test %u: Got unexpected hr %#lx.\n", i, hr);
        if (FAILED(hr))
            continue;

        hr = IDirect3DIndexBuffer9_GetDesc(ib, &ib_desc);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        ok(ib_desc.Usage == tests[i].usage, "Test %u: Got unexpected usage %#lx.\n", i, ib_desc.Usage);
        ok(ib_desc.Pool == tests[i].pool, "Test %u: Got unexpected pool %#x.\n", i, ib_desc.Pool);

        hr = IDirect3DIndexBuffer9_Lock(ib, 0, 0, &data, 0);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        hr = IDirect3DIndexBuffer9_Unlock(ib);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);

        hr = IDirect3DDevice9Ex_SetIndices(device, ib);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        hr = IDirect3DDevice9Ex_SetIndices(device, NULL);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);

        IDirect3DIndexBuffer9_Release(ib);
    }

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        D3DVERTEXBUFFER_DESC vb_desc;
        IDirect3DVertexBuffer9 *vb;
        void *data;

        hr = IDirect3DDevice9Ex_CreateVertexBuffer(device, 16, tests[i].usage,
                tests[i].format == FORMAT_COLOUR ? 0 : D3DFVF_XYZRHW, tests[i].pool, &vb, NULL);
        ok(hr == (tests[i].pool == D3DPOOL_SCRATCH || tests[i].pool == D3DPOOL_MANAGED
                || (tests[i].usage & ~D3DUSAGE_DYNAMIC) ? D3DERR_INVALIDCALL : D3D_OK),
                "Test %u: Got unexpected hr %#lx.\n", i, hr);
        if (FAILED(hr))
            continue;

        hr = IDirect3DVertexBuffer9_GetDesc(vb, &vb_desc);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        ok(vb_desc.Usage == tests[i].usage, "Test %u: Got unexpected usage %#lx.\n", i, vb_desc.Usage);
        ok(vb_desc.Pool == tests[i].pool, "Test %u: Got unexpected pool %#x.\n", i, vb_desc.Pool);

        hr = IDirect3DVertexBuffer9_Lock(vb, 0, 0, &data, 0);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        hr = IDirect3DVertexBuffer9_Unlock(vb);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);

        hr = IDirect3DDevice9Ex_SetStreamSource(device, 0, vb, 0, 16);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);
        hr = IDirect3DDevice9Ex_SetStreamSource(device, 0, NULL, 0, 0);
        ok(hr == D3D_OK, "Test %u: Got unexpected hr %#lx.\n", i, hr);

        IDirect3DVertexBuffer9_Release(vb);
    }

    IDirect3DSurface9_Release(depth_stencil);
    IDirect3DSurface9_Release(backbuffer);
    refcount = IDirect3DDevice9Ex_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_sysmem_draw(void)
{
    static const DWORD texture_data[4] = {0xffff0000, 0xff00ff00, 0xff0000ff, 0xffffffff};
    static const D3DVERTEXELEMENT9 decl_elements[] =
    {
        {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        {1, 0, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
        D3DDECL_END()
    };
    static const short indices[] = {0, 1, 2, 3};
    static const struct
    {
        struct vec3
        {
            float x, y, z;
        } position;
        DWORD diffuse;
    }
    quad[] =
    {
        {{-1.0f, -1.0f, 0.0f}, 0xffff0000},
        {{-1.0f,  1.0f, 0.0f}, 0xff00ff00},
        {{ 1.0f, -1.0f, 0.0f}, 0xff0000ff},
        {{ 1.0f,  1.0f, 0.0f}, 0xffffffff},
    };
    static const struct vec3 quad_s0[] =
    {
        {-1.0f, -1.0f, 0.0f},
        {-1.0f,  1.0f, 0.0f},
        { 1.0f, -1.0f, 0.0f},
        { 1.0f,  1.0f, 0.0f},
    };
    static const DWORD quad_s1[] =
    {
        0xffff0000,
        0xff00ff00,
        0xff0000ff,
        0xffffffff,
    };
    IDirect3DVertexDeclaration9 *vertex_declaration;
    IDirect3DVertexBuffer9 *vb, *vb_s0, *vb_s1;
    IDirect3DTexture9 *texture;
    IDirect3DDevice9Ex *device;
    IDirect3DIndexBuffer9 *ib;
    unsigned int colour;
    D3DLOCKED_RECT lr;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    BYTE *data;

    window = create_window();
    if (!(device = create_device(window, NULL)))
    {
        skip("Failed to create a D3D device.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9Ex_SetRenderState(device, D3DRS_LIGHTING, FALSE);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_CreateVertexBuffer(device, sizeof(quad), 0, 0, D3DPOOL_SYSTEMMEM, &vb, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DVertexBuffer9_Lock(vb, 0, sizeof(quad), (void **)&data, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    memcpy(data, quad, sizeof(quad));
    hr = IDirect3DVertexBuffer9_Unlock(vb);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x77777777, 0.0f, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_SetFVF(device, D3DFVF_XYZ | D3DFVF_DIFFUSE);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_SetStreamSource(device, 0, vb, 0, sizeof(*quad));
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_BeginScene(device);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, 0, 2);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_EndScene(device);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    colour = get_pixel_color(device, 320, 240);
    ok(color_match(colour, 0x00007f7f, 1), "Got unexpected colour 0x%08x.\n", colour);

    hr = IDirect3DDevice9Ex_CreateIndexBuffer(device, sizeof(indices), 0,
            D3DFMT_INDEX16, D3DPOOL_SYSTEMMEM, &ib, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DIndexBuffer9_Lock(ib, 0, sizeof(indices), (void **)&data, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    memcpy(data, indices, sizeof(indices));
    hr = IDirect3DIndexBuffer9_Unlock(ib);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x77777777, 0.0f, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_SetIndices(device, ib);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_BeginScene(device);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_DrawIndexedPrimitive(device, D3DPT_TRIANGLESTRIP, 0, 0, 4, 0, 2);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_EndScene(device);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    colour = get_pixel_color(device, 320, 240);
    ok(color_match(colour, 0x00007f7f, 1), "Got unexpected colour 0x%08x.\n", colour);

    hr = IDirect3DDevice9Ex_CreateVertexDeclaration(device, decl_elements, &vertex_declaration);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_SetVertexDeclaration(device, vertex_declaration);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_CreateVertexBuffer(device, sizeof(quad_s0), 0, 0, D3DPOOL_SYSTEMMEM, &vb_s0, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DVertexBuffer9_Lock(vb_s0, 0, sizeof(quad_s0), (void **)&data, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    memcpy(data, quad_s0, sizeof(quad_s0));
    hr = IDirect3DVertexBuffer9_Unlock(vb_s0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CreateVertexBuffer(device, sizeof(quad_s1), 0, 0, D3DPOOL_SYSTEMMEM, &vb_s1, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DVertexBuffer9_Lock(vb_s1, 0, sizeof(quad_s1), (void **)&data, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    memcpy(data, quad_s1, sizeof(quad_s1));
    hr = IDirect3DVertexBuffer9_Unlock(vb_s1);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_SetStreamSource(device, 0, vb_s0, 0, sizeof(*quad_s0));
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_SetStreamSource(device, 1, vb_s1, 0, sizeof(*quad_s1));
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x77777777, 0.0f, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_BeginScene(device);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_DrawIndexedPrimitive(device, D3DPT_TRIANGLESTRIP, 0, 0, 4, 0, 2);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_EndScene(device);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    colour = get_pixel_color(device, 320, 240);
    ok(color_match(colour, 0x00007f7f, 1), "Got unexpected colour 0x%08x.\n", colour);

    hr = IDirect3DDevice9Ex_CreateTexture(device, 2, 2, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &texture, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    memset(&lr, 0, sizeof(lr));
    hr = IDirect3DTexture9_LockRect(texture, 0, &lr, NULL, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    memcpy(lr.pBits, texture_data, sizeof(texture_data));
    hr = IDirect3DTexture9_UnlockRect(texture, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_SetTexture(device, 0, (IDirect3DBaseTexture9 *)texture);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0x77777777, 0.0f, 0);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_SetFVF(device, D3DFVF_XYZ);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_BeginScene(device);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_DrawPrimitive(device, D3DPT_TRIANGLESTRIP, 0, 2);
    ok(hr == D3D_OK || hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_EndScene(device);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(hr == D3D_OK, "Got unexpected hr %#lx.\n", hr);

    IDirect3DTexture9_Release(texture);
    IDirect3DVertexBuffer9_Release(vb_s1);
    IDirect3DVertexBuffer9_Release(vb_s0);
    IDirect3DVertexDeclaration9_Release(vertex_declaration);
    IDirect3DIndexBuffer9_Release(ib);
    IDirect3DVertexBuffer9_Release(vb);
    refcount = IDirect3DDevice9Ex_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
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
        {0, 0, D3DPOOL_SYSTEMMEM},
    };
    static const unsigned int vertex_count = 1024;
    struct device_desc device_desc;
    IDirect3DVertexBuffer9 *buffer;
    IDirect3DDevice9Ex *device;
    D3DVERTEXBUFFER_DESC desc;
    unsigned int i, test;
    struct vec3
    {
        float x, y, z;
    } *ptr, *ptr2;
    UINT refcount;
    HWND window;
    HRESULT hr;

    window = create_window();

    for (test = 0; test < ARRAY_SIZE(tests); ++test)
    {
        device_desc.device_window = window;
        device_desc.width = 640;
        device_desc.height = 480;
        device_desc.flags = tests[test].device_flags;
        if (!(device = create_device(window, &device_desc)))
        {
            skip("Test %u: failed to create a D3D device.\n", test);
            continue;
        }

        hr = IDirect3DDevice9Ex_CreateVertexBuffer(device, vertex_count * sizeof(*ptr),
                tests[test].usage, 0, tests[test].pool, &buffer, NULL);
        ok(hr == D3D_OK, "Test %u: got unexpected hr %#lx.\n", test, hr);
        hr = IDirect3DVertexBuffer9_GetDesc(buffer, &desc);
        ok(hr == D3D_OK, "Test %u: got unexpected hr %#lx.\n", test, hr);
        ok(desc.Pool == tests[test].pool, "Test %u: got unexpected pool %#x.\n", test, desc.Pool);
        ok(desc.Usage == tests[test].usage, "Test %u: got unexpected usage %#lx.\n", test, desc.Usage);

        hr = IDirect3DVertexBuffer9_Lock(buffer, 0, vertex_count * sizeof(*ptr), (void **)&ptr, D3DLOCK_DISCARD);
        ok(hr == D3D_OK, "Test %u: got unexpected hr %#lx.\n", test, hr);
        for (i = 0; i < vertex_count; ++i)
        {
            ptr[i].x = i * 1.0f;
            ptr[i].y = i * 2.0f;
            ptr[i].z = i * 3.0f;
        }
        hr = IDirect3DVertexBuffer9_Unlock(buffer);
        ok(hr == D3D_OK, "Test %u: got unexpected hr %#lx.\n", test, hr);

        hr = IDirect3DDevice9Ex_SetFVF(device, D3DFVF_XYZ);
        ok(hr == D3D_OK, "Test %u: got unexpected hr %#lx.\n", test, hr);
        hr = IDirect3DDevice9Ex_SetStreamSource(device, 0, buffer, 0, sizeof(*ptr));
        ok(hr == D3D_OK, "Test %u: got unexpected hr %#lx.\n", test, hr);
        hr = IDirect3DDevice9Ex_BeginScene(device);
        ok(hr == D3D_OK, "Test %u: got unexpected hr %#lx.\n", test, hr);
        hr = IDirect3DDevice9Ex_DrawPrimitive(device, D3DPT_TRIANGLELIST, 0, 2);
        ok(hr == D3D_OK, "Test %u: got unexpected hr %#lx.\n", test, hr);
        hr = IDirect3DDevice9Ex_EndScene(device);
        ok(hr == D3D_OK, "Test %u: got unexpected hr %#lx.\n", test, hr);

        hr = IDirect3DVertexBuffer9_Lock(buffer, 0, vertex_count * sizeof(*ptr2), (void **)&ptr2, D3DLOCK_DISCARD);
        ok(hr == D3D_OK, "Test %u: got unexpected hr %#lx.\n", test, hr);
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
        ok(hr == D3D_OK, "Test %u: got unexpected hr %#lx.\n", test, hr);

        IDirect3DVertexBuffer9_Release(buffer);
        refcount = IDirect3DDevice9Ex_Release(device);
        ok(!refcount, "Test %u: device has %u references left.\n", test, refcount);
    }
    DestroyWindow(window);
}

static void test_desktop_window(void)
{
    IDirect3DVertexShader9 *shader;
    IDirect3DDevice9Ex *device;
    unsigned int color;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    static const DWORD simple_vs[] =
    {
        0xfffe0101,                                     /* vs_1_1             */
        0x0000001f, 0x80000000, 0x900f0000,             /* dcl_position0 v0   */
        0x00000009, 0xc0010000, 0x90e40000, 0xa0e40000, /* dp4 oPos.x, v0, c0 */
        0x00000009, 0xc0020000, 0x90e40000, 0xa0e40001, /* dp4 oPos.y, v0, c1 */
        0x00000009, 0xc0040000, 0x90e40000, 0xa0e40002, /* dp4 oPos.z, v0, c2 */
        0x00000009, 0xc0080000, 0x90e40000, 0xa0e40003, /* dp4 oPos.w, v0, c3 */
        0x0000ffff,                                     /* end                */
    };

    window = create_window();
    if (!(device = create_device(window, NULL)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        DestroyWindow(window);
        return;
    }
    IDirect3DDevice9Ex_Release(device);
    DestroyWindow(window);

    device = create_device(GetDesktopWindow(), NULL);
    ok(!!device, "Failed to create a D3D device.\n");

    hr = IDirect3DDevice9Ex_Clear(device, 0, NULL, D3DCLEAR_TARGET, 0xffff0000, 1.0f, 0);
    ok(SUCCEEDED(hr), "Failed to clear, hr %#lx.\n", hr);
    color = get_pixel_color(device, 1, 1);
    ok(color == 0x00ff0000, "Got unexpected color 0x%08x.\n", color);

    hr = IDirect3DDevice9Ex_Present(device, NULL, NULL, NULL, NULL);
    ok(SUCCEEDED(hr), "Failed to present, hr %#lx.\n", hr);

    refcount = IDirect3DDevice9Ex_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);

    /* test device with NULL HWND */
    device = create_device(NULL, NULL);
    ok(!!device, "Failed to create a D3D device\n");

    hr = IDirect3DDevice9Ex_CreateVertexShader(device, simple_vs, &shader);
    ok(SUCCEEDED(hr), "Failed to create vertex shader, hr %#lx.\n", hr);
    IDirect3DVertexShader9_Release(shader);

    IDirect3DDevice9Ex_Release(device);
}

static void test_scene(void)
{
    IDirect3DSurface9 *surface1, *surface2, *surface3;
    IDirect3DSurface9 *backBuffer, *rt, *ds;
    RECT rect = {0, 0, 128, 128};
    IDirect3DDevice9Ex *device;
    IDirect3D9 *d3d;
    ULONG refcount;
    D3DCAPS9 caps;
    HWND window;
    HRESULT hr;

    window = create_window();
    if (!(device = create_device(window, NULL)))
    {
        skip("Failed to create a D3D device.\n");
        DestroyWindow(window);
        return;
    }

    hr = IDirect3DDevice9Ex_GetDirect3D(device, &d3d);
    ok(SUCCEEDED(hr), "Failed to get Direct3D9, hr %#lx.\n", hr);

    /* Get the caps, they will be needed to tell if an operation is supposed to be valid */
    memset(&caps, 0, sizeof(caps));
    hr = IDirect3DDevice9Ex_GetDeviceCaps(device, &caps);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Test an EndScene without BeginScene. Should return an error */
    hr = IDirect3DDevice9Ex_EndScene(device);
    ok(hr == D3DERR_INVALIDCALL, "Got hr %#lx.\n", hr);

    /* Test a normal BeginScene / EndScene pair, this should work */
    hr = IDirect3DDevice9Ex_BeginScene(device);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_EndScene(device);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Test another EndScene without having begun a new scene. Should return an error */
    hr = IDirect3DDevice9Ex_EndScene(device);
    ok(hr == D3DERR_INVALIDCALL, "Got hr %#lx.\n", hr);

    /* Two nested BeginScene and EndScene calls */
    hr = IDirect3DDevice9Ex_BeginScene(device);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_BeginScene(device);
    ok(hr == D3DERR_INVALIDCALL, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_EndScene(device);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_EndScene(device);
    ok(hr == D3DERR_INVALIDCALL, "Got hr %#lx.\n", hr);

    /* Calling Reset does not clear scene state, different from d3d9. */
    hr = IDirect3DDevice9Ex_BeginScene(device);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    reset_device(device, NULL);
    hr = IDirect3DDevice9Ex_EndScene(device);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_BeginScene(device);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_EndScene(device);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Create some surfaces to test stretchrect between the scenes */
    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurface(device, 128, 128,
            D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &surface1, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CreateOffscreenPlainSurface(device, 128, 128,
            D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &surface2, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CreateDepthStencilSurface(device, 800, 600,
            D3DFMT_D16, D3DMULTISAMPLE_NONE, 0, FALSE, &surface3, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_CreateRenderTarget(device, 128, 128,
            D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, FALSE, &rt, NULL);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_GetBackBuffer(device, 0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_GetDepthStencilSurface(device, &ds);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* First make sure a simple StretchRect call works */
    hr = IDirect3DDevice9Ex_StretchRect(device, surface1, NULL, surface2, NULL, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_StretchRect(device, backBuffer, &rect, rt, NULL, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    if (0) /* Disabled for now because it crashes in wine */
    {
        HRESULT expected = caps.DevCaps2 & D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES ? D3D_OK : D3DERR_INVALIDCALL;
        hr = IDirect3DDevice9Ex_StretchRect(device, ds, NULL, surface3, NULL, 0);
        ok(hr == expected, "Got unexpected hr %#lx, expected %#lx.\n", hr, expected);
    }

    /* Now try it in a BeginScene - EndScene pair. Seems to be allowed in a
     * BeginScene - Endscene pair with normal surfaces and render targets, but
     * not depth stencil surfaces. */
    hr = IDirect3DDevice9Ex_BeginScene(device);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_StretchRect(device, surface1, NULL, surface2, NULL, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_StretchRect(device, backBuffer, &rect, rt, NULL, 0);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    /* This is supposed to fail inside a BeginScene - EndScene pair. */
    hr = IDirect3DDevice9Ex_StretchRect(device, ds, NULL, surface3, NULL, 0);
    ok(hr == D3DERR_INVALIDCALL, "Got hr %#lx.\n", hr);

    hr = IDirect3DDevice9Ex_EndScene(device);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    /* Does a SetRenderTarget influence BeginScene / EndScene ?
     * Set a new render target, then see if it started a new scene. Flip the rt back and see if that maybe
     * ended the scene. Expected result is that the scene is not affected by SetRenderTarget
     */
    hr = IDirect3DDevice9Ex_SetRenderTarget(device, 0, rt);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_BeginScene(device);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_SetRenderTarget(device, 0, backBuffer);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);
    hr = IDirect3DDevice9Ex_EndScene(device);
    ok(hr == S_OK, "Got hr %#lx.\n", hr);

    IDirect3DSurface9_Release(rt);
    IDirect3DSurface9_Release(ds);
    IDirect3DSurface9_Release(backBuffer);
    IDirect3DSurface9_Release(surface1);
    IDirect3DSurface9_Release(surface2);
    IDirect3DSurface9_Release(surface3);
    refcount = IDirect3DDevice9Ex_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);

    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

START_TEST(d3d9ex)
{
    DEVMODEW current_mode;
    WNDCLASSA wc = {0};

    d3d9_handle = LoadLibraryA("d3d9.dll");
    if (!d3d9_handle)
    {
        skip("Could not load d3d9.dll\n");
        return;
    }

    pDirect3DCreate9Ex = (void *)GetProcAddress(d3d9_handle, "Direct3DCreate9Ex");
    if (!pDirect3DCreate9Ex) {
        win_skip("Failed to get address of Direct3DCreate9Ex\n");
        return;
    }

    memset(&current_mode, 0, sizeof(current_mode));
    current_mode.dmSize = sizeof(current_mode);
    ok(EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &current_mode), "Failed to get display mode.\n");
    registry_mode.dmSize = sizeof(registry_mode);
    ok(EnumDisplaySettingsW(NULL, ENUM_REGISTRY_SETTINGS, &registry_mode), "Failed to get display mode.\n");
    if (current_mode.dmPelsWidth != registry_mode.dmPelsWidth
            || current_mode.dmPelsHeight != registry_mode.dmPelsHeight)
    {
        skip("Current mode does not match registry mode, skipping test.\n");
        return;
    }

    wc.lpfnWndProc = DefWindowProcA;
    wc.lpszClassName = "d3d9_test_wc";
    RegisterClassA(&wc);

    test_qi_base_to_ex();
    test_qi_ex_to_base();
    test_swapchain_get_displaymode_ex();
    test_get_adapter_luid();
    test_get_adapter_displaymode_ex();
    test_create_depth_stencil_surface_ex();
    test_user_memory();
    test_reset();
    test_reset_ex();
    test_reset_resources();
    test_vidmem_accounting();
    test_user_memory_getdc();
    test_lost_device();
    test_unsupported_shaders();
    test_wndproc();
    test_wndproc_windowed();
    test_window_style();
    test_swapchain_parameters();
    test_backbuffer_resize();
    test_format_unknown();
    test_device_caps();
    test_frame_latency();
    test_resource_access();
    test_sysmem_draw();
    test_pinned_buffers();
    test_desktop_window();
    test_scene();

    UnregisterClassA("d3d9_test_wc", GetModuleHandleA(NULL));
}
