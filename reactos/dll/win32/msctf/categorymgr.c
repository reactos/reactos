/*
 *  ITfCategoryMgr implementation
 *
 *  Copyright 2009 Aric Stewart, CodeWeavers
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

#include "config.h"

#include <stdarg.h>

#define COBJMACROS

#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "shlwapi.h"
#include "winerror.h"
#include "objbase.h"

#include "wine/unicode.h"

#include "msctf.h"
#include "msctf_internal.h"

WINE_DEFAULT_DEBUG_CHANNEL(msctf);

typedef struct tagCategoryMgr {
    const ITfCategoryMgrVtbl *CategoryMgrVtbl;
    LONG refCount;
} CategoryMgr;

static void CategoryMgr_Destructor(CategoryMgr *This)
{
    TRACE("destroying %p\n", This);
    HeapFree(GetProcessHeap(),0,This);
}

static HRESULT WINAPI CategoryMgr_QueryInterface(ITfCategoryMgr *iface, REFIID iid, LPVOID *ppvOut)
{
    CategoryMgr *This = (CategoryMgr *)iface;
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_ITfCategoryMgr))
    {
        *ppvOut = This;
    }

    if (*ppvOut)
    {
        IUnknown_AddRef(iface);
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI CategoryMgr_AddRef(ITfCategoryMgr *iface)
{
    CategoryMgr *This = (CategoryMgr *)iface;
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI CategoryMgr_Release(ITfCategoryMgr *iface)
{
    CategoryMgr *This = (CategoryMgr *)iface;
    ULONG ret;

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        CategoryMgr_Destructor(This);
    return ret;
}

/*****************************************************
 * ITfCategoryMgr functions
 *****************************************************/

static HRESULT WINAPI CategoryMgr_RegisterCategory ( ITfCategoryMgr *iface,
        REFCLSID rclsid, REFGUID rcatid, REFGUID rguid)
{
    WCHAR fullkey[110];
    WCHAR buf[39];
    WCHAR buf2[39];
    ULONG res;
    HKEY tipkey,catkey,itmkey;
    CategoryMgr *This = (CategoryMgr*)iface;

    static const WCHAR ctg[] = {'C','a','t','e','g','o','r','y',0};
    static const WCHAR itm[] = {'I','t','e','m',0};
    static const WCHAR fmt[] = {'%','s','\\','%','s',0};
    static const WCHAR fmt2[] = {'%','s','\\','%','s','\\','%','s','\\','%','s',0};

    TRACE("(%p) %s %s %s\n",This,debugstr_guid(rclsid), debugstr_guid(rcatid), debugstr_guid(rguid));

    StringFromGUID2(rclsid, buf, 39);
    sprintfW(fullkey,fmt,szwSystemTIPKey,buf);

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,fullkey, 0, KEY_READ | KEY_WRITE,
                &tipkey ) != ERROR_SUCCESS)
        return E_FAIL;

    StringFromGUID2(rcatid, buf, 39);
    StringFromGUID2(rguid, buf2, 39);
    sprintfW(fullkey,fmt2,ctg,ctg,buf,buf2);

    res = RegCreateKeyExW(tipkey, fullkey, 0, NULL, 0, KEY_READ | KEY_WRITE,
            NULL, &catkey, NULL);
    RegCloseKey(catkey);

    if (!res)
    {
        sprintfW(fullkey,fmt2,ctg,itm,buf2,buf);
        res = RegCreateKeyExW(tipkey, fullkey, 0, NULL, 0, KEY_READ | KEY_WRITE,
                NULL, &itmkey, NULL);

        RegCloseKey(itmkey);
    }

    RegCloseKey(tipkey);

    if (!res)
        return S_OK;
    else
        return E_FAIL;
}

