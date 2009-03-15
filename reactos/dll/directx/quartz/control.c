/*
 * Filter Seeking and Control Interfaces
 *
 * Copyright 2003 Robert Shearman
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

#include "quartz_private.h"
#include "control_private.h"

#include "uuids.h"
#include "wine/debug.h"

#include <assert.h>

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static const IMediaSeekingVtbl IMediaSeekingPassThru_Vtbl;

typedef struct PassThruImpl {
    const ISeekingPassThruVtbl *IPassThru_vtbl;
    const IUnknownVtbl *IInner_vtbl;
    const IMediaSeekingVtbl *IMediaSeeking_vtbl;

    LONG ref;
    IUnknown * pUnkOuter;
    IPin * pin;
    BOOL bUnkOuterValid;
    BOOL bAggregatable;
    BOOL renderer;
} PassThruImpl;

static HRESULT WINAPI SeekInner_QueryInterface(IUnknown * iface,
					  REFIID riid,
					  LPVOID *ppvObj) {
    ICOM_THIS_MULTI(PassThruImpl, IInner_vtbl, iface);
    TRACE("(%p)->(%s (%p), %p)\n", This, debugstr_guid(riid), riid, ppvObj);

    if (This->bAggregatable)
        This->bUnkOuterValid = TRUE;

    if (IsEqualGUID(&IID_IUnknown, riid))
    {
        *ppvObj = &(This->IInner_vtbl);
        TRACE("   returning IUnknown interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_ISeekingPassThru, riid)) {
        *ppvObj = &(This->IPassThru_vtbl);
        TRACE("   returning ISeekingPassThru interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IMediaSeeking, riid)) {
        *ppvObj = &(This->IMediaSeeking_vtbl);
        TRACE("   returning IMediaSeeking interface (%p)\n", *ppvObj);
    } else {
        *ppvObj = NULL;
        FIXME("unknown interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)(*ppvObj));
    return S_OK;
}

static ULONG WINAPI SeekInner_AddRef(IUnknown * iface) {
    ICOM_THIS_MULTI(PassThruImpl, IInner_vtbl, iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", This, ref);

    return ref;
}

static ULONG WINAPI SeekInner_Release(IUnknown * iface) {
    ICOM_THIS_MULTI(PassThruImpl, IInner_vtbl, iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %d\n", This, ref);

    if (ref == 0)
    {
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

    if (This->pUnkOuter)
    {
        if (This->bAggregatable)
            return IUnknown_QueryInterface(This->pUnkOuter, riid, ppv);

        if (IsEqualIID(riid, &IID_IUnknown))
        {
            HRESULT hr;

            IUnknown_AddRef((IUnknown *)&(This->IInner_vtbl));
            hr = IUnknown_QueryInterface((IUnknown *)&(This->IInner_vtbl), riid, ppv);
            IUnknown_Release((IUnknown *)&(This->IInner_vtbl));
            This->bAggregatable = TRUE;
            return hr;
        }

        *ppv = NULL;
        return E_NOINTERFACE;
    }

    return IUnknown_QueryInterface((IUnknown *)&(This->IInner_vtbl), riid, ppv);
}

static ULONG SeekOuter_AddRef(PassThruImpl *This)
{
    if (This->pUnkOuter && This->bUnkOuterValid)
        return IUnknown_AddRef(This->pUnkOuter);
    return IUnknown_AddRef((IUnknown *)&(This->IInner_vtbl));
}

static ULONG SeekOuter_Release(PassThruImpl *This)
{
    if (This->pUnkOuter && This->bUnkOuterValid)
        return IUnknown_Release(This->pUnkOuter);
    return IUnknown_Release((IUnknown *)&(This->IInner_vtbl));
}

static HRESULT WINAPI SeekingPassThru_QueryInterface(ISeekingPassThru *iface, REFIID riid, LPVOID *ppvObj)
{
    ICOM_THIS_MULTI(PassThruImpl, IPassThru_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return SeekOuter_QueryInterface(This, riid, ppvObj);
}

static ULONG WINAPI SeekingPassThru_AddRef(ISeekingPassThru *iface)
{
    ICOM_THIS_MULTI(PassThruImpl, IPassThru_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return SeekOuter_AddRef(This);
}

static ULONG WINAPI SeekingPassThru_Release(ISeekingPassThru *iface)
{
    ICOM_THIS_MULTI(PassThruImpl, IPassThru_vtbl, iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return SeekOuter_Release(This);
}

static HRESULT WINAPI SeekingPassThru_Init(ISeekingPassThru *iface, BOOL renderer, IPin *pin)
{
    ICOM_THIS_MULTI(PassThruImpl, IPassThru_vtbl, iface);

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

HRESULT SeekingPassThru_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    PassThruImpl *fimpl;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    *ppObj = fimpl = CoTaskMemAlloc(sizeof(*fimpl));
    if (!fimpl)
        return E_OUTOFMEMORY;

    fimpl->pUnkOuter = pUnkOuter;
    fimpl->bUnkOuterValid = FALSE;
    fimpl->bAggregatable = FALSE;
    fimpl->IInner_vtbl = &IInner_VTable;
    fimpl->IPassThru_vtbl = &ISeekingPassThru_Vtbl;
    fimpl->IMediaSeeking_vtbl = &IMediaSeekingPassThru_Vtbl;
    fimpl->ref = 1;
    fimpl->pin = NULL;
    return S_OK;
}

typedef HRESULT (*SeekFunc)( IMediaSeeking *to, LPVOID arg );

static HRESULT ForwardCmdSeek( PCRITICAL_SECTION crit_sect, IBaseFilter* from, SeekFunc fnSeek, LPVOID arg )
{
    HRESULT hr = S_OK;
    HRESULT hr_return = S_OK;
    IEnumPins *enumpins = NULL;
    BOOL foundend = FALSE, allnotimpl = TRUE;

    hr = IBaseFilter_EnumPins( from, &enumpins );
    if (FAILED(hr))
        goto out;

    hr = IEnumPins_Reset( enumpins );
    while (hr == S_OK) {
        IPin *pin = NULL;
        hr = IEnumPins_Next( enumpins, 1, &pin, NULL );
        if (hr == VFW_E_ENUM_OUT_OF_SYNC)
        {
            hr = IEnumPins_Reset( enumpins );
            continue;
        }
        if (pin)
        {
            PIN_DIRECTION dir;

            IPin_QueryDirection( pin, &dir );
            if (dir == PINDIR_INPUT)
            {
                IPin *connected = NULL;

                IPin_ConnectedTo( pin, &connected );
                if (connected)
                {
                    HRESULT hr_local;
                    IMediaSeeking *seek = NULL;

                    hr_local = IPin_QueryInterface( connected, &IID_IMediaSeeking, (void**)&seek );
                    if (hr_local == S_OK)
                    {
                        foundend = TRUE;
                        if (crit_sect)
                        {
                            LeaveCriticalSection( crit_sect );
                            hr_local = fnSeek( seek , arg );
                            EnterCriticalSection( crit_sect );
                        }
                        else
                            hr_local = fnSeek( seek , arg );

                        if (hr_local != E_NOTIMPL)
                            allnotimpl = FALSE;

                        hr_return = updatehres( hr_return, hr_local );
                        IMediaSeeking_Release( seek );
                    }
                    IPin_Release(connected);
                }
            }
            IPin_Release( pin );
        }
    }
    if (foundend && allnotimpl)
        hr = E_NOTIMPL;
    else
        hr = hr_return;

out:
    TRACE("Returning: %08x\n", hr);
    return hr;
}


HRESULT MediaSeekingImpl_Init(IBaseFilter *pUserData, CHANGEPROC fnChangeStop, CHANGEPROC fnChangeCurrent, CHANGEPROC fnChangeRate, MediaSeekingImpl * pSeeking, PCRITICAL_SECTION crit_sect)
{
    assert(fnChangeStop && fnChangeCurrent && fnChangeRate);

    pSeeking->refCount = 1;
    pSeeking->pUserData = pUserData;
    pSeeking->fnChangeRate = fnChangeRate;
    pSeeking->fnChangeStop = fnChangeStop;
    pSeeking->fnChangeCurrent = fnChangeCurrent;
    pSeeking->dwCapabilities = AM_SEEKING_CanSeekForwards |
        AM_SEEKING_CanSeekBackwards |
        AM_SEEKING_CanSeekAbsolute |
        AM_SEEKING_CanGetStopPos |
        AM_SEEKING_CanGetDuration;
    pSeeking->llCurrent = 0;
    pSeeking->llStop = ((ULONGLONG)0x80000000) << 32;
    pSeeking->llDuration = pSeeking->llStop;
    pSeeking->dRate = 1.0;
    pSeeking->timeformat = TIME_FORMAT_MEDIA_TIME;
    pSeeking->crst = crit_sect;

    return S_OK;
}

struct pos_args {
    LONGLONG* current, *stop;
    DWORD curflags, stopflags;
};

static HRESULT fwd_setposition(IMediaSeeking *seek, LPVOID pargs)
{
    struct pos_args *args = (void*)pargs;

    return IMediaSeeking_SetPositions(seek, args->current, args->curflags, args->stop, args->stopflags);
}

static HRESULT fwd_checkcaps(IMediaSeeking *iface, LPVOID pcaps)
{
    DWORD *caps = pcaps;
    return IMediaSeeking_CheckCapabilities(iface, caps);
}

static HRESULT fwd_settimeformat(IMediaSeeking *iface, LPVOID pformat)
{
    const GUID *format = pformat;
    return IMediaSeeking_SetTimeFormat(iface, format);
}

static HRESULT fwd_getduration(IMediaSeeking *iface, LPVOID pdur)
{
    LONGLONG *duration = pdur;
    LONGLONG mydur = *duration;
    HRESULT hr;

    hr = IMediaSeeking_GetDuration(iface, &mydur);
    if (FAILED(hr))
        return hr;

    if ((mydur < *duration) || (*duration < 0 && mydur > 0))
        *duration = mydur;
    return hr;
}

static HRESULT fwd_getstopposition(IMediaSeeking *iface, LPVOID pdur)
{
    LONGLONG *duration = pdur;
    LONGLONG mydur = *duration;
    HRESULT hr;

    hr = IMediaSeeking_GetStopPosition(iface, &mydur);
    if (FAILED(hr))
        return hr;

    if ((mydur < *duration) || (*duration < 0 && mydur > 0))
        *duration = mydur;
    return hr;
}

static HRESULT fwd_getcurposition(IMediaSeeking *iface, LPVOID pdur)
{
    LONGLONG *duration = pdur;
    LONGLONG mydur = *duration;
    HRESULT hr;

    hr = IMediaSeeking_GetCurrentPosition(iface, &mydur);
    if (FAILED(hr))
        return hr;

    if ((mydur < *duration) || (*duration < 0 && mydur > 0))
        *duration = mydur;
    return hr;
}

static HRESULT fwd_setrate(IMediaSeeking *iface, LPVOID prate)
{
    double *rate = prate;

    HRESULT hr;

    hr = IMediaSeeking_SetRate(iface, *rate);
    if (FAILED(hr))
        return hr;

    return hr;
}


HRESULT WINAPI MediaSeekingImpl_GetCapabilities(IMediaSeeking * iface, DWORD * pCapabilities)
{
    MediaSeekingImpl *This = (MediaSeekingImpl *)iface;

    TRACE("(%p)\n", pCapabilities);

    *pCapabilities = This->dwCapabilities;

    return S_OK;
}

HRESULT WINAPI MediaSeekingImpl_CheckCapabilities(IMediaSeeking * iface, DWORD * pCapabilities)
{
    MediaSeekingImpl *This = (MediaSeekingImpl *)iface;
    HRESULT hr;
    DWORD dwCommonCaps;

    TRACE("(%p)\n", pCapabilities);

    if (!pCapabilities)
        return E_POINTER;

    EnterCriticalSection(This->crst);
    hr = ForwardCmdSeek(This->crst, This->pUserData, fwd_checkcaps, pCapabilities);
    LeaveCriticalSection(This->crst);
    if (FAILED(hr) && hr != E_NOTIMPL)
        return hr;

    dwCommonCaps = *pCapabilities & This->dwCapabilities;

    if (!dwCommonCaps)
        hr = E_FAIL;
    else
        hr = (*pCapabilities == dwCommonCaps) ?  S_OK : S_FALSE;
    *pCapabilities = dwCommonCaps;

    return hr;
}

HRESULT WINAPI MediaSeekingImpl_IsFormatSupported(IMediaSeeking * iface, const GUID * pFormat)
{
    TRACE("(%s)\n", qzdebugstr_guid(pFormat));

    return (IsEqualIID(pFormat, &TIME_FORMAT_MEDIA_TIME) ? S_OK : S_FALSE);
}

HRESULT WINAPI MediaSeekingImpl_QueryPreferredFormat(IMediaSeeking * iface, GUID * pFormat)
{
    TRACE("(%s)\n", qzdebugstr_guid(pFormat));

    *pFormat = TIME_FORMAT_MEDIA_TIME;
    return S_OK;
}

HRESULT WINAPI MediaSeekingImpl_GetTimeFormat(IMediaSeeking * iface, GUID * pFormat)
{
    MediaSeekingImpl *This = (MediaSeekingImpl *)iface;
    TRACE("(%s)\n", qzdebugstr_guid(pFormat));

    EnterCriticalSection(This->crst);
    *pFormat = This->timeformat;
    LeaveCriticalSection(This->crst);

    return S_OK;
}

HRESULT WINAPI MediaSeekingImpl_IsUsingTimeFormat(IMediaSeeking * iface, const GUID * pFormat)
{
    MediaSeekingImpl *This = (MediaSeekingImpl *)iface;
    HRESULT hr = S_OK;

    TRACE("(%s)\n", qzdebugstr_guid(pFormat));

    EnterCriticalSection(This->crst);
    if (!IsEqualIID(pFormat, &This->timeformat))
        hr = S_FALSE;
    LeaveCriticalSection(This->crst);

    return hr;
}


HRESULT WINAPI MediaSeekingImpl_SetTimeFormat(IMediaSeeking * iface, const GUID * pFormat)
{
    MediaSeekingImpl *This = (MediaSeekingImpl *)iface;
    TRACE("(%s)\n", qzdebugstr_guid(pFormat));

    EnterCriticalSection(This->crst);
    ForwardCmdSeek(This->crst, This->pUserData, fwd_settimeformat, (LPVOID)pFormat);
    LeaveCriticalSection(This->crst);

    return (IsEqualIID(pFormat, &TIME_FORMAT_MEDIA_TIME) ? S_OK : S_FALSE);
}


HRESULT WINAPI MediaSeekingImpl_GetDuration(IMediaSeeking * iface, LONGLONG * pDuration)
{
    MediaSeekingImpl *This = (MediaSeekingImpl *)iface;

    TRACE("(%p)\n", pDuration);

    EnterCriticalSection(This->crst);
    *pDuration = This->llDuration;
    ForwardCmdSeek(This->crst, This->pUserData, fwd_getduration, pDuration);
    LeaveCriticalSection(This->crst);

    return S_OK;
}

HRESULT WINAPI MediaSeekingImpl_GetStopPosition(IMediaSeeking * iface, LONGLONG * pStop)
{
    MediaSeekingImpl *This = (MediaSeekingImpl *)iface;

    TRACE("(%p)\n", pStop);

    EnterCriticalSection(This->crst);
    *pStop = This->llStop;
    ForwardCmdSeek(This->crst, This->pUserData, fwd_getstopposition, pStop);
    LeaveCriticalSection(This->crst);

    return S_OK;
}

/* FIXME: Make use of the info the filter should expose */
HRESULT WINAPI MediaSeekingImpl_GetCurrentPosition(IMediaSeeking * iface, LONGLONG * pCurrent)
{
    MediaSeekingImpl *This = (MediaSeekingImpl *)iface;

    TRACE("(%p)\n", pCurrent);

    EnterCriticalSection(This->crst);
    *pCurrent = This->llCurrent;
    ForwardCmdSeek(This->crst, This->pUserData, fwd_getcurposition, pCurrent);
    LeaveCriticalSection(This->crst);

    return S_OK;
}

