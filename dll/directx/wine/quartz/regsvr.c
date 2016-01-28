/*
 *	self-registerable dll functions for quartz.dll
 *
 * Copyright (C) 2003 John K. Hohm
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

#include "quartz_private.h"

/*
 * Near the bottom of this file are the exported DllRegisterServer and
 * DllUnregisterServer, which make all this worthwhile.
 */

/***********************************************************************
 *		interface for self-registering
 */
struct regsvr_interface
{
    IID const *iid;		/* NULL for end of list */
    LPCSTR name;		/* can be NULL to omit */
    IID const *base_iid;	/* can be NULL to omit */
    int num_methods;		/* can be <0 to omit */
    CLSID const *ps_clsid;	/* can be NULL to omit */
    CLSID const *ps_clsid32;	/* can be NULL to omit */
};

static HRESULT register_interfaces(struct regsvr_interface const *list);
static HRESULT unregister_interfaces(struct regsvr_interface const *list);

struct regsvr_coclass
{
    CLSID const *clsid;		/* NULL for end of list */
    LPCSTR name;		/* can be NULL to omit */
    LPCSTR ips;			/* can be NULL to omit */
    LPCSTR ips32;		/* can be NULL to omit */
    LPCSTR ips32_tmodel;	/* can be NULL to omit */
    LPCSTR progid;		/* can be NULL to omit */
    LPCSTR viprogid;		/* can be NULL to omit */
    LPCSTR progid_extra;	/* can be NULL to omit */
};

static HRESULT register_coclasses(struct regsvr_coclass const *list);
static HRESULT unregister_coclasses(struct regsvr_coclass const *list);

struct regsvr_mediatype_parsing
{
    CLSID const *majortype;	/* NULL for end of list */
    CLSID const *subtype;
    LPCSTR line[11];		/* NULL for end of list */
};

static HRESULT register_mediatypes_parsing(struct regsvr_mediatype_parsing const *list);
static HRESULT unregister_mediatypes_parsing(struct regsvr_mediatype_parsing const *list);

struct regsvr_mediatype_extension
{
    CLSID const *majortype;	/* NULL for end of list */
    CLSID const *subtype;
    LPCSTR extension;
};

struct mediatype
{
    CLSID const *majortype;	/* NULL for end of list */
    CLSID const *subtype;
    DWORD fourcc;
};

struct pin
{
    DWORD flags;		/* 0xFFFFFFFF for end of list */
    struct mediatype mediatypes[11];
};

struct regsvr_filter
{
    CLSID const *clsid;		/* NULL for end of list */
    CLSID const *category;
    WCHAR name[50];
    DWORD merit;
    struct pin pins[11];
};

static HRESULT register_mediatypes_extension(struct regsvr_mediatype_extension const *list);
static HRESULT unregister_mediatypes_extension(struct regsvr_mediatype_extension const *list);

static HRESULT register_filters(struct regsvr_filter const *list);
static HRESULT unregister_filters(struct regsvr_filter const *list);

/***********************************************************************
 *		static string constants
 */
static const WCHAR interface_keyname[] = {
    'I', 'n', 't', 'e', 'r', 'f', 'a', 'c', 'e', 0 };
static const WCHAR base_ifa_keyname[] = {
    'B', 'a', 's', 'e', 'I', 'n', 't', 'e', 'r', 'f', 'a', 'c',
    'e', 0 };
static const WCHAR num_methods_keyname[] = {
    'N', 'u', 'm', 'M', 'e', 't', 'h', 'o', 'd', 's', 0 };
static const WCHAR ps_clsid_keyname[] = {
    'P', 'r', 'o', 'x', 'y', 'S', 't', 'u', 'b', 'C', 'l', 's',
    'i', 'd', 0 };
static const WCHAR ps_clsid32_keyname[] = {
    'P', 'r', 'o', 'x', 'y', 'S', 't', 'u', 'b', 'C', 'l', 's',
    'i', 'd', '3', '2', 0 };
static const WCHAR clsid_keyname[] = {
    'C', 'L', 'S', 'I', 'D', 0 };
static const WCHAR curver_keyname[] = {
    'C', 'u', 'r', 'V', 'e', 'r', 0 };
static const WCHAR ips_keyname[] = {
    'I', 'n', 'P', 'r', 'o', 'c', 'S', 'e', 'r', 'v', 'e', 'r',
    0 };
static const WCHAR ips32_keyname[] = {
    'I', 'n', 'P', 'r', 'o', 'c', 'S', 'e', 'r', 'v', 'e', 'r',
    '3', '2', 0 };
static const WCHAR progid_keyname[] = {
    'P', 'r', 'o', 'g', 'I', 'D', 0 };
static const WCHAR viprogid_keyname[] = {
    'V', 'e', 'r', 's', 'i', 'o', 'n', 'I', 'n', 'd', 'e', 'p',
    'e', 'n', 'd', 'e', 'n', 't', 'P', 'r', 'o', 'g', 'I', 'D',
    0 };
static const char tmodel_valuename[] = "ThreadingModel";
static const WCHAR mediatype_name[] = {
    'M', 'e', 'd', 'i', 'a', ' ', 'T', 'y', 'p', 'e', 0 };
