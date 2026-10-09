/*
 * Implementation of IShellBrowser interface
 *
 * Copyright 2011 Piotr Caban for CodeWeavers
 * Copyright 2012 Jacek Caban for CodeWeavers
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

#include <assert.h>

#include "ieframe.h"
#include "exdispid.h"
#include "shlwapi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ieframe);

const WCHAR *error_url_frag(const WCHAR *url)
{
    if(!wcsncmp(url, L"res://", ARRAY_SIZE(L"res://")-1)) {
        WCHAR buf[MAX_PATH];
        UINT len = GetSystemDirectoryW(buf, ARRAY_SIZE(buf));

        if(len && !wcsncmp(url + ARRAY_SIZE(L"res://")-1, buf, len)) {
            len += ARRAY_SIZE(L"res://")-1;
            if(!wcsncmp(url + len, L"\\shdoclc.dll/ERROR.HTM", ARRAY_SIZE(L"\\shdoclc.dll/ERROR.HTM")-1)) {
                len += ARRAY_SIZE(L"\\shdoclc.dll/ERROR.HTM")-1;
                url = wcschr(url + len, '#');
                return url ? url + 1 : NULL;
            }
        }
    }
    return NULL;
}

static inline ShellBrowser *impl_from_IShellBrowser(IShellBrowser *iface)
{
    return CONTAINING_RECORD(iface, ShellBrowser, IShellBrowser_iface);
}

static HRESULT WINAPI ShellBrowser_QueryInterface(IShellBrowser* iface, REFIID riid, void **ppv)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IShellBrowser_iface;
    }else if(IsEqualGUID(&IID_IOleWindow, riid)) {
        TRACE("(%p)->(IID_IOleWindow %p)\n", This, ppv);
        *ppv = &This->IShellBrowser_iface;
    }else if(IsEqualGUID(&IID_IShellBrowser, riid)) {
        TRACE("(%p)->(IID_IShellBrowser %p)\n", This, ppv);
        *ppv = &This->IShellBrowser_iface;
    }else if(IsEqualGUID(&IID_IBrowserService, riid)) {
        TRACE("(%p)->(IID_IBrowserService %p)\n", This, ppv);
        *ppv = &This->IBrowserService_iface;
    }else if(IsEqualGUID(&IID_IDocObjectService, riid)) {
        TRACE("(%p)->(IID_IDocObjectService %p)\n", This, ppv);
        *ppv = &This->IDocObjectService_iface;
    }else {
        FIXME("%p %s %p\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ShellBrowser_AddRef(
        IShellBrowser* iface)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI ShellBrowser_Release(IShellBrowser* iface)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        assert(!This->doc_host);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI ShellBrowser_GetWindow(
        IShellBrowser* iface,
        HWND *phwnd)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    FIXME("%p %p\n", This, phwnd);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_ContextSensitiveHelp(
        IShellBrowser* iface,
        BOOL fEnterMode)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    FIXME("%p %d\n", This, fEnterMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_InsertMenusSB(
        IShellBrowser* iface,
        HMENU hmenuShared,
        LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    FIXME("%p %p %p\n", This, hmenuShared, lpMenuWidths);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_SetMenuSB(
        IShellBrowser* iface,
        HMENU hmenuShared,
        HOLEMENU holemenuReserved,
        HWND hwndActiveObject)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    FIXME("%p %p %p %p\n", This, hmenuShared, holemenuReserved, hwndActiveObject);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_RemoveMenusSB(
        IShellBrowser* iface,
        HMENU hmenuShared)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    FIXME("%p %p\n", This, hmenuShared);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_SetStatusTextSB(
        IShellBrowser* iface,
        LPCOLESTR pszStatusText)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    FIXME("%p %s\n", This, debugstr_w(pszStatusText));
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_EnableModelessSB(
        IShellBrowser* iface,
        BOOL fEnable)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    FIXME("%p %d\n", This, fEnable);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_TranslateAcceleratorSB(
        IShellBrowser* iface,
        MSG *pmsg,
        WORD wID)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    FIXME("%p %p %d\n", This, pmsg, (int)wID);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_BrowseObject(
        IShellBrowser* iface,
        LPCITEMIDLIST pidl,
        UINT wFlags)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    FIXME("%p %p %u\n", This, pidl, wFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_GetViewStateStream(
        IShellBrowser* iface,
        DWORD grfMode,
        IStream **ppStrm)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    FIXME("%p %lx %p\n", This, grfMode, ppStrm);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_GetControlWindow(
        IShellBrowser* iface,
        UINT id,
        HWND *phwnd)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    FIXME("%p %u %p\n", This, id, phwnd);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_SendControlMsg(
        IShellBrowser* iface,
        UINT id,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam,
        LRESULT *pret)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    FIXME("%p %u %u %p\n", This, id, uMsg, pret);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_QueryActiveShellView(
        IShellBrowser* iface,
        IShellView **ppshv)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    FIXME("%p %p\n", This, ppshv);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_OnViewWindowActive(
        IShellBrowser* iface,
        IShellView *pshv)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    FIXME("%p %p\n", This, pshv);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellBrowser_SetToolbarItems(
        IShellBrowser* iface,
        LPTBBUTTONSB lpButtons,
        UINT nButtons,
        UINT uFlags)
{
    ShellBrowser *This = impl_from_IShellBrowser(iface);
    FIXME("%p %p %u %u\n", This, lpButtons, nButtons, uFlags);
    return E_NOTIMPL;
}

static const IShellBrowserVtbl ShellBrowserVtbl = {
    ShellBrowser_QueryInterface,
    ShellBrowser_AddRef,
    ShellBrowser_Release,
    ShellBrowser_GetWindow,
    ShellBrowser_ContextSensitiveHelp,
    ShellBrowser_InsertMenusSB,
    ShellBrowser_SetMenuSB,
    ShellBrowser_RemoveMenusSB,
    ShellBrowser_SetStatusTextSB,
    ShellBrowser_EnableModelessSB,
    ShellBrowser_TranslateAcceleratorSB,
    ShellBrowser_BrowseObject,
    ShellBrowser_GetViewStateStream,
    ShellBrowser_GetControlWindow,
    ShellBrowser_SendControlMsg,
    ShellBrowser_QueryActiveShellView,
    ShellBrowser_OnViewWindowActive,
    ShellBrowser_SetToolbarItems
};

static inline ShellBrowser *impl_from_IBrowserService(IBrowserService *iface)
{
    return CONTAINING_RECORD(iface, ShellBrowser, IBrowserService_iface);
}

static HRESULT WINAPI BrowserService_QueryInterface(
        IBrowserService* iface,
        REFIID riid,
        void **ppvObject)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    return IShellBrowser_QueryInterface(&This->IShellBrowser_iface, riid, ppvObject);
}

static ULONG WINAPI BrowserService_AddRef(
        IBrowserService *iface)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    return IShellBrowser_AddRef(&This->IShellBrowser_iface);
}

static ULONG WINAPI BrowserService_Release(
        IBrowserService* iface)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    return IShellBrowser_Release(&This->IShellBrowser_iface);
}

static HRESULT WINAPI BrowserService_GetParentSite(
        IBrowserService* iface,
        IOleInPlaceSite **ppipsite)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    FIXME("%p %p\n", This, ppipsite);
    return E_NOTIMPL;
}

static HRESULT WINAPI BrowserService_SetTitle(
        IBrowserService* iface,
        IShellView *psv,
        LPCWSTR pszName)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    FIXME("%p %p %s\n", This, psv, debugstr_w(pszName));
    return E_NOTIMPL;
}

static HRESULT WINAPI BrowserService_GetTitle(
        IBrowserService* iface,
        IShellView *psv,
        LPWSTR pszName,
        DWORD cchName)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    FIXME("%p %p %p %ld\n", This, psv, pszName, cchName);
    return E_NOTIMPL;
}

static HRESULT WINAPI BrowserService_GetOleObject(
        IBrowserService* iface,
        IOleObject **ppobjv)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    FIXME("%p %p\n", This, ppobjv);
    return E_NOTIMPL;
}

static HRESULT WINAPI BrowserService_GetTravelLog(
        IBrowserService* iface,
        ITravelLog **pptl)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    FIXME("%p %p\n", This, pptl);
    return E_NOTIMPL;
}

static HRESULT WINAPI BrowserService_ShowControlWindow(
        IBrowserService* iface,
        UINT id,
        BOOL fShow)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    FIXME("%p %u %d\n", This, id, fShow);
    return E_NOTIMPL;
}

static HRESULT WINAPI BrowserService_IsControlWindowShown(
        IBrowserService* iface,
        UINT id,
        BOOL *pfShown)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    FIXME("%p %u %p\n", This, id, pfShown);
    return E_NOTIMPL;
}

static HRESULT WINAPI BrowserService_IEGetDisplayName(
        IBrowserService* iface,
        PCIDLIST_ABSOLUTE pidl,
        LPWSTR pwszName,
        UINT uFlags)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    FIXME("%p %p %p %u\n", This, pidl, pwszName, uFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI BrowserService_IEParseDisplayName(
        IBrowserService* iface,
        UINT uiCP,
        LPCWSTR pwszPath,
        PIDLIST_ABSOLUTE *ppidlOut)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    FIXME("%p %u %s %p\n", This, uiCP, debugstr_w(pwszPath), ppidlOut);
    return E_NOTIMPL;
}

static HRESULT WINAPI BrowserService_DisplayParseError(
        IBrowserService* iface,
        HRESULT hres,
        LPCWSTR pwszPath)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    FIXME("%p %lx %s\n", This, hres, debugstr_w(pwszPath));
    return E_NOTIMPL;
}

static HRESULT WINAPI BrowserService_NavigateToPidl(
        IBrowserService* iface,
        PCIDLIST_ABSOLUTE pidl,
        DWORD grfHLNF)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    FIXME("%p %p %ld\n", This, pidl, grfHLNF);
    return E_NOTIMPL;
}

static HRESULT WINAPI BrowserService_SetNavigateState(
        IBrowserService* iface,
        BNSTATE bnstate)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    FIXME("%p %d\n", This, bnstate);
    return E_NOTIMPL;
}

static HRESULT WINAPI BrowserService_GetNavigateState(
        IBrowserService* iface,
        BNSTATE *pbnstate)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    FIXME("%p %p\n", This, pbnstate);
    return E_NOTIMPL;
}

static HRESULT WINAPI BrowserService_NotifyRedirect(
        IBrowserService* iface,
        IShellView *psv,
        PCIDLIST_ABSOLUTE pidl,
        BOOL *pfDidBrowse)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    FIXME("%p %p %p %p\n", This, psv, pidl, pfDidBrowse);
    return E_NOTIMPL;
}

static HRESULT WINAPI BrowserService_UpdateWindowList(
        IBrowserService* iface)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI BrowserService_UpdateBackForwardState(
        IBrowserService* iface)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI BrowserService_SetFlags(
        IBrowserService* iface,
        DWORD dwFlags,
        DWORD dwFlagMask)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    FIXME("%p %lx %lx\n", This, dwFlags, dwFlagMask);
    return E_NOTIMPL;
}

static HRESULT WINAPI BrowserService_GetFlags(
        IBrowserService* iface,
        DWORD *pdwFlags)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    FIXME("%p %p\n", This, pdwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI BrowserService_CanNavigateNow(
        IBrowserService* iface)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI BrowserService_GetPidl(
        IBrowserService* iface,
        PIDLIST_ABSOLUTE *ppidl)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    FIXME("%p %p\n", This, ppidl);
    return E_NOTIMPL;
}

static HRESULT WINAPI BrowserService_SetReferrer(
        IBrowserService* iface,
        PCIDLIST_ABSOLUTE pidl)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    FIXME("%p %p\n", This, pidl);
    return E_NOTIMPL;
}

static DWORD WINAPI BrowserService_GetBrowserIndex(
        IBrowserService* iface)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI BrowserService_GetBrowserByIndex(
        IBrowserService* iface,
        DWORD dwID,
        IUnknown **ppunk)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    FIXME("%p %lx %p\n", This, dwID, ppunk);
    return E_NOTIMPL;
}

static HRESULT WINAPI BrowserService_GetHistoryObject(
        IBrowserService* iface,
        IOleObject **ppole,
        IStream **pstm,
        IBindCtx **ppbc)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    FIXME("%p %p %p %p\n", This, ppole, pstm, ppbc);
    return E_NOTIMPL;
}

static HRESULT WINAPI BrowserService_SetHistoryObject(
        IBrowserService* iface,
        IOleObject *pole,
        BOOL fIsLocalAnchor)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    FIXME("%p %p %d\n", This, pole, fIsLocalAnchor);
    return E_NOTIMPL;
}

static HRESULT WINAPI BrowserService_CacheOLEServer(
        IBrowserService* iface,
        IOleObject *pole)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    FIXME("%p %p\n", This, pole);
    return E_NOTIMPL;
}

static HRESULT WINAPI BrowserService_GetSetCodePage(
        IBrowserService* iface,
        VARIANT *pvarIn,
        VARIANT *pvarOut)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    FIXME("%p %s %p\n", This, debugstr_variant(pvarIn), pvarOut);
    return E_NOTIMPL;
}

static HRESULT WINAPI BrowserService_OnHttpEquiv(
        IBrowserService* iface,
        IShellView *psv,
        BOOL fDone,
        VARIANT *pvarargIn,
        VARIANT *pvarargOut)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    FIXME("%p %p %d %s %p\n", This, psv, fDone, debugstr_variant(pvarargIn), pvarargOut);
    return E_NOTIMPL;
}

static HRESULT WINAPI BrowserService_GetPalette(
        IBrowserService* iface,
        HPALETTE *hpal)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    FIXME("%p %p\n", This, hpal);
    return E_NOTIMPL;
}

static HRESULT WINAPI BrowserService_RegisterWindow(
        IBrowserService* iface,
        BOOL fForceRegister,
        int swc)
{
    ShellBrowser *This = impl_from_IBrowserService(iface);
    FIXME("%p %d %d\n", This, fForceRegister, swc);
    return E_NOTIMPL;
}

static const IBrowserServiceVtbl BrowserServiceVtbl = {
    BrowserService_QueryInterface,
    BrowserService_AddRef,
    BrowserService_Release,
    BrowserService_GetParentSite,
    BrowserService_SetTitle,
    BrowserService_GetTitle,
    BrowserService_GetOleObject,
    BrowserService_GetTravelLog,
    BrowserService_ShowControlWindow,
    BrowserService_IsControlWindowShown,
    BrowserService_IEGetDisplayName,
    BrowserService_IEParseDisplayName,
    BrowserService_DisplayParseError,
    BrowserService_NavigateToPidl,
    BrowserService_SetNavigateState,
    BrowserService_GetNavigateState,
    BrowserService_NotifyRedirect,
    BrowserService_UpdateWindowList,
    BrowserService_UpdateBackForwardState,
    BrowserService_SetFlags,
    BrowserService_GetFlags,
    BrowserService_CanNavigateNow,
    BrowserService_GetPidl,
    BrowserService_SetReferrer,
    BrowserService_GetBrowserIndex,
    BrowserService_GetBrowserByIndex,
    BrowserService_GetHistoryObject,
    BrowserService_SetHistoryObject,
    BrowserService_CacheOLEServer,
    BrowserService_GetSetCodePage,
    BrowserService_OnHttpEquiv,
    BrowserService_GetPalette,
    BrowserService_RegisterWindow
};

static inline ShellBrowser *impl_from_IDocObjectService(IDocObjectService *iface)
{
    return CONTAINING_RECORD(iface, ShellBrowser, IDocObjectService_iface);
}

static HRESULT WINAPI DocObjectService_QueryInterface(
        IDocObjectService* iface,
        REFIID riid,
        void **ppvObject)
{
    ShellBrowser *This = impl_from_IDocObjectService(iface);
    return IShellBrowser_QueryInterface(&This->IShellBrowser_iface, riid, ppvObject);
}

static ULONG WINAPI DocObjectService_AddRef(
        IDocObjectService* iface)
{
    ShellBrowser *This = impl_from_IDocObjectService(iface);
    return IShellBrowser_AddRef(&This->IShellBrowser_iface);
}

static ULONG WINAPI DocObjectService_Release(
        IDocObjectService* iface)
{
    ShellBrowser *This = impl_from_IDocObjectService(iface);
    return IShellBrowser_Release(&This->IShellBrowser_iface);
}

static HRESULT WINAPI DocObjectService_FireBeforeNavigate2(
        IDocObjectService* iface,
        IDispatch *pDispatch,
        LPCWSTR lpszUrl,
        DWORD dwFlags,
        LPCWSTR lpszFrameName,
        BYTE *pPostData,
        DWORD cbPostData,
        LPCWSTR lpszHeaders,
        BOOL fPlayNavSound,
        BOOL *pfCancel)
{
    ShellBrowser *This = impl_from_IDocObjectService(iface);
    VARIANT var_url, var_flags, var_frame_name, var_post_data, var_post_data2, var_headers;
    VARIANTARG params[7];
    DISPPARAMS dp = {params, NULL, 7, 0};
    VARIANT_BOOL cancel = VARIANT_FALSE;
    SAFEARRAY *post_data;
    WCHAR file_path[MAX_PATH];
    DWORD file_path_len = ARRAY_SIZE(file_path);

    TRACE("%p %p %s %lx %s %p %ld %s %d %p\n", This, pDispatch, debugstr_w(lpszUrl),
            dwFlags, debugstr_w(lpszFrameName), pPostData, cbPostData,
            debugstr_w(lpszHeaders), fPlayNavSound, pfCancel);

    if(cbPostData) {
        post_data = SafeArrayCreateVector(VT_UI1, 0, cbPostData);
        if(!post_data)
            return E_OUTOFMEMORY;
        memcpy(post_data->pvData, pPostData, cbPostData);
    }else {
        post_data = NULL;
    }

    V_VT(params) = VT_BOOL|VT_BYREF;
    V_BOOLREF(params) = &cancel;

    V_VT(params+1) = (VT_BYREF|VT_VARIANT);
    V_VARIANTREF(params+1) = &var_headers;
    V_VT(&var_headers) = VT_BSTR;
    V_BSTR(&var_headers) = lpszHeaders ? SysAllocString(lpszHeaders) : NULL;

    V_VT(params+2) = (VT_BYREF|VT_VARIANT);
    V_VARIANTREF(params+2) = &var_post_data2;
    V_VT(&var_post_data2) = (VT_BYREF|VT_VARIANT);
    V_VARIANTREF(&var_post_data2) = &var_post_data;

    if(post_data) {
        V_VT(&var_post_data) = VT_UI1|VT_ARRAY;
        V_ARRAY(&var_post_data) = post_data;
    }else {
        V_VT(&var_post_data) = VT_EMPTY;
    }

    V_VT(params+3) = (VT_BYREF|VT_VARIANT);
    V_VARIANTREF(params+3) = &var_frame_name;
    V_VT(&var_frame_name) = VT_BSTR;
    V_BSTR(&var_frame_name) = lpszFrameName ? SysAllocString(lpszFrameName) : NULL;

    V_VT(params+4) = (VT_BYREF|VT_VARIANT);
    V_VARIANTREF(params+4) = &var_flags;
    V_VT(&var_flags) = VT_I4;
    V_I4(&var_flags) = 0;

    V_VT(params+5) = (VT_BYREF|VT_VARIANT);
    V_VARIANTREF(params+5) = &var_url;
    V_VT(&var_url) = VT_BSTR;
    if(PathCreateFromUrlW(lpszUrl, file_path, &file_path_len, 0) == S_OK)
        V_BSTR(&var_url) = SysAllocString(file_path);
    else
        V_BSTR(&var_url) = SysAllocString(lpszUrl);

    V_VT(params+6) = (VT_DISPATCH);
    V_DISPATCH(params+6) = (IDispatch*)This->doc_host->wb;

    /* Keep reference to This. It may be released in event handler. */
    IShellBrowser_AddRef(&This->IShellBrowser_iface);

    TRACE(">>>\n");
    call_sink(This->doc_host->cps.wbe2, DISPID_BEFORENAVIGATE2, &dp);
    TRACE("<<<\n");

    IShellBrowser_Release(&This->IShellBrowser_iface);

    SysFreeString(V_BSTR(&var_url));
    SysFreeString(V_BSTR(&var_headers));
    SysFreeString(V_BSTR(&var_frame_name));
    SafeArrayDestroy(post_data);

    *pfCancel = !!cancel;
    return S_OK;
}

