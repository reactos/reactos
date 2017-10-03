/*
 * Copyright (C) 2003 Michael Günnewig
 * Copyright (C) 2003 CodeWeavers Inc. (Ulrich Czekalla)
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

#include "precomp.h"

#include <winuser.h>
#include <winreg.h>
#include <wine/unicode.h>
#include <dmo.h>

#define MSDMO_MAJOR_VERSION 6

static const WCHAR szDMORootKey[] = 
{
    'D','i','r','e','c','t','S','h','o','w','\\',
    'M','e','d','i','a','O','b','j','e','c','t','s',0
}; 

static const WCHAR szDMOInputType[] =
{
    'I','n','p','u','t','T','y','p','e','s',0
};

static const WCHAR szDMOOutputType[] =
{
    'O','u','t','p','u','t','T','y','p','e','s',0
};

static const WCHAR szDMOKeyed[] =
{
    'K','e','y','e','d',0
};

static const WCHAR szDMOCategories[] =
{
    'C','a','t','e','g','o','r','i','e','s',0
};

static const WCHAR szGUIDFmt[] =
{
    '%','0','8','X','-','%','0','4','X','-','%','0','4','X','-','%','0',
    '2','X','%','0','2','X','-','%','0','2','X','%','0','2','X','%','0','2',
    'X','%','0','2','X','%','0','2','X','%','0','2','X',0
};

static const WCHAR szCat3Fmt[] =
{
    '%','s','\\','%','s','\\','%','s',0
};

static const WCHAR szCat2Fmt[] =
{
    '%','s','\\','%','s',0
};

static const WCHAR szToGuidFmt[] =
{
    '{','%','s','}',0
};


typedef struct
{
    IEnumDMO                    IEnumDMO_iface;
    LONG			ref;
    DWORD			index;
    GUID                        category;
    DWORD                       dwFlags;
    DWORD                       cInTypes;
    DMO_PARTIAL_MEDIATYPE       *pInTypes;
    DWORD                       cOutTypes;
    DMO_PARTIAL_MEDIATYPE       *pOutTypes;
    HKEY                        hkey;
} IEnumDMOImpl;

static inline IEnumDMOImpl *impl_from_IEnumDMO(IEnumDMO *iface)
{
    return CONTAINING_RECORD(iface, IEnumDMOImpl, IEnumDMO_iface);
}

static HRESULT read_types(HKEY root, LPCWSTR key, ULONG *supplied, ULONG requested, DMO_PARTIAL_MEDIATYPE* types);

static const IEnumDMOVtbl edmovt;

static LPWSTR GUIDToString(LPWSTR lpwstr, REFGUID lpcguid)
{
    wsprintfW(lpwstr, szGUIDFmt, lpcguid->Data1, lpcguid->Data2,
        lpcguid->Data3, lpcguid->Data4[0], lpcguid->Data4[1],
        lpcguid->Data4[2], lpcguid->Data4[3], lpcguid->Data4[4],
        lpcguid->Data4[5], lpcguid->Data4[6], lpcguid->Data4[7]);

    return lpwstr;
}

static BOOL IsMediaTypeEqual(const DMO_PARTIAL_MEDIATYPE* mt1, const DMO_PARTIAL_MEDIATYPE* mt2)
{

    return (IsEqualCLSID(&mt1->type, &mt2->type) ||
            IsEqualCLSID(&mt2->type, &GUID_NULL) ||
            IsEqualCLSID(&mt1->type, &GUID_NULL)) &&
            (IsEqualCLSID(&mt1->subtype, &mt2->subtype) ||
            IsEqualCLSID(&mt2->subtype, &GUID_NULL) ||
            IsEqualCLSID(&mt1->subtype, &GUID_NULL));
}

static HRESULT write_types(HKEY hkey, LPCWSTR name, const DMO_PARTIAL_MEDIATYPE* types, DWORD count)
{
    LONG ret;

    if (MSDMO_MAJOR_VERSION > 5)
    {
        ret = RegSetValueExW(hkey, name, 0, REG_BINARY, (const BYTE*) types,
                          count* sizeof(DMO_PARTIAL_MEDIATYPE));
    }
    else
    {
        HKEY skey1,skey2,skey3;
        DWORD index = 0;
        WCHAR szGuidKey[64];

        ret = RegCreateKeyExW(hkey, name, 0, NULL, REG_OPTION_NON_VOLATILE,
                               KEY_WRITE, NULL, &skey1, NULL);
        if (ret)
            return HRESULT_FROM_WIN32(ret);

        while (index < count)
        {
            GUIDToString(szGuidKey,&types[index].type);
            ret = RegCreateKeyExW(skey1, szGuidKey, 0, NULL,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &skey2, NULL);
            GUIDToString(szGuidKey,&types[index].subtype);
            ret = RegCreateKeyExW(skey2, szGuidKey, 0, NULL,
                        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &skey3, NULL);
            RegCloseKey(skey3);
            RegCloseKey(skey2);
            index ++;
        }
        RegCloseKey(skey1);
    }

    return HRESULT_FROM_WIN32(ret);
}

/***************************************************************
 * DMORegister (MSDMO.@)
 *
 * Register a DirectX Media Object.
 */
