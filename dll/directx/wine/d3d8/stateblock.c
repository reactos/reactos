/*
 * IDirect3DStateBlock8 implementation
 *
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2002-2003 Jason Edmeades
 * Copyright 2005 Oliver Stieber
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

#include "config.h"
#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d8);

/* NOTE: DirectX8 doesn't export an IDirect3DStateBlock8, the interface is used internally to keep d3d8 and d3d9 as similar as possible */
/* IDirect3DStateBlock8 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DStateBlock8Impl_QueryInterface(IDirect3DStateBlock8 *iface, REFIID riid, LPVOID *ppobj) {
    IDirect3DStateBlock8Impl *This = (IDirect3DStateBlock8Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DStateBlock8)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DStateBlock8Impl_AddRef(IDirect3DStateBlock8 *iface) {
    IDirect3DStateBlock8Impl *This = (IDirect3DStateBlock8Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %d\n", This, ref - 1);

    return ref;
}

static ULONG WINAPI IDirect3DStateBlock8Impl_Release(IDirect3DStateBlock8 *iface) {
    IDirect3DStateBlock8Impl *This = (IDirect3DStateBlock8Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %d\n", This, ref);

    if (ref == 0) {
        IWineD3DStateBlock_Release(This->wineD3DStateBlock);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DStateBlock8 Interface follow: */
static HRESULT WINAPI IDirect3DStateBlock8Impl_GetDevice(IDirect3DStateBlock8 *iface, IDirect3DDevice8 **ppDevice) {
    IDirect3DStateBlock8Impl *This = (IDirect3DStateBlock8Impl *)iface;
    TRACE("(%p) Relay\n", This); 
    return IDirect3DResource8Impl_GetDevice((LPDIRECT3DRESOURCE8) This, ppDevice);
}

static HRESULT WINAPI IDirect3DStateBlock8Impl_Capture(IDirect3DStateBlock8 *iface) {
    IDirect3DStateBlock8Impl *This = (IDirect3DStateBlock8Impl *)iface;
    TRACE("(%p) Relay\n", This); 
    return IWineD3DStateBlock_Capture(This->wineD3DStateBlock);
}

static HRESULT WINAPI IDirect3DStateBlock8Impl_Apply(IDirect3DStateBlock8 *iface) {
    IDirect3DStateBlock8Impl *This = (IDirect3DStateBlock8Impl *)iface;
    TRACE("(%p) Relay\n", This); 
    return IWineD3DStateBlock_Apply(This->wineD3DStateBlock);
}

const IDirect3DStateBlock8Vtbl Direct3DStateBlock8_Vtbl =
{
    /* IUnknown */
    IDirect3DStateBlock8Impl_QueryInterface,
    IDirect3DStateBlock8Impl_AddRef,
    IDirect3DStateBlock8Impl_Release,
    /* IDirect3DStateBlock8 */
    IDirect3DStateBlock8Impl_GetDevice,
    IDirect3DStateBlock8Impl_Capture,
    IDirect3DStateBlock8Impl_Apply
};
