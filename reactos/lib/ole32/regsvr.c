/*
 *	self-registerable dll functions for ole32.dll
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "winerror.h"

#include "ole2.h"
#include "olectl.h"

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
};

static HRESULT register_coclasses(struct regsvr_coclass const *list);
static HRESULT unregister_coclasses(struct regsvr_coclass const *list);

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
static char const tmodel_valuename[] = "ThreadingModel";

/***********************************************************************
 *		static helper functions
 */
static LONG register_key_guid(HKEY base, WCHAR const *name, GUID const *guid);
static LONG register_key_defvalueW(HKEY base, WCHAR const *name,
				   WCHAR const *value);
static LONG register_key_defvalueA(HKEY base, WCHAR const *name,
				   char const *value);
static LONG recursive_delete_key(HKEY key);


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
	    register_key_guid(iid_key, base_ifa_keyname, list->base_iid);
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
	    register_key_guid(iid_key, ps_clsid_keyname, list->ps_clsid);
	    if (res != ERROR_SUCCESS) goto error_close_iid_key;
	}

	if (list->ps_clsid32) {
	    register_key_guid(iid_key, ps_clsid32_keyname, list->ps_clsid32);
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
	HKEY iid_key;

	StringFromGUID2(list->iid, buf, 39);
	res = RegOpenKeyExW(interface_key, buf, 0,
			    KEY_READ | KEY_WRITE, &iid_key);
	if (res == ERROR_FILE_NOT_FOUND) {
	    res = ERROR_SUCCESS;
	    continue;
	}
	if (res != ERROR_SUCCESS) goto error_close_interface_key;
	res = recursive_delete_key(iid_key);
	RegCloseKey(iid_key);
	if (res != ERROR_SUCCESS) goto error_close_interface_key;
    }

error_close_interface_key:
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
	HKEY clsid_key;

	StringFromGUID2(list->clsid, buf, 39);
	res = RegOpenKeyExW(coclass_key, buf, 0,
			    KEY_READ | KEY_WRITE, &clsid_key);
	if (res == ERROR_FILE_NOT_FOUND) {
	    res = ERROR_SUCCESS;
	    continue;
	}
	if (res != ERROR_SUCCESS) goto error_close_coclass_key;
	res = recursive_delete_key(clsid_key);
	RegCloseKey(clsid_key);
	if (res != ERROR_SUCCESS) goto error_close_coclass_key;
    }

error_close_coclass_key:
    RegCloseKey(coclass_key);
error_return:
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
 *		recursive_delete_key
 */
static LONG recursive_delete_key(HKEY key)
{
    LONG res;
    WCHAR subkey_name[MAX_PATH];
    DWORD cName;
    HKEY subkey;

    for (;;) {
	cName = sizeof(subkey_name) / sizeof(WCHAR);
	res = RegEnumKeyExW(key, 0, subkey_name, &cName,
			    NULL, NULL, NULL, NULL);
	if (res != ERROR_SUCCESS && res != ERROR_MORE_DATA) {
	    res = ERROR_SUCCESS; /* presumably we're done enumerating */
	    break;
	}
	res = RegOpenKeyExW(key, subkey_name, 0,
			    KEY_READ | KEY_WRITE, &subkey);
	if (res == ERROR_FILE_NOT_FOUND) continue;
	if (res != ERROR_SUCCESS) break;

	res = recursive_delete_key(subkey);
	RegCloseKey(subkey);
	if (res != ERROR_SUCCESS) break;
    }

    if (res == ERROR_SUCCESS) res = RegDeleteKeyW(key, 0);
    return res;
}

/***********************************************************************
 *		coclass list
 */
static GUID const CLSID_FileMoniker = {
    0x00000303, 0x0000, 0x0000, {0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46} };

static GUID const CLSID_ItemMoniker = {
    0x00000304, 0x0000, 0x0000, {0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46} };

/* FIXME: DfMarshal and PSFactoryBuffer are defined elsewhere too */

static GUID const CLSID_DfMarshal = {
    0x0000030B, 0x0000, 0x0000, {0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46} };

static GUID const CLSID_PSFactoryBuffer = {
    0x00000320, 0x0000, 0x0000, {0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46} };

