/*
 *	ComCatMgr ICatInformation implementation for comcat.dll
 *
 * Copyright (C) 2002 John K. Hohm
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

#include <string.h>
#include "comcat_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

static LPENUMCATEGORYINFO COMCAT_IEnumCATEGORYINFO_Construct(LCID lcid);
static HRESULT COMCAT_GetCategoryDesc(HKEY key, LCID lcid, PWCHAR pszDesc,
				      ULONG buf_wchars);

struct class_categories {
    LPCWSTR impl_strings;
    LPCWSTR req_strings;
};

static struct class_categories *COMCAT_PrepareClassCategories(
    ULONG impl_count, const CATID *impl_catids, ULONG req_count, const CATID *req_catids);
static HRESULT COMCAT_IsClassOfCategories(
    HKEY key, struct class_categories const* class_categories);
static LPENUMGUID COMCAT_CLSID_IEnumGUID_Construct(
    struct class_categories *class_categories);
static LPENUMGUID COMCAT_CATID_IEnumGUID_Construct(
    REFCLSID rclsid, LPCWSTR impl_req);

/**********************************************************************
 * COMCAT_ICatInformation_QueryInterface
 */
static HRESULT WINAPI COMCAT_ICatInformation_QueryInterface(
    LPCATINFORMATION iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    ICOM_THIS_MULTI(ComCatMgrImpl, infVtbl, iface);
    TRACE("\n\tIID:\t%s\n",debugstr_guid(riid));

    if (This == NULL || ppvObj == NULL) return E_POINTER;

    return IUnknown_QueryInterface((LPUNKNOWN)&This->unkVtbl, riid, ppvObj);
}

/**********************************************************************
 * COMCAT_ICatInformation_AddRef
 */
static ULONG WINAPI COMCAT_ICatInformation_AddRef(LPCATINFORMATION iface)
{
    ICOM_THIS_MULTI(ComCatMgrImpl, infVtbl, iface);
    TRACE("\n");

    if (This == NULL) return E_POINTER;

    return IUnknown_AddRef((LPUNKNOWN)&This->unkVtbl);
}

/**********************************************************************
 * COMCAT_ICatInformation_Release
 */
static ULONG WINAPI COMCAT_ICatInformation_Release(LPCATINFORMATION iface)
{
    ICOM_THIS_MULTI(ComCatMgrImpl, infVtbl, iface);
    TRACE("\n");

    if (This == NULL) return E_POINTER;

    return IUnknown_Release((LPUNKNOWN)&This->unkVtbl);
}

/**********************************************************************
 * COMCAT_ICatInformation_EnumCategories
 */
static HRESULT WINAPI COMCAT_ICatInformation_EnumCategories(
    LPCATINFORMATION iface,
    LCID lcid,
    LPENUMCATEGORYINFO *ppenumCatInfo)
{
/*     ICOM_THIS_MULTI(ComCatMgrImpl, infVtbl, iface); */
    TRACE("\n");

    if (iface == NULL || ppenumCatInfo == NULL) return E_POINTER;

    *ppenumCatInfo = COMCAT_IEnumCATEGORYINFO_Construct(lcid);
    if (*ppenumCatInfo == NULL) return E_OUTOFMEMORY;
    IEnumCATEGORYINFO_AddRef(*ppenumCatInfo);
    return S_OK;
}

/**********************************************************************
 * COMCAT_ICatInformation_GetCategoryDesc
 */
static HRESULT WINAPI COMCAT_ICatInformation_GetCategoryDesc(
    LPCATINFORMATION iface,
    REFCATID rcatid,
    LCID lcid,
    PWCHAR *ppszDesc)
{
/*     ICOM_THIS_MULTI(ComCatMgrImpl, infVtbl, iface); */
    WCHAR keyname[60] = { 'C', 'o', 'm', 'p', 'o', 'n', 'e', 'n',
			  't', ' ', 'C', 'a', 't', 'e', 'g', 'o',
			  'r', 'i', 'e', 's', '\\', 0 };
    HKEY key;
    HRESULT res;

    TRACE("\n\tCATID:\t%s\n\tLCID:\t%X\n",debugstr_guid(rcatid), lcid);

    if (rcatid == NULL || ppszDesc == NULL) return E_INVALIDARG;

    /* Open the key for this category. */
    if (!StringFromGUID2(rcatid, keyname + 21, 39)) return E_FAIL;
    res = RegOpenKeyExW(HKEY_CLASSES_ROOT, keyname, 0, KEY_READ, &key);
    if (res != ERROR_SUCCESS) return CAT_E_CATIDNOEXIST;

    /* Allocate a sensible amount of memory for the description. */
    *ppszDesc = (PWCHAR) CoTaskMemAlloc(128 * sizeof(WCHAR));
    if (*ppszDesc == NULL) {
	RegCloseKey(key);
	return E_OUTOFMEMORY;
    }

    /* Get the description, and make sure it's null terminated. */
    res = COMCAT_GetCategoryDesc(key, lcid, *ppszDesc, 128);
    RegCloseKey(key);
    if (FAILED(res)) {
	CoTaskMemFree(*ppszDesc);
	return res;
    }

    return S_OK;
}

