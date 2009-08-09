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

#include "config.h"

#include <stdlib.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "vfwmsgs.h"
#include "uxtheme.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uxtheme);

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
