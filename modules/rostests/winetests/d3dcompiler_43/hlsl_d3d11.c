/*
 * Copyright (C) 2020 Zebediah Figura for CodeWeavers
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

#include <limits.h>
#include <math.h>

#define COBJMACROS
#include "d3dcompiler.h"
#include "d3d11.h"
#include "wine/test.h"

static HRESULT (WINAPI *pD3D11CreateDevice)(IDXGIAdapter *adapter, D3D_DRIVER_TYPE driver_type,
        HMODULE swrast, UINT flags, const D3D_FEATURE_LEVEL *feature_levels, UINT levels,
        UINT sdk_version, ID3D11Device **device_out, D3D_FEATURE_LEVEL *obtained_feature_level,
        ID3D11DeviceContext **immediate_context);

struct vec2
{
    float x, y;
};

struct vec4
{
    float x, y, z, w;
};

#define compile_shader(a, b) compile_shader_(__LINE__, a, b, 0)
#define compile_shader_flags(a, b, c) compile_shader_(__LINE__, a, b, c)
static ID3D10Blob *compile_shader_(unsigned int line, const char *source, const char *target, UINT flags)
{
    ID3D10Blob *blob = NULL, *errors = NULL;
    HRESULT hr;

    hr = D3DCompile(source, strlen(source), NULL, NULL, NULL, "main", target, flags, 0, &blob, &errors);
    ok_(__FILE__, line)(hr == S_OK, "Failed to compile shader, hr %#lx.\n", hr);
    if (errors)
    {
        if (winetest_debug > 1)
            trace_(__FILE__, line)("%s\n", (char *)ID3D10Blob_GetBufferPointer(errors));
        ID3D10Blob_Release(errors);
    }
    return blob;
}

static BOOL compare_uint(unsigned int x, unsigned int y, unsigned int max_diff)
{
    unsigned int diff = x > y ? x - y : y - x;

    return diff <= max_diff;
}

static BOOL compare_float(float f, float g, unsigned int ulps)
{
    int x = *(int *)&f;
    int y = *(int *)&g;

    if (x < 0)
        x = INT_MIN - x;
    if (y < 0)
        y = INT_MIN - y;

    return compare_uint(x, y, ulps);
}

static BOOL compare_vec4(const struct vec4 *vec, float x, float y, float z, float w, unsigned int ulps)
{
    return compare_float(vec->x, x, ulps)
            && compare_float(vec->y, y, ulps)
            && compare_float(vec->z, z, ulps)
            && compare_float(vec->w, w, ulps);
}

struct test_context
{
    ID3D11Device *device;
    HWND window;
    IDXGISwapChain *swapchain;
    ID3D11Texture2D *rt;
    ID3D11RenderTargetView *rtv;
    ID3D11DeviceContext *immediate_context;

    ID3D11InputLayout *input_layout;
    ID3D11VertexShader *vs;
    ID3D11Buffer *vb;
};

static ID3D11Device *create_device(void)
{
    static const D3D_FEATURE_LEVEL feature_level[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    ID3D11Device *device;

    if (SUCCEEDED(pD3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0,
            feature_level, ARRAY_SIZE(feature_level), D3D11_SDK_VERSION, &device, NULL, NULL)))
        return device;
    if (SUCCEEDED(pD3D11CreateDevice(NULL, D3D_DRIVER_TYPE_WARP, NULL, 0,
            feature_level, ARRAY_SIZE(feature_level), D3D11_SDK_VERSION, &device, NULL, NULL)))
        return device;
    if (SUCCEEDED(pD3D11CreateDevice(NULL, D3D_DRIVER_TYPE_REFERENCE, NULL, 0,
            feature_level, ARRAY_SIZE(feature_level), D3D11_SDK_VERSION, &device, NULL, NULL)))
        return device;

    return NULL;
}

static IDXGISwapChain *create_swapchain(ID3D11Device *device, HWND window)
{
    DXGI_SWAP_CHAIN_DESC dxgi_desc;
    IDXGISwapChain *swapchain;
    IDXGIDevice *dxgi_device;
    IDXGIAdapter *adapter;
    IDXGIFactory *factory;
    HRESULT hr;

    hr = ID3D11Device_QueryInterface(device, &IID_IDXGIDevice, (void **)&dxgi_device);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGIDevice_GetAdapter(dxgi_device, &adapter);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IDXGIDevice_Release(dxgi_device);
    hr = IDXGIAdapter_GetParent(adapter, &IID_IDXGIFactory, (void **)&factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IDXGIAdapter_Release(adapter);

    dxgi_desc.BufferDesc.Width = 640;
    dxgi_desc.BufferDesc.Height = 480;
    dxgi_desc.BufferDesc.RefreshRate.Numerator = 60;
    dxgi_desc.BufferDesc.RefreshRate.Denominator = 1;
    dxgi_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    dxgi_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    dxgi_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    dxgi_desc.SampleDesc.Count = 1;
    dxgi_desc.SampleDesc.Quality = 0;
    dxgi_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    dxgi_desc.BufferCount = 1;
    dxgi_desc.OutputWindow = window;
    dxgi_desc.Windowed = TRUE;
    dxgi_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    dxgi_desc.Flags = 0;

    hr = IDXGIFactory_CreateSwapChain(factory, (IUnknown *)device, &dxgi_desc, &swapchain);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IDXGIFactory_Release(factory);

    return swapchain;
}

#define init_test_context(a) init_test_context_(__LINE__, a)
static BOOL init_test_context_(unsigned int line, struct test_context *context)
{
    const D3D11_TEXTURE2D_DESC texture_desc =
    {
        .Width = 640,
        .Height = 480,
        .MipLevels = 1,
        .ArraySize = 1,
        .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
        .SampleDesc.Count = 1,
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = D3D11_BIND_RENDER_TARGET,
    };
    unsigned int rt_width, rt_height;
    D3D11_VIEWPORT vp;
    HRESULT hr;
    RECT rect;

    memset(context, 0, sizeof(*context));

    if (!(context->device = create_device()))
    {
        skip_(__FILE__, line)("Failed to create device.\n");
        return FALSE;
    }

    rt_width = 640;
    rt_height = 480;
    SetRect(&rect, 0, 0, rt_width, rt_height);
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    context->window = CreateWindowA("static", "d3dcompiler_test", WS_OVERLAPPEDWINDOW,
            0, 0, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, NULL, NULL);
    context->swapchain = create_swapchain(context->device, context->window);

    hr = ID3D11Device_CreateTexture2D(context->device, &texture_desc, NULL, &context->rt);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create texture, hr %#lx.\n", hr);

    hr = ID3D11Device_CreateRenderTargetView(context->device, (ID3D11Resource *)context->rt, NULL, &context->rtv);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create rendertarget view, hr %#lx.\n", hr);

    ID3D11Device_GetImmediateContext(context->device, &context->immediate_context);

    ID3D11DeviceContext_OMSetRenderTargets(context->immediate_context, 1, &context->rtv, NULL);

    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width = rt_width;
    vp.Height = rt_height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    ID3D11DeviceContext_RSSetViewports(context->immediate_context, 1, &vp);

    return TRUE;
}

#define release_test_context(context) release_test_context_(__LINE__, context)
static void release_test_context_(unsigned int line, struct test_context *context)
{
    ULONG ref;

    if (context->input_layout)
        ID3D11InputLayout_Release(context->input_layout);
    if (context->vs)
        ID3D11VertexShader_Release(context->vs);
    if (context->vb)
        ID3D11Buffer_Release(context->vb);

    ID3D11DeviceContext_Release(context->immediate_context);
    ID3D11RenderTargetView_Release(context->rtv);
    ID3D11Texture2D_Release(context->rt);
    IDXGISwapChain_Release(context->swapchain);
    DestroyWindow(context->window);

    ref = ID3D11Device_Release(context->device);
    ok_(__FILE__, line)(!ref, "Device has %lu references left.\n", ref);
}

#define create_buffer(a, b, c, d) create_buffer_(__LINE__, a, b, c, d)
static ID3D11Buffer *create_buffer_(unsigned int line, ID3D11Device *device,
        unsigned int bind_flags, unsigned int size, const void *data)
{
    D3D11_SUBRESOURCE_DATA resource_data = {.pSysMem = data};
    D3D11_BUFFER_DESC buffer_desc =
    {
        .ByteWidth = size,
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = bind_flags,
    };
    ID3D11Buffer *buffer;
    HRESULT hr;

    hr = ID3D11Device_CreateBuffer(device, &buffer_desc, data ? &resource_data : NULL, &buffer);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create buffer, hr %#lx.\n", hr);
    return buffer;
}

#define draw_quad(context, ps_code) draw_quad_(__LINE__, context, ps_code)
static void draw_quad_(unsigned int line, struct test_context *context, ID3D10Blob *ps_code)
{
    static const D3D11_INPUT_ELEMENT_DESC default_layout_desc[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    static const char vs_source[] =
        "float4 main(float4 position : POSITION) : SV_POSITION\n"
        "{\n"
        "    return position;\n"
        "}";

    static const struct vec2 quad[] =
    {
        {-1.0f, -1.0f},
        {-1.0f,  1.0f},
        { 1.0f, -1.0f},
        { 1.0f,  1.0f},
    };

    ID3D11Device *device = context->device;
    unsigned int stride, offset;
    ID3D11PixelShader *ps;
    HRESULT hr;

    if (!context->vs)
    {
        ID3D10Blob *vs_code = compile_shader_(line, vs_source, "vs_4_0", 0);

        hr = ID3D11Device_CreateInputLayout(device, default_layout_desc, ARRAY_SIZE(default_layout_desc),
                ID3D10Blob_GetBufferPointer(vs_code), ID3D10Blob_GetBufferSize(vs_code), &context->input_layout);
        ok_(__FILE__, line)(hr == S_OK, "Failed to create input layout, hr %#lx.\n", hr);

        hr = ID3D11Device_CreateVertexShader(device, ID3D10Blob_GetBufferPointer(vs_code),
                ID3D10Blob_GetBufferSize(vs_code), NULL, &context->vs);
        ok_(__FILE__, line)(hr == S_OK, "Failed to create vertex shader, hr %#lx.\n", hr);
    }

    if (!context->vb)
        context->vb = create_buffer_(line, device, D3D11_BIND_VERTEX_BUFFER, sizeof(quad), quad);

    hr = ID3D11Device_CreatePixelShader(device, ID3D10Blob_GetBufferPointer(ps_code),
            ID3D10Blob_GetBufferSize(ps_code), NULL, &ps);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create pixel shader, hr %#lx.\n", hr);

    ID3D11DeviceContext_IASetInputLayout(context->immediate_context, context->input_layout);
    ID3D11DeviceContext_IASetPrimitiveTopology(context->immediate_context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    stride = sizeof(*quad);
    offset = 0;
    ID3D11DeviceContext_IASetVertexBuffers(context->immediate_context, 0, 1, &context->vb, &stride, &offset);
    ID3D11DeviceContext_VSSetShader(context->immediate_context, context->vs, NULL, 0);
    ID3D11DeviceContext_PSSetShader(context->immediate_context, ps, NULL, 0);

    ID3D11DeviceContext_Draw(context->immediate_context, 4, 0);

    ID3D11PixelShader_Release(ps);
}

struct readback
{
    ID3D11Resource *resource;
    D3D11_MAPPED_SUBRESOURCE map_desc;
};

static void init_readback(struct test_context *context, struct readback *rb)
{
    D3D11_TEXTURE2D_DESC texture_desc;
    HRESULT hr;

    ID3D11Texture2D_GetDesc(context->rt, &texture_desc);
    texture_desc.Usage = D3D11_USAGE_STAGING;
    texture_desc.BindFlags = 0;
    texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    texture_desc.MiscFlags = 0;
    hr = ID3D11Device_CreateTexture2D(context->device, &texture_desc, NULL, (ID3D11Texture2D **)&rb->resource);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ID3D11DeviceContext_CopyResource(context->immediate_context, rb->resource, (ID3D11Resource *)context->rt);
    hr = ID3D11DeviceContext_Map(context->immediate_context, rb->resource, 0, D3D11_MAP_READ, 0, &rb->map_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
}

static void release_readback(struct test_context *context, struct readback *rb)
{
    ID3D11DeviceContext_Unmap(context->immediate_context, rb->resource, 0);
    ID3D11Resource_Release(rb->resource);
}

static const struct vec4 *get_readback_vec4(struct readback *rb, unsigned int x, unsigned int y)
{
    return (struct vec4 *)((BYTE *)rb->map_desc.pData + y * rb->map_desc.RowPitch) + x;
}

static struct vec4 get_color_vec4(struct test_context *context, unsigned int x, unsigned int y)
{
    struct readback rb;
    struct vec4 ret;

    init_readback(context, &rb);
    ret = *get_readback_vec4(&rb, x, y);
    release_readback(context, &rb);
    return ret;
}

static void test_swizzle(void)
{
    static const struct vec4 uniform = {0.0303f, 0.0f, 0.0f, 0.0202f};
    struct test_context test_context;
    ID3D10Blob *ps_code = NULL;
    ID3D11Buffer *cb;
    struct vec4 v;

    static const char ps_source[] =
        "uniform float4 color;\n"
        "float4 main() : SV_TARGET\n"
        "{\n"
        "    float4 ret = color;\n"
        "    ret.gb = ret.ra;\n"
        "    ret.ra = float2(0.0101, 0.0404);\n"
        "    return ret;\n"
        "}";

    if (!init_test_context(&test_context))
        return;

    ps_code = compile_shader(ps_source, "ps_4_0");
    cb = create_buffer(test_context.device, D3D11_BIND_CONSTANT_BUFFER, sizeof(uniform), &uniform);
    ID3D11DeviceContext_PSSetConstantBuffers(test_context.immediate_context, 0, 1, &cb);
    draw_quad(&test_context, ps_code);

    v = get_color_vec4(&test_context, 0, 0);
    ok(compare_vec4(&v, 0.0101f, 0.0303f, 0.0202f, 0.0404f, 0),
            "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v.x, v.y, v.z, v.w);

    ID3D11Buffer_Release(cb);
    ID3D10Blob_Release(ps_code);
    release_test_context(&test_context);
}

static void test_math(void)
{
    static const float uniforms[8] = {2.5f, 0.3f, 0.2f, 0.7f, 0.1f, 1.5f};
    struct test_context test_context;
    ID3D10Blob *ps_code = NULL;
    ID3D11Buffer *cb;
    struct vec4 v;

    static const char ps_source[] =
        "float4 main(uniform float u, uniform float v, uniform float w, uniform float x,\n"
        "            uniform float y, uniform float z) : SV_TARGET\n"
        "{\n"
        "    return float4(x * y - z / w + --u / -v,\n"
        "            z * x / y + w / -v,\n"
        "            u + v - w,\n"
        "            x / y / w);\n"
        "}";

    if (!init_test_context(&test_context))
        return;

    ps_code = compile_shader(ps_source, "ps_4_0");
    cb = create_buffer(test_context.device, D3D11_BIND_CONSTANT_BUFFER, sizeof(uniforms), uniforms);
    ID3D11DeviceContext_PSSetConstantBuffers(test_context.immediate_context, 0, 1, &cb);
    draw_quad(&test_context, ps_code);

    v = get_color_vec4(&test_context, 0, 0);
    ok(compare_vec4(&v, -12.43f, 9.833333f, 1.6f, 35.0f, 1),
            "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v.x, v.y, v.z, v.w);

    ID3D11Buffer_Release(cb);
    ID3D10Blob_Release(ps_code);
    release_test_context(&test_context);
}

static void test_conditionals(void)
{
    struct test_context test_context;
    ID3D10Blob *ps_code = NULL;
    const struct vec4 *v;
    struct readback rb;
    unsigned int i;

    static const char ps_source[] =
        "float4 main(float4 pos : SV_POSITION) : SV_TARGET\n"
        "{\n"
        "    if(pos.x > 200.0)\n"
        "        return float4(0.1, 0.2, 0.3, 0.4);\n"
        "    else\n"
        "        return float4(0.9, 0.8, 0.7, 0.6);\n"
        "}";

    if (!init_test_context(&test_context))
        return;

    ps_code = compile_shader(ps_source, "ps_4_0");
    draw_quad(&test_context, ps_code);
    init_readback(&test_context, &rb);

    for (i = 0; i < 200; i += 40)
    {
        v = get_readback_vec4(&rb, i, 0);
        ok(compare_vec4(v, 0.9f, 0.8f, 0.7f, 0.6f, 0),
                "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v->x, v->y, v->z, v->w);
    }

    for (i = 240; i < 640; i += 40)
    {
        v = get_readback_vec4(&rb, i, 0);
        ok(compare_vec4(v, 0.1f, 0.2f, 0.3f, 0.4f, 0),
                "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v->x, v->y, v->z, v->w);
    }

    release_readback(&test_context, &rb);
    ID3D10Blob_Release(ps_code);
    release_test_context(&test_context);
}

static void test_trig(void)
{
    struct test_context test_context;
    ID3D10Blob *ps_code = NULL;
    const struct vec4 *v;
    struct readback rb;
    unsigned int i;

    static const char ps_source[] =
        "float4 main(float4 pos : SV_POSITION) : SV_TARGET\n"
        "{\n"
        "    return float4(sin(pos.x - 0.5), cos(pos.x - 0.5), 0, 0);\n"
        "}";

    if (!init_test_context(&test_context))
        return;

    ps_code = compile_shader(ps_source, "ps_4_0");
    draw_quad(&test_context, ps_code);
    init_readback(&test_context, &rb);

    for (i = 0; i < 640; i += 20)
    {
        v = get_readback_vec4(&rb, i, 0);
        ok(compare_vec4(v, sinf(i), cosf(i), 0.0f, 0.0f, 16384),
                "Test %u: Got {%.8e, %.8e, %.8e, %.8e}, expected {%.8e, %.8e, %.8e, %.8e}.\n",
                i, v->x, v->y, v->z, v->w, sinf(i), cos(i), 0.0f, 0.0f);
    }

    release_readback(&test_context, &rb);
    ID3D10Blob_Release(ps_code);
    release_test_context(&test_context);
}

static void test_sampling(void)
{
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {0};
    struct test_context test_context;
    ID3D11ShaderResourceView *srv;
    ID3D11SamplerState *sampler;
    ID3D10Blob *ps_code = NULL;
    ID3D11Texture2D *texture;
    unsigned int i;
    struct vec4 v;
    HRESULT hr;

    static const char *tests[] =
    {
        "sampler s;\n"
        "float4 main() : COLOR\n"
        "{\n"
        "    return tex2D(s, float2(0.5, 0.5));\n"
        "}",

        "SamplerState s;\n"
        "float4 main() : COLOR\n"
        "{\n"
        "    return tex2D(s, float2(0.5, 0.5));\n"
        "}",

        "sampler2D s;\n"
        "float4 main() : COLOR\n"
        "{\n"
        "    return tex2D(s, float2(0.5, 0.5));\n"
        "}",

        "sampler s;\n"
        "Texture2D t;\n"
        "float4 main() : COLOR\n"
        "{\n"
        "    return t.Sample(s, float2(0.5, 0.5));\n"
        "}",

        "SamplerState s;\n"
        "Texture2D t;\n"
        "float4 main() : COLOR\n"
        "{\n"
        "    return t.Sample(s, float2(0.5, 0.5));\n"
        "}",
    };

    static const D3D11_TEXTURE2D_DESC texture_desc =
    {
        .Width = 2,
        .Height = 2,
        .MipLevels = 1,
        .ArraySize = 1,
        .Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
        .SampleDesc.Count = 1,
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = D3D11_BIND_SHADER_RESOURCE,
    };

    static const float texture_data[] =
    {
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
    };

    static const D3D11_SUBRESOURCE_DATA resource_data = {&texture_data, sizeof(texture_data) / 2};

    static const D3D11_SAMPLER_DESC sampler_desc =
    {
        .Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
        .AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
        .AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
        .AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
    };

    static const float red[] = {1.0f, 0.0f, 0.0f, 1.0f};

    if (!init_test_context(&test_context))
        return;

    hr = ID3D11Device_CreateTexture2D(test_context.device, &texture_desc, &resource_data, &texture);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    srv_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels = 1;
    hr = ID3D11Device_CreateShaderResourceView(test_context.device, (ID3D11Resource *)texture, &srv_desc, &srv);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D11DeviceContext_PSSetShaderResources(test_context.immediate_context, 0, 1, &srv);

    hr = ID3D11Device_CreateSamplerState(test_context.device, &sampler_desc, &sampler);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D11DeviceContext_PSSetSamplers(test_context.immediate_context, 0, 1, &sampler);

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        winetest_push_context("Test %u", i);

        ID3D11DeviceContext_ClearRenderTargetView(test_context.immediate_context, test_context.rtv, red);
        ps_code = compile_shader_flags(tests[i], "ps_4_0", D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY);
        draw_quad(&test_context, ps_code);

        v = get_color_vec4(&test_context, 0, 0);
        ok(compare_vec4(&v, 0.25f, 0.0f, 0.25f, 0.0f, 0),
                "Got unexpected value {%.8e, %.8e, %.8e, %.8e}.\n", v.x, v.y, v.z, v.w);

        ID3D10Blob_Release(ps_code);

        winetest_pop_context();
    }

    ID3D11Texture2D_Release(texture);
    ID3D11SamplerState_Release(sampler);
    ID3D11ShaderResourceView_Release(srv);
    release_test_context(&test_context);
}

static void check_type_desc(const D3D11_SHADER_TYPE_DESC *type, const D3D11_SHADER_TYPE_DESC *expect)
{
    ok(type->Class == expect->Class, "Got class %#x.\n", type->Class);
    ok(type->Type == expect->Type, "Got type %#x.\n", type->Type);
    ok(type->Rows == expect->Rows, "Got %u rows.\n", type->Rows);
    ok(type->Columns == expect->Columns, "Got %u columns.\n", type->Columns);
    ok(type->Elements == expect->Elements, "Got %u elements.\n", type->Elements);
    ok(type->Members == expect->Members, "Got %u members.\n", type->Members);
    ok(type->Offset == expect->Offset, "Got %u members.\n", type->Members);
    ok(!strcmp(type->Name, expect->Name), "Got name %s.\n", debugstr_a(type->Name));
}

static void check_resource_binding(const D3D11_SHADER_INPUT_BIND_DESC *desc,
        const D3D11_SHADER_INPUT_BIND_DESC *expect)
{
    ok(!strcmp(desc->Name, expect->Name), "Got name %s.\n", debugstr_a(desc->Name));
    ok(desc->Type == expect->Type, "Got type %#x.\n", desc->Type);
    ok(desc->BindPoint == expect->BindPoint, "Got bind point %u.\n", desc->BindPoint);
    ok(desc->BindCount == expect->BindCount, "Got bind count %u.\n", desc->BindCount);
    ok(desc->uFlags == expect->uFlags, "Got flags %#x.\n", desc->uFlags);
    ok(desc->ReturnType == expect->ReturnType, "Got return type %#x.\n", desc->ReturnType);
    ok(desc->Dimension == expect->Dimension, "Got dimension %#x.\n", desc->Dimension);
    ok(desc->NumSamples == expect->NumSamples, "Got multisample count %u.\n", desc->NumSamples);
}

static void test_reflection(void)
{
    ID3D11ShaderReflectionConstantBuffer *cbuffer;
    ID3D11ShaderReflectionType *type, *field;
    D3D11_SHADER_BUFFER_DESC buffer_desc;
    ID3D11ShaderReflectionVariable *var;
    D3D11_SHADER_VARIABLE_DESC var_desc;
    ID3D11ShaderReflection *reflection;
    D3D11_SHADER_TYPE_DESC type_desc;
    D3D11_SHADER_DESC shader_desc;
    ID3D10Blob *code = NULL;
    unsigned int i, j, k;
    ULONG refcount;
    HRESULT hr;

    static const char vs_source[] =
        "typedef uint uint_t;\n"
        "float m;\n"
        "\n"
        "cbuffer b1\n"
        "{\n"
        "    float a;\n"
        "    float2 b;\n"
        "    float4 c;\n"
        "    float d;\n"
        "    struct\n"
        "    {\n"
        "        float4 a;\n"
        "        float b;\n"
        "        float c;\n"
        "    } s;\n"
        /* In direct contradiction to the documentation, this does not align. */
        "    bool g;\n"
        "    float h[2];\n"
        "    int i;\n"
        "    uint_t j;\n"
        "    float3x1 k;\n"
        "    row_major float3x1 l;\n"
        "#pragma pack_matrix(row_major)\n"
        "    float3x1 o;\n"
        "    float4 p;\n"
        "    float q;\n"
        "    struct r_name {float a;} r;\n"
        "    column_major float3x1 t;\n"
        "};\n"
        "\n"
        "cbuffer b5 : register(b5)\n"
        "{\n"
        "    float4 u;\n"
        "}\n"
        "\n"
        "float4 main(uniform float4 n) : SV_POSITION\n"
        "{\n"
        "    return o._31 + m + n + u;\n"
        "}";

    struct shader_variable
    {
        D3D11_SHADER_VARIABLE_DESC var_desc;
        D3D11_SHADER_TYPE_DESC type_desc;
        const D3D11_SHADER_TYPE_DESC *field_types;
    };

    static const D3D11_SHADER_TYPE_DESC s_field_types[] =
    {
        {D3D_SVC_VECTOR, D3D_SVT_FLOAT, 1, 4, 0, 0, 0, "float4"},
        {D3D_SVC_SCALAR, D3D_SVT_FLOAT, 1, 1, 0, 0, 16, "float"},
        {D3D_SVC_SCALAR, D3D_SVT_FLOAT, 1, 1, 0, 0, 20, "float"},
    };

    static const D3D11_SHADER_TYPE_DESC r_field_types[] =
    {
        {D3D_SVC_SCALAR, D3D_SVT_FLOAT, 1, 1, 0, 0, 0, "float"},
    };

    static const struct shader_variable globals_vars =
        {{"m", 0, 4, D3D_SVF_USED}, {D3D_SVC_SCALAR, D3D_SVT_FLOAT, 1, 1, 0, 0, 0, "float"}};
    static const struct shader_variable params_vars =
        {{"n", 0, 16, D3D_SVF_USED}, {D3D_SVC_VECTOR, D3D_SVT_FLOAT, 1, 4, 0, 0, 0, "float4"}};
    static const struct shader_variable buffer_vars[] =
    {
        {{"a", 0, 4}, {D3D_SVC_SCALAR, D3D_SVT_FLOAT, 1, 1, 0, 0, 0, "float"}},
        {{"b", 4, 8}, {D3D_SVC_VECTOR, D3D_SVT_FLOAT, 1, 2, 0, 0, 0, "float2"}},
        {{"c", 16, 16}, {D3D_SVC_VECTOR, D3D_SVT_FLOAT, 1, 4, 0, 0, 0, "float4"}},
        {{"d", 32, 4}, {D3D_SVC_SCALAR, D3D_SVT_FLOAT, 1, 1, 0, 0, 0, "float"}},
        {{"s", 48, 24}, {D3D_SVC_STRUCT, D3D_SVT_VOID, 1, 6, 0, ARRAY_SIZE(s_field_types), 0, "<unnamed>"}, s_field_types},
        {{"g", 72, 4}, {D3D_SVC_SCALAR, D3D_SVT_BOOL, 1, 1, 0, 0, 0, "bool"}},
        {{"h", 80, 20}, {D3D_SVC_SCALAR, D3D_SVT_FLOAT, 1, 1, 2, 0, 0, "float"}},
        {{"i", 100, 4}, {D3D_SVC_SCALAR, D3D_SVT_INT, 1, 1, 0, 0, 0, "int"}},
        {{"j", 104, 4}, {D3D_SVC_SCALAR, D3D_SVT_UINT, 1, 1, 0, 0, 0, "uint_t"}},
        {{"k", 112, 12}, {D3D_SVC_MATRIX_COLUMNS, D3D_SVT_FLOAT, 3, 1, 0, 0, 0, "float3x1"}},
        {{"l", 128, 36}, {D3D_SVC_MATRIX_ROWS, D3D_SVT_FLOAT, 3, 1, 0, 0, 0, "float3x1"}},
        {{"o", 176, 36, D3D_SVF_USED}, {D3D_SVC_MATRIX_ROWS, D3D_SVT_FLOAT, 3, 1, 0, 0, 0, "float3x1"}},
        {{"p", 224, 16}, {D3D_SVC_VECTOR, D3D_SVT_FLOAT, 1, 4, 0, 0, 0, "float4"}},
        {{"q", 240, 4}, {D3D_SVC_SCALAR, D3D_SVT_FLOAT, 1, 1, 0, 0, 0, "float"}},
        {{"r", 256, 4}, {D3D_SVC_STRUCT, D3D_SVT_VOID, 1, 1, 0, ARRAY_SIZE(r_field_types), 0, "r_name"}, r_field_types},
        {{"t", 260, 12}, {D3D_SVC_MATRIX_COLUMNS, D3D_SVT_FLOAT, 3, 1, 0, 0, 0, "float3x1"}},
    };
    static const struct shader_variable b5_vars =
        {{"u", 0, 16, D3D_SVF_USED}, {D3D_SVC_VECTOR, D3D_SVT_FLOAT, 1, 4, 0, 0, 0, "float4"}};

    static const struct
    {
        D3D11_SHADER_BUFFER_DESC desc;
        const struct shader_variable *vars;
    }
    vs_buffers[] =
    {
        {{"$Globals", D3D_CT_CBUFFER, 1, 16}, &globals_vars},
        {{"$Params", D3D_CT_CBUFFER, 1, 16}, &params_vars},
        {{"b1", D3D_CT_CBUFFER, ARRAY_SIZE(buffer_vars), 272}, buffer_vars},
        {{"b5", D3D_CT_CBUFFER, 1, 16}, &b5_vars},
    };

    static const D3D11_SHADER_INPUT_BIND_DESC vs_bindings[] =
    {
        {"$Globals", D3D_SIT_CBUFFER, 0, 1},
        {"$Params", D3D_SIT_CBUFFER, 1, 1},
        {"b1", D3D_SIT_CBUFFER, 2, 1},
        {"b5", D3D_SIT_CBUFFER, 5, 1, D3D_SIF_USERPACKED},
    };

    static const char ps_source[] =
        "texture2D a;\n"
        "sampler c {};\n"
        "SamplerState d {};\n"
        "sampler e\n"
        "{\n"
        "    Texture = a;\n"
        "    foo = bar + 2;\n"
        "};\n"
        "SamplerState f\n"
        "{\n"
        "    Texture = a;\n"
        "    foo = bar + 2;\n"
        "};\n"
        "sampler2D g;\n"
        "sampler b : register(s5);\n"
        "float4 main(float2 pos : texcoord) : SV_TARGET\n"
        "{\n"
        "    return a.Sample(b, pos) + a.Sample(c, pos) + a.Sample(d, pos) + tex2D(f, pos) + tex2D(e, pos)"
        "            + tex2D(g, pos);\n"
        "}";

    static const D3D11_SHADER_INPUT_BIND_DESC ps_bindings[] =
    {
        {"c", D3D_SIT_SAMPLER, 0, 1},
        {"d", D3D_SIT_SAMPLER, 1, 1},
        {"e", D3D_SIT_SAMPLER, 2, 1},
        {"f", D3D_SIT_SAMPLER, 3, 1},
        {"g", D3D_SIT_SAMPLER, 4, 1},
        {"b", D3D_SIT_SAMPLER, 5, 1, D3D_SIF_USERPACKED},
        {"f", D3D_SIT_TEXTURE, 0, 1, D3D_SIF_TEXTURE_COMPONENTS, D3D_RETURN_TYPE_FLOAT, D3D_SRV_DIMENSION_TEXTURE2D, ~0u},
        {"e", D3D_SIT_TEXTURE, 1, 1, D3D_SIF_TEXTURE_COMPONENTS, D3D_RETURN_TYPE_FLOAT, D3D_SRV_DIMENSION_TEXTURE2D, ~0u},
        {"g", D3D_SIT_TEXTURE, 2, 1, D3D_SIF_TEXTURE_COMPONENTS, D3D_RETURN_TYPE_FLOAT, D3D_SRV_DIMENSION_TEXTURE2D, ~0u},
        {"a", D3D_SIT_TEXTURE, 3, 1, D3D_SIF_TEXTURE_COMPONENTS, D3D_RETURN_TYPE_FLOAT, D3D_SRV_DIMENSION_TEXTURE2D, ~0u},
    };

    code = compile_shader(vs_source, "vs_5_0");
    hr = D3DReflect(ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code),
            &IID_ID3D11ShaderReflection, (void **)&reflection);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = reflection->lpVtbl->GetDesc(reflection, &shader_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(shader_desc.ConstantBuffers == ARRAY_SIZE(vs_buffers), "Got %u buffers.\n", shader_desc.ConstantBuffers);
    ok(shader_desc.BoundResources == ARRAY_SIZE(vs_bindings), "Got %u resources.\n", shader_desc.BoundResources);

    for (i = 0; i < ARRAY_SIZE(vs_buffers); ++i)
    {
        winetest_push_context("Buffer %u", i);

        cbuffer = reflection->lpVtbl->GetConstantBufferByIndex(reflection, i);
        hr = cbuffer->lpVtbl->GetDesc(cbuffer, &buffer_desc);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(!strcmp(buffer_desc.Name, vs_buffers[i].desc.Name), "Got name %s.\n", debugstr_a(buffer_desc.Name));
        ok(buffer_desc.Type == vs_buffers[i].desc.Type, "Got type %#x.\n", buffer_desc.Type);
        ok(buffer_desc.Variables == vs_buffers[i].desc.Variables, "Got %u variables.\n", buffer_desc.Variables);
        ok(buffer_desc.Size == vs_buffers[i].desc.Size, "Got size %u.\n", buffer_desc.Size);
        ok(buffer_desc.uFlags == vs_buffers[i].desc.uFlags, "Got flags %#x.\n", buffer_desc.uFlags);

        for (j = 0; j < buffer_desc.Variables; ++j)
        {
            const struct shader_variable *expect = &vs_buffers[i].vars[j];

            winetest_push_context("Variable %u", j);

            var = cbuffer->lpVtbl->GetVariableByIndex(cbuffer, j);
            hr = var->lpVtbl->GetDesc(var, &var_desc);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
            ok(!strcmp(var_desc.Name, expect->var_desc.Name), "Got name %s.\n", debugstr_a(var_desc.Name));
            ok(var_desc.StartOffset == expect->var_desc.StartOffset, "Got offset %u.\n", var_desc.StartOffset);
            ok(var_desc.Size == expect->var_desc.Size, "Got size %u.\n", var_desc.Size);
            ok(var_desc.uFlags == expect->var_desc.uFlags, "Got flags %#x.\n", var_desc.uFlags);
            ok(!var_desc.DefaultValue, "Got default value %p.\n", var_desc.DefaultValue);

            type = var->lpVtbl->GetType(var);
            hr = type->lpVtbl->GetDesc(type, &type_desc);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
            check_type_desc(&type_desc, &expect->type_desc);

            for (k = 0; k < type_desc.Members; ++k)
            {
                winetest_push_context("Field %u", k);

                field = type->lpVtbl->GetMemberTypeByIndex(type, k);
                hr = field->lpVtbl->GetDesc(field, &type_desc);
                ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
                check_type_desc(&type_desc, &vs_buffers[i].vars[j].field_types[k]);

                winetest_pop_context();
            }

            winetest_pop_context();
        }

        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(vs_bindings); ++i)
    {
        D3D11_SHADER_INPUT_BIND_DESC desc;

        winetest_push_context("Binding %u", i);

        hr = reflection->lpVtbl->GetResourceBindingDesc(reflection, i, &desc);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        check_resource_binding(&desc, &vs_bindings[i]);

        winetest_pop_context();
    }

    ID3D10Blob_Release(code);
    refcount = reflection->lpVtbl->Release(reflection);
    ok(!refcount, "Got unexpected refcount %lu.\n", refcount);

    code = compile_shader_flags(ps_source, "ps_4_0", D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY);
    hr = D3DReflect(ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code),
            &IID_ID3D11ShaderReflection, (void **)&reflection);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = reflection->lpVtbl->GetDesc(reflection, &shader_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!shader_desc.ConstantBuffers, "Got %u buffers.\n", shader_desc.ConstantBuffers);
    ok(shader_desc.BoundResources == ARRAY_SIZE(ps_bindings), "Got %u resources.\n", shader_desc.BoundResources);

    for (i = 0; i < shader_desc.BoundResources; ++i)
    {
        D3D11_SHADER_INPUT_BIND_DESC desc;

        winetest_push_context("Binding %u", i);

        hr = reflection->lpVtbl->GetResourceBindingDesc(reflection, i, &desc);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        check_resource_binding(&desc, &ps_bindings[i]);

        winetest_pop_context();
    }

    ID3D10Blob_Release(code);
    refcount = reflection->lpVtbl->Release(reflection);
    ok(!refcount, "Got unexpected refcount %lu.\n", refcount);
}

