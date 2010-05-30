/*
 * Copyright 2009 Vincent Povirk for CodeWeavers
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#define COBJMACROS
#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "winerror.h"

#include "objbase.h"
#include "ocidl.h"
#include "wincodec.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

/***********************************************************************
 *		interface for self-registering
 */
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

struct decoder_pattern
{
    DWORD length;    /* 0 for end of list */
    DWORD position;
    const BYTE *pattern;
    const BYTE *mask;
    DWORD endofstream;
};

struct regsvr_decoder
{
    CLSID const *clsid;         /* NULL for end of list */
    LPCSTR author;
    LPCSTR friendlyname;
    LPCSTR version;
    GUID const *vendor;
    LPCSTR mimetypes;
    LPCSTR extensions;
    GUID const * const *formats;
    const struct decoder_pattern *patterns;
};

static HRESULT register_decoders(struct regsvr_decoder const *list);
static HRESULT unregister_decoders(struct regsvr_decoder const *list);

struct regsvr_converter
{
    CLSID const *clsid;         /* NULL for end of list */
    LPCSTR author;
    LPCSTR friendlyname;
    LPCSTR version;
    GUID const *vendor;
    GUID const * const *formats;
};

static HRESULT register_converters(struct regsvr_converter const *list);
static HRESULT unregister_converters(struct regsvr_converter const *list);

/***********************************************************************
 *		static string constants
 */
static WCHAR const clsid_keyname[6] = {
    'C', 'L', 'S', 'I', 'D', 0 };
static WCHAR const curver_keyname[7] = {
    'C', 'u', 'r', 'V', 'e', 'r', 0 };
static WCHAR const ips_keyname[13] = {
    'I', 'n', 'P', 'r', 'o', 'c', 'S', 'e', 'r', 'v', 'e', 'r',
    0 };
static WCHAR const ips32_keyname[15] = {
    'I', 'n', 'P', 'r', 'o', 'c', 'S', 'e', 'r', 'v', 'e', 'r',
    '3', '2', 0 };
static WCHAR const progid_keyname[7] = {
    'P', 'r', 'o', 'g', 'I', 'D', 0 };
static WCHAR const viprogid_keyname[25] = {
    'V', 'e', 'r', 's', 'i', 'o', 'n', 'I', 'n', 'd', 'e', 'p',
    'e', 'n', 'd', 'e', 'n', 't', 'P', 'r', 'o', 'g', 'I', 'D',
    0 };
static char const tmodel_valuename[] = "ThreadingModel";
static char const author_valuename[] = "Author";
static char const friendlyname_valuename[] = "FriendlyName";
static WCHAR const vendor_valuename[] = {'V','e','n','d','o','r',0};
static char const version_valuename[] = "Version";
static char const mimetypes_valuename[] = "MimeTypes";
static char const extensions_valuename[] = "FileExtensions";
static WCHAR const formats_keyname[] = {'F','o','r','m','a','t','s',0};
static WCHAR const patterns_keyname[] = {'P','a','t','t','e','r','n','s',0};
static WCHAR const instance_keyname[] = {'I','n','s','t','a','n','c','e',0};
static WCHAR const clsid_valuename[] = {'C','L','S','I','D',0};
static char const length_valuename[] = "Length";
static char const position_valuename[] = "Position";
static char const pattern_valuename[] = "Pattern";
static char const mask_valuename[] = "Mask";
static char const endofstream_valuename[] = "EndOfStream";
static WCHAR const pixelformats_keyname[] = {'P','i','x','e','l','F','o','r','m','a','t','s',0};

/***********************************************************************
 *		static helper functions
 */
static LONG register_key_defvalueW(HKEY base, WCHAR const *name,
				   WCHAR const *value);
static LONG register_key_defvalueA(HKEY base, WCHAR const *name,
				   char const *value);
static LONG register_progid(WCHAR const *clsid,
			    char const *progid, char const *curver_progid,
			    char const *name, char const *extra);

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
 *		register_decoders
 */
