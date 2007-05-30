/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2002,2003,2004,2005 Mike McCormack for CodeWeavers
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
#include "shlobj.h"
#include "shobjidl.h"
#include "objidl.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

static const WCHAR installerW[] = {'\\','I','n','s','t','a','l','l','e','r',0};

UINT WINAPI MsiOpenProductA(LPCSTR szProduct, MSIHANDLE *phProduct)
{
    UINT r;
    LPWSTR szwProd = NULL;

    TRACE("%s %p\n",debugstr_a(szProduct), phProduct);

    if( szProduct )
    {
        szwProd = strdupAtoW( szProduct );
        if( !szwProd )
            return ERROR_OUTOFMEMORY;
    }

    r = MsiOpenProductW( szwProd, phProduct );

    msi_free( szwProd );

    return r;
}

static UINT MSI_OpenProductW( LPCWSTR szProduct, MSIPACKAGE **ppackage )
{
    LPWSTR path = NULL;
    UINT r;
    HKEY hKeyProduct = NULL;
    DWORD count, type;

    TRACE("%s %p\n", debugstr_w(szProduct), ppackage );

    r = MSIREG_OpenUninstallKey(szProduct,&hKeyProduct,FALSE);
    if( r != ERROR_SUCCESS )
    {
        r = ERROR_UNKNOWN_PRODUCT;
        goto end;
    }

    /* find the size of the path */
    type = count = 0;
    r = RegQueryValueExW( hKeyProduct, INSTALLPROPERTY_LOCALPACKAGEW,
                          NULL, &type, NULL, &count );
    if( r != ERROR_SUCCESS )
    {
        r = ERROR_UNKNOWN_PRODUCT;
        goto end;
    }

    /* now alloc and fetch the path of the database to open */
    path = msi_alloc( count );
    if( !path )
        goto end;

    r = RegQueryValueExW( hKeyProduct, INSTALLPROPERTY_LOCALPACKAGEW,
                          NULL, &type, (LPBYTE) path, &count );
    if( r != ERROR_SUCCESS )
    {
        r = ERROR_UNKNOWN_PRODUCT;
        goto end;
    }

    r = MSI_OpenPackageW( path, ppackage );

end:
    msi_free( path );
    if( hKeyProduct )
        RegCloseKey( hKeyProduct );

    return r;
}

UINT WINAPI MsiOpenProductW( LPCWSTR szProduct, MSIHANDLE *phProduct )
{
   MSIPACKAGE *package = NULL;
   UINT r;

   r = MSI_OpenProductW( szProduct, &package );
   if( r == ERROR_SUCCESS )
   {
       *phProduct = alloc_msihandle( &package->hdr );
       if (! *phProduct)
           r = ERROR_NOT_ENOUGH_MEMORY;
       msiobj_release( &package->hdr );
   }
   return r;
}