static void check_parameter_desc(const D3D11_SIGNATURE_PARAMETER_DESC *desc,
        const D3D11_SIGNATURE_PARAMETER_DESC *expect)
{
    todo_wine_if(strcmp(desc->SemanticName, expect->SemanticName))
        ok(!strcmp(desc->SemanticName, expect->SemanticName), "Got name %s.\n", debugstr_a(desc->SemanticName));
    ok(desc->SemanticIndex == expect->SemanticIndex, "Got index %u.\n", desc->SemanticIndex);
    ok(desc->Register == expect->Register, "Got register %u.\n", desc->Register);
    todo_wine_if(desc->SystemValueType != expect->SystemValueType)
        ok(desc->SystemValueType == expect->SystemValueType, "Got sysval %u.\n", desc->SystemValueType);
    ok(desc->ComponentType == expect->ComponentType, "Got data type %u.\n", desc->ComponentType);
    ok(desc->Mask == expect->Mask, "Got mask %#x.\n", desc->Mask);
    todo_wine_if(desc->ReadWriteMask != expect->ReadWriteMask)
        ok(desc->ReadWriteMask == expect->ReadWriteMask, "Got used mask %#x.\n", desc->ReadWriteMask);
    ok(desc->Stream == expect->Stream, "Got stream %u.\n", desc->Stream);
}

