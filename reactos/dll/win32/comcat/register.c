/*
 *	ComCatMgr ICatRegister implementation for comcat.dll
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

#include "comcat_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/**********************************************************************
 * File-scope string constants
 */
static const WCHAR comcat_keyname[21] = {
    'C', 'o', 'm', 'p', 'o', 'n', 'e', 'n', 't', ' ', 'C', 'a',
    't', 'e', 'g', 'o', 'r', 'i', 'e', 's', 0 };
static const WCHAR impl_keyname[23] = {
    'I', 'm', 'p', 'l', 'e', 'm', 'e', 'n',
    't', 'e', 'd', ' ', 'C', 'a', 't', 'e',
    'g', 'o', 'r', 'i', 'e', 's', 0 };
static const WCHAR req_keyname[20] = {
    'R', 'e', 'q', 'u', 'i', 'r', 'e', 'd',
    ' ', 'C', 'a', 't', 'e', 'g', 'o', 'r',
    'i', 'e', 's', 0 };

static HRESULT COMCAT_RegisterClassCategories(
    REFCLSID rclsid, LPCWSTR type,
    ULONG cCategories, const CATID *rgcatid);
static HRESULT COMCAT_UnRegisterClassCategories(
    REFCLSID rclsid, LPCWSTR type,
    ULONG cCategories, const CATID *rgcatid);

/**********************************************************************
 * COMCAT_ICatRegister_QueryInterface
 */
static HRESULT WINAPI COMCAT_ICatRegister_QueryInterface(
    LPCATREGISTER iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    ICOM_THIS_MULTI(ComCatMgrImpl, regVtbl, iface);
    TRACE("\n\tIID:\t%s\n",debugstr_guid(riid));

    if (This == NULL || ppvObj == NULL) return E_POINTER;

    return IUnknown_QueryInterface((LPUNKNOWN)&This->unkVtbl, riid, ppvObj);
}

/**********************************************************************
 * COMCAT_ICatRegister_AddRef
 */
static ULONG WINAPI COMCAT_ICatRegister_AddRef(LPCATREGISTER iface)
{
    ICOM_THIS_MULTI(ComCatMgrImpl, regVtbl, iface);
    TRACE("\n");

    if (This == NULL) return E_POINTER;

    return IUnknown_AddRef((LPUNKNOWN)&This->unkVtbl);
}

/**********************************************************************
 * COMCAT_ICatRegister_Release
 */
static ULONG WINAPI COMCAT_ICatRegister_Release(LPCATREGISTER iface)
{
    ICOM_THIS_MULTI(ComCatMgrImpl, regVtbl, iface);
    TRACE("\n");

    if (This == NULL) return E_POINTER;

    return IUnknown_Release((LPUNKNOWN)&This->unkVtbl);
}

/**********************************************************************
 * COMCAT_ICatRegister_RegisterCategories
 */
