/*
 * uxtheme Double-buffered Drawing API
 *
 * Copyright (C) 2008 Reece H. Dunn
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

#include "uxthemep.h"
#include <wine/heap.h>

struct paintbuffer
{
    HDC targetdc;
    HDC memorydc;
    HBITMAP bitmap;
    RECT rect;
    void *bits;
};

static void free_paintbuffer(struct paintbuffer *buffer)
{
    DeleteObject(buffer->bitmap);
    DeleteDC(buffer->memorydc);
    heap_free(buffer);
}

static struct paintbuffer *get_buffer_obj(HPAINTBUFFER handle)
{
    return handle;
}

/***********************************************************************
 *      BufferedPaintInit                                  (UXTHEME.@)
 */
HRESULT WINAPI BufferedPaintInit(VOID)
{
    FIXME("Stub ()\n");
    return S_OK;
}

/***********************************************************************
 *      BufferedPaintUnInit                                (UXTHEME.@)
 */
HRESULT WINAPI BufferedPaintUnInit(VOID)
{
    FIXME("Stub ()\n");
    return S_OK;
}

/***********************************************************************
 *      BeginBufferedPaint                                 (UXTHEME.@)
 */
HPAINTBUFFER WINAPI BeginBufferedPaint(HDC targetdc, const RECT *rect,
        BP_BUFFERFORMAT format, BP_PAINTPARAMS *params, HDC *retdc)
{
#if (defined(_MSC_VER))
    RGBQUAD rgb;
    //TODO: Fix this for MSVC
    __debugbreak();
    char bmibuf[sizeof(rgb) * 256];
#else
    char bmibuf[FIELD_OFFSET(BITMAPINFO, bmiColors[256])];
#endif
    BITMAPINFO *bmi = (BITMAPINFO *)bmibuf;
    struct paintbuffer *buffer;

    TRACE("(%p %s %d %p %p)\n", targetdc, wine_dbgstr_rect(rect), format,
          params, retdc);

    if (retdc)
        *retdc = NULL;

    if (!targetdc || IsRectEmpty(rect))
        return NULL;

    if (params)
        FIXME("painting parameters are ignored\n");

    buffer = heap_alloc(sizeof(*buffer));
    buffer->targetdc = targetdc;
    buffer->rect = *rect;
    buffer->memorydc = CreateCompatibleDC(targetdc);

    switch (format)
    {
    case BPBF_COMPATIBLEBITMAP:
        buffer->bitmap = CreateCompatibleBitmap(targetdc, rect->right - rect->left, rect->bottom - rect->top);
        buffer->bits = NULL;
        break;
    case BPBF_DIB:
    case BPBF_TOPDOWNDIB:
    case BPBF_TOPDOWNMONODIB:
        /* create DIB section */
        memset(bmi, 0, sizeof(bmibuf));
        bmi->bmiHeader.biSize = sizeof(bmi->bmiHeader);
        bmi->bmiHeader.biHeight = format == BPBF_DIB ? rect->bottom - rect->top :
                -(rect->bottom - rect->top);
        bmi->bmiHeader.biWidth = rect->right - rect->left;
        bmi->bmiHeader.biBitCount = format == BPBF_TOPDOWNMONODIB ? 1 : 32;
        bmi->bmiHeader.biPlanes = 1;
        bmi->bmiHeader.biCompression = BI_RGB;
        buffer->bitmap = CreateDIBSection(buffer->memorydc, bmi, DIB_RGB_COLORS, &buffer->bits, NULL, 0);
        break;
    default:
        WARN("Unknown buffer format %d\n", format);
        buffer->bitmap = NULL;
        free_paintbuffer(buffer);
        return NULL;
    }

    if (!buffer->bitmap)
    {
        WARN("Failed to create buffer bitmap\n");
        free_paintbuffer(buffer);
        return NULL;
    }

    SetWindowOrgEx(buffer->memorydc, rect->left, rect->top, NULL);
    IntersectClipRect(buffer->memorydc, rect->left, rect->top, rect->right, rect->bottom);
    DeleteObject(SelectObject(buffer->memorydc, buffer->bitmap));

    *retdc = buffer->memorydc;

    return (HPAINTBUFFER)buffer;
}


/***********************************************************************
 *      EndBufferedPaint                                   (UXTHEME.@)
 */
HRESULT WINAPI EndBufferedPaint(HPAINTBUFFER bufferhandle, BOOL update)
{
    struct paintbuffer *buffer = get_buffer_obj(bufferhandle);

    TRACE("(%p %d)\n", bufferhandle, update);

    if (!buffer)
        return E_INVALIDARG;

    if (update)
    {
        if (!BitBlt(buffer->targetdc, buffer->rect.left, buffer->rect.top,
                buffer->rect.right - buffer->rect.left, buffer->rect.bottom - buffer->rect.top,
                buffer->memorydc, buffer->rect.left, buffer->rect.top, SRCCOPY))
        {
            WARN("BitBlt() failed\n");
        }
    }

    free_paintbuffer(buffer);
    return S_OK;
}

