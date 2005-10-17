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
    LPWSTR SystemFolder;
    LPWSTR dest;

    static const WCHAR szInstaller[] = 
        {'M','i','c','r','o','s','o','f','t','\\',
         'I','n','s','t','a','l','l','e','r','\\',0};
    static const WCHAR szFolder[] =
        {'A','p','p','D','a','t','a','F','o','l','d','e','r',0};

    SystemFolder = load_dynamic_property(package,szFolder,NULL);

    dest = build_directory_name(3, SystemFolder, szInstaller, package->ProductCode);

    create_full_pathW(dest);

    *FilePath = build_directory_name(2, dest, icon_name);

    HeapFree(GetProcessHeap(),0,SystemFolder);
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

MSICOMPONENT* get_loaded_component( MSIPACKAGE* package, LPCWSTR Component )
{
    MSICOMPONENT *comp;

    LIST_FOR_EACH_ENTRY( comp, &package->components, MSICOMPONENT, entry )
    {
        if (lstrcmpW(Component,comp->Component)==0)
            return comp;
    }
    return NULL;
}

MSIFEATURE* get_loaded_feature(MSIPACKAGE* package, LPCWSTR Feature )
{
    MSIFEATURE *feature;

    LIST_FOR_EACH_ENTRY( feature, &package->features, MSIFEATURE, entry )
    {
        if (lstrcmpW( Feature, feature->Feature )==0)
            return feature;
    }
    return NULL;
}

MSIFILE* get_loaded_file( MSIPACKAGE* package, LPCWSTR key )
{
    MSIFILE *file;

    LIST_FOR_EACH_ENTRY( file, &package->files, MSIFILE, entry )
    {
        if (lstrcmpW( key, file->File )==0)
            return file;
    }
    return NULL;
}

int track_tempfile( MSIPACKAGE *package, LPCWSTR name, LPCWSTR path )
{
    MSIFILE *file;

    if (!package)
        return -2;

    file = get_loaded_file( package, name );
    if (file)
        return -1;

    file = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof (MSIFILE) );

    file->File = strdupW( name );
    file->TargetPath = strdupW( path );
    file->Temporary = TRUE;

    TRACE("Tracking tempfile (%s)\n", debugstr_w( file->File ));  

    return 0;
}

MSIFOLDER *get_loaded_folder( MSIPACKAGE *package, LPCWSTR dir )
{
    MSIFOLDER *folder;
    
    LIST_FOR_EACH_ENTRY( folder, &package->folders, MSIFOLDER, entry )
    {
        if (lstrcmpW( dir, folder->Directory )==0)
            return folder;
    }
    return NULL;
}