HRESULT WINAPI DMORegister(
   LPCWSTR szName,
   REFCLSID clsidDMO,
   REFGUID guidCategory,
   DWORD dwFlags,
   DWORD cInTypes,
   const DMO_PARTIAL_MEDIATYPE *pInTypes,
   DWORD cOutTypes,
   const DMO_PARTIAL_MEDIATYPE *pOutTypes
)
{
    WCHAR szguid[64];
    HRESULT hres;
    HKEY hrkey = 0;
    HKEY hkey = 0;
    HKEY hckey = 0;
    HKEY hclskey = 0;
    LONG ret;

    TRACE("%s %s %s\n", debugstr_w(szName), debugstr_guid(clsidDMO), debugstr_guid(guidCategory));

    if (IsEqualGUID(guidCategory, &GUID_NULL))
        return E_INVALIDARG;

    ret = RegCreateKeyExW(HKEY_CLASSES_ROOT, szDMORootKey, 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hrkey, NULL);
    if (ret)
        return HRESULT_FROM_WIN32(ret);

    /* Create clsidDMO key under MediaObjects */ 
    ret = RegCreateKeyExW(hrkey, GUIDToString(szguid, clsidDMO), 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, NULL);
    if (ret)
        goto lend;

    /* Set default Name value */
    ret = RegSetValueExW(hkey, NULL, 0, REG_SZ, (const BYTE*) szName,
        (strlenW(szName) + 1) * sizeof(WCHAR));

    /* Set InputTypes */
    hres = write_types(hkey, szDMOInputType, pInTypes, cInTypes);

    /* Set OutputTypes */
    hres = write_types(hkey, szDMOOutputType, pOutTypes, cOutTypes);

    if (dwFlags & DMO_REGISTERF_IS_KEYED)
    {
        /* Create Keyed key */ 
        ret = RegCreateKeyExW(hkey, szDMOKeyed, 0, NULL,
            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hckey, NULL);
        if (ret)
            goto lend;
        RegCloseKey(hckey);
    }

    /* Register the category */
    ret = RegCreateKeyExW(hrkey, szDMOCategories, 0, NULL,
            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hckey, NULL);
    if (ret)
        goto lend;

    RegCloseKey(hkey);

    ret = RegCreateKeyExW(hckey, GUIDToString(szguid, guidCategory), 0, NULL,
            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, NULL);
    if (ret)
        goto lend;
    ret = RegCreateKeyExW(hkey, GUIDToString(szguid, clsidDMO), 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hclskey, NULL);
    if (ret)
        goto lend;

lend:
    if (hkey)
        RegCloseKey(hkey);
    if (hckey)
        RegCloseKey(hckey);
    if (hclskey)
        RegCloseKey(hclskey);
    if (hrkey)
        RegCloseKey(hrkey);

    hres = HRESULT_FROM_WIN32(ret);
    TRACE(" hresult=0x%08x\n", hres);
    return hres;
}

