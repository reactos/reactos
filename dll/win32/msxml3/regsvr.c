/*
 *	MSXML3 self-registerable dll functions
 *
 * Copyright (C) 2003 John K. Hohm
 * Copyright (C) 2006 Robert Shearman
 * Copyright (C) 2008 Alistair Leslie-Hughes
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
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "winerror.h"
#include "ole2.h"
#include "msxml.h"
#include "xmldom.h"
#include "xmldso.h"
#include "msxml2.h"

/* undef the #define in msxml2 so that we can access the v.2 version
   independent CLSID as well as the v.3 one. */
#undef CLSID_DOMDocument

#include "msxml_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

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
    LPCSTR version;		/* can be NULL to omit */
};

static HRESULT register_coclasses(struct regsvr_coclass const *list);
static HRESULT unregister_coclasses(struct regsvr_coclass const *list);

struct progid
{
    LPCSTR name;		/* NULL for end of list */
    LPCSTR description;		/* can be NULL to omit */
    CLSID const *clsid;
    LPCSTR curver;		/* can be NULL to omit */
};

static HRESULT register_progids(struct progid const *list);
static HRESULT unregister_progids(struct progid const *list);

/***********************************************************************
 *		static string constants
 */
static WCHAR const interface_keyname[10] = {
    'I', 'n', 't', 'e', 'r', 'f', 'a', 'c', 'e', 0 };
static WCHAR const base_ifa_keyname[14] = {
    'B', 'a', 's', 'e', 'I', 'n', 't', 'e', 'r', 'f', 'a', 'c',
    'e', 0 };
static WCHAR const num_methods_keyname[11] = {
    'N', 'u', 'm', 'M', 'e', 't', 'h', 'o', 'd', 's', 0 };
static WCHAR const ps_clsid_keyname[15] = {
    'P', 'r', 'o', 'x', 'y', 'S', 't', 'u', 'b', 'C', 'l', 's',
    'i', 'd', 0 };
static WCHAR const ps_clsid32_keyname[17] = {
    'P', 'r', 'o', 'x', 'y', 'S', 't', 'u', 'b', 'C', 'l', 's',
    'i', 'd', '3', '2', 0 };
static WCHAR const clsid_keyname[6] = {
    'C', 'L', 'S', 'I', 'D', 0 };
static WCHAR const ips_keyname[13] = {
    'I', 'n', 'P', 'r', 'o', 'c', 'S', 'e', 'r', 'v', 'e', 'r',
    0 };
static WCHAR const ips32_keyname[15] = {
    'I', 'n', 'P', 'r', 'o', 'c', 'S', 'e', 'r', 'v', 'e', 'r',
    '3', '2', 0 };
static WCHAR const progid_keyname[7] = {
    'P', 'r', 'o', 'g', 'I', 'D', 0 };
static WCHAR const versionindependentprogid_keyname[] = {
    'V', 'e', 'r', 's', 'i', 'o', 'n',
    'I', 'n', 'd', 'e', 'p', 'e', 'n', 'd', 'e', 'n', 't',
    'P', 'r', 'o', 'g', 'I', 'D', 0 };
static WCHAR const version_keyname[] = {
    'V', 'e', 'r', 's', 'i', 'o', 'n', 0 };
static WCHAR const curver_keyname[] = {
    'C', 'u', 'r', 'V', 'e', 'r', 0 };
static char const tmodel_valuename[] = "ThreadingModel";

/***********************************************************************
 *		static helper functions
 */
static LONG register_key_guid(HKEY base, WCHAR const *name, GUID const *guid);
static LONG register_key_defvalueW(HKEY base, WCHAR const *name,
				   WCHAR const *value);