/**********************************************************************
 * COMCAT_ICatInformation_EnumClassesOfCategories
 */
static HRESULT WINAPI COMCAT_ICatInformation_EnumClassesOfCategories(
    LPCATINFORMATION iface,
    ULONG cImplemented,
    CATID *rgcatidImpl,
    ULONG cRequired,
    CATID *rgcatidReq,
    LPENUMCLSID *ppenumCLSID)
{
/*     ICOM_THIS_MULTI(ComCatMgrImpl, infVtbl, iface); */
    struct class_categories *categories;

    TRACE("\n");

	if (cImplemented == (ULONG)-1)
		cImplemented = 0;
	if (cRequired == (ULONG)-1)
		cRequired = 0;

    if (iface == NULL || ppenumCLSID == NULL ||
	(cImplemented && rgcatidImpl == NULL) ||
	(cRequired && rgcatidReq == NULL)) return E_POINTER;

    categories = COMCAT_PrepareClassCategories(cImplemented, rgcatidImpl,
					       cRequired, rgcatidReq);
    if (categories == NULL) return E_OUTOFMEMORY;
    *ppenumCLSID = COMCAT_CLSID_IEnumGUID_Construct(categories);
    if (*ppenumCLSID == NULL) {
	HeapFree(GetProcessHeap(), 0, categories);
	return E_OUTOFMEMORY;
    }
    IEnumGUID_AddRef(*ppenumCLSID);
    return S_OK;
}

/**********************************************************************
 * COMCAT_ICatInformation_IsClassOfCategories
 */
static HRESULT WINAPI COMCAT_ICatInformation_IsClassOfCategories(
    LPCATINFORMATION iface,
    REFCLSID rclsid,
    ULONG cImplemented,
    CATID *rgcatidImpl,
    ULONG cRequired,
    CATID *rgcatidReq)
{
/*     ICOM_THIS_MULTI(ComCatMgrImpl, infVtbl, iface); */
    WCHAR keyname[45] = { 'C', 'L', 'S', 'I', 'D', '\\', 0 };
    HRESULT res;
    struct class_categories *categories;
    HKEY key;

    if (WINE_TRACE_ON(ole)) {
	ULONG count;
	TRACE("\n\tCLSID:\t%s\n\tImplemented %u\n",debugstr_guid(rclsid),cImplemented);
	for (count = 0; count < cImplemented; ++count)
	    TRACE("\t\t%s\n",debugstr_guid(&rgcatidImpl[count]));
	TRACE("\tRequired %u\n",cRequired);
	for (count = 0; count < cRequired; ++count)
	    TRACE("\t\t%s\n",debugstr_guid(&rgcatidReq[count]));
    }

    if ((cImplemented && rgcatidImpl == NULL) ||
	(cRequired && rgcatidReq == NULL)) return E_POINTER;

    res = StringFromGUID2(rclsid, keyname + 6, 39);
    if (FAILED(res)) return res;

    categories = COMCAT_PrepareClassCategories(cImplemented, rgcatidImpl,
					       cRequired, rgcatidReq);
    if (categories == NULL) return E_OUTOFMEMORY;

    res = RegOpenKeyExW(HKEY_CLASSES_ROOT, keyname, 0, KEY_READ, &key);
    if (res == ERROR_SUCCESS) {
	res = COMCAT_IsClassOfCategories(key, categories);
	RegCloseKey(key);
    } else res = S_FALSE;

    HeapFree(GetProcessHeap(), 0, categories);

    return res;
}

/**********************************************************************
 * COMCAT_ICatInformation_EnumImplCategoriesOfClass
 */
