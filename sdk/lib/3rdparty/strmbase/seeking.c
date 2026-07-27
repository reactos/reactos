/*
 * Filter Seeking and Control Interfaces
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

static inline SourceSeeking *impl_from_IMediaSeeking(IMediaSeeking *iface)
{
    return CONTAINING_RECORD(iface, SourceSeeking, IMediaSeeking_iface);
}

HRESULT strmbase_seeking_init(SourceSeeking *pSeeking, const IMediaSeekingVtbl *Vtbl,
        SourceSeeking_ChangeStop fnChangeStop, SourceSeeking_ChangeStart fnChangeStart,
        SourceSeeking_ChangeRate fnChangeRate)
{
    assert(fnChangeStop && fnChangeStart && fnChangeRate);

    pSeeking->IMediaSeeking_iface.lpVtbl = Vtbl;
    pSeeking->refCount = 1;
    pSeeking->fnChangeRate = fnChangeRate;
    pSeeking->fnChangeStop = fnChangeStop;
    pSeeking->fnChangeStart = fnChangeStart;
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
    if (!InitializeCriticalSectionEx(&pSeeking->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO))
        InitializeCriticalSection(&pSeeking->cs);
    pSeeking->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": SourceSeeking.cs");
    return S_OK;
}

void strmbase_seeking_cleanup(SourceSeeking *seeking)
{
    seeking->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&seeking->cs);
}

HRESULT WINAPI SourceSeekingImpl_GetCapabilities(IMediaSeeking * iface, DWORD * pCapabilities)
{
    SourceSeeking *This = impl_from_IMediaSeeking(iface);

    TRACE("(%p)\n", pCapabilities);

    *pCapabilities = This->dwCapabilities;

    return S_OK;
}

HRESULT WINAPI SourceSeekingImpl_CheckCapabilities(IMediaSeeking * iface, DWORD * pCapabilities)
{
    SourceSeeking *This = impl_from_IMediaSeeking(iface);
    HRESULT hr;
    DWORD dwCommonCaps;

    TRACE("(%p)\n", pCapabilities);

    if (!pCapabilities)
        return E_POINTER;

    dwCommonCaps = *pCapabilities & This->dwCapabilities;

    if (!dwCommonCaps)
        hr = E_FAIL;
    else
        hr = (*pCapabilities == dwCommonCaps) ?  S_OK : S_FALSE;
    *pCapabilities = dwCommonCaps;
    return hr;
}

HRESULT WINAPI SourceSeekingImpl_IsFormatSupported(IMediaSeeking * iface, const GUID * pFormat)
{
    TRACE("(%s)\n", debugstr_guid(pFormat));

    return (IsEqualIID(pFormat, &TIME_FORMAT_MEDIA_TIME) ? S_OK : S_FALSE);
}

HRESULT WINAPI SourceSeekingImpl_QueryPreferredFormat(IMediaSeeking * iface, GUID * pFormat)
{
    TRACE("(%s)\n", debugstr_guid(pFormat));

    *pFormat = TIME_FORMAT_MEDIA_TIME;
    return S_OK;
}

HRESULT WINAPI SourceSeekingImpl_GetTimeFormat(IMediaSeeking * iface, GUID * pFormat)
{
    SourceSeeking *This = impl_from_IMediaSeeking(iface);
    TRACE("(%s)\n", debugstr_guid(pFormat));

    EnterCriticalSection(&This->cs);
    *pFormat = This->timeformat;
    LeaveCriticalSection(&This->cs);

    return S_OK;
}

HRESULT WINAPI SourceSeekingImpl_IsUsingTimeFormat(IMediaSeeking * iface, const GUID * pFormat)
{
    SourceSeeking *This = impl_from_IMediaSeeking(iface);
    HRESULT hr = S_OK;

    TRACE("(%s)\n", debugstr_guid(pFormat));

    EnterCriticalSection(&This->cs);
    if (!IsEqualIID(pFormat, &This->timeformat))
        hr = S_FALSE;
    LeaveCriticalSection(&This->cs);

    return hr;
}

HRESULT WINAPI SourceSeekingImpl_SetTimeFormat(IMediaSeeking * iface, const GUID * pFormat)
{
    SourceSeeking *This = impl_from_IMediaSeeking(iface);
    TRACE("%p %s\n", This, debugstr_guid(pFormat));
    return (IsEqualIID(pFormat, &TIME_FORMAT_MEDIA_TIME) ? S_OK : E_INVALIDARG);
}


HRESULT WINAPI SourceSeekingImpl_GetDuration(IMediaSeeking * iface, LONGLONG * pDuration)
{
    SourceSeeking *This = impl_from_IMediaSeeking(iface);

    TRACE("(%p)\n", pDuration);

    EnterCriticalSection(&This->cs);
    *pDuration = This->llDuration;
    LeaveCriticalSection(&This->cs);

    return S_OK;
}

HRESULT WINAPI SourceSeekingImpl_GetStopPosition(IMediaSeeking * iface, LONGLONG * pStop)
{
    SourceSeeking *This = impl_from_IMediaSeeking(iface);

    TRACE("(%p)\n", pStop);

    EnterCriticalSection(&This->cs);
    *pStop = This->llStop;
    LeaveCriticalSection(&This->cs);

    return S_OK;
}

/* FIXME: Make use of the info the filter should expose */
HRESULT WINAPI SourceSeekingImpl_GetCurrentPosition(IMediaSeeking * iface, LONGLONG * pCurrent)
{
    SourceSeeking *This = impl_from_IMediaSeeking(iface);

    TRACE("(%p)\n", pCurrent);

    EnterCriticalSection(&This->cs);
    *pCurrent = This->llCurrent;
    LeaveCriticalSection(&This->cs);

    return S_OK;
}

