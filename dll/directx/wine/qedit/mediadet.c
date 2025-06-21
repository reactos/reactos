/*              DirectShow Media Detector object (QEDIT.DLL)
 *
 * Copyright 2008 Google (Lei Zhang, Dan Hipschman)
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
#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "qedit_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

typedef struct MediaDetImpl {
    IUnknown IUnknown_inner;
    IMediaDet IMediaDet_iface;
    IUnknown *outer_unk;
    LONG ref;
    IGraphBuilder *graph;
    IBaseFilter *source;
    IBaseFilter *splitter;
    WCHAR *filename;
    LONG num_streams;
    LONG cur_stream;
    IPin *cur_pin;
} MediaDetImpl;

static inline MediaDetImpl *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, MediaDetImpl, IUnknown_inner);
}

static inline MediaDetImpl *impl_from_IMediaDet(IMediaDet *iface)
{
    return CONTAINING_RECORD(iface, MediaDetImpl, IMediaDet_iface);
}

static void MD_cleanup(MediaDetImpl *This)
{
    if (This->cur_pin) IPin_Release(This->cur_pin);
    This->cur_pin = NULL;
    if (This->source) IBaseFilter_Release(This->source);
    This->source = NULL;
    if (This->splitter) IBaseFilter_Release(This->splitter);
    This->splitter = NULL;
    if (This->graph) IGraphBuilder_Release(This->graph);
    This->graph = NULL;
    free(This->filename);
    This->filename = NULL;
    This->num_streams = -1;
    This->cur_stream = 0;
}

static HRESULT get_filter_info(IMoniker *moniker, GUID *clsid, VARIANT *var)
{
    IPropertyBag *prop_bag;
    HRESULT hr;

    if (FAILED(hr = IMoniker_BindToStorage(moniker, NULL, NULL, &IID_IPropertyBag, (void **)&prop_bag)))
    {
        ERR("Failed to get property bag, hr %#lx.\n", hr);
        return hr;
    }

    VariantInit(var);
    V_VT(var) = VT_BSTR;
    if (FAILED(hr = IPropertyBag_Read(prop_bag, L"CLSID", var, NULL)))
    {
        ERR("Failed to get CLSID, hr %#lx.\n", hr);
        IPropertyBag_Release(prop_bag);
        return hr;
    }
    CLSIDFromString(V_BSTR(var), clsid);
    VariantClear(var);

    if (FAILED(hr = IPropertyBag_Read(prop_bag, L"FriendlyName", var, NULL)))
        ERR("Failed to get name, hr %#lx.\n", hr);

    IPropertyBag_Release(prop_bag);
    return hr;
}

static HRESULT get_pin_media_type(IPin *pin, AM_MEDIA_TYPE *out)
{
    IEnumMediaTypes *enummt;
    AM_MEDIA_TYPE *pmt;
    HRESULT hr;

    if (FAILED(hr = IPin_EnumMediaTypes(pin, &enummt)))
        return hr;
    hr = IEnumMediaTypes_Next(enummt, 1, &pmt, NULL);
    IEnumMediaTypes_Release(enummt);
    if (hr != S_OK)
        return E_NOINTERFACE;

    *out = *pmt;
    CoTaskMemFree(pmt);
    return S_OK;
}

static HRESULT find_splitter(MediaDetImpl *detector)
{
    IPin *source_pin, *splitter_pin;
    IEnumMoniker *enum_moniker;
    IFilterMapper2 *mapper;
    IBaseFilter *splitter;
    IEnumPins *enum_pins;
    AM_MEDIA_TYPE mt;
    IMoniker *mon;
    GUID type[2];
    VARIANT var;
    HRESULT hr;
    GUID clsid;

    if (FAILED(hr = IBaseFilter_EnumPins(detector->source, &enum_pins)))
    {
        ERR("Failed to enumerate source pins, hr %#lx.\n", hr);
        return hr;
    }
    hr = IEnumPins_Next(enum_pins, 1, &source_pin, NULL);
    IEnumPins_Release(enum_pins);
    if (hr != S_OK)
    {
        ERR("Failed to get source pin, hr %#lx.\n", hr);
        return hr;
    }

    if (FAILED(hr = get_pin_media_type(source_pin, &mt)))
    {
        ERR("Failed to get media type, hr %#lx.\n", hr);
        IPin_Release(source_pin);
        return hr;
    }

    type[0] = mt.majortype;
    type[1] = mt.subtype;
    FreeMediaType(&mt);

    if (FAILED(hr = CoCreateInstance(&CLSID_FilterMapper2, NULL,
            CLSCTX_INPROC_SERVER, &IID_IFilterMapper2, (void **)&mapper)))
    {
        IPin_Release(source_pin);
        return hr;
    }

    hr = IFilterMapper2_EnumMatchingFilters(mapper, &enum_moniker, 0, TRUE,
            MERIT_UNLIKELY, FALSE, 1, type, NULL, NULL, FALSE, TRUE, 0, NULL, NULL, NULL);
    IFilterMapper2_Release(mapper);
    if (FAILED(hr))
    {
        IPin_Release(source_pin);
        return hr;
    }

    hr = E_NOINTERFACE;
    while (IEnumMoniker_Next(enum_moniker, 1, &mon, NULL) == S_OK)
    {
        hr = get_filter_info(mon, &clsid, &var);
        IMoniker_Release(mon);
        if (FAILED(hr))
            continue;

        hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER,
                &IID_IBaseFilter, (void **)&splitter);
        if (FAILED(hr))
        {
            VariantClear(&var);
            continue;
        }

        hr = IGraphBuilder_AddFilter(detector->graph, splitter, V_BSTR(&var));
        VariantClear(&var);
        if (FAILED(hr))
        {
            IBaseFilter_Release(splitter);
            continue;
        }

        hr = IBaseFilter_EnumPins(splitter, &enum_pins);
        if (FAILED(hr))
            goto next;

        hr = IEnumPins_Next(enum_pins, 1, &splitter_pin, NULL);
        IEnumPins_Release(enum_pins);
        if (hr != S_OK)
            goto next;

        hr = IPin_Connect(source_pin, splitter_pin, NULL);
        IPin_Release(splitter_pin);
        if (SUCCEEDED(hr))
        {
            detector->splitter = splitter;
            break;
        }

next:
        IGraphBuilder_RemoveFilter(detector->graph, splitter);
        IBaseFilter_Release(splitter);
    }

    IEnumMoniker_Release(enum_moniker);
    IPin_Release(source_pin);
    return hr;
}

/* MediaDet inner IUnknown */
static HRESULT WINAPI MediaDet_inner_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    MediaDetImpl *This = impl_from_IUnknown(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = &This->IUnknown_inner;
    else if (IsEqualIID(riid, &IID_IMediaDet))
        *ppv = &This->IMediaDet_iface;
    else
        WARN("(%p, %s,%p): not found\n", This, debugstr_guid(riid), ppv);

    if (!*ppv)
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI MediaDet_inner_AddRef(IUnknown *iface)
{
    MediaDetImpl *detector = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&detector->ref);

    TRACE("%p increasing refcount to %lu.\n", detector, refcount);

    return refcount;
}

