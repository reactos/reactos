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
    const IHTMLElementCollectionVtbl *lpHTMLElementCollectionVtbl;

    IUnknown *ref_unk;
    HTMLElement **elems;
    DWORD len;

    LONG ref;
} HTMLElementCollection;

#define HTMLELEMCOL(x)  ((IHTMLElementCollection*) &(x)->lpHTMLElementCollectionVtbl)

typedef struct {
    HTMLElement **buf;
    DWORD len;
    DWORD size;
} elem_vector_t;

static IHTMLElementCollection *HTMLElementCollection_Create(IUnknown *ref_unk,
                                                            HTMLElement **elems, DWORD len);

static void elem_vector_add(elem_vector_t *buf, HTMLElement *elem)
{
    if(buf->len == buf->size) {
        buf->size <<= 1;
        buf->buf = heap_realloc(buf->buf, buf->size*sizeof(HTMLElement**));
    }

    buf->buf[buf->len++] = elem;
}

static void elem_vector_normalize(elem_vector_t *buf)
{
    if(!buf->len) {
        heap_free(buf->buf);
        buf->buf = NULL;
    }else if(buf->size > buf->len) {
        buf->buf = heap_realloc(buf->buf, buf->len*sizeof(HTMLElement**));
    }

    buf->size = buf->len;
}

static inline BOOL is_elem_node(nsIDOMNode *node)
{
    PRUint16 type=0;

    nsIDOMNode_GetNodeType(node, &type);

    return type == ELEMENT_NODE || type == COMMENT_NODE;
}

#define ELEMCOL_THIS(iface) DEFINE_THIS(HTMLElementCollection, HTMLElementCollection, iface)
#define HTMLELEM_NODE_THIS(iface) DEFINE_THIS2(HTMLElement, node, iface)

static HRESULT WINAPI HTMLElementCollection_QueryInterface(IHTMLElementCollection *iface,
                                                           REFIID riid, void **ppv)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = HTMLELEMCOL(This);
    }else if(IsEqualGUID(&IID_IHTMLElementCollection, riid)) {
        TRACE("(%p)->(IID_IHTMLElementCollection %p)\n", This, ppv);
        *ppv = HTMLELEMCOL(This);
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }

    if(*ppv) {
        IHTMLElementCollection_AddRef(HTMLELEMCOL(This));
        return S_OK;
    }

    FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI HTMLElementCollection_AddRef(IHTMLElementCollection *iface)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI HTMLElementCollection_Release(IHTMLElementCollection *iface)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        IUnknown_Release(This->ref_unk);
        heap_free(This->elems);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLElementCollection_GetTypeInfoCount(IHTMLElementCollection *iface,
                                                             UINT *pctinfo)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(&This->dispex), pctinfo);
}

