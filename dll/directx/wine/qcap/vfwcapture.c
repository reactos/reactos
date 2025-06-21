/* Video For Windows Steering structure
 *
 * Copyright 2005 Maarten Lankhorst
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
 *
 */

#include "qcap_private.h"
#include "winternl.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

#define V4L_CALL( func, params ) WINE_UNIX_CALL( unix_ ## func, params )

struct vfw_capture
{
    struct strmbase_filter filter;
    IAMStreamConfig IAMStreamConfig_iface;
    IAMVideoControl IAMVideoControl_iface;
    IAMVideoProcAmp IAMVideoProcAmp_iface;
    IAMFilterMiscFlags IAMFilterMiscFlags_iface;
    IPersistPropertyBag IPersistPropertyBag_iface;
    BOOL init;

    struct strmbase_source source;
    IKsPropertySet IKsPropertySet_iface;

    video_capture_device_t device;

    /* FIXME: It would be nice to avoid duplicating this variable with strmbase.
     * However, synchronization is tricky; we need access to be protected by a
     * separate lock. */
    FILTER_STATE state;
    CONDITION_VARIABLE state_cv;
    CRITICAL_SECTION state_cs;

    HANDLE thread;
};

static inline struct vfw_capture *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct vfw_capture, filter);
}

static inline struct vfw_capture *impl_from_IAMStreamConfig(IAMStreamConfig *iface)
{
    return CONTAINING_RECORD(iface, struct vfw_capture, IAMStreamConfig_iface);
}

static inline struct vfw_capture *impl_from_IAMVideoControl(IAMVideoControl *iface)
{
    return CONTAINING_RECORD(iface, struct vfw_capture, IAMVideoControl_iface);
}

static inline struct vfw_capture *impl_from_IAMVideoProcAmp(IAMVideoProcAmp *iface)
{
    return CONTAINING_RECORD(iface, struct vfw_capture, IAMVideoProcAmp_iface);
}

static inline struct vfw_capture *impl_from_IAMFilterMiscFlags(IAMFilterMiscFlags *iface)
{
    return CONTAINING_RECORD(iface, struct vfw_capture, IAMFilterMiscFlags_iface);
}

static inline struct vfw_capture *impl_from_IPersistPropertyBag(IPersistPropertyBag *iface)
{
    return CONTAINING_RECORD(iface, struct vfw_capture, IPersistPropertyBag_iface);
}

static struct strmbase_pin *vfw_capture_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    struct vfw_capture *filter = impl_from_strmbase_filter(iface);

    if (index >= 1)
        return NULL;

    return &filter->source.pin;
}

static void vfw_capture_destroy(struct strmbase_filter *iface)
{
    struct vfw_capture *filter = impl_from_strmbase_filter(iface);

    if (filter->init)
    {
        struct destroy_params params = { filter->device };
        V4L_CALL( destroy, &params );
    }

    if (filter->source.pin.peer)
    {
        IPin_Disconnect(filter->source.pin.peer);
        IPin_Disconnect(&filter->source.pin.IPin_iface);
    }
    filter->state_cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&filter->state_cs);
    strmbase_source_cleanup(&filter->source);
    strmbase_filter_cleanup(&filter->filter);
    free(filter);
}

