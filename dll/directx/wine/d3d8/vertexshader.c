/*
 * IDirect3DVertexShader8 implementation
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

/* IDirect3DVertexShader8 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DVertexShader8Impl_QueryInterface(IDirect3DVertexShader8 *iface, REFIID riid, LPVOID* ppobj) {
    IDirect3DVertexShader8Impl *This = (IDirect3DVertexShader8Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DVertexShader8)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DVertexShader8Impl_AddRef(IDirect3DVertexShader8 *iface) {
    IDirect3DVertexShader8Impl *This = (IDirect3DVertexShader8Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %d\n", This, ref - 1);

    return ref;
}

static ULONG WINAPI IDirect3DVertexShader8Impl_Release(IDirect3DVertexShader8 *iface) {
    IDirect3DVertexShader8Impl *This = (IDirect3DVertexShader8Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %d\n", This, ref);

    if (ref == 0) {
        IDirect3DVertexDeclaration8_Release(This->vertex_declaration);
        if (This->wineD3DVertexShader) IWineD3DVertexShader_Release(This->wineD3DVertexShader);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

const IDirect3DVertexShader8Vtbl Direct3DVertexShader8_Vtbl =
{
    /* IUnknown */
    IDirect3DVertexShader8Impl_QueryInterface,
    IDirect3DVertexShader8Impl_AddRef,
    IDirect3DVertexShader8Impl_Release,
};
