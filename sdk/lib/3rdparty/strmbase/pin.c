/*
 * Generic Implementation of IPin Interface
 *
 * Copyright 2003 Robert Shearman
 * Copyright 2010 Aric Stewart, CodeWeavers
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

#include "strmbase_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static const IMemInputPinVtbl MemInputPin_Vtbl;

typedef HRESULT (*SendPinFunc)( IPin *to, LPVOID arg );

struct enum_media_types
{
    IEnumMediaTypes IEnumMediaTypes_iface;
    LONG refcount;

    unsigned int index, count;
    struct strmbase_pin *pin;
};

static const IEnumMediaTypesVtbl enum_media_types_vtbl;

static HRESULT enum_media_types_create(struct strmbase_pin *pin, IEnumMediaTypes **out)
{
    struct enum_media_types *object;
    AM_MEDIA_TYPE mt;

    if (!out)
        return E_POINTER;

    if (!(object = heap_alloc_zero(sizeof(*object))))
    {
        *out = NULL;
        return E_OUTOFMEMORY;
    }

    object->IEnumMediaTypes_iface.lpVtbl = &enum_media_types_vtbl;
    object->refcount = 1;
    object->pin = pin;
    IPin_AddRef(&pin->IPin_iface);

    if (pin->ops->pin_get_media_type)
    {
        while (pin->ops->pin_get_media_type(pin, object->count, &mt) == S_OK)
        {
            FreeMediaType(&mt);
            ++object->count;
        }
    }

    TRACE("Created enumerator %p.\n", object);
    *out = &object->IEnumMediaTypes_iface;

    return S_OK;
}

static struct enum_media_types *impl_from_IEnumMediaTypes(IEnumMediaTypes *iface)
{
    return CONTAINING_RECORD(iface, struct enum_media_types, IEnumMediaTypes_iface);
}

static HRESULT WINAPI enum_media_types_QueryInterface(IEnumMediaTypes *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IEnumMediaTypes))
    {
        IEnumMediaTypes_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI enum_media_types_AddRef(IEnumMediaTypes *iface)
{
    struct enum_media_types *enummt = impl_from_IEnumMediaTypes(iface);
    ULONG refcount = InterlockedIncrement(&enummt->refcount);
    TRACE("%p increasing refcount to %lu.\n", enummt, refcount);
    return refcount;
}

static ULONG WINAPI enum_media_types_Release(IEnumMediaTypes *iface)
{
    struct enum_media_types *enummt = impl_from_IEnumMediaTypes(iface);
    ULONG refcount = InterlockedDecrement(&enummt->refcount);

    TRACE("%p decreasing refcount to %lu.\n", enummt, refcount);
    if (!refcount)
    {
        IPin_Release(&enummt->pin->IPin_iface);
        heap_free(enummt);
    }
    return refcount;
}

static HRESULT WINAPI enum_media_types_Next(IEnumMediaTypes *iface, ULONG count,
        AM_MEDIA_TYPE **mts, ULONG *ret_count)
{
    struct enum_media_types *enummt = impl_from_IEnumMediaTypes(iface);
    AM_MEDIA_TYPE mt;
    unsigned int i;
    HRESULT hr;

    TRACE("enummt %p, count %lu, mts %p, ret_count %p.\n", enummt, count, mts, ret_count);

    if (!enummt->pin->ops->pin_get_media_type)
    {
        if (ret_count)
            *ret_count = 0;
        return count ? S_FALSE : S_OK;
    }

    for (i = 0; i < count; ++i)
    {
        hr = enummt->pin->ops->pin_get_media_type(enummt->pin, enummt->index + i, &mt);
        if (hr == S_OK)
        {
            if ((mts[i] = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE))))
                *mts[i] = mt;
            else
                hr = E_OUTOFMEMORY;
        }
        if (FAILED(hr))
        {
            while (i--)
                DeleteMediaType(mts[i]);
            *ret_count = 0;
            return E_OUTOFMEMORY;
        }
        else if (hr != S_OK)
            break;

        if (TRACE_ON(quartz))
        {
            TRACE("Returning media type %u:\n", enummt->index + i);
            strmbase_dump_media_type(mts[i]);
        }
    }

    if (count != 1 || ret_count)
        *ret_count = i;
    enummt->index += i;
    return i == count ? S_OK : S_FALSE;
}

static HRESULT WINAPI enum_media_types_Skip(IEnumMediaTypes *iface, ULONG count)
{
    struct enum_media_types *enummt = impl_from_IEnumMediaTypes(iface);

    TRACE("enummt %p, count %lu.\n", enummt, count);

    enummt->index += count;

    return enummt->index > enummt->count ? S_FALSE : S_OK;
}

static HRESULT WINAPI enum_media_types_Reset(IEnumMediaTypes *iface)
{
    struct enum_media_types *enummt = impl_from_IEnumMediaTypes(iface);
    AM_MEDIA_TYPE mt;

    TRACE("enummt %p.\n", enummt);

    enummt->count = 0;
    if (enummt->pin->ops->pin_get_media_type)
    {
        while (enummt->pin->ops->pin_get_media_type(enummt->pin, enummt->count, &mt) == S_OK)
        {
            FreeMediaType(&mt);
            ++enummt->count;
        }
    }

    enummt->index = 0;

    return S_OK;
}

static HRESULT WINAPI enum_media_types_Clone(IEnumMediaTypes *iface, IEnumMediaTypes **out)
{
    struct enum_media_types *enummt = impl_from_IEnumMediaTypes(iface);
    HRESULT hr;

    TRACE("enummt %p, out %p.\n", enummt, out);

    if (FAILED(hr = enum_media_types_create(enummt->pin, out)))
        return hr;
    return IEnumMediaTypes_Skip(*out, enummt->index);
}

static const IEnumMediaTypesVtbl enum_media_types_vtbl =
{
    enum_media_types_QueryInterface,
    enum_media_types_AddRef,
    enum_media_types_Release,
    enum_media_types_Next,
    enum_media_types_Skip,
    enum_media_types_Reset,
    enum_media_types_Clone,
};

static inline struct strmbase_pin *impl_from_IPin(IPin *iface)
{
    return CONTAINING_RECORD(iface, struct strmbase_pin, IPin_iface);
}

/** Helper function, there are a lot of places where the error code is inherited
 * The following rules apply:
 *
 * Return the first received error code (E_NOTIMPL is ignored)
 * If no errors occur: return the first received non-error-code that isn't S_OK
 */