static HRESULT register_decoders(struct regsvr_decoder const *list)
{
    LONG res = ERROR_SUCCESS;
    HKEY coclass_key;
    WCHAR buf[39];
    HKEY decoders_key;
    HKEY instance_key;

    res = RegCreateKeyExW(HKEY_CLASSES_ROOT, clsid_keyname, 0, NULL, 0,
			  KEY_READ | KEY_WRITE, NULL, &coclass_key, NULL);
    if (res == ERROR_SUCCESS)  {
        StringFromGUID2(&CATID_WICBitmapDecoders, buf, 39);
        res = RegCreateKeyExW(coclass_key, buf, 0, NULL, 0,
			      KEY_READ | KEY_WRITE, NULL, &decoders_key, NULL);
        if (res == ERROR_SUCCESS)
        {
            res = RegCreateKeyExW(decoders_key, instance_keyname, 0, NULL, 0,
		              KEY_READ | KEY_WRITE, NULL, &instance_key, NULL);
            if (res != ERROR_SUCCESS) goto error_close_coclass_key;
        }
        if (res != ERROR_SUCCESS)
            RegCloseKey(coclass_key);
    }
    if (res != ERROR_SUCCESS) goto error_return;

    for (; res == ERROR_SUCCESS && list->clsid; ++list) {
	HKEY clsid_key;
	HKEY instance_clsid_key;

	StringFromGUID2(list->clsid, buf, 39);
	res = RegCreateKeyExW(coclass_key, buf, 0, NULL, 0,
			      KEY_READ | KEY_WRITE, NULL, &clsid_key, NULL);
	if (res != ERROR_SUCCESS) goto error_close_coclass_key;

	StringFromGUID2(list->clsid, buf, 39);
	res = RegCreateKeyExW(instance_key, buf, 0, NULL, 0,
			      KEY_READ | KEY_WRITE, NULL, &instance_clsid_key, NULL);
	if (res == ERROR_SUCCESS) {
	    res = RegSetValueExW(instance_clsid_key, clsid_valuename, 0, REG_SZ,
				 (CONST BYTE*)(buf), 78);
	    RegCloseKey(instance_clsid_key);
	}
	if (res != ERROR_SUCCESS) goto error_close_clsid_key;

        if (list->author) {
	    res = RegSetValueExA(clsid_key, author_valuename, 0, REG_SZ,
				 (CONST BYTE*)(list->author),
				 strlen(list->author) + 1);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->friendlyname) {
	    res = RegSetValueExA(clsid_key, friendlyname_valuename, 0, REG_SZ,
				 (CONST BYTE*)(list->friendlyname),
				 strlen(list->friendlyname) + 1);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->vendor) {
            StringFromGUID2(list->vendor, buf, 39);
	    res = RegSetValueExW(clsid_key, vendor_valuename, 0, REG_SZ,
				 (CONST BYTE*)(buf), 78);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->version) {
	    res = RegSetValueExA(clsid_key, version_valuename, 0, REG_SZ,
				 (CONST BYTE*)(list->version),
				 strlen(list->version) + 1);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->mimetypes) {
	    res = RegSetValueExA(clsid_key, mimetypes_valuename, 0, REG_SZ,
				 (CONST BYTE*)(list->mimetypes),
				 strlen(list->mimetypes) + 1);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->extensions) {
	    res = RegSetValueExA(clsid_key, extensions_valuename, 0, REG_SZ,
				 (CONST BYTE*)(list->extensions),
				 strlen(list->extensions) + 1);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->formats) {
            HKEY formats_key;
            GUID const * const *format;

            res = RegCreateKeyExW(clsid_key, formats_keyname, 0, NULL, 0,
                                  KEY_READ | KEY_WRITE, NULL, &formats_key, NULL);
            if (res != ERROR_SUCCESS) goto error_close_clsid_key;
            for (format=list->formats; *format; ++format)
            {
                HKEY format_key;
                StringFromGUID2(*format, buf, 39);
                res = RegCreateKeyExW(formats_key, buf, 0, NULL, 0,
                                      KEY_READ | KEY_WRITE, NULL, &format_key, NULL);
                if (res != ERROR_SUCCESS) break;
                RegCloseKey(format_key);
            }
            RegCloseKey(formats_key);
            if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->patterns) {
            HKEY patterns_key;
            int i;

            res = RegCreateKeyExW(clsid_key, patterns_keyname, 0, NULL, 0,
                                  KEY_READ | KEY_WRITE, NULL, &patterns_key, NULL);
            if (res != ERROR_SUCCESS) goto error_close_clsid_key;
            for (i=0; list->patterns[i].length; i++)
            {
                HKEY pattern_key;
                static const WCHAR int_format[] = {'%','i',0};
                snprintfW(buf, 39, int_format, i);
                res = RegCreateKeyExW(patterns_key, buf, 0, NULL, 0,
                                      KEY_READ | KEY_WRITE, NULL, &pattern_key, NULL);
                if (res != ERROR_SUCCESS) break;
	        res = RegSetValueExA(pattern_key, length_valuename, 0, REG_DWORD,
				     (CONST BYTE*)(&list->patterns[i].length), 4);
                if (res == ERROR_SUCCESS)
	            res = RegSetValueExA(pattern_key, position_valuename, 0, REG_DWORD,
				         (CONST BYTE*)(&list->patterns[i].position), 4);
                if (res == ERROR_SUCCESS)
	            res = RegSetValueExA(pattern_key, pattern_valuename, 0, REG_BINARY,
				         list->patterns[i].pattern,
				         list->patterns[i].length);
                if (res == ERROR_SUCCESS)
	            res = RegSetValueExA(pattern_key, mask_valuename, 0, REG_BINARY,
				         list->patterns[i].mask,
				         list->patterns[i].length);
                if (res == ERROR_SUCCESS)
	            res = RegSetValueExA(pattern_key, endofstream_valuename, 0, REG_DWORD,
				         (CONST BYTE*)&(list->patterns[i].endofstream), 4);
                RegCloseKey(pattern_key);
            }
            RegCloseKey(patterns_key);
            if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

    error_close_clsid_key:
	RegCloseKey(clsid_key);
    }

error_close_coclass_key:
    RegCloseKey(instance_key);
    RegCloseKey(decoders_key);
    RegCloseKey(coclass_key);
error_return:
    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
}

/***********************************************************************
 *		unregister_decoders
 */
static HRESULT unregister_decoders(struct regsvr_decoder const *list)
{
    LONG res = ERROR_SUCCESS;
    HKEY coclass_key;
    WCHAR buf[39];
    HKEY decoders_key;
    HKEY instance_key;

    res = RegOpenKeyExW(HKEY_CLASSES_ROOT, clsid_keyname, 0,
			KEY_READ | KEY_WRITE, &coclass_key);
    if (res == ERROR_FILE_NOT_FOUND) return S_OK;

    if (res == ERROR_SUCCESS)  {
        StringFromGUID2(&CATID_WICBitmapDecoders, buf, 39);
        res = RegCreateKeyExW(coclass_key, buf, 0, NULL, 0,
			      KEY_READ | KEY_WRITE, NULL, &decoders_key, NULL);
        if (res == ERROR_SUCCESS)
        {
            res = RegCreateKeyExW(decoders_key, instance_keyname, 0, NULL, 0,
		              KEY_READ | KEY_WRITE, NULL, &instance_key, NULL);
            if (res != ERROR_SUCCESS) goto error_close_coclass_key;
        }
        if (res != ERROR_SUCCESS)
            RegCloseKey(coclass_key);
    }
    if (res != ERROR_SUCCESS) goto error_return;

    for (; res == ERROR_SUCCESS && list->clsid; ++list) {
	StringFromGUID2(list->clsid, buf, 39);

	res = RegDeleteTreeW(coclass_key, buf);
	if (res == ERROR_FILE_NOT_FOUND) res = ERROR_SUCCESS;
	if (res != ERROR_SUCCESS) goto error_close_coclass_key;

	res = RegDeleteTreeW(instance_key, buf);
	if (res == ERROR_FILE_NOT_FOUND) res = ERROR_SUCCESS;
	if (res != ERROR_SUCCESS) goto error_close_coclass_key;
    }

error_close_coclass_key:
    RegCloseKey(instance_key);
    RegCloseKey(decoders_key);
    RegCloseKey(coclass_key);
error_return:
    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
}

/***********************************************************************
 *		register_converters
 */
static HRESULT register_converters(struct regsvr_converter const *list)
{
    LONG res = ERROR_SUCCESS;
    HKEY coclass_key;
    WCHAR buf[39];
    HKEY converters_key;
    HKEY instance_key;

    res = RegCreateKeyExW(HKEY_CLASSES_ROOT, clsid_keyname, 0, NULL, 0,
			  KEY_READ | KEY_WRITE, NULL, &coclass_key, NULL);
    if (res == ERROR_SUCCESS)  {
        StringFromGUID2(&CATID_WICFormatConverters, buf, 39);
        res = RegCreateKeyExW(coclass_key, buf, 0, NULL, 0,
			      KEY_READ | KEY_WRITE, NULL, &converters_key, NULL);
        if (res == ERROR_SUCCESS)
        {
            res = RegCreateKeyExW(converters_key, instance_keyname, 0, NULL, 0,
		              KEY_READ | KEY_WRITE, NULL, &instance_key, NULL);
            if (res != ERROR_SUCCESS) goto error_close_coclass_key;
        }
        if (res != ERROR_SUCCESS)
            RegCloseKey(coclass_key);
    }
    if (res != ERROR_SUCCESS) goto error_return;

    for (; res == ERROR_SUCCESS && list->clsid; ++list) {
	HKEY clsid_key;
	HKEY instance_clsid_key;

	StringFromGUID2(list->clsid, buf, 39);
	res = RegCreateKeyExW(coclass_key, buf, 0, NULL, 0,
			      KEY_READ | KEY_WRITE, NULL, &clsid_key, NULL);
	if (res != ERROR_SUCCESS) goto error_close_coclass_key;

	StringFromGUID2(list->clsid, buf, 39);
	res = RegCreateKeyExW(instance_key, buf, 0, NULL, 0,
			      KEY_READ | KEY_WRITE, NULL, &instance_clsid_key, NULL);
	if (res == ERROR_SUCCESS) {
	    res = RegSetValueExW(instance_clsid_key, clsid_valuename, 0, REG_SZ,
				 (CONST BYTE*)(buf), 78);
	    RegCloseKey(instance_clsid_key);
	}
	if (res != ERROR_SUCCESS) goto error_close_clsid_key;

        if (list->author) {
	    res = RegSetValueExA(clsid_key, author_valuename, 0, REG_SZ,
				 (CONST BYTE*)(list->author),
				 strlen(list->author) + 1);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->friendlyname) {
	    res = RegSetValueExA(clsid_key, friendlyname_valuename, 0, REG_SZ,
				 (CONST BYTE*)(list->friendlyname),
				 strlen(list->friendlyname) + 1);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->vendor) {
            StringFromGUID2(list->vendor, buf, 39);
	    res = RegSetValueExW(clsid_key, vendor_valuename, 0, REG_SZ,
				 (CONST BYTE*)(buf), 78);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->version) {
	    res = RegSetValueExA(clsid_key, version_valuename, 0, REG_SZ,
				 (CONST BYTE*)(list->version),
				 strlen(list->version) + 1);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->formats) {
            HKEY formats_key;
            GUID const * const *format;

            res = RegCreateKeyExW(clsid_key, pixelformats_keyname, 0, NULL, 0,
                                  KEY_READ | KEY_WRITE, NULL, &formats_key, NULL);
            if (res != ERROR_SUCCESS) goto error_close_clsid_key;
            for (format=list->formats; *format; ++format)
            {
                HKEY format_key;
                StringFromGUID2(*format, buf, 39);
                res = RegCreateKeyExW(formats_key, buf, 0, NULL, 0,
                                      KEY_READ | KEY_WRITE, NULL, &format_key, NULL);
                if (res != ERROR_SUCCESS) break;
                RegCloseKey(format_key);
            }
            RegCloseKey(formats_key);
            if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

    error_close_clsid_key:
	RegCloseKey(clsid_key);
    }

error_close_coclass_key:
    RegCloseKey(instance_key);
    RegCloseKey(converters_key);
    RegCloseKey(coclass_key);
error_return:
    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
}

/***********************************************************************
 *		unregister_converters
 */
static HRESULT unregister_converters(struct regsvr_converter const *list)
{
    LONG res = ERROR_SUCCESS;
    HKEY coclass_key;
    WCHAR buf[39];
    HKEY converters_key;
    HKEY instance_key;

    res = RegOpenKeyExW(HKEY_CLASSES_ROOT, clsid_keyname, 0,
			KEY_READ | KEY_WRITE, &coclass_key);
    if (res == ERROR_FILE_NOT_FOUND) return S_OK;

    if (res == ERROR_SUCCESS)  {
        StringFromGUID2(&CATID_WICFormatConverters, buf, 39);
        res = RegCreateKeyExW(coclass_key, buf, 0, NULL, 0,
			      KEY_READ | KEY_WRITE, NULL, &converters_key, NULL);
        if (res == ERROR_SUCCESS)
        {
            res = RegCreateKeyExW(converters_key, instance_keyname, 0, NULL, 0,
		              KEY_READ | KEY_WRITE, NULL, &instance_key, NULL);
            if (res != ERROR_SUCCESS) goto error_close_coclass_key;
        }
        if (res != ERROR_SUCCESS)
            RegCloseKey(coclass_key);
    }
    if (res != ERROR_SUCCESS) goto error_return;

    for (; res == ERROR_SUCCESS && list->clsid; ++list) {
	StringFromGUID2(list->clsid, buf, 39);

	res = RegDeleteTreeW(coclass_key, buf);
	if (res == ERROR_FILE_NOT_FOUND) res = ERROR_SUCCESS;
	if (res != ERROR_SUCCESS) goto error_close_coclass_key;

	res = RegDeleteTreeW(instance_key, buf);
	if (res == ERROR_FILE_NOT_FOUND) res = ERROR_SUCCESS;
	if (res != ERROR_SUCCESS) goto error_close_coclass_key;
    }

error_close_coclass_key:
    RegCloseKey(instance_key);
    RegCloseKey(converters_key);
    RegCloseKey(coclass_key);
error_return:
    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
}

/***********************************************************************
 *		register_key_defvalueW
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
 *		register_key_defvalueA
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
 *		register_progid
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
	res = RegSetValueExA(progid_key, NULL, 0, REG_SZ,
			     (CONST BYTE*)name, strlen(name) + 1);
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
    {   &CLSID_WICImagingFactory,
	"WIC Imaging Factory",
	NULL,
	"windowscodecs.dll",
	"Both"
    },
    {   &CLSID_WICBmpDecoder,
	"WIC BMP Decoder",
	NULL,
	"windowscodecs.dll",
	"Both"
    },
    {   &CLSID_WICPngDecoder,
	"WIC PNG Decoder",
	NULL,
	"windowscodecs.dll",
	"Both"
    },
    {   &CLSID_WICPngEncoder,
	"WIC PNG Encoder",
	NULL,
	"windowscodecs.dll",
	"Both"
    },
    {   &CLSID_WICBmpEncoder,
	"WIC BMP Encoder",
	NULL,
	"windowscodecs.dll",
	"Apartment"
    },
    {   &CLSID_WICGifDecoder,
	"WIC GIF Decoder",
	NULL,
	"windowscodecs.dll",
	"Both"
    },
    {   &CLSID_WICIcoDecoder,
	"WIC ICO Decoder",
	NULL,
	"windowscodecs.dll",
	"Both"
    },
    {   &CLSID_WICJpegDecoder,
	"WIC JPEG Decoder",
	NULL,
	"windowscodecs.dll",
	"Both"
    },
    {   &CLSID_WICTiffDecoder,
	"WIC TIFF Decoder",
	NULL,
	"windowscodecs.dll",
	"Both"
    },
    {   &CLSID_WICDefaultFormatConverter,
	"WIC Default Format Converter",
	NULL,
	"windowscodecs.dll",
	"Both"
    },
    { NULL }			/* list terminator */
};

/***********************************************************************
 *		decoder list
 */
static const BYTE mask_all[] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};

