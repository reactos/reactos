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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define NONAMELESSUNION

#include <stdarg.h>
#include <stdio.h>
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
#include "winuser.h"
#include "shlobj.h"
#include "wine/unicode.h"
#include "objbase.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

/*
 * The MSVC headers define the MSIDBOPEN_* macros cast to LPCTSTR,
 *  which is a problem because LPCTSTR isn't defined when compiling wine.
 * To work around this problem, we need to define LPCTSTR as LPCWSTR here,
 *  and make sure to only use it in W functions.
 */
#define LPCTSTR LPCWSTR

void MSI_FreePackage( MSIOBJECTHDR *arg)
{
    MSIPACKAGE *package= (MSIPACKAGE*) arg;

    ACTION_remove_tracked_tempfiles(package);

    if (package->features && package->loaded_features > 0)
        HeapFree(GetProcessHeap(),0,package->features);

    if (package->folders && package->loaded_folders > 0)
        HeapFree(GetProcessHeap(),0,package->folders);
    
    if (package->components && package->loaded_components > 0)
        HeapFree(GetProcessHeap(),0,package->components);

    if (package->files && package->loaded_files > 0)
        HeapFree(GetProcessHeap(),0,package->files);
    msiobj_release( &package->db->hdr );
}

UINT WINAPI MsiOpenPackageA(LPCSTR szPackage, MSIHANDLE *phPackage)
{
    LPWSTR szwPack = NULL;
    UINT len, ret;

    TRACE("%s %p\n",debugstr_a(szPackage), phPackage);

    if( szPackage )
    {
        len = MultiByteToWideChar( CP_ACP, 0, szPackage, -1, NULL, 0 );
        szwPack = HeapAlloc( GetProcessHeap(), 0, len * sizeof (WCHAR) );
        if( szwPack )
            MultiByteToWideChar( CP_ACP, 0, szPackage, -1, szwPack, len );
    }

    ret = MsiOpenPackageW( szwPack, phPackage );

    if( szwPack )
        HeapFree( GetProcessHeap(), 0, szwPack );

    return ret;
}


static const UINT clone_properties(MSIDATABASE *db)
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
       'f','r','o','m',' ','P','r','o','p','e','r','t','y',0};
    static const WCHAR Insert[] = {
       'I','N','S','E','R','T',' ','i','n','t','o',' ',
       '`','_','P','r','o','p','e','r','t','y','`',' ',
       '(','`','_','P','r','o','p','e','r','t','y','`',',',
       '`','V','a','l','u','e','`',')',' ',
       'V','A','L','U','E','S',' ','(','?',')',0};

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
 * There are a whole slew of these we need to set
 *
 *
http://msdn.microsoft.com/library/default.asp?url=/library/en-us/msi/setup/properties.asp
 */
