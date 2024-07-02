/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2005 Aric Stewart for CodeWeavers
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
#include "winreg.h"
#include "winnls.h"
#include "shlwapi.h"
#include "wine/debug.h"
#include "msi.h"
#include "msiquery.h"
#include "msipriv.h"
#include "wincrypt.h"
#include "winver.h"
#include "winuser.h"
#include "sddl.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

/*
 * These apis are defined in MSI 3.0
 */

struct media_info
{
    struct list entry;
    LPWSTR  path;
    WCHAR   szIndex[10];
    DWORD   index;
};

static UINT OpenSourceKey(LPCWSTR szProduct, HKEY* key, DWORD dwOptions,
                          MSIINSTALLCONTEXT context, BOOL create)
{
    HKEY rootkey = 0;
    UINT rc = ERROR_FUNCTION_FAILED;

    if (context == MSIINSTALLCONTEXT_USERUNMANAGED)
    {
        if (dwOptions & MSICODE_PATCH)
            rc = MSIREG_OpenUserPatchesKey(szProduct, &rootkey, create);
        else
            rc = MSIREG_OpenProductKey(szProduct, NULL, context,
                                       &rootkey, create);
    }
    else if (context == MSIINSTALLCONTEXT_USERMANAGED)
    {
        if (dwOptions & MSICODE_PATCH)
            rc = MSIREG_OpenUserPatchesKey(szProduct, &rootkey, create);
        else
            rc = MSIREG_OpenProductKey(szProduct, NULL, context,
                                       &rootkey, create);
    }
    else if (context == MSIINSTALLCONTEXT_MACHINE)
    {
        if (dwOptions & MSICODE_PATCH)
            rc = MSIREG_OpenPatchesKey(szProduct, &rootkey, create);
        else
            rc = MSIREG_OpenProductKey(szProduct, NULL, context,
                                       &rootkey, create);
    }

    if (rc != ERROR_SUCCESS)
    {
        if (dwOptions & MSICODE_PATCH)
            return ERROR_UNKNOWN_PATCH;
        else
            return ERROR_UNKNOWN_PRODUCT;
    }

    if (create)
        rc = RegCreateKeyW(rootkey, L"SourceList", key);
    else
    {
        rc = RegOpenKeyW(rootkey, L"SourceList", key);
        if (rc != ERROR_SUCCESS)
            rc = ERROR_BAD_CONFIGURATION;
    }
    RegCloseKey(rootkey);

    return rc;
}

static UINT OpenMediaSubkey(HKEY rootkey, HKEY *key, BOOL create)
{
    UINT rc;

    if (create)
        rc = RegCreateKeyW(rootkey, L"Media", key);
    else
        rc = RegOpenKeyW(rootkey, L"Media", key);

    return rc;
}

static UINT OpenNetworkSubkey(HKEY rootkey, HKEY *key, BOOL create)
{
    UINT rc;

    if (create)
        rc = RegCreateKeyW(rootkey, L"Net", key);
    else
        rc = RegOpenKeyW(rootkey, L"Net", key);

    return rc;
}

static UINT OpenURLSubkey(HKEY rootkey, HKEY *key, BOOL create)
{
    UINT rc;

    if (create)
        rc = RegCreateKeyW(rootkey, L"URL", key);
    else
        rc = RegOpenKeyW(rootkey, L"URL", key);

    return rc;
}

/******************************************************************
 *  MsiSourceListEnumMediaDisksA   (MSI.@)
 */
UINT WINAPI MsiSourceListEnumMediaDisksA( const char *szProductCodeOrPatchCode, const char *szUserSid,
                                          MSIINSTALLCONTEXT dwContext, DWORD dwOptions, DWORD dwIndex,
                                          DWORD *pdwDiskId, char *szVolumeLabel, DWORD *pcchVolumeLabel,
                                          char *szDiskPrompt, DWORD *pcchDiskPrompt )
{
    WCHAR *product = NULL, *usersid = NULL, *volume = NULL, *prompt = NULL;
    UINT r = ERROR_INVALID_PARAMETER;

    TRACE( "%s, %s, %d, %#lx, %lu, %p, %p, %p, %p, %p\n", debugstr_a(szProductCodeOrPatchCode),
           debugstr_a(szUserSid), dwContext, dwOptions, dwIndex, pdwDiskId, szVolumeLabel, pcchVolumeLabel,
           szDiskPrompt, pcchDiskPrompt );

    if (szDiskPrompt && !pcchDiskPrompt)
        return ERROR_INVALID_PARAMETER;

    if (szProductCodeOrPatchCode) product = strdupAtoW(szProductCodeOrPatchCode);
    if (szUserSid) usersid = strdupAtoW(szUserSid);

    /* FIXME: add tests for an invalid format */

    if (pcchVolumeLabel)
        volume = malloc(*pcchVolumeLabel * sizeof(WCHAR));

    if (pcchDiskPrompt)
        prompt = malloc(*pcchDiskPrompt * sizeof(WCHAR));

    if (volume) *volume = '\0';
    if (prompt) *prompt = '\0';
    r = MsiSourceListEnumMediaDisksW(product, usersid, dwContext, dwOptions,
                                     dwIndex, pdwDiskId, volume, pcchVolumeLabel,
                                     prompt, pcchDiskPrompt);
    if (r != ERROR_SUCCESS)
        goto done;

    if (szVolumeLabel && pcchVolumeLabel)
        WideCharToMultiByte(CP_ACP, 0, volume, -1, szVolumeLabel,
                            *pcchVolumeLabel + 1, NULL, NULL);

    if (szDiskPrompt)
        WideCharToMultiByte(CP_ACP, 0, prompt, -1, szDiskPrompt,
                            *pcchDiskPrompt + 1, NULL, NULL);

done:
    free(product);
    free(usersid);
    free(volume);
    free(prompt);

    return r;
}