static HRESULT unregister_dmo_from_category(const WCHAR *dmoW, const WCHAR *catW, HKEY categories)
{
    HKEY catkey;
    LONG ret;

    ret = RegOpenKeyExW(categories, catW, 0, KEY_WRITE, &catkey);
    if (!ret)
    {
        ret = RegDeleteKeyW(catkey, dmoW);
        RegCloseKey(catkey);
    }

    return !ret ? S_OK : S_FALSE;
}

/***************************************************************
 * DMOUnregister (MSDMO.@)
 *
 * Unregister a DirectX Media Object.
 */
HRESULT WINAPI DMOUnregister(REFCLSID dmo, REFGUID category)
{
    HKEY rootkey = 0, categorieskey = 0;
    WCHAR dmoW[64], catW[64];
    HRESULT hr = S_FALSE;
    LONG ret;

    TRACE("%s %s\n", debugstr_guid(dmo), debugstr_guid(category));

    ret = RegOpenKeyExW(HKEY_CLASSES_ROOT, szDMORootKey, 0, KEY_WRITE, &rootkey);
    if (ret)
        return S_FALSE;

    GUIDToString(dmoW, dmo);
    RegDeleteKeyW(rootkey, dmoW);

    /* open 'Categories' */
    ret = RegOpenKeyExW(rootkey, szDMOCategories, 0, KEY_WRITE|KEY_ENUMERATE_SUB_KEYS, &categorieskey);
    RegCloseKey(rootkey);
    if (ret)
    {
        hr = HRESULT_FROM_WIN32(ret);
        goto lend;
    }

    /* remove from all categories */
    if (IsEqualGUID(category, &GUID_NULL))
    {
        DWORD index = 0, len = sizeof(catW)/sizeof(WCHAR);

        while (!RegEnumKeyExW(categorieskey, index++, catW, &len, NULL, NULL, NULL, NULL))
            hr = unregister_dmo_from_category(dmoW, catW, categorieskey);
    }
    else
    {
        GUIDToString(catW, category);
        hr = unregister_dmo_from_category(dmoW, catW, categorieskey);
    }

lend:
    if (categorieskey)
        RegCloseKey(categorieskey);

    return hr;
}


/***************************************************************
 * DMOGetName (MSDMO.@)
 *
 * Get DMO Name from the registry
 */
HRESULT WINAPI DMOGetName(REFCLSID clsidDMO, WCHAR name[])
{
    static const INT max_name_len = 80*sizeof(WCHAR);
    DWORD count = max_name_len;
    WCHAR szguid[64];
    HKEY hrkey, hkey;
    LONG ret;

    TRACE("%s %p\n", debugstr_guid(clsidDMO), name);

    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, szDMORootKey, 0, KEY_READ, &hrkey))
        return E_FAIL;

    ret = RegOpenKeyExW(hrkey, GUIDToString(szguid, clsidDMO), 0, KEY_READ, &hkey);
    RegCloseKey(hrkey);
    if (ret)
        return E_FAIL;

    ret = RegQueryValueExW(hkey, NULL, NULL, NULL, (LPBYTE)name, &count);
    RegCloseKey(hkey);

    if (!ret && count > 1)
    {
        TRACE("name=%s\n", debugstr_w(name));
        return S_OK;
    }

    name[0] = 0;
    return S_FALSE;
}

static HRESULT dup_partial_mediatype(const DMO_PARTIAL_MEDIATYPE *types, DWORD count, DMO_PARTIAL_MEDIATYPE **ret)
{
    *ret = NULL;

    if (count == 0)
        return S_OK;

    *ret = HeapAlloc(GetProcessHeap(), 0, count*sizeof(*types));
    if (!*ret)
        return E_OUTOFMEMORY;

    memcpy(*ret, types, count*sizeof(*types));
    return S_OK;
}

/**************************************************************************
 *  IEnumDMO_Constructor
 */
