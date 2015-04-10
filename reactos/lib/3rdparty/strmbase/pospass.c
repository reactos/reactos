/*
 * Filter Seeking and Control Interfaces
 *
 * Copyright 2003 Robert Shearman
 * Copyright 2012 Aric Stewart, CodeWeavers
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
/* FIXME: critical sections */

#include "strmbase_private.h"

static const IMediaSeekingVtbl IMediaSeekingPassThru_Vtbl;
static const IMediaPositionVtbl IMediaPositionPassThru_Vtbl;

typedef struct PassThruImpl {
    IUnknown  IUnknown_inner;
    ISeekingPassThru ISeekingPassThru_iface;
    IMediaSeeking IMediaSeeking_iface;
    IMediaPosition IMediaPosition_iface;
    BaseDispatch baseDispatch;

    LONG ref;
    IUnknown * outer_unk;
    IPin * pin;
    BOOL bUnkOuterValid;
    BOOL bAggregatable;
    BOOL renderer;
    CRITICAL_SECTION time_cs;
    BOOL timevalid;
    REFERENCE_TIME time_earliest;
} PassThruImpl;

static inline PassThruImpl *impl_from_IUnknown_inner(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, PassThruImpl, IUnknown_inner);
}

static inline PassThruImpl *impl_from_ISeekingPassThru(ISeekingPassThru *iface)
{
    return CONTAINING_RECORD(iface, PassThruImpl, ISeekingPassThru_iface);
}

static inline PassThruImpl *impl_from_IMediaSeeking(IMediaSeeking *iface)
{
    return CONTAINING_RECORD(iface, PassThruImpl, IMediaSeeking_iface);
}

static inline PassThruImpl *impl_from_IMediaPosition(IMediaPosition *iface)
{
    return CONTAINING_RECORD(iface, PassThruImpl, IMediaPosition_iface);
}

