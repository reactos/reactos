/*
 * Copyright 2014 Henri Verbeet for CodeWeavers
 * Copyright 2016 Nikolay Sivov for CodeWeavers
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

static inline struct d2d_hwnd_render_target *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_hwnd_render_target, ID2D1HwndRenderTarget_iface);
}

static HRESULT d2d_hwnd_render_target_present(IUnknown *outer_unknown)
{
    struct d2d_hwnd_render_target *render_target = impl_from_IUnknown(outer_unknown);
    HRESULT hr;

    if (FAILED(hr = IDXGISwapChain_Present(render_target->swapchain, render_target->sync_interval, 0)))
        WARN("Present failed, %#lx.\n", hr);

    return S_OK;
}

static inline struct d2d_hwnd_render_target *impl_from_ID2D1HwndRenderTarget(ID2D1HwndRenderTarget *iface)
{
    return CONTAINING_RECORD(iface, struct d2d_hwnd_render_target, ID2D1HwndRenderTarget_iface);
}

static HRESULT STDMETHODCALLTYPE d2d_hwnd_render_target_QueryInterface(ID2D1HwndRenderTarget *iface,
        REFIID iid, void **out)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID2D1HwndRenderTarget)
            || IsEqualGUID(iid, &IID_ID2D1RenderTarget)
            || IsEqualGUID(iid, &IID_ID2D1Resource)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        ID2D1HwndRenderTarget_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    return IUnknown_QueryInterface(render_target->dxgi_inner, iid, out);
}

static ULONG STDMETHODCALLTYPE d2d_hwnd_render_target_AddRef(ID2D1HwndRenderTarget *iface)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);
    ULONG refcount = InterlockedIncrement(&render_target->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE d2d_hwnd_render_target_Release(ID2D1HwndRenderTarget *iface)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);
    ULONG refcount = InterlockedDecrement(&render_target->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        IUnknown_Release(render_target->dxgi_inner);
        IDXGISwapChain_Release(render_target->swapchain);
        free(render_target);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_GetFactory(ID2D1HwndRenderTarget *iface, ID2D1Factory **factory)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, factory %p.\n", iface, factory);

    ID2D1RenderTarget_GetFactory(render_target->dxgi_target, factory);
}

static HRESULT STDMETHODCALLTYPE d2d_hwnd_render_target_CreateBitmap(ID2D1HwndRenderTarget *iface,
        D2D1_SIZE_U size, const void *src_data, UINT32 pitch, const D2D1_BITMAP_PROPERTIES *desc, ID2D1Bitmap **bitmap)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, size {%u, %u}, src_data %p, pitch %u, desc %p, bitmap %p.\n",
            iface, size.width, size.height, src_data, pitch, desc, bitmap);

    return ID2D1RenderTarget_CreateBitmap(render_target->dxgi_target, size, src_data, pitch, desc, bitmap);
}

static HRESULT STDMETHODCALLTYPE d2d_hwnd_render_target_CreateBitmapFromWicBitmap(ID2D1HwndRenderTarget *iface,
        IWICBitmapSource *bitmap_source, const D2D1_BITMAP_PROPERTIES *desc, ID2D1Bitmap **bitmap)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, bitmap_source %p, desc %p, bitmap %p.\n",
            iface, bitmap_source, desc, bitmap);

    return ID2D1RenderTarget_CreateBitmapFromWicBitmap(render_target->dxgi_target, bitmap_source, desc, bitmap);
}

static HRESULT STDMETHODCALLTYPE d2d_hwnd_render_target_CreateSharedBitmap(ID2D1HwndRenderTarget *iface,
        REFIID iid, void *data, const D2D1_BITMAP_PROPERTIES *desc, ID2D1Bitmap **bitmap)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, iid %s, data %p, desc %p, bitmap %p.\n",
            iface, debugstr_guid(iid), data, desc, bitmap);

    return ID2D1RenderTarget_CreateSharedBitmap(render_target->dxgi_target, iid, data, desc, bitmap);
}

static HRESULT STDMETHODCALLTYPE d2d_hwnd_render_target_CreateBitmapBrush(ID2D1HwndRenderTarget *iface,
        ID2D1Bitmap *bitmap, const D2D1_BITMAP_BRUSH_PROPERTIES *bitmap_brush_desc,
        const D2D1_BRUSH_PROPERTIES *brush_desc, ID2D1BitmapBrush **brush)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, bitmap %p, bitmap_brush_desc %p, brush_desc %p, brush %p.\n",
            iface, bitmap, bitmap_brush_desc, brush_desc, brush);

    return ID2D1RenderTarget_CreateBitmapBrush(render_target->dxgi_target,
            bitmap, bitmap_brush_desc, brush_desc, brush);
}

static HRESULT STDMETHODCALLTYPE d2d_hwnd_render_target_CreateSolidColorBrush(ID2D1HwndRenderTarget *iface,
        const D2D1_COLOR_F *color, const D2D1_BRUSH_PROPERTIES *desc, ID2D1SolidColorBrush **brush)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, color %p, desc %p, brush %p.\n", iface, color, desc, brush);

    return ID2D1RenderTarget_CreateSolidColorBrush(render_target->dxgi_target, color, desc, brush);
}

static HRESULT STDMETHODCALLTYPE d2d_hwnd_render_target_CreateGradientStopCollection(ID2D1HwndRenderTarget *iface,
        const D2D1_GRADIENT_STOP *stops, UINT32 stop_count, D2D1_GAMMA gamma, D2D1_EXTEND_MODE extend_mode,
        ID2D1GradientStopCollection **gradient)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, stops %p, stop_count %u, gamma %#x, extend_mode %#x, gradient %p.\n",
            iface, stops, stop_count, gamma, extend_mode, gradient);

    return ID2D1RenderTarget_CreateGradientStopCollection(render_target->dxgi_target,
            stops, stop_count, gamma, extend_mode, gradient);
}

static HRESULT STDMETHODCALLTYPE d2d_hwnd_render_target_CreateLinearGradientBrush(ID2D1HwndRenderTarget *iface,
        const D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES *gradient_brush_desc, const D2D1_BRUSH_PROPERTIES *brush_desc,
        ID2D1GradientStopCollection *gradient, ID2D1LinearGradientBrush **brush)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, gradient_brush_desc %p, brush_desc %p, gradient %p, brush %p.\n",
            iface, gradient_brush_desc, brush_desc, gradient, brush);

    return ID2D1RenderTarget_CreateLinearGradientBrush(render_target->dxgi_target,
            gradient_brush_desc, brush_desc, gradient, brush);
}

static HRESULT STDMETHODCALLTYPE d2d_hwnd_render_target_CreateRadialGradientBrush(ID2D1HwndRenderTarget *iface,
        const D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES *gradient_brush_desc, const D2D1_BRUSH_PROPERTIES *brush_desc,
        ID2D1GradientStopCollection *gradient, ID2D1RadialGradientBrush **brush)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, gradient_brush_desc %p, brush_desc %p, gradient %p, brush %p.\n",
            iface, gradient_brush_desc, brush_desc, gradient, brush);

    return ID2D1RenderTarget_CreateRadialGradientBrush(render_target->dxgi_target,
            gradient_brush_desc, brush_desc, gradient, brush);
}

static HRESULT STDMETHODCALLTYPE d2d_hwnd_render_target_CreateCompatibleRenderTarget(ID2D1HwndRenderTarget *iface,
        const D2D1_SIZE_F *size, const D2D1_SIZE_U *pixel_size, const D2D1_PIXEL_FORMAT *format,
        D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS options, ID2D1BitmapRenderTarget **render_target)
{
    struct d2d_hwnd_render_target *rt = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, size %p, pixel_size %p, format %p, options %#x, render_target %p,\n",
            iface, size, pixel_size, format, options, render_target);

    return ID2D1RenderTarget_CreateCompatibleRenderTarget(rt->dxgi_target,
            size, pixel_size, format, options, render_target);
}

static HRESULT STDMETHODCALLTYPE d2d_hwnd_render_target_CreateLayer(ID2D1HwndRenderTarget *iface,
        const D2D1_SIZE_F *size, ID2D1Layer **layer)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, size %p, layer %p.\n", iface, size, layer);

    return ID2D1RenderTarget_CreateLayer(render_target->dxgi_target, size, layer);
}

static HRESULT STDMETHODCALLTYPE d2d_hwnd_render_target_CreateMesh(ID2D1HwndRenderTarget *iface, ID2D1Mesh **mesh)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, mesh %p.\n", iface, mesh);

    return ID2D1RenderTarget_CreateMesh(render_target->dxgi_target, mesh);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_DrawLine(ID2D1HwndRenderTarget *iface,
        D2D1_POINT_2F p0, D2D1_POINT_2F p1, ID2D1Brush *brush, float stroke_width, ID2D1StrokeStyle *stroke_style)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, p0 %s, p1 %s, brush %p, stroke_width %.8e, stroke_style %p.\n",
            iface, debug_d2d_point_2f(&p0), debug_d2d_point_2f(&p1), brush, stroke_width, stroke_style);

    ID2D1RenderTarget_DrawLine(render_target->dxgi_target, p0, p1, brush, stroke_width, stroke_style);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_DrawRectangle(ID2D1HwndRenderTarget *iface,
        const D2D1_RECT_F *rect, ID2D1Brush *brush, float stroke_width, ID2D1StrokeStyle *stroke_style)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, rect %s, brush %p, stroke_width %.8e, stroke_style %p.\n",
            iface, debug_d2d_rect_f(rect), brush, stroke_width, stroke_style);

    ID2D1RenderTarget_DrawRectangle(render_target->dxgi_target, rect, brush, stroke_width, stroke_style);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_FillRectangle(ID2D1HwndRenderTarget *iface,
        const D2D1_RECT_F *rect, ID2D1Brush *brush)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, rect %s, brush %p.\n", iface, debug_d2d_rect_f(rect), brush);

    ID2D1RenderTarget_FillRectangle(render_target->dxgi_target, rect, brush);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_DrawRoundedRectangle(ID2D1HwndRenderTarget *iface,
        const D2D1_ROUNDED_RECT *rect, ID2D1Brush *brush, float stroke_width, ID2D1StrokeStyle *stroke_style)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, rect %p, brush %p, stroke_width %.8e, stroke_style %p.\n",
            iface, rect, brush, stroke_width, stroke_style);

    ID2D1RenderTarget_DrawRoundedRectangle(render_target->dxgi_target, rect, brush, stroke_width, stroke_style);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_FillRoundedRectangle(ID2D1HwndRenderTarget *iface,
        const D2D1_ROUNDED_RECT *rect, ID2D1Brush *brush)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, rect %p, brush %p.\n", iface, rect, brush);

    ID2D1RenderTarget_FillRoundedRectangle(render_target->dxgi_target, rect, brush);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_DrawEllipse(ID2D1HwndRenderTarget *iface,
        const D2D1_ELLIPSE *ellipse, ID2D1Brush *brush, float stroke_width, ID2D1StrokeStyle *stroke_style)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, ellipse %p, brush %p, stroke_width %.8e, stroke_style %p.\n",
            iface, ellipse, brush, stroke_width, stroke_style);

    ID2D1RenderTarget_DrawEllipse(render_target->dxgi_target, ellipse, brush, stroke_width, stroke_style);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_FillEllipse(ID2D1HwndRenderTarget *iface,
        const D2D1_ELLIPSE *ellipse, ID2D1Brush *brush)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, ellipse %p, brush %p.\n", iface, ellipse, brush);

    ID2D1RenderTarget_FillEllipse(render_target->dxgi_target, ellipse, brush);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_DrawGeometry(ID2D1HwndRenderTarget *iface,
        ID2D1Geometry *geometry, ID2D1Brush *brush, float stroke_width, ID2D1StrokeStyle *stroke_style)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, geometry %p, brush %p, stroke_width %.8e, stroke_style %p.\n",
            iface, geometry, brush, stroke_width, stroke_style);

    ID2D1RenderTarget_DrawGeometry(render_target->dxgi_target, geometry, brush, stroke_width, stroke_style);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_FillGeometry(ID2D1HwndRenderTarget *iface,
        ID2D1Geometry *geometry, ID2D1Brush *brush, ID2D1Brush *opacity_brush)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, geometry %p, brush %p, opacity_brush %p.\n", iface, geometry, brush, opacity_brush);

    ID2D1RenderTarget_FillGeometry(render_target->dxgi_target, geometry, brush, opacity_brush);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_FillMesh(ID2D1HwndRenderTarget *iface,
        ID2D1Mesh *mesh, ID2D1Brush *brush)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, mesh %p, brush %p.\n", iface, mesh, brush);

    ID2D1RenderTarget_FillMesh(render_target->dxgi_target, mesh, brush);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_FillOpacityMask(ID2D1HwndRenderTarget *iface,
        ID2D1Bitmap *mask, ID2D1Brush *brush, D2D1_OPACITY_MASK_CONTENT content,
        const D2D1_RECT_F *dst_rect, const D2D1_RECT_F *src_rect)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, mask %p, brush %p, content %#x, dst_rect %s, src_rect %s.\n",
            iface, mask, brush, content, debug_d2d_rect_f(dst_rect), debug_d2d_rect_f(src_rect));

    ID2D1RenderTarget_FillOpacityMask(render_target->dxgi_target,
            mask, brush, content, dst_rect, src_rect);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_DrawBitmap(ID2D1HwndRenderTarget *iface,
        ID2D1Bitmap *bitmap, const D2D1_RECT_F *dst_rect, float opacity,
        D2D1_BITMAP_INTERPOLATION_MODE interpolation_mode, const D2D1_RECT_F *src_rect)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, bitmap %p, dst_rect %s, opacity %.8e, interpolation_mode %#x, src_rect %s.\n",
            iface, bitmap, debug_d2d_rect_f(dst_rect), opacity, interpolation_mode, debug_d2d_rect_f(src_rect));

    ID2D1RenderTarget_DrawBitmap(render_target->dxgi_target,
            bitmap, dst_rect, opacity, interpolation_mode, src_rect);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_DrawText(ID2D1HwndRenderTarget *iface,
        const WCHAR *string, UINT32 string_len, IDWriteTextFormat *text_format, const D2D1_RECT_F *layout_rect,
        ID2D1Brush *brush, D2D1_DRAW_TEXT_OPTIONS options, DWRITE_MEASURING_MODE measuring_mode)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, string %s, string_len %u, text_format %p, layout_rect %s, "
            "brush %p, options %#x, measuring_mode %#x.\n",
            iface, debugstr_wn(string, string_len), string_len, text_format, debug_d2d_rect_f(layout_rect),
            brush, options, measuring_mode);

    ID2D1RenderTarget_DrawText(render_target->dxgi_target, string, string_len,
            text_format, layout_rect, brush, options, measuring_mode);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_DrawTextLayout(ID2D1HwndRenderTarget *iface,
        D2D1_POINT_2F origin, IDWriteTextLayout *layout, ID2D1Brush *brush, D2D1_DRAW_TEXT_OPTIONS options)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, origin %s, layout %p, brush %p, options %#x.\n",
            iface, debug_d2d_point_2f(&origin), layout, brush, options);

    ID2D1RenderTarget_DrawTextLayout(render_target->dxgi_target, origin, layout, brush, options);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_DrawGlyphRun(ID2D1HwndRenderTarget *iface,
        D2D1_POINT_2F baseline_origin, const DWRITE_GLYPH_RUN *glyph_run, ID2D1Brush *brush,
        DWRITE_MEASURING_MODE measuring_mode)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, baseline_origin %s, glyph_run %p, brush %p, measuring_mode %#x.\n",
            iface, debug_d2d_point_2f(&baseline_origin), glyph_run, brush, measuring_mode);

    ID2D1RenderTarget_DrawGlyphRun(render_target->dxgi_target,
            baseline_origin, glyph_run, brush, measuring_mode);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_SetTransform(ID2D1HwndRenderTarget *iface,
        const D2D1_MATRIX_3X2_F *transform)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, transform %p.\n", iface, transform);

    ID2D1RenderTarget_SetTransform(render_target->dxgi_target, transform);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_GetTransform(ID2D1HwndRenderTarget *iface,
        D2D1_MATRIX_3X2_F *transform)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, transform %p.\n", iface, transform);

    ID2D1RenderTarget_GetTransform(render_target->dxgi_target, transform);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_SetAntialiasMode(ID2D1HwndRenderTarget *iface,
        D2D1_ANTIALIAS_MODE antialias_mode)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, antialias_mode %#x.\n", iface, antialias_mode);

    ID2D1RenderTarget_SetAntialiasMode(render_target->dxgi_target, antialias_mode);
}

static D2D1_ANTIALIAS_MODE STDMETHODCALLTYPE d2d_hwnd_render_target_GetAntialiasMode(ID2D1HwndRenderTarget *iface)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p.\n", iface);

    return ID2D1RenderTarget_GetAntialiasMode(render_target->dxgi_target);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_SetTextAntialiasMode(ID2D1HwndRenderTarget *iface,
        D2D1_TEXT_ANTIALIAS_MODE antialias_mode)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, antialias_mode %#x.\n", iface, antialias_mode);

    ID2D1RenderTarget_SetTextAntialiasMode(render_target->dxgi_target, antialias_mode);
}

static D2D1_TEXT_ANTIALIAS_MODE STDMETHODCALLTYPE d2d_hwnd_render_target_GetTextAntialiasMode(
        ID2D1HwndRenderTarget *iface)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p.\n", iface);

    return ID2D1RenderTarget_GetTextAntialiasMode(render_target->dxgi_target);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_SetTextRenderingParams(ID2D1HwndRenderTarget *iface,
        IDWriteRenderingParams *text_rendering_params)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, text_rendering_params %p.\n", iface, text_rendering_params);

    ID2D1RenderTarget_SetTextRenderingParams(render_target->dxgi_target, text_rendering_params);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_GetTextRenderingParams(ID2D1HwndRenderTarget *iface,
        IDWriteRenderingParams **text_rendering_params)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, text_rendering_params %p.\n", iface, text_rendering_params);

    ID2D1RenderTarget_GetTextRenderingParams(render_target->dxgi_target, text_rendering_params);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_SetTags(ID2D1HwndRenderTarget *iface, D2D1_TAG tag1, D2D1_TAG tag2)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, tag1 %s, tag2 %s.\n", iface, wine_dbgstr_longlong(tag1), wine_dbgstr_longlong(tag2));

    ID2D1RenderTarget_SetTags(render_target->dxgi_target, tag1, tag2);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_GetTags(ID2D1HwndRenderTarget *iface, D2D1_TAG *tag1,
        D2D1_TAG *tag2)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, tag1 %p, tag2 %p.\n", iface, tag1, tag2);

    ID2D1RenderTarget_GetTags(render_target->dxgi_target, tag1, tag2);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_PushLayer(ID2D1HwndRenderTarget *iface,
        const D2D1_LAYER_PARAMETERS *layer_parameters, ID2D1Layer *layer)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, layer_parameters %p, layer %p.\n", iface, layer_parameters, layer);

    ID2D1RenderTarget_PushLayer(render_target->dxgi_target, layer_parameters, layer);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_PopLayer(ID2D1HwndRenderTarget *iface)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p.\n", iface);

    ID2D1RenderTarget_PopLayer(render_target->dxgi_target);
}

static HRESULT STDMETHODCALLTYPE d2d_hwnd_render_target_Flush(ID2D1HwndRenderTarget *iface, D2D1_TAG *tag1,
        D2D1_TAG *tag2)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, tag1 %p, tag2 %p.\n", iface, tag1, tag2);

    return ID2D1RenderTarget_Flush(render_target->dxgi_target, tag1, tag2);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_SaveDrawingState(ID2D1HwndRenderTarget *iface,
        ID2D1DrawingStateBlock *state_block)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, state_block %p.\n", iface, state_block);

    ID2D1RenderTarget_SaveDrawingState(render_target->dxgi_target, state_block);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_RestoreDrawingState(ID2D1HwndRenderTarget *iface,
        ID2D1DrawingStateBlock *state_block)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, state_block %p.\n", iface, state_block);

    ID2D1RenderTarget_RestoreDrawingState(render_target->dxgi_target, state_block);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_PushAxisAlignedClip(ID2D1HwndRenderTarget *iface,
        const D2D1_RECT_F *clip_rect, D2D1_ANTIALIAS_MODE antialias_mode)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, clip_rect %s, antialias_mode %#x.\n", iface, debug_d2d_rect_f(clip_rect), antialias_mode);

    ID2D1RenderTarget_PushAxisAlignedClip(render_target->dxgi_target, clip_rect, antialias_mode);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_PopAxisAlignedClip(ID2D1HwndRenderTarget *iface)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p.\n", iface);

    ID2D1RenderTarget_PopAxisAlignedClip(render_target->dxgi_target);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_Clear(ID2D1HwndRenderTarget *iface, const D2D1_COLOR_F *color)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, color %p.\n", iface, color);

    ID2D1RenderTarget_Clear(render_target->dxgi_target, color);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_BeginDraw(ID2D1HwndRenderTarget *iface)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p.\n", iface);

    ID2D1RenderTarget_BeginDraw(render_target->dxgi_target);
}

static HRESULT STDMETHODCALLTYPE d2d_hwnd_render_target_EndDraw(ID2D1HwndRenderTarget *iface,
        D2D1_TAG *tag1, D2D1_TAG *tag2)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, tag1 %p, tag2 %p.\n", iface, tag1, tag2);

    return ID2D1RenderTarget_EndDraw(render_target->dxgi_target, tag1, tag2);
}

static D2D1_PIXEL_FORMAT * STDMETHODCALLTYPE d2d_hwnd_render_target_GetPixelFormat(ID2D1HwndRenderTarget *iface,
        D2D1_PIXEL_FORMAT *format)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, format %p.\n", iface, format);

    *format = ID2D1RenderTarget_GetPixelFormat(render_target->dxgi_target);
    return format;
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_SetDpi(ID2D1HwndRenderTarget *iface, float dpi_x, float dpi_y)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, dpi_x %.8e, dpi_y %.8e.\n", iface, dpi_x, dpi_y);

    ID2D1RenderTarget_SetDpi(render_target->dxgi_target, dpi_x, dpi_y);
}

static void STDMETHODCALLTYPE d2d_hwnd_render_target_GetDpi(ID2D1HwndRenderTarget *iface, float *dpi_x, float *dpi_y)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, dpi_x %p, dpi_y %p.\n", iface, dpi_x, dpi_y);

    ID2D1RenderTarget_GetDpi(render_target->dxgi_target, dpi_x, dpi_y);
}

static D2D1_SIZE_F * STDMETHODCALLTYPE d2d_hwnd_render_target_GetSize(ID2D1HwndRenderTarget *iface, D2D1_SIZE_F *size)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, size %p.\n", iface, size);

    *size = ID2D1RenderTarget_GetSize(render_target->dxgi_target);
    return size;
}

static D2D1_SIZE_U * STDMETHODCALLTYPE d2d_hwnd_render_target_GetPixelSize(ID2D1HwndRenderTarget *iface,
        D2D1_SIZE_U *pixel_size)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p, pixel_size %p.\n", iface, pixel_size);

    *pixel_size = ID2D1RenderTarget_GetPixelSize(render_target->dxgi_target);
    return pixel_size;
}

static UINT32 STDMETHODCALLTYPE d2d_hwnd_render_target_GetMaximumBitmapSize(ID2D1HwndRenderTarget *iface)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p.\n", iface);

    return ID2D1RenderTarget_GetMaximumBitmapSize(render_target->dxgi_target);
}

static BOOL STDMETHODCALLTYPE d2d_hwnd_render_target_IsSupported(ID2D1HwndRenderTarget *iface,
        const D2D1_RENDER_TARGET_PROPERTIES *desc)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);
    const D2D1_RENDER_TARGET_PROPERTIES *target_desc = &render_target->desc;
    D2D1_PIXEL_FORMAT pixel_format;

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (desc->type != D2D1_RENDER_TARGET_TYPE_DEFAULT
            && target_desc->type != desc->type)
    {
        return FALSE;
    }

    pixel_format = ID2D1RenderTarget_GetPixelFormat(render_target->dxgi_target);

    if (desc->pixelFormat.format != DXGI_FORMAT_UNKNOWN
            && pixel_format.format != desc->pixelFormat.format)
    {
        return FALSE;
    }

    if (desc->pixelFormat.alphaMode != D2D1_ALPHA_MODE_UNKNOWN
            && pixel_format.alphaMode != desc->pixelFormat.alphaMode)
    {
        return FALSE;
    }

    return (target_desc->usage & desc->usage) == desc->usage;
}

static D2D1_WINDOW_STATE STDMETHODCALLTYPE d2d_hwnd_render_target_CheckWindowState(ID2D1HwndRenderTarget *iface)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p.\n", iface);

    return IDXGISwapChain_Present(render_target->swapchain, render_target->sync_interval, DXGI_PRESENT_TEST) ==
            DXGI_STATUS_OCCLUDED ? D2D1_WINDOW_STATE_OCCLUDED : D2D1_WINDOW_STATE_NONE;
}

static HRESULT STDMETHODCALLTYPE d2d_hwnd_render_target_Resize(ID2D1HwndRenderTarget *iface, const D2D1_SIZE_U *size)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);
    IDXGISurface1 *dxgi_surface;
    ID2D1DeviceContext *context;
    ID2D1Bitmap1 *bitmap;
    HRESULT hr;

    TRACE("iface %p, width %u, height %u.\n", iface, size->width, size->height);

    ID2D1RenderTarget_QueryInterface(render_target->dxgi_target, &IID_ID2D1DeviceContext, (void **)&context);
    ID2D1DeviceContext_SetTarget(context, NULL);

    if (SUCCEEDED(hr = IDXGISwapChain_ResizeBuffers(render_target->swapchain, 1, size->width, size->height,
        DXGI_FORMAT_UNKNOWN, 0)))
    {
        if (FAILED(hr = IDXGISwapChain_GetBuffer(render_target->swapchain, 0, &IID_IDXGISurface1,
                (void **)&dxgi_surface)))
        {
            WARN("Failed to get buffer, hr %#lx.\n", hr);
            ID2D1DeviceContext_Release(context);
            return hr;
        }

        hr = ID2D1DeviceContext_CreateBitmapFromDxgiSurface(context, (IDXGISurface *)dxgi_surface, NULL, &bitmap);
        IDXGISurface1_Release(dxgi_surface);
        if (FAILED(hr))
        {
            WARN("Failed to create target bitmap, hr %#lx.\n", hr);
            ID2D1DeviceContext_Release(context);
            return hr;
        }

        ID2D1DeviceContext_SetTarget(context, (ID2D1Image *)bitmap);
        ID2D1Bitmap1_Release(bitmap);
    }

    ID2D1DeviceContext_Release(context);

    return hr;
}

static HWND STDMETHODCALLTYPE d2d_hwnd_render_target_GetHwnd(ID2D1HwndRenderTarget *iface)
{
    struct d2d_hwnd_render_target *render_target = impl_from_ID2D1HwndRenderTarget(iface);

    TRACE("iface %p.\n", iface);

    return render_target->hwnd;
}

static const struct ID2D1HwndRenderTargetVtbl d2d_hwnd_render_target_vtbl =
{
    d2d_hwnd_render_target_QueryInterface,
    d2d_hwnd_render_target_AddRef,
    d2d_hwnd_render_target_Release,
    d2d_hwnd_render_target_GetFactory,
    d2d_hwnd_render_target_CreateBitmap,
    d2d_hwnd_render_target_CreateBitmapFromWicBitmap,
    d2d_hwnd_render_target_CreateSharedBitmap,
    d2d_hwnd_render_target_CreateBitmapBrush,
    d2d_hwnd_render_target_CreateSolidColorBrush,
    d2d_hwnd_render_target_CreateGradientStopCollection,
    d2d_hwnd_render_target_CreateLinearGradientBrush,
    d2d_hwnd_render_target_CreateRadialGradientBrush,
    d2d_hwnd_render_target_CreateCompatibleRenderTarget,
    d2d_hwnd_render_target_CreateLayer,
    d2d_hwnd_render_target_CreateMesh,
    d2d_hwnd_render_target_DrawLine,
    d2d_hwnd_render_target_DrawRectangle,
    d2d_hwnd_render_target_FillRectangle,
    d2d_hwnd_render_target_DrawRoundedRectangle,
    d2d_hwnd_render_target_FillRoundedRectangle,
    d2d_hwnd_render_target_DrawEllipse,
    d2d_hwnd_render_target_FillEllipse,
    d2d_hwnd_render_target_DrawGeometry,
    d2d_hwnd_render_target_FillGeometry,
    d2d_hwnd_render_target_FillMesh,
    d2d_hwnd_render_target_FillOpacityMask,
    d2d_hwnd_render_target_DrawBitmap,
    d2d_hwnd_render_target_DrawText,
    d2d_hwnd_render_target_DrawTextLayout,
    d2d_hwnd_render_target_DrawGlyphRun,
    d2d_hwnd_render_target_SetTransform,
    d2d_hwnd_render_target_GetTransform,
    d2d_hwnd_render_target_SetAntialiasMode,
    d2d_hwnd_render_target_GetAntialiasMode,
    d2d_hwnd_render_target_SetTextAntialiasMode,
    d2d_hwnd_render_target_GetTextAntialiasMode,
    d2d_hwnd_render_target_SetTextRenderingParams,
    d2d_hwnd_render_target_GetTextRenderingParams,
    d2d_hwnd_render_target_SetTags,
    d2d_hwnd_render_target_GetTags,
    d2d_hwnd_render_target_PushLayer,
    d2d_hwnd_render_target_PopLayer,
    d2d_hwnd_render_target_Flush,
    d2d_hwnd_render_target_SaveDrawingState,
    d2d_hwnd_render_target_RestoreDrawingState,
    d2d_hwnd_render_target_PushAxisAlignedClip,
    d2d_hwnd_render_target_PopAxisAlignedClip,
    d2d_hwnd_render_target_Clear,
    d2d_hwnd_render_target_BeginDraw,
    d2d_hwnd_render_target_EndDraw,
    d2d_hwnd_render_target_GetPixelFormat,
    d2d_hwnd_render_target_SetDpi,
    d2d_hwnd_render_target_GetDpi,
    d2d_hwnd_render_target_GetSize,
    d2d_hwnd_render_target_GetPixelSize,
    d2d_hwnd_render_target_GetMaximumBitmapSize,
    d2d_hwnd_render_target_IsSupported,
    d2d_hwnd_render_target_CheckWindowState,
    d2d_hwnd_render_target_Resize,
    d2d_hwnd_render_target_GetHwnd
};

static const struct d2d_device_context_ops d2d_hwnd_render_target_ops =
{
    d2d_hwnd_render_target_present,
};

HRESULT d2d_hwnd_render_target_init(struct d2d_hwnd_render_target *render_target, ID2D1Factory1 *factory,
        ID3D10Device1 *d3d_device, const D2D1_RENDER_TARGET_PROPERTIES *desc,
        const D2D1_HWND_RENDER_TARGET_PROPERTIES *hwnd_rt_desc)
{
    D2D1_RENDER_TARGET_PROPERTIES dxgi_rt_desc;
    DXGI_SWAP_CHAIN_DESC swapchain_desc;
    IDXGIAdapter *dxgi_adapter;
    IDXGIFactory *dxgi_factory;
    IDXGISurface *dxgi_surface;
    IDXGIDevice *dxgi_device;
    ID2D1Device *device;
    HRESULT hr;

    if (!IsWindow(hwnd_rt_desc->hwnd))
        return HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE);

    render_target->ID2D1HwndRenderTarget_iface.lpVtbl = &d2d_hwnd_render_target_vtbl;
    render_target->hwnd = hwnd_rt_desc->hwnd;
    render_target->sync_interval = hwnd_rt_desc->presentOptions & D2D1_PRESENT_OPTIONS_IMMEDIATELY ? 0 : 1;

    if (FAILED(hr = ID3D10Device1_QueryInterface(d3d_device, &IID_IDXGIDevice, (void **)&dxgi_device)))
    {
        WARN("Failed to get IDXGIDevice interface, hr %#lx.\n", hr);
        return hr;
    }

    hr = IDXGIDevice_GetAdapter(dxgi_device, &dxgi_adapter);
    IDXGIDevice_Release(dxgi_device);
    if (FAILED(hr))
    {
        WARN("Failed to get IDXGIAdapter interface, hr %#lx.\n", hr);
        return hr;
    }

    hr = IDXGIAdapter_GetParent(dxgi_adapter, &IID_IDXGIFactory, (void **)&dxgi_factory);
    IDXGIAdapter_Release(dxgi_adapter);
    if (FAILED(hr))
    {
        WARN("Failed to get IDXGIFactory interface, hr %#lx.\n", hr);
        return hr;
    }

    dxgi_rt_desc = *desc;
    if (dxgi_rt_desc.dpiX == 0.0f && dxgi_rt_desc.dpiY == 0.0f)
        ID2D1Factory1_GetDesktopDpi(factory, &dxgi_rt_desc.dpiX, &dxgi_rt_desc.dpiY);

    if (dxgi_rt_desc.pixelFormat.format == DXGI_FORMAT_UNKNOWN)
        dxgi_rt_desc.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;

    if (dxgi_rt_desc.pixelFormat.alphaMode == D2D1_ALPHA_MODE_UNKNOWN)
        dxgi_rt_desc.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;

    if (dxgi_rt_desc.pixelFormat.alphaMode == D2D1_ALPHA_MODE_STRAIGHT)
    {
        IDXGIFactory_Release(dxgi_factory);
        WARN("Alpha mode %u is not supported.\n", dxgi_rt_desc.pixelFormat.alphaMode);
        return D2DERR_UNSUPPORTED_PIXEL_FORMAT;
    }

    render_target->desc = dxgi_rt_desc;
    /* FIXME: should be resolved to either HW or SW type. */
    if (render_target->desc.type == D2D1_RENDER_TARGET_TYPE_DEFAULT)
        render_target->desc.type = D2D1_RENDER_TARGET_TYPE_HARDWARE;

    swapchain_desc.BufferDesc.Width = hwnd_rt_desc->pixelSize.width;
    swapchain_desc.BufferDesc.Height = hwnd_rt_desc->pixelSize.height;
    swapchain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swapchain_desc.BufferDesc.RefreshRate.Denominator = 1;
    swapchain_desc.BufferDesc.Format = dxgi_rt_desc.pixelFormat.format;
    swapchain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapchain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapchain_desc.SampleDesc.Count = 1;
    swapchain_desc.SampleDesc.Quality = 0;
    swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchain_desc.BufferCount = 1;
    swapchain_desc.OutputWindow = hwnd_rt_desc->hwnd;
    swapchain_desc.Windowed = TRUE;
    swapchain_desc.SwapEffect = hwnd_rt_desc->presentOptions & D2D1_PRESENT_OPTIONS_RETAIN_CONTENTS ?
        DXGI_SWAP_EFFECT_SEQUENTIAL : DXGI_SWAP_EFFECT_DISCARD;
    swapchain_desc.Flags = 0;
    if (desc->usage & D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE)
        swapchain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE;

    hr = IDXGIFactory_CreateSwapChain(dxgi_factory, (IUnknown *)d3d_device, &swapchain_desc, &render_target->swapchain);
    IDXGIFactory_Release(dxgi_factory);
    if (FAILED(hr))
    {
        WARN("Failed to create a swapchain, hr %#lx.\n", hr);
        return hr;
    }

    if (FAILED(hr = IDXGISwapChain_GetBuffer(render_target->swapchain, 0, &IID_IDXGISurface, (void **)&dxgi_surface)))
    {
        WARN("Failed to get buffer, hr %#lx.\n", hr);
        IDXGISwapChain_Release(render_target->swapchain);
        return hr;
    }

    render_target->ID2D1HwndRenderTarget_iface.lpVtbl = &d2d_hwnd_render_target_vtbl;

    if (FAILED(hr = IDXGISurface_GetDevice(dxgi_surface, &IID_IDXGIDevice, (void **)&dxgi_device)))
    {
        WARN("Failed to get DXGI device, hr %#lx.\n", hr);
        IDXGISurface_Release(dxgi_surface);
        IDXGISwapChain_Release(render_target->swapchain);
        return hr;
    }

    hr = d2d_factory_create_device(factory, dxgi_device, false, &IID_ID2D1Device, (void **)&device);
    IDXGIDevice_Release(dxgi_device);
    if (FAILED(hr))
    {
        WARN("Failed to create D2D device, hr %#lx.\n", hr);
        IDXGISurface_Release(dxgi_surface);
        IDXGISwapChain_Release(render_target->swapchain);
        return hr;
    }

    hr = d2d_d3d_create_render_target(unsafe_impl_from_ID2D1Device((ID2D1Device1 *)device), dxgi_surface,
            (IUnknown *)&render_target->ID2D1HwndRenderTarget_iface, &d2d_hwnd_render_target_ops,
            &dxgi_rt_desc, (void **)&render_target->dxgi_inner);
    IDXGISurface_Release(dxgi_surface);
    ID2D1Device_Release(device);
    if (FAILED(hr))
    {
        WARN("Failed to create DXGI surface render target, hr %#lx.\n", hr);
        IDXGISwapChain_Release(render_target->swapchain);
        return hr;
    }

    if (FAILED(hr = IUnknown_QueryInterface(render_target->dxgi_inner,
            &IID_ID2D1RenderTarget, (void **)&render_target->dxgi_target)))
    {
        WARN("Failed to retrieve ID2D1RenderTarget interface, hr %#lx.\n", hr);
        IUnknown_Release(render_target->dxgi_inner);
        IDXGISwapChain_Release(render_target->swapchain);
        return hr;
    }

    return S_OK;
}
