/*
 * Copyright 2006 Jacek Caban for CodeWeavers
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

#include "mshtml_private.h"

static const WCHAR attrBackground[] =
    {'b','a','c','k','g','r','o','u','n','d',0};
static const WCHAR attrBackgroundAttachment[] =
    {'b','a','c','k','g','r','o','u','n','d','-','a','t','t','a','c','h','m','e','n','t',0};
static const WCHAR attrBackgroundColor[] =
    {'b','a','c','k','g','r','o','u','n','d','-','c','o','l','o','r',0};
static const WCHAR attrBackgroundImage[] =
    {'b','a','c','k','g','r','o','u','n','d','-','i','m','a','g','e',0};
static const WCHAR attrBackgroundPosition[] =
    {'b','a','c','k','g','r','o','u','n','d','-','p','o','s','i','t','i','o','n',0};
static const WCHAR attrBackgroundPositionX[] =
    {'b','a','c','k','g','r','o','u','n','d','-','p','o','s','i','t','i','o','n','-','x',0};
static const WCHAR attrBackgroundPositionY[] =
    {'b','a','c','k','g','r','o','u','n','d','-','p','o','s','i','t','i','o','n','-','y',0};
static const WCHAR attrBackgroundRepeat[] =
    {'b','a','c','k','g','r','o','u','n','d','-','r','e','p','e','a','t',0};
static const WCHAR attrBorder[] =
    {'b','o','r','d','e','r',0};
static const WCHAR attrBorderBottom[] =
    {'b','o','r','d','e','r','-','b','o','t','t','o','m',0};
static const WCHAR attrBorderBottomColor[] =
    {'b','o','r','d','e','r','-','b','o','t','t','o','m','-','c','o','l','o','r',0};
static const WCHAR attrBorderBottomStyle[] =
    {'b','o','r','d','e','r','-','b','o','t','t','o','m','-','s','t','y','l','e',0};
static const WCHAR attrBorderBottomWidth[] =
    {'b','o','r','d','e','r','-','b','o','t','t','o','m','-','w','i','d','t','h',0};
static const WCHAR attrBorderColor[] =
    {'b','o','r','d','e','r','-','c','o','l','o','r',0};
static const WCHAR attrBorderLeft[] =
    {'b','o','r','d','e','r','-','l','e','f','t',0};
static const WCHAR attrBorderLeftColor[] =
    {'b','o','r','d','e','r','-','l','e','f','t','-','c','o','l','o','r',0};
static const WCHAR attrBorderLeftStyle[] =
    {'b','o','r','d','e','r','-','l','e','f','t','-','s','t','y','l','e',0};
static const WCHAR attrBorderLeftWidth[] =
    {'b','o','r','d','e','r','-','l','e','f','t','-','w','i','d','t','h',0};
static const WCHAR attrBorderRight[] =
    {'b','o','r','d','e','r','-','r','i','g','h','t',0};
static const WCHAR attrBorderRightColor[] =
    {'b','o','r','d','e','r','-','r','i','g','h','t','-','c','o','l','o','r',0};
static const WCHAR attrBorderRightStyle[] =
    {'b','o','r','d','e','r','-','r','i','g','h','t','-','s','t','y','l','e',0};
static const WCHAR attrBorderRightWidth[] =
    {'b','o','r','d','e','r','-','r','i','g','h','t','-','w','i','d','t','h',0};
static const WCHAR attrBorderTop[] =
    {'b','o','r','d','e','r','-','t','o','p',0};
static const WCHAR attrBorderTopColor[] =
    {'b','o','r','d','e','r','-','t','o','p','-','c','o','l','o','r',0};
static const WCHAR attrBorderStyle[] =
    {'b','o','r','d','e','r','-','s','t','y','l','e',0};
static const WCHAR attrBorderTopStyle[] =
    {'b','o','r','d','e','r','-','t','o','p','-','s','t','y','l','e',0};
static const WCHAR attrBorderTopWidth[] =
    {'b','o','r','d','e','r','-','t','o','p','-','w','i','d','t','h',0};
static const WCHAR attrBorderWidth[] =
    {'b','o','r','d','e','r','-','w','i','d','t','h',0};
static const WCHAR attrBottom[] =
    {'b','o','t','t','o','m',0};
/* FIXME: Use unprefixed version (requires Gecko changes). */
static const WCHAR attrBoxSizing[] =
    {'-','m','o','z','-','b','o','x','-','s','i','z','i','n','g',0};
static const WCHAR attrClear[] =
    {'c','l','e','a','r',0};
static const WCHAR attrClip[] =
    {'c','l','i','p',0};
static const WCHAR attrColor[] =
    {'c','o','l','o','r',0};
static const WCHAR attrCursor[] =
    {'c','u','r','s','o','r',0};
static const WCHAR attrDirection[] =
    {'d','i','r','e','c','t','i','o','n',0};
static const WCHAR attrDisplay[] =
    {'d','i','s','p','l','a','y',0};
static const WCHAR attrFilter[] =
    {'f','i','l','e','t','e','r',0};
static const WCHAR attrFontFamily[] =
    {'f','o','n','t','-','f','a','m','i','l','y',0};
static const WCHAR attrFontSize[] =
    {'f','o','n','t','-','s','i','z','e',0};
static const WCHAR attrFontStyle[] =
    {'f','o','n','t','-','s','t','y','l','e',0};
static const WCHAR attrFontVariant[] =
    {'f','o','n','t','-','v','a','r','i','a','n','t',0};
static const WCHAR attrFontWeight[] =
    {'f','o','n','t','-','w','e','i','g','h','t',0};
static const WCHAR attrHeight[] =
    {'h','e','i','g','h','t',0};
static const WCHAR attrLeft[] =
    {'l','e','f','t',0};
static const WCHAR attrLetterSpacing[] =
    {'l','e','t','t','e','r','-','s','p','a','c','i','n','g',0};
static const WCHAR attrLineHeight[] =
    {'l','i','n','e','-','h','e','i','g','h','t',0};
static const WCHAR attrListStyle[] =
    {'l','i','s','t','-','s','t','y','l','e',0};
static const WCHAR attrListStyleType[] =
    {'l','i','s','t','-','s','t','y','l','e','-','t','y','p','e',0};
static const WCHAR attrListStylePosition[] =
    {'l','i','s','t','-','s','t','y','l','e','-','p','o','s','i','t','i','o','n',0};
static const WCHAR attrMargin[] =
    {'m','a','r','g','i','n',0};
static const WCHAR attrMarginBottom[] =
    {'m','a','r','g','i','n','-','b','o','t','t','o','m',0};
static const WCHAR attrMarginLeft[] =
    {'m','a','r','g','i','n','-','l','e','f','t',0};
static const WCHAR attrMarginRight[] =
    {'m','a','r','g','i','n','-','r','i','g','h','t',0};
static const WCHAR attrMarginTop[] =
    {'m','a','r','g','i','n','-','t','o','p',0};
static const WCHAR attrMinHeight[] =
    {'m','i','n','-','h','e','i','g','h','t',0};
static const WCHAR attrMinWidth[] =
    {'m','i','n','-','w','i','d','t','h',0};
static const WCHAR attrOutline[] =
    {'o','u','t','l','i','n','e',0};
static const WCHAR attrOverflow[] =
    {'o','v','e','r','f','l','o','w',0};
static const WCHAR attrOverflowX[] =
    {'o','v','e','r','f','l','o','w','-','x',0};
static const WCHAR attrOverflowY[] =
    {'o','v','e','r','f','l','o','w','-','y',0};
static const WCHAR attrPadding[] =
    {'p','a','d','d','i','n','g',0};
static const WCHAR attrPaddingBottom[] =
    {'p','a','d','d','i','n','g','-','b','o','t','t','o','m',0};
static const WCHAR attrPaddingLeft[] =
    {'p','a','d','d','i','n','g','-','l','e','f','t',0};
static const WCHAR attrPaddingRight[] =
    {'p','a','d','d','i','n','g','-','r','i','g','h','t',0};
static const WCHAR attrPaddingTop[] =
    {'p','a','d','d','i','n','g','-','t','o','p',0};
static const WCHAR attrPageBreakAfter[] =
    {'p','a','g','e','-','b','r','e','a','k','-','a','f','t','e','r',0};
static const WCHAR attrPageBreakBefore[] =
    {'p','a','g','e','-','b','r','e','a','k','-','b','e','f','o','r','e',0};
static const WCHAR attrPosition[] =
    {'p','o','s','i','t','i','o','n',0};
static const WCHAR attrRight[] =
    {'r','i','g','h','t',0};
static const WCHAR attrTableLayout[] =
    {'t','a','b','l','e','-','l','a','y','o','u','t',0};
static const WCHAR attrTextAlign[] =
    {'t','e','x','t','-','a','l','i','g','n',0};
static const WCHAR attrTextDecoration[] =
    {'t','e','x','t','-','d','e','c','o','r','a','t','i','o','n',0};
static const WCHAR attrTextIndent[] =
    {'t','e','x','t','-','i','n','d','e','n','t',0};
static const WCHAR attrTextTransform[] =
    {'t','e','x','t','-','t','r','a','n','s','f','o','r','m',0};
static const WCHAR attrTop[] =
    {'t','o','p',0};
static const WCHAR attrVerticalAlign[] =
    {'v','e','r','t','i','c','a','l','-','a','l','i','g','n',0};
static const WCHAR attrVisibility[] =
    {'v','i','s','i','b','i','l','i','t','y',0};
static const WCHAR attrWhiteSpace[] =
    {'w','h','i','t','e','-','s','p','a','c','e',0};
static const WCHAR attrWidth[] =
    {'w','i','d','t','h',0};
static const WCHAR attrWordSpacing[] =
    {'w','o','r','d','-','s','p','a','c','i','n','g',0};
static const WCHAR attrWordWrap[] =
    {'w','o','r','d','-','w','r','a','p',0};
static const WCHAR attrZIndex[] =
    {'z','-','i','n','d','e','x',0};


static const WCHAR pxW[] = {'p','x',0};

typedef struct {
    const WCHAR *name;
    DISPID dispid;
} style_tbl_entry_t;

