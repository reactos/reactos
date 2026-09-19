/*
 * Copyright 2006-2008 Jacek Caban for CodeWeavers
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

typedef struct {
    DispatchEx dispex;
    IHTMLElementCollection IHTMLElementCollection_iface;

    HTMLElement **elems;
    DWORD len;
} HTMLElementCollection;

typedef struct {
    IEnumVARIANT IEnumVARIANT_iface;

    LONG ref;

    ULONG iter;
    HTMLElementCollection *col;
} HTMLElementCollectionEnum;

typedef struct {
    HTMLElement **buf;
    DWORD len;
    DWORD size;
} elem_vector_t;

/* FIXME: Handle it better way */
static inline HTMLElement *elem_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLElement, node);
}

static IHTMLElementCollection *HTMLElementCollection_Create(HTMLElement**,DWORD,DispatchEx*);

static void elem_vector_add(elem_vector_t *buf, HTMLElement *elem)
{
    if(buf->len == buf->size) {
        buf->size <<= 1;
        buf->buf = realloc(buf->buf, buf->size * sizeof(HTMLElement*));
    }

    buf->buf[buf->len++] = elem;
}

static void elem_vector_normalize(elem_vector_t *buf)
{
    if(!buf->len) {
        free(buf->buf);
        buf->buf = NULL;
    }else if(buf->size > buf->len) {
        buf->buf = realloc(buf->buf, buf->len*sizeof(HTMLElement*));
    }

    buf->size = buf->len;
}

static inline BOOL is_elem_node(nsIDOMNode *node)
{
    UINT16 type=0;

    nsIDOMNode_GetNodeType(node, &type);

    return type == ELEMENT_NODE || type == COMMENT_NODE;
}

static inline HTMLElementCollectionEnum *impl_from_IEnumVARIANT(IEnumVARIANT *iface)
{
    return CONTAINING_RECORD(iface, HTMLElementCollectionEnum, IEnumVARIANT_iface);
}

