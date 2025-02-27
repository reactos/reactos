/*
 * Copyright 2017 Józef Kucia for CodeWeavers
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

#ifndef __MINGW32__
#define WIDL_C_INLINE_WRAPPERS
#endif
#define COBJMACROS
#define CONST_VTABLE
#include "vkd3d.h"
#include "vkd3d_blob.h"
#include "vkd3d_memory.h"
#include "d3d12shader.h"

struct vkd3d_blob
{
    ID3D10Blob ID3DBlob_iface;
    unsigned int refcount;

    void *buffer;
    SIZE_T size;
};

static struct vkd3d_blob *impl_from_ID3DBlob(ID3DBlob *iface)
{
    return CONTAINING_RECORD(iface, struct vkd3d_blob, ID3DBlob_iface);
}

static HRESULT STDMETHODCALLTYPE vkd3d_blob_QueryInterface(ID3DBlob *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3DBlob)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D10Blob_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE vkd3d_blob_AddRef(ID3DBlob *iface)
{
    struct vkd3d_blob *blob = impl_from_ID3DBlob(iface);
    unsigned int refcount = vkd3d_atomic_increment_u32(&blob->refcount);

    TRACE("%p increasing refcount to %u.\n", blob, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE vkd3d_blob_Release(ID3DBlob *iface)
{
    struct vkd3d_blob *blob = impl_from_ID3DBlob(iface);
    unsigned int refcount = vkd3d_atomic_decrement_u32(&blob->refcount);

    TRACE("%p decreasing refcount to %u.\n", blob, refcount);

    if (!refcount)
    {
        vkd3d_free(blob->buffer);

        vkd3d_free(blob);
    }

    return refcount;
}

static void * STDMETHODCALLTYPE vkd3d_blob_GetBufferPointer(ID3DBlob *iface)
{
    struct vkd3d_blob *blob = impl_from_ID3DBlob(iface);

    TRACE("iface %p.\n", iface);

    return blob->buffer;
}

static SIZE_T STDMETHODCALLTYPE vkd3d_blob_GetBufferSize(ID3DBlob *iface)
{
    struct vkd3d_blob *blob = impl_from_ID3DBlob(iface);

    TRACE("iface %p.\n", iface);

    return blob->size;
}

static const struct ID3D10BlobVtbl vkd3d_blob_vtbl =
{
    /* IUnknown methods */
    vkd3d_blob_QueryInterface,
    vkd3d_blob_AddRef,
    vkd3d_blob_Release,
    /* ID3DBlob methods */
    vkd3d_blob_GetBufferPointer,
    vkd3d_blob_GetBufferSize
};

static void vkd3d_blob_init(struct vkd3d_blob *blob, void *buffer, SIZE_T size)
{
    blob->ID3DBlob_iface.lpVtbl = &vkd3d_blob_vtbl;
    blob->refcount = 1;

    blob->buffer = buffer;
    blob->size = size;
}

HRESULT vkd3d_blob_create(void *buffer, SIZE_T size, ID3D10Blob **blob)
{
    struct vkd3d_blob *object;

    if (!(object = vkd3d_malloc(sizeof(*object))))
        return E_OUTOFMEMORY;

    vkd3d_blob_init(object, buffer, size);

    TRACE("Created blob object %p.\n", object);

    *blob = &object->ID3DBlob_iface;

    return S_OK;
}