static HRESULT WINAPI COMCAT_ICatInformation_EnumImplCategoriesOfClass(
    LPCATINFORMATION iface,
    REFCLSID rclsid,
    LPENUMCATID *ppenumCATID)
{
/*     ICOM_THIS_MULTI(ComCatMgrImpl, infVtbl, iface); */
    static const WCHAR postfix[24] = { '\\', 'I', 'm', 'p', 'l', 'e', 'm', 'e',
			  'n', 't', 'e', 'd', ' ', 'C', 'a', 't',
			  'e', 'g', 'o', 'r', 'i', 'e', 's', 0 };

    TRACE("\n\tCLSID:\t%s\n",debugstr_guid(rclsid));

    if (iface == NULL || rclsid == NULL || ppenumCATID == NULL)
	return E_POINTER;

    *ppenumCATID = COMCAT_CATID_IEnumGUID_Construct(rclsid, postfix);
    if (*ppenumCATID == NULL) return E_OUTOFMEMORY;
    return S_OK;
}

/**********************************************************************
 * COMCAT_ICatInformation_EnumReqCategoriesOfClass
 */
static HRESULT WINAPI COMCAT_ICatInformation_EnumReqCategoriesOfClass(
    LPCATINFORMATION iface,
    REFCLSID rclsid,
    LPENUMCATID *ppenumCATID)
{
/*     ICOM_THIS_MULTI(ComCatMgrImpl, infVtbl, iface); */
    static const WCHAR postfix[21] = { '\\', 'R', 'e', 'q', 'u', 'i', 'r', 'e',
			  'd', ' ', 'C', 'a', 't', 'e', 'g', 'o',
			  'r', 'i', 'e', 's', 0 };

    TRACE("\n\tCLSID:\t%s\n",debugstr_guid(rclsid));

    if (iface == NULL || rclsid == NULL || ppenumCATID == NULL)
	return E_POINTER;

    *ppenumCATID = COMCAT_CATID_IEnumGUID_Construct(rclsid, postfix);
    if (*ppenumCATID == NULL) return E_OUTOFMEMORY;
    return S_OK;
}

/**********************************************************************
 * COMCAT_ICatInformation_Vtbl
 */
const ICatInformationVtbl COMCAT_ICatInformation_Vtbl =
{
    COMCAT_ICatInformation_QueryInterface,
    COMCAT_ICatInformation_AddRef,
    COMCAT_ICatInformation_Release,
    COMCAT_ICatInformation_EnumCategories,
    COMCAT_ICatInformation_GetCategoryDesc,
    COMCAT_ICatInformation_EnumClassesOfCategories,
    COMCAT_ICatInformation_IsClassOfCategories,
    COMCAT_ICatInformation_EnumImplCategoriesOfClass,
    COMCAT_ICatInformation_EnumReqCategoriesOfClass
};

/**********************************************************************
 * IEnumCATEGORYINFO implementation
 *
 * This implementation is not thread-safe.  The manager itself is, but
 * I can't imagine a valid use of an enumerator in several threads.
 */
typedef struct
{
    const IEnumCATEGORYINFOVtbl *lpVtbl;
    LONG  ref;
    LCID  lcid;
    HKEY  key;
    DWORD next_index;
} IEnumCATEGORYINFOImpl;

static ULONG WINAPI COMCAT_IEnumCATEGORYINFO_AddRef(LPENUMCATEGORYINFO iface)
{
    IEnumCATEGORYINFOImpl *This = (IEnumCATEGORYINFOImpl *)iface;

    TRACE("\n");

    if (This == NULL) return E_POINTER;

    return InterlockedIncrement(&This->ref);
}

