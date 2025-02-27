/*
 * Copyright 2008 Henri Verbeet for CodeWeavers
 * Copyright 2015 Józef Kucia for CodeWeavers
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
#include "d3d11_1.h"
#include "wine/test.h"

static const D3D10_FEATURE_LEVEL1 d3d10_feature_levels[] =
{
    D3D10_FEATURE_LEVEL_10_1,
    D3D10_FEATURE_LEVEL_10_0,
    D3D10_FEATURE_LEVEL_9_3,
    D3D10_FEATURE_LEVEL_9_2,
    D3D10_FEATURE_LEVEL_9_1
};

static ULONG get_refcount(IUnknown *iface)
{
    IUnknown_AddRef(iface);
    return IUnknown_Release(iface);
}

struct device_desc
{
    D3D10_FEATURE_LEVEL1 feature_level;
    UINT flags;
};

static ID3D10Device1 *create_device(const struct device_desc *desc)
{
    D3D10_FEATURE_LEVEL1 feature_level = D3D10_FEATURE_LEVEL_10_1;
    ID3D10Device1 *device;
    UINT flags = 0;

    if (desc)
    {
        feature_level = desc->feature_level;
        flags = desc->flags;
    }

    if (SUCCEEDED(D3D10CreateDevice1(NULL, D3D10_DRIVER_TYPE_HARDWARE,
            NULL, flags, feature_level, D3D10_1_SDK_VERSION, &device)))
        return device;
    if (SUCCEEDED(D3D10CreateDevice1(NULL, D3D10_DRIVER_TYPE_WARP,
            NULL, flags, feature_level, D3D10_1_SDK_VERSION, &device)))
        return device;
    if (SUCCEEDED(D3D10CreateDevice1(NULL, D3D10_DRIVER_TYPE_REFERENCE,
            NULL, flags, feature_level, D3D10_1_SDK_VERSION, &device)))
        return device;

    return NULL;
}

#define check_interface(a, b, c, d) check_interface_(__LINE__, a, b, c, d)
static HRESULT check_interface_(unsigned int line, void *iface, REFIID iid, BOOL supported, BOOL is_broken)
{
    HRESULT hr, expected_hr, broken_hr;
    IUnknown *unknown = iface, *out;

    if (supported)
    {
        expected_hr = S_OK;
        broken_hr = E_NOINTERFACE;
    }
    else
    {
        expected_hr = E_NOINTERFACE;
        broken_hr = S_OK;
    }

    hr = IUnknown_QueryInterface(unknown, iid, (void **)&out);
    ok_(__FILE__, line)(hr == expected_hr || broken(is_broken && hr == broken_hr),
            "Got unexpected hr %#lx, expected %#lx.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(out);
    return hr;
}

static void test_create_device(void)
{
    D3D10_FEATURE_LEVEL1 feature_level, supported_feature_level;
    DXGI_SWAP_CHAIN_DESC swapchain_desc, obtained_desc;
    IDXGISwapChain *swapchain;
    ID3D10Device1 *device;
    unsigned int i;
    ULONG refcount;
    HWND window;
    HRESULT hr;

    for (i = 0; i < ARRAY_SIZE(d3d10_feature_levels); ++i)
    {
        if (SUCCEEDED(hr = D3D10CreateDevice1(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0,
                d3d10_feature_levels[i], D3D10_1_SDK_VERSION, &device)))
        {
            supported_feature_level = d3d10_feature_levels[i];
            break;
        }
    }

    if (FAILED(hr))
    {
        skip("Failed to create HAL device.\n");
        return;
    }

    feature_level = ID3D10Device1_GetFeatureLevel(device);
    ok(feature_level == supported_feature_level, "Got feature level %#x, expected %#x.\n",
            feature_level, supported_feature_level);

    ID3D10Device1_Release(device);

    hr = D3D10CreateDevice1(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0,
            supported_feature_level, D3D10_1_SDK_VERSION, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    device = (ID3D10Device1 *)0xdeadbeef;
    hr = D3D10CreateDevice1(NULL, 0xffffffff, NULL, 0,
            supported_feature_level, D3D10_1_SDK_VERSION, &device);
    todo_wine ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    ok(!device, "Got unexpected device pointer %p.\n", device);

    device = (ID3D10Device1 *)0xdeadbeef;
    hr = D3D10CreateDevice1(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0,
            0, D3D10_1_SDK_VERSION, &device);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    ok(!device, "Got unexpected device pointer %p.\n", device);

    window = CreateWindowA("static", "d3d10_1_test", 0, 0, 0, 0, 0, 0, 0, 0, 0);

    swapchain_desc.BufferDesc.Width = 800;
    swapchain_desc.BufferDesc.Height = 600;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 60;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = 1;
    swapchain_desc.OutputWindow = window;
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = 0;

    hr = D3D10CreateDeviceAndSwapChain1(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0,
            supported_feature_level, D3D10_1_SDK_VERSION, &swapchain_desc, &swapchain, &device);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    check_interface(swapchain, &IID_IDXGISwapChain1, TRUE, FALSE);

    memset(&obtained_desc, 0, sizeof(obtained_desc));
    hr = IDXGISwapChain_GetDesc(swapchain, &obtained_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(obtained_desc.BufferDesc.Width == swapchain_desc.BufferDesc.Width,
            "Got unexpected BufferDesc.Width %u.\n", obtained_desc.BufferDesc.Width);
    ok(obtained_desc.BufferDesc.Height == swapchain_desc.BufferDesc.Height,
            "Got unexpected BufferDesc.Height %u.\n", obtained_desc.BufferDesc.Height);
    todo_wine ok(obtained_desc.BufferDesc.RefreshRate.Numerator == swapchain_desc.BufferDesc.RefreshRate.Numerator,
            "Got unexpected BufferDesc.RefreshRate.Numerator %u.\n",
            obtained_desc.BufferDesc.RefreshRate.Numerator);
    todo_wine ok(obtained_desc.BufferDesc.RefreshRate.Denominator == swapchain_desc.BufferDesc.RefreshRate.Denominator,
            "Got unexpected BufferDesc.RefreshRate.Denominator %u.\n",
            obtained_desc.BufferDesc.RefreshRate.Denominator);
    ok(obtained_desc.BufferDesc.Format == swapchain_desc.BufferDesc.Format,
            "Got unexpected BufferDesc.Format %#x.\n", obtained_desc.BufferDesc.Format);
    ok(obtained_desc.BufferDesc.ScanlineOrdering == swapchain_desc.BufferDesc.ScanlineOrdering,
            "Got unexpected BufferDesc.ScanlineOrdering %#x.\n", obtained_desc.BufferDesc.ScanlineOrdering);
    ok(obtained_desc.BufferDesc.Scaling == swapchain_desc.BufferDesc.Scaling,
            "Got unexpected BufferDesc.Scaling %#x.\n", obtained_desc.BufferDesc.Scaling);
    ok(obtained_desc.SampleDesc.Count == swapchain_desc.SampleDesc.Count,
            "Got unexpected SampleDesc.Count %u.\n", obtained_desc.SampleDesc.Count);
    ok(obtained_desc.SampleDesc.Quality == swapchain_desc.SampleDesc.Quality,
            "Got unexpected SampleDesc.Quality %u.\n", obtained_desc.SampleDesc.Quality);
    ok(obtained_desc.BufferUsage == swapchain_desc.BufferUsage,
            "Got unexpected BufferUsage %#x.\n", obtained_desc.BufferUsage);
    ok(obtained_desc.BufferCount == swapchain_desc.BufferCount,
            "Got unexpected BufferCount %u.\n", obtained_desc.BufferCount);
    ok(obtained_desc.OutputWindow == swapchain_desc.OutputWindow,
            "Got unexpected OutputWindow %p.\n", obtained_desc.OutputWindow);
    ok(obtained_desc.Windowed == swapchain_desc.Windowed,
            "Got unexpected Windowed %#x.\n", obtained_desc.Windowed);
    ok(obtained_desc.SwapEffect == swapchain_desc.SwapEffect,
            "Got unexpected SwapEffect %#x.\n", obtained_desc.SwapEffect);
    ok(obtained_desc.Flags == swapchain_desc.Flags,
            "Got unexpected Flags %#x.\n", obtained_desc.Flags);

    refcount = IDXGISwapChain_Release(swapchain);
    ok(!refcount, "Swapchain has %lu references left.\n", refcount);

    feature_level = ID3D10Device1_GetFeatureLevel(device);
    ok(feature_level == supported_feature_level, "Got feature level %#x, expected %#x.\n",
            feature_level, supported_feature_level);

    refcount = ID3D10Device1_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);

    hr = D3D10CreateDeviceAndSwapChain1(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0,
            supported_feature_level, D3D10_1_SDK_VERSION, NULL, NULL, &device);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    refcount = ID3D10Device1_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);

    hr = D3D10CreateDeviceAndSwapChain1(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0,
            supported_feature_level, D3D10_1_SDK_VERSION, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = D3D10CreateDeviceAndSwapChain1(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0,
            supported_feature_level, D3D10_1_SDK_VERSION, &swapchain_desc, NULL, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    swapchain = (IDXGISwapChain *)0xdeadbeef;
    device = (ID3D10Device1 *)0xdeadbeef;
    hr = D3D10CreateDeviceAndSwapChain1(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0,
            0, D3D10_1_SDK_VERSION, &swapchain_desc, &swapchain, &device);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    ok(!swapchain, "Got unexpected swapchain pointer %p.\n", swapchain);
    ok(!device, "Got unexpected device pointer %p.\n", device);

    swapchain = (IDXGISwapChain *)0xdeadbeef;
    hr = D3D10CreateDeviceAndSwapChain1(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0,
            supported_feature_level, D3D10_1_SDK_VERSION, &swapchain_desc, &swapchain, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    ok(!swapchain, "Got unexpected swapchain pointer %p.\n", swapchain);

    swapchain_desc.OutputWindow = NULL;
    hr = D3D10CreateDeviceAndSwapChain1(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0,
            supported_feature_level, D3D10_1_SDK_VERSION, &swapchain_desc, NULL, &device);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    refcount = ID3D10Device1_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);

    swapchain = (IDXGISwapChain *)0xdeadbeef;
    device = (ID3D10Device1 *)0xdeadbeef;
    swapchain_desc.OutputWindow = NULL;
    hr = D3D10CreateDeviceAndSwapChain1(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0,
            supported_feature_level, D3D10_1_SDK_VERSION, &swapchain_desc, &swapchain, &device);
    ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);
    ok(!swapchain, "Got unexpected swapchain pointer %p.\n", swapchain);
    ok(!device, "Got unexpected device pointer %p.\n", device);

    swapchain = (IDXGISwapChain *)0xdeadbeef;
    device = (ID3D10Device1 *)0xdeadbeef;
    swapchain_desc.OutputWindow = window;
    swapchain_desc.BufferDesc.Format = DXGI_FORMAT_BC5_UNORM;
    hr = D3D10CreateDeviceAndSwapChain1(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0,
            supported_feature_level, D3D10_1_SDK_VERSION, &swapchain_desc, &swapchain, &device);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    ok(!swapchain, "Got unexpected swapchain pointer %p.\n", swapchain);
    ok(!device, "Got unexpected device pointer %p.\n", device);

    DestroyWindow(window);
}

static void test_device_interfaces(void)
{
    IDXGIAdapter *dxgi_adapter;
    IDXGIDevice *dxgi_device;
    ID3D10Device1 *device;
    IUnknown *iface;
    ULONG refcount;
    unsigned int i;
    HRESULT hr;

    for (i = 0; i < ARRAY_SIZE(d3d10_feature_levels); ++i)
    {
        struct device_desc device_desc;

        device_desc.feature_level = d3d10_feature_levels[i];
        device_desc.flags = 0;

        if (!(device = create_device(&device_desc)))
        {
            skip("Failed to create device for feature level %#x.\n", d3d10_feature_levels[i]);
            continue;
        }

        check_interface(device, &IID_IUnknown, TRUE, FALSE);
        check_interface(device, &IID_IDXGIObject, TRUE, FALSE);
        check_interface(device, &IID_IDXGIDevice, TRUE, FALSE);
        check_interface(device, &IID_IDXGIDevice1, TRUE, FALSE);
        check_interface(device, &IID_ID3D10Multithread, TRUE, TRUE); /* Not available on all Windows versions. */
        check_interface(device, &IID_ID3D10Device, TRUE, FALSE);
        check_interface(device, &IID_ID3D10InfoQueue, FALSE, FALSE); /* Non-debug mode. */
        check_interface(device, &IID_ID3D11Device, TRUE, TRUE); /* Not available on all Windows versions. */

        hr = ID3D10Device1_QueryInterface(device, &IID_IDXGIDevice, (void **)&dxgi_device);
        ok(SUCCEEDED(hr), "Device should implement IDXGIDevice.\n");
        hr = IDXGIDevice_GetParent(dxgi_device, &IID_IDXGIAdapter, (void **)&dxgi_adapter);
        ok(SUCCEEDED(hr), "Device parent should implement IDXGIAdapter.\n");
        hr = IDXGIAdapter_GetParent(dxgi_adapter, &IID_IDXGIFactory, (void **)&iface);
        ok(SUCCEEDED(hr), "Adapter parent should implement IDXGIFactory.\n");
        IUnknown_Release(iface);
        IDXGIAdapter_Release(dxgi_adapter);
        hr = IDXGIDevice_GetParent(dxgi_device, &IID_IDXGIAdapter1, (void **)&dxgi_adapter);
        ok(SUCCEEDED(hr), "Device parent should implement IDXGIAdapter1.\n");
        hr = IDXGIAdapter_GetParent(dxgi_adapter, &IID_IDXGIFactory1, (void **)&iface);
        ok(hr == E_NOINTERFACE, "Adapter parent should not implement IDXGIFactory1.\n");
        IDXGIAdapter_Release(dxgi_adapter);
        IDXGIDevice_Release(dxgi_device);

        refcount = ID3D10Device1_Release(device);
        ok(!refcount, "Device has %lu references left.\n", refcount);
    }

    for (i = 0; i < ARRAY_SIZE(d3d10_feature_levels); ++i)
    {
        struct device_desc device_desc;

        device_desc.feature_level = d3d10_feature_levels[i];
        device_desc.flags = D3D10_CREATE_DEVICE_DEBUG;
        if (!(device = create_device(&device_desc)))
        {
            skip("Failed to create device for feature level %#x.\n", d3d10_feature_levels[i]);
            continue;
        }

        todo_wine
        check_interface(device, &IID_ID3D10InfoQueue, TRUE, FALSE);

        refcount = ID3D10Device1_Release(device);
        ok(!refcount, "Device has %lu references left.\n", refcount);
    }
}

