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

#include "stdio.h"
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
    LPWSTR  path;
    WCHAR   szIndex[10];
    WCHAR   type;
} media_info;

static UINT OpenSourceKey(LPCWSTR szProduct, HKEY* key, BOOL user, BOOL create)
{
    HKEY rootkey = 0;
    UINT rc;
    static const WCHAR szSourceList[] = {'S','o','u','r','c','e','L','i','s','t',0};

    if (user)
        rc = MSIREG_OpenUserProductsKey(szProduct, &rootkey, create);
    else
        rc = MSIREG_OpenProductsKey(szProduct, &rootkey, create);

    if (rc)
        return rc;

    if (create)
        rc = RegCreateKeyW(rootkey, szSourceList, key);
    else
        rc = RegOpenKeyW(rootkey,szSourceList, key);

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


static UINT find_given_source(HKEY key, LPCWSTR szSource, media_info *ss)
{
    DWORD index = 0;
    WCHAR szIndex[10];
    DWORD size;
    DWORD val_size;
    LPWSTR val;
    UINT rc = ERROR_SUCCESS;

    while (rc == ERROR_SUCCESS)
    {
        val = NULL;
        val_size = 0;
        size = sizeof(szIndex)/sizeof(szIndex[0]);
        rc = RegEnumValueW(key, index, szIndex, &size, NULL, NULL, NULL, &val_size);
        if (rc != ERROR_NO_MORE_ITEMS)
        {
            val = msi_alloc(val_size);
            RegEnumValueW(key, index, szIndex, &size, NULL, NULL, (LPBYTE)val,
                &val_size);
            if (lstrcmpiW(szSource,val)==0)
            {
                ss->path = val;
                strcpyW(ss->szIndex,szIndex);
                break;
            }
            else
                strcpyW(ss->szIndex,szIndex);

            msi_free(val);
            index ++;
        }
    }
    return rc;
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

    if (!szProduct || lstrlenW(szProduct) > 39)
        return ERROR_INVALID_PARAMETER;

    if (szValue && !pcchValue)
        return ERROR_INVALID_PARAMETER;

    if (dwOptions == MSICODE_PATCH)
    {
        FIXME("Unhandled options MSICODE_PATCH\n");
        return ERROR_FUNCTION_FAILED;
    }

    if (szUserSid)
        FIXME("Unhandled UserSid %s\n",debugstr_w(szUserSid));

    if (dwContext == MSIINSTALLCONTEXT_USERUNMANAGED)
        FIXME("Unknown context MSIINSTALLCONTEXT_USERUNMANAGED\n");

    if (dwContext == MSIINSTALLCONTEXT_MACHINE)
        rc = OpenSourceKey(szProduct, &sourcekey, FALSE, FALSE);
    else
        rc = OpenSourceKey(szProduct, &sourcekey, TRUE, FALSE);

    if (rc != ERROR_SUCCESS)
        return ERROR_UNKNOWN_PRODUCT;

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
        rc = RegQueryValueExW(sourcekey, INSTALLPROPERTY_PACKAGENAMEW, 0, 0,
                (LPBYTE)szValue, pcchValue);
        if (rc != ERROR_SUCCESS && rc != ERROR_MORE_DATA)
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

    if (dwContext == MSIINSTALLCONTEXT_MACHINE)
        rc = OpenSourceKey(szProduct, &sourcekey, FALSE, TRUE);
    else
        rc = OpenSourceKey(szProduct, &sourcekey, TRUE, TRUE);

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
 *  MsiSourceListAddSourceExW (MSI.@)
 */
UINT WINAPI MsiSourceListAddSourceExW( LPCWSTR szProduct, LPCWSTR szUserSid,
        MSIINSTALLCONTEXT dwContext, DWORD dwOptions, LPCWSTR szSource,
        DWORD dwIndex)
{
    HKEY sourcekey;
    HKEY typekey;
    UINT rc;
    media_info source_struct;

    TRACE("%s %s %x %x %s %i\n", debugstr_w(szProduct), debugstr_w(szUserSid),
          dwContext, dwOptions, debugstr_w(szSource), dwIndex);

    if (!szProduct)
        return ERROR_INVALID_PARAMETER;

    if (!szSource)
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

    if (dwContext == MSIINSTALLCONTEXT_MACHINE)
        rc = OpenSourceKey(szProduct, &sourcekey, FALSE, TRUE);
    else
        rc = OpenSourceKey(szProduct, &sourcekey, TRUE, TRUE);

    if (rc != ERROR_SUCCESS)
        return ERROR_UNKNOWN_PRODUCT;

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

    source_struct.szIndex[0] = 0;
    if (find_given_source(typekey, szSource, &source_struct)==ERROR_SUCCESS)
    {
        DWORD current_index = atoiW(source_struct.szIndex);
        /* found the source */
        if (dwIndex > 0 && current_index != dwIndex)
            FIXME("Need to reorder the sources!\n");
        msi_free( source_struct.path );
    }
    else
    {
        DWORD current_index = 0;
        static const WCHAR fmt[] = {'%','i',0};
        DWORD size = lstrlenW(szSource)*sizeof(WCHAR);

        if (source_struct.szIndex[0])
            current_index = atoiW(source_struct.szIndex);
        /* new source */
        if (dwIndex > 0 && dwIndex < current_index)
            FIXME("Need to reorder the sources!\n");

        current_index ++;
        sprintfW(source_struct.szIndex,fmt,current_index);
        rc = RegSetValueExW(typekey, source_struct.szIndex, 0, REG_EXPAND_SZ,
                (const BYTE *)szSource, size);
    }

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

    if (dwContext == MSIINSTALLCONTEXT_MACHINE)
        rc = OpenSourceKey(szProduct, &sourcekey, FALSE, TRUE);
    else
        rc = OpenSourceKey(szProduct, &sourcekey, TRUE, TRUE);

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
 *  MsiSourceListAddSourceExA (MSI.@)
 */
UINT WINAPI MsiSourceListClearAllA( LPCSTR szProduct, LPCSTR szUserName, DWORD dwReserved )
{
    FIXME("(%s %s %d)\n", debugstr_a(szProduct), debugstr_a(szUserName), dwReserved);
    return ERROR_SUCCESS;
}

/******************************************************************
 *  MsiSourceListAddSourceExW (MSI.@)
 */
UINT WINAPI MsiSourceListClearAllW( LPCWSTR szProduct, LPCWSTR szUserName, DWORD dwReserved )
{
    FIXME("(%s %s %d)\n", debugstr_w(szProduct), debugstr_w(szUserName), dwReserved);
    return ERROR_SUCCESS;
}
