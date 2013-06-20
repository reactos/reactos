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
#include <config.h>
#include <wine/port.h>
//#include "winerror.h"
//#include "wine/debug.h"

//#include <string.h>

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

#define SIZE_BITS (WINEDDPCAPS_1BIT | WINEDDPCAPS_2BIT | WINEDDPCAPS_4BIT | WINEDDPCAPS_8BIT)

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

static WORD wined3d_palette_size(DWORD flags)
{
    switch (flags & SIZE_BITS)
    {
        case WINEDDPCAPS_1BIT: return 2;
        case WINEDDPCAPS_2BIT: return 4;
        case WINEDDPCAPS_4BIT: return 16;
        case WINEDDPCAPS_8BIT: return 256;
        default:
            FIXME("Unhandled size bits %#x.\n", flags & SIZE_BITS);
            return 256;
    }
}

HRESULT CDECL wined3d_palette_get_entries(const struct wined3d_palette *palette,
        DWORD flags, DWORD start, DWORD count, PALETTEENTRY *entries)
{
    TRACE("palette %p, flags %#x, start %u, count %u, entries %p.\n",
            palette, flags, start, count, entries);

    if (flags) return WINED3DERR_INVALIDCALL; /* unchecked */
    if (start + count > wined3d_palette_size(palette->flags))
        return WINED3DERR_INVALIDCALL;

    if (palette->flags & WINEDDPCAPS_8BITENTRIES)
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

    if (palette->flags & WINEDDPCAPS_8BITENTRIES)
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
        if (!(palette->flags & WINEDDPCAPS_ALLOW256))
        {
            TRACE("WINEDDPCAPS_ALLOW256 set, overriding palette entry 0 with black and 255 with white\n");
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

DWORD CDECL wined3d_palette_get_flags(const struct wined3d_palette *palette)
{
    TRACE("palette %p.\n", palette);

    return palette->flags;
}

void * CDECL wined3d_palette_get_parent(const struct wined3d_palette *palette)
{
    TRACE("palette %p.\n", palette);

    return palette->parent;
}

static HRESULT wined3d_palette_init(struct wined3d_palette *palette, struct wined3d_device *device,
        DWORD flags, const PALETTEENTRY *entries, void *parent)
{
    HRESULT hr;

    palette->ref = 1;
    palette->parent = parent;
    palette->device = device;
    palette->flags = flags;

    palette->palNumEntries = wined3d_palette_size(flags);
    palette->hpal = CreatePalette((const LOGPALETTE *)&palette->palVersion);
    if (!palette->hpal)
    {
        WARN("Failed to create palette.\n");
        return E_FAIL;
    }

    hr = wined3d_palette_set_entries(palette, 0, 0, wined3d_palette_size(flags), entries);
    if (FAILED(hr))
    {
        WARN("Failed to set palette entries, hr %#x.\n", hr);
        DeleteObject(palette->hpal);
        return hr;
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_palette_create(struct wined3d_device *device, DWORD flags,
        const PALETTEENTRY *entries, void *parent, struct wined3d_palette **palette)
{
    struct wined3d_palette *object;
    HRESULT hr;

    TRACE("device %p, flags %#x, entries %p, palette %p, parent %p.\n",
            device, flags, entries, palette, parent);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    hr = wined3d_palette_init(object, device, flags, entries, parent);
    if (FAILED(hr))
    {
        WARN("Failed to initialize palette, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created palette %p.\n", object);
    *palette = object;

    return WINED3D_OK;
}
