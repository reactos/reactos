/*
 * Copyright 2006 Jacek Caban for CodeWeavers
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

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

#define WEBBROWSER_THIS(iface) DEFINE_THIS(InternetExplorer, WebBrowser2, iface)

static HRESULT WINAPI InternetExplorer_QueryInterface(IWebBrowser2 *iface, REFIID riid, LPVOID *ppv)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);

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
    }else if(IsEqualGUID(&IID_IConnectionPointContainer, riid)) {
        TRACE("(%p)->(IID_IConnectionPointContainer %p)\n", This, ppv);
        *ppv = CONPTCONT(&This->doc_host.cps);
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("(%p)->(%s %p) interface not supported\n", This, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI InternetExplorer_AddRef(IWebBrowser2 *iface)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%d\n", This, ref);
    return ref;
}

static ULONG WINAPI InternetExplorer_Release(IWebBrowser2 *iface)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        DocHost_Release(&This->doc_host);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI InternetExplorer_GetTypeInfoCount(IWebBrowser2 *iface, UINT *pctinfo)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_GetTypeInfo(IWebBrowser2 *iface, UINT iTInfo, LCID lcid,
                                     LPTYPEINFO *ppTInfo)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%d %d %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_GetIDsOfNames(IWebBrowser2 *iface, REFIID riid,
                                       LPOLESTR *rgszNames, UINT cNames,
                                       LCID lcid, DISPID *rgDispId)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%s %p %d %d %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
            lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_Invoke(IWebBrowser2 *iface, DISPID dispIdMember,
                                REFIID riid, LCID lcid, WORD wFlags,
                                DISPPARAMS *pDispParams, VARIANT *pVarResult,
                                EXCEPINFO *pExepInfo, UINT *puArgErr)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%d %s %d %08x %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_GoBack(IWebBrowser2 *iface)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_GoForward(IWebBrowser2 *iface)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_GoHome(IWebBrowser2 *iface)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    TRACE("(%p)\n", This);
    return go_home(&This->doc_host);
}

static HRESULT WINAPI InternetExplorer_GoSearch(IWebBrowser2 *iface)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_Navigate(IWebBrowser2 *iface, BSTR szUrl,
                                  VARIANT *Flags, VARIANT *TargetFrameName,
                                  VARIANT *PostData, VARIANT *Headers)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);

    TRACE("(%p)->(%s %p %p %p %p)\n", This, debugstr_w(szUrl), Flags, TargetFrameName,
          PostData, Headers);

    return navigate_url(&This->doc_host, szUrl, Flags, TargetFrameName, PostData, Headers);
}

static HRESULT WINAPI InternetExplorer_Refresh(IWebBrowser2 *iface)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_Refresh2(IWebBrowser2 *iface, VARIANT *Level)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, Level);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_Stop(IWebBrowser2 *iface)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Application(IWebBrowser2 *iface, IDispatch **ppDisp)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppDisp);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Parent(IWebBrowser2 *iface, IDispatch **ppDisp)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppDisp);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Container(IWebBrowser2 *iface, IDispatch **ppDisp)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppDisp);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Document(IWebBrowser2 *iface, IDispatch **ppDisp)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, ppDisp);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_TopLevelContainer(IWebBrowser2 *iface, VARIANT_BOOL *pBool)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pBool);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Type(IWebBrowser2 *iface, BSTR *Type)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, Type);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Left(IWebBrowser2 *iface, LONG *pl)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pl);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_Left(IWebBrowser2 *iface, LONG Left)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%d)\n", This, Left);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Top(IWebBrowser2 *iface, LONG *pl)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pl);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_Top(IWebBrowser2 *iface, LONG Top)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%d)\n", This, Top);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Width(IWebBrowser2 *iface, LONG *pl)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pl);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_Width(IWebBrowser2 *iface, LONG Width)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%d)\n", This, Width);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Height(IWebBrowser2 *iface, LONG *pl)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pl);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_Height(IWebBrowser2 *iface, LONG Height)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%d)\n", This, Height);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_LocationName(IWebBrowser2 *iface, BSTR *LocationName)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, LocationName);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_LocationURL(IWebBrowser2 *iface, BSTR *LocationURL)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, LocationURL);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Busy(IWebBrowser2 *iface, VARIANT_BOOL *pBool)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pBool);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_Quit(IWebBrowser2 *iface)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_ClientToWindow(IWebBrowser2 *iface, int *pcx, int *pcy)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p %p)\n", This, pcx, pcy);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_PutProperty(IWebBrowser2 *iface, BSTR szProperty, VARIANT vtValue)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(szProperty));
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_GetProperty(IWebBrowser2 *iface, BSTR szProperty, VARIANT *pvtValue)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(szProperty), pvtValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Name(IWebBrowser2 *iface, BSTR *Name)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, Name);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_HWND(IWebBrowser2 *iface, LONG *pHWND)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pHWND);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_FullName(IWebBrowser2 *iface, BSTR *FullName)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, FullName);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Path(IWebBrowser2 *iface, BSTR *Path)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, Path);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Visible(IWebBrowser2 *iface, VARIANT_BOOL *pBool)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pBool);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_Visible(IWebBrowser2 *iface, VARIANT_BOOL Value)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    TRACE("(%p)->(%x)\n", This, Value);

    ShowWindow(This->frame_hwnd, Value ? SW_SHOW : SW_HIDE);

    return S_OK;
}

static HRESULT WINAPI InternetExplorer_get_StatusBar(IWebBrowser2 *iface, VARIANT_BOOL *pBool)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pBool);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_StatusBar(IWebBrowser2 *iface, VARIANT_BOOL Value)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%x)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_StatusText(IWebBrowser2 *iface, BSTR *StatusText)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, StatusText);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_StatusText(IWebBrowser2 *iface, BSTR StatusText)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(StatusText));
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_ToolBar(IWebBrowser2 *iface, int *Value)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_ToolBar(IWebBrowser2 *iface, int Value)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%d)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_MenuBar(IWebBrowser2 *iface, VARIANT_BOOL *Value)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_MenuBar(IWebBrowser2 *iface, VARIANT_BOOL Value)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%x)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_FullScreen(IWebBrowser2 *iface, VARIANT_BOOL *pbFullScreen)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pbFullScreen);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_FullScreen(IWebBrowser2 *iface, VARIANT_BOOL bFullScreen)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%x)\n", This, bFullScreen);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_Navigate2(IWebBrowser2 *iface, VARIANT *URL, VARIANT *Flags,
        VARIANT *TargetFrameName, VARIANT *PostData, VARIANT *Headers)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);

    TRACE("(%p)->(%p %p %p %p %p)\n", This, URL, Flags, TargetFrameName, PostData, Headers);

    if(!URL)
        return S_OK;

    if(V_VT(URL) != VT_BSTR) {
        FIXME("Unsupported V_VT(URL) %d\n", V_VT(URL));
        return E_INVALIDARG;
    }

    return navigate_url(&This->doc_host, V_BSTR(URL), Flags, TargetFrameName, PostData, Headers);
}

static HRESULT WINAPI InternetExplorer_QueryStatusWB(IWebBrowser2 *iface, OLECMDID cmdID, OLECMDF *pcmdf)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%d %p)\n", This, cmdID, pcmdf);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_ExecWB(IWebBrowser2 *iface, OLECMDID cmdID,
        OLECMDEXECOPT cmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%d %d %p %p)\n", This, cmdID, cmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_ShowBrowserBar(IWebBrowser2 *iface, VARIANT *pvaClsid,
        VARIANT *pvarShow, VARIANT *pvarSize)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p %p %p)\n", This, pvaClsid, pvarShow, pvarSize);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_ReadyState(IWebBrowser2 *iface, READYSTATE *lpReadyState)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, lpReadyState);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Offline(IWebBrowser2 *iface, VARIANT_BOOL *pbOffline)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pbOffline);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_Offline(IWebBrowser2 *iface, VARIANT_BOOL bOffline)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%x)\n", This, bOffline);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Silent(IWebBrowser2 *iface, VARIANT_BOOL *pbSilent)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pbSilent);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_Silent(IWebBrowser2 *iface, VARIANT_BOOL bSilent)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%x)\n", This, bSilent);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_RegisterAsBrowser(IWebBrowser2 *iface,
        VARIANT_BOOL *pbRegister)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pbRegister);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_RegisterAsBrowser(IWebBrowser2 *iface,
        VARIANT_BOOL bRegister)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%x)\n", This, bRegister);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_RegisterAsDropTarget(IWebBrowser2 *iface,
        VARIANT_BOOL *pbRegister)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pbRegister);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_RegisterAsDropTarget(IWebBrowser2 *iface,
        VARIANT_BOOL bRegister)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%x)\n", This, bRegister);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_TheaterMode(IWebBrowser2 *iface, VARIANT_BOOL *pbRegister)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, pbRegister);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_TheaterMode(IWebBrowser2 *iface, VARIANT_BOOL bRegister)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%x)\n", This, bRegister);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_AddressBar(IWebBrowser2 *iface, VARIANT_BOOL *Value)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_AddressBar(IWebBrowser2 *iface, VARIANT_BOOL Value)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%x)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Resizable(IWebBrowser2 *iface, VARIANT_BOOL *Value)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%p)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_Resizable(IWebBrowser2 *iface, VARIANT_BOOL Value)
{
    InternetExplorer *This = WEBBROWSER_THIS(iface);
    FIXME("(%p)->(%x)\n", This, Value);
    return E_NOTIMPL;
}

#undef WEBBROWSER_THIS

static const IWebBrowser2Vtbl InternetExplorerVtbl =
{
    InternetExplorer_QueryInterface,
    InternetExplorer_AddRef,
    InternetExplorer_Release,
    InternetExplorer_GetTypeInfoCount,
    InternetExplorer_GetTypeInfo,
    InternetExplorer_GetIDsOfNames,
    InternetExplorer_Invoke,
    InternetExplorer_GoBack,
    InternetExplorer_GoForward,
    InternetExplorer_GoHome,
    InternetExplorer_GoSearch,
    InternetExplorer_Navigate,
    InternetExplorer_Refresh,
    InternetExplorer_Refresh2,
    InternetExplorer_Stop,
    InternetExplorer_get_Application,
    InternetExplorer_get_Parent,
    InternetExplorer_get_Container,
    InternetExplorer_get_Document,
    InternetExplorer_get_TopLevelContainer,
    InternetExplorer_get_Type,
    InternetExplorer_get_Left,
    InternetExplorer_put_Left,
    InternetExplorer_get_Top,
    InternetExplorer_put_Top,
    InternetExplorer_get_Width,
    InternetExplorer_put_Width,
    InternetExplorer_get_Height,
    InternetExplorer_put_Height,
    InternetExplorer_get_LocationName,
    InternetExplorer_get_LocationURL,
    InternetExplorer_get_Busy,
    InternetExplorer_Quit,
    InternetExplorer_ClientToWindow,
    InternetExplorer_PutProperty,
    InternetExplorer_GetProperty,
    InternetExplorer_get_Name,
    InternetExplorer_get_HWND,
    InternetExplorer_get_FullName,
    InternetExplorer_get_Path,
    InternetExplorer_get_Visible,
    InternetExplorer_put_Visible,
    InternetExplorer_get_StatusBar,
    InternetExplorer_put_StatusBar,
    InternetExplorer_get_StatusText,
    InternetExplorer_put_StatusText,
    InternetExplorer_get_ToolBar,
    InternetExplorer_put_ToolBar,
    InternetExplorer_get_MenuBar,
    InternetExplorer_put_MenuBar,
    InternetExplorer_get_FullScreen,
    InternetExplorer_put_FullScreen,
    InternetExplorer_Navigate2,
    InternetExplorer_QueryStatusWB,
    InternetExplorer_ExecWB,
    InternetExplorer_ShowBrowserBar,
    InternetExplorer_get_ReadyState,
    InternetExplorer_get_Offline,
    InternetExplorer_put_Offline,
    InternetExplorer_get_Silent,
    InternetExplorer_put_Silent,
    InternetExplorer_get_RegisterAsBrowser,
    InternetExplorer_put_RegisterAsBrowser,
    InternetExplorer_get_RegisterAsDropTarget,
    InternetExplorer_put_RegisterAsDropTarget,
    InternetExplorer_get_TheaterMode,
    InternetExplorer_put_TheaterMode,
    InternetExplorer_get_AddressBar,
    InternetExplorer_put_AddressBar,
    InternetExplorer_get_Resizable,
    InternetExplorer_put_Resizable
};

void InternetExplorer_WebBrowser_Init(InternetExplorer *This)
{
    This->lpWebBrowser2Vtbl = &InternetExplorerVtbl;
}