/******************************************************************
 *  MsiSourceListEnumMediaDisksW   (MSI.@)
 */
UINT WINAPI MsiSourceListEnumMediaDisksW( const WCHAR *szProductCodeOrPatchCode, const WCHAR *szUserSid,
                                          MSIINSTALLCONTEXT dwContext, DWORD dwOptions, DWORD dwIndex,
                                          DWORD *pdwDiskId, WCHAR *szVolumeLabel, DWORD *pcchVolumeLabel,
                                          WCHAR *szDiskPrompt, DWORD *pcchDiskPrompt )
{
    WCHAR squashed_pc[SQUASHED_GUID_SIZE], convert[11];
    WCHAR *value = NULL, *data = NULL, *ptr, *ptr2;
    HKEY source, media;
    DWORD valuesz, datasz = 0, type, numvals, size;
    LONG res;
    UINT r;
    static DWORD index = 0;

    TRACE( "%s, %s, %d, %#lx, %lu, %p, %p, %p, %p\n", debugstr_w(szProductCodeOrPatchCode),
           debugstr_w(szUserSid), dwContext, dwOptions, dwIndex, szVolumeLabel, pcchVolumeLabel,
           szDiskPrompt, pcchDiskPrompt );

    if (!szProductCodeOrPatchCode || !squash_guid( szProductCodeOrPatchCode, squashed_pc ))
        return ERROR_INVALID_PARAMETER;

    if (dwContext == MSIINSTALLCONTEXT_MACHINE && szUserSid)
        return ERROR_INVALID_PARAMETER;

    if (dwOptions != MSICODE_PRODUCT && dwOptions != MSICODE_PATCH)
        return ERROR_INVALID_PARAMETER;

    if (szDiskPrompt && !pcchDiskPrompt)
        return ERROR_INVALID_PARAMETER;

    if (dwIndex == 0)
        index = 0;

    if (dwIndex != index)
        return ERROR_INVALID_PARAMETER;

    r = OpenSourceKey(szProductCodeOrPatchCode, &source, dwOptions, dwContext, FALSE);
    if (r != ERROR_SUCCESS)
        return r;

    r = OpenMediaSubkey(source, &media, FALSE);
    if (r != ERROR_SUCCESS)
    {
        RegCloseKey(source);
        return ERROR_NO_MORE_ITEMS;
    }

    res = RegQueryInfoKeyW(media, NULL, NULL, NULL, NULL, NULL,
                           NULL, &numvals, &valuesz, &datasz, NULL, NULL);
    if (res != ERROR_SUCCESS)
    {
        r = ERROR_BAD_CONFIGURATION;
        goto done;
    }

    value = malloc(++valuesz * sizeof(WCHAR));
    data = malloc(++datasz * sizeof(WCHAR));
    if (!value || !data)
    {
        r = ERROR_OUTOFMEMORY;
        goto done;
    }

    r = RegEnumValueW(media, dwIndex, value, &valuesz,
                      NULL, &type, (LPBYTE)data, &datasz);
    if (r != ERROR_SUCCESS)
        goto done;

    if (pdwDiskId)
        *pdwDiskId = wcstol(value, NULL, 10);

    ptr2 = data;
    ptr = wcschr(data, ';');
    if (!ptr)
        ptr = data;
    else
        *ptr = '\0';

    if (pcchVolumeLabel)
    {
        if (type == REG_DWORD)
        {
            swprintf(convert, ARRAY_SIZE(convert), L"#%d", *data);
            size = lstrlenW(convert);
            ptr2 = convert;
        }
        else
            size = lstrlenW(data);

        if (size >= *pcchVolumeLabel)
            r = ERROR_MORE_DATA;
        else if (szVolumeLabel)
            lstrcpyW(szVolumeLabel, ptr2);

        *pcchVolumeLabel = size;
    }

    if (pcchDiskPrompt)
    {
        if (!*ptr)
            ptr++;

        if (type == REG_DWORD)
        {
            swprintf(convert, ARRAY_SIZE(convert), L"#%d", *ptr);
            size = lstrlenW(convert);
            ptr = convert;
        }
        else
            size = lstrlenW(ptr);

        if (size >= *pcchDiskPrompt)
            r = ERROR_MORE_DATA;
        else if (szDiskPrompt)
            lstrcpyW(szDiskPrompt, ptr);

        *pcchDiskPrompt = size;
    }

    index++;

done:
    free(value);
    free(data);
    RegCloseKey(source);

    return r;
}

/******************************************************************
 *  MsiSourceListEnumSourcesA   (MSI.@)
 */
UINT WINAPI MsiSourceListEnumSourcesA( const char *szProductCodeOrPatch, const char *szUserSid,
                                       MSIINSTALLCONTEXT dwContext, DWORD dwOptions, DWORD dwIndex, char *szSource,
                                       DWORD *pcchSource )
{
    WCHAR *product = NULL, *usersid = NULL, *source = NULL;
    DWORD len = 0;
    UINT r = ERROR_INVALID_PARAMETER;
    static DWORD index = 0;

    TRACE( "%s, %s, %d, %#lx, %lu, %p, %p)\n", debugstr_a(szProductCodeOrPatch), debugstr_a(szUserSid), dwContext,
           dwOptions, dwIndex, szSource, pcchSource );

    if (dwIndex == 0)
        index = 0;

    if (szSource && !pcchSource)
        goto done;

    if (dwIndex != index)
        goto done;

    if (szProductCodeOrPatch) product = strdupAtoW(szProductCodeOrPatch);
    if (szUserSid) usersid = strdupAtoW(szUserSid);

    r = MsiSourceListEnumSourcesW(product, usersid, dwContext, dwOptions,
                                  dwIndex, NULL, &len);
    if (r != ERROR_SUCCESS)
        goto done;

    source = malloc(++len * sizeof(WCHAR));
    if (!source)
    {
        r = ERROR_OUTOFMEMORY;
        goto done;
    }

    *source = '\0';
    r = MsiSourceListEnumSourcesW(product, usersid, dwContext, dwOptions,
                                  dwIndex, source, &len);
    if (r != ERROR_SUCCESS)
        goto done;

    len = WideCharToMultiByte(CP_ACP, 0, source, -1, NULL, 0, NULL, NULL);
    if (pcchSource && *pcchSource >= len)
        WideCharToMultiByte(CP_ACP, 0, source, -1, szSource, len, NULL, NULL);
    else if (szSource)
        r = ERROR_MORE_DATA;

    if (pcchSource)
        *pcchSource = len - 1;

done:
    free(product);
    free(usersid);
    free(source);

    if (r == ERROR_SUCCESS)
    {
        if (szSource || !pcchSource) index++;
    }
    else if (dwIndex > index)
        index = 0;

    return r;
}

