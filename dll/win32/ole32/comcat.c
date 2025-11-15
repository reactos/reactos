/*
 * Comcat implementation
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
#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "winerror.h"

#include "ole2.h"
#include "comcat.h"
#include "compobj_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

static const ICatRegisterVtbl COMCAT_ICatRegister_Vtbl;
static const ICatInformationVtbl COMCAT_ICatInformation_Vtbl;

typedef struct
{
    ICatRegister ICatRegister_iface;
    ICatInformation ICatInformation_iface;
} ComCatMgrImpl;

/* static ComCatMgr instance */
static ComCatMgrImpl COMCAT_ComCatMgr =
{
    { &COMCAT_ICatRegister_Vtbl },
    { &COMCAT_ICatInformation_Vtbl }
};

struct class_categories
{
    ULONG   size;        /* total length, including structure itself */
    ULONG   impl_offset;
    ULONG   req_offset;
};

static HRESULT EnumCATEGORYINFO_Construct(LCID lcid, IEnumCATEGORYINFO **ret);
static HRESULT CLSIDEnumGUID_Construct(struct class_categories *class_categories, IEnumCLSID **ret);
static HRESULT CATIDEnumGUID_Construct(REFCLSID rclsid, LPCWSTR impl_req, IEnumCATID **ret);

/**********************************************************************
 * File-scope string constants
 */
static const WCHAR comcat_keyname[] = L"Component Categories";
static const WCHAR impl_keyname[] = L"Implemented Categories";
static const WCHAR req_keyname[] = L"Required Categories";

/**********************************************************************
 * COMCAT_RegisterClassCategories
 */
static HRESULT COMCAT_RegisterClassCategories(
    REFCLSID rclsid,
    LPCWSTR type,
    ULONG cCategories,
    const CATID *rgcatid)
{
    WCHAR keyname[CHARS_IN_GUID];
    HRESULT res;
    HKEY clsid_key, class_key, type_key;

    if (cCategories && rgcatid == NULL) return E_POINTER;

    /* Format the class key name. */
    res = StringFromGUID2(rclsid, keyname, CHARS_IN_GUID);
    if (FAILED(res)) return res;

    /* Create (or open) the CLSID key. */
    res = create_classes_key(HKEY_CLASSES_ROOT, L"CLSID", KEY_READ|KEY_WRITE, &clsid_key);
    if (res != ERROR_SUCCESS) return E_FAIL;

    /* Create (or open) the class key. */
    res = create_classes_key(clsid_key, keyname, KEY_READ|KEY_WRITE, &class_key);
    if (res == ERROR_SUCCESS) {
	/* Create (or open) the category type key. */
	res = create_classes_key(class_key, type, KEY_READ|KEY_WRITE, &type_key);
	if (res == ERROR_SUCCESS) {
	    for (; cCategories; --cCategories, ++rgcatid) {
		HKEY key;

		/* Format the category key name. */
		res = StringFromGUID2(rgcatid, keyname, CHARS_IN_GUID);
		if (FAILED(res)) continue;

		/* Do the register. */
		res = create_classes_key(type_key, keyname, KEY_READ|KEY_WRITE, &key);
		if (res == ERROR_SUCCESS) RegCloseKey(key);
	    }
	    res = S_OK;
	} else res = E_FAIL;
	RegCloseKey(class_key);
    } else res = E_FAIL;
    RegCloseKey(clsid_key);

    return res;
}

/**********************************************************************
 * COMCAT_UnRegisterClassCategories
 */
static HRESULT COMCAT_UnRegisterClassCategories(
    REFCLSID rclsid,
    LPCWSTR type,
    ULONG cCategories,
    const CATID *rgcatid)
{
    WCHAR keyname[68] = L"CLSID\\";
    HRESULT res;
    HKEY type_key;

    if (cCategories && rgcatid == NULL) return E_POINTER;

    /* Format the class category type key name. */
    res = StringFromGUID2(rclsid, keyname + 6, CHARS_IN_GUID);
    if (FAILED(res)) return res;
    keyname[44] = '\\';
    lstrcpyW(keyname + 45, type);

    /* Open the class category type key. */
    res = open_classes_key(HKEY_CLASSES_ROOT, keyname, KEY_READ|KEY_WRITE, &type_key);
    if (res != ERROR_SUCCESS) return E_FAIL;

    for (; cCategories; --cCategories, ++rgcatid) {
	/* Format the category key name. */
	res = StringFromGUID2(rgcatid, keyname, CHARS_IN_GUID);
	if (FAILED(res)) continue;

	/* Do the unregister. */
	RegDeleteKeyW(type_key, keyname);
    }
    RegCloseKey(type_key);

    return S_OK;
}

/**********************************************************************
 * COMCAT_GetCategoryDesc
 */