HRESULT WINAPI MediaSeekingImpl_ConvertTimeFormat(IMediaSeeking * iface, LONGLONG * pTarget, const GUID * pTargetFormat, LONGLONG Source, const GUID * pSourceFormat)
{
    if (IsEqualIID(pTargetFormat, &TIME_FORMAT_MEDIA_TIME) && IsEqualIID(pSourceFormat, &TIME_FORMAT_MEDIA_TIME))
    {
        *pTarget = Source;
        return S_OK;
    }
    /* FIXME: clear pTarget? */
    return E_INVALIDARG;
}

static inline LONGLONG Adjust(LONGLONG value, const LONGLONG * pModifier, DWORD dwFlags)
{
    switch (dwFlags & AM_SEEKING_PositioningBitsMask)
    {
    case AM_SEEKING_NoPositioning:
        return value;
    case AM_SEEKING_AbsolutePositioning:
        return *pModifier;
    case AM_SEEKING_RelativePositioning:
    case AM_SEEKING_IncrementalPositioning:
        return value + *pModifier;
    default:
        assert(FALSE);
        return 0;
    }
}

HRESULT WINAPI MediaSeekingImpl_SetPositions(IMediaSeeking * iface, LONGLONG * pCurrent, DWORD dwCurrentFlags, LONGLONG * pStop, DWORD dwStopFlags)
{
    MediaSeekingImpl *This = (MediaSeekingImpl *)iface;
    BOOL bChangeCurrent = FALSE, bChangeStop = FALSE;
    LONGLONG llNewCurrent, llNewStop;
    struct pos_args args;

    TRACE("(%p, %x, %p, %x)\n", pCurrent, dwCurrentFlags, pStop, dwStopFlags);

    args.current = pCurrent;
    args.stop = pStop;
    args.curflags = dwCurrentFlags;
    args.stopflags = dwStopFlags;

    EnterCriticalSection(This->crst);

    llNewCurrent = Adjust(This->llCurrent, pCurrent, dwCurrentFlags);
    llNewStop = Adjust(This->llStop, pStop, dwStopFlags);

    if (pCurrent)
        bChangeCurrent = TRUE;
    if (llNewStop != This->llStop)
        bChangeStop = TRUE;

    TRACE("Old: %u, New: %u\n", (DWORD)(This->llCurrent/10000000), (DWORD)(llNewCurrent/10000000));

    This->llCurrent = llNewCurrent;
    This->llStop = llNewStop;

    if (pCurrent && (dwCurrentFlags & AM_SEEKING_ReturnTime))
        *pCurrent = llNewCurrent;
    if (pStop && (dwStopFlags & AM_SEEKING_ReturnTime))
        *pStop = llNewStop;

    ForwardCmdSeek(This->crst, This->pUserData, fwd_setposition, &args);
    LeaveCriticalSection(This->crst);

    if (bChangeCurrent)
        This->fnChangeCurrent(This->pUserData);
    if (bChangeStop)
        This->fnChangeStop(This->pUserData);

    return S_OK;
}

