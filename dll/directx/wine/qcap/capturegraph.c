/* Capture Graph Builder, Minimal edition
 *
 * Copyright 2005 Maarten Lankhorst
 * Copyright 2005 Rolf Kalbermatter
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

#include "qcap_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

/***********************************************************************
*   ICaptureGraphBuilder & ICaptureGraphBuilder2 implementation
*/
typedef struct CaptureGraphImpl
{
    ICaptureGraphBuilder2 ICaptureGraphBuilder2_iface;
    ICaptureGraphBuilder ICaptureGraphBuilder_iface;
    LONG ref;
    IGraphBuilder *mygraph;
    CRITICAL_SECTION csFilter;
} CaptureGraphImpl;

static const ICaptureGraphBuilderVtbl builder_Vtbl;
static const ICaptureGraphBuilder2Vtbl builder2_Vtbl;

static inline CaptureGraphImpl *impl_from_ICaptureGraphBuilder(ICaptureGraphBuilder *iface)
{
    return CONTAINING_RECORD(iface, CaptureGraphImpl, ICaptureGraphBuilder_iface);
}

static inline CaptureGraphImpl *impl_from_ICaptureGraphBuilder2(ICaptureGraphBuilder2 *iface)
{
    return CONTAINING_RECORD(iface, CaptureGraphImpl, ICaptureGraphBuilder2_iface);
}


HRESULT capture_graph_create(IUnknown *outer, IUnknown **out)
{
    CaptureGraphImpl *object;

    if (outer)
        return CLASS_E_NOAGGREGATION;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->ICaptureGraphBuilder2_iface.lpVtbl = &builder2_Vtbl;
    object->ICaptureGraphBuilder_iface.lpVtbl = &builder_Vtbl;
    object->ref = 1;
    object->mygraph = NULL;
    InitializeCriticalSectionEx(&object->csFilter, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    object->csFilter.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": CaptureGraphImpl.csFilter");

    TRACE("Created capture graph builder %p.\n", object);
    *out = (IUnknown *)&object->ICaptureGraphBuilder_iface;
    return S_OK;
}

static HRESULT WINAPI
fnCaptureGraphBuilder2_QueryInterface(ICaptureGraphBuilder2 * iface,
                                      REFIID riid,
                                      LPVOID * ppv)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder2(iface);

    TRACE("(%p/%p)->(%s, %p)\n", This, iface, debugstr_guid(riid), ppv);

    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = &This->ICaptureGraphBuilder2_iface;
    else if (IsEqualIID(riid, &IID_ICaptureGraphBuilder))
        *ppv = &This->ICaptureGraphBuilder_iface;
    else if (IsEqualIID(riid, &IID_ICaptureGraphBuilder2))
        *ppv = &This->ICaptureGraphBuilder2_iface;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        TRACE ("-- Interface = %p\n", *ppv);
        return S_OK;
    }

    TRACE ("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI
fnCaptureGraphBuilder2_AddRef(ICaptureGraphBuilder2 * iface)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder2(iface);
    DWORD ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %lu.\n", This, ref);

    return ref;
}

static ULONG WINAPI fnCaptureGraphBuilder2_Release(ICaptureGraphBuilder2 * iface)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %lu.\n", This, ref);

    if (!ref)
    {
        This->csFilter.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->csFilter);
        if (This->mygraph)
            IGraphBuilder_Release(This->mygraph);
        free(This);
    }
    return ref;
}

static HRESULT WINAPI
fnCaptureGraphBuilder2_SetFilterGraph(ICaptureGraphBuilder2 * iface,
                                      IGraphBuilder *pfg)
{
/* The graph builder will automatically create a filter graph if you don't call
   this method. If you call this method after the graph builder has created its
   own filter graph, the call will fail. */
    IMediaEvent *pmev;
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder2(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pfg);

    if (This->mygraph)
        return E_UNEXPECTED;

    if (!pfg)
        return E_POINTER;

    This->mygraph = pfg;
    IGraphBuilder_AddRef(This->mygraph);
    if (SUCCEEDED(IGraphBuilder_QueryInterface(This->mygraph,
                                          &IID_IMediaEvent, (LPVOID *)&pmev)))
    {
        IMediaEvent_CancelDefaultHandling(pmev, EC_REPAINT);
        IMediaEvent_Release(pmev);
    }
    return S_OK;
}

