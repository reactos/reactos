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

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

#define CLIENTSITE_THIS(iface) DEFINE_THIS(WebBrowser, OleClientSite, iface)

static HRESULT WINAPI ClientSite_QueryInterface(IOleClientSite *iface, REFIID riid, void **ppv)
{
    WebBrowser *This = CLIENTSITE_THIS(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = CLIENTSITE(This);
    }else if(IsEqualGUID(&IID_IOleClientSite, riid)) {
        TRACE("(%p)->(IID_IOleClientSite %p)\n", This, ppv);
        *ppv = CLIENTSITE(This);
    }else if(IsEqualGUID(&IID_IOleWindow, riid)) {
        TRACE("(%p)->(IID_IOleWindow %p)\n", This, ppv);
        *ppv = INPLACESITE(This);
    }else if(IsEqualGUID(&IID_IOleInPlaceSite, riid)) {
        TRACE("(%p)->(IID_IOleInPlaceSite %p)\n", This, ppv);
        *ppv = INPLACESITE(This);
    }else if(IsEqualGUID(&IID_IDocHostUIHandler, riid)) {
        TRACE("(%p)->(IID_IDocHostUIHandler %p)\n", This, ppv);
        *ppv = DOCHOSTUI(This);
    }else if(IsEqualGUID(&IID_IDocHostUIHandler2, riid)) {
        TRACE("(%p)->(IID_IDocHostUIHandler2 %p)\n", This, ppv);
        *ppv = DOCHOSTUI2(This);
    }else if(IsEqualGUID(&IID_IOleDocumentSite, riid)) {
        TRACE("(%p)->(IID_IOleDocumentSite %p)\n", This, ppv);
        *ppv = DOCSITE(This);
    }

    if(*ppv) {
        IWebBrowser2_AddRef(WEBBROWSER(This));
        return S_OK;
    }

    WARN("Unsupported intrface %s\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI ClientSite_AddRef(IOleClientSite *iface)
{
    WebBrowser *This = CLIENTSITE_THIS(iface);
    return IWebBrowser2_AddRef(WEBBROWSER(This));
}

static ULONG WINAPI ClientSite_Release(IOleClientSite *iface)
{
    WebBrowser *This = CLIENTSITE_THIS(iface);
    return IWebBrowser2_Release(WEBBROWSER(This));
}

static HRESULT WINAPI ClientSite_SaveObject(IOleClientSite *iface)
{
    WebBrowser *This = CLIENTSITE_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_GetMoniker(IOleClientSite *iface, DWORD dwAssign,
                                            DWORD dwWhichMoniker, IMoniker **ppmk)
{
    WebBrowser *This = CLIENTSITE_THIS(iface);
    FIXME("(%p)->(%ld %ld %p)\n", This, dwAssign, dwWhichMoniker, ppmk);
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_GetContainer(IOleClientSite *iface, IOleContainer **ppContainer)
{
    WebBrowser *This = CLIENTSITE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppContainer);
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_ShowObject(IOleClientSite *iface)
{
    WebBrowser *This = CLIENTSITE_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_OnShowWindow(IOleClientSite *iface, BOOL fShow)
{
    WebBrowser *This = CLIENTSITE_THIS(iface);
    FIXME("(%p)->(%x)\n", This, fShow);
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_RequestNewObjectLayout(IOleClientSite *iface)
{
    WebBrowser *This = CLIENTSITE_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

#undef CLIENTSITE_THIS

static const IOleClientSiteVtbl OleClientSiteVtbl = {
    ClientSite_QueryInterface,
    ClientSite_AddRef,
    ClientSite_Release,
    ClientSite_SaveObject,
    ClientSite_GetMoniker,
    ClientSite_GetContainer,
    ClientSite_ShowObject,
    ClientSite_OnShowWindow,
    ClientSite_RequestNewObjectLayout
};

#define INPLACESITE_THIS(iface) DEFINE_THIS(WebBrowser, OleInPlaceSite, iface)

static HRESULT WINAPI InPlaceSite_QueryInterface(IOleInPlaceSite *iface, REFIID riid, void **ppv)
{
    WebBrowser *This = INPLACESITE_THIS(iface);
    return IOleClientSite_QueryInterface(CLIENTSITE(This), riid, ppv);
}

static ULONG WINAPI InPlaceSite_AddRef(IOleInPlaceSite *iface)
{
    WebBrowser *This = INPLACESITE_THIS(iface);
    return IOleClientSite_AddRef(CLIENTSITE(This));
}

static ULONG WINAPI InPlaceSite_Release(IOleInPlaceSite *iface)
{
    WebBrowser *This = INPLACESITE_THIS(iface);
    return IOleClientSite_Release(CLIENTSITE(This));
}

static HRESULT WINAPI InPlaceSite_GetWindow(IOleInPlaceSite *iface, HWND *phwnd)
{
    WebBrowser *This = INPLACESITE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, phwnd);

    *phwnd = This->doc_view_hwnd;
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_ContextSensitiveHelp(IOleInPlaceSite *iface, BOOL fEnterMode)
{
    WebBrowser *This = INPLACESITE_THIS(iface);
    FIXME("(%p)->(%x)\n", This, fEnterMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_CanInPlaceActivate(IOleInPlaceSite *iface)
{
    WebBrowser *This = INPLACESITE_THIS(iface);

    TRACE("(%p)\n", This);

    /* Nothing to do here */
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_OnInPlaceActivate(IOleInPlaceSite *iface)
{
    WebBrowser *This = INPLACESITE_THIS(iface);

    TRACE("(%p)\n", This);

    /* Nothing to do here */
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_OnUIActivate(IOleInPlaceSite *iface)
{
    WebBrowser *This = INPLACESITE_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_GetWindowContext(IOleInPlaceSite *iface,
        IOleInPlaceFrame **ppFrame, IOleInPlaceUIWindow **ppDoc, LPRECT lprcPosRect,
        LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    WebBrowser *This = INPLACESITE_THIS(iface);

    TRACE("(%p)->(%p %p %p %p %p)\n", This, ppFrame, ppDoc, lprcPosRect,
          lprcClipRect, lpFrameInfo);

    *ppFrame = INPLACEFRAME(This);
    *ppDoc = NULL;

    GetClientRect(This->doc_view_hwnd, lprcPosRect);
    memcpy(lprcClipRect, lprcPosRect, sizeof(RECT));

    lpFrameInfo->cb = sizeof(*lpFrameInfo);
    lpFrameInfo->fMDIApp = FALSE;
    lpFrameInfo->hwndFrame = This->shell_embedding_hwnd;
    lpFrameInfo->haccel = NULL;
    lpFrameInfo->cAccelEntries = 0; /* FIXME: should be 5 */

    return S_OK;
}

static HRESULT WINAPI InPlaceSite_Scroll(IOleInPlaceSite *iface, SIZE scrollExtent)
{
    WebBrowser *This = INPLACESITE_THIS(iface);
    FIXME("(%p)->({%ld %ld})\n", This, scrollExtent.cx, scrollExtent.cy);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_OnUIDeactivate(IOleInPlaceSite *iface, BOOL fUndoable)
{
    WebBrowser *This = INPLACESITE_THIS(iface);
    FIXME("(%p)->(%x)\n", This, fUndoable);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_OnInPlaceDeactivate(IOleInPlaceSite *iface)
{
    WebBrowser *This = INPLACESITE_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_DiscardUndoState(IOleInPlaceSite *iface)
{
    WebBrowser *This = INPLACESITE_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_DeactivateAndUndo(IOleInPlaceSite *iface)
{
    WebBrowser *This = INPLACESITE_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_OnPosRectChange(IOleInPlaceSite *iface,
                                                  LPCRECT lprcPosRect)
{
    WebBrowser *This = INPLACESITE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, lprcPosRect);
    return E_NOTIMPL;
}

#undef INPLACESITE_THIS

static const IOleInPlaceSiteVtbl OleInPlaceSiteVtbl = {
    InPlaceSite_QueryInterface,
    InPlaceSite_AddRef,
    InPlaceSite_Release,
    InPlaceSite_GetWindow,
    InPlaceSite_ContextSensitiveHelp,
    InPlaceSite_CanInPlaceActivate,
    InPlaceSite_OnInPlaceActivate,
    InPlaceSite_OnUIActivate,
    InPlaceSite_GetWindowContext,
    InPlaceSite_Scroll,
    InPlaceSite_OnUIDeactivate,
    InPlaceSite_OnInPlaceDeactivate,
    InPlaceSite_DiscardUndoState,
    InPlaceSite_DeactivateAndUndo,
    InPlaceSite_OnPosRectChange
};

#define DOCSITE_THIS(iface) DEFINE_THIS(WebBrowser, OleDocumentSite, iface)

static HRESULT WINAPI OleDocumentSite_QueryInterface(IOleDocumentSite *iface,
                                                     REFIID riid, void **ppv)
{
    WebBrowser *This = DOCSITE_THIS(iface);
    return IOleClientSite_QueryInterface(CLIENTSITE(This), riid, ppv);
}

static ULONG WINAPI OleDocumentSite_AddRef(IOleDocumentSite *iface)
{
    WebBrowser *This = DOCSITE_THIS(iface);
    return IOleClientSite_AddRef(CLIENTSITE(This));
}

static ULONG WINAPI OleDocumentSite_Release(IOleDocumentSite *iface)
{
    WebBrowser *This = DOCSITE_THIS(iface);
    return IOleClientSite_Release(CLIENTSITE(This));
}

static HRESULT WINAPI OleDocumentSite_ActivateMe(IOleDocumentSite *iface,
                                                 IOleDocumentView *pViewToActivate)
{
    WebBrowser *This = DOCSITE_THIS(iface);
    IOleDocument *oledoc;
    RECT rect;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, pViewToActivate);

    hres = IUnknown_QueryInterface(This->document, &IID_IOleDocument, (void**)&oledoc);
    if(FAILED(hres))
        return hres;

    IOleDocument_CreateView(oledoc, INPLACESITE(This), NULL, 0, &This->view);
    IOleDocument_Release(oledoc);

    GetClientRect(This->doc_view_hwnd, &rect);
    IOleDocumentView_SetRect(This->view, &rect);

    hres = IOleDocumentView_Show(This->view, TRUE);

    return hres;
}

#undef DOCSITE_THIS

static const IOleDocumentSiteVtbl OleDocumentSiteVtbl = {
    OleDocumentSite_QueryInterface,
    OleDocumentSite_AddRef,
    OleDocumentSite_Release,
    OleDocumentSite_ActivateMe
};

void WebBrowser_ClientSite_Init(WebBrowser *This)
{
    This->lpOleClientSiteVtbl   = &OleClientSiteVtbl;
    This->lpOleInPlaceSiteVtbl  = &OleInPlaceSiteVtbl;
    This->lpOleDocumentSiteVtbl = &OleDocumentSiteVtbl;

    This->view = NULL;
}

void WebBrowser_ClientSite_Destroy(WebBrowser *This)
{
    if(This->view)
        IOleDocumentView_Release(This->view);
}