static HRESULT IEnumDMO_Constructor(
    REFGUID guidCategory,
    DWORD dwFlags,
    DWORD cInTypes,
    const DMO_PARTIAL_MEDIATYPE *pInTypes,
    DWORD cOutTypes,
    const DMO_PARTIAL_MEDIATYPE *pOutTypes,
    IEnumDMO **obj)
{
    IEnumDMOImpl* lpedmo;
    HRESULT hr;
    LONG ret;

    *obj = NULL;

    lpedmo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IEnumDMOImpl));
    if (!lpedmo)
        return E_OUTOFMEMORY;

    lpedmo->IEnumDMO_iface.lpVtbl = &edmovt;
    lpedmo->ref = 1;
    lpedmo->index = -1;
    lpedmo->category = *guidCategory;
    lpedmo->dwFlags = dwFlags;
    lpedmo->cInTypes = cInTypes;
    lpedmo->cOutTypes = cOutTypes;

    hr = dup_partial_mediatype(pInTypes, cInTypes, &lpedmo->pInTypes);
    if (FAILED(hr))
        goto lerr;

    hr = dup_partial_mediatype(pOutTypes, cOutTypes, &lpedmo->pOutTypes);
    if (FAILED(hr))
        goto lerr;

    /* If not filtering by category enum from media objects root */
    if (IsEqualGUID(guidCategory, &GUID_NULL))
    {
        if ((ret = RegOpenKeyExW(HKEY_CLASSES_ROOT, szDMORootKey, 0, KEY_READ, &lpedmo->hkey)))
            hr = HRESULT_FROM_WIN32(ret);
    }
    else
    {
        WCHAR szguid[64];
        WCHAR szKey[MAX_PATH];

        wsprintfW(szKey, szCat3Fmt, szDMORootKey, szDMOCategories, GUIDToString(szguid, guidCategory));
        if ((ret = RegOpenKeyExW(HKEY_CLASSES_ROOT, szKey, 0, KEY_READ, &lpedmo->hkey)))
            hr = HRESULT_FROM_WIN32(ret);
    }

lerr:

    if (FAILED(hr))
        IEnumDMO_Release(&lpedmo->IEnumDMO_iface);
    else
    {
        TRACE("returning %p\n", lpedmo);
        *obj = &lpedmo->IEnumDMO_iface;
    }

    return hr;
}

/******************************************************************************
 * IEnumDMO_fnAddRef
 */
static ULONG WINAPI IEnumDMO_fnAddRef(IEnumDMO * iface)
{
    IEnumDMOImpl *This = impl_from_IEnumDMO(iface);
    ULONG refCount = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, refCount);
    return refCount;
}

/**************************************************************************
 *  EnumDMO_QueryInterface
 */
static HRESULT WINAPI IEnumDMO_fnQueryInterface(IEnumDMO* iface, REFIID riid, void **ppvObj)
{
    IEnumDMOImpl *This = impl_from_IEnumDMO(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObj);

    *ppvObj = NULL;

    if (IsEqualIID(riid, &IID_IEnumDMO) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = iface;
        IEnumDMO_AddRef(iface);
    }

    return *ppvObj ? S_OK : E_NOINTERFACE;
}

/******************************************************************************
 * IEnumDMO_fnRelease
 */
static ULONG WINAPI IEnumDMO_fnRelease(IEnumDMO * iface)
{
    IEnumDMOImpl *This = impl_from_IEnumDMO(iface);
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, refCount);

    if (!refCount)
    {
        if (This->hkey)
            RegCloseKey(This->hkey);
        HeapFree(GetProcessHeap(), 0, This->pInTypes);
        HeapFree(GetProcessHeap(), 0, This->pOutTypes);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return refCount;
}


/******************************************************************************
 * IEnumDMO_fnNext
 */
