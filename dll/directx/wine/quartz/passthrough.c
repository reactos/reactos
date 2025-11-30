/*
 * Seeking passthrough object
 *
 * Copyright 2020 Zebediah Figura for CodeWeavers
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

#include "quartz_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

struct seeking_passthrough
{
    struct strmbase_passthrough passthrough;

    IUnknown IUnknown_inner;
    IUnknown *outer_unk;
    LONG refcount;
};

static struct seeking_passthrough *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct seeking_passthrough, IUnknown_inner);
}

static HRESULT WINAPI seeking_passthrough_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    struct seeking_passthrough *passthrough = impl_from_IUnknown(iface);

    TRACE("passthrough %p, iid %s, out %p.\n", passthrough, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown))
        *out = iface;
    else if (IsEqualGUID(iid, &IID_IMediaPosition))
        *out = &passthrough->passthrough.IMediaPosition_iface;
    else if (IsEqualGUID(iid, &IID_IMediaSeeking))
        *out = &passthrough->passthrough.IMediaSeeking_iface;
    else if (IsEqualGUID(iid, &IID_ISeekingPassThru))
        *out = &passthrough->passthrough.ISeekingPassThru_iface;
    else
    {
        *out = NULL;
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI seeking_passthrough_AddRef(IUnknown *iface)
{
    struct seeking_passthrough *passthrough = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&passthrough->refcount);

    TRACE("%p increasing refcount to %lu.\n", passthrough, refcount);
    return refcount;
}

static ULONG WINAPI seeking_passthrough_Release(IUnknown *iface)
{
    struct seeking_passthrough *passthrough = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&passthrough->refcount);

    TRACE("%p decreasing refcount to %lu.\n", passthrough, refcount);
    if (!refcount)
    {
        strmbase_passthrough_cleanup(&passthrough->passthrough);
        free(passthrough);
    }
    return refcount;
}

static const IUnknownVtbl seeking_passthrough_vtbl =
{
    seeking_passthrough_QueryInterface,
    seeking_passthrough_AddRef,
    seeking_passthrough_Release,
};

HRESULT seeking_passthrough_create(IUnknown *outer, IUnknown **out)
{
    struct seeking_passthrough *object;

    TRACE("outer %p, out %p.\n", outer, out);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IUnknown_inner.lpVtbl = &seeking_passthrough_vtbl;
    object->outer_unk = outer ? outer : &object->IUnknown_inner;
    object->refcount = 1;

    strmbase_passthrough_init(&object->passthrough, object->outer_unk);

    TRACE("Created seeking passthrough %p.\n", object);
    *out = &object->IUnknown_inner;
    return S_OK;
}