/******************************************************************
 *  MsiSourceListEnumSourcesW   (MSI.@)
 */
UINT WINAPI MsiSourceListEnumSourcesW( const WCHAR *szProductCodeOrPatch, const WCHAR *szUserSid,
                                       MSIINSTALLCONTEXT dwContext, DWORD dwOptions, DWORD dwIndex, WCHAR *szSource,
                                       DWORD *pcchSource )
{
    WCHAR squashed_pc[SQUASHED_GUID_SIZE], name[32];
    HKEY source = NULL, subkey = NULL;
    LONG res;
    UINT r = ERROR_INVALID_PARAMETER;
    static DWORD index = 0;

    TRACE( "%s, %s, %d, %#lx, %lu, %p, %p\n", debugstr_w(szProductCodeOrPatch), debugstr_w(szUserSid), dwContext,
           dwOptions, dwIndex, szSource, pcchSource );

    if (dwIndex == 0)
        index = 0;

    if (!szProductCodeOrPatch || !squash_guid( szProductCodeOrPatch, squashed_pc ))
        goto done;

    if (szSource && !pcchSource)
        goto done;

    if (!(dwOptions & (MSISOURCETYPE_NETWORK | MSISOURCETYPE_URL)))
        goto done;

    if ((dwOptions & MSISOURCETYPE_NETWORK) && (dwOptions & MSISOURCETYPE_URL))
        goto done;

    if (dwContext == MSIINSTALLCONTEXT_MACHINE && szUserSid)
        goto done;

    if (dwIndex != index)
        goto done;

    r = OpenSourceKey( szProductCodeOrPatch, &source, dwOptions, dwContext, FALSE );
    if (r != ERROR_SUCCESS)
        goto done;

    if (dwOptions & MSISOURCETYPE_NETWORK)
        r = OpenNetworkSubkey(source, &subkey, FALSE);
    else if (dwOptions & MSISOURCETYPE_URL)
        r = OpenURLSubkey(source, &subkey, FALSE);

    if (r != ERROR_SUCCESS)
    {
        r = ERROR_NO_MORE_ITEMS;
        goto done;
    }

    swprintf(name, ARRAY_SIZE(name), L"%d", dwIndex + 1);

    res = RegQueryValueExW(subkey, name, 0, 0, (LPBYTE)szSource, pcchSource);
    if (res != ERROR_SUCCESS && res != ERROR_MORE_DATA)
        r = ERROR_NO_MORE_ITEMS;

done:
    RegCloseKey(subkey);
    RegCloseKey(source);

    if (r == ERROR_SUCCESS)
    {
        if (szSource || !pcchSource) index++;
    }
    else if (dwIndex > index)
        index = 0;

    return r;
}

/******************************************************************
 *  MsiSourceListGetInfoA   (MSI.@)
 */
UINT WINAPI MsiSourceListGetInfoA( LPCSTR szProduct, LPCSTR szUserSid,
                                   MSIINSTALLCONTEXT dwContext, DWORD dwOptions,
                                   LPCSTR szProperty, LPSTR szValue,
                                   LPDWORD pcchValue)
{
    UINT ret;
    LPWSTR product = NULL;
    LPWSTR usersid = NULL;
    LPWSTR property = NULL;
    LPWSTR value = NULL;
    DWORD len = 0;

    if (szValue && !pcchValue)
        return ERROR_INVALID_PARAMETER;

    if (szProduct) product = strdupAtoW(szProduct);
    if (szUserSid) usersid = strdupAtoW(szUserSid);
    if (szProperty) property = strdupAtoW(szProperty);

    ret = MsiSourceListGetInfoW(product, usersid, dwContext, dwOptions,
                                property, NULL, &len);
    if (ret != ERROR_SUCCESS)
        goto done;

    value = malloc(++len * sizeof(WCHAR));
    if (!value)
        return ERROR_OUTOFMEMORY;

    *value = '\0';
    ret = MsiSourceListGetInfoW(product, usersid, dwContext, dwOptions,
                                property, value, &len);
    if (ret != ERROR_SUCCESS)
        goto done;

    len = WideCharToMultiByte(CP_ACP, 0, value, -1, NULL, 0, NULL, NULL);
    if (*pcchValue >= len)
        WideCharToMultiByte(CP_ACP, 0, value, -1, szValue, len, NULL, NULL);
    else if (szValue)
        ret = ERROR_MORE_DATA;

    *pcchValue = len - 1;

done:
    free(product);
    free(usersid);
    free(property);
    free(value);
    return ret;
}

/******************************************************************
 *  MsiSourceListGetInfoW   (MSI.@)
 */