static HRESULT COMCAT_GetCategoryDesc(HKEY key, LCID lcid, PWCHAR pszDesc,
				      ULONG buf_wchars)
{
    WCHAR valname[5];
    HRESULT res;
    DWORD type, size = (buf_wchars - 1) * sizeof(WCHAR);

    if (pszDesc == NULL) return E_INVALIDARG;

    /* FIXME: lcid comparisons are more complex than this! */
    wsprintfW(valname, L"%lX", lcid);
    res = RegQueryValueExW(key, valname, 0, &type, (LPBYTE)pszDesc, &size);
    if (res != ERROR_SUCCESS || type != REG_SZ) {
	FIXME("Simplified lcid comparison\n");
	return CAT_E_NODESCRIPTION;
    }
    pszDesc[size / sizeof(WCHAR)] = 0;

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
    ULONG size;

    size = sizeof(struct class_categories) + ((impl_count + req_count)*CHARS_IN_GUID + 2)*sizeof(WCHAR);
    categories = HeapAlloc(GetProcessHeap(), 0, size);
    if (categories == NULL) return categories;

    categories->size = size;
    categories->impl_offset = sizeof(struct class_categories);
    categories->req_offset = categories->impl_offset + (impl_count*CHARS_IN_GUID + 1)*sizeof(WCHAR);

    strings = (WCHAR *)(categories + 1);
    while (impl_count--) {
	StringFromGUID2(impl_catids++, strings, CHARS_IN_GUID);
	strings += CHARS_IN_GUID;
    }
    *strings++ = 0;

    while (req_count--) {
	StringFromGUID2(req_catids++, strings, CHARS_IN_GUID);
	strings += CHARS_IN_GUID;
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
    const WCHAR *impl_strings, *req_strings;
    HKEY subkey;
    HRESULT res;
    DWORD index;
    LPCWSTR string;

    impl_strings = (WCHAR*)((BYTE*)categories + categories->impl_offset);
    req_strings  = (WCHAR*)((BYTE*)categories + categories->req_offset);

    /* Check that every given category is implemented by class. */
    if (*impl_strings) {
	res = open_classes_key(key, impl_keyname, KEY_READ, &subkey);
	if (res != ERROR_SUCCESS) return S_FALSE;
	for (string = impl_strings; *string; string += CHARS_IN_GUID) {
	    HKEY catkey;
	    res = open_classes_key(subkey, string, READ_CONTROL, &catkey);
	    if (res != ERROR_SUCCESS) {
		RegCloseKey(subkey);
		return S_FALSE;
	    }
	    RegCloseKey(catkey);
	}
	RegCloseKey(subkey);
    }

    /* Check that all categories required by class are given. */
    res = open_classes_key(key, req_keyname, KEY_READ, &subkey);
    if (res == ERROR_SUCCESS) {
	for (index = 0; ; ++index) {
	    WCHAR keyname[CHARS_IN_GUID];
	    DWORD size = CHARS_IN_GUID;

	    res = RegEnumKeyExW(subkey, index, keyname, &size,
				NULL, NULL, NULL, NULL);
	    if (res != ERROR_SUCCESS && res != ERROR_MORE_DATA) break;
	    if (size != CHARS_IN_GUID-1) continue; /* bogus catid in registry */
	    for (string = req_strings; *string; string += CHARS_IN_GUID)
		if (!wcsicmp(string, keyname)) break;
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
 * COMCAT_ICatRegister_QueryInterface
 */
static HRESULT WINAPI COMCAT_ICatRegister_QueryInterface(
    LPCATREGISTER iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    TRACE("%s\n",debugstr_guid(riid));

    if (ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_ICatRegister)) {
	*ppvObj = iface;
        ICatRegister_AddRef(iface);
	return S_OK;
    }

    if (IsEqualGUID(riid, &IID_ICatInformation)) {
        *ppvObj = &COMCAT_ComCatMgr.ICatInformation_iface;
        ICatRegister_AddRef(iface);
	return S_OK;
    }

    return E_NOINTERFACE;
}

/**********************************************************************
 * COMCAT_ICatRegister_AddRef
 */
static ULONG WINAPI COMCAT_ICatRegister_AddRef(LPCATREGISTER iface)
{
    return 2; /* non-heap based object */
}

/**********************************************************************
 * COMCAT_ICatRegister_Release
 */
static ULONG WINAPI COMCAT_ICatRegister_Release(LPCATREGISTER iface)
{
    return 1; /* non-heap based object */
}

/**********************************************************************
 * COMCAT_ICatRegister_RegisterCategories
 */
static HRESULT WINAPI COMCAT_ICatRegister_RegisterCategories(
    LPCATREGISTER iface,
    ULONG cCategories,
    CATEGORYINFO *rgci)
{
    HKEY comcat_key;
    HRESULT res;

    TRACE("\n");

    if (cCategories && rgci == NULL)
	return E_POINTER;

    /* Create (or open) the component categories key. */
    res = create_classes_key(HKEY_CLASSES_ROOT, comcat_keyname, KEY_READ|KEY_WRITE, &comcat_key);
    if (res != ERROR_SUCCESS) return E_FAIL;

    for (; cCategories; --cCategories, ++rgci)
    {
	WCHAR keyname[CHARS_IN_GUID];
	WCHAR valname[9];
	HKEY cat_key;

	/* Create (or open) the key for this category. */
	if (!StringFromGUID2(&rgci->catid, keyname, CHARS_IN_GUID)) continue;
	res = create_classes_key(comcat_key, keyname, KEY_READ|KEY_WRITE, &cat_key);
	if (res != ERROR_SUCCESS) continue;

	/* Set the value for this locale's description. */
	wsprintfW(valname, L"%lX", rgci->lcid);
        RegSetValueExW(cat_key, valname, 0, REG_SZ, (const BYTE*)rgci->szDescription,
		       (lstrlenW(rgci->szDescription) + 1) * sizeof(WCHAR));

	RegCloseKey(cat_key);
    }

    RegCloseKey(comcat_key);
    return S_OK;
}

/**********************************************************************
 * COMCAT_ICatRegister_UnRegisterCategories
 */
static HRESULT WINAPI COMCAT_ICatRegister_UnRegisterCategories(
    LPCATREGISTER iface,
    ULONG cCategories,
    CATID *rgcatid)
{
    HKEY comcat_key;
    HRESULT res;

    TRACE("\n");

    if (cCategories && rgcatid == NULL)
	return E_POINTER;

    /* Open the component categories key. */
    res = open_classes_key(HKEY_CLASSES_ROOT, comcat_keyname, KEY_READ|KEY_WRITE, &comcat_key);
    if (res != ERROR_SUCCESS) return E_FAIL;

    for (; cCategories; --cCategories, ++rgcatid) {
	WCHAR keyname[CHARS_IN_GUID];

	/* Delete the key for this category. */
	if (!StringFromGUID2(rgcatid, keyname, CHARS_IN_GUID)) continue;
	RegDeleteKeyW(comcat_key, keyname);
    }

    RegCloseKey(comcat_key);
    return S_OK;
}

/**********************************************************************
 * COMCAT_ICatRegister_RegisterClassImplCategories
 */
static HRESULT WINAPI COMCAT_ICatRegister_RegisterClassImplCategories(
    LPCATREGISTER iface,
    REFCLSID rclsid,
    ULONG cCategories,
    CATID *rgcatid)
{
    TRACE("\n");

    return COMCAT_RegisterClassCategories(
	rclsid,	impl_keyname, cCategories, rgcatid);
}

/**********************************************************************
 * COMCAT_ICatRegister_UnRegisterClassImplCategories
 */
static HRESULT WINAPI COMCAT_ICatRegister_UnRegisterClassImplCategories(
    LPCATREGISTER iface,
    REFCLSID rclsid,
    ULONG cCategories,
    CATID *rgcatid)
{
    TRACE("\n");

    return COMCAT_UnRegisterClassCategories(
	rclsid, impl_keyname, cCategories, rgcatid);
}

/**********************************************************************
 * COMCAT_ICatRegister_RegisterClassReqCategories
 */
static HRESULT WINAPI COMCAT_ICatRegister_RegisterClassReqCategories(
    LPCATREGISTER iface,
    REFCLSID rclsid,
    ULONG cCategories,
    CATID *rgcatid)
{
    TRACE("\n");

    return COMCAT_RegisterClassCategories(
	rclsid, req_keyname, cCategories, rgcatid);
}

/**********************************************************************
 * COMCAT_ICatRegister_UnRegisterClassReqCategories
 */
static HRESULT WINAPI COMCAT_ICatRegister_UnRegisterClassReqCategories(
    LPCATREGISTER iface,
    REFCLSID rclsid,
    ULONG cCategories,
    CATID *rgcatid)
{
    TRACE("\n");

    return COMCAT_UnRegisterClassCategories(
	rclsid, req_keyname, cCategories, rgcatid);
}

/**********************************************************************
 * COMCAT_ICatInformation_QueryInterface
 */
static HRESULT WINAPI COMCAT_ICatInformation_QueryInterface(
    LPCATINFORMATION iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    return ICatRegister_QueryInterface(&COMCAT_ComCatMgr.ICatRegister_iface, riid, ppvObj);
}

/**********************************************************************
 * COMCAT_ICatInformation_AddRef
 */
static ULONG WINAPI COMCAT_ICatInformation_AddRef(LPCATINFORMATION iface)
{
    return ICatRegister_AddRef(&COMCAT_ComCatMgr.ICatRegister_iface);
}

/**********************************************************************
 * COMCAT_ICatInformation_Release
 */
static ULONG WINAPI COMCAT_ICatInformation_Release(LPCATINFORMATION iface)
{
    return ICatRegister_Release(&COMCAT_ComCatMgr.ICatRegister_iface);
}

/**********************************************************************
 * COMCAT_ICatInformation_EnumCategories
 */
static HRESULT WINAPI COMCAT_ICatInformation_EnumCategories(
    LPCATINFORMATION iface,
    LCID lcid,
    IEnumCATEGORYINFO **ppenumCatInfo)
{
    TRACE("\n");

    if (ppenumCatInfo == NULL) return E_POINTER;

    return EnumCATEGORYINFO_Construct(lcid, ppenumCatInfo);
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
    WCHAR keyname[60] = L"Component Categories\\";
    HKEY key;
    HRESULT res;

    TRACE("CATID: %s LCID: %lx\n",debugstr_guid(rcatid), lcid);

    if (rcatid == NULL || ppszDesc == NULL) return E_INVALIDARG;

    /* Open the key for this category. */
    if (!StringFromGUID2(rcatid, keyname + 21, CHARS_IN_GUID)) return E_FAIL;
    res = open_classes_key(HKEY_CLASSES_ROOT, keyname, KEY_READ, &key);
    if (res != ERROR_SUCCESS) return CAT_E_CATIDNOEXIST;

    /* Allocate a sensible amount of memory for the description. */
    *ppszDesc = CoTaskMemAlloc(128 * sizeof(WCHAR));
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
    struct class_categories *categories;
    HRESULT hr;

    TRACE("\n");

	if (cImplemented == (ULONG)-1)
		cImplemented = 0;
	if (cRequired == (ULONG)-1)
		cRequired = 0;

    if (ppenumCLSID == NULL ||
	(cImplemented && rgcatidImpl == NULL) ||
	(cRequired && rgcatidReq == NULL)) return E_POINTER;

    categories = COMCAT_PrepareClassCategories(cImplemented, rgcatidImpl,
					       cRequired, rgcatidReq);
    if (categories == NULL) return E_OUTOFMEMORY;

    hr = CLSIDEnumGUID_Construct(categories, ppenumCLSID);
    if (FAILED(hr))
    {
	HeapFree(GetProcessHeap(), 0, categories);
	return hr;
    }

    return hr;
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
    WCHAR keyname[45] = L"CLSID\\";
    HRESULT res;
    struct class_categories *categories;
    HKEY key;

    if (TRACE_ON(ole)) {
	ULONG count;
	TRACE("CLSID: %s Implemented %lu\n",debugstr_guid(rclsid),cImplemented);
	for (count = 0; count < cImplemented; ++count)
	    TRACE("    %s\n",debugstr_guid(&rgcatidImpl[count]));
	TRACE("Required %lu\n",cRequired);
	for (count = 0; count < cRequired; ++count)
	    TRACE("    %s\n",debugstr_guid(&rgcatidReq[count]));
    }

    if ((cImplemented && rgcatidImpl == NULL) ||
	(cRequired && rgcatidReq == NULL)) return E_POINTER;

    res = StringFromGUID2(rclsid, keyname + 6, CHARS_IN_GUID);
    if (FAILED(res)) return res;

    categories = COMCAT_PrepareClassCategories(cImplemented, rgcatidImpl,
					       cRequired, rgcatidReq);
    if (categories == NULL) return E_OUTOFMEMORY;

    res = open_classes_key(HKEY_CLASSES_ROOT, keyname, KEY_READ, &key);
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
    TRACE("%s\n",debugstr_guid(rclsid));

    if (rclsid == NULL || ppenumCATID == NULL)
	return E_POINTER;

    return CATIDEnumGUID_Construct(rclsid, L"\\Implemented Categories", ppenumCATID);
}

/**********************************************************************
 * COMCAT_ICatInformation_EnumReqCategoriesOfClass
 */
static HRESULT WINAPI COMCAT_ICatInformation_EnumReqCategoriesOfClass(
    LPCATINFORMATION iface,
    REFCLSID rclsid,
    LPENUMCATID *ppenumCATID)
{
    TRACE("%s\n",debugstr_guid(rclsid));

    if (rclsid == NULL || ppenumCATID == NULL)
	return E_POINTER;

    return CATIDEnumGUID_Construct(rclsid, L"\\Required Categories", ppenumCATID);
}

/**********************************************************************
 * COMCAT_ICatRegister_Vtbl
 */
static const ICatRegisterVtbl COMCAT_ICatRegister_Vtbl =
{
    COMCAT_ICatRegister_QueryInterface,
    COMCAT_ICatRegister_AddRef,
    COMCAT_ICatRegister_Release,
    COMCAT_ICatRegister_RegisterCategories,
    COMCAT_ICatRegister_UnRegisterCategories,
    COMCAT_ICatRegister_RegisterClassImplCategories,
    COMCAT_ICatRegister_UnRegisterClassImplCategories,
    COMCAT_ICatRegister_RegisterClassReqCategories,
    COMCAT_ICatRegister_UnRegisterClassReqCategories
};


/**********************************************************************
 * COMCAT_ICatInformation_Vtbl
 */
static const ICatInformationVtbl COMCAT_ICatInformation_Vtbl =
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

HRESULT WINAPI ComCat_CreateInstance(IClassFactory *iface, IUnknown *pUnkOuter, REFIID riid, void **ppvObj)
{
    HRESULT res;
    TRACE("%s\n",debugstr_guid(riid));

    if (ppvObj == NULL) return E_POINTER;

    /* Don't support aggregation (Windows doesn't) */
    if (pUnkOuter != NULL) return CLASS_E_NOAGGREGATION;

    res = ICatRegister_QueryInterface(&COMCAT_ComCatMgr.ICatRegister_iface, riid, ppvObj);
    if (SUCCEEDED(res)) {
	return res;
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}

/**********************************************************************
 * IEnumCATEGORYINFO implementation
 *
 * This implementation is not thread-safe.  The manager itself is, but
 * I can't imagine a valid use of an enumerator in several threads.
 */
typedef struct
{
    IEnumCATEGORYINFO IEnumCATEGORYINFO_iface;
    LONG  ref;
    LCID  lcid;
    HKEY  key;
    DWORD next_index;
} IEnumCATEGORYINFOImpl;

static inline IEnumCATEGORYINFOImpl *impl_from_IEnumCATEGORYINFO(IEnumCATEGORYINFO *iface)
{
    return CONTAINING_RECORD(iface, IEnumCATEGORYINFOImpl, IEnumCATEGORYINFO_iface);
}

static ULONG WINAPI COMCAT_IEnumCATEGORYINFO_AddRef(IEnumCATEGORYINFO *iface)
{
    IEnumCATEGORYINFOImpl *This = impl_from_IEnumCATEGORYINFO(iface);

    TRACE("\n");

    return InterlockedIncrement(&This->ref);
}

static HRESULT WINAPI COMCAT_IEnumCATEGORYINFO_QueryInterface(
    IEnumCATEGORYINFO *iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    TRACE("%s\n",debugstr_guid(riid));

    if (ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
	IsEqualGUID(riid, &IID_IEnumCATEGORYINFO))
    {
        *ppvObj = iface;
	COMCAT_IEnumCATEGORYINFO_AddRef(iface);
	return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI COMCAT_IEnumCATEGORYINFO_Release(IEnumCATEGORYINFO *iface)
{
    IEnumCATEGORYINFOImpl *This = impl_from_IEnumCATEGORYINFO(iface);
    ULONG ref;

    TRACE("\n");

    ref = InterlockedDecrement(&This->ref);
    if (ref == 0) {
	if (This->key) RegCloseKey(This->key);
	HeapFree(GetProcessHeap(), 0, This);
	return 0;
    }
    return ref;
}

static HRESULT WINAPI COMCAT_IEnumCATEGORYINFO_Next(
    IEnumCATEGORYINFO *iface,
    ULONG celt,
    CATEGORYINFO *rgelt,
    ULONG *pceltFetched)
{
    IEnumCATEGORYINFOImpl *This = impl_from_IEnumCATEGORYINFO(iface);
    ULONG fetched = 0;

    TRACE("\n");

    if (rgelt == NULL) return E_POINTER;

    if (This->key) while (fetched < celt) {
	LSTATUS res;
	HRESULT hr;
	WCHAR catid[CHARS_IN_GUID];
	DWORD cName = CHARS_IN_GUID;
	HKEY subkey;

	res = RegEnumKeyExW(This->key, This->next_index, catid, &cName,
			    NULL, NULL, NULL, NULL);
	if (res != ERROR_SUCCESS && res != ERROR_MORE_DATA) break;
	++(This->next_index);

	hr = CLSIDFromString(catid, &rgelt->catid);
	if (FAILED(hr)) continue;

	res = open_classes_key(This->key, catid, KEY_READ, &subkey);
	if (res != ERROR_SUCCESS) continue;

	hr = COMCAT_GetCategoryDesc(subkey, This->lcid,
				    rgelt->szDescription, 128);
	RegCloseKey(subkey);
	if (FAILED(hr)) continue;

	rgelt->lcid = This->lcid;
	++fetched;
	++rgelt;
    }

    if (pceltFetched) *pceltFetched = fetched;
    return fetched == celt ? S_OK : S_FALSE;
}

static HRESULT WINAPI COMCAT_IEnumCATEGORYINFO_Skip(
    IEnumCATEGORYINFO *iface,
    ULONG celt)
{
    IEnumCATEGORYINFOImpl *This = impl_from_IEnumCATEGORYINFO(iface);

    TRACE("\n");

    This->next_index += celt;
    /* This should return S_FALSE when there aren't celt elems to skip. */
    return S_OK;
}

static HRESULT WINAPI COMCAT_IEnumCATEGORYINFO_Reset(IEnumCATEGORYINFO *iface)
{
    IEnumCATEGORYINFOImpl *This = impl_from_IEnumCATEGORYINFO(iface);

    TRACE("\n");

    This->next_index = 0;
    return S_OK;
}

static HRESULT WINAPI COMCAT_IEnumCATEGORYINFO_Clone(
    IEnumCATEGORYINFO *iface,
    IEnumCATEGORYINFO **ppenum)
{
    IEnumCATEGORYINFOImpl *This = impl_from_IEnumCATEGORYINFO(iface);
    IEnumCATEGORYINFOImpl *new_this;

    TRACE("\n");

    if (ppenum == NULL) return E_POINTER;

    new_this = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IEnumCATEGORYINFOImpl));
    if (new_this == NULL) return E_OUTOFMEMORY;

    new_this->IEnumCATEGORYINFO_iface = This->IEnumCATEGORYINFO_iface;
    new_this->ref = 1;
    new_this->lcid = This->lcid;
    /* FIXME: could we more efficiently use DuplicateHandle? */
    open_classes_key(HKEY_CLASSES_ROOT, comcat_keyname, KEY_READ, &new_this->key);
    new_this->next_index = This->next_index;

    *ppenum = &new_this->IEnumCATEGORYINFO_iface;
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

static HRESULT EnumCATEGORYINFO_Construct(LCID lcid, IEnumCATEGORYINFO **ret)
{
    IEnumCATEGORYINFOImpl *This;

    *ret = NULL;

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IEnumCATEGORYINFOImpl));
    if (!This) return E_OUTOFMEMORY;

    This->IEnumCATEGORYINFO_iface.lpVtbl = &COMCAT_IEnumCATEGORYINFO_Vtbl;
    This->ref = 1;
    This->lcid = lcid;
    open_classes_key(HKEY_CLASSES_ROOT, comcat_keyname, KEY_READ, &This->key);

    *ret = &This->IEnumCATEGORYINFO_iface;
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
    IEnumGUID IEnumGUID_iface;
    LONG  ref;
    struct class_categories *categories;
    HKEY  key;
    DWORD next_index;
} CLSID_IEnumGUIDImpl;

static inline CLSID_IEnumGUIDImpl *impl_from_IEnumCLSID(IEnumGUID *iface)
{
    return CONTAINING_RECORD(iface, CLSID_IEnumGUIDImpl, IEnumGUID_iface);
}

static HRESULT WINAPI CLSIDEnumGUID_QueryInterface(
    IEnumGUID *iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    TRACE("%s\n",debugstr_guid(riid));

    if (ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
	IsEqualGUID(riid, &IID_IEnumGUID))
    {
        *ppvObj = iface;
	IEnumGUID_AddRef(iface);
	return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI CLSIDEnumGUID_AddRef(IEnumGUID *iface)
{
    CLSID_IEnumGUIDImpl *This = impl_from_IEnumCLSID(iface);
    TRACE("\n");

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI CLSIDEnumGUID_Release(IEnumGUID *iface)
{
    CLSID_IEnumGUIDImpl *This = impl_from_IEnumCLSID(iface);
    ULONG ref;

    TRACE("\n");

    ref = InterlockedDecrement(&This->ref);
    if (ref == 0) {
	if (This->key) RegCloseKey(This->key);
        HeapFree(GetProcessHeap(), 0, This->categories);
	HeapFree(GetProcessHeap(), 0, This);
	return 0;
    }
    return ref;
}

static HRESULT WINAPI CLSIDEnumGUID_Next(
    IEnumGUID *iface,
    ULONG celt,
    GUID *rgelt,
    ULONG *pceltFetched)
{
    CLSID_IEnumGUIDImpl *This = impl_from_IEnumCLSID(iface);
    ULONG fetched = 0;

    TRACE("\n");

    if (rgelt == NULL) return E_POINTER;

    if (This->key) while (fetched < celt) {
	LSTATUS res;
	HRESULT hr;
	WCHAR clsid[CHARS_IN_GUID];
	DWORD cName = CHARS_IN_GUID;
	HKEY subkey;

	res = RegEnumKeyExW(This->key, This->next_index, clsid, &cName,
			    NULL, NULL, NULL, NULL);
	if (res != ERROR_SUCCESS && res != ERROR_MORE_DATA) break;
	++(This->next_index);

	hr = CLSIDFromString(clsid, rgelt);
	if (FAILED(hr)) continue;

	res = open_classes_key(This->key, clsid, KEY_READ, &subkey);
	if (res != ERROR_SUCCESS) continue;

	hr = COMCAT_IsClassOfCategories(subkey, This->categories);
	RegCloseKey(subkey);
	if (hr != S_OK) continue;

	++fetched;
	++rgelt;
    }

    if (pceltFetched) *pceltFetched = fetched;
    return fetched == celt ? S_OK : S_FALSE;
}

static HRESULT WINAPI CLSIDEnumGUID_Skip(
    IEnumGUID *iface,
    ULONG celt)
{
    CLSID_IEnumGUIDImpl *This = impl_from_IEnumCLSID(iface);

    TRACE("\n");

    This->next_index += celt;
    FIXME("Never returns S_FALSE\n");
    return S_OK;
}

static HRESULT WINAPI CLSIDEnumGUID_Reset(IEnumGUID *iface)
{
    CLSID_IEnumGUIDImpl *This = impl_from_IEnumCLSID(iface);

    TRACE("\n");

    This->next_index = 0;
    return S_OK;
}

static HRESULT WINAPI CLSIDEnumGUID_Clone(
    IEnumGUID *iface,
    IEnumGUID **ppenum)
{
    CLSID_IEnumGUIDImpl *This = impl_from_IEnumCLSID(iface);
    CLSID_IEnumGUIDImpl *cloned;

    TRACE("(%p)->(%p)\n", This, ppenum);

    if (ppenum == NULL) return E_POINTER;

    *ppenum = NULL;

    cloned = HeapAlloc(GetProcessHeap(), 0, sizeof(CLSID_IEnumGUIDImpl));
    if (cloned == NULL) return E_OUTOFMEMORY;

    cloned->IEnumGUID_iface.lpVtbl = This->IEnumGUID_iface.lpVtbl;
    cloned->ref = 1;

    cloned->categories = HeapAlloc(GetProcessHeap(), 0, This->categories->size);
    if (cloned->categories == NULL) {
	HeapFree(GetProcessHeap(), 0, cloned);
	return E_OUTOFMEMORY;
    }
    memcpy(cloned->categories, This->categories, This->categories->size);

    cloned->key = NULL;
    open_classes_key(HKEY_CLASSES_ROOT, L"CLSID", KEY_READ, &cloned->key);
    cloned->next_index = This->next_index;

    *ppenum = &cloned->IEnumGUID_iface;
    return S_OK;
}

static const IEnumGUIDVtbl CLSIDEnumGUIDVtbl =
{
    CLSIDEnumGUID_QueryInterface,
    CLSIDEnumGUID_AddRef,
    CLSIDEnumGUID_Release,
    CLSIDEnumGUID_Next,
    CLSIDEnumGUID_Skip,
    CLSIDEnumGUID_Reset,
    CLSIDEnumGUID_Clone
};

static HRESULT CLSIDEnumGUID_Construct(struct class_categories *categories, IEnumCLSID **ret)
{
    CLSID_IEnumGUIDImpl *This;

    *ret = NULL;

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CLSID_IEnumGUIDImpl));
    if (!This) return E_OUTOFMEMORY;

    This->IEnumGUID_iface.lpVtbl = &CLSIDEnumGUIDVtbl;
    This->ref = 1;
    This->categories = categories;
    open_classes_key(HKEY_CLASSES_ROOT, L"CLSID", KEY_READ, &This->key);

    *ret = &This->IEnumGUID_iface;

    return S_OK;
}

/**********************************************************************
 * CategoriesOfClass IEnumCATID (IEnumGUID) implementation
 *
 * This implementation is not thread-safe.  The manager itself is, but
 * I can't imagine a valid use of an enumerator in several threads.
 */
typedef struct
{
    IEnumGUID IEnumGUID_iface;
    LONG  ref;
    WCHAR keyname[68];
    HKEY  key;
    DWORD next_index;
} CATID_IEnumGUIDImpl;

static inline CATID_IEnumGUIDImpl *impl_from_IEnumCATID(IEnumGUID *iface)
{
    return CONTAINING_RECORD(iface, CATID_IEnumGUIDImpl, IEnumGUID_iface);
}

static HRESULT WINAPI CATIDEnumGUID_QueryInterface(
    IEnumGUID *iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    TRACE("%s\n",debugstr_guid(riid));

    if (ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
	IsEqualGUID(riid, &IID_IEnumGUID))
    {
        *ppvObj = iface;
	IEnumGUID_AddRef(iface);
	return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI CATIDEnumGUID_AddRef(IEnumGUID *iface)
{
    CATID_IEnumGUIDImpl *This = impl_from_IEnumCATID(iface);
    TRACE("\n");

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI CATIDEnumGUID_Release(IEnumGUID *iface)
{
    CATID_IEnumGUIDImpl *This = impl_from_IEnumCATID(iface);
    ULONG ref;

    TRACE("\n");

    ref = InterlockedDecrement(&This->ref);
    if (ref == 0) {
	if (This->key) RegCloseKey(This->key);
	HeapFree(GetProcessHeap(), 0, This);
	return 0;
    }
    return ref;
}

static HRESULT WINAPI CATIDEnumGUID_Next(
    IEnumGUID *iface,
    ULONG celt,
    GUID *rgelt,
    ULONG *pceltFetched)
{
    CATID_IEnumGUIDImpl *This = impl_from_IEnumCATID(iface);
    ULONG fetched = 0;

    TRACE("\n");

    if (rgelt == NULL) return E_POINTER;

    if (This->key) while (fetched < celt) {
	LSTATUS res;
	HRESULT hr;
	WCHAR catid[CHARS_IN_GUID];
	DWORD cName = CHARS_IN_GUID;

	res = RegEnumKeyExW(This->key, This->next_index, catid, &cName,
			    NULL, NULL, NULL, NULL);
	if (res != ERROR_SUCCESS && res != ERROR_MORE_DATA) break;
	++(This->next_index);

	hr = CLSIDFromString(catid, rgelt);
	if (FAILED(hr)) continue;

	++fetched;
	++rgelt;
    }

    if (pceltFetched) *pceltFetched = fetched;
    return fetched == celt ? S_OK : S_FALSE;
}

static HRESULT WINAPI CATIDEnumGUID_Skip(
    IEnumGUID *iface,
    ULONG celt)
{
    CATID_IEnumGUIDImpl *This = impl_from_IEnumCATID(iface);

    TRACE("\n");

    This->next_index += celt;
    FIXME("Never returns S_FALSE\n");
    return S_OK;
}

static HRESULT WINAPI CATIDEnumGUID_Reset(IEnumGUID *iface)
{
    CATID_IEnumGUIDImpl *This = impl_from_IEnumCATID(iface);

    TRACE("\n");

    This->next_index = 0;
    return S_OK;
}

static HRESULT WINAPI CATIDEnumGUID_Clone(
    IEnumGUID *iface,
    IEnumGUID **ppenum)
{
    CATID_IEnumGUIDImpl *This = impl_from_IEnumCATID(iface);
    CATID_IEnumGUIDImpl *new_this;

    TRACE("\n");

    if (ppenum == NULL) return E_POINTER;

    new_this = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CATID_IEnumGUIDImpl));
    if (new_this == NULL) return E_OUTOFMEMORY;

    new_this->IEnumGUID_iface.lpVtbl = This->IEnumGUID_iface.lpVtbl;
    new_this->ref = 1;
    lstrcpyW(new_this->keyname, This->keyname);
    /* FIXME: could we more efficiently use DuplicateHandle? */
    open_classes_key(HKEY_CLASSES_ROOT, new_this->keyname, KEY_READ, &new_this->key);
    new_this->next_index = This->next_index;

    *ppenum = &new_this->IEnumGUID_iface;
    return S_OK;
}

static const IEnumGUIDVtbl CATIDEnumGUIDVtbl =
{
    CATIDEnumGUID_QueryInterface,
    CATIDEnumGUID_AddRef,
    CATIDEnumGUID_Release,
    CATIDEnumGUID_Next,
    CATIDEnumGUID_Skip,
    CATIDEnumGUID_Reset,
    CATIDEnumGUID_Clone
};

static HRESULT CATIDEnumGUID_Construct(REFCLSID rclsid, LPCWSTR postfix, IEnumGUID **ret)
{
    WCHAR keyname[100], clsidW[CHARS_IN_GUID];
    CATID_IEnumGUIDImpl *This;

    *ret = NULL;

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CATID_IEnumGUIDImpl));
    if (!This) return E_OUTOFMEMORY;

    StringFromGUID2(rclsid, clsidW, CHARS_IN_GUID);

    This->IEnumGUID_iface.lpVtbl = &CATIDEnumGUIDVtbl;
    This->ref = 1;
    lstrcpyW(keyname, L"CLSID\\");
    lstrcatW(keyname, clsidW);
    lstrcatW(keyname, postfix);

    open_classes_key(HKEY_CLASSES_ROOT, keyname, KEY_READ, &This->key);

    *ret = &This->IEnumGUID_iface;
    return S_OK;
}
