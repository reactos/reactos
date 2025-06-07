/*
 * RichEdit GUIDs and OLE interface
 *
 * Copyright 2004 by Krzysztof Foltman
 * Copyright 2004 Aric Stewart
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
#include "wingdi.h"
#include "winuser.h"
#include "ole2.h"
#include "richole.h"
#include "editor.h"
#include "richedit.h"
#include "tom.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(richedit);

/* there is no way to be consistent across different sets of headers - mingw, Wine, Win32 SDK*/

#include "initguid.h"

DEFINE_GUID(LIBID_tom, 0x8cc497c9, 0xa1df, 0x11ce, 0x80, 0x98, 0x00, 0xaa, 0x00, 0x47, 0xbe, 0x5d);
DEFINE_GUID(IID_ITextServices, 0x8d33f740, 0xcf58, 0x11ce, 0xa8, 0x9d, 0x00, 0xaa, 0x00, 0x6c, 0xad, 0xc5);
DEFINE_GUID(IID_ITextHost, 0x13e670f4,0x1a5a,0x11cf,0xab,0xeb,0x00,0xaa,0x00,0xb6,0x5e,0xa1);
DEFINE_GUID(IID_ITextHost2, 0x13e670f5,0x1a5a,0x11cf,0xab,0xeb,0x00,0xaa,0x00,0xb6,0x5e,0xa1);
DEFINE_GUID(IID_ITextDocument, 0x8cc497c0, 0xa1df, 0x11ce, 0x80, 0x98, 0x00, 0xaa, 0x00, 0x47, 0xbe, 0x5d);
DEFINE_GUID(IID_ITextDocument2Old, 0x01c25500, 0x4268, 0x11d1, 0x88, 0x3a, 0x3c, 0x8b, 0x00, 0xc1, 0x00, 0x00);
DEFINE_GUID(IID_ITextRange, 0x8cc497c2, 0xa1df, 0x11ce, 0x80, 0x98, 0x00, 0xaa, 0x00, 0x47, 0xbe, 0x5d);
DEFINE_GUID(IID_ITextSelection, 0x8cc497c1, 0xa1df, 0x11ce, 0x80, 0x98, 0x00, 0xaa, 0x00, 0x47, 0xbe, 0x5d);
DEFINE_GUID(IID_ITextFont, 0x8cc497c3, 0xa1df, 0x11ce, 0x80, 0x98, 0x00, 0xaa, 0x00, 0x47, 0xbe, 0x5d);
DEFINE_GUID(IID_ITextPara, 0x8cc497c4, 0xa1df, 0x11ce, 0x80, 0x98, 0x00, 0xaa, 0x00, 0x47, 0xbe, 0x5d);

static ITypeLib *typelib;

enum tid_t {
    NULL_tid,
    ITextDocument_tid,
    ITextRange_tid,
    ITextSelection_tid,
    ITextFont_tid,
    ITextPara_tid,
    LAST_tid
};

static const IID * const tid_ids[] =
{
    &IID_NULL,
    &IID_ITextDocument,
    &IID_ITextRange,
    &IID_ITextSelection,
    &IID_ITextFont,
    &IID_ITextPara,
};
static ITypeInfo *typeinfos[LAST_tid];

static HRESULT load_typelib(void)
{
    ITypeLib *tl;
    HRESULT hr;

    hr = LoadRegTypeLib(&LIBID_tom, 1, 0, LOCALE_SYSTEM_DEFAULT, &tl);
    if (FAILED(hr)) {
        ERR("LoadRegTypeLib failed: %08lx\n", hr);
        return hr;
    }

    if (InterlockedCompareExchangePointer((void**)&typelib, tl, NULL))
        ITypeLib_Release(tl);
    return hr;
}

void release_typelib(void)
{
    unsigned i;

    if (!typelib)
        return;

    for (i = 0; i < ARRAY_SIZE(typeinfos); i++)
        if (typeinfos[i])
            ITypeInfo_Release(typeinfos[i]);

    ITypeLib_Release(typelib);
}

static HRESULT get_typeinfo(enum tid_t tid, ITypeInfo **typeinfo)
{
    HRESULT hr;

    if (!typelib)
        hr = load_typelib();
    if (!typelib)
        return hr;

    if (!typeinfos[tid])
    {
        ITypeInfo *ti;

        hr = ITypeLib_GetTypeInfoOfGuid(typelib, tid_ids[tid], &ti);
        if (FAILED(hr))
        {
            ERR("GetTypeInfoOfGuid(%s) failed: %08lx\n", debugstr_guid(tid_ids[tid]), hr);
            return hr;
        }

        if (InterlockedCompareExchangePointer((void**)(typeinfos+tid), ti, NULL))
            ITypeInfo_Release(ti);
    }

    *typeinfo = typeinfos[tid];
    return S_OK;
}

/* private IID used to get back IRichEditOleImpl pointer */
DEFINE_GUID(IID_Igetrichole, 0xe3ce5c7a, 0x8247, 0x4622, 0x81, 0xad, 0x11, 0x81, 0x02, 0xaa, 0x01, 0x30);

typedef struct IOleClientSiteImpl IOleClientSiteImpl;
typedef struct ITextRangeImpl ITextRangeImpl;

enum textfont_prop_id {
    FONT_ALLCAPS = 0,
    FONT_ANIMATION,
    FONT_BACKCOLOR,
    FONT_BOLD,
    FONT_EMBOSS,
    FONT_FORECOLOR,
    FONT_HIDDEN,
    FONT_ENGRAVE,
    FONT_ITALIC,
    FONT_KERNING,
    FONT_LANGID,
    FONT_NAME,
    FONT_OUTLINE,
    FONT_POSITION,
    FONT_PROTECTED,
    FONT_SHADOW,
    FONT_SIZE,
    FONT_SMALLCAPS,
    FONT_SPACING,
    FONT_STRIKETHROUGH,
    FONT_SUBSCRIPT,
    FONT_SUPERSCRIPT,
    FONT_UNDERLINE,
    FONT_WEIGHT,
    FONT_PROPID_LAST,
    FONT_PROPID_FIRST = FONT_ALLCAPS
};

static const DWORD textfont_prop_masks[][2] = {
    { CFM_ALLCAPS,     CFE_ALLCAPS },
    { CFM_ANIMATION },
    { CFM_BACKCOLOR,   CFE_AUTOBACKCOLOR },
    { CFM_BOLD,        CFE_BOLD },
    { CFM_EMBOSS,      CFE_EMBOSS },
    { CFM_COLOR,       CFE_AUTOCOLOR },
    { CFM_HIDDEN,      CFE_HIDDEN },
    { CFM_IMPRINT,     CFE_IMPRINT },
    { CFM_ITALIC,      CFE_ITALIC },
    { CFM_KERNING },
    { CFM_LCID },
    { CFM_FACE },
    { CFM_OUTLINE,     CFE_OUTLINE },
    { CFM_OFFSET },
    { CFM_PROTECTED,   CFE_PROTECTED },
    { CFM_SHADOW,      CFE_SHADOW },
    { CFM_SIZE },
    { CFM_SMALLCAPS,   CFE_SMALLCAPS },
    { CFM_SPACING },
    { CFM_STRIKEOUT,   CFE_STRIKEOUT },
    { CFM_SUBSCRIPT,   CFE_SUBSCRIPT },
    { CFM_SUPERSCRIPT, CFE_SUPERSCRIPT },
    { CFM_UNDERLINE,   CFE_UNDERLINE },
    { CFM_WEIGHT }
};

typedef union {
    FLOAT f;
    LONG  l;
    BSTR  str;
} textfont_prop_val;

enum range_update_op {
    RANGE_UPDATE_DELETE
};

struct reole_child {
    struct list entry;
    struct text_services *reole;
};

struct ITextRangeImpl {
    struct reole_child child;
    ITextRange ITextRange_iface;
    LONG ref;
    LONG start, end;
};

typedef struct ITextFontImpl {
    ITextFont ITextFont_iface;
    LONG ref;

    ITextRange *range;
    textfont_prop_val props[FONT_PROPID_LAST];
    BOOL get_cache_enabled;
    BOOL set_cache_enabled;
} ITextFontImpl;

typedef struct ITextParaImpl {
    ITextPara ITextPara_iface;
    LONG ref;

    ITextRange *range;
} ITextParaImpl;

struct IOleClientSiteImpl {
    struct reole_child child;
    IOleClientSite IOleClientSite_iface;
    IOleInPlaceSite IOleInPlaceSite_iface;
    LONG ref;
};

static inline struct text_services *impl_from_IRichEditOle( IRichEditOle *iface )
{
    return CONTAINING_RECORD( iface, struct text_services, IRichEditOle_iface );
}

static inline struct text_services *impl_from_ITextDocument2Old( ITextDocument2Old *iface )
{
    return CONTAINING_RECORD( iface, struct text_services, ITextDocument2Old_iface );
}

static inline IOleClientSiteImpl *impl_from_IOleInPlaceSite(IOleInPlaceSite *iface)
{
    return CONTAINING_RECORD(iface, IOleClientSiteImpl, IOleInPlaceSite_iface);
}

static inline ITextRangeImpl *impl_from_ITextRange(ITextRange *iface)
{
    return CONTAINING_RECORD(iface, ITextRangeImpl, ITextRange_iface);
}

static inline struct text_selection *impl_from_ITextSelection(ITextSelection *iface)
{
    return CONTAINING_RECORD(iface, struct text_selection, ITextSelection_iface);
}

static inline ITextFontImpl *impl_from_ITextFont(ITextFont *iface)
{
    return CONTAINING_RECORD(iface, ITextFontImpl, ITextFont_iface);
}

static inline ITextParaImpl *impl_from_ITextPara(ITextPara *iface)
{
    return CONTAINING_RECORD(iface, ITextParaImpl, ITextPara_iface);
}

static HRESULT create_textfont(ITextRange*, const ITextFontImpl*, ITextFont**);
static HRESULT create_textpara(ITextRange*, ITextPara**);
static struct text_selection *text_selection_create( struct text_services * );

static HRESULT textrange_get_storylength(ME_TextEditor *editor, LONG *length)
{
    if (!length)
        return E_INVALIDARG;

    *length = ME_GetTextLength(editor) + 1;
    return S_OK;
}

static void textranges_update_ranges(struct text_services *services, LONG start, LONG end, enum range_update_op op)
{
    ITextRangeImpl *range;

    LIST_FOR_EACH_ENTRY(range, &services->rangelist, ITextRangeImpl, child.entry) {
        switch (op)
        {
        case RANGE_UPDATE_DELETE:
            /* range fully covered by deleted range - collapse to insertion point */
            if (range->start >= start && range->end <= end)
                range->start = range->end = start;
            /* deleted range cuts from the right */
            else if (range->start < start && range->end <= end)
                range->end = start;
            /* deleted range cuts from the left */
            else if (range->start >= start && range->end > end) {
                range->start = start;
                range->end -= end - start;
            }
            /* deleted range cuts within */
            else
                range->end -= end - start;
            break;
        default:
            FIXME("unknown update op, %d\n", op);
        }
    }
}

static inline BOOL is_equal_textfont_prop_value(enum textfont_prop_id propid, textfont_prop_val *left,
    textfont_prop_val *right)
{
    switch (propid)
    {
    case FONT_ALLCAPS:
    case FONT_ANIMATION:
    case FONT_BACKCOLOR:
    case FONT_BOLD:
    case FONT_EMBOSS:
    case FONT_FORECOLOR:
    case FONT_HIDDEN:
    case FONT_ENGRAVE:
    case FONT_ITALIC:
    case FONT_KERNING:
    case FONT_LANGID:
    case FONT_OUTLINE:
    case FONT_PROTECTED:
    case FONT_SHADOW:
    case FONT_SMALLCAPS:
    case FONT_STRIKETHROUGH:
    case FONT_SUBSCRIPT:
    case FONT_SUPERSCRIPT:
    case FONT_UNDERLINE:
    case FONT_WEIGHT:
        return left->l == right->l;
    case FONT_NAME:
        return !wcscmp(left->str, right->str);
    case FONT_POSITION:
    case FONT_SIZE:
    case FONT_SPACING:
       return left->f == right->f;
    default:
        FIXME("unhandled font property %d\n", propid);
        return FALSE;
    }
}

static inline void init_textfont_prop_value(enum textfont_prop_id propid, textfont_prop_val *v)
{
    switch (propid)
    {
    case FONT_ALLCAPS:
    case FONT_ANIMATION:
    case FONT_BACKCOLOR:
    case FONT_BOLD:
    case FONT_EMBOSS:
    case FONT_FORECOLOR:
    case FONT_HIDDEN:
    case FONT_ENGRAVE:
    case FONT_ITALIC:
    case FONT_KERNING:
    case FONT_LANGID:
    case FONT_OUTLINE:
    case FONT_PROTECTED:
    case FONT_SHADOW:
    case FONT_SMALLCAPS:
    case FONT_STRIKETHROUGH:
    case FONT_SUBSCRIPT:
    case FONT_SUPERSCRIPT:
    case FONT_UNDERLINE:
    case FONT_WEIGHT:
        v->l = tomUndefined;
        return;
    case FONT_NAME:
        v->str = NULL;
        return;
    case FONT_POSITION:
    case FONT_SIZE:
    case FONT_SPACING:
        v->f = tomUndefined;
        return;
    default:
        FIXME("unhandled font property %d\n", propid);
        v->l = tomUndefined;
        return;
    }
}

static inline FLOAT twips_to_points(LONG value)
{
    return value * 72.0 / 1440;
}

static inline FLOAT points_to_twips(FLOAT value)
{
    return value * 1440 / 72.0;
}

static HRESULT get_textfont_prop_for_pos(const struct text_services *services, int pos, enum textfont_prop_id propid,
    textfont_prop_val *value)
{
    ME_Cursor from, to;
    CHARFORMAT2W fmt;

    memset(&fmt, 0, sizeof(fmt));
    fmt.cbSize = sizeof(fmt);
    fmt.dwMask = textfont_prop_masks[propid][0];

    cursor_from_char_ofs( services->editor, pos, &from );
    to = from;
    ME_MoveCursorChars( services->editor, &to, 1, FALSE );
    ME_GetCharFormat( services->editor, &from, &to, &fmt );

    switch (propid)
    {
    case FONT_ALLCAPS:
    case FONT_BOLD:
    case FONT_EMBOSS:
    case FONT_HIDDEN:
    case FONT_ENGRAVE:
    case FONT_ITALIC:
    case FONT_OUTLINE:
    case FONT_PROTECTED:
    case FONT_SHADOW:
    case FONT_SMALLCAPS:
    case FONT_STRIKETHROUGH:
    case FONT_SUBSCRIPT:
    case FONT_SUPERSCRIPT:
    case FONT_UNDERLINE:
        value->l = fmt.dwEffects & textfont_prop_masks[propid][1] ? tomTrue : tomFalse;
        break;
    case FONT_ANIMATION:
        value->l = fmt.bAnimation;
        break;
    case FONT_BACKCOLOR:
        value->l = fmt.dwEffects & CFE_AUTOBACKCOLOR ? GetSysColor(COLOR_WINDOW) : fmt.crBackColor;
        break;
    case FONT_FORECOLOR:
        value->l = fmt.dwEffects & CFE_AUTOCOLOR ? GetSysColor(COLOR_WINDOWTEXT) : fmt.crTextColor;
        break;
    case FONT_KERNING:
        value->f = twips_to_points(fmt.wKerning);
        break;
    case FONT_LANGID:
        value->l = fmt.lcid;
        break;
    case FONT_NAME:
        /* this case is used exclusively by GetName() */
        value->str = SysAllocString(fmt.szFaceName);
        if (!value->str)
            return E_OUTOFMEMORY;
        break;
    case FONT_POSITION:
        value->f = twips_to_points(fmt.yOffset);
        break;
    case FONT_SIZE:
        value->f = twips_to_points(fmt.yHeight);
        break;
    case FONT_SPACING:
        value->f = fmt.sSpacing;
        break;
    case FONT_WEIGHT:
        value->l = fmt.wWeight;
        break;
    default:
        FIXME("unhandled font property %d\n", propid);
        return E_FAIL;
    }

    return S_OK;
}

static inline const struct text_services *get_range_reole(ITextRange *range)
{
    struct text_services *services = NULL;
    ITextRange_QueryInterface(range, &IID_Igetrichole, (void**)&services);
    return services;
}

static void textrange_set_font(ITextRange *range, ITextFont *font)
{
    CHARFORMAT2W fmt;
    HRESULT hr;
    LONG value;
    BSTR str;
    FLOAT f;

#define CHARFORMAT_SET_B_FIELD(mask, value) \
    if (hr == S_OK && value != tomUndefined) { \
        fmt.dwMask |= CFM_##mask; \
        if (value == tomTrue) fmt.dwEffects |= CFE_##mask; \
    } \

    /* fill format data from font */
    memset(&fmt, 0, sizeof(fmt));
    fmt.cbSize = sizeof(fmt);

    value = tomUndefined;
    hr = ITextFont_GetAllCaps(font, &value);
    CHARFORMAT_SET_B_FIELD(ALLCAPS, value);

    value = tomUndefined;
    hr = ITextFont_GetBold(font, &value);
    CHARFORMAT_SET_B_FIELD(BOLD, value);

    value = tomUndefined;
    hr = ITextFont_GetEmboss(font, &value);
    CHARFORMAT_SET_B_FIELD(EMBOSS, value);

    value = tomUndefined;
    hr = ITextFont_GetHidden(font, &value);
    CHARFORMAT_SET_B_FIELD(HIDDEN, value);

    value = tomUndefined;
    hr = ITextFont_GetEngrave(font, &value);
    CHARFORMAT_SET_B_FIELD(IMPRINT, value);

    value = tomUndefined;
    hr = ITextFont_GetItalic(font, &value);
    CHARFORMAT_SET_B_FIELD(ITALIC, value);

    value = tomUndefined;
    hr = ITextFont_GetOutline(font, &value);
    CHARFORMAT_SET_B_FIELD(OUTLINE, value);

    value = tomUndefined;
    hr = ITextFont_GetProtected(font, &value);
    CHARFORMAT_SET_B_FIELD(PROTECTED, value);

    value = tomUndefined;
    hr = ITextFont_GetShadow(font, &value);
    CHARFORMAT_SET_B_FIELD(SHADOW, value);

    value = tomUndefined;
    hr = ITextFont_GetSmallCaps(font, &value);
    CHARFORMAT_SET_B_FIELD(SMALLCAPS, value);

    value = tomUndefined;
    hr = ITextFont_GetStrikeThrough(font, &value);
    CHARFORMAT_SET_B_FIELD(STRIKEOUT, value);

    value = tomUndefined;
    hr = ITextFont_GetSubscript(font, &value);
    CHARFORMAT_SET_B_FIELD(SUBSCRIPT, value);

    value = tomUndefined;
    hr = ITextFont_GetSuperscript(font, &value);
    CHARFORMAT_SET_B_FIELD(SUPERSCRIPT, value);

    value = tomUndefined;
    hr = ITextFont_GetUnderline(font, &value);
    CHARFORMAT_SET_B_FIELD(UNDERLINE, value);

#undef CHARFORMAT_SET_B_FIELD

    value = tomUndefined;
    hr = ITextFont_GetAnimation(font, &value);
    if (hr == S_OK && value != tomUndefined) {
        fmt.dwMask |= CFM_ANIMATION;
        fmt.bAnimation = value;
    }

    value = tomUndefined;
    hr = ITextFont_GetBackColor(font, &value);
    if (hr == S_OK && value != tomUndefined) {
        fmt.dwMask |= CFM_BACKCOLOR;
        if (value == tomAutoColor)
            fmt.dwEffects |= CFE_AUTOBACKCOLOR;
        else
            fmt.crBackColor = value;
    }

    value = tomUndefined;
    hr = ITextFont_GetForeColor(font, &value);
    if (hr == S_OK && value != tomUndefined) {
        fmt.dwMask |= CFM_COLOR;
        if (value == tomAutoColor)
            fmt.dwEffects |= CFE_AUTOCOLOR;
        else
            fmt.crTextColor = value;
    }

    value = tomUndefined;
    hr = ITextFont_GetKerning(font, &f);
    if (hr == S_OK && f != tomUndefined) {
        fmt.dwMask |= CFM_KERNING;
        fmt.wKerning = points_to_twips(f);
    }

    value = tomUndefined;
    hr = ITextFont_GetLanguageID(font, &value);
    if (hr == S_OK && value != tomUndefined) {
        fmt.dwMask |= CFM_LCID;
        fmt.lcid = value;
    }

    if (ITextFont_GetName(font, &str) == S_OK) {
        fmt.dwMask |= CFM_FACE;
        lstrcpynW(fmt.szFaceName, str, ARRAY_SIZE(fmt.szFaceName));
        SysFreeString(str);
    }

    hr = ITextFont_GetPosition(font, &f);
    if (hr == S_OK && f != tomUndefined) {
        fmt.dwMask |= CFM_OFFSET;
        fmt.yOffset = points_to_twips(f);
    }

    hr = ITextFont_GetSize(font, &f);
    if (hr == S_OK && f != tomUndefined) {
        fmt.dwMask |= CFM_SIZE;
        fmt.yHeight = points_to_twips(f);
    }

    hr = ITextFont_GetSpacing(font, &f);
    if (hr == S_OK && f != tomUndefined) {
        fmt.dwMask |= CFM_SPACING;
        fmt.sSpacing = f;
    }

    hr = ITextFont_GetWeight(font, &value);
    if (hr == S_OK && value != tomUndefined) {
        fmt.dwMask |= CFM_WEIGHT;
        fmt.wWeight = value;
    }

    if (fmt.dwMask)
    {
        const struct text_services *services = get_range_reole(range);
        ME_Cursor from, to;
        LONG start, end;

        ITextRange_GetStart(range, &start);
        ITextRange_GetEnd(range, &end);

        cursor_from_char_ofs( services->editor, start, &from );
        cursor_from_char_ofs( services->editor, end, &to );
        ME_SetCharFormat( services->editor, &from, &to, &fmt );
        ME_CommitUndo( services->editor );
        ME_WrapMarkedParagraphs( services->editor );
        ME_UpdateScrollBar( services->editor );
    }
}