UINT WINAPI MsiSourceListGetInfoW( LPCWSTR szProduct, LPCWSTR szUserSid,
                                   MSIINSTALLCONTEXT dwContext, DWORD dwOptions,
                                   LPCWSTR szProperty, LPWSTR szValue,
                                   LPDWORD pcchValue)
{
    WCHAR *source, *ptr, squashed_pc[SQUASHED_GUID_SIZE];
    HKEY sourcekey, media;
    DWORD size;
    UINT rc;

    TRACE("%s %s\n", debugstr_w(szProduct), debugstr_w(szProperty));

    if (!szProduct || !squash_guid( szProduct, squashed_pc ))
        return ERROR_INVALID_PARAMETER;

    if (szValue && !pcchValue)
        return ERROR_INVALID_PARAMETER;

    if (dwContext != MSIINSTALLCONTEXT_USERMANAGED &&
        dwContext != MSIINSTALLCONTEXT_USERUNMANAGED &&
        dwContext != MSIINSTALLCONTEXT_MACHINE)
        return ERROR_INVALID_PARAMETER;

    if (!szProperty)
        return ERROR_INVALID_PARAMETER;

    if (szUserSid)
        FIXME("Unhandled UserSid %s\n",debugstr_w(szUserSid));

    rc = OpenSourceKey(szProduct, &sourcekey, dwOptions, dwContext, FALSE);
    if (rc != ERROR_SUCCESS)
        return rc;

    if (!wcscmp( szProperty, INSTALLPROPERTY_MEDIAPACKAGEPATHW ) ||
        !wcscmp( szProperty, INSTALLPROPERTY_DISKPROMPTW ))
    {
        rc = OpenMediaSubkey(sourcekey, &media, FALSE);
        if (rc != ERROR_SUCCESS)
        {
            RegCloseKey(sourcekey);
            return ERROR_SUCCESS;
        }

        if (!wcscmp( szProperty, INSTALLPROPERTY_MEDIAPACKAGEPATHW ))
            szProperty = L"MediaPackage";

        RegQueryValueExW(media, szProperty, 0, 0, (LPBYTE)szValue, pcchValue);
        RegCloseKey(media);
    }
    else if (!wcscmp( szProperty, INSTALLPROPERTY_LASTUSEDSOURCEW ) ||
             !wcscmp( szProperty, INSTALLPROPERTY_LASTUSEDTYPEW ))
    {
        rc = RegQueryValueExW(sourcekey, INSTALLPROPERTY_LASTUSEDSOURCEW,
                              0, 0, NULL, &size);
        if (rc != ERROR_SUCCESS)
        {
            static WCHAR szEmpty[] = L"";
            rc = ERROR_SUCCESS;
            source = NULL;
            ptr = szEmpty;
            goto output_out;
        }

        source = malloc(size);
        RegQueryValueExW(sourcekey, INSTALLPROPERTY_LASTUSEDSOURCEW,
                         0, 0, (LPBYTE)source, &size);

        if (!*source)
        {
            free(source);
            RegCloseKey(sourcekey);
            return ERROR_SUCCESS;
        }

        if (!wcscmp( szProperty, INSTALLPROPERTY_LASTUSEDTYPEW ))
        {
            if (*source != 'n' && *source != 'u' && *source != 'm')
            {
                free(source);
                RegCloseKey(sourcekey);
                return ERROR_SUCCESS;
            }

            ptr = source;
            source[1] = '\0';
        }
        else
        {
            ptr = wcsrchr(source, ';');
            if (!ptr)
                ptr = source;
            else
                ptr++;
        }
output_out:
        if (szValue)
        {
            if (lstrlenW(ptr) < *pcchValue)
                lstrcpyW(szValue, ptr);
            else
                rc = ERROR_MORE_DATA;
        }

        *pcchValue = lstrlenW(ptr);
        free(source);
    }
    else if (!wcscmp( szProperty, INSTALLPROPERTY_PACKAGENAMEW ))
    {
        *pcchValue = *pcchValue * sizeof(WCHAR);
        rc = RegQueryValueExW(sourcekey, INSTALLPROPERTY_PACKAGENAMEW, 0, 0,
                              (LPBYTE)szValue, pcchValue);
        if (rc != ERROR_SUCCESS && rc != ERROR_MORE_DATA)
        {
            *pcchValue = 0;
            rc = ERROR_SUCCESS;
        }
        else
        {
            if (*pcchValue)
                *pcchValue = (*pcchValue - 1) / sizeof(WCHAR);
            if (szValue)
                szValue[*pcchValue] = '\0';
        }
    }
    else
    {
        FIXME("Unknown property %s\n",debugstr_w(szProperty));
        rc = ERROR_UNKNOWN_PROPERTY;
    }

    RegCloseKey(sourcekey);
    return rc;
}

/******************************************************************
 *  MsiSourceListSetInfoA   (MSI.@)
 */
UINT WINAPI MsiSourceListSetInfoA(LPCSTR szProduct, LPCSTR szUserSid,
                                  MSIINSTALLCONTEXT dwContext, DWORD dwOptions,
                                  LPCSTR szProperty, LPCSTR szValue)
{
    UINT ret;
    LPWSTR product = NULL;
    LPWSTR usersid = NULL;
    LPWSTR property = NULL;
    LPWSTR value = NULL;

    if (szProduct) product = strdupAtoW(szProduct);
    if (szUserSid) usersid = strdupAtoW(szUserSid);
    if (szProperty) property = strdupAtoW(szProperty);
    if (szValue) value = strdupAtoW(szValue);

    ret = MsiSourceListSetInfoW(product, usersid, dwContext, dwOptions,
                                property, value);

    free(product);
    free(usersid);
    free(property);
    free(value);

    return ret;
}