HRESULT WINAPI SourceSeekingImpl_ConvertTimeFormat(IMediaSeeking * iface, LONGLONG * pTarget, const GUID * pTargetFormat, LONGLONG Source, const GUID * pSourceFormat)
{
    SourceSeeking *This = impl_from_IMediaSeeking(iface);
    if (!pTargetFormat)
        pTargetFormat = &This->timeformat;
    if (!pSourceFormat)
        pSourceFormat = &This->timeformat;
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

HRESULT WINAPI SourceSeekingImpl_SetPositions(IMediaSeeking * iface, LONGLONG * pCurrent, DWORD dwCurrentFlags, LONGLONG * pStop, DWORD dwStopFlags)
{
    SourceSeeking *This = impl_from_IMediaSeeking(iface);
    BOOL bChangeCurrent = FALSE, bChangeStop = FALSE;
    LONGLONG llNewCurrent, llNewStop;

    TRACE("iface %p, current %s, current_flags %#lx, stop %s, stop_flags %#lx.\n", iface,
            pCurrent ? debugstr_time(*pCurrent) : "<null>", dwCurrentFlags,
            pStop ? debugstr_time(*pStop): "<null>", dwStopFlags);

    EnterCriticalSection(&This->cs);

    llNewCurrent = Adjust(This->llCurrent, pCurrent, dwCurrentFlags);
    llNewStop = Adjust(This->llStop, pStop, dwStopFlags);

    if (pCurrent)
        bChangeCurrent = TRUE;
    if (llNewStop != This->llStop)
        bChangeStop = TRUE;

    TRACE("Seeking from %s to %s.\n", debugstr_time(This->llCurrent), debugstr_time(llNewCurrent));

    This->llCurrent = llNewCurrent;
    This->llStop = llNewStop;

    if (pCurrent && (dwCurrentFlags & AM_SEEKING_ReturnTime))
        *pCurrent = llNewCurrent;
    if (pStop && (dwStopFlags & AM_SEEKING_ReturnTime))
        *pStop = llNewStop;
    LeaveCriticalSection(&This->cs);

    if (bChangeCurrent)
        This->fnChangeStart(iface);
    if (bChangeStop)
        This->fnChangeStop(iface);

    return S_OK;
}

HRESULT WINAPI SourceSeekingImpl_GetPositions(IMediaSeeking * iface, LONGLONG * pCurrent, LONGLONG * pStop)
{
    SourceSeeking *This = impl_from_IMediaSeeking(iface);

    TRACE("(%p, %p)\n", pCurrent, pStop);

    EnterCriticalSection(&This->cs);
    IMediaSeeking_GetCurrentPosition(iface, pCurrent);
    IMediaSeeking_GetStopPosition(iface, pStop);
    LeaveCriticalSection(&This->cs);

    return S_OK;
}

HRESULT WINAPI SourceSeekingImpl_GetAvailable(IMediaSeeking * iface, LONGLONG * pEarliest, LONGLONG * pLatest)
{
    SourceSeeking *This = impl_from_IMediaSeeking(iface);

    TRACE("(%p, %p)\n", pEarliest, pLatest);

    EnterCriticalSection(&This->cs);
    *pEarliest = 0;
    *pLatest = This->llDuration;
    LeaveCriticalSection(&This->cs);

    return S_OK;
}

HRESULT WINAPI SourceSeekingImpl_SetRate(IMediaSeeking * iface, double dRate)
{
    SourceSeeking *This = impl_from_IMediaSeeking(iface);
    BOOL bChangeRate = (dRate != This->dRate);
    HRESULT hr = S_OK;

    TRACE("(%e)\n", dRate);

    if (dRate > 100 || dRate < .001)
    {
        FIXME("Excessive rate %e, ignoring\n", dRate);
        return VFW_E_UNSUPPORTED_AUDIO;
    }

    EnterCriticalSection(&This->cs);
    This->dRate = dRate;
    if (bChangeRate)
        hr = This->fnChangeRate(iface);
    LeaveCriticalSection(&This->cs);

    return hr;
}

HRESULT WINAPI SourceSeekingImpl_GetRate(IMediaSeeking * iface, double * dRate)
{
    SourceSeeking *This = impl_from_IMediaSeeking(iface);

    TRACE("(%p)\n", dRate);

    EnterCriticalSection(&This->cs);
    /* Forward? */
    *dRate = This->dRate;
    LeaveCriticalSection(&This->cs);

    return S_OK;
}

HRESULT WINAPI SourceSeekingImpl_GetPreroll(IMediaSeeking * iface, LONGLONG * pPreroll)
{
    TRACE("(%p)\n", pPreroll);

    *pPreroll = 0;
    return S_OK;
}
