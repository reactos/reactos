/*
 *  ITfDisplayAttributeMgr implementation
 *
 *  Copyright 2010 CodeWeavers, Aric Stewart
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

#define COBJMACROS

#include "wine/debug.h"
#include "winbase.h"
#include "winreg.h"
#include "shlwapi.h"

#include "msctf.h"
#include "msctf_internal.h"

WINE_DEFAULT_DEBUG_CHANNEL(msctf);

typedef struct tagDisplayAttributeMgr {
    ITfDisplayAttributeMgr ITfDisplayAttributeMgr_iface;

    LONG refCount;

} DisplayAttributeMgr;

static inline DisplayAttributeMgr *impl_from_ITfDisplayAttributeMgr(ITfDisplayAttributeMgr *iface)
{
    return CONTAINING_RECORD(iface, DisplayAttributeMgr, ITfDisplayAttributeMgr_iface);
}

static void DisplayAttributeMgr_Destructor(DisplayAttributeMgr *This)
{
    TRACE("destroying %p\n", This);

    HeapFree(GetProcessHeap(),0,This);
}

static HRESULT WINAPI DisplayAttributeMgr_QueryInterface(ITfDisplayAttributeMgr *iface, REFIID iid, LPVOID *ppvOut)
{
    DisplayAttributeMgr *This = impl_from_ITfDisplayAttributeMgr(iface);
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_ITfDisplayAttributeMgr))
    {
        *ppvOut = &This->ITfDisplayAttributeMgr_iface;
    }

    if (*ppvOut)
    {
        ITfDisplayAttributeMgr_AddRef(iface);
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI DisplayAttributeMgr_AddRef(ITfDisplayAttributeMgr *iface)
{
    DisplayAttributeMgr *This = impl_from_ITfDisplayAttributeMgr(iface);
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI DisplayAttributeMgr_Release(ITfDisplayAttributeMgr *iface)
{
    DisplayAttributeMgr *This = impl_from_ITfDisplayAttributeMgr(iface);
    ULONG ret;

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        DisplayAttributeMgr_Destructor(This);
    return ret;
}

/*****************************************************
 * ITfDisplayAttributeMgr functions
 *****************************************************/

static HRESULT WINAPI DisplayAttributeMgr_OnUpdateInfo(ITfDisplayAttributeMgr *iface)
{
    DisplayAttributeMgr *This = impl_from_ITfDisplayAttributeMgr(iface);

    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DisplayAttributeMgr_EnumDisplayAttributeInfo(ITfDisplayAttributeMgr *iface, IEnumTfDisplayAttributeInfo **ppEnum)
{
    DisplayAttributeMgr *This = impl_from_ITfDisplayAttributeMgr(iface);

    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DisplayAttributeMgr_GetDisplayAttributeInfo(ITfDisplayAttributeMgr *iface, REFGUID guid, ITfDisplayAttributeInfo **ppInfo, CLSID *pclsidOwner)
{
    DisplayAttributeMgr *This = impl_from_ITfDisplayAttributeMgr(iface);

    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static const ITfDisplayAttributeMgrVtbl DisplayAttributeMgrVtbl =
{
    DisplayAttributeMgr_QueryInterface,
    DisplayAttributeMgr_AddRef,
    DisplayAttributeMgr_Release,
    DisplayAttributeMgr_OnUpdateInfo,
    DisplayAttributeMgr_EnumDisplayAttributeInfo,
    DisplayAttributeMgr_GetDisplayAttributeInfo
};

HRESULT DisplayAttributeMgr_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut)
{
    DisplayAttributeMgr *This;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(),0,sizeof(DisplayAttributeMgr));
    if (This == NULL)
        return E_OUTOFMEMORY;

    This->ITfDisplayAttributeMgr_iface.lpVtbl = &DisplayAttributeMgrVtbl;
    This->refCount = 1;

    *ppOut = (IUnknown *)&This->ITfDisplayAttributeMgr_iface;
    TRACE("returning %p\n", *ppOut);
    return S_OK;
}
