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

#include "ieframe.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ieframe);

static inline InternetExplorer *impl_from_IWebBrowser2(IWebBrowser2 *iface)
{
    return CONTAINING_RECORD(iface, InternetExplorer, IWebBrowser2_iface);
}

static HRESULT WINAPI InternetExplorer_QueryInterface(IWebBrowser2 *iface, REFIID riid, LPVOID *ppv)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);

    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IWebBrowser2_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IWebBrowser2_iface;
    }else if(IsEqualGUID(&IID_IWebBrowser, riid)) {
        TRACE("(%p)->(IID_IWebBrowser %p)\n", This, ppv);
        *ppv = &This->IWebBrowser2_iface;
    }else if(IsEqualGUID(&IID_IWebBrowserApp, riid)) {
        TRACE("(%p)->(IID_IWebBrowserApp %p)\n", This, ppv);
        *ppv = &This->IWebBrowser2_iface;
    }else if(IsEqualGUID(&IID_IWebBrowser2, riid)) {
        TRACE("(%p)->(IID_IWebBrowser2 %p)\n", This, ppv);
        *ppv = &This->IWebBrowser2_iface;
    }else if(IsEqualGUID(&IID_IConnectionPointContainer, riid)) {
        TRACE("(%p)->(IID_IConnectionPointContainer %p)\n", This, ppv);
        *ppv = &This->doc_host.cps.IConnectionPointContainer_iface;
    }else if(IsEqualGUID(&IID_IExternalConnection, riid)) {
        TRACE("(%p)->(IID_IExternalConnection %p)\n", This, ppv);
        *ppv = &This->IExternalConnection_iface;
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        TRACE("(%p)->(IID_IServiceProvider %p)\n", This, ppv);
        *ppv = &This->IServiceProvider_iface;
    }else if(HlinkFrame_QI(&This->hlink_frame, riid, ppv)) {
        return S_OK;
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
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref=%ld\n", This, ref);
    return ref;
}

static ULONG WINAPI InternetExplorer_Release(IWebBrowser2 *iface)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        deactivate_document(&This->doc_host);
        DocHost_Release(&This->doc_host);

        if(This->frame_hwnd)
            DestroyWindow(This->frame_hwnd);
        list_remove(&This->entry);
        free(This);

        released_obj();
    }

    return ref;
}

static HRESULT WINAPI InternetExplorer_GetTypeInfoCount(IWebBrowser2 *iface, UINT *pctinfo)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI InternetExplorer_GetTypeInfo(IWebBrowser2 *iface, UINT iTInfo, LCID lcid,
                                     LPTYPEINFO *ppTInfo)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    ITypeInfo *typeinfo;
    HRESULT hres;

    TRACE("(%p)->(%d %ld %p)\n", This, iTInfo, lcid, ppTInfo);

    hres = get_typeinfo(IWebBrowser2_tid, &typeinfo);
    if(FAILED(hres))
        return hres;

    ITypeInfo_AddRef(typeinfo);
    *ppTInfo = typeinfo;
    return S_OK;
}

static HRESULT WINAPI InternetExplorer_GetIDsOfNames(IWebBrowser2 *iface, REFIID riid,
                                       LPOLESTR *rgszNames, UINT cNames,
                                       LCID lcid, DISPID *rgDispId)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    ITypeInfo *typeinfo;
    HRESULT hres;

    TRACE("(%p)->(%s %p %d %ld %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    hres = get_typeinfo(IWebBrowser2_tid, &typeinfo);
    if(FAILED(hres))
        return hres;

    return ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
}

static HRESULT WINAPI InternetExplorer_Invoke(IWebBrowser2 *iface, DISPID dispIdMember,
                                REFIID riid, LCID lcid, WORD wFlags,
                                DISPPARAMS *pDispParams, VARIANT *pVarResult,
                                EXCEPINFO *pExepInfo, UINT *puArgErr)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    ITypeInfo *typeinfo;
    HRESULT hres;

    TRACE("(%p)->(%ld %s %ld %08x %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);

    hres = get_typeinfo(IWebBrowser2_tid, &typeinfo);
    if(FAILED(hres))
        return hres;

    return ITypeInfo_Invoke(typeinfo, &This->IWebBrowser2_iface, dispIdMember, wFlags, pDispParams,
            pVarResult, pExepInfo, puArgErr);
}

static HRESULT WINAPI InternetExplorer_GoBack(IWebBrowser2 *iface)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    TRACE("(%p)\n", This);
    return go_back(&This->doc_host);
}

static HRESULT WINAPI InternetExplorer_GoForward(IWebBrowser2 *iface)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    TRACE("(%p)\n", This);
    return go_forward(&This->doc_host);
}