static HRESULT get_textfont_prop(const ITextFontImpl *font, enum textfont_prop_id propid, textfont_prop_val *value)
{
    const struct text_services *services;
    textfont_prop_val v;
    LONG start, end, i;
    HRESULT hr;

    /* when font is not attached to any range use cached values */
    if (!font->range || font->get_cache_enabled) {
        *value = font->props[propid];
        return S_OK;
    }

    if (!(services = get_range_reole(font->range)))
        return CO_E_RELEASED;

    init_textfont_prop_value(propid, value);

    ITextRange_GetStart(font->range, &start);
    ITextRange_GetEnd(font->range, &end);

    /* iterate trough a range to see if property value is consistent */
    hr = get_textfont_prop_for_pos( services, start, propid, &v );
    if (FAILED(hr))
        return hr;

    for (i = start + 1; i < end; i++) {
        textfont_prop_val cur;

        hr = get_textfont_prop_for_pos( services, i, propid, &cur );
        if (FAILED(hr))
            return hr;

        if (!is_equal_textfont_prop_value(propid, &v, &cur))
            return S_OK;
    }

    *value = v;
    return S_OK;
}

static HRESULT get_textfont_propf(const ITextFontImpl *font, enum textfont_prop_id propid, FLOAT *value)
{
    textfont_prop_val v;
    HRESULT hr;

    if (!value)
        return E_INVALIDARG;

    hr = get_textfont_prop(font, propid, &v);
    *value = v.f;
    return hr;
}

static HRESULT get_textfont_propl(const ITextFontImpl *font, enum textfont_prop_id propid, LONG *value)
{
    textfont_prop_val v;
    HRESULT hr;

    if (!value)
        return E_INVALIDARG;

    hr = get_textfont_prop(font, propid, &v);
    *value = v.l;
    return hr;
}

/* Value should already have a terminal value, for boolean properties it means tomToggle is not handled */
static HRESULT set_textfont_prop(ITextFontImpl *font, enum textfont_prop_id propid, const textfont_prop_val *value)
{
    const struct text_services *services;
    ME_Cursor from, to;
    CHARFORMAT2W fmt;
    LONG start, end;

    /* when font is not attached to any range use cache */
    if (!font->range || font->set_cache_enabled) {
        if (propid == FONT_NAME) {
            SysFreeString(font->props[propid].str);
            font->props[propid].str = SysAllocString(value->str);
        }
        else
            font->props[propid] = *value;
        return S_OK;
    }

    if (!(services = get_range_reole(font->range)))
        return CO_E_RELEASED;

    memset(&fmt, 0, sizeof(fmt));
    fmt.cbSize = sizeof(fmt);
    fmt.dwMask = textfont_prop_masks[propid][0];

    switch (propid)
    {
    case FONT_ALLCAPS:
    case FONT_BOLD:
    case FONT_EMBOSS:
    case FONT_HIDDEN:
    case FONT_ENGRAVE:
    case FONT_ITALIC:
    case FONT_OUTLINE:
    case FONT_PROTECTED:
    case FONT_SHADOW:
    case FONT_SMALLCAPS:
    case FONT_STRIKETHROUGH:
    case FONT_SUBSCRIPT:
    case FONT_SUPERSCRIPT:
    case FONT_UNDERLINE:
        fmt.dwEffects = value->l == tomTrue ? textfont_prop_masks[propid][1] : 0;
        break;
    case FONT_ANIMATION:
        fmt.bAnimation = value->l;
        break;
    case FONT_BACKCOLOR:
    case FONT_FORECOLOR:
        if (value->l == tomAutoColor)
            fmt.dwEffects = textfont_prop_masks[propid][1];
        else if (propid == FONT_BACKCOLOR)
            fmt.crBackColor = value->l;
        else
            fmt.crTextColor = value->l;
        break;
    case FONT_KERNING:
        fmt.wKerning = value->f;
        break;
    case FONT_LANGID:
        fmt.lcid = value->l;
        break;
    case FONT_POSITION:
        fmt.yOffset = value->f;
        break;
    case FONT_SIZE:
        fmt.yHeight = value->f;
        break;
    case FONT_SPACING:
        fmt.sSpacing = value->f;
        break;
    case FONT_WEIGHT:
        fmt.wWeight = value->l;
        break;
    case FONT_NAME:
        lstrcpynW(fmt.szFaceName, value->str, ARRAY_SIZE(fmt.szFaceName));
        break;
    default:
        FIXME("unhandled font property %d\n", propid);
        return E_FAIL;
    }

    ITextRange_GetStart(font->range, &start);
    ITextRange_GetEnd(font->range, &end);

    cursor_from_char_ofs( services->editor, start, &from );
    cursor_from_char_ofs( services->editor, end, &to );
    ME_SetCharFormat( services->editor, &from, &to, &fmt );
    ME_CommitUndo( services->editor );
    ME_WrapMarkedParagraphs( services->editor );
    ME_UpdateScrollBar( services->editor );

    return S_OK;
}

static inline HRESULT set_textfont_propl(ITextFontImpl *font, enum textfont_prop_id propid, LONG value)
{
    textfont_prop_val v;
    v.l = value;
    return set_textfont_prop(font, propid, &v);
}

static inline HRESULT set_textfont_propf(ITextFontImpl *font, enum textfont_prop_id propid, FLOAT value)
{
    textfont_prop_val v;
    v.f = value;
    return set_textfont_prop(font, propid, &v);
}

static HRESULT set_textfont_propd(ITextFontImpl *font, enum textfont_prop_id propid, LONG value)
{
    textfont_prop_val v;

    switch (value)
    {
    case tomUndefined:
        return S_OK;
    case tomToggle: {
        LONG oldvalue;
        get_textfont_propl(font, propid, &oldvalue);
        if (oldvalue == tomFalse)
            value = tomTrue;
        else if (oldvalue == tomTrue)
            value = tomFalse;
        else
            return E_INVALIDARG;
        /* fallthrough */
    }
    case tomTrue:
    case tomFalse:
        v.l = value;
        return set_textfont_prop(font, propid, &v);
    default:
        return E_INVALIDARG;
    }
}

static HRESULT textfont_getname_from_range(ITextRange *range, BSTR *ret)
{
    const struct text_services *services;
    textfont_prop_val v;
    HRESULT hr;
    LONG start;

    if (!(services = get_range_reole( range )))
        return CO_E_RELEASED;

    ITextRange_GetStart(range, &start);
    hr = get_textfont_prop_for_pos( services, start, FONT_NAME, &v );
    *ret = v.str;
    return hr;
}

static void textfont_cache_range_props(ITextFontImpl *font)
{
    enum textfont_prop_id propid;
    for (propid = FONT_PROPID_FIRST; propid < FONT_PROPID_LAST; propid++) {
        if (propid == FONT_NAME)
            textfont_getname_from_range(font->range, &font->props[propid].str);
        else
            get_textfont_prop(font, propid, &font->props[propid]);
    }
}

static HRESULT textrange_expand(ITextRange *range, LONG unit, LONG *delta)
{
    LONG expand_start, expand_end;

    switch (unit)
    {
    case tomStory:
        expand_start = 0;
        ITextRange_GetStoryLength(range, &expand_end);
        break;
    default:
        FIXME("unit %ld is not supported\n", unit);
        return E_NOTIMPL;
    }

    if (delta) {
        LONG start, end;

        ITextRange_GetStart(range, &start);
        ITextRange_GetEnd(range, &end);
        *delta = expand_end - expand_start - (end - start);
    }

    ITextRange_SetStart(range, expand_start);
    ITextRange_SetEnd(range, expand_end);

    return S_OK;
}

static HRESULT WINAPI
IRichEditOle_fnQueryInterface(IRichEditOle *iface, REFIID riid, LPVOID *ppvObj)
{
    struct text_services *services = impl_from_IRichEditOle( iface );
    return IUnknown_QueryInterface( services->outer_unk, riid, ppvObj );
}

static ULONG WINAPI
IRichEditOle_fnAddRef(IRichEditOle *iface)
{
    struct text_services *services = impl_from_IRichEditOle( iface );
    return IUnknown_AddRef( services->outer_unk );
}

static ULONG WINAPI
IRichEditOle_fnRelease(IRichEditOle *iface)
{
    struct text_services *services = impl_from_IRichEditOle( iface );
    return IUnknown_Release( services->outer_unk );
}

static HRESULT WINAPI
IRichEditOle_fnActivateAs(IRichEditOle *iface, REFCLSID rclsid, REFCLSID rclsidAs)
{
    struct text_services *services = impl_from_IRichEditOle( iface );
    FIXME( "stub %p\n", services );
    return E_NOTIMPL;
}

static HRESULT WINAPI
IRichEditOle_fnContextSensitiveHelp(IRichEditOle *iface, BOOL fEnterMode)
{
    struct text_services *services = impl_from_IRichEditOle( iface );
    FIXME( "stub %p\n", services );
    return E_NOTIMPL;
}

static HRESULT WINAPI
IRichEditOle_fnConvertObject( IRichEditOle *iface, LONG iob, REFCLSID class, LPCSTR user_type )
{
    struct text_services *services = impl_from_IRichEditOle( iface );
    FIXME( "stub %p\n", services );
    return E_NOTIMPL;
}

static inline IOleClientSiteImpl *impl_from_IOleClientSite(IOleClientSite *iface)
{
    return CONTAINING_RECORD(iface, IOleClientSiteImpl, IOleClientSite_iface);
}

static HRESULT WINAPI
IOleClientSite_fnQueryInterface(IOleClientSite *me, REFIID riid, LPVOID *ppvObj)
{
    IOleClientSiteImpl *This = impl_from_IOleClientSite(me);
    TRACE("%p %s\n", me, debugstr_guid(riid) );

    *ppvObj = NULL;
    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IOleClientSite))
        *ppvObj = me;
    else if (IsEqualGUID(riid, &IID_IOleWindow) ||
             IsEqualGUID(riid, &IID_IOleInPlaceSite))
        *ppvObj = &This->IOleInPlaceSite_iface;
    if (*ppvObj)
    {
        IOleClientSite_AddRef(me);
        return S_OK;
    }
    FIXME("%p: unhandled interface %s\n", me, debugstr_guid(riid) );

    return E_NOINTERFACE;
}

static ULONG WINAPI IOleClientSite_fnAddRef(IOleClientSite *iface)
{
    IOleClientSiteImpl *This = impl_from_IOleClientSite(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%lu)\n", This, ref);
    return ref;
}

static ULONG WINAPI IOleClientSite_fnRelease(IOleClientSite *iface)
{
    IOleClientSiteImpl *This = impl_from_IOleClientSite(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%lu)\n", This, ref);

    if (ref == 0) {
        if (This->child.reole) {
            list_remove(&This->child.entry);
            This->child.reole = NULL;
        }
        free(This);
    }
    return ref;
}