static const WCHAR subtype_valuename[] = {
    'S', 'u', 'b', 't', 'y', 'p', 'e', 0 };
static const WCHAR sourcefilter_valuename[] = {
    'S', 'o', 'u', 'r', 'c', 'e', ' ', 'F', 'i', 'l', 't', 'e', 'r', 0 };
static const WCHAR extensions_keyname[] = {
    'E', 'x', 't', 'e', 'n', 's', 'i', 'o', 'n', 's', 0 };

/***********************************************************************
 *		static helper functions
 */
static LONG register_key_guid(HKEY base, WCHAR const *name, GUID const *guid);
static LONG register_key_defvalueW(HKEY base, WCHAR const *name,
				   WCHAR const *value);
static LONG register_key_defvalueA(HKEY base, WCHAR const *name,
				   char const *value);
static LONG register_progid(WCHAR const *clsid,
			    char const *progid, char const *curver_progid,
			    char const *name, char const *extra);

/***********************************************************************
 *		register_interfaces
 */
static HRESULT register_interfaces(struct regsvr_interface const *list)
{
    LONG res = ERROR_SUCCESS;
    HKEY interface_key;

    res = RegCreateKeyExW(HKEY_CLASSES_ROOT, interface_keyname, 0, NULL, 0,
			  KEY_READ | KEY_WRITE, NULL, &interface_key, NULL);
    if (res != ERROR_SUCCESS) goto error_return;

    for (; res == ERROR_SUCCESS && list->iid; ++list) {
	WCHAR buf[39];
	HKEY iid_key;

	StringFromGUID2(list->iid, buf, 39);
	res = RegCreateKeyExW(interface_key, buf, 0, NULL, 0,
			      KEY_READ | KEY_WRITE, NULL, &iid_key, NULL);
	if (res != ERROR_SUCCESS) goto error_close_interface_key;

	if (list->name) {
            res = RegSetValueExA(iid_key, NULL, 0, REG_SZ, (const BYTE*)list->name,
				 strlen(list->name) + 1);
	    if (res != ERROR_SUCCESS) goto error_close_iid_key;
	}

	if (list->base_iid) {
	    res = register_key_guid(iid_key, base_ifa_keyname, list->base_iid);
	    if (res != ERROR_SUCCESS) goto error_close_iid_key;
	}

	if (0 <= list->num_methods) {
	    static const WCHAR fmt[] = { '%', 'd', 0 };
	    HKEY key;

	    res = RegCreateKeyExW(iid_key, num_methods_keyname, 0, NULL, 0,
				  KEY_READ | KEY_WRITE, NULL, &key, NULL);
	    if (res != ERROR_SUCCESS) goto error_close_iid_key;

	    sprintfW(buf, fmt, list->num_methods);
            res = RegSetValueExW(key, NULL, 0, REG_SZ, (const BYTE*)buf,
				 (lstrlenW(buf) + 1) * sizeof(WCHAR));
	    RegCloseKey(key);

	    if (res != ERROR_SUCCESS) goto error_close_iid_key;
	}

	if (list->ps_clsid) {
	    res = register_key_guid(iid_key, ps_clsid_keyname, list->ps_clsid);
	    if (res != ERROR_SUCCESS) goto error_close_iid_key;
	}

	if (list->ps_clsid32) {
	    res = register_key_guid(iid_key, ps_clsid32_keyname, list->ps_clsid32);
	    if (res != ERROR_SUCCESS) goto error_close_iid_key;
	}

    error_close_iid_key:
	RegCloseKey(iid_key);
    }

error_close_interface_key:
    RegCloseKey(interface_key);
error_return:
    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
}

/***********************************************************************
 *		unregister_interfaces
 */
static HRESULT unregister_interfaces(struct regsvr_interface const *list)
{
    LONG res = ERROR_SUCCESS;
    HKEY interface_key;

    res = RegOpenKeyExW(HKEY_CLASSES_ROOT, interface_keyname, 0,
			KEY_READ | KEY_WRITE, &interface_key);
    if (res == ERROR_FILE_NOT_FOUND) return S_OK;
    if (res != ERROR_SUCCESS) goto error_return;

    for (; res == ERROR_SUCCESS && list->iid; ++list) {
	WCHAR buf[39];

	StringFromGUID2(list->iid, buf, 39);
	res = RegDeleteTreeW(interface_key, buf);
	if (res == ERROR_FILE_NOT_FOUND) res = ERROR_SUCCESS;
    }

    RegCloseKey(interface_key);
error_return:
    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
}

/***********************************************************************
 *		register_coclasses
 */