static HRESULT WINAPI InternetExplorer_GoHome(IWebBrowser2 *iface)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    TRACE("(%p)\n", This);
    return go_home(&This->doc_host);
}

static HRESULT WINAPI InternetExplorer_GoSearch(IWebBrowser2 *iface)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_Navigate(IWebBrowser2 *iface, BSTR szUrl,
                                  VARIANT *Flags, VARIANT *TargetFrameName,
                                  VARIANT *PostData, VARIANT *Headers)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%s %s %s %s %s)\n", This, debugstr_w(szUrl), debugstr_variant(Flags),
          debugstr_variant(TargetFrameName), debugstr_variant(PostData), debugstr_variant(Headers));

    return navigate_url(&This->doc_host, szUrl, Flags, TargetFrameName, PostData, Headers);
}

static HRESULT WINAPI InternetExplorer_Refresh(IWebBrowser2 *iface)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)\n", This);

    return refresh_document(&This->doc_host, NULL);
}

static HRESULT WINAPI InternetExplorer_Refresh2(IWebBrowser2 *iface, VARIANT *Level)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_variant(Level));

    return refresh_document(&This->doc_host, Level);
}

static HRESULT WINAPI InternetExplorer_Stop(IWebBrowser2 *iface)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Application(IWebBrowser2 *iface, IDispatch **ppDisp)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, ppDisp);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Parent(IWebBrowser2 *iface, IDispatch **ppDisp)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, ppDisp);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Container(IWebBrowser2 *iface, IDispatch **ppDisp)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, ppDisp);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Document(IWebBrowser2 *iface, IDispatch **ppDisp)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, ppDisp);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_TopLevelContainer(IWebBrowser2 *iface, VARIANT_BOOL *pBool)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, pBool);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Type(IWebBrowser2 *iface, BSTR *Type)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, Type);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Left(IWebBrowser2 *iface, LONG *pl)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, pl);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_Left(IWebBrowser2 *iface, LONG Left)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%ld)\n", This, Left);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Top(IWebBrowser2 *iface, LONG *pl)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, pl);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_Top(IWebBrowser2 *iface, LONG Top)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%ld)\n", This, Top);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Width(IWebBrowser2 *iface, LONG *pl)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, pl);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_Width(IWebBrowser2 *iface, LONG Width)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%ld)\n", This, Width);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Height(IWebBrowser2 *iface, LONG *pl)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, pl);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_Height(IWebBrowser2 *iface, LONG Height)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%ld)\n", This, Height);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_LocationName(IWebBrowser2 *iface, BSTR *LocationName)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, LocationName);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_LocationURL(IWebBrowser2 *iface, BSTR *LocationURL)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%p)\n", This, LocationURL);

    return get_location_url(&This->doc_host, LocationURL);
}

static HRESULT WINAPI InternetExplorer_get_Busy(IWebBrowser2 *iface, VARIANT_BOOL *pBool)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%p)\n", This, pBool);

    *pBool = This->doc_host.busy;
    return S_OK;
}

static HRESULT WINAPI InternetExplorer_Quit(IWebBrowser2 *iface)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_ClientToWindow(IWebBrowser2 *iface, int *pcx, int *pcy)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p %p)\n", This, pcx, pcy);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_PutProperty(IWebBrowser2 *iface, BSTR szProperty, VARIANT vtValue)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_w(szProperty), debugstr_variant(&vtValue));
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_GetProperty(IWebBrowser2 *iface, BSTR szProperty, VARIANT *pvtValue)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(szProperty), pvtValue);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Name(IWebBrowser2 *iface, BSTR *Name)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, Name);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_HWND(IWebBrowser2 *iface, SHANDLE_PTR *pHWND)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%p)\n", This, pHWND);

    *pHWND = (SHANDLE_PTR)This->frame_hwnd;
    return S_OK;
}

static HRESULT WINAPI InternetExplorer_get_FullName(IWebBrowser2 *iface, BSTR *FullName)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, FullName);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Path(IWebBrowser2 *iface, BSTR *Path)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, Path);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Visible(IWebBrowser2 *iface, VARIANT_BOOL *pBool)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%p)\n", This, pBool);

    *pBool = IsWindowVisible(This->frame_hwnd) ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI InternetExplorer_put_Visible(IWebBrowser2 *iface, VARIANT_BOOL Value)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    TRACE("(%p)->(%x)\n", This, Value);

    ShowWindow(This->frame_hwnd, Value ? SW_SHOW : SW_HIDE);

    return S_OK;
}

static HRESULT WINAPI InternetExplorer_get_StatusBar(IWebBrowser2 *iface, VARIANT_BOOL *pBool)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, pBool);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_StatusBar(IWebBrowser2 *iface, VARIANT_BOOL Value)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%x)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_StatusText(IWebBrowser2 *iface, BSTR *StatusText)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, StatusText);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_StatusText(IWebBrowser2 *iface, BSTR StatusText)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%s)\n", This, debugstr_w(StatusText));

    return update_ie_statustext(This, StatusText);
}

