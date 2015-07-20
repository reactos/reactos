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

#include "editor.h"

WINE_DEFAULT_DEBUG_CHANNEL(richedit);

/* there is no way to be consistent across different sets of headers - mingw, Wine, Win32 SDK*/

#include <initguid.h>

DEFINE_GUID(LIBID_tom, 0x8cc497c9, 0xa1df, 0x11ce, 0x80, 0x98, 0x00, 0xaa, 0x00, 0x47, 0xbe, 0x5d);
DEFINE_GUID(IID_ITextServices, 0x8d33f740, 0xcf58, 0x11ce, 0xa8, 0x9d, 0x00, 0xaa, 0x00, 0x6c, 0xad, 0xc5);
DEFINE_GUID(IID_ITextHost, 0x13e670f4,0x1a5a,0x11cf,0xab,0xeb,0x00,0xaa,0x00,0xb6,0x5e,0xa1);
DEFINE_GUID(IID_ITextHost2, 0x13e670f5,0x1a5a,0x11cf,0xab,0xeb,0x00,0xaa,0x00,0xb6,0x5e,0xa1);
DEFINE_GUID(IID_ITextDocument, 0x8cc497c0, 0xa1df, 0x11ce, 0x80, 0x98, 0x00, 0xaa, 0x00, 0x47, 0xbe, 0x5d);
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
        ERR("LoadRegTypeLib failed: %08x\n", hr);
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

    for (i = 0; i < sizeof(typeinfos)/sizeof(*typeinfos); i++)
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
            ERR("GetTypeInfoOfGuid(%s) failed: %08x\n", debugstr_guid(tid_ids[tid]), hr);
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

typedef struct ITextSelectionImpl ITextSelectionImpl;
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

typedef struct IRichEditOleImpl {
    IUnknown IUnknown_inner;
    IRichEditOle IRichEditOle_iface;
    ITextDocument ITextDocument_iface;
    IUnknown *outer_unk;
    LONG ref;

    ME_TextEditor *editor;
    ITextSelectionImpl *txtSel;

    struct list rangelist;
    struct list clientsites;
} IRichEditOleImpl;

struct reole_child {
    struct list entry;
    IRichEditOleImpl *reole;
};

struct ITextRangeImpl {
    struct reole_child child;
    ITextRange ITextRange_iface;
    LONG ref;
    LONG start, end;
};

struct ITextSelectionImpl {
    ITextSelection ITextSelection_iface;
    LONG ref;

    IRichEditOleImpl *reOle;
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
    IOleWindow IOleWindow_iface;
    IOleInPlaceSite IOleInPlaceSite_iface;
    LONG ref;
};

static inline IRichEditOleImpl *impl_from_IRichEditOle(IRichEditOle *iface)
{
    return CONTAINING_RECORD(iface, IRichEditOleImpl, IRichEditOle_iface);
}

static inline IRichEditOleImpl *impl_from_ITextDocument(ITextDocument *iface)
{
    return CONTAINING_RECORD(iface, IRichEditOleImpl, ITextDocument_iface);
}

static inline IRichEditOleImpl *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, IRichEditOleImpl, IUnknown_inner);
}

static inline IOleClientSiteImpl *impl_from_IOleWindow(IOleWindow *iface)
{
    return CONTAINING_RECORD(iface, IOleClientSiteImpl, IOleWindow_iface);
}

static inline IOleClientSiteImpl *impl_from_IOleInPlaceSite(IOleInPlaceSite *iface)
{
    return CONTAINING_RECORD(iface, IOleClientSiteImpl, IOleInPlaceSite_iface);
}

static inline ITextRangeImpl *impl_from_ITextRange(ITextRange *iface)
{
    return CONTAINING_RECORD(iface, ITextRangeImpl, ITextRange_iface);
}