static void test_create_shader_resource_view(void)
{
    D3D10_SHADER_RESOURCE_VIEW_DESC1 srv_desc;
    D3D10_TEXTURE2D_DESC texture_desc;
    ULONG refcount, expected_refcount;
    ID3D10ShaderResourceView1 *srview;
    D3D10_BUFFER_DESC buffer_desc;
    ID3D10Texture2D *texture;
    ID3D10Device *tmp_device;
    ID3D10Device1 *device;
    ID3D10Buffer *buffer;
    HRESULT hr;

    if (!(device = create_device(NULL)))
    {
        skip("Failed to create device.\n");
        return;
    }

    buffer_desc.ByteWidth = 1024;
    buffer_desc.Usage = D3D10_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
    buffer_desc.CPUAccessFlags = 0;
    buffer_desc.MiscFlags = 0;

    hr = ID3D10Device1_CreateBuffer(device, &buffer_desc, NULL, &buffer);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID3D10Device1_CreateShaderResourceView1(device, (ID3D10Resource *)buffer, NULL, &srview);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    srv_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    srv_desc.ViewDimension = D3D10_1_SRV_DIMENSION_BUFFER;
    srv_desc.Buffer.ElementOffset = 0;
    srv_desc.Buffer.ElementWidth = 64;

    hr = ID3D10Device1_CreateShaderResourceView1(device, NULL, &srv_desc, &srview);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device1_CreateShaderResourceView1(device, (ID3D10Resource *)buffer, &srv_desc, &srview);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %lu, expected >= %lu.\n", refcount, expected_refcount);
    tmp_device = NULL;
    expected_refcount = refcount + 1;
    ID3D10ShaderResourceView1_GetDevice(srview, &tmp_device);
    ok(tmp_device == (ID3D10Device *)device, "Got unexpected device %p, expected %p.\n", tmp_device, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %lu, expected %lu.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp_device);

    check_interface(srview, &IID_ID3D10ShaderResourceView, TRUE, FALSE);
    /* Not available on all Windows versions. */
    check_interface(srview, &IID_ID3D11ShaderResourceView, TRUE, TRUE);

    ID3D10ShaderResourceView1_Release(srview);
    ID3D10Buffer_Release(buffer);

    /* Without D3D10_BIND_SHADER_RESOURCE. */
    buffer_desc.ByteWidth = 1024;
    buffer_desc.Usage = D3D10_USAGE_DEFAULT;
    buffer_desc.BindFlags = 0;
    buffer_desc.CPUAccessFlags = 0;
    buffer_desc.MiscFlags = 0;

    hr = ID3D10Device1_CreateBuffer(device, &buffer_desc, NULL, &buffer);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID3D10Device1_CreateShaderResourceView1(device, (ID3D10Resource *)buffer, &srv_desc, &srview);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    ID3D10Buffer_Release(buffer);

    texture_desc.Width = 512;
    texture_desc.Height = 512;
    texture_desc.MipLevels = 0;
    texture_desc.ArraySize = 1;
    texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture_desc.SampleDesc.Count = 1;
    texture_desc.SampleDesc.Quality = 0;
    texture_desc.Usage = D3D10_USAGE_DEFAULT;
    texture_desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
    texture_desc.CPUAccessFlags = 0;
    texture_desc.MiscFlags = 0;

    hr = ID3D10Device1_CreateTexture2D(device, &texture_desc, NULL, &texture);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID3D10Device1_CreateShaderResourceView1(device, (ID3D10Resource *)texture, NULL, &srview);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10ShaderResourceView1_GetDesc1(srview, &srv_desc);
    ok(srv_desc.Format == texture_desc.Format, "Got unexpected format %#x.\n", srv_desc.Format);
    ok(srv_desc.ViewDimension == D3D10_1_SRV_DIMENSION_TEXTURE2D,
            "Got unexpected view dimension %#x.\n", srv_desc.ViewDimension);
    ok(srv_desc.Texture2D.MostDetailedMip == 0, "Got unexpected MostDetailedMip %u.\n",
            srv_desc.Texture2D.MostDetailedMip);
    ok(srv_desc.Texture2D.MipLevels == 10, "Got unexpected MipLevels %u.\n", srv_desc.Texture2D.MipLevels);

    check_interface(srview, &IID_ID3D10ShaderResourceView, TRUE, FALSE);
    /* Not available on all Windows versions. */
    check_interface(srview, &IID_ID3D11ShaderResourceView, TRUE, TRUE);

    ID3D10ShaderResourceView1_Release(srview);
    ID3D10Texture2D_Release(texture);

    refcount = ID3D10Device1_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

static void test_create_blend_state(void)
{
    static const D3D10_BLEND_DESC1 desc_conversion_tests[] =
    {
        {
            FALSE, FALSE,
            {
                {
                    FALSE, D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD,
                    D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD
                },
            },
        },
        {
            FALSE, TRUE,
            {
                {
                    TRUE, D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD,
                    D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD, D3D10_COLOR_WRITE_ENABLE_ALL
                },
                {
                    FALSE, D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD,
                    D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD, D3D10_COLOR_WRITE_ENABLE_RED
                },
                {
                    TRUE, D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD,
                    D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD, D3D10_COLOR_WRITE_ENABLE_ALL
                },
                {
                    FALSE, D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD,
                    D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD, D3D10_COLOR_WRITE_ENABLE_GREEN
                },
                {
                    TRUE, D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD,
                    D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD, D3D10_COLOR_WRITE_ENABLE_ALL
                },
                {
                    TRUE, D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD,
                    D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD, D3D10_COLOR_WRITE_ENABLE_ALL
                },
                {
                    TRUE, D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD,
                    D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD, D3D10_COLOR_WRITE_ENABLE_ALL
                },
                {
                    TRUE, D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD,
                    D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD, D3D10_COLOR_WRITE_ENABLE_ALL
                },
            },
        },
        {
            FALSE, TRUE,
            {
                {
                    TRUE, D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD,
                    D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD, D3D10_COLOR_WRITE_ENABLE_ALL
                },
                {
                    TRUE, D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_SUBTRACT,
                    D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD, D3D10_COLOR_WRITE_ENABLE_ALL
                },
                {
                    TRUE, D3D10_BLEND_ZERO, D3D10_BLEND_ONE, D3D10_BLEND_OP_ADD,
                    D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD, D3D10_COLOR_WRITE_ENABLE_ALL
                },
                {
                    TRUE, D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD,
                    D3D10_BLEND_ZERO, D3D10_BLEND_ONE, D3D10_BLEND_OP_ADD, D3D10_COLOR_WRITE_ENABLE_ALL
                },
                {
                    TRUE, D3D10_BLEND_ONE, D3D10_BLEND_ONE, D3D10_BLEND_OP_MAX,
                    D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD, D3D10_COLOR_WRITE_ENABLE_ALL
                },
                {
                    TRUE, D3D10_BLEND_ONE, D3D10_BLEND_ONE, D3D10_BLEND_OP_MIN,
                    D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD, D3D10_COLOR_WRITE_ENABLE_ALL
                },
                {
                    FALSE, D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD,
                    D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD, D3D10_COLOR_WRITE_ENABLE_ALL
                },
                {
                    FALSE, D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD,
                    D3D10_BLEND_ONE, D3D10_BLEND_ZERO, D3D10_BLEND_OP_ADD, D3D10_COLOR_WRITE_ENABLE_ALL
                },
            },
        },
    };

    ID3D10BlendState1 *blend_state1, *blend_state2;
    D3D10_BLEND_DESC1 desc, obtained_desc;
    ID3D10BlendState *d3d10_blend_state;
    D3D10_BLEND_DESC d3d10_blend_desc;
    ULONG refcount, expected_refcount;
    ID3D10Device1 *device;
    ID3D10Device *tmp;
    unsigned int i, j;
    HRESULT hr;

    if (!(device = create_device(NULL)))
    {
        skip("Failed to create device.\n");
        return;
    }

    hr = ID3D10Device1_CreateBlendState1(device, NULL, &blend_state1);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    memset(&desc, 0, sizeof(desc));
    desc.AlphaToCoverageEnable = FALSE;
    desc.IndependentBlendEnable = FALSE;
    desc.RenderTarget[0].BlendEnable = FALSE;
    desc.RenderTarget[0].SrcBlend = D3D10_BLEND_ONE;
    desc.RenderTarget[0].DestBlend = D3D10_BLEND_ZERO;
    desc.RenderTarget[0].BlendOp = D3D10_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlendAlpha = D3D10_BLEND_ONE;
    desc.RenderTarget[0].DestBlendAlpha = D3D10_BLEND_ZERO;
    desc.RenderTarget[0].BlendOpAlpha = D3D10_BLEND_OP_ADD;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D10_COLOR_WRITE_ENABLE_ALL;

    expected_refcount = get_refcount((IUnknown *)device) + 1;
    hr = ID3D10Device1_CreateBlendState1(device, &desc, &blend_state1);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3D10Device1_CreateBlendState1(device, &desc, &blend_state2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(blend_state1 == blend_state2, "Got different blend state objects.\n");
    refcount = get_refcount((IUnknown *)device);
    ok(refcount >= expected_refcount, "Got unexpected refcount %lu, expected >= %lu.\n", refcount, expected_refcount);
    tmp = NULL;
    expected_refcount = refcount + 1;
    ID3D10BlendState1_GetDevice(blend_state1, &tmp);
    ok(tmp == (ID3D10Device *)device, "Got unexpected device %p, expected %p.\n", tmp, device);
    refcount = get_refcount((IUnknown *)device);
    ok(refcount == expected_refcount, "Got unexpected refcount %lu, expected %lu.\n", refcount, expected_refcount);
    ID3D10Device_Release(tmp);

    ID3D10BlendState1_GetDesc1(blend_state1, &obtained_desc);
    ok(obtained_desc.AlphaToCoverageEnable == FALSE, "Got unexpected alpha to coverage enable %#x.\n",
            obtained_desc.AlphaToCoverageEnable);
    ok(obtained_desc.IndependentBlendEnable == FALSE, "Got unexpected independent blend enable %#x.\n",
            obtained_desc.IndependentBlendEnable);
    for (i = 0; i < D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
    {
        ok(obtained_desc.RenderTarget[i].BlendEnable == FALSE,
                "Got unexpected blend enable %#x for render target %u.\n",
                obtained_desc.RenderTarget[i].BlendEnable, i);
        ok(obtained_desc.RenderTarget[i].SrcBlend == D3D10_BLEND_ONE,
                "Got unexpected src blend %u for render target %u.\n",
                obtained_desc.RenderTarget[i].SrcBlend, i);
        ok(obtained_desc.RenderTarget[i].DestBlend == D3D10_BLEND_ZERO,
                "Got unexpected dest blend %u for render target %u.\n",
                obtained_desc.RenderTarget[i].DestBlend, i);
        ok(obtained_desc.RenderTarget[i].BlendOp == D3D10_BLEND_OP_ADD,
                "Got unexpected blend op %u for render target %u.\n",
                obtained_desc.RenderTarget[i].BlendOp, i);
        ok(obtained_desc.RenderTarget[i].SrcBlendAlpha == D3D10_BLEND_ONE,
                "Got unexpected src blend alpha %u for render target %u.\n",
                obtained_desc.RenderTarget[i].SrcBlendAlpha, i);
        ok(obtained_desc.RenderTarget[i].DestBlendAlpha == D3D10_BLEND_ZERO,
                "Got unexpected dest blend alpha %u for render target %u.\n",
                obtained_desc.RenderTarget[i].DestBlendAlpha, i);
        ok(obtained_desc.RenderTarget[i].BlendOpAlpha == D3D10_BLEND_OP_ADD,
                "Got unexpected blend op alpha %u for render target %u.\n",
                obtained_desc.RenderTarget[i].BlendOpAlpha, i);
        ok(obtained_desc.RenderTarget[i].RenderTargetWriteMask == D3D10_COLOR_WRITE_ENABLE_ALL,
                "Got unexpected render target write mask %#x for render target %u.\n",
                obtained_desc.RenderTarget[0].RenderTargetWriteMask, i);
    }

    check_interface(blend_state1, &IID_ID3D10BlendState, TRUE, FALSE);
    /* Not available on all Windows versions. */
    check_interface(blend_state1, &IID_ID3D11BlendState, TRUE, TRUE);

    refcount = ID3D10BlendState1_Release(blend_state1);
    ok(refcount == 1, "Got unexpected refcount %lu.\n", refcount);
    refcount = ID3D10BlendState1_Release(blend_state2);
    ok(!refcount, "Blend state has %lu references left.\n", refcount);

    for (i = 0; i < ARRAY_SIZE(desc_conversion_tests); ++i)
    {
        const D3D10_BLEND_DESC1 *current_desc = &desc_conversion_tests[i];

        hr = ID3D10Device1_CreateBlendState1(device, current_desc, &blend_state1);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = ID3D10BlendState1_QueryInterface(blend_state1, &IID_ID3D10BlendState, (void **)&d3d10_blend_state);
        ok(SUCCEEDED(hr), "Blend state should implement ID3D10BlendState.\n");

        ID3D10BlendState_GetDesc(d3d10_blend_state, &d3d10_blend_desc);
        ok(d3d10_blend_desc.AlphaToCoverageEnable == current_desc->AlphaToCoverageEnable,
                "Got unexpected alpha to coverage enable %#x for test %u.\n",
                d3d10_blend_desc.AlphaToCoverageEnable, i);
        ok(d3d10_blend_desc.SrcBlend == current_desc->RenderTarget[0].SrcBlend,
                "Got unexpected src blend %u for test %u.\n", d3d10_blend_desc.SrcBlend, i);
        ok(d3d10_blend_desc.DestBlend == current_desc->RenderTarget[0].DestBlend,
                "Got unexpected dest blend %u for test %u.\n", d3d10_blend_desc.DestBlend, i);
        ok(d3d10_blend_desc.BlendOp == current_desc->RenderTarget[0].BlendOp,
                "Got unexpected blend op %u for test %u.\n", d3d10_blend_desc.BlendOp, i);
        ok(d3d10_blend_desc.SrcBlendAlpha == current_desc->RenderTarget[0].SrcBlendAlpha,
                "Got unexpected src blend alpha %u for test %u.\n", d3d10_blend_desc.SrcBlendAlpha, i);
        ok(d3d10_blend_desc.DestBlendAlpha == current_desc->RenderTarget[0].DestBlendAlpha,
                "Got unexpected dest blend alpha %u for test %u.\n", d3d10_blend_desc.DestBlendAlpha, i);
        ok(d3d10_blend_desc.BlendOpAlpha == current_desc->RenderTarget[0].BlendOpAlpha,
                "Got unexpected blend op alpha %u for test %u.\n", d3d10_blend_desc.BlendOpAlpha, i);
        for (j = 0; j < D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT; j++)
        {
            unsigned int k = current_desc->IndependentBlendEnable ? j : 0;
            ok(d3d10_blend_desc.BlendEnable[j] == current_desc->RenderTarget[k].BlendEnable,
                    "Got unexpected blend enable %#x for test %u, render target %u.\n",
                    d3d10_blend_desc.BlendEnable[j], i, j);
            ok(d3d10_blend_desc.RenderTargetWriteMask[j] == current_desc->RenderTarget[k].RenderTargetWriteMask,
                    "Got unexpected render target write mask %#x for test %u, render target %u.\n",
                    d3d10_blend_desc.RenderTargetWriteMask[j], i, j);
        }

        ID3D10BlendState_Release(d3d10_blend_state);

        refcount = ID3D10BlendState1_Release(blend_state1);
        ok(!refcount, "Got unexpected refcount %lu.\n", refcount);
    }

    refcount = ID3D10Device1_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

static void test_getdc(void)
{
    struct device_desc device_desc;
    D3D10_TEXTURE2D_DESC desc;
    ID3D10Texture2D *texture;
    IDXGISurface1 *surface1;
    ID3D10Device1 *device;
    ULONG refcount;
    HRESULT hr;
    HDC dc;

    device_desc.feature_level = D3D10_FEATURE_LEVEL_10_1;
    device_desc.flags = D3D10_CREATE_DEVICE_BGRA_SUPPORT;
    if (!(device = create_device(&device_desc)))
    {
        skip("Failed to create device.\n");
        return;
    }

    /* Without D3D10_RESOURCE_MISC_GDI_COMPATIBLE. */
    desc.Width = 512;
    desc.Height = 512;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D10_USAGE_DEFAULT;
    desc.BindFlags = D3D10_BIND_RENDER_TARGET;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    hr = ID3D10Device1_CreateTexture2D(device, &desc, NULL, &texture);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID3D10Texture2D_QueryInterface(texture, &IID_IDXGISurface1, (void**)&surface1);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGISurface1_GetDC(surface1, FALSE, &dc);
    todo_wine ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);

    IDXGISurface1_Release(surface1);
    ID3D10Texture2D_Release(texture);

    desc.MiscFlags = D3D10_RESOURCE_MISC_GDI_COMPATIBLE;
    hr = ID3D10Device1_CreateTexture2D(device, &desc, NULL, &texture);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID3D10Texture2D_QueryInterface(texture, &IID_IDXGISurface1, (void**)&surface1);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGISurface1_ReleaseDC(surface1, NULL);
    todo_wine ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGISurface1_GetDC(surface1, FALSE, &dc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* One more time. */
    dc = (HDC)0xdeadbeef;
    hr = IDXGISurface1_GetDC(surface1, FALSE, &dc);
    todo_wine ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);
    ok(dc == (HDC)0xdeadbeef, "Got unexpected dc %p.\n", dc);

    hr = IDXGISurface1_ReleaseDC(surface1, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = IDXGISurface1_ReleaseDC(surface1, NULL);
    todo_wine ok(hr == DXGI_ERROR_INVALID_CALL, "Got unexpected hr %#lx.\n", hr);

    IDXGISurface1_Release(surface1);
    ID3D10Texture2D_Release(texture);

    refcount = ID3D10Device1_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

static inline HRESULT create_effect(DWORD *data, UINT flags, ID3D10Device1 *device,
        ID3D10EffectPool *effect_pool, ID3D10Effect **effect)
{
    /*
     * Don't use sizeof(data), use data[6] as size,
     * because the DWORD data[] has only complete DWORDs and
     * so it could happen that there are padded bytes at the end.
     *
     * The fx size (data[6]) could be up to 3 BYTEs smaller
     * than the sizeof(data).
     */
    return D3D10CreateEffectFromMemory(data, data[6], flags, (ID3D10Device *)device, effect_pool, effect);
}

#if 0
BlendState blend_state
{
    blendenable[0] = true;
    blendenable[1] = true;
    blendenable[2] = true;
    blendenable[3] = true;
    blendenable[4] = true;
    blendenable[5] = true;
    blendenable[6] = true;
    blendenable[7] = true;
    srcblend = one;
    srcblend[0] = zero;
};

BlendState blend_state2
{
    blendenable = true;
    srcblend = src_color;
};

BlendState default_blend_state {};
#endif
static DWORD fx_4_1_test_blend_state[] =
{
    0x43425844, 0x93b3fe48, 0xee555ce6, 0xc07d00df, 0x616889d5, 0x00000001, 0x000003d4, 0x00000001,
    0x00000024, 0x30315846, 0x000003a8, 0xfeff1011, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000184, 0x00000000, 0x00000000, 0x00000000, 0x00000003,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x6e656c42,
    0x61745364, 0x04006574, 0x02000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x02000000,
    0x62000000, 0x646e656c, 0x6174735f, 0x01006574, 0x04000000, 0x01000000, 0x01000000, 0x04000000,
    0x01000000, 0x01000000, 0x04000000, 0x01000000, 0x01000000, 0x04000000, 0x01000000, 0x01000000,
    0x04000000, 0x01000000, 0x01000000, 0x04000000, 0x01000000, 0x01000000, 0x04000000, 0x01000000,
    0x01000000, 0x04000000, 0x01000000, 0x01000000, 0x02000000, 0x02000000, 0x01000000, 0x02000000,
    0x02000000, 0x01000000, 0x02000000, 0x02000000, 0x01000000, 0x02000000, 0x02000000, 0x01000000,
    0x02000000, 0x02000000, 0x01000000, 0x02000000, 0x02000000, 0x01000000, 0x02000000, 0x02000000,
    0x01000000, 0x02000000, 0x01000000, 0x62000000, 0x646e656c, 0x6174735f, 0x00326574, 0x00000001,
    0x00000004, 0x00000001, 0x00000001, 0x00000002, 0x00000003, 0x00000001, 0x00000002, 0x00000003,
    0x00000001, 0x00000002, 0x00000003, 0x00000001, 0x00000002, 0x00000003, 0x00000001, 0x00000002,
    0x00000003, 0x00000001, 0x00000002, 0x00000003, 0x00000001, 0x00000002, 0x00000003, 0x00000001,
    0x00000002, 0x00000003, 0x61666564, 0x5f746c75, 0x6e656c62, 0x74735f64, 0x00657461, 0x0000002b,
    0x0000000f, 0x00000000, 0xffffffff, 0x00000010, 0x00000025, 0x00000000, 0x00000001, 0x00000037,
    0x00000025, 0x00000001, 0x00000001, 0x00000043, 0x00000025, 0x00000002, 0x00000001, 0x0000004f,
    0x00000025, 0x00000003, 0x00000001, 0x0000005b, 0x00000025, 0x00000004, 0x00000001, 0x00000067,
    0x00000025, 0x00000005, 0x00000001, 0x00000073, 0x00000025, 0x00000006, 0x00000001, 0x0000007f,
    0x00000025, 0x00000007, 0x00000001, 0x0000008b, 0x00000026, 0x00000001, 0x00000001, 0x00000097,
    0x00000026, 0x00000002, 0x00000001, 0x000000a3, 0x00000026, 0x00000003, 0x00000001, 0x000000af,
    0x00000026, 0x00000004, 0x00000001, 0x000000bb, 0x00000026, 0x00000005, 0x00000001, 0x000000c7,
    0x00000026, 0x00000006, 0x00000001, 0x000000d3, 0x00000026, 0x00000007, 0x00000001, 0x000000df,
    0x00000026, 0x00000000, 0x00000001, 0x000000eb, 0x00000000, 0x000000f7, 0x0000000f, 0x00000000,
    0xffffffff, 0x00000009, 0x00000025, 0x00000000, 0x00000001, 0x00000104, 0x00000026, 0x00000000,
    0x00000001, 0x00000110, 0x00000026, 0x00000001, 0x00000001, 0x0000011c, 0x00000026, 0x00000002,
    0x00000001, 0x00000128, 0x00000026, 0x00000003, 0x00000001, 0x00000134, 0x00000026, 0x00000004,
    0x00000001, 0x00000140, 0x00000026, 0x00000005, 0x00000001, 0x0000014c, 0x00000026, 0x00000006,
    0x00000001, 0x00000158, 0x00000026, 0x00000007, 0x00000001, 0x00000164, 0x00000000, 0x00000170,
    0x0000000f, 0x00000000, 0xffffffff, 0x00000000, 0x00000000,
};

static void test_fx_4_1_blend_state(void)
{
    ID3D10EffectBlendVariable *blend;
    struct device_desc device_desc;
    ID3D10EffectVariable *v;
    D3D10_BLEND_DESC1 desc1;
    ID3D10BlendState1 *bs1;
    ID3D10Device1 *device;
    D3D10_BLEND_DESC desc;
    ID3D10Effect *effect;
    ID3D10BlendState *bs;
    ULONG refcount;
    unsigned int i;
    HRESULT hr;

    if (!(device = create_device(NULL)))
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    hr = create_effect(fx_4_1_test_blend_state, 0, device, NULL, &effect);
    todo_wine
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    if (FAILED(hr))
    {
        ID3D10Device1_Release(device);
        return;
    }

    v = effect->lpVtbl->GetVariableByName(effect, "blend_state");
    ok(v->lpVtbl->IsValid(v), "Invalid variable.\n");
    blend = v->lpVtbl->AsBlend(v);
    ok(blend->lpVtbl->IsValid(blend), "Invalid variable.\n");

    memset(&desc, 0, sizeof(desc));
    hr = blend->lpVtbl->GetBackingStore(blend, 0, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!desc.AlphaToCoverageEnable, "Unexpected value %#x.\n", desc1.AlphaToCoverageEnable);
    for (i = 0; i < ARRAY_SIZE(desc.BlendEnable); ++i)
        ok(desc.BlendEnable[i], "Unexpected value[%u] %#x.\n", i, desc.BlendEnable[i]);
    ok(desc.SrcBlend == D3D10_BLEND_ZERO, "Unexpected value %u.\n", desc.SrcBlend);
    ok(desc.DestBlend == D3D10_BLEND_ZERO, "Unexpected value %u.\n", desc.DestBlend);
    ok(desc.BlendOp == D3D10_BLEND_OP_ADD, "Unexpected value %u.\n", desc.BlendOp);
    ok(desc.SrcBlendAlpha == D3D10_BLEND_ONE, "Unexpected value %u.\n", desc.SrcBlendAlpha);
    ok(desc.DestBlendAlpha == D3D10_BLEND_ZERO, "Unexpected value %u.\n", desc.DestBlendAlpha);
    ok(desc.BlendOpAlpha == D3D10_BLEND_OP_ADD, "Unexpected value %u.\n", desc.BlendOpAlpha);
    for (i = 0; i < ARRAY_SIZE(desc.RenderTargetWriteMask); ++i)
        ok(desc.RenderTargetWriteMask[i] == D3D10_COLOR_WRITE_ENABLE_ALL, "Unexpected value[%u] %#x.\n",
                i, desc.RenderTargetWriteMask[i]);

    hr = blend->lpVtbl->GetBlendState(blend, 0, &bs);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10BlendState_GetDesc(bs, &desc);
    ok(!desc.AlphaToCoverageEnable, "Unexpected value %#x.\n", desc1.AlphaToCoverageEnable);
    for (i = 0; i < ARRAY_SIZE(desc.BlendEnable); ++i)
        ok(desc.BlendEnable[i], "Unexpected value[%u] %#x.\n", i, desc.BlendEnable[i]);
    ok(desc.SrcBlend == D3D10_BLEND_ZERO, "Unexpected value %u.\n", desc.SrcBlend);
    ok(desc.DestBlend == D3D10_BLEND_ZERO, "Unexpected value %u.\n", desc.DestBlend);
    ok(desc.BlendOp == D3D10_BLEND_OP_ADD, "Unexpected value %u.\n", desc.BlendOp);
    ok(desc.SrcBlendAlpha == D3D10_BLEND_ONE, "Unexpected value %u.\n", desc.SrcBlendAlpha);
    ok(desc.DestBlendAlpha == D3D10_BLEND_ZERO, "Unexpected value %u.\n", desc.DestBlendAlpha);
    ok(desc.BlendOpAlpha == D3D10_BLEND_OP_ADD, "Unexpected value %u.\n", desc.BlendOpAlpha);
    for (i = 0; i < ARRAY_SIZE(desc.RenderTargetWriteMask); ++i)
        ok(desc.RenderTargetWriteMask[i] == D3D10_COLOR_WRITE_ENABLE_ALL, "Unexpected value[%u] %#x.\n",
                i, desc.RenderTargetWriteMask[i]);

    hr = ID3D10BlendState_QueryInterface(bs, &IID_ID3D10BlendState1, (void **)&bs1);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10BlendState1_GetDesc1(bs1, &desc1);
    ok(!desc1.AlphaToCoverageEnable, "Unexpected value %#x.\n", desc1.AlphaToCoverageEnable);
    ok(desc1.IndependentBlendEnable, "Unexpected value %#x.\n", desc1.IndependentBlendEnable);
    for (i = 0; i < ARRAY_SIZE(desc1.RenderTarget); ++i)
    {
        const D3D10_RENDER_TARGET_BLEND_DESC1 *p = &desc1.RenderTarget[i];

        winetest_push_context("Test %u", i);

        ok(p->BlendEnable, "Unexpected value %#x.\n", p->BlendEnable);
        ok(p->SrcBlend == (i == 0 ? D3D10_BLEND_ZERO : D3D10_BLEND_ONE), "Unexpected value %u.\n", p->SrcBlend);
        ok(p->DestBlend == D3D10_BLEND_ZERO, "Unexpected value %u.\n", p->DestBlend);
        ok(p->BlendOp == D3D10_BLEND_OP_ADD, "Unexpected value %u.\n", p->BlendOp);
        ok(p->SrcBlendAlpha == D3D10_BLEND_ONE, "Unexpected value %u.\n", p->SrcBlendAlpha);
        ok(p->DestBlendAlpha == D3D10_BLEND_ZERO, "Unexpected value %u.\n", p->DestBlendAlpha);
        ok(p->BlendOpAlpha == D3D10_BLEND_OP_ADD, "Unexpected value %u.\n", p->BlendOpAlpha);
        ok(p->RenderTargetWriteMask == D3D10_COLOR_WRITE_ENABLE_ALL, "Unexpected value %#x.\n", p->RenderTargetWriteMask);

        winetest_pop_context();
    }
    ID3D10BlendState1_Release(bs1);
    ID3D10BlendState_Release(bs);

    /* Default state. */
    v = effect->lpVtbl->GetVariableByName(effect, "default_blend_state");
    ok(v->lpVtbl->IsValid(v), "Invalid variable.\n");
    blend = v->lpVtbl->AsBlend(v);
    ok(blend->lpVtbl->IsValid(blend), "Invalid variable.\n");

    memset(&desc, 0, sizeof(desc));
    hr = blend->lpVtbl->GetBackingStore(blend, 0, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!desc.AlphaToCoverageEnable, "Unexpected value %#x.\n", desc1.AlphaToCoverageEnable);
    for (i = 0; i < ARRAY_SIZE(desc.BlendEnable); ++i)
        ok(!desc.BlendEnable[i], "Unexpected value[%u] %#x.\n", i, desc.BlendEnable[i]);
    ok(desc.SrcBlend == D3D10_BLEND_ONE, "Unexpected value %u.\n", desc.SrcBlend);
    ok(desc.DestBlend == D3D10_BLEND_ZERO, "Unexpected value %u.\n", desc.DestBlend);
    ok(desc.BlendOp == D3D10_BLEND_OP_ADD, "Unexpected value %u.\n", desc.BlendOp);
    ok(desc.SrcBlendAlpha == D3D10_BLEND_ONE, "Unexpected value %u.\n", desc.SrcBlendAlpha);
    ok(desc.DestBlendAlpha == D3D10_BLEND_ZERO, "Unexpected value %u.\n", desc.DestBlendAlpha);
    ok(desc.BlendOpAlpha == D3D10_BLEND_OP_ADD, "Unexpected value %u.\n", desc.BlendOpAlpha);
    for (i = 0; i < ARRAY_SIZE(desc.RenderTargetWriteMask); ++i)
        ok(desc.RenderTargetWriteMask[i] == D3D10_COLOR_WRITE_ENABLE_ALL, "Unexpected value[%u] %#x.\n",
                i, desc.RenderTargetWriteMask[i]);

    hr = blend->lpVtbl->GetBlendState(blend, 0, &bs);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3D10BlendState_QueryInterface(bs, &IID_ID3D10BlendState1, (void **)&bs1);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D10BlendState1_GetDesc1(bs1, &desc1);
    ok(!desc1.AlphaToCoverageEnable, "Unexpected value %#x.\n", desc1.AlphaToCoverageEnable);
    ok(desc1.IndependentBlendEnable, "Unexpected value %#x.\n", desc1.IndependentBlendEnable);
    for (i = 0; i < ARRAY_SIZE(desc1.RenderTarget); ++i)
    {
        const D3D10_RENDER_TARGET_BLEND_DESC1 *p = &desc1.RenderTarget[i];

        ok(!p->BlendEnable, "Unexpected value %#x.\n", p->BlendEnable);
        ok(p->SrcBlend == D3D10_BLEND_ONE, "Unexpected value %u.\n", p->SrcBlend);
        ok(p->DestBlend == D3D10_BLEND_ZERO, "Unexpected value %u.\n", p->DestBlend);
        ok(p->BlendOp == D3D10_BLEND_OP_ADD, "Unexpected value %u.\n", p->BlendOp);
        ok(p->SrcBlendAlpha == D3D10_BLEND_ONE, "Unexpected value %u.\n", p->SrcBlendAlpha);
        ok(p->DestBlendAlpha == D3D10_BLEND_ZERO, "Unexpected value %u.\n", p->DestBlendAlpha);
        ok(p->BlendOpAlpha == D3D10_BLEND_OP_ADD, "Unexpected value %u.\n", p->BlendOpAlpha);
        ok(p->RenderTargetWriteMask == D3D10_COLOR_WRITE_ENABLE_ALL, "Unexpected value %#x.\n", p->RenderTargetWriteMask);
    }
    ID3D10BlendState1_Release(bs1);
    ID3D10BlendState_Release(bs);

    refcount = effect->lpVtbl->Release(effect);
    ok(!refcount, "Unexpected refcount %lu.\n", refcount);

    refcount = ID3D10Device1_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);

    /* Using feature level 10.0 device. */
    device_desc.flags = 0;
    device_desc.feature_level = D3D10_FEATURE_LEVEL_10_0;

    device = create_device(&device_desc);
    ok(!!device, "Failed to create device.\n");

    hr = create_effect(fx_4_1_test_blend_state, 0, device, NULL, &effect);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    v = effect->lpVtbl->GetVariableByName(effect, "blend_state");
    ok(!v->lpVtbl->IsValid(v), "Unexpected variable.\n");

    v = effect->lpVtbl->GetVariableByName(effect, "blend_state2");
    ok(v->lpVtbl->IsValid(v), "Unexpected variable.\n");
    blend = v->lpVtbl->AsBlend(v);
    ok(blend->lpVtbl->IsValid(blend), "Invalid variable.\n");

    memset(&desc, 0, sizeof(desc));
    hr = blend->lpVtbl->GetBackingStore(blend, 0, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!desc.AlphaToCoverageEnable, "Unexpected value %#x.\n", desc.AlphaToCoverageEnable);
    ok(desc.BlendEnable[0], "Unexpected value %#x.\n", desc.BlendEnable[0]);
    for (i = 1; i < ARRAY_SIZE(desc.BlendEnable); ++i)
        ok(!desc.BlendEnable[i], "Unexpected value[%u] %#x.\n", i, desc.BlendEnable[i]);
    ok(desc.SrcBlend == D3D10_BLEND_SRC_COLOR, "Unexpected value %u.\n", desc.SrcBlend);
    ok(desc.DestBlend == D3D10_BLEND_ZERO, "Unexpected value %u.\n", desc.DestBlend);
    ok(desc.BlendOp == D3D10_BLEND_OP_ADD, "Unexpected value %u.\n", desc.BlendOp);
    ok(desc.SrcBlendAlpha == D3D10_BLEND_ONE, "Unexpected value %u.\n", desc.SrcBlendAlpha);
    ok(desc.DestBlendAlpha == D3D10_BLEND_ZERO, "Unexpected value %u.\n", desc.DestBlendAlpha);
    ok(desc.BlendOpAlpha == D3D10_BLEND_OP_ADD, "Unexpected value %u.\n", desc.BlendOpAlpha);
    for (i = 0; i < ARRAY_SIZE(desc.RenderTargetWriteMask); ++i)
        ok(desc.RenderTargetWriteMask[i] == D3D10_COLOR_WRITE_ENABLE_ALL, "Unexpected value[%u] %#x.\n",
                i, desc.RenderTargetWriteMask[i]);

    /* Default state. */
    v = effect->lpVtbl->GetVariableByName(effect, "default_blend_state");
    ok(v->lpVtbl->IsValid(v), "Invalid variable.\n");
    blend = v->lpVtbl->AsBlend(v);
    ok(blend->lpVtbl->IsValid(blend), "Invalid variable.\n");

    memset(&desc, 0, sizeof(desc));
    hr = blend->lpVtbl->GetBackingStore(blend, 0, &desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!desc.AlphaToCoverageEnable, "Unexpected value %#x.\n", desc1.AlphaToCoverageEnable);
    for (i = 0; i < ARRAY_SIZE(desc.BlendEnable); ++i)
        ok(!desc.BlendEnable[i], "Unexpected value[%u] %#x.\n", i, desc.BlendEnable[i]);
    ok(desc.SrcBlend == D3D10_BLEND_ONE, "Unexpected value %u.\n", desc.SrcBlend);
    ok(desc.DestBlend == D3D10_BLEND_ZERO, "Unexpected value %u.\n", desc.DestBlend);
    ok(desc.BlendOp == D3D10_BLEND_OP_ADD, "Unexpected value %u.\n", desc.BlendOp);
    ok(desc.SrcBlendAlpha == D3D10_BLEND_ONE, "Unexpected value %u.\n", desc.SrcBlendAlpha);
    ok(desc.DestBlendAlpha == D3D10_BLEND_ZERO, "Unexpected value %u.\n", desc.DestBlendAlpha);
    ok(desc.BlendOpAlpha == D3D10_BLEND_OP_ADD, "Unexpected value %u.\n", desc.BlendOpAlpha);
    for (i = 0; i < ARRAY_SIZE(desc.RenderTargetWriteMask); ++i)
        ok(desc.RenderTargetWriteMask[i] == D3D10_COLOR_WRITE_ENABLE_ALL, "Unexpected value[%u] %#x.\n",
                i, desc.RenderTargetWriteMask[i]);

    hr = blend->lpVtbl->GetBlendState(blend, 0, &bs);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3D10BlendState_QueryInterface(bs, &IID_ID3D10BlendState1, (void **)&bs1);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D10BlendState1_Release(bs1);
    ID3D10BlendState_Release(bs);

    refcount = effect->lpVtbl->Release(effect);
    ok(!refcount, "Unexpected refcount %lu.\n", refcount);

    refcount = ID3D10Device1_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

static void test_shader_profiles(void)
{
    const char *profile;

    profile = D3D10GetVertexShaderProfile(NULL);
    ok(!strcmp(profile, "vs_4_0"), "Unexpected profile %s.\n", profile);

    profile = D3D10GetGeometryShaderProfile(NULL);
    ok(!strcmp(profile, "gs_4_0"), "Unexpected profile %s.\n", profile);

    profile = D3D10GetPixelShaderProfile(NULL);
    ok(!strcmp(profile, "ps_4_0"), "Unexpected profile %s.\n", profile);
}

static void test_compile_effect(void)
{
    char default_bs_source[] = "BlendState default_blend_state {};";
    char bs_source2[] =
            "BlendState blend_state\n"
            "{\n"
            "     srcblend[0] = zero;\n"
            "};";
    ID3D10Blob *blob;
    HRESULT hr;

    hr = D3D10CompileEffectFromMemory(default_bs_source, strlen(default_bs_source),
            NULL, NULL, NULL, 0, 0, &blob, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ID3D10Blob_Release(blob);

    /* Compilation fails due to 10.1 feature incompatibility with fx_4_0 profile. */
    hr = D3D10CompileEffectFromMemory(bs_source2, strlen(bs_source2), NULL, NULL, NULL,
            0, 0, &blob, NULL);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);
}

START_TEST(d3d10_1)
{
    test_create_device();
    test_device_interfaces();
    test_create_shader_resource_view();
    test_create_blend_state();
    test_getdc();
    test_fx_4_1_blend_state();
    test_shader_profiles();
    test_compile_effect();
}
