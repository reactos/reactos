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

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static struct strmbase_passthrough *impl_from_ISeekingPassThru(ISeekingPassThru *iface)
{
    return CONTAINING_RECORD(iface, struct strmbase_passthrough, ISeekingPassThru_iface);
}

static struct strmbase_passthrough *impl_from_IMediaSeeking(IMediaSeeking *iface)
{
    return CONTAINING_RECORD(iface, struct strmbase_passthrough, IMediaSeeking_iface);
}

static struct strmbase_passthrough *impl_from_IMediaPosition(IMediaPosition *iface)
{
    return CONTAINING_RECORD(iface, struct strmbase_passthrough, IMediaPosition_iface);
}

static HRESULT WINAPI SeekingPassThru_QueryInterface(ISeekingPassThru *iface, REFIID iid, void **out)
{
    struct strmbase_passthrough *passthrough = impl_from_ISeekingPassThru(iface);

    return IUnknown_QueryInterface(passthrough->outer_unk, iid, out);
}

static ULONG WINAPI SeekingPassThru_AddRef(ISeekingPassThru *iface)
{
    struct strmbase_passthrough *passthrough = impl_from_ISeekingPassThru(iface);

    return IUnknown_AddRef(passthrough->outer_unk);
}

static ULONG WINAPI SeekingPassThru_Release(ISeekingPassThru *iface)
{
    struct strmbase_passthrough *passthrough = impl_from_ISeekingPassThru(iface);

    return IUnknown_Release(passthrough->outer_unk);
}

