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

#include "mshtml_private.h"

struct HTMLCurrentStyle {
    DispatchEx dispex;
    IHTMLCurrentStyle  IHTMLCurrentStyle_iface;
    IHTMLCurrentStyle2 IHTMLCurrentStyle2_iface;
    IHTMLCurrentStyle3 IHTMLCurrentStyle3_iface;
    IHTMLCurrentStyle4 IHTMLCurrentStyle4_iface;

    LONG ref;

    nsIDOMCSSStyleDeclaration *nsstyle;
    HTMLElement *elem;
};

static inline HTMLCurrentStyle *impl_from_IHTMLCurrentStyle(IHTMLCurrentStyle *iface)
{
    return CONTAINING_RECORD(iface, HTMLCurrentStyle, IHTMLCurrentStyle_iface);
}

static inline HTMLCurrentStyle *impl_from_IHTMLCurrentStyle2(IHTMLCurrentStyle2 *iface)
{
    return CONTAINING_RECORD(iface, HTMLCurrentStyle, IHTMLCurrentStyle2_iface);
}

static inline HTMLCurrentStyle *impl_from_IHTMLCurrentStyle3(IHTMLCurrentStyle3 *iface)
{
    return CONTAINING_RECORD(iface, HTMLCurrentStyle, IHTMLCurrentStyle3_iface);
}

static inline HTMLCurrentStyle *impl_from_IHTMLCurrentStyle4(IHTMLCurrentStyle4 *iface)
{
    return CONTAINING_RECORD(iface, HTMLCurrentStyle, IHTMLCurrentStyle4_iface);
}