static VOID set_installer_properties(MSIPACKAGE *package)
{
    WCHAR pth[MAX_PATH];
    OSVERSIONINFOA OSVersion;
    DWORD verval;
    WCHAR verstr[10], msiver[10];

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
    static const WCHAR LADF[] = 
{'L','o','c','a','l','A','p','p','D','a','t','a','F','o','l','d','e','r',0};
    static const WCHAR MPF[] = 
{'M','y','P','i','c','t','u','r','e','s','F','o','l','d','e','r',0};
    static const WCHAR PF[] = 
{'P','e','r','s','o','n','a','l','F','o','l','d','e','r',0};
    static const WCHAR WF[] = 
{'W','i','n','d','o','w','s','F','o','l','d','e','r',0};
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
    static const WCHAR szFormat2[] = {'%','l','i','.','%','l','i',0};

/*
 * Other things I notice set
 *
ScreenY
ScreenX
SystemLanguageID
ComputerName
UserLanguageID
LogonUser
VirtualMemory
PhysicalMemory
Intel
ShellAdvSupport
DefaultUIFont
VersionDatabase
PackagecodeChanging
ProductState
CaptionHeight
BorderTop
BorderSide
TextHeight
ColorBits
RedirectedDllSupport
Time
Date
Privileged
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

    sprintfW( msiver, szFormat2, MSI_MAJORVERSION, MSI_MINORVERSION);
    MSI_SetPropertyW( package, szVersionMsi, msiver );
}

UINT MSI_OpenPackageW(LPCWSTR szPackage, MSIPACKAGE **pPackage)
{
    UINT rc;
    MSIDATABASE *db = NULL;
    MSIPACKAGE *package = NULL;
    WCHAR uilevel[10];
    UINT ret = ERROR_FUNCTION_FAILED;

    static const WCHAR OriginalDatabase[] =
{'O','r','i','g','i','n','a','l','D','a','t','a','b','a','s','e',0};
    static const WCHAR Database[] =
{'D','A','T','A','B','A','S','E',0};
    static const WCHAR szpi[] = {'%','i',0};
    static const WCHAR szLevel[] = { 'U','I','L','e','v','e','l',0 };

    TRACE("%s %p\n",debugstr_w(szPackage), pPackage);

    if (szPackage[0] == '#')
    {
        INT handle = atoiW(&szPackage[1]);
        db = msihandle2msiinfo( handle , MSIHANDLETYPE_DATABASE);
    }
    else
    {
        rc = MSI_OpenDatabaseW(szPackage, MSIDBOPEN_READONLY, &db);
        if (rc != ERROR_SUCCESS)
            return ERROR_FUNCTION_FAILED;
    }

    package = alloc_msiobject( MSIHANDLETYPE_PACKAGE, sizeof (MSIPACKAGE),
                               MSI_FreePackage );

    if (package)
    {
        msiobj_addref( &db->hdr );

        package->db = db;
        package->features = NULL;
        package->folders = NULL;
        package->components = NULL;
        package->files = NULL;
        package->loaded_features = 0;
        package->loaded_folders = 0;
        package->loaded_components= 0;
        package->loaded_files = 0;

        /* OK, here is where we do a slew of things to the database to 
         * prep for all that is to come as a package */

        clone_properties(db);
        set_installer_properties(package);
        MSI_SetPropertyW(package, OriginalDatabase, szPackage);
        MSI_SetPropertyW(package, Database, szPackage);
        sprintfW(uilevel,szpi,gUILevel);
        MSI_SetPropertyW(package, szLevel, uilevel);

        msiobj_addref( &package->hdr );
        *pPackage = package;
        ret = ERROR_SUCCESS;
    }

    if( package )
        msiobj_release( &package->hdr );
    if( db )
        msiobj_release( &db->hdr );

    return ret;
}

UINT WINAPI MsiOpenPackageW(LPCWSTR szPackage, MSIHANDLE *phPackage)
{
    MSIPACKAGE *package = NULL;
    UINT ret;

    ret = MSI_OpenPackageW( szPackage, &package);
    if( ret == ERROR_SUCCESS )
    {
        *phPackage = alloc_msihandle( &package->hdr );
        msiobj_release( &package->hdr );
    }
    return ret;
}

