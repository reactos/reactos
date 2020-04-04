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
#include "config.h"

#include <stdio.h>
#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winerror.h"
#include "objbase.h"

#include "evcode.h"
#include "strmif.h"
#include "control.h"
#include "vfwmsgs.h"
/*
 *#include "amvideo.h"
 *#include "mmreg.h"
 *#include "dshow.h"
 *#include "ddraw.h"
 */
#include "uuids.h"
#include "qcap_main.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qcap);

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


IUnknown * CALLBACK QCAP_createCaptureGraphBuilder2(IUnknown *pUnkOuter,
                                                    HRESULT *phr)
{
    CaptureGraphImpl * pCapture = NULL;

    TRACE("(%p, %p)\n", pUnkOuter, phr);

    *phr = CLASS_E_NOAGGREGATION;
    if (pUnkOuter)
    {
        return NULL;
    }
    *phr = E_OUTOFMEMORY;

    pCapture = CoTaskMemAlloc(sizeof(CaptureGraphImpl));
    if (pCapture)
    {
        pCapture->ICaptureGraphBuilder2_iface.lpVtbl = &builder2_Vtbl;
        pCapture->ICaptureGraphBuilder_iface.lpVtbl = &builder_Vtbl;
        pCapture->ref = 1;
        pCapture->mygraph = NULL;
        InitializeCriticalSection(&pCapture->csFilter);
        pCapture->csFilter.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": CaptureGraphImpl.csFilter");
        *phr = S_OK;
        ObjectRefCount(TRUE);
    }
    return (IUnknown *)&pCapture->ICaptureGraphBuilder_iface;
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

    TRACE("(%p/%p)->() AddRef from %d\n", This, iface, ref - 1);
    return ref;
}

static ULONG WINAPI fnCaptureGraphBuilder2_Release(ICaptureGraphBuilder2 * iface)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder2(iface);
    DWORD ref = InterlockedDecrement(&This->ref);

    TRACE("(%p/%p)->() Release from %d\n", This, iface, ref + 1);

    if (!ref)
    {
        This->csFilter.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->csFilter);
        if (This->mygraph)
            IGraphBuilder_Release(This->mygraph);
        CoTaskMemFree(This);
        ObjectRefCount(FALSE);
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

static HRESULT WINAPI
fnCaptureGraphBuilder2_FindInterface(ICaptureGraphBuilder2 * iface,
                                     const GUID *pCategory,
                                     const GUID *pType,
                                     IBaseFilter *pf,
                                     REFIID riid,
                                     void **ppint)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder2(iface);

    FIXME("(%p/%p)->(%s, %s, %p, %s, %p) - workaround stub!\n", This, iface,
          debugstr_guid(pCategory), debugstr_guid(pType),
          pf, debugstr_guid(riid), ppint);

    return IBaseFilter_QueryInterface(pf, riid, ppint);
    /* Looks for the specified interface on the filter, upstream and
     * downstream from the filter, and, optionally, only on the output
     * pin of the given category.
     */
}

static HRESULT match_smart_tee_pin(CaptureGraphImpl *This,
                                   const GUID *pCategory,
                                   const GUID *pType,
                                   IUnknown *pSource,
                                   IPin **source_out)
{
    static const WCHAR inputW[] = {'I','n','p','u','t',0};
    static const WCHAR captureW[] = {'C','a','p','t','u','r','e',0};
    static const WCHAR previewW[] = {'P','r','e','v','i','e','w',0};
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
                hr = IBaseFilter_FindPin(smartTee, inputW, &smartTeeInput);
                if (SUCCEEDED(hr)) {
                    hr = IGraphBuilder_ConnectDirect(This->mygraph, capture, smartTeeInput, NULL);
                    IPin_Release(smartTeeInput);
                }
            }
        }
        if (FAILED(hr)) {
            TRACE("adding SmartTee failed with hr=0x%08x\n", hr);
            hr = E_INVALIDARG;
            goto end;
        }
    } else {
        hr = E_INVALIDARG;
        goto end;
    }
    if (IsEqualIID(pCategory, &PIN_CATEGORY_CAPTURE))
        hr = IBaseFilter_FindPin(smartTee, captureW, source_out);
    else {
        hr = IBaseFilter_FindPin(smartTee, previewW, source_out);
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
    TRACE("for %s returning hr=0x%08x, *source_out=%p\n", IsEqualIID(pCategory, &PIN_CATEGORY_CAPTURE) ? "capture" : "preview", hr, source_out ? *source_out : 0);
    return hr;
}

