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

#include <stdarg.h>
#include <math.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "mshtmdid.h"

#include "mshtml_private.h"
#include "htmlstyle.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

static const WCHAR attrBackground[] =
    {'b','a','c','k','g','r','o','u','n','d',0};
static const WCHAR attrBackgroundColor[] =
    {'b','a','c','k','g','r','o','u','n','d','-','c','o','l','o','r',0};
static const WCHAR attrBackgroundImage[] =
    {'b','a','c','k','g','r','o','u','n','d','-','i','m','a','g','e',0};
static const WCHAR attrBorder[] =
    {'b','o','r','d','e','r',0};
static const WCHAR attrBorderBottomStyle[] =
    {'b','o','r','d','e','r','-','b','o','t','t','o','m','-','s','t','y','l','e',0};
static const WCHAR attrBorderLeft[] =
    {'b','o','r','d','e','r','-','l','e','f','t',0};
static const WCHAR attrBorderLeftStyle[] =
    {'b','o','r','d','e','r','-','l','e','f','t','-','s','t','y','l','e',0};
static const WCHAR attrBorderRightStyle[] =
    {'b','o','r','d','e','r','-','r','i','g','h','t','-','s','t','y','l','e',0};
static const WCHAR attrBorderTopStyle[] =
    {'b','o','r','d','e','r','-','t','o','p','-','s','t','y','l','e',0};
static const WCHAR attrBorderWidth[] =
    {'b','o','r','d','e','r','-','w','i','d','t','h',0};
static const WCHAR attrColor[] =
    {'c','o','l','o','r',0};
static const WCHAR attrCursor[] =
    {'c','u','r','s','o','r',0};
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
static const WCHAR attrMargin[] =
    {'m','a','r','g','i','n',0};
static const WCHAR attrMarginLeft[] =
    {'m','a','r','g','i','n','-','l','e','f','t',0};
static const WCHAR attrMarginRight[] =
    {'m','a','r','g','i','n','-','r','i','g','h','t',0};
static const WCHAR attrOverflow[] =
    {'o','v','e','r','f','l','o','w',0};
static const WCHAR attrPaddingLeft[] =
    {'p','a','d','d','i','n','g','-','l','e','f','t',0};
static const WCHAR attrPosition[] =
    {'p','o','s','i','t','i','o','n',0};
static const WCHAR attrTextAlign[] =
    {'t','e','x','t','-','a','l','i','g','n',0};
static const WCHAR attrTextDecoration[] =
    {'t','e','x','t','-','d','e','c','o','r','a','t','i','o','n',0};
static const WCHAR attrTop[] =
    {'t','o','p',0};
static const WCHAR attrVerticalAlign[] =
    {'v','e','r','t','i','c','a','l','-','a','l','i','g','n',0};
static const WCHAR attrVisibility[] =
    {'v','i','s','i','b','i','l','i','t','y',0};
static const WCHAR attrWidth[] =
    {'w','i','d','t','h',0};
static const WCHAR attrZIndex[] =
    {'z','-','i','n','d','e','x',0};

