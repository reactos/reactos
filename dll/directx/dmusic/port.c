/* IDirectMusicPort Implementation
 *
 * Copyright (C) 2003-2004 Rok Mandeljc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "dmusic_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dmusic);

/* IDirectMusicPortImpl IUnknown part: */
static HRESULT WINAPI IDirectMusicPortImpl_QueryInterface (LPDIRECTMUSICPORT iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicPortImpl, lpVtbl, iface);

	TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);

	if (IsEqualIID (riid, &IID_IUnknown) ||
	    IsEqualGUID(riid, &IID_IDirectMusicPort) ||
	    IsEqualGUID(riid, &IID_IDirectMusicPort8)) {
		*ppobj = &This->lpVtbl;
		IDirectMusicPort_AddRef((LPDIRECTMUSICPORT)*ppobj);
		return S_OK;
	} else if (IsEqualGUID(riid, &IID_IDirectMusicPortDownload) ||
		   IsEqualGUID(riid, &IID_IDirectMusicPortDownload8)) {
		*ppobj = &This->lpDownloadVtbl;
		IDirectMusicPortDownload_AddRef((LPDIRECTMUSICPORTDOWNLOAD)*ppobj);
		return S_OK;
	} else if (IsEqualGUID(riid, &IID_IDirectMusicThru) ||
		   IsEqualGUID(riid, &IID_IDirectMusicThru8)) {
		*ppobj = &This->lpThruVtbl;
		IDirectMusicThru_AddRef((LPDIRECTMUSICTHRU)*ppobj);
		return S_OK;
	}
	WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicPortImpl_AddRef (LPDIRECTMUSICPORT iface) {
	IDirectMusicPortImpl *This = (IDirectMusicPortImpl *)iface;
	ULONG refCount = InterlockedIncrement(&This->ref);

	TRACE("(%p)->(ref before=%u)\n", This, refCount - 1);

	DMUSIC_LockModule();

	return refCount;
}

static ULONG WINAPI IDirectMusicPortImpl_Release (LPDIRECTMUSICPORT iface) {
	IDirectMusicPortImpl *This = (IDirectMusicPortImpl *)iface;
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(ref before=%u)\n", This, refCount + 1);

	if (!refCount) {
		HeapFree(GetProcessHeap(), 0, This);
	}

	DMUSIC_UnlockModule();

	return refCount;
}