UINT msi_set_last_used_source(LPCWSTR product, LPCWSTR usersid,
                              MSIINSTALLCONTEXT context, DWORD options,
                              LPCWSTR value)
{
    HKEY source;
    LPWSTR buffer;
    WCHAR typechar;
    DWORD size;
    UINT r;
    int index = 1;

    if (options & MSISOURCETYPE_NETWORK)
        typechar = 'n';
    else if (options & MSISOURCETYPE_URL)
        typechar = 'u';
    else if (options & MSISOURCETYPE_MEDIA)
        typechar = 'm';
    else
        return ERROR_INVALID_PARAMETER;

    if (!(options & MSISOURCETYPE_MEDIA))
    {
        r = MsiSourceListAddSourceExW(product, usersid, context,
                                      options, value, 0);
        if (r != ERROR_SUCCESS)
            return r;

        index = 0;
        while ((r = MsiSourceListEnumSourcesW(product, usersid, context, options,
                                              index, NULL, NULL)) == ERROR_SUCCESS)
            index++;

        if (r != ERROR_NO_MORE_ITEMS)
            return r;
    }

    size = lstrlenW(L"%c;%d;%s") + lstrlenW(value) + 7;
    buffer = malloc(size * sizeof(WCHAR));
    if (!buffer)
        return ERROR_OUTOFMEMORY;

    r = OpenSourceKey(product, &source, MSICODE_PRODUCT, context, FALSE);
    if (r != ERROR_SUCCESS)
    {
        free(buffer);
        return r;
    }

    swprintf(buffer, size, L"%c;%d;%s", typechar, index, value);

    size = (lstrlenW(buffer) + 1) * sizeof(WCHAR);
    r = RegSetValueExW(source, INSTALLPROPERTY_LASTUSEDSOURCEW, 0,
                       REG_SZ, (LPBYTE)buffer, size);
    free(buffer);

    RegCloseKey(source);
    return r;
}

/******************************************************************
 *  MsiSourceListSetInfoW   (MSI.@)
 */
UINT WINAPI MsiSourceListSetInfoW( const WCHAR *szProduct, const WCHAR *szUserSid, MSIINSTALLCONTEXT dwContext,
                                   DWORD dwOptions, const WCHAR *szProperty, const WCHAR *szValue )
{
    WCHAR squashed_pc[SQUASHED_GUID_SIZE];
    HKEY sourcekey, media;
    LPCWSTR property;
    UINT rc;

    TRACE( "%s, %s, %d, %#lx, %s, %s\n", debugstr_w(szProduct), debugstr_w(szUserSid), dwContext, dwOptions,
           debugstr_w(szProperty), debugstr_w(szValue) );

    if (!szProduct || !squash_guid( szProduct, squashed_pc ))
        return ERROR_INVALID_PARAMETER;

    if (!szProperty)
        return ERROR_INVALID_PARAMETER;

    if (!szValue)
        return ERROR_UNKNOWN_PROPERTY;

    if (dwContext == MSIINSTALLCONTEXT_MACHINE && szUserSid)
        return ERROR_INVALID_PARAMETER;

    if (dwOptions & MSICODE_PATCH)
    {
        FIXME("Unhandled options MSICODE_PATCH\n");
        return ERROR_UNKNOWN_PATCH;
    }

    property = szProperty;
    if (!wcscmp( szProperty, INSTALLPROPERTY_MEDIAPACKAGEPATHW ))
        property = L"MediaPackage";

    rc = OpenSourceKey(szProduct, &sourcekey, MSICODE_PRODUCT, dwContext, FALSE);
    if (rc != ERROR_SUCCESS)
        return rc;

    if (wcscmp( szProperty, INSTALLPROPERTY_LASTUSEDSOURCEW ) &&
        dwOptions & (MSISOURCETYPE_NETWORK | MSISOURCETYPE_URL))
    {
        RegCloseKey(sourcekey);
        return ERROR_INVALID_PARAMETER;
    }

    if (!wcscmp( szProperty, INSTALLPROPERTY_MEDIAPACKAGEPATHW ) ||
        !wcscmp( szProperty, INSTALLPROPERTY_DISKPROMPTW ))
    {
        rc = OpenMediaSubkey(sourcekey, &media, TRUE);
        if (rc == ERROR_SUCCESS)
        {
            rc = msi_reg_set_val_str(media, property, szValue);
            RegCloseKey(media);
        }
    }
    else if (!wcscmp( szProperty, INSTALLPROPERTY_PACKAGENAMEW ))
    {
        DWORD size = (lstrlenW(szValue) + 1) * sizeof(WCHAR);
        rc = RegSetValueExW(sourcekey, INSTALLPROPERTY_PACKAGENAMEW, 0,
                REG_SZ, (const BYTE *)szValue, size);
        if (rc != ERROR_SUCCESS)
            rc = ERROR_UNKNOWN_PROPERTY;
    }
    else if (!wcscmp( szProperty, INSTALLPROPERTY_LASTUSEDSOURCEW ))
    {
        if (!(dwOptions & (MSISOURCETYPE_NETWORK | MSISOURCETYPE_URL)))
            rc = ERROR_INVALID_PARAMETER;
        else
            rc = msi_set_last_used_source(szProduct, szUserSid, dwContext,
                                          dwOptions, szValue);
    }
    else
        rc = ERROR_UNKNOWN_PROPERTY;

    RegCloseKey(sourcekey);
    return rc;
}

/******************************************************************
 *  MsiSourceListAddSourceW (MSI.@)
 */
