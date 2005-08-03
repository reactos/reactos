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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Here are helper functions formally in action.c that are used by a variaty of
 * actions and functions.
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/debug.h"
#include "msipriv.h"
#include "winuser.h"
#include "wine/unicode.h"
#include "action.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

static const WCHAR cszTargetDir[] = {'T','A','R','G','E','T','D','I','R',0};
static const WCHAR cszDatabase[]={'D','A','T','A','B','A','S','E',0};

const WCHAR cszSourceDir[] = {'S','o','u','r','c','e','D','i','r',0};
const WCHAR szProductCode[]= {'P','r','o','d','u','c','t','C','o','d','e',0};
const WCHAR cszRootDrive[] = {'R','O','O','T','D','R','I','V','E',0};
const WCHAR cszbs[]={'\\',0};

DWORD build_version_dword(LPCWSTR version_string)
{
    SHORT major,minor;
    WORD build;
    DWORD rc = 0x00000000;
    LPCWSTR ptr1;

    ptr1 = version_string;

    if (!ptr1)
        return rc;
    else
        major = atoiW(ptr1);


    if(ptr1)
        ptr1 = strchrW(ptr1,'.');
    if (ptr1)
    {
        ptr1++;
        minor = atoiW(ptr1);
    }
    else
        minor = 0;

    if (ptr1)
        ptr1 = strchrW(ptr1,'.');

    if (ptr1)
    {
        ptr1++;
        build = atoiW(ptr1);
    }
    else
        build = 0;

    rc = MAKELONG(build,MAKEWORD(minor,major));
    TRACE("%s -> 0x%lx\n",debugstr_w(version_string),rc);
    return rc;
}

UINT build_icon_path(MSIPACKAGE *package, LPCWSTR icon_name, 
                            LPWSTR *FilePath)
{
    LPWSTR ProductCode;
    LPWSTR SystemFolder;
    LPWSTR dest;
    UINT rc;

    static const WCHAR szInstaller[] = 
        {'M','i','c','r','o','s','o','f','t','\\',
         'I','n','s','t','a','l','l','e','r','\\',0};
    static const WCHAR szFolder[] =
        {'A','p','p','D','a','t','a','F','o','l','d','e','r',0};

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

WCHAR *load_dynamic_stringW(MSIRECORD *row, INT index)
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

LPWSTR load_dynamic_property(MSIPACKAGE *package, LPCWSTR prop, UINT* rc)
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

int get_loaded_component(MSIPACKAGE* package, LPCWSTR Component )
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

int get_loaded_feature(MSIPACKAGE* package, LPCWSTR Feature )
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

int get_loaded_file(MSIPACKAGE* package, LPCWSTR file)
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

int track_tempfile(MSIPACKAGE *package, LPCWSTR name, LPCWSTR path)
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

    package->files[index].File = strdupW(name);
    package->files[index].TargetPath = strdupW(path);
    package->files[index].Temporary = TRUE;

    TRACE("Tracking tempfile (%s)\n",debugstr_w(package->files[index].File));  

    return 0;
}

LPWSTR resolve_folder(MSIPACKAGE *package, LPCWSTR name, BOOL source, 
                      BOOL set_prop, MSIFOLDER **folder)
{
    DWORD i;
    LPWSTR p, path = NULL;

    TRACE("Working to resolve %s\n",debugstr_w(name));

    if (!name)
        return NULL;

    /* special resolving for Target and Source root dir */
    if (strcmpW(name,cszTargetDir)==0 || strcmpW(name,cszSourceDir)==0)
    {
        if (!source)
        {
            LPWSTR check_path;
            check_path = load_dynamic_property(package,cszTargetDir,NULL);
            if (!check_path)
            {
                check_path = load_dynamic_property(package,cszRootDrive,NULL);
                if (set_prop)
                    MSI_SetPropertyW(package,cszTargetDir,check_path);
            }

            /* correct misbuilt target dir */
            path = build_directory_name(2, check_path, NULL);
            if (strcmpiW(path,check_path)!=0)
                MSI_SetPropertyW(package,cszTargetDir,path);

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
        path = strdupW(package->folders[i].ResolvedTarget);
        TRACE("   already resolved to %s\n",debugstr_w(path));
        return path;
    }
    else if (source && package->folders[i].ResolvedSource)
    {
        path = strdupW(package->folders[i].ResolvedSource);
        TRACE("   (source)already resolved to %s\n",debugstr_w(path));
        return path;
    }
    else if (!source && package->folders[i].Property)
    {
        path = build_directory_name(2, package->folders[i].Property, NULL);
                    
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
            TRACE("   TargetDefault = %s\n",
                    debugstr_w(package->folders[i].TargetDefault));

            path = build_directory_name(3, p, 
                            package->folders[i].TargetDefault, NULL);
            package->folders[i].ResolvedTarget = strdupW(path);
            TRACE("   resolved into %s\n",debugstr_w(path));
            if (set_prop)
                MSI_SetPropertyW(package,name,path);
        }
        else 
        {
            if (package->folders[i].SourceDefault && 
                package->folders[i].SourceDefault[0]!='.')
                path = build_directory_name(3, p, package->folders[i].SourceDefault, NULL);
            else
                path = strdupW(p);
            TRACE("   (source)resolved into %s\n",debugstr_w(path));
            package->folders[i].ResolvedSource = strdupW(path);
        }
        HeapFree(GetProcessHeap(),0,p);
    }
    return path;
}

