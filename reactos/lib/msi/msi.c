/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2002,2003,2004 Mike McCormack for CodeWeavers
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

DEFINE_GUID( CLSID_MsiDatabase, 0x000c1084, 0x0000, 0x0000, 0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);

static const WCHAR szInstaller[] = {
'S','o','f','t','w','a','r','e','\\',
'M','i','c','r','o','s','o','f','t','\\',
'W','i','n','d','o','w','s','\\',
'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
'I','n','s','t','a','l','l','e','r',0 };

static const WCHAR szFeatures[] = {
'F','e','a','t','u','r','e','s',0 };
static const WCHAR szComponents[] = {
'C','o','m','p','o','n','e','n','t','s',0 };

/* the UI level */
INSTALLUILEVEL gUILevel;
HWND           gUIhwnd;
INSTALLUI_HANDLERA gUIHandler;
DWORD gUIFilter;
LPVOID gUIContext;
WCHAR gszLogFile[MAX_PATH];

/*
 *  .MSI  file format
 *
 *  A .msi file is a structured storage file.
 *  It should contain a number of streams.
 */

BOOL unsquash_guid(LPCWSTR in, LPWSTR out)
{
    DWORD i,n=0;

    out[n++]='{';
    for(i=0; i<8; i++)
        out[n++] = in[7-i];
    out[n++]='-';
    for(i=0; i<4; i++)
        out[n++] = in[11-i];
    out[n++]='-';
    for(i=0; i<4; i++)
        out[n++] = in[15-i];
    out[n++]='-';
    for(i=0; i<2; i++)
    {
        out[n++] = in[17+i*2];
        out[n++] = in[16+i*2];
    }
    out[n++]='-';
    for( ; i<8; i++)
    {
        out[n++] = in[17+i*2];
        out[n++] = in[16+i*2];
    }
    out[n++]='}';
    out[n]=0;
    return TRUE;
}

BOOL squash_guid(LPCWSTR in, LPWSTR out)
{
    DWORD i,n=0;

    if(in[n++] != '{')
        return FALSE;
    for(i=0; i<8; i++)
        out[7-i] = in[n++];
    if(in[n++] != '-')
        return FALSE;
    for(i=0; i<4; i++)
        out[11-i] = in[n++];
    if(in[n++] != '-')
        return FALSE;
    for(i=0; i<4; i++)
        out[15-i] = in[n++];
    if(in[n++] != '-')
        return FALSE;
    for(i=0; i<2; i++)
    {
        out[17+i*2] = in[n++];
        out[16+i*2] = in[n++];
    }
    if(in[n++] != '-')
        return FALSE;
    for( ; i<8; i++)
    {
        out[17+i*2] = in[n++];
        out[16+i*2] = in[n++];
    }
    out[32]=0;
    if(in[n++] != '}')
        return FALSE;
    if(in[n])
        return FALSE;
    return TRUE;
}

/* tables for encoding and decoding base85 */
static const unsigned char table_dec85[0x80] = {
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0x00,0xff,0xff,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0xff,
0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0xff,0xff,0xff,0x16,0xff,0x17,
0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0xff,0x34,0x35,0x36,
0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,0x40,0x41,0x42,0x43,0x44,0x45,0x46,
0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,0x50,0x51,0x52,0xff,0x53,0x54,0xff,
};

static const char table_enc85[] =
"!$%&'()*+,-.0123456789=?@ABCDEFGHIJKLMNO"
"PQRSTUVWXYZ[]^_`abcdefghijklmnopqrstuvwx"
"yz{}~";

/*
 *  Converts a base85 encoded guid into a GUID pointer
 *  Base85 encoded GUIDs should be 20 characters long.
 *
 *  returns TRUE if successful, FALSE if not
 */
BOOL decode_base85_guid( LPCWSTR str, GUID *guid )
{
    DWORD i, val = 0, base = 1, *p;

    p = (DWORD*) guid;
    for( i=0; i<20; i++ )
    {
        if( (i%5) == 0 )
        {
            val = 0;
            base = 1;
        }
        val += table_dec85[str[i]] * base;
        if( str[i] >= 0x80 )
            return FALSE;
        if( table_dec85[str[i]] == 0xff )
            return FALSE;
        if( (i%5) == 4 )
            p[i/5] = val;
        base *= 85;
    }
    return TRUE;
}

/*
 *  Encodes a base85 guid given a GUID pointer
 *  Caller should provide a 21 character buffer for the encoded string.
 *
 *  returns TRUE if successful, FALSE if not
 */
BOOL encode_base85_guid( GUID *guid, LPWSTR str )
{
    unsigned int x, *p, i;

    p = (unsigned int*) guid;
    for( i=0; i<4; i++ )
    {
        x = p[i];
        *str++ = table_enc85[x%85];
        x = x/85;
        *str++ = table_enc85[x%85];
        x = x/85;
        *str++ = table_enc85[x%85];
        x = x/85;
        *str++ = table_enc85[x%85];
        x = x/85;
        *str++ = table_enc85[x%85];
    }
    *str = 0;
    
    return TRUE;
}