static const struct{
    const WCHAR *name;
    DISPID dispid;
} style_tbl[] = {
    {attrBackground,           DISPID_IHTMLSTYLE_BACKGROUND},
    {attrBackgroundColor,      DISPID_IHTMLSTYLE_BACKGROUNDCOLOR},
    {attrBackgroundImage,      DISPID_IHTMLSTYLE_BACKGROUNDIMAGE},
    {attrBorder,               DISPID_IHTMLSTYLE_BORDER},
    {attrBorderBottomStyle,    DISPID_IHTMLSTYLE_BORDERBOTTOMSTYLE},
    {attrBorderLeft,           DISPID_IHTMLSTYLE_BORDERLEFT},
    {attrBorderLeftStyle,      DISPID_IHTMLSTYLE_BORDERLEFTSTYLE},
    {attrBorderRightStyle,     DISPID_IHTMLSTYLE_BORDERRIGHTSTYLE},
    {attrBorderTopStyle,       DISPID_IHTMLSTYLE_BORDERTOPSTYLE},
    {attrBorderWidth,          DISPID_IHTMLSTYLE_BORDERWIDTH},
    {attrColor,                DISPID_IHTMLSTYLE_COLOR},
    {attrCursor,               DISPID_IHTMLSTYLE_CURSOR},
    {attrDisplay,              DISPID_IHTMLSTYLE_DISPLAY},
    {attrFilter,               DISPID_IHTMLSTYLE_FILTER},
    {attrFontFamily,           DISPID_IHTMLSTYLE_FONTFAMILY},
    {attrFontSize,             DISPID_IHTMLSTYLE_FONTSIZE},
    {attrFontStyle,            DISPID_IHTMLSTYLE_FONTSTYLE},
    {attrFontVariant,          DISPID_IHTMLSTYLE_FONTVARIANT},
    {attrFontWeight,           DISPID_IHTMLSTYLE_FONTWEIGHT},
    {attrHeight,               DISPID_IHTMLSTYLE_HEIGHT},
    {attrLeft,                 DISPID_IHTMLSTYLE_LEFT},
    {attrMargin,               DISPID_IHTMLSTYLE_MARGIN},
    {attrMarginLeft,           DISPID_IHTMLSTYLE_MARGINLEFT},
    {attrMarginRight,          DISPID_IHTMLSTYLE_MARGINRIGHT},
    {attrOverflow,             DISPID_IHTMLSTYLE_OVERFLOW},
    {attrPaddingLeft,          DISPID_IHTMLSTYLE_PADDINGLEFT},
    {attrPosition,             DISPID_IHTMLSTYLE2_POSITION},
    {attrTextAlign,            DISPID_IHTMLSTYLE_TEXTALIGN},
    {attrTextDecoration,       DISPID_IHTMLSTYLE_TEXTDECORATION},
    {attrTop,                  DISPID_IHTMLSTYLE_TOP},
    {attrVerticalAlign,        DISPID_IHTMLSTYLE_VERTICALALIGN},
    {attrVisibility,           DISPID_IHTMLSTYLE_VISIBILITY},
    {attrWidth,                DISPID_IHTMLSTYLE_WIDTH},
    {attrZIndex,               DISPID_IHTMLSTYLE_ZINDEX}
};

static const WCHAR valLineThrough[] =
    {'l','i','n','e','-','t','h','r','o','u','g','h',0};
static const WCHAR valUnderline[] =
    {'u','n','d','e','r','l','i','n','e',0};
static const WCHAR szNormal[] =
    {'n','o','r','m','a','l',0};

static const WCHAR px_formatW[] = {'%','d','p','x',0};
static const WCHAR emptyW[] = {0};

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

#define ATTR_FIX_PX      1
#define ATTR_FIX_URL     2
#define ATTR_STR_TO_INT  4

HRESULT set_nsstyle_attr(nsIDOMCSSStyleDeclaration *nsstyle, styleid_t sid, LPCWSTR value, DWORD flags)
{
    nsAString str_name, str_value, str_empty;
    LPWSTR val = NULL;
    nsresult nsres;

    static const PRUnichar wszEmpty[] = {0};

    if(flags & ATTR_FIX_PX)
        val = fix_px_value(value);
    if(flags & ATTR_FIX_URL)
        val = fix_url_value(value);

    nsAString_Init(&str_name, style_tbl[sid].name);
    nsAString_Init(&str_value, val ? val : value);
    nsAString_Init(&str_empty, wszEmpty);
    heap_free(val);

    nsres = nsIDOMCSSStyleDeclaration_SetProperty(nsstyle, &str_name, &str_value, &str_empty);
    if(NS_FAILED(nsres))
        ERR("SetProperty failed: %08x\n", nsres);

    nsAString_Finish(&str_name);
    nsAString_Finish(&str_value);
    nsAString_Finish(&str_empty);

    return S_OK;
}

static HRESULT set_nsstyle_attr_var(nsIDOMCSSStyleDeclaration *nsstyle, styleid_t sid, VARIANT *value, DWORD flags)
{
    switch(V_VT(value)) {
    case VT_NULL:
        return set_nsstyle_attr(nsstyle, sid, emptyW, flags);

    case VT_BSTR:
        return set_nsstyle_attr(nsstyle, sid, V_BSTR(value), flags);

    default:
        FIXME("not implemented vt %d\n", V_VT(value));
        return E_NOTIMPL;

    }

    return S_OK;
}

static inline HRESULT set_style_attr(HTMLStyle *This, styleid_t sid, LPCWSTR value, DWORD flags)
{
    return set_nsstyle_attr(This->nsstyle, sid, value, flags);
}

