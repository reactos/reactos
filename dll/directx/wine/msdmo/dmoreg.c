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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "winreg.h"
#include "objbase.h"
#include "wine/debug.h"
#include "dmo.h"

WINE_DEFAULT_DEBUG_CHANNEL(msdmo);

BOOL array_reserve(void **elements, unsigned int *capacity, unsigned int count, unsigned int size)
{
    unsigned int max_capacity, new_capacity;
    void *new_elements;

    if (count <= *capacity)
        return TRUE;

    max_capacity = ~0u / size;
    if (count > max_capacity)
        return FALSE;

    new_capacity = max(8, *capacity);
    while (new_capacity < count && new_capacity <= max_capacity / 2)
        new_capacity *= 2;
    if (new_capacity < count)
        new_capacity = count;

    if (!(new_elements = realloc(*elements, new_capacity * size)))
    {
        ERR("Failed to allocate memory.\n");
        return FALSE;
    }

    *elements = new_elements;
    *capacity = new_capacity;
    return TRUE;
}

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

static const IEnumDMOVtbl edmovt;

static const WCHAR *GUIDToString(WCHAR *string, const GUID *guid)
{
    swprintf(string, 37, L"%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
            guid->Data1, guid->Data2, guid->Data3,
            guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
            guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
    return string;
}

static HRESULT string_to_guid(const WCHAR *string, GUID *guid)
{
    WCHAR buffer[39];
    buffer[0] = '{';
    wcscpy(buffer + 1, string);
    buffer[37] = '}';
    buffer[38] = 0;
    return CLSIDFromString(buffer, guid);
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
    return HRESULT_FROM_WIN32(RegSetValueExW(hkey, name, 0, REG_BINARY,
            (const BYTE *)types, count * sizeof(DMO_PARTIAL_MEDIATYPE)));
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

    ret = RegCreateKeyExW(HKEY_CLASSES_ROOT, L"DirectShow\\MediaObjects", 0,
            NULL, 0, KEY_WRITE, NULL, &hrkey, NULL);
    if (ret)
        return E_FAIL;

    /* Create clsidDMO key under MediaObjects */ 
    ret = RegCreateKeyExW(hrkey, GUIDToString(szguid, clsidDMO), 0, NULL,
        REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, NULL);
    if (ret)
        goto lend;

    /* Set default Name value */
    ret = RegSetValueExW(hkey, NULL, 0, REG_SZ, (const BYTE*) szName,
        (wcslen(szName) + 1) * sizeof(WCHAR));

    /* Set InputTypes */
    hres = write_types(hkey, L"InputTypes", pInTypes, cInTypes);

    /* Set OutputTypes */
    hres = write_types(hkey, L"OutputTypes", pOutTypes, cOutTypes);

    if (dwFlags & DMO_REGISTERF_IS_KEYED)
    {
        /* Create Keyed key */ 
        ret = RegCreateKeyExW(hkey, L"Keyed", 0, NULL, 0, KEY_WRITE, NULL, &hckey, NULL);
        if (ret)
            goto lend;
        RegCloseKey(hckey);
    }

    /* Register the category */
    ret = RegCreateKeyExW(hrkey, L"Categories", 0, NULL, 0, KEY_WRITE, NULL, &hckey, NULL);
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
    TRACE(" hresult=0x%08lx\n", hres);
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

    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"DirectShow\\MediaObjects", 0, KEY_WRITE, &rootkey))
        return S_FALSE;

    GUIDToString(dmoW, dmo);
    RegDeleteKeyW(rootkey, dmoW);

    /* open 'Categories' */
    ret = RegOpenKeyExW(rootkey, L"Categories", 0, KEY_WRITE|KEY_ENUMERATE_SUB_KEYS, &categorieskey);
    RegCloseKey(rootkey);
    if (ret)
    {
        hr = HRESULT_FROM_WIN32(ret);
        goto lend;
    }

    /* remove from all categories */
    if (IsEqualGUID(category, &GUID_NULL))
    {
        DWORD index = 0, len = ARRAY_SIZE(catW);

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
HRESULT WINAPI DMOGetName(REFCLSID clsidDMO, WCHAR name[80])
{
    static const INT max_name_len = 80*sizeof(WCHAR);
    DWORD count = max_name_len;
    WCHAR szguid[64];
    HKEY hrkey, hkey;
    LONG ret;

    TRACE("%s %p\n", debugstr_guid(clsidDMO), name);

    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"DirectShow\\MediaObjects", 0, KEY_READ, &hrkey))
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
    lpedmo->hkey = NULL;

    hr = dup_partial_mediatype(pInTypes, cInTypes, &lpedmo->pInTypes);
    if (FAILED(hr))
        goto lerr;

    hr = dup_partial_mediatype(pOutTypes, cOutTypes, &lpedmo->pOutTypes);
    if (FAILED(hr))
        goto lerr;

    /* If not filtering by category enum from media objects root */
    if (IsEqualGUID(guidCategory, &GUID_NULL))
    {
        RegOpenKeyExW(HKEY_CLASSES_ROOT, L"DirectShow\\MediaObjects", 0, KEY_READ, &lpedmo->hkey);
    }
    else
    {
        WCHAR szguid[64];
        WCHAR szKey[MAX_PATH];

        swprintf(szKey, ARRAY_SIZE(szKey), L"DirectShow\\MediaObjects\\Categories\\%s",
                GUIDToString(szguid, guidCategory));
        RegOpenKeyExW(HKEY_CLASSES_ROOT, szKey, 0, KEY_READ, &lpedmo->hkey);
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
    TRACE("(%p)->(%ld)\n", This, refCount);
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

    TRACE("(%p)->(%ld)\n", This, refCount);

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

static BOOL any_types_match(const DMO_PARTIAL_MEDIATYPE *a, unsigned int a_count,
        const DMO_PARTIAL_MEDIATYPE *b, unsigned int b_count)
{
    unsigned int i, j;

    for (i = 0; i < a_count; ++i)
    {
        for (j = 0; j < b_count; ++j)
        {
            if (IsMediaTypeEqual(&a[i], &b[j]))
                return TRUE;
        }
    }
    return FALSE;
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
    DMO_PARTIAL_MEDIATYPE *types = NULL;
    unsigned int types_size = 0;
    HKEY hkey;
    WCHAR szNextKey[MAX_PATH];
    WCHAR path[MAX_PATH];
    WCHAR szValue[MAX_PATH];
    DWORD len;
    UINT count = 0;
    HRESULT hres = S_OK;
    LONG ret;

    IEnumDMOImpl *This = impl_from_IEnumDMO(iface);

    TRACE("(%p)->(%ld %p %p %p)\n", This, cItemsToFetch, pCLSID, Names, pcItemsFetched);

    if (!pCLSID)
        return E_POINTER;

    if (!pcItemsFetched && cItemsToFetch > 1)
        return E_INVALIDARG;

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

        if (string_to_guid(szNextKey, &pCLSID[count]) != S_OK)
            continue;

        TRACE("found %s\n", debugstr_w(szNextKey));

        if (!(This->dwFlags & DMO_ENUMF_INCLUDE_KEYED))
        {
            swprintf(path, ARRAY_SIZE(path), L"DirectShow\\MediaObjects\\%s\\Keyed", szNextKey);
            ret = RegOpenKeyExW(HKEY_CLASSES_ROOT, path, 0, KEY_READ, &hkey);
            if (ERROR_SUCCESS == ret)
            {
                RegCloseKey(hkey);
                /* Skip Keyed entries */
                continue;
            }
        }

        swprintf(path, ARRAY_SIZE(path), L"DirectShow\\MediaObjects\\%s", szNextKey);
        ret = RegOpenKeyExW(HKEY_CLASSES_ROOT, path, 0, KEY_READ, &hkey);
        TRACE("Testing %s.\n", debugstr_w(path));

        if (This->pInTypes)
        {
            DWORD size, i;

            for (;;)
            {
                size = types_size;
                ret = RegQueryValueExW(hkey, L"InputTypes", NULL, NULL, (BYTE *)types, &size);
                if (ret != ERROR_SUCCESS && ret != ERROR_MORE_DATA)
                    break;
                if (size <= types_size)
                    break;
                if (!array_reserve((void **)&types, &types_size, size, 1))
                {
                    RegCloseKey(hkey);
                    free(types);
                    return E_OUTOFMEMORY;
                }
            }
            if (ret)
            {
                RegCloseKey(hkey);
                continue;
            }

            for (i = 0; i < size / sizeof(DMO_PARTIAL_MEDIATYPE); ++i)
                TRACE("intype %ld: type %s, subtype %s\n", i, debugstr_guid(&types[i].type),
                    debugstr_guid(&types[i].subtype));

            if (!any_types_match(types, size / sizeof(DMO_PARTIAL_MEDIATYPE), This->pInTypes, This->cInTypes))
            {
                RegCloseKey(hkey);
                continue;
            }
        }

        if (This->pOutTypes)
        {
            DWORD size = types_size, i;

            for (;;)
            {
                size = types_size;
                ret = RegQueryValueExW(hkey, L"OutputTypes", NULL, NULL, (BYTE *)types, &size);
                if (ret != ERROR_SUCCESS && ret != ERROR_MORE_DATA)
                    break;
                if (size <= types_size)
                    break;
                if (!array_reserve((void **)&types, &types_size, size, 1))
                {
                    RegCloseKey(hkey);
                    free(types);
                    return E_OUTOFMEMORY;
                }
            }
            if (ret)
            {
                RegCloseKey(hkey);
                continue;
            }

            for (i = 0; i < size / sizeof(DMO_PARTIAL_MEDIATYPE); ++i)
                TRACE("outtype %ld: type %s, subtype %s\n", i, debugstr_guid(&types[i].type),
                    debugstr_guid(&types[i].subtype));

            if (!any_types_match(types, size / sizeof(DMO_PARTIAL_MEDIATYPE), This->pOutTypes, This->cOutTypes))
            {
                RegCloseKey(hkey);
                continue;
            }
        }

        /* Media object wasn't filtered so add it to return list */
        len = MAX_PATH * sizeof(WCHAR);
        ret = RegQueryValueExW(hkey, NULL, NULL, NULL, (LPBYTE)szValue, &len);
        if (Names)
        {
            Names[count] = NULL;
            if (ret == ERROR_SUCCESS)
            {
                Names[count] = CoTaskMemAlloc((wcslen(szValue) + 1) * sizeof(WCHAR));
                if (Names[count])
                    wcscpy(Names[count], szValue);
            }
        }

        TRACE("found match %s %s\n", debugstr_w(szValue), debugstr_w(szNextKey));
        RegCloseKey(hkey);
        count++;
    }

    free(types);

    if (pcItemsFetched) *pcItemsFetched = count;
    if (count < cItemsToFetch)
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

    TRACE("(%p)->(%ld)\n", This, cItemsToSkip);
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
    TRACE("%s 0x%08lx %ld %p %ld %p %p\n", debugstr_guid(category), flags, cInTypes, pInTypes,
        cOutTypes, pOutTypes, ppEnum);

    if (TRACE_ON(msdmo))
    {
        DWORD i;
        if (cInTypes)
        {
            for (i = 0; i < cInTypes; i++)
                TRACE("intype %ld - type %s, subtype %s\n", i, debugstr_guid(&pInTypes[i].type),
                    debugstr_guid(&pInTypes[i].subtype));
        }

        if (cOutTypes) {
            for (i = 0; i < cOutTypes; i++)
                TRACE("outtype %ld - type %s, subtype %s\n", i, debugstr_guid(&pOutTypes[i].type),
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

/***************************************************************
 * DMOGetTypes (MSDMO.@)
 */
HRESULT WINAPI DMOGetTypes(REFCLSID clsid, ULONG input_count, ULONG *ret_input_count, DMO_PARTIAL_MEDIATYPE *input,
        ULONG output_count, ULONG *ret_output_count, DMO_PARTIAL_MEDIATYPE *output)
{
    WCHAR guidstr[64];
    HKEY root, key;
    LSTATUS ret;
    DWORD size;

    TRACE("clsid %s, input_count %lu, ret_input_count %p, input %p, output_count %lu, ret_output_count %p, output %p.\n",
            debugstr_guid(clsid), input_count, ret_input_count, input, output_count, ret_output_count, output);

    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"DirectShow\\MediaObjects", 0, KEY_READ, &root))
        return E_FAIL;

    if (RegOpenKeyExW(root, GUIDToString(guidstr, clsid), 0, KEY_READ, &key))
    {
        RegCloseKey(root);
        return E_FAIL;
    }

    *ret_input_count = 0;
    size = input_count * sizeof(DMO_PARTIAL_MEDIATYPE);
    ret = RegQueryValueExW(key, L"InputTypes", NULL, NULL, (BYTE *)input, &size);
    if (!ret || ret == ERROR_MORE_DATA)
        *ret_input_count = min(input_count, size / sizeof(DMO_PARTIAL_MEDIATYPE));

    *ret_output_count = 0;
    size = output_count * sizeof(DMO_PARTIAL_MEDIATYPE);
    ret = RegQueryValueExW(key, L"OutputTypes", NULL, NULL, (BYTE *)output, &size);
    if (!ret || ret == ERROR_MORE_DATA)
        *ret_output_count = min(output_count, size / sizeof(DMO_PARTIAL_MEDIATYPE));

    RegCloseKey(key);
    RegCloseKey(root);
    return S_OK;
}
