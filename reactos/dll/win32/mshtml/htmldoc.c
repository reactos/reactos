/*
 * Copyright 2005 Jacek Caban
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
#include "perhist.h"

#include "wine/debug.h"

#include "mshtml_private.h"
#include "htmlevent.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define HTMLDOC_THIS(iface) DEFINE_THIS(HTMLDocument, HTMLDocument2, iface)

static HRESULT WINAPI HTMLDocument_QueryInterface(IHTMLDocument2 *iface, REFIID riid, void **ppvObject)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);

    *ppvObject = NULL;
    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown, %p)\n", This, ppvObject);
        *ppvObject = HTMLDOC(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch, %p)\n", This, ppvObject);
        *ppvObject = DISPATCHEX(This);
    }else if(IsEqualGUID(&IID_IDispatchEx, riid)) {
        TRACE("(%p)->(IID_IDispatchEx, %p)\n", This, ppvObject);
        *ppvObject = DISPATCHEX(This);
    }else if(IsEqualGUID(&IID_IHTMLDocument, riid)) {
        TRACE("(%p)->(IID_IHTMLDocument, %p)\n", This, ppvObject);
        *ppvObject = HTMLDOC(This);
    }else if(IsEqualGUID(&IID_IHTMLDocument2, riid)) {
        TRACE("(%p)->(IID_IHTMLDocument2, %p)\n", This, ppvObject);
        *ppvObject = HTMLDOC(This);
    }else if(IsEqualGUID(&IID_IHTMLDocument3, riid)) {
        TRACE("(%p)->(IID_IHTMLDocument3, %p)\n", This, ppvObject);
        *ppvObject = HTMLDOC3(This);
    }else if(IsEqualGUID(&IID_IHTMLDocument4, riid)) {
        TRACE("(%p)->(IID_IHTMLDocument4, %p)\n", This, ppvObject);
        *ppvObject = HTMLDOC4(This);
    }else if(IsEqualGUID(&IID_IHTMLDocument5, riid)) {
        TRACE("(%p)->(IID_IHTMLDocument5, %p)\n", This, ppvObject);
        *ppvObject = HTMLDOC5(This);
    }else if(IsEqualGUID(&IID_IPersist, riid)) {
        TRACE("(%p)->(IID_IPersist, %p)\n", This, ppvObject);
        *ppvObject = PERSIST(This);
    }else if(IsEqualGUID(&IID_IPersistMoniker, riid)) {
        TRACE("(%p)->(IID_IPersistMoniker, %p)\n", This, ppvObject);
        *ppvObject = PERSISTMON(This);
    }else if(IsEqualGUID(&IID_IPersistFile, riid)) {
        TRACE("(%p)->(IID_IPersistFile, %p)\n", This, ppvObject);
        *ppvObject = PERSISTFILE(This);
    }else if(IsEqualGUID(&IID_IMonikerProp, riid)) {
        TRACE("(%p)->(IID_IMonikerProp, %p)\n", This, ppvObject);
        *ppvObject = MONPROP(This);
    }else if(IsEqualGUID(&IID_IOleObject, riid)) {
        TRACE("(%p)->(IID_IOleObject, %p)\n", This, ppvObject);
        *ppvObject = OLEOBJ(This);
    }else if(IsEqualGUID(&IID_IOleDocument, riid)) {
        TRACE("(%p)->(IID_IOleDocument, %p)\n", This, ppvObject);
        *ppvObject = OLEDOC(This);
    }else if(IsEqualGUID(&IID_IOleDocumentView, riid)) {
        TRACE("(%p)->(IID_IOleDocumentView, %p)\n", This, ppvObject);
        *ppvObject = DOCVIEW(This);
    }else if(IsEqualGUID(&IID_IOleInPlaceActiveObject, riid)) {
        TRACE("(%p)->(IID_IOleInPlaceActiveObject, %p)\n", This, ppvObject);
        *ppvObject = ACTOBJ(This);
    }else if(IsEqualGUID(&IID_IViewObject, riid)) {
        TRACE("(%p)->(IID_IViewObject, %p)\n", This, ppvObject);
        *ppvObject = VIEWOBJ(This);
    }else if(IsEqualGUID(&IID_IViewObject2, riid)) {
        TRACE("(%p)->(IID_IViewObject2, %p)\n", This, ppvObject);
        *ppvObject = VIEWOBJ2(This);
    }else if(IsEqualGUID(&IID_IOleWindow, riid)) {
        TRACE("(%p)->(IID_IOleWindow, %p)\n", This, ppvObject);
        *ppvObject = OLEWIN(This);
    }else if(IsEqualGUID(&IID_IOleInPlaceObject, riid)) {
        TRACE("(%p)->(IID_IOleInPlaceObject, %p)\n", This, ppvObject);
        *ppvObject = INPLACEOBJ(This);
    }else if(IsEqualGUID(&IID_IOleInPlaceObjectWindowless, riid)) {
        TRACE("(%p)->(IID_IOleInPlaceObjectWindowless, %p)\n", This, ppvObject);
        *ppvObject = INPLACEWIN(This);
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        TRACE("(%p)->(IID_IServiceProvider, %p)\n", This, ppvObject);
        *ppvObject = SERVPROV(This);
    }else if(IsEqualGUID(&IID_IOleCommandTarget, riid)) {
        TRACE("(%p)->(IID_IOleCommandTarget, %p)\n", This, ppvObject);
        *ppvObject = CMDTARGET(This);
    }else if(IsEqualGUID(&IID_IOleControl, riid)) {
        TRACE("(%p)->(IID_IOleControl, %p)\n", This, ppvObject);
        *ppvObject = CONTROL(This);
    }else if(IsEqualGUID(&IID_IHlinkTarget, riid)) {
        TRACE("(%p)->(IID_IHlinkTarget, %p)\n", This, ppvObject);
        *ppvObject = HLNKTARGET(This);
    }else if(IsEqualGUID(&IID_IConnectionPointContainer, riid)) {
        TRACE("(%p)->(IID_IConnectionPointContainer %p)\n", This, ppvObject);
        *ppvObject = CONPTCONT(&This->cp_container);
    }else if(IsEqualGUID(&IID_IPersistStreamInit, riid)) {
        TRACE("(%p)->(IID_IPersistStreamInit %p)\n", This, ppvObject);
        *ppvObject = PERSTRINIT(This);
    }else if(IsEqualGUID(&IID_ICustomDoc, riid)) {
        TRACE("(%p)->(IID_ICustomDoc %p)\n", This, ppvObject);
        *ppvObject = CUSTOMDOC(This);
    }else if(IsEqualGUID(&DIID_DispHTMLDocument, riid)) {
        TRACE("(%p)->(DIID_DispHTMLDocument %p)\n", This, ppvObject);
        *ppvObject = HTMLDOC(This);
    }else if(IsEqualGUID(&IID_ISupportErrorInfo, riid)) {
        TRACE("(%p)->(IID_ISupportErrorInfo %p)\n", This, ppvObject);
        *ppvObject = SUPPERRINFO(This);
    }else if(IsEqualGUID(&IID_IPersistHistory, riid)) {
        TRACE("(%p)->(IID_IPersistHistory %p)\n", This, ppvObject);
        *ppvObject = PERSISTHIST(This);
    }else if(IsEqualGUID(&CLSID_CMarkup, riid)) {
        FIXME("(%p)->(CLSID_CMarkup %p)\n", This, ppvObject);
        return E_NOINTERFACE;
    }else if(IsEqualGUID(&IID_IRunnableObject, riid)) {
        TRACE("(%p)->(IID_IRunnableObject %p) returning NULL\n", This, ppvObject);
        return E_NOINTERFACE;
    }else if(IsEqualGUID(&IID_IPersistPropertyBag, riid)) {
        TRACE("(%p)->(IID_IPersistPropertyBag %p) returning NULL\n", This, ppvObject);
        return E_NOINTERFACE;
    }else if(IsEqualGUID(&IID_IMarshal, riid)) {
        TRACE("(%p)->(IID_IMarshal %p) returning NULL\n", This, ppvObject);
        return E_NOINTERFACE;
    }else if(dispex_query_interface(&This->dispex, riid, ppvObject)) {
        return *ppvObject ? S_OK : E_NOINTERFACE;
    }

    if(*ppvObject) {
        IHTMLDocument2_AddRef(iface);
        return S_OK;
    }

    FIXME("(%p)->(%s %p) interface not supported\n", This, debugstr_guid(riid), ppvObject);
    return E_NOINTERFACE;
}

static ULONG WINAPI HTMLDocument_AddRef(IHTMLDocument2 *iface)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref = %u\n", This, ref);
    return ref;
}

static ULONG WINAPI HTMLDocument_Release(IHTMLDocument2 *iface)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref = %u\n", This, ref);

    if(!ref) {
        remove_doc_tasks(This);
        release_script_hosts(This);

        if(This->client)
            IOleObject_SetClientSite(OLEOBJ(This), NULL);
        if(This->in_place_active)
            IOleInPlaceObjectWindowless_InPlaceDeactivate(INPLACEWIN(This));
        if(This->ipsite)
            IOleDocumentView_SetInPlaceSite(DOCVIEW(This), NULL);
        if(This->undomgr)
            IOleUndoManager_Release(This->undomgr);

        set_document_bscallback(This, NULL);
        set_current_mon(This, NULL);

        if(This->tooltips_hwnd)
            DestroyWindow(This->tooltips_hwnd);
        if(This->hwnd)
            DestroyWindow(This->hwnd);

        if(This->option_factory) {
            This->option_factory->doc = NULL;
            IHTMLOptionElementFactory_Release(HTMLOPTFACTORY(This->option_factory));
        }

        if(This->location)
            This->location->doc = NULL;

        if(This->window)
            IHTMLWindow2_Release(HTMLWINDOW2(This->window));

        if(This->event_target)
            release_event_target(This->event_target);

        heap_free(This->mime);
        detach_selection(This);
        detach_ranges(This);
        release_nodes(This);

        ConnectionPointContainer_Destroy(&This->cp_container);

        if(This->nsdoc) {
            remove_mutation_observer(This->nscontainer, This->nsdoc);
            nsIDOMHTMLDocument_Release(This->nsdoc);
        }
        if(This->nscontainer)
            NSContainer_Release(This->nscontainer);

        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI HTMLDocument_GetTypeInfoCount(IHTMLDocument2 *iface, UINT *pctinfo)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);

    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(This), pctinfo);
}

static HRESULT WINAPI HTMLDocument_GetTypeInfo(IHTMLDocument2 *iface, UINT iTInfo,
                                                LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);

    return IDispatchEx_GetTypeInfo(DISPATCHEX(This), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI HTMLDocument_GetIDsOfNames(IHTMLDocument2 *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);

    return IDispatchEx_GetIDsOfNames(DISPATCHEX(This), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI HTMLDocument_Invoke(IHTMLDocument2 *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);

    return IDispatchEx_Invoke(DISPATCHEX(This), dispIdMember, riid, lcid, wFlags, pDispParams,
            pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI HTMLDocument_get_Script(IHTMLDocument2 *iface, IDispatch **p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = (IDispatch*)HTMLWINDOW2(This->window);
    IDispatch_AddRef(*p);
    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_all(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    nsIDOMElement *nselem = NULL;
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
        *p = create_all_collection(get_node(This, (nsIDOMNode*)nselem, TRUE), TRUE);
        nsIDOMElement_Release(nselem);
    }else {
        *p = NULL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_body(IHTMLDocument2 *iface, IHTMLElement **p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    nsIDOMHTMLElement *nsbody = NULL;
    HTMLDOMNode *node;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMHTMLDocument_GetBody(This->nsdoc, &nsbody);
    if(NS_FAILED(nsres)) {
        TRACE("Could not get body: %08x\n", nsres);
        return E_UNEXPECTED;
    }

    if(nsbody) {
        node = get_node(This, (nsIDOMNode*)nsbody, TRUE);
        nsIDOMHTMLElement_Release(nsbody);

        IHTMLDOMNode_QueryInterface(HTMLDOMNODE(node), &IID_IHTMLElement, (void**)p);
    }else {
        *p = NULL;
    }

    TRACE("*p = %p\n", *p);
    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_activeElement(IHTMLDocument2 *iface, IHTMLElement **p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_images(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    nsIDOMHTMLCollection *nscoll = NULL;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_INVALIDARG;

    *p = NULL;

    if(!This->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMHTMLDocument_GetImages(This->nsdoc, &nscoll);
    if(NS_FAILED(nsres)) {
        ERR("GetImages failed: %08x\n", nsres);
        return E_FAIL;
    }

    if(nscoll) {
        *p = create_collection_from_htmlcol(This, (IUnknown*)HTMLDOC(This), nscoll);
        nsIDOMElement_Release(nscoll);
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_applets(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    nsIDOMHTMLCollection *nscoll = NULL;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_INVALIDARG;

    *p = NULL;

    if(!This->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMHTMLDocument_GetApplets(This->nsdoc, &nscoll);
    if(NS_FAILED(nsres)) {
        ERR("GetApplets failed: %08x\n", nsres);
        return E_FAIL;
    }

    if(nscoll) {
        *p = create_collection_from_htmlcol(This, (IUnknown*)HTMLDOC(This), nscoll);
        nsIDOMElement_Release(nscoll);
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_links(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    nsIDOMHTMLCollection *nscoll = NULL;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_INVALIDARG;

    *p = NULL;

    if(!This->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMHTMLDocument_GetLinks(This->nsdoc, &nscoll);
    if(NS_FAILED(nsres)) {
        ERR("GetLinks failed: %08x\n", nsres);
        return E_FAIL;
    }

    if(nscoll) {
        *p = create_collection_from_htmlcol(This, (IUnknown*)HTMLDOC(This), nscoll);
        nsIDOMElement_Release(nscoll);
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_forms(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    nsIDOMHTMLCollection *nscoll = NULL;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_INVALIDARG;

    *p = NULL;

    if(!This->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMHTMLDocument_GetForms(This->nsdoc, &nscoll);
    if(NS_FAILED(nsres)) {
        ERR("GetForms failed: %08x\n", nsres);
        return E_FAIL;
    }

    if(nscoll) {
        *p = create_collection_from_htmlcol(This, (IUnknown*)HTMLDOC(This), nscoll);
        nsIDOMElement_Release(nscoll);
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_anchors(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    nsIDOMHTMLCollection *nscoll = NULL;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!p)
        return E_INVALIDARG;

    *p = NULL;

    if(!This->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsres = nsIDOMHTMLDocument_GetAnchors(This->nsdoc, &nscoll);
    if(NS_FAILED(nsres)) {
        ERR("GetAnchors failed: %08x\n", nsres);
        return E_FAIL;
    }

    if(nscoll) {
        *p = create_collection_from_htmlcol(This, (IUnknown*)HTMLDOC(This), nscoll);
        nsIDOMElement_Release(nscoll);
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_put_title(IHTMLDocument2 *iface, BSTR v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(!This->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsAString_Init(&nsstr, v);
    nsres = nsIDOMHTMLDocument_SetTitle(This->nsdoc, &nsstr);
    nsAString_Finish(&nsstr);
    if(NS_FAILED(nsres))
        ERR("SetTitle failed: %08x\n", nsres);

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_title(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    const PRUnichar *ret;
    nsAString nsstr;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }


    nsAString_Init(&nsstr, NULL);
    nsres = nsIDOMHTMLDocument_GetTitle(This->nsdoc, &nsstr);
    if (NS_SUCCEEDED(nsres)) {
        nsAString_GetData(&nsstr, &ret);
        *p = SysAllocString(ret);
    }
    nsAString_Finish(&nsstr);

    if(NS_FAILED(nsres)) {
        ERR("GetTitle failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_scripts(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_designMode(IHTMLDocument2 *iface, BSTR v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_designMode(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    static WCHAR szOff[] = {'O','f','f',0};
    FIXME("(%p)->(%p) always returning Off\n", This, p);

    if(!p)
        return E_INVALIDARG;

    *p = SysAllocString(szOff);

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_selection(IHTMLDocument2 *iface, IHTMLSelectionObject **p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    nsISelection *nsselection = NULL;

    TRACE("(%p)->(%p)\n", This, p);

    if(This->nscontainer) {
        nsIDOMWindow *dom_window = NULL;

        nsIWebBrowser_GetContentDOMWindow(This->nscontainer->webbrowser, &dom_window);
        if(dom_window) {
            nsIDOMWindow_GetSelection(dom_window, &nsselection);
            nsIDOMWindow_Release(dom_window);
        }
    }

    *p = HTMLSelectionObject_Create(This, nsselection);
    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_readyState(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);

    static const WCHAR wszUninitialized[] = {'u','n','i','n','i','t','i','a','l','i','z','e','d',0};
    static const WCHAR wszLoading[] = {'l','o','a','d','i','n','g',0};
    static const WCHAR wszLoaded[] = {'l','o','a','d','e','d',0};
    static const WCHAR wszInteractive[] = {'i','n','t','e','r','a','c','t','i','v','e',0};
    static const WCHAR wszComplete[] = {'c','o','m','p','l','e','t','e',0};

    static const LPCWSTR readystate_str[] = {
        wszUninitialized,
        wszLoading,
        wszLoaded,
        wszInteractive,
        wszComplete
    };

    TRACE("(%p)->(%p)\n", iface, p);

    if(!p)
        return E_POINTER;

    *p = SysAllocString(readystate_str[This->readystate]);
    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_frames(IHTMLDocument2 *iface, IHTMLFramesCollection2 **p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_embeds(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_plugins(IHTMLDocument2 *iface, IHTMLElementCollection **p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_alinkColor(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_alinkColor(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_bgColor(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_bgColor(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_fgColor(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_fgColor(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_linkColor(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_linkColor(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_vlinkColor(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_vlinkColor(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_referrer(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_location(IHTMLDocument2 *iface, IHTMLLocation **p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->location)
        IHTMLLocation_AddRef(HTMLLOCATION(This->location));
    else
        This->location = HTMLLocation_Create(This);

    *p = HTMLLOCATION(This->location);
    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_lastModified(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_URL(IHTMLDocument2 *iface, BSTR v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_URL(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);

    static const WCHAR about_blank_url[] =
        {'a','b','o','u','t',':','b','l','a','n','k',0};

    TRACE("(%p)->(%p)\n", iface, p);

    *p = SysAllocString(This->url ? This->url : about_blank_url);
    return S_OK;
}

static HRESULT WINAPI HTMLDocument_put_domain(IHTMLDocument2 *iface, BSTR v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_domain(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_cookie(IHTMLDocument2 *iface, BSTR v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_cookie(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_expando(IHTMLDocument2 *iface, VARIANT_BOOL v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%x)\n", This, v);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_expando(IHTMLDocument2 *iface, VARIANT_BOOL *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_charset(IHTMLDocument2 *iface, BSTR v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_charset(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_defaultCharset(IHTMLDocument2 *iface, BSTR v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(v));
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_defaultCharset(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_mimeType(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_fileSize(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_fileCreatedDate(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_fileModifiedDate(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_fileUpdatedDate(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_security(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_protocol(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_nameProp(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_write(IHTMLDocument2 *iface, SAFEARRAY *psarray)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    nsAString nsstr;
    VARIANT *var;
    ULONG i;
    nsresult nsres;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", iface, psarray);

    if(!This->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    if(psarray->cDims != 1) {
        FIXME("cDims=%d\n", psarray->cDims);
        return E_INVALIDARG;
    }

    hres = SafeArrayAccessData(psarray, (void**)&var);
    if(FAILED(hres)) {
        WARN("SafeArrayAccessData failed: %08x\n", hres);
        return hres;
    }

    nsAString_Init(&nsstr, NULL);

    for(i=0; i < psarray->rgsabound[0].cElements; i++) {
        if(V_VT(var+i) == VT_BSTR) {
            nsAString_SetData(&nsstr, V_BSTR(var+i));
            nsres = nsIDOMHTMLDocument_Write(This->nsdoc, &nsstr);
            if(NS_FAILED(nsres))
                ERR("Write failed: %08x\n", nsres);
        }else {
            FIXME("vt=%d\n", V_VT(var+i));
        }
    }

    nsAString_Finish(&nsstr);
    SafeArrayUnaccessData(psarray);

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_writeln(IHTMLDocument2 *iface, SAFEARRAY *psarray)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, psarray);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_open(IHTMLDocument2 *iface, BSTR url, VARIANT name,
                        VARIANT features, VARIANT replace, IDispatch **pomWindowResult)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    nsresult nsres;

    static const WCHAR text_htmlW[] = {'t','e','x','t','/','h','t','m','l',0};

    TRACE("(%p)->(%s %s %s %s %p)\n", This, debugstr_w(url), debugstr_variant(&name),
          debugstr_variant(&features), debugstr_variant(&replace), pomWindowResult);

    if(!This->nsdoc) {
        ERR("!nsdoc\n");
        return E_NOTIMPL;
    }

    if(!url || strcmpW(url, text_htmlW) || V_VT(&name) != VT_ERROR
       || V_VT(&features) != VT_ERROR || V_VT(&replace) != VT_ERROR)
        FIXME("unsupported args\n");

    nsres = nsIDOMHTMLDocument_Open(This->nsdoc);
    if(NS_FAILED(nsres)) {
        ERR("Open failed: %08x\n", nsres);
        return E_FAIL;
    }

    *pomWindowResult = (IDispatch*)HTMLWINDOW2(This->window);
    IHTMLWindow2_AddRef(HTMLWINDOW2(This->window));
    return S_OK;
}

static HRESULT WINAPI HTMLDocument_close(IHTMLDocument2 *iface)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    nsresult nsres;

    TRACE("(%p)\n", This);

    if(!This->nsdoc) {
        ERR("!nsdoc\n");
        return E_NOTIMPL;
    }

    nsres = nsIDOMHTMLDocument_Close(This->nsdoc);
    if(NS_FAILED(nsres)) {
        ERR("Close failed: %08x\n", nsres);
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_clear(IHTMLDocument2 *iface)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_queryCommandSupported(IHTMLDocument2 *iface, BSTR cmdID,
                                                        VARIANT_BOOL *pfRet)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_queryCommandEnabled(IHTMLDocument2 *iface, BSTR cmdID,
                                                        VARIANT_BOOL *pfRet)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_queryCommandState(IHTMLDocument2 *iface, BSTR cmdID,
                                                        VARIANT_BOOL *pfRet)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_queryCommandIndeterm(IHTMLDocument2 *iface, BSTR cmdID,
                                                        VARIANT_BOOL *pfRet)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_queryCommandText(IHTMLDocument2 *iface, BSTR cmdID,
                                                        BSTR *pfRet)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_queryCommandValue(IHTMLDocument2 *iface, BSTR cmdID,
                                                        VARIANT *pfRet)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_execCommand(IHTMLDocument2 *iface, BSTR cmdID,
                                VARIANT_BOOL showUI, VARIANT value, VARIANT_BOOL *pfRet)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%s %x %p)\n", This, debugstr_w(cmdID), showUI, pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_execCommandShowHelp(IHTMLDocument2 *iface, BSTR cmdID,
                                                        VARIANT_BOOL *pfRet)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(cmdID), pfRet);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_createElement(IHTMLDocument2 *iface, BSTR eTag,
                                                 IHTMLElement **newElem)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    nsIDOMElement *nselem;
    HTMLElement *elem;
    nsAString tag_str;
    nsresult nsres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(eTag), newElem);

    if(!This->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsAString_Init(&tag_str, eTag);
    nsres = nsIDOMDocument_CreateElement(This->nsdoc, &tag_str, &nselem);
    nsAString_Finish(&tag_str);
    if(NS_FAILED(nsres)) {
        ERR("CreateElement failed: %08x\n", nsres);
        return E_FAIL;
    }

    elem = HTMLElement_Create(This, (nsIDOMNode*)nselem, TRUE);
    nsIDOMElement_Release(nselem);

    *newElem = HTMLELEM(elem);
    IHTMLElement_AddRef(HTMLELEM(elem));
    return S_OK;
}

static HRESULT WINAPI HTMLDocument_put_onhelp(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_onhelp(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_onclick(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_onclick(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_ondblclick(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_ondblclick(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_onkeyup(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_KEYUP, &v);
}

static HRESULT WINAPI HTMLDocument_get_onkeyup(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_KEYUP, p);
}

static HRESULT WINAPI HTMLDocument_put_onkeydown(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_KEYDOWN, &v);
}

static HRESULT WINAPI HTMLDocument_get_onkeydown(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_KEYDOWN, p);
}

static HRESULT WINAPI HTMLDocument_put_onkeypress(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_onkeypress(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_onmouseup(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_onmouseup(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_onmousedown(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_onmousedown(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_onmousemove(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_onmousemove(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_onmouseout(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_onmouseout(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_onmouseover(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);

    TRACE("(%p)\n", This);

    return set_doc_event(This, EVENTID_MOUSEOVER, &v);
}

static HRESULT WINAPI HTMLDocument_get_onmouseover(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_MOUSEOVER, p);
}

static HRESULT WINAPI HTMLDocument_put_onreadystatechange(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_onreadystatechange(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_onafterupdate(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_onafterupdate(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_onrowexit(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_onrowexit(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_onrowenter(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_onrowenter(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_ondragstart(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_DRAGSTART, &v);
}

static HRESULT WINAPI HTMLDocument_get_ondragstart(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_DRAGSTART, p);
}

static HRESULT WINAPI HTMLDocument_put_onselectstart(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(&v));

    return set_doc_event(This, EVENTID_SELECTSTART, &v);
}

static HRESULT WINAPI HTMLDocument_get_onselectstart(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return get_doc_event(This, EVENTID_SELECTSTART, p);
}

static HRESULT WINAPI HTMLDocument_elementFromPoint(IHTMLDocument2 *iface, LONG x, LONG y,
                                                        IHTMLElement **elementHit)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%d %d %p)\n", This, x, y, elementHit);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_parentWindow(IHTMLDocument2 *iface, IHTMLWindow2 **p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);

    TRACE("(%p)->(%p)\n", This, p);

    *p = HTMLWINDOW2(This->window);
    IHTMLWindow2_AddRef(*p);
    return S_OK;
}

static HRESULT WINAPI HTMLDocument_get_styleSheets(IHTMLDocument2 *iface,
                                                   IHTMLStyleSheetsCollection **p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    nsIDOMStyleSheetList *nsstylelist;
    nsIDOMDocumentStyle *nsdocstyle;
    nsresult nsres;

    TRACE("(%p)->(%p)\n", This, p);

    *p = NULL;

    if(!This->nsdoc) {
        WARN("NULL nsdoc\n");
        return E_UNEXPECTED;
    }

    nsIDOMHTMLDocument_QueryInterface(This->nsdoc, &IID_nsIDOMDocumentStyle, (void**)&nsdocstyle);
    nsres = nsIDOMDocumentStyle_GetStyleSheets(nsdocstyle, &nsstylelist);
    nsIDOMDocumentStyle_Release(nsdocstyle);
    if(NS_FAILED(nsres)) {
        ERR("GetStyleSheets failed: %08x\n", nsres);
        return E_FAIL;
    }

    *p = HTMLStyleSheetsCollection_Create(nsstylelist);
    nsIDOMDocumentStyle_Release(nsstylelist);

    return S_OK;
}

static HRESULT WINAPI HTMLDocument_put_onbeforeupdate(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_onbeforeupdate(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_put_onerrorupdate(IHTMLDocument2 *iface, VARIANT v)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_get_onerrorupdate(IHTMLDocument2 *iface, VARIANT *p)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_toString(IHTMLDocument2 *iface, BSTR *String)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, String);
    return E_NOTIMPL;
}

static HRESULT WINAPI HTMLDocument_createStyleSheet(IHTMLDocument2 *iface, BSTR bstrHref,
                                            LONG lIndex, IHTMLStyleSheet **ppnewStyleSheet)
{
    HTMLDocument *This = HTMLDOC_THIS(iface);

    FIXME("(%p)->(%s %d %p) semi-stub\n", This, debugstr_w(bstrHref), lIndex, ppnewStyleSheet);

    *ppnewStyleSheet = HTMLStyleSheet_Create(NULL);
    return S_OK;
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

#define SUPPINFO_THIS(iface) DEFINE_THIS(HTMLDocument, SupportErrorInfo, iface)

static HRESULT WINAPI SupportErrorInfo_QueryInterface(ISupportErrorInfo *iface, REFIID riid, void **ppv)
{
    HTMLDocument *This = SUPPINFO_THIS(iface);
    return IHTMLDocument_QueryInterface(HTMLDOC(This), riid, ppv);
}

static ULONG WINAPI SupportErrorInfo_AddRef(ISupportErrorInfo *iface)
{
    HTMLDocument *This = SUPPINFO_THIS(iface);
    return IHTMLDocument_AddRef(HTMLDOC(This));
}

static ULONG WINAPI SupportErrorInfo_Release(ISupportErrorInfo *iface)
{
    HTMLDocument *This = SUPPINFO_THIS(iface);
    return IHTMLDocument_Release(HTMLDOC(This));
}

static HRESULT WINAPI SupportErrorInfo_InterfaceSupportsErrorInfo(ISupportErrorInfo *iface, REFIID riid)
{
    FIXME("(%p)->(%s)\n", iface, debugstr_guid(riid));
    return S_FALSE;
}

static const ISupportErrorInfoVtbl SupportErrorInfoVtbl = {
    SupportErrorInfo_QueryInterface,
    SupportErrorInfo_AddRef,
    SupportErrorInfo_Release,
    SupportErrorInfo_InterfaceSupportsErrorInfo
};

#define DISPEX_THIS(iface) DEFINE_THIS(HTMLDocument, IDispatchEx, iface)

static HRESULT WINAPI DocDispatchEx_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    HTMLDocument *This = DISPEX_THIS(iface);

    return IHTMLDocument2_QueryInterface(HTMLDOC(This), riid, ppv);
}

static ULONG WINAPI DocDispatchEx_AddRef(IDispatchEx *iface)
{
    HTMLDocument *This = DISPEX_THIS(iface);

    return IHTMLDocument2_AddRef(HTMLDOC(This));
}

static ULONG WINAPI DocDispatchEx_Release(IDispatchEx *iface)
{
    HTMLDocument *This = DISPEX_THIS(iface);

    return IHTMLDocument2_Release(HTMLDOC(This));
}

static HRESULT WINAPI DocDispatchEx_GetTypeInfoCount(IDispatchEx *iface, UINT *pctinfo)
{
    HTMLDocument *This = DISPEX_THIS(iface);

    return IDispatchEx_GetTypeInfoCount(DISPATCHEX(&This->dispex), pctinfo);
}

static HRESULT WINAPI DocDispatchEx_GetTypeInfo(IDispatchEx *iface, UINT iTInfo,
                                               LCID lcid, ITypeInfo **ppTInfo)
{
    HTMLDocument *This = DISPEX_THIS(iface);

    return IDispatchEx_GetTypeInfo(DISPATCHEX(&This->dispex), iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI DocDispatchEx_GetIDsOfNames(IDispatchEx *iface, REFIID riid,
                                                 LPOLESTR *rgszNames, UINT cNames,
                                                 LCID lcid, DISPID *rgDispId)
{
    HTMLDocument *This = DISPEX_THIS(iface);

    return IDispatchEx_GetIDsOfNames(DISPATCHEX(&This->dispex), riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI DocDispatchEx_Invoke(IDispatchEx *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HTMLDocument *This = DISPEX_THIS(iface);

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    switch(dispIdMember) {
    case DISPID_READYSTATE:
        TRACE("DISPID_READYSTATE\n");

        if(!(wFlags & DISPATCH_PROPERTYGET))
            return E_INVALIDARG;

        V_VT(pVarResult) = VT_I4;
        V_I4(pVarResult) = This->readystate;
        return S_OK;
    }

    return IDispatchEx_Invoke(DISPATCHEX(&This->dispex), dispIdMember, riid, lcid, wFlags, pDispParams,
                              pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI DocDispatchEx_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HTMLDocument *This = DISPEX_THIS(iface);

    return IDispatchEx_GetDispID(DISPATCHEX(&This->dispex), bstrName, grfdex, pid);
}

static HRESULT WINAPI DocDispatchEx_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    HTMLDocument *This = DISPEX_THIS(iface);

    return IDispatchEx_InvokeEx(DISPATCHEX(&This->dispex), id, lcid, wFlags, pdp, pvarRes, pei, pspCaller);
}

static HRESULT WINAPI DocDispatchEx_DeleteMemberByName(IDispatchEx *iface, BSTR bstrName, DWORD grfdex)
{
    HTMLDocument *This = DISPEX_THIS(iface);

    return IDispatchEx_DeleteMemberByName(DISPATCHEX(&This->dispex), bstrName, grfdex);
}

static HRESULT WINAPI DocDispatchEx_DeleteMemberByDispID(IDispatchEx *iface, DISPID id)
{
    HTMLDocument *This = DISPEX_THIS(iface);

    return IDispatchEx_DeleteMemberByDispID(DISPATCHEX(&This->dispex), id);
}

static HRESULT WINAPI DocDispatchEx_GetMemberProperties(IDispatchEx *iface, DISPID id, DWORD grfdexFetch, DWORD *pgrfdex)
{
    HTMLDocument *This = DISPEX_THIS(iface);

    return IDispatchEx_GetMemberProperties(DISPATCHEX(&This->dispex), id, grfdexFetch, pgrfdex);
}

static HRESULT WINAPI DocDispatchEx_GetMemberName(IDispatchEx *iface, DISPID id, BSTR *pbstrName)
{
    HTMLDocument *This = DISPEX_THIS(iface);

    return IDispatchEx_GetMemberName(DISPATCHEX(&This->dispex), id, pbstrName);
}

static HRESULT WINAPI DocDispatchEx_GetNextDispID(IDispatchEx *iface, DWORD grfdex, DISPID id, DISPID *pid)
{
    HTMLDocument *This = DISPEX_THIS(iface);

    return IDispatchEx_GetNextDispID(DISPATCHEX(&This->dispex), grfdex, id, pid);
}

static HRESULT WINAPI DocDispatchEx_GetNameSpaceParent(IDispatchEx *iface, IUnknown **ppunk)
{
    HTMLDocument *This = DISPEX_THIS(iface);

    return IDispatchEx_GetNameSpaceParent(DISPATCHEX(&This->dispex), ppunk);
}

#undef DISPEX_THIS

static const IDispatchExVtbl DocDispatchExVtbl = {
    DocDispatchEx_QueryInterface,
    DocDispatchEx_AddRef,
    DocDispatchEx_Release,
    DocDispatchEx_GetTypeInfoCount,
    DocDispatchEx_GetTypeInfo,
    DocDispatchEx_GetIDsOfNames,
    DocDispatchEx_Invoke,
    DocDispatchEx_GetDispID,
    DocDispatchEx_InvokeEx,
    DocDispatchEx_DeleteMemberByName,
    DocDispatchEx_DeleteMemberByDispID,
    DocDispatchEx_GetMemberProperties,
    DocDispatchEx_GetMemberName,
    DocDispatchEx_GetNextDispID,
    DocDispatchEx_GetNameSpaceParent
};

static const tid_t HTMLDocument_iface_tids[] = {
    IHTMLDocument2_tid,
    IHTMLDocument3_tid,
    IHTMLDocument4_tid,
    IHTMLDocument5_tid,
    0
};
static dispex_static_data_t HTMLDocument_dispex = {
    NULL,
    DispHTMLDocument_tid,
    NULL,
    HTMLDocument_iface_tids
};

static HRESULT alloc_doc(HTMLDocument **ret)
{
    HTMLDocument *doc;

    doc = heap_alloc_zero(sizeof(HTMLDocument));
    doc->lpHTMLDocument2Vtbl = &HTMLDocumentVtbl;
    doc->lpIDispatchExVtbl = &DocDispatchExVtbl;
    doc->lpSupportErrorInfoVtbl = &SupportErrorInfoVtbl;
    doc->ref = 1;
    doc->readystate = READYSTATE_UNINITIALIZED;
    doc->scriptmode = SCRIPTMODE_GECKO;

    list_init(&doc->bindings);
    list_init(&doc->script_hosts);
    list_init(&doc->selection_list);
    list_init(&doc->range_list);

    HTMLDocument_HTMLDocument3_Init(doc);
    HTMLDocument_HTMLDocument5_Init(doc);
    HTMLDocument_Persist_Init(doc);
    HTMLDocument_OleCmd_Init(doc);
    HTMLDocument_OleObj_Init(doc);
    HTMLDocument_View_Init(doc);
    HTMLDocument_Window_Init(doc);
    HTMLDocument_Service_Init(doc);
    HTMLDocument_Hlink_Init(doc);

    ConnectionPointContainer_Init(&doc->cp_container, (IUnknown*)HTMLDOC(doc));
    ConnectionPoint_Init(&doc->cp_propnotif, &doc->cp_container, &IID_IPropertyNotifySink);
    ConnectionPoint_Init(&doc->cp_htmldocevents, &doc->cp_container, &DIID_HTMLDocumentEvents);
    ConnectionPoint_Init(&doc->cp_htmldocevents2, &doc->cp_container, &DIID_HTMLDocumentEvents2);

    init_dispex(&doc->dispex, (IUnknown*)HTMLDOC(doc), &HTMLDocument_dispex);

    *ret = doc;
    return S_OK;
}

HRESULT create_doc_from_nsdoc(nsIDOMHTMLDocument *nsdoc, HTMLDocument **ret)
{
    HTMLDocument *doc;
    HRESULT hres;

    hres = alloc_doc(&doc);
    if(FAILED(hres))
        return hres;

    nsIDOMHTMLDocument_AddRef(nsdoc);
    doc->nsdoc = nsdoc;

    hres = HTMLWindow_Create(doc, NULL, &doc->window);
    if(FAILED(hres)) {
        IHTMLDocument_Release(HTMLDOC(doc));
        return hres;
    }

    *ret = doc;
    return S_OK;
}

HRESULT HTMLDocument_Create(IUnknown *pUnkOuter, REFIID riid, void** ppvObject)
{
    HTMLDocument *doc;
    nsIDOMWindow *nswindow = NULL;
    HRESULT hres;

    TRACE("(%p %s %p)\n", pUnkOuter, debugstr_guid(riid), ppvObject);

    hres = alloc_doc(&doc);
    if(FAILED(hres))
        return hres;

    hres = IHTMLDocument_QueryInterface(HTMLDOC(doc), riid, ppvObject);
    IHTMLDocument_Release(HTMLDOC(doc));
    if(FAILED(hres))
        return hres;

    doc->nscontainer = NSContainer_Create(doc, NULL);
    update_nsdocument(doc);

    if(doc->nscontainer) {
        nsresult nsres;

        nsres = nsIWebBrowser_GetContentDOMWindow(doc->nscontainer->webbrowser, &nswindow);
        if(NS_FAILED(nsres))
            ERR("GetContentDOMWindow failed: %08x\n", nsres);
    }

    hres = HTMLWindow_Create(doc, nswindow, &doc->window);
    if(nswindow)
        nsIDOMWindow_Release(nswindow);
    if(FAILED(hres)) {
        IHTMLDocument_Release(HTMLDOC(doc));
        return hres;
    }

    get_thread_hwnd();

    return S_OK;
}
