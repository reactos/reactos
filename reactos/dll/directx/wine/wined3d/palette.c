/*              DirectDraw - IDirectPalette base interface
 *
 * Copyright 1997-2000 Marcus Meissner
 * Copyright 2000-2001 TransGaming Technologies Inc.
 * Copyright 2006 Stefan Dösinger for CodeWeavers
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

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

ULONG CDECL wined3d_palette_incref(struct wined3d_palette *palette)
{
    ULONG refcount = InterlockedIncrement(&palette->ref);

    TRACE("%p increasing refcount to %u.\n", palette, refcount);

    return refcount;
}

ULONG CDECL wined3d_palette_decref(struct wined3d_palette *palette)
{
    ULONG refcount = InterlockedDecrement(&palette->ref);

    TRACE("%p decreasing refcount to %u.\n", palette, refcount);

    if (!refcount)
    {
        DeleteObject(palette->hpal);
        HeapFree(GetProcessHeap(), 0, palette);
    }

    return refcount;
}

HRESULT CDECL wined3d_palette_get_entries(const struct wined3d_palette *palette,
        DWORD flags, DWORD start, DWORD count, PALETTEENTRY *entries)
{
    TRACE("palette %p, flags %#x, start %u, count %u, entries %p.\n",
            palette, flags, start, count, entries);

    if (flags)
        return WINED3DERR_INVALIDCALL; /* unchecked */
    if (start > palette->palNumEntries || count > palette->palNumEntries - start)
        return WINED3DERR_INVALIDCALL;

    if (palette->flags & WINED3D_PALETTE_8BIT_ENTRIES)
    {
        BYTE *entry = (BYTE *)entries;
        unsigned int i;

        for (i = start; i < count + start; ++i)
            *entry++ = palette->palents[i].peRed;
    }
    else
        memcpy(entries, palette->palents + start, count * sizeof(*entries));

    return WINED3D_OK;
}

HRESULT CDECL wined3d_palette_set_entries(struct wined3d_palette *palette,
        DWORD flags, DWORD start, DWORD count, const PALETTEENTRY *entries)
{
    struct wined3d_resource *resource;

    TRACE("palette %p, flags %#x, start %u, count %u, entries %p.\n",
            palette, flags, start, count, entries);
    TRACE("Palette flags: %#x.\n", palette->flags);

    if (palette->flags & WINED3D_PALETTE_8BIT_ENTRIES)
    {
        const BYTE *entry = (const BYTE *)entries;
        unsigned int i;

        for (i = start; i < count + start; ++i)
            palette->palents[i].peRed = *entry++;
    }
    else
    {
        memcpy(palette->palents + start, entries, count * sizeof(*palette->palents));

        /* When WINEDDCAPS_ALLOW256 isn't set we need to override entry 0 with black and 255 with white */
        if (!(palette->flags & WINED3D_PALETTE_ALLOW_256))
        {
            TRACE("WINED3D_PALETTE_ALLOW_256 not set, overriding palette entry 0 with black and 255 with white.\n");
            palette->palents[0].peRed = 0;
            palette->palents[0].peGreen = 0;
            palette->palents[0].peBlue = 0;

            palette->palents[255].peRed = 255;
            palette->palents[255].peGreen = 255;
            palette->palents[255].peBlue = 255;
        }

        if (palette->hpal)
            SetPaletteEntries(palette->hpal, start, count, palette->palents + start);
    }

    /* If the palette is attached to the render target, update all render targets */
    LIST_FOR_EACH_ENTRY(resource, &palette->device->resources, struct wined3d_resource, resource_list_entry)
    {
        if (resource->type == WINED3D_RTYPE_SURFACE)
        {
            struct wined3d_surface *surface = surface_from_resource(resource);
            if (surface->palette == palette)
                surface->surface_ops->surface_realize_palette(surface);
        }
    }

    return WINED3D_OK;
}

static HRESULT wined3d_palette_init(struct wined3d_palette *palette, struct wined3d_device *device,
        DWORD flags, unsigned int entry_count, const PALETTEENTRY *entries)
{
    HRESULT hr;

    palette->ref = 1;
    palette->device = device;
    palette->flags = flags;

    palette->palNumEntries = entry_count;
    palette->hpal = CreatePalette((const LOGPALETTE *)&palette->palVersion);
    if (!palette->hpal)
    {
        WARN("Failed to create palette.\n");
        return E_FAIL;
    }

    if (FAILED(hr = wined3d_palette_set_entries(palette, 0, 0, entry_count, entries)))
    {
        WARN("Failed to set palette entries, hr %#x.\n", hr);
        DeleteObject(palette->hpal);
        return hr;
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_palette_create(struct wined3d_device *device, DWORD flags,
        unsigned int entry_count, const PALETTEENTRY *entries, struct wined3d_palette **palette)
{
    struct wined3d_palette *object;
    HRESULT hr;

    TRACE("device %p, flags %#x, entries %p, palette %p.\n",
            device, flags, entries, palette);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    if (FAILED(hr = wined3d_palette_init(object, device, flags, entry_count, entries)))
    {
        WARN("Failed to initialize palette, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created palette %p.\n", object);
    *palette = object;

    return WINED3D_OK;
}