static HRESULT WINAPI COMCAT_IEnumCATEGORYINFO_QueryInterface(
    LPENUMCATEGORYINFO iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    IEnumCATEGORYINFOImpl *This = (IEnumCATEGORYINFOImpl *)iface;
    TRACE("\n\tIID:\t%s\n",debugstr_guid(riid));

    if (This == NULL || ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
	IsEqualGUID(riid, &IID_IEnumCATEGORYINFO))
    {
	*ppvObj = (LPVOID)iface;
	COMCAT_IEnumCATEGORYINFO_AddRef(iface);
	return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI COMCAT_IEnumCATEGORYINFO_Release(LPENUMCATEGORYINFO iface)
{
    IEnumCATEGORYINFOImpl *This = (IEnumCATEGORYINFOImpl *)iface;
    ULONG ref;

    TRACE("\n");

    if (This == NULL) return E_POINTER;

    ref = InterlockedDecrement(&This->ref);
    if (ref == 0) {
	if (This->key) RegCloseKey(This->key);
	HeapFree(GetProcessHeap(), 0, This);
	return 0;
    }
    return ref;
}

static HRESULT WINAPI COMCAT_IEnumCATEGORYINFO_Next(
    LPENUMCATEGORYINFO iface,
    ULONG celt,
    CATEGORYINFO *rgelt,
    ULONG *pceltFetched)
{
    IEnumCATEGORYINFOImpl *This = (IEnumCATEGORYINFOImpl *)iface;
    ULONG fetched = 0;

    TRACE("\n");

    if (This == NULL || rgelt == NULL) return E_POINTER;

    if (This->key) while (fetched < celt) {
	HRESULT res;
	WCHAR catid[39];
	DWORD cName = 39;
	HKEY subkey;

	res = RegEnumKeyExW(This->key, This->next_index, catid, &cName,
			    NULL, NULL, NULL, NULL);
	if (res != ERROR_SUCCESS && res != ERROR_MORE_DATA) break;
	++(This->next_index);

	res = CLSIDFromString(catid, &rgelt->catid);
	if (FAILED(res)) continue;

	res = RegOpenKeyExW(This->key, catid, 0, KEY_READ, &subkey);
	if (res != ERROR_SUCCESS) continue;

	res = COMCAT_GetCategoryDesc(subkey, This->lcid,
				     rgelt->szDescription, 128);
	RegCloseKey(subkey);
	if (FAILED(res)) continue;

	rgelt->lcid = This->lcid;
	++fetched;
	++rgelt;
    }

    if (pceltFetched) *pceltFetched = fetched;
    return fetched == celt ? S_OK : S_FALSE;
}

static HRESULT WINAPI COMCAT_IEnumCATEGORYINFO_Skip(
    LPENUMCATEGORYINFO iface,
    ULONG celt)
{
    IEnumCATEGORYINFOImpl *This = (IEnumCATEGORYINFOImpl *)iface;

    TRACE("\n");

    if (This == NULL) return E_POINTER;
    This->next_index += celt;
    /* This should return S_FALSE when there aren't celt elems to skip. */
    return S_OK;
}

static HRESULT WINAPI COMCAT_IEnumCATEGORYINFO_Reset(LPENUMCATEGORYINFO iface)
{
    IEnumCATEGORYINFOImpl *This = (IEnumCATEGORYINFOImpl *)iface;

    TRACE("\n");

    if (This == NULL) return E_POINTER;
    This->next_index = 0;
    return S_OK;
}

static HRESULT WINAPI COMCAT_IEnumCATEGORYINFO_Clone(
    LPENUMCATEGORYINFO iface,
    IEnumCATEGORYINFO **ppenum)
{
    IEnumCATEGORYINFOImpl *This = (IEnumCATEGORYINFOImpl *)iface;
    static const WCHAR keyname[] = { 'C', 'o', 'm', 'p', 'o', 'n', 'e', 'n',
                                     't', ' ', 'C', 'a', 't', 'e', 'g', 'o',
                                     'r', 'i', 'e', 's', 0 };
    IEnumCATEGORYINFOImpl *new_this;

    TRACE("\n");

    if (This == NULL || ppenum == NULL) return E_POINTER;

    new_this = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IEnumCATEGORYINFOImpl));
    if (new_this == NULL) return E_OUTOFMEMORY;

    new_this->lpVtbl = This->lpVtbl;
    new_this->ref = 1;
    new_this->lcid = This->lcid;
    /* FIXME: could we more efficiently use DuplicateHandle? */
    RegOpenKeyExW(HKEY_CLASSES_ROOT, keyname, 0, KEY_READ, &new_this->key);
    new_this->next_index = This->next_index;

    *ppenum = (LPENUMCATEGORYINFO)new_this;
    return S_OK;
}

static const IEnumCATEGORYINFOVtbl COMCAT_IEnumCATEGORYINFO_Vtbl =
{
    COMCAT_IEnumCATEGORYINFO_QueryInterface,
    COMCAT_IEnumCATEGORYINFO_AddRef,
    COMCAT_IEnumCATEGORYINFO_Release,
    COMCAT_IEnumCATEGORYINFO_Next,
    COMCAT_IEnumCATEGORYINFO_Skip,
    COMCAT_IEnumCATEGORYINFO_Reset,
    COMCAT_IEnumCATEGORYINFO_Clone
};