static struct regsvr_coclass const coclass_list[] = {
    {   &CLSID_FileMoniker,
	"FileMoniker",
	NULL,
	"ole32.dll",
	"Both"
    },
    {   &CLSID_ItemMoniker,
	"ItemMoniker",
	NULL,
	"ole32.dll",
	"Both"
    },
    {   &CLSID_DfMarshal,
	"DfMarshal",
	NULL,
	"ole32.dll",
	"Both"
    },
    {	&CLSID_PSFactoryBuffer,
	"PSFactoryBuffer",
	NULL,
	"ole32.dll",
	"Both"
    },
    {   &CLSID_StdGlobalInterfaceTable,
	"StdGlobalInterfaceTable",
	NULL,
	"ole32.dll",
	"Apartment"
    },
    { NULL }			/* list terminator */
};

/***********************************************************************
 *		interface list
 */

/* FIXME: perhaps the interfaces that are proxied by another dll
 * should be registered in that dll?  Or maybe the choice of proxy is
 * arbitrary at this point? */

static GUID const CLSID_PSFactoryBuffer_ole2disp = {
    0x00020420, 0x0000, 0x0000, {0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46} };

static GUID const CLSID_PSFactoryBuffer_oleaut32 = {
    0xB196B286, 0xBAB4, 0x101A, {0xB6,0x9C,0x00,0xAA,0x00,0x34,0x1D,0x07} };

/* FIXME: these interfaces should be defined in ocidl.idl */

static IID const IID_IFontEventsDisp = {
    0x4EF6100A, 0xAF88, 0x11D0, {0x98,0x46,0x00,0xC0,0x4F,0xC2,0x99,0x93} };

static IID const IID_IProvideMultipleClassInfo = {
    0xA7ABA9C1, 0x8983, 0x11CF, {0x8F,0x20,0x00,0x80,0x5F,0x2C,0xD0,0x64} };

static IID const IID_IObjectWithSite = {
    0xFC4801A3, 0x2BA9, 0x11CF, {0xA2,0x29,0x00,0xAA,0x00,0x3D,0x73,0x52} };