VOID MSI_CloseDatabase( MSIOBJECTHDR *arg )
{
    MSIDATABASE *db = (MSIDATABASE *) arg;

    free_cached_tables( db );
    IStorage_Release( db->storage );
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
    if( szwPersist )
        HeapFree( GetProcessHeap(), 0, szwPersist );
    if( szwDBPath )
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

    if( szwProd )
        HeapFree( GetProcessHeap(), 0, szwProd );

    return ret;
}

UINT WINAPI MsiOpenProductW(LPCWSTR szProduct, MSIHANDLE *phProduct)
{
    static const WCHAR szKey[] = {
        'S','o','f','t','w','a','r','e','\\',
        'M','i','c','r','o','s','o','f','t','\\',
        'W','i','n','d','o','w','s','\\',
        'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
        'U','n','i','n','s','t','a','l','l',0 };
    static const WCHAR szLocalPackage[] = {
        'L','o','c','a','l','P','a','c','k','a','g','e', 0
    };
    LPWSTR path = NULL;
    UINT r;
    HKEY hKeyProduct = NULL, hKeyUninstall = NULL;
    DWORD count, type;

    TRACE("%s %p\n",debugstr_w(szProduct), phProduct);

    r = RegOpenKeyW( HKEY_LOCAL_MACHINE, szKey, &hKeyUninstall );
    if( r != ERROR_SUCCESS )
        return ERROR_UNKNOWN_PRODUCT;

    r = RegOpenKeyW( hKeyUninstall, szProduct, &hKeyProduct );
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
    if( path )
        HeapFree( GetProcessHeap(), 0, path );
    if( hKeyProduct )
        RegCloseKey( hKeyProduct );
    RegCloseKey( hKeyUninstall );

    return r;
}

