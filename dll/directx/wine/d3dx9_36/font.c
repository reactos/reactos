/*
 * Copyright (C) 2008 Tony Wasserka
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
 *
 */

#include "config.h"
#include "wine/port.h"

#include "d3dx9_private.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

struct d3dx_font
{
    ID3DXFont ID3DXFont_iface;
    LONG ref;

    IDirect3DDevice9 *device;
    D3DXFONT_DESCW desc;

    HDC hdc;
    HFONT hfont;
};

static inline struct d3dx_font *impl_from_ID3DXFont(ID3DXFont *iface)
{
    return CONTAINING_RECORD(iface, struct d3dx_font, ID3DXFont_iface);
}

static HRESULT WINAPI ID3DXFontImpl_QueryInterface(ID3DXFont *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_ID3DXFont)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ID3DXFontImpl_AddRef(ID3DXFont *iface)
{
    struct d3dx_font *This = impl_from_ID3DXFont(iface);
    ULONG ref=InterlockedIncrement(&This->ref);
    TRACE("%p increasing refcount to %u\n", iface, ref);
    return ref;
}

static ULONG WINAPI ID3DXFontImpl_Release(ID3DXFont *iface)
{
    struct d3dx_font *This = impl_from_ID3DXFont(iface);
    ULONG ref=InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u\n", iface, ref);

    if(ref==0) {
        DeleteObject(This->hfont);
        DeleteDC(This->hdc);
        IDirect3DDevice9_Release(This->device);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static HRESULT WINAPI ID3DXFontImpl_GetDevice(ID3DXFont *iface, IDirect3DDevice9 **device)
{
    struct d3dx_font *This = impl_from_ID3DXFont(iface);

    TRACE("iface %p, device %p\n", iface, device);

    if( !device ) return D3DERR_INVALIDCALL;
    *device = This->device;
    IDirect3DDevice9_AddRef(This->device);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXFontImpl_GetDescA(ID3DXFont *iface, D3DXFONT_DESCA *desc)
{
    struct d3dx_font *This = impl_from_ID3DXFont(iface);

    TRACE("iface %p, desc %p\n", iface, desc);

    if( !desc ) return D3DERR_INVALIDCALL;
    memcpy(desc, &This->desc, FIELD_OFFSET(D3DXFONT_DESCA, FaceName));
    WideCharToMultiByte(CP_ACP, 0, This->desc.FaceName, -1, desc->FaceName, ARRAY_SIZE(desc->FaceName), NULL, NULL);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXFontImpl_GetDescW(ID3DXFont *iface, D3DXFONT_DESCW *desc)
{
    struct d3dx_font *This = impl_from_ID3DXFont(iface);

    TRACE("iface %p, desc %p\n", iface, desc);

    if( !desc ) return D3DERR_INVALIDCALL;
    *desc = This->desc;

    return D3D_OK;
}

static BOOL WINAPI ID3DXFontImpl_GetTextMetricsA(ID3DXFont *iface, TEXTMETRICA *metrics)
{
    struct d3dx_font *This = impl_from_ID3DXFont(iface);
    TRACE("iface %p, metrics %p\n", iface, metrics);
    return GetTextMetricsA(This->hdc, metrics);
}

static BOOL WINAPI ID3DXFontImpl_GetTextMetricsW(ID3DXFont *iface, TEXTMETRICW *metrics)
{
    struct d3dx_font *This = impl_from_ID3DXFont(iface);
    TRACE("iface %p, metrics %p\n", iface, metrics);
    return GetTextMetricsW(This->hdc, metrics);
}

static HDC WINAPI ID3DXFontImpl_GetDC(ID3DXFont *iface)
{
    struct d3dx_font *This = impl_from_ID3DXFont(iface);
    TRACE("iface %p\n", iface);
    return This->hdc;
}

static HRESULT WINAPI ID3DXFontImpl_GetGlyphData(ID3DXFont *iface, UINT glyph,
        IDirect3DTexture9 **texture, RECT *blackbox, POINT *cellinc)
{
    FIXME("iface %p, glyph %#x, texture %p, blackbox %p, cellinc %p stub!\n",
            iface, glyph, texture, blackbox, cellinc);
    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXFontImpl_PreloadCharacters(ID3DXFont *iface, UINT first, UINT last)
{
    FIXME("iface %p, first %u, last %u stub!\n", iface, first, last);
    return S_OK;
}

static HRESULT WINAPI ID3DXFontImpl_PreloadGlyphs(ID3DXFont *iface, UINT first, UINT last)
{
    FIXME("iface %p, first %u, last %u stub!\n", iface, first, last);
    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXFontImpl_PreloadTextA(ID3DXFont *iface, const char *string, INT count)
{
    FIXME("iface %p, string %s, count %d stub!\n", iface, debugstr_a(string), count);
    return E_NOTIMPL;
}

static HRESULT WINAPI ID3DXFontImpl_PreloadTextW(ID3DXFont *iface, const WCHAR *string, INT count)
{
    FIXME("iface %p, string %s, count %d stub!\n", iface, debugstr_w(string), count);
    return E_NOTIMPL;
}

static INT WINAPI ID3DXFontImpl_DrawTextA(ID3DXFont *iface, ID3DXSprite *sprite,
        const char *string, INT count, RECT *rect, DWORD format, D3DCOLOR color)
{
    FIXME("iface %p, sprite %p, string %s, count %d, rect %s, format %#x, color 0x%08x stub!\n",
            iface,  sprite, debugstr_a(string), count, wine_dbgstr_rect(rect), format, color);
    return 1;
}

static INT WINAPI ID3DXFontImpl_DrawTextW(ID3DXFont *iface, ID3DXSprite *sprite,
        const WCHAR *string, INT count, RECT *rect, DWORD format, D3DCOLOR color)
{
    FIXME("iface %p, sprite %p, string %s, count %d, rect %s, format %#x, color 0x%08x stub!\n",
            iface,  sprite, debugstr_w(string), count, wine_dbgstr_rect(rect), format, color);
    return 1;
}

static HRESULT WINAPI ID3DXFontImpl_OnLostDevice(ID3DXFont *iface)
{
    FIXME("iface %p stub!\n", iface);
    return D3D_OK;
}

static HRESULT WINAPI ID3DXFontImpl_OnResetDevice(ID3DXFont *iface)
{
    FIXME("iface %p stub\n", iface);
    return D3D_OK;
}

static const ID3DXFontVtbl D3DXFont_Vtbl =
{
    /*** IUnknown methods ***/
    ID3DXFontImpl_QueryInterface,
    ID3DXFontImpl_AddRef,
    ID3DXFontImpl_Release,
    /*** ID3DXFont methods ***/
    ID3DXFontImpl_GetDevice,
    ID3DXFontImpl_GetDescA,
    ID3DXFontImpl_GetDescW,
    ID3DXFontImpl_GetTextMetricsA,
    ID3DXFontImpl_GetTextMetricsW,
    ID3DXFontImpl_GetDC,
    ID3DXFontImpl_GetGlyphData,
    ID3DXFontImpl_PreloadCharacters,
    ID3DXFontImpl_PreloadGlyphs,
    ID3DXFontImpl_PreloadTextA,
    ID3DXFontImpl_PreloadTextW,
    ID3DXFontImpl_DrawTextA,
    ID3DXFontImpl_DrawTextW,
    ID3DXFontImpl_OnLostDevice,
    ID3DXFontImpl_OnResetDevice
};

HRESULT WINAPI D3DXCreateFontA(struct IDirect3DDevice9 *device, INT height, UINT width,
        UINT weight, UINT miplevels, BOOL italic, DWORD charset, DWORD precision, DWORD quality,
        DWORD pitchandfamily, const char *facename, struct ID3DXFont **font)
{
    D3DXFONT_DESCA desc;

    if( !device || !font ) return D3DERR_INVALIDCALL;

    desc.Height=height;
    desc.Width=width;
    desc.Weight=weight;
    desc.MipLevels=miplevels;
    desc.Italic=italic;
    desc.CharSet=charset;
    desc.OutputPrecision=precision;
    desc.Quality=quality;
    desc.PitchAndFamily=pitchandfamily;
    if(facename != NULL) lstrcpyA(desc.FaceName, facename);
    else desc.FaceName[0] = '\0';

    return D3DXCreateFontIndirectA(device, &desc, font);
}

HRESULT WINAPI D3DXCreateFontW(IDirect3DDevice9 *device, INT height, UINT width, UINT weight, UINT miplevels, BOOL italic, DWORD charset,
                               DWORD precision, DWORD quality, DWORD pitchandfamily, const WCHAR *facename, ID3DXFont **font)
{
    D3DXFONT_DESCW desc;

    if( !device || !font ) return D3DERR_INVALIDCALL;

    desc.Height=height;
    desc.Width=width;
    desc.Weight=weight;
    desc.MipLevels=miplevels;
    desc.Italic=italic;
    desc.CharSet=charset;
    desc.OutputPrecision=precision;
    desc.Quality=quality;
    desc.PitchAndFamily=pitchandfamily;
    if(facename != NULL) strcpyW(desc.FaceName, facename);
    else desc.FaceName[0] = '\0';

    return D3DXCreateFontIndirectW(device, &desc, font);
}

/***********************************************************************
 *           D3DXCreateFontIndirectA    (D3DX9_36.@)
 */
HRESULT WINAPI D3DXCreateFontIndirectA(IDirect3DDevice9 *device, const D3DXFONT_DESCA *desc, ID3DXFont **font)
{
    D3DXFONT_DESCW widedesc;

    if( !device || !desc || !font ) return D3DERR_INVALIDCALL;

    /* Copy everything but the last structure member. This requires the
       two D3DXFONT_DESC structures to be equal until the FaceName member */
    memcpy(&widedesc, desc, FIELD_OFFSET(D3DXFONT_DESCA, FaceName));
    MultiByteToWideChar(CP_ACP, 0, desc->FaceName, -1, widedesc.FaceName, ARRAY_SIZE(widedesc.FaceName));
    return D3DXCreateFontIndirectW(device, &widedesc, font);
}

/***********************************************************************
 *           D3DXCreateFontIndirectW    (D3DX9_36.@)
 */
HRESULT WINAPI D3DXCreateFontIndirectW(IDirect3DDevice9 *device, const D3DXFONT_DESCW *desc, ID3DXFont **font)
{
    D3DDEVICE_CREATION_PARAMETERS cpars;
    D3DDISPLAYMODE mode;
    struct d3dx_font *object;
    IDirect3D9 *d3d;
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", device, desc, font);

    if( !device || !desc || !font ) return D3DERR_INVALIDCALL;

    /* the device MUST support D3DFMT_A8R8G8B8 */
    IDirect3DDevice9_GetDirect3D(device, &d3d);
    IDirect3DDevice9_GetCreationParameters(device, &cpars);
    IDirect3DDevice9_GetDisplayMode(device, 0, &mode);
    hr = IDirect3D9_CheckDeviceFormat(d3d, cpars.AdapterOrdinal, cpars.DeviceType, mode.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_A8R8G8B8);
    if(FAILED(hr)) {
        IDirect3D9_Release(d3d);
        return D3DXERR_INVALIDDATA;
    }
    IDirect3D9_Release(d3d);

    object = HeapAlloc(GetProcessHeap(), 0, sizeof(struct d3dx_font));
    if(object==NULL) {
        *font=NULL;
        return E_OUTOFMEMORY;
    }
    object->ID3DXFont_iface.lpVtbl = &D3DXFont_Vtbl;
    object->ref=1;
    object->device=device;
    object->desc=*desc;

    object->hdc = CreateCompatibleDC(NULL);
    if( !object->hdc ) {
        HeapFree(GetProcessHeap(), 0, object);
        return D3DXERR_INVALIDDATA;
    }

    object->hfont = CreateFontW(desc->Height, desc->Width, 0, 0, desc->Weight, desc->Italic, FALSE, FALSE, desc->CharSet,
                                desc->OutputPrecision, CLIP_DEFAULT_PRECIS, desc->Quality, desc->PitchAndFamily, desc->FaceName);
    if( !object->hfont ) {
        DeleteDC(object->hdc);
        HeapFree(GetProcessHeap(), 0, object);
        return D3DXERR_INVALIDDATA;
    }
    SelectObject(object->hdc, object->hfont);

    IDirect3DDevice9_AddRef(device);
    *font=&object->ID3DXFont_iface;

    return D3D_OK;
}
