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

static inline struct d2d_stroke_style *impl_from_ID2D1StrokeStyle1(ID2D1StrokeStyle1 *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_stroke_style, ID2D1StrokeStyle1_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_stroke_style_QueryInterface(ID2D1StrokeStyle1 *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1StrokeStyle)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1StrokeStyle1_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_stroke_style_AddRef(ID2D1StrokeStyle1 *iface)
{
    struct d2d_stroke_style *style = impl_from_ID2D1StrokeStyle1(iface);
    ULONG refcount = InterlockedIncrement(&style->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_stroke_style_Release(ID2D1StrokeStyle1 *iface)
{
    struct d2d_stroke_style *style = impl_from_ID2D1StrokeStyle1(iface);
    ULONG refcount = InterlockedDecrement(&style->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        ID2D1Factory_Release(style->factory);
        if (style->desc.dashStyle == D2D1_DASH_STYLE_CUSTOM)
            free(style->dashes);
        free(style);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d2d_stroke_style_GetFactory(ID2D1StrokeStyle1 *iface, ID2D1Factory **factory)
{
    struct d2d_stroke_style *style = impl_from_ID2D1StrokeStyle1(iface);

    TRACE("iface %p, factory %p.\n", iface, factory);

    ID2D1Factory_AddRef(*factory = style->factory);
}

static D2D1_CAP_STYLE STDMETHODCALLTYPE d2d_stroke_style_GetStartCap(ID2D1StrokeStyle1 *iface)
{
    struct d2d_stroke_style *style = impl_from_ID2D1StrokeStyle1(iface);

    TRACE("iface %p.\n", iface);

    return style->desc.startCap;
}

static D2D1_CAP_STYLE STDMETHODCALLTYPE d2d_stroke_style_GetEndCap(ID2D1StrokeStyle1 *iface)
{
    struct d2d_stroke_style *style = impl_from_ID2D1StrokeStyle1(iface);

    TRACE("iface %p.\n", iface);

    return style->desc.endCap;
}

static D2D1_CAP_STYLE STDMETHODCALLTYPE d2d_stroke_style_GetDashCap(ID2D1StrokeStyle1 *iface)
{
    struct d2d_stroke_style *style = impl_from_ID2D1StrokeStyle1(iface);

    TRACE("iface %p.\n", iface);

    return style->desc.dashCap;
}

static float STDMETHODCALLTYPE d2d_stroke_style_GetMiterLimit(ID2D1StrokeStyle1 *iface)
{
    struct d2d_stroke_style *style = impl_from_ID2D1StrokeStyle1(iface);

    TRACE("iface %p.\n", iface);

    return style->desc.miterLimit;
}

static D2D1_LINE_JOIN STDMETHODCALLTYPE d2d_stroke_style_GetLineJoin(ID2D1StrokeStyle1 *iface)
{
    struct d2d_stroke_style *style = impl_from_ID2D1StrokeStyle1(iface);

    TRACE("iface %p.\n", iface);

    return style->desc.lineJoin;
}

static float STDMETHODCALLTYPE d2d_stroke_style_GetDashOffset(ID2D1StrokeStyle1 *iface)
{
    struct d2d_stroke_style *style = impl_from_ID2D1StrokeStyle1(iface);

    TRACE("iface %p.\n", iface);

    return style->desc.dashOffset;
}

static D2D1_DASH_STYLE STDMETHODCALLTYPE d2d_stroke_style_GetDashStyle(ID2D1StrokeStyle1 *iface)
{
    struct d2d_stroke_style *style = impl_from_ID2D1StrokeStyle1(iface);

    TRACE("iface %p.\n", iface);

    return style->desc.dashStyle;
}

static UINT32 STDMETHODCALLTYPE d2d_stroke_style_GetDashesCount(ID2D1StrokeStyle1 *iface)
{
    struct d2d_stroke_style *style = impl_from_ID2D1StrokeStyle1(iface);

    TRACE("iface %p.\n", iface);

    return style->dash_count;
}

static void STDMETHODCALLTYPE d2d_stroke_style_GetDashes(ID2D1StrokeStyle1 *iface, float *dashes, UINT32 dash_count)
{
    struct d2d_stroke_style *style = impl_from_ID2D1StrokeStyle1(iface);

    TRACE("iface %p, dashes %p, count %u.\n", iface, dashes, dash_count);

    memcpy(dashes, style->dashes, min(style->dash_count, dash_count) * sizeof(*dashes));
    if (dash_count > style->dash_count)
        memset(dashes + style->dash_count, 0, (dash_count - style->dash_count) * sizeof(*dashes));
}

static D2D1_STROKE_TRANSFORM_TYPE STDMETHODCALLTYPE d2d_stroke_style_GetStrokeTransformType(ID2D1StrokeStyle1 *iface)
{
    struct d2d_stroke_style *style = impl_from_ID2D1StrokeStyle1(iface);

    TRACE("iface %p.\n", iface);

    return style->desc.transformType;
}

static const struct ID2D1StrokeStyle1Vtbl d2d_stroke_style_vtbl =
{
    d2d_stroke_style_QueryInterface,
    d2d_stroke_style_AddRef,
    d2d_stroke_style_Release,
    d2d_stroke_style_GetFactory,
    d2d_stroke_style_GetStartCap,
    d2d_stroke_style_GetEndCap,
    d2d_stroke_style_GetDashCap,
    d2d_stroke_style_GetMiterLimit,
    d2d_stroke_style_GetLineJoin,
    d2d_stroke_style_GetDashOffset,
    d2d_stroke_style_GetDashStyle,
    d2d_stroke_style_GetDashesCount,
    d2d_stroke_style_GetDashes,
    d2d_stroke_style_GetStrokeTransformType
};

struct d2d_stroke_style *unsafe_impl_from_ID2D1StrokeStyle(ID2D1StrokeStyle *iface)
{
    if (!iface)
        return NULL;
    assert((const struct ID2D1StrokeStyle1Vtbl *)iface->lpVtbl == &d2d_stroke_style_vtbl);
    return CONTAINING_RECORD(iface, struct d2d_stroke_style, ID2D1StrokeStyle1_iface);
}

HRESULT d2d_stroke_style_init(struct d2d_stroke_style *style, ID2D1Factory *factory,
        const D2D1_STROKE_STYLE_PROPERTIES1 *desc, const float *dashes, UINT32 dash_count)
{
    static const struct
    {
        UINT32 dash_count;
        float dashes[6];
    }
    builtin_dash_styles[] =
    {
        /* D2D1_DASH_STYLE_SOLID */        { 0 },
        /* D2D1_DASH_STYLE_DASH */         { 2, {2.0f, 2.0f}},
        /* D2D1_DASH_STYLE_DOT */          { 2, {0.0f, 2.0f}},
        /* D2D1_DASH_STYLE_DASH_DOT */     { 4, {2.0f, 2.0f, 0.0f, 2.0f}},
        /* D2D1_DASH_STYLE_DASH_DOT_DOT */ { 6, {2.0f, 2.0f, 0.0f, 2.0f, 0.0f, 2.0f}},
    };

    if (desc->dashStyle > D2D1_DASH_STYLE_CUSTOM)
        return E_INVALIDARG;

    if (desc->transformType != D2D1_STROKE_TRANSFORM_TYPE_NORMAL)
        FIXME("transformType %d is not supported\n", desc->transformType);

    style->ID2D1StrokeStyle1_iface.lpVtbl = &d2d_stroke_style_vtbl;
    style->refcount = 1;

    if (desc->dashStyle == D2D1_DASH_STYLE_CUSTOM)
    {
        if (!dashes || !dash_count)
            return E_INVALIDARG;

        if (!(style->dashes = calloc(dash_count, sizeof(*style->dashes))))
            return E_OUTOFMEMORY;
        memcpy(style->dashes, dashes, dash_count * sizeof(*style->dashes));
        style->dash_count = dash_count;
    }
    else
    {
        if (dashes)
            return E_INVALIDARG;

        style->dashes = (float *)builtin_dash_styles[desc->dashStyle].dashes;
        style->dash_count = builtin_dash_styles[desc->dashStyle].dash_count;
    }

    ID2D1Factory_AddRef(style->factory = factory);
    style->desc = *desc;

    return S_OK;
}
