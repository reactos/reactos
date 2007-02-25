/*
 * Implementation of IWebBrowser interface for WebBrowser control
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "wine/debug.h"
#include "shdocvw.h"
#include "mshtml.h"


WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

/**********************************************************************
 * Implement the IWebBrowser interface
 */

#define WEBBROWSER_THIS(iface) DEFINE_THIS(WebBrowser, WebBrowser2, iface)

static HRESULT WINAPI WebBrowser_QueryInterface(IWebBrowser2 *iface, REFIID riid, LPVOID *ppv)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);

    if (ppv == NULL)
        return E_POINTER;
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = WEBBROWSER(This);
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = WEBBROWSER(This);
    }else if(IsEqualGUID(&IID_IWebBrowser, riid)) {
        TRACE("(%p)->(IID_IWebBrowser %p)\n", This, ppv);
        *ppv = WEBBROWSER(This);
    }else if(IsEqualGUID(&IID_IWebBrowserApp, riid)) {
        TRACE("(%p)->(IID_IWebBrowserApp %p)\n", This, ppv);
        *ppv = WEBBROWSER(This);
    }else if(IsEqualGUID(&IID_IWebBrowser2, riid)) {
        TRACE("(%p)->(IID_IWebBrowser2 %p)\n", This, ppv);
        *ppv = WEBBROWSER(This);
    }else if(IsEqualGUID(&IID_IOleObject, riid)) {
        TRACE("(%p)->(IID_IOleObject %p)\n", This, ppv);
        *ppv = OLEOBJ(This);
    }else if(IsEqualGUID(&IID_IOleWindow, riid)) {
        TRACE("(%p)->(IID_IOleWindow %p)\n", This, ppv);
        *ppv = INPLACEOBJ(This);
    }else if(IsEqualGUID (&IID_IOleInPlaceObject, riid)) {
        TRACE("(%p)->(IID_IOleInPlaceObject %p)\n", This, ppv);
        *ppv = INPLACEOBJ(This);
    }else if(IsEqualGUID(&IID_IOleControl, riid)) {
        TRACE("(%p)->(IID_IOleControl %p)\n", This, ppv);
        *ppv = CONTROL(This);
    }else if(IsEqualGUID(&IID_IPersist, riid)) {
        TRACE("(%p)->(IID_IPersist %p)\n", This, ppv);
        *ppv = PERSTORAGE(This);
    }else if(IsEqualGUID(&IID_IPersistStorage, riid)) {
        TRACE("(%p)->(IID_IPersistStorage %p)\n", This, ppv);
        *ppv = PERSTORAGE(This);
    }else if(IsEqualGUID (&IID_IPersistStreamInit, riid)) {
        TRACE("(%p)->(IID_IPersistStreamInit %p)\n", This, ppv);
        *ppv = PERSTRINIT(This);
    }else if(IsEqualGUID(&IID_IProvideClassInfo, riid)) {
        TRACE("(%p)->(IID_IProvideClassInfo %p)\n", This, ppv);
        *ppv = CLASSINFO(This);
    }else if(IsEqualGUID(&IID_IProvideClassInfo2, riid)) {
        TRACE("(%p)->(IID_IProvideClassInfo2 %p)\n", This, ppv);
        *ppv = CLASSINFO(This);
    }else if(IsEqualGUID(&IID_IQuickActivate, riid)) {
        TRACE("(%p)->(IID_IQuickActivate %p)\n", This, ppv);
        *ppv = QUICKACT(This);
    }else if(IsEqualGUID(&IID_IConnectionPointContainer, riid)) {
        TRACE("(%p)->(IID_IConnectionPointContainer %p)\n", This, ppv);
        *ppv = CONPTCONT(This);
    }else if(IsEqualGUID(&IID_IViewObject, riid)) {
        TRACE("(%p)->(IID_IViewObject %p)\n", This, ppv);
        *ppv = VIEWOBJ(This);
    }else if(IsEqualGUID(&IID_IViewObject2, riid)) {
        TRACE("(%p)->(IID_IViewObject2 %p)\n", This, ppv);
        *ppv = VIEWOBJ2(This);
    }else if(IsEqualGUID(&IID_IOleInPlaceActiveObject, riid)) {
        TRACE("(%p)->(IID_IOleInPlaceActiveObject %p)\n", This, ppv);
        *ppv = ACTIVEOBJ(This);
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    FIXME("(%p)->(%s %p) interface not supported\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI WebBrowser_AddRef(IWebBrowser2 *iface)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%ld\n", This, ref);
    return ref;
}

