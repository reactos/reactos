/*
 * IDirect3DPixelShader8 implementation
 *
 * Copyright 2002-2003 Jason Edmeades
 *                     Raphael Junqueira
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

/* IDirect3DPixelShader8 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DPixelShader8Impl_QueryInterface(IDirect3DPixelShader8 *iface, REFIID riid, LPVOID *ppobj) {
    IDirect3DPixelShader8Impl *This = (IDirect3DPixelShader8Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DPixelShader8)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DPixelShader8Impl_AddRef(IDirect3DPixelShader8 *iface) {
    IDirect3DPixelShader8Impl *This = (IDirect3DPixelShader8Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %d\n", This, ref - 1);

    return ref;
}

static ULONG WINAPI IDirect3DPixelShader8Impl_Release(IDirect3DPixelShader8 * iface) {
    IDirect3DPixelShader8Impl *This = (IDirect3DPixelShader8Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %d\n", This, ref);

    if (ref == 0) {
        EnterCriticalSection(&d3d8_cs);
        IWineD3DPixelShader_Release(This->wineD3DPixelShader);
        LeaveCriticalSection(&d3d8_cs);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

const IDirect3DPixelShader8Vtbl Direct3DPixelShader8_Vtbl =
{
    /* IUnknown */
    IDirect3DPixelShader8Impl_QueryInterface,
    IDirect3DPixelShader8Impl_AddRef,
    IDirect3DPixelShader8Impl_Release,
};