static HRESULT register_coclasses(struct regsvr_coclass const *list)
{
    LONG res = ERROR_SUCCESS;
    HKEY coclass_key;

    res = RegCreateKeyExW(HKEY_CLASSES_ROOT, clsid_keyname, 0, NULL, 0,
			  KEY_READ | KEY_WRITE, NULL, &coclass_key, NULL);
    if (res != ERROR_SUCCESS) goto error_return;

    for (; res == ERROR_SUCCESS && list->clsid; ++list) {
	WCHAR buf[39];
	HKEY clsid_key;

	StringFromGUID2(list->clsid, buf, 39);
	res = RegCreateKeyExW(coclass_key, buf, 0, NULL, 0,
			      KEY_READ | KEY_WRITE, NULL, &clsid_key, NULL);
	if (res != ERROR_SUCCESS) goto error_close_coclass_key;

	if (list->name) {
            res = RegSetValueExA(clsid_key, NULL, 0, REG_SZ, (const BYTE*)list->name,
				 strlen(list->name) + 1);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
	}

	if (list->ips) {
	    res = register_key_defvalueA(clsid_key, ips_keyname, list->ips);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
	}

	if (list->ips32) {
	    HKEY ips32_key;

	    res = RegCreateKeyExW(clsid_key, ips32_keyname, 0, NULL, 0,
				  KEY_READ | KEY_WRITE, NULL,
				  &ips32_key, NULL);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;

            res = RegSetValueExA(ips32_key, NULL, 0, REG_SZ, (const BYTE*)list->ips32,
				 lstrlenA(list->ips32) + 1);
	    if (res == ERROR_SUCCESS && list->ips32_tmodel)
		res = RegSetValueExA(ips32_key, tmodel_valuename, 0, REG_SZ,
                                     (const BYTE*)list->ips32_tmodel,
				     strlen(list->ips32_tmodel) + 1);
	    RegCloseKey(ips32_key);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
	}

	if (list->progid) {
	    res = register_key_defvalueA(clsid_key, progid_keyname,
					 list->progid);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;

	    res = register_progid(buf, list->progid, NULL,
				  list->name, list->progid_extra);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
	}

	if (list->viprogid) {
	    res = register_key_defvalueA(clsid_key, viprogid_keyname,
					 list->viprogid);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;

	    res = register_progid(buf, list->viprogid, list->progid,
				  list->name, list->progid_extra);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
	}

    error_close_clsid_key:
	RegCloseKey(clsid_key);
    }

error_close_coclass_key:
    RegCloseKey(coclass_key);
error_return:
    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
}

/***********************************************************************
 *		unregister_coclasses
 */
static HRESULT unregister_coclasses(struct regsvr_coclass const *list)
{
    LONG res = ERROR_SUCCESS;
    HKEY coclass_key;

    res = RegOpenKeyExW(HKEY_CLASSES_ROOT, clsid_keyname, 0,
			KEY_READ | KEY_WRITE, &coclass_key);
    if (res == ERROR_FILE_NOT_FOUND) return S_OK;
    if (res != ERROR_SUCCESS) goto error_return;

    for (; res == ERROR_SUCCESS && list->clsid; ++list) {
	WCHAR buf[39];

	StringFromGUID2(list->clsid, buf, 39);
	res = RegDeleteTreeW(coclass_key, buf);
	if (res == ERROR_FILE_NOT_FOUND) res = ERROR_SUCCESS;
	if (res != ERROR_SUCCESS) goto error_close_coclass_key;

	if (list->progid) {
	    res = RegDeleteTreeA(HKEY_CLASSES_ROOT, list->progid);
	    if (res == ERROR_FILE_NOT_FOUND) res = ERROR_SUCCESS;
	    if (res != ERROR_SUCCESS) goto error_close_coclass_key;
	}

	if (list->viprogid) {
	    res = RegDeleteTreeA(HKEY_CLASSES_ROOT, list->viprogid);
	    if (res == ERROR_FILE_NOT_FOUND) res = ERROR_SUCCESS;
	    if (res != ERROR_SUCCESS) goto error_close_coclass_key;
	}
    }

error_close_coclass_key:
    RegCloseKey(coclass_key);
error_return:
    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
}

/***********************************************************************
 *		register_mediatypes_parsing
 */
static HRESULT register_mediatypes_parsing(struct regsvr_mediatype_parsing const *list)
{
    LONG res = ERROR_SUCCESS;
    HKEY mediatype_key;
    WCHAR buf[39];
    int i;

    res = RegCreateKeyExW(HKEY_CLASSES_ROOT, mediatype_name, 0, NULL, 0,
			  KEY_READ | KEY_WRITE, NULL, &mediatype_key, NULL);
    if (res != ERROR_SUCCESS) return HRESULT_FROM_WIN32(res);

    for (; res == ERROR_SUCCESS && list->majortype; ++list) {
	HKEY majortype_key = NULL;
	HKEY subtype_key = NULL;

	StringFromGUID2(list->majortype, buf, 39);
	res = RegCreateKeyExW(mediatype_key, buf, 0, NULL, 0,
			      KEY_READ | KEY_WRITE, NULL, &majortype_key, NULL);
	if (res != ERROR_SUCCESS) goto error_close_keys;

	StringFromGUID2(list->subtype, buf, 39);
	res = RegCreateKeyExW(majortype_key, buf, 0, NULL, 0,
			      KEY_READ | KEY_WRITE, NULL, &subtype_key, NULL);
	if (res != ERROR_SUCCESS) goto error_close_keys;

	StringFromGUID2(&CLSID_AsyncReader, buf, 39);
        res = RegSetValueExW(subtype_key, sourcefilter_valuename, 0, REG_SZ, (const BYTE*)buf,
			     (lstrlenW(buf) + 1) * sizeof(WCHAR));
	if (res != ERROR_SUCCESS) goto error_close_keys;

	for(i = 0; list->line[i]; i++) {
	    char buffer[3];
	    wsprintfA(buffer, "%d", i);
            res = RegSetValueExA(subtype_key, buffer, 0, REG_SZ, (const BYTE*)list->line[i],
				 lstrlenA(list->line[i]));
	    if (res != ERROR_SUCCESS) goto error_close_keys;
	}

error_close_keys:
	if (majortype_key)
	    RegCloseKey(majortype_key);
	if (subtype_key)
	    RegCloseKey(subtype_key);
    }

    RegCloseKey(mediatype_key);

    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
}

