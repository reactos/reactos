/* IReferenceClock Implementation
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

/* IReferenceClockImpl IUnknown part: */
static HRESULT WINAPI IReferenceClockImpl_QueryInterface (IReferenceClock *iface, REFIID riid, LPVOID *ppobj) {
	IReferenceClockImpl *This = (IReferenceClockImpl *)iface;
	TRACE("(%p, %s, %p)\n", This, debugstr_dmguid(riid), ppobj);

	if (IsEqualIID (riid, &IID_IUnknown) || 
	    IsEqualIID (riid, &IID_IReferenceClock)) {
		IUnknown_AddRef(iface);
		*ppobj = This;
		return S_OK;
	}
	WARN("(%p, %s, %p): not found\n", This, debugstr_dmguid(riid), ppobj);
	return E_NOINTERFACE;
}

static ULONG WINAPI IReferenceClockImpl_AddRef (IReferenceClock *iface) {
	IReferenceClockImpl *This = (IReferenceClockImpl *)iface;
	ULONG refCount = InterlockedIncrement(&This->ref);

	TRACE("(%p)->(ref before=%u)\n", This, refCount - 1);

	DMUSIC_LockModule();

	return refCount;
}

static ULONG WINAPI IReferenceClockImpl_Release (IReferenceClock *iface) {
	IReferenceClockImpl *This = (IReferenceClockImpl *)iface;
	ULONG refCount = InterlockedDecrement(&This->ref);

	TRACE("(%p)->(ref before=%u)\n", This, refCount + 1);

	if (!refCount) {
		HeapFree(GetProcessHeap(), 0, This);
	}

	DMUSIC_UnlockModule();

	return refCount;
}

/* IReferenceClockImpl IReferenceClock part: */
static HRESULT WINAPI IReferenceClockImpl_GetTime (IReferenceClock *iface, REFERENCE_TIME* pTime) {
	IReferenceClockImpl *This = (IReferenceClockImpl *)iface;
	TRACE("(%p, %p)\n", This, pTime);
	*pTime = This->rtTime;
	return S_OK;
}

static HRESULT WINAPI IReferenceClockImpl_AdviseTime (IReferenceClock *iface, REFERENCE_TIME baseTime, REFERENCE_TIME streamTime, HANDLE hEvent, DWORD* pdwAdviseCookie) {
	IReferenceClockImpl *This = (IReferenceClockImpl *)iface;
	FIXME("(%p, 0x%s, 0x%s, %p, %p): stub\n", This, wine_dbgstr_longlong(baseTime), wine_dbgstr_longlong(streamTime), hEvent, pdwAdviseCookie);
	return S_OK;
}

static HRESULT WINAPI IReferenceClockImpl_AdvisePeriodic (IReferenceClock *iface, REFERENCE_TIME startTime, REFERENCE_TIME periodTime, HANDLE hSemaphore, DWORD* pdwAdviseCookie) {
	IReferenceClockImpl *This = (IReferenceClockImpl *)iface;
	FIXME("(%p, 0x%s, 0x%s, %p, %p): stub\n", This, wine_dbgstr_longlong(startTime), wine_dbgstr_longlong(periodTime), hSemaphore, pdwAdviseCookie);
	return S_OK;
}

static HRESULT WINAPI IReferenceClockImpl_Unadvise (IReferenceClock *iface, DWORD dwAdviseCookie) {
	IReferenceClockImpl *This = (IReferenceClockImpl *)iface;
	FIXME("(%p, %d): stub\n", This, dwAdviseCookie);
	return S_OK;
}

static const IReferenceClockVtbl ReferenceClock_Vtbl = {
	IReferenceClockImpl_QueryInterface,
	IReferenceClockImpl_AddRef,
	IReferenceClockImpl_Release,
	IReferenceClockImpl_GetTime,
	IReferenceClockImpl_AdviseTime,
	IReferenceClockImpl_AdvisePeriodic,
	IReferenceClockImpl_Unadvise
};

/* for ClassFactory */
HRESULT WINAPI DMUSIC_CreateReferenceClockImpl (LPCGUID lpcGUID, LPVOID* ppobj, LPUNKNOWN pUnkOuter) {
	IReferenceClockImpl* clock;

	clock = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IReferenceClockImpl));
	if (NULL == clock) {
		*ppobj = NULL;
		return E_OUTOFMEMORY;
	}
	clock->lpVtbl = &ReferenceClock_Vtbl;
	clock->ref = 0; /* will be inited by QueryInterface */
	clock->rtTime = 0;
	clock->pClockInfo.dwSize = sizeof (DMUS_CLOCKINFO);
		
	return IReferenceClockImpl_QueryInterface ((IReferenceClock *)clock, lpcGUID, ppobj);
}
