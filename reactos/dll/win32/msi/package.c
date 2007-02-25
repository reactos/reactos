/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2004 Aric Stewart for CodeWeavers
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

#include <stdarg.h>
#include <stdio.h>
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winnls.h"
#include "shlwapi.h"
#include "wingdi.h"
#include "wine/debug.h"
#include "msi.h"
#include "msiquery.h"
#include "objidl.h"
#include "wincrypt.h"
#include "winuser.h"
#include "wininet.h"
#include "urlmon.h"
#include "shlobj.h"
#include "wine/unicode.h"
#include "objbase.h"
#include "msidefs.h"

#include "msipriv.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

static void msi_free_properties( MSIPACKAGE *package );

static void MSI_FreePackage( MSIOBJECTHDR *arg)
{
    MSIPACKAGE *package= (MSIPACKAGE*) arg;

    if( package->dialog )
        msi_dialog_destroy( package->dialog );
    ACTION_free_package_structures(package);

    msi_free_properties( package );

    msiobj_release( &package->db->hdr );
}

static UINT clone_properties( MSIPACKAGE *package )
{
    MSIQUERY * view = NULL;
    UINT rc;
    static const WCHAR Query[] = {
       'S','E','L','E','C','T',' ','*',' ',
       'F','R','O','M',' ','`','P','r','o','p','e','r','t','y','`',0};

    /* clone the existing properties */
    rc = MSI_DatabaseOpenViewW( package->db, Query, &view );
    if (rc != ERROR_SUCCESS)
        return rc;

    rc = MSI_ViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
        return rc;
    }
    while (1)
    {
        MSIRECORD * row;
        LPCWSTR name, value;

        rc = MSI_ViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
            break;

        name = MSI_RecordGetString( row, 1 );
        value = MSI_RecordGetString( row, 2 );
        MSI_SetPropertyW( package, name, value );

        msiobj_release( &row->hdr );
    }
    MSI_ViewClose(view);
    msiobj_release(&view->hdr);

    return rc;
}

/*
 * set_installed_prop
 *
 * Sets the "Installed" property to indicate that
 *  the product is installed for the current user.
 */
static UINT set_installed_prop( MSIPACKAGE *package )
{
    static const WCHAR szInstalled[] = {
        'I','n','s','t','a','l','l','e','d',0 };
    WCHAR val[2] = { '1', 0 };
    HKEY hkey = 0;
    UINT r;

    r = MSIREG_OpenUninstallKey( package->ProductCode, &hkey, FALSE );
    if (r == ERROR_SUCCESS)
    {
        RegCloseKey( hkey );
        MSI_SetPropertyW( package, szInstalled, val );
    }

    return r;
}

/*
 * There are a whole slew of these we need to set
 *
 *
http://msdn.microsoft.com/library/default.asp?url=/library/en-us/msi/setup/properties.asp
 */
