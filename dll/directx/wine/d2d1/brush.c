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

static inline struct d2d_gradient *impl_from_ID2D1GradientStopCollection(ID2D1GradientStopCollection *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_gradient, ID2D1GradientStopCollection_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_gradient_QueryInterface(ID2D1GradientStopCollection *iface,
        REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1GradientStopCollection)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1GradientStopCollection_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_gradient_AddRef(ID2D1GradientStopCollection *iface)
{
    struct d2d_gradient *gradient = impl_from_ID2D1GradientStopCollection(iface);
    ULONG refcount = InterlockedIncrement(&gradient->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_gradient_Release(ID2D1GradientStopCollection *iface)
{
    struct d2d_gradient *gradient = impl_from_ID2D1GradientStopCollection(iface);
    ULONG refcount = InterlockedDecrement(&gradient->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        free(gradient->stops);
        ID3D11ShaderResourceView_Release(gradient->view);
        ID2D1Factory_Release(gradient->factory);
        free(gradient);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d2d_gradient_GetFactory(ID2D1GradientStopCollection *iface, ID2D1Factory **factory)
{
    struct d2d_gradient *gradient = impl_from_ID2D1GradientStopCollection(iface);

    TRACE("iface %p, factory %p.\n", iface, factory);

    ID2D1Factory_AddRef(*factory = gradient->factory);
}

static UINT32 STDMETHODCALLTYPE d2d_gradient_GetGradientStopCount(ID2D1GradientStopCollection *iface)
{
    struct d2d_gradient *gradient = impl_from_ID2D1GradientStopCollection(iface);

    TRACE("iface %p.\n", iface);

    return gradient->stop_count;
}

static void STDMETHODCALLTYPE d2d_gradient_GetGradientStops(ID2D1GradientStopCollection *iface,
        D2D1_GRADIENT_STOP *stops, UINT32 stop_count)
{
    struct d2d_gradient *gradient = impl_from_ID2D1GradientStopCollection(iface);

    TRACE("iface %p, stops %p, stop_count %u.\n", iface, stops, stop_count);

    memcpy(stops, gradient->stops, min(gradient->stop_count, stop_count) * sizeof(*stops));
}

static D2D1_GAMMA STDMETHODCALLTYPE d2d_gradient_GetColorInterpolationGamma(ID2D1GradientStopCollection *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D2D1_GAMMA_1_0;
}

static D2D1_EXTEND_MODE STDMETHODCALLTYPE d2d_gradient_GetExtendMode(ID2D1GradientStopCollection *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D2D1_EXTEND_MODE_CLAMP;
}

static const struct ID2D1GradientStopCollectionVtbl d2d_gradient_vtbl =
{
    d2d_gradient_QueryInterface,
    d2d_gradient_AddRef,
    d2d_gradient_Release,
    d2d_gradient_GetFactory,
    d2d_gradient_GetGradientStopCount,
    d2d_gradient_GetGradientStops,
    d2d_gradient_GetColorInterpolationGamma,
    d2d_gradient_GetExtendMode,
};

HRESULT d2d_gradient_create(ID2D1Factory *factory, ID3D11Device1 *device, const D2D1_GRADIENT_STOP *stops,
        UINT32 stop_count, D2D1_GAMMA gamma, D2D1_EXTEND_MODE extend_mode, struct d2d_gradient **out)
{
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
    D3D11_SUBRESOURCE_DATA buffer_data;
    ID3D11ShaderResourceView *view;
    struct d2d_gradient *gradient;
    D3D11_BUFFER_DESC buffer_desc;
    struct d2d_vec4 *data;
    ID3D11Buffer *buffer;
    unsigned int i;
    HRESULT hr;

    *out = NULL;
    if (!(data = calloc(stop_count, 2 * sizeof(*data))))
    {
        ERR("Failed to allocate data.\n");
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < stop_count; ++i)
    {
        data[i * 2].x = stops[i].position;
        data[i * 2 + 1].x = stops[i].color.r;
        data[i * 2 + 1].y = stops[i].color.g;
        data[i * 2 + 1].z = stops[i].color.b;
        data[i * 2 + 1].w = stops[i].color.a;
    }

    buffer_desc.ByteWidth = 2 * stop_count * sizeof(*data);
    buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    buffer_desc.CPUAccessFlags = 0;
    buffer_desc.MiscFlags = 0;

    buffer_data.pSysMem = data;
    buffer_data.SysMemPitch = 0;
    buffer_data.SysMemSlicePitch = 0;

    hr = ID3D11Device1_CreateBuffer(device, &buffer_desc, &buffer_data, &buffer);
    free(data);
    if (FAILED(hr))
    {
        ERR("Failed to create buffer, hr %#lx.\n", hr);
        return hr;
    }

    srv_desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srv_desc.Buffer.ElementOffset = 0;
    srv_desc.Buffer.ElementWidth = 2 * stop_count;

    hr = ID3D11Device1_CreateShaderResourceView(device, (ID3D11Resource *)buffer, &srv_desc, &view);
    ID3D11Buffer_Release(buffer);
    if (FAILED(hr))
    {
        ERR("Failed to create view, hr %#lx.\n", hr);
        return hr;
    }

    if (!(gradient = calloc(1, sizeof(*gradient))))
    {
        ID3D11ShaderResourceView_Release(view);
        return E_OUTOFMEMORY;
    }

    if (gamma != D2D1_GAMMA_2_2)
        FIXME("Ignoring gamma %#x.\n", gamma);
    if (extend_mode != D2D1_EXTEND_MODE_CLAMP)
        FIXME("Ignoring extend mode %#x.\n", extend_mode);

    gradient->ID2D1GradientStopCollection_iface.lpVtbl = &d2d_gradient_vtbl;
    gradient->refcount = 1;
    ID2D1Factory_AddRef(gradient->factory = factory);
    gradient->view = view;

    gradient->stop_count = stop_count;
    if (!(gradient->stops = calloc(stop_count, sizeof(*stops))))
    {
        ID3D11ShaderResourceView_Release(view);
        free(gradient);
        return E_OUTOFMEMORY;
    }
    memcpy(gradient->stops, stops, stop_count * sizeof(*stops));

    TRACE("Created gradient %p.\n", gradient);
    *out = gradient;
    return S_OK;
}

static struct d2d_gradient *unsafe_impl_from_ID2D1GradientStopCollection(ID2D1GradientStopCollection *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d2d_gradient_vtbl);
    return CONTAINING_RECORD(iface, struct d2d_gradient, ID2D1GradientStopCollection_iface);
}

static void d2d_gradient_bind(struct d2d_gradient *gradient, ID3D11Device1 *device, unsigned int brush_idx)
{
    ID3D11DeviceContext *context;
    ID3D11Device1_GetImmediateContext(device, &context);
    ID3D11DeviceContext_PSSetShaderResources(context, 2 + brush_idx, 1, &gradient->view);
    ID3D11DeviceContext_Release(context);
}

static void d2d_brush_destroy(struct d2d_brush *brush)
{
    ID2D1Factory_Release(brush->factory);
    free(brush);
}

static void d2d_brush_init(struct d2d_brush *brush, ID2D1Factory *factory,
        enum d2d_brush_type type, const D2D1_BRUSH_PROPERTIES *desc, const struct ID2D1BrushVtbl *vtbl)
{
    static const D2D1_MATRIX_3X2_F identity =
    {{{
        1.0f, 0.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
    }}};

    brush->ID2D1Brush_iface.lpVtbl = vtbl;
    brush->refcount = 1;
    ID2D1Factory_AddRef(brush->factory = factory);
    brush->opacity = desc ? desc->opacity : 1.0f;
    brush->transform = desc ? desc->transform : identity;
    brush->type = type;
}

static inline struct d2d_brush *impl_from_ID2D1SolidColorBrush(ID2D1SolidColorBrush *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_brush, ID2D1Brush_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_solid_color_brush_QueryInterface(ID2D1SolidColorBrush *iface,
        REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1SolidColorBrush)
            || IsEqualGUID(iid, &IID_ID2D1Brush)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1SolidColorBrush_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_solid_color_brush_AddRef(ID2D1SolidColorBrush *iface)
{
    struct d2d_brush *brush = impl_from_ID2D1SolidColorBrush(iface);
    ULONG refcount = InterlockedIncrement(&brush->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_solid_color_brush_Release(ID2D1SolidColorBrush *iface)
{
    struct d2d_brush *brush = impl_from_ID2D1SolidColorBrush(iface);
    ULONG refcount = InterlockedDecrement(&brush->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
        d2d_brush_destroy(brush);

    return refcount;
}

static void STDMETHODCALLTYPE d2d_solid_color_brush_GetFactory(ID2D1SolidColorBrush *iface, ID2D1Factory **factory)
{
    struct d2d_brush *brush = impl_from_ID2D1SolidColorBrush(iface);

    TRACE("iface %p, factory %p.\n", iface, factory);

    ID2D1Factory_AddRef(*factory = brush->factory);
}

static void STDMETHODCALLTYPE d2d_solid_color_brush_SetOpacity(ID2D1SolidColorBrush *iface, float opacity)
{
    struct d2d_brush *brush = impl_from_ID2D1SolidColorBrush(iface);

    TRACE("iface %p, opacity %.8e.\n", iface, opacity);

    brush->opacity = opacity;
}

static void STDMETHODCALLTYPE d2d_solid_color_brush_SetTransform(ID2D1SolidColorBrush *iface,
        const D2D1_MATRIX_3X2_F *transform)
{
    struct d2d_brush *brush = impl_from_ID2D1SolidColorBrush(iface);

    TRACE("iface %p, transform %p.\n", iface, transform);

    brush->transform = *transform;
}

static float STDMETHODCALLTYPE d2d_solid_color_brush_GetOpacity(ID2D1SolidColorBrush *iface)
{
    struct d2d_brush *brush = impl_from_ID2D1SolidColorBrush(iface);

    TRACE("iface %p.\n", iface);

    return brush->opacity;
}

static void STDMETHODCALLTYPE d2d_solid_color_brush_GetTransform(ID2D1SolidColorBrush *iface,
        D2D1_MATRIX_3X2_F *transform)
{
    struct d2d_brush *brush = impl_from_ID2D1SolidColorBrush(iface);

    TRACE("iface %p, transform %p.\n", iface, transform);

    *transform = brush->transform;
}

static void STDMETHODCALLTYPE d2d_solid_color_brush_SetColor(ID2D1SolidColorBrush *iface, const D2D1_COLOR_F *color)
{
    struct d2d_brush *brush = impl_from_ID2D1SolidColorBrush(iface);

    TRACE("iface %p, color %p.\n", iface, color);

    brush->u.solid.color = *color;
}

static D2D1_COLOR_F * STDMETHODCALLTYPE d2d_solid_color_brush_GetColor(ID2D1SolidColorBrush *iface, D2D1_COLOR_F *color)
{
    struct d2d_brush *brush = impl_from_ID2D1SolidColorBrush(iface);

    TRACE("iface %p, color %p.\n", iface, color);

    *color = brush->u.solid.color;
    return color;
}

static const struct ID2D1SolidColorBrushVtbl d2d_solid_color_brush_vtbl =
{
    d2d_solid_color_brush_QueryInterface,
    d2d_solid_color_brush_AddRef,
    d2d_solid_color_brush_Release,
    d2d_solid_color_brush_GetFactory,
    d2d_solid_color_brush_SetOpacity,
    d2d_solid_color_brush_SetTransform,
    d2d_solid_color_brush_GetOpacity,
    d2d_solid_color_brush_GetTransform,
    d2d_solid_color_brush_SetColor,
    d2d_solid_color_brush_GetColor,
};

HRESULT d2d_solid_color_brush_create(ID2D1Factory *factory, const D2D1_COLOR_F *color,
        const D2D1_BRUSH_PROPERTIES *desc, struct d2d_brush **brush)
{
    if (!(*brush = calloc(1, sizeof(**brush))))
        return E_OUTOFMEMORY;

    d2d_brush_init(*brush, factory, D2D_BRUSH_TYPE_SOLID, desc,
            (ID2D1BrushVtbl *)&d2d_solid_color_brush_vtbl);
    (*brush)->u.solid.color = *color;

    TRACE("Created brush %p.\n", *brush);
    return S_OK;
}

static inline struct d2d_brush *impl_from_ID2D1LinearGradientBrush(ID2D1LinearGradientBrush *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_brush, ID2D1Brush_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_linear_gradient_brush_QueryInterface(ID2D1LinearGradientBrush *iface,
        REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1LinearGradientBrush)
            || IsEqualGUID(iid, &IID_ID2D1Brush)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1LinearGradientBrush_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_linear_gradient_brush_AddRef(ID2D1LinearGradientBrush *iface)
{
    struct d2d_brush *brush = impl_from_ID2D1LinearGradientBrush(iface);
    ULONG refcount = InterlockedIncrement(&brush->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_linear_gradient_brush_Release(ID2D1LinearGradientBrush *iface)
{
    struct d2d_brush *brush = impl_from_ID2D1LinearGradientBrush(iface);
    ULONG refcount = InterlockedDecrement(&brush->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        ID2D1GradientStopCollection_Release(&brush->u.linear.gradient->ID2D1GradientStopCollection_iface);
        d2d_brush_destroy(brush);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d2d_linear_gradient_brush_GetFactory(ID2D1LinearGradientBrush *iface,
        ID2D1Factory **factory)
{
    struct d2d_brush *brush = impl_from_ID2D1LinearGradientBrush(iface);

    TRACE("iface %p, factory %p.\n", iface, factory);

    ID2D1Factory_AddRef(*factory = brush->factory);
}

static void STDMETHODCALLTYPE d2d_linear_gradient_brush_SetOpacity(ID2D1LinearGradientBrush *iface, float opacity)
{
    struct d2d_brush *brush = impl_from_ID2D1LinearGradientBrush(iface);

    TRACE("iface %p, opacity %.8e.\n", iface, opacity);

    brush->opacity = opacity;
}

static void STDMETHODCALLTYPE d2d_linear_gradient_brush_SetTransform(ID2D1LinearGradientBrush *iface,
        const D2D1_MATRIX_3X2_F *transform)
{
    struct d2d_brush *brush = impl_from_ID2D1LinearGradientBrush(iface);

    TRACE("iface %p, transform %p.\n", iface, transform);

    brush->transform = *transform;
}

static float STDMETHODCALLTYPE d2d_linear_gradient_brush_GetOpacity(ID2D1LinearGradientBrush *iface)
{
    struct d2d_brush *brush = impl_from_ID2D1LinearGradientBrush(iface);

    TRACE("iface %p.\n", iface);

    return brush->opacity;
}

static void STDMETHODCALLTYPE d2d_linear_gradient_brush_GetTransform(ID2D1LinearGradientBrush *iface,
        D2D1_MATRIX_3X2_F *transform)
{
    struct d2d_brush *brush = impl_from_ID2D1LinearGradientBrush(iface);

    TRACE("iface %p, transform %p.\n", iface, transform);

    *transform = brush->transform;
}

static void STDMETHODCALLTYPE d2d_linear_gradient_brush_SetStartPoint(ID2D1LinearGradientBrush *iface,
        D2D1_POINT_2F start_point)
{
    struct d2d_brush *brush = impl_from_ID2D1LinearGradientBrush(iface);

    TRACE("iface %p, start_point %s.\n", iface, debug_d2d_point_2f(&start_point));

    brush->u.linear.start = start_point;
}

static void STDMETHODCALLTYPE d2d_linear_gradient_brush_SetEndPoint(ID2D1LinearGradientBrush *iface,
        D2D1_POINT_2F end_point)
{
    struct d2d_brush *brush = impl_from_ID2D1LinearGradientBrush(iface);

    TRACE("iface %p, end_point %s.\n", iface, debug_d2d_point_2f(&end_point));

    brush->u.linear.end = end_point;
}

static D2D1_POINT_2F * STDMETHODCALLTYPE d2d_linear_gradient_brush_GetStartPoint(ID2D1LinearGradientBrush *iface,
        D2D1_POINT_2F *point)
{
    struct d2d_brush *brush = impl_from_ID2D1LinearGradientBrush(iface);

    TRACE("iface %p, point %p.\n", iface, point);

    *point = brush->u.linear.start;
    return point;
}

static D2D1_POINT_2F * STDMETHODCALLTYPE d2d_linear_gradient_brush_GetEndPoint(ID2D1LinearGradientBrush *iface,
        D2D1_POINT_2F *point)
{
    struct d2d_brush *brush = impl_from_ID2D1LinearGradientBrush(iface);

    TRACE("iface %p, point %p.\n", iface, point);

    *point = brush->u.linear.end;
    return point;
}

static void STDMETHODCALLTYPE d2d_linear_gradient_brush_GetGradientStopCollection(ID2D1LinearGradientBrush *iface,
        ID2D1GradientStopCollection **gradient)
{
    struct d2d_brush *brush = impl_from_ID2D1LinearGradientBrush(iface);

    TRACE("iface %p, gradient %p.\n", iface, gradient);

    ID2D1GradientStopCollection_AddRef(*gradient = &brush->u.linear.gradient->ID2D1GradientStopCollection_iface);
}

static const struct ID2D1LinearGradientBrushVtbl d2d_linear_gradient_brush_vtbl =
{
    d2d_linear_gradient_brush_QueryInterface,
    d2d_linear_gradient_brush_AddRef,
    d2d_linear_gradient_brush_Release,
    d2d_linear_gradient_brush_GetFactory,
    d2d_linear_gradient_brush_SetOpacity,
    d2d_linear_gradient_brush_SetTransform,
    d2d_linear_gradient_brush_GetOpacity,
    d2d_linear_gradient_brush_GetTransform,
    d2d_linear_gradient_brush_SetStartPoint,
    d2d_linear_gradient_brush_SetEndPoint,
    d2d_linear_gradient_brush_GetStartPoint,
    d2d_linear_gradient_brush_GetEndPoint,
    d2d_linear_gradient_brush_GetGradientStopCollection,
};

HRESULT d2d_linear_gradient_brush_create(ID2D1Factory *factory,
        const D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES *gradient_brush_desc, const D2D1_BRUSH_PROPERTIES *brush_desc,
        ID2D1GradientStopCollection *gradient, struct d2d_brush **brush)
{
    if (!(*brush = calloc(1, sizeof(**brush))))
        return E_OUTOFMEMORY;

    d2d_brush_init(*brush, factory, D2D_BRUSH_TYPE_LINEAR, brush_desc,
            (ID2D1BrushVtbl *)&d2d_linear_gradient_brush_vtbl);
    (*brush)->u.linear.gradient = unsafe_impl_from_ID2D1GradientStopCollection(gradient);
    ID2D1GradientStopCollection_AddRef(&(*brush)->u.linear.gradient->ID2D1GradientStopCollection_iface);
    (*brush)->u.linear.start = gradient_brush_desc->startPoint;
    (*brush)->u.linear.end = gradient_brush_desc->endPoint;

    TRACE("Created brush %p.\n", *brush);
    return S_OK;
}

static inline struct d2d_brush *impl_from_ID2D1RadialGradientBrush(ID2D1RadialGradientBrush *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_brush, ID2D1Brush_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_radial_gradient_brush_QueryInterface(ID2D1RadialGradientBrush *iface,
        REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1RadialGradientBrush)
            || IsEqualGUID(iid, &IID_ID2D1Brush)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1RadialGradientBrush_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_radial_gradient_brush_AddRef(ID2D1RadialGradientBrush *iface)
{
    struct d2d_brush *brush = impl_from_ID2D1RadialGradientBrush(iface);
    ULONG refcount = InterlockedIncrement(&brush->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_radial_gradient_brush_Release(ID2D1RadialGradientBrush *iface)
{
    struct d2d_brush *brush = impl_from_ID2D1RadialGradientBrush(iface);
    ULONG refcount = InterlockedDecrement(&brush->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        ID2D1GradientStopCollection_Release(&brush->u.radial.gradient->ID2D1GradientStopCollection_iface);
        d2d_brush_destroy(brush);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d2d_radial_gradient_brush_GetFactory(ID2D1RadialGradientBrush *iface,
        ID2D1Factory **factory)
{
    struct d2d_brush *brush = impl_from_ID2D1RadialGradientBrush(iface);

    TRACE("iface %p, factory %p.\n", iface, factory);

    ID2D1Factory_AddRef(*factory = brush->factory);
}

static void STDMETHODCALLTYPE d2d_radial_gradient_brush_SetOpacity(ID2D1RadialGradientBrush *iface, float opacity)
{
    struct d2d_brush *brush = impl_from_ID2D1RadialGradientBrush(iface);

    TRACE("iface %p, opacity %.8e.\n", iface, opacity);

    brush->opacity = opacity;
}

static void STDMETHODCALLTYPE d2d_radial_gradient_brush_SetTransform(ID2D1RadialGradientBrush *iface,
        const D2D1_MATRIX_3X2_F *transform)
{
    struct d2d_brush *brush = impl_from_ID2D1RadialGradientBrush(iface);

    TRACE("iface %p, transform %p.\n", iface, transform);

    brush->transform = *transform;
}

static float STDMETHODCALLTYPE d2d_radial_gradient_brush_GetOpacity(ID2D1RadialGradientBrush *iface)
{
    struct d2d_brush *brush = impl_from_ID2D1RadialGradientBrush(iface);

    TRACE("iface %p.\n", iface);

    return brush->opacity;
}

static void STDMETHODCALLTYPE d2d_radial_gradient_brush_GetTransform(ID2D1RadialGradientBrush *iface,
        D2D1_MATRIX_3X2_F *transform)
{
    struct d2d_brush *brush = impl_from_ID2D1RadialGradientBrush(iface);

    TRACE("iface %p, transform %p.\n", iface, transform);

    *transform = brush->transform;
}

static void STDMETHODCALLTYPE d2d_radial_gradient_brush_SetCenter(ID2D1RadialGradientBrush *iface,
        D2D1_POINT_2F centre)
{
    struct d2d_brush *brush = impl_from_ID2D1RadialGradientBrush(iface);

    TRACE("iface %p, centre %s.\n", iface, debug_d2d_point_2f(&centre));

    brush->u.radial.centre = centre;
}

static void STDMETHODCALLTYPE d2d_radial_gradient_brush_SetGradientOriginOffset(ID2D1RadialGradientBrush *iface,
        D2D1_POINT_2F offset)
{
    struct d2d_brush *brush = impl_from_ID2D1RadialGradientBrush(iface);

    TRACE("iface %p, offset %s.\n", iface, debug_d2d_point_2f(&offset));

    brush->u.radial.offset = offset;
}

static void STDMETHODCALLTYPE d2d_radial_gradient_brush_SetRadiusX(ID2D1RadialGradientBrush *iface, float radius)
{
    struct d2d_brush *brush = impl_from_ID2D1RadialGradientBrush(iface);

    TRACE("iface %p, radius %.8e.\n", iface, radius);

    brush->u.radial.radius.x = radius;
}

static void STDMETHODCALLTYPE d2d_radial_gradient_brush_SetRadiusY(ID2D1RadialGradientBrush *iface, float radius)
{
    struct d2d_brush *brush = impl_from_ID2D1RadialGradientBrush(iface);

    TRACE("iface %p, radius %.8e.\n", iface, radius);

    brush->u.radial.radius.y = radius;
}

static D2D1_POINT_2F * STDMETHODCALLTYPE d2d_radial_gradient_brush_GetCenter(ID2D1RadialGradientBrush *iface,
        D2D1_POINT_2F *centre)
{
    struct d2d_brush *brush = impl_from_ID2D1RadialGradientBrush(iface);

    TRACE("iface %p, centre %p.\n", iface, centre);

    *centre = brush->u.radial.centre;
    return centre;
}

static D2D1_POINT_2F * STDMETHODCALLTYPE d2d_radial_gradient_brush_GetGradientOriginOffset(
        ID2D1RadialGradientBrush *iface, D2D1_POINT_2F *offset)
{
    struct d2d_brush *brush = impl_from_ID2D1RadialGradientBrush(iface);

    TRACE("iface %p, offset %p.\n", iface, offset);

    *offset = brush->u.radial.offset;
    return offset;
}

static float STDMETHODCALLTYPE d2d_radial_gradient_brush_GetRadiusX(ID2D1RadialGradientBrush *iface)
{
    struct d2d_brush *brush = impl_from_ID2D1RadialGradientBrush(iface);

    TRACE("iface %p.\n", iface);

    return brush->u.radial.radius.x;
}

static float STDMETHODCALLTYPE d2d_radial_gradient_brush_GetRadiusY(ID2D1RadialGradientBrush *iface)
{
    struct d2d_brush *brush = impl_from_ID2D1RadialGradientBrush(iface);

    TRACE("iface %p.\n", iface);

    return brush->u.radial.radius.y;
}

static void STDMETHODCALLTYPE d2d_radial_gradient_brush_GetGradientStopCollection(ID2D1RadialGradientBrush *iface,
        ID2D1GradientStopCollection **gradient)
{
    struct d2d_brush *brush = impl_from_ID2D1RadialGradientBrush(iface);

    TRACE("iface %p, gradient %p.\n", iface, gradient);

    ID2D1GradientStopCollection_AddRef(*gradient = &brush->u.radial.gradient->ID2D1GradientStopCollection_iface);
}

static const struct ID2D1RadialGradientBrushVtbl d2d_radial_gradient_brush_vtbl =
{
    d2d_radial_gradient_brush_QueryInterface,
    d2d_radial_gradient_brush_AddRef,
    d2d_radial_gradient_brush_Release,
    d2d_radial_gradient_brush_GetFactory,
    d2d_radial_gradient_brush_SetOpacity,
    d2d_radial_gradient_brush_SetTransform,
    d2d_radial_gradient_brush_GetOpacity,
    d2d_radial_gradient_brush_GetTransform,
    d2d_radial_gradient_brush_SetCenter,
    d2d_radial_gradient_brush_SetGradientOriginOffset,
    d2d_radial_gradient_brush_SetRadiusX,
    d2d_radial_gradient_brush_SetRadiusY,
    d2d_radial_gradient_brush_GetCenter,
    d2d_radial_gradient_brush_GetGradientOriginOffset,
    d2d_radial_gradient_brush_GetRadiusX,
    d2d_radial_gradient_brush_GetRadiusY,
    d2d_radial_gradient_brush_GetGradientStopCollection,
};

HRESULT d2d_radial_gradient_brush_create(ID2D1Factory *factory,
        const D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES *gradient_desc, const D2D1_BRUSH_PROPERTIES *brush_desc,
        ID2D1GradientStopCollection *gradient, struct d2d_brush **brush)
{
    struct d2d_brush *b;

    if (!(b = calloc(1, sizeof(*b))))
        return E_OUTOFMEMORY;

    d2d_brush_init(b, factory, D2D_BRUSH_TYPE_RADIAL, brush_desc, (ID2D1BrushVtbl *)&d2d_radial_gradient_brush_vtbl);
    b->u.radial.gradient = unsafe_impl_from_ID2D1GradientStopCollection(gradient);
    ID2D1GradientStopCollection_AddRef(&b->u.radial.gradient->ID2D1GradientStopCollection_iface);
    b->u.radial.centre = gradient_desc->center;
    b->u.radial.offset = gradient_desc->gradientOriginOffset;
    b->u.radial.radius.x = gradient_desc->radiusX;
    b->u.radial.radius.y = gradient_desc->radiusY;

    TRACE("Created brush %p.\n", b);
    *brush = b;

    return S_OK;
}

static inline struct d2d_brush *impl_from_ID2D1BitmapBrush1(ID2D1BitmapBrush1 *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_brush, ID2D1Brush_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_bitmap_brush_QueryInterface(ID2D1BitmapBrush1 *iface,
        REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1BitmapBrush1)
            || IsEqualGUID(iid, &IID_ID2D1BitmapBrush)
            || IsEqualGUID(iid, &IID_ID2D1Brush)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1BitmapBrush1_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_bitmap_brush_AddRef(ID2D1BitmapBrush1 *iface)
{
    struct d2d_brush *brush = impl_from_ID2D1BitmapBrush1(iface);
    ULONG refcount = InterlockedIncrement(&brush->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_bitmap_brush_Release(ID2D1BitmapBrush1 *iface)
{
    struct d2d_brush *brush = impl_from_ID2D1BitmapBrush1(iface);
    ULONG refcount = InterlockedDecrement(&brush->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (brush->u.bitmap.bitmap)
            ID2D1Bitmap1_Release(&brush->u.bitmap.bitmap->ID2D1Bitmap1_iface);
        d2d_brush_destroy(brush);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d2d_bitmap_brush_GetFactory(ID2D1BitmapBrush1 *iface,
        ID2D1Factory **factory)
{
    struct d2d_brush *brush = impl_from_ID2D1BitmapBrush1(iface);

    TRACE("iface %p, factory %p.\n", iface, factory);

    ID2D1Factory_AddRef(*factory = brush->factory);
}

static void STDMETHODCALLTYPE d2d_bitmap_brush_SetOpacity(ID2D1BitmapBrush1 *iface, float opacity)
{
    struct d2d_brush *brush = impl_from_ID2D1BitmapBrush1(iface);

    TRACE("iface %p, opacity %.8e.\n", iface, opacity);

    brush->opacity = opacity;
}

static void STDMETHODCALLTYPE d2d_bitmap_brush_SetTransform(ID2D1BitmapBrush1 *iface,
        const D2D1_MATRIX_3X2_F *transform)
{
    struct d2d_brush *brush = impl_from_ID2D1BitmapBrush1(iface);

    TRACE("iface %p, transform %p.\n", iface, transform);

    brush->transform = *transform;
}

static float STDMETHODCALLTYPE d2d_bitmap_brush_GetOpacity(ID2D1BitmapBrush1 *iface)
{
    struct d2d_brush *brush = impl_from_ID2D1BitmapBrush1(iface);

    TRACE("iface %p.\n", iface);

    return brush->opacity;
}

static void STDMETHODCALLTYPE d2d_bitmap_brush_GetTransform(ID2D1BitmapBrush1 *iface,
        D2D1_MATRIX_3X2_F *transform)
{
    struct d2d_brush *brush = impl_from_ID2D1BitmapBrush1(iface);

    TRACE("iface %p, transform %p.\n", iface, transform);

    *transform = brush->transform;
}

static void STDMETHODCALLTYPE d2d_bitmap_brush_SetExtendModeX(ID2D1BitmapBrush1 *iface, D2D1_EXTEND_MODE mode)
{
    struct d2d_brush *brush = impl_from_ID2D1BitmapBrush1(iface);

    TRACE("iface %p, mode %#x.\n", iface, mode);

    brush->u.bitmap.extend_mode_x = mode;
}

static void STDMETHODCALLTYPE d2d_bitmap_brush_SetExtendModeY(ID2D1BitmapBrush1 *iface, D2D1_EXTEND_MODE mode)
{
    struct d2d_brush *brush = impl_from_ID2D1BitmapBrush1(iface);

    TRACE("iface %p, mode %#x.\n", iface, mode);

    brush->u.bitmap.extend_mode_y = mode;
}

static void STDMETHODCALLTYPE d2d_bitmap_brush_SetInterpolationMode(ID2D1BitmapBrush1 *iface,
        D2D1_BITMAP_INTERPOLATION_MODE mode)
{
    struct d2d_brush *brush = impl_from_ID2D1BitmapBrush1(iface);

    TRACE("iface %p, mode %#x.\n", iface, mode);

    switch (mode)
    {
        case D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR:
        case D2D1_BITMAP_INTERPOLATION_MODE_LINEAR:
            break;
        default:
            WARN("Unknown interpolation mode %#x.\n", mode);
            return;
    }

    brush->u.bitmap.interpolation_mode = d2d1_1_interp_mode_from_d2d1(mode);
}

static void STDMETHODCALLTYPE d2d_bitmap_brush_SetBitmap(ID2D1BitmapBrush1 *iface, ID2D1Bitmap *bitmap)
{
    struct d2d_brush *brush = impl_from_ID2D1BitmapBrush1(iface);

    TRACE("iface %p, bitmap %p.\n", iface, bitmap);

    if (bitmap)
        ID2D1Bitmap_AddRef(bitmap);
    if (brush->u.bitmap.bitmap)
        ID2D1Bitmap1_Release(&brush->u.bitmap.bitmap->ID2D1Bitmap1_iface);
    brush->u.bitmap.bitmap = unsafe_impl_from_ID2D1Bitmap(bitmap);
}

static D2D1_EXTEND_MODE STDMETHODCALLTYPE d2d_bitmap_brush_GetExtendModeX(ID2D1BitmapBrush1 *iface)
{
    struct d2d_brush *brush = impl_from_ID2D1BitmapBrush1(iface);

    TRACE("iface %p.\n", iface);

    return brush->u.bitmap.extend_mode_x;
}

static D2D1_EXTEND_MODE STDMETHODCALLTYPE d2d_bitmap_brush_GetExtendModeY(ID2D1BitmapBrush1 *iface)
{
    struct d2d_brush *brush = impl_from_ID2D1BitmapBrush1(iface);

    TRACE("iface %p.\n", iface);

    return brush->u.bitmap.extend_mode_y;
}

static D2D1_BITMAP_INTERPOLATION_MODE STDMETHODCALLTYPE d2d_bitmap_brush_GetInterpolationMode(ID2D1BitmapBrush1 *iface)
{
    struct d2d_brush *brush = impl_from_ID2D1BitmapBrush1(iface);

    TRACE("iface %p.\n", iface);

    switch (brush->u.bitmap.interpolation_mode)
    {
        case D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR:
            return D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;
        case D2D1_INTERPOLATION_MODE_LINEAR:
        default:
            return D2D1_BITMAP_INTERPOLATION_MODE_LINEAR;
    }
}

static void STDMETHODCALLTYPE d2d_bitmap_brush_GetBitmap(ID2D1BitmapBrush1 *iface, ID2D1Bitmap **bitmap)
{
    struct d2d_brush *brush = impl_from_ID2D1BitmapBrush1(iface);

    TRACE("iface %p, bitmap %p.\n", iface, bitmap);

    if ((*bitmap = (ID2D1Bitmap *)&brush->u.bitmap.bitmap->ID2D1Bitmap1_iface))
        ID2D1Bitmap_AddRef(*bitmap);
}

static void STDMETHODCALLTYPE d2d_bitmap_brush_SetInterpolationMode1(ID2D1BitmapBrush1 *iface,
        D2D1_INTERPOLATION_MODE mode)
{
    struct d2d_brush *brush = impl_from_ID2D1BitmapBrush1(iface);

    TRACE("iface %p, mode %#x.\n", iface, mode);

    switch (mode)
    {
        case D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR:
        case D2D1_INTERPOLATION_MODE_LINEAR:
            break;
        case D2D1_INTERPOLATION_MODE_CUBIC:
        case D2D1_INTERPOLATION_MODE_MULTI_SAMPLE_LINEAR:
        case D2D1_INTERPOLATION_MODE_ANISOTROPIC:
        case D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC:
            FIXME("Unhandled interpolation mode %#x.\n", mode);
            break;
        default:
            WARN("Unknown interpolation mode %#x.\n", mode);
            return;
    }

    brush->u.bitmap.interpolation_mode = mode;
}

static D2D1_INTERPOLATION_MODE STDMETHODCALLTYPE d2d_bitmap_brush_GetInterpolationMode1(ID2D1BitmapBrush1 *iface)
{
    struct d2d_brush *brush = impl_from_ID2D1BitmapBrush1(iface);

    TRACE("iface %p.\n", iface);

    return brush->u.bitmap.interpolation_mode;
}

static const struct ID2D1BitmapBrush1Vtbl d2d_bitmap_brush_vtbl =
{
    d2d_bitmap_brush_QueryInterface,
    d2d_bitmap_brush_AddRef,
    d2d_bitmap_brush_Release,
    d2d_bitmap_brush_GetFactory,
    d2d_bitmap_brush_SetOpacity,
    d2d_bitmap_brush_SetTransform,
    d2d_bitmap_brush_GetOpacity,
    d2d_bitmap_brush_GetTransform,
    d2d_bitmap_brush_SetExtendModeX,
    d2d_bitmap_brush_SetExtendModeY,
    d2d_bitmap_brush_SetInterpolationMode,
    d2d_bitmap_brush_SetBitmap,
    d2d_bitmap_brush_GetExtendModeX,
    d2d_bitmap_brush_GetExtendModeY,
    d2d_bitmap_brush_GetInterpolationMode,
    d2d_bitmap_brush_GetBitmap,
    d2d_bitmap_brush_SetInterpolationMode1,
    d2d_bitmap_brush_GetInterpolationMode1,
};

HRESULT d2d_bitmap_brush_create(ID2D1Factory *factory, ID2D1Bitmap *bitmap,
        const D2D1_BITMAP_BRUSH_PROPERTIES1 *bitmap_brush_desc, const D2D1_BRUSH_PROPERTIES *brush_desc,
        struct d2d_brush **brush)
{
    if (!(*brush = calloc(1, sizeof(**brush))))
        return E_OUTOFMEMORY;

    d2d_brush_init(*brush, factory, D2D_BRUSH_TYPE_BITMAP,
            brush_desc, (ID2D1BrushVtbl *)&d2d_bitmap_brush_vtbl);
    if (((*brush)->u.bitmap.bitmap = unsafe_impl_from_ID2D1Bitmap(bitmap)))
        ID2D1Bitmap1_AddRef(&(*brush)->u.bitmap.bitmap->ID2D1Bitmap1_iface);
    if (bitmap_brush_desc)
    {
        (*brush)->u.bitmap.extend_mode_x = bitmap_brush_desc->extendModeX;
        (*brush)->u.bitmap.extend_mode_y = bitmap_brush_desc->extendModeY;
        (*brush)->u.bitmap.interpolation_mode = bitmap_brush_desc->interpolationMode;
    }
    else
    {
        (*brush)->u.bitmap.extend_mode_x = D2D1_EXTEND_MODE_CLAMP;
        (*brush)->u.bitmap.extend_mode_y = D2D1_EXTEND_MODE_CLAMP;
        (*brush)->u.bitmap.interpolation_mode = D2D1_INTERPOLATION_MODE_LINEAR;
    }

    TRACE("Created brush %p.\n", *brush);
    return S_OK;
}

static inline struct d2d_brush *impl_from_ID2D1ImageBrush(ID2D1ImageBrush *iface)
{
    return CONTAINING_RECORD((ID2D1Brush *)iface, struct d2d_brush, ID2D1Brush_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_image_brush_QueryInterface(ID2D1ImageBrush *iface,
        REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1ImageBrush)
            || IsEqualGUID(iid, &IID_ID2D1Brush)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1ImageBrush_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_image_brush_AddRef(ID2D1ImageBrush *iface)
{
    struct d2d_brush *brush = impl_from_ID2D1ImageBrush(iface);
    ULONG refcount = InterlockedIncrement(&brush->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_image_brush_Release(ID2D1ImageBrush *iface)
{
    struct d2d_brush *brush = impl_from_ID2D1ImageBrush(iface);
    ULONG refcount = InterlockedDecrement(&brush->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (brush->u.image.image)
            ID2D1Image_Release(brush->u.image.image);
        d2d_brush_destroy(brush);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d2d_image_brush_GetFactory(ID2D1ImageBrush *iface,
        ID2D1Factory **factory)
{
    struct d2d_brush *brush = impl_from_ID2D1ImageBrush(iface);

    TRACE("iface %p, factory %p.\n", iface, factory);

    ID2D1Factory_AddRef(*factory = brush->factory);
}

static void STDMETHODCALLTYPE d2d_image_brush_SetOpacity(ID2D1ImageBrush *iface, float opacity)
{
    struct d2d_brush *brush = impl_from_ID2D1ImageBrush(iface);

    TRACE("iface %p, opacity %.8e.\n", iface, opacity);

    brush->opacity = opacity;
}

static void STDMETHODCALLTYPE d2d_image_brush_SetTransform(ID2D1ImageBrush *iface,
        const D2D1_MATRIX_3X2_F *transform)
{
    struct d2d_brush *brush = impl_from_ID2D1ImageBrush(iface);

    TRACE("iface %p, transform %p.\n", iface, transform);

    brush->transform = *transform;
}

static float STDMETHODCALLTYPE d2d_image_brush_GetOpacity(ID2D1ImageBrush *iface)
{
    struct d2d_brush *brush = impl_from_ID2D1ImageBrush(iface);

    TRACE("iface %p.\n", iface);

    return brush->opacity;
}

static void STDMETHODCALLTYPE d2d_image_brush_GetTransform(ID2D1ImageBrush *iface,
        D2D1_MATRIX_3X2_F *transform)
{
    struct d2d_brush *brush = impl_from_ID2D1ImageBrush(iface);

    TRACE("iface %p, transform %p.\n", iface, transform);

    *transform = brush->transform;
}

static void STDMETHODCALLTYPE d2d_image_brush_SetImage(ID2D1ImageBrush *iface, ID2D1Image *image)
{
    struct d2d_brush *brush = impl_from_ID2D1ImageBrush(iface);

    TRACE("iface %p, image %p.\n", iface, image);

    if (image)
        ID2D1Image_AddRef(image);
    if (brush->u.image.image)
        ID2D1Image_Release(brush->u.image.image);
    brush->u.image.image = image;
}

static void STDMETHODCALLTYPE d2d_image_brush_SetExtendModeX(ID2D1ImageBrush *iface, D2D1_EXTEND_MODE mode)
{
    struct d2d_brush *brush = impl_from_ID2D1ImageBrush(iface);

    TRACE("iface %p, mode %#x.\n", iface, mode);

    brush->u.image.extend_mode_x = mode;
}

static void STDMETHODCALLTYPE d2d_image_brush_SetExtendModeY(ID2D1ImageBrush *iface, D2D1_EXTEND_MODE mode)
{
    struct d2d_brush *brush = impl_from_ID2D1ImageBrush(iface);

    TRACE("iface %p, mode %#x.\n", iface, mode);

    brush->u.image.extend_mode_y = mode;
}

static void STDMETHODCALLTYPE d2d_image_brush_SetInterpolationMode(ID2D1ImageBrush *iface,
        D2D1_INTERPOLATION_MODE mode)
{
    struct d2d_brush *brush = impl_from_ID2D1ImageBrush(iface);

    TRACE("iface %p, mode %#x.\n", iface, mode);

    brush->u.image.interpolation_mode = mode;
}

static void STDMETHODCALLTYPE d2d_image_brush_SetSourceRectangle(ID2D1ImageBrush *iface, const D2D1_RECT_F *rect)
{
    struct d2d_brush *brush = impl_from_ID2D1ImageBrush(iface);

    TRACE("iface %p, rect %s.\n", iface, debug_d2d_rect_f(rect));

    brush->u.image.source_rect = *rect;
}

static void STDMETHODCALLTYPE d2d_image_brush_GetImage(ID2D1ImageBrush *iface, ID2D1Image **image)
{
    struct d2d_brush *brush = impl_from_ID2D1ImageBrush(iface);

    TRACE("iface %p, image %p.\n", iface, image);

    if ((*image = brush->u.image.image))
        ID2D1Image_AddRef(*image);
}

static D2D1_EXTEND_MODE STDMETHODCALLTYPE d2d_image_brush_GetExtendModeX(ID2D1ImageBrush *iface)
{
    struct d2d_brush *brush = impl_from_ID2D1ImageBrush(iface);

    TRACE("iface %p.\n", iface);

    return brush->u.image.extend_mode_x;
}

static D2D1_EXTEND_MODE STDMETHODCALLTYPE d2d_image_brush_GetExtendModeY(ID2D1ImageBrush *iface)
{
    struct d2d_brush *brush = impl_from_ID2D1ImageBrush(iface);

    TRACE("iface %p.\n", iface);

    return brush->u.image.extend_mode_y;
}

static D2D1_INTERPOLATION_MODE STDMETHODCALLTYPE d2d_image_brush_GetInterpolationMode(ID2D1ImageBrush *iface)
{
    struct d2d_brush *brush = impl_from_ID2D1ImageBrush(iface);

    TRACE("iface %p.\n", iface);

    return brush->u.image.interpolation_mode;
}

static void STDMETHODCALLTYPE d2d_image_brush_GetSourceRectangle(ID2D1ImageBrush *iface, D2D1_RECT_F *rect)
{
    struct d2d_brush *brush = impl_from_ID2D1ImageBrush(iface);

    TRACE("iface %p, rect %p.\n", iface, rect);

    *rect = brush->u.image.source_rect;
}

static const struct ID2D1ImageBrushVtbl d2d_image_brush_vtbl =
{
    d2d_image_brush_QueryInterface,
    d2d_image_brush_AddRef,
    d2d_image_brush_Release,
    d2d_image_brush_GetFactory,
    d2d_image_brush_SetOpacity,
    d2d_image_brush_SetTransform,
    d2d_image_brush_GetOpacity,
    d2d_image_brush_GetTransform,
    d2d_image_brush_SetImage,
    d2d_image_brush_SetExtendModeX,
    d2d_image_brush_SetExtendModeY,
    d2d_image_brush_SetInterpolationMode,
    d2d_image_brush_SetSourceRectangle,
    d2d_image_brush_GetImage,
    d2d_image_brush_GetExtendModeX,
    d2d_image_brush_GetExtendModeY,
    d2d_image_brush_GetInterpolationMode,
    d2d_image_brush_GetSourceRectangle
};

HRESULT d2d_image_brush_create(ID2D1Factory *factory, ID2D1Image *image,
        const D2D1_IMAGE_BRUSH_PROPERTIES *image_brush_desc, const D2D1_BRUSH_PROPERTIES *brush_desc,
        struct d2d_brush **brush)
{
    if (!(*brush = calloc(1, sizeof(**brush))))
        return E_OUTOFMEMORY;

    d2d_brush_init(*brush, factory, D2D_BRUSH_TYPE_IMAGE,
            brush_desc, (ID2D1BrushVtbl *)&d2d_image_brush_vtbl);
    if (((*brush)->u.image.image = image))
        ID2D1Image_AddRef((*brush)->u.image.image);

    (*brush)->u.image.source_rect = image_brush_desc->sourceRectangle;
    (*brush)->u.image.extend_mode_x = image_brush_desc->extendModeX;
    (*brush)->u.image.extend_mode_y = image_brush_desc->extendModeY;
    (*brush)->u.image.interpolation_mode = image_brush_desc->interpolationMode;

    TRACE("Created brush %p.\n", *brush);
    return S_OK;
}

struct d2d_brush *unsafe_impl_from_ID2D1Brush(ID2D1Brush *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == (const ID2D1BrushVtbl *)&d2d_solid_color_brush_vtbl
            || iface->lpVtbl == (const ID2D1BrushVtbl *)&d2d_linear_gradient_brush_vtbl
            || iface->lpVtbl == (const ID2D1BrushVtbl *)&d2d_radial_gradient_brush_vtbl
            || iface->lpVtbl == (const ID2D1BrushVtbl *)&d2d_bitmap_brush_vtbl
            || iface->lpVtbl == (const ID2D1BrushVtbl *)&d2d_image_brush_vtbl);
    return CONTAINING_RECORD(iface, struct d2d_brush, ID2D1Brush_iface);
}

static D3D11_TEXTURE_ADDRESS_MODE texture_address_mode_from_extend_mode(D2D1_EXTEND_MODE mode)
{
    switch (mode)
    {
        case D2D1_EXTEND_MODE_CLAMP:
            return D3D11_TEXTURE_ADDRESS_CLAMP;
        case D2D1_EXTEND_MODE_WRAP:
            return D3D11_TEXTURE_ADDRESS_WRAP;
        case D2D1_EXTEND_MODE_MIRROR:
            return D3D11_TEXTURE_ADDRESS_MIRROR;
        default:
            FIXME("Unhandled extend mode %#x.\n", mode);
            return D3D11_TEXTURE_ADDRESS_CLAMP;
    }
}

BOOL d2d_brush_fill_cb(const struct d2d_brush *brush, struct d2d_brush_cb *cb)
{
    float theta, sin_theta, cos_theta;
    float dpi_scale, d, s1, s2, t, u;
    struct d2d_bitmap *bitmap;
    D2D1_POINT_2F v_p, v_q;
    D2D1_COLOR_F *colour;
    D2D_MATRIX_3X2_F b;

    if (!brush)
    {
        cb->type = D2D_BRUSH_TYPE_COUNT;
        return TRUE;
    }

    cb->type = brush->type;
    cb->opacity = brush->opacity;

    switch (brush->type)
    {
        case D2D_BRUSH_TYPE_SOLID:
            colour = &cb->u.solid.colour;

            *colour = brush->u.solid.color;
            colour->r *= colour->a;
            colour->g *= colour->a;
            colour->b *= colour->a;

            return TRUE;

        case D2D_BRUSH_TYPE_LINEAR:
            b = brush->transform;
            d2d_point_transform(&cb->u.linear.start, &b, brush->u.linear.start.x, brush->u.linear.start.y);
            d2d_point_transform(&cb->u.linear.end, &b, brush->u.linear.end.x, brush->u.linear.end.y);
            cb->u.linear.stop_count = brush->u.linear.gradient->stop_count;

            return TRUE;

        case D2D_BRUSH_TYPE_RADIAL:
            b = brush->transform;
            d2d_point_transform(&cb->u.radial.centre, &b, brush->u.radial.centre.x, brush->u.radial.centre.y);
            b._31 = b._32 = 0.0f;
            d2d_point_transform(&cb->u.radial.offset, &b, brush->u.radial.offset.x, brush->u.radial.offset.y);

            /* After a transformation, the axes of the ellipse are no longer
             * necessarily orthogonal, but they are conjugate diameters.
             *
             * A = ⎡a.x b.x⎤
             *     ⎣a.y b.y⎦
             *
             *   = ⎡cos(θ) -sin(θ)⎤⎡σ₁ 0 ⎤⎡cos(φ) -sin(φ)⎤
             *     ⎣sin(θ)  cos(θ)⎦⎣0  σ₂⎦⎣sin(φ)  cos(φ)⎦
             *
             * a' = [σ₁· cos(θ) σ₁·sin(θ)]
             * b' = [σ₂·-sin(θ) σ₂·cos(θ)] */

            d2d_point_set(&v_p, brush->u.radial.radius.x * b._11, brush->u.radial.radius.y * b._21);
            d2d_point_set(&v_q, brush->u.radial.radius.x * b._12, brush->u.radial.radius.y * b._22);

            t = 0.5f * d2d_point_dot(&v_p, &v_p);
            u = 0.5f * d2d_point_dot(&v_q, &v_q);
            s1 = t + u;

            t = t - u;
            u = d2d_point_dot(&v_p, &v_q);
            s2 = sqrtf(t * t + u * u);

            theta = 0.5f * atan2(u, t);
            sin_theta = sinf(theta);
            cos_theta = cosf(theta);

            t = sqrtf(s1 + s2);
            d2d_point_set(&cb->u.radial.ra, t * cos_theta, t * sin_theta);
            t = sqrtf(s1 - s2);
            d2d_point_set(&cb->u.radial.rb, t * -sin_theta, t * cos_theta);

            cb->u.radial.stop_count = brush->u.linear.gradient->stop_count;

            return TRUE;

        case D2D_BRUSH_TYPE_BITMAP:
        case D2D_BRUSH_TYPE_IMAGE:
        {
            ID2D1Bitmap *image_bitmap = NULL;

            if (brush->type == D2D_BRUSH_TYPE_BITMAP)
                bitmap = brush->u.bitmap.bitmap;
            else
            {
                if (FAILED(ID2D1Image_QueryInterface(brush->u.image.image, &IID_ID2D1Bitmap, (void **)&image_bitmap)))
                {
                    FIXME("Only image brushes with bitmaps are supported.\n");
                    return FALSE;
                }

                bitmap = unsafe_impl_from_ID2D1Bitmap(image_bitmap);
                cb->type = D2D_BRUSH_TYPE_BITMAP;
            }

            /* Scale for bitmap size and dpi. */
            b = brush->transform;
            dpi_scale = bitmap->pixel_size.width * (96.0f / bitmap->dpi_x);
            b._11 *= dpi_scale;
            b._21 *= dpi_scale;
            dpi_scale = bitmap->pixel_size.height * (96.0f / bitmap->dpi_y);
            b._12 *= dpi_scale;
            b._22 *= dpi_scale;

            /* Invert the matrix. (Because the matrix is applied to the
             * sampling coordinates. I.e., to scale the bitmap by 2 we need to
             * divide the coordinates by 2.) */
            d = b._11 * b._22 - b._21 * b._12;
            if (d != 0.0f)
            {
                cb->u.bitmap._11 = b._22 / d;
                cb->u.bitmap._21 = -b._21 / d;
                cb->u.bitmap._31 = (b._21 * b._32 - b._31 * b._22) / d;
                cb->u.bitmap._12 = -b._12 / d;
                cb->u.bitmap._22 = b._11 / d;
                cb->u.bitmap._32 = -(b._11 * b._32 - b._31 * b._12) / d;
            }

            cb->u.bitmap.ignore_alpha = bitmap->format.alphaMode == D2D1_ALPHA_MODE_IGNORE;

            if (image_bitmap)
                ID2D1Bitmap_Release(image_bitmap);

            return TRUE;
        }

        default:
            FIXME("Unhandled brush type %#x.\n", brush->type);
            return FALSE;
    }
}

static void d2d_brush_bind_bitmap(struct d2d_bitmap *bitmap, struct d2d_device_context *context,
        D2D1_EXTEND_MODE extend_mode_x, D2D1_EXTEND_MODE extend_mode_y,
        D2D1_INTERPOLATION_MODE interpolation_mode, unsigned int brush_idx)
{
    ID3D11SamplerState **sampler_state;
    ID3D11DeviceContext *d3d_context;
    HRESULT hr;

    ID3D11Device1_GetImmediateContext(context->d3d_device, &d3d_context);
    ID3D11DeviceContext_PSSetShaderResources(d3d_context, brush_idx, 1, &bitmap->srv);

    sampler_state = &context->sampler_states
            [interpolation_mode % D2D_SAMPLER_INTERPOLATION_MODE_COUNT]
            [extend_mode_x % D2D_SAMPLER_EXTEND_MODE_COUNT]
            [extend_mode_y % D2D_SAMPLER_EXTEND_MODE_COUNT];

    if (!*sampler_state)
    {
        D3D11_SAMPLER_DESC sampler_desc;

        if (interpolation_mode == D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR)
            sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        else
            sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampler_desc.AddressU = texture_address_mode_from_extend_mode(extend_mode_x);
        sampler_desc.AddressV = texture_address_mode_from_extend_mode(extend_mode_y);
        sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        sampler_desc.MipLODBias = 0.0f;
        sampler_desc.MaxAnisotropy = 0;
        sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sampler_desc.BorderColor[0] = 0.0f;
        sampler_desc.BorderColor[1] = 0.0f;
        sampler_desc.BorderColor[2] = 0.0f;
        sampler_desc.BorderColor[3] = 0.0f;
        sampler_desc.MinLOD = 0.0f;
        sampler_desc.MaxLOD = 0.0f;

        if (FAILED(hr = ID3D11Device1_CreateSamplerState(context->d3d_device, &sampler_desc, sampler_state)))
            ERR("Failed to create sampler state, hr %#lx.\n", hr);
    }

    ID3D11DeviceContext_PSSetSamplers(d3d_context, brush_idx, 1, sampler_state);
    ID3D11DeviceContext_Release(d3d_context);
}

static void d2d_brush_bind_image(struct d2d_brush *brush, struct d2d_device_context *context,
        unsigned int brush_idx)
{
    ID2D1Bitmap *image_bitmap;
    struct d2d_bitmap *bitmap;

    if (FAILED(ID2D1Image_QueryInterface(brush->u.image.image, &IID_ID2D1Bitmap, (void **)&image_bitmap)))
    {
        FIXME("Only image brushes with bitmaps are supported.\n");
        return;
    }

    bitmap = unsafe_impl_from_ID2D1Bitmap(image_bitmap);

    d2d_brush_bind_bitmap(bitmap, context, brush->u.image.extend_mode_x, brush->u.image.extend_mode_y,
            brush->u.image.interpolation_mode, brush_idx);

    ID2D1Bitmap_Release(image_bitmap);
}

void d2d_brush_bind_resources(struct d2d_brush *brush, struct d2d_device_context *context, unsigned int brush_idx)
{
    switch (brush->type)
    {
        case D2D_BRUSH_TYPE_SOLID:
            break;

        case D2D_BRUSH_TYPE_LINEAR:
            d2d_gradient_bind(brush->u.linear.gradient, context->d3d_device, brush_idx);
            break;

        case D2D_BRUSH_TYPE_RADIAL:
            d2d_gradient_bind(brush->u.radial.gradient, context->d3d_device, brush_idx);
            break;

        case D2D_BRUSH_TYPE_BITMAP:
            d2d_brush_bind_bitmap(brush->u.bitmap.bitmap, context, brush->u.bitmap.extend_mode_x,
                    brush->u.bitmap.extend_mode_y, brush->u.bitmap.interpolation_mode, brush_idx);
            break;

        case D2D_BRUSH_TYPE_IMAGE:
            d2d_brush_bind_image(brush, context, brush_idx);
            break;

        default:
            FIXME("Unhandled brush type %#x.\n", brush->type);
            break;
    }
}
