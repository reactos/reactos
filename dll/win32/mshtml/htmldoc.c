/*
 * Copyright 2005-2009 Jacek Caban for CodeWeavers
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
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wininet.h"
#include "ole2.h"
#include "perhist.h"
#include "mshtmdid.h"
#include "mshtmcid.h"

#include "wine/debug.h"

#include "mshtml_private.h"
#include "htmlevent.h"
#include "pluginhost.h"
#include "binding.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

static HRESULT create_document_fragment(nsIDOMNode *nsnode, HTMLDocumentNode *doc_node, HTMLDocumentNode **ret);

HRESULT get_doc_elem_by_id(HTMLDocumentNode *doc, const WCHAR *id, HTMLElement **ret)
{
    nsIDOMNodeList *nsnode_list;
    nsIDOMNode *nsnode = NULL;
    nsIDOMElement *nselem;
    nsAString id_str;
    nsresult nsres;
    HRESULT hres;

    if(!doc->dom_document) {
        WARN("NULL dom_document\n");
        return E_UNEXPECTED;
    }

    nsAString_InitDepend(&id_str, id);
    /* get element by id attribute */
    nsres = nsIDOMDocument_GetElementById(doc->dom_document, &id_str, &nselem);
    if(FAILED(nsres)) {
        ERR("GetElementById failed: %08lx\n", nsres);
        nsAString_Finish(&id_str);
        return E_FAIL;
    }

    /* get first element by name attribute */
    if(doc->html_document) {
        nsres = nsIDOMHTMLDocument_GetElementsByName(doc->html_document, &id_str, &nsnode_list);
        nsAString_Finish(&id_str);
        if(FAILED(nsres)) {
            ERR("getElementsByName failed: %08lx\n", nsres);
            if(nselem)
                nsIDOMElement_Release(nselem);
            return E_FAIL;
        }

        nsres = nsIDOMNodeList_Item(nsnode_list, 0, &nsnode);
        nsIDOMNodeList_Release(nsnode_list);
        assert(nsres == NS_OK);
    }

    if(nsnode && nselem) {
        UINT16 pos;

        nsres = nsIDOMNode_CompareDocumentPosition(nsnode, (nsIDOMNode*)nselem, &pos);
        if(NS_FAILED(nsres)) {
            FIXME("CompareDocumentPosition failed: 0x%08lx\n", nsres);
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

    hres = get_element(nselem, ret);
    nsIDOMElement_Release(nselem);
    return hres;
}

UINT get_document_charset(HTMLDocumentNode *doc)
{
    nsAString charset_str;
    UINT ret = 0;
    nsresult nsres;

    if(doc->charset)
        return doc->charset;

    nsAString_Init(&charset_str, NULL);
    nsres = nsIDOMDocument_GetCharacterSet(doc->dom_document, &charset_str);
    if(NS_SUCCEEDED(nsres)) {
        const PRUnichar *charset;

        nsAString_GetData(&charset_str, &charset);

        if(*charset) {
            BSTR str = SysAllocString(charset);
            ret = cp_from_charset_string(str);
            SysFreeString(str);
        }
    }else {
        ERR("GetCharset failed: %08lx\n", nsres);
    }
    nsAString_Finish(&charset_str);

    if(!ret)
        return CP_UTF8;

    return doc->charset = ret;
}

typedef struct {
    HTMLDOMNode node;
    IDOMDocumentType IDOMDocumentType_iface;
} DocumentType;

static inline DocumentType *impl_from_IDOMDocumentType(IDOMDocumentType *iface)
{
    return CONTAINING_RECORD(iface, DocumentType, IDOMDocumentType_iface);
}

DISPEX_IDISPATCH_IMPL(DocumentType, IDOMDocumentType,
                      impl_from_IDOMDocumentType(iface)->node.event_target.dispex)

static HRESULT WINAPI DocumentType_get_name(IDOMDocumentType *iface, BSTR *p)
{
    DocumentType *This = impl_from_IDOMDocumentType(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMDocumentType_GetName((nsIDOMDocumentType*)This->node.nsnode, &nsstr);
    return return_nsstr(nsres, &nsstr, p);
}

static HRESULT WINAPI DocumentType_get_entities(IDOMDocumentType *iface, IDispatch **p)
{
    DocumentType *This = impl_from_IDOMDocumentType(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI DocumentType_get_notations(IDOMDocumentType *iface, IDispatch **p)
{
    DocumentType *This = impl_from_IDOMDocumentType(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI DocumentType_get_publicId(IDOMDocumentType *iface, VARIANT *p)
{
    DocumentType *This = impl_from_IDOMDocumentType(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI DocumentType_get_systemId(IDOMDocumentType *iface, VARIANT *p)
{
    DocumentType *This = impl_from_IDOMDocumentType(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static HRESULT WINAPI DocumentType_get_internalSubset(IDOMDocumentType *iface, VARIANT *p)
{
    DocumentType *This = impl_from_IDOMDocumentType(iface);

    FIXME("(%p)->(%p)\n", This, p);

    return E_NOTIMPL;
}

static const IDOMDocumentTypeVtbl DocumentTypeVtbl = {
    DocumentType_QueryInterface,
    DocumentType_AddRef,
    DocumentType_Release,
    DocumentType_GetTypeInfoCount,
    DocumentType_GetTypeInfo,
    DocumentType_GetIDsOfNames,
    DocumentType_Invoke,
    DocumentType_get_name,
    DocumentType_get_entities,
    DocumentType_get_notations,
    DocumentType_get_publicId,
    DocumentType_get_systemId,
    DocumentType_get_internalSubset
};

static inline DocumentType *DocumentType_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, DocumentType, node);
}

static inline DocumentType *DocumentType_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, DocumentType, node.event_target.dispex);
}

static HRESULT DocumentType_clone(HTMLDOMNode *iface, nsIDOMNode *nsnode, HTMLDOMNode **ret)
{
    DocumentType *This = DocumentType_from_HTMLDOMNode(iface);

    return create_doctype_node(This->node.doc, nsnode, ret);
}

static const cpc_entry_t DocumentType_cpc[] = {{NULL}};

static const NodeImplVtbl DocumentTypeImplVtbl = {
    .cpc_entries           = DocumentType_cpc,
    .clone                 = DocumentType_clone
};

static void *DocumentType_query_interface(DispatchEx *dispex, REFIID riid)
{
    DocumentType *This = DocumentType_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IDOMDocumentType, riid))
        return &This->IDOMDocumentType_iface;

    return HTMLDOMNode_query_interface(&This->node.event_target.dispex, riid);
}

static nsISupports *DocumentType_get_gecko_target(DispatchEx *dispex)
{
    DocumentType *This = DocumentType_from_DispatchEx(dispex);
    return (nsISupports*)This->node.nsnode;
}

static EventTarget *DocumentType_get_parent_event_target(DispatchEx *dispex)
{
    DocumentType *This = DocumentType_from_DispatchEx(dispex);
    nsIDOMNode *nsnode;
    HTMLDOMNode *node;
    nsresult nsres;
    HRESULT hres;

    nsres = nsIDOMNode_GetParentNode(This->node.nsnode, &nsnode);
    assert(nsres == NS_OK);
    if(!nsnode)
        return NULL;

    hres = get_node(nsnode, TRUE, &node);
    nsIDOMNode_Release(nsnode);
    if(FAILED(hres))
        return NULL;

    return &node->event_target;
}

static IHTMLEventObj *DocumentType_set_current_event(DispatchEx *dispex, IHTMLEventObj *event)
{
    DocumentType *This = DocumentType_from_DispatchEx(dispex);
    return default_set_current_event(This->node.doc->window, event);
}

static const event_target_vtbl_t DocumentType_event_target_vtbl = {
    {
        .query_interface     = DocumentType_query_interface,
        .destructor          = HTMLDOMNode_destructor,
        .traverse            = HTMLDOMNode_traverse,
        .unlink              = HTMLDOMNode_unlink
    },
    .get_gecko_target        = DocumentType_get_gecko_target,
    .get_parent_event_target = DocumentType_get_parent_event_target,
    .set_current_event       = DocumentType_set_current_event
};

static const tid_t DocumentType_iface_tids[] = {
    IDOMDocumentType_tid,
    0
};

dispex_static_data_t DocumentType_dispex = {
    .id           = PROT_DocumentType,
    .prototype_id = PROT_Node,
    .vtbl         = &DocumentType_event_target_vtbl.dispex_vtbl,
    .disp_tid     = DispDOMDocumentType_tid,
    .iface_tids   = DocumentType_iface_tids,
    .init_info    = HTMLDOMNode_init_dispex_info,
};

HRESULT create_doctype_node(HTMLDocumentNode *doc, nsIDOMNode *nsnode, HTMLDOMNode **ret)
{
    nsIDOMDocumentType *nsdoctype;
    DocumentType *doctype;
    nsresult nsres;

    if(!(doctype = calloc(1, sizeof(*doctype))))
        return E_OUTOFMEMORY;

    nsres = nsIDOMNode_QueryInterface(nsnode, &IID_nsIDOMDocumentType, (void**)&nsdoctype);
    assert(nsres == NS_OK);

    doctype->node.vtbl = &DocumentTypeImplVtbl;
    doctype->IDOMDocumentType_iface.lpVtbl = &DocumentTypeVtbl;
    HTMLDOMNode_Init(doc, &doctype->node, (nsIDOMNode*)nsdoctype, &DocumentType_dispex);
    nsIDOMDocumentType_Release(nsdoctype);

    *ret = &doctype->node;
    return S_OK;
}

static inline HTMLDocumentNode *impl_from_IHTMLDocument2(IHTMLDocument2 *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IHTMLDocument2_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLDocument, IHTMLDocument2,
                      impl_from_IHTMLDocument2(iface)->node.event_target.dispex)

static HRESULT WINAPI HTMLDocument_get_Script(IHTMLDocument2 *iface, IDispatch **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    hres = IHTMLDocument7_get_parentWindow(&This->IHTMLDocument7_iface, (IHTMLWindow2**)p);
    return hres == S_OK && !*p ? E_PENDING : hres;
}

static HRESULT WINAPI HTMLDocument_get_all(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);
    *p = create_all_collection(&This->node, FALSE);

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_body(IHTMLDocument2 *iface, IHTMLElement **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    nsIDOMElement *nsbody = NULL;
    HTMLElement *element;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->html_document) {
        nsres = nsIDOMHTMLDocument_GetBody(This->html_document, (nsIDOMHTMLElement **)&nsbody);
        if(NS_FAILED(nsres)) {
            TRACE("Could not get body: %08lx\n", nsres);
            return E_UNEXPECTED;
        }
    }else {
        nsAString nsnode_name;
        nsIDOMDocumentFragment *frag;

        nsres = nsIDOMNode_QueryInterface(This->node.nsnode, &IID_nsIDOMDocumentFragment, (void**)&frag);
        if(!NS_FAILED(nsres)) {
            nsAString_InitDepend(&nsnode_name, L"BODY");
            nsIDOMDocumentFragment_QuerySelector(frag, &nsnode_name, &nsbody);
            nsAString_Finish(&nsnode_name);
            nsIDOMDocumentFragment_Release(frag);
        }
    }

    if(!nsbody) {
        *p = NULL;
        return S_OK;
    }

    hres = get_element(nsbody, &element);
    nsIDOMElement_Release(nsbody);
    if(FAILED(hres))
        return hres;

    *p = &element->IHTMLElement_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_activeElement(IHTMLDocument2 *iface, IHTMLElement **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    nsIDOMElement *nselem;
    HTMLElement *elem;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->dom_document) {
        *p = NULL;
        return S_OK;
    }

    /*
     * NOTE: Gecko may return an active element even if the document is not visible.
     * IE returns NULL in this case.
     */
    nsres = nsIDOMDocument_GetActiveElement(This->dom_document, &nselem);
    if(NS_FAILED(nsres)) {
        ERR("GetActiveElement failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if(!nselem) {
        *p = NULL;
        return S_OK;
    }

    hres = get_element(nselem, &elem);
    nsIDOMElement_Release(nselem);
    if(FAILED(hres))
        return hres;

    *p = &elem->IHTMLElement_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_images(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    nsIDOMHTMLCollection *nscoll = NULL;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_INVALIDARG;

    *p = NULL;

    if(!This->dom_document) {
        WARN("NULL dom_document\n");
        return E_UNEXPECTED;
    }

    if(!This->html_document) {
        FIXME("Not implemented for XML document\n");
        return E_NOTIMPL;
    }

    nsres = nsIDOMHTMLDocument_GetImages(This->html_document, &nscoll);
    if(NS_FAILED(nsres)) {
        ERR("GetImages failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if(nscoll) {
        *p = create_collection_from_htmlcol(nscoll, &This->node.event_target.dispex);
        nsIDOMHTMLCollection_Release(nscoll);
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_applets(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    nsIDOMHTMLCollection *nscoll = NULL;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_INVALIDARG;

    *p = NULL;

    if(!This->dom_document) {
        WARN("NULL dom_document\n");
        return E_UNEXPECTED;
    }

    if(!This->html_document) {
        FIXME("Not implemented for XML document\n");
        return E_NOTIMPL;
    }

    nsres = nsIDOMHTMLDocument_GetApplets(This->html_document, &nscoll);
    if(NS_FAILED(nsres)) {
        ERR("GetApplets failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if(nscoll) {
        *p = create_collection_from_htmlcol(nscoll, &This->node.event_target.dispex);
        nsIDOMHTMLCollection_Release(nscoll);
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_links(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    nsIDOMHTMLCollection *nscoll = NULL;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_INVALIDARG;

    *p = NULL;

    if(!This->dom_document) {
        WARN("NULL dom_document\n");
        return E_UNEXPECTED;
    }

    if(!This->html_document) {
        FIXME("Not implemented for XML document\n");
        return E_NOTIMPL;
    }

    nsres = nsIDOMHTMLDocument_GetLinks(This->html_document, &nscoll);
    if(NS_FAILED(nsres)) {
        ERR("GetLinks failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if(nscoll) {
        *p = create_collection_from_htmlcol(nscoll, &This->node.event_target.dispex);
        nsIDOMHTMLCollection_Release(nscoll);
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_forms(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    nsIDOMHTMLCollection *nscoll = NULL;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_INVALIDARG;

    *p = NULL;

    if(!This->dom_document) {
        WARN("NULL dom_document\n");
        return E_UNEXPECTED;
    }

    if(!This->html_document) {
        FIXME("Not implemented for XML document\n");
        return E_NOTIMPL;
    }

    nsres = nsIDOMHTMLDocument_GetForms(This->html_document, &nscoll);
    if(NS_FAILED(nsres)) {
        ERR("GetForms failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if(nscoll) {
        *p = create_collection_from_htmlcol(nscoll, &This->node.event_target.dispex);
        nsIDOMHTMLCollection_Release(nscoll);
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_anchors(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    nsIDOMHTMLCollection *nscoll = NULL;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_INVALIDARG;

    *p = NULL;

    if(!This->dom_document) {
        WARN("NULL dom_document\n");
        return E_UNEXPECTED;
    }

    if(!This->html_document) {
        FIXME("Not implemented for XML document\n");
        return E_NOTIMPL;
    }

    nsres = nsIDOMHTMLDocument_GetAnchors(This->html_document, &nscoll);
    if(NS_FAILED(nsres)) {
        ERR("GetAnchors failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if(nscoll) {
        *p = create_collection_from_htmlcol(nscoll, &This->node.event_target.dispex);
        nsIDOMHTMLCollection_Release(nscoll);
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_put_title(IHTMLDocument2 *iface, BSTR v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->dom_document) {
        WARN("NULL dom_document\n");
        return E_UNEXPECTED;
    }

    nsAString_InitDepend(&nsstr, v);
    nsres = nsIDOMDocument_SetTitle(This->dom_document, &nsstr);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres))
        ERR("SetTitle failed: %08lx\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_title(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    const PRUnichar *ret;
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->dom_document) {
        WARN("NULL dom_document\n");
        return E_UNEXPECTED;
    }


    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMDocument_GetTitle(This->dom_document, &nsstr);
    if (NS_SUCCEEDED(nsres)) {
        nsAString_GetData(&nsstr, &ret);
        *p = SysAllocString(ret);
    }
    nsAString_Finish(&nsstr);

    if(NS_FAILED(nsres)) {
        ERR("GetTitle failed: %08lx\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_scripts(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    nsIDOMHTMLCollection *nscoll = NULL;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_INVALIDARG;

    *p = NULL;

    if(!This->dom_document) {
        WARN("NULL dom_document\n");
        return E_UNEXPECTED;
    }

    if(!This->html_document) {
        FIXME("Not implemented for XML document\n");
        return E_NOTIMPL;
    }

    nsres = nsIDOMHTMLDocument_GetScripts(This->html_document, &nscoll);
    if(NS_FAILED(nsres)) {
        ERR("GetImages failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if(nscoll) {
        *p = create_collection_from_htmlcol(nscoll, &This->node.event_target.dispex);
        nsIDOMHTMLCollection_Release(nscoll);
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_put_designMode(IHTMLDocument2 *iface, BSTR v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    HTMLDocumentObj *doc_obj;
    HRESULT hres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(wcsicmp(v, L"on")) {
        FIXME("Unsupported arg %s\n", debugstr_w(v));
        return E_NOTIMPL;
    }

    doc_obj = This->doc_obj;
    IUnknown_AddRef(doc_obj->outer_unk);
    hres = setup_edit_mode(doc_obj);
    IUnknown_Release(doc_obj->outer_unk);
    if(FAILED(hres))
        return hres;

    call_property_onchanged(&This->cp_container, DISPID_IHTMLDOCUMENT2_DESIGNMODE);
    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_designMode(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p) always returning Off\n", This, p);

    if(!p)
        return E_INVALIDARG;

    *p = SysAllocString(L"Off");

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_selection(IHTMLDocument2 *iface, IHTMLSelectionObject **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    nsISelection *nsselection;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->html_document) {
        FIXME("Not implemented for XML document\n");
        return E_NOTIMPL;
    }

    nsres = nsIDOMHTMLDocument_GetSelection(This->html_document, &nsselection);
    if(NS_FAILED(nsres)) {
        ERR("GetSelection failed: %08lx\n", nsres);
        return E_FAIL;
    }

    return HTMLSelectionObject_Create(This, nsselection, p);
}

static HRESULT WINAPI HTMLDocument_get_readyState(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", iface, p);

    if(!p)
        return E_POINTER;

    return get_readystate_string(This->window && This->window->base.outer_window ? This->window->base.outer_window->readystate : 0, p);
}

static HRESULT WINAPI HTMLDocument_get_frames(IHTMLDocument2 *iface, IHTMLFramesCollection2 **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->window) {
        /* Not implemented by IE */
        return E_NOTIMPL;
    }
    if(!This->window->base.outer_window)
        return E_FAIL;
    return IHTMLWindow2_get_frames(&This->window->base.outer_window->base.IHTMLWindow2_iface, p);
}

static HRESULT WINAPI HTMLDocument_get_embeds(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_plugins(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_alinkColor(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_alinkColor(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_bgColor(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    IHTMLElement *element = NULL;
    IHTMLBodyElement *body;
    HRESULT hr;

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    hr = IHTMLDocument2_get_body(iface, &element);
    if (FAILED(hr))
    {
        ERR("Failed to get body (0x%08lx)\n", hr);
        return hr;
    }

    if(!element)
    {
        FIXME("Empty body element.\n");
        return hr;
    }

    hr = IHTMLElement_QueryInterface(element, &IID_IHTMLBodyElement, (void**)&body);
    if (SUCCEEDED(hr))
    {
        hr = IHTMLBodyElement_put_bgColor(body, v);
        IHTMLBodyElement_Release(body);
    }
    IHTMLElement_Release(element);

    return hr;
}

static HRESULT WINAPI HTMLDocument_get_bgColor(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    nsAString nsstr;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->dom_document) {
        WARN("NULL dom_document\n");
        return E_UNEXPECTED;
    }

    if(!This->html_document) {
        FIXME("Not implemented for XML document\n");
        return E_NOTIMPL;
    }

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMHTMLDocument_GetBgColor(This->html_document, &nsstr);
    hres = return_nsstr_variant(nsres, &nsstr, NSSTR_COLOR, p);
    if(hres == S_OK && V_VT(p) == VT_BSTR && !V_BSTR(p)) {
        TRACE("default #ffffff\n");
        if(!(V_BSTR(p) = SysAllocString(L"#ffffff")))
            hres = E_OUTOFMEMORY;
    }
    return hres;
}

static HRESULT WINAPI HTMLDocument_put_fgColor(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_fgColor(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_linkColor(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_linkColor(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_vlinkColor(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_vlinkColor(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_referrer(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_InitDepend(&nsstr, NULL);
    nsres = nsIDOMDocument_GetReferrer(This->dom_document, &nsstr);
    return return_nsstr(nsres, &nsstr, p);
}

static HRESULT WINAPI HTMLDocument_get_location(IHTMLDocument2 *iface, IHTMLLocation **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->dom_document || !This->window) {
        WARN("NULL window\n");
        return E_UNEXPECTED;
    }

    return IHTMLWindow2_get_location(&This->window->base.IHTMLWindow2_iface, p);
}

static HRESULT WINAPI HTMLDocument_get_lastModified(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMDocument_GetLastModified(This->dom_document, &nsstr);
    return return_nsstr(nsres, &nsstr, p);
}

static HRESULT WINAPI HTMLDocument_put_URL(IHTMLDocument2 *iface, BSTR v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->window || !This->window->base.outer_window) {
        FIXME("No window available\n");
        return E_FAIL;
    }

    return navigate_url(This->window->base.outer_window, v,
                        This->window->base.outer_window->uri, BINDING_NAVIGATED);
}

static HRESULT WINAPI HTMLDocument_get_URL(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", iface, p);

    if(This->window && !This->window->base.outer_window) {
        WARN("detached document\n");
        return E_FAIL;
    }

    if(This->window && This->window->base.outer_window->url)
        *p = SysAllocString(This->window->base.outer_window->url);
    else
        *p = SysAllocString(L"about:blank");
    return *p ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI HTMLDocument_put_domain(IHTMLDocument2 *iface, BSTR v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->html_document) {
        FIXME("Not implemented for XML document\n");
        return E_NOTIMPL;
    }

    nsAString_InitDepend(&nsstr, v);
    nsres = nsIDOMHTMLDocument_SetDomain(This->html_document, &nsstr);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres)) {
        ERR("SetDomain failed: %08lx\n", nsres);
        return E_INVALIDARG;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_domain(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->html_document) {
        FIXME("Not implemented for XML document\n");
        return E_NOTIMPL;
    }

    if(This->window && (!This->window->base.outer_window || !This->window->base.outer_window->uri))
        return E_FAIL;

    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMHTMLDocument_GetDomain(This->html_document, &nsstr);
    if(NS_SUCCEEDED(nsres) && This->window) {
        const PRUnichar *str;
        HRESULT hres;

        nsAString_GetData(&nsstr, &str);
        if(!*str) {
            TRACE("Gecko returned empty string, fallback to loaded URL.\n");
            nsAString_Finish(&nsstr);
            hres = IUri_GetHost(This->window->base.outer_window->uri, p);
            return FAILED(hres) ? hres : S_OK;
        }
    }

    return return_nsstr(nsres, &nsstr, p);
}

static HRESULT WINAPI HTMLDocument_put_cookie(IHTMLDocument2 *iface, BSTR v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    BOOL bret;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->window)
        return S_OK;
    if(!This->window->base.outer_window)
        return E_FAIL;

    bret = InternetSetCookieExW(This->window->base.outer_window->url, NULL, v, 0, 0);
    if(!bret) {
        FIXME("InternetSetCookieExW failed: %lu\n", GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_cookie(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    DWORD size;
    BOOL bret;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->window) {
        *p = NULL;
        return S_OK;
    }
    if(!This->window->base.outer_window)
        return E_FAIL;

    size = 0;
    bret = InternetGetCookieExW(This->window->base.outer_window->url, NULL, NULL, &size, 0, NULL);
    if(!bret && GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        WARN("InternetGetCookieExW failed: %lu\n", GetLastError());
        *p = NULL;
        return S_OK;
    }

    if(!size) {
        *p = NULL;
        return S_OK;
    }

    *p = SysAllocStringLen(NULL, size/sizeof(WCHAR)-1);
    if(!*p)
        return E_OUTOFMEMORY;

    bret = InternetGetCookieExW(This->window->base.outer_window->url, NULL, *p, &size, 0, NULL);
    if(!bret) {
        ERR("InternetGetCookieExW failed: %lu\n", GetLastError());
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_put_expando(IHTMLDocument2 *iface, VARIANT_BOOL v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_expando(IHTMLDocument2 *iface, VARIANT_BOOL *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_charset(IHTMLDocument2 *iface, BSTR v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s) returning S_OK\n", This, debugstr_w(v));
    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_charset(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLDocument7_get_characterSet(&This->IHTMLDocument7_iface, p);
}

static HRESULT WINAPI HTMLDocument_put_defaultCharset(IHTMLDocument2 *iface, BSTR v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_defaultCharset(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = charset_string_from_cp(GetACP());
    return *p ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI HTMLDocument_get_mimeType(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    const PRUnichar *content_type;
    nsAString nsstr;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    *p = NULL;

    if(This->window && !This->window->navigation_start_time)
        return (*p = SysAllocString(L"")) ? S_OK : E_FAIL;

    nsAString_InitDepend(&nsstr, NULL);
    nsres = nsIDOMDocument_GetContentType(This->dom_document, &nsstr);
    if(NS_FAILED(nsres))
        return map_nsresult(nsres);

    nsAString_GetData(&nsstr, &content_type);
    hres = get_mime_type_display_name(content_type, p);
    nsAString_Finish(&nsstr);
    return hres;
}

static HRESULT WINAPI HTMLDocument_get_fileSize(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_fileCreatedDate(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_fileModifiedDate(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_fileUpdatedDate(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_security(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_protocol(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_nameProp(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT document_write(HTMLDocumentNode *This, SAFEARRAY *psarray, BOOL ln)
{
    VARIANT *var, tmp;
    JSContext *jsctx;
    nsAString nsstr;
    ULONG i, argc;
    nsresult nsres;
    HRESULT hres;

    if(!This->dom_document) {
        WARN("NULL dom_document\n");
        return E_UNEXPECTED;
    }

    if(!This->html_document) {
        FIXME("Not implemented for XML document\n");
        return E_NOTIMPL;
    }

    if (!psarray)
        return S_OK;

    if(psarray->cDims != 1) {
        FIXME("cDims=%d\n", psarray->cDims);
        return E_INVALIDARG;
    }

    hres = SafeArrayAccessData(psarray, (void**)&var);
    if(FAILED(hres)) {
        WARN("SafeArrayAccessData failed: %08lx\n", hres);
        return hres;
    }

    V_VT(&tmp) = VT_EMPTY;

    jsctx = get_context_from_document(This->dom_document);
    argc = psarray->rgsabound[0].cElements;
    for(i=0; i < argc; i++) {
        if(V_VT(var+i) == VT_BSTR) {
            nsAString_InitDepend(&nsstr, V_BSTR(var+i));
        }else {
            hres = VariantChangeTypeEx(&tmp, var+i, MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT), 0, VT_BSTR);
            if(FAILED(hres)) {
                WARN("Could not convert %s to string\n", debugstr_variant(var+i));
                break;
            }
            nsAString_InitDepend(&nsstr, V_BSTR(&tmp));
        }

        if(!ln || i != argc-1)
            nsres = nsIDOMHTMLDocument_Write(This->html_document, &nsstr, jsctx);
        else
            nsres = nsIDOMHTMLDocument_Writeln(This->html_document, &nsstr, jsctx);
        nsAString_Finish(&nsstr);
        if(V_VT(var+i) != VT_BSTR)
            VariantClear(&tmp);
        if(NS_FAILED(nsres)) {
            ERR("Write failed: %08lx\n", nsres);
            hres = E_FAIL;
            break;
        }
    }

    SafeArrayUnaccessData(psarray);

    return hres;
}

static HRESULT WINAPI HTMLDocument_write(IHTMLDocument2 *iface, SAFEARRAY *psarray)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", iface, psarray);

    return document_write(This, psarray, FALSE);
}

static HRESULT WINAPI HTMLDocument_writeln(IHTMLDocument2 *iface, SAFEARRAY *psarray)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, psarray);

    return document_write(This, psarray, TRUE);
}

static HRESULT WINAPI HTMLDocument_open(IHTMLDocument2 *iface, BSTR url, VARIANT name,
        VARIANT features, VARIANT replace, IDispatch **pomWindowResult)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    nsISupports *tmp;
    nsresult nsres;

    TRACE("(%p)->(%s %s %s %s %p)\n", This, debugstr_w(url), debugstr_variant(&name),
          debugstr_variant(&features), debugstr_variant(&replace), pomWindowResult);

    *pomWindowResult = NULL;

    if(!This->window || !This->window->base.outer_window)
        return E_FAIL;

    if(!This->dom_document) {
        ERR("!dom_document\n");
        return E_NOTIMPL;
    }

    if(!This->html_document) {
        FIXME("Not implemented for XML document\n");
        return E_NOTIMPL;
    }

    if(!url || wcscmp(url, L"text/html") || V_VT(&name) != VT_ERROR
       || V_VT(&features) != VT_ERROR || V_VT(&replace) != VT_ERROR)
        FIXME("unsupported args\n");

    nsres = nsIDOMHTMLDocument_Open(This->html_document, NULL, NULL, NULL,
            get_context_from_document(This->dom_document), 0, &tmp);
    if(NS_FAILED(nsres)) {
        ERR("Open failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if(tmp)
        nsISupports_Release(tmp);

    *pomWindowResult = (IDispatch*)&This->window->base.outer_window->base.IHTMLWindow2_iface;
    IHTMLWindow2_AddRef(&This->window->base.outer_window->base.IHTMLWindow2_iface);
    return S_OK;
}

static HRESULT WINAPI HTMLDocument_close(IHTMLDocument2 *iface)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    nsresult nsres;

    TRACE("(%p)\n", This);

    if(!This->dom_document) {
        ERR("!dom_document\n");
        return E_NOTIMPL;
    }

    if(!This->html_document) {
        FIXME("Not implemented for XML document\n");
        return E_NOTIMPL;
    }

    nsres = nsIDOMHTMLDocument_Close(This->html_document);
    if(NS_FAILED(nsres)) {
        ERR("Close failed: %08lx\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_clear(IHTMLDocument2 *iface)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    nsresult nsres;

    TRACE("(%p)\n", This);

    if(!This->html_document) {
        FIXME("Not implemented for XML document\n");
        return E_NOTIMPL;
    }

    nsres = nsIDOMHTMLDocument_Clear(This->html_document);
    if(NS_FAILED(nsres)) {
        ERR("Clear failed: %08lx\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static const struct {
    const WCHAR *name;
    OLECMDID id;
}command_names[] = {
    {L"copy", IDM_COPY},
    {L"cut", IDM_CUT},
    {L"fontname", IDM_FONTNAME},
    {L"fontsize", IDM_FONTSIZE},
    {L"indent", IDM_INDENT},
    {L"insertorderedlist", IDM_ORDERLIST},
    {L"insertunorderedlist", IDM_UNORDERLIST},
    {L"outdent", IDM_OUTDENT},
    {L"paste", IDM_PASTE},
    {L"respectvisibilityindesign", IDM_RESPECTVISIBILITY_INDESIGN}
};

static BOOL cmdid_from_string(const WCHAR *str, OLECMDID *cmdid)
{
    int i;

    for(i = 0; i < ARRAY_SIZE(command_names); i++) {
        if(!wcsicmp(command_names[i].name, str)) {
            *cmdid = command_names[i].id;
            return TRUE;
        }
    }

    FIXME("Unknown command %s\n", debugstr_w(str));
    return FALSE;
}

static HRESULT WINAPI HTMLDocument_queryCommandSupported(IHTMLDocument2 *iface, BSTR cmdID,
        VARIANT_BOOL *pfRet)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_queryCommandEnabled(IHTMLDocument2 *iface, BSTR cmdID,
        VARIANT_BOOL *pfRet)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_queryCommandState(IHTMLDocument2 *iface, BSTR cmdID,
        VARIANT_BOOL *pfRet)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_queryCommandIndeterm(IHTMLDocument2 *iface, BSTR cmdID,
        VARIANT_BOOL *pfRet)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_queryCommandText(IHTMLDocument2 *iface, BSTR cmdID,
        BSTR *pfRet)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_queryCommandValue(IHTMLDocument2 *iface, BSTR cmdID,
        VARIANT *pfRet)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_execCommand(IHTMLDocument2 *iface, BSTR cmdID,
        VARIANT_BOOL showUI, VARIANT value, VARIANT_BOOL *pfRet)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    OLECMDID cmdid;
    VARIANT ret;
    HRESULT hres;

    TRACE("(%p)->(%s %x %s %p)\n", This, debugstr_w(cmdID), showUI, debugstr_variant(&value), pfRet);

    if(!cmdid_from_string(cmdID, &cmdid))
        return OLECMDERR_E_NOTSUPPORTED;

    V_VT(&ret) = VT_EMPTY;
    hres = IOleCommandTarget_Exec(&This->IOleCommandTarget_iface, &CGID_MSHTML, cmdid,
            showUI ? 0 : OLECMDEXECOPT_DONTPROMPTUSER, &value, &ret);
    if(FAILED(hres))
        return hres;

    if(V_VT(&ret) != VT_EMPTY) {
        FIXME("Handle ret %s\n", debugstr_variant(&ret));
        VariantClear(&ret);
    }

    *pfRet = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument_execCommandShowHelp(IHTMLDocument2 *iface, BSTR cmdID,
        VARIANT_BOOL *pfRet)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_createElement(IHTMLDocument2 *iface, BSTR eTag,
        IHTMLElement **newElem)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    HTMLElement *elem;
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(eTag), newElem);

    hres = create_element(This, eTag, &elem);
    if(FAILED(hres))
        return hres;

    *newElem = &elem->IHTMLElement_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument_put_onhelp(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_onhelp(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_onclick(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_CLICK, &v);
}

static HRESULT WINAPI HTMLDocument_get_onclick(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_CLICK, p);
}

static HRESULT WINAPI HTMLDocument_put_ondblclick(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_DBLCLICK, &v);
}

static HRESULT WINAPI HTMLDocument_get_ondblclick(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_DBLCLICK, p);
}

static HRESULT WINAPI HTMLDocument_put_onkeyup(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_KEYUP, &v);
}

static HRESULT WINAPI HTMLDocument_get_onkeyup(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_KEYUP, p);
}

static HRESULT WINAPI HTMLDocument_put_onkeydown(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_KEYDOWN, &v);
}

static HRESULT WINAPI HTMLDocument_get_onkeydown(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_KEYDOWN, p);
}

static HRESULT WINAPI HTMLDocument_put_onkeypress(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_KEYPRESS, &v);
}

static HRESULT WINAPI HTMLDocument_get_onkeypress(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_KEYPRESS, p);
}

static HRESULT WINAPI HTMLDocument_put_onmouseup(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_MOUSEUP, &v);
}

static HRESULT WINAPI HTMLDocument_get_onmouseup(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_MOUSEUP, p);
}

static HRESULT WINAPI HTMLDocument_put_onmousedown(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_MOUSEDOWN, &v);
}

static HRESULT WINAPI HTMLDocument_get_onmousedown(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_MOUSEDOWN, p);
}

static HRESULT WINAPI HTMLDocument_put_onmousemove(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_MOUSEMOVE, &v);
}

static HRESULT WINAPI HTMLDocument_get_onmousemove(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_MOUSEMOVE, p);
}

static HRESULT WINAPI HTMLDocument_put_onmouseout(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_MOUSEOUT, &v);
}

static HRESULT WINAPI HTMLDocument_get_onmouseout(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_MOUSEOUT, p);
}

static HRESULT WINAPI HTMLDocument_put_onmouseover(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_MOUSEOVER, &v);
}

static HRESULT WINAPI HTMLDocument_get_onmouseover(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_MOUSEOVER, p);
}

static HRESULT WINAPI HTMLDocument_put_onreadystatechange(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_READYSTATECHANGE, &v);
}

static HRESULT WINAPI HTMLDocument_get_onreadystatechange(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_READYSTATECHANGE, p);
}

static HRESULT WINAPI HTMLDocument_put_onafterupdate(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_onafterupdate(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_onrowexit(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_onrowexit(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_onrowenter(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_onrowenter(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_ondragstart(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_DRAGSTART, &v);
}

static HRESULT WINAPI HTMLDocument_get_ondragstart(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_DRAGSTART, p);
}

static HRESULT WINAPI HTMLDocument_put_onselectstart(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_SELECTSTART, &v);
}

static HRESULT WINAPI HTMLDocument_get_onselectstart(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_SELECTSTART, p);
}

static HRESULT WINAPI HTMLDocument_elementFromPoint(IHTMLDocument2 *iface, LONG x, LONG y,
        IHTMLElement **elementHit)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    nsIDOMElement *nselem;
    HTMLElement *element;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%ld %ld %p)\n", This, x, y, elementHit);

    nsres = nsIDOMDocument_ElementFromPoint(This->dom_document, x, y, &nselem);
    if(NS_FAILED(nsres)) {
        ERR("ElementFromPoint failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if(!nselem) {
        *elementHit = NULL;
        return S_OK;
    }

    hres = get_element(nselem, &element);
    nsIDOMElement_Release(nselem);
    if(FAILED(hres))
        return hres;

    *elementHit = &element->IHTMLElement_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_parentWindow(IHTMLDocument2 *iface, IHTMLWindow2 **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    hres = IHTMLDocument7_get_defaultView(&This->IHTMLDocument7_iface, p);
    return hres == S_OK && !*p ? E_FAIL : hres;
}

static HRESULT WINAPI HTMLDocument_get_styleSheets(IHTMLDocument2 *iface,
        IHTMLStyleSheetsCollection **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    nsIDOMStyleSheetList *nsstylelist;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    *p = NULL;

    if(!This->dom_document) {
        WARN("NULL dom_document\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMDocument_GetStyleSheets(This->dom_document, &nsstylelist);
    if(NS_FAILED(nsres)) {
        ERR("GetStyleSheets failed: %08lx\n", nsres);
        return map_nsresult(nsres);
    }

    hres = create_style_sheet_collection(nsstylelist, This, p);
    nsIDOMStyleSheetList_Release(nsstylelist);
    return hres;
}

static HRESULT WINAPI HTMLDocument_put_onbeforeupdate(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_onbeforeupdate(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_onerrorupdate(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_onerrorupdate(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_toString(IHTMLDocument2 *iface, BSTR *String)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, String);

    return dispex_to_string(&This->node.event_target.dispex, String);
}

static HRESULT WINAPI HTMLDocument_createStyleSheet(IHTMLDocument2 *iface, BSTR bstrHref,
        LONG lIndex, IHTMLStyleSheet **ppnewStyleSheet)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument2(iface);
    nsIDOMHTMLHeadElement *head_elem;
    IHTMLStyleElement *style_elem;
    HTMLElement *elem;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s %ld %p)\n", This, debugstr_w(bstrHref), lIndex, ppnewStyleSheet);

    if(!This->dom_document) {
        FIXME("not a real doc object\n");
        return E_NOTIMPL;
    }

    if(!This->html_document) {
        FIXME("Not implemented for XML document\n");
        return E_NOTIMPL;
    }

    if(lIndex != -1)
        FIXME("Unsupported lIndex %ld\n", lIndex);

    if(bstrHref && *bstrHref) {
        FIXME("semi-stub for href %s\n", debugstr_w(bstrHref));
        return create_style_sheet(NULL, &This->node.event_target.dispex, ppnewStyleSheet);
    }

    hres = create_element(This, L"style", &elem);
    if(FAILED(hres))
        return hres;

    nsres = nsIDOMHTMLDocument_GetHead(This->html_document, &head_elem);
    if(NS_SUCCEEDED(nsres)) {
        nsIDOMNode *head_node, *tmp_node;

        nsres = nsIDOMHTMLHeadElement_QueryInterface(head_elem, &IID_nsIDOMNode, (void**)&head_node);
        nsIDOMHTMLHeadElement_Release(head_elem);
        assert(nsres == NS_OK);

        nsres = nsIDOMNode_AppendChild(head_node, elem->node.nsnode, &tmp_node);
        nsIDOMNode_Release(head_node);
        if(NS_SUCCEEDED(nsres) && tmp_node)
            nsIDOMNode_Release(tmp_node);
    }
    if(NS_FAILED(nsres)) {
        IHTMLElement_Release(&elem->IHTMLElement_iface);
        return E_FAIL;
    }

    hres = IHTMLElement_QueryInterface(&elem->IHTMLElement_iface, &IID_IHTMLStyleElement, (void**)&style_elem);
    assert(hres == S_OK);
    IHTMLElement_Release(&elem->IHTMLElement_iface);

    hres = IHTMLStyleElement_get_styleSheet(style_elem, ppnewStyleSheet);
    IHTMLStyleElement_Release(style_elem);
    return hres;
}

static const IHTMLDocument2Vtbl HTMLDocumentVtbl = {
    HTMLDocument_QueryInterface,
    HTMLDocument_AddRef,
    HTMLDocument_Release,
    HTMLDocument_GetTypeInfoCount,
    HTMLDocument_GetTypeInfo,
    HTMLDocument_GetIDsOfNames,
    HTMLDocument_Invoke,
    HTMLDocument_get_Script,
    HTMLDocument_get_all,
    HTMLDocument_get_body,
    HTMLDocument_get_activeElement,
    HTMLDocument_get_images,
    HTMLDocument_get_applets,
    HTMLDocument_get_links,
    HTMLDocument_get_forms,
    HTMLDocument_get_anchors,
    HTMLDocument_put_title,
    HTMLDocument_get_title,
    HTMLDocument_get_scripts,
    HTMLDocument_put_designMode,
    HTMLDocument_get_designMode,
    HTMLDocument_get_selection,
    HTMLDocument_get_readyState,
    HTMLDocument_get_frames,
    HTMLDocument_get_embeds,
    HTMLDocument_get_plugins,
    HTMLDocument_put_alinkColor,
    HTMLDocument_get_alinkColor,
    HTMLDocument_put_bgColor,
    HTMLDocument_get_bgColor,
    HTMLDocument_put_fgColor,
    HTMLDocument_get_fgColor,
    HTMLDocument_put_linkColor,
    HTMLDocument_get_linkColor,
    HTMLDocument_put_vlinkColor,
    HTMLDocument_get_vlinkColor,
    HTMLDocument_get_referrer,
    HTMLDocument_get_location,
    HTMLDocument_get_lastModified,
    HTMLDocument_put_URL,
    HTMLDocument_get_URL,
    HTMLDocument_put_domain,
    HTMLDocument_get_domain,
    HTMLDocument_put_cookie,
    HTMLDocument_get_cookie,
    HTMLDocument_put_expando,
    HTMLDocument_get_expando,
    HTMLDocument_put_charset,
    HTMLDocument_get_charset,
    HTMLDocument_put_defaultCharset,
    HTMLDocument_get_defaultCharset,
    HTMLDocument_get_mimeType,
    HTMLDocument_get_fileSize,
    HTMLDocument_get_fileCreatedDate,
    HTMLDocument_get_fileModifiedDate,
    HTMLDocument_get_fileUpdatedDate,
    HTMLDocument_get_security,
    HTMLDocument_get_protocol,
    HTMLDocument_get_nameProp,
    HTMLDocument_write,
    HTMLDocument_writeln,
    HTMLDocument_open,
    HTMLDocument_close,
    HTMLDocument_clear,
    HTMLDocument_queryCommandSupported,
    HTMLDocument_queryCommandEnabled,
    HTMLDocument_queryCommandState,
    HTMLDocument_queryCommandIndeterm,
    HTMLDocument_queryCommandText,
    HTMLDocument_queryCommandValue,
    HTMLDocument_execCommand,
    HTMLDocument_execCommandShowHelp,
    HTMLDocument_createElement,
    HTMLDocument_put_onhelp,
    HTMLDocument_get_onhelp,
    HTMLDocument_put_onclick,
    HTMLDocument_get_onclick,
    HTMLDocument_put_ondblclick,
    HTMLDocument_get_ondblclick,
    HTMLDocument_put_onkeyup,
    HTMLDocument_get_onkeyup,
    HTMLDocument_put_onkeydown,
    HTMLDocument_get_onkeydown,
    HTMLDocument_put_onkeypress,
    HTMLDocument_get_onkeypress,
    HTMLDocument_put_onmouseup,
    HTMLDocument_get_onmouseup,
    HTMLDocument_put_onmousedown,
    HTMLDocument_get_onmousedown,
    HTMLDocument_put_onmousemove,
    HTMLDocument_get_onmousemove,
    HTMLDocument_put_onmouseout,
    HTMLDocument_get_onmouseout,
    HTMLDocument_put_onmouseover,
    HTMLDocument_get_onmouseover,
    HTMLDocument_put_onreadystatechange,
    HTMLDocument_get_onreadystatechange,
    HTMLDocument_put_onafterupdate,
    HTMLDocument_get_onafterupdate,
    HTMLDocument_put_onrowexit,
    HTMLDocument_get_onrowexit,
    HTMLDocument_put_onrowenter,
    HTMLDocument_get_onrowenter,
    HTMLDocument_put_ondragstart,
    HTMLDocument_get_ondragstart,
    HTMLDocument_put_onselectstart,
    HTMLDocument_get_onselectstart,
    HTMLDocument_elementFromPoint,
    HTMLDocument_get_parentWindow,
    HTMLDocument_get_styleSheets,
    HTMLDocument_put_onbeforeupdate,
    HTMLDocument_get_onbeforeupdate,
    HTMLDocument_put_onerrorupdate,
    HTMLDocument_get_onerrorupdate,
    HTMLDocument_toString,
    HTMLDocument_createStyleSheet
};

static inline HTMLDocumentNode *impl_from_IHTMLDocument3(IHTMLDocument3 *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IHTMLDocument3_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLDocument3, IHTMLDocument3,
                      impl_from_IHTMLDocument3(iface)->node.event_target.dispex)

static HRESULT WINAPI HTMLDocument3_releaseCapture(IHTMLDocument3 *iface)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_recalc(IHTMLDocument3 *iface, VARIANT_BOOL fForce)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);

    WARN("(%p)->(%x)\n", This, fForce);

    /* Doing nothing here should be fine for us. */
    return S_OK;
}

static HRESULT WINAPI HTMLDocument3_createTextNode(IHTMLDocument3 *iface, BSTR text, IHTMLDOMNode **newTextNode)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    nsIDOMText *nstext;
    HTMLDOMNode *node;
    nsAString text_str;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(text), newTextNode);

    if(!This->dom_document) {
        WARN("NULL dom_document\n");
        return E_UNEXPECTED;
    }

    nsAString_InitDepend(&text_str, text);
    nsres = nsIDOMDocument_CreateTextNode(This->dom_document, &text_str, &nstext);
    nsAString_Finish(&text_str);
    if(NS_FAILED(nsres)) {
        ERR("CreateTextNode failed: %08lx\n", nsres);
        return E_FAIL;
    }

    hres = HTMLDOMTextNode_Create(This, (nsIDOMNode*)nstext, &node);
    nsIDOMText_Release(nstext);
    if(FAILED(hres))
        return hres;

    *newTextNode = &node->IHTMLDOMNode_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument3_get_documentElement(IHTMLDocument3 *iface, IHTMLElement **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    nsIDOMElement *nselem = NULL;
    HTMLElement *element;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->window) {
        if(!This->window->base.outer_window)
            return E_FAIL;
        if(This->window->base.outer_window->readystate == READYSTATE_UNINITIALIZED) {
            *p = NULL;
            return S_OK;
        }
    }

    if(!This->dom_document) {
        WARN("NULL dom_document\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMDocument_GetDocumentElement(This->dom_document, &nselem);
    if(NS_FAILED(nsres)) {
        ERR("GetDocumentElement failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if(!nselem) {
        *p = NULL;
        return S_OK;
    }

    hres = get_element(nselem, &element);
    nsIDOMElement_Release(nselem);
    if(FAILED(hres))
        return hres;

    *p = &element->IHTMLElement_iface;
    return hres;
}

static HRESULT WINAPI HTMLDocument3_get_uniqueID(IHTMLDocument3 *iface, BSTR *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return elem_unique_id(++This->unique_id, p);
}

static HRESULT WINAPI HTMLDocument3_attachEvent(IHTMLDocument3 *iface, BSTR event, IDispatch* pDisp,
        VARIANT_BOOL *pfResult)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);

    TRACE("(%p)->(%s %p %p)\n", This, debugstr_w(event), pDisp, pfResult);

    return attach_event(&This->node.event_target, event, pDisp, pfResult);
}

static HRESULT WINAPI HTMLDocument3_detachEvent(IHTMLDocument3 *iface, BSTR event, IDispatch *pDisp)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(event), pDisp);

    return detach_event(&This->node.event_target, event, pDisp);
}

static HRESULT WINAPI HTMLDocument3_put_onrowsdelete(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_onrowsdelete(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_onrowsinserted(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_onrowsinserted(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_oncellchange(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_oncellchange(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_ondatasetchanged(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_ondatasetchanged(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_ondataavailable(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_ondataavailable(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_ondatasetcomplete(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_ondatasetcomplete(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_onpropertychange(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_onpropertychange(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_dir(IHTMLDocument3 *iface, BSTR v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    nsAString dir_str;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->dom_document) {
        FIXME("NULL dom_document\n");
        return E_UNEXPECTED;
    }

    nsAString_InitDepend(&dir_str, v);
    nsres = nsIDOMDocument_SetDir(This->dom_document, &dir_str);
    nsAString_Finish(&dir_str);
    if(NS_FAILED(nsres)) {
        ERR("SetDir failed: %08lx\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument3_get_dir(IHTMLDocument3 *iface, BSTR *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    nsAString dir_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->dom_document) {
        FIXME("NULL dom_document\n");
        return E_UNEXPECTED;
    }

    nsAString_Init(&dir_str, NULL);
    nsres = nsIDOMDocument_GetDir(This->dom_document, &dir_str);
    return return_nsstr(nsres, &dir_str, p);
}

static HRESULT WINAPI HTMLDocument3_put_oncontextmenu(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);

    TRACE("(%p)->()\n", This);

    return set_doc_event(This, EVENTID_CONTEXTMENU, &v);
}

static HRESULT WINAPI HTMLDocument3_get_oncontextmenu(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_CONTEXTMENU, p);
}

static HRESULT WINAPI HTMLDocument3_put_onstop(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_onstop(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_createDocumentFragment(IHTMLDocument3 *iface, IHTMLDocument2 **ppNewDoc)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    nsIDOMDocumentFragment *doc_frag;
    HTMLDocumentNode *docnode;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, ppNewDoc);

    if(!This->dom_document) {
        FIXME("NULL dom_document\n");
        return E_NOTIMPL;
    }

    nsres = nsIDOMDocument_CreateDocumentFragment(This->dom_document, &doc_frag);
    if(NS_FAILED(nsres)) {
        ERR("CreateDocumentFragment failed: %08lx\n", nsres);
        return E_FAIL;
    }

    hres = create_document_fragment((nsIDOMNode*)doc_frag, This, &docnode);
    nsIDOMDocumentFragment_Release(doc_frag);
    if(FAILED(hres))
        return hres;

    *ppNewDoc = &docnode->IHTMLDocument2_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument3_get_parentDocument(IHTMLDocument3 *iface, IHTMLDocument2 **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_enableDownload(IHTMLDocument3 *iface, VARIANT_BOOL v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_enableDownload(IHTMLDocument3 *iface, VARIANT_BOOL *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_baseUrl(IHTMLDocument3 *iface, BSTR v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_baseUrl(IHTMLDocument3 *iface, BSTR *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_childNodes(IHTMLDocument3 *iface, IDispatch **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLDOMNode_get_childNodes(&This->node.IHTMLDOMNode_iface, p);
}

static HRESULT WINAPI HTMLDocument3_put_inheritStyleSheets(IHTMLDocument3 *iface, VARIANT_BOOL v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_inheritStyleSheets(IHTMLDocument3 *iface, VARIANT_BOOL *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_onbeforeeditfocus(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_onbeforeeditfocus(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_getElementsByName(IHTMLDocument3 *iface, BSTR v,
        IHTMLElementCollection **ppelColl)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    nsIDOMNodeList *node_list = NULL;
    nsAString selector_str;
    WCHAR *selector;
    nsresult nsres;
    static const WCHAR formatW[] = L"*[id=%s],*[name=%s]";

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(v), ppelColl);

    if(!This->dom_document) {
        /* We should probably return an empty collection. */
        FIXME("No dom_document\n");
        return E_NOTIMPL;
    }

    selector = malloc(2 * SysStringLen(v) * sizeof(WCHAR) + sizeof(formatW));
    if(!selector)
        return E_OUTOFMEMORY;
    swprintf(selector, 2*SysStringLen(v) + ARRAY_SIZE(formatW), formatW, v, v);

    /*
     * NOTE: IE getElementsByName implementation differs from Gecko. It searches both name and id attributes.
     * That's why we use CSS selector instead. We should also use name only when it applies to given element
     * types and search should be case insensitive. Those are currently not supported properly.
     */
    nsAString_InitDepend(&selector_str, selector);
    nsres = nsIDOMDocument_QuerySelectorAll(This->dom_document, &selector_str, &node_list);
    nsAString_Finish(&selector_str);
    free(selector);
    if(NS_FAILED(nsres)) {
        ERR("QuerySelectorAll failed: %08lx\n", nsres);
        if(node_list)
            nsIDOMNodeList_Release(node_list);
        return E_FAIL;
    }

    *ppelColl = create_collection_from_nodelist(node_list, &This->node.event_target.dispex);
    nsIDOMNodeList_Release(node_list);
    return S_OK;
}


static HRESULT WINAPI HTMLDocument3_getElementById(IHTMLDocument3 *iface, BSTR v, IHTMLElement **pel)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    HTMLElement *elem;
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(v), pel);

    hres = get_doc_elem_by_id(This, v, &elem);
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
    HTMLDocumentNode *This = impl_from_IHTMLDocument3(iface);
    nsIDOMNodeList *nslist;
    nsAString id_str;
    nsresult nsres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(v), pelColl);

    if(This->dom_document) {
        nsAString_InitDepend(&id_str, v);
        nsres = nsIDOMDocument_GetElementsByTagName(This->dom_document, &id_str, &nslist);
        nsAString_Finish(&id_str);
        if(FAILED(nsres)) {
            ERR("GetElementByName failed: %08lx\n", nsres);
            return E_FAIL;
        }
    }else {
        nsIDOMDocumentFragment *docfrag;
        nsAString nsstr;

        if(v) {
            const WCHAR *ptr;
            for(ptr=v; *ptr; ptr++) {
                if(!iswalnum(*ptr)) {
                    FIXME("Unsupported invalid tag %s\n", debugstr_w(v));
                    return E_NOTIMPL;
                }
            }
        }

        nsres = nsIDOMNode_QueryInterface(This->node.nsnode, &IID_nsIDOMDocumentFragment, (void**)&docfrag);
        if(NS_FAILED(nsres)) {
            ERR("Could not get nsIDOMDocumentFragment iface: %08lx\n", nsres);
            return E_UNEXPECTED;
        }

        nslist = NULL;
        nsAString_InitDepend(&nsstr, v);
        nsres = nsIDOMDocumentFragment_QuerySelectorAll(docfrag, &nsstr, &nslist);
        nsAString_Finish(&nsstr);
        nsIDOMDocumentFragment_Release(docfrag);
        if(NS_FAILED(nsres)) {
            ERR("QuerySelectorAll failed: %08lx\n", nsres);
            if(nslist)
                nsIDOMNodeList_Release(nslist);
            return E_FAIL;
        }
    }


    *pelColl = create_collection_from_nodelist(nslist, &This->node.event_target.dispex);
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
    HTMLDocument3_get_uniqueID,
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

static inline HTMLDocumentNode *impl_from_IHTMLDocument4(IHTMLDocument4 *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IHTMLDocument4_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLDocument4, IHTMLDocument4,
                      impl_from_IHTMLDocument4(iface)->node.event_target.dispex)

static HRESULT WINAPI HTMLDocument4_focus(IHTMLDocument4 *iface)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument4(iface);
    nsIDOMHTMLElement *nsbody;
    nsresult nsres;

    TRACE("(%p)->()\n", This);

    if(!This->html_document) {
        FIXME("Not implemented for XML document\n");
        return E_NOTIMPL;
    }

    nsres = nsIDOMHTMLDocument_GetBody(This->html_document, &nsbody);
    if(NS_FAILED(nsres) || !nsbody) {
        ERR("GetBody failed: %08lx\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMHTMLElement_Focus(nsbody);
    nsIDOMHTMLElement_Release(nsbody);
    if(NS_FAILED(nsres)) {
        ERR("Focus failed: %08lx\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument4_hasFocus(IHTMLDocument4 *iface, VARIANT_BOOL *pfFocus)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument4(iface);
    cpp_bool has_focus;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, pfFocus);

    if(!This->dom_document) {
        FIXME("Unimplemented for fragments.\n");
        return E_NOTIMPL;
    }

    nsres = nsIDOMDocument_HasFocus(This->dom_document, &has_focus);
    assert(nsres == NS_OK);

    *pfFocus = variant_bool(has_focus);
    return S_OK;
}

static HRESULT WINAPI HTMLDocument4_put_onselectionchange(IHTMLDocument4 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument4(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_SELECTIONCHANGE, &v);
}

static HRESULT WINAPI HTMLDocument4_get_onselectionchange(IHTMLDocument4 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument4(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_SELECTIONCHANGE, p);
}

static HRESULT WINAPI HTMLDocument4_get_namespaces(IHTMLDocument4 *iface, IDispatch **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument4(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->namespaces) {
        HRESULT hres;

        hres = create_namespace_collection(This, &This->namespaces);
        if(FAILED(hres))
            return hres;
    }

    IHTMLNamespaceCollection_AddRef(This->namespaces);
    *p = (IDispatch*)This->namespaces;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument4_createDocumentFromUrl(IHTMLDocument4 *iface, BSTR bstrUrl,
        BSTR bstrOptions, IHTMLDocument2 **newDoc)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument4(iface);
    FIXME("(%p)->(%s %s %p)\n", This, debugstr_w(bstrUrl), debugstr_w(bstrOptions), newDoc);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_put_media(IHTMLDocument4 *iface, BSTR v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument4(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_get_media(IHTMLDocument4 *iface, BSTR *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_createEventObject(IHTMLDocument4 *iface,
        VARIANT *pvarEventObject, IHTMLEventObj **ppEventObj)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument4(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_variant(pvarEventObject), ppEventObj);

    if(pvarEventObject && V_VT(pvarEventObject) != VT_ERROR && V_VT(pvarEventObject) != VT_EMPTY) {
        FIXME("unsupported pvarEventObject %s\n", debugstr_variant(pvarEventObject));
        return E_NOTIMPL;
    }

    return create_event_obj(NULL, This, ppEventObj);
}

static HRESULT WINAPI HTMLDocument4_fireEvent(IHTMLDocument4 *iface, BSTR bstrEventName,
        VARIANT *pvarEventObject, VARIANT_BOOL *pfCanceled)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument4(iface);

    TRACE("(%p)->(%s %p %p)\n", This, debugstr_w(bstrEventName), pvarEventObject, pfCanceled);

    return fire_event(&This->node, bstrEventName, pvarEventObject, pfCanceled);
}

static HRESULT WINAPI HTMLDocument4_createRenderStyle(IHTMLDocument4 *iface, BSTR v,
        IHTMLRenderStyle **ppIHTMLRenderStyle)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument4(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(v), ppIHTMLRenderStyle);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_put_oncontrolselect(IHTMLDocument4 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument4(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_get_oncontrolselect(IHTMLDocument4 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument4(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_get_URLUnencoded(IHTMLDocument4 *iface, BSTR *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument4(iface);
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
    HTMLDocument4_get_namespaces,
    HTMLDocument4_createDocumentFromUrl,
    HTMLDocument4_put_media,
    HTMLDocument4_get_media,
    HTMLDocument4_createEventObject,
    HTMLDocument4_fireEvent,
    HTMLDocument4_createRenderStyle,
    HTMLDocument4_put_oncontrolselect,
    HTMLDocument4_get_oncontrolselect,
    HTMLDocument4_get_URLUnencoded
};

static inline HTMLDocumentNode *impl_from_IHTMLDocument5(IHTMLDocument5 *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IHTMLDocument5_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLDocument5, IHTMLDocument5,
                      impl_from_IHTMLDocument5(iface)->node.event_target.dispex)

static HRESULT WINAPI HTMLDocument5_put_onmousewheel(IHTMLDocument5 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument5(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_MOUSEWHEEL, &v);
}

static HRESULT WINAPI HTMLDocument5_get_onmousewheel(IHTMLDocument5 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument5(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_MOUSEWHEEL, p);
}

static HRESULT WINAPI HTMLDocument5_get_doctype(IHTMLDocument5 *iface, IHTMLDOMNode **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument5(iface);
    nsIDOMDocumentType *nsdoctype;
    HTMLDOMNode *doctype_node;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(dispex_compat_mode(&This->node.event_target.dispex) < COMPAT_MODE_IE9) {
        *p = NULL;
        return S_OK;
    }

    nsres = nsIDOMDocument_GetDoctype(This->dom_document, &nsdoctype);
    if(NS_FAILED(nsres))
        return map_nsresult(nsres);
    if(!nsdoctype) {
        *p = NULL;
        return S_OK;
    }

    hres = get_node((nsIDOMNode*)nsdoctype, TRUE, &doctype_node);
    nsIDOMDocumentType_Release(nsdoctype);

    if(SUCCEEDED(hres))
        *p = &doctype_node->IHTMLDOMNode_iface;
    return hres;
}

static HRESULT WINAPI HTMLDocument5_get_implementation(IHTMLDocument5 *iface, IHTMLDOMImplementation **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument5(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->dom_implementation) {
        HRESULT hres;

        hres = create_dom_implementation(This, &This->dom_implementation);
        if(FAILED(hres))
            return hres;
    }

    IHTMLDOMImplementation_AddRef(This->dom_implementation);
    *p = This->dom_implementation;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument5_createAttribute(IHTMLDocument5 *iface, BSTR bstrattrName,
        IHTMLDOMAttribute **ppattribute)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument5(iface);
    HTMLDOMAttribute *attr;
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(bstrattrName), ppattribute);

    hres = HTMLDOMAttribute_Create(bstrattrName, NULL, 0, This, &attr);
    if(FAILED(hres))
        return hres;

    *ppattribute = &attr->IHTMLDOMAttribute_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument5_createComment(IHTMLDocument5 *iface, BSTR bstrdata,
        IHTMLDOMNode **ppRetNode)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument5(iface);
    nsIDOMComment *nscomment;
    HTMLElement *elem;
    nsAString str;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(bstrdata), ppRetNode);

    if(!This->dom_document) {
        WARN("NULL dom_document\n");
        return E_UNEXPECTED;
    }

    nsAString_InitDepend(&str, bstrdata);
    nsres = nsIDOMDocument_CreateComment(This->dom_document, &str, &nscomment);
    nsAString_Finish(&str);
    if(NS_FAILED(nsres)) {
        ERR("CreateTextNode failed: %08lx\n", nsres);
        return E_FAIL;
    }

    hres = HTMLCommentElement_Create(This, (nsIDOMNode*)nscomment, &elem);
    nsIDOMComment_Release(nscomment);
    if(FAILED(hres))
        return hres;

    *ppRetNode = &elem->node.IHTMLDOMNode_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument5_put_onfocusin(IHTMLDocument5 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument5(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_FOCUSIN, &v);
}

static HRESULT WINAPI HTMLDocument5_get_onfocusin(IHTMLDocument5 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument5(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_FOCUSIN, p);
}

static HRESULT WINAPI HTMLDocument5_put_onfocusout(IHTMLDocument5 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument5(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_FOCUSOUT, &v);
}

static HRESULT WINAPI HTMLDocument5_get_onfocusout(IHTMLDocument5 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument5(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_FOCUSOUT, p);
}

static HRESULT WINAPI HTMLDocument5_put_onactivate(IHTMLDocument5 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_get_onactivate(IHTMLDocument5 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_put_ondeactivate(IHTMLDocument5 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_get_ondeactivate(IHTMLDocument5 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_put_onbeforeactivate(IHTMLDocument5 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_get_onbeforeactivate(IHTMLDocument5 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_put_onbeforedeactivate(IHTMLDocument5 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_get_onbeforedeactivate(IHTMLDocument5 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument5(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument5_get_compatMode(IHTMLDocument5 *iface, BSTR *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument5(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = SysAllocString(This->document_mode <= COMPAT_MODE_IE5 ? L"BackCompat" : L"CSS1Compat");
    return *p ? S_OK : E_OUTOFMEMORY;
}

static const IHTMLDocument5Vtbl HTMLDocument5Vtbl = {
    HTMLDocument5_QueryInterface,
    HTMLDocument5_AddRef,
    HTMLDocument5_Release,
    HTMLDocument5_GetTypeInfoCount,
    HTMLDocument5_GetTypeInfo,
    HTMLDocument5_GetIDsOfNames,
    HTMLDocument5_Invoke,
    HTMLDocument5_put_onmousewheel,
    HTMLDocument5_get_onmousewheel,
    HTMLDocument5_get_doctype,
    HTMLDocument5_get_implementation,
    HTMLDocument5_createAttribute,
    HTMLDocument5_createComment,
    HTMLDocument5_put_onfocusin,
    HTMLDocument5_get_onfocusin,
    HTMLDocument5_put_onfocusout,
    HTMLDocument5_get_onfocusout,
    HTMLDocument5_put_onactivate,
    HTMLDocument5_get_onactivate,
    HTMLDocument5_put_ondeactivate,
    HTMLDocument5_get_ondeactivate,
    HTMLDocument5_put_onbeforeactivate,
    HTMLDocument5_get_onbeforeactivate,
    HTMLDocument5_put_onbeforedeactivate,
    HTMLDocument5_get_onbeforedeactivate,
    HTMLDocument5_get_compatMode
};

static inline HTMLDocumentNode *impl_from_IHTMLDocument6(IHTMLDocument6 *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IHTMLDocument6_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLDocument6, IHTMLDocument6,
                      impl_from_IHTMLDocument6(iface)->node.event_target.dispex)

static HRESULT WINAPI HTMLDocument6_get_compatible(IHTMLDocument6 *iface,
        IHTMLDocumentCompatibleInfoCollection **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument6(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument6_get_documentMode(IHTMLDocument6 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument6(iface);

    TRACE("(%p)->(%p)\n", This, p);

    V_VT(p) = VT_R4;
    V_R4(p) = compat_mode_info[This->document_mode].document_mode;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument6_get_onstorage(IHTMLDocument6 *iface,
        VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument6(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_STORAGE, p);
}

static HRESULT WINAPI HTMLDocument6_put_onstorage(IHTMLDocument6 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument6(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_STORAGE, &v);
}

static HRESULT WINAPI HTMLDocument6_get_onstoragecommit(IHTMLDocument6 *iface,
        VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument6(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_STORAGECOMMIT, p);
}

static HRESULT WINAPI HTMLDocument6_put_onstoragecommit(IHTMLDocument6 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument6(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_STORAGECOMMIT, &v);
}

static HRESULT WINAPI HTMLDocument6_getElementById(IHTMLDocument6 *iface,
        BSTR bstrId, IHTMLElement2 **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument6(iface);
    nsIDOMElement *nselem;
    HTMLElement *elem;
    nsAString nsstr;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(bstrId), p);

    /*
     * Unlike IHTMLDocument3 implementation, this is standard compliant and does
     * not search for name attributes, so we may simply let Gecko do the right thing.
     */

    if(!This->dom_document) {
        FIXME("Not a document\n");
        return E_FAIL;
    }

    nsAString_InitDepend(&nsstr, bstrId);
    nsres = nsIDOMDocument_GetElementById(This->dom_document, &nsstr, &nselem);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres)) {
        ERR("GetElementById failed: %08lx\n", nsres);
        return E_FAIL;
    }

    if(!nselem) {
        *p = NULL;
        return S_OK;
    }

    hres = get_element(nselem, &elem);
    nsIDOMElement_Release(nselem);
    if(FAILED(hres))
        return hres;

    *p = &elem->IHTMLElement2_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument6_updateSettings(IHTMLDocument6 *iface)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument6(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static const IHTMLDocument6Vtbl HTMLDocument6Vtbl = {
    HTMLDocument6_QueryInterface,
    HTMLDocument6_AddRef,
    HTMLDocument6_Release,
    HTMLDocument6_GetTypeInfoCount,
    HTMLDocument6_GetTypeInfo,
    HTMLDocument6_GetIDsOfNames,
    HTMLDocument6_Invoke,
    HTMLDocument6_get_compatible,
    HTMLDocument6_get_documentMode,
    HTMLDocument6_put_onstorage,
    HTMLDocument6_get_onstorage,
    HTMLDocument6_put_onstoragecommit,
    HTMLDocument6_get_onstoragecommit,
    HTMLDocument6_getElementById,
    HTMLDocument6_updateSettings
};

static inline HTMLDocumentNode *impl_from_IHTMLDocument7(IHTMLDocument7 *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IHTMLDocument7_iface);
}

DISPEX_IDISPATCH_IMPL(HTMLDocument7, IHTMLDocument7,
                      impl_from_IHTMLDocument7(iface)->node.event_target.dispex)

static HRESULT WINAPI HTMLDocument7_get_defaultView(IHTMLDocument7 *iface, IHTMLWindow2 **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->window && This->window->base.outer_window) {
        *p = &This->window->base.outer_window->base.IHTMLWindow2_iface;
        IHTMLWindow2_AddRef(*p);
    }else {
        *p = NULL;
    }
    return S_OK;
}

static HRESULT WINAPI HTMLDocument7_createCDATASection(IHTMLDocument7 *iface, BSTR text, IHTMLDOMNode **newCDATASectionNode)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, newCDATASectionNode);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_getSelection(IHTMLDocument7 *iface, IHTMLSelection **ppIHTMLSelection)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, ppIHTMLSelection);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_getElementsByTagNameNS(IHTMLDocument7 *iface, VARIANT *pvarNS,
        BSTR bstrLocalName, IHTMLElementCollection **pelColl)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s %s %p)\n", This, debugstr_variant(pvarNS), debugstr_w(bstrLocalName), pelColl);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_createElementNS(IHTMLDocument7 *iface, VARIANT *pvarNS, BSTR bstrTag, IHTMLElement **newElem)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    nsIDOMElement *dom_element;
    HTMLElement *element;
    nsAString ns, tag;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_variant(pvarNS), debugstr_w(bstrTag), newElem);

    if(!This->dom_document) {
        FIXME("NULL dom_document\n");
        return E_FAIL;
    }

    if(pvarNS && V_VT(pvarNS) != VT_NULL && V_VT(pvarNS) != VT_BSTR)
        FIXME("Unsupported namespace %s\n", debugstr_variant(pvarNS));

    nsAString_InitDepend(&ns, pvarNS && V_VT(pvarNS) == VT_BSTR ? V_BSTR(pvarNS) : NULL);
    nsAString_InitDepend(&tag, bstrTag);
    nsres = nsIDOMDocument_CreateElementNS(This->dom_document, &ns, &tag, &dom_element);
    nsAString_Finish(&ns);
    nsAString_Finish(&tag);
    if(NS_FAILED(nsres)) {
        WARN("CreateElementNS failed: %08lx\n", nsres);
        return map_nsresult(nsres);
    }

    hres = HTMLElement_Create(This, (nsIDOMNode*)dom_element, FALSE, &element);
    nsIDOMElement_Release(dom_element);
    if(FAILED(hres))
        return hres;

    *newElem = &element->IHTMLElement_iface;
    return S_OK;
}

static HRESULT WINAPI HTMLDocument7_createAttributeNS(IHTMLDocument7 *iface, VARIANT *pvarNS,
        BSTR bstrAttrName, IHTMLDOMAttribute **ppAttribute)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s %s %p)\n", This, debugstr_variant(pvarNS), debugstr_w(bstrAttrName), ppAttribute);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onmsthumbnailclick(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onmsthumbnailclick(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_MSTHUMBNAILCLICK, p);
}

static HRESULT WINAPI HTMLDocument7_get_characterSet(IHTMLDocument7 *iface, BSTR *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    nsAString charset_str;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->dom_document) {
        FIXME("NULL dom_document\n");
        return E_FAIL;
    }

    nsAString_Init(&charset_str, NULL);
    nsres = nsIDOMDocument_GetCharacterSet(This->dom_document, &charset_str);
    return return_nsstr(nsres, &charset_str, p);
}

static HRESULT WINAPI HTMLDocument7_createElement(IHTMLDocument7 *iface, BSTR bstrTag, IHTMLElement **newElem)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(bstrTag), newElem);

    return IHTMLDocument2_createElement(&This->IHTMLDocument2_iface, bstrTag, newElem);
}

static HRESULT WINAPI HTMLDocument7_createAttribute(IHTMLDocument7 *iface, BSTR bstrAttrName, IHTMLDOMAttribute **ppAttribute)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(bstrAttrName), ppAttribute);

    return IHTMLDocument5_createAttribute(&This->IHTMLDocument5_iface, bstrAttrName, ppAttribute);
}

static HRESULT WINAPI HTMLDocument7_getElementsByClassName(IHTMLDocument7 *iface, BSTR v, IHTMLElementCollection **pel)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    nsIDOMNodeList *nslist;
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(v), pel);

    if(!This->dom_document) {
        FIXME("NULL dom_document not supported\n");
        return E_NOTIMPL;
    }

    nsAString_InitDepend(&nsstr, v);
    nsres = nsIDOMDocument_GetElementsByClassName(This->dom_document, &nsstr, &nslist);
    nsAString_Finish(&nsstr);
    if(FAILED(nsres)) {
        ERR("GetElementByClassName failed: %08lx\n", nsres);
        return E_FAIL;
    }


    *pel = create_collection_from_nodelist(nslist, &This->node.event_target.dispex);
    nsIDOMNodeList_Release(nslist);
    return S_OK;
}

static HRESULT WINAPI HTMLDocument7_createProcessingInstruction(IHTMLDocument7 *iface, BSTR target,
        BSTR data, IDOMProcessingInstruction **newProcessingInstruction)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s %s %p)\n", This, debugstr_w(target), debugstr_w(data), newProcessingInstruction);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_adoptNode(IHTMLDocument7 *iface, IHTMLDOMNode *pNodeSource, IHTMLDOMNode3 **ppNodeDest)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p %p)\n", This, pNodeSource, ppNodeDest);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onmssitemodejumplistitemremoved(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onmssitemodejumplistitemremoved(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_all(IHTMLDocument7 *iface, IHTMLElementCollection **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLDocument2_get_all(&This->IHTMLDocument2_iface, p);
}

static HRESULT WINAPI HTMLDocument7_get_inputEncoding(IHTMLDocument7 *iface, BSTR *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_xmlEncoding(IHTMLDocument7 *iface, BSTR *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_xmlStandalone(IHTMLDocument7 *iface, VARIANT_BOOL v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_xmlStandalone(IHTMLDocument7 *iface, VARIANT_BOOL *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_xmlVersion(IHTMLDocument7 *iface, BSTR v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_xmlVersion(IHTMLDocument7 *iface, BSTR *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_hasAttributes(IHTMLDocument7 *iface, VARIANT_BOOL *pfHasAttributes)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, pfHasAttributes);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onabort(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_ABORT, &v);
}

static HRESULT WINAPI HTMLDocument7_get_onabort(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_ABORT, p);
}

static HRESULT WINAPI HTMLDocument7_put_onblur(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_BLUR, &v);
}

static HRESULT WINAPI HTMLDocument7_get_onblur(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_BLUR, p);
}

static HRESULT WINAPI HTMLDocument7_put_oncanplay(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_oncanplay(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_oncanplaythrough(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_oncanplaythrough(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onchange(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_CHANGE, &v);
}

static HRESULT WINAPI HTMLDocument7_get_onchange(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_CHANGE, p);
}

static HRESULT WINAPI HTMLDocument7_put_ondrag(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_DRAG, &v);
}

static HRESULT WINAPI HTMLDocument7_get_ondrag(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_DRAG, p);
}

static HRESULT WINAPI HTMLDocument7_put_ondragend(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_ondragend(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_ondragenter(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_ondragenter(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_ondragleave(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_ondragleave(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_ondragover(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_ondragover(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_ondrop(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_ondrop(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_ondurationchange(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_ondurationchange(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onemptied(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onemptied(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onended(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onended(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onerror(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_ERROR, &v);
}

static HRESULT WINAPI HTMLDocument7_get_onerror(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_ERROR, p);
}

static HRESULT WINAPI HTMLDocument7_put_onfocus(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_FOCUS, &v);
}

static HRESULT WINAPI HTMLDocument7_get_onfocus(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_FOCUS, p);
}

static HRESULT WINAPI HTMLDocument7_put_oninput(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_INPUT, &v);
}

static HRESULT WINAPI HTMLDocument7_get_oninput(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_INPUT, p);
}

static HRESULT WINAPI HTMLDocument7_put_onload(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_LOAD, &v);
}

static HRESULT WINAPI HTMLDocument7_get_onload(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_LOAD, p);
}

static HRESULT WINAPI HTMLDocument7_put_onloadeddata(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onloadeddata(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onloadedmetadata(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onloadedmetadata(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onloadstart(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onloadstart(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onpause(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onpause(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onplay(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onplay(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onplaying(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onplaying(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onprogress(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onprogress(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onratechange(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onratechange(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onreset(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onreset(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onscroll(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_SCROLL, &v);
}

static HRESULT WINAPI HTMLDocument7_get_onscroll(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_SCROLL, p);
}

static HRESULT WINAPI HTMLDocument7_put_onseekend(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onseekend(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onseeking(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onseeking(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onselect(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onselect(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onstalled(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onstalled(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onsubmit(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_SUBMIT, &v);
}

static HRESULT WINAPI HTMLDocument7_get_onsubmit(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_SUBMIT, p);
}

static HRESULT WINAPI HTMLDocument7_put_onsuspend(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onsuspend(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_ontimeupdate(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_ontimeupdate(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onvolumechange(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onvolumechange(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_put_onwaiting(IHTMLDocument7 *iface, VARIANT v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(&v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_onwaiting(IHTMLDocument7 *iface, VARIANT *p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_normalize(IHTMLDocument7 *iface)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_importNode(IHTMLDocument7 *iface, IHTMLDOMNode *pNodeSource,
        VARIANT_BOOL fDeep, IHTMLDOMNode3 **ppNodeDest)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    nsIDOMNode *nsnode = NULL;
    HTMLDOMNode *node;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p %x %p)\n", This, pNodeSource, fDeep, ppNodeDest);

    if(!This->dom_document) {
        WARN("NULL dom_document\n");
        return E_UNEXPECTED;
    }

    if(!(node = unsafe_impl_from_IHTMLDOMNode(pNodeSource))) {
        ERR("not our node\n");
        return E_FAIL;
    }

    nsres = nsIDOMDocument_ImportNode(This->dom_document, node->nsnode, !!fDeep, 1, &nsnode);
    if(NS_FAILED(nsres)) {
        ERR("ImportNode failed: %08lx\n", nsres);
        return map_nsresult(nsres);
    }

    if(!nsnode) {
        *ppNodeDest = NULL;
        return S_OK;
    }

    hres = get_node(nsnode, TRUE, &node);
    nsIDOMNode_Release(nsnode);
    if(FAILED(hres))
        return hres;

    *ppNodeDest = &node->IHTMLDOMNode3_iface;
    return hres;
}

static HRESULT WINAPI HTMLDocument7_get_parentWindow(IHTMLDocument7 *iface, IHTMLWindow2 **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLDocument7_get_defaultView(&This->IHTMLDocument7_iface, p);
}

static HRESULT WINAPI HTMLDocument7_put_body(IHTMLDocument7 *iface, IHTMLElement *v)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    FIXME("(%p)->(%p)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument7_get_body(IHTMLDocument7 *iface, IHTMLElement **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLDocument2_get_body(&This->IHTMLDocument2_iface, p);
}

static HRESULT WINAPI HTMLDocument7_get_head(IHTMLDocument7 *iface, IHTMLElement **p)
{
    HTMLDocumentNode *This = impl_from_IHTMLDocument7(iface);
    nsIDOMHTMLHeadElement *nshead;
    nsIDOMElement *nselem;
    HTMLElement *elem;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->dom_document) {
        FIXME("No document\n");
        return E_FAIL;
    }

    if(!This->html_document) {
        FIXME("Not implemented for XML document\n");
        return E_NOTIMPL;
    }

    nsres = nsIDOMHTMLDocument_GetHead(This->html_document, &nshead);
    assert(nsres == NS_OK);

    if(!nshead) {
        *p = NULL;
        return S_OK;
    }

    nsres = nsIDOMHTMLHeadElement_QueryInterface(nshead, &IID_nsIDOMElement, (void**)&nselem);
    nsIDOMHTMLHeadElement_Release(nshead);
    assert(nsres == NS_OK);

    hres = get_element(nselem, &elem);
    nsIDOMElement_Release(nselem);
    if(FAILED(hres))
        return hres;

    *p = &elem->IHTMLElement_iface;
    return S_OK;
}

static const IHTMLDocument7Vtbl HTMLDocument7Vtbl = {
    HTMLDocument7_QueryInterface,
    HTMLDocument7_AddRef,
    HTMLDocument7_Release,
    HTMLDocument7_GetTypeInfoCount,
    HTMLDocument7_GetTypeInfo,
    HTMLDocument7_GetIDsOfNames,
    HTMLDocument7_Invoke,
    HTMLDocument7_get_defaultView,
    HTMLDocument7_createCDATASection,
    HTMLDocument7_getSelection,
    HTMLDocument7_getElementsByTagNameNS,
    HTMLDocument7_createElementNS,
    HTMLDocument7_createAttributeNS,
    HTMLDocument7_put_onmsthumbnailclick,
    HTMLDocument7_get_onmsthumbnailclick,
    HTMLDocument7_get_characterSet,
    HTMLDocument7_createElement,
    HTMLDocument7_createAttribute,
    HTMLDocument7_getElementsByClassName,
    HTMLDocument7_createProcessingInstruction,
    HTMLDocument7_adoptNode,
    HTMLDocument7_put_onmssitemodejumplistitemremoved,
    HTMLDocument7_get_onmssitemodejumplistitemremoved,
    HTMLDocument7_get_all,
    HTMLDocument7_get_inputEncoding,
    HTMLDocument7_get_xmlEncoding,
    HTMLDocument7_put_xmlStandalone,
    HTMLDocument7_get_xmlStandalone,
    HTMLDocument7_put_xmlVersion,
    HTMLDocument7_get_xmlVersion,
    HTMLDocument7_hasAttributes,
    HTMLDocument7_put_onabort,
    HTMLDocument7_get_onabort,
    HTMLDocument7_put_onblur,
    HTMLDocument7_get_onblur,
    HTMLDocument7_put_oncanplay,
    HTMLDocument7_get_oncanplay,
    HTMLDocument7_put_oncanplaythrough,
    HTMLDocument7_get_oncanplaythrough,
    HTMLDocument7_put_onchange,
    HTMLDocument7_get_onchange,
    HTMLDocument7_put_ondrag,
    HTMLDocument7_get_ondrag,
    HTMLDocument7_put_ondragend,
    HTMLDocument7_get_ondragend,
    HTMLDocument7_put_ondragenter,
    HTMLDocument7_get_ondragenter,
    HTMLDocument7_put_ondragleave,
    HTMLDocument7_get_ondragleave,
    HTMLDocument7_put_ondragover,
    HTMLDocument7_get_ondragover,
    HTMLDocument7_put_ondrop,
    HTMLDocument7_get_ondrop,
    HTMLDocument7_put_ondurationchange,
    HTMLDocument7_get_ondurationchange,
    HTMLDocument7_put_onemptied,
    HTMLDocument7_get_onemptied,
    HTMLDocument7_put_onended,
    HTMLDocument7_get_onended,
    HTMLDocument7_put_onerror,
    HTMLDocument7_get_onerror,
    HTMLDocument7_put_onfocus,
    HTMLDocument7_get_onfocus,
    HTMLDocument7_put_oninput,
    HTMLDocument7_get_oninput,
    HTMLDocument7_put_onload,
    HTMLDocument7_get_onload,
    HTMLDocument7_put_onloadeddata,
    HTMLDocument7_get_onloadeddata,
    HTMLDocument7_put_onloadedmetadata,
    HTMLDocument7_get_onloadedmetadata,
    HTMLDocument7_put_onloadstart,
    HTMLDocument7_get_onloadstart,
    HTMLDocument7_put_onpause,
    HTMLDocument7_get_onpause,
    HTMLDocument7_put_onplay,
    HTMLDocument7_get_onplay,
    HTMLDocument7_put_onplaying,
    HTMLDocument7_get_onplaying,
    HTMLDocument7_put_onprogress,
    HTMLDocument7_get_onprogress,
    HTMLDocument7_put_onratechange,
    HTMLDocument7_get_onratechange,
    HTMLDocument7_put_onreset,
    HTMLDocument7_get_onreset,
    HTMLDocument7_put_onscroll,
    HTMLDocument7_get_onscroll,
    HTMLDocument7_put_onseekend,
    HTMLDocument7_get_onseekend,
    HTMLDocument7_put_onseeking,
    HTMLDocument7_get_onseeking,
    HTMLDocument7_put_onselect,
    HTMLDocument7_get_onselect,
    HTMLDocument7_put_onstalled,
    HTMLDocument7_get_onstalled,
    HTMLDocument7_put_onsubmit,
    HTMLDocument7_get_onsubmit,
    HTMLDocument7_put_onsuspend,
    HTMLDocument7_get_onsuspend,
    HTMLDocument7_put_ontimeupdate,
    HTMLDocument7_get_ontimeupdate,
    HTMLDocument7_put_onvolumechange,
    HTMLDocument7_get_onvolumechange,
    HTMLDocument7_put_onwaiting,
    HTMLDocument7_get_onwaiting,
    HTMLDocument7_normalize,
    HTMLDocument7_importNode,
    HTMLDocument7_get_parentWindow,
    HTMLDocument7_put_body,
    HTMLDocument7_get_body,
    HTMLDocument7_get_head
};

static inline HTMLDocumentNode *impl_from_IDocumentSelector(IDocumentSelector *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IDocumentSelector_iface);
}

DISPEX_IDISPATCH_IMPL(DocumentSelector, IDocumentSelector,
                      impl_from_IDocumentSelector(iface)->node.event_target.dispex)

static HRESULT WINAPI DocumentSelector_querySelector(IDocumentSelector *iface, BSTR v, IHTMLElement **pel)
{
    HTMLDocumentNode *This = impl_from_IDocumentSelector(iface);
    nsIDOMElement *nselem;
    HTMLElement *elem;
    nsAString nsstr;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(v), pel);

    nsAString_InitDepend(&nsstr, v);
    if(This->dom_document)
        nsres = nsIDOMDocument_QuerySelector(This->dom_document, &nsstr, &nselem);
    else {
        nsIDOMDocumentFragment *frag;
        nsres = nsIDOMNode_QueryInterface(This->node.nsnode, &IID_nsIDOMDocumentFragment, (void**)&frag);
        if(NS_SUCCEEDED(nsres)) {
            nsres = nsIDOMDocumentFragment_QuerySelector(frag, &nsstr, &nselem);
            nsIDOMDocumentFragment_Release(frag);
        }
    }
    nsAString_Finish(&nsstr);

    if(NS_FAILED(nsres)) {
        WARN("QuerySelector failed: %08lx\n", nsres);
        return map_nsresult(nsres);
    }

    if(!nselem) {
        *pel = NULL;
        return S_OK;
    }

    hres = get_element(nselem, &elem);
    nsIDOMElement_Release(nselem);
    if(FAILED(hres))
        return hres;

    *pel = &elem->IHTMLElement_iface;
    return S_OK;
}

static HRESULT WINAPI DocumentSelector_querySelectorAll(IDocumentSelector *iface, BSTR v, IHTMLDOMChildrenCollection **pel)
{
    HTMLDocumentNode *This = impl_from_IDocumentSelector(iface);
    nsIDOMNodeList *node_list = NULL;
    nsAString nsstr;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(v), pel);

    nsAString_InitDepend(&nsstr, v);
    if(This->dom_document)
        nsres = nsIDOMDocument_QuerySelectorAll(This->dom_document, &nsstr, &node_list);
    else {
        nsIDOMDocumentFragment *frag;
        nsres = nsIDOMNode_QueryInterface(This->node.nsnode, &IID_nsIDOMDocumentFragment, (void**)&frag);
        if(NS_SUCCEEDED(nsres)) {
            nsres = nsIDOMDocumentFragment_QuerySelectorAll(frag, &nsstr, &node_list);
            nsIDOMDocumentFragment_Release(frag);
        }
    }
    nsAString_Finish(&nsstr);

    if(NS_FAILED(nsres)) {
        WARN("QuerySelectorAll failed: %08lx\n", nsres);
        if(node_list)
            nsIDOMNodeList_Release(node_list);
        return map_nsresult(nsres);
    }

    hres = create_child_collection(node_list, &This->node.event_target.dispex, pel);
    nsIDOMNodeList_Release(node_list);
    return hres;
}

static const IDocumentSelectorVtbl DocumentSelectorVtbl = {
    DocumentSelector_QueryInterface,
    DocumentSelector_AddRef,
    DocumentSelector_Release,
    DocumentSelector_GetTypeInfoCount,
    DocumentSelector_GetTypeInfo,
    DocumentSelector_GetIDsOfNames,
    DocumentSelector_Invoke,
    DocumentSelector_querySelector,
    DocumentSelector_querySelectorAll
};

static inline HTMLDocumentNode *impl_from_IDocumentEvent(IDocumentEvent *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IDocumentEvent_iface);
}

#ifdef __REACTOS__
/* Our winspool.h turns this into DocumentEventA/DocumentEventW */
#ifdef DocumentEvent
#undef DocumentEvent
#endif
#endif
DISPEX_IDISPATCH_IMPL(DocumentEvent, IDocumentEvent,
                      impl_from_IDocumentEvent(iface)->node.event_target.dispex)

static HRESULT WINAPI DocumentEvent_createEvent(IDocumentEvent *iface, BSTR eventType, IDOMEvent **p)
{
    HTMLDocumentNode *This = impl_from_IDocumentEvent(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(eventType), p);

    return create_document_event_str(This, eventType, p);
}

static const IDocumentEventVtbl DocumentEventVtbl = {
    DocumentEvent_QueryInterface,
    DocumentEvent_AddRef,
    DocumentEvent_Release,
    DocumentEvent_GetTypeInfoCount,
    DocumentEvent_GetTypeInfo,
    DocumentEvent_GetIDsOfNames,
    DocumentEvent_Invoke,
    DocumentEvent_createEvent
};

static void HTMLDocumentNode_on_advise(IUnknown *iface, cp_static_data_t *cp)
{
    HTMLDocumentNode *This = CONTAINING_RECORD((IHTMLDocument2*)iface, HTMLDocumentNode, IHTMLDocument2_iface);

    if(This->window && This->window->base.outer_window)
        update_doc_cp_events(This, cp);
}

static inline HTMLDocumentNode *impl_from_ISupportErrorInfo(ISupportErrorInfo *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, ISupportErrorInfo_iface);
}

static HRESULT WINAPI SupportErrorInfo_QueryInterface(ISupportErrorInfo *iface, REFIID riid, void **ppv)
{
    HTMLDocumentNode *This = impl_from_ISupportErrorInfo(iface);
    return IHTMLDOMNode_QueryInterface(&This->node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI SupportErrorInfo_AddRef(ISupportErrorInfo *iface)
{
    HTMLDocumentNode *This = impl_from_ISupportErrorInfo(iface);
    return IHTMLDOMNode_AddRef(&This->node.IHTMLDOMNode_iface);
}

static ULONG WINAPI SupportErrorInfo_Release(ISupportErrorInfo *iface)
{
    HTMLDocumentNode *This = impl_from_ISupportErrorInfo(iface);
    return IHTMLDOMNode_Release(&This->node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI SupportErrorInfo_InterfaceSupportsErrorInfo(ISupportErrorInfo *iface, REFIID riid)
{
    FIXME("(%p)->(%s)\n", iface, debugstr_mshtml_guid(riid));
    return S_FALSE;
}

static const ISupportErrorInfoVtbl SupportErrorInfoVtbl = {
    SupportErrorInfo_QueryInterface,
    SupportErrorInfo_AddRef,
    SupportErrorInfo_Release,
    SupportErrorInfo_InterfaceSupportsErrorInfo
};

static HRESULT has_elem_name(nsIDOMHTMLDocument *html_document, const WCHAR *name)
{
    static const WCHAR fmt[] = L":-moz-any(applet,embed,form,iframe,img,object)[name=\"%s\"]";
    WCHAR buf[128], *selector = buf;
    nsAString selector_str;
    nsIDOMElement *nselem;
    nsresult nsres;
    size_t len;

    len = wcslen(name) + ARRAY_SIZE(fmt) - 2 /* %s */;
    if(len > ARRAY_SIZE(buf) && !(selector = malloc(len * sizeof(WCHAR))))
        return E_OUTOFMEMORY;
    swprintf(selector, len, fmt, name);

    nsAString_InitDepend(&selector_str, selector);
    nsres = nsIDOMHTMLDocument_QuerySelector(html_document, &selector_str, &nselem);
    nsAString_Finish(&selector_str);
    if(selector != buf)
        free(selector);
    if(NS_FAILED(nsres))
        return map_nsresult(nsres);

    if(!nselem)
        return DISP_E_UNKNOWNNAME;
    nsIDOMElement_Release(nselem);
    return S_OK;
}

static HRESULT get_elem_by_name_or_id(nsIDOMHTMLDocument *html_document, const WCHAR *name, nsIDOMElement **ret)
{
    static const WCHAR fmt[] = L":-moz-any(embed,form,iframe,img):-moz-any([name=\"%s\"],[id=\"%s\"][name]),"
                               L":-moz-any(applet,object):-moz-any([name=\"%s\"],[id=\"%s\"])";
    WCHAR buf[384], *selector = buf;
    nsAString selector_str;
    nsIDOMElement *nselem;
    nsresult nsres;
    size_t len;

    len = wcslen(name) * 4 + ARRAY_SIZE(fmt) - 8 /* %s */;
    if(len > ARRAY_SIZE(buf) && !(selector = malloc(len * sizeof(WCHAR))))
        return E_OUTOFMEMORY;
    swprintf(selector, len, fmt, name, name, name, name);

    nsAString_InitDepend(&selector_str, selector);
    nsres = nsIDOMHTMLDocument_QuerySelector(html_document, &selector_str, &nselem);
    nsAString_Finish(&selector_str);
    if(selector != buf)
        free(selector);
    if(NS_FAILED(nsres))
        return map_nsresult(nsres);

    if(ret) {
        *ret = nselem;
        return S_OK;
    }

    if(nselem) {
        nsIDOMElement_Release(nselem);
        return S_OK;
    }
    return DISP_E_UNKNOWNNAME;
}

static HRESULT dispid_from_elem_name(HTMLDocumentNode *This, const WCHAR *name, DISPID *dispid)
{
    unsigned i;

    for(i=0; i < This->elem_vars_cnt; i++) {
        if(!wcscmp(name, This->elem_vars[i])) {
            *dispid = MSHTML_DISPID_CUSTOM_MIN+i;
            return S_OK;
        }
    }

    if(This->elem_vars_cnt == This->elem_vars_size) {
        WCHAR **new_vars;

        if(This->elem_vars_size) {
            new_vars = realloc(This->elem_vars, This->elem_vars_size * 2 * sizeof(WCHAR*));
            if(!new_vars)
                return E_OUTOFMEMORY;
            This->elem_vars_size *= 2;
        }else {
            new_vars = malloc(16 * sizeof(WCHAR*));
            if(!new_vars)
                return E_OUTOFMEMORY;
            This->elem_vars_size = 16;
        }

        This->elem_vars = new_vars;
    }

    This->elem_vars[This->elem_vars_cnt] = wcsdup(name);
    if(!This->elem_vars[This->elem_vars_cnt])
        return E_OUTOFMEMORY;

    *dispid = MSHTML_DISPID_CUSTOM_MIN+This->elem_vars_cnt++;
    return S_OK;
}

static inline HTMLDocumentNode *impl_from_IProvideMultipleClassInfo(IProvideMultipleClassInfo *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IProvideMultipleClassInfo_iface);
}

static HRESULT WINAPI ProvideClassInfo_QueryInterface(IProvideMultipleClassInfo *iface,
        REFIID riid, void **ppv)
{
    HTMLDocumentNode *This = impl_from_IProvideMultipleClassInfo(iface);
    return IHTMLDOMNode_QueryInterface(&This->node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI ProvideClassInfo_AddRef(IProvideMultipleClassInfo *iface)
{
    HTMLDocumentNode *This = impl_from_IProvideMultipleClassInfo(iface);
    return IHTMLDOMNode_AddRef(&This->node.IHTMLDOMNode_iface);
}

static ULONG WINAPI ProvideClassInfo_Release(IProvideMultipleClassInfo *iface)
{
    HTMLDocumentNode *This = impl_from_IProvideMultipleClassInfo(iface);
    return IHTMLDOMNode_Release(&This->node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI ProvideClassInfo_GetClassInfo(IProvideMultipleClassInfo *iface, ITypeInfo **ppTI)
{
    HTMLDocumentNode *This = impl_from_IProvideMultipleClassInfo(iface);
    TRACE("(%p)->(%p)\n", This, ppTI);
    return get_class_typeinfo(&CLSID_HTMLDocument, ppTI);
}

static HRESULT WINAPI ProvideClassInfo2_GetGUID(IProvideMultipleClassInfo *iface, DWORD dwGuidKind, GUID *pGUID)
{
    HTMLDocumentNode *This = impl_from_IProvideMultipleClassInfo(iface);
    FIXME("(%p)->(%lu %p)\n", This, dwGuidKind, pGUID);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProvideMultipleClassInfo_GetMultiTypeInfoCount(IProvideMultipleClassInfo *iface, ULONG *pcti)
{
    HTMLDocumentNode *This = impl_from_IProvideMultipleClassInfo(iface);
    FIXME("(%p)->(%p)\n", This, pcti);
    *pcti = 1;
    return S_OK;
}

static HRESULT WINAPI ProvideMultipleClassInfo_GetInfoOfIndex(IProvideMultipleClassInfo *iface, ULONG iti,
        DWORD dwFlags, ITypeInfo **pptiCoClass, DWORD *pdwTIFlags, ULONG *pcdispidReserved, IID *piidPrimary, IID *piidSource)
{
    HTMLDocumentNode *This = impl_from_IProvideMultipleClassInfo(iface);
    FIXME("(%p)->(%lu %lx %p %p %p %p %p)\n", This, iti, dwFlags, pptiCoClass, pdwTIFlags, pcdispidReserved, piidPrimary, piidSource);
    return E_NOTIMPL;
}

static const IProvideMultipleClassInfoVtbl ProvideMultipleClassInfoVtbl = {
    ProvideClassInfo_QueryInterface,
    ProvideClassInfo_AddRef,
    ProvideClassInfo_Release,
    ProvideClassInfo_GetClassInfo,
    ProvideClassInfo2_GetGUID,
    ProvideMultipleClassInfo_GetMultiTypeInfoCount,
    ProvideMultipleClassInfo_GetInfoOfIndex
};

/**********************************************************
 * IMarkupServices implementation
 */
static inline HTMLDocumentNode *impl_from_IMarkupServices(IMarkupServices *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IMarkupServices_iface);
}

static HRESULT WINAPI MarkupServices_QueryInterface(IMarkupServices *iface, REFIID riid, void **ppvObject)
{
    HTMLDocumentNode *This = impl_from_IMarkupServices(iface);
    return IHTMLDOMNode_QueryInterface(&This->node.IHTMLDOMNode_iface, riid, ppvObject);
}

static ULONG WINAPI MarkupServices_AddRef(IMarkupServices *iface)
{
    HTMLDocumentNode *This = impl_from_IMarkupServices(iface);
    return IHTMLDOMNode_AddRef(&This->node.IHTMLDOMNode_iface);
}

static ULONG WINAPI MarkupServices_Release(IMarkupServices *iface)
{
    HTMLDocumentNode *This = impl_from_IMarkupServices(iface);
    return IHTMLDOMNode_Release(&This->node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI MarkupServices_CreateMarkupPointer(IMarkupServices *iface, IMarkupPointer **ppPointer)
{
    HTMLDocumentNode *This = impl_from_IMarkupServices(iface);

    TRACE("(%p)->(%p)\n", This, ppPointer);

    return create_markup_pointer(ppPointer);
}

static HRESULT WINAPI MarkupServices_CreateMarkupContainer(IMarkupServices *iface, IMarkupContainer **ppMarkupContainer)
{
    HTMLDocumentNode *This = impl_from_IMarkupServices(iface);
    IHTMLDocument2 *frag;
    HRESULT hres;
    TRACE("(%p)->(%p)\n", This, ppMarkupContainer);

    hres = IHTMLDocument3_createDocumentFragment(&This->IHTMLDocument3_iface, &frag);
    if(FAILED(hres))
        return hres;

    IHTMLDocument2_QueryInterface(frag, &IID_IMarkupContainer, (void**)ppMarkupContainer);
    IHTMLDocument2_Release(frag);

    return S_OK;
}

static HRESULT WINAPI MarkupServices_CreateElement(IMarkupServices *iface,
    ELEMENT_TAG_ID tagID, OLECHAR *pchAttributes, IHTMLElement **ppElement)
{
    HTMLDocumentNode *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%d,%s,%p)\n", This, tagID, debugstr_w(pchAttributes), ppElement);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_CloneElement(IMarkupServices *iface,
    IHTMLElement *pElemCloneThis, IHTMLElement **ppElementTheClone)
{
    HTMLDocumentNode *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%p,%p)\n", This, pElemCloneThis, ppElementTheClone);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_InsertElement(IMarkupServices *iface,
    IHTMLElement *pElementInsert, IMarkupPointer *pPointerStart,
    IMarkupPointer *pPointerFinish)
{
    HTMLDocumentNode *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%p,%p,%p)\n", This, pElementInsert, pPointerStart, pPointerFinish);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_RemoveElement(IMarkupServices *iface, IHTMLElement *pElementRemove)
{
    HTMLDocumentNode *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%p)\n", This, pElementRemove);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_Remove(IMarkupServices *iface,
    IMarkupPointer *pPointerStart, IMarkupPointer *pPointerFinish)
{
    HTMLDocumentNode *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%p,%p)\n", This, pPointerStart, pPointerFinish);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_Copy(IMarkupServices *iface,
    IMarkupPointer *pPointerSourceStart, IMarkupPointer *pPointerSourceFinish,
    IMarkupPointer *pPointerTarget)
{
    HTMLDocumentNode *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%p,%p,%p)\n", This, pPointerSourceStart, pPointerSourceFinish, pPointerTarget);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_Move(IMarkupServices *iface,
    IMarkupPointer *pPointerSourceStart, IMarkupPointer *pPointerSourceFinish,
    IMarkupPointer *pPointerTarget)
{
    HTMLDocumentNode *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%p,%p,%p)\n", This, pPointerSourceStart, pPointerSourceFinish, pPointerTarget);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_InsertText(IMarkupServices *iface,
    OLECHAR *pchText, LONG cch, IMarkupPointer *pPointerTarget)
{
    HTMLDocumentNode *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%s,%lx,%p)\n", This, debugstr_w(pchText), cch, pPointerTarget);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_ParseString(IMarkupServices *iface,
    OLECHAR *pchHTML, DWORD dwFlags, IMarkupContainer **ppContainerResult,
    IMarkupPointer *pPointerStart, IMarkupPointer *pPointerFinish)
{
    HRESULT hres;
    IMarkupContainer *container;
    IHTMLDocument2 *doc;
    IHTMLDOMNode *node, *html_node, *new_html_node = NULL;
    IHTMLElement *html, *body;
    HTMLDocumentNode *This = impl_from_IMarkupServices(iface);
    TRACE("(%p)->(%s,%lx,%p,%p,%p)\n", This, debugstr_w(pchHTML), dwFlags, ppContainerResult, pPointerStart, pPointerFinish);

    if(dwFlags != 0)
        FIXME("flags %lx not implemented.\n", dwFlags);
    if(pPointerStart || pPointerFinish) {
        FIXME("Pointers not implemented.\n");
        return E_NOTIMPL;
    }

    hres = IMarkupServices_CreateMarkupContainer(iface, &container);
    if(FAILED(hres))
        return hres;

    IMarkupContainer_QueryInterface(container, &IID_IHTMLDocument2, (void**)&doc);
    IHTMLDocument2_QueryInterface(doc, &IID_IHTMLDOMNode, (void**)&node);
    IHTMLDocument2_createElement(&This->IHTMLDocument2_iface, (BSTR)L"html", &html);

    IHTMLElement_put_innerHTML(html, (BSTR)L"<head><title></title></head><body></body>");
    IHTMLElement_QueryInterface(html, &IID_IHTMLDOMNode, (void**)&html_node);
    IHTMLElement_Release(html);

    IHTMLDOMNode_appendChild(node, html_node, &new_html_node);
    IHTMLDOMNode_Release(node);
    IHTMLDOMNode_Release(html_node);
    IHTMLDOMNode_Release(new_html_node);

    IHTMLDocument2_get_body(doc, &body);
    hres = IHTMLElement_put_innerHTML(body, pchHTML);
    if (FAILED(hres))
    {
        ERR("Failed put_innerHTML hr %#lx\n", hres);
        IMarkupContainer_Release(container);
        container = NULL;
    }

    IHTMLDocument2_Release(doc);
    IHTMLElement_Release(body);

    *ppContainerResult = container;

    return hres;
}

static HRESULT WINAPI MarkupServices_ParseGlobal(IMarkupServices *iface,
    HGLOBAL hglobalHTML, DWORD dwFlags, IMarkupContainer **ppContainerResult,
    IMarkupPointer *pPointerStart, IMarkupPointer *pPointerFinish)
{
    HTMLDocumentNode *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%s,%lx,%p,%p,%p)\n", This, debugstr_w(hglobalHTML), dwFlags, ppContainerResult, pPointerStart, pPointerFinish);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_IsScopedElement(IMarkupServices *iface,
    IHTMLElement *pElement, BOOL *pfScoped)
{
    HTMLDocumentNode *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%p,%p)\n", This, pElement, pfScoped);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_GetElementTagId(IMarkupServices *iface,
    IHTMLElement *pElement, ELEMENT_TAG_ID *ptagId)
{
    HTMLDocumentNode *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%p,%p)\n", This, pElement, ptagId);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_GetTagIDForName(IMarkupServices *iface,
    BSTR bstrName, ELEMENT_TAG_ID *ptagId)
{
    HTMLDocumentNode *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%s,%p)\n", This, debugstr_w(bstrName), ptagId);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_GetNameForTagID(IMarkupServices *iface,
    ELEMENT_TAG_ID tagId, BSTR *pbstrName)
{
    HTMLDocumentNode *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%d,%p)\n", This, tagId, pbstrName);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_MovePointersToRange(IMarkupServices *iface,
    IHTMLTxtRange *pIRange, IMarkupPointer *pPointerStart, IMarkupPointer *pPointerFinish)
{
    HTMLDocumentNode *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%p,%p,%p)\n", This, pIRange, pPointerStart, pPointerFinish);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_MoveRangeToPointers(IMarkupServices *iface,
    IMarkupPointer *pPointerStart, IMarkupPointer *pPointerFinish, IHTMLTxtRange *pIRange)
{
    HTMLDocumentNode *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%p,%p,%p)\n", This, pPointerStart, pPointerFinish, pIRange);
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_BeginUndoUnit(IMarkupServices *iface, OLECHAR *pchTitle)
{
    HTMLDocumentNode *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(pchTitle));
    return E_NOTIMPL;
}

static HRESULT WINAPI MarkupServices_EndUndoUnit(IMarkupServices *iface)
{
    HTMLDocumentNode *This = impl_from_IMarkupServices(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static const IMarkupServicesVtbl MarkupServicesVtbl = {
    MarkupServices_QueryInterface,
    MarkupServices_AddRef,
    MarkupServices_Release,
    MarkupServices_CreateMarkupPointer,
    MarkupServices_CreateMarkupContainer,
    MarkupServices_CreateElement,
    MarkupServices_CloneElement,
    MarkupServices_InsertElement,
    MarkupServices_RemoveElement,
    MarkupServices_Remove,
    MarkupServices_Copy,
    MarkupServices_Move,
    MarkupServices_InsertText,
    MarkupServices_ParseString,
    MarkupServices_ParseGlobal,
    MarkupServices_IsScopedElement,
    MarkupServices_GetElementTagId,
    MarkupServices_GetTagIDForName,
    MarkupServices_GetNameForTagID,
    MarkupServices_MovePointersToRange,
    MarkupServices_MoveRangeToPointers,
    MarkupServices_BeginUndoUnit,
    MarkupServices_EndUndoUnit
};

/**********************************************************
 * IMarkupContainer implementation
 */
static inline HTMLDocumentNode *impl_from_IMarkupContainer(IMarkupContainer *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IMarkupContainer_iface);
}

static HRESULT WINAPI MarkupContainer_QueryInterface(IMarkupContainer *iface, REFIID riid, void **ppvObject)
{
    HTMLDocumentNode *This = impl_from_IMarkupContainer(iface);
    return IHTMLDOMNode_QueryInterface(&This->node.IHTMLDOMNode_iface, riid, ppvObject);
}

static ULONG WINAPI MarkupContainer_AddRef(IMarkupContainer *iface)
{
    HTMLDocumentNode *This = impl_from_IMarkupContainer(iface);
    return IHTMLDOMNode_AddRef(&This->node.IHTMLDOMNode_iface);
}

static ULONG WINAPI MarkupContainer_Release(IMarkupContainer *iface)
{
    HTMLDocumentNode *This = impl_from_IMarkupContainer(iface);
    return IHTMLDOMNode_Release(&This->node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI MarkupContainer_OwningDoc(IMarkupContainer *iface, IHTMLDocument2 **ppDoc)
{
    HTMLDocumentNode *This = impl_from_IMarkupContainer(iface);
    FIXME("(%p)->(%p)\n", This, ppDoc);
    return E_NOTIMPL;
}

static const IMarkupContainerVtbl MarkupContainerVtbl = {
    MarkupContainer_QueryInterface,
    MarkupContainer_AddRef,
    MarkupContainer_Release,
    MarkupContainer_OwningDoc
};

/**********************************************************
 * IDisplayServices implementation
 */
static inline HTMLDocumentNode *impl_from_IDisplayServices(IDisplayServices *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IDisplayServices_iface);
}

static HRESULT WINAPI DisplayServices_QueryInterface(IDisplayServices *iface, REFIID riid, void **ppvObject)
{
    HTMLDocumentNode *This = impl_from_IDisplayServices(iface);
    return IHTMLDOMNode_QueryInterface(&This->node.IHTMLDOMNode_iface, riid, ppvObject);
}

static ULONG WINAPI DisplayServices_AddRef(IDisplayServices *iface)
{
    HTMLDocumentNode *This = impl_from_IDisplayServices(iface);
    return IHTMLDOMNode_AddRef(&This->node.IHTMLDOMNode_iface);
}

static ULONG WINAPI DisplayServices_Release(IDisplayServices *iface)
{
    HTMLDocumentNode *This = impl_from_IDisplayServices(iface);
    return IHTMLDOMNode_Release(&This->node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI DisplayServices_CreateDisplayPointer(IDisplayServices *iface, IDisplayPointer **ppDispPointer)
{
    HTMLDocumentNode *This = impl_from_IDisplayServices(iface);
    FIXME("(%p)->(%p)\n", This, ppDispPointer);
    return E_NOTIMPL;
}

static HRESULT WINAPI DisplayServices_TransformRect(IDisplayServices *iface,
    RECT *pRect, COORD_SYSTEM eSource, COORD_SYSTEM eDestination, IHTMLElement *pIElement)
{
    HTMLDocumentNode *This = impl_from_IDisplayServices(iface);
    FIXME("(%p)->(%s,%d,%d,%p)\n", This, wine_dbgstr_rect(pRect), eSource, eDestination, pIElement);
    return E_NOTIMPL;
}

static HRESULT WINAPI DisplayServices_TransformPoint(IDisplayServices *iface,
    POINT *pPoint, COORD_SYSTEM eSource, COORD_SYSTEM eDestination, IHTMLElement *pIElement)
{
    HTMLDocumentNode *This = impl_from_IDisplayServices(iface);
    FIXME("(%p)->(%s,%d,%d,%p)\n", This, wine_dbgstr_point(pPoint), eSource, eDestination, pIElement);
    return E_NOTIMPL;
}

static HRESULT WINAPI DisplayServices_GetCaret(IDisplayServices *iface, IHTMLCaret **ppCaret)
{
    HTMLDocumentNode *This = impl_from_IDisplayServices(iface);
    FIXME("(%p)->(%p)\n", This, ppCaret);
    return E_NOTIMPL;
}

static HRESULT WINAPI DisplayServices_GetComputedStyle(IDisplayServices *iface,
    IMarkupPointer *pPointer, IHTMLComputedStyle **ppComputedStyle)
{
    HTMLDocumentNode *This = impl_from_IDisplayServices(iface);
    FIXME("(%p)->(%p,%p)\n", This, pPointer, ppComputedStyle);
    return E_NOTIMPL;
}

static HRESULT WINAPI DisplayServices_ScrollRectIntoView(IDisplayServices *iface,
    IHTMLElement *pIElement, RECT rect)
{
    HTMLDocumentNode *This = impl_from_IDisplayServices(iface);
    FIXME("(%p)->(%p,%s)\n", This, pIElement, wine_dbgstr_rect(&rect));
    return E_NOTIMPL;
}

static HRESULT WINAPI DisplayServices_HasFlowLayout(IDisplayServices *iface,
    IHTMLElement *pIElement, BOOL *pfHasFlowLayout)
{
    HTMLDocumentNode *This = impl_from_IDisplayServices(iface);
    FIXME("(%p)->(%p,%p)\n", This, pIElement, pfHasFlowLayout);
    return E_NOTIMPL;
}

static const IDisplayServicesVtbl DisplayServicesVtbl = {
    DisplayServices_QueryInterface,
    DisplayServices_AddRef,
    DisplayServices_Release,
    DisplayServices_CreateDisplayPointer,
    DisplayServices_TransformRect,
    DisplayServices_TransformPoint,
    DisplayServices_GetCaret,
    DisplayServices_GetComputedStyle,
    DisplayServices_ScrollRectIntoView,
    DisplayServices_HasFlowLayout
};

/**********************************************************
 * IDocumentRange implementation
 */
static inline HTMLDocumentNode *impl_from_IDocumentRange(IDocumentRange *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IDocumentRange_iface);
}

DISPEX_IDISPATCH_IMPL(DocumentRange, IDocumentRange,
                      impl_from_IDocumentRange(iface)->node.event_target.dispex)

static HRESULT WINAPI DocumentRange_createRange(IDocumentRange *iface, IHTMLDOMRange **p)
{
    HTMLDocumentNode *This = impl_from_IDocumentRange(iface);
    nsIDOMRange *nsrange;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->dom_document) {
        WARN("NULL dom_document\n");
        return E_UNEXPECTED;
    }

    if(NS_FAILED(nsIDOMDocument_CreateRange(This->dom_document, &nsrange)))
        return E_FAIL;

    hres = create_dom_range(nsrange, This, p);
    nsIDOMRange_Release(nsrange);
    return hres;
}

static const IDocumentRangeVtbl DocumentRangeVtbl = {
    DocumentRange_QueryInterface,
    DocumentRange_AddRef,
    DocumentRange_Release,
    DocumentRange_GetTypeInfoCount,
    DocumentRange_GetTypeInfo,
    DocumentRange_GetIDsOfNames,
    DocumentRange_Invoke,
    DocumentRange_createRange
};

static cp_static_data_t HTMLDocumentNodeEvents_data = { HTMLDocumentEvents_tid, HTMLDocumentNode_on_advise };
static cp_static_data_t HTMLDocumentNodeEvents2_data = { HTMLDocumentEvents2_tid, HTMLDocumentNode_on_advise, TRUE };

static const cpc_entry_t HTMLDocumentNode_cpc[] = {
    {&IID_IDispatch, &HTMLDocumentNodeEvents_data},
    {&IID_IPropertyNotifySink},
    {&DIID_HTMLDocumentEvents, &HTMLDocumentNodeEvents_data},
    {&DIID_HTMLDocumentEvents2, &HTMLDocumentNodeEvents2_data},
    {NULL}
};

static inline HTMLDocumentNode *impl_from_HTMLDOMNode(HTMLDOMNode *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, node);
}

void detach_document_node(HTMLDocumentNode *doc)
{
    unsigned i;

    while(!list_empty(&doc->plugin_hosts))
        detach_plugin_host(LIST_ENTRY(list_head(&doc->plugin_hosts), PluginHost, entry));

    if(doc->dom_implementation)
        detach_dom_implementation(doc->dom_implementation);

    unlink_ref(&doc->dom_implementation);
    unlink_ref(&doc->namespaces);
    detach_events(doc);
    detach_selection(doc);
    detach_ranges(doc);

    for(i=0; i < doc->elem_vars_cnt; i++)
        free(doc->elem_vars[i]);
    free(doc->elem_vars);
    doc->elem_vars_cnt = doc->elem_vars_size = 0;
    doc->elem_vars = NULL;

    unlink_ref(&doc->catmgr);
    if(doc->browser) {
        list_remove(&doc->browser_entry);
        doc->browser = NULL;
    }
}

static HRESULT HTMLDocumentNode_clone(HTMLDOMNode *iface, nsIDOMNode *nsnode, HTMLDOMNode **ret)
{
    HTMLDocumentNode *This = impl_from_HTMLDOMNode(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static const NodeImplVtbl HTMLDocumentNodeImplVtbl = {
    .clsid                 = &CLSID_HTMLDocument,
    .cpc_entries           = HTMLDocumentNode_cpc,
    .clone                 = HTMLDocumentNode_clone,
};

static HRESULT HTMLDocumentFragment_clone(HTMLDOMNode *iface, nsIDOMNode *nsnode, HTMLDOMNode **ret)
{
    HTMLDocumentNode *This = impl_from_HTMLDOMNode(iface);
    HTMLDocumentNode *new_node;
    HRESULT hres;

    hres = create_document_fragment(nsnode, This->node.doc, &new_node);
    if(FAILED(hres))
        return hres;

    *ret = &new_node->node;
    return S_OK;
}

static inline HTMLDocumentNode *impl_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, node.event_target.dispex);
}

static void *HTMLDocumentNode_query_interface(DispatchEx *dispex, REFIID riid)
{
    HTMLDocumentNode *This = impl_from_DispatchEx(dispex);

    if(IsEqualGUID(&IID_IHTMLDocument, riid) || IsEqualGUID(&IID_IHTMLDocument2, riid))
        return &This->IHTMLDocument2_iface;
    if(IsEqualGUID(&IID_IHTMLDocument3, riid))
        return &This->IHTMLDocument3_iface;
    if(IsEqualGUID(&IID_IHTMLDocument4, riid))
        return &This->IHTMLDocument4_iface;
    if(IsEqualGUID(&IID_IHTMLDocument5, riid))
        return &This->IHTMLDocument5_iface;
    if(IsEqualGUID(&IID_IHTMLDocument6, riid))
        return &This->IHTMLDocument6_iface;
    if(IsEqualGUID(&IID_IHTMLDocument7, riid))
        return &This->IHTMLDocument7_iface;
    if(IsEqualGUID(&IID_IDocumentSelector, riid))
        return &This->IDocumentSelector_iface;
    if(IsEqualGUID(&IID_IDocumentEvent, riid))
        return &This->IDocumentEvent_iface;
    if(IsEqualGUID(&DIID_DispHTMLDocument, riid))
        return &This->IHTMLDocument2_iface;
    if(IsEqualGUID(&IID_ISupportErrorInfo, riid))
        return &This->ISupportErrorInfo_iface;
    if(IsEqualGUID(&IID_IProvideClassInfo, riid))
        return &This->IProvideMultipleClassInfo_iface;
    if(IsEqualGUID(&IID_IProvideClassInfo2, riid))
        return &This->IProvideMultipleClassInfo_iface;
    if(IsEqualGUID(&IID_IProvideMultipleClassInfo, riid))
        return &This->IProvideMultipleClassInfo_iface;
    if(IsEqualGUID(&IID_IMarkupServices, riid))
        return &This->IMarkupServices_iface;
    if(IsEqualGUID(&IID_IMarkupContainer, riid))
        return &This->IMarkupContainer_iface;
    if(IsEqualGUID(&IID_IDisplayServices, riid))
        return &This->IDisplayServices_iface;
    if(IsEqualGUID(&IID_IDocumentRange, riid))
        return &This->IDocumentRange_iface;
    if(IsEqualGUID(&IID_IPersist, riid))
        return &This->IPersistFile_iface;
    if(IsEqualGUID(&IID_IPersistMoniker, riid))
        return &This->IPersistMoniker_iface;
    if(IsEqualGUID(&IID_IPersistFile, riid))
        return &This->IPersistFile_iface;
    if(IsEqualGUID(&IID_IMonikerProp, riid))
        return &This->IMonikerProp_iface;
    if(IsEqualGUID(&IID_IPersistStreamInit, riid))
        return &This->IPersistStreamInit_iface;
    if(IsEqualGUID(&IID_IPersistHistory, riid))
        return &This->IPersistHistory_iface;
    if(IsEqualGUID(&IID_IHlinkTarget, riid))
        return &This->IHlinkTarget_iface;
    if(IsEqualGUID(&IID_IOleCommandTarget, riid))
        return &This->IOleCommandTarget_iface;
    if(IsEqualGUID(&IID_IOleObject, riid))
        return &This->IOleObject_iface;
    if(IsEqualGUID(&IID_IOleDocument, riid))
        return &This->IOleDocument_iface;
    if(IsEqualGUID(&IID_IOleInPlaceActiveObject, riid))
        return &This->IOleInPlaceActiveObject_iface;
    if(IsEqualGUID(&IID_IOleWindow, riid))
        return &This->IOleInPlaceActiveObject_iface;
    if(IsEqualGUID(&IID_IOleInPlaceObject, riid))
        return &This->IOleInPlaceObjectWindowless_iface;
    if(IsEqualGUID(&IID_IOleInPlaceObjectWindowless, riid))
        return &This->IOleInPlaceObjectWindowless_iface;
    if(IsEqualGUID(&IID_IOleControl, riid))
        return &This->IOleControl_iface;
    if(IsEqualGUID(&IID_IObjectWithSite, riid))
        return &This->IObjectWithSite_iface;
    if(IsEqualGUID(&IID_IOleContainer, riid))
        return &This->IOleContainer_iface;
    if(IsEqualGUID(&IID_IObjectSafety, riid))
        return &This->IObjectSafety_iface;
    if(IsEqualGUID(&IID_IServiceProvider, riid))
        return &This->IServiceProvider_iface;
    if(IsEqualGUID(&IID_IInternetHostSecurityManager, riid))
        return &This->IInternetHostSecurityManager_iface;
    if(IsEqualGUID(&IID_IConnectionPointContainer, riid))
        return &This->cp_container.IConnectionPointContainer_iface;
    if(IsEqualGUID(&CLSID_CMarkup, riid)) {
        FIXME("(%p)->(CLSID_CMarkup)\n", This);
        return NULL;
    }else if(IsEqualGUID(&IID_IRunnableObject, riid)) {
        return NULL;
    }else if(IsEqualGUID(&IID_IPersistPropertyBag, riid)) {
        return NULL;
    }else if(IsEqualGUID(&IID_IExternalConnection, riid)) {
        return NULL;
    }else if(IsEqualGUID(&IID_IStdMarshalInfo, riid)) {
        return NULL;
    }

    return HTMLDOMNode_query_interface(&This->node.event_target.dispex, riid);
}

static void HTMLDocumentNode_traverse(DispatchEx *dispex, nsCycleCollectionTraversalCallback *cb)
{
    HTMLDocumentNode *This = impl_from_DispatchEx(dispex);
    HTMLDOMNode_traverse(dispex, cb);

    if(This->window)
        note_cc_edge((nsISupports*)&This->window->base.IHTMLWindow2_iface, "window", cb);
    if(This->dom_implementation)
        note_cc_edge((nsISupports*)This->dom_implementation, "dom_implementation", cb);
    if(This->namespaces)
        note_cc_edge((nsISupports*)This->namespaces, "namespaces", cb);
}

static void HTMLDocumentNode_unlink(DispatchEx *dispex)
{
    HTMLDocumentNode *This = impl_from_DispatchEx(dispex);
    HTMLInnerWindow *window = This->window;
    HTMLDOMNode_unlink(dispex);

    if(This->script_global) {
        list_remove(&This->script_global_entry);
        This->script_global = NULL;
    }
    if(window) {
        This->window = NULL;
        IHTMLWindow2_Release(&window->base.IHTMLWindow2_iface);
    }

    if(This->dom_document) {
        release_document_mutation(This);
        detach_document_node(This);
        This->dom_document = NULL;
        This->html_document = NULL;
    }else if(window) {
        detach_document_node(This);
    }
}

static void HTMLDocumentNode_destructor(DispatchEx *dispex)
{
    HTMLDocumentNode *This = impl_from_DispatchEx(dispex);

    free(This->event_vector);
    ConnectionPointContainer_Destroy(&This->cp_container);
    HTMLDOMNode_destructor(&This->node.event_target.dispex);
}

static HRESULT HTMLDocumentNode_get_dispid(DispatchEx *dispex, const WCHAR *name, DWORD grfdex, DISPID *dispid)
{
    HTMLDocumentNode *This = impl_from_DispatchEx(dispex);
    HRESULT hres = DISP_E_UNKNOWNNAME;

    if(This->html_document && dispex_compat_mode(&This->node.event_target.dispex) < COMPAT_MODE_IE9) {
        hres = get_elem_by_name_or_id(This->html_document, name, NULL);
        if(SUCCEEDED(hres))
            hres = dispid_from_elem_name(This, name, dispid);
    }

    return hres;
}

static HRESULT HTMLDocumentNode_find_dispid(DispatchEx *dispex, const WCHAR *name, DWORD grfdex, DISPID *dispid)
{
    HTMLDocumentNode *This = impl_from_DispatchEx(dispex);
    HRESULT hres = DISP_E_UNKNOWNNAME;

    if(This->html_document && dispex_compat_mode(&This->node.event_target.dispex) >= COMPAT_MODE_IE9) {
        hres = get_elem_by_name_or_id(This->html_document, name, NULL);
        if(SUCCEEDED(hres))
            hres = dispid_from_elem_name(This, name, dispid);
    }

    return hres;
}

static HRESULT HTMLDocumentNode_invoke(DispatchEx *dispex, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLDocumentNode *This = impl_from_DispatchEx(dispex);
    nsIDOMElement *nselem;
    HTMLDOMNode *node;
    unsigned i;
    HRESULT hres;

    if(flags != DISPATCH_PROPERTYGET && flags != (DISPATCH_METHOD|DISPATCH_PROPERTYGET))
        return MSHTML_E_INVALID_PROPERTY;

    i = id - MSHTML_DISPID_CUSTOM_MIN;

    if(!This->html_document || i >= This->elem_vars_cnt)
        return DISP_E_MEMBERNOTFOUND;

    hres = get_elem_by_name_or_id(This->html_document, This->elem_vars[i], &nselem);
    if(FAILED(hres))
        return hres;
    if(!nselem)
        return DISP_E_MEMBERNOTFOUND;

    hres = get_node((nsIDOMNode*)nselem, TRUE, &node);
    nsIDOMElement_Release(nselem);
    if(FAILED(hres))
        return hres;

    V_VT(res) = VT_DISPATCH;
    V_DISPATCH(res) = (IDispatch*)&node->IHTMLDOMNode_iface;
    return S_OK;
}

static HRESULT HTMLDocumentNode_disp_invoke(DispatchEx *dispex, DISPID id, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLDocumentNode *This = impl_from_DispatchEx(dispex);

    if(This->window) {
        switch(id) {
        case DISPID_READYSTATE:
            TRACE("DISPID_READYSTATE\n");

            if(!(flags & DISPATCH_PROPERTYGET))
                return E_INVALIDARG;

            V_VT(res) = VT_I4;
            V_I4(res) = This->window->base.outer_window->readystate;
            return S_OK;
        default:
            break;
        }
    }

    return S_FALSE;
}

static HRESULT HTMLDocumentNode_next_dispid(DispatchEx *dispex, DISPID id, DISPID *pid)
{
    DWORD idx = (id == DISPID_STARTENUM) ? 0 : id - MSHTML_DISPID_CUSTOM_MIN + 1;
    HTMLDocumentNode *This = impl_from_DispatchEx(dispex);
    nsIDOMNodeList *node_list = NULL;
    const PRUnichar *name;
    nsIDOMElement *nselem;
    nsIDOMNode *nsnode;
    nsAString nsstr;
    nsresult nsres;
    HRESULT hres;
    UINT32 i;

    if(!This->html_document)
        return S_FALSE;

    while(idx < This->elem_vars_cnt) {
        hres = has_elem_name(This->html_document, This->elem_vars[idx]);
        if(SUCCEEDED(hres)) {
            *pid = idx + MSHTML_DISPID_CUSTOM_MIN;
            return S_OK;
        }
        if(hres != DISP_E_UNKNOWNNAME)
            return hres;
        idx++;
    }

    /* Populate possibly missing DISPIDs */
    nsAString_InitDepend(&nsstr, L":-moz-any(applet,embed,form,iframe,img,object)[name]");
    nsres = nsIDOMHTMLDocument_QuerySelectorAll(This->html_document, &nsstr, &node_list);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres)) {
        if(node_list)
            nsIDOMNodeList_Release(node_list);
        return map_nsresult(nsres);
    }

    for(i = 0, hres = S_OK; SUCCEEDED(hres); i++) {
        nsres = nsIDOMNodeList_Item(node_list, i, &nsnode);
        if(NS_FAILED(nsres)) {
            hres = map_nsresult(nsres);
            break;
        }
        if(!nsnode)
            break;

        nsres = nsIDOMNode_QueryInterface(nsnode, &IID_nsIDOMElement, (void**)&nselem);
        nsIDOMNode_Release(nsnode);
        if(nsres != S_OK)
            continue;

        nsres = get_elem_attr_value(nselem, L"name", &nsstr, &name);
        nsIDOMElement_Release(nselem);
        if(NS_FAILED(nsres))
            hres = map_nsresult(nsres);
        else {
            hres = dispid_from_elem_name(This, name, &id);
            nsAString_Finish(&nsstr);
        }
    }
    nsIDOMNodeList_Release(node_list);
    if(FAILED(hres))
        return hres;

    if(idx >= This->elem_vars_cnt)
        return S_FALSE;

    *pid = idx + MSHTML_DISPID_CUSTOM_MIN;
    return S_OK;
}

static HRESULT HTMLDocumentNode_get_prop_desc(DispatchEx *dispex, DISPID id, struct property_info *desc)
{
    HTMLDocumentNode *This = impl_from_DispatchEx(dispex);
    DWORD idx = id - MSHTML_DISPID_CUSTOM_MIN;

    if(!This->dom_document || idx >= This->elem_vars_cnt)
        return DISP_E_MEMBERNOTFOUND;

    desc->name = This->elem_vars[idx];
    desc->id = id;
    desc->flags = PROPF_WRITABLE | PROPF_CONFIGURABLE | PROPF_ENUMERABLE;
    desc->iid = 0;
    return S_OK;
}

static HTMLInnerWindow *HTMLDocumentNode_get_script_global(DispatchEx *dispex, dispex_static_data_t **dispex_data)
{
    HTMLDocumentNode *This = impl_from_DispatchEx(dispex);

    TRACE("(%p) using %u compat mode\n", This, This->document_mode);

    lock_document_mode(This);
    if(This->node.vtbl != &HTMLDocumentNodeImplVtbl)
        *dispex_data = &DocumentFragment_dispex;
    else
        *dispex_data = This->document_mode < COMPAT_MODE_IE11 ? &Document_dispex : &HTMLDocument_dispex;
    return This->script_global;
}

static nsISupports *HTMLDocumentNode_get_gecko_target(DispatchEx *dispex)
{
    HTMLDocumentNode *This = impl_from_DispatchEx(dispex);
    return (nsISupports*)This->node.nsnode;
}

static void HTMLDocumentNode_bind_event(DispatchEx *dispex, eventid_t eid)
{
    HTMLDocumentNode *This = impl_from_DispatchEx(dispex);
    ensure_doc_nsevent_handler(This, This->node.nsnode, eid);
}

static EventTarget *HTMLDocumentNode_get_parent_event_target(DispatchEx *dispex)
{
    HTMLDocumentNode *This = impl_from_DispatchEx(dispex);
    if(!This->window)
        return NULL;
    IHTMLWindow2_AddRef(&This->window->base.IHTMLWindow2_iface);
    return &This->window->event_target;
}

static ConnectionPointContainer *HTMLDocumentNode_get_cp_container(DispatchEx *dispex)
{
    HTMLDocumentNode *This = impl_from_DispatchEx(dispex);
    ConnectionPointContainer *container = This->doc_obj
        ? &This->doc_obj->cp_container : &This->cp_container;
    IConnectionPointContainer_AddRef(&container->IConnectionPointContainer_iface);
    return container;
}

static IHTMLEventObj *HTMLDocumentNode_set_current_event(DispatchEx *dispex, IHTMLEventObj *event)
{
    HTMLDocumentNode *This = impl_from_DispatchEx(dispex);
    return default_set_current_event(This->window, event);
}

static HRESULT HTMLDocumentNode_location_hook(DispatchEx *dispex, WORD flags, DISPPARAMS *dp, VARIANT *res,
        EXCEPINFO *ei, IServiceProvider *caller)
{
    HTMLDocumentNode *This = impl_from_DispatchEx(dispex);

    if(!(flags & DISPATCH_PROPERTYPUT) || !This->window)
        return S_FALSE;
    if(!This->window->base.outer_window)
        return E_FAIL;

    return IWineJSDispatchHost_InvokeEx(&This->window->event_target.dispex.IWineJSDispatchHost_iface,
                                    DISPID_IHTMLWINDOW2_LOCATION, 0, flags, dp, res, ei, caller);
}

static HRESULT HTMLDocumentNode_pre_handle_event(DispatchEx* dispex, DOMEvent *event)
{
    HTMLDocumentNode *doc = impl_from_DispatchEx(dispex);
    switch(event->event_id) {
    case EVENTID_DOMCONTENTLOADED: {
        if(event->trusted && doc->window)
            doc->window->dom_content_loaded_event_start_time = get_time_stamp();
        break;
    }
    default:
        break;
    }

    return S_OK;
}

static HRESULT HTMLDocumentNode_handle_event(DispatchEx* dispex, DOMEvent *event, BOOL *prevent_default)
{
    HTMLDocumentNode *doc = impl_from_DispatchEx(dispex);
    switch(event->event_id) {
    case EVENTID_DOMCONTENTLOADED: {
        if(event->trusted && doc->window)
            doc->window->dom_content_loaded_event_end_time = get_time_stamp();
        break;
    }
    default:
        break;
    }

    return S_OK;
}

static const event_target_vtbl_t HTMLDocument_event_target_vtbl = {
    {
        .query_interface     = HTMLDocumentNode_query_interface,
        .destructor          = HTMLDocumentNode_destructor,
        .traverse            = HTMLDocumentNode_traverse,
        .unlink              = HTMLDocumentNode_unlink,
        .get_dispid          = HTMLDocumentNode_get_dispid,
        .find_dispid         = HTMLDocumentNode_find_dispid,
        .get_prop_desc       = HTMLDocumentNode_get_prop_desc,
        .invoke              = HTMLDocumentNode_invoke,
        .disp_invoke         = HTMLDocumentNode_disp_invoke,
        .next_dispid         = HTMLDocumentNode_next_dispid,
        .get_script_global   = HTMLDocumentNode_get_script_global,
    },
    .get_gecko_target        = HTMLDocumentNode_get_gecko_target,
    .bind_event              = HTMLDocumentNode_bind_event,
    .get_parent_event_target = HTMLDocumentNode_get_parent_event_target,
    .pre_handle_event        = HTMLDocumentNode_pre_handle_event,
    .handle_event            = HTMLDocumentNode_handle_event,
    .get_cp_container        = HTMLDocumentNode_get_cp_container,
    .set_current_event       = HTMLDocumentNode_set_current_event,
};

static const NodeImplVtbl HTMLDocumentFragmentImplVtbl = {
    .clsid                 = &CLSID_HTMLDocument,
    .cpc_entries           = HTMLDocumentNode_cpc,
    .clone                 = HTMLDocumentFragment_clone,
};

static const tid_t HTMLDocumentNode_iface_tids[] = {
    IHTMLDOMNode_tid,
    IHTMLDocument5_tid,
    IDocumentSelector_tid,
    0
};

static void HTMLDocumentNode_init_dispex_info(dispex_data_t *info, compat_mode_t mode)
{
    static const dispex_hook_t document2_ie9_hooks[] = {
        {DISPID_IHTMLDOCUMENT2_CLEAR},
        {DISPID_IHTMLDOCUMENT2_EXPANDO},
        {DISPID_IHTMLDOCUMENT2_TOSTRING},
        {DISPID_IHTMLDOCUMENT2_URL,      NULL, L"URL"},
        {DISPID_IHTMLDOCUMENT2_LOCATION, HTMLDocumentNode_location_hook},
        {DISPID_UNKNOWN}
    };
    static const dispex_hook_t document2_ie11_hooks[] = {
        {DISPID_IHTMLDOCUMENT2_CREATESTYLESHEET},
        {DISPID_IHTMLDOCUMENT2_FILESIZE},
        {DISPID_IHTMLDOCUMENT2_SELECTION},
        {DISPID_IHTMLDOCUMENT2_ONAFTERUPDATE},
        {DISPID_IHTMLDOCUMENT2_ONROWEXIT},
        {DISPID_IHTMLDOCUMENT2_ONROWENTER},
        {DISPID_IHTMLDOCUMENT2_ONBEFOREUPDATE},
        {DISPID_IHTMLDOCUMENT2_ONERRORUPDATE},

        /* IE10+ */
        {DISPID_IHTMLDOCUMENT2_EXPANDO},
        {DISPID_IHTMLDOCUMENT2_TOSTRING},

        /* all modes */
        {DISPID_IHTMLDOCUMENT2_URL,      NULL, L"URL"},
        {DISPID_IHTMLDOCUMENT2_LOCATION, HTMLDocumentNode_location_hook},
        {DISPID_UNKNOWN}
    };
    const dispex_hook_t *const document2_ie10_hooks = document2_ie11_hooks + 8;
    const dispex_hook_t *const document2_hooks      = document2_ie10_hooks + 2;
    static const dispex_hook_t document3_ie11_hooks[] = {
        {DISPID_IHTMLDOCUMENT3_ATTACHEVENT},
        {DISPID_IHTMLDOCUMENT3_DETACHEVENT},
        {DISPID_IHTMLDOCUMENT3_ONROWSDELETE},
        {DISPID_IHTMLDOCUMENT3_ONROWSINSERTED},
        {DISPID_IHTMLDOCUMENT3_ONCELLCHANGE},
        {DISPID_IHTMLDOCUMENT3_ONDATASETCHANGED},
        {DISPID_IHTMLDOCUMENT3_ONDATAAVAILABLE},
        {DISPID_IHTMLDOCUMENT3_ONDATASETCOMPLETE},
        {DISPID_IHTMLDOCUMENT3_ONPROPERTYCHANGE},
        {DISPID_IHTMLDOCUMENT3_ONBEFOREEDITFOCUS},

        /* IE9+ */
        {DISPID_IHTMLDOCUMENT3_RECALC},
        {DISPID_UNKNOWN}
    };
    const dispex_hook_t *const document3_ie9_hooks = document3_ie11_hooks + 10;
    static const dispex_hook_t document4_ie11_hooks[] = {
        {DISPID_IHTMLDOCUMENT4_CREATEEVENTOBJECT},
        {DISPID_IHTMLDOCUMENT4_FIREEVENT},
        {DISPID_IHTMLDOCUMENT4_ONCONTROLSELECT},

        /* IE10+ */
        {DISPID_IHTMLDOCUMENT4_NAMESPACES},

        /* IE9+ */
        {DISPID_IHTMLDOCUMENT4_CREATEDOCUMENTFROMURL},
        {DISPID_IHTMLDOCUMENT4_CREATERENDERSTYLE},
        {DISPID_UNKNOWN}
    };
    const dispex_hook_t *const document4_ie10_hooks = document4_ie11_hooks + 3;
    const dispex_hook_t *const document4_ie9_hooks  = document4_ie10_hooks + 1;
    static const dispex_hook_t document6_ie9_hooks[] = {
        {DISPID_IHTMLDOCUMENT6_ONSTORAGE},
        {DISPID_UNKNOWN}
    };

    HTMLDOMNode_init_dispex_info(info, mode);

    if(mode >= COMPAT_MODE_IE9) {
        dispex_info_add_interface(info, IHTMLDocument7_tid, NULL);
        dispex_info_add_interface(info, IDocumentEvent_tid, NULL);
    }

    /* Depending on compatibility version, we add interfaces in different order
     * so that the right getElementById implementation is used. */
    if(mode < COMPAT_MODE_IE8) {
        dispex_info_add_interface(info, IHTMLDocument3_tid, NULL);
        dispex_info_add_interface(info, IHTMLDocument6_tid, NULL);
    }else {
        dispex_info_add_interface(info, IHTMLDocument6_tid, mode >= COMPAT_MODE_IE9 ? document6_ie9_hooks : NULL);
        dispex_info_add_interface(info, IHTMLDocument3_tid, mode >= COMPAT_MODE_IE11 ? document3_ie11_hooks :
                                                            mode >= COMPAT_MODE_IE9  ? document3_ie9_hooks  : NULL);
    }
    dispex_info_add_interface(info, IHTMLDocument2_tid, mode >= COMPAT_MODE_IE11 ? document2_ie11_hooks :
                                                        mode >= COMPAT_MODE_IE10 ? document2_ie10_hooks :
                                                        mode >= COMPAT_MODE_IE9  ? document2_ie9_hooks  : document2_hooks);
    dispex_info_add_interface(info, IHTMLDocument4_tid, mode >= COMPAT_MODE_IE11 ? document4_ie11_hooks :
                                                        mode >= COMPAT_MODE_IE10 ? document4_ie10_hooks :
                                                        mode >= COMPAT_MODE_IE9  ? document4_ie9_hooks  : NULL);
}

dispex_static_data_t Document_dispex = {
    .id           = PROT_Document,
    .prototype_id = PROT_Node,
    .vtbl         = &HTMLDocument_event_target_vtbl.dispex_vtbl,
    .disp_tid     = DispHTMLDocument_tid,
    .iface_tids   = HTMLDocumentNode_iface_tids,
    .init_info    = HTMLDocumentNode_init_dispex_info,
};

dispex_static_data_t HTMLDocument_dispex = {
    .id           = PROT_HTMLDocument,
    .prototype_id = PROT_Document,
    .vtbl         = &HTMLDocument_event_target_vtbl.dispex_vtbl,
    .disp_tid     = DispHTMLDocument_tid,
    .iface_tids   = HTMLDocumentNode_iface_tids,
    .init_info    = HTMLDocumentNode_init_dispex_info,
    .min_compat_mode = COMPAT_MODE_IE11,
};

static HTMLDocumentNode *alloc_doc_node(HTMLDocumentObj *doc_obj, HTMLInnerWindow *window, HTMLInnerWindow *script_global)
{
    HTMLDocumentNode *doc;

    doc = calloc(1, sizeof(HTMLDocumentNode));
    if(!doc)
        return NULL;

    doc->IHTMLDocument2_iface.lpVtbl = &HTMLDocumentVtbl;
    doc->IHTMLDocument3_iface.lpVtbl = &HTMLDocument3Vtbl;
    doc->IHTMLDocument4_iface.lpVtbl = &HTMLDocument4Vtbl;
    doc->IHTMLDocument5_iface.lpVtbl = &HTMLDocument5Vtbl;
    doc->IHTMLDocument6_iface.lpVtbl = &HTMLDocument6Vtbl;
    doc->IHTMLDocument7_iface.lpVtbl = &HTMLDocument7Vtbl;
    doc->IDocumentSelector_iface.lpVtbl = &DocumentSelectorVtbl;
    doc->IDocumentEvent_iface.lpVtbl = &DocumentEventVtbl;
    doc->ISupportErrorInfo_iface.lpVtbl = &SupportErrorInfoVtbl;
    doc->IProvideMultipleClassInfo_iface.lpVtbl = &ProvideMultipleClassInfoVtbl;
    doc->IMarkupServices_iface.lpVtbl = &MarkupServicesVtbl;
    doc->IMarkupContainer_iface.lpVtbl = &MarkupContainerVtbl;
    doc->IDisplayServices_iface.lpVtbl = &DisplayServicesVtbl;
    doc->IDocumentRange_iface.lpVtbl = &DocumentRangeVtbl;

    doc->doc_obj = doc_obj;
    doc->window = window;

    if(window)
        IHTMLWindow2_AddRef(&window->base.IHTMLWindow2_iface);

    if(script_global) {
        doc->script_global = script_global;
        list_add_tail(&script_global->documents, &doc->script_global_entry);
    }

    ConnectionPointContainer_Init(&doc->cp_container, (IUnknown*)&doc->IHTMLDocument2_iface, HTMLDocumentNode_cpc);
    HTMLDocumentNode_Persist_Init(doc);
    HTMLDocumentNode_Service_Init(doc);
    HTMLDocumentNode_OleCmd_Init(doc);
    HTMLDocumentNode_OleObj_Init(doc);
    HTMLDocumentNode_SecMgr_Init(doc);

    list_init(&doc->selection_list);
    list_init(&doc->range_list);
    list_init(&doc->plugin_hosts);

    return doc;
}

HRESULT create_document_node(nsIDOMDocument *nsdoc, GeckoBrowser *browser, HTMLInnerWindow *window,
                             HTMLInnerWindow *script_global, compat_mode_t parent_mode, HTMLDocumentNode **ret)
{
    HTMLDocumentObj *doc_obj = browser->doc;
    HTMLDocumentNode *doc;

    doc = alloc_doc_node(doc_obj, window, script_global);
    if(!doc)
        return E_OUTOFMEMORY;

    if(parent_mode >= COMPAT_MODE_IE9) {
        TRACE("using parent mode %u\n", parent_mode);
        doc->document_mode = parent_mode;
        lock_document_mode(doc);
    }

    if(doc_obj && (!doc_obj->window || (window && is_main_content_window(window->base.outer_window))))
        doc->cp_container.forward_container = &doc_obj->cp_container;

    /* Share reference with HTMLDOMNode */
    if(NS_SUCCEEDED(nsIDOMDocument_QueryInterface(nsdoc, &IID_nsIDOMHTMLDocument, (void**)&doc->html_document))) {
        doc->dom_document = (nsIDOMDocument*)doc->html_document;
        nsIDOMHTMLDocument_Release(doc->html_document);
    }else {
        doc->dom_document = nsdoc;
        doc->html_document = NULL;
    }

    HTMLDOMNode_Init(doc, &doc->node, (nsIDOMNode*)doc->dom_document, &HTMLDocument_dispex);

    init_document_mutation(doc);
    doc_init_events(doc);

    doc->node.vtbl = &HTMLDocumentNodeImplVtbl;

    list_add_head(&browser->document_nodes, &doc->browser_entry);
    doc->browser = browser;

    if(browser->usermode == EDITMODE && doc->html_document) {
        nsAString mode_str;
        nsresult nsres;

        nsAString_InitDepend(&mode_str, L"on");
        nsres = nsIDOMHTMLDocument_SetDesignMode(doc->html_document, &mode_str);
        nsAString_Finish(&mode_str);
        if(NS_FAILED(nsres))
            ERR("SetDesignMode failed: %08lx\n", nsres);
    }

    *ret = doc;
    return S_OK;
}

static void DocumentFragment_init_dispex_info(dispex_data_t *info, compat_mode_t mode)
{
    static const DISPID document3_dispids[] = {
        DISPID_IHTMLDOCUMENT3_ATTACHEVENT,
        DISPID_IHTMLDOCUMENT3_DETACHEVENT,
        DISPID_UNKNOWN
    };

    if(mode < COMPAT_MODE_IE9) {
        HTMLDocumentNode_init_dispex_info(info, mode);
        dispex_info_add_interface(info, IHTMLDocument5_tid, NULL);
    } else if(mode < COMPAT_MODE_IE11) {
        dispex_info_add_dispids(info, IHTMLDocument3_tid, document3_dispids);
    }
}

static const tid_t DocumentFragment_iface_tids[] = {
    IHTMLDOMNode_tid,
    IHTMLDOMNode2_tid,
    IDocumentSelector_tid,
    0
};
dispex_static_data_t DocumentFragment_dispex = {
    .id           = PROT_DocumentFragment,
    .prototype_id = PROT_Node,
    .vtbl         = &HTMLDocument_event_target_vtbl.dispex_vtbl,
    .disp_tid     = DispHTMLDocument_tid,
    .iface_tids   = DocumentFragment_iface_tids,
    .init_info    = DocumentFragment_init_dispex_info,
};

static HRESULT create_document_fragment(nsIDOMNode *nsnode, HTMLDocumentNode *doc_node, HTMLDocumentNode **ret)
{
    HTMLDocumentNode *doc_frag;

    doc_frag = alloc_doc_node(doc_node->doc_obj, doc_node->window, doc_node->script_global);
    if(!doc_frag)
        return E_OUTOFMEMORY;

    HTMLDOMNode_Init(doc_node, &doc_frag->node, nsnode, &DocumentFragment_dispex);
    doc_frag->node.vtbl = &HTMLDocumentFragmentImplVtbl;
    doc_frag->document_mode = lock_document_mode(doc_node);
    dispex_compat_mode(&doc_frag->node.event_target.dispex); /* make sure the object is fully initialized */

    *ret = doc_frag;
    return S_OK;
}

HRESULT get_document_node(nsIDOMDocument *dom_document, HTMLDocumentNode **ret)
{
    HTMLDOMNode *node;
    HRESULT hres;

    hres = get_node((nsIDOMNode*)dom_document, FALSE, &node);
    if(FAILED(hres))
        return hres;

    if(!node) {
        ERR("document not initialized\n");
        return E_FAIL;
    }

    assert(node->vtbl == &HTMLDocumentNodeImplVtbl);
    *ret = impl_from_HTMLDOMNode(node);
    return S_OK;
}