static HRESULT WINAPI SeekInner_QueryInterface(IUnknown * iface,
					  REFIID riid,
					  LPVOID *ppvObj) {
    PassThruImpl *This = impl_from_IUnknown_inner(iface);
    TRACE("(%p)->(%s (%p), %p)\n", This, debugstr_guid(riid), riid, ppvObj);

    if (This->bAggregatable)
        This->bUnkOuterValid = TRUE;

    if (IsEqualGUID(&IID_IUnknown, riid))
    {
        *ppvObj = &(This->IUnknown_inner);
        TRACE("   returning IUnknown interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_ISeekingPassThru, riid)) {
        *ppvObj = &(This->ISeekingPassThru_iface);
        TRACE("   returning ISeekingPassThru interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IMediaSeeking, riid)) {
        *ppvObj = &(This->IMediaSeeking_iface);
        TRACE("   returning IMediaSeeking interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IMediaPosition, riid)) {
        *ppvObj = &(This->IMediaPosition_iface);
        TRACE("   returning IMediaPosition interface (%p)\n", *ppvObj);
    } else {
        *ppvObj = NULL;
        FIXME("unknown interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)(*ppvObj));
    return S_OK;
}

static ULONG WINAPI SeekInner_AddRef(IUnknown * iface) {
    PassThruImpl *This = impl_from_IUnknown_inner(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", This, ref);

    return ref;
}

static ULONG WINAPI SeekInner_Release(IUnknown * iface) {
    PassThruImpl *This = impl_from_IUnknown_inner(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", This, ref);

    if (ref == 0)
    {
        BaseDispatch_Destroy(&This->baseDispatch);
        This->time_cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->time_cs);
        CoTaskMemFree(This);
    }
    return ref;
}

static const IUnknownVtbl IInner_VTable =
{
    SeekInner_QueryInterface,
    SeekInner_AddRef,
    SeekInner_Release
};

/* Generic functions for aggregation */
static HRESULT SeekOuter_QueryInterface(PassThruImpl *This, REFIID riid, LPVOID *ppv)
{
    if (This->bAggregatable)
        This->bUnkOuterValid = TRUE;

    if (This->outer_unk)
    {
        if (This->bAggregatable)
            return IUnknown_QueryInterface(This->outer_unk, riid, ppv);

        if (IsEqualIID(riid, &IID_IUnknown))
        {
            HRESULT hr;

            IUnknown_AddRef((IUnknown *)&(This->IUnknown_inner));
            hr = IUnknown_QueryInterface((IUnknown *)&(This->IUnknown_inner), riid, ppv);
            IUnknown_Release((IUnknown *)&(This->IUnknown_inner));
            This->bAggregatable = TRUE;
            return hr;
        }

        *ppv = NULL;
        return E_NOINTERFACE;
    }

    return IUnknown_QueryInterface((IUnknown *)&(This->IUnknown_inner), riid, ppv);
}

static ULONG SeekOuter_AddRef(PassThruImpl *This)
{
    if (This->outer_unk && This->bUnkOuterValid)
        return IUnknown_AddRef(This->outer_unk);
    return IUnknown_AddRef((IUnknown *)&(This->IUnknown_inner));
}

static ULONG SeekOuter_Release(PassThruImpl *This)
{
    if (This->outer_unk && This->bUnkOuterValid)
        return IUnknown_Release(This->outer_unk);
    return IUnknown_Release((IUnknown *)&(This->IUnknown_inner));
}

static HRESULT WINAPI SeekingPassThru_QueryInterface(ISeekingPassThru *iface, REFIID riid, LPVOID *ppvObj)
{
    PassThruImpl *This = impl_from_ISeekingPassThru(iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return SeekOuter_QueryInterface(This, riid, ppvObj);
}

static ULONG WINAPI SeekingPassThru_AddRef(ISeekingPassThru *iface)
{
    PassThruImpl *This = impl_from_ISeekingPassThru(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return SeekOuter_AddRef(This);
}

static ULONG WINAPI SeekingPassThru_Release(ISeekingPassThru *iface)
{
    PassThruImpl *This = impl_from_ISeekingPassThru(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return SeekOuter_Release(This);
}

static HRESULT WINAPI SeekingPassThru_Init(ISeekingPassThru *iface, BOOL renderer, IPin *pin)
{
    PassThruImpl *This = impl_from_ISeekingPassThru(iface);

    TRACE("(%p/%p)->(%d, %p)\n", This, iface, renderer, pin);

    if (This->pin)
        FIXME("Re-initializing?\n");

    This->renderer = renderer;
    This->pin = pin;

    return S_OK;
}

static const ISeekingPassThruVtbl ISeekingPassThru_Vtbl =
{
    SeekingPassThru_QueryInterface,
    SeekingPassThru_AddRef,
    SeekingPassThru_Release,
    SeekingPassThru_Init
};

HRESULT WINAPI CreatePosPassThru(IUnknown* pUnkOuter, BOOL bRenderer, IPin *pPin, IUnknown **ppPassThru)
{
    HRESULT hr;
    ISeekingPassThru *passthru;

    hr = CoCreateInstance(&CLSID_SeekingPassThru, pUnkOuter, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void**)ppPassThru);
    if (FAILED(hr))
        return hr;

    IUnknown_QueryInterface(*ppPassThru, &IID_ISeekingPassThru, (void**)&passthru);
    hr = ISeekingPassThru_Init(passthru, bRenderer, pPin);
    ISeekingPassThru_Release(passthru);

    return hr;
}

HRESULT WINAPI PosPassThru_Construct(IUnknown *pUnkOuter, LPVOID *ppPassThru)
{
    PassThruImpl *fimpl;

    TRACE("(%p,%p)\n", pUnkOuter, ppPassThru);

    *ppPassThru = fimpl = CoTaskMemAlloc(sizeof(*fimpl));
    if (!fimpl)
        return E_OUTOFMEMORY;

    fimpl->outer_unk = pUnkOuter;
    fimpl->bUnkOuterValid = FALSE;
    fimpl->bAggregatable = FALSE;
    fimpl->IUnknown_inner.lpVtbl = &IInner_VTable;
    fimpl->ISeekingPassThru_iface.lpVtbl = &ISeekingPassThru_Vtbl;
    fimpl->IMediaSeeking_iface.lpVtbl = &IMediaSeekingPassThru_Vtbl;
    fimpl->IMediaPosition_iface.lpVtbl = &IMediaPositionPassThru_Vtbl;
    fimpl->ref = 1;
    fimpl->pin = NULL;
    fimpl->timevalid = FALSE;
    InitializeCriticalSection(&fimpl->time_cs);
    fimpl->time_cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": PassThruImpl.time_cs");
    BaseDispatch_Init(&fimpl->baseDispatch, &IID_IMediaPosition);
    return S_OK;
}

static HRESULT WINAPI MediaSeekingPassThru_QueryInterface(IMediaSeeking *iface, REFIID riid, LPVOID *ppvObj)
{
    PassThruImpl *This = impl_from_IMediaSeeking(iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return SeekOuter_QueryInterface(This, riid, ppvObj);
}

static ULONG WINAPI MediaSeekingPassThru_AddRef(IMediaSeeking *iface)
{
    PassThruImpl *This = impl_from_IMediaSeeking(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return SeekOuter_AddRef(This);
}

static ULONG WINAPI MediaSeekingPassThru_Release(IMediaSeeking *iface)
{
    PassThruImpl *This = impl_from_IMediaSeeking(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return SeekOuter_Release(This);
}

static HRESULT get_connected(PassThruImpl *This, REFIID riid, LPVOID *ppvObj) {
    HRESULT hr;
    IPin *pin;
    *ppvObj = NULL;
    hr = IPin_ConnectedTo(This->pin, &pin);
    if (FAILED(hr))
        return VFW_E_NOT_CONNECTED;
    hr = IPin_QueryInterface(pin, riid, ppvObj);
    IPin_Release(pin);
    if (FAILED(hr))
        hr = E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_GetCapabilities(IMediaSeeking * iface, DWORD * pCapabilities)
{
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p)\n", iface, This, pCapabilities);
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_GetCapabilities(seek, pCapabilities);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_CheckCapabilities(IMediaSeeking * iface, DWORD * pCapabilities)
{
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p)\n", iface, This, pCapabilities);
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_CheckCapabilities(seek, pCapabilities);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_IsFormatSupported(IMediaSeeking * iface, const GUID * pFormat)
{
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%s)\n", iface, This, debugstr_guid(pFormat));
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_IsFormatSupported(seek, pFormat);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_QueryPreferredFormat(IMediaSeeking * iface, GUID * pFormat)
{
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p)\n", iface, This, pFormat);
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_QueryPreferredFormat(seek, pFormat);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_GetTimeFormat(IMediaSeeking * iface, GUID * pFormat)
{
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p)\n", iface, This, pFormat);
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_GetTimeFormat(seek, pFormat);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_IsUsingTimeFormat(IMediaSeeking * iface, const GUID * pFormat)
{
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%s)\n", iface, This, debugstr_guid(pFormat));
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_IsUsingTimeFormat(seek, pFormat);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_SetTimeFormat(IMediaSeeking * iface, const GUID * pFormat)
{
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%s)\n", iface, This, debugstr_guid(pFormat));
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_SetTimeFormat(seek, pFormat);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_GetDuration(IMediaSeeking * iface, LONGLONG * pDuration)
{
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p)\n", iface, This, pDuration);
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_GetDuration(seek, pDuration);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_GetStopPosition(IMediaSeeking * iface, LONGLONG * pStop)
{
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p)\n", iface, This, pStop);
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_GetStopPosition(seek, pStop);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_GetCurrentPosition(IMediaSeeking * iface, LONGLONG * pCurrent)
{
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr = S_OK;
    TRACE("(%p/%p)->(%p)\n", iface, This, pCurrent);
    if (!pCurrent)
        return E_POINTER;
    EnterCriticalSection(&This->time_cs);
    if (This->timevalid)
        *pCurrent = This->time_earliest;
    else
        hr = E_FAIL;
    LeaveCriticalSection(&This->time_cs);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_ConvertTimeFormat(iface, pCurrent, NULL, *pCurrent, &TIME_FORMAT_MEDIA_TIME);
        return hr;
    }
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_GetCurrentPosition(seek, pCurrent);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_ConvertTimeFormat(IMediaSeeking * iface, LONGLONG * pTarget, const GUID * pTargetFormat, LONGLONG Source, const GUID * pSourceFormat)
{
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p,%s,%x%08x,%s)\n", iface, This, pTarget, debugstr_guid(pTargetFormat), (DWORD)(Source>>32), (DWORD)Source, debugstr_guid(pSourceFormat));
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_ConvertTimeFormat(seek, pTarget, pTargetFormat, Source, pSourceFormat);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_SetPositions(IMediaSeeking * iface, LONGLONG * pCurrent, DWORD dwCurrentFlags, LONGLONG * pStop, DWORD dwStopFlags)
{
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p,%x,%p,%x)\n", iface, This, pCurrent, dwCurrentFlags, pStop, dwStopFlags);
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_SetPositions(seek, pCurrent, dwCurrentFlags, pStop, dwStopFlags);
        IMediaSeeking_Release(seek);
    } else if (hr == VFW_E_NOT_CONNECTED)
        hr = S_OK;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_GetPositions(IMediaSeeking * iface, LONGLONG * pCurrent, LONGLONG * pStop)
{
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p, %p)\n", iface, This, pCurrent, pStop);
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_GetPositions(seek, pCurrent, pStop);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_GetAvailable(IMediaSeeking * iface, LONGLONG * pEarliest, LONGLONG * pLatest)
{
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p,%p)\n", iface, This, pEarliest, pLatest);
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_GetAvailable(seek, pEarliest, pLatest);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_SetRate(IMediaSeeking * iface, double dRate)
{
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%e)\n", iface, This, dRate);
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_SetRate(seek, dRate);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_GetRate(IMediaSeeking * iface, double * dRate)
{
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p/%p)->(%p)\n", iface, This, dRate);
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_GetRate(seek, dRate);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_GetPreroll(IMediaSeeking * iface, LONGLONG * pPreroll)
{
    PassThruImpl *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;
    TRACE("(%p)\n", pPreroll);
    hr = get_connected(This, &IID_IMediaSeeking, (LPVOID*)&seek);
    if (SUCCEEDED(hr)) {
        hr = IMediaSeeking_GetPreroll(seek, pPreroll);
        IMediaSeeking_Release(seek);
    }
    else
        return E_NOTIMPL;
    return hr;
}

HRESULT WINAPI RendererPosPassThru_RegisterMediaTime(IUnknown *iface, REFERENCE_TIME start)
{
    PassThruImpl *This = impl_from_IUnknown_inner(iface);
    EnterCriticalSection(&This->time_cs);
    This->time_earliest = start;
    This->timevalid = TRUE;
    LeaveCriticalSection(&This->time_cs);
    return S_OK;
}

HRESULT WINAPI RendererPosPassThru_ResetMediaTime(IUnknown *iface)
{
    PassThruImpl *This = impl_from_IUnknown_inner(iface);
    EnterCriticalSection(&This->time_cs);
    This->timevalid = FALSE;
    LeaveCriticalSection(&This->time_cs);
    return S_OK;
}

HRESULT WINAPI RendererPosPassThru_EOS(IUnknown *iface)
{
    PassThruImpl *This = impl_from_IUnknown_inner(iface);
    REFERENCE_TIME time;
    HRESULT hr;
    hr = IMediaSeeking_GetStopPosition(&This->IMediaSeeking_iface, &time);
    EnterCriticalSection(&This->time_cs);
    if (SUCCEEDED(hr)) {
        This->timevalid = TRUE;
        This->time_earliest = time;
    } else
        This->timevalid = FALSE;
    LeaveCriticalSection(&This->time_cs);
    return hr;
}

static const IMediaSeekingVtbl IMediaSeekingPassThru_Vtbl =
{
    MediaSeekingPassThru_QueryInterface,
    MediaSeekingPassThru_AddRef,
    MediaSeekingPassThru_Release,
    MediaSeekingPassThru_GetCapabilities,
    MediaSeekingPassThru_CheckCapabilities,
    MediaSeekingPassThru_IsFormatSupported,
    MediaSeekingPassThru_QueryPreferredFormat,
    MediaSeekingPassThru_GetTimeFormat,
    MediaSeekingPassThru_IsUsingTimeFormat,
    MediaSeekingPassThru_SetTimeFormat,
    MediaSeekingPassThru_GetDuration,
    MediaSeekingPassThru_GetStopPosition,
    MediaSeekingPassThru_GetCurrentPosition,
    MediaSeekingPassThru_ConvertTimeFormat,
    MediaSeekingPassThru_SetPositions,
    MediaSeekingPassThru_GetPositions,
    MediaSeekingPassThru_GetAvailable,
    MediaSeekingPassThru_SetRate,
    MediaSeekingPassThru_GetRate,
    MediaSeekingPassThru_GetPreroll
};

static HRESULT WINAPI MediaPositionPassThru_QueryInterface(IMediaPosition *iface, REFIID riid, LPVOID *ppvObj)
{
    PassThruImpl *This = impl_from_IMediaPosition(iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return SeekOuter_QueryInterface(This, riid, ppvObj);
}

static ULONG WINAPI MediaPositionPassThru_AddRef(IMediaPosition *iface)
{
    PassThruImpl *This = impl_from_IMediaPosition(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return SeekOuter_AddRef(This);
}

static ULONG WINAPI MediaPositionPassThru_Release(IMediaPosition *iface)
{
    PassThruImpl *This = impl_from_IMediaPosition(iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return SeekOuter_Release(This);
}

static HRESULT WINAPI MediaPositionPassThru_GetTypeInfoCount(IMediaPosition *iface, UINT*pctinfo)
{
    PassThruImpl *This = impl_from_IMediaPosition(iface);

    return BaseDispatchImpl_GetTypeInfoCount(&This->baseDispatch, pctinfo);
}

static HRESULT WINAPI MediaPositionPassThru_GetTypeInfo(IMediaPosition *iface, UINT iTInfo, LCID lcid, ITypeInfo**ppTInfo)
{
    PassThruImpl *This = impl_from_IMediaPosition(iface);

    return BaseDispatchImpl_GetTypeInfo(&This->baseDispatch, &IID_NULL, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI MediaPositionPassThru_GetIDsOfNames(IMediaPosition *iface, REFIID riid, LPOLESTR*rgszNames, UINT cNames, LCID lcid, DISPID*rgDispId)
{
    PassThruImpl *This = impl_from_IMediaPosition(iface);

    return BaseDispatchImpl_GetIDsOfNames(&This->baseDispatch, riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI MediaPositionPassThru_Invoke(IMediaPosition *iface, DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS*pDispParams, VARIANT*pVarResult, EXCEPINFO*pExepInfo, UINT*puArgErr)
{
    PassThruImpl *This = impl_from_IMediaPosition(iface);
    HRESULT hr = S_OK;
    ITypeInfo *pTypeInfo;

    hr = BaseDispatchImpl_GetTypeInfo(&This->baseDispatch, riid, 1, lcid, &pTypeInfo);
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(pTypeInfo, &This->IMediaPosition_iface, dispIdMember, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);
        ITypeInfo_Release(pTypeInfo);
    }

    return hr;
}

static HRESULT WINAPI MediaPositionPassThru_get_Duration(IMediaPosition *iface, REFTIME *plength)
{
    PassThruImpl *This = impl_from_IMediaPosition(iface);
    IMediaPosition *pos;
    HRESULT hr;

    TRACE("(%p)\n", plength);

    hr = get_connected(This, &IID_IMediaPosition, (LPVOID*)&pos);
    if (SUCCEEDED(hr)) {
        hr = IMediaPosition_get_Duration(pos, plength);
        IMediaPosition_Release(pos);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaPositionPassThru_put_CurrentPosition(IMediaPosition *iface, REFTIME llTime)
{
    PassThruImpl *This = impl_from_IMediaPosition(iface);
    IMediaPosition *pos;
    HRESULT hr;

    TRACE("(%s)\n", wine_dbgstr_longlong(llTime));

    hr = get_connected(This, &IID_IMediaPosition, (LPVOID*)&pos);
    if (SUCCEEDED(hr)) {
        hr = IMediaPosition_put_CurrentPosition(pos, llTime);
        IMediaPosition_Release(pos);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaPositionPassThru_get_CurrentPosition(IMediaPosition *iface, REFTIME *pllTime)
{
    PassThruImpl *This = impl_from_IMediaPosition(iface);
    IMediaPosition *pos;
    HRESULT hr;

    TRACE("(%p)\n", pllTime);

    hr = get_connected(This, &IID_IMediaPosition, (LPVOID*)&pos);
    if (SUCCEEDED(hr)) {
        hr = IMediaPosition_get_CurrentPosition(pos, pllTime);
        IMediaPosition_Release(pos);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaPositionPassThru_get_StopTime(IMediaPosition *iface, REFTIME *pllTime)
{
    PassThruImpl *This = impl_from_IMediaPosition(iface);
    IMediaPosition *pos;
    HRESULT hr;

    TRACE("(%p)\n", pllTime);

    hr = get_connected(This, &IID_IMediaPosition, (LPVOID*)&pos);
    if (SUCCEEDED(hr)) {
        hr = IMediaPosition_get_StopTime(pos, pllTime);
        IMediaPosition_Release(pos);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaPositionPassThru_put_StopTime(IMediaPosition *iface, REFTIME llTime)
{
    PassThruImpl *This = impl_from_IMediaPosition(iface);
    IMediaPosition *pos;
    HRESULT hr;

    TRACE("(%s)\n", wine_dbgstr_longlong(llTime));

    hr = get_connected(This, &IID_IMediaPosition, (LPVOID*)&pos);
    if (SUCCEEDED(hr)) {
        hr = IMediaPosition_put_StopTime(pos, llTime);
        IMediaPosition_Release(pos);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaPositionPassThru_get_PrerollTime(IMediaPosition *iface, REFTIME *pllTime)
{
    PassThruImpl *This = impl_from_IMediaPosition(iface);
    IMediaPosition *pos;
    HRESULT hr;

    TRACE("(%p)\n", pllTime);

    hr = get_connected(This, &IID_IMediaPosition, (LPVOID*)&pos);
    if (SUCCEEDED(hr)) {
        hr = IMediaPosition_get_PrerollTime(pos, pllTime);
        IMediaPosition_Release(pos);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaPositionPassThru_put_PrerollTime(IMediaPosition *iface, REFTIME llTime)
{
    PassThruImpl *This = impl_from_IMediaPosition(iface);
    IMediaPosition *pos;
    HRESULT hr;

    TRACE("(%s)\n", wine_dbgstr_longlong(llTime));

    hr = get_connected(This, &IID_IMediaPosition, (LPVOID*)&pos);
    if (SUCCEEDED(hr)) {
        hr = IMediaPosition_put_PrerollTime(pos, llTime);
        IMediaPosition_Release(pos);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaPositionPassThru_put_Rate(IMediaPosition *iface, double dRate)
{
    PassThruImpl *This = impl_from_IMediaPosition(iface);
    IMediaPosition *pos;
    HRESULT hr;

    TRACE("(%f)\n", dRate);

    hr = get_connected(This, &IID_IMediaPosition, (LPVOID*)&pos);
    if (SUCCEEDED(hr)) {
        hr = IMediaPosition_put_Rate(pos, dRate);
        IMediaPosition_Release(pos);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaPositionPassThru_get_Rate(IMediaPosition *iface, double *pdRate)
{
    PassThruImpl *This = impl_from_IMediaPosition(iface);
    IMediaPosition *pos;
    HRESULT hr;

    TRACE("(%p)\n", pdRate);

    hr = get_connected(This, &IID_IMediaPosition, (LPVOID*)&pos);
    if (SUCCEEDED(hr)) {
        hr = IMediaPosition_get_Rate(pos, pdRate);
        IMediaPosition_Release(pos);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaPositionPassThru_CanSeekForward(IMediaPosition *iface, LONG *pCanSeekForward)
{
    PassThruImpl *This = impl_from_IMediaPosition(iface);
    IMediaPosition *pos;
    HRESULT hr;

    TRACE("(%p)\n", pCanSeekForward);

    hr = get_connected(This, &IID_IMediaPosition, (LPVOID*)&pos);
    if (SUCCEEDED(hr)) {
        hr = IMediaPosition_CanSeekForward(pos, pCanSeekForward);
        IMediaPosition_Release(pos);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static HRESULT WINAPI MediaPositionPassThru_CanSeekBackward(IMediaPosition *iface, LONG *pCanSeekBackward)
{
    PassThruImpl *This = impl_from_IMediaPosition(iface);
    IMediaPosition *pos;
    HRESULT hr;

    TRACE("(%p)\n", pCanSeekBackward);

    hr = get_connected(This, &IID_IMediaPosition, (LPVOID*)&pos);
    if (SUCCEEDED(hr)) {
        hr = IMediaPosition_CanSeekBackward(pos, pCanSeekBackward);
        IMediaPosition_Release(pos);
    }
    else
        return E_NOTIMPL;
    return hr;
}

static const IMediaPositionVtbl IMediaPositionPassThru_Vtbl =
{
    MediaPositionPassThru_QueryInterface,
    MediaPositionPassThru_AddRef,
    MediaPositionPassThru_Release,
    MediaPositionPassThru_GetTypeInfoCount,
    MediaPositionPassThru_GetTypeInfo,
    MediaPositionPassThru_GetIDsOfNames,
    MediaPositionPassThru_Invoke,
    MediaPositionPassThru_get_Duration,
    MediaPositionPassThru_put_CurrentPosition,
    MediaPositionPassThru_get_CurrentPosition,
    MediaPositionPassThru_get_StopTime,
    MediaPositionPassThru_put_StopTime,
    MediaPositionPassThru_get_PrerollTime,
    MediaPositionPassThru_put_PrerollTime,
    MediaPositionPassThru_put_Rate,
    MediaPositionPassThru_get_Rate,
    MediaPositionPassThru_CanSeekForward,
    MediaPositionPassThru_CanSeekBackward
};
