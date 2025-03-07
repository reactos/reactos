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

#include "wine/debug.h"

#include "wincodecs_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

/***********************************************************************
 *		interface for self-registering
 */
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
    GUID const *container_format;
    LPCSTR mimetypes;
    LPCSTR extensions;
    GUID const * const *formats;
    const struct decoder_pattern *patterns;
};

static HRESULT register_decoders(struct regsvr_decoder const *list);
static HRESULT unregister_decoders(struct regsvr_decoder const *list);

struct regsvr_encoder
{
    CLSID const *clsid;         /* NULL for end of list */
    LPCSTR author;
    LPCSTR friendlyname;
    LPCSTR version;
    GUID const *vendor;
    GUID const *container_format;
    LPCSTR mimetypes;
    LPCSTR extensions;
    GUID const * const *formats;
};

static HRESULT register_encoders(struct regsvr_encoder const *list);
static HRESULT unregister_encoders(struct regsvr_encoder const *list);

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

struct metadata_pattern
{
    DWORD position;
    DWORD length;
    const BYTE *pattern;
    const BYTE *mask;
    DWORD data_offset;
};

struct reader_containers
{
    GUID const *format;
    const struct metadata_pattern *patterns;
};

struct regsvr_metadatareader
{
    CLSID const *clsid;         /* NULL for end of list */
    LPCSTR author;
    LPCSTR friendlyname;
    LPCSTR version;
    LPCSTR specversion;
    GUID const *vendor;
    GUID const *metadata_format;
    DWORD requires_fullstream;
    DWORD supports_padding;
    DWORD requires_fixedsize;
    const struct reader_containers *containers;
};

static HRESULT register_metadatareaders(struct regsvr_metadatareader const *list);
static HRESULT unregister_metadatareaders(struct regsvr_metadatareader const *list);

struct regsvr_pixelformat
{
    CLSID const *clsid;         /* NULL for end of list */
    LPCSTR author;
    LPCSTR friendlyname;
    LPCSTR version;
    GUID const *vendor;
    UINT bitsperpixel;
    UINT channelcount;
    BYTE const * const *channelmasks;
    WICPixelFormatNumericRepresentation numericrepresentation;
    UINT supportsalpha;
};

static HRESULT register_pixelformats(struct regsvr_pixelformat const *list);
static HRESULT unregister_pixelformats(struct regsvr_pixelformat const *list);

/***********************************************************************
 *		static string constants
 */