static HRESULT WINAPI
fnCaptureGraphBuilder2_GetFilterGraph(ICaptureGraphBuilder2 * iface,
                                      IGraphBuilder **pfg)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder2(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pfg);

    if (!pfg)
        return E_POINTER;

    *pfg = This->mygraph;
    if (!This->mygraph)
    {
        TRACE("(%p) Getting NULL filtergraph\n", iface);
        return E_UNEXPECTED;
    }

    IGraphBuilder_AddRef(This->mygraph);

    TRACE("(%p) return filtergraph %p\n", iface, *pfg);
    return S_OK;
}

static HRESULT WINAPI
fnCaptureGraphBuilder2_SetOutputFileName(ICaptureGraphBuilder2 * iface,
                                         const GUID *pType,
                                         LPCOLESTR lpstrFile,
                                         IBaseFilter **ppf,
                                         IFileSinkFilter **ppSink)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder2(iface);

    FIXME("(%p/%p)->(%s, %s, %p, %p) Stub!\n", This, iface,
          debugstr_guid(pType), debugstr_w(lpstrFile), ppf, ppSink);

    return E_NOTIMPL;
}

static BOOL pin_has_majortype(IPin *pin, const GUID *majortype)
{
    IEnumMediaTypes *enummt;
    AM_MEDIA_TYPE *mt;

    if (FAILED(IPin_EnumMediaTypes(pin, &enummt)))
        return FALSE;

    while (IEnumMediaTypes_Next(enummt, 1, &mt, NULL) == S_OK)
    {
        if (IsEqualGUID(&mt->majortype, majortype))
        {
            DeleteMediaType(mt);
            IEnumMediaTypes_Release(enummt);
            return TRUE;
        }
        DeleteMediaType(mt);
    }
    IEnumMediaTypes_Release(enummt);
    return FALSE;
}

static BOOL pin_matches(IPin *pin, PIN_DIRECTION dir, const GUID *category,
        const GUID *majortype, BOOL unconnected)
{
    PIN_DIRECTION candidate_dir;
    HRESULT hr;
    IPin *peer;

    if (FAILED(hr = IPin_QueryDirection(pin, &candidate_dir)))
        ERR("Failed to query direction, hr %#lx.\n", hr);

    if (dir != candidate_dir)
        return FALSE;

    if (unconnected && IPin_ConnectedTo(pin, &peer) == S_OK && peer)
    {
        IPin_Release(peer);
        return FALSE;
    }

    if (category)
    {
        IKsPropertySet *set;
        GUID property;
        DWORD size;

        if (FAILED(IPin_QueryInterface(pin, &IID_IKsPropertySet, (void **)&set)))
            return FALSE;

        hr = IKsPropertySet_Get(set, &AMPROPSETID_Pin, AMPROPERTY_PIN_CATEGORY,
                NULL, 0, &property, sizeof(property), &size);
        IKsPropertySet_Release(set);
        if (FAILED(hr) || !IsEqualGUID(&property, category))
            return FALSE;
    }

    if (majortype && !pin_has_majortype(pin, majortype))
        return FALSE;

    return TRUE;
}

static HRESULT find_interface_recurse(PIN_DIRECTION dir, const GUID *category,
        const GUID *majortype, IBaseFilter *filter, REFIID iid, void **out)
{
    BOOL found_category = FALSE;
    IEnumPins *enumpins;
    IPin *pin, *peer;
    PIN_INFO info;
    HRESULT hr;

    TRACE("Looking for %s pins, category %s, majortype %s from filter %p.\n",
            dir == PINDIR_INPUT ? "sink" : "source", debugstr_guid(category),
            debugstr_guid(majortype), filter);

    if (FAILED(hr = IBaseFilter_EnumPins(filter, &enumpins)))
    {
        ERR("Failed to enumerate pins, hr %#lx.\n", hr);
        return hr;
    }

    while (IEnumPins_Next(enumpins, 1, &pin, NULL) == S_OK)
    {
        if (!pin_matches(pin, dir, category, majortype, FALSE))
        {
            IPin_Release(pin);
            continue;
        }

        if (category)
            found_category = TRUE;

        if (IPin_QueryInterface(pin, iid, out) == S_OK)
        {
            IPin_Release(pin);
            IEnumPins_Release(enumpins);
            return S_OK;
        }

        hr = IPin_ConnectedTo(pin, &peer);
        IPin_Release(pin);
        if (hr == S_OK)
        {
            if (IPin_QueryInterface(peer, iid, out) == S_OK)
            {
                IPin_Release(peer);
                IEnumPins_Release(enumpins);
                return S_OK;
            }

            IPin_QueryPinInfo(peer, &info);
            IPin_Release(peer);

            if (IBaseFilter_QueryInterface(info.pFilter, iid, out) == S_OK)
            {
                IBaseFilter_Release(info.pFilter);
                IEnumPins_Release(enumpins);
                return S_OK;
            }

            hr = find_interface_recurse(dir, NULL, NULL, info.pFilter, iid, out);
            IBaseFilter_Release(info.pFilter);
            if (hr == S_OK)
            {
                IEnumPins_Release(enumpins);
                return S_OK;
            }
        }
    }
    IEnumPins_Release(enumpins);

    if (category && !found_category)
        return E_NOINTERFACE;

    return E_FAIL;
}

