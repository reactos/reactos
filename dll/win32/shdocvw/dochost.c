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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "wine/debug.h"
#include "shdocvw.h"
#include "hlink.h"
#include "exdispid.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

static ATOM doc_view_atom = 0;

static void navigate_complete(WebBrowser *This)
{
    IDispatch *disp = NULL;
    DISPPARAMS dispparams;
    VARIANTARG params[2];
    VARIANT url;
    HRESULT hres;

    hres = IUnknown_QueryInterface(This->document, &IID_IDispatch, (void**)&disp);
    if(FAILED(hres))
        FIXME("Could not get IDispatch interface\n");

    dispparams.cArgs = 2;
    dispparams.cNamedArgs = 0;
    dispparams.rgdispidNamedArgs = NULL;
    dispparams.rgvarg = params;

    V_VT(params) = (VT_BYREF|VT_VARIANT);
    V_BYREF(params) = &url;

    V_VT(params+1) = VT_DISPATCH;
    V_DISPATCH(params+1) = disp;

    V_VT(&url) = VT_BSTR;
    V_BSTR(&url) = This->url;

    call_sink(This->cp_wbe2, DISPID_NAVIGATECOMPLETE2, &dispparams);
    call_sink(This->cp_wbe2, DISPID_DOCUMENTCOMPLETE, &dispparams);

    if(disp)
        IDispatch_Release(disp);
}

static LRESULT navigate2(WebBrowser *This)
{
    IHlinkTarget *hlink;
    HRESULT hres;

    TRACE("(%p)\n", This);

    if(!This->document) {
        WARN("document == NULL\n");
        return 0;
    }

    hres = IUnknown_QueryInterface(This->document, &IID_IHlinkTarget, (void**)&hlink);
    if(FAILED(hres)) {
        FIXME("Could not get IHlinkTarget interface\n");
        return 0;
    }

    hres = IHlinkTarget_Navigate(hlink, 0, NULL);
    IHlinkTarget_Release(hlink);
    if(FAILED(hres)) {
        FIXME("Navigate failed\n");
        return 0;
    }

    navigate_complete(This);

    return 0;
}

static LRESULT resize_document(WebBrowser *This, LONG width, LONG height)
{
    RECT rect = {0, 0, width, height};

    TRACE("(%p)->(%ld %ld)\n", This, width, height);

    if(This->view)
        IOleDocumentView_SetRect(This->view, &rect);

    return 0;
}

static LRESULT WINAPI doc_view_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
        return resize_document(This, LOWORD(lParam), HIWORD(lParam));
    case WB_WM_NAVIGATE2:
        return navigate2(This);
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

void create_doc_view_hwnd(WebBrowser *This)
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
            (HBRUSH)COLOR_WINDOWFRAME, NULL,
            wszShell_DocObject_View,
            NULL
        };

        wndclass.hInstance = shdocvw_hinstance;

        doc_view_atom = RegisterClassExW(&wndclass);
    }

    GetWindowRect(This->shell_embedding_hwnd, &rect);
    This->doc_view_hwnd = CreateWindowExW(0, wszShell_DocObject_View,
         wszShell_DocObject_View,
         WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TABSTOP | WS_MAXIMIZEBOX,
         rect.left, rect.top, rect.right, rect.bottom, This->shell_embedding_hwnd,
         NULL, shdocvw_hinstance, This);
}

#define DOCHOSTUI_THIS(iface) DEFINE_THIS(WebBrowser, DocHostUIHandler, iface)

static HRESULT WINAPI DocHostUIHandler_QueryInterface(IDocHostUIHandler2 *iface,
                                                      REFIID riid, void **ppv)
{
    WebBrowser *This = DOCHOSTUI_THIS(iface);
    return IOleClientSite_QueryInterface(CLIENTSITE(This), riid, ppv);
}

static ULONG WINAPI DocHostUIHandler_AddRef(IDocHostUIHandler2 *iface)
{
    WebBrowser *This = DOCHOSTUI_THIS(iface);
    return IOleClientSite_AddRef(CLIENTSITE(This));
}

static ULONG WINAPI DocHostUIHandler_Release(IDocHostUIHandler2 *iface)
{
    WebBrowser *This = DOCHOSTUI_THIS(iface);
    return IOleClientSite_Release(CLIENTSITE(This));
}

