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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

struct HTMLFormElement {
    HTMLElement element;

    const IHTMLFormElementVtbl *lpHTMLFormElementVtbl;

    nsIDOMHTMLFormElement *nsform;
};

#define HTMLFORM(x)  (&(x)->lpHTMLFormElementVtbl)

#define HTMLFORM_THIS(iface) DEFINE_THIS(HTMLFormElement, HTMLFormElement, iface)

static HRESULT WINAPI HTMLFormElement_QueryInterface(IHTMLFormElement *iface,
        REFIID riid, void **ppv)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);

    return IHTMLDOMNode_QueryInterface(HTMLDOMNODE(&This->element.node), riid, ppv);
}

static ULONG WINAPI HTMLFormElement_AddRef(IHTMLFormElement *iface)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);

    return IHTMLDOMNode_AddRef(HTMLDOMNODE(&This->element.node));
}

static ULONG WINAPI HTMLFormElement_Release(IHTMLFormElement *iface)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);

    return IHTMLDOMNode_Release(HTMLDOMNODE(&This->element.node));
}

static HRESULT WINAPI HTMLFormElement_GetTypeInfoCount(IHTMLFormElement *iface, UINT *pctinfo)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);
    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(&This->element.node.dispex), pctinfo);
}

static HRESULT WINAPI HTMLFormElement_GetTypeInfo(IHTMLFormElement *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);
    return IDispatchEx_GetTypeInfo(DISPATCHEX(&This->element.node.dispex), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLFormElement_GetIDsOfNames(IHTMLFormElement *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);
    return IDispatchEx_GetIDsOfNames(DISPATCHEX(&This->element.node.dispex), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLFormElement_Invoke(IHTMLFormElement *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);
    return IDispatchEx_Invoke(DISPATCHEX(&This->element.node.dispex), dispIdMember, riid, lcid, wFlags, pDispParams,
            pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLFormElement_put_action(IHTMLFormElement *iface, BSTR v)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);
    FIXME("(%p)->(%s)\n", This, wine_dbgstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_get_action(IHTMLFormElement *iface, BSTR *p)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_put_dir(IHTMLFormElement *iface, BSTR v)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);
    FIXME("(%p)->(%s)\n", This, wine_dbgstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_get_dir(IHTMLFormElement *iface, BSTR *p)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_put_encoding(IHTMLFormElement *iface, BSTR v)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);
    FIXME("(%p)->(%s)\n", This, wine_dbgstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_get_encoding(IHTMLFormElement *iface, BSTR *p)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_put_method(IHTMLFormElement *iface, BSTR v)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);
    FIXME("(%p)->(%s)\n", This, wine_dbgstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_get_method(IHTMLFormElement *iface, BSTR *p)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_get_elements(IHTMLFormElement *iface, IDispatch **p)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_put_target(IHTMLFormElement *iface, BSTR v)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);
    FIXME("(%p)->(%s)\n", This, wine_dbgstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_get_target(IHTMLFormElement *iface, BSTR *p)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_put_name(IHTMLFormElement *iface, BSTR v)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);
    FIXME("(%p)->(%s)\n", This, wine_dbgstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_get_name(IHTMLFormElement *iface, BSTR *p)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_put_onsubmit(IHTMLFormElement *iface, VARIANT v)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);
    FIXME("(%p)->(v)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_get_onsubmit(IHTMLFormElement *iface, VARIANT *p)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_put_onreset(IHTMLFormElement *iface, VARIANT v)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);
    FIXME("(%p)->(v)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_get_onreset(IHTMLFormElement *iface, VARIANT *p)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_submit(IHTMLFormElement *iface)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_reset(IHTMLFormElement *iface)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_put_length(IHTMLFormElement *iface, LONG v)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_get_length(IHTMLFormElement *iface, LONG *p)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement__newEnum(IHTMLFormElement *iface, IUnknown **p)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_item(IHTMLFormElement *iface, VARIANT name,
        VARIANT index, IDispatch **pdisp)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);
    FIXME("(%p)->(v v %p)\n", This, pdisp);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLFormElement_tags(IHTMLFormElement *iface, VARIANT tagName,
        IDispatch **pdisp)
{
    HTMLFormElement *This = HTMLFORM_THIS(iface);
    FIXME("(%p)->(v %p)\n", This, pdisp);
    return E_NOTIMPL;
}

#undef HTMLFORM_THIS

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

#define HTMLFORM_NODE_THIS(iface) DEFINE_THIS2(HTMLFormElement, element.node, iface)

static HRESULT HTMLFormElement_QI(HTMLDOMNode *iface, REFIID riid, void **ppv)
{
    HTMLFormElement *This = HTMLFORM_NODE_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLFORM(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = HTMLFORM(This);
    }else if(IsEqualGUID(&IID_IHTMLFormElement, riid)) {
        TRACE("(%p)->(IID_IHTMLFormElement %p)\n", This, ppv);
        *ppv = HTMLFORM(This);
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    return HTMLElement_QI(&This->element.node, riid, ppv);
}

static void HTMLFormElement_destructor(HTMLDOMNode *iface)
{
    HTMLFormElement *This = HTMLFORM_NODE_THIS(iface);

    if(This->nsform)
        nsIDOMHTMLFormElement_Release(This->nsform);

    HTMLElement_destructor(&This->element.node);
}

static HRESULT HTMLFormElement_get_dispid(HTMLDOMNode *iface,
        BSTR name, DWORD grfdex, DISPID *pid)
{
    HTMLFormElement *This = HTMLFORM_NODE_THIS(iface);
    nsIDOMHTMLCollection *elements;
    nsAString nsname, nsstr;
    PRUint32 len, i;
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

    nsAString_InitDepend(&nsname, nameW);
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
        nsres = nsIDOMHTMLElement_GetAttribute(nshtml_elem, &nsname, &nsstr);
        nsIDOMHTMLElement_Release(nshtml_elem);
        nsAString_GetData(&nsstr, &str);
        if(!strcmpiW(str, name)) {
            /* FIXME: using index for dispid */
            *pid = MSHTML_DISPID_CUSTOM_MIN + i;
            hres = S_OK;
            break;
        }
    }
    nsAString_Finish(&nsname);
    nsAString_Finish(&nsstr);

    nsIDOMHTMLCollection_Release(elements);

    return hres;
}

static HRESULT HTMLFormElement_invoke(HTMLDOMNode *iface,
        DISPID id, LCID lcid, WORD flags, DISPPARAMS *params, VARIANT *res,
        EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLFormElement *This = HTMLFORM_NODE_THIS(iface);
    nsIDOMHTMLCollection *elements;
    nsIDOMNode *item;
    HTMLDOMNode *node;
    nsresult nsres;

    TRACE("(%p)->(%x %x %x %p %p %p %p)\n", This, id, lcid, flags, params, res, ei, caller);

    nsres = nsIDOMHTMLFormElement_GetElements(This->nsform, &elements);
    if(NS_FAILED(nsres)) {
        FIXME("GetElements failed: 0x%08x\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMHTMLCollection_Item(elements, id - MSHTML_DISPID_CUSTOM_MIN, &item);
    nsIDOMHTMLCollection_Release(elements);
    if(NS_FAILED(nsres)) {
        FIXME("Item failed: 0x%08x\n", nsres);
        return E_FAIL;
    }

    node = get_node(This->element.node.doc, item, TRUE);

    V_VT(res) = VT_DISPATCH;
    V_DISPATCH(res) = (IDispatch*)node;

    IHTMLDOMNode_AddRef(HTMLDOMNODE(node));
    nsIDOMNode_Release(item);

    return S_OK;
}

#undef HTMLFORM_NODE_THIS

static const NodeImplVtbl HTMLFormElementImplVtbl = {
    HTMLFormElement_QI,
    HTMLFormElement_destructor,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    HTMLFormElement_get_dispid,
    HTMLFormElement_invoke
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

HTMLElement *HTMLFormElement_Create(HTMLDocumentNode *doc, nsIDOMHTMLElement *nselem)
{
    HTMLFormElement *ret = heap_alloc_zero(sizeof(HTMLFormElement));
    nsresult nsres;

    ret->lpHTMLFormElementVtbl = &HTMLFormElementVtbl;
    ret->element.node.vtbl = &HTMLFormElementImplVtbl;

    HTMLElement_Init(&ret->element, doc, nselem, &HTMLFormElement_dispex);

    nsres = nsIDOMHTMLElement_QueryInterface(nselem, &IID_nsIDOMHTMLFormElement, (void**)&ret->nsform);
    if(NS_FAILED(nsres))
        ERR("Could not get nsIDOMHTMLFormElement interface: %08x\n", nsres);

    return &ret->element;
}
