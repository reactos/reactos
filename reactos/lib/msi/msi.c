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
#include "objidl.h"
#include "wincrypt.h"
#include "wine/unicode.h"
#include "objbase.h"
#include "winver.h"
#include "winuser.h"

#include "initguid.h"

UINT WINAPI MsiGetFileVersionW(LPCWSTR szFilePath, LPWSTR lpVersionBuf, DWORD* pcchVersionBuf, LPWSTR lpLangBuf, DWORD* pcchLangBuf);


WINE_DEFAULT_DEBUG_CHANNEL(msi);

/*
 * The MSVC headers define the MSIDBOPEN_* macros cast to LPCTSTR,
 *  which is a problem because LPCTSTR isn't defined when compiling wine.
 * To work around this problem, we need to define LPCTSTR as LPCWSTR here,
 *  and make sure to only use it in W functions.
 */
#define LPCTSTR LPCWSTR

DEFINE_GUID( CLSID_MsiDatabase, 0x000c1084, 0x0000, 0x0000,
             0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);

/* the UI level */
INSTALLUILEVEL gUILevel = INSTALLUILEVEL_BASIC;
HWND           gUIhwnd = 0;
INSTALLUI_HANDLERA gUIHandlerA = NULL;
INSTALLUI_HANDLERW gUIHandlerW = NULL;
DWORD gUIFilter = 0;
LPVOID gUIContext = NULL;
WCHAR gszLogFile[MAX_PATH];
HINSTANCE msi_hInstance;

/*
 *  .MSI  file format
 *
 *  A .msi file is a structured storage file.
 *  It should contain a number of streams.
 */

VOID MSI_CloseDatabase( MSIOBJECTHDR *arg )
{
    MSIDATABASE *db = (MSIDATABASE *) arg;
    DWORD r;

    free_cached_tables( db );
    r = IStorage_Release( db->storage );
    if( r )
        ERR("database reference count was not zero (%ld)\n", r);
}

UINT MSI_OpenDatabaseW(LPCWSTR szDBPath, LPCWSTR szPersist, MSIDATABASE **pdb)
{
    IStorage *stg = NULL;
    HRESULT r;
    MSIDATABASE *db = NULL;
    UINT ret = ERROR_FUNCTION_FAILED;
    LPWSTR szMode;
    STATSTG stat;

    TRACE("%s %s\n",debugstr_w(szDBPath),debugstr_w(szPersist) );

    if( !pdb )
        return ERROR_INVALID_PARAMETER;

    szMode = (LPWSTR) szPersist;
    if( HIWORD( szPersist ) )
    {
        /* UINT len = lstrlenW( szPerist ) + 1; */
        FIXME("don't support persist files yet\b");
        return ERROR_INVALID_PARAMETER;
        /* szMode = HeapAlloc( GetProcessHeap(), 0, len * sizeof (DWORD) ); */
    }
    else if( szPersist == MSIDBOPEN_READONLY )
    {
        r = StgOpenStorage( szDBPath, NULL,
              STGM_DIRECT|STGM_READ|STGM_SHARE_DENY_WRITE, NULL, 0, &stg);
    }
    else if( szPersist == MSIDBOPEN_CREATE )
    {
        r = StgCreateDocfile( szDBPath, 
              STGM_DIRECT|STGM_READWRITE|STGM_SHARE_EXCLUSIVE, 0, &stg);
        if( r == ERROR_SUCCESS )
        {
            IStorage_SetClass( stg, &CLSID_MsiDatabase );
            r = init_string_table( stg );
        }
    }
    else if( szPersist == MSIDBOPEN_TRANSACT )
    {
        r = StgOpenStorage( szDBPath, NULL,
              STGM_DIRECT|STGM_READWRITE|STGM_SHARE_EXCLUSIVE, NULL, 0, &stg);
    }
    else
    {
        ERR("unknown flag %p\n",szPersist);
        return ERROR_INVALID_PARAMETER;
    }

    if( FAILED( r ) )
    {
        FIXME("open failed r = %08lx!\n",r);
        return ERROR_FUNCTION_FAILED;
    }

    r = IStorage_Stat( stg, &stat, STATFLAG_NONAME );
    if( FAILED( r ) )
    {
        FIXME("Failed to stat storage\n");
        goto end;
    }

    if( memcmp( &stat.clsid, &CLSID_MsiDatabase, sizeof (GUID) ) )
    {
        ERR("storage GUID is not a MSI database GUID %s\n",
             debugstr_guid(&stat.clsid) );
        goto end;
    }


    db = alloc_msiobject( MSIHANDLETYPE_DATABASE, sizeof (MSIDATABASE),
                              MSI_CloseDatabase );
    if( !db )
    {
        FIXME("Failed to allocate a handle\n");
        goto end;
    }

    if( TRACE_ON( msi ) )
        enum_stream_names( stg );

    db->storage = stg;
    db->mode = szMode;

    ret = load_string_table( db );
    if( ret != ERROR_SUCCESS )
        goto end;

    msiobj_addref( &db->hdr );
    IStorage_AddRef( stg );
    *pdb = db;

end:
    if( db )
        msiobj_release( &db->hdr );
    if( stg )
        IStorage_Release( stg );

    return ret;
}

