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

WINE_DEFAULT_DEBUG_CHANNEL(qedit);

typedef struct MediaDetImpl {
    const IMediaDetVtbl *MediaDet_Vtbl;
    LONG refCount;
    IGraphBuilder *graph;
    IBaseFilter *source;
    IBaseFilter *splitter;
    long num_streams;
    long cur_stream;
    IPin *cur_pin;
} MediaDetImpl;

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
    This->num_streams = -1;
    This->cur_stream = 0;
}

static ULONG WINAPI MediaDet_AddRef(IMediaDet* iface)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->refCount);
    TRACE("(%p)->() AddRef from %d\n", This, refCount - 1);
    return refCount;
}

static ULONG WINAPI MediaDet_Release(IMediaDet* iface)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->refCount);
    TRACE("(%p)->() Release from %d\n", This, refCount + 1);

    if (refCount == 0)
    {
        MD_cleanup(This);
        CoTaskMemFree(This);
        return 0;
    }

    return refCount;
}

static HRESULT WINAPI MediaDet_QueryInterface(IMediaDet* iface, REFIID riid,
                                              void **ppvObject)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IMediaDet)) {
        MediaDet_AddRef(iface);
        *ppvObject = This;
        return S_OK;
    }
    *ppvObject = NULL;
    WARN("(%p, %s,%p): not found\n", This, debugstr_guid(riid), ppvObject);
    return E_NOINTERFACE;
}

static HRESULT WINAPI MediaDet_get_Filter(IMediaDet* iface, IUnknown **pVal)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    FIXME("(%p)->(%p): not implemented!\n", This, pVal);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaDet_put_Filter(IMediaDet* iface, IUnknown *newVal)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    FIXME("(%p)->(%p): not implemented!\n", This, newVal);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaDet_get_OutputStreams(IMediaDet* iface, long *pVal)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
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

static HRESULT WINAPI MediaDet_get_CurrentStream(IMediaDet* iface, long *pVal)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    TRACE("(%p)\n", This);

    if (!pVal)
        return E_POINTER;

    *pVal = This->cur_stream;
    return S_OK;
}

static HRESULT SetCurPin(MediaDetImpl *This, long strm)
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

static HRESULT WINAPI MediaDet_put_CurrentStream(IMediaDet* iface, long newVal)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    HRESULT hr;

    TRACE("(%p)->(%ld)\n", This, newVal);

    if (This->num_streams == -1)
    {
        long n;
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

static HRESULT WINAPI MediaDet_get_StreamType(IMediaDet* iface, GUID *pVal)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    FIXME("(%p)->(%p): not implemented!\n", This, debugstr_guid(pVal));
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaDet_get_StreamTypeB(IMediaDet* iface, BSTR *pVal)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    FIXME("(%p)->(%p): not implemented!\n", This, pVal);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaDet_get_StreamLength(IMediaDet* iface, double *pVal)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    FIXME("(%p): stub!\n", This);
    return VFW_E_INVALIDMEDIATYPE;
}

static HRESULT WINAPI MediaDet_get_Filename(IMediaDet* iface, BSTR *pVal)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    IFileSourceFilter *file;
    LPOLESTR name;
    HRESULT hr;

    TRACE("(%p)\n", This);

    if (!pVal)
        return E_POINTER;

    *pVal = NULL;
    /* MSDN says it should return E_FAIL if no file is open, but tests
       show otherwise.  */
    if (!This->source)
        return S_OK;

    hr = IBaseFilter_QueryInterface(This->source, &IID_IFileSourceFilter,
                                    (void **) &file);
    if (FAILED(hr))
        return hr;

    hr = IFileSourceFilter_GetCurFile(file, &name, NULL);
    IFileSourceFilter_Release(file);
    if (FAILED(hr))
        return hr;

    *pVal = SysAllocString(name);
    CoTaskMemFree(name);
    if (!*pVal)
        return E_OUTOFMEMORY;

    return S_OK;
}