UINT WINAPI MsiSourceListAddSourceW( LPCWSTR szProduct, LPCWSTR szUserName,
        DWORD dwReserved, LPCWSTR szSource)
{
    WCHAR *sidstr = NULL, squashed_pc[SQUASHED_GUID_SIZE];
    INT ret;
    DWORD sidsize = 0, domsize = 0, context;
    HKEY hkey = 0;
    UINT r;

    TRACE("%s %s %s\n", debugstr_w(szProduct), debugstr_w(szUserName), debugstr_w(szSource));

    if (!szSource || !*szSource)
        return ERROR_INVALID_PARAMETER;

    if (dwReserved != 0)
        return ERROR_INVALID_PARAMETER;

    if (!szProduct || !squash_guid( szProduct, squashed_pc ))
        return ERROR_INVALID_PARAMETER;

    if (!szUserName || !*szUserName)
        context = MSIINSTALLCONTEXT_MACHINE;
    else
    {
        if (LookupAccountNameW(NULL, szUserName, NULL, &sidsize, NULL, &domsize, NULL))
        {
            PSID psid = malloc(sidsize);

            if (LookupAccountNameW(NULL, szUserName, psid, &sidsize, NULL, &domsize, NULL))
                ConvertSidToStringSidW(psid, &sidstr);

            free(psid);
        }

        r = MSIREG_OpenProductKey(szProduct, NULL,
                                  MSIINSTALLCONTEXT_USERMANAGED, &hkey, FALSE);
        if (r == ERROR_SUCCESS)
            context = MSIINSTALLCONTEXT_USERMANAGED;
        else
        {
            r = MSIREG_OpenProductKey(szProduct, NULL,
                                      MSIINSTALLCONTEXT_USERUNMANAGED,
                                      &hkey, FALSE);
            if (r != ERROR_SUCCESS)
                return ERROR_UNKNOWN_PRODUCT;

            context = MSIINSTALLCONTEXT_USERUNMANAGED;
        }

        RegCloseKey(hkey);
    }

    ret = MsiSourceListAddSourceExW(szProduct, sidstr,
        context, MSISOURCETYPE_NETWORK, szSource, 0);

    if (sidstr)
        LocalFree(sidstr);

    return ret;
}

/******************************************************************
 *  MsiSourceListAddSourceA (MSI.@)
 */
UINT WINAPI MsiSourceListAddSourceA( LPCSTR szProduct, LPCSTR szUserName,
        DWORD dwReserved, LPCSTR szSource)
{
    INT ret;
    LPWSTR szwproduct;
    LPWSTR szwusername;
    LPWSTR szwsource;

    szwproduct = strdupAtoW( szProduct );
    szwusername = strdupAtoW( szUserName );
    szwsource = strdupAtoW( szSource );

    ret = MsiSourceListAddSourceW(szwproduct, szwusername, dwReserved, szwsource);

    free(szwproduct);
    free(szwusername);
    free(szwsource);

    return ret;
}

/******************************************************************
 *  MsiSourceListAddSourceExA (MSI.@)
 */
UINT WINAPI MsiSourceListAddSourceExA(LPCSTR szProduct, LPCSTR szUserSid,
        MSIINSTALLCONTEXT dwContext, DWORD dwOptions, LPCSTR szSource, DWORD dwIndex)
{
    UINT ret;
    LPWSTR product, usersid, source;

    product = strdupAtoW(szProduct);
    usersid = strdupAtoW(szUserSid);
    source = strdupAtoW(szSource);

    ret = MsiSourceListAddSourceExW(product, usersid, dwContext,
                                    dwOptions, source, dwIndex);

    free(product);
    free(usersid);
    free(source);

    return ret;
}

static void free_source_list(struct list *sourcelist)
{
    while (!list_empty(sourcelist))
    {
        struct media_info *info = LIST_ENTRY(list_head(sourcelist), struct media_info, entry);
        list_remove(&info->entry);
        free(info->path);
        free(info);
    }
}

static void add_source_to_list(struct list *sourcelist, struct media_info *info, DWORD *index)
{
    struct media_info *iter;
    BOOL found = FALSE;

    if (index) *index = 0;

    if (list_empty(sourcelist))
    {
        list_add_head(sourcelist, &info->entry);
        return;
    }

    LIST_FOR_EACH_ENTRY(iter, sourcelist, struct media_info, entry)
    {
        if (!found && info->index < iter->index)
        {
            found = TRUE;
            list_add_before(&iter->entry, &info->entry);
        }

        /* update the rest of the list */
        if (found)
            swprintf(iter->szIndex, ARRAY_SIZE(iter->szIndex), L"%d", ++iter->index);
        else if (index)
            (*index)++;
    }

    if (!found)
        list_add_after(&iter->entry, &info->entry);
}

static UINT fill_source_list(struct list *sourcelist, HKEY sourcekey, DWORD *count)
{
    UINT r = ERROR_SUCCESS;
    DWORD index = 0;
    WCHAR name[10];
    DWORD size, val_size;
    struct media_info *entry;

    *count = 0;

    while (r == ERROR_SUCCESS)
    {
        size = ARRAY_SIZE(name);
        r = RegEnumValueW(sourcekey, index, name, &size, NULL, NULL, NULL, &val_size);
        if (r != ERROR_SUCCESS)
            return r;

        entry = malloc(sizeof(*entry));
        if (!entry)
            goto error;

        entry->path = malloc(val_size);
        if (!entry->path)
        {
            free(entry);
            goto error;
        }

        lstrcpyW(entry->szIndex, name);
        entry->index = wcstol(name, NULL, 10);

        size++;
        r = RegEnumValueW(sourcekey, index, name, &size, NULL,
                          NULL, (LPBYTE)entry->path, &val_size);
        if (r != ERROR_SUCCESS)
        {
            free(entry->path);
            free(entry);
            goto error;
        }

        index = ++(*count);
        add_source_to_list(sourcelist, entry, NULL);
    }

error:
    *count = -1;
    free_source_list(sourcelist);
    return ERROR_OUTOFMEMORY;
}

/******************************************************************
 *  MsiSourceListAddSourceExW (MSI.@)
 */