/***********************************************************************
 *		register_mediatypes_extension
 */
static HRESULT register_mediatypes_extension(struct regsvr_mediatype_extension const *list)
{
    LONG res = ERROR_SUCCESS;
    HKEY mediatype_key;
    HKEY extensions_root_key = NULL;
    WCHAR buf[39];

    res = RegCreateKeyExW(HKEY_CLASSES_ROOT, mediatype_name, 0, NULL, 0,
			  KEY_READ | KEY_WRITE, NULL, &mediatype_key, NULL);
    if (res != ERROR_SUCCESS) return HRESULT_FROM_WIN32(res);

    res = RegCreateKeyExW(mediatype_key, extensions_keyname, 0, NULL, 0,
			  KEY_READ | KEY_WRITE, NULL, &extensions_root_key, NULL);
    if (res != ERROR_SUCCESS) goto error_return;

    for (; res == ERROR_SUCCESS && list->majortype; ++list) {
	HKEY extension_key;

	res = RegCreateKeyExA(extensions_root_key, list->extension, 0, NULL, 0,
			      KEY_READ | KEY_WRITE, NULL, &extension_key, NULL);
	if (res != ERROR_SUCCESS) break;

	StringFromGUID2(list->majortype, buf, 39);
        res = RegSetValueExW(extension_key, mediatype_name, 0, REG_SZ, (const BYTE*)buf,
			     (lstrlenW(buf) + 1) * sizeof(WCHAR));
	if (res != ERROR_SUCCESS) goto error_close_key;

	StringFromGUID2(list->subtype, buf, 39);
        res = RegSetValueExW(extension_key, subtype_valuename, 0, REG_SZ, (const BYTE*)buf,
			     (lstrlenW(buf) + 1) * sizeof(WCHAR));
	if (res != ERROR_SUCCESS) goto error_close_key;

	StringFromGUID2(&CLSID_AsyncReader, buf, 39);
        res = RegSetValueExW(extension_key, sourcefilter_valuename, 0, REG_SZ, (const BYTE*)buf,
			     (lstrlenW(buf) + 1) * sizeof(WCHAR));
	if (res != ERROR_SUCCESS) goto error_close_key;

error_close_key:
	RegCloseKey(extension_key);
    }

error_return:
    RegCloseKey(mediatype_key);
    if (extensions_root_key)
	RegCloseKey(extensions_root_key);

    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
}

/***********************************************************************
 *		unregister_mediatypes_parsing
 */
static HRESULT unregister_mediatypes_parsing(struct regsvr_mediatype_parsing const *list)
{
    LONG res;
    HKEY mediatype_key;
    HKEY majortype_key;
    WCHAR buf[39];

    res = RegOpenKeyExW(HKEY_CLASSES_ROOT, mediatype_name, 0,
			KEY_READ | KEY_WRITE, &mediatype_key);
    if (res == ERROR_FILE_NOT_FOUND) return S_OK;
    if (res != ERROR_SUCCESS) return HRESULT_FROM_WIN32(res);

    for (; res == ERROR_SUCCESS && list->majortype; ++list) {
	StringFromGUID2(list->majortype, buf, 39);
	res = RegOpenKeyExW(mediatype_key, buf, 0,
			KEY_READ | KEY_WRITE, &majortype_key);
	if (res == ERROR_FILE_NOT_FOUND) {
	    res = ERROR_SUCCESS;
	    continue;
	}
	if (res != ERROR_SUCCESS) break;

	StringFromGUID2(list->subtype, buf, 39);
	res = RegDeleteTreeW(majortype_key, buf);
    	if (res == ERROR_FILE_NOT_FOUND) res = ERROR_SUCCESS;

	/* Removed majortype key if there is no more subtype key */
	res = RegDeleteKeyW(majortype_key, 0);
	if (res == ERROR_ACCESS_DENIED) res = ERROR_SUCCESS;

	RegCloseKey(majortype_key);
    }

    RegCloseKey(mediatype_key);

    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
}

/***********************************************************************
 *		unregister_mediatypes_extension
 */