static ULONG WINAPI WebBrowser_Release(IWebBrowser2 *iface)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        if(This->document)
            IUnknown_Release(This->document);

        WebBrowser_OleObject_Destroy(This);
        WebBrowser_Events_Destroy(This);
        WebBrowser_ClientSite_Destroy(This);

        SysFreeString(This->url);
        HeapFree(GetProcessHeap(), 0, This);
        SHDOCVW_UnlockModule();
    }

    return ref;
}

/* IDispatch methods */
static HRESULT WINAPI WebBrowser_GetTypeInfoCount(IWebBrowser2 *iface, UINT *pctinfo)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_GetTypeInfo(IWebBrowser2 *iface, UINT iTInfo, LCID lcid,
                                     LPTYPEINFO *ppTInfo)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%d %ld %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_GetIDsOfNames(IWebBrowser2 *iface, REFIID riid,
                                       LPOLESTR *rgszNames, UINT cNames,
                                       LCID lcid, DISPID *rgDispId)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%s %p %d %ld %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
            lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_Invoke(IWebBrowser2 *iface, DISPID dispIdMember,
                                REFIID riid, LCID lcid, WORD wFlags,
                                DISPPARAMS *pDispParams, VARIANT *pVarResult,
                                EXCEPINFO *pExepInfo, UINT *puArgErr)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%ld %s %ld %08x %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);
    return E_NOTIMPL;
}

/* IWebBrowser methods */
static HRESULT WINAPI WebBrowser_GoBack(IWebBrowser2 *iface)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_GoForward(IWebBrowser2 *iface)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_GoHome(IWebBrowser2 *iface)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_GoSearch(IWebBrowser2 *iface)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_Navigate(IWebBrowser2 *iface, BSTR URL,
                                  VARIANT *Flags, VARIANT *TargetFrameName,
                                  VARIANT *PostData, VARIANT *Headers)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%s %p %p %p %p)\n", This, debugstr_w(URL), Flags, TargetFrameName,
          PostData, Headers);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_Refresh(IWebBrowser2 *iface)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_Refresh2(IWebBrowser2 *iface, VARIANT *Level)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, Level);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_Stop(IWebBrowser2 *iface)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Application(IWebBrowser2 *iface, IDispatch **ppDisp)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppDisp);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Parent(IWebBrowser2 *iface, IDispatch **ppDisp)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppDisp);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Container(IWebBrowser2 *iface, IDispatch **ppDisp)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppDisp);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Document(IWebBrowser2 *iface, IDispatch **ppDisp)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);

    TRACE("(%p)->(%p)\n", This, ppDisp);

    *ppDisp = NULL;
    if(This->document)
        IUnknown_QueryInterface(This->document, &IID_IDispatch, (void**)ppDisp);

    return S_OK;
}