static const style_tbl_entry_t style_tbl[] = {
    {attrBackground,           DISPID_IHTMLSTYLE_BACKGROUND},
    {attrBackgroundAttachment, DISPID_IHTMLSTYLE_BACKGROUNDATTACHMENT},
    {attrBackgroundColor,      DISPID_IHTMLSTYLE_BACKGROUNDCOLOR},
    {attrBackgroundImage,      DISPID_IHTMLSTYLE_BACKGROUNDIMAGE},
    {attrBackgroundPosition,   DISPID_IHTMLSTYLE_BACKGROUNDPOSITION},
    {attrBackgroundPositionX,  DISPID_IHTMLSTYLE_BACKGROUNDPOSITIONX},
    {attrBackgroundPositionY,  DISPID_IHTMLSTYLE_BACKGROUNDPOSITIONY},
    {attrBackgroundRepeat,     DISPID_IHTMLSTYLE_BACKGROUNDREPEAT},
    {attrBorder,               DISPID_IHTMLSTYLE_BORDER},
    {attrBorderBottom,         DISPID_IHTMLSTYLE_BORDERBOTTOM},
    {attrBorderBottomColor,    DISPID_IHTMLSTYLE_BORDERBOTTOMCOLOR},
    {attrBorderBottomStyle,    DISPID_IHTMLSTYLE_BORDERBOTTOMSTYLE},
    {attrBorderBottomWidth,    DISPID_IHTMLSTYLE_BORDERBOTTOMWIDTH},
    {attrBorderColor,          DISPID_IHTMLSTYLE_BORDERCOLOR},
    {attrBorderLeft,           DISPID_IHTMLSTYLE_BORDERLEFT},
    {attrBorderLeftColor,      DISPID_IHTMLSTYLE_BORDERLEFTCOLOR},
    {attrBorderLeftStyle,      DISPID_IHTMLSTYLE_BORDERLEFTSTYLE},
    {attrBorderLeftWidth,      DISPID_IHTMLSTYLE_BORDERLEFTWIDTH},
    {attrBorderRight,          DISPID_IHTMLSTYLE_BORDERRIGHT},
    {attrBorderRightColor,     DISPID_IHTMLSTYLE_BORDERRIGHTCOLOR},
    {attrBorderRightStyle,     DISPID_IHTMLSTYLE_BORDERRIGHTSTYLE},
    {attrBorderRightWidth,     DISPID_IHTMLSTYLE_BORDERRIGHTWIDTH},
    {attrBorderStyle,          DISPID_IHTMLSTYLE_BORDERSTYLE},
    {attrBorderTop,            DISPID_IHTMLSTYLE_BORDERTOP},
    {attrBorderTopColor,       DISPID_IHTMLSTYLE_BORDERTOPCOLOR},
    {attrBorderTopStyle,       DISPID_IHTMLSTYLE_BORDERTOPSTYLE},
    {attrBorderTopWidth,       DISPID_IHTMLSTYLE_BORDERTOPWIDTH},
    {attrBorderWidth,          DISPID_IHTMLSTYLE_BORDERWIDTH},
    {attrBottom,               DISPID_IHTMLSTYLE2_BOTTOM},
    {attrBoxSizing,            DISPID_IHTMLSTYLE6_BOXSIZING},
    {attrClear,                DISPID_IHTMLSTYLE_CLEAR},
    {attrClip,                 DISPID_IHTMLSTYLE_CLIP},
    {attrColor,                DISPID_IHTMLSTYLE_COLOR},
    {attrCursor,               DISPID_IHTMLSTYLE_CURSOR},
    {attrDirection,            DISPID_IHTMLSTYLE2_DIRECTION},
    {attrDisplay,              DISPID_IHTMLSTYLE_DISPLAY},
    {attrFilter,               DISPID_IHTMLSTYLE_FILTER},
    {attrFontFamily,           DISPID_IHTMLSTYLE_FONTFAMILY},
    {attrFontSize,             DISPID_IHTMLSTYLE_FONTSIZE},
    {attrFontStyle,            DISPID_IHTMLSTYLE_FONTSTYLE},
    {attrFontVariant,          DISPID_IHTMLSTYLE_FONTVARIANT},
    {attrFontWeight,           DISPID_IHTMLSTYLE_FONTWEIGHT},
    {attrHeight,               DISPID_IHTMLSTYLE_HEIGHT},
    {attrLeft,                 DISPID_IHTMLSTYLE_LEFT},
    {attrLetterSpacing,        DISPID_IHTMLSTYLE_LETTERSPACING},
    {attrLineHeight,           DISPID_IHTMLSTYLE_LINEHEIGHT},
    {attrListStyle,            DISPID_IHTMLSTYLE_LISTSTYLE},
    {attrListStylePosition,    DISPID_IHTMLSTYLE_LISTSTYLEPOSITION},
    {attrListStyleType,        DISPID_IHTMLSTYLE_LISTSTYLETYPE},
    {attrMargin,               DISPID_IHTMLSTYLE_MARGIN},
    {attrMarginBottom,         DISPID_IHTMLSTYLE_MARGINBOTTOM},
    {attrMarginLeft,           DISPID_IHTMLSTYLE_MARGINLEFT},
    {attrMarginRight,          DISPID_IHTMLSTYLE_MARGINRIGHT},
    {attrMarginTop,            DISPID_IHTMLSTYLE_MARGINTOP},
    {attrMinHeight,            DISPID_IHTMLSTYLE4_MINHEIGHT},
    {attrMinWidth,             DISPID_IHTMLSTYLE5_MINWIDTH},
    {attrOutline,              DISPID_IHTMLSTYLE6_OUTLINE},
    {attrOverflow,             DISPID_IHTMLSTYLE_OVERFLOW},
    {attrOverflowX,            DISPID_IHTMLSTYLE2_OVERFLOWX},
    {attrOverflowY,            DISPID_IHTMLSTYLE2_OVERFLOWY},
    {attrPadding,              DISPID_IHTMLSTYLE_PADDING},
    {attrPaddingBottom,        DISPID_IHTMLSTYLE_PADDINGBOTTOM},
    {attrPaddingLeft,          DISPID_IHTMLSTYLE_PADDINGLEFT},
    {attrPaddingRight,         DISPID_IHTMLSTYLE_PADDINGRIGHT},
    {attrPaddingTop,           DISPID_IHTMLSTYLE_PADDINGTOP},
    {attrPageBreakAfter,       DISPID_IHTMLSTYLE_PAGEBREAKAFTER},
    {attrPageBreakBefore,      DISPID_IHTMLSTYLE_PAGEBREAKBEFORE},
    {attrPosition,             DISPID_IHTMLSTYLE2_POSITION},
    {attrRight,                DISPID_IHTMLSTYLE2_RIGHT},
    {attrTableLayout,          DISPID_IHTMLSTYLE2_TABLELAYOUT},
    {attrTextAlign,            DISPID_IHTMLSTYLE_TEXTALIGN},
    {attrTextDecoration,       DISPID_IHTMLSTYLE_TEXTDECORATION},
    {attrTextIndent,           DISPID_IHTMLSTYLE_TEXTINDENT},
    {attrTextTransform,        DISPID_IHTMLSTYLE_TEXTTRANSFORM},
    {attrTop,                  DISPID_IHTMLSTYLE_TOP},
    {attrVerticalAlign,        DISPID_IHTMLSTYLE_VERTICALALIGN},
    {attrVisibility,           DISPID_IHTMLSTYLE_VISIBILITY},
    {attrWhiteSpace,           DISPID_IHTMLSTYLE_WHITESPACE},
    {attrWidth,                DISPID_IHTMLSTYLE_WIDTH},
    {attrWordSpacing,          DISPID_IHTMLSTYLE_WORDSPACING},
    {attrWordWrap,             DISPID_IHTMLSTYLE3_WORDWRAP},
    {attrZIndex,               DISPID_IHTMLSTYLE_ZINDEX}
};

static const WCHAR valLineThrough[] =
    {'l','i','n','e','-','t','h','r','o','u','g','h',0};
static const WCHAR valUnderline[] =
    {'u','n','d','e','r','l','i','n','e',0};
static const WCHAR szNormal[] =
    {'n','o','r','m','a','l',0};
static const WCHAR styleNone[] =
    {'n','o','n','e',0};
static const WCHAR valOverline[] =
    {'o','v','e','r','l','i','n','e',0};
static const WCHAR valBlink[] =
    {'b','l','i','n','k',0};

static const WCHAR px_formatW[] = {'%','d','p','x',0};
static const WCHAR emptyW[] = {0};

static const style_tbl_entry_t *lookup_style_tbl(const WCHAR *name)
{
    int c, i, min = 0, max = sizeof(style_tbl)/sizeof(*style_tbl)-1;

    while(min <= max) {
        i = (min+max)/2;

        c = strcmpW(style_tbl[i].name, name);
        if(!c)
            return style_tbl+i;

        if(c > 0)
            max = i-1;
        else
            min = i+1;
    }

    return NULL;
}

static LPWSTR fix_px_value(LPCWSTR val)
{
    LPCWSTR ptr = val;

    while(*ptr) {
        while(*ptr && isspaceW(*ptr))
            ptr++;
        if(!*ptr)
            break;

        while(*ptr && isdigitW(*ptr))
            ptr++;

        if(!*ptr || isspaceW(*ptr)) {
            LPWSTR ret, p;
            int len = strlenW(val)+1;

            ret = heap_alloc((len+2)*sizeof(WCHAR));
            memcpy(ret, val, (ptr-val)*sizeof(WCHAR));
            p = ret + (ptr-val);
            *p++ = 'p';
            *p++ = 'x';
            strcpyW(p, ptr);

            TRACE("fixed %s -> %s\n", debugstr_w(val), debugstr_w(ret));

            return ret;
        }

        while(*ptr && !isspaceW(*ptr))
            ptr++;
    }

    return NULL;
}

static LPWSTR fix_url_value(LPCWSTR val)
{
    WCHAR *ret, *ptr;

    static const WCHAR urlW[] = {'u','r','l','('};

    if(strncmpW(val, urlW, sizeof(urlW)/sizeof(WCHAR)) || !strchrW(val, '\\'))
        return NULL;

    ret = heap_strdupW(val);

    for(ptr = ret; *ptr; ptr++) {
        if(*ptr == '\\')
            *ptr = '/';
    }

    return ret;
}

HRESULT set_nsstyle_attr(nsIDOMCSSStyleDeclaration *nsstyle, styleid_t sid, const WCHAR *value, DWORD flags)
{
    nsAString str_name, str_value, str_empty;
    LPWSTR val = NULL;
    nsresult nsres;

    if(value) {
        if(flags & ATTR_FIX_PX)
            val = fix_px_value(value);
        if(flags & ATTR_FIX_URL)
            val = fix_url_value(value);
    }

    nsAString_InitDepend(&str_name, style_tbl[sid].name);
    nsAString_InitDepend(&str_value, val ? val : value);
    nsAString_InitDepend(&str_empty, emptyW);

    nsres = nsIDOMCSSStyleDeclaration_SetProperty(nsstyle, &str_name, &str_value, &str_empty);
    if(NS_FAILED(nsres))
        ERR("SetProperty failed: %08x\n", nsres);

    nsAString_Finish(&str_name);
    nsAString_Finish(&str_value);
    nsAString_Finish(&str_empty);
    heap_free(val);

    return S_OK;
}

static HRESULT var_to_styleval(const VARIANT *v, WCHAR *buf, DWORD flags, const WCHAR **ret)
{
    switch(V_VT(v)) {
    case VT_NULL:
        *ret = emptyW;
        return S_OK;

    case VT_BSTR:
        *ret = V_BSTR(v);
        return S_OK;

    case VT_BSTR|VT_BYREF:
        *ret = *V_BSTRREF(v);
        return S_OK;

    case VT_I4: {
        static const WCHAR formatW[] = {'%','d',0};
        static const WCHAR hex_formatW[] = {'#','%','0','6','x',0};

        if(flags & ATTR_HEX_INT)
            wsprintfW(buf, hex_formatW, V_I4(v));
        else if(flags & ATTR_FIX_PX)
            wsprintfW(buf, px_formatW, V_I4(v));
        else
            wsprintfW(buf, formatW, V_I4(v));

        *ret = buf;
        return S_OK;
    }
    default:
        FIXME("not implemented for %s\n", debugstr_variant(v));
        return E_NOTIMPL;

    }
}