static VOID set_installer_properties(MSIPACKAGE *package)
{
    WCHAR pth[MAX_PATH];
    WCHAR *ptr;
    OSVERSIONINFOA OSVersion;
    MEMORYSTATUSEX msex;
    DWORD verval;
    WCHAR verstr[10], bufstr[20];
    HDC dc;
    LPWSTR check;
    HKEY hkey;
    LONG res;

    static const WCHAR cszbs[]={'\\',0};
    static const WCHAR CFF[] = 
{'C','o','m','m','o','n','F','i','l','e','s','F','o','l','d','e','r',0};
    static const WCHAR PFF[] = 
{'P','r','o','g','r','a','m','F','i','l','e','s','F','o','l','d','e','r',0};
    static const WCHAR CADF[] = 
{'C','o','m','m','o','n','A','p','p','D','a','t','a','F','o','l','d','e','r',0};
    static const WCHAR FaF[] = 
{'F','a','v','o','r','i','t','e','s','F','o','l','d','e','r',0};
    static const WCHAR FoF[] = 
{'F','o','n','t','s','F','o','l','d','e','r',0};
    static const WCHAR SendTF[] = 
{'S','e','n','d','T','o','F','o','l','d','e','r',0};
    static const WCHAR SMF[] = 
{'S','t','a','r','t','M','e','n','u','F','o','l','d','e','r',0};
    static const WCHAR StF[] = 
{'S','t','a','r','t','u','p','F','o','l','d','e','r',0};
    static const WCHAR TemplF[] = 
{'T','e','m','p','l','a','t','e','F','o','l','d','e','r',0};
    static const WCHAR DF[] = 
{'D','e','s','k','t','o','p','F','o','l','d','e','r',0};
    static const WCHAR PMF[] = 
{'P','r','o','g','r','a','m','M','e','n','u','F','o','l','d','e','r',0};
    static const WCHAR ATF[] = 
{'A','d','m','i','n','T','o','o','l','s','F','o','l','d','e','r',0};
    static const WCHAR ADF[] = 
{'A','p','p','D','a','t','a','F','o','l','d','e','r',0};
    static const WCHAR SF[] = 
{'S','y','s','t','e','m','F','o','l','d','e','r',0};
    static const WCHAR SF16[] = 
{'S','y','s','t','e','m','1','6','F','o','l','d','e','r',0};
    static const WCHAR LADF[] = 
{'L','o','c','a','l','A','p','p','D','a','t','a','F','o','l','d','e','r',0};
    static const WCHAR MPF[] = 
{'M','y','P','i','c','t','u','r','e','s','F','o','l','d','e','r',0};
    static const WCHAR PF[] = 
{'P','e','r','s','o','n','a','l','F','o','l','d','e','r',0};
    static const WCHAR WF[] = 
{'W','i','n','d','o','w','s','F','o','l','d','e','r',0};
    static const WCHAR WV[] = 
{'W','i','n','d','o','w','s','V','o','l','u','m','e',0};
    static const WCHAR TF[]=
{'T','e','m','p','F','o','l','d','e','r',0};
    static const WCHAR szAdminUser[] =
{'A','d','m','i','n','U','s','e','r',0};
    static const WCHAR szPriv[] =
{'P','r','i','v','i','l','e','g','e','d',0};
    static const WCHAR szOne[] =
{'1',0};
    static const WCHAR v9x[] = { 'V','e','r','s','i','o','n','9','X',0 };
    static const WCHAR vNT[] = { 'V','e','r','s','i','o','n','N','T',0 };
    static const WCHAR szFormat[] = {'%','l','i',0};
    static const WCHAR szWinBuild[] =
{'W','i','n','d','o','w','s','B','u','i','l','d', 0 };
    static const WCHAR szSPL[] = 
{'S','e','r','v','i','c','e','P','a','c','k','L','e','v','e','l',0 };
    static const WCHAR szSix[] = {'6',0 };

    static const WCHAR szVersionMsi[] = { 'V','e','r','s','i','o','n','M','s','i',0 };
    static const WCHAR szPhysicalMemory[] = { 'P','h','y','s','i','c','a','l','M','e','m','o','r','y',0 };
    static const WCHAR szFormat2[] = {'%','l','i','.','%','l','i',0};
/* Screen properties */
    static const WCHAR szScreenX[] = {'S','c','r','e','e','n','X',0};
    static const WCHAR szScreenY[] = {'S','c','r','e','e','n','Y',0};
    static const WCHAR szColorBits[] = {'C','o','l','o','r','B','i','t','s',0};
    static const WCHAR szScreenFormat[] = {'%','d',0};
    static const WCHAR szIntel[] = { 'I','n','t','e','l',0 };
    static const WCHAR szAllUsers[] = { 'A','L','L','U','S','E','R','S',0 };
    static const WCHAR szCurrentVersion[] = {
        'S','O','F','T','W','A','R','E','\\',
        'M','i','c','r','o','s','o','f','t','\\',
        'W','i','n','d','o','w','s',' ','N','T','\\',
        'C','u','r','r','e','n','t','V','e','r','s','i','o','n',0
    };
    static const WCHAR szRegisteredUser[] = {'R','e','g','i','s','t','e','r','e','d','O','w','n','e','r',0};
    static const WCHAR szRegisteredOrg[] = {
        'R','e','g','i','s','t','e','r','e','d','O','r','g','a','n','i','z','a','t','i','o','n',0
    };
    static const WCHAR szUSERNAME[] = {'U','S','E','R','N','A','M','E',0};
    static const WCHAR szCOMPANYNAME[] = {'C','O','M','P','A','N','Y','N','A','M','E',0};
    SYSTEM_INFO sys_info;

    /*
     * Other things that probably should be set:
     *
     * SystemLanguageID ComputerName UserLanguageID LogonUser VirtualMemory
     * Intel ShellAdvSupport DefaultUIFont VersionDatabase PackagecodeChanging
     * ProductState CaptionHeight BorderTop BorderSide TextHeight
     * RedirectedDllSupport Time Date Privileged
     */

    SHGetFolderPathW(NULL,CSIDL_PROGRAM_FILES_COMMON,NULL,0,pth);
    strcatW(pth,cszbs);
    MSI_SetPropertyW(package, CFF, pth);

    SHGetFolderPathW(NULL,CSIDL_PROGRAM_FILES,NULL,0,pth);
    strcatW(pth,cszbs);
    MSI_SetPropertyW(package, PFF, pth);

    SHGetFolderPathW(NULL,CSIDL_COMMON_APPDATA,NULL,0,pth);
    strcatW(pth,cszbs);
    MSI_SetPropertyW(package, CADF, pth);

    SHGetFolderPathW(NULL,CSIDL_FAVORITES,NULL,0,pth);
    strcatW(pth,cszbs);
    MSI_SetPropertyW(package, FaF, pth);

    SHGetFolderPathW(NULL,CSIDL_FONTS,NULL,0,pth);
    strcatW(pth,cszbs);
    MSI_SetPropertyW(package, FoF, pth);

    SHGetFolderPathW(NULL,CSIDL_SENDTO,NULL,0,pth);
    strcatW(pth,cszbs);
    MSI_SetPropertyW(package, SendTF, pth);

    SHGetFolderPathW(NULL,CSIDL_STARTMENU,NULL,0,pth);
    strcatW(pth,cszbs);
    MSI_SetPropertyW(package, SMF, pth);

    SHGetFolderPathW(NULL,CSIDL_STARTUP,NULL,0,pth);
    strcatW(pth,cszbs);
    MSI_SetPropertyW(package, StF, pth);

    SHGetFolderPathW(NULL,CSIDL_TEMPLATES,NULL,0,pth);
    strcatW(pth,cszbs);
    MSI_SetPropertyW(package, TemplF, pth);

    SHGetFolderPathW(NULL,CSIDL_DESKTOP,NULL,0,pth);
    strcatW(pth,cszbs);
    MSI_SetPropertyW(package, DF, pth);

    SHGetFolderPathW(NULL,CSIDL_PROGRAMS,NULL,0,pth);
    strcatW(pth,cszbs);
    MSI_SetPropertyW(package, PMF, pth);

    SHGetFolderPathW(NULL,CSIDL_ADMINTOOLS,NULL,0,pth);
    strcatW(pth,cszbs);
    MSI_SetPropertyW(package, ATF, pth);

    SHGetFolderPathW(NULL,CSIDL_APPDATA,NULL,0,pth);
    strcatW(pth,cszbs);
    MSI_SetPropertyW(package, ADF, pth);

    SHGetFolderPathW(NULL,CSIDL_SYSTEM,NULL,0,pth);
    strcatW(pth,cszbs);
    MSI_SetPropertyW(package, SF, pth);
    MSI_SetPropertyW(package, SF16, pth);

    SHGetFolderPathW(NULL,CSIDL_LOCAL_APPDATA,NULL,0,pth);
    strcatW(pth,cszbs);
    MSI_SetPropertyW(package, LADF, pth);

    SHGetFolderPathW(NULL,CSIDL_MYPICTURES,NULL,0,pth);
    strcatW(pth,cszbs);
    MSI_SetPropertyW(package, MPF, pth);

    SHGetFolderPathW(NULL,CSIDL_PERSONAL,NULL,0,pth);
    strcatW(pth,cszbs);
    MSI_SetPropertyW(package, PF, pth);

    SHGetFolderPathW(NULL,CSIDL_WINDOWS,NULL,0,pth);
    strcatW(pth,cszbs);
    MSI_SetPropertyW(package, WF, pth);
    
    /* Physical Memory is specified in MB. Using total amount. */
    msex.dwLength = sizeof(msex);
    GlobalMemoryStatusEx( &msex );
    sprintfW( bufstr, szScreenFormat, (int)(msex.ullTotalPhys/1024/1024));
    MSI_SetPropertyW(package, szPhysicalMemory, bufstr);

    SHGetFolderPathW(NULL,CSIDL_WINDOWS,NULL,0,pth);
    ptr = strchrW(pth,'\\');
    if (ptr)
	*(ptr+1) = 0;
    MSI_SetPropertyW(package, WV, pth);
    
    GetTempPathW(MAX_PATH,pth);
    MSI_SetPropertyW(package, TF, pth);


    /* in a wine environment the user is always admin and privileged */
    MSI_SetPropertyW(package,szAdminUser,szOne);
    MSI_SetPropertyW(package,szPriv,szOne);
    MSI_SetPropertyW(package, szAllUsers, szOne);

    /* set the os things */
    OSVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    GetVersionExA(&OSVersion);
    verval = OSVersion.dwMinorVersion+OSVersion.dwMajorVersion*100;
    sprintfW(verstr,szFormat,verval);
    switch (OSVersion.dwPlatformId)
    {
        case VER_PLATFORM_WIN32_WINDOWS:    
            MSI_SetPropertyW(package,v9x,verstr);
            break;
        case VER_PLATFORM_WIN32_NT:
            MSI_SetPropertyW(package,vNT,verstr);
            break;
    }
    sprintfW(verstr,szFormat,OSVersion.dwBuildNumber);
    MSI_SetPropertyW(package,szWinBuild,verstr);
    /* just fudge this */
    MSI_SetPropertyW(package,szSPL,szSix);

    sprintfW( bufstr, szFormat2, MSI_MAJORVERSION, MSI_MINORVERSION);
    MSI_SetPropertyW( package, szVersionMsi, bufstr );

    GetSystemInfo( &sys_info );
    if (sys_info.u.s.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
    {
        sprintfW( bufstr, szScreenFormat, sys_info.wProcessorLevel );
        MSI_SetPropertyW( package, szIntel, bufstr );
    }

    /* Screen properties. */
    dc = GetDC(0);
    sprintfW( bufstr, szScreenFormat, GetDeviceCaps( dc, HORZRES ) );
    MSI_SetPropertyW( package, szScreenX, bufstr );
    sprintfW( bufstr, szScreenFormat, GetDeviceCaps( dc, VERTRES ));
    MSI_SetPropertyW( package, szScreenY, bufstr );
    sprintfW( bufstr, szScreenFormat, GetDeviceCaps( dc, BITSPIXEL ));
    MSI_SetPropertyW( package, szColorBits, bufstr );
    ReleaseDC(0, dc);

    /* USERNAME and COMPANYNAME */
    res = RegOpenKeyW( HKEY_LOCAL_MACHINE, szCurrentVersion, &hkey );
    if (res != ERROR_SUCCESS)
        return;

    check = msi_dup_property( package, szUSERNAME );
    if (!check)
    {
        LPWSTR user = msi_reg_get_val_str( hkey, szRegisteredUser );
        MSI_SetPropertyW( package, szUSERNAME, user );
        msi_free( user );
    }

    msi_free( check );

    check = msi_dup_property( package, szCOMPANYNAME );
    if (!check)
    {
        LPWSTR company = msi_reg_get_val_str( hkey, szRegisteredOrg );
        MSI_SetPropertyW( package, szCOMPANYNAME, company );
        msi_free( company );
    }

    msi_free( check );
    CloseHandle( hkey );
}

static UINT msi_get_word_count( MSIPACKAGE *package )
{
    UINT rc;
    INT word_count;
    MSIHANDLE suminfo;
    MSIHANDLE hdb = alloc_msihandle( &package->db->hdr );

    if (!hdb) {
        ERR("Unable to allocate handle\n");
        return 0;
    }
    rc = MsiGetSummaryInformationW( hdb, NULL, 0, &suminfo );
    MsiCloseHandle(hdb);
    if (rc != ERROR_SUCCESS)
    {
        ERR("Unable to open Summary Information\n");
        return 0;
    }

    rc = MsiSummaryInfoGetPropertyW( suminfo, PID_WORDCOUNT, NULL,
                                     &word_count, NULL, NULL, NULL );
    if (rc != ERROR_SUCCESS)
    {
        ERR("Unable to query word count\n");
        MsiCloseHandle(suminfo);
        return 0;
    }

    MsiCloseHandle(suminfo);
    return word_count;
}

MSIPACKAGE *MSI_CreatePackage( MSIDATABASE *db, LPWSTR base_url )
{
    static const WCHAR szLevel[] = { 'U','I','L','e','v','e','l',0 };
    static const WCHAR szpi[] = {'%','i',0};
    static const WCHAR szProductCode[] = {
        'P','r','o','d','u','c','t','C','o','d','e',0};
    MSIPACKAGE *package = NULL;
    WCHAR uilevel[10];
    int i;

    TRACE("%p\n", db);

    package = alloc_msiobject( MSIHANDLETYPE_PACKAGE, sizeof (MSIPACKAGE),
                               MSI_FreePackage );
    if( package )
    {
        msiobj_addref( &db->hdr );

        package->db = db;
        list_init( &package->components );
        list_init( &package->features );
        list_init( &package->files );
        list_init( &package->tempfiles );
        list_init( &package->folders );
        package->ActionFormat = NULL;
        package->LastAction = NULL;
        package->dialog = NULL;
        package->next_dialog = NULL;
        list_init( &package->subscriptions );
        list_init( &package->appids );
        list_init( &package->classes );
        list_init( &package->mimes );
        list_init( &package->extensions );
        list_init( &package->progids );
        list_init( &package->RunningActions );

        package->WordCount = msi_get_word_count( package );
        package->PackagePath = strdupW( db->path );
        package->BaseURL = strdupW( base_url );

        /* OK, here is where we do a slew of things to the database to 
         * prep for all that is to come as a package */

        for (i=0; i<PROPERTY_HASH_SIZE; i++)
            list_init( &package->props[i] );

        clone_properties( package );
        set_installer_properties(package);
        sprintfW(uilevel,szpi,gUILevel);
        MSI_SetPropertyW(package, szLevel, uilevel);

        package->ProductCode = msi_dup_property( package, szProductCode );
        set_installed_prop( package );
    }

    return package;
}

/*
 * copy_package_to_temp   [internal]
 *
 * copy the msi file to a temp file to prevent locking a CD
 * with a multi disc install 
 *
 * FIXME: I think this is wrong, and instead of copying the package,
 *        we should read all the tables to memory, then open the
 *        database to read binary streams on demand.
 */ 
static LPCWSTR copy_package_to_temp( LPCWSTR szPackage, LPWSTR filename )
{
    WCHAR path[MAX_PATH];
    static const WCHAR szMSI[] = {'M','S','I',0};

    GetTempPathW( MAX_PATH, path );
    GetTempFileNameW( path, szMSI, 0, filename );

    if( !CopyFileW( szPackage, filename, FALSE ) )
    {
        ERR("failed to copy package %s\n", debugstr_w(szPackage) );
        return szPackage;
    }

    TRACE("Opening relocated package %s\n", debugstr_w( filename ));
    return filename;
}

LPCWSTR msi_download_file( LPCWSTR szUrl, LPWSTR filename )
{
    LPINTERNET_CACHE_ENTRY_INFOW cache_entry;
    DWORD size = 0;
    HRESULT hr;

    /* call will always fail, becase size is 0,
     * but will return ERROR_FILE_NOT_FOUND first
     * if the file doesn't exist
     */
    GetUrlCacheEntryInfoW( szUrl, NULL, &size );
    if ( GetLastError() != ERROR_FILE_NOT_FOUND )
    {
        cache_entry = HeapAlloc( GetProcessHeap(), 0, size );
        if ( !GetUrlCacheEntryInfoW( szUrl, cache_entry, &size ) )
        {
            HeapFree( GetProcessHeap(), 0, cache_entry );
            return szUrl;
        }

        lstrcpyW( filename, cache_entry->lpszLocalFileName );
        HeapFree( GetProcessHeap(), 0, cache_entry );
        return filename;
    }

    hr = URLDownloadToCacheFileW( NULL, szUrl, filename, MAX_PATH, 0, NULL );
    if ( FAILED(hr) )
        return szUrl;

    return filename;
}

UINT MSI_OpenPackageW(LPCWSTR szPackage, MSIPACKAGE **pPackage)
{
    MSIDATABASE *db = NULL;
    MSIPACKAGE *package;
    MSIHANDLE handle;
    LPWSTR ptr, base_url = NULL;
    UINT r;

    static const WCHAR OriginalDatabase[] =
        {'O','r','i','g','i','n','a','l','D','a','t','a','b','a','s','e',0};
    static const WCHAR Database[] = {'D','A','T','A','B','A','S','E',0};

    TRACE("%s %p\n", debugstr_w(szPackage), pPackage);

    if( szPackage[0] == '#' )
    {
        handle = atoiW(&szPackage[1]);
        db = msihandle2msiinfo( handle, MSIHANDLETYPE_DATABASE );
        if( !db )
            return ERROR_INVALID_HANDLE;
    }
    else
    {
        WCHAR temppath[MAX_PATH];
        LPCWSTR file;

        if ( UrlIsW( szPackage, URLIS_URL ) )
        {
            file = msi_download_file( szPackage, temppath );

            base_url = strdupW( szPackage );
            if ( !base_url )
                return ERROR_OUTOFMEMORY;

            ptr = strrchrW( base_url, '/' );
            if (ptr) *(ptr + 1) = '\0';
        }
        else
            file = copy_package_to_temp( szPackage, temppath );

        r = MSI_OpenDatabaseW( file, MSIDBOPEN_READONLY, &db );

        if (file != szPackage)
            DeleteFileW( file );

        if( r != ERROR_SUCCESS )
        {
            if (GetLastError() == ERROR_FILE_NOT_FOUND)
                msi_ui_error( 4, MB_OK | MB_ICONWARNING );

            return r;
        }
    }

    package = MSI_CreatePackage( db, base_url );
    msi_free( base_url );
    msiobj_release( &db->hdr );
    if( !package )
        return ERROR_FUNCTION_FAILED;

    if( szPackage[0] != '#' )
    {
        MSI_SetPropertyW( package, OriginalDatabase, szPackage );
        MSI_SetPropertyW( package, Database, szPackage );
    }
    else
    {
        MSI_SetPropertyW( package, OriginalDatabase, db->path );
        MSI_SetPropertyW( package, Database, db->path );
    }

    *pPackage = package;

    return ERROR_SUCCESS;
}

UINT WINAPI MsiOpenPackageExW(LPCWSTR szPackage, DWORD dwOptions, MSIHANDLE *phPackage)
{
    MSIPACKAGE *package = NULL;
    UINT ret;

    TRACE("%s %08x %p\n", debugstr_w(szPackage), dwOptions, phPackage );

    if( szPackage == NULL )
        return ERROR_INVALID_PARAMETER;

    if( dwOptions )
        FIXME("dwOptions %08x not supported\n", dwOptions);

    ret = MSI_OpenPackageW( szPackage, &package );
    if( ret == ERROR_SUCCESS )
    {
        *phPackage = alloc_msihandle( &package->hdr );
        if (! *phPackage)
            ret = ERROR_NOT_ENOUGH_MEMORY;
        msiobj_release( &package->hdr );
    }

    return ret;
}

UINT WINAPI MsiOpenPackageW(LPCWSTR szPackage, MSIHANDLE *phPackage)
{
    return MsiOpenPackageExW( szPackage, 0, phPackage );
}

UINT WINAPI MsiOpenPackageExA(LPCSTR szPackage, DWORD dwOptions, MSIHANDLE *phPackage)
{
    LPWSTR szwPack = NULL;
    UINT ret;

    if( szPackage )
    {
        szwPack = strdupAtoW( szPackage );
        if( !szwPack )
            return ERROR_OUTOFMEMORY;
    }

    ret = MsiOpenPackageExW( szwPack, dwOptions, phPackage );

    msi_free( szwPack );

    return ret;
}

UINT WINAPI MsiOpenPackageA(LPCSTR szPackage, MSIHANDLE *phPackage)
{
    return MsiOpenPackageExA( szPackage, 0, phPackage );
}

MSIHANDLE WINAPI MsiGetActiveDatabase(MSIHANDLE hInstall)
{
    MSIPACKAGE *package;
    MSIHANDLE handle = 0;

    TRACE("(%ld)\n",hInstall);

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE);
    if( package)
    {
        handle = alloc_msihandle( &package->db->hdr );
        msiobj_release( &package->hdr );
    }

    return handle;
}