static HRESULT WINAPI IOleClientSite_fnSaveObject(IOleClientSite *iface)
{
    IOleClientSiteImpl *This = impl_from_IOleClientSite(iface);
    if (!This->child.reole)
        return CO_E_RELEASED;

    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI IOleClientSite_fnGetMoniker(IOleClientSite *iface, DWORD dwAssign,
        DWORD dwWhichMoniker, IMoniker **ppmk)
{
    IOleClientSiteImpl *This = impl_from_IOleClientSite(iface);
    if (!This->child.reole)
        return CO_E_RELEASED;

    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI IOleClientSite_fnGetContainer(IOleClientSite *iface,
        IOleContainer **ppContainer)
{
    IOleClientSiteImpl *This = impl_from_IOleClientSite(iface);
    if (!This->child.reole)
        return CO_E_RELEASED;

    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI IOleClientSite_fnShowObject(IOleClientSite *iface)
{
    IOleClientSiteImpl *This = impl_from_IOleClientSite(iface);
    if (!This->child.reole)
        return CO_E_RELEASED;

    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI IOleClientSite_fnOnShowWindow(IOleClientSite *iface, BOOL fShow)
{
    IOleClientSiteImpl *This = impl_from_IOleClientSite(iface);
    if (!This->child.reole)
        return CO_E_RELEASED;

    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI IOleClientSite_fnRequestNewObjectLayout(IOleClientSite *iface)
{
    IOleClientSiteImpl *This = impl_from_IOleClientSite(iface);
    if (!This->child.reole)
        return CO_E_RELEASED;

    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static const IOleClientSiteVtbl ocst = {
    IOleClientSite_fnQueryInterface,
    IOleClientSite_fnAddRef,
    IOleClientSite_fnRelease,
    IOleClientSite_fnSaveObject,
    IOleClientSite_fnGetMoniker,
    IOleClientSite_fnGetContainer,
    IOleClientSite_fnShowObject,
    IOleClientSite_fnOnShowWindow,
    IOleClientSite_fnRequestNewObjectLayout
};

/* IOleInPlaceSite interface */
static HRESULT STDMETHODCALLTYPE IOleInPlaceSite_fnQueryInterface(IOleInPlaceSite *iface, REFIID riid, void **ppvObj)
{
    IOleClientSiteImpl *This = impl_from_IOleInPlaceSite(iface);
    return IOleClientSite_QueryInterface(&This->IOleClientSite_iface, riid, ppvObj);
}

static ULONG STDMETHODCALLTYPE IOleInPlaceSite_fnAddRef(IOleInPlaceSite *iface)
{
    IOleClientSiteImpl *This = impl_from_IOleInPlaceSite(iface);
    return IOleClientSite_AddRef(&This->IOleClientSite_iface);
}

static ULONG STDMETHODCALLTYPE IOleInPlaceSite_fnRelease(IOleInPlaceSite *iface)
{
    IOleClientSiteImpl *This = impl_from_IOleInPlaceSite(iface);
    return IOleClientSite_Release(&This->IOleClientSite_iface);
}

static HRESULT STDMETHODCALLTYPE IOleInPlaceSite_fnGetWindow( IOleInPlaceSite *iface, HWND *window )
{
    IOleClientSiteImpl *This = impl_from_IOleInPlaceSite(iface);

    TRACE( "(%p)->(%p)\n", This, window );

    if (!This->child.reole)
        return CO_E_RELEASED;

    if (!window) return E_INVALIDARG;

    if (!This->child.reole->editor->have_texthost2) return E_NOTIMPL;
    return ITextHost2_TxGetWindow( This->child.reole->editor->texthost, window );
}

static HRESULT STDMETHODCALLTYPE IOleInPlaceSite_fnContextSensitiveHelp(IOleInPlaceSite *iface, BOOL fEnterMode)
{
    IOleClientSiteImpl *This = impl_from_IOleInPlaceSite(iface);
    FIXME("not implemented: (%p)->(%d)\n", This, fEnterMode);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE IOleInPlaceSite_fnCanInPlaceActivate(IOleInPlaceSite *iface)
{
    IOleClientSiteImpl *This = impl_from_IOleInPlaceSite(iface);
    FIXME("not implemented: (%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE IOleInPlaceSite_fnOnInPlaceActivate(IOleInPlaceSite *iface)
{
    IOleClientSiteImpl *This = impl_from_IOleInPlaceSite(iface);
    FIXME("not implemented: (%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE IOleInPlaceSite_fnOnUIActivate(IOleInPlaceSite *iface)
{
    IOleClientSiteImpl *This = impl_from_IOleInPlaceSite(iface);
    FIXME("not implemented: (%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE IOleInPlaceSite_fnGetWindowContext(IOleInPlaceSite *iface, IOleInPlaceFrame **ppFrame,
                                                                    IOleInPlaceUIWindow **ppDoc, LPRECT lprcPosRect,
                                                                    LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    IOleClientSiteImpl *This = impl_from_IOleInPlaceSite(iface);
    FIXME("not implemented: (%p)->(%p %p %p %p %p)\n", This, ppFrame, ppDoc, lprcPosRect, lprcClipRect, lpFrameInfo);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE IOleInPlaceSite_fnScroll(IOleInPlaceSite *iface, SIZE scrollExtent)
{
    IOleClientSiteImpl *This = impl_from_IOleInPlaceSite(iface);
    FIXME("not implemented: (%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE IOleInPlaceSite_fnOnUIDeactivate(IOleInPlaceSite *iface, BOOL fUndoable)
{
    IOleClientSiteImpl *This = impl_from_IOleInPlaceSite(iface);
    FIXME("not implemented: (%p)->(%d)\n", This, fUndoable);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE IOleInPlaceSite_fnOnInPlaceDeactivate(IOleInPlaceSite *iface)
{
    IOleClientSiteImpl *This = impl_from_IOleInPlaceSite(iface);
    FIXME("not implemented: (%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE IOleInPlaceSite_fnDiscardUndoState(IOleInPlaceSite *iface)
{
    IOleClientSiteImpl *This = impl_from_IOleInPlaceSite(iface);
    FIXME("not implemented: (%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE IOleInPlaceSite_fnDeactivateAndUndo(IOleInPlaceSite *iface)
{
    IOleClientSiteImpl *This = impl_from_IOleInPlaceSite(iface);
    FIXME("not implemented: (%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE IOleInPlaceSite_fnOnPosRectChange(IOleInPlaceSite *iface, LPCRECT lprcPosRect)
{
    IOleClientSiteImpl *This = impl_from_IOleInPlaceSite(iface);
    FIXME("not implemented: (%p)->(%p)\n", This, lprcPosRect);
    return E_NOTIMPL;
}

static const IOleInPlaceSiteVtbl olestvt =
{
    IOleInPlaceSite_fnQueryInterface,
    IOleInPlaceSite_fnAddRef,
    IOleInPlaceSite_fnRelease,
    IOleInPlaceSite_fnGetWindow,
    IOleInPlaceSite_fnContextSensitiveHelp,
    IOleInPlaceSite_fnCanInPlaceActivate,
    IOleInPlaceSite_fnOnInPlaceActivate,
    IOleInPlaceSite_fnOnUIActivate,
    IOleInPlaceSite_fnGetWindowContext,
    IOleInPlaceSite_fnScroll,
    IOleInPlaceSite_fnOnUIDeactivate,
    IOleInPlaceSite_fnOnInPlaceDeactivate,
    IOleInPlaceSite_fnDiscardUndoState,
    IOleInPlaceSite_fnDeactivateAndUndo,
    IOleInPlaceSite_fnOnPosRectChange
};

static HRESULT CreateOleClientSite( struct text_services *services, IOleClientSite **ret )
{
    IOleClientSiteImpl *clientSite = malloc(sizeof *clientSite);

    if (!clientSite)
        return E_OUTOFMEMORY;

    clientSite->IOleClientSite_iface.lpVtbl = &ocst;
    clientSite->IOleInPlaceSite_iface.lpVtbl = &olestvt;
    clientSite->ref = 1;
    clientSite->child.reole = services;
    list_add_head( &services->clientsites, &clientSite->child.entry );

    *ret = &clientSite->IOleClientSite_iface;
    return S_OK;
}

static HRESULT WINAPI
IRichEditOle_fnGetClientSite( IRichEditOle *iface, IOleClientSite **clientsite )
{
    struct text_services *services = impl_from_IRichEditOle( iface );

    TRACE("(%p)->(%p)\n", services, clientsite);

    if (!clientsite)
        return E_INVALIDARG;

    return CreateOleClientSite( services, clientsite );
}

static HRESULT WINAPI
IRichEditOle_fnGetClipboardData(IRichEditOle *iface, CHARRANGE *lpchrg,
               DWORD reco, LPDATAOBJECT *lplpdataobj)
{
    struct text_services *services = impl_from_IRichEditOle( iface );
    ME_Cursor start;
    int nChars;

    TRACE("(%p,%p,%ld)\n", services, lpchrg, reco);
    if(!lplpdataobj)
        return E_INVALIDARG;
    if(!lpchrg)
    {
        LONG nFrom, nTo;
        int nStartCur = ME_GetSelectionOfs( services->editor, &nFrom, &nTo );
        start = services->editor->pCursors[nStartCur];
        nChars = nTo - nFrom;
    }
    else
    {
        cursor_from_char_ofs( services->editor, lpchrg->cpMin, &start );
        nChars = lpchrg->cpMax - lpchrg->cpMin;
    }
    return ME_GetDataObject( services->editor, &start, nChars, lplpdataobj );
}

static LONG WINAPI IRichEditOle_fnGetLinkCount(IRichEditOle *iface)
{
    struct text_services *services = impl_from_IRichEditOle( iface );
    FIXME("stub %p\n", services);
    return E_NOTIMPL;
}

static HRESULT WINAPI
IRichEditOle_fnGetObject(IRichEditOle *iface, LONG iob,
               REOBJECT *lpreobject, DWORD dwFlags)
{
    struct text_services *services = impl_from_IRichEditOle( iface );
    struct re_object *reobj = NULL;
    LONG count = 0;

    TRACE("(%p)->(%lx, %p, %lx)\n", services, iob, lpreobject, dwFlags);

    if (!lpreobject || !lpreobject->cbStruct)
        return E_INVALIDARG;

    if (iob == REO_IOB_USE_CP)
    {
        ME_Cursor cursor;

        TRACE("character offset: %ld\n", lpreobject->cp);
        cursor_from_char_ofs( services->editor, lpreobject->cp, &cursor );
        if (!cursor.run->reobj)
            return E_INVALIDARG;
        else
            reobj = cursor.run->reobj;
    }
    else if (iob == REO_IOB_SELECTION)
    {
        ME_Cursor *from, *to;

        ME_GetSelection(services->editor, &from, &to);
        if (!from->run->reobj)
            return E_INVALIDARG;
        else
            reobj = from->run->reobj;
    }
    else
    {
        if (iob < 0 || iob >= IRichEditOle_GetObjectCount( iface ))
            return E_INVALIDARG;
        LIST_FOR_EACH_ENTRY(reobj, &services->editor->reobj_list, struct re_object, entry)
        {
            if (count == iob)
                break;
            count++;
        }
    }
    ME_CopyReObject(lpreobject, &reobj->obj, dwFlags);
    lpreobject->cp = run_char_ofs( reobj->run, 0 );
    return S_OK;
}

static LONG WINAPI
IRichEditOle_fnGetObjectCount( IRichEditOle *iface )
{
    struct text_services *services = impl_from_IRichEditOle( iface );
    TRACE("(%p)\n", services);
    return list_count( &services->editor->reobj_list );
}

static HRESULT WINAPI
IRichEditOle_fnHandsOffStorage(IRichEditOle *iface, LONG iob)
{
    struct text_services *services = impl_from_IRichEditOle( iface );
    FIXME("stub %p\n", services);
    return E_NOTIMPL;
}

static HRESULT WINAPI
IRichEditOle_fnImportDataObject(IRichEditOle *iface, LPDATAOBJECT lpdataobj,
               CLIPFORMAT cf, HGLOBAL hMetaPict)
{
    struct text_services *services = impl_from_IRichEditOle( iface );
    FIXME("stub %p\n", services);
    return E_NOTIMPL;
}

static HRESULT WINAPI
IRichEditOle_fnInPlaceDeactivate(IRichEditOle *iface)
{
    struct text_services *services = impl_from_IRichEditOle( iface );
    FIXME("stub %p\n", services);
    return E_NOTIMPL;
}

static HRESULT WINAPI
IRichEditOle_fnInsertObject(IRichEditOle *iface, REOBJECT *reo)
{
    struct text_services *services = impl_from_IRichEditOle( iface );
    HRESULT hr;

    TRACE("(%p,%p)\n", services, reo);

    if (!reo)
        return E_INVALIDARG;

    if (reo->cbStruct < sizeof(*reo)) return STG_E_INVALIDPARAMETER;

    hr = editor_insert_oleobj(services->editor, reo);
    if (hr != S_OK)
        return hr;

    ME_CommitUndo(services->editor);
    ME_UpdateRepaint(services->editor, FALSE);
    return S_OK;
}

static HRESULT WINAPI IRichEditOle_fnSaveCompleted(IRichEditOle *iface, LONG iob,
               LPSTORAGE lpstg)
{
    struct text_services *services = impl_from_IRichEditOle( iface );
    FIXME("stub %p\n", services);
    return E_NOTIMPL;
}

static HRESULT WINAPI
IRichEditOle_fnSetDvaspect(IRichEditOle *iface, LONG iob, DWORD dvaspect)
{
    struct text_services *services = impl_from_IRichEditOle( iface );
    FIXME("stub %p\n", services);
    return E_NOTIMPL;
}

static HRESULT WINAPI IRichEditOle_fnSetHostNames(IRichEditOle *iface,
               LPCSTR lpstrContainerApp, LPCSTR lpstrContainerObj)
{
    struct text_services *services = impl_from_IRichEditOle( iface );
    FIXME("stub %p %s %s\n", services, lpstrContainerApp, lpstrContainerObj);
    return E_NOTIMPL;
}

static HRESULT WINAPI
IRichEditOle_fnSetLinkAvailable(IRichEditOle *iface, LONG iob, BOOL fAvailable)
{
    struct text_services *services = impl_from_IRichEditOle( iface );
    FIXME("stub %p\n", services);
    return E_NOTIMPL;
}

const IRichEditOleVtbl re_ole_vtbl =
{
    IRichEditOle_fnQueryInterface,
    IRichEditOle_fnAddRef,
    IRichEditOle_fnRelease,
    IRichEditOle_fnGetClientSite,
    IRichEditOle_fnGetObjectCount,
    IRichEditOle_fnGetLinkCount,
    IRichEditOle_fnGetObject,
    IRichEditOle_fnInsertObject,
    IRichEditOle_fnConvertObject,
    IRichEditOle_fnActivateAs,
    IRichEditOle_fnSetHostNames,
    IRichEditOle_fnSetLinkAvailable,
    IRichEditOle_fnSetDvaspect,
    IRichEditOle_fnHandsOffStorage,
    IRichEditOle_fnSaveCompleted,
    IRichEditOle_fnInPlaceDeactivate,
    IRichEditOle_fnContextSensitiveHelp,
    IRichEditOle_fnGetClipboardData,
    IRichEditOle_fnImportDataObject
};

/* ITextRange interface */
static HRESULT WINAPI ITextRange_fnQueryInterface(ITextRange *me, REFIID riid, void **ppvObj)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    *ppvObj = NULL;
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDispatch)
        || IsEqualGUID(riid, &IID_ITextRange))
    {
        *ppvObj = me;
        ITextRange_AddRef(me);
        return S_OK;
    }
    else if (IsEqualGUID(riid, &IID_Igetrichole))
    {
        *ppvObj = This->child.reole;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI ITextRange_fnAddRef(ITextRange *me)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ITextRange_fnRelease(ITextRange *me)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE ("%p ref=%lu\n", This, ref);
    if (ref == 0)
    {
        if (This->child.reole)
        {
            list_remove(&This->child.entry);
            This->child.reole = NULL;
        }
        free(This);
    }
    return ref;
}

static HRESULT WINAPI ITextRange_fnGetTypeInfoCount(ITextRange *me, UINT *pctinfo)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);
    TRACE("(%p)->(%p)\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI ITextRange_fnGetTypeInfo(ITextRange *me, UINT iTInfo, LCID lcid,
                                               ITypeInfo **ppTInfo)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);
    HRESULT hr;

    TRACE("(%p)->(%u,%ld,%p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(ITextRange_tid, ppTInfo);
    if (SUCCEEDED(hr))
        ITypeInfo_AddRef(*ppTInfo);
    return hr;
}

static HRESULT WINAPI ITextRange_fnGetIDsOfNames(ITextRange *me, REFIID riid, LPOLESTR *rgszNames,
                                                 UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);
    ITypeInfo *ti;
    HRESULT hr;

    TRACE("(%p)->(%s, %p, %u, %ld, %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid,
            rgDispId);

    hr = get_typeinfo(ITextRange_tid, &ti);
    if (SUCCEEDED(hr))
        hr = ITypeInfo_GetIDsOfNames(ti, rgszNames, cNames, rgDispId);
    return hr;
}

static HRESULT WINAPI ITextRange_fnInvoke(ITextRange *me, DISPID dispIdMember, REFIID riid,
                                          LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                                          VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
                                          UINT *puArgErr)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);
    ITypeInfo *ti;
    HRESULT hr;

    TRACE("(%p)->(%ld, %s, %ld, %u, %p, %p, %p, %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(ITextRange_tid, &ti);
    if (SUCCEEDED(hr))
        hr = ITypeInfo_Invoke(ti, me, dispIdMember, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return hr;
}

static HRESULT WINAPI ITextRange_fnGetText(ITextRange *me, BSTR *str)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);
    ME_TextEditor *editor;
    ME_Cursor start, end;
    int length;
    BOOL bEOP;

    TRACE("(%p)->(%p)\n", This, str);

    if (!This->child.reole)
        return CO_E_RELEASED;

    if (!str)
        return E_INVALIDARG;

    /* return early for degenerate range */
    if (This->start == This->end) {
        *str = NULL;
        return S_OK;
    }

    editor = This->child.reole->editor;
    cursor_from_char_ofs( editor, This->start, &start );
    cursor_from_char_ofs( editor, This->end, &end );

    length = This->end - This->start;
    *str = SysAllocStringLen(NULL, length);
    if (!*str)
        return E_OUTOFMEMORY;

    bEOP = (!para_next( para_next( end.para )) && This->end > ME_GetTextLength(editor));
    ME_GetTextW(editor, *str, length, &start, length, FALSE, bEOP);
    return S_OK;
}

static HRESULT WINAPI ITextRange_fnSetText(ITextRange *me, BSTR str)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);
    ME_TextEditor *editor;
    ME_Cursor cursor;
    ME_Style *style;
    int len;

    TRACE("(%p)->(%s)\n", This, debugstr_w(str));

    if (!This->child.reole)
        return CO_E_RELEASED;

    editor = This->child.reole->editor;

    /* delete only where's something to delete */
    if (This->start != This->end)
    {
        cursor_from_char_ofs( editor, This->start, &cursor );
        ME_InternalDeleteText(editor, &cursor, This->end - This->start, FALSE);
    }

    if (!str || !*str)
    {
        /* will update this range as well */
        textranges_update_ranges(This->child.reole, This->start, This->end, RANGE_UPDATE_DELETE);
        return S_OK;
    }

    /* it's safer not to rely on stored BSTR length */
    len = lstrlenW(str);
    cursor = editor->pCursors[0];
    cursor_from_char_ofs( editor, This->start, &editor->pCursors[0] );
    style = style_get_insert_style( editor, editor->pCursors );
    ME_InsertTextFromCursor(editor, 0, str, len, style);
    ME_ReleaseStyle(style);
    editor->pCursors[0] = cursor;

    if (len < This->end - This->start)
        textranges_update_ranges(This->child.reole, This->start + len, This->end, RANGE_UPDATE_DELETE);
    else
        This->end = len - This->start;

    return S_OK;
}

static HRESULT range_GetChar(ME_TextEditor *editor, ME_Cursor *cursor, LONG *pch)
{
    WCHAR wch[2];

    ME_GetTextW(editor, wch, 1, cursor, 1, FALSE, !para_next( para_next( cursor->para ) ));
    *pch = wch[0];

    return S_OK;
}

static HRESULT WINAPI ITextRange_fnGetChar(ITextRange *me, LONG *pch)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);
    ME_TextEditor *editor;
    ME_Cursor cursor;

    TRACE("(%p)->(%p)\n", This, pch);

    if (!This->child.reole)
        return CO_E_RELEASED;

    if (!pch)
        return E_INVALIDARG;

    editor = This->child.reole->editor;
    cursor_from_char_ofs( editor, This->start, &cursor );
    return range_GetChar(editor, &cursor, pch);
}

static HRESULT WINAPI ITextRange_fnSetChar(ITextRange *me, LONG ch)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%lx): stub\n", This, ch);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT CreateITextRange(struct text_services *services, LONG start, LONG end, ITextRange** ppRange);

static HRESULT WINAPI ITextRange_fnGetDuplicate(ITextRange *me, ITextRange **ppRange)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    TRACE("(%p)->(%p)\n", This, ppRange);

    if (!This->child.reole)
        return CO_E_RELEASED;

    if (!ppRange)
        return E_INVALIDARG;

    return CreateITextRange(This->child.reole, This->start, This->end, ppRange);
}

static HRESULT WINAPI ITextRange_fnGetFormattedText(ITextRange *me, ITextRange **range)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%p): stub\n", This, range);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnSetFormattedText(ITextRange *me, ITextRange *range)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%p): stub\n", This, range);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnGetStart(ITextRange *me, LONG *start)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    TRACE("(%p)->(%p)\n", This, start);

    if (!This->child.reole)
        return CO_E_RELEASED;

    if (!start)
        return E_INVALIDARG;

    *start = This->start;
    return S_OK;
}

static HRESULT textrange_setstart(const struct text_services *services, LONG value, LONG *start, LONG *end)
{
    int len;

    if (value < 0)
        value = 0;

    if (value == *start)
        return S_FALSE;

    if (value <= *end) {
        *start = value;
        return S_OK;
    }

    len = ME_GetTextLength(services->editor);
    *start = *end = value > len ? len : value;
    return S_OK;
}

static HRESULT WINAPI ITextRange_fnSetStart(ITextRange *me, LONG value)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    TRACE("(%p)->(%ld)\n", This, value);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return textrange_setstart(This->child.reole, value, &This->start, &This->end);
}

static HRESULT WINAPI ITextRange_fnGetEnd(ITextRange *me, LONG *end)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    TRACE("(%p)->(%p)\n", This, end);

    if (!This->child.reole)
        return CO_E_RELEASED;

    if (!end)
        return E_INVALIDARG;

    *end = This->end;
    return S_OK;
}

static HRESULT textrange_setend(const struct text_services *services, LONG value, LONG *start, LONG *end)
{
    int len;

    if (value == *end)
        return S_FALSE;

    if (value < *start) {
        *start = *end = max(0, value);
        return S_OK;
    }

    len = ME_GetTextLength( services->editor );
    *end = value > len ? len + 1 : value;
    return S_OK;
}

static HRESULT WINAPI ITextRange_fnSetEnd(ITextRange *me, LONG value)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    TRACE("(%p)->(%ld)\n", This, value);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return textrange_setend(This->child.reole, value, &This->start, &This->end);
}

static HRESULT WINAPI ITextRange_fnGetFont(ITextRange *me, ITextFont **font)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    TRACE("(%p)->(%p)\n", This, font);

    if (!This->child.reole)
        return CO_E_RELEASED;

    if (!font)
        return E_INVALIDARG;

    return create_textfont(me, NULL, font);
}

static HRESULT WINAPI ITextRange_fnSetFont(ITextRange *me, ITextFont *font)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    TRACE("(%p)->(%p)\n", This, font);

    if (!font)
        return E_INVALIDARG;

    if (!This->child.reole)
        return CO_E_RELEASED;

    textrange_set_font(me, font);
    return S_OK;
}

static HRESULT WINAPI ITextRange_fnGetPara(ITextRange *me, ITextPara **para)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    TRACE("(%p)->(%p)\n", This, para);

    if (!This->child.reole)
        return CO_E_RELEASED;

    if (!para)
        return E_INVALIDARG;

    return create_textpara(me, para);
}

static HRESULT WINAPI ITextRange_fnSetPara(ITextRange *me, ITextPara *para)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%p): stub\n", This, para);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnGetStoryLength(ITextRange *me, LONG *length)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    TRACE("(%p)->(%p)\n", This, length);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return textrange_get_storylength(This->child.reole->editor, length);
}

static HRESULT WINAPI ITextRange_fnGetStoryType(ITextRange *me, LONG *value)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    TRACE("(%p)->(%p)\n", This, value);

    if (!This->child.reole)
        return CO_E_RELEASED;

    if (!value)
        return E_INVALIDARG;

    *value = tomUnknownStory;
    return S_OK;
}

static HRESULT range_Collapse(LONG bStart, LONG *start, LONG *end)
{
  if (*end == *start)
      return S_FALSE;

  if (bStart == tomEnd)
      *start = *end;
  else
      *end = *start;
  return S_OK;
}