HRESULT set_nsstyle_attr_var(nsIDOMCSSStyleDeclaration *nsstyle, styleid_t sid, VARIANT *value, DWORD flags)
{
    const WCHAR *val;
    WCHAR buf[14];
    HRESULT hres;

    hres = var_to_styleval(value, buf, flags, &val);
    if(FAILED(hres))
        return hres;

    return set_nsstyle_attr(nsstyle, sid, val, flags);
}

static inline HRESULT set_style_attr(HTMLStyle *This, styleid_t sid, LPCWSTR value, DWORD flags)
{
    return set_nsstyle_attr(This->nsstyle, sid, value, flags);
}

static HRESULT get_nsstyle_attr_nsval(nsIDOMCSSStyleDeclaration *nsstyle, styleid_t sid, nsAString *value)
{
    nsAString str_name;
    nsresult nsres;

    nsAString_InitDepend(&str_name, style_tbl[sid].name);

    nsres = nsIDOMCSSStyleDeclaration_GetPropertyValue(nsstyle, &str_name, value);
    if(NS_FAILED(nsres)) {
        ERR("SetProperty failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsAString_Finish(&str_name);

    return NS_OK;
}

static HRESULT nsstyle_to_bstr(const WCHAR *val, DWORD flags, BSTR *p)
{
    BSTR ret;
    DWORD len;

    if(!*val) {
        *p = (flags & ATTR_NO_NULL) ? SysAllocStringLen(NULL, 0) : NULL;
        return S_OK;
    }

    ret = SysAllocString(val);
    if(!ret)
        return E_OUTOFMEMORY;

    len = SysStringLen(ret);

    if(flags & ATTR_REMOVE_COMMA) {
        DWORD new_len = len;
        WCHAR *ptr, *ptr2;

        for(ptr = ret; (ptr = strchrW(ptr, ',')); ptr++)
            new_len--;

        if(new_len != len) {
            BSTR new_ret;

            new_ret = SysAllocStringLen(NULL, new_len);
            if(!new_ret) {
                SysFreeString(ret);
                return E_OUTOFMEMORY;
            }

            for(ptr2 = new_ret, ptr = ret; *ptr; ptr++) {
                if(*ptr != ',')
                    *ptr2++ = *ptr;
            }

            SysFreeString(ret);
            ret = new_ret;
        }
    }

    *p = ret;
    return S_OK;
}

HRESULT get_nsstyle_attr(nsIDOMCSSStyleDeclaration *nsstyle, styleid_t sid, BSTR *p, DWORD flags)
{
    nsAString str_value;
    const PRUnichar *value;
    HRESULT hres;

    nsAString_Init(&str_value, NULL);

    get_nsstyle_attr_nsval(nsstyle, sid, &str_value);

    nsAString_GetData(&str_value, &value);
    hres = nsstyle_to_bstr(value, flags, p);
    nsAString_Finish(&str_value);

    TRACE("%s -> %s\n", debugstr_w(style_tbl[sid].name), debugstr_w(*p));
    return hres;
}

HRESULT get_nsstyle_attr_var(nsIDOMCSSStyleDeclaration *nsstyle, styleid_t sid, VARIANT *p, DWORD flags)
{
    nsAString str_value;
    const PRUnichar *value;
    BOOL set = FALSE;
    HRESULT hres = S_OK;

    nsAString_Init(&str_value, NULL);

    get_nsstyle_attr_nsval(nsstyle, sid, &str_value);

    nsAString_GetData(&str_value, &value);

    if(flags & ATTR_STR_TO_INT) {
        const PRUnichar *ptr = value;
        BOOL neg = FALSE;
        INT i = 0;

        if(*ptr == '-') {
            neg = TRUE;
            ptr++;
        }

        while(isdigitW(*ptr))
            i = i*10 + (*ptr++ - '0');

        if(!*ptr) {
            V_VT(p) = VT_I4;
            V_I4(p) = neg ? -i : i;
            set = TRUE;
        }
    }

    if(!set) {
        BSTR str;

        hres = nsstyle_to_bstr(value, flags, &str);
        if(SUCCEEDED(hres)) {
            V_VT(p) = VT_BSTR;
            V_BSTR(p) = str;
        }
    }

    nsAString_Finish(&str_value);

    TRACE("%s -> %s\n", debugstr_w(style_tbl[sid].name), debugstr_variant(p));
    return S_OK;
}

static inline HRESULT get_style_attr(HTMLStyle *This, styleid_t sid, BSTR *p)
{
    return get_nsstyle_attr(This->nsstyle, sid, p, 0);
}

static HRESULT check_style_attr_value(HTMLStyle *This, styleid_t sid, LPCWSTR exval, VARIANT_BOOL *p)
{
    nsAString str_value;
    const PRUnichar *value;

    nsAString_Init(&str_value, NULL);

    get_nsstyle_attr_nsval(This->nsstyle, sid, &str_value);

    nsAString_GetData(&str_value, &value);
    *p = strcmpW(value, exval) ? VARIANT_FALSE : VARIANT_TRUE;
    nsAString_Finish(&str_value);

    TRACE("%s -> %x\n", debugstr_w(style_tbl[sid].name), *p);
    return S_OK;
}

static inline HRESULT set_style_pos(HTMLStyle *This, styleid_t sid, float value)
{
    WCHAR szValue[25];
    WCHAR szFormat[] = {'%','.','0','f','p','x',0};

    value = floor(value);

    sprintfW(szValue, szFormat, value);

    return set_style_attr(This, sid, szValue, 0);
}

static HRESULT set_style_pxattr(nsIDOMCSSStyleDeclaration *nsstyle, styleid_t sid, LONG value)
{
    WCHAR value_str[16];

    sprintfW(value_str, px_formatW, value);

    return set_nsstyle_attr(nsstyle, sid, value_str, 0);
}

static HRESULT get_nsstyle_pos(HTMLStyle *This, styleid_t sid, float *p)
{
    nsAString str_value;
    HRESULT hres;

    TRACE("%p %d %p\n", This, sid, p);

    *p = 0.0f;

    nsAString_Init(&str_value, NULL);

    hres = get_nsstyle_attr_nsval(This->nsstyle, sid, &str_value);
    if(hres == S_OK)
    {
        WCHAR *ptr;
        const PRUnichar *value;

        nsAString_GetData(&str_value, &value);
        if(value)
        {
            *p = strtolW(value, &ptr, 10);

            if(*ptr && strcmpW(ptr, pxW))
            {
                nsAString_Finish(&str_value);
                FIXME("only px values are currently supported\n");
                hres = E_FAIL;
            }
        }
    }

    TRACE("ret %f\n", *p);

    nsAString_Finish(&str_value);
    return hres;
}

static HRESULT get_nsstyle_pixel_val(HTMLStyle *This, styleid_t sid, LONG *p)
{
    nsAString str_value;
    HRESULT hres;

    if(!p)
        return E_POINTER;

    nsAString_Init(&str_value, NULL);

    hres = get_nsstyle_attr_nsval(This->nsstyle, sid, &str_value);
    if(hres == S_OK) {
        WCHAR *ptr;
        const PRUnichar *value;

        nsAString_GetData(&str_value, &value);
        if(value) {
            *p = strtolW(value, &ptr, 10);

            if(*ptr == '.') {
                /* Skip all digits. We have tests showing that we should not round the value. */
                while(isdigitW(*++ptr));
            }

            if(*ptr && strcmpW(ptr, pxW)) {
                nsAString_Finish(&str_value);
                FIXME("%s: only px values are currently supported\n", debugstr_w(value));
                hres = E_NOTIMPL;
            }
        }else {
            *p = 0;
        }
    }

    nsAString_Finish(&str_value);
    return hres;
}

static BOOL is_valid_border_style(BSTR v)
{
    static const WCHAR styleDotted[] = {'d','o','t','t','e','d',0};
    static const WCHAR styleDashed[] = {'d','a','s','h','e','d',0};
    static const WCHAR styleSolid[]  = {'s','o','l','i','d',0};
    static const WCHAR styleDouble[] = {'d','o','u','b','l','e',0};
    static const WCHAR styleGroove[] = {'g','r','o','o','v','e',0};
    static const WCHAR styleRidge[]  = {'r','i','d','g','e',0};
    static const WCHAR styleInset[]  = {'i','n','s','e','t',0};
    static const WCHAR styleOutset[] = {'o','u','t','s','e','t',0};

    TRACE("%s\n", debugstr_w(v));

    if(!v || strcmpiW(v, styleNone)   == 0 || strcmpiW(v, styleDotted) == 0 ||
             strcmpiW(v, styleDashed) == 0 || strcmpiW(v, styleSolid)  == 0 ||
             strcmpiW(v, styleDouble) == 0 || strcmpiW(v, styleGroove) == 0 ||
             strcmpiW(v, styleRidge)  == 0 || strcmpiW(v, styleInset)  == 0 ||
             strcmpiW(v, styleOutset) == 0 )
    {
        return TRUE;
    }

    return FALSE;
}

static inline HTMLStyle *impl_from_IHTMLStyle(IHTMLStyle *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyle, IHTMLStyle_iface);
}

static HRESULT WINAPI HTMLStyle_QueryInterface(IHTMLStyle *iface, REFIID riid, void **ppv)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHTMLStyle_iface;
    }else if(IsEqualGUID(&IID_IHTMLStyle, riid)) {
        *ppv = &This->IHTMLStyle_iface;
    }else if(IsEqualGUID(&IID_IHTMLStyle2, riid)) {
        *ppv = &This->IHTMLStyle2_iface;
    }else if(IsEqualGUID(&IID_IHTMLStyle3, riid)) {
        *ppv = &This->IHTMLStyle3_iface;
    }else if(IsEqualGUID(&IID_IHTMLStyle4, riid)) {
        *ppv = &This->IHTMLStyle4_iface;
    }else if(IsEqualGUID(&IID_IHTMLStyle5, riid)) {
        *ppv = &This->IHTMLStyle5_iface;
    }else if(IsEqualGUID(&IID_IHTMLStyle6, riid)) {
        *ppv = &This->IHTMLStyle6_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        *ppv = NULL;
        WARN("unsupported iface %s\n", debugstr_mshtml_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLStyle_AddRef(IHTMLStyle *iface)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLStyle_Release(IHTMLStyle *iface)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        assert(!This->elem);
        if(This->nsstyle)
            nsIDOMCSSStyleDeclaration_Release(This->nsstyle);
        release_dispex(&This->dispex);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLStyle_GetTypeInfoCount(IHTMLStyle *iface, UINT *pctinfo)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLStyle_GetTypeInfo(IHTMLStyle *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLStyle_GetIDsOfNames(IHTMLStyle *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLStyle_Invoke(IHTMLStyle *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLStyle_put_fontFamily(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_attr(This, STYLEID_FONT_FAMILY, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_fontFamily(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_FONT_FAMILY, p);
}

static HRESULT WINAPI HTMLStyle_put_fontStyle(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    static const WCHAR szItalic[]  = {'i','t','a','l','i','c',0};
    static const WCHAR szOblique[]  = {'o','b','l','i','q','u','e',0};

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    /* fontStyle can only be one of the follow values. */
    if(!v || strcmpiW(szNormal, v) == 0 || strcmpiW(szItalic, v) == 0 ||
             strcmpiW(szOblique, v) == 0)
    {
        return set_nsstyle_attr(This->nsstyle, STYLEID_FONT_STYLE, v, 0);
    }

    return E_INVALIDARG;
}

static HRESULT WINAPI HTMLStyle_get_fontStyle(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_FONT_STYLE, p);
}

static HRESULT WINAPI HTMLStyle_put_fontVariant(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    static const WCHAR szCaps[]  = {'s','m','a','l','l','-','c','a','p','s',0};

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    /* fontVariant can only be one of the follow values. */
    if(!v || strcmpiW(szNormal, v) == 0 || strcmpiW(szCaps, v) == 0)
    {
        return set_nsstyle_attr(This->nsstyle, STYLEID_FONT_VARIANT, v, 0);
    }

    return E_INVALIDARG;
}

static HRESULT WINAPI HTMLStyle_get_fontVariant(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
       return E_INVALIDARG;

    return get_style_attr(This, STYLEID_FONT_VARIANT, p);
}

static HRESULT WINAPI HTMLStyle_put_fontWeight(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    static const WCHAR styleBold[] = {'b','o','l','d',0};
    static const WCHAR styleBolder[] = {'b','o','l','d','e','r',0};
    static const WCHAR styleLighter[]  = {'l','i','g','h','t','e','r',0};
    static const WCHAR style100[] = {'1','0','0',0};
    static const WCHAR style200[] = {'2','0','0',0};
    static const WCHAR style300[] = {'3','0','0',0};
    static const WCHAR style400[] = {'4','0','0',0};
    static const WCHAR style500[] = {'5','0','0',0};
    static const WCHAR style600[] = {'6','0','0',0};
    static const WCHAR style700[] = {'7','0','0',0};
    static const WCHAR style800[] = {'8','0','0',0};
    static const WCHAR style900[] = {'9','0','0',0};

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    /* fontWeight can only be one of the following */
    if(!v || strcmpiW(szNormal, v) == 0    || strcmpiW(styleBold, v) == 0    ||
             strcmpiW(styleBolder, v) == 0 || strcmpiW(styleLighter, v) == 0 ||
             strcmpiW(style100, v) == 0    || strcmpiW(style200, v) == 0     ||
             strcmpiW(style300, v) == 0    || strcmpiW(style400, v) == 0     ||
             strcmpiW(style500, v) == 0    || strcmpiW(style600, v) == 0     ||
             strcmpiW(style700, v) == 0    || strcmpiW(style800, v) == 0     ||
             strcmpiW(style900, v) == 0
             )
    {
        return set_nsstyle_attr(This->nsstyle, STYLEID_FONT_WEIGHT, v, 0);
    }

    return E_INVALIDARG;
}

static HRESULT WINAPI HTMLStyle_get_fontWeight(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_FONT_WEIGHT, p);
}

static HRESULT WINAPI HTMLStyle_put_fontSize(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_FONT_SIZE, &v, ATTR_FIX_PX);
}

static HRESULT WINAPI HTMLStyle_get_fontSize(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr_var(This->nsstyle, STYLEID_FONT_SIZE, p, 0);
}

static HRESULT WINAPI HTMLStyle_put_font(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_font(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_color(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_COLOR, &v, ATTR_HEX_INT);
}

static HRESULT WINAPI HTMLStyle_get_color(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr_var(This->nsstyle, STYLEID_COLOR, p, 0);
}

static HRESULT WINAPI HTMLStyle_put_background(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_attr(This, STYLEID_BACKGROUND, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_background(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_BACKGROUND, p);
}

static HRESULT WINAPI HTMLStyle_put_backgroundColor(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_BACKGROUND_COLOR, &v, ATTR_HEX_INT);
}

static HRESULT WINAPI HTMLStyle_get_backgroundColor(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr_var(This->nsstyle, STYLEID_BACKGROUND_COLOR, p, 0);
}

static HRESULT WINAPI HTMLStyle_put_backgroundImage(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_attr(This, STYLEID_BACKGROUND_IMAGE, v, ATTR_FIX_URL);
}

static HRESULT WINAPI HTMLStyle_get_backgroundImage(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_BACKGROUND_IMAGE, p);
}

static HRESULT WINAPI HTMLStyle_put_backgroundRepeat(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    static const WCHAR styleRepeat[]   = {'r','e','p','e','a','t',0};
    static const WCHAR styleNoRepeat[] = {'n','o','-','r','e','p','e','a','t',0};
    static const WCHAR styleRepeatX[]  = {'r','e','p','e','a','t','-','x',0};
    static const WCHAR styleRepeatY[]  = {'r','e','p','e','a','t','-','y',0};

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    /* fontWeight can only be one of the following */
    if(!v || strcmpiW(styleRepeat, v) == 0    || strcmpiW(styleNoRepeat, v) == 0    ||
             strcmpiW(styleRepeatX, v) == 0 || strcmpiW(styleRepeatY, v) == 0 )
    {
        return set_style_attr(This, STYLEID_BACKGROUND_REPEAT , v, 0);
    }

    return E_INVALIDARG;
}

static HRESULT WINAPI HTMLStyle_get_backgroundRepeat(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_BACKGROUND_REPEAT, p);
}

static HRESULT WINAPI HTMLStyle_put_backgroundAttachment(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_attr(This, STYLEID_BACKGROUND_ATTACHMENT, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_backgroundAttachment(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_BACKGROUND_ATTACHMENT, p);
}

static HRESULT WINAPI HTMLStyle_put_backgroundPosition(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_attr(This, STYLEID_BACKGROUND_POSITION, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_backgroundPosition(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_BACKGROUND_POSITION, p);
}

static HRESULT WINAPI HTMLStyle_put_backgroundPositionX(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    WCHAR buf[14], *pos_val;
    nsAString pos_str;
    const WCHAR *val;
    DWORD val_len;
    HRESULT hres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    hres = var_to_styleval(&v, buf, ATTR_FIX_PX, &val);
    if(FAILED(hres))
        return hres;

    val_len = val ? strlenW(val) : 0;

    nsAString_Init(&pos_str, NULL);
    hres = get_nsstyle_attr_nsval(This->nsstyle, STYLEID_BACKGROUND_POSITION, &pos_str);
    if(SUCCEEDED(hres)) {
        const PRUnichar *pos, *posy;
        DWORD posy_len;

        nsAString_GetData(&pos_str, &pos);
        posy = strchrW(pos, ' ');
        if(!posy) {
            static const WCHAR zero_pxW[] = {' ','0','p','x',0};

            TRACE("no space in %s\n", debugstr_w(pos));
            posy = zero_pxW;
        }

        posy_len = strlenW(posy);
        pos_val = heap_alloc((val_len+posy_len+1)*sizeof(WCHAR));
        if(pos_val) {
            if(val_len)
                memcpy(pos_val, val, val_len*sizeof(WCHAR));
            if(posy_len)
                memcpy(pos_val+val_len, posy, posy_len*sizeof(WCHAR));
            pos_val[val_len+posy_len] = 0;
        }else {
            hres = E_OUTOFMEMORY;
        }
    }
    nsAString_Finish(&pos_str);
    if(FAILED(hres))
        return hres;

    TRACE("setting position to %s\n", debugstr_w(pos_val));
    hres = set_nsstyle_attr(This->nsstyle, STYLEID_BACKGROUND_POSITION, pos_val, ATTR_FIX_PX);
    heap_free(pos_val);
    return hres;
}

static HRESULT WINAPI HTMLStyle_get_backgroundPositionX(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    nsAString pos_str;
    BSTR ret;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&pos_str, NULL);
    hres = get_nsstyle_attr_nsval(This->nsstyle, STYLEID_BACKGROUND_POSITION, &pos_str);
    if(SUCCEEDED(hres)) {
        const PRUnichar *pos, *space;

        nsAString_GetData(&pos_str, &pos);
        space = strchrW(pos, ' ');
        if(!space) {
            WARN("no space in %s\n", debugstr_w(pos));
            space = pos + strlenW(pos);
        }

        if(space != pos) {
            ret = SysAllocStringLen(pos, space-pos);
            if(!ret)
                hres = E_OUTOFMEMORY;
        }else {
            ret = NULL;
        }
    }
    nsAString_Finish(&pos_str);
    if(FAILED(hres))
        return hres;

    TRACE("returning %s\n", debugstr_w(ret));
    V_VT(p) = VT_BSTR;
    V_BSTR(p) = ret;
    return S_OK;
}

static HRESULT WINAPI HTMLStyle_put_backgroundPositionY(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    WCHAR buf[14], *pos_val;
    nsAString pos_str;
    const WCHAR *val;
    DWORD val_len;
    HRESULT hres;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    hres = var_to_styleval(&v, buf, ATTR_FIX_PX, &val);
    if(FAILED(hres))
        return hres;

    val_len = val ? strlenW(val) : 0;

    nsAString_Init(&pos_str, NULL);
    hres = get_nsstyle_attr_nsval(This->nsstyle, STYLEID_BACKGROUND_POSITION, &pos_str);
    if(SUCCEEDED(hres)) {
        const PRUnichar *pos, *space;
        DWORD posx_len;

        nsAString_GetData(&pos_str, &pos);
        space = strchrW(pos, ' ');
        if(space) {
            space++;
        }else {
            static const WCHAR zero_pxW[] = {'0','p','x',' ',0};

            TRACE("no space in %s\n", debugstr_w(pos));
            pos = zero_pxW;
            space = pos + sizeof(zero_pxW)/sizeof(WCHAR)-1;
        }

        posx_len = space-pos;

        pos_val = heap_alloc((posx_len+val_len+1)*sizeof(WCHAR));
        if(pos_val) {
            memcpy(pos_val, pos, posx_len*sizeof(WCHAR));
            if(val_len)
                memcpy(pos_val+posx_len, val, val_len*sizeof(WCHAR));
            pos_val[posx_len+val_len] = 0;
        }else {
            hres = E_OUTOFMEMORY;
        }
    }
    nsAString_Finish(&pos_str);
    if(FAILED(hres))
        return hres;

    TRACE("setting position to %s\n", debugstr_w(pos_val));
    hres = set_nsstyle_attr(This->nsstyle, STYLEID_BACKGROUND_POSITION, pos_val, ATTR_FIX_PX);
    heap_free(pos_val);
    return hres;
}

static HRESULT WINAPI HTMLStyle_get_backgroundPositionY(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    nsAString pos_str;
    BSTR ret;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&pos_str, NULL);
    hres = get_nsstyle_attr_nsval(This->nsstyle, STYLEID_BACKGROUND_POSITION, &pos_str);
    if(SUCCEEDED(hres)) {
        const PRUnichar *pos, *posy;

        nsAString_GetData(&pos_str, &pos);
        posy = strchrW(pos, ' ');
        if(posy) {
            ret = SysAllocString(posy+1);
            if(!ret)
                hres = E_OUTOFMEMORY;
        }else {
            ret = NULL;
        }
    }
    nsAString_Finish(&pos_str);
    if(FAILED(hres))
        return hres;

    TRACE("returning %s\n", debugstr_w(ret));
    V_VT(p) = VT_BSTR;
    V_BSTR(p) = ret;
    return S_OK;
}

static HRESULT WINAPI HTMLStyle_put_wordSpacing(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_WORD_SPACING, &v, 0);
}

static HRESULT WINAPI HTMLStyle_get_wordSpacing(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_WORD_SPACING, p, 0);
}

