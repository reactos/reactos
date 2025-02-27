/*
 * Copyright 2017 Henri Verbeet for CodeWeavers
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

#include "d2d1_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d2d);

static inline struct d2d_layer *impl_from_ID2D1Layer(ID2D1Layer *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_layer, ID2D1Layer_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_layer_QueryInterface(ID2D1Layer *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1Layer)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1Layer_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_layer_AddRef(ID2D1Layer *iface)
{
    struct d2d_layer *layer = impl_from_ID2D1Layer(iface);
    ULONG refcount = InterlockedIncrement(&layer->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_layer_Release(ID2D1Layer *iface)
{
    struct d2d_layer *layer = impl_from_ID2D1Layer(iface);
    ULONG refcount = InterlockedDecrement(&layer->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        ID2D1Factory_Release(layer->factory);
        free(layer);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d2d_layer_GetFactory(ID2D1Layer *iface, ID2D1Factory **factory)
{
    struct d2d_layer *layer = impl_from_ID2D1Layer(iface);

    TRACE("iface %p, factory %p.\n", iface, factory);

    ID2D1Factory_AddRef(*factory = layer->factory);
}

static D2D1_SIZE_F * STDMETHODCALLTYPE d2d_layer_GetSize(ID2D1Layer *iface, D2D1_SIZE_F *size)
{
    struct d2d_layer *layer = impl_from_ID2D1Layer(iface);

    TRACE("iface %p, size %p.\n", iface, size);

    *size = layer->size;
    return size;
}

static const struct ID2D1LayerVtbl d2d_layer_vtbl =
{
    d2d_layer_QueryInterface,
    d2d_layer_AddRef,
    d2d_layer_Release,
    d2d_layer_GetFactory,
    d2d_layer_GetSize,
};

HRESULT d2d_layer_create(ID2D1Factory *factory, const D2D1_SIZE_F *size, struct d2d_layer **layer)
{
    if (!(*layer = calloc(1, sizeof(**layer))))
        return E_OUTOFMEMORY;

    (*layer)->ID2D1Layer_iface.lpVtbl = &d2d_layer_vtbl;
    (*layer)->refcount = 1;
    ID2D1Factory_AddRef((*layer)->factory = factory);
    if (size)
        (*layer)->size = *size;

    TRACE("Created layer %p.\n", *layer);

    return S_OK;
}