static HRESULT WINAPI fnCaptureGraphBuilder2_FindInterface(ICaptureGraphBuilder2 *iface,
        const GUID *category, const GUID *majortype, IBaseFilter *filter, REFIID iid, void **out)
{
    CaptureGraphImpl *graph = impl_from_ICaptureGraphBuilder2(iface);
    HRESULT hr;

    TRACE("graph %p, category %s, majortype %s, filter %p, iid %s, out %p.\n",
            graph, debugstr_guid(category), debugstr_guid(majortype), filter, debugstr_guid(iid), out);

    if (!filter)
        return E_POINTER;

    if (category && IsEqualGUID(category, &LOOK_DOWNSTREAM_ONLY))
        return find_interface_recurse(PINDIR_OUTPUT, NULL, NULL, filter, iid, out);

    if (category && IsEqualGUID(category, &LOOK_UPSTREAM_ONLY))
        return find_interface_recurse(PINDIR_INPUT, NULL, NULL, filter, iid, out);

    if (IBaseFilter_QueryInterface(filter, iid, out) == S_OK)
        return S_OK;

    if (!category)
        majortype = NULL;

    hr = find_interface_recurse(PINDIR_OUTPUT, category, majortype, filter, iid, out);
    if (hr == S_OK || hr == E_NOINTERFACE)
        return hr;

    return find_interface_recurse(PINDIR_INPUT, NULL, NULL, filter, iid, out);
}