UINT WINAPI MsiOpenDatabaseW(LPCWSTR szDBPath, LPCWSTR szPersist, MSIHANDLE *phDB)
{
    MSIDATABASE *db;
    UINT ret;

    TRACE("%s %s %p\n",debugstr_w(szDBPath),debugstr_w(szPersist), phDB);

    ret = MSI_OpenDatabaseW( szDBPath, szPersist, &db );
    if( ret == ERROR_SUCCESS )
    {
        *phDB = alloc_msihandle( &db->hdr );
        msiobj_release( &db->hdr );
    }

    return ret;
}

UINT WINAPI MsiOpenDatabaseA(LPCSTR szDBPath, LPCSTR szPersist, MSIHANDLE *phDB)
{
    HRESULT r = ERROR_FUNCTION_FAILED;
    LPWSTR szwDBPath = NULL, szwPersist = NULL;
    UINT len;

    TRACE("%s %s %p\n", debugstr_a(szDBPath), debugstr_a(szPersist), phDB);

    if( szDBPath )
    {
        len = MultiByteToWideChar( CP_ACP, 0, szDBPath, -1, NULL, 0 );
        szwDBPath = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        if( !szwDBPath )
            goto end;
        MultiByteToWideChar( CP_ACP, 0, szDBPath, -1, szwDBPath, len );
    }

    if( HIWORD(szPersist) )
    {
        len = MultiByteToWideChar( CP_ACP, 0, szPersist, -1, NULL, 0 );
        szwPersist = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        if( !szwPersist )
            goto end;
        MultiByteToWideChar( CP_ACP, 0, szPersist, -1, szwPersist, len );
    }
    else
        szwPersist = (LPWSTR) szPersist;

    r = MsiOpenDatabaseW( szwDBPath, szwPersist, phDB );

end:
    HeapFree( GetProcessHeap(), 0, szwPersist );
    HeapFree( GetProcessHeap(), 0, szwDBPath );

    return r;
}