static HRESULT WINAPI IEnumDMO_fnNext(
    IEnumDMO * iface, 
    DWORD cItemsToFetch,
    CLSID * pCLSID,
    WCHAR ** Names,
    DWORD * pcItemsFetched)
{
    HKEY hkey;
    WCHAR szNextKey[MAX_PATH];
    WCHAR szGuidKey[64];
    WCHAR szKey[MAX_PATH];
    WCHAR szValue[MAX_PATH];
    DWORD len;
    UINT count = 0;
    HRESULT hres = S_OK;
    LONG ret;

    IEnumDMOImpl *This = impl_from_IEnumDMO(iface);

    TRACE("(%p)->(%d %p %p %p)\n", This, cItemsToFetch, pCLSID, Names, pcItemsFetched);

    if (!pCLSID || !Names || !pcItemsFetched)
        return E_POINTER;

    while (count < cItemsToFetch)
    {
        This->index++;

        len = MAX_PATH;
        ret = RegEnumKeyExW(This->hkey, This->index, szNextKey, &len, NULL, NULL, NULL, NULL);
        if (ret != ERROR_SUCCESS)
        {
            hres = HRESULT_FROM_WIN32(ret);
            break;
        }

        TRACE("found %s\n", debugstr_w(szNextKey));

        if (!(This->dwFlags & DMO_ENUMF_INCLUDE_KEYED))
        {
            wsprintfW(szKey, szCat3Fmt, szDMORootKey, szNextKey, szDMOKeyed);
            ret = RegOpenKeyExW(HKEY_CLASSES_ROOT, szKey, 0, KEY_READ, &hkey);
            if (ERROR_SUCCESS == ret)
            {
                RegCloseKey(hkey);
                /* Skip Keyed entries */
                continue;
            }
        }

        wsprintfW(szKey, szCat2Fmt, szDMORootKey, szNextKey);
        ret = RegOpenKeyExW(HKEY_CLASSES_ROOT, szKey, 0, KEY_READ, &hkey);
        TRACE("testing %s\n", debugstr_w(szKey));

        if (This->pInTypes)
        {
            UINT i, j;
            DWORD cInTypes;
            DMO_PARTIAL_MEDIATYPE* pInTypes;

            hres = read_types(hkey, szDMOInputType, &cInTypes,
                    sizeof(szValue)/sizeof(DMO_PARTIAL_MEDIATYPE),
                    (DMO_PARTIAL_MEDIATYPE*)szValue);

            if (FAILED(hres))
            {
                RegCloseKey(hkey);
                continue;
            }

	    pInTypes = (DMO_PARTIAL_MEDIATYPE*) szValue;

            TRACE("read %d intypes for %s:\n", cInTypes, debugstr_w(szKey));
            for (i = 0; i < cInTypes; i++) {
                TRACE("intype %d: type %s, subtype %s\n", i, debugstr_guid(&pInTypes[i].type),
                    debugstr_guid(&pInTypes[i].subtype));
            }

            for (i = 0; i < This->cInTypes; i++)
            {
                for (j = 0; j < cInTypes; j++) 
                {
                    if (IsMediaTypeEqual(&pInTypes[j], &This->pInTypes[i]))
		        break;
                }

		if (j >= cInTypes)
                    break;
            }

            if (i < This->cInTypes)
            {
                RegCloseKey(hkey);
                continue;
            }
        }

        if (This->pOutTypes)
        {
            UINT i, j;
            DWORD cOutTypes;
            DMO_PARTIAL_MEDIATYPE* pOutTypes;

            hres = read_types(hkey, szDMOOutputType, &cOutTypes,
                    sizeof(szValue)/sizeof(DMO_PARTIAL_MEDIATYPE),
                    (DMO_PARTIAL_MEDIATYPE*)szValue);

	    if (FAILED(hres))
            {
                RegCloseKey(hkey);
                continue;
            }

	    pOutTypes = (DMO_PARTIAL_MEDIATYPE*) szValue;

            TRACE("read %d outtypes for %s:\n", cOutTypes, debugstr_w(szKey));
            for (i = 0; i < cOutTypes; i++) {
                TRACE("outtype %d: type %s, subtype %s\n", i, debugstr_guid(&pOutTypes[i].type),
                    debugstr_guid(&pOutTypes[i].subtype));
            }

            for (i = 0; i < This->cOutTypes; i++)
            {
                for (j = 0; j < cOutTypes; j++) 
                {
                    if (IsMediaTypeEqual(&pOutTypes[j], &This->pOutTypes[i]))
		        break;
                }

		if (j >= cOutTypes)
                    break;
            }

            if (i < This->cOutTypes)
            {
                RegCloseKey(hkey);
                continue;
            }
        }

	/* Media object wasn't filtered so add it to return list */
        Names[count] = NULL;
	len = MAX_PATH * sizeof(WCHAR);
        ret = RegQueryValueExW(hkey, NULL, NULL, NULL, (LPBYTE)szValue, &len);
        if (ERROR_SUCCESS == ret)
	{
            Names[count] = CoTaskMemAlloc((strlenW(szValue) + 1) * sizeof(WCHAR));
	    if (Names[count])
                strcpyW(Names[count], szValue);
	}
        wsprintfW(szGuidKey,szToGuidFmt,szNextKey);
        CLSIDFromString(szGuidKey, &pCLSID[count]);

        TRACE("found match %s %s\n", debugstr_w(szValue), debugstr_w(szNextKey));
        RegCloseKey(hkey);
	count++;
    }

    *pcItemsFetched = count;
    if (*pcItemsFetched < cItemsToFetch)
        hres = S_FALSE;

    TRACE("<-- %i found\n",count);
    return hres;
}
 