static const BYTE bmp_magic[] = {0x42,0x4d};

static GUID const * const bmp_formats[] = {
    &GUID_WICPixelFormat1bppIndexed,
    &GUID_WICPixelFormat2bppIndexed,
    &GUID_WICPixelFormat4bppIndexed,
    &GUID_WICPixelFormat8bppIndexed,
    &GUID_WICPixelFormat16bppBGR555,
    &GUID_WICPixelFormat16bppBGR565,
    &GUID_WICPixelFormat24bppBGR,
    &GUID_WICPixelFormat32bppBGR,
    &GUID_WICPixelFormat32bppBGRA,
    NULL
};

static struct decoder_pattern const bmp_patterns[] = {
    {2,0,bmp_magic,mask_all,0},
    {0}
};

static const BYTE gif87a_magic[6] = "GIF87a";
static const BYTE gif89a_magic[6] = "GIF89a";

static GUID const * const gif_formats[] = {
    &GUID_WICPixelFormat8bppIndexed,
    NULL
};

static struct decoder_pattern const gif_patterns[] = {
    {6,0,gif87a_magic,mask_all,0},
    {6,0,gif89a_magic,mask_all,0},
    {0}
};

static const BYTE ico_magic[] = {00,00,01,00};

static GUID const * const ico_formats[] = {
    &GUID_WICPixelFormat32bppBGRA,
    NULL
};