static HRESULT vfw_capture_query_interface(struct strmbase_filter *iface, REFIID iid, void **out)
{
    struct vfw_capture *filter = impl_from_strmbase_filter(iface);

    if (IsEqualGUID(iid, &IID_IPersistPropertyBag))
        *out = &filter->IPersistPropertyBag_iface;
    else if (IsEqualGUID(iid, &IID_IAMVideoControl))
        *out = &filter->IAMVideoControl_iface;
    else if (IsEqualGUID(iid, &IID_IAMVideoProcAmp))
        *out = &filter->IAMVideoProcAmp_iface;
    else if (IsEqualGUID(iid, &IID_IAMFilterMiscFlags))
        *out = &filter->IAMFilterMiscFlags_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static unsigned int get_image_size(struct vfw_capture *filter)
{
    const VIDEOINFOHEADER *format = (const VIDEOINFOHEADER *)filter->source.pin.mt.pbFormat;

    return format->bmiHeader.biWidth * format->bmiHeader.biHeight * format->bmiHeader.biBitCount / 8;
}

static DWORD WINAPI stream_thread(void *arg)
{
    struct vfw_capture *filter = arg;
    const unsigned int image_size = get_image_size(filter);
    struct read_frame_params params;

    for (;;)
    {
        IMediaSample *sample;
        HRESULT hr;
        BYTE *data;

        EnterCriticalSection(&filter->state_cs);

        while (filter->state == State_Paused)
            SleepConditionVariableCS(&filter->state_cv, &filter->state_cs, INFINITE);

        if (filter->state == State_Stopped)
        {
            LeaveCriticalSection(&filter->state_cs);
            break;
        }

        LeaveCriticalSection(&filter->state_cs);

        if (FAILED(hr = IMemAllocator_GetBuffer(filter->source.pAllocator, &sample, NULL, NULL, 0)))
        {
            ERR("Failed to get sample, hr %#lx.\n", hr);
            break;
        }

        IMediaSample_SetActualDataLength(sample, image_size);
        IMediaSample_GetPointer(sample, &data);

        params.device = filter->device;
        params.data = data;
        if (!V4L_CALL( read_frame, &params ))
        {
            IMediaSample_Release(sample);
            break;
        }

        hr = IMemInputPin_Receive(filter->source.pMemInputPin, sample);
        IMediaSample_Release(sample);
        if (FAILED(hr))
        {
            ERR("IMemInputPin::Receive() returned %#lx.\n", hr);
            break;
        }
    }

    return 0;
}

static HRESULT vfw_capture_init_stream(struct strmbase_filter *iface)
{
    struct vfw_capture *filter = impl_from_strmbase_filter(iface);
    HRESULT hr;

    if (!filter->source.pin.peer)
        return S_OK;

    if (FAILED(hr = IMemAllocator_Commit(filter->source.pAllocator)))
        ERR("Failed to commit allocator, hr %#lx.\n", hr);

    EnterCriticalSection(&filter->state_cs);
    filter->state = State_Paused;
    LeaveCriticalSection(&filter->state_cs);

    filter->thread = CreateThread(NULL, 0, stream_thread, filter, 0, NULL);

    return S_OK;
}

static HRESULT vfw_capture_start_stream(struct strmbase_filter *iface, REFERENCE_TIME time)
{
    struct vfw_capture *filter = impl_from_strmbase_filter(iface);
    struct start_params params;
    HRESULT hr;

    if (!filter->source.pin.peer)
        return S_OK;

    params.device = filter->device;
    if (FAILED(hr = V4L_CALL( start, &params )))
    {
        ERR("start stream failed.\n");
        return hr;
    }

    EnterCriticalSection(&filter->state_cs);
    filter->state = State_Running;
    LeaveCriticalSection(&filter->state_cs);
    WakeConditionVariable(&filter->state_cv);
    return S_OK;
}

static HRESULT vfw_capture_stop_stream(struct strmbase_filter *iface)
{
    struct vfw_capture *filter = impl_from_strmbase_filter(iface);

    if (!filter->source.pin.peer)
        return S_OK;

    EnterCriticalSection(&filter->state_cs);
    filter->state = State_Paused;
    LeaveCriticalSection(&filter->state_cs);
    return S_OK;
}

static HRESULT vfw_capture_cleanup_stream(struct strmbase_filter *iface)
{
    struct vfw_capture *filter = impl_from_strmbase_filter(iface);
    HRESULT hr;

    if (!filter->source.pin.peer)
        return S_OK;

    EnterCriticalSection(&filter->state_cs);
    filter->state = State_Stopped;
    LeaveCriticalSection(&filter->state_cs);
    WakeConditionVariable(&filter->state_cv);

    WaitForSingleObject(filter->thread, INFINITE);
    CloseHandle(filter->thread);
    filter->thread = NULL;

    hr = IMemAllocator_Decommit(filter->source.pAllocator);
    if (hr != S_OK && hr != VFW_E_NOT_COMMITTED)
        ERR("Failed to decommit allocator, hr %#lx.\n", hr);

    return S_OK;
}

static HRESULT vfw_capture_wait_state(struct strmbase_filter *iface, DWORD timeout)
{
    struct vfw_capture *filter = impl_from_strmbase_filter(iface);

    if (filter->source.pin.peer && filter->filter.state == State_Paused)
        return VFW_S_CANT_CUE;
    return S_OK;
}

static const struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = vfw_capture_get_pin,
    .filter_destroy = vfw_capture_destroy,
    .filter_query_interface = vfw_capture_query_interface,
    .filter_init_stream = vfw_capture_init_stream,
    .filter_start_stream = vfw_capture_start_stream,
    .filter_stop_stream = vfw_capture_stop_stream,
    .filter_cleanup_stream = vfw_capture_cleanup_stream,
    .filter_wait_state = vfw_capture_wait_state,
};