static HRESULT match_smart_tee_pin(CaptureGraphImpl *This,
                                   const GUID *pCategory,
                                   const GUID *pType,
                                   IUnknown *pSource,
                                   IPin **source_out)
{
    IPin *capture = NULL;
    IPin *preview = NULL;
    IPin *peer = NULL;
    IBaseFilter *smartTee = NULL;
    BOOL needSmartTee = FALSE;
    HRESULT hr;

    TRACE("(%p, %s, %s, %p, %p)\n", This, debugstr_guid(pCategory), debugstr_guid(pType), pSource, source_out);
    hr = ICaptureGraphBuilder2_FindPin(&This->ICaptureGraphBuilder2_iface, pSource,
            PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE, pType, FALSE, 0, &capture);
    if (SUCCEEDED(hr)) {
        hr = ICaptureGraphBuilder2_FindPin(&This->ICaptureGraphBuilder2_iface, pSource,
                PINDIR_OUTPUT, &PIN_CATEGORY_PREVIEW, pType, FALSE, 0, &preview);
        if (FAILED(hr))
            needSmartTee = TRUE;
    } else {
        hr = E_INVALIDARG;
        goto end;
    }
    if (!needSmartTee) {
        if (IsEqualIID(pCategory, &PIN_CATEGORY_CAPTURE)) {
            hr = IPin_ConnectedTo(capture, &peer);
            if (hr == VFW_E_NOT_CONNECTED) {
                *source_out = capture;
                IPin_AddRef(*source_out);
                hr = S_OK;
            } else
                hr = E_INVALIDARG;
        } else {
            hr = IPin_ConnectedTo(preview, &peer);
            if (hr == VFW_E_NOT_CONNECTED) {
                *source_out = preview;
                IPin_AddRef(*source_out);
                hr = S_OK;
            } else
                hr = E_INVALIDARG;
        }
        goto end;
    }
    hr = IPin_ConnectedTo(capture, &peer);
    if (SUCCEEDED(hr)) {
        PIN_INFO pinInfo;
        GUID classID;
        hr = IPin_QueryPinInfo(peer, &pinInfo);
        if (SUCCEEDED(hr)) {
            hr = IBaseFilter_GetClassID(pinInfo.pFilter, &classID);
            if (SUCCEEDED(hr)) {
                if (IsEqualIID(&classID, &CLSID_SmartTee)) {
                    smartTee = pinInfo.pFilter;
                    IBaseFilter_AddRef(smartTee);
                }
            }
            IBaseFilter_Release(pinInfo.pFilter);
        }
        if (!smartTee) {
            hr = E_INVALIDARG;
            goto end;
        }
    } else if (hr == VFW_E_NOT_CONNECTED) {
        hr = CoCreateInstance(&CLSID_SmartTee, NULL, CLSCTX_INPROC_SERVER,
                &IID_IBaseFilter, (LPVOID*)&smartTee);
        if (SUCCEEDED(hr)) {
            hr = IGraphBuilder_AddFilter(This->mygraph, smartTee, NULL);
            if (SUCCEEDED(hr)) {
                IPin *smartTeeInput = NULL;
                hr = IBaseFilter_FindPin(smartTee, L"Input", &smartTeeInput);
                if (SUCCEEDED(hr)) {
                    hr = IGraphBuilder_ConnectDirect(This->mygraph, capture, smartTeeInput, NULL);
                    IPin_Release(smartTeeInput);
                }
            }
        }
        if (FAILED(hr)) {
            TRACE("adding SmartTee failed with hr=0x%08lx\n", hr);
            hr = E_INVALIDARG;
            goto end;
        }
    } else {
        hr = E_INVALIDARG;
        goto end;
    }
    if (IsEqualIID(pCategory, &PIN_CATEGORY_CAPTURE))
        hr = IBaseFilter_FindPin(smartTee, L"Capture", source_out);
    else {
        hr = IBaseFilter_FindPin(smartTee, L"Preview", source_out);
        if (SUCCEEDED(hr))
            hr = VFW_S_NOPREVIEWPIN;
    }

end:
    if (capture)
        IPin_Release(capture);
    if (preview)
        IPin_Release(preview);
    if (peer)
        IPin_Release(peer);
    if (smartTee)
        IBaseFilter_Release(smartTee);
    TRACE("for %s returning hr=0x%08lx, *source_out=%p\n", IsEqualIID(pCategory, &PIN_CATEGORY_CAPTURE) ? "capture" : "preview", hr, source_out ? *source_out : 0);
    return hr;
}

static HRESULT find_unconnected_source_from_filter(CaptureGraphImpl *capture_graph,
        const GUID *category, const GUID *majortype, IBaseFilter *filter, IPin **ret);

static HRESULT find_unconnected_source_from_pin(CaptureGraphImpl *capture_graph,
        const GUID *category, const GUID *majortype, IPin *pin, IPin **ret)
{
    PIN_DIRECTION dir;
    PIN_INFO info;
    HRESULT hr;
    IPin *peer;

    IPin_QueryDirection(pin, &dir);
    if (dir != PINDIR_OUTPUT)
        return VFW_E_INVALID_DIRECTION;

    if (category && (IsEqualGUID(category, &PIN_CATEGORY_CAPTURE)
            || IsEqualGUID(category, &PIN_CATEGORY_PREVIEW)))
    {
        if (FAILED(hr = match_smart_tee_pin(capture_graph, category, majortype, (IUnknown *)pin, &pin)))
            return hr;

        if (FAILED(IPin_ConnectedTo(pin, &peer)))
        {
            *ret = pin;
            return S_OK;
        }
    }
    else
    {
        if (FAILED(IPin_ConnectedTo(pin, &peer)))
        {
            if (!pin_matches(pin, PINDIR_OUTPUT, category, majortype, FALSE))
                return E_FAIL;

            IPin_AddRef(*ret = pin);
            return S_OK;
        }
        IPin_AddRef(pin);
    }

    IPin_QueryPinInfo(peer, &info);
    hr = find_unconnected_source_from_filter(capture_graph, category, majortype, info.pFilter, ret);
    IBaseFilter_Release(info.pFilter);
    IPin_Release(peer);
    IPin_Release(pin);
    return hr;
}

