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

#include "msipriv.h"
#include "action.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

static void MSI_FreePackage( MSIOBJECTHDR *arg)
{
    MSIPACKAGE *package= (MSIPACKAGE*) arg;

    if( package->dialog )
        msi_dialog_destroy( package->dialog );
    ACTION_free_package_structures(package);

    msiobj_release( &package->db->hdr );
}

static UINT clone_properties(MSIDATABASE *db)
{
    MSIQUERY * view = NULL;
    UINT rc;
    static const WCHAR CreateSql[] = {
       'C','R','E','A','T','E',' ','T','A','B','L','E',' ','`','_','P','r','o',
       'p','e','r','t','y','`',' ','(',' ','`','_','P','r','o','p','e','r','t',
       'y','`',' ','C','H','A','R','(','5','6',')',' ','N','O','T',' ','N','U',
       'L','L',',',' ','`','V','a','l','u','e','`',' ','C','H','A','R','(','9',
       '8',')',' ','N','O','T',' ','N','U','L','L',' ','P','R','I','M','A','R',
       'Y',' ','K','E','Y',' ','`','_','P','r','o','p','e','r','t','y','`',')',0};
    static const WCHAR Query[] = {
       'S','E','L','E','C','T',' ','*',' ',
       'F','R','O','M',' ','`','P','r','o','p','e','r','t','y','`',0};
    static const WCHAR Insert[] = {
       'I','N','S','E','R','T',' ','i','n','t','o',' ',
       '`','_','P','r','o','p','e','r','t','y','`',' ',
       '(','`','_','P','r','o','p','e','r','t','y','`',',',
       '`','V','a','l','u','e','`',')',' ',
       'V','A','L','U','E','S',' ','(','?',',','?',')',0};

    /* create the temporary properties table */
    rc = MSI_DatabaseOpenViewW(db, CreateSql, &view);
    if (rc != ERROR_SUCCESS)
        return rc;
    rc = MSI_ViewExecute(view,0);   
    MSI_ViewClose(view);
    msiobj_release(&view->hdr); 
    if (rc != ERROR_SUCCESS)
        return rc;

    /* clone the existing properties */
    rc = MSI_DatabaseOpenViewW(db, Query, &view);
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
        MSIQUERY * view2;

        rc = MSI_ViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
            break;

        rc = MSI_DatabaseOpenViewW(db,Insert,&view2);  
        if (rc!= ERROR_SUCCESS)
            continue;
        rc = MSI_ViewExecute(view2,row);
        MSI_ViewClose(view2);
        msiobj_release(&view2->hdr);
 
        if (rc == ERROR_SUCCESS) 
            msiobj_release(&row->hdr); 
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
}

MSIPACKAGE *MSI_CreatePackage( MSIDATABASE *db )
{
    static const WCHAR szLevel[] = { 'U','I','L','e','v','e','l',0 };
    static const WCHAR szpi[] = {'%','i',0};
    static const WCHAR szProductCode[] = {
        'P','r','o','d','u','c','t','C','o','d','e',0};
    MSIPACKAGE *package = NULL;
    WCHAR uilevel[10];

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

        /* OK, here is where we do a slew of things to the database to 
         * prep for all that is to come as a package */

        clone_properties(db);
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

static LPCWSTR msi_download_package( LPCWSTR szUrl, LPWSTR filename )
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
    UINT r;

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
            file = msi_download_package( szPackage, temppath );
        else
            file = copy_package_to_temp( szPackage, temppath );

        r = MSI_OpenDatabaseW( file, MSIDBOPEN_READONLY, &db );

        if (file != szPackage)
            DeleteFileW( file );

        if( r != ERROR_SUCCESS )
            return r;
    }

    package = MSI_CreatePackage( db );
    msiobj_release( &db->hdr );
    if( !package )
        return ERROR_FUNCTION_FAILED;

    /* 
     * FIXME:  I don't think this is right.  Maybe we should be storing the
     * name of the database in the MSIDATABASE structure and fetching this
     * info from there, or maybe this is only relevant to cached databases.
     */
    if( szPackage[0] != '#' )
    {
        static const WCHAR OriginalDatabase[] =
          {'O','r','i','g','i','n','a','l','D','a','t','a','b','a','s','e',0};
        static const WCHAR Database[] = {'D','A','T','A','B','A','S','E',0};

        MSI_SetPropertyW( package, OriginalDatabase, szPackage );
        MSI_SetPropertyW( package, Database, szPackage );
    }

    *pPackage = package;

    return ERROR_SUCCESS;
}

