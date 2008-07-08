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
#include "shlguid.h"
#include "mshtmdid.h"
#include "idispids.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

/**********************************************************
 * IOleObject implementation
 */

#define OLEOBJ_THIS(iface) DEFINE_THIS(HTMLDocument, OleObject, iface)

static HRESULT WINAPI OleObject_QueryInterface(IOleObject *iface, REFIID riid, void **ppvObject)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    return IHTMLDocument2_QueryInterface(HTMLDOC(This), riid, ppvObject);
}

static ULONG WINAPI OleObject_AddRef(IOleObject *iface)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    return IHTMLDocument2_AddRef(HTMLDOC(This));
}

static ULONG WINAPI OleObject_Release(IOleObject *iface)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    return IHTMLDocument2_Release(HTMLDOC(This));
}

static void update_hostinfo(HTMLDocument *This, DOCHOSTUIINFO *hostinfo)
{
    nsIScrollable *scrollable;
    nsresult nsres;

    if(!This->nscontainer)
        return;

    nsres = nsIWebBrowser_QueryInterface(This->nscontainer->webbrowser, &IID_nsIScrollable, (void**)&scrollable);
    if(NS_SUCCEEDED(nsres)) {
        nsres = nsIScrollable_SetDefaultScrollbarPreferences(scrollable, ScrollOrientation_Y,
                (hostinfo->dwFlags & DOCHOSTUIFLAG_SCROLL_NO) ? Scrollbar_Never : Scrollbar_Always);
        if(NS_FAILED(nsres))
            ERR("Could not set default Y scrollbar prefs: %08x\n", nsres);

        nsres = nsIScrollable_SetDefaultScrollbarPreferences(scrollable, ScrollOrientation_X,
                hostinfo->dwFlags & DOCHOSTUIFLAG_SCROLL_NO ? Scrollbar_Never : Scrollbar_Auto);
        if(NS_FAILED(nsres))
            ERR("Could not set default X scrollbar prefs: %08x\n", nsres);

        nsIScrollable_Release(scrollable);
    }else {
        ERR("Could not get nsIScrollable: %08x\n", nsres);
    }
}