static struct decoder_pattern const ico_patterns[] = {
    {4,0,ico_magic,mask_all,0},
    {0}
};

static const BYTE jpeg_magic[] = {0xff, 0xd8, 0xff, 0xe0};

static GUID const * const jpeg_formats[] = {
    &GUID_WICPixelFormat24bppBGR,
    &GUID_WICPixelFormat8bppGray,
    NULL
};

static struct decoder_pattern const jpeg_patterns[] = {
    {4,0,jpeg_magic,mask_all,0},
    {0}
};

static const BYTE png_magic[] = {137,80,78,71,13,10,26,10};

static GUID const * const png_formats[] = {
    &GUID_WICPixelFormatBlackWhite,
    &GUID_WICPixelFormat2bppGray,
    &GUID_WICPixelFormat4bppGray,
    &GUID_WICPixelFormat8bppGray,
    &GUID_WICPixelFormat16bppGray,
    &GUID_WICPixelFormat32bppBGRA,
    &GUID_WICPixelFormat64bppRGBA,
    &GUID_WICPixelFormat1bppIndexed,
    &GUID_WICPixelFormat2bppIndexed,
    &GUID_WICPixelFormat4bppIndexed,
    &GUID_WICPixelFormat8bppIndexed,
    &GUID_WICPixelFormat24bppBGR,
    &GUID_WICPixelFormat48bppRGB,
    NULL
};

