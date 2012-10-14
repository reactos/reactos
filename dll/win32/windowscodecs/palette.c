/*
 * Copyright 2009 Vincent Povirk for CodeWeavers
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "objbase.h"
#include "wincodec.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

typedef struct {
    const IWICPaletteVtbl *lpIWICPaletteVtbl;
    LONG ref;
    UINT count;
    WICColor *colors;
    WICBitmapPaletteType type;
    CRITICAL_SECTION lock; /* must be held when count, colors, or type is accessed */
} PaletteImpl;

static HRESULT WINAPI PaletteImpl_QueryInterface(IWICPalette *iface, REFIID iid,
    void **ppv)
{
    PaletteImpl *This = (PaletteImpl*)iface;
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) || IsEqualIID(&IID_IWICPalette, iid))
    {
        *ppv = This;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI PaletteImpl_AddRef(IWICPalette *iface)
{
    PaletteImpl *This = (PaletteImpl*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI PaletteImpl_Release(IWICPalette *iface)
{
    PaletteImpl *This = (PaletteImpl*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        HeapFree(GetProcessHeap(), 0, This->colors);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI PaletteImpl_InitializePredefined(IWICPalette *iface,
    WICBitmapPaletteType ePaletteType, BOOL fAddTransparentColor)
{
    FIXME("(%p,%u,%i): stub\n", iface, ePaletteType, fAddTransparentColor);
    return E_NOTIMPL;
}

static HRESULT WINAPI PaletteImpl_InitializeCustom(IWICPalette *iface,
    WICColor *pColors, UINT colorCount)
{
    PaletteImpl *This = (PaletteImpl*)iface;
    WICColor *new_colors;

    TRACE("(%p,%p,%u)\n", iface, pColors, colorCount);

    if (colorCount == 0)
    {
        new_colors = NULL;
    }
    else
    {
        if (!pColors) return E_INVALIDARG;
        new_colors = HeapAlloc(GetProcessHeap(), 0, sizeof(WICColor) * colorCount);
        if (!new_colors) return E_OUTOFMEMORY;
        memcpy(new_colors, pColors, sizeof(WICColor) * colorCount);
    }

    EnterCriticalSection(&This->lock);
    HeapFree(GetProcessHeap(), 0, This->colors);
    This->colors = new_colors;
    This->count = colorCount;
    This->type = WICBitmapPaletteTypeCustom;
    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI PaletteImpl_InitializeFromBitmap(IWICPalette *iface,
    IWICBitmapSource *pISurface, UINT colorCount, BOOL fAddTransparentColor)
{
    FIXME("(%p,%p,%u,%i): stub\n", iface, pISurface, colorCount, fAddTransparentColor);
    return E_NOTIMPL;
}

static HRESULT WINAPI PaletteImpl_InitializeFromPalette(IWICPalette *iface,
    IWICPalette *pIPalette)
{
    FIXME("(%p,%p): stub\n", iface, pIPalette);
    return E_NOTIMPL;
}

static HRESULT WINAPI PaletteImpl_GetType(IWICPalette *iface,
    WICBitmapPaletteType *pePaletteType)
{
    PaletteImpl *This = (PaletteImpl*)iface;

    TRACE("(%p,%p)\n", iface, pePaletteType);

    if (!pePaletteType) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);
    *pePaletteType = This->type;
    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI PaletteImpl_GetColorCount(IWICPalette *iface, UINT *pcCount)
{
    PaletteImpl *This = (PaletteImpl*)iface;

    TRACE("(%p,%p)\n", iface, pcCount);

    if (!pcCount) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);
    *pcCount = This->count;
    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI PaletteImpl_GetColors(IWICPalette *iface, UINT colorCount,
    WICColor *pColors, UINT *pcActualColors)
{
    PaletteImpl *This = (PaletteImpl*)iface;

    TRACE("(%p,%i,%p,%p)\n", iface, colorCount, pColors, pcActualColors);

    if (!pColors || !pcActualColors) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);

    if (This->count < colorCount) colorCount = This->count;

    memcpy(pColors, This->colors, sizeof(WICColor) * colorCount);

    *pcActualColors = colorCount;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI PaletteImpl_IsBlackWhite(IWICPalette *iface, BOOL *pfIsBlackWhite)
{
    PaletteImpl *This = (PaletteImpl*)iface;

    TRACE("(%p,%p)\n", iface, pfIsBlackWhite);

    if (!pfIsBlackWhite) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);
    if (This->type == WICBitmapPaletteTypeFixedBW)
        *pfIsBlackWhite = TRUE;
    else
        *pfIsBlackWhite = FALSE;
    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI PaletteImpl_IsGrayscale(IWICPalette *iface, BOOL *pfIsGrayscale)
{
    PaletteImpl *This = (PaletteImpl*)iface;

    TRACE("(%p,%p)\n", iface, pfIsGrayscale);

    if (!pfIsGrayscale) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);
    switch(This->type)
    {
        case WICBitmapPaletteTypeFixedBW:
        case WICBitmapPaletteTypeFixedGray4:
        case WICBitmapPaletteTypeFixedGray16:
        case WICBitmapPaletteTypeFixedGray256:
            *pfIsGrayscale = TRUE;
            break;
        default:
            *pfIsGrayscale = FALSE;
    }
    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI PaletteImpl_HasAlpha(IWICPalette *iface, BOOL *pfHasAlpha)
{
    PaletteImpl *This = (PaletteImpl*)iface;
    int i;

    TRACE("(%p,%p)\n", iface, pfHasAlpha);

    if (!pfHasAlpha) return E_INVALIDARG;

    *pfHasAlpha = FALSE;

    EnterCriticalSection(&This->lock);
    for (i=0; i<This->count; i++)
        if ((This->colors[i]&0xff000000) != 0xff000000)
        {
            *pfHasAlpha = TRUE;
            break;
        }
    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static const IWICPaletteVtbl PaletteImpl_Vtbl = {
    PaletteImpl_QueryInterface,
    PaletteImpl_AddRef,
    PaletteImpl_Release,
    PaletteImpl_InitializePredefined,
    PaletteImpl_InitializeCustom,
    PaletteImpl_InitializeFromBitmap,
    PaletteImpl_InitializeFromPalette,
    PaletteImpl_GetType,
    PaletteImpl_GetColorCount,
    PaletteImpl_GetColors,
    PaletteImpl_IsBlackWhite,
    PaletteImpl_IsGrayscale,
    PaletteImpl_HasAlpha
};

HRESULT PaletteImpl_Create(IWICPalette **palette)
{
    PaletteImpl *This;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(PaletteImpl));
    if (!This) return E_OUTOFMEMORY;

    This->lpIWICPaletteVtbl = &PaletteImpl_Vtbl;
    This->ref = 1;
    This->count = 0;
    This->colors = NULL;
    This->type = WICBitmapPaletteTypeCustom;
    InitializeCriticalSection(&This->lock);
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": PaletteImpl.lock");

    *palette = (IWICPalette*)This;

    return S_OK;
}