static HRESULT WINAPI HTMLCurrentStyle_QueryInterface(IHTMLCurrentStyle *iface, REFIID riid, void **ppv)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IHTMLCurrentStyle_iface;
    }else if(IsEqualGUID(&IID_IHTMLCurrentStyle, riid)) {
        *ppv = &This->IHTMLCurrentStyle_iface;
    }else if(IsEqualGUID(&IID_IHTMLCurrentStyle2, riid)) {
        *ppv = &This->IHTMLCurrentStyle2_iface;
    }else if(IsEqualGUID(&IID_IHTMLCurrentStyle3, riid)) {
        *ppv = &This->IHTMLCurrentStyle3_iface;
    }else if(IsEqualGUID(&IID_IHTMLCurrentStyle4, riid)) {
        *ppv = &This->IHTMLCurrentStyle4_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        *ppv = NULL;
        WARN("unsupported %s\n", debugstr_mshtml_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLCurrentStyle_AddRef(IHTMLCurrentStyle *iface)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLCurrentStyle_Release(IHTMLCurrentStyle *iface)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        if(This->nsstyle)
            nsIDOMCSSStyleDeclaration_Release(This->nsstyle);
        IHTMLElement_Release(&This->elem->IHTMLElement_iface);
        release_dispex(&This->dispex);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLCurrentStyle_GetTypeInfoCount(IHTMLCurrentStyle *iface, UINT *pctinfo)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLCurrentStyle_GetTypeInfo(IHTMLCurrentStyle *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLCurrentStyle_GetIDsOfNames(IHTMLCurrentStyle *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLCurrentStyle_Invoke(IHTMLCurrentStyle *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLCurrentStyle_get_position(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr(This->nsstyle, STYLEID_POSITION, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_styleFloat(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_color(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_COLOR, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_backgroundColor(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_BACKGROUND_COLOR, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_fontFamily(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr(This->nsstyle, STYLEID_FONT_FAMILY, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_fontStyle(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_FONT_STYLE, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_fontVariant(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_FONT_VARIANT, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_fontWeight(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_FONT_WEIGHT, p, ATTR_STR_TO_INT);
}

static HRESULT WINAPI HTMLCurrentStyle_get_fontSize(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_FONT_SIZE, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_backgroundImage(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_BACKGROUND_IMAGE, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_backgroundPositionX(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_backgroundPositionY(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_backgroundRepeat(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_BACKGROUND_REPEAT, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderLeftColor(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_BORDER_LEFT_COLOR, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderTopColor(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_BORDER_TOP_COLOR, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderRightColor(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_BORDER_RIGHT_COLOR, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderBottomColor(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_BORDER_BOTTOM_COLOR, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderTopStyle(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_BORDER_TOP_STYLE, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderRightStyle(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_BORDER_RIGHT_STYLE, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderBottomStyle(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_BORDER_BOTTOM_STYLE, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderLeftStyle(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_BORDER_LEFT_STYLE, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderTopWidth(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_BORDER_TOP_WIDTH, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderRightWidth(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_BORDER_RIGHT_WIDTH, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderBottomWidth(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_BORDER_BOTTOM_WIDTH, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderLeftWidth(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_BORDER_LEFT_WIDTH, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_left(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_LEFT, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_top(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_TOP, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_width(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_WIDTH, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_height(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_HEIGHT, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_paddingLeft(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_PADDING_LEFT, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_paddingTop(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_PADDING_TOP, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_paddingRight(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_PADDING_RIGHT, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_paddingBottom(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_PADDING_BOTTOM, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_textAlign(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_TEXT_ALIGN, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_textDecoration(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_TEXT_DECORATION, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_display(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr(This->nsstyle, STYLEID_DISPLAY, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_visibility(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr(This->nsstyle, STYLEID_VISIBILITY, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_zIndex(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_Z_INDEX, p, ATTR_STR_TO_INT);
}

static HRESULT WINAPI HTMLCurrentStyle_get_letterSpacing(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_LETTER_SPACING, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_lineHeight(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_LINE_HEIGHT, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_textIndent(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_TEXT_INDENT, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_verticalAlign(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_VERTICAL_ALIGN, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_backgroundAttachment(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_marginTop(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_MARGIN_TOP, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_marginRight(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_MARGIN_RIGHT, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_marginBottom(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_MARGIN_BOTTOM, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_marginLeft(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_MARGIN_LEFT, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_clear(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_listStyleType(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_listStylePosition(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_listStyleImage(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_clipTop(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_clipRight(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_clipBottom(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_clipLeft(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_overflow(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_OVERFLOW, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_pageBreakBefore(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_pageBreakAfter(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_cursor(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_CURSOR, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_tableLayout(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderCollapse(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_direction(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_behavior(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_getAttribute(IHTMLCurrentStyle *iface, BSTR strAttributeName,
        LONG lFlags, VARIANT *AttributeValue)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%s %x %p)\n", This, debugstr_w(strAttributeName), lFlags, AttributeValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_unicodeBidi(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_right(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_RIGHT, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_bottom(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr_var(This->nsstyle, STYLEID_BOTTOM, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_imeMode(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_rubyAlign(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_rubyPosition(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_rubyOverhang(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_textAutospace(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_lineBreak(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_wordBreak(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_textJustify(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_textJustifyTrim(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_textKashida(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_blockDirection(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_layoutGridChar(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_layoutGridLine(IHTMLCurrentStyle *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_layoutGridMode(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_layoutGridType(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderStyle(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_BORDER_STYLE, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderColor(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_BORDER_COLOR, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_borderWidth(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_BORDER_WIDTH, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_padding(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_PADDING, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_margin(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_MARGIN, p, 0);
}

static HRESULT WINAPI HTMLCurrentStyle_get_accelerator(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_overflowX(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_overflowY(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle_get_textTransform(IHTMLCurrentStyle *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return get_nsstyle_attr(This->nsstyle, STYLEID_TEXT_TRANSFORM, p, 0);
}

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

/* IHTMLCurrentStyle2 */
static HRESULT WINAPI HTMLCurrentStyle2_QueryInterface(IHTMLCurrentStyle2 *iface, REFIID riid, void **ppv)
{
   HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);

   return IHTMLCurrentStyle_QueryInterface(&This->IHTMLCurrentStyle_iface, riid, ppv);
}

static ULONG WINAPI HTMLCurrentStyle2_AddRef(IHTMLCurrentStyle2 *iface)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);

    return IHTMLCurrentStyle_AddRef(&This->IHTMLCurrentStyle_iface);
}

static ULONG WINAPI HTMLCurrentStyle2_Release(IHTMLCurrentStyle2 *iface)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    return IHTMLCurrentStyle_Release(&This->IHTMLCurrentStyle_iface);
}

static HRESULT WINAPI HTMLCurrentStyle2_GetTypeInfoCount(IHTMLCurrentStyle2 *iface, UINT *pctinfo)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLCurrentStyle2_GetTypeInfo(IHTMLCurrentStyle2 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLCurrentStyle2_GetIDsOfNames(IHTMLCurrentStyle2 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLCurrentStyle2_Invoke(IHTMLCurrentStyle2 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLCurrentStyle2_get_layoutFlow(IHTMLCurrentStyle2 *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_wordWrap(IHTMLCurrentStyle2 *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_textUnderlinePosition(IHTMLCurrentStyle2 *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_hasLayout(IHTMLCurrentStyle2 *iface, VARIANT_BOOL *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);

    FIXME("(%p)->(%p) returning true\n", This, p);

    *p = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_scrollbarBaseColor(IHTMLCurrentStyle2 *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_scrollbarFaceColor(IHTMLCurrentStyle2 *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_scrollbar3dLightColor(IHTMLCurrentStyle2 *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_scrollbarShadowColor(IHTMLCurrentStyle2 *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_scrollbarHighlightColor(IHTMLCurrentStyle2 *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_scrollbarDarkShadowColor(IHTMLCurrentStyle2 *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_scrollbarArrowColor(IHTMLCurrentStyle2 *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_scrollbarTrackColor(IHTMLCurrentStyle2 *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_writingMode(IHTMLCurrentStyle2 *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_zoom(IHTMLCurrentStyle2 *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_filter(IHTMLCurrentStyle2 *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->elem->filter) {
        *p = SysAllocString(This->elem->filter);
        if(!*p)
            return E_OUTOFMEMORY;
    }else {
        *p = NULL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_textAlignLast(IHTMLCurrentStyle2 *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_textKashidaSpace(IHTMLCurrentStyle2 *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle2_get_isBlock(IHTMLCurrentStyle2 *iface, VARIANT_BOOL *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLCurrentStyle2Vtbl HTMLCurrentStyle2Vtbl = {
    HTMLCurrentStyle2_QueryInterface,
    HTMLCurrentStyle2_AddRef,
    HTMLCurrentStyle2_Release,
    HTMLCurrentStyle2_GetTypeInfoCount,
    HTMLCurrentStyle2_GetTypeInfo,
    HTMLCurrentStyle2_GetIDsOfNames,
    HTMLCurrentStyle2_Invoke,
    HTMLCurrentStyle2_get_layoutFlow,
    HTMLCurrentStyle2_get_wordWrap,
    HTMLCurrentStyle2_get_textUnderlinePosition,
    HTMLCurrentStyle2_get_hasLayout,
    HTMLCurrentStyle2_get_scrollbarBaseColor,
    HTMLCurrentStyle2_get_scrollbarFaceColor,
    HTMLCurrentStyle2_get_scrollbar3dLightColor,
    HTMLCurrentStyle2_get_scrollbarShadowColor,
    HTMLCurrentStyle2_get_scrollbarHighlightColor,
    HTMLCurrentStyle2_get_scrollbarDarkShadowColor,
    HTMLCurrentStyle2_get_scrollbarArrowColor,
    HTMLCurrentStyle2_get_scrollbarTrackColor,
    HTMLCurrentStyle2_get_writingMode,
    HTMLCurrentStyle2_get_zoom,
    HTMLCurrentStyle2_get_filter,
    HTMLCurrentStyle2_get_textAlignLast,
    HTMLCurrentStyle2_get_textKashidaSpace,
    HTMLCurrentStyle2_get_isBlock
};

/* IHTMLCurrentStyle3 */
static HRESULT WINAPI HTMLCurrentStyle3_QueryInterface(IHTMLCurrentStyle3 *iface, REFIID riid, void **ppv)
{
   HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle3(iface);

   return IHTMLCurrentStyle_QueryInterface(&This->IHTMLCurrentStyle_iface, riid, ppv);
}

static ULONG WINAPI HTMLCurrentStyle3_AddRef(IHTMLCurrentStyle3 *iface)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle3(iface);

    return IHTMLCurrentStyle_AddRef(&This->IHTMLCurrentStyle_iface);
}

static ULONG WINAPI HTMLCurrentStyle3_Release(IHTMLCurrentStyle3 *iface)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle3(iface);
    return IHTMLCurrentStyle_Release(&This->IHTMLCurrentStyle_iface);
}

static HRESULT WINAPI HTMLCurrentStyle3_GetTypeInfoCount(IHTMLCurrentStyle3 *iface, UINT *pctinfo)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle3(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLCurrentStyle3_GetTypeInfo(IHTMLCurrentStyle3 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle3(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLCurrentStyle3_GetIDsOfNames(IHTMLCurrentStyle3 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle3(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLCurrentStyle3_Invoke(IHTMLCurrentStyle3 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle3(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLCurrentStyle3_get_textOverflow(IHTMLCurrentStyle3 *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle3_get_minHeight(IHTMLCurrentStyle3 *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle3_get_wordSpacing(IHTMLCurrentStyle3 *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle3_get_whiteSpace(IHTMLCurrentStyle3 *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLCurrentStyle3Vtbl HTMLCurrentStyle3Vtbl = {
    HTMLCurrentStyle3_QueryInterface,
    HTMLCurrentStyle3_AddRef,
    HTMLCurrentStyle3_Release,
    HTMLCurrentStyle3_GetTypeInfoCount,
    HTMLCurrentStyle3_GetTypeInfo,
    HTMLCurrentStyle3_GetIDsOfNames,
    HTMLCurrentStyle3_Invoke,
    HTMLCurrentStyle3_get_textOverflow,
    HTMLCurrentStyle3_get_minHeight,
    HTMLCurrentStyle3_get_wordSpacing,
    HTMLCurrentStyle3_get_whiteSpace
};

/* IHTMLCurrentStyle4 */
static HRESULT WINAPI HTMLCurrentStyle4_QueryInterface(IHTMLCurrentStyle4 *iface, REFIID riid, void **ppv)
{
   HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle4(iface);

   return IHTMLCurrentStyle_QueryInterface(&This->IHTMLCurrentStyle_iface, riid, ppv);
}

static ULONG WINAPI HTMLCurrentStyle4_AddRef(IHTMLCurrentStyle4 *iface)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle4(iface);

    return IHTMLCurrentStyle_AddRef(&This->IHTMLCurrentStyle_iface);
}

static ULONG WINAPI HTMLCurrentStyle4_Release(IHTMLCurrentStyle4 *iface)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle4(iface);
    return IHTMLCurrentStyle_Release(&This->IHTMLCurrentStyle_iface);
}

static HRESULT WINAPI HTMLCurrentStyle4_GetTypeInfoCount(IHTMLCurrentStyle4 *iface, UINT *pctinfo)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle4(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLCurrentStyle4_GetTypeInfo(IHTMLCurrentStyle4 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle4(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLCurrentStyle4_GetIDsOfNames(IHTMLCurrentStyle4 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle4(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLCurrentStyle4_Invoke(IHTMLCurrentStyle4 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle4(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLCurrentStyle4_msInterpolationMode(IHTMLCurrentStyle4 *iface, BSTR *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle4_get_maxHeight(IHTMLCurrentStyle4 *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle4_get_minWidth(IHTMLCurrentStyle4 *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLCurrentStyle4_get_maxWidth(IHTMLCurrentStyle4 *iface, VARIANT *p)
{
    HTMLCurrentStyle *This = impl_from_IHTMLCurrentStyle4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLCurrentStyle4Vtbl HTMLCurrentStyle4Vtbl = {
    HTMLCurrentStyle4_QueryInterface,
    HTMLCurrentStyle4_AddRef,
    HTMLCurrentStyle4_Release,
    HTMLCurrentStyle4_GetTypeInfoCount,
    HTMLCurrentStyle4_GetTypeInfo,
    HTMLCurrentStyle4_GetIDsOfNames,
    HTMLCurrentStyle4_Invoke,
    HTMLCurrentStyle4_msInterpolationMode,
    HTMLCurrentStyle4_get_maxHeight,
    HTMLCurrentStyle4_get_minWidth,
    HTMLCurrentStyle4_get_maxWidth
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
    nsIDOMWindow *nsview;
    nsAString nsempty_str;
    HTMLCurrentStyle *ret;
    nsresult nsres;

    if(!elem->node.doc->nsdoc)  {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMHTMLDocument_GetDefaultView(elem->node.doc->nsdoc, &nsview);
    if(NS_FAILED(nsres)) {
        ERR("GetDefaultView failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsAString_Init(&nsempty_str, NULL);
    nsres = nsIDOMWindow_GetComputedStyle(nsview, (nsIDOMElement*)elem->nselem, &nsempty_str, &nsstyle);
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

    ret->IHTMLCurrentStyle_iface.lpVtbl  = &HTMLCurrentStyleVtbl;
    ret->IHTMLCurrentStyle2_iface.lpVtbl = &HTMLCurrentStyle2Vtbl;
    ret->IHTMLCurrentStyle3_iface.lpVtbl = &HTMLCurrentStyle3Vtbl;
    ret->IHTMLCurrentStyle4_iface.lpVtbl = &HTMLCurrentStyle4Vtbl;
    ret->ref = 1;
    ret->nsstyle = nsstyle;

    init_dispex(&ret->dispex, (IUnknown*)&ret->IHTMLCurrentStyle_iface,  &HTMLCurrentStyle_dispex);

    IHTMLElement_AddRef(&elem->IHTMLElement_iface);
    ret->elem = elem;

    *p = &ret->IHTMLCurrentStyle_iface;
    return S_OK;
}
