/*
 * Copyright 2017 Józef Kucia for CodeWeavers
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

#include <assert.h>
#include <stdlib.h>
#define COBJMACROS
#include "initguid.h"
#include "d3d12.h"
#include "d3d12sdklayers.h"
#include "dxgi1_6.h"
#include "wine/test.h"

static BOOL compare_uint(unsigned int x, unsigned int y, unsigned int max_diff)
{
    unsigned int diff = x > y ? x - y : y - x;

    return diff <= max_diff;
}

static BOOL compare_color(DWORD c1, DWORD c2, unsigned int max_diff)
{
    return compare_uint(c1 & 0xff, c2 & 0xff, max_diff)
            && compare_uint((c1 >> 8) & 0xff, (c2 >> 8) & 0xff, max_diff)
            && compare_uint((c1 >> 16) & 0xff, (c2 >> 16) & 0xff, max_diff)
            && compare_uint((c1 >> 24) & 0xff, (c2 >> 24) & 0xff, max_diff);
}

static BOOL equal_luid(LUID a, LUID b)
{
    return a.LowPart == b.LowPart && a.HighPart == b.HighPart;
}

static unsigned int format_size(DXGI_FORMAT format)
{
    switch (format)
    {
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
            return 4;
        default:
            trace("Unhandled format %#x.\n", format);
            return 1;
    }
}

static size_t align(size_t addr, size_t alignment)
{
    return (addr + (alignment - 1)) & ~(alignment - 1);
}

static void set_viewport(D3D12_VIEWPORT *vp, float x, float y,
        float width, float height, float min_depth, float max_depth)
{
    vp->TopLeftX = x;
    vp->TopLeftY = y;
    vp->Width = width;
    vp->Height = height;
    vp->MinDepth = min_depth;
    vp->MaxDepth = max_depth;
}

static BOOL use_warp_adapter;
static unsigned int use_adapter_idx;

static IDXGIAdapter *create_adapter(void)
{
    IDXGIFactory4 *factory;
    IDXGIAdapter *adapter;
    HRESULT hr;

    if (!use_warp_adapter && !use_adapter_idx)
        return NULL;

    hr = CreateDXGIFactory2(0, &IID_IDXGIFactory4, (void **)&factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    adapter = NULL;
    if (use_warp_adapter)
    {
        hr = IDXGIFactory4_EnumWarpAdapter(factory, &IID_IDXGIAdapter, (void **)&adapter);
    }
    else
    {
        hr = IDXGIFactory4_EnumAdapters(factory, use_adapter_idx, &adapter);
    }
    IDXGIFactory4_Release(factory);
    if (FAILED(hr))
        trace("Failed to get adapter, hr %#lx.\n", hr);
    return adapter;
}

static ID3D12Device *create_device(void)
{
    IDXGIAdapter *adapter;
    ID3D12Device *device;
    HRESULT hr;

    adapter = create_adapter();
    hr = D3D12CreateDevice((IUnknown *)adapter, D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device, (void **)&device);
    if (adapter)
        IDXGIAdapter_Release(adapter);
    if (FAILED(hr))
        return NULL;

    return device;
}

static void print_adapter_info(void)
{
    DXGI_ADAPTER_DESC adapter_desc;
    IDXGIFactory4 *factory;
    IDXGIAdapter *adapter;
    ID3D12Device *device;
    HRESULT hr;
    LUID luid;

    if (!(device = create_device()))
        return;
    luid = ID3D12Device_GetAdapterLuid(device);
    ID3D12Device_Release(device);

    hr = CreateDXGIFactory2(0, &IID_IDXGIFactory4, (void **)&factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGIFactory4_EnumAdapterByLuid(factory, luid, &IID_IDXGIAdapter, (void **)&adapter);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IDXGIFactory4_Release(factory);

    if (FAILED(hr))
        return;

    hr = IDXGIAdapter_GetDesc(adapter, &adapter_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IDXGIAdapter_Release(adapter);

    trace("Adapter: %s, %04x:%04x.\n", wine_dbgstr_w(adapter_desc.Description),
            adapter_desc.VendorId, adapter_desc.DeviceId);
}

static ULONG get_refcount(void *iface)
{
    IUnknown *unknown = iface;
    IUnknown_AddRef(unknown);
    return IUnknown_Release(unknown);
}

#define check_interface(a, b, c) check_interface_(__LINE__, a, b, c)
static void check_interface_(unsigned int line, void *iface_ptr, REFIID iid, BOOL supported)
{
    IUnknown *iface = iface_ptr;
    HRESULT hr, expected_hr;
    IUnknown *unk;

    expected_hr = supported ? S_OK : E_NOINTERFACE;

    hr = IUnknown_QueryInterface(iface, iid, (void **)&unk);
    ok_(__FILE__, line)(hr == expected_hr, "Got unexpected hr %#lx, expected %#lx.\n", hr, expected_hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unk);
}

static HRESULT create_root_signature(ID3D12Device *device, const D3D12_ROOT_SIGNATURE_DESC *desc,
        ID3D12RootSignature **root_signature)
{
    ID3DBlob *blob;
    HRESULT hr;

    if (FAILED(hr = D3D12SerializeRootSignature(desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &blob, NULL)))
        return hr;

    hr = ID3D12Device_CreateRootSignature(device, 0, ID3D10Blob_GetBufferPointer(blob),
            ID3D10Blob_GetBufferSize(blob), &IID_ID3D12RootSignature, (void **)root_signature);
    ID3D10Blob_Release(blob);
    return hr;
}

static ID3D12RootSignature *create_default_root_signature(ID3D12Device *device)
{
    D3D12_ROOT_SIGNATURE_DESC root_signature_desc;
    ID3D12RootSignature *root_signature = NULL;
    D3D12_ROOT_PARAMETER root_parameters[1];
    HRESULT hr;

    root_parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    root_parameters[0].Constants.ShaderRegister = 0;
    root_parameters[0].Constants.RegisterSpace = 0;
    root_parameters[0].Constants.Num32BitValues = 4;
    root_parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    root_signature_desc.NumParameters = ARRAY_SIZE(root_parameters);
    root_signature_desc.pParameters = root_parameters;
    root_signature_desc.NumStaticSamplers = 0;
    root_signature_desc.pStaticSamplers = NULL;
    root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
    hr = create_root_signature(device, &root_signature_desc, &root_signature);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    return root_signature;
}

#define create_pipeline_state(a, b, c, d) create_pipeline_state_(__LINE__, a, b, c, d)
static ID3D12PipelineState *create_pipeline_state_(unsigned int line, ID3D12Device *device,
        ID3D12RootSignature *root_signature, DXGI_FORMAT rt_format, const D3D12_SHADER_BYTECODE *ps)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_state_desc;
    ID3D12PipelineState *pipeline_state;
    HRESULT hr;

    static const DWORD vs_code[] =
    {
#if 0
        void main(uint id : SV_VertexID, out float4 position : SV_Position)
        {
            float2 coords = float2((id << 1) & 2, id & 2);
            position = float4(coords * float2(2, -2) + float2(-1, 1), 0, 1);
        }
#endif
        0x43425844, 0xf900d25e, 0x68bfefa7, 0xa63ac0a7, 0xa476af7a, 0x00000001, 0x0000018c, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000006, 0x00000001, 0x00000000, 0x00000101, 0x565f5653, 0x65747265, 0x00444978,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000001, 0x00000003,
        0x00000000, 0x0000000f, 0x505f5653, 0x7469736f, 0x006e6f69, 0x58454853, 0x000000f0, 0x00010050,
        0x0000003c, 0x0100086a, 0x04000060, 0x00101012, 0x00000000, 0x00000006, 0x04000067, 0x001020f2,
        0x00000000, 0x00000001, 0x02000068, 0x00000001, 0x0b00008c, 0x00100012, 0x00000000, 0x00004001,
        0x00000001, 0x00004001, 0x00000001, 0x0010100a, 0x00000000, 0x00004001, 0x00000000, 0x07000001,
        0x00100042, 0x00000000, 0x0010100a, 0x00000000, 0x00004001, 0x00000002, 0x05000056, 0x00100032,
        0x00000000, 0x00100086, 0x00000000, 0x0f000032, 0x00102032, 0x00000000, 0x00100046, 0x00000000,
        0x00004002, 0x40000000, 0xc0000000, 0x00000000, 0x00000000, 0x00004002, 0xbf800000, 0x3f800000,
        0x00000000, 0x00000000, 0x08000036, 0x001020c2, 0x00000000, 0x00004002, 0x00000000, 0x00000000,
        0x00000000, 0x3f800000, 0x0100003e,
    };
    static const D3D12_SHADER_BYTECODE vs = {vs_code, sizeof(vs_code)};
    static const DWORD ps_code[] =
    {
#if 0
        void main(const in float4 position : SV_Position, out float4 target : SV_Target0)
        {
            target = float4(0.0f, 1.0f, 0.0f, 1.0f);
        }
#endif
        0x43425844, 0x8a4a8140, 0x5eba8e0b, 0x714e0791, 0xb4b8eed2, 0x00000001, 0x000000d8, 0x00000003,
        0x0000002c, 0x00000060, 0x00000094, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008, 0x00000020,
        0x00000000, 0x00000001, 0x00000003, 0x00000000, 0x0000000f, 0x505f5653, 0x7469736f, 0x006e6f69,
        0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
        0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x58454853, 0x0000003c, 0x00000050,
        0x0000000f, 0x0100086a, 0x03000065, 0x001020f2, 0x00000000, 0x08000036, 0x001020f2, 0x00000000,
        0x00004002, 0x00000000, 0x3f800000, 0x00000000, 0x3f800000, 0x0100003e,
    };
    static const D3D12_SHADER_BYTECODE default_ps = {ps_code, sizeof(ps_code)};

    if (!ps)
        ps = &default_ps;

    memset(&pipeline_state_desc, 0, sizeof(pipeline_state_desc));
    pipeline_state_desc.pRootSignature = root_signature;
    pipeline_state_desc.VS = vs;
    pipeline_state_desc.PS = *ps;
    pipeline_state_desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    pipeline_state_desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    pipeline_state_desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    pipeline_state_desc.SampleMask = ~(UINT)0;
    pipeline_state_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipeline_state_desc.NumRenderTargets = 1;
    pipeline_state_desc.RTVFormats[0] = rt_format;
    pipeline_state_desc.SampleDesc.Count = 1;
    hr = ID3D12Device_CreateGraphicsPipelineState(device, &pipeline_state_desc,
            &IID_ID3D12PipelineState, (void **)&pipeline_state);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create graphics pipeline state, hr %#lx.\n", hr);

    return pipeline_state;
}

#define create_command_queue(a, b) create_command_queue_(__LINE__, a, b)
static ID3D12CommandQueue *create_command_queue_(unsigned int line,
        ID3D12Device *device, D3D12_COMMAND_LIST_TYPE type)
{
    D3D12_COMMAND_QUEUE_DESC command_queue_desc;
    ID3D12CommandQueue *queue;
    HRESULT hr;

    command_queue_desc.Type = type;
    command_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    command_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    command_queue_desc.NodeMask = 0;
    hr = ID3D12Device_CreateCommandQueue(device, &command_queue_desc,
            &IID_ID3D12CommandQueue, (void **)&queue);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create command queue, hr %#lx.\n", hr);

    return queue;
}

struct test_context_desc
{
    BOOL no_render_target;
    BOOL no_pipeline;
    const D3D12_SHADER_BYTECODE *ps;
};

#define MAX_FRAME_COUNT 4

struct test_context
{
    ID3D12Device *device;

    ID3D12CommandQueue *queue;
    ID3D12CommandAllocator *allocator[MAX_FRAME_COUNT];
    ID3D12GraphicsCommandList *list[MAX_FRAME_COUNT];

    ID3D12DescriptorHeap *rtv_heap;
    D3D12_CPU_DESCRIPTOR_HANDLE rtv[MAX_FRAME_COUNT];
    ID3D12Resource *render_target[MAX_FRAME_COUNT];

    ID3D12RootSignature *root_signature;
    ID3D12PipelineState *pipeline_state;

    D3D12_VIEWPORT viewport;
    RECT scissor_rect;
};

#define reset_command_list(a, b) reset_command_list_(__LINE__, a, b)
static void reset_command_list_(unsigned int line, struct test_context *context, unsigned int index)
{
    HRESULT hr;

    assert(index < MAX_FRAME_COUNT);

    hr = ID3D12CommandAllocator_Reset(context->allocator[index]);
    ok_(__FILE__, line)(hr == S_OK, "Failed to reset command allocator, hr %#lx.\n", hr);
    hr = ID3D12GraphicsCommandList_Reset(context->list[index], context->allocator[index], NULL);
    ok_(__FILE__, line)(hr == S_OK, "Failed to reset command list, hr %#lx.\n", hr);
}

static void destroy_render_targets(struct test_context *context)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(context->render_target); ++i)
    {
        if (context->render_target[i])
        {
            ID3D12Resource_Release(context->render_target[i]);
            context->render_target[i] = NULL;
        }
    }
}

#define create_render_target(context) create_render_target_(__LINE__, context)
static void create_render_target_(unsigned int line, struct test_context *context)
{
    D3D12_HEAP_PROPERTIES heap_properties;
    D3D12_RESOURCE_DESC resource_desc;
    D3D12_CLEAR_VALUE clear_value;
    HRESULT hr;

    destroy_render_targets(context);

    memset(&heap_properties, 0, sizeof(heap_properties));
    heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;

    resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resource_desc.Alignment = 0;
    resource_desc.Width = 32;
    resource_desc.Height = 32;
    resource_desc.DepthOrArraySize = 1;
    resource_desc.MipLevels = 1;
    resource_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    resource_desc.SampleDesc.Count = 1;
    resource_desc.SampleDesc.Quality = 0;
    resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    clear_value.Format = resource_desc.Format;
    clear_value.Color[0] = 1.0f;
    clear_value.Color[1] = 1.0f;
    clear_value.Color[2] = 1.0f;
    clear_value.Color[3] = 1.0f;
    hr = ID3D12Device_CreateCommittedResource(context->device,
            &heap_properties, D3D12_HEAP_FLAG_NONE, &resource_desc,
            D3D12_RESOURCE_STATE_RENDER_TARGET, &clear_value,
            &IID_ID3D12Resource, (void **)&context->render_target[0]);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create texture, hr %#lx.\n", hr);

    set_viewport(&context->viewport, 0.0f, 0.0f, resource_desc.Width, resource_desc.Height, 0.0f, 1.0f);
    SetRect(&context->scissor_rect, 0, 0, resource_desc.Width, resource_desc.Height);

    ID3D12Device_CreateRenderTargetView(context->device, context->render_target[0], NULL, context->rtv[0]);
}

#define init_test_context(a, b) init_test_context_(__LINE__, a, b)
static BOOL init_test_context_(unsigned int line, struct test_context *context,
        const struct test_context_desc *desc)
{
    D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc;
    unsigned int rtv_size;
    ID3D12Device *device;
    unsigned int i;
    HRESULT hr;

    memset(context, 0, sizeof(*context));

    if (!(context->device = create_device()))
    {
        skip_(__FILE__, line)("Failed to create device.\n");
        return FALSE;
    }
    device = context->device;

    context->queue = create_command_queue_(line, device, D3D12_COMMAND_LIST_TYPE_DIRECT);

    for (i = 0; i < ARRAY_SIZE(context->allocator); ++i)
    {
        hr = ID3D12Device_CreateCommandAllocator(device, D3D12_COMMAND_LIST_TYPE_DIRECT,
                &IID_ID3D12CommandAllocator, (void **)&context->allocator[i]);
        ok_(__FILE__, line)(hr == S_OK, "Failed to create command allocator %u, hr %#lx.\n", i, hr);
    }

    for (i = 0; i < ARRAY_SIZE(context->list); ++i)
    {
        hr = ID3D12Device_CreateCommandList(device, 0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                context->allocator[i], NULL, &IID_ID3D12GraphicsCommandList, (void **)&context->list[i]);
        ok_(__FILE__, line)(hr == S_OK, "Failed to create command list %u, hr %#lx.\n", i, hr);
    }

    if (desc && desc->no_render_target)
        return TRUE;

    rtv_heap_desc.NumDescriptors = MAX_FRAME_COUNT;
    rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtv_heap_desc.NodeMask = 0;
    hr = ID3D12Device_CreateDescriptorHeap(device, &rtv_heap_desc,
            &IID_ID3D12DescriptorHeap, (void **)&context->rtv_heap);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create descriptor heap, hr %#lx.\n", hr);

    rtv_size = ID3D12Device_GetDescriptorHandleIncrementSize(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    for (i = 0; i < ARRAY_SIZE(context->rtv); ++i)
    {
        context->rtv[i] = ID3D12DescriptorHeap_GetCPUDescriptorHandleForHeapStart(context->rtv_heap);
        context->rtv[i].ptr += i * rtv_size;
    }

    context->root_signature = create_default_root_signature(device);

    if (desc && desc->no_pipeline)
        return TRUE;

    context->pipeline_state = create_pipeline_state_(line, device,
            context->root_signature, DXGI_FORMAT_B8G8R8A8_UNORM, desc ? desc->ps : NULL);

    return TRUE;
}

#define destroy_test_context(context) destroy_test_context_(__LINE__, context)
static void destroy_test_context_(unsigned int line, struct test_context *context)
{
    unsigned int i;
    ULONG refcount;

    if (context->pipeline_state)
        ID3D12PipelineState_Release(context->pipeline_state);
    if (context->root_signature)
        ID3D12RootSignature_Release(context->root_signature);

    if (context->rtv_heap)
        ID3D12DescriptorHeap_Release(context->rtv_heap);
    destroy_render_targets(context);

    for (i = 0; i < ARRAY_SIZE(context->allocator); ++i)
        ID3D12CommandAllocator_Release(context->allocator[i]);
    ID3D12CommandQueue_Release(context->queue);
    for (i = 0; i < ARRAY_SIZE(context->list); ++i)
        ID3D12GraphicsCommandList_Release(context->list[i]);

    refcount = ID3D12Device_Release(context->device);
    ok_(__FILE__, line)(!refcount, "ID3D12Device has %u references left.\n", (unsigned int)refcount);
}

static void exec_command_list(ID3D12CommandQueue *queue, ID3D12GraphicsCommandList *list)
{
    ID3D12CommandList *lists[] = {(ID3D12CommandList *)list};
    ID3D12CommandQueue_ExecuteCommandLists(queue, 1, lists);
}

static HRESULT wait_for_fence(ID3D12Fence *fence, UINT64 value)
{
    HANDLE event;
    HRESULT hr;
    DWORD ret;

    if (ID3D12Fence_GetCompletedValue(fence) >= value)
        return S_OK;

    if (!(event = CreateEventA(NULL, FALSE, FALSE, NULL)))
        return E_FAIL;

    if (FAILED(hr = ID3D12Fence_SetEventOnCompletion(fence, value, event)))
    {
        CloseHandle(event);
        return hr;
    }

    ret = WaitForSingleObject(event, INFINITE);
    CloseHandle(event);
    return ret == WAIT_OBJECT_0 ? S_OK : E_FAIL;
}

#define wait_queue_idle(a, b) wait_queue_idle_(__LINE__, a, b)
static void wait_queue_idle_(unsigned int line, ID3D12Device *device, ID3D12CommandQueue *queue)
{
    ID3D12Fence *fence;
    HRESULT hr;

    hr = ID3D12Device_CreateFence(device, 0, D3D12_FENCE_FLAG_NONE,
            &IID_ID3D12Fence, (void **)&fence);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create fence, hr %#lx.\n", hr);

    hr = ID3D12CommandQueue_Signal(queue, fence, 1);
    ok_(__FILE__, line)(hr == S_OK, "Failed to signal fence, hr %#lx.\n", hr);
    hr = wait_for_fence(fence, 1);
    ok_(__FILE__, line)(hr == S_OK, "Failed to wait for fence, hr %#lx.\n", hr);

    ID3D12Fence_Release(fence);
}

static void transition_sub_resource_state(ID3D12GraphicsCommandList *list, ID3D12Resource *resource,
        unsigned int sub_resource_idx, D3D12_RESOURCE_STATES state_before, D3D12_RESOURCE_STATES state_after)
{
    D3D12_RESOURCE_BARRIER barrier;

    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource;
    barrier.Transition.Subresource = sub_resource_idx;
    barrier.Transition.StateBefore = state_before;
    barrier.Transition.StateAfter = state_after;

    ID3D12GraphicsCommandList_ResourceBarrier(list, 1, &barrier);
}

#define create_buffer(a, b, c, d, e) create_buffer_(__LINE__, a, b, c, d, e)
static ID3D12Resource *create_buffer_(unsigned int line, ID3D12Device *device,
        D3D12_HEAP_TYPE heap_type, size_t size, D3D12_RESOURCE_FLAGS resource_flags,
        D3D12_RESOURCE_STATES initial_resource_state)
{
    D3D12_HEAP_PROPERTIES heap_properties;
    D3D12_RESOURCE_DESC resource_desc;
    ID3D12Resource *buffer;
    HRESULT hr;

    memset(&heap_properties, 0, sizeof(heap_properties));
    heap_properties.Type = heap_type;

    resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resource_desc.Alignment = 0;
    resource_desc.Width = size;
    resource_desc.Height = 1;
    resource_desc.DepthOrArraySize = 1;
    resource_desc.MipLevels = 1;
    resource_desc.Format = DXGI_FORMAT_UNKNOWN;
    resource_desc.SampleDesc.Count = 1;
    resource_desc.SampleDesc.Quality = 0;
    resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resource_desc.Flags = resource_flags;

    hr = ID3D12Device_CreateCommittedResource(device, &heap_properties,
            D3D12_HEAP_FLAG_NONE, &resource_desc, initial_resource_state,
            NULL, &IID_ID3D12Resource, (void **)&buffer);
    ok_(__FILE__, line)(hr == S_OK, "Failed to create buffer, hr %#lx.\n", hr);
    return buffer;
}

#define create_readback_buffer(a, b) create_readback_buffer_(__LINE__, a, b)
static ID3D12Resource *create_readback_buffer_(unsigned int line, ID3D12Device *device,
        size_t size)
{
    return create_buffer_(line, device, D3D12_HEAP_TYPE_READBACK, size,
            D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
}

static HWND create_window(DWORD style)
{
    return CreateWindowA("static", "d3d12_test", style, 0, 0, 256, 256, NULL, NULL, NULL, NULL);
}

static IDXGISwapChain3 *create_swapchain(struct test_context *context, ID3D12CommandQueue *queue,
        HWND window, unsigned int buffer_count, DXGI_FORMAT format, unsigned int width, unsigned int height)
{
    IDXGISwapChain1 *swapchain1;
    DXGI_SWAP_CHAIN_DESC1 desc;
    IDXGISwapChain3 *swapchain;
    IDXGIFactory4 *factory;
    unsigned int i;
    HRESULT hr;

    assert(buffer_count <= MAX_FRAME_COUNT);

    if (context)
        destroy_render_targets(context);

    hr = CreateDXGIFactory2(0, &IID_IDXGIFactory4, (void **)&factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    desc.Width = width;
    desc.Height = height;
    desc.Format = format;
    desc.Stereo = FALSE;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = buffer_count;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    desc.Flags = 0;
    hr = IDXGIFactory4_CreateSwapChainForHwnd(factory, (IUnknown *)queue, window, &desc, NULL, NULL, &swapchain1);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    IDXGIFactory4_Release(factory);

    hr = IDXGISwapChain1_QueryInterface(swapchain1, &IID_IDXGISwapChain3, (void **)&swapchain);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IDXGISwapChain1_Release(swapchain1);

    if (context)
    {
        set_viewport(&context->viewport, 0.0f, 0.0f, width, height, 0.0f, 1.0f);
        SetRect(&context->scissor_rect, 0, 0, width, height);

        for (i = 0; i < buffer_count; ++i)
        {
            hr = IDXGISwapChain3_GetBuffer(swapchain, i, &IID_ID3D12Resource, (void **)&context->render_target[i]);
            ok(hr == S_OK, "Failed to get swapchain buffer %u, hr %#lx.\n", i, hr);
            ID3D12Device_CreateRenderTargetView(context->device, context->render_target[i], NULL, context->rtv[i]);
        }
    }

    return swapchain;
}

struct resource_readback
{
    unsigned int width;
    unsigned int height;
    ID3D12Resource *resource;
    unsigned int row_pitch;
    void *data;
};

static void get_texture_readback_with_command_list(ID3D12Resource *texture, unsigned int sub_resource,
        struct resource_readback *rb, ID3D12CommandQueue *queue, ID3D12GraphicsCommandList *command_list)
{
    D3D12_TEXTURE_COPY_LOCATION dst_location, src_location;
    D3D12_RESOURCE_DESC resource_desc;
    D3D12_RANGE read_range;
    unsigned int miplevel;
    ID3D12Device *device;
    DXGI_FORMAT format;
    HRESULT hr;

    hr = ID3D12Resource_GetDevice(texture, &IID_ID3D12Device, (void **)&device);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    resource_desc = ID3D12Resource_GetDesc(texture);
    ok(resource_desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER,
            "Resource %p is not texture.\n", texture);
    ok(resource_desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE3D,
            "Readback not implemented for 3D textures.\n");

    miplevel = sub_resource % resource_desc.MipLevels;
    rb->width = max(1, resource_desc.Width >> miplevel);
    rb->height = max(1, resource_desc.Height >> miplevel);
    rb->row_pitch = align(rb->width * format_size(resource_desc.Format), D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
    rb->data = NULL;

    format = resource_desc.Format;

    rb->resource = create_readback_buffer(device, rb->row_pitch * rb->height);

    dst_location.pResource = rb->resource;
    dst_location.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dst_location.PlacedFootprint.Offset = 0;
    dst_location.PlacedFootprint.Footprint.Format = format;
    dst_location.PlacedFootprint.Footprint.Width = rb->width;
    dst_location.PlacedFootprint.Footprint.Height = rb->height;
    dst_location.PlacedFootprint.Footprint.Depth = 1;
    dst_location.PlacedFootprint.Footprint.RowPitch = rb->row_pitch;

    src_location.pResource = texture;
    src_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    src_location.SubresourceIndex = sub_resource;

    ID3D12GraphicsCommandList_CopyTextureRegion(command_list, &dst_location, 0, 0, 0, &src_location, NULL);
    hr = ID3D12GraphicsCommandList_Close(command_list);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    exec_command_list(queue, command_list);
    wait_queue_idle(device, queue);
    ID3D12Device_Release(device);

    read_range.Begin = 0;
    read_range.End = resource_desc.Width;
    hr = ID3D12Resource_Map(rb->resource, 0, &read_range, &rb->data);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
}

static void *get_readback_data(struct resource_readback *rb, unsigned int x, unsigned int y,
        size_t element_size)
{
    return &((BYTE *)rb->data)[rb->row_pitch * y + x * element_size];
}

static unsigned int get_readback_uint(struct resource_readback *rb, unsigned int x, unsigned int y)
{
    return *(unsigned int *)get_readback_data(rb, x, y, sizeof(unsigned int));
}

static void release_resource_readback(struct resource_readback *rb)
{
    D3D12_RANGE range = {0, 0};
    ID3D12Resource_Unmap(rb->resource, 0, &range);
    ID3D12Resource_Release(rb->resource);
}

#define check_readback_data_uint(a, b, c, d) check_readback_data_uint_(__LINE__, a, b, c, d)
static void check_readback_data_uint_(unsigned int line, struct resource_readback *rb,
        const RECT *rect, unsigned int expected, unsigned int max_diff)
{
    RECT r = {0, 0, rb->width, rb->height};
    unsigned int x = 0, y;
    BOOL all_match = TRUE;
    unsigned int got = 0;

    if (rect)
        r = *rect;

    for (y = r.top; y < r.bottom; ++y)
    {
        for (x = r.left; x < r.right; ++x)
        {
            got = get_readback_uint(rb, x, y);
            if (!compare_color(got, expected, max_diff))
            {
                all_match = FALSE;
                break;
            }
        }
        if (!all_match)
            break;
    }
    ok_(__FILE__, line)(all_match, "Got 0x%08x, expected 0x%08x at (%u, %u).\n", got, expected, x, y);
}

#define check_sub_resource_uint(a, b, c, d, e, f) check_sub_resource_uint_(__LINE__, a, b, c, d, e, f)
static void check_sub_resource_uint_(unsigned int line, ID3D12Resource *texture,
        unsigned int sub_resource_idx, ID3D12CommandQueue *queue, ID3D12GraphicsCommandList *command_list,
        unsigned int expected, unsigned int max_diff)
{
    struct resource_readback rb;

    get_texture_readback_with_command_list(texture, 0, &rb, queue, command_list);
    check_readback_data_uint_(line, &rb, NULL, expected, max_diff);
    release_resource_readback(&rb);
}

static void test_ordinals(void)
{
    PFN_D3D12_CREATE_DEVICE pfn_D3D12CreateDevice, pfn_101;
    HMODULE d3d12;

    d3d12 = GetModuleHandleA("d3d12.dll");
    ok(!!d3d12, "Failed to get module handle.\n");

    pfn_D3D12CreateDevice = (void *)GetProcAddress(d3d12, "D3D12CreateDevice");
    ok(!!pfn_D3D12CreateDevice, "Failed to get D3D12CreateDevice() proc address.\n");

    pfn_101 = (void *)GetProcAddress(d3d12, (const char *)101);
    ok(pfn_101 == pfn_D3D12CreateDevice, "Got %p, expected %p.\n", pfn_101, pfn_D3D12CreateDevice);
}

static void test_interfaces(void)
{
    D3D12_COMMAND_QUEUE_DESC desc;
    ID3D12CommandQueue *queue;
    ID3D12Device *device;
    ULONG refcount;
    HRESULT hr;

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }

    check_interface(device, &IID_ID3D12Object, TRUE);
    check_interface(device, &IID_ID3D12DeviceChild, FALSE);
    check_interface(device, &IID_ID3D12Pageable, FALSE);
    check_interface(device, &IID_ID3D12Device, TRUE);

    check_interface(device, &IID_IDXGIObject, FALSE);
    check_interface(device, &IID_IDXGIDeviceSubObject, FALSE);
    check_interface(device, &IID_IDXGIDevice, FALSE);
    check_interface(device, &IID_IDXGIDevice1, FALSE);
    check_interface(device, &IID_IDXGIDevice2, FALSE);
    check_interface(device, &IID_IDXGIDevice3, FALSE);
    check_interface(device, &IID_IDXGIDevice4, FALSE);

    desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;
    hr = ID3D12Device_CreateCommandQueue(device, &desc, &IID_ID3D12CommandQueue, (void **)&queue);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    check_interface(queue, &IID_ID3D12Object, TRUE);
    check_interface(queue, &IID_ID3D12DeviceChild, TRUE);
    check_interface(queue, &IID_ID3D12Pageable, TRUE);
    check_interface(queue, &IID_ID3D12CommandQueue, TRUE);

    check_interface(queue, &IID_IDXGIObject, FALSE);
    check_interface(queue, &IID_IDXGIDeviceSubObject, FALSE);
    check_interface(queue, &IID_IDXGIDevice, FALSE);
    check_interface(queue, &IID_IDXGIDevice1, FALSE);
    check_interface(queue, &IID_IDXGIDevice2, FALSE);
    check_interface(queue, &IID_IDXGIDevice3, FALSE);
    check_interface(queue, &IID_IDXGIDevice4, FALSE);

    refcount = ID3D12CommandQueue_Release(queue);
    ok(!refcount, "Command queue has %lu references left.\n", refcount);
    refcount = ID3D12Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

static void test_create_device(void)
{
    DXGI_ADAPTER_DESC adapter_desc;
    IDXGISwapChain3 *swapchain;
    ID3D12CommandQueue *queue;
    LUID adapter_luid, luid;
    IDXGIFactory4 *factory;
    IDXGIAdapter *adapter;
    ID3D12Device *device;
    IDXGIOutput *output;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    RECT rect;
    BOOL ret;

    if (!(device = create_device()))
    {
        skip("Failed to create device.\n");
        return;
    }
    refcount = ID3D12Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);

    hr = CreateDXGIFactory2(0, &IID_IDXGIFactory4, (void **)&factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = IDXGIFactory4_EnumAdapters(factory, 0, &adapter);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IDXGIFactory4_Release(factory);

    hr = IDXGIAdapter_GetDesc(adapter, &adapter_desc);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    adapter_luid = adapter_desc.AdapterLuid;

    refcount = get_refcount(adapter);
    ok(refcount == 1, "Got unexpected refcount %lu.\n", refcount);
    hr = D3D12CreateDevice((IUnknown *)adapter, D3D_FEATURE_LEVEL_11_0, &IID_ID3D12Device, (void **)&device);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    IDXGIAdapter_Release(adapter);
    adapter = NULL;

    luid = ID3D12Device_GetAdapterLuid(device);
    ok(equal_luid(luid, adapter_luid), "Got unexpected LUID %08lx:%08lx, expected %08lx:%08lx.\n",
            luid.HighPart, luid.LowPart, adapter_luid.HighPart, adapter_luid.LowPart);

    queue = create_command_queue(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    window = create_window(WS_VISIBLE);
    ret = GetClientRect(window, &rect);
    ok(ret, "Failed to get client rect.\n");
    swapchain = create_swapchain(NULL, queue, window, 2, DXGI_FORMAT_B8G8R8A8_UNORM, rect.right, rect.bottom);

    hr = IDXGISwapChain3_GetContainingOutput(swapchain, &output);
    if (hr != DXGI_ERROR_UNSUPPORTED)
    {
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = IDXGIOutput_GetParent(output, &IID_IDXGIAdapter, (void **)&adapter);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        IDXGIOutput_Release(output);

        memset(&adapter_desc, 0, sizeof(adapter_desc));
        hr = IDXGIAdapter_GetDesc(adapter, &adapter_desc);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        IDXGIAdapter_Release(adapter);

        ok(equal_luid(adapter_desc.AdapterLuid, adapter_luid),
                "Got unexpected LUID %08lx:%08lx, expected %08lx:%08lx.\n",
                adapter_desc.AdapterLuid.HighPart, adapter_desc.AdapterLuid.LowPart,
                adapter_luid.HighPart, adapter_luid.LowPart);
    }
    else
    {
        skip("GetContainingOutput() is not supported.\n");
    }

    refcount = IDXGISwapChain3_Release(swapchain);
    ok(!refcount, "Swapchain has %lu references left.\n", refcount);
    DestroyWindow(window);
    ID3D12CommandQueue_Release(queue);

    refcount = ID3D12Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

static void test_draw(void)
{
    static const float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
    ID3D12GraphicsCommandList *command_list;
    struct test_context context;
    ID3D12CommandQueue *queue;

    if (!init_test_context(&context, NULL))
        return;
    command_list = context.list[0];
    queue = context.queue;

    create_render_target(&context);

    ID3D12GraphicsCommandList_ClearRenderTargetView(command_list, context.rtv[0], white, 0, NULL);

    ID3D12GraphicsCommandList_OMSetRenderTargets(command_list, 1, &context.rtv[0], FALSE, NULL);
    ID3D12GraphicsCommandList_SetGraphicsRootSignature(command_list, context.root_signature);
    ID3D12GraphicsCommandList_SetPipelineState(command_list, context.pipeline_state);
    ID3D12GraphicsCommandList_IASetPrimitiveTopology(command_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D12GraphicsCommandList_RSSetViewports(command_list, 1, &context.viewport);
    ID3D12GraphicsCommandList_RSSetScissorRects(command_list, 1, &context.scissor_rect);
    ID3D12GraphicsCommandList_DrawInstanced(command_list, 3, 1, 0, 0);

    transition_sub_resource_state(command_list, context.render_target[0], 0,
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);

    check_sub_resource_uint(context.render_target[0], 0, queue, command_list, 0xff00ff00, 0);

    destroy_test_context(&context);
}

static void test_swapchain_draw(void)
{
    static const float white[] = {1.0f, 1.0f, 1.0f, 1.0f};
    ID3D12GraphicsCommandList *command_list;
    D3D12_CPU_DESCRIPTOR_HANDLE rtv;
    struct test_context_desc desc;
    struct test_context context;
    ID3D12Resource *backbuffer;
    IDXGISwapChain3 *swapchain;
    ID3D12CommandQueue *queue;
    unsigned int i, index;
    ID3D12Device *device;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    RECT rect;
    BOOL ret;

    static const DWORD ps_code[] =
    {
#if 0
        float4 color;

        float4 main() : SV_Target
        {
            return color;
        }
#endif
        0x43425844, 0x69e703c1, 0xf0db50aa, 0x9af7ae76, 0x623b93f7, 0x00000001, 0x000000bc, 0x00000003,
        0x0000002c, 0x0000003c, 0x00000070, 0x4e475349, 0x00000008, 0x00000000, 0x00000008, 0x4e47534f,
        0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
        0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x58454853, 0x00000044, 0x00000050, 0x00000011,
        0x0100086a, 0x04000059, 0x00208e46, 0x00000000, 0x00000001, 0x03000065, 0x001020f2, 0x00000000,
        0x06000036, 0x001020f2, 0x00000000, 0x00208e46, 0x00000000, 0x00000000, 0x0100003e,
    };
    static const D3D12_SHADER_BYTECODE ps = {ps_code, sizeof(ps_code)};

    static const struct
    {
        DXGI_FORMAT format;
        float input[4];
        unsigned int color;
    }
    tests[] =
    {
        {DXGI_FORMAT_B8G8R8A8_UNORM,    {1.0f, 0.0f, 0.0f, 1.0f}, 0xffff0000},
        {DXGI_FORMAT_B8G8R8A8_UNORM,    {0.0f, 1.0f, 0.0f, 1.0f}, 0xff00ff00},
        {DXGI_FORMAT_R8G8B8A8_UNORM,    {1.0f, 0.0f, 0.0f, 1.0f}, 0xff0000ff},
        {DXGI_FORMAT_R8G8B8A8_UNORM,    {0.0f, 1.0f, 0.0f, 1.0f}, 0xff00ff00},
        {DXGI_FORMAT_R10G10B10A2_UNORM, {1.0f, 0.0f, 0.0f, 1.0f}, 0xc00003ff},
        {DXGI_FORMAT_R10G10B10A2_UNORM, {0.0f, 1.0f, 0.0f, 1.0f}, 0xc00ffc00},
    };

    memset(&desc, 0, sizeof(desc));
    desc.no_pipeline = TRUE;
    if (!init_test_context(&context, &desc))
        return;
    device = context.device;
    command_list = context.list[0];
    queue = context.queue;

    window = create_window(WS_VISIBLE);
    ret = GetClientRect(window, &rect);
    ok(ret, "Failed to get client rect.\n");

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        context.pipeline_state = create_pipeline_state(device,
                context.root_signature, tests[i].format, &ps);

        swapchain = create_swapchain(&context, queue, window, 2, tests[i].format, rect.right, rect.bottom);
        index = IDXGISwapChain3_GetCurrentBackBufferIndex(swapchain);
        backbuffer = context.render_target[index];
        rtv = context.rtv[index];

        transition_sub_resource_state(command_list, backbuffer, 0,
                D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

        ID3D12GraphicsCommandList_ClearRenderTargetView(command_list, rtv, white, 0, NULL);

        ID3D12GraphicsCommandList_OMSetRenderTargets(command_list, 1, &rtv, FALSE, NULL);
        ID3D12GraphicsCommandList_SetGraphicsRootSignature(command_list, context.root_signature);
        ID3D12GraphicsCommandList_SetPipelineState(command_list, context.pipeline_state);
        ID3D12GraphicsCommandList_IASetPrimitiveTopology(command_list, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ID3D12GraphicsCommandList_RSSetViewports(command_list, 1, &context.viewport);
        ID3D12GraphicsCommandList_RSSetScissorRects(command_list, 1, &rect);
        ID3D12GraphicsCommandList_SetGraphicsRoot32BitConstants(command_list, 0, 4, tests[i].input, 0);
        ID3D12GraphicsCommandList_DrawInstanced(command_list, 3, 1, 0, 0);

        transition_sub_resource_state(command_list, backbuffer, 0,
                D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
        check_sub_resource_uint(backbuffer, 0, queue, command_list, tests[i].color, 0);

        reset_command_list(&context, 0);
        transition_sub_resource_state(command_list, backbuffer, 0,
                D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PRESENT);
        hr = ID3D12GraphicsCommandList_Close(command_list);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        exec_command_list(queue, command_list);

        hr = IDXGISwapChain3_Present(swapchain, 0, 0);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        wait_queue_idle(device, queue);

        destroy_render_targets(&context);
        refcount = IDXGISwapChain3_Release(swapchain);
        ok(!refcount, "Swapchain has %lu references left.\n", refcount);
        ID3D12PipelineState_Release(context.pipeline_state);
        context.pipeline_state = NULL;

        reset_command_list(&context, 0);
    }

    DestroyWindow(window);
    destroy_test_context(&context);
}

static void test_swapchain_refcount(void)
{
    const unsigned int buffer_count = 4;
    struct test_context_desc desc;
    struct test_context context;
    IDXGISwapChain3 *swapchain;
    unsigned int i;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    RECT rect;
    BOOL ret;

    memset(&desc, 0, sizeof(desc));
    desc.no_pipeline = TRUE;
    if (!init_test_context(&context, &desc))
        return;

    window = create_window(WS_VISIBLE);
    ret = GetClientRect(window, &rect);
    ok(ret, "Failed to get client rect.\n");
    swapchain = create_swapchain(&context, context.queue, window,
            buffer_count, DXGI_FORMAT_B8G8R8A8_UNORM, rect.right, rect.bottom);

    for (i = 0; i < buffer_count; ++i)
    {
        refcount = get_refcount(swapchain);
        todo_wine ok(refcount == 2, "Got refcount %lu.\n", refcount);
        ID3D12Resource_Release(context.render_target[i]);
        context.render_target[i] = NULL;
    }
    refcount = get_refcount(swapchain);
    ok(refcount == 1, "Got refcount %lu.\n", refcount);

    refcount = IDXGISwapChain3_AddRef(swapchain);
    ok(refcount == 2, "Got refcount %lu.\n", refcount);
    hr = IDXGISwapChain3_GetBuffer(swapchain, 0, &IID_ID3D12Resource, (void **)&context.render_target[0]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    refcount = get_refcount(swapchain);
    todo_wine ok(refcount == 3, "Got refcount %lu.\n", refcount);

    refcount = ID3D12Resource_AddRef(context.render_target[0]);
    ok(refcount == 2, "Got refcount %lu.\n", refcount);
    refcount = get_refcount(swapchain);
    todo_wine ok(refcount == 3, "Got refcount %lu.\n", refcount);

    hr = IDXGISwapChain3_GetBuffer(swapchain, 1, &IID_ID3D12Resource, (void **)&context.render_target[1]);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    refcount = get_refcount(swapchain);
    todo_wine ok(refcount == 3, "Got refcount %lu.\n", refcount);

    ID3D12Resource_Release(context.render_target[0]);
    ID3D12Resource_Release(context.render_target[0]);
    context.render_target[0] = NULL;
    refcount = get_refcount(swapchain);
    todo_wine ok(refcount == 3, "Got refcount %lu.\n", refcount);

    refcount = IDXGISwapChain3_Release(swapchain);
    todo_wine ok(refcount == 2, "Got refcount %lu.\n", refcount);
    ID3D12Resource_Release(context.render_target[1]);
    context.render_target[1] = NULL;
    refcount = get_refcount(swapchain);
    ok(refcount == 1, "Got refcount %lu.\n", refcount);

    refcount = IDXGISwapChain3_Release(swapchain);
    ok(!refcount, "Swapchain has %lu references left.\n", refcount);
    DestroyWindow(window);
    destroy_test_context(&context);
}

static void test_swapchain_size_mismatch(void)
{
    static const float green[] = {0.0f, 1.0f, 0.0f, 1.0f};
    UINT64 frame_fence_value[MAX_FRAME_COUNT] = {0};
    ID3D12GraphicsCommandList *command_list;
    D3D12_CPU_DESCRIPTOR_HANDLE rtv;
    struct test_context_desc desc;
    struct test_context context;
    ID3D12Resource *backbuffer;
    IDXGISwapChain3 *swapchain;
    ID3D12CommandQueue *queue;
    unsigned int index, i;
    ID3D12Device *device;
    ID3D12Fence *fence;
    UINT64 fence_value;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    RECT rect;
    BOOL ret;

    memset(&desc, 0, sizeof(desc));
    desc.no_pipeline = TRUE;
    if (!init_test_context(&context, &desc))
        return;
    device = context.device;
    command_list = context.list[0];
    queue = context.queue;

    window = CreateWindowA("static", "d3d12_test", WS_VISIBLE, 0, 0, 200, 200, NULL, NULL, NULL, NULL);
    swapchain = create_swapchain(&context, queue, window, 2, DXGI_FORMAT_B8G8R8A8_UNORM, 400, 400);
    index = IDXGISwapChain3_GetCurrentBackBufferIndex(swapchain);
    backbuffer = context.render_target[index];
    rtv = context.rtv[index];

    transition_sub_resource_state(command_list, backbuffer, 0,
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    ID3D12GraphicsCommandList_ClearRenderTargetView(command_list, rtv, green, 0, NULL);
    transition_sub_resource_state(command_list, backbuffer, 0,
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
    check_sub_resource_uint(backbuffer, 0, queue, command_list, 0xff00ff00, 0);

    reset_command_list(&context, 0);
    transition_sub_resource_state(command_list, backbuffer, 0,
            D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PRESENT);
    hr = ID3D12GraphicsCommandList_Close(command_list);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    exec_command_list(queue, command_list);

    hr = IDXGISwapChain3_Present(swapchain, 1, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    wait_queue_idle(device, queue);
    reset_command_list(&context, 0);

    destroy_render_targets(&context);
    refcount = IDXGISwapChain3_Release(swapchain);
    ok(!refcount, "Swapchain has %lu references left.\n", refcount);
    DestroyWindow(window);

    window = create_window(WS_VISIBLE);
    ret = GetClientRect(window, &rect);
    ok(ret, "Failed to get client rect.\n");
    swapchain = create_swapchain(&context, queue, window, 4, DXGI_FORMAT_B8G8R8A8_UNORM, rect.right, rect.bottom);

    hr = ID3D12Device_CreateFence(device, 0, D3D12_FENCE_FLAG_NONE, &IID_ID3D12Fence, (void **)&fence);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(context.list); ++i)
    {
        hr = ID3D12GraphicsCommandList_Close(context.list[i]);
        ok(hr == S_OK, "Failed to close command list %u, hr %#lx.\n", i, hr);
    }

    fence_value = 1;
    for (i = 0; i < 20; ++i)
    {
        index = IDXGISwapChain3_GetCurrentBackBufferIndex(swapchain);

        hr = wait_for_fence(fence, frame_fence_value[index]);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        reset_command_list(&context, index);
        backbuffer = context.render_target[index];
        command_list = context.list[index];
        rtv = context.rtv[index];

        transition_sub_resource_state(command_list, backbuffer, 0,
                D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        ID3D12GraphicsCommandList_ClearRenderTargetView(command_list, rtv, green, 0, NULL);
        transition_sub_resource_state(command_list, backbuffer, 0,
                D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        hr = ID3D12GraphicsCommandList_Close(command_list);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        exec_command_list(queue, command_list);

        hr = IDXGISwapChain3_Present(swapchain, 1, 0);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        if (i == 6)
        {
            wait_queue_idle(device, queue);
            MoveWindow(window, 0, 0, 100, 100, TRUE);
        }

        frame_fence_value[index] = fence_value;
        hr = ID3D12CommandQueue_Signal(queue, fence, fence_value);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ++fence_value;
    }

    wait_queue_idle(device, queue);

    ID3D12Fence_Release(fence);
    destroy_render_targets(&context);
    refcount = IDXGISwapChain3_Release(swapchain);
    ok(!refcount, "Swapchain has %lu references left.\n", refcount);
    DestroyWindow(window);
    destroy_test_context(&context);
}

static void test_swapchain_backbuffer_index(void)
{
    static const float green[] = {0.0f, 1.0f, 0.0f, 1.0f};
    UINT64 frame_fence_value[MAX_FRAME_COUNT] = {0};
    ID3D12GraphicsCommandList *command_list;
    unsigned int expected_index, index, i;
    const unsigned int buffer_count = 2;
    D3D12_CPU_DESCRIPTOR_HANDLE rtv;
    struct test_context_desc desc;
    struct test_context context;
    ID3D12Resource *backbuffer;
    unsigned int sync_interval;
    IDXGISwapChain3 *swapchain;
    ID3D12CommandQueue *queue;
    ID3D12Device *device;
    ID3D12Fence *fence;
    UINT64 fence_value;
    ULONG refcount;
    HWND window;
    HRESULT hr;
    RECT rect;
    BOOL ret;

    memset(&desc, 0, sizeof(desc));
    desc.no_pipeline = TRUE;
    if (!init_test_context(&context, &desc))
        return;
    device = context.device;
    queue = context.queue;

    window = create_window(WS_VISIBLE);
    ret = GetClientRect(window, &rect);
    ok(ret, "Failed to get client rect.\n");
    swapchain = create_swapchain(&context, queue, window,
            buffer_count, DXGI_FORMAT_B8G8R8A8_UNORM, rect.right, rect.bottom);

    hr = ID3D12Device_CreateFence(device, 0, D3D12_FENCE_FLAG_NONE, &IID_ID3D12Fence, (void **)&fence);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(context.list); ++i)
    {
        hr = ID3D12GraphicsCommandList_Close(context.list[i]);
        ok(hr == S_OK, "Failed to close command list %u, hr %#lx.\n", i, hr);
    }

    index = 1;
    fence_value = 1;
    for (i = 0; i < 20; ++i)
    {
        expected_index = (index + 1) % buffer_count;
        index = IDXGISwapChain3_GetCurrentBackBufferIndex(swapchain);
        ok(index == expected_index, "Test %u: Got index %u, expected %u.\n", i, index, expected_index);

        hr = wait_for_fence(fence, frame_fence_value[index]);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        reset_command_list(&context, index);
        backbuffer = context.render_target[index];
        command_list = context.list[index];
        rtv = context.rtv[index];

        transition_sub_resource_state(command_list, backbuffer, 0,
                D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        ID3D12GraphicsCommandList_ClearRenderTargetView(command_list, rtv, green, 0, NULL);
        transition_sub_resource_state(command_list, backbuffer, 0,
                D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        hr = ID3D12GraphicsCommandList_Close(command_list);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        exec_command_list(queue, command_list);

        if (i <= 4 || (8 <= i && i <= 14))
            sync_interval = 1;
        else
            sync_interval = 0;

        hr = IDXGISwapChain3_Present(swapchain, sync_interval, 0);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        frame_fence_value[index] = fence_value;
        hr = ID3D12CommandQueue_Signal(queue, fence, fence_value);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ++fence_value;
    }

    wait_queue_idle(device, queue);

    ID3D12Fence_Release(fence);
    destroy_render_targets(&context);
    refcount = IDXGISwapChain3_Release(swapchain);
    ok(!refcount, "Swapchain has %lu references left.\n", refcount);
    DestroyWindow(window);
    destroy_test_context(&context);
}

static void test_desktop_window(void)
{
    DXGI_SWAP_CHAIN_DESC1 swapchain_desc;
    struct test_context_desc desc;
    struct test_context context;
    IDXGISwapChain1 *swapchain;
    IDXGIFactory4 *factory;
    IUnknown *queue;
    HWND window;
    HRESULT hr;
    RECT rect;
    BOOL ret;

    memset(&desc, 0, sizeof(desc));
    desc.no_render_target = TRUE;
    if (!init_test_context(&context, NULL))
        return;
    queue = (IUnknown *)context.queue;

    window = GetDesktopWindow();
    ret = GetClientRect(window, &rect);
    ok(ret, "Failed to get client rect.\n");

    hr = CreateDXGIFactory2(0, &IID_IDXGIFactory4, (void **)&factory);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    swapchain_desc.Width = 640;
    swapchain_desc.Height = 480;
    swapchain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchain_desc.Stereo = FALSE;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = 2;
    swapchain_desc.Scaling = DXGI_SCALING_STRETCH;
    swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapchain_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapchain_desc.Flags = 0;
    hr = IDXGIFactory4_CreateSwapChainForHwnd(factory, queue, window, &swapchain_desc, NULL, NULL, &swapchain);
    ok(hr == E_ACCESSDENIED, "Got unexpected hr %#lx.\n", hr);

    swapchain_desc.Width = rect.right;
    swapchain_desc.Height = rect.bottom;
    hr = IDXGIFactory4_CreateSwapChainForHwnd(factory, queue, window, &swapchain_desc, NULL, NULL, &swapchain);
    ok(hr == E_ACCESSDENIED || broken(hr == E_OUTOFMEMORY /* win10 1709 */),
       "Got unexpected hr %#lx.\n", hr);

    IDXGIFactory4_Release(factory);
    destroy_test_context(&context);
}