static inline ITextSelectionImpl *impl_from_ITextSelection(ITextSelection *iface)
{
    return CONTAINING_RECORD(iface, ITextSelectionImpl, ITextSelection_iface);
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
static ITextSelectionImpl *CreateTextSelection(IRichEditOleImpl*);

static HRESULT textrange_get_storylength(ME_TextEditor *editor, LONG *length)
{
    if (!length)
        return E_INVALIDARG;

    *length = ME_GetTextLength(editor) + 1;
    return S_OK;
}

static void textranges_update_ranges(IRichEditOleImpl *reole, LONG start, LONG end, enum range_update_op op)
{
    ITextRangeImpl *range;

    LIST_FOR_EACH_ENTRY(range, &reole->rangelist, ITextRangeImpl, child.entry) {
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
        return !strcmpW(left->str, right->str);
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

static HRESULT get_textfont_prop_for_pos(const IRichEditOleImpl *reole, int pos, enum textfont_prop_id propid,
    textfont_prop_val *value)
{
    ME_Cursor from, to;
    CHARFORMAT2W fmt;

    memset(&fmt, 0, sizeof(fmt));
    fmt.cbSize = sizeof(fmt);
    fmt.dwMask = textfont_prop_masks[propid][0];

    ME_CursorFromCharOfs(reole->editor, pos, &from);
    to = from;
    ME_MoveCursorChars(reole->editor, &to, 1);
    ME_GetCharFormat(reole->editor, &from, &to, &fmt);

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

static inline const IRichEditOleImpl *get_range_reole(ITextRange *range)
{
    IRichEditOleImpl *reole = NULL;
    ITextRange_QueryInterface(range, &IID_Igetrichole, (void**)&reole);
    return reole;
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
        lstrcpynW(fmt.szFaceName, str, sizeof(fmt.szFaceName)/sizeof(WCHAR));
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

    if (fmt.dwMask) {
        const IRichEditOleImpl *reole = get_range_reole(range);
        ME_Cursor from, to;
        LONG start, end;

        ITextRange_GetStart(range, &start);
        ITextRange_GetEnd(range, &end);

        ME_CursorFromCharOfs(reole->editor, start, &from);
        ME_CursorFromCharOfs(reole->editor, end, &to);
        ME_SetCharFormat(reole->editor, &from, &to, &fmt);
    }
}

static HRESULT get_textfont_prop(const ITextFontImpl *font, enum textfont_prop_id propid, textfont_prop_val *value)
{
    const IRichEditOleImpl *reole;
    textfont_prop_val v;
    LONG start, end, i;
    HRESULT hr;

    /* when font is not attached to any range use cached values */
    if (!font->range || font->get_cache_enabled) {
        *value = font->props[propid];
        return S_OK;
    }

    if (!(reole = get_range_reole(font->range)))
        return CO_E_RELEASED;

    init_textfont_prop_value(propid, value);

    ITextRange_GetStart(font->range, &start);
    ITextRange_GetEnd(font->range, &end);

    /* iterate trough a range to see if property value is consistent */
    hr = get_textfont_prop_for_pos(reole, start, propid, &v);
    if (FAILED(hr))
        return hr;

    for (i = start + 1; i < end; i++) {
        textfont_prop_val cur;

        hr = get_textfont_prop_for_pos(reole, i, propid, &cur);
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
    const IRichEditOleImpl *reole;
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

    if (!(reole = get_range_reole(font->range)))
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
        lstrcpynW(fmt.szFaceName, value->str, sizeof(fmt.szFaceName)/sizeof(WCHAR));
        break;
    default:
        FIXME("unhandled font property %d\n", propid);
        return E_FAIL;
    }

    ITextRange_GetStart(font->range, &start);
    ITextRange_GetEnd(font->range, &end);

    ME_CursorFromCharOfs(reole->editor, start, &from);
    ME_CursorFromCharOfs(reole->editor, end, &to);
    ME_SetCharFormat(reole->editor, &from, &to, &fmt);

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
    const IRichEditOleImpl *reole;
    textfont_prop_val v;
    HRESULT hr;
    LONG start;

    if (!(reole = get_range_reole(range)))
        return CO_E_RELEASED;

    ITextRange_GetStart(range, &start);
    hr = get_textfont_prop_for_pos(reole, start, FONT_NAME, &v);
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
        FIXME("unit %d is not supported\n", unit);
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

static HRESULT WINAPI IRichEditOleImpl_inner_fnQueryInterface(IUnknown *iface, REFIID riid, LPVOID *ppvObj)
{
    IRichEditOleImpl *This = impl_from_IUnknown(iface);

    TRACE("%p %s\n", This, debugstr_guid(riid));

    *ppvObj = NULL;
    if (IsEqualGUID(riid, &IID_IUnknown))
        *ppvObj = &This->IUnknown_inner;
    else if (IsEqualGUID(riid, &IID_IRichEditOle))
        *ppvObj = &This->IRichEditOle_iface;
    else if (IsEqualGUID(riid, &IID_ITextDocument))
        *ppvObj = &This->ITextDocument_iface;
    if (*ppvObj)
    {
        IUnknown_AddRef((IUnknown *)*ppvObj);
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_ITextServices))
    {
        static int once;
        if (!once++) FIXME("%p: unhandled interface IID_ITextServices\n", This);
        return E_NOINTERFACE;
    }

    FIXME("%p: unhandled interface %s\n", This, debugstr_guid(riid));
 
    return E_NOINTERFACE;   
}

static ULONG WINAPI IRichEditOleImpl_inner_fnAddRef(IUnknown *iface)
{
    IRichEditOleImpl *This = impl_from_IUnknown(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p ref = %u\n", This, ref);

    return ref;
}

static ULONG WINAPI IRichEditOleImpl_inner_fnRelease(IUnknown *iface)
{
    IRichEditOleImpl *This = impl_from_IUnknown(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE ("%p ref=%u\n", This, ref);

    if (!ref)
    {
        IOleClientSiteImpl *clientsite;
        ITextRangeImpl *txtRge;

        This->editor->reOle = NULL;
        if (This->txtSel) {
            This->txtSel->reOle = NULL;
            ITextSelection_Release(&This->txtSel->ITextSelection_iface);
        }

        LIST_FOR_EACH_ENTRY(txtRge, &This->rangelist, ITextRangeImpl, child.entry)
            txtRge->child.reole = NULL;

        LIST_FOR_EACH_ENTRY(clientsite, &This->clientsites, IOleClientSiteImpl, child.entry)
            clientsite->child.reole = NULL;

        heap_free(This);
    }
    return ref;
}

static const IUnknownVtbl reo_unk_vtbl =
{
    IRichEditOleImpl_inner_fnQueryInterface,
    IRichEditOleImpl_inner_fnAddRef,
    IRichEditOleImpl_inner_fnRelease
};

static HRESULT WINAPI
IRichEditOle_fnQueryInterface(IRichEditOle *me, REFIID riid, LPVOID *ppvObj)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppvObj);
}

static ULONG WINAPI
IRichEditOle_fnAddRef(IRichEditOle *me)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI
IRichEditOle_fnRelease(IRichEditOle *me)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI
IRichEditOle_fnActivateAs(IRichEditOle *me, REFCLSID rclsid, REFCLSID rclsidAs)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
IRichEditOle_fnContextSensitiveHelp(IRichEditOle *me, BOOL fEnterMode)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
IRichEditOle_fnConvertObject(IRichEditOle *me, LONG iob,
               REFCLSID rclsidNew, LPCSTR lpstrUserTypeNew)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    FIXME("stub %p\n",This);
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
    else if (IsEqualGUID(riid, &IID_IOleWindow))
        *ppvObj = &This->IOleWindow_iface;
    else if (IsEqualGUID(riid, &IID_IOleInPlaceSite))
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
    TRACE("(%p)->(%u)\n", This, ref);
    return ref;
}

static ULONG WINAPI IOleClientSite_fnRelease(IOleClientSite *iface)
{
    IOleClientSiteImpl *This = impl_from_IOleClientSite(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%u)\n", This, ref);

    if (ref == 0) {
        if (This->child.reole) {
            list_remove(&This->child.entry);
            This->child.reole = NULL;
        }
        heap_free(This);
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

/* IOleWindow interface */
static HRESULT WINAPI IOleWindow_fnQueryInterface(IOleWindow *iface, REFIID riid, void **ppvObj)
{
    IOleClientSiteImpl *This = impl_from_IOleWindow(iface);
    return IOleClientSite_QueryInterface(&This->IOleClientSite_iface, riid, ppvObj);
}

static ULONG WINAPI IOleWindow_fnAddRef(IOleWindow *iface)
{
    IOleClientSiteImpl *This = impl_from_IOleWindow(iface);
    return IOleClientSite_AddRef(&This->IOleClientSite_iface);
}

static ULONG WINAPI IOleWindow_fnRelease(IOleWindow *iface)
{
    IOleClientSiteImpl *This = impl_from_IOleWindow(iface);
    return IOleClientSite_Release(&This->IOleClientSite_iface);
}

static HRESULT WINAPI IOleWindow_fnContextSensitiveHelp(IOleWindow *iface, BOOL fEnterMode)
{
    IOleClientSiteImpl *This = impl_from_IOleWindow(iface);
    FIXME("not implemented: (%p)->(%d)\n", This, fEnterMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI IOleWindow_fnGetWindow(IOleWindow *iface, HWND *phwnd)
{
    IOleClientSiteImpl *This = impl_from_IOleWindow(iface);

    TRACE("(%p)->(%p)\n", This, phwnd);

    if (!This->child.reole)
        return CO_E_RELEASED;

    if (!phwnd)
        return E_INVALIDARG;

    *phwnd = This->child.reole->editor->hWnd;
    return S_OK;
}

static const IOleWindowVtbl olewinvt = {
    IOleWindow_fnQueryInterface,
    IOleWindow_fnAddRef,
    IOleWindow_fnRelease,
    IOleWindow_fnGetWindow,
    IOleWindow_fnContextSensitiveHelp
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

static HRESULT STDMETHODCALLTYPE IOleInPlaceSite_fnGetWindow(IOleInPlaceSite *iface, HWND *phwnd)
{
    IOleClientSiteImpl *This = impl_from_IOleInPlaceSite(iface);
    return IOleWindow_GetWindow(&This->IOleWindow_iface, phwnd);
}

static HRESULT STDMETHODCALLTYPE IOleInPlaceSite_fnContextSensitiveHelp(IOleInPlaceSite *iface, BOOL fEnterMode)
{
    IOleClientSiteImpl *This = impl_from_IOleInPlaceSite(iface);
    return IOleWindow_ContextSensitiveHelp(&This->IOleWindow_iface, fEnterMode);
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

static HRESULT CreateOleClientSite(IRichEditOleImpl *reOle, IOleClientSite **ret)
{
    IOleClientSiteImpl *clientSite = heap_alloc(sizeof *clientSite);

    if (!clientSite)
        return E_OUTOFMEMORY;

    clientSite->IOleClientSite_iface.lpVtbl = &ocst;
    clientSite->IOleWindow_iface.lpVtbl = &olewinvt;
    clientSite->IOleInPlaceSite_iface.lpVtbl = &olestvt;
    clientSite->ref = 1;
    clientSite->child.reole = reOle;
    list_add_head(&reOle->clientsites, &clientSite->child.entry);

    *ret = &clientSite->IOleClientSite_iface;
    return S_OK;
}

static HRESULT WINAPI
IRichEditOle_fnGetClientSite(IRichEditOle *me, IOleClientSite **clientsite)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);

    TRACE("(%p)->(%p)\n", This, clientsite);

    if (!clientsite)
        return E_INVALIDARG;

    return CreateOleClientSite(This, clientsite);
}

static HRESULT WINAPI
IRichEditOle_fnGetClipboardData(IRichEditOle *me, CHARRANGE *lpchrg,
               DWORD reco, LPDATAOBJECT *lplpdataobj)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    ME_Cursor start;
    int nChars;

    TRACE("(%p,%p,%d)\n",This, lpchrg, reco);
    if(!lplpdataobj)
        return E_INVALIDARG;
    if(!lpchrg) {
        int nFrom, nTo, nStartCur = ME_GetSelectionOfs(This->editor, &nFrom, &nTo);
        start = This->editor->pCursors[nStartCur];
        nChars = nTo - nFrom;
    } else {
        ME_CursorFromCharOfs(This->editor, lpchrg->cpMin, &start);
        nChars = lpchrg->cpMax - lpchrg->cpMin;
    }
    return ME_GetDataObject(This->editor, &start, nChars, lplpdataobj);
}

static LONG WINAPI IRichEditOle_fnGetLinkCount(IRichEditOle *me)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
IRichEditOle_fnGetObject(IRichEditOle *me, LONG iob,
               REOBJECT *lpreobject, DWORD dwFlags)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static LONG WINAPI
IRichEditOle_fnGetObjectCount(IRichEditOle *me)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    FIXME("stub %p\n",This);
    return 0;
}

static HRESULT WINAPI
IRichEditOle_fnHandsOffStorage(IRichEditOle *me, LONG iob)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
IRichEditOle_fnImportDataObject(IRichEditOle *me, LPDATAOBJECT lpdataobj,
               CLIPFORMAT cf, HGLOBAL hMetaPict)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
IRichEditOle_fnInPlaceDeactivate(IRichEditOle *me)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
IRichEditOle_fnInsertObject(IRichEditOle *me, REOBJECT *reo)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);

    TRACE("(%p,%p)\n", This, reo);

    if (!reo)
        return E_INVALIDARG;

    if (reo->cbStruct < sizeof(*reo)) return STG_E_INVALIDPARAMETER;

    ME_InsertOLEFromCursor(This->editor, reo, 0);
    ME_CommitUndo(This->editor);
    ME_UpdateRepaint(This->editor, FALSE);
    return S_OK;
}

static HRESULT WINAPI IRichEditOle_fnSaveCompleted(IRichEditOle *me, LONG iob,
               LPSTORAGE lpstg)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
IRichEditOle_fnSetDvaspect(IRichEditOle *me, LONG iob, DWORD dvaspect)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI IRichEditOle_fnSetHostNames(IRichEditOle *me,
               LPCSTR lpstrContainerApp, LPCSTR lpstrContainerObj)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    FIXME("stub %p %s %s\n",This, lpstrContainerApp, lpstrContainerObj);
    return E_NOTIMPL;
}

static HRESULT WINAPI
IRichEditOle_fnSetLinkAvailable(IRichEditOle *me, LONG iob, BOOL fAvailable)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static const IRichEditOleVtbl revt = {
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

    TRACE ("%p ref=%u\n", This, ref);
    if (ref == 0)
    {
        if (This->child.reole)
        {
            list_remove(&This->child.entry);
            This->child.reole = NULL;
        }
        heap_free(This);
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

    TRACE("(%p)->(%u,%d,%p)\n", This, iTInfo, lcid, ppTInfo);

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

    TRACE("(%p)->(%p,%p,%u,%d,%p)\n", This, riid, rgszNames, cNames, lcid,
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

    TRACE("(%p)->(%d,%p,%d,%u,%p,%p,%p,%p)\n", This, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

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
    ME_CursorFromCharOfs(editor, This->start, &start);
    ME_CursorFromCharOfs(editor, This->end, &end);

    length = This->end - This->start;
    *str = SysAllocStringLen(NULL, length);
    if (!*str)
        return E_OUTOFMEMORY;

    bEOP = (end.pRun->next->type == diTextEnd && This->end > ME_GetTextLength(editor));
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
    if (This->start != This->end) {
        ME_CursorFromCharOfs(editor, This->start, &cursor);
        ME_InternalDeleteText(editor, &cursor, This->end - This->start, FALSE);
    }

    if (!str || !*str) {
        /* will update this range as well */
        textranges_update_ranges(This->child.reole, This->start, This->end, RANGE_UPDATE_DELETE);
        return S_OK;
    }

    /* it's safer not to rely on stored BSTR length */
    len = strlenW(str);
    cursor = editor->pCursors[0];
    ME_CursorFromCharOfs(editor, This->start, &editor->pCursors[0]);
    style = ME_GetInsertStyle(editor, 0);
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

    ME_GetTextW(editor, wch, 1, cursor, 1, FALSE, cursor->pRun->next->type == diTextEnd);
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
    ME_CursorFromCharOfs(editor, This->start, &cursor);
    return range_GetChar(editor, &cursor, pch);
}

static HRESULT WINAPI ITextRange_fnSetChar(ITextRange *me, LONG ch)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%x): stub\n", This, ch);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT CreateITextRange(IRichEditOleImpl *reOle, LONG start, LONG end, ITextRange** ppRange);

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

static HRESULT textrange_setstart(const IRichEditOleImpl *reole, LONG value, LONG *start, LONG *end)
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

    len = ME_GetTextLength(reole->editor);
    *start = *end = value > len ? len : value;
    return S_OK;
}

static HRESULT WINAPI ITextRange_fnSetStart(ITextRange *me, LONG value)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    TRACE("(%p)->(%d)\n", This, value);

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

static HRESULT textrange_setend(const IRichEditOleImpl *reole, LONG value, LONG *start, LONG *end)
{
    int len;

    if (value == *end)
        return S_FALSE;

    if (value < *start) {
        *start = *end = max(0, value);
        return S_OK;
    }

    len = ME_GetTextLength(reole->editor);
    *end = value > len ? len + 1 : value;
    return S_OK;
}

static HRESULT WINAPI ITextRange_fnSetEnd(ITextRange *me, LONG value)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    TRACE("(%p)->(%d)\n", This, value);

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

  if (bStart == tomEnd || bStart == tomFalse)
      *start = *end;
  else
      *end = *start;
  return S_OK;
}

static HRESULT WINAPI ITextRange_fnCollapse(ITextRange *me, LONG bStart)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    TRACE("(%p)->(%d)\n", This, bStart);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return range_Collapse(bStart, &This->start, &This->end);
}

static HRESULT WINAPI ITextRange_fnExpand(ITextRange *me, LONG unit, LONG *delta)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    TRACE("(%p)->(%d %p)\n", This, unit, delta);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return textrange_expand(me, unit, delta);
}