/* From quartz, 2008/04/07 */
static HRESULT GetFilterInfo(IMoniker *pMoniker, GUID *pclsid, VARIANT *pvar)
{
    static const WCHAR wszClsidName[] = {'C','L','S','I','D',0};
    static const WCHAR wszFriendlyName[] = {'F','r','i','e','n','d','l','y','N','a','m','e',0};
    IPropertyBag *pPropBagCat = NULL;
    HRESULT hr;

    VariantInit(pvar);
    V_VT(pvar) = VT_BSTR;

    hr = IMoniker_BindToStorage(pMoniker, NULL, NULL, &IID_IPropertyBag,
                                (LPVOID *) &pPropBagCat);

    if (SUCCEEDED(hr))
        hr = IPropertyBag_Read(pPropBagCat, wszClsidName, pvar, NULL);

    if (SUCCEEDED(hr))
        hr = CLSIDFromString(V_UNION(pvar, bstrVal), pclsid);

    if (SUCCEEDED(hr))
        hr = IPropertyBag_Read(pPropBagCat, wszFriendlyName, pvar, NULL);

    if (SUCCEEDED(hr))
        TRACE("Moniker = %s - %s\n", debugstr_guid(pclsid),
              debugstr_w(V_UNION(pvar, bstrVal)));

    if (pPropBagCat)
        IPropertyBag_Release(pPropBagCat);

    return hr;
}

static HRESULT GetSplitter(MediaDetImpl *This)
{
    IFileSourceFilter *file;
    LPOLESTR name;
    AM_MEDIA_TYPE mt;
    GUID type[2];
    IFilterMapper2 *map;
    IEnumMoniker *filters;
    IMoniker *mon;
    VARIANT var;
    GUID clsid;
    IBaseFilter *splitter;
    IEnumPins *pins;
    IPin *source_pin, *splitter_pin;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IFilterMapper2, (void **) &map);
    if (FAILED(hr))
        return hr;

    hr = IBaseFilter_QueryInterface(This->source, &IID_IFileSourceFilter,
                                    (void **) &file);
    if (FAILED(hr))
    {
        IFilterMapper2_Release(map);
        return hr;
    }

    hr = IFileSourceFilter_GetCurFile(file, &name, &mt);
    IFileSourceFilter_Release(file);
    CoTaskMemFree(name);
    if (FAILED(hr))
    {
        IFilterMapper2_Release(map);
        return hr;
    }
    type[0] = mt.majortype;
    type[1] = mt.subtype;
    CoTaskMemFree(mt.pbFormat);

    hr = IFilterMapper2_EnumMatchingFilters(map, &filters, 0, TRUE,
                                            MERIT_UNLIKELY, FALSE, 1, type,
                                            NULL, NULL, FALSE, TRUE,
                                            0, NULL, NULL, NULL);
    IFilterMapper2_Release(map);
    if (FAILED(hr))
        return hr;

    hr = IEnumMoniker_Next(filters, 1, &mon, NULL);
    IEnumMoniker_Release(filters);
    if (hr != S_OK)    /* No matches, what do we do?  */
        return E_NOINTERFACE;

    hr = GetFilterInfo(mon, &clsid, &var);
    IMoniker_Release(mon);
    if (FAILED(hr))
        return hr;

    hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IBaseFilter, (void **) &splitter);
    if (FAILED(hr))
        return hr;

    hr = IGraphBuilder_AddFilter(This->graph, splitter,
                                 V_UNION(&var, bstrVal));
    if (FAILED(hr))
    {
        IBaseFilter_Release(splitter);
        return hr;
    }
    This->splitter = splitter;

    hr = IBaseFilter_EnumPins(This->source, &pins);
    if (FAILED(hr))
        return hr;
    IEnumPins_Next(pins, 1, &source_pin, NULL);
    IEnumPins_Release(pins);

    hr = IBaseFilter_EnumPins(splitter, &pins);
    if (FAILED(hr))
    {
        IPin_Release(source_pin);
        return hr;
    }
    IEnumPins_Next(pins, 1, &splitter_pin, NULL);
    IEnumPins_Release(pins);

    hr = IPin_Connect(source_pin, splitter_pin, NULL);
    IPin_Release(source_pin);
    IPin_Release(splitter_pin);
    if (FAILED(hr))
        return hr;

    return S_OK;
}

