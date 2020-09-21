#ifdef __REACTOS__
#include "precomp.h"
#else
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


#include "d3dx9_private.h"
#endif /* __REACTOS__ */

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

struct d3dx_glyph
{
    unsigned int id;
    RECT black_box;
    POINT cell_inc;
    IDirect3DTexture9 *texture;

    struct wine_rb_entry entry;
};

struct d3dx_font
{
    ID3DXFont ID3DXFont_iface;
    LONG ref;

    IDirect3DDevice9 *device;
    D3DXFONT_DESCW desc;
    TEXTMETRICW metrics;

    HDC hdc;
    HFONT hfont;

    struct wine_rb_tree glyph_tree;

    IDirect3DTexture9 **textures;
    unsigned int texture_count, texture_pos;

    unsigned int texture_size, glyph_size, glyphs_per_texture;
};

static int glyph_rb_compare(const void *key, const struct wine_rb_entry *entry)
{
    struct d3dx_glyph *glyph = WINE_RB_ENTRY_VALUE(entry, struct d3dx_glyph, entry);
    unsigned int id = (UINT_PTR)key;

    return id - glyph->id;
}

static void glyph_rb_free(struct wine_rb_entry *entry, void *context)
{
    struct d3dx_glyph *glyph = WINE_RB_ENTRY_VALUE(entry, struct d3dx_glyph, entry);

    heap_free(glyph);
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
    struct d3dx_font *font = impl_from_ID3DXFont(iface);
    ULONG ref = InterlockedIncrement(&font->ref);

    TRACE("%p increasing refcount to %u\n", iface, ref);
    return ref;
}

static ULONG WINAPI ID3DXFontImpl_Release(ID3DXFont *iface)
{
    struct d3dx_font *font = impl_from_ID3DXFont(iface);
    ULONG ref = InterlockedDecrement(&font->ref);
    unsigned int i;

    TRACE("%p decreasing refcount to %u\n", iface, ref);

    if (!ref)
    {
        for (i = 0; i < font->texture_count; ++i)
            IDirect3DTexture9_Release(font->textures[i]);

        heap_free(font->textures);

        wine_rb_destroy(&font->glyph_tree, glyph_rb_free, NULL);

        DeleteObject(font->hfont);
        DeleteDC(font->hdc);
        IDirect3DDevice9_Release(font->device);
        heap_free(font);
    }
    return ref;
}