INT MSI_ProcessMessage( MSIPACKAGE *package, INSTALLMESSAGE eMessageType,
                               MSIRECORD *record)
{
    static const WCHAR szActionData[] =
        {'A','c','t','i','o','n','D','a','t','a',0};
    static const WCHAR szSetProgress[] =
        {'S','e','t','P','r','o','g','r','e','s','s',0};
    static const WCHAR szActionText[] =
        {'A','c','t','i','o','n','T','e','x','t',0};
    DWORD log_type = 0;
    LPWSTR message;
    DWORD sz;
    DWORD total_size = 0;
    INT i;
    INT rc;
    char *msg;
    int len;

    TRACE("%x\n", eMessageType);
    rc = 0;

    if ((eMessageType & 0xff000000) == INSTALLMESSAGE_ERROR)
        log_type |= INSTALLLOGMODE_ERROR;
    if ((eMessageType & 0xff000000) == INSTALLMESSAGE_WARNING)
        log_type |= INSTALLLOGMODE_WARNING;
    if ((eMessageType & 0xff000000) == INSTALLMESSAGE_USER)
        log_type |= INSTALLLOGMODE_USER;
    if ((eMessageType & 0xff000000) == INSTALLMESSAGE_INFO)
        log_type |= INSTALLLOGMODE_INFO;
    if ((eMessageType & 0xff000000) == INSTALLMESSAGE_COMMONDATA)
        log_type |= INSTALLLOGMODE_COMMONDATA;
    if ((eMessageType & 0xff000000) == INSTALLMESSAGE_ACTIONSTART)
        log_type |= INSTALLLOGMODE_ACTIONSTART;
    if ((eMessageType & 0xff000000) == INSTALLMESSAGE_ACTIONDATA)
        log_type |= INSTALLLOGMODE_ACTIONDATA;
    /* just a guess */
    if ((eMessageType & 0xff000000) == INSTALLMESSAGE_PROGRESS)
        log_type |= 0x800;

    if ((eMessageType & 0xff000000) == INSTALLMESSAGE_ACTIONSTART)
    {
        static const WCHAR template_s[]=
            {'A','c','t','i','o','n',' ','%','s',':',' ','%','s','.',' ',0};
        static const WCHAR format[] = 
            {'H','H','\'',':','\'','m','m','\'',':','\'','s','s',0};
        WCHAR timet[0x100];
        LPCWSTR action_text, action;
        LPWSTR deformatted = NULL;

        GetTimeFormatW(LOCALE_USER_DEFAULT, 0, NULL, format, timet, 0x100);

        action = MSI_RecordGetString(record, 1);
        action_text = MSI_RecordGetString(record, 2);

        if (!action || !action_text)
            return IDOK;

        deformat_string(package, action_text, &deformatted);

        len = strlenW(timet) + strlenW(action) + strlenW(template_s);
        if (deformatted)
            len += strlenW(deformatted);
        message = msi_alloc(len*sizeof(WCHAR));
        sprintfW(message, template_s, timet, action);
        if (deformatted)
            strcatW(message, deformatted);
        msi_free(deformatted);
    }
    else
    {
        INT msg_field=1;
        message = msi_alloc(1*sizeof (WCHAR));
        message[0]=0;
        msg_field = MSI_RecordGetFieldCount(record);
        for (i = 1; i <= msg_field; i++)
        {
            LPWSTR tmp;
            WCHAR number[3];
            static const WCHAR format[] = { '%','i',':',' ',0};
            static const WCHAR space[] = { ' ',0};
            sz = 0;
            MSI_RecordGetStringW(record,i,NULL,&sz);
            sz+=4;
            total_size+=sz*sizeof(WCHAR);
            tmp = msi_alloc(sz*sizeof(WCHAR));
            message = msi_realloc(message,total_size*sizeof (WCHAR));

            MSI_RecordGetStringW(record,i,tmp,&sz);

            if (msg_field > 1)
            {
                sprintfW(number,format,i);
                strcatW(message,number);
            }
            strcatW(message,tmp);
            if (msg_field > 1)
                strcatW(message,space);

            msi_free(tmp);
        }
    }

    TRACE("(%p %x %x %s)\n", gUIHandlerA, gUIFilter, log_type,
                             debugstr_w(message));

    /* convert it to ASCII */
    len = WideCharToMultiByte( CP_ACP, 0, message, -1,
                               NULL, 0, NULL, NULL );
    msg = msi_alloc( len );
    WideCharToMultiByte( CP_ACP, 0, message, -1,
                         msg, len, NULL, NULL );

    if (gUIHandlerA && (gUIFilter & log_type))
    {
        rc = gUIHandlerA(gUIContext,eMessageType,msg);
    }

    if ((!rc) && (gszLogFile[0]) && !((eMessageType & 0xff000000) ==
                                      INSTALLMESSAGE_PROGRESS))
    {
        DWORD write;
        HANDLE log_file = CreateFileW(gszLogFile,GENERIC_WRITE, 0, NULL,
                                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (log_file != INVALID_HANDLE_VALUE)
        {
            SetFilePointer(log_file,0, NULL, FILE_END);
            WriteFile(log_file,msg,strlen(msg),&write,NULL);
            WriteFile(log_file,"\n",1,&write,NULL);
            CloseHandle(log_file);
        }
    }
    msi_free( msg );

    msi_free( message);

    switch (eMessageType & 0xff000000)
    {
    case INSTALLMESSAGE_ACTIONDATA:
        /* FIXME: format record here instead of in ui_actiondata to get the
         * correct action data for external scripts */
        ControlEvent_FireSubscribedEvent(package, szActionData, record);
        break;
    case INSTALLMESSAGE_ACTIONSTART:
    {
        MSIRECORD *uirow;
        LPWSTR deformated;
        LPCWSTR action_text = MSI_RecordGetString(record, 2);

        deformat_string(package, action_text, &deformated);
        uirow = MSI_CreateRecord(1);
        MSI_RecordSetStringW(uirow, 1, deformated);
        TRACE("INSTALLMESSAGE_ACTIONSTART: %s\n", debugstr_w(deformated));
        msi_free(deformated);

        ControlEvent_FireSubscribedEvent(package, szActionText, uirow);

        msiobj_release(&uirow->hdr);
        break;
    }
    case INSTALLMESSAGE_PROGRESS:
        ControlEvent_FireSubscribedEvent(package, szSetProgress, record);
        break;
    }

    return ERROR_SUCCESS;
}

INT WINAPI MsiProcessMessage( MSIHANDLE hInstall, INSTALLMESSAGE eMessageType,
                              MSIHANDLE hRecord)
{
    UINT ret = ERROR_INVALID_HANDLE;
    MSIPACKAGE *package = NULL;
    MSIRECORD *record = NULL;

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE );
    if( !package )
        return ERROR_INVALID_HANDLE;

    record = msihandle2msiinfo( hRecord, MSIHANDLETYPE_RECORD );
    if( !record )
        goto out;

    ret = MSI_ProcessMessage( package, eMessageType, record );

out:
    msiobj_release( &package->hdr );
    if( record )
        msiobj_release( &record->hdr );

    return ret;
}