static HRESULT WINAPI ITextRange_fnGetIndex(ITextRange *me, LONG unit, LONG *index)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%d %p): stub\n", This, unit, index);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnSetIndex(ITextRange *me, LONG unit, LONG index,
                                            LONG extend)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%d %d %d): stub\n", This, unit, index, extend);

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

    FIXME("(%p)->(%d %d): stub\n", This, anchor, active);

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

    ME_SetSelection(This->child.reole->editor, This->start, This->end);
    return S_OK;
}

static HRESULT WINAPI ITextRange_fnStartOf(ITextRange *me, LONG unit, LONG extend,
                                           LONG *delta)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%d %d %p): stub\n", This, unit, extend, delta);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnEndOf(ITextRange *me, LONG unit, LONG extend,
                                         LONG *delta)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%d %d %p): stub\n", This, unit, extend, delta);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnMove(ITextRange *me, LONG unit, LONG count, LONG *delta)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%d %d %p): stub\n", This, unit, count, delta);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnMoveStart(ITextRange *me, LONG unit, LONG count,
                                             LONG *delta)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%d %d %p): stub\n", This, unit, count, delta);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnMoveEnd(ITextRange *me, LONG unit, LONG count,
                                           LONG *delta)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%d %d %p): stub\n", This, unit, count, delta);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnMoveWhile(ITextRange *me, VARIANT *charset, LONG count,
                                             LONG *delta)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%s %d %p): stub\n", This, debugstr_variant(charset), count, delta);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnMoveStartWhile(ITextRange *me, VARIANT *charset, LONG count,
                                                  LONG *delta)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%s %d %p): stub\n", This, debugstr_variant(charset), count, delta);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnMoveEndWhile(ITextRange *me, VARIANT *charset, LONG count,
                                                LONG *delta)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%s %d %p): stub\n", This, debugstr_variant(charset), count, delta);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnMoveUntil(ITextRange *me, VARIANT *charset, LONG count,
                                             LONG *delta)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%s %d %p): stub\n", This, debugstr_variant(charset), count, delta);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnMoveStartUntil(ITextRange *me, VARIANT *charset, LONG count,
                                                  LONG *delta)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%s %d %p): stub\n", This, debugstr_variant(charset), count, delta);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnMoveEndUntil(ITextRange *me, VARIANT *charset, LONG count,
                                                LONG *delta)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%s %d %p): stub\n", This, debugstr_variant(charset), count, delta);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnFindText(ITextRange *me, BSTR text, LONG count, LONG flags,
                                            LONG *length)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%s %d %x %p): stub\n", This, debugstr_w(text), count, flags, length);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnFindTextStart(ITextRange *me, BSTR text, LONG count,
                                                 LONG flags, LONG *length)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%s %d %x %p): stub\n", This, debugstr_w(text), count, flags, length);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnFindTextEnd(ITextRange *me, BSTR text, LONG count,
                                               LONG flags, LONG *length)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%s %d %x %p): stub\n", This, debugstr_w(text), count, flags, length);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnDelete(ITextRange *me, LONG unit, LONG count, LONG *delta)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%d %d %p): stub\n", This, unit, count, delta);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnCut(ITextRange *me, VARIANT *v)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%p): stub\n", This, v);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnCopy(ITextRange *me, VARIANT *v)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%p): stub\n", This, v);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnPaste(ITextRange *me, VARIANT *v, LONG format)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%s %x): stub\n", This, debugstr_variant(v), format);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnCanPaste(ITextRange *me, VARIANT *v, LONG format, LONG *ret)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%s %x %p): stub\n", This, debugstr_variant(v), format, ret);

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

    FIXME("(%p)->(%d): stub\n", This, type);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnGetPoint(ITextRange *me, LONG type, LONG *cx, LONG *cy)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%d %p %p): stub\n", This, type, cx, cy);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnSetPoint(ITextRange *me, LONG x, LONG y, LONG type,
                                            LONG extend)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%d %d %d %d): stub\n", This, x, y, type, extend);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextRange_fnScrollIntoView(ITextRange *me, LONG value)
{
    ITextRangeImpl *This = impl_from_ITextRange(me);

    FIXME("(%p)->(%d): stub\n", This, value);

    if (!This->child.reole)
        return CO_E_RELEASED;

    return E_NOTIMPL;
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
    TRACE("(%p)->(%u)\n", This, ref);
    return ref;
}

