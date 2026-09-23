/*
 * Copyright 2011 Jacek Caban for CodeWeavers
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

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

static inline HTMLDOMAttribute *impl_from_IHTMLDOMAttribute(IHTMLDOMAttribute *iface)
{
    return CONTAINING_RECORD(iface, HTMLDOMAttribute, IHTMLDOMAttribute_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLDOMAttribute, IHTMLDOMAttribute, impl_from_IHTMLDOMAttribute(iface)->dispex)

static HRESULT WINAPI HTMLDOMAttribute_get_nodeName(IHTMLDOMAttribute *iface, BSTR *p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->elem) {
        if(!This->name) {
            FIXME("No name available\n");
            return E_FAIL;
        }

        *p = SysAllocString(This->name);
        return *p ? S_OK : E_OUTOFMEMORY;
    }

    return dispex_prop_name(&This->elem->node.event_target.dispex, This->dispid, p);
}

static HRESULT WINAPI HTMLDOMAttribute_put_nodeValue(IHTMLDOMAttribute *iface, VARIANT v)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute(iface);
    EXCEPINFO ei;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    if(!This->elem)
        return VariantCopy(&This->value, &v);

    memset(&ei, 0, sizeof(ei));
    return dispex_prop_put(&This->elem->node.event_target.dispex, This->dispid, LOCALE_SYSTEM_DEFAULT, &v, &ei, NULL);
}

static HRESULT WINAPI HTMLDOMAttribute_get_nodeValue(IHTMLDOMAttribute *iface, VARIANT *p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->elem)
        return VariantCopy(p, &This->value);

    return get_elem_attr_value_by_dispid(This->elem, This->dispid, p);
}

static HRESULT WINAPI HTMLDOMAttribute_get_specified(IHTMLDOMAttribute *iface, VARIANT_BOOL *p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute(iface);
    nsIDOMAttr *nsattr;
    nsAString nsname;
    BSTR name;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->elem || !This->elem->dom_element) {
        FIXME("NULL This->elem\n");
        return E_UNEXPECTED;
    }

    if(get_dispid_type(This->dispid) != DISPEXPROP_BUILTIN) {
        *p = VARIANT_TRUE;
        return S_OK;
    }

    hres = dispex_prop_name(&This->elem->node.event_target.dispex, This->dispid, &name);
    if(FAILED(hres))
        return hres;

    /* FIXME: This is not exactly right, we have some attributes that don't map directly to Gecko attributes. */
    nsAString_InitDepend(&nsname, name);
    nsres = nsIDOMElement_GetAttributeNode(This->elem->dom_element, &nsname, &nsattr);
    nsAString_Finish(&nsname);
    SysFreeString(name);
    if(NS_FAILED(nsres))
        return E_FAIL;

    /* If the Gecko attribute node can be found, we know that the attribute is specified.
       There is no point in calling GetSpecified */
    if(nsattr) {
        nsIDOMAttr_Release(nsattr);
        *p = VARIANT_TRUE;
    }else {
        *p = VARIANT_FALSE;
    }
    return S_OK;
}

static const IHTMLDOMAttributeVtbl HTMLDOMAttributeVtbl = {
    HTMLDOMAttribute_QueryInterface,
    HTMLDOMAttribute_AddRef,
    HTMLDOMAttribute_Release,
    HTMLDOMAttribute_GetTypeInfoCount,
    HTMLDOMAttribute_GetTypeInfo,
    HTMLDOMAttribute_GetIDsOfNames,
    HTMLDOMAttribute_Invoke,
    HTMLDOMAttribute_get_nodeName,
    HTMLDOMAttribute_put_nodeValue,
    HTMLDOMAttribute_get_nodeValue,
    HTMLDOMAttribute_get_specified
};

static inline HTMLDOMAttribute *impl_from_IHTMLDOMAttribute2(IHTMLDOMAttribute2 *iface)
{
    return CONTAINING_RECORD(iface, HTMLDOMAttribute, IHTMLDOMAttribute2_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLDOMAttribute2, IHTMLDOMAttribute2, impl_from_IHTMLDOMAttribute2(iface)->dispex)

static HRESULT WINAPI HTMLDOMAttribute2_get_name(IHTMLDOMAttribute2 *iface, BSTR *p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLDOMAttribute_get_nodeName(&This->IHTMLDOMAttribute_iface, p);
}

static HRESULT WINAPI HTMLDOMAttribute2_put_value(IHTMLDOMAttribute2 *iface, BSTR v)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);
    VARIANT var;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = v;
    return IHTMLDOMAttribute_put_nodeValue(&This->IHTMLDOMAttribute_iface, var);
}

