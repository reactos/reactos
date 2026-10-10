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
    unsigned int refcount = InterlockedIncrement(&palette->ref);

    TRACE("%p increasing refcount to %u.\n", palette, refcount);

    return refcount;
}

static void wined3d_palette_destroy_object(void *object)
{
    TRACE("object %p.\n", object);

    free(object);
}

ULONG CDECL wined3d_palette_decref(struct wined3d_palette *palette)
{
    unsigned int refcount = InterlockedDecrement(&palette->ref);

    TRACE("%p decreasing refcount to %u.\n", palette, refcount);

    if (!refcount)
    {
        wined3d_mutex_lock();
        wined3d_cs_destroy_object(palette->device->cs, wined3d_palette_destroy_object, palette);
        wined3d_mutex_unlock();
    }

    return refcount;
}

HRESULT CDECL wined3d_palette_get_entries(const struct wined3d_palette *palette,
        uint32_t flags, unsigned int start, unsigned int count, PALETTEENTRY *entries)
{
    unsigned int i;
    TRACE("palette %p, flags %#x, start %u, count %u, entries %p.\n",
            palette, flags, start, count, entries);

    if (flags)
        return WINED3DERR_INVALIDCALL; /* unchecked */
    if (!wined3d_bound_range(start, count, palette->size))
        return WINED3DERR_INVALIDCALL;

    if (palette->flags & WINED3D_PALETTE_8BIT_ENTRIES)
    {
        BYTE *entry = (BYTE *)entries;

        for (i = start; i < count + start; ++i)
            *entry++ = palette->colors[i].rgbRed;
    }
    else
    {
        for (i = 0; i < count; ++i)
        {
            entries[i].peRed = palette->colors[i + start].rgbRed;
            entries[i].peGreen = palette->colors[i + start].rgbGreen;
            entries[i].peBlue = palette->colors[i + start].rgbBlue;
            entries[i].peFlags = palette->colors[i + start].rgbReserved;
        }
    }

    return WINED3D_OK;
}

void CDECL wined3d_palette_apply_to_dc(const struct wined3d_palette *palette, HDC dc)
{
    if (SetDIBColorTable(dc, 0, 256, palette->colors) != 256)
        ERR("Failed to set DIB color table.\n");
}

HRESULT CDECL wined3d_palette_set_entries(struct wined3d_palette *palette,
        uint32_t flags, unsigned int start, unsigned int count, const PALETTEENTRY *entries)
{
    unsigned int i;

    TRACE("palette %p, flags %#x, start %u, count %u, entries %p.\n",
            palette, flags, start, count, entries);
    TRACE("Palette flags: %#x.\n", palette->flags);

    wined3d_cs_finish(palette->device->cs, WINED3D_CS_QUEUE_DEFAULT);

    if (palette->flags & WINED3D_PALETTE_8BIT_ENTRIES)
    {
        const BYTE *entry = (const BYTE *)entries;

        for (i = start; i < count + start; ++i)
            palette->colors[i].rgbRed = *entry++;
    }
    else
    {
        for (i = 0; i < count; ++i)
        {
            palette->colors[i + start].rgbRed = entries[i].peRed;
            palette->colors[i + start].rgbGreen = entries[i].peGreen;
            palette->colors[i + start].rgbBlue = entries[i].peBlue;
            palette->colors[i + start].rgbReserved = entries[i].peFlags;
        }

        /* When WINEDDCAPS_ALLOW256 isn't set we need to override entry 0 with black and 255 with white */
        if (!(palette->flags & WINED3D_PALETTE_ALLOW_256))
        {
            TRACE("WINED3D_PALETTE_ALLOW_256 not set, overriding palette entry 0 with black and 255 with white.\n");
            palette->colors[0].rgbRed = 0;
            palette->colors[0].rgbGreen = 0;
            palette->colors[0].rgbBlue = 0;

            palette->colors[255].rgbRed = 255;
            palette->colors[255].rgbGreen = 255;
            palette->colors[255].rgbBlue = 255;
        }
    }

    return WINED3D_OK;
}

static HRESULT wined3d_palette_init(struct wined3d_palette *palette, struct wined3d_device *device,
        uint32_t flags, unsigned int entry_count, const PALETTEENTRY *entries)
{
    HRESULT hr;

    palette->ref = 1;
    palette->device = device;
    palette->flags = flags;
    palette->size = entry_count;

    if (FAILED(hr = wined3d_palette_set_entries(palette, 0, 0, entry_count, entries)))
    {
        WARN("Failed to set palette entries, hr %#lx.\n", hr);
        return hr;
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_palette_create(struct wined3d_device *device, uint32_t flags,
        unsigned int entry_count, const PALETTEENTRY *entries, struct wined3d_palette **palette)
{
    struct wined3d_palette *object;
    HRESULT hr;

    TRACE("device %p, flags %#x, entry_count %u, entries %p, palette %p.\n",
            device, flags, entry_count, entries, palette);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = wined3d_palette_init(object, device, flags, entry_count, entries)))
    {
        WARN("Failed to initialize palette, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    TRACE("Created palette %p.\n", object);
    *palette = object;

    return WINED3D_OK;
}