UINT WINAPI MsiOpenPackageExW(LPCWSTR szPackage, DWORD dwOptions, MSIHANDLE *phPackage)
{
    MSIPACKAGE *package = NULL;
    UINT ret;

    TRACE("%s %08lx %p\n", debugstr_w(szPackage), dwOptions, phPackage );

    if( szPackage == NULL )
        return ERROR_INVALID_PARAMETER;

    if( dwOptions )
        FIXME("dwOptions %08lx not supported\n", dwOptions);

    ret = MSI_OpenPackageW( szPackage, &package );
    if( ret == ERROR_SUCCESS )
    {
        *phPackage = alloc_msihandle( &package->hdr );
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

    TRACE("(%p %lx %lx %s)\n",gUIHandlerA, gUIFilter, log_type,
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
    MSIQUERY *view;
    MSIRECORD *row;
    UINT rc;
    DWORD sz = 0;
    static const WCHAR Insert[]=
     {'I','N','S','E','R','T',' ','i','n','t','o',' ','`','_','P','r','o','p'
,'e','r','t','y','`',' ','(','`','_','P','r','o','p','e','r','t','y','`'
,',','`','V','a','l','u','e','`',')',' ','V','A','L','U','E','S'
,' ','(','?',',','?',')',0};
    static const WCHAR Update[]=
     {'U','P','D','A','T','E',' ','_','P','r','o','p','e'
,'r','t','y',' ','s','e','t',' ','`','V','a','l','u','e','`',' ','='
,' ','?',' ','w','h','e','r','e',' ','`','_','P','r','o','p'
,'e','r','t','y','`',' ','=',' ','\'','%','s','\'',0};
    WCHAR Query[1024];

    TRACE("%p %s %s\n", package, debugstr_w(szName), debugstr_w(szValue));

    if (!szName)
        return ERROR_INVALID_PARAMETER;

    /* this one is weird... */
    if (!szName[0])
        return szValue ? ERROR_FUNCTION_FAILED : ERROR_SUCCESS;

    rc = MSI_GetPropertyW(package,szName,0,&sz);
    if (rc==ERROR_MORE_DATA || rc == ERROR_SUCCESS)
    {
        sprintfW(Query,Update,szName);

        row = MSI_CreateRecord(1);
        MSI_RecordSetStringW(row,1,szValue);

    }
    else
    {
        strcpyW(Query,Insert);

        row = MSI_CreateRecord(2);
        MSI_RecordSetStringW(row,1,szName);
        MSI_RecordSetStringW(row,2,szValue);
    }

    rc = MSI_DatabaseOpenViewW(package->db,Query,&view);
    if (rc == ERROR_SUCCESS)
    {
        rc = MSI_ViewExecute(view,row);

        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
    }
    msiobj_release(&row->hdr);

    return rc;
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

static MSIRECORD *MSI_GetPropertyRow( MSIPACKAGE *package, LPCWSTR name )
{
    static const WCHAR query[]=
    {'S','E','L','E','C','T',' ','`','V','a','l','u','e','`',' ',
     'F','R','O','M',' ' ,'`','_','P','r','o','p','e','r','t','y','`',
     ' ','W','H','E','R','E',' ' ,'`','_','P','r','o','p','e','r','t','y','`',
     '=','\'','%','s','\'',0};

    if (!name || !name[0])
        return NULL;

    return MSI_QueryGetRecord( package->db, query, name );
}
 
/* internal function, not compatible with MsiGetPropertyW */
UINT MSI_GetPropertyW( MSIPACKAGE *package, LPCWSTR szName, 
                       LPWSTR szValueBuf, DWORD* pchValueBuf )
{
    MSIRECORD *row;
    UINT rc = ERROR_FUNCTION_FAILED;

    row = MSI_GetPropertyRow( package, szName );

    if (*pchValueBuf > 0)
        szValueBuf[0] = 0;

    if (row)
    {
        rc = MSI_RecordGetStringW(row,1,szValueBuf,pchValueBuf);
        msiobj_release(&row->hdr);
    }

    if (rc == ERROR_SUCCESS)
        TRACE("returning %s for property %s\n", debugstr_w(szValueBuf),
            debugstr_w(szName));
    else if (rc == ERROR_MORE_DATA)
        TRACE("need %li sized buffer for %s\n", *pchValueBuf,
            debugstr_w(szName));
    else
    {
        *pchValueBuf = 0;
        TRACE("property %s not found\n", debugstr_w(szName));
    }

    return rc;
}

static UINT MSI_GetProperty( MSIHANDLE handle, LPCWSTR name, 
                             awstring *szValueBuf, DWORD* pchValueBuf )
{
    static const WCHAR empty[] = {0};
    MSIPACKAGE *package;
    MSIRECORD *row = NULL;
    UINT r;
    LPCWSTR val = NULL;

    TRACE("%lu %s %p %p\n", handle, debugstr_w(name),
          szValueBuf->str.w, pchValueBuf );

    if (!name)
        return ERROR_INVALID_PARAMETER;

    package = msihandle2msiinfo( handle, MSIHANDLETYPE_PACKAGE );
    if (!package)
        return ERROR_INVALID_HANDLE;

    row = MSI_GetPropertyRow( package, name );
    if (row)
        val = MSI_RecordGetString( row, 1 );

    if (!val)
        val = empty;

    r = msi_strcpy_to_awstring( val, szValueBuf, pchValueBuf );

    if (row)
        msiobj_release( &row->hdr );
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
