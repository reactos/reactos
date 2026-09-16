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

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "shlguid.h"
#include "shdeprecated.h"
#include "mscoree.h"
#include "mshtmdid.h"
#include "idispids.h"

#include "wine/debug.h"

#include "mshtml_private.h"
#include "htmlevent.h"
#include "binding.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define DOCHOST_DOCCANNAVIGATE  0

typedef struct {
    IEnumUnknown IEnumUnknown_iface;
    LONG ref;
} EnumUnknown;

static inline EnumUnknown *impl_from_IEnumUnknown(IEnumUnknown *iface)
{
    return CONTAINING_RECORD(iface, EnumUnknown, IEnumUnknown_iface);
}

static HRESULT WINAPI EnumUnknown_QueryInterface(IEnumUnknown *iface, REFIID riid, void **ppv)
{
    EnumUnknown *This = impl_from_IEnumUnknown(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IEnumUnknown_iface;
    }else if(IsEqualGUID(&IID_IEnumUnknown, riid)) {
        TRACE("(%p)->(IID_IEnumUnknown %p)\n", This, ppv);
        *ppv = &This->IEnumUnknown_iface;
    }else {
        WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI EnumUnknown_AddRef(IEnumUnknown *iface)
{
    EnumUnknown *This = impl_from_IEnumUnknown(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI EnumUnknown_Release(IEnumUnknown *iface)
{
    EnumUnknown *This = impl_from_IEnumUnknown(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref)
        free(This);

    return ref;
}

static HRESULT WINAPI EnumUnknown_Next(IEnumUnknown *iface, ULONG celt, IUnknown **rgelt, ULONG *pceltFetched)
{
    EnumUnknown *This = impl_from_IEnumUnknown(iface);

    TRACE("(%p)->(%lu %p %p)\n", This, celt, rgelt, pceltFetched);

    /* FIXME: It's not clear if we should ever return something here */
    if(pceltFetched)
        *pceltFetched = 0;
    return S_FALSE;
}

static HRESULT WINAPI EnumUnknown_Skip(IEnumUnknown *iface, ULONG celt)
{
    EnumUnknown *This = impl_from_IEnumUnknown(iface);
    FIXME("(%p)->(%lu)\n", This, celt);
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumUnknown_Reset(IEnumUnknown *iface)
{
    EnumUnknown *This = impl_from_IEnumUnknown(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI EnumUnknown_Clone(IEnumUnknown *iface, IEnumUnknown **ppenum)
{
    EnumUnknown *This = impl_from_IEnumUnknown(iface);
    FIXME("(%p)->(%p)\n", This, ppenum);
    return E_NOTIMPL;
}

static const IEnumUnknownVtbl EnumUnknownVtbl = {
    EnumUnknown_QueryInterface,
    EnumUnknown_AddRef,
    EnumUnknown_Release,
    EnumUnknown_Next,
    EnumUnknown_Skip,
    EnumUnknown_Reset,
    EnumUnknown_Clone
};

/**********************************************************
 * IOleObject implementation
 */

static inline HTMLDocumentNode *HTMLDocumentNode_from_IOleObject(IOleObject *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IOleObject_iface);
}

static HRESULT WINAPI DocNodeOleObject_QueryInterface(IOleObject *iface, REFIID riid, void **ppv)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    return IHTMLDOMNode_QueryInterface(&This->node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI DocNodeOleObject_AddRef(IOleObject *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    return IHTMLDOMNode_AddRef(&This->node.IHTMLDOMNode_iface);
}

static ULONG WINAPI DocNodeOleObject_Release(IOleObject *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    return IHTMLDOMNode_Release(&This->node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI DocNodeOleObject_SetClientSite(IOleObject *iface, IOleClientSite *pClientSite)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    return IOleObject_SetClientSite(&This->doc_obj->IOleObject_iface, pClientSite);
}

static HRESULT WINAPI DocNodeOleObject_GetClientSite(IOleObject *iface, IOleClientSite **ppClientSite)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    return IOleObject_GetClientSite(&This->doc_obj->IOleObject_iface, ppClientSite);
}

static HRESULT WINAPI DocNodeOleObject_SetHostNames(IOleObject *iface, LPCOLESTR szContainerApp, LPCOLESTR szContainerObj)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_w(szContainerApp), debugstr_w(szContainerObj));
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleObject_Close(IOleObject *iface, DWORD dwSaveOption)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    return IOleObject_Close(&This->doc_obj->IOleObject_iface, dwSaveOption);
}

static HRESULT WINAPI DocNodeOleObject_SetMoniker(IOleObject *iface, DWORD dwWhichMoniker, IMoniker *pmk)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    FIXME("(%p %ld %p)->()\n", This, dwWhichMoniker, pmk);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleObject_GetMoniker(IOleObject *iface, DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    FIXME("(%p)->(%ld %ld %p)\n", This, dwAssign, dwWhichMoniker, ppmk);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleObject_InitFromData(IOleObject *iface, IDataObject *pDataObject, BOOL fCreation,
                                                    DWORD dwReserved)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    FIXME("(%p)->(%p %x %ld)\n", This, pDataObject, fCreation, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleObject_GetClipboardData(IOleObject *iface, DWORD dwReserved, IDataObject **ppDataObject)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    FIXME("(%p)->(%ld %p)\n", This, dwReserved, ppDataObject);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleObject_DoVerb(IOleObject *iface, LONG iVerb, LPMSG lpmsg, IOleClientSite *pActiveSite,
                                              LONG lindex, HWND hwndParent, LPCRECT lprcPosRect)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    return IOleObject_DoVerb(&This->doc_obj->IOleObject_iface, iVerb, lpmsg, pActiveSite, lindex, hwndParent, lprcPosRect);
}

static HRESULT WINAPI DocNodeOleObject_EnumVerbs(IOleObject *iface, IEnumOLEVERB **ppEnumOleVerb)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    FIXME("(%p)->(%p)\n", This, ppEnumOleVerb);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleObject_Update(IOleObject *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleObject_IsUpToDate(IOleObject *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleObject_GetUserClassID(IOleObject *iface, CLSID *pClsid)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    return IOleObject_GetUserClassID(&This->doc_obj->IOleObject_iface, pClsid);
}

static HRESULT WINAPI DocNodeOleObject_GetUserType(IOleObject *iface, DWORD dwFormOfType, LPOLESTR *pszUserType)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    FIXME("(%p)->(%ld %p)\n", This, dwFormOfType, pszUserType);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleObject_SetExtent(IOleObject *iface, DWORD dwDrawAspect, SIZEL *psizel)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    return IOleObject_SetExtent(&This->doc_obj->IOleObject_iface, dwDrawAspect, psizel);
}

static HRESULT WINAPI DocNodeOleObject_GetExtent(IOleObject *iface, DWORD dwDrawAspect, SIZEL *psizel)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    return IOleObject_GetExtent(&This->doc_obj->IOleObject_iface, dwDrawAspect, psizel);
}

static HRESULT WINAPI DocNodeOleObject_Advise(IOleObject *iface, IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    return IOleObject_Advise(&This->doc_obj->IOleObject_iface, pAdvSink, pdwConnection);
}

static HRESULT WINAPI DocNodeOleObject_Unadvise(IOleObject *iface, DWORD dwConnection)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    return IOleObject_Unadvise(&This->doc_obj->IOleObject_iface, dwConnection);
}

static HRESULT WINAPI DocNodeOleObject_EnumAdvise(IOleObject *iface, IEnumSTATDATA **ppenumAdvise)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    return IOleObject_EnumAdvise(&This->doc_obj->IOleObject_iface, ppenumAdvise);
}

static HRESULT WINAPI DocNodeOleObject_GetMiscStatus(IOleObject *iface, DWORD dwAspect, DWORD *pdwStatus)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    FIXME("(%p)->(%ld %p)\n", This, dwAspect, pdwStatus);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleObject_SetColorScheme(IOleObject *iface, LOGPALETTE *pLogpal)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleObject(iface);
    FIXME("(%p)->(%p)\n", This, pLogpal);
    return E_NOTIMPL;
}

static const IOleObjectVtbl DocNodeOleObjectVtbl = {
    DocNodeOleObject_QueryInterface,
    DocNodeOleObject_AddRef,
    DocNodeOleObject_Release,
    DocNodeOleObject_SetClientSite,
    DocNodeOleObject_GetClientSite,
    DocNodeOleObject_SetHostNames,
    DocNodeOleObject_Close,
    DocNodeOleObject_SetMoniker,
    DocNodeOleObject_GetMoniker,
    DocNodeOleObject_InitFromData,
    DocNodeOleObject_GetClipboardData,
    DocNodeOleObject_DoVerb,
    DocNodeOleObject_EnumVerbs,
    DocNodeOleObject_Update,
    DocNodeOleObject_IsUpToDate,
    DocNodeOleObject_GetUserClassID,
    DocNodeOleObject_GetUserType,
    DocNodeOleObject_SetExtent,
    DocNodeOleObject_GetExtent,
    DocNodeOleObject_Advise,
    DocNodeOleObject_Unadvise,
    DocNodeOleObject_EnumAdvise,
    DocNodeOleObject_GetMiscStatus,
    DocNodeOleObject_SetColorScheme
};

static inline HTMLDocumentObj *HTMLDocumentObj_from_IOleObject(IOleObject *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IOleObject_iface);
}

static HRESULT WINAPI DocObjOleObject_QueryInterface(IOleObject *iface, REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI DocObjOleObject_AddRef(IOleObject *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI DocObjOleObject_Release(IOleObject *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    return IUnknown_Release(This->outer_unk);
}

static void update_hostinfo(HTMLDocumentObj *This, DOCHOSTUIINFO *hostinfo)
{
    nsIScrollable *scrollable;
    nsresult nsres;

    if(!This->nscontainer)
        return;

    nsres = nsIWebBrowser_QueryInterface(This->nscontainer->webbrowser, &IID_nsIScrollable, (void**)&scrollable);
    if(NS_SUCCEEDED(nsres)) {
        nsres = nsIScrollable_SetDefaultScrollbarPreferences(scrollable, ScrollOrientation_Y,
                (hostinfo->dwFlags & DOCHOSTUIFLAG_SCROLL_NO) ? Scrollbar_Never : Scrollbar_Auto);
        if(NS_FAILED(nsres))
            ERR("Could not set default Y scrollbar prefs: %08lx\n", nsres);

        nsres = nsIScrollable_SetDefaultScrollbarPreferences(scrollable, ScrollOrientation_X,
                hostinfo->dwFlags & DOCHOSTUIFLAG_SCROLL_NO ? Scrollbar_Never : Scrollbar_Auto);
        if(NS_FAILED(nsres))
            ERR("Could not set default X scrollbar prefs: %08lx\n", nsres);

        nsIScrollable_Release(scrollable);
    }else {
        ERR("Could not get nsIScrollable: %08lx\n", nsres);
    }
}

/* Calls undocumented 84 cmd of CGID_ShellDocView */
void call_docview_84(HTMLDocumentObj *doc)
{
    IOleCommandTarget *olecmd;
    VARIANT var;
    HRESULT hres;

    if(!doc->client)
        return;

    hres = IOleClientSite_QueryInterface(doc->client, &IID_IOleCommandTarget, (void**)&olecmd);
    if(FAILED(hres))
        return;

    VariantInit(&var);
    hres = IOleCommandTarget_Exec(olecmd, &CGID_ShellDocView, 84, 0, NULL, &var);
    IOleCommandTarget_Release(olecmd);
    if(SUCCEEDED(hres) && V_VT(&var) != VT_NULL)
        FIXME("handle result\n");
}

void set_document_navigation(HTMLDocumentObj *doc, BOOL doc_can_navigate)
{
    VARIANT var;

    if(!doc->client_cmdtrg)
        return;

    if(doc_can_navigate) {
        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = (IUnknown*)&doc->window->base.IHTMLWindow2_iface;
    }

    IOleCommandTarget_Exec(doc->client_cmdtrg, &CGID_DocHostCmdPriv, DOCHOST_DOCCANNAVIGATE, 0,
            doc_can_navigate ? &var : NULL, NULL);
}

static void load_settings(HTMLDocumentObj *doc)
{
    HKEY settings_key;
    DWORD val, size;
    LONG res;

    res = RegOpenKeyW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Internet Explorer", &settings_key);
    if(res != ERROR_SUCCESS)
        return;

    size = sizeof(val);
    res = RegGetValueW(settings_key, L"Zoom", L"ZoomFactor", RRF_RT_REG_DWORD, NULL, &val, &size);
    RegCloseKey(settings_key);
    if(res == ERROR_SUCCESS)
        set_viewer_zoom(doc->nscontainer, (float)val/100000);
}

static HRESULT WINAPI DocObjOleObject_SetClientSite(IOleObject *iface, IOleClientSite *pClientSite)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    IBrowserService *browser_service;
    IOleCommandTarget *cmdtrg = NULL;
    IOleWindow *ole_window;
    BOOL hostui_setup;
    VARIANT silent;
    HWND hwnd;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, pClientSite);

    if(pClientSite == This->client)
        return S_OK;

    if(This->client)
        This->nscontainer->usermode = UNKNOWN_USERMODE;

    unlink_ref(&This->client);
    unlink_ref(&This->client_cmdtrg);
    if(!This->custom_hostui)
        unlink_ref(&This->hostui);
    unlink_ref(&This->doc_object_service);
    unlink_ref(&This->webbrowser);
    unlink_ref(&This->browser_service);
    unlink_ref(&This->travel_log);

    memset(&This->hostinfo, 0, sizeof(DOCHOSTUIINFO));

    if(!pClientSite)
        return S_OK;

    IOleClientSite_AddRef(pClientSite);
    This->client = pClientSite;

    hostui_setup = This->hostui_setup;

    if(!This->hostui) {
        IDocHostUIHandler *uihandler;

        This->custom_hostui = FALSE;

        hres = IOleClientSite_QueryInterface(pClientSite, &IID_IDocHostUIHandler, (void**)&uihandler);
        if(SUCCEEDED(hres))
            This->hostui = uihandler;
    }

    if(This->hostui) {
        DOCHOSTUIINFO hostinfo;
        LPOLESTR key_path = NULL, override_key_path = NULL;
        IDocHostUIHandler2 *uihandler2;

        memset(&hostinfo, 0, sizeof(DOCHOSTUIINFO));
        hostinfo.cbSize = sizeof(DOCHOSTUIINFO);
        hres = IDocHostUIHandler_GetHostInfo(This->hostui, &hostinfo);
        if(SUCCEEDED(hres)) {
            TRACE("hostinfo = {%lu %08lx %08lx %s %s}\n",
                    hostinfo.cbSize, hostinfo.dwFlags, hostinfo.dwDoubleClick,
                    debugstr_w(hostinfo.pchHostCss), debugstr_w(hostinfo.pchHostNS));
            update_hostinfo(This, &hostinfo);
            This->hostinfo = hostinfo;
        }

        if(!hostui_setup) {
            hres = IDocHostUIHandler_GetOptionKeyPath(This->hostui, &key_path, 0);
            if(hres == S_OK && key_path) {
                if(key_path[0]) {
                    /* FIXME: use key_path */
                    FIXME("key_path = %s\n", debugstr_w(key_path));
                }
                CoTaskMemFree(key_path);
            }

            hres = IDocHostUIHandler_QueryInterface(This->hostui, &IID_IDocHostUIHandler2,
                    (void**)&uihandler2);
            if(SUCCEEDED(hres)) {
                hres = IDocHostUIHandler2_GetOverrideKeyPath(uihandler2, &override_key_path, 0);
                if(hres == S_OK && override_key_path) {
                    if(override_key_path[0]) {
                        /*FIXME: use override_key_path */
                        FIXME("override_key_path = %s\n", debugstr_w(override_key_path));
                    }
                    CoTaskMemFree(override_key_path);
                }
                IDocHostUIHandler2_Release(uihandler2);
            }

            This->hostui_setup = TRUE;
        }
    }

    load_settings(This);

    /* Native calls here GetWindow. What is it for?
     * We don't have anything to do with it here (yet). */
    hres = IOleClientSite_QueryInterface(pClientSite, &IID_IOleWindow, (void**)&ole_window);
    if(SUCCEEDED(hres)) {
        IOleWindow_GetWindow(ole_window, &hwnd);
        IOleWindow_Release(ole_window);
    }

    hres = do_query_service((IUnknown*)pClientSite, &IID_IShellBrowser,
            &IID_IBrowserService, (void**)&browser_service);
    if(SUCCEEDED(hres)) {
        ITravelLog *travel_log;

        This->browser_service = (IUnknown*)browser_service;

        hres = IBrowserService_GetTravelLog(browser_service, &travel_log);
        if(SUCCEEDED(hres))
            This->travel_log = travel_log;
    }else {
        browser_service = NULL;
    }

    hres = IOleClientSite_QueryInterface(pClientSite, &IID_IOleCommandTarget, (void**)&cmdtrg);
    if(SUCCEEDED(hres)) {
        VARIANT var;
        OLECMD cmd = {OLECMDID_SETPROGRESSTEXT, 0};

        This->client_cmdtrg = cmdtrg;

        if(!hostui_setup) {
            IDocObjectService *doc_object_service;
            IWebBrowser2 *wb;

            set_document_navigation(This, TRUE);

            if(browser_service) {
                hres = IBrowserService_QueryInterface(browser_service,
                        &IID_IDocObjectService, (void**)&doc_object_service);
                if(SUCCEEDED(hres)) {
                    This->doc_object_service = doc_object_service;

                    /*
                     * Some embedding routines, esp. in regards to use of IDocObjectService, differ if
                     * embedder supports IWebBrowserApp.
                     */
                    hres = do_query_service((IUnknown*)pClientSite, &IID_IWebBrowserApp, &IID_IWebBrowser2, (void**)&wb);
                    if(SUCCEEDED(hres))
                        This->webbrowser = (IUnknown*)wb;
                }
            }
        }

        call_docview_84(This);

        IOleCommandTarget_QueryStatus(cmdtrg, NULL, 1, &cmd, NULL);

        V_VT(&var) = VT_I4;
        V_I4(&var) = 0;
        IOleCommandTarget_Exec(cmdtrg, NULL, OLECMDID_SETPROGRESSMAX,
                OLECMDEXECOPT_DONTPROMPTUSER, &var, NULL);
        IOleCommandTarget_Exec(cmdtrg, NULL, OLECMDID_SETPROGRESSPOS, 
                OLECMDEXECOPT_DONTPROMPTUSER, &var, NULL);
    }

    if(This->nscontainer->usermode == UNKNOWN_USERMODE)
        IOleControl_OnAmbientPropertyChange(&This->IOleControl_iface, DISPID_AMBIENT_USERMODE);

    IOleControl_OnAmbientPropertyChange(&This->IOleControl_iface,
            DISPID_AMBIENT_OFFLINEIFNOTCONNECTED);

    hres = get_client_disp_property(This->client, DISPID_AMBIENT_SILENT, &silent);
    if(SUCCEEDED(hres)) {
        if(V_VT(&silent) != VT_BOOL)
            WARN("silent = %s\n", debugstr_variant(&silent));
        else if(V_BOOL(&silent))
            FIXME("silent == true\n");
    }

    IOleControl_OnAmbientPropertyChange(&This->IOleControl_iface, DISPID_AMBIENT_USERAGENT);
    IOleControl_OnAmbientPropertyChange(&This->IOleControl_iface, DISPID_AMBIENT_PALETTE);

    return S_OK;
}

static HRESULT WINAPI DocObjOleObject_GetClientSite(IOleObject *iface, IOleClientSite **ppClientSite)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);

    TRACE("(%p)->(%p)\n", This, ppClientSite);

    if(!ppClientSite)
        return E_INVALIDARG;

    if(This->client)
        IOleClientSite_AddRef(This->client);
    *ppClientSite = This->client;

    return S_OK;
}

