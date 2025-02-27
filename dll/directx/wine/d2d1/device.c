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
#include <d3dcompiler.h>

WINE_DEFAULT_DEBUG_CHANNEL(d2d);

#define INITIAL_CLIP_STACK_SIZE 4

static const D2D1_MATRIX_3X2_F identity =
{{{
    1.0f, 0.0f,
    0.0f, 1.0f,
    0.0f, 0.0f,
}}};

struct d2d_draw_text_layout_ctx
{
    ID2D1Brush *brush;
    D2D1_DRAW_TEXT_OPTIONS options;
};

static inline struct d2d_device *impl_from_ID2D1Device(ID2D1Device6 *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_device, ID2D1Device6_iface);
}

static ID2D1Brush *d2d_draw_get_text_brush(struct d2d_draw_text_layout_ctx *context, IUnknown *effect)
{
    ID2D1Brush *brush = NULL;

    if (effect && SUCCEEDED(IUnknown_QueryInterface(effect, &IID_ID2D1Brush, (void**)&brush)))
        return brush;

    ID2D1Brush_AddRef(context->brush);
    return context->brush;
}

static void d2d_rect_intersect(D2D1_RECT_F *dst, const D2D1_RECT_F *src)
{
    if (src->left > dst->left)
        dst->left = src->left;
    if (src->top > dst->top)
        dst->top = src->top;
    if (src->right < dst->right)
        dst->right = src->right;
    if (src->bottom < dst->bottom)
        dst->bottom = src->bottom;
}

static void d2d_rect_set(D2D1_RECT_F *dst, float left, float top, float right, float bottom)
{
    dst->left = left;
    dst->top = top;
    dst->right = right;
    dst->bottom = bottom;
}

static void d2d_size_set(D2D1_SIZE_U *dst, float width, float height)
{
    dst->width = width;
    dst->height = height;
}

static BOOL d2d_clip_stack_init(struct d2d_clip_stack *stack)
{
    if (!(stack->stack = malloc(INITIAL_CLIP_STACK_SIZE * sizeof(*stack->stack))))
        return FALSE;

    stack->size = INITIAL_CLIP_STACK_SIZE;
    stack->count = 0;

    return TRUE;
}

static void d2d_clip_stack_cleanup(struct d2d_clip_stack *stack)
{
    free(stack->stack);
}

static BOOL d2d_clip_stack_push(struct d2d_clip_stack *stack, const D2D1_RECT_F *rect)
{
    D2D1_RECT_F r;

    if (!d2d_array_reserve((void **)&stack->stack, &stack->size, stack->count + 1, sizeof(*stack->stack)))
        return FALSE;

    r = *rect;
    if (stack->count)
        d2d_rect_intersect(&r, &stack->stack[stack->count - 1]);
    stack->stack[stack->count++] = r;

    return TRUE;
}

static void d2d_clip_stack_pop(struct d2d_clip_stack *stack)
{
    if (!stack->count)
        return;
    --stack->count;
}

static void d2d_device_context_draw(struct d2d_device_context *render_target, enum d2d_shape_type shape_type,
        ID3D11Buffer *ib, unsigned int index_count, ID3D11Buffer *vb, unsigned int vb_stride,
        struct d2d_brush *brush, struct d2d_brush *opacity_brush)
{
    struct d2d_shape_resources *shape_resources = &render_target->shape_resources[shape_type];
    ID3DDeviceContextState *prev_state;
    ID3D11Device1 *device = render_target->d3d_device;
    ID3D11DeviceContext1 *context;
    ID3D11Buffer *vs_cb = render_target->vs_cb, *ps_cb = render_target->ps_cb;
    D3D11_RECT scissor_rect;
    unsigned int offset;
    D3D11_VIEWPORT vp;

    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.Width = render_target->pixel_size.width;
    vp.Height = render_target->pixel_size.height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;

    if (render_target->cs)
        EnterCriticalSection(render_target->cs);

    ID3D11Device1_GetImmediateContext1(device, &context);
    ID3D11DeviceContext1_SwapDeviceContextState(context, render_target->d3d_state, &prev_state);

    ID3D11DeviceContext1_IASetInputLayout(context, shape_resources->il);
    ID3D11DeviceContext1_IASetPrimitiveTopology(context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext1_IASetIndexBuffer(context, ib, DXGI_FORMAT_R16_UINT, 0);
    offset = 0;
    ID3D11DeviceContext1_IASetVertexBuffers(context, 0, 1, &vb, &vb_stride, &offset);
    ID3D11DeviceContext1_VSSetConstantBuffers(context, 0, 1, &vs_cb);
    ID3D11DeviceContext1_VSSetShader(context, shape_resources->vs, NULL, 0);
    ID3D11DeviceContext1_PSSetConstantBuffers(context, 0, 1, &ps_cb);
    ID3D11DeviceContext1_PSSetShader(context, render_target->ps, NULL, 0);
    ID3D11DeviceContext1_RSSetViewports(context, 1, &vp);
    if (render_target->clip_stack.count)
    {
        const D2D1_RECT_F *clip_rect;

        clip_rect = &render_target->clip_stack.stack[render_target->clip_stack.count - 1];
        scissor_rect.left = ceilf(clip_rect->left - 0.5f);
        scissor_rect.top = ceilf(clip_rect->top - 0.5f);
        scissor_rect.right = ceilf(clip_rect->right - 0.5f);
        scissor_rect.bottom = ceilf(clip_rect->bottom - 0.5f);
    }
    else
    {
        scissor_rect.left = 0.0f;
        scissor_rect.top = 0.0f;
        scissor_rect.right = render_target->pixel_size.width;
        scissor_rect.bottom = render_target->pixel_size.height;
    }
    ID3D11DeviceContext1_RSSetScissorRects(context, 1, &scissor_rect);
    ID3D11DeviceContext1_RSSetState(context, render_target->rs);
    ID3D11DeviceContext1_OMSetRenderTargets(context, 1, &render_target->target.bitmap->rtv, NULL);
    if (brush)
    {
        ID3D11DeviceContext1_OMSetBlendState(context, render_target->bs, NULL, D3D11_DEFAULT_SAMPLE_MASK);
        d2d_brush_bind_resources(brush, render_target, 0);
    }
    else
    {
        ID3D11DeviceContext1_OMSetBlendState(context, NULL, NULL, D3D11_DEFAULT_SAMPLE_MASK);
    }
    if (opacity_brush)
        d2d_brush_bind_resources(opacity_brush, render_target, 1);

    if (ib)
        ID3D11DeviceContext1_DrawIndexed(context, index_count, 0, 0);
    else
        ID3D11DeviceContext1_Draw(context, index_count, 0);

    ID3D11DeviceContext1_SwapDeviceContextState(context, prev_state, NULL);
    ID3D11DeviceContext1_Release(context);
    ID3DDeviceContextState_Release(prev_state);

    if (render_target->cs)
        LeaveCriticalSection(render_target->cs);
}

static void d2d_device_context_set_error(struct d2d_device_context *context, HRESULT code)
{
    context->error.code = code;
    context->error.tag1 = context->drawing_state.tag1;
    context->error.tag2 = context->drawing_state.tag2;
}

static inline struct d2d_device_context *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_device_context, IUnknown_iface);
}

static inline struct d2d_device_context *impl_from_ID2D1DeviceContext(ID2D1DeviceContext6 *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_device_context, ID2D1DeviceContext6_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_inner_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    struct d2d_device_context *context = impl_from_IUnknown(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1DeviceContext6)
            || IsEqualGUID(iid, &IID_ID2D1DeviceContext5)
            || IsEqualGUID(iid, &IID_ID2D1DeviceContext4)
            || IsEqualGUID(iid, &IID_ID2D1DeviceContext3)
            || IsEqualGUID(iid, &IID_ID2D1DeviceContext2)
            || IsEqualGUID(iid, &IID_ID2D1DeviceContext1)
            || IsEqualGUID(iid, &IID_ID2D1DeviceContext)
            || IsEqualGUID(iid, &IID_ID2D1RenderTarget)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1DeviceContext6_AddRef(&context->ID2D1DeviceContext6_iface);
        *out = &context->ID2D1DeviceContext6_iface;
        return S_OK;
    }
    else if (IsEqualGUID(iid, &IID_ID2D1GdiInteropRenderTarget))
    {
        ID2D1GdiInteropRenderTarget_AddRef(&context->ID2D1GdiInteropRenderTarget_iface);
        *out = &context->ID2D1GdiInteropRenderTarget_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_device_context_inner_AddRef(IUnknown *iface)
{
    struct d2d_device_context *context = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&context->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_device_context_inner_Release(IUnknown *iface)
{
    struct d2d_device_context *context = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&context->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        unsigned int i, j, k;

        d2d_clip_stack_cleanup(&context->clip_stack);
        IDWriteRenderingParams_Release(context->default_text_rendering_params);
        if (context->text_rendering_params)
            IDWriteRenderingParams_Release(context->text_rendering_params);
        if (context->bs)
            ID3D11BlendState_Release(context->bs);
        ID3D11RasterizerState_Release(context->rs);
        ID3D11Buffer_Release(context->vb);
        ID3D11Buffer_Release(context->ib);
        ID3D11Buffer_Release(context->ps_cb);
        ID3D11PixelShader_Release(context->ps);
        ID3D11Buffer_Release(context->vs_cb);
        for (i = 0; i < D2D_SHAPE_TYPE_COUNT; ++i)
        {
            ID3D11VertexShader_Release(context->shape_resources[i].vs);
            ID3D11InputLayout_Release(context->shape_resources[i].il);
        }
        for (i = 0; i < D2D_SAMPLER_INTERPOLATION_MODE_COUNT; ++i)
        {
            for (j = 0; j < D2D_SAMPLER_EXTEND_MODE_COUNT; ++j)
            {
                for (k = 0; k < D2D_SAMPLER_EXTEND_MODE_COUNT; ++k)
                {
                    if (context->sampler_states[i][j][k])
                        ID3D11SamplerState_Release(context->sampler_states[i][j][k]);
                }
            }
        }
        if (context->d3d_state)
            ID3DDeviceContextState_Release(context->d3d_state);
        if (context->target.object)
            IUnknown_Release(context->target.object);
        ID3D11Device1_Release(context->d3d_device);
        ID2D1Factory_Release(context->factory);
        ID2D1Device6_Release(&context->device->ID2D1Device6_iface);
        d2d_device_indexed_objects_clear(&context->vertex_buffers);
        free(context);
    }

    return refcount;
}

static const struct IUnknownVtbl d2d_device_context_inner_unknown_vtbl =
{
    d2d_device_context_inner_QueryInterface,
    d2d_device_context_inner_AddRef,
    d2d_device_context_inner_Release,
};

static HRESULT STDMETHODCALLTYPE d2d_device_context_QueryInterface(ID2D1DeviceContext6 *iface,
        REFIID iid, void **out)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    return IUnknown_QueryInterface(context->outer_unknown, iid, out);
}

static ULONG STDMETHODCALLTYPE d2d_device_context_AddRef(ID2D1DeviceContext6 *iface)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p.\n", iface);

    return IUnknown_AddRef(context->outer_unknown);
}

static ULONG STDMETHODCALLTYPE d2d_device_context_Release(ID2D1DeviceContext6 *iface)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p.\n", iface);

    return IUnknown_Release(context->outer_unknown);
}

static void STDMETHODCALLTYPE d2d_device_context_GetFactory(ID2D1DeviceContext6 *iface, ID2D1Factory **factory)
{
    struct d2d_device_context *render_target = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p, factory %p.\n", iface, factory);

    *factory = render_target->factory;
    ID2D1Factory_AddRef(*factory);
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateBitmap(ID2D1DeviceContext6 *iface,
        D2D1_SIZE_U size, const void *src_data, UINT32 pitch, const D2D1_BITMAP_PROPERTIES *desc, ID2D1Bitmap **bitmap)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);
    D2D1_BITMAP_PROPERTIES1 bitmap_desc;
    struct d2d_bitmap *object;
    HRESULT hr;

    TRACE("iface %p, size {%u, %u}, src_data %p, pitch %u, desc %p, bitmap %p.\n",
            iface, size.width, size.height, src_data, pitch, desc, bitmap);

    if (desc)
    {
        memcpy(&bitmap_desc, desc, sizeof(*desc));
        bitmap_desc.bitmapOptions = 0;
        bitmap_desc.colorContext = NULL;
    }

    if (SUCCEEDED(hr = d2d_bitmap_create(context, size, src_data, pitch, desc ? &bitmap_desc : NULL, &object)))
        *bitmap = (ID2D1Bitmap *)&object->ID2D1Bitmap1_iface;

    return hr;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateBitmapFromWicBitmap(ID2D1DeviceContext6 *iface,
        IWICBitmapSource *bitmap_source, const D2D1_BITMAP_PROPERTIES *desc, ID2D1Bitmap **bitmap)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);
    D2D1_BITMAP_PROPERTIES1 bitmap_desc;
    struct d2d_bitmap *object;
    HRESULT hr;

    TRACE("iface %p, bitmap_source %p, desc %p, bitmap %p.\n",
            iface, bitmap_source, desc, bitmap);

    if (desc)
    {
        memcpy(&bitmap_desc, desc, sizeof(*desc));
        bitmap_desc.bitmapOptions = 0;
        bitmap_desc.colorContext = NULL;
    }

    if (SUCCEEDED(hr = d2d_bitmap_create_from_wic_bitmap(context, bitmap_source, desc ? &bitmap_desc : NULL, &object)))
        *bitmap = (ID2D1Bitmap *)&object->ID2D1Bitmap1_iface;

    return hr;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateSharedBitmap(ID2D1DeviceContext6 *iface,
        REFIID iid, void *data, const D2D1_BITMAP_PROPERTIES *desc, ID2D1Bitmap **bitmap)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);
    D2D1_BITMAP_PROPERTIES1 bitmap_desc;
    struct d2d_bitmap *object;
    HRESULT hr;

    TRACE("iface %p, iid %s, data %p, desc %p, bitmap %p.\n",
            iface, debugstr_guid(iid), data, desc, bitmap);

    if (desc)
    {
        memcpy(&bitmap_desc, desc, sizeof(*desc));
        if (IsEqualIID(iid, &IID_IDXGISurface) || IsEqualIID(iid, &IID_IDXGISurface1))
            bitmap_desc.bitmapOptions = d2d_get_bitmap_options_for_surface(data);
        else
            bitmap_desc.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
        bitmap_desc.colorContext = NULL;
    }

    if (SUCCEEDED(hr = d2d_bitmap_create_shared(context, iid, data, desc ? &bitmap_desc : NULL, &object)))
        *bitmap = (ID2D1Bitmap *)&object->ID2D1Bitmap1_iface;

    return hr;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateBitmapBrush(ID2D1DeviceContext6 *iface,
        ID2D1Bitmap *bitmap, const D2D1_BITMAP_BRUSH_PROPERTIES *bitmap_brush_desc,
        const D2D1_BRUSH_PROPERTIES *brush_desc, ID2D1BitmapBrush **brush)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);
    struct d2d_brush *object;
    HRESULT hr;

    TRACE("iface %p, bitmap %p, bitmap_brush_desc %p, brush_desc %p, brush %p.\n",
            iface, bitmap, bitmap_brush_desc, brush_desc, brush);

    if (SUCCEEDED(hr = d2d_bitmap_brush_create(context->factory, bitmap, (const D2D1_BITMAP_BRUSH_PROPERTIES1 *)bitmap_brush_desc,
            brush_desc, &object)))
        *brush = (ID2D1BitmapBrush *)&object->ID2D1Brush_iface;

    return hr;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateSolidColorBrush(ID2D1DeviceContext6 *iface,
        const D2D1_COLOR_F *color, const D2D1_BRUSH_PROPERTIES *desc, ID2D1SolidColorBrush **brush)
{
    struct d2d_device_context *render_target = impl_from_ID2D1DeviceContext(iface);
    struct d2d_brush *object;
    HRESULT hr;

    TRACE("iface %p, color %p, desc %p, brush %p.\n", iface, color, desc, brush);

    if (SUCCEEDED(hr = d2d_solid_color_brush_create(render_target->factory, color, desc, &object)))
        *brush = (ID2D1SolidColorBrush *)&object->ID2D1Brush_iface;

    return hr;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateGradientStopCollection(ID2D1DeviceContext6 *iface,
        const D2D1_GRADIENT_STOP *stops, UINT32 stop_count, D2D1_GAMMA gamma, D2D1_EXTEND_MODE extend_mode,
        ID2D1GradientStopCollection **gradient)
{
    struct d2d_device_context *render_target = impl_from_ID2D1DeviceContext(iface);
    struct d2d_gradient *object;
    HRESULT hr;

    TRACE("iface %p, stops %p, stop_count %u, gamma %#x, extend_mode %#x, gradient %p.\n",
            iface, stops, stop_count, gamma, extend_mode, gradient);

    if (SUCCEEDED(hr = d2d_gradient_create(render_target->factory, render_target->d3d_device,
            stops, stop_count, gamma, extend_mode, &object)))
        *gradient = &object->ID2D1GradientStopCollection_iface;

    return hr;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateLinearGradientBrush(ID2D1DeviceContext6 *iface,
        const D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES *gradient_brush_desc, const D2D1_BRUSH_PROPERTIES *brush_desc,
        ID2D1GradientStopCollection *gradient, ID2D1LinearGradientBrush **brush)
{
    struct d2d_device_context *render_target = impl_from_ID2D1DeviceContext(iface);
    struct d2d_brush *object;
    HRESULT hr;

    TRACE("iface %p, gradient_brush_desc %p, brush_desc %p, gradient %p, brush %p.\n",
            iface, gradient_brush_desc, brush_desc, gradient, brush);

    if (SUCCEEDED(hr = d2d_linear_gradient_brush_create(render_target->factory, gradient_brush_desc, brush_desc,
        gradient, &object)))
        *brush = (ID2D1LinearGradientBrush *)&object->ID2D1Brush_iface;

    return hr;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateRadialGradientBrush(ID2D1DeviceContext6 *iface,
        const D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES *gradient_brush_desc, const D2D1_BRUSH_PROPERTIES *brush_desc,
        ID2D1GradientStopCollection *gradient, ID2D1RadialGradientBrush **brush)
{
    struct d2d_device_context *render_target = impl_from_ID2D1DeviceContext(iface);
    struct d2d_brush *object;
    HRESULT hr;

    TRACE("iface %p, gradient_brush_desc %p, brush_desc %p, gradient %p, brush %p.\n",
            iface, gradient_brush_desc, brush_desc, gradient, brush);

    if (SUCCEEDED(hr = d2d_radial_gradient_brush_create(render_target->factory,
            gradient_brush_desc, brush_desc, gradient, &object)))
        *brush = (ID2D1RadialGradientBrush *)&object->ID2D1Brush_iface;

    return hr;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateCompatibleRenderTarget(ID2D1DeviceContext6 *iface,
        const D2D1_SIZE_F *size, const D2D1_SIZE_U *pixel_size, const D2D1_PIXEL_FORMAT *format,
        D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS options, ID2D1BitmapRenderTarget **rt)
{
    struct d2d_device_context *render_target = impl_from_ID2D1DeviceContext(iface);
    struct d2d_bitmap_render_target *object;
    HRESULT hr;

    TRACE("iface %p, size %p, pixel_size %p, format %p, options %#x, render_target %p.\n",
            iface, size, pixel_size, format, options, rt);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d2d_bitmap_render_target_init(object, render_target, size, pixel_size,
            format, options)))
    {
        WARN("Failed to initialise render target, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    TRACE("Created render target %p.\n", object);
    *rt = &object->ID2D1BitmapRenderTarget_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateLayer(ID2D1DeviceContext6 *iface,
        const D2D1_SIZE_F *size, ID2D1Layer **layer)
{
    struct d2d_device_context *render_target = impl_from_ID2D1DeviceContext(iface);
    struct d2d_layer *object;
    HRESULT hr;

    TRACE("iface %p, size %p, layer %p.\n", iface, size, layer);

    if (SUCCEEDED(hr = d2d_layer_create(render_target->factory, size, &object)))
        *layer = &object->ID2D1Layer_iface;

    return hr;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateMesh(ID2D1DeviceContext6 *iface, ID2D1Mesh **mesh)
{
    struct d2d_device_context *render_target = impl_from_ID2D1DeviceContext(iface);
    struct d2d_mesh *object;
    HRESULT hr;

    TRACE("iface %p, mesh %p.\n", iface, mesh);

    if (SUCCEEDED(hr = d2d_mesh_create(render_target->factory, &object)))
        *mesh = &object->ID2D1Mesh_iface;

    return hr;
}

static void STDMETHODCALLTYPE d2d_device_context_DrawLine(ID2D1DeviceContext6 *iface,
        D2D1_POINT_2F p0, D2D1_POINT_2F p1, ID2D1Brush *brush, float stroke_width, ID2D1StrokeStyle *stroke_style)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);
    ID2D1PathGeometry *geometry;
    ID2D1GeometrySink *sink;
    HRESULT hr;

    TRACE("iface %p, p0 %s, p1 %s, brush %p, stroke_width %.8e, stroke_style %p.\n",
            iface, debug_d2d_point_2f(&p0), debug_d2d_point_2f(&p1), brush, stroke_width, stroke_style);

    if (context->target.type == D2D_TARGET_COMMAND_LIST)
    {
        d2d_command_list_draw_line(context->target.command_list, context, p0, p1, brush, stroke_width, stroke_style);
        return;
    }

    if (FAILED(hr = ID2D1Factory_CreatePathGeometry(context->factory, &geometry)))
    {
        WARN("Failed to create path geometry, hr %#lx.\n", hr);
        return;
    }

    if (FAILED(hr = ID2D1PathGeometry_Open(geometry, &sink)))
    {
        WARN("Failed to open geometry sink, hr %#lx.\n", hr);
        ID2D1PathGeometry_Release(geometry);
        return;
    }

    ID2D1GeometrySink_BeginFigure(sink, p0, D2D1_FIGURE_BEGIN_HOLLOW);
    ID2D1GeometrySink_AddLine(sink, p1);
    ID2D1GeometrySink_EndFigure(sink, D2D1_FIGURE_END_OPEN);
    if (FAILED(hr = ID2D1GeometrySink_Close(sink)))
        WARN("Failed to close geometry sink, hr %#lx.\n", hr);
    ID2D1GeometrySink_Release(sink);

    ID2D1DeviceContext6_DrawGeometry(iface, (ID2D1Geometry *)geometry, brush, stroke_width, stroke_style);
    ID2D1PathGeometry_Release(geometry);
}

static void STDMETHODCALLTYPE d2d_device_context_DrawRectangle(ID2D1DeviceContext6 *iface,
        const D2D1_RECT_F *rect, ID2D1Brush *brush, float stroke_width, ID2D1StrokeStyle *stroke_style)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);
    ID2D1RectangleGeometry *geometry;
    HRESULT hr;

    TRACE("iface %p, rect %s, brush %p, stroke_width %.8e, stroke_style %p.\n",
            iface, debug_d2d_rect_f(rect), brush, stroke_width, stroke_style);

    if (context->target.type == D2D_TARGET_COMMAND_LIST)
    {
        d2d_command_list_draw_rectangle(context->target.command_list, context, rect, brush, stroke_width, stroke_style);
        return;
    }

    if (FAILED(hr = ID2D1Factory_CreateRectangleGeometry(context->factory, rect, &geometry)))
    {
        ERR("Failed to create geometry, hr %#lx.\n", hr);
        return;
    }

    ID2D1DeviceContext6_DrawGeometry(iface, (ID2D1Geometry *)geometry, brush, stroke_width, stroke_style);
    ID2D1RectangleGeometry_Release(geometry);
}

static void STDMETHODCALLTYPE d2d_device_context_FillRectangle(ID2D1DeviceContext6 *iface,
        const D2D1_RECT_F *rect, ID2D1Brush *brush)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);
    ID2D1RectangleGeometry *geometry;
    HRESULT hr;

    TRACE("iface %p, rect %s, brush %p.\n", iface, debug_d2d_rect_f(rect), brush);

    if (context->target.type == D2D_TARGET_COMMAND_LIST)
    {
        d2d_command_list_fill_rectangle(context->target.command_list, context, rect, brush);
        return;
    }

    if (FAILED(hr = ID2D1Factory_CreateRectangleGeometry(context->factory, rect, &geometry)))
    {
        ERR("Failed to create geometry, hr %#lx.\n", hr);
        return;
    }

    ID2D1DeviceContext6_FillGeometry(iface, (ID2D1Geometry *)geometry, brush, NULL);
    ID2D1RectangleGeometry_Release(geometry);
}