static HRESULT WINAPI InternetExplorer_get_ToolBar(IWebBrowser2 *iface, int *Value)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_ToolBar(IWebBrowser2 *iface, int Value)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%d)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_MenuBar(IWebBrowser2 *iface, VARIANT_BOOL *Value)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_MenuBar(IWebBrowser2 *iface, VARIANT_BOOL Value)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    HMENU menu = NULL;

    TRACE("(%p)->(%x)\n", This, Value);

    if(Value)
        menu = This->menu;

    if(!SetMenu(This->frame_hwnd, menu))
        return HRESULT_FROM_WIN32(GetLastError());

    return S_OK;
}

static HRESULT WINAPI InternetExplorer_get_FullScreen(IWebBrowser2 *iface, VARIANT_BOOL *pbFullScreen)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, pbFullScreen);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_FullScreen(IWebBrowser2 *iface, VARIANT_BOOL bFullScreen)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%x)\n", This, bFullScreen);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_Navigate2(IWebBrowser2 *iface, VARIANT *URL, VARIANT *Flags,
        VARIANT *TargetFrameName, VARIANT *PostData, VARIANT *Headers)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);

    TRACE("(%p)->(%s %s %s %s %s)\n", This, debugstr_variant(URL), debugstr_variant(Flags),
        debugstr_variant(TargetFrameName), debugstr_variant(PostData), debugstr_variant(Headers));

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
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%d %p)\n", This, cmdID, pcmdf);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_ExecWB(IWebBrowser2 *iface, OLECMDID cmdID,
        OLECMDEXECOPT cmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%d %d %s %p)\n", This, cmdID, cmdexecopt, debugstr_variant(pvaIn), pvaOut);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_ShowBrowserBar(IWebBrowser2 *iface, VARIANT *pvaClsid,
        VARIANT *pvarShow, VARIANT *pvarSize)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%s %s %s)\n", This, debugstr_variant(pvaClsid), debugstr_variant(pvarShow),
        debugstr_variant(pvarSize));
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_ReadyState(IWebBrowser2 *iface, READYSTATE *lpReadyState)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, lpReadyState);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Offline(IWebBrowser2 *iface, VARIANT_BOOL *pbOffline)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, pbOffline);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_Offline(IWebBrowser2 *iface, VARIANT_BOOL bOffline)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%x)\n", This, bOffline);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Silent(IWebBrowser2 *iface, VARIANT_BOOL *pbSilent)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, pbSilent);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_Silent(IWebBrowser2 *iface, VARIANT_BOOL bSilent)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%x)\n", This, bSilent);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_RegisterAsBrowser(IWebBrowser2 *iface,
        VARIANT_BOOL *pbRegister)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, pbRegister);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_RegisterAsBrowser(IWebBrowser2 *iface,
        VARIANT_BOOL bRegister)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%x)\n", This, bRegister);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_RegisterAsDropTarget(IWebBrowser2 *iface,
        VARIANT_BOOL *pbRegister)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, pbRegister);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_RegisterAsDropTarget(IWebBrowser2 *iface,
        VARIANT_BOOL bRegister)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%x)\n", This, bRegister);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_TheaterMode(IWebBrowser2 *iface, VARIANT_BOOL *pbRegister)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, pbRegister);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_TheaterMode(IWebBrowser2 *iface, VARIANT_BOOL bRegister)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%x)\n", This, bRegister);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_AddressBar(IWebBrowser2 *iface, VARIANT_BOOL *Value)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_AddressBar(IWebBrowser2 *iface, VARIANT_BOOL Value)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%x)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_get_Resizable(IWebBrowser2 *iface, VARIANT_BOOL *Value)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%p)\n", This, Value);
    return E_NOTIMPL;
}

static HRESULT WINAPI InternetExplorer_put_Resizable(IWebBrowser2 *iface, VARIANT_BOOL Value)
{
    InternetExplorer *This = impl_from_IWebBrowser2(iface);
    FIXME("(%p)->(%x)\n", This, Value);
    return E_NOTIMPL;
}

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

static inline InternetExplorer *impl_from_IExternalConnection(IExternalConnection *iface)
{
    return CONTAINING_RECORD(iface, InternetExplorer, IExternalConnection_iface);
}

/*
 * Document may keep references to InternetExplorer object causing circular references.
 * We keep track of external references and release the document object when all
 * external references are released. A visible main window is also considered as
 * an external reference.
 */