static HRESULT WINAPI AMStreamConfig_QueryInterface(IAMStreamConfig *iface, REFIID iid, void **out)
{
    struct vfw_capture *filter = impl_from_IAMStreamConfig(iface);
    return IPin_QueryInterface(&filter->source.pin.IPin_iface, iid, out);
}

static ULONG WINAPI AMStreamConfig_AddRef(IAMStreamConfig *iface)
{
    struct vfw_capture *filter = impl_from_IAMStreamConfig(iface);
    return IPin_AddRef(&filter->source.pin.IPin_iface);
}

static ULONG WINAPI AMStreamConfig_Release(IAMStreamConfig *iface)
{
    struct vfw_capture *filter = impl_from_IAMStreamConfig(iface);
    return IPin_Release(&filter->source.pin.IPin_iface);
}

static HRESULT WINAPI
AMStreamConfig_SetFormat(IAMStreamConfig *iface, AM_MEDIA_TYPE *pmt)
{
    struct vfw_capture *This = impl_from_IAMStreamConfig(iface);
    struct set_format_params params;
    HRESULT hr;

    TRACE("filter %p, mt %p.\n", This, pmt);
    strmbase_dump_media_type(pmt);

    if (This->filter.state != State_Stopped)
    {
        TRACE("Returning not stopped error\n");
        return VFW_E_NOT_STOPPED;
    }

    if (!pmt)
    {
        TRACE("pmt is NULL\n");
        return E_POINTER;
    }

    if (!IsEqualGUID(&pmt->majortype, &MEDIATYPE_Video))
        return E_FAIL;

    if (This->source.pin.peer)
    {
        hr = IPin_QueryAccept(This->source.pin.peer, pmt);
        TRACE("QueryAccept() returned %#lx.\n", hr);
        if (hr == S_FALSE)
            return VFW_E_INVALIDMEDIATYPE;
    }

    params.device = This->device;
    params.mt = pmt;
    hr = V4L_CALL( set_format, &params );
    if (SUCCEEDED(hr) && This->filter.graph && This->source.pin.peer)
    {
        hr = IFilterGraph_Reconnect(This->filter.graph, &This->source.pin.IPin_iface);
        if (SUCCEEDED(hr))
            TRACE("Reconnection completed, with new media format..\n");
    }
    TRACE("Returning %#lx.\n", hr);
    return hr;
}