UINT WINAPI MsiAdvertiseProductA(LPCSTR szPackagePath, LPCSTR szScriptfilePath,
                LPCSTR szTransforms, LANGID lgidLanguage)
{
    FIXME("%s %s %s %08x\n",debugstr_a(szPackagePath),
          debugstr_a(szScriptfilePath), debugstr_a(szTransforms), lgidLanguage);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiAdvertiseProductW(LPCWSTR szPackagePath, LPCWSTR szScriptfilePath,
                LPCWSTR szTransforms, LANGID lgidLanguage)
{
    FIXME("%s %s %s %08x\n",debugstr_w(szPackagePath),
          debugstr_w(szScriptfilePath), debugstr_w(szTransforms), lgidLanguage);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiAdvertiseProductExA(LPCSTR szPackagePath, LPCSTR szScriptfilePath,
      LPCSTR szTransforms, LANGID lgidLanguage, DWORD dwPlatform, DWORD dwOptions)
{
    FIXME("%s %s %s %08x %08x %08x\n", debugstr_a(szPackagePath),
          debugstr_a(szScriptfilePath), debugstr_a(szTransforms),
          lgidLanguage, dwPlatform, dwOptions);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiAdvertiseProductExW( LPCWSTR szPackagePath, LPCWSTR szScriptfilePath,
      LPCWSTR szTransforms, LANGID lgidLanguage, DWORD dwPlatform, DWORD dwOptions)
{
    FIXME("%s %s %s %08x %08x %08x\n", debugstr_w(szPackagePath),
          debugstr_w(szScriptfilePath), debugstr_w(szTransforms),
          lgidLanguage, dwPlatform, dwOptions);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiInstallProductA(LPCSTR szPackagePath, LPCSTR szCommandLine)
{
    LPWSTR szwPath = NULL, szwCommand = NULL;
    UINT r = ERROR_OUTOFMEMORY;

    TRACE("%s %s\n",debugstr_a(szPackagePath), debugstr_a(szCommandLine));

    if( szPackagePath )
    {
        szwPath = strdupAtoW( szPackagePath );
        if( !szwPath )
            goto end;
    }

    if( szCommandLine )
    {
        szwCommand = strdupAtoW( szCommandLine );
        if( !szwCommand )
            goto end;
    }

    r = MsiInstallProductW( szwPath, szwCommand );

end:
    msi_free( szwPath );
    msi_free( szwCommand );

    return r;
}

UINT WINAPI MsiInstallProductW(LPCWSTR szPackagePath, LPCWSTR szCommandLine)
{
    MSIPACKAGE *package = NULL;
    UINT r;

    TRACE("%s %s\n",debugstr_w(szPackagePath), debugstr_w(szCommandLine));

    r = MSI_OpenPackageW( szPackagePath, &package );
    if (r == ERROR_SUCCESS)
    {
        r = MSI_InstallPackage( package, szPackagePath, szCommandLine );
        msiobj_release( &package->hdr );
    }

    return r;
}

UINT WINAPI MsiReinstallProductA(LPCSTR szProduct, DWORD dwReinstallMode)
{
    FIXME("%s %08x\n", debugstr_a(szProduct), dwReinstallMode);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiReinstallProductW(LPCWSTR szProduct, DWORD dwReinstallMode)
{
    FIXME("%s %08x\n", debugstr_w(szProduct), dwReinstallMode);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiApplyPatchA(LPCSTR szPatchPackage, LPCSTR szInstallPackage,
        INSTALLTYPE eInstallType, LPCSTR szCommandLine)
{
    FIXME("%s %s %d %s\n", debugstr_a(szPatchPackage), debugstr_a(szInstallPackage),
          eInstallType, debugstr_a(szCommandLine));
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiApplyPatchW(LPCWSTR szPatchPackage, LPCWSTR szInstallPackage,
         INSTALLTYPE eInstallType, LPCWSTR szCommandLine)
{
    FIXME("%s %s %d %s\n", debugstr_w(szPatchPackage), debugstr_w(szInstallPackage),
          eInstallType, debugstr_w(szCommandLine));
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiConfigureProductExW(LPCWSTR szProduct, int iInstallLevel,
                        INSTALLSTATE eInstallState, LPCWSTR szCommandLine)
{
    MSIPACKAGE* package = NULL;
    UINT r;
    DWORD sz;
    WCHAR sourcepath[MAX_PATH];
    WCHAR filename[MAX_PATH];
    static const WCHAR szInstalled[] = {
        ' ','I','n','s','t','a','l','l','e','d','=','1',0};
    LPWSTR commandline;

    TRACE("%s %d %d %s\n",debugstr_w(szProduct), iInstallLevel, eInstallState,
          debugstr_w(szCommandLine));

    if (eInstallState != INSTALLSTATE_LOCAL &&
        eInstallState != INSTALLSTATE_DEFAULT)
    {
        FIXME("Not implemented for anything other than local installs\n");
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    sz = sizeof(sourcepath);
    MsiSourceListGetInfoW(szProduct, NULL, MSIINSTALLCONTEXT_USERMANAGED, 
            MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDSOURCEW, sourcepath,
            &sz);

    sz = sizeof(filename);
    MsiSourceListGetInfoW(szProduct, NULL, MSIINSTALLCONTEXT_USERMANAGED, 
            MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAMEW, filename, &sz);

    lstrcatW(sourcepath,filename);

    /*
     * ok 1, we need to find the msi file for this product.
     *    2, find the source dir for the files
     *    3, do the configure/install.
     *    4, cleanupany runonce entry.
     */

    r = MSI_OpenProductW( szProduct, &package );
    if (r != ERROR_SUCCESS)
        return r;

    sz = lstrlenW(szInstalled) + 1;

    if (szCommandLine)
        sz += lstrlenW(szCommandLine);

    commandline = msi_alloc(sz * sizeof(WCHAR));
    if (!commandline )
    {
        r = ERROR_OUTOFMEMORY;
        goto end;
    }

    commandline[0] = 0;
    if (szCommandLine)
        lstrcpyW(commandline,szCommandLine);

    if (MsiQueryProductStateW(szProduct) != INSTALLSTATE_UNKNOWN)
        lstrcatW(commandline,szInstalled);

    r = MSI_InstallPackage( package, sourcepath, commandline );

    msi_free(commandline);

end:
    msiobj_release( &package->hdr );

    return r;
}

UINT WINAPI MsiConfigureProductExA(LPCSTR szProduct, int iInstallLevel,
                        INSTALLSTATE eInstallState, LPCSTR szCommandLine)
{
    LPWSTR szwProduct = NULL;
    LPWSTR szwCommandLine = NULL;
    UINT r = ERROR_OUTOFMEMORY;

    if( szProduct )
    {
        szwProduct = strdupAtoW( szProduct );
        if( !szwProduct )
            goto end;
    }

    if( szCommandLine)
    {
        szwCommandLine = strdupAtoW( szCommandLine );
        if( !szwCommandLine)
            goto end;
    }

    r = MsiConfigureProductExW( szwProduct, iInstallLevel, eInstallState,
                                szwCommandLine );
end:
    msi_free( szwProduct );
    msi_free( szwCommandLine);

    return r;
}

UINT WINAPI MsiConfigureProductA(LPCSTR szProduct, int iInstallLevel,
                                 INSTALLSTATE eInstallState)
{
    LPWSTR szwProduct = NULL;
    UINT r;

    TRACE("%s %d %d\n",debugstr_a(szProduct), iInstallLevel, eInstallState);

    if( szProduct )
    {
        szwProduct = strdupAtoW( szProduct );
        if( !szwProduct )
            return ERROR_OUTOFMEMORY;
    }

    r = MsiConfigureProductW( szwProduct, iInstallLevel, eInstallState );
    msi_free( szwProduct );

    return r;
}

UINT WINAPI MsiConfigureProductW(LPCWSTR szProduct, int iInstallLevel,
                                 INSTALLSTATE eInstallState)
{
    return MsiConfigureProductExW(szProduct, iInstallLevel, eInstallState, NULL);
}

UINT WINAPI MsiGetProductCodeA(LPCSTR szComponent, LPSTR szBuffer)
{
    LPWSTR szwComponent = NULL;
    UINT r;
    WCHAR szwBuffer[GUID_SIZE];

    TRACE("%s %s\n",debugstr_a(szComponent), debugstr_a(szBuffer));

    if( szComponent )
    {
        szwComponent = strdupAtoW( szComponent );
        if( !szwComponent )
            return ERROR_OUTOFMEMORY;
    }

    r = MsiGetProductCodeW( szwComponent, szwBuffer );

    if( ERROR_SUCCESS == r )
        WideCharToMultiByte(CP_ACP, 0, szwBuffer, -1, szBuffer, GUID_SIZE, NULL, NULL);

    msi_free( szwComponent );

    return r;
}

UINT WINAPI MsiGetProductCodeW(LPCWSTR szComponent, LPWSTR szBuffer)
{
    UINT rc;
    HKEY hkey;
    WCHAR szSquished[GUID_SIZE];
    DWORD sz = GUID_SIZE;
    static const WCHAR szPermKey[] =
        { '0','0','0','0','0','0','0','0','0','0','0','0',
          '0','0','0','0','0','0','0','0','0','0','0','0',
          '0','0','0','0','0','0','0','0',0};

    TRACE("%s %p\n",debugstr_w(szComponent), szBuffer);

    if (NULL == szComponent)
        return ERROR_INVALID_PARAMETER;

    rc = MSIREG_OpenComponentsKey( szComponent, &hkey, FALSE);
    if (rc != ERROR_SUCCESS)
        return ERROR_UNKNOWN_COMPONENT;

    rc = RegEnumValueW(hkey, 0, szSquished, &sz, NULL, NULL, NULL, NULL);
    if (rc == ERROR_SUCCESS && strcmpW(szSquished,szPermKey)==0)
    {
        sz = GUID_SIZE;
        rc = RegEnumValueW(hkey, 1, szSquished, &sz, NULL, NULL, NULL, NULL);
    }

    RegCloseKey(hkey);

    if (rc != ERROR_SUCCESS)
        return ERROR_INSTALL_FAILURE;

    unsquash_guid(szSquished, szBuffer);
    return ERROR_SUCCESS;
}

static UINT WINAPI MSI_GetProductInfo(LPCWSTR szProduct, LPCWSTR szAttribute,
                                      awstring *szValue, DWORD *pcchValueBuf)
{
    UINT r;
    HKEY hkey;
    LPWSTR val = NULL;

    TRACE("%s %s %p %p\n", debugstr_w(szProduct),
          debugstr_w(szAttribute), szValue, pcchValueBuf);

    /*
     * FIXME: Values seem scattered/duplicated in the registry. Is there a system?
     */

    if ((szValue->str.w && !pcchValueBuf) || !szProduct || !szAttribute)
        return ERROR_INVALID_PARAMETER;

    /* check for special properties */
    if (!lstrcmpW(szAttribute, INSTALLPROPERTY_PACKAGECODEW))
    {
        LPWSTR regval;
        WCHAR packagecode[35];

        r = MSIREG_OpenUserProductsKey(szProduct, &hkey, FALSE);
        if (r != ERROR_SUCCESS)
            return ERROR_UNKNOWN_PRODUCT;

        regval = msi_reg_get_val_str( hkey, szAttribute );
        if (regval)
        {
            if (unsquash_guid(regval, packagecode))
                val = strdupW(packagecode);
            msi_free(regval);
        }

        RegCloseKey(hkey);
    }
    else if (!lstrcmpW(szAttribute, INSTALLPROPERTY_ASSIGNMENTTYPEW))
    {
        static const WCHAR one[] = { '1',0 };
        /*
         * FIXME: should be in the Product key (user or system?)
         *        but isn't written yet...
         */
        val = strdupW( one );
    }
    else if (!lstrcmpW(szAttribute, INSTALLPROPERTY_LANGUAGEW) ||
             !lstrcmpW(szAttribute, INSTALLPROPERTY_VERSIONW))
    {
        static const WCHAR fmt[] = { '%','u',0 };
        WCHAR szVal[16];
        DWORD regval;

        r = MSIREG_OpenUninstallKey(szProduct, &hkey, FALSE);
        if (r != ERROR_SUCCESS)
            return ERROR_UNKNOWN_PRODUCT;

        if (msi_reg_get_val_dword( hkey, szAttribute, &regval))
        {
            sprintfW(szVal, fmt, regval);
            val = strdupW( szVal );
        }

        RegCloseKey(hkey);
    }
    else if (!lstrcmpW(szAttribute, INSTALLPROPERTY_PRODUCTNAMEW))
    {
        r = MSIREG_OpenUserProductsKey(szProduct, &hkey, FALSE);
        if (r != ERROR_SUCCESS)
            return ERROR_UNKNOWN_PRODUCT;

        val = msi_reg_get_val_str( hkey, szAttribute );

        RegCloseKey(hkey);
    }
    else
    {
        static const WCHAR szDisplayVersion[] = {
            'D','i','s','p','l','a','y','V','e','r','s','i','o','n',0 };

        FIXME("%s\n", debugstr_w(szAttribute));
        /* FIXME: some attribute values not tested... */

        if (!lstrcmpW( szAttribute, INSTALLPROPERTY_VERSIONSTRINGW ))
            szAttribute = szDisplayVersion;

        r = MSIREG_OpenUninstallKey( szProduct, &hkey, FALSE );
        if (r != ERROR_SUCCESS)
            return ERROR_UNKNOWN_PRODUCT;

        val = msi_reg_get_val_str( hkey, szAttribute );

        RegCloseKey(hkey);
    }

    TRACE("returning %s\n", debugstr_w(val));

    if (!val)
        return ERROR_UNKNOWN_PROPERTY;

    r = msi_strcpy_to_awstring( val, szValue, pcchValueBuf );

    msi_free(val);

    return r;
}

UINT WINAPI MsiGetProductInfoA(LPCSTR szProduct, LPCSTR szAttribute,
                               LPSTR szBuffer, DWORD *pcchValueBuf)
{
    LPWSTR szwProduct, szwAttribute = NULL;
    UINT r = ERROR_OUTOFMEMORY;
    awstring buffer;

    TRACE("%s %s %p %p\n", debugstr_a(szProduct), debugstr_a(szAttribute),
          szBuffer, pcchValueBuf);

    szwProduct = strdupAtoW( szProduct );
    if( szProduct && !szwProduct )
        goto end;

    szwAttribute = strdupAtoW( szAttribute );
    if( szAttribute && !szwAttribute )
        goto end;

    buffer.unicode = FALSE;
    buffer.str.a = szBuffer;

    r = MSI_GetProductInfo( szwProduct, szwAttribute,
                            &buffer, pcchValueBuf );

end:
    msi_free( szwProduct );
    msi_free( szwAttribute );

    return r;
}

UINT WINAPI MsiGetProductInfoW(LPCWSTR szProduct, LPCWSTR szAttribute,
                               LPWSTR szBuffer, DWORD *pcchValueBuf)
{
    awstring buffer;

    TRACE("%s %s %p %p\n", debugstr_w(szProduct), debugstr_w(szAttribute),
          szBuffer, pcchValueBuf);

    buffer.unicode = TRUE;
    buffer.str.w = szBuffer;

    return MSI_GetProductInfo( szProduct, szAttribute,
                               &buffer, pcchValueBuf );
}

UINT WINAPI MsiEnableLogA(DWORD dwLogMode, LPCSTR szLogFile, DWORD attributes)
{
    LPWSTR szwLogFile = NULL;
    UINT r;

    TRACE("%08x %s %08x\n", dwLogMode, debugstr_a(szLogFile), attributes);

    if( szLogFile )
    {
        szwLogFile = strdupAtoW( szLogFile );
        if( !szwLogFile )
            return ERROR_OUTOFMEMORY;
    }
    r = MsiEnableLogW( dwLogMode, szwLogFile, attributes );
    msi_free( szwLogFile );
    return r;
}

UINT WINAPI MsiEnableLogW(DWORD dwLogMode, LPCWSTR szLogFile, DWORD attributes)
{
    HANDLE file = INVALID_HANDLE_VALUE;

    TRACE("%08x %s %08x\n", dwLogMode, debugstr_w(szLogFile), attributes);

    if (szLogFile)
    {
        lstrcpyW(gszLogFile,szLogFile);
        if (!(attributes & INSTALLLOGATTRIBUTES_APPEND))
            DeleteFileW(szLogFile);
        file = CreateFileW(szLogFile, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL, NULL);
        if (file != INVALID_HANDLE_VALUE)
            CloseHandle(file);
        else
            ERR("Unable to enable log %s\n",debugstr_w(szLogFile));
    }
    else
        gszLogFile[0] = '\0';

    return ERROR_SUCCESS;
}

INSTALLSTATE WINAPI MsiQueryProductStateA(LPCSTR szProduct)
{
    LPWSTR szwProduct = NULL;
    INSTALLSTATE r;

    if( szProduct )
    {
         szwProduct = strdupAtoW( szProduct );
         if( !szwProduct )
             return ERROR_OUTOFMEMORY;
    }
    r = MsiQueryProductStateW( szwProduct );
    msi_free( szwProduct );
    return r;
}

INSTALLSTATE WINAPI MsiQueryProductStateW(LPCWSTR szProduct)
{
    UINT rc;
    INSTALLSTATE rrc = INSTALLSTATE_UNKNOWN;
    HKEY hkey = 0;
    static const WCHAR szWindowsInstaller[] = {
         'W','i','n','d','o','w','s','I','n','s','t','a','l','l','e','r',0 };
    DWORD sz;

    TRACE("%s\n", debugstr_w(szProduct));

    if (!szProduct)
        return INSTALLSTATE_INVALIDARG;

    rc = MSIREG_OpenUserProductsKey(szProduct,&hkey,FALSE);
    if (rc != ERROR_SUCCESS)
        goto end;

    RegCloseKey(hkey);

    rc = MSIREG_OpenUninstallKey(szProduct,&hkey,FALSE);
    if (rc != ERROR_SUCCESS)
        goto end;

    sz = sizeof(rrc);
    rc = RegQueryValueExW(hkey,szWindowsInstaller,NULL,NULL,(LPVOID)&rrc, &sz);
    if (rc != ERROR_SUCCESS)
        goto end;

    switch (rrc)
    {
    case 1:
        /* default */
        rrc = INSTALLSTATE_DEFAULT;
        break;
    default:
        FIXME("Unknown install state read from registry (%i)\n",rrc);
        rrc = INSTALLSTATE_UNKNOWN;
        break;
    }
end:
    RegCloseKey(hkey);
    return rrc;
}

INSTALLUILEVEL WINAPI MsiSetInternalUI(INSTALLUILEVEL dwUILevel, HWND *phWnd)
{
    INSTALLUILEVEL old = gUILevel;
    HWND oldwnd = gUIhwnd;

    TRACE("%08x %p\n", dwUILevel, phWnd);

    gUILevel = dwUILevel;
    if (phWnd)
    {
        gUIhwnd = *phWnd;
        *phWnd = oldwnd;
    }
    return old;
}

INSTALLUI_HANDLERA WINAPI MsiSetExternalUIA(INSTALLUI_HANDLERA puiHandler,
                                  DWORD dwMessageFilter, LPVOID pvContext)
{
    INSTALLUI_HANDLERA prev = gUIHandlerA;

    TRACE("%p %x %p\n",puiHandler, dwMessageFilter,pvContext);
    gUIHandlerA = puiHandler;
    gUIFilter = dwMessageFilter;
    gUIContext = pvContext;

    return prev;
}

INSTALLUI_HANDLERW WINAPI MsiSetExternalUIW(INSTALLUI_HANDLERW puiHandler,
                                  DWORD dwMessageFilter, LPVOID pvContext)
{
    INSTALLUI_HANDLERW prev = gUIHandlerW;

    TRACE("%p %x %p\n",puiHandler,dwMessageFilter,pvContext);
    gUIHandlerW = puiHandler;
    gUIFilter = dwMessageFilter;
    gUIContext = pvContext;

    return prev;
}

/******************************************************************
 *  MsiLoadStringW            [MSI.@]
 *
 * Loads a string from MSI's string resources.
 *
 * PARAMS
 *
 *   handle        [I]  only -1 is handled currently
 *   id            [I]  id of the string to be loaded
 *   lpBuffer      [O]  buffer for the string to be written to
 *   nBufferMax    [I]  maximum size of the buffer in characters
 *   lang          [I]  the preferred language for the string
 *
 * RETURNS
 *
 *   If successful, this function returns the language id of the string loaded
 *   If the function fails, the function returns zero.
 *
 * NOTES
 *
 *   The type of the first parameter is unknown.  LoadString's prototype
 *  suggests that it might be a module handle.  I have made it an MSI handle
 *  for starters, as -1 is an invalid MSI handle, but not an invalid module
 *  handle.  Maybe strings can be stored in an MSI database somehow.
 */
LANGID WINAPI MsiLoadStringW( MSIHANDLE handle, UINT id, LPWSTR lpBuffer,
                int nBufferMax, LANGID lang )
{
    HRSRC hres;
    HGLOBAL hResData;
    LPWSTR p;
    DWORD i, len;

    TRACE("%ld %u %p %d %d\n", handle, id, lpBuffer, nBufferMax, lang);

    if( handle != -1 )
        FIXME("don't know how to deal with handle = %08lx\n", handle);

    if( !lang )
        lang = GetUserDefaultLangID();

    hres = FindResourceExW( msi_hInstance, (LPCWSTR) RT_STRING,
                            (LPWSTR)1, lang );
    if( !hres )
        return 0;
    hResData = LoadResource( msi_hInstance, hres );
    if( !hResData )
        return 0;
    p = LockResource( hResData );
    if( !p )
        return 0;

    for (i = 0; i < (id&0xf); i++)
	p += *p + 1;
    len = *p;

    if( nBufferMax <= len )
        return 0;

    memcpy( lpBuffer, p+1, len * sizeof(WCHAR));
    lpBuffer[ len ] = 0;

    TRACE("found -> %s\n", debugstr_w(lpBuffer));

    return lang;
}

LANGID WINAPI MsiLoadStringA( MSIHANDLE handle, UINT id, LPSTR lpBuffer,
                int nBufferMax, LANGID lang )
{
    LPWSTR bufW;
    LANGID r;
    DWORD len;

    bufW = msi_alloc(nBufferMax*sizeof(WCHAR));
    r = MsiLoadStringW(handle, id, bufW, nBufferMax, lang);
    if( r )
    {
        len = WideCharToMultiByte(CP_ACP, 0, bufW, -1, NULL, 0, NULL, NULL );
        if( len <= nBufferMax )
            WideCharToMultiByte( CP_ACP, 0, bufW, -1,
                                 lpBuffer, nBufferMax, NULL, NULL );
        else
            r = 0;
    }
    msi_free(bufW);
    return r;
}

INSTALLSTATE WINAPI MsiLocateComponentA(LPCSTR szComponent, LPSTR lpPathBuf,
                DWORD *pcchBuf)
{
    char szProduct[GUID_SIZE];

    TRACE("%s %p %p\n", debugstr_a(szComponent), lpPathBuf, pcchBuf);

    if (MsiGetProductCodeA( szComponent, szProduct ) != ERROR_SUCCESS)
        return INSTALLSTATE_UNKNOWN;

    return MsiGetComponentPathA( szProduct, szComponent, lpPathBuf, pcchBuf );
}

INSTALLSTATE WINAPI MsiLocateComponentW(LPCWSTR szComponent, LPWSTR lpPathBuf,
                DWORD *pcchBuf)
{
    WCHAR szProduct[GUID_SIZE];

    TRACE("%s %p %p\n", debugstr_w(szComponent), lpPathBuf, pcchBuf);

    if (MsiGetProductCodeW( szComponent, szProduct ) != ERROR_SUCCESS)
        return INSTALLSTATE_UNKNOWN;

    return MsiGetComponentPathW( szProduct, szComponent, lpPathBuf, pcchBuf );
}

UINT WINAPI MsiMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType,
                WORD wLanguageId, DWORD f)
{
    FIXME("%p %s %s %u %08x %08x\n", hWnd, debugstr_a(lpText), debugstr_a(lpCaption),
          uType, wLanguageId, f);
    return MessageBoxExA(hWnd,lpText,lpCaption,uType,wLanguageId); 
}

UINT WINAPI MsiMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType,
                WORD wLanguageId, DWORD f)
{
    FIXME("%p %s %s %u %08x %08x\n", hWnd, debugstr_w(lpText), debugstr_w(lpCaption),
          uType, wLanguageId, f);
    return MessageBoxExW(hWnd,lpText,lpCaption,uType,wLanguageId); 
}

UINT WINAPI MsiProvideAssemblyA( LPCSTR szAssemblyName, LPCSTR szAppContext,
                DWORD dwInstallMode, DWORD dwAssemblyInfo, LPSTR lpPathBuf,
                DWORD* pcchPathBuf )
{
    FIXME("%s %s %08x %08x %p %p\n", debugstr_a(szAssemblyName),
          debugstr_a(szAppContext), dwInstallMode, dwAssemblyInfo, lpPathBuf,
          pcchPathBuf);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiProvideAssemblyW( LPCWSTR szAssemblyName, LPCWSTR szAppContext,
                DWORD dwInstallMode, DWORD dwAssemblyInfo, LPWSTR lpPathBuf,
                DWORD* pcchPathBuf )
{
    FIXME("%s %s %08x %08x %p %p\n", debugstr_w(szAssemblyName),
          debugstr_w(szAppContext), dwInstallMode, dwAssemblyInfo, lpPathBuf,
          pcchPathBuf);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiProvideComponentFromDescriptorA( LPCSTR szDescriptor,
                LPSTR szPath, DWORD *pcchPath, DWORD *pcchArgs )
{
    FIXME("%s %p %p %p\n", debugstr_a(szDescriptor), szPath, pcchPath, pcchArgs );
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiProvideComponentFromDescriptorW( LPCWSTR szDescriptor,
                LPWSTR szPath, DWORD *pcchPath, DWORD *pcchArgs )
{
    FIXME("%s %p %p %p\n", debugstr_w(szDescriptor), szPath, pcchPath, pcchArgs );
    return ERROR_CALL_NOT_IMPLEMENTED;
}

HRESULT WINAPI MsiGetFileSignatureInformationA( LPCSTR szSignedObjectPath,
                DWORD dwFlags, PCCERT_CONTEXT* ppcCertContext, BYTE* pbHashData,
                DWORD* pcbHashData)
{
    FIXME("%s %08x %p %p %p\n", debugstr_a(szSignedObjectPath), dwFlags,
          ppcCertContext, pbHashData, pcbHashData);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

HRESULT WINAPI MsiGetFileSignatureInformationW( LPCWSTR szSignedObjectPath,
                DWORD dwFlags, PCCERT_CONTEXT* ppcCertContext, BYTE* pbHashData,
                DWORD* pcbHashData)
{
    FIXME("%s %08x %p %p %p\n", debugstr_w(szSignedObjectPath), dwFlags,
          ppcCertContext, pbHashData, pcbHashData);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiGetProductPropertyA( MSIHANDLE hProduct, LPCSTR szProperty,
                                    LPSTR szValue, DWORD *pccbValue )
{
    FIXME("%ld %s %p %p\n", hProduct, debugstr_a(szProperty), szValue, pccbValue);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiGetProductPropertyW( MSIHANDLE hProduct, LPCWSTR szProperty,
                                    LPWSTR szValue, DWORD *pccbValue )
{
    FIXME("%ld %s %p %p\n", hProduct, debugstr_w(szProperty), szValue, pccbValue);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiVerifyPackageA( LPCSTR szPackage )
{
    UINT r;
    LPWSTR szPack = NULL;

    TRACE("%s\n", debugstr_a(szPackage) );

    if( szPackage )
    {
        szPack = strdupAtoW( szPackage );
        if( !szPack )
            return ERROR_OUTOFMEMORY;
    }

    r = MsiVerifyPackageW( szPack );

    msi_free( szPack );

    return r;
}

UINT WINAPI MsiVerifyPackageW( LPCWSTR szPackage )
{
    MSIHANDLE handle;
    UINT r;

    TRACE("%s\n", debugstr_w(szPackage) );

    r = MsiOpenDatabaseW( szPackage, MSIDBOPEN_READONLY, &handle );
    MsiCloseHandle( handle );

    return r;
}

static INSTALLSTATE WINAPI MSI_GetComponentPath(LPCWSTR szProduct, LPCWSTR szComponent,
                                                awstring* lpPathBuf, DWORD* pcchBuf)
{
    WCHAR squished_pc[GUID_SIZE], squished_comp[GUID_SIZE];
    UINT rc;
    HKEY hkey = 0;
    LPWSTR path = NULL;
    INSTALLSTATE r;

    TRACE("%s %s %p %p\n", debugstr_w(szProduct),
           debugstr_w(szComponent), lpPathBuf->str.w, pcchBuf);

    if( !szProduct || !szComponent )
        return INSTALLSTATE_INVALIDARG;
    if( lpPathBuf->str.w && !pcchBuf )
        return INSTALLSTATE_INVALIDARG;

    if (!squash_guid( szProduct, squished_pc ) ||
        !squash_guid( szComponent, squished_comp ))
        return INSTALLSTATE_INVALIDARG;

    rc = MSIREG_OpenProductsKey( szProduct, &hkey, FALSE);
    if( rc != ERROR_SUCCESS )
        return INSTALLSTATE_UNKNOWN;

    RegCloseKey(hkey);

    rc = MSIREG_OpenComponentsKey( szComponent, &hkey, FALSE);
    if( rc != ERROR_SUCCESS )
        return INSTALLSTATE_UNKNOWN;

    path = msi_reg_get_val_str( hkey, squished_pc );
    RegCloseKey(hkey);

    TRACE("found path of (%s:%s)(%s)\n", debugstr_w(szComponent),
           debugstr_w(szProduct), debugstr_w(path));

    if (!path)
        return INSTALLSTATE_UNKNOWN;

    if (path[0])
        r = INSTALLSTATE_LOCAL;
    else
        r = INSTALLSTATE_NOTUSED;

    msi_strcpy_to_awstring( path, lpPathBuf, pcchBuf );

    msi_free( path );
    return r;
}

/******************************************************************
 * MsiGetComponentPathW      [MSI.@]
 */
INSTALLSTATE WINAPI MsiGetComponentPathW(LPCWSTR szProduct, LPCWSTR szComponent,
                                         LPWSTR lpPathBuf, DWORD* pcchBuf)
{
    awstring path;

    path.unicode = TRUE;
    path.str.w = lpPathBuf;

    return MSI_GetComponentPath( szProduct, szComponent, &path, pcchBuf );
}

/******************************************************************
 * MsiGetComponentPathA      [MSI.@]
 */
INSTALLSTATE WINAPI MsiGetComponentPathA(LPCSTR szProduct, LPCSTR szComponent,
                                         LPSTR lpPathBuf, DWORD* pcchBuf)
{
    LPWSTR szwProduct, szwComponent = NULL;
    INSTALLSTATE r = INSTALLSTATE_UNKNOWN;
    awstring path;

    szwProduct = strdupAtoW( szProduct );
    if( szProduct && !szwProduct)
        goto end;

    szwComponent = strdupAtoW( szComponent );
    if( szComponent && !szwComponent )
        goto end;

    path.unicode = FALSE;
    path.str.a = lpPathBuf;

    r = MSI_GetComponentPath( szwProduct, szwComponent, &path, pcchBuf );

end:
    msi_free( szwProduct );
    msi_free( szwComponent );

    return r;
}

/******************************************************************
 * MsiQueryFeatureStateA      [MSI.@]
 */
INSTALLSTATE WINAPI MsiQueryFeatureStateA(LPCSTR szProduct, LPCSTR szFeature)
{
    LPWSTR szwProduct = NULL, szwFeature= NULL;
    INSTALLSTATE rc = INSTALLSTATE_UNKNOWN;

    szwProduct = strdupAtoW( szProduct );
    if ( szProduct && !szwProduct )
        goto end;

    szwFeature = strdupAtoW( szFeature );
    if ( szFeature && !szwFeature )
        goto end;

    rc = MsiQueryFeatureStateW(szwProduct, szwFeature);

end:
    msi_free( szwProduct);
    msi_free( szwFeature);

    return rc;
}

/******************************************************************
 * MsiQueryFeatureStateW      [MSI.@]
 *
 * Checks the state of a feature
 *
 * PARAMS
 *   szProduct     [I]  Product's GUID string
 *   szFeature     [I]  Feature's GUID string
 *
 * RETURNS
 *   INSTALLSTATE_LOCAL        Feature is installed and useable
 *   INSTALLSTATE_ABSENT       Feature is absent
 *   INSTALLSTATE_ADVERTISED   Feature should be installed on demand
 *   INSTALLSTATE_UNKNOWN      An error occurred
 *   INSTALLSTATE_INVALIDARG   One of the GUIDs was invalid
 *
 */
INSTALLSTATE WINAPI MsiQueryFeatureStateW(LPCWSTR szProduct, LPCWSTR szFeature)
{
    WCHAR squishProduct[33], comp[GUID_SIZE];
    GUID guid;
    LPWSTR components, p, parent_feature;
    UINT rc;
    HKEY hkey;
    INSTALLSTATE r;
    BOOL missing = FALSE;

    TRACE("%s %s\n", debugstr_w(szProduct), debugstr_w(szFeature));

    if (!szProduct || !szFeature)
        return INSTALLSTATE_INVALIDARG;

    if (!squash_guid( szProduct, squishProduct ))
        return INSTALLSTATE_INVALIDARG;

    /* check that it's installed at all */
    rc = MSIREG_OpenUserFeaturesKey(szProduct, &hkey, FALSE);
    if (rc != ERROR_SUCCESS)
        return INSTALLSTATE_UNKNOWN;

    parent_feature = msi_reg_get_val_str( hkey, szFeature );
    RegCloseKey(hkey);

    if (!parent_feature)
        return INSTALLSTATE_UNKNOWN;

    r = (parent_feature[0] == 6) ? INSTALLSTATE_ABSENT : INSTALLSTATE_LOCAL;
    msi_free(parent_feature);
    if (r == INSTALLSTATE_ABSENT)
        return r;

    /* now check if it's complete or advertised */
    rc = MSIREG_OpenFeaturesKey(szProduct, &hkey, FALSE);
    if (rc != ERROR_SUCCESS)
        return INSTALLSTATE_UNKNOWN;

    components = msi_reg_get_val_str( hkey, szFeature );
    RegCloseKey(hkey);

    TRACE("rc = %d buffer = %s\n", rc, debugstr_w(components));

    if (!components)
    {
        ERR("components missing %s %s\n",
            debugstr_w(szProduct), debugstr_w(szFeature));
        return INSTALLSTATE_UNKNOWN;
    }

    for( p = components; *p != 2 ; p += 20)
    {
        if (!decode_base85_guid( p, &guid ))
        {
            ERR("%s\n", debugstr_w(p));
            break;
        }
        StringFromGUID2(&guid, comp, GUID_SIZE);
        r = MsiGetComponentPathW(szProduct, comp, NULL, 0);
        TRACE("component %s state %d\n", debugstr_guid(&guid), r);
        switch (r)
        {
        case INSTALLSTATE_NOTUSED:
        case INSTALLSTATE_LOCAL:
        case INSTALLSTATE_SOURCE:
            break;
        default:
            missing = TRUE;
        }
    }

    TRACE("%s %s -> %d\n", debugstr_w(szProduct), debugstr_w(szFeature), r);
    msi_free(components);

    if (missing)
        return INSTALLSTATE_ADVERTISED;

    return INSTALLSTATE_LOCAL;
}

/******************************************************************
 * MsiGetFileVersionA         [MSI.@]
 */
UINT WINAPI MsiGetFileVersionA(LPCSTR szFilePath, LPSTR lpVersionBuf,
                DWORD* pcchVersionBuf, LPSTR lpLangBuf, DWORD* pcchLangBuf)
{
    LPWSTR szwFilePath = NULL, lpwVersionBuff = NULL, lpwLangBuff = NULL;
    UINT ret = ERROR_OUTOFMEMORY;

    if( szFilePath )
    {
        szwFilePath = strdupAtoW( szFilePath );
        if( !szwFilePath )
            goto end;
    }

    if( lpVersionBuf && pcchVersionBuf && *pcchVersionBuf )
    {
        lpwVersionBuff = msi_alloc(*pcchVersionBuf*sizeof(WCHAR));
        if( !lpwVersionBuff )
            goto end;
    }

    if( lpLangBuf && pcchLangBuf && *pcchLangBuf )
    {
        lpwLangBuff = msi_alloc(*pcchVersionBuf*sizeof(WCHAR));
        if( !lpwLangBuff )
            goto end;
    }

    ret = MsiGetFileVersionW(szwFilePath, lpwVersionBuff, pcchVersionBuf,
                             lpwLangBuff, pcchLangBuf);

    if( lpwVersionBuff )
        WideCharToMultiByte(CP_ACP, 0, lpwVersionBuff, -1,
                            lpVersionBuf, *pcchVersionBuf, NULL, NULL);
    if( lpwLangBuff )
        WideCharToMultiByte(CP_ACP, 0, lpwLangBuff, -1,
                            lpLangBuf, *pcchLangBuf, NULL, NULL);

end:
    msi_free(szwFilePath);
    msi_free(lpwVersionBuff);
    msi_free(lpwLangBuff);

    return ret;
}

/******************************************************************
 * MsiGetFileVersionW         [MSI.@]
 */
UINT WINAPI MsiGetFileVersionW(LPCWSTR szFilePath, LPWSTR lpVersionBuf,
                DWORD* pcchVersionBuf, LPWSTR lpLangBuf, DWORD* pcchLangBuf)
{
    static WCHAR szVersionResource[] = {'\\',0};
    static const WCHAR szVersionFormat[] = {
        '%','d','.','%','d','.','%','d','.','%','d',0};
    static const WCHAR szLangFormat[] = {'%','d',0};
    UINT ret = 0;
    DWORD dwVerLen;
    LPVOID lpVer = NULL;
    VS_FIXEDFILEINFO *ffi;
    UINT puLen;
    WCHAR tmp[32];

    TRACE("%s %p %d %p %d\n", debugstr_w(szFilePath),
          lpVersionBuf, pcchVersionBuf?*pcchVersionBuf:0,
          lpLangBuf, pcchLangBuf?*pcchLangBuf:0);

    dwVerLen = GetFileVersionInfoSizeW(szFilePath, NULL);
    if( !dwVerLen )
        return GetLastError();

    lpVer = msi_alloc(dwVerLen);
    if( !lpVer )
    {
        ret = ERROR_OUTOFMEMORY;
        goto end;
    }

    if( !GetFileVersionInfoW(szFilePath, 0, dwVerLen, lpVer) )
    {
        ret = GetLastError();
        goto end;
    }
    if( lpVersionBuf && pcchVersionBuf && *pcchVersionBuf )
    {
        if( VerQueryValueW(lpVer, szVersionResource, (LPVOID*)&ffi, &puLen) &&
            (puLen > 0) )
        {
            wsprintfW(tmp, szVersionFormat,
                  HIWORD(ffi->dwFileVersionMS), LOWORD(ffi->dwFileVersionMS),
                  HIWORD(ffi->dwFileVersionLS), LOWORD(ffi->dwFileVersionLS));
            lstrcpynW(lpVersionBuf, tmp, *pcchVersionBuf);
            *pcchVersionBuf = lstrlenW(lpVersionBuf);
        }
        else
        {
            *lpVersionBuf = 0;
            *pcchVersionBuf = 0;
        }
    }

    if( lpLangBuf && pcchLangBuf && *pcchLangBuf )
    {
        DWORD lang = GetUserDefaultLangID();

        FIXME("Retrieve language from file\n");
        wsprintfW(tmp, szLangFormat, lang);
        lstrcpynW(lpLangBuf, tmp, *pcchLangBuf);
        *pcchLangBuf = lstrlenW(lpLangBuf);
    }

end:
    msi_free(lpVer);
    return ret;
}

/***********************************************************************
 * MsiGetFeatureUsageW           [MSI.@]
 */
UINT WINAPI MsiGetFeatureUsageW( LPCWSTR szProduct, LPCWSTR szFeature,
                                 DWORD* pdwUseCount, WORD* pwDateUsed )
{
    FIXME("%s %s %p %p\n",debugstr_w(szProduct), debugstr_w(szFeature),
          pdwUseCount, pwDateUsed);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 * MsiGetFeatureUsageA           [MSI.@]
 */
UINT WINAPI MsiGetFeatureUsageA( LPCSTR szProduct, LPCSTR szFeature,
                                 DWORD* pdwUseCount, WORD* pwDateUsed )
{
    LPWSTR prod = NULL, feat = NULL;
    UINT ret = ERROR_OUTOFMEMORY;

    TRACE("%s %s %p %p\n", debugstr_a(szProduct), debugstr_a(szFeature),
          pdwUseCount, pwDateUsed);

    prod = strdupAtoW( szProduct );
    if (szProduct && !prod)
        goto end;

    feat = strdupAtoW( szFeature );
    if (szFeature && !feat)
        goto end;

    ret = MsiGetFeatureUsageW( prod, feat, pdwUseCount, pwDateUsed );

end:
    msi_free( prod );
    msi_free( feat );

    return ret;
}

/***********************************************************************
 * MsiUseFeatureExW           [MSI.@]
 */
INSTALLSTATE WINAPI MsiUseFeatureExW( LPCWSTR szProduct, LPCWSTR szFeature,
                                      DWORD dwInstallMode, DWORD dwReserved )
{
    INSTALLSTATE state;

    TRACE("%s %s %i %i\n", debugstr_w(szProduct), debugstr_w(szFeature),
          dwInstallMode, dwReserved);

    state = MsiQueryFeatureStateW( szProduct, szFeature );

    if (dwReserved)
        return INSTALLSTATE_INVALIDARG;

    if (state == INSTALLSTATE_LOCAL && dwInstallMode != INSTALLMODE_NODETECTION)
    {
        FIXME("mark product %s feature %s as used\n",
              debugstr_w(szProduct), debugstr_w(szFeature) );
    }

    return state;
}

/***********************************************************************
 * MsiUseFeatureExA           [MSI.@]
 */
INSTALLSTATE WINAPI MsiUseFeatureExA( LPCSTR szProduct, LPCSTR szFeature,
                                      DWORD dwInstallMode, DWORD dwReserved )
{
    INSTALLSTATE ret = INSTALLSTATE_UNKNOWN;
    LPWSTR prod = NULL, feat = NULL;

    TRACE("%s %s %i %i\n", debugstr_a(szProduct), debugstr_a(szFeature),
          dwInstallMode, dwReserved);

    prod = strdupAtoW( szProduct );
    if (szProduct && !prod)
        goto end;

    feat = strdupAtoW( szFeature );
    if (szFeature && !feat)
        goto end;

    ret = MsiUseFeatureExW( prod, feat, dwInstallMode, dwReserved );

end:
    msi_free( prod );
    msi_free( feat );

    return ret;
}

/***********************************************************************
 * MsiUseFeatureW             [MSI.@]
 */
INSTALLSTATE WINAPI MsiUseFeatureW( LPCWSTR szProduct, LPCWSTR szFeature )
{
    return MsiUseFeatureExW(szProduct, szFeature, 0, 0);
}

/***********************************************************************
 * MsiUseFeatureA             [MSI.@]
 */
INSTALLSTATE WINAPI MsiUseFeatureA( LPCSTR szProduct, LPCSTR szFeature )
{
    return MsiUseFeatureExA(szProduct, szFeature, 0, 0);
}

/***********************************************************************
 * MSI_ProvideQualifiedComponentEx [internal]
 */
static UINT WINAPI MSI_ProvideQualifiedComponentEx(LPCWSTR szComponent,
                LPCWSTR szQualifier, DWORD dwInstallMode, LPWSTR szProduct,
                DWORD Unused1, DWORD Unused2, awstring *lpPathBuf,
                DWORD* pcchPathBuf)
{
    WCHAR product[MAX_FEATURE_CHARS+1], component[MAX_FEATURE_CHARS+1],
          feature[MAX_FEATURE_CHARS+1];
    LPWSTR info;
    HKEY hkey;
    DWORD sz;
    UINT rc;

    TRACE("%s %s %i %s %i %i %p %p\n", debugstr_w(szComponent),
          debugstr_w(szQualifier), dwInstallMode, debugstr_w(szProduct),
          Unused1, Unused2, lpPathBuf, pcchPathBuf);

    rc = MSIREG_OpenUserComponentsKey(szComponent, &hkey, FALSE);
    if (rc != ERROR_SUCCESS)
        return ERROR_INDEX_ABSENT;

    info = msi_reg_get_val_str( hkey, szQualifier );
    RegCloseKey(hkey);

    if (!info)
        return ERROR_INDEX_ABSENT;

    MsiDecomposeDescriptorW(info, product, feature, component, &sz);

    if (!szProduct)
        rc = MSI_GetComponentPath(product, component, lpPathBuf, pcchPathBuf);
    else
        rc = MSI_GetComponentPath(szProduct, component, lpPathBuf, pcchPathBuf);

    msi_free( info );

    if (rc != INSTALLSTATE_LOCAL)
        return ERROR_FILE_NOT_FOUND;

    return ERROR_SUCCESS;
}

/***********************************************************************
 * MsiProvideQualifiedComponentExW [MSI.@]
 */
UINT WINAPI MsiProvideQualifiedComponentExW(LPCWSTR szComponent,
                LPCWSTR szQualifier, DWORD dwInstallMode, LPWSTR szProduct,
                DWORD Unused1, DWORD Unused2, LPWSTR lpPathBuf,
                DWORD* pcchPathBuf)
{
    awstring path;

    path.unicode = TRUE;
    path.str.w = lpPathBuf;

    return MSI_ProvideQualifiedComponentEx(szComponent, szQualifier,
            dwInstallMode, szProduct, Unused1, Unused2, &path, pcchPathBuf);
}

/***********************************************************************
 * MsiProvideQualifiedComponentExA [MSI.@]
 */
UINT WINAPI MsiProvideQualifiedComponentExA(LPCSTR szComponent,
                LPCSTR szQualifier, DWORD dwInstallMode, LPSTR szProduct,
                DWORD Unused1, DWORD Unused2, LPSTR lpPathBuf,
                DWORD* pcchPathBuf)
{
    LPWSTR szwComponent, szwQualifier = NULL, szwProduct = NULL;
    UINT r = ERROR_OUTOFMEMORY;
    awstring path;

    TRACE("%s %s %u %s %u %u %p %p\n", debugstr_a(szComponent),
          debugstr_a(szQualifier), dwInstallMode, debugstr_a(szProduct),
          Unused1, Unused2, lpPathBuf, pcchPathBuf);

    szwComponent = strdupAtoW( szComponent );
    if (szComponent && !szwComponent)
        goto end;

    szwQualifier = strdupAtoW( szQualifier );
    if (szQualifier && !szwQualifier)
        goto end;

    szwProduct = strdupAtoW( szProduct );
    if (szProduct && !szwProduct)
        goto end;

    path.unicode = FALSE;
    path.str.a = lpPathBuf;

    r = MSI_ProvideQualifiedComponentEx(szwComponent, szwQualifier,
                              dwInstallMode, szwProduct, Unused1,
                              Unused2, &path, pcchPathBuf);
end:
    msi_free(szwProduct);
    msi_free(szwComponent);
    msi_free(szwQualifier);

    return r;
}

/***********************************************************************
 * MsiProvideQualifiedComponentW [MSI.@]
 */
UINT WINAPI MsiProvideQualifiedComponentW( LPCWSTR szComponent,
                LPCWSTR szQualifier, DWORD dwInstallMode, LPWSTR lpPathBuf,
                DWORD* pcchPathBuf)
{
    return MsiProvideQualifiedComponentExW(szComponent, szQualifier, 
                    dwInstallMode, NULL, 0, 0, lpPathBuf, pcchPathBuf);
}

/***********************************************************************
 * MsiProvideQualifiedComponentA [MSI.@]
 */
UINT WINAPI MsiProvideQualifiedComponentA( LPCSTR szComponent,
                LPCSTR szQualifier, DWORD dwInstallMode, LPSTR lpPathBuf,
                DWORD* pcchPathBuf)
{
    return MsiProvideQualifiedComponentExA(szComponent, szQualifier,
                              dwInstallMode, NULL, 0, 0, lpPathBuf, pcchPathBuf);
}

/***********************************************************************
 * MSI_GetUserInfo [internal]
 */
static USERINFOSTATE WINAPI MSI_GetUserInfo(LPCWSTR szProduct,
                awstring *lpUserNameBuf, DWORD* pcchUserNameBuf,
                awstring *lpOrgNameBuf, DWORD* pcchOrgNameBuf,
                awstring *lpSerialBuf, DWORD* pcchSerialBuf)
{
    HKEY hkey;
    LPWSTR user, org, serial;
    UINT r;
    USERINFOSTATE state;

    TRACE("%s %p %p %p %p %p %p\n",debugstr_w(szProduct), lpUserNameBuf,
          pcchUserNameBuf, lpOrgNameBuf, pcchOrgNameBuf, lpSerialBuf,
          pcchSerialBuf);

    if (!szProduct)
        return USERINFOSTATE_INVALIDARG;

    r = MSIREG_OpenUninstallKey(szProduct, &hkey, FALSE);
    if (r != ERROR_SUCCESS)
        return USERINFOSTATE_UNKNOWN;

    user = msi_reg_get_val_str( hkey, INSTALLPROPERTY_REGOWNERW );
    org = msi_reg_get_val_str( hkey, INSTALLPROPERTY_REGCOMPANYW );
    serial = msi_reg_get_val_str( hkey, INSTALLPROPERTY_PRODUCTIDW );

    RegCloseKey(hkey);

    state = USERINFOSTATE_PRESENT;

    r = msi_strcpy_to_awstring( user, lpUserNameBuf, pcchUserNameBuf );
    if (r == ERROR_MORE_DATA)
        state = USERINFOSTATE_MOREDATA;
    r = msi_strcpy_to_awstring( org, lpOrgNameBuf, pcchOrgNameBuf );
    if (r == ERROR_MORE_DATA)
        state = USERINFOSTATE_MOREDATA;
    r = msi_strcpy_to_awstring( serial, lpSerialBuf, pcchSerialBuf );
    if (r == ERROR_MORE_DATA)
        state = USERINFOSTATE_MOREDATA;

    msi_free( user );
    msi_free( org );
    msi_free( serial );

    return state;
}

/***********************************************************************
 * MsiGetUserInfoW [MSI.@]
 */
USERINFOSTATE WINAPI MsiGetUserInfoW(LPCWSTR szProduct,
                LPWSTR lpUserNameBuf, DWORD* pcchUserNameBuf,
                LPWSTR lpOrgNameBuf, DWORD* pcchOrgNameBuf,
                LPWSTR lpSerialBuf, DWORD* pcchSerialBuf)
{
    awstring user, org, serial;

    user.unicode = TRUE;
    user.str.w = lpUserNameBuf;
    org.unicode = TRUE;
    org.str.w = lpOrgNameBuf;
    serial.unicode = TRUE;
    serial.str.w = lpSerialBuf;

    return MSI_GetUserInfo( szProduct, &user, pcchUserNameBuf,
                            &org, pcchOrgNameBuf,
                            &serial, pcchSerialBuf );
}

USERINFOSTATE WINAPI MsiGetUserInfoA(LPCSTR szProduct,
                LPSTR lpUserNameBuf, DWORD* pcchUserNameBuf,
                LPSTR lpOrgNameBuf, DWORD* pcchOrgNameBuf,
                LPSTR lpSerialBuf, DWORD* pcchSerialBuf)
{
    awstring user, org, serial;
    LPWSTR prod;
    UINT r;

    prod = strdupAtoW( szProduct );
    if (szProduct && !prod)
        return ERROR_OUTOFMEMORY;

    user.unicode = FALSE;
    user.str.a = lpUserNameBuf;
    org.unicode = FALSE;
    org.str.a = lpOrgNameBuf;
    serial.unicode = FALSE;
    serial.str.a = lpSerialBuf;

    r = MSI_GetUserInfo( prod, &user, pcchUserNameBuf,
                         &org, pcchOrgNameBuf,
                         &serial, pcchSerialBuf );

    msi_free( prod );

    return r;
}

UINT WINAPI MsiCollectUserInfoW(LPCWSTR szProduct)
{
    MSIHANDLE handle;
    UINT rc;
    MSIPACKAGE *package;
    static const WCHAR szFirstRun[] = {'F','i','r','s','t','R','u','n',0};

    TRACE("(%s)\n",debugstr_w(szProduct));

    rc = MsiOpenProductW(szProduct,&handle);
    if (rc != ERROR_SUCCESS)
        return ERROR_INVALID_PARAMETER;

    package = msihandle2msiinfo(handle, MSIHANDLETYPE_PACKAGE);
    rc = ACTION_PerformUIAction(package, szFirstRun);
    msiobj_release( &package->hdr );

    MsiCloseHandle(handle);

    return rc;
}

UINT WINAPI MsiCollectUserInfoA(LPCSTR szProduct)
{
    MSIHANDLE handle;
    UINT rc;
    MSIPACKAGE *package;
    static const WCHAR szFirstRun[] = {'F','i','r','s','t','R','u','n',0};

    TRACE("(%s)\n",debugstr_a(szProduct));

    rc = MsiOpenProductA(szProduct,&handle);
    if (rc != ERROR_SUCCESS)
        return ERROR_INVALID_PARAMETER;

    package = msihandle2msiinfo(handle, MSIHANDLETYPE_PACKAGE);
    rc = ACTION_PerformUIAction(package, szFirstRun);
    msiobj_release( &package->hdr );

    MsiCloseHandle(handle);

    return rc;
}

/***********************************************************************
 * MsiConfigureFeatureA            [MSI.@]
 */
UINT WINAPI MsiConfigureFeatureA(LPCSTR szProduct, LPCSTR szFeature, INSTALLSTATE eInstallState)
{
    LPWSTR prod, feat = NULL;
    UINT r = ERROR_OUTOFMEMORY;

    TRACE("%s %s %i\n", debugstr_a(szProduct), debugstr_a(szFeature), eInstallState);

    prod = strdupAtoW( szProduct );
    if (szProduct && !prod)
        goto end;

    feat = strdupAtoW( szFeature );
    if (szFeature && !feat)
        goto end;

    r = MsiConfigureFeatureW(prod, feat, eInstallState);

end:
    msi_free(feat);
    msi_free(prod);

    return r;
}

/***********************************************************************
 * MsiConfigureFeatureW            [MSI.@]
 */
UINT WINAPI MsiConfigureFeatureW(LPCWSTR szProduct, LPCWSTR szFeature, INSTALLSTATE eInstallState)
{
    static const WCHAR szCostInit[] = { 'C','o','s','t','I','n','i','t','i','a','l','i','z','e',0 };
    MSIPACKAGE *package = NULL;
    UINT r;
    WCHAR sourcepath[MAX_PATH], filename[MAX_PATH];
    DWORD sz;

    TRACE("%s %s %i\n", debugstr_w(szProduct), debugstr_w(szFeature), eInstallState);

    if (!szProduct || !szFeature)
        return ERROR_INVALID_PARAMETER;

    switch (eInstallState)
    {
    case INSTALLSTATE_DEFAULT:
        /* FIXME: how do we figure out the default location? */
        eInstallState = INSTALLSTATE_LOCAL;
        break;
    case INSTALLSTATE_LOCAL:
    case INSTALLSTATE_SOURCE:
    case INSTALLSTATE_ABSENT:
    case INSTALLSTATE_ADVERTISED:
        break;
    default:
        return ERROR_INVALID_PARAMETER;
    }

    r = MSI_OpenProductW( szProduct, &package );
    if (r != ERROR_SUCCESS)
        return r;

    sz = sizeof(sourcepath);
    MsiSourceListGetInfoW(szProduct, NULL, MSIINSTALLCONTEXT_USERMANAGED,
                MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDSOURCEW, sourcepath, &sz);

    sz = sizeof(filename);
    MsiSourceListGetInfoW(szProduct, NULL, MSIINSTALLCONTEXT_USERMANAGED,
                MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAMEW, filename, &sz);

    lstrcatW( sourcepath, filename );

    MsiSetInternalUI( INSTALLUILEVEL_BASIC, NULL );

    r = ACTION_PerformUIAction( package, szCostInit );
    if (r != ERROR_SUCCESS)
        goto end;

    r = MSI_SetFeatureStateW( package, szFeature, eInstallState);
    if (r != ERROR_SUCCESS)
        goto end;

    r = MSI_InstallPackage( package, sourcepath, NULL );

end:
    msiobj_release( &package->hdr );

    return r;
}

/***********************************************************************
 * MsiCreateAndVerifyInstallerDirectory [MSI.@]
 *
 * Notes: undocumented
 */
UINT WINAPI MsiCreateAndVerifyInstallerDirectory(DWORD dwReserved)
{
    WCHAR path[MAX_PATH];

    TRACE("%d\n", dwReserved);

    if (dwReserved)
    {
        FIXME("dwReserved=%d\n", dwReserved);
        return ERROR_INVALID_PARAMETER;
    }

    if (!GetWindowsDirectoryW(path, MAX_PATH))
        return ERROR_FUNCTION_FAILED;

    lstrcatW(path, installerW);

    if (!CreateDirectoryW(path, NULL))
        return ERROR_FUNCTION_FAILED;

    return ERROR_SUCCESS;
}

/***********************************************************************
 * MsiGetShortcutTargetA           [MSI.@]
 */
UINT WINAPI MsiGetShortcutTargetA( LPCSTR szShortcutTarget,
                                   LPSTR szProductCode, LPSTR szFeatureId,
                                   LPSTR szComponentCode )
{
    LPWSTR target;
    const int len = MAX_FEATURE_CHARS+1;
    WCHAR product[MAX_FEATURE_CHARS+1], feature[MAX_FEATURE_CHARS+1], component[MAX_FEATURE_CHARS+1];
    UINT r;

    target = strdupAtoW( szShortcutTarget );
    if (szShortcutTarget && !target )
        return ERROR_OUTOFMEMORY;
    product[0] = 0;
    feature[0] = 0;
    component[0] = 0;
    r = MsiGetShortcutTargetW( target, product, feature, component );
    msi_free( target );
    if (r == ERROR_SUCCESS)
    {
        WideCharToMultiByte( CP_ACP, 0, product, -1, szProductCode, len, NULL, NULL );
        WideCharToMultiByte( CP_ACP, 0, feature, -1, szFeatureId, len, NULL, NULL );
        WideCharToMultiByte( CP_ACP, 0, component, -1, szComponentCode, len, NULL, NULL );
    }
    return r;
}

/***********************************************************************
 * MsiGetShortcutTargetW           [MSI.@]
 */
UINT WINAPI MsiGetShortcutTargetW( LPCWSTR szShortcutTarget,
                                   LPWSTR szProductCode, LPWSTR szFeatureId,
                                   LPWSTR szComponentCode )
{
    IShellLinkDataList *dl = NULL;
    IPersistFile *pf = NULL;
    LPEXP_DARWIN_LINK darwin = NULL;
    HRESULT r, init;

    TRACE("%s %p %p %p\n", debugstr_w(szShortcutTarget),
          szProductCode, szFeatureId, szComponentCode );

    init = CoInitialize(NULL);

    r = CoCreateInstance( &CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IPersistFile, (LPVOID*) &pf );
    if( SUCCEEDED( r ) )
    {
        r = IPersistFile_Load( pf, szShortcutTarget,
                               STGM_READ | STGM_SHARE_DENY_WRITE );
        if( SUCCEEDED( r ) )
        {
            r = IPersistFile_QueryInterface( pf, &IID_IShellLinkDataList,
                                             (LPVOID*) &dl );
            if( SUCCEEDED( r ) )
            {
                IShellLinkDataList_CopyDataBlock( dl, EXP_DARWIN_ID_SIG,
                                                  (LPVOID) &darwin );
                IShellLinkDataList_Release( dl );
            }
        }
        IPersistFile_Release( pf );
    }

    if (SUCCEEDED(init))
        CoUninitialize();

    TRACE("darwin = %p\n", darwin);

    if (darwin)
    {
        DWORD sz;
        UINT ret;

        ret = MsiDecomposeDescriptorW( darwin->szwDarwinID,
                  szProductCode, szFeatureId, szComponentCode, &sz );
        LocalFree( darwin );
        return ret;
    }

    return ERROR_FUNCTION_FAILED;
}

UINT WINAPI MsiReinstallFeatureW( LPCWSTR szProduct, LPCWSTR szFeature,
                                  DWORD dwReinstallMode )
{
    MSIPACKAGE* package = NULL;
    UINT r;
    WCHAR sourcepath[MAX_PATH];
    WCHAR filename[MAX_PATH];
    static const WCHAR szLogVerbose[] = {
        ' ','L','O','G','V','E','R','B','O','S','E',0 };
    static const WCHAR szInstalled[] = { 'I','n','s','t','a','l','l','e','d',0};
    static const WCHAR szReinstall[] = {'R','E','I','N','S','T','A','L','L',0};
    static const WCHAR szReinstallMode[] = {'R','E','I','N','S','T','A','L','L','M','O','D','E',0};
    static const WCHAR szOne[] = {'1',0};
    WCHAR reinstallmode[11];
    LPWSTR ptr;
    DWORD sz;

    FIXME("%s %s %i\n", debugstr_w(szProduct), debugstr_w(szFeature),
                           dwReinstallMode);

    ptr = reinstallmode;

    if (dwReinstallMode & REINSTALLMODE_FILEMISSING)
        *ptr++ = 'p';
    if (dwReinstallMode & REINSTALLMODE_FILEOLDERVERSION)
        *ptr++ = 'o';
    if (dwReinstallMode & REINSTALLMODE_FILEEQUALVERSION)
        *ptr++ = 'w';
    if (dwReinstallMode & REINSTALLMODE_FILEEXACT)
        *ptr++ = 'd';
    if (dwReinstallMode & REINSTALLMODE_FILEVERIFY)
        *ptr++ = 'c';
    if (dwReinstallMode & REINSTALLMODE_FILEREPLACE)
        *ptr++ = 'a';
    if (dwReinstallMode & REINSTALLMODE_USERDATA)
        *ptr++ = 'u';
    if (dwReinstallMode & REINSTALLMODE_MACHINEDATA)
        *ptr++ = 'm';
    if (dwReinstallMode & REINSTALLMODE_SHORTCUT)
        *ptr++ = 's';
    if (dwReinstallMode & REINSTALLMODE_PACKAGE)
        *ptr++ = 'v';
    *ptr = 0;
    
    sz = sizeof(sourcepath);
    MsiSourceListGetInfoW(szProduct, NULL, MSIINSTALLCONTEXT_USERMANAGED, 
            MSICODE_PRODUCT, INSTALLPROPERTY_LASTUSEDSOURCEW, sourcepath, &sz);

    sz = sizeof(filename);
    MsiSourceListGetInfoW(szProduct, NULL, MSIINSTALLCONTEXT_USERMANAGED, 
            MSICODE_PRODUCT, INSTALLPROPERTY_PACKAGENAMEW, filename, &sz);

    lstrcatW( sourcepath, filename );

    if (dwReinstallMode & REINSTALLMODE_PACKAGE)
        r = MSI_OpenPackageW( sourcepath, &package );
    else
        r = MSI_OpenProductW( szProduct, &package );

    if (r != ERROR_SUCCESS)
        return r;

    MSI_SetPropertyW( package, szReinstallMode, reinstallmode );
    MSI_SetPropertyW( package, szInstalled, szOne );
    MSI_SetPropertyW( package, szLogVerbose, szOne );
    MSI_SetPropertyW( package, szReinstall, szFeature );

    r = MSI_InstallPackage( package, sourcepath, NULL );

    msiobj_release( &package->hdr );

    return r;
}

UINT WINAPI MsiReinstallFeatureA( LPCSTR szProduct, LPCSTR szFeature,
                                  DWORD dwReinstallMode )
{
    LPWSTR wszProduct;
    LPWSTR wszFeature;
    UINT rc;

    TRACE("%s %s %i\n", debugstr_a(szProduct), debugstr_a(szFeature),
                           dwReinstallMode);

    wszProduct = strdupAtoW(szProduct);
    wszFeature = strdupAtoW(szFeature);

    rc = MsiReinstallFeatureW(wszProduct, wszFeature, dwReinstallMode);

    msi_free(wszProduct);
    msi_free(wszFeature);
    return rc;
}

typedef struct
{
    unsigned int i[2];
    unsigned int buf[4];
    unsigned char in[64];
    unsigned char digest[16];
} MD5_CTX;

extern VOID WINAPI MD5Init( MD5_CTX *);
extern VOID WINAPI MD5Update( MD5_CTX *, const unsigned char *, unsigned int );
extern VOID WINAPI MD5Final( MD5_CTX *);

/***********************************************************************
 * MsiGetFileHashW            [MSI.@]
 */
UINT WINAPI MsiGetFileHashW( LPCWSTR szFilePath, DWORD dwOptions,
                             PMSIFILEHASHINFO pHash )
{
    HANDLE handle, mapping;
    void *p;
    DWORD length;
    UINT r = ERROR_FUNCTION_FAILED;

    TRACE("%s %08x %p\n", debugstr_w(szFilePath), dwOptions, pHash );

    if (dwOptions)
        return ERROR_INVALID_PARAMETER;
    if (!pHash)
        return ERROR_INVALID_PARAMETER;
    if (pHash->dwFileHashInfoSize < sizeof *pHash)
        return ERROR_INVALID_PARAMETER;

    handle = CreateFileW( szFilePath, GENERIC_READ,
                          FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL );
    if (handle == INVALID_HANDLE_VALUE)
        return ERROR_FILE_NOT_FOUND;

    length = GetFileSize( handle, NULL );

    mapping = CreateFileMappingW( handle, NULL, PAGE_READONLY, 0, 0, NULL );
    if (mapping)
    {
        p = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, length );
        if (p)
        {
            MD5_CTX ctx;

            MD5Init( &ctx );
            MD5Update( &ctx, p, length );
            MD5Final( &ctx );
            UnmapViewOfFile( p );

            memcpy( pHash->dwData, &ctx.digest, sizeof pHash->dwData );
            r = ERROR_SUCCESS;
        }
        CloseHandle( mapping );
    }
    CloseHandle( handle );

    return r;
}

/***********************************************************************
 * MsiGetFileHashA            [MSI.@]
 */
UINT WINAPI MsiGetFileHashA( LPCSTR szFilePath, DWORD dwOptions,
                             PMSIFILEHASHINFO pHash )
{
    LPWSTR file;
    UINT r;

    TRACE("%s %08x %p\n", debugstr_a(szFilePath), dwOptions, pHash );

    file = strdupAtoW( szFilePath );
    if (szFilePath && !file)
        return ERROR_OUTOFMEMORY;

    r = MsiGetFileHashW( file, dwOptions, pHash );
    msi_free( file );
    return r;
}

/***********************************************************************
 * MsiAdvertiseScriptW        [MSI.@]
 */
UINT WINAPI MsiAdvertiseScriptW( LPCWSTR szScriptFile, DWORD dwFlags,
                                 PHKEY phRegData, BOOL fRemoveItems )
{
    FIXME("%s %08x %p %d\n",
          debugstr_w( szScriptFile ), dwFlags, phRegData, fRemoveItems );
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 * MsiAdvertiseScriptA        [MSI.@]
 */
UINT WINAPI MsiAdvertiseScriptA( LPCSTR szScriptFile, DWORD dwFlags,
                                 PHKEY phRegData, BOOL fRemoveItems )
{
    FIXME("%s %08x %p %d\n",
          debugstr_a( szScriptFile ), dwFlags, phRegData, fRemoveItems );
    return ERROR_CALL_NOT_IMPLEMENTED;
}