static void STDMETHODCALLTYPE d2d_device_context_DrawRoundedRectangle(ID2D1DeviceContext6 *iface,
        const D2D1_ROUNDED_RECT *rect, ID2D1Brush *brush, float stroke_width, ID2D1StrokeStyle *stroke_style)
{
    struct d2d_device_context *render_target = impl_from_ID2D1DeviceContext(iface);
    ID2D1RoundedRectangleGeometry *geometry;
    HRESULT hr;

    TRACE("iface %p, rect %p, brush %p, stroke_width %.8e, stroke_style %p.\n",
            iface, rect, brush, stroke_width, stroke_style);

    if (FAILED(hr = ID2D1Factory_CreateRoundedRectangleGeometry(render_target->factory, rect, &geometry)))
    {
        ERR("Failed to create geometry, hr %#lx.\n", hr);
        return;
    }

    ID2D1DeviceContext6_DrawGeometry(iface, (ID2D1Geometry *)geometry, brush, stroke_width, stroke_style);
    ID2D1RoundedRectangleGeometry_Release(geometry);
}

static void STDMETHODCALLTYPE d2d_device_context_FillRoundedRectangle(ID2D1DeviceContext6 *iface,
        const D2D1_ROUNDED_RECT *rect, ID2D1Brush *brush)
{
    struct d2d_device_context *render_target = impl_from_ID2D1DeviceContext(iface);
    ID2D1RoundedRectangleGeometry *geometry;
    HRESULT hr;

    TRACE("iface %p, rect %p, brush %p.\n", iface, rect, brush);

    if (FAILED(hr = ID2D1Factory_CreateRoundedRectangleGeometry(render_target->factory, rect, &geometry)))
    {
        ERR("Failed to create geometry, hr %#lx.\n", hr);
        return;
    }

    ID2D1DeviceContext6_FillGeometry(iface, (ID2D1Geometry *)geometry, brush, NULL);
    ID2D1RoundedRectangleGeometry_Release(geometry);
}

static void STDMETHODCALLTYPE d2d_device_context_DrawEllipse(ID2D1DeviceContext6 *iface,
        const D2D1_ELLIPSE *ellipse, ID2D1Brush *brush, float stroke_width, ID2D1StrokeStyle *stroke_style)
{
    struct d2d_device_context *render_target = impl_from_ID2D1DeviceContext(iface);
    ID2D1EllipseGeometry *geometry;
    HRESULT hr;

    TRACE("iface %p, ellipse %p, brush %p, stroke_width %.8e, stroke_style %p.\n",
            iface, ellipse, brush, stroke_width, stroke_style);

    if (FAILED(hr = ID2D1Factory_CreateEllipseGeometry(render_target->factory, ellipse, &geometry)))
    {
        ERR("Failed to create geometry, hr %#lx.\n", hr);
        return;
    }

    ID2D1DeviceContext6_DrawGeometry(iface, (ID2D1Geometry *)geometry, brush, stroke_width, stroke_style);
    ID2D1EllipseGeometry_Release(geometry);
}

static void STDMETHODCALLTYPE d2d_device_context_FillEllipse(ID2D1DeviceContext6 *iface,
        const D2D1_ELLIPSE *ellipse, ID2D1Brush *brush)
{
    struct d2d_device_context *render_target = impl_from_ID2D1DeviceContext(iface);
    ID2D1EllipseGeometry *geometry;
    HRESULT hr;

    TRACE("iface %p, ellipse %p, brush %p.\n", iface, ellipse, brush);

    if (FAILED(hr = ID2D1Factory_CreateEllipseGeometry(render_target->factory, ellipse, &geometry)))
    {
        ERR("Failed to create geometry, hr %#lx.\n", hr);
        return;
    }

    ID2D1DeviceContext6_FillGeometry(iface, (ID2D1Geometry *)geometry, brush, NULL);
    ID2D1EllipseGeometry_Release(geometry);
}

static HRESULT d2d_device_context_update_ps_cb(struct d2d_device_context *context,
        struct d2d_brush *brush, struct d2d_brush *opacity_brush, BOOL outline, BOOL is_arc)
{
    D3D11_MAPPED_SUBRESOURCE map_desc;
    ID3D11DeviceContext *d3d_context;
    struct d2d_ps_cb *cb_data;
    HRESULT hr;

    ID3D11Device1_GetImmediateContext(context->d3d_device, &d3d_context);

    if (FAILED(hr = ID3D11DeviceContext_Map(d3d_context, (ID3D11Resource *)context->ps_cb,
            0, D3D11_MAP_WRITE_DISCARD, 0, &map_desc)))
    {
        WARN("Failed to map constant buffer, hr %#lx.\n", hr);
        ID3D11DeviceContext_Release(d3d_context);
        return hr;
    }

    cb_data = map_desc.pData;
    cb_data->outline = outline;
    cb_data->is_arc = is_arc;
    cb_data->pad[0] = 0;
    cb_data->pad[1] = 0;
    if (!d2d_brush_fill_cb(brush, &cb_data->colour_brush))
        WARN("Failed to initialize colour brush buffer.\n");
    if (!d2d_brush_fill_cb(opacity_brush, &cb_data->opacity_brush))
        WARN("Failed to initialize opacity brush buffer.\n");

    ID3D11DeviceContext_Unmap(d3d_context, (ID3D11Resource *)context->ps_cb, 0);
    ID3D11DeviceContext_Release(d3d_context);

    return hr;
}

static HRESULT d2d_device_context_update_vs_cb(struct d2d_device_context *context,
        const D2D_MATRIX_3X2_F *geometry_transform, float stroke_width)
{
    D3D11_MAPPED_SUBRESOURCE map_desc;
    ID3D11DeviceContext *d3d_context;
    const D2D1_MATRIX_3X2_F *w;
    struct d2d_vs_cb *cb_data;
    float tmp_x, tmp_y;
    HRESULT hr;

    ID3D11Device1_GetImmediateContext(context->d3d_device, &d3d_context);

    if (FAILED(hr = ID3D11DeviceContext_Map(d3d_context, (ID3D11Resource *)context->vs_cb,
            0, D3D11_MAP_WRITE_DISCARD, 0, &map_desc)))
    {
        WARN("Failed to map constant buffer, hr %#lx.\n", hr);
        ID3D11DeviceContext_Release(d3d_context);
        return hr;
    }

    cb_data = map_desc.pData;
    cb_data->transform_geometry._11 = geometry_transform->_11;
    cb_data->transform_geometry._21 = geometry_transform->_21;
    cb_data->transform_geometry._31 = geometry_transform->_31;
    cb_data->transform_geometry.pad0 = 0.0f;
    cb_data->transform_geometry._12 = geometry_transform->_12;
    cb_data->transform_geometry._22 = geometry_transform->_22;
    cb_data->transform_geometry._32 = geometry_transform->_32;
    cb_data->transform_geometry.stroke_width = stroke_width;

    w = &context->drawing_state.transform;

    tmp_x = context->desc.dpiX / 96.0f;
    cb_data->transform_rtx.x = w->_11 * tmp_x;
    cb_data->transform_rtx.y = w->_21 * tmp_x;
    cb_data->transform_rtx.z = w->_31 * tmp_x;
    cb_data->transform_rtx.w = 2.0f / context->pixel_size.width;

    tmp_y = context->desc.dpiY / 96.0f;
    cb_data->transform_rty.x = w->_12 * tmp_y;
    cb_data->transform_rty.y = w->_22 * tmp_y;
    cb_data->transform_rty.z = w->_32 * tmp_y;
    cb_data->transform_rty.w = -2.0f / context->pixel_size.height;

    ID3D11DeviceContext_Unmap(d3d_context, (ID3D11Resource *)context->vs_cb, 0);
    ID3D11DeviceContext_Release(d3d_context);

    return S_OK;
}

static void d2d_device_context_draw_geometry(struct d2d_device_context *render_target,
        const struct d2d_geometry *geometry, struct d2d_brush *brush, float stroke_width)
{
    D3D11_SUBRESOURCE_DATA buffer_data;
    D3D11_BUFFER_DESC buffer_desc;
    ID3D11Buffer *ib, *vb;
    HRESULT hr;

    if (FAILED(hr = d2d_device_context_update_vs_cb(render_target, &geometry->transform, stroke_width)))
    {
        WARN("Failed to update vs constant buffer, hr %#lx.\n", hr);
        return;
    }

    if (FAILED(hr = d2d_device_context_update_ps_cb(render_target, brush, NULL, TRUE, FALSE)))
    {
        WARN("Failed to update ps constant buffer, hr %#lx.\n", hr);
        return;
    }

    buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    buffer_desc.CPUAccessFlags = 0;
    buffer_desc.MiscFlags = 0;

    buffer_data.SysMemPitch = 0;
    buffer_data.SysMemSlicePitch = 0;

    if (geometry->outline.face_count)
    {
        buffer_desc.ByteWidth = geometry->outline.face_count * sizeof(*geometry->outline.faces);
        buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        buffer_data.pSysMem = geometry->outline.faces;

        if (FAILED(hr = ID3D11Device1_CreateBuffer(render_target->d3d_device, &buffer_desc, &buffer_data, &ib)))
        {
            WARN("Failed to create index buffer, hr %#lx.\n", hr);
            return;
        }

        buffer_desc.ByteWidth = geometry->outline.vertex_count * sizeof(*geometry->outline.vertices);
        buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        buffer_data.pSysMem = geometry->outline.vertices;

        if (FAILED(hr = ID3D11Device1_CreateBuffer(render_target->d3d_device, &buffer_desc, &buffer_data, &vb)))
        {
            ERR("Failed to create vertex buffer, hr %#lx.\n", hr);
            ID3D11Buffer_Release(ib);
            return;
        }

        d2d_device_context_draw(render_target, D2D_SHAPE_TYPE_OUTLINE, ib, 3 * geometry->outline.face_count, vb,
                sizeof(*geometry->outline.vertices), brush, NULL);

        ID3D11Buffer_Release(vb);
        ID3D11Buffer_Release(ib);
    }

    if (geometry->outline.bezier_face_count)
    {
        buffer_desc.ByteWidth = geometry->outline.bezier_face_count * sizeof(*geometry->outline.bezier_faces);
        buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        buffer_data.pSysMem = geometry->outline.bezier_faces;

        if (FAILED(hr = ID3D11Device1_CreateBuffer(render_target->d3d_device, &buffer_desc, &buffer_data, &ib)))
        {
            WARN("Failed to create curves index buffer, hr %#lx.\n", hr);
            return;
        }

        buffer_desc.ByteWidth = geometry->outline.bezier_count * sizeof(*geometry->outline.beziers);
        buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        buffer_data.pSysMem = geometry->outline.beziers;

        if (FAILED(hr = ID3D11Device1_CreateBuffer(render_target->d3d_device, &buffer_desc, &buffer_data, &vb)))
        {
            ERR("Failed to create curves vertex buffer, hr %#lx.\n", hr);
            ID3D11Buffer_Release(ib);
            return;
        }

        d2d_device_context_draw(render_target, D2D_SHAPE_TYPE_BEZIER_OUTLINE, ib,
                3 * geometry->outline.bezier_face_count, vb,
                sizeof(*geometry->outline.beziers), brush, NULL);

        ID3D11Buffer_Release(vb);
        ID3D11Buffer_Release(ib);
    }

    if (geometry->outline.arc_face_count)
    {
        buffer_desc.ByteWidth = geometry->outline.arc_face_count * sizeof(*geometry->outline.arc_faces);
        buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        buffer_data.pSysMem = geometry->outline.arc_faces;

        if (FAILED(hr = ID3D11Device1_CreateBuffer(render_target->d3d_device, &buffer_desc, &buffer_data, &ib)))
        {
            WARN("Failed to create arcs index buffer, hr %#lx.\n", hr);
            return;
        }

        buffer_desc.ByteWidth = geometry->outline.arc_count * sizeof(*geometry->outline.arcs);
        buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        buffer_data.pSysMem = geometry->outline.arcs;

        if (FAILED(hr = ID3D11Device1_CreateBuffer(render_target->d3d_device, &buffer_desc, &buffer_data, &vb)))
        {
            ERR("Failed to create arcs vertex buffer, hr %#lx.\n", hr);
            ID3D11Buffer_Release(ib);
            return;
        }

        if (SUCCEEDED(d2d_device_context_update_ps_cb(render_target, brush, NULL, TRUE, TRUE)))
            d2d_device_context_draw(render_target, D2D_SHAPE_TYPE_ARC_OUTLINE, ib,
                    3 * geometry->outline.arc_face_count, vb,
                    sizeof(*geometry->outline.arcs), brush, NULL);

        ID3D11Buffer_Release(vb);
        ID3D11Buffer_Release(ib);
    }
}

static void STDMETHODCALLTYPE d2d_device_context_DrawGeometry(ID2D1DeviceContext6 *iface,
        ID2D1Geometry *geometry, ID2D1Brush *brush, float stroke_width, ID2D1StrokeStyle *stroke_style)
{
    const struct d2d_geometry *geometry_impl = unsafe_impl_from_ID2D1Geometry(geometry);
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);
    struct d2d_brush *brush_impl = unsafe_impl_from_ID2D1Brush(brush);
    struct d2d_stroke_style *stroke_style_impl = unsafe_impl_from_ID2D1StrokeStyle(stroke_style);

    TRACE("iface %p, geometry %p, brush %p, stroke_width %.8e, stroke_style %p.\n",
            iface, geometry, brush, stroke_width, stroke_style);

    if (context->target.type == D2D_TARGET_COMMAND_LIST)
    {
        d2d_command_list_draw_geometry(context->target.command_list, context, geometry, brush,
                stroke_width, stroke_style);
        return;
    }

    if (stroke_style)
        FIXME("Ignoring stroke style %p.\n", stroke_style);

    if (stroke_style_impl)
    {
        if (stroke_style_impl->desc.transformType == D2D1_STROKE_TRANSFORM_TYPE_FIXED)
            stroke_width /= context->drawing_state.transform.m11;
    }

    d2d_device_context_draw_geometry(context, geometry_impl, brush_impl, stroke_width);
}

static void d2d_device_context_fill_geometry(struct d2d_device_context *render_target,
        const struct d2d_geometry *geometry, struct d2d_brush *brush, struct d2d_brush *opacity_brush)
{
    D3D11_SUBRESOURCE_DATA buffer_data;
    D3D11_BUFFER_DESC buffer_desc;
    ID3D11Buffer *ib, *vb;
    HRESULT hr;

    buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    buffer_desc.CPUAccessFlags = 0;
    buffer_desc.MiscFlags = 0;

    buffer_data.SysMemPitch = 0;
    buffer_data.SysMemSlicePitch = 0;

    if (FAILED(hr = d2d_device_context_update_vs_cb(render_target, &geometry->transform, 0.0f)))
    {
        WARN("Failed to update vs constant buffer, hr %#lx.\n", hr);
        return;
    }

    if (FAILED(hr = d2d_device_context_update_ps_cb(render_target, brush, opacity_brush, FALSE, FALSE)))
    {
        WARN("Failed to update ps constant buffer, hr %#lx.\n", hr);
        return;
    }

    if (geometry->fill.face_count)
    {
        buffer_desc.ByteWidth = geometry->fill.face_count * sizeof(*geometry->fill.faces);
        buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        buffer_data.pSysMem = geometry->fill.faces;

        if (FAILED(hr = ID3D11Device1_CreateBuffer(render_target->d3d_device, &buffer_desc, &buffer_data, &ib)))
        {
            WARN("Failed to create index buffer, hr %#lx.\n", hr);
            return;
        }

        buffer_desc.ByteWidth = geometry->fill.vertex_count * sizeof(*geometry->fill.vertices);
        buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        buffer_data.pSysMem = geometry->fill.vertices;

        if (FAILED(hr = ID3D11Device1_CreateBuffer(render_target->d3d_device, &buffer_desc, &buffer_data, &vb)))
        {
            ERR("Failed to create vertex buffer, hr %#lx.\n", hr);
            ID3D11Buffer_Release(ib);
            return;
        }

        d2d_device_context_draw(render_target, D2D_SHAPE_TYPE_TRIANGLE, ib, 3 * geometry->fill.face_count, vb,
                sizeof(*geometry->fill.vertices), brush, opacity_brush);

        ID3D11Buffer_Release(vb);
        ID3D11Buffer_Release(ib);
    }

    if (geometry->fill.bezier_vertex_count)
    {
        buffer_desc.ByteWidth = geometry->fill.bezier_vertex_count * sizeof(*geometry->fill.bezier_vertices);
        buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        buffer_data.pSysMem = geometry->fill.bezier_vertices;

        if (FAILED(hr = ID3D11Device1_CreateBuffer(render_target->d3d_device, &buffer_desc, &buffer_data, &vb)))
        {
            ERR("Failed to create curves vertex buffer, hr %#lx.\n", hr);
            return;
        }

        d2d_device_context_draw(render_target, D2D_SHAPE_TYPE_CURVE, NULL, geometry->fill.bezier_vertex_count, vb,
                sizeof(*geometry->fill.bezier_vertices), brush, opacity_brush);

        ID3D11Buffer_Release(vb);
    }

    if (geometry->fill.arc_vertex_count)
    {
        buffer_desc.ByteWidth = geometry->fill.arc_vertex_count * sizeof(*geometry->fill.arc_vertices);
        buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        buffer_data.pSysMem = geometry->fill.arc_vertices;

        if (FAILED(hr = ID3D11Device1_CreateBuffer(render_target->d3d_device, &buffer_desc, &buffer_data, &vb)))
        {
            ERR("Failed to create arc vertex buffer, hr %#lx.\n", hr);
            return;
        }

        if (SUCCEEDED(d2d_device_context_update_ps_cb(render_target, brush, opacity_brush, FALSE, TRUE)))
            d2d_device_context_draw(render_target, D2D_SHAPE_TYPE_CURVE, NULL, geometry->fill.arc_vertex_count, vb,
                    sizeof(*geometry->fill.arc_vertices), brush, opacity_brush);

        ID3D11Buffer_Release(vb);
    }
}

static void STDMETHODCALLTYPE d2d_device_context_FillGeometry(ID2D1DeviceContext6 *iface,
        ID2D1Geometry *geometry, ID2D1Brush *brush, ID2D1Brush *opacity_brush)
{
    const struct d2d_geometry *geometry_impl = unsafe_impl_from_ID2D1Geometry(geometry);
    struct d2d_brush *opacity_brush_impl = unsafe_impl_from_ID2D1Brush(opacity_brush);
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);
    struct d2d_brush *brush_impl = unsafe_impl_from_ID2D1Brush(brush);

    TRACE("iface %p, geometry %p, brush %p, opacity_brush %p.\n", iface, geometry, brush, opacity_brush);

    if (FAILED(context->error.code))
        return;

    if (opacity_brush && brush_impl->type != D2D_BRUSH_TYPE_BITMAP)
    {
        d2d_device_context_set_error(context, D2DERR_INCOMPATIBLE_BRUSH_TYPES);
        return;
    }

    if (context->target.type == D2D_TARGET_COMMAND_LIST)
        d2d_command_list_fill_geometry(context->target.command_list, context, geometry, brush, opacity_brush);
    else
        d2d_device_context_fill_geometry(context, geometry_impl, brush_impl, opacity_brush_impl);
}

static void STDMETHODCALLTYPE d2d_device_context_FillMesh(ID2D1DeviceContext6 *iface,
        ID2D1Mesh *mesh, ID2D1Brush *brush)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);

    FIXME("iface %p, mesh %p, brush %p stub!\n", iface, mesh, brush);

    if (context->target.type == D2D_TARGET_COMMAND_LIST)
        d2d_command_list_fill_mesh(context->target.command_list, context, mesh, brush);
}

static void STDMETHODCALLTYPE d2d_device_context_FillOpacityMask(ID2D1DeviceContext6 *iface,
        ID2D1Bitmap *mask, ID2D1Brush *brush, D2D1_OPACITY_MASK_CONTENT content,
        const D2D1_RECT_F *dst_rect, const D2D1_RECT_F *src_rect)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);

    FIXME("iface %p, mask %p, brush %p, content %#x, dst_rect %s, src_rect %s stub!\n",
            iface, mask, brush, content, debug_d2d_rect_f(dst_rect), debug_d2d_rect_f(src_rect));

    if (FAILED(context->error.code))
        return;

    if (context->drawing_state.antialiasMode != D2D1_ANTIALIAS_MODE_ALIASED)
    {
        d2d_device_context_set_error(context, D2DERR_WRONG_STATE);
        return;
    }

    if ((unsigned int)content > D2D1_OPACITY_MASK_CONTENT_TEXT_GDI_COMPATIBLE)
    {
        d2d_device_context_set_error(context, E_INVALIDARG);
        return;
    }

    if (context->target.type == D2D_TARGET_COMMAND_LIST)
        d2d_command_list_fill_opacity_mask(context->target.command_list, context, mask, brush, dst_rect, src_rect);
}

static void d2d_device_context_draw_bitmap(struct d2d_device_context *context, ID2D1Bitmap *bitmap,
        const D2D1_RECT_F *dst_rect, float opacity, D2D1_INTERPOLATION_MODE interpolation_mode,
        const D2D1_RECT_F *src_rect, const D2D1_POINT_2F *offset,
        const D2D1_MATRIX_4X4_F *perspective_transform)
{
    D2D1_BITMAP_BRUSH_PROPERTIES1 bitmap_brush_desc;
    D2D1_BRUSH_PROPERTIES brush_desc;
    struct d2d_brush *brush;
    D2D1_SIZE_F size;
    D2D1_RECT_F s, d;
    HRESULT hr;

    if (perspective_transform)
        FIXME("Perspective transform is ignored.\n");

    size = ID2D1Bitmap_GetSize(bitmap);
    d2d_rect_set(&s, 0.0f, 0.0f, size.width, size.height);
    if (src_rect && src_rect->left <= src_rect->right
            && src_rect->top <= src_rect->bottom)
    {
        d2d_rect_intersect(&s, src_rect);
    }

    if (s.left == s.right || s.top == s.bottom)
        return;

    if (dst_rect)
    {
        d = *dst_rect;
    }
    else
    {
        d.left = 0.0f;
        d.top = 0.0f;
        d.right = s.right - s.left;
        d.bottom = s.bottom - s.top;
    }

    if (offset)
    {
        d.left += offset->x;
        d.top += offset->y;
        d.right += offset->x;
        d.bottom += offset->y;
    }

    bitmap_brush_desc.extendModeX = D2D1_EXTEND_MODE_CLAMP;
    bitmap_brush_desc.extendModeY = D2D1_EXTEND_MODE_CLAMP;
    bitmap_brush_desc.interpolationMode = interpolation_mode;

    brush_desc.opacity = opacity;
    brush_desc.transform._11 = fabsf((d.right - d.left) / (s.right - s.left));
    brush_desc.transform._21 = 0.0f;
    brush_desc.transform._31 = min(d.left, d.right) - min(s.left, s.right) * brush_desc.transform._11;
    brush_desc.transform._12 = 0.0f;
    brush_desc.transform._22 = fabsf((d.bottom - d.top) / (s.bottom - s.top));
    brush_desc.transform._32 = min(d.top, d.bottom) - min(s.top, s.bottom) * brush_desc.transform._22;

    if (FAILED(hr = d2d_bitmap_brush_create(context->factory, bitmap, &bitmap_brush_desc, &brush_desc, &brush)))
    {
        ERR("Failed to create bitmap brush, hr %#lx.\n", hr);
        return;
    }

    d2d_device_context_FillRectangle(&context->ID2D1DeviceContext6_iface, &d, &brush->ID2D1Brush_iface);
    ID2D1Brush_Release(&brush->ID2D1Brush_iface);
}