UINT WINAPI MsiSourceListAddSourceExW( const WCHAR *szProduct, const WCHAR *szUserSid, MSIINSTALLCONTEXT dwContext,
                                       DWORD dwOptions, const WCHAR *szSource, DWORD dwIndex )
{
    HKEY sourcekey, typekey;
    UINT rc;
    struct list sourcelist;
    struct media_info *info;
    WCHAR *source, squashed_pc[SQUASHED_GUID_SIZE], name[10];
    LPCWSTR postfix;
    DWORD size, count, index;

    TRACE( "%s, %s, %d, %#lx, %s, %lu\n", debugstr_w(szProduct), debugstr_w(szUserSid), dwContext, dwOptions,
           debugstr_w(szSource), dwIndex );

    if (!szProduct || !squash_guid( szProduct, squashed_pc ))
        return ERROR_INVALID_PARAMETER;

    if (!szSource || !*szSource)
        return ERROR_INVALID_PARAMETER;

    if (!(dwOptions & (MSISOURCETYPE_NETWORK | MSISOURCETYPE_URL)))
        return ERROR_INVALID_PARAMETER;

    if (dwOptions & MSICODE_PATCH)
    {
        FIXME("Unhandled options MSICODE_PATCH\n");
        return ERROR_FUNCTION_FAILED;
    }

    if (szUserSid && (dwContext & MSIINSTALLCONTEXT_MACHINE))
        return ERROR_INVALID_PARAMETER;

    rc = OpenSourceKey(szProduct, &sourcekey, MSICODE_PRODUCT, dwContext, FALSE);
    if (rc != ERROR_SUCCESS)
        return rc;

    if (dwOptions & MSISOURCETYPE_NETWORK)
        rc = OpenNetworkSubkey(sourcekey, &typekey, TRUE);
    else if (dwOptions & MSISOURCETYPE_URL)
        rc = OpenURLSubkey(sourcekey, &typekey, TRUE);
    else if (dwOptions & MSISOURCETYPE_MEDIA)
        rc = OpenMediaSubkey(sourcekey, &typekey, TRUE);
    else
    {
        ERR( "unknown media type: %#lx\n", dwOptions );
        RegCloseKey(sourcekey);
        return ERROR_FUNCTION_FAILED;
    }
    if (rc != ERROR_SUCCESS)
    {
        ERR("can't open subkey %u\n", rc);
        RegCloseKey(sourcekey);
        return rc;
    }

    postfix = (dwOptions & MSISOURCETYPE_NETWORK) ? L"\\" : L"/";
    if (szSource[lstrlenW(szSource) - 1] == *postfix)
        source = wcsdup(szSource);
    else
    {
        size = lstrlenW(szSource) + 2;
        source = malloc(size * sizeof(WCHAR));
        lstrcpyW(source, szSource);
        lstrcatW(source, postfix);
    }

    list_init(&sourcelist);
    rc = fill_source_list(&sourcelist, typekey, &count);
    if (rc != ERROR_NO_MORE_ITEMS)
        goto done;

    size = (lstrlenW(source) + 1) * sizeof(WCHAR);

    if (count == 0)
    {
        rc = RegSetValueExW(typekey, L"1", 0, REG_EXPAND_SZ, (LPBYTE)source, size);
        goto done;
    }
    else if (dwIndex > count || dwIndex == 0)
    {
        swprintf(name, ARRAY_SIZE(name), L"%d", count + 1);
        rc = RegSetValueExW(typekey, name, 0, REG_EXPAND_SZ, (LPBYTE)source, size);
        goto done;
    }
    else
    {
        swprintf(name, ARRAY_SIZE(name), L"%d", dwIndex);
        info = malloc(sizeof(*info));
        if (!info)
        {
            rc = ERROR_OUTOFMEMORY;
            goto done;
        }

        info->path = wcsdup(source);
        lstrcpyW(info->szIndex, name);
        info->index = dwIndex;
        add_source_to_list(&sourcelist, info, &index);

        LIST_FOR_EACH_ENTRY(info, &sourcelist, struct media_info, entry)
        {
            if (info->index < index)
                continue;

            size = (lstrlenW(info->path) + 1) * sizeof(WCHAR);
            rc = RegSetValueExW(typekey, info->szIndex, 0,
                                REG_EXPAND_SZ, (LPBYTE)info->path, size);
            if (rc != ERROR_SUCCESS)
                goto done;
        }
    }

done:
    free_source_list(&sourcelist);
    free(source);
    RegCloseKey(typekey);
    RegCloseKey(sourcekey);
    return rc;
}

/******************************************************************
 *  MsiSourceListAddMediaDiskA (MSI.@)
 */
UINT WINAPI MsiSourceListAddMediaDiskA(LPCSTR szProduct, LPCSTR szUserSid,
        MSIINSTALLCONTEXT dwContext, DWORD dwOptions, DWORD dwDiskId,
        LPCSTR szVolumeLabel, LPCSTR szDiskPrompt)
{
    UINT r;
    LPWSTR product = NULL;
    LPWSTR usersid = NULL;
    LPWSTR volume = NULL;
    LPWSTR prompt = NULL;

    if (szProduct) product = strdupAtoW(szProduct);
    if (szUserSid) usersid = strdupAtoW(szUserSid);
    if (szVolumeLabel) volume = strdupAtoW(szVolumeLabel);
    if (szDiskPrompt) prompt = strdupAtoW(szDiskPrompt);

    r = MsiSourceListAddMediaDiskW(product, usersid, dwContext, dwOptions,
                                     dwDiskId, volume, prompt);

    free(product);
    free(usersid);
    free(volume);
    free(prompt);

    return r;
}

/******************************************************************
 *  MsiSourceListAddMediaDiskW (MSI.@)
 */