static HRESULT WINAPI HTMLElementCollection_GetTypeInfo(IHTMLElementCollection *iface,
        UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    return IDispatchEx_GetTypeInfo(DISPATCHEX(&This->dispex), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLElementCollection_GetIDsOfNames(IHTMLElementCollection *iface,
        REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    return IDispatchEx_GetIDsOfNames(DISPATCHEX(&This->dispex), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLElementCollection_Invoke(IHTMLElementCollection *iface,
        DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    return IDispatchEx_Invoke(DISPATCHEX(&This->dispex), dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLElementCollection_toString(IHTMLElementCollection *iface,
                                                     BSTR *String)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    FIXME("(%p)->(%p)\n", This, String);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElementCollection_put_length(IHTMLElementCollection *iface,
                                                       LONG v)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    FIXME("(%p)->(%d)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLElementCollection_get_length(IHTMLElementCollection *iface,
                                                       LONG *p)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = This->len;
    return S_OK;
}

static HRESULT WINAPI HTMLElementCollection_get__newEnum(IHTMLElementCollection *iface,
                                                         IUnknown **p)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static BOOL is_elem_name(HTMLElement *elem, LPCWSTR name)
{
    const PRUnichar *str;
    nsAString nsstr, nsname;
    BOOL ret = FALSE;
    nsresult nsres;

    static const PRUnichar nameW[] = {'n','a','m','e',0};

    if(!elem->nselem)
        return FALSE;

    nsAString_Init(&nsstr, NULL);
    nsIDOMHTMLElement_GetId(elem->nselem, &nsstr);
    nsAString_GetData(&nsstr, &str);
    if(!strcmpiW(str, name)) {
        nsAString_Finish(&nsstr);
        return TRUE;
    }

    nsAString_Init(&nsname, nameW);
    nsres =  nsIDOMHTMLElement_GetAttribute(elem->nselem, &nsname, &nsstr);
    nsAString_Finish(&nsname);
    if(NS_SUCCEEDED(nsres)) {
        nsAString_GetData(&nsstr, &str);
        ret = !strcmpiW(str, name);
    }

    nsAString_Finish(&nsstr);
    return ret;
}

static HRESULT WINAPI HTMLElementCollection_item(IHTMLElementCollection *iface,
        VARIANT name, VARIANT index, IDispatch **pdisp)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);

    TRACE("(%p)->(v(%d) v(%d) %p)\n", This, V_VT(&name), V_VT(&index), pdisp);

    *pdisp = NULL;

    if(V_VT(&name) == VT_I4) {
        TRACE("name is VT_I4: %d\n", V_I4(&name));

        if(V_I4(&name) < 0)
            return E_INVALIDARG;
        if(V_I4(&name) >= This->len)
            return S_OK;

        *pdisp = (IDispatch*)This->elems[V_I4(&name)];
        IDispatch_AddRef(*pdisp);
        TRACE("Returning pdisp=%p\n", pdisp);
        return S_OK;
    }

    if(V_VT(&name) == VT_BSTR) {
        DWORD i;

        TRACE("name is VT_BSTR: %s\n", debugstr_w(V_BSTR(&name)));

        if(V_VT(&index) == VT_I4) {
            LONG idx = V_I4(&index);

            TRACE("index = %d\n", idx);

            if(idx < 0)
                return E_INVALIDARG;

            for(i=0; i<This->len; i++) {
                if(is_elem_name(This->elems[i], V_BSTR(&name)) && !idx--)
                    break;
            }

            if(i != This->len) {
                *pdisp = (IDispatch*)HTMLELEM(This->elems[i]);
                IDispatch_AddRef(*pdisp);
            }

            return S_OK;
        }else {
            elem_vector_t buf = {NULL, 0, 8};

            buf.buf = heap_alloc(buf.size*sizeof(HTMLElement*));

            for(i=0; i<This->len; i++) {
                if(is_elem_name(This->elems[i], V_BSTR(&name)))
                    elem_vector_add(&buf, This->elems[i]);
            }

            if(buf.len > 1) {
                elem_vector_normalize(&buf);
                *pdisp = (IDispatch*)HTMLElementCollection_Create(This->ref_unk, buf.buf, buf.len);
            }else {
                if(buf.len == 1) {
                    *pdisp = (IDispatch*)HTMLELEM(buf.buf[0]);
                    IDispatch_AddRef(*pdisp);
                }

                heap_free(buf.buf);
            }

            return S_OK;
        }
    }

    FIXME("unsupported arguments\n");
    return E_INVALIDARG;
}

static HRESULT WINAPI HTMLElementCollection_tags(IHTMLElementCollection *iface,
                                                 VARIANT tagName, IDispatch **pdisp)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    DWORD i;
    nsAString tag_str;
    const PRUnichar *tag;
    elem_vector_t buf = {NULL, 0, 8};

    if(V_VT(&tagName) != VT_BSTR) {
        WARN("Invalid arg\n");
        return DISP_E_MEMBERNOTFOUND;
    }

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(V_BSTR(&tagName)), pdisp);

    buf.buf = heap_alloc(buf.size*sizeof(HTMLElement*));

    nsAString_Init(&tag_str, NULL);

    for(i=0; i<This->len; i++) {
        if(!This->elems[i]->nselem)
            continue;

        nsIDOMElement_GetTagName(This->elems[i]->nselem, &tag_str);
        nsAString_GetData(&tag_str, &tag);

        if(CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, tag, -1,
                          V_BSTR(&tagName), -1) == CSTR_EQUAL)
            elem_vector_add(&buf, This->elems[i]);
    }

    nsAString_Finish(&tag_str);
    elem_vector_normalize(&buf);

    TRACE("fount %d tags\n", buf.len);

    *pdisp = (IDispatch*)HTMLElementCollection_Create(This->ref_unk, buf.buf, buf.len);
    return S_OK;
}

#define DISPID_ELEMCOL_0 MSHTML_DISPID_CUSTOM_MIN

static HRESULT HTMLElementCollection_get_dispid(IUnknown *iface, BSTR name, DWORD flags, DISPID *dispid)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    WCHAR *ptr;
    DWORD idx=0;

    if(!*name)
        return DISP_E_UNKNOWNNAME;

    for(ptr = name; *ptr && isdigitW(*ptr); ptr++)
        idx = idx*10 + (*ptr-'0');

    if(*ptr || idx >= This->len)
        return DISP_E_UNKNOWNNAME;

    *dispid = DISPID_ELEMCOL_0 + idx;
    TRACE("ret %x\n", *dispid);
    return S_OK;
}