static struct decoder_pattern const png_patterns[] = {
    {8,0,png_magic,mask_all,0},
    {0}
};

static const BYTE tiff_magic_le[] = {0x49,0x49,42,0};
static const BYTE tiff_magic_be[] = {0x4d,0x4d,0,42};

static GUID const * const tiff_formats[] = {
    &GUID_WICPixelFormatBlackWhite,
    &GUID_WICPixelFormat4bppGray,
    &GUID_WICPixelFormat8bppGray,
    &GUID_WICPixelFormat4bppIndexed,
    &GUID_WICPixelFormat8bppIndexed,
    &GUID_WICPixelFormat32bppBGR,
    &GUID_WICPixelFormat32bppBGRA,
    &GUID_WICPixelFormat32bppPBGRA,
    NULL
};

static struct decoder_pattern const tiff_patterns[] = {
    {4,0,tiff_magic_le,mask_all,0},
    {4,0,tiff_magic_be,mask_all,0},
    {0}
};

static struct regsvr_decoder const decoder_list[] = {
    {   &CLSID_WICBmpDecoder,
	"The Wine Project",
	"BMP Decoder",
	"1.0.0.0",
	&GUID_VendorMicrosoft,
	"image/bmp",
	".bmp,.dib,.rle",
	bmp_formats,
	bmp_patterns
    },
    {   &CLSID_WICGifDecoder,
	"The Wine Project",
	"GIF Decoder",
	"1.0.0.0",
	&GUID_VendorMicrosoft,
	"image/gif",
	".gif",
	gif_formats,
	gif_patterns
    },
    {   &CLSID_WICIcoDecoder,
	"The Wine Project",
	"ICO Decoder",
	"1.0.0.0",
	&GUID_VendorMicrosoft,
	"image/vnd.microsoft.icon",
	".ico",
	ico_formats,
	ico_patterns
    },
    {   &CLSID_WICJpegDecoder,
	"The Wine Project",
	"JPEG Decoder",
	"1.0.0.0",
	&GUID_VendorMicrosoft,
	"image/jpeg",
	".jpg;.jpeg;.jfif",
	jpeg_formats,
	jpeg_patterns
    },
    {   &CLSID_WICPngDecoder,
	"The Wine Project",
	"PNG Decoder",
	"1.0.0.0",
	&GUID_VendorMicrosoft,
	"image/png",
	".png",
	png_formats,
	png_patterns
    },
    {   &CLSID_WICTiffDecoder,
	"The Wine Project",
	"TIFF Decoder",
	"1.0.0.0",
	&GUID_VendorMicrosoft,
	"image/tiff",
	".tif;.tiff",
	tiff_formats,
	tiff_patterns
    },
    { NULL }			/* list terminator */
};