static HRESULT WINAPI OleObject_SetClientSite(IOleObject *iface, IOleClientSite *pClientSite)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    IDocHostUIHandler *pDocHostUIHandler = NULL;
    IOleCommandTarget *cmdtrg = NULL;
    VARIANT silent;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, pClientSite);

    if(pClientSite == This->client)
        return S_OK;

    if(This->client) {
        IOleClientSite_Release(This->client);
        This->client = NULL;
        This->usermode = UNKNOWN_USERMODE;
    }

    if(This->hostui) {
        IDocHostUIHandler_Release(This->hostui);
        This->hostui = NULL;
    }

    memset(&This->hostinfo, 0, sizeof(DOCHOSTUIINFO));

    if(!pClientSite)
        return S_OK;

    hres = IOleObject_QueryInterface(pClientSite, &IID_IDocHostUIHandler, (void**)&pDocHostUIHandler);
    if(SUCCEEDED(hres)) {
        DOCHOSTUIINFO hostinfo;
        LPOLESTR key_path = NULL, override_key_path = NULL;
        IDocHostUIHandler2 *pDocHostUIHandler2;

        memset(&hostinfo, 0, sizeof(DOCHOSTUIINFO));
        hostinfo.cbSize = sizeof(DOCHOSTUIINFO);
        hres = IDocHostUIHandler_GetHostInfo(pDocHostUIHandler, &hostinfo);
        if(SUCCEEDED(hres)) {
            TRACE("hostinfo = {%u %08x %08x %s %s}\n",
                    hostinfo.cbSize, hostinfo.dwFlags, hostinfo.dwDoubleClick,
                    debugstr_w(hostinfo.pchHostCss), debugstr_w(hostinfo.pchHostNS));
            update_hostinfo(This, &hostinfo);
            This->hostinfo = hostinfo;
        }

        if(!This->has_key_path) {
            hres = IDocHostUIHandler_GetOptionKeyPath(pDocHostUIHandler, &key_path, 0);
            if(hres == S_OK && key_path) {
                if(key_path[0]) {
                    /* FIXME: use key_path */
                    TRACE("key_path = %s\n", debugstr_w(key_path));
                }
                CoTaskMemFree(key_path);
            }

            hres = IDocHostUIHandler_QueryInterface(pDocHostUIHandler, &IID_IDocHostUIHandler2,
                    (void**)&pDocHostUIHandler2);
            if(SUCCEEDED(hres)) {
                hres = IDocHostUIHandler2_GetOverrideKeyPath(pDocHostUIHandler2, &override_key_path, 0);
                if(hres == S_OK && override_key_path && override_key_path[0]) {
                    if(override_key_path[0]) {
                        /*FIXME: use override_key_path */
                        TRACE("override_key_path = %s\n", debugstr_w(override_key_path));
                    }
                    CoTaskMemFree(override_key_path);
                }
                IDocHostUIHandler2_Release(pDocHostUIHandler2);
            }

            This->has_key_path = TRUE;
        }
    }

    /* Native calls here GetWindow. What is it for?
     * We don't have anything to do with it here (yet). */
    if(pClientSite) {
        IOleWindow *pOleWindow = NULL;
        HWND hwnd;

        hres = IOleClientSite_QueryInterface(pClientSite, &IID_IOleWindow, (void**)&pOleWindow);
        if(SUCCEEDED(hres)) {
            IOleWindow_GetWindow(pOleWindow, &hwnd);
            IOleWindow_Release(pOleWindow);
        }
    }

    hres = IOleClientSite_QueryInterface(pClientSite, &IID_IOleCommandTarget, (void**)&cmdtrg);
    if(SUCCEEDED(hres)) {
        VARIANT var;
        OLECMD cmd = {OLECMDID_SETPROGRESSTEXT, 0};

        IOleCommandTarget_QueryStatus(cmdtrg, NULL, 1, &cmd, NULL);

        V_VT(&var) = VT_I4;
        V_I4(&var) = 0;
        IOleCommandTarget_Exec(cmdtrg, NULL, OLECMDID_SETPROGRESSMAX,
                OLECMDEXECOPT_DONTPROMPTUSER, &var, NULL);
        IOleCommandTarget_Exec(cmdtrg, NULL, OLECMDID_SETPROGRESSPOS, 
                OLECMDEXECOPT_DONTPROMPTUSER, &var, NULL);

        IOleCommandTarget_Release(cmdtrg);
    }

    IOleClientSite_AddRef(pClientSite);
    This->client = pClientSite;
    This->hostui = pDocHostUIHandler;

    if(This->usermode == UNKNOWN_USERMODE)
        IOleControl_OnAmbientPropertyChange(CONTROL(This), DISPID_AMBIENT_USERMODE);

    IOleControl_OnAmbientPropertyChange(CONTROL(This), DISPID_AMBIENT_OFFLINEIFNOTCONNECTED); 

    hres = get_client_disp_property(This->client, DISPID_AMBIENT_SILENT, &silent);
    if(SUCCEEDED(hres)) {
        if(V_VT(&silent) != VT_BOOL)
            WARN("V_VT(silent) = %d\n", V_VT(&silent));
        else if(V_BOOL(&silent))
            FIXME("silent == true\n");
    }

    IOleControl_OnAmbientPropertyChange(CONTROL(This), DISPID_AMBIENT_USERAGENT);
    IOleControl_OnAmbientPropertyChange(CONTROL(This), DISPID_AMBIENT_PALETTE);

    return S_OK;
}

static HRESULT WINAPI OleObject_GetClientSite(IOleObject *iface, IOleClientSite **ppClientSite)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);

    TRACE("(%p)->(%p)\n", This, ppClientSite);

    if(!ppClientSite)
        return E_INVALIDARG;

    if(This->client)
        IOleClientSite_AddRef(This->client);
    *ppClientSite = This->client;

    return S_OK;
}

