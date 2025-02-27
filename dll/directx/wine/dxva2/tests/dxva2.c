/*
 * Copyright 2020 Nikolay Sivov
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
#include "d3d9.h"

#include "initguid.h"
#include "dxva2api.h"

static unsigned int get_refcount(void *object)
{
    IUnknown *iface = object;
    IUnknown_AddRef(iface);
    return IUnknown_Release(iface);
}

static HWND create_window(void)
{
    RECT r = {0, 0, 640, 480};

    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW | WS_VISIBLE, FALSE);

    return CreateWindowA("static", "d3d9_test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            0, 0, r.right - r.left, r.bottom - r.top, NULL, NULL, NULL, NULL);
}

static IDirect3DDevice9 *create_device(IDirect3D9 *d3d9, HWND focus_window)
{
    D3DPRESENT_PARAMETERS present_parameters = {0};
    IDirect3DDevice9 *device = NULL;

    present_parameters.BackBufferWidth = 640;
    present_parameters.BackBufferHeight = 480;
    present_parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    present_parameters.hDeviceWindow = focus_window;
    present_parameters.Windowed = TRUE;
    present_parameters.EnableAutoDepthStencil = TRUE;
    present_parameters.AutoDepthStencilFormat = D3DFMT_D24S8;

    IDirect3D9_CreateDevice(d3d9, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, focus_window,
            D3DCREATE_HARDWARE_VERTEXPROCESSING, &present_parameters, &device);

    return device;
}

static void test_surface_desc(IDirect3DSurface9 *surface)
{
    D3DSURFACE_DESC desc = { 0 };
    HRESULT hr;

    hr = IDirect3DSurface9_GetDesc(surface, &desc);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ok(desc.Format == D3DFMT_X8R8G8B8, "Unexpected format %d.\n", desc.Format);
    ok(desc.Type == D3DRTYPE_SURFACE, "Unexpected type %d.\n", desc.Type);
    ok(!desc.Usage, "Unexpected usage %ld.\n", desc.Usage);
    ok(desc.Pool == D3DPOOL_DEFAULT, "Unexpected pool %d.\n", desc.Pool);
    ok(desc.MultiSampleType == D3DMULTISAMPLE_NONE, "Unexpected multisample type %d.\n", desc.MultiSampleType);
    ok(!desc.MultiSampleQuality, "Unexpected multisample quality %ld.\n", desc.MultiSampleQuality);
    ok(desc.Width == 64, "Unexpected width %u.\n", desc.Width);
    ok(desc.Height == 64, "Unexpected height %u.\n", desc.Height);
}

static void init_video_desc(DXVA2_VideoDesc *video_desc, D3DFORMAT format)
{
    memset(video_desc, 0, sizeof(*video_desc));
    video_desc->SampleWidth = 64;
    video_desc->SampleHeight = 64;
    video_desc->Format = format;
}

static void test_device_manager(void)
{
    IDirectXVideoProcessorService *processor_service;
    IDirectXVideoAccelerationService *accel_service;
    IDirect3DDevice9 *device, *device2, *device3;
    IDirectXVideoProcessorService *proc_service;
    IDirect3DDeviceManager9 *manager;
    IDirect3DSurface9 *surfaces[2];
    DXVA2_VideoDesc video_desc;
    int refcount, refcount2;
    HANDLE handle, handle1;
    D3DFORMAT *formats;
    UINT token, count;
    IDirect3D9 *d3d;
    unsigned int i;
    HWND window;
    GUID *guids;
    HRESULT hr;
    RECT rect;

    static const D3DFORMAT rt_formats[] =
    {
        D3DFMT_A8R8G8B8,
        D3DFMT_X8R8G8B8,
        D3DFMT_YUY2,
        MAKEFOURCC('A','Y','U','V'),
    };
    static const D3DFORMAT rt_unsupported_formats[] =
    {
        D3DFMT_A1R5G5B5,
        D3DFMT_X1R5G5B5,
        D3DFMT_A2R10G10B10,
        D3DFMT_A8B8G8R8,
        D3DFMT_X8B8G8R8,
        D3DFMT_R5G6B5,
        D3DFMT_UYVY,
    };

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = DXVA2CreateDirect3DDeviceManager9(&token, &manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle);
    ok(hr == DXVA2_E_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_LockDevice(manager, 0, &device2, FALSE);
    ok(hr == DXVA2_E_NOT_INITIALIZED, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, 0);
    ok(hr == E_HANDLE, "Unexpected hr %#lx.\n", hr);

    /* Invalid token. */
    hr = IDirect3DDeviceManager9_ResetDevice(manager, device, token + 1);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_ResetDevice(manager, device, token);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    refcount = get_refcount(device);

    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, 0);
    ok(hr == E_HANDLE, "Unexpected hr %#lx.\n", hr);

    handle1 = NULL;
    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!handle1, "Unexpected handle value.\n");

    refcount2 = get_refcount(device);
    ok(refcount2 == refcount, "Unexpected refcount %d.\n", refcount);

    handle = NULL;
    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(handle != handle1, "Unexpected handle.\n");

    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Already closed. */
    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle);
    ok(hr == E_HANDLE, "Unexpected hr %#lx.\n", hr);

    handle = NULL;
    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_TestDevice(manager, handle1);
    ok(hr == E_HANDLE, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_TestDevice(manager, 0);
    ok(hr == E_HANDLE, "Unexpected hr %#lx.\n", hr);

    handle = NULL;
    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    handle1 = NULL;
    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_GetVideoService(manager, handle, &IID_IDirectXVideoProcessorService,
            (void **)&processor_service);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IDirectXVideoProcessorService_Release(processor_service);

    device2 = create_device(d3d, window);

    hr = IDirect3DDeviceManager9_ResetDevice(manager, device2, token);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_GetVideoService(manager, handle, &IID_IDirectXVideoProcessorService,
            (void **)&processor_service);
    ok(hr == DXVA2_E_NEW_VIDEO_DEVICE, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_TestDevice(manager, handle);
    ok(hr == DXVA2_E_NEW_VIDEO_DEVICE, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Lock/Unlock. */
    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_LockDevice(manager, handle, &device3, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(device2 == device3, "Unexpected device pointer.\n");
    IDirect3DDevice9_Release(device3);

    hr = IDirect3DDeviceManager9_UnlockDevice(manager, handle, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_UnlockDevice(manager, handle, FALSE);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_UnlockDevice(manager, (HANDLE)((ULONG_PTR)handle + 100), FALSE);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* Locked with one handle, unlock with another. */
    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_LockDevice(manager, handle, &device3, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(device2 == device3, "Unexpected device pointer.\n");
    IDirect3DDevice9_Release(device3);

    hr = IDirect3DDeviceManager9_UnlockDevice(manager, handle1, FALSE);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    /* Closing unlocks the device. */
    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_LockDevice(manager, handle1, &device3, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(device2 == device3, "Unexpected device pointer.\n");
    IDirect3DDevice9_Release(device3);

    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Open two handles. */
    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_LockDevice(manager, handle, &device3, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(device2 == device3, "Unexpected device pointer.\n");
    IDirect3DDevice9_Release(device3);

    hr = IDirect3DDeviceManager9_LockDevice(manager, handle1, &device3, FALSE);
    ok(hr == DXVA2_E_VIDEO_DEVICE_LOCKED, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* State saving function. */
    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_LockDevice(manager, handle, &device3, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(device2 == device3, "Unexpected device pointer.\n");

    SetRect(&rect, 50, 60, 70, 80);
    hr = IDirect3DDevice9_SetScissorRect(device3, &rect);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_UnlockDevice(manager, handle, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    SetRect(&rect, 30, 60, 70, 80);
    hr = IDirect3DDevice9_SetScissorRect(device3, &rect);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IDirect3DDevice9_Release(device3);

    hr = IDirect3DDeviceManager9_LockDevice(manager, handle, &device3, FALSE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(device2 == device3, "Unexpected device pointer.\n");

    hr = IDirect3DDevice9_GetScissorRect(device3, &rect);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(rect.left == 50 && rect.top == 60 && rect.right == 70 && rect.bottom == 80,
            "Got unexpected scissor rect %s.\n", wine_dbgstr_rect(&rect));

    IDirect3DDevice9_Release(device3);

    hr = IDirect3DDeviceManager9_UnlockDevice(manager, handle, TRUE);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Acceleration service. */
    hr = DXVA2CreateVideoService(device, &IID_IDirectXVideoAccelerationService, (void **)&accel_service);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    surfaces[0] = surfaces[1] = NULL;
    hr = IDirectXVideoAccelerationService_CreateSurface(accel_service, 64, 64, 0, D3DFMT_X8R8G8B8,
            D3DPOOL_DEFAULT, 0, DXVA2_VideoProcessorRenderTarget, surfaces, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!surfaces[0] && !surfaces[1], "Unexpected surfaces.\n");
    IDirect3DSurface9_Release(surfaces[0]);

    hr = IDirectXVideoAccelerationService_CreateSurface(accel_service, 64, 64, 1, D3DFMT_X8R8G8B8,
            D3DPOOL_DEFAULT, 0, DXVA2_VideoProcessorRenderTarget, surfaces, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(!!surfaces[0] && !!surfaces[1], "Unexpected surfaces.\n");
    test_surface_desc(surfaces[0]);
    IDirect3DSurface9_Release(surfaces[0]);
    IDirect3DSurface9_Release(surfaces[1]);

    IDirectXVideoAccelerationService_Release(accel_service);

    /* RT formats. */
    hr = DXVA2CreateVideoService(device, &IID_IDirectXVideoProcessorService, (void **)&proc_service);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    init_video_desc(&video_desc, D3DFMT_A8R8G8B8);

    hr = IDirectXVideoProcessorService_GetVideoProcessorDeviceGuids(proc_service, &video_desc, &count, &guids);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(count, "Unexpected format count %u.\n", count);
    CoTaskMemFree(guids);

    for (i = 0; i < ARRAY_SIZE(rt_formats); ++i)
    {
        init_video_desc(&video_desc, rt_formats[i]);

        count = 0;
        hr = IDirectXVideoProcessorService_GetVideoProcessorDeviceGuids(proc_service, &video_desc, &count, &guids);
        todo_wine_if(rt_formats[i] == MAKEFOURCC('A','Y','U','V'))
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        if (FAILED(hr)) continue;
        ok(count > 0, "Unexpected device count.\n");
        CoTaskMemFree(guids);

        count = 0;
        hr = IDirectXVideoProcessorService_GetVideoProcessorRenderTargets(proc_service, &DXVA2_VideoProcSoftwareDevice,
                &video_desc, &count, &formats);
        ok(hr == S_OK, "Unexpected hr %#lx, format %d.\n", hr, rt_formats[i]);
        ok(count == 2, "Unexpected format count %u.\n", count);
        if (count == 2)
            ok(formats[0] == D3DFMT_X8R8G8B8 && formats[1] == D3DFMT_A8R8G8B8, "Unexpected formats %d,%d.\n",
                    formats[0], formats[1]);
        CoTaskMemFree(formats);
    }

    for (i = 0; i < ARRAY_SIZE(rt_unsupported_formats); ++i)
    {
        init_video_desc(&video_desc, rt_unsupported_formats[i]);

        hr = IDirectXVideoProcessorService_GetVideoProcessorRenderTargets(proc_service, &DXVA2_VideoProcSoftwareDevice,
                &video_desc, &count, &formats);
        ok(hr == E_FAIL, "Unexpected hr %#lx, format %d.\n", hr, rt_unsupported_formats[i]);
    }

    IDirectXVideoProcessorService_Release(proc_service);

    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_GetVideoService(manager, handle, &IID_IDirectXVideoAccelerationService,
            (void **)&accel_service);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXVideoAccelerationService_CreateSurface(accel_service, 64, 64, 0, D3DFMT_X8R8G8B8,
            D3DPOOL_DEFAULT, 0, DXVA2_VideoProcessorRenderTarget, surfaces, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DSurface9_GetDevice(surfaces[0], &device3);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(device2 == device3, "Unexpected device.\n");
    IDirect3DDevice9_Release(device3);

    IDirect3DSurface9_Release(surfaces[0]);

    IDirectXVideoAccelerationService_Release(accel_service);

    IDirect3DDevice9_Release(device);
    IDirect3DDevice9_Release(device2);

    IDirect3DDeviceManager9_Release(manager);

done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static void test_video_processor(void)
{
    IDirectXVideoProcessorService *service, *service2;
    IDirectXVideoProcessor *processor, *processor2;
    IDirect3DDeviceManager9 *manager;
    DXVA2_VideoProcessorCaps caps;
    DXVA2_VideoDesc video_desc;
    IDirect3DDevice9 *device;
    HANDLE handle, handle1;
    D3DFORMAT format;
    IDirect3D9 *d3d;
    HWND window;
    UINT token;
    HRESULT hr;
    GUID guid;

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = DXVA2CreateDirect3DDeviceManager9(&token, &manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_ResetDevice(manager, device, token);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    handle = NULL;
    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    handle1 = NULL;
    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    ok(get_refcount(manager) == 1, "Unexpected refcount %u.\n", get_refcount(manager));

    hr = IDirect3DDeviceManager9_GetVideoService(manager, handle, &IID_IDirectXVideoProcessorService,
            (void **)&service);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    memset(&video_desc, 0, sizeof(video_desc));
    video_desc.SampleWidth = 64;
    video_desc.SampleHeight = 64;
    video_desc.Format = D3DFMT_A8R8G8B8;

    /* Number of substreams does not include reference stream. */
    hr = IDirectXVideoProcessorService_CreateVideoProcessor(service, &DXVA2_VideoProcSoftwareDevice, &video_desc,
            D3DFMT_A8R8G8B8, 16, &processor);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXVideoProcessorService_CreateVideoProcessor(service, &DXVA2_VideoProcSoftwareDevice, &video_desc,
            D3DFMT_A8R8G8B8, 15, &processor);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IDirectXVideoProcessor_Release(processor);

    hr = IDirectXVideoProcessorService_CreateVideoProcessor(service, &DXVA2_VideoProcSoftwareDevice, &video_desc,
            D3DFMT_A8R8G8B8, 0, &processor);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    IDirectXVideoProcessor_Release(processor);

    hr = IDirectXVideoProcessorService_CreateVideoProcessor(service, &DXVA2_VideoProcSoftwareDevice, &video_desc,
            D3DFMT_A8R8G8B8, 1, &processor);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXVideoProcessor_GetVideoProcessorCaps(processor, &caps);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(caps.DeviceCaps == DXVA2_VPDev_SoftwareDevice, "Unexpected device type %#x.\n", caps.DeviceCaps);
    ok(caps.InputPool == D3DPOOL_SYSTEMMEM, "Unexpected input pool %#x.\n", caps.InputPool);
    ok(!caps.NumForwardRefSamples, "Unexpected sample count.\n");
    ok(!caps.NumBackwardRefSamples, "Unexpected sample count.\n");
    ok(!caps.Reserved, "Unexpected field.\n");
    ok(caps.DeinterlaceTechnology == DXVA2_DeinterlaceTech_Unknown, "Unexpected deinterlace technology %#x.\n",
            caps.DeinterlaceTechnology);
    ok(!caps.ProcAmpControlCaps, "Unexpected proc amp mask %#x.\n", caps.ProcAmpControlCaps);
    ok(caps.VideoProcessorOperations == (DXVA2_VideoProcess_PlanarAlpha | DXVA2_VideoProcess_YUV2RGB |
            DXVA2_VideoProcess_StretchX | DXVA2_VideoProcess_StretchY | DXVA2_VideoProcess_SubRects |
            DXVA2_VideoProcess_SubStreams | DXVA2_VideoProcess_SubStreamsExtended | DXVA2_VideoProcess_YUV2RGBExtended),
            "Unexpected processor operations %#x.\n", caps.VideoProcessorOperations);
    ok(caps.NoiseFilterTechnology == DXVA2_NoiseFilterTech_Unsupported, "Unexpected noise filter technology %#x.\n",
            caps.NoiseFilterTechnology);
    ok(caps.DetailFilterTechnology == DXVA2_DetailFilterTech_Unsupported, "Unexpected detail filter technology %#x.\n",
            caps.DetailFilterTechnology);

    hr = IDirectXVideoProcessorService_CreateVideoProcessor(service, &DXVA2_VideoProcSoftwareDevice, &video_desc,
            D3DFMT_A8R8G8B8, 1, &processor2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(processor2 != processor, "Unexpected instance.\n");

    hr = IDirectXVideoProcessor_GetCreationParameters(processor, NULL, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXVideoProcessor_GetCreationParameters(processor, &guid, NULL, NULL, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &DXVA2_VideoProcSoftwareDevice), "Unexpected device guid.\n");

    hr = IDirectXVideoProcessor_GetCreationParameters(processor, NULL, NULL, &format, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(format == D3DFMT_A8R8G8B8, "Unexpected format %u.\n", format);

    IDirectXVideoProcessor_Release(processor);
    IDirectXVideoProcessor_Release(processor2);

    hr = IDirect3DDeviceManager9_GetVideoService(manager, handle, &IID_IDirectXVideoProcessorService,
            (void **)&service2);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(service == service2, "Unexpected pointer.\n");

    IDirectXVideoProcessorService_Release(service2);
    IDirectXVideoProcessorService_Release(service);

    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle1);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IDirect3DDevice9_Release(device);
    IDirect3DDeviceManager9_Release(manager);

done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

static BOOL check_format_list(D3DFORMAT format, const D3DFORMAT *list, unsigned int count)
{
    unsigned int i;
    for (i = 0; i < count; ++i)
        if (list[i] == format) return TRUE;
    return FALSE;
}

static void test_progressive_device(void)
{
    static const unsigned int processor_ops = DXVA2_VideoProcess_YUV2RGB |
            DXVA2_VideoProcess_StretchX | DXVA2_VideoProcess_StretchY;
    IDirectXVideoProcessorService *service;
    IDirectXVideoProcessor *processor;
    IDirect3DDeviceManager9 *manager;
    D3DFORMAT format, *rt_formats;
    DXVA2_VideoProcessorCaps caps;
    DXVA2_VideoDesc video_desc;
    IDirect3DDevice9 *device;
    unsigned int count, i, j;
    GUID guid, *guids;
    IDirect3D9 *d3d;
    HANDLE handle;
    HWND window;
    UINT token;
    HRESULT hr;

    static const D3DFORMAT input_formats[] =
    {
        D3DFMT_X8R8G8B8,
        D3DFMT_YUY2,
        MAKEFOURCC('N','V','1','2'),
    };

    static const D3DFORMAT supported_rt_formats[] =
    {
        D3DFMT_X8R8G8B8,
        MAKEFOURCC('N','V','1','2'),
        D3DFMT_YUY2,
        D3DFMT_A2R10G10B10,
    };

    window = create_window();
    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    ok(!!d3d, "Failed to create a D3D object.\n");
    if (!(device = create_device(d3d, window)))
    {
        skip("Failed to create a D3D device, skipping tests.\n");
        goto done;
    }

    hr = DXVA2CreateDirect3DDeviceManager9(&token, &manager);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_ResetDevice(manager, device, token);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    handle = NULL;
    hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirect3DDeviceManager9_GetVideoService(manager, handle, &IID_IDirectXVideoProcessorService,
            (void **)&service);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    memset(&video_desc, 0, sizeof(video_desc));
    video_desc.SampleWidth = 64;
    video_desc.SampleHeight = 64;
    video_desc.Format = MAKEFOURCC('N','V','1','2');

    hr = IDirectXVideoProcessorService_CreateVideoProcessor(service, &DXVA2_VideoProcProgressiveDevice, &video_desc,
            D3DFMT_A8R8G8B8, 0, &processor);
    if (FAILED(hr))
    {
        win_skip("VideoProcProgressiveDevice is not supported.\n");
        goto unsupported;
    }
    IDirectXVideoProcessor_Release(processor);

    for (i = 0; i < ARRAY_SIZE(input_formats); ++i)
    {
        init_video_desc(&video_desc, input_formats[i]);

        /* Check that progressive device is returned for given input format. */
        count = 0;
        hr = IDirectXVideoProcessorService_GetVideoProcessorDeviceGuids(service, &video_desc, &count, &guids);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        ok(count > 0, "Unexpected device count.\n");
        for (j = 0; j < count; ++j)
        {
            if (IsEqualGUID(&guids[j], &DXVA2_VideoProcProgressiveDevice)) break;
        }
        ok(j < count, "Expected progressive device for format %#x.\n", input_formats[i]);
        CoTaskMemFree(guids);

        count = 0;
        hr = IDirectXVideoProcessorService_GetVideoProcessorRenderTargets(service, &DXVA2_VideoProcProgressiveDevice,
                &video_desc, &count, &rt_formats);
        ok(hr == S_OK, "Unexpected hr %#lx, format %d.\n", hr, input_formats[i]);
        ok(count > 0, "Unexpected format count %u.\n", count);
        for (j = 0; j < count; ++j)
        {
            ok(check_format_list(rt_formats[j], supported_rt_formats, ARRAY_SIZE(supported_rt_formats)),
                    "Unexpected rt format %#x for input format %#x.\n", rt_formats[j], input_formats[i]);
        }
        CoTaskMemFree(rt_formats);
    }

    memset(&video_desc, 0, sizeof(video_desc));
    video_desc.SampleWidth = 64;
    video_desc.SampleHeight = 64;
    video_desc.Format = MAKEFOURCC('N','V','1','2');

    hr = IDirectXVideoProcessorService_CreateVideoProcessor(service, &DXVA2_VideoProcProgressiveDevice, &video_desc,
            D3DFMT_A8R8G8B8, 0, &processor);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IDirectXVideoProcessor_GetVideoProcessorCaps(processor, &caps);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(caps.DeviceCaps == DXVA2_VPDev_HardwareDevice, "Unexpected device type %#x.\n", caps.DeviceCaps);
    ok(!caps.NumForwardRefSamples, "Unexpected sample count.\n");
    ok(!caps.NumBackwardRefSamples, "Unexpected sample count.\n");
    ok(!caps.Reserved, "Unexpected field.\n");
    ok((caps.VideoProcessorOperations & processor_ops) == processor_ops, "Unexpected processor operations %#x.\n",
            caps.VideoProcessorOperations);

    hr = IDirectXVideoProcessor_GetCreationParameters(processor, &guid, NULL, &format, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(IsEqualGUID(&guid, &DXVA2_VideoProcProgressiveDevice), "Unexpected device guid.\n");
    ok(format == D3DFMT_A8R8G8B8, "Unexpected format %u.\n", format);

    IDirectXVideoProcessor_Release(processor);

unsupported:
    IDirectXVideoProcessorService_Release(service);

    hr = IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    IDirect3DDevice9_Release(device);
    IDirect3DDeviceManager9_Release(manager);

done:
    IDirect3D9_Release(d3d);
    DestroyWindow(window);
}

START_TEST(dxva2)
{
    IDirect3D9 *d3d;

    d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d)
    {
        skip("Failed to initialize D3D9. Skipping tests.\n");
        return;
    }
    IDirect3D9_Release(d3d);

    test_device_manager();
    test_video_processor();
    test_progressive_device();
}
