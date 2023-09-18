/* DirectDrawClipper implementation
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

#include "ddraw_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

static const struct IDirectDrawClipperVtbl ddraw_clipper_vtbl;

static inline struct ddraw_clipper *impl_from_IDirectDrawClipper(IDirectDrawClipper *iface)
{
    return CONTAINING_RECORD(iface, struct ddraw_clipper, IDirectDrawClipper_iface);
}

BOOL ddraw_clipper_is_valid(const struct ddraw_clipper *clipper)
{
    /* Native is very lenient when you invoke the clipper methods with a clipper pointer that
     * points to something that is either not accessible or not a clipper, or if you break
     * a clipper after assigning it to a surface. Deus Ex: Goty depends on this. */

    if (IsBadReadPtr(clipper, sizeof(*clipper)))
    {
        WARN("The application gave us an invalid clipper pointer %p.\n", clipper);
        return FALSE;
    }
    if (clipper->IDirectDrawClipper_iface.lpVtbl != &ddraw_clipper_vtbl)
    {
        WARN("The clipper vtable is modified: %p, expected %p.\n",
                clipper->IDirectDrawClipper_iface.lpVtbl, &ddraw_clipper_vtbl);
        return FALSE;
    }

    return TRUE;
}

static HRESULT WINAPI ddraw_clipper_QueryInterface(IDirectDrawClipper *iface, REFIID iid, void **object)
{
    struct ddraw_clipper *clipper = impl_from_IDirectDrawClipper(iface);

    TRACE("iface %p, iid %s, object %p.\n", iface, debugstr_guid(iid), object);

    if (!ddraw_clipper_is_valid(clipper))
        return DDERR_INVALIDPARAMS;

    if (IsEqualGUID(&IID_IDirectDrawClipper, iid)
            || IsEqualGUID(&IID_IUnknown, iid))
    {
        IDirectDrawClipper_AddRef(&clipper->IDirectDrawClipper_iface);
        *object = &clipper->IDirectDrawClipper_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *object = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI ddraw_clipper_AddRef(IDirectDrawClipper *iface)
{
    struct ddraw_clipper *clipper = impl_from_IDirectDrawClipper(iface);
    ULONG refcount;

    if (!ddraw_clipper_is_valid(clipper))
    {
        WARN("Invalid clipper, returning 0.\n");
        return 0;
    }

    refcount = InterlockedIncrement(&clipper->ref);
    TRACE("%p increasing refcount to %u.\n", clipper, refcount);
    return refcount;
}

static ULONG WINAPI ddraw_clipper_Release(IDirectDrawClipper *iface)
{
    struct ddraw_clipper *clipper = impl_from_IDirectDrawClipper(iface);
    ULONG refcount;

    if (!ddraw_clipper_is_valid(clipper))
    {
        WARN("Invalid clipper, returning 0.\n");
        return 0;
    }

    refcount = InterlockedDecrement(&clipper->ref);

    TRACE("%p decreasing refcount to %u.\n", clipper, refcount);

    if (!refcount)
    {
        if (clipper->region)
            DeleteObject(clipper->region);
        clipper->IDirectDrawClipper_iface.lpVtbl = NULL; /* Should help with detecting freed clippers. */
        heap_free(clipper);
    }

    return refcount;
}

static HRESULT WINAPI ddraw_clipper_SetHWnd(IDirectDrawClipper *iface, DWORD flags, HWND window)
{
    struct ddraw_clipper *clipper = impl_from_IDirectDrawClipper(iface);

    TRACE("iface %p, flags %#x, window %p.\n", iface, flags, window);

    if (!ddraw_clipper_is_valid(clipper))
        return DDERR_INVALIDPARAMS;

    if (flags)
    {
        FIXME("flags %#x, not supported.\n", flags);
        return DDERR_INVALIDPARAMS;
    }

    wined3d_mutex_lock();
    clipper->window = window;
    wined3d_mutex_unlock();

    return DD_OK;
}

static HRGN get_window_region(HWND window)
{
    POINT origin;
    HRGN rgn;
    HDC dc;

    if (!(dc = GetDC(window)))
    {
        WARN("Failed to get dc.\n");
        return NULL;
    }

    if (!(rgn = CreateRectRgn(0, 0, 0, 0)))
    {
        ERR("Failed to create region.\n");
        ReleaseDC(window, dc);
        return NULL;
    }

    if (GetRandomRgn(dc, rgn, SYSRGN) != 1)
    {
        ERR("Failed to get window region.\n");
        DeleteObject(rgn);
        ReleaseDC(window, dc);
        return NULL;
    }

    if (GetVersion() & 0x80000000)
    {
        GetDCOrgEx(dc, &origin);
        OffsetRgn(rgn, origin.x, origin.y);
    }

    ReleaseDC(window, dc);
    return rgn;
}

/*****************************************************************************
 * IDirectDrawClipper::GetClipList
 *
 * Retrieve a copy of the clip list
 *
 * Arguments:
 *  rect: Rectangle to be used to clip the clip list or NULL for the
 *      entire clip list.
 *  clip_list: structure for the resulting copy of the clip list.
 *      If NULL, fills Size up to the number of bytes necessary to hold
 *      the entire clip.
 *  clip_list_size: Size of resulting clip list; size of the buffer at clip_list
 *      or, if clip_list is NULL, receives the required size of the buffer
 *      in bytes.
 *
 * RETURNS
 *  Either DD_OK or DDERR_*
 ************************************************************************/
static HRESULT WINAPI ddraw_clipper_GetClipList(IDirectDrawClipper *iface, RECT *rect,
        RGNDATA *clip_list, DWORD *clip_list_size)
{
    struct ddraw_clipper *clipper = impl_from_IDirectDrawClipper(iface);
    HRGN region;

    TRACE("iface %p, rect %s, clip_list %p, clip_list_size %p.\n",
            iface, wine_dbgstr_rect(rect), clip_list, clip_list_size);

    if (!ddraw_clipper_is_valid(clipper))
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();

    if (clipper->window)
    {
        if (!(region = get_window_region(clipper->window)))
        {
            wined3d_mutex_unlock();
            WARN("Failed to get window region.\n");
            return E_FAIL;
        }
    }
    else
    {
        if (!(region = clipper->region))
        {
            wined3d_mutex_unlock();
            WARN("No clip list set.\n");
            return DDERR_NOCLIPLIST;
        }
    }

    if (rect)
    {
        HRGN clip_region;

        if (!(clip_region = CreateRectRgnIndirect(rect)))
        {
            wined3d_mutex_unlock();
            ERR("Failed to create region.\n");
            if (clipper->window)
                DeleteObject(region);
            return E_FAIL;
        }

        if (CombineRgn(clip_region, region, clip_region, RGN_AND) == ERROR)
        {
            wined3d_mutex_unlock();
            ERR("Failed to combine regions.\n");
            DeleteObject(clip_region);
            if (clipper->window)
                DeleteObject(region);
            return E_FAIL;
        }

        if (clipper->window)
            DeleteObject(region);
        region = clip_region;
    }

    *clip_list_size = GetRegionData(region, *clip_list_size, clip_list);
    if (rect || clipper->window)
        DeleteObject(region);

    wined3d_mutex_unlock();
    return DD_OK;
}

/*****************************************************************************
 * IDirectDrawClipper::SetClipList
 *
 * Sets or deletes (if region is NULL) the clip list
 *
 * This implementation is a stub and returns DD_OK always to make the app
 * happy.
 *
 * PARAMS
 *  region  Pointer to a LRGNDATA structure or NULL
 *  flags   not used, must be 0
 * RETURNS
 *  Either DD_OK or DDERR_*
 *****************************************************************************/
static HRESULT WINAPI ddraw_clipper_SetClipList(IDirectDrawClipper *iface, RGNDATA *region, DWORD flags)
{
    struct ddraw_clipper *clipper = impl_from_IDirectDrawClipper(iface);

    TRACE("iface %p, region %p, flags %#x.\n", iface, region, flags);

    if (!ddraw_clipper_is_valid(clipper))
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();

    if (clipper->window)
    {
        wined3d_mutex_unlock();
        return DDERR_CLIPPERISUSINGHWND;
    }

    if (clipper->region)
        DeleteObject(clipper->region);
    if (!region)
        clipper->region = NULL;
    else if (!(clipper->region = ExtCreateRegion(NULL, 0, region)))
    {
        wined3d_mutex_unlock();
        ERR("Failed to create region.\n");
        return E_FAIL;
    }

    wined3d_mutex_unlock();

    return DD_OK;
}

static HRESULT WINAPI ddraw_clipper_GetHWnd(IDirectDrawClipper *iface, HWND *window)
{
    struct ddraw_clipper *clipper = impl_from_IDirectDrawClipper(iface);

    TRACE("iface %p, window %p.\n", iface, window);

    if (!ddraw_clipper_is_valid(clipper))
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    *window = clipper->window;
    wined3d_mutex_unlock();

    return DD_OK;
}

static HRESULT WINAPI ddraw_clipper_Initialize(IDirectDrawClipper *iface,
        IDirectDraw *ddraw, DWORD flags)
{
    struct ddraw_clipper *clipper = impl_from_IDirectDrawClipper(iface);

    TRACE("iface %p, ddraw %p, flags %#x.\n", iface, ddraw, flags);

    if (!ddraw_clipper_is_valid(clipper))
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    if (clipper->initialized)
    {
        wined3d_mutex_unlock();
        return DDERR_ALREADYINITIALIZED;
    }

    clipper->initialized = TRUE;
    wined3d_mutex_unlock();

    return DD_OK;
}

static HRESULT WINAPI ddraw_clipper_IsClipListChanged(IDirectDrawClipper *iface, BOOL *changed)
{
    struct ddraw_clipper *clipper = impl_from_IDirectDrawClipper(iface);

    FIXME("iface %p, changed %p stub!\n", iface, changed);

    if (!ddraw_clipper_is_valid(clipper))
        return DDERR_INVALIDPARAMS;

    /* XXX What is safest? */
    *changed = FALSE;

    return DD_OK;
}

static const struct IDirectDrawClipperVtbl ddraw_clipper_vtbl =
{
    ddraw_clipper_QueryInterface,
    ddraw_clipper_AddRef,
    ddraw_clipper_Release,
    ddraw_clipper_GetClipList,
    ddraw_clipper_GetHWnd,
    ddraw_clipper_Initialize,
    ddraw_clipper_IsClipListChanged,
    ddraw_clipper_SetClipList,
    ddraw_clipper_SetHWnd,
};

HRESULT ddraw_clipper_init(struct ddraw_clipper *clipper)
{
    clipper->IDirectDrawClipper_iface.lpVtbl = &ddraw_clipper_vtbl;
    clipper->ref = 1;

    return DD_OK;
}

struct ddraw_clipper *unsafe_impl_from_IDirectDrawClipper(IDirectDrawClipper *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &ddraw_clipper_vtbl);

    return impl_from_IDirectDrawClipper(iface);
}