static HRESULT WINAPI AMStreamConfig_GetFormat(IAMStreamConfig *iface, AM_MEDIA_TYPE **mt)
{
    struct vfw_capture *filter = impl_from_IAMStreamConfig(iface);
    VIDEOINFOHEADER *format;
    HRESULT hr;

    TRACE("filter %p, mt %p.\n", filter, mt);

    if (!(*mt = CoTaskMemAlloc(sizeof(**mt))))
        return E_OUTOFMEMORY;

    EnterCriticalSection(&filter->filter.filter_cs);

    if (filter->source.pin.peer)
    {
        hr = CopyMediaType(*mt, &filter->source.pin.mt);
    }
    else
    {
        if ((format = CoTaskMemAlloc(sizeof(VIDEOINFOHEADER))))
        {
            struct get_format_params params = { filter->device, *mt, format };
            V4L_CALL( get_format, &params );
            (*mt)->cbFormat = sizeof(VIDEOINFOHEADER);
            (*mt)->pbFormat = (BYTE *)format;
            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    LeaveCriticalSection(&filter->filter.filter_cs);

    if (SUCCEEDED(hr))
        strmbase_dump_media_type(*mt);
    else
        CoTaskMemFree(*mt);
    return hr;
}

static HRESULT WINAPI AMStreamConfig_GetNumberOfCapabilities(IAMStreamConfig *iface,
        int *count, int *size)
{
    struct vfw_capture *filter = impl_from_IAMStreamConfig(iface);
    struct get_caps_count_params params = { filter->device, count };

    TRACE("filter %p, count %p, size %p.\n", filter, count, size);

    if (!count || !size)
        return E_POINTER;

    V4L_CALL( get_caps_count, &params );
    *size = sizeof(VIDEO_STREAM_CONFIG_CAPS);

    return S_OK;
}

static HRESULT WINAPI AMStreamConfig_GetStreamCaps(IAMStreamConfig *iface,
        int index, AM_MEDIA_TYPE **pmt, BYTE *vscc)
{
    struct vfw_capture *filter = impl_from_IAMStreamConfig(iface);
    VIDEOINFOHEADER *format;
    AM_MEDIA_TYPE *mt;
    int count;
    struct get_caps_count_params count_params = { filter->device, &count };
    struct get_caps_params caps_params;

    TRACE("filter %p, index %d, pmt %p, vscc %p.\n", filter, index, pmt, vscc);

    V4L_CALL( get_caps_count, &count_params );
    if (index > count)
        return S_FALSE;

    if (!(mt = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE))))
        return E_OUTOFMEMORY;

    if (!(format = CoTaskMemAlloc(sizeof(VIDEOINFOHEADER))))
    {
        CoTaskMemFree(mt);
        return E_OUTOFMEMORY;
    }

    caps_params.device = filter->device;
    caps_params.index  = index;
    caps_params.mt     = mt;
    caps_params.format = format;
    caps_params.caps   = (VIDEO_STREAM_CONFIG_CAPS *)vscc;
    V4L_CALL( get_caps, &caps_params );
    mt->cbFormat = sizeof(VIDEOINFOHEADER);
    mt->pbFormat = (BYTE *)format;
    *pmt = mt;
    return S_OK;
}

static const IAMStreamConfigVtbl IAMStreamConfig_VTable =
{
    AMStreamConfig_QueryInterface,
    AMStreamConfig_AddRef,
    AMStreamConfig_Release,
    AMStreamConfig_SetFormat,
    AMStreamConfig_GetFormat,
    AMStreamConfig_GetNumberOfCapabilities,
    AMStreamConfig_GetStreamCaps
};

static HRESULT WINAPI AMVideoProcAmp_QueryInterface(IAMVideoProcAmp *iface, REFIID iid, void **out)
{
    struct vfw_capture *filter = impl_from_IAMVideoProcAmp(iface);
    return IUnknown_QueryInterface(filter->filter.outer_unk, iid, out);
}

static ULONG WINAPI AMVideoProcAmp_AddRef(IAMVideoProcAmp * iface)
{
    struct vfw_capture *filter = impl_from_IAMVideoProcAmp(iface);
    return IUnknown_AddRef(filter->filter.outer_unk);
}

static ULONG WINAPI AMVideoProcAmp_Release(IAMVideoProcAmp * iface)
{
    struct vfw_capture *filter = impl_from_IAMVideoProcAmp(iface);
    return IUnknown_Release(filter->filter.outer_unk);
}