static HRESULT WINAPI HTMLElementCollectionEnum_QueryInterface(IEnumVARIANT *iface, REFIID riid, void **ppv)
{
    HTMLElementCollectionEnum *This = impl_from_IEnumVARIANT(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(riid, &IID_IUnknown)) {
        *ppv = &This->IEnumVARIANT_iface;
    }else if(IsEqualGUID(riid, &IID_IEnumVARIANT)) {
        *ppv = &This->IEnumVARIANT_iface;
    }else {
        FIXME("Unsupported iface %s\n", debugstr_mshtml_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLElementCollectionEnum_AddRef(IEnumVARIANT *iface)
{
    HTMLElementCollectionEnum *This = impl_from_IEnumVARIANT(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLElementCollectionEnum_Release(IEnumVARIANT *iface)
{
    HTMLElementCollectionEnum *This = impl_from_IEnumVARIANT(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        IHTMLElementCollection_Release(&This->col->IHTMLElementCollection_iface);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLElementCollectionEnum_Next(IEnumVARIANT *iface, ULONG celt, VARIANT *rgVar, ULONG *pCeltFetched)
{
    HTMLElementCollectionEnum *This = impl_from_IEnumVARIANT(iface);
    ULONG fetched = 0;

    TRACE("(%p)->(%ld %p %p)\n", This, celt, rgVar, pCeltFetched);

    while(This->iter+fetched < This->col->len && fetched < celt) {
        V_VT(rgVar+fetched) = VT_DISPATCH;
        V_DISPATCH(rgVar+fetched) = (IDispatch*)&This->col->elems[This->iter+fetched]->IHTMLElement_iface;
        IDispatch_AddRef(V_DISPATCH(rgVar+fetched));
        fetched++;
    }

    This->iter += fetched;
    if(pCeltFetched)
        *pCeltFetched = fetched;
    return fetched == celt ? S_OK : S_FALSE;
}

static HRESULT WINAPI HTMLElementCollectionEnum_Skip(IEnumVARIANT *iface, ULONG celt)
{
    HTMLElementCollectionEnum *This = impl_from_IEnumVARIANT(iface);

    TRACE("(%p)->(%ld)\n", This, celt);

    if(This->iter + celt > This->col->len) {
        This->iter = This->col->len;
        return S_FALSE;
    }

    This->iter += celt;
    return S_OK;
}

static HRESULT WINAPI HTMLElementCollectionEnum_Reset(IEnumVARIANT *iface)
{
    HTMLElementCollectionEnum *This = impl_from_IEnumVARIANT(iface);

    TRACE("(%p)->()\n", This);

    This->iter = 0;
    return S_OK;
}

static HRESULT WINAPI HTMLElementCollectionEnum_Clone(IEnumVARIANT *iface, IEnumVARIANT **ppEnum)
{
    HTMLElementCollectionEnum *This = impl_from_IEnumVARIANT(iface);
    FIXME("(%p)->(%p)\n", This, ppEnum);
    return E_NOTIMPL;
}

static const IEnumVARIANTVtbl HTMLElementCollectionEnumVtbl = {
    HTMLElementCollectionEnum_QueryInterface,
    HTMLElementCollectionEnum_AddRef,
    HTMLElementCollectionEnum_Release,
    HTMLElementCollectionEnum_Next,
    HTMLElementCollectionEnum_Skip,
    HTMLElementCollectionEnum_Reset,
    HTMLElementCollectionEnum_Clone
};

static inline HTMLElementCollection *impl_from_IHTMLElementCollection(IHTMLElementCollection *iface)
{
    return CONTAINING_RECORD(iface, HTMLElementCollection, IHTMLElementCollection_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLElementCollection, IHTMLElementCollection,
                      impl_from_IHTMLElementCollection(iface)->dispex)

static HRESULT WINAPI HTMLElementCollection_toString(IHTMLElementCollection *iface,
                                                     BSTR *String)
{
    HTMLElementCollection *This = impl_from_IHTMLElementCollection(iface);

    TRACE("(%p)->(%p)\n", This, String);

    return dispex_to_string(&This->dispex, String);
}

static HRESULT WINAPI HTMLElementCollection_put_length(IHTMLElementCollection *iface,
                                                       LONG v)
{
    HTMLElementCollection *This = impl_from_IHTMLElementCollection(iface);
    FIXME("(%p)->(%ld)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElementCollection_get_length(IHTMLElementCollection *iface,
                                                       LONG *p)
{
    HTMLElementCollection *This = impl_from_IHTMLElementCollection(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = This->len;
    return S_OK;
}

static HRESULT WINAPI HTMLElementCollection_get__newEnum(IHTMLElementCollection *iface,
                                                         IUnknown **p)
{
    HTMLElementCollection *This = impl_from_IHTMLElementCollection(iface);
    HTMLElementCollectionEnum *ret;

    TRACE("(%p)->(%p)\n", This, p);

    ret = malloc(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IEnumVARIANT_iface.lpVtbl = &HTMLElementCollectionEnumVtbl;
    ret->ref = 1;
    ret->iter = 0;

    IHTMLElementCollection_AddRef(&This->IHTMLElementCollection_iface);
    ret->col = This;

    *p = (IUnknown*)&ret->IEnumVARIANT_iface;
    return S_OK;
}

static BOOL is_elem_id(HTMLElement *elem, LPCWSTR name)
{
    BSTR elem_id;
    HRESULT hres;

    hres = IHTMLElement_get_id(&elem->IHTMLElement_iface, &elem_id);
    if(FAILED(hres)){
        WARN("IHTMLElement_get_id failed: 0x%08lx\n", hres);
        return FALSE;
    }

    if(elem_id && !wcscmp(elem_id, name)) {
        SysFreeString(elem_id);
        return TRUE;
    }

    SysFreeString(elem_id);
    return FALSE;
}

static BOOL is_elem_name(HTMLElement *elem, LPCWSTR name)
{
    const PRUnichar *str;
    nsAString nsstr;
    BOOL ret = FALSE;
    nsresult nsres;

    if(!elem->dom_element)
        return FALSE;

    nsAString_Init(&nsstr, NULL);
    nsIDOMElement_GetId(elem->dom_element, &nsstr);
    nsAString_GetData(&nsstr, &str);
    if(!wcsicmp(str, name)) {
        nsAString_Finish(&nsstr);
        return TRUE;
    }

    nsres = get_elem_attr_value(elem->dom_element, L"name", &nsstr, &str);
    if(NS_SUCCEEDED(nsres)) {
        ret = !wcsicmp(str, name);
        nsAString_Finish(&nsstr);
    }

    return ret;
}

static HRESULT get_item_idx(HTMLElementCollection *This, UINT idx, IDispatch **ret)
{
    if(idx < This->len) {
        *ret = (IDispatch*)&This->elems[idx]->node.event_target.dispex.IWineJSDispatchHost_iface;
        IDispatch_AddRef(*ret);
    }

    return S_OK;
}

static HRESULT WINAPI HTMLElementCollection_item(IHTMLElementCollection *iface,
        VARIANT name, VARIANT index, IDispatch **pdisp)
{
    HTMLElementCollection *This = impl_from_IHTMLElementCollection(iface);
    HRESULT hres = S_OK;

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_variant(&name), debugstr_variant(&index), pdisp);

    *pdisp = NULL;

    switch(V_VT(&name)) {
    case VT_I4:
    case VT_INT:
        if(V_I4(&name) < 0)
            return dispex_compat_mode(&This->dispex) < COMPAT_MODE_IE9 ? E_INVALIDARG : S_OK;
        hres = get_item_idx(This, V_I4(&name), pdisp);
        break;

    case VT_UI4:
    case VT_UINT:
        hres = get_item_idx(This, V_UINT(&name), pdisp);
        break;

    case VT_BSTR: {
        DWORD i;

        if(V_VT(&index) == VT_I4) {
            LONG idx = V_I4(&index);

            if(idx < 0)
                return E_INVALIDARG;

            for(i=0; i<This->len; i++) {
                if(is_elem_name(This->elems[i], V_BSTR(&name)) && !idx--)
                    break;
            }

            if(i != This->len) {
                *pdisp = (IDispatch*)&This->elems[i]->IHTMLElement_iface;
                IDispatch_AddRef(*pdisp);
            }
        }else {
            elem_vector_t buf = {NULL, 0, 8};

            buf.buf = malloc(buf.size * sizeof(HTMLElement*));

            for(i=0; i<This->len; i++) {
                if(is_elem_name(This->elems[i], V_BSTR(&name))) {
                    node_addref(&This->elems[i]->node);
                    elem_vector_add(&buf, This->elems[i]);
                }
            }

            if(buf.len > 1) {
                elem_vector_normalize(&buf);
                *pdisp = (IDispatch*)HTMLElementCollection_Create(buf.buf, buf.len, &This->dispex);
            }else {
                if(buf.len == 1) {
                    /* Already AddRef-ed */
                    *pdisp = (IDispatch*)&buf.buf[0]->IHTMLElement_iface;
                }

                free(buf.buf);
            }
        }
        break;
    }

    default:
        FIXME("Unsupported name %s\n", debugstr_variant(&name));
        hres = E_NOTIMPL;
    }

    if(SUCCEEDED(hres))
        TRACE("returning %p\n", *pdisp);
    return hres;
}

static HRESULT WINAPI HTMLElementCollection_tags(IHTMLElementCollection *iface,
                                                 VARIANT tagName, IDispatch **pdisp)
{
    HTMLElementCollection *This = impl_from_IHTMLElementCollection(iface);
    DWORD i;
    nsAString tag_str;
    const PRUnichar *tag;
    elem_vector_t buf = {NULL, 0, 8};

    if(V_VT(&tagName) != VT_BSTR) {
        WARN("Invalid arg\n");
        return DISP_E_MEMBERNOTFOUND;
    }

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(V_BSTR(&tagName)), pdisp);

    buf.buf = malloc(buf.size * sizeof(HTMLElement*));

    nsAString_Init(&tag_str, NULL);

    for(i=0; i<This->len; i++) {
        if(!This->elems[i]->dom_element)
            continue;

        nsIDOMElement_GetTagName(This->elems[i]->dom_element, &tag_str);
        nsAString_GetData(&tag_str, &tag);

        if(CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, tag, -1,
                          V_BSTR(&tagName), -1) == CSTR_EQUAL) {
            node_addref(&This->elems[i]->node);
            elem_vector_add(&buf, This->elems[i]);
        }
    }

    nsAString_Finish(&tag_str);
    elem_vector_normalize(&buf);

    TRACE("found %ld tags\n", buf.len);

    *pdisp = (IDispatch*)HTMLElementCollection_Create(buf.buf, buf.len, &This->dispex);
    return S_OK;
}

static const IHTMLElementCollectionVtbl HTMLElementCollectionVtbl = {
    HTMLElementCollection_QueryInterface,
    HTMLElementCollection_AddRef,
    HTMLElementCollection_Release,
    HTMLElementCollection_GetTypeInfoCount,
    HTMLElementCollection_GetTypeInfo,
    HTMLElementCollection_GetIDsOfNames,
    HTMLElementCollection_Invoke,
    HTMLElementCollection_toString,
    HTMLElementCollection_put_length,
    HTMLElementCollection_get_length,
    HTMLElementCollection_get__newEnum,
    HTMLElementCollection_item,
    HTMLElementCollection_tags
};

static inline HTMLElementCollection *impl_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLElementCollection, dispex);
}

static void *HTMLElementCollection_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLElementCollection *This = impl_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLElementCollection, riid))
        return &This->IHTMLElementCollection_iface;

    return NULL;
}

static void HTMLElementCollection_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    HTMLElementCollection *This = impl_from_DispatchEx(dispex);
    unsigned i;

    for(i = 0; i < This->len; i++)
        note_cc_edge((nsISupports*)&This->elems[i]->node.IHTMLDOMNode_iface, "elem", cb);
}

static void HTMLElementCollection_unlink(DispatchEx *dispex)
{
    HTMLElementCollection *This = impl_from_DispatchEx(dispex);
    unsigned i, len = This->len;

    if(len) {
        This->len = 0;
        for(i = 0; i < len; i++)
            node_release(&This->elems[i]->node);
        free(This->elems);
    }
}

static void HTMLElementCollection_destructor(DispatchEx *dispex)
{
    HTMLElementCollection *This = impl_from_DispatchEx(dispex);
    free(This);
}

#define DISPID_ELEMCOL_0 MSHTML_DISPID_CUSTOM_MIN

static HRESULT HTMLElementCollection_get_dispid(DispatchEx *dispex, const WCHAR *name, DWORD flags, DISPID *dispid)
{
    HTMLElementCollection *This = impl_from_DispatchEx(dispex);
    const WCHAR *ptr;
    DWORD idx=0;

    if(!*name)
        return DISP_E_UNKNOWNNAME;

    for(ptr = name; *ptr && is_digit(*ptr); ptr++)
        idx = idx*10 + (*ptr-'0');

    if(*ptr) {
        /* the name contains alpha characters, so search by name & id */
        for(idx = 0; idx < This->len; ++idx) {
            if(is_elem_id(This->elems[idx], name) ||
                    is_elem_name(This->elems[idx], name))
                break;
        }
    }

    if(idx >= This->len)
        return DISP_E_UNKNOWNNAME;

    *dispid = DISPID_ELEMCOL_0 + idx;
    TRACE("ret %lx\n", *dispid);
    return S_OK;
}

static HRESULT HTMLElementCollection_invoke(DispatchEx *dispex, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLElementCollection *This = impl_from_DispatchEx(dispex);
    DWORD idx;

    TRACE("(%p)->(%lx %lx %x %p %p %p %p)\n", This, id, lcid, flags, params, res, ei, caller);

    idx = id - DISPID_ELEMCOL_0;
    if(idx >= This->len)
        return DISP_E_MEMBERNOTFOUND;

    switch(flags) {
    case DISPATCH_PROPERTYGET:
        V_VT(res) = VT_DISPATCH;
        V_DISPATCH(res) = (IDispatch*)&This->elems[idx]->IHTMLElement_iface;
        IHTMLElement_AddRef(&This->elems[idx]->IHTMLElement_iface);
        break;
    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static const dispex_static_data_vtbl_t HTMLElementColection_dispex_vtbl = {
    .query_interface  = HTMLElementCollection_query_interface,
    .destructor       = HTMLElementCollection_destructor,
    .traverse         = HTMLElementCollection_traverse,
    .unlink           = HTMLElementCollection_unlink,
    .get_dispid       = HTMLElementCollection_get_dispid,
    .get_prop_desc    = dispex_index_prop_desc,
    .invoke           = HTMLElementCollection_invoke,
};

static const tid_t HTMLCollection_iface_tids[] = {
    IHTMLElementCollection_tid,
    0
};

dispex_static_data_t HTMLCollection_dispex = {
    .id         = PROT_HTMLCollection,
    .vtbl       = &HTMLElementColection_dispex_vtbl,
    .disp_tid   = DispHTMLElementCollection_tid,
    .iface_tids = HTMLCollection_iface_tids,
};

static void create_all_list(HTMLDOMNode *elem, elem_vector_t *buf)
{
    nsIDOMNodeList *nsnode_list;
    nsIDOMNode *iter;
    UINT32 list_len = 0, i;
    nsresult nsres;
    HRESULT hres;

    nsres = nsIDOMNode_GetChildNodes(elem->nsnode, &nsnode_list);
    if(NS_FAILED(nsres)) {
        ERR("GetChildNodes failed: %08lx\n", nsres);
        return;
    }

    nsIDOMNodeList_GetLength(nsnode_list, &list_len);

    for(i=0; i<list_len; i++) {
        nsres = nsIDOMNodeList_Item(nsnode_list, i, &iter);
        if(NS_FAILED(nsres)) {
            ERR("Item failed: %08lx\n", nsres);
            continue;
        }

        if(is_elem_node(iter)) {
            HTMLDOMNode *node;

            hres = get_node(iter, TRUE, &node);
            if(FAILED(hres)) {
                FIXME("get_node failed: %08lx\n", hres);
                nsIDOMNode_Release(iter);
                continue;
            }

            elem_vector_add(buf, elem_from_HTMLDOMNode(node));
            create_all_list(node, buf);
        }

        nsIDOMNode_Release(iter);
    }

    nsIDOMNodeList_Release(nsnode_list);
}

IHTMLElementCollection *create_all_collection(HTMLDOMNode *node, BOOL include_root)
{
    elem_vector_t buf = {NULL, 0, 8};

    buf.buf = malloc(buf.size * sizeof(HTMLElement*));

    if(include_root) {
        node_addref(node);
        elem_vector_add(&buf, elem_from_HTMLDOMNode(node));
    }
    create_all_list(node, &buf);
    elem_vector_normalize(&buf);

    return HTMLElementCollection_Create(buf.buf, buf.len, &node->event_target.dispex);
}

IHTMLElementCollection *create_collection_from_nodelist(nsIDOMNodeList *nslist, DispatchEx *owner)
{
    UINT32 length = 0, i;
    HTMLDOMNode *node;
    elem_vector_t buf;
    HRESULT hres;

    nsIDOMNodeList_GetLength(nslist, &length);

    buf.len = 0;
    buf.size = length;
    if(length) {
        nsIDOMNode *nsnode;

        buf.buf = malloc(buf.size * sizeof(HTMLElement*));

        for(i=0; i<length; i++) {
            nsIDOMNodeList_Item(nslist, i, &nsnode);
            if(is_elem_node(nsnode)) {
                hres = get_node(nsnode, TRUE, &node);
                if(FAILED(hres))
                    continue;
                buf.buf[buf.len++] = elem_from_HTMLDOMNode(node);
            }
            nsIDOMNode_Release(nsnode);
        }

        elem_vector_normalize(&buf);
    }else {
        buf.buf = NULL;
    }

    return HTMLElementCollection_Create(buf.buf, buf.len, owner);
}

IHTMLElementCollection *create_collection_from_htmlcol(nsIDOMHTMLCollection *nscol, DispatchEx *owner)
{
    UINT32 length = 0, i;
    elem_vector_t buf;
    HTMLDOMNode *node;
    HRESULT hres = S_OK;

    if(nscol)
        nsIDOMHTMLCollection_GetLength(nscol, &length);

    buf.len = buf.size = length;
    if(buf.len) {
        nsIDOMNode *nsnode;

        buf.buf = malloc(buf.size * sizeof(HTMLElement*));

        for(i=0; i<length; i++) {
            nsIDOMHTMLCollection_Item(nscol, i, &nsnode);
            hres = get_node(nsnode, TRUE, &node);
            nsIDOMNode_Release(nsnode);
            if(FAILED(hres))
                break;
            buf.buf[i] = elem_from_HTMLDOMNode(node);
        }
    }else {
        buf.buf = NULL;
    }

    if(FAILED(hres)) {
        free(buf.buf);
        return NULL;
    }

    return HTMLElementCollection_Create(buf.buf, buf.len, owner);
}

HRESULT get_elem_source_index(HTMLElement *elem, LONG *ret)
{
    elem_vector_t buf = {NULL, 0, 8};
    nsIDOMNode *parent_node, *iter;
    UINT16 parent_type;
    HTMLDOMNode *node;
    nsresult nsres;
    unsigned i, j;
    HRESULT hres;

    iter = elem->node.nsnode;
    nsIDOMNode_AddRef(iter);

    /* Find document or document fragment parent. */
    while(1) {
        nsres = nsIDOMNode_GetParentNode(iter, &parent_node);
        nsIDOMNode_Release(iter);
        assert(nsres == NS_OK);
        if(!parent_node)
            break;

        nsres = nsIDOMNode_GetNodeType(parent_node, &parent_type);
        assert(nsres == NS_OK);

        if(parent_type != ELEMENT_NODE) {
            if(parent_type != DOCUMENT_NODE && parent_type != DOCUMENT_FRAGMENT_NODE)
                FIXME("Unexpected parent_type %d\n", parent_type);
            break;
        }

        iter = parent_node;
    }

    if(!parent_node) {
        *ret = -1;
        return S_OK;
    }

    hres = get_node(parent_node, TRUE, &node);
    nsIDOMNode_Release(parent_node);
    if(FAILED(hres))
        return hres;

    /* Create all children collection and find the element in it.
     * This could be optimized if we ever find the reason. */
    buf.buf = malloc(buf.size * sizeof(*buf.buf));
    if(!buf.buf) {
        IHTMLDOMNode_Release(&node->IHTMLDOMNode_iface);
        return E_OUTOFMEMORY;
    }

    create_all_list(node, &buf);

    for(i=0; i < buf.len; i++) {
        if(buf.buf[i] == elem)
            break;
    }
    IHTMLDOMNode_Release(&node->IHTMLDOMNode_iface);

    for(j = 0; j < buf.len; j++)
        IHTMLDOMNode_Release(&buf.buf[j]->node.IHTMLDOMNode_iface);
    free(buf.buf);

    if(i == buf.len) {
        FIXME("The element is not in parent's child list?\n");
        return E_UNEXPECTED;
    }

    *ret = i;
    return S_OK;
}

static IHTMLElementCollection *HTMLElementCollection_Create(HTMLElement **elems, DWORD len,
                                                            DispatchEx *owner)
{
    HTMLElementCollection *ret = calloc(1, sizeof(HTMLElementCollection));

    if (!ret)
        return NULL;

    ret->IHTMLElementCollection_iface.lpVtbl = &HTMLElementCollectionVtbl;
    ret->elems = elems;
    ret->len = len;

    init_dispatch_with_owner(&ret->dispex, &HTMLCollection_dispex, owner);

    TRACE("ret=%p len=%ld\n", ret, len);

    return &ret->IHTMLElementCollection_iface;
}