UINT WINAPI MsiSourceListAddMediaDiskW( const WCHAR *szProduct, const WCHAR *szUserSid, MSIINSTALLCONTEXT dwContext,
                                        DWORD dwOptions, DWORD dwDiskId, const WCHAR *szVolumeLabel,
                                        const WCHAR *szDiskPrompt )
{
    HKEY sourcekey, mediakey;
    UINT rc;
    WCHAR *buffer, squashed_pc[SQUASHED_GUID_SIZE], szIndex[10];
    DWORD size;

    TRACE( "%s, %s, %d, %#lx, %lu, %s, %s\n", debugstr_w(szProduct), debugstr_w(szUserSid), dwContext, dwOptions,
           dwDiskId, debugstr_w(szVolumeLabel), debugstr_w(szDiskPrompt) );

    if (!szProduct || !squash_guid( szProduct, squashed_pc ))
        return ERROR_INVALID_PARAMETER;

    if (dwOptions != MSICODE_PRODUCT && dwOptions != MSICODE_PATCH)
        return ERROR_INVALID_PARAMETER;

    if ((szVolumeLabel && !*szVolumeLabel) || (szDiskPrompt && !*szDiskPrompt))
        return ERROR_INVALID_PARAMETER;

    if ((dwContext & MSIINSTALLCONTEXT_MACHINE) && szUserSid)
        return ERROR_INVALID_PARAMETER;

    if (dwOptions & MSICODE_PATCH)
    {
        FIXME("Unhandled options MSICODE_PATCH\n");
        return ERROR_FUNCTION_FAILED;
    }

    rc = OpenSourceKey(szProduct, &sourcekey, MSICODE_PRODUCT, dwContext, FALSE);
    if (rc != ERROR_SUCCESS)
        return rc;

    OpenMediaSubkey(sourcekey, &mediakey, TRUE);

    swprintf(szIndex, ARRAY_SIZE(szIndex), L"%d", dwDiskId);

    size = 2;
    if (szVolumeLabel) size += lstrlenW(szVolumeLabel);
    if (szDiskPrompt) size += lstrlenW(szDiskPrompt);

    size *= sizeof(WCHAR);
    buffer = malloc(size);
    *buffer = '\0';

    if (szVolumeLabel) lstrcpyW(buffer, szVolumeLabel);
    lstrcatW(buffer, L";");
    if (szDiskPrompt) lstrcatW(buffer, szDiskPrompt);

    RegSetValueExW(mediakey, szIndex, 0, REG_SZ, (LPBYTE)buffer, size);
    free(buffer);

    RegCloseKey(sourcekey);
    RegCloseKey(mediakey);

    return ERROR_SUCCESS;
}

/******************************************************************
 *  MsiSourceListClearAllA (MSI.@)
 */
UINT WINAPI MsiSourceListClearAllA( const char *szProduct, const char *szUserName, DWORD dwReserved )
{
    FIXME( "%s, %s, %#lx\n", debugstr_a(szProduct), debugstr_a(szUserName), dwReserved );
    return ERROR_SUCCESS;
}

/******************************************************************
 *  MsiSourceListClearAllW (MSI.@)
 */
UINT WINAPI MsiSourceListClearAllW( const WCHAR *szProduct, const WCHAR *szUserName, DWORD dwReserved )
{
    FIXME( "%s, %s, %#lx\n", debugstr_w(szProduct), debugstr_w(szUserName), dwReserved );
    return ERROR_SUCCESS;
}

/******************************************************************
 *  MsiSourceListClearAllExA (MSI.@)
 */
UINT WINAPI MsiSourceListClearAllExA( const char *szProduct, const char *szUserSid, MSIINSTALLCONTEXT dwContext,
                                      DWORD dwOptions )
{
    FIXME( "%s, %s, %d, %#lx\n", debugstr_a(szProduct), debugstr_a(szUserSid), dwContext, dwOptions );
    return ERROR_SUCCESS;
}

/******************************************************************
 *  MsiSourceListClearAllExW (MSI.@)
 */
UINT WINAPI MsiSourceListClearAllExW( const WCHAR *szProduct, const WCHAR *szUserSid, MSIINSTALLCONTEXT dwContext,
                                      DWORD dwOptions )
{
    FIXME( "%s, %s, %d, %#lx\n", debugstr_w(szProduct), debugstr_w(szUserSid), dwContext, dwOptions );
    return ERROR_SUCCESS;
}

/******************************************************************
 *  MsiSourceListClearSourceA (MSI.@)
 */
UINT WINAPI MsiSourceListClearSourceA( const char *szProductCodeOrPatchCode, const char *szUserSid,
                                       MSIINSTALLCONTEXT dwContext, DWORD dwOptions, const char *szSource )
{
    FIXME( "%s, %s, %d, %#lx, %s\n", debugstr_a(szProductCodeOrPatchCode), debugstr_a(szUserSid), dwContext,
           dwOptions, debugstr_a(szSource) );
    return ERROR_SUCCESS;
}

/******************************************************************
 *  MsiSourceListClearSourceW (MSI.@)
 */
UINT WINAPI MsiSourceListClearSourceW( const WCHAR *szProductCodeOrPatchCode, const WCHAR *szUserSid,
                                       MSIINSTALLCONTEXT dwContext, DWORD dwOptions, LPCWSTR szSource )
{
    FIXME( "%s, %s, %d, %#lx, %s\n", debugstr_w(szProductCodeOrPatchCode), debugstr_w(szUserSid), dwContext,
           dwOptions, debugstr_w(szSource) );
    return ERROR_SUCCESS;
}

/******************************************************************
 *  MsiSourceListForceResolutionA (MSI.@)
 */
UINT WINAPI MsiSourceListForceResolutionA( const char *product, const char *user, DWORD reserved )
{
    FIXME( "%s, %s, %#lx\n", debugstr_a(product), debugstr_a(user), reserved );
    return ERROR_SUCCESS;
}

/******************************************************************
 *  MsiSourceListForceResolutionW (MSI.@)
 */
UINT WINAPI MsiSourceListForceResolutionW( const WCHAR *product, const WCHAR *user, DWORD reserved )
{
    FIXME( "%s, %s, %#lx\n", debugstr_w(product), debugstr_w(user), reserved );
    return ERROR_SUCCESS;
}