static HRESULT WINAPI ITextRange_fnCollapse(ITextRange *me, LONG bStart)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    TRACE("(%p)->(%ld)\n", This, bStart);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return range_Collapse(bStart, &This->start, &This->end);
}

static HRESULT WINAPI ITextRange_fnExpand(ITextRange *me, LONG unit, LONG *delta)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    TRACE("(%p)->(%ld %p)\n", This, unit, delta);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return textrange_expand(me, unit, delta);
}

static HRESULT WINAPI ITextRange_fnGetIndex(ITextRange *me, LONG unit, LONG *index)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%ld %p): stub\n", This, unit, index);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnSetIndex(ITextRange *me, LONG unit, LONG index,
                                            LONG extend)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%ld %ld %ld): stub\n", This, unit, index, extend);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static void cp2range(ME_TextEditor *editor, LONG *cp1, LONG *cp2)
{
    int len = ME_GetTextLength(editor) + 1;

    *cp1 = max(*cp1, 0);
    *cp2 = max(*cp2, 0);
    *cp1 = min(*cp1, len);
    *cp2 = min(*cp2, len);
    if (*cp1 > *cp2)
    {
        int tmp = *cp1;
        *cp1 = *cp2;
        *cp2 = tmp;
    }
    if (*cp1 == len)
        *cp1 = *cp2 = len - 1;
}

static HRESULT WINAPI ITextRange_fnSetRange(ITextRange *me, LONG anchor, LONG active)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    TRACE("(%p)->(%ld %ld)\n", This, anchor, active);

    if (!This->child.reole)
        return CO_E_RELEASED;

    cp2range(This->child.reole->editor, &anchor, &active);
    if (anchor == This->start && active == This->end)
        return S_FALSE;

    This->start = anchor;
    This->end = active;
    return S_OK;
}

static HRESULT textrange_inrange(LONG start, LONG end, ITextRange *range, LONG *ret)
{
    LONG from, to, v;

    if (!ret)
        ret = &v;

    if (FAILED(ITextRange_GetStart(range, &from)) || FAILED(ITextRange_GetEnd(range, &to))) {
        *ret = tomFalse;
    }
    else
        *ret = (start >= from && end <= to) ? tomTrue : tomFalse;
    return *ret == tomTrue ? S_OK : S_FALSE;
}

static HRESULT WINAPI ITextRange_fnInRange(ITextRange *me, ITextRange *range, LONG *ret)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    TRACE("(%p)->(%p %p)\n", This, range, ret);

    if (ret)
        *ret = tomFalse;

    if (!This->child.reole)
        return CO_E_RELEASED;

    if (!range)
        return S_FALSE;

    return textrange_inrange(This->start, This->end, range, ret);
}

static HRESULT WINAPI ITextRange_fnInStory(ITextRange *me, ITextRange *pRange, LONG *ret)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%p): stub\n", This, ret);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT textrange_isequal(LONG start, LONG end, ITextRange *range, LONG *ret)
{
    LONG from, to, v;

    if (!ret)
        ret = &v;

    if (FAILED(ITextRange_GetStart(range, &from)) || FAILED(ITextRange_GetEnd(range, &to))) {
        *ret = tomFalse;
    }
    else
        *ret = (start == from && end == to) ? tomTrue : tomFalse;
    return *ret == tomTrue ? S_OK : S_FALSE;
}

static HRESULT WINAPI ITextRange_fnIsEqual(ITextRange *me, ITextRange *range, LONG *ret)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    TRACE("(%p)->(%p %p)\n", This, range, ret);

    if (ret)
        *ret = tomFalse;

    if (!This->child.reole)
        return CO_E_RELEASED;

    if (!range)
        return S_FALSE;

    return textrange_isequal(This->start, This->end, range, ret);
}

static HRESULT WINAPI ITextRange_fnSelect(ITextRange *me)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    TRACE("(%p)\n", This);

    if (!This->child.reole)
        return CO_E_RELEASED;

    set_selection(This->child.reole->editor, This->start, This->end);
    return S_OK;
}

static HRESULT textrange_startof(ITextRange *range, LONG unit, LONG extend, LONG *delta)
{
    HRESULT hr;
    LONG start, end;
    LONG moved;

    ITextRange_GetStart(range, &start);
    ITextRange_GetEnd(range, &end);

    switch (unit)
    {
    case tomCharacter:
    {
        moved = 0;
        if (extend == tomMove) {
            if (start != end) {
                ITextRange_SetEnd(range, start);
                moved = -1;
            }
        }
        if (delta)
            *delta = moved;
        hr = moved ? S_OK : S_FALSE;
        break;
    }
    default:
        FIXME("unit %ld is not supported\n", unit);
        return E_NOTIMPL;
    }
    return hr;
}

static HRESULT WINAPI ITextRange_fnStartOf(ITextRange *me, LONG unit, LONG extend,
                                           LONG *delta)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    TRACE("(%p)->(%ld %ld %p)\n", This, unit, extend, delta);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return textrange_startof(me, unit, extend, delta);
}

static HRESULT textrange_endof(ITextRange *range, ME_TextEditor *editor, LONG unit, LONG extend, LONG *delta)
{
    HRESULT hr;
    LONG old_start, old_end, new_end;
    LONG moved;

    ITextRange_GetStart(range, &old_start);
    ITextRange_GetEnd(range, &old_end);

    switch (unit)
    {
    case tomCharacter:
    {
        moved = 0;
        new_end = old_end;
        if (old_end == 0)
        {
            ME_Cursor cursor;
            cursor_from_char_ofs( editor, old_end, &cursor );
            moved = ME_MoveCursorChars(editor, &cursor, 1, TRUE);
            new_end = old_end + moved;
        }
        else if (extend == tomMove && old_start != old_end)
            moved = 1;

        ITextRange_SetEnd(range, new_end);
        if (extend == tomMove)
            ITextRange_SetStart(range, new_end);
        if (delta)
            *delta = moved;
        hr = moved ? S_OK : S_FALSE;
        break;
    }
    default:
        FIXME("unit %ld is not supported\n", unit);
        return E_NOTIMPL;
    }
    return hr;
}

static HRESULT WINAPI ITextRange_fnEndOf(ITextRange *me, LONG unit, LONG extend,
                                         LONG *delta)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    TRACE("(%p)->(%ld %ld %p)\n", This, unit, extend, delta);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return textrange_endof(me, This->child.reole->editor, unit, extend, delta);
}

static HRESULT textrange_move(ITextRange *range, ME_TextEditor *editor, LONG unit, LONG count, LONG *delta)
{
    LONG old_start, old_end, new_start, new_end;
    LONG move_by;
    LONG moved;
    HRESULT hr = S_OK;

    if (!count)
    {
        if (delta)
            *delta = 0;
        return S_FALSE;
    }

    ITextRange_GetStart(range, &old_start);
    ITextRange_GetEnd(range, &old_end);
    switch (unit)
    {
    case tomCharacter:
    {
        ME_Cursor cursor;

        if (count > 0)
        {
            cursor_from_char_ofs( editor, old_end, &cursor );
            move_by = count;
            if (old_start != old_end)
                --move_by;
        }
        else
        {
            cursor_from_char_ofs( editor, old_start, &cursor );
            move_by = count;
            if (old_start != old_end)
                ++move_by;
        }
        moved = ME_MoveCursorChars(editor, &cursor, move_by, FALSE);
        if (count > 0)
        {
            new_end = old_end + moved;
            new_start = new_end;
            if (old_start != old_end)
                ++moved;
        }
        else
        {
            new_start = old_start + moved;
            new_end = new_start;
            if (old_start != old_end)
                --moved;
        }
        if (delta) *delta = moved;
        break;
    }
    default:
        FIXME("unit %ld is not supported\n", unit);
        return E_NOTIMPL;
    }
    if (moved == 0)
        hr = S_FALSE;
    ITextRange_SetStart(range, new_start);
    ITextRange_SetEnd(range, new_end);

    return hr;
}

static HRESULT WINAPI ITextRange_fnMove(ITextRange *me, LONG unit, LONG count, LONG *delta)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    TRACE("(%p)->(%ld %ld %p)\n", This, unit, count, delta);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return textrange_move(me, This->child.reole->editor, unit, count, delta);
}

static HRESULT textrange_movestart(ITextRange *range, ME_TextEditor *editor, LONG unit, LONG count, LONG *delta)
{
    LONG old_start, old_end, new_start, new_end;
    HRESULT hr = S_OK;

    if (!count)
    {
        if (delta)
            *delta = 0;
        return S_FALSE;
    }

    ITextRange_GetStart(range, &old_start);
    ITextRange_GetEnd(range, &old_end);
    switch (unit)
    {
    case tomCharacter:
    {
        ME_Cursor cursor;
        LONG moved;

        cursor_from_char_ofs( editor, old_start, &cursor );
        moved = ME_MoveCursorChars(editor, &cursor, count, FALSE);
        new_start = old_start + moved;
        new_end = old_end;
        if (new_end < new_start)
            new_end = new_start;
        if (delta)
            *delta = moved;
        break;
    }
    default:
        FIXME("unit %ld is not supported\n", unit);
        return E_NOTIMPL;
    }
    if (new_start == old_start)
        hr = S_FALSE;
    ITextRange_SetStart(range, new_start);
    ITextRange_SetEnd(range, new_end);

    return hr;
}

static HRESULT WINAPI ITextRange_fnMoveStart(ITextRange *me, LONG unit, LONG count,
                                             LONG *delta)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    TRACE("(%p)->(%ld %ld %p)\n", This, unit, count, delta);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return textrange_movestart(me, This->child.reole->editor, unit, count, delta);
}

static HRESULT textrange_moveend(ITextRange *range, ME_TextEditor *editor, LONG unit, LONG count, LONG *delta)
{
    LONG old_start, old_end, new_start, new_end;
    HRESULT hr = S_OK;

    if (!count)
    {
        if (delta)
            *delta = 0;
        return S_FALSE;
    }

    ITextRange_GetStart(range, &old_start);
    ITextRange_GetEnd(range, &old_end);
    switch (unit)
    {
    case tomCharacter:
    {
        ME_Cursor cursor;
        LONG moved;

        cursor_from_char_ofs( editor, old_end, &cursor );
        moved = ME_MoveCursorChars(editor, &cursor, count, TRUE);
        new_start = old_start;
        new_end = old_end + moved;
        if (new_end < new_start)
            new_start = new_end;
        if (delta)
            *delta = moved;
        break;
    }
    case tomStory:
        if (count < 0)
            new_start = new_end = 0;
        else
        {
            new_start = old_start;
            ITextRange_GetStoryLength(range, &new_end);
        }
        if (delta)
        {
            if (new_end < old_end)
                *delta = -1;
            else if (new_end == old_end)
                *delta = 0;
            else
                *delta = 1;
        }
        break;
    default:
        FIXME("unit %ld is not supported\n", unit);
        return E_NOTIMPL;
    }
    if (new_end == old_end)
        hr = S_FALSE;
    ITextRange_SetStart(range, new_start);
    ITextRange_SetEnd(range, new_end);

    return hr;
}

static HRESULT WINAPI ITextRange_fnMoveEnd(ITextRange *me, LONG unit, LONG count,
                                           LONG *delta)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    TRACE("(%p)->(%ld %ld %p)\n", This, unit, count, delta);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return textrange_moveend(me, This->child.reole->editor, unit, count, delta);
}

static HRESULT WINAPI ITextRange_fnMoveWhile(ITextRange *me, VARIANT *charset, LONG count,
                                             LONG *delta)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%s %ld %p): stub\n", This, debugstr_variant(charset), count, delta);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnMoveStartWhile(ITextRange *me, VARIANT *charset, LONG count,
                                                  LONG *delta)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%s %ld %p): stub\n", This, debugstr_variant(charset), count, delta);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnMoveEndWhile(ITextRange *me, VARIANT *charset, LONG count,
                                                LONG *delta)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%s %ld %p): stub\n", This, debugstr_variant(charset), count, delta);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnMoveUntil(ITextRange *me, VARIANT *charset, LONG count,
                                             LONG *delta)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%s %ld %p): stub\n", This, debugstr_variant(charset), count, delta);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnMoveStartUntil(ITextRange *me, VARIANT *charset, LONG count,
                                                  LONG *delta)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%s %ld %p): stub\n", This, debugstr_variant(charset), count, delta);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnMoveEndUntil(ITextRange *me, VARIANT *charset, LONG count,
                                                LONG *delta)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%s %ld %p): stub\n", This, debugstr_variant(charset), count, delta);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnFindText(ITextRange *me, BSTR text, LONG count, LONG flags,
                                            LONG *length)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%s %ld %lx %p): stub\n", This, debugstr_w(text), count, flags, length);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnFindTextStart(ITextRange *me, BSTR text, LONG count,
                                                 LONG flags, LONG *length)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%s %ld %lx %p): stub\n", This, debugstr_w(text), count, flags, length);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnFindTextEnd(ITextRange *me, BSTR text, LONG count,
                                               LONG flags, LONG *length)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%s %ld %lx %p): stub\n", This, debugstr_w(text), count, flags, length);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnDelete(ITextRange *me, LONG unit, LONG count, LONG *delta)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%ld %ld %p): stub\n", This, unit, count, delta);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT textrange_copy_or_cut( ITextRange *range, ME_TextEditor *editor, BOOL cut, VARIANT *v )
{
    LONG start, end;
    ME_Cursor cursor;
    IDataObject **data_out = NULL;

    ITextRange_GetStart( range, &start );
    ITextRange_GetEnd( range, &end );
    if (start == end)
    {
        /* If the range is empty, all text is copied */
        LONG prev_end = end;
        ITextRange_SetEnd( range, MAXLONG );
        start = 0;
        ITextRange_GetEnd( range, &end );
        ITextRange_SetEnd( range, prev_end );
    }
    cursor_from_char_ofs( editor, start, &cursor );

    if (v && V_VT(v) == (VT_UNKNOWN | VT_BYREF) && V_UNKNOWNREF( v ))
        data_out = (IDataObject **)V_UNKNOWNREF( v );

    return editor_copy_or_cut( editor, cut, &cursor, end - start, data_out );
}

static HRESULT WINAPI ITextRange_fnCut(ITextRange *me, VARIANT *v)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    TRACE("(%p)->(%p)\n", This, v);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return textrange_copy_or_cut(me, This->child.reole->editor, TRUE, v);
}

static HRESULT WINAPI ITextRange_fnCopy(ITextRange *me, VARIANT *v)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    TRACE("(%p)->(%p)\n", This, v);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return textrange_copy_or_cut(me, This->child.reole->editor, FALSE, v);
}

static HRESULT WINAPI ITextRange_fnPaste(ITextRange *me, VARIANT *v, LONG format)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%s %lx): stub\n", This, debugstr_variant(v), format);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnCanPaste(ITextRange *me, VARIANT *v, LONG format, LONG *ret)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%s %lx %p): stub\n", This, debugstr_variant(v), format, ret);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnCanEdit(ITextRange *me, LONG *ret)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%p): stub\n", This, ret);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnChangeCase(ITextRange *me, LONG type)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%ld): stub\n", This, type);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnGetPoint(ITextRange *me, LONG type, LONG *cx, LONG *cy)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%ld %p %p): stub\n", This, type, cx, cy);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnSetPoint(ITextRange *me, LONG x, LONG y, LONG type,
                                            LONG extend)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%ld %ld %ld %ld): stub\n", This, x, y, type, extend);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnScrollIntoView(ITextRange *me, LONG value)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);
    ME_TextEditor *editor;
    ME_Cursor cursor;
    int x, y, height;

    TRACE("(%p)->(%ld)\n", This, value);

    if (!This->child.reole)
        return CO_E_RELEASED;

    editor = This->child.reole->editor;

    switch (value)
    {
    case tomStart:
        cursor_from_char_ofs( editor, This->start, &cursor );
        cursor_coords( editor, &cursor, &x, &y, &height );
        break;
    case tomEnd:
        cursor_from_char_ofs( editor, This->end, &cursor );
        cursor_coords( editor, &cursor, &x, &y, &height );
        break;
    default:
        FIXME("bStart value %ld not handled\n", value);
        return E_NOTIMPL;
    }
    scroll_abs( editor, x, y, TRUE );
    return S_OK;
}

static HRESULT WINAPI ITextRange_fnGetEmbeddedObject(ITextRange *me, IUnknown **ppv)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%p): stub\n", This, ppv);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static const ITextRangeVtbl trvt = {
    ITextRange_fnQueryInterface,
    ITextRange_fnAddRef,
    ITextRange_fnRelease,
    ITextRange_fnGetTypeInfoCount,
    ITextRange_fnGetTypeInfo,
    ITextRange_fnGetIDsOfNames,
    ITextRange_fnInvoke,
    ITextRange_fnGetText,
    ITextRange_fnSetText,
    ITextRange_fnGetChar,
    ITextRange_fnSetChar,
    ITextRange_fnGetDuplicate,
    ITextRange_fnGetFormattedText,
    ITextRange_fnSetFormattedText,
    ITextRange_fnGetStart,
    ITextRange_fnSetStart,
    ITextRange_fnGetEnd,
    ITextRange_fnSetEnd,
    ITextRange_fnGetFont,
    ITextRange_fnSetFont,
    ITextRange_fnGetPara,
    ITextRange_fnSetPara,
    ITextRange_fnGetStoryLength,
    ITextRange_fnGetStoryType,
    ITextRange_fnCollapse,
    ITextRange_fnExpand,
    ITextRange_fnGetIndex,
    ITextRange_fnSetIndex,
    ITextRange_fnSetRange,
    ITextRange_fnInRange,
    ITextRange_fnInStory,
    ITextRange_fnIsEqual,
    ITextRange_fnSelect,
    ITextRange_fnStartOf,
    ITextRange_fnEndOf,
    ITextRange_fnMove,
    ITextRange_fnMoveStart,
    ITextRange_fnMoveEnd,
    ITextRange_fnMoveWhile,
    ITextRange_fnMoveStartWhile,
    ITextRange_fnMoveEndWhile,
    ITextRange_fnMoveUntil,
    ITextRange_fnMoveStartUntil,
    ITextRange_fnMoveEndUntil,
    ITextRange_fnFindText,
    ITextRange_fnFindTextStart,
    ITextRange_fnFindTextEnd,
    ITextRange_fnDelete,
    ITextRange_fnCut,
    ITextRange_fnCopy,
    ITextRange_fnPaste,
    ITextRange_fnCanPaste,
    ITextRange_fnCanEdit,
    ITextRange_fnChangeCase,
    ITextRange_fnGetPoint,
    ITextRange_fnSetPoint,
    ITextRange_fnScrollIntoView,
    ITextRange_fnGetEmbeddedObject
};

/* ITextFont */
static HRESULT WINAPI TextFont_QueryInterface(ITextFont *iface, REFIID riid, void **ppv)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    if (IsEqualIID(riid, &IID_ITextFont) ||
        IsEqualIID(riid, &IID_IDispatch) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *ppv = iface;
        ITextFont_AddRef(iface);
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI TextFont_AddRef(ITextFont *iface)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%lu)\n", This, ref);
    return ref;
}