static void STDMETHODCALLTYPE d2d_device_context_DrawBitmap(ID2D1DeviceContext6 *iface,
        ID2D1Bitmap *bitmap, const D2D1_RECT_F *dst_rect, float opacity,
        D2D1_BITMAP_INTERPOLATION_MODE interpolation_mode, const D2D1_RECT_F *src_rect)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p, bitmap %p, dst_rect %s, opacity %.8e, interpolation_mode %#x, src_rect %s.\n",
            iface, bitmap, debug_d2d_rect_f(dst_rect), opacity, interpolation_mode, debug_d2d_rect_f(src_rect));

    if (interpolation_mode != D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR
            && interpolation_mode != D2D1_BITMAP_INTERPOLATION_MODE_LINEAR)
    {
        d2d_device_context_set_error(context, E_INVALIDARG);
        return;
    }

    if (context->target.type == D2D_TARGET_COMMAND_LIST)
    {
        d2d_command_list_draw_bitmap(context->target.command_list, bitmap, dst_rect, opacity,
                d2d1_1_interp_mode_from_d2d1(interpolation_mode), src_rect, NULL);
    }
    else
    {
        d2d_device_context_draw_bitmap(context, bitmap, dst_rect, opacity,
                d2d1_1_interp_mode_from_d2d1(interpolation_mode), src_rect, NULL, NULL);
    }
}

static void STDMETHODCALLTYPE d2d_device_context_DrawText(ID2D1DeviceContext6 *iface,
        const WCHAR *string, UINT32 string_len, IDWriteTextFormat *text_format, const D2D1_RECT_F *layout_rect,
        ID2D1Brush *brush, D2D1_DRAW_TEXT_OPTIONS options, DWRITE_MEASURING_MODE measuring_mode)
{
    struct d2d_device_context *render_target = impl_from_ID2D1DeviceContext(iface);
    IDWriteTextLayout *text_layout;
    IDWriteFactory *dwrite_factory;
    D2D1_POINT_2F origin;
    float width, height;
    HRESULT hr;

    TRACE("iface %p, string %s, string_len %u, text_format %p, layout_rect %s, "
            "brush %p, options %#x, measuring_mode %#x.\n",
            iface, debugstr_wn(string, string_len), string_len, text_format, debug_d2d_rect_f(layout_rect),
            brush, options, measuring_mode);

    if (FAILED(hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
            &IID_IDWriteFactory, (IUnknown **)&dwrite_factory)))
    {
        ERR("Failed to create dwrite factory, hr %#lx.\n", hr);
        return;
    }

    width = max(0.0f, layout_rect->right - layout_rect->left);
    height = max(0.0f, layout_rect->bottom - layout_rect->top);
    if (measuring_mode == DWRITE_MEASURING_MODE_NATURAL)
        hr = IDWriteFactory_CreateTextLayout(dwrite_factory, string, string_len, text_format,
                width, height, &text_layout);
    else
        hr = IDWriteFactory_CreateGdiCompatibleTextLayout(dwrite_factory, string, string_len, text_format,
                width, height, render_target->desc.dpiX / 96.0f, (DWRITE_MATRIX *)&render_target->drawing_state.transform,
                measuring_mode == DWRITE_MEASURING_MODE_GDI_NATURAL, &text_layout);
    IDWriteFactory_Release(dwrite_factory);
    if (FAILED(hr))
    {
        ERR("Failed to create text layout, hr %#lx.\n", hr);
        return;
    }

    d2d_point_set(&origin, min(layout_rect->left, layout_rect->right), min(layout_rect->top, layout_rect->bottom));
    ID2D1DeviceContext1_DrawTextLayout((ID2D1DeviceContext1 *)iface, origin, text_layout, brush, options);
    IDWriteTextLayout_Release(text_layout);
}

static void STDMETHODCALLTYPE d2d_device_context_DrawTextLayout(ID2D1DeviceContext6 *iface,
        D2D1_POINT_2F origin, IDWriteTextLayout *layout, ID2D1Brush *brush, D2D1_DRAW_TEXT_OPTIONS options)
{
    struct d2d_device_context *render_target = impl_from_ID2D1DeviceContext(iface);
    struct d2d_draw_text_layout_ctx ctx;
    HRESULT hr;

    TRACE("iface %p, origin %s, layout %p, brush %p, options %#x.\n",
            iface, debug_d2d_point_2f(&origin), layout, brush, options);

    ctx.brush = brush;
    ctx.options = options;

    if (FAILED(hr = IDWriteTextLayout_Draw(layout,
            &ctx, &render_target->IDWriteTextRenderer_iface, origin.x, origin.y)))
        FIXME("Failed to draw text layout, hr %#lx.\n", hr);
}

static D2D1_ANTIALIAS_MODE d2d_device_context_set_aa_mode_from_text_aa_mode(struct d2d_device_context *rt)
{
    D2D1_ANTIALIAS_MODE prev_antialias_mode = rt->drawing_state.antialiasMode;
    rt->drawing_state.antialiasMode = rt->drawing_state.textAntialiasMode == D2D1_TEXT_ANTIALIAS_MODE_ALIASED ?
            D2D1_ANTIALIAS_MODE_ALIASED : D2D1_ANTIALIAS_MODE_PER_PRIMITIVE;
    return prev_antialias_mode;
}

static void d2d_device_context_draw_glyph_run_outline(struct d2d_device_context *render_target,
        D2D1_POINT_2F baseline_origin, const DWRITE_GLYPH_RUN *glyph_run, ID2D1Brush *brush)
{
    D2D1_MATRIX_3X2_F *transform, prev_transform;
    D2D1_ANTIALIAS_MODE prev_antialias_mode;
    ID2D1PathGeometry *geometry;
    ID2D1GeometrySink *sink;
    HRESULT hr;

    if (FAILED(hr = ID2D1Factory_CreatePathGeometry(render_target->factory, &geometry)))
    {
        ERR("Failed to create geometry, hr %#lx.\n", hr);
        return;
    }

    if (FAILED(hr = ID2D1PathGeometry_Open(geometry, &sink)))
    {
        ERR("Failed to open geometry sink, hr %#lx.\n", hr);
        ID2D1PathGeometry_Release(geometry);
        return;
    }

    if (FAILED(hr = IDWriteFontFace_GetGlyphRunOutline(glyph_run->fontFace, glyph_run->fontEmSize,
            glyph_run->glyphIndices, glyph_run->glyphAdvances, glyph_run->glyphOffsets, glyph_run->glyphCount,
            glyph_run->isSideways, glyph_run->bidiLevel & 1, (IDWriteGeometrySink *)sink)))
    {
        ERR("Failed to get glyph run outline, hr %#lx.\n", hr);
        ID2D1GeometrySink_Release(sink);
        ID2D1PathGeometry_Release(geometry);
        return;
    }

    if (FAILED(hr = ID2D1GeometrySink_Close(sink)))
        ERR("Failed to close geometry sink, hr %#lx.\n", hr);
    ID2D1GeometrySink_Release(sink);

    transform = &render_target->drawing_state.transform;
    prev_transform = *transform;
    transform->_31 += baseline_origin.x * transform->_11 + baseline_origin.y * transform->_21;
    transform->_32 += baseline_origin.x * transform->_12 + baseline_origin.y * transform->_22;
    prev_antialias_mode = d2d_device_context_set_aa_mode_from_text_aa_mode(render_target);
    d2d_device_context_fill_geometry(render_target, unsafe_impl_from_ID2D1Geometry((ID2D1Geometry *)geometry),
            unsafe_impl_from_ID2D1Brush(brush), NULL);
    render_target->drawing_state.antialiasMode = prev_antialias_mode;
    *transform = prev_transform;

    ID2D1PathGeometry_Release(geometry);
}

static void d2d_device_context_draw_glyph_run_bitmap(struct d2d_device_context *context,
        D2D1_POINT_2F baseline_origin, const DWRITE_GLYPH_RUN *glyph_run, ID2D1Brush *brush,
        DWRITE_RENDERING_MODE rendering_mode, DWRITE_MEASURING_MODE measuring_mode,
        DWRITE_TEXT_ANTIALIAS_MODE antialias_mode)
{
    ID2D1RectangleGeometry *geometry = NULL;
    ID2D1BitmapBrush *opacity_brush = NULL;
    D2D1_BITMAP_PROPERTIES bitmap_desc;
    ID2D1Bitmap *opacity_bitmap = NULL;
    IDWriteGlyphRunAnalysis *analysis;
    DWRITE_TEXTURE_TYPE texture_type;
    D2D1_BRUSH_PROPERTIES brush_desc;
    IDWriteFactory2 *dwrite_factory;
    D2D1_MATRIX_3X2_F *transform, m;
    void *opacity_values = NULL;
    size_t opacity_values_size;
    D2D1_SIZE_U bitmap_size;
    float scale_x, scale_y;
    D2D1_RECT_F run_rect;
    RECT bounds;
    HRESULT hr;

    if (FAILED(hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
            &IID_IDWriteFactory2, (IUnknown **)&dwrite_factory)))
    {
        ERR("Failed to create dwrite factory, hr %#lx.\n", hr);
        return;
    }

    transform = &context->drawing_state.transform;

    scale_x = context->desc.dpiX / 96.0f;
    m._11 = transform->_11 * scale_x;
    m._21 = transform->_21 * scale_x;
    m._31 = transform->_31 * scale_x;

    scale_y = context->desc.dpiY / 96.0f;
    m._12 = transform->_12 * scale_y;
    m._22 = transform->_22 * scale_y;
    m._32 = transform->_32 * scale_y;

    hr = IDWriteFactory2_CreateGlyphRunAnalysis(dwrite_factory, glyph_run, (DWRITE_MATRIX *)&m,
            rendering_mode, measuring_mode, DWRITE_GRID_FIT_MODE_DEFAULT, antialias_mode,
            baseline_origin.x, baseline_origin.y, &analysis);
    IDWriteFactory2_Release(dwrite_factory);
    if (FAILED(hr))
    {
        ERR("Failed to create glyph run analysis, hr %#lx.\n", hr);
        return;
    }

    if (rendering_mode == DWRITE_RENDERING_MODE_ALIASED || antialias_mode == DWRITE_TEXT_ANTIALIAS_MODE_GRAYSCALE)
        texture_type = DWRITE_TEXTURE_ALIASED_1x1;
    else
        texture_type = DWRITE_TEXTURE_CLEARTYPE_3x1;

    if (FAILED(hr = IDWriteGlyphRunAnalysis_GetAlphaTextureBounds(analysis, texture_type, &bounds)))
    {
        ERR("Failed to get alpha texture bounds, hr %#lx.\n", hr);
        goto done;
    }

    d2d_size_set(&bitmap_size, bounds.right - bounds.left, bounds.bottom - bounds.top);
    if (!bitmap_size.width || !bitmap_size.height)
    {
        /* Empty run, nothing to do. */
        goto done;
    }

    if (texture_type == DWRITE_TEXTURE_CLEARTYPE_3x1)
        bitmap_size.width *= 3;
    if (!(opacity_values = calloc(bitmap_size.height, bitmap_size.width)))
    {
        ERR("Failed to allocate opacity values.\n");
        goto done;
    }
    opacity_values_size = bitmap_size.height * bitmap_size.width;

    if (FAILED(hr = IDWriteGlyphRunAnalysis_CreateAlphaTexture(analysis,
            texture_type, &bounds, opacity_values, opacity_values_size)))
    {
        ERR("Failed to create alpha texture, hr %#lx.\n", hr);
        goto done;
    }

    bitmap_desc.pixelFormat.format = DXGI_FORMAT_A8_UNORM;
    bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    bitmap_desc.dpiX = context->desc.dpiX;
    if (texture_type == DWRITE_TEXTURE_CLEARTYPE_3x1)
        bitmap_desc.dpiX *= 3.0f;
    bitmap_desc.dpiY = context->desc.dpiY;
    if (FAILED(hr = d2d_device_context_CreateBitmap(&context->ID2D1DeviceContext6_iface,
            bitmap_size, opacity_values, bitmap_size.width, &bitmap_desc, &opacity_bitmap)))
    {
        ERR("Failed to create opacity bitmap, hr %#lx.\n", hr);
        goto done;
    }

    d2d_rect_set(&run_rect, bounds.left / scale_x, bounds.top / scale_y,
            bounds.right / scale_x, bounds.bottom / scale_y);

    brush_desc.opacity = 1.0f;
    brush_desc.transform._11 = 1.0f;
    brush_desc.transform._12 = 0.0f;
    brush_desc.transform._21 = 0.0f;
    brush_desc.transform._22 = 1.0f;
    brush_desc.transform._31 = run_rect.left;
    brush_desc.transform._32 = run_rect.top;
    if (FAILED(hr = d2d_device_context_CreateBitmapBrush(&context->ID2D1DeviceContext6_iface,
            opacity_bitmap, NULL, &brush_desc, &opacity_brush)))
    {
        ERR("Failed to create opacity bitmap brush, hr %#lx.\n", hr);
        goto done;
    }

    if (FAILED(hr = ID2D1Factory_CreateRectangleGeometry(context->factory, &run_rect, &geometry)))
    {
        ERR("Failed to create geometry, hr %#lx.\n", hr);
        goto done;
    }

    m = *transform;
    *transform = identity;
    d2d_device_context_fill_geometry(context, unsafe_impl_from_ID2D1Geometry((ID2D1Geometry *)geometry),
            unsafe_impl_from_ID2D1Brush(brush), unsafe_impl_from_ID2D1Brush((ID2D1Brush *)opacity_brush));
    *transform = m;

done:
    if (geometry)
        ID2D1RectangleGeometry_Release(geometry);
    if (opacity_brush)
        ID2D1BitmapBrush_Release(opacity_brush);
    if (opacity_bitmap)
        ID2D1Bitmap_Release(opacity_bitmap);
    free(opacity_values);
    IDWriteGlyphRunAnalysis_Release(analysis);
}

static void d2d_device_context_draw_glyph_run(struct d2d_device_context *context,
        D2D1_POINT_2F baseline_origin, const DWRITE_GLYPH_RUN *glyph_run,
        const DWRITE_GLYPH_RUN_DESCRIPTION *glyph_run_desc, ID2D1Brush *brush, DWRITE_MEASURING_MODE measuring_mode)
{
    DWRITE_TEXT_ANTIALIAS_MODE antialias_mode = DWRITE_TEXT_ANTIALIAS_MODE_GRAYSCALE;
    IDWriteRenderingParams *rendering_params;
    DWRITE_RENDERING_MODE rendering_mode;
    HRESULT hr;

    if (FAILED(context->error.code))
        return;

    if (context->target.type == D2D_TARGET_COMMAND_LIST)
    {
        d2d_command_list_draw_glyph_run(context->target.command_list, context, baseline_origin, glyph_run,
                glyph_run_desc, brush, measuring_mode);
        return;
    }

    rendering_params = context->text_rendering_params ? context->text_rendering_params
            : context->default_text_rendering_params;

    rendering_mode = IDWriteRenderingParams_GetRenderingMode(rendering_params);

    switch (context->drawing_state.textAntialiasMode)
    {
        case D2D1_TEXT_ANTIALIAS_MODE_ALIASED:
            if (rendering_mode == DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL
                    || rendering_mode == DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL_SYMMETRIC
                    || rendering_mode == DWRITE_RENDERING_MODE_CLEARTYPE_GDI_NATURAL
                    || rendering_mode == DWRITE_RENDERING_MODE_CLEARTYPE_GDI_CLASSIC)
                d2d_device_context_set_error(context, E_INVALIDARG);
            break;

        case D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE:
            if (rendering_mode == DWRITE_RENDERING_MODE_ALIASED
                    || rendering_mode == DWRITE_RENDERING_MODE_OUTLINE)
                d2d_device_context_set_error(context, E_INVALIDARG);
            break;

        case D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE:
            if (rendering_mode == DWRITE_RENDERING_MODE_ALIASED)
                d2d_device_context_set_error(context, E_INVALIDARG);
            break;

        default:
            break;
    }

    if (FAILED(context->error.code))
        return;

    rendering_mode = DWRITE_RENDERING_MODE_DEFAULT;
    switch (context->drawing_state.textAntialiasMode)
    {
        case D2D1_TEXT_ANTIALIAS_MODE_DEFAULT:
            if (IDWriteRenderingParams_GetClearTypeLevel(rendering_params) > 0.0f)
                antialias_mode = DWRITE_TEXT_ANTIALIAS_MODE_CLEARTYPE;
            break;

        case D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE:
            antialias_mode = DWRITE_TEXT_ANTIALIAS_MODE_CLEARTYPE;
            break;

        case D2D1_TEXT_ANTIALIAS_MODE_ALIASED:
            rendering_mode = DWRITE_RENDERING_MODE_ALIASED;
            break;

        default:
            break;
    }

    if (rendering_mode == DWRITE_RENDERING_MODE_DEFAULT)
    {
        if (FAILED(hr = IDWriteFontFace_GetRecommendedRenderingMode(glyph_run->fontFace, glyph_run->fontEmSize,
                max(context->desc.dpiX, context->desc.dpiY) / 96.0f,
                measuring_mode, rendering_params, &rendering_mode)))
        {
            ERR("Failed to get recommended rendering mode, hr %#lx.\n", hr);
            rendering_mode = DWRITE_RENDERING_MODE_OUTLINE;
        }
    }

    if (rendering_mode == DWRITE_RENDERING_MODE_OUTLINE)
        d2d_device_context_draw_glyph_run_outline(context, baseline_origin, glyph_run, brush);
    else
        d2d_device_context_draw_glyph_run_bitmap(context, baseline_origin, glyph_run, brush,
                rendering_mode, measuring_mode, antialias_mode);
}

static void STDMETHODCALLTYPE d2d_device_context_DrawGlyphRun(ID2D1DeviceContext6 *iface,
        D2D1_POINT_2F baseline_origin, const DWRITE_GLYPH_RUN *glyph_run, ID2D1Brush *brush,
        DWRITE_MEASURING_MODE measuring_mode)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p, baseline_origin %s, glyph_run %p, brush %p, measuring_mode %#x.\n",
            iface, debug_d2d_point_2f(&baseline_origin), glyph_run, brush, measuring_mode);

    d2d_device_context_draw_glyph_run(context, baseline_origin, glyph_run, NULL, brush, measuring_mode);
}

static void STDMETHODCALLTYPE d2d_device_context_SetTransform(ID2D1DeviceContext6 *iface,
        const D2D1_MATRIX_3X2_F *transform)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p, transform %p.\n", iface, transform);

    if (context->target.type == D2D_TARGET_COMMAND_LIST)
        d2d_command_list_set_transform(context->target.command_list, transform);

    context->drawing_state.transform = *transform;
}

static void STDMETHODCALLTYPE d2d_device_context_GetTransform(ID2D1DeviceContext6 *iface,
        D2D1_MATRIX_3X2_F *transform)
{
    struct d2d_device_context *render_target = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p, transform %p.\n", iface, transform);

    *transform = render_target->drawing_state.transform;
}

static void STDMETHODCALLTYPE d2d_device_context_SetAntialiasMode(ID2D1DeviceContext6 *iface,
        D2D1_ANTIALIAS_MODE antialias_mode)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p, antialias_mode %#x stub!\n", iface, antialias_mode);

    if (context->target.type == D2D_TARGET_COMMAND_LIST)
        d2d_command_list_set_antialias_mode(context->target.command_list, antialias_mode);

    context->drawing_state.antialiasMode = antialias_mode;
}

static D2D1_ANTIALIAS_MODE STDMETHODCALLTYPE d2d_device_context_GetAntialiasMode(ID2D1DeviceContext6 *iface)
{
    struct d2d_device_context *render_target = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p.\n", iface);

    return render_target->drawing_state.antialiasMode;
}

static void STDMETHODCALLTYPE d2d_device_context_SetTextAntialiasMode(ID2D1DeviceContext6 *iface,
        D2D1_TEXT_ANTIALIAS_MODE antialias_mode)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p, antialias_mode %#x.\n", iface, antialias_mode);

    if (context->target.type == D2D_TARGET_COMMAND_LIST)
        d2d_command_list_set_text_antialias_mode(context->target.command_list, antialias_mode);

    context->drawing_state.textAntialiasMode = antialias_mode;
}

static D2D1_TEXT_ANTIALIAS_MODE STDMETHODCALLTYPE d2d_device_context_GetTextAntialiasMode(ID2D1DeviceContext6 *iface)
{
    struct d2d_device_context *render_target = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p.\n", iface);

    return render_target->drawing_state.textAntialiasMode;
}

static void STDMETHODCALLTYPE d2d_device_context_SetTextRenderingParams(ID2D1DeviceContext6 *iface,
        IDWriteRenderingParams *text_rendering_params)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p, text_rendering_params %p.\n", iface, text_rendering_params);

    if (context->target.type == D2D_TARGET_COMMAND_LIST)
        d2d_command_list_set_text_rendering_params(context->target.command_list, text_rendering_params);

    if (text_rendering_params)
        IDWriteRenderingParams_AddRef(text_rendering_params);
    if (context->text_rendering_params)
        IDWriteRenderingParams_Release(context->text_rendering_params);
    context->text_rendering_params = text_rendering_params;
}

static void STDMETHODCALLTYPE d2d_device_context_GetTextRenderingParams(ID2D1DeviceContext6 *iface,
        IDWriteRenderingParams **text_rendering_params)
{
    struct d2d_device_context *render_target = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p, text_rendering_params %p.\n", iface, text_rendering_params);

    if ((*text_rendering_params = render_target->text_rendering_params))
        IDWriteRenderingParams_AddRef(*text_rendering_params);
}

static void STDMETHODCALLTYPE d2d_device_context_SetTags(ID2D1DeviceContext6 *iface, D2D1_TAG tag1, D2D1_TAG tag2)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p, tag1 %s, tag2 %s.\n", iface, wine_dbgstr_longlong(tag1), wine_dbgstr_longlong(tag2));

    if (context->target.type == D2D_TARGET_COMMAND_LIST)
        d2d_command_list_set_tags(context->target.command_list, tag1, tag2);

    context->drawing_state.tag1 = tag1;
    context->drawing_state.tag2 = tag2;
}

static void STDMETHODCALLTYPE d2d_device_context_GetTags(ID2D1DeviceContext6 *iface, D2D1_TAG *tag1, D2D1_TAG *tag2)
{
    struct d2d_device_context *render_target = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p, tag1 %p, tag2 %p.\n", iface, tag1, tag2);

    *tag1 = render_target->drawing_state.tag1;
    *tag2 = render_target->drawing_state.tag2;
}

static void STDMETHODCALLTYPE d2d_device_context_PushLayer(ID2D1DeviceContext6 *iface,
        const D2D1_LAYER_PARAMETERS *layer_parameters, ID2D1Layer *layer)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);

    FIXME("iface %p, layer_parameters %p, layer %p stub!\n", iface, layer_parameters, layer);

    if (context->target.type == D2D_TARGET_COMMAND_LIST)
    {
        D2D1_LAYER_PARAMETERS1 parameters;

        memcpy(&parameters, layer_parameters, sizeof(*layer_parameters));
        parameters.layerOptions = D2D1_LAYER_OPTIONS1_NONE;
        d2d_command_list_push_layer(context->target.command_list, context, &parameters, layer);
    }
}