static HRESULT WINAPI OleObject_SetHostNames(IOleObject *iface, LPCOLESTR szContainerApp, LPCOLESTR szContainerObj)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_w(szContainerApp), debugstr_w(szContainerObj));
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Close(IOleObject *iface, DWORD dwSaveOption)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);

    TRACE("(%p)->(%08x)\n", This, dwSaveOption);

    if(dwSaveOption == OLECLOSE_PROMPTSAVE)
        FIXME("OLECLOSE_PROMPTSAVE not implemented\n");

    if(This->in_place_active)
        IOleInPlaceObjectWindowless_InPlaceDeactivate(INPLACEWIN(This));

    HTMLDocument_LockContainer(This, FALSE);
    
    return S_OK;
}

static HRESULT WINAPI OleObject_SetMoniker(IOleObject *iface, DWORD dwWhichMoniker, IMoniker *pmk)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p %d %p)->()\n", This, dwWhichMoniker, pmk);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetMoniker(IOleObject *iface, DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%d %d %p)\n", This, dwAssign, dwWhichMoniker, ppmk);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_InitFromData(IOleObject *iface, IDataObject *pDataObject, BOOL fCreation,
                                        DWORD dwReserved)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%p %x %d)\n", This, pDataObject, fCreation, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetClipboardData(IOleObject *iface, DWORD dwReserved, IDataObject **ppDataObject)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%d %p)\n", This, dwReserved, ppDataObject);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_DoVerb(IOleObject *iface, LONG iVerb, LPMSG lpmsg, IOleClientSite *pActiveSite,
                                        LONG lindex, HWND hwndParent, LPCRECT lprcPosRect)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    IOleDocumentSite *pDocSite;
    HRESULT hres;

    TRACE("(%p)->(%d %p %p %d %p %p)\n", This, iVerb, lpmsg, pActiveSite, lindex, hwndParent, lprcPosRect);

    if(iVerb != OLEIVERB_SHOW && iVerb != OLEIVERB_UIACTIVATE && iVerb != OLEIVERB_INPLACEACTIVATE) { 
        FIXME("iVerb = %d not supported\n", iVerb);
        return E_NOTIMPL;
    }

    if(!pActiveSite)
        pActiveSite = This->client;

    hres = IOleClientSite_QueryInterface(pActiveSite, &IID_IOleDocumentSite, (void**)&pDocSite);
    if(SUCCEEDED(hres)) {
        HTMLDocument_LockContainer(This, TRUE);

        /* FIXME: Create new IOleDocumentView. See CreateView for more info. */
        hres = IOleDocumentSite_ActivateMe(pDocSite, DOCVIEW(This));
        IOleDocumentSite_Release(pDocSite);
    }else {
        hres = IOleDocumentView_UIActivate(DOCVIEW(This), TRUE);
        if(SUCCEEDED(hres)) {
            if(lprcPosRect) {
                RECT rect; /* We need to pass rect as not const pointer */
                rect = *lprcPosRect;
                IOleDocumentView_SetRect(DOCVIEW(This), &rect);
            }
            IOleDocumentView_Show(DOCVIEW(This), TRUE);
        }
    }

    return hres;
}

static HRESULT WINAPI OleObject_EnumVerbs(IOleObject *iface, IEnumOLEVERB **ppEnumOleVerb)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppEnumOleVerb);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Update(IOleObject *iface)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_IsUpToDate(IOleObject *iface)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetUserClassID(IOleObject *iface, CLSID *pClsid)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);

    TRACE("(%p)->(%p)\n", This, pClsid);

    if(!pClsid)
        return E_INVALIDARG;

    *pClsid = CLSID_HTMLDocument;
    return S_OK;
}