static HRESULT WINAPI HTMLStyle_put_letterSpacing(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_LETTER_SPACING, &v, 0);
}

static HRESULT WINAPI HTMLStyle_get_letterSpacing(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_LETTER_SPACING, p, 0);
}

static HRESULT WINAPI HTMLStyle_put_textDecoration(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    /* textDecoration can only be one of the following */
    if(!v || strcmpiW(styleNone, v)   == 0 || strcmpiW(valUnderline, v)   == 0 ||
             strcmpiW(valOverline, v) == 0 || strcmpiW(valLineThrough, v) == 0 ||
             strcmpiW(valBlink, v)    == 0)
    {
        return set_style_attr(This, STYLEID_TEXT_DECORATION , v, 0);
    }

    return E_INVALIDARG;
}

static HRESULT WINAPI HTMLStyle_get_textDecoration(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_TEXT_DECORATION, p);
}

static HRESULT WINAPI HTMLStyle_put_textDecorationNone(IHTMLStyle *iface, VARIANT_BOOL v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%x)\n", This, v);

    return set_style_attr(This, STYLEID_TEXT_DECORATION, v ? styleNone : emptyW, 0);
}

static HRESULT WINAPI HTMLStyle_get_textDecorationNone(IHTMLStyle *iface, VARIANT_BOOL *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return check_style_attr_value(This, STYLEID_TEXT_DECORATION, styleNone, p);
}

