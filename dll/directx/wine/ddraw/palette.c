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

#include "config.h"
#include "wine/port.h"

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
static HRESULT WINAPI
IDirectDrawPaletteImpl_QueryInterface(IDirectDrawPalette *iface,
                                      REFIID refiid,
                                      void **obj)
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
static ULONG WINAPI
IDirectDrawPaletteImpl_AddRef(IDirectDrawPalette *iface)
{
    IDirectDrawPaletteImpl *This = (IDirectDrawPaletteImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %u.\n", This, ref);

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
static ULONG WINAPI
IDirectDrawPaletteImpl_Release(IDirectDrawPalette *iface)
{
    IDirectDrawPaletteImpl *This = (IDirectDrawPaletteImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u.\n", This, ref);

    if (ref == 0)
    {
        EnterCriticalSection(&ddraw_cs);
        IWineD3DPalette_Release(This->wineD3DPalette);
        if(This->ifaceToRelease)
        {
            IUnknown_Release(This->ifaceToRelease);
        }
        LeaveCriticalSection(&ddraw_cs);
        HeapFree(GetProcessHeap(), 0, This);
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
static HRESULT WINAPI
IDirectDrawPaletteImpl_Initialize(IDirectDrawPalette *iface,
                                  IDirectDraw *DD,
                                  DWORD Flags,
                                  PALETTEENTRY *ColorTable)
{
    TRACE("iface %p, ddraw %p, flags %#x, entries %p.\n",
            iface, DD, Flags, ColorTable);

    return DDERR_ALREADYINITIALIZED;
}

/*****************************************************************************
 * IDirectDrawPalette::GetCaps
 *
 * Returns the palette description
 *
 * Params:
 *  Caps: Address to store the caps at
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if Caps is NULL
 *  For more details, see IWineD3DPalette::GetCaps
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawPaletteImpl_GetCaps(IDirectDrawPalette *iface,
                               DWORD *Caps)
{
    IDirectDrawPaletteImpl *This = (IDirectDrawPaletteImpl *)iface;
    HRESULT hr;

    TRACE("iface %p, caps %p.\n", iface, Caps);

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DPalette_GetCaps(This->wineD3DPalette, Caps);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
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
 *  For details, see IWineD3DDevice::SetEntries
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawPaletteImpl_SetEntries(IDirectDrawPalette *iface,
                                  DWORD Flags,
                                  DWORD Start,
                                  DWORD Count,
                                  PALETTEENTRY *PalEnt)
{
    IDirectDrawPaletteImpl *This = (IDirectDrawPaletteImpl *)iface;
    HRESULT hr;

    TRACE("iface %p, flags %#x, start %u, count %u, entries %p.\n",
            iface, Flags, Start, Count, PalEnt);

    if(!PalEnt)
        return DDERR_INVALIDPARAMS;

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DPalette_SetEntries(This->wineD3DPalette, Flags, Start, Count, PalEnt);
    LeaveCriticalSection(&ddraw_cs);
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
 *  For details, see IWineD3DDevice::SetEntries
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirectDrawPaletteImpl_GetEntries(IDirectDrawPalette *iface,
                                  DWORD Flags,
                                  DWORD Start,
                                  DWORD Count,
                                  PALETTEENTRY *PalEnt)
{
    IDirectDrawPaletteImpl *This = (IDirectDrawPaletteImpl *)iface;
    HRESULT hr;

    TRACE("iface %p, flags %#x, start %u, count %u, entries %p.\n",
            iface, Flags, Start, Count, PalEnt);

    if(!PalEnt)
        return DDERR_INVALIDPARAMS;

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DPalette_GetEntries(This->wineD3DPalette, Flags, Start, Count, PalEnt);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

static const struct IDirectDrawPaletteVtbl ddraw_palette_vtbl =
{
    /*** IUnknown ***/
    IDirectDrawPaletteImpl_QueryInterface,
    IDirectDrawPaletteImpl_AddRef,
    IDirectDrawPaletteImpl_Release,
    /*** IDirectDrawPalette ***/
    IDirectDrawPaletteImpl_GetCaps,
    IDirectDrawPaletteImpl_GetEntries,
    IDirectDrawPaletteImpl_Initialize,
    IDirectDrawPaletteImpl_SetEntries
};

HRESULT ddraw_palette_init(IDirectDrawPaletteImpl *palette,
        IDirectDrawImpl *ddraw, DWORD flags, PALETTEENTRY *entries)
{
    HRESULT hr;

    palette->lpVtbl = &ddraw_palette_vtbl;
    palette->ref = 1;

    hr = IWineD3DDevice_CreatePalette(ddraw->wineD3DDevice, flags,
            entries, palette, &palette->wineD3DPalette);
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d palette, hr %#x.\n", hr);
        return hr;
    }

    palette->ifaceToRelease = (IUnknown *)ddraw;
    IUnknown_AddRef(palette->ifaceToRelease);

    return DD_OK;
}
