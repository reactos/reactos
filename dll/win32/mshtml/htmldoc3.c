/*
 * Copyright 2005 Jacek Caban for CodeWeavers
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

HRESULT get_doc_elem_by_id(HTMLDocumentNode *doc, const WCHAR *id, HTMLElement **ret)
{
    nsIDOMNodeList *nsnode_list;
    nsIDOMElement *nselem;
    nsIDOMNode *nsnode;
    nsAString id_str;
    nsresult nsres;
    HRESULT hres;

    if(!doc->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsAString_InitDepend(&id_str, id);
    /* get element by id attribute */
    nsres = nsIDOMHTMLDocument_GetElementById(doc->nsdoc, &id_str, &nselem);
    if(FAILED(nsres)) {
        ERR("GetElementById failed: %08x\n", nsres);
        nsAString_Finish(&id_str);
        return E_FAIL;
    }

    /* get first element by name attribute */
    nsres = nsIDOMHTMLDocument_GetElementsByName(doc->nsdoc, &id_str, &nsnode_list);
    nsAString_Finish(&id_str);
    if(FAILED(nsres)) {
        ERR("getElementsByName failed: %08x\n", nsres);
        if(nselem)
            nsIDOMElement_Release(nselem);
        return E_FAIL;
    }

    nsres = nsIDOMNodeList_Item(nsnode_list, 0, &nsnode);
    nsIDOMNodeList_Release(nsnode_list);
    assert(nsres == NS_OK);

    if(nsnode && nselem) {
        UINT16 pos;

        nsres = nsIDOMNode_CompareDocumentPosition(nsnode, (nsIDOMNode*)nselem, &pos);
        if(NS_FAILED(nsres)) {
            FIXME("CompareDocumentPosition failed: 0x%08x\n", nsres);
            nsIDOMNode_Release(nsnode);
            nsIDOMElement_Release(nselem);
            return E_FAIL;
        }

        TRACE("CompareDocumentPosition gave: 0x%x\n", pos);
        if(!(pos & (DOCUMENT_POSITION_PRECEDING | DOCUMENT_POSITION_CONTAINS))) {
            nsIDOMElement_Release(nselem);
            nselem = NULL;
        }
    }

    if(nsnode) {
        if(!nselem) {
            nsres = nsIDOMNode_QueryInterface(nsnode, &IID_nsIDOMElement, (void**)&nselem);
            assert(nsres == NS_OK);
        }
        nsIDOMNode_Release(nsnode);
    }

    if(!nselem) {
        *ret = NULL;
        return S_OK;
    }

    hres = get_elem(doc, nselem, ret);
    nsIDOMElement_Release(nselem);
    return hres;
}

static inline HTMLDocument *impl_from_IHTMLDocument3(IHTMLDocument3 *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocument, IHTMLDocument3_iface);
}

static HRESULT WINAPI HTMLDocument3_QueryInterface(IHTMLDocument3 *iface,
                                                  REFIID riid, void **ppv)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    return htmldoc_query_interface(This, riid, ppv);
}

static ULONG WINAPI HTMLDocument3_AddRef(IHTMLDocument3 *iface)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    return htmldoc_addref(This);
}

static ULONG WINAPI HTMLDocument3_Release(IHTMLDocument3 *iface)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    return htmldoc_release(This);
}

