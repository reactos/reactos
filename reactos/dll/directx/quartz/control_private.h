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

#ifndef QUARTZ_CONTROL_H
#define QUARTZ_CONTROL_H

typedef HRESULT (* CHANGEPROC)(IBaseFilter *pUserData);

typedef struct MediaSeekingImpl
{
	const IMediaSeekingVtbl * lpVtbl;

	ULONG refCount;
	IBaseFilter *pUserData;
	CHANGEPROC fnChangeStop;
	CHANGEPROC fnChangeCurrent;
	CHANGEPROC fnChangeRate;
	DWORD dwCapabilities;
	double dRate;
	LONGLONG llCurrent, llStop, llDuration;
	GUID timeformat;
	PCRITICAL_SECTION crst;
} MediaSeekingImpl;

HRESULT MediaSeekingImpl_Init(IBaseFilter *pUserData, CHANGEPROC fnChangeStop, CHANGEPROC fnChangeCurrent, CHANGEPROC fnChangeRate, MediaSeekingImpl * pSeeking, PCRITICAL_SECTION crit_sect);

HRESULT WINAPI MediaSeekingImpl_GetCapabilities(IMediaSeeking * iface, DWORD * pCapabilities);
HRESULT WINAPI MediaSeekingImpl_CheckCapabilities(IMediaSeeking * iface, DWORD * pCapabilities);
HRESULT WINAPI MediaSeekingImpl_IsFormatSupported(IMediaSeeking * iface, const GUID * pFormat);
HRESULT WINAPI MediaSeekingImpl_QueryPreferredFormat(IMediaSeeking * iface, GUID * pFormat);
HRESULT WINAPI MediaSeekingImpl_GetTimeFormat(IMediaSeeking * iface, GUID * pFormat);
HRESULT WINAPI MediaSeekingImpl_IsUsingTimeFormat(IMediaSeeking * iface, const GUID * pFormat);
HRESULT WINAPI MediaSeekingImpl_SetTimeFormat(IMediaSeeking * iface, const GUID * pFormat);
HRESULT WINAPI MediaSeekingImpl_GetDuration(IMediaSeeking * iface, LONGLONG * pDuration);
HRESULT WINAPI MediaSeekingImpl_GetStopPosition(IMediaSeeking * iface, LONGLONG * pStop);
HRESULT WINAPI MediaSeekingImpl_GetCurrentPosition(IMediaSeeking * iface, LONGLONG * pCurrent);
HRESULT WINAPI MediaSeekingImpl_ConvertTimeFormat(IMediaSeeking * iface, LONGLONG * pTarget, const GUID * pTargetFormat, LONGLONG Source, const GUID * pSourceFormat);
HRESULT WINAPI MediaSeekingImpl_SetPositions(IMediaSeeking * iface, LONGLONG * pCurrent, DWORD dwCurrentFlags, LONGLONG * pStop, DWORD dwStopFlags);
HRESULT WINAPI MediaSeekingImpl_GetPositions(IMediaSeeking * iface, LONGLONG * pCurrent, LONGLONG * pStop);
HRESULT WINAPI MediaSeekingImpl_GetAvailable(IMediaSeeking * iface, LONGLONG * pEarliest, LONGLONG * pLatest);
HRESULT WINAPI MediaSeekingImpl_SetRate(IMediaSeeking * iface, double dRate);
HRESULT WINAPI MediaSeekingImpl_GetRate(IMediaSeeking * iface, double * dRate);
HRESULT WINAPI MediaSeekingImpl_GetPreroll(IMediaSeeking * iface, LONGLONG * pPreroll);

#endif /*QUARTZ_CONTROL_H*/