static HRESULT WINAPI COMCAT_ICatRegister_RegisterCategories(
    LPCATREGISTER iface,
    ULONG cCategories,
    CATEGORYINFO *rgci)
{
/*     ICOM_THIS_MULTI(ComCatMgrImpl, regVtbl, iface); */
    HKEY comcat_key;
    HRESULT res;

    TRACE("\n");

    if (iface == NULL || (cCategories && rgci == NULL))
	return E_POINTER;

    /* Create (or open) the component categories key. */
    res = RegCreateKeyExW(HKEY_CLASSES_ROOT, comcat_keyname, 0, NULL, 0,
			  KEY_READ | KEY_WRITE, NULL, &comcat_key, NULL);
    if (res != ERROR_SUCCESS) return E_FAIL;

    for (; cCategories; --cCategories, ++rgci) {
	static const WCHAR fmt[] = { '%', 'l', 'X', 0 };
	WCHAR keyname[39];
	WCHAR valname[9];
	HKEY cat_key;

	/* Create (or open) the key for this category. */
	if (!StringFromGUID2(&rgci->catid, keyname, 39)) continue;
	res = RegCreateKeyExW(comcat_key, keyname, 0, NULL, 0,
			      KEY_READ | KEY_WRITE, NULL, &cat_key, NULL);
	if (res != ERROR_SUCCESS) continue;

	/* Set the value for this locale's description. */
	wsprintfW(valname, fmt, rgci->lcid);
	RegSetValueExW(cat_key, valname, 0, REG_SZ,
		       (CONST BYTE*)(rgci->szDescription),
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
/*     ICOM_THIS_MULTI(ComCatMgrImpl, regVtbl, iface); */
    HKEY comcat_key;
    HRESULT res;

    TRACE("\n");

    if (iface == NULL || (cCategories && rgcatid == NULL))
	return E_POINTER;

    /* Open the component categories key. */
    res = RegOpenKeyExW(HKEY_CLASSES_ROOT, comcat_keyname, 0,
			KEY_READ | KEY_WRITE, &comcat_key);
    if (res != ERROR_SUCCESS) return E_FAIL;

    for (; cCategories; --cCategories, ++rgcatid) {
	WCHAR keyname[39];

	/* Delete the key for this category. */
	if (!StringFromGUID2(rgcatid, keyname, 39)) continue;
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
/*     ICOM_THIS_MULTI(ComCatMgrImpl, regVtbl, iface); */
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
/*     ICOM_THIS_MULTI(ComCatMgrImpl, regVtbl, iface); */
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
/*     ICOM_THIS_MULTI(ComCatMgrImpl, regVtbl, iface); */
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
/*     ICOM_THIS_MULTI(ComCatMgrImpl, regVtbl, iface); */
    TRACE("\n");

    return COMCAT_UnRegisterClassCategories(
	rclsid, req_keyname, cCategories, rgcatid);
}

/**********************************************************************
 * COMCAT_ICatRegister_Vtbl
 */
const ICatRegisterVtbl COMCAT_ICatRegister_Vtbl =
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
 * COMCAT_RegisterClassCategories
 */
static HRESULT COMCAT_RegisterClassCategories(
    REFCLSID rclsid,
    LPCWSTR type,
    ULONG cCategories,
    const CATID *rgcatid)
{
    WCHAR keyname[39];
    HRESULT res;
    HKEY clsid_key, class_key, type_key;

    if (cCategories && rgcatid == NULL) return E_POINTER;

    /* Format the class key name. */
    res = StringFromGUID2(rclsid, keyname, 39);
    if (FAILED(res)) return res;

    /* Create (or open) the CLSID key. */
    res = RegCreateKeyExW(HKEY_CLASSES_ROOT, clsid_keyname, 0, NULL, 0,
			  KEY_READ | KEY_WRITE, NULL, &clsid_key, NULL);
    if (res != ERROR_SUCCESS) return E_FAIL;

    /* Create (or open) the class key. */
    res = RegCreateKeyExW(clsid_key, keyname, 0, NULL, 0,
			  KEY_READ | KEY_WRITE, NULL, &class_key, NULL);
    if (res == ERROR_SUCCESS) {
	/* Create (or open) the category type key. */
	res = RegCreateKeyExW(class_key, type, 0, NULL, 0,
			      KEY_READ | KEY_WRITE, NULL, &type_key, NULL);
	if (res == ERROR_SUCCESS) {
	    for (; cCategories; --cCategories, ++rgcatid) {
		HKEY key;

		/* Format the category key name. */
		res = StringFromGUID2(rgcatid, keyname, 39);
		if (FAILED(res)) continue;

		/* Do the register. */
		res = RegCreateKeyExW(type_key, keyname, 0, NULL, 0,
				      KEY_READ | KEY_WRITE, NULL, &key, NULL);
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
    WCHAR keyname[68] = { 'C', 'L', 'S', 'I', 'D', '\\' };
    HRESULT res;
    HKEY type_key;

    if (cCategories && rgcatid == NULL) return E_POINTER;

    /* Format the class category type key name. */
    res = StringFromGUID2(rclsid, keyname + 6, 39);
    if (FAILED(res)) return res;
    keyname[44] = '\\';
    lstrcpyW(keyname + 45, type);

    /* Open the class category type key. */
    res = RegOpenKeyExW(HKEY_CLASSES_ROOT, keyname, 0,
			KEY_READ | KEY_WRITE, &type_key);
    if (res != ERROR_SUCCESS) return E_FAIL;

    for (; cCategories; --cCategories, ++rgcatid) {
	/* Format the category key name. */
	res = StringFromGUID2(rgcatid, keyname, 39);
	if (FAILED(res)) continue;

	/* Do the unregister. */
	RegDeleteKeyW(type_key, keyname);
    }
    RegCloseKey(type_key);

    return S_OK;
}