static HRESULT WINAPI WebBrowser_get_TopLevelContainer(IWebBrowser2 *iface, VARIANT_BOOL *pBool)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pBool);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Type(IWebBrowser2 *iface, BSTR *Type)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, Type);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Left(IWebBrowser2 *iface, long *pl)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pl);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_Left(IWebBrowser2 *iface, long Left)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, Left);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Top(IWebBrowser2 *iface, long *pl)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pl);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_Top(IWebBrowser2 *iface, long Top)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, Top);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Width(IWebBrowser2 *iface, long *pl)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pl);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_Width(IWebBrowser2 *iface, long Width)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, Width);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Height(IWebBrowser2 *iface, long *pl)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pl);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_Height(IWebBrowser2 *iface, long Height)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%ld)\n", This, Height);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_LocationName(IWebBrowser2 *iface, BSTR *LocationName)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, LocationName);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_LocationURL(IWebBrowser2 *iface, BSTR *LocationURL)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, LocationURL);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Busy(IWebBrowser2 *iface, VARIANT_BOOL *pBool)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pBool);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_Quit(IWebBrowser2 *iface)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_ClientToWindow(IWebBrowser2 *iface, int *pcx, int *pcy)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pcx, pcy);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_PutProperty(IWebBrowser2 *iface, BSTR szProperty, VARIANT vtValue)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(szProperty));
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_GetProperty(IWebBrowser2 *iface, BSTR szProperty, VARIANT *pvtValue)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(szProperty), pvtValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Name(IWebBrowser2 *iface, BSTR *Name)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, Name);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_HWND(IWebBrowser2 *iface, long *pHWND)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pHWND);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_FullName(IWebBrowser2 *iface, BSTR *FullName)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, FullName);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Path(IWebBrowser2 *iface, BSTR *Path)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, Path);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Visible(IWebBrowser2 *iface, VARIANT_BOOL *pBool)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pBool);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_Visible(IWebBrowser2 *iface, VARIANT_BOOL Value)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%x)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_StatusBar(IWebBrowser2 *iface, VARIANT_BOOL *pBool)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pBool);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_StatusBar(IWebBrowser2 *iface, VARIANT_BOOL Value)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%x)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_StatusText(IWebBrowser2 *iface, BSTR *StatusText)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, StatusText);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_StatusText(IWebBrowser2 *iface, BSTR StatusText)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(StatusText));
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_ToolBar(IWebBrowser2 *iface, int *Value)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_ToolBar(IWebBrowser2 *iface, int Value)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%d)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_MenuBar(IWebBrowser2 *iface, VARIANT_BOOL *Value)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_MenuBar(IWebBrowser2 *iface, VARIANT_BOOL Value)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%x)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_FullScreen(IWebBrowser2 *iface, VARIANT_BOOL *pbFullScreen)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pbFullScreen);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_FullScreen(IWebBrowser2 *iface, VARIANT_BOOL bFullScreen)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%x)\n", This, bFullScreen);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_Navigate2(IWebBrowser2 *iface, VARIANT *URL, VARIANT *Flags,
        VARIANT *TargetFrameName, VARIANT *PostData, VARIANT *Headers)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    IPersistMoniker *persist;
    IOleObject *oleobj;
    IMoniker *mon;
    HRESULT hres;

    TRACE("(%p)->(%p %p %p %p %p)\n", This, URL, Flags, TargetFrameName, PostData, Headers);

    if(!This->client)
        return E_FAIL;

    if((Flags && V_VT(Flags) != VT_EMPTY) 
       || (TargetFrameName && V_VT(TargetFrameName) != VT_EMPTY)
       || (PostData && V_VT(PostData) != VT_EMPTY) 
       || (Headers && V_VT(Headers) != VT_EMPTY))
        FIXME("Unsupported arguments\n");

    if(!URL)
        return S_OK;
    if(V_VT(URL) != VT_BSTR)
        return E_INVALIDARG;

    if(!This->doc_view_hwnd)
        create_doc_view_hwnd(This);

    /*
     * FIXME:
     * We should use URLMoniker's BindToObject instead creating HTMLDocument here.
     * This should be fixed when mshtml.dll and urlmon.dll will be good enough.
     */

    if(!This->document) {
        hres = CoCreateInstance(&CLSID_HTMLDocument, NULL,
                                CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
                                &IID_IUnknown, (void**)&This->document);
        if(FAILED(hres))
            return hres;
    }

    hres = IUnknown_QueryInterface(This->document, &IID_IPersistMoniker, (void**)&persist);
    if(FAILED(hres))
        return hres;

    hres = CreateURLMoniker(NULL, V_BSTR(URL), &mon);
    if(FAILED(hres)) {
        IPersistMoniker_Release(persist);
        return hres;
    }

    hres = IPersistMoniker_Load(persist, FALSE, mon, NULL /* FIXME */, 0);
    IMoniker_Release(mon);
    IPersistMoniker_Release(persist);
    if(FAILED(hres)) {
        WARN("Load failed: %08lx\n", hres);
        return hres;
    }

    This->url = SysAllocString(V_BSTR(URL));

    hres = IUnknown_QueryInterface(This->document, &IID_IOleObject, (void**)&oleobj);
    if(FAILED(hres))
        return hres;

    hres = IOleObject_SetClientSite(oleobj, CLIENTSITE(This));
    IOleObject_Release(oleobj);

    PostMessageW(This->doc_view_hwnd, WB_WM_NAVIGATE2, 0, 0);

    return hres;
}