static HRESULT WINAPI ID3DXFontImpl_GetDevice(ID3DXFont *iface, IDirect3DDevice9 **device)
{
    struct d3dx_font *font = impl_from_ID3DXFont(iface);

    TRACE("iface %p, device %p\n", iface, device);

    if( !device ) return D3DERR_INVALIDCALL;
    *device = font->device;
    IDirect3DDevice9_AddRef(font->device);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXFontImpl_GetDescA(ID3DXFont *iface, D3DXFONT_DESCA *desc)
{
    struct d3dx_font *font = impl_from_ID3DXFont(iface);

    TRACE("iface %p, desc %p\n", iface, desc);

    if( !desc ) return D3DERR_INVALIDCALL;
    memcpy(desc, &font->desc, FIELD_OFFSET(D3DXFONT_DESCA, FaceName));
    WideCharToMultiByte(CP_ACP, 0, font->desc.FaceName, -1, desc->FaceName, ARRAY_SIZE(desc->FaceName), NULL, NULL);

    return D3D_OK;
}

static HRESULT WINAPI ID3DXFontImpl_GetDescW(ID3DXFont *iface, D3DXFONT_DESCW *desc)
{
    struct d3dx_font *font = impl_from_ID3DXFont(iface);

    TRACE("iface %p, desc %p\n", iface, desc);

    if( !desc ) return D3DERR_INVALIDCALL;
    *desc = font->desc;

    return D3D_OK;
}

static BOOL WINAPI ID3DXFontImpl_GetTextMetricsA(ID3DXFont *iface, TEXTMETRICA *metrics)
{
    struct d3dx_font *font = impl_from_ID3DXFont(iface);
    TRACE("iface %p, metrics %p\n", iface, metrics);
    return GetTextMetricsA(font->hdc, metrics);
}

static BOOL WINAPI ID3DXFontImpl_GetTextMetricsW(ID3DXFont *iface, TEXTMETRICW *metrics)
{
    struct d3dx_font *font = impl_from_ID3DXFont(iface);
    TRACE("iface %p, metrics %p\n", iface, metrics);
    return GetTextMetricsW(font->hdc, metrics);
}

static HDC WINAPI ID3DXFontImpl_GetDC(ID3DXFont *iface)
{
    struct d3dx_font *font = impl_from_ID3DXFont(iface);
    TRACE("iface %p\n", iface);
    return font->hdc;
}

static HRESULT WINAPI ID3DXFontImpl_GetGlyphData(ID3DXFont *iface, UINT glyph,
        IDirect3DTexture9 **texture, RECT *black_box, POINT *cell_inc)
{
    struct d3dx_font *font = impl_from_ID3DXFont(iface);
    struct wine_rb_entry *entry;
    HRESULT hr;

    TRACE("iface %p, glyph %#x, texture %p, black_box %p, cell_inc %p.\n",
          iface, glyph, texture, black_box, cell_inc);

    hr = ID3DXFont_PreloadGlyphs(iface, glyph, glyph);
    if (FAILED(hr))
        return hr;

    entry = wine_rb_get(&font->glyph_tree, ULongToPtr(glyph));
    if (entry)
    {
        struct d3dx_glyph *current_glyph = WINE_RB_ENTRY_VALUE(entry, struct d3dx_glyph, entry);

        if (cell_inc)
            *cell_inc = current_glyph->cell_inc;
        if (black_box)
            *black_box = current_glyph->black_box;
        if (texture)
        {
            *texture = current_glyph->texture;
            if (*texture)
                IDirect3DTexture9_AddRef(current_glyph->texture);
        }
        return D3D_OK;
    }

    return D3DXERR_INVALIDDATA;
}

static HRESULT WINAPI ID3DXFontImpl_PreloadCharacters(ID3DXFont *iface, UINT first, UINT last)
{
    FIXME("iface %p, first %u, last %u stub!\n", iface, first, last);
    return S_OK;
}

static uint32_t morton_decode(uint32_t x)
{
    x &= 0x55555555;
    x = (x ^ (x >> 1)) & 0x33333333;
    x = (x ^ (x >> 2)) & 0x0f0f0f0f;
    x = (x ^ (x >> 4)) & 0x00ff00ff;
    x = (x ^ (x >> 8)) & 0x0000ffff;
    return x;
}

/* The glyphs are stored in a grid. Cell sizes vary between different font
 * sizes.
 *
 * The grid is filled in Morton order:
 *  1   2   5   6  17  18  21  22
 *  3   4   7   8  19  20  23  24
 *  9  10  13  14  25  26  29  30
 * 11  12  15  16  27  28  31  32
 * 33  34 ...
 * ...
 *
 * i.e. we try to fill one small square, then three equal-sized squares so
 * that we get one big square, etc.
 *
 * The glyphs are positioned around their baseline, which is located at y
 * position glyph_size * i + tmAscent. Concerning the x position, the glyphs
 * are centered around glyph_size * (i + 0.5). */
static HRESULT WINAPI ID3DXFontImpl_PreloadGlyphs(ID3DXFont *iface, UINT first, UINT last)
{
    static const MAT2 mat = { {0,1}, {0,0}, {0,0}, {0,1} };
    struct d3dx_font *font = impl_from_ID3DXFont(iface);
    IDirect3DTexture9 *current_texture = NULL;
    unsigned int size, stride, glyph, x, y;
    struct d3dx_glyph *current_glyph;
    D3DLOCKED_RECT lockrect;
    GLYPHMETRICS metrics;
    BOOL mapped = FALSE;
    DWORD *pixel_data;
    BYTE *buffer;
    HRESULT hr;

    TRACE("iface %p, first %u, last %u.\n", iface, first, last);

    if (last < first)
        return D3D_OK;

    if (font->texture_count)
        current_texture = font->textures[font->texture_count - 1];

    for (glyph = first; glyph <= last; ++glyph)
    {
        if (wine_rb_get(&font->glyph_tree, ULongToPtr(glyph)))
            continue;

        current_glyph = heap_alloc(sizeof(*current_glyph));
        if (!current_glyph)
        {
            if (mapped)
                IDirect3DTexture9_UnlockRect(current_texture, 0);
            return E_OUTOFMEMORY;
        }

        current_glyph->id = glyph;
        current_glyph->texture = NULL;
        wine_rb_put(&font->glyph_tree, ULongToPtr(current_glyph->id), &current_glyph->entry);

        size = GetGlyphOutlineW(font->hdc, glyph, GGO_GLYPH_INDEX | GGO_GRAY8_BITMAP, &metrics, 0, NULL, &mat);
        if (size == GDI_ERROR)
        {
            WARN("GetGlyphOutlineW failed.\n");
            continue;
        }
        if (!size)
            continue;

        buffer = heap_alloc(size);
        if (!buffer)
        {
            if (mapped)
                IDirect3DTexture9_UnlockRect(current_texture, 0);
            return E_OUTOFMEMORY;
        }

        GetGlyphOutlineW(font->hdc, glyph, GGO_GLYPH_INDEX | GGO_GRAY8_BITMAP, &metrics, size, buffer, &mat);

        if (font->texture_pos == font->glyphs_per_texture)
        {
            unsigned int new_texture_count = font->texture_count + 1;
            IDirect3DTexture9 **new_textures;

            if (mapped)
                IDirect3DTexture9_UnlockRect(current_texture, 0);
            mapped = FALSE;
            new_textures = heap_realloc(font->textures, new_texture_count * sizeof(*new_textures));
            if (!new_textures)
            {
                heap_free(buffer);
                return E_OUTOFMEMORY;
            }
            font->textures = new_textures;

            if (FAILED(hr = IDirect3DDevice9_CreateTexture(font->device, font->texture_size,
                    font->texture_size, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
                    &font->textures[font->texture_count], NULL)))
            {
                heap_free(buffer);
                return hr;
            }

            current_texture = font->textures[font->texture_count++];
            font->texture_pos = 0;
        }

        if (!mapped)
        {
            if (FAILED(hr = IDirect3DTexture9_LockRect(current_texture, 0, &lockrect, NULL, 0)))
            {
                heap_free(buffer);
                return hr;
            }
            mapped = TRUE;
        }

        x = morton_decode(font->texture_pos) * font->glyph_size;
        y = morton_decode(font->texture_pos >> 1) * font->glyph_size;

        current_glyph->black_box.left = x - metrics.gmptGlyphOrigin.x + font->glyph_size / 2
                - metrics.gmBlackBoxX / 2;
        current_glyph->black_box.top = y - metrics.gmptGlyphOrigin.y + font->metrics.tmAscent + 1;
        current_glyph->black_box.right = current_glyph->black_box.left + metrics.gmBlackBoxX;
        current_glyph->black_box.bottom = current_glyph->black_box.top + metrics.gmBlackBoxY;
        current_glyph->cell_inc.x = metrics.gmptGlyphOrigin.x - 1;
        current_glyph->cell_inc.y = font->metrics.tmAscent - metrics.gmptGlyphOrigin.y - 1;
        current_glyph->texture = current_texture;

        pixel_data = lockrect.pBits;
        stride = (metrics.gmBlackBoxX + 3) & ~3;
        for (y = 0; y < metrics.gmBlackBoxY; ++y)
            for (x = 0; x < metrics.gmBlackBoxX; ++x)
                pixel_data[(current_glyph->black_box.top + y) * lockrect.Pitch / 4
                        + current_glyph->black_box.left + x] =
                        (buffer[y * stride + x] * 255 / 64 << 24) | 0x00ffffffu;

        heap_free(buffer);
        ++font->texture_pos;
    }
    if (mapped)
        IDirect3DTexture9_UnlockRect(current_texture, 0);

    return D3D_OK;
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
    if(facename != NULL) lstrcpyW(desc.FaceName, facename);
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
    struct d3dx_font *object;
    D3DDISPLAYMODE mode;
    IDirect3D9 *d3d;
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", device, desc, font);

    if( !device || !desc || !font ) return D3DERR_INVALIDCALL;

    /* the device MUST support D3DFMT_A8R8G8B8 */
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

    object = heap_alloc_zero(sizeof(*object));
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
        heap_free(object);
        return D3DXERR_INVALIDDATA;
    }

    object->hfont = CreateFontW(desc->Height, desc->Width, 0, 0, desc->Weight, desc->Italic, FALSE, FALSE, desc->CharSet,
                                desc->OutputPrecision, CLIP_DEFAULT_PRECIS, desc->Quality, desc->PitchAndFamily, desc->FaceName);
    if (!object->hfont)
    {
        DeleteDC(object->hdc);
        heap_free(object);
        return D3DXERR_INVALIDDATA;
    }
    SelectObject(object->hdc, object->hfont);

    wine_rb_init(&object->glyph_tree, glyph_rb_compare);

    if (!GetTextMetricsW(object->hdc, &object->metrics))
    {
        DeleteObject(object->hfont);
        DeleteDC(object->hdc);
        heap_free(object);
        return D3DXERR_INVALIDDATA;
    }

    object->glyph_size = make_pow2(object->metrics.tmHeight);

    object->texture_size = object->glyph_size;
    if (object->glyph_size < 256)
        object->texture_size = min(256, object->texture_size * 16);

    object->glyphs_per_texture = object->texture_size * object->texture_size
            / (object->glyph_size * object->glyph_size);
    object->texture_pos = object->glyphs_per_texture;

    IDirect3DDevice9_AddRef(device);
    *font = &object->ID3DXFont_iface;

    return D3D_OK;
}