static LPENUMCATEGORYINFO COMCAT_IEnumCATEGORYINFO_Construct(LCID lcid)
{
    IEnumCATEGORYINFOImpl *This;

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IEnumCATEGORYINFOImpl));
    if (This) {
        static const WCHAR keyname[] = { 'C', 'o', 'm', 'p', 'o', 'n', 'e', 'n',
                                         't', ' ', 'C', 'a', 't', 'e', 'g', 'o',
                                         'r', 'i', 'e', 's', 0 };

	This->lpVtbl = &COMCAT_IEnumCATEGORYINFO_Vtbl;
	This->lcid = lcid;
	RegOpenKeyExW(HKEY_CLASSES_ROOT, keyname, 0, KEY_READ, &This->key);
    }
    return (LPENUMCATEGORYINFO)This;
}

/**********************************************************************
 * COMCAT_GetCategoryDesc
 */
static HRESULT COMCAT_GetCategoryDesc(HKEY key, LCID lcid, PWCHAR pszDesc,
				      ULONG buf_wchars)
{
    static const WCHAR fmt[] = { '%', 'l', 'X', 0 };
    WCHAR valname[5];
    HRESULT res;
    DWORD type, size = (buf_wchars - 1) * sizeof(WCHAR);

    if (pszDesc == NULL) return E_INVALIDARG;

    /* FIXME: lcid comparisons are more complex than this! */
    wsprintfW(valname, fmt, lcid);
    res = RegQueryValueExW(key, valname, 0, &type, (LPBYTE)pszDesc, &size);
    if (res != ERROR_SUCCESS || type != REG_SZ) {
	FIXME("Simplified lcid comparison\n");
	return CAT_E_NODESCRIPTION;
    }
    pszDesc[size / sizeof(WCHAR)] = (WCHAR)0;

    return S_OK;
}

/**********************************************************************
 * COMCAT_PrepareClassCategories
 */
static struct class_categories *COMCAT_PrepareClassCategories(
    ULONG impl_count, const CATID *impl_catids, ULONG req_count, const CATID *req_catids)
{
    struct class_categories *categories;
    WCHAR *strings;

    categories = HeapAlloc(
	GetProcessHeap(), HEAP_ZERO_MEMORY,
	sizeof(struct class_categories) +
	((impl_count + req_count) * 39 + 2) * sizeof(WCHAR));
    if (categories == NULL) return categories;

    strings = (WCHAR *)(categories + 1);
    categories->impl_strings = strings;
    while (impl_count--) {
	StringFromGUID2(impl_catids++, strings, 39);
	strings += 39;
    }
    *strings++ = 0;

    categories->req_strings = strings;
    while (req_count--) {
	StringFromGUID2(req_catids++, strings, 39);
	strings += 39;
    }
    *strings++ = 0;

    return categories;
}

/**********************************************************************
 * COMCAT_IsClassOfCategories
 */
static HRESULT COMCAT_IsClassOfCategories(
    HKEY key,
    struct class_categories const* categories)
{
    static const WCHAR impl_keyname[] = { 'I', 'm', 'p', 'l', 'e', 'm', 'e', 'n',
                                          't', 'e', 'd', ' ', 'C', 'a', 't', 'e',
                                          'g', 'o', 'r', 'i', 'e', 's', 0 };
    static const WCHAR req_keyname[]  = { 'R', 'e', 'q', 'u', 'i', 'r', 'e', 'd',
                                          ' ', 'C', 'a', 't', 'e', 'g', 'o', 'r',
                                          'i', 'e', 's', 0 };
    HKEY subkey;
    HRESULT res;
    DWORD index;
    LPCWSTR string;

    /* Check that every given category is implemented by class. */
    res = RegOpenKeyExW(key, impl_keyname, 0, KEY_READ, &subkey);
    if (res != ERROR_SUCCESS) return S_FALSE;
    for (string = categories->impl_strings; *string; string += 39) {
	HKEY catkey;
	res = RegOpenKeyExW(subkey, string, 0, 0, &catkey);
	if (res != ERROR_SUCCESS) {
	    RegCloseKey(subkey);
	    return S_FALSE;
	}
	RegCloseKey(catkey);
    }
    RegCloseKey(subkey);

