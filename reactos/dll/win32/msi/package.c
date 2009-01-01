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
#define COBJMACROS

#include <stdarg.h>
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
#include "winver.h"
#include "urlmon.h"
#include "shlobj.h"
#include "wine/unicode.h"
#include "objbase.h"
#include "msidefs.h"
#include "sddl.h"

#include "msipriv.h"
#include "msiserver.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

static void MSI_FreePackage( MSIOBJECTHDR *arg)
{
    MSIPACKAGE *package= (MSIPACKAGE*) arg;

    if( package->dialog )
        msi_dialog_destroy( package->dialog );

    msiobj_release( &package->db->hdr );
    ACTION_free_package_structures(package);
}

static UINT create_temp_property_table(MSIPACKAGE *package)
{
    MSIQUERY *view = NULL;
    UINT rc;

    static const WCHAR CreateSql[] = {
       'C','R','E','A','T','E',' ','T','A','B','L','E',' ','`','_','P','r','o',
       'p','e','r','t','y','`',' ','(',' ','`','_','P','r','o','p','e','r','t',
       'y','`',' ','C','H','A','R','(','5','6',')',' ','N','O','T',' ','N','U',
       'L','L',' ','T','E','M','P','O','R','A','R','Y',',',' ','`','V','a','l',
       'u','e','`',' ','C','H','A','R','(','9','8',')',' ','N','O','T',' ','N',
       'U','L','L',' ','T','E','M','P','O','R','A','R','Y',' ','P','R','I','M',
       'A','R','Y',' ','K','E','Y',' ','`','_','P','r','o','p','e','r','t','y',
        '`',')',0};

    rc = MSI_DatabaseOpenViewW(package->db, CreateSql, &view);
    if (rc != ERROR_SUCCESS)
        return rc;

    rc = MSI_ViewExecute(view, 0);
    MSI_ViewClose(view);
    msiobj_release(&view->hdr);
    return rc;
}