static HRESULT get_nsstyle_attr_nsval(nsIDOMCSSStyleDeclaration *nsstyle, styleid_t sid, nsAString *value)
{
    nsAString str_name;
    nsresult nsres;

    nsAString_Init(&str_name, style_tbl[sid].name);

    nsres = nsIDOMCSSStyleDeclaration_GetPropertyValue(nsstyle, &str_name, value);
    if(NS_FAILED(nsres)) {
        ERR("SetProperty failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsAString_Finish(&str_name);

    return NS_OK;
}

HRESULT get_nsstyle_attr(nsIDOMCSSStyleDeclaration *nsstyle, styleid_t sid, BSTR *p)
{
    nsAString str_value;
    const PRUnichar *value;

    nsAString_Init(&str_value, NULL);

    get_nsstyle_attr_nsval(nsstyle, sid, &str_value);

    nsAString_GetData(&str_value, &value);
    *p = *value ? SysAllocString(value) : NULL;

    nsAString_Finish(&str_value);

    TRACE("%s -> %s\n", debugstr_w(style_tbl[sid].name), debugstr_w(*p));
    return S_OK;
}

static HRESULT get_nsstyle_attr_var(nsIDOMCSSStyleDeclaration *nsstyle, styleid_t sid, VARIANT *p, DWORD flags)
{
    nsAString str_value;
    const PRUnichar *value;
    BOOL set = FALSE;

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
        BSTR str = NULL;

        if(*value) {
            str = SysAllocString(value);
            if(!str)
                return E_OUTOFMEMORY;
        }

        V_VT(p) = VT_BSTR;
        V_BSTR(p) = str;
    }

    nsAString_Finish(&str_value);

    TRACE("%s -> %s\n", debugstr_w(style_tbl[sid].name), debugstr_variant(p));
    return S_OK;
}

static inline HRESULT get_style_attr(HTMLStyle *This, styleid_t sid, BSTR *p)
{
    return get_nsstyle_attr(This->nsstyle, sid, p);
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

static HRESULT get_nsstyle_pos(HTMLStyle *This, styleid_t sid, float *p)
{
    nsAString str_value;
    HRESULT hres;
    WCHAR pxW[] = {'p','x',0};

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
                return E_FAIL;
            }
        }
    }

    TRACE("ret %f\n", *p);

    nsAString_Finish(&str_value);

    return hres;
}

static BOOL is_valid_border_style(BSTR v)
{
    static const WCHAR styleNone[]   = {'n','o','n','e',0};
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

#define HTMLSTYLE_THIS(iface) DEFINE_THIS(HTMLStyle, HTMLStyle, iface)

static HRESULT WINAPI HTMLStyle_QueryInterface(IHTMLStyle *iface, REFIID riid, void **ppv)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLSTYLE(This);
    }else if(IsEqualGUID(&IID_IHTMLStyle, riid)) {
        TRACE("(%p)->(IID_IHTMLStyle %p)\n", This, ppv);
        *ppv = HTMLSTYLE(This);
    }else if(IsEqualGUID(&IID_IHTMLStyle2, riid)) {
        TRACE("(%p)->(IID_IHTMLStyle2 %p)\n", This, ppv);
        *ppv = HTMLSTYLE2(This);
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("unsupported %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI HTMLStyle_AddRef(IHTMLStyle *iface)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLStyle_Release(IHTMLStyle *iface)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->nsstyle)
            nsIDOMCSSStyleDeclaration_Release(This->nsstyle);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLStyle_GetTypeInfoCount(IHTMLStyle *iface, UINT *pctinfo)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(&This->dispex), pctinfo);
}