static void STDMETHODCALLTYPE d2d_device_context_PopLayer(ID2D1DeviceContext6 *iface)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);

    FIXME("iface %p stub!\n", iface);

    if (context->target.type == D2D_TARGET_COMMAND_LIST)
        d2d_command_list_pop_layer(context->target.command_list);
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_Flush(ID2D1DeviceContext6 *iface, D2D1_TAG *tag1, D2D1_TAG *tag2)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);

    FIXME("iface %p, tag1 %p, tag2 %p stub!\n", iface, tag1, tag2);

    if (context->ops && context->ops->device_context_present)
        context->ops->device_context_present(context->outer_unknown);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d2d_device_context_SaveDrawingState(ID2D1DeviceContext6 *iface,
        ID2D1DrawingStateBlock *state_block)
{
    struct d2d_device_context *render_target = impl_from_ID2D1DeviceContext(iface);
    struct d2d_state_block *state_block_impl;

    TRACE("iface %p, state_block %p.\n", iface, state_block);

    if (!(state_block_impl = unsafe_impl_from_ID2D1DrawingStateBlock(state_block))) return;
    state_block_impl->drawing_state = render_target->drawing_state;
    if (render_target->text_rendering_params)
        IDWriteRenderingParams_AddRef(render_target->text_rendering_params);
    if (state_block_impl->text_rendering_params)
        IDWriteRenderingParams_Release(state_block_impl->text_rendering_params);
    state_block_impl->text_rendering_params = render_target->text_rendering_params;
}

static void STDMETHODCALLTYPE d2d_device_context_RestoreDrawingState(ID2D1DeviceContext6 *iface,
        ID2D1DrawingStateBlock *state_block)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);
    struct d2d_state_block *state_block_impl;

    TRACE("iface %p, state_block %p.\n", iface, state_block);

    if (!(state_block_impl = unsafe_impl_from_ID2D1DrawingStateBlock(state_block))) return;
    if (context->target.type == D2D_TARGET_COMMAND_LIST)
    {
        struct d2d_command_list *command_list = context->target.command_list;

        if (context->drawing_state.antialiasMode != state_block_impl->drawing_state.antialiasMode)
            d2d_command_list_set_antialias_mode(command_list, state_block_impl->drawing_state.antialiasMode);
        d2d_command_list_set_text_antialias_mode(command_list, state_block_impl->drawing_state.textAntialiasMode);
        d2d_command_list_set_tags(command_list, state_block_impl->drawing_state.tag1, state_block_impl->drawing_state.tag2);
        d2d_command_list_set_transform(command_list, &state_block_impl->drawing_state.transform);
        d2d_command_list_set_primitive_blend(command_list, state_block_impl->drawing_state.primitiveBlend);
        d2d_command_list_set_unit_mode(command_list, state_block_impl->drawing_state.unitMode);
        d2d_command_list_set_text_rendering_params(command_list, state_block_impl->text_rendering_params);
    }

    context->drawing_state = state_block_impl->drawing_state;
    if (state_block_impl->text_rendering_params)
        IDWriteRenderingParams_AddRef(state_block_impl->text_rendering_params);
    if (context->text_rendering_params)
        IDWriteRenderingParams_Release(context->text_rendering_params);
    context->text_rendering_params = state_block_impl->text_rendering_params;
}

static void STDMETHODCALLTYPE d2d_device_context_PushAxisAlignedClip(ID2D1DeviceContext6 *iface,
        const D2D1_RECT_F *clip_rect, D2D1_ANTIALIAS_MODE antialias_mode)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);
    D2D1_RECT_F transformed_rect;
    float x_scale, y_scale;
    D2D1_POINT_2F point;

    TRACE("iface %p, clip_rect %s, antialias_mode %#x.\n", iface, debug_d2d_rect_f(clip_rect), antialias_mode);

    if (context->target.type == D2D_TARGET_COMMAND_LIST)
        d2d_command_list_push_clip(context->target.command_list, clip_rect, antialias_mode);

    if (antialias_mode != D2D1_ANTIALIAS_MODE_ALIASED)
        FIXME("Ignoring antialias_mode %#x.\n", antialias_mode);

    x_scale = context->desc.dpiX / 96.0f;
    y_scale = context->desc.dpiY / 96.0f;
    d2d_point_transform(&point, &context->drawing_state.transform,
            clip_rect->left * x_scale, clip_rect->top * y_scale);
    d2d_rect_set(&transformed_rect, point.x, point.y, point.x, point.y);
    d2d_point_transform(&point, &context->drawing_state.transform,
            clip_rect->left * x_scale, clip_rect->bottom * y_scale);
    d2d_rect_expand(&transformed_rect, &point);
    d2d_point_transform(&point, &context->drawing_state.transform,
            clip_rect->right * x_scale, clip_rect->top * y_scale);
    d2d_rect_expand(&transformed_rect, &point);
    d2d_point_transform(&point, &context->drawing_state.transform,
            clip_rect->right * x_scale, clip_rect->bottom * y_scale);
    d2d_rect_expand(&transformed_rect, &point);

    if (!d2d_clip_stack_push(&context->clip_stack, &transformed_rect))
        WARN("Failed to push clip rect.\n");
}

static void STDMETHODCALLTYPE d2d_device_context_PopAxisAlignedClip(ID2D1DeviceContext6 *iface)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p.\n", iface);

    if (context->target.type == D2D_TARGET_COMMAND_LIST)
        d2d_command_list_pop_clip(context->target.command_list);

    d2d_clip_stack_pop(&context->clip_stack);
}

static void STDMETHODCALLTYPE d2d_device_context_Clear(ID2D1DeviceContext6 *iface, const D2D1_COLOR_F *colour)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);
    D3D11_MAPPED_SUBRESOURCE map_desc;
    ID3D11DeviceContext *d3d_context;
    struct d2d_ps_cb *ps_cb_data;
    struct d2d_vs_cb *vs_cb_data;
    D2D1_COLOR_F *c;
    HRESULT hr;

    TRACE("iface %p, colour %p.\n", iface, colour);

    if (context->target.type == D2D_TARGET_COMMAND_LIST)
    {
        d2d_command_list_clear(context->target.command_list, colour);
        return;
    }

    ID3D11Device1_GetImmediateContext(context->d3d_device, &d3d_context);

    if (FAILED(hr = ID3D11DeviceContext_Map(d3d_context, (ID3D11Resource *)context->vs_cb,
            0, D3D11_MAP_WRITE_DISCARD, 0, &map_desc)))
    {
        WARN("Failed to map vs constant buffer, hr %#lx.\n", hr);
        ID3D11DeviceContext_Release(d3d_context);
        return;
    }

    vs_cb_data = map_desc.pData;
    vs_cb_data->transform_geometry._11 = 1.0f;
    vs_cb_data->transform_geometry._21 = 0.0f;
    vs_cb_data->transform_geometry._31 = 0.0f;
    vs_cb_data->transform_geometry.pad0 = 0.0f;
    vs_cb_data->transform_geometry._12 = 0.0f;
    vs_cb_data->transform_geometry._22 = 1.0f;
    vs_cb_data->transform_geometry._32 = 0.0f;
    vs_cb_data->transform_geometry.stroke_width = 0.0f;
    vs_cb_data->transform_rtx.x = 1.0f;
    vs_cb_data->transform_rtx.y = 0.0f;
    vs_cb_data->transform_rtx.z = 1.0f;
    vs_cb_data->transform_rtx.w = 1.0f;
    vs_cb_data->transform_rty.x = 0.0f;
    vs_cb_data->transform_rty.y = 1.0f;
    vs_cb_data->transform_rty.z = 1.0f;
    vs_cb_data->transform_rty.w = -1.0f;

    ID3D11DeviceContext_Unmap(d3d_context, (ID3D11Resource *)context->vs_cb, 0);

    if (FAILED(hr = ID3D11DeviceContext_Map(d3d_context, (ID3D11Resource *)context->ps_cb,
            0, D3D11_MAP_WRITE_DISCARD, 0, &map_desc)))
    {
        WARN("Failed to map ps constant buffer, hr %#lx.\n", hr);
        ID3D11DeviceContext_Release(d3d_context);
        return;
    }

    ps_cb_data = map_desc.pData;
    memset(ps_cb_data, 0, sizeof(*ps_cb_data));
    ps_cb_data->colour_brush.type = D2D_BRUSH_TYPE_SOLID;
    ps_cb_data->colour_brush.opacity = 1.0f;
    ps_cb_data->opacity_brush.type = D2D_BRUSH_TYPE_COUNT;
    c = &ps_cb_data->colour_brush.u.solid.colour;
    if (colour)
        *c = *colour;
    if (context->desc.pixelFormat.alphaMode == D2D1_ALPHA_MODE_IGNORE)
        c->a = 1.0f;
    c->r *= c->a;
    c->g *= c->a;
    c->b *= c->a;

    ID3D11DeviceContext_Unmap(d3d_context, (ID3D11Resource *)context->ps_cb, 0);
    ID3D11DeviceContext_Release(d3d_context);

    d2d_device_context_draw(context, D2D_SHAPE_TYPE_TRIANGLE, context->ib, 6,
            context->vb, context->vb_stride, NULL, NULL);
}

static void STDMETHODCALLTYPE d2d_device_context_BeginDraw(ID2D1DeviceContext6 *iface)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p.\n", iface);

    if (context->target.type == D2D_TARGET_COMMAND_LIST)
        d2d_command_list_begin_draw(context->target.command_list, context);

    memset(&context->error, 0, sizeof(context->error));
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_EndDraw(ID2D1DeviceContext6 *iface,
        D2D1_TAG *tag1, D2D1_TAG *tag2)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);
    HRESULT hr;

    TRACE("iface %p, tag1 %p, tag2 %p.\n", iface, tag1, tag2);

    if (context->target.type == D2D_TARGET_COMMAND_LIST)
    {
        FIXME("Unimplemented for command list target.\n");
        return E_NOTIMPL;
    }

    if (tag1)
        *tag1 = context->error.tag1;
    if (tag2)
        *tag2 = context->error.tag2;

    if (context->ops && context->ops->device_context_present)
    {
        if (FAILED(hr = context->ops->device_context_present(context->outer_unknown)))
            context->error.code = hr;
    }

    return context->error.code;
}

static D2D1_PIXEL_FORMAT * STDMETHODCALLTYPE d2d_device_context_GetPixelFormat(ID2D1DeviceContext6 *iface,
        D2D1_PIXEL_FORMAT *format)
{
    struct d2d_device_context *render_target = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p, format %p.\n", iface, format);

    *format = render_target->desc.pixelFormat;
    return format;
}

static void STDMETHODCALLTYPE d2d_device_context_SetDpi(ID2D1DeviceContext6 *iface, float dpi_x, float dpi_y)
{
    struct d2d_device_context *render_target = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p, dpi_x %.8e, dpi_y %.8e.\n", iface, dpi_x, dpi_y);

    if (dpi_x == 0.0f && dpi_y == 0.0f)
    {
        dpi_x = 96.0f;
        dpi_y = 96.0f;
    }
    else if (dpi_x <= 0.0f || dpi_y <= 0.0f)
        return;

    render_target->desc.dpiX = dpi_x;
    render_target->desc.dpiY = dpi_y;
}

static void STDMETHODCALLTYPE d2d_device_context_GetDpi(ID2D1DeviceContext6 *iface, float *dpi_x, float *dpi_y)
{
    struct d2d_device_context *render_target = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p, dpi_x %p, dpi_y %p.\n", iface, dpi_x, dpi_y);

    *dpi_x = render_target->desc.dpiX;
    *dpi_y = render_target->desc.dpiY;
}

static D2D1_SIZE_F * STDMETHODCALLTYPE d2d_device_context_GetSize(ID2D1DeviceContext6 *iface, D2D1_SIZE_F *size)
{
    struct d2d_device_context *render_target = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p, size %p.\n", iface, size);

    size->width = render_target->pixel_size.width / (render_target->desc.dpiX / 96.0f);
    size->height = render_target->pixel_size.height / (render_target->desc.dpiY / 96.0f);
    return size;
}

static D2D1_SIZE_U * STDMETHODCALLTYPE d2d_device_context_GetPixelSize(ID2D1DeviceContext6 *iface,
        D2D1_SIZE_U *pixel_size)
{
    struct d2d_device_context *render_target = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p, pixel_size %p.\n", iface, pixel_size);

    *pixel_size = render_target->pixel_size;
    return pixel_size;
}

static UINT32 STDMETHODCALLTYPE d2d_device_context_GetMaximumBitmapSize(ID2D1DeviceContext6 *iface)
{
    TRACE("iface %p.\n", iface);

    return D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
}

static BOOL STDMETHODCALLTYPE d2d_device_context_IsSupported(ID2D1DeviceContext6 *iface,
        const D2D1_RENDER_TARGET_PROPERTIES *desc)
{
    FIXME("iface %p, desc %p stub!\n", iface, desc);

    return FALSE;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_ID2D1DeviceContext_CreateBitmap(ID2D1DeviceContext6 *iface,
        D2D1_SIZE_U size, const void *src_data, UINT32 pitch,
        const D2D1_BITMAP_PROPERTIES1 *desc, ID2D1Bitmap1 **bitmap)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);
    struct d2d_bitmap *object;
    HRESULT hr;

    TRACE("iface %p, size {%u, %u}, src_data %p, pitch %u, desc %p, bitmap %p.\n",
            iface, size.width, size.height, src_data, pitch, desc, bitmap);

    if (SUCCEEDED(hr = d2d_bitmap_create(context, size, src_data, pitch, desc, &object)))
        *bitmap = &object->ID2D1Bitmap1_iface;

    return hr;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_ID2D1DeviceContext_CreateBitmapFromWicBitmap(
        ID2D1DeviceContext6 *iface, IWICBitmapSource *bitmap_source,
        const D2D1_BITMAP_PROPERTIES1 *desc, ID2D1Bitmap1 **bitmap)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);
    struct d2d_bitmap *object;
    HRESULT hr;

    TRACE("iface %p, bitmap_source %p, desc %p, bitmap %p.\n", iface, bitmap_source, desc, bitmap);

    if (SUCCEEDED(hr = d2d_bitmap_create_from_wic_bitmap(context, bitmap_source, desc, &object)))
        *bitmap = &object->ID2D1Bitmap1_iface;

    return hr;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateColorContext(ID2D1DeviceContext6 *iface,
        D2D1_COLOR_SPACE space, const BYTE *profile, UINT32 profile_size, ID2D1ColorContext **color_context)
{
    FIXME("iface %p, space %#x, profile %p, profile_size %u, color_context %p stub!\n",
            iface, space, profile, profile_size, color_context);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateColorContextFromFilename(ID2D1DeviceContext6 *iface,
        const WCHAR *filename, ID2D1ColorContext **color_context)
{
    FIXME("iface %p, filename %s, color_context %p stub!\n", iface, debugstr_w(filename), color_context);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateColorContextFromWicColorContext(ID2D1DeviceContext6 *iface,
        IWICColorContext *wic_color_context, ID2D1ColorContext **color_context)
{
    FIXME("iface %p, wic_color_context %p, color_context %p stub!\n", iface, wic_color_context, color_context);

    return E_NOTIMPL;
}

static BOOL d2d_bitmap_check_options_with_surface(unsigned int options, unsigned int surface_options)
{
    switch (options)
    {
        case D2D1_BITMAP_OPTIONS_NONE:
        case D2D1_BITMAP_OPTIONS_TARGET:
        case D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW:
        case D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW | D2D1_BITMAP_OPTIONS_GDI_COMPATIBLE:
        case D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_GDI_COMPATIBLE:
        case D2D1_BITMAP_OPTIONS_CANNOT_DRAW | D2D1_BITMAP_OPTIONS_CPU_READ:
        case D2D1_BITMAP_OPTIONS_CANNOT_DRAW:
            break;
        default:
            WARN("Invalid bitmap options %#x.\n", options);
            return FALSE;
    }

    if (options && (options & D2D1_BITMAP_OPTIONS_TARGET) != (surface_options & D2D1_BITMAP_OPTIONS_TARGET))
        return FALSE;
    if (!(options & D2D1_BITMAP_OPTIONS_CANNOT_DRAW) && (surface_options & D2D1_BITMAP_OPTIONS_CANNOT_DRAW))
        return FALSE;
    if (options & D2D1_BITMAP_OPTIONS_TARGET)
    {
        if (options & D2D1_BITMAP_OPTIONS_GDI_COMPATIBLE && !(surface_options & D2D1_BITMAP_OPTIONS_GDI_COMPATIBLE))
            return FALSE;
        return TRUE;
    }

    if (options & D2D1_BITMAP_OPTIONS_CANNOT_DRAW)
    {
        if (!(surface_options & D2D1_BITMAP_OPTIONS_CANNOT_DRAW))
            return FALSE;

        if (options & D2D1_BITMAP_OPTIONS_CPU_READ && !(surface_options & D2D1_BITMAP_OPTIONS_CPU_READ))
            return FALSE;
    }

    return TRUE;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateBitmapFromDxgiSurface(ID2D1DeviceContext6 *iface,
        IDXGISurface *surface, const D2D1_BITMAP_PROPERTIES1 *desc, ID2D1Bitmap1 **bitmap)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);
    D2D1_BITMAP_PROPERTIES1 bitmap_desc;
    unsigned int surface_options;
    struct d2d_bitmap *object;
    HRESULT hr;

    TRACE("iface %p, surface %p, desc %p, bitmap %p.\n", iface, surface, desc, bitmap);

    surface_options = d2d_get_bitmap_options_for_surface(surface);

    if (desc)
    {
        if (!d2d_bitmap_check_options_with_surface(desc->bitmapOptions, surface_options))
        {
            WARN("Incompatible bitmap options %#x, surface options %#x.\n",
                    desc->bitmapOptions, surface_options);
            return E_INVALIDARG;
        }
    }
    else
    {
        DXGI_SURFACE_DESC surface_desc;

        if (FAILED(hr = IDXGISurface_GetDesc(surface, &surface_desc)))
        {
            WARN("Failed to get surface desc, hr %#lx.\n", hr);
            return hr;
        }

        memset(&bitmap_desc, 0, sizeof(bitmap_desc));
        bitmap_desc.pixelFormat.format = surface_desc.Format;
        bitmap_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
        bitmap_desc.bitmapOptions = surface_options;
        desc = &bitmap_desc;
    }

    if (SUCCEEDED(hr = d2d_bitmap_create_shared(context, &IID_IDXGISurface, surface, desc, &object)))
        *bitmap = &object->ID2D1Bitmap1_iface;

    return hr;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateEffect(ID2D1DeviceContext6 *iface,
        REFCLSID effect_id, ID2D1Effect **effect)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p, effect_id %s, effect %p.\n", iface, debugstr_guid(effect_id), effect);

    return d2d_effect_create(context, effect_id, effect);
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_ID2D1DeviceContext_CreateGradientStopCollection(
        ID2D1DeviceContext6 *iface, const D2D1_GRADIENT_STOP *stops, UINT32 stop_count,
        D2D1_COLOR_SPACE preinterpolation_space, D2D1_COLOR_SPACE postinterpolation_space,
        D2D1_BUFFER_PRECISION buffer_precision, D2D1_EXTEND_MODE extend_mode,
        D2D1_COLOR_INTERPOLATION_MODE color_interpolation_mode, ID2D1GradientStopCollection1 **gradient)
{
    FIXME("iface %p, stops %p, stop_count %u, preinterpolation_space %#x, postinterpolation_space %#x, "
            "buffer_precision %#x, extend_mode %#x, color_interpolation_mode %#x, gradient %p stub!\n",
            iface, stops, stop_count, preinterpolation_space, postinterpolation_space,
            buffer_precision, extend_mode, color_interpolation_mode, gradient);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateImageBrush(ID2D1DeviceContext6 *iface,
        ID2D1Image *image, const D2D1_IMAGE_BRUSH_PROPERTIES *image_brush_desc,
        const D2D1_BRUSH_PROPERTIES *brush_desc, ID2D1ImageBrush **brush)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);
    struct d2d_brush *object;
    HRESULT hr;

    TRACE("iface %p, image %p, image_brush_desc %p, brush_desc %p, brush %p.\n", iface, image, image_brush_desc,
            brush_desc, brush);

    if (SUCCEEDED(hr = d2d_image_brush_create(context->factory, image, image_brush_desc,
            brush_desc, &object)))
        *brush = (ID2D1ImageBrush *)&object->ID2D1Brush_iface;

    return hr;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_ID2D1DeviceContext_CreateBitmapBrush(ID2D1DeviceContext6 *iface,
        ID2D1Bitmap *bitmap, const D2D1_BITMAP_BRUSH_PROPERTIES1 *bitmap_brush_desc,
        const D2D1_BRUSH_PROPERTIES *brush_desc, ID2D1BitmapBrush1 **brush)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);
    struct d2d_brush *object;
    HRESULT hr;

    TRACE("iface %p, bitmap %p, bitmap_brush_desc %p, brush_desc %p, brush %p.\n", iface, bitmap, bitmap_brush_desc,
            brush_desc, brush);

    if (SUCCEEDED(hr = d2d_bitmap_brush_create(context->factory, bitmap, bitmap_brush_desc, brush_desc, &object)))
        *brush = (ID2D1BitmapBrush1 *)&object->ID2D1Brush_iface;

    return hr;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateCommandList(ID2D1DeviceContext6 *iface,
        ID2D1CommandList **command_list)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);
    struct d2d_command_list *object;
    HRESULT hr;

    TRACE("iface %p, command_list %p.\n", iface, command_list);

    if (SUCCEEDED(hr = d2d_command_list_create(context->factory, &object)))
        *command_list = &object->ID2D1CommandList_iface;

    return hr;
}

static BOOL STDMETHODCALLTYPE d2d_device_context_IsDxgiFormatSupported(ID2D1DeviceContext6 *iface, DXGI_FORMAT format)
{
    FIXME("iface %p, format %#x stub!\n", iface, format);

    return FALSE;
}

