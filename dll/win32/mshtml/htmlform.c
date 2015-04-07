/*
 * Copyright 2009 Andrew Eikum for CodeWeavers
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

struct HTMLFormElement {
    HTMLElement element;

    IHTMLFormElement IHTMLFormElement_iface;

    nsIDOMHTMLFormElement *nsform;
};

static HRESULT htmlform_item(HTMLFormElement *This, int i, IDispatch **ret)
{
    nsIDOMHTMLCollection *elements;
    nsIDOMNode *item;
    HTMLDOMNode *node;
    nsresult nsres;
    HRESULT hres;

    nsres = nsIDOMHTMLFormElement_GetElements(This->nsform, &elements);
    if(NS_FAILED(nsres)) {
        FIXME("GetElements failed: 0x%08x\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMHTMLCollection_Item(elements, i, &item);
    nsIDOMHTMLCollection_Release(elements);
    if(NS_FAILED(nsres)) {
        FIXME("Item failed: 0x%08x\n", nsres);
        return E_FAIL;
    }

    if(item) {
        hres = get_node(This->element.node.doc, item, TRUE, &node);
        if(FAILED(hres))
            return hres;

        nsIDOMNode_Release(item);
        *ret = (IDispatch*)&node->IHTMLDOMNode_iface;
    }else {
        *ret = NULL;
    }

    return S_OK;
}

static inline HTMLFormElement *impl_from_IHTMLFormElement(IHTMLFormElement *iface)
{
    return CONTAINING_RECORD(iface, HTMLFormElement, IHTMLFormElement_iface);
}

static HRESULT WINAPI HTMLFormElement_QueryInterface(IHTMLFormElement *iface,
        REFIID riid, void **ppv)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);

    return IHTMLDOMNode_QueryInterface(&This->element.node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI HTMLFormElement_AddRef(IHTMLFormElement *iface)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);

    return IHTMLDOMNode_AddRef(&This->element.node.IHTMLDOMNode_iface);
}

static ULONG WINAPI HTMLFormElement_Release(IHTMLFormElement *iface)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);

    return IHTMLDOMNode_Release(&This->element.node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI HTMLFormElement_GetTypeInfoCount(IHTMLFormElement *iface, UINT *pctinfo)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    return IDispatchEx_GetTypeInfoCount(&This->element.node.dispex.IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLFormElement_GetTypeInfo(IHTMLFormElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    return IDispatchEx_GetTypeInfo(&This->element.node.dispex.IDispatchEx_iface, iTInfo, lcid,
            ppTInfo);
}

static HRESULT WINAPI HTMLFormElement_GetIDsOfNames(IHTMLFormElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    return IDispatchEx_GetIDsOfNames(&This->element.node.dispex.IDispatchEx_iface, riid, rgszNames,
            cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLFormElement_Invoke(IHTMLFormElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    return IDispatchEx_Invoke(&This->element.node.dispex.IDispatchEx_iface, dispIdMember, riid,
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLFormElement_put_action(IHTMLFormElement *iface, BSTR v)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    nsAString action_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, wine_dbgstr_w(v));

    nsAString_InitDepend(&action_str, v);
    nsres = nsIDOMHTMLFormElement_SetAction(This->nsform, &action_str);
    nsAString_Finish(&action_str);
    if(NS_FAILED(nsres)) {
        ERR("SetAction failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLFormElement_get_action(IHTMLFormElement *iface, BSTR *p)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    nsAString action_str;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&action_str, NULL);
    nsres = nsIDOMHTMLFormElement_GetAction(This->nsform, &action_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *action;
        nsAString_GetData(&action_str, &action);
        hres = nsuri_to_url(action, FALSE, p);
    }else {
        ERR("GetAction failed: %08x\n", nsres);
        hres = E_FAIL;
    }

    nsAString_Finish(&action_str);
    return hres;
}

static HRESULT WINAPI HTMLFormElement_put_dir(IHTMLFormElement *iface, BSTR v)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    FIXME("(%p)->(%s)\n", This, wine_dbgstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_get_dir(IHTMLFormElement *iface, BSTR *p)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_put_encoding(IHTMLFormElement *iface, BSTR v)
{
    static const WCHAR urlencodedW[] = {'a','p','p','l','i','c','a','t','i','o','n','/',
        'x','-','w','w','w','-','f','o','r','m','-','u','r','l','e','n','c','o','d','e','d',0};
    static const WCHAR dataW[] = {'m','u','l','t','i','p','a','r','t','/',
        'f','o','r','m','-','d','a','t','a',0};
    static const WCHAR plainW[] = {'t','e','x','t','/','p','l','a','i','n',0};

    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    nsAString encoding_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, wine_dbgstr_w(v));

    if(lstrcmpiW(v, urlencodedW) && lstrcmpiW(v, dataW) && lstrcmpiW(v, plainW)) {
        WARN("incorrect enctype\n");
        return E_INVALIDARG;
    }

    nsAString_InitDepend(&encoding_str, v);
    nsres = nsIDOMHTMLFormElement_SetEnctype(This->nsform, &encoding_str);
    nsAString_Finish(&encoding_str);
    if(NS_FAILED(nsres))
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI HTMLFormElement_get_encoding(IHTMLFormElement *iface, BSTR *p)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    nsAString encoding_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&encoding_str, NULL);
    nsres = nsIDOMHTMLFormElement_GetEnctype(This->nsform, &encoding_str);
    return return_nsstr(nsres, &encoding_str, p);
}

static HRESULT WINAPI HTMLFormElement_put_method(IHTMLFormElement *iface, BSTR v)
{
    static const WCHAR postW[] = {'P','O','S','T',0};
    static const WCHAR getW[] = {'G','E','T',0};

    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    nsAString method_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, wine_dbgstr_w(v));

    if(lstrcmpiW(v, postW) && lstrcmpiW(v, getW)) {
        WARN("unrecognized method\n");
        return E_INVALIDARG;
    }

    nsAString_InitDepend(&method_str, v);
    nsres = nsIDOMHTMLFormElement_SetMethod(This->nsform, &method_str);
    nsAString_Finish(&method_str);
    if(NS_FAILED(nsres))
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI HTMLFormElement_get_method(IHTMLFormElement *iface, BSTR *p)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    nsAString method_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&method_str, NULL);
    nsres = nsIDOMHTMLFormElement_GetMethod(This->nsform, &method_str);
    return return_nsstr(nsres, &method_str, p);
}

static HRESULT WINAPI HTMLFormElement_get_elements(IHTMLFormElement *iface, IDispatch **p)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = (IDispatch*)&This->IHTMLFormElement_iface;
    IDispatch_AddRef(*p);
    return S_OK;
}

static HRESULT WINAPI HTMLFormElement_put_target(IHTMLFormElement *iface, BSTR v)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    nsAString str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, wine_dbgstr_w(v));

    nsAString_InitDepend(&str, v);

    nsres = nsIDOMHTMLFormElement_SetTarget(This->nsform, &str);

    nsAString_Finish(&str);
    if (NS_FAILED(nsres)) {
        ERR("Set Target(%s) failed: %08x\n", wine_dbgstr_w(v), nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLFormElement_get_target(IHTMLFormElement *iface, BSTR *p)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    nsAString str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&str, NULL);
    nsres = nsIDOMHTMLFormElement_GetTarget(This->nsform, &str);

    return return_nsstr(nsres, &str, p);
}

static HRESULT WINAPI HTMLFormElement_put_name(IHTMLFormElement *iface, BSTR v)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    nsAString name_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, wine_dbgstr_w(v));

    nsAString_InitDepend(&name_str, v);
    nsres = nsIDOMHTMLFormElement_SetName(This->nsform, &name_str);
    nsAString_Finish(&name_str);
    if(NS_FAILED(nsres))
        return E_FAIL;

    return S_OK;
}

static HRESULT WINAPI HTMLFormElement_get_name(IHTMLFormElement *iface, BSTR *p)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    nsAString name_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&name_str, NULL);
    nsres = nsIDOMHTMLFormElement_GetName(This->nsform, &name_str);
    return return_nsstr(nsres, &name_str, p);
}

static HRESULT WINAPI HTMLFormElement_put_onsubmit(IHTMLFormElement *iface, VARIANT v)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_node_event(&This->element.node, EVENTID_SUBMIT, &v);
}

static HRESULT WINAPI HTMLFormElement_get_onsubmit(IHTMLFormElement *iface, VARIANT *p)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_node_event(&This->element.node, EVENTID_SUBMIT, p);
}

static HRESULT WINAPI HTMLFormElement_put_onreset(IHTMLFormElement *iface, VARIANT v)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_get_onreset(IHTMLFormElement *iface, VARIANT *p)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_submit(IHTMLFormElement *iface)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    HTMLOuterWindow *window = NULL, *this_window = NULL;
    nsAString action_uri_str, target_str, method_str;
    nsIInputStream *post_stream;
    BOOL is_post_submit = FALSE;
    IUri *uri;
    nsresult nsres;
    HRESULT hres;
    BOOL use_new_window = FALSE;

    TRACE("(%p)\n", This);

    if(This->element.node.doc) {
        HTMLDocumentNode *doc = This->element.node.doc;
        if(doc->window && doc->window->base.outer_window)
            this_window = doc->window->base.outer_window;
    }
    if(!this_window) {
        TRACE("No outer window\n");
        return S_OK;
    }

    nsAString_Init(&target_str, NULL);
    nsres = nsIDOMHTMLFormElement_GetTarget(This->nsform, &target_str);
    if(NS_SUCCEEDED(nsres))
        window = get_target_window(this_window, &target_str, &use_new_window);

    if(!window && !use_new_window) {
        nsAString_Finish(&target_str);
        return S_OK;
    }

    nsAString_Init(&method_str, NULL);
    nsres = nsIDOMHTMLFormElement_GetMethod(This->nsform, &method_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *method;

        static const PRUnichar postW[] = {'p','o','s','t',0};

        nsAString_GetData(&method_str, &method);
        TRACE("method is %s\n", debugstr_w(method));
        is_post_submit = !strcmpiW(method, postW);
    }
    nsAString_Finish(&method_str);

    /*
     * FIXME: We currently use our submit implementation for POST submit. We should always use it.
     */
    if(window && !is_post_submit) {
        nsres = nsIDOMHTMLFormElement_Submit(This->nsform);
        nsAString_Finish(&target_str);
        IHTMLWindow2_Release(&window->base.IHTMLWindow2_iface);
        if(NS_FAILED(nsres)) {
            ERR("Submit failed: %08x\n", nsres);
            return E_FAIL;
        }

        return S_OK;
    }

    nsAString_Init(&action_uri_str, NULL);
    nsres = nsIDOMHTMLFormElement_GetFormData(This->nsform, NULL, &action_uri_str, &post_stream);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *action_uri;

        nsAString_GetData(&action_uri_str, &action_uri);
        hres = create_uri(action_uri, 0, &uri);
    }else {
        ERR("GetFormData failed: %08x\n", nsres);
        hres = E_FAIL;
    }
    nsAString_Finish(&action_uri_str);
    if(SUCCEEDED(hres)) {
        const PRUnichar *target;

        nsAString_GetData(&target_str, &target);
        hres = submit_form(window, target, uri, post_stream);
        IUri_Release(uri);
    }

    nsAString_Finish(&target_str);
    if(window)
        IHTMLWindow2_Release(&window->base.IHTMLWindow2_iface);
    if(post_stream)
        nsIInputStream_Release(post_stream);
    return hres;
}

