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

#include <stdio.h>

#include "wine/debug.h"
#include "shdocvw.h"
#include "mshtmdid.h"
#include "idispids.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

#define CLIENTSITE_THIS(iface) DEFINE_THIS(DocHost, OleClientSite, iface)

static HRESULT WINAPI ClientSite_QueryInterface(IOleClientSite *iface, REFIID riid, void **ppv)
{
    DocHost *This = CLIENTSITE_THIS(iface);

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
    }else if(IsEqualGUID(&IID_IOleCommandTarget, riid)) {
        TRACE("(%p)->(IID_IOleCommandTarget %p)\n", This, ppv);
        *ppv = OLECMD(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = CLDISP(This);
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        TRACE("(%p)->(IID_IServiceProvider %p)\n", This, ppv);
        *ppv = SERVPROV(This);
    }

    if(*ppv) {
        IOleClientSite_AddRef(CLIENTSITE(This));
        return S_OK;
    }

    WARN("Unsupported intrface %s\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI ClientSite_AddRef(IOleClientSite *iface)
{
    DocHost *This = CLIENTSITE_THIS(iface);
    return IDispatch_AddRef(This->disp);
}

static ULONG WINAPI ClientSite_Release(IOleClientSite *iface)
{
    DocHost *This = CLIENTSITE_THIS(iface);
    return IDispatch_Release(This->disp);
}

static HRESULT WINAPI ClientSite_SaveObject(IOleClientSite *iface)
{
    DocHost *This = CLIENTSITE_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_GetMoniker(IOleClientSite *iface, DWORD dwAssign,
                                            DWORD dwWhichMoniker, IMoniker **ppmk)
{
    DocHost *This = CLIENTSITE_THIS(iface);
    FIXME("(%p)->(%d %d %p)\n", This, dwAssign, dwWhichMoniker, ppmk);
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_GetContainer(IOleClientSite *iface, IOleContainer **ppContainer)
{
    DocHost *This = CLIENTSITE_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppContainer);
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_ShowObject(IOleClientSite *iface)
{
    DocHost *This = CLIENTSITE_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_OnShowWindow(IOleClientSite *iface, BOOL fShow)
{
    DocHost *This = CLIENTSITE_THIS(iface);
    FIXME("(%p)->(%x)\n", This, fShow);
    return E_NOTIMPL;
}

static HRESULT WINAPI ClientSite_RequestNewObjectLayout(IOleClientSite *iface)
{
    DocHost *This = CLIENTSITE_THIS(iface);
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

#define INPLACESITE_THIS(iface) DEFINE_THIS(DocHost, OleInPlaceSite, iface)

static HRESULT WINAPI InPlaceSite_QueryInterface(IOleInPlaceSite *iface, REFIID riid, void **ppv)
{
    DocHost *This = INPLACESITE_THIS(iface);
    return IOleClientSite_QueryInterface(CLIENTSITE(This), riid, ppv);
}

static ULONG WINAPI InPlaceSite_AddRef(IOleInPlaceSite *iface)
{
    DocHost *This = INPLACESITE_THIS(iface);
    return IOleClientSite_AddRef(CLIENTSITE(This));
}

static ULONG WINAPI InPlaceSite_Release(IOleInPlaceSite *iface)
{
    DocHost *This = INPLACESITE_THIS(iface);
    return IOleClientSite_Release(CLIENTSITE(This));
}

static HRESULT WINAPI InPlaceSite_GetWindow(IOleInPlaceSite *iface, HWND *phwnd)
{
    DocHost *This = INPLACESITE_THIS(iface);

    TRACE("(%p)->(%p)\n", This, phwnd);

    *phwnd = This->hwnd;
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_ContextSensitiveHelp(IOleInPlaceSite *iface, BOOL fEnterMode)
{
    DocHost *This = INPLACESITE_THIS(iface);
    FIXME("(%p)->(%x)\n", This, fEnterMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_CanInPlaceActivate(IOleInPlaceSite *iface)
{
    DocHost *This = INPLACESITE_THIS(iface);

    TRACE("(%p)\n", This);

    /* Nothing to do here */
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_OnInPlaceActivate(IOleInPlaceSite *iface)
{
    DocHost *This = INPLACESITE_THIS(iface);

    TRACE("(%p)\n", This);

    /* Nothing to do here */
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_OnUIActivate(IOleInPlaceSite *iface)
{
    DocHost *This = INPLACESITE_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_GetWindowContext(IOleInPlaceSite *iface,
        IOleInPlaceFrame **ppFrame, IOleInPlaceUIWindow **ppDoc, LPRECT lprcPosRect,
        LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    DocHost *This = INPLACESITE_THIS(iface);

    TRACE("(%p)->(%p %p %p %p %p)\n", This, ppFrame, ppDoc, lprcPosRect,
          lprcClipRect, lpFrameInfo);

    IOleInPlaceFrame_AddRef(INPLACEFRAME(This));
    *ppFrame = INPLACEFRAME(This);
    *ppDoc = NULL;

    GetClientRect(This->hwnd, lprcPosRect);
    *lprcClipRect = *lprcPosRect;

    lpFrameInfo->cb = sizeof(*lpFrameInfo);
    lpFrameInfo->fMDIApp = FALSE;
    lpFrameInfo->hwndFrame = This->frame_hwnd;
    lpFrameInfo->haccel = NULL;
    lpFrameInfo->cAccelEntries = 0; /* FIXME: should be 5 */

    return S_OK;
}

static HRESULT WINAPI InPlaceSite_Scroll(IOleInPlaceSite *iface, SIZE scrollExtent)
{
    DocHost *This = INPLACESITE_THIS(iface);
    FIXME("(%p)->({%d %d})\n", This, scrollExtent.cx, scrollExtent.cy);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_OnUIDeactivate(IOleInPlaceSite *iface, BOOL fUndoable)
{
    DocHost *This = INPLACESITE_THIS(iface);
    FIXME("(%p)->(%x)\n", This, fUndoable);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_OnInPlaceDeactivate(IOleInPlaceSite *iface)
{
    DocHost *This = INPLACESITE_THIS(iface);

    TRACE("(%p)\n", This);

    /* Nothing to do here */
    return S_OK;
}

static HRESULT WINAPI InPlaceSite_DiscardUndoState(IOleInPlaceSite *iface)
{
    DocHost *This = INPLACESITE_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_DeactivateAndUndo(IOleInPlaceSite *iface)
{
    DocHost *This = INPLACESITE_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceSite_OnPosRectChange(IOleInPlaceSite *iface,
                                                  LPCRECT lprcPosRect)
{
    DocHost *This = INPLACESITE_THIS(iface);
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

#define DOCSITE_THIS(iface) DEFINE_THIS(DocHost, OleDocumentSite, iface)

static HRESULT WINAPI OleDocumentSite_QueryInterface(IOleDocumentSite *iface,
                                                     REFIID riid, void **ppv)
{
    DocHost *This = DOCSITE_THIS(iface);
    return IOleClientSite_QueryInterface(CLIENTSITE(This), riid, ppv);
}

static ULONG WINAPI OleDocumentSite_AddRef(IOleDocumentSite *iface)
{
    DocHost *This = DOCSITE_THIS(iface);
    return IOleClientSite_AddRef(CLIENTSITE(This));
}

static ULONG WINAPI OleDocumentSite_Release(IOleDocumentSite *iface)
{
    DocHost *This = DOCSITE_THIS(iface);
    return IOleClientSite_Release(CLIENTSITE(This));
}

static HRESULT WINAPI OleDocumentSite_ActivateMe(IOleDocumentSite *iface,
                                                 IOleDocumentView *pViewToActivate)
{
    DocHost *This = DOCSITE_THIS(iface);
    IOleDocument *oledoc;
    RECT rect;
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, pViewToActivate);

    hres = IUnknown_QueryInterface(This->document, &IID_IOleDocument, (void**)&oledoc);
    if(FAILED(hres))
        return hres;

    IOleDocument_CreateView(oledoc, INPLACESITE(This), NULL, 0, &This->view);
    IOleDocument_Release(oledoc);

    GetClientRect(This->hwnd, &rect);
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

#define DISP_THIS(iface) DEFINE_THIS(DocHost, Dispatch, iface)

static HRESULT WINAPI ClDispatch_QueryInterface(IDispatch *iface, REFIID riid, void **ppv)
{
    DocHost *This = DISP_THIS(iface);
    return IOleClientSite_QueryInterface(CLIENTSITE(This), riid, ppv);
}

static ULONG WINAPI ClDispatch_AddRef(IDispatch *iface)
{
    DocHost *This = DISP_THIS(iface);
    return IOleClientSite_AddRef(CLIENTSITE(This));
}

static ULONG WINAPI ClDispatch_Release(IDispatch *iface)
{
    DocHost *This = DISP_THIS(iface);
    return IOleClientSite_Release(CLIENTSITE(This));
}

static HRESULT WINAPI ClDispatch_GetTypeInfoCount(IDispatch *iface, UINT *pctinfo)
{
    DocHost *This = DISP_THIS(iface);

    TRACE("(%p)->(%p)\n", This, pctinfo);

    return E_NOTIMPL;
}

static HRESULT WINAPI ClDispatch_GetTypeInfo(IDispatch *iface, UINT iTInfo, LCID lcid,
                                             ITypeInfo **ppTInfo)
{
    DocHost *This = DISP_THIS(iface);

    TRACE("(%p)->(%u %d %p)\n", This, iTInfo, lcid, ppTInfo);

    return E_NOTIMPL;
}

static HRESULT WINAPI ClDispatch_GetIDsOfNames(IDispatch *iface, REFIID riid, LPOLESTR *rgszNames,
                                               UINT cNames, LCID lcid, DISPID *rgDispId)
{
    DocHost *This = DISP_THIS(iface);

    TRACE("(%p)->(%s %p %u %d %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    return E_NOTIMPL;
}

static const char *debugstr_dispid(DISPID dispid)
{
    static char buf[16];

#define CASE_DISPID(did) case did: return #did
    switch(dispid) {
        CASE_DISPID(DISPID_AMBIENT_USERMODE);
        CASE_DISPID(DISPID_AMBIENT_DLCONTROL);
        CASE_DISPID(DISPID_AMBIENT_USERAGENT);
        CASE_DISPID(DISPID_AMBIENT_PALETTE);
        CASE_DISPID(DISPID_AMBIENT_OFFLINEIFNOTCONNECTED);
        CASE_DISPID(DISPID_AMBIENT_SILENT);
    }
#undef CASE_DISPID

    sprintf(buf, "%d", dispid);
    return buf;
}

static HRESULT WINAPI ClDispatch_Invoke(IDispatch *iface, DISPID dispIdMember, REFIID riid,
                                        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                                        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    DocHost *This = DISP_THIS(iface);

    TRACE("(%p)->(%s %s %d %04x %p %p %p %p)\n", This, debugstr_dispid(dispIdMember),
          debugstr_guid(riid), lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    switch(dispIdMember) {
    case DISPID_AMBIENT_USERMODE:
    case DISPID_AMBIENT_DLCONTROL:
    case DISPID_AMBIENT_USERAGENT:
    case DISPID_AMBIENT_PALETTE:
        if(!This->client_disp)
            return E_FAIL;
        return IDispatch_Invoke(This->client_disp, dispIdMember, riid, lcid, wFlags,
                                pDispParams, pVarResult, pExcepInfo, puArgErr);
    case DISPID_AMBIENT_OFFLINEIFNOTCONNECTED:
        V_VT(pVarResult) = VT_BOOL;
        V_BOOL(pVarResult) = This->offline;
        return S_OK;
    case DISPID_AMBIENT_SILENT:
        V_VT(pVarResult) = VT_BOOL;
        V_BOOL(pVarResult) = This->offline;
        return S_OK;
    }

    FIXME("unhandled dispid %d\n", dispIdMember);
    return E_NOTIMPL;
}

#undef DISP_THIS

static const IDispatchVtbl DispatchVtbl = {
    ClDispatch_QueryInterface,
    ClDispatch_AddRef,
    ClDispatch_Release,
    ClDispatch_GetTypeInfoCount,
    ClDispatch_GetTypeInfo,
    ClDispatch_GetIDsOfNames,
    ClDispatch_Invoke
};

#define SERVPROV_THIS(iface) DEFINE_THIS(DocHost, ServiceProvider, iface)

static HRESULT WINAPI ClServiceProvider_QueryInterface(IServiceProvider *iface, REFIID riid,
                                                       void **ppv)
{
    DocHost *This = SERVPROV_THIS(iface);
    return IOleClientSite_QueryInterface(CLIENTSITE(This), riid, ppv);
}

static ULONG WINAPI ClServiceProvider_AddRef(IServiceProvider *iface)
{
    DocHost *This = SERVPROV_THIS(iface);
    return IOleClientSite_AddRef(CLIENTSITE(This));
}

static ULONG WINAPI ClServiceProvider_Release(IServiceProvider *iface)
{
    DocHost *This = SERVPROV_THIS(iface);
    return IOleClientSite_Release(CLIENTSITE(This));
}

static HRESULT WINAPI ClServiceProvider_QueryService(IServiceProvider *iface, REFGUID guidService,
                                                     REFIID riid, void **ppv)
{
    DocHost *This = SERVPROV_THIS(iface);

    if(IsEqualGUID(&IID_IHlinkFrame, guidService)) {
        TRACE("(%p)->(IID_IHlinkFrame %s %p)\n", This, debugstr_guid(riid), ppv);
        return IDispatch_QueryInterface(This->disp, riid, ppv);
    }

    FIXME("(%p)->(%s %s %p)\n", This, debugstr_guid(guidService), debugstr_guid(riid), ppv);

    return E_NOINTERFACE;
}

#undef SERVPROV_THIS

static const IServiceProviderVtbl ServiceProviderVtbl = {
    ClServiceProvider_QueryInterface,
    ClServiceProvider_AddRef,
    ClServiceProvider_Release,
    ClServiceProvider_QueryService
};

void DocHost_ClientSite_Init(DocHost *This)
{
    This->lpOleClientSiteVtbl   = &OleClientSiteVtbl;
    This->lpOleInPlaceSiteVtbl  = &OleInPlaceSiteVtbl;
    This->lpOleDocumentSiteVtbl = &OleDocumentSiteVtbl;
    This->lpDispatchVtbl        = &DispatchVtbl;
    This->lpServiceProviderVtbl = &ServiceProviderVtbl;

    This->view = NULL;
}

void DocHost_ClientSite_Release(DocHost *This)
{
    if(This->view)
        IOleDocumentView_Release(This->view);
}
