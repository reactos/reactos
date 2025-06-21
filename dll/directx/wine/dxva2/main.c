/*
 * Copyright 2014 Michael Müller for Pipelight
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

#include <stdarg.h>
#include <limits.h>
#include "windef.h"
#include "winbase.h"
#include "d3d9.h"
#include "physicalmonitorenumerationapi.h"
#include "lowlevelmonitorconfigurationapi.h"
#include "highlevelmonitorconfigurationapi.h"
#include "initguid.h"
#include "dxva2api.h"
#include "dxvahd.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxva2);

#define D3DFMT_NV12 MAKEFOURCC('N','V','1','2')

enum device_handle_flags
{
    HANDLE_FLAG_OPEN = 0x1,
    HANDLE_FLAG_INVALID = 0x2,
};

struct device_handle
{
    unsigned int flags;
    IDirect3DStateBlock9 *state_block;
};

struct device_manager
{
    IDirect3DDeviceManager9 IDirect3DDeviceManager9_iface;
    IDirectXVideoProcessorService IDirectXVideoProcessorService_iface;
    IDirectXVideoDecoderService IDirectXVideoDecoderService_iface;
    LONG refcount;

    IDirect3DDevice9 *device;
    UINT token;

    struct device_handle *handles;
    size_t count;
    size_t capacity;

    HANDLE locking_handle;

    CRITICAL_SECTION cs;
    CONDITION_VARIABLE lock;
};

struct video_processor
{
    IDirectXVideoProcessor IDirectXVideoProcessor_iface;
    LONG refcount;

    IDirectXVideoProcessorService *service;
    GUID device;
    DXVA2_VideoDesc video_desc;
    D3DFORMAT rt_format;
    unsigned int max_substreams;
};

static BOOL dxva_array_reserve(void **elements, size_t *capacity, size_t count, size_t size)
{
    size_t new_capacity, max_capacity;
    void *new_elements;

    if (count <= *capacity)
        return TRUE;

    max_capacity = ~(SIZE_T)0 / size;
    if (count > max_capacity)
        return FALSE;

    new_capacity = max(4, *capacity);
    while (new_capacity < count && new_capacity <= max_capacity / 2)
        new_capacity *= 2;
    if (new_capacity < count)
        new_capacity = max_capacity;

    if (!(new_elements = realloc(*elements, new_capacity * size)))
        return FALSE;

    *elements = new_elements;
    *capacity = new_capacity;

    return TRUE;
}

static struct device_manager *impl_from_IDirect3DDeviceManager9(IDirect3DDeviceManager9 *iface)
{
    return CONTAINING_RECORD(iface, struct device_manager, IDirect3DDeviceManager9_iface);
}

static struct device_manager *impl_from_IDirectXVideoProcessorService(IDirectXVideoProcessorService *iface)
{
    return CONTAINING_RECORD(iface, struct device_manager, IDirectXVideoProcessorService_iface);
}

static struct device_manager *impl_from_IDirectXVideoDecoderService(IDirectXVideoDecoderService *iface)
{
    return CONTAINING_RECORD(iface, struct device_manager, IDirectXVideoDecoderService_iface);
}

static struct video_processor *impl_from_IDirectXVideoProcessor(IDirectXVideoProcessor *iface)
{
    return CONTAINING_RECORD(iface, struct video_processor, IDirectXVideoProcessor_iface);
}

static const DXVA2_VideoProcessorCaps software_processor_caps =
{
    .DeviceCaps = DXVA2_VPDev_SoftwareDevice,
    .InputPool = D3DPOOL_SYSTEMMEM,
    .VideoProcessorOperations = DXVA2_VideoProcess_PlanarAlpha | DXVA2_VideoProcess_YUV2RGB |
            DXVA2_VideoProcess_StretchX | DXVA2_VideoProcess_StretchY | DXVA2_VideoProcess_SubRects |
            DXVA2_VideoProcess_SubStreams | DXVA2_VideoProcess_SubStreamsExtended | DXVA2_VideoProcess_YUV2RGBExtended,
};

static const DXVA2_VideoProcessorCaps progressive_processor_caps =
{
    .DeviceCaps = DXVA2_VPDev_HardwareDevice,
    .InputPool = D3DPOOL_DEFAULT,
    .VideoProcessorOperations = DXVA2_VideoProcess_YUV2RGB | DXVA2_VideoProcess_StretchX | DXVA2_VideoProcess_StretchY,
};

static HRESULT WINAPI video_processor_QueryInterface(IDirectXVideoProcessor *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IDirectXVideoProcessor) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IDirectXVideoProcessor_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI video_processor_AddRef(IDirectXVideoProcessor *iface)
{
    struct video_processor *processor = impl_from_IDirectXVideoProcessor(iface);
    ULONG refcount = InterlockedIncrement(&processor->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI video_processor_Release(IDirectXVideoProcessor *iface)
{
    struct video_processor *processor = impl_from_IDirectXVideoProcessor(iface);
    ULONG refcount = InterlockedDecrement(&processor->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        IDirectXVideoProcessorService_Release(processor->service);
        free(processor);
    }

    return refcount;
}

static HRESULT WINAPI video_processor_GetVideoProcessorService(IDirectXVideoProcessor *iface,
        IDirectXVideoProcessorService **service)
{
    struct video_processor *processor = impl_from_IDirectXVideoProcessor(iface);

    TRACE("%p, %p.\n", iface, service);

    *service = processor->service;
    IDirectXVideoProcessorService_AddRef(*service);

    return S_OK;
}

static HRESULT WINAPI video_processor_GetCreationParameters(IDirectXVideoProcessor *iface,
        GUID *device, DXVA2_VideoDesc *video_desc, D3DFORMAT *rt_format, UINT *max_substreams)
{
    struct video_processor *processor = impl_from_IDirectXVideoProcessor(iface);

    TRACE("%p, %p, %p, %p, %p.\n", iface, device, video_desc, rt_format, max_substreams);

    if (!device && !video_desc && !rt_format && !max_substreams)
        return E_INVALIDARG;

    if (device)
        *device = processor->device;
    if (video_desc)
        *video_desc = processor->video_desc;
    if (rt_format)
        *rt_format = processor->rt_format;
    if (max_substreams)
        *max_substreams = processor->max_substreams;

    return S_OK;
}

static HRESULT WINAPI video_processor_GetVideoProcessorCaps(IDirectXVideoProcessor *iface,
        DXVA2_VideoProcessorCaps *caps)
{
    struct video_processor *processor = impl_from_IDirectXVideoProcessor(iface);

    TRACE("%p, %p.\n", iface, caps);

    if (IsEqualGUID(&processor->device, &DXVA2_VideoProcSoftwareDevice))
    {
        *caps = software_processor_caps;
    }
    else if (IsEqualGUID(&processor->device, &DXVA2_VideoProcProgressiveDevice))
    {
        *caps = progressive_processor_caps;
    }
    else
    {
        FIXME("Unsupported device %s.\n", debugstr_guid(&processor->device));
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI video_processor_GetProcAmpRange(IDirectXVideoProcessor *iface, UINT cap, DXVA2_ValueRange *range)
{
    FIXME("%p, %u, %p.\n", iface, cap, range);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_processor_GetFilterPropertyRange(IDirectXVideoProcessor *iface, UINT setting,
        DXVA2_ValueRange *range)
{
    FIXME("%p, %u, %p.\n", iface, setting, range);

    return E_NOTIMPL;
}

static BOOL intersect_rect(RECT *dest, const RECT *src1, const RECT *src2)
{
    if (IsRectEmpty(src1) || IsRectEmpty(src2) ||
        (src1->left >= src2->right) || (src2->left >= src1->right) ||
        (src1->top >= src2->bottom) || (src2->top >= src1->bottom))
    {
        SetRectEmpty(dest);
        return FALSE;
    }
    dest->left   = max(src1->left, src2->left);
    dest->right  = min(src1->right, src2->right);
    dest->top    = max(src1->top, src2->top);
    dest->bottom = min(src1->bottom, src2->bottom);

    return TRUE;
}

static D3DCOLOR video_processor_get_background_color(const DXVA2_AYUVSample16 *ayuv)
{
    float y, cb, cr;
    BYTE r, g, b;

    y = (ayuv->Y >> 8) - 16;
    cb = (ayuv->Cb >> 8) - 128;
    cr = (ayuv->Cr >> 8) - 128;

    y = 255.0f * y / 219.0f;
    cb = 255.0f * cb / 224.0f;
    cr = 255.0f * cr / 224.0f;

    r = y + 1.402 * cr;
    g = y - 0.344 * cb - 0.714 * cr;
    b = y + 1.772 * cb;

    return D3DCOLOR_XRGB(r, g, b);
}

static HRESULT WINAPI video_processor_VideoProcessBlt(IDirectXVideoProcessor *iface, IDirect3DSurface9 *rt,
        const DXVA2_VideoProcessBltParams *params, const DXVA2_VideoSample *samples, UINT sample_count,
        HANDLE *complete_handle)
{
    IDirect3DDevice9 *device;
    unsigned int i;
    RECT dst_rect;
    HRESULT hr;

    TRACE("%p, %p, %p, %p, %u, %p.\n", iface, rt, params, samples, sample_count, complete_handle);

    if (params->BackgroundColor.Alpha != 0xffff)
    {
        WARN("Unexpected background alpha %#x.\n", params->BackgroundColor.Alpha);
        return E_INVALIDARG;
    }

    if (FAILED(hr = IDirect3DSurface9_GetDevice(rt, &device)))
    {
        WARN("Failed to get surface device, hr %#lx.\n", hr);
        return hr;
    }

    IDirect3DDevice9_ColorFill(device, rt, &params->TargetRect, video_processor_get_background_color(&params->BackgroundColor));

    for (i = 0; i < sample_count; ++i)
    {
        dst_rect = params->TargetRect;

        if (!intersect_rect(&dst_rect, &dst_rect, &samples[i].DstRect))
            continue;

        if (FAILED(hr = IDirect3DDevice9_StretchRect(device, samples[i].SrcSurface, &samples[i].SrcRect,
                rt, &dst_rect, D3DTEXF_POINT)))
        {
            WARN("Failed to copy sample %u, hr %#lx.\n", i, hr);
        }
    }

    IDirect3DDevice9_Release(device);

    return S_OK;
}

static const IDirectXVideoProcessorVtbl video_processor_vtbl =
{
    video_processor_QueryInterface,
    video_processor_AddRef,
    video_processor_Release,
    video_processor_GetVideoProcessorService,
    video_processor_GetCreationParameters,
    video_processor_GetVideoProcessorCaps,
    video_processor_GetProcAmpRange,
    video_processor_GetFilterPropertyRange,
    video_processor_VideoProcessBlt,
};

static HRESULT WINAPI device_manager_processor_service_QueryInterface(IDirectXVideoProcessorService *iface,
        REFIID riid, void **obj)
{
    struct device_manager *manager = impl_from_IDirectXVideoProcessorService(iface);

    if (IsEqualIID(riid, &IID_IDirectXVideoProcessorService) ||
            IsEqualIID(riid, &IID_IDirectXVideoAccelerationService) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
    }
    else if (IsEqualIID(riid, &IID_IDirectXVideoDecoderService))
    {
        *obj = &manager->IDirectXVideoDecoderService_iface;
    }
    else
    {
        WARN("Unsupported interface %s.\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);
    return S_OK;
}

static ULONG WINAPI device_manager_processor_service_AddRef(IDirectXVideoProcessorService *iface)
{
    struct device_manager *manager = impl_from_IDirectXVideoProcessorService(iface);
    return IDirect3DDeviceManager9_AddRef(&manager->IDirect3DDeviceManager9_iface);
}

static ULONG WINAPI device_manager_processor_service_Release(IDirectXVideoProcessorService *iface)
{
    struct device_manager *manager = impl_from_IDirectXVideoProcessorService(iface);
    return IDirect3DDeviceManager9_Release(&manager->IDirect3DDeviceManager9_iface);
}

static HRESULT WINAPI device_manager_processor_service_CreateSurface(IDirectXVideoProcessorService *iface,
        UINT width, UINT height, UINT backbuffers, D3DFORMAT format, D3DPOOL pool, DWORD usage, DWORD dxvaType,
        IDirect3DSurface9 **surfaces, HANDLE *shared_handle)
{
    struct device_manager *manager = impl_from_IDirectXVideoProcessorService(iface);
    unsigned int i, j;
    HRESULT hr;

    TRACE("%p, %u, %u, %u, %u, %u, %lu, %lu, %p, %p.\n", iface, width, height, backbuffers, format, pool, usage, dxvaType,
            surfaces, shared_handle);

    if (backbuffers >= UINT_MAX)
        return E_INVALIDARG;

    memset(surfaces, 0, (backbuffers + 1) * sizeof(*surfaces));

    for (i = 0; i < backbuffers + 1; ++i)
    {
        if (FAILED(hr = IDirect3DDevice9_CreateOffscreenPlainSurface(manager->device, width, height, format,
                pool, &surfaces[i], NULL)))
            break;
    }

    if (FAILED(hr))
    {
        for (j = 0; j < i; ++j)
        {
            if (surfaces[j])
            {
                IDirect3DSurface9_Release(surfaces[j]);
                surfaces[j] = NULL;
            }
        }
    }

    return hr;
}

static HRESULT WINAPI device_manager_processor_service_RegisterVideoProcessorSoftwareDevice(
        IDirectXVideoProcessorService *iface, void *callbacks)
{
    FIXME("%p, %p.\n", iface, callbacks);

    return E_NOTIMPL;
}

struct dxva_processor_device_desc
{
    const GUID *guid;
    const D3DFORMAT *input_formats;
};

static const D3DFORMAT software_processor_input_formats[] =
{
    D3DFMT_A8R8G8B8, D3DFMT_X8R8G8B8, D3DFMT_YUY2, 0
};

static const D3DFORMAT progressive_processor_input_formats[] =
{
    D3DFMT_A8R8G8B8, D3DFMT_X8R8G8B8, D3DFMT_YUY2, D3DFMT_NV12, 0
};

static const struct dxva_processor_device_desc processor_devices[] =
{
    { &DXVA2_VideoProcProgressiveDevice, progressive_processor_input_formats },
    { &DXVA2_VideoProcSoftwareDevice, software_processor_input_formats },
};

static BOOL dxva_is_supported_stream_format(const DXVA2_VideoDesc *video_desc, const D3DFORMAT *formats)
{
    while (*formats)
    {
        if (*formats == video_desc->Format) return TRUE;
        formats++;
    }

    return FALSE;
}

static HRESULT WINAPI device_manager_processor_service_GetVideoProcessorDeviceGuids(
        IDirectXVideoProcessorService *iface, const DXVA2_VideoDesc *video_desc, UINT *count, GUID **guids)
{
    unsigned int i;

    FIXME("%p, %p, %p, %p semi-stub.\n", iface, video_desc, count, guids);

    *count = 0;

    if (!(*guids = CoTaskMemAlloc(ARRAY_SIZE(processor_devices) * sizeof(**guids))))
        return E_OUTOFMEMORY;

    for (i = 0; i < ARRAY_SIZE(processor_devices); ++i)
    {
        if (dxva_is_supported_stream_format(video_desc, processor_devices[i].input_formats))
        {
            (*guids)[*count] = *processor_devices[i].guid;
            *count += 1;
        }
    }

    if (!*count)
    {
        CoTaskMemFree(*guids);
        *guids = NULL;
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI device_manager_processor_service_GetVideoProcessorRenderTargets(
        IDirectXVideoProcessorService *iface, REFGUID deviceguid, const DXVA2_VideoDesc *video_desc, UINT *count,
        D3DFORMAT **formats)
{
    TRACE("%p, %s, %p, %p, %p.\n", iface, debugstr_guid(deviceguid), video_desc, count, formats);

    if (IsEqualGUID(deviceguid, &DXVA2_VideoProcSoftwareDevice))
    {
        if (!dxva_is_supported_stream_format(video_desc, software_processor_input_formats))
        {
            WARN("Unsupported content format %#x.\n", video_desc->Format);
            return E_FAIL;
        }

        if (!(*formats = CoTaskMemAlloc(2 * sizeof(**formats))))
            return E_OUTOFMEMORY;

        *count = 2;
        (*formats)[0] = D3DFMT_X8R8G8B8;
        (*formats)[1] = D3DFMT_A8R8G8B8;

        return S_OK;
    }
    else if (IsEqualGUID(deviceguid, &DXVA2_VideoProcProgressiveDevice))
    {
        if (!dxva_is_supported_stream_format(video_desc, progressive_processor_input_formats))
        {
            WARN("Unsupported content format %#x.\n", video_desc->Format);
            return E_FAIL;
        }

        if (!(*formats = CoTaskMemAlloc(2 * sizeof(**formats))))
            return E_OUTOFMEMORY;

        *count = 2;
        (*formats)[0] = D3DFMT_X8R8G8B8;
        (*formats)[1] = D3DFMT_NV12;

        return S_OK;
    }
    else
        FIXME("Unsupported device %s.\n", debugstr_guid(deviceguid));

    return E_NOTIMPL;
}

static HRESULT WINAPI device_manager_processor_service_GetVideoProcessorSubStreamFormats(
        IDirectXVideoProcessorService *iface, REFGUID deviceguid, const DXVA2_VideoDesc *video_desc,
        D3DFORMAT rt_format, UINT *count, D3DFORMAT **formats)
{
    FIXME("%p, %s, %p, %u, %p, %p.\n", iface, debugstr_guid(deviceguid), video_desc, rt_format, count, formats);

    return E_NOTIMPL;
}

static HRESULT WINAPI device_manager_processor_service_GetVideoProcessorCaps(
        IDirectXVideoProcessorService *iface, REFGUID device, const DXVA2_VideoDesc *video_desc,
        D3DFORMAT rt_format, DXVA2_VideoProcessorCaps *caps)
{
    TRACE("%p, %s, %p, %u, %p.\n", iface, debugstr_guid(device), video_desc, rt_format, caps);

    if (IsEqualGUID(device, &DXVA2_VideoProcSoftwareDevice))
    {
        *caps = software_processor_caps;
    }
    else if (IsEqualGUID(device, &DXVA2_VideoProcProgressiveDevice))
    {
        *caps = progressive_processor_caps;
    }
    else
    {
        FIXME("Unrecognized device %s.\n", debugstr_guid(device));
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT WINAPI device_manager_processor_service_GetProcAmpRange(
        IDirectXVideoProcessorService *iface, REFGUID deviceguid, const DXVA2_VideoDesc *video_desc,
        D3DFORMAT rt_format, UINT ProcAmpCap, DXVA2_ValueRange *range)
{
    FIXME("%p, %s, %p, %u, %u, %p.\n", iface, debugstr_guid(deviceguid), video_desc, rt_format, ProcAmpCap, range);

    return E_NOTIMPL;
}

static HRESULT WINAPI device_manager_processor_service_GetFilterPropertyRange(
        IDirectXVideoProcessorService *iface, REFGUID deviceguid, const DXVA2_VideoDesc *video_desc,
        D3DFORMAT rt_format, UINT filter_setting, DXVA2_ValueRange *range)
{
    FIXME("%p, %s, %p, %d, %d, %p.\n", iface, debugstr_guid(deviceguid), video_desc, rt_format, filter_setting, range);

    return E_NOTIMPL;
}

static HRESULT WINAPI device_manager_processor_service_CreateVideoProcessor(IDirectXVideoProcessorService *iface,
        REFGUID device, const DXVA2_VideoDesc *video_desc, D3DFORMAT rt_format, UINT max_substreams,
        IDirectXVideoProcessor **processor)
{
    struct video_processor *object;

    FIXME("%p, %s, %p, %d, %u, %p.\n", iface, debugstr_guid(device), video_desc, rt_format, max_substreams,
            processor);

    /* FIXME: validate render target format */

    if (max_substreams >= 16)
    {
        WARN("Invalid substreams count %u.\n", max_substreams);
        return E_INVALIDARG;
    }

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IDirectXVideoProcessor_iface.lpVtbl = &video_processor_vtbl;
    object->refcount = 1;
    object->service = iface;
    IDirectXVideoProcessorService_AddRef(object->service);
    object->device = *device;
    object->video_desc = *video_desc;
    object->rt_format = rt_format;
    object->max_substreams = max_substreams;

    *processor = &object->IDirectXVideoProcessor_iface;

    return S_OK;
}