static HRESULT find_unconnected_source_from_filter(CaptureGraphImpl *capture_graph,
        const GUID *category, const GUID *majortype, IBaseFilter *filter, IPin **ret)
{
    IEnumPins *enumpins;
    IPin *pin, *peer;
    HRESULT hr;

    if (category && (IsEqualGUID(category, &PIN_CATEGORY_CAPTURE)
            || IsEqualGUID(category, &PIN_CATEGORY_PREVIEW)))
    {
        if (FAILED(hr = match_smart_tee_pin(capture_graph, category, majortype, (IUnknown *)filter, &pin)))
            return hr;

        if (FAILED(IPin_ConnectedTo(pin, &peer)))
        {
            *ret = pin;
            return hr;
        }

        IPin_Release(peer);
        IPin_Release(pin);
        return E_INVALIDARG;
    }

    if (FAILED(hr = IBaseFilter_EnumPins(filter, &enumpins)))
        return hr;

    while (IEnumPins_Next(enumpins, 1, &pin, NULL) == S_OK)
    {
        if (SUCCEEDED(hr = find_unconnected_source_from_pin(capture_graph, category, majortype, pin, ret)))
        {
            IEnumPins_Release(enumpins);
            IPin_Release(pin);
            return hr;
        }
        IPin_Release(pin);
    }
    IEnumPins_Release(enumpins);

    return E_INVALIDARG;
}

static HRESULT WINAPI fnCaptureGraphBuilder2_RenderStream(ICaptureGraphBuilder2 *iface,
        const GUID *category, const GUID *majortype, IUnknown *source,
        IBaseFilter *pfCompressor, IBaseFilter *pfRenderer)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder2(iface);
    IPin *source_out = NULL, *renderer_in;
    BOOL rendererNeedsRelease = FALSE;
    HRESULT hr, return_hr = S_OK;
    IBaseFilter *filter;
    IPin *pin;

    TRACE("graph %p, category %s, majortype %s, source %p, intermediate %p, sink %p.\n",
            This, debugstr_guid(category), debugstr_guid(majortype), source, pfCompressor, pfRenderer);

    if (!This->mygraph)
    {
        FIXME("Need a capture graph\n");
        return E_UNEXPECTED;
    }

    if (category && IsEqualGUID(category, &PIN_CATEGORY_VBI))
    {
        FIXME("Tee/Sink-to-Sink filter not supported\n");
        return E_NOTIMPL;
    }

    if (IUnknown_QueryInterface(source, &IID_IPin, (void **)&pin) == S_OK)
    {
        hr = find_unconnected_source_from_pin(This, category, majortype, pin, &source_out);
        IPin_Release(pin);
    }
    else if (IUnknown_QueryInterface(source, &IID_IBaseFilter, (void **)&filter) == S_OK)
    {
        hr = find_unconnected_source_from_filter(This, category, majortype, filter, &source_out);
        IBaseFilter_Release(filter);
    }
    else
    {
        WARN("Source object does not expose IBaseFilter or IPin.\n");
        return E_INVALIDARG;
    }
    if (FAILED(hr))
        return hr;
    return_hr = hr;

    if (!pfRenderer)
    {
        IEnumMediaTypes *enumMedia = NULL;
        hr = IPin_EnumMediaTypes(source_out, &enumMedia);
        if (SUCCEEDED(hr)) {
            AM_MEDIA_TYPE *mediaType;
            hr = IEnumMediaTypes_Next(enumMedia, 1, &mediaType, NULL);
            if (SUCCEEDED(hr)) {
                if (IsEqualIID(&mediaType->majortype, &MEDIATYPE_Video)) {
                    hr = CoCreateInstance(&CLSID_VideoRenderer, NULL, CLSCTX_INPROC_SERVER,
                            &IID_IBaseFilter, (void**)&pfRenderer);
                } else if (IsEqualIID(&mediaType->majortype, &MEDIATYPE_Audio)) {
                    hr = CoCreateInstance(&CLSID_DSoundRender, NULL, CLSCTX_INPROC_SERVER,
                            &IID_IBaseFilter, (void**)&pfRenderer);
                } else {
                    FIXME("cannot automatically load renderer for majortype %s\n", debugstr_guid(&mediaType->majortype));
                    hr = E_FAIL;
                }
                if (SUCCEEDED(hr)) {
                    rendererNeedsRelease = TRUE;
                    hr = IGraphBuilder_AddFilter(This->mygraph, pfRenderer, NULL);
                }
                DeleteMediaType(mediaType);
            }
            IEnumMediaTypes_Release(enumMedia);
        }
        if (FAILED(hr)) {
            if (rendererNeedsRelease)
                IBaseFilter_Release(pfRenderer);
            IPin_Release(source_out);
            return hr;
        }
    }

    hr = ICaptureGraphBuilder2_FindPin(iface, (IUnknown*)pfRenderer, PINDIR_INPUT, NULL, NULL, TRUE, 0, &renderer_in);
    if (FAILED(hr))
    {
        if (rendererNeedsRelease)
            IBaseFilter_Release(pfRenderer);
        IPin_Release(source_out);
        return hr;
    }

    if (!pfCompressor)
        hr = IGraphBuilder_Connect(This->mygraph, source_out, renderer_in);
    else
    {
        IPin *compressor_in, *compressor_out;

        hr = ICaptureGraphBuilder2_FindPin(iface, (IUnknown*)pfCompressor,
                PINDIR_INPUT, NULL, NULL, TRUE, 0, &compressor_in);
        if (SUCCEEDED(hr))
        {
            hr = IGraphBuilder_Connect(This->mygraph, source_out, compressor_in);
            IPin_Release(compressor_in);
        }

        if (SUCCEEDED(hr))
        {
            hr = ICaptureGraphBuilder2_FindPin(iface, (IUnknown*)pfCompressor,
                    PINDIR_OUTPUT, NULL, NULL, TRUE, 0, &compressor_out);
            if (SUCCEEDED(hr))
            {
                hr = IGraphBuilder_Connect(This->mygraph, compressor_out, renderer_in);
                IPin_Release(compressor_out);
            }
        }
    }

    IPin_Release(source_out);
    IPin_Release(renderer_in);
    if (rendererNeedsRelease)
        IBaseFilter_Release(pfRenderer);
    if (SUCCEEDED(hr))
        return return_hr;
    return hr;
}