static HRESULT WINAPI HTMLDocument3_GetTypeInfoCount(IHTMLDocument3 *iface, UINT *pctinfo)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    return IDispatchEx_GetTypeInfoCount(&This->IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLDocument3_GetTypeInfo(IHTMLDocument3 *iface, UINT iTInfo,
                                                LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    return IDispatchEx_GetTypeInfo(&This->IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLDocument3_GetIDsOfNames(IHTMLDocument3 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    return IDispatchEx_GetIDsOfNames(&This->IDispatchEx_iface, riid, rgszNames, cNames, lcid,
            rgDispId);
}

static HRESULT WINAPI HTMLDocument3_Invoke(IHTMLDocument3 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    return IDispatchEx_Invoke(&This->IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLDocument3_releaseCapture(IHTMLDocument3 *iface)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_recalc(IHTMLDocument3 *iface, VARIANT_BOOL fForce)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%x)\n", This, fForce);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_createTextNode(IHTMLDocument3 *iface, BSTR text,
                                                   IHTMLDOMNode **newTextNode)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    nsIDOMText *nstext;
    HTMLDOMNode *node;
    nsAString text_str;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(text), newTextNode);

    if(!This->doc_node->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsAString_InitDepend(&text_str, text);
    nsres = nsIDOMHTMLDocument_CreateTextNode(This->doc_node->nsdoc, &text_str, &nstext);
    nsAString_Finish(&text_str);
    if(NS_FAILED(nsres)) {
        ERR("CreateTextNode failed: %08x\n", nsres);
        return E_FAIL;
    }

    hres = HTMLDOMTextNode_Create(This->doc_node, (nsIDOMNode*)nstext, &node);
    nsIDOMText_Release(nstext);
    if(FAILED(hres))
        return hres;

    *newTextNode = &node->IHTMLDOMNode_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument3_get_documentElement(IHTMLDocument3 *iface, IHTMLElement **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    nsIDOMElement *nselem = NULL;
    HTMLDOMNode *node;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->window->readystate == READYSTATE_UNINITIALIZED) {
        *p = NULL;
        return S_OK;
    }

    if(!This->doc_node->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMHTMLDocument_GetDocumentElement(This->doc_node->nsdoc, &nselem);
    if(NS_FAILED(nsres)) {
        ERR("GetDocumentElement failed: %08x\n", nsres);
        return E_FAIL;
    }

    if(!nselem) {
        *p = NULL;
        return S_OK;
    }

    hres = get_node(This->doc_node, (nsIDOMNode *)nselem, TRUE, &node);
    nsIDOMElement_Release(nselem);
    if(FAILED(hres))
        return hres;

    hres = IHTMLDOMNode_QueryInterface(&node->IHTMLDOMNode_iface, &IID_IHTMLElement, (void**)p);
    node_release(node);
    return hres;
}

static HRESULT WINAPI HTMLDocument3_uniqueID(IHTMLDocument3 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_attachEvent(IHTMLDocument3 *iface, BSTR event,
                                                IDispatch* pDisp, VARIANT_BOOL *pfResult)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);

    TRACE("(%p)->(%s %p %p)\n", This, debugstr_w(event), pDisp, pfResult);

    return attach_event(&This->doc_node->node.event_target, This->doc_node->node.nsnode, This, event, pDisp, pfResult);
}

static HRESULT WINAPI HTMLDocument3_detachEvent(IHTMLDocument3 *iface, BSTR event,
                                                IDispatch *pDisp)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(event), pDisp);

    return detach_event(This->doc_node->node.event_target, This, event, pDisp);
}