static HRESULT WINAPI DocHostUIHandler_ShowContextMenu(IDocHostUIHandler2 *iface,
         DWORD dwID, POINT *ppt, IUnknown *pcmdtReserved, IDispatch *pdispReserved)
{
    WebBrowser *This = DOCHOSTUI_THIS(iface);
    FIXME("(%p)->(%ld %p %p %p)\n", This, dwID, ppt, pcmdtReserved, pdispReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetHostInfo(IDocHostUIHandler2 *iface,
        DOCHOSTUIINFO *pInfo)
{
    WebBrowser *This = DOCHOSTUI_THIS(iface);
    IDocHostUIHandler *handler;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, pInfo);

    if(This->client) {
        hres = IOleClientSite_QueryInterface(This->client, &IID_IDocHostUIHandler, (void**)&handler);
        if(SUCCEEDED(hres)) {
            hres = IDocHostUIHandler_GetHostInfo(handler, pInfo);
            IDocHostUIHandler_Release(handler);
            if(SUCCEEDED(hres))
                return hres;
        }
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
    WebBrowser *This = DOCHOSTUI_THIS(iface);
    FIXME("(%p)->(%ld %p %p %p %p)\n", This, dwID, pActiveObject, pCommandTarget,
          pFrame, pDoc);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_HideUI(IDocHostUIHandler2 *iface)
{
    WebBrowser *This = DOCHOSTUI_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_UpdateUI(IDocHostUIHandler2 *iface)
{
    WebBrowser *This = DOCHOSTUI_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_EnableModeless(IDocHostUIHandler2 *iface,
                                                      BOOL fEnable)
{
    WebBrowser *This = DOCHOSTUI_THIS(iface);
    FIXME("(%p)->(%x)\n", This, fEnable);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_OnDocWindowActivate(IDocHostUIHandler2 *iface,
                                                           BOOL fActivate)
{
    WebBrowser *This = DOCHOSTUI_THIS(iface);
    FIXME("(%p)->(%x)\n", This, fActivate);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_OnFrameWindowActivate(IDocHostUIHandler2 *iface,
                                                             BOOL fActivate)
{
    WebBrowser *This = DOCHOSTUI_THIS(iface);
    FIXME("(%p)->(%x)\n", This, fActivate);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_ResizeBorder(IDocHostUIHandler2 *iface,
        LPCRECT prcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fRameWindow)
{
    WebBrowser *This = DOCHOSTUI_THIS(iface);
    FIXME("(%p)->(%p %p %X)\n", This, prcBorder, pUIWindow, fRameWindow);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_TranslateAccelerator(IDocHostUIHandler2 *iface,
        LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID)
{
    WebBrowser *This = DOCHOSTUI_THIS(iface);
    FIXME("(%p)->(%p %p %ld)\n", This, lpMsg, pguidCmdGroup, nCmdID);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetOptionKeyPath(IDocHostUIHandler2 *iface,
        LPOLESTR *pchKey, DWORD dw)
{
    WebBrowser *This = DOCHOSTUI_THIS(iface);
    IDocHostUIHandler *handler;
    HRESULT hres;

    TRACE("(%p)->(%p %ld)\n", This, pchKey, dw);

    if(!This->client)
        return S_OK;

    hres = IOleClientSite_QueryInterface(This->client, &IID_IDocHostUIHandler,
                                         (void**)&handler);
    if(SUCCEEDED(hres)) {
        hres = IDocHostUIHandler_GetOptionKeyPath(handler, pchKey, dw);
        IDocHostUIHandler_Release(handler);
        return hres;
    }

    return S_OK;
}

static HRESULT WINAPI DocHostUIHandler_GetDropTarget(IDocHostUIHandler2 *iface,
        IDropTarget *pDropTarget, IDropTarget **ppDropTarget)
{
    WebBrowser *This = DOCHOSTUI_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetExternal(IDocHostUIHandler2 *iface,
        IDispatch **ppDispatch)
{
    WebBrowser *This = DOCHOSTUI_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppDispatch);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_TranslateUrl(IDocHostUIHandler2 *iface,
        DWORD dwTranslate, OLECHAR *pchURLIn, OLECHAR **ppchURLOut)
{
    WebBrowser *This = DOCHOSTUI_THIS(iface);
    FIXME("(%p)->(%ld %s %p)\n", This, dwTranslate, debugstr_w(pchURLIn), ppchURLOut);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_FilterDataObject(IDocHostUIHandler2 *iface,
        IDataObject *pDO, IDataObject **ppDORet)
{
    WebBrowser *This = DOCHOSTUI_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pDO, ppDORet);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocHostUIHandler_GetOverrideKeyPath(IDocHostUIHandler2 *iface,
        LPOLESTR *pchKey, DWORD dw)
{
    WebBrowser *This = DOCHOSTUI_THIS(iface);
    IDocHostUIHandler2 *handler;
    HRESULT hres;

    TRACE("(%p)->(%p %ld)\n", This, pchKey, dw);

    if(!This->client)
        return S_OK;

    hres = IOleClientSite_QueryInterface(This->client, &IID_IDocHostUIHandler2,
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

void WebBrowser_DocHost_Init(WebBrowser *This)
{
    This->lpDocHostUIHandlerVtbl = &DocHostUIHandler2Vtbl;

    This->doc_view_hwnd = NULL;
}