static const IDirectXVideoProcessorServiceVtbl device_manager_processor_service_vtbl =
{
    device_manager_processor_service_QueryInterface,
    device_manager_processor_service_AddRef,
    device_manager_processor_service_Release,
    device_manager_processor_service_CreateSurface,
    device_manager_processor_service_RegisterVideoProcessorSoftwareDevice,
    device_manager_processor_service_GetVideoProcessorDeviceGuids,
    device_manager_processor_service_GetVideoProcessorRenderTargets,
    device_manager_processor_service_GetVideoProcessorSubStreamFormats,
    device_manager_processor_service_GetVideoProcessorCaps,
    device_manager_processor_service_GetProcAmpRange,
    device_manager_processor_service_GetFilterPropertyRange,
    device_manager_processor_service_CreateVideoProcessor,
};

static HRESULT WINAPI device_manager_decoder_service_QueryInterface(IDirectXVideoDecoderService *iface,
        REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IDirectXVideoDecoderService) ||
            IsEqualIID(riid, &IID_IDirectXVideoAccelerationService) ||
            IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IDirectXVideoDecoderService_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI device_manager_decoder_service_AddRef(IDirectXVideoDecoderService *iface)
{
    struct device_manager *manager = impl_from_IDirectXVideoDecoderService(iface);
    return IDirect3DDeviceManager9_AddRef(&manager->IDirect3DDeviceManager9_iface);
}