static HRESULT WINAPI
fnCaptureGraphBuilder2_ControlStream(ICaptureGraphBuilder2 * iface,
                                     const GUID *pCategory,
                                     const GUID *pType,
                                     IBaseFilter *pFilter,
                                     REFERENCE_TIME *pstart,
                                     REFERENCE_TIME *pstop,
                                     WORD wStartCookie,
                                     WORD wStopCookie)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder2(iface);

    FIXME("(%p/%p)->(%s, %s, %p, %p, %p, %i, %i) Stub!\n", This, iface,
          debugstr_guid(pCategory), debugstr_guid(pType),
          pFilter, pstart, pstop, wStartCookie, wStopCookie);

    return E_NOTIMPL;
}

static HRESULT WINAPI
fnCaptureGraphBuilder2_AllocCapFile(ICaptureGraphBuilder2 * iface,
                                    LPCOLESTR lpwstr,
                                    DWORDLONG dwlSize)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder2(iface);

    FIXME("(%p/%p)->(%s, 0x%s) Stub!\n", This, iface,
          debugstr_w(lpwstr), wine_dbgstr_longlong(dwlSize));

    return E_NOTIMPL;
}

static HRESULT WINAPI
fnCaptureGraphBuilder2_CopyCaptureFile(ICaptureGraphBuilder2 * iface,
                                       LPOLESTR lpwstrOld,
                                       LPOLESTR lpwstrNew,
                                       int fAllowEscAbort,
                                       IAMCopyCaptureFileProgress *pCallback)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder2(iface);

    FIXME("(%p/%p)->(%s, %s, %i, %p) Stub!\n", This, iface,
          debugstr_w(lpwstrOld), debugstr_w(lpwstrNew),
          fAllowEscAbort, pCallback);

    return E_NOTIMPL;
}