static ULONG WINAPI TextFont_Release(ITextFont *iface)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%u)\n", This, ref);

    if (!ref)
    {
        if (This->range)
            ITextRange_Release(This->range);
        SysFreeString(This->props[FONT_NAME].str);
        heap_free(This);
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

    TRACE("(%p)->(%u,%d,%p)\n", This, iTInfo, lcid, ppTInfo);

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

    TRACE("(%p)->(%p,%p,%u,%d,%p)\n", This, riid, rgszNames, cNames, lcid,
            rgDispId);

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

    TRACE("(%p)->(%d,%p,%d,%u,%p,%p,%p,%p)\n", This, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

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

    if (This->range && !get_range_reole(This->range))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextFont_CanChange(ITextFont *iface, LONG *ret)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    FIXME("(%p)->(%p): stub\n", This, ret);

    if (This->range && !get_range_reole(This->range))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextFont_IsEqual(ITextFont *iface, ITextFont *font, LONG *ret)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    FIXME("(%p)->(%p %p): stub\n", This, font, ret);

    if (This->range && !get_range_reole(This->range))
        return CO_E_RELEASED;

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
            static const WCHAR sysW[] = {'S','y','s','t','e','m',0};
            SysFreeString(font->props[id].str);
            font->props[id].str = SysAllocString(sysW);
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

    TRACE("(%p)->(%d)\n", This, value);

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
            FIXME("reset mode %d not supported\n", value);
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

    FIXME("reset mode %d not supported\n", value);
    return E_NOTIMPL;
}

static HRESULT WINAPI TextFont_GetStyle(ITextFont *iface, LONG *value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    FIXME("(%p)->(%p): stub\n", This, value);

    if (This->range && !get_range_reole(This->range))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextFont_SetStyle(ITextFont *iface, LONG value)
{
    ITextFontImpl *This = impl_from_ITextFont(iface);
    FIXME("(%p)->(%d): stub\n", This, value);

    if (This->range && !get_range_reole(This->range))
        return CO_E_RELEASED;

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
    TRACE("(%p)->(%d)\n", This, value);
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

    TRACE("(%p)->(%d)\n", This, value);

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
    TRACE("(%p)->(%d)\n", This, value);
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
    TRACE("(%p)->(%d)\n", This, value);
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
    TRACE("(%p)->(%d)\n", This, value);
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
    TRACE("(%p)->(%d)\n", This, value);
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
    TRACE("(%p)->(%d)\n", This, value);
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
    TRACE("(%p)->(%d)\n", This, value);
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
    TRACE("(%p)->(%d)\n", This, value);
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
    TRACE("(%p)->(%d)\n", This, value);
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
    TRACE("(%p)->(%d)\n", This, value);
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
    TRACE("(%p)->(%d)\n", This, value);
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
    TRACE("(%p)->(%d)\n", This, value);
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
    TRACE("(%p)->(%d)\n", This, value);
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
    TRACE("(%p)->(%d)\n", This, value);
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
    TRACE("(%p)->(%d)\n", This, value);
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
    TRACE("(%p)->(%d)\n", This, value);
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
    TRACE("(%p)->(%d)\n", This, value);
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
    TRACE("(%p)->(%d)\n", This, value);
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
    font = heap_alloc(sizeof(*font));
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
    TRACE("(%p)->(%u)\n", This, ref);
    return ref;
}

static ULONG WINAPI TextPara_Release(ITextPara *iface)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%u)\n", This, ref);

    if (!ref)
    {
        ITextRange_Release(This->range);
        heap_free(This);
    }

    return ref;
}

static IRichEditOleImpl *para_get_reole(ITextParaImpl *This)
{
    if (This->range)
    {
        ITextRangeImpl *rng = impl_from_ITextRange(This->range);
        return rng->child.reole;
    }
    return NULL;
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

    TRACE("(%p)->(%u,%d,%p)\n", This, iTInfo, lcid, ppTInfo);

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

    TRACE("(%p)->(%p,%p,%u,%d,%p)\n", This, riid, rgszNames, cNames, lcid,
            rgDispId);

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

    TRACE("(%p)->(%d,%p,%d,%u,%p,%p,%p,%p)\n", This, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(ITextPara_tid, &ti);
    if (SUCCEEDED(hr))
        hr = ITypeInfo_Invoke(ti, iface, dispIdMember, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return hr;
}

static HRESULT WINAPI TextPara_GetDuplicate(ITextPara *iface, ITextPara **ret)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, ret);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetDuplicate(ITextPara *iface, ITextPara *para)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, para);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_CanChange(ITextPara *iface, LONG *ret)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, ret);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_IsEqual(ITextPara *iface, ITextPara *para, LONG *ret)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p %p)\n", This, para, ret);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_Reset(ITextPara *iface, LONG value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%d)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetStyle(ITextPara *iface, LONG *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetStyle(ITextPara *iface, LONG value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%d)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetAlignment(ITextPara *iface, LONG *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    static int once;

    if (!once++) FIXME("(%p)->(%p)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetAlignment(ITextPara *iface, LONG value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%d)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetHyphenation(ITextPara *iface, LONG *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetHyphenation(ITextPara *iface, LONG value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%d)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetFirstLineIndent(ITextPara *iface, FLOAT *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetKeepTogether(ITextPara *iface, LONG *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetKeepTogether(ITextPara *iface, LONG value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%d)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetKeepWithNext(ITextPara *iface, LONG *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetKeepWithNext(ITextPara *iface, LONG value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%d)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetLeftIndent(ITextPara *iface, FLOAT *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetLineSpacing(ITextPara *iface, FLOAT *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetLineSpacingRule(ITextPara *iface, LONG *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetListAlignment(ITextPara *iface, LONG *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetListAlignment(ITextPara *iface, LONG value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%d)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetListLevelIndex(ITextPara *iface, LONG *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetListLevelIndex(ITextPara *iface, LONG value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%d)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetListStart(ITextPara *iface, LONG *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetListStart(ITextPara *iface, LONG value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%d)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetListTab(ITextPara *iface, FLOAT *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetListTab(ITextPara *iface, FLOAT value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%.2f)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetListType(ITextPara *iface, LONG *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetListType(ITextPara *iface, LONG value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%d)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetNoLineNumber(ITextPara *iface, LONG *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetNoLineNumber(ITextPara *iface, LONG value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%d)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetPageBreakBefore(ITextPara *iface, LONG *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetPageBreakBefore(ITextPara *iface, LONG value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%d)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetRightIndent(ITextPara *iface, FLOAT *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetRightIndent(ITextPara *iface, FLOAT value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%.2f)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetIndents(ITextPara *iface, FLOAT StartIndent, FLOAT LeftIndent, FLOAT RightIndent)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%.2f %.2f %.2f)\n", This, StartIndent, LeftIndent, RightIndent);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetLineSpacing(ITextPara *iface, LONG LineSpacingRule, FLOAT LineSpacing)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%d %.2f)\n", This, LineSpacingRule, LineSpacing);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetSpaceAfter(ITextPara *iface, FLOAT *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetSpaceAfter(ITextPara *iface, FLOAT value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%.2f)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetSpaceBefore(ITextPara *iface, FLOAT *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetSpaceBefore(ITextPara *iface, FLOAT value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%.2f)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetWidowControl(ITextPara *iface, LONG *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_SetWidowControl(ITextPara *iface, LONG value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%d)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetTabCount(ITextPara *iface, LONG *value)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%p)\n", This, value);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_AddTab(ITextPara *iface, FLOAT tbPos, LONG tbAlign, LONG tbLeader)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%.2f %d %d)\n", This, tbPos, tbAlign, tbLeader);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_ClearAllTabs(ITextPara *iface)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)\n", This);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_DeleteTab(ITextPara *iface, FLOAT pos)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%.2f)\n", This, pos);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI TextPara_GetTab(ITextPara *iface, LONG iTab, FLOAT *ptbPos, LONG *ptbAlign, LONG *ptbLeader)
{
    ITextParaImpl *This = impl_from_ITextPara(iface);
    FIXME("(%p)->(%d %p %p %p)\n", This, iTab, ptbPos, ptbAlign, ptbLeader);

    if (!para_get_reole(This))
        return CO_E_RELEASED;

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
    para = heap_alloc(sizeof(*para));
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
static HRESULT WINAPI
ITextDocument_fnQueryInterface(ITextDocument* me, REFIID riid,
    void** ppvObject)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    return IRichEditOle_QueryInterface(&This->IRichEditOle_iface, riid, ppvObject);
}