static void test_semantic_reflection(void)
{
    D3D11_SIGNATURE_PARAMETER_DESC desc;
    ID3D11ShaderReflection *reflection;
    D3D11_SHADER_DESC shader_desc;
    ID3D10Blob *code = NULL;
    unsigned int i, j;
    ULONG refcount;
    HRESULT hr;

    static const char vs1_source[] =
        "void main(\n"
        "        in float4 a : apple,\n"
        "        out float4 b : banana2,\n"
        "        inout float4 c : color,\n"
        "        inout float4 d : depth,\n"
        "        inout float4 e : sv_position,\n"
        "        in uint3 f : fruit,\n"
        "        inout bool2 g : grape,\n"
        "        in int h : honeydew)\n"
        "{\n"
        "    b.yw = a.xz;\n"
        "}";

    static const D3D11_SIGNATURE_PARAMETER_DESC vs1_inputs[] =
    {
        {"apple",       0, 0, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0xf, 0x5},
        {"color",       0, 1, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0xf, 0xf},
        {"depth",       0, 2, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0xf, 0xf},
        {"sv_position", 0, 3, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0xf, 0xf},
        {"fruit",       0, 4, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_UINT32, 0x7},
        {"grape",       0, 5, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_UINT32, 0x3, 0x3},
        {"honeydew",    0, 6, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_SINT32, 0x1},
    };

    static const D3D11_SIGNATURE_PARAMETER_DESC vs1_outputs[] =
    {
        {"banana",      2, 0, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0xf, 0x5},
        {"color",       0, 1, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0xf},
        {"depth",       0, 2, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0xf},
        {"sv_position", 0, 3, D3D_NAME_POSITION,  D3D_REGISTER_COMPONENT_FLOAT32, 0xf},
        {"grape",       0, 4, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_UINT32, 0x3, 0xc},
    };

    static const char vs2_source[] =
        "void main(inout float4 pos : position)\n"
        "{\n"
        "}";

    static const D3D11_SIGNATURE_PARAMETER_DESC vs2_inputs[] =
    {
        {"position",    0, 0, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0xf, 0xf},
    };

    static const D3D11_SIGNATURE_PARAMETER_DESC vs2_outputs[] =
    {
        {"position",    0, 0, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0xf},
    };

    static const D3D11_SIGNATURE_PARAMETER_DESC vs2_legacy_outputs[] =
    {
        {"SV_Position", 0, 0, D3D_NAME_POSITION, D3D_REGISTER_COMPONENT_FLOAT32, 0xf},
    };

    static const char ps1_source[] =
        "void main(\n"
        "        in float2 a : apple,\n"
        "        out float4 b : sv_target2,\n"
        "        out float c : sv_depth,\n"
        "        in float4 d : position,\n"
        "        in float4 e : sv_position)\n"
        "{\n"
        "    b = d;\n"
        "    c = 0;\n"
        "}";

    static const D3D11_SIGNATURE_PARAMETER_DESC ps1_inputs[] =
    {
        {"apple",       0, 0, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0x3},
        {"position",    0, 1, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0xf, 0xf},
        {"sv_position", 0, 2, D3D_NAME_POSITION,  D3D_REGISTER_COMPONENT_FLOAT32, 0xf},
    };

    static const D3D11_SIGNATURE_PARAMETER_DESC ps1_outputs[] =
    {
        {"sv_target",   2, 2,   D3D_NAME_TARGET, D3D_REGISTER_COMPONENT_FLOAT32, 0xf},
        {"sv_depth",    0, ~0u, D3D_NAME_DEPTH,  D3D_REGISTER_COMPONENT_FLOAT32, 0x1, 0xe},
    };

    static const char ps2_source[] =
        "void main(\n"
        "        inout float4 a : color2,\n"
        "        inout float b : depth,\n"
        "        in float4 c : position)\n"
        "{\n"
        "}";

    static const D3D11_SIGNATURE_PARAMETER_DESC ps2_inputs[] =
    {
        {"color",       2, 0, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0xf, 0xf},
        {"depth",       0, 1, D3D_NAME_UNDEFINED, D3D_REGISTER_COMPONENT_FLOAT32, 0x1, 0x1},
        {"SV_Position", 0, 2, D3D_NAME_POSITION,  D3D_REGISTER_COMPONENT_FLOAT32, 0xf},
    };

    static const D3D11_SIGNATURE_PARAMETER_DESC ps2_outputs[] =
    {
        {"SV_Target",   2, 2,   D3D_NAME_TARGET, D3D_REGISTER_COMPONENT_FLOAT32, 0xf},
        {"SV_Depth",    0, ~0u, D3D_NAME_DEPTH,  D3D_REGISTER_COMPONENT_FLOAT32, 0x1, 0xe},
    };

    static const char cs1_source[] =
        "[numthreads(1, 1, 1)]\n"
        "void main(in uint a : sv_dispatchthreadid)\n"
        "{\n"
        "}";

    static const char gs1_source[] =
        "struct input\n"
        "{\n"
        "    float4 a : sv_position;\n"
        "    float4 b : apple2;\n"
        "};\n"
        "struct vertex\n"
        "{\n"
        "    float4 a : sv_position;\n"
        "    float4 b : apple2;\n"
        "    uint c : sv_primitiveid;\n"
        "};\n"
        "[maxvertexcount(1)]\n"
        "void main(\n"
        "        point input i[1],\n"
        "        inout PointStream<vertex> o,\n"
        "        uint a : sv_primitiveid)\n"
        "{\n"
        "    struct vertex v;\n"
        "    v.a = i[0].a;\n"
        "    v.b = i[0].b;\n"
        "    v.c = a;\n"
        "    o.Append(v);\n"
        "}";

    static const D3D11_SIGNATURE_PARAMETER_DESC gs1_inputs[] =
    {
        {"sv_position",     0, 0,   D3D_NAME_POSITION,      D3D_REGISTER_COMPONENT_FLOAT32, 0xf, 0xf},
        {"apple",           2, 1,   D3D_NAME_UNDEFINED,     D3D_REGISTER_COMPONENT_FLOAT32, 0xf, 0xf},
        {"sv_primitiveid",  0, ~0u, D3D_NAME_PRIMITIVE_ID,  D3D_REGISTER_COMPONENT_UINT32,  0x1, 0x1},
    };

    static const D3D11_SIGNATURE_PARAMETER_DESC gs1_outputs[] =
    {
        {"sv_position",     0, 0,   D3D_NAME_POSITION,      D3D_REGISTER_COMPONENT_FLOAT32, 0xf},
        {"apple",           2, 1,   D3D_NAME_UNDEFINED,     D3D_REGISTER_COMPONENT_FLOAT32, 0xf},
        {"sv_primitiveid",  0, 2,   D3D_NAME_PRIMITIVE_ID,  D3D_REGISTER_COMPONENT_UINT32,  0x1, 0xe},
    };

    static const struct
    {
        const char *source;
        const char *target;
        BOOL legacy;
        const D3D11_SIGNATURE_PARAMETER_DESC *inputs;
        unsigned int input_count;
        const D3D11_SIGNATURE_PARAMETER_DESC *outputs;
        unsigned int output_count;
    }
    tests[] =
    {
        {vs1_source, "vs_4_0", FALSE, vs1_inputs, ARRAY_SIZE(vs1_inputs), vs1_outputs, ARRAY_SIZE(vs1_outputs)},
        {vs1_source, "vs_4_0", TRUE,  vs1_inputs, ARRAY_SIZE(vs1_inputs), vs1_outputs, ARRAY_SIZE(vs1_outputs)},
        {vs2_source, "vs_4_0", FALSE, vs2_inputs, ARRAY_SIZE(vs2_inputs), vs2_outputs, ARRAY_SIZE(vs2_outputs)},
        {vs2_source, "vs_4_0", TRUE,  vs2_inputs, ARRAY_SIZE(vs2_inputs), vs2_legacy_outputs, ARRAY_SIZE(vs2_legacy_outputs)},
        {ps1_source, "ps_4_0", FALSE, ps1_inputs, ARRAY_SIZE(ps1_inputs), ps1_outputs, ARRAY_SIZE(ps1_outputs)},
        {ps2_source, "ps_4_0", TRUE,  ps2_inputs, ARRAY_SIZE(ps2_inputs), ps2_outputs, ARRAY_SIZE(ps2_outputs)},
        {cs1_source, "cs_4_0", FALSE, NULL, 0, NULL, 0},
        {gs1_source, "gs_4_0", FALSE, gs1_inputs, ARRAY_SIZE(gs1_inputs), gs1_outputs, ARRAY_SIZE(gs1_outputs)},
    };

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        winetest_push_context("Test %u", i);

        todo_wine_if (i > 6) code = compile_shader_flags(tests[i].source, tests[i].target,
                tests[i].legacy ? D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY : 0);
        if (!code)
        {
            winetest_pop_context();
            continue;
        }

        hr = D3DReflect(ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code),
                &IID_ID3D11ShaderReflection, (void **)&reflection);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = reflection->lpVtbl->GetDesc(reflection, &shader_desc);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(shader_desc.InputParameters == tests[i].input_count,
                "Got %u input parameters.\n", shader_desc.InputParameters);
        ok(shader_desc.OutputParameters == tests[i].output_count,
                "Got %u output parameters.\n", shader_desc.OutputParameters);

        for (j = 0; j < shader_desc.InputParameters; ++j)
        {
            winetest_push_context("Input %u", j);
            hr = reflection->lpVtbl->GetInputParameterDesc(reflection, j, &desc);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
            check_parameter_desc(&desc, &tests[i].inputs[j]);
            winetest_pop_context();
        }

        for (j = 0; j < shader_desc.OutputParameters; ++j)
        {
            winetest_push_context("Output %u", j);
            hr = reflection->lpVtbl->GetOutputParameterDesc(reflection, j, &desc);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
            check_parameter_desc(&desc, &tests[i].outputs[j]);
            winetest_pop_context();
        }

        ID3D10Blob_Release(code);
        refcount = reflection->lpVtbl->Release(reflection);
        ok(!refcount, "Got unexpected refcount %lu.\n", refcount);

        winetest_pop_context();
    }
}

START_TEST(hlsl_d3d11)
{
    HMODULE mod;

    test_reflection();
    test_semantic_reflection();

    if (!(mod = LoadLibraryA("d3d11.dll")))
    {
        skip("Direct3D 11 is not available.\n");
        return;
    }
    pD3D11CreateDevice = (void *)GetProcAddress(mod, "D3D11CreateDevice");

    test_swizzle();
    test_math();
    test_conditionals();
    test_trig();
    test_sampling();
}
