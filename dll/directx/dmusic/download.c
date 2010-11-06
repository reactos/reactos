/* IDirectMusicDownload Implementation
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

/* IDirectMusicDownloadImpl IUnknown part: */
static HRESULT WINAPI IDirectMusicDownloadImpl_QueryInterface (LPDIRECTMUSICDOWNLOAD iface, REFIID riid, LPVOID *ppobj) {
	IDirectMusicDownloadImpl *This = (IDirectMusicDownloadImpl *)iface;
	TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);

	if (IsEqualIID (riid, &IID_IUnknown) 
		|| IsEqualIID (riid, &IID_IDirectMusicDownload)) {
		IUnknown_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI IDirectMusicDownloadImpl_AddRef (LPDIRECTMUSICDOWNLOAD iface) {
	IDirectMusicDownloadImpl *This = (IDirectMusicDownloadImpl *)iface;
	ULONG refCount = InterlockedIncrement(&This->ref);

	TRACE("(%p)->(ref before=%u)\n", This, refCount - 1);

	DMUSIC_LockModule();

	return refCount;
}

static ULONG WINAPI IDirectMusicDownloadImpl_Release (LPDIRECTMUSICDOWNLOAD iface) {
	IDirectMusicDownloadImpl *This = (IDirectMusicDownloadImpl *)iface;
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(ref before=%u)\n", This, refCount + 1);

	if (!refCount) {
		HeapFree(GetProcessHeap(), 0, This);
	}

	DMUSIC_UnlockModule();
	
	return refCount;
}

/* IDirectMusicDownloadImpl IDirectMusicDownload part: */
static HRESULT WINAPI IDirectMusicDownloadImpl_GetBuffer (LPDIRECTMUSICDOWNLOAD iface, void** ppvBuffer, DWORD* pdwSize) {
	IDirectMusicDownloadImpl *This = (IDirectMusicDownloadImpl *)iface;
	FIXME("(%p, %p, %p): stub\n", This, ppvBuffer, pdwSize);
	return S_OK;
}

static const IDirectMusicDownloadVtbl DirectMusicDownload_Vtbl = {
	IDirectMusicDownloadImpl_QueryInterface,
	IDirectMusicDownloadImpl_AddRef,
	IDirectMusicDownloadImpl_Release,
	IDirectMusicDownloadImpl_GetBuffer
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateDirectMusicDownloadImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter) {
	IDirectMusicDownloadImpl* dmdl;
	
	dmdl = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectMusicDownloadImpl));
	if (NULL == dmdl) {
		*ppobj = NULL;
		return E_OUTOFMEMORY;
	}
	dmdl->lpVtbl = &DirectMusicDownload_Vtbl;
	dmdl->ref = 0; /* will be inited by QueryInterface */
	
	return IDirectMusicDownloadImpl_QueryInterface ((LPDIRECTMUSICDOWNLOAD)dmdl, lpcGUID, ppobj);
}