/******************************************************************************
 * IEnumDMO_fnSkip
 */
static HRESULT WINAPI IEnumDMO_fnSkip(IEnumDMO * iface, DWORD cItemsToSkip)
{
    IEnumDMOImpl *This = impl_from_IEnumDMO(iface);

    TRACE("(%p)->(%d)\n", This, cItemsToSkip);
    This->index += cItemsToSkip;

    return S_OK;
}


/******************************************************************************
 * IEnumDMO_fnReset
 */
static HRESULT WINAPI IEnumDMO_fnReset(IEnumDMO * iface)
{
    IEnumDMOImpl *This = impl_from_IEnumDMO(iface);

    TRACE("(%p)\n", This);
    This->index = -1;

    return S_OK;
}
 

/******************************************************************************
 * IEnumDMO_fnClone
 */
static HRESULT WINAPI IEnumDMO_fnClone(IEnumDMO *iface, IEnumDMO **ppEnum)
{
    IEnumDMOImpl *This = impl_from_IEnumDMO(iface);
    TRACE("(%p)->(%p)\n", This, ppEnum);
    return IEnumDMO_Constructor(&This->category, This->dwFlags, This->cInTypes, This->pInTypes,
        This->cOutTypes, This->pOutTypes, ppEnum);
}


/***************************************************************
 * DMOEnum (MSDMO.@)
 *
 * Enumerate DirectX Media Objects in the registry.
 */
HRESULT WINAPI DMOEnum(
    REFGUID category,
    DWORD flags,
    DWORD cInTypes,
    const DMO_PARTIAL_MEDIATYPE *pInTypes,
    DWORD cOutTypes,
    const DMO_PARTIAL_MEDIATYPE *pOutTypes,
    IEnumDMO **ppEnum)
{
    TRACE("%s 0x%08x %d %p %d %p %p\n", debugstr_guid(category), flags, cInTypes, pInTypes,
        cOutTypes, pOutTypes, ppEnum);

    if (TRACE_ON(msdmo))
    {
        DWORD i;
        if (cInTypes)
        {
            for (i = 0; i < cInTypes; i++)
                TRACE("intype %d - type %s, subtype %s\n", i, debugstr_guid(&pInTypes[i].type),
                    debugstr_guid(&pInTypes[i].subtype));
        }

        if (cOutTypes) {
            for (i = 0; i < cOutTypes; i++)
                TRACE("outtype %d - type %s, subtype %s\n", i, debugstr_guid(&pOutTypes[i].type),
                    debugstr_guid(&pOutTypes[i].subtype));
        }
    }

    return IEnumDMO_Constructor(category, flags, cInTypes,
        pInTypes, cOutTypes, pOutTypes, ppEnum);
}


static const IEnumDMOVtbl edmovt =
{
	IEnumDMO_fnQueryInterface,
	IEnumDMO_fnAddRef,
	IEnumDMO_fnRelease,
	IEnumDMO_fnNext,
	IEnumDMO_fnSkip,
	IEnumDMO_fnReset,
	IEnumDMO_fnClone,
};