/* IDirectMusicPortImpl IDirectMusicPort part: */
static HRESULT WINAPI IDirectMusicPortImpl_PlayBuffer (LPDIRECTMUSICPORT iface, LPDIRECTMUSICBUFFER pBuffer) {
	IDirectMusicPortImpl *This = (IDirectMusicPortImpl *)iface;
	FIXME("(%p, %p): stub\n", This, pBuffer);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicPortImpl_SetReadNotificationHandle (LPDIRECTMUSICPORT iface, HANDLE hEvent) {
	IDirectMusicPortImpl *This = (IDirectMusicPortImpl *)iface;
	FIXME("(%p, %p): stub\n", This, hEvent);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicPortImpl_Read (LPDIRECTMUSICPORT iface, LPDIRECTMUSICBUFFER pBuffer) {
	IDirectMusicPortImpl *This = (IDirectMusicPortImpl *)iface;
	FIXME("(%p, %p): stub\n", This, pBuffer);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicPortImpl_DownloadInstrument (LPDIRECTMUSICPORT iface, IDirectMusicInstrument* pInstrument, IDirectMusicDownloadedInstrument** ppDownloadedInstrument, DMUS_NOTERANGE* pNoteRanges, DWORD dwNumNoteRanges) {
	IDirectMusicPortImpl *This = (IDirectMusicPortImpl *)iface;

	FIXME("(%p, %p, %p, %p, %d): stub\n", This, pInstrument, ppDownloadedInstrument, pNoteRanges, dwNumNoteRanges);

	if (!pInstrument || !ppDownloadedInstrument || (dwNumNoteRanges && !pNoteRanges))
		return E_POINTER;

	return DMUSIC_CreateDirectMusicDownloadedInstrumentImpl(&IID_IDirectMusicDownloadedInstrument, (LPVOID*)ppDownloadedInstrument, NULL);
}

static HRESULT WINAPI IDirectMusicPortImpl_UnloadInstrument (LPDIRECTMUSICPORT iface, IDirectMusicDownloadedInstrument *pDownloadedInstrument) {
	IDirectMusicPortImpl *This = (IDirectMusicPortImpl *)iface;
	FIXME("(%p, %p): stub\n", This, pDownloadedInstrument);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicPortImpl_GetLatencyClock (LPDIRECTMUSICPORT iface, IReferenceClock** ppClock) {
	IDirectMusicPortImpl *This = (IDirectMusicPortImpl *)iface;
	TRACE("(%p, %p)\n", This, ppClock);
	*ppClock = This->pLatencyClock;
	IReferenceClock_AddRef (*ppClock);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicPortImpl_GetRunningStats (LPDIRECTMUSICPORT iface, LPDMUS_SYNTHSTATS pStats) {
	IDirectMusicPortImpl *This = (IDirectMusicPortImpl *)iface;
	FIXME("(%p, %p): stub\n", This, pStats);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicPortImpl_Compact (LPDIRECTMUSICPORT iface) {
	IDirectMusicPortImpl *This = (IDirectMusicPortImpl *)iface;
	FIXME("(%p): stub\n", This);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicPortImpl_GetCaps (LPDIRECTMUSICPORT iface, LPDMUS_PORTCAPS pPortCaps) {
	IDirectMusicPortImpl *This = (IDirectMusicPortImpl *)iface;
	TRACE("(%p, %p)\n", This, pPortCaps);
	*pPortCaps = This->caps;
	return S_OK;
}

static HRESULT WINAPI IDirectMusicPortImpl_DeviceIoControl (LPDIRECTMUSICPORT iface, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped) {
	IDirectMusicPortImpl *This = (IDirectMusicPortImpl *)iface;
	FIXME("(%p, %d, %p, %d, %p, %d, %p, %p): stub\n", This, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicPortImpl_SetNumChannelGroups (LPDIRECTMUSICPORT iface, DWORD dwChannelGroups) {
	IDirectMusicPortImpl *This = (IDirectMusicPortImpl *)iface;
	FIXME("(%p, %d): semi-stub\n", This, dwChannelGroups);
	This->nrofgroups = dwChannelGroups;
	return S_OK;
}

static HRESULT WINAPI IDirectMusicPortImpl_GetNumChannelGroups (LPDIRECTMUSICPORT iface, LPDWORD pdwChannelGroups) {
	IDirectMusicPortImpl *This = (IDirectMusicPortImpl *)iface;
	TRACE("(%p, %p)\n", This, pdwChannelGroups);
	*pdwChannelGroups = This->nrofgroups;
	return S_OK;
}

HRESULT WINAPI IDirectMusicPortImpl_Activate (LPDIRECTMUSICPORT iface, BOOL fActive) {
	IDirectMusicPortImpl *This = (IDirectMusicPortImpl *)iface;
	TRACE("(%p, %d)\n", This, fActive);
	This->fActive = fActive;
	return S_OK;
}

static HRESULT WINAPI IDirectMusicPortImpl_SetChannelPriority (LPDIRECTMUSICPORT iface, DWORD dwChannelGroup, DWORD dwChannel, DWORD dwPriority) {
	IDirectMusicPortImpl *This = (IDirectMusicPortImpl *)iface;
	FIXME("(%p, %d, %d, %d): semi-stub\n", This, dwChannelGroup, dwChannel, dwPriority);
	if (dwChannel > 16) {
		WARN("isn't there supposed to be 16 channels (no. %d requested)?! (faking as it is ok)\n", dwChannel);
		/*return E_INVALIDARG;*/
	}	
	return S_OK;
}

static HRESULT WINAPI IDirectMusicPortImpl_GetChannelPriority (LPDIRECTMUSICPORT iface, DWORD dwChannelGroup, DWORD dwChannel, LPDWORD pdwPriority) {
	IDirectMusicPortImpl *This = (IDirectMusicPortImpl *)iface;
	TRACE("(%p, %d, %d, %p)\n", This, dwChannelGroup, dwChannel, pdwPriority);
	*pdwPriority = This->group[dwChannelGroup-1].channel[dwChannel].priority;
	return S_OK;
}

static HRESULT WINAPI IDirectMusicPortImpl_SetDirectSound (LPDIRECTMUSICPORT iface, LPDIRECTSOUND pDirectSound, LPDIRECTSOUNDBUFFER pDirectSoundBuffer) {
	IDirectMusicPortImpl *This = (IDirectMusicPortImpl *)iface;
	FIXME("(%p, %p, %p): stub\n", This, pDirectSound, pDirectSoundBuffer);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicPortImpl_GetFormat (LPDIRECTMUSICPORT iface, LPWAVEFORMATEX pWaveFormatEx, LPDWORD pdwWaveFormatExSize, LPDWORD pdwBufferSize) {
	IDirectMusicPortImpl *This = (IDirectMusicPortImpl *)iface;
	WAVEFORMATEX format;
	FIXME("(%p, %p, %p, %p): stub\n", This, pWaveFormatEx, pdwWaveFormatExSize, pdwBufferSize);

	if (pWaveFormatEx == NULL)
	{
		if (pdwWaveFormatExSize)
			*pdwWaveFormatExSize = sizeof(format);
		else
			return E_POINTER;
	}
	else
	{
		if (pdwWaveFormatExSize == NULL)
			return E_POINTER;

		/* Just fill this in with something that will not crash Direct Sound for now. */
		/* It won't be used anyway until Performances are completed */
		format.wFormatTag = WAVE_FORMAT_PCM;
		format.nChannels = 2; /* This->params.dwAudioChannels; */
		format.nSamplesPerSec = 44100; /* This->params.dwSampleRate; */
		format.wBitsPerSample = 16;	/* FIXME: check this */
		format.nBlockAlign = (format.wBitsPerSample * format.nChannels) / 8;
		format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
		format.cbSize = 0;

		if (*pdwWaveFormatExSize >= sizeof(format))
		{
			CopyMemory(pWaveFormatEx, &format, min(sizeof(format), *pdwWaveFormatExSize));
			*pdwWaveFormatExSize = sizeof(format);	/* FIXME check if this is set */
		}
		else
			return E_POINTER;	/* FIXME find right error */
	}

	if (pdwBufferSize)
		*pdwBufferSize = 44100 * 2 * 2;
	else
		return E_POINTER;

	return S_OK;
}

static const IDirectMusicPortVtbl DirectMusicPort_Vtbl = {
	IDirectMusicPortImpl_QueryInterface,
	IDirectMusicPortImpl_AddRef,
	IDirectMusicPortImpl_Release,
	IDirectMusicPortImpl_PlayBuffer,
	IDirectMusicPortImpl_SetReadNotificationHandle,
	IDirectMusicPortImpl_Read,
	IDirectMusicPortImpl_DownloadInstrument,
	IDirectMusicPortImpl_UnloadInstrument,
	IDirectMusicPortImpl_GetLatencyClock,
	IDirectMusicPortImpl_GetRunningStats,
	IDirectMusicPortImpl_Compact,
	IDirectMusicPortImpl_GetCaps,
	IDirectMusicPortImpl_DeviceIoControl,
	IDirectMusicPortImpl_SetNumChannelGroups,
	IDirectMusicPortImpl_GetNumChannelGroups,
	IDirectMusicPortImpl_Activate,
	IDirectMusicPortImpl_SetChannelPriority,
	IDirectMusicPortImpl_GetChannelPriority,
	IDirectMusicPortImpl_SetDirectSound,
	IDirectMusicPortImpl_GetFormat
};

/* IDirectMusicPortDownload IUnknown parts follow: */
static HRESULT WINAPI IDirectMusicPortDownloadImpl_QueryInterface (LPDIRECTMUSICPORTDOWNLOAD iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicPortImpl, lpDownloadVtbl, iface);
	TRACE("(%p/%p)->(%s, %p)\n", This, iface, debugstr_dmguid(riid), ppobj);
	return IUnknown_QueryInterface((IUnknown *)&(This->lpVtbl), riid, ppobj);
}

static ULONG WINAPI IDirectMusicPortDownloadImpl_AddRef (LPDIRECTMUSICPORTDOWNLOAD iface) {
	ICOM_THIS_MULTI(IDirectMusicPortImpl, lpDownloadVtbl, iface);
	TRACE("(%p/%p)->()\n", This, iface);
	return IUnknown_AddRef((IUnknown *)&(This->lpVtbl));
}

static ULONG WINAPI IDirectMusicPortDownloadImpl_Release (LPDIRECTMUSICPORTDOWNLOAD iface) {
	ICOM_THIS_MULTI(IDirectMusicPortImpl, lpDownloadVtbl, iface);
	TRACE("(%p/%p)->()\n", This, iface);
	return IUnknown_Release((IUnknown *)&(This->lpVtbl));
}

/* IDirectMusicPortDownload Interface follow: */
static HRESULT WINAPI IDirectMusicPortDownloadImpl_GetBuffer (LPDIRECTMUSICPORTDOWNLOAD iface, DWORD dwDLId, IDirectMusicDownload** ppIDMDownload) {
	ICOM_THIS_MULTI(IDirectMusicPortImpl, lpDownloadVtbl, iface);

	FIXME("(%p/%p)->(%d, %p): stub\n", This, iface, dwDLId, ppIDMDownload);

	if (!ppIDMDownload)
		return E_POINTER;

	return DMUSIC_CreateDirectMusicDownloadImpl(&IID_IDirectMusicDownload, (LPVOID*)ppIDMDownload, NULL);
}

static HRESULT WINAPI IDirectMusicPortDownloadImpl_AllocateBuffer (LPDIRECTMUSICPORTDOWNLOAD iface, DWORD dwSize, IDirectMusicDownload** ppIDMDownload) {
	ICOM_THIS_MULTI(IDirectMusicPortImpl, lpDownloadVtbl, iface);
	FIXME("(%p/%p)->(%d, %p): stub\n", This, iface, dwSize, ppIDMDownload);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicPortDownloadImpl_GetDLId (LPDIRECTMUSICPORTDOWNLOAD iface, DWORD* pdwStartDLId, DWORD dwCount) {
	ICOM_THIS_MULTI(IDirectMusicPortImpl, lpDownloadVtbl, iface);
	FIXME("(%p/%p)->(%p, %d): stub\n", This, iface, pdwStartDLId, dwCount);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicPortDownloadImpl_GetAppend (LPDIRECTMUSICPORTDOWNLOAD iface, DWORD* pdwAppend) {
	IDirectMusicPortImpl *This = (IDirectMusicPortImpl *)iface;
	FIXME("(%p/%p)->(%p): stub\n", This, iface, pdwAppend);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicPortDownloadImpl_Download (LPDIRECTMUSICPORTDOWNLOAD iface, IDirectMusicDownload* pIDMDownload) {
	IDirectMusicPortImpl *This = (IDirectMusicPortImpl *)iface;
	FIXME("(%p/%p)->(%p): stub\n", This, iface, pIDMDownload);
	return S_OK;
}

static HRESULT WINAPI IDirectMusicPortDownloadImpl_Unload (LPDIRECTMUSICPORTDOWNLOAD iface, IDirectMusicDownload* pIDMDownload) {
	IDirectMusicPortImpl *This = (IDirectMusicPortImpl *)iface;
	FIXME("(%p/%p)->(%p): stub\n", This, iface, pIDMDownload);
	return S_OK;
}

static const IDirectMusicPortDownloadVtbl DirectMusicPortDownload_Vtbl = {
	IDirectMusicPortDownloadImpl_QueryInterface,
	IDirectMusicPortDownloadImpl_AddRef,
	IDirectMusicPortDownloadImpl_Release,
	IDirectMusicPortDownloadImpl_GetBuffer,
	IDirectMusicPortDownloadImpl_AllocateBuffer,
	IDirectMusicPortDownloadImpl_GetDLId,
	IDirectMusicPortDownloadImpl_GetAppend,
	IDirectMusicPortDownloadImpl_Download,
	IDirectMusicPortDownloadImpl_Unload
};

/* IDirectMusicThru IUnknown parts follow: */
static HRESULT WINAPI IDirectMusicThruImpl_QueryInterface (LPDIRECTMUSICTHRU iface, REFIID riid, LPVOID *ppobj) {
	ICOM_THIS_MULTI(IDirectMusicPortImpl, lpThruVtbl, iface);
	TRACE("(%p/%p)->(%s, %p)\n", This, iface, debugstr_dmguid(riid), ppobj);
	return IUnknown_QueryInterface((IUnknown *)&(This->lpVtbl), riid, ppobj);
}

static ULONG WINAPI IDirectMusicThruImpl_AddRef (LPDIRECTMUSICTHRU iface) {
	ICOM_THIS_MULTI(IDirectMusicPortImpl, lpThruVtbl, iface);
	TRACE("(%p/%p)->()\n", This, iface);
	return IUnknown_AddRef((IUnknown *)&(This->lpVtbl));
}

static ULONG WINAPI IDirectMusicThruImpl_Release (LPDIRECTMUSICTHRU iface) {
	ICOM_THIS_MULTI(IDirectMusicPortImpl, lpThruVtbl, iface);
	TRACE("(%p/%p)->()\n", This, iface);
	return IUnknown_Release((IUnknown *)&(This->lpVtbl));
}

/* IDirectMusicThru Interface follow: */
static HRESULT WINAPI IDirectMusicThruImpl_ThruChannel (LPDIRECTMUSICTHRU iface, DWORD dwSourceChannelGroup, DWORD dwSourceChannel, DWORD dwDestinationChannelGroup, DWORD dwDestinationChannel, LPDIRECTMUSICPORT pDestinationPort) {
	ICOM_THIS_MULTI(IDirectMusicPortImpl, lpThruVtbl, iface);
	FIXME("(%p/%p)->(%d, %d, %d, %d, %p): stub\n", This, iface, dwSourceChannelGroup, dwSourceChannel, dwDestinationChannelGroup, dwDestinationChannel, pDestinationPort);
	return S_OK;
}

static const IDirectMusicThruVtbl DirectMusicThru_Vtbl = {
	IDirectMusicThruImpl_QueryInterface,
	IDirectMusicThruImpl_AddRef,
	IDirectMusicThruImpl_Release,
	IDirectMusicThruImpl_ThruChannel
};

HRESULT WINAPI DMUSIC_CreateDirectMusicPortImpl (LPCGUID lpcGUID, LPVOID *ppobj, LPUNKNOWN pUnkOuter, LPDMUS_PORTPARAMS pPortParams, LPDMUS_PORTCAPS pPortCaps) {
	IDirectMusicPortImpl *obj;
	HRESULT hr = E_FAIL;

	TRACE("(%p,%p,%p)\n", lpcGUID, ppobj, pUnkOuter);

	obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicPortImpl));
	if (NULL == obj) {
		*ppobj = NULL;
		return E_OUTOFMEMORY;
	}
	obj->lpVtbl = &DirectMusicPort_Vtbl;
	obj->lpDownloadVtbl = &DirectMusicPortDownload_Vtbl;
	obj->lpThruVtbl = &DirectMusicThru_Vtbl;
	obj->ref = 0;  /* will be inited by QueryInterface */
	obj->fActive = FALSE;
	obj->params = *pPortParams;
	obj->caps = *pPortCaps;
	obj->pDirectSound = NULL;
	obj->pLatencyClock = NULL;
	hr = DMUSIC_CreateReferenceClockImpl(&IID_IReferenceClock, (LPVOID*)&obj->pLatencyClock, NULL);

#if 0
	if (pPortParams->dwValidParams & DMUS_PORTPARAMS_CHANNELGROUPS) {
	  obj->nrofgroups = pPortParams->dwChannelGroups;
	  /* setting default priorities */			
	  for (j = 0; j < obj->nrofgroups; j++) {
	    TRACE ("Setting default channel priorities on channel group %i\n", j + 1);
	    obj->group[j].channel[0].priority = DAUD_CHAN1_DEF_VOICE_PRIORITY;
	    obj->group[j].channel[1].priority = DAUD_CHAN2_DEF_VOICE_PRIORITY;
	    obj->group[j].channel[2].priority = DAUD_CHAN3_DEF_VOICE_PRIORITY;
	    obj->group[j].channel[3].priority = DAUD_CHAN4_DEF_VOICE_PRIORITY;
	    obj->group[j].channel[4].priority = DAUD_CHAN5_DEF_VOICE_PRIORITY;
	    obj->group[j].channel[5].priority = DAUD_CHAN6_DEF_VOICE_PRIORITY;
	    obj->group[j].channel[6].priority = DAUD_CHAN7_DEF_VOICE_PRIORITY;
	    obj->group[j].channel[7].priority = DAUD_CHAN8_DEF_VOICE_PRIORITY;
	    obj->group[j].channel[8].priority = DAUD_CHAN9_DEF_VOICE_PRIORITY;
	    obj->group[j].channel[9].priority = DAUD_CHAN10_DEF_VOICE_PRIORITY;
	    obj->group[j].channel[10].priority = DAUD_CHAN11_DEF_VOICE_PRIORITY;
	    obj->group[j].channel[11].priority = DAUD_CHAN12_DEF_VOICE_PRIORITY;
	    obj->group[j].channel[12].priority = DAUD_CHAN13_DEF_VOICE_PRIORITY;
	    obj->group[j].channel[13].priority = DAUD_CHAN14_DEF_VOICE_PRIORITY;
	    obj->group[j].channel[14].priority = DAUD_CHAN15_DEF_VOICE_PRIORITY;
	    obj->group[j].channel[15].priority = DAUD_CHAN16_DEF_VOICE_PRIORITY;
	  }
	}
#endif

	return IDirectMusicPortImpl_QueryInterface ((LPDIRECTMUSICPORT)obj, lpcGUID, ppobj);
}