HRESULT WINAPI MediaSeekingImpl_GetPositions(IMediaSeeking * iface, LONGLONG * pCurrent, LONGLONG * pStop)
{
    MediaSeekingImpl *This = (MediaSeekingImpl *)iface;

    TRACE("(%p, %p)\n", pCurrent, pStop);

    EnterCriticalSection(This->crst);
    *pCurrent = This->llCurrent;
    *pStop = This->llStop;
    LeaveCriticalSection(This->crst);

    return S_OK;
}

HRESULT WINAPI MediaSeekingImpl_GetAvailable(IMediaSeeking * iface, LONGLONG * pEarliest, LONGLONG * pLatest)
{
    MediaSeekingImpl *This = (MediaSeekingImpl *)iface;

    TRACE("(%p, %p)\n", pEarliest, pLatest);

    EnterCriticalSection(This->crst);
    *pEarliest = 0;
    *pLatest = This->llDuration;
    LeaveCriticalSection(This->crst);

    return S_OK;
}

HRESULT WINAPI MediaSeekingImpl_SetRate(IMediaSeeking * iface, double dRate)
{
    MediaSeekingImpl *This = (MediaSeekingImpl *)iface;
    BOOL bChangeRate = (dRate != This->dRate);
    HRESULT hr = S_OK;

    TRACE("(%e)\n", dRate);

    if (dRate > 100 || dRate < .001)
    {
        FIXME("Excessive rate %e, ignoring\n", dRate);
        return VFW_E_UNSUPPORTED_AUDIO;
    }

    EnterCriticalSection(This->crst);
    This->dRate = dRate;
    if (bChangeRate)
        hr = This->fnChangeRate(This->pUserData);
    ForwardCmdSeek(This->crst, This->pUserData, fwd_setrate, &dRate);
    LeaveCriticalSection(This->crst);

    return hr;
}