static HRESULT WINAPI HTMLStyle_put_textDecorationUnderline(IHTMLStyle *iface, VARIANT_BOOL v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%x)\n", This, v);

    return set_style_attr(This, STYLEID_TEXT_DECORATION, v ? valUnderline : emptyW, 0);
}

static HRESULT WINAPI HTMLStyle_get_textDecorationUnderline(IHTMLStyle *iface, VARIANT_BOOL *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return check_style_attr_value(This, STYLEID_TEXT_DECORATION, valUnderline, p);
}

static HRESULT WINAPI HTMLStyle_put_textDecorationOverline(IHTMLStyle *iface, VARIANT_BOOL v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%x)\n", This, v);

    return set_style_attr(This, STYLEID_TEXT_DECORATION, v ? valOverline : emptyW, 0);
}

static HRESULT WINAPI HTMLStyle_get_textDecorationOverline(IHTMLStyle *iface, VARIANT_BOOL *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return check_style_attr_value(This, STYLEID_TEXT_DECORATION, valOverline, p);
}

static HRESULT WINAPI HTMLStyle_put_textDecorationLineThrough(IHTMLStyle *iface, VARIANT_BOOL v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%x)\n", This, v);

    return set_style_attr(This, STYLEID_TEXT_DECORATION, v ? valLineThrough : emptyW, 0);
}

static HRESULT WINAPI HTMLStyle_get_textDecorationLineThrough(IHTMLStyle *iface, VARIANT_BOOL *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return check_style_attr_value(This, STYLEID_TEXT_DECORATION, valLineThrough, p);
}

static HRESULT WINAPI HTMLStyle_put_textDecorationBlink(IHTMLStyle *iface, VARIANT_BOOL v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%x)\n", This, v);

    return set_style_attr(This, STYLEID_TEXT_DECORATION, v ? valBlink : emptyW, 0);
}

static HRESULT WINAPI HTMLStyle_get_textDecorationBlink(IHTMLStyle *iface, VARIANT_BOOL *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return check_style_attr_value(This, STYLEID_TEXT_DECORATION, valBlink, p);
}

static HRESULT WINAPI HTMLStyle_put_verticalAlign(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_VERTICAL_ALIGN, &v, ATTR_FIX_PX);
}

static HRESULT WINAPI HTMLStyle_get_verticalAlign(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr_var(This->nsstyle, STYLEID_VERTICAL_ALIGN, p, 0);
}

static HRESULT WINAPI HTMLStyle_put_textTransform(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_attr(This, STYLEID_TEXT_TRANSFORM, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_textTransform(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_TEXT_TRANSFORM, p);
}

static HRESULT WINAPI HTMLStyle_put_textAlign(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_attr(This, STYLEID_TEXT_ALIGN, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_textAlign(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_TEXT_ALIGN, p);
}

static HRESULT WINAPI HTMLStyle_put_textIndent(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_TEXT_INDENT, &v, ATTR_FIX_PX);
}

static HRESULT WINAPI HTMLStyle_get_textIndent(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr_var(This->nsstyle, STYLEID_TEXT_INDENT, p, 0);
}

static HRESULT WINAPI HTMLStyle_put_lineHeight(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_LINE_HEIGHT, &v, 0);
}

static HRESULT WINAPI HTMLStyle_get_lineHeight(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr_var(This->nsstyle, STYLEID_LINE_HEIGHT, p, 0);
}

static HRESULT WINAPI HTMLStyle_put_marginTop(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_MARGIN_TOP, &v, ATTR_FIX_PX);
}

static HRESULT WINAPI HTMLStyle_get_marginTop(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr_var(This->nsstyle, STYLEID_MARGIN_TOP, p, 0);
}

static HRESULT WINAPI HTMLStyle_put_marginRight(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_MARGIN_RIGHT, &v, ATTR_FIX_PX);
}

static HRESULT WINAPI HTMLStyle_get_marginRight(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_MARGIN_RIGHT, p, 0);
}

static HRESULT WINAPI HTMLStyle_put_marginBottom(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_MARGIN_BOTTOM, &v, ATTR_FIX_PX);
}

static HRESULT WINAPI HTMLStyle_get_marginBottom(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr_var(This->nsstyle, STYLEID_MARGIN_BOTTOM, p, 0);
}

static HRESULT WINAPI HTMLStyle_put_marginLeft(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_MARGIN_LEFT, &v, ATTR_FIX_PX);
}

static HRESULT WINAPI HTMLStyle_put_margin(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_attr(This, STYLEID_MARGIN, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_margin(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_MARGIN, p);
}

static HRESULT WINAPI HTMLStyle_get_marginLeft(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_MARGIN_LEFT, p, 0);
}

static HRESULT WINAPI HTMLStyle_put_paddingTop(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_PADDING_TOP, &v, ATTR_FIX_PX);
}

static HRESULT WINAPI HTMLStyle_get_paddingTop(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr_var(This->nsstyle, STYLEID_PADDING_TOP, p, 0);
}

static HRESULT WINAPI HTMLStyle_put_paddingRight(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_PADDING_RIGHT, &v, ATTR_FIX_PX);
}

static HRESULT WINAPI HTMLStyle_get_paddingRight(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr_var(This->nsstyle, STYLEID_PADDING_RIGHT, p, 0);
}

static HRESULT WINAPI HTMLStyle_put_paddingBottom(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_PADDING_BOTTOM, &v, ATTR_FIX_PX);
}

static HRESULT WINAPI HTMLStyle_get_paddingBottom(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr_var(This->nsstyle, STYLEID_PADDING_BOTTOM, p, 0);
}

static HRESULT WINAPI HTMLStyle_put_paddingLeft(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_PADDING_LEFT, &v, ATTR_FIX_PX);
}

static HRESULT WINAPI HTMLStyle_get_paddingLeft(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr_var(This->nsstyle, STYLEID_PADDING_LEFT, p, 0);
}

static HRESULT WINAPI HTMLStyle_put_padding(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_attr(This, STYLEID_PADDING, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_padding(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_PADDING, p);
}

static HRESULT WINAPI HTMLStyle_put_border(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_attr(This, STYLEID_BORDER, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_border(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_BORDER, p);
}

static HRESULT WINAPI HTMLStyle_put_borderTop(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_attr(This, STYLEID_BORDER_TOP, v, ATTR_FIX_PX);
}

static HRESULT WINAPI HTMLStyle_get_borderTop(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_attr(This, STYLEID_BORDER_TOP, p);
}

static HRESULT WINAPI HTMLStyle_put_borderRight(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_attr(This, STYLEID_BORDER_RIGHT, v, ATTR_FIX_PX);
}

static HRESULT WINAPI HTMLStyle_get_borderRight(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_attr(This, STYLEID_BORDER_RIGHT, p);
}

static HRESULT WINAPI HTMLStyle_put_borderBottom(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_attr(This, STYLEID_BORDER_BOTTOM, v, ATTR_FIX_PX);
}

static HRESULT WINAPI HTMLStyle_get_borderBottom(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_attr(This, STYLEID_BORDER_BOTTOM, p);
}

static HRESULT WINAPI HTMLStyle_put_borderLeft(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_attr(This, STYLEID_BORDER_LEFT, v, ATTR_FIX_PX);
}

static HRESULT WINAPI HTMLStyle_get_borderLeft(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_BORDER_LEFT, p);
}

static HRESULT WINAPI HTMLStyle_put_borderColor(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_attr(This, STYLEID_BORDER_COLOR, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_borderColor(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_BORDER_COLOR, p);
}

static HRESULT WINAPI HTMLStyle_put_borderTopColor(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_BORDER_TOP_COLOR, &v, ATTR_HEX_INT);
}

static HRESULT WINAPI HTMLStyle_get_borderTopColor(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr_var(This->nsstyle, STYLEID_BORDER_TOP_COLOR, p, 0);
}

static HRESULT WINAPI HTMLStyle_put_borderRightColor(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_BORDER_RIGHT_COLOR, &v, ATTR_HEX_INT);
}

static HRESULT WINAPI HTMLStyle_get_borderRightColor(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr_var(This->nsstyle, STYLEID_BORDER_RIGHT_COLOR, p, 0);
}

static HRESULT WINAPI HTMLStyle_put_borderBottomColor(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_BORDER_BOTTOM_COLOR, &v, ATTR_HEX_INT);
}

static HRESULT WINAPI HTMLStyle_get_borderBottomColor(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr_var(This->nsstyle, STYLEID_BORDER_BOTTOM_COLOR, p, 0);
}

static HRESULT WINAPI HTMLStyle_put_borderLeftColor(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_BORDER_LEFT_COLOR, &v, ATTR_HEX_INT);
}

static HRESULT WINAPI HTMLStyle_get_borderLeftColor(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr_var(This->nsstyle, STYLEID_BORDER_LEFT_COLOR, p, 0);
}

static HRESULT WINAPI HTMLStyle_put_borderWidth(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_attr(This, STYLEID_BORDER_WIDTH, v, ATTR_FIX_PX);
}

static HRESULT WINAPI HTMLStyle_get_borderWidth(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_attr(This, STYLEID_BORDER_WIDTH, p);
}

static HRESULT WINAPI HTMLStyle_put_borderTopWidth(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_BORDER_TOP_WIDTH, &v, 0);
}