static GUID const * const converter_formats[] = {
    &GUID_WICPixelFormat1bppIndexed,
    &GUID_WICPixelFormat2bppIndexed,
    &GUID_WICPixelFormat4bppIndexed,
    &GUID_WICPixelFormat8bppIndexed,
    &GUID_WICPixelFormatBlackWhite,
    &GUID_WICPixelFormat2bppGray,
    &GUID_WICPixelFormat4bppGray,
    &GUID_WICPixelFormat8bppGray,
    &GUID_WICPixelFormat16bppGray,
    &GUID_WICPixelFormat16bppBGR555,
    &GUID_WICPixelFormat16bppBGR565,
    &GUID_WICPixelFormat24bppBGR,
    &GUID_WICPixelFormat32bppBGR,
    &GUID_WICPixelFormat32bppBGRA,
    &GUID_WICPixelFormat48bppRGB,
    &GUID_WICPixelFormat64bppRGBA,
    NULL
};

static struct regsvr_converter const converter_list[] = {
    {   &CLSID_WICDefaultFormatConverter,
	"The Wine Project",
	"Default Pixel Format Converter",
	"1.0.0.0",
	&GUID_VendorMicrosoft,
	converter_formats
    },
    { NULL }			/* list terminator */
};

HRESULT WINAPI DllRegisterServer(void)
{
    HRESULT hr;

    TRACE("\n");

    hr = register_coclasses(coclass_list);
    if (SUCCEEDED(hr))
        register_decoders(decoder_list);
    if (SUCCEEDED(hr))
        register_converters(converter_list);
    return hr;
}

HRESULT WINAPI DllUnregisterServer(void)
{
    HRESULT hr;

    TRACE("\n");

    hr = unregister_coclasses(coclass_list);
    if (SUCCEEDED(hr))
        unregister_decoders(decoder_list);
    if (SUCCEEDED(hr))
        unregister_converters(converter_list);
    return hr;
}