UINT WINAPI MsiOpenProductA(LPCSTR szProduct, MSIHANDLE *phProduct)
{
    UINT len, ret;
    LPWSTR szwProd = NULL;

    TRACE("%s %p\n",debugstr_a(szProduct), phProduct);

    if( szProduct )
    {
        len = MultiByteToWideChar( CP_ACP, 0, szProduct, -1, NULL, 0 );
        szwProd = HeapAlloc( GetProcessHeap(), 0, len * sizeof (WCHAR) );
        if( szwProd )
            MultiByteToWideChar( CP_ACP, 0, szProduct, -1, szwProd, len );
    }

    ret = MsiOpenProductW( szwProd, phProduct );

    HeapFree( GetProcessHeap(), 0, szwProd );

    return ret;
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
    UINT r = ERROR_FUNCTION_FAILED; /* FIXME: check return code */

    TRACE("%s %s\n",debugstr_a(szPackagePath), debugstr_a(szCommandLine));

    if( szPackagePath )
    {
        UINT len = MultiByteToWideChar( CP_ACP, 0, szPackagePath, -1, NULL, 0 );
        szwPath = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        if( !szwPath )
            goto end;
        MultiByteToWideChar( CP_ACP, 0, szPackagePath, -1, szwPath, len );
    }

    if( szCommandLine )
    {
        UINT len = MultiByteToWideChar( CP_ACP, 0, szCommandLine, -1, NULL, 0 );
        szwCommand = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        if( !szwCommand )
            goto end;
        MultiByteToWideChar( CP_ACP, 0, szCommandLine, -1, szwCommand, len );
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
    UINT rc = ERROR_SUCCESS; 
    MSIHANDLE handle;

    FIXME("%s %s\n",debugstr_w(szPackagePath), debugstr_w(szCommandLine));

    rc = MsiVerifyPackageW(szPackagePath);
    if (rc != ERROR_SUCCESS)
        return rc;

    rc = MSI_OpenPackageW(szPackagePath,&package);
    if (rc != ERROR_SUCCESS)
        return rc;

    handle = alloc_msihandle( &package->hdr );

    rc = ACTION_DoTopLevelINSTALL(package, szPackagePath, szCommandLine);

    MsiCloseHandle(handle);
    msiobj_release( &package->hdr );
    return rc;
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
    MSIHANDLE handle; 
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
  
    sz = strlenW(szInstalled);

    if (szCommandLine)
        sz += strlenW(szCommandLine);

    commandline = HeapAlloc(GetProcessHeap(),0,sz * sizeof(WCHAR));

    if (szCommandLine) 
        strcpyW(commandline,szCommandLine);
    else
        commandline[0] = 0;

    if (MsiQueryProductStateW(szProduct) != INSTALLSTATE_UNKNOWN)
        strcatW(commandline,szInstalled);

    rc = ACTION_DoTopLevelINSTALL(package, sourcepath, commandline);

    msiobj_release( &package->hdr );

    HeapFree(GetProcessHeap(),0,commandline);
end:
    RegCloseKey(hkey);

    return rc;
}

UINT WINAPI MsiConfigureProductExA(LPCSTR szProduct, int iInstallLevel,
                        INSTALLSTATE eInstallState, LPCSTR szCommandLine)
{
    LPWSTR szwProduct = NULL;
    LPWSTR szwCommandLine = NULL;
    UINT hr = ERROR_FUNCTION_FAILED;

    if( szProduct )
    {
        UINT len = MultiByteToWideChar( CP_ACP, 0, szProduct, -1, NULL, 0 );
        szwProduct = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        if( !szwProduct )
            goto end;
        MultiByteToWideChar( CP_ACP, 0, szProduct, -1, szwProduct, len );
    }

    if( szCommandLine)
    {
        UINT len = MultiByteToWideChar( CP_ACP, 0, szCommandLine, -1, NULL, 0 );
        szwCommandLine= HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        if( !szwCommandLine)
            goto end;
        MultiByteToWideChar( CP_ACP, 0, szCommandLine, -1, szwCommandLine, len );
    }

    hr = MsiConfigureProductExW( szwProduct, iInstallLevel, eInstallState,
                                szwCommandLine );
end:
    HeapFree( GetProcessHeap(), 0, szwProduct );
    HeapFree( GetProcessHeap(), 0, szwCommandLine);

    return hr;
}

UINT WINAPI MsiConfigureProductA(LPCSTR szProduct, int iInstallLevel, 
                                 INSTALLSTATE eInstallState)
{
    LPWSTR szwProduct = NULL;
    UINT hr = ERROR_SUCCESS;

    FIXME("%s %d %d\n",debugstr_a(szProduct), iInstallLevel, eInstallState);

    if( szProduct )
    {
        UINT len = MultiByteToWideChar( CP_ACP, 0, szProduct, -1, NULL, 0 );
        szwProduct = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        if( !szwProduct )
            goto end;
        MultiByteToWideChar( CP_ACP, 0, szProduct, -1, szwProduct, len );
    }

    hr = MsiConfigureProductW( szwProduct, iInstallLevel, eInstallState );

end:
    HeapFree( GetProcessHeap(), 0, szwProduct );

    return hr;
}

UINT WINAPI MsiConfigureProductW(LPCWSTR szProduct, int iInstallLevel, 
                                 INSTALLSTATE eInstallState)
{
    FIXME("%s %d %d\n", debugstr_w(szProduct), iInstallLevel, eInstallState);

    return MsiConfigureProductExW(szProduct, iInstallLevel, eInstallState,
                                  NULL);
}

UINT WINAPI MsiGetProductCodeA(LPCSTR szComponent, LPSTR szBuffer)
{
    LPWSTR szwComponent = NULL;
    UINT hr = ERROR_INSTALL_FAILURE;
    WCHAR szwBuffer[GUID_SIZE];

    FIXME("%s %s\n",debugstr_a(szComponent), debugstr_a(szBuffer));

    if( szComponent )
    {
        UINT len = MultiByteToWideChar( CP_ACP, 0, szComponent, -1, NULL, 0 );
        szwComponent = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        if( !szwComponent )
            goto end;
        MultiByteToWideChar( CP_ACP, 0, szComponent, -1, szwComponent, len );
    }
    else
        return ERROR_INVALID_PARAMETER;

    hr = MsiGetProductCodeW( szwComponent, szwBuffer );

    if( ERROR_SUCCESS == hr )
        WideCharToMultiByte(CP_ACP, 0, szwBuffer, -1, szBuffer, GUID_SIZE, NULL, NULL);

end:
    HeapFree( GetProcessHeap(), 0, szwComponent );

    return hr;
}

UINT WINAPI MsiGetProductCodeW(LPCWSTR szComponent, LPWSTR szBuffer)
{
    FIXME("%s %s\n",debugstr_w(szComponent), debugstr_w(szBuffer));
    if (NULL == szComponent)
        return ERROR_INVALID_PARAMETER;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiGetProductInfoA(LPCSTR szProduct, LPCSTR szAttribute,
                 LPSTR szBuffer, DWORD *pcchValueBuf)
{
    LPWSTR szwProduct = NULL, szwAttribute = NULL, szwBuffer = NULL;
    UINT hr = ERROR_INSTALL_FAILURE;

    FIXME("%s %s %p %p\n",debugstr_a(szProduct), debugstr_a(szAttribute),
          szBuffer, pcchValueBuf);

    if( NULL != szBuffer && NULL == pcchValueBuf )
        return ERROR_INVALID_PARAMETER;
    if( szProduct )
    {
        UINT len = MultiByteToWideChar( CP_ACP, 0, szProduct, -1, NULL, 0 );
        szwProduct = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        if( !szwProduct )
            goto end;
        MultiByteToWideChar( CP_ACP, 0, szProduct, -1, szwProduct, len );
    }
    else
        return ERROR_INVALID_PARAMETER;
    
    if( szAttribute )
    {
        UINT len = MultiByteToWideChar( CP_ACP, 0, szAttribute, -1, NULL, 0 );
        szwAttribute = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        if( !szwAttribute )
            goto end;
        MultiByteToWideChar( CP_ACP, 0, szAttribute, -1, szwAttribute, len );
    }
    else
    {
        hr = ERROR_INVALID_PARAMETER;
        goto end;
    }

    if( szBuffer )
    {
        szwBuffer = HeapAlloc( GetProcessHeap(), 0, (*pcchValueBuf) * sizeof(WCHAR) );
        if( !szwBuffer )     
            goto end;
    }

    hr = MsiGetProductInfoW( szwProduct, szwAttribute, szwBuffer, pcchValueBuf );

    if( ERROR_SUCCESS == hr )
        WideCharToMultiByte(CP_ACP, 0, szwBuffer, -1, szBuffer, *pcchValueBuf, NULL, NULL);

end:
    HeapFree( GetProcessHeap(), 0, szwProduct );
    HeapFree( GetProcessHeap(), 0, szwAttribute );
    HeapFree( GetProcessHeap(), 0, szwBuffer );

    return hr;    
}

UINT WINAPI MsiGetProductInfoW(LPCWSTR szProduct, LPCWSTR szAttribute,
                LPWSTR szBuffer, DWORD *pcchValueBuf)
{
    MSIHANDLE hProduct;
    UINT hr;
    
    FIXME("%s %s %p %p\n",debugstr_w(szProduct), debugstr_w(szAttribute),
          szBuffer, pcchValueBuf);

    if (NULL != szBuffer && NULL == pcchValueBuf)
        return ERROR_INVALID_PARAMETER;
    if (NULL == szProduct || NULL == szAttribute)
        return ERROR_INVALID_PARAMETER;

    hr = MsiOpenProductW(szProduct, &hProduct);
    if (ERROR_SUCCESS != hr)
        return hr;

    hr = MsiGetPropertyW(hProduct, szAttribute, szBuffer, pcchValueBuf);
    MsiCloseHandle(hProduct);
    return hr;
}

UINT WINAPI MsiDatabaseImportA(LPCSTR szFolderPath, LPCSTR szFilename)
{
    FIXME("%s %s\n",debugstr_a(szFolderPath), debugstr_a(szFilename));
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiDatabaseImportW(LPCWSTR szFolderPath, LPCWSTR szFilename)
{
    FIXME("%s %s\n",debugstr_w(szFolderPath), debugstr_w(szFilename));
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiEnableLogA(DWORD dwLogMode, LPCSTR szLogFile, DWORD attributes)
{
    LPWSTR szwLogFile = NULL;
    UINT hr = ERROR_INSTALL_FAILURE;

    FIXME("%08lx %s %08lx\n", dwLogMode, debugstr_a(szLogFile), attributes);

    if( szLogFile )
    {
        UINT len = MultiByteToWideChar( CP_ACP, 0, szLogFile, -1, NULL, 0 );
        szwLogFile = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        if( !szwLogFile )
            goto end;
        MultiByteToWideChar( CP_ACP, 0, szLogFile, -1, szwLogFile, len );
    }
    else
        return ERROR_INVALID_PARAMETER;

    hr = MsiEnableLogW( dwLogMode, szwLogFile, attributes );

end:
    HeapFree( GetProcessHeap(), 0, szwLogFile );

    return hr;
}

UINT WINAPI MsiEnableLogW(DWORD dwLogMode, LPCWSTR szLogFile, DWORD attributes)
{
    HANDLE file = INVALID_HANDLE_VALUE;

    TRACE("%08lx %s %08lx\n", dwLogMode, debugstr_w(szLogFile), attributes);

    strcpyW(gszLogFile,szLogFile);
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
    LPWSTR szwProduct;
    UINT len;
    INSTALLSTATE rc;

    len = MultiByteToWideChar(CP_ACP,0,szProduct,-1,NULL,0);
    szwProduct = HeapAlloc(GetProcessHeap(),0,len*sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP,0,szProduct,-1,szwProduct,len);
    rc = MsiQueryProductStateW(szwProduct);
    HeapFree(GetProcessHeap(),0,szwProduct);
    return rc;
}

INSTALLSTATE WINAPI MsiQueryProductStateW(LPCWSTR szProduct)
{
    UINT rc;
    INSTALLSTATE rrc = INSTALLSTATE_UNKNOWN;
    HKEY hkey=0;
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
 *   lang          [I]  the prefered language for the string
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

UINT WINAPI MsiEnumProductsA(DWORD index, LPSTR lpguid)
{
    DWORD r;
    WCHAR szwGuid[GUID_SIZE];

    TRACE("%ld %p\n",index,lpguid);
    
    if (NULL == lpguid) {
      return ERROR_INVALID_PARAMETER;
    }
    r = MsiEnumProductsW(index, szwGuid);
    if( r == ERROR_SUCCESS )
        WideCharToMultiByte(CP_ACP, 0, szwGuid, -1, lpguid, GUID_SIZE, NULL, NULL);

    return r;
}

UINT WINAPI MsiEnumProductsW(DWORD index, LPWSTR lpguid)
{
    HKEY hkeyFeatures = 0;
    DWORD r;
    WCHAR szKeyName[33];

    TRACE("%ld %p\n",index,lpguid);

    if (NULL == lpguid)
        return ERROR_INVALID_PARAMETER;

    r = MSIREG_OpenFeatures(&hkeyFeatures);
    if( r != ERROR_SUCCESS )
        goto end;

    r = RegEnumKeyW(hkeyFeatures, index, szKeyName, GUID_SIZE);

    unsquash_guid(szKeyName, lpguid);

end:

    if( hkeyFeatures )
        RegCloseKey(hkeyFeatures);

    return r;
}

UINT WINAPI MsiEnumFeaturesA(LPCSTR szProduct, DWORD index, 
      LPSTR szFeature, LPSTR szParent)
{
    DWORD r;
    WCHAR szwFeature[GUID_SIZE], szwParent[GUID_SIZE];
    LPWSTR szwProduct = NULL;

    TRACE("%s %ld %p %p\n",debugstr_a(szProduct),index,szFeature,szParent);

    if( szProduct )
    {
        UINT len = MultiByteToWideChar( CP_ACP, 0, szProduct, -1, NULL, 0 );
        szwProduct = HeapAlloc( GetProcessHeap(), 0, len * sizeof (WCHAR) );
        if( szwProduct )
            MultiByteToWideChar( CP_ACP, 0, szProduct, -1, szwProduct, len );
        else
            return ERROR_FUNCTION_FAILED;
    }

    r = MsiEnumFeaturesW(szwProduct, index, szwFeature, szwParent);
    if( r == ERROR_SUCCESS )
    {
        WideCharToMultiByte(CP_ACP, 0, szwFeature, -1,
                            szFeature, GUID_SIZE, NULL, NULL);
        WideCharToMultiByte(CP_ACP, 0, szwParent, -1,
                            szParent, GUID_SIZE, NULL, NULL);
    }

    HeapFree( GetProcessHeap(), 0, szwProduct);

    return r;
}

UINT WINAPI MsiEnumFeaturesW(LPCWSTR szProduct, DWORD index, 
      LPWSTR szFeature, LPWSTR szParent)
{
    HKEY hkeyProduct = 0;
    DWORD r, sz;

    TRACE("%s %ld %p %p\n",debugstr_w(szProduct),index,szFeature,szParent);

    r = MSIREG_OpenFeaturesKey(szProduct,&hkeyProduct,FALSE);
    if( r != ERROR_SUCCESS )
        goto end;

    sz = GUID_SIZE;
    r = RegEnumValueW(hkeyProduct, index, szFeature, &sz, NULL, NULL, NULL, NULL);

end:
    if( hkeyProduct )
        RegCloseKey(hkeyProduct);

    return r;
}

UINT WINAPI MsiEnumComponentsA(DWORD index, LPSTR lpguid)
{
    DWORD r;
    WCHAR szwGuid[GUID_SIZE];

    TRACE("%ld %p\n",index,lpguid);

    r = MsiEnumComponentsW(index, szwGuid);
    if( r == ERROR_SUCCESS )
        WideCharToMultiByte(CP_ACP, 0, szwGuid, -1, lpguid, GUID_SIZE, NULL, NULL);

    return r;
}

UINT WINAPI MsiEnumComponentsW(DWORD index, LPWSTR lpguid)
{
    HKEY hkeyComponents = 0;
    DWORD r;
    WCHAR szKeyName[33];

    TRACE("%ld %p\n",index,lpguid);

    r = MSIREG_OpenComponents(&hkeyComponents);
    if( r != ERROR_SUCCESS )
        goto end;

    r = RegEnumKeyW(hkeyComponents, index, szKeyName, GUID_SIZE);

    unsquash_guid(szKeyName, lpguid);

end:

    if( hkeyComponents )
        RegCloseKey(hkeyComponents);

    return r;
}

UINT WINAPI MsiEnumClientsA(LPCSTR szComponent, DWORD index, LPSTR szProduct)
{
    DWORD r;
    WCHAR szwProduct[GUID_SIZE];
    LPWSTR szwComponent = NULL;

    TRACE("%s %ld %p\n",debugstr_a(szComponent),index,szProduct);

    if( szComponent )
    {
        UINT len = MultiByteToWideChar( CP_ACP, 0, szComponent, -1, NULL, 0 );
        szwComponent = HeapAlloc( GetProcessHeap(), 0, len * sizeof (WCHAR) );
        if( szwComponent )
            MultiByteToWideChar( CP_ACP, 0, szComponent, -1, szwComponent, len );
        else
            return ERROR_FUNCTION_FAILED;
    }

    r = MsiEnumClientsW(szComponent?szwComponent:NULL, index, szwProduct);
    if( r == ERROR_SUCCESS )
    {
        WideCharToMultiByte(CP_ACP, 0, szwProduct, -1,
                            szProduct, GUID_SIZE, NULL, NULL);
    }

    HeapFree( GetProcessHeap(), 0, szwComponent);

    return r;
}

UINT WINAPI MsiEnumClientsW(LPCWSTR szComponent, DWORD index, LPWSTR szProduct)
{
    HKEY hkeyComp = 0;
    DWORD r, sz;
    WCHAR szValName[GUID_SIZE];

    TRACE("%s %ld %p\n",debugstr_w(szComponent),index,szProduct);

    r = MSIREG_OpenComponentsKey(szComponent,&hkeyComp,FALSE);
    if( r != ERROR_SUCCESS )
        goto end;

    sz = GUID_SIZE;
    r = RegEnumValueW(hkeyComp, index, szValName, &sz, NULL, NULL, NULL, NULL);
    if( r != ERROR_SUCCESS )
        goto end;

    unsquash_guid(szValName, szProduct);

end:
    if( hkeyComp )
        RegCloseKey(hkeyComp);

    return r;
}

UINT WINAPI MsiEnumComponentQualifiersA( LPSTR szComponent, DWORD iIndex,
                LPSTR lpQualifierBuf, DWORD* pcchQualifierBuf,
                LPSTR lpApplicationDataBuf, DWORD* pcchApplicationDataBuf)
{
    FIXME("%s %08lx %p %p %p %p\n", debugstr_a(szComponent), iIndex,
          lpQualifierBuf, pcchQualifierBuf, lpApplicationDataBuf,
          pcchApplicationDataBuf);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiEnumComponentQualifiersW( LPWSTR szComponent, DWORD iIndex,
                LPWSTR lpQualifierBuf, DWORD* pcchQualifierBuf,
                LPWSTR lpApplicationDataBuf, DWORD* pcchApplicationDataBuf )
{
    FIXME("%s %08lx %p %p %p %p\n", debugstr_w(szComponent), iIndex,
          lpQualifierBuf, pcchQualifierBuf, lpApplicationDataBuf,
          pcchApplicationDataBuf);
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
    UINT r, len;
    LPWSTR szPack = NULL;

    TRACE("%s\n", debugstr_a(szPackage) );

    if( szPackage )
    {
        len = MultiByteToWideChar( CP_ACP, 0, szPackage, -1, NULL, 0 );
        szPack = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
        if( !szPack )
            return ERROR_OUTOFMEMORY;
        MultiByteToWideChar( CP_ACP, 0, szPackage, -1, szPack, len );
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
    UINT len, incoming_len;

    if( szProduct )
    {
        len = MultiByteToWideChar( CP_ACP, 0, szProduct, -1, NULL, 0 );
        szwProduct= HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
        if( !szwProduct)
            return ERROR_OUTOFMEMORY;
        MultiByteToWideChar( CP_ACP, 0, szProduct, -1, szwProduct, len );
    }

    if( szComponent )
    {
        len = MultiByteToWideChar( CP_ACP, 0, szComponent, -1, NULL, 0 );
        szwComponent= HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
        if( !szwComponent )
        {
            HeapFree( GetProcessHeap(), 0, szwProduct);
            return ERROR_OUTOFMEMORY;
        }
        MultiByteToWideChar( CP_ACP, 0, szComponent, -1, szwComponent, len );
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
    HKEY hkey=0;

    TRACE("%s %s %p %p\n", debugstr_w(szProduct),
           debugstr_w(szComponent), lpPathBuf, pcchBuf);

    squash_guid(szProduct,squished_pc);

    rc = MSIREG_OpenProductsKey(szProduct,&hkey,FALSE);
    if (rc != ERROR_SUCCESS)
        goto end;

    RegCloseKey(hkey);

    rc = MSIREG_OpenComponentsKey(szComponent,&hkey,FALSE);
    if (rc != ERROR_SUCCESS)
        goto end;

    *pcchBuf *= sizeof(WCHAR);
    rc = RegQueryValueExW(hkey,squished_pc,NULL,NULL,(LPVOID)lpPathBuf,
                          pcchBuf);
    *pcchBuf /= sizeof(WCHAR);

    if (rc!= ERROR_SUCCESS)
        goto end;

    TRACE("found path of (%s:%s)(%s)\n", debugstr_w(szComponent),
           debugstr_w(szProduct), debugstr_w(lpPathBuf));

    FIXME("Only working for installed files, not registry keys\n");
    if (GetFileAttributesW(lpPathBuf) != INVALID_FILE_ATTRIBUTES)
        rrc = INSTALLSTATE_LOCAL;
    else
        rrc = INSTALLSTATE_ABSENT;

end:
    RegCloseKey(hkey);
    return rrc;
}

INSTALLSTATE WINAPI MsiQueryFeatureStateA(LPCSTR szProduct, LPCSTR szFeature)
{
    INSTALLSTATE rc;
    UINT len;
    LPWSTR szwProduct= NULL;
    LPWSTR szwFeature= NULL;

    if( szProduct )
    {
        len = MultiByteToWideChar( CP_ACP, 0, szProduct, -1, NULL, 0 );
        szwProduct= HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
        if( !szwProduct)
            return ERROR_OUTOFMEMORY;
        MultiByteToWideChar( CP_ACP, 0, szProduct, -1, szwProduct, len );
    }

    if( szFeature )
    {
        len = MultiByteToWideChar( CP_ACP, 0, szFeature, -1, NULL, 0 );
        szwFeature= HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
        if( !szwFeature)
        {
            HeapFree( GetProcessHeap(), 0, szwProduct);
            return ERROR_OUTOFMEMORY;
        }
        MultiByteToWideChar( CP_ACP, 0, szFeature, -1, szwFeature, len );
    }

    rc = MsiQueryFeatureStateW(szwProduct, szwFeature);

    HeapFree( GetProcessHeap(), 0, szwProduct);
    HeapFree( GetProcessHeap(), 0, szwFeature);

    return rc;
}

INSTALLSTATE WINAPI MsiQueryFeatureStateW(LPCWSTR szProduct, LPCWSTR szFeature)
{
    FIXME("%s %s\n", debugstr_w(szProduct), debugstr_w(szFeature));
    return INSTALLSTATE_UNKNOWN;
}

UINT WINAPI MsiGetFileVersionA(LPCSTR szFilePath, LPSTR lpVersionBuf,
                DWORD* pcchVersionBuf, LPSTR lpLangBuf, DWORD* pcchLangBuf)
{
    LPWSTR szwFilePath = NULL, lpwVersionBuff = NULL, lpwLangBuff = NULL;
    UINT len, ret = ERROR_OUTOFMEMORY;
    
    if( szFilePath )
    {
        len = MultiByteToWideChar( CP_ACP, 0, szFilePath, -1, NULL, 0 );
        szwFilePath = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
        if( !szwFilePath )
            goto end;
        MultiByteToWideChar( CP_ACP, 0, szFilePath, -1, szwFilePath, len );
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
            *pcchVersionBuf = strlenW(lpVersionBuf);
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
        *pcchLangBuf = strlenW(lpLangBuf);
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
 *    	DllGetClassObject [MSI.@]
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
 *    	DllGetVersion [MSI.@]
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
 *    	DllCanUnloadNow [MSI.@]
 */
BOOL WINAPI MSI_DllCanUnloadNow(void)
{
    return S_FALSE;
}

UINT WINAPI MsiEnumRelatedProductsW(LPCWSTR szUpgradeCode, DWORD dwReserved,
                                    DWORD iProductIndex, LPWSTR lpProductBuf)
{
    FIXME("%s %lu %lu %p\n", debugstr_w(szUpgradeCode), dwReserved,
          iProductIndex, lpProductBuf);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiEnumRelatedProductsA(LPCSTR szUpgradeCode, DWORD dwReserved,
                                    DWORD iProductIndex, LPSTR lpProductBuf)
{
    FIXME("%s %lu %lu %p\n", debugstr_a(szUpgradeCode), dwReserved,
          iProductIndex, lpProductBuf);
    return ERROR_CALL_NOT_IMPLEMENTED;
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

UINT WINAPI MsiUseFeatureExW(LPCWSTR szProduct, LPCWSTR szFeature, 
                             DWORD dwInstallMode, DWORD dwReserved)
{
    FIXME("%s %s %li %li\n", debugstr_w(szProduct), debugstr_w(szFeature),
          dwInstallMode, dwReserved);
   
    return INSTALLSTATE_LOCAL; 
}

UINT WINAPI MsiProvideQualifiedComponentExW(LPCWSTR szComponent, 
                LPCWSTR szQualifier, DWORD dwInstallMode, LPWSTR szProduct,
                DWORD Unused1, DWORD Unused2, LPWSTR lpPathBuf, 
                DWORD* pcchPathBuf)
{
    FIXME("%s %s %li %s %li %li %p %p\n", debugstr_w(szComponent),
          debugstr_w(szQualifier), dwInstallMode, debugstr_w(szProduct),
          Unused1, Unused2, lpPathBuf, pcchPathBuf);

    return ERROR_INDEX_ABSENT;
}

USERINFOSTATE WINAPI MsiGetUserInfoW(LPCWSTR szProduct, LPWSTR lpUserNameBuf, 
                DWORD* pcchUserNameBuf, LPWSTR lpOrgNameBuf, 
                DWORD* pcchOrgNameBuf, LPWSTR lpSerialBuf, DWORD* pcchSerialBuf)
{
    FIXME("%s %p %p %p %p %p %p\n",debugstr_w(szProduct), lpUserNameBuf,
          pcchUserNameBuf, lpOrgNameBuf, pcchOrgNameBuf, lpSerialBuf,
          pcchSerialBuf);
  
    return USERINFOSTATE_UNKNOWN; 
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
    FIXME("%s\n",debugstr_w(szProduct));
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiCollectUserInfoA(LPCSTR szProduct)
{
    FIXME("%s\n",debugstr_a(szProduct));
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiCreateAndVerifyInstallerDirectory(void)
{
    FIXME("\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiGetShortcutTargetA( LPCSTR szShortcutTarget,
                                  LPSTR szProductCode, LPSTR szFeatureId,
                                  LPSTR szComponentCode)
{
    FIXME("\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiGetShortcutTargetW( LPCWSTR szShortcutTarget,
                                  LPWSTR szProductCode, LPWSTR szFeatureId,
                                  LPWSTR szComponentCode)
{
    FIXME("\n");
    return ERROR_CALL_NOT_IMPLEMENTED;
}