static BOOL STDMETHODCALLTYPE d2d_device_context_IsBufferPrecisionSupported(ID2D1DeviceContext6 *iface,
        D2D1_BUFFER_PRECISION buffer_precision)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);
    DXGI_FORMAT format;
    UINT support = 0;
    HRESULT hr;

    TRACE("iface %p, buffer_precision %u.\n", iface, buffer_precision);

    switch (buffer_precision)
    {
        case D2D1_BUFFER_PRECISION_8BPC_UNORM: format = DXGI_FORMAT_R8G8B8A8_UNORM; break;
        case D2D1_BUFFER_PRECISION_8BPC_UNORM_SRGB: format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; break;
        case D2D1_BUFFER_PRECISION_16BPC_UNORM: format = DXGI_FORMAT_R16G16B16A16_UNORM; break;
        case D2D1_BUFFER_PRECISION_16BPC_FLOAT: format = DXGI_FORMAT_R16G16B16A16_FLOAT; break;
        case D2D1_BUFFER_PRECISION_32BPC_FLOAT: format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
        default:
            WARN("Unexpected precision %u.\n", buffer_precision);
            return FALSE;
    }

    if (FAILED(hr = ID3D11Device1_CheckFormatSupport(context->d3d_device, format, &support)))
    {
        WARN("Format support check failed, hr %#lx.\n", hr);
    }

    return !!(support & D3D11_FORMAT_SUPPORT_BUFFER);
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_GetImageLocalBounds(ID2D1DeviceContext6 *iface,
        ID2D1Image *image, D2D1_RECT_F *local_bounds)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);
    D2D_SIZE_U pixel_size;
    ID2D1Bitmap *bitmap;
    D2D_SIZE_F size;

    TRACE("iface %p, image %p, local_bounds %p.\n", iface, image, local_bounds);

    if (SUCCEEDED(ID2D1Image_QueryInterface(image, &IID_ID2D1Bitmap, (void **)&bitmap)))
    {
        local_bounds->left = 0.0f;
        local_bounds->top  = 0.0f;
        switch (context->drawing_state.unitMode)
        {
            case D2D1_UNIT_MODE_DIPS:
                size = ID2D1Bitmap_GetSize(bitmap);
                local_bounds->right  = size.width;
                local_bounds->bottom = size.height;
                break;

            case D2D1_UNIT_MODE_PIXELS:
                pixel_size = ID2D1Bitmap_GetPixelSize(bitmap);
                local_bounds->right  = pixel_size.width;
                local_bounds->bottom = pixel_size.height;
                break;

            default:
                WARN("Unknown unit mode %#x.\n", context->drawing_state.unitMode);
                break;
        }
        ID2D1Bitmap_Release(bitmap);

        return S_OK;
    }
    else
    {
        FIXME("Unable to get local bounds of image %p.\n", image);

        return E_NOTIMPL;
    }
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_GetImageWorldBounds(ID2D1DeviceContext6 *iface,
        ID2D1Image *image, D2D1_RECT_F *world_bounds)
{
    FIXME("iface %p, image %p, world_bounds %p stub!\n", iface, image, world_bounds);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_GetGlyphRunWorldBounds(ID2D1DeviceContext6 *iface,
        D2D1_POINT_2F baseline_origin, const DWRITE_GLYPH_RUN *glyph_run,
        DWRITE_MEASURING_MODE measuring_mode, D2D1_RECT_F *bounds)
{
    FIXME("iface %p, baseline_origin %s, glyph_run %p, measuring_mode %#x, bounds %p stub!\n",
            iface, debug_d2d_point_2f(&baseline_origin), glyph_run, measuring_mode, bounds);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d2d_device_context_GetDevice(ID2D1DeviceContext6 *iface, ID2D1Device **device)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (ID2D1Device *)&context->device->ID2D1Device6_iface;
    ID2D1Device_AddRef(*device);
}

static void d2d_device_context_reset_target(struct d2d_device_context *context)
{
    if (!context->target.object)
        return;

    IUnknown_Release(context->target.object);
    memset(&context->target, 0, sizeof(context->target));

    /* Note that DPI settings are kept. */
    memset(&context->desc.pixelFormat, 0, sizeof(context->desc.pixelFormat));
    memset(&context->pixel_size, 0, sizeof(context->pixel_size));

    if (context->bs)
        ID3D11BlendState_Release(context->bs);
    context->bs = NULL;
}

static void STDMETHODCALLTYPE d2d_device_context_SetTarget(ID2D1DeviceContext6 *iface, ID2D1Image *target)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);
    struct d2d_command_list *command_list_impl;
    struct d2d_bitmap *bitmap_impl;
    ID2D1CommandList *command_list;
    D3D11_BLEND_DESC blend_desc;
    ID2D1Bitmap *bitmap;
    HRESULT hr;

    TRACE("iface %p, target %p.\n", iface, target);

    if (!target)
    {
        d2d_device_context_reset_target(context);
        return;
    }

    if (SUCCEEDED(ID2D1Image_QueryInterface(target, &IID_ID2D1Bitmap, (void **)&bitmap)))
    {
        bitmap_impl = unsafe_impl_from_ID2D1Bitmap(bitmap);

        if (!(bitmap_impl->options & D2D1_BITMAP_OPTIONS_TARGET))
        {
            ID2D1Bitmap_Release(bitmap);
            d2d_device_context_set_error(context, D2DERR_INVALID_TARGET);
            return;
        }

        d2d_device_context_reset_target(context);

        /* Set sizes and pixel format. */
        context->pixel_size = bitmap_impl->pixel_size;
        context->desc.pixelFormat = bitmap_impl->format;
        context->target.bitmap = bitmap_impl;
        context->target.object = target;
        context->target.type = D2D_TARGET_BITMAP;

        memset(&blend_desc, 0, sizeof(blend_desc));
        blend_desc.IndependentBlendEnable = FALSE;
        blend_desc.RenderTarget[0].BlendEnable = TRUE;
        blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
        blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if (FAILED(hr = ID3D11Device1_CreateBlendState(context->d3d_device, &blend_desc, &context->bs)))
            WARN("Failed to create blend state, hr %#lx.\n", hr);
    }
    else if (SUCCEEDED(ID2D1Image_QueryInterface(target, &IID_ID2D1CommandList, (void **)&command_list)))
    {
        command_list_impl = unsafe_impl_from_ID2D1CommandList(command_list);

        d2d_device_context_reset_target(context);

        context->target.command_list = command_list_impl;
        context->target.object = target;
        context->target.type = D2D_TARGET_COMMAND_LIST;
    }
    else
    {
        WARN("Unsupported target type.\n");
    }
}

static void STDMETHODCALLTYPE d2d_device_context_GetTarget(ID2D1DeviceContext6 *iface, ID2D1Image **target)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p, target %p.\n", iface, target);

    *target = context->target.object ? context->target.object : NULL;
    if (*target)
        ID2D1Image_AddRef(*target);
}

static void STDMETHODCALLTYPE d2d_device_context_SetRenderingControls(ID2D1DeviceContext6 *iface,
        const D2D1_RENDERING_CONTROLS *rendering_controls)
{
    FIXME("iface %p, rendering_controls %p stub!\n", iface, rendering_controls);
}

static void STDMETHODCALLTYPE d2d_device_context_GetRenderingControls(ID2D1DeviceContext6 *iface,
        D2D1_RENDERING_CONTROLS *rendering_controls)
{
    FIXME("iface %p, rendering_controls %p stub!\n", iface, rendering_controls);
}

static void STDMETHODCALLTYPE d2d_device_context_SetPrimitiveBlend(ID2D1DeviceContext6 *iface,
        D2D1_PRIMITIVE_BLEND primitive_blend)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p, primitive_blend %u.\n", iface, primitive_blend);

    if (primitive_blend > D2D1_PRIMITIVE_BLEND_MAX)
    {
        WARN("Unknown blend mode %u.\n", primitive_blend);
        return;
    }

    if (context->target.type == D2D_TARGET_COMMAND_LIST)
        d2d_command_list_set_primitive_blend(context->target.command_list, primitive_blend);

    context->drawing_state.primitiveBlend = primitive_blend;
}

static D2D1_PRIMITIVE_BLEND STDMETHODCALLTYPE d2d_device_context_GetPrimitiveBlend(ID2D1DeviceContext6 *iface)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p.\n", iface);

    return context->drawing_state.primitiveBlend;
}

static void STDMETHODCALLTYPE d2d_device_context_SetUnitMode(ID2D1DeviceContext6 *iface, D2D1_UNIT_MODE unit_mode)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p, unit_mode %#x.\n", iface, unit_mode);

    if (unit_mode != D2D1_UNIT_MODE_DIPS && unit_mode != D2D1_UNIT_MODE_PIXELS)
    {
        WARN("Unknown unit mode %#x.\n", unit_mode);
        return;
    }

    if (context->target.type == D2D_TARGET_COMMAND_LIST)
        d2d_command_list_set_unit_mode(context->target.command_list, unit_mode);

    context->drawing_state.unitMode = unit_mode;
}

static D2D1_UNIT_MODE STDMETHODCALLTYPE d2d_device_context_GetUnitMode(ID2D1DeviceContext6 *iface)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p.\n", iface);

    return context->drawing_state.unitMode;
}

static void STDMETHODCALLTYPE d2d_device_context_ID2D1DeviceContext_DrawGlyphRun(ID2D1DeviceContext6 *iface,
        D2D1_POINT_2F baseline_origin, const DWRITE_GLYPH_RUN *glyph_run,
        const DWRITE_GLYPH_RUN_DESCRIPTION *glyph_run_desc, ID2D1Brush *brush, DWRITE_MEASURING_MODE measuring_mode)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p, baseline_origin %s, glyph_run %p, glyph_run_desc %p, brush %p, measuring_mode %#x.\n",
            iface, debug_d2d_point_2f(&baseline_origin), glyph_run, glyph_run_desc, brush, measuring_mode);

    d2d_device_context_draw_glyph_run(context, baseline_origin, glyph_run, glyph_run_desc, brush, measuring_mode);
}

static void STDMETHODCALLTYPE d2d_device_context_DrawImage(ID2D1DeviceContext6 *iface, ID2D1Image *image,
        const D2D1_POINT_2F *target_offset, const D2D1_RECT_F *image_rect, D2D1_INTERPOLATION_MODE interpolation_mode,
        D2D1_COMPOSITE_MODE composite_mode)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);
    ID2D1Bitmap *bitmap;

    TRACE("iface %p, image %p, target_offset %s, image_rect %s, interpolation_mode %#x, composite_mode %#x.\n",
            iface, image, debug_d2d_point_2f(target_offset), debug_d2d_rect_f(image_rect),
            interpolation_mode, composite_mode);

    if (context->target.type == D2D_TARGET_COMMAND_LIST)
    {
        d2d_command_list_draw_image(context->target.command_list, image, target_offset, image_rect,
                interpolation_mode, composite_mode);
        return;
    }

    if (composite_mode != D2D1_COMPOSITE_MODE_SOURCE_OVER)
        FIXME("Unhandled composite mode %#x.\n", composite_mode);

    if (SUCCEEDED(ID2D1Image_QueryInterface(image, &IID_ID2D1Bitmap, (void **)&bitmap)))
    {
        d2d_device_context_draw_bitmap(context, bitmap, NULL, 1.0f, interpolation_mode, image_rect, target_offset, NULL);

        ID2D1Bitmap_Release(bitmap);
        return;
    }

    FIXME("Unhandled image %p.\n", image);
}

static void STDMETHODCALLTYPE d2d_device_context_DrawGdiMetafile(ID2D1DeviceContext6 *iface,
        ID2D1GdiMetafile *metafile, const D2D1_POINT_2F *target_offset)
{
    FIXME("iface %p, metafile %p, target_offset %s stub!\n",
            iface, metafile, debug_d2d_point_2f(target_offset));
}

static void STDMETHODCALLTYPE d2d_device_context_ID2D1DeviceContext_DrawBitmap(ID2D1DeviceContext6 *iface,
        ID2D1Bitmap *bitmap, const D2D1_RECT_F *dst_rect, float opacity, D2D1_INTERPOLATION_MODE interpolation_mode,
        const D2D1_RECT_F *src_rect, const D2D1_MATRIX_4X4_F *perspective_transform)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);

    TRACE("iface %p, bitmap %p, dst_rect %s, opacity %.8e, interpolation_mode %#x, "
            "src_rect %s, perspective_transform %p.\n",
            iface, bitmap, debug_d2d_rect_f(dst_rect), opacity, interpolation_mode,
            debug_d2d_rect_f(src_rect), perspective_transform);

    if (context->target.type == D2D_TARGET_COMMAND_LIST)
    {
        d2d_command_list_draw_bitmap(context->target.command_list, bitmap, dst_rect, opacity, interpolation_mode,
                src_rect, perspective_transform);
    }
    else
    {
        d2d_device_context_draw_bitmap(context, bitmap, dst_rect, opacity, interpolation_mode, src_rect,
                NULL, perspective_transform);
    }
}

