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

/*
 * Pages I need
 *
http://msdn.microsoft.com/library/default.asp?url=/library/en-us/msi/setup/installexecutesequence_table.asp

http://msdn.microsoft.com/library/default.asp?url=/library/en-us/msi/setup/standard_actions_reference.asp
 */

#include <stdarg.h>
#include <stdio.h>

#include <fcntl.h>
#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "wine/debug.h"
#include "fdi.h"
#include "msi.h"
#include "msiquery.h"
//#include "msvcrt/fcntl.h"
#include "objbase.h"
#include "objidl.h"
#include "msipriv.h"
#include "winnls.h"
#include "winuser.h"
#include "shlobj.h"
#include "wine/unicode.h"
#include "ver.h"

#define CUSTOM_ACTION_TYPE_MASK 0x3F
#define REG_PROGRESS_VALUE 13200
#define COMPONENT_PROGRESS_VALUE 24000

WINE_DEFAULT_DEBUG_CHANNEL(msi);

typedef struct tagMSIFEATURE
{
    WCHAR Feature[96];
    WCHAR Feature_Parent[96];
    WCHAR Title[0x100];
    WCHAR Description[0x100];
    INT Display;
    INT Level;
    WCHAR Directory[96];
    INT Attributes;
    
    INSTALLSTATE Installed;
    INSTALLSTATE ActionRequest;
    INSTALLSTATE Action;

    INT ComponentCount;
    INT Components[1024]; /* yes hardcoded limit.... I am bad */
    INT Cost;
} MSIFEATURE;

typedef struct tagMSICOMPONENT
{
    WCHAR Component[96];
    WCHAR ComponentId[96];
    WCHAR Directory[96];
    INT Attributes;
    WCHAR Condition[0x100];
    WCHAR KeyPath[96];

    INSTALLSTATE Installed;
    INSTALLSTATE ActionRequest;
    INSTALLSTATE Action;

    BOOL Enabled;
    INT  Cost;
} MSICOMPONENT;

typedef struct tagMSIFOLDER
{
    LPWSTR Directory;
    LPWSTR TargetDefault;
    LPWSTR SourceDefault;

    LPWSTR ResolvedTarget;
    LPWSTR ResolvedSource;
    LPWSTR Property;   /* initially set property */
    INT   ParentIndex;
    INT   State;
        /* 0 = uninitialized */
        /* 1 = existing */
        /* 2 = created remove if empty */
        /* 3 = created persist if empty */
    INT   Cost;
    INT   Space;
}MSIFOLDER;

typedef struct tagMSIFILE
{
    LPWSTR File;
    INT ComponentIndex;
    LPWSTR FileName;
    INT FileSize;
    LPWSTR Version;
    LPWSTR Language;
    INT Attributes;
    INT Sequence;   

    INT State;
       /* 0 = uninitialize */
       /* 1 = not present */
       /* 2 = present but replace */
       /* 3 = present do not replace */
       /* 4 = Installed */
    LPWSTR  SourcePath;
    LPWSTR  TargetPath;
    BOOL    Temporary; 
}MSIFILE;

/*
 * Prototypes
 */
static UINT ACTION_ProcessExecSequence(MSIPACKAGE *package, BOOL UIran);
static UINT ACTION_ProcessUISequence(MSIPACKAGE *package);

static UINT ACTION_PerformActionSequence(MSIPACKAGE *package, UINT seq);
UINT ACTION_PerformAction(MSIPACKAGE *package, const WCHAR *action);

static UINT ACTION_LaunchConditions(MSIPACKAGE *package);
static UINT ACTION_CostInitialize(MSIPACKAGE *package);
static UINT ACTION_CreateFolders(MSIPACKAGE *package);
static UINT ACTION_CostFinalize(MSIPACKAGE *package);
static UINT ACTION_FileCost(MSIPACKAGE *package);
static UINT ACTION_InstallFiles(MSIPACKAGE *package);
static UINT ACTION_DuplicateFiles(MSIPACKAGE *package);
static UINT ACTION_WriteRegistryValues(MSIPACKAGE *package);
static UINT ACTION_CustomAction(MSIPACKAGE *package,const WCHAR *action);
static UINT ACTION_InstallInitialize(MSIPACKAGE *package);
static UINT ACTION_InstallValidate(MSIPACKAGE *package);
static UINT ACTION_ProcessComponents(MSIPACKAGE *package);
static UINT ACTION_RegisterTypeLibraries(MSIPACKAGE *package);
static UINT ACTION_RegisterClassInfo(MSIPACKAGE *package);
static UINT ACTION_RegisterProgIdInfo(MSIPACKAGE *package);
static UINT ACTION_CreateShortcuts(MSIPACKAGE *package);
static UINT ACTION_PublishProduct(MSIPACKAGE *package);

static UINT HANDLE_CustomType1(MSIPACKAGE *package, const LPWSTR source, 
                                const LPWSTR target, const INT type);
static UINT HANDLE_CustomType2(MSIPACKAGE *package, const LPWSTR source, 
                                const LPWSTR target, const INT type);
static UINT HANDLE_CustomType18(MSIPACKAGE *package, const LPWSTR source,
                                const LPWSTR target, const INT type);
static UINT HANDLE_CustomType50(MSIPACKAGE *package, const LPWSTR source,
                                const LPWSTR target, const INT type);
static UINT HANDLE_CustomType34(MSIPACKAGE *package, const LPWSTR source,
                                const LPWSTR target, const INT type);

static DWORD deformat_string(MSIPACKAGE *package, WCHAR* ptr,WCHAR** data);
static LPWSTR resolve_folder(MSIPACKAGE *package, LPCWSTR name,
                           BOOL source, BOOL set_prop, MSIFOLDER **folder);

static int track_tempfile(MSIPACKAGE *package, LPCWSTR name, LPCWSTR path);
 
/*
 * consts and values used
 */
static const WCHAR cszSourceDir[] = {'S','o','u','r','c','e','D','i','r',0};
static const WCHAR cszRootDrive[] = {'R','O','O','T','D','R','I','V','E',0};
static const WCHAR cszTargetDir[] = {'T','A','R','G','E','T','D','I','R',0};
static const WCHAR cszTempFolder[]= {'T','e','m','p','F','o','l','d','e','r',0};
static const WCHAR cszDatabase[]={'D','A','T','A','B','A','S','E',0};
static const WCHAR c_collen[] = {'C',':','\\',0};
 
static const WCHAR cszlsb[]={'[',0};
static const WCHAR cszrsb[]={']',0};
static const WCHAR cszbs[]={'\\',0};

const static WCHAR szCreateFolders[] =
    {'C','r','e','a','t','e','F','o','l','d','e','r','s',0};
const static WCHAR szCostFinalize[] =
    {'C','o','s','t','F','i','n','a','l','i','z','e',0};
const static WCHAR szInstallFiles[] =
    {'I','n','s','t','a','l','l','F','i','l','e','s',0};
const static WCHAR szDuplicateFiles[] =
    {'D','u','p','l','i','c','a','t','e','F','i','l','e','s',0};
const static WCHAR szWriteRegistryValues[] =
{'W','r','i','t','e','R','e','g','i','s','t','r','y','V','a','l','u','e','s',0};
const static WCHAR szCostInitialize[] =
    {'C','o','s','t','I','n','i','t','i','a','l','i','z','e',0};
const static WCHAR szFileCost[] = {'F','i','l','e','C','o','s','t',0};
const static WCHAR szInstallInitialize[] = 
    {'I','n','s','t','a','l','l','I','n','i','t','i','a','l','i','z','e',0};
const static WCHAR szInstallValidate[] = 
    {'I','n','s','t','a','l','l','V','a','l','i','d','a','t','e',0};
const static WCHAR szLaunchConditions[] = 
    {'L','a','u','n','c','h','C','o','n','d','i','t','i','o','n','s',0};
const static WCHAR szProcessComponents[] = 
    {'P','r','o','c','e','s','s','C','o','m','p','o','n','e','n','t','s',0};
const static WCHAR szRegisterTypeLibraries[] = 
{'R','e','g','i','s','t','e','r','T','y','p','e','L','i','b','r','a','r',
'i','e','s',0};
const static WCHAR szRegisterClassInfo[] = 
{'R','e','g','i','s','t','e','r','C','l','a','s','s','I','n','f','o',0};
const static WCHAR szRegisterProgIdInfo[] = 
{'R','e','g','i','s','t','e','r','P','r','o','g','I','d','I','n','f','o',0};
const static WCHAR szCreateShortcuts[] = 
{'C','r','e','a','t','e','S','h','o','r','t','c','u','t','s',0};
const static WCHAR szPublishProduct[] = 
{'P','u','b','l','i','s','h','P','r','o','d','u','c','t',0};

/******************************************************** 
 * helper functions to get around current HACKS and such
 ********************************************************/
inline static void reduce_to_longfilename(WCHAR* filename)
{
    LPWSTR p = strchrW(filename,'|');
    if (p)
        memmove(filename, p+1, (strlenW(p+1)+1)*sizeof(WCHAR));
}

inline static char *strdupWtoA( const WCHAR *str )
{
    char *ret = NULL;
    if (str)
    {
        DWORD len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL
);
        if ((ret = HeapAlloc( GetProcessHeap(), 0, len )))
            WideCharToMultiByte( CP_ACP, 0, str, -1, ret, len, NULL, NULL );
    }
    return ret;
}

inline static WCHAR *strdupAtoW( const char *str )
{
    WCHAR *ret = NULL;
    if (str)
    {
        DWORD len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
        if ((ret = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
            MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len );
    }
    return ret;
}

static LPWSTR dupstrW(LPCWSTR src)
{
    LPWSTR dest;
    if (!src) return NULL;
    dest = HeapAlloc(GetProcessHeap(), 0, (strlenW(src)+1)*sizeof(WCHAR));
    strcpyW(dest, src);
    return dest;
}

inline static WCHAR *load_dynamic_stringW(MSIRECORD *row, INT index)
{
    UINT rc;
    DWORD sz;
    LPWSTR ret;
   
    sz = 0; 
    if (MSI_RecordIsNull(row,index))
        return NULL;

    rc = MSI_RecordGetStringW(row,index,NULL,&sz);

    /* having an empty string is different than NULL */
    if (sz == 0)
    {
        ret = HeapAlloc(GetProcessHeap(),0,sizeof(WCHAR));
        ret[0] = 0;
        return ret;
    }

    sz ++;
    ret = HeapAlloc(GetProcessHeap(),0,sz * sizeof (WCHAR));
    rc = MSI_RecordGetStringW(row,index,ret,&sz);
    if (rc!=ERROR_SUCCESS)
    {
        ERR("Unable to load dynamic string\n");
        HeapFree(GetProcessHeap(), 0, ret);
        ret = NULL;
    }
    return ret;
}

inline static LPWSTR load_dynamic_property(MSIPACKAGE *package, LPCWSTR prop,
                                           UINT* rc)
{
    DWORD sz = 0;
    LPWSTR str;
    UINT r;

    r = MSI_GetPropertyW(package, prop, NULL, &sz);
    if (r != ERROR_SUCCESS && r != ERROR_MORE_DATA)
    {
        if (rc)
            *rc = r;
        return NULL;
    }
    sz++;
    str = HeapAlloc(GetProcessHeap(),0,sz*sizeof(WCHAR));
    r = MSI_GetPropertyW(package, prop, str, &sz);
    if (r != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(),0,str);
        str = NULL;
    }
    if (rc)
        *rc = r;
    return str;
}

inline static int get_loaded_component(MSIPACKAGE* package, LPCWSTR Component )
{
    int rc = -1;
    DWORD i;

    for (i = 0; i < package->loaded_components; i++)
    {
        if (strcmpW(Component,package->components[i].Component)==0)
        {
            rc = i;
            break;
        }
    }
    return rc;
}

inline static int get_loaded_feature(MSIPACKAGE* package, LPCWSTR Feature )
{
    int rc = -1;
    DWORD i;

    for (i = 0; i < package->loaded_features; i++)
    {
        if (strcmpW(Feature,package->features[i].Feature)==0)
        {
            rc = i;
            break;
        }
    }
    return rc;
}

inline static int get_loaded_file(MSIPACKAGE* package, LPCWSTR file)
{
    int rc = -1;
    DWORD i;

    for (i = 0; i < package->loaded_files; i++)
    {
        if (strcmpW(file,package->files[i].File)==0)
        {
            rc = i;
            break;
        }
    }
    return rc;
}


static int track_tempfile(MSIPACKAGE *package, LPCWSTR name, LPCWSTR path)
{
    DWORD i;
    DWORD index;

    if (!package)
        return -2;

    for (i=0; i < package->loaded_files; i++)
        if (strcmpW(package->files[i].File,name)==0)
            return -1;

    index = package->loaded_files;
    package->loaded_files++;
    if (package->loaded_files== 1)
        package->files = HeapAlloc(GetProcessHeap(),0,sizeof(MSIFILE));
    else
        package->files = HeapReAlloc(GetProcessHeap(),0,
            package->files , package->loaded_files * sizeof(MSIFILE));

    memset(&package->files[index],0,sizeof(MSIFILE));

    package->files[index].File = dupstrW(name);
    package->files[index].TargetPath = dupstrW(path);
    package->files[index].Temporary = TRUE;

    TRACE("Tracking tempfile (%s)\n",debugstr_w(package->files[index].File));  

    return 0;
}

void ACTION_remove_tracked_tempfiles(MSIPACKAGE* package)
{
    DWORD i;

    if (!package)
        return;

    for (i = 0; i < package->loaded_files; i++)
    {
        if (package->files[i].Temporary)
        {
            TRACE("Cleaning up %s\n",debugstr_w(package->files[i].TargetPath));
            DeleteFileW(package->files[i].TargetPath);
        }

    }
}

/* Called when the package is being closed */
extern void ACTION_free_package_structures( MSIPACKAGE* package)
{
    INT i;
    
    TRACE("Freeing package action data\n");

    /* No dynamic buffers in features */
    if (package->features && package->loaded_features > 0)
        HeapFree(GetProcessHeap(),0,package->features);

    for (i = 0; i < package->loaded_folders; i++)
    {
        HeapFree(GetProcessHeap(),0,package->folders[i].Directory);
        HeapFree(GetProcessHeap(),0,package->folders[i].TargetDefault);
        HeapFree(GetProcessHeap(),0,package->folders[i].SourceDefault);
        HeapFree(GetProcessHeap(),0,package->folders[i].ResolvedTarget);
        HeapFree(GetProcessHeap(),0,package->folders[i].ResolvedSource);
        HeapFree(GetProcessHeap(),0,package->folders[i].Property);
    }
    if (package->folders && package->loaded_folders > 0)
        HeapFree(GetProcessHeap(),0,package->folders);

    /* no dynamic buffers in components */ 
    if (package->components && package->loaded_components > 0)
        HeapFree(GetProcessHeap(),0,package->components);

    for (i = 0; i < package->loaded_files; i++)
    {
        HeapFree(GetProcessHeap(),0,package->files[i].File);
        HeapFree(GetProcessHeap(),0,package->files[i].FileName);
        HeapFree(GetProcessHeap(),0,package->files[i].Version);
        HeapFree(GetProcessHeap(),0,package->files[i].Language);
        HeapFree(GetProcessHeap(),0,package->files[i].SourcePath);
        HeapFree(GetProcessHeap(),0,package->files[i].TargetPath);
    }

    if (package->files && package->loaded_files > 0)
        HeapFree(GetProcessHeap(),0,package->files);
}

static UINT ACTION_OpenQuery( MSIDATABASE *db, MSIQUERY **view, LPCWSTR fmt, ... )
{
    LPWSTR szQuery;
    LPCWSTR p;
    UINT sz, rc;
    va_list va;

    /* figure out how much space we need to allocate */
    va_start(va, fmt);
    sz = strlenW(fmt) + 1;
    p = fmt;
    while (*p)
    {
        p = strchrW(p, '%');
        if (!p)
            break;
        p++;
        switch (*p)
        {
        case 's':  /* a string */
            sz += strlenW(va_arg(va,LPCWSTR));
            break;
        case 'd':
        case 'i':  /* an integer -2147483648 seems to be longest */
            sz += 3*sizeof(int);
            (void)va_arg(va,int);
            break;
        case '%':  /* a single % - leave it alone */
            break;
        default:
            FIXME("Unhandled character type %c\n",*p);
        }
        p++;
    }
    va_end(va);

    /* construct the string */
    szQuery = HeapAlloc(GetProcessHeap(), 0, sz*sizeof(WCHAR));
    va_start(va, fmt);
    vsnprintfW(szQuery, sz, fmt, va);
    va_end(va);

    /* perform the query */
    rc = MSI_DatabaseOpenViewW(db, szQuery, view);
    HeapFree(GetProcessHeap(), 0, szQuery);
    return rc;
}

static void ui_progress(MSIPACKAGE *package, int a, int b, int c, int d )
{
    MSIRECORD * row;

    row = MSI_CreateRecord(4);
    MSI_RecordSetInteger(row,1,a);
    MSI_RecordSetInteger(row,2,b);
    MSI_RecordSetInteger(row,3,c);
    MSI_RecordSetInteger(row,4,d);
    MSI_ProcessMessage(package, INSTALLMESSAGE_PROGRESS, row);
    msiobj_release(&row->hdr);
}

static void ui_actiondata(MSIPACKAGE *package, LPCWSTR action, MSIRECORD * record)
{
    static const WCHAR Query_t[] = 
{'S','E','L','E','C','T',' ','*',' ','f','r','o','m',' ','A','c','t','i','o',
'n','T','e','x','t',' ','w','h','e','r','e',' ','A','c','t','i','o','n',' ','=',
' ','\'','%','s','\'',0};
    WCHAR message[1024];
    UINT rc;
    MSIQUERY * view;
    MSIRECORD * row = 0;
    LPWSTR ptr;

    if (!package->LastAction || strcmpW(package->LastAction,action))
    {
        rc = ACTION_OpenQuery(package->db, &view, Query_t, action);
        if (rc != ERROR_SUCCESS)
            return;

        rc = MSI_ViewExecute(view, 0);
        if (rc != ERROR_SUCCESS)
        {
            MSI_ViewClose(view);
            return;
        }
        rc = MSI_ViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            MSI_ViewClose(view);
            return;
        }

        if (MSI_RecordIsNull(row,3))
        {
            msiobj_release(&row->hdr);
            MSI_ViewClose(view);
            msiobj_release(&view->hdr);
            return;
        }

        /* update the cached actionformat */
        if (package->ActionFormat)
            HeapFree(GetProcessHeap(),0,package->ActionFormat);
        package->ActionFormat = load_dynamic_stringW(row,3);

        if (package->LastAction)
            HeapFree(GetProcessHeap(),0,package->LastAction);
        package->LastAction = dupstrW(action);

        msiobj_release(&row->hdr);
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
    }

    message[0]=0;
    ptr = package->ActionFormat;
    while (*ptr)
    {
        LPWSTR ptr2;
        LPWSTR data=NULL;
        WCHAR tmp[1023];
        INT field;

        ptr2 = strchrW(ptr,'[');
        if (ptr2)
        {
            strncpyW(tmp,ptr,ptr2-ptr);
            tmp[ptr2-ptr]=0;
            strcatW(message,tmp);
            ptr2++;
            field = atoiW(ptr2);
            data = load_dynamic_stringW(record,field);
            if (data)
            {
                strcatW(message,data);
                HeapFree(GetProcessHeap(),0,data);
            }
            ptr=strchrW(ptr2,']');
            ptr++;
        }
        else
        {
            strcatW(message,ptr);
            break;
        }
    }

    row = MSI_CreateRecord(1);
    MSI_RecordSetStringW(row,1,message);
 
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, row);
    msiobj_release(&row->hdr);
}