static LONG register_key_defvalueA(HKEY base, WCHAR const *name,
				   char const *value);

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
	    res = RegSetValueExA(iid_key, NULL, 0, REG_SZ,
				 (CONST BYTE*)(list->name),
				 strlen(list->name) + 1);
	    if (res != ERROR_SUCCESS) goto error_close_iid_key;
	}

	if (list->base_iid) {
	    res = register_key_guid(iid_key, base_ifa_keyname, list->base_iid);
	    if (res != ERROR_SUCCESS) goto error_close_iid_key;
	}

	if (0 <= list->num_methods) {
	    static WCHAR const fmt[3] = { '%', 'd', 0 };
	    HKEY key;

	    res = RegCreateKeyExW(iid_key, num_methods_keyname, 0, NULL, 0,
				  KEY_READ | KEY_WRITE, NULL, &key, NULL);
	    if (res != ERROR_SUCCESS) goto error_close_iid_key;

	    wsprintfW(buf, fmt, list->num_methods);
	    res = RegSetValueExW(key, NULL, 0, REG_SZ,
				 (CONST BYTE*)buf,
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
	    res = RegSetValueExA(clsid_key, NULL, 0, REG_SZ,
				 (CONST BYTE*)(list->name),
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

	    res = RegSetValueExA(ips32_key, NULL, 0, REG_SZ,
				 (CONST BYTE*)list->ips32,
				 lstrlenA(list->ips32) + 1);
	    if (res == ERROR_SUCCESS && list->ips32_tmodel)
		res = RegSetValueExA(ips32_key, tmodel_valuename, 0, REG_SZ,
				     (CONST BYTE*)list->ips32_tmodel,
				     strlen(list->ips32_tmodel) + 1);
	    RegCloseKey(ips32_key);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
	}

	if (list->progid) {
            char *buffer = NULL;
            LPCSTR progid;

	    if (list->version) {
		buffer = HeapAlloc(GetProcessHeap(), 0, strlen(list->progid) + strlen(list->version) + 2);
		if (!buffer) {
		    res = ERROR_OUTOFMEMORY;
		    goto error_close_clsid_key;
		}
		strcpy(buffer, list->progid);
		strcat(buffer, ".");
		strcat(buffer, list->version);
		progid = buffer;
	    } else
		progid = list->progid;
	    res = register_key_defvalueA(clsid_key, progid_keyname,
					 progid);
	    HeapFree(GetProcessHeap(), 0, buffer);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;

	    if (list->version) {
	        res = register_key_defvalueA(clsid_key, versionindependentprogid_keyname,
					     list->progid);
		if (res != ERROR_SUCCESS) goto error_close_clsid_key;
	    }
	}

	if (list->version) {
	    res = register_key_defvalueA(clsid_key, version_keyname,
					 list->version);
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
    }

error_close_coclass_key:
    RegCloseKey(coclass_key);
error_return:
    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
}

/***********************************************************************
 *		register_progids
 */
static HRESULT register_progids(struct progid const *list)
{
    LONG res = ERROR_SUCCESS;

    for (; res == ERROR_SUCCESS && list->name; ++list) {
	WCHAR buf[39];
	HKEY progid_key;

	res = RegCreateKeyExA(HKEY_CLASSES_ROOT, list->name, 0,
			  NULL, 0, KEY_READ | KEY_WRITE, NULL,
			  &progid_key, NULL);
	if (res != ERROR_SUCCESS) goto error_close_clsid_key;

	res = RegSetValueExA(progid_key, NULL, 0, REG_SZ,
			 (CONST BYTE*)list->description,
			 strlen(list->description) + 1);
	if (res != ERROR_SUCCESS) goto error_close_clsid_key;

	StringFromGUID2(list->clsid, buf, 39);

	res = register_key_defvalueW(progid_key, clsid_keyname, buf);
	if (res != ERROR_SUCCESS) goto error_close_clsid_key;

	if (list->curver) {
	    res = register_key_defvalueA(progid_key, curver_keyname, list->curver);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
	}

    error_close_clsid_key:
	RegCloseKey(progid_key);
    }

    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
}

/***********************************************************************
 *		unregister_progids
 */
static HRESULT unregister_progids(struct progid const *list)
{
    LONG res = ERROR_SUCCESS;

    for (; res == ERROR_SUCCESS && list->name; ++list) {
	res = RegDeleteTreeA(HKEY_CLASSES_ROOT, list->name);
	if (res == ERROR_FILE_NOT_FOUND) res = ERROR_SUCCESS;
    }

    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
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
    res = RegSetValueExW(key, NULL, 0, REG_SZ, (CONST BYTE*)value,
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
    res = RegSetValueExA(key, NULL, 0, REG_SZ, (CONST BYTE*)value,
			 lstrlenA(value) + 1);
    RegCloseKey(key);
    return res;
}

/***********************************************************************
 *		coclass list
 */
static struct regsvr_coclass const coclass_list[] = {
    {   &CLSID_DOMDocument,
	"XML DOM Document",
	NULL,
	"msxml3.dll",
	"Both",
	"Microsoft.XMLDOM",
	"1.0"
    },
    {   &CLSID_DOMDocument2,
        "XML DOM Document",
        NULL,
        "msxml3.dll",
        "Both",
        "Msxml2.DOMDocument",
        "3.0"
    },
    {   &CLSID_DOMDocument30,
        "XML DOM Document 3.0",
        NULL,
        "msxml3.dll",
        "Both",
        "Msxml2.DOMDocument",
        "3.0"
    },
    {   &CLSID_DOMFreeThreadedDocument,
	"Free threaded XML DOM Document",
	NULL,
	"msxml3.dll",
	"Both",
	"Microsoft.FreeThreadedXMLDOM",
	"1.0"
    },
    {   &CLSID_DOMFreeThreadedDocument,
        "Free threaded XML DOM Document",
        NULL,
        "msxml3.dll",
        "Both",
        "Microsoft.FreeThreadedXMLDOM",
        NULL
    },
    {   &CLSID_FreeThreadedDOMDocument,
        "Free Threaded XML DOM Document",
        NULL,
        "msxml3.dll",
        "Both",
        "Microsoft.FreeThreadedXMLDOM.1.0",
        "1.0"
     },

    {   &CLSID_XMLHTTPRequest,
	"XML HTTP Request",
	NULL,
	"msxml3.dll",
	"Apartment",
	"Microsoft.XMLHTTP",
	"1.0"
    },
    {   &CLSID_XMLDSOControl,
	"XML Data Source Object",
	NULL,
	"msxml3.dll",
	"Apartment",
	"Microsoft.XMLDSO",
	"1.0"
    },
    {   &CLSID_XMLDocument,
	"Msxml",
	NULL,
	"msxml3.dll",
	"Both",
	"Msxml"
    },
    {   &CLSID_XMLSchemaCache,
	"XML Schema Cache",
	NULL,
	"msxml3.dll",
	"Both",
	"Msxml2.XMLSchemaCache",
        "3.0"
    },
    {   &CLSID_XMLSchemaCache30,
	"XML Schema Cache 3.0",
	NULL,
	"msxml3.dll",
	"Both",
	"Msxml2.XMLSchemaCache",
        "3.0"
    },
    {   &CLSID_SAXXMLReader,
        "SAX XML Reader",
        NULL,
        "msxml3.dll",
        "Both",
        "Msxml2.SAXXMLReader",
        "3.0"
    },
    { NULL }			/* list terminator */
};

/***********************************************************************
 *		interface list
 */
static struct regsvr_interface const interface_list[] = {
    { NULL }			/* list terminator */
};

/***********************************************************************
 *		progid list
 */
static struct progid const progid_list[] = {
    {   "Microsoft.XMLDOM",
	"XML DOM Document",
	&CLSID_DOMDocument,
	"Microsoft.XMLDOM.1.0"
    },
    {   "Microsoft.XMLDOM.1.0",
	"XML DOM Document",
	&CLSID_DOMDocument,
	NULL
    },
    {   "MSXML.DOMDocument",
	"XML DOM Document",
	&CLSID_DOMDocument,
	"Microsoft.XMLDOM.1.0"
    },
    {   "Msxml2.DOMDocument",
        "XML DOM Document",
        &CLSID_DOMDocument2,
        "Msxml2.DOMDocument.3.0"
    },
    {   "Msxml2.DOMDocument.3.0",
        "XML DOM Document 3.0",
        &CLSID_DOMDocument30,
        NULL
    },
    {   "Microsoft.FreeThreadedXMLDOM",
	"Free threaded XML DOM Document",
	&CLSID_DOMFreeThreadedDocument,
	"Microsoft.FreeThreadedXMLDOM.1.0"
    },
    {   "Microsoft.FreeThreadedXMLDOM.1.0",
	"Free threaded XML DOM Document",
	&CLSID_DOMFreeThreadedDocument,
	NULL
    },
    {   "MSXML.FreeThreadedDOMDocument",
	"Free threaded XML DOM Document",
	&CLSID_DOMFreeThreadedDocument,
	"Microsoft.FreeThreadedXMLDOM.1.0"
    },
    {   "Microsoft.XMLHTTP",
	"XML HTTP Request",
	&CLSID_XMLHTTPRequest,
	"Microsoft.XMLHTTP.1.0"
    },
    {   "Microsoft.XMLHTTP.1.0",
	"XML HTTP Request",
	&CLSID_XMLHTTPRequest,
	NULL
    },
    {   "Microsoft.XMLDSO",
	"XML Data Source Object",
	&CLSID_XMLDSOControl,
	"Microsoft.XMLDSO.1.0"
    },
    {   "Microsoft.XMLDSO.1.0",
	"XML Data Source Object",
	&CLSID_XMLDSOControl,
	NULL
    },
    {   "Msxml",
	"Msxml",
	&CLSID_XMLDocument,
	NULL
    },
    {   "Msxml2.XMLSchemaCache",
        "XML Schema Cache",
        &CLSID_XMLSchemaCache,
        "Msxml2.XMLSchemaCache.3.0"
    },
    {   "Msxml2.XMLSchemaCache.3.0",
        "XML Schema Cache 3.0",
        &CLSID_XMLSchemaCache30,
        NULL
    },

    { NULL }			/* list terminator */
};

/***********************************************************************
 *		DllRegisterServer (OLEAUT32.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    HRESULT hr;
    ITypeLib *tl;
    static const WCHAR wszMsXml3[] = {'m','s','x','m','l','3','.','d','l','l',0};

    TRACE("\n");

    hr = register_coclasses(coclass_list);
    if (SUCCEEDED(hr))
	hr = register_interfaces(interface_list);
    if (SUCCEEDED(hr))
	hr = register_progids(progid_list);

    if(SUCCEEDED(hr)) {

        hr = LoadTypeLibEx(wszMsXml3, REGKIND_REGISTER, &tl);
        if(SUCCEEDED(hr))
            ITypeLib_Release(tl);
    }

    return hr;
}

/***********************************************************************
 *		DllUnregisterServer (OLEAUT32.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    HRESULT hr;

    TRACE("\n");

    hr = unregister_coclasses(coclass_list);
    if (SUCCEEDED(hr))
	hr = unregister_interfaces(interface_list);
    if (SUCCEEDED(hr))
	hr = unregister_progids(progid_list);
    if (SUCCEEDED(hr))
        hr = UnRegisterTypeLib(&LIBID_MSXML2, 3, 0, LOCALE_SYSTEM_DEFAULT, SYS_WIN32);

    return hr;
}