static HRESULT WINAPI HTMLDOMAttribute2_get_value(IHTMLDOMAttribute2 *iface, BSTR *p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);
    VARIANT val;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    V_VT(&val) = VT_EMPTY;
    if(This->elem)
        hres = get_elem_attr_value_by_dispid(This->elem, This->dispid, &val);
    else
        hres = VariantCopy(&val, &This->value);
    if(SUCCEEDED(hres))
        hres = attr_value_to_string(&val);
    if(FAILED(hres))
        return hres;

    assert(V_VT(&val) == VT_BSTR);
    *p = V_BSTR(&val);
    if(!*p && !(*p = SysAllocStringLen(NULL, 0)))
        return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT WINAPI HTMLDOMAttribute2_get_expando(IHTMLDOMAttribute2 *iface, VARIANT_BOOL *p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = variant_bool(This->elem && get_dispid_type(This->dispid) != DISPEXPROP_BUILTIN);
    return S_OK;
}

static HRESULT WINAPI HTMLDOMAttribute2_get_nodeType(IHTMLDOMAttribute2 *iface, LONG *p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMAttribute2_get_parentNode(IHTMLDOMAttribute2 *iface, IHTMLDOMNode **p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = NULL;
    return S_OK;
}

static HRESULT WINAPI HTMLDOMAttribute2_get_childNodes(IHTMLDOMAttribute2 *iface, IDispatch **p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMAttribute2_get_firstChild(IHTMLDOMAttribute2 *iface, IHTMLDOMNode **p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMAttribute2_get_lastChild(IHTMLDOMAttribute2 *iface, IHTMLDOMNode **p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMAttribute2_get_previousSibling(IHTMLDOMAttribute2 *iface, IHTMLDOMNode **p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMAttribute2_get_nextSibling(IHTMLDOMAttribute2 *iface, IHTMLDOMNode **p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMAttribute2_get_attributes(IHTMLDOMAttribute2 *iface, IDispatch **p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMAttribute2_get_ownerDocument(IHTMLDOMAttribute2 *iface, IDispatch **p)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMAttribute2_insertBefore(IHTMLDOMAttribute2 *iface, IHTMLDOMNode *newChild,
        VARIANT refChild, IHTMLDOMNode **node)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);
    FIXME("(%p)->(%p %s %p)\n", This, newChild, debugstr_variant(&refChild), node);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMAttribute2_replaceChild(IHTMLDOMAttribute2 *iface, IHTMLDOMNode *newChild,
        IHTMLDOMNode *oldChild, IHTMLDOMNode **node)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);
    FIXME("(%p)->(%p %p %p)\n", This, newChild, oldChild, node);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMAttribute2_removeChild(IHTMLDOMAttribute2 *iface, IHTMLDOMNode *oldChild,
        IHTMLDOMNode **node)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);
    FIXME("(%p)->(%p %p)\n", This, oldChild, node);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMAttribute2_appendChild(IHTMLDOMAttribute2 *iface, IHTMLDOMNode *newChild,
        IHTMLDOMNode **node)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);
    FIXME("(%p)->(%p %p)\n", This, newChild, node);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMAttribute2_hasChildNodes(IHTMLDOMAttribute2 *iface, VARIANT_BOOL *fChildren)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);
    FIXME("(%p)->(%p)\n", This, fChildren);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDOMAttribute2_cloneNode(IHTMLDOMAttribute2 *iface, VARIANT_BOOL fDeep,
        IHTMLDOMAttribute **clonedNode)
{
    HTMLDOMAttribute *This = impl_from_IHTMLDOMAttribute2(iface);
    FIXME("(%p)->(%x %p)\n", This, fDeep, clonedNode);
    return E_NOTIMPL;
}

static const IHTMLDOMAttribute2Vtbl HTMLDOMAttribute2Vtbl = {
    HTMLDOMAttribute2_QueryInterface,
    HTMLDOMAttribute2_AddRef,
    HTMLDOMAttribute2_Release,
    HTMLDOMAttribute2_GetTypeInfoCount,
    HTMLDOMAttribute2_GetTypeInfo,
    HTMLDOMAttribute2_GetIDsOfNames,
    HTMLDOMAttribute2_Invoke,
    HTMLDOMAttribute2_get_name,
    HTMLDOMAttribute2_put_value,
    HTMLDOMAttribute2_get_value,
    HTMLDOMAttribute2_get_expando,
    HTMLDOMAttribute2_get_nodeType,
    HTMLDOMAttribute2_get_parentNode,
    HTMLDOMAttribute2_get_childNodes,
    HTMLDOMAttribute2_get_firstChild,
    HTMLDOMAttribute2_get_lastChild,
    HTMLDOMAttribute2_get_previousSibling,
    HTMLDOMAttribute2_get_nextSibling,
    HTMLDOMAttribute2_get_attributes,
    HTMLDOMAttribute2_get_ownerDocument,
    HTMLDOMAttribute2_insertBefore,
    HTMLDOMAttribute2_replaceChild,
    HTMLDOMAttribute2_removeChild,
    HTMLDOMAttribute2_appendChild,
    HTMLDOMAttribute2_hasChildNodes,
    HTMLDOMAttribute2_cloneNode
};

static inline HTMLDOMAttribute *impl_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLDOMAttribute, dispex);
}