static HRESULT WINAPI DocObjOleObject_SetHostNames(IOleObject *iface, LPCOLESTR szContainerApp, LPCOLESTR szContainerObj)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_w(szContainerApp), debugstr_w(szContainerObj));
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleObject_Close(IOleObject *iface, DWORD dwSaveOption)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);

    TRACE("(%p)->(%08lx)\n", This, dwSaveOption);

    if(dwSaveOption == OLECLOSE_PROMPTSAVE)
        FIXME("OLECLOSE_PROMPTSAVE not implemented\n");

    if(This->in_place_active)
        IOleInPlaceObjectWindowless_InPlaceDeactivate(&This->IOleInPlaceObjectWindowless_iface);

    HTMLDocument_LockContainer(This, FALSE);

    if(This->advise_holder)
        IOleAdviseHolder_SendOnClose(This->advise_holder);

    return S_OK;
}

static HRESULT WINAPI DocObjOleObject_SetMoniker(IOleObject *iface, DWORD dwWhichMoniker, IMoniker *pmk)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    FIXME("(%p %ld %p)->()\n", This, dwWhichMoniker, pmk);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleObject_GetMoniker(IOleObject *iface, DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    FIXME("(%p)->(%ld %ld %p)\n", This, dwAssign, dwWhichMoniker, ppmk);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleObject_InitFromData(IOleObject *iface, IDataObject *pDataObject, BOOL fCreation,
                                                   DWORD dwReserved)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    FIXME("(%p)->(%p %x %ld)\n", This, pDataObject, fCreation, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleObject_GetClipboardData(IOleObject *iface, DWORD dwReserved, IDataObject **ppDataObject)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    FIXME("(%p)->(%ld %p)\n", This, dwReserved, ppDataObject);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleObject_DoVerb(IOleObject *iface, LONG iVerb, LPMSG lpmsg, IOleClientSite *pActiveSite,
                                             LONG lindex, HWND hwndParent, LPCRECT lprcPosRect)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    IOleDocumentSite *pDocSite;
    HRESULT hres;

    TRACE("(%p)->(%ld %p %p %ld %p %p)\n", This, iVerb, lpmsg, pActiveSite, lindex, hwndParent, lprcPosRect);

    if(iVerb != OLEIVERB_SHOW && iVerb != OLEIVERB_UIACTIVATE && iVerb != OLEIVERB_INPLACEACTIVATE) {
        FIXME("iVerb = %ld not supported\n", iVerb);
        return E_NOTIMPL;
    }

    if(!pActiveSite)
        pActiveSite = This->client;

    hres = IOleClientSite_QueryInterface(pActiveSite, &IID_IOleDocumentSite, (void**)&pDocSite);
    if(SUCCEEDED(hres)) {
        HTMLDocument_LockContainer(This, TRUE);

        /* FIXME: Create new IOleDocumentView. See CreateView for more info. */
        hres = IOleDocumentSite_ActivateMe(pDocSite, &This->IOleDocumentView_iface);
        IOleDocumentSite_Release(pDocSite);
    }else {
        hres = IOleDocumentView_UIActivate(&This->IOleDocumentView_iface, TRUE);
        if(SUCCEEDED(hres)) {
            if(lprcPosRect) {
                RECT rect; /* We need to pass rect as not const pointer */
                rect = *lprcPosRect;
                IOleDocumentView_SetRect(&This->IOleDocumentView_iface, &rect);
            }
            IOleDocumentView_Show(&This->IOleDocumentView_iface, TRUE);
        }
    }

    return hres;
}

static HRESULT WINAPI DocObjOleObject_EnumVerbs(IOleObject *iface, IEnumOLEVERB **ppEnumOleVerb)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    FIXME("(%p)->(%p)\n", This, ppEnumOleVerb);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleObject_Update(IOleObject *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleObject_IsUpToDate(IOleObject *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleObject_GetUserClassID(IOleObject *iface, CLSID *pClsid)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);

    TRACE("(%p)->(%p)\n", This, pClsid);

    if(!pClsid)
        return E_INVALIDARG;

    *pClsid = CLSID_HTMLDocument;
    return S_OK;
}

static HRESULT WINAPI DocObjOleObject_GetUserType(IOleObject *iface, DWORD dwFormOfType, LPOLESTR *pszUserType)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    FIXME("(%p)->(%ld %p)\n", This, dwFormOfType, pszUserType);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleObject_SetExtent(IOleObject *iface, DWORD dwDrawAspect, SIZEL *psizel)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);

    TRACE("(%p)->(%ld %p)\n", This, dwDrawAspect, psizel);

    if (dwDrawAspect != DVASPECT_CONTENT)
        return E_INVALIDARG;

    This->extent = *psizel;
    return S_OK;
}

static HRESULT WINAPI DocObjOleObject_GetExtent(IOleObject *iface, DWORD dwDrawAspect, SIZEL *psizel)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);

    TRACE("(%p)->(%ld %p)\n", This, dwDrawAspect, psizel);

    if (dwDrawAspect != DVASPECT_CONTENT)
        return E_INVALIDARG;

    *psizel = This->extent;
    return S_OK;
}

static HRESULT WINAPI DocObjOleObject_Advise(IOleObject *iface, IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    TRACE("(%p)->(%p %p)\n", This, pAdvSink, pdwConnection);

    if(!pdwConnection)
        return E_INVALIDARG;

    if(!pAdvSink) {
        *pdwConnection = 0;
        return E_INVALIDARG;
    }

    if(!This->advise_holder) {
        CreateOleAdviseHolder(&This->advise_holder);
        if(!This->advise_holder)
            return E_OUTOFMEMORY;
    }

    return IOleAdviseHolder_Advise(This->advise_holder, pAdvSink, pdwConnection);
}

static HRESULT WINAPI DocObjOleObject_Unadvise(IOleObject *iface, DWORD dwConnection)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    TRACE("(%p)->(%ld)\n", This, dwConnection);

    if(!This->advise_holder)
        return OLE_E_NOCONNECTION;

    return IOleAdviseHolder_Unadvise(This->advise_holder, dwConnection);
}

static HRESULT WINAPI DocObjOleObject_EnumAdvise(IOleObject *iface, IEnumSTATDATA **ppenumAdvise)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);

    if(!This->advise_holder) {
        *ppenumAdvise = NULL;
        return S_OK;
    }

    return IOleAdviseHolder_EnumAdvise(This->advise_holder, ppenumAdvise);
}

static HRESULT WINAPI DocObjOleObject_GetMiscStatus(IOleObject *iface, DWORD dwAspect, DWORD *pdwStatus)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    FIXME("(%p)->(%ld %p)\n", This, dwAspect, pdwStatus);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleObject_SetColorScheme(IOleObject *iface, LOGPALETTE *pLogpal)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleObject(iface);
    FIXME("(%p)->(%p)\n", This, pLogpal);
    return E_NOTIMPL;
}

static const IOleObjectVtbl DocObjOleObjectVtbl = {
    DocObjOleObject_QueryInterface,
    DocObjOleObject_AddRef,
    DocObjOleObject_Release,
    DocObjOleObject_SetClientSite,
    DocObjOleObject_GetClientSite,
    DocObjOleObject_SetHostNames,
    DocObjOleObject_Close,
    DocObjOleObject_SetMoniker,
    DocObjOleObject_GetMoniker,
    DocObjOleObject_InitFromData,
    DocObjOleObject_GetClipboardData,
    DocObjOleObject_DoVerb,
    DocObjOleObject_EnumVerbs,
    DocObjOleObject_Update,
    DocObjOleObject_IsUpToDate,
    DocObjOleObject_GetUserClassID,
    DocObjOleObject_GetUserType,
    DocObjOleObject_SetExtent,
    DocObjOleObject_GetExtent,
    DocObjOleObject_Advise,
    DocObjOleObject_Unadvise,
    DocObjOleObject_EnumAdvise,
    DocObjOleObject_GetMiscStatus,
    DocObjOleObject_SetColorScheme
};

/**********************************************************
 * IOleDocument implementation
 */

static inline HTMLDocumentNode *HTMLDocumentNode_from_IOleDocument(IOleDocument *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IOleDocument_iface);
}

static HRESULT WINAPI DocNodeOleDocument_QueryInterface(IOleDocument *iface, REFIID riid, void **ppv)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleDocument(iface);
    return IHTMLDOMNode_QueryInterface(&This->node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI DocNodeOleDocument_AddRef(IOleDocument *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleDocument(iface);
    return IHTMLDOMNode_AddRef(&This->node.IHTMLDOMNode_iface);
}

static ULONG WINAPI DocNodeOleDocument_Release(IOleDocument *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleDocument(iface);
    return IHTMLDOMNode_Release(&This->node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI DocNodeOleDocument_CreateView(IOleDocument *iface, IOleInPlaceSite *pIPSite, IStream *pstm,
                                                    DWORD dwReserved, IOleDocumentView **ppView)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleDocument(iface);
    return IOleDocument_CreateView(&This->doc_obj->IOleDocument_iface, pIPSite, pstm, dwReserved, ppView);
}

static HRESULT WINAPI DocNodeOleDocument_GetDocMiscStatus(IOleDocument *iface, DWORD *pdwStatus)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleDocument(iface);
    FIXME("(%p)->(%p)\n", This, pdwStatus);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleDocument_EnumViews(IOleDocument *iface, IEnumOleDocumentViews **ppEnum,
                                                   IOleDocumentView **ppView)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleDocument(iface);
    FIXME("(%p)->(%p %p)\n", This, ppEnum, ppView);
    return E_NOTIMPL;
}

static const IOleDocumentVtbl DocNodeOleDocumentVtbl = {
    DocNodeOleDocument_QueryInterface,
    DocNodeOleDocument_AddRef,
    DocNodeOleDocument_Release,
    DocNodeOleDocument_CreateView,
    DocNodeOleDocument_GetDocMiscStatus,
    DocNodeOleDocument_EnumViews
};

static inline HTMLDocumentObj *HTMLDocumentObj_from_IOleDocument(IOleDocument *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IOleDocument_iface);
}

static HRESULT WINAPI DocObjOleDocument_QueryInterface(IOleDocument *iface, REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleDocument(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI DocObjOleDocument_AddRef(IOleDocument *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleDocument(iface);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI DocObjOleDocument_Release(IOleDocument *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleDocument(iface);
    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI DocObjOleDocument_CreateView(IOleDocument *iface, IOleInPlaceSite *pIPSite, IStream *pstm,
                                                   DWORD dwReserved, IOleDocumentView **ppView)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleDocument(iface);
    HRESULT hres;

    TRACE("(%p)->(%p %p %ld %p)\n", This, pIPSite, pstm, dwReserved, ppView);

    if(!ppView)
        return E_INVALIDARG;

    /* FIXME:
     * Windows implementation creates new IOleDocumentView when function is called for the
     * first time and returns E_FAIL when it is called for the second time, but it doesn't matter
     * if the application uses returned interfaces, passed to ActivateMe or returned by
     * QueryInterface, so there is no reason to create new interface. This needs more testing.
     */

    if(pIPSite) {
        hres = IOleDocumentView_SetInPlaceSite(&This->IOleDocumentView_iface, pIPSite);
        if(FAILED(hres))
            return hres;
    }

    if(pstm)
        FIXME("pstm is not supported\n");

    IOleDocumentView_AddRef(&This->IOleDocumentView_iface);
    *ppView = &This->IOleDocumentView_iface;
    return S_OK;
}

static HRESULT WINAPI DocObjOleDocument_GetDocMiscStatus(IOleDocument *iface, DWORD *pdwStatus)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleDocument(iface);
    FIXME("(%p)->(%p)\n", This, pdwStatus);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleDocument_EnumViews(IOleDocument *iface, IEnumOleDocumentViews **ppEnum,
                                                  IOleDocumentView **ppView)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleDocument(iface);
    FIXME("(%p)->(%p %p)\n", This, ppEnum, ppView);
    return E_NOTIMPL;
}

static const IOleDocumentVtbl DocObjOleDocumentVtbl = {
    DocObjOleDocument_QueryInterface,
    DocObjOleDocument_AddRef,
    DocObjOleDocument_Release,
    DocObjOleDocument_CreateView,
    DocObjOleDocument_GetDocMiscStatus,
    DocObjOleDocument_EnumViews
};

/**********************************************************
 * IOleControl implementation
 */

static inline HTMLDocumentNode *HTMLDocumentNode_from_IOleControl(IOleControl *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IOleControl_iface);
}

static HRESULT WINAPI DocNodeOleControl_QueryInterface(IOleControl *iface, REFIID riid, void **ppv)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleControl(iface);
    return IHTMLDOMNode_QueryInterface(&This->node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI DocNodeOleControl_AddRef(IOleControl *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleControl(iface);
    return IHTMLDOMNode_AddRef(&This->node.IHTMLDOMNode_iface);
}

static ULONG WINAPI DocNodeOleControl_Release(IOleControl *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleControl(iface);
    return IHTMLDOMNode_Release(&This->node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI DocNodeOleControl_GetControlInfo(IOleControl *iface, CONTROLINFO *pCI)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleControl(iface);
    FIXME("(%p)->(%p)\n", This, pCI);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleControl_OnMnemonic(IOleControl *iface, MSG *pMsg)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleControl(iface);
    FIXME("(%p)->(%p)\n", This, pMsg);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleControl_OnAmbientPropertyChange(IOleControl *iface, DISPID dispID)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleControl(iface);
    return IOleControl_OnAmbientPropertyChange(&This->doc_obj->IOleControl_iface, dispID);
}

static HRESULT WINAPI DocNodeOleControl_FreezeEvents(IOleControl *iface, BOOL bFreeze)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleControl(iface);
    FIXME("(%p)->(%x)\n", This, bFreeze);
    return E_NOTIMPL;
}

static const IOleControlVtbl DocNodeOleControlVtbl = {
    DocNodeOleControl_QueryInterface,
    DocNodeOleControl_AddRef,
    DocNodeOleControl_Release,
    DocNodeOleControl_GetControlInfo,
    DocNodeOleControl_OnMnemonic,
    DocNodeOleControl_OnAmbientPropertyChange,
    DocNodeOleControl_FreezeEvents
};

static inline HTMLDocumentObj *HTMLDocumentObj_from_IOleControl(IOleControl *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IOleControl_iface);
}

static HRESULT WINAPI DocObjOleControl_QueryInterface(IOleControl *iface, REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleControl(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI DocObjOleControl_AddRef(IOleControl *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleControl(iface);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI DocObjOleControl_Release(IOleControl *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleControl(iface);
    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI DocObjOleControl_GetControlInfo(IOleControl *iface, CONTROLINFO *pCI)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleControl(iface);
    FIXME("(%p)->(%p)\n", This, pCI);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleControl_OnMnemonic(IOleControl *iface, MSG *pMsg)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleControl(iface);
    FIXME("(%p)->(%p)\n", This, pMsg);
    return E_NOTIMPL;
}

HRESULT get_client_disp_property(IOleClientSite *client, DISPID dispid, VARIANT *res)
{
    IDispatch *disp = NULL;
    DISPPARAMS dispparams = {NULL, 0};
    UINT err;
    HRESULT hres;

    hres = IOleClientSite_QueryInterface(client, &IID_IDispatch, (void**)&disp);
    if(FAILED(hres)) {
        TRACE("Could not get IDispatch\n");
        return hres;
    }

    VariantInit(res);

    hres = IDispatch_Invoke(disp, dispid, &IID_NULL, LOCALE_SYSTEM_DEFAULT,
            DISPATCH_PROPERTYGET, &dispparams, res, NULL, &err);

    IDispatch_Release(disp);

    return hres;
}

static HRESULT on_change_dlcontrol(HTMLDocumentObj *This)
{
    VARIANT res;
    HRESULT hres;

    hres = get_client_disp_property(This->client, DISPID_AMBIENT_DLCONTROL, &res);
    if(SUCCEEDED(hres))
        FIXME("unsupported dlcontrol %08lx\n", V_I4(&res));

    return S_OK;
}

static HRESULT WINAPI DocObjOleControl_OnAmbientPropertyChange(IOleControl *iface, DISPID dispID)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleControl(iface);
    IOleClientSite *client;
    VARIANT res;
    HRESULT hres;

    client = This->client;
    if(!client) {
        TRACE("client = NULL\n");
        return S_OK;
    }

    switch(dispID) {
    case DISPID_AMBIENT_USERMODE:
        TRACE("(%p)->(DISPID_AMBIENT_USERMODE)\n", This);
        hres = get_client_disp_property(client, DISPID_AMBIENT_USERMODE, &res);
        if(FAILED(hres))
            return S_OK;

        if(V_VT(&res) == VT_BOOL) {
            if(V_BOOL(&res)) {
                This->nscontainer->usermode = BROWSEMODE;
            }else {
                FIXME("edit mode is not supported\n");
                This->nscontainer->usermode = EDITMODE;
            }
        }else {
            FIXME("usermode=%s\n", debugstr_variant(&res));
        }
        return S_OK;
    case DISPID_AMBIENT_DLCONTROL:
        TRACE("(%p)->(DISPID_AMBIENT_DLCONTROL)\n", This);
        return on_change_dlcontrol(This);
    case DISPID_AMBIENT_OFFLINEIFNOTCONNECTED:
        TRACE("(%p)->(DISPID_AMBIENT_OFFLINEIFNOTCONNECTED)\n", This);
        on_change_dlcontrol(This);
        hres = get_client_disp_property(client, DISPID_AMBIENT_OFFLINEIFNOTCONNECTED, &res);
        if(FAILED(hres))
            return S_OK;

        if(V_VT(&res) == VT_BOOL) {
            if(V_BOOL(&res)) {
                FIXME("offline connection is not supported\n");
                hres = E_FAIL;
            }
        }else {
            FIXME("offlineconnected=%s\n", debugstr_variant(&res));
        }
        return S_OK;
    case DISPID_AMBIENT_SILENT:
        TRACE("(%p)->(DISPID_AMBIENT_SILENT)\n", This);
        on_change_dlcontrol(This);
        hres = get_client_disp_property(client, DISPID_AMBIENT_SILENT, &res);
        if(FAILED(hres))
            return S_OK;

        if(V_VT(&res) == VT_BOOL) {
            if(V_BOOL(&res)) {
                FIXME("silent mode is not supported\n");
            }
        }else {
            FIXME("silent=%s\n", debugstr_variant(&res));
        }
        return S_OK;
    case DISPID_AMBIENT_USERAGENT:
        TRACE("(%p)->(DISPID_AMBIENT_USERAGENT)\n", This);
        hres = get_client_disp_property(client, DISPID_AMBIENT_USERAGENT, &res);
        if(FAILED(hres))
            return S_OK;

        FIXME("not supported AMBIENT_USERAGENT\n");
        return S_OK;
    case DISPID_AMBIENT_PALETTE:
        TRACE("(%p)->(DISPID_AMBIENT_PALETTE)\n", This);
        hres = get_client_disp_property(client, DISPID_AMBIENT_PALETTE, &res);
        if(FAILED(hres))
            return S_OK;

        FIXME("not supported AMBIENT_PALETTE\n");
        return S_OK;
    }

    FIXME("(%p) unsupported dispID=%ld\n", This, dispID);
    return E_FAIL;
}

static HRESULT WINAPI DocObjOleControl_FreezeEvents(IOleControl *iface, BOOL bFreeze)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleControl(iface);
    FIXME("(%p)->(%x)\n", This, bFreeze);
    return E_NOTIMPL;
}

static const IOleControlVtbl DocObjOleControlVtbl = {
    DocObjOleControl_QueryInterface,
    DocObjOleControl_AddRef,
    DocObjOleControl_Release,
    DocObjOleControl_GetControlInfo,
    DocObjOleControl_OnMnemonic,
    DocObjOleControl_OnAmbientPropertyChange,
    DocObjOleControl_FreezeEvents
};

/**********************************************************
 * IOleInPlaceActiveObject implementation
 */

static inline HTMLDocumentNode *HTMLDocumentNode_from_IOleInPlaceActiveObject(IOleInPlaceActiveObject *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IOleInPlaceActiveObject_iface);
}

static HRESULT WINAPI DocNodeOleInPlaceActiveObject_QueryInterface(IOleInPlaceActiveObject *iface, REFIID riid, void **ppv)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceActiveObject(iface);
    return IHTMLDOMNode_QueryInterface(&This->node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI DocNodeOleInPlaceActiveObject_AddRef(IOleInPlaceActiveObject *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceActiveObject(iface);
    return IHTMLDOMNode_AddRef(&This->node.IHTMLDOMNode_iface);
}

static ULONG WINAPI DocNodeOleInPlaceActiveObject_Release(IOleInPlaceActiveObject *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceActiveObject(iface);
    return IHTMLDOMNode_Release(&This->node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI DocNodeOleInPlaceActiveObject_GetWindow(IOleInPlaceActiveObject *iface, HWND *phwnd)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceActiveObject(iface);
    return IOleInPlaceActiveObject_GetWindow(&This->doc_obj->IOleInPlaceActiveObject_iface, phwnd);
}

static HRESULT WINAPI DocNodeOleInPlaceActiveObject_ContextSensitiveHelp(IOleInPlaceActiveObject *iface, BOOL fEnterMode)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%x)\n", This, fEnterMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleInPlaceActiveObject_TranslateAccelerator(IOleInPlaceActiveObject *iface, LPMSG lpmsg)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%p)\n", This, lpmsg);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleInPlaceActiveObject_OnFrameWindowActivate(IOleInPlaceActiveObject *iface,
        BOOL fActivate)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceActiveObject(iface);
    return IOleInPlaceActiveObject_OnFrameWindowActivate(&This->doc_obj->IOleInPlaceActiveObject_iface, fActivate);
}