static ULONG WINAPI
ITextDocument_fnAddRef(ITextDocument* me)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    return IRichEditOle_AddRef(&This->IRichEditOle_iface);
}

static ULONG WINAPI
ITextDocument_fnRelease(ITextDocument* me)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    return IRichEditOle_Release(&This->IRichEditOle_iface);
}

static HRESULT WINAPI
ITextDocument_fnGetTypeInfoCount(ITextDocument* me,
    UINT* pctinfo)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    TRACE("(%p)->(%p)\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI
ITextDocument_fnGetTypeInfo(ITextDocument* me, UINT iTInfo, LCID lcid,
    ITypeInfo** ppTInfo)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    HRESULT hr;

    TRACE("(%p)->(%u,%d,%p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(ITextDocument_tid, ppTInfo);
    if (SUCCEEDED(hr))
        ITypeInfo_AddRef(*ppTInfo);
    return hr;
}

static HRESULT WINAPI
ITextDocument_fnGetIDsOfNames(ITextDocument* me, REFIID riid,
    LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    ITypeInfo *ti;
    HRESULT hr;

    TRACE("(%p)->(%p,%p,%u,%d,%p)\n", This, riid, rgszNames, cNames, lcid,
            rgDispId);

    hr = get_typeinfo(ITextDocument_tid, &ti);
    if (SUCCEEDED(hr))
        hr = ITypeInfo_GetIDsOfNames(ti, rgszNames, cNames, rgDispId);
    return hr;
}

static HRESULT WINAPI
ITextDocument_fnInvoke(ITextDocument* me, DISPID dispIdMember,
    REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams,
    VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    ITypeInfo *ti;
    HRESULT hr;

    TRACE("(%p)->(%d,%p,%d,%u,%p,%p,%p,%p)\n", This, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(ITextDocument_tid, &ti);
    if (SUCCEEDED(hr))
        hr = ITypeInfo_Invoke(ti, me, dispIdMember, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return hr;
}

static HRESULT WINAPI
ITextDocument_fnGetName(ITextDocument* me, BSTR* pName)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnGetSelection(ITextDocument *me, ITextSelection **selection)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);

    TRACE("(%p)->(%p)\n", me, selection);

    if (!selection)
      return E_INVALIDARG;

    if (!This->txtSel) {
      This->txtSel = CreateTextSelection(This);
      if (!This->txtSel) {
        *selection = NULL;
        return E_OUTOFMEMORY;
      }
    }

    *selection = &This->txtSel->ITextSelection_iface;
    ITextSelection_AddRef(*selection);
    return S_OK;
}

static HRESULT WINAPI
ITextDocument_fnGetStoryCount(ITextDocument* me, LONG* pCount)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnGetStoryRanges(ITextDocument* me,
    ITextStoryRanges** ppStories)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnGetSaved(ITextDocument* me, LONG* pValue)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnSetSaved(ITextDocument* me, LONG Value)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnGetDefaultTabStop(ITextDocument* me, float* pValue)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnSetDefaultTabStop(ITextDocument* me, float Value)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnNew(ITextDocument* me)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnOpen(ITextDocument* me, VARIANT* pVar, LONG Flags,
    LONG CodePage)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnSave(ITextDocument* me, VARIANT* pVar, LONG Flags,
    LONG CodePage)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnFreeze(ITextDocument* me, LONG* pCount)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnUnfreeze(ITextDocument* me, LONG* pCount)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnBeginEditCollection(ITextDocument* me)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnEndEditCollection(ITextDocument* me)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnUndo(ITextDocument* me, LONG Count, LONG* prop)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI
ITextDocument_fnRedo(ITextDocument* me, LONG Count, LONG* prop)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static HRESULT CreateITextRange(IRichEditOleImpl *reOle, LONG start, LONG end, ITextRange** ppRange)
{
    ITextRangeImpl *txtRge = heap_alloc(sizeof(ITextRangeImpl));

    if (!txtRge)
        return E_OUTOFMEMORY;
    txtRge->ITextRange_iface.lpVtbl = &trvt;
    txtRge->ref = 1;
    txtRge->child.reole = reOle;
    txtRge->start = start;
    txtRge->end = end;
    list_add_head(&reOle->rangelist, &txtRge->child.entry);
    *ppRange = &txtRge->ITextRange_iface;
    return S_OK;
}

static HRESULT WINAPI
ITextDocument_fnRange(ITextDocument* me, LONG cp1, LONG cp2,
    ITextRange** ppRange)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);

    TRACE("%p %p %d %d\n", This, ppRange, cp1, cp2);
    if (!ppRange)
        return E_INVALIDARG;

    cp2range(This->editor, &cp1, &cp2);
    return CreateITextRange(This, cp1, cp2, ppRange);
}

static HRESULT WINAPI
ITextDocument_fnRangeFromPoint(ITextDocument* me, LONG x, LONG y,
    ITextRange** ppRange)
{
    IRichEditOleImpl *This = impl_from_ITextDocument(me);
    FIXME("stub %p\n",This);
    return E_NOTIMPL;
}

static const ITextDocumentVtbl tdvt = {
    ITextDocument_fnQueryInterface,
    ITextDocument_fnAddRef,
    ITextDocument_fnRelease,
    ITextDocument_fnGetTypeInfoCount,
    ITextDocument_fnGetTypeInfo,
    ITextDocument_fnGetIDsOfNames,
    ITextDocument_fnInvoke,
    ITextDocument_fnGetName,
    ITextDocument_fnGetSelection,
    ITextDocument_fnGetStoryCount,
    ITextDocument_fnGetStoryRanges,
    ITextDocument_fnGetSaved,
    ITextDocument_fnSetSaved,
    ITextDocument_fnGetDefaultTabStop,
    ITextDocument_fnSetDefaultTabStop,
    ITextDocument_fnNew,
    ITextDocument_fnOpen,
    ITextDocument_fnSave,
    ITextDocument_fnFreeze,
    ITextDocument_fnUnfreeze,
    ITextDocument_fnBeginEditCollection,
    ITextDocument_fnEndEditCollection,
    ITextDocument_fnUndo,
    ITextDocument_fnRedo,
    ITextDocument_fnRange,
    ITextDocument_fnRangeFromPoint
};