LPWSTR resolve_folder(MSIPACKAGE *package, LPCWSTR name, BOOL source, 
                      BOOL set_prop, MSIFOLDER **folder)
{
    MSIFOLDER *f;
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
        }
        if (folder)
            *folder = get_loaded_folder( package, name );
        return path;
    }

    f = get_loaded_folder( package, name );
    if (!f)
        return NULL;

    if (folder)
        *folder = f;

    if (!source && f->ResolvedTarget)
    {
        path = strdupW( f->ResolvedTarget );
        TRACE("   already resolved to %s\n",debugstr_w(path));
        return path;
    }
    else if (source && f->ResolvedSource)
    {
        path = strdupW( f->ResolvedSource );
        TRACE("   (source)already resolved to %s\n",debugstr_w(path));
        return path;
    }
    else if (!source && f->Property)
    {
        path = build_directory_name( 2, f->Property, NULL );
                    
        TRACE("   internally set to %s\n",debugstr_w(path));
        if (set_prop)
            MSI_SetPropertyW( package, name, path );
        return path;
    }

    if (f->Parent)
    {
        LPWSTR parent = f->Parent->Directory;

        TRACE(" ! Parent is %s\n", debugstr_w(parent));

        p = resolve_folder(package, parent, source, set_prop, NULL);
        if (!source)
        {
            TRACE("   TargetDefault = %s\n", debugstr_w(f->TargetDefault));

            path = build_directory_name( 3, p, f->TargetDefault, NULL );
            f->ResolvedTarget = strdupW( path );
            TRACE("   resolved into %s\n",debugstr_w(path));
            if (set_prop)
                MSI_SetPropertyW(package,name,path);
        }
        else 
        {
            if (f->SourceDefault && f->SourceDefault[0]!='.')
                path = build_directory_name( 3, p, f->SourceDefault, NULL );
            else
                path = strdupW(p);
            TRACE("   (source)resolved into %s\n",debugstr_w(path));
            f->ResolvedSource = strdupW( path );
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
    MSIFILE *file;

    if (!package)
        return;

    LIST_FOR_EACH_ENTRY( file, &package->files, MSIFILE, entry )
    {
        if (file->Temporary)
        {
            TRACE("Cleaning up %s\n", debugstr_w( file->TargetPath ));
            DeleteFileW( file->TargetPath );
        }
    }
}

static void free_feature( MSIFEATURE *feature )
{
    struct list *item, *cursor;

    LIST_FOR_EACH_SAFE( item, cursor, &feature->Components )
    {
        ComponentList *cl = LIST_ENTRY( item, ComponentList, entry );
        list_remove( &cl->entry );
        HeapFree( GetProcessHeap(), 0, cl );
    }
    HeapFree( GetProcessHeap(), 0, feature );
}


/* Called when the package is being closed */
void ACTION_free_package_structures( MSIPACKAGE* package)
{
    INT i;
    struct list *item, *cursor;
    
    TRACE("Freeing package action data\n");

    remove_tracked_tempfiles(package);

    LIST_FOR_EACH_SAFE( item, cursor, &package->features )
    {
        MSIFEATURE *feature = LIST_ENTRY( item, MSIFEATURE, entry );
        list_remove( &feature->entry );
        free_feature( feature );
    }

    LIST_FOR_EACH_SAFE( item, cursor, &package->folders )
    {
        MSIFOLDER *folder = LIST_ENTRY( item, MSIFOLDER, entry );

        list_remove( &folder->entry );
        HeapFree( GetProcessHeap(), 0, folder->Directory );
        HeapFree( GetProcessHeap(), 0, folder->TargetDefault );
        HeapFree( GetProcessHeap(), 0, folder->SourceDefault );
        HeapFree( GetProcessHeap(), 0, folder->ResolvedTarget );
        HeapFree( GetProcessHeap(), 0, folder->ResolvedSource );
        HeapFree( GetProcessHeap(), 0, folder->Property );
    }

    LIST_FOR_EACH_SAFE( item, cursor, &package->components )
    {
        MSICOMPONENT *comp = LIST_ENTRY( item, MSICOMPONENT, entry );
        
        list_remove( &comp->entry );
        HeapFree( GetProcessHeap(), 0, comp->FullKeypath );
        HeapFree( GetProcessHeap(), 0, comp );
    }

    LIST_FOR_EACH_SAFE( item, cursor, &package->files )
    {
        MSIFILE *file = LIST_ENTRY( item, MSIFILE, entry );

        list_remove( &file->entry );
        HeapFree( GetProcessHeap(), 0, file->File );
        HeapFree( GetProcessHeap(), 0, file->FileName );
        HeapFree( GetProcessHeap(), 0, file->ShortName );
        HeapFree( GetProcessHeap(), 0, file->Version );
        HeapFree( GetProcessHeap(), 0, file->Language );
        HeapFree( GetProcessHeap(), 0, file->SourcePath );
        HeapFree( GetProcessHeap(), 0, file->TargetPath );
        HeapFree( GetProcessHeap(), 0, file );
    }

    /* clean up extension, progid, class and verb structures */
    LIST_FOR_EACH_SAFE( item, cursor, &package->classes )
    {
        MSICLASS *cls = LIST_ENTRY( item, MSICLASS, entry );

        list_remove( &cls->entry );
        HeapFree( GetProcessHeap(), 0, cls->Description );
        HeapFree( GetProcessHeap(), 0, cls->FileTypeMask );
        HeapFree( GetProcessHeap(), 0, cls->IconPath );
        HeapFree( GetProcessHeap(), 0, cls->DefInprocHandler );
        HeapFree( GetProcessHeap(), 0, cls->DefInprocHandler32 );
        HeapFree( GetProcessHeap(), 0, cls->Argument );
        HeapFree( GetProcessHeap(), 0, cls->ProgIDText );
        HeapFree( GetProcessHeap(), 0, cls );
    }

    LIST_FOR_EACH_SAFE( item, cursor, &package->extensions )
    {
        MSIEXTENSION *ext = LIST_ENTRY( item, MSIEXTENSION, entry );

        list_remove( &ext->entry );
        HeapFree( GetProcessHeap(), 0, ext->ProgIDText );
        HeapFree( GetProcessHeap(), 0, ext );
    }

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

    LIST_FOR_EACH_SAFE( item, cursor, &package->mimes )
    {
        MSIMIME *mt = LIST_ENTRY( item, MSIMIME, entry );

        list_remove( &mt->entry );
        HeapFree( GetProcessHeap(), 0, mt->ContentType );
        HeapFree( GetProcessHeap(), 0, mt );
    }

    LIST_FOR_EACH_SAFE( item, cursor, &package->appids )
    {
        MSIAPPID *appid = LIST_ENTRY( item, MSIAPPID, entry );

        list_remove( &appid->entry );
        HeapFree( GetProcessHeap(), 0, appid->RemoteServerName );
        HeapFree( GetProcessHeap(), 0, appid->LocalServer );
        HeapFree( GetProcessHeap(), 0, appid->ServiceParameters );
        HeapFree( GetProcessHeap(), 0, appid->DllSurrogate );
        HeapFree( GetProcessHeap(), 0, appid );
    }

    if (package->script)
    {
        for (i = 0; i < TOTAL_SCRIPTS; i++)
        {
            int j;
            for (j = 0; j < package->script->ActionCount[i]; j++)
                HeapFree(GetProcessHeap(),0,package->script->Actions[i][j]);
        
            HeapFree(GetProcessHeap(),0,package->script->Actions[i]);
        }

        for (i = 0; i < package->script->UniqueActionsCount; i++)
            HeapFree(GetProcessHeap(),0,package->script->UniqueActions[i]);

        HeapFree(GetProcessHeap(),0,package->script->UniqueActions);
        HeapFree(GetProcessHeap(),0,package->script);
    }

    HeapFree(GetProcessHeap(),0,package->PackagePath);
    HeapFree(GetProcessHeap(),0,package->msiFilePath);
    HeapFree(GetProcessHeap(),0,package->ProductCode);

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

BOOL ACTION_VerifyComponentForAction(MSIPACKAGE* package, MSICOMPONENT* comp,
                                            INSTALLSTATE check )
{
    if (comp->Installed == check)
        return FALSE;

    if (comp->ActionRequest == check)
        return TRUE;
    else
        return FALSE;
}

BOOL ACTION_VerifyFeatureForAction( MSIFEATURE* feature, INSTALLSTATE check )
{
    if (feature->Installed == check)
        return FALSE;

    if (feature->ActionRequest == check)
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

    CLSIDFromString(package->ProductCode, &clsid);
    
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
    
    return output;
}

/* update compoennt state based on a feature change */
void ACTION_UpdateComponentStates(MSIPACKAGE *package, LPCWSTR szFeature)
{
    INSTALLSTATE newstate;
    MSIFEATURE *feature;
    ComponentList *cl;

    feature = get_loaded_feature(package,szFeature);
    if (!feature)
        return;

    newstate = feature->ActionRequest;

    LIST_FOR_EACH_ENTRY( cl, &feature->Components, ComponentList, entry )
    {
        MSICOMPONENT* component = cl->component;
    
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
                ComponentList *clist;
                MSIFEATURE *f;

                component->ActionRequest = newstate;
                component->Action = newstate;

                /*if any other feature wants is local we need to set it local*/
                LIST_FOR_EACH_ENTRY( f, &package->features, MSIFEATURE, entry )
                {
                    if ( component->ActionRequest != INSTALLSTATE_LOCAL )
                        break;

                    LIST_FOR_EACH_ENTRY( clist, &f->Components, ComponentList, entry )
                    {
                        if ( clist->component == component )
                        {
                            if (f->ActionRequest == INSTALLSTATE_LOCAL)
                            {
                                TRACE("Saved by %s\n", debugstr_w(f->Feature));
                                component->ActionRequest = INSTALLSTATE_LOCAL;
                                component->Action = INSTALLSTATE_LOCAL;
                            }
                            break;
                        }
                    }
                }
            }
        }
        TRACE("Result (%i): Component %s (Installed %i, Action %i, Request %i)\n",
            newstate, debugstr_w(component->Component), component->Installed, 
            component->Action, component->ActionRequest);
    } 
}

