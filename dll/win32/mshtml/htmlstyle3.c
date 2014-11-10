/*
 * Copyright 2009 Alistair Leslie-Hughes
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

static inline HTMLStyle *impl_from_IHTMLStyle3(IHTMLStyle3 *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyle, IHTMLStyle3_iface);
}

static HRESULT WINAPI HTMLStyle3_QueryInterface(IHTMLStyle3 *iface, REFIID riid, void **ppv)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);

    return IHTMLStyle_QueryInterface(&This->IHTMLStyle_iface, riid, ppv);
}

static ULONG WINAPI HTMLStyle3_AddRef(IHTMLStyle3 *iface)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);

    return IHTMLStyle_AddRef(&This->IHTMLStyle_iface);
}

static ULONG WINAPI HTMLStyle3_Release(IHTMLStyle3 *iface)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);

    return IHTMLStyle_Release(&This->IHTMLStyle_iface);
}

static HRESULT WINAPI HTMLStyle3_GetTypeInfoCount(IHTMLStyle3 *iface, UINT *pctinfo)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLStyle3_GetTypeInfo(IHTMLStyle3 *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLStyle3_GetIDsOfNames(IHTMLStyle3 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLStyle3_Invoke(IHTMLStyle3 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLStyle3_put_layoutFlow(IHTMLStyle3 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_layoutFlow(IHTMLStyle3 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_zoom(IHTMLStyle3 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    /* zoom property is IE CSS extension that is mostly used as a hack to workaround IE bugs.
     * The value is set to 1 then. We can safely ignore setting zoom to 1. */
    if(V_VT(&v) == VT_I4 && V_I4(&v) == 1)
        return S_OK;

    FIXME("stub for %s\n", debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_zoom(IHTMLStyle3 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_wordWrap(IHTMLStyle3 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_nsstyle_attr(This->nsstyle, STYLEID_WORD_WRAP, v, 0);
}

static HRESULT WINAPI HTMLStyle3_get_wordWrap(IHTMLStyle3 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr(This->nsstyle, STYLEID_WORD_WRAP, p, 0);
}

static HRESULT WINAPI HTMLStyle3_put_textUnderlinePosition(IHTMLStyle3 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_textUnderlinePosition(IHTMLStyle3 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_scrollbarBaseColor(IHTMLStyle3 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_scrollbarBaseColor(IHTMLStyle3 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_scrollbarFaceColor(IHTMLStyle3 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_scrollbarFaceColor(IHTMLStyle3 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_scrollbar3dLightColor(IHTMLStyle3 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_scrollbar3dLightColor(IHTMLStyle3 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_scrollbarShadowColor(IHTMLStyle3 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_scrollbarShadowColor(IHTMLStyle3 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_scrollbarHighlightColor(IHTMLStyle3 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_scrollbarHighlightColor(IHTMLStyle3 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_scrollbarDarkShadowColor(IHTMLStyle3 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_scrollbarDarkShadowColor(IHTMLStyle3 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_scrollbarArrowColor(IHTMLStyle3 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_scrollbarArrowColor(IHTMLStyle3 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_scrollbarTrackColor(IHTMLStyle3 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_scrollbarTrackColor(IHTMLStyle3 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_writingMode(IHTMLStyle3 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_writingMode(IHTMLStyle3 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_textAlignLast(IHTMLStyle3 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_textAlignLast(IHTMLStyle3 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_put_textKashidaSpace(IHTMLStyle3 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle3_get_textKashidaSpace(IHTMLStyle3 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLStyle3Vtbl HTMLStyle3Vtbl = {
    HTMLStyle3_QueryInterface,
    HTMLStyle3_AddRef,
    HTMLStyle3_Release,
    HTMLStyle3_GetTypeInfoCount,
    HTMLStyle3_GetTypeInfo,
    HTMLStyle3_GetIDsOfNames,
    HTMLStyle3_Invoke,
    HTMLStyle3_put_layoutFlow,
    HTMLStyle3_get_layoutFlow,
    HTMLStyle3_put_zoom,
    HTMLStyle3_get_zoom,
    HTMLStyle3_put_wordWrap,
    HTMLStyle3_get_wordWrap,
    HTMLStyle3_put_textUnderlinePosition,
    HTMLStyle3_get_textUnderlinePosition,
    HTMLStyle3_put_scrollbarBaseColor,
    HTMLStyle3_get_scrollbarBaseColor,
    HTMLStyle3_put_scrollbarFaceColor,
    HTMLStyle3_get_scrollbarFaceColor,
    HTMLStyle3_put_scrollbar3dLightColor,
    HTMLStyle3_get_scrollbar3dLightColor,
    HTMLStyle3_put_scrollbarShadowColor,
    HTMLStyle3_get_scrollbarShadowColor,
    HTMLStyle3_put_scrollbarHighlightColor,
    HTMLStyle3_get_scrollbarHighlightColor,
    HTMLStyle3_put_scrollbarDarkShadowColor,
    HTMLStyle3_get_scrollbarDarkShadowColor,
    HTMLStyle3_put_scrollbarArrowColor,
    HTMLStyle3_get_scrollbarArrowColor,
    HTMLStyle3_put_scrollbarTrackColor,
    HTMLStyle3_get_scrollbarTrackColor,
    HTMLStyle3_put_writingMode,
    HTMLStyle3_get_writingMode,
    HTMLStyle3_put_textAlignLast,
    HTMLStyle3_get_textAlignLast,
    HTMLStyle3_put_textKashidaSpace,
    HTMLStyle3_get_textKashidaSpace
};

/*
 * IHTMLStyle4 Interface
 */
static inline HTMLStyle *impl_from_IHTMLStyle4(IHTMLStyle4 *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyle, IHTMLStyle4_iface);
}

static HRESULT WINAPI HTMLStyle4_QueryInterface(IHTMLStyle4 *iface, REFIID riid, void **ppv)
{
    HTMLStyle *This = impl_from_IHTMLStyle4(iface);

    return IHTMLStyle_QueryInterface(&This->IHTMLStyle_iface, riid, ppv);
}

static ULONG WINAPI HTMLStyle4_AddRef(IHTMLStyle4 *iface)
{
    HTMLStyle *This = impl_from_IHTMLStyle4(iface);

    return IHTMLStyle_AddRef(&This->IHTMLStyle_iface);
}

static ULONG WINAPI HTMLStyle4_Release(IHTMLStyle4 *iface)
{
    HTMLStyle *This = impl_from_IHTMLStyle4(iface);

    return IHTMLStyle_Release(&This->IHTMLStyle_iface);
}

static HRESULT WINAPI HTMLStyle4_GetTypeInfoCount(IHTMLStyle4 *iface, UINT *pctinfo)
{
    HTMLStyle *This = impl_from_IHTMLStyle4(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLStyle4_GetTypeInfo(IHTMLStyle4 *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLStyle *This = impl_from_IHTMLStyle4(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLStyle4_GetIDsOfNames(IHTMLStyle4 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLStyle *This = impl_from_IHTMLStyle4(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLStyle4_Invoke(IHTMLStyle4 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLStyle *This = impl_from_IHTMLStyle4(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLStyle4_put_textOverflow(IHTMLStyle4 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle4(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle4_get_textOverflow(IHTMLStyle4 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle4_put_minHeight(IHTMLStyle4 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle4(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_MIN_HEIGHT, &v, 0);
}

static HRESULT WINAPI HTMLStyle4_get_minHeight(IHTMLStyle4 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle4(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr_var(This->nsstyle, STYLEID_MIN_HEIGHT, p, 0);
}

static const IHTMLStyle4Vtbl HTMLStyle4Vtbl = {
    HTMLStyle4_QueryInterface,
    HTMLStyle4_AddRef,
    HTMLStyle4_Release,
    HTMLStyle4_GetTypeInfoCount,
    HTMLStyle4_GetTypeInfo,
    HTMLStyle4_GetIDsOfNames,
    HTMLStyle4_Invoke,
    HTMLStyle4_put_textOverflow,
    HTMLStyle4_get_textOverflow,
    HTMLStyle4_put_minHeight,
    HTMLStyle4_get_minHeight
};

static inline HTMLStyle *impl_from_IHTMLStyle5(IHTMLStyle5 *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyle, IHTMLStyle5_iface);
}

static HRESULT WINAPI HTMLStyle5_QueryInterface(IHTMLStyle5 *iface, REFIID riid, void **ppv)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);

    return IHTMLStyle_QueryInterface(&This->IHTMLStyle_iface, riid, ppv);
}

static ULONG WINAPI HTMLStyle5_AddRef(IHTMLStyle5 *iface)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);

    return IHTMLStyle_AddRef(&This->IHTMLStyle_iface);
}

static ULONG WINAPI HTMLStyle5_Release(IHTMLStyle5 *iface)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);

    return IHTMLStyle_Release(&This->IHTMLStyle_iface);
}

static HRESULT WINAPI HTMLStyle5_GetTypeInfoCount(IHTMLStyle5 *iface, UINT *pctinfo)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLStyle5_GetTypeInfo(IHTMLStyle5 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLStyle5_GetIDsOfNames(IHTMLStyle5 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLStyle5_Invoke(IHTMLStyle5 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLStyle5_put_msInterpolationMode(IHTMLStyle5 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle5_get_msInterpolationMode(IHTMLStyle5 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle5_put_maxHeight(IHTMLStyle5 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle5_get_maxHeight(IHTMLStyle5 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle5_put_minWidth(IHTMLStyle5 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_nsstyle_attr_var(This->nsstyle, STYLEID_MIN_WIDTH, &v, ATTR_FIX_PX);
}

static HRESULT WINAPI HTMLStyle5_get_minWidth(IHTMLStyle5 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr_var(This->nsstyle, STYLEID_MIN_WIDTH, p, 0);
}

static HRESULT WINAPI HTMLStyle5_put_maxWidth(IHTMLStyle5 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle5_get_maxWidth(IHTMLStyle5 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle5(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLStyle5Vtbl HTMLStyle5Vtbl = {
    HTMLStyle5_QueryInterface,
    HTMLStyle5_AddRef,
    HTMLStyle5_Release,
    HTMLStyle5_GetTypeInfoCount,
    HTMLStyle5_GetTypeInfo,
    HTMLStyle5_GetIDsOfNames,
    HTMLStyle5_Invoke,
    HTMLStyle5_put_msInterpolationMode,
    HTMLStyle5_get_msInterpolationMode,
    HTMLStyle5_put_maxHeight,
    HTMLStyle5_get_maxHeight,
    HTMLStyle5_put_minWidth,
    HTMLStyle5_get_minWidth,
    HTMLStyle5_put_maxWidth,
    HTMLStyle5_get_maxWidth
};

static inline HTMLStyle *impl_from_IHTMLStyle6(IHTMLStyle6 *iface)
{
    return CONTAINING_RECORD(iface, HTMLStyle, IHTMLStyle6_iface);
}

static HRESULT WINAPI HTMLStyle6_QueryInterface(IHTMLStyle6 *iface, REFIID riid, void **ppv)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);

    return IHTMLStyle_QueryInterface(&This->IHTMLStyle_iface, riid, ppv);
}

static ULONG WINAPI HTMLStyle6_AddRef(IHTMLStyle6 *iface)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);

    return IHTMLStyle_AddRef(&This->IHTMLStyle_iface);
}

static ULONG WINAPI HTMLStyle6_Release(IHTMLStyle6 *iface)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);

    return IHTMLStyle_Release(&This->IHTMLStyle_iface);
}

static HRESULT WINAPI HTMLStyle6_GetTypeInfoCount(IHTMLStyle6 *iface, UINT *pctinfo)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    return IDispatchEx_GetTypeInfoCount(&This->dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLStyle6_GetTypeInfo(IHTMLStyle6 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    return IDispatchEx_GetTypeInfo(&This->dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLStyle6_GetIDsOfNames(IHTMLStyle6 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    return IDispatchEx_GetIDsOfNames(&This->dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLStyle6_Invoke(IHTMLStyle6 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    return IDispatchEx_Invoke(&This->dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLStyle6_put_content(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_content(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_contentSide(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_contentSide(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_counterIncrement(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_counterIncrement(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_counterReset(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_counterReset(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_outline(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_nsstyle_attr(This->nsstyle, STYLEID_OUTLINE, v, 0);
}

static HRESULT WINAPI HTMLStyle6_get_outline(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr(This->nsstyle, STYLEID_OUTLINE, p, ATTR_NO_NULL);
}

static HRESULT WINAPI HTMLStyle6_put_outlineWidth(IHTMLStyle6 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_outlineWidth(IHTMLStyle6 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_outlineStyle(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_outlineStyle(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_outlineColor(IHTMLStyle6 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_outlineColor(IHTMLStyle6 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_boxSizing(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return set_nsstyle_attr(This->nsstyle, STYLEID_BOX_SIZING, v, 0);
}

static HRESULT WINAPI HTMLStyle6_get_boxSizing(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_nsstyle_attr(This->nsstyle, STYLEID_BOX_SIZING, p, 0);
}

static HRESULT WINAPI HTMLStyle6_put_boxSpacing(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_boxSpacing(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_orphans(IHTMLStyle6 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_orphans(IHTMLStyle6 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_windows(IHTMLStyle6 *iface, VARIANT v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_windows(IHTMLStyle6 *iface, VARIANT *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_pageBreakInside(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_pageBreakInside(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_emptyCells(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_emptyCells(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_msBlockProgression(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_msBlockProgression(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_put_quotes(IHTMLStyle6 *iface, BSTR v)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLStyle6_get_quotes(IHTMLStyle6 *iface, BSTR *p)
{
    HTMLStyle *This = impl_from_IHTMLStyle6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLStyle6Vtbl HTMLStyle6Vtbl = {
    HTMLStyle6_QueryInterface,
    HTMLStyle6_AddRef,
    HTMLStyle6_Release,
    HTMLStyle6_GetTypeInfoCount,
    HTMLStyle6_GetTypeInfo,
    HTMLStyle6_GetIDsOfNames,
    HTMLStyle6_Invoke,
    HTMLStyle6_put_content,
    HTMLStyle6_get_content,
    HTMLStyle6_put_contentSide,
    HTMLStyle6_get_contentSide,
    HTMLStyle6_put_counterIncrement,
    HTMLStyle6_get_counterIncrement,
    HTMLStyle6_put_counterReset,
    HTMLStyle6_get_counterReset,
    HTMLStyle6_put_outline,
    HTMLStyle6_get_outline,
    HTMLStyle6_put_outlineWidth,
    HTMLStyle6_get_outlineWidth,
    HTMLStyle6_put_outlineStyle,
    HTMLStyle6_get_outlineStyle,
    HTMLStyle6_put_outlineColor,
    HTMLStyle6_get_outlineColor,
    HTMLStyle6_put_boxSizing,
    HTMLStyle6_get_boxSizing,
    HTMLStyle6_put_boxSpacing,
    HTMLStyle6_get_boxSpacing,
    HTMLStyle6_put_orphans,
    HTMLStyle6_get_orphans,
    HTMLStyle6_put_windows,
    HTMLStyle6_get_windows,
    HTMLStyle6_put_pageBreakInside,
    HTMLStyle6_get_pageBreakInside,
    HTMLStyle6_put_emptyCells,
    HTMLStyle6_get_emptyCells,
    HTMLStyle6_put_msBlockProgression,
    HTMLStyle6_get_msBlockProgression,
    HTMLStyle6_put_quotes,
    HTMLStyle6_get_quotes
};

void HTMLStyle3_Init(HTMLStyle *This)
{
    This->IHTMLStyle3_iface.lpVtbl = &HTMLStyle3Vtbl;
    This->IHTMLStyle4_iface.lpVtbl = &HTMLStyle4Vtbl;
    This->IHTMLStyle5_iface.lpVtbl = &HTMLStyle5Vtbl;
    This->IHTMLStyle6_iface.lpVtbl = &HTMLStyle6Vtbl;
}