/* wrapper to resist a need for a full rewrite right now */
DWORD deformat_string(MSIPACKAGE *package, LPCWSTR ptr, WCHAR** data )
{
    if (ptr)
    {
        MSIRECORD *rec = MSI_CreateRecord(1);
        DWORD size = 0;

        MSI_RecordSetStringW(rec,0,ptr);
        MSI_FormatRecordW(package,rec,NULL,&size);
        if (size >= 0)
        {
            size++;
            *data = HeapAlloc(GetProcessHeap(),0,size*sizeof(WCHAR));
            if (size > 1)
                MSI_FormatRecordW(package,rec,*data,&size);
            else
                *data[0] = 0;
            msiobj_release( &rec->hdr );
            return sizeof(WCHAR)*size;
        }
        msiobj_release( &rec->hdr );
    }

    *data = NULL;
    return 0;
}

UINT schedule_action(MSIPACKAGE *package, UINT script, LPCWSTR action)
{
    UINT count;
    LPWSTR *newbuf = NULL;
    if (script >= TOTAL_SCRIPTS)
    {
        FIXME("Unknown script requested %i\n",script);
        return ERROR_FUNCTION_FAILED;
    }
    TRACE("Scheduling Action %s in script %i\n",debugstr_w(action), script);
    
    count = package->script->ActionCount[script];
    package->script->ActionCount[script]++;
    if (count != 0)
        newbuf = HeapReAlloc(GetProcessHeap(),0,
                        package->script->Actions[script],
                        package->script->ActionCount[script]* sizeof(LPWSTR));
    else
        newbuf = HeapAlloc(GetProcessHeap(),0, sizeof(LPWSTR));

    newbuf[count] = strdupW(action);
    package->script->Actions[script] = newbuf;

   return ERROR_SUCCESS;
}