/***********************************************************************
 *      BufferedPaintClear                                 (UXTHEME.@)
 */
HRESULT WINAPI BufferedPaintClear(HPAINTBUFFER hBufferedPaint, const RECT *prc)
{
    FIXME("Stub (%p %p)\n", hBufferedPaint, prc);
    return E_NOTIMPL;
}

/***********************************************************************
 *      BufferedPaintSetAlpha                              (UXTHEME.@)
 */
HRESULT WINAPI BufferedPaintSetAlpha(HPAINTBUFFER hBufferedPaint, const RECT *prc, BYTE alpha)
{
    FIXME("Stub (%p %p %u)\n", hBufferedPaint, prc, alpha);
    return E_NOTIMPL;
}

/***********************************************************************
 *      GetBufferedPaintBits                               (UXTHEME.@)
 */
HRESULT WINAPI GetBufferedPaintBits(HPAINTBUFFER bufferhandle, RGBQUAD **bits, int *width)
{
    struct paintbuffer *buffer = get_buffer_obj(bufferhandle);

    TRACE("(%p %p %p)\n", buffer, bits, width);

    if (!bits || !width)
        return E_POINTER;

    if (!buffer || !buffer->bits)
        return E_FAIL;

    *bits = buffer->bits;
    *width = buffer->rect.right - buffer->rect.left;

    return S_OK;
}

/***********************************************************************
 *      GetBufferedPaintDC                                 (UXTHEME.@)
 */
HDC WINAPI GetBufferedPaintDC(HPAINTBUFFER bufferhandle)
{
    struct paintbuffer *buffer = get_buffer_obj(bufferhandle);

    TRACE("(%p)\n", buffer);

    return buffer ? buffer->memorydc : NULL;
}

/***********************************************************************
 *      GetBufferedPaintTargetDC                           (UXTHEME.@)
 */
HDC WINAPI GetBufferedPaintTargetDC(HPAINTBUFFER bufferhandle)
{
    struct paintbuffer *buffer = get_buffer_obj(bufferhandle);

    TRACE("(%p)\n", buffer);

    return buffer ? buffer->targetdc : NULL;
}

/***********************************************************************
 *      GetBufferedPaintTargetRect                         (UXTHEME.@)
 */
HRESULT WINAPI GetBufferedPaintTargetRect(HPAINTBUFFER bufferhandle, RECT *rect)
{
    struct paintbuffer *buffer = get_buffer_obj(bufferhandle);

    TRACE("(%p %p)\n", buffer, rect);

    if (!rect)
        return E_POINTER;

    if (!buffer)
        return E_FAIL;

    *rect = buffer->rect;
    return S_OK;
}

/***********************************************************************
 *      BeginBufferedAnimation                             (UXTHEME.@)
 */
HANIMATIONBUFFER WINAPI BeginBufferedAnimation(HWND hwnd, HDC hdcTarget, const RECT *rcTarget,
                                               BP_BUFFERFORMAT dwFormat, BP_PAINTPARAMS *pPaintParams,
                                               BP_ANIMATIONPARAMS *pAnimationParams, HDC *phdcFrom,
                                               HDC *phdcTo)
{
    FIXME("Stub (%p %p %p %u %p %p %p %p)\n", hwnd, hdcTarget, rcTarget, dwFormat,
          pPaintParams, pAnimationParams, phdcFrom, phdcTo);

    return NULL;
}

/***********************************************************************
 *      BufferedPaintRenderAnimation                       (UXTHEME.@)
 */
BOOL WINAPI BufferedPaintRenderAnimation(HWND hwnd, HDC hdcTarget)
{
    FIXME("Stub (%p %p)\n", hwnd, hdcTarget);

    return FALSE;
}

/***********************************************************************
 *      BufferedPaintStopAllAnimations                     (UXTHEME.@)
 */
HRESULT WINAPI BufferedPaintStopAllAnimations(HWND hwnd)
{
    FIXME("Stub (%p)\n", hwnd);

    return E_NOTIMPL;
}

/***********************************************************************
 *      EndBufferedAnimation                               (UXTHEME.@)
 */
HRESULT WINAPI EndBufferedAnimation(HANIMATIONBUFFER hbpAnimation, BOOL fUpdateTarget)
{
    FIXME("Stub (%p %u)\n", hbpAnimation, fUpdateTarget);

    return E_NOTIMPL;
}