    /* Check that all categories required by class are given. */
    res = RegOpenKeyExW(key, req_keyname, 0, KEY_READ, &subkey);
    if (res == ERROR_SUCCESS) {
	for (index = 0; ; ++index) {
	    WCHAR keyname[39];
	    DWORD size = 39;

	    res = RegEnumKeyExW(subkey, index, keyname, &size,
				NULL, NULL, NULL, NULL);
	    if (res != ERROR_SUCCESS && res != ERROR_MORE_DATA) break;
	    if (size != 38) continue; /* bogus catid in registry */
	    for (string = categories->req_strings; *string; string += 39)
		if (!strcmpiW(string, keyname)) break;
	    if (!*string) {
		RegCloseKey(subkey);
		return S_FALSE;
	    }
	}
	RegCloseKey(subkey);
    }

    return S_OK;
}

/**********************************************************************
 * ClassesOfCategories IEnumCLSID (IEnumGUID) implementation
 *
 * This implementation is not thread-safe.  The manager itself is, but
 * I can't imagine a valid use of an enumerator in several threads.
 */
typedef struct
{
    const IEnumGUIDVtbl *lpVtbl;
    LONG  ref;
    struct class_categories *categories;
    HKEY  key;
    DWORD next_index;
} CLSID_IEnumGUIDImpl;

static ULONG WINAPI COMCAT_CLSID_IEnumGUID_AddRef(LPENUMGUID iface)
{
    CLSID_IEnumGUIDImpl *This = (CLSID_IEnumGUIDImpl *)iface;
    TRACE("\n");

    if (This == NULL) return E_POINTER;

    return InterlockedIncrement(&This->ref);
}

static HRESULT WINAPI COMCAT_CLSID_IEnumGUID_QueryInterface(
    LPENUMGUID iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    CLSID_IEnumGUIDImpl *This = (CLSID_IEnumGUIDImpl *)iface;
    TRACE("\n\tIID:\t%s\n",debugstr_guid(riid));

    if (This == NULL || ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
	IsEqualGUID(riid, &IID_IEnumGUID))
    {
	*ppvObj = (LPVOID)iface;
	COMCAT_CLSID_IEnumGUID_AddRef(iface);
	return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI COMCAT_CLSID_IEnumGUID_Release(LPENUMGUID iface)
{
    CLSID_IEnumGUIDImpl *This = (CLSID_IEnumGUIDImpl *)iface;
    ULONG ref;

    TRACE("\n");

    if (This == NULL) return E_POINTER;

    ref = InterlockedDecrement(&This->ref);
    if (ref == 0) {
	if (This->key) RegCloseKey(This->key);
	HeapFree(GetProcessHeap(), 0, (LPVOID)This->categories);
	HeapFree(GetProcessHeap(), 0, This);
	return 0;
    }
    return ref;
}

static HRESULT WINAPI COMCAT_CLSID_IEnumGUID_Next(
    LPENUMGUID iface,
    ULONG celt,
    GUID *rgelt,
    ULONG *pceltFetched)
{
    CLSID_IEnumGUIDImpl *This = (CLSID_IEnumGUIDImpl *)iface;
    ULONG fetched = 0;

    TRACE("\n");

    if (This == NULL || rgelt == NULL) return E_POINTER;

    if (This->key) while (fetched < celt) {
	HRESULT res;
	WCHAR clsid[39];
	DWORD cName = 39;
	HKEY subkey;

	res = RegEnumKeyExW(This->key, This->next_index, clsid, &cName,
			    NULL, NULL, NULL, NULL);
	if (res != ERROR_SUCCESS && res != ERROR_MORE_DATA) break;
	++(This->next_index);

	res = CLSIDFromString(clsid, rgelt);
	if (FAILED(res)) continue;

	res = RegOpenKeyExW(This->key, clsid, 0, KEY_READ, &subkey);
	if (res != ERROR_SUCCESS) continue;

	res = COMCAT_IsClassOfCategories(subkey, This->categories);
	RegCloseKey(subkey);
	if (res != S_OK) continue;

	++fetched;
	++rgelt;
    }

    if (pceltFetched) *pceltFetched = fetched;
    return fetched == celt ? S_OK : S_FALSE;
}

static HRESULT WINAPI COMCAT_CLSID_IEnumGUID_Skip(
    LPENUMGUID iface,
    ULONG celt)
{
    CLSID_IEnumGUIDImpl *This = (CLSID_IEnumGUIDImpl *)iface;

    TRACE("\n");

    if (This == NULL) return E_POINTER;
    This->next_index += celt;
    FIXME("Never returns S_FALSE\n");
    return S_OK;
}

static HRESULT WINAPI COMCAT_CLSID_IEnumGUID_Reset(LPENUMGUID iface)
{
    CLSID_IEnumGUIDImpl *This = (CLSID_IEnumGUIDImpl *)iface;

    TRACE("\n");

    if (This == NULL) return E_POINTER;
    This->next_index = 0;
    return S_OK;
}

static HRESULT WINAPI COMCAT_CLSID_IEnumGUID_Clone(
    LPENUMGUID iface,
    IEnumGUID **ppenum)
{
    CLSID_IEnumGUIDImpl *This = (CLSID_IEnumGUIDImpl *)iface;
    static const WCHAR keyname[] = { 'C', 'L', 'S', 'I', 'D', 0 };
    CLSID_IEnumGUIDImpl *new_this;
    DWORD size;

    TRACE("\n");

    if (This == NULL || ppenum == NULL) return E_POINTER;

    new_this = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CLSID_IEnumGUIDImpl));
    if (new_this == NULL) return E_OUTOFMEMORY;

    new_this->lpVtbl = This->lpVtbl;
    new_this->ref = 1;
    size = HeapSize(GetProcessHeap(), 0, (LPVOID)This->categories);
    new_this->categories =
	HeapAlloc(GetProcessHeap(), 0, size);
    if (new_this->categories == NULL) {
	HeapFree(GetProcessHeap(), 0, new_this);
	return E_OUTOFMEMORY;
    }
    memcpy((LPVOID)new_this->categories, This->categories, size);
    /* FIXME: could we more efficiently use DuplicateHandle? */
    RegOpenKeyExW(HKEY_CLASSES_ROOT, keyname, 0, KEY_READ, &new_this->key);
    new_this->next_index = This->next_index;

    *ppenum = (LPENUMGUID)new_this;
    return S_OK;
}