static void ui_actionstart(MSIPACKAGE *package, LPCWSTR action)
{
    static const WCHAR template_s[]=
{'A','c','t','i','o','n',' ','%','s',':',' ','%','s','.',' ','%','s','.',0};
    static const WCHAR format[] = 
{'H','H','\'',':','\'','m','m','\'',':','\'','s','s',0};
    static const WCHAR Query_t[] = 
{'S','E','L','E','C','T',' ','*',' ','f','r','o','m',' ','A','c','t','i','o',
'n','T','e','x','t',' ','w','h','e','r','e',' ','A','c','t','i','o','n',' ','=',
' ','\'','%','s','\'',0};
    WCHAR message[1024];
    WCHAR timet[0x100];
    UINT rc;
    MSIQUERY * view;
    MSIRECORD * row = 0;
    WCHAR *ActionText=NULL;

    GetTimeFormatW(LOCALE_USER_DEFAULT, 0, NULL, format, timet, 0x100);

    rc = ACTION_OpenQuery(package->db, &view, Query_t, action);
    if (rc != ERROR_SUCCESS)
        return;
    rc = MSI_ViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
        return;
    }
    rc = MSI_ViewFetch(view,&row);
    if (rc != ERROR_SUCCESS)
    {
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
        return;
    }

    ActionText = load_dynamic_stringW(row,2);
    msiobj_release(&row->hdr);
    MSI_ViewClose(view);
    msiobj_release(&view->hdr);

    sprintfW(message,template_s,timet,action,ActionText);

    row = MSI_CreateRecord(1);
    MSI_RecordSetStringW(row,1,message);
 
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONSTART, row);
    msiobj_release(&row->hdr);
    HeapFree(GetProcessHeap(),0,ActionText);
}

static void ui_actioninfo(MSIPACKAGE *package, LPCWSTR action, BOOL start, 
                          UINT rc)
{
    MSIRECORD * row;
    static const WCHAR template_s[]=
{'A','c','t','i','o','n',' ','s','t','a','r','t',' ','%','s',':',' ','%','s',
'.',0};
    static const WCHAR template_e[]=
{'A','c','t','i','o','n',' ','e','n','d','e','d',' ','%','s',':',' ','%','s',
'.',' ','R','e','t','u','r','n',' ','v','a','l','u','e',' ','%','i','.',0};
    static const WCHAR format[] = 
{'H','H','\'',':','\'','m','m','\'',':','\'','s','s',0};
    WCHAR message[1024];
    WCHAR timet[0x100];

    GetTimeFormatW(LOCALE_USER_DEFAULT, 0, NULL, format, timet, 0x100);
    if (start)
        sprintfW(message,template_s,timet,action);
    else
        sprintfW(message,template_e,timet,action,rc);
    
    row = MSI_CreateRecord(1);
    MSI_RecordSetStringW(row,1,message);
 
    MSI_ProcessMessage(package, INSTALLMESSAGE_INFO, row);
    msiobj_release(&row->hdr);
}

/*
 *  build_directory_name()
 *
 *  This function is to save messing round with directory names
 *  It handles adding backslashes between path segments, 
 *   and can add \ at the end of the directory name if told to.
 *
 *  It takes a variable number of arguments.
 *  It always allocates a new string for the result, so make sure
 *   to free the return value when finished with it.
 *
 *  The first arg is the number of path segments that follow.
 *  The arguments following count are a list of path segments.
 *  A path segment may be NULL.
 *
 *  Path segments will be added with a \ separating them.
 *  A \ will not be added after the last segment, however if the
 *    last segment is NULL, then the last character will be a \
 * 
 */
static LPWSTR build_directory_name(DWORD count, ...)
{
    DWORD sz = 1, i;
    LPWSTR dir;
    va_list va;

    va_start(va,count);
    for(i=0; i<count; i++)
    {
        LPCWSTR str = va_arg(va,LPCWSTR);
        if (str)
            sz += strlenW(str) + 1;
    }
    va_end(va);

    dir = HeapAlloc(GetProcessHeap(), 0, sz*sizeof(WCHAR));
    dir[0]=0;

    va_start(va,count);
    for(i=0; i<count; i++)
    {
        LPCWSTR str = va_arg(va,LPCWSTR);
        if (!str)
            continue;
        strcatW(dir, str);
        if( ((i+1)!=count) && dir[strlenW(dir)-1]!='\\')
            strcatW(dir, cszbs);
    }
    return dir;
}


/****************************************************
 * TOP level entry points 
 *****************************************************/

UINT ACTION_DoTopLevelINSTALL(MSIPACKAGE *package, LPCWSTR szPackagePath,
                              LPCWSTR szCommandLine)
{
    DWORD sz;
    WCHAR buffer[10];
    UINT rc;
    static const WCHAR szUILevel[] = {'U','I','L','e','v','e','l',0};

    if (szPackagePath)   
    {
        LPWSTR p, check, path;
 
        path = dupstrW(szPackagePath);
        p = strrchrW(path,'\\');    
        if (p)
        {
            p++;
            *p=0;
        }

        check = load_dynamic_property(package, cszSourceDir,NULL);
        if (!check)
            MSI_SetPropertyW(package, cszSourceDir, path);
        else
            HeapFree(GetProcessHeap(), 0, check);

        HeapFree(GetProcessHeap(), 0, path);
    }

    if (szCommandLine)
    {
        LPWSTR ptr,ptr2;
        ptr = (LPWSTR)szCommandLine;
       
        while (*ptr)
        {
            WCHAR *prop = NULL;
            WCHAR *val = NULL;

            TRACE("Looking at %s\n",debugstr_w(ptr));

            ptr2 = strchrW(ptr,'=');
            if (ptr2)
            {
                BOOL quote=FALSE;
                DWORD len = 0;

                while (*ptr == ' ') ptr++;
                len = ptr2-ptr;
                prop = HeapAlloc(GetProcessHeap(),0,(len+1)*sizeof(WCHAR));
                strncpyW(prop,ptr,len);
                prop[len]=0;
                ptr2++;
           
                len = 0; 
                ptr = ptr2; 
                while (*ptr && (quote || (!quote && *ptr!=' ')))
                {
                    if (*ptr == '"')
                        quote = !quote;
                    ptr++;
                    len++;
                }
               
                if (*ptr2=='"')
                {
                    ptr2++;
                    len -= 2;
                }
                val = HeapAlloc(GetProcessHeap(),0,(len+1)*sizeof(WCHAR));
                strncpyW(val,ptr2,len);
                val[len] = 0;

                if (strlenW(prop) > 0)
                {
                    TRACE("Found commandline property (%s) = (%s)\n", 
                                       debugstr_w(prop), debugstr_w(val));
                    MSI_SetPropertyW(package,prop,val);
                }
                HeapFree(GetProcessHeap(),0,val);
                HeapFree(GetProcessHeap(),0,prop);
            }
            ptr++;
        }
    }
  
    sz = 10; 
    if (MSI_GetPropertyW(package,szUILevel,buffer,&sz) == ERROR_SUCCESS)
    {
        if (atoiW(buffer) >= INSTALLUILEVEL_REDUCED)
        {
            rc = ACTION_ProcessUISequence(package);
            if (rc == ERROR_SUCCESS)
                rc = ACTION_ProcessExecSequence(package,TRUE);
        }
        else
            rc = ACTION_ProcessExecSequence(package,FALSE);
    }
    else
        rc = ACTION_ProcessExecSequence(package,FALSE);

    /* process the ending type action */
    if (rc == ERROR_SUCCESS)
        rc = ACTION_PerformActionSequence(package,-1);
    else if (rc == ERROR_FUNCTION_FAILED) 
        rc = ACTION_PerformActionSequence(package,-3);
    
    return rc;
}

static UINT ACTION_PerformActionSequence(MSIPACKAGE *package, UINT seq)
{
    MSIQUERY * view;
    UINT rc;
    WCHAR buffer[0x100];
    DWORD sz = 0x100;
    MSIRECORD * row = 0;
    static const WCHAR ExecSeqQuery[] =  {
   's','e','l','e','c','t',' ','*',' ',
   'f','r','o','m',' ',
       'I','n','s','t','a','l','l','E','x','e','c','u','t','e',
       'S','e','q','u','e','n','c','e',' ',
   'w','h','e','r','e',' ','S','e','q','u','e','n','c','e',' ',
       '=',' ','%','i',0};

    rc = ACTION_OpenQuery(package->db, &view, ExecSeqQuery, seq);

    if (rc == ERROR_SUCCESS)
    {
        rc = MSI_ViewExecute(view, 0);

        if (rc != ERROR_SUCCESS)
        {
            MSI_ViewClose(view);
            msiobj_release(&view->hdr);
            goto end;
        }
       
        TRACE("Running the actions\n"); 

        rc = MSI_ViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            rc = ERROR_SUCCESS;
            goto end;
        }

        /* check conditions */
        if (!MSI_RecordIsNull(row,2))
        {
            LPWSTR cond = NULL;
            cond = load_dynamic_stringW(row,2);

            if (cond)
            {
                /* this is a hack to skip errors in the condition code */
                if (MSI_EvaluateConditionW(package, cond) == MSICONDITION_FALSE)
                {
                    HeapFree(GetProcessHeap(),0,cond);
                    msiobj_release(&row->hdr);
                    goto end;
                }
                else
                    HeapFree(GetProcessHeap(),0,cond);
            }
        }

        sz=0x100;
        rc =  MSI_RecordGetStringW(row,1,buffer,&sz);
        if (rc != ERROR_SUCCESS)
        {
            ERR("Error is %x\n",rc);
            msiobj_release(&row->hdr);
            goto end;
        }

        rc = ACTION_PerformAction(package,buffer);
        msiobj_release(&row->hdr);
end:
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
    }
    else
        rc = ERROR_SUCCESS;

    return rc;
}

static UINT ACTION_ProcessExecSequence(MSIPACKAGE *package, BOOL UIran)
{
    MSIQUERY * view;
    UINT rc;
    static const WCHAR ExecSeqQuery[] =  {
       's','e','l','e','c','t',' ','*',' ',
       'f','r','o','m',' ',
           'I','n','s','t','a','l','l','E','x','e','c','u','t','e',
           'S','e','q','u','e','n','c','e',' ',
       'w','h','e','r','e',' ','S','e','q','u','e','n','c','e',' ',
           '>',' ','%','i',' ','o','r','d','e','r',' ',
       'b','y',' ','S','e','q','u','e','n','c','e',0 };
    MSIRECORD * row = 0;
    static const WCHAR IVQuery[] = {
       's','e','l','e','c','t',' ','S','e','q','u','e','n','c','e',' ',
       'f','r','o','m',' ','I','n','s','t','a','l','l',
           'E','x','e','c','u','t','e','S','e','q','u','e','n','c','e',' ',
       'w','h','e','r','e',' ','A','c','t','i','o','n',' ','=',' ',
           '`','I','n','s','t','a','l','l','V','a','l','i','d','a','t','e','`',
       0};
    INT seq = 0;

    /* get the sequence number */
    if (UIran)
    {
        rc = MSI_DatabaseOpenViewW(package->db, IVQuery, &view);
        if (rc != ERROR_SUCCESS)
            return rc;
        rc = MSI_ViewExecute(view, 0);
        if (rc != ERROR_SUCCESS)
        {
            MSI_ViewClose(view);
            msiobj_release(&view->hdr);
            return rc;
        }
        rc = MSI_ViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            MSI_ViewClose(view);
            msiobj_release(&view->hdr);
            return rc;
        }
        seq = MSI_RecordGetInteger(row,1);
        msiobj_release(&row->hdr);
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
    }

    rc = ACTION_OpenQuery(package->db, &view, ExecSeqQuery, seq);
    if (rc == ERROR_SUCCESS)
    {
        rc = MSI_ViewExecute(view, 0);

        if (rc != ERROR_SUCCESS)
        {
            MSI_ViewClose(view);
            msiobj_release(&view->hdr);
            goto end;
        }
       
        TRACE("Running the actions\n"); 

        while (1)
        {
            WCHAR buffer[0x100];
            DWORD sz = 0x100;

            rc = MSI_ViewFetch(view,&row);
            if (rc != ERROR_SUCCESS)
            {
                rc = ERROR_SUCCESS;
                break;
            }

            /* check conditions */
            if (!MSI_RecordIsNull(row,2))
            {
                LPWSTR cond = NULL;
                cond = load_dynamic_stringW(row,2);

                if (cond)
                {
                    /* this is a hack to skip errors in the condition code */
                    if (MSI_EvaluateConditionW(package, cond) ==
                            MSICONDITION_FALSE)
                    {
                        HeapFree(GetProcessHeap(),0,cond);
                        msiobj_release(&row->hdr);
                        continue; 
                    }
                    else
                        HeapFree(GetProcessHeap(),0,cond);
                }
            }

            sz=0x100;
            rc =  MSI_RecordGetStringW(row,1,buffer,&sz);
            if (rc != ERROR_SUCCESS)
            {
                ERR("Error is %x\n",rc);
                msiobj_release(&row->hdr);
                break;
            }

            rc = ACTION_PerformAction(package,buffer);

            if (rc == ERROR_FUNCTION_NOT_CALLED)
                rc = ERROR_SUCCESS;

            if (rc != ERROR_SUCCESS)
            {
                ERR("Execution halted due to error (%i)\n",rc);
                msiobj_release(&row->hdr);
                break;
            }

            msiobj_release(&row->hdr);
        }

        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
    }

end:
    return rc;
}


static UINT ACTION_ProcessUISequence(MSIPACKAGE *package)
{
    MSIQUERY * view;
    UINT rc;
    static const WCHAR ExecSeqQuery [] = {
      's','e','l','e','c','t',' ','*',' ',
      'f','r','o','m',' ','I','n','s','t','a','l','l',
            'U','I','S','e','q','u','e','n','c','e',' ',
      'w','h','e','r','e',' ','S','e','q','u','e','n','c','e',' ', '>',' ','0',' ',
      'o','r','d','e','r',' ','b','y',' ','S','e','q','u','e','n','c','e',0};
    
    rc = MSI_DatabaseOpenViewW(package->db, ExecSeqQuery, &view);
    
    if (rc == ERROR_SUCCESS)
    {
        rc = MSI_ViewExecute(view, 0);

        if (rc != ERROR_SUCCESS)
        {
            MSI_ViewClose(view);
            msiobj_release(&view->hdr);
            goto end;
        }
       
        TRACE("Running the actions \n"); 

        while (1)
        {
            WCHAR buffer[0x100];
            DWORD sz = 0x100;
            MSIRECORD * row = 0;

            rc = MSI_ViewFetch(view,&row);
            if (rc != ERROR_SUCCESS)
            {
                rc = ERROR_SUCCESS;
                break;
            }

            /* check conditions */
            if (!MSI_RecordIsNull(row,2))
            {
                LPWSTR cond = NULL;
                cond = load_dynamic_stringW(row,2);

                if (cond)
                {
                    /* this is a hack to skip errors in the condition code */
                    if (MSI_EvaluateConditionW(package, cond) ==
                            MSICONDITION_FALSE)
                    {
                        HeapFree(GetProcessHeap(),0,cond);
                        msiobj_release(&row->hdr);
                        continue; 
                    }
                    else
                        HeapFree(GetProcessHeap(),0,cond);
                }
            }

            sz=0x100;
            rc =  MSI_RecordGetStringW(row,1,buffer,&sz);
            if (rc != ERROR_SUCCESS)
            {
                ERR("Error is %x\n",rc);
                msiobj_release(&row->hdr);
                break;
            }

            rc = ACTION_PerformAction(package,buffer);

            if (rc == ERROR_FUNCTION_NOT_CALLED)
                rc = ERROR_SUCCESS;

            if (rc != ERROR_SUCCESS)
            {
                ERR("Execution halted due to error (%i)\n",rc);
                msiobj_release(&row->hdr);
                break;
            }

            msiobj_release(&row->hdr);
        }

        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
    }

end:
    return rc;
}

/********************************************************
 * ACTION helper functions and functions that perform the actions
 *******************************************************/

/* 
 * Alot of actions are really important even if they don't do anything
 * explicit.. Lots of properties are set at the beginning of the installation
 * CostFinalize does a bunch of work to translated the directories and such
 * 
 * But until I get write access to the database that is hard, so I am going to
 * hack it to see if I can get something to run.
 */
UINT ACTION_PerformAction(MSIPACKAGE *package, const WCHAR *action)
{
    UINT rc = ERROR_SUCCESS; 

    TRACE("Performing action (%s)\n",debugstr_w(action));
    ui_actioninfo(package, action, TRUE, 0);
    ui_actionstart(package, action);

    /* pre install, setup and configuration block */
    if (strcmpW(action,szLaunchConditions)==0)
        rc = ACTION_LaunchConditions(package);
    else if (strcmpW(action,szCostInitialize)==0)
        rc = ACTION_CostInitialize(package);
    else if (strcmpW(action,szFileCost)==0)
        rc = ACTION_FileCost(package);
    else if (strcmpW(action,szCostFinalize)==0)
        rc = ACTION_CostFinalize(package);
    else if (strcmpW(action,szInstallValidate)==0)
        rc = ACTION_InstallValidate(package);

    /* install block */
    else if (strcmpW(action,szProcessComponents)==0)
        rc = ACTION_ProcessComponents(package);
    else if (strcmpW(action,szInstallInitialize)==0)
        rc = ACTION_InstallInitialize(package);
    else if (strcmpW(action,szCreateFolders)==0)
        rc = ACTION_CreateFolders(package);
    else if (strcmpW(action,szInstallFiles)==0)
        rc = ACTION_InstallFiles(package);
    else if (strcmpW(action,szDuplicateFiles)==0)
        rc = ACTION_DuplicateFiles(package);
    else if (strcmpW(action,szWriteRegistryValues)==0)
        rc = ACTION_WriteRegistryValues(package);
     else if (strcmpW(action,szRegisterTypeLibraries)==0)
        rc = ACTION_RegisterTypeLibraries(package);
     else if (strcmpW(action,szRegisterClassInfo)==0)
        rc = ACTION_RegisterClassInfo(package);
     else if (strcmpW(action,szRegisterProgIdInfo)==0)
        rc = ACTION_RegisterProgIdInfo(package);
     else if (strcmpW(action,szCreateShortcuts)==0)
        rc = ACTION_CreateShortcuts(package);
    else if (strcmpW(action,szPublishProduct)==0)
        rc = ACTION_PublishProduct(package);

    /*
     Called during iTunes but unimplemented and seem important

     ResolveSource  (sets SourceDir)
     RegisterProduct
     InstallFinalize
     */
     else if ((rc = ACTION_CustomAction(package,action)) != ERROR_SUCCESS)
     {
        FIXME("UNHANDLED MSI ACTION %s\n",debugstr_w(action));
        rc = ERROR_FUNCTION_NOT_CALLED;
     }

    ui_actioninfo(package, action, FALSE, rc);
    return rc;
}


static UINT ACTION_CustomAction(MSIPACKAGE *package,const WCHAR *action)
{
    UINT rc = ERROR_SUCCESS;
    MSIQUERY * view;
    MSIRECORD * row = 0;
    static const WCHAR ExecSeqQuery[] =
    {'s','e','l','e','c','t',' ','*',' ','f','r','o','m',' ','C','u','s','t','o'
        ,'m','A','c','t','i','o','n',' ','w','h','e','r','e',' ','`','A','c','t','i'
        ,'o','n','`',' ','=',' ','`','%','s','`',0};
    UINT type;
    LPWSTR source;
    LPWSTR target;
    WCHAR *deformated=NULL;

    rc = ACTION_OpenQuery(package->db, &view, ExecSeqQuery, action);
    if (rc != ERROR_SUCCESS)
        return rc;

    rc = MSI_ViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
        return rc;
    }

    rc = MSI_ViewFetch(view,&row);
    if (rc != ERROR_SUCCESS)
    {
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
        return rc;
    }

    type = MSI_RecordGetInteger(row,2);

    source = load_dynamic_stringW(row,3);
    target = load_dynamic_stringW(row,4);

    TRACE("Handling custom action %s (%x %s %s)\n",debugstr_w(action),type,
          debugstr_w(source), debugstr_w(target));

    /* we are ignoring ALOT of flags and important synchronization stuff */
    switch (type & CUSTOM_ACTION_TYPE_MASK)
    {
        case 1: /* DLL file stored in a Binary table stream */
            rc = HANDLE_CustomType1(package,source,target,type);
            break;
        case 2: /* EXE file stored in a Binary table strem */
            rc = HANDLE_CustomType2(package,source,target,type);
            break;
        case 18: /*EXE file installed with package */
            rc = HANDLE_CustomType18(package,source,target,type);
            break;
        case 50: /*EXE file specified by a property value */
            rc = HANDLE_CustomType50(package,source,target,type);
            break;
        case 34: /*EXE to be run in specified directory */
            rc = HANDLE_CustomType34(package,source,target,type);
            break;
        case 35: /* Directory set with formatted text. */
            deformat_string(package,target,&deformated);
            MSI_SetTargetPathW(package, source, deformated);
            HeapFree(GetProcessHeap(),0,deformated);
            break;
        case 51: /* Property set with formatted text. */
            deformat_string(package,target,&deformated);
            rc = MSI_SetPropertyW(package,source,deformated);
            HeapFree(GetProcessHeap(),0,deformated);
            break;
        default:
            FIXME("UNHANDLED ACTION TYPE %i (%s %s)\n",
             type & CUSTOM_ACTION_TYPE_MASK, debugstr_w(source),
             debugstr_w(target));
    }

    HeapFree(GetProcessHeap(),0,source);
    HeapFree(GetProcessHeap(),0,target);
    msiobj_release(&row->hdr);
    MSI_ViewClose(view);
    msiobj_release(&view->hdr);
    return rc;
}

