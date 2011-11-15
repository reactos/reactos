/* IWineD3DClipper implementation
 *
 * Copyright 2000 (c) Marcus Meissner
 * Copyright 2000 (c) TransGaming Technologies Inc.
 * Copyright 2006 (c) Stefan Dösinger
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
#include <stdio.h>
#ifdef HAVE_FLOAT_H
# include <float.h>
#endif
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

ULONG CDECL wined3d_clipper_incref(struct wined3d_clipper *clipper)
{
    ULONG refcount = InterlockedIncrement(&clipper->ref);

    TRACE("%p increasing refcount to %u.\n", clipper, refcount);

    return refcount;
}

ULONG CDECL wined3d_clipper_decref(struct wined3d_clipper *clipper)
{
    ULONG refcount = InterlockedDecrement(&clipper->ref);

    TRACE("%p decreasing refcount to %u.\n", clipper, refcount);

    if (!refcount)
        HeapFree(GetProcessHeap(), 0, clipper);

    return refcount;
}

HRESULT CDECL wined3d_clipper_set_window(struct wined3d_clipper *clipper, DWORD flags, HWND window)
{
    TRACE("clipper %p, flags %#x, window %p.\n", clipper, flags, window);

    if (flags)
    {
        FIXME("flags %#x, not supported.\n", flags);
        return WINED3DERR_INVALIDCALL;
    }

    clipper->hWnd = window;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_clipper_get_clip_list(const struct wined3d_clipper *clipper, const RECT *rect,
        RGNDATA *clip_list, DWORD *clip_list_size)
{
    TRACE("clipper %p, rect %s, clip_list %p, clip_list_size %p.\n",
            clipper, wine_dbgstr_rect(rect), clip_list, clip_list_size);

    if (clipper->hWnd)
    {
        HDC hDC = GetDCEx(clipper->hWnd, NULL, DCX_WINDOW);
        if (hDC)
        {
            HRGN hRgn = CreateRectRgn(0,0,0,0);
            if (GetRandomRgn(hDC, hRgn, SYSRGN))
            {
                if (GetVersion() & 0x80000000)
                {
                    /* map region to screen coordinates */
                    POINT org;
                    GetDCOrgEx(hDC, &org);
                    OffsetRgn(hRgn, org.x, org.y);
                }
                if (rect)
                {
                    HRGN hRgnClip = CreateRectRgn(rect->left, rect->top,
                            rect->right, rect->bottom);
                    CombineRgn(hRgn, hRgn, hRgnClip, RGN_AND);
                    DeleteObject(hRgnClip);
                }
                *clip_list_size = GetRegionData(hRgn, *clip_list_size, clip_list);
            }
            DeleteObject(hRgn);
            ReleaseDC(clipper->hWnd, hDC);
        }
        return WINED3D_OK;
    }
    else
    {
        static unsigned int once;

        if (!once++)
            FIXME("clipper %p, rect %s, clip_list %p, clip_list_size %p stub!\n",
                    clipper, wine_dbgstr_rect(rect), clip_list, clip_list_size);

        if (clip_list_size)
            *clip_list_size = 0;

        return WINEDDERR_NOCLIPLIST;
    }
}

HRESULT CDECL wined3d_clipper_set_clip_list(struct wined3d_clipper *clipper, const RGNDATA *region, DWORD flags)
{
    static unsigned int once;

    if (!once++ || !region)
        FIXME("clipper %p, region %p, flags %#x stub!\n", clipper, region, flags);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_clipper_get_window(const struct wined3d_clipper *clipper, HWND *window)
{
    TRACE("clipper %p, window %p.\n", clipper, window);

    *window = clipper->hWnd;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_clipper_is_clip_list_changed(const struct wined3d_clipper *clipper, BOOL *changed)
{
    FIXME("clipper %p, changed %p stub!\n", clipper, changed);

    /* XXX What is safest? */
    *changed = FALSE;

    return WINED3D_OK;
}

struct wined3d_clipper * CDECL wined3d_clipper_create(void)
{
    struct wined3d_clipper *clipper;

    TRACE("\n");

    clipper = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*clipper));
    if (!clipper)
    {
        ERR("Out of memory when trying to allocate a WineD3D Clipper\n");
        return NULL;
    }

    wined3d_clipper_incref(clipper);

    return clipper;
}