static HRESULT WINAPI
fnCaptureGraphBuilder2_FindPin(ICaptureGraphBuilder2 * iface,
                               IUnknown *pSource,
                               PIN_DIRECTION pindir,
                               const GUID *pCategory,
                               const GUID *pType,
                               BOOL fUnconnected,
                               INT num,
                               IPin **ppPin)
{
    HRESULT hr;
    IEnumPins *enumpins = NULL;
    IPin *pin;
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder2(iface);

    TRACE("(%p/%p)->(%p, %x, %s, %s, %d, %i, %p)\n", This, iface,
          pSource, pindir, debugstr_guid(pCategory), debugstr_guid(pType),
          fUnconnected, num, ppPin);

    pin = NULL;

    hr = IUnknown_QueryInterface(pSource, &IID_IPin, (void**)&pin);
    if (hr == E_NOINTERFACE)
    {
        IBaseFilter *filter = NULL;
        int numcurrent = 0;

        hr = IUnknown_QueryInterface(pSource, &IID_IBaseFilter, (void**)&filter);
        if (hr == E_NOINTERFACE)
        {
            WARN("Input not filter or pin?!\n");
            return E_NOINTERFACE;
        }

        hr = IBaseFilter_EnumPins(filter, &enumpins);
        if (FAILED(hr))
        {
            WARN("Could not enumerate\n");
            IBaseFilter_Release(filter);
            return hr;
        }

        while (1)
        {
            ULONG fetched;

            hr = IEnumPins_Next(enumpins, 1, &pin, &fetched);
            if (hr == VFW_E_ENUM_OUT_OF_SYNC)
            {
                numcurrent = 0;
                IEnumPins_Reset(enumpins);
                pin = NULL;
                continue;
            }
            if (hr != S_OK)
                break;
            if (fetched != 1)
            {
                hr = E_FAIL;
                break;
            }

            TRACE("Testing match\n");
            if (pin_matches(pin, pindir, pCategory, pType, fUnconnected) && numcurrent++ == num)
                break;
            IPin_Release(pin);
            pin = NULL;
        }
        IEnumPins_Release(enumpins);
        IBaseFilter_Release(filter);

        if (hr != S_OK)
        {
            WARN("Could not find %s pin # %d\n", (pindir == PINDIR_OUTPUT ? "output" : "input"), numcurrent);
            return E_FAIL;
        }
    }
    else if (!pin_matches(pin, pindir, pCategory, pType, fUnconnected))
    {
        IPin_Release(pin);
        return E_FAIL;
    }

    *ppPin = pin;
    return S_OK;
}

static const ICaptureGraphBuilder2Vtbl builder2_Vtbl =
{
    fnCaptureGraphBuilder2_QueryInterface,
    fnCaptureGraphBuilder2_AddRef,
    fnCaptureGraphBuilder2_Release,
    fnCaptureGraphBuilder2_SetFilterGraph,
    fnCaptureGraphBuilder2_GetFilterGraph,
    fnCaptureGraphBuilder2_SetOutputFileName,
    fnCaptureGraphBuilder2_FindInterface,
    fnCaptureGraphBuilder2_RenderStream,
    fnCaptureGraphBuilder2_ControlStream,
    fnCaptureGraphBuilder2_AllocCapFile,
    fnCaptureGraphBuilder2_CopyCaptureFile,
    fnCaptureGraphBuilder2_FindPin
};


static HRESULT WINAPI
fnCaptureGraphBuilder_QueryInterface(ICaptureGraphBuilder * iface,
                                     REFIID riid, LPVOID * ppv)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder(iface);
    TRACE("%p --> Forwarding to v2 (%p)\n", iface, This);
    return ICaptureGraphBuilder2_QueryInterface(&This->ICaptureGraphBuilder2_iface, riid, ppv);
}

static ULONG WINAPI
fnCaptureGraphBuilder_AddRef(ICaptureGraphBuilder * iface)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder(iface);
    TRACE("%p --> Forwarding to v2 (%p)\n", iface, This);
    return ICaptureGraphBuilder2_AddRef(&This->ICaptureGraphBuilder2_iface);
}

static ULONG WINAPI
fnCaptureGraphBuilder_Release(ICaptureGraphBuilder * iface)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder(iface);
    TRACE("%p --> Forwarding to v2 (%p)\n", iface, This);
    return ICaptureGraphBuilder2_Release(&This->ICaptureGraphBuilder2_iface);
}

static HRESULT WINAPI
fnCaptureGraphBuilder_SetFiltergraph(ICaptureGraphBuilder * iface,
                                     IGraphBuilder *pfg)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder(iface);
    TRACE("%p --> Forwarding to v2 (%p)\n", iface, This);
    return ICaptureGraphBuilder2_SetFiltergraph(&This->ICaptureGraphBuilder2_iface, pfg);
}