/* ITextSelection */
static HRESULT WINAPI ITextSelection_fnQueryInterface(
    ITextSelection *me,
    REFIID riid,
    void **ppvObj)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

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
        *ppvObj = This->reOle;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI ITextSelection_fnAddRef(ITextSelection *me)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ITextSelection_fnRelease(ITextSelection *me)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);
    ULONG ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
        heap_free(This);
    return ref;
}

static HRESULT WINAPI ITextSelection_fnGetTypeInfoCount(ITextSelection *me, UINT *pctinfo)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);
    TRACE("(%p)->(%p)\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI ITextSelection_fnGetTypeInfo(ITextSelection *me, UINT iTInfo, LCID lcid,
    ITypeInfo **ppTInfo)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);
    HRESULT hr;

    TRACE("(%p)->(%u,%d,%p)\n", This, iTInfo, lcid, ppTInfo);

    hr = get_typeinfo(ITextSelection_tid, ppTInfo);
    if (SUCCEEDED(hr))
        ITypeInfo_AddRef(*ppTInfo);
    return hr;
}

static HRESULT WINAPI ITextSelection_fnGetIDsOfNames(ITextSelection *me, REFIID riid,
    LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);
    ITypeInfo *ti;
    HRESULT hr;

    TRACE("(%p)->(%p,%p,%u,%d,%p)\n", This, riid, rgszNames, cNames, lcid,
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
    ITextSelectionImpl *This = impl_from_ITextSelection(me);
    ITypeInfo *ti;
    HRESULT hr;

    TRACE("(%p)->(%d,%p,%d,%u,%p,%p,%p,%p)\n", This, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(ITextSelection_tid, &ti);
    if (SUCCEEDED(hr))
        hr = ITypeInfo_Invoke(ti, me, dispIdMember, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
    return hr;
}

/*** ITextRange methods ***/
static HRESULT WINAPI ITextSelection_fnGetText(ITextSelection *me, BSTR *pbstr)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);
    ME_Cursor *start = NULL, *end = NULL;
    int nChars, endOfs;
    BOOL bEOP;

    TRACE("(%p)->(%p)\n", This, pbstr);

    if (!This->reOle)
        return CO_E_RELEASED;

    if (!pbstr)
        return E_INVALIDARG;

    ME_GetSelection(This->reOle->editor, &start, &end);
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

    bEOP = (end->pRun->next->type == diTextEnd && endOfs > ME_GetTextLength(This->reOle->editor));
    ME_GetTextW(This->reOle->editor, *pbstr, nChars, start, nChars, FALSE, bEOP);
    TRACE("%s\n", wine_dbgstr_w(*pbstr));

    return S_OK;
}

static HRESULT WINAPI ITextSelection_fnSetText(ITextSelection *me, BSTR str)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);
    ME_TextEditor *editor;
    int len, to, from;

    TRACE("(%p)->(%s)\n", This, debugstr_w(str));

    if (!This->reOle)
        return CO_E_RELEASED;

    editor = This->reOle->editor;
    len = strlenW(str);
    ME_GetSelectionOfs(editor, &from, &to);
    ME_ReplaceSel(editor, FALSE, str, len);

    if (len < to - from)
        textranges_update_ranges(This->reOle, from, len, RANGE_UPDATE_DELETE);

    return S_OK;
}

static HRESULT WINAPI ITextSelection_fnGetChar(ITextSelection *me, LONG *pch)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);
    ME_Cursor *start = NULL, *end = NULL;

    TRACE("(%p)->(%p)\n", This, pch);

    if (!This->reOle)
        return CO_E_RELEASED;

    if (!pch)
        return E_INVALIDARG;

    ME_GetSelection(This->reOle->editor, &start, &end);
    return range_GetChar(This->reOle->editor, start, pch);
}

static HRESULT WINAPI ITextSelection_fnSetChar(ITextSelection *me, LONG ch)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%x): stub\n", This, ch);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnGetDuplicate(ITextSelection *me, ITextRange **range)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);
    LONG start, end;

    TRACE("(%p)->(%p)\n", This, range);

    if (!This->reOle)
        return CO_E_RELEASED;

    if (!range)
        return E_INVALIDARG;

    ITextSelection_GetStart(me, &start);
    ITextSelection_GetEnd(me, &end);
    return CreateITextRange(This->reOle, start, end, range);
}

static HRESULT WINAPI ITextSelection_fnGetFormattedText(ITextSelection *me, ITextRange **range)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%p): stub\n", This, range);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnSetFormattedText(ITextSelection *me, ITextRange *range)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%p): stub\n", This, range);

    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnGetStart(ITextSelection *me, LONG *pcpFirst)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);
    LONG lim;

    TRACE("(%p)->(%p)\n", This, pcpFirst);

    if (!This->reOle)
        return CO_E_RELEASED;

    if (!pcpFirst)
        return E_INVALIDARG;
    ME_GetSelectionOfs(This->reOle->editor, pcpFirst, &lim);
    return S_OK;
}

static HRESULT WINAPI ITextSelection_fnSetStart(ITextSelection *me, LONG value)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);
    LONG start, end;
    HRESULT hr;

    TRACE("(%p)->(%d)\n", This, value);

    if (!This->reOle)
        return CO_E_RELEASED;

    ME_GetSelectionOfs(This->reOle->editor, &start, &end);
    hr = textrange_setstart(This->reOle, value, &start, &end);
    if (hr == S_OK)
        ME_SetSelection(This->reOle->editor, start, end);

    return hr;
}

static HRESULT WINAPI ITextSelection_fnGetEnd(ITextSelection *me, LONG *pcpLim)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);
    LONG first;

    TRACE("(%p)->(%p)\n", This, pcpLim);

    if (!This->reOle)
        return CO_E_RELEASED;

    if (!pcpLim)
        return E_INVALIDARG;
    ME_GetSelectionOfs(This->reOle->editor, &first, pcpLim);
    return S_OK;
}

static HRESULT WINAPI ITextSelection_fnSetEnd(ITextSelection *me, LONG value)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);
    LONG start, end;
    HRESULT hr;

    TRACE("(%p)->(%d)\n", This, value);

    if (!This->reOle)
        return CO_E_RELEASED;

    ME_GetSelectionOfs(This->reOle->editor, &start, &end);
    hr = textrange_setend(This->reOle, value, &start, &end);
    if (hr == S_OK)
        ME_SetSelection(This->reOle->editor, start, end);

    return hr;
}

static HRESULT WINAPI ITextSelection_fnGetFont(ITextSelection *me, ITextFont **font)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    TRACE("(%p)->(%p)\n", This, font);

    if (!This->reOle)
        return CO_E_RELEASED;

    if (!font)
        return E_INVALIDARG;

    return create_textfont((ITextRange*)me, NULL, font);
}

static HRESULT WINAPI ITextSelection_fnSetFont(ITextSelection *me, ITextFont *font)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    TRACE("(%p)->(%p)\n", This, font);

    if (!font)
        return E_INVALIDARG;

    if (!This->reOle)
        return CO_E_RELEASED;

    textrange_set_font((ITextRange*)me, font);
    return S_OK;
}

static HRESULT WINAPI ITextSelection_fnGetPara(ITextSelection *me, ITextPara **para)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    TRACE("(%p)->(%p)\n", This, para);

    if (!This->reOle)
        return CO_E_RELEASED;

    if (!para)
        return E_INVALIDARG;

    return create_textpara((ITextRange*)me, para);
}

static HRESULT WINAPI ITextSelection_fnSetPara(ITextSelection *me, ITextPara *para)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%p): stub\n", This, para);

    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnGetStoryLength(ITextSelection *me, LONG *length)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    TRACE("(%p)->(%p)\n", This, length);

    if (!This->reOle)
        return CO_E_RELEASED;

    return textrange_get_storylength(This->reOle->editor, length);
}