static HRESULT WINAPI HTMLDocument3_put_onrowsdelete(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_onrowsdelete(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_onrowsinserted(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_onrowsinserted(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_oncellchange(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_oncellchange(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_ondatasetchanged(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_ondatasetchanged(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_ondataavailable(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_ondataavailable(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_ondatasetcomplete(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_ondatasetcomplete(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_onpropertychange(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_onpropertychange(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_dir(IHTMLDocument3 *iface, BSTR v)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_dir(IHTMLDocument3 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_oncontextmenu(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);

    TRACE("(%p)->()\n", This);

    return set_doc_event(This, EVENTID_CONTEXTMENU, &v);
}

static HRESULT WINAPI HTMLDocument3_get_oncontextmenu(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_CONTEXTMENU, p);
}

static HRESULT WINAPI HTMLDocument3_put_onstop(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_onstop(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_createDocumentFragment(IHTMLDocument3 *iface,
                                                           IHTMLDocument2 **ppNewDoc)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    nsIDOMDocumentFragment *doc_frag;
    HTMLDocumentNode *docnode;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, ppNewDoc);

    if(!This->doc_node->nsdoc) {
        FIXME("NULL nsdoc\n");
        return E_NOTIMPL;
    }

    nsres = nsIDOMHTMLDocument_CreateDocumentFragment(This->doc_node->nsdoc, &doc_frag);
    if(NS_FAILED(nsres)) {
        ERR("CreateDocumentFragment failed: %08x\n", nsres);
        return E_FAIL;
    }

    hres = create_document_fragment((nsIDOMNode*)doc_frag, This->doc_node, &docnode);
    nsIDOMDocumentFragment_Release(doc_frag);
    if(FAILED(hres))
        return hres;

    *ppNewDoc = &docnode->basedoc.IHTMLDocument2_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument3_get_parentDocument(IHTMLDocument3 *iface,
                                                       IHTMLDocument2 **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_enableDownload(IHTMLDocument3 *iface,
                                                       VARIANT_BOOL v)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_enableDownload(IHTMLDocument3 *iface,
                                                       VARIANT_BOOL *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_baseUrl(IHTMLDocument3 *iface, BSTR v)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_baseUrl(IHTMLDocument3 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_childNodes(IHTMLDocument3 *iface, IDispatch **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLDOMNode_get_childNodes(&This->doc_node->node.IHTMLDOMNode_iface, p);
}

static HRESULT WINAPI HTMLDocument3_put_inheritStyleSheets(IHTMLDocument3 *iface,
                                                           VARIANT_BOOL v)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_inheritStyleSheets(IHTMLDocument3 *iface,
                                                           VARIANT_BOOL *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_onbeforeeditfocus(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_onbeforeeditfocus(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_getElementsByName(IHTMLDocument3 *iface, BSTR v,
                                                      IHTMLElementCollection **ppelColl)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(v), ppelColl);
    return E_NOTIMPL;
}


static HRESULT WINAPI HTMLDocument3_getElementById(IHTMLDocument3 *iface, BSTR v,
                                                   IHTMLElement **pel)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    HTMLElement *elem;
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(v), pel);

    hres = get_doc_elem_by_id(This->doc_node, v, &elem);
    if(FAILED(hres) || !elem) {
        *pel = NULL;
        return hres;
    }

    *pel = &elem->IHTMLElement_iface;
    return S_OK;
}


static HRESULT WINAPI HTMLDocument3_getElementsByTagName(IHTMLDocument3 *iface, BSTR v,
                                                         IHTMLElementCollection **pelColl)
{
    HTMLDocument *This = impl_from_IHTMLDocument3(iface);
    nsIDOMNodeList *nslist;
    nsAString id_str;
    nsresult nsres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(v), pelColl);

    if(!This->doc_node->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsAString_InitDepend(&id_str, v);
    nsres = nsIDOMHTMLDocument_GetElementsByTagName(This->doc_node->nsdoc, &id_str, &nslist);
    nsAString_Finish(&id_str);
    if(FAILED(nsres)) {
        ERR("GetElementByName failed: %08x\n", nsres);
        return E_FAIL;
    }

    *pelColl = create_collection_from_nodelist(This->doc_node, nslist);
    nsIDOMNodeList_Release(nslist);

    return S_OK;
}

static const IHTMLDocument3Vtbl HTMLDocument3Vtbl = {
    HTMLDocument3_QueryInterface,
    HTMLDocument3_AddRef,
    HTMLDocument3_Release,
    HTMLDocument3_GetTypeInfoCount,
    HTMLDocument3_GetTypeInfo,
    HTMLDocument3_GetIDsOfNames,
    HTMLDocument3_Invoke,
    HTMLDocument3_releaseCapture,
    HTMLDocument3_recalc,
    HTMLDocument3_createTextNode,
    HTMLDocument3_get_documentElement,
    HTMLDocument3_uniqueID,
    HTMLDocument3_attachEvent,
    HTMLDocument3_detachEvent,
    HTMLDocument3_put_onrowsdelete,
    HTMLDocument3_get_onrowsdelete,
    HTMLDocument3_put_onrowsinserted,
    HTMLDocument3_get_onrowsinserted,
    HTMLDocument3_put_oncellchange,
    HTMLDocument3_get_oncellchange,
    HTMLDocument3_put_ondatasetchanged,
    HTMLDocument3_get_ondatasetchanged,
    HTMLDocument3_put_ondataavailable,
    HTMLDocument3_get_ondataavailable,
    HTMLDocument3_put_ondatasetcomplete,
    HTMLDocument3_get_ondatasetcomplete,
    HTMLDocument3_put_onpropertychange,
    HTMLDocument3_get_onpropertychange,
    HTMLDocument3_put_dir,
    HTMLDocument3_get_dir,
    HTMLDocument3_put_oncontextmenu,
    HTMLDocument3_get_oncontextmenu,
    HTMLDocument3_put_onstop,
    HTMLDocument3_get_onstop,
    HTMLDocument3_createDocumentFragment,
    HTMLDocument3_get_parentDocument,
    HTMLDocument3_put_enableDownload,
    HTMLDocument3_get_enableDownload,
    HTMLDocument3_put_baseUrl,
    HTMLDocument3_get_baseUrl,
    HTMLDocument3_get_childNodes,
    HTMLDocument3_put_inheritStyleSheets,
    HTMLDocument3_get_inheritStyleSheets,
    HTMLDocument3_put_onbeforeeditfocus,
    HTMLDocument3_get_onbeforeeditfocus,
    HTMLDocument3_getElementsByName,
    HTMLDocument3_getElementById,
    HTMLDocument3_getElementsByTagName
};

static inline HTMLDocument *impl_from_IHTMLDocument4(IHTMLDocument4 *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocument, IHTMLDocument4_iface);
}

static HRESULT WINAPI HTMLDocument4_QueryInterface(IHTMLDocument4 *iface,
                                                   REFIID riid, void **ppv)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    return htmldoc_query_interface(This, riid, ppv);
}

static ULONG WINAPI HTMLDocument4_AddRef(IHTMLDocument4 *iface)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    return htmldoc_addref(This);
}

static ULONG WINAPI HTMLDocument4_Release(IHTMLDocument4 *iface)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    return htmldoc_release(This);
}

static HRESULT WINAPI HTMLDocument4_GetTypeInfoCount(IHTMLDocument4 *iface, UINT *pctinfo)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    return IDispatchEx_GetTypeInfoCount(&This->IDispatchEx_iface, pctinfo);
}

static HRESULT WINAPI HTMLDocument4_GetTypeInfo(IHTMLDocument4 *iface, UINT iTInfo,
                                                LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    return IDispatchEx_GetTypeInfo(&This->IDispatchEx_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLDocument4_GetIDsOfNames(IHTMLDocument4 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    return IDispatchEx_GetIDsOfNames(&This->IDispatchEx_iface, riid, rgszNames, cNames, lcid,
            rgDispId);
}

static HRESULT WINAPI HTMLDocument4_Invoke(IHTMLDocument4 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    return IDispatchEx_Invoke(&This->IDispatchEx_iface, dispIdMember, riid, lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLDocument4_focus(IHTMLDocument4 *iface)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    nsIDOMHTMLElement *nsbody;
    nsresult nsres;

    TRACE("(%p)->()\n", This);

    nsres = nsIDOMHTMLDocument_GetBody(This->doc_node->nsdoc, &nsbody);
    if(NS_FAILED(nsres) || !nsbody) {
        ERR("GetBody failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMHTMLElement_Focus(nsbody);
    nsIDOMHTMLElement_Release(nsbody);
    if(NS_FAILED(nsres)) {
        ERR("Focus failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument4_hasFocus(IHTMLDocument4 *iface, VARIANT_BOOL *pfFocus)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    FIXME("(%p)->(%p)\n", This, pfFocus);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_put_onselectionchange(IHTMLDocument4 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_get_onselectionchange(IHTMLDocument4 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_get_namespace(IHTMLDocument4 *iface, IDispatch **p)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_createDocumentFromUrl(IHTMLDocument4 *iface, BSTR bstrUrl,
        BSTR bstrOptions, IHTMLDocument2 **newDoc)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    FIXME("(%p)->(%s %s %p)\n", This, debugstr_w(bstrUrl), debugstr_w(bstrOptions), newDoc);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_put_media(IHTMLDocument4 *iface, BSTR v)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_get_media(IHTMLDocument4 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_createEventObject(IHTMLDocument4 *iface,
        VARIANT *pvarEventObject, IHTMLEventObj **ppEventObj)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_variant(pvarEventObject), ppEventObj);

    if(pvarEventObject && V_VT(pvarEventObject) != VT_ERROR && V_VT(pvarEventObject) != VT_EMPTY) {
        FIXME("unsupported pvarEventObject %s\n", debugstr_variant(pvarEventObject));
        return E_NOTIMPL;
    }

    return create_event_obj(ppEventObj);
}

static HRESULT WINAPI HTMLDocument4_fireEvent(IHTMLDocument4 *iface, BSTR bstrEventName,
        VARIANT *pvarEventObject, VARIANT_BOOL *pfCanceled)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);

    TRACE("(%p)->(%s %p %p)\n", This, debugstr_w(bstrEventName), pvarEventObject, pfCanceled);

    return dispatch_event(&This->doc_node->node, bstrEventName, pvarEventObject, pfCanceled);
}

static HRESULT WINAPI HTMLDocument4_createRenderStyle(IHTMLDocument4 *iface, BSTR v,
        IHTMLRenderStyle **ppIHTMLRenderStyle)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(v), ppIHTMLRenderStyle);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_put_oncontrolselect(IHTMLDocument4 *iface, VARIANT v)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_get_oncontrolselect(IHTMLDocument4 *iface, VARIANT *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_get_URLEncoded(IHTMLDocument4 *iface, BSTR *p)
{
    HTMLDocument *This = impl_from_IHTMLDocument4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static const IHTMLDocument4Vtbl HTMLDocument4Vtbl = {
    HTMLDocument4_QueryInterface,
    HTMLDocument4_AddRef,
    HTMLDocument4_Release,
    HTMLDocument4_GetTypeInfoCount,
    HTMLDocument4_GetTypeInfo,
    HTMLDocument4_GetIDsOfNames,
    HTMLDocument4_Invoke,
    HTMLDocument4_focus,
    HTMLDocument4_hasFocus,
    HTMLDocument4_put_onselectionchange,
    HTMLDocument4_get_onselectionchange,
    HTMLDocument4_get_namespace,
    HTMLDocument4_createDocumentFromUrl,
    HTMLDocument4_put_media,
    HTMLDocument4_get_media,
    HTMLDocument4_createEventObject,
    HTMLDocument4_fireEvent,
    HTMLDocument4_createRenderStyle,
    HTMLDocument4_put_oncontrolselect,
    HTMLDocument4_get_oncontrolselect,
    HTMLDocument4_get_URLEncoded
};

void HTMLDocument_HTMLDocument3_Init(HTMLDocument *This)
{
    This->IHTMLDocument3_iface.lpVtbl = &HTMLDocument3Vtbl;
    This->IHTMLDocument4_iface.lpVtbl = &HTMLDocument4Vtbl;
}
