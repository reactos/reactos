/*
 * IReferenceClock Implementation
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

static inline IReferenceClockImpl *impl_from_IReferenceClock(IReferenceClock *iface)
{
    return CONTAINING_RECORD(iface, IReferenceClockImpl, IReferenceClock_iface);
}

/* IReferenceClockImpl IUnknown part: */
static HRESULT WINAPI IReferenceClockImpl_QueryInterface(IReferenceClock *iface, REFIID riid, LPVOID *ppobj)
{
	IReferenceClockImpl *This = impl_from_IReferenceClock(iface);
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

static ULONG WINAPI IReferenceClockImpl_AddRef(IReferenceClock *iface)
{
    IReferenceClockImpl *This = impl_from_IReferenceClock(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p): new ref = %lu\n", This, ref);

    return ref;
}

static ULONG WINAPI IReferenceClockImpl_Release(IReferenceClock *iface)
{
    IReferenceClockImpl *This = impl_from_IReferenceClock(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p): new ref = %lu\n", This, ref);

    if (!ref) {
        free(This);
    }

    return ref;
}

/* IReferenceClockImpl IReferenceClock part: */
static HRESULT WINAPI IReferenceClockImpl_GetTime(IReferenceClock *iface, REFERENCE_TIME* pTime)
{
    IReferenceClockImpl *This = impl_from_IReferenceClock(iface);

    TRACE("(%p)->(%p)\n", This, pTime);

    *pTime = This->rtTime;

    return S_OK;
}

static HRESULT WINAPI IReferenceClockImpl_AdviseTime(IReferenceClock *iface, REFERENCE_TIME base,
        REFERENCE_TIME offset, HEVENT event, DWORD_PTR *cookie)
{
    IReferenceClockImpl *This = impl_from_IReferenceClock(iface);
    FIXME("(%p)->(%I64d, %I64d, %#Ix, %p): stub\n", This, base, offset, event, cookie);
    return S_OK;
}

static HRESULT WINAPI IReferenceClockImpl_AdvisePeriodic(IReferenceClock *iface, REFERENCE_TIME start,
        REFERENCE_TIME period, HSEMAPHORE semaphore, DWORD_PTR *cookie)
{
    IReferenceClockImpl *This = impl_from_IReferenceClock(iface);
    FIXME("(%p)->(%I64d, %I64d, %#Ix, %p): stub\n", This, start, period, semaphore, cookie);
    return S_OK;
}

static HRESULT WINAPI IReferenceClockImpl_Unadvise(IReferenceClock *iface, DWORD_PTR cookie)
{
    IReferenceClockImpl *This = impl_from_IReferenceClock(iface);
    FIXME("(%p, %#Ix): stub\n", This, cookie);
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
HRESULT DMUSIC_CreateReferenceClockImpl(LPCGUID riid, LPVOID* ret_iface, LPUNKNOWN unkouter)
{
    IReferenceClockImpl* clock;
    HRESULT hr;

    TRACE("(%s, %p, %p)\n", debugstr_guid(riid), ret_iface, unkouter);

    clock = calloc(1, sizeof(IReferenceClockImpl));
    if (!clock) {
        *ret_iface = NULL;
        return E_OUTOFMEMORY;
    }

    clock->IReferenceClock_iface.lpVtbl = &ReferenceClock_Vtbl;
    clock->ref = 1;
    clock->rtTime = 0;
    clock->pClockInfo.dwSize = sizeof (DMUS_CLOCKINFO);

    hr = IReferenceClockImpl_QueryInterface(&clock->IReferenceClock_iface, riid, ret_iface);
    IReferenceClock_Release(&clock->IReferenceClock_iface);

    return hr;
}
