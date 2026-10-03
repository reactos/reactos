/*
 * Implementation of IOleObject interfaces for WebBrowser control
 *
 * - IOleObject
 * - IOleInPlaceObject
 * - IOleControl
 *
 * Copyright 2001 John R. Sheets (for CodeWeavers)
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

#include <string.h>

#include "ieframe.h"

#include "htiframe.h"
#include "idispids.h"
#include "mshtmdid.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ieframe);

/* shlwapi.dll */
HWND WINAPI SHSetParentHwnd(HWND hWnd, HWND hWndParent);

static ATOM shell_embedding_atom = 0;

static LRESULT resize_window(WebBrowser *This, LONG width, LONG height)
{
    if(This->doc_host.hwnd)
        SetWindowPos(This->doc_host.hwnd, NULL, 0, 0, width, height,
                     SWP_NOZORDER | SWP_NOACTIVATE);

    return 0;
}

static void notify_on_focus(WebBrowser *This, BOOL got_focus)
{
    IOleControlSite *control_site;
    HRESULT hres;

    if(!This->client)
        return;

    hres = IOleClientSite_QueryInterface(This->client, &IID_IOleControlSite, (void**)&control_site);
    if(FAILED(hres))
        return;

    IOleControlSite_OnFocus(control_site, got_focus);
    IOleControlSite_Release(control_site);
}