static HRESULT unregister_mediatypes_extension(struct regsvr_mediatype_extension const *list)
{
    LONG res;
    HKEY mediatype_key;
    HKEY extensions_root_key = NULL;

    res = RegOpenKeyExW(HKEY_CLASSES_ROOT, mediatype_name, 0,
			KEY_READ | KEY_WRITE, &mediatype_key);
    if (res == ERROR_FILE_NOT_FOUND) return S_OK;
    if (res != ERROR_SUCCESS) return HRESULT_FROM_WIN32(res);

    res = RegOpenKeyExW(mediatype_key, extensions_keyname, 0,
			KEY_READ | KEY_WRITE, &extensions_root_key);
    if (res == ERROR_FILE_NOT_FOUND)
	res = ERROR_SUCCESS;
    else if (res == ERROR_SUCCESS)
	for (; res == ERROR_SUCCESS && list->majortype; ++list) {
	    res = RegDeleteTreeA(extensions_root_key, list->extension);
	    if (res == ERROR_FILE_NOT_FOUND) res = ERROR_SUCCESS;
	}

    RegCloseKey(mediatype_key);
    if (extensions_root_key)
	RegCloseKey(extensions_root_key);

    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
}

/***********************************************************************
 *		register_filters
 */
static HRESULT register_filters(struct regsvr_filter const *list)
{
    HRESULT hr;
    IFilterMapper2* pFM2 = NULL;

    CoInitialize(NULL);
    hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER, &IID_IFilterMapper2, (LPVOID*)&pFM2);

    if (SUCCEEDED(hr)) {
	for (; SUCCEEDED(hr) && list->clsid; ++list) {
	    REGFILTER2 rf2;
	    REGFILTERPINS2* prfp2;
	    int i;

	    for (i = 0; list->pins[i].flags != 0xFFFFFFFF; i++) ;
	    rf2.dwVersion = 2;
	    rf2.dwMerit = list->merit;
	    rf2.u.s2.cPins2 = i;
	    rf2.u.s2.rgPins2 = prfp2 = CoTaskMemAlloc(i*sizeof(REGFILTERPINS2));
	    if (!prfp2) {
		hr = E_OUTOFMEMORY;
		break;
	    }
	    for (i = 0; list->pins[i].flags != 0xFFFFFFFF; i++) {
		REGPINTYPES* lpMediatype;
		CLSID* lpClsid;
		int j, nbmt;
                
		for (nbmt = 0; list->pins[i].mediatypes[nbmt].majortype; nbmt++) ;
		/* Allocate a single buffer for regpintypes struct and clsids */
		lpMediatype = CoTaskMemAlloc(nbmt*(sizeof(REGPINTYPES) + 2*sizeof(CLSID)));
		if (!lpMediatype) {
		    hr = E_OUTOFMEMORY;
		    break;
		}
		lpClsid = (CLSID*) (lpMediatype + nbmt);
		for (j = 0; j < nbmt; j++) {
		    (lpMediatype + j)->clsMajorType = lpClsid + j*2;
		    memcpy(lpClsid + j*2, list->pins[i].mediatypes[j].majortype, sizeof(CLSID));
		    (lpMediatype + j)->clsMinorType = lpClsid + j*2 + 1;
		    if (list->pins[i].mediatypes[j].subtype)
			memcpy(lpClsid + j*2 + 1, list->pins[i].mediatypes[j].subtype, sizeof(CLSID));
		    else {
                        /* Subtypes are often a combination of major type + fourcc/tag */
			memcpy(lpClsid + j*2 + 1, list->pins[i].mediatypes[j].majortype, sizeof(CLSID));
			*(DWORD*)(lpClsid + j*2 + 1) = list->pins[i].mediatypes[j].fourcc;
		    }
		}
		prfp2[i].dwFlags = list->pins[i].flags;
		prfp2[i].cInstances = 0;
		prfp2[i].nMediaTypes = j;
		prfp2[i].lpMediaType = lpMediatype;
		prfp2[i].nMediums = 0;
		prfp2[i].lpMedium = NULL;
		prfp2[i].clsPinCategory = NULL;
	    }

	    if (FAILED(hr)) {
		ERR("failed to register with hresult 0x%x\n", hr);
		CoTaskMemFree(prfp2);
		break;
	    }

	    hr = IFilterMapper2_RegisterFilter(pFM2, list->clsid, list->name, NULL, list->category, NULL, &rf2);

	    while (i) {
		CoTaskMemFree((REGPINTYPES*)prfp2[i-1].lpMediaType);
		i--;
	    }
	    CoTaskMemFree(prfp2);
	}
    }

    if (pFM2)
	IFilterMapper2_Release(pFM2);

    CoUninitialize();

    return hr;
}

/***********************************************************************
 *		unregister_filters
 */
static HRESULT unregister_filters(struct regsvr_filter const *list)
{
    HRESULT hr;
    IFilterMapper2* pFM2;

    CoInitialize(NULL);
    
    hr = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER, &IID_IFilterMapper2, (LPVOID*)&pFM2);

    if (SUCCEEDED(hr)) {
	for (; SUCCEEDED(hr) && list->clsid; ++list)
	    hr = IFilterMapper2_UnregisterFilter(pFM2, list->category, NULL, list->clsid);
	IFilterMapper2_Release(pFM2);
    }

    CoUninitialize();
    
    return hr;
}

