/*
 * Null renderer filter
 *
 * Copyright 2004 Christian Costa
 * Copyright 2008 Maarten Lankhorst
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
#include "dshow.h"
#include "wine/debug.h"
#include "wine/strmbase.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

struct null_renderer
{
    struct strmbase_renderer renderer;
};

static struct null_renderer *impl_from_strmbase_renderer(struct strmbase_renderer *iface)
{
    return CONTAINING_RECORD(iface, struct null_renderer, renderer);
}

static HRESULT null_renderer_render(struct strmbase_renderer *iface, IMediaSample *sample)
{
    return S_OK;
}

static HRESULT null_renderer_query_accept(struct strmbase_renderer *iface, const AM_MEDIA_TYPE *mt)
{
    return S_OK;
}

static void null_renderer_destroy(struct strmbase_renderer *iface)
{
    struct null_renderer *filter = impl_from_strmbase_renderer(iface);

    strmbase_renderer_cleanup(&filter->renderer);
    free(filter);
}

static const struct strmbase_renderer_ops renderer_ops =
{
    .renderer_query_accept = null_renderer_query_accept,
    .renderer_render = null_renderer_render,
    .renderer_destroy = null_renderer_destroy,
};

HRESULT null_renderer_create(IUnknown *outer, IUnknown **out)
{
    struct null_renderer *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    strmbase_renderer_init(&object->renderer, outer, &CLSID_NullRenderer, L"In", &renderer_ops);

    TRACE("Created null renderer %p.\n", object);
    *out = &object->renderer.filter.IUnknown_inner;
    return S_OK;
}