static HRESULT WINAPI OleObject_GetUserType(IOleObject *iface, DWORD dwFormOfType, LPOLESTR *pszUserType)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%d %p)\n", This, dwFormOfType, pszUserType);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_SetExtent(IOleObject *iface, DWORD dwDrawAspect, SIZEL *psizel)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%d %p)\n", This, dwDrawAspect, psizel);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetExtent(IOleObject *iface, DWORD dwDrawAspect, SIZEL *psizel)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%d %p)\n", This, dwDrawAspect, psizel);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Advise(IOleObject *iface, IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pAdvSink, pdwConnection);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Unadvise(IOleObject *iface, DWORD dwConnection)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%d)\n", This, dwConnection);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_EnumAdvise(IOleObject *iface, IEnumSTATDATA **ppenumAdvise)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppenumAdvise);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetMiscStatus(IOleObject *iface, DWORD dwAspect, DWORD *pdwStatus)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%d %p)\n", This, dwAspect, pdwStatus);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_SetColorScheme(IOleObject *iface, LOGPALETTE *pLogpal)
{
    HTMLDocument *This = OLEOBJ_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pLogpal);
    return E_NOTIMPL;
}

#undef OLEPBJ_THIS

static const IOleObjectVtbl OleObjectVtbl = {
    OleObject_QueryInterface,
    OleObject_AddRef,
    OleObject_Release,
    OleObject_SetClientSite,
    OleObject_GetClientSite,
    OleObject_SetHostNames,
    OleObject_Close,
    OleObject_SetMoniker,
    OleObject_GetMoniker,
    OleObject_InitFromData,
    OleObject_GetClipboardData,
    OleObject_DoVerb,
    OleObject_EnumVerbs,
    OleObject_Update,
    OleObject_IsUpToDate,
    OleObject_GetUserClassID,
    OleObject_GetUserType,
    OleObject_SetExtent,
    OleObject_GetExtent,
    OleObject_Advise,
    OleObject_Unadvise,
    OleObject_EnumAdvise,
    OleObject_GetMiscStatus,
    OleObject_SetColorScheme
};

/**********************************************************
 * IOleDocument implementation
 */

#define OLEDOC_THIS(iface) DEFINE_THIS(HTMLDocument, OleDocument, iface)

static HRESULT WINAPI OleDocument_QueryInterface(IOleDocument *iface, REFIID riid, void **ppvObject)
{
    HTMLDocument *This = OLEDOC_THIS(iface);
    return IHTMLDocument2_QueryInterface(HTMLDOC(This), riid, ppvObject);
}

static ULONG WINAPI OleDocument_AddRef(IOleDocument *iface)
{
    HTMLDocument *This = OLEDOC_THIS(iface);
    return IHTMLDocument2_AddRef(HTMLDOC(This));
}

static ULONG WINAPI OleDocument_Release(IOleDocument *iface)
{
    HTMLDocument *This = OLEDOC_THIS(iface);
    return IHTMLDocument2_Release(HTMLDOC(This));
}

static HRESULT WINAPI OleDocument_CreateView(IOleDocument *iface, IOleInPlaceSite *pIPSite, IStream *pstm,
                                   DWORD dwReserved, IOleDocumentView **ppView)
{
    HTMLDocument *This = OLEDOC_THIS(iface);
    HRESULT hres;

    TRACE("(%p)->(%p %p %d %p)\n", This, pIPSite, pstm, dwReserved, ppView);

    if(!ppView)
        return E_INVALIDARG;

    /* FIXME:
     * Windows implementation creates new IOleDocumentView when function is called for the
     * first time and returns E_FAIL when it is called for the second time, but it doesn't matter
     * if the application uses returned interfaces, passed to ActivateMe or returned by
     * QueryInterface, so there is no reason to create new interface. This needs more testing.
     */

    if(pIPSite) {
        hres = IOleDocumentView_SetInPlaceSite(DOCVIEW(This), pIPSite);
        if(FAILED(hres))
            return hres;
    }

    if(pstm)
        FIXME("pstm is not supported\n");

    IOleDocumentView_AddRef(DOCVIEW(This));
    *ppView = DOCVIEW(This);
    return S_OK;
}

static HRESULT WINAPI OleDocument_GetDocMiscStatus(IOleDocument *iface, DWORD *pdwStatus)
{
    HTMLDocument *This = OLEDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pdwStatus);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleDocument_EnumViews(IOleDocument *iface, IEnumOleDocumentViews **ppEnum,
                                   IOleDocumentView **ppView)
{
    HTMLDocument *This = OLEDOC_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, ppEnum, ppView);
    return E_NOTIMPL;
}