static const IEnumGUIDVtbl COMCAT_CLSID_IEnumGUID_Vtbl =
{
    COMCAT_CLSID_IEnumGUID_QueryInterface,
    COMCAT_CLSID_IEnumGUID_AddRef,
    COMCAT_CLSID_IEnumGUID_Release,
    COMCAT_CLSID_IEnumGUID_Next,
    COMCAT_CLSID_IEnumGUID_Skip,
    COMCAT_CLSID_IEnumGUID_Reset,
    COMCAT_CLSID_IEnumGUID_Clone
};

static LPENUMGUID COMCAT_CLSID_IEnumGUID_Construct(struct class_categories *categories)
{
    CLSID_IEnumGUIDImpl *This;

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CLSID_IEnumGUIDImpl));
    if (This) {
	static const WCHAR keyname[] = { 'C', 'L', 'S', 'I', 'D', 0 };

	This->lpVtbl = &COMCAT_CLSID_IEnumGUID_Vtbl;
	This->categories = categories;
	RegOpenKeyExW(HKEY_CLASSES_ROOT, keyname, 0, KEY_READ, &This->key);
    }
    return (LPENUMGUID)This;
}

/**********************************************************************
 * CategoriesOfClass IEnumCATID (IEnumGUID) implementation
 *
 * This implementation is not thread-safe.  The manager itself is, but
 * I can't imagine a valid use of an enumerator in several threads.
 */
typedef struct
{
    const IEnumGUIDVtbl *lpVtbl;
    LONG  ref;
    WCHAR keyname[68];
    HKEY  key;
    DWORD next_index;
} CATID_IEnumGUIDImpl;

static ULONG WINAPI COMCAT_CATID_IEnumGUID_AddRef(LPENUMGUID iface)
{
    CATID_IEnumGUIDImpl *This = (CATID_IEnumGUIDImpl *)iface;
    TRACE("\n");

    if (This == NULL) return E_POINTER;

    return InterlockedIncrement(&This->ref);
}

