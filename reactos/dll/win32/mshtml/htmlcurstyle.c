/*
 * Copyright 2008 Jacek Caban for CodeWeavers
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
#include "winuser.h"
#include "ole2.h"

#include "mshtml_private.h"
#include "htmlstyle.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

struct HTMLCurrentStyle {
    DispatchEx dispex;
    const IHTMLCurrentStyleVtbl *lpIHTMLCurrentStyleVtbl;

    LONG ref;

    nsIDOMCSSStyleDeclaration *nsstyle;
};

#define HTMLCURSTYLE(x)  ((IHTMLCurrentStyle*)  &(x)->lpIHTMLCurrentStyleVtbl)

#define HTMLCURSTYLE_THIS(iface) DEFINE_THIS(HTMLCurrentStyle, IHTMLCurrentStyle, iface)

static HRESULT WINAPI HTMLCurrentStyle_QueryInterface(IHTMLCurrentStyle *iface, REFIID riid, void **ppv)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLCURSTYLE(This);
    }else if(IsEqualGUID(&IID_IHTMLCurrentStyle, riid)) {
        TRACE("(%p)->(IID_IHTMLCurrentStyle %p)\n", This, ppv);
        *ppv = HTMLCURSTYLE(This);
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

static ULONG WINAPI HTMLCurrentStyle_AddRef(IHTMLCurrentStyle *iface)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLCurrentStyle_Release(IHTMLCurrentStyle *iface)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->nsstyle)
            nsIDOMCSSStyleDeclaration_Release(This->nsstyle);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLCurrentStyle_GetTypeInfoCount(IHTMLCurrentStyle *iface, UINT *pctinfo)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(&This->dispex), pctinfo);
}

static HRESULT WINAPI HTMLCurrentStyle_GetTypeInfo(IHTMLCurrentStyle *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    return IDispatchEx_GetTypeInfo(DISPATCHEX(&This->dispex), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLCurrentStyle_GetIDsOfNames(IHTMLCurrentStyle *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    return IDispatchEx_GetIDsOfNames(DISPATCHEX(&This->dispex), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLCurrentStyle_Invoke(IHTMLCurrentStyle *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    return IDispatchEx_Invoke(DISPATCHEX(&This->dispex), dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLCurrentStyle_get_position(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr(This->nsstyle, STYLEID_POSITION, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_styleFloat(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_color(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_backgroundColor(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_fontFamily(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr(This->nsstyle, STYLEID_FONT_FAMILY, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_fontStyle(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_FONT_STYLE, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_fontVariant(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_FONT_VARIANT, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_fontWeight(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_FONT_WEIGHT, p, ATTR_STR_TO_INT);
}

static HRESULT WINAPI HTMLCurrentStyle_get_fontSize(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_FONT_SIZE, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_backgroundImage(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_BACKGROUND_IMAGE, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_backgroundPositionX(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_backgroundPositionY(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_backgroundRepeat(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_BACKGROUND_REPEAT, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderLeftColor(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderTopColor(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderRightColor(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderBottomColor(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderTopStyle(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_BORDER_TOP_STYLE, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderRightStyle(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_BORDER_RIGHT_STYLE, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderBottomStyle(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_BORDER_BOTTOM_STYLE, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderLeftStyle(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_BORDER_LEFT_STYLE, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderTopWidth(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderRightWidth(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderBottomWidth(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderLeftWidth(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_left(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_LEFT, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_top(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_TOP, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_width(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_WIDTH, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_height(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_HEIGHT, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_paddingLeft(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_PADDING_LEFT, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_paddingTop(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_paddingRight(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_paddingBottom(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_textAlign(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_TEXT_ALIGN, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_textDecoration(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_TEXT_DECORATION, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_display(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr(This->nsstyle, STYLEID_DISPLAY, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_visibility(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_zIndex(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_Z_INDEX, p, ATTR_STR_TO_INT);
}

static HRESULT WINAPI HTMLCurrentStyle_get_letterSpacing(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_lineHeight(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_textIndent(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_verticalAlign(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_VERTICAL_ALIGN, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_backgroundAttachment(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_marginTop(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_marginRight(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_MARGIN_RIGHT, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_marginBottom(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_marginLeft(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_MARGIN_LEFT, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_clear(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_listStyleType(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_listStylePosition(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_listStyleImage(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_clipTop(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_clipRight(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_clipBottom(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_clipLeft(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_overflow(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_pageBreakBefore(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_pageBreakAfter(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_cursor(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_CURSOR, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_tableLayout(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderCollapse(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_direction(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_behavior(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_getAttribute(IHTMLCurrentStyle *iface, BSTR strAttributeName,
        LONG lFlags, VARIANT *AttributeValue)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%s %x %p)\n", This, debugstr_w(strAttributeName), lFlags, AttributeValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_unicodeBidi(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_right(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_bottom(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_imeMode(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_rubyAlign(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_rubyPosition(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_rubyOverhang(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_textAutospace(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_lineBreak(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_wordBreak(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_textJustify(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_textJustifyTrim(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_textKashida(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_blockDirection(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_layoutGridChar(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_layoutGridLine(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_layoutGridMode(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_layoutGridType(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderStyle(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_BORDER_STYLE, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderColor(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_BORDER_COLOR, p);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderWidth(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_padding(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_margin(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_accelerator(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_overflowX(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_overflowY(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_textTransform(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = HTMLCURSTYLE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

#undef HTMLCURSTYLE_THIS

static const IHTMLCurrentStyleVtbl HTMLCurrentStyleVtbl = {
    HTMLCurrentStyle_QueryInterface,
    HTMLCurrentStyle_AddRef,
    HTMLCurrentStyle_Release,
    HTMLCurrentStyle_GetTypeInfoCount,
    HTMLCurrentStyle_GetTypeInfo,
    HTMLCurrentStyle_GetIDsOfNames,
    HTMLCurrentStyle_Invoke,
    HTMLCurrentStyle_get_position,
    HTMLCurrentStyle_get_styleFloat,
    HTMLCurrentStyle_get_color,
    HTMLCurrentStyle_get_backgroundColor,
    HTMLCurrentStyle_get_fontFamily,
    HTMLCurrentStyle_get_fontStyle,
    HTMLCurrentStyle_get_fontVariant,
    HTMLCurrentStyle_get_fontWeight,
    HTMLCurrentStyle_get_fontSize,
    HTMLCurrentStyle_get_backgroundImage,
    HTMLCurrentStyle_get_backgroundPositionX,
    HTMLCurrentStyle_get_backgroundPositionY,
    HTMLCurrentStyle_get_backgroundRepeat,
    HTMLCurrentStyle_get_borderLeftColor,
    HTMLCurrentStyle_get_borderTopColor,
    HTMLCurrentStyle_get_borderRightColor,
    HTMLCurrentStyle_get_borderBottomColor,
    HTMLCurrentStyle_get_borderTopStyle,
    HTMLCurrentStyle_get_borderRightStyle,
    HTMLCurrentStyle_get_borderBottomStyle,
    HTMLCurrentStyle_get_borderLeftStyle,
    HTMLCurrentStyle_get_borderTopWidth,
    HTMLCurrentStyle_get_borderRightWidth,
    HTMLCurrentStyle_get_borderBottomWidth,
    HTMLCurrentStyle_get_borderLeftWidth,
    HTMLCurrentStyle_get_left,
    HTMLCurrentStyle_get_top,
    HTMLCurrentStyle_get_width,
    HTMLCurrentStyle_get_height,
    HTMLCurrentStyle_get_paddingLeft,
    HTMLCurrentStyle_get_paddingTop,
    HTMLCurrentStyle_get_paddingRight,
    HTMLCurrentStyle_get_paddingBottom,
    HTMLCurrentStyle_get_textAlign,
    HTMLCurrentStyle_get_textDecoration,
    HTMLCurrentStyle_get_display,
    HTMLCurrentStyle_get_visibility,
    HTMLCurrentStyle_get_zIndex,
    HTMLCurrentStyle_get_letterSpacing,
    HTMLCurrentStyle_get_lineHeight,
    HTMLCurrentStyle_get_textIndent,
    HTMLCurrentStyle_get_verticalAlign,
    HTMLCurrentStyle_get_backgroundAttachment,
    HTMLCurrentStyle_get_marginTop,
    HTMLCurrentStyle_get_marginRight,
    HTMLCurrentStyle_get_marginBottom,
    HTMLCurrentStyle_get_marginLeft,
    HTMLCurrentStyle_get_clear,
    HTMLCurrentStyle_get_listStyleType,
    HTMLCurrentStyle_get_listStylePosition,
    HTMLCurrentStyle_get_listStyleImage,
    HTMLCurrentStyle_get_clipTop,
    HTMLCurrentStyle_get_clipRight,
    HTMLCurrentStyle_get_clipBottom,
    HTMLCurrentStyle_get_clipLeft,
    HTMLCurrentStyle_get_overflow,
    HTMLCurrentStyle_get_pageBreakBefore,
    HTMLCurrentStyle_get_pageBreakAfter,
    HTMLCurrentStyle_get_cursor,
    HTMLCurrentStyle_get_tableLayout,
    HTMLCurrentStyle_get_borderCollapse,
    HTMLCurrentStyle_get_direction,
    HTMLCurrentStyle_get_behavior,
    HTMLCurrentStyle_getAttribute,
    HTMLCurrentStyle_get_unicodeBidi,
    HTMLCurrentStyle_get_right,
    HTMLCurrentStyle_get_bottom,
    HTMLCurrentStyle_get_imeMode,
    HTMLCurrentStyle_get_rubyAlign,
    HTMLCurrentStyle_get_rubyPosition,
    HTMLCurrentStyle_get_rubyOverhang,
    HTMLCurrentStyle_get_textAutospace,
    HTMLCurrentStyle_get_lineBreak,
    HTMLCurrentStyle_get_wordBreak,
    HTMLCurrentStyle_get_textJustify,
    HTMLCurrentStyle_get_textJustifyTrim,
    HTMLCurrentStyle_get_textKashida,
    HTMLCurrentStyle_get_blockDirection,
    HTMLCurrentStyle_get_layoutGridChar,
    HTMLCurrentStyle_get_layoutGridLine,
    HTMLCurrentStyle_get_layoutGridMode,
    HTMLCurrentStyle_get_layoutGridType,
    HTMLCurrentStyle_get_borderStyle,
    HTMLCurrentStyle_get_borderColor,
    HTMLCurrentStyle_get_borderWidth,
    HTMLCurrentStyle_get_padding,
    HTMLCurrentStyle_get_margin,
    HTMLCurrentStyle_get_accelerator,
    HTMLCurrentStyle_get_overflowX,
    HTMLCurrentStyle_get_overflowY,
    HTMLCurrentStyle_get_textTransform
};

static const tid_t HTMLCurrentStyle_iface_tids[] = {
    IHTMLCurrentStyle_tid,
    IHTMLCurrentStyle2_tid,
    IHTMLCurrentStyle3_tid,
    IHTMLCurrentStyle4_tid,
    0
};
static dispex_static_data_t HTMLCurrentStyle_dispex = {
    NULL,
    DispHTMLCurrentStyle_tid,
    NULL,
    HTMLCurrentStyle_iface_tids
};

HRESULT HTMLCurrentStyle_Create(HTMLElement *elem, IHTMLCurrentStyle **p)
{
    nsIDOMCSSStyleDeclaration *nsstyle;
    nsIDOMDocumentView *nsdocview;
    nsIDOMAbstractView *nsview;
    nsIDOMViewCSS *nsviewcss;
    nsAString nsempty_str;
    HTMLCurrentStyle *ret;
    nsresult nsres;

    if(!elem->node.doc->nsdoc)  {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMHTMLDocument_QueryInterface(elem->node.doc->nsdoc, &IID_nsIDOMDocumentView, (void**)&nsdocview);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMDocumentView: %08x\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMDocumentView_GetDefaultView(nsdocview, &nsview);
    nsIDOMDocumentView_Release(nsdocview);
    if(NS_FAILED(nsres)) {
        ERR("GetDefaultView failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMAbstractView_QueryInterface(nsview, &IID_nsIDOMViewCSS, (void**)&nsviewcss);
    nsIDOMAbstractView_Release(nsview);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMViewCSS: %08x\n", nsres);
        return E_FAIL;
    }

    nsAString_Init(&nsempty_str, NULL);
    nsres = nsIDOMViewCSS_GetComputedStyle(nsviewcss, (nsIDOMElement*)elem->nselem, &nsempty_str, &nsstyle);
    nsIDOMViewCSS_Release(nsviewcss);
    nsAString_Finish(&nsempty_str);
    if(NS_FAILED(nsres)) {
        ERR("GetComputedStyle failed: %08x\n", nsres);
        return E_FAIL;
    }

    ret = heap_alloc_zero(sizeof(HTMLCurrentStyle));
    if(!ret) {
        nsIDOMCSSStyleDeclaration_Release(nsstyle);
        return E_OUTOFMEMORY;
    }

    ret->lpIHTMLCurrentStyleVtbl = &HTMLCurrentStyleVtbl;
    ret->ref = 1;
    ret->nsstyle = nsstyle;

    init_dispex(&ret->dispex, (IUnknown*)HTMLCURSTYLE(ret),  &HTMLCurrentStyle_dispex);

    *p = HTMLCURSTYLE(ret);
    return S_OK;
}
