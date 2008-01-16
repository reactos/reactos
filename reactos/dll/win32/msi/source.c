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
#define NONAMELESSUNION

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
#include "wine/unicode.h"
#include "sddl.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

/*
 * These apis are defined in MSI 3.0
 */

typedef struct tagMediaInfo
{
    struct list entry;
    LPWSTR  path;
    WCHAR   szIndex[10];
    DWORD   index;
} media_info;

static UINT OpenSourceKey(LPCWSTR szProduct, HKEY* key, DWORD dwOptions,
                          MSIINSTALLCONTEXT context, BOOL create)
{
    HKEY rootkey = 0; 
    UINT rc = ERROR_FUNCTION_FAILED;
    static const WCHAR szSourceList[] = {'S','o','u','r','c','e','L','i','s','t',0};

    if (context == MSIINSTALLCONTEXT_USERUNMANAGED)
    {
        if (dwOptions == MSICODE_PATCH)
            rc = MSIREG_OpenUserPatchesKey(szProduct, &rootkey, create);
        else
            rc = MSIREG_OpenUserProductsKey(szProduct, &rootkey, create);
    }
    else if (context == MSIINSTALLCONTEXT_USERMANAGED)
    {
        if (dwOptions == MSICODE_PATCH)
            rc = MSIREG_OpenUserPatchesKey(szProduct, &rootkey, create);
        else
            rc = MSIREG_OpenLocalManagedProductKey(szProduct, &rootkey, create);
    }
    else if (context == MSIINSTALLCONTEXT_MACHINE)
    {
        if (dwOptions == MSICODE_PATCH)
            rc = MSIREG_OpenPatchesKey(szProduct, &rootkey, create);
        else
            rc = MSIREG_OpenLocalClassesProductKey(szProduct, &rootkey, create);
    }

    if (rc)
    {
        if (dwOptions == MSICODE_PATCH)
            return ERROR_UNKNOWN_PATCH;
        else
            return ERROR_UNKNOWN_PRODUCT;
    }

    if (create)
        rc = RegCreateKeyW(rootkey, szSourceList, key);
    else
    {
        rc = RegOpenKeyW(rootkey,szSourceList, key);
        if (rc != ERROR_SUCCESS)
            rc = ERROR_BAD_CONFIGURATION;
    }

    return rc;
}

static UINT OpenMediaSubkey(HKEY rootkey, HKEY *key, BOOL create)
{
    UINT rc;
    static const WCHAR media[] = {'M','e','d','i','a',0};

    if (create)
        rc = RegCreateKeyW(rootkey, media, key);
    else
        rc = RegOpenKeyW(rootkey,media, key); 

    return rc;
}

static UINT OpenNetworkSubkey(HKEY rootkey, HKEY *key, BOOL create)
{
    UINT rc;
    static const WCHAR net[] = {'N','e','t',0};

    if (create)
        rc = RegCreateKeyW(rootkey, net, key);
    else
        rc = RegOpenKeyW(rootkey, net, key); 

    return rc;
}

static UINT OpenURLSubkey(HKEY rootkey, HKEY *key, BOOL create)
{
    UINT rc;
    static const WCHAR URL[] = {'U','R','L',0};

    if (create)
        rc = RegCreateKeyW(rootkey, URL, key);
    else
        rc = RegOpenKeyW(rootkey, URL, key); 

    return rc;
}

/******************************************************************
 *  MsiSourceListEnumSourcesA   (MSI.@)
 */