static ULONG WINAPI TextFont_Release(ITextFont *iface)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%lu)\n", This, ref);

    if (!ref)
    {
        if (This->range)
            ITextRange_Release(This->range);
        SysFreeString(This->props[FONT_NAME].str);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI TextFont_GetTypeInfoCount(ITextFont *iface, UINT *pctinfo)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%p)\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI TextFont_GetTypeInfo(ITextFont *iface, UINT iTInfo, LCID lcid,
    ITypeInfo **ppTInfo)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    HRESULT hr;

    TRACE("(%p)->(%u,%ld,%p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(ITextFont_tid, ppTInfo);
    if (SUCCEEDED(hr))
        ITypeInfo_AddRef(*ppTInfo);
    return hr;
}

static HRESULT WINAPI TextFont_GetIDsOfNames(ITextFont *iface, REFIID riid,
    LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    ITypeInfo *ti;
    HRESULT hr;

    TRACE("(%p)->(%s, %p, %u, %ld, %p)\n", This, debugstr_guid(riid),
            rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo(ITextFont_tid, &ti);
    if (SUCCEEDED(hr))
        hr = ITypeInfo_GetIDsOfNames(ti, rgszNames, cNames, rgDispId);
    return hr;
}

static HRESULT WINAPI TextFont_Invoke(
    ITextFont *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    ITypeInfo *ti;
    HRESULT hr;

    TRACE("(%p)->(%ld, %s, %ld, %u, %p, %p, %p, %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(ITextFont_tid, &ti);
    if (SUCCEEDED(hr))
        hr = ITypeInfo_Invoke(ti, iface, dispIdMember, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return hr;
}

static HRESULT WINAPI TextFont_GetDuplicate(ITextFont *iface, ITextFont **ret)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);

    TRACE("(%p)->(%p)\n", This, ret);

    if (!ret)
        return E_INVALIDARG;

    *ret = NULL;
    if (This->range && !get_range_reole(This->range))
        return CO_E_RELEASED;

    return create_textfont(NULL, This, ret);
}

static HRESULT WINAPI TextFont_SetDuplicate(ITextFont *iface, ITextFont *pFont)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    FIXME("(%p)->(%p): stub\n", This, pFont);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextFont_CanChange(ITextFont *iface, LONG *ret)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    FIXME("(%p)->(%p): stub\n", This, ret);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextFont_IsEqual(ITextFont *iface, ITextFont *font, LONG *ret)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    FIXME("(%p)->(%p %p): stub\n", This, font, ret);
    return E_NOTIMPL;
}

static void textfont_reset_to_default(ITextFontImpl *font)
{
    enum textfont_prop_id id;

    for (id = FONT_PROPID_FIRST; id < FONT_PROPID_LAST; id++) {
        switch (id)
        {
        case FONT_ALLCAPS:
        case FONT_ANIMATION:
        case FONT_BOLD:
        case FONT_EMBOSS:
        case FONT_HIDDEN:
        case FONT_ENGRAVE:
        case FONT_ITALIC:
        case FONT_OUTLINE:
        case FONT_PROTECTED:
        case FONT_SHADOW:
        case FONT_SMALLCAPS:
        case FONT_STRIKETHROUGH:
        case FONT_SUBSCRIPT:
        case FONT_SUPERSCRIPT:
        case FONT_UNDERLINE:
            font->props[id].l = tomFalse;
            break;
        case FONT_BACKCOLOR:
        case FONT_FORECOLOR:
            font->props[id].l = tomAutoColor;
            break;
        case FONT_KERNING:
        case FONT_POSITION:
        case FONT_SIZE:
        case FONT_SPACING:
            font->props[id].f = 0.0;
            break;
        case FONT_LANGID:
            font->props[id].l = GetSystemDefaultLCID();
            break;
        case FONT_NAME: {
            SysFreeString(font->props[id].str);
            font->props[id].str = SysAllocString(L"System");
            break;
        }
        case FONT_WEIGHT:
            font->props[id].l = FW_NORMAL;
            break;
        default:
            FIXME("font property %d not handled\n", id);
        }
    }
}

static void textfont_reset_to_undefined(ITextFontImpl *font)
{
    enum textfont_prop_id id;

    for (id = FONT_PROPID_FIRST; id < FONT_PROPID_LAST; id++) {
        switch (id)
        {
        case FONT_ALLCAPS:
        case FONT_ANIMATION:
        case FONT_BOLD:
        case FONT_EMBOSS:
        case FONT_HIDDEN:
        case FONT_ENGRAVE:
        case FONT_ITALIC:
        case FONT_OUTLINE:
        case FONT_PROTECTED:
        case FONT_SHADOW:
        case FONT_SMALLCAPS:
        case FONT_STRIKETHROUGH:
        case FONT_SUBSCRIPT:
        case FONT_SUPERSCRIPT:
        case FONT_UNDERLINE:
        case FONT_BACKCOLOR:
        case FONT_FORECOLOR:
        case FONT_LANGID:
        case FONT_WEIGHT:
            font->props[id].l = tomUndefined;
            break;
        case FONT_KERNING:
        case FONT_POSITION:
        case FONT_SIZE:
        case FONT_SPACING:
            font->props[id].f = tomUndefined;
            break;
        case FONT_NAME:
            break;
        default:
            FIXME("font property %d not handled\n", id);
        }
    }
}

static void textfont_apply_range_props(ITextFontImpl *font)
{
    enum textfont_prop_id propid;
    for (propid = FONT_PROPID_FIRST; propid < FONT_PROPID_LAST; propid++)
        set_textfont_prop(font, propid, &font->props[propid]);
}

static HRESULT WINAPI TextFont_Reset(ITextFont *iface, LONG value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);

    TRACE("(%p)->(%ld)\n", This, value);

    /* If font is attached to a range, released or not, we can't
       reset to undefined */
    if (This->range) {
        if (!get_range_reole(This->range))
            return CO_E_RELEASED;

        switch (value)
        {
        case tomUndefined:
            return E_INVALIDARG;
        case tomCacheParms:
            textfont_cache_range_props(This);
            This->get_cache_enabled = TRUE;
            break;
        case tomTrackParms:
            This->get_cache_enabled = FALSE;
            break;
        case tomApplyLater:
            This->set_cache_enabled = TRUE;
            break;
        case tomApplyNow:
            This->set_cache_enabled = FALSE;
            textfont_apply_range_props(This);
            break;
        case tomUsePoints:
        case tomUseTwips:
            return E_INVALIDARG;
        default:
            FIXME("reset mode %ld not supported\n", value);
        }

        return S_OK;
    }
    else {
        switch (value)
        {
        /* reset to global defaults */
        case tomDefault:
            textfont_reset_to_default(This);
            return S_OK;
        /* all properties are set to tomUndefined, font name is retained */
        case tomUndefined:
            textfont_reset_to_undefined(This);
            return S_OK;
        case tomApplyNow:
        case tomApplyLater:
        case tomTrackParms:
        case tomCacheParms:
            return S_OK;
        case tomUsePoints:
        case tomUseTwips:
            return E_INVALIDARG;
        }
    }

    FIXME("reset mode %ld not supported\n", value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextFont_GetStyle(ITextFont *iface, LONG *value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    FIXME("(%p)->(%p): stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextFont_SetStyle(ITextFont *iface, LONG value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    FIXME("(%p)->(%ld): stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextFont_GetAllCaps(ITextFont *iface, LONG *value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%p)\n", This, value);
    return get_textfont_propl(This, FONT_ALLCAPS, value);
}

static HRESULT WINAPI TextFont_SetAllCaps(ITextFont *iface, LONG value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%ld)\n", This, value);
    return set_textfont_propd(This, FONT_ALLCAPS, value);
}

static HRESULT WINAPI TextFont_GetAnimation(ITextFont *iface, LONG *value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%p)\n", This, value);
    return get_textfont_propl(This, FONT_ANIMATION, value);
}

static HRESULT WINAPI TextFont_SetAnimation(ITextFont *iface, LONG value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);

    TRACE("(%p)->(%ld)\n", This, value);

    if (value < tomNoAnimation || value > tomAnimationMax)
        return E_INVALIDARG;

    return set_textfont_propl(This, FONT_ANIMATION, value);
}

static HRESULT WINAPI TextFont_GetBackColor(ITextFont *iface, LONG *value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%p)\n", This, value);
    return get_textfont_propl(This, FONT_BACKCOLOR, value);
}

static HRESULT WINAPI TextFont_SetBackColor(ITextFont *iface, LONG value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%ld)\n", This, value);
    return set_textfont_propl(This, FONT_BACKCOLOR, value);
}

static HRESULT WINAPI TextFont_GetBold(ITextFont *iface, LONG *value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%p)\n", This, value);
    return get_textfont_propl(This, FONT_BOLD, value);
}

static HRESULT WINAPI TextFont_SetBold(ITextFont *iface, LONG value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%ld)\n", This, value);
    return set_textfont_propd(This, FONT_BOLD, value);
}

static HRESULT WINAPI TextFont_GetEmboss(ITextFont *iface, LONG *value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%p)\n", This, value);
    return get_textfont_propl(This, FONT_EMBOSS, value);
}

static HRESULT WINAPI TextFont_SetEmboss(ITextFont *iface, LONG value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%ld)\n", This, value);
    return set_textfont_propd(This, FONT_EMBOSS, value);
}

static HRESULT WINAPI TextFont_GetForeColor(ITextFont *iface, LONG *value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%p)\n", This, value);
    return get_textfont_propl(This, FONT_FORECOLOR, value);
}

static HRESULT WINAPI TextFont_SetForeColor(ITextFont *iface, LONG value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%ld)\n", This, value);
    return set_textfont_propl(This, FONT_FORECOLOR, value);
}

static HRESULT WINAPI TextFont_GetHidden(ITextFont *iface, LONG *value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%p)\n", This, value);
    return get_textfont_propl(This, FONT_HIDDEN, value);
}

static HRESULT WINAPI TextFont_SetHidden(ITextFont *iface, LONG value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%ld)\n", This, value);
    return set_textfont_propd(This, FONT_HIDDEN, value);
}

static HRESULT WINAPI TextFont_GetEngrave(ITextFont *iface, LONG *value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%p)\n", This, value);
    return get_textfont_propl(This, FONT_ENGRAVE, value);
}

static HRESULT WINAPI TextFont_SetEngrave(ITextFont *iface, LONG value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%ld)\n", This, value);
    return set_textfont_propd(This, FONT_ENGRAVE, value);
}

static HRESULT WINAPI TextFont_GetItalic(ITextFont *iface, LONG *value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%p)\n", This, value);
    return get_textfont_propl(This, FONT_ITALIC, value);
}

static HRESULT WINAPI TextFont_SetItalic(ITextFont *iface, LONG value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%ld)\n", This, value);
    return set_textfont_propd(This, FONT_ITALIC, value);
}

static HRESULT WINAPI TextFont_GetKerning(ITextFont *iface, FLOAT *value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%p)\n", This, value);
    return get_textfont_propf(This, FONT_KERNING, value);
}

static HRESULT WINAPI TextFont_SetKerning(ITextFont *iface, FLOAT value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%.2f)\n", This, value);
    return set_textfont_propf(This, FONT_KERNING, value);
}

static HRESULT WINAPI TextFont_GetLanguageID(ITextFont *iface, LONG *value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%p)\n", This, value);
    return get_textfont_propl(This, FONT_LANGID, value);
}

static HRESULT WINAPI TextFont_SetLanguageID(ITextFont *iface, LONG value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%ld)\n", This, value);
    return set_textfont_propl(This, FONT_LANGID, value);
}

static HRESULT WINAPI TextFont_GetName(ITextFont *iface, BSTR *value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);

    TRACE("(%p)->(%p)\n", This, value);

    if (!value)
        return E_INVALIDARG;

    *value = NULL;

    if (!This->range) {
        if (This->props[FONT_NAME].str)
            *value = SysAllocString(This->props[FONT_NAME].str);
        else
            *value = SysAllocStringLen(NULL, 0);
        return *value ? S_OK : E_OUTOFMEMORY;
    }

    return textfont_getname_from_range(This->range, value);
}

static HRESULT WINAPI TextFont_SetName(ITextFont *iface, BSTR value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    textfont_prop_val v;

    TRACE("(%p)->(%s)\n", This, debugstr_w(value));

    v.str = value;
    return set_textfont_prop(This, FONT_NAME, &v);
}

static HRESULT WINAPI TextFont_GetOutline(ITextFont *iface, LONG *value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%p)\n", This, value);
    return get_textfont_propl(This, FONT_OUTLINE, value);
}

static HRESULT WINAPI TextFont_SetOutline(ITextFont *iface, LONG value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%ld)\n", This, value);
    return set_textfont_propd(This, FONT_OUTLINE, value);
}

static HRESULT WINAPI TextFont_GetPosition(ITextFont *iface, FLOAT *value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%p)\n", This, value);
    return get_textfont_propf(This, FONT_POSITION, value);
}

static HRESULT WINAPI TextFont_SetPosition(ITextFont *iface, FLOAT value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%.2f)\n", This, value);
    return set_textfont_propf(This, FONT_POSITION, value);
}

static HRESULT WINAPI TextFont_GetProtected(ITextFont *iface, LONG *value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%p)\n", This, value);
    return get_textfont_propl(This, FONT_PROTECTED, value);
}

static HRESULT WINAPI TextFont_SetProtected(ITextFont *iface, LONG value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%ld)\n", This, value);
    return set_textfont_propd(This, FONT_PROTECTED, value);
}

static HRESULT WINAPI TextFont_GetShadow(ITextFont *iface, LONG *value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%p)\n", This, value);
    return get_textfont_propl(This, FONT_SHADOW, value);
}

static HRESULT WINAPI TextFont_SetShadow(ITextFont *iface, LONG value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%ld)\n", This, value);
    return set_textfont_propd(This, FONT_SHADOW, value);
}

static HRESULT WINAPI TextFont_GetSize(ITextFont *iface, FLOAT *value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%p)\n", This, value);
    return get_textfont_propf(This, FONT_SIZE, value);
}

static HRESULT WINAPI TextFont_SetSize(ITextFont *iface, FLOAT value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%.2f)\n", This, value);
    return set_textfont_propf(This, FONT_SIZE, value);
}

static HRESULT WINAPI TextFont_GetSmallCaps(ITextFont *iface, LONG *value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%p)\n", This, value);
    return get_textfont_propl(This, FONT_SMALLCAPS, value);
}

static HRESULT WINAPI TextFont_SetSmallCaps(ITextFont *iface, LONG value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%ld)\n", This, value);
    return set_textfont_propd(This, FONT_SMALLCAPS, value);
}

static HRESULT WINAPI TextFont_GetSpacing(ITextFont *iface, FLOAT *value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%p)\n", This, value);
    return get_textfont_propf(This, FONT_SPACING, value);
}

static HRESULT WINAPI TextFont_SetSpacing(ITextFont *iface, FLOAT value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%.2f)\n", This, value);
    return set_textfont_propf(This, FONT_SPACING, value);
}

static HRESULT WINAPI TextFont_GetStrikeThrough(ITextFont *iface, LONG *value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%p)\n", This, value);
    return get_textfont_propl(This, FONT_STRIKETHROUGH, value);
}

static HRESULT WINAPI TextFont_SetStrikeThrough(ITextFont *iface, LONG value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%ld)\n", This, value);
    return set_textfont_propd(This, FONT_STRIKETHROUGH, value);
}

static HRESULT WINAPI TextFont_GetSubscript(ITextFont *iface, LONG *value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%p)\n", This, value);
    return get_textfont_propl(This, FONT_SUBSCRIPT, value);
}

static HRESULT WINAPI TextFont_SetSubscript(ITextFont *iface, LONG value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%ld)\n", This, value);
    return set_textfont_propd(This, FONT_SUBSCRIPT, value);
}

static HRESULT WINAPI TextFont_GetSuperscript(ITextFont *iface, LONG *value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%p)\n", This, value);
    return get_textfont_propl(This, FONT_SUPERSCRIPT, value);
}

static HRESULT WINAPI TextFont_SetSuperscript(ITextFont *iface, LONG value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%ld)\n", This, value);
    return set_textfont_propd(This, FONT_SUPERSCRIPT, value);
}

static HRESULT WINAPI TextFont_GetUnderline(ITextFont *iface, LONG *value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%p)\n", This, value);
    return get_textfont_propl(This, FONT_UNDERLINE, value);
}

static HRESULT WINAPI TextFont_SetUnderline(ITextFont *iface, LONG value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%ld)\n", This, value);
    return set_textfont_propd(This, FONT_UNDERLINE, value);
}

static HRESULT WINAPI TextFont_GetWeight(ITextFont *iface, LONG *value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%p)\n", This, value);
    return get_textfont_propl(This, FONT_WEIGHT, value);
}

static HRESULT WINAPI TextFont_SetWeight(ITextFont *iface, LONG value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    TRACE("(%p)->(%ld)\n", This, value);
    return set_textfont_propl(This, FONT_WEIGHT, value);
}

static ITextFontVtbl textfontvtbl = {
    TextFont_QueryInterface,
    TextFont_AddRef,
    TextFont_Release,
    TextFont_GetTypeInfoCount,
    TextFont_GetTypeInfo,
    TextFont_GetIDsOfNames,
    TextFont_Invoke,
    TextFont_GetDuplicate,
    TextFont_SetDuplicate,
    TextFont_CanChange,
    TextFont_IsEqual,
    TextFont_Reset,
    TextFont_GetStyle,
    TextFont_SetStyle,
    TextFont_GetAllCaps,
    TextFont_SetAllCaps,
    TextFont_GetAnimation,
    TextFont_SetAnimation,
    TextFont_GetBackColor,
    TextFont_SetBackColor,
    TextFont_GetBold,
    TextFont_SetBold,
    TextFont_GetEmboss,
    TextFont_SetEmboss,
    TextFont_GetForeColor,
    TextFont_SetForeColor,
    TextFont_GetHidden,
    TextFont_SetHidden,
    TextFont_GetEngrave,
    TextFont_SetEngrave,
    TextFont_GetItalic,
    TextFont_SetItalic,
    TextFont_GetKerning,
    TextFont_SetKerning,
    TextFont_GetLanguageID,
    TextFont_SetLanguageID,
    TextFont_GetName,
    TextFont_SetName,
    TextFont_GetOutline,
    TextFont_SetOutline,
    TextFont_GetPosition,
    TextFont_SetPosition,
    TextFont_GetProtected,
    TextFont_SetProtected,
    TextFont_GetShadow,
    TextFont_SetShadow,
    TextFont_GetSize,
    TextFont_SetSize,
    TextFont_GetSmallCaps,
    TextFont_SetSmallCaps,
    TextFont_GetSpacing,
    TextFont_SetSpacing,
    TextFont_GetStrikeThrough,
    TextFont_SetStrikeThrough,
    TextFont_GetSubscript,
    TextFont_SetSubscript,
    TextFont_GetSuperscript,
    TextFont_SetSuperscript,
    TextFont_GetUnderline,
    TextFont_SetUnderline,
    TextFont_GetWeight,
    TextFont_SetWeight
};

static HRESULT create_textfont(ITextRange *range, const ITextFontImpl *src, ITextFont **ret)
{
    ITextFontImpl *font;

    *ret = NULL;
    font = malloc(sizeof(*font));
    if (!font)
        return E_OUTOFMEMORY;

    font->ITextFont_iface.lpVtbl = &textfontvtbl;
    font->ref = 1;

    if (src) {
        font->range = NULL;
        font->get_cache_enabled = TRUE;
        font->set_cache_enabled = TRUE;
        memcpy(&font->props, &src->props, sizeof(font->props));
        if (font->props[FONT_NAME].str)
            font->props[FONT_NAME].str = SysAllocString(font->props[FONT_NAME].str);
    }
    else {
        font->range = range;
        ITextRange_AddRef(range);

        /* cache current properties */
        font->get_cache_enabled = FALSE;
        font->set_cache_enabled = FALSE;
        textfont_cache_range_props(font);
    }

    *ret = &font->ITextFont_iface;
    return S_OK;
}