DWORD release_extern_ref(InternetExplorer *This, BOOL last_closes)
{
    LONG ref = InterlockedDecrement(&This->extern_ref);

    TRACE("ref = %ld\n", ref);

    if(ref)
        return ref;

    if(!last_closes) {
        WARN("Last external connection released with FALSE last_closes.\n");
        return ref;
    }

    deactivate_document(&This->doc_host);
    return ref;
}

static HRESULT WINAPI ExternalConnection_QueryInterface(IExternalConnection *iface, REFIID riid, void **ppv)
{
    InternetExplorer *This = impl_from_IExternalConnection(iface);
    return IWebBrowser2_QueryInterface(&This->IWebBrowser2_iface, riid, ppv);
}

static ULONG WINAPI ExternalConnection_AddRef(IExternalConnection *iface)
{
    InternetExplorer *This = impl_from_IExternalConnection(iface);
    return IWebBrowser2_AddRef(&This->IWebBrowser2_iface);
}

static ULONG WINAPI ExternalConnection_Release(IExternalConnection *iface)
{
    InternetExplorer *This = impl_from_IExternalConnection(iface);
    return IWebBrowser2_Release(&This->IWebBrowser2_iface);
}

static DWORD WINAPI ExternalConnection_AddConnection(IExternalConnection *iface, DWORD extconn, DWORD reserved)
{
    InternetExplorer *This = impl_from_IExternalConnection(iface);

    TRACE("(%p)->(%lx %lx)\n", This, extconn, reserved);

    if(extconn != EXTCONN_STRONG)
        return 0;

    return InterlockedIncrement(&This->extern_ref);
}

static DWORD WINAPI ExternalConnection_ReleaseConnection(IExternalConnection *iface, DWORD extconn,
        DWORD reserved, BOOL fLastReleaseCloses)
{
    InternetExplorer *This = impl_from_IExternalConnection(iface);

    TRACE("(%p)->(%lx %lx %x)\n", This, extconn, reserved, fLastReleaseCloses);

    if(extconn != EXTCONN_STRONG)
        return 0;

    return release_extern_ref(This, fLastReleaseCloses);
}

static const IExternalConnectionVtbl ExternalConnectionVtbl = {
    ExternalConnection_QueryInterface,
    ExternalConnection_AddRef,
    ExternalConnection_Release,
    ExternalConnection_AddConnection,
    ExternalConnection_ReleaseConnection
};

static inline InternetExplorer *impl_from_IServiceProvider(IServiceProvider *iface)
{
    return CONTAINING_RECORD(iface, InternetExplorer, IServiceProvider_iface);
}

static HRESULT WINAPI IEServiceProvider_QueryInterface(IServiceProvider *iface,
            REFIID riid, LPVOID *ppv)
{
    InternetExplorer *This = impl_from_IServiceProvider(iface);
    return IWebBrowser2_QueryInterface(&This->IWebBrowser2_iface, riid, ppv);
}

static ULONG WINAPI IEServiceProvider_AddRef(IServiceProvider *iface)
{
    InternetExplorer *This = impl_from_IServiceProvider(iface);
    return IWebBrowser2_AddRef(&This->IWebBrowser2_iface);
}

static ULONG WINAPI IEServiceProvider_Release(IServiceProvider *iface)
{
    InternetExplorer *This = impl_from_IServiceProvider(iface);
    return IWebBrowser2_Release(&This->IWebBrowser2_iface);
}

static HRESULT WINAPI IEServiceProvider_QueryService(IServiceProvider *iface,
        REFGUID guidService, REFIID riid, void **ppv)
{
    InternetExplorer *This = impl_from_IServiceProvider(iface);

    if(IsEqualGUID(&SID_SHTMLWindow, riid)) {
        TRACE("(%p)->(SID_SHTMLWindow)\n", This);
        return IHTMLWindow2_QueryInterface(&This->doc_host.html_window.IHTMLWindow2_iface, riid, ppv);
    }

    FIXME("(%p)->(%s, %s %p)\n", This, debugstr_guid(guidService), debugstr_guid(riid), ppv);
    *ppv = NULL;
    return E_NOINTERFACE;
}

static const IServiceProviderVtbl ServiceProviderVtbl =
{
    IEServiceProvider_QueryInterface,
    IEServiceProvider_AddRef,
    IEServiceProvider_Release,
    IEServiceProvider_QueryService
};

void InternetExplorer_WebBrowser_Init(InternetExplorer *This)
{
    This->IWebBrowser2_iface.lpVtbl = &InternetExplorerVtbl;
    This->IExternalConnection_iface.lpVtbl = &ExternalConnectionVtbl;
    This->IServiceProvider_iface.lpVtbl = &ServiceProviderVtbl;
}

HRESULT WINAPI IERefreshElevationPolicy(void)
{
    FIXME("(): stub\n");
    return S_OK;
}