static void *HTMLDOMAttribute_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLDOMAttribute *This = impl_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLDOMAttribute, riid))
        return &This->IHTMLDOMAttribute_iface;
    if(IsEqualGUID(&IID_IHTMLDOMAttribute2, riid))
        return &This->IHTMLDOMAttribute2_iface;

    return NULL;
}

static void HTMLDOMAttribute_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    HTMLDOMAttribute *This = impl_from_DispatchEx(dispex);

    if(This->elem)
        note_cc_edge((nsISupports*)&This->elem->node.IHTMLDOMNode_iface, "elem", cb);
    traverse_variant(&This->value, "value", cb);
}

static void HTMLDOMAttribute_unlink(DispatchEx *dispex)
{
    HTMLDOMAttribute *This = impl_from_DispatchEx(dispex);

    if(This->elem) {
        HTMLElement *elem = This->elem;
        This->elem = NULL;
        IHTMLDOMNode_Release(&elem->node.IHTMLDOMNode_iface);
    }
    unlink_variant(&This->value);
}

static void HTMLDOMAttribute_destructor(DispatchEx *dispex)
{
    HTMLDOMAttribute *This = impl_from_DispatchEx(dispex);
    VariantClear(&This->value);
    free(This->name);
    free(This);
}

static const dispex_static_data_vtbl_t HTMLDOMAttribute_dispex_vtbl = {
    .query_interface  = HTMLDOMAttribute_query_interface,
    .destructor       = HTMLDOMAttribute_destructor,
    .traverse         = HTMLDOMAttribute_traverse,
    .unlink           = HTMLDOMAttribute_unlink
};

static const tid_t HTMLDOMAttribute_iface_tids[] = {
    IHTMLDOMAttribute_tid,
    IHTMLDOMAttribute2_tid,
    0
};
dispex_static_data_t Attr_dispex = {
    .id           = PROT_Attr,
    .prototype_id = PROT_Node,
    .vtbl         = &HTMLDOMAttribute_dispex_vtbl,
    .disp_tid     = DispHTMLDOMAttribute_tid,
    .iface_tids   = HTMLDOMAttribute_iface_tids,
};

HTMLDOMAttribute *unsafe_impl_from_IHTMLDOMAttribute(IHTMLDOMAttribute *iface)
{
    return iface->lpVtbl == &HTMLDOMAttributeVtbl ? impl_from_IHTMLDOMAttribute(iface) : NULL;
}

HRESULT HTMLDOMAttribute_Create(const WCHAR *name, HTMLElement *elem, DISPID dispid,
                                HTMLDocumentNode *doc, HTMLDOMAttribute **attr)
{
    HTMLAttributeCollection *col;
    HTMLDOMAttribute *ret;
    HRESULT hres;

    ret = calloc(1, sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IHTMLDOMAttribute_iface.lpVtbl = &HTMLDOMAttributeVtbl;
    ret->IHTMLDOMAttribute2_iface.lpVtbl = &HTMLDOMAttribute2Vtbl;
    ret->dispid = dispid;
    ret->elem = elem;

    init_dispatch(&ret->dispex, &Attr_dispex, doc->script_global,
                  dispex_compat_mode(&doc->script_global->event_target.dispex));

    /* For attributes attached to an element, (elem,dispid) pair should be valid used for its operation. */
    if(elem) {
        IHTMLDOMNode_AddRef(&elem->node.IHTMLDOMNode_iface);

        hres = HTMLElement_get_attr_col(&elem->node, &col);
        if(FAILED(hres)) {
            IHTMLDOMAttribute_Release(&ret->IHTMLDOMAttribute_iface);
            return hres;
        }
        IHTMLAttributeCollection_Release(&col->IHTMLAttributeCollection_iface);

        list_add_tail(&elem->attrs->attrs, &ret->entry);
    }

    /* For detached attributes we may still do most operations if we have its name available. */
    if(name) {
        ret->name = wcsdup(name);
        if(!ret->name) {
            IHTMLDOMAttribute_Release(&ret->IHTMLDOMAttribute_iface);
            return E_OUTOFMEMORY;
        }
    }

    *attr = ret;
    return S_OK;
}