static HRESULT updatehres( HRESULT original, HRESULT new )
{
    if (FAILED( original ) || new == E_NOTIMPL)
        return original;

    if (FAILED( new ) || original == S_OK)
        return new;

    return original;
}

/** Sends a message from a pin further to other, similar pins
 * fnMiddle is called on each pin found further on the stream.
 * fnEnd (can be NULL) is called when the message can't be sent any further (this is a renderer or source)
 *
 * If the pin given is an input pin, the message will be sent downstream to other input pins
 * If the pin given is an output pin, the message will be sent upstream to other output pins
 */
static HRESULT SendFurther(struct strmbase_sink *sink, SendPinFunc func, void *arg)
{
    struct strmbase_pin *pin;
    HRESULT hr = S_OK;
    unsigned int i;

    for (i = 0; (pin = sink->pin.filter->ops->filter_get_pin(sink->pin.filter, i)); ++i)
    {
        if (pin->dir == PINDIR_OUTPUT && pin->peer)
            hr = updatehres(hr, func(pin->peer, arg));
    }

    return hr;
}

static HRESULT WINAPI pin_QueryInterface(IPin *iface, REFIID iid, void **out)
{
    struct strmbase_pin *pin = impl_from_IPin(iface);
    HRESULT hr;

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    *out = NULL;

    if (pin->ops->pin_query_interface && SUCCEEDED(hr = pin->ops->pin_query_interface(pin, iid, out)))
        return hr;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IPin))
        *out = iface;
    else
    {
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI pin_AddRef(IPin *iface)
{
    struct strmbase_pin *pin = impl_from_IPin(iface);
    return IBaseFilter_AddRef(&pin->filter->IBaseFilter_iface);
}

static ULONG WINAPI pin_Release(IPin *iface)
{
    struct strmbase_pin *pin = impl_from_IPin(iface);
    return IBaseFilter_Release(&pin->filter->IBaseFilter_iface);
}

static HRESULT WINAPI pin_ConnectedTo(IPin * iface, IPin ** ppPin)
{
    struct strmbase_pin *This = impl_from_IPin(iface);
    HRESULT hr;

    TRACE("pin %p %s:%s, peer %p.\n", This, debugstr_w(This->filter->name), debugstr_w(This->name), ppPin);

    EnterCriticalSection(&This->filter->filter_cs);
    {
        if (This->peer)
        {
            *ppPin = This->peer;
            IPin_AddRef(*ppPin);
            hr = S_OK;
        }
        else
        {
            hr = VFW_E_NOT_CONNECTED;
            *ppPin = NULL;
        }
    }
    LeaveCriticalSection(&This->filter->filter_cs);

    return hr;
}

static HRESULT WINAPI pin_ConnectionMediaType(IPin *iface, AM_MEDIA_TYPE *pmt)
{
    struct strmbase_pin *This = impl_from_IPin(iface);
    HRESULT hr;

    TRACE("pin %p %s:%s, pmt %p.\n", This, debugstr_w(This->filter->name), debugstr_w(This->name), pmt);

    EnterCriticalSection(&This->filter->filter_cs);
    {
        if (This->peer)
        {
            CopyMediaType(pmt, &This->mt);
            strmbase_dump_media_type(pmt);
            hr = S_OK;
        }
        else
        {
            ZeroMemory(pmt, sizeof(*pmt));
            hr = VFW_E_NOT_CONNECTED;
        }
    }
    LeaveCriticalSection(&This->filter->filter_cs);

    return hr;
}

static HRESULT WINAPI pin_QueryPinInfo(IPin *iface, PIN_INFO *info)
{
    struct strmbase_pin *pin = impl_from_IPin(iface);

    TRACE("pin %p %s:%s, info %p.\n", pin, debugstr_w(pin->filter->name), debugstr_w(pin->name), info);

    info->dir = pin->dir;
    IBaseFilter_AddRef(info->pFilter = &pin->filter->IBaseFilter_iface);
    lstrcpyW(info->achName, pin->name);

    return S_OK;
}

static HRESULT WINAPI pin_QueryDirection(IPin *iface, PIN_DIRECTION *dir)
{
    struct strmbase_pin *pin = impl_from_IPin(iface);

    TRACE("pin %p %s:%s, dir %p.\n", pin, debugstr_w(pin->filter->name), debugstr_w(pin->name), dir);

    *dir = pin->dir;

    return S_OK;
}

static HRESULT WINAPI pin_QueryId(IPin *iface, WCHAR **id)
{
    struct strmbase_pin *pin = impl_from_IPin(iface);

    TRACE("pin %p %s:%s, id %p.\n", pin, debugstr_w(pin->filter->name), debugstr_w(pin->name), id);

    if (!(*id = CoTaskMemAlloc((lstrlenW(pin->id) + 1) * sizeof(WCHAR))))
        return E_OUTOFMEMORY;

    lstrcpyW(*id, pin->id);

    return S_OK;
}

static BOOL query_accept(struct strmbase_pin *pin, const AM_MEDIA_TYPE *mt)
{
    if (pin->ops->pin_query_accept && pin->ops->pin_query_accept(pin, mt) != S_OK)
        return FALSE;
    return TRUE;
}

static HRESULT WINAPI pin_QueryAccept(IPin *iface, const AM_MEDIA_TYPE *mt)
{
    struct strmbase_pin *pin = impl_from_IPin(iface);

    TRACE("pin %p %s:%s, mt %p.\n", pin, debugstr_w(pin->filter->name), debugstr_w(pin->name), mt);
    strmbase_dump_media_type(mt);

    return query_accept(pin, mt) ? S_OK : S_FALSE;
}

static HRESULT WINAPI pin_EnumMediaTypes(IPin *iface, IEnumMediaTypes **enum_media_types)
{
    struct strmbase_pin *pin = impl_from_IPin(iface);
    AM_MEDIA_TYPE mt;
    HRESULT hr;

    TRACE("pin %p %s:%s, enum_media_types %p.\n", pin, debugstr_w(pin->filter->name),
            debugstr_w(pin->name), enum_media_types);

    if (pin->ops->pin_get_media_type)
    {
        if (FAILED(hr = pin->ops->pin_get_media_type(pin, 0, &mt)))
            return hr;
        if (hr == S_OK)
            FreeMediaType(&mt);
    }

    return enum_media_types_create(pin, enum_media_types);
}

static HRESULT WINAPI pin_QueryInternalConnections(IPin *iface, IPin **pins, ULONG *count)
{
    struct strmbase_pin *pin = impl_from_IPin(iface);

    TRACE("pin %p %s:%s, pins %p, count %p.\n", pin, debugstr_w(pin->filter->name),
            debugstr_w(pin->name), pins, count);

    return E_NOTIMPL; /* to tell caller that all input pins connected to all output pins */
}

/*** OutputPin implementation ***/

static inline struct strmbase_source *impl_source_from_IPin( IPin *iface )
{
    return CONTAINING_RECORD(iface, struct strmbase_source, pin.IPin_iface);
}

static BOOL compare_media_types(const AM_MEDIA_TYPE *req_mt, const AM_MEDIA_TYPE *pin_mt)
{
    if (!req_mt)
        return TRUE;

    if (!IsEqualGUID(&req_mt->majortype, &pin_mt->majortype)
            && !IsEqualGUID(&req_mt->majortype, &GUID_NULL))
        return FALSE;

    if (!IsEqualGUID(&req_mt->subtype, &pin_mt->subtype)
            && !IsEqualGUID(&req_mt->subtype, &GUID_NULL))
        return FALSE;

    if (!IsEqualGUID(&req_mt->formattype, &pin_mt->formattype)
            && !IsEqualGUID(&req_mt->formattype, &GUID_NULL))
        return FALSE;

    return TRUE;
}

static HRESULT WINAPI source_Connect(IPin *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    struct strmbase_source *pin = impl_source_from_IPin(iface);
    AM_MEDIA_TYPE candidate, *candidate_ptr;
    IEnumMediaTypes *enummt;
    PIN_DIRECTION dir;
    unsigned int i;
    ULONG count;
    HRESULT hr;

    TRACE("pin %p %s:%s, peer %p, mt %p.\n", pin, debugstr_w(pin->pin.filter->name),
            debugstr_w(pin->pin.name), peer, mt);
    strmbase_dump_media_type(mt);

    if (!peer)
        return E_POINTER;

    IPin_QueryDirection(peer, &dir);
    if (dir != PINDIR_INPUT)
    {
        WARN("Attempt to connect to another source pin, returning VFW_E_INVALID_DIRECTION.\n");
        return VFW_E_INVALID_DIRECTION;
    }

    EnterCriticalSection(&pin->pin.filter->filter_cs);

    if (pin->pin.peer)
    {
        LeaveCriticalSection(&pin->pin.filter->filter_cs);
        WARN("Pin is already connected, returning VFW_E_ALREADY_CONNECTED.\n");
        return VFW_E_ALREADY_CONNECTED;
    }

    if (pin->pin.filter->state != State_Stopped)
    {
        LeaveCriticalSection(&pin->pin.filter->filter_cs);
        WARN("Filter is not stopped; returning VFW_E_NOT_STOPPED.\n");
        return VFW_E_NOT_STOPPED;
    }

    /* We don't check the subtype here. The rationale (as given by the DirectX
     * documentation) is that the format type is supposed to provide at least
     * as much information as the subtype. */
    if (mt && !IsEqualGUID(&mt->majortype, &GUID_NULL)
            && !IsEqualGUID(&mt->formattype, &GUID_NULL))
    {
        hr = pin->pFuncsTable->pfnAttemptConnection(pin, peer, mt);
        LeaveCriticalSection(&pin->pin.filter->filter_cs);
        return hr;
    }

    if (SUCCEEDED(IPin_EnumMediaTypes(peer, &enummt)))
    {
        while (IEnumMediaTypes_Next(enummt, 1, &candidate_ptr, &count) == S_OK)
        {
            if (compare_media_types(mt, candidate_ptr)
                    && pin->pFuncsTable->pfnAttemptConnection(pin, peer, candidate_ptr) == S_OK)
            {
                LeaveCriticalSection(&pin->pin.filter->filter_cs);
                DeleteMediaType(candidate_ptr);
                IEnumMediaTypes_Release(enummt);
                return S_OK;
            }
            DeleteMediaType(candidate_ptr);
        }

        IEnumMediaTypes_Release(enummt);
    }

    if (pin->pFuncsTable->base.pin_get_media_type)
    {
        for (i = 0; pin->pFuncsTable->base.pin_get_media_type(&pin->pin, i, &candidate) == S_OK; ++i)
        {
            strmbase_dump_media_type(&candidate);
            if (compare_media_types(mt, &candidate)
                    && pin->pFuncsTable->pfnAttemptConnection(pin, peer, &candidate) == S_OK)
            {
                LeaveCriticalSection(&pin->pin.filter->filter_cs);
                FreeMediaType(&candidate);
                return S_OK;
            }
            FreeMediaType(&candidate);
        }
    }

    LeaveCriticalSection(&pin->pin.filter->filter_cs);

    return VFW_E_NO_ACCEPTABLE_TYPES;
}

static HRESULT WINAPI source_ReceiveConnection(IPin *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    struct strmbase_source *pin = impl_source_from_IPin(iface);

    WARN("pin %p %s:%s, peer %p, mt %p, unexpected call.\n", pin,
            debugstr_w(pin->pin.filter->name), debugstr_w(pin->pin.name), peer, mt);

    return E_UNEXPECTED;
}

static HRESULT WINAPI source_Disconnect(IPin *iface)
{
    HRESULT hr;
    struct strmbase_source *This = impl_source_from_IPin(iface);

    TRACE("pin %p %s:%s.\n", This, debugstr_w(This->pin.filter->name), debugstr_w(This->pin.name));

    EnterCriticalSection(&This->pin.filter->filter_cs);
    {
        if (This->pin.filter->state != State_Stopped)
        {
            LeaveCriticalSection(&This->pin.filter->filter_cs);
            WARN("Filter is not stopped; returning VFW_E_NOT_STOPPED.\n");
            return VFW_E_NOT_STOPPED;
        }

        if (This->pFuncsTable->source_disconnect)
            This->pFuncsTable->source_disconnect(This);

        if (This->pMemInputPin)
        {
            IMemInputPin_Release(This->pMemInputPin);
            This->pMemInputPin = NULL;
        }

        if (This->pAllocator)
        {
            IMemAllocator_Release(This->pAllocator);
            This->pAllocator = NULL;
        }

        if (This->pin.peer)
        {
            IPin_Release(This->pin.peer);
            This->pin.peer = NULL;
            FreeMediaType(&This->pin.mt);
            ZeroMemory(&This->pin.mt, sizeof(This->pin.mt));
            hr = S_OK;
        }
        else
            hr = S_FALSE;
    }
    LeaveCriticalSection(&This->pin.filter->filter_cs);

    return hr;
}

static HRESULT WINAPI source_EndOfStream(IPin *iface)
{
    struct strmbase_source *pin = impl_source_from_IPin(iface);

    WARN("pin %p %s:%s, unexpected call.\n", pin, debugstr_w(pin->pin.filter->name), debugstr_w(pin->pin.name));

    /* not supposed to do anything in an output pin */

    return E_UNEXPECTED;
}

static HRESULT WINAPI source_BeginFlush(IPin *iface)
{
    struct strmbase_source *pin = impl_source_from_IPin(iface);

    WARN("pin %p %s:%s, unexpected call.\n", pin, debugstr_w(pin->pin.filter->name), debugstr_w(pin->pin.name));

    /* not supposed to do anything in an output pin */

    return E_UNEXPECTED;
}

static HRESULT WINAPI source_EndFlush(IPin *iface)
{
    struct strmbase_source *pin = impl_source_from_IPin(iface);

    WARN("pin %p %s:%s, unexpected call.\n", pin, debugstr_w(pin->pin.filter->name), debugstr_w(pin->pin.name));

    /* not supposed to do anything in an output pin */

    return E_UNEXPECTED;
}

static HRESULT WINAPI source_NewSegment(IPin * iface, REFERENCE_TIME start, REFERENCE_TIME stop, double rate)
{
    struct strmbase_source *pin = impl_source_from_IPin(iface);

    TRACE("pin %p %s:%s, start %s, stop %s, rate %.16e.\n", pin, debugstr_w(pin->pin.filter->name),
            debugstr_w(pin->pin.name), debugstr_time(start), debugstr_time(stop), rate);

    return S_OK;
}

static const IPinVtbl source_vtbl =
{
    pin_QueryInterface,
    pin_AddRef,
    pin_Release,
    source_Connect,
    source_ReceiveConnection,
    source_Disconnect,
    pin_ConnectedTo,
    pin_ConnectionMediaType,
    pin_QueryPinInfo,
    pin_QueryDirection,
    pin_QueryId,
    pin_QueryAccept,
    pin_EnumMediaTypes,
    pin_QueryInternalConnections,
    source_EndOfStream,
    source_BeginFlush,
    source_EndFlush,
    source_NewSegment,
};

HRESULT WINAPI BaseOutputPinImpl_DecideAllocator(struct strmbase_source *This,
        IMemInputPin *pPin, IMemAllocator **pAlloc)
{
    HRESULT hr;

    hr = IMemInputPin_GetAllocator(pPin, pAlloc);

    if (hr == VFW_E_NO_ALLOCATOR)
        hr = CoCreateInstance(&CLSID_MemoryAllocator, NULL,
                CLSCTX_INPROC_SERVER, &IID_IMemAllocator, (void **)pAlloc);

    if (SUCCEEDED(hr))
    {
        ALLOCATOR_PROPERTIES rProps;
        ZeroMemory(&rProps, sizeof(ALLOCATOR_PROPERTIES));

        IMemInputPin_GetAllocatorRequirements(pPin, &rProps);
        hr = This->pFuncsTable->pfnDecideBufferSize(This, *pAlloc, &rProps);
    }

    if (SUCCEEDED(hr))
        hr = IMemInputPin_NotifyAllocator(pPin, *pAlloc, FALSE);

    return hr;
}

/*** The Construct functions ***/

/* Function called as a helper to IPin_Connect */
/* specific AM_MEDIA_TYPE - it cannot be NULL */
HRESULT WINAPI BaseOutputPinImpl_AttemptConnection(struct strmbase_source *This,
        IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    HRESULT hr;
    IMemAllocator * pMemAlloc = NULL;

    TRACE("(%p)->(%p, %p)\n", This, pReceivePin, pmt);

    if (!query_accept(&This->pin, pmt))
        return VFW_E_TYPE_NOT_ACCEPTED;

    This->pin.peer = pReceivePin;
    IPin_AddRef(pReceivePin);
    CopyMediaType(&This->pin.mt, pmt);

    hr = IPin_ReceiveConnection(pReceivePin, &This->pin.IPin_iface, pmt);

    /* get the IMemInputPin interface we will use to deliver samples to the
     * connected pin */
    if (SUCCEEDED(hr))
    {
        This->pMemInputPin = NULL;
        hr = IPin_QueryInterface(pReceivePin, &IID_IMemInputPin, (LPVOID)&This->pMemInputPin);

        if (SUCCEEDED(hr))
        {
            hr = This->pFuncsTable->pfnDecideAllocator(This, This->pMemInputPin, &pMemAlloc);
            if (SUCCEEDED(hr))
                This->pAllocator = pMemAlloc;
            else if (pMemAlloc)
                IMemAllocator_Release(pMemAlloc);
        }

        /* break connection if we couldn't get the allocator */
        if (FAILED(hr))
        {
            if (This->pMemInputPin)
                IMemInputPin_Release(This->pMemInputPin);
            This->pMemInputPin = NULL;

            IPin_Disconnect(pReceivePin);
        }
    }

    if (FAILED(hr))
    {
        IPin_Release(This->pin.peer);
        This->pin.peer = NULL;
        FreeMediaType(&This->pin.mt);
    }

    TRACE("Returning %#lx.\n", hr);
    return hr;
}

void strmbase_source_init(struct strmbase_source *pin, struct strmbase_filter *filter,
        const WCHAR *name, const struct strmbase_source_ops *func_table)
{
    memset(pin, 0, sizeof(*pin));
    pin->pin.IPin_iface.lpVtbl = &source_vtbl;
    pin->pin.filter = filter;
    pin->pin.dir = PINDIR_OUTPUT;
    lstrcpyW(pin->pin.name, name);
    lstrcpyW(pin->pin.id, name);
    pin->pin.ops = &func_table->base;
    pin->pFuncsTable = func_table;
}

void strmbase_source_cleanup(struct strmbase_source *pin)
{
    FreeMediaType(&pin->pin.mt);
    if (pin->pAllocator)
        IMemAllocator_Release(pin->pAllocator);
    pin->pAllocator = NULL;
}

static struct strmbase_sink *impl_sink_from_IPin(IPin *iface)
{
    return CONTAINING_RECORD(iface, struct strmbase_sink, pin.IPin_iface);
}

static HRESULT WINAPI sink_Connect(IPin *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    struct strmbase_sink *pin = impl_sink_from_IPin(iface);

    WARN("pin %p %s:%s, peer %p, mt %p, unexpected call.\n", pin, debugstr_w(pin->pin.name),
            debugstr_w(pin->pin.filter->name), peer, mt);

    return E_UNEXPECTED;
}


static HRESULT WINAPI sink_ReceiveConnection(IPin *iface, IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    struct strmbase_sink *This = impl_sink_from_IPin(iface);
    PIN_DIRECTION pindirReceive;
    HRESULT hr = S_OK;

    TRACE("pin %p %s:%s, peer %p, mt %p.\n", This, debugstr_w(This->pin.filter->name),
            debugstr_w(This->pin.name), pReceivePin, pmt);
    strmbase_dump_media_type(pmt);

    if (!pmt)
        return E_POINTER;

    EnterCriticalSection(&This->pin.filter->filter_cs);
    {
        if (This->pin.filter->state != State_Stopped)
        {
            LeaveCriticalSection(&This->pin.filter->filter_cs);
            WARN("Filter is not stopped; returning VFW_E_NOT_STOPPED.\n");
            return VFW_E_NOT_STOPPED;
        }

        if (This->pin.peer)
            hr = VFW_E_ALREADY_CONNECTED;

        if (SUCCEEDED(hr) && !query_accept(&This->pin, pmt))
            hr = VFW_E_TYPE_NOT_ACCEPTED; /* FIXME: shouldn't we just map common errors onto
                                           * VFW_E_TYPE_NOT_ACCEPTED and pass the value on otherwise? */

        if (SUCCEEDED(hr))
        {
            IPin_QueryDirection(pReceivePin, &pindirReceive);

            if (pindirReceive != PINDIR_OUTPUT)
            {
                ERR("Can't connect from non-output pin\n");
                hr = VFW_E_INVALID_DIRECTION;
            }
        }

        if (SUCCEEDED(hr) && This->pFuncsTable->sink_connect)
            hr = This->pFuncsTable->sink_connect(This, pReceivePin, pmt);

        if (SUCCEEDED(hr))
        {
            CopyMediaType(&This->pin.mt, pmt);
            This->pin.peer = pReceivePin;
            IPin_AddRef(pReceivePin);
        }
    }
    LeaveCriticalSection(&This->pin.filter->filter_cs);

    return hr;
}

static HRESULT WINAPI sink_Disconnect(IPin *iface)
{
    struct strmbase_sink *pin = impl_sink_from_IPin(iface);
    HRESULT hr;

    TRACE("pin %p %s:%s.\n", pin, debugstr_w(pin->pin.filter->name), debugstr_w(pin->pin.name));

    EnterCriticalSection(&pin->pin.filter->filter_cs);

    if (pin->pin.filter->state != State_Stopped)
    {
        LeaveCriticalSection(&pin->pin.filter->filter_cs);
        WARN("Filter is not stopped; returning VFW_E_NOT_STOPPED.\n");
        return VFW_E_NOT_STOPPED;
    }

    if (pin->pin.peer)
    {
        if (pin->pFuncsTable->sink_disconnect)
            pin->pFuncsTable->sink_disconnect(pin);

        if (pin->pAllocator)
        {
            IMemAllocator_Release(pin->pAllocator);
            pin->pAllocator = NULL;
        }

        IPin_Release(pin->pin.peer);
        pin->pin.peer = NULL;
        FreeMediaType(&pin->pin.mt);
        memset(&pin->pin.mt, 0, sizeof(AM_MEDIA_TYPE));
        hr = S_OK;
    }
    else
        hr = S_FALSE;

    LeaveCriticalSection(&pin->pin.filter->filter_cs);

    return hr;
}

static HRESULT deliver_endofstream(IPin* pin, LPVOID unused)
{
    return IPin_EndOfStream( pin );
}

static HRESULT WINAPI sink_EndOfStream(IPin *iface)
{
    struct strmbase_sink *pin = impl_sink_from_IPin(iface);
    HRESULT hr = S_OK;

    TRACE("pin %p %s:%s.\n", pin, debugstr_w(pin->pin.filter->name), debugstr_w(pin->pin.name));

    if (pin->pFuncsTable->sink_eos)
    {
        EnterCriticalSection(&pin->pin.filter->stream_cs);
        hr = pin->pFuncsTable->sink_eos(pin);
        LeaveCriticalSection(&pin->pin.filter->stream_cs);
        return hr;
    }

    EnterCriticalSection(&pin->pin.filter->filter_cs);
    if (pin->flushing)
        hr = S_FALSE;
    LeaveCriticalSection(&pin->pin.filter->filter_cs);

    if (hr == S_OK)
        hr = SendFurther(pin, deliver_endofstream, NULL);
    return hr;
}

static HRESULT deliver_beginflush(IPin* pin, LPVOID unused)
{
    return IPin_BeginFlush( pin );
}

static HRESULT WINAPI sink_BeginFlush(IPin *iface)
{
    struct strmbase_sink *pin = impl_sink_from_IPin(iface);
    HRESULT hr;

    TRACE("pin %p %s:%s.\n", pin, debugstr_w(pin->pin.filter->name), debugstr_w(pin->pin.name));

    EnterCriticalSection(&pin->pin.filter->filter_cs);

    pin->flushing = TRUE;

    if (pin->pFuncsTable->sink_begin_flush)
        hr = pin->pFuncsTable->sink_begin_flush(pin);
    else
        hr = SendFurther(pin, deliver_beginflush, NULL);

    LeaveCriticalSection(&pin->pin.filter->filter_cs);

    return hr;
}

static HRESULT deliver_endflush(IPin* pin, LPVOID unused)
{
    return IPin_EndFlush( pin );
}

static HRESULT WINAPI sink_EndFlush(IPin * iface)
{
    struct strmbase_sink *pin = impl_sink_from_IPin(iface);
    HRESULT hr;

    TRACE("pin %p %s:%s.\n", pin, debugstr_w(pin->pin.filter->name), debugstr_w(pin->pin.name));

    EnterCriticalSection(&pin->pin.filter->filter_cs);

    pin->flushing = FALSE;

    if (pin->pFuncsTable->sink_end_flush)
        hr = pin->pFuncsTable->sink_end_flush(pin);
    else
        hr = SendFurther(pin, deliver_endflush, NULL);

    LeaveCriticalSection(&pin->pin.filter->filter_cs);

    return hr;
}

typedef struct newsegmentargs
{
    REFERENCE_TIME tStart, tStop;
    double rate;
} newsegmentargs;

static HRESULT deliver_newsegment(IPin *pin, LPVOID data)
{
    newsegmentargs *args = data;
    return IPin_NewSegment(pin, args->tStart, args->tStop, args->rate);
}

static HRESULT WINAPI sink_NewSegment(IPin *iface, REFERENCE_TIME start, REFERENCE_TIME stop, double rate)
{
    struct strmbase_sink *pin = impl_sink_from_IPin(iface);
    newsegmentargs args;

    TRACE("pin %p %s:%s, start %s, stop %s, rate %.16e.\n", pin, debugstr_w(pin->pin.filter->name),
            debugstr_w(pin->pin.name), debugstr_time(start), debugstr_time(stop), rate);

    if (pin->pFuncsTable->sink_new_segment)
        return pin->pFuncsTable->sink_new_segment(pin, start, stop, rate);

    args.tStart = start;
    args.tStop = stop;
    args.rate = rate;

    return SendFurther(pin, deliver_newsegment, &args);
}

static const IPinVtbl sink_vtbl =
{
    pin_QueryInterface,
    pin_AddRef,
    pin_Release,
    sink_Connect,
    sink_ReceiveConnection,
    sink_Disconnect,
    pin_ConnectedTo,
    pin_ConnectionMediaType,
    pin_QueryPinInfo,
    pin_QueryDirection,
    pin_QueryId,
    pin_QueryAccept,
    pin_EnumMediaTypes,
    pin_QueryInternalConnections,
    sink_EndOfStream,
    sink_BeginFlush,
    sink_EndFlush,
    sink_NewSegment,
};

/*** IMemInputPin implementation ***/

static inline struct strmbase_sink *impl_from_IMemInputPin(IMemInputPin *iface)
{
    return CONTAINING_RECORD(iface, struct strmbase_sink, IMemInputPin_iface);
}

static HRESULT WINAPI MemInputPin_QueryInterface(IMemInputPin * iface, REFIID riid, LPVOID * ppv)
{
    struct strmbase_sink *This = impl_from_IMemInputPin(iface);

    return IPin_QueryInterface(&This->pin.IPin_iface, riid, ppv);
}

static ULONG WINAPI MemInputPin_AddRef(IMemInputPin * iface)
{
    struct strmbase_sink *This = impl_from_IMemInputPin(iface);

    return IPin_AddRef(&This->pin.IPin_iface);
}

static ULONG WINAPI MemInputPin_Release(IMemInputPin * iface)
{
    struct strmbase_sink *This = impl_from_IMemInputPin(iface);

    return IPin_Release(&This->pin.IPin_iface);
}

static HRESULT WINAPI MemInputPin_GetAllocator(IMemInputPin * iface, IMemAllocator ** ppAllocator)
{
    struct strmbase_sink *This = impl_from_IMemInputPin(iface);

    TRACE("pin %p %s:%s, allocator %p.\n", This, debugstr_w(This->pin.filter->name),
            debugstr_w(This->pin.name), ppAllocator);

    *ppAllocator = This->pAllocator;
    if (*ppAllocator)
        IMemAllocator_AddRef(*ppAllocator);

    return *ppAllocator ? S_OK : VFW_E_NO_ALLOCATOR;
}

static HRESULT WINAPI MemInputPin_NotifyAllocator(IMemInputPin * iface, IMemAllocator * pAllocator, BOOL bReadOnly)
{
    struct strmbase_sink *This = impl_from_IMemInputPin(iface);

    TRACE("pin %p %s:%s, allocator %p, read_only %d.\n", This, debugstr_w(This->pin.filter->name),
            debugstr_w(This->pin.name), pAllocator, bReadOnly);

    if (bReadOnly)
        FIXME("Read only flag not handled yet!\n");

    /* FIXME: Should we release the allocator on disconnection? */
    if (!pAllocator)
    {
        WARN("Null allocator\n");
        return E_POINTER;
    }

    if (This->preferred_allocator && pAllocator != This->preferred_allocator)
        return E_FAIL;

    if (This->pAllocator)
        IMemAllocator_Release(This->pAllocator);
    This->pAllocator = pAllocator;
    if (This->pAllocator)
        IMemAllocator_AddRef(This->pAllocator);

    return S_OK;
}

static HRESULT WINAPI MemInputPin_GetAllocatorRequirements(IMemInputPin *iface, ALLOCATOR_PROPERTIES *props)
{
    struct strmbase_sink *pin = impl_from_IMemInputPin(iface);

    TRACE("pin %p %s:%s, props %p.\n", pin, debugstr_w(pin->pin.filter->name),
            debugstr_w(pin->pin.name), props);

    /* override this method if you have any specific requirements */

    return E_NOTIMPL;
}

static HRESULT WINAPI MemInputPin_Receive(IMemInputPin *iface, IMediaSample *sample)
{
    struct strmbase_sink *pin = impl_from_IMemInputPin(iface);
    HRESULT hr = S_FALSE;

    TRACE("pin %p %s:%s, sample %p.\n", pin, debugstr_w(pin->pin.filter->name),
            debugstr_w(pin->pin.name), sample);

    if (pin->pFuncsTable->pfnReceive)
    {
        EnterCriticalSection(&pin->pin.filter->stream_cs);
        hr = pin->pFuncsTable->pfnReceive(pin, sample);
        LeaveCriticalSection(&pin->pin.filter->stream_cs);
    }
    return hr;
}

static HRESULT WINAPI MemInputPin_ReceiveMultiple(IMemInputPin * iface, IMediaSample ** pSamples, LONG nSamples, LONG *nSamplesProcessed)
{
    HRESULT hr = S_OK;

    for (*nSamplesProcessed = 0; *nSamplesProcessed < nSamples; (*nSamplesProcessed)++)
    {
        hr = IMemInputPin_Receive(iface, pSamples[*nSamplesProcessed]);
        if (hr != S_OK)
            break;
    }

    return hr;
}

static HRESULT WINAPI MemInputPin_ReceiveCanBlock(IMemInputPin * iface)
{
    struct strmbase_sink *pin = impl_from_IMemInputPin(iface);

    TRACE("pin %p %s:%s.\n", pin, debugstr_w(pin->pin.filter->name), debugstr_w(pin->pin.name));

    return S_OK;
}

static const IMemInputPinVtbl MemInputPin_Vtbl =
{
    MemInputPin_QueryInterface,
    MemInputPin_AddRef,
    MemInputPin_Release,
    MemInputPin_GetAllocator,
    MemInputPin_NotifyAllocator,
    MemInputPin_GetAllocatorRequirements,
    MemInputPin_Receive,
    MemInputPin_ReceiveMultiple,
    MemInputPin_ReceiveCanBlock
};

void strmbase_sink_init(struct strmbase_sink *pin, struct strmbase_filter *filter,
        const WCHAR *name, const struct strmbase_sink_ops *func_table, IMemAllocator *allocator)
{
    memset(pin, 0, sizeof(*pin));
    pin->pin.IPin_iface.lpVtbl = &sink_vtbl;
    pin->pin.filter = filter;
    pin->pin.dir = PINDIR_INPUT;
    lstrcpyW(pin->pin.name, name);
    lstrcpyW(pin->pin.id, name);
    pin->pin.ops = &func_table->base;
    pin->pFuncsTable = func_table;
    pin->pAllocator = pin->preferred_allocator = allocator;
    if (pin->preferred_allocator)
        IMemAllocator_AddRef(pin->preferred_allocator);
    pin->IMemInputPin_iface.lpVtbl = &MemInputPin_Vtbl;
}

void strmbase_sink_cleanup(struct strmbase_sink *pin)
{
    FreeMediaType(&pin->pin.mt);
    if (pin->pAllocator)
        IMemAllocator_Release(pin->pAllocator);
    pin->pAllocator = NULL;
    pin->pin.IPin_iface.lpVtbl = NULL;
}
