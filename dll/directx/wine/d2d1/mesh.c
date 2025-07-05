/*
 * Copyright 2014 Henri Verbeet for CodeWeavers
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

static inline struct d2d_mesh *impl_from_ID2D1Mesh(ID2D1Mesh *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_mesh, ID2D1Mesh_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_mesh_QueryInterface(ID2D1Mesh *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1Mesh)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1Mesh_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_mesh_AddRef(ID2D1Mesh *iface)
{
    struct d2d_mesh *mesh = impl_from_ID2D1Mesh(iface);
    ULONG refcount = InterlockedIncrement(&mesh->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_mesh_Release(ID2D1Mesh *iface)
{
    struct d2d_mesh *mesh = impl_from_ID2D1Mesh(iface);
    ULONG refcount = InterlockedDecrement(&mesh->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        ID2D1Factory_Release(mesh->factory);
        free(mesh);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d2d_mesh_GetFactory(ID2D1Mesh *iface, ID2D1Factory **factory)
{
    struct d2d_mesh *mesh = impl_from_ID2D1Mesh(iface);

    TRACE("iface %p, factory %p.\n", iface, factory);

    ID2D1Factory_AddRef(*factory = mesh->factory);
}

static HRESULT STDMETHODCALLTYPE d2d_mesh_Open(ID2D1Mesh *iface, ID2D1TessellationSink **sink)
{
    FIXME("iface %p, sink %p stub!\n", iface, sink);

    return E_NOTIMPL;
}

static const struct ID2D1MeshVtbl d2d_mesh_vtbl =
{
    d2d_mesh_QueryInterface,
    d2d_mesh_AddRef,
    d2d_mesh_Release,
    d2d_mesh_GetFactory,
    d2d_mesh_Open,
};

HRESULT d2d_mesh_create(ID2D1Factory *factory, struct d2d_mesh **mesh)
{
    if (!(*mesh = calloc(1, sizeof(**mesh))))
        return E_OUTOFMEMORY;

    (*mesh)->ID2D1Mesh_iface.lpVtbl = &d2d_mesh_vtbl;
    (*mesh)->refcount = 1;
    ID2D1Factory_AddRef((*mesh)->factory = factory);

    TRACE("Created mesh %p.\n", *mesh);
    return S_OK;
}