static HRESULT WINAPI HTMLStyle_get_borderTopWidth(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr_var(This->nsstyle, STYLEID_BORDER_TOP_WIDTH, p, 0);
}

static HRESULT WINAPI HTMLStyle_put_borderRightWidth(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_BORDER_RIGHT_WIDTH, &v, 0);
}

static HRESULT WINAPI HTMLStyle_get_borderRightWidth(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr_var(This->nsstyle, STYLEID_BORDER_RIGHT_WIDTH, p, 0);
}

static HRESULT WINAPI HTMLStyle_put_borderBottomWidth(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_BORDER_BOTTOM_WIDTH, &v, 0);
}

static HRESULT WINAPI HTMLStyle_get_borderBottomWidth(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_BORDER_BOTTOM_WIDTH, p, 0);
}

static HRESULT WINAPI HTMLStyle_put_borderLeftWidth(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_BORDER_LEFT_WIDTH, &v, 0);
}

static HRESULT WINAPI HTMLStyle_get_borderLeftWidth(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_BORDER_LEFT_WIDTH, p, 0);
}

static HRESULT WINAPI HTMLStyle_put_borderStyle(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    static const WCHAR styleWindowInset[]  = {'w','i','n','d','o','w','-','i','n','s','e','t',0};
    HRESULT hres = S_OK;
    BSTR pstyle;
    int i=0;
    int last = 0;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    while(v[i] && hres == S_OK)
    {
        if(v[i] == (WCHAR)' ')
        {
            pstyle = SysAllocStringLen(&v[last], (i-last));
            if( !(is_valid_border_style(pstyle) || strcmpiW(styleWindowInset, pstyle) == 0))
            {
                TRACE("1. Invalid style (%s)\n", debugstr_w(pstyle));
                hres = E_INVALIDARG;
            }
            SysFreeString(pstyle);
            last = i+1;
        }
        i++;
    }

    if(hres == S_OK)
    {
        pstyle = SysAllocStringLen(&v[last], i-last);
        if( !(is_valid_border_style(pstyle) || strcmpiW(styleWindowInset, pstyle) == 0))
        {
            TRACE("2. Invalid style (%s)\n", debugstr_w(pstyle));
            hres = E_INVALIDARG;
        }
        SysFreeString(pstyle);
    }

    if(hres == S_OK)
        hres = set_nsstyle_attr(This->nsstyle, STYLEID_BORDER_STYLE, v, 0);

    return hres;
}

static HRESULT WINAPI HTMLStyle_get_borderStyle(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_attr(This, STYLEID_BORDER_STYLE, p);
}