UINT WINAPI MsiOpenPackageExA(LPCSTR szPackage, DWORD dwOptions, MSIHANDLE *phPackage)
{
    FIXME("%s 0x%08lx %p\n",debugstr_a(szPackage), dwOptions, phPackage);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

UINT WINAPI MsiOpenPackageExW(LPCWSTR szPackage, DWORD dwOptions, MSIHANDLE *phPackage)
{
    FIXME("%s 0x%08lx %p\n",debugstr_w(szPackage), dwOptions, phPackage);
    return ERROR_CALL_NOT_IMPLEMENTED;
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
    DWORD log_type = 0;
    LPWSTR message;
    DWORD sz;
    DWORD total_size = 0;
    INT msg_field=1;
    INT i;
    INT rc;
    char *msg;
    int len;

    TRACE("%x \n",eMessageType);
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

    message = HeapAlloc(GetProcessHeap(),0,1*sizeof (WCHAR));
    message[0]=0;
    msg_field = MSI_RecordGetFieldCount(record);
    for (i = 1; i <= msg_field; i++)
    {
        LPWSTR tmp;
        WCHAR number[3];
        const static WCHAR format[] = { '%','i',':',' ',0};
        const static WCHAR space[] = { ' ',0};
        sz = 0;
        MSI_RecordGetStringW(record,i,NULL,&sz);
        sz+=4;
        total_size+=sz*sizeof(WCHAR);
        tmp = HeapAlloc(GetProcessHeap(),0,sz*sizeof(WCHAR));
        message = HeapReAlloc(GetProcessHeap(),0,message,total_size*sizeof (WCHAR));

        MSI_RecordGetStringW(record,i,tmp,&sz);

        if (msg_field > 1)
        {
            sprintfW(number,format,i);
            strcatW(message,number);
        }
        strcatW(message,tmp);
        if (msg_field > 1)
            strcatW(message,space);

        HeapFree(GetProcessHeap(),0,tmp);
    }

    TRACE("(%p %lx %lx %s)\n",gUIHandler, gUIFilter, log_type,
                             debugstr_w(message));

    /* convert it to ASCII */
    len = WideCharToMultiByte( CP_ACP, 0, message, -1,
                               NULL, 0, NULL, NULL );
    msg = HeapAlloc( GetProcessHeap(), 0, len );
    WideCharToMultiByte( CP_ACP, 0, message, -1,
                         msg, len, NULL, NULL );

    if (gUIHandler && (gUIFilter & log_type))
    {
        rc = gUIHandler(gUIContext,eMessageType,msg);
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
    HeapFree( GetProcessHeap(), 0, msg );
    
    HeapFree(GetProcessHeap(),0,message);
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
        goto out;

    record = msihandle2msiinfo( hRecord, MSIHANDLETYPE_RECORD );
    if( !record )
        goto out;

    ret = MSI_ProcessMessage( package, eMessageType, record );

out:
    if( package )
        msiobj_release( &package->hdr );
    if( record )
        msiobj_release( &record->hdr );

    return ret;
}

/* property code */
UINT WINAPI MsiSetPropertyA( MSIHANDLE hInstall, LPCSTR szName, LPCSTR szValue)
{
    LPWSTR szwName = NULL, szwValue = NULL;
    UINT hr = ERROR_INSTALL_FAILURE;
    UINT len;

    if (0 == hInstall) {
      return ERROR_INVALID_HANDLE;
    }
    if (NULL == szName) {
      return ERROR_INVALID_PARAMETER;
    }
    if (NULL == szValue) {
      return ERROR_INVALID_PARAMETER;
    }

    len = MultiByteToWideChar( CP_ACP, 0, szName, -1, NULL, 0 );
    szwName = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
    if( !szwName )
        goto end;
    MultiByteToWideChar( CP_ACP, 0, szName, -1, szwName, len );

    len = MultiByteToWideChar( CP_ACP, 0, szValue, -1, NULL, 0 );
    szwValue = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
    if( !szwValue)
        goto end;
    MultiByteToWideChar( CP_ACP, 0, szValue , -1, szwValue, len );

    hr = MsiSetPropertyW( hInstall, szwName, szwValue);

end:
    if( szwName )
        HeapFree( GetProcessHeap(), 0, szwName );
    if( szwValue )
        HeapFree( GetProcessHeap(), 0, szwValue );

    return hr;
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
,' ','(','?',')',0};
    static const WCHAR Update[]=
     {'U','P','D','A','T','E',' ','_','P','r','o','p','e'
,'r','t','y',' ','s','e','t',' ','`','V','a','l','u','e','`',' ','='
,' ','?',' ','w','h','e','r','e',' ','`','_','P','r','o','p'
,'e','r','t','y','`',' ','=',' ','\'','%','s','\'',0};
    WCHAR Query[1024];

    TRACE("Setting property (%s %s)\n",debugstr_w(szName),
          debugstr_w(szValue));

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
    if (rc!= ERROR_SUCCESS)
    {
        msiobj_release(&row->hdr);
        return rc;
    }

    rc = MSI_ViewExecute(view,row);

    msiobj_release(&row->hdr);
    MSI_ViewClose(view);
    msiobj_release(&view->hdr);

    return rc;
}

UINT WINAPI MsiSetPropertyW( MSIHANDLE hInstall, LPCWSTR szName, LPCWSTR szValue)
{
    MSIPACKAGE *package;
    UINT ret;

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE);
    if( !package)
        return ERROR_INVALID_HANDLE;
    ret = MSI_SetPropertyW( package, szName, szValue);
    msiobj_release( &package->hdr );
    return ret;
}