static UINT store_binary_to_temp(MSIPACKAGE *package, const LPWSTR source, 
                                LPWSTR tmp_file)
{
    DWORD sz=MAX_PATH;

    if (MSI_GetPropertyW(package, cszTempFolder, tmp_file, &sz) 
        != ERROR_SUCCESS)
        GetTempPathW(MAX_PATH,tmp_file);

    strcatW(tmp_file,source);

    if (GetFileAttributesW(tmp_file) != INVALID_FILE_ATTRIBUTES)
    {
        TRACE("File already exists\n");
        return ERROR_SUCCESS;
    }
    else
    {
        /* write out the file */
        UINT rc;
        MSIQUERY * view;
        MSIRECORD * row = 0;
        static const WCHAR fmt[] =
        {'s','e','l','e','c','t',' ','*',' ','f','r','o','m',' ','B','i'
,'n','a','r','y',' ','w','h','e','r','e',' ','N','a','m','e','=','`','%','s','`',0};
        HANDLE the_file;
        CHAR buffer[1024];

        if (track_tempfile(package, source, tmp_file)!=0)
            FIXME("File Name in temp tracking collision\n");

        the_file = CreateFileW(tmp_file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, NULL);
    
        if (the_file == INVALID_HANDLE_VALUE)
            return ERROR_FUNCTION_FAILED;

        rc = ACTION_OpenQuery(package->db, &view, fmt, source);
        if (rc != ERROR_SUCCESS)
            return rc;

        rc = MSI_ViewExecute(view, 0);
        if (rc != ERROR_SUCCESS)
        {
            MSI_ViewClose(view);
            msiobj_release(&view->hdr);
            return rc;
        }

        rc = MSI_ViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            MSI_ViewClose(view);
            msiobj_release(&view->hdr);
            return rc;
        }

        do 
        {
            DWORD write;
            sz = 1024;
            rc = MSI_RecordReadStream(row,2,buffer,&sz);
            if (rc != ERROR_SUCCESS)
            {
                ERR("Failed to get stream\n");
                CloseHandle(the_file);  
                DeleteFileW(tmp_file);
                break;
            }
            WriteFile(the_file,buffer,sz,&write,NULL);
        } while (sz == 1024);

        CloseHandle(the_file);

        msiobj_release(&row->hdr);
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
    }

    return ERROR_SUCCESS;
}

typedef UINT __stdcall CustomEntry(MSIHANDLE);
typedef struct 
{
        MSIPACKAGE *package;
        WCHAR *target;
        WCHAR *source;
} thread_struct;

static DWORD WINAPI ACTION_CallDllFunction(thread_struct *stuff)
{
    HANDLE hModule;
    LPSTR proc;
    CustomEntry *fn;

    TRACE("calling function (%s, %s) \n", debugstr_w(stuff->source),
          debugstr_w(stuff->target));

    hModule = LoadLibraryW(stuff->source);
    if (hModule)
    {
        proc = strdupWtoA( stuff->target );
        fn = (CustomEntry*)GetProcAddress(hModule,proc);
        if (fn)
        {
            MSIHANDLE hPackage;
            MSIPACKAGE *package = stuff->package;

            TRACE("Calling function %s\n", proc);
            hPackage = msiobj_findhandle( &package->hdr );
            if (hPackage )
            {
                fn(hPackage);
                msiobj_release( &package->hdr );
            }
            else
                ERR("Handle for object %p not found\n", package );
        }
        else
            ERR("Cannot load functon\n");

        HeapFree(GetProcessHeap(),0,proc);
        FreeLibrary(hModule);
    }
    else
        ERR("Unable to load library\n");
    msiobj_release( &stuff->package->hdr );
    HeapFree(GetProcessHeap(),0,stuff->source);
    HeapFree(GetProcessHeap(),0,stuff->target);
    HeapFree(GetProcessHeap(), 0, stuff);
    return 0;
}

static DWORD WINAPI DllThread(LPVOID info)
{
    thread_struct *stuff;
    DWORD rc = 0;
  
    TRACE("MSI Thread (0x%lx) started for custom action\n",
                        GetCurrentThreadId());
    
    stuff = (thread_struct*)info;
    rc = ACTION_CallDllFunction(stuff);

    TRACE("MSI Thread (0x%lx) finished\n",GetCurrentThreadId());
    /* clse all handles for this thread */
    MsiCloseAllHandles();
    return rc;
}

static UINT HANDLE_CustomType1(MSIPACKAGE *package, const LPWSTR source, 
                                const LPWSTR target, const INT type)
{
    WCHAR tmp_file[MAX_PATH];
    thread_struct *info;
    DWORD ThreadId;
    HANDLE ThreadHandle;

    store_binary_to_temp(package, source, tmp_file);

    TRACE("Calling function %s from %s\n",debugstr_w(target),
          debugstr_w(tmp_file));

    if (!strchrW(tmp_file,'.'))
    {
        static const WCHAR dot[]={'.',0};
        strcatW(tmp_file,dot);
    } 

    info = HeapAlloc( GetProcessHeap(), 0, sizeof(*info) );
    msiobj_addref( &package->hdr );
    info->package = package;
    info->target = dupstrW(target);
    info->source = dupstrW(tmp_file);

    ThreadHandle = CreateThread(NULL,0,DllThread,(LPVOID)info,0,&ThreadId);

    if (!(type & 0xc0))
        WaitForSingleObject(ThreadHandle,INFINITE);

    CloseHandle(ThreadHandle);
 
    return ERROR_SUCCESS;
}

static UINT HANDLE_CustomType2(MSIPACKAGE *package, const LPWSTR source, 
                                const LPWSTR target, const INT type)
{
    WCHAR tmp_file[MAX_PATH];
    STARTUPINFOW si;
    PROCESS_INFORMATION info;
    BOOL rc;
    INT len;
    WCHAR *deformated;
    WCHAR *cmd;
    static const WCHAR spc[] = {' ',0};

    memset(&si,0,sizeof(STARTUPINFOW));

    store_binary_to_temp(package, source, tmp_file);

    deformat_string(package,target,&deformated);

    len = strlenW(tmp_file) + strlenW(deformated) + 2;
   
    cmd = (WCHAR*)HeapAlloc(GetProcessHeap(),0,sizeof(WCHAR)*len);

    strcpyW(cmd,tmp_file);
    strcatW(cmd,spc);
    strcatW(cmd,deformated);

    HeapFree(GetProcessHeap(),0,deformated);

    TRACE("executing exe %s \n",debugstr_w(cmd));

    rc = CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL,
                  c_collen, &si, &info);

    HeapFree(GetProcessHeap(),0,cmd);

    if ( !rc )
    {
        ERR("Unable to execute command\n");
        return ERROR_SUCCESS;
    }

    if (!(type & 0xc0))
        WaitForSingleObject(info.hProcess,INFINITE);

    CloseHandle( info.hProcess );
    CloseHandle( info.hThread );
    return ERROR_SUCCESS;
}

static UINT HANDLE_CustomType18(MSIPACKAGE *package, const LPWSTR source,
                                const LPWSTR target, const INT type)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION info;
    BOOL rc;
    WCHAR *deformated;
    WCHAR *cmd;
    INT len;
    static const WCHAR spc[] = {' ',0};
    int index;

    memset(&si,0,sizeof(STARTUPINFOW));

    index = get_loaded_file(package,source);

    len = strlenW(package->files[index].TargetPath);

    deformat_string(package,target,&deformated);
    len += strlenW(deformated);
    len += 2;

    cmd = (WCHAR*)HeapAlloc(GetProcessHeap(),0,len * sizeof(WCHAR));

    strcpyW(cmd, package->files[index].TargetPath);
    strcatW(cmd, spc);
    strcatW(cmd, deformated);

    HeapFree(GetProcessHeap(),0,deformated);

    TRACE("executing exe %s \n",debugstr_w(cmd));

    rc = CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL,
                  c_collen, &si, &info);

    HeapFree(GetProcessHeap(),0,cmd);
    
    if ( !rc )
    {
        ERR("Unable to execute command\n");
        return ERROR_SUCCESS;
    }

    if (!(type & 0xc0))
        WaitForSingleObject(info.hProcess,INFINITE);

    CloseHandle( info.hProcess );
    CloseHandle( info.hThread );
    return ERROR_SUCCESS;
}

static UINT HANDLE_CustomType50(MSIPACKAGE *package, const LPWSTR source,
                                const LPWSTR target, const INT type)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION info;
    WCHAR *prop;
    BOOL rc;
    WCHAR *deformated;
    WCHAR *cmd;
    INT len;
    UINT prc;
    static const WCHAR spc[] = {' ',0};

    memset(&si,0,sizeof(STARTUPINFOW));
    memset(&info,0,sizeof(PROCESS_INFORMATION));

    prop = load_dynamic_property(package,source,&prc);
    if (!prop)
        return prc;

    deformat_string(package,target,&deformated);
    len = strlenW(prop) + strlenW(deformated) + 2;
    cmd = (WCHAR*)HeapAlloc(GetProcessHeap(),0,sizeof(WCHAR)*len);

    strcpyW(cmd,prop);
    strcatW(cmd,spc);
    strcatW(cmd,deformated);

    HeapFree(GetProcessHeap(),0,deformated);

    TRACE("executing exe %s \n",debugstr_w(cmd));

    rc = CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL,
                  c_collen, &si, &info);

    HeapFree(GetProcessHeap(),0,cmd);
    
    if ( !rc )
    {
        ERR("Unable to execute command\n");
        return ERROR_SUCCESS;
    }

    if (!(type & 0xc0))
        WaitForSingleObject(info.hProcess,INFINITE);

    CloseHandle( info.hProcess );
    CloseHandle( info.hThread );
    return ERROR_SUCCESS;
}

static UINT HANDLE_CustomType34(MSIPACKAGE *package, const LPWSTR source,
                                const LPWSTR target, const INT type)
{
    LPWSTR filename, deformated;
    STARTUPINFOW si;
    PROCESS_INFORMATION info;
    BOOL rc;

    memset(&si,0,sizeof(STARTUPINFOW));

    filename = resolve_folder(package, source, FALSE, FALSE, NULL);

    if (!filename)
        return ERROR_FUNCTION_FAILED;

    SetCurrentDirectoryW(filename);
    HeapFree(GetProcessHeap(),0,filename);

    deformat_string(package,target,&deformated);

    TRACE("executing exe %s \n",debugstr_w(deformated));

    rc = CreateProcessW(NULL, deformated, NULL, NULL, FALSE, 0, NULL,
                  c_collen, &si, &info);
    HeapFree(GetProcessHeap(),0,deformated);

    if ( !rc )
    {
        ERR("Unable to execute command\n");
        return ERROR_SUCCESS;
    }

    if (!(type & 0xc0))
        WaitForSingleObject(info.hProcess,INFINITE);

    CloseHandle( info.hProcess );
    CloseHandle( info.hThread );
    return ERROR_SUCCESS;
}

/***********************************************************************
 *            create_full_pathW
 *
 * Recursively create all directories in the path.
 *
 * shamelessly stolen from setupapi/queue.c
 */
static BOOL create_full_pathW(const WCHAR *path)
{
    BOOL ret = TRUE;
    int len;
    WCHAR *new_path;

    new_path = HeapAlloc(GetProcessHeap(), 0, (strlenW(path) + 1) *
                                              sizeof(WCHAR));

    strcpyW(new_path, path);

    while((len = strlenW(new_path)) && new_path[len - 1] == '\\')
    new_path[len - 1] = 0;

    while(!CreateDirectoryW(new_path, NULL))
    {
        WCHAR *slash;
        DWORD last_error = GetLastError();
        if(last_error == ERROR_ALREADY_EXISTS)
            break;

        if(last_error != ERROR_PATH_NOT_FOUND)
        {
            ret = FALSE;
            break;
        }

        if(!(slash = strrchrW(new_path, '\\')))
        {
            ret = FALSE;
            break;
        }

        len = slash - new_path;
        new_path[len] = 0;
        if(!create_full_pathW(new_path))
        {
            ret = FALSE;
            break;
        }
        new_path[len] = '\\';
    }

    HeapFree(GetProcessHeap(), 0, new_path);
    return ret;
}

/*
 * Also we cannot enable/disable components either, so for now I am just going 
 * to do all the directories for all the components.
 */
static UINT ACTION_CreateFolders(MSIPACKAGE *package)
{
    static const WCHAR ExecSeqQuery[] = {
        's','e','l','e','c','t',' ','D','i','r','e','c','t','o','r','y','_',' ',
        'f','r','o','m',' ','C','r','e','a','t','e','F','o','l','d','e','r',0 };
    UINT rc;
    MSIQUERY *view;
    MSIFOLDER *folder;

    rc = MSI_DatabaseOpenViewW(package->db, ExecSeqQuery, &view );
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_ViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
        return rc;
    }
    
    while (1)
    {
        WCHAR dir[0x100];
        LPWSTR full_path;
        DWORD sz;
        MSIRECORD *row = NULL, *uirow;

        rc = MSI_ViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            rc = ERROR_SUCCESS;
            break;
        }

        sz=0x100;
        rc = MSI_RecordGetStringW(row,1,dir,&sz);

        if (rc!= ERROR_SUCCESS)
        {
            ERR("Unable to get folder id \n");
            msiobj_release(&row->hdr);
            continue;
        }

        sz = MAX_PATH;
        full_path = resolve_folder(package,dir,FALSE,FALSE,&folder);
        if (!full_path)
        {
            ERR("Unable to resolve folder id %s\n",debugstr_w(dir));
            msiobj_release(&row->hdr);
            continue;
        }

        TRACE("Folder is %s\n",debugstr_w(full_path));

        /* UI stuff */
        uirow = MSI_CreateRecord(1);
        MSI_RecordSetStringW(uirow,1,full_path);
        ui_actiondata(package,szCreateFolders,uirow);
        msiobj_release( &uirow->hdr );

        if (folder->State == 0)
            create_full_pathW(full_path);

        folder->State = 3;

        msiobj_release(&row->hdr);
        HeapFree(GetProcessHeap(),0,full_path);
    }
    MSI_ViewClose(view);
    msiobj_release(&view->hdr);
   
    return rc;
}

static int load_component(MSIPACKAGE* package, MSIRECORD * row)
{
    int index = package->loaded_components;
    DWORD sz;

    /* fill in the data */

    package->loaded_components++;
    if (package->loaded_components == 1)
        package->components = HeapAlloc(GetProcessHeap(),0,
                                        sizeof(MSICOMPONENT));
    else
        package->components = HeapReAlloc(GetProcessHeap(),0,
            package->components, package->loaded_components * 
            sizeof(MSICOMPONENT));

    memset(&package->components[index],0,sizeof(MSICOMPONENT));

    sz = 96;       
    MSI_RecordGetStringW(row,1,package->components[index].Component,&sz);

    TRACE("Loading Component %s\n",
           debugstr_w(package->components[index].Component));

    sz = 0x100;
    if (!MSI_RecordIsNull(row,2))
        MSI_RecordGetStringW(row,2,package->components[index].ComponentId,&sz);
            
    sz = 96;       
    MSI_RecordGetStringW(row,3,package->components[index].Directory,&sz);

    package->components[index].Attributes = MSI_RecordGetInteger(row,4);

    sz = 0x100;       
    MSI_RecordGetStringW(row,5,package->components[index].Condition,&sz);

    sz = 96;       
    MSI_RecordGetStringW(row,6,package->components[index].KeyPath,&sz);

    package->components[index].Installed = INSTALLSTATE_ABSENT;
    package->components[index].Action = INSTALLSTATE_UNKNOWN;
    package->components[index].ActionRequest = INSTALLSTATE_UNKNOWN;

    package->components[index].Enabled = TRUE;

    return index;
}

static void load_feature(MSIPACKAGE* package, MSIRECORD * row)
{
    int index = package->loaded_features;
    DWORD sz;
    static const WCHAR Query1[] = {'S','E','L','E','C','T',' ','C','o','m','p',
        'o','n','e','n','t','_',' ','F','R','O','M',' ','F','e','a','t','u','r','e',
        'C','o','m','p','o','n','e','n','t','s',' ','W','H','E','R','E',' ','F','e',
        'a','t','u','r','e','_','=','\'','%','s','\'',0};
    static const WCHAR Query2[] = {'S','E','L','E','C','T',' ','*',' ','F','R',
        'O','M',' ','C','o','m','p','o','n','e','n','t',' ','W','H','E','R','E',' ','C',
        'o','m','p','o','n','e','n','t','=','\'','%','s','\'',0};
    MSIQUERY * view;
    MSIQUERY * view2;
    MSIRECORD * row2;
    MSIRECORD * row3;
    UINT    rc;

    /* fill in the data */

    package->loaded_features ++;
    if (package->loaded_features == 1)
        package->features = HeapAlloc(GetProcessHeap(),0,sizeof(MSIFEATURE));
    else
        package->features = HeapReAlloc(GetProcessHeap(),0,package->features,
                                package->loaded_features * sizeof(MSIFEATURE));

    memset(&package->features[index],0,sizeof(MSIFEATURE));
    
    sz = 96;       
    MSI_RecordGetStringW(row,1,package->features[index].Feature,&sz);

    TRACE("Loading feature %s\n",debugstr_w(package->features[index].Feature));

    sz = 96;
    if (!MSI_RecordIsNull(row,2))
        MSI_RecordGetStringW(row,2,package->features[index].Feature_Parent,&sz);

    sz = 0x100;
     if (!MSI_RecordIsNull(row,3))
        MSI_RecordGetStringW(row,3,package->features[index].Title,&sz);

     sz = 0x100;
     if (!MSI_RecordIsNull(row,4))
        MSI_RecordGetStringW(row,4,package->features[index].Description,&sz);

    if (!MSI_RecordIsNull(row,5))
        package->features[index].Display = MSI_RecordGetInteger(row,5);
  
    package->features[index].Level= MSI_RecordGetInteger(row,6);

     sz = 96;
     if (!MSI_RecordIsNull(row,7))
        MSI_RecordGetStringW(row,7,package->features[index].Directory,&sz);

    package->features[index].Attributes= MSI_RecordGetInteger(row,8);

    package->features[index].Installed = INSTALLSTATE_ABSENT;
    package->features[index].Action = INSTALLSTATE_UNKNOWN;
    package->features[index].ActionRequest = INSTALLSTATE_UNKNOWN;

    /* load feature components */

    rc = ACTION_OpenQuery(package->db, &view, Query1, package->features[index].Feature);
    if (rc != ERROR_SUCCESS)
        return;
    rc = MSI_ViewExecute(view,0);
    if (rc != ERROR_SUCCESS)
    {
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
        return;
    }
    while (1)
    {
        DWORD sz = 0x100;
        WCHAR buffer[0x100];
        DWORD rc;
        INT c_indx;
        INT cnt = package->features[index].ComponentCount;

        rc = MSI_ViewFetch(view,&row2);
        if (rc != ERROR_SUCCESS)
            break;

        sz = 0x100;
        MSI_RecordGetStringW(row2,1,buffer,&sz);

        /* check to see if the component is already loaded */
        c_indx = get_loaded_component(package,buffer);
        if (c_indx != -1)
        {
            TRACE("Component %s already loaded at %i\n", debugstr_w(buffer),
                  c_indx);
            package->features[index].Components[cnt] = c_indx;
            package->features[index].ComponentCount ++;
            continue;
        }

        rc = ACTION_OpenQuery(package->db, &view2, Query2, buffer);
        if (rc != ERROR_SUCCESS)
        {
            msiobj_release( &row2->hdr );
            continue;
        }
        rc = MSI_ViewExecute(view2,0);
        if (rc != ERROR_SUCCESS)
        {
            msiobj_release( &row2->hdr );
            MSI_ViewClose(view2);
            msiobj_release( &view2->hdr );  
            continue;
        }
        while (1)
        {
            DWORD rc;

            rc = MSI_ViewFetch(view2,&row3);
            if (rc != ERROR_SUCCESS)
                break;
            c_indx = load_component(package,row3);
            msiobj_release( &row3->hdr );

            package->features[index].Components[cnt] = c_indx;
            package->features[index].ComponentCount ++;
            TRACE("Loaded new component to index %i\n",c_indx);
        }
        MSI_ViewClose(view2);
        msiobj_release( &view2->hdr );
        msiobj_release( &row2->hdr );
    }
    MSI_ViewClose(view);
    msiobj_release(&view->hdr);
}

