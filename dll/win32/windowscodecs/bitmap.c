/*
 * Copyright 2012 Vincent Povirk for CodeWeavers
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

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <config.h>

#include <stdarg.h>

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <objbase.h>
#include <wincodec.h>

#include "wincodecs_private.h"

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

typedef struct BitmapImpl {
    IWICBitmap IWICBitmap_iface;
    LONG ref;
    IWICPalette *palette;
    int palette_set;
    LONG lock; /* 0 if not locked, -1 if locked for writing, count if locked for reading */
    BYTE *data;
    UINT width, height;
    UINT stride;
    UINT bpp;
    WICPixelFormatGUID pixelformat;
    double dpix, dpiy;
    CRITICAL_SECTION cs;
} BitmapImpl;

typedef struct BitmapLockImpl {
    IWICBitmapLock IWICBitmapLock_iface;
    LONG ref;
    BitmapImpl *parent;
    UINT width, height;
    BYTE *data;
} BitmapLockImpl;

static inline BitmapImpl *impl_from_IWICBitmap(IWICBitmap *iface)
{
    return CONTAINING_RECORD(iface, BitmapImpl, IWICBitmap_iface);
}

static inline BitmapLockImpl *impl_from_IWICBitmapLock(IWICBitmapLock *iface)
{
    return CONTAINING_RECORD(iface, BitmapLockImpl, IWICBitmapLock_iface);
}

static BOOL BitmapImpl_AcquireLock(BitmapImpl *This, int write)
{
    if (write)
    {
        return 0 == InterlockedCompareExchange(&This->lock, -1, 0);
    }
    else
    {
        while (1)
        {
            LONG prev_val = This->lock;
            if (prev_val == -1)
                return FALSE;
            if (prev_val == InterlockedCompareExchange(&This->lock, prev_val+1, prev_val))
                return TRUE;
        }
    }
}

static void BitmapImpl_ReleaseLock(BitmapImpl *This)
{
    while (1)
    {
        LONG prev_val = This->lock, new_val;
        if (prev_val == -1)
            new_val = 0;
        else
            new_val = prev_val - 1;
        if (prev_val == InterlockedCompareExchange(&This->lock, new_val, prev_val))
            break;
    }
}