UINT WINAPI MsiAdvertiseProductA(LPCSTR szPackagePath, LPCSTR szScriptfilePath, LPCSTR szTransforms, LANGID lgidLanguage)
{
    FIXME("%s %s %s 0x%08x\n",debugstr_a(szPackagePath), debugstr_a(szScriptfilePath), debugstr_a(szTransforms), lgidLanguage);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiAdvertiseProductW(LPCWSTR szPackagePath, LPCWSTR szScriptfilePath, LPCWSTR szTransforms, LANGID lgidLanguage)
{
    FIXME("%s %s %s 0x%08x\n",debugstr_w(szPackagePath), debugstr_w(szScriptfilePath), debugstr_w(szTransforms), lgidLanguage);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiAdvertiseProductExA(LPCSTR szPackagePath, LPCSTR szScriptfilePath, LPCSTR szTransforms, LANGID lgidLanguage, DWORD dwPlatform, DWORD dwOptions)
{
    FIXME("%s %s %s 0x%08x 0x%08lx 0x%08lx\n",
	debugstr_a(szPackagePath), debugstr_a(szScriptfilePath), debugstr_a(szTransforms), lgidLanguage, dwPlatform, dwOptions);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiAdvertiseProductExW( LPCWSTR szPackagePath, LPCWSTR szScriptfilePath, LPCWSTR szTransforms, LANGID lgidLanguage, DWORD dwPlatform, DWORD dwOptions)
{
    FIXME("%s %s %s 0x%08x 0x%08lx 0x%08lx\n",
	debugstr_w(szPackagePath), debugstr_w(szScriptfilePath), debugstr_w(szTransforms), lgidLanguage, dwPlatform, dwOptions);
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
    if( szwPath )
        HeapFree( GetProcessHeap(), 0, szwPath );
    
    if( szwCommand )
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
	FIXME("%s 0x%08lx\n", debugstr_a(szProduct), dwReinstallMode);
	return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiReinstallProductW(LPCWSTR szProduct, DWORD dwReinstallMode)
{
	FIXME("%s 0x%08lx\n", debugstr_w(szProduct), dwReinstallMode);
	return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiApplyPatchA(LPCSTR szPatchPackage, LPCSTR szInstallPackage, INSTALLTYPE eInstallType, LPCSTR szCommandLine)
{
	FIXME("%s %s %d %s\n", debugstr_a(szPatchPackage), debugstr_a(szInstallPackage), eInstallType, debugstr_a(szCommandLine));
	return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiApplyPatchW(LPCWSTR szPatchPackage, LPCWSTR szInstallPackage, INSTALLTYPE eInstallType, LPCWSTR szCommandLine)
{
	FIXME("%s %s %d %s\n", debugstr_w(szPatchPackage), debugstr_w(szInstallPackage), eInstallType, debugstr_w(szCommandLine));
	return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiConfigureProductA(LPCSTR szProduct, int iInstallLevel, INSTALLSTATE eInstallState)
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
    if( szwProduct )
        HeapFree( GetProcessHeap(), 0, szwProduct );

    return hr;
}

UINT WINAPI MsiConfigureProductW(LPCWSTR szProduct, int iInstallLevel, INSTALLSTATE eInstallState)
{
    FIXME("%s %d %d\n",debugstr_w(szProduct), iInstallLevel, eInstallState);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiGetProductCodeA(LPCSTR szComponent, LPSTR szBuffer)
{
    LPWSTR szwComponent = NULL, szwBuffer = NULL;
    UINT hr = ERROR_INSTALL_FAILURE;

    FIXME("%s %s\n",debugstr_a(szComponent), debugstr_a(szBuffer));

    if( szComponent )
    {
        UINT len = MultiByteToWideChar( CP_ACP, 0, szComponent, -1, NULL, 0 );
        szwComponent = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        if( !szwComponent )
            goto end;
        MultiByteToWideChar( CP_ACP, 0, szComponent, -1, szwComponent, len );
    } else {
      return ERROR_INVALID_PARAMETER;
    }

    {
        szwBuffer = HeapAlloc( GetProcessHeap(), 0, GUID_SIZE * sizeof(WCHAR) );
        if( !szwBuffer )	 
            goto end;
    }

    hr = MsiGetProductCodeW( szwComponent, szwBuffer );

    if( ERROR_SUCCESS == hr )
    {
        WideCharToMultiByte(CP_ACP, 0, szwBuffer, -1, szBuffer, GUID_SIZE, NULL, NULL);
    }

end:
    if( szwComponent )
        HeapFree( GetProcessHeap(), 0, szwComponent );
    if( szwBuffer )
        HeapFree( GetProcessHeap(), 0, szwBuffer );

    return hr;
}

UINT WINAPI MsiGetProductCodeW(LPCWSTR szComponent, LPWSTR szBuffer)
{
    FIXME("%s %s\n",debugstr_w(szComponent), debugstr_w(szBuffer));
    if (NULL == szComponent) {
      return ERROR_INVALID_PARAMETER;
    }
    return ERROR_CALL_NOT_IMPLEMENTED;
}




UINT WINAPI MsiGetProductInfoA(LPCSTR szProduct, LPCSTR szAttribute, LPSTR szBuffer, DWORD *pcchValueBuf)
{
    LPWSTR szwProduct = NULL, szwAttribute = NULL, szwBuffer = NULL;
    UINT hr = ERROR_INSTALL_FAILURE;

    FIXME("%s %s %p %p\n",debugstr_a(szProduct), debugstr_a(szAttribute), szBuffer, pcchValueBuf);

    if (NULL != szBuffer && NULL == pcchValueBuf) {
      return ERROR_INVALID_PARAMETER;
    }
    if( szProduct )
    {
        UINT len = MultiByteToWideChar( CP_ACP, 0, szProduct, -1, NULL, 0 );
        szwProduct = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        if( !szwProduct )
            goto end;
        MultiByteToWideChar( CP_ACP, 0, szProduct, -1, szwProduct, len );
    } else {
      return ERROR_INVALID_PARAMETER;
    }
    
    if( szAttribute )
    {
        UINT len = MultiByteToWideChar( CP_ACP, 0, szAttribute, -1, NULL, 0 );
        szwAttribute = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        if( !szwAttribute )
            goto end;
        MultiByteToWideChar( CP_ACP, 0, szAttribute, -1, szwAttribute, len );
    } else {
      return ERROR_INVALID_PARAMETER;
    }

    if( szBuffer )
    {
        szwBuffer = HeapAlloc( GetProcessHeap(), 0, (*pcchValueBuf) * sizeof(WCHAR) );
        if( !szwBuffer )	 
            goto end;
    }

    hr = MsiGetProductInfoW( szwProduct, szwAttribute, szwBuffer, pcchValueBuf );

    if( ERROR_SUCCESS == hr )
    {
        WideCharToMultiByte(CP_ACP, 0, szwBuffer, -1, szBuffer, *pcchValueBuf, NULL, NULL);
    }

end:
    if( szwProduct )
        HeapFree( GetProcessHeap(), 0, szwProduct );
    if( szwAttribute )
        HeapFree( GetProcessHeap(), 0, szwAttribute );
    if( szwBuffer )
        HeapFree( GetProcessHeap(), 0, szwBuffer );

    return hr;    
}

UINT WINAPI MsiGetProductInfoW(LPCWSTR szProduct, LPCWSTR szAttribute, LPWSTR szBuffer, DWORD *pcchValueBuf)
{
    MSIHANDLE hProduct;
    UINT hr;
    
    FIXME("%s %s %p %p\n",debugstr_w(szProduct), debugstr_w(szAttribute), szBuffer, pcchValueBuf);

    if (NULL != szBuffer && NULL == pcchValueBuf) {
      return ERROR_INVALID_PARAMETER;
    }
    if (NULL == szProduct || NULL == szAttribute) {
      return ERROR_INVALID_PARAMETER;
    }

    hr = MsiOpenProductW(szProduct, &hProduct);
    if (ERROR_SUCCESS != hr) return hr;

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
    } else {
      return ERROR_INVALID_PARAMETER;
    }

    hr = MsiEnableLogW( dwLogMode, szwLogFile, attributes );

end:
    if( szwLogFile )
        HeapFree( GetProcessHeap(), 0, szwLogFile );

    return hr;
}

UINT WINAPI MsiEnableLogW(DWORD dwLogMode, LPCWSTR szLogFile, DWORD attributes)
{
    HANDLE the_file = INVALID_HANDLE_VALUE;
    TRACE("%08lx %s %08lx\n", dwLogMode, debugstr_w(szLogFile), attributes);
    strcpyW(gszLogFile,szLogFile);
    if (!(attributes & INSTALLLOGATTRIBUTES_APPEND))
        DeleteFileW(szLogFile);
    the_file = CreateFileW(szLogFile, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, NULL);
    if (the_file != INVALID_HANDLE_VALUE)
        CloseHandle(the_file);
    else
        ERR("Unable to enable log %s\n",debugstr_w(szLogFile));

    return ERROR_SUCCESS;
}

INSTALLSTATE WINAPI MsiQueryProductStateA(LPCSTR szProduct)
{
    FIXME("%s\n", debugstr_a(szProduct));
    return INSTALLSTATE_UNKNOWN;
}

INSTALLSTATE WINAPI MsiQueryProductStateW(LPCWSTR szProduct)
{
    FIXME("%s\n", debugstr_w(szProduct));
    return INSTALLSTATE_UNKNOWN;
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
    INSTALLUI_HANDLERA prev = gUIHandler;

    TRACE("(%p %lx %p)\n",puiHandler,dwMessageFilter,pvContext);
    gUIHandler = puiHandler;
    gUIFilter = dwMessageFilter;
    gUIContext = pvContext;

    return prev;
}

UINT WINAPI MsiLoadStringA(HINSTANCE hInstance, UINT uID, LPSTR lpBuffer, int nBufferMax, DWORD e)
{
    /*FIXME("%08lx %08lx %08lx %08lx %08lx\n",a,b,c,d,e);*/
    FIXME("%p %u %p %d %08lx\n",hInstance,uID,lpBuffer,nBufferMax,e);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiLoadStringW(HINSTANCE hInstance, UINT uID, LPWSTR lpBuffer, int nBufferMax, DWORD e)
{
    FIXME("%p %u %p %d %08lx\n",hInstance,uID,lpBuffer,nBufferMax,e);
    /*
    int ret =  LoadStringW(hInstance,uID,lpBuffer,nBufferMax);
    FIXME("%s\n",debugstr_w(lpBuffer));
    return ret;
    */
    return ERROR_CALL_NOT_IMPLEMENTED;
}

INSTALLSTATE WINAPI MsiLocateComponentA(LPCSTR szComponent, LPSTR lpPathBuf, DWORD *pcchBuf)
{
    FIXME("%s %p %08lx\n", debugstr_a(szComponent), lpPathBuf, *pcchBuf);
    return INSTALLSTATE_UNKNOWN;
}

INSTALLSTATE WINAPI MsiLocateComponentW(LPCWSTR szComponent, LPSTR lpPathBuf, DWORD *pcchBuf)
{
    FIXME("%s %p %08lx\n", debugstr_w(szComponent), lpPathBuf, *pcchBuf);
    return INSTALLSTATE_UNKNOWN;
}

#include "winuser.h"

UINT WINAPI MsiMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType, WORD wLanguageId, DWORD f)
{
    FIXME("%p %s %s %u %08x %08lx\n",hWnd,debugstr_a(lpText),debugstr_a(lpCaption),uType,wLanguageId,f);
    /*
    MessageBoxExA(hWnd,lpText,lpCaption,uType|MB_OK,wLanguageId);
    */
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType, WORD wLanguageId, DWORD f)
{
    /*FIXME("%08lx %08lx %08lx %08lx %08lx %08lx\n",a,b,c,d,e,f);*/
    FIXME("%p %s %s %u %08x %08lx\n",hWnd,debugstr_w(lpText),debugstr_w(lpCaption),uType,wLanguageId,f);
    /*
    MessageBoxExW(hWnd,lpText,lpCaption,uType|MB_OK,wLanguageId);
    */
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
    HKEY hkey = 0, hkeyFeatures = 0;
    DWORD r;
    WCHAR szKeyName[33];

    TRACE("%ld %p\n",index,lpguid);

    if (NULL == lpguid) {
      return ERROR_INVALID_PARAMETER;
    }
    r = RegOpenKeyW(HKEY_LOCAL_MACHINE, szInstaller, &hkey);
    if( r != ERROR_SUCCESS )
        goto end;

    r = RegOpenKeyW(hkey, szFeatures, &hkeyFeatures);
    if( r != ERROR_SUCCESS )
        goto end;

    r = RegEnumKeyW(hkeyFeatures, index, szKeyName, GUID_SIZE);

    unsquash_guid(szKeyName, lpguid);

end:

    if( hkeyFeatures )
        RegCloseKey(hkeyFeatures);
    if( hkey )
        RegCloseKey(hkey);

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

    if( szwProduct )
        HeapFree( GetProcessHeap(), 0, szwProduct);

    return r;
}

UINT WINAPI MsiEnumFeaturesW(LPCWSTR szProduct, DWORD index, 
      LPWSTR szFeature, LPWSTR szParent)
{
    HKEY hkey = 0, hkeyFeatures = 0, hkeyProduct = 0;
    DWORD r, sz;
    WCHAR szRegName[GUID_SIZE];

    TRACE("%s %ld %p %p\n",debugstr_w(szProduct),index,szFeature,szParent);

    if( !squash_guid(szProduct, szRegName) )
        return ERROR_INVALID_PARAMETER;

    r = RegOpenKeyW(HKEY_LOCAL_MACHINE, szInstaller, &hkey);
    if( r != ERROR_SUCCESS )
        goto end;

    r = RegOpenKeyW(hkey, szFeatures, &hkeyFeatures);
    if( r != ERROR_SUCCESS )
        goto end;

    r = RegOpenKeyW(hkeyFeatures, szRegName, &hkeyProduct);
    if( r != ERROR_SUCCESS )
        goto end;

    sz = GUID_SIZE;
    r = RegEnumValueW(hkeyProduct, index, szFeature, &sz, NULL, NULL, NULL, NULL);

end:
    if( hkeyProduct )
        RegCloseKey(hkeyProduct);
    if( hkeyFeatures )
        RegCloseKey(hkeyFeatures);
    if( hkey )
        RegCloseKey(hkey);

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
    HKEY hkey = 0, hkeyComponents = 0;
    DWORD r;
    WCHAR szKeyName[33];

    TRACE("%ld %p\n",index,lpguid);

    r = RegOpenKeyW(HKEY_LOCAL_MACHINE, szInstaller, &hkey);
    if( r != ERROR_SUCCESS )
        goto end;

    r = RegOpenKeyW(hkey, szComponents, &hkeyComponents);
    if( r != ERROR_SUCCESS )
        goto end;

    r = RegEnumKeyW(hkeyComponents, index, szKeyName, GUID_SIZE);

    unsquash_guid(szKeyName, lpguid);

end:

    if( hkeyComponents )
        RegCloseKey(hkeyComponents);
    if( hkey )
        RegCloseKey(hkey);

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

    if( szwComponent )
        HeapFree( GetProcessHeap(), 0, szwComponent);

    return r;
}

UINT WINAPI MsiEnumClientsW(LPCWSTR szComponent, DWORD index, LPWSTR szProduct)
{
    HKEY hkey = 0, hkeyComponents = 0, hkeyComp = 0;
    DWORD r, sz;
    WCHAR szRegName[GUID_SIZE], szValName[GUID_SIZE];

    TRACE("%s %ld %p\n",debugstr_w(szComponent),index,szProduct);

    if( !squash_guid(szComponent, szRegName) )
        return ERROR_INVALID_PARAMETER;

    r = RegOpenKeyW(HKEY_LOCAL_MACHINE, szInstaller, &hkey);
    if( r != ERROR_SUCCESS )
        goto end;

    r = RegOpenKeyW(hkey, szComponents, &hkeyComponents);
    if( r != ERROR_SUCCESS )
        goto end;

    r = RegOpenKeyW(hkeyComponents, szRegName, &hkeyComp);
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
    if( hkeyComponents )
        RegCloseKey(hkeyComponents);
    if( hkey )
        RegCloseKey(hkey);

    return r;
}

UINT WINAPI MsiEnumComponentQualifiersA(
    LPSTR szComponent, DWORD iIndex, LPSTR lpQualifierBuf, DWORD* pcchQualifierBuf, LPSTR lpApplicationDataBuf, DWORD* pcchApplicationDataBuf)
{
FIXME("%s 0x%08lx %p %p %p %p\n", debugstr_a(szComponent), iIndex, lpQualifierBuf, pcchQualifierBuf, lpApplicationDataBuf, pcchApplicationDataBuf);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiEnumComponentQualifiersW(
    LPWSTR szComponent, DWORD iIndex, LPWSTR lpQualifierBuf, DWORD* pcchQualifierBuf, LPWSTR lpApplicationDataBuf, DWORD* pcchApplicationDataBuf)
{
FIXME("%s 0x%08lx %p %p %p %p\n", debugstr_w(szComponent), iIndex, lpQualifierBuf, pcchQualifierBuf, lpApplicationDataBuf, pcchApplicationDataBuf);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiProvideAssemblyA(
    LPCSTR szAssemblyName, LPCSTR szAppContext, DWORD dwInstallMode, DWORD dwAssemblyInfo, LPSTR lpPathBuf, DWORD* pcchPathBuf) 
{
    FIXME("%s %s 0x%08lx 0x%08lx %p %p\n", 
	debugstr_a(szAssemblyName),  debugstr_a(szAppContext), dwInstallMode, dwAssemblyInfo, lpPathBuf, pcchPathBuf);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiProvideAssemblyW(
    LPCWSTR szAssemblyName, LPCWSTR szAppContext, DWORD dwInstallMode, DWORD dwAssemblyInfo, LPWSTR lpPathBuf, DWORD* pcchPathBuf) 
{
    FIXME("%s %s 0x%08lx 0x%08lx %p %p\n", 
	debugstr_w(szAssemblyName),  debugstr_w(szAppContext), dwInstallMode, dwAssemblyInfo, lpPathBuf, pcchPathBuf);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiProvideComponentFromDescriptorA( LPCSTR szDescriptor, LPSTR szPath, DWORD *pcchPath, DWORD *pcchArgs )
{
    FIXME("%s %p %p %p\n", debugstr_a(szDescriptor), szPath, pcchPath, pcchArgs );
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiProvideComponentFromDescriptorW( LPCWSTR szDescriptor, LPWSTR szPath, DWORD *pcchPath, DWORD *pcchArgs )
{
    FIXME("%s %p %p %p\n", debugstr_w(szDescriptor), szPath, pcchPath, pcchArgs );
    return ERROR_CALL_NOT_IMPLEMENTED;
}

HRESULT WINAPI MsiGetFileSignatureInformationA(
  LPCSTR szSignedObjectPath, DWORD dwFlags, PCCERT_CONTEXT* ppcCertContext, BYTE* pbHashData, DWORD* pcbHashData)
{
    FIXME("%s 0x%08lx %p %p %p\n", debugstr_a(szSignedObjectPath), dwFlags, ppcCertContext, pbHashData, pcbHashData);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

HRESULT WINAPI MsiGetFileSignatureInformationW(
  LPCWSTR szSignedObjectPath, DWORD dwFlags, PCCERT_CONTEXT* ppcCertContext, BYTE* pbHashData, DWORD* pcbHashData)
{
    FIXME("%s 0x%08lx %p %p %p\n", debugstr_w(szSignedObjectPath), dwFlags, ppcCertContext, pbHashData, pcbHashData);
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
    INSTALLSTATE rc;
    UINT len;
    LPWSTR szwProduct= NULL;
    LPWSTR szwComponent= NULL;
    LPWSTR lpwPathBuf= NULL;

    if( szProduct)
    {
        len = MultiByteToWideChar( CP_ACP, 0, szProduct, -1, NULL, 0 );
        szwProduct= HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
        if( !szwProduct)
            return ERROR_OUTOFMEMORY;
        MultiByteToWideChar( CP_ACP, 0, szProduct, -1, szwProduct, len );
    }

    if( szComponent)
    {
        len = MultiByteToWideChar( CP_ACP, 0, szComponent, -1, NULL, 0 );
        szwComponent= HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
        if( !szwComponent)
            return ERROR_OUTOFMEMORY;
        MultiByteToWideChar( CP_ACP, 0, szComponent, -1, szwComponent, len );
    }

    if (pcchBuf && *pcchBuf > 0)
        lpwPathBuf = HeapAlloc( GetProcessHeap(), 0, *pcchBuf * sizeof(WCHAR));
    else
        lpwPathBuf = NULL;

    rc = MsiGetComponentPathW(szwProduct, szwComponent, lpwPathBuf, pcchBuf);

    HeapFree( GetProcessHeap(), 0, szwProduct);
    HeapFree( GetProcessHeap(), 0, szwComponent);
    if (lpwPathBuf)
    {
        WideCharToMultiByte(CP_ACP, 0, lpwPathBuf, *pcchBuf,
                            lpPathBuf, GUID_SIZE, NULL, NULL);
        HeapFree( GetProcessHeap(), 0, lpwPathBuf);
    }

    return rc;
}

INSTALLSTATE WINAPI MsiGetComponentPathW(LPCWSTR szProduct, LPCWSTR szComponent,
                                         LPWSTR lpPathBuf, DWORD* pcchBuf)
{
    FIXME("STUB: (%s %s %p %p)\n", debugstr_w(szProduct),
           debugstr_w(szComponent), lpPathBuf, pcchBuf);

    return INSTALLSTATE_UNKNOWN;
}

INSTALLSTATE WINAPI MsiQueryFeatureStateA(LPCSTR szProduct, LPCSTR szFeature)
{
    INSTALLSTATE rc;
    UINT len;
    LPWSTR szwProduct= NULL;
    LPWSTR szwFeature= NULL;

    if( szProduct)
    {
        len = MultiByteToWideChar( CP_ACP, 0, szProduct, -1, NULL, 0 );
        szwProduct= HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
        if( !szwProduct)
            return ERROR_OUTOFMEMORY;
        MultiByteToWideChar( CP_ACP, 0, szProduct, -1, szwProduct, len );
    }

    if( szFeature)
    {
        len = MultiByteToWideChar( CP_ACP, 0, szFeature, -1, NULL, 0 );
        szwFeature= HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
        if( !szwFeature)
            return ERROR_OUTOFMEMORY;
        MultiByteToWideChar( CP_ACP, 0, szFeature, -1, szwFeature, len );
    }

    rc = MsiQueryFeatureStateW(szwProduct, szwFeature);

    HeapFree( GetProcessHeap(), 0, szwProduct);
    HeapFree( GetProcessHeap(), 0, szwFeature);

    return rc;
}

INSTALLSTATE WINAPI MsiQueryFeatureStateW(LPCWSTR szProduct, LPCWSTR szFeature)
{
    FIXME("STUB: (%s %s)\n", debugstr_w(szProduct), debugstr_w(szFeature));
    return INSTALLSTATE_UNKNOWN;
}

UINT WINAPI MsiGetFileVersionA(LPCSTR szFilePath, LPSTR lpVersionBuf, DWORD* pcchVersionBuf, LPSTR lpLangBuf, DWORD* pcchLangBuf)
{
    UINT len;
    UINT ret;
    LPWSTR szwFilePath = NULL;
    LPWSTR lpwVersionBuff = NULL;
    LPWSTR lpwLangBuff = NULL;
    
    if(szFilePath) {
        len = MultiByteToWideChar( CP_ACP, 0, szFilePath, -1, NULL, 0 );
        szwFilePath = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
        if( !szwFilePath)
            return ERROR_OUTOFMEMORY;
        MultiByteToWideChar( CP_ACP, 0, szFilePath, -1, szwFilePath, len );
    }
    
    if(lpVersionBuf && pcchVersionBuf && *pcchVersionBuf) {
        lpwVersionBuff = HeapAlloc(GetProcessHeap(), 0, *pcchVersionBuf * sizeof(WCHAR));
        if( !lpwVersionBuff)
            return ERROR_OUTOFMEMORY;
    }

    if(lpLangBuf && pcchLangBuf && *pcchLangBuf) {
        lpwLangBuff = HeapAlloc(GetProcessHeap(), 0, *pcchVersionBuf * sizeof(WCHAR));
        if( !lpwLangBuff)
            return ERROR_OUTOFMEMORY;
    }
        
    ret = MsiGetFileVersionW(szwFilePath, lpwVersionBuff, pcchVersionBuf, lpwLangBuff, pcchLangBuf);
    
    if(lpwVersionBuff)
        WideCharToMultiByte(CP_ACP, 0, lpwVersionBuff, -1, lpVersionBuf, *pcchVersionBuf, NULL, NULL);
    if(lpwLangBuff)
        WideCharToMultiByte(CP_ACP, 0, lpwLangBuff, -1, lpLangBuf, *pcchLangBuf, NULL, NULL);
    
    if(szwFilePath) HeapFree(GetProcessHeap(), 0, szwFilePath);
    if(lpwVersionBuff) HeapFree(GetProcessHeap(), 0, lpwVersionBuff);
    if(lpwLangBuff) HeapFree(GetProcessHeap(), 0, lpwLangBuff);
    
    return ret;
}

UINT WINAPI MsiGetFileVersionW(LPCWSTR szFilePath, LPWSTR lpVersionBuf, DWORD* pcchVersionBuf, LPWSTR lpLangBuf, DWORD* pcchLangBuf)
{
    static const WCHAR szVersionResource[] = {'\\',0};
    static const WCHAR szVersionFormat[] = {'%','d','.','%','d','.','%','d','.','%','d',0};
    static const WCHAR szLangFormat[] = {'%','d',0};
    UINT ret = 0;
    DWORD dwVerLen;
    LPVOID lpVer = NULL;
    VS_FIXEDFILEINFO *ffi;
    UINT puLen;
    WCHAR tmp[32];

    TRACE("(%s,%p,%ld,%p,%ld)\n", debugstr_w(szFilePath),
          lpVersionBuf, pcchVersionBuf?*pcchVersionBuf:0,
          lpLangBuf, pcchLangBuf?*pcchLangBuf:0);

    dwVerLen = GetFileVersionInfoSizeW(szFilePath, NULL);
    if(!dwVerLen)
        return GetLastError();

    lpVer = HeapAlloc(GetProcessHeap(), 0, dwVerLen);
    if(!lpVer) {
        ret = ERROR_OUTOFMEMORY;
        goto end;
    }

    if(!GetFileVersionInfoW(szFilePath, 0, dwVerLen, lpVer)) {
        ret = GetLastError();
        goto end;
    }
    if(lpVersionBuf && pcchVersionBuf && *pcchVersionBuf) {
        if(VerQueryValueW(lpVer, szVersionResource, (LPVOID*)&ffi, &puLen) && puLen > 0) {
            wsprintfW(tmp, szVersionFormat, HIWORD(ffi->dwFileVersionMS), LOWORD(ffi->dwFileVersionMS), HIWORD(ffi->dwFileVersionLS), LOWORD(ffi->dwFileVersionLS));
            lstrcpynW(lpVersionBuf, tmp, *pcchVersionBuf);
            *pcchVersionBuf = strlenW(lpVersionBuf);
        }
        else {
            *lpVersionBuf = 0;
            *pcchVersionBuf = 0;
        }
    }

    if(lpLangBuf && pcchLangBuf && *pcchLangBuf) {
        DWORD lang = GetUserDefaultLangID();
        FIXME("Retrieve language from file\n");
        wsprintfW(tmp, szLangFormat, lang);
        lstrcpynW(lpLangBuf, tmp, *pcchLangBuf);
        *pcchLangBuf = strlenW(lpLangBuf);
    }

end:
    if(lpVer) HeapFree(GetProcessHeap(), 0, lpVer);
    return ret;
}


/******************************************************************
 *		DllMain
 *
 * @todo: maybe we can check here if MsiServer service is declared no ?
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
  if (fdwReason == DLL_PROCESS_ATTACH) {
    DisableThreadLibraryCalls(hinstDLL);
    /*
     * UI Initialization
     */
    gUILevel = INSTALLUILEVEL_BASIC;
    gUIhwnd = 0;
    gUIHandler = NULL;
    gUIFilter = 0;
    gUIContext = NULL;
    gszLogFile[0]=0;
    /* FIXME: Initialisation */
  } else if (fdwReason == DLL_PROCESS_DETACH) {
    /* FIXME: Cleanup */
  }
  /*
  static const WCHAR szMSIServerSvc[] = { 'M','S','I','S','e','r','v','e','r',0 };
  static const WCHAR szNull[] = { 0 };
  if (!strcmpW(lpServiceName, szMSIServerSvc)) {
    hKey = CreateServiceW(hSCManager, 
			  szMSIServerSvc, 
			  szMSIServerSvc, 
			  SC_MANAGER_ALL_ACCESS, 
			  SERVICE_WIN32_OWN_PROCESS|SERVICE_INTERACTIVE_PROCESS, 
			  SERVICE_AUTO_START, 
			  SERVICE_ERROR_IGNORE,
			  szNull, 
			  NULL, 
			  NULL,
			  NULL,
			  NULL,
			  szNull);
  */
  return TRUE;
}

typedef struct {
  /* IUnknown fields */
  IClassFactoryVtbl          *lpVtbl;
  DWORD                       ref;
} IClassFactoryImpl;

static HRESULT WINAPI MsiCF_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
  IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
  FIXME("(%p, %s, %p): stub\n",This,debugstr_guid(riid),ppobj);
  return E_NOINTERFACE;
}

static ULONG WINAPI MsiCF_AddRef(LPCLASSFACTORY iface) {
  IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
  return ++(This->ref);
}

static ULONG WINAPI MsiCF_Release(LPCLASSFACTORY iface) {
  IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
  /* static class, won't be  freed */
  return --(This->ref);
}

static HRESULT WINAPI MsiCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter, REFIID riid, LPVOID *ppobj) {
  IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
  FIXME ("(%p, %p, %s, %p): to implement\n", This, pOuter, debugstr_guid(riid), ppobj);
  return 0;
}

static HRESULT WINAPI MsiCF_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
  IClassFactoryImpl *This = (IClassFactoryImpl *)iface;
  FIXME("(%p, %d): stub\n", This, dolock);
  return S_OK;
}

static IClassFactoryVtbl MsiCF_Vtbl = {
  MsiCF_QueryInterface,
  MsiCF_AddRef,
  MsiCF_Release,
  MsiCF_CreateInstance,
  MsiCF_LockServer
};

static IClassFactoryImpl Msi_CF = {&MsiCF_Vtbl, 1 };

/******************************************************************
 *		DllGetClassObject (MSI.@)
 */
HRESULT WINAPI MSI_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv) {
  FIXME("(%s, %s, %p): almost a stub.\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
  if (IsEqualCLSID (rclsid, &CLSID_IMsiServer)) {
    *ppv = (LPVOID) &Msi_CF;
    IClassFactory_AddRef((IClassFactory*)*ppv);
    return S_OK;
  } else if (IsEqualCLSID (rclsid, &CLSID_IMsiServerMessage)) {
    *ppv = (LPVOID) &Msi_CF;
    IClassFactory_AddRef((IClassFactory*)*ppv);
    return S_OK;
  } else if (IsEqualCLSID (rclsid, &CLSID_IMsiServerX1)) {
    *ppv = (LPVOID) &Msi_CF;
    IClassFactory_AddRef((IClassFactory*)*ppv);
    return S_OK;
  } else if (IsEqualCLSID (rclsid, &CLSID_IMsiServerX2)) {
    *ppv = (LPVOID) &Msi_CF;
    IClassFactory_AddRef((IClassFactory*)*ppv);
    return S_OK;
  } else if (IsEqualCLSID (rclsid, &CLSID_IMsiServerX3)) {
    *ppv = (LPVOID) &Msi_CF;
    IClassFactory_AddRef((IClassFactory*)*ppv);
    return S_OK;
  }
  WARN("(%s, %s, %p): no interface found.\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
  return CLASS_E_CLASSNOTAVAILABLE;
}

/******************************************************************
 *		DllGetVersion (MSI.@)
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
 *		DllCanUnloadNow (MSI.@)
 */
BOOL WINAPI MSI_DllCanUnloadNow(void)
{
  return S_FALSE;
}

UINT WINAPI MsiEnumRelatedProductsA (LPCSTR lpUpgradeCode, DWORD dwReserved,
                                    DWORD iProductIndex, LPSTR lpProductBuf)
{
    FIXME("STUB: (%s, %li %li %s)\n",lpUpgradeCode, dwReserved, iProductIndex,
          lpProductBuf);
    return ERROR_CALL_NOT_IMPLEMENTED;
}