static HRESULT WINAPI HTMLStyle_GetTypeInfo(IHTMLStyle *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    return IDispatchEx_GetTypeInfo(DISPATCHEX(&This->dispex), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLStyle_GetIDsOfNames(IHTMLStyle *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    return IDispatchEx_GetIDsOfNames(DISPATCHEX(&This->dispex), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLStyle_Invoke(IHTMLStyle *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    return IDispatchEx_Invoke(DISPATCHEX(&This->dispex), dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLStyle_put_fontFamily(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_attr(This, STYLEID_FONT_FAMILY, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_fontFamily(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_FONT_FAMILY, p);
}

static HRESULT WINAPI HTMLStyle_put_fontStyle(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
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
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_FONT_STYLE, p);
}

static HRESULT WINAPI HTMLStyle_put_fontVariant(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
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
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
       return E_INVALIDARG;

    return get_style_attr(This, STYLEID_FONT_VARIANT, p);
}

static HRESULT WINAPI HTMLStyle_put_fontWeight(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_fontWeight(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_FONT_WEIGHT, p);
}

static HRESULT WINAPI HTMLStyle_put_fontSize(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(v%d)\n", This, V_VT(&v));

    switch(V_VT(&v)) {
    case VT_BSTR:
        return set_style_attr(This, STYLEID_FONT_SIZE, V_BSTR(&v), 0);
    default:
        FIXME("not supported vt %d\n", V_VT(&v));
    }

    return S_OK;
}

static HRESULT WINAPI HTMLStyle_get_fontSize(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    V_VT(p) = VT_BSTR;
    return get_style_attr(This, STYLEID_FONT_SIZE, &V_BSTR(p));
}

static HRESULT WINAPI HTMLStyle_put_font(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_font(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_color(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(v%d)\n", This, V_VT(&v));

    switch(V_VT(&v)) {
    case VT_BSTR:
        TRACE("%s\n", debugstr_w(V_BSTR(&v)));
        return set_style_attr(This, STYLEID_COLOR, V_BSTR(&v), 0);

    default:
        FIXME("unsupported vt=%d\n", V_VT(&v));
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_color(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    V_VT(p) = VT_BSTR;
    return get_style_attr(This, STYLEID_COLOR, &V_BSTR(p));
}

static HRESULT WINAPI HTMLStyle_put_background(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_attr(This, STYLEID_BACKGROUND, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_background(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_BACKGROUND, p);
}

static HRESULT WINAPI HTMLStyle_put_backgroundColor(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(v%d)\n", This, V_VT(&v));

    switch(V_VT(&v)) {
    case VT_BSTR:
        return set_style_attr(This, STYLEID_BACKGROUND_COLOR, V_BSTR(&v), 0);
    case VT_I4: {
        WCHAR value[10];
        static const WCHAR format[] = {'#','%','0','6','x',0};

        wsprintfW(value, format, V_I4(&v));
        return set_style_attr(This, STYLEID_BACKGROUND_COLOR, value, 0);
    }
    default:
        FIXME("unsupported vt %d\n", V_VT(&v));
    }

    return S_OK;
}

static HRESULT WINAPI HTMLStyle_get_backgroundColor(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_backgroundImage(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_attr(This, STYLEID_BACKGROUND_IMAGE, v, ATTR_FIX_URL);
}

static HRESULT WINAPI HTMLStyle_get_backgroundImage(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_BACKGROUND_IMAGE, p);
}

static HRESULT WINAPI HTMLStyle_put_backgroundRepeat(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_backgroundRepeat(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_backgroundAttachment(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_backgroundAttachment(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_backgroundPosition(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_backgroundPosition(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_backgroundPositionX(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(v%d)\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_backgroundPositionX(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_backgroundPositionY(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(v%d)\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_backgroundPositionY(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_wordSpacing(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(v%d)\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_wordSpacing(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_letterSpacing(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(v%d)\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_letterSpacing(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_textDecoration(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_textDecoration(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_TEXT_DECORATION, p);
}

static HRESULT WINAPI HTMLStyle_put_textDecorationNone(IHTMLStyle *iface, VARIANT_BOOL v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_textDecorationNone(IHTMLStyle *iface, VARIANT_BOOL *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_textDecorationUnderline(IHTMLStyle *iface, VARIANT_BOOL v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_textDecorationUnderline(IHTMLStyle *iface, VARIANT_BOOL *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return check_style_attr_value(This, STYLEID_TEXT_DECORATION, valUnderline, p);
}

static HRESULT WINAPI HTMLStyle_put_textDecorationOverline(IHTMLStyle *iface, VARIANT_BOOL v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_textDecorationOverline(IHTMLStyle *iface, VARIANT_BOOL *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_textDecorationLineThrough(IHTMLStyle *iface, VARIANT_BOOL v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_textDecorationLineThrough(IHTMLStyle *iface, VARIANT_BOOL *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return check_style_attr_value(This, STYLEID_TEXT_DECORATION, valLineThrough, p);
}

static HRESULT WINAPI HTMLStyle_put_textDecorationBlink(IHTMLStyle *iface, VARIANT_BOOL v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_textDecorationBlink(IHTMLStyle *iface, VARIANT_BOOL *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_verticalAlign(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    switch(V_VT(&v)) {
    case VT_BSTR:
        return set_style_attr(This, STYLEID_VERTICAL_ALIGN, V_BSTR(&v), 0);
    default:
        FIXME("not implemented vt %d\n", V_VT(&v));
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLStyle_get_verticalAlign(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    BSTR ret;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    hres = get_style_attr(This, STYLEID_VERTICAL_ALIGN, &ret);
    if(FAILED(hres))
        return hres;

    V_VT(p) = VT_BSTR;
    V_BSTR(p) = ret;
    return S_OK;
}

static HRESULT WINAPI HTMLStyle_put_textTransform(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_textTransform(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_textAlign(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_attr(This, STYLEID_TEXT_ALIGN, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_textAlign(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_TEXT_ALIGN, p);
}

static HRESULT WINAPI HTMLStyle_put_textIndent(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(v%d)\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_textIndent(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_lineHeight(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(v%d)\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_lineHeight(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_marginTop(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(v%d)\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_marginTop(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_marginRight(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(v(%d))\n", This, V_VT(&v));

    switch(V_VT(&v)) {
    case VT_NULL:
        return set_style_attr(This, STYLEID_MARGIN_RIGHT, emptyW, 0);
    case VT_I4: {
        WCHAR buf[14];

        wsprintfW(buf, px_formatW, V_I4(&v));
        return set_style_attr(This, STYLEID_MARGIN_RIGHT, buf, 0);
    }
    case VT_BSTR:
        return set_style_attr(This, STYLEID_MARGIN_RIGHT, V_BSTR(&v), 0);
    default:
        FIXME("Unsupported vt=%d\n", V_VT(&v));
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_marginRight(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_marginBottom(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(v%d)\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_marginBottom(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_marginLeft(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    switch(V_VT(&v)) {
    case VT_NULL:
        TRACE("(%p)->(NULL)\n", This);
        return set_style_attr(This, STYLEID_MARGIN_LEFT, emptyW, 0);
    case VT_I4: {
        WCHAR buf[14];

        TRACE("(%p)->(%d)\n", This, V_I4(&v));

        wsprintfW(buf, px_formatW, V_I4(&v));
        return set_style_attr(This, STYLEID_MARGIN_LEFT, buf, 0);
    }
    case VT_BSTR:
        TRACE("(%p)->(%s)\n", This, debugstr_w(V_BSTR(&v)));
        return set_style_attr(This, STYLEID_MARGIN_LEFT, V_BSTR(&v), 0);
    default:
        FIXME("Unsupported vt=%d\n", V_VT(&v));
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_margin(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_attr(This, STYLEID_MARGIN, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_margin(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_MARGIN, p);
}

static HRESULT WINAPI HTMLStyle_get_marginLeft(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_paddingTop(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(v%d)\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_paddingTop(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_paddingRight(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(v%d)\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_paddingRight(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_paddingBottom(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(v%d)\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_paddingBottom(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_paddingLeft(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(vt=%d)\n", This, V_VT(&v));

    switch(V_VT(&v)) {
    case VT_I4: {
        WCHAR buf[14];

        wsprintfW(buf, px_formatW, V_I4(&v));
        return set_style_attr(This, STYLEID_PADDING_LEFT, buf, 0);
    }
    case VT_BSTR:
        return set_style_attr(This, STYLEID_PADDING_LEFT, V_BSTR(&v), 0);
    default:
        FIXME("unsupported vt=%d\n", V_VT(&v));
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_paddingLeft(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_padding(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_padding(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_border(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_attr(This, STYLEID_BORDER, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_border(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_BORDER, p);
}

static HRESULT WINAPI HTMLStyle_put_borderTop(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_borderTop(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_borderRight(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_borderRight(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_borderBottom(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_borderBottom(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_borderLeft(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_attr(This, STYLEID_BORDER_LEFT, v, ATTR_FIX_PX);
}

static HRESULT WINAPI HTMLStyle_get_borderLeft(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_borderColor(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_borderColor(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_borderTopColor(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(v%d)\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_borderTopColor(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_borderRightColor(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(v%d)\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_borderRightColor(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_borderBottomColor(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(v%d)\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_borderBottomColor(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_borderLeftColor(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(v%d)\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_borderLeftColor(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_borderWidth(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));
    return set_style_attr(This, STYLEID_BORDER_WIDTH, v, ATTR_FIX_PX);
}

static HRESULT WINAPI HTMLStyle_get_borderWidth(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_attr(This, STYLEID_BORDER_WIDTH, p);
}

static HRESULT WINAPI HTMLStyle_put_borderTopWidth(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(v%d)\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_borderTopWidth(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_borderRightWidth(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(v%d)\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_borderRightWidth(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_borderBottomWidth(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(v%d)\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_borderBottomWidth(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_borderLeftWidth(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(v%d)\n", This, V_VT(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_borderLeftWidth(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_borderStyle(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_borderStyle(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_borderTopStyle(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!is_valid_border_style(v))
        return E_INVALIDARG;

    return set_style_attr(This, STYLEID_BORDER_TOP_STYLE, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_borderTopStyle(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_attr(This, STYLEID_BORDER_TOP_STYLE, p);
}

static HRESULT WINAPI HTMLStyle_put_borderRightStyle(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!is_valid_border_style(v))
        return E_INVALIDARG;

    return set_style_attr(This, STYLEID_BORDER_RIGHT_STYLE, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_borderRightStyle(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_attr(This, STYLEID_BORDER_RIGHT_STYLE, p);
}

static HRESULT WINAPI HTMLStyle_put_borderBottomStyle(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!is_valid_border_style(v))
        return E_INVALIDARG;

    return set_style_attr(This, STYLEID_BORDER_BOTTOM_STYLE, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_borderBottomStyle(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_attr(This, STYLEID_BORDER_BOTTOM_STYLE, p);
}

static HRESULT WINAPI HTMLStyle_put_borderLeftStyle(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!is_valid_border_style(v))
        return E_INVALIDARG;

    return set_style_attr(This, STYLEID_BORDER_LEFT_STYLE, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_borderLeftStyle(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_style_attr(This, STYLEID_BORDER_LEFT_STYLE, p);
}

static HRESULT WINAPI HTMLStyle_put_width(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(v%d)\n", This, V_VT(&v));

    switch(V_VT(&v)) {
    case VT_BSTR:
        TRACE("%s\n", debugstr_w(V_BSTR(&v)));
        return set_style_attr(This, STYLEID_WIDTH, V_BSTR(&v), 0);
    default:
        FIXME("unsupported vt %d\n", V_VT(&v));
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_width(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    V_VT(p) = VT_BSTR;
    return get_style_attr(This, STYLEID_WIDTH, &V_BSTR(p));
}

static HRESULT WINAPI HTMLStyle_put_height(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    switch(V_VT(&v)) {
    case VT_BSTR:
        return set_style_attr(This, STYLEID_HEIGHT, V_BSTR(&v), 0);
    default:
        FIXME("unimplemented vt %d\n", V_VT(&v));
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLStyle_get_height(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    BSTR ret;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    hres = get_style_attr(This, STYLEID_HEIGHT, &ret);
    if(FAILED(hres))
        return hres;

    V_VT(p) = VT_BSTR;
    V_BSTR(p) = ret;
    return S_OK;
}

static HRESULT WINAPI HTMLStyle_put_styleFloat(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_styleFloat(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_clear(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_clear(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_display(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_attr(This, STYLEID_DISPLAY, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_display(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_DISPLAY, p);
}

static HRESULT WINAPI HTMLStyle_put_visibility(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_attr(This, STYLEID_VISIBILITY, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_visibility(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_VISIBILITY, p);
}

static HRESULT WINAPI HTMLStyle_put_listStyleType(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_listStyleType(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_listStylePosition(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_listStylePosition(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_listStyleImage(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_listStyleImage(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_listStyle(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_listStyle(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_whiteSpace(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_whiteSpace(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_top(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_TOP, &v, 0);
}

static HRESULT WINAPI HTMLStyle_get_top(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    BSTR ret;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    hres = get_style_attr(This, STYLEID_TOP, &ret);
    if(FAILED(hres))
        return hres;

    V_VT(p) = VT_BSTR;
    V_BSTR(p) = ret;
    return S_OK;
}

static HRESULT WINAPI HTMLStyle_put_left(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_LEFT, &v, 0);
}

static HRESULT WINAPI HTMLStyle_get_left(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    BSTR ret;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    hres = get_style_attr(This, STYLEID_LEFT, &ret);
    if(FAILED(hres))
        return hres;

    V_VT(p) = VT_BSTR;
    V_BSTR(p) = ret;
    return S_OK;
}

static HRESULT WINAPI HTMLStyle_get_position(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return IHTMLStyle2_get_position(HTMLSTYLE2(This), p);
}

static HRESULT WINAPI HTMLStyle_put_zIndex(IHTMLStyle *iface, VARIANT v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    switch(V_VT(&v)) {
    case VT_BSTR:
        return set_style_attr(This, STYLEID_Z_INDEX, V_BSTR(&v), 0);
    case VT_I4: {
        WCHAR value[14];
        static const WCHAR format[] = {'%','d',0};

        wsprintfW(value, format, V_I4(&v));
        return set_style_attr(This, STYLEID_Z_INDEX, value, 0);
    }
    default:
        FIXME("unimplemented vt %d\n", V_VT(&v));
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLStyle_get_zIndex(IHTMLStyle *iface, VARIANT *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr_var(This->nsstyle, STYLEID_Z_INDEX, p, ATTR_STR_TO_INT);
}

static HRESULT WINAPI HTMLStyle_put_overflow(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    static const WCHAR szVisible[] = {'v','i','s','i','b','l','e',0};
    static const WCHAR szScroll[]  = {'s','c','r','o','l','l',0};
    static const WCHAR szHidden[]  = {'h','i','d','d','e','n',0};
    static const WCHAR szAuto[]    = {'a','u','t','o',0};

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    /* overflow can only be one of the follow values. */
    if(!v || strcmpiW(szVisible, v) == 0 || strcmpiW(szScroll, v) == 0 ||
             strcmpiW(szHidden, v) == 0  || strcmpiW(szAuto, v) == 0)
    {
        return set_nsstyle_attr(This->nsstyle, STYLEID_OVERFLOW, v, 0);
    }

    return E_INVALIDARG;
}


static HRESULT WINAPI HTMLStyle_get_overflow(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
       return E_INVALIDARG;

    return get_style_attr(This, STYLEID_OVERFLOW, p);
}

static HRESULT WINAPI HTMLStyle_put_pageBreakBefore(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_pageBreakBefore(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_pageBreakAfter(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_pageBreakAfter(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_cssText(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_cssText(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_pixelTop(IHTMLStyle *iface, long v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_pixelTop(IHTMLStyle *iface, long *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_pixelLeft(IHTMLStyle *iface, long v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_pixelLeft(IHTMLStyle *iface, long *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_pixelWidth(IHTMLStyle *iface, long v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_pixelWidth(IHTMLStyle *iface, long *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_pixelHeight(IHTMLStyle *iface, long v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_pixelHeight(IHTMLStyle *iface, long *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_posTop(IHTMLStyle *iface, float v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%f)\n", This, v);

    return set_style_pos(This, STYLEID_TOP, v);
}

static HRESULT WINAPI HTMLStyle_get_posTop(IHTMLStyle *iface, float *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    return get_nsstyle_pos(This, STYLEID_TOP, p);
}

static HRESULT WINAPI HTMLStyle_put_posLeft(IHTMLStyle *iface, float v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%f)\n", This, v);

    return set_style_pos(This, STYLEID_LEFT, v);
}

static HRESULT WINAPI HTMLStyle_get_posLeft(IHTMLStyle *iface, float *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    return get_nsstyle_pos(This, STYLEID_LEFT, p);
}

static HRESULT WINAPI HTMLStyle_put_posWidth(IHTMLStyle *iface, float v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%f)\n", This, v);

    return set_style_pos(This, STYLEID_WIDTH, v);
}

static HRESULT WINAPI HTMLStyle_get_posWidth(IHTMLStyle *iface, float *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    if(get_nsstyle_pos(This, STYLEID_WIDTH, p) != S_OK)
        *p = 0.0f;

    return S_OK;
}

static HRESULT WINAPI HTMLStyle_put_posHeight(IHTMLStyle *iface, float v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%f)\n", This, v);

    return set_style_pos(This, STYLEID_HEIGHT, v);
}

static HRESULT WINAPI HTMLStyle_get_posHeight(IHTMLStyle *iface, float *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_POINTER;

    if(get_nsstyle_pos(This, STYLEID_HEIGHT, p) != S_OK)
        *p = 0.0f;

    return S_OK;
}

static HRESULT WINAPI HTMLStyle_put_cursor(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_style_attr(This, STYLEID_CURSOR, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_cursor(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_style_attr(This, STYLEID_CURSOR, p);
}

static HRESULT WINAPI HTMLStyle_put_clip(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_get_clip(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_put_filter(IHTMLStyle *iface, BSTR v)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    WARN("(%p)->(%s)\n", This, debugstr_w(v));

    /* FIXME: Handle MS-style filters */
    return set_style_attr(This, STYLEID_FILTER, v, 0);
}

static HRESULT WINAPI HTMLStyle_get_filter(IHTMLStyle *iface, BSTR *p)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);

    WARN("(%p)->(%p)\n", This, p);

    /* FIXME: Handle MS-style filters */
    return get_style_attr(This, STYLEID_FILTER, p);
}

static HRESULT WINAPI HTMLStyle_setAttribute(IHTMLStyle *iface, BSTR strAttributeName,
        VARIANT AttributeValue, LONG lFlags)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    HRESULT hres;
    DISPID dispid;

    TRACE("(%p)->(%s v%d %08x)\n", This, debugstr_w(strAttributeName),
           V_VT(&AttributeValue), lFlags);

    if(!strAttributeName)
        return E_INVALIDARG;

    if(lFlags == 1)
        FIXME("Parameter lFlags ignored\n");

    hres = HTMLStyle_GetIDsOfNames(iface, &IID_NULL, (LPOLESTR*)&strAttributeName, 1,
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
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    HRESULT hres;
    DISPID dispid;

    TRACE("(%p)->(%s v%p %08x)\n", This, debugstr_w(strAttributeName),
          AttributeValue, lFlags);

    if(!AttributeValue || !strAttributeName)
        return E_INVALIDARG;

    if(lFlags == 1)
        FIXME("Parameter lFlags ignored\n");

    hres = HTMLStyle_GetIDsOfNames(iface, &IID_NULL, (LPOLESTR*)&strAttributeName, 1,
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
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%s %08x %p)\n", This, debugstr_w(strAttributeName),
         lFlags, pfSuccess);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle_toString(IHTMLStyle *iface, BSTR *String)
{
    HTMLStyle *This = HTMLSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, String);
    return E_NOTIMPL;
}

static HRESULT HTMLStyle_get_dispid(IUnknown *iface, BSTR name, DWORD flags, DISPID *dispid)
{
    int c, i, min=0, max = sizeof(style_tbl)/sizeof(*style_tbl)-1;

    while(min <= max) {
        i = (min+max)/2;

        c = strcmpW(style_tbl[i].name, name);
        if(!c) {
            *dispid = style_tbl[i].dispid;
            return S_OK;
        }

        if(c > 0)
            max = i-1;
        else
            min = i+1;
    }

    return DISP_E_UNKNOWNNAME;
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

static const dispex_static_data_vtbl_t HTMLStyle_dispex_vtbl = {
    HTMLStyle_get_dispid,
    NULL
};

static const tid_t HTMLStyle_iface_tids[] = {
    IHTMLStyle_tid,
    IHTMLStyle2_tid,
    0
};
static dispex_static_data_t HTMLStyle_dispex = {
    &HTMLStyle_dispex_vtbl,
    DispHTMLStyle_tid,
    NULL,
    HTMLStyle_iface_tids
};

IHTMLStyle *HTMLStyle_Create(nsIDOMCSSStyleDeclaration *nsstyle)
{
    HTMLStyle *ret = heap_alloc_zero(sizeof(HTMLStyle));

    ret->lpHTMLStyleVtbl = &HTMLStyleVtbl;
    ret->ref = 1;
    ret->nsstyle = nsstyle;
    HTMLStyle2_Init(ret);

    nsIDOMCSSStyleDeclaration_AddRef(nsstyle);

    init_dispex(&ret->dispex, (IUnknown*)HTMLSTYLE(ret),  &HTMLStyle_dispex);

    return HTMLSTYLE(ret);
}