static struct regsvr_interface const interface_list[] = {
    {   &IID_IUnknown,
	"IUnknown",
	NULL,
	3,
	NULL,
	&CLSID_PSFactoryBuffer
    },
    {   &IID_IClassFactory,
	"IClassFactory",
	NULL,
	5,
	NULL,
	&CLSID_PSFactoryBuffer
    },
    {   &IID_IStorage,
	"IStorage",
	NULL,
	18,
	NULL,
	&CLSID_PSFactoryBuffer
    },
    {   &IID_IStream,
	"IStream",
	NULL,
	14,
	NULL,
	&CLSID_PSFactoryBuffer
    },
    {   &IID_IPersistStorage,
	"IPersistStorage",
	&IID_IPersist,
	10,
	NULL,
	&CLSID_PSFactoryBuffer
    },
    {   &IID_IDataObject,
	"IDataObject",
	NULL,
	12,
	NULL,
	&CLSID_PSFactoryBuffer
    },
    {   &IID_IAdviseSink,
	"IAdviseSink",
	NULL,
	8,
	NULL,
	&CLSID_PSFactoryBuffer
    },
    {   &IID_IOleObject,
	"IOleObject",
	NULL,
	24,
	NULL,
	&CLSID_PSFactoryBuffer
    },
    {   &IID_IOleClientSite,
	"IOleClientSite",
	NULL,
	9,
	NULL,
	&CLSID_PSFactoryBuffer
    },
    {   &IID_IDispatch,
	"IDispatch",
	NULL,
	7,
	&CLSID_PSFactoryBuffer_ole2disp,
	&CLSID_PSFactoryBuffer_ole2disp
    },
    {   &IID_ITypeLib2,
	"ITypeLib2",
	NULL,
	16,
	NULL,
	&CLSID_PSFactoryBuffer_ole2disp
    },
    {   &IID_ITypeInfo2,
	"ITypeInfo2",
	NULL,
	32,
	NULL,
	&CLSID_PSFactoryBuffer_ole2disp
    },
    {   &IID_IPropertyPage2,
	"IPropertyPage2",
	NULL,
	15,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IErrorInfo,
	"IErrorInfo",
	NULL,
	8,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_ICreateErrorInfo,
	"ICreateErrorInfo",
	NULL,
	8,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IPersistPropertyBag2,
	"IPersistPropertyBag2",
	NULL,
	8,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IPropertyBag2,
	"IPropertyBag2",
	NULL,
	8,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IErrorLog,
	"IErrorLog",
	NULL,
	4,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IPerPropertyBrowsing,
	"IPerPropertyBrowsing",
	NULL,
	7,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IPersistPropertyBag,
	"IPersistPropertyBag",
	NULL,
	7,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IAdviseSinkEx,
	"IAdviseSinkEx",
	NULL,
	9,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IFontEventsDisp,
	"IFontEventsDisp",
	NULL,
	7,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IPropertyBag,
	"IPropertyBag",
	NULL,
	5,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IPointerInactive,
	"IPointerInactive",
	NULL,
	6,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_ISimpleFrameSite,
	"ISimpleFrameSite",
	NULL,
	5,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IPicture,
	"IPicture",
	NULL,
	17,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IPictureDisp,
	"IPictureDisp",
	NULL,
	7,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IPersistStreamInit,
	"IPersistStreamInit",
	NULL,
	9,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IOleUndoUnit,
	"IOleUndoUnit",
	NULL,
	7,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IPropertyNotifySink,
	"IPropertyNotifySink",
	NULL,
	5,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IOleInPlaceSiteEx,
	"IOleInPlaceSiteEx",
	NULL,
	18,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IOleParentUndoUnit,
	"IOleParentUndoUnit",
	NULL,
	12,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IProvideClassInfo2,
	"IProvideClassInfo2",
	NULL,
	5,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IProvideMultipleClassInfo,
	"IProvideMultipleClassInfo",
	NULL,
	7,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IProvideClassInfo,
	"IProvideClassInfo",
	NULL,
	4,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IConnectionPointContainer,
	"IConnectionPointContainer",
	NULL,
	5,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IEnumConnectionPoints,
	"IEnumConnectionPoints",
	NULL,
	7,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IConnectionPoint,
	"IConnectionPoint",
	NULL,
	8,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IEnumConnections,
	"IEnumConnections",
	NULL,
	7,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IOleControl,
	"IOleControl",
	NULL,
	7,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IOleControlSite,
	"IOleControlSite",
	NULL,
	10,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_ISpecifyPropertyPages,
	"ISpecifyPropertyPages",
	NULL,
	4,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IPropertyPageSite,
	"IPropertyPageSite",
	NULL,
	7,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IPropertyPage,
	"IPropertyPage",
	NULL,
	14,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IClassFactory2,
	"IClassFactory2",
	NULL,
	8,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IEnumOleUndoUnits,
	"IEnumOleUndoUnits",
	NULL,
	7,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IPersistMemory,
	"IPersistMemory",
	NULL,
	9,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IFont,
	"IFont",
	NULL,
	27,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IFontDisp,
	"IFontDisp",
	NULL,
	7,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IQuickActivate,
	"IQuickActivate",
	NULL,
	6,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IOleUndoManager,
	"IOleUndoManager",
	NULL,
	15,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    {   &IID_IObjectWithSite,
	"IObjectWithSite",
	NULL,
	5,
	NULL,
	&CLSID_PSFactoryBuffer_oleaut32
    },
    { NULL }			/* list terminator */
};

/***********************************************************************
 *		DllRegisterServer (OLE32.@)
 */
HRESULT WINAPI OLE32_DllRegisterServer()
{
    HRESULT hr;

    TRACE("\n");

    hr = register_coclasses(coclass_list);
    if (SUCCEEDED(hr))
	hr = register_interfaces(interface_list);
    return hr;
}

/***********************************************************************
 *		DllUnregisterServer (OLE32.@)
 */
HRESULT WINAPI OLE32_DllUnregisterServer()
{
    HRESULT hr;

    TRACE("\n");

    hr = unregister_coclasses(coclass_list);
    if (SUCCEEDED(hr))
	hr = unregister_interfaces(interface_list);
    return hr;
}