/***********************************************************************
 *		regsvr_key_guid
 */
static LONG register_key_guid(HKEY base, WCHAR const *name, GUID const *guid)
{
    WCHAR buf[39];

    StringFromGUID2(guid, buf, 39);
    return register_key_defvalueW(base, name, buf);
}

/***********************************************************************
 *		regsvr_key_defvalueW
 */
static LONG register_key_defvalueW(
    HKEY base,
    WCHAR const *name,
    WCHAR const *value)
{
    LONG res;
    HKEY key;

    res = RegCreateKeyExW(base, name, 0, NULL, 0,
			  KEY_READ | KEY_WRITE, NULL, &key, NULL);
    if (res != ERROR_SUCCESS) return res;
    res = RegSetValueExW(key, NULL, 0, REG_SZ, (const BYTE*)value,
			 (lstrlenW(value) + 1) * sizeof(WCHAR));
    RegCloseKey(key);
    return res;
}

/***********************************************************************
 *		regsvr_key_defvalueA
 */
static LONG register_key_defvalueA(
    HKEY base,
    WCHAR const *name,
    char const *value)
{
    LONG res;
    HKEY key;

    res = RegCreateKeyExW(base, name, 0, NULL, 0,
			  KEY_READ | KEY_WRITE, NULL, &key, NULL);
    if (res != ERROR_SUCCESS) return res;
    res = RegSetValueExA(key, NULL, 0, REG_SZ, (const BYTE*)value, lstrlenA(value) + 1);
    RegCloseKey(key);
    return res;
}

/***********************************************************************
 *		regsvr_progid
 */
static LONG register_progid(
    WCHAR const *clsid,
    char const *progid,
    char const *curver_progid,
    char const *name,
    char const *extra)
{
    LONG res;
    HKEY progid_key;

    res = RegCreateKeyExA(HKEY_CLASSES_ROOT, progid, 0,
			  NULL, 0, KEY_READ | KEY_WRITE, NULL,
			  &progid_key, NULL);
    if (res != ERROR_SUCCESS) return res;

    if (name) {
        res = RegSetValueExA(progid_key, NULL, 0, REG_SZ, (const BYTE*)name, strlen(name) + 1);
	if (res != ERROR_SUCCESS) goto error_close_progid_key;
    }

    if (clsid) {
	res = register_key_defvalueW(progid_key, clsid_keyname, clsid);
	if (res != ERROR_SUCCESS) goto error_close_progid_key;
    }

    if (curver_progid) {
	res = register_key_defvalueA(progid_key, curver_keyname,
				     curver_progid);
	if (res != ERROR_SUCCESS) goto error_close_progid_key;
    }

    if (extra) {
	HKEY extra_key;

	res = RegCreateKeyExA(progid_key, extra, 0,
			      NULL, 0, KEY_READ | KEY_WRITE, NULL,
			      &extra_key, NULL);
	if (res == ERROR_SUCCESS)
	    RegCloseKey(extra_key);
    }

error_close_progid_key:
    RegCloseKey(progid_key);
    return res;
}

/***********************************************************************
 *		coclass list
 */
static struct regsvr_coclass const coclass_list[] = {
    { NULL }			/* list terminator */
};

/***********************************************************************
 *		interface list
 */

static struct regsvr_interface const interface_list[] = {
    { NULL }			/* list terminator */
};

/***********************************************************************
 *		mediatype list
 */

static struct regsvr_mediatype_parsing const mediatype_parsing_list[] = {
    {	&MEDIATYPE_Stream,
	&MEDIASUBTYPE_Avi,
	{   "0,4,,52494646,8,4,,41564920",
	    NULL }
    },
    {	&MEDIATYPE_Stream,
	&MEDIASUBTYPE_MPEG1System,
	{   "0, 16, FFFFFFFFF100010001800001FFFFFFFF, 000001BA2100010001800001000001BB",
	    NULL }
    },
    {	&MEDIATYPE_Stream,
	&MEDIASUBTYPE_MPEG1VideoCD,
	{   "0, 4, , 52494646, 8, 8, , 43445841666D7420, 36, 20, FFFFFFFF00000000FFFFFFFFFFFFFFFFFFFFFFFF, 646174610000000000FFFFFFFFFFFFFFFFFFFF00",
	    NULL }
    },
    {	&MEDIATYPE_Stream,
	&MEDIASUBTYPE_MPEG1Video,
	{   "0, 4, , 000001B3",
	    NULL }
    },
    {	&MEDIATYPE_Stream,
	&MEDIASUBTYPE_MPEG1Audio,
	{   "0, 2, FFE0, FFE0",
            "0, 10, FFFFFF00000080808080, 494433000000000000",
	    NULL }
    },
    {	&MEDIATYPE_Stream,
	&MEDIASUBTYPE_QTMovie,
	{   "4, 4, , 6d646174",
	    "4, 4, , 6d6f6f76",
	    NULL }
    },
    {	&MEDIATYPE_Stream,
	&MEDIASUBTYPE_WAVE,
	{   "0,4,,52494646,8,4,,57415645",
	    NULL }
    },
    {	&MEDIATYPE_Stream,
	&MEDIASUBTYPE_AU,
	{   "0,4,,2e736e64",
	    NULL }
    },
    {	&MEDIATYPE_Stream,
	&MEDIASUBTYPE_AIFF,
	{   "0,4,,464f524d,8,4,,41494646",
	    "0,4,,464f524d,8,4,,41494643",
	    NULL }
    },
    {	&MEDIATYPE_Stream,
	&MEDIATYPE_Text,
	{   "0,4,,4C595249",
	    "0,4,,6C797269",
	    NULL }
    },
    {	&MEDIATYPE_Stream,
	&MEDIATYPE_Midi,
	{   "0,4,,52494646,8,4,,524D4944",
	    "0,4,,4D546864",
	    NULL }
    },
    { NULL }			/* list terminator */
};