/* property code */

typedef struct msi_property {
    struct list entry;
    LPWSTR key;
    LPWSTR value;
} msi_property;

static UINT msi_prop_makehash( const WCHAR *str )
{
    UINT hash = 0;

    if (str==NULL)
        return hash;

    while( *str )
    {
        hash ^= *str++;
        hash *= 53;
        hash = (hash<<5) | (hash>>27);
    }
    return hash % PROPERTY_HASH_SIZE;
}

static msi_property *msi_prop_find( MSIPACKAGE *package, LPCWSTR key )
{
    UINT hash = msi_prop_makehash( key );
    msi_property *prop;

    LIST_FOR_EACH_ENTRY( prop, &package->props[hash], msi_property, entry )
        if (!lstrcmpW( prop->key, key ))
            return prop;
    return NULL;
}

static msi_property *msi_prop_add( MSIPACKAGE *package, LPCWSTR key )
{
    UINT hash = msi_prop_makehash( key );
    msi_property *prop;

    prop = msi_alloc( sizeof *prop );
    if (prop)
    {
        prop->key = strdupW( key );
        prop->value = NULL;
        list_add_head( &package->props[hash], &prop->entry );
    }
    return prop;
}

static void msi_delete_property( msi_property *prop )
{
    list_remove( &prop->entry );
    msi_free( prop->key );
    msi_free( prop->value );
    msi_free( prop );
}

