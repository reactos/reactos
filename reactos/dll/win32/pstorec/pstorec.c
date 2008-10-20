/*
 * Protected Storage (pstores)
 *
 * Copyright 2004 Mike McCormack for CodeWeavers
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

#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "pstore.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(pstores);

typedef struct
{
    const IPStoreVtbl *lpVtbl;
    LONG ref;
} PStore_impl;

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("%p %x %p\n", hinst, fdwReason, fImpLoad);

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinst);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

/**************************************************************************
 *  IPStore->QueryInterface
 */
static HRESULT WINAPI PStore_fnQueryInterface(
        IPStore* iface,
        REFIID riid,
        LPVOID *ppvObj)
{
    PStore_impl *This = (PStore_impl *)iface;

    TRACE("%p %s\n",This,debugstr_guid(riid));

    *ppvObj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = This;
    }

    if (*ppvObj)
    {
        IUnknown_AddRef((IUnknown*)(*ppvObj));
        return S_OK;
    }
    TRACE("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

/******************************************************************************
 * IPStore->AddRef
 */
static ULONG WINAPI PStore_fnAddRef(IPStore* iface)
{
    PStore_impl *This = (PStore_impl *)iface;

    TRACE("%p %u\n", This, This->ref);

    return InterlockedIncrement( &This->ref );
}

/******************************************************************************
 * IPStore->Release
 */
static ULONG WINAPI PStore_fnRelease(IPStore* iface)
{
    PStore_impl *This = (PStore_impl *)iface;
    LONG ref;

    TRACE("%p %u\n", This, This->ref);

    ref = InterlockedDecrement( &This->ref );
    if( !ref )
        HeapFree( GetProcessHeap(), 0, This );

    return ref;
}

/******************************************************************************
 * IPStore->GetInfo
 */
static HRESULT WINAPI PStore_fnGetInfo( IPStore* iface, PPST_PROVIDERINFO* ppProperties)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/******************************************************************************
 * IPStore->GetProvParam
 */
static HRESULT WINAPI PStore_fnGetProvParam( IPStore* iface,
    DWORD dwParam, DWORD* pcbData, BYTE** ppbData, DWORD dwFlags)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/******************************************************************************
 * IPStore->SetProvParam
 */
static HRESULT WINAPI PStore_fnSetProvParam( IPStore* This,
    DWORD dwParam, DWORD cbData, BYTE* pbData, DWORD* dwFlags)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/******************************************************************************
 * IPStore->CreateType
 */
static HRESULT WINAPI PStore_fnCreateType( IPStore* This,
    PST_KEY Key, const GUID* pType, PPST_TYPEINFO pInfo, DWORD dwFlags)
{
    FIXME("%p %08x %s %p(%d,%s) %08x\n", This, Key, debugstr_guid(pType),
          pInfo, pInfo->cbSize, debugstr_w(pInfo->szDisplayName), dwFlags);

    return E_NOTIMPL;
}

/******************************************************************************
 * IPStore->GetTypeInfo
 */
static HRESULT WINAPI PStore_fnGetTypeInfo( IPStore* This,
    PST_KEY Key, const GUID* pType, PPST_TYPEINFO** ppInfo, DWORD dwFlags)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/******************************************************************************
 * IPStore->DeleteType
 */
static HRESULT WINAPI PStore_fnDeleteType( IPStore* This,
    PST_KEY Key, const GUID* pType, DWORD dwFlags)
{
    FIXME("%p %d %s %08x\n", This, Key, debugstr_guid(pType), dwFlags);
    return E_NOTIMPL;
}

/******************************************************************************
 * IPStore->CreateSubtype
 */
static HRESULT WINAPI PStore_fnCreateSubtype( IPStore* This,
    PST_KEY Key, const GUID* pType, const GUID* pSubtype,
    PPST_TYPEINFO pInfo, PPST_ACCESSRULESET pRules, DWORD dwFlags)
{
    FIXME("%p %08x %s %s %p %p %08x\n", This, Key, debugstr_guid(pType),
           debugstr_guid(pSubtype), pInfo, pRules, dwFlags);
    return E_NOTIMPL;
}

/******************************************************************************
 * IPStore->GetSubtypeInfo
 */
static HRESULT WINAPI PStore_fnGetSubtypeInfo( IPStore* This,
    PST_KEY Key, const GUID* pType, const GUID* pSubtype,
    PPST_TYPEINFO** ppInfo, DWORD dwFlags)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/******************************************************************************
 * IPStore->DeleteSubtype
 */
static HRESULT WINAPI PStore_fnDeleteSubtype( IPStore* This,
    PST_KEY Key, const GUID* pType, const GUID* pSubtype, DWORD dwFlags)
{
    FIXME("%p %u %s %s %08x\n", This, Key,
          debugstr_guid(pType), debugstr_guid(pSubtype), dwFlags);
    return E_NOTIMPL;
}

/******************************************************************************
 * IPStore->ReadAccessRuleset
 */
static HRESULT WINAPI PStore_fnReadAccessRuleset( IPStore* This,
    PST_KEY Key, const GUID* pType, const GUID* pSubtype, PPST_TYPEINFO pInfo,
    PPST_ACCESSRULESET** ppRules, DWORD dwFlags)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/******************************************************************************
 * IPStore->WriteAccessRuleSet
 */
static HRESULT WINAPI PStore_fnWriteAccessRuleset( IPStore* This,
    PST_KEY Key, const GUID* pType, const GUID* pSubtype,
    PPST_TYPEINFO pInfo, PPST_ACCESSRULESET pRules, DWORD dwFlags)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/******************************************************************************
 * IPStore->EnumTypes
 */
static HRESULT WINAPI PStore_fnEnumTypes( IPStore* This, PST_KEY Key,
    DWORD dwFlags, IEnumPStoreTypes** ppenum)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/******************************************************************************
 * IPStore->EnumSubtypes
 */
static HRESULT WINAPI PStore_fnEnumSubtypes( IPStore* This, PST_KEY Key,
    const GUID* pType, DWORD dwFlags, IEnumPStoreTypes** ppenum)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/******************************************************************************
 * IPStore->DeleteItem
 */
static HRESULT WINAPI PStore_fnDeleteItem( IPStore* This, PST_KEY Key,
    const GUID* pItemType, const GUID* pItemSubType, LPCWSTR szItemName,
    PPST_PROMPTINFO pPromptInfo, DWORD dwFlags)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/******************************************************************************
 * IPStore->ReadItem
 */
static HRESULT WINAPI PStore_fnReadItem( IPStore* This, PST_KEY Key,
    const GUID* pItemType, const GUID* pItemSubtype, LPCWSTR szItemName,
    DWORD *cbData, BYTE** pbData, PPST_PROMPTINFO pPromptInfo, DWORD dwFlags)
{
    FIXME("%p %08x %s %s %s %p %p %p %08x\n", This, Key,
          debugstr_guid(pItemType), debugstr_guid(pItemSubtype),
          debugstr_w(szItemName), cbData, pbData, pPromptInfo, dwFlags);
    return E_NOTIMPL;
}

/******************************************************************************
 * IPStore->WriteItem
 */
static HRESULT WINAPI PStore_fnWriteItem( IPStore* This, PST_KEY Key,
    const GUID* pItemType, const GUID* pItemSubtype, LPCWSTR szItemName,
    DWORD cbData, BYTE* ppbData, PPST_PROMPTINFO pPromptInfo,
    DWORD dwDefaultConfirmationStyle, DWORD dwFlags)
{
    FIXME("%p %08x %s %s %s %d %p %p %08x\n", This, Key,
          debugstr_guid(pItemType), debugstr_guid(pItemSubtype),
          debugstr_w(szItemName), cbData, ppbData, pPromptInfo, dwFlags);
    return E_NOTIMPL;
}

/******************************************************************************
 * IPStore->OpenItem
 */
static HRESULT WINAPI PStore_fnOpenItem( IPStore* This, PST_KEY Key,
    const GUID* pItemType, const GUID* pItemSubtype, LPCWSTR szItemName,
    PST_ACCESSMODE ModeFlags, PPST_PROMPTINFO pProomptInfo, DWORD dwFlags )
{
    FIXME("%p %08x %s %s %p %08x %p %08x\n", This, Key,
           debugstr_guid(pItemType), debugstr_guid(pItemSubtype),
           debugstr_w(szItemName), ModeFlags, pProomptInfo, dwFlags);
    return E_NOTIMPL;
}

/******************************************************************************
 * IPStore->CloseItem
 */
static HRESULT WINAPI PStore_fnCloseItem( IPStore* This, PST_KEY Key,
    const GUID* pItemType, const GUID* pItemSubtype, LPCWSTR* szItemName,
    DWORD dwFlags)
{
    FIXME("\n");
    return E_NOTIMPL;
}

/******************************************************************************
 * IPStore->EnumItems
 */
static HRESULT WINAPI PStore_fnEnumItems( IPStore* This, PST_KEY Key,
    const GUID* pItemType, const GUID* pItemSubtype, DWORD dwFlags,
    IEnumPStoreItems** ppenum)
{
    FIXME("\n");
    return E_NOTIMPL;
}


static const IPStoreVtbl pstores_vtbl =
{
    PStore_fnQueryInterface,
    PStore_fnAddRef,
    PStore_fnRelease,
    PStore_fnGetInfo,
    PStore_fnGetProvParam,
    PStore_fnSetProvParam,
    PStore_fnCreateType,
    PStore_fnGetTypeInfo,
    PStore_fnDeleteType,
    PStore_fnCreateSubtype,
    PStore_fnGetSubtypeInfo,
    PStore_fnDeleteSubtype,
    PStore_fnReadAccessRuleset,
    PStore_fnWriteAccessRuleset,
    PStore_fnEnumTypes,
    PStore_fnEnumSubtypes,
    PStore_fnDeleteItem,
    PStore_fnReadItem,
    PStore_fnWriteItem,
    PStore_fnOpenItem,
    PStore_fnCloseItem,
    PStore_fnEnumItems
};

HRESULT WINAPI PStoreCreateInstance( IPStore** ppProvider,
            PST_PROVIDERID* pProviderID, void* pReserved, DWORD dwFlags)
{
    PStore_impl *ips;

    TRACE("%p %s %p %08x\n", ppProvider, debugstr_guid(pProviderID), pReserved, dwFlags);

    ips = HeapAlloc( GetProcessHeap(), 0, sizeof (PStore_impl) );
    if( !ips )
        return E_OUTOFMEMORY;

    ips->lpVtbl = &pstores_vtbl;
    ips->ref = 1;

    *ppProvider = (IPStore*) ips;

    return S_OK;
}

HRESULT WINAPI DllRegisterServer(void)
{
    FIXME("\n");
    return S_OK;
}

HRESULT WINAPI DllUnregisterServer(void)
{
    FIXME("\n");
    return S_OK;
}

/***********************************************************************
 *             DllGetClassObject (PSTOREC.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    FIXME("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllCanUnloadNow(void)
{
    FIXME("\n");
    return S_OK;
}