static HRESULT WINAPI MediaDet_put_Filename(IMediaDet* iface, BSTR newVal)
{
    static const WCHAR reader[] = {'R','e','a','d','e','r',0};
    MediaDetImpl *This = (MediaDetImpl *)iface;
    IGraphBuilder *gb;
    IBaseFilter *bf;
    HRESULT hr;

    TRACE("(%p)->(%s)\n", This, debugstr_w(newVal));

    if (This->graph)
    {
        WARN("MSDN says not to call this method twice\n");
        MD_cleanup(This);
    }

    hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IGraphBuilder, (void **) &gb);
    if (FAILED(hr))
        return hr;

    hr = IGraphBuilder_AddSourceFilter(gb, newVal, reader, &bf);
    if (FAILED(hr))
    {
        IGraphBuilder_Release(gb);
        return hr;
    }

    This->graph = gb;
    This->source = bf;
    hr = GetSplitter(This);
    if (FAILED(hr))
        return hr;

    return MediaDet_put_CurrentStream(iface, 0);
}

static HRESULT WINAPI MediaDet_GetBitmapBits(IMediaDet* iface,
                                             double StreamTime,
                                             long *pBufferSize, char *pBuffer,
                                             long Width, long Height)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    FIXME("(%p)->(%f %p %p %ld %ld): not implemented!\n", This, StreamTime, pBufferSize, pBuffer,
          Width, Height);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaDet_WriteBitmapBits(IMediaDet* iface,
                                               double StreamTime, long Width,
                                               long Height, BSTR Filename)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    FIXME("(%p)->(%f %ld %ld %p): not implemented!\n", This, StreamTime, Width, Height, Filename);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaDet_get_StreamMediaType(IMediaDet* iface,
                                                   AM_MEDIA_TYPE *pVal)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    IEnumMediaTypes *types;
    AM_MEDIA_TYPE *pmt;
    HRESULT hr;

    TRACE("(%p)\n", This);

    if (!pVal)
        return E_POINTER;

    if (!This->cur_pin)
        return E_INVALIDARG;

    hr = IPin_EnumMediaTypes(This->cur_pin, &types);
    if (SUCCEEDED(hr))
    {
        hr = (IEnumMediaTypes_Next(types, 1, &pmt, NULL) == S_OK
              ? S_OK
              : E_NOINTERFACE);
        IEnumMediaTypes_Release(types);
    }

    if (SUCCEEDED(hr))
    {
        *pVal = *pmt;
        CoTaskMemFree(pmt);
    }

    return hr;
}

static HRESULT WINAPI MediaDet_GetSampleGrabber(IMediaDet* iface,
                                                ISampleGrabber **ppVal)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
    FIXME("(%p)->(%p): not implemented!\n", This, ppVal);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaDet_get_FrameRate(IMediaDet* iface, double *pVal)
{
    MediaDetImpl *This = (MediaDetImpl *)iface;
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
    MediaDetImpl *This = (MediaDetImpl *)iface;
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

HRESULT MediaDet_create(IUnknown * pUnkOuter, LPVOID * ppv) {
    MediaDetImpl* obj = NULL;

    TRACE("(%p,%p)\n", ppv, pUnkOuter);

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    obj = CoTaskMemAlloc(sizeof(MediaDetImpl));
    if (NULL == obj) {
        *ppv = NULL;
        return E_OUTOFMEMORY;
    }
    ZeroMemory(obj, sizeof(MediaDetImpl));

    obj->refCount = 1;
    obj->MediaDet_Vtbl = &IMediaDet_VTable;
    obj->graph = NULL;
    obj->source = NULL;
    obj->splitter = NULL;
    obj->cur_pin = NULL;
    obj->num_streams = -1;
    obj->cur_stream = 0;
    *ppv = obj;

    return S_OK;
}