/*
 * I am not doing any of the costing functionality yet. 
 * Mostly looking at doing the Component and Feature loading
 *
 * The native MSI does ALOT of modification to tables here. Mostly adding alot
 * of temporary columns to the Feature and Component tables. 
 *
 *    note: native msi also tracks the short filename. but I am only going to
 *          track the long ones.  Also looking at this directory table
 *          it appears that the directory table does not get the parents
 *          resolved base on property only based on their entrys in the 
 *          directory table.
 */
static UINT ACTION_CostInitialize(MSIPACKAGE *package)
{
    MSIQUERY * view;
    MSIRECORD * row;
    UINT rc;
    static const WCHAR Query_all[] = {
       'S','E','L','E','C','T',' ','*',' ',
       'F','R','O','M',' ','F','e','a','t','u','r','e',0};
    static const WCHAR szCosting[] = {
       'C','o','s','t','i','n','g','C','o','m','p','l','e','t','e',0 };
    static const WCHAR szZero[] = { '0', 0 };

    MSI_SetPropertyW(package, szCosting, szZero);
    MSI_SetPropertyW(package, cszRootDrive , c_collen);

    rc = MSI_DatabaseOpenViewW(package->db,Query_all,&view);
    if (rc != ERROR_SUCCESS)
        return rc;
    rc = MSI_ViewExecute(view,0);
    if (rc != ERROR_SUCCESS)
    {
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
        return rc;
    }
    while (1)
    {
        DWORD rc;

        rc = MSI_ViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
            break;
       
        load_feature(package,row); 
        msiobj_release(&row->hdr);
    }
    MSI_ViewClose(view);
    msiobj_release(&view->hdr);

    return ERROR_SUCCESS;
}

static UINT load_file(MSIPACKAGE* package, MSIRECORD * row)
{
    DWORD index = package->loaded_files;
    DWORD i;
    LPWSTR buffer;

    /* fill in the data */

    package->loaded_files++;
    if (package->loaded_files== 1)
        package->files = HeapAlloc(GetProcessHeap(),0,sizeof(MSIFILE));
    else
        package->files = HeapReAlloc(GetProcessHeap(),0,
            package->files , package->loaded_files * sizeof(MSIFILE));

    memset(&package->files[index],0,sizeof(MSIFILE));
 
    package->files[index].File = load_dynamic_stringW(row, 1);
    buffer = load_dynamic_stringW(row, 2);

    package->files[index].ComponentIndex = -1;
    for (i = 0; i < package->loaded_components; i++)
        if (strcmpW(package->components[i].Component,buffer)==0)
        {
            package->files[index].ComponentIndex = i;
            break;
        }
    if (package->files[index].ComponentIndex == -1)
        ERR("Unfound Component %s\n",debugstr_w(buffer));
    HeapFree(GetProcessHeap(), 0, buffer);

    package->files[index].FileName = load_dynamic_stringW(row,3);

    reduce_to_longfilename(package->files[index].FileName);
    
    package->files[index].FileSize = MSI_RecordGetInteger(row,4);
    package->files[index].Version = load_dynamic_stringW(row, 5);
    package->files[index].Language = load_dynamic_stringW(row, 6);
    package->files[index].Attributes= MSI_RecordGetInteger(row,7);
    package->files[index].Sequence= MSI_RecordGetInteger(row,8);

    package->files[index].Temporary = FALSE;
    package->files[index].State = 0;

    TRACE("File Loaded (%s)\n",debugstr_w(package->files[index].File));  
 
    return ERROR_SUCCESS;
}

static UINT ACTION_FileCost(MSIPACKAGE *package)
{
    MSIQUERY * view;
    MSIRECORD * row;
    UINT rc;
    static const WCHAR Query[] = {
        'S','E','L','E','C','T',' ','*',' ',
        'F','R','O','M',' ','F','i','l','e',' ',
        'O','r','d','e','r',' ','b','y',' ','S','e','q','u','e','n','c','e', 0};

    if (!package)
        return ERROR_INVALID_HANDLE;

    rc = MSI_DatabaseOpenViewW(package->db, Query, &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;
   
    rc = MSI_ViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
        return ERROR_SUCCESS;
    }

    while (1)
    {
        rc = MSI_ViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            rc = ERROR_SUCCESS;
            break;
        }
        load_file(package,row);
        msiobj_release(&row->hdr);
    }
    MSI_ViewClose(view);
    msiobj_release(&view->hdr);

    return ERROR_SUCCESS;
}

static INT load_folder(MSIPACKAGE *package, const WCHAR* dir)

{
    static const WCHAR Query[] =
        {'s','e','l','e','c','t',' ','*',' ','f','r','o','m',' ','D','i','r','e','c',
         't','o','r','y',' ','w','h','e','r','e',' ','`','D','i','r','e','c','t',
         'o','r','y','`',' ','=',' ','`','%','s','`',0};
    UINT rc;
    MSIQUERY * view;
    LPWSTR targetdir, parent, srcdir;
    MSIRECORD * row = 0;
    INT index = -1;
    DWORD i;

    TRACE("Looking for dir %s\n",debugstr_w(dir));

    for (i = 0; i < package->loaded_folders; i++)
    {
        if (strcmpW(package->folders[i].Directory,dir)==0)
        {
            TRACE(" %s retuning on index %lu\n",debugstr_w(dir),i);
            return i;
        }
    }

    TRACE("Working to load %s\n",debugstr_w(dir));

    index = package->loaded_folders++;
    if (package->loaded_folders==1)
        package->folders = HeapAlloc(GetProcessHeap(),0,
                                        sizeof(MSIFOLDER));
    else
        package->folders= HeapReAlloc(GetProcessHeap(),0,
            package->folders, package->loaded_folders* 
            sizeof(MSIFOLDER));

    memset(&package->folders[index],0,sizeof(MSIFOLDER));

    package->folders[index].Directory = dupstrW(dir);

    rc = ACTION_OpenQuery(package->db, &view, Query, dir);
    if (rc != ERROR_SUCCESS)
        return -1;

    rc = MSI_ViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
        return -1;
    }

    rc = MSI_ViewFetch(view,&row);
    if (rc != ERROR_SUCCESS)
    {
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
        return -1;
    }

    targetdir = load_dynamic_stringW(row,3);

    /* split src and target dir */
    if (strchrW(targetdir,':'))
    {
        srcdir=strchrW(targetdir,':');
        *srcdir=0;
        srcdir ++;
    }
    else
        srcdir=NULL;

    /* for now only pick long filename versions */
    if (strchrW(targetdir,'|'))
    {
        targetdir = strchrW(targetdir,'|'); 
        *targetdir = 0;
        targetdir ++;
    }
    if (srcdir && strchrW(srcdir,'|'))
    {
        srcdir= strchrW(srcdir,'|'); 
        *srcdir= 0;
        srcdir ++;
    }

    /* now check for root dirs */
    if (targetdir[0] == '.' && targetdir[1] == 0)
        targetdir = NULL;
        
    if (srcdir && srcdir[0] == '.' && srcdir[1] == 0)
        srcdir = NULL;

    if (targetdir)
    {
        TRACE("   TargetDefault = %s\n",debugstr_w(targetdir));
        if (package->folders[index].TargetDefault)
            HeapFree(GetProcessHeap(),0, package->folders[index].TargetDefault);
        package->folders[index].TargetDefault = dupstrW(targetdir);
    }

    if (srcdir)
       package->folders[index].SourceDefault = dupstrW(srcdir);
    else if (targetdir)
        package->folders[index].SourceDefault = dupstrW(targetdir);
    HeapFree(GetProcessHeap(), 0, targetdir);

    parent = load_dynamic_stringW(row,2);
    if (parent) 
    {
        i = load_folder(package,parent);
        package->folders[index].ParentIndex = i;
        TRACE("Parent is index %i... %s %s\n",
                    package->folders[index].ParentIndex,
        debugstr_w(package->folders[package->folders[index].ParentIndex].Directory),
                    debugstr_w(parent));
    }
    else
        package->folders[index].ParentIndex = -2;
    HeapFree(GetProcessHeap(), 0, parent);

    package->folders[index].Property = load_dynamic_property(package, dir,NULL);

    msiobj_release(&row->hdr);
    MSI_ViewClose(view);
    msiobj_release(&view->hdr);
    TRACE(" %s retuning on index %i\n",debugstr_w(dir),index);
    return index;
}


static LPWSTR resolve_folder(MSIPACKAGE *package, LPCWSTR name,
                           BOOL source, BOOL set_prop, MSIFOLDER **folder)
{
    DWORD i;
    LPWSTR p, path = NULL;

    TRACE("Working to resolve %s\n",debugstr_w(name));

    /* special resolving for Target and Source root dir */
    if (strcmpW(name,cszTargetDir)==0 || strcmpW(name,cszSourceDir)==0)
    {
        if (!source)
        {
            path = load_dynamic_property(package,cszTargetDir,NULL);
            if (!path)
            {
                path = load_dynamic_property(package,cszRootDrive,NULL);
                if (set_prop)
                    MSI_SetPropertyW(package,cszTargetDir,path);
            }
            if (folder)
            {
                for (i = 0; i < package->loaded_folders; i++)
                {
                    if (strcmpW(package->folders[i].Directory,name)==0)
                        break;
                }
                *folder = &(package->folders[i]);
            }
            return path;
        }
        else
        {
            path = load_dynamic_property(package,cszSourceDir,NULL);
            if (!path)
            {
                path = load_dynamic_property(package,cszDatabase,NULL);
                if (path)
                {
                    p = strrchrW(path,'\\');
                    if (p)
                        *(p+1) = 0;
                }
            }
            if (folder)
            {
                for (i = 0; i < package->loaded_folders; i++)
                {
                    if (strcmpW(package->folders[i].Directory,name)==0)
                        break;
                }
                *folder = &(package->folders[i]);
            }
            return path;
        }
    }

    for (i = 0; i < package->loaded_folders; i++)
    {
        if (strcmpW(package->folders[i].Directory,name)==0)
            break;
    }

    if (i >= package->loaded_folders)
        return NULL;

    if (folder)
        *folder = &(package->folders[i]);

    if (!source && package->folders[i].ResolvedTarget)
    {
        path = dupstrW(package->folders[i].ResolvedTarget);
        TRACE("   already resolved to %s\n",debugstr_w(path));
        return path;
    }
    else if (source && package->folders[i].ResolvedSource)
    {
        path = dupstrW(package->folders[i].ResolvedSource);
        return path;
    }
    else if (!source && package->folders[i].Property)
    {
        path = dupstrW(package->folders[i].Property);
        TRACE("   internally set to %s\n",debugstr_w(path));
        if (set_prop)
            MSI_SetPropertyW(package,name,path);
        return path;
    }

    if (package->folders[i].ParentIndex >= 0)
    {
        LPWSTR parent = package->folders[package->folders[i].ParentIndex].Directory;

        TRACE(" ! Parent is %s\n", debugstr_w(parent));

        p = resolve_folder(package, parent, source, set_prop, NULL);
        if (!source)
        {
            TRACE("   TargetDefault = %s\n",debugstr_w(package->folders[i].TargetDefault));
            path = build_directory_name(3, p, package->folders[i].TargetDefault, NULL);
            package->folders[i].ResolvedTarget = dupstrW(path);
            TRACE("   resolved into %s\n",debugstr_w(path));
            if (set_prop)
                MSI_SetPropertyW(package,name,path);
        }
        else 
        {
            path = build_directory_name(3, p, package->folders[i].SourceDefault, NULL);
            package->folders[i].ResolvedSource = dupstrW(path);
        }
        HeapFree(GetProcessHeap(),0,p);
    }
    return path;
}

static UINT SetFeatureStates(MSIPACKAGE *package)
{
    LPWSTR level;
    INT install_level;
    DWORD i;
    INT j;
    LPWSTR override = NULL;
    static const WCHAR all[]={'A','L','L',0};
    static const WCHAR szlevel[] = {
        'I','N','S','T','A','L','L','L','E','V','E','L',0};
    static const WCHAR szAddLocal[] = {
        'A','D','D','L','O','C','A','L',0};

    /* I do not know if this is where it should happen.. but */

    TRACE("Checking Install Level\n");

    level = load_dynamic_property(package,szlevel,NULL);
    if (level)
    {
        install_level = atoiW(level);
        HeapFree(GetProcessHeap(), 0, level);
    }
    else
        install_level = 1;

    /* ok hereis the rub
     * ADDLOCAL and its friend OVERRIDE INSTALLLEVLE
     * I have confirmed this if ADDLOCALis stated then the INSTALLLEVEL is
     * itnored for all the features. seems strange, epsecially since it is not
     * documented anywhere, but it is how it works. 
     */
    
    override = load_dynamic_property(package,szAddLocal,NULL);
  
    if (override)
    {
        for(i = 0; i < package->loaded_features; i++)
        {
            if (strcmpiW(override,all)==0)
            {
                package->features[i].ActionRequest= INSTALLSTATE_LOCAL;
                package->features[i].Action = INSTALLSTATE_LOCAL;
            }
            else
            {
                LPWSTR ptr = override;
                LPWSTR ptr2 = strchrW(override,',');

                while (ptr)
                {
                    if ((ptr2 && 
                        strncmpW(ptr,package->features[i].Feature, ptr2-ptr)==0)
                        || (!ptr2 &&
                        strcmpW(ptr,package->features[i].Feature)==0))
                    {
                        package->features[i].ActionRequest= INSTALLSTATE_LOCAL;
                        package->features[i].Action = INSTALLSTATE_LOCAL;
                        break;
                    }
                    if (ptr2)
                    {
                        ptr=ptr2+1;
                        ptr2 = strchrW(ptr,',');
                    }
                    else
                        break;
                }
            }
        }
        HeapFree(GetProcessHeap(),0,override);
    } 
    else
    {
        for(i = 0; i < package->loaded_features; i++)
        {
            BOOL feature_state= ((package->features[i].Level > 0) &&
                             (package->features[i].Level <= install_level));

            if (feature_state)
            {
                package->features[i].ActionRequest= INSTALLSTATE_LOCAL;
                package->features[i].Action = INSTALLSTATE_LOCAL;
            }
        }
    }

    /*
     * now we want to enable or disable components base on feature 
    */

    for(i = 0; i < package->loaded_features; i++)
    {
        MSIFEATURE* feature = &package->features[i];
        TRACE("Examining Feature %s (Installed %i, Action %i, Request %i)\n",
            debugstr_w(feature->Feature), feature->Installed, feature->Action,
            feature->ActionRequest);

        for( j = 0; j < feature->ComponentCount; j++)
        {
            MSICOMPONENT* component = &package->components[
                                                    feature->Components[j]];

            if (!component->Enabled)
            {
                component->Action = INSTALLSTATE_ABSENT;
                component->ActionRequest = INSTALLSTATE_ABSENT;
            }
            else
            {
                if (feature->Action == INSTALLSTATE_LOCAL)
                    component->Action = INSTALLSTATE_LOCAL;
                if (feature->ActionRequest == INSTALLSTATE_LOCAL)
                    component->ActionRequest = INSTALLSTATE_LOCAL;
            }
        }
    } 

    for(i = 0; i < package->loaded_components; i++)
    {
        MSICOMPONENT* component= &package->components[i];

        TRACE("Result: Component %s (Installed %i, Action %i, Request %i)\n",
            debugstr_w(component->Component), component->Installed, 
            component->Action, component->ActionRequest);
    }


    return ERROR_SUCCESS;
}

/* 
 * Alot is done in this function aside from just the costing.
 * The costing needs to be implemented at some point but for now I am going
 * to focus on the directory building
 *
 */
static UINT ACTION_CostFinalize(MSIPACKAGE *package)
{
    static const WCHAR ExecSeqQuery[] = {
        's','e','l','e','c','t',' ','*',' ','f','r','o','m',' ',
        'D','i','r','e','c','t','o','r','y',0};
    static const WCHAR ConditionQuery[] = {
        's','e','l','e','c','t',' ','*',' ','f','r','o','m',' ',
        'C','o','n','d','i','t','i','o','n',0};
    static const WCHAR szCosting[] = {
       'C','o','s','t','i','n','g','C','o','m','p','l','e','t','e',0 };
    static const WCHAR szlevel[] = {
        'I','N','S','T','A','L','L','L','E','V','E','L',0};
    static const WCHAR szOne[] = { '1', 0 };
    UINT rc;
    MSIQUERY * view;
    DWORD i;
    LPWSTR level;

    TRACE("Building Directory properties\n");

    rc = MSI_DatabaseOpenViewW(package->db, ExecSeqQuery, &view);
    if (rc == ERROR_SUCCESS)
    {
        rc = MSI_ViewExecute(view, 0);
        if (rc != ERROR_SUCCESS)
        {
            MSI_ViewClose(view);
            msiobj_release(&view->hdr);
            return rc;
        }

        while (1)
        {
            WCHAR name[0x100];
            LPWSTR path;
            MSIRECORD * row = 0;
            DWORD sz;

            rc = MSI_ViewFetch(view,&row);
            if (rc != ERROR_SUCCESS)
            {
                rc = ERROR_SUCCESS;
                break;
            }

            sz=0x100;
            MSI_RecordGetStringW(row,1,name,&sz);

            /* This helper function now does ALL the work */
            TRACE("Dir %s ...\n",debugstr_w(name));
            load_folder(package,name);
            path = resolve_folder(package,name,FALSE,TRUE,NULL);
            TRACE("resolves to %s\n",debugstr_w(path));
            HeapFree( GetProcessHeap(), 0, path);

            msiobj_release(&row->hdr);
        }
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
    }

    TRACE("File calculations %i files\n",package->loaded_files);

    for (i = 0; i < package->loaded_files; i++)
    {
        MSICOMPONENT* comp = NULL;
        MSIFILE* file= NULL;

        file = &package->files[i];
        if (file->ComponentIndex >= 0)
            comp = &package->components[file->ComponentIndex];

        if (file->Temporary == TRUE)
            continue;

        if (comp)
        {
            LPWSTR p;

            /* calculate target */
            p = resolve_folder(package, comp->Directory, FALSE, FALSE, NULL);

            if (file->TargetPath)
                HeapFree(GetProcessHeap(),0,file->TargetPath);

            TRACE("file %s is named %s\n",
                   debugstr_w(file->File),debugstr_w(file->FileName));       

            file->TargetPath = build_directory_name(2, p, file->FileName);

            HeapFree(GetProcessHeap(),0,p);

            TRACE("file %s resolves to %s\n",
                   debugstr_w(file->File),debugstr_w(file->TargetPath));       

            if (GetFileAttributesW(file->TargetPath) == INVALID_FILE_ATTRIBUTES)
            {
                file->State = 1;
                comp->Cost += file->FileSize;
            }
            else
            {
                if (file->Version)
                {
                    DWORD handle;
                    DWORD versize;
                    UINT sz;
                    LPVOID version;
                    static const WCHAR name[] = 
                    {'\\',0};
                    static const WCHAR name_fmt[] = 
                    {'%','u','.','%','u','.','%','u','.','%','u',0};
                    WCHAR filever[0x100];
                    VS_FIXEDFILEINFO *lpVer;

                    FIXME("Version comparison.. \n");
                    versize = GetFileVersionInfoSizeW(file->TargetPath,&handle);
                    version = HeapAlloc(GetProcessHeap(),0,versize);
                    GetFileVersionInfoW(file->TargetPath, 0, versize, version);

                    VerQueryValueW(version, name, (LPVOID*)&lpVer, &sz);

                    sprintfW(filever,name_fmt,
                        HIWORD(lpVer->dwFileVersionMS),
                        LOWORD(lpVer->dwFileVersionMS),
                        HIWORD(lpVer->dwFileVersionLS),
                        LOWORD(lpVer->dwFileVersionLS));

                    TRACE("new %s old %s\n", debugstr_w(file->Version),
                          debugstr_w(filever));
                    if (strcmpiW(filever,file->Version)<0)
                    {
                        file->State = 2;
                        FIXME("cost should be diff in size\n");
                        comp->Cost += file->FileSize;
                    }
                    else
                        file->State = 3;
                    HeapFree(GetProcessHeap(),0,version);
                }
                else
                    file->State = 3;
            }
        } 
    }

    TRACE("Evaluating Condition Table\n");

    rc = MSI_DatabaseOpenViewW(package->db, ConditionQuery, &view);
    if (rc == ERROR_SUCCESS)
    {
        rc = MSI_ViewExecute(view, 0);
        if (rc != ERROR_SUCCESS)
        {
            MSI_ViewClose(view);
            msiobj_release(&view->hdr);
            return rc;
        }
    
        while (1)
        {
            WCHAR Feature[0x100];
            MSIRECORD * row = 0;
            DWORD sz;
            int feature_index;

            rc = MSI_ViewFetch(view,&row);

            if (rc != ERROR_SUCCESS)
            {
                rc = ERROR_SUCCESS;
                break;
            }

            sz = 0x100;
            MSI_RecordGetStringW(row,1,Feature,&sz);

            feature_index = get_loaded_feature(package,Feature);
            if (feature_index < 0)
                ERR("FAILED to find loaded feature %s\n",debugstr_w(Feature));
            else
            {
                LPWSTR Condition;
                Condition = load_dynamic_stringW(row,3);

                if (MSI_EvaluateConditionW(package,Condition) == 
                    MSICONDITION_TRUE)
                {
                    int level = MSI_RecordGetInteger(row,2);
                    TRACE("Reseting feature %s to level %i\n",
                           debugstr_w(Feature), level);
                    package->features[feature_index].Level = level;
                }
                HeapFree(GetProcessHeap(),0,Condition);
            }

            msiobj_release(&row->hdr);
        }
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
    }

    TRACE("Enabling or Disabling Components\n");
    for (i = 0; i < package->loaded_components; i++)
    {
        if (package->components[i].Condition[0])
        {
            if (MSI_EvaluateConditionW(package,
                package->components[i].Condition) == MSICONDITION_FALSE)
            {
                TRACE("Disabling component %s\n",
                      debugstr_w(package->components[i].Component));
                package->components[i].Enabled = FALSE;
            }
        }
    }

    MSI_SetPropertyW(package,szCosting,szOne);
    /* set default run level if not set */
    level = load_dynamic_property(package,szlevel,NULL);
    if (!level)
        MSI_SetPropertyW(package,szlevel, szOne);
    else
        HeapFree(GetProcessHeap(),0,level);

    return SetFeatureStates(package);

}

