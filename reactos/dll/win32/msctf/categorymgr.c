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

#include "msctf_internal.h"

typedef struct tagCategoryMgr {
    ITfCategoryMgr ITfCategoryMgr_iface;
    LONG refCount;
} CategoryMgr;

static inline CategoryMgr *impl_from_ITfCategoryMgr(ITfCategoryMgr *iface)
{
    return CONTAINING_RECORD(iface, CategoryMgr, ITfCategoryMgr_iface);
}

static void CategoryMgr_Destructor(CategoryMgr *This)
{
    TRACE("destroying %p\n", This);
    HeapFree(GetProcessHeap(),0,This);
}

static HRESULT WINAPI CategoryMgr_QueryInterface(ITfCategoryMgr *iface, REFIID iid, LPVOID *ppvOut)
{
    CategoryMgr *This = impl_from_ITfCategoryMgr(iface);
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_ITfCategoryMgr))
    {
        *ppvOut = &This->ITfCategoryMgr_iface;
    }

    if (*ppvOut)
    {
        ITfCategoryMgr_AddRef(iface);
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI CategoryMgr_AddRef(ITfCategoryMgr *iface)
{
    CategoryMgr *This = impl_from_ITfCategoryMgr(iface);
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI CategoryMgr_Release(ITfCategoryMgr *iface)
{
    CategoryMgr *This = impl_from_ITfCategoryMgr(iface);
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
    CategoryMgr *This = impl_from_ITfCategoryMgr(iface);

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
    WCHAR fullkey[110];
    WCHAR buf[39];
    WCHAR buf2[39];
    HKEY tipkey;
    CategoryMgr *This = impl_from_ITfCategoryMgr(iface);

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

    sprintfW(fullkey,fmt2,ctg,itm,buf2,buf);
    RegDeleteTreeW(tipkey, fullkey);
    sprintfW(fullkey,fmt2,ctg,itm,buf2,buf);
    RegDeleteTreeW(tipkey, fullkey);

    RegCloseKey(tipkey);
    return S_OK;
}

static HRESULT WINAPI CategoryMgr_EnumCategoriesInItem ( ITfCategoryMgr *iface,
        REFGUID rguid, IEnumGUID **ppEnum)
{
    CategoryMgr *This = impl_from_ITfCategoryMgr(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI CategoryMgr_EnumItemsInCategory ( ITfCategoryMgr *iface,
        REFGUID rcatid, IEnumGUID **ppEnum)
{
    CategoryMgr *This = impl_from_ITfCategoryMgr(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI CategoryMgr_FindClosestCategory ( ITfCategoryMgr *iface,
        REFGUID rguid, GUID *pcatid, const GUID **ppcatidList, ULONG ulCount)
{
    static const WCHAR fmt[] = { '%','s','\\','%','s','\\','C','a','t','e','g','o','r','y','\\','I','t','e','m','\\','%','s',0};

    WCHAR fullkey[120];
    WCHAR buf[39];
    HKEY key;
    HRESULT hr = S_FALSE;
    INT index = 0;
    CategoryMgr *This = impl_from_ITfCategoryMgr(iface);

    TRACE("(%p)\n",This);

    if (!pcatid || (ulCount && ppcatidList == NULL))
        return E_INVALIDARG;

    StringFromGUID2(rguid, buf, 39);
    sprintfW(fullkey,fmt,szwSystemTIPKey,buf,buf);
    *pcatid = GUID_NULL;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,fullkey, 0, KEY_READ, &key ) !=
            ERROR_SUCCESS)
        return S_FALSE;

    while (1)
    {
        HRESULT hr2;
        ULONG res;
        GUID guid;
        WCHAR catid[39];
        DWORD cName;

        cName = 39;
        res = RegEnumKeyExW(key, index, catid, &cName, NULL, NULL, NULL, NULL);
        if (res != ERROR_SUCCESS && res != ERROR_MORE_DATA) break;
        index ++;

        hr2 = CLSIDFromString(catid, &guid);
        if (FAILED(hr2)) continue;

        if (ulCount)
        {
            ULONG j;
            BOOL found = FALSE;
            for (j = 0; j < ulCount; j++)
                if (IsEqualGUID(&guid, ppcatidList[j]))
                {
                    found = TRUE;
                    *pcatid = guid;
                    hr = S_OK;
                    break;
                }
            if (found) break;
        }
        else
        {
            *pcatid = guid;
            hr = S_OK;
            break;
        }
    }

    return hr;
}

static HRESULT WINAPI CategoryMgr_RegisterGUIDDescription (
        ITfCategoryMgr *iface, REFCLSID rclsid, REFGUID rguid,
        const WCHAR *pchDesc, ULONG cch)
{
    CategoryMgr *This = impl_from_ITfCategoryMgr(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI CategoryMgr_UnregisterGUIDDescription (
        ITfCategoryMgr *iface, REFCLSID rclsid, REFGUID rguid)
{
    CategoryMgr *This = impl_from_ITfCategoryMgr(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI CategoryMgr_GetGUIDDescription ( ITfCategoryMgr *iface,
        REFGUID rguid, BSTR *pbstrDesc)
{
    CategoryMgr *This = impl_from_ITfCategoryMgr(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI CategoryMgr_RegisterGUIDDWORD ( ITfCategoryMgr *iface,
        REFCLSID rclsid, REFGUID rguid, DWORD dw)
{
    CategoryMgr *This = impl_from_ITfCategoryMgr(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI CategoryMgr_UnregisterGUIDDWORD ( ITfCategoryMgr *iface,
        REFCLSID rclsid, REFGUID rguid)
{
    CategoryMgr *This = impl_from_ITfCategoryMgr(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI CategoryMgr_GetGUIDDWORD ( ITfCategoryMgr *iface,
        REFGUID rguid, DWORD *pdw)
{
    CategoryMgr *This = impl_from_ITfCategoryMgr(iface);
    FIXME("STUB:(%p)\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI CategoryMgr_RegisterGUID ( ITfCategoryMgr *iface,
        REFGUID rguid, TfGuidAtom *pguidatom
)
{
    DWORD index;
    GUID *checkguid;
    DWORD id;
    CategoryMgr *This = impl_from_ITfCategoryMgr(iface);

    TRACE("(%p) %s %p\n",This,debugstr_guid(rguid),pguidatom);

    if (!pguidatom)
        return E_INVALIDARG;

    index = 0;
    do {
        id = enumerate_Cookie(COOKIE_MAGIC_GUIDATOM,&index);
        if (id && IsEqualGUID(rguid,get_Cookie_data(id)))
        {
            *pguidatom = id;
            return S_OK;
        }
    } while(id);

    checkguid = HeapAlloc(GetProcessHeap(),0,sizeof(GUID));
    *checkguid = *rguid;
    id = generate_Cookie(COOKIE_MAGIC_GUIDATOM,checkguid);

    if (!id)
    {
        HeapFree(GetProcessHeap(),0,checkguid);
        return E_FAIL;
    }

    *pguidatom = id;

    return S_OK;
}

static HRESULT WINAPI CategoryMgr_GetGUID ( ITfCategoryMgr *iface,
        TfGuidAtom guidatom, GUID *pguid)
{
    CategoryMgr *This = impl_from_ITfCategoryMgr(iface);

    TRACE("(%p) %i\n",This,guidatom);

    if (!pguid)
        return E_INVALIDARG;

    *pguid = GUID_NULL;

    if (get_Cookie_magic(guidatom) == COOKIE_MAGIC_GUIDATOM)
        *pguid = *((REFGUID)get_Cookie_data(guidatom));

    return S_OK;
}

static HRESULT WINAPI CategoryMgr_IsEqualTfGuidAtom ( ITfCategoryMgr *iface,
        TfGuidAtom guidatom, REFGUID rguid, BOOL *pfEqual)
{
    CategoryMgr *This = impl_from_ITfCategoryMgr(iface);

    TRACE("(%p) %i %s %p\n",This,guidatom,debugstr_guid(rguid),pfEqual);

    if (!pfEqual)
        return E_INVALIDARG;

    *pfEqual = FALSE;
    if (get_Cookie_magic(guidatom) == COOKIE_MAGIC_GUIDATOM)
    {
        if (IsEqualGUID(rguid,get_Cookie_data(guidatom)))
            *pfEqual = TRUE;
    }

    return S_OK;
}


static const ITfCategoryMgrVtbl CategoryMgrVtbl =
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

    This->ITfCategoryMgr_iface.lpVtbl = &CategoryMgrVtbl;
    This->refCount = 1;

    *ppOut = (IUnknown *)&This->ITfCategoryMgr_iface;
    TRACE("returning %p\n", *ppOut);
    return S_OK;
}