static HRESULT WINAPI WebBrowser_QueryStatusWB(IWebBrowser2 *iface, OLECMDID cmdID, OLECMDF *pcmdf)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%d %p)\n", This, cmdID, pcmdf);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_ExecWB(IWebBrowser2 *iface, OLECMDID cmdID,
        OLECMDEXECOPT cmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%d %d %p %p)\n", This, cmdID, cmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_ShowBrowserBar(IWebBrowser2 *iface, VARIANT *pvaClsid,
        VARIANT *pvarShow, VARIANT *pvarSize)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p %p %p)\n", This, pvaClsid, pvarShow, pvarSize);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_ReadyState(IWebBrowser2 *iface, READYSTATE *lpReadyState)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, lpReadyState);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Offline(IWebBrowser2 *iface, VARIANT_BOOL *pbOffline)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pbOffline);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_Offline(IWebBrowser2 *iface, VARIANT_BOOL bOffline)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%x)\n", This, bOffline);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Silent(IWebBrowser2 *iface, VARIANT_BOOL *pbSilent)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pbSilent);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_Silent(IWebBrowser2 *iface, VARIANT_BOOL bSilent)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%x)\n", This, bSilent);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_RegisterAsBrowser(IWebBrowser2 *iface,
        VARIANT_BOOL *pbRegister)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pbRegister);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_RegisterAsBrowser(IWebBrowser2 *iface,
        VARIANT_BOOL bRegister)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%x)\n", This, bRegister);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_RegisterAsDropTarget(IWebBrowser2 *iface,
        VARIANT_BOOL *pbRegister)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pbRegister);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_RegisterAsDropTarget(IWebBrowser2 *iface,
        VARIANT_BOOL bRegister)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%x)\n", This, bRegister);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_TheaterMode(IWebBrowser2 *iface, VARIANT_BOOL *pbRegister)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pbRegister);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_TheaterMode(IWebBrowser2 *iface, VARIANT_BOOL bRegister)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%x)\n", This, bRegister);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_AddressBar(IWebBrowser2 *iface, VARIANT_BOOL *Value)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_AddressBar(IWebBrowser2 *iface, VARIANT_BOOL Value)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%x)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_get_Resizable(IWebBrowser2 *iface, VARIANT_BOOL *Value)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI WebBrowser_put_Resizable(IWebBrowser2 *iface, VARIANT_BOOL Value)
{
    WebBrowser *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%x)\n", This, Value);
    return E_NOTIMPL;
}

#undef WEBBROWSER_THIS