static const WCHAR clsid_keyname[] = L"CLSID";
static const char author_valuename[] = "Author";
static const char friendlyname_valuename[] = "FriendlyName";
static const WCHAR vendor_valuename[] = L"Vendor";
static const WCHAR containerformat_valuename[] = L"ContainerFormat";
static const char version_valuename[] = "Version";
static const char mimetypes_valuename[] = "MimeTypes";
static const char extensions_valuename[] = "FileExtensions";
static const WCHAR formats_keyname[] = L"Formats";
static const WCHAR patterns_keyname[] = L"Patterns";
static const WCHAR instance_keyname[] = L"Instance";
static const WCHAR clsid_valuename[] = L"CLSID";
static const char length_valuename[] = "Length";
static const char position_valuename[] = "Position";
static const char pattern_valuename[] = "Pattern";
static const char mask_valuename[] = "Mask";
static const char endofstream_valuename[] = "EndOfStream";
static const WCHAR pixelformats_keyname[] = L"PixelFormats";
static const WCHAR metadataformat_valuename[] = L"MetadataFormat";
static const char specversion_valuename[] = "SpecVersion";
static const char requiresfullstream_valuename[] = "RequiresFullStream";
static const char supportspadding_valuename[] = "SupportsPadding";
static const char requiresfixedsize_valuename[] = "FixedSize";
static const WCHAR containers_keyname[] = L"Containers";
static const char dataoffset_valuename[] = "DataOffset";
static const char bitsperpixel_valuename[] = "BitLength";
static const char channelcount_valuename[] = "ChannelCount";
static const char numericrepresentation_valuename[] = "NumericRepresentation";
static const char supportstransparency_valuename[] = "SupportsTransparency";
static const WCHAR channelmasks_keyname[] = L"ChannelMasks";

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
                                 (const BYTE*)buf, 78);
	    RegCloseKey(instance_clsid_key);
	}
	if (res != ERROR_SUCCESS) goto error_close_clsid_key;

        if (list->author) {
	    res = RegSetValueExA(clsid_key, author_valuename, 0, REG_SZ,
                                 (const BYTE*)list->author,
				 strlen(list->author) + 1);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->friendlyname) {
	    res = RegSetValueExA(clsid_key, friendlyname_valuename, 0, REG_SZ,
                                 (const BYTE*)list->friendlyname,
				 strlen(list->friendlyname) + 1);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->vendor) {
            StringFromGUID2(list->vendor, buf, 39);
	    res = RegSetValueExW(clsid_key, vendor_valuename, 0, REG_SZ,
                                 (const BYTE*)buf, 78);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->container_format) {
            StringFromGUID2(list->container_format, buf, 39);
	    res = RegSetValueExW(clsid_key, containerformat_valuename, 0, REG_SZ,
                                 (const BYTE*)buf, 78);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->version) {
	    res = RegSetValueExA(clsid_key, version_valuename, 0, REG_SZ,
                                 (const BYTE*)list->version,
				 strlen(list->version) + 1);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->mimetypes) {
	    res = RegSetValueExA(clsid_key, mimetypes_valuename, 0, REG_SZ,
                                 (const BYTE*)list->mimetypes,
				 strlen(list->mimetypes) + 1);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->extensions) {
	    res = RegSetValueExA(clsid_key, extensions_valuename, 0, REG_SZ,
                                 (const BYTE*)list->extensions,
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
                swprintf(buf, 39, L"%i", i);
                res = RegCreateKeyExW(patterns_key, buf, 0, NULL, 0,
                                      KEY_READ | KEY_WRITE, NULL, &pattern_key, NULL);
                if (res != ERROR_SUCCESS) break;
	        res = RegSetValueExA(pattern_key, length_valuename, 0, REG_DWORD,
                                     (const BYTE*)&list->patterns[i].length, 4);
                if (res == ERROR_SUCCESS)
	            res = RegSetValueExA(pattern_key, position_valuename, 0, REG_DWORD,
                                         (const BYTE*)&list->patterns[i].position, 4);
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
                                         (const BYTE*)&list->patterns[i].endofstream, 4);
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
 *		register_encoders
 */
static HRESULT register_encoders(struct regsvr_encoder const *list)
{
    LONG res = ERROR_SUCCESS;
    HKEY coclass_key;
    WCHAR buf[39];
    HKEY encoders_key;
    HKEY instance_key;

    res = RegCreateKeyExW(HKEY_CLASSES_ROOT, clsid_keyname, 0, NULL, 0,
			  KEY_READ | KEY_WRITE, NULL, &coclass_key, NULL);
    if (res == ERROR_SUCCESS)  {
        StringFromGUID2(&CATID_WICBitmapEncoders, buf, 39);
        res = RegCreateKeyExW(coclass_key, buf, 0, NULL, 0,
			      KEY_READ | KEY_WRITE, NULL, &encoders_key, NULL);
        if (res == ERROR_SUCCESS)
        {
            res = RegCreateKeyExW(encoders_key, instance_keyname, 0, NULL, 0,
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
                                 (const BYTE*)buf, 78);
	    RegCloseKey(instance_clsid_key);
	}
	if (res != ERROR_SUCCESS) goto error_close_clsid_key;

        if (list->author) {
	    res = RegSetValueExA(clsid_key, author_valuename, 0, REG_SZ,
                                 (const BYTE*)list->author,
				 strlen(list->author) + 1);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->friendlyname) {
	    res = RegSetValueExA(clsid_key, friendlyname_valuename, 0, REG_SZ,
                                 (const BYTE*)list->friendlyname,
				 strlen(list->friendlyname) + 1);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->vendor) {
            StringFromGUID2(list->vendor, buf, 39);
	    res = RegSetValueExW(clsid_key, vendor_valuename, 0, REG_SZ,
                                 (const BYTE*)buf, 78);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->container_format) {
            StringFromGUID2(list->container_format, buf, 39);
	    res = RegSetValueExW(clsid_key, containerformat_valuename, 0, REG_SZ,
                                 (const BYTE*)buf, 78);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->version) {
	    res = RegSetValueExA(clsid_key, version_valuename, 0, REG_SZ,
                                 (const BYTE*)list->version,
				 strlen(list->version) + 1);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->mimetypes) {
	    res = RegSetValueExA(clsid_key, mimetypes_valuename, 0, REG_SZ,
                                 (const BYTE*)list->mimetypes,
				 strlen(list->mimetypes) + 1);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->extensions) {
	    res = RegSetValueExA(clsid_key, extensions_valuename, 0, REG_SZ,
                                 (const BYTE*)list->extensions,
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

    error_close_clsid_key:
	RegCloseKey(clsid_key);
    }

error_close_coclass_key:
    RegCloseKey(instance_key);
    RegCloseKey(encoders_key);
    RegCloseKey(coclass_key);
error_return:
    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
}

/***********************************************************************
 *		unregister_encoders
 */
static HRESULT unregister_encoders(struct regsvr_encoder const *list)
{
    LONG res = ERROR_SUCCESS;
    HKEY coclass_key;
    WCHAR buf[39];
    HKEY encoders_key;
    HKEY instance_key;

    res = RegOpenKeyExW(HKEY_CLASSES_ROOT, clsid_keyname, 0,
			KEY_READ | KEY_WRITE, &coclass_key);
    if (res == ERROR_FILE_NOT_FOUND) return S_OK;

    if (res == ERROR_SUCCESS)  {
        StringFromGUID2(&CATID_WICBitmapEncoders, buf, 39);
        res = RegCreateKeyExW(coclass_key, buf, 0, NULL, 0,
			      KEY_READ | KEY_WRITE, NULL, &encoders_key, NULL);
        if (res == ERROR_SUCCESS)
        {
            res = RegCreateKeyExW(encoders_key, instance_keyname, 0, NULL, 0,
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
    RegCloseKey(encoders_key);
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
                                 (const BYTE*)buf, 78);
	    RegCloseKey(instance_clsid_key);
	}
	if (res != ERROR_SUCCESS) goto error_close_clsid_key;

        if (list->author) {
	    res = RegSetValueExA(clsid_key, author_valuename, 0, REG_SZ,
                                 (const BYTE*)list->author,
				 strlen(list->author) + 1);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->friendlyname) {
	    res = RegSetValueExA(clsid_key, friendlyname_valuename, 0, REG_SZ,
                                 (const BYTE*)list->friendlyname,
				 strlen(list->friendlyname) + 1);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->vendor) {
            StringFromGUID2(list->vendor, buf, 39);
	    res = RegSetValueExW(clsid_key, vendor_valuename, 0, REG_SZ,
                                 (const BYTE*)buf, 78);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->version) {
	    res = RegSetValueExA(clsid_key, version_valuename, 0, REG_SZ,
                                 (const BYTE*)list->version,
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
 *		register_metadatareaders
 */
static HRESULT register_metadatareaders(struct regsvr_metadatareader const *list)
{
    LONG res = ERROR_SUCCESS;
    HKEY coclass_key;
    WCHAR buf[39];
    HKEY readers_key;
    HKEY instance_key;

    res = RegCreateKeyExW(HKEY_CLASSES_ROOT, clsid_keyname, 0, NULL, 0,
			  KEY_READ | KEY_WRITE, NULL, &coclass_key, NULL);
    if (res == ERROR_SUCCESS)  {
        StringFromGUID2(&CATID_WICMetadataReader, buf, 39);
        res = RegCreateKeyExW(coclass_key, buf, 0, NULL, 0,
			      KEY_READ | KEY_WRITE, NULL, &readers_key, NULL);
        if (res == ERROR_SUCCESS)
        {
            res = RegCreateKeyExW(readers_key, instance_keyname, 0, NULL, 0,
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
                                 (const BYTE*)buf, 78);
	    RegCloseKey(instance_clsid_key);
	}
	if (res != ERROR_SUCCESS) goto error_close_clsid_key;

        if (list->author) {
	    res = RegSetValueExA(clsid_key, author_valuename, 0, REG_SZ,
                                 (const BYTE*)list->author,
				 strlen(list->author) + 1);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->friendlyname) {
	    res = RegSetValueExA(clsid_key, friendlyname_valuename, 0, REG_SZ,
                                 (const BYTE*)list->friendlyname,
				 strlen(list->friendlyname) + 1);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->vendor) {
            StringFromGUID2(list->vendor, buf, 39);
	    res = RegSetValueExW(clsid_key, vendor_valuename, 0, REG_SZ,
                                 (const BYTE*)buf, 78);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->metadata_format) {
            StringFromGUID2(list->metadata_format, buf, 39);
	    res = RegSetValueExW(clsid_key, metadataformat_valuename, 0, REG_SZ,
                                 (const BYTE*)buf, 78);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->version) {
	    res = RegSetValueExA(clsid_key, version_valuename, 0, REG_SZ,
                                 (const BYTE*)list->version,
				 strlen(list->version) + 1);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->specversion) {
	    res = RegSetValueExA(clsid_key, specversion_valuename, 0, REG_SZ,
                                 (const BYTE*)list->version,
				 strlen(list->version) + 1);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        res = RegSetValueExA(clsid_key, requiresfullstream_valuename, 0, REG_DWORD,
                             (const BYTE*)&list->requires_fullstream, 4);
        if (res != ERROR_SUCCESS) goto error_close_clsid_key;

        res = RegSetValueExA(clsid_key, supportspadding_valuename, 0, REG_DWORD,
                             (const BYTE*)&list->supports_padding, 4);
        if (res != ERROR_SUCCESS) goto error_close_clsid_key;

        if (list->requires_fixedsize) {
	    res = RegSetValueExA(clsid_key, requiresfixedsize_valuename, 0, REG_DWORD,
                                 (const BYTE*)&list->requires_fixedsize, 4);
	    if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->containers) {
            HKEY containers_key;
            const struct reader_containers *container;

            res = RegCreateKeyExW(clsid_key, containers_keyname, 0, NULL, 0,
                                  KEY_READ | KEY_WRITE, NULL, &containers_key, NULL);
            if (res != ERROR_SUCCESS) goto error_close_clsid_key;
            for (container=list->containers; container->format; ++container)
            {
                HKEY format_key;
                int i;
                StringFromGUID2(container->format, buf, 39);
                res = RegCreateKeyExW(containers_key, buf, 0, NULL, 0,
                                      KEY_READ | KEY_WRITE, NULL, &format_key, NULL);
                if (res != ERROR_SUCCESS) break;

                for (i=0; container->patterns[i].length; i++)
                {
                    HKEY pattern_key;
                    swprintf(buf, 39, L"%i", i);
                    res = RegCreateKeyExW(format_key, buf, 0, NULL, 0,
                                          KEY_READ | KEY_WRITE, NULL, &pattern_key, NULL);
                    if (res != ERROR_SUCCESS) break;
                    res = RegSetValueExA(pattern_key, position_valuename, 0, REG_DWORD,
                                         (const BYTE*)&container->patterns[i].position, 4);
                    if (res == ERROR_SUCCESS)
                        res = RegSetValueExA(pattern_key, pattern_valuename, 0, REG_BINARY,
                                             container->patterns[i].pattern,
                                             container->patterns[i].length);
                    if (res == ERROR_SUCCESS)
                        res = RegSetValueExA(pattern_key, mask_valuename, 0, REG_BINARY,
                                             container->patterns[i].mask,
                                             container->patterns[i].length);
                    if (res == ERROR_SUCCESS && container->patterns[i].data_offset)
                        res = RegSetValueExA(pattern_key, dataoffset_valuename, 0, REG_DWORD,
                                             (const BYTE*)&container->patterns[i].data_offset, 4);
                    RegCloseKey(pattern_key);
                }

                RegCloseKey(format_key);
            }
            RegCloseKey(containers_key);
        }

    error_close_clsid_key:
	RegCloseKey(clsid_key);
    }

error_close_coclass_key:
    RegCloseKey(instance_key);
    RegCloseKey(readers_key);
    RegCloseKey(coclass_key);
error_return:
    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
}

/***********************************************************************
 *		unregister_metadatareaders
 */
static HRESULT unregister_metadatareaders(struct regsvr_metadatareader const *list)
{
    LONG res = ERROR_SUCCESS;
    HKEY coclass_key;
    WCHAR buf[39];
    HKEY readers_key;
    HKEY instance_key;

    res = RegOpenKeyExW(HKEY_CLASSES_ROOT, clsid_keyname, 0,
			KEY_READ | KEY_WRITE, &coclass_key);
    if (res == ERROR_FILE_NOT_FOUND) return S_OK;

    if (res == ERROR_SUCCESS)  {
        StringFromGUID2(&CATID_WICMetadataReader, buf, 39);
        res = RegCreateKeyExW(coclass_key, buf, 0, NULL, 0,
			      KEY_READ | KEY_WRITE, NULL, &readers_key, NULL);
        if (res == ERROR_SUCCESS)
        {
            res = RegCreateKeyExW(readers_key, instance_keyname, 0, NULL, 0,
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
    RegCloseKey(readers_key);
    RegCloseKey(coclass_key);
error_return:
    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
}

/***********************************************************************
 *                register_pixelformats
 */
static HRESULT register_pixelformats(struct regsvr_pixelformat const *list)
{
    LONG res = ERROR_SUCCESS;
    HKEY coclass_key;
    WCHAR buf[39];
    HKEY formats_key;
    HKEY instance_key;

    res = RegCreateKeyExW(HKEY_CLASSES_ROOT, clsid_keyname, 0, NULL, 0,
                          KEY_READ | KEY_WRITE, NULL, &coclass_key, NULL);
    if (res == ERROR_SUCCESS)  {
        StringFromGUID2(&CATID_WICPixelFormats, buf, 39);
        res = RegCreateKeyExW(coclass_key, buf, 0, NULL, 0,
                              KEY_READ | KEY_WRITE, NULL, &formats_key, NULL);
        if (res == ERROR_SUCCESS)
        {
            res = RegCreateKeyExW(formats_key, instance_keyname, 0, NULL, 0,
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
                                 (const BYTE*)buf, 78);
            RegCloseKey(instance_clsid_key);
        }
        if (res != ERROR_SUCCESS) goto error_close_clsid_key;

        if (list->author) {
            res = RegSetValueExA(clsid_key, author_valuename, 0, REG_SZ,
                                 (const BYTE*)list->author,
                                 strlen(list->author) + 1);
            if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->friendlyname) {
            res = RegSetValueExA(clsid_key, friendlyname_valuename, 0, REG_SZ,
                                 (const BYTE*)list->friendlyname,
                                 strlen(list->friendlyname) + 1);
            if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->vendor) {
            StringFromGUID2(list->vendor, buf, 39);
            res = RegSetValueExW(clsid_key, vendor_valuename, 0, REG_SZ,
                                 (const BYTE*)buf, 78);
            if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        if (list->version) {
            res = RegSetValueExA(clsid_key, version_valuename, 0, REG_SZ,
                                 (const BYTE*)list->version,
                                 strlen(list->version) + 1);
            if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

        res = RegSetValueExA(clsid_key, bitsperpixel_valuename, 0, REG_DWORD,
                             (const BYTE*)&list->bitsperpixel, 4);
        if (res != ERROR_SUCCESS) goto error_close_clsid_key;

        res = RegSetValueExA(clsid_key, channelcount_valuename, 0, REG_DWORD,
                             (const BYTE*)&list->channelcount, 4);
        if (res != ERROR_SUCCESS) goto error_close_clsid_key;

        res = RegSetValueExA(clsid_key, numericrepresentation_valuename, 0, REG_DWORD,
                             (const BYTE*)&list->numericrepresentation, 4);
        if (res != ERROR_SUCCESS) goto error_close_clsid_key;

        res = RegSetValueExA(clsid_key, supportstransparency_valuename, 0, REG_DWORD,
                             (const BYTE*)&list->supportsalpha, 4);
        if (res != ERROR_SUCCESS) goto error_close_clsid_key;

        if (list->channelmasks) {
            HKEY masks_key;
            UINT i, mask_size;
            WCHAR mask_valuename[11];

            mask_size = (list->bitsperpixel + 7)/8;

            res = RegCreateKeyExW(clsid_key, channelmasks_keyname, 0, NULL, 0,
                                  KEY_READ | KEY_WRITE, NULL, &masks_key, NULL);
            if (res != ERROR_SUCCESS) goto error_close_clsid_key;
            for (i=0; i < list->channelcount; i++)
            {
                swprintf(mask_valuename, ARRAY_SIZE(mask_valuename), L"%d", i);
                res = RegSetValueExW(masks_key, mask_valuename, 0, REG_BINARY,
                                     list->channelmasks[i], mask_size);
                if (res != ERROR_SUCCESS) break;
            }
            RegCloseKey(masks_key);
            if (res != ERROR_SUCCESS) goto error_close_clsid_key;
        }

    error_close_clsid_key:
        RegCloseKey(clsid_key);
    }

error_close_coclass_key:
    RegCloseKey(instance_key);
    RegCloseKey(formats_key);
    RegCloseKey(coclass_key);
error_return:
    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
}

/***********************************************************************
 *                unregister_pixelformats
 */
static HRESULT unregister_pixelformats(struct regsvr_pixelformat const *list)
{
    LONG res = ERROR_SUCCESS;
    HKEY coclass_key;
    WCHAR buf[39];
    HKEY formats_key;
    HKEY instance_key;

    res = RegOpenKeyExW(HKEY_CLASSES_ROOT, clsid_keyname, 0,
                        KEY_READ | KEY_WRITE, &coclass_key);
    if (res == ERROR_FILE_NOT_FOUND) return S_OK;

    if (res == ERROR_SUCCESS)  {
        StringFromGUID2(&CATID_WICPixelFormats, buf, 39);
        res = RegCreateKeyExW(coclass_key, buf, 0, NULL, 0,
                              KEY_READ | KEY_WRITE, NULL, &formats_key, NULL);
        if (res == ERROR_SUCCESS)
        {
            res = RegCreateKeyExW(formats_key, instance_keyname, 0, NULL, 0,
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
    RegCloseKey(formats_key);
    RegCloseKey(coclass_key);
error_return:
    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
}

/***********************************************************************
 *		decoder list
 */
static const BYTE mask_all[] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};

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

static const BYTE dds_magic[] = "DDS ";

static GUID const * const dds_formats[] = {
    &GUID_WICPixelFormat32bppBGRA,
    NULL
};

static struct decoder_pattern const dds_patterns[] = {
    {4,0,dds_magic,mask_all,0},
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

static const BYTE jpeg_magic[] = {0xff, 0xd8};

static GUID const * const jpeg_formats[] = {
    &GUID_WICPixelFormat24bppBGR,
    &GUID_WICPixelFormat32bppCMYK,
    &GUID_WICPixelFormat8bppGray,
    NULL
};

static struct decoder_pattern const jpeg_patterns[] = {
    {2,0,jpeg_magic,mask_all,0},
    {0}
};

static const BYTE wmp_magic_v0[] = {0x49, 0x49, 0xbc, 0x00};
static const BYTE wmp_magic_v1[] = {0x49, 0x49, 0xbc, 0x01};

static GUID const * const wmp_formats[] = {
    &GUID_WICPixelFormat128bppRGBAFixedPoint,
    &GUID_WICPixelFormat128bppRGBAFloat,
    &GUID_WICPixelFormat128bppRGBFloat,
    &GUID_WICPixelFormat16bppBGR555,
    &GUID_WICPixelFormat16bppBGR565,
    &GUID_WICPixelFormat16bppGray,
    &GUID_WICPixelFormat16bppGrayFixedPoint,
    &GUID_WICPixelFormat16bppGrayHalf,
    &GUID_WICPixelFormat24bppBGR,
    &GUID_WICPixelFormat24bppRGB,
    &GUID_WICPixelFormat32bppBGR,
    &GUID_WICPixelFormat32bppBGR101010,
    &GUID_WICPixelFormat32bppBGRA,
    &GUID_WICPixelFormat32bppCMYK,
    &GUID_WICPixelFormat32bppGrayFixedPoint,
    &GUID_WICPixelFormat32bppGrayFloat,
    &GUID_WICPixelFormat32bppRGBE,
    &GUID_WICPixelFormat40bppCMYKAlpha,
    &GUID_WICPixelFormat48bppRGB,
    &GUID_WICPixelFormat48bppRGBFixedPoint,
    &GUID_WICPixelFormat48bppRGBHalf,
    &GUID_WICPixelFormat64bppCMYK,
    &GUID_WICPixelFormat64bppRGBA,
    &GUID_WICPixelFormat64bppRGBAFixedPoint,
    &GUID_WICPixelFormat64bppRGBAHalf,
    &GUID_WICPixelFormat80bppCMYKAlpha,
    &GUID_WICPixelFormat8bppGray,
    &GUID_WICPixelFormat96bppRGBFixedPoint,
    &GUID_WICPixelFormatBlackWhite,
    NULL
};

static struct decoder_pattern const wmp_patterns[] = {
    {4,0,wmp_magic_v0,mask_all,0},
    {4,0,wmp_magic_v1,mask_all,0},
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

static GUID const * const tiff_decode_formats[] = {
    &GUID_WICPixelFormatBlackWhite,
    &GUID_WICPixelFormat4bppGray,
    &GUID_WICPixelFormat8bppGray,
    &GUID_WICPixelFormat16bppGray,
    &GUID_WICPixelFormat32bppGrayFloat,
    &GUID_WICPixelFormat1bppIndexed,
    &GUID_WICPixelFormat2bppIndexed,
    &GUID_WICPixelFormat4bppIndexed,
    &GUID_WICPixelFormat8bppIndexed,
    &GUID_WICPixelFormat24bppBGR,
    &GUID_WICPixelFormat32bppBGR,
    &GUID_WICPixelFormat32bppBGRA,
    &GUID_WICPixelFormat32bppPBGRA,
    &GUID_WICPixelFormat48bppRGB,
    &GUID_WICPixelFormat64bppRGBA,
    &GUID_WICPixelFormat64bppPRGBA,
    &GUID_WICPixelFormat32bppCMYK,
    &GUID_WICPixelFormat64bppCMYK,
    &GUID_WICPixelFormat96bppRGBFloat,
    &GUID_WICPixelFormat128bppRGBAFloat,
    &GUID_WICPixelFormat128bppPRGBAFloat,
    NULL
};

static struct decoder_pattern const tiff_patterns[] = {
    {4,0,tiff_magic_le,mask_all,0},
    {4,0,tiff_magic_be,mask_all,0},
    {0}
};

static const BYTE tga_footer_magic[18] = "TRUEVISION-XFILE.";

static const BYTE tga_indexed_magic[18] = {0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,8,0};
static const BYTE tga_indexed_mask[18] = {0,0xff,0xf7,0,0,0,0,0,0,0,0,0,0,0,0,0,0xff,0xcf};

static const BYTE tga_truecolor_magic[18] = {0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static const BYTE tga_truecolor_mask[18] = {0,0xff,0xf7,0,0,0,0,0,0,0,0,0,0,0,0,0,0x87,0xc0};

static const BYTE tga_grayscale_magic[18] = {0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,8,0};
static const BYTE tga_grayscale_mask[18] = {0,0xff,0xf7,0,0,0,0,0,0,0,0,0,0,0,0,0,0xff,0xcf};

static GUID const * const tga_formats[] = {
    &GUID_WICPixelFormat8bppGray,
    &GUID_WICPixelFormat8bppIndexed,
    &GUID_WICPixelFormat16bppGray,
    &GUID_WICPixelFormat16bppBGR555,
    &GUID_WICPixelFormat24bppBGR,
    &GUID_WICPixelFormat32bppBGRA,
    &GUID_WICPixelFormat32bppPBGRA,
    NULL
};

static struct decoder_pattern const tga_patterns[] = {
    {18,18,tga_footer_magic,mask_all,1},
    {18,0,tga_indexed_magic,tga_indexed_mask,0},
    {18,0,tga_truecolor_magic,tga_truecolor_mask,0},
    {18,0,tga_grayscale_magic,tga_grayscale_mask,0},
    {0}
};

static struct regsvr_decoder const decoder_list[] = {
    {   &CLSID_WICBmpDecoder,
	"The Wine Project",
	"BMP Decoder",
	"1.0.0.0",
	&GUID_VendorMicrosoft,
	&GUID_ContainerFormatBmp,
	"image/bmp",
	".bmp,.dib,.rle",
	bmp_formats,
	bmp_patterns
    },
    {   &CLSID_WICDdsDecoder,
    "The Wine Project",
    "DDS Decoder",
    "1.0.0.0",
    &GUID_VendorMicrosoft,
    &GUID_ContainerFormatDds,
    "image/vnd.ms-dds",
    ".dds",
    dds_formats,
    dds_patterns
    },
    {   &CLSID_WICGifDecoder,
	"The Wine Project",
	"GIF Decoder",
	"1.0.0.0",
	&GUID_VendorMicrosoft,
	&GUID_ContainerFormatGif,
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
	&GUID_ContainerFormatIco,
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
	&GUID_ContainerFormatJpeg,
	"image/jpeg",
	".jpg;.jpeg;.jfif",
	jpeg_formats,
	jpeg_patterns
    },
    {   &CLSID_WICWmpDecoder,
	"The Wine Project",
	"JPEG-XR Decoder",
	"1.0.0.0",
	&GUID_VendorMicrosoft,
	&GUID_ContainerFormatWmp,
	"image/jxr",
	".jxr;.hdp;.wdp",
	wmp_formats,
	wmp_patterns
    },
    {   &CLSID_WICPngDecoder,
	"The Wine Project",
	"PNG Decoder",
	"1.0.0.0",
	&GUID_VendorMicrosoft,
	&GUID_ContainerFormatPng,
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
	&GUID_ContainerFormatTiff,
	"image/tiff",
	".tif;.tiff",
	tiff_decode_formats,
	tiff_patterns
    },
    {   &CLSID_WineTgaDecoder,
	"The Wine Project",
	"TGA Decoder",
	"1.0.0.0",
	&GUID_VendorWine,
	&GUID_WineContainerFormatTga,
	"image/x-targa",
	".tga;.tpic",
	tga_formats,
	tga_patterns
    },
    { NULL }			/* list terminator */
};

static GUID const * const bmp_encode_formats[] = {
    &GUID_WICPixelFormat16bppBGR555,
    &GUID_WICPixelFormat16bppBGR565,
    &GUID_WICPixelFormat24bppBGR,
    &GUID_WICPixelFormat32bppBGR,
    &GUID_WICPixelFormatBlackWhite,
    &GUID_WICPixelFormat1bppIndexed,
    &GUID_WICPixelFormat4bppIndexed,
    &GUID_WICPixelFormat8bppIndexed,
    NULL
};

static GUID const * const png_encode_formats[] = {
    &GUID_WICPixelFormat24bppBGR,
    &GUID_WICPixelFormatBlackWhite,
    &GUID_WICPixelFormat2bppGray,
    &GUID_WICPixelFormat4bppGray,
    &GUID_WICPixelFormat8bppGray,
    &GUID_WICPixelFormat16bppGray,
    &GUID_WICPixelFormat32bppBGR,
    &GUID_WICPixelFormat32bppBGRA,
    &GUID_WICPixelFormat48bppRGB,
    &GUID_WICPixelFormat64bppRGBA,
    &GUID_WICPixelFormat1bppIndexed,
    &GUID_WICPixelFormat2bppIndexed,
    &GUID_WICPixelFormat4bppIndexed,
    &GUID_WICPixelFormat8bppIndexed,
    NULL
};

static GUID const * const tiff_encode_formats[] = {
    &GUID_WICPixelFormatBlackWhite,
    &GUID_WICPixelFormat4bppGray,
    &GUID_WICPixelFormat8bppGray,
    &GUID_WICPixelFormat1bppIndexed,
    &GUID_WICPixelFormat4bppIndexed,
    &GUID_WICPixelFormat8bppIndexed,
    &GUID_WICPixelFormat24bppBGR,
    &GUID_WICPixelFormat32bppBGRA,
    &GUID_WICPixelFormat32bppPBGRA,
    &GUID_WICPixelFormat48bppRGB,
    &GUID_WICPixelFormat64bppRGBA,
    &GUID_WICPixelFormat64bppPRGBA,
    NULL
};

static struct regsvr_encoder const encoder_list[] = {
    {   &CLSID_WICBmpEncoder,
	"The Wine Project",
	"BMP Encoder",
	"1.0.0.0",
	&GUID_VendorMicrosoft,
	&GUID_ContainerFormatBmp,
	"image/bmp",
	".bmp,.dib,.rle",
	bmp_encode_formats
    },
    {   &CLSID_WICGifEncoder,
	"The Wine Project",
	"GIF Encoder",
	"1.0.0.0",
	&GUID_VendorMicrosoft,
	&GUID_ContainerFormatGif,
	"image/gif",
	".gif",
	gif_formats
    },
    {   &CLSID_WICJpegEncoder,
	"The Wine Project",
	"JPEG Encoder",
	"1.0.0.0",
	&GUID_VendorMicrosoft,
	&GUID_ContainerFormatJpeg,
	"image/jpeg",
	".jpg;.jpeg;.jfif",
	jpeg_formats
    },
    {   &CLSID_WICPngEncoder,
	"The Wine Project",
	"PNG Encoder",
	"1.0.0.0",
	&GUID_VendorMicrosoft,
	&GUID_ContainerFormatPng,
	"image/png",
	".png",
	png_encode_formats
    },
    {   &CLSID_WICTiffEncoder,
	"The Wine Project",
	"TIFF Encoder",
	"1.0.0.0",
	&GUID_VendorMicrosoft,
	&GUID_ContainerFormatTiff,
	"image/tiff",
	".tif;.tiff",
	tiff_encode_formats
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
    &GUID_WICPixelFormat16bppBGRA5551,
    &GUID_WICPixelFormat24bppBGR,
    &GUID_WICPixelFormat24bppRGB,
    &GUID_WICPixelFormat32bppBGR,
    &GUID_WICPixelFormat32bppRGB,
    &GUID_WICPixelFormat32bppBGRA,
    &GUID_WICPixelFormat32bppRGBA,
    &GUID_WICPixelFormat32bppPBGRA,
    &GUID_WICPixelFormat32bppPRGBA,
    &GUID_WICPixelFormat32bppGrayFloat,
    &GUID_WICPixelFormat48bppRGB,
    &GUID_WICPixelFormat64bppRGBA,
    &GUID_WICPixelFormat32bppCMYK,
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

static const BYTE no_magic[1] = { 0 };
static const BYTE no_mask[1] = { 0 };

static const struct metadata_pattern ifd_metadata_pattern[] = {
    { 0, 1, no_magic, no_mask, 0 },
    { 0 }
};

static const struct reader_containers ifd_containers[] = {
    {
        &GUID_ContainerFormatTiff,
        ifd_metadata_pattern
    },
    { NULL } /* list terminator */
};

static const BYTE tEXt[] = "tEXt";

static const struct metadata_pattern pngtext_metadata_pattern[] = {
    { 4, 4, tEXt, mask_all, 4 },
    { 0 }
};

static const struct reader_containers pngtext_containers[] = {
    {
        &GUID_ContainerFormatPng,
        pngtext_metadata_pattern
    },
    { NULL } /* list terminator */
};

static const BYTE gAMA[] = "gAMA";

static const struct metadata_pattern pnggama_metadata_pattern[] = {
    { 4, 4, gAMA, mask_all, 4 },
    { 0 }
};

static const struct reader_containers pnggama_containers[] = {
    {
        &GUID_ContainerFormatPng,
        pnggama_metadata_pattern
    },
    { NULL } /* list terminator */
};

static const BYTE cHRM[] = "cHRM";

static const struct metadata_pattern pngchrm_metadata_pattern[] = {
    { 4, 4, cHRM, mask_all, 4 },
    { 0 }
};

static const struct reader_containers pngchrm_containers[] = {
    {
        &GUID_ContainerFormatPng,
        pngchrm_metadata_pattern
    },
    { NULL } /* list terminator */
};

static const BYTE hIST[] = "hIST";

static const struct metadata_pattern pnghist_metadata_pattern[] = {
    { 4, 4, hIST, mask_all, 4 },
    { 0 }
};

static const struct reader_containers pnghist_containers[] = {
    {
        &GUID_ContainerFormatPng,
        pnghist_metadata_pattern
    },
    { NULL } /* list terminator */
};

static const BYTE tIME[] = "tIME";

static const struct metadata_pattern pngtime_metadata_pattern[] = {
    { 4, 4, tIME, mask_all, 4 },
    { 0 }
};

static const struct reader_containers pngtime_containers[] = {
    {
        &GUID_ContainerFormatPng,
        pngtime_metadata_pattern
    },
    { NULL } /* list terminator */
};

static const struct metadata_pattern lsd_metadata_patterns[] = {
    { 0, 6, gif87a_magic, mask_all, 0 },
    { 0, 6, gif89a_magic, mask_all, 0 },
    { 0 }
};

static const struct reader_containers lsd_containers[] = {
    {
        &GUID_ContainerFormatGif,
        lsd_metadata_patterns
    },
    { NULL } /* list terminator */
};

static const BYTE imd_magic[] = { 0x2c };

static const struct metadata_pattern imd_metadata_pattern[] = {
    { 0, 1, imd_magic, mask_all, 1 },
    { 0 }
};

static const struct reader_containers imd_containers[] = {
    {
        &GUID_ContainerFormatGif,
        imd_metadata_pattern
    },
    { NULL } /* list terminator */
};

static const BYTE gce_magic[] = { 0x21, 0xf9, 0x04 };

static const struct metadata_pattern gce_metadata_pattern[] = {
    { 0, 3, gce_magic, mask_all, 3 },
    { 0 }
};

static const struct reader_containers gce_containers[] = {
    {
        &GUID_ContainerFormatGif,
        gce_metadata_pattern
    },
    { NULL } /* list terminator */
};

static const BYTE ape_magic[] = { 0x21, 0xff, 0x0b };

static const struct metadata_pattern ape_metadata_pattern[] = {
    { 0, 3, ape_magic, mask_all, 0 },
    { 0 }
};

static const struct reader_containers ape_containers[] = {
    {
        &GUID_ContainerFormatGif,
        ape_metadata_pattern
    },
    { NULL } /* list terminator */
};

static const BYTE gif_comment_magic[] = { 0x21, 0xfe };

static const struct metadata_pattern gif_comment_metadata_pattern[] = {
    { 0, 2, gif_comment_magic, mask_all, 0 },
    { 0 }
};

static const struct reader_containers gif_comment_containers[] = {
    {
        &GUID_ContainerFormatGif,
        gif_comment_metadata_pattern
    },
    { NULL } /* list terminator */
};

static struct regsvr_metadatareader const metadatareader_list[] = {
    {   &CLSID_WICUnknownMetadataReader,
        "The Wine Project",
        "Unknown Metadata Reader",
        "1.0.0.0",
        "1.0.0.0",
        &GUID_VendorMicrosoft,
        &GUID_MetadataFormatUnknown,
        0, 0, 0,
        NULL
    },
    {   &CLSID_WICIfdMetadataReader,
        "The Wine Project",
        "Ifd Reader",
        "1.0.0.0",
        "1.0.0.0",
        &GUID_VendorMicrosoft,
        &GUID_MetadataFormatIfd,
        1, 1, 0,
        ifd_containers
    },
    {   &CLSID_WICPngChrmMetadataReader,
        "The Wine Project",
        "Chunk cHRM Reader",
        "1.0.0.0",
        "1.0.0.0",
        &GUID_VendorMicrosoft,
        &GUID_MetadataFormatChunkcHRM,
        0, 0, 0,
        pngchrm_containers
    },
    {   &CLSID_WICPngGamaMetadataReader,
        "The Wine Project",
        "Chunk gAMA Reader",
        "1.0.0.0",
        "1.0.0.0",
        &GUID_VendorMicrosoft,
        &GUID_MetadataFormatChunkgAMA,
        0, 0, 0,
        pnggama_containers
    },
    {   &CLSID_WICPngHistMetadataReader,
        "The Wine Project",
        "Chunk hIST Reader",
        "1.0.0.0",
        "1.0.0.0",
        &GUID_VendorMicrosoft,
        &GUID_MetadataFormatChunkhIST,
        0, 0, 0,
        pnghist_containers
    },
    {   &CLSID_WICPngTextMetadataReader,
        "The Wine Project",
        "Chunk tEXt Reader",
        "1.0.0.0",
        "1.0.0.0",
        &GUID_VendorMicrosoft,
        &GUID_MetadataFormatChunktEXt,
        0, 0, 0,
        pngtext_containers
    },
    {   &CLSID_WICPngTimeMetadataReader,
        "The Wine Project",
        "Chunk tIME Reader",
        "1.0.0.0",
        "1.0.0.0",
        &GUID_VendorMicrosoft,
        &GUID_MetadataFormatChunktIME,
        0, 0, 0,
        pngtime_containers
    },
    {   &CLSID_WICLSDMetadataReader,
        "The Wine Project",
        "Logical Screen Descriptor Reader",
        "1.0.0.0",
        "1.0.0.0",
        &GUID_VendorMicrosoft,
        &GUID_MetadataFormatLSD,
        0, 0, 0,
        lsd_containers
    },
    {   &CLSID_WICIMDMetadataReader,
        "The Wine Project",
        "Image Descriptor Reader",
        "1.0.0.0",
        "1.0.0.0",
        &GUID_VendorMicrosoft,
        &GUID_MetadataFormatIMD,
        0, 0, 0,
        imd_containers
    },
    {   &CLSID_WICGCEMetadataReader,
        "The Wine Project",
        "Graphic Control Extension Reader",
        "1.0.0.0",
        "1.0.0.0",
        &GUID_VendorMicrosoft,
        &GUID_MetadataFormatGCE,
        0, 0, 0,
        gce_containers
    },
    {   &CLSID_WICAPEMetadataReader,
        "The Wine Project",
        "Application Extension Reader",
        "1.0.0.0",
        "1.0.0.0",
        &GUID_VendorMicrosoft,
        &GUID_MetadataFormatAPE,
        0, 0, 0,
        ape_containers
    },
    {   &CLSID_WICGifCommentMetadataReader,
        "The Wine Project",
        "Comment Extension Reader",
        "1.0.0.0",
        "1.0.0.0",
        &GUID_VendorMicrosoft,
        &GUID_MetadataFormatGifComment,
        0, 0, 0,
        gif_comment_containers
    },
    { NULL }			/* list terminator */
};

static BYTE const channel_mask_1bit[] = { 0x01 };
static BYTE const channel_mask_2bit[] = { 0x03 };
static BYTE const channel_mask_4bit[] = { 0x0f };

static BYTE const channel_mask_8bit[] = { 0xff, 0x00, 0x00, 0x00 };
static BYTE const channel_mask_8bit2[] = { 0x00, 0xff, 0x00, 0x00 };
static BYTE const channel_mask_8bit3[] = { 0x00, 0x00, 0xff, 0x00 };
static BYTE const channel_mask_8bit4[] = { 0x00, 0x00, 0x00, 0xff };

static BYTE const channel_mask_16bit[] = { 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static BYTE const channel_mask_16bit2[] = { 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 };
static BYTE const channel_mask_16bit3[] = { 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00 };
static BYTE const channel_mask_16bit4[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff };

static BYTE const channel_mask_32bit[] = { 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 };

static BYTE const channel_mask_96bit1[] = { 0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
static BYTE const channel_mask_96bit2[] = { 0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00 };
static BYTE const channel_mask_96bit3[] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff };

static BYTE const channel_mask_128bit1[] = { 0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
static BYTE const channel_mask_128bit2[] = { 0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
static BYTE const channel_mask_128bit3[] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00 };
static BYTE const channel_mask_128bit4[] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff };

static BYTE const channel_mask_5bit[] = { 0x1f, 0x00 };
static BYTE const channel_mask_5bit2[] = { 0xe0, 0x03 };
static BYTE const channel_mask_5bit3[] = { 0x00, 0x7c };
static BYTE const channel_mask_5bit4[] = { 0x00, 0x80 };

static BYTE const channel_mask_BGR565_2[] = { 0xe0, 0x07 };
static BYTE const channel_mask_BGR565_3[] = { 0x00, 0xf8 };

static BYTE const * const channel_masks_1bit[] = { channel_mask_1bit };
static BYTE const * const channel_masks_2bit[] = { channel_mask_2bit };
static BYTE const * const channel_masks_4bit[] = { channel_mask_4bit };
static BYTE const * const channel_masks_8bit[] = { channel_mask_8bit,
    channel_mask_8bit2, channel_mask_8bit3, channel_mask_8bit4 };
static BYTE const * const channel_masks_16bit[] = { channel_mask_16bit,
    channel_mask_16bit2, channel_mask_16bit3, channel_mask_16bit4};

static BYTE const * const channel_masks_32bit[] = { channel_mask_32bit };
static BYTE const * const channel_masks_96bit[] = { channel_mask_96bit1, channel_mask_96bit2, channel_mask_96bit3 };
static BYTE const * const channel_masks_128bit[] = { channel_mask_128bit1, channel_mask_128bit2, channel_mask_128bit3, channel_mask_128bit4 };

static BYTE const * const channel_masks_BGRA5551[] = { channel_mask_5bit,
    channel_mask_5bit2, channel_mask_5bit3, channel_mask_5bit4 };

static BYTE const * const channel_masks_BGR565[] = { channel_mask_5bit,
    channel_mask_BGR565_2, channel_mask_BGR565_3 };

static struct regsvr_pixelformat const pixelformat_list[] = {
    {   &GUID_WICPixelFormat1bppIndexed,
        "The Wine Project",
        "1bpp Indexed",
        NULL, /* no version */
        &GUID_VendorMicrosoft,
        1, /* bitsperpixel */
        1, /* channel count */
        channel_masks_1bit,
        WICPixelFormatNumericRepresentationIndexed,
        0
    },
    {   &GUID_WICPixelFormat2bppIndexed,
        "The Wine Project",
        "2bpp Indexed",
        NULL, /* no version */
        &GUID_VendorMicrosoft,
        2, /* bitsperpixel */
        1, /* channel count */
        channel_masks_2bit,
        WICPixelFormatNumericRepresentationIndexed,
        0
    },
    {   &GUID_WICPixelFormat4bppIndexed,
        "The Wine Project",
        "4bpp Indexed",
        NULL, /* no version */
        &GUID_VendorMicrosoft,
        4, /* bitsperpixel */
        1, /* channel count */
        channel_masks_4bit,
        WICPixelFormatNumericRepresentationIndexed,
        0
    },
    {   &GUID_WICPixelFormat8bppIndexed,
        "The Wine Project",
        "8bpp Indexed",
        NULL, /* no version */
        &GUID_VendorMicrosoft,
        8, /* bitsperpixel */
        1, /* channel count */
        channel_masks_8bit,
        WICPixelFormatNumericRepresentationIndexed,
        0
    },
    {   &GUID_WICPixelFormatBlackWhite,
        "The Wine Project",
        "Black and White",
        NULL, /* no version */
        &GUID_VendorMicrosoft,
        1, /* bitsperpixel */
        1, /* channel count */
        channel_masks_1bit,
        WICPixelFormatNumericRepresentationUnsignedInteger,
        0
    },
    {   &GUID_WICPixelFormat2bppGray,
        "The Wine Project",
        "2bpp Grayscale",
        NULL, /* no version */
        &GUID_VendorMicrosoft,
        2, /* bitsperpixel */
        1, /* channel count */
        channel_masks_2bit,
        WICPixelFormatNumericRepresentationUnsignedInteger,
        0
    },
    {   &GUID_WICPixelFormat4bppGray,
        "The Wine Project",
        "4bpp Grayscale",
        NULL, /* no version */
        &GUID_VendorMicrosoft,
        4, /* bitsperpixel */
        1, /* channel count */
        channel_masks_4bit,
        WICPixelFormatNumericRepresentationUnsignedInteger,
        0
    },
    {   &GUID_WICPixelFormat8bppGray,
        "The Wine Project",
        "8bpp Grayscale",
        NULL, /* no version */
        &GUID_VendorMicrosoft,
        8, /* bitsperpixel */
        1, /* channel count */
        channel_masks_8bit,
        WICPixelFormatNumericRepresentationUnsignedInteger,
        0
    },
    {   &GUID_WICPixelFormat16bppGray,
        "The Wine Project",
        "16bpp Grayscale",
        NULL, /* no version */
        &GUID_VendorMicrosoft,
        16, /* bitsperpixel */
        1, /* channel count */
        channel_masks_16bit,
        WICPixelFormatNumericRepresentationUnsignedInteger,
        0
    },
    {   &GUID_WICPixelFormat16bppBGR555,
        "The Wine Project",
        "16bpp BGR555",
        NULL, /* no version */
        &GUID_VendorMicrosoft,
        16, /* bitsperpixel */
        3, /* channel count */
        channel_masks_BGRA5551,
        WICPixelFormatNumericRepresentationUnsignedInteger,
        0
    },
    {   &GUID_WICPixelFormat16bppBGR565,
        "The Wine Project",
        "16bpp BGR565",
        NULL, /* no version */
        &GUID_VendorMicrosoft,
        16, /* bitsperpixel */
        3, /* channel count */
        channel_masks_BGR565,
        WICPixelFormatNumericRepresentationUnsignedInteger,
        0
    },
    {   &GUID_WICPixelFormat16bppBGRA5551,
        "The Wine Project",
        "16bpp BGRA5551",
        NULL, /* no version */
        &GUID_VendorMicrosoft,
        16, /* bitsperpixel */
        4, /* channel count */
        channel_masks_BGRA5551,
        WICPixelFormatNumericRepresentationUnsignedInteger,
        1
    },
    {   &GUID_WICPixelFormat24bppBGR,
        "The Wine Project",
        "24bpp BGR",
        NULL, /* no version */
        &GUID_VendorMicrosoft,
        24, /* bitsperpixel */
        3, /* channel count */
        channel_masks_8bit,
        WICPixelFormatNumericRepresentationUnsignedInteger,
        0
    },
    {   &GUID_WICPixelFormat24bppRGB,
        "The Wine Project",
        "24bpp RGB",
        NULL, /* no version */
        &GUID_VendorMicrosoft,
        24, /* bitsperpixel */
        3, /* channel count */
        channel_masks_8bit,
        WICPixelFormatNumericRepresentationUnsignedInteger,
        0
    },
    {   &GUID_WICPixelFormat32bppBGR,
        "The Wine Project",
        "32bpp BGR",
        NULL, /* no version */
        &GUID_VendorMicrosoft,
        32, /* bitsperpixel */
        3, /* channel count */
        channel_masks_8bit,
        WICPixelFormatNumericRepresentationUnsignedInteger,
        0
    },
    {   &GUID_WICPixelFormat32bppRGB,
        "The Wine Project",
        "32bpp RGB",
        NULL, /* no version */
        &GUID_VendorMicrosoft,
        32, /* bitsperpixel */
        3, /* channel count */
        channel_masks_8bit,
        WICPixelFormatNumericRepresentationUnsignedInteger,
        0
    },
    {   &GUID_WICPixelFormat32bppBGRA,
        "The Wine Project",
        "32bpp BGRA",
        NULL, /* no version */
        &GUID_VendorMicrosoft,
        32, /* bitsperpixel */
        4, /* channel count */
        channel_masks_8bit,
        WICPixelFormatNumericRepresentationUnsignedInteger,
        1
    },
    {   &GUID_WICPixelFormat32bppRGBA,
        "The Wine Project",
        "32bpp RGBA",
        NULL, /* no version */
        &GUID_VendorMicrosoft,
        32, /* bitsperpixel */
        4, /* channel count */
        channel_masks_8bit,
        WICPixelFormatNumericRepresentationUnsignedInteger,
        1
    },
    {   &GUID_WICPixelFormat32bppPBGRA,
        "The Wine Project",
        "32bpp PBGRA",
        NULL, /* no version */
        &GUID_VendorMicrosoft,
        32, /* bitsperpixel */
        4, /* channel count */
        channel_masks_8bit,
        WICPixelFormatNumericRepresentationUnsignedInteger,
        1
    },
    {   &GUID_WICPixelFormat32bppPRGBA,
        "The Wine Project",
        "32bpp PRGBA",
        NULL, /* no version */
        &GUID_VendorMicrosoft,
        32, /* bitsperpixel */
        4, /* channel count */
        channel_masks_8bit,
        WICPixelFormatNumericRepresentationUnsignedInteger,
        1
    },
    {   &GUID_WICPixelFormat32bppGrayFloat,
        "The Wine Project",
        "32bpp GrayFloat",
        NULL, /* no version */
        &GUID_VendorMicrosoft,
        32, /* bitsperpixel */
        1, /* channel count */
        channel_masks_32bit,
        WICPixelFormatNumericRepresentationFloat,
        0
    },
    {   &GUID_WICPixelFormat48bppRGB,
        "The Wine Project",
        "48bpp RGB",
        NULL, /* no version */
        &GUID_VendorMicrosoft,
        48, /* bitsperpixel */
        3, /* channel count */
        channel_masks_16bit,
        WICPixelFormatNumericRepresentationUnsignedInteger,
        0
    },
    {   &GUID_WICPixelFormat64bppRGBA,
        "The Wine Project",
        "64bpp RGBA",
        NULL, /* no version */
        &GUID_VendorMicrosoft,
        64, /* bitsperpixel */
        4, /* channel count */
        channel_masks_16bit,
        WICPixelFormatNumericRepresentationUnsignedInteger,
        1
    },
    {   &GUID_WICPixelFormat64bppPRGBA,
        "The Wine Project",
        "64bpp PRGBA",
        NULL, /* no version */
        &GUID_VendorMicrosoft,
        64, /* bitsperpixel */
        4, /* channel count */
        channel_masks_16bit,
        WICPixelFormatNumericRepresentationUnsignedInteger,
        1
    },
    {   &GUID_WICPixelFormat32bppCMYK,
        "The Wine Project",
        "32bpp CMYK",
        NULL, /* no version */
        &GUID_VendorMicrosoft,
        32, /* bitsperpixel */
        4, /* channel count */
        channel_masks_8bit,
        WICPixelFormatNumericRepresentationUnsignedInteger,
        0
    },
    {   &GUID_WICPixelFormat64bppCMYK,
        "The Wine Project",
        "64bpp CMYK",
        NULL, /* no version */
        &GUID_VendorMicrosoft,
        64, /* bitsperpixel */
        4, /* channel count */
        channel_masks_16bit,
        WICPixelFormatNumericRepresentationUnsignedInteger,
        0
    },
    {   &GUID_WICPixelFormat96bppRGBFloat,
        "The Wine Project",
        "96bpp RGBFloat",
        NULL, /* no version */
        &GUID_VendorMicrosoft,
        96, /* bitsperpixel */
        3, /* channel count */
        channel_masks_96bit,
        WICPixelFormatNumericRepresentationFloat,
        0
    },
    {   &GUID_WICPixelFormat128bppRGBAFloat,
        "The Wine Project",
        "128bpp RGBAFloat",
        NULL, /* no version */
        &GUID_VendorMicrosoft,
        128, /* bitsperpixel */
        4, /* channel count */
        channel_masks_128bit,
        WICPixelFormatNumericRepresentationFloat,
        1
    },
    {   &GUID_WICPixelFormat128bppPRGBAFloat,
        "The Wine Project",
        "128bpp PRGBAFloat",
        NULL, /* no version */
        &GUID_VendorMicrosoft,
        128, /* bitsperpixel */
        4, /* channel count */
        channel_masks_128bit,
        WICPixelFormatNumericRepresentationFloat,
        1
    },
    { NULL }			/* list terminator */
};

struct regsvr_category
{
    const CLSID *clsid; /* NULL for end of list */
};

static const struct regsvr_category category_list[] = {
    { &CATID_WICBitmapDecoders },
    { &CATID_WICBitmapEncoders },
    { &CATID_WICFormatConverters },
    { &CATID_WICMetadataReader },
    { &CATID_WICPixelFormats },
    { NULL }
};

static HRESULT register_categories(const struct regsvr_category *list)
{
    LONG res;
    WCHAR buf[39];
    HKEY coclass_key, categories_key, instance_key;

    res = RegCreateKeyExW(HKEY_CLASSES_ROOT, clsid_keyname, 0, NULL, 0,
                          KEY_READ | KEY_WRITE, NULL, &coclass_key, NULL);
    if (res != ERROR_SUCCESS) return HRESULT_FROM_WIN32(res);

    StringFromGUID2(&CLSID_WICImagingCategories, buf, 39);
    res = RegCreateKeyExW(coclass_key, buf, 0, NULL, 0,
                          KEY_READ | KEY_WRITE, NULL, &categories_key, NULL);
    if (res != ERROR_SUCCESS)
    {
        RegCloseKey(coclass_key);
        return HRESULT_FROM_WIN32(res);
    }

    res = RegCreateKeyExW(categories_key, instance_keyname, 0, NULL, 0,
                          KEY_READ | KEY_WRITE, NULL, &instance_key, NULL);

    for (; res == ERROR_SUCCESS && list->clsid; list++)
    {
        HKEY instance_clsid_key;

        StringFromGUID2(list->clsid, buf, 39);
        res = RegCreateKeyExW(instance_key, buf, 0, NULL, 0,
                              KEY_READ | KEY_WRITE, NULL, &instance_clsid_key, NULL);
        if (res == ERROR_SUCCESS)
        {
            res = RegSetValueExW(instance_clsid_key, clsid_valuename, 0, REG_SZ,
                                 (const BYTE *)buf, 78);
            RegCloseKey(instance_clsid_key);
        }
    }

    RegCloseKey(instance_key);
    RegCloseKey(categories_key);
    RegCloseKey(coclass_key);

    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
}

static HRESULT unregister_categories(const struct regsvr_category *list)
{
    LONG res;
    WCHAR buf[39];
    HKEY coclass_key, categories_key, instance_key;

    res = RegOpenKeyExW(HKEY_CLASSES_ROOT, clsid_keyname, 0,
                        KEY_READ | KEY_WRITE, &coclass_key);
    if (res != ERROR_SUCCESS) return HRESULT_FROM_WIN32(res);

    StringFromGUID2(&CLSID_WICImagingCategories, buf, 39);
    res = RegOpenKeyExW(coclass_key, buf, 0,
                        KEY_READ | KEY_WRITE, &categories_key);
    if (res != ERROR_SUCCESS)
    {
        if (res == ERROR_FILE_NOT_FOUND) res = ERROR_SUCCESS;
        RegCloseKey(coclass_key);
        return HRESULT_FROM_WIN32(res);
    }

    res = RegOpenKeyExW(categories_key, instance_keyname, 0,
                          KEY_READ | KEY_WRITE, &instance_key);

    for (; res == ERROR_SUCCESS && list->clsid; list++)
    {
        StringFromGUID2(list->clsid, buf, 39);
        res = RegDeleteTreeW(instance_key, buf);
    }

    RegCloseKey(instance_key);
    RegCloseKey(categories_key);

    StringFromGUID2(&CLSID_WICImagingCategories, buf, 39);
    res = RegDeleteTreeW(coclass_key, buf);

    RegCloseKey(coclass_key);

    return res != ERROR_SUCCESS ? HRESULT_FROM_WIN32(res) : S_OK;
}

extern HRESULT WINAPI WIC_DllRegisterServer(void);
extern HRESULT WINAPI WIC_DllUnregisterServer(void);

HRESULT WINAPI DllRegisterServer(void)
{
    HRESULT hr;

    TRACE("\n");

    hr = WIC_DllRegisterServer();
    if (SUCCEEDED(hr))
        hr = register_categories(category_list);
    if (SUCCEEDED(hr))
        hr = register_decoders(decoder_list);
    if (SUCCEEDED(hr))
        hr = register_encoders(encoder_list);
    if (SUCCEEDED(hr))
        hr = register_converters(converter_list);
    if (SUCCEEDED(hr))
        hr = register_metadatareaders(metadatareader_list);
    if (SUCCEEDED(hr))
        hr = register_pixelformats(pixelformat_list);
    return hr;
}

HRESULT WINAPI DllUnregisterServer(void)
{
    HRESULT hr;

    TRACE("\n");

    hr = WIC_DllUnregisterServer();
    if (SUCCEEDED(hr))
        hr = unregister_categories(category_list);
    if (SUCCEEDED(hr))
        hr = unregister_decoders(decoder_list);
    if (SUCCEEDED(hr))
        hr = unregister_encoders(encoder_list);
    if (SUCCEEDED(hr))
        hr = unregister_converters(converter_list);
    if (SUCCEEDED(hr))
        hr = unregister_metadatareaders(metadatareader_list);
    if (SUCCEEDED(hr))
        hr = unregister_pixelformats(pixelformat_list);
    return hr;
}