static HRESULT WINAPI DocNodeOleInPlaceActiveObject_OnDocWindowActivate(IOleInPlaceActiveObject *iface, BOOL fActivate)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%x)\n", This, fActivate);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleInPlaceActiveObject_ResizeBorder(IOleInPlaceActiveObject *iface, LPCRECT prcBorder,
                                                                 IOleInPlaceUIWindow *pUIWindow, BOOL fFrameWindow)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%p %p %x)\n", This, prcBorder, pUIWindow, fFrameWindow);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleInPlaceActiveObject_EnableModeless(IOleInPlaceActiveObject *iface, BOOL fEnable)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%x)\n", This, fEnable);
    return E_NOTIMPL;
}

static const IOleInPlaceActiveObjectVtbl DocNodeOleInPlaceActiveObjectVtbl = {
    DocNodeOleInPlaceActiveObject_QueryInterface,
    DocNodeOleInPlaceActiveObject_AddRef,
    DocNodeOleInPlaceActiveObject_Release,
    DocNodeOleInPlaceActiveObject_GetWindow,
    DocNodeOleInPlaceActiveObject_ContextSensitiveHelp,
    DocNodeOleInPlaceActiveObject_TranslateAccelerator,
    DocNodeOleInPlaceActiveObject_OnFrameWindowActivate,
    DocNodeOleInPlaceActiveObject_OnDocWindowActivate,
    DocNodeOleInPlaceActiveObject_ResizeBorder,
    DocNodeOleInPlaceActiveObject_EnableModeless
};

static inline HTMLDocumentObj *HTMLDocumentObj_from_IOleInPlaceActiveObject(IOleInPlaceActiveObject *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IOleInPlaceActiveObject_iface);
}

static HRESULT WINAPI DocObjOleInPlaceActiveObject_QueryInterface(IOleInPlaceActiveObject *iface, REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceActiveObject(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI DocObjOleInPlaceActiveObject_AddRef(IOleInPlaceActiveObject *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceActiveObject(iface);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI DocObjOleInPlaceActiveObject_Release(IOleInPlaceActiveObject *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceActiveObject(iface);
    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI DocObjOleInPlaceActiveObject_GetWindow(IOleInPlaceActiveObject *iface, HWND *phwnd)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceActiveObject(iface);

    TRACE("(%p)->(%p)\n", This, phwnd);

    if(!phwnd)
        return E_INVALIDARG;

    if(!This->in_place_active) {
        *phwnd = NULL;
        return E_FAIL;
    }

    *phwnd = This->hwnd;
    return S_OK;
}

static HRESULT WINAPI DocObjOleInPlaceActiveObject_ContextSensitiveHelp(IOleInPlaceActiveObject *iface, BOOL fEnterMode)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%x)\n", This, fEnterMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleInPlaceActiveObject_TranslateAccelerator(IOleInPlaceActiveObject *iface, LPMSG lpmsg)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%p)\n", This, lpmsg);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleInPlaceActiveObject_OnFrameWindowActivate(IOleInPlaceActiveObject *iface,
        BOOL fActivate)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceActiveObject(iface);

    TRACE("(%p)->(%x)\n", This, fActivate);

    if(This->hostui)
        IDocHostUIHandler_OnFrameWindowActivate(This->hostui, fActivate);

    return S_OK;
}

static HRESULT WINAPI DocObjOleInPlaceActiveObject_OnDocWindowActivate(IOleInPlaceActiveObject *iface, BOOL fActivate)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%x)\n", This, fActivate);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleInPlaceActiveObject_ResizeBorder(IOleInPlaceActiveObject *iface, LPCRECT prcBorder,
                                                                IOleInPlaceUIWindow *pUIWindow, BOOL fFrameWindow)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%p %p %x)\n", This, prcBorder, pUIWindow, fFrameWindow);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleInPlaceActiveObject_EnableModeless(IOleInPlaceActiveObject *iface, BOOL fEnable)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%x)\n", This, fEnable);
    return E_NOTIMPL;
}

static const IOleInPlaceActiveObjectVtbl DocObjOleInPlaceActiveObjectVtbl = {
    DocObjOleInPlaceActiveObject_QueryInterface,
    DocObjOleInPlaceActiveObject_AddRef,
    DocObjOleInPlaceActiveObject_Release,
    DocObjOleInPlaceActiveObject_GetWindow,
    DocObjOleInPlaceActiveObject_ContextSensitiveHelp,
    DocObjOleInPlaceActiveObject_TranslateAccelerator,
    DocObjOleInPlaceActiveObject_OnFrameWindowActivate,
    DocObjOleInPlaceActiveObject_OnDocWindowActivate,
    DocObjOleInPlaceActiveObject_ResizeBorder,
    DocObjOleInPlaceActiveObject_EnableModeless
};

/**********************************************************
 * IOleInPlaceObjectWindowless implementation
 */

static inline HTMLDocumentNode *HTMLDocumentNode_from_IOleInPlaceObjectWindowless(IOleInPlaceObjectWindowless *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IOleInPlaceObjectWindowless_iface);
}

static HRESULT WINAPI DocNodeOleInPlaceObjectWindowless_QueryInterface(IOleInPlaceObjectWindowless *iface,
        REFIID riid, void **ppv)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceObjectWindowless(iface);
    return IHTMLDOMNode_QueryInterface(&This->node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI DocNodeOleInPlaceObjectWindowless_AddRef(IOleInPlaceObjectWindowless *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceObjectWindowless(iface);
    return IHTMLDOMNode_AddRef(&This->node.IHTMLDOMNode_iface);
}

static ULONG WINAPI DocNodeOleInPlaceObjectWindowless_Release(IOleInPlaceObjectWindowless *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceObjectWindowless(iface);
    return IHTMLDOMNode_Release(&This->node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI DocNodeOleInPlaceObjectWindowless_GetWindow(IOleInPlaceObjectWindowless *iface,
        HWND *phwnd)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceObjectWindowless(iface);
    return IOleInPlaceActiveObject_GetWindow(&This->IOleInPlaceActiveObject_iface, phwnd);
}

static HRESULT WINAPI DocNodeOleInPlaceObjectWindowless_ContextSensitiveHelp(IOleInPlaceObjectWindowless *iface,
        BOOL fEnterMode)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceObjectWindowless(iface);
    return IOleInPlaceActiveObject_ContextSensitiveHelp(&This->IOleInPlaceActiveObject_iface, fEnterMode);
}

static HRESULT WINAPI DocNodeOleInPlaceObjectWindowless_InPlaceDeactivate(IOleInPlaceObjectWindowless *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceObjectWindowless(iface);
    return IOleInPlaceObjectWindowless_InPlaceDeactivate(&This->doc_obj->IOleInPlaceObjectWindowless_iface);
}

static HRESULT WINAPI DocNodeOleInPlaceObjectWindowless_UIDeactivate(IOleInPlaceObjectWindowless *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleInPlaceObjectWindowless_SetObjectRects(IOleInPlaceObjectWindowless *iface,
        const RECT *pos, const RECT *clip)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceObjectWindowless(iface);
    return IOleInPlaceObjectWindowless_SetObjectRects(&This->doc_obj->IOleInPlaceObjectWindowless_iface, pos, clip);
}

static HRESULT WINAPI DocNodeOleInPlaceObjectWindowless_ReactivateAndUndo(IOleInPlaceObjectWindowless *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleInPlaceObjectWindowless_OnWindowMessage(IOleInPlaceObjectWindowless *iface,
        UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *lpResult)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)->(%u %Iu %Iu %p)\n", This, msg, wParam, lParam, lpResult);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleInPlaceObjectWindowless_GetDropTarget(IOleInPlaceObjectWindowless *iface,
        IDropTarget **ppDropTarget)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)->(%p)\n", This, ppDropTarget);
    return E_NOTIMPL;
}

static const IOleInPlaceObjectWindowlessVtbl DocNodeOleInPlaceObjectWindowlessVtbl = {
    DocNodeOleInPlaceObjectWindowless_QueryInterface,
    DocNodeOleInPlaceObjectWindowless_AddRef,
    DocNodeOleInPlaceObjectWindowless_Release,
    DocNodeOleInPlaceObjectWindowless_GetWindow,
    DocNodeOleInPlaceObjectWindowless_ContextSensitiveHelp,
    DocNodeOleInPlaceObjectWindowless_InPlaceDeactivate,
    DocNodeOleInPlaceObjectWindowless_UIDeactivate,
    DocNodeOleInPlaceObjectWindowless_SetObjectRects,
    DocNodeOleInPlaceObjectWindowless_ReactivateAndUndo,
    DocNodeOleInPlaceObjectWindowless_OnWindowMessage,
    DocNodeOleInPlaceObjectWindowless_GetDropTarget
};

static inline HTMLDocumentObj *HTMLDocumentObj_from_IOleInPlaceObjectWindowless(IOleInPlaceObjectWindowless *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IOleInPlaceObjectWindowless_iface);
}

static HRESULT WINAPI DocObjOleInPlaceObjectWindowless_QueryInterface(IOleInPlaceObjectWindowless *iface,
        REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceObjectWindowless(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI DocObjOleInPlaceObjectWindowless_AddRef(IOleInPlaceObjectWindowless *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceObjectWindowless(iface);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI DocObjOleInPlaceObjectWindowless_Release(IOleInPlaceObjectWindowless *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceObjectWindowless(iface);
    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI DocObjOleInPlaceObjectWindowless_GetWindow(IOleInPlaceObjectWindowless *iface,
        HWND *phwnd)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceObjectWindowless(iface);
    return IOleInPlaceActiveObject_GetWindow(&This->IOleInPlaceActiveObject_iface, phwnd);
}

static HRESULT WINAPI DocObjOleInPlaceObjectWindowless_ContextSensitiveHelp(IOleInPlaceObjectWindowless *iface,
        BOOL fEnterMode)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceObjectWindowless(iface);
    return IOleInPlaceActiveObject_ContextSensitiveHelp(&This->IOleInPlaceActiveObject_iface, fEnterMode);
}

static HRESULT WINAPI DocObjOleInPlaceObjectWindowless_InPlaceDeactivate(IOleInPlaceObjectWindowless *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceObjectWindowless(iface);

    TRACE("(%p)\n", This);

    if(This->ui_active)
        IOleDocumentView_UIActivate(&This->IOleDocumentView_iface, FALSE);
    This->window_active = FALSE;

    if(!This->in_place_active)
        return S_OK;

    unlink_ref(&This->frame);
    if(This->hwnd) {
        ShowWindow(This->hwnd, SW_HIDE);
        SetWindowPos(This->hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
    }

    This->focus = FALSE;
    notif_focus(This);

    This->in_place_active = FALSE;
    if(This->ipsite) {
        IOleInPlaceSiteEx *ipsiteex;
        HRESULT hres;

        hres = IOleInPlaceSite_QueryInterface(This->ipsite, &IID_IOleInPlaceSiteEx, (void**)&ipsiteex);
        if(SUCCEEDED(hres)) {
            IOleInPlaceSiteEx_OnInPlaceDeactivateEx(ipsiteex, TRUE);
            IOleInPlaceSiteEx_Release(ipsiteex);
        }else {
            IOleInPlaceSite_OnInPlaceDeactivate(This->ipsite);
        }
    }

    return S_OK;
}

static HRESULT WINAPI DocObjOleInPlaceObjectWindowless_UIDeactivate(IOleInPlaceObjectWindowless *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleInPlaceObjectWindowless_SetObjectRects(IOleInPlaceObjectWindowless *iface,
        const RECT *pos, const RECT *clip)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceObjectWindowless(iface);
    RECT r;

    TRACE("(%p)->(%s %s)\n", This, wine_dbgstr_rect(pos), wine_dbgstr_rect(clip));

    if(clip && !EqualRect(clip, pos))
        FIXME("Ignoring clip rect %s\n", wine_dbgstr_rect(clip));

    r = *pos;
    return IOleDocumentView_SetRect(&This->IOleDocumentView_iface, &r);
}

static HRESULT WINAPI DocObjOleInPlaceObjectWindowless_ReactivateAndUndo(IOleInPlaceObjectWindowless *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleInPlaceObjectWindowless_OnWindowMessage(IOleInPlaceObjectWindowless *iface,
        UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *lpResult)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)->(%u %Iu %Iu %p)\n", This, msg, wParam, lParam, lpResult);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleInPlaceObjectWindowless_GetDropTarget(IOleInPlaceObjectWindowless *iface,
        IDropTarget **ppDropTarget)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)->(%p)\n", This, ppDropTarget);
    return E_NOTIMPL;
}

static const IOleInPlaceObjectWindowlessVtbl DocObjOleInPlaceObjectWindowlessVtbl = {
    DocObjOleInPlaceObjectWindowless_QueryInterface,
    DocObjOleInPlaceObjectWindowless_AddRef,
    DocObjOleInPlaceObjectWindowless_Release,
    DocObjOleInPlaceObjectWindowless_GetWindow,
    DocObjOleInPlaceObjectWindowless_ContextSensitiveHelp,
    DocObjOleInPlaceObjectWindowless_InPlaceDeactivate,
    DocObjOleInPlaceObjectWindowless_UIDeactivate,
    DocObjOleInPlaceObjectWindowless_SetObjectRects,
    DocObjOleInPlaceObjectWindowless_ReactivateAndUndo,
    DocObjOleInPlaceObjectWindowless_OnWindowMessage,
    DocObjOleInPlaceObjectWindowless_GetDropTarget
};

/**********************************************************
 * IObjectWithSite implementation
 */

static inline HTMLDocumentNode *HTMLDocumentNode_from_IObjectWithSite(IObjectWithSite *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IObjectWithSite_iface);
}