UINT WINAPI MsiSourceListEnumSourcesA(LPCSTR szProductCodeOrPatch, LPCSTR szUserSid,
                                      MSIINSTALLCONTEXT dwContext,
                                      DWORD dwOptions, DWORD dwIndex,
                                      LPSTR szSource, LPDWORD pcchSource)
{
    FIXME("(%s, %s, %d, %d, %d, %p, %p): stub!\n", szProductCodeOrPatch, szUserSid,
          dwContext, dwOptions, dwIndex, szSource, pcchSource);
    return ERROR_CALL_NOT_IMPLEMENTED;
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

    value = msi_alloc(++len * sizeof(WCHAR));
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
    msi_free(product);
    msi_free(usersid);
    msi_free(property);
    msi_free(value);
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
    HKEY sourcekey;
    UINT rc;

    TRACE("%s %s\n", debugstr_w(szProduct), debugstr_w(szProperty));

    if (!szProduct || !*szProduct)
        return ERROR_INVALID_PARAMETER;

    if (lstrlenW(szProduct) != GUID_SIZE - 1 ||
        (szProduct[0] != '{' && szProduct[GUID_SIZE - 2] != '}'))
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

    if (dwContext != MSIINSTALLCONTEXT_USERUNMANAGED)
        FIXME("Unhandled context %d\n", dwContext);

    rc = OpenSourceKey(szProduct, &sourcekey, dwOptions, dwContext, FALSE);
    if (rc != ERROR_SUCCESS)
        return rc;

    if (strcmpW(szProperty, INSTALLPROPERTY_MEDIAPACKAGEPATHW) == 0)
    {
        HKEY key;
        rc = OpenMediaSubkey(sourcekey, &key, FALSE);
        if (rc == ERROR_SUCCESS)
            rc = RegQueryValueExW(key, INSTALLPROPERTY_MEDIAPACKAGEPATHW,
                    0, 0, (LPBYTE)szValue, pcchValue);
        if (rc != ERROR_SUCCESS && rc != ERROR_MORE_DATA)
            rc = ERROR_UNKNOWN_PROPERTY;
        RegCloseKey(key);
    }
    else if (strcmpW(szProperty, INSTALLPROPERTY_DISKPROMPTW) ==0)
    {
        HKEY key;
        rc = OpenMediaSubkey(sourcekey, &key, FALSE);
        if (rc == ERROR_SUCCESS)
            rc = RegQueryValueExW(key, INSTALLPROPERTY_DISKPROMPTW, 0, 0,
                    (LPBYTE)szValue, pcchValue);
        if (rc != ERROR_SUCCESS && rc != ERROR_MORE_DATA)
            rc = ERROR_UNKNOWN_PROPERTY;
        RegCloseKey(key);
    }
    else if (strcmpW(szProperty, INSTALLPROPERTY_LASTUSEDSOURCEW)==0)
    {
        LPWSTR buffer;
        DWORD size = 0;

        RegQueryValueExW(sourcekey, INSTALLPROPERTY_LASTUSEDSOURCEW, 0, 0,
                NULL, &size);
        if (size == 0)
            rc = ERROR_UNKNOWN_PROPERTY;
        else
        {
            LPWSTR ptr;
            buffer = msi_alloc(size);
            rc = RegQueryValueExW(sourcekey, INSTALLPROPERTY_LASTUSEDSOURCEW,
                    0, 0, (LPBYTE)buffer,&size); 
            ptr = strchrW(buffer,';');
            if (ptr) ptr = strchrW(ptr+1,';');
            if (!ptr)
                rc = ERROR_UNKNOWN_PROPERTY;
            else
            {
                ptr ++;
                lstrcpynW(szValue, ptr, *pcchValue);
                if (lstrlenW(ptr) > *pcchValue)
                {
                    *pcchValue = lstrlenW(ptr)+1;
                    rc = ERROR_MORE_DATA;
                }
                else
                    rc = ERROR_SUCCESS;
            }
            msi_free(buffer);
        }
    }
    else if (strcmpW(INSTALLPROPERTY_LASTUSEDTYPEW, szProperty)==0)
    {
        LPWSTR buffer;
        DWORD size = 0;

        RegQueryValueExW(sourcekey, INSTALLPROPERTY_LASTUSEDSOURCEW, 0, 0,
                NULL, &size);
        if (size == 0)
            rc = ERROR_UNKNOWN_PROPERTY;
        else
        {
            buffer = msi_alloc(size);
            rc = RegQueryValueExW(sourcekey, INSTALLPROPERTY_LASTUSEDSOURCEW,
                    0, 0, (LPBYTE)buffer,&size); 
            if (*pcchValue < 1)
            {
                rc = ERROR_MORE_DATA;
                *pcchValue = 1;
            }
            else
            {
                szValue[0] = buffer[0];
                rc = ERROR_SUCCESS;
            }
            msi_free(buffer);
        }
    }
    else if (strcmpW(INSTALLPROPERTY_PACKAGENAMEW, szProperty)==0)
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
 *  MsiSourceListSetInfoW   (MSI.@)
 */
UINT WINAPI MsiSourceListSetInfoW( LPCWSTR szProduct, LPCWSTR szUserSid,
                                   MSIINSTALLCONTEXT dwContext, DWORD dwOptions,
                                   LPCWSTR szProperty, LPCWSTR szValue)
{
    HKEY sourcekey;
    UINT rc;

    TRACE("%s %s %x %x %s %s\n", debugstr_w(szProduct), debugstr_w(szUserSid),
            dwContext, dwOptions, debugstr_w(szProperty), debugstr_w(szValue));

    if (!szProduct || lstrlenW(szProduct) > 39)
        return ERROR_INVALID_PARAMETER;

    if (dwOptions & MSICODE_PATCH)
    {
        FIXME("Unhandled options MSICODE_PATCH\n");
        return ERROR_FUNCTION_FAILED;
    }
    
    if (szUserSid)
        FIXME("Unhandled UserSid %s\n",debugstr_w(szUserSid));

    if (dwContext == MSIINSTALLCONTEXT_USERUNMANAGED)
        FIXME("Unknown context MSIINSTALLCONTEXT_USERUNMANAGED\n");

    rc = OpenSourceKey(szProduct, &sourcekey, MSICODE_PRODUCT, dwContext, TRUE);
    if (rc != ERROR_SUCCESS)
        return ERROR_UNKNOWN_PRODUCT;


    if (strcmpW(szProperty, INSTALLPROPERTY_MEDIAPACKAGEPATHW) == 0)
    {
        HKEY key;
        DWORD size = lstrlenW(szValue)*sizeof(WCHAR);
        rc = OpenMediaSubkey(sourcekey, &key, FALSE);
        if (rc == ERROR_SUCCESS)
            rc = RegSetValueExW(key, INSTALLPROPERTY_MEDIAPACKAGEPATHW, 0,
                    REG_SZ, (const BYTE *)szValue, size);
        if (rc != ERROR_SUCCESS)
            rc = ERROR_UNKNOWN_PROPERTY;
        RegCloseKey(key);
    }
    else if (strcmpW(szProperty, INSTALLPROPERTY_DISKPROMPTW) == 0)
    {
        HKEY key;
        DWORD size = lstrlenW(szValue)*sizeof(WCHAR);
        rc = OpenMediaSubkey(sourcekey, &key, FALSE);
        if (rc == ERROR_SUCCESS)
            rc = RegSetValueExW(key, INSTALLPROPERTY_DISKPROMPTW, 0,
                    REG_SZ, (const BYTE *)szValue, size);
        if (rc != ERROR_SUCCESS)
            rc = ERROR_UNKNOWN_PROPERTY;
        RegCloseKey(key);
    }
    else if (strcmpW(szProperty, INSTALLPROPERTY_LASTUSEDSOURCEW)==0)
    {
        LPWSTR buffer = NULL;
        DWORD size;
        WCHAR typechar = 'n';
        static const WCHAR LastUsedSource_Fmt[] = {'%','c',';','%','i',';','%','s',0};

        /* make sure the source is registered */
        MsiSourceListAddSourceExW(szProduct, szUserSid, dwContext, 
                dwOptions, szValue, 0); 

        if (dwOptions & MSISOURCETYPE_NETWORK)
            typechar = 'n';
        else if (dwOptions & MSISOURCETYPE_URL)
            typechar = 'u';
        else if (dwOptions & MSISOURCETYPE_MEDIA)
            typechar = 'm';
        else
            ERR("Unknown source type! %x\n", dwOptions);

        size = (lstrlenW(szValue)+5)*sizeof(WCHAR);
        buffer = msi_alloc(size);
        sprintfW(buffer, LastUsedSource_Fmt, typechar, 1, szValue);
        rc = RegSetValueExW(sourcekey, INSTALLPROPERTY_LASTUSEDSOURCEW, 0, 
                REG_EXPAND_SZ, (LPBYTE)buffer, size);
        if (rc != ERROR_SUCCESS)
            rc = ERROR_UNKNOWN_PROPERTY;
        msi_free( buffer );
    }
    else if (strcmpW(INSTALLPROPERTY_PACKAGENAMEW, szProperty)==0)
    {
        DWORD size = lstrlenW(szValue)*sizeof(WCHAR);
        rc = RegSetValueExW(sourcekey, INSTALLPROPERTY_PACKAGENAMEW, 0,
                REG_SZ, (const BYTE *)szValue, size);
        if (rc != ERROR_SUCCESS)
            rc = ERROR_UNKNOWN_PROPERTY;
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
 *  MsiSourceListAddSourceW (MSI.@)
 */
UINT WINAPI MsiSourceListAddSourceW( LPCWSTR szProduct, LPCWSTR szUserName,
        DWORD dwReserved, LPCWSTR szSource)
{
    INT ret;
    LPWSTR sidstr = NULL;
    DWORD sidsize = 0;
    DWORD domsize = 0;

    TRACE("%s %s %s\n", debugstr_w(szProduct), debugstr_w(szUserName), debugstr_w(szSource));

    if (LookupAccountNameW(NULL, szUserName, NULL, &sidsize, NULL, &domsize, NULL))
    {
        PSID psid = msi_alloc(sidsize);

        if (LookupAccountNameW(NULL, szUserName, psid, &sidsize, NULL, &domsize, NULL))
            ConvertSidToStringSidW(psid, &sidstr);

        msi_free(psid);
    }

    ret = MsiSourceListAddSourceExW(szProduct, sidstr, 
        MSIINSTALLCONTEXT_USERMANAGED, MSISOURCETYPE_NETWORK, szSource, 0);

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

    ret = MsiSourceListAddSourceW(szwproduct, szwusername, 0, szwsource);

    msi_free(szwproduct);
    msi_free(szwusername);
    msi_free(szwsource);

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

    msi_free(product);
    msi_free(usersid);
    msi_free(source);

    return ret;
}

static void free_source_list(struct list *sourcelist)
{
    while (!list_empty(sourcelist))
    {
        media_info *info = LIST_ENTRY(list_head(sourcelist), media_info, entry);
        list_remove(&info->entry);
        msi_free(info->path);
        msi_free(info);
    }
}

static void add_source_to_list(struct list *sourcelist, media_info *info)
{
    media_info *iter;
    BOOL found = FALSE;
    static const WCHAR fmt[] = {'%','i',0};

    if (list_empty(sourcelist))
    {
        list_add_head(sourcelist, &info->entry);
        return;
    }

    LIST_FOR_EACH_ENTRY(iter, sourcelist, media_info, entry)
    {
        if (!found && info->index < iter->index)
        {
            found = TRUE;
            list_add_before(&iter->entry, &info->entry);
        }

        /* update the rest of the list */
        if (found)
            sprintfW(iter->szIndex, fmt, ++iter->index);
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
    media_info *entry;

    *count = 0;

    while (r == ERROR_SUCCESS)
    {
        size = sizeof(name) / sizeof(name[0]);
        r = RegEnumValueW(sourcekey, index, name, &size, NULL, NULL, NULL, &val_size);
        if (r != ERROR_SUCCESS)
            return r;

        entry = msi_alloc(sizeof(media_info));
        if (!entry)
            goto error;

        entry->path = msi_alloc(val_size);
        if (!entry->path)
        {
            msi_free(entry);
            goto error;
        }

        lstrcpyW(entry->szIndex, name);
        entry->index = atoiW(name);

        size++;
        r = RegEnumValueW(sourcekey, index, name, &size, NULL,
                          NULL, (LPBYTE)entry->path, &val_size);
        if (r != ERROR_SUCCESS)
        {
            msi_free(entry->path);
            msi_free(entry);
            goto error;
        }

        index = ++(*count);
        add_source_to_list(sourcelist, entry);
    }

error:
    *count = -1;
    free_source_list(sourcelist);
    return ERROR_OUTOFMEMORY;
}

/******************************************************************
 *  MsiSourceListAddSourceExW (MSI.@)
 */
UINT WINAPI MsiSourceListAddSourceExW( LPCWSTR szProduct, LPCWSTR szUserSid,
        MSIINSTALLCONTEXT dwContext, DWORD dwOptions, LPCWSTR szSource, 
        DWORD dwIndex)
{
    HKEY sourcekey;
    HKEY typekey;
    UINT rc;
    struct list sourcelist;
    media_info *info;
    WCHAR squished_pc[GUID_SIZE];
    WCHAR name[10];
    LPWSTR source;
    LPCWSTR postfix;
    DWORD size, count;

    static const WCHAR fmt[] = {'%','i',0};
    static const WCHAR one[] = {'1',0};
    static const WCHAR backslash[] = {'\\',0};
    static const WCHAR forwardslash[] = {'/',0};

    TRACE("%s %s %x %x %s %i\n", debugstr_w(szProduct), debugstr_w(szUserSid),
          dwContext, dwOptions, debugstr_w(szSource), dwIndex);

    if (!szProduct || !squash_guid(szProduct, squished_pc))
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
        ERR("unknown media type: %08x\n", dwOptions);
        RegCloseKey(sourcekey);
        return ERROR_FUNCTION_FAILED;
    }

    postfix = (dwOptions & MSISOURCETYPE_NETWORK) ? backslash : forwardslash;
    if (szSource[lstrlenW(szSource) - 1] == *postfix)
        source = strdupW(szSource);
    else
    {
        size = lstrlenW(szSource) + 2;
        source = msi_alloc(size * sizeof(WCHAR));
        lstrcpyW(source, szSource);
        lstrcatW(source, postfix);
    }

    list_init(&sourcelist);
    rc = fill_source_list(&sourcelist, typekey, &count);
    if (rc != ERROR_NO_MORE_ITEMS)
        return rc;

    size = (lstrlenW(source) + 1) * sizeof(WCHAR);

    if (count == 0)
    {
        rc = RegSetValueExW(typekey, one, 0, REG_EXPAND_SZ, (LPBYTE)source, size);
        goto done;
    }
    else if (dwIndex > count)
    {
        sprintfW(name, fmt, count + 1);
        rc = RegSetValueExW(typekey, name, 0, REG_EXPAND_SZ, (LPBYTE)source, size);
        goto done;
    }
    else
    {
        /* add to the end of the list */
        if (dwIndex == 0)
            dwIndex = count + 1;

        sprintfW(name, fmt, dwIndex);
        info = msi_alloc(sizeof(media_info));
        if (!info)
        {
            rc = ERROR_OUTOFMEMORY;
            goto done;
        }

        info->path = strdupW(source);
        lstrcpyW(info->szIndex, name);
        info->index = dwIndex;
        add_source_to_list(&sourcelist, info);

        LIST_FOR_EACH_ENTRY(info, &sourcelist, media_info, entry)
        {
            size = (lstrlenW(info->path) + 1) * sizeof(WCHAR);
            rc = RegSetValueExW(typekey, info->szIndex, 0,
                                REG_EXPAND_SZ, (LPBYTE)info->path, size);
            if (rc != ERROR_SUCCESS)
                goto done;
        }
    }

done:
    free_source_list(&sourcelist);
    msi_free(source);
    RegCloseKey(typekey);
    RegCloseKey(sourcekey);
    return rc;
}

/******************************************************************
 *  MsiSourceListAddMediaDisk(MSI.@)
 */
UINT WINAPI MsiSourceListAddMediaDiskW(LPCWSTR szProduct, LPCWSTR szUserSid, 
        MSIINSTALLCONTEXT dwContext, DWORD dwOptions, DWORD dwDiskId, 
        LPCWSTR szVolumeLabel, LPCWSTR szDiskPrompt)
{
    HKEY sourcekey;
    HKEY mediakey;
    UINT rc;
    WCHAR szIndex[10];
    static const WCHAR fmt[] = {'%','i',0};
    static const WCHAR disk_fmt[] = {'%','s',';','%','s',0};
    static const WCHAR empty[1] = {0};
    LPCWSTR pt1,pt2;
    LPWSTR buffer;
    DWORD size;

    TRACE("%s %s %x %x %i %s %s\n", debugstr_w(szProduct),
            debugstr_w(szUserSid), dwContext, dwOptions, dwDiskId,
            debugstr_w(szVolumeLabel), debugstr_w(szDiskPrompt));

    if (!szProduct || lstrlenW(szProduct) > 39)
        return ERROR_INVALID_PARAMETER;

    if (dwOptions & MSICODE_PATCH)
    {
        FIXME("Unhandled options MSICODE_PATCH\n");
        return ERROR_FUNCTION_FAILED;
    }
    
    if (szUserSid)
        FIXME("Unhandled UserSid %s\n",debugstr_w(szUserSid));

    if (dwContext == MSIINSTALLCONTEXT_USERUNMANAGED)
        FIXME("Unknown context MSIINSTALLCONTEXT_USERUNMANAGED\n");

    rc = OpenSourceKey(szProduct, &sourcekey, MSICODE_PRODUCT, dwContext, TRUE);
    if (rc != ERROR_SUCCESS)
        return ERROR_UNKNOWN_PRODUCT;

    OpenMediaSubkey(sourcekey,&mediakey,TRUE);

    sprintfW(szIndex,fmt,dwDiskId);

    size = 2;
    if (szVolumeLabel)
    {
        size +=lstrlenW(szVolumeLabel);
        pt1 = szVolumeLabel;
    }
    else
        pt1 = empty;
    if (szDiskPrompt)
    {
        size +=lstrlenW(szDiskPrompt);
        pt2 = szDiskPrompt;
    }
    else
        pt2 = empty;

    size *=sizeof(WCHAR);

    buffer = msi_alloc(size);
    sprintfW(buffer,disk_fmt,pt1,pt2);

    RegSetValueExW(mediakey, szIndex, 0, REG_SZ, (LPBYTE)buffer, size);
    msi_free( buffer );

    RegCloseKey(sourcekey);
    RegCloseKey(mediakey);

    return ERROR_SUCCESS;
}

/******************************************************************
 *  MsiSourceListClearAllA (MSI.@)
 */
UINT WINAPI MsiSourceListClearAllA( LPCSTR szProduct, LPCSTR szUserName, DWORD dwReserved )
{
    FIXME("(%s %s %d)\n", debugstr_a(szProduct), debugstr_a(szUserName), dwReserved);
    return ERROR_SUCCESS;
}

/******************************************************************
 *  MsiSourceListClearAllW (MSI.@)
 */
UINT WINAPI MsiSourceListClearAllW( LPCWSTR szProduct, LPCWSTR szUserName, DWORD dwReserved )
{
    FIXME("(%s %s %d)\n", debugstr_w(szProduct), debugstr_w(szUserName), dwReserved);
    return ERROR_SUCCESS;
}
