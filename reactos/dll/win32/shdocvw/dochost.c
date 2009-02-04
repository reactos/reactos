/*
 * Copyright 2005-2006 Jacek Caban for CodeWeavers
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

#include "wine/debug.h"
#include "shdocvw.h"
#include "hlink.h"
#include "exdispid.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

static ATOM doc_view_atom = 0;

void push_dochost_task(DocHost *This, task_header_t *task, task_proc_t proc, BOOL send)
{
    task->proc = proc;

    /* FIXME: Don't use lParam */
    if(send)
        SendMessageW(This->frame_hwnd, WM_DOCHOSTTASK, 0, (LPARAM)task);
    else
        PostMessageW(This->frame_hwnd, WM_DOCHOSTTASK, 0, (LPARAM)task);
}

LRESULT process_dochost_task(DocHost *This, LPARAM lparam)
{
    task_header_t *task = (task_header_t*)lparam;

    task->proc(This, task);

    heap_free(task);
    return 0;
}

static void navigate_complete(DocHost *This)
{
    DISPPARAMS dispparams;
    VARIANTARG params[2];
    VARIANT url;

    dispparams.cArgs = 2;
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = params;

    V_VT(params) = (VT_BYREF|VT_VARIANT);
    V_BYREF(params) = &url;

    V_VT(params+1) = VT_DISPATCH;
    V_DISPATCH(params+1) = This->disp;

    V_VT(&url) = VT_BSTR;
    V_BSTR(&url) = SysAllocString(This->url);

    call_sink(This->cps.wbe2, DISPID_NAVIGATECOMPLETE2, &dispparams);
    call_sink(This->cps.wbe2, DISPID_DOCUMENTCOMPLETE, &dispparams);

    SysFreeString(V_BSTR(&url));
    This->busy = VARIANT_FALSE;
}

void object_available(DocHost *This)
{
    IHlinkTarget *hlink;
    HRESULT hres;

    TRACE("(%p)\n", This);

    if(!This->document) {
        WARN("document == NULL\n");
        return;
    }

    hres = IUnknown_QueryInterface(This->document, &IID_IHlinkTarget, (void**)&hlink);
    if(FAILED(hres)) {
        FIXME("Could not get IHlinkTarget interface\n");
        return;
    }

    hres = IHlinkTarget_Navigate(hlink, 0, NULL);
    IHlinkTarget_Release(hlink);
    if(FAILED(hres)) {
        FIXME("Navigate failed\n");
        return;
    }

    navigate_complete(This);

    return;
}

static LRESULT resize_document(DocHost *This, LONG width, LONG height)
{
    RECT rect = {0, 0, width, height};

    TRACE("(%p)->(%d %d)\n", This, width, height);

    if(This->view)
        IOleDocumentView_SetRect(This->view, &rect);

    return 0;
}

