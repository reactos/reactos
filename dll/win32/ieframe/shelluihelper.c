/*
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

#include "ieframe.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ieframe);

struct ShellUIHelper {
    IShellUIHelper2 IShellUIHelper2_iface;
    LONG ref;
};

static inline ShellUIHelper *impl_from_IShellUIHelper2(IShellUIHelper2 *iface)
{
    return CONTAINING_RECORD(iface, ShellUIHelper, IShellUIHelper2_iface);
}

static HRESULT WINAPI ShellUIHelper2_QueryInterface(IShellUIHelper2 *iface, REFIID riid, void **ppv)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IShellUIHelper2_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IShellUIHelper2_iface;
    }else if(IsEqualGUID(&IID_IShellUIHelper, riid)) {
        TRACE("(%p)->(IID_IShellUIHelper %p)\n", This, ppv);
        *ppv = &This->IShellUIHelper2_iface;
    }else if(IsEqualGUID(&IID_IShellUIHelper2, riid)) {
        TRACE("(%p)->(IID_IShellUIHelper2 %p)\n", This, ppv);
        *ppv = &This->IShellUIHelper2_iface;
    }else {
        WARN("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ShellUIHelper2_AddRef(IShellUIHelper2 *iface)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI ShellUIHelper2_Release(IShellUIHelper2 *iface)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref)
        free(This);

    return ref;
}

static HRESULT WINAPI ShellUIHelper2_GetTypeInfoCount(IShellUIHelper2 *iface, UINT *pctinfo)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI ShellUIHelper2_GetTypeInfo(IShellUIHelper2 *iface, UINT iTInfo, LCID lcid, LPTYPEINFO *ppTInfo)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->(%d %ld %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellUIHelper2_GetIDsOfNames(IShellUIHelper2 *iface, REFIID riid, LPOLESTR *rgszNames, UINT cNames,
        LCID lcid, DISPID *rgDispId)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    unsigned i;

    FIXME("(%p)->(%s %p %d %ld %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);
    for(i = 0; i < cNames; i++)
        FIXME("%s\n", debugstr_w(rgszNames[i]));

    return DISP_E_UNKNOWNNAME;
}

static HRESULT WINAPI ShellUIHelper2_Invoke(IShellUIHelper2 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExepInfo, UINT *puArgErr)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->(%ld %s %ld %08x %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellUIHelper2_ResetFirstBootMode(IShellUIHelper2 *iface)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellUIHelper2_ResetSafeMode(IShellUIHelper2 *iface)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellUIHelper2_RefreshOfflineDesktop(IShellUIHelper2 *iface)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellUIHelper2_AddFavourite(IShellUIHelper2 *iface, BSTR URL, VARIANT *Title)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_w(URL), debugstr_variant(Title));
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellUIHelper2_AddChannel(IShellUIHelper2 *iface, BSTR URL)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(URL));
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellUIHelper2_AddDesktopComponent(IShellUIHelper2 *iface, BSTR URL, BSTR Type,
        VARIANT *Left, VARIANT *Top, VARIANT *Width, VARIANT *Height)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->(%s %s %s %s %s %s)\n", This, debugstr_w(URL), debugstr_w(Type), debugstr_variant(Left),
          debugstr_variant(Top), debugstr_variant(Width), debugstr_variant(Height));
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellUIHelper2_IsSubscribed(IShellUIHelper2 *iface, BSTR URL, VARIANT_BOOL *pBool)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(URL), pBool);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellUIHelper2_NavigateAndFind(IShellUIHelper2 *iface, BSTR URL, BSTR strQuery, VARIANT *varTargetFrame)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->(%s %s %s)\n", This, debugstr_w(URL), debugstr_w(strQuery), debugstr_variant(varTargetFrame));
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellUIHelper2_ImportExportFavourites(IShellUIHelper2 *iface, VARIANT_BOOL fImport, BSTR strImpExpPath)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->(%x %s)\n", This, fImport, debugstr_w(strImpExpPath));
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellUIHelper2_AutoCompleteSaveForm(IShellUIHelper2 *iface, VARIANT *Form)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(Form));
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellUIHelper2_AutoScan(IShellUIHelper2 *iface, BSTR strSearch, BSTR strFailureUrl, VARIANT *pvarTargetFrame)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->(%s %s %s)\n", This, debugstr_w(strSearch), debugstr_w(strFailureUrl), debugstr_variant(pvarTargetFrame));
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellUIHelper2_AutoCompleteAttach(IShellUIHelper2 *iface, VARIANT *Reserved)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_variant(Reserved));
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellUIHelper2_ShowBrowserUI(IShellUIHelper2 *iface, BSTR bstrName, VARIANT *pvarIn, VARIANT *pvarOut)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->(%s %s %p)\n", This, debugstr_w(bstrName), debugstr_variant(pvarIn), pvarOut);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellUIHelper2_AddSearchProvider(IShellUIHelper2 *iface, BSTR URL)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(URL));
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellUIHelper2_RunOnceShown(IShellUIHelper2 *iface)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellUIHelper2_SkipRunOnce(IShellUIHelper2 *iface)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellUIHelper2_CustomizeSettings(IShellUIHelper2 *iface, VARIANT_BOOL fSQM,
        VARIANT_BOOL fPhishing, BSTR bstrLocale)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->(%x %x %s)\n", This, fSQM, fPhishing, debugstr_w(bstrLocale));
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellUIHelper2_SqmEnabled(IShellUIHelper2 *iface, VARIANT_BOOL *pfEnabled)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->(%p)\n", This, pfEnabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellUIHelper2_PhishingEnabled(IShellUIHelper2 *iface, VARIANT_BOOL *pfEnabled)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->(%p)\n", This, pfEnabled);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellUIHelper2_BrandImageUri(IShellUIHelper2 *iface, BSTR *pbstrUri)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->(%p)\n", This, pbstrUri);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellUIHelper2_SkipTabsWelcome(IShellUIHelper2 *iface)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellUIHelper2_DiagnoseConnection(IShellUIHelper2 *iface)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellUIHelper2_CustomizeClearType(IShellUIHelper2 *iface, VARIANT_BOOL fSet)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->(%x)\n", This, fSet);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellUIHelper2_IsSearchProviderInstalled(IShellUIHelper2 *iface, BSTR URL, DWORD *pdwResult)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(URL), pdwResult);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellUIHelper2_IsSearchMigrated(IShellUIHelper2 *iface, VARIANT_BOOL *pfMigrated)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->(%p)\n", This, pfMigrated);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellUIHelper2_DefaultSearchProvider(IShellUIHelper2 *iface, BSTR *pbstrName)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->(%p)\n", This, pbstrName);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellUIHelper2_RunOnceRequiredSettingsComplete(IShellUIHelper2 *iface, VARIANT_BOOL fComplete)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->(%x)\n", This, fComplete);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellUIHelper2_RunOnceHasShown(IShellUIHelper2 *iface, VARIANT_BOOL *pfShown)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->(%p)\n", This, pfShown);
    return E_NOTIMPL;
}

static HRESULT WINAPI ShellUIHelper2_SearchGuideUrl(IShellUIHelper2 *iface, BSTR *pbstrUrl)
{
    ShellUIHelper *This = impl_from_IShellUIHelper2(iface);
    FIXME("(%p)->(%p)\n", This, pbstrUrl);
    return E_NOTIMPL;
}

static const IShellUIHelper2Vtbl ShellUIHelper2Vtbl = {
    ShellUIHelper2_QueryInterface,
    ShellUIHelper2_AddRef,
    ShellUIHelper2_Release,
    ShellUIHelper2_GetTypeInfoCount,
    ShellUIHelper2_GetTypeInfo,
    ShellUIHelper2_GetIDsOfNames,
    ShellUIHelper2_Invoke,
    ShellUIHelper2_ResetFirstBootMode,
    ShellUIHelper2_ResetSafeMode,
    ShellUIHelper2_RefreshOfflineDesktop,
    ShellUIHelper2_AddFavourite,
    ShellUIHelper2_AddChannel,
    ShellUIHelper2_AddDesktopComponent,
    ShellUIHelper2_IsSubscribed,
    ShellUIHelper2_NavigateAndFind,
    ShellUIHelper2_ImportExportFavourites,
    ShellUIHelper2_AutoCompleteSaveForm,
    ShellUIHelper2_AutoScan,
    ShellUIHelper2_AutoCompleteAttach,
    ShellUIHelper2_ShowBrowserUI,
    ShellUIHelper2_AddSearchProvider,
    ShellUIHelper2_RunOnceShown,
    ShellUIHelper2_SkipRunOnce,
    ShellUIHelper2_CustomizeSettings,
    ShellUIHelper2_SqmEnabled,
    ShellUIHelper2_PhishingEnabled,
    ShellUIHelper2_BrandImageUri,
    ShellUIHelper2_SkipTabsWelcome,
    ShellUIHelper2_DiagnoseConnection,
    ShellUIHelper2_CustomizeClearType,
    ShellUIHelper2_IsSearchProviderInstalled,
    ShellUIHelper2_IsSearchMigrated,
    ShellUIHelper2_DefaultSearchProvider,
    ShellUIHelper2_RunOnceRequiredSettingsComplete,
    ShellUIHelper2_RunOnceHasShown,
    ShellUIHelper2_SearchGuideUrl
};

HRESULT create_shell_ui_helper(IShellUIHelper2 **_ret)
{
    ShellUIHelper *ret;

    ret = malloc(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IShellUIHelper2_iface.lpVtbl = &ShellUIHelper2Vtbl;
    ret->ref = 1;

    *_ret = &ret->IShellUIHelper2_iface;
    return S_OK;
}