static void test_invalid_command_queue_types(void)
{
    static const enum D3D12_COMMAND_LIST_TYPE queue_types[] =
    {
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        D3D12_COMMAND_LIST_TYPE_COMPUTE,
        D3D12_COMMAND_LIST_TYPE_COPY,
    };

    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    ID3D12CommandQueue *queue;
    IDXGISwapChain *swapchain;
    IDXGIFactory *factory;
    ID3D12Device *device;
    HRESULT hr, expected;
    IUnknown *queue_unk;
    RECT client_rect;
    ULONG refcount;
    unsigned int i;
    HWND window;
    BOOL ret;

    if (!(device = create_device()))
    {
        skip("Failed to create Direct3D 12 device.\n");
        return;
    }

    window = create_window(WS_VISIBLE);
    ret = GetClientRect(window, &client_rect);
    ok(ret, "Failed to get client rect.\n");

    for (i = 0; i < ARRAY_SIZE(queue_types); ++i)
    {
        queue = create_command_queue(device, queue_types[i]);
        hr = ID3D12CommandQueue_QueryInterface(queue, &IID_IUnknown, (void **)&queue_unk);
        ok(hr == S_OK, "Got unexpected hr %lx.\n", hr);

        hr = CreateDXGIFactory(&IID_IDXGIFactory, (void **)&factory);
        ok(hr == S_OK, "Got unexpected hr %lx.\n", hr);

        swapchain_desc.BufferDesc.Width = 640;
        swapchain_desc.BufferDesc.Height = 480;
        swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
        swapchain_desc.BufferDesc.RefreshRate.Denominator = 1;
        swapchain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        swapchain_desc.SampleDesc.Count = 1;
        swapchain_desc.SampleDesc.Quality = 0;
        swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchain_desc.BufferCount = 2;
        swapchain_desc.OutputWindow = window;
        swapchain_desc.Windowed = TRUE;
        swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapchain_desc.Flags = 0;

        hr = IDXGIFactory_CreateSwapChain(factory, queue_unk, &swapchain_desc, &swapchain);
        expected = queue_types[i] == D3D12_COMMAND_LIST_TYPE_DIRECT ? S_OK : DXGI_ERROR_INVALID_CALL;
        ok(hr == expected, "Got unexpected hr %#lx.\n", hr);

        if (hr == S_OK)
        {
            refcount = IDXGISwapChain_Release(swapchain);
            ok(!refcount, "Swapchain has %lu references left.\n", refcount);
        }

        wait_queue_idle(device, queue);

        IDXGIFactory_Release(factory);

        IUnknown_Release(queue_unk);
        refcount = ID3D12CommandQueue_Release(queue);
        ok(!refcount, "Command queue has %lu references left.\n", refcount);
    }

    DestroyWindow(window);

    refcount = ID3D12Device_Release(device);
    ok(!refcount, "Device has %lu references left.\n", refcount);
}

START_TEST(d3d12)
{
    BOOL enable_debug_layer = FALSE;
    unsigned int argc, i;
    ID3D12Debug *debug;
    char **argv;

    argc = winetest_get_mainargs(&argv);
    for (i = 2; i < argc; ++i)
    {
        if (!strcmp(argv[i], "--validate"))
            enable_debug_layer = TRUE;
        else if (!strcmp(argv[i], "--warp"))
            use_warp_adapter = TRUE;
        else if (!strcmp(argv[i], "--adapter") && i + 1 < argc)
            use_adapter_idx = atoi(argv[++i]);
    }

    if (enable_debug_layer && SUCCEEDED(D3D12GetDebugInterface(&IID_ID3D12Debug, (void **)&debug)))
    {
        ID3D12Debug_EnableDebugLayer(debug);
        ID3D12Debug_Release(debug);
    }

    print_adapter_info();

    test_ordinals();
    test_interfaces();
    test_create_device();
    test_draw();
    test_swapchain_draw();
    test_swapchain_refcount();
    test_swapchain_size_mismatch();
    test_swapchain_backbuffer_index();
    test_desktop_window();
    test_invalid_command_queue_types();
}