HRESULT WINAPI MediaSeekingImpl_GetRate(IMediaSeeking * iface, double * dRate)
{
    MediaSeekingImpl *This = (MediaSeekingImpl *)iface;

    TRACE("(%p)\n", dRate);

    EnterCriticalSection(This->crst);
    /* Forward? */
    *dRate = This->dRate;
    LeaveCriticalSection(This->crst);

    return S_OK;
}

HRESULT WINAPI MediaSeekingImpl_GetPreroll(IMediaSeeking * iface, LONGLONG * pPreroll)
{
    TRACE("(%p)\n", pPreroll);

    *pPreroll = 0;
    return S_OK;
}

static HRESULT WINAPI MediaSeekingPassThru_QueryInterface(IMediaSeeking *iface, REFIID riid, LPVOID *ppvObj)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return SeekOuter_QueryInterface(This, riid, ppvObj);
}

static ULONG WINAPI MediaSeekingPassThru_AddRef(IMediaSeeking *iface)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return SeekOuter_AddRef(This);
}

static ULONG WINAPI MediaSeekingPassThru_Release(IMediaSeeking *iface)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->()\n", iface, This);

    return SeekOuter_Release(This);
}

static HRESULT WINAPI MediaSeekingPassThru_GetCapabilities(IMediaSeeking * iface, DWORD * pCapabilities)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", iface, This, pCapabilities);

    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeekingPassThru_CheckCapabilities(IMediaSeeking * iface, DWORD * pCapabilities)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", iface, This, pCapabilities);

    if (!pCapabilities)
        return E_POINTER;

    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeekingPassThru_IsFormatSupported(IMediaSeeking * iface, const GUID * pFormat)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);
    TRACE("(%p/%p)->(%s)\n", iface, This, qzdebugstr_guid(pFormat));

    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeekingPassThru_QueryPreferredFormat(IMediaSeeking * iface, GUID * pFormat)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);
    TRACE("(%p/%p)->(%p)\n", iface, This, pFormat);

    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeekingPassThru_GetTimeFormat(IMediaSeeking * iface, GUID * pFormat)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);
    TRACE("(%p/%p)->(%p)\n", iface, This, pFormat);

    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeekingPassThru_IsUsingTimeFormat(IMediaSeeking * iface, const GUID * pFormat)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%s)\n", iface, This, qzdebugstr_guid(pFormat));

    FIXME("stub\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI MediaSeekingPassThru_SetTimeFormat(IMediaSeeking * iface, const GUID * pFormat)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);
    TRACE("(%p/%p)->(%s)\n", iface, This, qzdebugstr_guid(pFormat));

    FIXME("stub\n");
    return E_NOTIMPL;
}