#undef OLEDOC_THIS

static const IOleDocumentVtbl OleDocumentVtbl = {
    OleDocument_QueryInterface,
    OleDocument_AddRef,
    OleDocument_Release,
    OleDocument_CreateView,
    OleDocument_GetDocMiscStatus,
    OleDocument_EnumViews
};

/**********************************************************
 * IOleControl implementation
 */

#define CONTROL_THIS(iface) DEFINE_THIS(HTMLDocument, OleControl, iface)

static HRESULT WINAPI OleControl_QueryInterface(IOleControl *iface, REFIID riid, void **ppv)
{
    HTMLDocument *This = CONTROL_THIS(iface);
    return IHTMLDocument2_QueryInterface(HTMLDOC(This), riid, ppv);
}

static ULONG WINAPI OleControl_AddRef(IOleControl *iface)
{
    HTMLDocument *This = CONTROL_THIS(iface);
    return IHTMLDocument2_AddRef(HTMLDOC(This));
}

static ULONG WINAPI OleControl_Release(IOleControl *iface)
{
    HTMLDocument *This = CONTROL_THIS(iface);
    return IHTMLDocument_Release(HTMLDOC(This));
}

static HRESULT WINAPI OleControl_GetControlInfo(IOleControl *iface, CONTROLINFO *pCI)
{
    HTMLDocument *This = CONTROL_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pCI);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleControl_OnMnemonic(IOleControl *iface, MSG *pMsg)
{
    HTMLDocument *This = CONTROL_THIS(iface);
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

static HRESULT on_change_dlcontrol(HTMLDocument *This)
{
    VARIANT res;
    HRESULT hres;
    
    hres = get_client_disp_property(This->client, DISPID_AMBIENT_DLCONTROL, &res);
    if(SUCCEEDED(hres))
        FIXME("unsupported dlcontrol %08x\n", V_I4(&res));

    return S_OK;
}

static HRESULT WINAPI OleControl_OnAmbientPropertyChange(IOleControl *iface, DISPID dispID)
{
    HTMLDocument *This = CONTROL_THIS(iface);
    VARIANT res;
    HRESULT hres;

    if(!This->client) {
        TRACE("This->client = NULL\n");
        return S_OK;
    }

    switch(dispID) {
    case DISPID_AMBIENT_USERMODE:
        TRACE("(%p)->(DISPID_AMBIENT_USERMODE)\n", This);
        hres = get_client_disp_property(This->client, DISPID_AMBIENT_USERMODE, &res);
        if(FAILED(hres))
            return S_OK;

        if(V_VT(&res) == VT_BOOL) {
            if(V_BOOL(&res)) {
                This->usermode = BROWSEMODE;
            }else {
                FIXME("edit mode is not supported\n");
                This->usermode = EDITMODE;
            }
        }else {
            FIXME("V_VT(res)=%d\n", V_VT(&res));
        }
        return S_OK;
    case DISPID_AMBIENT_DLCONTROL:
        TRACE("(%p)->(DISPID_AMBIENT_DLCONTROL)\n", This);
        return on_change_dlcontrol(This);
    case DISPID_AMBIENT_OFFLINEIFNOTCONNECTED:
        TRACE("(%p)->(DISPID_AMBIENT_OFFLINEIFNOTCONNECTED)\n", This);
        on_change_dlcontrol(This);
        hres = get_client_disp_property(This->client, DISPID_AMBIENT_OFFLINEIFNOTCONNECTED, &res);
        if(FAILED(hres))
            return S_OK;

        if(V_VT(&res) == VT_BOOL) {
            if(V_BOOL(&res)) {
                FIXME("offline connection is not supported\n");
                hres = E_FAIL;
            }
        }else {
            FIXME("V_VT(res)=%d\n", V_VT(&res));
        }
        return S_OK;
    case DISPID_AMBIENT_SILENT:
        TRACE("(%p)->(DISPID_AMBIENT_SILENT)\n", This);
        on_change_dlcontrol(This);
        hres = get_client_disp_property(This->client, DISPID_AMBIENT_SILENT, &res);
        if(FAILED(hres))
            return S_OK;

        if(V_VT(&res) == VT_BOOL) {
            if(V_BOOL(&res)) {
                FIXME("silent mode is not supported\n");
                hres = E_FAIL;
            }
        }else {
            FIXME("V_VT(res)=%d\n", V_VT(&res));
        }
        return S_OK;
    case DISPID_AMBIENT_USERAGENT:
        TRACE("(%p)->(DISPID_AMBIENT_USERAGENT)\n", This);
        hres = get_client_disp_property(This->client, DISPID_AMBIENT_USERAGENT, &res);
        if(FAILED(hres))
            return S_OK;

        FIXME("not supported AMBIENT_USERAGENT\n");
        hres = E_FAIL;
        return S_OK;
    case DISPID_AMBIENT_PALETTE:
        TRACE("(%p)->(DISPID_AMBIENT_PALETTE)\n", This);
        hres = get_client_disp_property(This->client, DISPID_AMBIENT_PALETTE, &res);
        if(FAILED(hres))
            return S_OK;

        FIXME("not supported AMBIENT_PALETTE\n");
        hres = E_FAIL;
        return S_OK;
    }

    FIXME("(%p) unsupported dispID=%d\n", This, dispID);
    return E_FAIL;
}

static HRESULT WINAPI OleControl_FreezeEvents(IOleControl *iface, BOOL bFreeze)
{
    HTMLDocument *This = CONTROL_THIS(iface);
    FIXME("(%p)->(%x)\n", This, bFreeze);
    return E_NOTIMPL;
}

#undef CONTROL_THIS

static const IOleControlVtbl OleControlVtbl = {
    OleControl_QueryInterface,
    OleControl_AddRef,
    OleControl_Release,
    OleControl_GetControlInfo,
    OleControl_OnMnemonic,
    OleControl_OnAmbientPropertyChange,
    OleControl_FreezeEvents
};

/**********************************************************
 * ICustomDoc implementation
 */

#define CUSTOMDOC_THIS(iface) DEFINE_THIS(HTMLDocument, CustomDoc, iface)

static HRESULT WINAPI CustomDoc_QueryInterface(ICustomDoc *iface, REFIID riid, void **ppv)
{
    HTMLDocument *This = CUSTOMDOC_THIS(iface);
    return IHTMLDocument2_QueryInterface(HTMLDOC(This), riid, ppv);
}

static ULONG WINAPI CustomDoc_AddRef(ICustomDoc *iface)
{
    HTMLDocument *This = CUSTOMDOC_THIS(iface);
    return IHTMLDocument2_AddRef(HTMLDOC(This));
}

static ULONG WINAPI CustomDoc_Release(ICustomDoc *iface)
{
    HTMLDocument *This = CUSTOMDOC_THIS(iface);
    return IHTMLDocument_Release(HTMLDOC(This));
}

static HRESULT WINAPI CustomDoc_SetUIHandler(ICustomDoc *iface, IDocHostUIHandler *pUIHandler)
{
    HTMLDocument *This = CUSTOMDOC_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pUIHandler);
    return E_NOTIMPL;
}

#undef CUSTOMDOC_THIS

static const ICustomDocVtbl CustomDocVtbl = {
    CustomDoc_QueryInterface,
    CustomDoc_AddRef,
    CustomDoc_Release,
    CustomDoc_SetUIHandler
};

void HTMLDocument_LockContainer(HTMLDocument *This, BOOL fLock)
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

void HTMLDocument_OleObj_Init(HTMLDocument *This)
{
    This->lpOleObjectVtbl = &OleObjectVtbl;
    This->lpOleDocumentVtbl = &OleDocumentVtbl;
    This->lpOleControlVtbl = &OleControlVtbl;
    This->lpCustomDocVtbl = &CustomDocVtbl;

    This->usermode = UNKNOWN_USERMODE;

    This->client = NULL;
    This->hostui = NULL;

    This->has_key_path = FALSE;
    This->container_locked = FALSE;

    memset(&This->hostinfo, 0, sizeof(DOCHOSTUIINFO));
}