static HRESULT WINAPI BitmapLockImpl_QueryInterface(IWICBitmapLock *iface, REFIID iid,
    void **ppv)
{
    BitmapLockImpl *This = impl_from_IWICBitmapLock(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapLock, iid))
    {
        *ppv = &This->IWICBitmapLock_iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI BitmapLockImpl_AddRef(IWICBitmapLock *iface)
{
    BitmapLockImpl *This = impl_from_IWICBitmapLock(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI BitmapLockImpl_Release(IWICBitmapLock *iface)
{
    BitmapLockImpl *This = impl_from_IWICBitmapLock(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        BitmapImpl_ReleaseLock(This->parent);
        IWICBitmap_Release(&This->parent->IWICBitmap_iface);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI BitmapLockImpl_GetSize(IWICBitmapLock *iface,
    UINT *puiWidth, UINT *puiHeight)
{
    BitmapLockImpl *This = impl_from_IWICBitmapLock(iface);
    TRACE("(%p,%p,%p)\n", iface, puiWidth, puiHeight);

    if (!puiWidth || !puiHeight)
        return E_INVALIDARG;

    *puiWidth = This->width;
    *puiHeight = This->height;

    return S_OK;
}

static HRESULT WINAPI BitmapLockImpl_GetStride(IWICBitmapLock *iface,
    UINT *pcbStride)
{
    BitmapLockImpl *This = impl_from_IWICBitmapLock(iface);
    TRACE("(%p,%p)\n", iface, pcbStride);

    if (!pcbStride)
        return E_INVALIDARG;

    *pcbStride = This->parent->stride;

    return S_OK;
}

static HRESULT WINAPI BitmapLockImpl_GetDataPointer(IWICBitmapLock *iface,
    UINT *pcbBufferSize, BYTE **ppbData)
{
    BitmapLockImpl *This = impl_from_IWICBitmapLock(iface);
    TRACE("(%p,%p,%p)\n", iface, pcbBufferSize, ppbData);

    if (!pcbBufferSize || !ppbData)
        return E_INVALIDARG;

    *pcbBufferSize = This->parent->stride * (This->height - 1) +
        ((This->parent->bpp * This->width) + 7)/8;
    *ppbData = This->data;

    return S_OK;
}

static HRESULT WINAPI BitmapLockImpl_GetPixelFormat(IWICBitmapLock *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    BitmapLockImpl *This = impl_from_IWICBitmapLock(iface);
    TRACE("(%p,%p)\n", iface, pPixelFormat);

    return IWICBitmap_GetPixelFormat(&This->parent->IWICBitmap_iface, pPixelFormat);
}

static const IWICBitmapLockVtbl BitmapLockImpl_Vtbl = {
    BitmapLockImpl_QueryInterface,
    BitmapLockImpl_AddRef,
    BitmapLockImpl_Release,
    BitmapLockImpl_GetSize,
    BitmapLockImpl_GetStride,
    BitmapLockImpl_GetDataPointer,
    BitmapLockImpl_GetPixelFormat
};

static HRESULT WINAPI BitmapImpl_QueryInterface(IWICBitmap *iface, REFIID iid,
    void **ppv)
{
    BitmapImpl *This = impl_from_IWICBitmap(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICBitmapSource, iid) ||
        IsEqualIID(&IID_IWICBitmap, iid))
    {
        *ppv = &This->IWICBitmap_iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI BitmapImpl_AddRef(IWICBitmap *iface)
{
    BitmapImpl *This = impl_from_IWICBitmap(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI BitmapImpl_Release(IWICBitmap *iface)
{
    BitmapImpl *This = impl_from_IWICBitmap(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        if (This->palette) IWICPalette_Release(This->palette);
        This->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->cs);
        HeapFree(GetProcessHeap(), 0, This->data);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI BitmapImpl_GetSize(IWICBitmap *iface,
    UINT *puiWidth, UINT *puiHeight)
{
    BitmapImpl *This = impl_from_IWICBitmap(iface);
    TRACE("(%p,%p,%p)\n", iface, puiWidth, puiHeight);

    if (!puiWidth || !puiHeight)
        return E_INVALIDARG;

    *puiWidth = This->width;
    *puiHeight = This->height;

    return S_OK;
}

static HRESULT WINAPI BitmapImpl_GetPixelFormat(IWICBitmap *iface,
    WICPixelFormatGUID *pPixelFormat)
{
    BitmapImpl *This = impl_from_IWICBitmap(iface);
    TRACE("(%p,%p)\n", iface, pPixelFormat);

    if (!pPixelFormat)
        return E_INVALIDARG;

    memcpy(pPixelFormat, &This->pixelformat, sizeof(GUID));

    return S_OK;
}

static HRESULT WINAPI BitmapImpl_GetResolution(IWICBitmap *iface,
    double *pDpiX, double *pDpiY)
{
    BitmapImpl *This = impl_from_IWICBitmap(iface);
    TRACE("(%p,%p,%p)\n", iface, pDpiX, pDpiY);

    if (!pDpiX || !pDpiY)
        return E_INVALIDARG;

    EnterCriticalSection(&This->cs);
    *pDpiX = This->dpix;
    *pDpiY = This->dpiy;
    LeaveCriticalSection(&This->cs);

    return S_OK;
}

static HRESULT WINAPI BitmapImpl_CopyPalette(IWICBitmap *iface,
    IWICPalette *pIPalette)
{
    BitmapImpl *This = impl_from_IWICBitmap(iface);
    TRACE("(%p,%p)\n", iface, pIPalette);

    if (!This->palette_set)
        return WINCODEC_ERR_PALETTEUNAVAILABLE;

    return IWICPalette_InitializeFromPalette(pIPalette, This->palette);
}

static HRESULT WINAPI BitmapImpl_CopyPixels(IWICBitmap *iface,
    const WICRect *prc, UINT cbStride, UINT cbBufferSize, BYTE *pbBuffer)
{
    BitmapImpl *This = impl_from_IWICBitmap(iface);
    TRACE("(%p,%p,%u,%u,%p)\n", iface, prc, cbStride, cbBufferSize, pbBuffer);

    return copy_pixels(This->bpp, This->data, This->width, This->height,
        This->stride, prc, cbStride, cbBufferSize, pbBuffer);
}

static HRESULT WINAPI BitmapImpl_Lock(IWICBitmap *iface, const WICRect *prcLock,
    DWORD flags, IWICBitmapLock **ppILock)
{
    BitmapImpl *This = impl_from_IWICBitmap(iface);
    BitmapLockImpl *result;
    WICRect rc;

    TRACE("(%p,%p,%x,%p)\n", iface, prcLock, flags, ppILock);

    if (!(flags & (WICBitmapLockRead|WICBitmapLockWrite)) || !ppILock)
        return E_INVALIDARG;

    if (!prcLock)
    {
        rc.X = rc.Y = 0;
        rc.Width = This->width;
        rc.Height = This->height;
        prcLock = &rc;
    }
    else if (prcLock->X >= This->width || prcLock->Y >= This->height ||
             prcLock->X + prcLock->Width > This->width ||
             prcLock->Y + prcLock->Height > This->height ||
             prcLock->Width <= 0 || prcLock->Height <= 0)
        return E_INVALIDARG;
    else if (((prcLock->X * This->bpp) % 8) != 0)
    {
        FIXME("Cannot lock at an X coordinate not at a full byte\n");
        return E_FAIL;
    }

    result = HeapAlloc(GetProcessHeap(), 0, sizeof(BitmapLockImpl));
    if (!result)
        return E_OUTOFMEMORY;

    if (!BitmapImpl_AcquireLock(This, flags & WICBitmapLockWrite))
    {
        HeapFree(GetProcessHeap(), 0, result);
        return WINCODEC_ERR_ALREADYLOCKED;
    }

    result->IWICBitmapLock_iface.lpVtbl = &BitmapLockImpl_Vtbl;
    result->ref = 1;
    result->parent = This;
    result->width = prcLock->Width;
    result->height = prcLock->Height;
    result->data = This->data + This->stride * prcLock->Y +
        (This->bpp * prcLock->X)/8;

    IWICBitmap_AddRef(&This->IWICBitmap_iface);
    *ppILock = &result->IWICBitmapLock_iface;

    return S_OK;
}

static HRESULT WINAPI BitmapImpl_SetPalette(IWICBitmap *iface, IWICPalette *pIPalette)
{
    BitmapImpl *This = impl_from_IWICBitmap(iface);
    HRESULT hr;

    TRACE("(%p,%p)\n", iface, pIPalette);

    if (!This->palette)
    {
        IWICPalette *new_palette;
        hr = PaletteImpl_Create(&new_palette);

        if (FAILED(hr)) return hr;

        if (InterlockedCompareExchangePointer((void**)&This->palette, new_palette, NULL))
        {
            /* someone beat us to it */
            IWICPalette_Release(new_palette);
        }
    }

    hr = IWICPalette_InitializeFromPalette(This->palette, pIPalette);

    if (SUCCEEDED(hr))
        This->palette_set = 1;

    return S_OK;
}

static HRESULT WINAPI BitmapImpl_SetResolution(IWICBitmap *iface,
    double dpiX, double dpiY)
{
    BitmapImpl *This = impl_from_IWICBitmap(iface);
    TRACE("(%p,%f,%f)\n", iface, dpiX, dpiY);

    EnterCriticalSection(&This->cs);
    This->dpix = dpiX;
    This->dpiy = dpiY;
    LeaveCriticalSection(&This->cs);

    return S_OK;
}

static const IWICBitmapVtbl BitmapImpl_Vtbl = {
    BitmapImpl_QueryInterface,
    BitmapImpl_AddRef,
    BitmapImpl_Release,
    BitmapImpl_GetSize,
    BitmapImpl_GetPixelFormat,
    BitmapImpl_GetResolution,
    BitmapImpl_CopyPalette,
    BitmapImpl_CopyPixels,
    BitmapImpl_Lock,
    BitmapImpl_SetPalette,
    BitmapImpl_SetResolution
};

HRESULT BitmapImpl_Create(UINT uiWidth, UINT uiHeight,
    REFWICPixelFormatGUID pixelFormat, WICBitmapCreateCacheOption option,
    IWICBitmap **ppIBitmap)
{
    HRESULT hr;
    BitmapImpl *This;
    UINT bpp, stride, datasize;
    BYTE *data;

    hr = get_pixelformat_bpp(pixelFormat, &bpp);
    if (FAILED(hr)) return hr;

    stride = (((bpp*uiWidth)+31)/32)*4;
    datasize = stride * uiHeight;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(BitmapImpl));
    data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, datasize);
    if (!This || !data)
    {
        HeapFree(GetProcessHeap(), 0, This);
        HeapFree(GetProcessHeap(), 0, data);
        return E_OUTOFMEMORY;
    }

    This->IWICBitmap_iface.lpVtbl = &BitmapImpl_Vtbl;
    This->ref = 1;
    This->palette = NULL;
    This->palette_set = 0;
    This->lock = 0;
    This->data = data;
    This->width = uiWidth;
    This->height = uiHeight;
    This->stride = stride;
    This->bpp = bpp;
    memcpy(&This->pixelformat, pixelFormat, sizeof(GUID));
    This->dpix = This->dpiy = 0.0;
    InitializeCriticalSection(&This->cs);
    This->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": BitmapImpl.lock");

    *ppIBitmap = &This->IWICBitmap_iface;

    return S_OK;
}
