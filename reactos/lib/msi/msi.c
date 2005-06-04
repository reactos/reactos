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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#include "action.h"

UINT WINAPI MsiGetFileVersionW(LPCWSTR szFilePath, LPWSTR lpVersionBuf, DWORD* pcchVersionBuf, LPWSTR lpLangBuf, DWORD* pcchLangBuf);


WINE_DEFAULT_DEBUG_CHANNEL(msi);

/*
 * The MSVC headers define the MSIDBOPEN_* macros cast to LPCTSTR,
 *  which is a problem because LPCTSTR isn't defined when compiling wine.
 * To work around this problem, we need to define LPCTSTR as LPCWSTR here,
 *  and make sure to only use it in W functions.
 */
#define LPCTSTR LPCWSTR

/* the UI level */
INSTALLUILEVEL gUILevel = INSTALLUILEVEL_BASIC;
HWND           gUIhwnd = 0;
INSTALLUI_HANDLERA gUIHandlerA = NULL;
INSTALLUI_HANDLERW gUIHandlerW = NULL;
DWORD gUIFilter = 0;
LPVOID gUIContext = NULL;
WCHAR gszLogFile[MAX_PATH];
HINSTANCE msi_hInstance;

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

    HeapFree( GetProcessHeap(), 0, szwProd );

    return r;
}