static HRESULT WINAPI DocObjectService_FireNavigateComplete2(
        IDocObjectService* iface,
        IHTMLWindow2 *pHTMLWindow2,
        DWORD dwFlags)
{
    ShellBrowser *This = impl_from_IDocObjectService(iface);
    DocHost *doc_host = This->doc_host;
    IHTMLPrivateWindow *priv_window;
    const WCHAR *orig_url;
    VARIANTARG params[2];
    DISPPARAMS dp = {params, NULL, 2, 0};
    VARIANT url_var;
    BSTR url;
    HRESULT hres;

    TRACE("%p %p %lx\n", This, pHTMLWindow2, dwFlags);

    update_navigation_commands(This->doc_host);

    if(doc_host->travellog.loading_pos != -1) {
        WARN("histupdate not notified\n");
        doc_host->travellog.position = doc_host->travellog.loading_pos;
        doc_host->travellog.loading_pos = -1;
    }

    hres = IHTMLWindow2_QueryInterface(pHTMLWindow2, &IID_IHTMLPrivateWindow, (void**)&priv_window);
    if(FAILED(hres))
        return hres;

    hres = IHTMLPrivateWindow_GetAddressBarUrl(priv_window, &url);
    IHTMLPrivateWindow_Release(priv_window);
    if(FAILED(hres))
        return hres;

    TRACE("got URL %s\n", debugstr_w(url));
    set_dochost_url(This->doc_host, url);

    orig_url = error_url_frag(url);
    if(orig_url) {
        BSTR tmp = SysAllocString(orig_url);
        SysFreeString(url);
        url = tmp;
    }

    V_VT(params) = (VT_BYREF|VT_VARIANT);
    V_VARIANTREF(params) = &url_var;

    V_VT(params+1) = VT_DISPATCH;
    V_DISPATCH(params+1) = (IDispatch*)doc_host->wb;

    V_VT(&url_var) = VT_BSTR;
    V_BSTR(&url_var) = url;

    /* Keep reference to This. It may be released in event handler. */
    IShellBrowser_AddRef(&This->IShellBrowser_iface);

    TRACE(">>>\n");
    call_sink(This->doc_host->cps.wbe2, DISPID_NAVIGATECOMPLETE2, &dp);
    TRACE("<<<\n");

    SysFreeString(url);

    IShellBrowser_Release(&This->IShellBrowser_iface);
    return S_OK;
}

