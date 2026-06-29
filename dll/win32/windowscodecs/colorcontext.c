/*
 * Copyright 2012 Hans Leidekker for CodeWeavers
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

typedef struct ColorContext {
    IWICColorContext IWICColorContext_iface;
    LONG ref;
    WICColorContextType type;
    BYTE *profile;
    UINT profile_len;
    UINT exif_color_space;
} ColorContext;

static inline ColorContext *impl_from_IWICColorContext(IWICColorContext *iface)
{
    return CONTAINING_RECORD(iface, ColorContext, IWICColorContext_iface);
}

static HRESULT WINAPI ColorContext_QueryInterface(IWICColorContext *iface, REFIID iid,
    void **ppv)
{
    ColorContext *This = impl_from_IWICColorContext(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICColorContext, iid))
    {
        *ppv = &This->IWICColorContext_iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ColorContext_AddRef(IWICColorContext *iface)
{
    ColorContext *This = impl_from_IWICColorContext(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI ColorContext_Release(IWICColorContext *iface)
{
    ColorContext *This = impl_from_IWICColorContext(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
    {
        free(This->profile);
        free(This);
    }

    return ref;
}

static HRESULT load_profile(const WCHAR *filename, BYTE **profile, UINT *len)
{
    HANDLE handle;
    DWORD count;
    LARGE_INTEGER size;
    BOOL ret;

    *len = 0;
    *profile = NULL;
    handle = CreateFileW(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (handle == INVALID_HANDLE_VALUE) return HRESULT_FROM_WIN32(GetLastError());

    if (!(GetFileSizeEx(handle, &size)))
    {
        CloseHandle(handle);
        return HRESULT_FROM_WIN32(GetLastError());
    }
    if (size.u.HighPart)
    {
        WARN("file too large\n");
        CloseHandle(handle);
        return E_FAIL;
    }
    if (!(*profile = malloc(size.u.LowPart)))
    {
        CloseHandle(handle);
        return E_OUTOFMEMORY;
    }
    ret = ReadFile(handle, *profile, size.u.LowPart, &count, NULL);
    CloseHandle(handle);
    if (!ret) {
        free(*profile);
        *profile = NULL;
        return HRESULT_FROM_WIN32(GetLastError());
    }
    if (count != size.u.LowPart) {
        free(*profile);
        *profile = NULL;
        return E_FAIL;
    }
    *len = count;
    return S_OK;
}

static HRESULT WINAPI ColorContext_InitializeFromFilename(IWICColorContext *iface,
    LPCWSTR wzFilename)
{
    ColorContext *This = impl_from_IWICColorContext(iface);
    BYTE *profile;
    UINT len;
    HRESULT hr;
    TRACE("(%p,%s)\n", iface, debugstr_w(wzFilename));

    if (This->type != WICColorContextUninitialized && This->type != WICColorContextProfile)
        return WINCODEC_ERR_WRONGSTATE;

    if (!wzFilename) return E_INVALIDARG;

    hr = load_profile(wzFilename, &profile, &len);
    if (FAILED(hr)) return hr;

    free(This->profile);
    This->profile = profile;
    This->profile_len = len;
    This->type = WICColorContextProfile;

    return S_OK;
}

static HRESULT WINAPI ColorContext_InitializeFromMemory(IWICColorContext *iface,
    const BYTE *pbBuffer, UINT cbBufferSize)
{
    ColorContext *This = impl_from_IWICColorContext(iface);
    BYTE *profile;
    TRACE("(%p,%p,%u)\n", iface, pbBuffer, cbBufferSize);

    if (This->type != WICColorContextUninitialized && This->type != WICColorContextProfile)
        return WINCODEC_ERR_WRONGSTATE;

    if (!(profile = malloc(cbBufferSize))) return E_OUTOFMEMORY;
    memcpy(profile, pbBuffer, cbBufferSize);

    free(This->profile);
    This->profile = profile;
    This->profile_len = cbBufferSize;
    This->type = WICColorContextProfile;

    return S_OK;
}

static HRESULT WINAPI ColorContext_InitializeFromExifColorSpace(IWICColorContext *iface,
    UINT value)
{
    ColorContext *This = impl_from_IWICColorContext(iface);
    TRACE("(%p,%u)\n", iface, value);

    if (This->type != WICColorContextUninitialized && This->type != WICColorContextExifColorSpace)
        return WINCODEC_ERR_WRONGSTATE;

    This->exif_color_space = value;
    This->type = WICColorContextExifColorSpace;

    return S_OK;
}

static HRESULT WINAPI ColorContext_GetType(IWICColorContext *iface,
    WICColorContextType *pType)
{
    ColorContext *This = impl_from_IWICColorContext(iface);
    TRACE("(%p,%p)\n", iface, pType);

    if (!pType) return E_INVALIDARG;

    *pType = This->type;
    return S_OK;
}

static HRESULT WINAPI ColorContext_GetProfileBytes(IWICColorContext *iface,
    UINT cbBuffer, BYTE *pbBuffer, UINT *pcbActual)
{
    ColorContext *This = impl_from_IWICColorContext(iface);
    TRACE("(%p,%u,%p,%p)\n", iface, cbBuffer, pbBuffer, pcbActual);

    if (This->type != WICColorContextProfile)
        return WINCODEC_ERR_NOTINITIALIZED;

    if (!pcbActual) return E_INVALIDARG;

    if (cbBuffer >= This->profile_len && pbBuffer)
        memcpy(pbBuffer, This->profile, This->profile_len);

    *pcbActual = This->profile_len;

    return S_OK;
}

static HRESULT WINAPI ColorContext_GetExifColorSpace(IWICColorContext *iface,
    UINT *pValue)
{
    ColorContext *This = impl_from_IWICColorContext(iface);
    TRACE("(%p,%p)\n", iface, pValue);

    if (!pValue) return E_INVALIDARG;

    *pValue = This->exif_color_space;
    return S_OK;
}

static const IWICColorContextVtbl ColorContext_Vtbl = {
    ColorContext_QueryInterface,
    ColorContext_AddRef,
    ColorContext_Release,
    ColorContext_InitializeFromFilename,
    ColorContext_InitializeFromMemory,
    ColorContext_InitializeFromExifColorSpace,
    ColorContext_GetType,
    ColorContext_GetProfileBytes,
    ColorContext_GetExifColorSpace
};

HRESULT ColorContext_Create(IWICColorContext **colorcontext)
{
    ColorContext *This;

    if (!colorcontext) return E_INVALIDARG;

    This = malloc(sizeof(ColorContext));
    if (!This) return E_OUTOFMEMORY;

    This->IWICColorContext_iface.lpVtbl = &ColorContext_Vtbl;
    This->ref = 1;
    This->type = 0;
    This->profile = NULL;
    This->profile_len = 0;
    This->exif_color_space = ~0u;

    *colorcontext = &This->IWICColorContext_iface;

    return S_OK;
}
