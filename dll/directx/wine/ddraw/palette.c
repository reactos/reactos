/*
 * Copyright 2006 Stefan Dösinger
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

/*****************************************************************************
 * IDirectDrawPalette::QueryInterface
 *
 * A usual QueryInterface implementation. Can only Query IUnknown and
 * IDirectDrawPalette
 *
 * Params:
 *  refiid: The interface id queried for
 *  obj: Address to return the interface pointer at
 *
 * Returns:
 *  S_OK on success
 *  E_NOINTERFACE if the requested interface wasn't found
 *****************************************************************************/
static HRESULT WINAPI ddraw_palette_QueryInterface(IDirectDrawPalette *iface, REFIID refiid, void **obj)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(refiid), obj);

    if (IsEqualGUID(refiid, &IID_IUnknown)
        || IsEqualGUID(refiid, &IID_IDirectDrawPalette))
    {
        *obj = iface;
        IDirectDrawPalette_AddRef(iface);
        return S_OK;
    }
    else
    {
        *obj = NULL;
        return E_NOINTERFACE;
    }
}

/*****************************************************************************
 * IDirectDrawPaletteImpl::AddRef
 *
 * Increases the refcount.
 *
 * Returns:
 *  The new refcount
 *
 *****************************************************************************/
static ULONG WINAPI ddraw_palette_AddRef(IDirectDrawPalette *iface)
{
    struct ddraw_palette *This = impl_from_IDirectDrawPalette(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %lu.\n", This, ref);

    return ref;
}

/*****************************************************************************
 * IDirectDrawPaletteImpl::Release
 *
 * Reduces the refcount. If the refcount falls to 0, the object is destroyed
 *
 * Returns:
 *  The new refcount
 *
 *****************************************************************************/
static ULONG WINAPI ddraw_palette_Release(IDirectDrawPalette *iface)
{
    struct ddraw_palette *palette = impl_from_IDirectDrawPalette(iface);
    ULONG ref = InterlockedDecrement(&palette->ref);

    TRACE("%p decreasing refcount to %lu.\n", palette, ref);

    if (ref == 0)
    {
        wined3d_mutex_lock();
        wined3d_palette_decref(palette->wined3d_palette);
        if ((palette->flags & DDPCAPS_PRIMARYSURFACE) && palette->ddraw->primary)
            palette->ddraw->primary->palette = NULL;
        if (palette->ifaceToRelease)
            IUnknown_Release(palette->ifaceToRelease);
        wined3d_mutex_unlock();

        free(palette);
    }

    return ref;
}

/*****************************************************************************
 * IDirectDrawPalette::Initialize
 *
 * Initializes the palette. As we start initialized, return
 * DDERR_ALREADYINITIALIZED
 *
 * Params:
 *  DD: DirectDraw interface this palette is assigned to
 *  Flags: Some flags, as usual
 *  ColorTable: The startup color table
 *
 * Returns:
 *  DDERR_ALREADYINITIALIZED
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw_palette_Initialize(IDirectDrawPalette *iface,
        IDirectDraw *ddraw, DWORD flags, PALETTEENTRY *entries)
{
    TRACE("iface %p, ddraw %p, flags %#lx, entries %p.\n",
            iface, ddraw, flags, entries);

    return DDERR_ALREADYINITIALIZED;
}

static HRESULT WINAPI ddraw_palette_GetCaps(IDirectDrawPalette *iface, DWORD *caps)
{
    struct ddraw_palette *palette = impl_from_IDirectDrawPalette(iface);

    TRACE("iface %p, caps %p.\n", iface, caps);

    wined3d_mutex_lock();
    *caps = palette->flags;
    wined3d_mutex_unlock();

    return D3D_OK;
}

/*****************************************************************************
 * IDirectDrawPalette::SetEntries
 *
 * Sets the palette entries from a PALETTEENTRY structure. WineD3D takes
 * care for updating the surface.
 *
 * Params:
 *  Flags: Flags, as usual
 *  Start: First palette entry to set
 *  Count: Number of entries to set
 *  PalEnt: Source entries
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if PalEnt is NULL
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw_palette_SetEntries(IDirectDrawPalette *iface,
        DWORD flags, DWORD start, DWORD count, PALETTEENTRY *entries)
{
    struct ddraw_palette *palette = impl_from_IDirectDrawPalette(iface);
    HRESULT hr;

    TRACE("iface %p, flags %#lx, start %lu, count %lu, entries %p.\n",
            iface, flags, start, count, entries);

    if (!entries)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    hr = wined3d_palette_set_entries(palette->wined3d_palette, flags, start, count, entries);

    if (SUCCEEDED(hr) && palette->flags & DDPCAPS_PRIMARYSURFACE)
        ddraw_surface_update_frontbuffer(palette->ddraw->primary, NULL, FALSE, 0);

    wined3d_mutex_unlock();

    return hr;
}

/*****************************************************************************
 * IDirectDrawPalette::GetEntries
 *
 * Returns the entries stored in this interface.
 *
 * Params:
 *  Flags: Flags :)
 *  Start: First entry to return
 *  Count: The number of entries to return
 *  PalEnt: PALETTEENTRY structure to write the entries to
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if PalEnt is NULL
 *
 *****************************************************************************/
static HRESULT WINAPI ddraw_palette_GetEntries(IDirectDrawPalette *iface,
        DWORD flags, DWORD start, DWORD count, PALETTEENTRY *entries)
{
    struct ddraw_palette *palette = impl_from_IDirectDrawPalette(iface);
    HRESULT hr;

    TRACE("iface %p, flags %#lx, start %lu, count %lu, entries %p.\n",
            iface, flags, start, count, entries);

    if (!entries)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    hr = wined3d_palette_get_entries(palette->wined3d_palette, flags, start, count, entries);
    wined3d_mutex_unlock();

    return hr;
}

/* Some windowed mode wrappers expect this vtbl to be writable. */
static struct IDirectDrawPaletteVtbl ddraw_palette_vtbl =
{
    /*** IUnknown ***/
    ddraw_palette_QueryInterface,
    ddraw_palette_AddRef,
    ddraw_palette_Release,
    /*** IDirectDrawPalette ***/
    ddraw_palette_GetCaps,
    ddraw_palette_GetEntries,
    ddraw_palette_Initialize,
    ddraw_palette_SetEntries
};

struct ddraw_palette *unsafe_impl_from_IDirectDrawPalette(IDirectDrawPalette *iface)
{
    if (!iface) return NULL;
    assert(iface->lpVtbl == &ddraw_palette_vtbl);
    return CONTAINING_RECORD(iface, struct ddraw_palette, IDirectDrawPalette_iface);
}

static unsigned int palette_size(DWORD flags)
{
    switch (flags & (DDPCAPS_1BIT | DDPCAPS_2BIT | DDPCAPS_4BIT | DDPCAPS_8BIT))
    {
        case DDPCAPS_1BIT:
            return 2;
        case DDPCAPS_2BIT:
            return 4;
        case DDPCAPS_4BIT:
            return 16;
        case DDPCAPS_8BIT:
            return 256;
        default:
            return ~0u;
    }
}

HRESULT ddraw_palette_init(struct ddraw_palette *palette,
        struct ddraw *ddraw, DWORD flags, PALETTEENTRY *entries)
{
    unsigned int entry_count;
    DWORD wined3d_flags = 0;
    HRESULT hr;

    if ((entry_count = palette_size(flags)) == ~0u)
    {
        WARN("Invalid flags %#lx.\n", flags);
        return DDERR_INVALIDPARAMS;
    }

    if (flags & DDPCAPS_8BITENTRIES)
        wined3d_flags |= WINED3D_PALETTE_8BIT_ENTRIES;
    if (flags & DDPCAPS_ALLOW256)
        wined3d_flags |= WINED3D_PALETTE_ALLOW_256;
    if (flags & DDPCAPS_ALPHA)
        wined3d_flags |= WINED3D_PALETTE_ALPHA;

    palette->IDirectDrawPalette_iface.lpVtbl = &ddraw_palette_vtbl;
    palette->ref = 1;
    palette->flags = flags;

    if (FAILED(hr = wined3d_palette_create(ddraw->wined3d_device,
            wined3d_flags, entry_count, entries, &palette->wined3d_palette)))
    {
        WARN("Failed to create wined3d palette, hr %#lx.\n", hr);
        return hr;
    }

    palette->ddraw = ddraw;
    palette->ifaceToRelease = (IUnknown *)&ddraw->IDirectDraw7_iface;
    IUnknown_AddRef(palette->ifaceToRelease);

    return DD_OK;
}