/* ITextPara */
static HRESULT WINAPI TextPara_QueryInterface(ITextPara *iface, REFIID riid, void **ppv)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    if (IsEqualIID(riid, &IID_ITextPara) ||
        IsEqualIID(riid, &IID_IDispatch) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *ppv = iface;
        ITextPara_AddRef(iface);
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI TextPara_AddRef(ITextPara *iface)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%lu)\n", This, ref);
    return ref;
}

static ULONG WINAPI TextPara_Release(ITextPara *iface)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%lu)\n", This, ref);

    if (!ref)
    {
        ITextRange_Release(This->range);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI TextPara_GetTypeInfoCount(ITextPara *iface, UINT *pctinfo)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    TRACE("(%p)->(%p)\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI TextPara_GetTypeInfo(ITextPara *iface, UINT iTInfo, LCID lcid,
    ITypeInfo **ppTInfo)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    HRESULT hr;

    TRACE("(%p)->(%u,%ld,%p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(ITextPara_tid, ppTInfo);
    if (SUCCEEDED(hr))
        ITypeInfo_AddRef(*ppTInfo);
    return hr;
}

static HRESULT WINAPI TextPara_GetIDsOfNames(ITextPara *iface, REFIID riid,
    LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    ITypeInfo *ti;
    HRESULT hr;

    TRACE("(%p)->(%s, %p, %u, %ld, %p)\n", This, debugstr_guid(riid), rgszNames,
            cNames, lcid, rgDispId);

    hr = get_typeinfo(ITextPara_tid, &ti);
    if (SUCCEEDED(hr))
        hr = ITypeInfo_GetIDsOfNames(ti, rgszNames, cNames, rgDispId);
    return hr;
}

static HRESULT WINAPI TextPara_Invoke(
    ITextPara *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    ITypeInfo *ti;
    HRESULT hr;

    TRACE("(%p)->(%ld, %s, %ld, %u, %p, %p, %p, %p)\n", This, dispIdMember,
            debugstr_guid(riid), lcid, wFlags, pDispParams, pVarResult,
            pExcepInfo, puArgErr);

    hr = get_typeinfo(ITextPara_tid, &ti);
    if (SUCCEEDED(hr))
        hr = ITypeInfo_Invoke(ti, iface, dispIdMember, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return hr;
}

static HRESULT WINAPI TextPara_GetDuplicate(ITextPara *iface, ITextPara **ret)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, ret);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetDuplicate(ITextPara *iface, ITextPara *para)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, para);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_CanChange(ITextPara *iface, LONG *ret)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, ret);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_IsEqual(ITextPara *iface, ITextPara *para, LONG *ret)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p %p)\n", This, para, ret);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_Reset(ITextPara *iface, LONG value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%ld)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetStyle(ITextPara *iface, LONG *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetStyle(ITextPara *iface, LONG value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%ld)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetAlignment(ITextPara *iface, LONG *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetAlignment(ITextPara *iface, LONG value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%ld)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetHyphenation(ITextPara *iface, LONG *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetHyphenation(ITextPara *iface, LONG value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%ld)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetFirstLineIndent(ITextPara *iface, FLOAT *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetKeepTogether(ITextPara *iface, LONG *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetKeepTogether(ITextPara *iface, LONG value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%ld)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetKeepWithNext(ITextPara *iface, LONG *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetKeepWithNext(ITextPara *iface, LONG value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%ld)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetLeftIndent(ITextPara *iface, FLOAT *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetLineSpacing(ITextPara *iface, FLOAT *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetLineSpacingRule(ITextPara *iface, LONG *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetListAlignment(ITextPara *iface, LONG *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetListAlignment(ITextPara *iface, LONG value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%ld)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetListLevelIndex(ITextPara *iface, LONG *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetListLevelIndex(ITextPara *iface, LONG value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%ld)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetListStart(ITextPara *iface, LONG *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetListStart(ITextPara *iface, LONG value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%ld)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetListTab(ITextPara *iface, FLOAT *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetListTab(ITextPara *iface, FLOAT value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%.2f)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetListType(ITextPara *iface, LONG *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetListType(ITextPara *iface, LONG value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%ld)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetNoLineNumber(ITextPara *iface, LONG *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetNoLineNumber(ITextPara *iface, LONG value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%ld)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetPageBreakBefore(ITextPara *iface, LONG *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetPageBreakBefore(ITextPara *iface, LONG value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%ld)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetRightIndent(ITextPara *iface, FLOAT *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetRightIndent(ITextPara *iface, FLOAT value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%.2f)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetIndents(ITextPara *iface, FLOAT StartIndent, FLOAT LeftIndent, FLOAT RightIndent)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%.2f %.2f %.2f)\n", This, StartIndent, LeftIndent, RightIndent);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetLineSpacing(ITextPara *iface, LONG LineSpacingRule, FLOAT LineSpacing)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%ld %.2f)\n", This, LineSpacingRule, LineSpacing);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetSpaceAfter(ITextPara *iface, FLOAT *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetSpaceAfter(ITextPara *iface, FLOAT value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%.2f)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetSpaceBefore(ITextPara *iface, FLOAT *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetSpaceBefore(ITextPara *iface, FLOAT value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%.2f)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetWidowControl(ITextPara *iface, LONG *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetWidowControl(ITextPara *iface, LONG value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%ld)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetTabCount(ITextPara *iface, LONG *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_AddTab(ITextPara *iface, FLOAT tbPos, LONG tbAlign, LONG tbLeader)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%.2f %ld %ld)\n", This, tbPos, tbAlign, tbLeader);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_ClearAllTabs(ITextPara *iface)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_DeleteTab(ITextPara *iface, FLOAT pos)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%.2f)\n", This, pos);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetTab(ITextPara *iface, LONG iTab, FLOAT *ptbPos, LONG *ptbAlign, LONG *ptbLeader)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%ld %p %p %p)\n", This, iTab, ptbPos, ptbAlign, ptbLeader);
    return E_NOTIMPL;
}

static ITextParaVtbl textparavtbl = {
    TextPara_QueryInterface,
    TextPara_AddRef,
    TextPara_Release,
    TextPara_GetTypeInfoCount,
    TextPara_GetTypeInfo,
    TextPara_GetIDsOfNames,
    TextPara_Invoke,
    TextPara_GetDuplicate,
    TextPara_SetDuplicate,
    TextPara_CanChange,
    TextPara_IsEqual,
    TextPara_Reset,
    TextPara_GetStyle,
    TextPara_SetStyle,
    TextPara_GetAlignment,
    TextPara_SetAlignment,
    TextPara_GetHyphenation,
    TextPara_SetHyphenation,
    TextPara_GetFirstLineIndent,
    TextPara_GetKeepTogether,
    TextPara_SetKeepTogether,
    TextPara_GetKeepWithNext,
    TextPara_SetKeepWithNext,
    TextPara_GetLeftIndent,
    TextPara_GetLineSpacing,
    TextPara_GetLineSpacingRule,
    TextPara_GetListAlignment,
    TextPara_SetListAlignment,
    TextPara_GetListLevelIndex,
    TextPara_SetListLevelIndex,
    TextPara_GetListStart,
    TextPara_SetListStart,
    TextPara_GetListTab,
    TextPara_SetListTab,
    TextPara_GetListType,
    TextPara_SetListType,
    TextPara_GetNoLineNumber,
    TextPara_SetNoLineNumber,
    TextPara_GetPageBreakBefore,
    TextPara_SetPageBreakBefore,
    TextPara_GetRightIndent,
    TextPara_SetRightIndent,
    TextPara_SetIndents,
    TextPara_SetLineSpacing,
    TextPara_GetSpaceAfter,
    TextPara_SetSpaceAfter,
    TextPara_GetSpaceBefore,
    TextPara_SetSpaceBefore,
    TextPara_GetWidowControl,
    TextPara_SetWidowControl,
    TextPara_GetTabCount,
    TextPara_AddTab,
    TextPara_ClearAllTabs,
    TextPara_DeleteTab,
    TextPara_GetTab
};

static HRESULT create_textpara(ITextRange *range, ITextPara **ret)
{
    ITextParaImpl *para;

    *ret = NULL;
    para = malloc(sizeof(*para));
    if (!para)
        return E_OUTOFMEMORY;

    para->ITextPara_iface.lpVtbl = &textparavtbl;
    para->ref = 1;
    para->range = range;
    ITextRange_AddRef(range);

    *ret = &para->ITextPara_iface;
    return S_OK;
}

/* ITextDocument */
static HRESULT WINAPI ITextDocument2Old_fnQueryInterface(ITextDocument2Old* iface, REFIID riid,
                                                         void **ppvObject)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);
    return IUnknown_QueryInterface( services->outer_unk, riid, ppvObject );
}

static ULONG WINAPI ITextDocument2Old_fnAddRef(ITextDocument2Old *iface)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);
    return IUnknown_AddRef( services->outer_unk );
}

static ULONG WINAPI ITextDocument2Old_fnRelease(ITextDocument2Old *iface)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);
    return IUnknown_Release( services->outer_unk );
}

static HRESULT WINAPI ITextDocument2Old_fnGetTypeInfoCount(ITextDocument2Old *iface,
                                                           UINT *pctinfo)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);
    TRACE("(%p)->(%p)\n", services, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI ITextDocument2Old_fnGetTypeInfo(ITextDocument2Old *iface, UINT iTInfo, LCID lcid,
                                                      ITypeInfo **ppTInfo)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);
    HRESULT hr;

    TRACE("(%p)->(%u,%ld,%p)\n", services, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(ITextDocument_tid, ppTInfo);
    if (SUCCEEDED(hr))
        ITypeInfo_AddRef(*ppTInfo);
    return hr;
}

static HRESULT WINAPI ITextDocument2Old_fnGetIDsOfNames(ITextDocument2Old *iface, REFIID riid,
                                                        LPOLESTR *rgszNames, UINT cNames,
                                                        LCID lcid, DISPID *rgDispId)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);
    ITypeInfo *ti;
    HRESULT hr;

    TRACE("(%p)->(%s, %p, %u, %ld, %p)\n", services, debugstr_guid(riid),
            rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo(ITextDocument_tid, &ti);
    if (SUCCEEDED(hr))
        hr = ITypeInfo_GetIDsOfNames(ti, rgszNames, cNames, rgDispId);
    return hr;
}

static HRESULT WINAPI ITextDocument2Old_fnInvoke(ITextDocument2Old *iface, DISPID dispIdMember,
                                                 REFIID riid, LCID lcid, WORD wFlags,
                                                 DISPPARAMS *pDispParams, VARIANT *pVarResult,
                                                 EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);
    ITypeInfo *ti;
    HRESULT hr;

    TRACE("(%p)->(%ld, %s, %ld, %u, %p, %p, %p, %p)\n", services, dispIdMember,
            debugstr_guid(riid), lcid, wFlags, pDispParams, pVarResult,
            pExcepInfo, puArgErr);

    hr = get_typeinfo(ITextDocument_tid, &ti);
    if (SUCCEEDED(hr))
        hr = ITypeInfo_Invoke(ti, iface, dispIdMember, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return hr;
}

static HRESULT WINAPI ITextDocument2Old_fnGetName(ITextDocument2Old *iface, BSTR *pName)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);
    FIXME("stub %p\n", services);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnGetSelection(ITextDocument2Old *iface, ITextSelection **selection)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);

    TRACE("(%p)->(%p)\n", iface, selection);

    if (!selection)
      return E_INVALIDARG;

    if (!services->text_selection)
    {
        services->text_selection = text_selection_create( services );
        if (!services->text_selection)
        {
            *selection = NULL;
            return E_OUTOFMEMORY;
        }
    }

    *selection = &services->text_selection->ITextSelection_iface;
    ITextSelection_AddRef(*selection);
    return S_OK;
}

static HRESULT WINAPI ITextDocument2Old_fnGetStoryCount(ITextDocument2Old *iface, LONG *pCount)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);
    FIXME("stub %p\n", services);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnGetStoryRanges(ITextDocument2Old *iface,
                                                         ITextStoryRanges **ppStories)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);
    FIXME("stub %p\n", services);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnGetSaved(ITextDocument2Old *iface, LONG *pValue)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);
    FIXME("stub %p\n", services);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnSetSaved(ITextDocument2Old *iface, LONG Value)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);
    FIXME("stub %p\n", services);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnGetDefaultTabStop(ITextDocument2Old *iface, float *pValue)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);
    FIXME("stub %p\n", services);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnSetDefaultTabStop(ITextDocument2Old *iface, float Value)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);
    FIXME("stub %p\n", services);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnNew(ITextDocument2Old *iface)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);
    FIXME("stub %p\n", services);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnOpen(ITextDocument2Old *iface, VARIANT *pVar,
                                               LONG Flags, LONG CodePage)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);
    FIXME("stub %p\n", services);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnSave(ITextDocument2Old *iface, VARIANT *pVar,
                                               LONG Flags, LONG CodePage)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);
    FIXME("stub %p\n", services);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnFreeze(ITextDocument2Old *iface, LONG *pCount)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);

    if (services->editor->freeze_count < LONG_MAX) services->editor->freeze_count++;

    if (pCount) *pCount = services->editor->freeze_count;
    return services->editor->freeze_count != 0 ? S_OK : S_FALSE;
}

static HRESULT WINAPI ITextDocument2Old_fnUnfreeze(ITextDocument2Old *iface, LONG *pCount)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);

    if (services->editor->freeze_count && !--services->editor->freeze_count)
        ME_RewrapRepaint(services->editor);

    if (pCount) *pCount = services->editor->freeze_count;
    return services->editor->freeze_count == 0 ? S_OK : S_FALSE;
}

static HRESULT WINAPI ITextDocument2Old_fnBeginEditCollection(ITextDocument2Old *iface)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);
    FIXME("stub %p\n", services);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnEndEditCollection(ITextDocument2Old *iface)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);
    FIXME("stub %p\n", services);
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnUndo(ITextDocument2Old *iface, LONG Count, LONG *prop)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);
    LONG actual_undo_count;

    if (prop) *prop = 0;

    switch (Count)
    {
    case tomFalse:
        editor_disable_undo(services->editor);
        return S_OK;
    default:
        if (Count > 0) break;
        /* fallthrough */
    case tomTrue:
        editor_enable_undo(services->editor);
        return S_FALSE;
    case tomSuspend:
        if (services->editor->undo_ctl_state == undoActive)
        {
            services->editor->undo_ctl_state = undoSuspended;
        }
        return S_FALSE;
    case tomResume:
        services->editor->undo_ctl_state = undoActive;
        return S_FALSE;
    }

    for (actual_undo_count = 0; actual_undo_count < Count; actual_undo_count++)
    {
        if (!ME_Undo(services->editor)) break;
    }

    if (prop) *prop = actual_undo_count;
    return actual_undo_count == Count ? S_OK : S_FALSE;
}

static HRESULT WINAPI ITextDocument2Old_fnRedo(ITextDocument2Old *iface, LONG Count, LONG *prop)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);
    LONG actual_redo_count;

    if (prop) *prop = 0;

    for (actual_redo_count = 0; actual_redo_count < Count; actual_redo_count++)
    {
        if (!ME_Redo(services->editor)) break;
    }

    if (prop) *prop = actual_redo_count;
    return actual_redo_count == Count ? S_OK : S_FALSE;
}

static HRESULT CreateITextRange(struct text_services *services, LONG start, LONG end, ITextRange** ppRange)
{
    ITextRangeImpl *txtRge = malloc(sizeof(ITextRangeImpl));

    if (!txtRge)
        return E_OUTOFMEMORY;
    txtRge->ITextRange_iface.lpVtbl = &trvt;
    txtRge->ref = 1;
    txtRge->child.reole = services;
    txtRge->start = start;
    txtRge->end = end;
    list_add_head( &services->rangelist, &txtRge->child.entry );
    *ppRange = &txtRge->ITextRange_iface;
    return S_OK;
}

static HRESULT WINAPI ITextDocument2Old_fnRange(ITextDocument2Old *iface, LONG cp1, LONG cp2,
                                                ITextRange **ppRange)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);

    TRACE("%p %p %ld %ld\n", services, ppRange, cp1, cp2);
    if (!ppRange)
        return E_INVALIDARG;

    cp2range(services->editor, &cp1, &cp2);
    return CreateITextRange(services, cp1, cp2, ppRange);
}

static HRESULT WINAPI ITextDocument2Old_fnRangeFromPoint(ITextDocument2Old *iface, LONG x, LONG y,
                                                         ITextRange **ppRange)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);
    FIXME("stub %p\n", services);
    return E_NOTIMPL;
}

/* ITextDocument2Old methods */
static HRESULT WINAPI ITextDocument2Old_fnAttachMsgFilter(ITextDocument2Old *iface, IUnknown *filter)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);

    FIXME("(%p)->(%p): stub\n", services, filter);

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnSetEffectColor(ITextDocument2Old *iface, LONG index, COLORREF cr)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);

    FIXME("(%p)->(%ld, 0x%lx): stub\n", services, index, cr);

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnGetEffectColor(ITextDocument2Old *iface, LONG index, COLORREF *cr)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);

    FIXME("(%p)->(%ld, %p): stub\n", services, index, cr);

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnGetCaretType(ITextDocument2Old *iface, LONG *type)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);

    FIXME("(%p)->(%p): stub\n", services, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnSetCaretType(ITextDocument2Old *iface, LONG type)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);

    FIXME("(%p)->(%ld): stub\n", services, type);

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnGetImmContext(ITextDocument2Old *iface, LONG *context)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);

    FIXME("(%p)->(%p): stub\n", services, context);

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnReleaseImmContext(ITextDocument2Old *iface, LONG context)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);

    FIXME("(%p)->(%ld): stub\n", services, context);

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnGetPreferredFont(ITextDocument2Old *iface, LONG cp, LONG charrep,
                                                           LONG options, LONG current_charrep, LONG current_fontsize,
                                                           BSTR *bstr, LONG *pitch_family, LONG *new_fontsize)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);

    FIXME("(%p)->(%ld, %ld, %ld, %ld, %ld, %p, %p, %p): stub\n", services, cp, charrep, options, current_charrep,
          current_fontsize, bstr, pitch_family, new_fontsize);

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnGetNotificationMode(ITextDocument2Old *iface, LONG *mode)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);

    FIXME("(%p)->(%p): stub\n", services, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnSetNotificationMode(ITextDocument2Old *iface, LONG mode)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);

    FIXME("(%p)->(0x%lx): stub\n", services, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnGetClientRect(ITextDocument2Old *iface, LONG type, LONG *left, LONG *top,
                                                        LONG *right, LONG *bottom)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);

    FIXME("(%p)->(%ld, %p, %p, %p, %p): stub\n", services, type, left, top, right, bottom);

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnGetSelectionEx(ITextDocument2Old *iface, ITextSelection **selection)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);

    FIXME("(%p)->(%p): stub\n", services, selection);

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnGetWindow(ITextDocument2Old *iface, LONG *hwnd)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);

    FIXME("(%p)->(%p): stub\n", services, hwnd);

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnGetFEFlags(ITextDocument2Old *iface, LONG *flags)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);

    FIXME("(%p)->(%p): stub\n", services, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnUpdateWindow(ITextDocument2Old *iface)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);

    FIXME("(%p): stub\n", services);

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnCheckTextLimit(ITextDocument2Old *iface, LONG cch, LONG *exceed)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);

    FIXME("(%p)->(%ld, %p): stub\n", services, cch, exceed);

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnIMEInProgress(ITextDocument2Old *iface, LONG mode)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);

    FIXME("(%p)->(0x%lx): stub\n", services, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnSysBeep(ITextDocument2Old *iface)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);

    FIXME("(%p): stub\n", services);

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnUpdate(ITextDocument2Old *iface, LONG mode)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);

    FIXME("(%p)->(0x%lx): stub\n", services, mode);

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextDocument2Old_fnNotify(ITextDocument2Old *iface, LONG notify)
{
    struct text_services *services = impl_from_ITextDocument2Old(iface);

    FIXME("(%p)->(%ld): stub\n", services, notify);

    return E_NOTIMPL;
}

