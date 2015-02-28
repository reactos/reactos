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

#include "d3dx9_36_private.h"

struct d3dx_font
{
    ID3DXFont ID3DXFont_iface;
    LONG ref;

    IDirect3DDevice9 *device;
    D3DXFONT_DESCW desc;

    HDC hdc;
    HFONT hfont;

    UINT tex_width;
    UINT tex_height;
    IDirect3DTexture9 *texture;
    HBITMAP bitmap;
    BYTE *bits;
};

/* Returns the smallest power of 2 which is greater than or equal to num */
static UINT make_pow2(UINT num)
{
    UINT result = 1;

    /* In the unlikely event somebody passes a large value, make sure we don't enter an infinite loop */
    if (num >= 0x80000000)
        return 0x80000000;

    while (result < num)
        result <<= 1;

    return result;
}

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
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %u\n", iface, ref);

    return ref;
}

static ULONG WINAPI ID3DXFontImpl_Release(ID3DXFont *iface)
{
    struct d3dx_font *This = impl_from_ID3DXFont(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u\n", iface, ref);

    if (!ref)
    {
        if (This->texture)
        {
            IDirect3DTexture9_Release(This->texture);
            DeleteObject(This->bitmap);
        }
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
    WideCharToMultiByte(CP_ACP, 0, This->desc.FaceName, -1, desc->FaceName, sizeof(desc->FaceName) / sizeof(CHAR), NULL, NULL);

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
    LPWSTR stringW;
    INT countW, ret = 0;

    TRACE("iface %p, sprite %p, string %s, count %d, rect %s, format %#x, color 0x%08x\n",
            iface,  sprite, debugstr_a(string), count, wine_dbgstr_rect(rect), format, color);

    if (!string || !count)
        return 0;

    countW = MultiByteToWideChar(CP_ACP, 0, string, count, NULL, 0);
    stringW = HeapAlloc(GetProcessHeap(), 0, countW * sizeof(WCHAR));
    if (stringW)
    {
        MultiByteToWideChar(CP_ACP, 0, string, count, stringW, countW);
        ret = ID3DXFont_DrawTextW(iface, sprite, stringW, countW, rect, format, color);
        HeapFree(GetProcessHeap(), 0, stringW);
    }

    return ret;
}

static INT WINAPI ID3DXFontImpl_DrawTextW(ID3DXFont *iface, ID3DXSprite *sprite,
        const WCHAR *string, INT count, RECT *rect, DWORD format, D3DCOLOR color)
{
    struct d3dx_font *This = impl_from_ID3DXFont(iface);
    RECT calc_rect = *rect;
    INT height;

    TRACE("iface %p, sprite %p, string %s, count %d, rect %s, format %#x, color 0x%08x\n",
            iface,  sprite, debugstr_w(string), count, wine_dbgstr_rect(rect), format, color);

    if (!string || !count)
        return 0;

    /* Strip terminating NULL characters */
    while (count > 0 && !string[count-1])
        count--;

    height = DrawTextW(This->hdc, string, count, &calc_rect, format | DT_CALCRECT);

    if (format & DT_CALCRECT)
    {
        *rect = calc_rect;
        return height;
    }

    if (format & DT_CENTER)
    {
        UINT new_width = calc_rect.right - calc_rect.left;
        calc_rect.left = (rect->right + rect->left - new_width) / 2;
        calc_rect.right = calc_rect.left + new_width;
    }

    if (height && (calc_rect.left < calc_rect.right))
    {
        D3DLOCKED_RECT locked_rect;
        D3DXVECTOR3 position;
        UINT text_width, text_height;
        RECT text_rect;
        ID3DXSprite *target = sprite;
        HRESULT hr;
        int i, j;

        /* Get rect position and dimensions */
        position.x = calc_rect.left;
        position.y = calc_rect.top;
        position.z = 0;
        text_width = calc_rect.right - calc_rect.left;
        text_height = calc_rect.bottom - calc_rect.top;
        text_rect.left = 0;
        text_rect.top = 0;
        text_rect.right = text_width;
        text_rect.bottom = text_height;

        /* We need to flush as it seems all draws in the begin/end sequence use only the latest updated texture */
        if (sprite)
            ID3DXSprite_Flush(sprite);

        /* Extend texture and DIB section to contain text */
        if ((text_width > This->tex_width) || (text_height > This->tex_height))
        {
            BITMAPINFOHEADER header;

            if (text_width > This->tex_width)
                This->tex_width = make_pow2(text_width);
            if (text_height > This->tex_height)
                This->tex_height = make_pow2(text_height);

            if (This->texture)
            {
                IDirect3DTexture9_Release(This->texture);
                DeleteObject(This->bitmap);
            }

            hr = D3DXCreateTexture(This->device, This->tex_width, This->tex_height, 1, 0,
                                   D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &This->texture);
            if (FAILED(hr))
            {
                This->texture = NULL;
                return 0;
            }

            header.biSize = sizeof(header);
            header.biWidth = This->tex_width;
            header.biHeight = -This->tex_height;
            header.biPlanes = 1;
            header.biBitCount = 32;
            header.biCompression = BI_RGB;
            header.biSizeImage = sizeof(DWORD) * This->tex_width * This->tex_height;
            header.biXPelsPerMeter = 0;
            header.biYPelsPerMeter = 0;
            header.biClrUsed = 0;
            header.biClrImportant = 0;

            This->bitmap = CreateDIBSection(This->hdc, (const BITMAPINFO*)&header,
                                            DIB_RGB_COLORS, (void**)&This->bits, NULL, 0);
            if (!This->bitmap)
            {
                IDirect3DTexture9_Release(This->texture);
                This->texture = NULL;
                return 0;
            }

            SelectObject(This->hdc, This->bitmap);
        }

        if (FAILED(IDirect3DTexture9_LockRect(This->texture, 0, &locked_rect, &text_rect, D3DLOCK_DISCARD)))
            return 0;

        /* Clear rect */
        for (i = 0; i < text_height; i++)
            memset(This->bits + i * This->tex_width * sizeof(DWORD), 0,
                   text_width * sizeof(DWORD));

        DrawTextW(This->hdc, string, count, &text_rect, format);

        /* All RGB components are equal so take one as alpha and set RGB
         * color to white, so it can be modulated with color parameter */
        for (i = 0; i < text_height; i++)
        {
            DWORD *src = (DWORD *)This->bits + i * This->tex_width;
            DWORD *dst = (DWORD *)((BYTE *)locked_rect.pBits + i * locked_rect.Pitch);
            for (j = 0; j < text_width; j++)
            {
                *dst++ = (*src++ << 24) | 0xFFFFFF;
            }
        }

        IDirect3DTexture9_UnlockRect(This->texture, 0);

        if (!sprite)
        {
            hr = D3DXCreateSprite(This->device, &target);
            if (FAILED(hr))
                 return 0;
            ID3DXSprite_Begin(target, 0);
        }

        hr = target->lpVtbl->Draw(target, This->texture, &text_rect, NULL, &position, color);

        if (!sprite)
        {
            ID3DXSprite_End(target);
            ID3DXSprite_Release(target);
        }

        if (FAILED(hr))
            return 0;
    }

    return height;
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
    MultiByteToWideChar(CP_ACP, 0, desc->FaceName, -1,
                        widedesc.FaceName, sizeof(widedesc.FaceName)/sizeof(WCHAR));
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

    if (!device || !desc || !font) return D3DERR_INVALIDCALL;

    TRACE("desc: %d %d %d %d %d %d %d %d %d %s\n", desc->Height, desc->Width, desc->Weight, desc->MipLevels, desc->Italic,
            desc->CharSet, desc->OutputPrecision, desc->Quality, desc->PitchAndFamily, debugstr_w(desc->FaceName));

    /* The device MUST support D3DFMT_A8R8G8B8 */
    IDirect3DDevice9_GetDirect3D(device, &d3d);
    IDirect3DDevice9_GetCreationParameters(device, &cpars);
    IDirect3DDevice9_GetDisplayMode(device, 0, &mode);
    hr = IDirect3D9_CheckDeviceFormat(d3d, cpars.AdapterOrdinal, cpars.DeviceType, mode.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_A8R8G8B8);
    if (FAILED(hr))
    {
        IDirect3D9_Release(d3d);
        return D3DXERR_INVALIDDATA;
    }
    IDirect3D9_Release(d3d);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct d3dx_font));
    if (!object)
    {
        *font = NULL;
        return E_OUTOFMEMORY;
    }
    object->ID3DXFont_iface.lpVtbl = &D3DXFont_Vtbl;
    object->ref = 1;
    object->device = device;
    object->desc = *desc;

    object->hdc = CreateCompatibleDC(NULL);
    if (!object->hdc)
    {
        HeapFree(GetProcessHeap(), 0, object);
        return D3DXERR_INVALIDDATA;
    }

    object->hfont = CreateFontW(desc->Height, desc->Width, 0, 0, desc->Weight, desc->Italic, FALSE, FALSE, desc->CharSet,
                                desc->OutputPrecision, CLIP_DEFAULT_PRECIS, desc->Quality, desc->PitchAndFamily, desc->FaceName);
    if (!object->hfont)
    {
        DeleteDC(object->hdc);
        HeapFree(GetProcessHeap(), 0, object);
        return D3DXERR_INVALIDDATA;
    }
    SelectObject(object->hdc, object->hfont);
    SetTextColor(object->hdc, 0x00ffffff);
    SetBkColor(object->hdc, 0x00000000);

    IDirect3DDevice9_AddRef(device);
    *font = &object->ID3DXFont_iface;

    return D3D_OK;
}