static HRESULT WINAPI COMCAT_CATID_IEnumGUID_QueryInterface(
    LPENUMGUID iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    CATID_IEnumGUIDImpl *This = (CATID_IEnumGUIDImpl *)iface;
    TRACE("\n\tIID:\t%s\n",debugstr_guid(riid));

    if (This == NULL || ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
	IsEqualGUID(riid, &IID_IEnumGUID))
    {
	*ppvObj = (LPVOID)iface;
	COMCAT_CATID_IEnumGUID_AddRef(iface);
	return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI COMCAT_CATID_IEnumGUID_Release(LPENUMGUID iface)
{
    CATID_IEnumGUIDImpl *This = (CATID_IEnumGUIDImpl *)iface;
    ULONG ref;

    TRACE("\n");

    if (This == NULL) return E_POINTER;

    ref = InterlockedDecrement(&This->ref);
    if (ref == 0) {
	if (This->key) RegCloseKey(This->key);
	HeapFree(GetProcessHeap(), 0, This);
	return 0;
    }
    return ref;
}

static HRESULT WINAPI COMCAT_CATID_IEnumGUID_Next(
    LPENUMGUID iface,
    ULONG celt,
    GUID *rgelt,
    ULONG *pceltFetched)
{
    CATID_IEnumGUIDImpl *This = (CATID_IEnumGUIDImpl *)iface;
    ULONG fetched = 0;

    TRACE("\n");

    if (This == NULL || rgelt == NULL) return E_POINTER;

    if (This->key) while (fetched < celt) {
	HRESULT res;
	WCHAR catid[39];
	DWORD cName = 39;

	res = RegEnumKeyExW(This->key, This->next_index, catid, &cName,
			    NULL, NULL, NULL, NULL);
	if (res != ERROR_SUCCESS && res != ERROR_MORE_DATA) break;
	++(This->next_index);

	res = CLSIDFromString(catid, rgelt);
	if (FAILED(res)) continue;

	++fetched;
	++rgelt;
    }

    if (pceltFetched) *pceltFetched = fetched;
    return fetched == celt ? S_OK : S_FALSE;
}

static HRESULT WINAPI COMCAT_CATID_IEnumGUID_Skip(
    LPENUMGUID iface,
    ULONG celt)
{
    CATID_IEnumGUIDImpl *This = (CATID_IEnumGUIDImpl *)iface;

    TRACE("\n");

    if (This == NULL) return E_POINTER;
    This->next_index += celt;
    FIXME("Never returns S_FALSE\n");
    return S_OK;
}

static HRESULT WINAPI COMCAT_CATID_IEnumGUID_Reset(LPENUMGUID iface)
{
    CATID_IEnumGUIDImpl *This = (CATID_IEnumGUIDImpl *)iface;

    TRACE("\n");

    if (This == NULL) return E_POINTER;
    This->next_index = 0;
    return S_OK;
}

static HRESULT WINAPI COMCAT_CATID_IEnumGUID_Clone(
    LPENUMGUID iface,
    IEnumGUID **ppenum)
{
    CATID_IEnumGUIDImpl *This = (CATID_IEnumGUIDImpl *)iface;
    CATID_IEnumGUIDImpl *new_this;

    TRACE("\n");

    if (This == NULL || ppenum == NULL) return E_POINTER;

    new_this = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CATID_IEnumGUIDImpl));
    if (new_this == NULL) return E_OUTOFMEMORY;

    new_this->lpVtbl = This->lpVtbl;
    new_this->ref = 1;
    lstrcpyW(new_this->keyname, This->keyname);
    /* FIXME: could we more efficiently use DuplicateHandle? */
    RegOpenKeyExW(HKEY_CLASSES_ROOT, new_this->keyname, 0, KEY_READ, &new_this->key);
    new_this->next_index = This->next_index;

    *ppenum = (LPENUMGUID)new_this;
    return S_OK;
}

static const IEnumGUIDVtbl COMCAT_CATID_IEnumGUID_Vtbl =
{
    COMCAT_CATID_IEnumGUID_QueryInterface,
    COMCAT_CATID_IEnumGUID_AddRef,
    COMCAT_CATID_IEnumGUID_Release,
    COMCAT_CATID_IEnumGUID_Next,
    COMCAT_CATID_IEnumGUID_Skip,
    COMCAT_CATID_IEnumGUID_Reset,
    COMCAT_CATID_IEnumGUID_Clone
};

static LPENUMGUID COMCAT_CATID_IEnumGUID_Construct(
    REFCLSID rclsid, LPCWSTR postfix)
{
    CATID_IEnumGUIDImpl *This;

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CATID_IEnumGUIDImpl));
    if (This) {
	WCHAR prefix[6] = { 'C', 'L', 'S', 'I', 'D', '\\' };

	This->lpVtbl = &COMCAT_CATID_IEnumGUID_Vtbl;
	memcpy(This->keyname, prefix, sizeof(prefix));
	StringFromGUID2(rclsid, This->keyname + 6, 39);
	lstrcpyW(This->keyname + 44, postfix);
	RegOpenKeyExW(HKEY_CLASSES_ROOT, This->keyname, 0, KEY_READ, &This->key);
    }
    return (LPENUMGUID)This;
}