static HRESULT WINAPI HTMLFormElement_reset(IHTMLFormElement *iface)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    nsresult nsres;

    TRACE("(%p)->()\n", This);
    nsres = nsIDOMHTMLFormElement_Reset(This->nsform);
    if (NS_FAILED(nsres)) {
        ERR("Reset failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLFormElement_put_length(IHTMLFormElement *iface, LONG v)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_get_length(IHTMLFormElement *iface, LONG *p)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsres = nsIDOMHTMLFormElement_GetLength(This->nsform, p);
    if(NS_FAILED(nsres)) {
        ERR("GetLength failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLFormElement__newEnum(IHTMLFormElement *iface, IUnknown **p)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_item(IHTMLFormElement *iface, VARIANT name,
        VARIANT index, IDispatch **pdisp)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_variant(&name), debugstr_variant(&index), pdisp);

    if(!pdisp)
        return E_INVALIDARG;
    *pdisp = NULL;

    if(V_VT(&name) == VT_I4) {
        if(V_I4(&name) < 0)
            return E_INVALIDARG;
        return htmlform_item(This, V_I4(&name), pdisp);
    }

    FIXME("Unsupported args\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_tags(IHTMLFormElement *iface, VARIANT tagName,
        IDispatch **pdisp)
{
    HTMLFormElement *This = impl_from_IHTMLFormElement(iface);
    FIXME("(%p)->(v %p)\n", This, pdisp);
    return E_NOTIMPL;
}

static const IHTMLFormElementVtbl HTMLFormElementVtbl = {
    HTMLFormElement_QueryInterface,
    HTMLFormElement_AddRef,
    HTMLFormElement_Release,
    HTMLFormElement_GetTypeInfoCount,
    HTMLFormElement_GetTypeInfo,
    HTMLFormElement_GetIDsOfNames,
    HTMLFormElement_Invoke,
    HTMLFormElement_put_action,
    HTMLFormElement_get_action,
    HTMLFormElement_put_dir,
    HTMLFormElement_get_dir,
    HTMLFormElement_put_encoding,
    HTMLFormElement_get_encoding,
    HTMLFormElement_put_method,
    HTMLFormElement_get_method,
    HTMLFormElement_get_elements,
    HTMLFormElement_put_target,
    HTMLFormElement_get_target,
    HTMLFormElement_put_name,
    HTMLFormElement_get_name,
    HTMLFormElement_put_onsubmit,
    HTMLFormElement_get_onsubmit,
    HTMLFormElement_put_onreset,
    HTMLFormElement_get_onreset,
    HTMLFormElement_submit,
    HTMLFormElement_reset,
    HTMLFormElement_put_length,
    HTMLFormElement_get_length,
    HTMLFormElement__newEnum,
    HTMLFormElement_item,
    HTMLFormElement_tags
};

static inline HTMLFormElement *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLFormElement, element.node);
}