static UINT MSI_GetPropertyRow(MSIPACKAGE *package, LPCWSTR szName, MSIRECORD **row)
{
    MSIQUERY *view;
    UINT rc, sz;
    static const WCHAR select[]=
    {'s','e','l','e','c','t',' ','V','a','l','u','e',' ','f','r','o','m',' '
     ,'_','P','r','o','p','e','r','t','y',' ','w','h','e','r','e',' '
     ,'_','P','r','o','p','e','r','t','y','=','`','%','s','`',0};
    LPWSTR query;

    if (!szName)
        return ERROR_INVALID_PARAMETER;

    sz = sizeof select + strlenW(szName)*sizeof(WCHAR);
    query = HeapAlloc(GetProcessHeap(), 0, sz);
    sprintfW(query,select,szName);

    rc = MSI_DatabaseOpenViewW(package->db, query, &view);
    HeapFree(GetProcessHeap(), 0, query);
    if (rc == ERROR_SUCCESS)
    {
        rc = MSI_ViewExecute(view, 0);
        if (rc == ERROR_SUCCESS)
            rc = MSI_ViewFetch(view,row);

        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
    }

    return rc;
}

UINT MSI_GetPropertyW(MSIPACKAGE *package, LPCWSTR szName, 
                           LPWSTR szValueBuf, DWORD* pchValueBuf)
{
    MSIRECORD *row;
    UINT rc;

    rc = MSI_GetPropertyRow(package, szName, &row);
    if (rc == ERROR_SUCCESS)
    {
        rc = MSI_RecordGetStringW(row,1,szValueBuf,pchValueBuf);
        msiobj_release(&row->hdr);
    }

    if (rc == ERROR_SUCCESS)
        TRACE("returning %s for property %s\n", debugstr_w(szValueBuf),
            debugstr_w(szName));
    else
    {
        *pchValueBuf = 0;
        TRACE("property not found\n");
    }

    return rc;
}

UINT MSI_GetPropertyA(MSIPACKAGE *package, LPCSTR szName, 
                           LPSTR szValueBuf, DWORD* pchValueBuf)
{
    MSIRECORD *row;
    UINT rc, len;
    LPWSTR szwName;

    len = MultiByteToWideChar( CP_ACP, 0, szName, -1, NULL, 0 );
    szwName = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
    if (!szwName)
        return ERROR_NOT_ENOUGH_MEMORY;
    MultiByteToWideChar( CP_ACP, 0, szName, -1, szwName, len );

    rc = MSI_GetPropertyRow(package, szwName, &row);
    if (rc == ERROR_SUCCESS)
    {
        rc = MSI_RecordGetStringA(row,1,szValueBuf,pchValueBuf);
        msiobj_release(&row->hdr);
    }

    if (rc == ERROR_SUCCESS)
        TRACE("returning %s for property %s\n", debugstr_a(szValueBuf),
            debugstr_a(szName));
    else
    {
        *pchValueBuf = 0;
        TRACE("property not found\n");
    }
    HeapFree( GetProcessHeap(), 0, szwName );

    return rc;
}

UINT WINAPI MsiGetPropertyA(MSIHANDLE hInstall, LPCSTR szName, LPSTR szValueBuf, DWORD* pchValueBuf) 
{
    MSIPACKAGE *package;
    UINT ret;

    TRACE("%lu %s %lu\n", hInstall, debugstr_a(szName), *pchValueBuf);

    if (0 == hInstall)
        return ERROR_INVALID_HANDLE;
    if (NULL == szName)
        return ERROR_INVALID_PARAMETER;
    if (NULL != szValueBuf && NULL == pchValueBuf)
        return ERROR_INVALID_PARAMETER;

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE);
    if (!package)
        return ERROR_INVALID_HANDLE;
    ret = MSI_GetPropertyA(package, szName, szValueBuf, pchValueBuf );
    msiobj_release( &package->hdr );
    return ret;
}

  
UINT WINAPI MsiGetPropertyW(MSIHANDLE hInstall, LPCWSTR szName, 
                           LPWSTR szValueBuf, DWORD* pchValueBuf)
{
    MSIPACKAGE *package;
    UINT ret;

    if (0 == hInstall)
        return ERROR_INVALID_HANDLE;
    if (NULL == szName)
        return ERROR_INVALID_PARAMETER;
    if (NULL != szValueBuf && NULL == pchValueBuf)
        return ERROR_INVALID_PARAMETER;

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE);
    if (!package)
        return ERROR_INVALID_HANDLE;
    ret = MSI_GetPropertyW(package, szName, szValueBuf, pchValueBuf );
    msiobj_release( &package->hdr );
    return ret;
}