/***********************************************************************
 *		mediatype list
 */

static struct regsvr_mediatype_extension const mediatype_extension_list[] = {
    {	&MEDIATYPE_Stream,
	&MEDIASUBTYPE_MPEG1Audio,
	".mp3"
    },
    { NULL }			/* list terminator */
};

/***********************************************************************
 *		filter list
 */

static struct regsvr_filter const filter_list[] = {
    {   &CLSID_AviSplitter,
	&CLSID_LegacyAmFilterCategory,
	{'A','V','I',' ','S','p','l','i','t','t','e','r',0},
	0x5ffff0,
	{   {   0,
		{   { &MEDIATYPE_Stream, &MEDIASUBTYPE_Avi },
		    { NULL }
		},
	    },
	    {   REG_PINFLAG_B_OUTPUT,
		{   { &MEDIATYPE_Video, &GUID_NULL },
		    { NULL }
		},
	    },
	    { 0xFFFFFFFF },
	}
    },
    {   &CLSID_MPEG1Splitter,
        &CLSID_LegacyAmFilterCategory,
        {'M','P','E','G','-','I',' ','S','t','r','e','a','m',' ','S','p','l','i','t','t','e','r',0},
        0x5ffff0,
        {   {   0,
                {   { &MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1Audio },
                    { &MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1Video },
                    { &MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1System },
                    { &MEDIATYPE_Stream, &MEDIASUBTYPE_MPEG1VideoCD },
                    { NULL }
                },
            },
            {   REG_PINFLAG_B_OUTPUT,
                {   { &MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1Packet },
                    { &MEDIATYPE_Audio, &MEDIASUBTYPE_MPEG1AudioPayload },
                    { NULL }
                },
            },
            {   REG_PINFLAG_B_OUTPUT,
                {   { &MEDIATYPE_Video, &MEDIASUBTYPE_MPEG1Packet },
                    { &MEDIATYPE_Video, &MEDIASUBTYPE_MPEG1Payload },
                    { NULL }
                },
            },
            { 0xFFFFFFFF },
        }
    },
    {   &CLSID_NullRenderer,
        &CLSID_LegacyAmFilterCategory,
        {'N','u','l','l',' ','R','e','n','d','e','r','e','r',0},
        0x200000,
        {   {   REG_PINFLAG_B_RENDERER,
                {   { &MEDIATYPE_NULL, &GUID_NULL },
                    { NULL }
                },
            },
            { 0xFFFFFFFF },
        }
    },
    {   &CLSID_VideoRenderer,
	&CLSID_LegacyAmFilterCategory,
	{'V','i','d','e','o',' ','R','e','n','d','e','r','e','r',0},
	0x800000,
	{   {   REG_PINFLAG_B_RENDERER,
		{   { &MEDIATYPE_Video, &GUID_NULL },
		    { NULL }
		},
	    },
	    { 0xFFFFFFFF },
	}
    },
    {   &CLSID_VideoRendererDefault,
        &CLSID_LegacyAmFilterCategory,
        {'V','i','d','e','o',' ','R','e','n','d','e','r','e','r',0},
        0x800000,
        {   {   REG_PINFLAG_B_RENDERER,
                {   { &MEDIATYPE_Video, &GUID_NULL },
                    { NULL }
                },
            },
            { 0xFFFFFFFF },
        }
    },
    {   &CLSID_VideoMixingRenderer,
        &CLSID_LegacyAmFilterCategory,
        {'V','i','d','e','o',' ','M','i','x','i','n','g',' ','R','e','n','d','e','r','e','r',0},
        0x200000,
        {   {   REG_PINFLAG_B_RENDERER,
                {   { &MEDIATYPE_Video, &GUID_NULL },
                    { NULL }
                },
            },
            { 0xFFFFFFFF },
        }
    },
    {   &CLSID_VideoMixingRenderer9,
        &CLSID_LegacyAmFilterCategory,
        {'V','i','d','e','o',' ','M','i','x','i','n','g',' ','R','e','n','d','e','r','e','r',' ','9',0},
        0x200000,
        {   {   REG_PINFLAG_B_RENDERER,
                {   { &MEDIATYPE_Video, &GUID_NULL },
                    { NULL }
                },
            },
            { 0xFFFFFFFF },
        }
    },
    {   &CLSID_DSoundRender,
        &CLSID_LegacyAmFilterCategory,
        {'A','u','d','i','o',' ','R','e','n','d','e','r','e','r',0},
        0x800000,
        {   {   REG_PINFLAG_B_RENDERER,
                {   { &MEDIATYPE_Audio, &MEDIASUBTYPE_PCM },
/*                  { &MEDIATYPE_Audio, &MEDIASUBTYPE_IEEE_FLOAT }, */
                    { NULL }
                },
            },
            { 0xFFFFFFFF },
        }
    },
    {   &CLSID_AudioRender,
        &CLSID_LegacyAmFilterCategory,
        {'A','u','d','i','o',' ','R','e','n','d','e','r','e','r',0},
        0x800000,
        {   {   REG_PINFLAG_B_RENDERER,
                {   { &MEDIATYPE_Audio, &MEDIASUBTYPE_PCM },
/*                  { &MEDIATYPE_Audio, &MEDIASUBTYPE_IEEE_FLOAT }, */
                    { NULL }
                },
            },
            { 0xFFFFFFFF },
        }
    },
    {   &CLSID_AVIDec,
	&CLSID_LegacyAmFilterCategory,
	{'A','V','I',' ','D','e','c','o','m','p','r','e','s','s','o','r',0},
	0x5ffff0,
	{   {   0,
		{   { &MEDIATYPE_Video, &GUID_NULL },
		    { NULL }
		},
	    },
	    {   REG_PINFLAG_B_OUTPUT,
		{   { &MEDIATYPE_Video, &GUID_NULL },
		    { NULL }
		},
	    },
	    { 0xFFFFFFFF },
	}
    },
    {   &CLSID_AsyncReader,
	&CLSID_LegacyAmFilterCategory,
	{'F','i','l','e',' ','S','o','u','r','c','e',' ','(','A','s','y','n','c','.',')',0},
	0x400000,
	{   {   REG_PINFLAG_B_OUTPUT,
		{   { &MEDIATYPE_Stream, &GUID_NULL },
		    { NULL }
		},
	    },
	    { 0xFFFFFFFF },
	}
    },
    {   &CLSID_ACMWrapper,
	&CLSID_LegacyAmFilterCategory,
	{'A','C','M',' ','W','r','a','p','p','e','r',0},
	0x5ffff0,
	{   {   0,
		{   { &MEDIATYPE_Audio, &GUID_NULL },
		    { NULL }
		},
	    },
	    {   REG_PINFLAG_B_OUTPUT,
		{   { &MEDIATYPE_Audio, &GUID_NULL },
		    { NULL }
		},
	    },
	    { 0xFFFFFFFF },
	}
    },
    {   &CLSID_WAVEParser,
	&CLSID_LegacyAmFilterCategory,
	{'W','a','v','e',' ','P','a','r','s','e','r',0},
	0x400000,
	{   {   0,
		{   { &MEDIATYPE_Stream, &MEDIASUBTYPE_WAVE },
		    { &MEDIATYPE_Stream, &MEDIASUBTYPE_AU },
		    { &MEDIATYPE_Stream, &MEDIASUBTYPE_AIFF },
		    { NULL }
		},
	    },
	    {   REG_PINFLAG_B_OUTPUT,
		{   { &MEDIATYPE_Audio, &GUID_NULL },
		    { NULL }
		},
	    },
	    { 0xFFFFFFFF },
	}
    },
    { NULL }		/* list terminator */
};