static HRESULT HTMLFormElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLFormElement *This = impl_from_HTMLDOMNode(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IHTMLFormElement_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IHTMLFormElement_iface;
    }else if(IsEqualGUID(&IID_IHTMLFormElement, riid)) {
        TRACE("(%p)->(IID_IHTMLFormElement %p)\n", This, ppv);
        *ppv = &This->IHTMLFormElement_iface;
    }else if(IsEqualGUID(&DIID_DispHTMLFormElement, riid)) {
        TRACE("(%p)->(DIID_DispHTMLFormElement %p)\n", This, ppv);
        *ppv = &This->IHTMLFormElement_iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    return HTMLElement_QI(&This->element.node, riid, ppv);
}

static HRESULT HTMLFormElement_get_dispid(HTMLDOMNode *iface,
        BSTR name, DWORD grfdex, DISPID *pid)
{
    HTMLFormElement *This = impl_from_HTMLDOMNode(iface);
    nsIDOMHTMLCollection *elements;
    nsAString nsstr, name_str;
    UINT32 len, i;
    nsresult nsres;
    HRESULT hres = DISP_E_UNKNOWNNAME;

    static const PRUnichar nameW[] = {'n','a','m','e',0};

    TRACE("(%p)->(%s %x %p)\n", This, wine_dbgstr_w(name), grfdex, pid);

    nsres = nsIDOMHTMLFormElement_GetElements(This->nsform, &elements);
    if(NS_FAILED(nsres)) {
        FIXME("GetElements failed: 0x%08x\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMHTMLCollection_GetLength(elements, &len);
    if(NS_FAILED(nsres)) {
        FIXME("GetLength failed: 0x%08x\n", nsres);
        nsIDOMHTMLCollection_Release(elements);
        return E_FAIL;
    }

    if(len > MSHTML_CUSTOM_DISPID_CNT)
        len = MSHTML_CUSTOM_DISPID_CNT;

    /* FIXME: Implement in more generic way */
    if('0' <= *name && *name <= '9') {
        WCHAR *end_ptr;

        i = strtoulW(name, &end_ptr, 10);
        if(!*end_ptr && i < len) {
            *pid = MSHTML_DISPID_CUSTOM_MIN + i;
            return S_OK;
        }
    }

    nsAString_Init(&nsstr, NULL);
    for(i = 0; i < len; ++i) {
        nsIDOMNode *nsitem;
        nsIDOMHTMLElement *nshtml_elem;
        const PRUnichar *str;

        nsres = nsIDOMHTMLCollection_Item(elements, i, &nsitem);
        if(NS_FAILED(nsres)) {
            FIXME("Item failed: 0x%08x\n", nsres);
            hres = E_FAIL;
            break;
        }

        nsres = nsIDOMNode_QueryInterface(nsitem, &IID_nsIDOMHTMLElement, (void**)&nshtml_elem);
        nsIDOMNode_Release(nsitem);
        if(NS_FAILED(nsres)) {
            FIXME("Failed to get nsIDOMHTMLNode interface: 0x%08x\n", nsres);
            hres = E_FAIL;
            break;
        }

        /* compare by id attr */
        nsres = nsIDOMHTMLElement_GetId(nshtml_elem, &nsstr);
        if(NS_FAILED(nsres)) {
            FIXME("GetId failed: 0x%08x\n", nsres);
            nsIDOMHTMLElement_Release(nshtml_elem);
            hres = E_FAIL;
            break;
        }
        nsAString_GetData(&nsstr, &str);
        if(!strcmpiW(str, name)) {
            nsIDOMHTMLElement_Release(nshtml_elem);
            /* FIXME: using index for dispid */
            *pid = MSHTML_DISPID_CUSTOM_MIN + i;
            hres = S_OK;
            break;
        }

        /* compare by name attr */
        nsres = get_elem_attr_value(nshtml_elem, nameW, &name_str, &str);
        nsIDOMHTMLElement_Release(nshtml_elem);
        if(NS_SUCCEEDED(nsres)) {
            if(!strcmpiW(str, name)) {
                nsAString_Finish(&name_str);
                /* FIXME: using index for dispid */
                *pid = MSHTML_DISPID_CUSTOM_MIN + i;
                hres = S_OK;
                break;
            }
            nsAString_Finish(&name_str);
        }
    }

    nsAString_Finish(&nsstr);
    nsIDOMHTMLCollection_Release(elements);
    return hres;
}

static HRESULT HTMLFormElement_invoke(HTMLDOMNode *iface,
        DISPID id, LCID lcid, WORD flags, DISPPARAMS *params, VARIANT *res,
        EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLFormElement *This = impl_from_HTMLDOMNode(iface);
    IDispatch *ret;
    HRESULT hres;

    TRACE("(%p)->(%x %x %x %p %p %p %p)\n", This, id, lcid, flags, params, res, ei, caller);

    hres = htmlform_item(This, id - MSHTML_DISPID_CUSTOM_MIN, &ret);
    if(FAILED(hres))
        return hres;

    if(ret) {
        V_VT(res) = VT_DISPATCH;
        V_DISPATCH(res) = ret;
    }else {
        V_VT(res) = VT_NULL;
    }
    return S_OK;
}

static HRESULT HTMLFormElement_handle_event(HTMLDOMNode *iface, eventid_t eid, nsIDOMEvent *event, BOOL *prevent_default)
{
    HTMLFormElement *This = impl_from_HTMLDOMNode(iface);

    if(eid == EVENTID_SUBMIT) {
        *prevent_default = TRUE;
        return IHTMLFormElement_submit(&This->IHTMLFormElement_iface);
    }

    return HTMLElement_handle_event(&This->element.node, eid, event, prevent_default);
}

static void HTMLFormElement_traverse(HTMLDOMNode *iface, nsCycleCollectionTraversalCallback *cb)
{
    HTMLFormElement *This = impl_from_HTMLDOMNode(iface);

    if(This->nsform)
        note_cc_edge((nsISupports*)This->nsform, "This->nsform", cb);
}

static void HTMLFormElement_unlink(HTMLDOMNode *iface)
{
    HTMLFormElement *This = impl_from_HTMLDOMNode(iface);

    if(This->nsform) {
        nsIDOMHTMLFormElement *nsform = This->nsform;

        This->nsform = NULL;
        nsIDOMHTMLFormElement_Release(nsform);
    }
}

static const NodeImplVtbl HTMLFormElementImplVtbl = {
    HTMLFormElement_QI,
    HTMLElement_destructor,
    HTMLElement_cpc,
    HTMLElement_clone,
    HTMLFormElement_handle_event,
    HTMLElement_get_attr_col,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    HTMLFormElement_get_dispid,
    HTMLFormElement_invoke,
    NULL,
    HTMLFormElement_traverse,
    HTMLFormElement_unlink
};

static const tid_t HTMLFormElement_iface_tids[] = {
    HTMLELEMENT_TIDS,
    IHTMLFormElement_tid,
    0
};

static dispex_static_data_t HTMLFormElement_dispex = {
    NULL,
    DispHTMLFormElement_tid,
    NULL,
    HTMLFormElement_iface_tids
};

HRESULT HTMLFormElement_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem, HTMLElement **elem)
{
    HTMLFormElement *ret;
    nsresult nsres;

    ret = heap_alloc_zero(sizeof(HTMLFormElement));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLFormElement_iface.lpVtbl = &HTMLFormElementVtbl;
    ret->element.node.vtbl = &HTMLFormElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLFormElement_dispex);

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLFormElement, (void**)&ret->nsform);
    assert(nsres == NS_OK);

    *elem = &ret->element;
    return S_OK;
}
