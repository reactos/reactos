/* IDirectMusicDownloadedInstrument Implementation
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

/* IDirectMusicDownloadedInstrumentImpl IUnknown part: */
static HRESULT WINAPI IDirectMusicDownloadedInstrumentImpl_QueryInterface (LPDIRECTMUSICDOWNLOADEDINSTRUMENT iface, REFIID riid, LPVOID *ppobj) {
	IDirectMusicDownloadedInstrumentImpl *This = (IDirectMusicDownloadedInstrumentImpl *)iface;
	TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);

	if (IsEqualIID (riid, &IID_IUnknown)
		|| IsEqualIID (riid, &IID_IDirectMusicDownloadedInstrument)
		|| IsEqualIID (riid, &IID_IDirectMusicDownloadedInstrument8)) {
		IUnknown_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicDownloadedInstrumentImpl_AddRef (LPDIRECTMUSICDOWNLOADEDINSTRUMENT iface) {
	IDirectMusicDownloadedInstrumentImpl *This = (IDirectMusicDownloadedInstrumentImpl *)iface;
	ULONG refCount = InterlockedIncrement(&This->ref);

	TRACE("(%p)->(ref before=%u)\n", This, refCount - 1);

	DMUSIC_LockModule();

	return refCount;
}

static ULONG WINAPI IDirectMusicDownloadedInstrumentImpl_Release (LPDIRECTMUSICDOWNLOADEDINSTRUMENT iface) {
	IDirectMusicDownloadedInstrumentImpl *This = (IDirectMusicDownloadedInstrumentImpl *)iface;
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(ref before=%u)\n", This, refCount + 1);

	if (!refCount) {
		HeapFree(GetProcessHeap(), 0, This);
	}

	DMUSIC_UnlockModule();
	
	return refCount;
}

/* IDirectMusicDownloadedInstrumentImpl IDirectMusicDownloadedInstrument part: */
/* none at this time */

static const IDirectMusicDownloadedInstrumentVtbl DirectMusicDownloadedInstrument_Vtbl = {
	IDirectMusicDownloadedInstrumentImpl_QueryInterface,
	IDirectMusicDownloadedInstrumentImpl_AddRef,
	IDirectMusicDownloadedInstrumentImpl_Release
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicDownloadedInstrumentImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter) {
	IDirectMusicDownloadedInstrumentImpl* dmdlinst;
	
	dmdlinst = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicDownloadedInstrumentImpl));
	if (NULL == dmdlinst) {
		*ppobj = NULL;
		return E_OUTOFMEMORY;
	}
	dmdlinst->lpVtbl = &DirectMusicDownloadedInstrument_Vtbl;
	dmdlinst->ref = 0; /* will be inited by QueryInterface */
	
	return IDirectMusicDownloadedInstrumentImpl_QueryInterface ((LPDIRECTMUSICDOWNLOADEDINSTRUMENT)dmdlinst, lpcGUID, ppobj);	
}