UINT WINAPI MsiOpenProductW(LPCWSTR szProduct, MSIHANDLE *phProduct)
{
    static const WCHAR szLocalPackage[] = {
        'L','o','c','a','l','P','a','c','k','a','g','e', 0
    };
    LPWSTR path = NULL;
    UINT r;
    HKEY hKeyProduct = NULL;
    DWORD count, type;

    TRACE("%s %p\n",debugstr_w(szProduct), phProduct);

    r = MSIREG_OpenUninstallKey(szProduct,&hKeyProduct,FALSE);
    if( r != ERROR_SUCCESS )
    {
        r = ERROR_UNKNOWN_PRODUCT;
        goto end;
    }

    /* find the size of the path */
    type = count = 0;
    r = RegQueryValueExW( hKeyProduct, szLocalPackage,
                          NULL, &type, NULL, &count );
    if( r != ERROR_SUCCESS )
    {
        r = ERROR_UNKNOWN_PRODUCT;
        goto end;
    }

    /* now alloc and fetch the path of the database to open */
    path = HeapAlloc( GetProcessHeap(), 0, count );
    if( !path )
        goto end;

    r = RegQueryValueExW( hKeyProduct, szLocalPackage,
                          NULL, &type, (LPBYTE) path, &count );
    if( r != ERROR_SUCCESS )
    {
        r = ERROR_UNKNOWN_PRODUCT;
        goto end;
    }

    r = MsiOpenPackageW( path, phProduct );

end:
    HeapFree( GetProcessHeap(), 0, path );
    if( hKeyProduct )
        RegCloseKey( hKeyProduct );

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
    FIXME("%s %s %s %08x %08lx %08lx\n", debugstr_a(szPackagePath),
          debugstr_a(szScriptfilePath), debugstr_a(szTransforms),
          lgidLanguage, dwPlatform, dwOptions);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiAdvertiseProductExW( LPCWSTR szPackagePath, LPCWSTR szScriptfilePath,
      LPCWSTR szTransforms, LANGID lgidLanguage, DWORD dwPlatform, DWORD dwOptions)
{
    FIXME("%s %s %s %08x %08lx %08lx\n", debugstr_w(szPackagePath),
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
    HeapFree( GetProcessHeap(), 0, szwPath );
    HeapFree( GetProcessHeap(), 0, szwCommand );

    return r;
}

UINT WINAPI MsiInstallProductW(LPCWSTR szPackagePath, LPCWSTR szCommandLine)
{
    MSIPACKAGE *package = NULL;
    UINT r;
    MSIHANDLE handle;

    FIXME("%s %s\n",debugstr_w(szPackagePath), debugstr_w(szCommandLine));

    r = MsiVerifyPackageW(szPackagePath);
    if (r != ERROR_SUCCESS)
        return r;

    r = MSI_OpenPackageW(szPackagePath,&package);
    if (r != ERROR_SUCCESS)
        return r;

    handle = alloc_msihandle( &package->hdr );

    r = ACTION_DoTopLevelINSTALL(package, szPackagePath, szCommandLine);

    MsiCloseHandle(handle);
    msiobj_release( &package->hdr );
    return r;
}

UINT WINAPI MsiReinstallProductA(LPCSTR szProduct, DWORD dwReinstallMode)
{
    FIXME("%s %08lx\n", debugstr_a(szProduct), dwReinstallMode);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiReinstallProductW(LPCWSTR szProduct, DWORD dwReinstallMode)
{
    FIXME("%s %08lx\n", debugstr_w(szProduct), dwReinstallMode);
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
    MSIHANDLE handle = -1;
    MSIPACKAGE* package;
    UINT rc;
    HKEY hkey=0,hkey1=0;
    DWORD sz;
    static const WCHAR szSouceList[] = {
        'S','o','u','r','c','e','L','i','s','t',0};
    static const WCHAR szLUS[] = {
        'L','a','s','t','U','s','e','d','S','o','u','r','c','e',0};
    WCHAR sourcepath[0x200];
    static const WCHAR szInstalled[] = {
        ' ','I','n','s','t','a','l','l','e','d','=','1',0};
    LPWSTR commandline;

    FIXME("%s %d %d %s\n",debugstr_w(szProduct), iInstallLevel, eInstallState,
          debugstr_w(szCommandLine));

    if (eInstallState != INSTALLSTATE_LOCAL &&
        eInstallState != INSTALLSTATE_DEFAULT)
    {
        FIXME("Not implemented for anything other than local installs\n");
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    rc = MSIREG_OpenUserProductsKey(szProduct,&hkey,FALSE);
    if (rc != ERROR_SUCCESS)
        goto end;

    rc = RegOpenKeyW(hkey,szSouceList,&hkey1);
    if (rc != ERROR_SUCCESS)
        goto end;

    sz = sizeof(sourcepath);
    rc = RegQueryValueExW(hkey1, szLUS, NULL, NULL,(LPBYTE)sourcepath, &sz);
    if (rc != ERROR_SUCCESS)
        goto end;

    RegCloseKey(hkey1);
    /*
     * ok 1, we need to find the msi file for this product.
     *    2, find the source dir for the files
     *    3, do the configure/install.
     *    4, cleanupany runonce entry.
     */

    rc = MsiOpenProductW(szProduct,&handle);
    if (rc != ERROR_SUCCESS)
        goto end;

    package = msihandle2msiinfo(handle, MSIHANDLETYPE_PACKAGE);
    if (!package)
    {
        rc = ERROR_INVALID_HANDLE;
        goto end;
    }

    sz = lstrlenW(szInstalled);

    if (szCommandLine)
        sz += lstrlenW(szCommandLine);

    commandline = HeapAlloc(GetProcessHeap(),0,sz * sizeof(WCHAR));

    if (szCommandLine)
        lstrcpyW(commandline,szCommandLine);
    else
        commandline[0] = 0;

    if (MsiQueryProductStateW(szProduct) != INSTALLSTATE_UNKNOWN)
        lstrcatW(commandline,szInstalled);

    rc = ACTION_DoTopLevelINSTALL(package, sourcepath, commandline);

    msiobj_release( &package->hdr );

    HeapFree(GetProcessHeap(),0,commandline);
end:
    RegCloseKey(hkey);
    if (handle != -1)
        MsiCloseHandle(handle);

    return rc;
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
    HeapFree( GetProcessHeap(), 0, szwProduct );
    HeapFree( GetProcessHeap(), 0, szwCommandLine);

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
    HeapFree( GetProcessHeap(), 0, szwProduct );

    return r;
}

UINT WINAPI MsiConfigureProductW(LPCWSTR szProduct, int iInstallLevel,
                                 INSTALLSTATE eInstallState)
{
    FIXME("%s %d %d\n", debugstr_w(szProduct), iInstallLevel, eInstallState);

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

    HeapFree( GetProcessHeap(), 0, szwComponent );

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
          '0','0','0','0','0','0','0', '0','0','0','0','0',
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

UINT WINAPI MsiGetProductInfoA(LPCSTR szProduct, LPCSTR szAttribute,
                 LPSTR szBuffer, DWORD *pcchValueBuf)
{
    LPWSTR szwProduct = NULL, szwAttribute = NULL, szwBuffer = NULL;
    UINT r = ERROR_OUTOFMEMORY;

    TRACE("%s %s %p %p\n", debugstr_a(szProduct), debugstr_a(szAttribute),
          szBuffer, pcchValueBuf);

    if( szProduct )
    {
        szwProduct = strdupAtoW( szProduct );
        if( !szwProduct )
            goto end;
    }
    
    if( szAttribute )
    {
        szwAttribute = strdupAtoW( szAttribute );
        if( !szwAttribute )
            goto end;
    }

    if( szBuffer )
    {
        szwBuffer = HeapAlloc( GetProcessHeap(), 0, (*pcchValueBuf) * sizeof(WCHAR) );
        if( !szwBuffer )     
            goto end;
    }

    r = MsiGetProductInfoW( szwProduct, szwAttribute, szwBuffer, pcchValueBuf );

    if( ERROR_SUCCESS == r )
        WideCharToMultiByte(CP_ACP, 0, szwBuffer, -1, szBuffer, *pcchValueBuf, NULL, NULL);

end:
    HeapFree( GetProcessHeap(), 0, szwProduct );
    HeapFree( GetProcessHeap(), 0, szwAttribute );
    HeapFree( GetProcessHeap(), 0, szwBuffer );

    return r;
}

UINT WINAPI MsiGetProductInfoW(LPCWSTR szProduct, LPCWSTR szAttribute,
                LPWSTR szBuffer, DWORD *pcchValueBuf)
{
    MSIHANDLE hProduct;
    UINT r;
    static const WCHAR szPackageCode[] =
        {'P','a','c','k','a','g','e','C','o','d','e',0};
    static const WCHAR szVersionString[] =
        {'V','e','r','s','i','o','n','S','t','r','i','n','g',0};
    static const WCHAR szProductVersion[] =
        {'P','r','o','d','u','c','t','V','e','r','s','i','o','n',0};
    static const WCHAR szAssignmentType[] =
        {'A','s','s','i','g','n','m','e','n','t','T','y','p','e',0};

    FIXME("%s %s %p %p\n",debugstr_w(szProduct), debugstr_w(szAttribute),
          szBuffer, pcchValueBuf);

    if (NULL != szBuffer && NULL == pcchValueBuf)
        return ERROR_INVALID_PARAMETER;
    if (NULL == szProduct || NULL == szAttribute)
        return ERROR_INVALID_PARAMETER;
    
    /* check for special properties */
    if (strcmpW(szAttribute, szPackageCode)==0)
    {
        HKEY hkey;
        WCHAR squished[GUID_SIZE];
        WCHAR package[200];
        DWORD sz = sizeof(squished);

        r = MSIREG_OpenUserProductsKey(szProduct, &hkey, FALSE);
        if (r != ERROR_SUCCESS)
            return ERROR_UNKNOWN_PRODUCT;

        r = RegQueryValueExW(hkey, szPackageCode, NULL, NULL, 
                        (LPBYTE)squished, &sz);
        if (r != ERROR_SUCCESS)
        {
            RegCloseKey(hkey);
            return ERROR_UNKNOWN_PRODUCT;
        }

        unsquash_guid(squished, package);
        *pcchValueBuf = strlenW(package);
        if (strlenW(package) > *pcchValueBuf)
        {
            RegCloseKey(hkey);
            return ERROR_MORE_DATA;
        }
        else
            strcpyW(szBuffer, package);

        RegCloseKey(hkey);
        r = ERROR_SUCCESS;
    }
    else if (strcmpW(szAttribute, szVersionString)==0)
    {
        r = MsiOpenProductW(szProduct, &hProduct);
        if (ERROR_SUCCESS != r)
            return r;

        r = MsiGetPropertyW(hProduct, szProductVersion, szBuffer, pcchValueBuf);
        MsiCloseHandle(hProduct);
    }
    else if (strcmpW(szAttribute, szAssignmentType)==0)
    {
        FIXME("0 (zero) if advertised, 1(one) if per machine.\n");
        if (szBuffer)
            szBuffer[0] = 1;
        r = ERROR_SUCCESS;
    }
    else
    {
        r = MsiOpenProductW(szProduct, &hProduct);
        if (ERROR_SUCCESS != r)
            return r;

        r = MsiGetPropertyW(hProduct, szAttribute, szBuffer, pcchValueBuf);
        MsiCloseHandle(hProduct);
    }

    return r;
}

UINT WINAPI MsiEnableLogA(DWORD dwLogMode, LPCSTR szLogFile, DWORD attributes)
{
    LPWSTR szwLogFile = NULL;
    UINT r;

    TRACE("%08lx %s %08lx\n", dwLogMode, debugstr_a(szLogFile), attributes);

    if( szLogFile )
    {
        szwLogFile = strdupAtoW( szLogFile );
        if( !szwLogFile )
            return ERROR_OUTOFMEMORY;
    }
    r = MsiEnableLogW( dwLogMode, szwLogFile, attributes );
    HeapFree( GetProcessHeap(), 0, szwLogFile );
    return r;
}

UINT WINAPI MsiEnableLogW(DWORD dwLogMode, LPCWSTR szLogFile, DWORD attributes)
{
    HANDLE file = INVALID_HANDLE_VALUE;

    TRACE("%08lx %s %08lx\n", dwLogMode, debugstr_w(szLogFile), attributes);

    lstrcpyW(gszLogFile,szLogFile);
    if (!(attributes & INSTALLLOGATTRIBUTES_APPEND))
        DeleteFileW(szLogFile);
    file = CreateFileW(szLogFile, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, NULL);
    if (file != INVALID_HANDLE_VALUE)
        CloseHandle(file);
    else
        ERR("Unable to enable log %s\n",debugstr_w(szLogFile));

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
    HeapFree( GetProcessHeap(), 0, szwProduct );
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

    TRACE("%p %lx %p\n",puiHandler, dwMessageFilter,pvContext);
    gUIHandlerA = puiHandler;
    gUIFilter = dwMessageFilter;
    gUIContext = pvContext;

    return prev;
}

INSTALLUI_HANDLERW WINAPI MsiSetExternalUIW(INSTALLUI_HANDLERW puiHandler,
                                  DWORD dwMessageFilter, LPVOID pvContext)
{
    INSTALLUI_HANDLERW prev = gUIHandlerW;

    TRACE("%p %lx %p\n",puiHandler,dwMessageFilter,pvContext);
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

    bufW = HeapAlloc(GetProcessHeap(), 0, nBufferMax*sizeof(WCHAR));
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
    HeapFree(GetProcessHeap(), 0, bufW);
    return r;
}

INSTALLSTATE WINAPI MsiLocateComponentA(LPCSTR szComponent, LPSTR lpPathBuf,
                DWORD *pcchBuf)
{
    FIXME("%s %p %08lx\n", debugstr_a(szComponent), lpPathBuf, *pcchBuf);
    return INSTALLSTATE_UNKNOWN;
}

INSTALLSTATE WINAPI MsiLocateComponentW(LPCWSTR szComponent, LPSTR lpPathBuf,
                DWORD *pcchBuf)
{
    FIXME("%s %p %08lx\n", debugstr_w(szComponent), lpPathBuf, *pcchBuf);
    return INSTALLSTATE_UNKNOWN;
}

UINT WINAPI MsiMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType,
                WORD wLanguageId, DWORD f)
{
    FIXME("%p %s %s %u %08x %08lx\n",hWnd,debugstr_a(lpText),debugstr_a(lpCaption),
          uType,wLanguageId,f);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType,
                WORD wLanguageId, DWORD f)
{
    FIXME("%p %s %s %u %08x %08lx\n",hWnd,debugstr_w(lpText),debugstr_w(lpCaption),
          uType,wLanguageId,f);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiProvideAssemblyA( LPCSTR szAssemblyName, LPCSTR szAppContext,
                DWORD dwInstallMode, DWORD dwAssemblyInfo, LPSTR lpPathBuf,
                DWORD* pcchPathBuf )
{
    FIXME("%s %s %08lx %08lx %p %p\n", debugstr_a(szAssemblyName),
          debugstr_a(szAppContext), dwInstallMode, dwAssemblyInfo, lpPathBuf,
          pcchPathBuf);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiProvideAssemblyW( LPCWSTR szAssemblyName, LPCWSTR szAppContext,
                DWORD dwInstallMode, DWORD dwAssemblyInfo, LPWSTR lpPathBuf,
                DWORD* pcchPathBuf )
{
    FIXME("%s %s %08lx %08lx %p %p\n", debugstr_w(szAssemblyName),
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
    FIXME("%s %08lx %p %p %p\n", debugstr_a(szSignedObjectPath), dwFlags,
          ppcCertContext, pbHashData, pcbHashData);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

HRESULT WINAPI MsiGetFileSignatureInformationW( LPCWSTR szSignedObjectPath,
                DWORD dwFlags, PCCERT_CONTEXT* ppcCertContext, BYTE* pbHashData,
                DWORD* pcbHashData)
{
    FIXME("%s %08lx %p %p %p\n", debugstr_w(szSignedObjectPath), dwFlags,
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

    HeapFree( GetProcessHeap(), 0, szPack );

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

INSTALLSTATE WINAPI MsiGetComponentPathA(LPCSTR szProduct, LPCSTR szComponent,
                                         LPSTR lpPathBuf, DWORD* pcchBuf)
{
    LPWSTR szwProduct = NULL, szwComponent = NULL, lpwPathBuf= NULL;
    INSTALLSTATE rc;
    UINT incoming_len;

    if( szProduct )
    {
        szwProduct = strdupAtoW( szProduct );
        if( !szwProduct)
            return ERROR_OUTOFMEMORY;
    }

    if( szComponent )
    {
        szwComponent = strdupAtoW( szComponent );
        if( !szwComponent )
        {
            HeapFree( GetProcessHeap(), 0, szwProduct);
            return ERROR_OUTOFMEMORY;
        }
    }

    if( pcchBuf && *pcchBuf > 0 )
        lpwPathBuf = HeapAlloc( GetProcessHeap(), 0, *pcchBuf * sizeof(WCHAR));
    else
        lpwPathBuf = NULL;

    incoming_len = *pcchBuf;
    rc = MsiGetComponentPathW(szwProduct, szwComponent, lpwPathBuf, pcchBuf);

    HeapFree( GetProcessHeap(), 0, szwProduct);
    HeapFree( GetProcessHeap(), 0, szwComponent);
    if (lpwPathBuf)
    {
        if (rc != INSTALLSTATE_UNKNOWN)
            WideCharToMultiByte(CP_ACP, 0, lpwPathBuf, incoming_len,
                            lpPathBuf, incoming_len, NULL, NULL);
        HeapFree( GetProcessHeap(), 0, lpwPathBuf);
    }

    return rc;
}

INSTALLSTATE WINAPI MsiGetComponentPathW(LPCWSTR szProduct, LPCWSTR szComponent,
                                         LPWSTR lpPathBuf, DWORD* pcchBuf)
{
    WCHAR squished_pc[GUID_SIZE];
    UINT rc;
    INSTALLSTATE rrc = INSTALLSTATE_UNKNOWN;
    HKEY hkey = 0;
    LPWSTR path = NULL;
    DWORD sz, type;

    TRACE("%s %s %p %p\n", debugstr_w(szProduct),
           debugstr_w(szComponent), lpPathBuf, pcchBuf);

    if( lpPathBuf && !pcchBuf )
        return INSTALLSTATE_INVALIDARG;

    squash_guid(szProduct,squished_pc);

    rc = MSIREG_OpenProductsKey( szProduct, &hkey, FALSE);
    if( rc != ERROR_SUCCESS )
        goto end;

    RegCloseKey(hkey);

    rc = MSIREG_OpenComponentsKey( szComponent, &hkey, FALSE);
    if( rc != ERROR_SUCCESS )
        goto end;

    sz = 0;
    type = 0;
    rc = RegQueryValueExW( hkey, squished_pc, NULL, &type, NULL, &sz );
    if( rc != ERROR_SUCCESS )
        goto end;
    if( type != REG_SZ )
        goto end;

    sz += sizeof(WCHAR);
    path = HeapAlloc( GetProcessHeap(), 0, sz );
    if( !path )
        goto end;

    rc = RegQueryValueExW( hkey, squished_pc, NULL, NULL, (LPVOID) path, &sz );
    if( rc != ERROR_SUCCESS )
        goto end;

    TRACE("found path of (%s:%s)(%s)\n", debugstr_w(szComponent),
           debugstr_w(szProduct), debugstr_w(path));

    if (path[0]=='0')
    {
        FIXME("Registry entry.. check entry\n");
        rrc = INSTALLSTATE_LOCAL;
    }
    else
    {
        /* PROBABLY a file */
        if ( GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES )
            rrc = INSTALLSTATE_LOCAL;
        else
            rrc = INSTALLSTATE_ABSENT;
    }

    if( pcchBuf )
    {
        sz = sz / sizeof(WCHAR);
        if( *pcchBuf >= sz )
            lstrcpyW( lpPathBuf, path );
        *pcchBuf = sz;
    }

end:
    HeapFree(GetProcessHeap(), 0, path );
    RegCloseKey(hkey);
    return rrc;
}

/******************************************************************
 * MsiQueryFeatureStateA      [MSI.@]
 */
INSTALLSTATE WINAPI MsiQueryFeatureStateA(LPCSTR szProduct, LPCSTR szFeature)
{
    INSTALLSTATE rc;
    LPWSTR szwProduct= NULL;
    LPWSTR szwFeature= NULL;

    if( szProduct )
    {
        szwProduct = strdupAtoW( szProduct );
        if( !szwProduct)
            return ERROR_OUTOFMEMORY;
    }

    if( szFeature )
    {
        szwFeature = strdupAtoW( szFeature );
        if( !szwFeature)
        {
            HeapFree( GetProcessHeap(), 0, szwProduct);
            return ERROR_OUTOFMEMORY;
        }
    }

    rc = MsiQueryFeatureStateW(szwProduct, szwFeature);

    HeapFree( GetProcessHeap(), 0, szwProduct);
    HeapFree( GetProcessHeap(), 0, szwFeature);

    return rc;
}

/******************************************************************
 * MsiQueryFeatureStateW      [MSI.@]
 *
 * This does not verify that the Feature is functional. So i am only going to
 * check the existence of the key in the registry. This should tell me if it is
 * installed.
 */
INSTALLSTATE WINAPI MsiQueryFeatureStateW(LPCWSTR szProduct, LPCWSTR szFeature)
{
    UINT rc;
    DWORD sz = 0;
    HKEY hkey;

    TRACE("%s %s\n", debugstr_w(szProduct), debugstr_w(szFeature));

    rc = MSIREG_OpenFeaturesKey(szProduct, &hkey, FALSE);
    if (rc != ERROR_SUCCESS)
        return INSTALLSTATE_UNKNOWN;

    rc = RegQueryValueExW( hkey, szFeature, NULL, NULL, NULL, &sz);
    RegCloseKey(hkey);

    if (rc == ERROR_SUCCESS)
        return INSTALLSTATE_LOCAL;
    else
        return INSTALLSTATE_ABSENT;
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
        lpwVersionBuff = HeapAlloc(GetProcessHeap(), 0, *pcchVersionBuf*sizeof(WCHAR));
        if( !lpwVersionBuff )
            goto end;
    }

    if( lpLangBuf && pcchLangBuf && *pcchLangBuf )
    {
        lpwLangBuff = HeapAlloc(GetProcessHeap(), 0, *pcchVersionBuf*sizeof(WCHAR));
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
    HeapFree(GetProcessHeap(), 0, szwFilePath);
    HeapFree(GetProcessHeap(), 0, lpwVersionBuff);
    HeapFree(GetProcessHeap(), 0, lpwLangBuff);

    return ret;
}

/******************************************************************
 * MsiGetFileVersionW         [MSI.@]
 */
UINT WINAPI MsiGetFileVersionW(LPCWSTR szFilePath, LPWSTR lpVersionBuf,
                DWORD* pcchVersionBuf, LPWSTR lpLangBuf, DWORD* pcchLangBuf)
{
    static const WCHAR szVersionResource[] = {'\\',0};
    static const WCHAR szVersionFormat[] = {
        '%','d','.','%','d','.','%','d','.','%','d',0};
    static const WCHAR szLangFormat[] = {'%','d',0};
    UINT ret = 0;
    DWORD dwVerLen;
    LPVOID lpVer = NULL;
    VS_FIXEDFILEINFO *ffi;
    UINT puLen;
    WCHAR tmp[32];

    TRACE("%s %p %ld %p %ld\n", debugstr_w(szFilePath),
          lpVersionBuf, pcchVersionBuf?*pcchVersionBuf:0,
          lpLangBuf, pcchLangBuf?*pcchLangBuf:0);

    dwVerLen = GetFileVersionInfoSizeW(szFilePath, NULL);
    if( !dwVerLen )
        return GetLastError();

    lpVer = HeapAlloc(GetProcessHeap(), 0, dwVerLen);
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
    HeapFree(GetProcessHeap(), 0, lpVer);
    return ret;
}


/******************************************************************
 *    	DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch(fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        msi_hInstance = hinstDLL;
        DisableThreadLibraryCalls(hinstDLL);
        msi_dialog_register_class();
        break;
    case DLL_PROCESS_DETACH:
        msi_dialog_unregister_class();
        /* FIXME: Cleanup */
        break;
    }
    return TRUE;
}

typedef struct tagIClassFactoryImpl
{
    IClassFactoryVtbl *lpVtbl;
} IClassFactoryImpl;

static HRESULT WINAPI MsiCF_QueryInterface(LPCLASSFACTORY iface,
                REFIID riid,LPVOID *ppobj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
    FIXME("%p %s %p\n",This,debugstr_guid(riid),ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI MsiCF_AddRef(LPCLASSFACTORY iface)
{
    return 2;
}

static ULONG WINAPI MsiCF_Release(LPCLASSFACTORY iface)
{
    return 1;
}

static HRESULT WINAPI MsiCF_CreateInstance(LPCLASSFACTORY iface,
    LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

    FIXME("%p %p %s %p\n", This, pOuter, debugstr_guid(riid), ppobj);
    return E_FAIL;
}

static HRESULT WINAPI MsiCF_LockServer(LPCLASSFACTORY iface, BOOL dolock)
{
    IClassFactoryImpl *This = (IClassFactoryImpl *)iface;

    FIXME("%p %d\n", This, dolock);
    return S_OK;
}

static IClassFactoryVtbl MsiCF_Vtbl =
{
    MsiCF_QueryInterface,
    MsiCF_AddRef,
    MsiCF_Release,
    MsiCF_CreateInstance,
    MsiCF_LockServer
};

static IClassFactoryImpl Msi_CF = { &MsiCF_Vtbl };

/******************************************************************
 * DllGetClassObject          [MSI.@]
 */
HRESULT WINAPI MSI_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    TRACE("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if( IsEqualCLSID (rclsid, &CLSID_IMsiServer) ||
        IsEqualCLSID (rclsid, &CLSID_IMsiServerMessage) ||
        IsEqualCLSID (rclsid, &CLSID_IMsiServerX1) ||
        IsEqualCLSID (rclsid, &CLSID_IMsiServerX2) ||
        IsEqualCLSID (rclsid, &CLSID_IMsiServerX3) )
    {
        *ppv = (LPVOID) &Msi_CF;
        return S_OK;
    }
    return CLASS_E_CLASSNOTAVAILABLE;
}

/******************************************************************
 * DllGetVersion              [MSI.@]
 */
HRESULT WINAPI MSI_DllGetVersion(DLLVERSIONINFO *pdvi)
{
    TRACE("%p\n",pdvi);

    if (pdvi->cbSize != sizeof(DLLVERSIONINFO))
        return E_INVALIDARG;

    pdvi->dwMajorVersion = MSI_MAJORVERSION;
    pdvi->dwMinorVersion = MSI_MINORVERSION;
    pdvi->dwBuildNumber = MSI_BUILDNUMBER;
    pdvi->dwPlatformID = 1;

    return S_OK;
}

/******************************************************************
 * DllCanUnloadNow            [MSI.@]
 */
BOOL WINAPI MSI_DllCanUnloadNow(void)
{
    return S_FALSE;
}

UINT WINAPI MsiGetFeatureUsageW(LPCWSTR szProduct, LPCWSTR szFeature,
                                DWORD* pdwUseCount, WORD* pwDateUsed)
{
    FIXME("%s %s %p %p\n",debugstr_w(szProduct), debugstr_w(szFeature),
          pdwUseCount, pwDateUsed);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiGetFeatureUsageA(LPCSTR szProduct, LPCSTR szFeature,
                                DWORD* pdwUseCount, WORD* pwDateUsed)
{
    FIXME("%s %s %p %p\n", debugstr_a(szProduct), debugstr_a(szFeature),
          pdwUseCount, pwDateUsed);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

INSTALLSTATE WINAPI MsiUseFeatureExW(LPCWSTR szProduct, LPCWSTR szFeature,
                             DWORD dwInstallMode, DWORD dwReserved)
{
    FIXME("%s %s %li %li\n", debugstr_w(szProduct), debugstr_w(szFeature),
          dwInstallMode, dwReserved);

    /*
     * Polls all the components of the feature to find install state and then
     *  writes:
     *    Software\\Microsoft\\Windows\\CurrentVersion\\
     *    Installer\\Products\\<squishguid>\\<feature>
     *      "Usage"=dword:........
     */

    return INSTALLSTATE_LOCAL;
}

/***********************************************************************
 * MsiUseFeatureExA           [MSI.@]
 */
INSTALLSTATE WINAPI MsiUseFeatureExA(LPCSTR szProduct, LPCSTR szFeature,
                             DWORD dwInstallMode, DWORD dwReserved)
{
    FIXME("%s %s %li %li\n", debugstr_a(szProduct), debugstr_a(szFeature),
          dwInstallMode, dwReserved);

    return INSTALLSTATE_LOCAL;
}

INSTALLSTATE WINAPI MsiUseFeatureW(LPCWSTR szProduct, LPCWSTR szFeature)
{
    FIXME("%s %s\n", debugstr_w(szProduct), debugstr_w(szFeature));

    return INSTALLSTATE_LOCAL;
}

INSTALLSTATE WINAPI MsiUseFeatureA(LPCSTR szProduct, LPCSTR szFeature)
{
    FIXME("%s %s\n", debugstr_a(szProduct), debugstr_a(szFeature));

    return INSTALLSTATE_LOCAL;
}

UINT WINAPI MsiProvideQualifiedComponentExW(LPCWSTR szComponent,
                LPCWSTR szQualifier, DWORD dwInstallMode, LPWSTR szProduct,
                DWORD Unused1, DWORD Unused2, LPWSTR lpPathBuf,
                DWORD* pcchPathBuf)
{
    HKEY hkey;
    UINT rc;
    LPWSTR info;
    DWORD sz;
    LPWSTR product = NULL;
    LPWSTR component = NULL;
    LPWSTR ptr;
    GUID clsid;

    TRACE("%s %s %li %s %li %li %p %p\n", debugstr_w(szComponent),
          debugstr_w(szQualifier), dwInstallMode, debugstr_w(szProduct),
          Unused1, Unused2, lpPathBuf, pcchPathBuf);
   
    rc = MSIREG_OpenUserComponentsKey(szComponent, &hkey, FALSE);
    if (rc != ERROR_SUCCESS)
        return ERROR_INDEX_ABSENT;

    sz = 0;
    rc = RegQueryValueExW( hkey, szQualifier, NULL, NULL, NULL, &sz);
    if (sz <= 0)
    {
        RegCloseKey(hkey);
        return ERROR_INDEX_ABSENT;
    }

    info = HeapAlloc(GetProcessHeap(),0,sz);
    rc = RegQueryValueExW( hkey, szQualifier, NULL, NULL, (LPBYTE)info, &sz);
    if (rc != ERROR_SUCCESS)
    {
        RegCloseKey(hkey);
        HeapFree(GetProcessHeap(),0,info);
        return ERROR_INDEX_ABSENT;
    }

    /* find the component */
    ptr = strchrW(&info[20],'>');
    if (ptr)
        ptr++;
    else
    {
        RegCloseKey(hkey);
        HeapFree(GetProcessHeap(),0,info);
        return ERROR_INDEX_ABSENT;
    }

    if (!szProduct)
    {
        decode_base85_guid(info,&clsid);
        StringFromCLSID(&clsid, &product);
    }
    decode_base85_guid(ptr,&clsid);
    StringFromCLSID(&clsid, &component);

    if (!szProduct)
        rc = MsiGetComponentPathW(product, component, lpPathBuf, pcchPathBuf);
    else
        rc = MsiGetComponentPathW(szProduct, component, lpPathBuf, pcchPathBuf);
   
    RegCloseKey(hkey);
    HeapFree(GetProcessHeap(),0,info);
    HeapFree(GetProcessHeap(),0,product);
    HeapFree(GetProcessHeap(),0,component);

    if (rc == INSTALLSTATE_LOCAL)
        return ERROR_SUCCESS;
    else 
        return ERROR_FILE_NOT_FOUND;
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
    LPWSTR szwComponent, szwQualifier, lpwPathBuf;
    DWORD pcchwPathBuf;
    UINT rc;

    TRACE("%s %s %li %p %p\n",szComponent, szQualifier,
                    dwInstallMode, lpPathBuf, pcchPathBuf);

    szwComponent= strdupAtoW( szComponent);
    szwQualifier= strdupAtoW( szQualifier);

    lpwPathBuf = HeapAlloc(GetProcessHeap(),0,*pcchPathBuf * sizeof(WCHAR));

    pcchwPathBuf = *pcchPathBuf;

    rc = MsiProvideQualifiedComponentW(szwComponent, szwQualifier, 
                    dwInstallMode, lpwPathBuf, &pcchwPathBuf);

    HeapFree(GetProcessHeap(),0,szwComponent);
    HeapFree(GetProcessHeap(),0,szwQualifier);
    *pcchPathBuf = WideCharToMultiByte(CP_ACP, 0, lpwPathBuf, pcchwPathBuf,
                    lpPathBuf, *pcchPathBuf, NULL, NULL);

    HeapFree(GetProcessHeap(),0,lpwPathBuf);
    return rc;
}

USERINFOSTATE WINAPI MsiGetUserInfoW(LPCWSTR szProduct, LPWSTR lpUserNameBuf,
                DWORD* pcchUserNameBuf, LPWSTR lpOrgNameBuf,
                DWORD* pcchOrgNameBuf, LPWSTR lpSerialBuf, DWORD* pcchSerialBuf)
{
    HKEY hkey;
    DWORD sz;
    UINT rc = ERROR_SUCCESS,rc2 = ERROR_SUCCESS;
    static const WCHAR szOwner[] = {'R','e','g','O','w','n','e','r',0};
    static const WCHAR szCompany[] = {'R','e','g','C','o','m','p','a','n','y',0};
    static const WCHAR szSerial[] = {'P','r','o','d','u','c','t','I','D',0};

    TRACE("%s %p %p %p %p %p %p\n",debugstr_w(szProduct), lpUserNameBuf,
          pcchUserNameBuf, lpOrgNameBuf, pcchOrgNameBuf, lpSerialBuf,
          pcchSerialBuf);

    rc = MSIREG_OpenUninstallKey(szProduct, &hkey, FALSE);
    if (rc != ERROR_SUCCESS)
        return USERINFOSTATE_UNKNOWN;

    if (lpUserNameBuf)
    {
        sz = *lpUserNameBuf * sizeof(WCHAR);
        rc = RegQueryValueExW( hkey, szOwner, NULL, NULL, (LPBYTE)lpUserNameBuf,
                               &sz);
    }
    if (!lpUserNameBuf && pcchUserNameBuf)
    {
        sz = 0;
        rc = RegQueryValueExW( hkey, szOwner, NULL, NULL, NULL, &sz);
    }

    if (pcchUserNameBuf)
        *pcchUserNameBuf = sz / sizeof(WCHAR);

    if (lpOrgNameBuf)
    {
        sz = *pcchOrgNameBuf * sizeof(WCHAR);
        rc2 = RegQueryValueExW( hkey, szCompany, NULL, NULL, 
                               (LPBYTE)lpOrgNameBuf, &sz);
    }
    if (!lpOrgNameBuf && pcchOrgNameBuf)
    {
        sz = 0;
        rc2 = RegQueryValueExW( hkey, szCompany, NULL, NULL, NULL, &sz);
    }

    if (pcchOrgNameBuf)
        *pcchOrgNameBuf = sz / sizeof(WCHAR);

    if (rc != ERROR_SUCCESS && rc != ERROR_MORE_DATA && 
        rc2 != ERROR_SUCCESS && rc2 != ERROR_MORE_DATA)
    {
        RegCloseKey(hkey);
        return USERINFOSTATE_ABSENT;
    }

    if (lpSerialBuf)
    {
        sz = *pcchSerialBuf * sizeof(WCHAR);
        RegQueryValueExW( hkey, szSerial, NULL, NULL, (LPBYTE)lpSerialBuf,
                               &sz);
    }
    if (!lpSerialBuf && pcchSerialBuf)
    {
        sz = 0;
        rc = RegQueryValueExW( hkey, szSerial, NULL, NULL, NULL, &sz);
    }
    if (pcchSerialBuf)
        *pcchSerialBuf = sz / sizeof(WCHAR);
    
    RegCloseKey(hkey);
    return USERINFOSTATE_PRESENT;
}

USERINFOSTATE WINAPI MsiGetUserInfoA(LPCSTR szProduct, LPSTR lpUserNameBuf,
                DWORD* pcchUserNameBuf, LPSTR lpOrgNameBuf,
                DWORD* pcchOrgNameBuf, LPSTR lpSerialBuf, DWORD* pcchSerialBuf)
{
    FIXME("%s %p %p %p %p %p %p\n",debugstr_a(szProduct), lpUserNameBuf,
          pcchUserNameBuf, lpOrgNameBuf, pcchOrgNameBuf, lpSerialBuf,
          pcchSerialBuf);

    return USERINFOSTATE_UNKNOWN;
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

UINT WINAPI MsiCreateAndVerifyInstallerDirectory(DWORD dwReserved)
{
    WCHAR path[MAX_PATH];

    if(dwReserved) {
        FIXME("Don't know how to handle argument %ld\n", dwReserved);
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

   if(!GetWindowsDirectoryW(path, MAX_PATH)) {
        FIXME("GetWindowsDirectory failed unexpected! Error %ld\n",
              GetLastError());
        return ERROR_CALL_NOT_IMPLEMENTED;
   }

   strcatW(path, installerW);

   CreateDirectoryW(path, NULL);

   return 0;
}

UINT WINAPI MsiGetShortcutTargetA( LPCSTR szShortcutTarget,
                                   LPSTR szProductCode, LPSTR szFeatureId,
                                   LPSTR szComponentCode )
{
    FIXME("\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiGetShortcutTargetW( LPCWSTR szShortcutTarget,
                                   LPWSTR szProductCode, LPWSTR szFeatureId,
                                   LPWSTR szComponentCode )
{
    FIXME("\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiReinstallFeatureW( LPCWSTR szProduct, LPCWSTR szFeature,
                                  DWORD dwReinstallMode )
{
    FIXME("%s %s %li\n", debugstr_w(szProduct), debugstr_w(szFeature),
                           dwReinstallMode);
    return ERROR_SUCCESS;
}

UINT WINAPI MsiReinstallFeatureA( LPCSTR szProduct, LPCSTR szFeature,
                                  DWORD dwReinstallMode )
{
    FIXME("%s %s %li\n", debugstr_a(szProduct), debugstr_a(szFeature),
                           dwReinstallMode);
    return ERROR_SUCCESS;
}
