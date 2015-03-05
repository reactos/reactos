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

#include <limits.h>

typedef struct {
    HTMLElement element;

    IHTMLInputElement IHTMLInputElement_iface;
    IHTMLInputTextElement IHTMLInputTextElement_iface;

    nsIDOMHTMLInputElement *nsinput;
} HTMLInputElement;

static const WCHAR forW[] = {'f','o','r',0};

static inline HTMLInputElement *impl_from_IHTMLInputElement(IHTMLInputElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLInputElement, IHTMLInputElement_iface);
}

static inline HTMLInputElement *impl_from_IHTMLInputTextElement(IHTMLInputTextElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLInputElement, IHTMLInputTextElement_iface);
}

static HRESULT WINAPI HTMLInputElement_QueryInterface(IHTMLInputElement *iface,
                                                         REFIID riid, void **ppv)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLInputElement_AddRef(IHTMLInputElement *iface)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLInputElement_Release(IHTMLInputElement *iface)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLInputElement_GetTypeInfoCount(IHTMLInputElement *iface, UINT *pctinfo)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);

    return IDispatchEx_GetTypeInfoCount(&This->element.node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLInputElement_GetTypeInfo(IHTMLInputElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);

    return IDispatchEx_GetTypeInfo(&This->element.node.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLInputElement_GetIDsOfNames(IHTMLInputElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);

    return IDispatchEx_GetIDsOfNames(&This->element.node.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLInputElement_Invoke(IHTMLInputElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);

    return IDispatchEx_Invoke(&This->element.node.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLInputElement_put_type(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    nsAString type_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    /*
     * FIXME:
     * On IE setting type works only on dynamically created elements before adding them to DOM tree.
     */
    nsAString_InitDepend(&type_str, v);
    nsres = nsIDOMHTMLInputElement_SetType(This->nsinput, &type_str);
    nsAString_Finish(&type_str);
    if(NS_FAILED(nsres)) {
        ERR("SetType failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLInputElement_get_type(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    nsAString type_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&type_str, NULL);
    nsres = nsIDOMHTMLInputElement_GetType(This->nsinput, &type_str);
    return return_nsstr(nsres, &type_str, p);
}

static HRESULT WINAPI HTMLInputElement_put_value(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    nsAString val_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&val_str, v);
    nsres = nsIDOMHTMLInputElement_SetValue(This->nsinput, &val_str);
    nsAString_Finish(&val_str);
    if(NS_FAILED(nsres))
        ERR("SetValue failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLInputElement_get_value(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    nsAString value_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&value_str, NULL);
    nsres = nsIDOMHTMLInputElement_GetValue(This->nsinput, &value_str);
    return return_nsstr(nsres, &value_str, p);
}

static HRESULT WINAPI HTMLInputElement_put_name(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    nsAString name_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&name_str, v);
    nsres = nsIDOMHTMLInputElement_SetName(This->nsinput, &name_str);
    nsAString_Finish(&name_str);
    if(NS_FAILED(nsres)) {
        ERR("SetName failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLInputElement_get_name(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    nsAString name_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&name_str, NULL);
    nsres = nsIDOMHTMLInputElement_GetName(This->nsinput, &name_str);
    return return_nsstr(nsres, &name_str, p);
}

static HRESULT WINAPI HTMLInputElement_put_status(IHTMLInputElement *iface, VARIANT_BOOL v)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_status(IHTMLInputElement *iface, VARIANT_BOOL *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_disabled(IHTMLInputElement *iface, VARIANT_BOOL v)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%x)\n", This, v);

    nsres = nsIDOMHTMLInputElement_SetDisabled(This->nsinput, v != VARIANT_FALSE);
    if(NS_FAILED(nsres))
        ERR("SetDisabled failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLInputElement_get_disabled(IHTMLInputElement *iface, VARIANT_BOOL *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    cpp_bool disabled = FALSE;

    TRACE("(%p)->(%p)\n", This, p);

    nsIDOMHTMLInputElement_GetDisabled(This->nsinput, &disabled);

    *p = disabled ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI HTMLInputElement_get_form(IHTMLInputElement *iface, IHTMLFormElement **p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    nsIDOMHTMLFormElement *nsform;
    HTMLDOMNode *node;
    HRESULT hres;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLInputElement_GetForm(This->nsinput, &nsform);
    if (NS_FAILED(nsres) || nsform == NULL) {
        ERR("GetForm failed: %08x, nsform: %p\n", nsres, nsform);
        *p = NULL;
        return E_FAIL;
    }

    hres = get_node(This->element.node.doc, (nsIDOMNode*)nsform, TRUE, &node);
    nsIDOMHTMLFormElement_Release(nsform);
    if (FAILED(hres))
        return hres;

    hres = IHTMLDOMNode_QueryInterface(&node->IHTMLDOMNode_iface, &IID_IHTMLElement, (void**)p);

    node_release(node);
    return hres;
}

static HRESULT WINAPI HTMLInputElement_put_size(IHTMLInputElement *iface, LONG v)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    UINT32 val = v;
    nsresult nsres;

    TRACE("(%p)->(%d)\n", This, v);
    if (v <= 0)
        return CTL_E_INVALIDPROPERTYVALUE;

    nsres = nsIDOMHTMLInputElement_SetSize(This->nsinput, val);
    if (NS_FAILED(nsres)) {
        ERR("Set Size(%u) failed: %08x\n", val, nsres);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLInputElement_get_size(IHTMLInputElement *iface, LONG *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    UINT32 val;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);
    if (p == NULL)
        return E_INVALIDARG;

    nsres = nsIDOMHTMLInputElement_GetSize(This->nsinput, &val);
    if (NS_FAILED(nsres)) {
        ERR("Get Size failed: %08x\n", nsres);
        return E_FAIL;
    }
    *p = val;
    return S_OK;
}

static HRESULT WINAPI HTMLInputElement_put_maxLength(IHTMLInputElement *iface, LONG v)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%d)\n", This, v);

    nsres = nsIDOMHTMLInputElement_SetMaxLength(This->nsinput, v);
    if(NS_FAILED(nsres)) {
        /* FIXME: Gecko throws an error on negative values, while MSHTML should accept them */
        FIXME("SetMaxLength failed\n");
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLInputElement_get_maxLength(IHTMLInputElement *iface, LONG *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    LONG max_length;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLInputElement_GetMaxLength(This->nsinput, &max_length);
    assert(nsres == NS_OK);

    /* Gecko reports -1 as default value, while MSHTML uses INT_MAX */
    *p = max_length == -1 ? INT_MAX : max_length;
    return S_OK;
}

static HRESULT WINAPI HTMLInputElement_select(IHTMLInputElement *iface)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    nsresult nsres;

    TRACE("(%p)\n", This);

    nsres = nsIDOMHTMLInputElement_Select(This->nsinput);
    if(NS_FAILED(nsres)) {
        ERR("Select failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLInputElement_put_onchange(IHTMLInputElement *iface, VARIANT v)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);

    TRACE("(%p)->()\n", This);

    return set_node_event(&This->element.node, EVENTID_CHANGE, &v);
}

static HRESULT WINAPI HTMLInputElement_get_onchange(IHTMLInputElement *iface, VARIANT *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->element.node, EVENTID_CHANGE, p);
}

static HRESULT WINAPI HTMLInputElement_put_onselect(IHTMLInputElement *iface, VARIANT v)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_onselect(IHTMLInputElement *iface, VARIANT *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_defaultValue(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&nsstr, v);
    nsres = nsIDOMHTMLInputElement_SetDefaultValue(This->nsinput, &nsstr);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres)) {
        ERR("SetValue failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLInputElement_get_defaultValue(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMHTMLInputElement_GetDefaultValue(This->nsinput, &nsstr);
    return return_nsstr(nsres, &nsstr, p);
}

static HRESULT WINAPI HTMLInputElement_put_readOnly(IHTMLInputElement *iface, VARIANT_BOOL v)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%x)\n", This, v);

    nsres = nsIDOMHTMLInputElement_SetReadOnly(This->nsinput, v != VARIANT_FALSE);
    if (NS_FAILED(nsres)) {
        ERR("Set ReadOnly Failed: %08x\n", nsres);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLInputElement_get_readOnly(IHTMLInputElement *iface, VARIANT_BOOL *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    nsresult nsres;
    cpp_bool b;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLInputElement_GetReadOnly(This->nsinput, &b);
    if (NS_FAILED(nsres)) {
        ERR("Get ReadOnly Failed: %08x\n", nsres);
        return E_FAIL;
    }
    *p = b ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI HTMLInputElement_createTextRange(IHTMLInputElement *iface, IHTMLTxtRange **range)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%p)\n", This, range);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_indeterminate(IHTMLInputElement *iface, VARIANT_BOOL v)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_indeterminate(IHTMLInputElement *iface, VARIANT_BOOL *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_defaultChecked(IHTMLInputElement *iface, VARIANT_BOOL v)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%x)\n", This, v);

    nsres = nsIDOMHTMLInputElement_SetDefaultChecked(This->nsinput, v != VARIANT_FALSE);
    if(NS_FAILED(nsres)) {
        ERR("SetDefaultChecked failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLInputElement_get_defaultChecked(IHTMLInputElement *iface, VARIANT_BOOL *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    cpp_bool default_checked = FALSE;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLInputElement_GetDefaultChecked(This->nsinput, &default_checked);
    if(NS_FAILED(nsres)) {
        ERR("GetDefaultChecked failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = default_checked ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI HTMLInputElement_put_checked(IHTMLInputElement *iface, VARIANT_BOOL v)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%x)\n", This, v);

    nsres = nsIDOMHTMLInputElement_SetChecked(This->nsinput, v != VARIANT_FALSE);
    if(NS_FAILED(nsres)) {
        ERR("SetChecked failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLInputElement_get_checked(IHTMLInputElement *iface, VARIANT_BOOL *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    cpp_bool checked;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLInputElement_GetChecked(This->nsinput, &checked);
    if(NS_FAILED(nsres)) {
        ERR("GetChecked failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = checked ? VARIANT_TRUE : VARIANT_FALSE;
    TRACE("checked=%x\n", *p);
    return S_OK;
}

static HRESULT WINAPI HTMLInputElement_put_border(IHTMLInputElement *iface, VARIANT v)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_border(IHTMLInputElement *iface, VARIANT *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_vspace(IHTMLInputElement *iface, LONG v)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_vspace(IHTMLInputElement *iface, LONG *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_hspace(IHTMLInputElement *iface, LONG v)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_hspace(IHTMLInputElement *iface, LONG *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_alt(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_alt(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_src(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&nsstr, v);
    nsres = nsIDOMHTMLInputElement_SetSrc(This->nsinput, &nsstr);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres))
        ERR("SetSrc failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLInputElement_get_src(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    const PRUnichar *src;
    nsAString src_str;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&src_str, NULL);
    nsres = nsIDOMHTMLInputElement_GetSrc(This->nsinput, &src_str);
    if(NS_FAILED(nsres)) {
        ERR("GetSrc failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsAString_GetData(&src_str, &src);
    hres = nsuri_to_url(src, FALSE, p);
    nsAString_Finish(&src_str);

    return hres;
}

static HRESULT WINAPI HTMLInputElement_put_lowsrc(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_lowsrc(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_vrml(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_vrml(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_dynsrc(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_dynsrc(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_readyState(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_complete(IHTMLInputElement *iface, VARIANT_BOOL *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_loop(IHTMLInputElement *iface, VARIANT v)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_loop(IHTMLInputElement *iface, VARIANT *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_align(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_align(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_onload(IHTMLInputElement *iface, VARIANT v)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_onload(IHTMLInputElement *iface, VARIANT *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_onerror(IHTMLInputElement *iface, VARIANT v)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_onerror(IHTMLInputElement *iface, VARIANT *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_onabort(IHTMLInputElement *iface, VARIANT v)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_onabort(IHTMLInputElement *iface, VARIANT *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_width(IHTMLInputElement *iface, LONG v)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_width(IHTMLInputElement *iface, LONG *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_height(IHTMLInputElement *iface, LONG v)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_height(IHTMLInputElement *iface, LONG *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_put_start(IHTMLInputElement *iface, BSTR v)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputElement_get_start(IHTMLInputElement *iface, BSTR *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLInputElementVtbl HTMLInputElementVtbl = {
    HTMLInputElement_QueryInterface,
    HTMLInputElement_AddRef,
    HTMLInputElement_Release,
    HTMLInputElement_GetTypeInfoCount,
    HTMLInputElement_GetTypeInfo,
    HTMLInputElement_GetIDsOfNames,
    HTMLInputElement_Invoke,
    HTMLInputElement_put_type,
    HTMLInputElement_get_type,
    HTMLInputElement_put_value,
    HTMLInputElement_get_value,
    HTMLInputElement_put_name,
    HTMLInputElement_get_name,
    HTMLInputElement_put_status,
    HTMLInputElement_get_status,
    HTMLInputElement_put_disabled,
    HTMLInputElement_get_disabled,
    HTMLInputElement_get_form,
    HTMLInputElement_put_size,
    HTMLInputElement_get_size,
    HTMLInputElement_put_maxLength,
    HTMLInputElement_get_maxLength,
    HTMLInputElement_select,
    HTMLInputElement_put_onchange,
    HTMLInputElement_get_onchange,
    HTMLInputElement_put_onselect,
    HTMLInputElement_get_onselect,
    HTMLInputElement_put_defaultValue,
    HTMLInputElement_get_defaultValue,
    HTMLInputElement_put_readOnly,
    HTMLInputElement_get_readOnly,
    HTMLInputElement_createTextRange,
    HTMLInputElement_put_indeterminate,
    HTMLInputElement_get_indeterminate,
    HTMLInputElement_put_defaultChecked,
    HTMLInputElement_get_defaultChecked,
    HTMLInputElement_put_checked,
    HTMLInputElement_get_checked,
    HTMLInputElement_put_border,
    HTMLInputElement_get_border,
    HTMLInputElement_put_vspace,
    HTMLInputElement_get_vspace,
    HTMLInputElement_put_hspace,
    HTMLInputElement_get_hspace,
    HTMLInputElement_put_alt,
    HTMLInputElement_get_alt,
    HTMLInputElement_put_src,
    HTMLInputElement_get_src,
    HTMLInputElement_put_lowsrc,
    HTMLInputElement_get_lowsrc,
    HTMLInputElement_put_vrml,
    HTMLInputElement_get_vrml,
    HTMLInputElement_put_dynsrc,
    HTMLInputElement_get_dynsrc,
    HTMLInputElement_get_readyState,
    HTMLInputElement_get_complete,
    HTMLInputElement_put_loop,
    HTMLInputElement_get_loop,
    HTMLInputElement_put_align,
    HTMLInputElement_get_align,
    HTMLInputElement_put_onload,
    HTMLInputElement_get_onload,
    HTMLInputElement_put_onerror,
    HTMLInputElement_get_onerror,
    HTMLInputElement_put_onabort,
    HTMLInputElement_get_onabort,
    HTMLInputElement_put_width,
    HTMLInputElement_get_width,
    HTMLInputElement_put_height,
    HTMLInputElement_get_height,
    HTMLInputElement_put_start,
    HTMLInputElement_get_start
};

static HRESULT WINAPI HTMLInputTextElement_QueryInterface(IHTMLInputTextElement *iface,
        REFIID riid, void **ppv)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLInputTextElement_AddRef(IHTMLInputTextElement *iface)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLInputTextElement_Release(IHTMLInputTextElement *iface)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLInputTextElement_GetTypeInfoCount(IHTMLInputTextElement *iface, UINT *pctinfo)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);
    return IDispatchEx_GetTypeInfoCount(&This->element.node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLInputTextElement_GetTypeInfo(IHTMLInputTextElement *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);
    return IDispatchEx_GetTypeInfo(&This->element.node.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLInputTextElement_GetIDsOfNames(IHTMLInputTextElement *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);
    return IDispatchEx_GetIDsOfNames(&This->element.node.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLInputTextElement_Invoke(IHTMLInputTextElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);
    return IDispatchEx_Invoke(&This->element.node.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLInputTextElement_get_type(IHTMLInputTextElement *iface, BSTR *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLInputElement_get_type(&This->IHTMLInputElement_iface, p);
}

static HRESULT WINAPI HTMLInputTextElement_put_value(IHTMLInputTextElement *iface, BSTR v)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return IHTMLInputElement_put_value(&This->IHTMLInputElement_iface, v);
}

static HRESULT WINAPI HTMLInputTextElement_get_value(IHTMLInputTextElement *iface, BSTR *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLInputElement_get_value(&This->IHTMLInputElement_iface, p);
}

static HRESULT WINAPI HTMLInputTextElement_put_name(IHTMLInputTextElement *iface, BSTR v)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return IHTMLInputElement_put_name(&This->IHTMLInputElement_iface, v);
}

static HRESULT WINAPI HTMLInputTextElement_get_name(IHTMLInputTextElement *iface, BSTR *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLInputElement_get_name(&This->IHTMLInputElement_iface, p);
}

static HRESULT WINAPI HTMLInputTextElement_put_status(IHTMLInputTextElement *iface, VARIANT v)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputTextElement_get_status(IHTMLInputTextElement *iface, VARIANT *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);
    TRACE("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLInputTextElement_put_disabled(IHTMLInputTextElement *iface, VARIANT_BOOL v)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);

    TRACE("(%p)->(%x)\n", This, v);

    return IHTMLInputElement_put_disabled(&This->IHTMLInputElement_iface, v);
}

static HRESULT WINAPI HTMLInputTextElement_get_disabled(IHTMLInputTextElement *iface, VARIANT_BOOL *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLInputElement_get_disabled(&This->IHTMLInputElement_iface, p);
}

static HRESULT WINAPI HTMLInputTextElement_get_form(IHTMLInputTextElement *iface, IHTMLFormElement **p)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLInputElement_get_form(&This->IHTMLInputElement_iface, p);
}

static HRESULT WINAPI HTMLInputTextElement_put_defaultValue(IHTMLInputTextElement *iface, BSTR v)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    return IHTMLInputElement_put_defaultValue(&This->IHTMLInputElement_iface, v);
}

static HRESULT WINAPI HTMLInputTextElement_get_defaultValue(IHTMLInputTextElement *iface, BSTR *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLInputElement_get_defaultValue(&This->IHTMLInputElement_iface, p);
}

static HRESULT WINAPI HTMLInputTextElement_put_size(IHTMLInputTextElement *iface, LONG v)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);

    TRACE("(%p)->(%d)\n", This, v);

    return IHTMLInputElement_put_size(&This->IHTMLInputElement_iface, v);
}

static HRESULT WINAPI HTMLInputTextElement_get_size(IHTMLInputTextElement *iface, LONG *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLInputElement_get_size(&This->IHTMLInputElement_iface, p);
}

static HRESULT WINAPI HTMLInputTextElement_put_maxLength(IHTMLInputTextElement *iface, LONG v)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);

    TRACE("(%p)->(%d)\n", This, v);

    return IHTMLInputElement_put_maxLength(&This->IHTMLInputElement_iface, v);
}

static HRESULT WINAPI HTMLInputTextElement_get_maxLength(IHTMLInputTextElement *iface, LONG *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLInputElement_get_maxLength(&This->IHTMLInputElement_iface, p);
}

static HRESULT WINAPI HTMLInputTextElement_select(IHTMLInputTextElement *iface)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);

    TRACE("(%p)\n", This);

    return IHTMLInputElement_select(&This->IHTMLInputElement_iface);
}

static HRESULT WINAPI HTMLInputTextElement_put_onchange(IHTMLInputTextElement *iface, VARIANT v)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);

    TRACE("(%p)->()\n", This);

    return IHTMLInputElement_put_onchange(&This->IHTMLInputElement_iface, v);
}

static HRESULT WINAPI HTMLInputTextElement_get_onchange(IHTMLInputTextElement *iface, VARIANT *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLInputElement_get_onchange(&This->IHTMLInputElement_iface, p);
}

static HRESULT WINAPI HTMLInputTextElement_put_onselect(IHTMLInputTextElement *iface, VARIANT v)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);

    TRACE("(%p)->()\n", This);

    return IHTMLInputElement_put_onselect(&This->IHTMLInputElement_iface, v);
}

static HRESULT WINAPI HTMLInputTextElement_get_onselect(IHTMLInputTextElement *iface, VARIANT *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLInputElement_get_onselect(&This->IHTMLInputElement_iface, p);
}

static HRESULT WINAPI HTMLInputTextElement_put_readOnly(IHTMLInputTextElement *iface, VARIANT_BOOL v)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);

    TRACE("(%p)->(%x)\n", This, v);

    return IHTMLInputElement_put_readOnly(&This->IHTMLInputElement_iface, v);
}

static HRESULT WINAPI HTMLInputTextElement_get_readOnly(IHTMLInputTextElement *iface, VARIANT_BOOL *p)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLInputElement_get_readOnly(&This->IHTMLInputElement_iface, p);
}

static HRESULT WINAPI HTMLInputTextElement_createTextRange(IHTMLInputTextElement *iface, IHTMLTxtRange **range)
{
    HTMLInputElement *This = impl_from_IHTMLInputTextElement(iface);

    TRACE("(%p)->(%p)\n", This, range);

    return IHTMLInputElement_createTextRange(&This->IHTMLInputElement_iface, range);
}

static const IHTMLInputTextElementVtbl HTMLInputTextElementVtbl = {
    HTMLInputTextElement_QueryInterface,
    HTMLInputTextElement_AddRef,
    HTMLInputTextElement_Release,
    HTMLInputTextElement_GetTypeInfoCount,
    HTMLInputTextElement_GetTypeInfo,
    HTMLInputTextElement_GetIDsOfNames,
    HTMLInputTextElement_Invoke,
    HTMLInputTextElement_get_type,
    HTMLInputTextElement_put_value,
    HTMLInputTextElement_get_value,
    HTMLInputTextElement_put_name,
    HTMLInputTextElement_get_name,
    HTMLInputTextElement_put_status,
    HTMLInputTextElement_get_status,
    HTMLInputTextElement_put_disabled,
    HTMLInputTextElement_get_disabled,
    HTMLInputTextElement_get_form,
    HTMLInputTextElement_put_defaultValue,
    HTMLInputTextElement_get_defaultValue,
    HTMLInputTextElement_put_size,
    HTMLInputTextElement_get_size,
    HTMLInputTextElement_put_maxLength,
    HTMLInputTextElement_get_maxLength,
    HTMLInputTextElement_select,
    HTMLInputTextElement_put_onchange,
    HTMLInputTextElement_get_onchange,
    HTMLInputTextElement_put_onselect,
    HTMLInputTextElement_get_onselect,
    HTMLInputTextElement_put_readOnly,
    HTMLInputTextElement_get_readOnly,
    HTMLInputTextElement_createTextRange
};

static inline HTMLInputElement *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLInputElement, element.node);
}

static HRESULT HTMLInputElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLInputElement *This = impl_from_HTMLDOMNode(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IHTMLInputElement_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IHTMLInputElement_iface;
    }else if(IsEqualGUID(&IID_IHTMLInputElement, riid)) {
        TRACE("(%p)->(IID_IHTMLInputElement %p)\n", This, ppv);
        *ppv = &This->IHTMLInputElement_iface;
    }else if(IsEqualGUID(&IID_IHTMLInputTextElement, riid)) {
        TRACE("(%p)->(IID_IHTMLInputTextElement %p)\n", This, ppv);
        *ppv = &This->IHTMLInputTextElement_iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    return HTMLElement_QI(&This->element.node, riid, ppv);
}

static HRESULT HTMLInputElementImpl_fire_event(HTMLDOMNode *iface, eventid_t eid, BOOL *handled)
{
    HTMLInputElement *This = impl_from_HTMLDOMNode(iface);

    if(eid == EVENTID_CLICK) {
        nsresult nsres;

        *handled = TRUE;

        nsres = nsIDOMHTMLInputElement_Click(This->nsinput);
        if(NS_FAILED(nsres)) {
            ERR("Click failed: %08x\n", nsres);
            return E_FAIL;
        }
    }

    return S_OK;
}

static HRESULT HTMLInputElementImpl_put_disabled(HTMLDOMNode *iface, VARIANT_BOOL v)
{
    HTMLInputElement *This = impl_from_HTMLDOMNode(iface);
    return IHTMLInputElement_put_disabled(&This->IHTMLInputElement_iface, v);
}

static HRESULT HTMLInputElementImpl_get_disabled(HTMLDOMNode *iface, VARIANT_BOOL *p)
{
    HTMLInputElement *This = impl_from_HTMLDOMNode(iface);
    return IHTMLInputElement_get_disabled(&This->IHTMLInputElement_iface, p);
}

static const NodeImplVtbl HTMLInputElementImplVtbl = {
    HTMLInputElement_QI,
    HTMLElement_destructor,
    HTMLElement_cpc,
    HTMLElement_clone,
    HTMLElement_handle_event,
    HTMLElement_get_attr_col,
    NULL,
    HTMLInputElementImpl_fire_event,
    HTMLInputElementImpl_put_disabled,
    HTMLInputElementImpl_get_disabled,
};

static const tid_t HTMLInputElement_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLInputElement_tid,
    0
};
static dispex_static_data_t HTMLInputElement_dispex = {
    NULL,
    DispHTMLInputElement_tid,
    NULL,
    HTMLInputElement_iface_tids
};

HRESULT HTMLInputElement_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem, HTMLElement **elem)
{
    HTMLInputElement *ret;
    nsresult nsres;

    ret = heap_alloc_zero(sizeof(HTMLInputElement));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLInputElement_iface.lpVtbl = &HTMLInputElementVtbl;
    ret->IHTMLInputTextElement_iface.lpVtbl = &HTMLInputTextElementVtbl;
    ret->element.node.vtbl = &HTMLInputElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLInputElement_dispex);

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLInputElement, (void**)&ret->nsinput);

    /* Share nsinput reference with nsnode */
    assert(nsres == NS_OK && (nsIDOMNode*)ret->nsinput == ret->element.node.nsnode);
    nsIDOMNode_Release(ret->element.node.nsnode);

    *elem = &ret->element;
    return S_OK;
}

typedef struct {
    HTMLElement element;

    IHTMLLabelElement IHTMLLabelElement_iface;
} HTMLLabelElement;

static inline HTMLLabelElement *impl_from_IHTMLLabelElement(IHTMLLabelElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLLabelElement, IHTMLLabelElement_iface);
}

static HRESULT WINAPI HTMLLabelElement_QueryInterface(IHTMLLabelElement *iface,
                                                         REFIID riid, void **ppv)
{
    HTMLLabelElement *This = impl_from_IHTMLLabelElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLLabelElement_AddRef(IHTMLLabelElement *iface)
{
    HTMLLabelElement *This = impl_from_IHTMLLabelElement(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLLabelElement_Release(IHTMLLabelElement *iface)
{
    HTMLLabelElement *This = impl_from_IHTMLLabelElement(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLLabelElement_GetTypeInfoCount(IHTMLLabelElement *iface, UINT *pctinfo)
{
    HTMLLabelElement *This = impl_from_IHTMLLabelElement(iface);

    return IDispatchEx_GetTypeInfoCount(&This->element.node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLLabelElement_GetTypeInfo(IHTMLLabelElement *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLLabelElement *This = impl_from_IHTMLLabelElement(iface);

    return IDispatchEx_GetTypeInfo(&This->element.node.dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLLabelElement_GetIDsOfNames(IHTMLLabelElement *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLLabelElement *This = impl_from_IHTMLLabelElement(iface);

    return IDispatchEx_GetIDsOfNames(&This->element.node.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLLabelElement_Invoke(IHTMLLabelElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLLabelElement *This = impl_from_IHTMLLabelElement(iface);

    return IDispatchEx_Invoke(&This->element.node.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLLabelElement_put_htmlFor(IHTMLLabelElement *iface, BSTR v)
{
    HTMLLabelElement *This = impl_from_IHTMLLabelElement(iface);
    nsAString for_str, val_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&for_str, forW);
    nsAString_InitDepend(&val_str, v);
    nsres = nsIDOMHTMLElement_SetAttribute(This->element.nselem, &for_str, &val_str);
    nsAString_Finish(&for_str);
    nsAString_Finish(&val_str);
    if(NS_FAILED(nsres)) {
        ERR("SetAttribute failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLLabelElement_get_htmlFor(IHTMLLabelElement *iface, BSTR *p)
{
    HTMLLabelElement *This = impl_from_IHTMLLabelElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return elem_string_attr_getter(&This->element, forW, FALSE, p);
}

static HRESULT WINAPI HTMLLabelElement_put_accessKey(IHTMLLabelElement *iface, BSTR v)
{
    HTMLLabelElement *This = impl_from_IHTMLLabelElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLLabelElement_get_accessKey(IHTMLLabelElement *iface, BSTR *p)
{
    HTMLLabelElement *This = impl_from_IHTMLLabelElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLLabelElementVtbl HTMLLabelElementVtbl = {
    HTMLLabelElement_QueryInterface,
    HTMLLabelElement_AddRef,
    HTMLLabelElement_Release,
    HTMLLabelElement_GetTypeInfoCount,
    HTMLLabelElement_GetTypeInfo,
    HTMLLabelElement_GetIDsOfNames,
    HTMLLabelElement_Invoke,
    HTMLLabelElement_put_htmlFor,
    HTMLLabelElement_get_htmlFor,
    HTMLLabelElement_put_accessKey,
    HTMLLabelElement_get_accessKey
};

static inline HTMLLabelElement *label_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLLabelElement, element.node);
}

static HRESULT HTMLLabelElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLLabelElement *This = label_from_HTMLDOMNode(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IHTMLLabelElement_iface;
    }else if(IsEqualGUID(&IID_IHTMLLabelElement, riid)) {
        TRACE("(%p)->(IID_IHTMLLabelElement %p)\n", This, ppv);
        *ppv = &This->IHTMLLabelElement_iface;
    }else {
        return HTMLElement_QI(&This->element.node, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static const NodeImplVtbl HTMLLabelElementImplVtbl = {
    HTMLLabelElement_QI,
    HTMLElement_destructor,
    HTMLElement_cpc,
    HTMLElement_clone,
    HTMLElement_handle_event,
    HTMLElement_get_attr_col,
};

static const tid_t HTMLLabelElement_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLLabelElement_tid,
    0
};

static dispex_static_data_t HTMLLabelElement_dispex = {
    NULL,
    DispHTMLLabelElement_tid,
    NULL,
    HTMLLabelElement_iface_tids
};

HRESULT HTMLLabelElement_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem, HTMLElement **elem)
{
    HTMLLabelElement *ret;

    ret = heap_alloc_zero(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLLabelElement_iface.lpVtbl = &HTMLLabelElementVtbl;
    ret->element.node.vtbl = &HTMLLabelElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLLabelElement_dispex);
    *elem = &ret->element;
    return S_OK;
}

typedef struct {
    HTMLElement element;

    IHTMLButtonElement IHTMLButtonElement_iface;

    nsIDOMHTMLButtonElement *nsbutton;
} HTMLButtonElement;

static inline HTMLButtonElement *impl_from_IHTMLButtonElement(IHTMLButtonElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLButtonElement, IHTMLButtonElement_iface);
}

static HRESULT WINAPI HTMLButtonElement_QueryInterface(IHTMLButtonElement *iface,
                                                         REFIID riid, void **ppv)
{
    HTMLButtonElement *This = impl_from_IHTMLButtonElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLButtonElement_AddRef(IHTMLButtonElement *iface)
{
    HTMLButtonElement *This = impl_from_IHTMLButtonElement(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLButtonElement_Release(IHTMLButtonElement *iface)
{
    HTMLButtonElement *This = impl_from_IHTMLButtonElement(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLButtonElement_GetTypeInfoCount(IHTMLButtonElement *iface, UINT *pctinfo)
{
    HTMLButtonElement *This = impl_from_IHTMLButtonElement(iface);

    return IDispatchEx_GetTypeInfoCount(&This->element.node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLButtonElement_GetTypeInfo(IHTMLButtonElement *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLButtonElement *This = impl_from_IHTMLButtonElement(iface);

    return IDispatchEx_GetTypeInfo(&This->element.node.dispex.IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLButtonElement_GetIDsOfNames(IHTMLButtonElement *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLButtonElement *This = impl_from_IHTMLButtonElement(iface);

    return IDispatchEx_GetIDsOfNames(&This->element.node.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLButtonElement_Invoke(IHTMLButtonElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLButtonElement *This = impl_from_IHTMLButtonElement(iface);

    return IDispatchEx_Invoke(&This->element.node.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLButtonElement_get_type(IHTMLButtonElement *iface, BSTR *p)
{
    HTMLButtonElement *This = impl_from_IHTMLButtonElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLButtonElement_put_value(IHTMLButtonElement *iface, BSTR v)
{
    HTMLButtonElement *This = impl_from_IHTMLButtonElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLButtonElement_get_value(IHTMLButtonElement *iface, BSTR *p)
{
    HTMLButtonElement *This = impl_from_IHTMLButtonElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLButtonElement_put_name(IHTMLButtonElement *iface, BSTR v)
{
    HTMLButtonElement *This = impl_from_IHTMLButtonElement(iface);
    nsAString name_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    nsAString_InitDepend(&name_str, v);
    nsres = nsIDOMHTMLButtonElement_SetName(This->nsbutton, &name_str);
    nsAString_Finish(&name_str);
    if(NS_FAILED(nsres)) {
        ERR("SetName failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLButtonElement_get_name(IHTMLButtonElement *iface, BSTR *p)
{
    HTMLButtonElement *This = impl_from_IHTMLButtonElement(iface);
    nsAString name_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&name_str, NULL);
    nsres = nsIDOMHTMLButtonElement_GetName(This->nsbutton, &name_str);
    return return_nsstr(nsres, &name_str, p);
}

static HRESULT WINAPI HTMLButtonElement_put_status(IHTMLButtonElement *iface, VARIANT v)
{
    HTMLButtonElement *This = impl_from_IHTMLButtonElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLButtonElement_get_status(IHTMLButtonElement *iface, VARIANT *p)
{
    HTMLButtonElement *This = impl_from_IHTMLButtonElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLButtonElement_put_disabled(IHTMLButtonElement *iface, VARIANT_BOOL v)
{
    HTMLButtonElement *This = impl_from_IHTMLButtonElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%x)\n", This, v);

    nsres = nsIDOMHTMLButtonElement_SetDisabled(This->nsbutton, !!v);
    if(NS_FAILED(nsres)) {
        ERR("SetDisabled failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLButtonElement_get_disabled(IHTMLButtonElement *iface, VARIANT_BOOL *p)
{
    HTMLButtonElement *This = impl_from_IHTMLButtonElement(iface);
    cpp_bool disabled;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLButtonElement_GetDisabled(This->nsbutton, &disabled);
    if(NS_FAILED(nsres)) {
        ERR("GetDisabled failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = disabled ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI HTMLButtonElement_get_form(IHTMLButtonElement *iface, IHTMLFormElement **p)
{
    HTMLButtonElement *This = impl_from_IHTMLButtonElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLButtonElement_createTextRange(IHTMLButtonElement *iface, IHTMLTxtRange **range)
{
    HTMLButtonElement *This = impl_from_IHTMLButtonElement(iface);
    FIXME("(%p)->(%p)\n", This, range);
    return E_NOTIMPL;
}

static const IHTMLButtonElementVtbl HTMLButtonElementVtbl = {
    HTMLButtonElement_QueryInterface,
    HTMLButtonElement_AddRef,
    HTMLButtonElement_Release,
    HTMLButtonElement_GetTypeInfoCount,
    HTMLButtonElement_GetTypeInfo,
    HTMLButtonElement_GetIDsOfNames,
    HTMLButtonElement_Invoke,
    HTMLButtonElement_get_type,
    HTMLButtonElement_put_value,
    HTMLButtonElement_get_value,
    HTMLButtonElement_put_name,
    HTMLButtonElement_get_name,
    HTMLButtonElement_put_status,
    HTMLButtonElement_get_status,
    HTMLButtonElement_put_disabled,
    HTMLButtonElement_get_disabled,
    HTMLButtonElement_get_form,
    HTMLButtonElement_createTextRange
};

static inline HTMLButtonElement *button_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLButtonElement, element.node);
}

static HRESULT HTMLButtonElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLButtonElement *This = button_from_HTMLDOMNode(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IHTMLButtonElement_iface;
    }else if(IsEqualGUID(&IID_IHTMLButtonElement, riid)) {
        TRACE("(%p)->(IID_IHTMLButtonElement %p)\n", This, ppv);
        *ppv = &This->IHTMLButtonElement_iface;
    }else {
        return HTMLElement_QI(&This->element.node, riid, ppv);
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static HRESULT HTMLButtonElementImpl_put_disabled(HTMLDOMNode *iface, VARIANT_BOOL v)
{
    HTMLButtonElement *This = button_from_HTMLDOMNode(iface);
    return IHTMLButtonElement_put_disabled(&This->IHTMLButtonElement_iface, v);
}

static HRESULT HTMLButtonElementImpl_get_disabled(HTMLDOMNode *iface, VARIANT_BOOL *p)
{
    HTMLButtonElement *This = button_from_HTMLDOMNode(iface);
    return IHTMLButtonElement_get_disabled(&This->IHTMLButtonElement_iface, p);
}

static const NodeImplVtbl HTMLButtonElementImplVtbl = {
    HTMLButtonElement_QI,
    HTMLElement_destructor,
    HTMLElement_cpc,
    HTMLElement_clone,
    HTMLElement_handle_event,
    HTMLElement_get_attr_col,
    NULL,
    NULL,
    HTMLButtonElementImpl_put_disabled,
    HTMLButtonElementImpl_get_disabled
};

static const tid_t HTMLButtonElement_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLButtonElement_tid,
    0
};

static dispex_static_data_t HTMLButtonElement_dispex = {
    NULL,
    DispHTMLButtonElement_tid,
    NULL,
    HTMLButtonElement_iface_tids
};

HRESULT HTMLButtonElement_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem, HTMLElement **elem)
{
    HTMLButtonElement *ret;
    nsresult nsres;

    ret = heap_alloc_zero(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLButtonElement_iface.lpVtbl = &HTMLButtonElementVtbl;
    ret->element.node.vtbl = &HTMLButtonElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLButtonElement_dispex);

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLButtonElement, (void**)&ret->nsbutton);

    /* Share nsbutton reference with nsnode */
    assert(nsres == NS_OK && (nsIDOMNode*)ret->nsbutton == ret->element.node.nsnode);
    nsIDOMNode_Release(ret->element.node.nsnode);

    *elem = &ret->element;
    return S_OK;
}