static HRESULT find_unconnected_pin(CaptureGraphImpl *This,
        const GUID *pCategory, const GUID *pType, IUnknown *pSource, IPin **out_pin)
{
    int index = 0;
    IPin *source_out;
    HRESULT hr;
    BOOL usedSmartTeePreviewPin = FALSE;

    /* depth-first search the graph for the first unconnected pin that matches
     * the given category and type */
    for(;;){
        IPin *nextpin;

        if (pCategory && (IsEqualIID(pCategory, &PIN_CATEGORY_CAPTURE) || IsEqualIID(pCategory, &PIN_CATEGORY_PREVIEW))){
            IBaseFilter *sourceFilter = NULL;
            hr = IUnknown_QueryInterface(pSource, &IID_IBaseFilter, (void**)&sourceFilter);
            if (SUCCEEDED(hr)) {
                hr = match_smart_tee_pin(This, pCategory, pType, pSource, &source_out);
                if (hr == VFW_S_NOPREVIEWPIN)
                    usedSmartTeePreviewPin = TRUE;
                IBaseFilter_Release(sourceFilter);
            } else {
                hr = ICaptureGraphBuilder2_FindPin(&This->ICaptureGraphBuilder2_iface, pSource, PINDIR_OUTPUT, pCategory, pType, FALSE, index, &source_out);
            }
            if (FAILED(hr))
                return E_INVALIDARG;
        } else {
            hr = ICaptureGraphBuilder2_FindPin(&This->ICaptureGraphBuilder2_iface, pSource, PINDIR_OUTPUT, pCategory, pType, FALSE, index, &source_out);
            if (FAILED(hr))
                return E_INVALIDARG;
        }

        hr = IPin_ConnectedTo(source_out, &nextpin);
        if(SUCCEEDED(hr)){
            PIN_INFO info;

            IPin_Release(source_out);

            hr = IPin_QueryPinInfo(nextpin, &info);
            if(FAILED(hr) || !info.pFilter){
                WARN("QueryPinInfo failed: %08x\n", hr);
                return hr;
            }

            hr = find_unconnected_pin(This, pCategory, pType, (IUnknown*)info.pFilter, out_pin);

            IBaseFilter_Release(info.pFilter);

            if(SUCCEEDED(hr))
                return hr;
        }else{
            *out_pin = source_out;
            if(usedSmartTeePreviewPin)
                return VFW_S_NOPREVIEWPIN;
            return S_OK;
        }

        index++;
    }
}

static HRESULT WINAPI
fnCaptureGraphBuilder2_RenderStream(ICaptureGraphBuilder2 * iface,
                                    const GUID *pCategory,
                                    const GUID *pType,
                                    IUnknown *pSource,
                                    IBaseFilter *pfCompressor,
                                    IBaseFilter *pfRenderer)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder2(iface);
    IPin *source_out = NULL, *renderer_in;
    BOOL rendererNeedsRelease = FALSE;
    HRESULT hr, return_hr = S_OK;

    FIXME("(%p/%p)->(%s, %s, %p, %p, %p) semi-stub!\n", This, iface,
          debugstr_guid(pCategory), debugstr_guid(pType),
          pSource, pfCompressor, pfRenderer);

    if (!This->mygraph)
    {
        FIXME("Need a capture graph\n");
        return E_UNEXPECTED;
    }

    if (pCategory && IsEqualIID(pCategory, &PIN_CATEGORY_VBI)) {
        FIXME("Tee/Sink-to-Sink filter not supported\n");
        return E_NOTIMPL;
    }

    hr = find_unconnected_pin(This, pCategory, pType, pSource, &source_out);
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

static HRESULT pin_matches(IPin *pin, PIN_DIRECTION direction, const GUID *cat, const GUID *type, BOOL unconnected)
{
    IPin *partner;
    PIN_DIRECTION pindir;
    HRESULT hr;

    hr = IPin_QueryDirection(pin, &pindir);

    if (unconnected && IPin_ConnectedTo(pin, &partner) == S_OK && partner!=NULL)
    {
        IPin_Release(partner);
        TRACE("No match, %p already connected to %p\n", pin, partner);
        return FAILED(hr) ? hr : S_FALSE;
    }

    if (FAILED(hr))
        return hr;
    if (SUCCEEDED(hr) && pindir != direction)
        return S_FALSE;

    if (cat)
    {
        IKsPropertySet *props;
        GUID category;
        DWORD fetched;

        hr = IPin_QueryInterface(pin, &IID_IKsPropertySet, (void**)&props);
        if (FAILED(hr))
            return S_FALSE;

        hr = IKsPropertySet_Get(props, &AMPROPSETID_Pin, 0, NULL,
                0, &category, sizeof(category), &fetched);
        IKsPropertySet_Release(props);
        if (FAILED(hr) || !IsEqualIID(&category, cat))
            return S_FALSE;
    }

    if (type)
    {
        IEnumMediaTypes *types;
        AM_MEDIA_TYPE *media_type;
        ULONG fetched;

        hr = IPin_EnumMediaTypes(pin, &types);
        if (FAILED(hr))
            return S_FALSE;

        IEnumMediaTypes_Reset(types);
        while (1) {
            if (IEnumMediaTypes_Next(types, 1, &media_type, &fetched) != S_OK || fetched != 1)
            {
                IEnumMediaTypes_Release(types);
                return S_FALSE;
            }

            if (IsEqualIID(&media_type->majortype, type))
            {
                DeleteMediaType(media_type);
                break;
            }
            DeleteMediaType(media_type);
        }
        IEnumMediaTypes_Release(types);
    }

    TRACE("Pin matched\n");
    return S_OK;
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
            hr = pin_matches(pin, pindir, pCategory, pType, fUnconnected);
            if (hr == S_OK && numcurrent++ == num)
                break;
            IPin_Release(pin);
            pin = NULL;
            if (FAILED(hr))
                break;
        }
        IEnumPins_Release(enumpins);
        IBaseFilter_Release(filter);

        if (hr != S_OK)
        {
            WARN("Could not find %s pin # %d\n", (pindir == PINDIR_OUTPUT ? "output" : "input"), numcurrent);
            return E_FAIL;
        }
    }
    else if (pin_matches(pin, pindir, pCategory, pType, fUnconnected) != S_OK)
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