static void STDMETHODCALLTYPE d2d_device_context_ID2D1DeviceContext_PushLayer(ID2D1DeviceContext6 *iface,
        const D2D1_LAYER_PARAMETERS1 *layer_parameters, ID2D1Layer *layer)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);

    FIXME("iface %p, layer_parameters %p, layer %p stub!\n", iface, layer_parameters, layer);

    if (context->target.type == D2D_TARGET_COMMAND_LIST)
        d2d_command_list_push_layer(context->target.command_list, context, layer_parameters, layer);
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_InvalidateEffectInputRectangle(ID2D1DeviceContext6 *iface,
        ID2D1Effect *effect, UINT32 input, const D2D1_RECT_F *input_rect)
{
    FIXME("iface %p, effect %p, input %u, input_rect %s stub!\n",
            iface, effect, input, debug_d2d_rect_f(input_rect));

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_GetEffectInvalidRectangleCount(ID2D1DeviceContext6 *iface,
        ID2D1Effect *effect, UINT32 *rect_count)
{
    FIXME("iface %p, effect %p, rect_count %p stub!\n", iface, effect, rect_count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_GetEffectInvalidRectangles(ID2D1DeviceContext6 *iface,
        ID2D1Effect *effect, D2D1_RECT_F *rectangles, UINT32 rect_count)
{
    FIXME("iface %p, effect %p, rectangles %p, rect_count %u stub!\n", iface, effect, rectangles, rect_count);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_GetEffectRequiredInputRectangles(ID2D1DeviceContext6 *iface,
        ID2D1Effect *effect, const D2D1_RECT_F *image_rect, const D2D1_EFFECT_INPUT_DESCRIPTION *desc,
        D2D1_RECT_F *input_rect, UINT32 input_count)
{
    FIXME("iface %p, effect %p, image_rect %s, desc %p, input_rect %p, input_count %u stub!\n",
            iface, effect, debug_d2d_rect_f(image_rect), desc, input_rect, input_count);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d2d_device_context_ID2D1DeviceContext_FillOpacityMask(ID2D1DeviceContext6 *iface,
        ID2D1Bitmap *mask, ID2D1Brush *brush, const D2D1_RECT_F *dst_rect, const D2D1_RECT_F *src_rect)
{
    struct d2d_device_context *context = impl_from_ID2D1DeviceContext(iface);

    FIXME("iface %p, mask %p, brush %p, dst_rect %s, src_rect %s stub!\n",
            iface, mask, brush, debug_d2d_rect_f(dst_rect), debug_d2d_rect_f(src_rect));

    if (FAILED(context->error.code))
        return;

    if (context->drawing_state.antialiasMode != D2D1_ANTIALIAS_MODE_ALIASED)
    {
        d2d_device_context_set_error(context, D2DERR_WRONG_STATE);
        return;
    }

    if (context->target.type == D2D_TARGET_COMMAND_LIST)
        d2d_command_list_fill_opacity_mask(context->target.command_list, context, mask, brush, dst_rect, src_rect);
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateFilledGeometryRealization(ID2D1DeviceContext6 *iface,
        ID2D1Geometry *geometry, float tolerance, ID2D1GeometryRealization **realization)
{
    FIXME("iface %p, geometry %p, tolerance %.8e, realization %p stub!\n", iface, geometry, tolerance,
            realization);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateStrokedGeometryRealization(
        ID2D1DeviceContext6 *iface, ID2D1Geometry *geometry, float tolerance, float stroke_width,
        ID2D1StrokeStyle *stroke_style, ID2D1GeometryRealization **realization)
{
    FIXME("iface %p, geometry %p, tolerance %.8e, stroke_width %.8e, stroke_style %p, realization %p stub!\n",
            iface, geometry, tolerance, stroke_width, stroke_style, realization);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d2d_device_context_DrawGeometryRealization(ID2D1DeviceContext6 *iface,
        ID2D1GeometryRealization *realization, ID2D1Brush *brush)
{
    FIXME("iface %p, realization %p, brush %p stub!\n", iface, realization, brush);
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateInk(ID2D1DeviceContext6 *iface,
        const D2D1_INK_POINT *start_point, ID2D1Ink **ink)
{
    FIXME("iface %p, start_point %p, ink %p stub!\n", iface, start_point, ink);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateInkStyle(ID2D1DeviceContext6 *iface,
        const D2D1_INK_STYLE_PROPERTIES *ink_style_properties, ID2D1InkStyle **ink_style)
{
    FIXME("iface %p, ink_style_properties %p, ink_style %p stub!\n", iface, ink_style_properties, ink_style);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateGradientMesh(ID2D1DeviceContext6 *iface,
        const D2D1_GRADIENT_MESH_PATCH *patches, UINT32 patches_count,
        ID2D1GradientMesh **gradient_mesh)
{
    FIXME("iface %p, patches %p, patches_count %u, gradient_mesh %p stub!\n", iface, patches,
            patches_count, gradient_mesh);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateImageSourceFromWic(ID2D1DeviceContext6 *iface,
        IWICBitmapSource *wic_bitmap_source, D2D1_IMAGE_SOURCE_LOADING_OPTIONS loading_options,
        D2D1_ALPHA_MODE alpha_mode, ID2D1ImageSourceFromWic **image_source)
{
    FIXME("iface %p, wic_bitmap_source %p, loading_options %#x, alpha_mode %u, image_source %p stub!\n",
            iface, wic_bitmap_source, loading_options, alpha_mode, image_source);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateLookupTable3D(ID2D1DeviceContext6 *iface,
        D2D1_BUFFER_PRECISION precision, const UINT32 *extents, const BYTE *data,
        UINT32 data_count, const UINT32 *strides, ID2D1LookupTable3D **lookup_table)
{
    FIXME("iface %p, precision %u, extents %p, data %p, data_count %u, strides %p, lookup_table %p stub!\n",
            iface, precision, extents, data, data_count, strides, lookup_table);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateImageSourceFromDxgi(ID2D1DeviceContext6 *iface,
        IDXGISurface **surfaces, UINT32 surface_count, DXGI_COLOR_SPACE_TYPE color_space,
        D2D1_IMAGE_SOURCE_FROM_DXGI_OPTIONS options, ID2D1ImageSource **image_source)
{
    FIXME("iface %p, surfaces %p, surface_count %u, color_space %u, options %#x, image_source %p stub!\n",
            iface, surfaces, surface_count, color_space, options, image_source);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_GetGradientMeshWorldBounds(ID2D1DeviceContext6 *iface,
        ID2D1GradientMesh *gradient_mesh, D2D1_RECT_F *bounds)
{
    FIXME("iface %p, gradient_mesh %p, bounds %p stub!\n", iface, gradient_mesh, bounds);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d2d_device_context_DrawInk(ID2D1DeviceContext6 *iface, ID2D1Ink *ink,
        ID2D1Brush *brush, ID2D1InkStyle *ink_style)
{
    FIXME("iface %p, ink %p, brush %p, ink_style %p stub!\n", iface, ink, brush, ink_style);
}

static void STDMETHODCALLTYPE d2d_device_context_DrawGradientMesh(ID2D1DeviceContext6 *iface,
        ID2D1GradientMesh *gradient_mesh)
{
    FIXME("iface %p, gradient_mesh %p stub!\n", iface, gradient_mesh);
}

static void STDMETHODCALLTYPE d2d_device_context_ID2D1DeviceContext2_DrawGdiMetafile(
        ID2D1DeviceContext6 *iface, ID2D1GdiMetafile *gdi_metafile, const D2D1_RECT_F *dst_rect,
        const D2D1_RECT_F *src_rect)
{
    FIXME("iface %p, gdi_metafile %p, dst_rect %s, src_rect %s stub!\n", iface, gdi_metafile,
            debug_d2d_rect_f(dst_rect), debug_d2d_rect_f(src_rect));
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateTransformedImageSource(ID2D1DeviceContext6 *iface,
        ID2D1ImageSource *source, const D2D1_TRANSFORMED_IMAGE_SOURCE_PROPERTIES *props,
        ID2D1TransformedImageSource **transformed)
{
    FIXME("iface %p, source %p, props %p, transformed %p stub!\n", iface, source, props, transformed);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateSpriteBatch(ID2D1DeviceContext6 *iface,
        ID2D1SpriteBatch **sprite_batch)
{
    FIXME("iface %p, sprite_batch %p stub!\n", iface, sprite_batch);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d2d_device_context_DrawSpriteBatch(ID2D1DeviceContext6 *iface,
        ID2D1SpriteBatch *sprite_batch, UINT32 start_index, UINT32 sprite_count, ID2D1Bitmap *bitmap,
        D2D1_BITMAP_INTERPOLATION_MODE interpolation_mode, D2D1_SPRITE_OPTIONS sprite_options)
{
    FIXME("iface %p, sprite_batch %p, start_index %u, sprite_count %u, bitmap %p, interpolation_mode %u,"
            "sprite_options %u stub!\n", iface, sprite_batch, start_index, sprite_count, bitmap,
            interpolation_mode, sprite_options);
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateSvgGlyphStyle(ID2D1DeviceContext6 *iface,
        ID2D1SvgGlyphStyle **svg_glyph_style)
{
    FIXME("iface %p, svg_glyph_style %p stub!\n", iface, svg_glyph_style);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d2d_device_context_ID2D1DeviceContext4_DrawText(ID2D1DeviceContext6 *iface,
        const WCHAR *string, UINT32 string_length, IDWriteTextFormat *text_format, const D2D1_RECT_F *layout_rect,
        ID2D1Brush *default_fill_brush, ID2D1SvgGlyphStyle *svg_glyph_style, UINT32 color_palette_index,
        D2D1_DRAW_TEXT_OPTIONS options, DWRITE_MEASURING_MODE measuring_mode)
{
    FIXME("iface %p, string %s, string_length %u, text_format %p, layout_rect %s, default_fill_brush %p,"
            "svg_glyph_style %p, color_palette_index %u, options %#x, measuring_mode %u stub!\n",
            iface, debugstr_wn(string, string_length), string_length, text_format, debug_d2d_rect_f(layout_rect),
            default_fill_brush, svg_glyph_style, color_palette_index, options, measuring_mode);
}

static void STDMETHODCALLTYPE d2d_device_context_ID2D1DeviceContext4_DrawTextLayout(ID2D1DeviceContext6 *iface,
        D2D1_POINT_2F origin, IDWriteTextLayout *text_layout, ID2D1Brush *default_fill_brush,
        ID2D1SvgGlyphStyle *svg_glyph_style, UINT32 color_palette_index, D2D1_DRAW_TEXT_OPTIONS options)
{
    FIXME("iface %p, origin %s, text_layout %p, default_fill_brush %p, svg_glyph_style %p, color_palette_index %u,"
            "options %#x stub!\n", iface, debug_d2d_point_2f(&origin), text_layout, default_fill_brush,
            svg_glyph_style, color_palette_index, options);
}

static void STDMETHODCALLTYPE d2d_device_context_DrawColorBitmapGlyphRun(ID2D1DeviceContext6 *iface,
        DWRITE_GLYPH_IMAGE_FORMATS glyph_image_format, D2D1_POINT_2F baseline_origin, const DWRITE_GLYPH_RUN *glyph_run,
        DWRITE_MEASURING_MODE measuring_mode, D2D1_COLOR_BITMAP_GLYPH_SNAP_OPTION bitmap_snap_option)
{
    FIXME("iface %p, glyph_image_format %#x, baseline_origin %s, glyph_run %p, measuring_mode %u, bitmap_snap_option %#x stub!\n",
            iface, glyph_image_format, debug_d2d_point_2f(&baseline_origin), glyph_run, measuring_mode, bitmap_snap_option);
}

static void STDMETHODCALLTYPE d2d_device_context_DrawSvgGlyphRun(ID2D1DeviceContext6 *iface,
        D2D1_POINT_2F baseline_origin, const DWRITE_GLYPH_RUN *glyph_run, ID2D1Brush *default_fill_brush,
        ID2D1SvgGlyphStyle *svg_glyph_style, UINT32 color_palette_index, DWRITE_MEASURING_MODE measuring_mode)
{
    FIXME("iface %p, baseline_origin %s, glyph_run %p, default_fill_brush %p, svg_glyph_style %p,"
            "color_palette_index %u, measuring_mode %u stub!\n", iface, debug_d2d_point_2f(&baseline_origin),
            glyph_run, default_fill_brush, svg_glyph_style, color_palette_index, measuring_mode);
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_GetColorBitmapGlyphImage(ID2D1DeviceContext6 *iface,
        DWRITE_GLYPH_IMAGE_FORMATS glyph_image_format, D2D1_POINT_2F glyph_origin, IDWriteFontFace *font_face,
        FLOAT font_em_size, UINT16 glyph_index, BOOL is_sideways, const D2D1_MATRIX_3X2_F *world_transform,
        FLOAT dpi_x, FLOAT dpi_y, D2D1_MATRIX_3X2_F *glyph_transform, ID2D1Image **glyph_image)
{
    FIXME("iface %p, glyph_image_format %u, glyph_origin %s, font_face %p, font_em_size %f, glyph_index %u,"
            "is_sideways %d, world_transform %p, dpi_x %f, dpi_y %f, glyph_transform %p, glyph_image %p stub!\n",
            iface, glyph_image_format, debug_d2d_point_2f(&glyph_origin), font_face, font_em_size, glyph_index,
            is_sideways, world_transform, dpi_x, dpi_y, glyph_transform, glyph_image);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_GetSvgGlyphImage(ID2D1DeviceContext6 *iface,
        D2D1_POINT_2F glyph_origin, IDWriteFontFace *font_face, FLOAT font_em_size, UINT16 glyph_index,
        BOOL is_sideways, const D2D1_MATRIX_3X2_F *world_transform, ID2D1Brush *default_fill_brush,
        ID2D1SvgGlyphStyle *svg_glyph_style, UINT32 color_palette_index, D2D1_MATRIX_3X2_F *glyph_transform,
        ID2D1CommandList **glyph_image)
{
    FIXME("iface %p, glyph_origin %s, font_face %p, font_em_size %f, glyph_index %u, is_sideways %d,"
            "world_transform %p, default_fill_brush %p, svg_glyph_style %p, color_palette_index %u,"
            "glyph_transform %p, glyph_image %p stub!\n", iface, debug_d2d_point_2f(&glyph_origin),
            font_face, font_em_size, glyph_index, is_sideways, world_transform, default_fill_brush,
            svg_glyph_style, color_palette_index, glyph_transform, glyph_image);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateSvgDocument(ID2D1DeviceContext6 *iface,
        IStream *input_xml_stream, D2D1_SIZE_F viewport_size, ID2D1SvgDocument **svg_document)
{
    FIXME("iface %p, input_xml_stream %p, svg_document %p stub!\n", iface, input_xml_stream,
            svg_document);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d2d_device_context_DrawSvgDocument(ID2D1DeviceContext6 *iface,
        ID2D1SvgDocument *svg_document)
{
    FIXME("iface %p, svg_document %p stub!\n", iface, svg_document);
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateColorContextFromDxgiColorSpace(
        ID2D1DeviceContext6 *iface, DXGI_COLOR_SPACE_TYPE color_space, ID2D1ColorContext1 **color_context)
{
    FIXME("iface %p, color_space %u, color_context %p stub!\n", iface, color_space, color_context);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE d2d_device_context_CreateColorContextFromSimpleColorProfile(
        ID2D1DeviceContext6 *iface, const D2D1_SIMPLE_COLOR_PROFILE *simple_profile, ID2D1ColorContext1 **color_context)
{
    FIXME("iface %p, simple_profile %p, color_context %p stub!\n", iface, simple_profile, color_context);

    return E_NOTIMPL;
}

static void STDMETHODCALLTYPE d2d_device_context_BlendImage(ID2D1DeviceContext6 *iface, ID2D1Image *image,
        D2D1_BLEND_MODE blend_mode, const D2D1_POINT_2F *target_offset, const D2D1_RECT_F *image_rect,
        D2D1_INTERPOLATION_MODE interpolation_mode)
{
    FIXME("iface %p, image %p, blend_mode %u, target_offset %s, image_rect %s, interpolation_mode %u stub!\n",
            iface, image, blend_mode, debug_d2d_point_2f(target_offset), debug_d2d_rect_f(image_rect),
            interpolation_mode);
}

static const struct ID2D1DeviceContext6Vtbl d2d_device_context_vtbl =
{
    d2d_device_context_QueryInterface,
    d2d_device_context_AddRef,
    d2d_device_context_Release,
    d2d_device_context_GetFactory,
    d2d_device_context_CreateBitmap,
    d2d_device_context_CreateBitmapFromWicBitmap,
    d2d_device_context_CreateSharedBitmap,
    d2d_device_context_CreateBitmapBrush,
    d2d_device_context_CreateSolidColorBrush,
    d2d_device_context_CreateGradientStopCollection,
    d2d_device_context_CreateLinearGradientBrush,
    d2d_device_context_CreateRadialGradientBrush,
    d2d_device_context_CreateCompatibleRenderTarget,
    d2d_device_context_CreateLayer,
    d2d_device_context_CreateMesh,
    d2d_device_context_DrawLine,
    d2d_device_context_DrawRectangle,
    d2d_device_context_FillRectangle,
    d2d_device_context_DrawRoundedRectangle,
    d2d_device_context_FillRoundedRectangle,
    d2d_device_context_DrawEllipse,
    d2d_device_context_FillEllipse,
    d2d_device_context_DrawGeometry,
    d2d_device_context_FillGeometry,
    d2d_device_context_FillMesh,
    d2d_device_context_FillOpacityMask,
    d2d_device_context_DrawBitmap,
    d2d_device_context_DrawText,
    d2d_device_context_DrawTextLayout,
    d2d_device_context_DrawGlyphRun,
    d2d_device_context_SetTransform,
    d2d_device_context_GetTransform,
    d2d_device_context_SetAntialiasMode,
    d2d_device_context_GetAntialiasMode,
    d2d_device_context_SetTextAntialiasMode,
    d2d_device_context_GetTextAntialiasMode,
    d2d_device_context_SetTextRenderingParams,
    d2d_device_context_GetTextRenderingParams,
    d2d_device_context_SetTags,
    d2d_device_context_GetTags,
    d2d_device_context_PushLayer,
    d2d_device_context_PopLayer,
    d2d_device_context_Flush,
    d2d_device_context_SaveDrawingState,
    d2d_device_context_RestoreDrawingState,
    d2d_device_context_PushAxisAlignedClip,
    d2d_device_context_PopAxisAlignedClip,
    d2d_device_context_Clear,
    d2d_device_context_BeginDraw,
    d2d_device_context_EndDraw,
    d2d_device_context_GetPixelFormat,
    d2d_device_context_SetDpi,
    d2d_device_context_GetDpi,
    d2d_device_context_GetSize,
    d2d_device_context_GetPixelSize,
    d2d_device_context_GetMaximumBitmapSize,
    d2d_device_context_IsSupported,
    d2d_device_context_ID2D1DeviceContext_CreateBitmap,
    d2d_device_context_ID2D1DeviceContext_CreateBitmapFromWicBitmap,
    d2d_device_context_CreateColorContext,
    d2d_device_context_CreateColorContextFromFilename,
    d2d_device_context_CreateColorContextFromWicColorContext,
    d2d_device_context_CreateBitmapFromDxgiSurface,
    d2d_device_context_CreateEffect,
    d2d_device_context_ID2D1DeviceContext_CreateGradientStopCollection,
    d2d_device_context_CreateImageBrush,
    d2d_device_context_ID2D1DeviceContext_CreateBitmapBrush,
    d2d_device_context_CreateCommandList,
    d2d_device_context_IsDxgiFormatSupported,
    d2d_device_context_IsBufferPrecisionSupported,
    d2d_device_context_GetImageLocalBounds,
    d2d_device_context_GetImageWorldBounds,
    d2d_device_context_GetGlyphRunWorldBounds,
    d2d_device_context_GetDevice,
    d2d_device_context_SetTarget,
    d2d_device_context_GetTarget,
    d2d_device_context_SetRenderingControls,
    d2d_device_context_GetRenderingControls,
    d2d_device_context_SetPrimitiveBlend,
    d2d_device_context_GetPrimitiveBlend,
    d2d_device_context_SetUnitMode,
    d2d_device_context_GetUnitMode,
    d2d_device_context_ID2D1DeviceContext_DrawGlyphRun,
    d2d_device_context_DrawImage,
    d2d_device_context_DrawGdiMetafile,
    d2d_device_context_ID2D1DeviceContext_DrawBitmap,
    d2d_device_context_ID2D1DeviceContext_PushLayer,
    d2d_device_context_InvalidateEffectInputRectangle,
    d2d_device_context_GetEffectInvalidRectangleCount,
    d2d_device_context_GetEffectInvalidRectangles,
    d2d_device_context_GetEffectRequiredInputRectangles,
    d2d_device_context_ID2D1DeviceContext_FillOpacityMask,
    d2d_device_context_CreateFilledGeometryRealization,
    d2d_device_context_CreateStrokedGeometryRealization,
    d2d_device_context_DrawGeometryRealization,
    d2d_device_context_CreateInk,
    d2d_device_context_CreateInkStyle,
    d2d_device_context_CreateGradientMesh,
    d2d_device_context_CreateImageSourceFromWic,
    d2d_device_context_CreateLookupTable3D,
    d2d_device_context_CreateImageSourceFromDxgi,
    d2d_device_context_GetGradientMeshWorldBounds,
    d2d_device_context_DrawInk,
    d2d_device_context_DrawGradientMesh,
    d2d_device_context_ID2D1DeviceContext2_DrawGdiMetafile,
    d2d_device_context_CreateTransformedImageSource,
    d2d_device_context_CreateSpriteBatch,
    d2d_device_context_DrawSpriteBatch,
    d2d_device_context_CreateSvgGlyphStyle,
    d2d_device_context_ID2D1DeviceContext4_DrawText,
    d2d_device_context_ID2D1DeviceContext4_DrawTextLayout,
    d2d_device_context_DrawColorBitmapGlyphRun,
    d2d_device_context_DrawSvgGlyphRun,
    d2d_device_context_GetColorBitmapGlyphImage,
    d2d_device_context_GetSvgGlyphImage,
    d2d_device_context_CreateSvgDocument,
    d2d_device_context_DrawSvgDocument,
    d2d_device_context_CreateColorContextFromDxgiColorSpace,
    d2d_device_context_CreateColorContextFromSimpleColorProfile,
    d2d_device_context_BlendImage,
};

static inline struct d2d_device_context *impl_from_IDWriteTextRenderer(IDWriteTextRenderer *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_device_context, IDWriteTextRenderer_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_text_renderer_QueryInterface(IDWriteTextRenderer *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IDWriteTextRenderer)
            || IsEqualGUID(iid, &IID_IDWritePixelSnapping)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        IDWriteTextRenderer_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d2d_text_renderer_AddRef(IDWriteTextRenderer *iface)
{
    struct d2d_device_context *context = impl_from_IDWriteTextRenderer(iface);

    TRACE("iface %p.\n", iface);

    return d2d_device_context_AddRef(&context->ID2D1DeviceContext6_iface);
}

static ULONG STDMETHODCALLTYPE d2d_text_renderer_Release(IDWriteTextRenderer *iface)
{
    struct d2d_device_context *context = impl_from_IDWriteTextRenderer(iface);

    TRACE("iface %p.\n", iface);

    return d2d_device_context_Release(&context->ID2D1DeviceContext6_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_text_renderer_IsPixelSnappingDisabled(IDWriteTextRenderer *iface,
        void *ctx, BOOL *disabled)
{
    struct d2d_draw_text_layout_ctx *context = ctx;

    TRACE("iface %p, ctx %p, disabled %p.\n", iface, ctx, disabled);

    *disabled = context->options & D2D1_DRAW_TEXT_OPTIONS_NO_SNAP;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_text_renderer_GetCurrentTransform(IDWriteTextRenderer *iface,
        void *ctx, DWRITE_MATRIX *transform)
{
    struct d2d_device_context *context = impl_from_IDWriteTextRenderer(iface);

    TRACE("iface %p, ctx %p, transform %p.\n", iface, ctx, transform);

    d2d_device_context_GetTransform(&context->ID2D1DeviceContext6_iface, (D2D1_MATRIX_3X2_F *)transform);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_text_renderer_GetPixelsPerDip(IDWriteTextRenderer *iface, void *ctx, float *ppd)
{
    struct d2d_device_context *render_target = impl_from_IDWriteTextRenderer(iface);

    TRACE("iface %p, ctx %p, ppd %p.\n", iface, ctx, ppd);

    *ppd = render_target->desc.dpiY / 96.0f;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_text_renderer_DrawGlyphRun(IDWriteTextRenderer *iface, void *ctx,
        float baseline_origin_x, float baseline_origin_y, DWRITE_MEASURING_MODE measuring_mode,
        const DWRITE_GLYPH_RUN *glyph_run, const DWRITE_GLYPH_RUN_DESCRIPTION *glyph_run_desc, IUnknown *effect)
{
    struct d2d_device_context *render_target = impl_from_IDWriteTextRenderer(iface);
    D2D1_POINT_2F baseline_origin = {baseline_origin_x, baseline_origin_y};
    struct d2d_draw_text_layout_ctx *context = ctx;
    BOOL color_font = FALSE;
    ID2D1Brush *brush;

    TRACE("iface %p, ctx %p, baseline_origin_x %.8e, baseline_origin_y %.8e, "
            "measuring_mode %#x, glyph_run %p, glyph_run_desc %p, effect %p.\n",
            iface, ctx, baseline_origin_x, baseline_origin_y,
            measuring_mode, glyph_run, glyph_run_desc, effect);

    if (context->options & ~(D2D1_DRAW_TEXT_OPTIONS_NO_SNAP | D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT))
        FIXME("Ignoring options %#x.\n", context->options);

    brush = d2d_draw_get_text_brush(context, effect);

    TRACE("%s\n", debugstr_wn(glyph_run_desc->string, glyph_run_desc->stringLength));

    if (context->options & D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT)
    {
        IDWriteFontFace2 *fontface;

        if (SUCCEEDED(IDWriteFontFace_QueryInterface(glyph_run->fontFace,
                &IID_IDWriteFontFace2, (void **)&fontface)))
        {
            color_font = IDWriteFontFace2_IsColorFont(fontface);
            IDWriteFontFace2_Release(fontface);
        }
    }

    if (color_font)
    {
        IDWriteColorGlyphRunEnumerator *layers;
        IDWriteFactory2 *dwrite_factory;
        HRESULT hr;

        if (FAILED(hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, &IID_IDWriteFactory2,
                (IUnknown **)&dwrite_factory)))
        {
            ERR("Failed to create dwrite factory, hr %#lx.\n", hr);
            ID2D1Brush_Release(brush);
            return hr;
        }

        hr = IDWriteFactory2_TranslateColorGlyphRun(dwrite_factory, baseline_origin_x, baseline_origin_y,
                glyph_run, glyph_run_desc, measuring_mode, (DWRITE_MATRIX *)&render_target->drawing_state.transform, 0, &layers);
        IDWriteFactory2_Release(dwrite_factory);
        if (FAILED(hr))
        {
            ERR("Failed to create colour glyph run enumerator, hr %#lx.\n", hr);
            ID2D1Brush_Release(brush);
            return hr;
        }

        for (;;)
        {
            const DWRITE_COLOR_GLYPH_RUN *color_run;
            ID2D1Brush *color_brush;
            D2D1_POINT_2F origin;
            BOOL has_run = FALSE;

            if (FAILED(hr = IDWriteColorGlyphRunEnumerator_MoveNext(layers, &has_run)))
            {
                ERR("Failed to switch colour glyph layer, hr %#lx.\n", hr);
                break;
            }

            if (!has_run)
                break;

            if (FAILED(hr = IDWriteColorGlyphRunEnumerator_GetCurrentRun(layers, &color_run)))
            {
                ERR("Failed to get current colour run, hr %#lx.\n", hr);
                break;
            }

            if (color_run->paletteIndex == 0xffff)
                color_brush = brush;
            else
            {
                if (FAILED(hr = d2d_device_context_CreateSolidColorBrush(&render_target->ID2D1DeviceContext6_iface,
                        &color_run->runColor, NULL, (ID2D1SolidColorBrush **)&color_brush)))
                {
                    ERR("Failed to create solid colour brush, hr %#lx.\n", hr);
                    break;
                }
            }

            origin.x = color_run->baselineOriginX;
            origin.y = color_run->baselineOriginY;
            d2d_device_context_draw_glyph_run(render_target, origin, &color_run->glyphRun,
                    color_run->glyphRunDescription, color_brush, measuring_mode);

            if (color_brush != brush)
                ID2D1Brush_Release(color_brush);
        }

        IDWriteColorGlyphRunEnumerator_Release(layers);
    }
    else
        d2d_device_context_draw_glyph_run(render_target, baseline_origin, glyph_run, glyph_run_desc,
                brush, measuring_mode);

    ID2D1Brush_Release(brush);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_text_renderer_DrawUnderline(IDWriteTextRenderer *iface, void *ctx,
        float baseline_origin_x, float baseline_origin_y, const DWRITE_UNDERLINE *underline, IUnknown *effect)
{
    struct d2d_device_context *render_target = impl_from_IDWriteTextRenderer(iface);
    const D2D1_MATRIX_3X2_F *m = &render_target->drawing_state.transform;
    struct d2d_draw_text_layout_ctx *context = ctx;
    D2D1_ANTIALIAS_MODE prev_antialias_mode;
    D2D1_POINT_2F start, end;
    ID2D1Brush *brush;
    float thickness;

    TRACE("iface %p, ctx %p, baseline_origin_x %.8e, baseline_origin_y %.8e, underline %p, effect %p\n",
            iface, ctx, baseline_origin_x, baseline_origin_y, underline, effect);

    /* minimal thickness in DIPs that will result in at least 1 pixel thick line */
    thickness = max(96.0f / (render_target->desc.dpiY * sqrtf(m->_21 * m->_21 + m->_22 * m->_22)),
            underline->thickness);

    brush = d2d_draw_get_text_brush(context, effect);

    start.x = baseline_origin_x;
    start.y = baseline_origin_y + underline->offset + thickness / 2.0f;
    end.x = start.x + underline->width;
    end.y = start.y;
    prev_antialias_mode = d2d_device_context_set_aa_mode_from_text_aa_mode(render_target);
    d2d_device_context_DrawLine(&render_target->ID2D1DeviceContext6_iface, start, end, brush, thickness, NULL);
    render_target->drawing_state.antialiasMode = prev_antialias_mode;

    ID2D1Brush_Release(brush);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_text_renderer_DrawStrikethrough(IDWriteTextRenderer *iface, void *ctx,
        float baseline_origin_x, float baseline_origin_y, const DWRITE_STRIKETHROUGH *strikethrough, IUnknown *effect)
{
    struct d2d_device_context *render_target = impl_from_IDWriteTextRenderer(iface);
    const D2D1_MATRIX_3X2_F *m = &render_target->drawing_state.transform;
    struct d2d_draw_text_layout_ctx *context = ctx;
    D2D1_ANTIALIAS_MODE prev_antialias_mode;
    D2D1_POINT_2F start, end;
    ID2D1Brush *brush;
    float thickness;

    TRACE("iface %p, ctx %p, baseline_origin_x %.8e, baseline_origin_y %.8e, strikethrough %p, effect %p.\n",
            iface, ctx, baseline_origin_x, baseline_origin_y, strikethrough, effect);

    /* minimal thickness in DIPs that will result in at least 1 pixel thick line */
    thickness = max(96.0f / (render_target->desc.dpiY * sqrtf(m->_21 * m->_21 + m->_22 * m->_22)),
            strikethrough->thickness);

    brush = d2d_draw_get_text_brush(context, effect);

    start.x = baseline_origin_x;
    start.y = baseline_origin_y + strikethrough->offset + thickness / 2.0f;
    end.x = start.x + strikethrough->width;
    end.y = start.y;
    prev_antialias_mode = d2d_device_context_set_aa_mode_from_text_aa_mode(render_target);
    d2d_device_context_DrawLine(&render_target->ID2D1DeviceContext6_iface, start, end, brush, thickness, NULL);
    render_target->drawing_state.antialiasMode = prev_antialias_mode;

    ID2D1Brush_Release(brush);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_text_renderer_DrawInlineObject(IDWriteTextRenderer *iface, void *ctx,
        float origin_x, float origin_y, IDWriteInlineObject *object, BOOL is_sideways, BOOL is_rtl, IUnknown *effect)
{
    struct d2d_draw_text_layout_ctx *context = ctx;
    ID2D1Brush *brush;
    HRESULT hr;

    TRACE("iface %p, ctx %p, origin_x %.8e, origin_y %.8e, object %p, is_sideways %#x, is_rtl %#x, effect %p.\n",
            iface, ctx, origin_x, origin_y, object, is_sideways, is_rtl, effect);

    /* Inline objects may not pass effects all the way down, when using layout object internally for example.
       This is how default trimming sign object in DirectWrite works - it does not use effect passed to Draw(),
       and resulting DrawGlyphRun() is always called with NULL effect, however original effect is used and correct
       brush is selected at Direct2D level. */
    brush = context->brush;
    context->brush = d2d_draw_get_text_brush(context, effect);

    hr = IDWriteInlineObject_Draw(object, ctx, iface, origin_x, origin_y, is_sideways, is_rtl, effect);

    ID2D1Brush_Release(context->brush);
    context->brush = brush;

    return hr;
}

static const struct IDWriteTextRendererVtbl d2d_text_renderer_vtbl =
{
    d2d_text_renderer_QueryInterface,
    d2d_text_renderer_AddRef,
    d2d_text_renderer_Release,
    d2d_text_renderer_IsPixelSnappingDisabled,
    d2d_text_renderer_GetCurrentTransform,
    d2d_text_renderer_GetPixelsPerDip,
    d2d_text_renderer_DrawGlyphRun,
    d2d_text_renderer_DrawUnderline,
    d2d_text_renderer_DrawStrikethrough,
    d2d_text_renderer_DrawInlineObject,
};

static inline struct d2d_device_context *impl_from_ID2D1GdiInteropRenderTarget(ID2D1GdiInteropRenderTarget *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_device_context, ID2D1GdiInteropRenderTarget_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_gdi_interop_render_target_QueryInterface(ID2D1GdiInteropRenderTarget *iface,
        REFIID iid, void **out)
{
    struct d2d_device_context *render_target = impl_from_ID2D1GdiInteropRenderTarget(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    return IUnknown_QueryInterface(render_target->outer_unknown, iid, out);
}

static ULONG STDMETHODCALLTYPE d2d_gdi_interop_render_target_AddRef(ID2D1GdiInteropRenderTarget *iface)
{
    struct d2d_device_context *render_target = impl_from_ID2D1GdiInteropRenderTarget(iface);

    TRACE("iface %p.\n", iface);

    return IUnknown_AddRef(render_target->outer_unknown);
}

static ULONG STDMETHODCALLTYPE d2d_gdi_interop_render_target_Release(ID2D1GdiInteropRenderTarget *iface)
{
    struct d2d_device_context *render_target = impl_from_ID2D1GdiInteropRenderTarget(iface);

    TRACE("iface %p.\n", iface);

    return IUnknown_Release(render_target->outer_unknown);
}

static HRESULT d2d_gdi_interop_get_surface(struct d2d_device_context *context, IDXGISurface1 **surface)
{
    ID3D11Resource *resource;
    HRESULT hr;

    if (context->target.type != D2D_TARGET_BITMAP)
    {
        FIXME("Unimplemented for target type %u.\n", context->target.type);
        return E_NOTIMPL;
    }

    if (!(context->target.bitmap->options & D2D1_BITMAP_OPTIONS_GDI_COMPATIBLE))
        return D2DERR_TARGET_NOT_GDI_COMPATIBLE;

    ID3D11RenderTargetView_GetResource(context->target.bitmap->rtv, &resource);
    hr = ID3D11Resource_QueryInterface(resource, &IID_IDXGISurface1, (void **)surface);
    ID3D11Resource_Release(resource);
    if (FAILED(hr))
    {
        *surface = NULL;
        WARN("Failed to get DXGI surface, %#lx.\n", hr);
        return hr;
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE d2d_gdi_interop_render_target_GetDC(ID2D1GdiInteropRenderTarget *iface,
        D2D1_DC_INITIALIZE_MODE mode, HDC *dc)
{
    struct d2d_device_context *render_target = impl_from_ID2D1GdiInteropRenderTarget(iface);
    IDXGISurface1 *surface;
    HRESULT hr;

    TRACE("iface %p, mode %d, dc %p.\n", iface, mode, dc);

    *dc = NULL;

    if (render_target->target.hdc)
        return D2DERR_WRONG_STATE;

    if (FAILED(hr = d2d_gdi_interop_get_surface(render_target, &surface)))
        return hr;

    hr = IDXGISurface1_GetDC(surface, mode != D2D1_DC_INITIALIZE_MODE_COPY, &render_target->target.hdc);
    IDXGISurface1_Release(surface);

    if (SUCCEEDED(hr))
        *dc = render_target->target.hdc;

    return hr;
}

static HRESULT STDMETHODCALLTYPE d2d_gdi_interop_render_target_ReleaseDC(ID2D1GdiInteropRenderTarget *iface,
        const RECT *update)
{
    struct d2d_device_context *render_target = impl_from_ID2D1GdiInteropRenderTarget(iface);
    IDXGISurface1 *surface;
    RECT update_rect;
    HRESULT hr;

    TRACE("iface %p, update rect %s.\n", iface, wine_dbgstr_rect(update));

    if (!render_target->target.hdc)
        return D2DERR_WRONG_STATE;

    if (FAILED(hr = d2d_gdi_interop_get_surface(render_target, &surface)))
        return hr;

    render_target->target.hdc = NULL;
    if (update)
        update_rect = *update;
    hr = IDXGISurface1_ReleaseDC(surface, update ? &update_rect : NULL);
    IDXGISurface1_Release(surface);

    return hr;
}

static const struct ID2D1GdiInteropRenderTargetVtbl d2d_gdi_interop_render_target_vtbl =
{
    d2d_gdi_interop_render_target_QueryInterface,
    d2d_gdi_interop_render_target_AddRef,
    d2d_gdi_interop_render_target_Release,
    d2d_gdi_interop_render_target_GetDC,
    d2d_gdi_interop_render_target_ReleaseDC,
};

static HRESULT d2d_device_context_init(struct d2d_device_context *render_target,
        struct d2d_device *device, IUnknown *outer_unknown, const struct d2d_device_context_ops *ops)
{
    D3D11_SUBRESOURCE_DATA buffer_data;
    IDWriteFactory *dwrite_factory;
    D3D11_RASTERIZER_DESC rs_desc;
    D3D11_BUFFER_DESC buffer_desc;
    struct d2d_factory *factory;
    ID3D10Blob *compiled;
    unsigned int i;
    HRESULT hr;

    static const D3D11_INPUT_ELEMENT_DESC il_desc_outline[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"PREV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NEXT", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    static const D3D11_INPUT_ELEMENT_DESC il_desc_curve_outline[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"P", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"P", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"P", 2, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"PREV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NEXT", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    static const D3D11_INPUT_ELEMENT_DESC il_desc_triangle[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    static const D3D11_INPUT_ELEMENT_DESC il_desc_curve[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    static const char vs_code_outline[] =
        "float3x2 transform_geometry;\n"
        "float stroke_width;\n"
        "float4 transform_rtx;\n"
        "float4 transform_rty;\n"
        "\n"
        "struct output\n"
        "{\n"
        "    float2 p : WORLD_POSITION;\n"
        "    float4 b : BEZIER;\n"
        "    nointerpolation float2x2 stroke_transform : STROKE_TRANSFORM;\n"
        "    float4 position : SV_POSITION;\n"
        "};\n"
        "\n"
        "/* The lines PₚᵣₑᵥP₀ and P₀Pₙₑₓₜ, both offset by ±½w, intersect each other at:\n"
        " *\n"
        " *   Pᵢ = P₀ ± w · ½q⃑ᵢ.\n"
        " *\n"
        " * Where:\n"
        " *\n"
        " *   q⃑ᵢ = q̂ₚᵣₑᵥ⊥ + tan(½θ) · -q̂ₚᵣₑᵥ\n"
        " *   θ  = ∠PₚᵣₑᵥP₀Pₙₑₓₜ\n"
        " *   q⃑ₚᵣₑᵥ = P₀ - Pₚᵣₑᵥ */\n"
        "void main(float2 position : POSITION, float2 prev : PREV, float2 next : NEXT, out struct output o)\n"
        "{\n"
        "    float2 q_prev, q_next, v_p, q_i;\n"
        "    float2x2 geom;\n"
        "    float l;\n"
        "\n"
        "    o.stroke_transform = float2x2(transform_rtx.xy, transform_rty.xy) * stroke_width * 0.5f;\n"
        "\n"
        "    geom = float2x2(transform_geometry._11_21, transform_geometry._12_22);\n"
        "    q_prev = normalize(mul(geom, prev));\n"
        "    q_next = normalize(mul(geom, next));\n"
        "\n"
        "    /* tan(½θ) = sin(θ) / (1 + cos(θ))\n"
        "     *         = (q̂ₚᵣₑᵥ⊥ · q̂ₙₑₓₜ) / (1 + (q̂ₚᵣₑᵥ · q̂ₙₑₓₜ)) */\n"
        "    v_p = float2(-q_prev.y, q_prev.x);\n"
        "    l = -dot(v_p, q_next) / (1.0f + dot(q_prev, q_next));\n"
        "    q_i = l * q_prev + v_p;\n"
        "\n"
        "    o.b = float4(0.0, 0.0, 0.0, 0.0);\n"
        "\n"
        "    o.p = mul(float3(position, 1.0f), transform_geometry) + stroke_width * 0.5f * q_i;\n"
        "    position = mul(float2x3(transform_rtx.xyz, transform_rty.xyz), float3(o.p, 1.0f))\n"
        "            * float2(transform_rtx.w, transform_rty.w);\n"
        "    o.position = float4(position + float2(-1.0f, 1.0f), 0.0f, 1.0f);\n"
        "}\n";
    /*     ⎡p0.x p0.y 1⎤
     * A = ⎢p1.x p1.y 1⎥
     *     ⎣p2.x p2.y 1⎦
     *
     *     ⎡0 0⎤
     * B = ⎢½ 0⎥
     *     ⎣1 1⎦
     *
     * A' = ⎡p1.x-p0.x p1.y-p0.y⎤
     *      ⎣p2.x-p0.x p2.y-p0.y⎦
     *
     * B' = ⎡½ 0⎤
     *      ⎣1 1⎦
     *
     * A'T = B'
     * T = A'⁻¹B'
     */
    static const char vs_code_bezier_outline[] =
        "float3x2 transform_geometry;\n"
        "float stroke_width;\n"
        "float4 transform_rtx;\n"
        "float4 transform_rty;\n"
        "\n"
        "struct output\n"
        "{\n"
        "    float2 p : WORLD_POSITION;\n"
        "    float4 b : BEZIER;\n"
        "    nointerpolation float2x2 stroke_transform : STROKE_TRANSFORM;\n"
        "    float4 position : SV_POSITION;\n"
        "};\n"
        "\n"
        "void main(float2 position : POSITION, float2 p0 : P0, float2 p1 : P1, float2 p2 : P2,\n"
        "        float2 prev : PREV, float2 next : NEXT, out struct output o)\n"
        "{\n"
        "    float2 q_prev, q_next, v_p, q_i, p;\n"
        "    float2x2 geom, rt;\n"
        "    float l;\n"
        "\n"
        "    geom = float2x2(transform_geometry._11_21, transform_geometry._12_22);\n"
        "    rt = float2x2(transform_rtx.xy, transform_rty.xy);\n"
        "    o.stroke_transform = rt * stroke_width * 0.5f;\n"
        "\n"
        "    p = mul(geom, position);\n"
        "    p0 = mul(geom, p0);\n"
        "    p1 = mul(geom, p1);\n"
        "    p2 = mul(geom, p2);\n"
        "\n"
        "    p -= p0;\n"
        "    p1 -= p0;\n"
        "    p2 -= p0;\n"
        "\n"
        "    q_prev = normalize(mul(geom, prev));\n"
        "    q_next = normalize(mul(geom, next));\n"
        "\n"
        "    v_p = float2(-q_prev.y, q_prev.x);\n"
        "    l = -dot(v_p, q_next) / (1.0f + dot(q_prev, q_next));\n"
        "    q_i = l * q_prev + v_p;\n"
        "    p += 0.5f * stroke_width * q_i;\n"
        "\n"
        "    v_p = mul(rt, p2);\n"
        "    v_p = normalize(float2(-v_p.y, v_p.x));\n"
        "    if (abs(dot(mul(rt, p1), v_p)) < 1.0f)\n"
        "    {\n"
        "        o.b.xzw = float3(0.0f, 0.0f, 0.0f);\n"
        "        o.b.y = dot(mul(rt, p), v_p);\n"
        "    }\n"
        "    else\n"
        "    {\n"
        "        o.b.zw = sign(dot(mul(rt, p1), v_p)) * v_p;\n"
        "        v_p = -float2(-p.y, p.x) / dot(float2(-p1.y, p1.x), p2);\n"
        "        o.b.x = dot(v_p, p1 - 0.5f * p2);\n"
        "        o.b.y = dot(v_p, p1);\n"
        "    }\n"
        "\n"
        "    o.p = mul(float3(position, 1.0f), transform_geometry) + 0.5f * stroke_width * q_i;\n"
        "    position = mul(float2x3(transform_rtx.xyz, transform_rty.xyz), float3(o.p, 1.0f))\n"
        "            * float2(transform_rtx.w, transform_rty.w);\n"
        "    o.position = float4(position + float2(-1.0f, 1.0f), 0.0f, 1.0f);\n"
        "}\n";
    /*     ⎡p0.x p0.y 1⎤
     * A = ⎢p1.x p1.y 1⎥
     *     ⎣p2.x p2.y 1⎦
     *
     *     ⎡1 0⎤
     * B = ⎢1 1⎥
     *     ⎣0 1⎦
     *
     * A' = ⎡p1.x-p0.x p1.y-p0.y⎤
     *      ⎣p2.x-p0.x p2.y-p0.y⎦
     *
     * B' = ⎡ 0 1⎤
     *      ⎣-1 1⎦
     *
     * A'T = B'
     * T = A'⁻¹B' = (B'⁻¹A')⁻¹
     */
    static const char vs_code_arc_outline[] =
        "float3x2 transform_geometry;\n"
        "float stroke_width;\n"
        "float4 transform_rtx;\n"
        "float4 transform_rty;\n"
        "\n"
        "struct output\n"
        "{\n"
        "    float2 p : WORLD_POSITION;\n"
        "    float4 b : BEZIER;\n"
        "    nointerpolation float2x2 stroke_transform : STROKE_TRANSFORM;\n"
        "    float4 position : SV_POSITION;\n"
        "};\n"
        "\n"
        "void main(float2 position : POSITION, float2 p0 : P0, float2 p1 : P1, float2 p2 : P2,\n"
        "        float2 prev : PREV, float2 next : NEXT, out struct output o)\n"
        "{\n"
        "    float2 q_prev, q_next, v_p, q_i, p;\n"
        "    float2x2 geom, rt, p_inv;\n"
        "    float l;\n"
        "    float a;\n"
        "    float2 bc;\n"
        "\n"
        "    geom = float2x2(transform_geometry._11_21, transform_geometry._12_22);\n"
        "    rt = float2x2(transform_rtx.xy, transform_rty.xy);\n"
        "    o.stroke_transform = rt * stroke_width * 0.5f;\n"
        "\n"
        "    p = mul(geom, position);\n"
        "    p0 = mul(geom, p0);\n"
        "    p1 = mul(geom, p1);\n"
        "    p2 = mul(geom, p2);\n"
        "\n"
        "    p -= p0;\n"
        "    p1 -= p0;\n"
        "    p2 -= p0;\n"
        "\n"
        "    q_prev = normalize(mul(geom, prev));\n"
        "    q_next = normalize(mul(geom, next));\n"
        "\n"
        "    v_p = float2(-q_prev.y, q_prev.x);\n"
        "    l = -dot(v_p, q_next) / (1.0f + dot(q_prev, q_next));\n"
        "    q_i = l * q_prev + v_p;\n"
        "    p += 0.5f * stroke_width * q_i;\n"
        "\n"
        "    p_inv = float2x2(p1.y, -p1.x, p2.y - p1.y, p1.x - p2.x) / (p1.x * p2.y - p2.x * p1.y);\n"
        "    o.b.xy = mul(p_inv, p) + float2(1.0f, 0.0f);\n"
        "    o.b.zw = 0.0f;\n"
        "\n"
        "    o.p = mul(float3(position, 1.0f), transform_geometry) + 0.5f * stroke_width * q_i;\n"
        "    position = mul(float2x3(transform_rtx.xyz, transform_rty.xyz), float3(o.p, 1.0f))\n"
        "            * float2(transform_rtx.w, transform_rty.w);\n"
        "    o.position = float4(position + float2(-1.0f, 1.0f), 0.0f, 1.0f);\n"
        "}\n";
    static const char vs_code_triangle[] =
        "float3x2 transform_geometry;\n"
        "float4 transform_rtx;\n"
        "float4 transform_rty;\n"
        "\n"
        "struct output\n"
        "{\n"
        "    float2 p : WORLD_POSITION;\n"
        "    float4 b : BEZIER;\n"
        "    nointerpolation float2x2 stroke_transform : STROKE_TRANSFORM;\n"
        "    float4 position : SV_POSITION;\n"
        "};\n"
        "\n"
        "void main(float2 position : POSITION, out struct output o)\n"
        "{\n"
        "    o.p = mul(float3(position, 1.0f), transform_geometry);\n"
        "    o.b = float4(1.0, 0.0, 1.0, 1.0);\n"
        "    o.stroke_transform = float2x2(1.0, 0.0, 0.0, 1.0);\n"
        "    position = mul(float2x3(transform_rtx.xyz, transform_rty.xyz), float3(o.p, 1.0f))\n"
        "            * float2(transform_rtx.w, transform_rty.w);\n"
        "    o.position = float4(position + float2(-1.0f, 1.0f), 0.0f, 1.0f);\n"
        "}\n";
    static const char vs_code_curve[] =
        "float3x2 transform_geometry;\n"
        "float4 transform_rtx;\n"
        "float4 transform_rty;\n"
        "\n"
        "struct output\n"
        "{\n"
        "    float2 p : WORLD_POSITION;\n"
        "    float4 b : BEZIER;\n"
        "    nointerpolation float2x2 stroke_transform : STROKE_TRANSFORM;\n"
        "    float4 position : SV_POSITION;\n"
        "};\n"
        "\n"
        "void main(float2 position : POSITION, float3 texcoord : TEXCOORD0, out struct output o)\n"
        "{\n"
        "    o.p = mul(float3(position, 1.0f), transform_geometry);\n"
        "    o.b = float4(texcoord, 1.0);\n"
        "    o.stroke_transform = float2x2(1.0, 0.0, 0.0, 1.0);\n"
        "    position = mul(float2x3(transform_rtx.xyz, transform_rty.xyz), float3(o.p, 1.0f))\n"
        "            * float2(transform_rtx.w, transform_rty.w);\n"
        "    o.position = float4(position + float2(-1.0f, 1.0f), 0.0f, 1.0f);\n"
        "}\n";
    static const char ps_code[] =
        "#define BRUSH_TYPE_SOLID    0\n"
        "#define BRUSH_TYPE_LINEAR   1\n"
        "#define BRUSH_TYPE_RADIAL   2\n"
        "#define BRUSH_TYPE_BITMAP   3\n"
        "#define BRUSH_TYPE_COUNT    4\n"
        "\n"
        "bool outline;\n"
        "bool is_arc;\n"
        "struct brush\n"
        "{\n"
        "    uint type;\n"
        "    float opacity;\n"
        "    float4 data[3];\n"
        "} colour_brush, opacity_brush;\n"
        "\n"
        "SamplerState s0, s1;\n"
        "Texture2D t0, t1;\n"
        "Buffer<float4> b0, b1;\n"
        "\n"
        "struct input\n"
        "{\n"
        "    float2 p : WORLD_POSITION;\n"
        "    float4 b : BEZIER;\n"
        "    nointerpolation float2x2 stroke_transform : STROKE_TRANSFORM;\n"
        "};\n"
        "\n"
        "float4 sample_gradient(Buffer<float4> gradient, uint stop_count, float position)\n"
        "{\n"
        "    float4 c_low, c_high;\n"
        "    float p_low, p_high;\n"
        "    uint i;\n"
        "\n"
        "    p_low = gradient.Load(0).x;\n"
        "    c_low = gradient.Load(1);\n"
        "    c_high = c_low;\n"
        "\n"
        "    if (position < p_low)\n"
        "        return c_low;\n"
        "\n"
        "    for (i = 1; i < stop_count; ++i)\n"
        "    {\n"
        "        p_high = gradient.Load(i * 2).x;\n"
        "        c_high = gradient.Load(i * 2 + 1);\n"
        "\n"
        "        if (position >= p_low && position <= p_high)\n"
        "            return lerp(c_low, c_high, (position - p_low) / (p_high - p_low));\n"
        "\n"
        "        p_low = p_high;\n"
        "        c_low = c_high;\n"
        "    }\n"
        "\n"
        "    return c_high;\n"
        "}\n"
        "\n"
        "float4 brush_linear(struct brush brush, Buffer<float4> gradient, float2 position)\n"
        "{\n"
        "    float2 start, end, v_p, v_q;\n"
        "    uint stop_count;\n"
        "    float p;\n"
        "\n"
        "    start = brush.data[0].xy;\n"
        "    end = brush.data[0].zw;\n"
        "    stop_count = asuint(brush.data[1].x);\n"
        "\n"
        "    v_p = position - start;\n"
        "    v_q = end - start;\n"
        "    p = dot(v_q, v_p) / dot(v_q, v_q);\n"
        "\n"
        "    return sample_gradient(gradient, stop_count, p);\n"
        "}\n"
        "\n"
        "float4 brush_radial(struct brush brush, Buffer<float4> gradient, float2 position)\n"
        "{\n"
        "    float2 centre, offset, ra, rb, v_p, v_q, r;\n"
        "    float b, c, l, t;\n"
        "    uint stop_count;\n"
        "\n"
        "    centre = brush.data[0].xy;\n"
        "    offset = brush.data[0].zw;\n"
        "    ra = brush.data[1].xy;\n"
        "    rb = brush.data[1].zw;\n"
        "    stop_count = asuint(brush.data[2].x);\n"
        "\n"
        "    /* Project onto ra, rb. */\n"
        "    r = float2(dot(ra, ra), dot(rb, rb));\n"
        "    v_p = position - (centre + offset);\n"
        "    v_p = float2(dot(v_p, ra), dot(v_p, rb)) / r;\n"
        "    v_q = float2(dot(offset, ra), dot(offset, rb)) / r;\n"
        "\n"
        "    /* ‖t·p̂ + q⃑‖ = 1\n"
        "     * (t·p̂ + q⃑) · (t·p̂ + q⃑) = 1\n"
        "     * t² + 2·(p̂·q⃑)·t + (q⃑·q⃑) = 1\n"
        "     *\n"
        "     * b = p̂·q⃑\n"
        "     * c = q⃑·q⃑ - 1\n"
        "     * t = -b + √(b² - c) */\n"
        "    l = length(v_p);\n"
        "    b = dot(v_p, v_q) / l;\n"
        "    c = dot(v_q, v_q) - 1.0;\n"
        "    t = -b + sqrt(b * b - c);\n"
        "\n"
        "    return sample_gradient(gradient, stop_count, l / t);\n"
        "}\n"
        "\n"
        "float4 brush_bitmap(struct brush brush, Texture2D t, SamplerState s, float2 position)\n"
        "{\n"
        "    float3 transform[2];\n"
        "    bool ignore_alpha;\n"
        "    float2 texcoord;\n"
        "    float4 colour;\n"
        "\n"
        "    transform[0] = brush.data[0].xyz;\n"
        "    transform[1] = brush.data[1].xyz;\n"
        "    ignore_alpha = asuint(brush.data[1].w);\n"
        "\n"
        "    texcoord.x = dot(position.xy, transform[0].xy) + transform[0].z;\n"
        "    texcoord.y = dot(position.xy, transform[1].xy) + transform[1].z;\n"
        "    colour = t.Sample(s, texcoord);\n"
        "    if (ignore_alpha)\n"
        "        colour.a = 1.0;\n"
        "    return colour;\n"
        "}\n"
        "\n"
        "float4 sample_brush(struct brush brush, Texture2D t, SamplerState s, Buffer<float4> b, float2 position)\n"
        "{\n"
        "    if (brush.type == BRUSH_TYPE_SOLID)\n"
        "        return brush.data[0] * brush.opacity;\n"
        "    if (brush.type == BRUSH_TYPE_LINEAR)\n"
        "        return brush_linear(brush, b, position) * brush.opacity;\n"
        "    if (brush.type == BRUSH_TYPE_RADIAL)\n"
        "        return brush_radial(brush, b, position) * brush.opacity;\n"
        "    if (brush.type == BRUSH_TYPE_BITMAP)\n"
        "        return brush_bitmap(brush, t, s, position) * brush.opacity;\n"
        "    return float4(0.0, 0.0, 0.0, brush.opacity);\n"
        "}\n"
        "\n"
        "float4 main(struct input i) : SV_Target\n"
        "{\n"
        "    float4 colour;\n"
        "\n"
        "    colour = sample_brush(colour_brush, t0, s0, b0, i.p);\n"
        "    if (opacity_brush.type < BRUSH_TYPE_COUNT)\n"
        "        colour *= sample_brush(opacity_brush, t1, s1, b1, i.p).a;\n"
        "\n"
        "    if (outline)\n"
        "    {\n"
        "        float2 du, dv, df;\n"
        "        float4 uv;\n"
        "\n"
        "        /* Evaluate the implicit form of the curve (u² - v = 0\n"
        "         * for Béziers, u² + v² - 1 = 0 for arcs) in texture\n"
        "         * space, using the screen-space partial derivatives\n"
        "         * to convert the calculated distance to object space.\n"
        "         *\n"
        "         * d(x, y) = |f(x, y)| / ‖∇f(x, y)‖\n"
        "         *         = |f(x, y)| / √((∂f/∂x)² + (∂f/∂y)²)\n"
        "         *\n"
        "         * For Béziers:\n"
        "         * f(x, y) = u(x, y)² - v(x, y)\n"
        "         * ∂f/∂x = 2u · ∂u/∂x - ∂v/∂x\n"
        "         * ∂f/∂y = 2u · ∂u/∂y - ∂v/∂y\n"
        "         *\n"
        "         * For arcs:\n"
        "         * f(x, y) = u(x, y)² + v(x, y)² - 1\n"
        "         * ∂f/∂x = 2u · ∂u/∂x + 2v · ∂v/∂x\n"
        "         * ∂f/∂y = 2u · ∂u/∂y + 2v · ∂v/∂y */\n"
        "        uv = i.b;\n"
        "        du = float2(ddx(uv.x), ddy(uv.x));\n"
        "        dv = float2(ddx(uv.y), ddy(uv.y));\n"
        "\n"
        "        if (!is_arc)\n"
        "        {\n"
        "            df = 2.0f * uv.x * du - dv;\n"
        "\n"
        "            clip(dot(df, uv.zw));\n"
        "            clip(length(mul(i.stroke_transform, df)) - abs(uv.x * uv.x - uv.y));\n"
        "        }\n"
        "        else\n"
        "        {\n"
        "            df = 2.0f * uv.x * du + 2.0f * uv.y * dv;\n"
        "\n"
        "            clip(dot(df, uv.zw));\n"
        "            clip(length(mul(i.stroke_transform, df)) - abs(uv.x * uv.x + uv.y * uv.y - 1.0f));\n"
        "        }\n"
        "    }\n"
        "    else\n"
        "    {\n"
        "        /* Evaluate the implicit form of the curve in texture space.\n"
        "         * \"i.b.z\" determines which side of the curve is shaded. */\n"
        "        if (!is_arc)\n"
        "        {\n"
        "            clip((i.b.x * i.b.x - i.b.y) * i.b.z);\n"
        "        }\n"
        "        else\n"
        "        {\n"
        "            clip((i.b.x * i.b.x + i.b.y * i.b.y - 1.0) * i.b.z);\n"
        "        }\n"
        "    }\n"
        "\n"
        "    return colour;\n"
        "}\n";
    static const struct shape_info
    {
        enum d2d_shape_type shape_type;
        const D3D11_INPUT_ELEMENT_DESC *il_desc;
        unsigned int il_element_count;
        const char *name;
        const char *vs_code;
        size_t vs_code_size;
    }
    shape_info[] =
    {
        {D2D_SHAPE_TYPE_OUTLINE,        il_desc_outline,        ARRAY_SIZE(il_desc_outline),
         "outline",                     vs_code_outline,        sizeof(vs_code_outline) - 1},
        {D2D_SHAPE_TYPE_BEZIER_OUTLINE, il_desc_curve_outline,  ARRAY_SIZE(il_desc_curve_outline),
         "bezier_outline",              vs_code_bezier_outline, sizeof(vs_code_bezier_outline) - 1},
        {D2D_SHAPE_TYPE_ARC_OUTLINE,    il_desc_curve_outline,  ARRAY_SIZE(il_desc_curve_outline),
         "arc_outline",                 vs_code_arc_outline,    sizeof(vs_code_arc_outline) - 1},
        {D2D_SHAPE_TYPE_TRIANGLE,       il_desc_triangle,       ARRAY_SIZE(il_desc_triangle),
         "triangle",                    vs_code_triangle,       sizeof(vs_code_triangle) - 1},
        {D2D_SHAPE_TYPE_CURVE,          il_desc_curve,          ARRAY_SIZE(il_desc_curve),
         "curve",                       vs_code_curve,          sizeof(vs_code_curve) - 1},
    };
    static const struct
    {
        float x, y;
    }
    quad[] =
    {
        {-1.0f,  1.0f},
        {-1.0f, -1.0f},
        { 1.0f,  1.0f},
        { 1.0f, -1.0f},
    };
    static const UINT16 indices[] = {0, 1, 2, 2, 1, 3};
    static const D3D_FEATURE_LEVEL feature_levels = D3D_FEATURE_LEVEL_10_0;

    render_target->ID2D1DeviceContext6_iface.lpVtbl = &d2d_device_context_vtbl;
    render_target->ID2D1GdiInteropRenderTarget_iface.lpVtbl = &d2d_gdi_interop_render_target_vtbl;
    render_target->IDWriteTextRenderer_iface.lpVtbl = &d2d_text_renderer_vtbl;
    render_target->IUnknown_iface.lpVtbl = &d2d_device_context_inner_unknown_vtbl;
    render_target->refcount = 1;
    ID2D1Device1_GetFactory((ID2D1Device1 *)&device->ID2D1Device6_iface, &render_target->factory);
    render_target->device = device;
    ID2D1Device6_AddRef(&render_target->device->ID2D1Device6_iface);

    factory = unsafe_impl_from_ID2D1Factory(render_target->factory);
    if (factory->factory_type == D2D1_FACTORY_TYPE_MULTI_THREADED)
        render_target->cs = &factory->cs;

    render_target->outer_unknown = outer_unknown ? outer_unknown : &render_target->IUnknown_iface;
    render_target->ops = ops;

    if (FAILED(hr = IDXGIDevice_QueryInterface(device->dxgi_device,
            &IID_ID3D11Device1, (void **)&render_target->d3d_device)))
    {
        WARN("Failed to query ID3D11Device1 interface, hr %#lx.\n", hr);
        goto err;
    }

    if (FAILED(hr = ID3D11Device1_CreateDeviceContextState(render_target->d3d_device,
            0, &feature_levels, 1, D3D11_SDK_VERSION, &IID_ID3D11Device1, NULL,
            &render_target->d3d_state)))
    {
        WARN("Failed to create device context state, hr %#lx.\n", hr);
        goto err;
    }

    for (i = 0; i < ARRAY_SIZE(shape_info); ++i)
    {
        const struct shape_info *si = &shape_info[i];

        if (FAILED(hr = D3DCompile(si->vs_code, si->vs_code_size, si->name, NULL, NULL,
                "main", "vs_4_0", 0, 0, &compiled, NULL)))
        {
            WARN("Failed to compile shader for shape type %#x, hr %#lx.\n", si->shape_type, hr);
            goto err;
        }

        if (FAILED(hr = ID3D11Device1_CreateInputLayout(render_target->d3d_device, si->il_desc, si->il_element_count,
                ID3D10Blob_GetBufferPointer(compiled), ID3D10Blob_GetBufferSize(compiled),
                &render_target->shape_resources[si->shape_type].il)))
        {
            WARN("Failed to create input layout for shape type %#x, hr %#lx.\n", si->shape_type, hr);
            ID3D10Blob_Release(compiled);
            goto err;
        }

        if (FAILED(hr = ID3D11Device1_CreateVertexShader(render_target->d3d_device,
                ID3D10Blob_GetBufferPointer(compiled), ID3D10Blob_GetBufferSize(compiled),
                NULL, &render_target->shape_resources[si->shape_type].vs)))
        {
            WARN("Failed to create vertex shader for shape type %#x, hr %#lx.\n", si->shape_type, hr);
            ID3D10Blob_Release(compiled);
            goto err;
        }

        ID3D10Blob_Release(compiled);
    }

    buffer_desc.ByteWidth = sizeof(struct d2d_vs_cb);
    buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
    buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    buffer_desc.MiscFlags = 0;

    if (FAILED(hr = ID3D11Device1_CreateBuffer(render_target->d3d_device, &buffer_desc, NULL,
            &render_target->vs_cb)))
    {
        WARN("Failed to create constant buffer, hr %#lx.\n", hr);
        goto err;
    }

    if (FAILED(hr = D3DCompile(ps_code, sizeof(ps_code) - 1, "ps", NULL, NULL, "main", "ps_4_0", 0, 0, &compiled, NULL)))
    {
        WARN("Failed to compile the pixel shader, hr %#lx.\n", hr);
        goto err;
    }

    if (FAILED(hr = ID3D11Device1_CreatePixelShader(render_target->d3d_device,
            ID3D10Blob_GetBufferPointer(compiled), ID3D10Blob_GetBufferSize(compiled),
            NULL, &render_target->ps)))
    {
        WARN("Failed to create pixel shader, hr %#lx.\n", hr);
        ID3D10Blob_Release(compiled);
        goto err;
    }

    ID3D10Blob_Release(compiled);

    buffer_desc.ByteWidth = sizeof(struct d2d_ps_cb);
    buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
    buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    buffer_desc.MiscFlags = 0;

    if (FAILED(hr = ID3D11Device1_CreateBuffer(render_target->d3d_device, &buffer_desc, NULL,
            &render_target->ps_cb)))
    {
        WARN("Failed to create constant buffer, hr %#lx.\n", hr);
        goto err;
    }

    buffer_desc.ByteWidth = sizeof(indices);
    buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    buffer_desc.CPUAccessFlags = 0;
    buffer_desc.MiscFlags = 0;

    buffer_data.pSysMem = indices;
    buffer_data.SysMemPitch = 0;
    buffer_data.SysMemSlicePitch = 0;

    if (FAILED(hr = ID3D11Device1_CreateBuffer(render_target->d3d_device,
            &buffer_desc, &buffer_data, &render_target->ib)))
    {
        WARN("Failed to create clear index buffer, hr %#lx.\n", hr);
        goto err;
    }

    buffer_desc.ByteWidth = sizeof(quad);
    buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    buffer_data.pSysMem = quad;

    render_target->vb_stride = sizeof(*quad);
    if (FAILED(hr = ID3D11Device1_CreateBuffer(render_target->d3d_device,
            &buffer_desc, &buffer_data, &render_target->vb)))
    {
        WARN("Failed to create clear vertex buffer, hr %#lx.\n", hr);
        goto err;
    }

    rs_desc.FillMode = D3D11_FILL_SOLID;
    rs_desc.CullMode = D3D11_CULL_NONE;
    rs_desc.FrontCounterClockwise = FALSE;
    rs_desc.DepthBias = 0;
    rs_desc.DepthBiasClamp = 0.0f;
    rs_desc.SlopeScaledDepthBias = 0.0f;
    rs_desc.DepthClipEnable = TRUE;
    rs_desc.ScissorEnable = TRUE;
    rs_desc.MultisampleEnable = FALSE;
    rs_desc.AntialiasedLineEnable = FALSE;
    if (FAILED(hr = ID3D11Device1_CreateRasterizerState(render_target->d3d_device, &rs_desc, &render_target->rs)))
    {
        WARN("Failed to create clear rasteriser state, hr %#lx.\n", hr);
        goto err;
    }

    if (FAILED(hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
            &IID_IDWriteFactory, (IUnknown **)&dwrite_factory)))
    {
        ERR("Failed to create dwrite factory, hr %#lx.\n", hr);
        goto err;
    }

    hr = IDWriteFactory_CreateRenderingParams(dwrite_factory, &render_target->default_text_rendering_params);
    IDWriteFactory_Release(dwrite_factory);
    if (FAILED(hr))
    {
        ERR("Failed to create default text rendering parameters, hr %#lx.\n", hr);
        goto err;
    }

    render_target->drawing_state.transform = identity;

    if (!d2d_clip_stack_init(&render_target->clip_stack))
    {
        WARN("Failed to initialize clip stack.\n");
        hr = E_FAIL;
        goto err;
    }

    render_target->desc.dpiX = 96.0f;
    render_target->desc.dpiY = 96.0f;

    return S_OK;

err:
    if (render_target->default_text_rendering_params)
        IDWriteRenderingParams_Release(render_target->default_text_rendering_params);
    if (render_target->rs)
        ID3D11RasterizerState_Release(render_target->rs);
    if (render_target->vb)
        ID3D11Buffer_Release(render_target->vb);
    if (render_target->ib)
        ID3D11Buffer_Release(render_target->ib);
    if (render_target->ps_cb)
        ID3D11Buffer_Release(render_target->ps_cb);
    if (render_target->ps)
        ID3D11PixelShader_Release(render_target->ps);
    if (render_target->vs_cb)
        ID3D11Buffer_Release(render_target->vs_cb);
    for (i = 0; i < D2D_SHAPE_TYPE_COUNT; ++i)
    {
        if (render_target->shape_resources[i].vs)
            ID3D11VertexShader_Release(render_target->shape_resources[i].vs);
        if (render_target->shape_resources[i].il)
            ID3D11InputLayout_Release(render_target->shape_resources[i].il);
    }
    if (render_target->d3d_state)
        ID3DDeviceContextState_Release(render_target->d3d_state);
    if (render_target->d3d_device)
        ID3D11Device1_Release(render_target->d3d_device);
    ID2D1Device6_Release(&render_target->device->ID2D1Device6_iface);
    ID2D1Factory_Release(render_target->factory);
    return hr;
}

HRESULT d2d_d3d_create_render_target(struct d2d_device *device, IDXGISurface *surface, IUnknown *outer_unknown,
        const struct d2d_device_context_ops *ops, const D2D1_RENDER_TARGET_PROPERTIES *desc, void **render_target)
{
    D2D1_BITMAP_PROPERTIES1 bitmap_desc;
    struct d2d_device_context *object;
    ID2D1Bitmap1 *bitmap;
    HRESULT hr;

    if (desc->type != D2D1_RENDER_TARGET_TYPE_DEFAULT && desc->type != D2D1_RENDER_TARGET_TYPE_HARDWARE)
        WARN("Ignoring render target type %#x.\n", desc->type);
    if (desc->usage != D2D1_RENDER_TARGET_USAGE_NONE)
        FIXME("Ignoring render target usage %#x.\n", desc->usage);
    if (desc->minLevel != D2D1_FEATURE_LEVEL_DEFAULT)
        WARN("Ignoring feature level %#x.\n", desc->minLevel);

    bitmap_desc.dpiX = desc->dpiX;
    bitmap_desc.dpiY = desc->dpiY;

    if (bitmap_desc.dpiX == 0.0f && bitmap_desc.dpiY == 0.0f)
    {
        bitmap_desc.dpiX = 96.0f;
        bitmap_desc.dpiY = 96.0f;
    }
    else if (bitmap_desc.dpiX <= 0.0f || bitmap_desc.dpiY <= 0.0f)
        return E_INVALIDARG;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d2d_device_context_init(object, device, outer_unknown, ops)))
    {
        WARN("Failed to initialise render target, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    ID2D1DeviceContext6_SetDpi(&object->ID2D1DeviceContext6_iface, bitmap_desc.dpiX, bitmap_desc.dpiY);

    if (surface)
    {
        bitmap_desc.pixelFormat = desc->pixelFormat;
        bitmap_desc.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
        if (desc->usage & D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE)
            bitmap_desc.bitmapOptions |= D2D1_BITMAP_OPTIONS_GDI_COMPATIBLE;
        bitmap_desc.colorContext = NULL;

        if (FAILED(hr = ID2D1DeviceContext6_CreateBitmapFromDxgiSurface(&object->ID2D1DeviceContext6_iface,
                surface, &bitmap_desc, &bitmap)))
        {
            WARN("Failed to create target bitmap, hr %#lx.\n", hr);
            IUnknown_Release(&object->IUnknown_iface);
            return hr;
        }

        ID2D1DeviceContext6_SetTarget(&object->ID2D1DeviceContext6_iface, (ID2D1Image *)bitmap);
        ID2D1Bitmap1_Release(bitmap);
    }
    else
        object->desc.pixelFormat = desc->pixelFormat;

    TRACE("Created render target %p.\n", object);
    *render_target = outer_unknown ? &object->IUnknown_iface : (IUnknown *)&object->ID2D1DeviceContext6_iface;

    return S_OK;
}

static HRESULT WINAPI d2d_device_QueryInterface(ID2D1Device6 *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1Device6)
            || IsEqualGUID(iid, &IID_ID2D1Device5)
            || IsEqualGUID(iid, &IID_ID2D1Device4)
            || IsEqualGUID(iid, &IID_ID2D1Device3)
            || IsEqualGUID(iid, &IID_ID2D1Device2)
            || IsEqualGUID(iid, &IID_ID2D1Device1)
            || IsEqualGUID(iid, &IID_ID2D1Device)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1Device6_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d2d_device_AddRef(ID2D1Device6 *iface)
{
    struct d2d_device *device = impl_from_ID2D1Device(iface);
    ULONG refcount = InterlockedIncrement(&device->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

void d2d_device_indexed_objects_clear(struct d2d_indexed_objects *objects)
{
    size_t i;

    for (i = 0; i < objects->count; ++i)
        IUnknown_Release(objects->elements[i].object);
    free(objects->elements);
    objects->elements = NULL;
}

static ULONG WINAPI d2d_device_Release(ID2D1Device6 *iface)
{
    struct d2d_device *device = impl_from_ID2D1Device(iface);
    ULONG refcount = InterlockedDecrement(&device->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        IDXGIDevice_Release(device->dxgi_device);
        ID2D1Factory1_Release(device->factory);
        d2d_device_indexed_objects_clear(&device->shaders);
        free(device);
    }

    return refcount;
}

static void WINAPI d2d_device_GetFactory(ID2D1Device6 *iface, ID2D1Factory **factory)
{
    struct d2d_device *device = impl_from_ID2D1Device(iface);

    TRACE("iface %p, factory %p.\n", iface, factory);

    *factory = (ID2D1Factory *)device->factory;
    ID2D1Factory1_AddRef(device->factory);
}

static HRESULT d2d_device_create_device_context(struct d2d_device *device,
        D2D1_DEVICE_CONTEXT_OPTIONS options, REFIID iid, void **context)
{
    struct d2d_device_context *object;
    HRESULT hr;

    if (options)
        FIXME("Options are ignored %#x.\n", options);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d2d_device_context_init(object, device, NULL, NULL)))
    {
        WARN("Failed to initialise device context, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    TRACE("Created device context %p.\n", object);

    hr = ID2D1DeviceContext6_QueryInterface(&object->ID2D1DeviceContext6_iface, iid, context);
    ID2D1DeviceContext6_Release(&object->ID2D1DeviceContext6_iface);

    return hr;
}

static HRESULT WINAPI d2d_device_CreateDeviceContext(ID2D1Device6 *iface, D2D1_DEVICE_CONTEXT_OPTIONS options,
        ID2D1DeviceContext **context)
{
    struct d2d_device *device = impl_from_ID2D1Device(iface);

    TRACE("iface %p, options %#x, context %p.\n", iface, options, context);

    return d2d_device_create_device_context(device, options, &IID_ID2D1DeviceContext, (void **)context);
}

static HRESULT WINAPI d2d_device_CreatePrintControl(ID2D1Device6 *iface, IWICImagingFactory *wic_factory,
        IPrintDocumentPackageTarget *document_target, const D2D1_PRINT_CONTROL_PROPERTIES *desc,
        ID2D1PrintControl **print_control)
{
    FIXME("iface %p, wic_factory %p, document_target %p, desc %p, print_control %p stub!\n", iface, wic_factory,
            document_target, desc, print_control);

    return E_NOTIMPL;
}

static void WINAPI d2d_device_SetMaximumTextureMemory(ID2D1Device6 *iface, UINT64 max_texture_memory)
{
    FIXME("iface %p, max_texture_memory %s stub!\n", iface, wine_dbgstr_longlong(max_texture_memory));
}

static UINT64 WINAPI d2d_device_GetMaximumTextureMemory(ID2D1Device6 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static HRESULT WINAPI d2d_device_ClearResources(ID2D1Device6 *iface, UINT msec_since_use)
{
    FIXME("iface %p, msec_since_use %u stub!\n", iface, msec_since_use);

    return E_NOTIMPL;
}

static D2D1_RENDERING_PRIORITY WINAPI d2d_device_GetRenderingPriority(ID2D1Device6 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return D2D1_RENDERING_PRIORITY_NORMAL;
}

static void WINAPI d2d_device_SetRenderingPriority(ID2D1Device6 *iface, D2D1_RENDERING_PRIORITY priority)
{
    FIXME("iface %p, priority %#x stub!\n", iface, priority);
}

static HRESULT WINAPI d2d_device_CreateDeviceContext1(ID2D1Device6 *iface, D2D1_DEVICE_CONTEXT_OPTIONS options,
        ID2D1DeviceContext1 **context)
{
    struct d2d_device *device = impl_from_ID2D1Device(iface);

    TRACE("iface %p, options %#x, context %p.\n", iface, options, context);

    return d2d_device_create_device_context(device, options, &IID_ID2D1DeviceContext1, (void **)context);
}

static HRESULT STDMETHODCALLTYPE d2d_device_ID2D1Device2_CreateDeviceContext(ID2D1Device6 *iface,
        D2D1_DEVICE_CONTEXT_OPTIONS options, ID2D1DeviceContext2 **context)
{
    struct d2d_device *device = impl_from_ID2D1Device(iface);

    TRACE("iface %p, options %#x, context %p.\n", iface, options, context);

    return d2d_device_create_device_context(device, options, &IID_ID2D1DeviceContext2, (void **)context);
}

static void STDMETHODCALLTYPE d2d_device_FlushDeviceContexts(ID2D1Device6 *iface,
        ID2D1Bitmap *bitmap)
{
    FIXME("iface %p, bitmap %p stub!\n", iface, bitmap);
}

static HRESULT STDMETHODCALLTYPE d2d_device_GetDxgiDevice(ID2D1Device6 *iface,
        IDXGIDevice **dxgi_device)
{
    struct d2d_device *device = impl_from_ID2D1Device(iface);

    TRACE("iface %p, dxgi_device %p.\n", iface, dxgi_device);

    if (!device->allow_get_dxgi_device)
    {
        *dxgi_device = NULL;
        return D2DERR_INVALID_CALL;
    }

    IDXGIDevice_AddRef(device->dxgi_device);
    *dxgi_device = device->dxgi_device;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE d2d_device_ID2D1Device3_CreateDeviceContext(ID2D1Device6 *iface,
        D2D1_DEVICE_CONTEXT_OPTIONS options, ID2D1DeviceContext3 **context)
{
    struct d2d_device *device = impl_from_ID2D1Device(iface);

    TRACE("iface %p, options %#x, context %p.\n", iface, options, context);

    return d2d_device_create_device_context(device, options, &IID_ID2D1DeviceContext3, (void **)context);
}

static HRESULT STDMETHODCALLTYPE d2d_device_ID2D1Device4_CreateDeviceContext(ID2D1Device6 *iface,
        D2D1_DEVICE_CONTEXT_OPTIONS options, ID2D1DeviceContext4 **context)
{
    struct d2d_device *device = impl_from_ID2D1Device(iface);

    TRACE("iface %p, options %#x, context %p.\n", iface, options, context);

    return d2d_device_create_device_context(device, options, &IID_ID2D1DeviceContext4, (void **)context);
}

static void STDMETHODCALLTYPE d2d_device_SetMaximumColorGlyphCacheMemory(ID2D1Device6 *iface,
        UINT64 size)
{
    FIXME("iface %p, size %s stub!\n", iface, wine_dbgstr_longlong(size));
}

static UINT64 STDMETHODCALLTYPE d2d_device_GetMaximumColorGlyphCacheMemory(ID2D1Device6 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static HRESULT STDMETHODCALLTYPE d2d_device_ID2D1Device5_CreateDeviceContext(ID2D1Device6 *iface,
        D2D1_DEVICE_CONTEXT_OPTIONS options, ID2D1DeviceContext5 **context)
{
    struct d2d_device *device = impl_from_ID2D1Device(iface);

    TRACE("iface %p, options %#x, context %p.\n", iface, options, context);

    return d2d_device_create_device_context(device, options, &IID_ID2D1DeviceContext5, (void **)context);
}

static HRESULT STDMETHODCALLTYPE d2d_device_ID2D1Device6_CreateDeviceContext(ID2D1Device6 *iface,
        D2D1_DEVICE_CONTEXT_OPTIONS options, ID2D1DeviceContext6 **context)
{
    struct d2d_device *device = impl_from_ID2D1Device(iface);

    TRACE("iface %p, options %#x, context %p.\n", iface, options, context);

    return d2d_device_create_device_context(device, options, &IID_ID2D1DeviceContext6, (void **)context);
}

static const struct ID2D1Device6Vtbl d2d_device_vtbl =
{
    d2d_device_QueryInterface,
    d2d_device_AddRef,
    d2d_device_Release,
    d2d_device_GetFactory,
    d2d_device_CreateDeviceContext,
    d2d_device_CreatePrintControl,
    d2d_device_SetMaximumTextureMemory,
    d2d_device_GetMaximumTextureMemory,
    d2d_device_ClearResources,
    d2d_device_GetRenderingPriority,
    d2d_device_SetRenderingPriority,
    d2d_device_CreateDeviceContext1,
    d2d_device_ID2D1Device2_CreateDeviceContext,
    d2d_device_FlushDeviceContexts,
    d2d_device_GetDxgiDevice,
    d2d_device_ID2D1Device3_CreateDeviceContext,
    d2d_device_ID2D1Device4_CreateDeviceContext,
    d2d_device_SetMaximumColorGlyphCacheMemory,
    d2d_device_GetMaximumColorGlyphCacheMemory,
    d2d_device_ID2D1Device5_CreateDeviceContext,
    d2d_device_ID2D1Device6_CreateDeviceContext,
};

struct d2d_device *unsafe_impl_from_ID2D1Device(ID2D1Device1 *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == (ID2D1Device1Vtbl *)&d2d_device_vtbl);
    return CONTAINING_RECORD(iface, struct d2d_device, ID2D1Device6_iface);
}

void d2d_device_init(struct d2d_device *device, ID2D1Factory1 *factory, IDXGIDevice *dxgi_device,
    bool allow_get_dxgi_device)
{
    device->ID2D1Device6_iface.lpVtbl = &d2d_device_vtbl;
    device->refcount = 1;
    device->factory = factory;
    ID2D1Factory1_AddRef(device->factory);
    device->dxgi_device = dxgi_device;
    IDXGIDevice_AddRef(device->dxgi_device);
    device->allow_get_dxgi_device = allow_get_dxgi_device;
}

HRESULT d2d_device_add_indexed_object(struct d2d_indexed_objects *objects,
        const GUID *id, IUnknown *object)
{
    if (!d2d_array_reserve((void **)&objects->elements, &objects->size, objects->count + 1,
            sizeof(*objects->elements)))
    {
        WARN("Failed to resize elements array.\n");
        return E_OUTOFMEMORY;
    }

    objects->elements[objects->count].id = *id;
    objects->elements[objects->count].object = object;
    IUnknown_AddRef(object);
    objects->count++;

    return S_OK;
}

BOOL d2d_device_get_indexed_object(struct d2d_indexed_objects *objects, const GUID *id,
        IUnknown **object)
{
    size_t i;

    for (i = 0; i < objects->count; ++i)
    {
        if (IsEqualGUID(id, &objects->elements[i].id))
        {
            if (object)
            {
                *object = objects->elements[i].object;
                IUnknown_AddRef(*object);
            }
            return TRUE;
        }
    }

    if (object) *object = NULL;
    return FALSE;
}