static LRESULT WINAPI shell_embedding_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WebBrowser *This;

    static const WCHAR wszTHIS[] = {'T','H','I','S',0};

    if(msg == WM_CREATE) {
        This = *(WebBrowser**)lParam;
        SetPropW(hwnd, wszTHIS, This);
    }else {
        This = GetPropW(hwnd, wszTHIS);
    }

    switch(msg) {
    case WM_SIZE:
        return resize_window(This, LOWORD(lParam), HIWORD(lParam));
    case WM_DOCHOSTTASK:
        return process_dochost_tasks(&This->doc_host);
    case WM_SETFOCUS:
        notify_on_focus(This, TRUE);
        break;
    case WM_KILLFOCUS:
        notify_on_focus(This, FALSE);
        break;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static void create_shell_embedding_hwnd(WebBrowser *This)
{
    IOleInPlaceSite *inplace;
    HWND parent = NULL;
    HRESULT hres;

    static const WCHAR wszShellEmbedding[] =
        {'S','h','e','l','l',' ','E','m','b','e','d','d','i','n','g',0};

    if(!shell_embedding_atom) {
        static WNDCLASSEXW wndclass = {
            sizeof(wndclass),
            CS_DBLCLKS,
            shell_embedding_proc,
            0, 0 /* native uses 8 */, NULL, NULL, NULL,
            (HBRUSH)(COLOR_WINDOW + 1), NULL,
            wszShellEmbedding,
            NULL
        };
        wndclass.hInstance = ieframe_instance;

        RegisterClassExW(&wndclass);
    }

    hres = IOleClientSite_QueryInterface(This->client, &IID_IOleInPlaceSite, (void**)&inplace);
    if(SUCCEEDED(hres)) {
        IOleInPlaceSite_GetWindow(inplace, &parent);
        IOleInPlaceSite_Release(inplace);
    }

    This->doc_host.frame_hwnd = This->shell_embedding_hwnd = CreateWindowExW(
            WS_EX_WINDOWEDGE,
            wszShellEmbedding, wszShellEmbedding,
            WS_CLIPSIBLINGS | WS_CLIPCHILDREN
            | (parent ? WS_CHILD | WS_TABSTOP : WS_POPUP | WS_MAXIMIZEBOX),
            0, 0, 0, 0, parent,
            NULL, ieframe_instance, This);

    TRACE("parent=%p hwnd=%p\n", parent, This->shell_embedding_hwnd);
}

static HRESULT activate_inplace(WebBrowser *This, IOleClientSite *active_site)
{
    HWND parent_hwnd;
    HRESULT hres;

    if(This->inplace)
        return S_OK;

    if(!active_site)
        return E_INVALIDARG;

    hres = IOleClientSite_QueryInterface(active_site, &IID_IOleInPlaceSite,
                                         (void**)&This->inplace);
    if(FAILED(hres)) {
        WARN("Could not get IOleInPlaceSite\n");
        return hres;
    }

    hres = IOleInPlaceSiteEx_CanInPlaceActivate(This->inplace);
    if(hres != S_OK) {
        WARN("CanInPlaceActivate returned: %08lx\n", hres);
        IOleInPlaceSiteEx_Release(This->inplace);
        This->inplace = NULL;
        return E_FAIL;
    }

    hres = IOleInPlaceSiteEx_GetWindow(This->inplace, &parent_hwnd);
    if(SUCCEEDED(hres))
        SHSetParentHwnd(This->shell_embedding_hwnd, parent_hwnd);

    IOleInPlaceSiteEx_OnInPlaceActivate(This->inplace);

    This->frameinfo.cb = sizeof(OLEINPLACEFRAMEINFO);
    IOleInPlaceSiteEx_GetWindowContext(This->inplace, &This->doc_host.frame, &This->uiwindow,
                                       &This->pos_rect, &This->clip_rect,
                                       &This->frameinfo);

    SetWindowPos(This->shell_embedding_hwnd, NULL,
                 This->pos_rect.left, This->pos_rect.top,
                 This->pos_rect.right-This->pos_rect.left,
                 This->pos_rect.bottom-This->pos_rect.top,
                 SWP_NOZORDER | SWP_SHOWWINDOW);

    if(This->client) {
        IOleContainer *container;

        IOleClientSite_ShowObject(This->client);

        hres = IOleClientSite_GetContainer(This->client, &container);
        if(SUCCEEDED(hres)) {
            if(This->container)
                IOleContainer_Release(This->container);
            This->container = container;
        }
    }

    if(This->doc_host.frame)
        IOleInPlaceFrame_GetWindow(This->doc_host.frame, &This->frame_hwnd);

    return S_OK;
}

static HRESULT activate_ui(WebBrowser *This, IOleClientSite *active_site)
{
    HRESULT hres;

    static const WCHAR wszitem[] = {'i','t','e','m',0};

    if(This->inplace)
    {
        if(This->shell_embedding_hwnd)
            ShowWindow(This->shell_embedding_hwnd, SW_SHOW);
        return S_OK;
    }

    hres = activate_inplace(This, active_site);
    if(FAILED(hres))
        return hres;

    if(This->ui_activated)
        return S_OK;

    IOleInPlaceSiteEx_OnUIActivate(This->inplace);

    if(This->doc_host.frame)
        IOleInPlaceFrame_SetActiveObject(This->doc_host.frame, &This->IOleInPlaceActiveObject_iface, wszitem);
    if(This->uiwindow)
        IOleInPlaceUIWindow_SetActiveObject(This->uiwindow, &This->IOleInPlaceActiveObject_iface, wszitem);

    if(This->doc_host.frame)
        IOleInPlaceFrame_SetMenu(This->doc_host.frame, NULL, NULL, This->shell_embedding_hwnd);

    SetFocus(This->shell_embedding_hwnd);
    notify_on_focus(This, TRUE);

    activate_document(&This->doc_host);

    This->ui_activated = TRUE;

    return S_OK;
}

static HRESULT get_client_disp_property(IOleClientSite *client, DISPID dispid, VARIANT *res)
{
    IDispatch *disp = NULL;
    DISPPARAMS dispparams = {NULL, 0};
    HRESULT hres;

    VariantInit(res);

    if(!client)
        return S_OK;

    hres = IOleClientSite_QueryInterface(client, &IID_IDispatch, (void**)&disp);
    if(FAILED(hres)) {
        TRACE("Could not get IDispatch\n");
        return hres;
    }

    hres = IDispatch_Invoke(disp, dispid, &IID_NULL, LOCALE_SYSTEM_DEFAULT,
            DISPATCH_PROPERTYGET, &dispparams, res, NULL, NULL);

    IDispatch_Release(disp);

    return hres;
}

static HRESULT on_offlineconnected_change(WebBrowser *This)
{
    VARIANT offline;

    get_client_disp_property(This->client, DISPID_AMBIENT_OFFLINEIFNOTCONNECTED, &offline);

    if(V_VT(&offline) == VT_BOOL)
        IWebBrowser2_put_Offline(&This->IWebBrowser2_iface, V_BOOL(&offline));
    else if(V_VT(&offline) != VT_EMPTY)
        WARN("wrong V_VT(silent) %d\n", V_VT(&offline));

    return S_OK;
}

static HRESULT on_silent_change(WebBrowser *This)
{
    VARIANT silent;

    get_client_disp_property(This->client, DISPID_AMBIENT_SILENT, &silent);

    if(V_VT(&silent) == VT_BOOL)
        IWebBrowser2_put_Silent(&This->IWebBrowser2_iface, V_BOOL(&silent));
    else if(V_VT(&silent) != VT_EMPTY)
        WARN("wrong V_VT(silent) %d\n", V_VT(&silent));

    return S_OK;
}

static void release_client_site(WebBrowser *This, BOOL destroy_win)
{
    release_dochost_client(&This->doc_host);

    if(This->client) {
        IOleClientSite_Release(This->client);
        This->client = NULL;
    }

    if(This->client_closed) {
        IOleClientSite_Release(This->client_closed);
        This->client_closed = NULL;
    }

    if(destroy_win && This->shell_embedding_hwnd) {
        DestroyWindow(This->shell_embedding_hwnd);
        This->shell_embedding_hwnd = NULL;
    }

    if(This->inplace) {
        IOleInPlaceSiteEx_Release(This->inplace);
        This->inplace = NULL;
    }

    if(This->container) {
        IOleContainer_Release(This->container);
        This->container = NULL;
    }

    if(This->uiwindow) {
        IOleInPlaceUIWindow_Release(This->uiwindow);
        This->uiwindow = NULL;
    }
}

typedef struct {
    IEnumOLEVERB IEnumOLEVERB_iface;
    LONG ref;
    LONG iter;
} EnumOLEVERB;

static inline EnumOLEVERB *impl_from_IEnumOLEVERB(IEnumOLEVERB *iface)
{
    return CONTAINING_RECORD(iface, EnumOLEVERB, IEnumOLEVERB_iface);
}

static HRESULT WINAPI EnumOLEVERB_QueryInterface(IEnumOLEVERB *iface, REFIID riid, void **ppv)
{
    EnumOLEVERB *This = impl_from_IEnumOLEVERB(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IEnumOLEVERB_iface;
    }else if(IsEqualGUID(&IID_IEnumOLEVERB, riid)) {
        TRACE("(%p)->(IID_IEnumOLEVERB %p)\n", This, ppv);
        *ppv = &This->IEnumOLEVERB_iface;
    }else {
        WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI EnumOLEVERB_AddRef(IEnumOLEVERB *iface)
{
    EnumOLEVERB *This = impl_from_IEnumOLEVERB(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI EnumOLEVERB_Release(IEnumOLEVERB *iface)
{
    EnumOLEVERB *This = impl_from_IEnumOLEVERB(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref)
        free(This);

    return ref;
}

static HRESULT WINAPI EnumOLEVERB_Next(IEnumOLEVERB *iface, ULONG celt, OLEVERB *rgelt, ULONG *pceltFetched)
{
    EnumOLEVERB *This = impl_from_IEnumOLEVERB(iface);

    static const OLEVERB verbs[] =
        {{OLEIVERB_PRIMARY},{OLEIVERB_INPLACEACTIVATE},{OLEIVERB_UIACTIVATE},{OLEIVERB_SHOW},{OLEIVERB_HIDE}};

    TRACE("(%p)->(%lu %p %p)\n", This, celt, rgelt, pceltFetched);

    /* There are a few problems with this implementation, but that's how it seems to work in native. See tests. */
    if(pceltFetched)
        *pceltFetched = 0;

    if(This->iter == ARRAY_SIZE(verbs))
        return S_FALSE;

    if(celt)
        *rgelt = verbs[This->iter++];
    return S_OK;
}

static HRESULT WINAPI EnumOLEVERB_Skip(IEnumOLEVERB *iface, ULONG celt)
{
    EnumOLEVERB *This = impl_from_IEnumOLEVERB(iface);
    TRACE("(%p)->(%lu)\n", This, celt);
    return S_OK;
}

static HRESULT WINAPI EnumOLEVERB_Reset(IEnumOLEVERB *iface)
{
    EnumOLEVERB *This = impl_from_IEnumOLEVERB(iface);

    TRACE("(%p)\n", This);

    This->iter = 0;
    return S_OK;
}

static HRESULT WINAPI EnumOLEVERB_Clone(IEnumOLEVERB *iface, IEnumOLEVERB **ppenum)
{
    EnumOLEVERB *This = impl_from_IEnumOLEVERB(iface);
    FIXME("(%p)->(%p)\n", This, ppenum);
    return E_NOTIMPL;
}

static const IEnumOLEVERBVtbl EnumOLEVERBVtbl = {
    EnumOLEVERB_QueryInterface,
    EnumOLEVERB_AddRef,
    EnumOLEVERB_Release,
    EnumOLEVERB_Next,
    EnumOLEVERB_Skip,
    EnumOLEVERB_Reset,
    EnumOLEVERB_Clone
};

/**********************************************************************
 * Implement the IOleObject interface for the WebBrowser control
 */

static inline WebBrowser *impl_from_IOleObject(IOleObject *iface)
{
    return CONTAINING_RECORD(iface, WebBrowser, IOleObject_iface);
}

static HRESULT WINAPI OleObject_QueryInterface(IOleObject *iface, REFIID riid, void **ppv)
{
    WebBrowser *This = impl_from_IOleObject(iface);
    return IUnknown_QueryInterface(This->hlink_frame.outer, riid, ppv);
}

static ULONG WINAPI OleObject_AddRef(IOleObject *iface)
{
    WebBrowser *This = impl_from_IOleObject(iface);
    return IUnknown_AddRef(This->hlink_frame.outer);
}

static ULONG WINAPI OleObject_Release(IOleObject *iface)
{
    WebBrowser *This = impl_from_IOleObject(iface);
    return IUnknown_Release(This->hlink_frame.outer);
}

static HRESULT WINAPI OleObject_SetClientSite(IOleObject *iface, LPOLECLIENTSITE pClientSite)
{
    WebBrowser *This = impl_from_IOleObject(iface);
    IDocHostUIHandler *hostui;
    IOleCommandTarget *olecmd;
    BOOL get_olecmd = TRUE;
    IOleContainer *container;
    IDispatch *disp;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, pClientSite);

    if(This->client_closed) {
        IOleClientSite_Release(This->client_closed);
        This->client_closed = NULL;
    }

    if(This->client == pClientSite)
        return S_OK;

    if(This->client && pClientSite) {
        get_olecmd = FALSE;
        olecmd = This->doc_host.olecmd;
        if(olecmd)
            IOleCommandTarget_AddRef(olecmd);
    }

    release_client_site(This, !pClientSite);

    if(!pClientSite) {
        on_commandstate_change(&This->doc_host, CSC_NAVIGATEBACK, FALSE);
        on_commandstate_change(&This->doc_host, CSC_NAVIGATEFORWARD, FALSE);

        if(This->doc_host.document)
            deactivate_document(&This->doc_host);
        return S_OK;
    }

    IOleClientSite_AddRef(pClientSite);
    This->client = pClientSite;

    hres = IOleClientSite_QueryInterface(This->client, &IID_IDispatch,
            (void**)&disp);
    if(SUCCEEDED(hres))
        This->doc_host.client_disp = disp;

    hres = IOleClientSite_QueryInterface(This->client, &IID_IDocHostUIHandler,
            (void**)&hostui);
    if(SUCCEEDED(hres))
        This->doc_host.hostui = hostui;

    if(get_olecmd) {
        hres = IOleClientSite_GetContainer(This->client, &container);
        if(SUCCEEDED(hres)) {
            ITargetContainer *target_container;

            hres = IOleContainer_QueryInterface(container, &IID_ITargetContainer,
                    (void**)&target_container);
            if(SUCCEEDED(hres)) {
                FIXME("Unsupported ITargetContainer\n");
                ITargetContainer_Release(target_container);
            }

            hres = IOleContainer_QueryInterface(container, &IID_IOleCommandTarget, (void**)&olecmd);
            if(FAILED(hres))
                olecmd = NULL;

            IOleContainer_Release(container);
        }else {
            hres = IOleClientSite_QueryInterface(This->client, &IID_IOleCommandTarget, (void**)&olecmd);
            if(FAILED(hres))
                olecmd = NULL;
        }
    }

    This->doc_host.olecmd = olecmd;

    if(This->shell_embedding_hwnd) {
        IOleInPlaceSite *inplace;
        HWND parent;

        hres = IOleClientSite_QueryInterface(This->client, &IID_IOleInPlaceSite, (void**)&inplace);
        if(SUCCEEDED(hres)) {
            hres = IOleInPlaceSite_GetWindow(inplace, &parent);
            IOleInPlaceSite_Release(inplace);
            if(SUCCEEDED(hres))
                SHSetParentHwnd(This->shell_embedding_hwnd, parent);
        }
    }else {
        create_shell_embedding_hwnd(This);
    }

    on_offlineconnected_change(This);
    on_silent_change(This);
    SetQueryNetSessionCount(SESSION_INCREMENT);

    return S_OK;
}

static HRESULT WINAPI OleObject_GetClientSite(IOleObject *iface, LPOLECLIENTSITE *ppClientSite)
{
    WebBrowser *This = impl_from_IOleObject(iface);

    TRACE("(%p)->(%p)\n", This, ppClientSite);

    if(!ppClientSite)
        return E_INVALIDARG;

    if(This->client)
        IOleClientSite_AddRef(This->client);
    *ppClientSite = This->client;

    return S_OK;
}

static HRESULT WINAPI OleObject_SetHostNames(IOleObject *iface, LPCOLESTR szContainerApp,
        LPCOLESTR szContainerObj)
{
    WebBrowser *This = impl_from_IOleObject(iface);

    TRACE("(%p)->(%s, %s)\n", This, debugstr_w(szContainerApp), debugstr_w(szContainerObj));

    /* We have nothing to do here. */
    return S_OK;
}

static void deactivate_ui(WebBrowser *This)
{
    if(This->ui_activated) {
        if(This->doc_host.frame)
            IOleInPlaceFrame_SetActiveObject(This->doc_host.frame, NULL, NULL);

        if(This->uiwindow)
            IOleInPlaceUIWindow_SetActiveObject(This->uiwindow, NULL, NULL);

        if(This->inplace)
            IOleInPlaceSiteEx_OnUIDeactivate(This->inplace, FALSE);
        notify_on_focus(This, FALSE);

        This->ui_activated = FALSE;
    }

    if(This->inplace)
        IOleInPlaceSiteEx_OnInPlaceDeactivate(This->inplace);
}

static HRESULT WINAPI OleObject_Close(IOleObject *iface, DWORD dwSaveOption)
{
    WebBrowser *This = impl_from_IOleObject(iface);
    IOleClientSite *client;
    HRESULT hres;

    TRACE("(%p)->(%ld)\n", This, dwSaveOption);

    if(dwSaveOption != OLECLOSE_NOSAVE) {
        FIXME("unimplemented flag: %lx\n", dwSaveOption);
        return E_NOTIMPL;
    }

    deactivate_ui(This);

    /* store old client site - we need to restore it in DoVerb */
    client = This->client;
    if(This->client)
        IOleClientSite_AddRef(This->client);
    hres = IOleObject_SetClientSite(iface, NULL);
    This->client_closed = client;

    if(This->advise_holder)
        IOleAdviseHolder_SendOnClose(This->advise_holder);

    return hres;
}

static HRESULT WINAPI OleObject_SetMoniker(IOleObject *iface, DWORD dwWhichMoniker, IMoniker* pmk)
{
    WebBrowser *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%ld, %p)\n", This, dwWhichMoniker, pmk);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetMoniker(IOleObject *iface, DWORD dwAssign,
        DWORD dwWhichMoniker, LPMONIKER *ppmk)
{
    WebBrowser *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%ld, %ld, %p)\n", This, dwAssign, dwWhichMoniker, ppmk);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_InitFromData(IOleObject *iface, LPDATAOBJECT pDataObject,
        BOOL fCreation, DWORD dwReserved)
{
    WebBrowser *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p, %d, %ld)\n", This, pDataObject, fCreation, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetClipboardData(IOleObject *iface, DWORD dwReserved,
        LPDATAOBJECT *ppDataObject)
{
    WebBrowser *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%ld, %p)\n", This, dwReserved, ppDataObject);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_DoVerb(IOleObject *iface, LONG iVerb, struct tagMSG* lpmsg,
        LPOLECLIENTSITE pActiveSite, LONG lindex, HWND hwndParent, LPCRECT lprcPosRect)
{
    WebBrowser *This = impl_from_IOleObject(iface);

    TRACE("(%p)->(%ld %p %p %ld %p %s)\n", This, iVerb, lpmsg, pActiveSite, lindex, hwndParent,
          wine_dbgstr_rect(lprcPosRect));

    /* restore closed client site if we have one */
    if(!This->client && This->client_closed) {
        IOleClientSite *client = This->client_closed;
        This->client_closed = NULL;
        IOleObject_SetClientSite(iface, client);
        IOleClientSite_Release(client);
    }

    switch (iVerb)
    {
    case OLEIVERB_SHOW:
        TRACE("OLEIVERB_SHOW\n");
        return activate_ui(This, pActiveSite);
    case OLEIVERB_UIACTIVATE:
        TRACE("OLEIVERB_UIACTIVATE\n");
        return activate_ui(This, pActiveSite);
    case OLEIVERB_INPLACEACTIVATE:
        TRACE("OLEIVERB_INPLACEACTIVATE\n");
        return activate_inplace(This, pActiveSite);
    case OLEIVERB_HIDE:
        TRACE("OLEIVERB_HIDE\n");
        if(This->inplace) {
            deactivate_ui(This);
            IOleInPlaceSiteEx_Release(This->inplace);
            This->inplace = NULL;
        }
        if(This->shell_embedding_hwnd)
            ShowWindow(This->shell_embedding_hwnd, SW_HIDE);
        return S_OK;
    default:
        FIXME("stub for %ld\n", iVerb);
        break;
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_EnumVerbs(IOleObject *iface, IEnumOLEVERB **ppEnumOleVerb)
{
    WebBrowser *This = impl_from_IOleObject(iface);
    EnumOLEVERB *ret;

    TRACE("(%p)->(%p)\n", This, ppEnumOleVerb);

    ret = malloc(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IEnumOLEVERB_iface.lpVtbl = &EnumOLEVERBVtbl;
    ret->ref = 1;
    ret->iter = 0;

    *ppEnumOleVerb = &ret->IEnumOLEVERB_iface;
    return S_OK;
}

static HRESULT WINAPI OleObject_Update(IOleObject *iface)
{
    WebBrowser *This = impl_from_IOleObject(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_IsUpToDate(IOleObject *iface)
{
    WebBrowser *This = impl_from_IOleObject(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetUserClassID(IOleObject *iface, CLSID *pClsid)
{
    WebBrowser *This = impl_from_IOleObject(iface);

    TRACE("(%p)->(%p)\n", This, pClsid);

    *pClsid = This->version == 1 ? CLSID_WebBrowser_V1 : CLSID_WebBrowser;
    return S_OK;
}

static HRESULT WINAPI OleObject_GetUserType(IOleObject *iface, DWORD dwFormOfType,
        LPOLESTR* pszUserType)
{
    WebBrowser *This = impl_from_IOleObject(iface);
    TRACE("(%p, %ld, %p)\n", This, dwFormOfType, pszUserType);
    return OleRegGetUserType(&CLSID_WebBrowser, dwFormOfType, pszUserType);
}

static HRESULT WINAPI OleObject_SetExtent(IOleObject *iface, DWORD dwDrawAspect, SIZEL *psizel)
{
    WebBrowser *This = impl_from_IOleObject(iface);

    TRACE("(%p)->(%lx %p)\n", This, dwDrawAspect, psizel);

    /* Tests show that dwDrawAspect is ignored */
    This->extent = *psizel;
    return S_OK;
}

static HRESULT WINAPI OleObject_GetExtent(IOleObject *iface, DWORD dwDrawAspect, SIZEL *psizel)
{
    WebBrowser *This = impl_from_IOleObject(iface);

    TRACE("(%p)->(%lx, %p)\n", This, dwDrawAspect, psizel);

    /* Tests show that dwDrawAspect is ignored */
    *psizel = This->extent;
    return S_OK;
}

static HRESULT WINAPI OleObject_Advise(IOleObject *iface, IAdviseSink *pAdvSink,
        DWORD *pdwConnection)
{
    WebBrowser *This = impl_from_IOleObject(iface);
    HRESULT hr = S_OK;

    TRACE("(%p)->(%p, %p)\n", This, pAdvSink, pdwConnection);

    if(!pdwConnection)
        return E_INVALIDARG;

    *pdwConnection = 0;

    if(!pAdvSink)
        return E_INVALIDARG;

    if(!This->advise_holder)
        hr = CreateOleAdviseHolder(&This->advise_holder);

    if(hr == S_OK)
        hr = IOleAdviseHolder_Advise(This->advise_holder, pAdvSink, pdwConnection);

    return hr;
}

static HRESULT WINAPI OleObject_Unadvise(IOleObject *iface, DWORD dwConnection)
{
    WebBrowser *This = impl_from_IOleObject(iface);

    TRACE("(%p)->(%ld)\n", This, dwConnection);

    if(!This->advise_holder)
        return OLE_E_NOCONNECTION;

    return IOleAdviseHolder_Unadvise(This->advise_holder, dwConnection);
}

static HRESULT WINAPI OleObject_EnumAdvise(IOleObject *iface, IEnumSTATDATA **ppenumAdvise)
{
    WebBrowser *This = impl_from_IOleObject(iface);

    TRACE("(%p)->(%p): returning E_NOTIMPL\n", This, ppenumAdvise);

    *ppenumAdvise = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetMiscStatus(IOleObject *iface, DWORD dwAspect, DWORD *pdwStatus)
{
    WebBrowser *This = impl_from_IOleObject(iface);

    TRACE("(%p)->(%lx, %p)\n", This, dwAspect, pdwStatus);

    *pdwStatus = OLEMISC_SETCLIENTSITEFIRST|OLEMISC_ACTIVATEWHENVISIBLE|OLEMISC_INSIDEOUT
        |OLEMISC_CANTLINKINSIDE|OLEMISC_RECOMPOSEONRESIZE;

    return S_OK;
}

static HRESULT WINAPI OleObject_SetColorScheme(IOleObject *iface, LOGPALETTE* pLogpal)
{
    WebBrowser *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p)\n", This, pLogpal);
    return E_NOTIMPL;
}

static const IOleObjectVtbl OleObjectVtbl =
{
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

/**********************************************************************
 * Implement the IOleInPlaceObject interface
 */

static inline WebBrowser *impl_from_IOleInPlaceObject(IOleInPlaceObject *iface)
{
    return CONTAINING_RECORD(iface, WebBrowser, IOleInPlaceObject_iface);
}

static HRESULT WINAPI OleInPlaceObject_QueryInterface(IOleInPlaceObject *iface,
        REFIID riid, LPVOID *ppobj)
{
    WebBrowser *This = impl_from_IOleInPlaceObject(iface);
    return IUnknown_QueryInterface(This->hlink_frame.outer, riid, ppobj);
}

static ULONG WINAPI OleInPlaceObject_AddRef(IOleInPlaceObject *iface)
{
    WebBrowser *This = impl_from_IOleInPlaceObject(iface);
    return IUnknown_AddRef(This->hlink_frame.outer);
}

static ULONG WINAPI OleInPlaceObject_Release(IOleInPlaceObject *iface)
{
    WebBrowser *This = impl_from_IOleInPlaceObject(iface);
    return IUnknown_Release(This->hlink_frame.outer);
}

static HRESULT WINAPI OleInPlaceObject_GetWindow(IOleInPlaceObject *iface, HWND* phwnd)
{
    WebBrowser *This = impl_from_IOleInPlaceObject(iface);

    TRACE("(%p)->(%p)\n", This, phwnd);

    *phwnd = This->shell_embedding_hwnd;
    return S_OK;
}

static HRESULT WINAPI OleInPlaceObject_ContextSensitiveHelp(IOleInPlaceObject *iface,
        BOOL fEnterMode)
{
    WebBrowser *This = impl_from_IOleInPlaceObject(iface);
    FIXME("(%p)->(%x)\n", This, fEnterMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObject_InPlaceDeactivate(IOleInPlaceObject *iface)
{
    WebBrowser *This = impl_from_IOleInPlaceObject(iface);
    FIXME("(%p)\n", This);

    if(This->inplace) {
        IOleInPlaceSiteEx_Release(This->inplace);
        This->inplace = NULL;
    }

    return S_OK;
}

static HRESULT WINAPI OleInPlaceObject_UIDeactivate(IOleInPlaceObject *iface)
{
    WebBrowser *This = impl_from_IOleInPlaceObject(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObject_SetObjectRects(IOleInPlaceObject *iface,
        LPCRECT lprcPosRect, LPCRECT lprcClipRect)
{
    WebBrowser *This = impl_from_IOleInPlaceObject(iface);

    TRACE("(%p)->(%s %s)\n", This, wine_dbgstr_rect(lprcPosRect), wine_dbgstr_rect(lprcClipRect));

    This->pos_rect = *lprcPosRect;

    if(lprcClipRect)
        This->clip_rect = *lprcClipRect;

    if(This->shell_embedding_hwnd) {
        SetWindowPos(This->shell_embedding_hwnd, NULL,
                     lprcPosRect->left, lprcPosRect->top,
                     lprcPosRect->right-lprcPosRect->left,
                     lprcPosRect->bottom-lprcPosRect->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    }

    return S_OK;
}

static HRESULT WINAPI OleInPlaceObject_ReactivateAndUndo(IOleInPlaceObject *iface)
{
    WebBrowser *This = impl_from_IOleInPlaceObject(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static const IOleInPlaceObjectVtbl OleInPlaceObjectVtbl =
{
    OleInPlaceObject_QueryInterface,
    OleInPlaceObject_AddRef,
    OleInPlaceObject_Release,
    OleInPlaceObject_GetWindow,
    OleInPlaceObject_ContextSensitiveHelp,
    OleInPlaceObject_InPlaceDeactivate,
    OleInPlaceObject_UIDeactivate,
    OleInPlaceObject_SetObjectRects,
    OleInPlaceObject_ReactivateAndUndo
};

/**********************************************************************
 * Implement the IOleControl interface
 */

static inline WebBrowser *impl_from_IOleControl(IOleControl *iface)
{
    return CONTAINING_RECORD(iface, WebBrowser, IOleControl_iface);
}

static HRESULT WINAPI OleControl_QueryInterface(IOleControl *iface,
        REFIID riid, LPVOID *ppobj)
{
    WebBrowser *This = impl_from_IOleControl(iface);
    return IUnknown_QueryInterface(This->hlink_frame.outer, riid, ppobj);
}

static ULONG WINAPI OleControl_AddRef(IOleControl *iface)
{
    WebBrowser *This = impl_from_IOleControl(iface);
    return IUnknown_AddRef(This->hlink_frame.outer);
}

static ULONG WINAPI OleControl_Release(IOleControl *iface)
{
    WebBrowser *This = impl_from_IOleControl(iface);
    return IUnknown_Release(This->hlink_frame.outer);
}

static HRESULT WINAPI OleControl_GetControlInfo(IOleControl *iface, LPCONTROLINFO pCI)
{
    WebBrowser *This = impl_from_IOleControl(iface);

    TRACE("(%p)->(%p)\n", This, pCI);

    /* Tests show that this function should be not implemented */
    return E_NOTIMPL;
}

static HRESULT WINAPI OleControl_OnMnemonic(IOleControl *iface, struct tagMSG *pMsg)
{
    WebBrowser *This = impl_from_IOleControl(iface);
    FIXME("(%p)->(%p)\n", This, pMsg);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleControl_OnAmbientPropertyChange(IOleControl *iface, DISPID dispID)
{
    WebBrowser *This = impl_from_IOleControl(iface);

    TRACE("(%p)->(%ld)\n", This, dispID);

    switch(dispID) {
    case DISPID_UNKNOWN:
        /* Unknown means multiple properties changed, so check them all.
         * BUT the Webbrowser OleControl object doesn't appear to do this.
         */
        return S_OK;
    case DISPID_AMBIENT_DLCONTROL:
        return S_OK;
    case DISPID_AMBIENT_OFFLINEIFNOTCONNECTED:
        return on_offlineconnected_change(This);
    case DISPID_AMBIENT_SILENT:
        return on_silent_change(This);
    }

    FIXME("Unknown dispID %ld\n", dispID);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleControl_FreezeEvents(IOleControl *iface, BOOL bFreeze)
{
    WebBrowser *This = impl_from_IOleControl(iface);
    FIXME("(%p)->(%x)\n", This, bFreeze);
    return E_NOTIMPL;
}

static const IOleControlVtbl OleControlVtbl =
{
    OleControl_QueryInterface,
    OleControl_AddRef,
    OleControl_Release,
    OleControl_GetControlInfo,
    OleControl_OnMnemonic,
    OleControl_OnAmbientPropertyChange,
    OleControl_FreezeEvents
};

static inline WebBrowser *impl_from_IOleInPlaceActiveObject(IOleInPlaceActiveObject *iface)
{
    return CONTAINING_RECORD(iface, WebBrowser, IOleInPlaceActiveObject_iface);
}

static HRESULT WINAPI InPlaceActiveObject_QueryInterface(IOleInPlaceActiveObject *iface,
        REFIID riid, void **ppv)
{
    WebBrowser *This = impl_from_IOleInPlaceActiveObject(iface);
    return IUnknown_QueryInterface(This->hlink_frame.outer, riid, ppv);
}

static ULONG WINAPI InPlaceActiveObject_AddRef(IOleInPlaceActiveObject *iface)
{
    WebBrowser *This = impl_from_IOleInPlaceActiveObject(iface);
    return IUnknown_AddRef(This->hlink_frame.outer);
}

static ULONG WINAPI InPlaceActiveObject_Release(IOleInPlaceActiveObject *iface)
{
    WebBrowser *This = impl_from_IOleInPlaceActiveObject(iface);
    return IUnknown_Release(This->hlink_frame.outer);
}

static HRESULT WINAPI InPlaceActiveObject_GetWindow(IOleInPlaceActiveObject *iface,
                                                    HWND *phwnd)
{
    WebBrowser *This = impl_from_IOleInPlaceActiveObject(iface);
    return IOleInPlaceObject_GetWindow(&This->IOleInPlaceObject_iface, phwnd);
}

static HRESULT WINAPI InPlaceActiveObject_ContextSensitiveHelp(IOleInPlaceActiveObject *iface,
                                                               BOOL fEnterMode)
{
    WebBrowser *This = impl_from_IOleInPlaceActiveObject(iface);
    return IOleInPlaceObject_ContextSensitiveHelp(&This->IOleInPlaceObject_iface, fEnterMode);
}

static HRESULT WINAPI InPlaceActiveObject_TranslateAccelerator(IOleInPlaceActiveObject *iface,
                                                               LPMSG lpmsg)
{
    WebBrowser *This = impl_from_IOleInPlaceActiveObject(iface);
    IOleInPlaceActiveObject *activeobj;
    HRESULT hr = S_FALSE;

    TRACE("(%p)->(%p)\n", This, lpmsg);

    if(This->doc_host.document) {
        if(SUCCEEDED(IUnknown_QueryInterface(This->doc_host.document,
                                             &IID_IOleInPlaceActiveObject,
                                             (void**)&activeobj))) {
            hr = IOleInPlaceActiveObject_TranslateAccelerator(activeobj, lpmsg);
            IOleInPlaceActiveObject_Release(activeobj);
        }
    }

    if(SUCCEEDED(hr))
        return hr;
    else
        return S_FALSE;
}

static HRESULT WINAPI InPlaceActiveObject_OnFrameWindowActivate(IOleInPlaceActiveObject *iface,
                                                                BOOL fActivate)
{
    WebBrowser *This = impl_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%x)\n", This, fActivate);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceActiveObject_OnDocWindowActivate(IOleInPlaceActiveObject *iface,
                                                              BOOL fActivate)
{
    WebBrowser *This = impl_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%x)\n", This, fActivate);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceActiveObject_ResizeBorder(IOleInPlaceActiveObject *iface,
        LPCRECT lprcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fFrameWindow)
{
    WebBrowser *This = impl_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%p %p %x)\n", This, lprcBorder, pUIWindow, fFrameWindow);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceActiveObject_EnableModeless(IOleInPlaceActiveObject *iface,
                                                         BOOL fEnable)
{
    WebBrowser *This = impl_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%x)\n", This, fEnable);
    return E_NOTIMPL;
}

static const IOleInPlaceActiveObjectVtbl OleInPlaceActiveObjectVtbl = {
    InPlaceActiveObject_QueryInterface,
    InPlaceActiveObject_AddRef,
    InPlaceActiveObject_Release,
    InPlaceActiveObject_GetWindow,
    InPlaceActiveObject_ContextSensitiveHelp,
    InPlaceActiveObject_TranslateAccelerator,
    InPlaceActiveObject_OnFrameWindowActivate,
    InPlaceActiveObject_OnDocWindowActivate,
    InPlaceActiveObject_ResizeBorder,
    InPlaceActiveObject_EnableModeless
};

static inline WebBrowser *impl_from_IOleCommandTarget(IOleCommandTarget *iface)
{
    return CONTAINING_RECORD(iface, WebBrowser, IOleCommandTarget_iface);
}

static HRESULT WINAPI WBOleCommandTarget_QueryInterface(IOleCommandTarget *iface,
        REFIID riid, void **ppv)
{
    WebBrowser *This = impl_from_IOleCommandTarget(iface);
    return IUnknown_QueryInterface(This->hlink_frame.outer, riid, ppv);
}

static ULONG WINAPI WBOleCommandTarget_AddRef(IOleCommandTarget *iface)
{
    WebBrowser *This = impl_from_IOleCommandTarget(iface);
    return IUnknown_AddRef(This->hlink_frame.outer);
}

static ULONG WINAPI WBOleCommandTarget_Release(IOleCommandTarget *iface)
{
    WebBrowser *This = impl_from_IOleCommandTarget(iface);
    return IUnknown_Release(This->hlink_frame.outer);
}

static HRESULT WINAPI WBOleCommandTarget_QueryStatus(IOleCommandTarget *iface,
        const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    WebBrowser *This = impl_from_IOleCommandTarget(iface);
    IOleCommandTarget *cmdtrg;
    HRESULT hres;

    TRACE("(%p)->(%s %lu %p %p)\n", This, debugstr_guid(pguidCmdGroup), cCmds, prgCmds,
          pCmdText);

    if(!This->doc_host.document)
        return 0x80040104;

    /* NOTE: There are probably some commands that we should handle here
     * instead of forwarding to document object. */

    hres = IUnknown_QueryInterface(This->doc_host.document, &IID_IOleCommandTarget, (void**)&cmdtrg);
    if(FAILED(hres))
        return hres;

    hres = IOleCommandTarget_QueryStatus(cmdtrg, pguidCmdGroup, cCmds, prgCmds, pCmdText);
    IOleCommandTarget_Release(cmdtrg);

    return hres;
}

static HRESULT WINAPI WBOleCommandTarget_Exec(IOleCommandTarget *iface,
        const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn,
        VARIANT *pvaOut)
{
    WebBrowser *This = impl_from_IOleCommandTarget(iface);
    FIXME("(%p)->(%s %ld %ld %s %p)\n", This, debugstr_guid(pguidCmdGroup), nCmdID,
          nCmdexecopt, debugstr_variant(pvaIn), pvaOut);
    return E_NOTIMPL;
}

static const IOleCommandTargetVtbl OleCommandTargetVtbl = {
    WBOleCommandTarget_QueryInterface,
    WBOleCommandTarget_AddRef,
    WBOleCommandTarget_Release,
    WBOleCommandTarget_QueryStatus,
    WBOleCommandTarget_Exec
};

void WebBrowser_OleObject_Init(WebBrowser *This)
{
    DWORD dpi_x;
    DWORD dpi_y;
    HDC hdc;

    /* default aspect ratio is 96dpi / 96dpi */
    hdc = GetDC(0);
    dpi_x = GetDeviceCaps(hdc, LOGPIXELSX);
    dpi_y = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(0, hdc);

    This->IOleObject_iface.lpVtbl              = &OleObjectVtbl;
    This->IOleInPlaceObject_iface.lpVtbl       = &OleInPlaceObjectVtbl;
    This->IOleControl_iface.lpVtbl             = &OleControlVtbl;
    This->IOleInPlaceActiveObject_iface.lpVtbl = &OleInPlaceActiveObjectVtbl;
    This->IOleCommandTarget_iface.lpVtbl       = &OleCommandTargetVtbl;

    /* Default size is 50x20 pixels, in himetric units */
    This->extent.cx = MulDiv( 50, 2540, dpi_x );
    This->extent.cy = MulDiv( 20, 2540, dpi_y );
}

void WebBrowser_OleObject_Destroy(WebBrowser *This)
{
    release_client_site(This, TRUE);
}