static HRESULT HTMLElementCollection_invoke(IUnknown *iface, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLElementCollection *This = ELEMCOL_THIS(iface);
    DWORD idx;

    TRACE("(%p)->(%x %x %x %p %p %p %p)\n", This, id, lcid, flags, params, res, ei, caller);

    idx = id - DISPID_ELEMCOL_0;
    if(idx >= This->len)
        return DISP_E_UNKNOWNNAME;

    switch(flags) {
    case INVOKE_PROPERTYGET:
        V_VT(res) = VT_DISPATCH;
        V_DISPATCH(res) = (IDispatch*)HTMLELEM(This->elems[idx]);
        IHTMLElement_AddRef(HTMLELEM(This->elems[idx]));
        break;
    default:
        FIXME("unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

#undef ELEMCOL_THIS

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

static const dispex_static_data_vtbl_t HTMLElementColection_dispex_vtbl = {
    HTMLElementCollection_get_dispid,
    HTMLElementCollection_invoke
};

static const tid_t HTMLElementCollection_iface_tids[] = {
    IHTMLElementCollection_tid,
    0
};
static dispex_static_data_t HTMLElementCollection_dispex = {
    &HTMLElementColection_dispex_vtbl,
    DispHTMLElementCollection_tid,
    NULL,
    HTMLElementCollection_iface_tids
};

static void create_all_list(HTMLDocument *doc, HTMLDOMNode *elem, elem_vector_t *buf)
{
    nsIDOMNodeList *nsnode_list;
    nsIDOMNode *iter;
    PRUint32 list_len = 0, i;
    nsresult nsres;

    nsres = nsIDOMNode_GetChildNodes(elem->nsnode, &nsnode_list);
    if(NS_FAILED(nsres)) {
        ERR("GetChildNodes failed: %08x\n", nsres);
        return;
    }

    nsIDOMNodeList_GetLength(nsnode_list, &list_len);
    if(!list_len)
        return;

    for(i=0; i<list_len; i++) {
        nsres = nsIDOMNodeList_Item(nsnode_list, i, &iter);
        if(NS_FAILED(nsres)) {
            ERR("Item failed: %08x\n", nsres);
            continue;
        }

        if(is_elem_node(iter)) {
            HTMLDOMNode *node = get_node(doc, iter, TRUE);

            elem_vector_add(buf, HTMLELEM_NODE_THIS(node));
            create_all_list(doc, node, buf);
        }
    }
}

IHTMLElementCollection *create_all_collection(HTMLDOMNode *node, BOOL include_root)
{
    elem_vector_t buf = {NULL, 0, 8};

    buf.buf = heap_alloc(buf.size*sizeof(HTMLElement**));

    if(include_root)
        elem_vector_add(&buf, HTMLELEM_NODE_THIS(node));
    create_all_list(node->doc, node, &buf);
    elem_vector_normalize(&buf);

    return HTMLElementCollection_Create((IUnknown*)HTMLDOMNODE(node), buf.buf, buf.len);
}

IHTMLElementCollection *create_collection_from_nodelist(HTMLDocument *doc, IUnknown *unk, nsIDOMNodeList *nslist)
{
    PRUint32 length = 0, i;
    elem_vector_t buf;

    nsIDOMNodeList_GetLength(nslist, &length);

    buf.len = 0;
    buf.size = length;
    if(length) {
        nsIDOMNode *nsnode;

        buf.buf = heap_alloc(buf.size*sizeof(HTMLElement*));

        for(i=0; i<length; i++) {
            nsIDOMNodeList_Item(nslist, i, &nsnode);
            if(is_elem_node(nsnode))
                buf.buf[buf.len++] = HTMLELEM_NODE_THIS(get_node(doc, nsnode, TRUE));
            nsIDOMNode_Release(nsnode);
        }

        elem_vector_normalize(&buf);
    }else {
        buf.buf = NULL;
    }

    return HTMLElementCollection_Create(unk, buf.buf, buf.len);
}

IHTMLElementCollection *create_collection_from_htmlcol(HTMLDocument *doc, IUnknown *unk, nsIDOMHTMLCollection *nscol)
{
    PRUint32 length = 0, i;
    elem_vector_t buf;

    nsIDOMHTMLCollection_GetLength(nscol, &length);

    buf.len = buf.size = length;
    if(buf.len) {
        nsIDOMNode *nsnode;

        buf.buf = heap_alloc(buf.size*sizeof(HTMLElement*));

        for(i=0; i<length; i++) {
            nsIDOMHTMLCollection_Item(nscol, i, &nsnode);
            buf.buf[i] = HTMLELEM_NODE_THIS(get_node(doc, nsnode, TRUE));
            nsIDOMNode_Release(nsnode);
        }
    }else {
        buf.buf = NULL;
    }

    return HTMLElementCollection_Create(unk, buf.buf, buf.len);
}

static IHTMLElementCollection *HTMLElementCollection_Create(IUnknown *ref_unk,
            HTMLElement **elems, DWORD len)
{
    HTMLElementCollection *ret = heap_alloc_zero(sizeof(HTMLElementCollection));

    ret->lpHTMLElementCollectionVtbl = &HTMLElementCollectionVtbl;
    ret->ref = 1;
    ret->elems = elems;
    ret->len = len;

    init_dispex(&ret->dispex, (IUnknown*)HTMLELEMCOL(ret), &HTMLElementCollection_dispex);

    IUnknown_AddRef(ref_unk);
    ret->ref_unk = ref_unk;

    TRACE("ret=%p len=%d\n", ret, len);

    return HTMLELEMCOL(ret);
}