static LRESULT WINAPI doc_view_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    DocHost *This;

    static const WCHAR wszTHIS[] = {'T','H','I','S',0};

    if(msg == WM_CREATE) {
        This = *(DocHost**)lParam;
        SetPropW(hwnd, wszTHIS, This);
    }else {
        This = GetPropW(hwnd, wszTHIS);
    }

    switch(msg) {
    case WM_SIZE:
        return resize_document(This, LOWORD(lParam), HIWORD(lParam));
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void create_doc_view_hwnd(DocHost *This)
{
    RECT rect;

    static const WCHAR wszShell_DocObject_View[] =
        {'S','h','e','l','l',' ','D','o','c','O','b','j','e','c','t',' ','V','i','e','w',0};

    if(!doc_view_atom) {
        static WNDCLASSEXW wndclass = {
            sizeof(wndclass),
            CS_PARENTDC,
            doc_view_proc,
            0, 0 /* native uses 4*/, NULL, NULL, NULL,
            (HBRUSH)(COLOR_WINDOW + 1), NULL,
            wszShell_DocObject_View,
            NULL
        };

        wndclass.hInstance = shdocvw_hinstance;

        doc_view_atom = RegisterClassExW(&wndclass);
    }

    GetClientRect(This->frame_hwnd, &rect); /* FIXME */
    This->hwnd = CreateWindowExW(0, wszShell_DocObject_View,
         wszShell_DocObject_View,
         WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TABSTOP,
         rect.left, rect.top, rect.right, rect.bottom, This->frame_hwnd,
         NULL, shdocvw_hinstance, This);
}

void deactivate_document(DocHost *This)
{
    IOleInPlaceObjectWindowless *winobj;
    IOleObject *oleobj = NULL;
    IHlinkTarget *hlink = NULL;
    HRESULT hres;

    if(This->view)
        IOleDocumentView_UIActivate(This->view, FALSE);

    hres = IUnknown_QueryInterface(This->document, &IID_IOleInPlaceObjectWindowless,
                                   (void**)&winobj);
    if(SUCCEEDED(hres)) {
        IOleInPlaceObjectWindowless_InPlaceDeactivate(winobj);
        IOleInPlaceObjectWindowless_Release(winobj);
    }

    if(This->view) {
        IOleDocumentView_Show(This->view, FALSE);
        IOleDocumentView_CloseView(This->view, 0);
        IOleDocumentView_SetInPlaceSite(This->view, NULL);
        IOleDocumentView_Release(This->view);
        This->view = NULL;
    }

    hres = IUnknown_QueryInterface(This->document, &IID_IOleObject, (void**)&oleobj);
    if(SUCCEEDED(hres))
        IOleObject_Close(oleobj, OLECLOSE_NOSAVE);

    hres = IUnknown_QueryInterface(This->document, &IID_IHlinkTarget, (void**)&hlink);
    if(SUCCEEDED(hres)) {
        IHlinkTarget_SetBrowseContext(hlink, NULL);
        IHlinkTarget_Release(hlink);
    }

    if(oleobj) {
        IOleClientSite *client_site = NULL;

        IOleObject_GetClientSite(oleobj, &client_site);
        if(client_site) {
            if(client_site == CLIENTSITE(This))
                IOleObject_SetClientSite(oleobj, NULL);
            IOleClientSite_Release(client_site);
        }

        IOleObject_Release(oleobj);
    }

    IUnknown_Release(This->document);
    This->document = NULL;
}

#define OLECMD_THIS(iface) DEFINE_THIS(DocHost, OleCommandTarget, iface)

static HRESULT WINAPI ClOleCommandTarget_QueryInterface(IOleCommandTarget *iface,
        REFIID riid, void **ppv)
{
    DocHost *This = OLECMD_THIS(iface);
    return IOleClientSite_QueryInterface(CLIENTSITE(This), riid, ppv);
}

static ULONG WINAPI ClOleCommandTarget_AddRef(IOleCommandTarget *iface)
{
    DocHost *This = OLECMD_THIS(iface);
    return IOleClientSite_AddRef(CLIENTSITE(This));
}

static ULONG WINAPI ClOleCommandTarget_Release(IOleCommandTarget *iface)
{
    DocHost *This = OLECMD_THIS(iface);
    return IOleClientSite_Release(CLIENTSITE(This));
}

static HRESULT WINAPI ClOleCommandTarget_QueryStatus(IOleCommandTarget *iface,
        const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    DocHost *This = OLECMD_THIS(iface);
    FIXME("(%p)->(%s %u %p %p)\n", This, debugstr_guid(pguidCmdGroup), cCmds, prgCmds,
          pCmdText);
    return E_NOTIMPL;
}

static HRESULT WINAPI ClOleCommandTarget_Exec(IOleCommandTarget *iface,
        const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn,
        VARIANT *pvaOut)
{
    DocHost *This = OLECMD_THIS(iface);
    FIXME("(%p)->(%s %d %d %p %p)\n", This, debugstr_guid(pguidCmdGroup), nCmdID,
          nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

#undef OLECMD_THIS

static const IOleCommandTargetVtbl OleCommandTargetVtbl = {
    ClOleCommandTarget_QueryInterface,
    ClOleCommandTarget_AddRef,
    ClOleCommandTarget_Release,
    ClOleCommandTarget_QueryStatus,
    ClOleCommandTarget_Exec
};

#define DOCHOSTUI_THIS(iface) DEFINE_THIS(DocHost, DocHostUIHandler, iface)

static HRESULT WINAPI DocHostUIHandler_QueryInterface(IDocHostUIHandler2 *iface,
                                                      REFIID riid, void **ppv)
{
    DocHost *This = DOCHOSTUI_THIS(iface);
    return IOleClientSite_QueryInterface(CLIENTSITE(This), riid, ppv);
}

static ULONG WINAPI DocHostUIHandler_AddRef(IDocHostUIHandler2 *iface)
{
    DocHost *This = DOCHOSTUI_THIS(iface);
    return IOleClientSite_AddRef(CLIENTSITE(This));
}

static ULONG WINAPI DocHostUIHandler_Release(IDocHostUIHandler2 *iface)
{
    DocHost *This = DOCHOSTUI_THIS(iface);
    return IOleClientSite_Release(CLIENTSITE(This));
}

static HRESULT WINAPI DocHostUIHandler_ShowContextMenu(IDocHostUIHandler2 *iface,
         DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved)
{
    DocHost *This = DOCHOSTUI_THIS(iface);
    HRESULT hres;

    TRACE("(%p)->(%d %p %p %p)\n", This, dwID, ppt, pcmdtReserved, pdispReserved);

    if(This->hostui) {
        hres = IDocHostUIHandler_ShowContextMenu(This->hostui, dwID, ppt, pcmdtReserved,
                                                 pdispReserved);
        if(hres == S_OK)
            return S_OK;
    }

    FIXME("default action not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetHostInfo(IDocHostUIHandler2 *iface,
        DOCHOSTUIINFO *pInfo)
{
    DocHost *This = DOCHOSTUI_THIS(iface);
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, pInfo);

    if(This->hostui) {
        hres = IDocHostUIHandler_GetHostInfo(This->hostui, pInfo);
        if(SUCCEEDED(hres))
            return hres;
    }

    pInfo->dwFlags = DOCHOSTUIFLAG_DISABLE_HELP_MENU | DOCHOSTUIFLAG_OPENNEWWIN
        | DOCHOSTUIFLAG_URL_ENCODING_ENABLE_UTF8 | DOCHOSTUIFLAG_ENABLE_INPLACE_NAVIGATION
        | DOCHOSTUIFLAG_IME_ENABLE_RECONVERSION;
    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_ShowUI(IDocHostUIHandler2 *iface, DWORD dwID,
        IOleInPlaceActiveObject *pActiveObject, IOleCommandTarget *pCommandTarget,
        IOleInPlaceFrame *pFrame, IOleInPlaceUIWindow *pDoc)
{
    DocHost *This = DOCHOSTUI_THIS(iface);
    FIXME("(%p)->(%d %p %p %p %p)\n", This, dwID, pActiveObject, pCommandTarget,
          pFrame, pDoc);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_HideUI(IDocHostUIHandler2 *iface)
{
    DocHost *This = DOCHOSTUI_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_UpdateUI(IDocHostUIHandler2 *iface)
{
    DocHost *This = DOCHOSTUI_THIS(iface);

    TRACE("(%p)\n", This);

    if(!This->hostui)
        return S_FALSE;

    return IDocHostUIHandler_UpdateUI(This->hostui);
}

static HRESULT WINAPI DocHostUIHandler_EnableModeless(IDocHostUIHandler2 *iface,
                                                      BOOL fEnable)
{
    DocHost *This = DOCHOSTUI_THIS(iface);
    FIXME("(%p)->(%x)\n", This, fEnable);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_OnDocWindowActivate(IDocHostUIHandler2 *iface,
                                                           BOOL fActivate)
{
    DocHost *This = DOCHOSTUI_THIS(iface);
    FIXME("(%p)->(%x)\n", This, fActivate);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_OnFrameWindowActivate(IDocHostUIHandler2 *iface,
                                                             BOOL fActivate)
{
    DocHost *This = DOCHOSTUI_THIS(iface);
    FIXME("(%p)->(%x)\n", This, fActivate);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_ResizeBorder(IDocHostUIHandler2 *iface,
        LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow)
{
    DocHost *This = DOCHOSTUI_THIS(iface);
    FIXME("(%p)->(%p %p %X)\n", This, prcBorder, pUIWindow, fRameWindow);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_TranslateAccelerator(IDocHostUIHandler2 *iface,
        LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID)
{
    DocHost *This = DOCHOSTUI_THIS(iface);
    FIXME("(%p)->(%p %p %d)\n", This, lpMsg, pguidCmdGroup, nCmdID);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetOptionKeyPath(IDocHostUIHandler2 *iface,
        LPOLESTR *pchKey, DWORD dw)
{
    DocHost *This = DOCHOSTUI_THIS(iface);

    TRACE("(%p)->(%p %d)\n", This, pchKey, dw);

    if(This->hostui)
        return IDocHostUIHandler_GetOptionKeyPath(This->hostui, pchKey, dw);

    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_GetDropTarget(IDocHostUIHandler2 *iface,
        IDropTarget *pDropTarget, IDropTarget **ppDropTarget)
{
    DocHost *This = DOCHOSTUI_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetExternal(IDocHostUIHandler2 *iface,
        IDispatch **ppDispatch)
{
    DocHost *This = DOCHOSTUI_THIS(iface);

    TRACE("(%p)->(%p)\n", This, ppDispatch);

    if(This->hostui)
        return IDocHostUIHandler_GetExternal(This->hostui, ppDispatch);

    FIXME("default action not implemented\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_TranslateUrl(IDocHostUIHandler2 *iface,
        DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut)
{
    DocHost *This = DOCHOSTUI_THIS(iface);

    TRACE("(%p)->(%d %s %p)\n", This, dwTranslate, debugstr_w(pchURLIn), ppchURLOut);

    if(This->hostui)
        return IDocHostUIHandler_TranslateUrl(This->hostui, dwTranslate,
                                              pchURLIn, ppchURLOut);

    return S_FALSE;
}

static HRESULT WINAPI DocHostUIHandler_FilterDataObject(IDocHostUIHandler2 *iface,
        IDataObject *pDO, IDataObject **ppDORet)
{
    DocHost *This = DOCHOSTUI_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pDO, ppDORet);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetOverrideKeyPath(IDocHostUIHandler2 *iface,
        LPOLESTR *pchKey, DWORD dw)
{
    DocHost *This = DOCHOSTUI_THIS(iface);
    IDocHostUIHandler2 *handler;
    HRESULT hres;

    TRACE("(%p)->(%p %d)\n", This, pchKey, dw);

    if(!This->hostui)
        return S_OK;

    hres = IDocHostUIHandler_QueryInterface(This->hostui, &IID_IDocHostUIHandler2,
                                            (void**)&handler);
    if(SUCCEEDED(hres)) {
        hres = IDocHostUIHandler2_GetOverrideKeyPath(handler, pchKey, dw);
        IDocHostUIHandler2_Release(handler);
        return hres;
    }

    return S_OK;
}

#undef DOCHOSTUI_THIS

static const IDocHostUIHandler2Vtbl DocHostUIHandler2Vtbl = {
    DocHostUIHandler_QueryInterface,
    DocHostUIHandler_AddRef,
    DocHostUIHandler_Release,
    DocHostUIHandler_ShowContextMenu,
    DocHostUIHandler_GetHostInfo,
    DocHostUIHandler_ShowUI,
    DocHostUIHandler_HideUI,
    DocHostUIHandler_UpdateUI,
    DocHostUIHandler_EnableModeless,
    DocHostUIHandler_OnDocWindowActivate,
    DocHostUIHandler_OnFrameWindowActivate,
    DocHostUIHandler_ResizeBorder,
    DocHostUIHandler_TranslateAccelerator,
    DocHostUIHandler_GetOptionKeyPath,
    DocHostUIHandler_GetDropTarget,
    DocHostUIHandler_GetExternal,
    DocHostUIHandler_TranslateUrl,
    DocHostUIHandler_FilterDataObject,
    DocHostUIHandler_GetOverrideKeyPath
};

void DocHost_Init(DocHost *This, IDispatch *disp)
{
    This->lpDocHostUIHandlerVtbl = &DocHostUIHandler2Vtbl;
    This->lpOleCommandTargetVtbl = &OleCommandTargetVtbl;

    This->disp = disp;

    This->client_disp = NULL;

    This->document = NULL;
    This->hostui = NULL;
    This->frame = NULL;

    This->hwnd = NULL;
    This->frame_hwnd = NULL;
    This->url = NULL;

    This->silent = VARIANT_FALSE;
    This->offline = VARIANT_FALSE;

    DocHost_ClientSite_Init(This);
    DocHost_Frame_Init(This);

    ConnectionPointContainer_Init(&This->cps, (IUnknown*)disp);
}

void DocHost_Release(DocHost *This)
{
    if(This->client_disp)
        IDispatch_Release(This->client_disp);
    if(This->frame)
        IOleInPlaceFrame_Release(This->frame);

    DocHost_ClientSite_Release(This);

    ConnectionPointContainer_Destroy(&This->cps);

    SysFreeString(This->url);
}