static ULONG WINAPI device_manager_decoder_service_Release(IDirectXVideoDecoderService *iface)
{
    struct device_manager *manager = impl_from_IDirectXVideoDecoderService(iface);
    return IDirect3DDeviceManager9_Release(&manager->IDirect3DDeviceManager9_iface);
}

static HRESULT WINAPI device_manager_decoder_service_CreateSurface(IDirectXVideoDecoderService *iface,
        UINT width, UINT height, UINT backbuffers, D3DFORMAT format, D3DPOOL pool, DWORD usage, DWORD dxvaType,
        IDirect3DSurface9 **surfaces, HANDLE *shared_handle)
{
    FIXME("%p, %u, %u, %u, %#x, %d, %ld, %ld, %p, %p.\n", iface, width, height, backbuffers, format, pool, usage,
            dxvaType, surfaces, shared_handle);

    return E_NOTIMPL;
}

static HRESULT WINAPI device_manager_decoder_service_GetDecoderDeviceGuids(IDirectXVideoDecoderService *iface,
        UINT *count, GUID **guids)
{
    FIXME("%p, %p, %p.\n", iface, count, guids);

    return E_NOTIMPL;
}

static HRESULT WINAPI device_manager_decoder_service_GetDecoderRenderTargets(IDirectXVideoDecoderService *iface,
        REFGUID guid, UINT *count, D3DFORMAT **formats)
{
    FIXME("%p, %s, %p, %p.\n", iface, debugstr_guid(guid), count, formats);

    return E_NOTIMPL;
}