const ITextDocument2OldVtbl text_doc2old_vtbl =
{
    ITextDocument2Old_fnQueryInterface,
    ITextDocument2Old_fnAddRef,
    ITextDocument2Old_fnRelease,
    ITextDocument2Old_fnGetTypeInfoCount,
    ITextDocument2Old_fnGetTypeInfo,
    ITextDocument2Old_fnGetIDsOfNames,
    ITextDocument2Old_fnInvoke,
    ITextDocument2Old_fnGetName,
    ITextDocument2Old_fnGetSelection,
    ITextDocument2Old_fnGetStoryCount,
    ITextDocument2Old_fnGetStoryRanges,
    ITextDocument2Old_fnGetSaved,
    ITextDocument2Old_fnSetSaved,
    ITextDocument2Old_fnGetDefaultTabStop,
    ITextDocument2Old_fnSetDefaultTabStop,
    ITextDocument2Old_fnNew,
    ITextDocument2Old_fnOpen,
    ITextDocument2Old_fnSave,
    ITextDocument2Old_fnFreeze,
    ITextDocument2Old_fnUnfreeze,
    ITextDocument2Old_fnBeginEditCollection,
    ITextDocument2Old_fnEndEditCollection,
    ITextDocument2Old_fnUndo,
    ITextDocument2Old_fnRedo,
    ITextDocument2Old_fnRange,
    ITextDocument2Old_fnRangeFromPoint,
    /* ITextDocument2Old methods */
    ITextDocument2Old_fnAttachMsgFilter,
    ITextDocument2Old_fnSetEffectColor,
    ITextDocument2Old_fnGetEffectColor,
    ITextDocument2Old_fnGetCaretType,
    ITextDocument2Old_fnSetCaretType,
    ITextDocument2Old_fnGetImmContext,
    ITextDocument2Old_fnReleaseImmContext,
    ITextDocument2Old_fnGetPreferredFont,
    ITextDocument2Old_fnGetNotificationMode,
    ITextDocument2Old_fnSetNotificationMode,
    ITextDocument2Old_fnGetClientRect,
    ITextDocument2Old_fnGetSelectionEx,
    ITextDocument2Old_fnGetWindow,
    ITextDocument2Old_fnGetFEFlags,
    ITextDocument2Old_fnUpdateWindow,
    ITextDocument2Old_fnCheckTextLimit,
    ITextDocument2Old_fnIMEInProgress,
    ITextDocument2Old_fnSysBeep,
    ITextDocument2Old_fnUpdate,
    ITextDocument2Old_fnNotify
};

/* ITextSelection */
static HRESULT WINAPI ITextSelection_fnQueryInterface(
    ITextSelection *me,
    REFIID riid,
    void **ppvObj)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    *ppvObj = NULL;
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDispatch)
        || IsEqualGUID(riid, &IID_ITextRange)
        || IsEqualGUID(riid, &IID_ITextSelection))
    {
        *ppvObj = me;
        ITextSelection_AddRef(me);
        return S_OK;
    }
    else if (IsEqualGUID(riid, &IID_Igetrichole))
    {
        *ppvObj = This->services;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI ITextSelection_fnAddRef(ITextSelection *me)
{
    struct text_selection *This = impl_from_ITextSelection(me);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ITextSelection_fnRelease(ITextSelection *me)
{
    struct text_selection *This = impl_from_ITextSelection(me);
    ULONG ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
        free(This);
    return ref;
}

static HRESULT WINAPI ITextSelection_fnGetTypeInfoCount(ITextSelection *me, UINT *pctinfo)
{
    struct text_selection *This = impl_from_ITextSelection(me);
    TRACE("(%p)->(%p)\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI ITextSelection_fnGetTypeInfo(ITextSelection *me, UINT iTInfo, LCID lcid,
    ITypeInfo **ppTInfo)
{
    struct text_selection *This = impl_from_ITextSelection(me);
    HRESULT hr;

    TRACE("(%p)->(%u,%ld,%p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(ITextSelection_tid, ppTInfo);
    if (SUCCEEDED(hr))
        ITypeInfo_AddRef(*ppTInfo);
    return hr;
}

static HRESULT WINAPI ITextSelection_fnGetIDsOfNames(ITextSelection *me, REFIID riid,
    LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    struct text_selection *This = impl_from_ITextSelection(me);
    ITypeInfo *ti;
    HRESULT hr;

    TRACE("(%p)->(%s, %p, %u, %ld, %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid,
            rgDispId);

    hr = get_typeinfo(ITextSelection_tid, &ti);
    if (SUCCEEDED(hr))
        hr = ITypeInfo_GetIDsOfNames(ti, rgszNames, cNames, rgDispId);
    return hr;
}

static HRESULT WINAPI ITextSelection_fnInvoke(
    ITextSelection *me,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr)
{
    struct text_selection *This = impl_from_ITextSelection(me);
    ITypeInfo *ti;
    HRESULT hr;

    TRACE("(%p)->(%ld, %s, %ld, %u, %p, %p, %p, %p)\n", This, dispIdMember, debugstr_guid(riid), lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(ITextSelection_tid, &ti);
    if (SUCCEEDED(hr))
        hr = ITypeInfo_Invoke(ti, me, dispIdMember, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return hr;
}

/*** ITextRange methods ***/
static HRESULT WINAPI ITextSelection_fnGetText(ITextSelection *me, BSTR *pbstr)
{
    struct text_selection *This = impl_from_ITextSelection(me);
    ME_Cursor *start = NULL, *end = NULL;
    int nChars, endOfs;
    BOOL bEOP;

    TRACE("(%p)->(%p)\n", This, pbstr);

    if (!This->services)
        return CO_E_RELEASED;

    if (!pbstr)
        return E_INVALIDARG;

    ME_GetSelection(This->services->editor, &start, &end);
    endOfs = ME_GetCursorOfs(end);
    nChars = endOfs - ME_GetCursorOfs(start);
    if (!nChars)
    {
        *pbstr = NULL;
        return S_OK;
    }

    *pbstr = SysAllocStringLen(NULL, nChars);
    if (!*pbstr)
        return E_OUTOFMEMORY;

    bEOP = (!para_next( para_next( end->para ) ) && endOfs > ME_GetTextLength(This->services->editor));
    ME_GetTextW(This->services->editor, *pbstr, nChars, start, nChars, FALSE, bEOP);
    TRACE("%s\n", wine_dbgstr_w(*pbstr));

    return S_OK;
}

static HRESULT WINAPI ITextSelection_fnSetText(ITextSelection *me, BSTR str)
{
    struct text_selection *This = impl_from_ITextSelection(me);
    ME_TextEditor *editor;
    int len;
    LONG to, from;

    TRACE("(%p)->(%s)\n", This, debugstr_w(str));

    if (!This->services)
        return CO_E_RELEASED;

    editor = This->services->editor;
    len = lstrlenW(str);
    ME_GetSelectionOfs(editor, &from, &to);
    ME_ReplaceSel(editor, FALSE, str, len);

    if (len < to - from)
        textranges_update_ranges(This->services, from, len, RANGE_UPDATE_DELETE);

    return S_OK;
}

static HRESULT WINAPI ITextSelection_fnGetChar(ITextSelection *me, LONG *pch)
{
    struct text_selection *This = impl_from_ITextSelection(me);
    ME_Cursor *start = NULL, *end = NULL;

    TRACE("(%p)->(%p)\n", This, pch);

    if (!This->services)
        return CO_E_RELEASED;

    if (!pch)
        return E_INVALIDARG;

    ME_GetSelection(This->services->editor, &start, &end);
    return range_GetChar(This->services->editor, start, pch);
}

static HRESULT WINAPI ITextSelection_fnSetChar(ITextSelection *me, LONG ch)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%lx): stub\n", This, ch);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnGetDuplicate(ITextSelection *me, ITextRange **range)
{
    struct text_selection *This = impl_from_ITextSelection(me);
    LONG start, end;

    TRACE("(%p)->(%p)\n", This, range);

    if (!This->services)
        return CO_E_RELEASED;

    if (!range)
        return E_INVALIDARG;

    ITextSelection_GetStart(me, &start);
    ITextSelection_GetEnd(me, &end);
    return CreateITextRange(This->services, start, end, range);
}

static HRESULT WINAPI ITextSelection_fnGetFormattedText(ITextSelection *me, ITextRange **range)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%p): stub\n", This, range);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnSetFormattedText(ITextSelection *me, ITextRange *range)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%p): stub\n", This, range);

    if (!This->services)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnGetStart(ITextSelection *me, LONG *pcpFirst)
{
    struct text_selection *This = impl_from_ITextSelection(me);
    LONG lim;

    TRACE("(%p)->(%p)\n", This, pcpFirst);

    if (!This->services)
        return CO_E_RELEASED;

    if (!pcpFirst)
        return E_INVALIDARG;
    ME_GetSelectionOfs(This->services->editor, pcpFirst, &lim);
    return S_OK;
}

static HRESULT WINAPI ITextSelection_fnSetStart(ITextSelection *me, LONG value)
{
    struct text_selection *This = impl_from_ITextSelection(me);
    LONG start, end;
    HRESULT hr;

    TRACE("(%p)->(%ld)\n", This, value);

    if (!This->services)
        return CO_E_RELEASED;

    ME_GetSelectionOfs(This->services->editor, &start, &end);
    hr = textrange_setstart(This->services, value, &start, &end);
    if (hr == S_OK)
        set_selection(This->services->editor, start, end);

    return hr;
}

static HRESULT WINAPI ITextSelection_fnGetEnd(ITextSelection *me, LONG *pcpLim)
{
    struct text_selection *This = impl_from_ITextSelection(me);
    LONG first;

    TRACE("(%p)->(%p)\n", This, pcpLim);

    if (!This->services)
        return CO_E_RELEASED;

    if (!pcpLim)
        return E_INVALIDARG;
    ME_GetSelectionOfs(This->services->editor, &first, pcpLim);
    return S_OK;
}

static HRESULT WINAPI ITextSelection_fnSetEnd(ITextSelection *me, LONG value)
{
    struct text_selection *This = impl_from_ITextSelection(me);
    LONG start, end;
    HRESULT hr;

    TRACE("(%p)->(%ld)\n", This, value);

    if (!This->services)
        return CO_E_RELEASED;

    ME_GetSelectionOfs(This->services->editor, &start, &end);
    hr = textrange_setend(This->services, value, &start, &end);
    if (hr == S_OK)
        set_selection(This->services->editor, start, end);

    return hr;
}

static HRESULT WINAPI ITextSelection_fnGetFont(ITextSelection *me, ITextFont **font)
{
    struct text_selection *This = impl_from_ITextSelection(me);
    ITextRange *range = NULL;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, font);

    if (!This->services)
        return CO_E_RELEASED;

    if (!font)
        return E_INVALIDARG;

    ITextSelection_QueryInterface(me, &IID_ITextRange, (void**)&range);
    hr = create_textfont(range, NULL, font);
    ITextRange_Release(range);
    return hr;
}

static HRESULT WINAPI ITextSelection_fnSetFont(ITextSelection *me, ITextFont *font)
{
    struct text_selection *This = impl_from_ITextSelection(me);
    ITextRange *range = NULL;

    TRACE("(%p)->(%p)\n", This, font);

    if (!font)
        return E_INVALIDARG;

    if (!This->services)
        return CO_E_RELEASED;

    ITextSelection_QueryInterface(me, &IID_ITextRange, (void**)&range);
    textrange_set_font(range, font);
    ITextRange_Release(range);
    return S_OK;
}

static HRESULT WINAPI ITextSelection_fnGetPara(ITextSelection *me, ITextPara **para)
{
    struct text_selection *This = impl_from_ITextSelection(me);
    ITextRange *range = NULL;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, para);

    if (!This->services)
        return CO_E_RELEASED;

    if (!para)
        return E_INVALIDARG;

    ITextSelection_QueryInterface(me, &IID_ITextRange, (void**)&range);
    hr = create_textpara(range, para);
    ITextRange_Release(range);
    return hr;
}

static HRESULT WINAPI ITextSelection_fnSetPara(ITextSelection *me, ITextPara *para)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%p): stub\n", This, para);

    if (!This->services)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnGetStoryLength(ITextSelection *me, LONG *length)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    TRACE("(%p)->(%p)\n", This, length);

    if (!This->services)
        return CO_E_RELEASED;

    return textrange_get_storylength(This->services->editor, length);
}

static HRESULT WINAPI ITextSelection_fnGetStoryType(ITextSelection *me, LONG *value)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    TRACE("(%p)->(%p)\n", This, value);

    if (!This->services)
        return CO_E_RELEASED;

    if (!value)
        return E_INVALIDARG;

    *value = tomUnknownStory;
    return S_OK;
}

static HRESULT WINAPI ITextSelection_fnCollapse(ITextSelection *me, LONG bStart)
{
    struct text_selection *This = impl_from_ITextSelection(me);
    LONG start, end;
    HRESULT hres;

    TRACE("(%p)->(%ld)\n", This, bStart);

    if (!This->services)
        return CO_E_RELEASED;

    ME_GetSelectionOfs(This->services->editor, &start, &end);
    hres = range_Collapse(bStart, &start, &end);
    if (SUCCEEDED(hres))
        set_selection(This->services->editor, start, end);
    return hres;
}

static HRESULT WINAPI ITextSelection_fnExpand(ITextSelection *me, LONG unit, LONG *delta)
{
    struct text_selection *This = impl_from_ITextSelection(me);
    ITextRange *range = NULL;
    HRESULT hr;

    TRACE("(%p)->(%ld %p)\n", This, unit, delta);

    if (!This->services)
        return CO_E_RELEASED;

    ITextSelection_QueryInterface(me, &IID_ITextRange, (void**)&range);
    hr = textrange_expand(range, unit, delta);
    ITextRange_Release(range);
    return hr;
}

static HRESULT WINAPI ITextSelection_fnGetIndex(ITextSelection *me, LONG unit, LONG *index)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%ld %p): stub\n", This, unit, index);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnSetIndex(ITextSelection *me, LONG unit, LONG index,
    LONG extend)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%ld %ld %ld): stub\n", This, unit, index, extend);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnSetRange(ITextSelection *me, LONG anchor, LONG active)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%ld %ld): stub\n", This, anchor, active);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnInRange(ITextSelection *me, ITextRange *range, LONG *ret)
{
    struct text_selection *This = impl_from_ITextSelection(me);
    ITextSelection *selection = NULL;
    LONG start, end;

    TRACE("(%p)->(%p %p)\n", This, range, ret);

    if (ret)
        *ret = tomFalse;

    if (!This->services)
        return CO_E_RELEASED;

    if (!range)
        return S_FALSE;

    ITextRange_QueryInterface(range, &IID_ITextSelection, (void**)&selection);
    if (!selection)
        return S_FALSE;
    ITextSelection_Release(selection);

    ITextSelection_GetStart(me, &start);
    ITextSelection_GetEnd(me, &end);
    return textrange_inrange(start, end, range, ret);
}

static HRESULT WINAPI ITextSelection_fnInStory(ITextSelection *me, ITextRange *range, LONG *ret)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%p %p): stub\n", This, range, ret);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnIsEqual(ITextSelection *me, ITextRange *range, LONG *ret)
{
    struct text_selection *This = impl_from_ITextSelection(me);
    ITextSelection *selection = NULL;
    LONG start, end;

    TRACE("(%p)->(%p %p)\n", This, range, ret);

    if (ret)
        *ret = tomFalse;

    if (!This->services)
        return CO_E_RELEASED;

    if (!range)
        return S_FALSE;

    ITextRange_QueryInterface(range, &IID_ITextSelection, (void**)&selection);
    if (!selection)
        return S_FALSE;
    ITextSelection_Release(selection);

    ITextSelection_GetStart(me, &start);
    ITextSelection_GetEnd(me, &end);
    return textrange_isequal(start, end, range, ret);
}

static HRESULT WINAPI ITextSelection_fnSelect(ITextSelection *me)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    TRACE("(%p)\n", This);

    if (!This->services)
        return CO_E_RELEASED;

    /* nothing to do */
    return S_OK;
}

static HRESULT WINAPI ITextSelection_fnStartOf(ITextSelection *me, LONG unit, LONG extend,
    LONG *delta)
{
    struct text_selection *This = impl_from_ITextSelection(me);
    ITextRange *range = NULL;
    HRESULT hr;

    TRACE("(%p)->(%ld %ld %p)\n", This, unit, extend, delta);

    if (!This->services)
        return CO_E_RELEASED;

    ITextSelection_QueryInterface(me, &IID_ITextRange, (void**)&range);
    hr = textrange_startof(range, unit, extend, delta);
    ITextRange_Release(range);
    return hr;
}

static HRESULT WINAPI ITextSelection_fnEndOf(ITextSelection *me, LONG unit, LONG extend,
    LONG *delta)
{
    struct text_selection *This = impl_from_ITextSelection(me);
    ITextRange *range = NULL;
    HRESULT hr;

    TRACE("(%p)->(%ld %ld %p)\n", This, unit, extend, delta);

    if (!This->services)
        return CO_E_RELEASED;

    ITextSelection_QueryInterface(me, &IID_ITextRange, (void**)&range);
    hr = textrange_endof(range, This->services->editor, unit, extend, delta);
    ITextRange_Release(range);
    return hr;
}

static HRESULT WINAPI ITextSelection_fnMove(ITextSelection *me, LONG unit, LONG count, LONG *delta)
{
    struct text_selection *This = impl_from_ITextSelection(me);
    ITextRange *range = NULL;
    HRESULT hr;

    TRACE("(%p)->(%ld %ld %p)\n", This, unit, count, delta);

    if (!This->services)
        return CO_E_RELEASED;

    ITextSelection_QueryInterface(me, &IID_ITextRange, (void**)&range);
    hr = textrange_movestart(range, This->services->editor, unit, count, delta);
    ITextRange_Release(range);
    return hr;
}

static HRESULT WINAPI ITextSelection_fnMoveStart(ITextSelection *me, LONG unit, LONG count,
    LONG *delta)
{
    struct text_selection *This = impl_from_ITextSelection(me);
    ITextRange *range = NULL;
    HRESULT hr;

    TRACE("(%p)->(%ld %ld %p)\n", This, unit, count, delta);

    if (!This->services)
        return CO_E_RELEASED;

    ITextSelection_QueryInterface(me, &IID_ITextRange, (void**)&range);
    hr = textrange_movestart(range, This->services->editor, unit, count, delta);
    ITextRange_Release(range);
    return hr;
}