static HRESULT WINAPI
fnCaptureGraphBuilder_GetFiltergraph(ICaptureGraphBuilder * iface,
                                     IGraphBuilder **pfg)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder(iface);
    TRACE("%p --> Forwarding to v2 (%p)\n", iface, This);
    return ICaptureGraphBuilder2_GetFiltergraph(&This->ICaptureGraphBuilder2_iface, pfg);
}

static HRESULT WINAPI
fnCaptureGraphBuilder_SetOutputFileName(ICaptureGraphBuilder * iface,
                                        const GUID *pType, LPCOLESTR lpstrFile,
                                        IBaseFilter **ppf, IFileSinkFilter **ppSink)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder(iface);
    TRACE("%p --> Forwarding to v2 (%p)\n", iface, This);
    return ICaptureGraphBuilder2_SetOutputFileName(&This->ICaptureGraphBuilder2_iface, pType,
                                                   lpstrFile, ppf, ppSink);
}

static HRESULT WINAPI
fnCaptureGraphBuilder_FindInterface(ICaptureGraphBuilder * iface,
                                    const GUID *pCategory, IBaseFilter *pf,
                                    REFIID riid, void **ppint)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder(iface);
    TRACE("%p --> Forwarding to v2 (%p)\n", iface, This);
    return ICaptureGraphBuilder2_FindInterface(&This->ICaptureGraphBuilder2_iface, pCategory, NULL,
                                               pf, riid, ppint);
}

static HRESULT WINAPI
fnCaptureGraphBuilder_RenderStream(ICaptureGraphBuilder * iface,
                                   const GUID *pCategory, IUnknown *pSource,
                                   IBaseFilter *pfCompressor, IBaseFilter *pfRenderer)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder(iface);
    TRACE("%p --> Forwarding to v2 (%p)\n", iface, This);
    return ICaptureGraphBuilder2_RenderStream(&This->ICaptureGraphBuilder2_iface, pCategory, NULL,
                                              pSource, pfCompressor, pfRenderer);
}

static HRESULT WINAPI
fnCaptureGraphBuilder_ControlStream(ICaptureGraphBuilder * iface,
                                    const GUID *pCategory, IBaseFilter *pFilter,
                                    REFERENCE_TIME *pstart, REFERENCE_TIME *pstop,
                                    WORD wStartCookie, WORD wStopCookie)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder(iface);
    TRACE("%p --> Forwarding to v2 (%p)\n", iface, This);
    return ICaptureGraphBuilder2_ControlStream(&This->ICaptureGraphBuilder2_iface, pCategory, NULL,
                                               pFilter, pstart, pstop, wStartCookie, wStopCookie);
}

static HRESULT WINAPI
fnCaptureGraphBuilder_AllocCapFile(ICaptureGraphBuilder * iface,
                                   LPCOLESTR lpstr, DWORDLONG dwlSize)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder(iface);
    TRACE("%p --> Forwarding to v2 (%p)\n", iface, This);
    return ICaptureGraphBuilder2_AllocCapFile(&This->ICaptureGraphBuilder2_iface, lpstr, dwlSize);
}

static HRESULT WINAPI
fnCaptureGraphBuilder_CopyCaptureFile(ICaptureGraphBuilder * iface,
                                      LPOLESTR lpwstrOld, LPOLESTR lpwstrNew,
                                      int fAllowEscAbort,
                                      IAMCopyCaptureFileProgress *pCallback)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder(iface);
    TRACE("%p --> Forwarding to v2 (%p)\n", iface, This);
    return ICaptureGraphBuilder2_CopyCaptureFile(&This->ICaptureGraphBuilder2_iface, lpwstrOld,
                                                 lpwstrNew, fAllowEscAbort, pCallback);
}

static const ICaptureGraphBuilderVtbl builder_Vtbl =
{
   fnCaptureGraphBuilder_QueryInterface,
   fnCaptureGraphBuilder_AddRef,
   fnCaptureGraphBuilder_Release,
   fnCaptureGraphBuilder_SetFiltergraph,
   fnCaptureGraphBuilder_GetFiltergraph,
   fnCaptureGraphBuilder_SetOutputFileName,
   fnCaptureGraphBuilder_FindInterface,
   fnCaptureGraphBuilder_RenderStream,
   fnCaptureGraphBuilder_ControlStream,
   fnCaptureGraphBuilder_AllocCapFile,
   fnCaptureGraphBuilder_CopyCaptureFile
};