static void msi_free_properties( MSIPACKAGE *package )
{
    int i;

    for ( i=0; i<PROPERTY_HASH_SIZE; i++ )
    {
        while ( !list_empty(&package->props[i]) )
        {
            msi_property *prop;
            prop = LIST_ENTRY( list_head( &package->props[i] ),
                               msi_property, entry );
            msi_delete_property( prop );
        }
    }
}

UINT WINAPI MsiSetPropertyA( MSIHANDLE hInstall, LPCSTR szName, LPCSTR szValue )
{
    LPWSTR szwName = NULL, szwValue = NULL;
    UINT r = ERROR_OUTOFMEMORY;

    szwName = strdupAtoW( szName );
    if( szName && !szwName )
        goto end;

    szwValue = strdupAtoW( szValue );
    if( szValue && !szwValue )
        goto end;

    r = MsiSetPropertyW( hInstall, szwName, szwValue);

end:
    msi_free( szwName );
    msi_free( szwValue );

    return r;
}

UINT MSI_SetPropertyW( MSIPACKAGE *package, LPCWSTR szName, LPCWSTR szValue)
{
    msi_property *prop;

    TRACE("%p %s %s\n", package, debugstr_w(szName), debugstr_w(szValue));

    if (!szName)
        return ERROR_INVALID_PARAMETER;

    /* this one is weird... */
    if (!szName[0])
        return szValue ? ERROR_FUNCTION_FAILED : ERROR_SUCCESS;

    prop = msi_prop_find( package, szName );
    if (!prop)
        prop = msi_prop_add( package, szName );

    if (!prop)
        return ERROR_OUTOFMEMORY;

    if (szValue)
    {
        msi_free( prop->value );
        prop->value = strdupW( szValue );
    }
    else
        msi_delete_property( prop );

    return ERROR_SUCCESS;
}