static HRESULT WINAPI ITextSelection_fnGetStoryType(ITextSelection *me, LONG *value)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    TRACE("(%p)->(%p)\n", This, value);

    if (!This->reOle)
        return CO_E_RELEASED;

    if (!value)
        return E_INVALIDARG;

    *value = tomUnknownStory;
    return S_OK;
}

static HRESULT WINAPI ITextSelection_fnCollapse(ITextSelection *me, LONG bStart)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);
    LONG start, end;
    HRESULT hres;

    TRACE("(%p)->(%d)\n", This, bStart);

    if (!This->reOle)
        return CO_E_RELEASED;

    ME_GetSelectionOfs(This->reOle->editor, &start, &end);
    hres = range_Collapse(bStart, &start, &end);
    if (SUCCEEDED(hres))
        ME_SetSelection(This->reOle->editor, start, end);
    return hres;
}

static HRESULT WINAPI ITextSelection_fnExpand(ITextSelection *me, LONG unit, LONG *delta)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    TRACE("(%p)->(%d %p)\n", This, unit, delta);

    if (!This->reOle)
        return CO_E_RELEASED;

    return textrange_expand((ITextRange*)me, unit, delta);
}

static HRESULT WINAPI ITextSelection_fnGetIndex(ITextSelection *me, LONG unit, LONG *index)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%d %p): stub\n", This, unit, index);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnSetIndex(ITextSelection *me, LONG unit, LONG index,
    LONG extend)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%d %d %d): stub\n", This, unit, index, extend);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnSetRange(ITextSelection *me, LONG anchor, LONG active)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%d %d): stub\n", This, anchor, active);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnInRange(ITextSelection *me, ITextRange *range, LONG *ret)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);
    ITextSelection *selection = NULL;
    LONG start, end;

    TRACE("(%p)->(%p %p)\n", This, range, ret);

    if (ret)
        *ret = tomFalse;

    if (!This->reOle)
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
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%p %p): stub\n", This, range, ret);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnIsEqual(ITextSelection *me, ITextRange *range, LONG *ret)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);
    ITextSelection *selection = NULL;
    LONG start, end;

    TRACE("(%p)->(%p %p)\n", This, range, ret);

    if (ret)
        *ret = tomFalse;

    if (!This->reOle)
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
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    TRACE("(%p)\n", This);

    if (!This->reOle)
        return CO_E_RELEASED;

    /* nothing to do */
    return S_OK;
}

static HRESULT WINAPI ITextSelection_fnStartOf(ITextSelection *me, LONG unit, LONG extend,
    LONG *delta)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%d %d %p): stub\n", This, unit, extend, delta);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnEndOf(ITextSelection *me, LONG unit, LONG extend,
    LONG *delta)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%d %d %p): stub\n", This, unit, extend, delta);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMove(ITextSelection *me, LONG unit, LONG count, LONG *delta)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%d %d %p): stub\n", This, unit, count, delta);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveStart(ITextSelection *me, LONG unit, LONG count,
    LONG *delta)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%d %d %p): stub\n", This, unit, count, delta);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveEnd(ITextSelection *me, LONG unit, LONG count,
    LONG *delta)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%d %d %p): stub\n", This, unit, count, delta);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveWhile(ITextSelection *me, VARIANT *charset, LONG count,
    LONG *delta)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%s %d %p): stub\n", This, debugstr_variant(charset), count, delta);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveStartWhile(ITextSelection *me, VARIANT *charset, LONG count,
    LONG *delta)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%s %d %p): stub\n", This, debugstr_variant(charset), count, delta);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveEndWhile(ITextSelection *me, VARIANT *charset, LONG count,
    LONG *delta)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%s %d %p): stub\n", This, debugstr_variant(charset), count, delta);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveUntil(ITextSelection *me, VARIANT *charset, LONG count,
    LONG *delta)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%s %d %p): stub\n", This, debugstr_variant(charset), count, delta);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveStartUntil(ITextSelection *me, VARIANT *charset, LONG count,
    LONG *delta)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%s %d %p): stub\n", This, debugstr_variant(charset), count, delta);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveEndUntil(ITextSelection *me, VARIANT *charset, LONG count,
    LONG *delta)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%s %d %p): stub\n", This, debugstr_variant(charset), count, delta);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnFindText(ITextSelection *me, BSTR text, LONG count, LONG flags,
    LONG *length)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%s %d %x %p): stub\n", This, debugstr_w(text), count, flags, length);

    if (!This->reOle)
        return CO_E_RELEASED;

    FIXME("not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnFindTextStart(ITextSelection *me, BSTR text, LONG count,
    LONG flags, LONG *length)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%s %d %x %p): stub\n", This, debugstr_w(text), count, flags, length);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnFindTextEnd(ITextSelection *me, BSTR text, LONG count,
    LONG flags, LONG *length)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%s %d %x %p): stub\n", This, debugstr_w(text), count, flags, length);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnDelete(ITextSelection *me, LONG unit, LONG count,
    LONG *delta)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%d %d %p): stub\n", This, unit, count, delta);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnCut(ITextSelection *me, VARIANT *v)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%p): stub\n", This, v);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnCopy(ITextSelection *me, VARIANT *v)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%p): stub\n", This, v);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnPaste(ITextSelection *me, VARIANT *v, LONG format)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%s %x): stub\n", This, debugstr_variant(v), format);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnCanPaste(ITextSelection *me, VARIANT *v, LONG format,
    LONG *ret)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%s %x %p): stub\n", This, debugstr_variant(v), format, ret);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnCanEdit(ITextSelection *me, LONG *ret)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%p): stub\n", This, ret);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnChangeCase(ITextSelection *me, LONG type)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%d): stub\n", This, type);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnGetPoint(ITextSelection *me, LONG type, LONG *cx, LONG *cy)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%d %p %p): stub\n", This, type, cx, cy);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnSetPoint(ITextSelection *me, LONG x, LONG y, LONG type,
    LONG extend)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%d %d %d %d): stub\n", This, x, y, type, extend);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnScrollIntoView(ITextSelection *me, LONG value)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%d): stub\n", This, value);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnGetEmbeddedObject(ITextSelection *me, IUnknown **ppv)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%p): stub\n", This, ppv);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

/*** ITextSelection methods ***/
static HRESULT WINAPI ITextSelection_fnGetFlags(ITextSelection *me, LONG *flags)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%p): stub\n", This, flags);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnSetFlags(ITextSelection *me, LONG flags)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%x): stub\n", This, flags);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnGetType(ITextSelection *me, LONG *type)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%p): stub\n", This, type);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveLeft(ITextSelection *me, LONG unit, LONG count,
    LONG extend, LONG *delta)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%d %d %d %p): stub\n", This, unit, count, extend, delta);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveRight(ITextSelection *me, LONG unit, LONG count,
    LONG extend, LONG *delta)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%d %d %d %p): stub\n", This, unit, count, extend, delta);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveUp(ITextSelection *me, LONG unit, LONG count,
    LONG extend, LONG *delta)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%d %d %d %p): stub\n", This, unit, count, extend, delta);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnMoveDown(ITextSelection *me, LONG unit, LONG count,
    LONG extend, LONG *delta)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%d %d %d %p): stub\n", This, unit, count, extend, delta);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnHomeKey(ITextSelection *me, LONG unit, LONG extend,
    LONG *delta)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%d %d %p): stub\n", This, unit, extend, delta);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnEndKey(ITextSelection *me, LONG unit, LONG extend,
    LONG *delta)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%d %d %p): stub\n", This, unit, extend, delta);

    if (!This->reOle)
        return CO_E_RELEASED;

    return E_NOTIMPL;
}