UINT register_unique_action(MSIPACKAGE *package, LPCWSTR action)
{
    UINT count;
    LPWSTR *newbuf = NULL;

    if (!package || !package->script)
        return FALSE;

    TRACE("Registering Action %s as having fun\n",debugstr_w(action));
    
    count = package->script->UniqueActionsCount;
    package->script->UniqueActionsCount++;
    if (count != 0)
        newbuf = HeapReAlloc(GetProcessHeap(),0,
                        package->script->UniqueActions,
                        package->script->UniqueActionsCount* sizeof(LPWSTR));
    else
        newbuf = HeapAlloc(GetProcessHeap(),0, sizeof(LPWSTR));

    newbuf[count] = strdupW(action);
    package->script->UniqueActions = newbuf;

   return ERROR_SUCCESS;
}

BOOL check_unique_action(MSIPACKAGE *package, LPCWSTR action)
{
    INT i;

    if (!package || !package->script)
        return FALSE;

    for (i = 0; i < package->script->UniqueActionsCount; i++)
        if (!strcmpW(package->script->UniqueActions[i],action))
            return TRUE;

    return FALSE;
}

WCHAR* generate_error_string(MSIPACKAGE *package, UINT error, DWORD count, ... )
{
    static const WCHAR query[] = {'S','E','L','E','C','T',' ','`','M','e','s','s','a','g','e','`',' ','F','R','O','M',' ','`','E','r','r','o','r','`',' ','W','H','E','R','E',' ','`','E','r','r','o','r','`',' ','=',' ','%','i',0};

    MSIRECORD *rec;
    MSIRECORD *row;
    DWORD size = 0;
    DWORD i;
    va_list va;
    LPCWSTR str;
    LPWSTR data;

    row = MSI_QueryGetRecord(package->db, query, error);
    if (!row)
        return 0;

    rec = MSI_CreateRecord(count+2);

    str = MSI_RecordGetString(row,1);
    MSI_RecordSetStringW(rec,0,str);
    msiobj_release( &row->hdr );
    MSI_RecordSetInteger(rec,1,error);

    va_start(va,count);
    for (i = 0; i < count; i++)
    {
        str = va_arg(va,LPCWSTR);
        MSI_RecordSetStringW(rec,(i+2),str);
    }
    va_end(va);

    MSI_FormatRecordW(package,rec,NULL,&size);
    if (size >= 0)
    {
        size++;
        data = HeapAlloc(GetProcessHeap(),0,size*sizeof(WCHAR));
        if (size > 1)
            MSI_FormatRecordW(package,rec,data,&size);
        else
            data[0] = 0;
        msiobj_release( &rec->hdr );
        return data;
    }

    msiobj_release( &rec->hdr );
    data = NULL;
    return data;
}