static HRESULT WINAPI MediaSeekingPassThru_GetDuration(IMediaSeeking * iface, LONGLONG * pDuration)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);
    PIN_INFO info;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", iface, This, pDuration);

    IPin_QueryPinInfo(This->pin, &info);

    hr = ForwardCmdSeek(NULL, info.pFilter, fwd_getduration, pDuration);
    IBaseFilter_Release(info.pFilter);

    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_GetStopPosition(IMediaSeeking * iface, LONGLONG * pStop)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", iface, This, pStop);

    FIXME("stub\n");
    return E_NOTIMPL;
}

/* FIXME: Make use of the info the filter should expose */
static HRESULT WINAPI MediaSeekingPassThru_GetCurrentPosition(IMediaSeeking * iface, LONGLONG * pCurrent)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", iface, This, pCurrent);

    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeekingPassThru_ConvertTimeFormat(IMediaSeeking * iface, LONGLONG * pTarget, const GUID * pTargetFormat, LONGLONG Source, const GUID * pSourceFormat)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p,%s,%x%08x,%s)\n", iface, This, pTarget, debugstr_guid(pTargetFormat), (DWORD)(Source>>32), (DWORD)Source, debugstr_guid(pSourceFormat));

    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeekingPassThru_SetPositions(IMediaSeeking * iface, LONGLONG * pCurrent, DWORD dwCurrentFlags, LONGLONG * pStop, DWORD dwStopFlags)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);
    struct pos_args args;
    PIN_INFO info;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p)\n", iface, This, pCurrent, pStop);
    args.current = pCurrent;
    args.stop = pStop;
    args.curflags = dwCurrentFlags;
    args.stopflags = dwStopFlags;

    IPin_QueryPinInfo(This->pin, &info);

    hr = ForwardCmdSeek(NULL, info.pFilter, fwd_setposition, &args);
    IBaseFilter_Release(info.pFilter);
    return hr;
}

static HRESULT WINAPI MediaSeekingPassThru_GetPositions(IMediaSeeking * iface, LONGLONG * pCurrent, LONGLONG * pStop)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p, %p)\n", iface, This, pCurrent, pStop);

    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeekingPassThru_GetAvailable(IMediaSeeking * iface, LONGLONG * pEarliest, LONGLONG * pLatest)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p,%p)\n", iface, This, pEarliest, pLatest);

    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeekingPassThru_SetRate(IMediaSeeking * iface, double dRate)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%e)\n", iface, This, dRate);

    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeekingPassThru_GetRate(IMediaSeeking * iface, double * dRate)
{
    ICOM_THIS_MULTI(PassThruImpl, IMediaSeeking_vtbl, iface);

    TRACE("(%p/%p)->(%p)\n", iface, This, dRate);

    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaSeekingPassThru_GetPreroll(IMediaSeeking * iface, LONGLONG * pPreroll)
{
    TRACE("(%p)\n", pPreroll);

    FIXME("stub\n");
    return E_NOTIMPL;
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