/*
 * This is a helper function for handling embedded cabinet media
 */
static UINT writeout_cabinet_stream(MSIPACKAGE *package, WCHAR* stream_name,
                                    WCHAR* source)
{
    UINT rc;
    USHORT* data;
    UINT    size;
    DWORD   write;
    HANDLE  the_file;
    WCHAR tmp[MAX_PATH];

    rc = read_raw_stream_data(package->db,stream_name,&data,&size); 
    if (rc != ERROR_SUCCESS)
        return rc;

    write = MAX_PATH;
    if (MSI_GetPropertyW(package, cszTempFolder, tmp, &write))
        GetTempPathW(MAX_PATH,tmp);

    GetTempFileNameW(tmp,stream_name,0,source);

    track_tempfile(package,strrchrW(source,'\\'), source);
    the_file = CreateFileW(source, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, NULL);

    if (the_file == INVALID_HANDLE_VALUE)
    {
        rc = ERROR_FUNCTION_FAILED;
        goto end;
    }

    WriteFile(the_file,data,size,&write,NULL);
    CloseHandle(the_file);
    TRACE("wrote %li bytes to %s\n",write,debugstr_w(source));
end:
    HeapFree(GetProcessHeap(),0,data);
    return rc;
}


/* Support functions for FDI functions */
typedef struct
{
    MSIPACKAGE* package;
    LPCSTR cab_path;
    LPCSTR file_name;
} CabData;

static void * cabinet_alloc(ULONG cb)
{
    return HeapAlloc(GetProcessHeap(), 0, cb);
}

static void cabinet_free(void *pv)
{
    HeapFree(GetProcessHeap(), 0, pv);
}

static INT_PTR cabinet_open(char *pszFile, int oflag, int pmode)
{
    DWORD dwAccess = 0;
    DWORD dwShareMode = 0;
    DWORD dwCreateDisposition = OPEN_EXISTING;
    switch (oflag & _O_ACCMODE)
    {
    case _O_RDONLY:
        dwAccess = GENERIC_READ;
        dwShareMode = FILE_SHARE_READ | FILE_SHARE_DELETE;
        break;
    case _O_WRONLY:
        dwAccess = GENERIC_WRITE;
        dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
        break;
    case _O_RDWR:
        dwAccess = GENERIC_READ | GENERIC_WRITE;
        dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
        break;
    }
    if ((oflag & (_O_CREAT | _O_EXCL)) == (_O_CREAT | _O_EXCL))
        dwCreateDisposition = CREATE_NEW;
    else if (oflag & _O_CREAT)
        dwCreateDisposition = CREATE_ALWAYS;
    return (INT_PTR)CreateFileA(pszFile, dwAccess, dwShareMode, NULL, dwCreateDisposition, 0, NULL);
}

static UINT cabinet_read(INT_PTR hf, void *pv, UINT cb)
{
    DWORD dwRead;
    if (ReadFile((HANDLE)hf, pv, cb, &dwRead, NULL))
        return dwRead;
    return 0;
}

static UINT cabinet_write(INT_PTR hf, void *pv, UINT cb)
{
    DWORD dwWritten;
    if (WriteFile((HANDLE)hf, pv, cb, &dwWritten, NULL))
        return dwWritten;
    return 0;
}

static int cabinet_close(INT_PTR hf)
{
    return CloseHandle((HANDLE)hf) ? 0 : -1;
}

static long cabinet_seek(INT_PTR hf, long dist, int seektype)
{
    /* flags are compatible and so are passed straight through */
    return SetFilePointer((HANDLE)hf, dist, NULL, seektype);
}

static INT_PTR cabinet_notify(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin)
{
    /* FIXME: try to do more processing in this function */
    switch (fdint)
    {
    case fdintCOPY_FILE:
    {
        CabData *data = (CabData*) pfdin->pv;
        ULONG len = strlen(data->cab_path) + strlen(pfdin->psz1);
        char *file;

        LPWSTR trackname;
        LPWSTR trackpath;
        LPWSTR tracknametmp;
        static const WCHAR tmpprefix[] = {'C','A','B','T','M','P','_',0};
       
        if (data->file_name && strcmp(data->file_name,pfdin->psz1))
                return 0;
        
        file = cabinet_alloc((len+1)*sizeof(char));
        strcpy(file, data->cab_path);
        strcat(file, pfdin->psz1);

        TRACE("file: %s\n", debugstr_a(file));

        /* track this file so it can be deleted if not installed */
        trackpath=strdupAtoW(file);
        tracknametmp=strdupAtoW(strrchr(file,'\\')+1);
        trackname = HeapAlloc(GetProcessHeap(),0,(strlenW(tracknametmp) + 
                                  strlenW(tmpprefix)+1) * sizeof(WCHAR));

        strcpyW(trackname,tmpprefix);
        strcatW(trackname,tracknametmp);

        track_tempfile(data->package, trackname, trackpath);

        HeapFree(GetProcessHeap(),0,trackpath);
        HeapFree(GetProcessHeap(),0,trackname);
        HeapFree(GetProcessHeap(),0,tracknametmp);

        return cabinet_open(file, _O_WRONLY | _O_CREAT, 0);
    }
    case fdintCLOSE_FILE_INFO:
    {
        FILETIME ft;
	    FILETIME ftLocal;
        if (!DosDateTimeToFileTime(pfdin->date, pfdin->time, &ft))
            return -1;
        if (!LocalFileTimeToFileTime(&ft, &ftLocal))
            return -1;
        if (!SetFileTime((HANDLE)pfdin->hf, &ftLocal, 0, &ftLocal))
            return -1;

        cabinet_close(pfdin->hf);
        return 1;
    }
    default:
        return 0;
    }
}

/***********************************************************************
 *            extract_cabinet_file
 *
 * Extract files from a cab file.
 */
static BOOL extract_a_cabinet_file(MSIPACKAGE* package, const WCHAR* source, 
                                 const WCHAR* path, const WCHAR* file)
{
    HFDI hfdi;
    ERF erf;
    BOOL ret;
    char *cabinet;
    char *cab_path;
    char *file_name;
    CabData data;

    TRACE("Extracting %s (%s) to %s\n",debugstr_w(source), 
                    debugstr_w(file), debugstr_w(path));

    hfdi = FDICreate(cabinet_alloc,
                     cabinet_free,
                     cabinet_open,
                     cabinet_read,
                     cabinet_write,
                     cabinet_close,
                     cabinet_seek,
                     0,
                     &erf);
    if (!hfdi)
    {
        ERR("FDICreate failed\n");
        return FALSE;
    }

    if (!(cabinet = strdupWtoA( source )))
    {
        FDIDestroy(hfdi);
        return FALSE;
    }
    if (!(cab_path = strdupWtoA( path )))
    {
        FDIDestroy(hfdi);
        HeapFree(GetProcessHeap(), 0, cabinet);
        return FALSE;
    }

    data.package = package;
    data.cab_path = cab_path;
    file_name = strdupWtoA(file);
    data.file_name = file_name;

    ret = FDICopy(hfdi, cabinet, "", 0, cabinet_notify, NULL, &data);

    if (!ret)
        ERR("FDICopy failed\n");

    FDIDestroy(hfdi);

    HeapFree(GetProcessHeap(), 0, cabinet);
    HeapFree(GetProcessHeap(), 0, cab_path);
    HeapFree(GetProcessHeap(), 0, file_name);

    return ret;
}

static UINT ready_media_for_file(MSIPACKAGE *package, UINT sequence, 
                                 WCHAR* path, WCHAR* file)
{
    UINT rc;
    MSIQUERY * view;
    MSIRECORD * row = 0;
    static WCHAR source[MAX_PATH];
    static const WCHAR ExecSeqQuery[] = {
        's','e','l','e','c','t',' ','*',' ',
        'f','r','o','m',' ','M','e','d','i','a',' ',
        'w','h','e','r','e',' ','L','a','s','t','S','e','q','u','e','n','c','e',' ','>','=',' ','%','i',' ',
        'o','r','d','e','r',' ','b','y',' ','L','a','s','t','S','e','q','u','e','n','c','e',0};
    WCHAR Query[1024];
    WCHAR cab[0x100];
    DWORD sz=0x100;
    INT seq;
    static UINT last_sequence = 0; 

    if (sequence <= last_sequence)
    {
        TRACE("Media already ready (%u, %u)\n",sequence,last_sequence);
        extract_a_cabinet_file(package, source,path,file);
        return ERROR_SUCCESS;
    }

    sprintfW(Query,ExecSeqQuery,sequence);

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

    rc = MSI_ViewFetch(view,&row);
    if (rc != ERROR_SUCCESS)
    {
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
        return rc;
    }
    seq = MSI_RecordGetInteger(row,2);
    last_sequence = seq;

    if (!MSI_RecordIsNull(row,4))
    {
        sz=0x100;
        MSI_RecordGetStringW(row,4,cab,&sz);
        TRACE("Source is CAB %s\n",debugstr_w(cab));
        /* the stream does not contain the # character */
        if (cab[0]=='#')
        {
            writeout_cabinet_stream(package,&cab[1],source);
            strcpyW(path,source);
            *(strrchrW(path,'\\')+1)=0;
        }
        else
        {
            sz = MAX_PATH;
            if (MSI_GetPropertyW(package, cszSourceDir, source, &sz))
            {
                ERR("No Source dir defined \n");
                rc = ERROR_FUNCTION_FAILED;
            }
            else
            {
                strcpyW(path,source);
                strcatW(source,cab);
                /* extract the cab file into a folder in the temp folder */
                sz = MAX_PATH;
                if (MSI_GetPropertyW(package, cszTempFolder,path, &sz) 
                                    != ERROR_SUCCESS)
                    GetTempPathW(MAX_PATH,path);
            }
        }
        rc = !extract_a_cabinet_file(package, source,path,file);
    }
    msiobj_release(&row->hdr);
    MSI_ViewClose(view);
    msiobj_release(&view->hdr);
    return rc;
}

inline static UINT create_component_directory ( MSIPACKAGE* package, INT component)
{
    UINT rc;
    MSIFOLDER *folder;
    LPWSTR install_path;

    install_path = resolve_folder(package, package->components[component].Directory,
                        FALSE, FALSE, &folder);
    if (!install_path)
        return ERROR_FUNCTION_FAILED; 

    /* create the path */
    if (folder->State == 0)
    {
        create_full_pathW(install_path);
        folder->State = 2;
    }
    HeapFree(GetProcessHeap(), 0, install_path);

    return rc;
}

static UINT ACTION_InstallFiles(MSIPACKAGE *package)
{
    UINT rc = ERROR_SUCCESS;
    DWORD index;
    MSIRECORD * uirow;
    WCHAR uipath[MAX_PATH];

    if (!package)
        return ERROR_INVALID_HANDLE;

    /* increment progress bar each time action data is sent */
    ui_progress(package,1,1,0,0);

    for (index = 0; index < package->loaded_files; index++)
    {
        WCHAR path_to_source[MAX_PATH];
        MSIFILE *file;
        
        file = &package->files[index];

        if (file->Temporary)
            continue;

        if (package->components[file->ComponentIndex].ActionRequest != 
             INSTALLSTATE_LOCAL)
        {
            ui_progress(package,2,file->FileSize,0,0);
            TRACE("File %s is not scheduled for install\n",
                   debugstr_w(file->File));

            continue;
        }

        if ((file->State == 1) || (file->State == 2))
        {
            LPWSTR p;
            INT len;
            MSICOMPONENT* comp = NULL;

            TRACE("Installing %s\n",debugstr_w(file->File));
            rc = ready_media_for_file(package,file->Sequence,path_to_source,
                            file->File);
            /* 
             * WARNING!
             * our file table could change here because a new temp file
             * may have been created
             */
            file = &package->files[index];
            if (rc != ERROR_SUCCESS)
            {
                ERR("Unable to ready media\n");
                rc = ERROR_FUNCTION_FAILED;
                break;
            }

            create_component_directory( package, file->ComponentIndex);

            /* recalculate file paths because things may have changed */

            if (file->ComponentIndex >= 0)
                comp = &package->components[file->ComponentIndex];

            p = resolve_folder(package, comp->Directory, FALSE, FALSE, NULL);
            if (file->TargetPath)
                HeapFree(GetProcessHeap(),0,file->TargetPath);

            file->TargetPath = build_directory_name(2, p, file->FileName);

            len = strlenW(path_to_source) + strlenW(file->File) + 2;
            file->SourcePath = HeapAlloc(GetProcessHeap(),0,len*sizeof(WCHAR));
            strcpyW(file->SourcePath, path_to_source);
            strcatW(file->SourcePath, file->File);

            TRACE("file paths %s to %s\n",debugstr_w(file->SourcePath),
                  debugstr_w(file->TargetPath));

            /* the UI chunk */
            uirow=MSI_CreateRecord(9);
            MSI_RecordSetStringW(uirow,1,file->File);
            strcpyW(uipath,file->TargetPath);
            *(strrchrW(uipath,'\\')+1)=0;
            MSI_RecordSetStringW(uirow,9,uipath);
            MSI_RecordSetInteger(uirow,6,file->FileSize);
            ui_actiondata(package,szInstallFiles,uirow);
            msiobj_release( &uirow->hdr );
            ui_progress(package,2,file->FileSize,0,0);

            if (!MoveFileW(file->SourcePath,file->TargetPath))
            {
                rc = GetLastError();
                ERR("Unable to move file (%s -> %s) (error %d)\n",
                     debugstr_w(file->SourcePath), debugstr_w(file->TargetPath),
                      rc);
                if (rc == ERROR_ALREADY_EXISTS && file->State == 2)
                {
                    CopyFileW(file->SourcePath,file->TargetPath,FALSE);
                    DeleteFileW(file->SourcePath);
                    rc = 0;
                }
                else if (rc == ERROR_FILE_NOT_FOUND)
                {
                    ERR("Source File Not Found!  Continueing\n");
                    rc = 0;
                }
                else
                {
                    ERR("Ignoring Error and continuing...\n");
                    rc = 0;
                }
            }
            else
                file->State = 4;
        }
    }

    return rc;
}

inline static UINT get_file_target(MSIPACKAGE *package, LPCWSTR file_key, 
                                   LPWSTR* file_source)
{
    DWORD index;

    if (!package)
        return ERROR_INVALID_HANDLE;

    for (index = 0; index < package->loaded_files; index ++)
    {
        if (strcmpW(file_key,package->files[index].File)==0)
        {
            if (package->files[index].State >= 3)
            {
                *file_source = dupstrW(package->files[index].TargetPath);
                return ERROR_SUCCESS;
            }
            else
                return ERROR_FILE_NOT_FOUND;
        }
    }

    return ERROR_FUNCTION_FAILED;
}

static UINT ACTION_DuplicateFiles(MSIPACKAGE *package)
{
    UINT rc;
    MSIQUERY * view;
    MSIRECORD * row = 0;
    static const WCHAR ExecSeqQuery[] = {
        's','e','l','e','c','t',' ','*',' ','f','r','o','m',' ',
        'D','u','p','l','i','c','a','t','e','F','i','l','e',0};

    if (!package)
        return ERROR_INVALID_HANDLE;

    rc = MSI_DatabaseOpenViewW(package->db, ExecSeqQuery, &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_ViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
        return rc;
    }

    while (1)
    {
        WCHAR file_key[0x100];
        WCHAR *file_source = NULL;
        WCHAR dest_name[0x100];
        LPWSTR dest_path, dest;
        WCHAR component[0x100];
        INT component_index;

        DWORD sz=0x100;

        rc = MSI_ViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            rc = ERROR_SUCCESS;
            break;
        }

        sz=0x100;
        rc = MSI_RecordGetStringW(row,2,component,&sz);
        if (rc != ERROR_SUCCESS)
        {
            ERR("Unable to get component\n");
            msiobj_release(&row->hdr);
            break;
        }

        component_index = get_loaded_component(package,component);
        if (package->components[component_index].ActionRequest != 
             INSTALLSTATE_LOCAL)
        {
            TRACE("Skipping copy due to disabled component\n");
            msiobj_release(&row->hdr);
            continue;
        }

        sz=0x100;
        rc = MSI_RecordGetStringW(row,3,file_key,&sz);
        if (rc != ERROR_SUCCESS)
        {
            ERR("Unable to get file key\n");
            msiobj_release(&row->hdr);
            break;
        }

        rc = get_file_target(package,file_key,&file_source);

        if (rc != ERROR_SUCCESS)
        {
            ERR("Original file unknown %s\n",debugstr_w(file_key));
            msiobj_release(&row->hdr);
            if (file_source)
                HeapFree(GetProcessHeap(),0,file_source);
            break;
        }

        if (MSI_RecordIsNull(row,4))
        {
            strcpyW(dest_name,strrchrW(file_source,'\\')+1);
        }
        else
        {
            sz=0x100;
            MSI_RecordGetStringW(row,4,dest_name,&sz);
            reduce_to_longfilename(dest_name);
         }

        if (MSI_RecordIsNull(row,5))
        {
            LPWSTR p;
            dest_path = dupstrW(file_source);
            p = strrchrW(dest_path,'\\');
            if (p)
                *p=0;
        }
        else
        {
            WCHAR destkey[0x100];
            sz=0x100;
            MSI_RecordGetStringW(row,5,destkey,&sz);
            sz = 0x100;
            dest_path = resolve_folder(package, destkey, FALSE,FALSE,NULL);
            if (!dest_path)
            {
                ERR("Unable to get destination folder\n");
                msiobj_release(&row->hdr);
                if (file_source)
                    HeapFree(GetProcessHeap(),0,file_source);
                break;
            }
        }

        dest = build_directory_name(2, dest_path, dest_name);
        HeapFree(GetProcessHeap(), 0, dest_path);
           
        TRACE("Duplicating file %s to %s\n",debugstr_w(file_source),
              debugstr_w(dest)); 
        
        if (strcmpW(file_source,dest))
            rc = !CopyFileW(file_source,dest,TRUE);
        else
            rc = ERROR_SUCCESS;
        
        if (rc != ERROR_SUCCESS)
            ERR("Failed to copy file\n");

        FIXME("We should track these duplicate files as well\n");   
 
        msiobj_release(&row->hdr);
        HeapFree(GetProcessHeap(),0,dest);
        HeapFree(GetProcessHeap(),0,file_source);
    }
    MSI_ViewClose(view);
    msiobj_release(&view->hdr);
    return rc;
}