static HRESULT WINAPI ITextSelection_fnTypeText(ITextSelection *me, BSTR text)
{
    ITextSelectionImpl *This = impl_from_ITextSelection(me);

    FIXME("(%p)->(%s): stub\n", This, debugstr_w(text));

    if (!This->reOle)
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

static ITextSelectionImpl *
CreateTextSelection(IRichEditOleImpl *reOle)
{
    ITextSelectionImpl *txtSel = heap_alloc(sizeof *txtSel);
    if (!txtSel)
        return NULL;

    txtSel->ITextSelection_iface.lpVtbl = &tsvt;
    txtSel->ref = 1;
    txtSel->reOle = reOle;
    return txtSel;
}

LRESULT CreateIRichEditOle(IUnknown *outer_unk, ME_TextEditor *editor, LPVOID *ppvObj)
{
    IRichEditOleImpl *reo;

    reo = heap_alloc(sizeof(IRichEditOleImpl));
    if (!reo)
        return 0;

    reo->IUnknown_inner.lpVtbl = &reo_unk_vtbl;
    reo->IRichEditOle_iface.lpVtbl = &revt;
    reo->ITextDocument_iface.lpVtbl = &tdvt;
    reo->ref = 1;
    reo->editor = editor;
    reo->txtSel = NULL;

    TRACE("Created %p\n",reo);
    list_init(&reo->rangelist);
    list_init(&reo->clientsites);
    if (outer_unk)
        reo->outer_unk = outer_unk;
    else
        reo->outer_unk = &reo->IUnknown_inner;
    *ppvObj = &reo->IRichEditOle_iface;

    return 1;
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
  assert(run->ole_obj);

  if (run->ole_obj->sizel.cx != 0 || run->ole_obj->sizel.cy != 0)
  {
    convert_sizel(c, &run->ole_obj->sizel, pSize);
    if (c->editor->nZoomNumerator != 0)
    {
      pSize->cx = MulDiv(pSize->cx, c->editor->nZoomNumerator, c->editor->nZoomDenominator);
      pSize->cy = MulDiv(pSize->cy, c->editor->nZoomNumerator, c->editor->nZoomDenominator);
    }
    return;
  }

  if (!run->ole_obj->poleobj)
  {
    pSize->cx = pSize->cy = 0;
    return;
  }

  if (IOleObject_QueryInterface(run->ole_obj->poleobj, &IID_IDataObject, (void**)&ido) != S_OK)
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

  switch (stgm.tymed)
  {
  case TYMED_GDI:
    GetObjectW(stgm.u.hBitmap, sizeof(dibsect), &dibsect);
    pSize->cx = dibsect.dsBm.bmWidth;
    pSize->cy = dibsect.dsBm.bmHeight;
    if (!stgm.pUnkForRelease) DeleteObject(stgm.u.hBitmap);
    break;
  case TYMED_ENHMF:
    GetEnhMetaFileHeader(stgm.u.hEnhMetaFile, sizeof(emh), &emh);
    pSize->cx = emh.rclBounds.right - emh.rclBounds.left;
    pSize->cy = emh.rclBounds.bottom - emh.rclBounds.top;
    if (!stgm.pUnkForRelease) DeleteEnhMetaFile(stgm.u.hEnhMetaFile);
    break;
  default:
    FIXME("Unsupported tymed %d\n", stgm.tymed);
    break;
  }
  IDataObject_Release(ido);
  if (c->editor->nZoomNumerator != 0)
  {
    pSize->cx = MulDiv(pSize->cx, c->editor->nZoomNumerator, c->editor->nZoomDenominator);
    pSize->cy = MulDiv(pSize->cy, c->editor->nZoomNumerator, c->editor->nZoomDenominator);
  }
}

void ME_DrawOLE(ME_Context *c, int x, int y, ME_Run *run,
                ME_Paragraph *para, BOOL selected)
{
  IDataObject*  ido;
  FORMATETC     fmt;
  STGMEDIUM     stgm;
  DIBSECTION    dibsect;
  ENHMETAHEADER emh;
  HDC           hMemDC;
  SIZE          sz;
  BOOL          has_size;

  assert(run->nFlags & MERF_GRAPHICS);
  assert(run->ole_obj);
  if (IOleObject_QueryInterface(run->ole_obj->poleobj, &IID_IDataObject, (void**)&ido) != S_OK)
  {
    FIXME("Couldn't get interface\n");
    return;
  }
  has_size = run->ole_obj->sizel.cx != 0 || run->ole_obj->sizel.cy != 0;
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
  switch (stgm.tymed)
  {
  case TYMED_GDI:
    GetObjectW(stgm.u.hBitmap, sizeof(dibsect), &dibsect);
    hMemDC = CreateCompatibleDC(c->hDC);
    SelectObject(hMemDC, stgm.u.hBitmap);
    if (has_size)
    {
      convert_sizel(c, &run->ole_obj->sizel, &sz);
    } else {
      sz.cx = MulDiv(dibsect.dsBm.bmWidth, c->dpi.cx, 96);
      sz.cy = MulDiv(dibsect.dsBm.bmHeight, c->dpi.cy, 96);
    }
    if (c->editor->nZoomNumerator != 0)
    {
      sz.cx = MulDiv(sz.cx, c->editor->nZoomNumerator, c->editor->nZoomDenominator);
      sz.cy = MulDiv(sz.cy, c->editor->nZoomNumerator, c->editor->nZoomDenominator);
    }
    if (sz.cx == dibsect.dsBm.bmWidth && sz.cy == dibsect.dsBm.bmHeight)
    {
      BitBlt(c->hDC, x, y - sz.cy,
             dibsect.dsBm.bmWidth, dibsect.dsBm.bmHeight,
             hMemDC, 0, 0, SRCCOPY);
    } else {
      StretchBlt(c->hDC, x, y - sz.cy, sz.cx, sz.cy,
                 hMemDC, 0, 0, dibsect.dsBm.bmWidth,
                 dibsect.dsBm.bmHeight, SRCCOPY);
    }
    DeleteDC(hMemDC);
    if (!stgm.pUnkForRelease) DeleteObject(stgm.u.hBitmap);
    break;
  case TYMED_ENHMF:
    GetEnhMetaFileHeader(stgm.u.hEnhMetaFile, sizeof(emh), &emh);
    if (has_size)
    {
      convert_sizel(c, &run->ole_obj->sizel, &sz);
    } else {
      sz.cy = MulDiv(emh.rclBounds.bottom - emh.rclBounds.top, c->dpi.cx, 96);
      sz.cx = MulDiv(emh.rclBounds.right - emh.rclBounds.left, c->dpi.cy, 96);
    }
    if (c->editor->nZoomNumerator != 0)
    {
      sz.cx = MulDiv(sz.cx, c->editor->nZoomNumerator, c->editor->nZoomDenominator);
      sz.cy = MulDiv(sz.cy, c->editor->nZoomNumerator, c->editor->nZoomDenominator);
    }

    {
      RECT    rc;

      rc.left = x;
      rc.top = y - sz.cy;
      rc.right = x + sz.cx;
      rc.bottom = y;
      PlayEnhMetaFile(c->hDC, stgm.u.hEnhMetaFile, &rc);
    }
    if (!stgm.pUnkForRelease) DeleteEnhMetaFile(stgm.u.hEnhMetaFile);
    break;
  default:
    FIXME("Unsupported tymed %d\n", stgm.tymed);
    selected = FALSE;
    break;
  }
  if (selected && !c->editor->bHideSelection)
    PatBlt(c->hDC, x, y - sz.cy, sz.cx, sz.cy, DSTINVERT);
  IDataObject_Release(ido);
}

void ME_DeleteReObject(REOBJECT* reo)
{
    if (reo->poleobj)   IOleObject_Release(reo->poleobj);
    if (reo->pstg)      IStorage_Release(reo->pstg);
    if (reo->polesite)  IOleClientSite_Release(reo->polesite);
    FREE_OBJ(reo);
}

void ME_CopyReObject(REOBJECT* dst, const REOBJECT* src)
{
    *dst = *src;

    if (dst->poleobj)   IOleObject_AddRef(dst->poleobj);
    if (dst->pstg)      IStorage_AddRef(dst->pstg);
    if (dst->polesite)  IOleClientSite_AddRef(dst->polesite);
}

void ME_GetITextDocumentInterface(IRichEditOle *iface, LPVOID *ppvObj)
{
    IRichEditOleImpl *This = impl_from_IRichEditOle(iface);
    *ppvObj = &This->ITextDocument_iface;
}