static HRESULT WINAPI AMVideoProcAmp_GetRange(IAMVideoProcAmp *iface, LONG property,
        LONG *min, LONG *max, LONG *step, LONG *default_value, LONG *flags)
{
    struct vfw_capture *filter = impl_from_IAMVideoProcAmp(iface);
    struct get_prop_range_params params = { filter->device, property, min, max, step, default_value, flags };

    TRACE("filter %p, property %#lx, min %p, max %p, step %p, default_value %p, flags %p.\n",
            filter, property, min, max, step, default_value, flags);

    return V4L_CALL( get_prop_range, &params );
}

static HRESULT WINAPI AMVideoProcAmp_Set(IAMVideoProcAmp *iface, LONG property,
        LONG value, LONG flags)
{
    struct vfw_capture *filter = impl_from_IAMVideoProcAmp(iface);
    struct set_prop_params params = { filter->device, property, value, flags };

    TRACE("filter %p, property %#lx, value %ld, flags %#lx.\n", filter, property, value, flags);

    return V4L_CALL( set_prop, &params );
}

static HRESULT WINAPI AMVideoProcAmp_Get(IAMVideoProcAmp *iface, LONG property,
        LONG *value, LONG *flags)
{
    struct vfw_capture *filter = impl_from_IAMVideoProcAmp(iface);
    struct get_prop_params params = { filter->device, property, value, flags };

    TRACE("filter %p, property %#lx, value %p, flags %p.\n", filter, property, value, flags);

    return V4L_CALL( get_prop, &params );
}

static const IAMVideoProcAmpVtbl IAMVideoProcAmp_VTable =
{
    AMVideoProcAmp_QueryInterface,
    AMVideoProcAmp_AddRef,
    AMVideoProcAmp_Release,
    AMVideoProcAmp_GetRange,
    AMVideoProcAmp_Set,
    AMVideoProcAmp_Get,
};

static HRESULT WINAPI PPB_QueryInterface(IPersistPropertyBag *iface, REFIID iid, void **out)
{
    struct vfw_capture *filter = impl_from_IPersistPropertyBag(iface);
    return IUnknown_QueryInterface(filter->filter.outer_unk, iid, out);
}

static ULONG WINAPI PPB_AddRef(IPersistPropertyBag * iface)
{
    struct vfw_capture *filter = impl_from_IPersistPropertyBag(iface);
    return IUnknown_AddRef(filter->filter.outer_unk);
}

static ULONG WINAPI PPB_Release(IPersistPropertyBag * iface)
{
    struct vfw_capture *filter = impl_from_IPersistPropertyBag(iface);
    return IUnknown_Release(filter->filter.outer_unk);
}