extern HRESULT WINAPI QUARTZ_DllRegisterServer(void) DECLSPEC_HIDDEN;
extern HRESULT WINAPI QUARTZ_DllUnregisterServer(void) DECLSPEC_HIDDEN;

/***********************************************************************
 *		DllRegisterServer (QUARTZ.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    HRESULT hr;

    TRACE("\n");

    hr = QUARTZ_DllRegisterServer();
    if (SUCCEEDED(hr))
        hr = register_coclasses(coclass_list);
    if (SUCCEEDED(hr))
	hr = register_interfaces(interface_list);
    if (SUCCEEDED(hr))
        hr = register_mediatypes_parsing(mediatype_parsing_list);
    if (SUCCEEDED(hr))
        hr = register_mediatypes_extension(mediatype_extension_list);
    if (SUCCEEDED(hr))
        hr = register_filters(filter_list);
    return hr;
}

/***********************************************************************
 *		DllUnregisterServer (QUARTZ.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    HRESULT hr;

    TRACE("\n");

    hr = unregister_filters(filter_list);
    if (SUCCEEDED(hr))
	hr = unregister_coclasses(coclass_list);
    if (SUCCEEDED(hr))
	hr = unregister_interfaces(interface_list);
    if (SUCCEEDED(hr))
	hr = unregister_mediatypes_parsing(mediatype_parsing_list);
    if (SUCCEEDED(hr))
	hr = unregister_mediatypes_extension(mediatype_extension_list);
    if (SUCCEEDED(hr))
        hr = QUARTZ_DllUnregisterServer();
    return hr;
}