static HRESULT WINAPI DocObjectService_FireDownloadBegin(
        IDocObjectService* iface)
{
    ShellBrowser *This = impl_from_IDocObjectService(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjectService_FireDownloadComplete(
        IDocObjectService* iface)
{
    ShellBrowser *This = impl_from_IDocObjectService(iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjectService_FireDocumentComplete(
        IDocObjectService* iface,
        IHTMLWindow2 *pHTMLWindow,
        DWORD dwFlags)
{
    ShellBrowser *This = impl_from_IDocObjectService(iface);
    IHTMLPrivateWindow *priv_window;
    const WCHAR *orig_url;
    VARIANTARG params[2];
    DISPPARAMS dp = {params, NULL, 2, 0};
    VARIANT url_var;
    BSTR url;
    HRESULT hres;

    TRACE("%p %p %lx\n", This, pHTMLWindow, dwFlags);

    hres = IHTMLWindow2_QueryInterface(pHTMLWindow, &IID_IHTMLPrivateWindow, (void**)&priv_window);
    if(FAILED(hres))
        return hres;

    hres = IHTMLPrivateWindow_GetAddressBarUrl(priv_window, &url);
    IHTMLPrivateWindow_Release(priv_window);
    if(FAILED(hres))
        return hres;

    TRACE("got URL %s\n", debugstr_w(url));

    orig_url = error_url_frag(url);
    if(orig_url) {
        BSTR tmp = SysAllocString(orig_url);
        SysFreeString(url);
        url = tmp;
    }

    V_VT(params) = (VT_BYREF|VT_VARIANT);
    V_VARIANTREF(params) = &url_var;

    V_VT(params+1) = VT_DISPATCH;
    V_DISPATCH(params+1) = (IDispatch*)This->doc_host->wb;

    V_VT(&url_var) = VT_BSTR;
    V_BSTR(&url_var) = url;

    /* Keep reference to This. It may be released in event handler. */
    IShellBrowser_AddRef(&This->IShellBrowser_iface);

    TRACE(">>>\n");
    call_sink(This->doc_host->cps.wbe2, DISPID_DOCUMENTCOMPLETE, &dp);
    TRACE("<<<\n");

    SysFreeString(url);

    IShellBrowser_Release(&This->IShellBrowser_iface);
    return S_OK;
}

static HRESULT WINAPI DocObjectService_UpdateDesktopComponent(
        IDocObjectService* iface,
        IHTMLWindow2 *pHTMLWindow)
{
    ShellBrowser *This = impl_from_IDocObjectService(iface);
    FIXME("%p %p\n", This, pHTMLWindow);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjectService_GetPendingUrl(
        IDocObjectService* iface,
        BSTR *pbstrPendingUrl)
{
    ShellBrowser *This = impl_from_IDocObjectService(iface);
    FIXME("%p %p\n", This, pbstrPendingUrl);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjectService_ActiveElementChanged(
        IDocObjectService* iface,
        IHTMLElement *pHTMLElement)
{
    ShellBrowser *This = impl_from_IDocObjectService(iface);
    FIXME("%p %p\n", This, pHTMLElement);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjectService_GetUrlSearchComponent(
        IDocObjectService* iface,
        BSTR *pbstrSearch)
{
    ShellBrowser *This = impl_from_IDocObjectService(iface);
    FIXME("%p %p\n", This, pbstrSearch);
    return E_NOTIMPL;
}

static HRESULT WINAPI DocObjectService_IsErrorUrl(
        IDocObjectService* iface,
        LPCWSTR lpszUrl,
        BOOL *pfIsError)
{
    ShellBrowser *This = impl_from_IDocObjectService(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(lpszUrl), pfIsError);

    *pfIsError = !!error_url_frag(lpszUrl);
    return S_OK;
}

static const IDocObjectServiceVtbl DocObjectServiceVtbl = {
    DocObjectService_QueryInterface,
    DocObjectService_AddRef,
    DocObjectService_Release,
    DocObjectService_FireBeforeNavigate2,
    DocObjectService_FireNavigateComplete2,
    DocObjectService_FireDownloadBegin,
    DocObjectService_FireDownloadComplete,
    DocObjectService_FireDocumentComplete,
    DocObjectService_UpdateDesktopComponent,
    DocObjectService_GetPendingUrl,
    DocObjectService_ActiveElementChanged,
    DocObjectService_GetUrlSearchComponent,
    DocObjectService_IsErrorUrl
};

HRESULT create_browser_service(DocHost *doc_host, ShellBrowser **ret)
{
    ShellBrowser *sb;

    sb = malloc(sizeof(ShellBrowser));
    if(!sb)
        return E_OUTOFMEMORY;

    sb->IShellBrowser_iface.lpVtbl = &ShellBrowserVtbl;
    sb->IBrowserService_iface.lpVtbl = &BrowserServiceVtbl;
    sb->IDocObjectService_iface.lpVtbl = &DocObjectServiceVtbl;

    sb->ref = 1;
    sb->doc_host = doc_host;

    *ret = sb;
    return S_OK;
}

void detach_browser_service(ShellBrowser *sb)
{
    sb->doc_host = NULL;
    IShellBrowser_Release(&sb->IShellBrowser_iface);
}

static inline NewWindowManager *impl_from_INewWindowManager(INewWindowManager *iface)
{
    return CONTAINING_RECORD(iface, NewWindowManager, INewWindowManager_iface);
}

static HRESULT WINAPI NewWindowManager_QueryInterface(INewWindowManager *iface, REFIID riid, void **ppv)
{
    NewWindowManager *This = impl_from_INewWindowManager(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->INewWindowManager_iface;
    }else if(IsEqualGUID(&IID_INewWindowManager, riid)) {
        TRACE("(%p)->(IID_INewWindowManager %p)\n", This, ppv);
        *ppv = &This->INewWindowManager_iface;
    }else {
        WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI NewWindowManager_AddRef(INewWindowManager *iface)
{
    NewWindowManager *This = impl_from_INewWindowManager(iface);

    TRACE("(%p)\n", This);

    return IOleClientSite_AddRef(&This->doc_host->IOleClientSite_iface);
}

static ULONG WINAPI NewWindowManager_Release(INewWindowManager *iface)
{
    NewWindowManager *This = impl_from_INewWindowManager(iface);

    TRACE("(%p)\n", This);

    return IOleClientSite_Release(&This->doc_host->IOleClientSite_iface);
}

static HRESULT WINAPI NewWindowManager_EvaluateNewWindow(INewWindowManager *iface, LPCWSTR pszUrl,
        LPCWSTR pszName, LPCWSTR pszUrlContext, LPCWSTR pszFeatures, BOOL fReplace, DWORD dwFlags,
        DWORD dwUserActionTime)
{
    NewWindowManager *This = impl_from_INewWindowManager(iface);
    FIXME("(%p)->(%s %s %s %s %x %lx %ld)\n", This, debugstr_w(pszUrl), debugstr_w(pszName), debugstr_w(pszUrlContext),
          debugstr_w(pszFeatures), fReplace, dwFlags, dwUserActionTime);
    return S_OK;
}

static const INewWindowManagerVtbl NewWindowManagerVtbl = {
    NewWindowManager_QueryInterface,
    NewWindowManager_AddRef,
    NewWindowManager_Release,
    NewWindowManager_EvaluateNewWindow
};

void NewWindowManager_Init(DocHost *doc_host)
{
    doc_host->nwm.INewWindowManager_iface.lpVtbl = &NewWindowManagerVtbl;
    doc_host->nwm.doc_host = doc_host;
}
