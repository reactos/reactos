/*
 *
 * Copyright 2002 Raphael Junqueira
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


#include "d3dx9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

struct ID3DXBufferImpl
{
    ID3DXBuffer ID3DXBuffer_iface;
    LONG ref;

    void *buffer;
    DWORD size;
};

static inline struct ID3DXBufferImpl *impl_from_ID3DXBuffer(ID3DXBuffer *iface)
{
    return CONTAINING_RECORD(iface, struct ID3DXBufferImpl, ID3DXBuffer_iface);
}

static HRESULT WINAPI ID3DXBufferImpl_QueryInterface(ID3DXBuffer *iface, REFIID riid, void **ppobj)
{
    TRACE("iface %p, riid %s, object %p\n", iface, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IUnknown)
      || IsEqualGUID(riid, &IID_ID3DXBuffer))
    {
        IUnknown_AddRef(iface);
        *ppobj = iface;
        return D3D_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI ID3DXBufferImpl_AddRef(ID3DXBuffer *iface)
{
    struct ID3DXBufferImpl *This = impl_from_ID3DXBuffer(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %lu.\n", This, ref);

    return ref;
}

static ULONG WINAPI ID3DXBufferImpl_Release(ID3DXBuffer *iface)
{
    struct ID3DXBufferImpl *This = impl_from_ID3DXBuffer(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %lu.\n", This, ref);

    if (ref == 0)
    {
        free(This->buffer);
        free(This);
    }

    return ref;
}

static void * WINAPI ID3DXBufferImpl_GetBufferPointer(ID3DXBuffer *iface)
{
    struct ID3DXBufferImpl *This = impl_from_ID3DXBuffer(iface);

    TRACE("iface %p\n", iface);

    return This->buffer;
}

static DWORD WINAPI ID3DXBufferImpl_GetBufferSize(ID3DXBuffer *iface)
{
    struct ID3DXBufferImpl *This = impl_from_ID3DXBuffer(iface);

    TRACE("iface %p\n", iface);

    return This->size;
}

static const struct ID3DXBufferVtbl ID3DXBufferImpl_Vtbl =
{
    /* IUnknown methods */
    ID3DXBufferImpl_QueryInterface,
    ID3DXBufferImpl_AddRef,
    ID3DXBufferImpl_Release,
    /* ID3DXBuffer methods */
    ID3DXBufferImpl_GetBufferPointer,
    ID3DXBufferImpl_GetBufferSize
};

static HRESULT d3dx9_buffer_init(struct ID3DXBufferImpl *buffer, DWORD size)
{
    buffer->ID3DXBuffer_iface.lpVtbl = &ID3DXBufferImpl_Vtbl;
    buffer->ref = 1;
    buffer->size = size;

    buffer->buffer = calloc(1, size);
    if (!buffer->buffer)
    {
        ERR("Failed to allocate buffer memory\n");
        return E_OUTOFMEMORY;
    }

    return D3D_OK;
}

HRESULT WINAPI D3DXCreateBuffer(DWORD size, ID3DXBuffer **buffer)
{
    struct ID3DXBufferImpl *object;
    HRESULT hr;

    TRACE("size %lu, buffer %p.\n", size, buffer);

    if (!buffer)
    {
        WARN("Invalid buffer specified.\n");
        return D3DERR_INVALIDCALL;
    }

    object = calloc(1, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    hr = d3dx9_buffer_init(object, size);
    if (FAILED(hr))
    {
        WARN("Failed to initialize buffer, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    *buffer = &object->ID3DXBuffer_iface;

    TRACE("Created ID3DXBuffer %p.\n", *buffer);

    return D3D_OK;
}