static HRESULT WINAPI SeekingPassThru_Init(ISeekingPassThru *iface, BOOL renderer, IPin *pin)
{
    struct strmbase_passthrough *This = impl_from_ISeekingPassThru(iface);

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

static HRESULT WINAPI MediaSeekingPassThru_QueryInterface(IMediaSeeking *iface, REFIID iid, void **out)
{
    struct strmbase_passthrough *passthrough = impl_from_IMediaSeeking(iface);

    return IUnknown_QueryInterface(passthrough->outer_unk, iid, out);
}

static ULONG WINAPI MediaSeekingPassThru_AddRef(IMediaSeeking *iface)
{
    struct strmbase_passthrough *passthrough = impl_from_IMediaSeeking(iface);

    return IUnknown_AddRef(passthrough->outer_unk);
}

static ULONG WINAPI MediaSeekingPassThru_Release(IMediaSeeking *iface)
{
    struct strmbase_passthrough *passthrough = impl_from_IMediaSeeking(iface);

    return IUnknown_Release(passthrough->outer_unk);
}

static HRESULT get_connected(struct strmbase_passthrough *This, REFIID riid, void **ppvObj)
{
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
    struct strmbase_passthrough *This = impl_from_IMediaSeeking(iface);
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
    struct strmbase_passthrough *This = impl_from_IMediaSeeking(iface);
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
    struct strmbase_passthrough *This = impl_from_IMediaSeeking(iface);
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
    struct strmbase_passthrough *This = impl_from_IMediaSeeking(iface);
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
    struct strmbase_passthrough *This = impl_from_IMediaSeeking(iface);
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
    struct strmbase_passthrough *This = impl_from_IMediaSeeking(iface);
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
    struct strmbase_passthrough *This = impl_from_IMediaSeeking(iface);
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
    struct strmbase_passthrough *This = impl_from_IMediaSeeking(iface);
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
    struct strmbase_passthrough *This = impl_from_IMediaSeeking(iface);
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
    struct strmbase_passthrough *This = impl_from_IMediaSeeking(iface);
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
    struct strmbase_passthrough *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;

    TRACE("iface %p, target %p, target_format %s, source %s, source_format %s.\n",
            iface, pTarget, debugstr_guid(pTargetFormat),
            wine_dbgstr_longlong(Source), debugstr_guid(pSourceFormat));

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
    struct strmbase_passthrough *This = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seek;
    HRESULT hr;

    TRACE("iface %p, current %p, current_flags %#lx, stop %p, stop_flags %#lx.\n",
            iface, pCurrent, dwCurrentFlags, pStop, dwStopFlags);

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
    struct strmbase_passthrough *This = impl_from_IMediaSeeking(iface);
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
    struct strmbase_passthrough *This = impl_from_IMediaSeeking(iface);
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
    struct strmbase_passthrough *This = impl_from_IMediaSeeking(iface);
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
    struct strmbase_passthrough *This = impl_from_IMediaSeeking(iface);
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
    struct strmbase_passthrough *This = impl_from_IMediaSeeking(iface);
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

static HRESULT WINAPI MediaPositionPassThru_QueryInterface(IMediaPosition *iface, REFIID iid, void **out)
{
    struct strmbase_passthrough *passthrough = impl_from_IMediaPosition(iface);

    return IUnknown_QueryInterface(passthrough->outer_unk, iid, out);
}

static ULONG WINAPI MediaPositionPassThru_AddRef(IMediaPosition *iface)
{
    struct strmbase_passthrough *passthrough = impl_from_IMediaPosition(iface);

    return IUnknown_AddRef(passthrough->outer_unk);
}

static ULONG WINAPI MediaPositionPassThru_Release(IMediaPosition *iface)
{
    struct strmbase_passthrough *passthrough = impl_from_IMediaPosition(iface);

    return IUnknown_Release(passthrough->outer_unk);
}

static HRESULT WINAPI MediaPositionPassThru_GetTypeInfoCount(IMediaPosition *iface, UINT *count)
{
    TRACE("iface %p, count %p.\n", iface, count);
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI MediaPositionPassThru_GetTypeInfo(IMediaPosition *iface, UINT index,
        LCID lcid, ITypeInfo **typeinfo)
{
    TRACE("iface %p, index %u, lcid %#lx, typeinfo %p.\n", iface, index, lcid, typeinfo);
    return strmbase_get_typeinfo(IMediaPosition_tid, typeinfo);
}

static HRESULT WINAPI MediaPositionPassThru_GetIDsOfNames(IMediaPosition *iface, REFIID iid,
        LPOLESTR *names, UINT count, LCID lcid, DISPID *ids)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("iface %p, iid %s, names %p, count %u, lcid %#lx, ids %p.\n",
            iface, debugstr_guid(iid), names, count, lcid, ids);

    if (SUCCEEDED(hr = strmbase_get_typeinfo(IMediaPosition_tid, &typeinfo)))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, names, count, ids);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

static HRESULT WINAPI MediaPositionPassThru_Invoke(IMediaPosition *iface, DISPID id, REFIID iid, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *error_arg)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("iface %p, id %ld, iid %s, lcid %#lx, flags %#x, params %p, result %p, excepinfo %p, error_arg %p.\n",
            iface, id, debugstr_guid(iid), lcid, flags, params, result, excepinfo, error_arg);

    if (SUCCEEDED(hr = strmbase_get_typeinfo(IMediaPosition_tid, &typeinfo)))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, id, flags, params, result, excepinfo, error_arg);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

static HRESULT WINAPI MediaPositionPassThru_get_Duration(IMediaPosition *iface, REFTIME *plength)
{
    struct strmbase_passthrough *This = impl_from_IMediaPosition(iface);
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
    struct strmbase_passthrough *This = impl_from_IMediaPosition(iface);
    IMediaPosition *pos;
    HRESULT hr;

    TRACE("iface %p, time %.16e.\n", iface, llTime);

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
    struct strmbase_passthrough *This = impl_from_IMediaPosition(iface);
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
    struct strmbase_passthrough *This = impl_from_IMediaPosition(iface);
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
    struct strmbase_passthrough *This = impl_from_IMediaPosition(iface);
    IMediaPosition *pos;
    HRESULT hr;

    TRACE("iface %p, time %.16e.\n", iface, llTime);

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
    struct strmbase_passthrough *This = impl_from_IMediaPosition(iface);
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
    struct strmbase_passthrough *This = impl_from_IMediaPosition(iface);
    IMediaPosition *pos;
    HRESULT hr;

    TRACE("iface %p, time %.16e.\n", iface, llTime);

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
    struct strmbase_passthrough *This = impl_from_IMediaPosition(iface);
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
    struct strmbase_passthrough *This = impl_from_IMediaPosition(iface);
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
    struct strmbase_passthrough *This = impl_from_IMediaPosition(iface);
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
    struct strmbase_passthrough *This = impl_from_IMediaPosition(iface);
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

void strmbase_passthrough_init(struct strmbase_passthrough *passthrough, IUnknown *outer)
{
    memset(passthrough, 0, sizeof(*passthrough));

    passthrough->outer_unk = outer;
    passthrough->IMediaPosition_iface.lpVtbl = &IMediaPositionPassThru_Vtbl;
    passthrough->IMediaSeeking_iface.lpVtbl = &IMediaSeekingPassThru_Vtbl;
    passthrough->ISeekingPassThru_iface.lpVtbl = &ISeekingPassThru_Vtbl;
    if (!InitializeCriticalSectionEx(&passthrough->time_cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO))
        InitializeCriticalSection(&passthrough->time_cs);
    passthrough->time_cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": strmbase_passthrough.time_cs" );
}

void strmbase_passthrough_cleanup(struct strmbase_passthrough *passthrough)
{
    passthrough->time_cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&passthrough->time_cs);
}

void strmbase_passthrough_update_time(struct strmbase_passthrough *passthrough, REFERENCE_TIME time)
{
    EnterCriticalSection(&passthrough->time_cs);
    passthrough->time_earliest = time;
    passthrough->timevalid = TRUE;
    LeaveCriticalSection(&passthrough->time_cs);
}

void strmbase_passthrough_invalidate_time(struct strmbase_passthrough *passthrough)
{
    EnterCriticalSection(&passthrough->time_cs);
    passthrough->timevalid = FALSE;
    LeaveCriticalSection(&passthrough->time_cs);
}

void strmbase_passthrough_eos(struct strmbase_passthrough *passthrough)
{
    REFERENCE_TIME time;
    HRESULT hr;

    hr = IMediaSeeking_GetStopPosition(&passthrough->IMediaSeeking_iface, &time);
    EnterCriticalSection(&passthrough->time_cs);
    if (SUCCEEDED(hr))
    {
        passthrough->timevalid = TRUE;
        passthrough->time_earliest = time;
    }
    else
        passthrough->timevalid = FALSE;
    LeaveCriticalSection(&passthrough->time_cs);
}