static HRESULT WINAPI HTMLStyle_put_borderTopStyle(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!is_valid_border_style(v))
        return E_INVALIDARG;

    return set_style_attr(This, STYLEID_BORDER_TOP_STYLE, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_borderTopStyle(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_attr(This, STYLEID_BORDER_TOP_STYLE, p);
}

static HRESULT WINAPI HTMLStyle_put_borderRightStyle(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!is_valid_border_style(v))
        return E_INVALIDARG;

    return set_style_attr(This, STYLEID_BORDER_RIGHT_STYLE, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_borderRightStyle(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_attr(This, STYLEID_BORDER_RIGHT_STYLE, p);
}

static HRESULT WINAPI HTMLStyle_put_borderBottomStyle(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!is_valid_border_style(v))
        return E_INVALIDARG;

    return set_style_attr(This, STYLEID_BORDER_BOTTOM_STYLE, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_borderBottomStyle(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_attr(This, STYLEID_BORDER_BOTTOM_STYLE, p);
}

static HRESULT WINAPI HTMLStyle_put_borderLeftStyle(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!is_valid_border_style(v))
        return E_INVALIDARG;

    return set_style_attr(This, STYLEID_BORDER_LEFT_STYLE, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_borderLeftStyle(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_attr(This, STYLEID_BORDER_LEFT_STYLE, p);
}

static HRESULT WINAPI HTMLStyle_put_width(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_WIDTH, &v, ATTR_FIX_PX);
}

static HRESULT WINAPI HTMLStyle_get_width(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr_var(This->nsstyle, STYLEID_WIDTH, p, 0);
}

static HRESULT WINAPI HTMLStyle_put_height(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_HEIGHT, &v, ATTR_FIX_PX);
}

static HRESULT WINAPI HTMLStyle_get_height(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr_var(This->nsstyle, STYLEID_HEIGHT, p, 0);
}

static HRESULT WINAPI HTMLStyle_put_styleFloat(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_styleFloat(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_clear(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_attr(This, STYLEID_CLEAR, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_clear(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_CLEAR, p);
}

static HRESULT WINAPI HTMLStyle_put_display(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_attr(This, STYLEID_DISPLAY, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_display(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_DISPLAY, p);
}

static HRESULT WINAPI HTMLStyle_put_visibility(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_attr(This, STYLEID_VISIBILITY, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_visibility(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_VISIBILITY, p);
}

static HRESULT WINAPI HTMLStyle_put_listStyleType(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_attr(This, STYLEID_LISTSTYLETYPE, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_listStyleType(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_LISTSTYLETYPE, p);
}

static HRESULT WINAPI HTMLStyle_put_listStylePosition(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_attr(This, STYLEID_LISTSTYLEPOSITION, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_listStylePosition(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_LISTSTYLEPOSITION, p);
}

static HRESULT WINAPI HTMLStyle_put_listStyleImage(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_listStyleImage(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_listStyle(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_attr(This, STYLEID_LIST_STYLE, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_listStyle(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_LIST_STYLE, p);
}

static HRESULT WINAPI HTMLStyle_put_whiteSpace(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_nsstyle_attr(This->nsstyle, STYLEID_WHITE_SPACE, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_whiteSpace(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr(This->nsstyle, STYLEID_WHITE_SPACE, p, 0);
}

static HRESULT WINAPI HTMLStyle_put_top(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_TOP, &v, 0);
}

static HRESULT WINAPI HTMLStyle_get_top(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr_var(This->nsstyle, STYLEID_TOP, p, 0);
}

static HRESULT WINAPI HTMLStyle_put_left(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_LEFT, &v, 0);
}

static HRESULT WINAPI HTMLStyle_get_left(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr_var(This->nsstyle, STYLEID_LEFT, p, 0);
}

static HRESULT WINAPI HTMLStyle_get_position(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return IHTMLStyle2_get_position(&This->IHTMLStyle2_iface, p);
}

static HRESULT WINAPI HTMLStyle_put_zIndex(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_Z_INDEX, &v, 0);
}

static HRESULT WINAPI HTMLStyle_get_zIndex(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr_var(This->nsstyle, STYLEID_Z_INDEX, p, ATTR_STR_TO_INT);
}

static HRESULT WINAPI HTMLStyle_put_overflow(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    static const WCHAR szVisible[] = {'v','i','s','i','b','l','e',0};
    static const WCHAR szScroll[]  = {'s','c','r','o','l','l',0};
    static const WCHAR szHidden[]  = {'h','i','d','d','e','n',0};
    static const WCHAR szAuto[]    = {'a','u','t','o',0};

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    /* overflow can only be one of the follow values. */
    if(!v || !*v || strcmpiW(szVisible, v) == 0 || strcmpiW(szScroll, v) == 0 ||
             strcmpiW(szHidden, v) == 0  || strcmpiW(szAuto, v) == 0)
    {
        return set_nsstyle_attr(This->nsstyle, STYLEID_OVERFLOW, v, 0);
    }

    return E_INVALIDARG;
}


static HRESULT WINAPI HTMLStyle_get_overflow(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
       return E_INVALIDARG;

    return get_style_attr(This, STYLEID_OVERFLOW, p);
}

static HRESULT WINAPI HTMLStyle_put_pageBreakBefore(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_nsstyle_attr(This->nsstyle, STYLEID_PAGE_BREAK_BEFORE, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_pageBreakBefore(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr(This->nsstyle, STYLEID_PAGE_BREAK_BEFORE, p, 0);
}

static HRESULT WINAPI HTMLStyle_put_pageBreakAfter(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_nsstyle_attr(This->nsstyle, STYLEID_PAGE_BREAK_AFTER, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_pageBreakAfter(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr(This->nsstyle, STYLEID_PAGE_BREAK_AFTER, p, 0);
}

static HRESULT WINAPI HTMLStyle_put_cssText(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    nsAString text_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&text_str, v);
    nsres = nsIDOMCSSStyleDeclaration_SetCssText(This->nsstyle, &text_str);
    nsAString_Finish(&text_str);
    if(NS_FAILED(nsres)) {
        FIXME("SetCssStyle failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLStyle_get_cssText(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    nsAString text_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    /* FIXME: Gecko style formatting is different than IE (uppercase). */
    nsAString_Init(&text_str, NULL);
    nsres = nsIDOMCSSStyleDeclaration_GetCssText(This->nsstyle, &text_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *text;

        nsAString_GetData(&text_str, &text);
        *p = *text ? SysAllocString(text) : NULL;
    }else {
        FIXME("GetCssStyle failed: %08x\n", nsres);
        *p = NULL;
    }

    nsAString_Finish(&text_str);
    return S_OK;
}

static HRESULT WINAPI HTMLStyle_put_pixelTop(IHTMLStyle *iface, LONG v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%d)\n", This, v);

    return set_style_pxattr(This->nsstyle, STYLEID_TOP, v);
}

static HRESULT WINAPI HTMLStyle_get_pixelTop(IHTMLStyle *iface, LONG *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_pixel_val(This, STYLEID_TOP, p);
}

static HRESULT WINAPI HTMLStyle_put_pixelLeft(IHTMLStyle *iface, LONG v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%d)\n", This, v);

    return set_style_pxattr(This->nsstyle, STYLEID_LEFT, v);
}

static HRESULT WINAPI HTMLStyle_get_pixelLeft(IHTMLStyle *iface, LONG *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_pixel_val(This, STYLEID_LEFT, p);
}

static HRESULT WINAPI HTMLStyle_put_pixelWidth(IHTMLStyle *iface, LONG v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->()\n", This);

    return set_style_pxattr(This->nsstyle, STYLEID_WIDTH, v);
}

static HRESULT WINAPI HTMLStyle_get_pixelWidth(IHTMLStyle *iface, LONG *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_pixel_val(This, STYLEID_WIDTH, p);
}

static HRESULT WINAPI HTMLStyle_put_pixelHeight(IHTMLStyle *iface, LONG v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%d)\n", This, v);

    return set_style_pxattr(This->nsstyle, STYLEID_HEIGHT, v);
}

static HRESULT WINAPI HTMLStyle_get_pixelHeight(IHTMLStyle *iface, LONG *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_pixel_val(This, STYLEID_HEIGHT, p);
}

static HRESULT WINAPI HTMLStyle_put_posTop(IHTMLStyle *iface, float v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%f)\n", This, v);

    return set_style_pos(This, STYLEID_TOP, v);
}

static HRESULT WINAPI HTMLStyle_get_posTop(IHTMLStyle *iface, float *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    return get_nsstyle_pos(This, STYLEID_TOP, p);
}

static HRESULT WINAPI HTMLStyle_put_posLeft(IHTMLStyle *iface, float v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%f)\n", This, v);

    return set_style_pos(This, STYLEID_LEFT, v);
}

static HRESULT WINAPI HTMLStyle_get_posLeft(IHTMLStyle *iface, float *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    return get_nsstyle_pos(This, STYLEID_LEFT, p);
}

static HRESULT WINAPI HTMLStyle_put_posWidth(IHTMLStyle *iface, float v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%f)\n", This, v);

    return set_style_pos(This, STYLEID_WIDTH, v);
}

static HRESULT WINAPI HTMLStyle_get_posWidth(IHTMLStyle *iface, float *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    if(get_nsstyle_pos(This, STYLEID_WIDTH, p) != S_OK)
        *p = 0.0f;

    return S_OK;
}

static HRESULT WINAPI HTMLStyle_put_posHeight(IHTMLStyle *iface, float v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%f)\n", This, v);

    return set_style_pos(This, STYLEID_HEIGHT, v);
}

static HRESULT WINAPI HTMLStyle_get_posHeight(IHTMLStyle *iface, float *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    if(get_nsstyle_pos(This, STYLEID_HEIGHT, p) != S_OK)
        *p = 0.0f;

    return S_OK;
}

static HRESULT WINAPI HTMLStyle_put_cursor(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_attr(This, STYLEID_CURSOR, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_cursor(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_CURSOR, p);
}

static HRESULT WINAPI HTMLStyle_put_clip(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_attr(This, STYLEID_CLIP, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_clip(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr(This->nsstyle, STYLEID_CLIP, p, ATTR_REMOVE_COMMA);
}

static void set_opacity(HTMLStyle *This, const WCHAR *val)
{
    nsAString name_str, val_str, empty_str;
    nsresult nsres;

    static const WCHAR opacityW[] = {'o','p','a','c','i','t','y',0};

    TRACE("%s\n", debugstr_w(val));

    nsAString_InitDepend(&name_str, opacityW);
    nsAString_InitDepend(&val_str, val);
    nsAString_InitDepend(&empty_str, emptyW);

    nsres = nsIDOMCSSStyleDeclaration_SetProperty(This->nsstyle, &name_str, &val_str, &empty_str);
    if(NS_FAILED(nsres))
        ERR("SetProperty failed: %08x\n", nsres);

    nsAString_Finish(&name_str);
    nsAString_Finish(&val_str);
    nsAString_Finish(&empty_str);
}

static void update_filter(HTMLStyle *This)
{
    const WCHAR *ptr = This->elem->filter, *ptr2;

    static const WCHAR alphaW[] = {'a','l','p','h','a'};

    if(!ptr) {
        set_opacity(This, emptyW);
        return;
    }

    while(1) {
        while(isspaceW(*ptr))
            ptr++;
        if(!*ptr)
            break;

        ptr2 = ptr;
        while(isalnumW(*ptr))
            ptr++;
        if(ptr == ptr2) {
            WARN("unexpected char '%c'\n", *ptr);
            break;
        }
        if(*ptr != '(') {
            WARN("expected '('\n");
            continue;
        }

        if(ptr2 + sizeof(alphaW)/sizeof(WCHAR) == ptr && !memcmp(ptr2, alphaW, sizeof(alphaW))) {
            static const WCHAR formatW[] = {'%','f',0};
            static const WCHAR opacityW[] = {'o','p','a','c','i','t','y','='};

            ptr++;
            do {
                while(isspaceW(*ptr))
                    ptr++;

                ptr2 = ptr;
                while(*ptr && *ptr != ',' && *ptr != ')')
                    ptr++;
                if(!*ptr) {
                    WARN("unexpected end of string\n");
                    break;
                }

                if(ptr-ptr2 > sizeof(opacityW)/sizeof(WCHAR) && !memcmp(ptr2, opacityW, sizeof(opacityW))) {
                    float fval = 0.0f, e = 0.1f;
                    WCHAR buf[32];

                    ptr2 += sizeof(opacityW)/sizeof(WCHAR);

                    while(isdigitW(*ptr2))
                        fval = fval*10.0f + (float)(*ptr2++ - '0');

                    if(*ptr2 == '.') {
                        while(isdigitW(*++ptr2)) {
                            fval += e * (float)(*ptr2++ - '0');
                            e *= 0.1f;
                        }
                    }

                    sprintfW(buf, formatW, fval * 0.01f);
                    set_opacity(This, buf);
                }else {
                    FIXME("unknown param %s\n", debugstr_wn(ptr2, ptr-ptr2));
                }

                if(*ptr == ',')
                    ptr++;
            }while(*ptr != ')');
        }else {
            FIXME("unknown filter %s\n", debugstr_wn(ptr2, ptr-ptr2));
            ptr = strchrW(ptr, ')');
            if(!ptr)
                break;
            ptr++;
        }
    }
}

static HRESULT WINAPI HTMLStyle_put_filter(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    WCHAR *new_filter = NULL;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->elem) {
        FIXME("Element already destroyed\n");
        return E_UNEXPECTED;
    }

    if(v) {
        new_filter = heap_strdupW(v);
        if(!new_filter)
            return E_OUTOFMEMORY;
    }

    heap_free(This->elem->filter);
    This->elem->filter = new_filter;

    update_filter(This);
    return S_OK;
}

static HRESULT WINAPI HTMLStyle_get_filter(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->elem) {
        FIXME("Element already destroyed\n");
        return E_UNEXPECTED;
    }

    if(This->elem->filter) {
        *p = SysAllocString(This->elem->filter);
        if(!*p)
            return E_OUTOFMEMORY;
    }else {
        *p = NULL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLStyle_setAttribute(IHTMLStyle *iface, BSTR strAttributeName,
        VARIANT AttributeValue, LONG lFlags)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    HRESULT hres;
    DISPID dispid;

    TRACE("(%p)->(%s %s %08x)\n", This, debugstr_w(strAttributeName),
          debugstr_variant(&AttributeValue), lFlags);

    if(!strAttributeName)
        return E_INVALIDARG;

    if(lFlags == 1)
        FIXME("Parameter lFlags ignored\n");

    hres = HTMLStyle_GetIDsOfNames(iface, &IID_NULL, &strAttributeName, 1,
                        LOCALE_USER_DEFAULT, &dispid);
    if(hres == S_OK)
    {
        VARIANT ret;
        DISPID dispidNamed = DISPID_PROPERTYPUT;
        DISPPARAMS params;

        params.cArgs = 1;
        params.rgvarg = &AttributeValue;
        params.cNamedArgs = 1;
        params.rgdispidNamedArgs = &dispidNamed;

        hres = HTMLStyle_Invoke(iface, dispid, &IID_NULL, LOCALE_SYSTEM_DEFAULT,
            DISPATCH_PROPERTYPUT, &params, &ret, NULL, NULL);
    }
    else
    {
        FIXME("Custom attributes not supported.\n");
    }

    TRACE("ret: %08x\n", hres);

    return hres;
}

static HRESULT WINAPI HTMLStyle_getAttribute(IHTMLStyle *iface, BSTR strAttributeName,
        LONG lFlags, VARIANT *AttributeValue)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    HRESULT hres;
    DISPID dispid;

    TRACE("(%p)->(%s v%p %08x)\n", This, debugstr_w(strAttributeName),
          AttributeValue, lFlags);

    if(!AttributeValue || !strAttributeName)
        return E_INVALIDARG;

    if(lFlags == 1)
        FIXME("Parameter lFlags ignored\n");

    hres = HTMLStyle_GetIDsOfNames(iface, &IID_NULL, &strAttributeName, 1,
                        LOCALE_USER_DEFAULT, &dispid);
    if(hres == S_OK)
    {
        DISPPARAMS params = {NULL, NULL, 0, 0 };

        hres = HTMLStyle_Invoke(iface, dispid, &IID_NULL, LOCALE_SYSTEM_DEFAULT,
            DISPATCH_PROPERTYGET, &params, AttributeValue, NULL, NULL);
    }
    else
    {
        FIXME("Custom attributes not supported.\n");
    }

    return hres;
}

static HRESULT WINAPI HTMLStyle_removeAttribute(IHTMLStyle *iface, BSTR strAttributeName,
                                                LONG lFlags, VARIANT_BOOL *pfSuccess)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    const style_tbl_entry_t *style_entry;
    nsAString name_str, ret_str;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s %08x %p)\n", This, debugstr_w(strAttributeName), lFlags, pfSuccess);

    style_entry = lookup_style_tbl(strAttributeName);
    if(!style_entry) {
        DISPID dispid;
        unsigned i;

        hres = IDispatchEx_GetDispID(&This->dispex.IDispatchEx_iface, strAttributeName,
                (lFlags&1) ? fdexNameCaseSensitive : fdexNameCaseInsensitive, &dispid);
        if(hres != S_OK) {
            *pfSuccess = VARIANT_FALSE;
            return S_OK;
        }

        for(i=0; i < sizeof(style_tbl)/sizeof(*style_tbl); i++) {
            if(dispid == style_tbl[i].dispid)
                break;
        }

        if(i == sizeof(style_tbl)/sizeof(*style_tbl))
            return remove_prop(&This->dispex, strAttributeName, pfSuccess);
        style_entry = style_tbl+i;
    }

    /* filter property is a special case */
    if(style_entry->dispid == DISPID_IHTMLSTYLE_FILTER) {
        *pfSuccess = This->elem->filter && *This->elem->filter ? VARIANT_TRUE : VARIANT_FALSE;
        heap_free(This->elem->filter);
        This->elem->filter = NULL;
        update_filter(This);
        return S_OK;
    }

    nsAString_InitDepend(&name_str, style_entry->name);
    nsAString_Init(&ret_str, NULL);
    nsres = nsIDOMCSSStyleDeclaration_RemoveProperty(This->nsstyle, &name_str, &ret_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *ret;
        nsAString_GetData(&ret_str, &ret);
        *pfSuccess = *ret ? VARIANT_TRUE : VARIANT_FALSE;
    }else {
        ERR("RemoveProperty failed: %08x\n", nsres);
    }
    nsAString_Finish(&name_str);
    nsAString_Finish(&ret_str);
    return NS_SUCCEEDED(nsres) ? S_OK : E_FAIL;
}

static HRESULT WINAPI HTMLStyle_toString(IHTMLStyle *iface, BSTR *String)
{
    HTMLStyle *This = impl_from_IHTMLStyle(iface);
    FIXME("(%p)->(%p)\n", This, String);
    return E_NOTIMPL;
}

static const IHTMLStyleVtbl HTMLStyleVtbl = {
    HTMLStyle_QueryInterface,
    HTMLStyle_AddRef,
    HTMLStyle_Release,
    HTMLStyle_GetTypeInfoCount,
    HTMLStyle_GetTypeInfo,
    HTMLStyle_GetIDsOfNames,
    HTMLStyle_Invoke,
    HTMLStyle_put_fontFamily,
    HTMLStyle_get_fontFamily,
    HTMLStyle_put_fontStyle,
    HTMLStyle_get_fontStyle,
    HTMLStyle_put_fontVariant,
    HTMLStyle_get_fontVariant,
    HTMLStyle_put_fontWeight,
    HTMLStyle_get_fontWeight,
    HTMLStyle_put_fontSize,
    HTMLStyle_get_fontSize,
    HTMLStyle_put_font,
    HTMLStyle_get_font,
    HTMLStyle_put_color,
    HTMLStyle_get_color,
    HTMLStyle_put_background,
    HTMLStyle_get_background,
    HTMLStyle_put_backgroundColor,
    HTMLStyle_get_backgroundColor,
    HTMLStyle_put_backgroundImage,
    HTMLStyle_get_backgroundImage,
    HTMLStyle_put_backgroundRepeat,
    HTMLStyle_get_backgroundRepeat,
    HTMLStyle_put_backgroundAttachment,
    HTMLStyle_get_backgroundAttachment,
    HTMLStyle_put_backgroundPosition,
    HTMLStyle_get_backgroundPosition,
    HTMLStyle_put_backgroundPositionX,
    HTMLStyle_get_backgroundPositionX,
    HTMLStyle_put_backgroundPositionY,
    HTMLStyle_get_backgroundPositionY,
    HTMLStyle_put_wordSpacing,
    HTMLStyle_get_wordSpacing,
    HTMLStyle_put_letterSpacing,
    HTMLStyle_get_letterSpacing,
    HTMLStyle_put_textDecoration,
    HTMLStyle_get_textDecoration,
    HTMLStyle_put_textDecorationNone,
    HTMLStyle_get_textDecorationNone,
    HTMLStyle_put_textDecorationUnderline,
    HTMLStyle_get_textDecorationUnderline,
    HTMLStyle_put_textDecorationOverline,
    HTMLStyle_get_textDecorationOverline,
    HTMLStyle_put_textDecorationLineThrough,
    HTMLStyle_get_textDecorationLineThrough,
    HTMLStyle_put_textDecorationBlink,
    HTMLStyle_get_textDecorationBlink,
    HTMLStyle_put_verticalAlign,
    HTMLStyle_get_verticalAlign,
    HTMLStyle_put_textTransform,
    HTMLStyle_get_textTransform,
    HTMLStyle_put_textAlign,
    HTMLStyle_get_textAlign,
    HTMLStyle_put_textIndent,
    HTMLStyle_get_textIndent,
    HTMLStyle_put_lineHeight,
    HTMLStyle_get_lineHeight,
    HTMLStyle_put_marginTop,
    HTMLStyle_get_marginTop,
    HTMLStyle_put_marginRight,
    HTMLStyle_get_marginRight,
    HTMLStyle_put_marginBottom,
    HTMLStyle_get_marginBottom,
    HTMLStyle_put_marginLeft,
    HTMLStyle_get_marginLeft,
    HTMLStyle_put_margin,
    HTMLStyle_get_margin,
    HTMLStyle_put_paddingTop,
    HTMLStyle_get_paddingTop,
    HTMLStyle_put_paddingRight,
    HTMLStyle_get_paddingRight,
    HTMLStyle_put_paddingBottom,
    HTMLStyle_get_paddingBottom,
    HTMLStyle_put_paddingLeft,
    HTMLStyle_get_paddingLeft,
    HTMLStyle_put_padding,
    HTMLStyle_get_padding,
    HTMLStyle_put_border,
    HTMLStyle_get_border,
    HTMLStyle_put_borderTop,
    HTMLStyle_get_borderTop,
    HTMLStyle_put_borderRight,
    HTMLStyle_get_borderRight,
    HTMLStyle_put_borderBottom,
    HTMLStyle_get_borderBottom,
    HTMLStyle_put_borderLeft,
    HTMLStyle_get_borderLeft,
    HTMLStyle_put_borderColor,
    HTMLStyle_get_borderColor,
    HTMLStyle_put_borderTopColor,
    HTMLStyle_get_borderTopColor,
    HTMLStyle_put_borderRightColor,
    HTMLStyle_get_borderRightColor,
    HTMLStyle_put_borderBottomColor,
    HTMLStyle_get_borderBottomColor,
    HTMLStyle_put_borderLeftColor,
    HTMLStyle_get_borderLeftColor,
    HTMLStyle_put_borderWidth,
    HTMLStyle_get_borderWidth,
    HTMLStyle_put_borderTopWidth,
    HTMLStyle_get_borderTopWidth,
    HTMLStyle_put_borderRightWidth,
    HTMLStyle_get_borderRightWidth,
    HTMLStyle_put_borderBottomWidth,
    HTMLStyle_get_borderBottomWidth,
    HTMLStyle_put_borderLeftWidth,
    HTMLStyle_get_borderLeftWidth,
    HTMLStyle_put_borderStyle,
    HTMLStyle_get_borderStyle,
    HTMLStyle_put_borderTopStyle,
    HTMLStyle_get_borderTopStyle,
    HTMLStyle_put_borderRightStyle,
    HTMLStyle_get_borderRightStyle,
    HTMLStyle_put_borderBottomStyle,
    HTMLStyle_get_borderBottomStyle,
    HTMLStyle_put_borderLeftStyle,
    HTMLStyle_get_borderLeftStyle,
    HTMLStyle_put_width,
    HTMLStyle_get_width,
    HTMLStyle_put_height,
    HTMLStyle_get_height,
    HTMLStyle_put_styleFloat,
    HTMLStyle_get_styleFloat,
    HTMLStyle_put_clear,
    HTMLStyle_get_clear,
    HTMLStyle_put_display,
    HTMLStyle_get_display,
    HTMLStyle_put_visibility,
    HTMLStyle_get_visibility,
    HTMLStyle_put_listStyleType,
    HTMLStyle_get_listStyleType,
    HTMLStyle_put_listStylePosition,
    HTMLStyle_get_listStylePosition,
    HTMLStyle_put_listStyleImage,
    HTMLStyle_get_listStyleImage,
    HTMLStyle_put_listStyle,
    HTMLStyle_get_listStyle,
    HTMLStyle_put_whiteSpace,
    HTMLStyle_get_whiteSpace,
    HTMLStyle_put_top,
    HTMLStyle_get_top,
    HTMLStyle_put_left,
    HTMLStyle_get_left,
    HTMLStyle_get_position,
    HTMLStyle_put_zIndex,
    HTMLStyle_get_zIndex,
    HTMLStyle_put_overflow,
    HTMLStyle_get_overflow,
    HTMLStyle_put_pageBreakBefore,
    HTMLStyle_get_pageBreakBefore,
    HTMLStyle_put_pageBreakAfter,
    HTMLStyle_get_pageBreakAfter,
    HTMLStyle_put_cssText,
    HTMLStyle_get_cssText,
    HTMLStyle_put_pixelTop,
    HTMLStyle_get_pixelTop,
    HTMLStyle_put_pixelLeft,
    HTMLStyle_get_pixelLeft,
    HTMLStyle_put_pixelWidth,
    HTMLStyle_get_pixelWidth,
    HTMLStyle_put_pixelHeight,
    HTMLStyle_get_pixelHeight,
    HTMLStyle_put_posTop,
    HTMLStyle_get_posTop,
    HTMLStyle_put_posLeft,
    HTMLStyle_get_posLeft,
    HTMLStyle_put_posWidth,
    HTMLStyle_get_posWidth,
    HTMLStyle_put_posHeight,
    HTMLStyle_get_posHeight,
    HTMLStyle_put_cursor,
    HTMLStyle_get_cursor,
    HTMLStyle_put_clip,
    HTMLStyle_get_clip,
    HTMLStyle_put_filter,
    HTMLStyle_get_filter,
    HTMLStyle_setAttribute,
    HTMLStyle_getAttribute,
    HTMLStyle_removeAttribute,
    HTMLStyle_toString
};

static HRESULT HTMLStyle_get_dispid(DispatchEx *dispex, BSTR name, DWORD flags, DISPID *dispid)
{
    const style_tbl_entry_t *style_entry;

    style_entry = lookup_style_tbl(name);
    if(style_entry) {
        *dispid = style_entry->dispid;
        return S_OK;
    }

    return DISP_E_UNKNOWNNAME;
}

static const dispex_static_data_vtbl_t HTMLStyle_dispex_vtbl = {
    NULL,
    HTMLStyle_get_dispid,
    NULL,
    NULL
};

static const tid_t HTMLStyle_iface_tids[] = {
    IHTMLStyle_tid,
    IHTMLStyle2_tid,
    IHTMLStyle3_tid,
    IHTMLStyle4_tid,
    IHTMLStyle5_tid,
    IHTMLStyle6_tid,
    0
};
static dispex_static_data_t HTMLStyle_dispex = {
    &HTMLStyle_dispex_vtbl,
    DispHTMLStyle_tid,
    NULL,
    HTMLStyle_iface_tids
};

static HRESULT get_style_from_elem(HTMLElement *elem, nsIDOMCSSStyleDeclaration **ret)
{
    nsIDOMElementCSSInlineStyle *nselemstyle;
    nsresult nsres;

    if(!elem->nselem) {
        FIXME("NULL nselem\n");
        return E_NOTIMPL;
    }

    nsres = nsIDOMHTMLElement_QueryInterface(elem->nselem, &IID_nsIDOMElementCSSInlineStyle,
            (void**)&nselemstyle);
    assert(nsres == NS_OK);

    nsres = nsIDOMElementCSSInlineStyle_GetStyle(nselemstyle, ret);
    nsIDOMElementCSSInlineStyle_Release(nselemstyle);
    if(NS_FAILED(nsres)) {
        ERR("GetStyle failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

HRESULT HTMLStyle_Create(HTMLElement *elem, HTMLStyle **ret)
{
    nsIDOMCSSStyleDeclaration *nsstyle;
    HTMLStyle *style;
    HRESULT hres;

    hres = get_style_from_elem(elem, &nsstyle);
    if(FAILED(hres))
        return hres;

    style = heap_alloc_zero(sizeof(HTMLStyle));
    if(!style) {
        nsIDOMCSSStyleDeclaration_Release(nsstyle);
        return E_OUTOFMEMORY;
    }

    style->IHTMLStyle_iface.lpVtbl = &HTMLStyleVtbl;
    style->ref = 1;
    style->nsstyle = nsstyle;
    style->elem = elem;
    HTMLStyle2_Init(style);
    HTMLStyle3_Init(style);

    nsIDOMCSSStyleDeclaration_AddRef(nsstyle);

    init_dispex(&style->dispex, (IUnknown*)&style->IHTMLStyle_iface, &HTMLStyle_dispex);

    *ret = style;
    return S_OK;
}

HRESULT get_elem_style(HTMLElement *elem, styleid_t styleid, BSTR *ret)
{
    nsIDOMCSSStyleDeclaration *style;
    HRESULT hres;

    hres = get_style_from_elem(elem, &style);
    if(FAILED(hres))
        return hres;

    hres = get_nsstyle_attr(style, styleid, ret, 0);
    nsIDOMCSSStyleDeclaration_Release(style);
    return hres;
}

HRESULT set_elem_style(HTMLElement *elem, styleid_t styleid, const WCHAR *val)
{
    nsIDOMCSSStyleDeclaration *style;
    HRESULT hres;

    hres = get_style_from_elem(elem, &style);
    if(FAILED(hres))
        return hres;

    hres = set_nsstyle_attr(style, styleid, val, 0);
    nsIDOMCSSStyleDeclaration_Release(style);
    return hres;
}