UINT msi_clone_properties(MSIPACKAGE *package)
{
    MSIQUERY *view = NULL;
    UINT rc;

    static const WCHAR Query[] = {
       'S','E','L','E','C','T',' ','*',' ',
       'F','R','O','M',' ','`','P','r','o','p','e','r','t','y','`',0};
    static const WCHAR Insert[] = {
       'I','N','S','E','R','T',' ','i','n','t','o',' ',
       '`','_','P','r','o','p','e','r','t','y','`',' ',
       '(','`','_','P','r','o','p','e','r','t','y','`',',',
       '`','V','a','l','u','e','`',')',' ',
       'V','A','L','U','E','S',' ','(','?',',','?',')',0};

    /* clone the existing properties */
    rc = MSI_DatabaseOpenViewW(package->db, Query, &view);
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
        MSIRECORD *row;
        MSIQUERY *view2;

        rc = MSI_ViewFetch(view, &row);
        if (rc != ERROR_SUCCESS)
            break;

        rc = MSI_DatabaseOpenViewW(package->db, Insert, &view2);
        if (rc != ERROR_SUCCESS)
        {
            msiobj_release(&row->hdr);
            continue;
        }

        MSI_ViewExecute(view2, row);
        MSI_ViewClose(view2);
        msiobj_release(&view2->hdr);
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

static UINT set_user_sid_prop( MSIPACKAGE *package )
{
    SID_NAME_USE use;
    LPWSTR user_name;
    LPWSTR sid_str = NULL, dom = NULL;
    DWORD size, dom_size;
    PSID psid = NULL;
    UINT r = ERROR_FUNCTION_FAILED;

    static const WCHAR user_sid[] = {'U','s','e','r','S','I','D',0};

    size = 0;
    GetUserNameW( NULL, &size );

    user_name = msi_alloc( (size + 1) * sizeof(WCHAR) );
    if (!user_name)
        return ERROR_OUTOFMEMORY;

    if (!GetUserNameW( user_name, &size ))
        goto done;

    size = 0;
    dom_size = 0;
    LookupAccountNameW( NULL, user_name, NULL, &size, NULL, &dom_size, &use );

    psid = msi_alloc( size );
    dom = msi_alloc( dom_size*sizeof (WCHAR) );
    if (!psid || !dom)
    {
        r = ERROR_OUTOFMEMORY;
        goto done;
    }

    if (!LookupAccountNameW( NULL, user_name, psid, &size, dom, &dom_size, &use ))
        goto done;

    if (!ConvertSidToStringSidW( psid, &sid_str ))
        goto done;

    r = MSI_SetPropertyW( package, user_sid, sid_str );

done:
    LocalFree( sid_str );
    msi_free( dom );
    msi_free( psid );
    msi_free( user_name );

    return r;
}

static LPWSTR get_fusion_filename(MSIPACKAGE *package)
{
    HKEY netsetup;
    LONG res;
    LPWSTR file;
    DWORD index = 0, size;
    WCHAR ver[MAX_PATH];
    WCHAR name[MAX_PATH];
    WCHAR windir[MAX_PATH];

    static const WCHAR backslash[] = {'\\',0};
    static const WCHAR fusion[] = {'f','u','s','i','o','n','.','d','l','l',0};
    static const WCHAR sub[] = {
        'S','o','f','t','w','a','r','e','\\',
        'M','i','c','r','o','s','o','f','t','\\',
        'N','E','T',' ','F','r','a','m','e','w','o','r','k',' ','S','e','t','u','p','\\',
        'N','D','P',0
    };
    static const WCHAR subdir[] = {
        'M','i','c','r','o','s','o','f','t','.','N','E','T','\\',
        'F','r','a','m','e','w','o','r','k','\\',0
    };

    res = RegOpenKeyExW(HKEY_LOCAL_MACHINE, sub, 0, KEY_ENUMERATE_SUB_KEYS, &netsetup);
    if (res != ERROR_SUCCESS)
        return NULL;

    ver[0] = '\0';
    size = MAX_PATH;
    while (RegEnumKeyExW(netsetup, index, name, &size, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        index++;
        if (lstrcmpW(ver, name) < 0)
            lstrcpyW(ver, name);
    }

    RegCloseKey(netsetup);

    if (!index)
        return NULL;

    GetWindowsDirectoryW(windir, MAX_PATH);

    size = lstrlenW(windir) + lstrlenW(subdir) + lstrlenW(ver) +lstrlenW(fusion) + 3;
    file = msi_alloc(size * sizeof(WCHAR));
    if (!file)
        return NULL;

    lstrcpyW(file, windir);
    lstrcatW(file, backslash);
    lstrcatW(file, subdir);
    lstrcatW(file, ver);
    lstrcatW(file, backslash);
    lstrcatW(file, fusion);

    return file;
}

typedef struct tagLANGANDCODEPAGE
{
  WORD wLanguage;
  WORD wCodePage;
} LANGANDCODEPAGE;

static void set_msi_assembly_prop(MSIPACKAGE *package)
{
    UINT val_len;
    DWORD size, handle;
    LPVOID version = NULL;
    WCHAR buf[MAX_PATH];
    LPWSTR fusion, verstr;
    LANGANDCODEPAGE *translate;

    static const WCHAR netasm[] = {
        'M','s','i','N','e','t','A','s','s','e','m','b','l','y','S','u','p','p','o','r','t',0
    };
    static const WCHAR translation[] = {
        '\\','V','a','r','F','i','l','e','I','n','f','o',
        '\\','T','r','a','n','s','l','a','t','i','o','n',0
    };
    static const WCHAR verfmt[] = {
        '\\','S','t','r','i','n','g','F','i','l','e','I','n','f','o',
        '\\','%','0','4','x','%','0','4','x',
        '\\','P','r','o','d','u','c','t','V','e','r','s','i','o','n',0
    };

    fusion = get_fusion_filename(package);
    if (!fusion)
        return;

    size = GetFileVersionInfoSizeW(fusion, &handle);
    if (!size) return;

    version = msi_alloc(size);
    if (!version) return;

    if (!GetFileVersionInfoW(fusion, handle, size, version))
        goto done;

    if (!VerQueryValueW(version, translation, (LPVOID *)&translate, &val_len))
        goto done;

    sprintfW(buf, verfmt, translate[0].wLanguage, translate[0].wCodePage);

    if (!VerQueryValueW(version, buf, (LPVOID *)&verstr, &val_len))
        goto done;

    if (!val_len || !verstr)
        goto done;

    MSI_SetPropertyW(package, netasm, verstr);

done:
    msi_free(fusion);
    msi_free(version);
}

static VOID set_installer_properties(MSIPACKAGE *package)
{
    WCHAR pth[MAX_PATH];
    WCHAR *ptr;
    OSVERSIONINFOEXW OSVersion;
    MEMORYSTATUSEX msex;
    DWORD verval;
    WCHAR verstr[10], bufstr[20];
    HDC dc;
    HKEY hkey;
    LPWSTR username, companyname;
    SYSTEM_INFO sys_info;
    SYSTEMTIME systemtime;
    LANGID langid;

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
    static const WCHAR szMsiNTProductType[] = { 'M','s','i','N','T','P','r','o','d','u','c','t','T','y','p','e',0 };
    static const WCHAR szFormat[] = {'%','l','i',0};
    static const WCHAR szWinBuild[] =
{'W','i','n','d','o','w','s','B','u','i','l','d', 0 };
    static const WCHAR szSPL[] = 
{'S','e','r','v','i','c','e','P','a','c','k','L','e','v','e','l',0 };
    static const WCHAR szSix[] = {'6',0 };

    static const WCHAR szVersionMsi[] = { 'V','e','r','s','i','o','n','M','s','i',0 };
    static const WCHAR szVersionDatabase[] = { 'V','e','r','s','i','o','n','D','a','t','a','b','a','s','e',0 };
    static const WCHAR szPhysicalMemory[] = { 'P','h','y','s','i','c','a','l','M','e','m','o','r','y',0 };
    static const WCHAR szFormat2[] = {'%','l','i','.','%','l','i',0};
/* Screen properties */
    static const WCHAR szScreenX[] = {'S','c','r','e','e','n','X',0};
    static const WCHAR szScreenY[] = {'S','c','r','e','e','n','Y',0};
    static const WCHAR szColorBits[] = {'C','o','l','o','r','B','i','t','s',0};
    static const WCHAR szIntFormat[] = {'%','d',0};
    static const WCHAR szIntel[] = { 'I','n','t','e','l',0 };
    static const WCHAR szUserInfo[] = {
        'S','O','F','T','W','A','R','E','\\',
        'M','i','c','r','o','s','o','f','t','\\',
        'M','S',' ','S','e','t','u','p',' ','(','A','C','M','E',')','\\',
        'U','s','e','r',' ','I','n','f','o',0
    };
    static const WCHAR szDefName[] = { 'D','e','f','N','a','m','e',0 };
    static const WCHAR szDefCompany[] = { 'D','e','f','C','o','m','p','a','n','y',0 };
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
    static const WCHAR szDate[] = {'D','a','t','e',0};
    static const WCHAR szTime[] = {'T','i','m','e',0};
    static const WCHAR szUserLangID[] = {'U','s','e','r','L','a','n','g','u','a','g','e','I','D',0};

    /*
     * Other things that probably should be set:
     *
     * SystemLanguageID ComputerName UserLanguageID LogonUser VirtualMemory
     * ShellAdvSupport DefaultUIFont PackagecodeChanging
     * ProductState CaptionHeight BorderTop BorderSide TextHeight
     * RedirectedDllSupport
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
    sprintfW( bufstr, szIntFormat, (int)(msex.ullTotalPhys/1024/1024));
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
    OSVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
    GetVersionExW((OSVERSIONINFOW *)&OSVersion);
    verval = OSVersion.dwMinorVersion+OSVersion.dwMajorVersion*100;
    sprintfW(verstr,szFormat,verval);
    switch (OSVersion.dwPlatformId)
    {
        case VER_PLATFORM_WIN32_WINDOWS:    
            MSI_SetPropertyW(package,v9x,verstr);
            break;
        case VER_PLATFORM_WIN32_NT:
            MSI_SetPropertyW(package,vNT,verstr);
            sprintfW(verstr,szFormat,OSVersion.wProductType);
            MSI_SetPropertyW(package,szMsiNTProductType,verstr);
            break;
    }
    sprintfW(verstr,szFormat,OSVersion.dwBuildNumber);
    MSI_SetPropertyW(package,szWinBuild,verstr);
    /* just fudge this */
    MSI_SetPropertyW(package,szSPL,szSix);

    sprintfW( bufstr, szFormat2, MSI_MAJORVERSION, MSI_MINORVERSION);
    MSI_SetPropertyW( package, szVersionMsi, bufstr );
    sprintfW( bufstr, szFormat, MSI_MAJORVERSION * 100);
    MSI_SetPropertyW( package, szVersionDatabase, bufstr );

    GetSystemInfo( &sys_info );
    if (sys_info.u.s.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
    {
        sprintfW( bufstr, szIntFormat, sys_info.wProcessorLevel );
        MSI_SetPropertyW( package, szIntel, bufstr );
    }

    /* Screen properties. */
    dc = GetDC(0);
    sprintfW( bufstr, szIntFormat, GetDeviceCaps( dc, HORZRES ) );
    MSI_SetPropertyW( package, szScreenX, bufstr );
    sprintfW( bufstr, szIntFormat, GetDeviceCaps( dc, VERTRES ));
    MSI_SetPropertyW( package, szScreenY, bufstr );
    sprintfW( bufstr, szIntFormat, GetDeviceCaps( dc, BITSPIXEL ));
    MSI_SetPropertyW( package, szColorBits, bufstr );
    ReleaseDC(0, dc);

    /* USERNAME and COMPANYNAME */
    username = msi_dup_property( package, szUSERNAME );
    companyname = msi_dup_property( package, szCOMPANYNAME );

    if ((!username || !companyname) &&
        RegOpenKeyW( HKEY_CURRENT_USER, szUserInfo, &hkey ) == ERROR_SUCCESS)
    {
        if (!username &&
            (username = msi_reg_get_val_str( hkey, szDefName )))
            MSI_SetPropertyW( package, szUSERNAME, username );
        if (!companyname &&
            (companyname = msi_reg_get_val_str( hkey, szDefCompany )))
            MSI_SetPropertyW( package, szCOMPANYNAME, companyname );
        CloseHandle( hkey );
    }
    if ((!username || !companyname) &&
        RegOpenKeyW( HKEY_LOCAL_MACHINE, szCurrentVersion, &hkey ) == ERROR_SUCCESS)
    {
        if (!username &&
            (username = msi_reg_get_val_str( hkey, szRegisteredUser )))
            MSI_SetPropertyW( package, szUSERNAME, username );
        if (!companyname &&
            (companyname = msi_reg_get_val_str( hkey, szRegisteredOrg )))
            MSI_SetPropertyW( package, szCOMPANYNAME, companyname );
        CloseHandle( hkey );
    }
    msi_free( username );
    msi_free( companyname );

    if ( set_user_sid_prop( package ) != ERROR_SUCCESS)
        ERR("Failed to set the UserSID property\n");

    /* Date and time properties */
    GetSystemTime( &systemtime );
    if (GetDateFormatW( LOCALE_USER_DEFAULT, DATE_SHORTDATE, &systemtime,
                        NULL, bufstr, sizeof(bufstr)/sizeof(bufstr[0]) ))
        MSI_SetPropertyW( package, szDate, bufstr );
    else
        ERR("Couldn't set Date property: GetDateFormat failed with error %d\n", GetLastError());

    if (GetTimeFormatW( LOCALE_USER_DEFAULT,
                        TIME_FORCE24HOURFORMAT | TIME_NOTIMEMARKER,
                        &systemtime, NULL, bufstr,
                        sizeof(bufstr)/sizeof(bufstr[0]) ))
        MSI_SetPropertyW( package, szTime, bufstr );
    else
        ERR("Couldn't set Time property: GetTimeFormat failed with error %d\n", GetLastError());

    set_msi_assembly_prop( package );

    langid = GetUserDefaultLangID();
    sprintfW(bufstr, szIntFormat, langid);

    MSI_SetPropertyW( package, szUserLangID, bufstr );
}

static UINT msi_load_summary_properties( MSIPACKAGE *package )
{
    UINT rc;
    MSIHANDLE suminfo;
    MSIHANDLE hdb = alloc_msihandle( &package->db->hdr );
    INT count;
    DWORD len;
    LPWSTR package_code;
    static const WCHAR szPackageCode[] = {
        'P','a','c','k','a','g','e','C','o','d','e',0};

    if (!hdb) {
        ERR("Unable to allocate handle\n");
        return ERROR_OUTOFMEMORY;
    }

    rc = MsiGetSummaryInformationW( hdb, NULL, 0, &suminfo );
    MsiCloseHandle(hdb);
    if (rc != ERROR_SUCCESS)
    {
        ERR("Unable to open Summary Information\n");
        return rc;
    }

    rc = MsiSummaryInfoGetPropertyW( suminfo, PID_PAGECOUNT, NULL,
                                     &count, NULL, NULL, NULL );
    if (rc != ERROR_SUCCESS)
    {
        WARN("Unable to query page count: %d\n", rc);
        goto done;
    }

    /* load package code property */
    len = 0;
    rc = MsiSummaryInfoGetPropertyW( suminfo, PID_REVNUMBER, NULL,
                                     NULL, NULL, NULL, &len );
    if (rc != ERROR_MORE_DATA)
    {
        WARN("Unable to query revision number: %d\n", rc);
        rc = ERROR_FUNCTION_FAILED;
        goto done;
    }

    len++;
    package_code = msi_alloc( len * sizeof(WCHAR) );
    rc = MsiSummaryInfoGetPropertyW( suminfo, PID_REVNUMBER, NULL,
                                     NULL, NULL, package_code, &len );
    if (rc != ERROR_SUCCESS)
    {
        WARN("Unable to query rev number: %d\n", rc);
        goto done;
    }

    MSI_SetPropertyW( package, szPackageCode, package_code );
    msi_free( package_code );

    /* load package attributes */
    count = 0;
    MsiSummaryInfoGetPropertyW( suminfo, PID_WORDCOUNT, NULL,
                                &count, NULL, NULL, NULL );
    package->WordCount = count;

done:
    MsiCloseHandle(suminfo);
    return rc;
}

static MSIPACKAGE *msi_alloc_package( void )
{
    MSIPACKAGE *package;

    package = alloc_msiobject( MSIHANDLETYPE_PACKAGE, sizeof (MSIPACKAGE),
                               MSI_FreePackage );
    if( package )
    {
        list_init( &package->components );
        list_init( &package->features );
        list_init( &package->files );
        list_init( &package->tempfiles );
        list_init( &package->folders );
        list_init( &package->subscriptions );
        list_init( &package->appids );
        list_init( &package->classes );
        list_init( &package->mimes );
        list_init( &package->extensions );
        list_init( &package->progids );
        list_init( &package->RunningActions );
        list_init( &package->sourcelist_info );
        list_init( &package->sourcelist_media );

        package->patch = NULL;
        package->ActionFormat = NULL;
        package->LastAction = NULL;
        package->dialog = NULL;
        package->next_dialog = NULL;
        package->scheduled_action_running = FALSE;
        package->commit_action_running = FALSE;
        package->rollback_action_running = FALSE;
    }

    return package;
}

static UINT msi_load_admin_properties(MSIPACKAGE *package)
{
    BYTE *data;
    UINT r, sz;

    static const WCHAR stmname[] = {'A','d','m','i','n','P','r','o','p','e','r','t','i','e','s',0};

    r = read_stream_data(package->db->storage, stmname, FALSE, &data, &sz);
    if (r != ERROR_SUCCESS)
        return r;

    r = msi_parse_command_line(package, (WCHAR *)data, TRUE);

    msi_free(data);
    return r;
}

MSIPACKAGE *MSI_CreatePackage( MSIDATABASE *db, LPCWSTR base_url )
{
    static const WCHAR szLevel[] = { 'U','I','L','e','v','e','l',0 };
    static const WCHAR szpi[] = {'%','i',0};
    static const WCHAR szProductCode[] = {
        'P','r','o','d','u','c','t','C','o','d','e',0};
    MSIPACKAGE *package;
    WCHAR uilevel[10];
    UINT r;

    TRACE("%p\n", db);

    package = msi_alloc_package();
    if (package)
    {
        msiobj_addref( &db->hdr );
        package->db = db;

        package->WordCount = 0;
        package->PackagePath = strdupW( db->path );
        package->BaseURL = strdupW( base_url );

        create_temp_property_table( package );
        msi_clone_properties( package );
        set_installer_properties(package);
        sprintfW(uilevel,szpi,gUILevel);
        MSI_SetPropertyW(package, szLevel, uilevel);

        package->ProductCode = msi_dup_property( package, szProductCode );
        set_installed_prop( package );
        r = msi_load_summary_properties( package );
        if (r != ERROR_SUCCESS)
        {
            msiobj_release( &package->hdr );
            return NULL;
        }

        if (package->WordCount & msidbSumInfoSourceTypeAdminImage)
            msi_load_admin_properties( package );
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
    static const WCHAR szMSI[] = {'m','s','i',0};

    GetTempPathW( MAX_PATH, path );
    GetTempFileNameW( path, szMSI, 0, filename );

    if( !CopyFileW( szPackage, filename, FALSE ) )
    {
        DeleteFileW( filename );
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
    static const WCHAR OriginalDatabase[] =
        {'O','r','i','g','i','n','a','l','D','a','t','a','b','a','s','e',0};
    static const WCHAR Database[] = {'D','A','T','A','B','A','S','E',0};
    MSIDATABASE *db = NULL;
    MSIPACKAGE *package;
    MSIHANDLE handle;
    LPWSTR ptr, base_url = NULL;
    UINT r;
    WCHAR temppath[MAX_PATH];
    LPCWSTR file = szPackage;

    TRACE("%s %p\n", debugstr_w(szPackage), pPackage);

    if( szPackage[0] == '#' )
    {
        handle = atoiW(&szPackage[1]);
        db = msihandle2msiinfo( handle, MSIHANDLETYPE_DATABASE );
        if( !db )
        {
            IWineMsiRemoteDatabase *remote_database;

            remote_database = (IWineMsiRemoteDatabase *)msi_get_remote( handle );
            if ( !remote_database )
                return ERROR_INVALID_HANDLE;

            IWineMsiRemoteDatabase_Release( remote_database );
            WARN("MsiOpenPackage not allowed during a custom action!\n");

            return ERROR_FUNCTION_FAILED;
        }
    }
    else
    {
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
        if( r != ERROR_SUCCESS )
        {
            if (file != szPackage)
                DeleteFileW( file );

            if (GetFileAttributesW(szPackage) == INVALID_FILE_ATTRIBUTES)
                return ERROR_FILE_NOT_FOUND;

            return r;
        }
    }

    package = MSI_CreatePackage( db, base_url );
    msi_free( base_url );
    msiobj_release( &db->hdr );
    if( !package )
    {
        if (file != szPackage)
            DeleteFileW( file );

        return ERROR_INSTALL_PACKAGE_INVALID;
    }

    if( file != szPackage )
        track_tempfile( package, file );

    MSI_SetPropertyW( package, Database, db->path );

    if( UrlIsW( szPackage, URLIS_URL ) )
        MSI_SetPropertyW( package, OriginalDatabase, szPackage );
    else if( szPackage[0] == '#' )
        MSI_SetPropertyW( package, OriginalDatabase, db->path );
    else
    {
        WCHAR fullpath[MAX_PATH];

        GetFullPathNameW( szPackage, MAX_PATH, fullpath, NULL );
        MSI_SetPropertyW( package, OriginalDatabase, fullpath );
    }

    *pPackage = package;

    return ERROR_SUCCESS;
}

UINT WINAPI MsiOpenPackageExW(LPCWSTR szPackage, DWORD dwOptions, MSIHANDLE *phPackage)
{
    MSIPACKAGE *package = NULL;
    UINT ret;

    TRACE("%s %08x %p\n", debugstr_w(szPackage), dwOptions, phPackage );

    if( !szPackage || !phPackage )
        return ERROR_INVALID_PARAMETER;

    if ( !*szPackage )
    {
        FIXME("Should create an empty database and package\n");
        return ERROR_FUNCTION_FAILED;
    }

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
    IWineMsiRemotePackage *remote_package;

    TRACE("(%ld)\n",hInstall);

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE);
    if( package)
    {
        handle = alloc_msihandle( &package->db->hdr );
        msiobj_release( &package->hdr );
    }
    else if ((remote_package = (IWineMsiRemotePackage *)msi_get_remote( hInstall )))
    {
        IWineMsiRemotePackage_GetActiveDatabase(remote_package, &handle);
        IWineMsiRemotePackage_Release(remote_package);
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
    {
        HRESULT hr;
        IWineMsiRemotePackage *remote_package;

        remote_package = (IWineMsiRemotePackage *)msi_get_remote( hInstall );
        if (!remote_package)
            return ERROR_INVALID_HANDLE;

        hr = IWineMsiRemotePackage_ProcessMessage( remote_package, eMessageType, hRecord );

        IWineMsiRemotePackage_Release( remote_package );

        if (FAILED(hr))
        {
            if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
                return HRESULT_CODE(hr);

            return ERROR_FUNCTION_FAILED;
        }

        return ERROR_SUCCESS;
    }

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
    MSIRECORD *row = NULL;
    UINT rc;
    DWORD sz = 0;
    WCHAR Query[1024];

    static const WCHAR Insert[] = {
        'I','N','S','E','R','T',' ','i','n','t','o',' ',
        '`','_','P','r','o','p','e','r','t','y','`',' ','(',
        '`','_','P','r','o','p','e','r','t','y','`',',',
        '`','V','a','l','u','e','`',')',' ','V','A','L','U','E','S'
        ,' ','(','?',',','?',')',0};
    static const WCHAR Update[] = {
        'U','P','D','A','T','E',' ','`','_','P','r','o','p','e','r','t','y','`',
        ' ','s','e','t',' ','`','V','a','l','u','e','`',' ','=',' ','?',' ',
        'w','h','e','r','e',' ','`','_','P','r','o','p','e','r','t','y','`',
        ' ','=',' ','\'','%','s','\'',0};
    static const WCHAR Delete[] = {
        'D','E','L','E','T','E',' ','F','R','O','M',' ',
        '`','_','P','r','o','p','e','r','t','y','`',' ','W','H','E','R','E',' ',
        '`','_','P','r','o','p','e','r','t','y','`',' ','=',' ','\'','%','s','\'',0};

    TRACE("%p %s %s\n", package, debugstr_w(szName), debugstr_w(szValue));

    if (!szName)
        return ERROR_INVALID_PARAMETER;

    /* this one is weird... */
    if (!szName[0])
        return szValue ? ERROR_FUNCTION_FAILED : ERROR_SUCCESS;

    rc = MSI_GetPropertyW(package, szName, 0, &sz);
    if (!szValue || !*szValue)
    {
        sprintfW(Query, Delete, szName);
    }
    else if (rc == ERROR_MORE_DATA || rc == ERROR_SUCCESS)
    {
        sprintfW(Query, Update, szName);

        row = MSI_CreateRecord(1);
        MSI_RecordSetStringW(row, 1, szValue);
    }
    else
    {
        strcpyW(Query, Insert);

        row = MSI_CreateRecord(2);
        MSI_RecordSetStringW(row, 1, szName);
        MSI_RecordSetStringW(row, 2, szValue);
    }

    rc = MSI_DatabaseOpenViewW(package->db, Query, &view);
    if (rc == ERROR_SUCCESS)
    {
        rc = MSI_ViewExecute(view, row);
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
    }

    msiobj_release(&row->hdr);

    if (rc == ERROR_SUCCESS && (!lstrcmpW(szName, cszSourceDir)))
        msi_reset_folders(package, TRUE);

    return rc;
}

UINT WINAPI MsiSetPropertyW( MSIHANDLE hInstall, LPCWSTR szName, LPCWSTR szValue)
{
    MSIPACKAGE *package;
    UINT ret;

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE);
    if( !package )
    {
        HRESULT hr;
        BSTR name = NULL, value = NULL;
        IWineMsiRemotePackage *remote_package;

        remote_package = (IWineMsiRemotePackage *)msi_get_remote( hInstall );
        if (!remote_package)
            return ERROR_INVALID_HANDLE;

        name = SysAllocString( szName );
        value = SysAllocString( szValue );
        if ((!name && szName) || (!value && szValue))
        {
            SysFreeString( name );
            SysFreeString( value );
            IWineMsiRemotePackage_Release( remote_package );
            return ERROR_OUTOFMEMORY;
        }

        hr = IWineMsiRemotePackage_SetProperty( remote_package, name, value );

        SysFreeString( name );
        SysFreeString( value );
        IWineMsiRemotePackage_Release( remote_package );

        if (FAILED(hr))
        {
            if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
                return HRESULT_CODE(hr);

            return ERROR_FUNCTION_FAILED;
        }

        return ERROR_SUCCESS;
    }

    ret = MSI_SetPropertyW( package, szName, szValue);
    msiobj_release( &package->hdr );
    return ret;
}

static MSIRECORD *MSI_GetPropertyRow( MSIPACKAGE *package, LPCWSTR name )
{
    static const WCHAR query[]= {
        'S','E','L','E','C','T',' ','`','V','a','l','u','e','`',' ',
        'F','R','O','M',' ' ,'`','_','P','r','o','p','e','r','t','y','`',
        ' ','W','H','E','R','E',' ' ,'`','_','P','r','o','p','e','r','t','y','`',
        '=','\'','%','s','\'',0};

    if (!name || !*name)
        return NULL;

    return MSI_QueryGetRecord( package->db, query, name );
}

/* internal function, not compatible with MsiGetPropertyW */
UINT MSI_GetPropertyW( MSIPACKAGE *package, LPCWSTR szName, 
                       LPWSTR szValueBuf, LPDWORD pchValueBuf )
{
    MSIRECORD *row;
    UINT rc = ERROR_FUNCTION_FAILED;

    row = MSI_GetPropertyRow( package, szName );

    if (*pchValueBuf > 0)
        szValueBuf[0] = 0;

    if (row)
    {
        rc = MSI_RecordGetStringW(row, 1, szValueBuf, pchValueBuf);
        msiobj_release(&row->hdr);
    }

    if (rc == ERROR_SUCCESS)
        TRACE("returning %s for property %s\n", debugstr_w(szValueBuf),
            debugstr_w(szName));
    else if (rc == ERROR_MORE_DATA)
        TRACE("need %d sized buffer for %s\n", *pchValueBuf,
            debugstr_w(szName));
    else
    {
        *pchValueBuf = 0;
        TRACE("property %s not found\n", debugstr_w(szName));
    }

    return rc;
}

LPWSTR msi_dup_property(MSIPACKAGE *package, LPCWSTR prop)
{
    DWORD sz = 0;
    LPWSTR str;
    UINT r;

    r = MSI_GetPropertyW(package, prop, NULL, &sz);
    if (r != ERROR_SUCCESS && r != ERROR_MORE_DATA)
        return NULL;

    sz++;
    str = msi_alloc(sz * sizeof(WCHAR));
    r = MSI_GetPropertyW(package, prop, str, &sz);
    if (r != ERROR_SUCCESS)
    {
        msi_free(str);
        str = NULL;
    }

    return str;
}

int msi_get_property_int(MSIPACKAGE *package, LPCWSTR prop, int def)
{
    LPWSTR str = msi_dup_property(package, prop);
    int val = str ? atoiW(str) : def;
    msi_free(str);
    return val;
}

static UINT MSI_GetProperty( MSIHANDLE handle, LPCWSTR name,
                             awstring *szValueBuf, LPDWORD pchValueBuf )
{
    static const WCHAR empty[] = {0};
    MSIPACKAGE *package;
    MSIRECORD *row = NULL;
    UINT r = ERROR_FUNCTION_FAILED;
    LPCWSTR val = NULL;

    TRACE("%lu %s %p %p\n", handle, debugstr_w(name),
          szValueBuf->str.w, pchValueBuf );

    if (!name)
        return ERROR_INVALID_PARAMETER;

    package = msihandle2msiinfo( handle, MSIHANDLETYPE_PACKAGE );
    if (!package)
    {
        HRESULT hr;
        IWineMsiRemotePackage *remote_package;
        LPWSTR value = NULL;
        BSTR bname;
        DWORD len;

        remote_package = (IWineMsiRemotePackage *)msi_get_remote( handle );
        if (!remote_package)
            return ERROR_INVALID_HANDLE;

        bname = SysAllocString( name );
        if (!bname)
        {
            IWineMsiRemotePackage_Release( remote_package );
            return ERROR_OUTOFMEMORY;
        }

        len = 0;
        hr = IWineMsiRemotePackage_GetProperty( remote_package, bname, NULL, &len );
        if (FAILED(hr))
            goto done;

        len++;
        value = msi_alloc(len * sizeof(WCHAR));
        if (!value)
        {
            r = ERROR_OUTOFMEMORY;
            goto done;
        }

        hr = IWineMsiRemotePackage_GetProperty( remote_package, bname, (BSTR *)value, &len );
        if (FAILED(hr))
            goto done;

        r = msi_strcpy_to_awstring( value, szValueBuf, pchValueBuf );

        /* Bug required by Adobe installers */
        if (!szValueBuf->unicode && !szValueBuf->str.a)
            *pchValueBuf *= sizeof(WCHAR);

done:
        IWineMsiRemotePackage_Release(remote_package);
        SysFreeString(bname);
        msi_free(value);

        if (FAILED(hr))
        {
            if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
                return HRESULT_CODE(hr);

            return ERROR_FUNCTION_FAILED;
        }

        return r;
    }

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
                             LPSTR szValueBuf, LPDWORD pchValueBuf )
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
                             LPWSTR szValueBuf, LPDWORD pchValueBuf )
{
    awstring val;

    val.unicode = TRUE;
    val.str.w = szValueBuf;

    return MSI_GetProperty( hInstall, szName, &val, pchValueBuf );
}

typedef struct _msi_remote_package_impl {
    const IWineMsiRemotePackageVtbl *lpVtbl;
    MSIHANDLE package;
    LONG refs;
} msi_remote_package_impl;

static inline msi_remote_package_impl* mrp_from_IWineMsiRemotePackage( IWineMsiRemotePackage* iface )
{
    return (msi_remote_package_impl*) iface;
}

static HRESULT WINAPI mrp_QueryInterface( IWineMsiRemotePackage *iface,
                REFIID riid,LPVOID *ppobj)
{
    if( IsEqualCLSID( riid, &IID_IUnknown ) ||
        IsEqualCLSID( riid, &IID_IWineMsiRemotePackage ) )
    {
        IUnknown_AddRef( iface );
        *ppobj = iface;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI mrp_AddRef( IWineMsiRemotePackage *iface )
{
    msi_remote_package_impl* This = mrp_from_IWineMsiRemotePackage( iface );

    return InterlockedIncrement( &This->refs );
}

static ULONG WINAPI mrp_Release( IWineMsiRemotePackage *iface )
{
    msi_remote_package_impl* This = mrp_from_IWineMsiRemotePackage( iface );
    ULONG r;

    r = InterlockedDecrement( &This->refs );
    if (r == 0)
    {
        MsiCloseHandle( This->package );
        msi_free( This );
    }
    return r;
}

static HRESULT WINAPI mrp_SetMsiHandle( IWineMsiRemotePackage *iface, MSIHANDLE handle )
{
    msi_remote_package_impl* This = mrp_from_IWineMsiRemotePackage( iface );
    This->package = handle;
    return S_OK;
}

static HRESULT WINAPI mrp_GetActiveDatabase( IWineMsiRemotePackage *iface, MSIHANDLE *handle )
{
    msi_remote_package_impl* This = mrp_from_IWineMsiRemotePackage( iface );
    IWineMsiRemoteDatabase *rdb = NULL;
    HRESULT hr;
    MSIHANDLE hdb;

    hr = create_msi_remote_database( NULL, (LPVOID *)&rdb );
    if (FAILED(hr) || !rdb)
    {
        ERR("Failed to create remote database\n");
        return hr;
    }

    hdb = MsiGetActiveDatabase(This->package);

    hr = IWineMsiRemoteDatabase_SetMsiHandle( rdb, hdb );
    if (FAILED(hr))
    {
        ERR("Failed to set the database handle\n");
        return hr;
    }

    *handle = alloc_msi_remote_handle( (IUnknown *)rdb );
    return S_OK;
}

static HRESULT WINAPI mrp_GetProperty( IWineMsiRemotePackage *iface, BSTR property, BSTR *value, DWORD *size )
{
    msi_remote_package_impl* This = mrp_from_IWineMsiRemotePackage( iface );
    UINT r;

    r = MsiGetPropertyW(This->package, (LPWSTR)property, (LPWSTR)value, size);
    if (r != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(r);

    return S_OK;
}

static HRESULT WINAPI mrp_SetProperty( IWineMsiRemotePackage *iface, BSTR property, BSTR value )
{
    msi_remote_package_impl* This = mrp_from_IWineMsiRemotePackage( iface );
    UINT r = MsiSetPropertyW(This->package, (LPWSTR)property, (LPWSTR)value);
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrp_ProcessMessage( IWineMsiRemotePackage *iface, INSTALLMESSAGE message, MSIHANDLE record )
{
    msi_remote_package_impl* This = mrp_from_IWineMsiRemotePackage( iface );
    UINT r = MsiProcessMessage(This->package, message, record);
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrp_DoAction( IWineMsiRemotePackage *iface, BSTR action )
{
    msi_remote_package_impl* This = mrp_from_IWineMsiRemotePackage( iface );
    UINT r = MsiDoActionW(This->package, (LPWSTR)action);
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrp_Sequence( IWineMsiRemotePackage *iface, BSTR table, int sequence )
{
    msi_remote_package_impl* This = mrp_from_IWineMsiRemotePackage( iface );
    UINT r = MsiSequenceW(This->package, (LPWSTR)table, sequence);
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrp_GetTargetPath( IWineMsiRemotePackage *iface, BSTR folder, BSTR *value, DWORD *size )
{
    msi_remote_package_impl* This = mrp_from_IWineMsiRemotePackage( iface );
    UINT r = MsiGetTargetPathW(This->package, (LPWSTR)folder, (LPWSTR)value, size);
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrp_SetTargetPath( IWineMsiRemotePackage *iface, BSTR folder, BSTR value)
{
    msi_remote_package_impl* This = mrp_from_IWineMsiRemotePackage( iface );
    UINT r = MsiSetTargetPathW(This->package, (LPWSTR)folder, (LPWSTR)value);
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrp_GetSourcePath( IWineMsiRemotePackage *iface, BSTR folder, BSTR *value, DWORD *size )
{
    msi_remote_package_impl* This = mrp_from_IWineMsiRemotePackage( iface );
    UINT r = MsiGetSourcePathW(This->package, (LPWSTR)folder, (LPWSTR)value, size);
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrp_GetMode( IWineMsiRemotePackage *iface, MSIRUNMODE mode, BOOL *ret )
{
    msi_remote_package_impl* This = mrp_from_IWineMsiRemotePackage( iface );
    *ret = MsiGetMode(This->package, mode);
    return S_OK;
}

static HRESULT WINAPI mrp_GetFeatureState( IWineMsiRemotePackage *iface, BSTR feature,
                                    INSTALLSTATE *installed, INSTALLSTATE *action )
{
    msi_remote_package_impl* This = mrp_from_IWineMsiRemotePackage( iface );
    UINT r = MsiGetFeatureStateW(This->package, (LPWSTR)feature, installed, action);
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrp_SetFeatureState( IWineMsiRemotePackage *iface, BSTR feature, INSTALLSTATE state )
{
    msi_remote_package_impl* This = mrp_from_IWineMsiRemotePackage( iface );
    UINT r = MsiSetFeatureStateW(This->package, (LPWSTR)feature, state);
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrp_GetComponentState( IWineMsiRemotePackage *iface, BSTR component,
                                      INSTALLSTATE *installed, INSTALLSTATE *action )
{
    msi_remote_package_impl* This = mrp_from_IWineMsiRemotePackage( iface );
    UINT r = MsiGetComponentStateW(This->package, (LPWSTR)component, installed, action);
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrp_SetComponentState( IWineMsiRemotePackage *iface, BSTR component, INSTALLSTATE state )
{
    msi_remote_package_impl* This = mrp_from_IWineMsiRemotePackage( iface );
    UINT r = MsiSetComponentStateW(This->package, (LPWSTR)component, state);
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrp_GetLanguage( IWineMsiRemotePackage *iface, LANGID *language )
{
    msi_remote_package_impl* This = mrp_from_IWineMsiRemotePackage( iface );
    *language = MsiGetLanguage(This->package);
    return S_OK;
}

static HRESULT WINAPI mrp_SetInstallLevel( IWineMsiRemotePackage *iface, int level )
{
    msi_remote_package_impl* This = mrp_from_IWineMsiRemotePackage( iface );
    UINT r = MsiSetInstallLevel(This->package, level);
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrp_FormatRecord( IWineMsiRemotePackage *iface, MSIHANDLE record,
                                        BSTR *value)
{
    DWORD size = 0;
    msi_remote_package_impl* This = mrp_from_IWineMsiRemotePackage( iface );
    UINT r = MsiFormatRecordW(This->package, record, NULL, &size);
    if (r == ERROR_SUCCESS)
    {
        *value = SysAllocStringLen(NULL, size);
        if (!*value)
            return E_OUTOFMEMORY;
        size++;
        r = MsiFormatRecordW(This->package, record, *value, &size);
    }
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrp_EvaluateCondition( IWineMsiRemotePackage *iface, BSTR condition )
{
    msi_remote_package_impl* This = mrp_from_IWineMsiRemotePackage( iface );
    UINT r = MsiEvaluateConditionW(This->package, (LPWSTR)condition);
    return HRESULT_FROM_WIN32(r);
}

static const IWineMsiRemotePackageVtbl msi_remote_package_vtbl =
{
    mrp_QueryInterface,
    mrp_AddRef,
    mrp_Release,
    mrp_SetMsiHandle,
    mrp_GetActiveDatabase,
    mrp_GetProperty,
    mrp_SetProperty,
    mrp_ProcessMessage,
    mrp_DoAction,
    mrp_Sequence,
    mrp_GetTargetPath,
    mrp_SetTargetPath,
    mrp_GetSourcePath,
    mrp_GetMode,
    mrp_GetFeatureState,
    mrp_SetFeatureState,
    mrp_GetComponentState,
    mrp_SetComponentState,
    mrp_GetLanguage,
    mrp_SetInstallLevel,
    mrp_FormatRecord,
    mrp_EvaluateCondition,
};

HRESULT create_msi_remote_package( IUnknown *pOuter, LPVOID *ppObj )
{
    msi_remote_package_impl* This;

    This = msi_alloc( sizeof *This );
    if (!This)
        return E_OUTOFMEMORY;

    This->lpVtbl = &msi_remote_package_vtbl;
    This->package = 0;
    This->refs = 1;

    *ppObj = This;

    return S_OK;
}

UINT msi_package_add_info(MSIPACKAGE *package, DWORD context, DWORD options,
                          LPCWSTR property, LPWSTR value)
{
    MSISOURCELISTINFO *info;

    info = msi_alloc(sizeof(MSISOURCELISTINFO));
    if (!info)
        return ERROR_OUTOFMEMORY;

    info->context = context;
    info->options = options;
    info->property = property;
    info->value = strdupW(value);
    list_add_head(&package->sourcelist_info, &info->entry);

    return ERROR_SUCCESS;
}

UINT msi_package_add_media_disk(MSIPACKAGE *package, DWORD context, DWORD options,
                                DWORD disk_id, LPWSTR volume_label, LPWSTR disk_prompt)
{
    MSIMEDIADISK *disk;

    disk = msi_alloc(sizeof(MSIMEDIADISK));
    if (!disk)
        return ERROR_OUTOFMEMORY;

    disk->context = context;
    disk->options = options;
    disk->disk_id = disk_id;
    disk->volume_label = strdupW(volume_label);
    disk->disk_prompt = strdupW(disk_prompt);
    list_add_head(&package->sourcelist_media, &disk->entry);

    return ERROR_SUCCESS;
}