static HRESULT WINAPI CategoryMgr_UnregisterCategory ( ITfCategoryMgr *iface,
        REFCLSID rclsid, REFGUID rcatid, REFGUID rguid)
{
    CategoryMgr *This = (CategoryMgr*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI CategoryMgr_EnumCategoriesInItem ( ITfCategoryMgr *iface,
        REFGUID rguid, IEnumGUID **ppEnum)
{
    CategoryMgr *This = (CategoryMgr*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI CategoryMgr_EnumItemsInCategory ( ITfCategoryMgr *iface,
        REFGUID rcatid, IEnumGUID **ppEnum)
{
    CategoryMgr *This = (CategoryMgr*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI CategoryMgr_FindClosestCategory ( ITfCategoryMgr *iface,
        REFGUID rguid, GUID *pcatid, const GUID **ppcatidList, ULONG ulCount)
{
    CategoryMgr *This = (CategoryMgr*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI CategoryMgr_RegisterGUIDDescription (
        ITfCategoryMgr *iface, REFCLSID rclsid, REFGUID rguid,
        const WCHAR *pchDesc, ULONG cch)
{
    CategoryMgr *This = (CategoryMgr*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI CategoryMgr_UnregisterGUIDDescription (
        ITfCategoryMgr *iface, REFCLSID rclsid, REFGUID rguid)
{
    CategoryMgr *This = (CategoryMgr*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI CategoryMgr_GetGUIDDescription ( ITfCategoryMgr *iface,
        REFGUID rguid, BSTR *pbstrDesc)
{
    CategoryMgr *This = (CategoryMgr*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI CategoryMgr_RegisterGUIDDWORD ( ITfCategoryMgr *iface,
        REFCLSID rclsid, REFGUID rguid, DWORD dw)
{
    CategoryMgr *This = (CategoryMgr*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI CategoryMgr_UnregisterGUIDDWORD ( ITfCategoryMgr *iface,
        REFCLSID rclsid, REFGUID rguid)
{
    CategoryMgr *This = (CategoryMgr*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI CategoryMgr_GetGUIDDWORD ( ITfCategoryMgr *iface,
        REFGUID rguid, DWORD *pdw)
{
    CategoryMgr *This = (CategoryMgr*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI CategoryMgr_RegisterGUID ( ITfCategoryMgr *iface,
        REFGUID rguid, TfGuidAtom *pguidatom
)
{
    CategoryMgr *This = (CategoryMgr*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI CategoryMgr_GetGUID ( ITfCategoryMgr *iface,
        TfGuidAtom guidatom, GUID *pguid)
{
    CategoryMgr *This = (CategoryMgr*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI CategoryMgr_IsEqualTfGuidAtom ( ITfCategoryMgr *iface,
        TfGuidAtom guidatom, REFGUID rguid, BOOL *pfEqual)
{
    CategoryMgr *This = (CategoryMgr*)iface;
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}


static const ITfCategoryMgrVtbl CategoryMgr_CategoryMgrVtbl =
{
    CategoryMgr_QueryInterface,
    CategoryMgr_AddRef,
    CategoryMgr_Release,

    CategoryMgr_RegisterCategory,
    CategoryMgr_UnregisterCategory,
    CategoryMgr_EnumCategoriesInItem,
    CategoryMgr_EnumItemsInCategory,
    CategoryMgr_FindClosestCategory,
    CategoryMgr_RegisterGUIDDescription,
    CategoryMgr_UnregisterGUIDDescription,
    CategoryMgr_GetGUIDDescription,
    CategoryMgr_RegisterGUIDDWORD,
    CategoryMgr_UnregisterGUIDDWORD,
    CategoryMgr_GetGUIDDWORD,
    CategoryMgr_RegisterGUID,
    CategoryMgr_GetGUID,
    CategoryMgr_IsEqualTfGuidAtom
};

HRESULT CategoryMgr_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut)
{
    CategoryMgr *This;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    This = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(CategoryMgr));
    if (This == NULL)
        return E_OUTOFMEMORY;

    This->CategoryMgrVtbl= &CategoryMgr_CategoryMgrVtbl;
    This->refCount = 1;

    TRACE("returning %p\n", This);
    *ppOut = (IUnknown *)This;
    return S_OK;
}