/* OK this value is "interpretted" and then formatted based on the 
   first few characters */
static LPSTR parse_value(MSIPACKAGE *package, WCHAR *value, DWORD *type, 
                         DWORD *size)
{
    LPSTR data = NULL;
    if (value[0]=='#' && value[1]!='#' && value[1]!='%')
    {
        if (value[1]=='x')
        {
            LPWSTR ptr;
            CHAR byte[5];
            LPWSTR deformated;
            int count;

            deformat_string(package, &value[2], &deformated);

            /* binary value type */
            ptr = deformated; 
            *type=REG_BINARY;
            *size = strlenW(ptr)/2;
            data = HeapAlloc(GetProcessHeap(),0,*size);
          
            byte[0] = '0'; 
            byte[1] = 'x'; 
            byte[4] = 0; 
            count = 0;
            while (*ptr)
            {
                byte[2]= *ptr;
                ptr++;
                byte[3]= *ptr;
                ptr++;
                data[count] = (BYTE)strtol(byte,NULL,0);
                count ++;
            }
            HeapFree(GetProcessHeap(),0,deformated);

            TRACE("Data %li bytes(%i)\n",*size,count);
        }
        else
        {
            LPWSTR deformated;
            deformat_string(package, &value[1], &deformated);

            *type=REG_DWORD; 
            *size = sizeof(DWORD);
            data = HeapAlloc(GetProcessHeap(),0,*size);
            *(LPDWORD)data = atoiW(deformated); 
            TRACE("DWORD %i\n",*data);

            HeapFree(GetProcessHeap(),0,deformated);
        }
    }
    else
    {
        WCHAR *ptr;
        *type=REG_SZ;

        if (value[0]=='#')
        {
            if (value[1]=='%')
            {
                ptr = &value[2];
                *type=REG_EXPAND_SZ;
            }
            else
                ptr = &value[1];
         }
         else
            ptr=value;

        *size = deformat_string(package, ptr,(LPWSTR*)&data);
    }
    return data;
}

static UINT ACTION_WriteRegistryValues(MSIPACKAGE *package)
{
    UINT rc;
    MSIQUERY * view;
    MSIRECORD * row = 0;
    static const WCHAR ExecSeqQuery[] = {
        's','e','l','e','c','t',' ','*',' ',
        'f','r','o','m',' ','R','e','g','i','s','t','r','y',0 };

    if (!package)
        return ERROR_INVALID_HANDLE;

    rc = MSI_DatabaseOpenViewW(package->db, ExecSeqQuery, &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_ViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
        return rc;
    }

    /* increment progress bar each time action data is sent */
    ui_progress(package,1,REG_PROGRESS_VALUE,1,0);

    while (1)
    {
        static const WCHAR szHCR[] = 
{'H','K','E','Y','_','C','L','A','S','S','E','S','_','R','O','O','T','\\',0};
        static const WCHAR szHCU[] =
{'H','K','E','Y','_','C','U','R','R','E','N','T','_','U','S','E','R','\\',0};
        static const WCHAR szHLM[] =
{'H','K','E','Y','_','L','O','C','A','L','_','M','A','C','H','I','N','E',
'\\',0};
        static const WCHAR szHU[] =
{'H','K','E','Y','_','U','S','E','R','S','\\',0};

        LPSTR value_data = NULL;
        HKEY  root_key, hkey;
        DWORD type,size;
        LPWSTR value, key, name, component, deformated;
        LPCWSTR szRoot;
        INT component_index;
        MSIRECORD * uirow;
        LPWSTR uikey;
        INT   root;

        rc = MSI_ViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            rc = ERROR_SUCCESS;
            break;
        }
        ui_progress(package,2,0,0,0);

        value = NULL;
        key = NULL;
        uikey = NULL;
        name = NULL;

        component = load_dynamic_stringW(row, 6);
        component_index = get_loaded_component(package,component);

        if (package->components[component_index].ActionRequest != 
             INSTALLSTATE_LOCAL)
        {
            TRACE("Skipping write due to disabled component\n");
            msiobj_release(&row->hdr);
            goto next;
        }

        /* null values have special meanings during uninstalls and such */
        
        if(MSI_RecordIsNull(row,5))
        {
            msiobj_release(&row->hdr);
            goto next;
        }

        root = MSI_RecordGetInteger(row,2);
        key = load_dynamic_stringW(row, 3);
      
        name = load_dynamic_stringW(row, 4);
   
        /* get the root key */
        switch (root)
        {
            case 0:  root_key = HKEY_CLASSES_ROOT; 
                     szRoot = szHCR;
                     break;
            case 1:  root_key = HKEY_CURRENT_USER;
                     szRoot = szHCU;
                     break;
            case 2:  root_key = HKEY_LOCAL_MACHINE;
                     szRoot = szHLM;
                     break;
            case 3:  root_key = HKEY_USERS; 
                     szRoot = szHU;
                     break;
            default:
                 ERR("Unknown root %i\n",root);
                 root_key=NULL;
                 szRoot = NULL;
                 break;
        }
        if (!root_key)
        {
            msiobj_release(&row->hdr);
            goto next;
        }

        deformat_string(package, key , &deformated);
        size = strlenW(deformated) + strlenW(szRoot) + 1;
        uikey = HeapAlloc(GetProcessHeap(), 0, size*sizeof(WCHAR));
        strcpyW(uikey,szRoot);
        strcatW(uikey,deformated);

        if (RegCreateKeyW( root_key, deformated, &hkey))
        {
            ERR("Could not create key %s\n",debugstr_w(deformated));
            msiobj_release(&row->hdr);
            HeapFree(GetProcessHeap(),0,deformated);
            goto next;
        }
        HeapFree(GetProcessHeap(),0,deformated);

        value = load_dynamic_stringW(row,5);
        value_data = parse_value(package, value, &type, &size); 

        deformat_string(package, name, &deformated);

        if (value_data)
        {
            TRACE("Setting value %s\n",debugstr_w(deformated));
            RegSetValueExW(hkey, deformated, 0, type, value_data, size);

            uirow = MSI_CreateRecord(3);
            MSI_RecordSetStringW(uirow,2,deformated);
            MSI_RecordSetStringW(uirow,1,uikey);

            if (type == REG_SZ)
                MSI_RecordSetStringW(uirow,3,(LPWSTR)value_data);
            else
                MSI_RecordSetStringW(uirow,3,value);

            ui_actiondata(package,szWriteRegistryValues,uirow);
            msiobj_release( &uirow->hdr );

            HeapFree(GetProcessHeap(),0,value_data);
        }
        HeapFree(GetProcessHeap(),0,value);
        HeapFree(GetProcessHeap(),0,deformated);

        msiobj_release(&row->hdr);
        RegCloseKey(hkey);
next:
        if (uikey)
            HeapFree(GetProcessHeap(),0,uikey);
        if (key)
            HeapFree(GetProcessHeap(),0,key);
        if (name)
            HeapFree(GetProcessHeap(),0,name);
        if (component)
            HeapFree(GetProcessHeap(),0,component);
    }
    MSI_ViewClose(view);
    msiobj_release(&view->hdr);
    return rc;
}

/*
 * This helper function should probably go alot of places
 *
 * Thinking about this, maybe this should become yet another Bison file
 */
static DWORD deformat_string(MSIPACKAGE *package, WCHAR* ptr,WCHAR** data)
{
    WCHAR* mark=NULL;
    DWORD size=0;
    DWORD chunk=0;
    WCHAR key[0x100];
    LPWSTR value;
    DWORD sz;
    UINT rc;

    if (ptr==NULL)
    {
        TRACE("Deformatting NULL string\n");
        *data = NULL;
        return 0;
    }
    /* scan for special characters */
    if (!strchrW(ptr,'[') || (strchrW(ptr,'[') && !strchrW(ptr,']')))
    {
        /* not formatted */
        size = (strlenW(ptr)+1) * sizeof(WCHAR);
        *data = HeapAlloc(GetProcessHeap(),0,size);
        strcpyW(*data,ptr);
        return size;
    }
   
    /* formatted string located */ 
    mark = strchrW(ptr,'[');
    if (mark != ptr)
    {
        INT cnt = (mark - ptr);
        TRACE("%i  (%i) characters before marker\n",cnt,(mark-ptr));
        size = cnt * sizeof(WCHAR);
        size += sizeof(WCHAR);
        *data = HeapAlloc(GetProcessHeap(),0,size);
        strncpyW(*data,ptr,cnt);
        (*data)[cnt]=0;
    }
    else
    {
        size = sizeof(WCHAR);
        *data = HeapAlloc(GetProcessHeap(),0,size);
        (*data)[0]=0;
    }
    mark++;
    strcpyW(key,mark);
    *strchrW(key,']')=0;
    mark = strchrW(mark,']');
    mark++;
    TRACE("Current %s .. %s\n",debugstr_w(*data),debugstr_w(mark));
    sz = 0;
    rc = MSI_GetPropertyW(package, key, NULL, &sz);
    if ((rc == ERROR_SUCCESS) || (rc == ERROR_MORE_DATA))
    {
        LPWSTR newdata;

        sz++;
        value = HeapAlloc(GetProcessHeap(),0,sz * sizeof(WCHAR));
        MSI_GetPropertyW(package, key, value, &sz);

        chunk = (strlenW(value)+1) * sizeof(WCHAR);
        size+=chunk;   
        newdata = HeapReAlloc(GetProcessHeap(),0,*data,size);
        *data = newdata;
        strcatW(*data,value);
    }
    TRACE("Current %s .. %s\n",debugstr_w(*data),debugstr_w(mark));
    if (*mark!=0)
    {
        LPWSTR newdata;
        chunk = (strlenW(mark)+1) * sizeof(WCHAR);
        size+=chunk;
        newdata = HeapReAlloc(GetProcessHeap(),0,*data,size);
        *data = newdata;
        strcatW(*data,mark);
    }
    (*data)[strlenW(*data)]=0;
    TRACE("Current %s .. %s\n",debugstr_w(*data),debugstr_w(mark));

    /* recursively do this to clean up */
    mark = HeapAlloc(GetProcessHeap(),0,size);
    strcpyW(mark,*data);
    TRACE("String at this point %s\n",debugstr_w(mark));
    size = deformat_string(package,mark,data);
    HeapFree(GetProcessHeap(),0,mark);
    return size;
}

static UINT ACTION_InstallInitialize(MSIPACKAGE *package)
{
    return ERROR_SUCCESS;
}


static UINT ACTION_InstallValidate(MSIPACKAGE *package)
{
    DWORD progress = 0;
    DWORD total = 0;
    static const WCHAR q1[]={
        'S','E','L','E','C','T',' ','*',' ',
        'F','R','O','M',' ','R','e','g','i','s','t','r','y',0};
    UINT rc;
    MSIQUERY * view;
    MSIRECORD * row = 0;
    int i;

    TRACE(" InstallValidate \n");

    rc = MSI_DatabaseOpenViewW(package->db, q1, &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_ViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
        return rc;
    }
    while (1)
    {
        rc = MSI_ViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            rc = ERROR_SUCCESS;
            break;
        }
        progress +=1;

        msiobj_release(&row->hdr);
    }
    MSI_ViewClose(view);
    msiobj_release(&view->hdr);

    total = total + progress * REG_PROGRESS_VALUE;
    total = total + package->loaded_components * COMPONENT_PROGRESS_VALUE;
    for (i=0; i < package->loaded_files; i++)
        total += package->files[i].FileSize;
    ui_progress(package,0,total,0,0);

    return ERROR_SUCCESS;
}

static UINT ACTION_LaunchConditions(MSIPACKAGE *package)
{
    UINT rc;
    MSIQUERY * view = NULL;
    MSIRECORD * row = 0;
    static const WCHAR ExecSeqQuery[] = {
        'S','E','L','E','C','T',' ','*',' ',
        'f','r','o','m',' ','L','a','u','n','c','h','C','o','n','d','i','t','i','o','n',0};
    static const WCHAR title[]=
            {'I','n','s','t','a','l','l',' ','F','a', 'i','l','e','d',0};

    TRACE("Checking launch conditions\n");

    rc = MSI_DatabaseOpenViewW(package->db, ExecSeqQuery, &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_ViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
        return rc;
    }

    rc = ERROR_SUCCESS;
    while (rc == ERROR_SUCCESS)
    {
        LPWSTR cond = NULL; 
        LPWSTR message = NULL;

        rc = MSI_ViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            rc = ERROR_SUCCESS;
            break;
        }

        cond = load_dynamic_stringW(row,1);

        if (MSI_EvaluateConditionW(package,cond) != MSICONDITION_TRUE)
        {
            message = load_dynamic_stringW(row,2);
            MessageBoxW(NULL,message,title,MB_OK);
            HeapFree(GetProcessHeap(),0,message);
            rc = ERROR_FUNCTION_FAILED;
        }
        HeapFree(GetProcessHeap(),0,cond);
        msiobj_release(&row->hdr);
    }
    MSI_ViewClose(view);
    msiobj_release(&view->hdr);
    return rc;
}

static LPWSTR resolve_keypath( MSIPACKAGE* package, INT
                            component_index)
{
    MSICOMPONENT* cmp = &package->components[component_index];

    if (cmp->KeyPath[0]==0)
    {
        LPWSTR p = resolve_folder(package,cmp->Directory,FALSE,FALSE,NULL);
        return p;
    }
    if ((cmp->Attributes & 0x4) || (cmp->Attributes & 0x20))
    {
        FIXME("UNIMPLEMENTED keypath as Registry or ODBC Source\n");
        return NULL;
    }
    else
    {
        int j;
        j = get_loaded_file(package,cmp->KeyPath);

        if (j>=0)
        {
            LPWSTR p = dupstrW(package->files[j].TargetPath);
            return p;
        }
    }
    return NULL;
}

/*
 * Ok further analysis makes me think that this work is
 * actually done in the PublishComponents and PublishFeatures
 * step, and not here.  It appears like the keypath and all that is
 * resolved in this step, however actually written in the Publish steps.
 * But we will leave it here for now because it is unclear
 */
static UINT ACTION_ProcessComponents(MSIPACKAGE *package)
{
    LPWSTR productcode;
    WCHAR squished_pc[0x100];
    WCHAR squished_cc[0x100];
    UINT rc;
    DWORD i;
    HKEY hkey=0,hkey2=0,hkey3=0;
    static const WCHAR szProductCode[]=
         {'P','r','o','d','u','c','t','C','o','d','e',0};
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

    if (!package)
        return ERROR_INVALID_HANDLE;

    /* writes the Component and Features values to the registry */
    productcode = load_dynamic_property(package,szProductCode,&rc);
    if (!productcode)
        return rc;

    squash_guid(productcode,squished_pc);
    rc = RegCreateKeyW(HKEY_LOCAL_MACHINE,szInstaller,&hkey);
    if (rc != ERROR_SUCCESS)
        goto end;

    rc = RegCreateKeyW(hkey,szFeatures,&hkey2);
    if (rc != ERROR_SUCCESS)
        goto end;

    rc = RegCreateKeyW(hkey2,squished_pc,&hkey3);
    if (rc != ERROR_SUCCESS)
        goto end;

    /* here the guids are base 85 encoded */
    for (i = 0; i < package->loaded_features; i++)
    {
        LPWSTR data = NULL;
        GUID clsid;
        int j;
        INT size;

        size = package->features[i].ComponentCount*21*sizeof(WCHAR);
        data = HeapAlloc(GetProcessHeap(), 0, size);

        data[0] = 0;
        for (j = 0; j < package->features[i].ComponentCount; j++)
        {
            WCHAR buf[21];
            TRACE("From %s\n",debugstr_w(package->components
                            [package->features[i].Components[j]].ComponentId));
            CLSIDFromString(package->components
                            [package->features[i].Components[j]].ComponentId,
                            &clsid);
            encode_base85_guid(&clsid,buf);
            TRACE("to %s\n",debugstr_w(buf));
            strcatW(data,buf);
        }

        size = strlenW(data)*sizeof(WCHAR);
        RegSetValueExW(hkey3,package->features[i].Feature,0,REG_SZ,
                       (LPSTR)data,size);
        HeapFree(GetProcessHeap(),0,data);
    }

    RegCloseKey(hkey3);
    RegCloseKey(hkey2);

    rc = RegCreateKeyW(hkey,szComponents,&hkey2);
    if (rc != ERROR_SUCCESS)
        goto end;
      
    ui_progress(package,1,COMPONENT_PROGRESS_VALUE,1,0);
    for (i = 0; i < package->loaded_components; i++)
    {
        ui_progress(package,2,0,0,0);
        if (package->components[i].ComponentId[0]!=0)
        {
            WCHAR *keypath = NULL;
            MSIRECORD * uirow;

            squash_guid(package->components[i].ComponentId,squished_cc);
            rc = RegCreateKeyW(hkey2,squished_cc,&hkey3);
            if (rc != ERROR_SUCCESS)
                continue;
           
            keypath = resolve_keypath(package,i);
            if (keypath)
            {
                RegSetValueExW(hkey3,squished_pc,0,REG_SZ,(LPVOID)keypath,
                            (strlenW(keypath)+1)*sizeof(WCHAR));
                RegCloseKey(hkey3);
        
                /* UI stuff */
                uirow = MSI_CreateRecord(3);
                MSI_RecordSetStringW(uirow,1,productcode);
                MSI_RecordSetStringW(uirow,2,package->components[i].
                                                        ComponentId);
                MSI_RecordSetStringW(uirow,3,keypath);
                ui_actiondata(package,szProcessComponents,uirow);
                msiobj_release( &uirow->hdr );
                HeapFree(GetProcessHeap(),0,keypath);
            }
        }
    } 
end:
    HeapFree(GetProcessHeap(), 0, productcode);
    RegCloseKey(hkey2);
    RegCloseKey(hkey);
    return rc;
}

static UINT ACTION_RegisterTypeLibraries(MSIPACKAGE *package)
{
    /* 
     * OK this is a bit confusing.. I am given a _Component key and I believe
     * that the file that is being registered as a type library is the "key file
     * of that component" which I interpret to mean "The file in the KeyPath of
     * that component".
     */
    UINT rc;
    MSIQUERY * view;
    MSIRECORD * row = 0;
    static const WCHAR Query[] = {
        'S','E','L','E','C','T',' ','*',' ',
        'f','r','o','m',' ','T','y','p','e','L','i','b',0};
    ITypeLib *ptLib;
    HRESULT res;

    if (!package)
        return ERROR_INVALID_HANDLE;

    rc = MSI_DatabaseOpenViewW(package->db, Query, &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_ViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
        return rc;
    }

    while (1)
    {   
        WCHAR component[0x100];
        DWORD sz;
        INT index;

        rc = MSI_ViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            rc = ERROR_SUCCESS;
            break;
        }

        sz = 0x100;
        MSI_RecordGetStringW(row,3,component,&sz);

        index = get_loaded_component(package,component);
        if (index < 0)
        {
            msiobj_release(&row->hdr);
            continue;
        }

        if (package->components[index].ActionRequest != INSTALLSTATE_LOCAL)
        {
            TRACE("Skipping typelib reg due to disabled component\n");
            msiobj_release(&row->hdr);
            continue;
        }

        index = get_loaded_file(package,package->components[index].KeyPath); 
   
        if (index < 0)
        {
            msiobj_release(&row->hdr);
            continue;
        }

        res = LoadTypeLib(package->files[index].TargetPath,&ptLib);
        if (SUCCEEDED(res))
        {
            LPWSTR help;
            WCHAR helpid[0x100];

            sz = 0x100;
            MSI_RecordGetStringW(row,6,helpid,&sz);

            help = resolve_folder(package,helpid,FALSE,FALSE,NULL);
            res = RegisterTypeLib(ptLib,package->files[index].TargetPath,help);
            HeapFree(GetProcessHeap(),0,help);

            if (!SUCCEEDED(res))
                ERR("Failed to register type library %s\n",
                     debugstr_w(package->files[index].TargetPath));
            else
            {
                /* Yes the row has more fields than I need, but #1 is 
                   correct and the only one I need. Why make a new row? */

                ui_actiondata(package,szRegisterTypeLibraries,row);
                
                TRACE("Registered %s\n",
                       debugstr_w(package->files[index].TargetPath));
            }

            if (ptLib)
                ITypeLib_Release(ptLib);
        }
        else
            ERR("Failed to load type library %s\n",
                debugstr_w(package->files[index].TargetPath));
        
        msiobj_release(&row->hdr);
    }
    MSI_ViewClose(view);
    msiobj_release(&view->hdr);
    return rc;
   
}

