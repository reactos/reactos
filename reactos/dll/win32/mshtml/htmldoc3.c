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

#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define HTMLDOC3_THIS(iface) DEFINE_THIS(HTMLDocument, HTMLDocument3, iface)

static HRESULT WINAPI HTMLDocument3_QueryInterface(IHTMLDocument3 *iface,
                                                  REFIID riid, void **ppv)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    return IHTMLDocument2_QueryInterface(HTMLDOC(This), riid, ppv);
}

static ULONG WINAPI HTMLDocument3_AddRef(IHTMLDocument3 *iface)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    return IHTMLDocument2_AddRef(HTMLDOC(This));
}

static ULONG WINAPI HTMLDocument3_Release(IHTMLDocument3 *iface)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    return IHTMLDocument2_Release(HTMLDOC(This));
}

static HRESULT WINAPI HTMLDocument3_GetTypeInfoCount(IHTMLDocument3 *iface, UINT *pctinfo)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(This), pctinfo);
}

static HRESULT WINAPI HTMLDocument3_GetTypeInfo(IHTMLDocument3 *iface, UINT iTInfo,
                                                LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    return IDispatchEx_GetTypeInfo(DISPATCHEX(This), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLDocument3_GetIDsOfNames(IHTMLDocument3 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    return IDispatchEx_GetIDsOfNames(DISPATCHEX(This), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLDocument3_Invoke(IHTMLDocument3 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    return IDispatchEx_Invoke(DISPATCHEX(This), dispIdMember, riid, lcid, wFlags, pDispParams,
            pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLDocument3_releaseCapture(IHTMLDocument3 *iface)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_recalc(IHTMLDocument3 *iface, VARIANT_BOOL fForce)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->(%x)\n", This, fForce);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_createTextNode(IHTMLDocument3 *iface, BSTR text,
                                                   IHTMLDOMNode **newTextNode)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    nsIDOMText *nstext;
    HTMLDOMNode *node;
    nsAString text_str;
    nsresult nsres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(text), newTextNode);

    if(!This->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsAString_Init(&text_str, text);
    nsres = nsIDOMHTMLDocument_CreateTextNode(This->nsdoc, &text_str, &nstext);
    nsAString_Finish(&text_str);
    if(NS_FAILED(nsres)) {
        ERR("CreateTextNode failed: %08x\n", nsres);
        return E_FAIL;
    }

    node = HTMLDOMTextNode_Create(This, (nsIDOMNode*)nstext);
    nsIDOMElement_Release(nstext);

    *newTextNode = HTMLDOMNODE(node);
    IHTMLDOMNode_AddRef(HTMLDOMNODE(node));
    return S_OK;
}

static HRESULT WINAPI HTMLDocument3_get_documentElement(IHTMLDocument3 *iface, IHTMLElement **p)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    nsIDOMElement *nselem = NULL;
    HTMLDOMNode *node;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMHTMLDocument_GetDocumentElement(This->nsdoc, &nselem);
    if(NS_FAILED(nsres)) {
        ERR("GetDocumentElement failed: %08x\n", nsres);
        return E_FAIL;
    }

    if(nselem) {
        node = get_node(This, (nsIDOMNode *)nselem, TRUE);
        nsIDOMElement_Release(nselem);
        IHTMLDOMNode_QueryInterface(HTMLDOMNODE(node), &IID_IHTMLElement, (void**)p);
    }else {
        *p = NULL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument3_uniqueID(IHTMLDocument3 *iface, BSTR *p)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_attachEvent(IHTMLDocument3 *iface, BSTR event,
                                                IDispatch* pDisp, VARIANT_BOOL *pfResult)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->(%s %p %p)\n", This, debugstr_w(event), pDisp, pfResult);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_detachEvent(IHTMLDocument3 *iface, BSTR event,
                                                IDispatch *pDisp)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(event), pDisp);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_onrowsdelete(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_onrowsdelete(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_onrowsinserted(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_onrowsinserted(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_oncellchange(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_oncellchange(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_ondatasetchanged(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_ondatasetchanged(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_ondataavailable(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_ondataavailable(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_ondatasetcomplete(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_ondatasetcomplete(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_onpropertychange(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_onpropertychange(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_dir(IHTMLDocument3 *iface, BSTR v)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_dir(IHTMLDocument3 *iface, BSTR *p)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_oncontextmenu(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_oncontextmenu(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_onstop(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_onstop(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_createDocumentFragment(IHTMLDocument3 *iface,
                                                           IHTMLDocument2 **ppNewDoc)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppNewDoc);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_parentDocument(IHTMLDocument3 *iface,
                                                       IHTMLDocument2 **p)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_enableDownload(IHTMLDocument3 *iface,
                                                       VARIANT_BOOL v)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_enableDownload(IHTMLDocument3 *iface,
                                                       VARIANT_BOOL *p)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_baseUrl(IHTMLDocument3 *iface, BSTR v)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_baseUrl(IHTMLDocument3 *iface, BSTR *p)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_childNodes(IHTMLDocument3 *iface, IDispatch **p)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_inheritStyleSheets(IHTMLDocument3 *iface,
                                                           VARIANT_BOOL v)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_inheritStyleSheets(IHTMLDocument3 *iface,
                                                           VARIANT_BOOL *p)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_put_onbeforeeditfocus(IHTMLDocument3 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_get_onbeforeeditfocus(IHTMLDocument3 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument3_getElementsByName(IHTMLDocument3 *iface, BSTR v,
                                                      IHTMLElementCollection **ppelColl)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(v), ppelColl);
    return E_NOTIMPL;
}


static HRESULT WINAPI HTMLDocument3_getElementById(IHTMLDocument3 *iface, BSTR v,
                                                   IHTMLElement **pel)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    nsIDOMElement *nselem;
    HTMLDOMNode *node;
    nsAString id_str;
    nsresult nsres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(v), pel);

    if(!This->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsAString_Init(&id_str, v);
    nsres = nsIDOMHTMLDocument_GetElementById(This->nsdoc, &id_str, &nselem);
    nsAString_Finish(&id_str);
    if(FAILED(nsres)) {
        ERR("GetElementById failed: %08x\n", nsres);
        return E_FAIL;
    }

    if(nselem) {
        node = get_node(This, (nsIDOMNode*)nselem, TRUE);
        nsIDOMElement_Release(nselem);

        IHTMLDOMNode_QueryInterface(HTMLDOMNODE(node), &IID_IHTMLElement, (void**)pel);
    }else {
        *pel = NULL;
    }

    return S_OK;
}


static HRESULT WINAPI HTMLDocument3_getElementsByTagName(IHTMLDocument3 *iface, BSTR v,
                                                         IHTMLElementCollection **pelColl)
{
    HTMLDocument *This = HTMLDOC3_THIS(iface);
    nsIDOMNodeList *nslist;
    nsAString id_str, ns_str;
    nsresult nsres;
    static const WCHAR str[] = {'*',0};

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(v), pelColl);

    if(!This->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsAString_Init(&id_str, v);
    nsAString_Init(&ns_str, str);
    nsres = nsIDOMHTMLDocument_GetElementsByTagNameNS(This->nsdoc, &ns_str, &id_str, &nslist);
    nsAString_Finish(&id_str);
    nsAString_Finish(&ns_str);
    if(FAILED(nsres)) {
        ERR("GetElementByName failed: %08x\n", nsres);
        return E_FAIL;
    }

    *pelColl = (IHTMLElementCollection*)create_collection_from_nodelist(This, (IUnknown*)HTMLDOC3(This), nslist);
    nsIDOMNodeList_Release(nslist);

    return S_OK;
}

#undef HTMLDOC3_THIS

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

#define HTMLDOC4_THIS(iface) DEFINE_THIS(HTMLDocument, HTMLDocument4, iface)

static HRESULT WINAPI HTMLDocument4_QueryInterface(IHTMLDocument4 *iface,
                                                   REFIID riid, void **ppv)
{
    HTMLDocument *This = HTMLDOC4_THIS(iface);
    return IHTMLDocument2_QueryInterface(HTMLDOC(This), riid, ppv);
}

static ULONG WINAPI HTMLDocument4_AddRef(IHTMLDocument4 *iface)
{
    HTMLDocument *This = HTMLDOC4_THIS(iface);
    return IHTMLDocument2_AddRef(HTMLDOC(This));
}

static ULONG WINAPI HTMLDocument4_Release(IHTMLDocument4 *iface)
{
    HTMLDocument *This = HTMLDOC4_THIS(iface);
    return IHTMLDocument2_Release(HTMLDOC(This));
}

static HRESULT WINAPI HTMLDocument4_GetTypeInfoCount(IHTMLDocument4 *iface, UINT *pctinfo)
{
    HTMLDocument *This = HTMLDOC4_THIS(iface);
    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(This), pctinfo);
}

static HRESULT WINAPI HTMLDocument4_GetTypeInfo(IHTMLDocument4 *iface, UINT iTInfo,
                                                LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLDocument *This = HTMLDOC4_THIS(iface);
    return IDispatchEx_GetTypeInfo(DISPATCHEX(This), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLDocument4_GetIDsOfNames(IHTMLDocument4 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLDocument *This = HTMLDOC4_THIS(iface);
    return IDispatchEx_GetIDsOfNames(DISPATCHEX(This), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLDocument4_Invoke(IHTMLDocument4 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLDocument *This = HTMLDOC4_THIS(iface);
    return IDispatchEx_Invoke(DISPATCHEX(This), dispIdMember, riid, lcid, wFlags, pDispParams,
            pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLDocument4_focus(IHTMLDocument4 *iface)
{
    HTMLDocument *This = HTMLDOC4_THIS(iface);
    nsIDOMNSHTMLElement *nselem;
    nsIDOMHTMLElement *nsbody;
    nsresult nsres;

    TRACE("(%p)->()\n", This);

    nsres = nsIDOMHTMLDocument_GetBody(This->nsdoc, &nsbody);
    if(NS_FAILED(nsres) || !nsbody) {
        ERR("GetBody failed: %08x\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMHTMLElement_QueryInterface(nsbody, &IID_nsIDOMNSHTMLElement, (void**)&nselem);
    nsIDOMHTMLElement_Release(nsbody);
    if(NS_FAILED(nsres)) {
        ERR("Could not get nsIDOMNSHTMLElement: %08x\n", nsres);
        return E_FAIL;
    }

    nsres = nsIDOMNSHTMLElement_focus(nselem);
    nsIDOMNSHTMLElement_Release(nselem);
    if(NS_FAILED(nsres)) {
        ERR("Focus failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument4_hasFocus(IHTMLDocument4 *iface, VARIANT_BOOL *pfFocus)
{
    HTMLDocument *This = HTMLDOC4_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pfFocus);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_put_onselectionchange(IHTMLDocument4 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC4_THIS(iface);
    FIXME("(%p)->(v)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_get_onselectionchange(IHTMLDocument4 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC4_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_get_namespace(IHTMLDocument4 *iface, IDispatch **p)
{
    HTMLDocument *This = HTMLDOC4_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_createDocumentFromUrl(IHTMLDocument4 *iface, BSTR bstrUrl,
        BSTR bstrOptions, IHTMLDocument2 **newDoc)
{
    HTMLDocument *This = HTMLDOC4_THIS(iface);
    FIXME("(%p)->(%s %s %p)\n", This, debugstr_w(bstrUrl), debugstr_w(bstrOptions), newDoc);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_put_media(IHTMLDocument4 *iface, BSTR v)
{
    HTMLDocument *This = HTMLDOC4_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_get_media(IHTMLDocument4 *iface, BSTR *p)
{
    HTMLDocument *This = HTMLDOC4_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_createEventObject(IHTMLDocument4 *iface,
        VARIANT *pvarEventObject, IHTMLEventObj **ppEventObj)
{
    HTMLDocument *This = HTMLDOC4_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pvarEventObject, ppEventObj);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_fireEvent(IHTMLDocument4 *iface, BSTR bstrEventName,
        VARIANT *pvarEventObject, VARIANT_BOOL *pfCanceled)
{
    HTMLDocument *This = HTMLDOC4_THIS(iface);
    FIXME("(%p)->(%s %p %p)\n", This, debugstr_w(bstrEventName), pvarEventObject, pfCanceled);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_createRenderStyle(IHTMLDocument4 *iface, BSTR v,
        IHTMLRenderStyle **ppIHTMLRenderStyle)
{
    HTMLDocument *This = HTMLDOC4_THIS(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(v), ppIHTMLRenderStyle);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_put_oncontrolselect(IHTMLDocument4 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC4_THIS(iface);
    FIXME("(%p)->(v)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_get_oncontrolselect(IHTMLDocument4 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC4_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument4_get_URLEncoded(IHTMLDocument4 *iface, BSTR *p)
{
    HTMLDocument *This = HTMLDOC4_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

#undef HTMLDOC4_THIS

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
    This->lpHTMLDocument3Vtbl = &HTMLDocument3Vtbl;
    This->lpHTMLDocument4Vtbl = &HTMLDocument4Vtbl;
}