static HRESULT WINAPI DocNodeObjectWithSite_QueryInterface(IObjectWithSite *iface, REFIID riid, void **ppv)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IObjectWithSite(iface);
    return IHTMLDOMNode_QueryInterface(&This->node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI DocNodeObjectWithSite_AddRef(IObjectWithSite *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IObjectWithSite(iface);
    return IHTMLDOMNode_AddRef(&This->node.IHTMLDOMNode_iface);
}

static ULONG WINAPI DocNodeObjectWithSite_Release(IObjectWithSite *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IObjectWithSite(iface);
    return IHTMLDOMNode_Release(&This->node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI DocNodeObjectWithSite_SetSite(IObjectWithSite *iface, IUnknown *pUnkSite)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IObjectWithSite(iface);
    FIXME("(%p)->(%p)\n", This, pUnkSite);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeObjectWithSite_GetSite(IObjectWithSite* iface, REFIID riid, PVOID *ppvSite)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IObjectWithSite(iface);
    FIXME("(%p)->(%p)\n", This, ppvSite);
    return E_NOTIMPL;
}

static const IObjectWithSiteVtbl DocNodeObjectWithSiteVtbl = {
    DocNodeObjectWithSite_QueryInterface,
    DocNodeObjectWithSite_AddRef,
    DocNodeObjectWithSite_Release,
    DocNodeObjectWithSite_SetSite,
    DocNodeObjectWithSite_GetSite
};

static inline HTMLDocumentObj *HTMLDocumentObj_from_IObjectWithSite(IObjectWithSite *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IObjectWithSite_iface);
}

static HRESULT WINAPI DocObjObjectWithSite_QueryInterface(IObjectWithSite *iface, REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IObjectWithSite(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI DocObjObjectWithSite_AddRef(IObjectWithSite *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IObjectWithSite(iface);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI DocObjObjectWithSite_Release(IObjectWithSite *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IObjectWithSite(iface);
    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI DocObjObjectWithSite_SetSite(IObjectWithSite *iface, IUnknown *pUnkSite)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IObjectWithSite(iface);
    FIXME("(%p)->(%p)\n", This, pUnkSite);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjObjectWithSite_GetSite(IObjectWithSite* iface, REFIID riid, PVOID *ppvSite)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IObjectWithSite(iface);
    FIXME("(%p)->(%p)\n", This, ppvSite);
    return E_NOTIMPL;
}

static const IObjectWithSiteVtbl DocObjObjectWithSiteVtbl = {
    DocObjObjectWithSite_QueryInterface,
    DocObjObjectWithSite_AddRef,
    DocObjObjectWithSite_Release,
    DocObjObjectWithSite_SetSite,
    DocObjObjectWithSite_GetSite
};

/**********************************************************
 * IOleContainer implementation
 */

static inline HTMLDocumentNode *HTMLDocumentNode_from_IOleContainer(IOleContainer *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IOleContainer_iface);
}

static HRESULT WINAPI DocNodeOleContainer_QueryInterface(IOleContainer *iface, REFIID riid, void **ppv)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleContainer(iface);
    return IHTMLDOMNode_QueryInterface(&This->node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI DocNodeOleContainer_AddRef(IOleContainer *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleContainer(iface);
    return IHTMLDOMNode_AddRef(&This->node.IHTMLDOMNode_iface);
}

static ULONG WINAPI DocNodeOleContainer_Release(IOleContainer *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleContainer(iface);
    return IHTMLDOMNode_Release(&This->node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI DocNodeOleContainer_ParseDisplayName(IOleContainer *iface, IBindCtx *pbc, LPOLESTR pszDisplayName,
        ULONG *pchEaten, IMoniker **ppmkOut)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleContainer(iface);
    FIXME("(%p)->(%p %s %p %p)\n", This, pbc, debugstr_w(pszDisplayName), pchEaten, ppmkOut);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeOleContainer_EnumObjects(IOleContainer *iface, DWORD grfFlags, IEnumUnknown **ppenum)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleContainer(iface);
    return IOleContainer_EnumObjects(&This->doc_obj->IOleContainer_iface, grfFlags, ppenum);
}

static HRESULT WINAPI DocNodeOleContainer_LockContainer(IOleContainer *iface, BOOL fLock)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IOleContainer(iface);
    FIXME("(%p)->(%x)\n", This, fLock);
    return E_NOTIMPL;
}

static const IOleContainerVtbl DocNodeOleContainerVtbl = {
    DocNodeOleContainer_QueryInterface,
    DocNodeOleContainer_AddRef,
    DocNodeOleContainer_Release,
    DocNodeOleContainer_ParseDisplayName,
    DocNodeOleContainer_EnumObjects,
    DocNodeOleContainer_LockContainer
};

static inline HTMLDocumentObj *HTMLDocumentObj_from_IOleContainer(IOleContainer *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IOleContainer_iface);
}

static HRESULT WINAPI DocObjOleContainer_QueryInterface(IOleContainer *iface, REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleContainer(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI DocObjOleContainer_AddRef(IOleContainer *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleContainer(iface);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI DocObjOleContainer_Release(IOleContainer *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleContainer(iface);
    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI DocObjOleContainer_ParseDisplayName(IOleContainer *iface, IBindCtx *pbc, LPOLESTR pszDisplayName,
        ULONG *pchEaten, IMoniker **ppmkOut)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleContainer(iface);
    FIXME("(%p)->(%p %s %p %p)\n", This, pbc, debugstr_w(pszDisplayName), pchEaten, ppmkOut);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjOleContainer_EnumObjects(IOleContainer *iface, DWORD grfFlags, IEnumUnknown **ppenum)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleContainer(iface);
    EnumUnknown *ret;

    TRACE("(%p)->(%lx %p)\n", This, grfFlags, ppenum);

    ret = malloc(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IEnumUnknown_iface.lpVtbl = &EnumUnknownVtbl;
    ret->ref = 1;

    *ppenum = &ret->IEnumUnknown_iface;
    return S_OK;
}

static HRESULT WINAPI DocObjOleContainer_LockContainer(IOleContainer *iface, BOOL fLock)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IOleContainer(iface);
    FIXME("(%p)->(%x)\n", This, fLock);
    return E_NOTIMPL;
}

static const IOleContainerVtbl DocObjOleContainerVtbl = {
    DocObjOleContainer_QueryInterface,
    DocObjOleContainer_AddRef,
    DocObjOleContainer_Release,
    DocObjOleContainer_ParseDisplayName,
    DocObjOleContainer_EnumObjects,
    DocObjOleContainer_LockContainer
};

static inline HTMLDocumentObj *impl_from_ITargetContainer(ITargetContainer *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, ITargetContainer_iface);
}

static HRESULT WINAPI TargetContainer_QueryInterface(ITargetContainer *iface, REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = impl_from_ITargetContainer(iface);
    return ICustomDoc_QueryInterface(&This->ICustomDoc_iface, riid, ppv);
}

static ULONG WINAPI TargetContainer_AddRef(ITargetContainer *iface)
{
    HTMLDocumentObj *This = impl_from_ITargetContainer(iface);
    return ICustomDoc_AddRef(&This->ICustomDoc_iface);
}

static ULONG WINAPI TargetContainer_Release(ITargetContainer *iface)
{
    HTMLDocumentObj *This = impl_from_ITargetContainer(iface);
    return ICustomDoc_Release(&This->ICustomDoc_iface);
}

static HRESULT WINAPI TargetContainer_GetFrameUrl(ITargetContainer *iface, LPWSTR *ppszFrameSrc)
{
    HTMLDocumentObj *This = impl_from_ITargetContainer(iface);
    FIXME("(%p)->(%p)\n", This, ppszFrameSrc);
    return E_NOTIMPL;
}

static HRESULT WINAPI TargetContainer_GetFramesContainer(ITargetContainer *iface, IOleContainer **ppContainer)
{
    HTMLDocumentObj *This = impl_from_ITargetContainer(iface);

    TRACE("(%p)->(%p)\n", This, ppContainer);

    /* NOTE: we should return wrapped interface here */
    IOleContainer_AddRef(&This->IOleContainer_iface);
    *ppContainer = &This->IOleContainer_iface;
    return S_OK;
}

static const ITargetContainerVtbl TargetContainerVtbl = {
    TargetContainer_QueryInterface,
    TargetContainer_AddRef,
    TargetContainer_Release,
    TargetContainer_GetFrameUrl,
    TargetContainer_GetFramesContainer
};

void TargetContainer_Init(HTMLDocumentObj *This)
{
    This->ITargetContainer_iface.lpVtbl = &TargetContainerVtbl;
}

/**********************************************************
 * IObjectSafety implementation
 */

static inline HTMLDocumentNode *HTMLDocumentNode_from_IObjectSafety(IObjectSafety *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentNode, IObjectSafety_iface);
}

static HRESULT WINAPI DocNodeObjectSafety_QueryInterface(IObjectSafety *iface, REFIID riid, void **ppv)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IObjectSafety(iface);
    return IHTMLDOMNode_QueryInterface(&This->node.IHTMLDOMNode_iface, riid, ppv);
}

static ULONG WINAPI DocNodeObjectSafety_AddRef(IObjectSafety *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IObjectSafety(iface);
    return IHTMLDOMNode_AddRef(&This->node.IHTMLDOMNode_iface);
}

static ULONG WINAPI DocNodeObjectSafety_Release(IObjectSafety *iface)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IObjectSafety(iface);
    return IHTMLDOMNode_Release(&This->node.IHTMLDOMNode_iface);
}

static HRESULT WINAPI DocNodeObjectSafety_GetInterfaceSafetyOptions(IObjectSafety *iface,
        REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IObjectSafety(iface);
    FIXME("(%p)->(%s %p %p)\n", This, debugstr_guid(riid), pdwSupportedOptions, pdwEnabledOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocNodeObjectSafety_SetInterfaceSafetyOptions(IObjectSafety *iface,
        REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions)
{
    HTMLDocumentNode *This = HTMLDocumentNode_from_IObjectSafety(iface);
    return IObjectSafety_SetInterfaceSafetyOptions(&This->doc_obj->IObjectSafety_iface,
                                                   riid, dwOptionSetMask, dwEnabledOptions);
}

static const IObjectSafetyVtbl DocNodeObjectSafetyVtbl = {
    DocNodeObjectSafety_QueryInterface,
    DocNodeObjectSafety_AddRef,
    DocNodeObjectSafety_Release,
    DocNodeObjectSafety_GetInterfaceSafetyOptions,
    DocNodeObjectSafety_SetInterfaceSafetyOptions
};

static inline HTMLDocumentObj *HTMLDocumentObj_from_IObjectSafety(IObjectSafety *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IObjectSafety_iface);
}

static HRESULT WINAPI DocObjObjectSafety_QueryInterface(IObjectSafety *iface, REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IObjectSafety(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI DocObjObjectSafety_AddRef(IObjectSafety *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IObjectSafety(iface);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI DocObjObjectSafety_Release(IObjectSafety *iface)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IObjectSafety(iface);
    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI DocObjObjectSafety_GetInterfaceSafetyOptions(IObjectSafety *iface,
        REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IObjectSafety(iface);
    FIXME("(%p)->(%s %p %p)\n", This, debugstr_guid(riid), pdwSupportedOptions, pdwEnabledOptions);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjObjectSafety_SetInterfaceSafetyOptions(IObjectSafety *iface,
        REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions)
{
    HTMLDocumentObj *This = HTMLDocumentObj_from_IObjectSafety(iface);
    FIXME("(%p)->(%s %lx %lx)\n", This, debugstr_guid(riid), dwOptionSetMask, dwEnabledOptions);

    if(IsEqualGUID(&IID_IPersistMoniker, riid) &&
            dwOptionSetMask==INTERFACESAFE_FOR_UNTRUSTED_DATA &&
            dwEnabledOptions==INTERFACESAFE_FOR_UNTRUSTED_DATA)
        return S_OK;

    return E_NOTIMPL;
}

static const IObjectSafetyVtbl DocObjObjectSafetyVtbl = {
    DocObjObjectSafety_QueryInterface,
    DocObjObjectSafety_AddRef,
    DocObjObjectSafety_Release,
    DocObjObjectSafety_GetInterfaceSafetyOptions,
    DocObjObjectSafety_SetInterfaceSafetyOptions
};

void HTMLDocument_LockContainer(HTMLDocumentObj *This, BOOL fLock)
{
    IOleContainer *container;
    HRESULT hres;

    if(!This->client || This->container_locked == fLock)
        return;

    hres = IOleClientSite_GetContainer(This->client, &container);
    if(SUCCEEDED(hres)) {
        IOleContainer_LockContainer(container, fLock);
        This->container_locked = fLock;
        IOleContainer_Release(container);
    }
}

void HTMLDocumentNode_OleObj_Init(HTMLDocumentNode *This)
{
    This->IOleObject_iface.lpVtbl = &DocNodeOleObjectVtbl;
    This->IOleDocument_iface.lpVtbl = &DocNodeOleDocumentVtbl;
    This->IOleControl_iface.lpVtbl = &DocNodeOleControlVtbl;
    This->IOleInPlaceActiveObject_iface.lpVtbl = &DocNodeOleInPlaceActiveObjectVtbl;
    This->IOleInPlaceObjectWindowless_iface.lpVtbl = &DocNodeOleInPlaceObjectWindowlessVtbl;
    This->IObjectWithSite_iface.lpVtbl = &DocNodeObjectWithSiteVtbl;
    This->IOleContainer_iface.lpVtbl = &DocNodeOleContainerVtbl;
    This->IObjectSafety_iface.lpVtbl = &DocNodeObjectSafetyVtbl;
    if(This->doc_obj) {
        This->doc_obj->extent.cx = 1;
        This->doc_obj->extent.cy = 1;
    }
}

static void HTMLDocumentObj_OleObj_Init(HTMLDocumentObj *This)
{
    This->IOleObject_iface.lpVtbl = &DocObjOleObjectVtbl;
    This->IOleDocument_iface.lpVtbl = &DocObjOleDocumentVtbl;
    This->IOleControl_iface.lpVtbl = &DocObjOleControlVtbl;
    This->IOleInPlaceActiveObject_iface.lpVtbl = &DocObjOleInPlaceActiveObjectVtbl;
    This->IOleInPlaceObjectWindowless_iface.lpVtbl = &DocObjOleInPlaceObjectWindowlessVtbl;
    This->IObjectWithSite_iface.lpVtbl = &DocObjObjectWithSiteVtbl;
    This->IOleContainer_iface.lpVtbl = &DocObjOleContainerVtbl;
    This->IObjectSafety_iface.lpVtbl = &DocObjObjectSafetyVtbl;
    This->extent.cx = 1;
    This->extent.cy = 1;
}

#define HTMLDOCUMENTOBJ_IUNKNOWN_METHODS(iface) \
static HRESULT WINAPI DocObj##iface##_QueryInterface(I##iface *_0, REFIID riid, void **ppv) \
{ \
    HTMLDocumentObj *This = CONTAINING_RECORD(_0, HTMLDocumentObj, I##iface##_iface); \
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv); \
} \
static ULONG WINAPI DocObj##iface##_AddRef(I##iface *_0) \
{ \
    HTMLDocumentObj *This = CONTAINING_RECORD(_0, HTMLDocumentObj, I##iface##_iface); \
    return IUnknown_AddRef(This->outer_unk); \
} \
static ULONG WINAPI DocObj##iface##_Release(I##iface *_0) \
{ \
    HTMLDocumentObj *This = CONTAINING_RECORD(_0, HTMLDocumentObj, I##iface##_iface); \
    return IUnknown_Release(This->outer_unk); \
}

#define HTMLDOCUMENTOBJ_IDISPATCH_METHODS(iface) HTMLDOCUMENTOBJ_IUNKNOWN_METHODS(iface) \
static HRESULT WINAPI DocObj##iface##_GetTypeInfoCount(I##iface *_0, UINT *pctinfo) \
{ \
    HTMLDocumentObj *This = CONTAINING_RECORD(_0, HTMLDocumentObj, I##iface##_iface); \
    return IDispatchEx_GetTypeInfoCount(&This->IDispatchEx_iface, pctinfo); \
} \
static HRESULT WINAPI DocObj##iface##_GetTypeInfo(I##iface *_0, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo) \
{ \
    HTMLDocumentObj *This = CONTAINING_RECORD(_0, HTMLDocumentObj, I##iface##_iface); \
    return IDispatchEx_GetTypeInfo(&This->IDispatchEx_iface, iTInfo, lcid, ppTInfo); \
} \
static HRESULT WINAPI DocObj##iface##_GetIDsOfNames(I##iface *_0, REFIID riid, LPOLESTR *rgszNames, UINT cNames, \
        LCID lcid, DISPID *rgDispId) \
{ \
    HTMLDocumentObj *This = CONTAINING_RECORD(_0, HTMLDocumentObj, I##iface##_iface); \
    return IDispatchEx_GetIDsOfNames(&This->IDispatchEx_iface, riid, rgszNames, cNames, lcid, rgDispId); \
} \
static HRESULT WINAPI DocObj##iface##_Invoke(I##iface *_0, DISPID dispIdMember, REFIID riid, LCID lcid, \
        WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr) \
{ \
    HTMLDocumentObj *This = CONTAINING_RECORD(_0, HTMLDocumentObj, I##iface##_iface); \
    return IDispatchEx_Invoke(&This->IDispatchEx_iface, dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr); \
}

#define HTMLDOCUMENTOBJ_FWD_TO_NODE_0(iface, method) static HRESULT WINAPI DocObj##iface##_##method(I##iface *_0) \
{ \
    HTMLDocumentObj *This = CONTAINING_RECORD(_0, HTMLDocumentObj, I##iface##_iface); \
    return This->doc_node->I##iface##_iface.lpVtbl->method(&This->doc_node->I##iface##_iface); \
}

#define HTMLDOCUMENTOBJ_FWD_TO_NODE_1(iface, method, a) static HRESULT WINAPI DocObj##iface##_##method(I##iface *_0, a _1) \
{ \
    HTMLDocumentObj *This = CONTAINING_RECORD(_0, HTMLDocumentObj, I##iface##_iface); \
    return This->doc_node->I##iface##_iface.lpVtbl->method(&This->doc_node->I##iface##_iface, _1); \
}

#define HTMLDOCUMENTOBJ_FWD_TO_NODE_2(iface, method, a,b) static HRESULT WINAPI DocObj##iface##_##method(I##iface *_0, a _1, b _2) \
{ \
    HTMLDocumentObj *This = CONTAINING_RECORD(_0, HTMLDocumentObj, I##iface##_iface); \
    return This->doc_node->I##iface##_iface.lpVtbl->method(&This->doc_node->I##iface##_iface, _1, _2); \
}

#define HTMLDOCUMENTOBJ_FWD_TO_NODE_3(iface, method, a,b,c) static HRESULT WINAPI DocObj##iface##_##method(I##iface *_0, a _1, b _2, c _3) \
{ \
    HTMLDocumentObj *This = CONTAINING_RECORD(_0, HTMLDocumentObj, I##iface##_iface); \
    return This->doc_node->I##iface##_iface.lpVtbl->method(&This->doc_node->I##iface##_iface, _1, _2, _3); \
}

#define HTMLDOCUMENTOBJ_FWD_TO_NODE_4(iface, method, a,b,c,d) static HRESULT WINAPI DocObj##iface##_##method(I##iface *_0, a _1, b _2, c _3, d _4) \
{ \
    HTMLDocumentObj *This = CONTAINING_RECORD(_0, HTMLDocumentObj, I##iface##_iface); \
    return This->doc_node->I##iface##_iface.lpVtbl->method(&This->doc_node->I##iface##_iface, _1, _2, _3, _4); \
}

#define HTMLDOCUMENTOBJ_FWD_TO_NODE_5(iface, method, a,b,c,d,e) static HRESULT WINAPI DocObj##iface##_##method(I##iface *_0, a _1, b _2, c _3, d _4, e _5) \
{ \
    HTMLDocumentObj *This = CONTAINING_RECORD(_0, HTMLDocumentObj, I##iface##_iface); \
    return This->doc_node->I##iface##_iface.lpVtbl->method(&This->doc_node->I##iface##_iface, _1, _2, _3, _4, _5); \
}

/**********************************************************
 * IHTMLDocument2 implementation
 */
static inline HTMLDocumentObj *impl_from_IHTMLDocument2(IHTMLDocument2 *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IHTMLDocument2_iface);
}

HTMLDOCUMENTOBJ_IDISPATCH_METHODS(HTMLDocument2)

static HRESULT WINAPI DocObjHTMLDocument2_get_Script(IHTMLDocument2 *iface, IDispatch **p)
{
    HTMLDocumentObj *This = impl_from_IHTMLDocument2(iface);
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    hres = IHTMLDocument7_get_parentWindow(&This->IHTMLDocument7_iface, (IHTMLWindow2**)p);
    return hres == S_OK && !*p ? E_PENDING : hres;
}

HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_all, IHTMLElementCollection**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_body, IHTMLElement**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_activeElement, IHTMLElement**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_images, IHTMLElementCollection**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_applets, IHTMLElementCollection**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_links, IHTMLElementCollection**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_forms, IHTMLElementCollection**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_anchors, IHTMLElementCollection**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_title, BSTR)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_title, BSTR*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_scripts, IHTMLElementCollection**)

static HRESULT WINAPI DocObjHTMLDocument2_put_designMode(IHTMLDocument2 *iface, BSTR v)
{
    HTMLDocumentObj *This = impl_from_IHTMLDocument2(iface);
    HRESULT hres;

    TRACE("(%p)->(%s)\n", This, debugstr_w(v));

    if(wcsicmp(v, L"on")) {
        FIXME("Unsupported arg %s\n", debugstr_w(v));
        return E_NOTIMPL;
    }

    hres = setup_edit_mode(This);
    if(FAILED(hres))
        return hres;

    call_property_onchanged(&This->cp_container, DISPID_IHTMLDOCUMENT2_DESIGNMODE);
    return S_OK;
}

static HRESULT WINAPI DocObjHTMLDocument2_get_designMode(IHTMLDocument2 *iface, BSTR *p)
{
    HTMLDocumentObj *This = impl_from_IHTMLDocument2(iface);

    FIXME("(%p)->(%p) always returning Off\n", This, p);

    if(!p)
        return E_INVALIDARG;

    *p = SysAllocString(L"Off");
    return S_OK;
}

HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_selection, IHTMLSelectionObject**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_readyState, BSTR*)

static HRESULT WINAPI DocObjHTMLDocument2_get_frames(IHTMLDocument2 *iface, IHTMLFramesCollection2 **p)
{
    HTMLDocumentObj *This = impl_from_IHTMLDocument2(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(!This->window) {
        /* Not implemented by IE */
        return E_NOTIMPL;
    }
    return IHTMLWindow2_get_frames(&This->window->base.IHTMLWindow2_iface, p);
}

HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_embeds, IHTMLElementCollection**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_plugins, IHTMLElementCollection**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_alinkColor, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_alinkColor, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_bgColor, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_bgColor, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_fgColor, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_fgColor, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_linkColor, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_linkColor, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_vlinkColor, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_vlinkColor, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_referrer, BSTR*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_location, IHTMLLocation**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_lastModified, BSTR*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_URL, BSTR)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_URL, BSTR*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_domain, BSTR)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_domain, BSTR*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_cookie, BSTR)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_cookie, BSTR*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_expando, VARIANT_BOOL)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_expando, VARIANT_BOOL*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_charset, BSTR)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_charset, BSTR*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_defaultCharset, BSTR)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_defaultCharset, BSTR*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_mimeType, BSTR*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_fileSize, BSTR*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_fileCreatedDate, BSTR*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_fileModifiedDate, BSTR*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_fileUpdatedDate, BSTR*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_security, BSTR*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_protocol, BSTR*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_nameProp, BSTR*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, write, SAFEARRAY*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, writeln, SAFEARRAY*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_5(HTMLDocument2, open, BSTR,VARIANT,VARIANT,VARIANT,IDispatch**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_0(HTMLDocument2, close)
HTMLDOCUMENTOBJ_FWD_TO_NODE_0(HTMLDocument2, clear)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(HTMLDocument2, queryCommandSupported, BSTR,VARIANT_BOOL*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(HTMLDocument2, queryCommandEnabled, BSTR,VARIANT_BOOL*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(HTMLDocument2, queryCommandState, BSTR,VARIANT_BOOL*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(HTMLDocument2, queryCommandIndeterm, BSTR,VARIANT_BOOL*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(HTMLDocument2, queryCommandText, BSTR,BSTR*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(HTMLDocument2, queryCommandValue, BSTR,VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_4(HTMLDocument2, execCommand, BSTR,VARIANT_BOOL,VARIANT,VARIANT_BOOL*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(HTMLDocument2, execCommandShowHelp, BSTR,VARIANT_BOOL*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(HTMLDocument2, createElement, BSTR,IHTMLElement**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_onhelp, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_onhelp, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_onclick, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_onclick, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_ondblclick, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_ondblclick, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_onkeyup, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_onkeyup, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_onkeydown, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_onkeydown, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_onkeypress, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_onkeypress, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_onmouseup, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_onmouseup, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_onmousedown, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_onmousedown, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_onmousemove, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_onmousemove, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_onmouseout, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_onmouseout, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_onmouseover, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_onmouseover, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_onreadystatechange, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_onreadystatechange, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_onafterupdate, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_onafterupdate, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_onrowexit, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_onrowexit, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_onrowenter, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_onrowenter, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_ondragstart, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_ondragstart, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_onselectstart, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_onselectstart, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_3(HTMLDocument2, elementFromPoint, LONG,LONG,IHTMLElement**)

static HRESULT WINAPI DocObjHTMLDocument2_get_parentWindow(IHTMLDocument2 *iface, IHTMLWindow2 **p)
{
    HTMLDocumentObj *This = impl_from_IHTMLDocument2(iface);
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, p);

    hres = IHTMLDocument7_get_defaultView(&This->IHTMLDocument7_iface, p);
    return hres == S_OK && !*p ? E_FAIL : hres;
}

HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_styleSheets, IHTMLStyleSheetsCollection**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_onbeforeupdate, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_onbeforeupdate, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, put_onerrorupdate, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, get_onerrorupdate, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument2, toString, BSTR*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_3(HTMLDocument2, createStyleSheet, BSTR,LONG,IHTMLStyleSheet**)

static const IHTMLDocument2Vtbl DocObjHTMLDocument2Vtbl = {
    DocObjHTMLDocument2_QueryInterface,
    DocObjHTMLDocument2_AddRef,
    DocObjHTMLDocument2_Release,
    DocObjHTMLDocument2_GetTypeInfoCount,
    DocObjHTMLDocument2_GetTypeInfo,
    DocObjHTMLDocument2_GetIDsOfNames,
    DocObjHTMLDocument2_Invoke,
    DocObjHTMLDocument2_get_Script,
    DocObjHTMLDocument2_get_all,
    DocObjHTMLDocument2_get_body,
    DocObjHTMLDocument2_get_activeElement,
    DocObjHTMLDocument2_get_images,
    DocObjHTMLDocument2_get_applets,
    DocObjHTMLDocument2_get_links,
    DocObjHTMLDocument2_get_forms,
    DocObjHTMLDocument2_get_anchors,
    DocObjHTMLDocument2_put_title,
    DocObjHTMLDocument2_get_title,
    DocObjHTMLDocument2_get_scripts,
    DocObjHTMLDocument2_put_designMode,
    DocObjHTMLDocument2_get_designMode,
    DocObjHTMLDocument2_get_selection,
    DocObjHTMLDocument2_get_readyState,
    DocObjHTMLDocument2_get_frames,
    DocObjHTMLDocument2_get_embeds,
    DocObjHTMLDocument2_get_plugins,
    DocObjHTMLDocument2_put_alinkColor,
    DocObjHTMLDocument2_get_alinkColor,
    DocObjHTMLDocument2_put_bgColor,
    DocObjHTMLDocument2_get_bgColor,
    DocObjHTMLDocument2_put_fgColor,
    DocObjHTMLDocument2_get_fgColor,
    DocObjHTMLDocument2_put_linkColor,
    DocObjHTMLDocument2_get_linkColor,
    DocObjHTMLDocument2_put_vlinkColor,
    DocObjHTMLDocument2_get_vlinkColor,
    DocObjHTMLDocument2_get_referrer,
    DocObjHTMLDocument2_get_location,
    DocObjHTMLDocument2_get_lastModified,
    DocObjHTMLDocument2_put_URL,
    DocObjHTMLDocument2_get_URL,
    DocObjHTMLDocument2_put_domain,
    DocObjHTMLDocument2_get_domain,
    DocObjHTMLDocument2_put_cookie,
    DocObjHTMLDocument2_get_cookie,
    DocObjHTMLDocument2_put_expando,
    DocObjHTMLDocument2_get_expando,
    DocObjHTMLDocument2_put_charset,
    DocObjHTMLDocument2_get_charset,
    DocObjHTMLDocument2_put_defaultCharset,
    DocObjHTMLDocument2_get_defaultCharset,
    DocObjHTMLDocument2_get_mimeType,
    DocObjHTMLDocument2_get_fileSize,
    DocObjHTMLDocument2_get_fileCreatedDate,
    DocObjHTMLDocument2_get_fileModifiedDate,
    DocObjHTMLDocument2_get_fileUpdatedDate,
    DocObjHTMLDocument2_get_security,
    DocObjHTMLDocument2_get_protocol,
    DocObjHTMLDocument2_get_nameProp,
    DocObjHTMLDocument2_write,
    DocObjHTMLDocument2_writeln,
    DocObjHTMLDocument2_open,
    DocObjHTMLDocument2_close,
    DocObjHTMLDocument2_clear,
    DocObjHTMLDocument2_queryCommandSupported,
    DocObjHTMLDocument2_queryCommandEnabled,
    DocObjHTMLDocument2_queryCommandState,
    DocObjHTMLDocument2_queryCommandIndeterm,
    DocObjHTMLDocument2_queryCommandText,
    DocObjHTMLDocument2_queryCommandValue,
    DocObjHTMLDocument2_execCommand,
    DocObjHTMLDocument2_execCommandShowHelp,
    DocObjHTMLDocument2_createElement,
    DocObjHTMLDocument2_put_onhelp,
    DocObjHTMLDocument2_get_onhelp,
    DocObjHTMLDocument2_put_onclick,
    DocObjHTMLDocument2_get_onclick,
    DocObjHTMLDocument2_put_ondblclick,
    DocObjHTMLDocument2_get_ondblclick,
    DocObjHTMLDocument2_put_onkeyup,
    DocObjHTMLDocument2_get_onkeyup,
    DocObjHTMLDocument2_put_onkeydown,
    DocObjHTMLDocument2_get_onkeydown,
    DocObjHTMLDocument2_put_onkeypress,
    DocObjHTMLDocument2_get_onkeypress,
    DocObjHTMLDocument2_put_onmouseup,
    DocObjHTMLDocument2_get_onmouseup,
    DocObjHTMLDocument2_put_onmousedown,
    DocObjHTMLDocument2_get_onmousedown,
    DocObjHTMLDocument2_put_onmousemove,
    DocObjHTMLDocument2_get_onmousemove,
    DocObjHTMLDocument2_put_onmouseout,
    DocObjHTMLDocument2_get_onmouseout,
    DocObjHTMLDocument2_put_onmouseover,
    DocObjHTMLDocument2_get_onmouseover,
    DocObjHTMLDocument2_put_onreadystatechange,
    DocObjHTMLDocument2_get_onreadystatechange,
    DocObjHTMLDocument2_put_onafterupdate,
    DocObjHTMLDocument2_get_onafterupdate,
    DocObjHTMLDocument2_put_onrowexit,
    DocObjHTMLDocument2_get_onrowexit,
    DocObjHTMLDocument2_put_onrowenter,
    DocObjHTMLDocument2_get_onrowenter,
    DocObjHTMLDocument2_put_ondragstart,
    DocObjHTMLDocument2_get_ondragstart,
    DocObjHTMLDocument2_put_onselectstart,
    DocObjHTMLDocument2_get_onselectstart,
    DocObjHTMLDocument2_elementFromPoint,
    DocObjHTMLDocument2_get_parentWindow,
    DocObjHTMLDocument2_get_styleSheets,
    DocObjHTMLDocument2_put_onbeforeupdate,
    DocObjHTMLDocument2_get_onbeforeupdate,
    DocObjHTMLDocument2_put_onerrorupdate,
    DocObjHTMLDocument2_get_onerrorupdate,
    DocObjHTMLDocument2_toString,
    DocObjHTMLDocument2_createStyleSheet
};

/**********************************************************
 * IHTMLDocument3 implementation
 */
static inline HTMLDocumentObj *impl_from_IHTMLDocument3(IHTMLDocument3 *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IHTMLDocument3_iface);
}

HTMLDOCUMENTOBJ_IDISPATCH_METHODS(HTMLDocument3)
HTMLDOCUMENTOBJ_FWD_TO_NODE_0(HTMLDocument3, releaseCapture)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, recalc, VARIANT_BOOL)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(HTMLDocument3, createTextNode, BSTR,IHTMLDOMNode**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, get_documentElement, IHTMLElement**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, get_uniqueID, BSTR*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_3(HTMLDocument3, attachEvent, BSTR,IDispatch*,VARIANT_BOOL*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(HTMLDocument3, detachEvent, BSTR,IDispatch*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, put_onrowsdelete, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, get_onrowsdelete, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, put_onrowsinserted, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, get_onrowsinserted, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, put_oncellchange, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, get_oncellchange, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, put_ondatasetchanged, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, get_ondatasetchanged, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, put_ondataavailable, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, get_ondataavailable, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, put_ondatasetcomplete, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, get_ondatasetcomplete, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, put_onpropertychange, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, get_onpropertychange, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, put_dir, BSTR)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, get_dir, BSTR*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, put_oncontextmenu, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, get_oncontextmenu, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, put_onstop, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, get_onstop, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, createDocumentFragment, IHTMLDocument2**)

static HRESULT WINAPI DocObjHTMLDocument3_get_parentDocument(IHTMLDocument3 *iface, IHTMLDocument2 **p)
{
    HTMLDocumentObj *This = impl_from_IHTMLDocument3(iface);
    FIXME("(%p)->(%p)\n", This, p);
    return E_NOTIMPL;
}

HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, put_enableDownload, VARIANT_BOOL)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, get_enableDownload, VARIANT_BOOL*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, put_baseUrl, BSTR)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, get_baseUrl, BSTR*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, get_childNodes, IDispatch**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, put_inheritStyleSheets, VARIANT_BOOL)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, get_inheritStyleSheets, VARIANT_BOOL*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, put_onbeforeeditfocus, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument3, get_onbeforeeditfocus, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(HTMLDocument3, getElementsByName, BSTR,IHTMLElementCollection**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(HTMLDocument3, getElementById, BSTR,IHTMLElement**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(HTMLDocument3, getElementsByTagName, BSTR,IHTMLElementCollection**)

static const IHTMLDocument3Vtbl DocObjHTMLDocument3Vtbl = {
    DocObjHTMLDocument3_QueryInterface,
    DocObjHTMLDocument3_AddRef,
    DocObjHTMLDocument3_Release,
    DocObjHTMLDocument3_GetTypeInfoCount,
    DocObjHTMLDocument3_GetTypeInfo,
    DocObjHTMLDocument3_GetIDsOfNames,
    DocObjHTMLDocument3_Invoke,
    DocObjHTMLDocument3_releaseCapture,
    DocObjHTMLDocument3_recalc,
    DocObjHTMLDocument3_createTextNode,
    DocObjHTMLDocument3_get_documentElement,
    DocObjHTMLDocument3_get_uniqueID,
    DocObjHTMLDocument3_attachEvent,
    DocObjHTMLDocument3_detachEvent,
    DocObjHTMLDocument3_put_onrowsdelete,
    DocObjHTMLDocument3_get_onrowsdelete,
    DocObjHTMLDocument3_put_onrowsinserted,
    DocObjHTMLDocument3_get_onrowsinserted,
    DocObjHTMLDocument3_put_oncellchange,
    DocObjHTMLDocument3_get_oncellchange,
    DocObjHTMLDocument3_put_ondatasetchanged,
    DocObjHTMLDocument3_get_ondatasetchanged,
    DocObjHTMLDocument3_put_ondataavailable,
    DocObjHTMLDocument3_get_ondataavailable,
    DocObjHTMLDocument3_put_ondatasetcomplete,
    DocObjHTMLDocument3_get_ondatasetcomplete,
    DocObjHTMLDocument3_put_onpropertychange,
    DocObjHTMLDocument3_get_onpropertychange,
    DocObjHTMLDocument3_put_dir,
    DocObjHTMLDocument3_get_dir,
    DocObjHTMLDocument3_put_oncontextmenu,
    DocObjHTMLDocument3_get_oncontextmenu,
    DocObjHTMLDocument3_put_onstop,
    DocObjHTMLDocument3_get_onstop,
    DocObjHTMLDocument3_createDocumentFragment,
    DocObjHTMLDocument3_get_parentDocument,
    DocObjHTMLDocument3_put_enableDownload,
    DocObjHTMLDocument3_get_enableDownload,
    DocObjHTMLDocument3_put_baseUrl,
    DocObjHTMLDocument3_get_baseUrl,
    DocObjHTMLDocument3_get_childNodes,
    DocObjHTMLDocument3_put_inheritStyleSheets,
    DocObjHTMLDocument3_get_inheritStyleSheets,
    DocObjHTMLDocument3_put_onbeforeeditfocus,
    DocObjHTMLDocument3_get_onbeforeeditfocus,
    DocObjHTMLDocument3_getElementsByName,
    DocObjHTMLDocument3_getElementById,
    DocObjHTMLDocument3_getElementsByTagName
};

/**********************************************************
 * IHTMLDocument4 implementation
 */
HTMLDOCUMENTOBJ_IDISPATCH_METHODS(HTMLDocument4)
HTMLDOCUMENTOBJ_FWD_TO_NODE_0(HTMLDocument4, focus)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument4, hasFocus, VARIANT_BOOL*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument4, put_onselectionchange, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument4, get_onselectionchange, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument4, get_namespaces, IDispatch**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_3(HTMLDocument4, createDocumentFromUrl, BSTR,BSTR,IHTMLDocument2**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument4, put_media, BSTR)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument4, get_media, BSTR*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(HTMLDocument4, createEventObject, VARIANT*,IHTMLEventObj**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_3(HTMLDocument4, fireEvent, BSTR,VARIANT*,VARIANT_BOOL*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(HTMLDocument4, createRenderStyle, BSTR,IHTMLRenderStyle**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument4, put_oncontrolselect, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument4, get_oncontrolselect, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument4, get_URLUnencoded, BSTR*)

static const IHTMLDocument4Vtbl DocObjHTMLDocument4Vtbl = {
    DocObjHTMLDocument4_QueryInterface,
    DocObjHTMLDocument4_AddRef,
    DocObjHTMLDocument4_Release,
    DocObjHTMLDocument4_GetTypeInfoCount,
    DocObjHTMLDocument4_GetTypeInfo,
    DocObjHTMLDocument4_GetIDsOfNames,
    DocObjHTMLDocument4_Invoke,
    DocObjHTMLDocument4_focus,
    DocObjHTMLDocument4_hasFocus,
    DocObjHTMLDocument4_put_onselectionchange,
    DocObjHTMLDocument4_get_onselectionchange,
    DocObjHTMLDocument4_get_namespaces,
    DocObjHTMLDocument4_createDocumentFromUrl,
    DocObjHTMLDocument4_put_media,
    DocObjHTMLDocument4_get_media,
    DocObjHTMLDocument4_createEventObject,
    DocObjHTMLDocument4_fireEvent,
    DocObjHTMLDocument4_createRenderStyle,
    DocObjHTMLDocument4_put_oncontrolselect,
    DocObjHTMLDocument4_get_oncontrolselect,
    DocObjHTMLDocument4_get_URLUnencoded
};

/**********************************************************
 * IHTMLDocument5 implementation
 */
HTMLDOCUMENTOBJ_IDISPATCH_METHODS(HTMLDocument5)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument5, put_onmousewheel, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument5, get_onmousewheel, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument5, get_doctype, IHTMLDOMNode**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument5, get_implementation, IHTMLDOMImplementation**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(HTMLDocument5, createAttribute, BSTR,IHTMLDOMAttribute**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(HTMLDocument5, createComment, BSTR,IHTMLDOMNode**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument5, put_onfocusin, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument5, get_onfocusin, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument5, put_onfocusout, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument5, get_onfocusout, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument5, put_onactivate, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument5, get_onactivate, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument5, put_ondeactivate, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument5, get_ondeactivate, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument5, put_onbeforeactivate, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument5, get_onbeforeactivate, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument5, put_onbeforedeactivate, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument5, get_onbeforedeactivate, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument5, get_compatMode, BSTR*)

static const IHTMLDocument5Vtbl DocObjHTMLDocument5Vtbl = {
    DocObjHTMLDocument5_QueryInterface,
    DocObjHTMLDocument5_AddRef,
    DocObjHTMLDocument5_Release,
    DocObjHTMLDocument5_GetTypeInfoCount,
    DocObjHTMLDocument5_GetTypeInfo,
    DocObjHTMLDocument5_GetIDsOfNames,
    DocObjHTMLDocument5_Invoke,
    DocObjHTMLDocument5_put_onmousewheel,
    DocObjHTMLDocument5_get_onmousewheel,
    DocObjHTMLDocument5_get_doctype,
    DocObjHTMLDocument5_get_implementation,
    DocObjHTMLDocument5_createAttribute,
    DocObjHTMLDocument5_createComment,
    DocObjHTMLDocument5_put_onfocusin,
    DocObjHTMLDocument5_get_onfocusin,
    DocObjHTMLDocument5_put_onfocusout,
    DocObjHTMLDocument5_get_onfocusout,
    DocObjHTMLDocument5_put_onactivate,
    DocObjHTMLDocument5_get_onactivate,
    DocObjHTMLDocument5_put_ondeactivate,
    DocObjHTMLDocument5_get_ondeactivate,
    DocObjHTMLDocument5_put_onbeforeactivate,
    DocObjHTMLDocument5_get_onbeforeactivate,
    DocObjHTMLDocument5_put_onbeforedeactivate,
    DocObjHTMLDocument5_get_onbeforedeactivate,
    DocObjHTMLDocument5_get_compatMode
};

/**********************************************************
 * IHTMLDocument6 implementation
 */
HTMLDOCUMENTOBJ_IDISPATCH_METHODS(HTMLDocument6)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument6, get_compatible, IHTMLDocumentCompatibleInfoCollection**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument6, get_documentMode, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument6, put_onstorage, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument6, get_onstorage, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument6, put_onstoragecommit, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument6, get_onstoragecommit, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(HTMLDocument6, getElementById, BSTR,IHTMLElement2**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_0(HTMLDocument6, updateSettings)

static const IHTMLDocument6Vtbl DocObjHTMLDocument6Vtbl = {
    DocObjHTMLDocument6_QueryInterface,
    DocObjHTMLDocument6_AddRef,
    DocObjHTMLDocument6_Release,
    DocObjHTMLDocument6_GetTypeInfoCount,
    DocObjHTMLDocument6_GetTypeInfo,
    DocObjHTMLDocument6_GetIDsOfNames,
    DocObjHTMLDocument6_Invoke,
    DocObjHTMLDocument6_get_compatible,
    DocObjHTMLDocument6_get_documentMode,
    DocObjHTMLDocument6_put_onstorage,
    DocObjHTMLDocument6_get_onstorage,
    DocObjHTMLDocument6_put_onstoragecommit,
    DocObjHTMLDocument6_get_onstoragecommit,
    DocObjHTMLDocument6_getElementById,
    DocObjHTMLDocument6_updateSettings
};

/**********************************************************
 * IHTMLDocument7 implementation
 */
static inline HTMLDocumentObj *impl_from_IHTMLDocument7(IHTMLDocument7 *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IHTMLDocument7_iface);
}

HTMLDOCUMENTOBJ_IDISPATCH_METHODS(HTMLDocument7)

static HRESULT WINAPI DocObjHTMLDocument7_get_defaultView(IHTMLDocument7 *iface, IHTMLWindow2 **p)
{
    HTMLDocumentObj *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    if(This->window) {
        *p = &This->window->base.IHTMLWindow2_iface;
        IHTMLWindow2_AddRef(*p);
    }else {
        *p = NULL;
    }
    return S_OK;
}

HTMLDOCUMENTOBJ_FWD_TO_NODE_2(HTMLDocument7, createCDATASection, BSTR,IHTMLDOMNode**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, getSelection, IHTMLSelection**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_3(HTMLDocument7, getElementsByTagNameNS, VARIANT*,BSTR,IHTMLElementCollection**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_3(HTMLDocument7, createElementNS, VARIANT*,BSTR,IHTMLElement**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_3(HTMLDocument7, createAttributeNS, VARIANT*,BSTR,IHTMLDOMAttribute**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_onmsthumbnailclick, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_onmsthumbnailclick, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_characterSet, BSTR*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(HTMLDocument7, createElement, BSTR,IHTMLElement**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(HTMLDocument7, createAttribute, BSTR,IHTMLDOMAttribute**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(HTMLDocument7, getElementsByClassName, BSTR,IHTMLElementCollection**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_3(HTMLDocument7, createProcessingInstruction, BSTR,BSTR,IDOMProcessingInstruction**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(HTMLDocument7, adoptNode, IHTMLDOMNode*,IHTMLDOMNode3**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_onmssitemodejumplistitemremoved, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_onmssitemodejumplistitemremoved, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_all, IHTMLElementCollection**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_inputEncoding, BSTR*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_xmlEncoding, BSTR*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_xmlStandalone, VARIANT_BOOL)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_xmlStandalone, VARIANT_BOOL*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_xmlVersion, BSTR)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_xmlVersion, BSTR*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, hasAttributes, VARIANT_BOOL*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_onabort, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_onabort, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_onblur, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_onblur, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_oncanplay, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_oncanplay, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_oncanplaythrough, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_oncanplaythrough, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_onchange, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_onchange, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_ondrag, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_ondrag, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_ondragend, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_ondragend, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_ondragenter, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_ondragenter, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_ondragleave, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_ondragleave, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_ondragover, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_ondragover, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_ondrop, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_ondrop, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_ondurationchange, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_ondurationchange, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_onemptied, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_onemptied, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_onended, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_onended, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_onerror, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_onerror, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_onfocus, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_onfocus, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_oninput, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_oninput, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_onload, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_onload, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_onloadeddata, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_onloadeddata, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_onloadedmetadata, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_onloadedmetadata, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_onloadstart, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_onloadstart, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_onpause, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_onpause, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_onplay, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_onplay, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_onplaying, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_onplaying, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_onprogress, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_onprogress, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_onratechange, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_onratechange, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_onreset, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_onreset, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_onscroll, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_onscroll, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_onseeked, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_onseeked, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_onseeking, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_onseeking, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_onselect, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_onselect, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_onstalled, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_onstalled, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_onsubmit, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_onsubmit, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_onsuspend, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_onsuspend, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_ontimeupdate, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_ontimeupdate, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_onvolumechange, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_onvolumechange, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, put_onwaiting, VARIANT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_onwaiting, VARIANT*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_0(HTMLDocument7, normalize)
HTMLDOCUMENTOBJ_FWD_TO_NODE_3(HTMLDocument7, importNode, IHTMLDOMNode*,VARIANT_BOOL,IHTMLDOMNode3**)

static HRESULT WINAPI DocObjHTMLDocument7_get_parentWindow(IHTMLDocument7 *iface, IHTMLWindow2 **p)
{
    HTMLDocumentObj *This = impl_from_IHTMLDocument7(iface);

    TRACE("(%p)->(%p)\n", This, p);

    return IHTMLDocument7_get_defaultView(&This->IHTMLDocument7_iface, p);
}

HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, putref_body, IHTMLElement*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_body, IHTMLElement**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(HTMLDocument7, get_head, IHTMLElement**)

static const IHTMLDocument7Vtbl DocObjHTMLDocument7Vtbl = {
    DocObjHTMLDocument7_QueryInterface,
    DocObjHTMLDocument7_AddRef,
    DocObjHTMLDocument7_Release,
    DocObjHTMLDocument7_GetTypeInfoCount,
    DocObjHTMLDocument7_GetTypeInfo,
    DocObjHTMLDocument7_GetIDsOfNames,
    DocObjHTMLDocument7_Invoke,
    DocObjHTMLDocument7_get_defaultView,
    DocObjHTMLDocument7_createCDATASection,
    DocObjHTMLDocument7_getSelection,
    DocObjHTMLDocument7_getElementsByTagNameNS,
    DocObjHTMLDocument7_createElementNS,
    DocObjHTMLDocument7_createAttributeNS,
    DocObjHTMLDocument7_put_onmsthumbnailclick,
    DocObjHTMLDocument7_get_onmsthumbnailclick,
    DocObjHTMLDocument7_get_characterSet,
    DocObjHTMLDocument7_createElement,
    DocObjHTMLDocument7_createAttribute,
    DocObjHTMLDocument7_getElementsByClassName,
    DocObjHTMLDocument7_createProcessingInstruction,
    DocObjHTMLDocument7_adoptNode,
    DocObjHTMLDocument7_put_onmssitemodejumplistitemremoved,
    DocObjHTMLDocument7_get_onmssitemodejumplistitemremoved,
    DocObjHTMLDocument7_get_all,
    DocObjHTMLDocument7_get_inputEncoding,
    DocObjHTMLDocument7_get_xmlEncoding,
    DocObjHTMLDocument7_put_xmlStandalone,
    DocObjHTMLDocument7_get_xmlStandalone,
    DocObjHTMLDocument7_put_xmlVersion,
    DocObjHTMLDocument7_get_xmlVersion,
    DocObjHTMLDocument7_hasAttributes,
    DocObjHTMLDocument7_put_onabort,
    DocObjHTMLDocument7_get_onabort,
    DocObjHTMLDocument7_put_onblur,
    DocObjHTMLDocument7_get_onblur,
    DocObjHTMLDocument7_put_oncanplay,
    DocObjHTMLDocument7_get_oncanplay,
    DocObjHTMLDocument7_put_oncanplaythrough,
    DocObjHTMLDocument7_get_oncanplaythrough,
    DocObjHTMLDocument7_put_onchange,
    DocObjHTMLDocument7_get_onchange,
    DocObjHTMLDocument7_put_ondrag,
    DocObjHTMLDocument7_get_ondrag,
    DocObjHTMLDocument7_put_ondragend,
    DocObjHTMLDocument7_get_ondragend,
    DocObjHTMLDocument7_put_ondragenter,
    DocObjHTMLDocument7_get_ondragenter,
    DocObjHTMLDocument7_put_ondragleave,
    DocObjHTMLDocument7_get_ondragleave,
    DocObjHTMLDocument7_put_ondragover,
    DocObjHTMLDocument7_get_ondragover,
    DocObjHTMLDocument7_put_ondrop,
    DocObjHTMLDocument7_get_ondrop,
    DocObjHTMLDocument7_put_ondurationchange,
    DocObjHTMLDocument7_get_ondurationchange,
    DocObjHTMLDocument7_put_onemptied,
    DocObjHTMLDocument7_get_onemptied,
    DocObjHTMLDocument7_put_onended,
    DocObjHTMLDocument7_get_onended,
    DocObjHTMLDocument7_put_onerror,
    DocObjHTMLDocument7_get_onerror,
    DocObjHTMLDocument7_put_onfocus,
    DocObjHTMLDocument7_get_onfocus,
    DocObjHTMLDocument7_put_oninput,
    DocObjHTMLDocument7_get_oninput,
    DocObjHTMLDocument7_put_onload,
    DocObjHTMLDocument7_get_onload,
    DocObjHTMLDocument7_put_onloadeddata,
    DocObjHTMLDocument7_get_onloadeddata,
    DocObjHTMLDocument7_put_onloadedmetadata,
    DocObjHTMLDocument7_get_onloadedmetadata,
    DocObjHTMLDocument7_put_onloadstart,
    DocObjHTMLDocument7_get_onloadstart,
    DocObjHTMLDocument7_put_onpause,
    DocObjHTMLDocument7_get_onpause,
    DocObjHTMLDocument7_put_onplay,
    DocObjHTMLDocument7_get_onplay,
    DocObjHTMLDocument7_put_onplaying,
    DocObjHTMLDocument7_get_onplaying,
    DocObjHTMLDocument7_put_onprogress,
    DocObjHTMLDocument7_get_onprogress,
    DocObjHTMLDocument7_put_onratechange,
    DocObjHTMLDocument7_get_onratechange,
    DocObjHTMLDocument7_put_onreset,
    DocObjHTMLDocument7_get_onreset,
    DocObjHTMLDocument7_put_onscroll,
    DocObjHTMLDocument7_get_onscroll,
    DocObjHTMLDocument7_put_onseeked,
    DocObjHTMLDocument7_get_onseeked,
    DocObjHTMLDocument7_put_onseeking,
    DocObjHTMLDocument7_get_onseeking,
    DocObjHTMLDocument7_put_onselect,
    DocObjHTMLDocument7_get_onselect,
    DocObjHTMLDocument7_put_onstalled,
    DocObjHTMLDocument7_get_onstalled,
    DocObjHTMLDocument7_put_onsubmit,
    DocObjHTMLDocument7_get_onsubmit,
    DocObjHTMLDocument7_put_onsuspend,
    DocObjHTMLDocument7_get_onsuspend,
    DocObjHTMLDocument7_put_ontimeupdate,
    DocObjHTMLDocument7_get_ontimeupdate,
    DocObjHTMLDocument7_put_onvolumechange,
    DocObjHTMLDocument7_get_onvolumechange,
    DocObjHTMLDocument7_put_onwaiting,
    DocObjHTMLDocument7_get_onwaiting,
    DocObjHTMLDocument7_normalize,
    DocObjHTMLDocument7_importNode,
    DocObjHTMLDocument7_get_parentWindow,
    DocObjHTMLDocument7_putref_body,
    DocObjHTMLDocument7_get_body,
    DocObjHTMLDocument7_get_head
};

/**********************************************************
 * ISupportErrorInfo implementation
 */
HTMLDOCUMENTOBJ_IUNKNOWN_METHODS(SupportErrorInfo)

static HRESULT WINAPI DocObjSupportErrorInfo_InterfaceSupportsErrorInfo(ISupportErrorInfo *iface, REFIID riid)
{
    FIXME("(%p)->(%s)\n", iface, debugstr_mshtml_guid(riid));
    return S_FALSE;
}

static const ISupportErrorInfoVtbl DocObjSupportErrorInfoVtbl = {
    DocObjSupportErrorInfo_QueryInterface,
    DocObjSupportErrorInfo_AddRef,
    DocObjSupportErrorInfo_Release,
    DocObjSupportErrorInfo_InterfaceSupportsErrorInfo
};

/**********************************************************
 * IProvideMultipleClassInfo implementation
 */
static inline HTMLDocumentObj *impl_from_IProvideMultipleClassInfo(IProvideMultipleClassInfo *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IProvideMultipleClassInfo_iface);
}

HTMLDOCUMENTOBJ_IUNKNOWN_METHODS(ProvideMultipleClassInfo)

static HRESULT WINAPI DocObjProvideMultipleClassInfo_GetClassInfo(IProvideMultipleClassInfo *iface, ITypeInfo **ppTI)
{
    HTMLDocumentObj *This = impl_from_IProvideMultipleClassInfo(iface);
    TRACE("(%p)->(%p)\n", This, ppTI);
    return get_class_typeinfo(&CLSID_HTMLDocument, ppTI);
}

static HRESULT WINAPI DocObjProvideMultipleClassInfo_GetGUID(IProvideMultipleClassInfo *iface, DWORD dwGuidKind, GUID *pGUID)
{
    HTMLDocumentObj *This = impl_from_IProvideMultipleClassInfo(iface);
    FIXME("(%p)->(%lu %p)\n", This, dwGuidKind, pGUID);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjProvideMultipleClassInfo_GetMultiTypeInfoCount(IProvideMultipleClassInfo *iface, ULONG *pcti)
{
    HTMLDocumentObj *This = impl_from_IProvideMultipleClassInfo(iface);
    FIXME("(%p)->(%p)\n", This, pcti);
    *pcti = 1;
    return S_OK;
}

static HRESULT WINAPI DocObjProvideMultipleClassInfo_GetInfoOfIndex(IProvideMultipleClassInfo *iface, ULONG iti,
        DWORD dwFlags, ITypeInfo **pptiCoClass, DWORD *pdwTIFlags, ULONG *pcdispidReserved, IID *piidPrimary, IID *piidSource)
{
    HTMLDocumentObj *This = impl_from_IProvideMultipleClassInfo(iface);
    FIXME("(%p)->(%lu %lx %p %p %p %p %p)\n", This, iti, dwFlags, pptiCoClass, pdwTIFlags, pcdispidReserved, piidPrimary, piidSource);
    return E_NOTIMPL;
}

static const IProvideMultipleClassInfoVtbl DocObjProvideMultipleClassInfoVtbl = {
    DocObjProvideMultipleClassInfo_QueryInterface,
    DocObjProvideMultipleClassInfo_AddRef,
    DocObjProvideMultipleClassInfo_Release,
    DocObjProvideMultipleClassInfo_GetClassInfo,
    DocObjProvideMultipleClassInfo_GetGUID,
    DocObjProvideMultipleClassInfo_GetMultiTypeInfoCount,
    DocObjProvideMultipleClassInfo_GetInfoOfIndex
};

/**********************************************************
 * IMarkupServices implementation
 */
static inline HTMLDocumentObj *impl_from_IMarkupServices(IMarkupServices *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IMarkupServices_iface);
}

HTMLDOCUMENTOBJ_IUNKNOWN_METHODS(MarkupServices)

static HRESULT WINAPI DocObjMarkupServices_CreateMarkupPointer(IMarkupServices *iface, IMarkupPointer **ppPointer)
{
    HTMLDocumentObj *This = impl_from_IMarkupServices(iface);

    TRACE("(%p)->(%p)\n", This, ppPointer);

    return create_markup_pointer(ppPointer);
}

HTMLDOCUMENTOBJ_FWD_TO_NODE_1(MarkupServices, CreateMarkupContainer, IMarkupContainer**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_3(MarkupServices, CreateElement, ELEMENT_TAG_ID,OLECHAR*,IHTMLElement**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(MarkupServices, CloneElement, IHTMLElement*,IHTMLElement**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_3(MarkupServices, InsertElement, IHTMLElement*,IMarkupPointer*,IMarkupPointer*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(MarkupServices, RemoveElement, IHTMLElement*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(MarkupServices, Remove, IMarkupPointer*,IMarkupPointer*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_3(MarkupServices, Copy, IMarkupPointer*,IMarkupPointer*,IMarkupPointer*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_3(MarkupServices, Move, IMarkupPointer*,IMarkupPointer*,IMarkupPointer*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_3(MarkupServices, InsertText, OLECHAR*,LONG,IMarkupPointer*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_5(MarkupServices, ParseString, OLECHAR*,DWORD,IMarkupContainer**,IMarkupPointer*,IMarkupPointer*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_5(MarkupServices, ParseGlobal, HGLOBAL,DWORD,IMarkupContainer**,IMarkupPointer*,IMarkupPointer*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(MarkupServices, IsScopedElement, IHTMLElement*,BOOL*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(MarkupServices, GetElementTagId, IHTMLElement*,ELEMENT_TAG_ID*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(MarkupServices, GetTagIDForName, BSTR,ELEMENT_TAG_ID*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(MarkupServices, GetNameForTagID, ELEMENT_TAG_ID,BSTR*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_3(MarkupServices, MovePointersToRange, IHTMLTxtRange*,IMarkupPointer*,IMarkupPointer*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_3(MarkupServices, MoveRangeToPointers, IMarkupPointer*,IMarkupPointer*,IHTMLTxtRange*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(MarkupServices, BeginUndoUnit, OLECHAR*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_0(MarkupServices, EndUndoUnit)

static const IMarkupServicesVtbl DocObjMarkupServicesVtbl = {
    DocObjMarkupServices_QueryInterface,
    DocObjMarkupServices_AddRef,
    DocObjMarkupServices_Release,
    DocObjMarkupServices_CreateMarkupPointer,
    DocObjMarkupServices_CreateMarkupContainer,
    DocObjMarkupServices_CreateElement,
    DocObjMarkupServices_CloneElement,
    DocObjMarkupServices_InsertElement,
    DocObjMarkupServices_RemoveElement,
    DocObjMarkupServices_Remove,
    DocObjMarkupServices_Copy,
    DocObjMarkupServices_Move,
    DocObjMarkupServices_InsertText,
    DocObjMarkupServices_ParseString,
    DocObjMarkupServices_ParseGlobal,
    DocObjMarkupServices_IsScopedElement,
    DocObjMarkupServices_GetElementTagId,
    DocObjMarkupServices_GetTagIDForName,
    DocObjMarkupServices_GetNameForTagID,
    DocObjMarkupServices_MovePointersToRange,
    DocObjMarkupServices_MoveRangeToPointers,
    DocObjMarkupServices_BeginUndoUnit,
    DocObjMarkupServices_EndUndoUnit
};

/**********************************************************
 * IMarkupContainer implementation
 */
static inline HTMLDocumentObj *impl_from_IMarkupContainer(IMarkupContainer *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IMarkupContainer_iface);
}

HTMLDOCUMENTOBJ_IUNKNOWN_METHODS(MarkupContainer)

static HRESULT WINAPI DocObjMarkupContainer_OwningDoc(IMarkupContainer *iface, IHTMLDocument2 **ppDoc)
{
    HTMLDocumentObj *This = impl_from_IMarkupContainer(iface);
    FIXME("(%p)->(%p)\n", This, ppDoc);
    return E_NOTIMPL;
}

static const IMarkupContainerVtbl DocObjMarkupContainerVtbl = {
    DocObjMarkupContainer_QueryInterface,
    DocObjMarkupContainer_AddRef,
    DocObjMarkupContainer_Release,
    DocObjMarkupContainer_OwningDoc
};

/**********************************************************
 * IDisplayServices implementation
 */
HTMLDOCUMENTOBJ_IUNKNOWN_METHODS(DisplayServices)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(DisplayServices, CreateDisplayPointer, IDisplayPointer**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_4(DisplayServices, TransformRect, RECT*,COORD_SYSTEM,COORD_SYSTEM,IHTMLElement*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_4(DisplayServices, TransformPoint, POINT*,COORD_SYSTEM,COORD_SYSTEM,IHTMLElement*)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(DisplayServices, GetCaret, IHTMLCaret**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(DisplayServices, GetComputedStyle, IMarkupPointer*,IHTMLComputedStyle**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(DisplayServices, ScrollRectIntoView, IHTMLElement*,RECT)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(DisplayServices, HasFlowLayout, IHTMLElement*,BOOL*)

static const IDisplayServicesVtbl DocObjDisplayServicesVtbl = {
    DocObjDisplayServices_QueryInterface,
    DocObjDisplayServices_AddRef,
    DocObjDisplayServices_Release,
    DocObjDisplayServices_CreateDisplayPointer,
    DocObjDisplayServices_TransformRect,
    DocObjDisplayServices_TransformPoint,
    DocObjDisplayServices_GetCaret,
    DocObjDisplayServices_GetComputedStyle,
    DocObjDisplayServices_ScrollRectIntoView,
    DocObjDisplayServices_HasFlowLayout
};

/**********************************************************
 * IDocumentSelector implementation
 */
HTMLDOCUMENTOBJ_IDISPATCH_METHODS(DocumentSelector)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(DocumentSelector, querySelector, BSTR,IHTMLElement**)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(DocumentSelector, querySelectorAll, BSTR,IHTMLDOMChildrenCollection**)

static const IDocumentSelectorVtbl DocObjDocumentSelectorVtbl = {
    DocObjDocumentSelector_QueryInterface,
    DocObjDocumentSelector_AddRef,
    DocObjDocumentSelector_Release,
    DocObjDocumentSelector_GetTypeInfoCount,
    DocObjDocumentSelector_GetTypeInfo,
    DocObjDocumentSelector_GetIDsOfNames,
    DocObjDocumentSelector_Invoke,
    DocObjDocumentSelector_querySelector,
    DocObjDocumentSelector_querySelectorAll
};

/**********************************************************
 * IDocumentEvent implementation
 */
#ifdef __REACTOS__
/* Our winspool.h turns this into DocumentEventA/DocumentEventW */
#ifdef DocumentEvent
#undef DocumentEvent
#endif
#endif
HTMLDOCUMENTOBJ_IDISPATCH_METHODS(DocumentEvent)
HTMLDOCUMENTOBJ_FWD_TO_NODE_2(DocumentEvent, createEvent, BSTR,IDOMEvent**)

static const IDocumentEventVtbl DocObjDocumentEventVtbl = {
    DocObjDocumentEvent_QueryInterface,
    DocObjDocumentEvent_AddRef,
    DocObjDocumentEvent_Release,
    DocObjDocumentEvent_GetTypeInfoCount,
    DocObjDocumentEvent_GetTypeInfo,
    DocObjDocumentEvent_GetIDsOfNames,
    DocObjDocumentEvent_Invoke,
    DocObjDocumentEvent_createEvent
};

/**********************************************************
 * IDocumentRange implementation
 */
HTMLDOCUMENTOBJ_IDISPATCH_METHODS(DocumentRange)
HTMLDOCUMENTOBJ_FWD_TO_NODE_1(DocumentRange, createRange, IHTMLDOMRange**)

static const IDocumentRangeVtbl DocObjDocumentRangeVtbl = {
    DocObjDocumentRange_QueryInterface,
    DocObjDocumentRange_AddRef,
    DocObjDocumentRange_Release,
    DocObjDocumentRange_GetTypeInfoCount,
    DocObjDocumentRange_GetTypeInfo,
    DocObjDocumentRange_GetIDsOfNames,
    DocObjDocumentRange_Invoke,
    DocObjDocumentRange_createRange
};

/**********************************************************
 * IEventTarget implementation
 */
static inline HTMLDocumentObj *impl_from_IEventTarget(IEventTarget *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IEventTarget_iface);
}

HTMLDOCUMENTOBJ_IDISPATCH_METHODS(EventTarget)

static HRESULT WINAPI DocObjEventTarget_addEventListener(IEventTarget *iface, BSTR type, IDispatch *listener,
        VARIANT_BOOL capture)
{
    HTMLDocumentObj *This = impl_from_IEventTarget(iface);

    return IEventTarget_addEventListener(&This->doc_node->node.event_target.IEventTarget_iface, type, listener, capture);
}

static HRESULT WINAPI DocObjEventTarget_removeEventListener(IEventTarget *iface, BSTR type, IDispatch *listener,
        VARIANT_BOOL capture)
{
    HTMLDocumentObj *This = impl_from_IEventTarget(iface);

    return IEventTarget_removeEventListener(&This->doc_node->node.event_target.IEventTarget_iface, type, listener, capture);
}

static HRESULT WINAPI DocObjEventTarget_dispatchEvent(IEventTarget *iface, IDOMEvent *event_iface, VARIANT_BOOL *result)
{
    HTMLDocumentObj *This = impl_from_IEventTarget(iface);

    return IEventTarget_dispatchEvent(&This->doc_node->node.event_target.IEventTarget_iface, event_iface, result);
}

static const IEventTargetVtbl DocObjEventTargetVtbl = {
    DocObjEventTarget_QueryInterface,
    DocObjEventTarget_AddRef,
    DocObjEventTarget_Release,
    DocObjEventTarget_GetTypeInfoCount,
    DocObjEventTarget_GetTypeInfo,
    DocObjEventTarget_GetIDsOfNames,
    DocObjEventTarget_Invoke,
    DocObjEventTarget_addEventListener,
    DocObjEventTarget_removeEventListener,
    DocObjEventTarget_dispatchEvent
};

static inline HTMLDocumentObj *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IUnknown_inner);
}

static HRESULT WINAPI HTMLDocumentObj_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = impl_from_IUnknown(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IUnknown_inner;
    }else if(IsEqualGUID(&IID_IDispatch, riid) || IsEqualGUID(&IID_IDispatchEx, riid)) {
        *ppv = &This->IDispatchEx_iface;
    }else if(IsEqualGUID(&IID_IHTMLDocument, riid) || IsEqualGUID(&IID_IHTMLDocument2, riid)) {
        *ppv = &This->IHTMLDocument2_iface;
    }else if(IsEqualGUID(&IID_IHTMLDocument3, riid)) {
        *ppv = &This->IHTMLDocument3_iface;
    }else if(IsEqualGUID(&IID_IHTMLDocument4, riid)) {
        *ppv = &This->IHTMLDocument4_iface;
    }else if(IsEqualGUID(&IID_IHTMLDocument5, riid)) {
        *ppv = &This->IHTMLDocument5_iface;
    }else if(IsEqualGUID(&IID_IHTMLDocument6, riid)) {
        *ppv = &This->IHTMLDocument6_iface;
    }else if(IsEqualGUID(&IID_IHTMLDocument7, riid)) {
        *ppv = &This->IHTMLDocument7_iface;
    }else if(IsEqualGUID(&IID_ICustomDoc, riid)) {
        *ppv = &This->ICustomDoc_iface;
    }else if(IsEqualGUID(&IID_IDocumentSelector, riid)) {
        *ppv = &This->IDocumentSelector_iface;
    }else if(IsEqualGUID(&IID_IDocumentEvent, riid)) {
        *ppv = &This->IDocumentEvent_iface;
    }else if(IsEqualGUID(&DIID_DispHTMLDocument, riid)) {
        *ppv = &This->IHTMLDocument2_iface;
    }else if(IsEqualGUID(&IID_ISupportErrorInfo, riid)) {
        *ppv = &This->ISupportErrorInfo_iface;
    }else if(IsEqualGUID(&IID_IProvideClassInfo, riid)) {
        *ppv = &This->IProvideMultipleClassInfo_iface;
    }else if(IsEqualGUID(&IID_IProvideClassInfo2, riid)) {
        *ppv = &This->IProvideMultipleClassInfo_iface;
    }else if(IsEqualGUID(&IID_IProvideMultipleClassInfo, riid)) {
        *ppv = &This->IProvideMultipleClassInfo_iface;
    }else if(IsEqualGUID(&IID_IMarkupServices, riid)) {
        *ppv = &This->IMarkupServices_iface;
    }else if(IsEqualGUID(&IID_IMarkupContainer, riid)) {
        *ppv = &This->IMarkupContainer_iface;
    }else if(IsEqualGUID(&IID_IDisplayServices, riid)) {
        *ppv = &This->IDisplayServices_iface;
    }else if(IsEqualGUID(&IID_IDocumentRange, riid)) {
        *ppv = &This->IDocumentRange_iface;
    }else if(IsEqualGUID(&IID_IOleDocumentView, riid)) {
        *ppv = &This->IOleDocumentView_iface;
    }else if(IsEqualGUID(&IID_IViewObject, riid)) {
        *ppv = &This->IViewObjectEx_iface;
    }else if(IsEqualGUID(&IID_IViewObject2, riid)) {
        *ppv = &This->IViewObjectEx_iface;
    }else if(IsEqualGUID(&IID_IViewObjectEx, riid)) {
        *ppv = &This->IViewObjectEx_iface;
    }else if(IsEqualGUID(&IID_IPersist, riid)) {
        *ppv = &This->IPersistFile_iface;
    }else if(IsEqualGUID(&IID_IPersistMoniker, riid)) {
        *ppv = &This->IPersistMoniker_iface;
    }else if(IsEqualGUID(&IID_IPersistFile, riid)) {
        *ppv = &This->IPersistFile_iface;
    }else if(IsEqualGUID(&IID_IMonikerProp, riid)) {
        *ppv = &This->IMonikerProp_iface;
    }else if(IsEqualGUID(&IID_IPersistStreamInit, riid)) {
        *ppv = &This->IPersistStreamInit_iface;
    }else if(IsEqualGUID(&IID_IPersistHistory, riid)) {
        *ppv = &This->IPersistHistory_iface;
    }else if(IsEqualGUID(&IID_IHlinkTarget, riid)) {
        *ppv = &This->IHlinkTarget_iface;
    }else if(IsEqualGUID(&IID_IOleCommandTarget, riid)) {
        *ppv = &This->IOleCommandTarget_iface;
    }else if(IsEqualGUID(&IID_IOleObject, riid)) {
        *ppv = &This->IOleObject_iface;
    }else if(IsEqualGUID(&IID_IOleDocument, riid)) {
        *ppv = &This->IOleDocument_iface;
    }else if(IsEqualGUID(&IID_IOleInPlaceActiveObject, riid)) {
        *ppv = &This->IOleInPlaceActiveObject_iface;
    }else if(IsEqualGUID(&IID_IOleWindow, riid)) {
        *ppv = &This->IOleInPlaceActiveObject_iface;
    }else if(IsEqualGUID(&IID_IOleInPlaceObject, riid)) {
        *ppv = &This->IOleInPlaceObjectWindowless_iface;
    }else if(IsEqualGUID(&IID_IOleInPlaceObjectWindowless, riid)) {
        *ppv = &This->IOleInPlaceObjectWindowless_iface;
    }else if(IsEqualGUID(&IID_IOleControl, riid)) {
        *ppv = &This->IOleControl_iface;
    }else if(IsEqualGUID(&IID_IObjectWithSite, riid)) {
        *ppv = &This->IObjectWithSite_iface;
    }else if(IsEqualGUID(&IID_IOleContainer, riid)) {
        *ppv = &This->IOleContainer_iface;
    }else if(IsEqualGUID(&IID_IObjectSafety, riid)) {
        *ppv = &This->IObjectSafety_iface;
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        *ppv = &This->IServiceProvider_iface;
    }else if(IsEqualGUID(&IID_ITargetContainer, riid)) {
        *ppv = &This->ITargetContainer_iface;
    }else if(IsEqualGUID(&IID_IConnectionPointContainer, riid)) {
        *ppv = &This->cp_container.IConnectionPointContainer_iface;
    }else if(IsEqualGUID(&IID_IEventTarget, riid)) {
        /* IEventTarget is conditionally exposed. This breaks COM rules when
           it changes its compat mode, but it is how native works (see tests). */
        if(!This->doc_node || This->doc_node->document_mode < COMPAT_MODE_IE9) {
            *ppv = NULL;
            return E_NOINTERFACE;
        }
        *ppv = &This->IEventTarget_iface;
    }else if(IsEqualGUID(&CLSID_CMarkup, riid)) {
        FIXME("(%p)->(CLSID_CMarkup %p)\n", This, ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }else if(IsEqualGUID(&IID_IRunnableObject, riid)) {
        TRACE("(%p)->(IID_IRunnableObject %p) returning NULL\n", This, ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }else if(IsEqualGUID(&IID_IPersistPropertyBag, riid)) {
        TRACE("(%p)->(IID_IPersistPropertyBag %p) returning NULL\n", This, ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }else if(IsEqualGUID(&IID_IExternalConnection, riid)) {
        TRACE("(%p)->(IID_IExternalConnection %p) returning NULL\n", This, ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }else if(IsEqualGUID(&IID_IStdMarshalInfo, riid)) {
        TRACE("(%p)->(IID_IStdMarshalInfo %p) returning NULL\n", This, ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }else if(IsEqualGUID(&IID_IDispatchJS, riid) ||
             IsEqualGUID(&IID_UndocumentedScriptIface, riid) ||
             IsEqualGUID(&IID_IMarshal, riid) ||
             IsEqualGUID(&IID_IManagedObject, riid)) {
        *ppv = NULL;
        return E_NOINTERFACE;
    }else {
        FIXME("Unimplemented interface %s\n", debugstr_mshtml_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI HTMLDocumentObj_AddRef(IUnknown *iface)
{
    HTMLDocumentObj *This = impl_from_IUnknown(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref = %lu\n", This, ref);

    return ref;
}

static void set_window_uninitialized(HTMLOuterWindow *window)
{
    nsChannelBSC *channelbsc;
    nsWineURI *nsuri;
    IMoniker *mon;
    HRESULT hres;
    IUri *uri;

    window->readystate = READYSTATE_UNINITIALIZED;
    set_current_uri(window, NULL);
    if(window->mon) {
        IMoniker_Release(window->mon);
        window->mon = NULL;
    }

    if(!window->base.inner_window)
        return;

    hres = create_uri(L"about:blank", 0, &uri);
    if(FAILED(hres))
        return;

    hres = create_doc_uri(uri, &nsuri);
    IUri_Release(uri);
    if(FAILED(hres))
        return;

    hres = CreateURLMoniker(NULL, L"about:blank", &mon);
    if(SUCCEEDED(hres)) {
        hres = create_channelbsc(mon, NULL, NULL, 0, TRUE, &channelbsc);
        IMoniker_Release(mon);

        if(SUCCEEDED(hres)) {
            channelbsc->bsc.bindf = 0;  /* synchronous binding */

            abort_window_bindings(window->base.inner_window);
            window->base.inner_window->doc->unload_sent = TRUE;

            hres = load_nsuri(window, nsuri, NULL, channelbsc, LOAD_FLAGS_BYPASS_CACHE);
            if(SUCCEEDED(hres))
                hres = create_pending_window(window, channelbsc);
            IBindStatusCallback_Release(&channelbsc->bsc.IBindStatusCallback_iface);
        }
    }
    nsISupports_Release((nsISupports*)nsuri);
    if(FAILED(hres))
        return;

    window->load_flags |= BINDING_REPLACE;
    start_binding(window->pending_window, &window->pending_window->bscallback->bsc, NULL);
}

static ULONG WINAPI HTMLDocumentObj_Release(IUnknown *iface)
{
    HTMLDocumentObj *This = impl_from_IUnknown(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref = %lu\n", This, ref);

    if(!ref) {
        if(This->doc_node) {
            HTMLDocumentNode *doc_node = This->doc_node;

            if(This->nscontainer)
                This->nscontainer->doc = NULL;
            This->doc_node = NULL;
            doc_node->doc_obj = NULL;

            set_window_uninitialized(This->window);
            IHTMLDOMNode_Release(&doc_node->node.IHTMLDOMNode_iface);
        }
        if(This->window)
            IHTMLWindow2_Release(&This->window->base.IHTMLWindow2_iface);
        if(This->advise_holder)
            IOleAdviseHolder_Release(This->advise_holder);

        if(This->view_sink)
            IAdviseSink_Release(This->view_sink);
        if(This->client)
            IOleObject_SetClientSite(&This->IOleObject_iface, NULL);
        if(This->hostui)
            ICustomDoc_SetUIHandler(&This->ICustomDoc_iface, NULL);
        if(This->in_place_active)
            IOleInPlaceObjectWindowless_InPlaceDeactivate(&This->IOleInPlaceObjectWindowless_iface);
        if(This->ipsite)
            IOleDocumentView_SetInPlaceSite(&This->IOleDocumentView_iface, NULL);
        if(This->undomgr)
            IOleUndoManager_Release(This->undomgr);
        if(This->editsvcs)
            IHTMLEditServices_Release(This->editsvcs);
        if(This->tooltips_hwnd)
            DestroyWindow(This->tooltips_hwnd);

        if(This->hwnd)
            DestroyWindow(This->hwnd);
        free(This->mime);

        remove_target_tasks(This->task_magic);
        ConnectionPointContainer_Destroy(&This->cp_container);

        if(This->nscontainer)
            detach_gecko_browser(This->nscontainer);
        free(This);
    }

    return ref;
}

static const IUnknownVtbl HTMLDocumentObjVtbl = {
    HTMLDocumentObj_QueryInterface,
    HTMLDocumentObj_AddRef,
    HTMLDocumentObj_Release
};

/**********************************************************
 * IDispatchEx implementation
 *
 * Forwarding this breaks Dispatch rules by potentially retrieving
 * a different DISPID for the same name, if the node was changed
 * while using the same doc obj, but it is how native works.
 */
static inline HTMLDocumentObj *impl_from_IDispatchEx(IDispatchEx *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, IDispatchEx_iface);
}

HTMLDOCUMENTOBJ_IUNKNOWN_METHODS(DispatchEx)
DISPEX_IDISPATCH_NOUNK_IMPL(DocObjDispatchEx, IDispatchEx,
                            impl_from_IDispatchEx(iface)->doc_node->node.event_target.dispex)

static HRESULT WINAPI DocObjDispatchEx_GetDispID(IDispatchEx *iface, BSTR name, DWORD grfdex, DISPID *pid)
{
    HTMLDocumentObj *This = impl_from_IDispatchEx(iface);

    return IWineJSDispatchHost_GetDispID(&This->doc_node->node.event_target.dispex.IWineJSDispatchHost_iface, name, grfdex, pid);
}

static HRESULT WINAPI DocObjDispatchEx_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    HTMLDocumentObj *This = impl_from_IDispatchEx(iface);

    return IWineJSDispatchHost_InvokeEx(&This->doc_node->node.event_target.dispex.IWineJSDispatchHost_iface, id, lcid,
                                    wFlags, pdp, pvarRes, pei, pspCaller);
}

static HRESULT WINAPI DocObjDispatchEx_DeleteMemberByName(IDispatchEx *iface, BSTR bstrName, DWORD grfdex)
{
    HTMLDocumentObj *This = impl_from_IDispatchEx(iface);

    return IWineJSDispatchHost_DeleteMemberByName(&This->doc_node->node.event_target.dispex.IWineJSDispatchHost_iface, bstrName, grfdex);
}

static HRESULT WINAPI DocObjDispatchEx_DeleteMemberByDispID(IDispatchEx *iface, DISPID id)
{
    HTMLDocumentObj *This = impl_from_IDispatchEx(iface);

    return IWineJSDispatchHost_DeleteMemberByDispID(&This->doc_node->node.event_target.dispex.IWineJSDispatchHost_iface, id);
}

static HRESULT WINAPI DocObjDispatchEx_GetMemberProperties(IDispatchEx *iface, DISPID id, DWORD grfdexFetch, DWORD *pgrfdex)
{
    HTMLDocumentObj *This = impl_from_IDispatchEx(iface);

    return IWineJSDispatchHost_GetMemberProperties(&This->doc_node->node.event_target.dispex.IWineJSDispatchHost_iface, id, grfdexFetch,
            pgrfdex);
}

static HRESULT WINAPI DocObjDispatchEx_GetMemberName(IDispatchEx *iface, DISPID id, BSTR *name)
{
    HTMLDocumentObj *This = impl_from_IDispatchEx(iface);

    return IWineJSDispatchHost_GetMemberName(&This->doc_node->node.event_target.dispex.IWineJSDispatchHost_iface, id, name);
}

static HRESULT WINAPI DocObjDispatchEx_GetNextDispID(IDispatchEx *iface, DWORD grfdex, DISPID id, DISPID *pid)
{
    HTMLDocumentObj *This = impl_from_IDispatchEx(iface);

    return IWineJSDispatchHost_GetNextDispID(&This->doc_node->node.event_target.dispex.IWineJSDispatchHost_iface, grfdex, id, pid);
}

static HRESULT WINAPI DocObjDispatchEx_GetNameSpaceParent(IDispatchEx *iface, IUnknown **ppunk)
{
    HTMLDocumentObj *This = impl_from_IDispatchEx(iface);

    return IWineJSDispatchHost_GetNameSpaceParent(&This->doc_node->node.event_target.dispex.IWineJSDispatchHost_iface, ppunk);
}

static const IDispatchExVtbl DocObjDispatchExVtbl = {
    DocObjDispatchEx_QueryInterface,
    DocObjDispatchEx_AddRef,
    DocObjDispatchEx_Release,
    DocObjDispatchEx_GetTypeInfoCount,
    DocObjDispatchEx_GetTypeInfo,
    DocObjDispatchEx_GetIDsOfNames,
    DocObjDispatchEx_Invoke,
    DocObjDispatchEx_GetDispID,
    DocObjDispatchEx_InvokeEx,
    DocObjDispatchEx_DeleteMemberByName,
    DocObjDispatchEx_DeleteMemberByDispID,
    DocObjDispatchEx_GetMemberProperties,
    DocObjDispatchEx_GetMemberName,
    DocObjDispatchEx_GetNextDispID,
    DocObjDispatchEx_GetNameSpaceParent
};

/**********************************************************
 * ICustomDoc implementation
 */

static inline HTMLDocumentObj *impl_from_ICustomDoc(ICustomDoc *iface)
{
    return CONTAINING_RECORD(iface, HTMLDocumentObj, ICustomDoc_iface);
}

static HRESULT WINAPI CustomDoc_QueryInterface(ICustomDoc *iface, REFIID riid, void **ppv)
{
    HTMLDocumentObj *This = impl_from_ICustomDoc(iface);

    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI CustomDoc_AddRef(ICustomDoc *iface)
{
    HTMLDocumentObj *This = impl_from_ICustomDoc(iface);

    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI CustomDoc_Release(ICustomDoc *iface)
{
    HTMLDocumentObj *This = impl_from_ICustomDoc(iface);

    return IUnknown_Release(This->outer_unk);
}

static HRESULT WINAPI CustomDoc_SetUIHandler(ICustomDoc *iface, IDocHostUIHandler *pUIHandler)
{
    HTMLDocumentObj *This = impl_from_ICustomDoc(iface);
    IOleCommandTarget *cmdtrg;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, pUIHandler);

    if(This->custom_hostui && This->hostui == pUIHandler)
        return S_OK;

    This->custom_hostui = TRUE;

    if(This->hostui)
        IDocHostUIHandler_Release(This->hostui);
    if(pUIHandler)
        IDocHostUIHandler_AddRef(pUIHandler);
    This->hostui = pUIHandler;
    if(!pUIHandler)
        return S_OK;

    hres = IDocHostUIHandler_QueryInterface(pUIHandler, &IID_IOleCommandTarget, (void**)&cmdtrg);
    if(SUCCEEDED(hres)) {
        FIXME("custom UI handler supports IOleCommandTarget\n");
        IOleCommandTarget_Release(cmdtrg);
    }

    return S_OK;
}

static const ICustomDocVtbl CustomDocVtbl = {
    CustomDoc_QueryInterface,
    CustomDoc_AddRef,
    CustomDoc_Release,
    CustomDoc_SetUIHandler
};

static void HTMLDocumentObj_on_advise(IUnknown *iface, cp_static_data_t *cp)
{
    HTMLDocumentObj *This = impl_from_IUnknown(iface);

    if(This->window)
        update_doc_cp_events(This->doc_node, cp);
}

static cp_static_data_t HTMLDocumentObjEvents_data = { HTMLDocumentEvents_tid, HTMLDocumentObj_on_advise };
static cp_static_data_t HTMLDocumentObjEvents2_data = { HTMLDocumentEvents2_tid, HTMLDocumentObj_on_advise, TRUE };

static const cpc_entry_t HTMLDocumentObj_cpc[] = {
    {&IID_IDispatch, &HTMLDocumentObjEvents_data},
    {&IID_IPropertyNotifySink},
    {&DIID_HTMLDocumentEvents, &HTMLDocumentObjEvents_data},
    {&DIID_HTMLDocumentEvents2, &HTMLDocumentObjEvents2_data},
    {NULL}
};

static HRESULT create_document_object(BOOL is_mhtml, IUnknown *outer, REFIID riid, void **ppv)
{
    HTMLDocumentObj *doc;
    HRESULT hres;

    if(outer && !IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = NULL;
        return E_INVALIDARG;
    }

    /* ensure that security manager is initialized */
    if(!get_security_manager())
        return E_OUTOFMEMORY;

    doc = calloc(1, sizeof(HTMLDocumentObj));
    if(!doc)
        return E_OUTOFMEMORY;

    doc->ref = 1;
    doc->IUnknown_inner.lpVtbl = &HTMLDocumentObjVtbl;
    doc->IDispatchEx_iface.lpVtbl = &DocObjDispatchExVtbl;
    doc->ICustomDoc_iface.lpVtbl = &CustomDocVtbl;
    doc->IHTMLDocument2_iface.lpVtbl = &DocObjHTMLDocument2Vtbl;
    doc->IHTMLDocument3_iface.lpVtbl = &DocObjHTMLDocument3Vtbl;
    doc->IHTMLDocument4_iface.lpVtbl = &DocObjHTMLDocument4Vtbl;
    doc->IHTMLDocument5_iface.lpVtbl = &DocObjHTMLDocument5Vtbl;
    doc->IHTMLDocument6_iface.lpVtbl = &DocObjHTMLDocument6Vtbl;
    doc->IHTMLDocument7_iface.lpVtbl = &DocObjHTMLDocument7Vtbl;
    doc->IDocumentSelector_iface.lpVtbl = &DocObjDocumentSelectorVtbl;
    doc->IDocumentEvent_iface.lpVtbl = &DocObjDocumentEventVtbl;
    doc->ISupportErrorInfo_iface.lpVtbl = &DocObjSupportErrorInfoVtbl;
    doc->IProvideMultipleClassInfo_iface.lpVtbl = &DocObjProvideMultipleClassInfoVtbl;
    doc->IMarkupServices_iface.lpVtbl = &DocObjMarkupServicesVtbl;
    doc->IMarkupContainer_iface.lpVtbl = &DocObjMarkupContainerVtbl;
    doc->IDisplayServices_iface.lpVtbl = &DocObjDisplayServicesVtbl;
    doc->IDocumentRange_iface.lpVtbl = &DocObjDocumentRangeVtbl;
    doc->IEventTarget_iface.lpVtbl = &DocObjEventTargetVtbl;

    doc->outer_unk = outer ? outer : &doc->IUnknown_inner;

    ConnectionPointContainer_Init(&doc->cp_container, &doc->IUnknown_inner, HTMLDocumentObj_cpc);
    HTMLDocumentObj_Persist_Init(doc);
    HTMLDocumentObj_Service_Init(doc);
    HTMLDocumentObj_OleCmd_Init(doc);
    HTMLDocumentObj_OleObj_Init(doc);
    TargetContainer_Init(doc);
    doc->is_mhtml = is_mhtml;

    doc->task_magic = get_task_target_magic();

    HTMLDocument_View_Init(doc);

    hres = create_gecko_browser(doc, &doc->nscontainer);
    if(FAILED(hres)) {
        ERR("Failed to init Gecko, returning CLASS_E_CLASSNOTAVAILABLE\n");
        IUnknown_Release(&doc->IUnknown_inner);
        return hres;
    }

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &doc->IUnknown_inner;
    }else {
        hres = IUnknown_QueryInterface(doc->outer_unk, riid, ppv);
        IUnknown_Release(doc->outer_unk);
        if(FAILED(hres))
            return hres;
    }

    doc->window = doc->nscontainer->content_window;
    IHTMLWindow2_AddRef(&doc->window->base.IHTMLWindow2_iface);

    if(!doc->doc_node) {
        doc->doc_node = doc->window->base.inner_window->doc;
        IHTMLDOMNode_AddRef(&doc->doc_node->node.IHTMLDOMNode_iface);
    }

    get_thread_hwnd();

    return S_OK;
}

HRESULT HTMLDocument_Create(IUnknown *outer, REFIID riid, void **ppv)
{
    TRACE("(%p %s %p)\n", outer, debugstr_mshtml_guid(riid), ppv);
    return create_document_object(FALSE, outer, riid, ppv);
}

HRESULT MHTMLDocument_Create(IUnknown *outer, REFIID riid, void **ppv)
{
    TRACE("(%p %s %p)\n", outer, debugstr_mshtml_guid(riid), ppv);
    return create_document_object(TRUE, outer, riid, ppv);
}
