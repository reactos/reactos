/*
 * Copyright 2012 Austin English
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

#include "wmvcore.h"

#include "initguid.h"
#include "wmsdk.h"
#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(wmvcore);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
    }

    return TRUE;
}

HRESULT WINAPI DllRegisterServer(void)
{
    FIXME("(): stub\n");

    return S_OK;
}

HRESULT WINAPI WMCheckURLExtension(const WCHAR *url)
{
    FIXME("(%s): stub\n", wine_dbgstr_w(url));

    if (!url)
        return E_INVALIDARG;

    return NS_E_INVALID_NAME;
}

HRESULT WINAPI WMCheckURLScheme(const WCHAR *scheme)
{
    FIXME("(%s): stub\n", wine_dbgstr_w(scheme));

    return NS_E_INVALID_NAME;
}

HRESULT WINAPI WMCreateEditor(IWMMetadataEditor **editor)
{
    FIXME("(%p): stub\n", editor);

    *editor = NULL;

    return E_NOTIMPL;
}

HRESULT WINAPI WMCreateBackupRestorer(IUnknown *callback, IWMLicenseBackup **licBackup)
{
    FIXME("(%p %p): stub\n", callback, licBackup);

    if (!callback)
        return E_INVALIDARG;

    *licBackup = NULL;

    return E_NOTIMPL;
}

typedef struct {
    IWMProfileManager2 IWMProfileManager2_iface;
    LONG ref;
} WMProfileManager;

static inline WMProfileManager *impl_from_IWMProfileManager2(IWMProfileManager2 *iface)
{
    return CONTAINING_RECORD(iface, WMProfileManager, IWMProfileManager2_iface);
}

static HRESULT WINAPI WMProfileManager_QueryInterface(IWMProfileManager2 *iface, REFIID riid, void **ppv)
{
    WMProfileManager *This = impl_from_IWMProfileManager2(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IWMProfileManager2_iface;
    }else if(IsEqualGUID(&IID_IWMProfileManager, riid)) {
        TRACE("(%p)->(IID_IWMProfileManager %p)\n", This, ppv);
        *ppv = &This->IWMProfileManager2_iface;
    }else if(IsEqualGUID(&IID_IWMProfileManager2, riid)) {
        TRACE("(%p)->(IID_IWMProfileManager2 %p)\n", This, ppv);
        *ppv = &This->IWMProfileManager2_iface;
    }else {
        FIXME("Unsupported iface %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI WMProfileManager_AddRef(IWMProfileManager2 *iface)
{
    WMProfileManager *This = impl_from_IWMProfileManager2(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI WMProfileManager_Release(IWMProfileManager2 *iface)
{
    WMProfileManager *This = impl_from_IWMProfileManager2(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI WMProfileManager_CreateEmptyProfile(IWMProfileManager2 *iface, WMT_VERSION version, IWMProfile **ret)
{
    WMProfileManager *This = impl_from_IWMProfileManager2(iface);
    FIXME("(%p)->(%x %p)\n", This, version, ret);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfileManager_LoadProfileByID(IWMProfileManager2 *iface, REFGUID guid, IWMProfile **ret)
{
    WMProfileManager *This = impl_from_IWMProfileManager2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_guid(guid), ret);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfileManager_LoadProfileByData(IWMProfileManager2 *iface, const WCHAR *profile, IWMProfile **ret)
{
    WMProfileManager *This = impl_from_IWMProfileManager2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_w(profile), ret);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfileManager_SaveProfile(IWMProfileManager2 *iface, IWMProfile *profile, WCHAR *profile_str, DWORD *len)
{
    WMProfileManager *This = impl_from_IWMProfileManager2(iface);
    FIXME("(%p)->(%p %p %p)\n", This, profile, profile_str, len);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfileManager_GetSystemProfileCount(IWMProfileManager2 *iface, DWORD *ret)
{
    WMProfileManager *This = impl_from_IWMProfileManager2(iface);
    FIXME("(%p)->(%p)\n", This, ret);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfileManager_LoadSystemProfile(IWMProfileManager2 *iface, DWORD index, IWMProfile **ret)
{
    WMProfileManager *This = impl_from_IWMProfileManager2(iface);
    FIXME("(%p)->(%d %p)\n", This, index, ret);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfileManager2_GetSystemProfileVersion(IWMProfileManager2 *iface, WMT_VERSION *version)
{
    WMProfileManager *This = impl_from_IWMProfileManager2(iface);
    FIXME("(%p)->(%p)\n", This, version);
    return E_NOTIMPL;
}

static HRESULT WINAPI WMProfileManager2_SetSystemProfileVersion(IWMProfileManager2 *iface, WMT_VERSION version)
{
    WMProfileManager *This = impl_from_IWMProfileManager2(iface);
    FIXME("(%p)->(%x)\n", This, version);
    return E_NOTIMPL;
}

static const IWMProfileManager2Vtbl WMProfileManager2Vtbl = {
    WMProfileManager_QueryInterface,
    WMProfileManager_AddRef,
    WMProfileManager_Release,
    WMProfileManager_CreateEmptyProfile,
    WMProfileManager_LoadProfileByID,
    WMProfileManager_LoadProfileByData,
    WMProfileManager_SaveProfile,
    WMProfileManager_GetSystemProfileCount,
    WMProfileManager_LoadSystemProfile,
    WMProfileManager2_GetSystemProfileVersion,
    WMProfileManager2_SetSystemProfileVersion
};

HRESULT WINAPI WMCreateProfileManager(IWMProfileManager **ret)
{
    WMProfileManager *profile_mgr;

    TRACE("(%p)\n", ret);

    profile_mgr = heap_alloc(sizeof(*profile_mgr));
    if(!profile_mgr)
        return E_OUTOFMEMORY;

    profile_mgr->IWMProfileManager2_iface.lpVtbl = &WMProfileManager2Vtbl;
    profile_mgr->ref = 1;

    *ret = (IWMProfileManager *)&profile_mgr->IWMProfileManager2_iface;
    return S_OK;
}