static HRESULT WINAPI device_manager_decoder_service_GetDecoderConfigurations(IDirectXVideoDecoderService *iface,
        REFGUID guid, const DXVA2_VideoDesc *video_desc, void *reserved, UINT *count, DXVA2_ConfigPictureDecode **configs)
{
    FIXME("%p, %s, %p, %p, %p, %p.\n", iface, debugstr_guid(guid), video_desc, reserved, count, configs);

    return E_NOTIMPL;
}

static HRESULT WINAPI device_manager_decoder_service_CreateVideoDecoder(IDirectXVideoDecoderService *iface,
        REFGUID guid, const DXVA2_VideoDesc *video_desc, const DXVA2_ConfigPictureDecode *config, IDirect3DSurface9 **rts,
        UINT num_surfaces, IDirectXVideoDecoder **decoder)
{
    FIXME("%p, %s, %p, %p, %p, %u, %p.\n", iface, debugstr_guid(guid), video_desc, config, rts, num_surfaces,
            decoder);

    return E_NOTIMPL;
}

static const IDirectXVideoDecoderServiceVtbl device_manager_decoder_service_vtbl =
{
    device_manager_decoder_service_QueryInterface,
    device_manager_decoder_service_AddRef,
    device_manager_decoder_service_Release,
    device_manager_decoder_service_CreateSurface,
    device_manager_decoder_service_GetDecoderDeviceGuids,
    device_manager_decoder_service_GetDecoderRenderTargets,
    device_manager_decoder_service_GetDecoderConfigurations,
    device_manager_decoder_service_CreateVideoDecoder,
};