static ULONG WINAPI MediaDet_inner_Release(IUnknown *iface)
{
    MediaDetImpl *detector = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&detector->ref);

    TRACE("%p decreasing refcount to %lu.\n", detector, refcount);

    if (!refcount)
    {
        MD_cleanup(detector);
        CoTaskMemFree(detector);
    }

    return refcount;
}

static const IUnknownVtbl mediadet_vtbl =
{
    MediaDet_inner_QueryInterface,
    MediaDet_inner_AddRef,
    MediaDet_inner_Release,
};

/* IMediaDet implementation */
static HRESULT WINAPI MediaDet_QueryInterface(IMediaDet *iface, REFIID riid, void **ppv)
{
    MediaDetImpl *This = impl_from_IMediaDet(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI MediaDet_AddRef(IMediaDet *iface)
{
    MediaDetImpl *This = impl_from_IMediaDet(iface);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI MediaDet_Release(IMediaDet *iface)
{
    MediaDetImpl *This = impl_from_IMediaDet(iface);
    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI MediaDet_get_Filter(IMediaDet *iface, IUnknown **filter)
{
    MediaDetImpl *detector = impl_from_IMediaDet(iface);

    TRACE("detector %p, filter %p.\n", detector, filter);

    if (!filter)
        return E_POINTER;

    *filter = (IUnknown *)detector->source;
    if (*filter)
        IUnknown_AddRef(*filter);
    else
        return S_FALSE;

    return S_OK;
}

static HRESULT WINAPI MediaDet_put_Filter(IMediaDet *iface, IUnknown *unk)
{
    MediaDetImpl *detector = impl_from_IMediaDet(iface);
    IGraphBuilder *graph;
    IBaseFilter *filter;
    HRESULT hr;

    TRACE("detector %p, unk %p.\n", detector, unk);

    if (!unk)
        return E_POINTER;

    if (FAILED(hr = IUnknown_QueryInterface(unk, &IID_IBaseFilter, (void **)&filter)))
    {
        WARN("Object does not expose IBaseFilter.\n");
        return hr;
    }

    if (detector->graph)
        MD_cleanup(detector);

    if (FAILED(hr = CoCreateInstance(&CLSID_FilterGraph, NULL,
            CLSCTX_INPROC_SERVER, &IID_IGraphBuilder, (void **)&graph)))
    {
        IBaseFilter_Release(filter);
        return hr;
    }

    if (FAILED(hr = IGraphBuilder_AddFilter(graph, filter, L"Source")))
    {
        IGraphBuilder_Release(graph);
        IBaseFilter_Release(filter);
        return hr;
    }

    detector->graph = graph;
    detector->source = filter;
    if (FAILED(find_splitter(detector)))
    {
        detector->splitter = detector->source;
        IBaseFilter_AddRef(detector->splitter);
    }

    return IMediaDet_put_CurrentStream(&detector->IMediaDet_iface, 0);
}

static HRESULT WINAPI MediaDet_get_OutputStreams(IMediaDet* iface, LONG *pVal)
{
    MediaDetImpl *This = impl_from_IMediaDet(iface);
    IEnumPins *pins;
    IPin *pin;
    HRESULT hr;

    TRACE("(%p)\n", This);

    if (!This->splitter)
        return E_INVALIDARG;

    if (This->num_streams != -1)
    {
        *pVal = This->num_streams;
        return S_OK;
    }

    *pVal = 0;

    hr = IBaseFilter_EnumPins(This->splitter, &pins);
    if (FAILED(hr))
        return hr;

    while (IEnumPins_Next(pins, 1, &pin, NULL) == S_OK)
    {
        PIN_DIRECTION dir;
        hr = IPin_QueryDirection(pin, &dir);
        IPin_Release(pin);
        if (FAILED(hr))
        {
            IEnumPins_Release(pins);
            return hr;
        }

        if (dir == PINDIR_OUTPUT)
            ++*pVal;
    }
    IEnumPins_Release(pins);

    This->num_streams = *pVal;
    return S_OK;
}

static HRESULT WINAPI MediaDet_get_CurrentStream(IMediaDet* iface, LONG *pVal)
{
    MediaDetImpl *This = impl_from_IMediaDet(iface);
    TRACE("(%p)\n", This);

    if (!pVal)
        return E_POINTER;

    *pVal = This->cur_stream;
    return S_OK;
}

static HRESULT SetCurPin(MediaDetImpl *This, LONG strm)
{
    IEnumPins *pins;
    IPin *pin;
    HRESULT hr;

    assert(This->splitter);
    assert(0 <= strm && strm < This->num_streams);

    if (This->cur_pin)
    {
        IPin_Release(This->cur_pin);
        This->cur_pin = NULL;
    }

    hr = IBaseFilter_EnumPins(This->splitter, &pins);
    if (FAILED(hr))
        return hr;

    while (IEnumPins_Next(pins, 1, &pin, NULL) == S_OK && !This->cur_pin)
    {
        PIN_DIRECTION dir;
        hr = IPin_QueryDirection(pin, &dir);
        if (FAILED(hr))
        {
            IPin_Release(pin);
            IEnumPins_Release(pins);
            return hr;
        }

        if (dir == PINDIR_OUTPUT && strm-- == 0)
            This->cur_pin = pin;
        else
            IPin_Release(pin);
    }
    IEnumPins_Release(pins);

    assert(This->cur_pin);
    return S_OK;
}

static HRESULT WINAPI MediaDet_put_CurrentStream(IMediaDet* iface, LONG newVal)
{
    MediaDetImpl *This = impl_from_IMediaDet(iface);
    HRESULT hr;

    TRACE("detector %p, index %ld.\n", This, newVal);

    if (This->num_streams == -1)
    {
        LONG n;
        hr = MediaDet_get_OutputStreams(iface, &n);
        if (FAILED(hr))
            return hr;
    }

    if (newVal < 0 || This->num_streams <= newVal)
        return E_INVALIDARG;

    hr = SetCurPin(This, newVal);
    if (FAILED(hr))
        return hr;

    This->cur_stream = newVal;
    return S_OK;
}

static HRESULT WINAPI MediaDet_get_StreamType(IMediaDet *iface, GUID *majortype)
{
    MediaDetImpl *detector = impl_from_IMediaDet(iface);
    AM_MEDIA_TYPE mt;
    HRESULT hr;

    TRACE("detector %p, majortype %p.\n", detector, majortype);

    if (!majortype)
        return E_POINTER;

    if (SUCCEEDED(hr = IMediaDet_get_StreamMediaType(iface, &mt)))
    {
        *majortype = mt.majortype;
        FreeMediaType(&mt);
    }

    return hr;
}

static HRESULT WINAPI MediaDet_get_StreamTypeB(IMediaDet *iface, BSTR *bstr)
{
    MediaDetImpl *detector = impl_from_IMediaDet(iface);
    HRESULT hr;
    GUID guid;

    TRACE("detector %p, bstr %p.\n", detector, bstr);

    if (SUCCEEDED(hr = IMediaDet_get_StreamType(iface, &guid)))
    {
        if (!(*bstr = SysAllocStringLen(NULL, CHARS_IN_GUID - 1)))
            return E_OUTOFMEMORY;
        StringFromGUID2(&guid, *bstr, CHARS_IN_GUID);
    }
    return hr;
}

static HRESULT WINAPI MediaDet_get_StreamLength(IMediaDet *iface, double *length)
{
    MediaDetImpl *detector = impl_from_IMediaDet(iface);
    IMediaSeeking *seeking;
    HRESULT hr;

    TRACE("detector %p, length %p.\n", detector, length);

    if (!length)
        return E_POINTER;

    if (!detector->cur_pin)
        return E_INVALIDARG;

    if (SUCCEEDED(hr = IPin_QueryInterface(detector->cur_pin,
                &IID_IMediaSeeking, (void **)&seeking)))
    {
        LONGLONG duration;

        if (SUCCEEDED(hr = IMediaSeeking_GetDuration(seeking, &duration)))
        {
            /* Windows assumes the time format is TIME_FORMAT_MEDIA_TIME
               and does not check it nor convert it, as tests show. */
            *length = (REFTIME)duration / 10000000;
        }
        IMediaSeeking_Release(seeking);
    }

    return hr;
}

static HRESULT WINAPI MediaDet_get_Filename(IMediaDet* iface, BSTR *pVal)
{
    MediaDetImpl *This = impl_from_IMediaDet(iface);

    TRACE("(%p)\n", This);

    if (!pVal)
        return E_POINTER;

    *pVal = NULL;
    /* MSDN says it should return E_FAIL if no file is open, but tests
       show otherwise.  */
    if (!This->filename)
        return S_OK;

    *pVal = SysAllocString(This->filename);
    if (!*pVal)
        return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI MediaDet_put_Filename(IMediaDet *iface, BSTR filename)
{
    MediaDetImpl *This = impl_from_IMediaDet(iface);
    IGraphBuilder *gb;
    IBaseFilter *bf;
    HRESULT hr;

    TRACE("detector %p, filename %s.\n", This, debugstr_w(filename));

    if (This->graph)
    {
        WARN("MSDN says not to call this method twice\n");
        MD_cleanup(This);
    }

    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IGraphBuilder, (void **) &gb);
    if (FAILED(hr))
        return hr;

    if (FAILED(hr = IGraphBuilder_AddSourceFilter(gb, filename, L"Source", &bf)))
    {
        IGraphBuilder_Release(gb);
        return hr;
    }

    if (!(This->filename = wcsdup(filename)))
    {
        IBaseFilter_Release(bf);
        IGraphBuilder_Release(gb);
        return E_OUTOFMEMORY;
    }

    This->graph = gb;
    This->source = bf;
    hr = find_splitter(This);
    if (FAILED(hr))
        return hr;

    return MediaDet_put_CurrentStream(iface, 0);
}

static HRESULT WINAPI MediaDet_GetBitmapBits(IMediaDet* iface,
                                             double StreamTime,
                                             LONG *pBufferSize, char *pBuffer,
                                             LONG Width, LONG Height)
{
    MediaDetImpl *This = impl_from_IMediaDet(iface);
    FIXME("(%p)->(%.16e %p %p %ld %ld): not implemented!\n", This, StreamTime, pBufferSize, pBuffer,
          Width, Height);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaDet_WriteBitmapBits(IMediaDet* iface,
                                               double StreamTime, LONG Width,
                                               LONG Height, BSTR Filename)
{
    MediaDetImpl *This = impl_from_IMediaDet(iface);
    FIXME("(%p)->(%.16e %ld %ld %p): not implemented!\n", This, StreamTime, Width, Height, Filename);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaDet_get_StreamMediaType(IMediaDet* iface,
                                                   AM_MEDIA_TYPE *pVal)
{
    MediaDetImpl *This = impl_from_IMediaDet(iface);

    TRACE("(%p)\n", This);

    if (!pVal)
        return E_POINTER;

    if (!This->cur_pin)
        return E_INVALIDARG;

    return get_pin_media_type(This->cur_pin, pVal);
}

static HRESULT WINAPI MediaDet_GetSampleGrabber(IMediaDet* iface,
                                                ISampleGrabber **ppVal)
{
    MediaDetImpl *This = impl_from_IMediaDet(iface);
    FIXME("(%p)->(%p): not implemented!\n", This, ppVal);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaDet_get_FrameRate(IMediaDet* iface, double *pVal)
{
    MediaDetImpl *This = impl_from_IMediaDet(iface);
    AM_MEDIA_TYPE mt;
    VIDEOINFOHEADER *vh;
    HRESULT hr;

    TRACE("(%p)\n", This);

    if (!pVal)
        return E_POINTER;

    hr = MediaDet_get_StreamMediaType(iface, &mt);
    if (FAILED(hr))
        return hr;

    if (!IsEqualGUID(&mt.majortype, &MEDIATYPE_Video))
    {
        CoTaskMemFree(mt.pbFormat);
        return VFW_E_INVALIDMEDIATYPE;
    }

    vh = (VIDEOINFOHEADER *) mt.pbFormat;
    *pVal = 1.0e7 / (double) vh->AvgTimePerFrame;

    CoTaskMemFree(mt.pbFormat);
    return S_OK;
}

static HRESULT WINAPI MediaDet_EnterBitmapGrabMode(IMediaDet* iface,
                                                   double SeekTime)
{
    MediaDetImpl *This = impl_from_IMediaDet(iface);
    FIXME("(%p)->(%f): not implemented!\n", This, SeekTime);
    return E_NOTIMPL;
}

static const IMediaDetVtbl IMediaDet_VTable =
{
    MediaDet_QueryInterface,
    MediaDet_AddRef,
    MediaDet_Release,
    MediaDet_get_Filter,
    MediaDet_put_Filter,
    MediaDet_get_OutputStreams,
    MediaDet_get_CurrentStream,
    MediaDet_put_CurrentStream,
    MediaDet_get_StreamType,
    MediaDet_get_StreamTypeB,
    MediaDet_get_StreamLength,
    MediaDet_get_Filename,
    MediaDet_put_Filename,
    MediaDet_GetBitmapBits,
    MediaDet_WriteBitmapBits,
    MediaDet_get_StreamMediaType,
    MediaDet_GetSampleGrabber,
    MediaDet_get_FrameRate,
    MediaDet_EnterBitmapGrabMode,
};

HRESULT media_detector_create(IUnknown *pUnkOuter, IUnknown **ppv)
{
    MediaDetImpl* obj = NULL;

    TRACE("(%p,%p)\n", pUnkOuter, ppv);

    obj = CoTaskMemAlloc(sizeof(MediaDetImpl));
    if (NULL == obj) {
        *ppv = NULL;
        return E_OUTOFMEMORY;
    }
    ZeroMemory(obj, sizeof(MediaDetImpl));

    obj->ref = 1;
    obj->IUnknown_inner.lpVtbl = &mediadet_vtbl;
    obj->IMediaDet_iface.lpVtbl = &IMediaDet_VTable;
    obj->graph = NULL;
    obj->source = NULL;
    obj->splitter = NULL;
    obj->cur_pin = NULL;
    obj->num_streams = -1;
    obj->cur_stream = 0;

    if (pUnkOuter)
        obj->outer_unk = pUnkOuter;
    else
        obj->outer_unk = &obj->IUnknown_inner;

    *ppv = &obj->IUnknown_inner;
    return S_OK;
}