static UINT register_appid(MSIPACKAGE *package, LPCWSTR clsid, LPCWSTR app )
{
    static const WCHAR szAppID[] = { 'A','p','p','I','D',0 };
    UINT rc;
    MSIQUERY * view;
    MSIRECORD * row = 0;
    static const WCHAR ExecSeqQuery[] = 
        {'S','E','L','E','C','T',' ','*',' ','f','r','o','m',' ','A','p','p','I'
        ,'d',' ','w','h','e','r','e',' ','A','p','p','I','d','=','`','%','s','`',0};
    HKEY hkey2,hkey3;
    LPWSTR buffer=0;

    if (!package)
        return ERROR_INVALID_HANDLE;

    rc = ACTION_OpenQuery(package->db, &view, ExecSeqQuery, clsid);
    if (rc != ERROR_SUCCESS)
        return rc;

    rc = MSI_ViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
        return rc;
    }

    RegCreateKeyW(HKEY_CLASSES_ROOT,szAppID,&hkey2);
    RegCreateKeyW(hkey2,clsid,&hkey3);
    RegSetValueExW(hkey3,NULL,0,REG_SZ,(LPVOID)app,
                   (strlenW(app)+1)*sizeof(WCHAR));

    rc = MSI_ViewFetch(view,&row);
    if (rc != ERROR_SUCCESS)
    {
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
        return rc;
    }

    if (!MSI_RecordIsNull(row,2)) 
    {
        LPWSTR deformated=0;
        UINT size; 
        static const WCHAR szRemoteServerName[] =
             {'R','e','m','o','t','e','S','e','r','v','e','r','N','a','m','e',0};
        buffer = load_dynamic_stringW(row,2);
        size = deformat_string(package,buffer,&deformated);
        RegSetValueExW(hkey3,szRemoteServerName,0,REG_SZ,(LPVOID)deformated,
                       size);
        HeapFree(GetProcessHeap(),0,deformated);
        HeapFree(GetProcessHeap(),0,buffer);
    }

    if (!MSI_RecordIsNull(row,3)) 
    {
        static const WCHAR szLocalService[] =
             {'L','o','c','a','l','S','e','r','v','i','c','e',0};
        UINT size;
        buffer = load_dynamic_stringW(row,3);
        size = (strlenW(buffer)+1) * sizeof(WCHAR);
        RegSetValueExW(hkey3,szLocalService,0,REG_SZ,(LPVOID)buffer,size);
        HeapFree(GetProcessHeap(),0,buffer);
    }

    if (!MSI_RecordIsNull(row,4)) 
    {
        static const WCHAR szService[] =
             {'S','e','r','v','i','c','e','P','a','r','a','m','e','t','e','r','s',0};
        UINT size;
        buffer = load_dynamic_stringW(row,4);
        size = (strlenW(buffer)+1) * sizeof(WCHAR);
        RegSetValueExW(hkey3,szService,0,REG_SZ,(LPVOID)buffer,size);
        HeapFree(GetProcessHeap(),0,buffer);
    }

    if (!MSI_RecordIsNull(row,5)) 
    {
        static const WCHAR szDLL[] =
             {'D','l','l','S','u','r','r','o','g','a','t','e',0};
        UINT size;
        buffer = load_dynamic_stringW(row,5);
        size = (strlenW(buffer)+1) * sizeof(WCHAR);
        RegSetValueExW(hkey3,szDLL,0,REG_SZ,(LPVOID)buffer,size);
        HeapFree(GetProcessHeap(),0,buffer);
    }

    if (!MSI_RecordIsNull(row,6)) 
    {
        static const WCHAR szActivate[] =
             {'A','c','t','i','v','a','t','e','A','s','S','t','o','r','a','g','e',0};
        static const WCHAR szY[] = {'Y',0};

        if (MSI_RecordGetInteger(row,6))
            RegSetValueExW(hkey3,szActivate,0,REG_SZ,(LPVOID)szY,4);
    }

    if (!MSI_RecordIsNull(row,7)) 
    {
        static const WCHAR szRunAs[] = {'R','u','n','A','s',0};
        static const WCHAR szUser[] = 
             {'I','n','t','e','r','a','c','t','i','v','e',' ','U','s','e','r',0};

        if (MSI_RecordGetInteger(row,7))
            RegSetValueExW(hkey3,szRunAs,0,REG_SZ,(LPVOID)szUser,34);
    }

    msiobj_release(&row->hdr);
    MSI_ViewClose(view);
    msiobj_release(&view->hdr);
    RegCloseKey(hkey3);
    RegCloseKey(hkey2);
    return rc;
}

static UINT ACTION_RegisterClassInfo(MSIPACKAGE *package)
{
    /* 
     * Again I am assuming the words, "Whose key file represents" when referring
     * to a Component as to meaning that Components KeyPath file
     *
     * Also there is a very strong connection between ClassInfo and ProgID
     * that I am mostly glossing over.  
     * What would be more propper is to load the ClassInfo and the ProgID info
     * into memory data structures and then be able to enable and disable them
     * based on component. 
     */
    
    UINT rc;
    MSIQUERY * view;
    MSIRECORD * row = 0;
    static const WCHAR ExecSeqQuery[] = {
        'S','E','L','E','C','T',' ','*',' ',
        'f','r','o','m',' ','C','l','a','s','s',0};
    static const WCHAR szCLSID[] = { 'C','L','S','I','D',0 };
    static const WCHAR szProgID[] = { 'P','r','o','g','I','D',0 };
    static const WCHAR szAppID[] = { 'A','p','p','I','D',0 };
    HKEY hkey,hkey2,hkey3;

    if (!package)
        return ERROR_INVALID_HANDLE;

    rc = RegCreateKeyW(HKEY_CLASSES_ROOT,szCLSID,&hkey);
    if (rc != ERROR_SUCCESS)
        return ERROR_FUNCTION_FAILED;

    rc = MSI_DatabaseOpenViewW(package->db, ExecSeqQuery, &view);
    if (rc != ERROR_SUCCESS)
    {
        rc = ERROR_SUCCESS;
        goto end;
    }

    rc = MSI_ViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
        goto end;
    }

    while (1)
    {
        WCHAR clsid[0x100];
        WCHAR buffer[0x100];
        WCHAR desc[0x100];
        DWORD sz;
        INT index;
     
        rc = MSI_ViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            rc = ERROR_SUCCESS;
            break;
        }

        sz=0x100;
        MSI_RecordGetStringW(row,3,buffer,&sz);

        index = get_loaded_component(package,buffer);

        if (index < 0)
        {
            msiobj_release(&row->hdr);
            continue;
        }

        if (package->components[index].ActionRequest != INSTALLSTATE_LOCAL)
        {
            TRACE("Skipping class reg due to disabled component\n");
            msiobj_release(&row->hdr);
            continue;
        }

        sz=0x100;
        MSI_RecordGetStringW(row,1,clsid,&sz);
        RegCreateKeyW(hkey,clsid,&hkey2);

        if (!MSI_RecordIsNull(row,5))
        {
            sz=0x100;
            MSI_RecordGetStringW(row,5,desc,&sz);

            RegSetValueExW(hkey2,NULL,0,REG_SZ,(LPVOID)desc,
                           (strlenW(desc)+1)*sizeof(WCHAR));
        }
        else
            desc[0]=0;

        sz=0x100;
        MSI_RecordGetStringW(row,2,buffer,&sz);

        RegCreateKeyW(hkey2,buffer,&hkey3);

        index = get_loaded_file(package,package->components[index].KeyPath);
        RegSetValueExW(hkey3,NULL,0,REG_SZ,
                       (LPVOID)package->files[index].TargetPath,
                       (strlenW(package->files[index].TargetPath)+1)
                        *sizeof(WCHAR));

        RegCloseKey(hkey3);

        if (!MSI_RecordIsNull(row,4))
        {
            sz=0x100;
            MSI_RecordGetStringW(row,4,buffer,&sz);

            RegCreateKeyW(hkey2,szProgID,&hkey3);
    
            RegSetValueExW(hkey3,NULL,0,REG_SZ,(LPVOID)buffer,
                       (strlenW(buffer)+1)*sizeof(WCHAR));

            RegCloseKey(hkey3);
        }

        if (!MSI_RecordIsNull(row,6))
        { 
            sz=0x100;
            MSI_RecordGetStringW(row,6,buffer,&sz);

            RegSetValueExW(hkey2,szAppID,0,REG_SZ,(LPVOID)buffer,
                       (strlenW(buffer)+1)*sizeof(WCHAR));

            register_appid(package,buffer,desc);
        }

        RegCloseKey(hkey2);

        FIXME("Process the rest of the fields >7\n");

        ui_actiondata(package,szRegisterClassInfo,row);

        msiobj_release(&row->hdr);
    }
    MSI_ViewClose(view);
    msiobj_release(&view->hdr);

end:
    RegCloseKey(hkey);
    return rc;
}

static UINT register_progid_base(MSIRECORD * row, LPWSTR clsid)
{
    static const WCHAR szCLSID[] = { 'C','L','S','I','D',0 };
    HKEY hkey,hkey2;
    WCHAR buffer[0x100];
    DWORD sz;


    sz = 0x100;
    MSI_RecordGetStringW(row,1,buffer,&sz);
    RegCreateKeyW(HKEY_CLASSES_ROOT,buffer,&hkey);

    if (!MSI_RecordIsNull(row,4))
    {
        sz = 0x100;
        MSI_RecordGetStringW(row,4,buffer,&sz);
        RegSetValueExW(hkey,NULL,0,REG_SZ,(LPVOID)buffer, (strlenW(buffer)+1) *
                       sizeof(WCHAR));
    }

    if (!MSI_RecordIsNull(row,3))
    {   
        sz = 0x100;
    
        MSI_RecordGetStringW(row,3,buffer,&sz);
        RegCreateKeyW(hkey,szCLSID,&hkey2);
        RegSetValueExW(hkey2,NULL,0,REG_SZ,(LPVOID)buffer, (strlenW(buffer)+1) *
                       sizeof(WCHAR));

        if (clsid)
            strcpyW(clsid,buffer);

        RegCloseKey(hkey2);
    }
    else
    {
        FIXME("UNHANDLED case, Parent progid but classid is NULL\n");
        return ERROR_FUNCTION_FAILED;
    }
    if (!MSI_RecordIsNull(row,5))
        FIXME ("UNHANDLED icon in Progid\n");
    return ERROR_SUCCESS;
}

static UINT register_progid(MSIPACKAGE *package, MSIRECORD * row, LPWSTR clsid);

static UINT register_parent_progid(MSIPACKAGE *package, LPCWSTR parent, 
                                   LPWSTR clsid)
{
    UINT rc;
    MSIQUERY * view;
    MSIRECORD * row = 0;
    static const WCHAR Query_t[] = 
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','P','r','o','g'
        ,'I','d',' ','w','h','e','r','e',' ','P','r','o','g','I','d',' ','=',' ','`'
        ,'%','s','`',0};

    if (!package)
        return ERROR_INVALID_HANDLE;

    rc = ACTION_OpenQuery(package->db, &view, Query_t, parent);
    if (rc != ERROR_SUCCESS)
        return rc;

    rc = MSI_ViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
        return rc;
    }

    rc = MSI_ViewFetch(view,&row);
    if (rc != ERROR_SUCCESS)
    {
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
        return rc;
    }

    register_progid(package,row,clsid);

    msiobj_release(&row->hdr);
    MSI_ViewClose(view);
    msiobj_release(&view->hdr);
    return rc;
}

static UINT register_progid(MSIPACKAGE *package, MSIRECORD * row, LPWSTR clsid)
{
    UINT rc = ERROR_SUCCESS; 

    if (MSI_RecordIsNull(row,2))
        rc = register_progid_base(row,clsid);
    else
    {
        WCHAR buffer[0x1000];
        DWORD sz, disp;
        HKEY hkey,hkey2;
        static const WCHAR szCLSID[] = { 'C','L','S','I','D',0 };

        /* check if already registered */
        sz = 0x100;
        MSI_RecordGetStringW(row,1,buffer,&sz);
        RegCreateKeyExW(HKEY_CLASSES_ROOT, buffer, 0, NULL, 0,
                        KEY_ALL_ACCESS, NULL, &hkey, &disp );
        if (disp == REG_OPENED_EXISTING_KEY)
        {
            TRACE("Key already registered\n");
            RegCloseKey(hkey);
            return rc;
        }
        /* clsid is same as parent */
        RegCreateKeyW(hkey,szCLSID,&hkey2);
        RegSetValueExW(hkey2,NULL,0,REG_SZ,(LPVOID)clsid, (strlenW(clsid)+1) *
                       sizeof(WCHAR));

        RegCloseKey(hkey2);

        sz = 0x100;
        MSI_RecordGetStringW(row,2,buffer,&sz);
        rc = register_parent_progid(package,buffer,clsid);

        if (!MSI_RecordIsNull(row,4))
        {
            sz = 0x100;
            MSI_RecordGetStringW(row,4,buffer,&sz);
            RegSetValueExW(hkey,NULL,0,REG_SZ,(LPVOID)buffer,
                           (strlenW(buffer)+1) * sizeof(WCHAR));
        }

        if (!MSI_RecordIsNull(row,5))
            FIXME ("UNHANDLED icon in Progid\n");

        RegCloseKey(hkey);
    }
    return rc;
}

static UINT ACTION_RegisterProgIdInfo(MSIPACKAGE *package)
{
    /* 
     * Sigh, here I am just brute force registering all progids
     * this needs to be linked to the Classes that have been registered
     * but the easiest way to do that is to load all these stuff into
     * memory for easy checking.
     *
     * Gives me something to continue to work toward.
     */
    UINT rc;
    MSIQUERY * view;
    MSIRECORD * row = 0;
    static const WCHAR Query[] = {
        'S','E','L','E','C','T',' ','*',' ',
        'F','R','O','M',' ','P','r','o','g','I','d',0};

    if (!package)
        return ERROR_INVALID_HANDLE;

    rc = MSI_DatabaseOpenViewW(package->db, Query, &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_ViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
        return rc;
    }

    while (1)
    {
        WCHAR clsid[0x1000];

        rc = MSI_ViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            rc = ERROR_SUCCESS;
            break;
        }
        
        register_progid(package,row,clsid);
        ui_actiondata(package,szRegisterProgIdInfo,row);

        msiobj_release(&row->hdr);
    }
    MSI_ViewClose(view);
    msiobj_release(&view->hdr);
    return rc;
}

static UINT build_icon_path(MSIPACKAGE *package, LPCWSTR icon_name, 
                            LPWSTR *FilePath)
{
    LPWSTR ProductCode;
    LPWSTR SystemFolder;
    LPWSTR dest;
    UINT rc;

    static const WCHAR szInstaller[] = 
        {'I','n','s','t','a','l','l','e','r','\\',0};
    static const WCHAR szProductCode[] =
        {'P','r','o','d','u','c','t','C','o','d','e',0};
    static const WCHAR szFolder[] =
        {'W','i','n','d','o','w','s','F','o','l','d','e','r',0};

    ProductCode = load_dynamic_property(package,szProductCode,&rc);
    if (!ProductCode)
        return rc;

    SystemFolder = load_dynamic_property(package,szFolder,NULL);

    dest = build_directory_name(3, SystemFolder, szInstaller, ProductCode);

    create_full_pathW(dest);

    *FilePath = build_directory_name(2, dest, icon_name);

    HeapFree(GetProcessHeap(),0,SystemFolder);
    HeapFree(GetProcessHeap(),0,ProductCode);
    HeapFree(GetProcessHeap(),0,dest);
    return ERROR_SUCCESS;
}

static UINT ACTION_CreateShortcuts(MSIPACKAGE *package)
{
    UINT rc;
    MSIQUERY * view;
    MSIRECORD * row = 0;
    static const WCHAR Query[] = {
       'S','E','L','E','C','T',' ','*',' ','f','r','o','m',' ',
       'S','h','o','r','t','c','u','t',0};
    IShellLinkW *sl;
    IPersistFile *pf;
    HRESULT res;

    if (!package)
        return ERROR_INVALID_HANDLE;

    res = CoInitialize( NULL );
    if (FAILED (res))
    {
        ERR("CoInitialize failed\n");
        return ERROR_FUNCTION_FAILED;
    }

    rc = MSI_DatabaseOpenViewW(package->db, Query, &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_ViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
        return rc;
    }

    while (1)
    {
        LPWSTR target_file, target_folder;
        WCHAR buffer[0x100];
        DWORD sz;
        DWORD index;
        static const WCHAR szlnk[]={'.','l','n','k',0};

        rc = MSI_ViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            rc = ERROR_SUCCESS;
            break;
        }
        
        sz = 0x100;
        MSI_RecordGetStringW(row,4,buffer,&sz);

        index = get_loaded_component(package,buffer);

        if (index < 0)
        {
            msiobj_release(&row->hdr);
            continue;
        }

        if (package->components[index].ActionRequest != INSTALLSTATE_LOCAL)
        {
            TRACE("Skipping shortcut creation due to disabled component\n");
            msiobj_release(&row->hdr);
            continue;
        }

        ui_actiondata(package,szCreateShortcuts,row);

        res = CoCreateInstance( &CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                              &IID_IShellLinkW, (LPVOID *) &sl );

        if (FAILED(res))
        {
            ERR("Is IID_IShellLink\n");
            msiobj_release(&row->hdr);
            continue;
        }

        res = IShellLinkW_QueryInterface( sl, &IID_IPersistFile,(LPVOID*) &pf );
        if( FAILED( res ) )
        {
            ERR("Is IID_IPersistFile\n");
            msiobj_release(&row->hdr);
            continue;
        }

        sz = 0x100;
        MSI_RecordGetStringW(row,2,buffer,&sz);
        target_folder = resolve_folder(package, buffer,FALSE,FALSE,NULL);

        /* may be needed because of a bug somehwere else */
        create_full_pathW(target_folder);

        sz = 0x100;
        MSI_RecordGetStringW(row,3,buffer,&sz);
        reduce_to_longfilename(buffer);
        if (!strchrW(buffer,'.') || strcmpiW(strchrW(buffer,'.'),szlnk))
            strcatW(buffer,szlnk);
        target_file = build_directory_name(2, target_folder, buffer);
        HeapFree(GetProcessHeap(),0,target_folder);

        sz = 0x100;
        MSI_RecordGetStringW(row,5,buffer,&sz);
        if (strchrW(buffer,'['))
        {
            LPWSTR deformated;
            deformat_string(package,buffer,&deformated);
            IShellLinkW_SetPath(sl,deformated);
            HeapFree(GetProcessHeap(),0,deformated);
        }
        else
        {
            FIXME("UNHANDLED shortcut format, advertised shortcut\n");
            IPersistFile_Release( pf );
            IShellLinkW_Release( sl );
            msiobj_release(&row->hdr);
            continue;
        }

        if (!MSI_RecordIsNull(row,6))
        {
            LPWSTR deformated;
            sz = 0x100;
            MSI_RecordGetStringW(row,6,buffer,&sz);
            deformat_string(package,buffer,&deformated);
            IShellLinkW_SetArguments(sl,deformated);
            HeapFree(GetProcessHeap(),0,deformated);
        }

        if (!MSI_RecordIsNull(row,7))
        {
            LPWSTR deformated;
            deformated = load_dynamic_stringW(row,7);
            IShellLinkW_SetDescription(sl,deformated);
            HeapFree(GetProcessHeap(),0,deformated);
        }

        if (!MSI_RecordIsNull(row,8))
            IShellLinkW_SetHotkey(sl,MSI_RecordGetInteger(row,8));

        if (!MSI_RecordIsNull(row,9))
        {
            WCHAR *Path = NULL;
            INT index; 

            sz = 0x100;
            MSI_RecordGetStringW(row,9,buffer,&sz);

            build_icon_path(package,buffer,&Path);
            index = MSI_RecordGetInteger(row,10);

            IShellLinkW_SetIconLocation(sl,Path,index);
            HeapFree(GetProcessHeap(),0,Path);
        }

        if (!MSI_RecordIsNull(row,11))
            IShellLinkW_SetShowCmd(sl,MSI_RecordGetInteger(row,11));

        if (!MSI_RecordIsNull(row,12))
        {
            LPWSTR Path;
            sz = 0x100;
            MSI_RecordGetStringW(row,12,buffer,&sz);
            Path = resolve_folder(package, buffer, FALSE, FALSE, NULL);
            IShellLinkW_SetWorkingDirectory(sl,Path);
            HeapFree(GetProcessHeap(), 0, Path);
        }

        TRACE("Writing shortcut to %s\n",debugstr_w(target_file));
        IPersistFile_Save(pf,target_file,FALSE);
    
        HeapFree(GetProcessHeap(),0,target_file);    

        IPersistFile_Release( pf );
        IShellLinkW_Release( sl );

        msiobj_release(&row->hdr);
    }
    MSI_ViewClose(view);
    msiobj_release(&view->hdr);


    CoUninitialize();

    return rc;
}