UINT WINAPI MsiSetPropertyW( MSIHANDLE hInstall, LPCWSTR szName, LPCWSTR szValue)
{
    MSIPACKAGE *package;
    UINT ret;

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE);
    if( !package )
        return ERROR_INVALID_HANDLE;
    ret = MSI_SetPropertyW( package, szName, szValue);
    msiobj_release( &package->hdr );
    return ret;
}

/* internal function, not compatible with MsiGetPropertyW */
UINT MSI_GetPropertyW( MSIPACKAGE *package, LPCWSTR szName, 
                       LPWSTR szValueBuf, DWORD* pchValueBuf )
{
    msi_property *prop;
    UINT r, len;

    if (*pchValueBuf > 0)
        szValueBuf[0] = 0;

    prop = msi_prop_find( package, szName );
    if (!prop)
    {
        *pchValueBuf = 0;
        TRACE("property %s not found\n", debugstr_w(szName));
        return ERROR_FUNCTION_FAILED;
    }

    if (prop->value)
    {
        len = lstrlenW( prop->value );
        lstrcpynW(szValueBuf, prop->value, *pchValueBuf);
    }
    else
    {
        len = 1;
        if( *pchValueBuf > 0 )
            szValueBuf[0] = 0;
    }

    TRACE("%s -> %s\n", debugstr_w(szName), debugstr_w(szValueBuf));

    if ( *pchValueBuf <= len )
    {
        TRACE("have %u, need %u -> ERROR_MORE_DATA\n", *pchValueBuf, len);
        r = ERROR_MORE_DATA;
    }
    else
        r = ERROR_SUCCESS;

    *pchValueBuf = len;

    return r;
}