static HRESULT WINAPI
PPB_GetClassID( IPersistPropertyBag * iface, CLSID * pClassID )
{
    struct vfw_capture *This = impl_from_IPersistPropertyBag(iface);

    FIXME("%p - stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI PPB_InitNew(IPersistPropertyBag * iface)
{
    struct vfw_capture *This = impl_from_IPersistPropertyBag(iface);

    FIXME("%p - stub\n", This);

    return E_NOTIMPL;
}

static HRESULT WINAPI PPB_Load(IPersistPropertyBag *iface, IPropertyBag *bag, IErrorLog *error_log)
{
    struct vfw_capture *filter = impl_from_IPersistPropertyBag(iface);
    struct create_params params;
    HRESULT hr;
    VARIANT var;

    TRACE("filter %p, bag %p, error_log %p.\n", filter, bag, error_log);

    V_VT(&var) = VT_I4;
    if (FAILED(hr = IPropertyBag_Read(bag, L"VFWIndex", &var, error_log)))
        return hr;

    params.index = V_I4(&var);
    params.device = &filter->device;
    hr = V4L_CALL( create, &params );

    if (SUCCEEDED(hr)) filter->init = TRUE;
    return hr;
}

static HRESULT WINAPI
PPB_Save( IPersistPropertyBag * iface, IPropertyBag *pPropBag,
          BOOL fClearDirty, BOOL fSaveAllProperties )
{
    struct vfw_capture *This = impl_from_IPersistPropertyBag(iface);
    FIXME("%p - stub\n", This);
    return E_NOTIMPL;
}

static const IPersistPropertyBagVtbl IPersistPropertyBag_VTable =
{
    PPB_QueryInterface,
    PPB_AddRef,
    PPB_Release,
    PPB_GetClassID,
    PPB_InitNew,
    PPB_Load,
    PPB_Save
};

/* IKsPropertySet interface */
static inline struct vfw_capture *impl_from_IKsPropertySet(IKsPropertySet *iface)
{
    return CONTAINING_RECORD(iface, struct vfw_capture, IKsPropertySet_iface);
}

static HRESULT WINAPI KSP_QueryInterface(IKsPropertySet *iface, REFIID iid, void **out)
{
    struct vfw_capture *filter = impl_from_IKsPropertySet(iface);
    return IPin_QueryInterface(&filter->source.pin.IPin_iface, iid, out);
}

static ULONG WINAPI KSP_AddRef(IKsPropertySet * iface)
{
    struct vfw_capture *filter = impl_from_IKsPropertySet(iface);
    return IPin_AddRef(&filter->source.pin.IPin_iface);
}

static ULONG WINAPI KSP_Release(IKsPropertySet * iface)
{
    struct vfw_capture *filter = impl_from_IKsPropertySet(iface);
    return IPin_Release(&filter->source.pin.IPin_iface);
}

static HRESULT WINAPI
KSP_Set( IKsPropertySet * iface, REFGUID guidPropSet, DWORD dwPropID,
         LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData,
         DWORD cbPropData )
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI
KSP_Get( IKsPropertySet * iface, REFGUID guidPropSet, DWORD dwPropID,
         LPVOID pInstanceData, DWORD cbInstanceData, LPVOID pPropData,
         DWORD cbPropData, DWORD *pcbReturned )
{
    LPGUID pGuid;

    TRACE("()\n");

    if (!IsEqualIID(guidPropSet, &AMPROPSETID_Pin))
        return E_PROP_SET_UNSUPPORTED;
    if (pPropData == NULL && pcbReturned == NULL)
        return E_POINTER;
    if (pcbReturned)
        *pcbReturned = sizeof(GUID);
    if (pPropData == NULL)
        return S_OK;
    if (cbPropData < sizeof(GUID))
        return E_UNEXPECTED;
    pGuid = pPropData;
    *pGuid = PIN_CATEGORY_CAPTURE;
    FIXME("() Not adding a pin with PIN_CATEGORY_PREVIEW\n");
    return S_OK;
}

static HRESULT WINAPI
KSP_QuerySupported( IKsPropertySet * iface, REFGUID guidPropSet,
                    DWORD dwPropID, DWORD *pTypeSupport )
{
   FIXME("%p: stub\n", iface);
   return E_NOTIMPL;
}

static const IKsPropertySetVtbl IKsPropertySet_VTable =
{
   KSP_QueryInterface,
   KSP_AddRef,
   KSP_Release,
   KSP_Set,
   KSP_Get,
   KSP_QuerySupported
};

static inline struct vfw_capture *impl_from_strmbase_pin(struct strmbase_pin *pin)
{
    return CONTAINING_RECORD(pin, struct vfw_capture, source.pin);
}

static HRESULT source_query_accept(struct strmbase_pin *pin, const AM_MEDIA_TYPE *mt)
{
    struct vfw_capture *filter = impl_from_strmbase_pin(pin);
    struct check_format_params params = { filter->device, mt };

    if (!mt) return E_POINTER;
    return V4L_CALL( check_format, &params );
}

static HRESULT source_get_media_type(struct strmbase_pin *pin,
        unsigned int index, AM_MEDIA_TYPE *mt)
{
    struct vfw_capture *filter = impl_from_strmbase_pin(pin);
    struct get_media_type_params params;
    VIDEOINFOHEADER *format;
    HRESULT hr;

    if (!(format = CoTaskMemAlloc(sizeof(*format))))
        return E_OUTOFMEMORY;

    params.device = filter->device;
    params.index  = index;
    params.mt     = mt;
    params.format = format;
    if ((hr = V4L_CALL( get_media_type, &params )) != S_OK)
    {
        CoTaskMemFree(format);
        return hr;
    }
    mt->cbFormat = sizeof(VIDEOINFOHEADER);
    mt->pbFormat = (BYTE *)format;
    return S_OK;
}

static HRESULT source_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct vfw_capture *filter = impl_from_strmbase_pin(iface);

    if (IsEqualGUID(iid, &IID_IKsPropertySet))
        *out = &filter->IKsPropertySet_iface;
    else if (IsEqualGUID(iid, &IID_IAMStreamConfig))
        *out = &filter->IAMStreamConfig_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT WINAPI VfwPin_DecideBufferSize(struct strmbase_source *iface,
        IMemAllocator *allocator, ALLOCATOR_PROPERTIES *req_props)
{
    struct vfw_capture *filter = impl_from_strmbase_pin(&iface->pin);
    ALLOCATOR_PROPERTIES ret_props;

    if (!req_props->cBuffers)
        req_props->cBuffers = 3;
    if (!req_props->cbBuffer)
        req_props->cbBuffer = get_image_size(filter);
    if (!req_props->cbAlign)
        req_props->cbAlign = 1;

    return IMemAllocator_SetProperties(allocator, req_props, &ret_props);
}

static const struct strmbase_source_ops source_ops =
{
    .base.pin_query_accept = source_query_accept,
    .base.pin_get_media_type = source_get_media_type,
    .base.pin_query_interface = source_query_interface,
    .pfnAttemptConnection = BaseOutputPinImpl_AttemptConnection,
    .pfnDecideBufferSize = VfwPin_DecideBufferSize,
    .pfnDecideAllocator = BaseOutputPinImpl_DecideAllocator,
};

static HRESULT WINAPI misc_flags_QueryInterface(IAMFilterMiscFlags *iface, REFIID riid, void **ppv)
{
    struct vfw_capture *filter = impl_from_IAMFilterMiscFlags(iface);
    return IUnknown_QueryInterface(filter->filter.outer_unk, riid, ppv);
}

static ULONG WINAPI misc_flags_AddRef(IAMFilterMiscFlags *iface)
{
    struct vfw_capture *filter = impl_from_IAMFilterMiscFlags(iface);
    return IUnknown_AddRef(filter->filter.outer_unk);
}

static ULONG WINAPI misc_flags_Release(IAMFilterMiscFlags *iface)
{
    struct vfw_capture *filter = impl_from_IAMFilterMiscFlags(iface);
    return IUnknown_Release(filter->filter.outer_unk);
}

static ULONG WINAPI misc_flags_GetMiscFlags(IAMFilterMiscFlags *iface)
{
    return AM_FILTER_MISC_FLAGS_IS_SOURCE;
}

static const IAMFilterMiscFlagsVtbl IAMFilterMiscFlags_VTable =
{
    misc_flags_QueryInterface,
    misc_flags_AddRef,
    misc_flags_Release,
    misc_flags_GetMiscFlags
};

static HRESULT WINAPI video_control_QueryInterface(IAMVideoControl *iface, REFIID riid, void **ppv)
{
    struct vfw_capture *filter = impl_from_IAMVideoControl(iface);
    return IUnknown_QueryInterface(filter->filter.outer_unk, riid, ppv);
}

static ULONG WINAPI video_control_AddRef(IAMVideoControl *iface)
{
    struct vfw_capture *filter = impl_from_IAMVideoControl(iface);
    return IUnknown_AddRef(filter->filter.outer_unk);
}

static ULONG WINAPI video_control_Release(IAMVideoControl *iface)
{
    struct vfw_capture *filter = impl_from_IAMVideoControl(iface);
    return IUnknown_Release(filter->filter.outer_unk);
}

static HRESULT WINAPI video_control_GetCaps(IAMVideoControl *iface, IPin *pin, LONG *flags)
{
    struct vfw_capture *filter = impl_from_IAMVideoControl(iface);

    FIXME("filter %p, pin %p, flags %p: stub.\n", filter, pin, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_control_SetMode(IAMVideoControl *iface, IPin *pin, LONG mode)
{
    struct vfw_capture *filter = impl_from_IAMVideoControl(iface);

    FIXME("filter %p, pin %p, mode %ld, stub.\n", filter, pin, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_control_GetMode(IAMVideoControl *iface, IPin *pin, LONG *mode)
{
    struct vfw_capture *filter = impl_from_IAMVideoControl(iface);

    FIXME("filter %p, pin %p, mode %p: stub.\n", filter, pin, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_control_GetCurrentActualFrameRate(IAMVideoControl *iface, IPin *pin,
        LONGLONG *frame_rate)
{
    struct vfw_capture *filter = impl_from_IAMVideoControl(iface);

    FIXME("filter %p, pin %p, frame rate %p: stub.\n", filter, pin, frame_rate);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_control_GetMaxAvailableFrameRate(IAMVideoControl *iface, IPin *pin,
        LONG index, SIZE dimensions, LONGLONG *frame_rate)
{
    struct vfw_capture *filter = impl_from_IAMVideoControl(iface);

    FIXME("filter %p, pin %p, index %ld, dimensions (%ldx%ld), frame rate %p, stub.\n",
            filter, pin, index, dimensions.cx, dimensions.cy, frame_rate);

    return E_NOTIMPL;
}

static HRESULT WINAPI video_control_GetFrameRateList(IAMVideoControl *iface, IPin *pin, LONG index,
        SIZE dimensions, LONG *list_size, LONGLONG **frame_rate)
{
    struct vfw_capture *filter = impl_from_IAMVideoControl(iface);

    FIXME("filter %p, pin %p, index %ld, dimensions (%ldx%ld), list size %p, frame rate %p, stub.\n",
            filter, pin, index, dimensions.cx, dimensions.cy, list_size, frame_rate);

    return E_NOTIMPL;
}

static const IAMVideoControlVtbl IAMVideoControl_VTable =
{
    video_control_QueryInterface,
    video_control_AddRef,
    video_control_Release,
    video_control_GetCaps,
    video_control_SetMode,
    video_control_GetMode,
    video_control_GetCurrentActualFrameRate,
    video_control_GetMaxAvailableFrameRate,
    video_control_GetFrameRateList
};

static BOOL WINAPI load_capture_funcs(INIT_ONCE *once, void *param, void **context)
{
    __wine_init_unix_call();
    return TRUE;
}

static INIT_ONCE init_once = INIT_ONCE_STATIC_INIT;

HRESULT vfw_capture_create(IUnknown *outer, IUnknown **out)
{
    struct vfw_capture *object;

    if (!InitOnceExecuteOnce(&init_once, load_capture_funcs, NULL, NULL) || !__wine_unixlib_handle)
        return E_FAIL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    strmbase_filter_init(&object->filter, outer, &CLSID_VfwCapture, &filter_ops);

    object->IAMStreamConfig_iface.lpVtbl = &IAMStreamConfig_VTable;
    object->IAMVideoControl_iface.lpVtbl = &IAMVideoControl_VTable;
    object->IAMVideoProcAmp_iface.lpVtbl = &IAMVideoProcAmp_VTable;
    object->IAMFilterMiscFlags_iface.lpVtbl = &IAMFilterMiscFlags_VTable;
    object->IPersistPropertyBag_iface.lpVtbl = &IPersistPropertyBag_VTable;

    strmbase_source_init(&object->source, &object->filter, L"Output", &source_ops);

    object->IKsPropertySet_iface.lpVtbl = &IKsPropertySet_VTable;

    object->state = State_Stopped;
    InitializeConditionVariable(&object->state_cv);
    InitializeCriticalSectionEx(&object->state_cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    object->state_cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": vfw_capture.state_cs");

    TRACE("Created VFW capture filter %p.\n", object);
    *out = &object->filter.IUnknown_inner;
    return S_OK;
}