static HRESULT WINAPI device_manager_QueryInterface(IDirect3DDeviceManager9 *iface, REFIID riid, void **obj)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    if (IsEqualIID(&IID_IDirect3DDeviceManager9, riid) ||
            IsEqualIID(&IID_IUnknown, riid))
    {
        *obj = iface;
        IDirect3DDeviceManager9_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI device_manager_AddRef(IDirect3DDeviceManager9 *iface)
{
    struct device_manager *manager = impl_from_IDirect3DDeviceManager9(iface);
    ULONG refcount = InterlockedIncrement(&manager->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI device_manager_Release(IDirect3DDeviceManager9 *iface)
{
    struct device_manager *manager = impl_from_IDirect3DDeviceManager9(iface);
    ULONG refcount = InterlockedDecrement(&manager->refcount);
    size_t i;

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (manager->device)
            IDirect3DDevice9_Release(manager->device);
        DeleteCriticalSection(&manager->cs);
        for (i = 0; i < manager->count; ++i)
        {
            if (manager->handles[i].state_block)
                IDirect3DStateBlock9_Release(manager->handles[i].state_block);
        }
        free(manager->handles);
        free(manager);
    }

    return refcount;
}

static HRESULT WINAPI device_manager_ResetDevice(IDirect3DDeviceManager9 *iface, IDirect3DDevice9 *device,
        UINT token)
{
    struct device_manager *manager = impl_from_IDirect3DDeviceManager9(iface);
    size_t i;

    TRACE("%p, %p, %#x.\n", iface, device, token);

    if (token != manager->token)
        return E_INVALIDARG;

    EnterCriticalSection(&manager->cs);
    if (manager->device)
    {
        for (i = 0; i < manager->count; ++i)
        {
            if (manager->handles[i].state_block)
                IDirect3DStateBlock9_Release(manager->handles[i].state_block);
            manager->handles[i].state_block = NULL;
            manager->handles[i].flags |= HANDLE_FLAG_INVALID;
        }
        manager->locking_handle = NULL;
        IDirect3DDevice9_Release(manager->device);
    }
    manager->device = device;
    IDirect3DDevice9_AddRef(manager->device);
    LeaveCriticalSection(&manager->cs);

    WakeAllConditionVariable(&manager->lock);

    return S_OK;
}

static HRESULT WINAPI device_manager_OpenDeviceHandle(IDirect3DDeviceManager9 *iface, HANDLE *hdevice)
{
    struct device_manager *manager = impl_from_IDirect3DDeviceManager9(iface);
    HRESULT hr = S_OK;
    size_t i;

    TRACE("%p, %p.\n", iface, hdevice);

    *hdevice = NULL;

    EnterCriticalSection(&manager->cs);
    if (!manager->device)
        hr = DXVA2_E_NOT_INITIALIZED;
    else
    {
        for (i = 0; i < manager->count; ++i)
        {
            if (!(manager->handles[i].flags & HANDLE_FLAG_OPEN))
            {
                manager->handles[i].flags |= HANDLE_FLAG_OPEN;
                *hdevice = ULongToHandle(i + 1);
                break;
            }
        }

        if (dxva_array_reserve((void **)&manager->handles, &manager->capacity, manager->count + 1,
                sizeof(*manager->handles)))
        {
            *hdevice = ULongToHandle(manager->count + 1);
            manager->handles[manager->count].flags = HANDLE_FLAG_OPEN;
            manager->handles[manager->count].state_block = NULL;
            manager->count++;
        }
        else
            hr = E_OUTOFMEMORY;
    }
    LeaveCriticalSection(&manager->cs);

    return hr;
}

static HRESULT device_manager_get_handle_index(struct device_manager *manager, HANDLE hdevice, size_t *idx)
{
    if (!hdevice || hdevice > ULongToHandle(manager->count))
        return E_HANDLE;
    *idx = (ULONG_PTR)hdevice - 1;
    return S_OK;
}

static HRESULT WINAPI device_manager_CloseDeviceHandle(IDirect3DDeviceManager9 *iface, HANDLE hdevice)
{
    struct device_manager *manager = impl_from_IDirect3DDeviceManager9(iface);
    HRESULT hr;
    size_t idx;

    TRACE("%p, %p.\n", iface, hdevice);

    EnterCriticalSection(&manager->cs);
    if (SUCCEEDED(hr = device_manager_get_handle_index(manager, hdevice, &idx)))
    {
        if (manager->handles[idx].flags & HANDLE_FLAG_OPEN)
        {
            if (manager->locking_handle == hdevice)
                manager->locking_handle = NULL;
            manager->handles[idx].flags = 0;
            if (idx == manager->count - 1)
                manager->count--;
            if (manager->handles[idx].state_block)
                IDirect3DStateBlock9_Release(manager->handles[idx].state_block);
            manager->handles[idx].state_block = NULL;
        }
        else
            hr = E_HANDLE;
    }
    LeaveCriticalSection(&manager->cs);

    WakeAllConditionVariable(&manager->lock);

    return hr;
}

static HRESULT WINAPI device_manager_TestDevice(IDirect3DDeviceManager9 *iface, HANDLE hdevice)
{
    struct device_manager *manager = impl_from_IDirect3DDeviceManager9(iface);
    HRESULT hr;
    size_t idx;

    TRACE("%p, %p.\n", iface, hdevice);

    EnterCriticalSection(&manager->cs);
    if (SUCCEEDED(hr = device_manager_get_handle_index(manager, hdevice, &idx)))
    {
        unsigned int flags = manager->handles[idx].flags;

        if (flags & HANDLE_FLAG_INVALID)
            hr = DXVA2_E_NEW_VIDEO_DEVICE;
        else if (!(flags & HANDLE_FLAG_OPEN))
            hr = E_HANDLE;
    }
    LeaveCriticalSection(&manager->cs);

    return hr;
}

static HRESULT WINAPI device_manager_LockDevice(IDirect3DDeviceManager9 *iface, HANDLE hdevice,
        IDirect3DDevice9 **device, BOOL block)
{
    struct device_manager *manager = impl_from_IDirect3DDeviceManager9(iface);
    HRESULT hr;
    size_t idx;

    TRACE("%p, %p, %p, %d.\n", iface, hdevice, device, block);

    EnterCriticalSection(&manager->cs);
    if (!manager->device)
        hr = DXVA2_E_NOT_INITIALIZED;
    else if (SUCCEEDED(hr = device_manager_get_handle_index(manager, hdevice, &idx)))
    {
        if (manager->locking_handle && !block)
            hr = DXVA2_E_VIDEO_DEVICE_LOCKED;
        else
        {
            while (manager->locking_handle && block)
            {
                SleepConditionVariableCS(&manager->lock, &manager->cs, INFINITE);
            }

            if (SUCCEEDED(hr = device_manager_get_handle_index(manager, hdevice, &idx)))
            {
                if (manager->handles[idx].flags & HANDLE_FLAG_INVALID)
                    hr = DXVA2_E_NEW_VIDEO_DEVICE;
                else
                {
                    if (manager->handles[idx].state_block)
                    {
                        if (FAILED(IDirect3DStateBlock9_Apply(manager->handles[idx].state_block)))
                            WARN("Failed to apply state.\n");
                        IDirect3DStateBlock9_Release(manager->handles[idx].state_block);
                        manager->handles[idx].state_block = NULL;
                    }
                    *device = manager->device;
                    IDirect3DDevice9_AddRef(*device);
                    manager->locking_handle = hdevice;
                }
            }
        }
    }
    LeaveCriticalSection(&manager->cs);

    return hr;
}

static HRESULT WINAPI device_manager_UnlockDevice(IDirect3DDeviceManager9 *iface, HANDLE hdevice, BOOL savestate)
{
    struct device_manager *manager = impl_from_IDirect3DDeviceManager9(iface);
    HRESULT hr;
    size_t idx;

    TRACE("%p, %p, %d.\n", iface, hdevice, savestate);

    EnterCriticalSection(&manager->cs);

    if (hdevice != manager->locking_handle)
        hr = E_INVALIDARG;
    else if (SUCCEEDED(hr = device_manager_get_handle_index(manager, hdevice, &idx)))
    {
        manager->locking_handle = NULL;
        if (savestate)
            IDirect3DDevice9_CreateStateBlock(manager->device, D3DSBT_ALL, &manager->handles[idx].state_block);
    }

    LeaveCriticalSection(&manager->cs);

    WakeAllConditionVariable(&manager->lock);

    return hr;
}

static HRESULT WINAPI device_manager_GetVideoService(IDirect3DDeviceManager9 *iface, HANDLE hdevice, REFIID riid,
        void **obj)
{
    struct device_manager *manager = impl_from_IDirect3DDeviceManager9(iface);
    HRESULT hr;
    size_t idx;

    TRACE("%p, %p, %s, %p.\n", iface, hdevice, debugstr_guid(riid), obj);

    EnterCriticalSection(&manager->cs);
    if (SUCCEEDED(hr = device_manager_get_handle_index(manager, hdevice, &idx)))
    {
        unsigned int flags = manager->handles[idx].flags;

        if (flags & HANDLE_FLAG_INVALID)
            hr = DXVA2_E_NEW_VIDEO_DEVICE;
        else if (!(flags & HANDLE_FLAG_OPEN))
            hr = E_HANDLE;
        else
            hr = IDirectXVideoProcessorService_QueryInterface(&manager->IDirectXVideoProcessorService_iface,
                    riid, obj);
    }
    LeaveCriticalSection(&manager->cs);

    return hr;
}

static const IDirect3DDeviceManager9Vtbl device_manager_vtbl =
{
    device_manager_QueryInterface,
    device_manager_AddRef,
    device_manager_Release,
    device_manager_ResetDevice,
    device_manager_OpenDeviceHandle,
    device_manager_CloseDeviceHandle,
    device_manager_TestDevice,
    device_manager_LockDevice,
    device_manager_UnlockDevice,
    device_manager_GetVideoService,
};

BOOL WINAPI CapabilitiesRequestAndCapabilitiesReply( HMONITOR monitor, LPSTR buffer, DWORD length )
{
    FIXME("%p, %p, %ld: stub.\n", monitor, buffer, length);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

HRESULT WINAPI DXVA2CreateDirect3DDeviceManager9(UINT *token, IDirect3DDeviceManager9 **manager)
{
    struct device_manager *object;

    TRACE("%p, %p.\n", token, manager);

    *manager = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IDirect3DDeviceManager9_iface.lpVtbl = &device_manager_vtbl;
    object->IDirectXVideoProcessorService_iface.lpVtbl = &device_manager_processor_service_vtbl;
    object->IDirectXVideoDecoderService_iface.lpVtbl = &device_manager_decoder_service_vtbl;
    object->refcount = 1;
    object->token = GetTickCount();
    InitializeCriticalSection(&object->cs);
    InitializeConditionVariable(&object->lock);

    *token = object->token;
    *manager = &object->IDirect3DDeviceManager9_iface;

    return S_OK;
}

HRESULT WINAPI DXVA2CreateVideoService(IDirect3DDevice9 *device, REFIID riid, void **obj)
{
    IDirect3DDeviceManager9 *manager;
    HANDLE handle;
    HRESULT hr;
    UINT token;

    TRACE("%p, %s, %p.\n", device, debugstr_guid(riid), obj);

    if (FAILED(hr = DXVA2CreateDirect3DDeviceManager9(&token, &manager)))
        return hr;

    if (FAILED(hr = IDirect3DDeviceManager9_ResetDevice(manager, device, token)))
        goto done;

    if (FAILED(hr = IDirect3DDeviceManager9_OpenDeviceHandle(manager, &handle)))
        goto done;

    hr = IDirect3DDeviceManager9_GetVideoService(manager, handle, riid, obj);
    IDirect3DDeviceManager9_CloseDeviceHandle(manager, handle);

done:
    IDirect3DDeviceManager9_Release(manager);

    return hr;
}

BOOL WINAPI DegaussMonitor( HMONITOR monitor )
{
    FIXME("(%p): stub\n", monitor);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI DestroyPhysicalMonitor( HMONITOR monitor )
{
    FIXME("(%p): stub\n", monitor);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI DestroyPhysicalMonitors( DWORD arraySize, LPPHYSICAL_MONITOR array )
{
    FIXME("%lu, %p: stub.\n", arraySize, array);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetCapabilitiesStringLength( HMONITOR monitor, LPDWORD length )
{
    FIXME("(%p, %p): stub\n", monitor, length);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetMonitorBrightness( HMONITOR monitor, LPDWORD minimum, LPDWORD current, LPDWORD maximum )
{
    FIXME("(%p, %p, %p, %p): stub\n", monitor, minimum, current, maximum);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetMonitorCapabilities( HMONITOR monitor, LPDWORD capabilities, LPDWORD temperatures )
{
    FIXME("(%p, %p, %p): stub\n", monitor, capabilities, temperatures);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


BOOL WINAPI GetMonitorColorTemperature( HMONITOR monitor, LPMC_COLOR_TEMPERATURE temperature )
{
    FIXME("(%p, %p): stub\n", monitor, temperature);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetMonitorContrast( HMONITOR monitor, LPDWORD minimum, LPDWORD current, LPDWORD maximum )
{
    FIXME("(%p, %p, %p, %p): stub\n", monitor, minimum, current, maximum);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetMonitorDisplayAreaPosition( HMONITOR monitor, MC_POSITION_TYPE type, LPDWORD minimum,
                                           LPDWORD current, LPDWORD maximum )
{
    FIXME("(%p, 0x%x, %p, %p, %p): stub\n", monitor, type, minimum, current, maximum);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetMonitorDisplayAreaSize( HMONITOR monitor, MC_SIZE_TYPE type, LPDWORD minimum,
                                       LPDWORD current, LPDWORD maximum )
{
    FIXME("(%p, 0x%x, %p, %p, %p): stub\n", monitor, type, minimum, current, maximum);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetMonitorRedGreenOrBlueDrive( HMONITOR monitor, MC_DRIVE_TYPE type, LPDWORD minimum,
                                           LPDWORD current, LPDWORD maximum )
{
    FIXME("(%p, 0x%x, %p, %p, %p): stub\n", monitor, type, minimum, current, maximum);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetMonitorRedGreenOrBlueGain( HMONITOR monitor, MC_GAIN_TYPE type, LPDWORD minimum,
                                          LPDWORD current, LPDWORD maximum )
{
    FIXME("(%p, 0x%x, %p, %p, %p): stub\n", monitor, type, minimum, current, maximum);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetMonitorTechnologyType( HMONITOR monitor, LPMC_DISPLAY_TECHNOLOGY_TYPE type )
{
    FIXME("(%p, %p): stub\n", monitor, type);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetNumberOfPhysicalMonitorsFromHMONITOR( HMONITOR monitor, LPDWORD number )
{
    FIXME("(%p, %p): stub\n", monitor, number);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

HRESULT WINAPI GetNumberOfPhysicalMonitorsFromIDirect3DDevice9( IDirect3DDevice9 *device, LPDWORD number )
{
    FIXME("(%p, %p): stub\n", device, number);

    return E_NOTIMPL;
}

BOOL WINAPI GetPhysicalMonitorsFromHMONITOR( HMONITOR monitor, DWORD arraySize, LPPHYSICAL_MONITOR array )
{
    FIXME("%p, %lu, %p: stub.\n", monitor, arraySize, array);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

HRESULT WINAPI GetPhysicalMonitorsFromIDirect3DDevice9( IDirect3DDevice9 *device, DWORD arraySize, LPPHYSICAL_MONITOR array )
{
    FIXME("%p, %lu, %p: stub.\n", device, arraySize, array);

    return E_NOTIMPL;
}

BOOL WINAPI GetTimingReport( HMONITOR monitor, LPMC_TIMING_REPORT timingReport )
{
    FIXME("(%p, %p): stub\n", monitor, timingReport);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI GetVCPFeatureAndVCPFeatureReply( HMONITOR monitor, BYTE vcpCode, LPMC_VCP_CODE_TYPE pvct,
                                             LPDWORD current, LPDWORD maximum )
{
    FIXME("(%p, 0x%02x, %p, %p, %p): stub\n", monitor, vcpCode, pvct, current, maximum);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

HRESULT WINAPI OPMGetVideoOutputsFromHMONITOR( HMONITOR monitor, /* OPM_VIDEO_OUTPUT_SEMANTICS */ int vos,
                                               ULONG *numVideoOutputs, /* IOPMVideoOutput */ void ***videoOutputs )
{
    FIXME("(%p, 0x%x, %p, %p): stub\n", monitor, vos, numVideoOutputs, videoOutputs);

    return E_NOTIMPL;
}

HRESULT WINAPI OPMGetVideoOutputsFromIDirect3DDevice9Object( IDirect3DDevice9 *device, /* OPM_VIDEO_OUTPUT_SEMANTICS */ int vos,
                                                             ULONG *numVideoOutputs,  /* IOPMVideoOutput */ void ***videoOutputs )
{
    FIXME("(%p, 0x%x, %p, %p): stub\n", device, vos, numVideoOutputs, videoOutputs);

    return E_NOTIMPL;
}

BOOL WINAPI RestoreMonitorFactoryColorDefaults( HMONITOR monitor )
{
    FIXME("(%p): stub\n", monitor);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI RestoreMonitorFactoryDefaults( HMONITOR monitor )
{
    FIXME("(%p): stub\n", monitor);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SaveCurrentMonitorSettings( HMONITOR monitor )
{
    FIXME("(%p): stub\n", monitor);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SaveCurrentSettings( HMONITOR monitor )
{
    FIXME("(%p): stub\n", monitor);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SetMonitorBrightness( HMONITOR monitor, DWORD brightness )
{
    FIXME("%p, %#lx: stub.\n", monitor, brightness);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SetMonitorColorTemperature( HMONITOR monitor, MC_COLOR_TEMPERATURE temperature )
{
    FIXME("(%p, 0x%x): stub\n", monitor, temperature);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SetMonitorContrast( HMONITOR monitor, DWORD contrast )
{
    FIXME("%p, %#lx: stub.\n", monitor, contrast);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SetMonitorDisplayAreaPosition( HMONITOR monitor, MC_POSITION_TYPE type, DWORD position )
{
    FIXME("%p, 0x%x, %#lx: stub.\n", monitor, type, position);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SetMonitorDisplayAreaSize( HMONITOR monitor, MC_SIZE_TYPE type, DWORD size )
{
    FIXME("%p, 0x%x, %#lx: stub.\n", monitor, type, size);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SetMonitorRedGreenOrBlueDrive( HMONITOR monitor, MC_DRIVE_TYPE type, DWORD drive )
{
    FIXME("%p, 0x%x, %#lx: stub.\n", monitor, type, drive);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SetMonitorRedGreenOrBlueGain( HMONITOR monitor, MC_GAIN_TYPE type, DWORD gain )
{
    FIXME("%p, 0x%x, %#lx: stub.\n", monitor, type, gain);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SetVCPFeature( HMONITOR monitor, BYTE vcpCode, DWORD value )
{
    FIXME("%p, 0x%02x, %#lx: stub.\n", monitor, vcpCode, value);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


HRESULT WINAPI DXVAHD_CreateDevice(IDirect3DDevice9Ex *d3d9_device, const DXVAHD_CONTENT_DESC *desc,
        DXVAHD_DEVICE_USAGE usage, PDXVAHDSW_Plugin plugin, IDXVAHD_Device **device)
{
    FIXME("d3d9_device %p, desc %p, usage %u, plugin %p, device %p, stub!\n", d3d9_device, desc, usage, plugin, device);

    return E_NOTIMPL;
}
