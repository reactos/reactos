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

static inline HTMLElement *impl_from_IHTMLElement3(IHTMLElement3 *iface)
{
    return CONTAINING_RECORD(iface, HTMLElement, IHTMLElement3_iface);
}

static HRESULT WINAPI HTMLElement3_QueryInterface(IHTMLElement3 *iface,
                                                  REFIID riid, void **ppv)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    return IHTMLElement_QueryInterface(&This->IHTMLElement_iface, riid, ppv);
}

static ULONG WINAPI HTMLElement3_AddRef(IHTMLElement3 *iface)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    return IHTMLElement_AddRef(&This->IHTMLElement_iface);
}

static ULONG WINAPI HTMLElement3_Release(IHTMLElement3 *iface)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    return IHTMLElement_Release(&This->IHTMLElement_iface);
}

static HRESULT WINAPI HTMLElement3_GetTypeInfoCount(IHTMLElement3 *iface, UINT *pctinfo)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    return IDispatchEx_GetTypeInfoCount(&This->node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLElement3_GetTypeInfo(IHTMLElement3 *iface, UINT iTInfo,
                                               LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    return IDispatchEx_GetTypeInfo(&This->node.dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLElement3_GetIDsOfNames(IHTMLElement3 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    return IDispatchEx_GetIDsOfNames(&This->node.dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLElement3_Invoke(IHTMLElement3 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    return IDispatchEx_Invoke(&This->node.dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLElement3_mergeAttributes(IHTMLElement3 *iface, IHTMLElement *mergeThis, VARIANT *pvarFlags)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%p %p)\n", This, mergeThis, pvarFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_get_isMultiLine(IHTMLElement3 *iface, VARIANT_BOOL *p)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_get_canHaveHTML(IHTMLElement3 *iface, VARIANT_BOOL *p)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_put_onlayoutcomplete(IHTMLElement3 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_get_onlayoutcomplete(IHTMLElement3 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_put_onpage(IHTMLElement3 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_get_onpage(IHTMLElement3 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_put_inflateBlock(IHTMLElement3 *iface, VARIANT_BOOL v)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_get_inflateBlock(IHTMLElement3 *iface, VARIANT_BOOL *p)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_put_onbeforedeactivate(IHTMLElement3 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_get_onbeforedeactivate(IHTMLElement3 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_setActive(IHTMLElement3 *iface)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_put_contentEditable(IHTMLElement3 *iface, BSTR v)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    nsresult nsres;
    nsAString str;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&str, v);
    nsres = nsIDOMHTMLElement_SetContentEditable(This->nselem, &str);
    nsAString_Finish(&str);

    if (NS_FAILED(nsres)){
        ERR("SetContentEditable(%s) failed!\n", debugstr_w(v));
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLElement3_get_contentEditable(IHTMLElement3 *iface, BSTR *p)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    nsresult nsres;
    nsAString str;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&str, NULL);
    nsres = nsIDOMHTMLElement_GetContentEditable(This->nselem, &str);
    return return_nsstr(nsres, &str, p);
}

static HRESULT WINAPI HTMLElement3_get_isContentEditable(IHTMLElement3 *iface, VARIANT_BOOL *p)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_put_hideFocus(IHTMLElement3 *iface, VARIANT_BOOL v)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_get_hideFocus(IHTMLElement3 *iface, VARIANT_BOOL *p)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const WCHAR disabledW[] = {'d','i','s','a','b','l','e','d',0};

static HRESULT WINAPI HTMLElement3_put_disabled(IHTMLElement3 *iface, VARIANT_BOOL v)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    VARIANT *var;
    HRESULT hres;

    TRACE("(%p)->(%x)\n", This, v);

    if(This->node.vtbl->put_disabled)
        return This->node.vtbl->put_disabled(&This->node, v);

    hres = dispex_get_dprop_ref(&This->node.dispex, disabledW, TRUE, &var);
    if(FAILED(hres))
        return hres;

    VariantClear(var);
    V_VT(var) = VT_BOOL;
    V_BOOL(var) = v;
    return S_OK;
}

static HRESULT WINAPI HTMLElement3_get_disabled(IHTMLElement3 *iface, VARIANT_BOOL *p)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    VARIANT *var;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->node.vtbl->get_disabled)
        return This->node.vtbl->get_disabled(&This->node, p);

    hres = dispex_get_dprop_ref(&This->node.dispex, disabledW, FALSE, &var);
    if(hres == DISP_E_UNKNOWNNAME) {
        *p = VARIANT_FALSE;
        return S_OK;
    }
    if(FAILED(hres))
        return hres;

    if(V_VT(var) != VT_BOOL) {
        FIXME("value is %s\n", debugstr_variant(var));
        return E_NOTIMPL;
    }

    *p = V_BOOL(var);
    return S_OK;
}

static HRESULT WINAPI HTMLElement3_get_isDisabled(IHTMLElement3 *iface, VARIANT_BOOL *p)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_put_onmove(IHTMLElement3 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_get_onmove(IHTMLElement3 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_put_oncontrolselect(IHTMLElement3 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_get_oncontrolselect(IHTMLElement3 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_fireEvent(IHTMLElement3 *iface, BSTR bstrEventName,
        VARIANT *pvarEventObject, VARIANT_BOOL *pfCancelled)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_w(bstrEventName), debugstr_variant(pvarEventObject),
          pfCancelled);

    return dispatch_event(&This->node, bstrEventName, pvarEventObject, pfCancelled);
}

static HRESULT WINAPI HTMLElement3_put_onresizestart(IHTMLElement3 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_get_onresizestart(IHTMLElement3 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_put_onresizeend(IHTMLElement3 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_get_onresizeend(IHTMLElement3 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_put_onmovestart(IHTMLElement3 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_get_onmovestart(IHTMLElement3 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_put_onmoveend(IHTMLElement3 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_get_onmoveend(IHTMLElement3 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_put_onmousecenter(IHTMLElement3 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_get_onmousecenter(IHTMLElement3 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_put_onmouseleave(IHTMLElement3 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_get_onmouseleave(IHTMLElement3 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_put_onactivate(IHTMLElement3 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_get_onactivate(IHTMLElement3 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_put_ondeactivate(IHTMLElement3 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_get_ondeactivate(IHTMLElement3 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_dragDrop(IHTMLElement3 *iface, VARIANT_BOOL *pfRet)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%p)\n", This, pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement3_get_glyphMode(IHTMLElement3 *iface, LONG *p)
{
    HTMLElement *This = impl_from_IHTMLElement3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLElement3Vtbl HTMLElement3Vtbl = {
    HTMLElement3_QueryInterface,
    HTMLElement3_AddRef,
    HTMLElement3_Release,
    HTMLElement3_GetTypeInfoCount,
    HTMLElement3_GetTypeInfo,
    HTMLElement3_GetIDsOfNames,
    HTMLElement3_Invoke,
    HTMLElement3_mergeAttributes,
    HTMLElement3_get_isMultiLine,
    HTMLElement3_get_canHaveHTML,
    HTMLElement3_put_onlayoutcomplete,
    HTMLElement3_get_onlayoutcomplete,
    HTMLElement3_put_onpage,
    HTMLElement3_get_onpage,
    HTMLElement3_put_inflateBlock,
    HTMLElement3_get_inflateBlock,
    HTMLElement3_put_onbeforedeactivate,
    HTMLElement3_get_onbeforedeactivate,
    HTMLElement3_setActive,
    HTMLElement3_put_contentEditable,
    HTMLElement3_get_contentEditable,
    HTMLElement3_get_isContentEditable,
    HTMLElement3_put_hideFocus,
    HTMLElement3_get_hideFocus,
    HTMLElement3_put_disabled,
    HTMLElement3_get_disabled,
    HTMLElement3_get_isDisabled,
    HTMLElement3_put_onmove,
    HTMLElement3_get_onmove,
    HTMLElement3_put_oncontrolselect,
    HTMLElement3_get_oncontrolselect,
    HTMLElement3_fireEvent,
    HTMLElement3_put_onresizestart,
    HTMLElement3_get_onresizestart,
    HTMLElement3_put_onresizeend,
    HTMLElement3_get_onresizeend,
    HTMLElement3_put_onmovestart,
    HTMLElement3_get_onmovestart,
    HTMLElement3_put_onmoveend,
    HTMLElement3_get_onmoveend,
    HTMLElement3_put_onmousecenter,
    HTMLElement3_get_onmousecenter,
    HTMLElement3_put_onmouseleave,
    HTMLElement3_get_onmouseleave,
    HTMLElement3_put_onactivate,
    HTMLElement3_get_onactivate,
    HTMLElement3_put_ondeactivate,
    HTMLElement3_get_ondeactivate,
    HTMLElement3_dragDrop,
    HTMLElement3_get_glyphMode
};

static inline HTMLElement *impl_from_IHTMLElement4(IHTMLElement4 *iface)
{
    return CONTAINING_RECORD(iface, HTMLElement, IHTMLElement4_iface);
}

static HRESULT WINAPI HTMLElement4_QueryInterface(IHTMLElement4 *iface,
        REFIID riid, void **ppv)
{
    HTMLElement *This = impl_from_IHTMLElement4(iface);
    return IHTMLElement_QueryInterface(&This->IHTMLElement_iface, riid, ppv);
}

static ULONG WINAPI HTMLElement4_AddRef(IHTMLElement4 *iface)
{
    HTMLElement *This = impl_from_IHTMLElement4(iface);
    return IHTMLElement_AddRef(&This->IHTMLElement_iface);
}

static ULONG WINAPI HTMLElement4_Release(IHTMLElement4 *iface)
{
    HTMLElement *This = impl_from_IHTMLElement4(iface);
    return IHTMLElement_Release(&This->IHTMLElement_iface);
}

static HRESULT WINAPI HTMLElement4_GetTypeInfoCount(IHTMLElement4 *iface, UINT *pctinfo)
{
    HTMLElement *This = impl_from_IHTMLElement4(iface);
    return IDispatchEx_GetTypeInfoCount(&This->node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLElement4_GetTypeInfo(IHTMLElement4 *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLElement *This = impl_from_IHTMLElement4(iface);
    return IDispatchEx_GetTypeInfo(&This->node.dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLElement4_GetIDsOfNames(IHTMLElement4 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLElement *This = impl_from_IHTMLElement4(iface);
    return IDispatchEx_GetIDsOfNames(&This->node.dispex.IDispatchEx_iface, riid, rgszNames, cNames,
            lcid, rgDispId);
}

static HRESULT WINAPI HTMLElement4_Invoke(IHTMLElement4 *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLElement *This = impl_from_IHTMLElement4(iface);
    return IDispatchEx_Invoke(&This->node.dispex.IDispatchEx_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLElement4_put_onmousewheel(IHTMLElement4 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement4(iface);

    FIXME("(%p)->(%s) semi-stub\n", This, debugstr_variant(&v));

    return set_node_event(&This->node, EVENTID_MOUSEWHEEL, &v);
}

static HRESULT WINAPI HTMLElement4_get_onmousewheel(IHTMLElement4 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement4(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->node, EVENTID_MOUSEWHEEL, p);
}

static HRESULT WINAPI HTMLElement4_normalize(IHTMLElement4 *iface)
{
    HTMLElement *This = impl_from_IHTMLElement4(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement4_getAttributeNode(IHTMLElement4 *iface, BSTR bstrname, IHTMLDOMAttribute **ppAttribute)
{
    HTMLElement *This = impl_from_IHTMLElement4(iface);
    HTMLAttributeCollection *attrs;
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(bstrname), ppAttribute);

    hres = HTMLElement_get_attr_col(&This->node, &attrs);
    if(FAILED(hres))
        return hres;

    hres = IHTMLAttributeCollection2_getNamedItem(&attrs->IHTMLAttributeCollection2_iface, bstrname, ppAttribute);
    IHTMLAttributeCollection_Release(&attrs->IHTMLAttributeCollection_iface);
    return hres;
}

static HRESULT WINAPI HTMLElement4_setAttributeNode(IHTMLElement4 *iface, IHTMLDOMAttribute *pattr,
        IHTMLDOMAttribute **ppretAttribute)
{
    HTMLElement *This = impl_from_IHTMLElement4(iface);
    FIXME("(%p)->(%p %p)\n", This, pattr, ppretAttribute);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement4_removeAttributeNode(IHTMLElement4 *iface, IHTMLDOMAttribute *pattr,
        IHTMLDOMAttribute **ppretAttribute)
{
    HTMLElement *This = impl_from_IHTMLElement4(iface);
    FIXME("(%p)->(%p %p)\n", This, pattr, ppretAttribute);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement4_put_onbeforeactivate(IHTMLElement4 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement4(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement4_get_onbeforeactivate(IHTMLElement4 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement4_put_onfocusin(IHTMLElement4 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement4(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement4_get_onfocusin(IHTMLElement4 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement4_put_onfocusout(IHTMLElement4 *iface, VARIANT v)
{
    HTMLElement *This = impl_from_IHTMLElement4(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElement4_get_onfocusout(IHTMLElement4 *iface, VARIANT *p)
{
    HTMLElement *This = impl_from_IHTMLElement4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLElement4Vtbl HTMLElement4Vtbl = {
    HTMLElement4_QueryInterface,
    HTMLElement4_AddRef,
    HTMLElement4_Release,
    HTMLElement4_GetTypeInfoCount,
    HTMLElement4_GetTypeInfo,
    HTMLElement4_GetIDsOfNames,
    HTMLElement4_Invoke,
    HTMLElement4_put_onmousewheel,
    HTMLElement4_get_onmousewheel,
    HTMLElement4_normalize,
    HTMLElement4_getAttributeNode,
    HTMLElement4_setAttributeNode,
    HTMLElement4_removeAttributeNode,
    HTMLElement4_put_onbeforeactivate,
    HTMLElement4_get_onbeforeactivate,
    HTMLElement4_put_onfocusin,
    HTMLElement4_get_onfocusin,
    HTMLElement4_put_onfocusout,
    HTMLElement4_get_onfocusout
};

void HTMLElement3_Init(HTMLElement *This)
{
    This->IHTMLElement3_iface.lpVtbl = &HTMLElement3Vtbl;
    This->IHTMLElement4_iface.lpVtbl = &HTMLElement4Vtbl;
}
