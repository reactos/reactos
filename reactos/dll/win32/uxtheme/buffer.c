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
HPAINTBUFFER WINAPI BeginBufferedPaint(HDC hdcTarget,
                                       const RECT * prcTarget,
                                       BP_BUFFERFORMAT dwFormat,
                                       BP_PAINTPARAMS *pPaintParams,
                                       HDC *phdc)
{
    static int i;

    TRACE("Stub (%p %p %d %p %p)\n", hdcTarget, prcTarget, dwFormat,
          pPaintParams, phdc);

    if (!i++)
        FIXME("Stub (%p %p %d %p %p)\n", hdcTarget, prcTarget, dwFormat,
              pPaintParams, phdc);
    return NULL;
}


/***********************************************************************
 *      EndBufferedPaint                                   (UXTHEME.@)
 */
HRESULT WINAPI EndBufferedPaint(HPAINTBUFFER hPaintBuffer, BOOL fUpdateTarget)
{
    FIXME("Stub (%p %d)\n", hPaintBuffer, fUpdateTarget);
    return S_OK;
}

#ifndef __REACTOS__

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
HRESULT WINAPI GetBufferedPaintBits(HPAINTBUFFER hBufferedPaint, RGBQUAD **ppbBuffer,
                                    int *pcxRow)
{
    FIXME("Stub (%p %p %p)\n", hBufferedPaint, ppbBuffer, pcxRow);
    return E_NOTIMPL;
}

/***********************************************************************
 *      GetBufferedPaintDC                                 (UXTHEME.@)
 */
HDC WINAPI GetBufferedPaintDC(HPAINTBUFFER hBufferedPaint)
{
    FIXME("Stub (%p)\n", hBufferedPaint);
    return NULL;
}

/***********************************************************************
 *      GetBufferedPaintTargetDC                           (UXTHEME.@)
 */
HDC WINAPI GetBufferedPaintTargetDC(HPAINTBUFFER hBufferedPaint)
{
    FIXME("Stub (%p)\n", hBufferedPaint);
    return NULL;
}

/***********************************************************************
 *      GetBufferedPaintTargetRect                         (UXTHEME.@)
 */
HRESULT WINAPI GetBufferedPaintTargetRect(HPAINTBUFFER hBufferedPaint, RECT *prc)
{
    FIXME("Stub (%p %p)\n", hBufferedPaint, prc);
    return E_NOTIMPL;
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

#endif /* __REACTOS__ */
