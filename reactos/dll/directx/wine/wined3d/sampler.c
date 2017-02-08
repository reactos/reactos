/*
 * Copyright 2012 Henri Verbeet for CodeWeavers
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
 *
 */

#include <config.h>
#include <wine/port.h>

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

ULONG CDECL wined3d_sampler_incref(struct wined3d_sampler *sampler)
{
    ULONG refcount = InterlockedIncrement(&sampler->refcount);

    TRACE("%p increasing refcount to %u.\n", sampler, refcount);

    return refcount;
}

ULONG CDECL wined3d_sampler_decref(struct wined3d_sampler *sampler)
{
    ULONG refcount = InterlockedDecrement(&sampler->refcount);

    TRACE("%p decreasing refcount to %u.\n", sampler, refcount);

    if (!refcount)
        HeapFree(GetProcessHeap(), 0, sampler);

    return refcount;
}

void * CDECL wined3d_sampler_get_parent(const struct wined3d_sampler *sampler)
{
    TRACE("sampler %p.\n", sampler);

    return sampler->parent;
}

static void wined3d_sampler_init(struct wined3d_sampler *sampler, void *parent)
{
    sampler->refcount = 1;
    sampler->parent = parent;
}

HRESULT CDECL wined3d_sampler_create(void *parent, struct wined3d_sampler **sampler)
{
    struct wined3d_sampler *object;

    TRACE("parent %p, sampler %p.\n", parent, sampler);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    wined3d_sampler_init(object, parent);

    TRACE("Created sampler %p.\n", object);
    *sampler = object;

    return WINED3D_OK;
}