static HRESULT WINAPI ITextSelection_fnMoveEnd(ITextSelection *me, LONG unit, LONG count,
    LONG *delta)
{
    struct text_selection *This = impl_from_ITextSelection(me);
    ITextRange *range = NULL;
    HRESULT hr;

    TRACE("(%p)->(%ld %ld %p)\n", This, unit, count, delta);

    if (!This->services)
        return CO_E_RELEASED;

    ITextSelection_QueryInterface(me, &IID_ITextRange, (void**)&range);
    hr = textrange_moveend(range, This->services->editor, unit, count, delta);
    ITextRange_Release(range);
    return hr;
}

static HRESULT WINAPI ITextSelection_fnMoveWhile(ITextSelection *me, VARIANT *charset, LONG count,
    LONG *delta)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%s %ld %p): stub\n", This, debugstr_variant(charset), count, delta);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveStartWhile(ITextSelection *me, VARIANT *charset, LONG count,
    LONG *delta)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%s %ld %p): stub\n", This, debugstr_variant(charset), count, delta);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveEndWhile(ITextSelection *me, VARIANT *charset, LONG count,
    LONG *delta)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%s %ld %p): stub\n", This, debugstr_variant(charset), count, delta);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveUntil(ITextSelection *me, VARIANT *charset, LONG count,
    LONG *delta)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%s %ld %p): stub\n", This, debugstr_variant(charset), count, delta);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveStartUntil(ITextSelection *me, VARIANT *charset, LONG count,
    LONG *delta)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%s %ld %p): stub\n", This, debugstr_variant(charset), count, delta);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveEndUntil(ITextSelection *me, VARIANT *charset, LONG count,
    LONG *delta)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%s %ld %p): stub\n", This, debugstr_variant(charset), count, delta);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnFindText(ITextSelection *me, BSTR text, LONG count, LONG flags,
    LONG *length)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%s %ld %lx %p): stub\n", This, debugstr_w(text), count, flags, length);

    if (!This->services)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnFindTextStart(ITextSelection *me, BSTR text, LONG count,
    LONG flags, LONG *length)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%s %ld %lx %p): stub\n", This, debugstr_w(text), count, flags, length);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnFindTextEnd(ITextSelection *me, BSTR text, LONG count,
    LONG flags, LONG *length)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%s %ld %lx %p): stub\n", This, debugstr_w(text), count, flags, length);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnDelete(ITextSelection *me, LONG unit, LONG count,
    LONG *delta)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%ld %ld %p): stub\n", This, unit, count, delta);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnCut(ITextSelection *me, VARIANT *v)
{
    struct text_selection *This = impl_from_ITextSelection(me);
    ITextRange *range = NULL;
    HRESULT hr;

    TRACE("(%p)->(%p): stub\n", This, v);

    if (!This->services)
        return CO_E_RELEASED;

    ITextSelection_QueryInterface(me, &IID_ITextRange, (void**)&range);
    hr = textrange_copy_or_cut(range, This->services->editor, TRUE, v);
    ITextRange_Release(range);
    return hr;
}

static HRESULT WINAPI ITextSelection_fnCopy(ITextSelection *me, VARIANT *v)
{
    struct text_selection *This = impl_from_ITextSelection(me);
    ITextRange *range = NULL;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, v);

    if (!This->services)
        return CO_E_RELEASED;

    ITextSelection_QueryInterface(me, &IID_ITextRange, (void**)&range);
    hr = textrange_copy_or_cut(range, This->services->editor, FALSE, v);
    ITextRange_Release(range);
    return hr;
}

static HRESULT WINAPI ITextSelection_fnPaste(ITextSelection *me, VARIANT *v, LONG format)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%s %lx): stub\n", This, debugstr_variant(v), format);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnCanPaste(ITextSelection *me, VARIANT *v, LONG format,
    LONG *ret)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%s %lx %p): stub\n", This, debugstr_variant(v), format, ret);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnCanEdit(ITextSelection *me, LONG *ret)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%p): stub\n", This, ret);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnChangeCase(ITextSelection *me, LONG type)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%ld): stub\n", This, type);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnGetPoint(ITextSelection *me, LONG type, LONG *cx, LONG *cy)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%ld %p %p): stub\n", This, type, cx, cy);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnSetPoint(ITextSelection *me, LONG x, LONG y, LONG type,
    LONG extend)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%ld %ld %ld %ld): stub\n", This, x, y, type, extend);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnScrollIntoView(ITextSelection *me, LONG value)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%ld): stub\n", This, value);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnGetEmbeddedObject(ITextSelection *me, IUnknown **ppv)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%p): stub\n", This, ppv);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

/*** ITextSelection methods ***/
static HRESULT WINAPI ITextSelection_fnGetFlags(ITextSelection *me, LONG *flags)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%p): stub\n", This, flags);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnSetFlags(ITextSelection *me, LONG flags)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%lx): stub\n", This, flags);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnGetType(ITextSelection *me, LONG *type)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%p): stub\n", This, type);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveLeft(ITextSelection *me, LONG unit, LONG count,
    LONG extend, LONG *delta)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%ld %ld %ld %p): stub\n", This, unit, count, extend, delta);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveRight(ITextSelection *me, LONG unit, LONG count,
    LONG extend, LONG *delta)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%ld %ld %ld %p): stub\n", This, unit, count, extend, delta);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveUp(ITextSelection *me, LONG unit, LONG count,
    LONG extend, LONG *delta)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%ld %ld %ld %p): stub\n", This, unit, count, extend, delta);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveDown(ITextSelection *me, LONG unit, LONG count,
    LONG extend, LONG *delta)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%ld %ld %ld %p): stub\n", This, unit, count, extend, delta);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnHomeKey(ITextSelection *me, LONG unit, LONG extend,
    LONG *delta)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%ld %ld %p): stub\n", This, unit, extend, delta);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnEndKey(ITextSelection *me, LONG unit, LONG extend,
    LONG *delta)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%ld %ld %p): stub\n", This, unit, extend, delta);

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnTypeText(ITextSelection *me, BSTR text)
{
    struct text_selection *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%s): stub\n", This, debugstr_w(text));

    if (!This->services)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static const ITextSelectionVtbl tsvt = {
    ITextSelection_fnQueryInterface,
    ITextSelection_fnAddRef,
    ITextSelection_fnRelease,
    ITextSelection_fnGetTypeInfoCount,
    ITextSelection_fnGetTypeInfo,
    ITextSelection_fnGetIDsOfNames,
    ITextSelection_fnInvoke,
    ITextSelection_fnGetText,
    ITextSelection_fnSetText,
    ITextSelection_fnGetChar,
    ITextSelection_fnSetChar,
    ITextSelection_fnGetDuplicate,
    ITextSelection_fnGetFormattedText,
    ITextSelection_fnSetFormattedText,
    ITextSelection_fnGetStart,
    ITextSelection_fnSetStart,
    ITextSelection_fnGetEnd,
    ITextSelection_fnSetEnd,
    ITextSelection_fnGetFont,
    ITextSelection_fnSetFont,
    ITextSelection_fnGetPara,
    ITextSelection_fnSetPara,
    ITextSelection_fnGetStoryLength,
    ITextSelection_fnGetStoryType,
    ITextSelection_fnCollapse,
    ITextSelection_fnExpand,
    ITextSelection_fnGetIndex,
    ITextSelection_fnSetIndex,
    ITextSelection_fnSetRange,
    ITextSelection_fnInRange,
    ITextSelection_fnInStory,
    ITextSelection_fnIsEqual,
    ITextSelection_fnSelect,
    ITextSelection_fnStartOf,
    ITextSelection_fnEndOf,
    ITextSelection_fnMove,
    ITextSelection_fnMoveStart,
    ITextSelection_fnMoveEnd,
    ITextSelection_fnMoveWhile,
    ITextSelection_fnMoveStartWhile,
    ITextSelection_fnMoveEndWhile,
    ITextSelection_fnMoveUntil,
    ITextSelection_fnMoveStartUntil,
    ITextSelection_fnMoveEndUntil,
    ITextSelection_fnFindText,
    ITextSelection_fnFindTextStart,
    ITextSelection_fnFindTextEnd,
    ITextSelection_fnDelete,
    ITextSelection_fnCut,
    ITextSelection_fnCopy,
    ITextSelection_fnPaste,
    ITextSelection_fnCanPaste,
    ITextSelection_fnCanEdit,
    ITextSelection_fnChangeCase,
    ITextSelection_fnGetPoint,
    ITextSelection_fnSetPoint,
    ITextSelection_fnScrollIntoView,
    ITextSelection_fnGetEmbeddedObject,
    ITextSelection_fnGetFlags,
    ITextSelection_fnSetFlags,
    ITextSelection_fnGetType,
    ITextSelection_fnMoveLeft,
    ITextSelection_fnMoveRight,
    ITextSelection_fnMoveUp,
    ITextSelection_fnMoveDown,
    ITextSelection_fnHomeKey,
    ITextSelection_fnEndKey,
    ITextSelection_fnTypeText
};

static struct text_selection *text_selection_create(struct text_services *services)
{
    struct text_selection *txtSel = malloc(sizeof *txtSel);
    if (!txtSel)
        return NULL;

    txtSel->ITextSelection_iface.lpVtbl = &tsvt;
    txtSel->ref = 1;
    txtSel->services = services;
    return txtSel;
}

static void convert_sizel(const ME_Context *c, const SIZEL* szl, SIZE* sz)
{
  /* sizel is in .01 millimeters, sz in pixels */
  sz->cx = MulDiv(szl->cx, c->dpi.cx, 2540);
  sz->cy = MulDiv(szl->cy, c->dpi.cy, 2540);
}

/******************************************************************************
 * ME_GetOLEObjectSize
 *
 * Sets run extent for OLE objects.
 */
void ME_GetOLEObjectSize(const ME_Context *c, ME_Run *run, SIZE *pSize)
{
  IDataObject*  ido;
  FORMATETC     fmt;
  STGMEDIUM     stgm;
  DIBSECTION    dibsect;
  ENHMETAHEADER emh;

  assert(run->nFlags & MERF_GRAPHICS);
  assert(run->reobj);

  if (run->reobj->obj.sizel.cx != 0 || run->reobj->obj.sizel.cy != 0)
  {
    convert_sizel(c, &run->reobj->obj.sizel, pSize);
    if (c->editor->nZoomNumerator != 0)
    {
      pSize->cx = MulDiv(pSize->cx, c->editor->nZoomNumerator, c->editor->nZoomDenominator);
      pSize->cy = MulDiv(pSize->cy, c->editor->nZoomNumerator, c->editor->nZoomDenominator);
    }
    return;
  }

  if (!run->reobj->obj.poleobj)
  {
    pSize->cx = pSize->cy = 0;
    return;
  }

  if (IOleObject_QueryInterface(run->reobj->obj.poleobj, &IID_IDataObject, (void**)&ido) != S_OK)
  {
      FIXME("Query Interface IID_IDataObject failed!\n");
      pSize->cx = pSize->cy = 0;
      return;
  }
  fmt.cfFormat = CF_BITMAP;
  fmt.ptd = NULL;
  fmt.dwAspect = DVASPECT_CONTENT;
  fmt.lindex = -1;
  fmt.tymed = TYMED_GDI;
  if (IDataObject_GetData(ido, &fmt, &stgm) != S_OK)
  {
    fmt.cfFormat = CF_ENHMETAFILE;
    fmt.tymed = TYMED_ENHMF;
    if (IDataObject_GetData(ido, &fmt, &stgm) != S_OK)
    {
      FIXME("unsupported format\n");
      pSize->cx = pSize->cy = 0;
      IDataObject_Release(ido);
      return;
    }
  }
  IDataObject_Release(ido);

  switch (stgm.tymed)
  {
  case TYMED_GDI:
    GetObjectW(stgm.hBitmap, sizeof(dibsect), &dibsect);
    pSize->cx = dibsect.dsBm.bmWidth;
    pSize->cy = dibsect.dsBm.bmHeight;
    break;
  case TYMED_ENHMF:
    GetEnhMetaFileHeader(stgm.hEnhMetaFile, sizeof(emh), &emh);
    pSize->cx = emh.rclBounds.right - emh.rclBounds.left;
    pSize->cy = emh.rclBounds.bottom - emh.rclBounds.top;
    break;
  default:
    FIXME("Unsupported tymed %ld\n", stgm.tymed);
    break;
  }
  ReleaseStgMedium(&stgm);
  if (c->editor->nZoomNumerator != 0)
  {
    pSize->cx = MulDiv(pSize->cx, c->editor->nZoomNumerator, c->editor->nZoomDenominator);
    pSize->cy = MulDiv(pSize->cy, c->editor->nZoomNumerator, c->editor->nZoomDenominator);
  }
}

void draw_ole( ME_Context *c, int x, int y, ME_Run *run, BOOL selected )
{
  IDataObject*  ido;
  IViewObject*  ivo;
  FORMATETC     fmt;
  STGMEDIUM     stgm;
  DIBSECTION    dibsect;
  ENHMETAHEADER emh;
  HDC           hMemDC;
  SIZE          sz;
  BOOL          has_size;
  HBITMAP       old_bm;
  RECT          rc;

  assert(run->nFlags & MERF_GRAPHICS);
  assert(run->reobj);

  if (!run->reobj->obj.poleobj) return;

  if (SUCCEEDED(IOleObject_QueryInterface(run->reobj->obj.poleobj, &IID_IViewObject, (void**)&ivo)))
  {
    HRESULT hr;
    RECTL bounds;

    convert_sizel(c, &run->reobj->obj.sizel, &sz);
    if (c->editor->nZoomNumerator != 0)
    {
      sz.cx = MulDiv(sz.cx, c->editor->nZoomNumerator, c->editor->nZoomDenominator);
      sz.cy = MulDiv(sz.cy, c->editor->nZoomNumerator, c->editor->nZoomDenominator);
    }

    bounds.left = x;
    bounds.top = y - sz.cy;
    bounds.right = x + sz.cx;
    bounds.bottom = y;

    hr = IViewObject_Draw(ivo, DVASPECT_CONTENT, -1, 0, 0, 0, c->hDC, &bounds, NULL, NULL, 0);
    if (FAILED(hr))
    {
      WARN("failed to draw object: %#08lx\n", hr);
    }

    IViewObject_Release(ivo);
    return;
  }

  if (IOleObject_QueryInterface(run->reobj->obj.poleobj, &IID_IDataObject, (void**)&ido) != S_OK)
  {
    FIXME("Couldn't get interface\n");
    return;
  }
  has_size = run->reobj->obj.sizel.cx != 0 || run->reobj->obj.sizel.cy != 0;
  fmt.cfFormat = CF_BITMAP;
  fmt.ptd = NULL;
  fmt.dwAspect = DVASPECT_CONTENT;
  fmt.lindex = -1;
  fmt.tymed = TYMED_GDI;
  if (IDataObject_GetData(ido, &fmt, &stgm) != S_OK)
  {
    fmt.cfFormat = CF_ENHMETAFILE;
    fmt.tymed = TYMED_ENHMF;
    if (IDataObject_GetData(ido, &fmt, &stgm) != S_OK)
    {
      FIXME("Couldn't get storage medium\n");
      IDataObject_Release(ido);
      return;
    }
  }
  IDataObject_Release(ido);

  switch (stgm.tymed)
  {
  case TYMED_GDI:
    GetObjectW(stgm.hBitmap, sizeof(dibsect), &dibsect);
    hMemDC = CreateCompatibleDC(c->hDC);
    old_bm = SelectObject(hMemDC, stgm.hBitmap);
    if (has_size)
    {
      convert_sizel(c, &run->reobj->obj.sizel, &sz);
    } else {
      sz.cx = dibsect.dsBm.bmWidth;
      sz.cy = dibsect.dsBm.bmHeight;
    }
    if (c->editor->nZoomNumerator != 0)
    {
      sz.cx = MulDiv(sz.cx, c->editor->nZoomNumerator, c->editor->nZoomDenominator);
      sz.cy = MulDiv(sz.cy, c->editor->nZoomNumerator, c->editor->nZoomDenominator);
    }
    StretchBlt(c->hDC, x, y - sz.cy, sz.cx, sz.cy,
               hMemDC, 0, 0, dibsect.dsBm.bmWidth, dibsect.dsBm.bmHeight, SRCCOPY);

    SelectObject(hMemDC, old_bm);
    DeleteDC(hMemDC);
    break;
  case TYMED_ENHMF:
    GetEnhMetaFileHeader(stgm.hEnhMetaFile, sizeof(emh), &emh);
    if (has_size)
    {
      convert_sizel(c, &run->reobj->obj.sizel, &sz);
    } else {
      sz.cx = emh.rclBounds.right - emh.rclBounds.left;
      sz.cy = emh.rclBounds.bottom - emh.rclBounds.top;
    }
    if (c->editor->nZoomNumerator != 0)
    {
      sz.cx = MulDiv(sz.cx, c->editor->nZoomNumerator, c->editor->nZoomDenominator);
      sz.cy = MulDiv(sz.cy, c->editor->nZoomNumerator, c->editor->nZoomDenominator);
    }

    rc.left = x;
    rc.top = y - sz.cy;
    rc.right = x + sz.cx;
    rc.bottom = y;
    PlayEnhMetaFile(c->hDC, stgm.hEnhMetaFile, &rc);
    break;
  default:
    FIXME("Unsupported tymed %ld\n", stgm.tymed);
    selected = FALSE;
    break;
  }
  ReleaseStgMedium(&stgm);

  if (selected && !c->editor->bHideSelection)
    PatBlt(c->hDC, x, y - sz.cy, sz.cx, sz.cy, DSTINVERT);
}

void ME_DeleteReObject(struct re_object *reobj)
{
    if (reobj->obj.poleobj)   IOleObject_Release(reobj->obj.poleobj);
    if (reobj->obj.pstg)      IStorage_Release(reobj->obj.pstg);
    if (reobj->obj.polesite)  IOleClientSite_Release(reobj->obj.polesite);
    free(reobj);
}

void ME_CopyReObject(REOBJECT *dst, const REOBJECT *src, DWORD flags)
{
    *dst = *src;
    dst->poleobj = NULL;
    dst->pstg = NULL;
    dst->polesite = NULL;

    if ((flags & REO_GETOBJ_POLEOBJ) && src->poleobj)
    {
        dst->poleobj = src->poleobj;
        IOleObject_AddRef(dst->poleobj);
    }
    if ((flags & REO_GETOBJ_PSTG) && src->pstg)
    {
        dst->pstg = src->pstg;
        IStorage_AddRef(dst->pstg);
    }
    if ((flags & REO_GETOBJ_POLESITE) && src->polesite)
    {
        dst->polesite = src->polesite;
        IOleClientSite_AddRef(dst->polesite);
    }
}

void richole_release_children( struct text_services *services )
{
    ITextRangeImpl *range;
    IOleClientSiteImpl *site;

    if (services->text_selection)
    {
        services->text_selection->services = NULL;
        ITextSelection_Release( &services->text_selection->ITextSelection_iface );
    }

    LIST_FOR_EACH_ENTRY( range, &services->rangelist, ITextRangeImpl, child.entry )
        range->child.reole = NULL;

    LIST_FOR_EACH_ENTRY( site, &services->clientsites, IOleClientSiteImpl, child.entry )
        site->child.reole = NULL;
}