LPWSTR msi_dup_property( MSIPACKAGE *package, LPCWSTR szName )
{
    msi_property *prop;
    LPWSTR value = NULL;

    prop = msi_prop_find( package, szName );
    if (prop)
        value = strdupW( prop->value );

    return value;
}

int msi_get_property_int( MSIPACKAGE *package, LPCWSTR name, int value )
{
    msi_property *prop;

    prop = msi_prop_find( package, name );
    if (prop)
        value = atoiW( prop->value );
    return value;
}

static UINT MSI_GetProperty( MSIHANDLE handle, LPCWSTR name,
                             awstring *szValueBuf, DWORD* pchValueBuf )
{
    static const WCHAR empty[] = {0};
    msi_property *prop;
    MSIPACKAGE *package;
    UINT r;
    LPCWSTR val = NULL;

    TRACE("%lu %s %p %p\n", handle, debugstr_w(name),
          szValueBuf->str.w, pchValueBuf );

    if (!name)
        return ERROR_INVALID_PARAMETER;

    package = msihandle2msiinfo( handle, MSIHANDLETYPE_PACKAGE );
    if (!package)
        return ERROR_INVALID_HANDLE;

    prop = msi_prop_find( package, name );
    if (prop)
        val = prop->value;

    if (!val)
        val = empty;

    r = msi_strcpy_to_awstring( val, szValueBuf, pchValueBuf );

    msiobj_release( &package->hdr );

    return r;
}

UINT WINAPI MsiGetPropertyA( MSIHANDLE hInstall, LPCSTR szName,
                             LPSTR szValueBuf, DWORD* pchValueBuf )
{
    awstring val;
    LPWSTR name;
    UINT r;

    val.unicode = FALSE;
    val.str.a = szValueBuf;

    name = strdupAtoW( szName );
    if (szName && !name)
        return ERROR_OUTOFMEMORY;

    r = MSI_GetProperty( hInstall, name, &val, pchValueBuf );
    msi_free( name );
    return r;
}

UINT WINAPI MsiGetPropertyW( MSIHANDLE hInstall, LPCWSTR szName,
                             LPWSTR szValueBuf, DWORD* pchValueBuf )
{
    awstring val;

    val.unicode = TRUE;
    val.str.w = szValueBuf;

    return MSI_GetProperty( hInstall, szName, &val, pchValueBuf );
}