/*
 * 99% of the work done here is only done for 
 * advertised installs. However this is where the
 * Icon table is processed and written out
 * so that is what I am going to do here.
 */
static UINT ACTION_PublishProduct(MSIPACKAGE *package)
{
    UINT rc;
    MSIQUERY * view;
    MSIRECORD * row = 0;
    static const WCHAR Query[]={
        'S','E','L','E','C','T',' ','*',' ',
        'f','r','o','m',' ','I','c','o','n',0};
    DWORD sz;

    if (!package)
        return ERROR_INVALID_HANDLE;

    rc = MSI_DatabaseOpenViewW(package->db, Query, &view);
    if (rc != ERROR_SUCCESS)
        return ERROR_SUCCESS;

    rc = MSI_ViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
        return rc;
    }

    while (1)
    {
        HANDLE the_file;
        WCHAR *FilePath=NULL;
        WCHAR *FileName=NULL;
        CHAR buffer[1024];

        rc = MSI_ViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            rc = ERROR_SUCCESS;
            break;
        }
    
        FileName = load_dynamic_stringW(row,1);
        if (!FileName)
        {
            ERR("Unable to get FileName\n");
            msiobj_release(&row->hdr);
            continue;
        }

        build_icon_path(package,FileName,&FilePath);

        HeapFree(GetProcessHeap(),0,FileName);

        TRACE("Creating icon file at %s\n",debugstr_w(FilePath));
        
        the_file = CreateFileW(FilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, NULL);

        if (the_file == INVALID_HANDLE_VALUE)
        {
            ERR("Unable to create file %s\n",debugstr_w(FilePath));
            msiobj_release(&row->hdr);
            HeapFree(GetProcessHeap(),0,FilePath);
            continue;
        }

        do 
        {
            DWORD write;
            sz = 1024;
            rc = MSI_RecordReadStream(row,2,buffer,&sz);
            if (rc != ERROR_SUCCESS)
            {
                ERR("Failed to get stream\n");
                CloseHandle(the_file);  
                DeleteFileW(FilePath);
                break;
            }
            WriteFile(the_file,buffer,sz,&write,NULL);
        } while (sz == 1024);

        HeapFree(GetProcessHeap(),0,FilePath);

        CloseHandle(the_file);
        msiobj_release(&row->hdr);
    }
    MSI_ViewClose(view);
    msiobj_release(&view->hdr);
    return rc;

}

/* Msi functions that seem appropriate here */
UINT WINAPI MsiDoActionA( MSIHANDLE hInstall, LPCSTR szAction )
{
    LPWSTR szwAction;
    UINT rc;

    TRACE(" exteral attempt at action %s\n",szAction);

    if (!szAction)
        return ERROR_FUNCTION_FAILED;
    if (hInstall == 0)
        return ERROR_FUNCTION_FAILED;

    szwAction = strdupAtoW(szAction);

    if (!szwAction)
        return ERROR_FUNCTION_FAILED; 


    rc = MsiDoActionW(hInstall, szwAction);
    HeapFree(GetProcessHeap(),0,szwAction);
    return rc;
}

UINT WINAPI MsiDoActionW( MSIHANDLE hInstall, LPCWSTR szAction )
{
    MSIPACKAGE *package;
    UINT ret = ERROR_INVALID_HANDLE;

    TRACE(" external attempt at action %s \n",debugstr_w(szAction));

    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);
    if( package )
    {
        ret = ACTION_PerformAction(package,szAction);
        msiobj_release( &package->hdr );
    }
    return ret;
}

UINT WINAPI MsiGetTargetPathA( MSIHANDLE hInstall, LPCSTR szFolder, 
                               LPSTR szPathBuf, DWORD* pcchPathBuf) 
{
    LPWSTR szwFolder;
    LPWSTR szwPathBuf;
    UINT rc;

    TRACE("getting folder %s %p %li\n",szFolder,szPathBuf, *pcchPathBuf);

    if (!szFolder)
        return ERROR_FUNCTION_FAILED;
    if (hInstall == 0)
        return ERROR_FUNCTION_FAILED;

    szwFolder = strdupAtoW(szFolder);

    if (!szwFolder)
        return ERROR_FUNCTION_FAILED; 

    szwPathBuf = HeapAlloc( GetProcessHeap(), 0 , *pcchPathBuf * sizeof(WCHAR));

    rc = MsiGetTargetPathW(hInstall, szwFolder, szwPathBuf,pcchPathBuf);

    WideCharToMultiByte( CP_ACP, 0, szwPathBuf, *pcchPathBuf, szPathBuf,
                         *pcchPathBuf, NULL, NULL );

    HeapFree(GetProcessHeap(),0,szwFolder);
    HeapFree(GetProcessHeap(),0,szwPathBuf);

    return rc;
}

UINT WINAPI MsiGetTargetPathW( MSIHANDLE hInstall, LPCWSTR szFolder, LPWSTR
                                szPathBuf, DWORD* pcchPathBuf) 
{
    LPWSTR path;
    UINT rc = ERROR_FUNCTION_FAILED;
    MSIPACKAGE *package;

    TRACE("(%s %p %li)\n",debugstr_w(szFolder),szPathBuf,*pcchPathBuf);

    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);
    if (!package)
        return ERROR_INVALID_HANDLE;
    path = resolve_folder(package, szFolder, FALSE, FALSE, NULL);
    msiobj_release( &package->hdr );

    if (path && (strlenW(path) > *pcchPathBuf))
    {
        *pcchPathBuf = strlenW(path)+1;
        rc = ERROR_MORE_DATA;
    }
    else if (path)
    {
        *pcchPathBuf = strlenW(path)+1;
        strcpyW(szPathBuf,path);
        TRACE("Returning Path %s\n",debugstr_w(path));
        rc = ERROR_SUCCESS;
    }
    HeapFree(GetProcessHeap(),0,path);
    
    return rc;
}


UINT WINAPI MsiGetSourcePathA( MSIHANDLE hInstall, LPCSTR szFolder, 
                               LPSTR szPathBuf, DWORD* pcchPathBuf) 
{
    LPWSTR szwFolder;
    LPWSTR szwPathBuf;
    UINT rc;

    TRACE("getting source %s %p %li\n",szFolder,szPathBuf, *pcchPathBuf);

    if (!szFolder)
        return ERROR_FUNCTION_FAILED;
    if (hInstall == 0)
        return ERROR_FUNCTION_FAILED;

    szwFolder = strdupAtoW(szFolder);
    if (!szwFolder)
        return ERROR_FUNCTION_FAILED; 

    szwPathBuf = HeapAlloc( GetProcessHeap(), 0 , *pcchPathBuf * sizeof(WCHAR));

    rc = MsiGetSourcePathW(hInstall, szwFolder, szwPathBuf,pcchPathBuf);

    WideCharToMultiByte( CP_ACP, 0, szwPathBuf, *pcchPathBuf, szPathBuf,
                         *pcchPathBuf, NULL, NULL );

    HeapFree(GetProcessHeap(),0,szwFolder);
    HeapFree(GetProcessHeap(),0,szwPathBuf);

    return rc;
}

UINT WINAPI MsiGetSourcePathW( MSIHANDLE hInstall, LPCWSTR szFolder, LPWSTR
                                szPathBuf, DWORD* pcchPathBuf) 
{
    LPWSTR path;
    UINT rc = ERROR_FUNCTION_FAILED;
    MSIPACKAGE *package;

    TRACE("(%s %p %li)\n",debugstr_w(szFolder),szPathBuf,*pcchPathBuf);

    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);
    if( !package )
        return ERROR_INVALID_HANDLE;
    path = resolve_folder(package, szFolder, TRUE, FALSE, NULL);
    msiobj_release( &package->hdr );

    if (path && strlenW(path) > *pcchPathBuf)
    {
        *pcchPathBuf = strlenW(path)+1;
        rc = ERROR_MORE_DATA;
    }
    else if (path)
    {
        *pcchPathBuf = strlenW(path)+1;
        strcpyW(szPathBuf,path);
        TRACE("Returning Path %s\n",debugstr_w(path));
        rc = ERROR_SUCCESS;
    }
    HeapFree(GetProcessHeap(),0,path);
    
    return rc;
}


UINT WINAPI MsiSetTargetPathA(MSIHANDLE hInstall, LPCSTR szFolder, 
                             LPCSTR szFolderPath)
{
    LPWSTR szwFolder;
    LPWSTR szwFolderPath;
    UINT rc;

    if (!szFolder)
        return ERROR_FUNCTION_FAILED;
    if (hInstall == 0)
        return ERROR_FUNCTION_FAILED;

    szwFolder = strdupAtoW(szFolder);
    if (!szwFolder)
        return ERROR_FUNCTION_FAILED; 

    szwFolderPath = strdupAtoW(szFolderPath);
    if (!szwFolderPath)
    {
        HeapFree(GetProcessHeap(),0,szwFolder);
        return ERROR_FUNCTION_FAILED; 
    }

    rc = MsiSetTargetPathW(hInstall, szwFolder, szwFolderPath);

    HeapFree(GetProcessHeap(),0,szwFolder);
    HeapFree(GetProcessHeap(),0,szwFolderPath);

    return rc;
}

UINT MSI_SetTargetPathW(MSIPACKAGE *package, LPCWSTR szFolder, 
                             LPCWSTR szFolderPath)
{
    DWORD i;
    LPWSTR path = NULL;
    LPWSTR path2 = NULL;
    INT len;
    MSIFOLDER *folder;

    TRACE("(%p %s %s)\n",package, debugstr_w(szFolder),debugstr_w(szFolderPath));

    if (package==NULL)
        return ERROR_INVALID_HANDLE;

    if (szFolderPath[0]==0)
        return ERROR_FUNCTION_FAILED;

    if (GetFileAttributesW(szFolderPath) == INVALID_FILE_ATTRIBUTES)
        return ERROR_FUNCTION_FAILED;

    path = resolve_folder(package,szFolder,FALSE,FALSE,&folder);

    if (!path)
        return ERROR_INVALID_PARAMETER;

    if (folder->Property)
        HeapFree(GetProcessHeap(),0,folder->Property);

    len = strlenW(szFolderPath);

    if (szFolderPath[len-1]!='\\')
    {
        len +=2;
        folder->Property = HeapAlloc(GetProcessHeap(),0,len*sizeof(WCHAR));
        strcpyW(folder->Property,szFolderPath);
        strcatW(folder->Property,cszbs);
    }
    else
        folder->Property = dupstrW(szFolderPath);

    if (strcmpiW(path, szFolderPath) == 0)
    {
        /*
         *  Resolved Target has not really changed, so just 
         *  set this folder and do not recalculate everything.
         */
        HeapFree(GetProcessHeap(),0,folder->ResolvedTarget);
        folder->ResolvedTarget = NULL;
        path2 = resolve_folder(package,szFolder,FALSE,TRUE,NULL);
        HeapFree(GetProcessHeap(),0,path2);
    }
    else
    {
        for (i = 0; i < package->loaded_folders; i++)
        {
            if (package->folders[i].ResolvedTarget)
                HeapFree(GetProcessHeap(),0,package->folders[i].ResolvedTarget);
            package->folders[i].ResolvedTarget=NULL;
        }

        for (i = 0; i < package->loaded_folders; i++)
        {
            path2=resolve_folder(package, package->folders[i].Directory, FALSE,
                       TRUE, NULL);
            HeapFree(GetProcessHeap(),0,path2);
        }
    }
    HeapFree(GetProcessHeap(),0,path);

    return ERROR_SUCCESS;
}

UINT WINAPI MsiSetTargetPathW(MSIHANDLE hInstall, LPCWSTR szFolder, 
                             LPCWSTR szFolderPath)
{
    MSIPACKAGE *package;
    UINT ret;

    TRACE("(%s %s)\n",debugstr_w(szFolder),debugstr_w(szFolderPath));

    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);
    ret = MSI_SetTargetPathW( package, szFolder, szFolderPath );
    msiobj_release( &package->hdr );
    return ret;
}

/***********************************************************************
 *           MsiGetMode    (MSI.@)
 *
 * Returns an internal installer state (if it is running in a mode iRunMode)
 *
 * PARAMS
 *   hInstall    [I]  Handle to the installation
 *   hRunMode    [I]  Checking run mode
 *        MSIRUNMODE_ADMIN             Administrative mode
 *        MSIRUNMODE_ADVERTISE         Advertisement mode
 *        MSIRUNMODE_MAINTENANCE       Maintenance mode
 *        MSIRUNMODE_ROLLBACKENABLED   Rollback is enabled
 *        MSIRUNMODE_LOGENABLED        Log file is writing
 *        MSIRUNMODE_OPERATIONS        Operations in progress??
 *        MSIRUNMODE_REBOOTATEND       We need to reboot after installation completed
 *        MSIRUNMODE_REBOOTNOW         We need to reboot to continue the installation
 *        MSIRUNMODE_CABINET           Files from cabinet are installed
 *        MSIRUNMODE_SOURCESHORTNAMES  Long names in source files is supressed
 *        MSIRUNMODE_TARGETSHORTNAMES  Long names in destination files is supressed
 *        MSIRUNMODE_RESERVED11        Reserved
 *        MSIRUNMODE_WINDOWS9X         Running under Windows95/98
 *        MSIRUNMODE_ZAWENABLED        Demand installation is supported
 *        MSIRUNMODE_RESERVED14        Reserved
 *        MSIRUNMODE_RESERVED15        Reserved
 *        MSIRUNMODE_SCHEDULED         called from install script
 *        MSIRUNMODE_ROLLBACK          called from rollback script
 *        MSIRUNMODE_COMMIT            called from commit script
 *
 * RETURNS
 *    In the state: TRUE
 *    Not in the state: FALSE
 *
 */

BOOL WINAPI MsiGetMode(MSIHANDLE hInstall, MSIRUNMODE iRunMode)
{
    FIXME("STUB (iRunMode=%i)\n",iRunMode);
    return TRUE;
}

/*
 * According to the docs, when this is called it immediately recalculates
 * all the component states as well
 */
UINT WINAPI MsiSetFeatureStateA(MSIHANDLE hInstall, LPCSTR szFeature,
                                INSTALLSTATE iState)
{
    LPWSTR szwFeature = NULL;
    UINT rc;

    szwFeature = strdupAtoW(szFeature);

    if (!szwFeature)
        return ERROR_FUNCTION_FAILED;
   
    rc = MsiSetFeatureStateW(hInstall,szwFeature, iState); 

    HeapFree(GetProcessHeap(),0,szwFeature);

    return rc;
}

UINT WINAPI MsiSetFeatureStateW(MSIHANDLE hInstall, LPCWSTR szFeature,
                                INSTALLSTATE iState)
{
    MSIPACKAGE* package;
    INT index;

    TRACE(" %s to %i\n",debugstr_w(szFeature), iState);

    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);
    if (!package)
        return ERROR_INVALID_HANDLE;

    index = get_loaded_feature(package,szFeature);
    if (index < 0)
        return ERROR_UNKNOWN_FEATURE;

    package->features[index].ActionRequest= iState;

    return ERROR_SUCCESS;
}

UINT WINAPI MsiGetFeatureStateA(MSIHANDLE hInstall, LPSTR szFeature,
                  INSTALLSTATE *piInstalled, INSTALLSTATE *piAction)
{
    LPWSTR szwFeature = NULL;
    UINT rc;
    
    szwFeature = strdupAtoW(szFeature);

    rc = MsiGetFeatureStateW(hInstall,szwFeature,piInstalled, piAction);

    HeapFree( GetProcessHeap(), 0 , szwFeature);

    return rc;
}

UINT MSI_GetFeatureStateW(MSIPACKAGE *package, LPWSTR szFeature,
                  INSTALLSTATE *piInstalled, INSTALLSTATE *piAction)
{
    INT index;

    index = get_loaded_feature(package,szFeature);
    if (index < 0)
        return ERROR_UNKNOWN_FEATURE;

    if (piInstalled)
        *piInstalled = package->features[index].Installed;

    if (piAction)
        *piAction = package->features[index].Action;

    TRACE("returning %i %i\n",*piInstalled,*piAction);

    return ERROR_SUCCESS;
}

UINT WINAPI MsiGetFeatureStateW(MSIHANDLE hInstall, LPWSTR szFeature,
                  INSTALLSTATE *piInstalled, INSTALLSTATE *piAction)
{
    MSIPACKAGE* package;
    UINT ret;

    TRACE("%ld %s %p %p\n", hInstall, debugstr_w(szFeature), piInstalled,
piAction);

    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);
    if (!package)
        return ERROR_INVALID_HANDLE;
    ret = MSI_GetFeatureStateW(package, szFeature, piInstalled, piAction);
    msiobj_release( &package->hdr );
    return ret;
}

UINT WINAPI MsiGetComponentStateA(MSIHANDLE hInstall, LPSTR szComponent,
                  INSTALLSTATE *piInstalled, INSTALLSTATE *piAction)
{
    LPWSTR szwComponent= NULL;
    UINT rc;
    
    szwComponent= strdupAtoW(szComponent);

    rc = MsiGetComponentStateW(hInstall,szwComponent,piInstalled, piAction);

    HeapFree( GetProcessHeap(), 0 , szwComponent);

    return rc;
}

UINT MSI_GetComponentStateW(MSIPACKAGE *package, LPWSTR szComponent,
                  INSTALLSTATE *piInstalled, INSTALLSTATE *piAction)
{
    INT index;

    TRACE("%p %s %p %p\n", package, debugstr_w(szComponent), piInstalled,
piAction);

    index = get_loaded_component(package,szComponent);
    if (index < 0)
        return ERROR_UNKNOWN_COMPONENT;

    if (piInstalled)
        *piInstalled = package->components[index].Installed;

    if (piAction)
        *piInstalled = package->components[index].Action;

    return ERROR_SUCCESS;
}

UINT WINAPI MsiGetComponentStateW(MSIHANDLE hInstall, LPWSTR szComponent,
                  INSTALLSTATE *piInstalled, INSTALLSTATE *piAction)
{
    MSIPACKAGE* package;
    UINT ret;

    TRACE("%ld %s %p %p\n", hInstall, debugstr_w(szComponent),
           piInstalled, piAction);

    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);
    if (!package)
        return ERROR_INVALID_HANDLE;
    ret = MSI_GetComponentStateW( package, szComponent, piInstalled, piAction);
    msiobj_release( &package->hdr );
    return ret;
}

#if 0
static UINT ACTION_Template(MSIPACKAGE *package)
{
    UINT rc;
    MSIQUERY * view;
    MSIRECORD * row = 0;
    static const WCHAR ExecSeqQuery[] = {0};

    rc = MsiDatabaseOpenViewW(package->db, ExecSeqQuery, &view);
    if (rc != ERROR_SUCCESS)
        return rc;

    rc = MsiViewExecute(view, 0);
    if (rc != ERROR_SUCCESS)
    {
        MsiViewClose(view);
        msiobj_release(&view->hdr);
        return rc;
    }

    while (1)
    {
        rc = MsiViewFetch(view,&row);
        if (rc != ERROR_SUCCESS)
        {
            rc = ERROR_SUCCESS;
            break;
        }

        msiobj_release(&row->hdr);
    }
    MsiViewClose(view);
    msiobj_release(&view->hdr);
    return rc;
}
#endif