static const IWebBrowser2Vtbl WebBrowser2Vtbl =
{
    WebBrowser_QueryInterface,
    WebBrowser_AddRef,
    WebBrowser_Release,
    WebBrowser_GetTypeInfoCount,
    WebBrowser_GetTypeInfo,
    WebBrowser_GetIDsOfNames,
    WebBrowser_Invoke,
    WebBrowser_GoBack,
    WebBrowser_GoForward,
    WebBrowser_GoHome,
    WebBrowser_GoSearch,
    WebBrowser_Navigate,
    WebBrowser_Refresh,
    WebBrowser_Refresh2,
    WebBrowser_Stop,
    WebBrowser_get_Application,
    WebBrowser_get_Parent,
    WebBrowser_get_Container,
    WebBrowser_get_Document,
    WebBrowser_get_TopLevelContainer,
    WebBrowser_get_Type,
    WebBrowser_get_Left,
    WebBrowser_put_Left,
    WebBrowser_get_Top,
    WebBrowser_put_Top,
    WebBrowser_get_Width,
    WebBrowser_put_Width,
    WebBrowser_get_Height,
    WebBrowser_put_Height,
    WebBrowser_get_LocationName,
    WebBrowser_get_LocationURL,
    WebBrowser_get_Busy,
    WebBrowser_Quit,
    WebBrowser_ClientToWindow,
    WebBrowser_PutProperty,
    WebBrowser_GetProperty,
    WebBrowser_get_Name,
    WebBrowser_get_HWND,
    WebBrowser_get_FullName,
    WebBrowser_get_Path,
    WebBrowser_get_Visible,
    WebBrowser_put_Visible,
    WebBrowser_get_StatusBar,
    WebBrowser_put_StatusBar,
    WebBrowser_get_StatusText,
    WebBrowser_put_StatusText,
    WebBrowser_get_ToolBar,
    WebBrowser_put_ToolBar,
    WebBrowser_get_MenuBar,
    WebBrowser_put_MenuBar,
    WebBrowser_get_FullScreen,
    WebBrowser_put_FullScreen,
    WebBrowser_Navigate2,
    WebBrowser_QueryStatusWB,
    WebBrowser_ExecWB,
    WebBrowser_ShowBrowserBar,
    WebBrowser_get_ReadyState,
    WebBrowser_get_Offline,
    WebBrowser_put_Offline,
    WebBrowser_get_Silent,
    WebBrowser_put_Silent,
    WebBrowser_get_RegisterAsBrowser,
    WebBrowser_put_RegisterAsBrowser,
    WebBrowser_get_RegisterAsDropTarget,
    WebBrowser_put_RegisterAsDropTarget,
    WebBrowser_get_TheaterMode,
    WebBrowser_put_TheaterMode,
    WebBrowser_get_AddressBar,
    WebBrowser_put_AddressBar,
    WebBrowser_get_Resizable,
    WebBrowser_put_Resizable
};

HRESULT WebBrowser_Create(IUnknown *pOuter, REFIID riid, void **ppv)
{
    WebBrowser *ret;
    HRESULT hres;

    TRACE("(%p %s %p)\n", pOuter, debugstr_guid(riid), ppv);

    ret = HeapAlloc(GetProcessHeap(), 0, sizeof(WebBrowser));

    ret->lpWebBrowser2Vtbl = &WebBrowser2Vtbl;
    ret->ref = 0;

    ret->document = NULL;
    ret->url = NULL;

    WebBrowser_OleObject_Init(ret);
    WebBrowser_ViewObject_Init(ret);
    WebBrowser_Persist_Init(ret);
    WebBrowser_ClassInfo_Init(ret);
    WebBrowser_Misc_Init(ret);
    WebBrowser_Events_Init(ret);
    WebBrowser_ClientSite_Init(ret);
    WebBrowser_DocHost_Init(ret);
    WebBrowser_Frame_Init(ret);

    hres = IWebBrowser_QueryInterface(WEBBROWSER(ret), riid, ppv);
    if(SUCCEEDED(hres)) {
        SHDOCVW_LockModule();
    }else {
        HeapFree(GetProcessHeap(), 0, ret);
        return hres;
    }

    return hres;
}