HRESULT read_types(HKEY root, LPCWSTR key, ULONG *supplied, ULONG requested, DMO_PARTIAL_MEDIATYPE* types )
{
    HRESULT ret = S_OK;

    if (MSDMO_MAJOR_VERSION > 5)
    {
        DWORD len;
        LONG rc;

        len = requested * sizeof(DMO_PARTIAL_MEDIATYPE);
        rc = RegQueryValueExW(root, key, NULL, NULL, (LPBYTE) types, &len);
        ret = HRESULT_FROM_WIN32(rc);

        *supplied = len / sizeof(DMO_PARTIAL_MEDIATYPE);
    }
    else
    {
        HKEY hkey;
        WCHAR szGuidKey[64];

        *supplied = 0;
        if (ERROR_SUCCESS == RegOpenKeyExW(root, key, 0, KEY_READ, &hkey))
        {
          int index = 0;
          WCHAR szNextKey[MAX_PATH];
          DWORD len;
          LONG rc = ERROR_SUCCESS;

          while (rc == ERROR_SUCCESS)
          {
            len = MAX_PATH;
            rc = RegEnumKeyExW(hkey, index, szNextKey, &len, NULL, NULL, NULL, NULL);
            if (rc == ERROR_SUCCESS)
            {
              HKEY subk;
              int sub_index = 0;
              LONG rcs = ERROR_SUCCESS;
              WCHAR szSubKey[MAX_PATH];

              RegOpenKeyExW(hkey, szNextKey, 0, KEY_READ, &subk);
              while (rcs == ERROR_SUCCESS)
              {
                len = MAX_PATH;
                rcs = RegEnumKeyExW(subk, sub_index, szSubKey, &len, NULL, NULL, NULL, NULL);
                if (rcs == ERROR_SUCCESS)
                {
                  if (*supplied >= requested)
                  {
                    /* Bailing */
                    ret = S_FALSE;
                    rc = ERROR_MORE_DATA;
                    rcs = ERROR_MORE_DATA;
                    break;
                  }

                  wsprintfW(szGuidKey,szToGuidFmt,szNextKey);
                  CLSIDFromString(szGuidKey, &types[*supplied].type);
                  wsprintfW(szGuidKey,szToGuidFmt,szSubKey);
                  CLSIDFromString(szGuidKey, &types[*supplied].subtype);
                  TRACE("Adding type %s subtype %s at index %i\n",
                    debugstr_guid(&types[*supplied].type),
                    debugstr_guid(&types[*supplied].subtype),
                    *supplied);
                  (*supplied)++;
                }
                sub_index++;
              }
              index++;
            }
          }
          RegCloseKey(hkey);
        }
    }
    return ret;
}

/***************************************************************
 * DMOGetTypes (MSDMO.@)
 */
HRESULT WINAPI DMOGetTypes(REFCLSID clsidDMO,
               ULONG ulInputTypesRequested,
               ULONG* pulInputTypesSupplied,
               DMO_PARTIAL_MEDIATYPE* pInputTypes,
               ULONG ulOutputTypesRequested,
               ULONG* pulOutputTypesSupplied,
               DMO_PARTIAL_MEDIATYPE* pOutputTypes)
{
  HKEY root,hkey;
  HRESULT ret = S_OK;
  WCHAR szguid[64];

  TRACE ("(%s,%u,%p,%p,%u,%p,%p)\n", debugstr_guid(clsidDMO), ulInputTypesRequested,
        pulInputTypesSupplied, pInputTypes, ulOutputTypesRequested, pulOutputTypesSupplied,
        pOutputTypes);

  if (ERROR_SUCCESS != RegOpenKeyExW(HKEY_CLASSES_ROOT, szDMORootKey, 0,
                                     KEY_READ, &root))
    return E_FAIL;

  if (ERROR_SUCCESS != RegOpenKeyExW(root,GUIDToString(szguid,clsidDMO) , 0,
                                     KEY_READ, &hkey))
  {
    RegCloseKey(root);
    return E_FAIL;
  }

  if (ulInputTypesRequested > 0)
  {
    ret = read_types(hkey, szDMOInputType, pulInputTypesSupplied, ulInputTypesRequested, pInputTypes );
  }
  else
    *pulInputTypesSupplied = 0;

  if (ulOutputTypesRequested > 0)
  {
    HRESULT ret2;
    ret2 = read_types(hkey, szDMOOutputType, pulOutputTypesSupplied, ulOutputTypesRequested, pOutputTypes );

    if (ret == S_OK)
        ret = ret2;
  }
  else
    *pulOutputTypesSupplied = 0;

  return ret;
}
