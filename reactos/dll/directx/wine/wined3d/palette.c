/*		DirectDraw - IDirectPalette base interface
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
#include "config.h"
#include "winerror.h"
#include "wine/debug.h"

#include <assert.h>
#include <string.h>

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

#define SIZE_BITS (WINEDDPCAPS_1BIT | WINEDDPCAPS_2BIT | WINEDDPCAPS_4BIT | WINEDDPCAPS_8BIT)

static HRESULT  WINAPI IWineD3DPaletteImpl_QueryInterface(IWineD3DPalette *iface, REFIID refiid, void **obj) {
    IWineD3DPaletteImpl *This = (IWineD3DPaletteImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(refiid),obj);

    if (IsEqualGUID(refiid, &IID_IUnknown)
        || IsEqualGUID(refiid, &IID_IWineD3DPalette)) {
        *obj = iface;
        IWineD3DPalette_AddRef(iface);
        return S_OK;
    }
    else {
        *obj = NULL;
        return E_NOINTERFACE;
    }
}

static ULONG  WINAPI IWineD3DPaletteImpl_AddRef(IWineD3DPalette *iface) {
    IWineD3DPaletteImpl *This = (IWineD3DPaletteImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->() incrementing from %u.\n", This, ref - 1);

    return ref;
}

static ULONG  WINAPI IWineD3DPaletteImpl_Release(IWineD3DPalette *iface) {
    IWineD3DPaletteImpl *This = (IWineD3DPaletteImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->() decrementing from %u.\n", This, ref + 1);

    if (!ref) {
        DeleteObject(This->hpal);
        HeapFree(GetProcessHeap(), 0, This);
        return 0;
    }

    return ref;
}

/* Not called from the vtable */
DWORD IWineD3DPaletteImpl_Size(DWORD dwFlags) {
    switch (dwFlags & SIZE_BITS) {
        case WINEDDPCAPS_1BIT: return 2;
        case WINEDDPCAPS_2BIT: return 4;
        case WINEDDPCAPS_4BIT: return 16;
        case WINEDDPCAPS_8BIT: return 256;
        default: assert(0); return 256;
    }
}

static HRESULT  WINAPI IWineD3DPaletteImpl_GetEntries(IWineD3DPalette *iface, DWORD Flags, DWORD Start, DWORD Count, PALETTEENTRY *PalEnt) {
    IWineD3DPaletteImpl *This = (IWineD3DPaletteImpl *)iface;

    TRACE("(%p)->(%08x,%d,%d,%p)\n",This,Flags,Start,Count,PalEnt);

    if (Flags != 0) return WINED3DERR_INVALIDCALL; /* unchecked */
    if (Start + Count > IWineD3DPaletteImpl_Size(This->Flags))
        return WINED3DERR_INVALIDCALL;

    if (This->Flags & WINEDDPCAPS_8BITENTRIES)
    {
        unsigned int i;
        LPBYTE entry = (LPBYTE)PalEnt;

        for (i=Start; i < Count+Start; i++)
            *entry++ = This->palents[i].peRed;
    }
    else
        memcpy(PalEnt, This->palents+Start, Count * sizeof(PALETTEENTRY));

    return WINED3D_OK;
}

static HRESULT  WINAPI IWineD3DPaletteImpl_SetEntries(IWineD3DPalette *iface, DWORD Flags, DWORD Start, DWORD Count, PALETTEENTRY *PalEnt)
{
    IWineD3DPaletteImpl *This = (IWineD3DPaletteImpl *)iface;
    IWineD3DResourceImpl *res;

    TRACE("(%p)->(%08x,%d,%d,%p)\n",This,Flags,Start,Count,PalEnt);
    TRACE("Palette flags: %#x\n", This->Flags);

    if (This->Flags & WINEDDPCAPS_8BITENTRIES) {
        unsigned int i;
        const BYTE* entry = (const BYTE*)PalEnt;

        for (i=Start; i < Count+Start; i++)
            This->palents[i].peRed = *entry++;
    }
    else {
        memcpy(This->palents+Start, PalEnt, Count * sizeof(PALETTEENTRY));

        /* When WINEDDCAPS_ALLOW256 isn't set we need to override entry 0 with black and 255 with white */
        if(!(This->Flags & WINEDDPCAPS_ALLOW256))
        {
            TRACE("WINEDDPCAPS_ALLOW256 set, overriding palette entry 0 with black and 255 with white\n");
            This->palents[0].peRed = 0;
            This->palents[0].peGreen = 0;
            This->palents[0].peBlue = 0;

            This->palents[255].peRed = 255;
            This->palents[255].peGreen = 255;
            This->palents[255].peBlue = 255;
        }

        if (This->hpal)
            SetPaletteEntries(This->hpal, Start, Count, This->palents+Start);
    }

#if 0
    /* Now, if we are in 'depth conversion mode', update the screen palette */
    /* FIXME: we need to update the image or we won't get palette fading. */
    if (This->ddraw->d->palette_convert != NULL)
        This->ddraw->d->palette_convert(palent,This->screen_palents,start,count);
#endif

    /* If the palette is attached to the render target, update all render targets */

    LIST_FOR_EACH_ENTRY(res, &This->wineD3DDevice->resources, IWineD3DResourceImpl, resource.resource_list_entry) {
        if(IWineD3DResource_GetType((IWineD3DResource *) res) == WINED3DRTYPE_SURFACE) {
            IWineD3DSurfaceImpl *impl = (IWineD3DSurfaceImpl *) res;
            if(impl->palette == This)
                IWineD3DSurface_RealizePalette((IWineD3DSurface *) res);
        }
    }

    return WINED3D_OK;
}

static HRESULT  WINAPI IWineD3DPaletteImpl_GetCaps(IWineD3DPalette *iface, DWORD *Caps) {
    IWineD3DPaletteImpl *This = (IWineD3DPaletteImpl *)iface;
    TRACE("(%p)->(%p)\n", This, Caps);

    *Caps = This->Flags;
    return WINED3D_OK;
}

static HRESULT  WINAPI IWineD3DPaletteImpl_GetParent(IWineD3DPalette *iface, IUnknown **Parent) {
    IWineD3DPaletteImpl *This = (IWineD3DPaletteImpl *)iface;
    TRACE("(%p)->(%p)\n", This, Parent);

    *Parent = This->parent;
    IUnknown_AddRef(This->parent);
    return WINED3D_OK;
}

const IWineD3DPaletteVtbl IWineD3DPalette_Vtbl =
{
    /*** IUnknown ***/
    IWineD3DPaletteImpl_QueryInterface,
    IWineD3DPaletteImpl_AddRef,
    IWineD3DPaletteImpl_Release,
    /*** IWineD3DPalette ***/
    IWineD3DPaletteImpl_GetParent,
    IWineD3DPaletteImpl_GetEntries,
    IWineD3DPaletteImpl_GetCaps,
    IWineD3DPaletteImpl_SetEntries
};