static void remove_tracked_tempfiles(MSIPACKAGE* package)
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
void ACTION_free_package_structures( MSIPACKAGE* package)
{
    INT i;
    
    TRACE("Freeing package action data\n");

    remove_tracked_tempfiles(package);

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

    for (i = 0; i < package->loaded_components; i++)
        HeapFree(GetProcessHeap(),0,package->components[i].FullKeypath);

    if (package->components && package->loaded_components > 0)
        HeapFree(GetProcessHeap(),0,package->components);

    for (i = 0; i < package->loaded_files; i++)
    {
        HeapFree(GetProcessHeap(),0,package->files[i].File);
        HeapFree(GetProcessHeap(),0,package->files[i].FileName);
        HeapFree(GetProcessHeap(),0,package->files[i].ShortName);
        HeapFree(GetProcessHeap(),0,package->files[i].Version);
        HeapFree(GetProcessHeap(),0,package->files[i].Language);
        HeapFree(GetProcessHeap(),0,package->files[i].SourcePath);
        HeapFree(GetProcessHeap(),0,package->files[i].TargetPath);
    }

    if (package->files && package->loaded_files > 0)
        HeapFree(GetProcessHeap(),0,package->files);

    /* clean up extension, progid, class and verb structures */
    for (i = 0; i < package->loaded_classes; i++)
    {
        HeapFree(GetProcessHeap(),0,package->classes[i].Description);
        HeapFree(GetProcessHeap(),0,package->classes[i].FileTypeMask);
        HeapFree(GetProcessHeap(),0,package->classes[i].IconPath);
        HeapFree(GetProcessHeap(),0,package->classes[i].DefInprocHandler);
        HeapFree(GetProcessHeap(),0,package->classes[i].DefInprocHandler32);
        HeapFree(GetProcessHeap(),0,package->classes[i].Argument);
        HeapFree(GetProcessHeap(),0,package->classes[i].ProgIDText);
    }

    if (package->classes && package->loaded_classes > 0)
        HeapFree(GetProcessHeap(),0,package->classes);

    for (i = 0; i < package->loaded_extensions; i++)
    {
        HeapFree(GetProcessHeap(),0,package->extensions[i].ProgIDText);
    }

    if (package->extensions && package->loaded_extensions > 0)
        HeapFree(GetProcessHeap(),0,package->extensions);

    for (i = 0; i < package->loaded_progids; i++)
    {
        HeapFree(GetProcessHeap(),0,package->progids[i].ProgID);
        HeapFree(GetProcessHeap(),0,package->progids[i].Description);
        HeapFree(GetProcessHeap(),0,package->progids[i].IconPath);
    }

    if (package->progids && package->loaded_progids > 0)
        HeapFree(GetProcessHeap(),0,package->progids);

    for (i = 0; i < package->loaded_verbs; i++)
    {
        HeapFree(GetProcessHeap(),0,package->verbs[i].Verb);
        HeapFree(GetProcessHeap(),0,package->verbs[i].Command);
        HeapFree(GetProcessHeap(),0,package->verbs[i].Argument);
    }

    if (package->verbs && package->loaded_verbs > 0)
        HeapFree(GetProcessHeap(),0,package->verbs);

    for (i = 0; i < package->loaded_mimes; i++)
        HeapFree(GetProcessHeap(),0,package->mimes[i].ContentType);

    if (package->mimes && package->loaded_mimes > 0)
        HeapFree(GetProcessHeap(),0,package->mimes);

    for (i = 0; i < package->loaded_appids; i++)
    {
        HeapFree(GetProcessHeap(),0,package->appids[i].RemoteServerName);
        HeapFree(GetProcessHeap(),0,package->appids[i].LocalServer);
        HeapFree(GetProcessHeap(),0,package->appids[i].ServiceParameters);
        HeapFree(GetProcessHeap(),0,package->appids[i].DllSurrogate);
    }

    if (package->appids && package->loaded_appids > 0)
        HeapFree(GetProcessHeap(),0,package->appids);

    if (package->script)
    {
        for (i = 0; i < TOTAL_SCRIPTS; i++)
        {
            int j;
            for (j = 0; j < package->script->ActionCount[i]; j++)
                HeapFree(GetProcessHeap(),0,package->script->Actions[i][j]);
        
            HeapFree(GetProcessHeap(),0,package->script->Actions[i]);
        }
        HeapFree(GetProcessHeap(),0,package->script);
    }

    HeapFree(GetProcessHeap(),0,package->PackagePath);

    /* cleanup control event subscriptions */
    ControlEvent_CleanupSubscriptions(package);
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
LPWSTR build_directory_name(DWORD count, ...)
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

/***********************************************************************
 *            create_full_pathW
 *
 * Recursively create all directories in the path.
 *
 * shamelessly stolen from setupapi/queue.c
 */
BOOL create_full_pathW(const WCHAR *path)
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

void ui_progress(MSIPACKAGE *package, int a, int b, int c, int d )
{
    MSIRECORD * row;

    row = MSI_CreateRecord(4);
    MSI_RecordSetInteger(row,1,a);
    MSI_RecordSetInteger(row,2,b);
    MSI_RecordSetInteger(row,3,c);
    MSI_RecordSetInteger(row,4,d);
    MSI_ProcessMessage(package, INSTALLMESSAGE_PROGRESS, row);
    msiobj_release(&row->hdr);

    msi_dialog_check_messages(NULL);
}

void ui_actiondata(MSIPACKAGE *package, LPCWSTR action, MSIRECORD * record)
{
    static const WCHAR Query_t[] = 
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','A','c','t','i','o', 'n','T','e','x','t','`',' ',
         'W','H','E','R','E',' ', '`','A','c','t','i','o','n','`',' ','=', 
         ' ','\'','%','s','\'',0};
    WCHAR message[1024];
    MSIRECORD * row = 0;
    DWORD size;
    static const WCHAR szActionData[] = 
        {'A','c','t','i','o','n','D','a','t','a',0};

    if (!package->LastAction || strcmpW(package->LastAction,action))
    {
        row = MSI_QueryGetRecord(package->db, Query_t, action);
        if (!row)
            return;

        if (MSI_RecordIsNull(row,3))
        {
            msiobj_release(&row->hdr);
            return;
        }

        /* update the cached actionformat */
        HeapFree(GetProcessHeap(),0,package->ActionFormat);
        package->ActionFormat = load_dynamic_stringW(row,3);

        HeapFree(GetProcessHeap(),0,package->LastAction);
        package->LastAction = strdupW(action);

        msiobj_release(&row->hdr);
    }

    MSI_RecordSetStringW(record,0,package->ActionFormat);
    size = 1024;
    MSI_FormatRecordW(package,record,message,&size);

    row = MSI_CreateRecord(1);
    MSI_RecordSetStringW(row,1,message);
 
    MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, row);

    ControlEvent_FireSubscribedEvent(package,szActionData, row);

    msiobj_release(&row->hdr);
}

BOOL ACTION_VerifyComponentForAction(MSIPACKAGE* package, INT index, 
                                            INSTALLSTATE check )
{
    if (package->components[index].Installed == check)
        return FALSE;

    if (package->components[index].ActionRequest == check)
        return TRUE;
    else
        return FALSE;
}

BOOL ACTION_VerifyFeatureForAction(MSIPACKAGE* package, INT index, 
                                            INSTALLSTATE check )
{
    if (package->features[index].Installed == check)
        return FALSE;

    if (package->features[index].ActionRequest == check)
        return TRUE;
    else
        return FALSE;
}

void reduce_to_longfilename(WCHAR* filename)
{
    LPWSTR p = strchrW(filename,'|');
    if (p)
        memmove(filename, p+1, (strlenW(p+1)+1)*sizeof(WCHAR));
}

void reduce_to_shortfilename(WCHAR* filename)
{
    LPWSTR p = strchrW(filename,'|');
    if (p)
        *p = 0;
}

LPWSTR create_component_advertise_string(MSIPACKAGE* package, 
                MSICOMPONENT* component, LPCWSTR feature)
{
    LPWSTR productid=NULL;
    GUID clsid;
    WCHAR productid_85[21];
    WCHAR component_85[21];
    /*
     * I have a fair bit of confusion as to when a < is used and when a > is
     * used. I do not think i have it right...
     *
     * Ok it appears that the > is used if there is a guid for the compoenent
     * and the < is used if not.
     */
    static WCHAR fmt1[] = {'%','s','%','s','<',0,0};
    static WCHAR fmt2[] = {'%','s','%','s','>','%','s',0,0};
    LPWSTR output = NULL;
    DWORD sz = 0;

    memset(productid_85,0,sizeof(productid_85));
    memset(component_85,0,sizeof(component_85));

    productid = load_dynamic_property(package,szProductCode,NULL);
    CLSIDFromString(productid, &clsid);
    
    encode_base85_guid(&clsid,productid_85);

    CLSIDFromString(component->ComponentId, &clsid);
    encode_base85_guid(&clsid,component_85);

    TRACE("Doing something with this... %s %s %s\n", 
            debugstr_w(productid_85), debugstr_w(feature),
            debugstr_w(component_85));
 
    sz = lstrlenW(productid_85) + lstrlenW(feature);
    if (component)
        sz += lstrlenW(component_85);

    sz+=3;
    sz *= sizeof(WCHAR);
           
    output = HeapAlloc(GetProcessHeap(),0,sz);
    memset(output,0,sz);

    if (component)
        sprintfW(output,fmt2,productid_85,feature,component_85);
    else
        sprintfW(output,fmt1,productid_85,feature);

    HeapFree(GetProcessHeap(),0,productid);
    
    return output;
}

/* update compoennt state based on a feature change */
void ACTION_UpdateComponentStates(MSIPACKAGE *package, LPCWSTR szFeature)
{
    int i;
    INSTALLSTATE newstate;
    MSIFEATURE *feature;

    i = get_loaded_feature(package,szFeature);
    if (i < 0)
        return;

    feature = &package->features[i];
    newstate = feature->ActionRequest;

    for( i = 0; i < feature->ComponentCount; i++)
    {
        MSICOMPONENT* component = &package->components[feature->Components[i]];

        TRACE("MODIFYING(%i): Component %s (Installed %i, Action %i, Request %i)\n",
            newstate, debugstr_w(component->Component), component->Installed, 
            component->Action, component->ActionRequest);
        
        if (!component->Enabled)
            continue;
        else
        {
            if (newstate == INSTALLSTATE_LOCAL)
            {
                component->ActionRequest = INSTALLSTATE_LOCAL;
                component->Action = INSTALLSTATE_LOCAL;
            }
            else 
            {
                int j,k;

                component->ActionRequest = newstate;
                component->Action = newstate;

                /*if any other feature wants is local we need to set it local*/
                for (j = 0; 
                     j < package->loaded_features &&
                     component->ActionRequest != INSTALLSTATE_LOCAL; 
                     j++)
                {
                    for (k = 0; k < package->features[j].ComponentCount; k++)
                        if ( package->features[j].Components[k] ==
                             feature->Components[i] )
                        {
                            if (package->features[j].ActionRequest == 
                                INSTALLSTATE_LOCAL)
                            {
                                TRACE("Saved by %s\n", debugstr_w(package->features[j].Feature));
                                component->ActionRequest = INSTALLSTATE_LOCAL;
                                component->Action = INSTALLSTATE_LOCAL;
                            }
                            break;
                        }
                }
            }
        }
        TRACE("Result (%i): Component %s (Installed %i, Action %i, Request %i)\n",
            newstate, debugstr_w(component->Component), component->Installed, 
            component->Action, component->ActionRequest);
    } 
}
