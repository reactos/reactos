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
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

static void free_feature( MSIFEATURE *feature )
{
    struct list *item, *cursor;

    LIST_FOR_EACH_SAFE( item, cursor, &feature->Children )
    {
        FeatureList *fl = LIST_ENTRY( item, FeatureList, entry );
        list_remove( &fl->entry );
        msi_free( fl );
    }

    LIST_FOR_EACH_SAFE( item, cursor, &feature->Components )
    {
        ComponentList *cl = LIST_ENTRY( item, ComponentList, entry );
        list_remove( &cl->entry );
        msi_free( cl );
    }
    msi_free( feature->Feature );
    msi_free( feature->Feature_Parent );
    msi_free( feature->Directory );
    msi_free( feature->Description );
    msi_free( feature->Title );
    msi_free( feature );
}

static void free_folder( MSIFOLDER *folder )
{
    struct list *item, *cursor;

    LIST_FOR_EACH_SAFE( item, cursor, &folder->children )
    {
        FolderList *fl = LIST_ENTRY( item, FolderList, entry );
        list_remove( &fl->entry );
        msi_free( fl );
    }
    msi_free( folder->Parent );
    msi_free( folder->Directory );
    msi_free( folder->TargetDefault );
    msi_free( folder->SourceLongPath );
    msi_free( folder->SourceShortPath );
    msi_free( folder->ResolvedTarget );
    msi_free( folder->ResolvedSource );
    msi_free( folder );
}

static void free_extension( MSIEXTENSION *ext )
{
    struct list *item, *cursor;

    LIST_FOR_EACH_SAFE( item, cursor, &ext->verbs )
    {
        MSIVERB *verb = LIST_ENTRY( item, MSIVERB, entry );

        list_remove( &verb->entry );
        msi_free( verb->Verb );
        msi_free( verb->Command );
        msi_free( verb->Argument );
        msi_free( verb );
    }

    msi_free( ext->Extension );
    msi_free( ext->ProgIDText );
    msi_free( ext );
}

static void free_assembly( MSIASSEMBLY *assembly )
{
    msi_free( assembly->feature );
    msi_free( assembly->manifest );
    msi_free( assembly->application );
    msi_free( assembly->display_name );
    if (assembly->tempdir) RemoveDirectoryW( assembly->tempdir );
    msi_free( assembly->tempdir );
    msi_free( assembly );
}

void msi_free_action_script( MSIPACKAGE *package, UINT script )
{
    UINT i;
    for (i = 0; i < package->script_actions_count[script]; i++)
        msi_free( package->script_actions[script][i] );

    msi_free( package->script_actions[script] );
    package->script_actions[script] = NULL;
    package->script_actions_count[script] = 0;
}

static void free_package_structures( MSIPACKAGE *package )
{
    struct list *item, *cursor;
    int i;

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
        free_folder( folder );
    }

    LIST_FOR_EACH_SAFE( item, cursor, &package->files )
    {
        MSIFILE *file = LIST_ENTRY( item, MSIFILE, entry );

        list_remove( &file->entry );
        msi_free( file->File );
        msi_free( file->FileName );
        msi_free( file->ShortName );
        msi_free( file->LongName );
        msi_free( file->Version );
        msi_free( file->Language );
        if (msi_is_global_assembly( file->Component )) DeleteFileW( file->TargetPath );
        msi_free( file->TargetPath );
        msi_free( file );
    }

    LIST_FOR_EACH_SAFE( item, cursor, &package->components )
    {
        MSICOMPONENT *comp = LIST_ENTRY( item, MSICOMPONENT, entry );

        list_remove( &comp->entry );
        msi_free( comp->Component );
        msi_free( comp->ComponentId );
        msi_free( comp->Directory );
        msi_free( comp->Condition );
        msi_free( comp->KeyPath );
        msi_free( comp->FullKeypath );
        if (comp->assembly) free_assembly( comp->assembly );
        msi_free( comp );
    }

    LIST_FOR_EACH_SAFE( item, cursor, &package->filepatches )
    {
        MSIFILEPATCH *patch = LIST_ENTRY( item, MSIFILEPATCH, entry );

        list_remove( &patch->entry );
        msi_free( patch->path );
        msi_free( patch );
    }

    /* clean up extension, progid, class and verb structures */
    LIST_FOR_EACH_SAFE( item, cursor, &package->classes )
    {
        MSICLASS *cls = LIST_ENTRY( item, MSICLASS, entry );

        list_remove( &cls->entry );
        msi_free( cls->clsid );
        msi_free( cls->Context );
        msi_free( cls->Description );
        msi_free( cls->FileTypeMask );
        msi_free( cls->IconPath );
        msi_free( cls->DefInprocHandler );
        msi_free( cls->DefInprocHandler32 );
        msi_free( cls->Argument );
        msi_free( cls->ProgIDText );
        msi_free( cls );
    }

    LIST_FOR_EACH_SAFE( item, cursor, &package->extensions )
    {
        MSIEXTENSION *ext = LIST_ENTRY( item, MSIEXTENSION, entry );

        list_remove( &ext->entry );
        free_extension( ext );
    }

    LIST_FOR_EACH_SAFE( item, cursor, &package->progids )
    {
        MSIPROGID *progid = LIST_ENTRY( item, MSIPROGID, entry );

        list_remove( &progid->entry );
        msi_free( progid->ProgID );
        msi_free( progid->Description );
        msi_free( progid->IconPath );
        msi_free( progid );
    }

    LIST_FOR_EACH_SAFE( item, cursor, &package->mimes )
    {
        MSIMIME *mt = LIST_ENTRY( item, MSIMIME, entry );

        list_remove( &mt->entry );
        msi_free( mt->suffix );
        msi_free( mt->clsid );
        msi_free( mt->ContentType );
        msi_free( mt );
    }

    LIST_FOR_EACH_SAFE( item, cursor, &package->appids )
    {
        MSIAPPID *appid = LIST_ENTRY( item, MSIAPPID, entry );

        list_remove( &appid->entry );
        msi_free( appid->AppID );
        msi_free( appid->RemoteServerName );
        msi_free( appid->LocalServer );
        msi_free( appid->ServiceParameters );
        msi_free( appid->DllSurrogate );
        msi_free( appid );
    }

    LIST_FOR_EACH_SAFE( item, cursor, &package->sourcelist_info )
    {
        MSISOURCELISTINFO *info = LIST_ENTRY( item, MSISOURCELISTINFO, entry );

        list_remove( &info->entry );
        msi_free( info->value );
        msi_free( info );
    }

    LIST_FOR_EACH_SAFE( item, cursor, &package->sourcelist_media )
    {
        MSIMEDIADISK *info = LIST_ENTRY( item, MSIMEDIADISK, entry );

        list_remove( &info->entry );
        msi_free( info->volume_label );
        msi_free( info->disk_prompt );
        msi_free( info );
    }

    for (i = 0; i < SCRIPT_MAX; i++)
        msi_free_action_script( package, i );

    for (i = 0; i < package->unique_actions_count; i++)
        msi_free( package->unique_actions[i] );
    msi_free( package->unique_actions);

    LIST_FOR_EACH_SAFE( item, cursor, &package->binaries )
    {
        MSIBINARY *binary = LIST_ENTRY( item, MSIBINARY, entry );

        list_remove( &binary->entry );
        if (binary->module)
            FreeLibrary( binary->module );
        if (!DeleteFileW( binary->tmpfile ))
            ERR("failed to delete %s (%u)\n", debugstr_w(binary->tmpfile), GetLastError());
        msi_free( binary->source );
        msi_free( binary->tmpfile );
        msi_free( binary );
    }

    LIST_FOR_EACH_SAFE( item, cursor, &package->cabinet_streams )
    {
        MSICABINETSTREAM *cab = LIST_ENTRY( item, MSICABINETSTREAM, entry );

        list_remove( &cab->entry );
        IStorage_Release( cab->storage );
        msi_free( cab->stream );
        msi_free( cab );
    }

    LIST_FOR_EACH_SAFE( item, cursor, &package->patches )
    {
        MSIPATCHINFO *patch = LIST_ENTRY( item, MSIPATCHINFO, entry );

        list_remove( &patch->entry );
        if (patch->delete_on_close && !DeleteFileW( patch->localfile ))
        {
            ERR("failed to delete %s (%u)\n", debugstr_w(patch->localfile), GetLastError());
        }
        msi_free_patchinfo( patch );
    }

    msi_free( package->BaseURL );
    msi_free( package->PackagePath );
    msi_free( package->ProductCode );
    msi_free( package->ActionFormat );
    msi_free( package->LastAction );
    msi_free( package->LastActionTemplate );
    msi_free( package->langids );

    /* cleanup control event subscriptions */
    msi_event_cleanup_all_subscriptions( package );
}

static void MSI_FreePackage( MSIOBJECTHDR *arg)
{
    MSIPACKAGE *package = (MSIPACKAGE *)arg;

    msi_destroy_assembly_caches( package );

    if( package->dialog )
        msi_dialog_destroy( package->dialog );

    msiobj_release( &package->db->hdr );
    free_package_structures(package);
    CloseHandle( package->log_file );

    if (package->delete_on_close) DeleteFileW( package->localfile );
    msi_free( package->localfile );
    MSI_ProcessMessage(NULL, INSTALLMESSAGE_TERMINATE, 0);
}

static UINT create_temp_property_table(MSIPACKAGE *package)
{
    static const WCHAR query[] = {
        'C','R','E','A','T','E',' ','T','A','B','L','E',' ',
        '`','_','P','r','o','p','e','r','t','y','`',' ','(',' ',
        '`','_','P','r','o','p','e','r','t','y','`',' ',
        'C','H','A','R','(','5','6',')',' ','N','O','T',' ','N','U','L','L',' ',
        'T','E','M','P','O','R','A','R','Y',',',' ',
        '`','V','a','l','u','e','`',' ','C','H','A','R','(','9','8',')',' ',
        'N','O','T',' ','N','U','L','L',' ','T','E','M','P','O','R','A','R','Y',
        ' ','P','R','I','M','A','R','Y',' ','K','E','Y',' ',
        '`','_','P','r','o','p','e','r','t','y','`',')',' ','H','O','L','D',0};
    MSIQUERY *view;
    UINT rc;

    rc = MSI_DatabaseOpenViewW(package->db, query, &view);
    if (rc != ERROR_SUCCESS)
        return rc;

    rc = MSI_ViewExecute(view, 0);
    MSI_ViewClose(view);
    msiobj_release(&view->hdr);
    return rc;
}

UINT msi_clone_properties( MSIDATABASE *db )
{
    static const WCHAR query_select[] = {
        'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
        '`','P','r','o','p','e','r','t','y','`',0};
    static const WCHAR query_insert[] = {
        'I','N','S','E','R','T',' ','I','N','T','O',' ',
        '`','_','P','r','o','p','e','r','t','y','`',' ',
        '(','`','_','P','r','o','p','e','r','t','y','`',',','`','V','a','l','u','e','`',')',' ',
        'V','A','L','U','E','S',' ','(','?',',','?',')',0};
    static const WCHAR query_update[] = {
        'U','P','D','A','T','E',' ','`','_','P','r','o','p','e','r','t','y','`',' ',
        'S','E','T',' ','`','V','a','l','u','e','`',' ','=',' ','?',' ',
        'W','H','E','R','E',' ','`','_','P','r','o','p','e','r','t','y','`',' ','=',' ','?',0};
    MSIQUERY *view_select;
    UINT rc;

    rc = MSI_DatabaseOpenViewW( db, query_select, &view_select );
    if (rc != ERROR_SUCCESS)
        return rc;

    rc = MSI_ViewExecute( view_select, 0 );
    if (rc != ERROR_SUCCESS)
    {
        MSI_ViewClose( view_select );
        msiobj_release( &view_select->hdr );
        return rc;
    }

    while (1)
    {
        MSIQUERY *view_insert, *view_update;
        MSIRECORD *rec_select;

        rc = MSI_ViewFetch( view_select, &rec_select );
        if (rc != ERROR_SUCCESS)
            break;

        rc = MSI_DatabaseOpenViewW( db, query_insert, &view_insert );
        if (rc != ERROR_SUCCESS)
        {
            msiobj_release( &rec_select->hdr );
            continue;
        }

        rc = MSI_ViewExecute( view_insert, rec_select );
        MSI_ViewClose( view_insert );
        msiobj_release( &view_insert->hdr );
        if (rc != ERROR_SUCCESS)
        {
            MSIRECORD *rec_update;

            TRACE("insert failed, trying update\n");

            rc = MSI_DatabaseOpenViewW( db, query_update, &view_update );
            if (rc != ERROR_SUCCESS)
            {
                WARN("open view failed %u\n", rc);
                msiobj_release( &rec_select->hdr );
                continue;
            }

            rec_update = MSI_CreateRecord( 2 );
            MSI_RecordCopyField( rec_select, 1, rec_update, 2 );
            MSI_RecordCopyField( rec_select, 2, rec_update, 1 );
            rc = MSI_ViewExecute( view_update, rec_update );
            if (rc != ERROR_SUCCESS)
                WARN("update failed %u\n", rc);

            MSI_ViewClose( view_update );
            msiobj_release( &view_update->hdr );
            msiobj_release( &rec_update->hdr );
        }

        msiobj_release( &rec_select->hdr );
    }

    MSI_ViewClose( view_select );
    msiobj_release( &view_select->hdr );
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
    HKEY hkey;
    UINT r;

    if (!package->ProductCode) return ERROR_FUNCTION_FAILED;

    r = MSIREG_OpenUninstallKey( package->ProductCode, package->platform, &hkey, FALSE );
    if (r == ERROR_SUCCESS)
    {
        RegCloseKey( hkey );
        msi_set_property( package->db, szInstalled, szOne, -1 );
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

    r = msi_set_property( package->db, szUserSID, sid_str, -1 );

done:
    LocalFree( sid_str );
    msi_free( dom );
    msi_free( psid );
    msi_free( user_name );

    return r;
}

static LPWSTR get_fusion_filename(MSIPACKAGE *package)
{
    static const WCHAR fusion[] =
        {'f','u','s','i','o','n','.','d','l','l',0};
    static const WCHAR subkey[] =
        {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
         'N','E','T',' ','F','r','a','m','e','w','o','r','k',' ','S','e','t','u','p','\\','N','D','P',0};
    static const WCHAR subdir[] =
        {'M','i','c','r','o','s','o','f','t','.','N','E','T','\\','F','r','a','m','e','w','o','r','k','\\',0};
    static const WCHAR v2050727[] =
        {'v','2','.','0','.','5','0','7','2','7',0};
    static const WCHAR v4client[] =
        {'v','4','\\','C','l','i','e','n','t',0};
    static const WCHAR installpath[] =
        {'I','n','s','t','a','l','l','P','a','t','h',0};
    HKEY netsetup, hkey;
    LONG res;
    DWORD size, len, type;
    WCHAR windir[MAX_PATH], path[MAX_PATH], *filename = NULL;

    res = RegOpenKeyExW(HKEY_LOCAL_MACHINE, subkey, 0, KEY_CREATE_SUB_KEY, &netsetup);
    if (res != ERROR_SUCCESS)
        return NULL;

    if (!RegCreateKeyExW(netsetup, v4client, 0, NULL, 0, KEY_QUERY_VALUE, NULL, &hkey, NULL))
    {
        size = sizeof(path)/sizeof(path[0]);
        if (!RegQueryValueExW(hkey, installpath, NULL, &type, (BYTE *)path, &size))
        {
            len = strlenW(path) + strlenW(fusion) + 2;
            if (!(filename = msi_alloc(len * sizeof(WCHAR)))) return NULL;

            strcpyW(filename, path);
            strcatW(filename, szBackSlash);
            strcatW(filename, fusion);
            if (GetFileAttributesW(filename) != INVALID_FILE_ATTRIBUTES)
            {
                TRACE( "found %s\n", debugstr_w(filename) );
                RegCloseKey(hkey);
                RegCloseKey(netsetup);
                return filename;
            }
        }
        RegCloseKey(hkey);
    }

    if (!RegCreateKeyExW(netsetup, v2050727, 0, NULL, 0, KEY_QUERY_VALUE, NULL, &hkey, NULL))
    {
        RegCloseKey(hkey);
        GetWindowsDirectoryW(windir, MAX_PATH);
        len = strlenW(windir) + strlenW(subdir) + strlenW(v2050727) + strlenW(fusion) + 3;
        if (!(filename = msi_alloc(len * sizeof(WCHAR)))) return NULL;

        strcpyW(filename, windir);
        strcatW(filename, szBackSlash);
        strcatW(filename, subdir);
        strcatW(filename, v2050727);
        strcatW(filename, szBackSlash);
        strcatW(filename, fusion);
        if (GetFileAttributesW(filename) != INVALID_FILE_ATTRIBUTES)
        {
            TRACE( "found %s\n", debugstr_w(filename) );
            RegCloseKey(netsetup);
            return filename;
        }
    }

    RegCloseKey(netsetup);
    return filename;
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
    if (!size)
        goto done;

    version = msi_alloc(size);
    if (!version)
        goto done;

    if (!GetFileVersionInfoW(fusion, handle, size, version))
        goto done;

    if (!VerQueryValueW(version, translation, (LPVOID *)&translate, &val_len))
        goto done;

    sprintfW(buf, verfmt, translate[0].wLanguage, translate[0].wCodePage);

    if (!VerQueryValueW(version, buf, (LPVOID *)&verstr, &val_len))
        goto done;

    if (!val_len || !verstr)
        goto done;

    msi_set_property( package->db, netasm, verstr, -1 );

done:
    msi_free(fusion);
    msi_free(version);
}

static VOID set_installer_properties(MSIPACKAGE *package)
{
    WCHAR *ptr;
    OSVERSIONINFOEXW OSVersion;
    MEMORYSTATUSEX msex;
    DWORD verval, len;
    WCHAR pth[MAX_PATH], verstr[11], bufstr[22];
    HDC dc;
    HKEY hkey;
    LPWSTR username, companyname;
    SYSTEM_INFO sys_info;
    LANGID langid;

    static const WCHAR szCommonFilesFolder[] = {'C','o','m','m','o','n','F','i','l','e','s','F','o','l','d','e','r',0};
    static const WCHAR szProgramFilesFolder[] = {'P','r','o','g','r','a','m','F','i','l','e','s','F','o','l','d','e','r',0};
    static const WCHAR szCommonAppDataFolder[] = {'C','o','m','m','o','n','A','p','p','D','a','t','a','F','o','l','d','e','r',0};
    static const WCHAR szFavoritesFolder[] = {'F','a','v','o','r','i','t','e','s','F','o','l','d','e','r',0};
    static const WCHAR szFontsFolder[] = {'F','o','n','t','s','F','o','l','d','e','r',0};
    static const WCHAR szSendToFolder[] = {'S','e','n','d','T','o','F','o','l','d','e','r',0};
    static const WCHAR szStartMenuFolder[] = {'S','t','a','r','t','M','e','n','u','F','o','l','d','e','r',0};
    static const WCHAR szStartupFolder[] = {'S','t','a','r','t','u','p','F','o','l','d','e','r',0};
    static const WCHAR szTemplateFolder[] = {'T','e','m','p','l','a','t','e','F','o','l','d','e','r',0};
    static const WCHAR szDesktopFolder[] = {'D','e','s','k','t','o','p','F','o','l','d','e','r',0};
    static const WCHAR szProgramMenuFolder[] = {'P','r','o','g','r','a','m','M','e','n','u','F','o','l','d','e','r',0};
    static const WCHAR szAdminToolsFolder[] = {'A','d','m','i','n','T','o','o','l','s','F','o','l','d','e','r',0};
    static const WCHAR szSystemFolder[] = {'S','y','s','t','e','m','F','o','l','d','e','r',0};
    static const WCHAR szSystem16Folder[] = {'S','y','s','t','e','m','1','6','F','o','l','d','e','r',0};
    static const WCHAR szLocalAppDataFolder[] = {'L','o','c','a','l','A','p','p','D','a','t','a','F','o','l','d','e','r',0};
    static const WCHAR szMyPicturesFolder[] = {'M','y','P','i','c','t','u','r','e','s','F','o','l','d','e','r',0};
    static const WCHAR szPersonalFolder[] = {'P','e','r','s','o','n','a','l','F','o','l','d','e','r',0};
    static const WCHAR szWindowsVolume[] = {'W','i','n','d','o','w','s','V','o','l','u','m','e',0};
    static const WCHAR szPrivileged[] = {'P','r','i','v','i','l','e','g','e','d',0};
    static const WCHAR szVersion9x[] = {'V','e','r','s','i','o','n','9','X',0};
    static const WCHAR szVersionNT[] = {'V','e','r','s','i','o','n','N','T',0};
    static const WCHAR szMsiNTProductType[] = {'M','s','i','N','T','P','r','o','d','u','c','t','T','y','p','e',0};
    static const WCHAR szFormat[] = {'%','u',0};
    static const WCHAR szFormat2[] = {'%','u','.','%','u',0};
    static const WCHAR szWindowsBuild[] = {'W','i','n','d','o','w','s','B','u','i','l','d',0};
    static const WCHAR szServicePackLevel[] = {'S','e','r','v','i','c','e','P','a','c','k','L','e','v','e','l',0};
    static const WCHAR szVersionMsi[] = { 'V','e','r','s','i','o','n','M','s','i',0 };
    static const WCHAR szVersionDatabase[] = { 'V','e','r','s','i','o','n','D','a','t','a','b','a','s','e',0 };
    static const WCHAR szPhysicalMemory[] = { 'P','h','y','s','i','c','a','l','M','e','m','o','r','y',0 };
    static const WCHAR szScreenX[] = {'S','c','r','e','e','n','X',0};
    static const WCHAR szScreenY[] = {'S','c','r','e','e','n','Y',0};
    static const WCHAR szColorBits[] = {'C','o','l','o','r','B','i','t','s',0};
    static const WCHAR szIntFormat[] = {'%','d',0};
    static const WCHAR szMsiAMD64[] = { 'M','s','i','A','M','D','6','4',0 };
    static const WCHAR szMsix64[] = { 'M','s','i','x','6','4',0 };
    static const WCHAR szSystem64Folder[] = { 'S','y','s','t','e','m','6','4','F','o','l','d','e','r',0 };
    static const WCHAR szCommonFiles64Folder[] = { 'C','o','m','m','o','n','F','i','l','e','s','6','4','F','o','l','d','e','r',0 };
    static const WCHAR szProgramFiles64Folder[] = { 'P','r','o','g','r','a','m','F','i','l','e','s','6','4','F','o','l','d','e','r',0 };
    static const WCHAR szVersionNT64[] = { 'V','e','r','s','i','o','n','N','T','6','4',0 };
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
    static const WCHAR szRegisteredOrganization[] = {
        'R','e','g','i','s','t','e','r','e','d','O','r','g','a','n','i','z','a','t','i','o','n',0
    };
    static const WCHAR szUSERNAME[] = {'U','S','E','R','N','A','M','E',0};
    static const WCHAR szCOMPANYNAME[] = {'C','O','M','P','A','N','Y','N','A','M','E',0};
    static const WCHAR szUserLanguageID[] = {'U','s','e','r','L','a','n','g','u','a','g','e','I','D',0};
    static const WCHAR szSystemLangID[] = {'S','y','s','t','e','m','L','a','n','g','u','a','g','e','I','D',0};
    static const WCHAR szProductState[] = {'P','r','o','d','u','c','t','S','t','a','t','e',0};
    static const WCHAR szLogonUser[] = {'L','o','g','o','n','U','s','e','r',0};
    static const WCHAR szNetHoodFolder[] = {'N','e','t','H','o','o','d','F','o','l','d','e','r',0};
    static const WCHAR szPrintHoodFolder[] = {'P','r','i','n','t','H','o','o','d','F','o','l','d','e','r',0};
    static const WCHAR szRecentFolder[] = {'R','e','c','e','n','t','F','o','l','d','e','r',0};
    static const WCHAR szComputerName[] = {'C','o','m','p','u','t','e','r','N','a','m','e',0};

    /*
     * Other things that probably should be set:
     *
     * VirtualMemory ShellAdvSupport DefaultUIFont PackagecodeChanging
     * CaptionHeight BorderTop BorderSide TextHeight RedirectedDllSupport
     */

    SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, 0, pth);
    strcatW(pth, szBackSlash);
    msi_set_property( package->db, szCommonAppDataFolder, pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_FAVORITES, NULL, 0, pth);
    strcatW(pth, szBackSlash);
    msi_set_property( package->db, szFavoritesFolder, pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_FONTS, NULL, 0, pth);
    strcatW(pth, szBackSlash);
    msi_set_property( package->db, szFontsFolder, pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_SENDTO, NULL, 0, pth);
    strcatW(pth, szBackSlash);
    msi_set_property( package->db, szSendToFolder, pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_STARTMENU, NULL, 0, pth);
    strcatW(pth, szBackSlash);
    msi_set_property( package->db, szStartMenuFolder, pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_STARTUP, NULL, 0, pth);
    strcatW(pth, szBackSlash);
    msi_set_property( package->db, szStartupFolder, pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_TEMPLATES, NULL, 0, pth);
    strcatW(pth, szBackSlash);
    msi_set_property( package->db, szTemplateFolder, pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_DESKTOP, NULL, 0, pth);
    strcatW(pth, szBackSlash);
    msi_set_property( package->db, szDesktopFolder, pth, -1 );

    /* FIXME: set to AllUsers profile path if ALLUSERS is set */
    SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, 0, pth);
    strcatW(pth, szBackSlash);
    msi_set_property( package->db, szProgramMenuFolder, pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_ADMINTOOLS, NULL, 0, pth);
    strcatW(pth, szBackSlash);
    msi_set_property( package->db, szAdminToolsFolder, pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, pth);
    strcatW(pth, szBackSlash);
    msi_set_property( package->db, szAppDataFolder, pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_SYSTEM, NULL, 0, pth);
    strcatW(pth, szBackSlash);
    msi_set_property( package->db, szSystemFolder, pth, -1 );
    msi_set_property( package->db, szSystem16Folder, pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, pth);
    strcatW(pth, szBackSlash);
    msi_set_property( package->db, szLocalAppDataFolder, pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_MYPICTURES, NULL, 0, pth);
    strcatW(pth, szBackSlash);
    msi_set_property( package->db, szMyPicturesFolder, pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, 0, pth);
    strcatW(pth, szBackSlash);
    msi_set_property( package->db, szPersonalFolder, pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_WINDOWS, NULL, 0, pth);
    strcatW(pth, szBackSlash);
    msi_set_property( package->db, szWindowsFolder, pth, -1 );
    
    SHGetFolderPathW(NULL, CSIDL_PRINTHOOD, NULL, 0, pth);
    strcatW(pth, szBackSlash);
    msi_set_property( package->db, szPrintHoodFolder, pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_NETHOOD, NULL, 0, pth);
    strcatW(pth, szBackSlash);
    msi_set_property( package->db, szNetHoodFolder, pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_RECENT, NULL, 0, pth);
    strcatW(pth, szBackSlash);
    msi_set_property( package->db, szRecentFolder, pth, -1 );

    /* Physical Memory is specified in MB. Using total amount. */
    msex.dwLength = sizeof(msex);
    GlobalMemoryStatusEx( &msex );
    len = sprintfW( bufstr, szIntFormat, (int)(msex.ullTotalPhys / 1024 / 1024) );
    msi_set_property( package->db, szPhysicalMemory, bufstr, len );

    SHGetFolderPathW(NULL, CSIDL_WINDOWS, NULL, 0, pth);
    ptr = strchrW(pth,'\\');
    if (ptr) *(ptr + 1) = 0;
    msi_set_property( package->db, szWindowsVolume, pth, -1 );
    
    len = GetTempPathW(MAX_PATH, pth);
    msi_set_property( package->db, szTempFolder, pth, len );

    /* in a wine environment the user is always admin and privileged */
    msi_set_property( package->db, szAdminUser, szOne, -1 );
    msi_set_property( package->db, szPrivileged, szOne, -1 );

    /* set the os things */
    OSVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
    GetVersionExW((OSVERSIONINFOW *)&OSVersion);
    verval = OSVersion.dwMinorVersion + OSVersion.dwMajorVersion * 100;
    len = sprintfW( verstr, szFormat, verval );
    switch (OSVersion.dwPlatformId)
    {
        case VER_PLATFORM_WIN32_WINDOWS:    
            msi_set_property( package->db, szVersion9x, verstr, len );
            break;
        case VER_PLATFORM_WIN32_NT:
            msi_set_property( package->db, szVersionNT, verstr, len );
            len = sprintfW( bufstr, szFormat,OSVersion.wProductType );
            msi_set_property( package->db, szMsiNTProductType, bufstr, len );
            break;
    }
    len = sprintfW( bufstr, szFormat, OSVersion.dwBuildNumber );
    msi_set_property( package->db, szWindowsBuild, bufstr, len );
    len = sprintfW( bufstr, szFormat, OSVersion.wServicePackMajor );
    msi_set_property( package->db, szServicePackLevel, bufstr, len );

    len = sprintfW( bufstr, szFormat2, MSI_MAJORVERSION, MSI_MINORVERSION );
    msi_set_property( package->db, szVersionMsi, bufstr, len );
    len = sprintfW( bufstr, szFormat, MSI_MAJORVERSION * 100 );
    msi_set_property( package->db, szVersionDatabase, bufstr, len );

    GetNativeSystemInfo( &sys_info );
    len = sprintfW( bufstr, szIntFormat, sys_info.wProcessorLevel );
    msi_set_property( package->db, szIntel, bufstr, len );
    if (sys_info.u.s.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
    {
        GetSystemDirectoryW( pth, MAX_PATH );
        PathAddBackslashW( pth );
        msi_set_property( package->db, szSystemFolder, pth, -1 );

        SHGetFolderPathW( NULL, CSIDL_PROGRAM_FILES, NULL, 0, pth );
        PathAddBackslashW( pth );
        msi_set_property( package->db, szProgramFilesFolder, pth, -1 );

        SHGetFolderPathW( NULL, CSIDL_PROGRAM_FILES_COMMON, NULL, 0, pth );
        PathAddBackslashW( pth );
        msi_set_property( package->db, szCommonFilesFolder, pth, -1 );
    }
    else if (sys_info.u.s.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
    {
        msi_set_property( package->db, szMsiAMD64, bufstr, -1 );
        msi_set_property( package->db, szMsix64, bufstr, -1 );
        msi_set_property( package->db, szVersionNT64, verstr, -1 );

        GetSystemDirectoryW( pth, MAX_PATH );
        PathAddBackslashW( pth );
        msi_set_property( package->db, szSystem64Folder, pth, -1 );

        GetSystemWow64DirectoryW( pth, MAX_PATH );
        PathAddBackslashW( pth );
        msi_set_property( package->db, szSystemFolder, pth, -1 );

        SHGetFolderPathW( NULL, CSIDL_PROGRAM_FILES, NULL, 0, pth );
        PathAddBackslashW( pth );
        msi_set_property( package->db, szProgramFiles64Folder, pth, -1 );

        SHGetFolderPathW( NULL, CSIDL_PROGRAM_FILESX86, NULL, 0, pth );
        PathAddBackslashW( pth );
        msi_set_property( package->db, szProgramFilesFolder, pth, -1 );

        SHGetFolderPathW( NULL, CSIDL_PROGRAM_FILES_COMMON, NULL, 0, pth );
        PathAddBackslashW( pth );
        msi_set_property( package->db, szCommonFiles64Folder, pth, -1 );

        SHGetFolderPathW( NULL, CSIDL_PROGRAM_FILES_COMMONX86, NULL, 0, pth );
        PathAddBackslashW( pth );
        msi_set_property( package->db, szCommonFilesFolder, pth, -1 );
    }

    /* Screen properties. */
    dc = GetDC(0);
    len = sprintfW( bufstr, szIntFormat, GetDeviceCaps(dc, HORZRES) );
    msi_set_property( package->db, szScreenX, bufstr, len );
    len = sprintfW( bufstr, szIntFormat, GetDeviceCaps(dc, VERTRES) );
    msi_set_property( package->db, szScreenY, bufstr, len );
    len = sprintfW( bufstr, szIntFormat, GetDeviceCaps(dc, BITSPIXEL) );
    msi_set_property( package->db, szColorBits, bufstr, len );
    ReleaseDC(0, dc);

    /* USERNAME and COMPANYNAME */
    username = msi_dup_property( package->db, szUSERNAME );
    companyname = msi_dup_property( package->db, szCOMPANYNAME );

    if ((!username || !companyname) &&
        RegOpenKeyW( HKEY_CURRENT_USER, szUserInfo, &hkey ) == ERROR_SUCCESS)
    {
        if (!username &&
            (username = msi_reg_get_val_str( hkey, szDefName )))
            msi_set_property( package->db, szUSERNAME, username, -1 );
        if (!companyname &&
            (companyname = msi_reg_get_val_str( hkey, szDefCompany )))
            msi_set_property( package->db, szCOMPANYNAME, companyname, -1 );
        CloseHandle( hkey );
    }
    if ((!username || !companyname) &&
        RegOpenKeyW( HKEY_LOCAL_MACHINE, szCurrentVersion, &hkey ) == ERROR_SUCCESS)
    {
        if (!username &&
            (username = msi_reg_get_val_str( hkey, szRegisteredUser )))
            msi_set_property( package->db, szUSERNAME, username, -1 );
        if (!companyname &&
            (companyname = msi_reg_get_val_str( hkey, szRegisteredOrganization )))
            msi_set_property( package->db, szCOMPANYNAME, companyname, -1 );
        CloseHandle( hkey );
    }
    msi_free( username );
    msi_free( companyname );

    if ( set_user_sid_prop( package ) != ERROR_SUCCESS)
        ERR("Failed to set the UserSID property\n");

    set_msi_assembly_prop( package );

    langid = GetUserDefaultLangID();
    len = sprintfW( bufstr, szIntFormat, langid );
    msi_set_property( package->db, szUserLanguageID, bufstr, len );

    langid = GetSystemDefaultLangID();
    len = sprintfW( bufstr, szIntFormat, langid );
    msi_set_property( package->db, szSystemLangID, bufstr, len );

    len = sprintfW( bufstr, szIntFormat, MsiQueryProductStateW(package->ProductCode) );
    msi_set_property( package->db, szProductState, bufstr, len );

    len = 0;
    if (!GetUserNameW( NULL, &len ) && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        WCHAR *username;
        if ((username = msi_alloc( len * sizeof(WCHAR) )))
        {
            if (GetUserNameW( username, &len ))
                msi_set_property( package->db, szLogonUser, username, len - 1 );
            msi_free( username );
        }
    }
    len = 0;
    if (!GetComputerNameW( NULL, &len ) && GetLastError() == ERROR_BUFFER_OVERFLOW)
    {
        WCHAR *computername;
        if ((computername = msi_alloc( len * sizeof(WCHAR) )))
        {
            if (GetComputerNameW( computername, &len ))
                msi_set_property( package->db, szComputerName, computername, len );
            msi_free( computername );
        }
    }
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
        list_init( &package->filepatches );
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
        list_init( &package->patches );
        list_init( &package->binaries );
        list_init( &package->cabinet_streams );
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

void msi_adjust_privilege_properties( MSIPACKAGE *package )
{
    /* FIXME: this should depend on the user's privileges */
    if (msi_get_property_int( package->db, szAllUsers, 0 ) == 2)
    {
        TRACE("resetting ALLUSERS property from 2 to 1\n");
        msi_set_property( package->db, szAllUsers, szOne, -1 );
    }
    msi_set_property( package->db, szAdminUser, szOne, -1 );
}

MSIPACKAGE *MSI_CreatePackage( MSIDATABASE *db, LPCWSTR base_url )
{
    static const WCHAR fmtW[] = {'%','u',0};
    MSIPACKAGE *package;
    WCHAR uilevel[11];
    int len;
    UINT r;

    TRACE("%p\n", db);

    package = msi_alloc_package();
    if (package)
    {
        msiobj_addref( &db->hdr );
        package->db = db;

        package->LastAction = NULL;
        package->LastActionTemplate = NULL;
        package->LastActionResult = MSI_NULL_INTEGER;
        package->WordCount = 0;
        package->PackagePath = strdupW( db->path );
        package->BaseURL = strdupW( base_url );

        create_temp_property_table( package );
        msi_clone_properties( package->db );
        msi_adjust_privilege_properties( package );

        package->ProductCode = msi_dup_property( package->db, szProductCode );

        set_installer_properties( package );

        package->ui_level = gUILevel;
        len = sprintfW( uilevel, fmtW, gUILevel & INSTALLUILEVEL_MASK );
        msi_set_property( package->db, szUILevel, uilevel, len );

        r = msi_load_suminfo_properties( package );
        if (r != ERROR_SUCCESS)
        {
            msiobj_release( &package->hdr );
            return NULL;
        }

        if (package->WordCount & msidbSumInfoSourceTypeAdminImage)
            msi_load_admin_properties( package );

        package->log_file = INVALID_HANDLE_VALUE;
    }
    return package;
}

UINT msi_download_file( LPCWSTR szUrl, LPWSTR filename )
{
    LPINTERNET_CACHE_ENTRY_INFOW cache_entry;
    DWORD size = 0;
    HRESULT hr;

    /* call will always fail, because size is 0,
     * but will return ERROR_FILE_NOT_FOUND first
     * if the file doesn't exist
     */
    GetUrlCacheEntryInfoW( szUrl, NULL, &size );
    if ( GetLastError() != ERROR_FILE_NOT_FOUND )
    {
        cache_entry = msi_alloc( size );
        if ( !GetUrlCacheEntryInfoW( szUrl, cache_entry, &size ) )
        {
            UINT error = GetLastError();
            msi_free( cache_entry );
            return error;
        }

        lstrcpyW( filename, cache_entry->lpszLocalFileName );
        msi_free( cache_entry );
        return ERROR_SUCCESS;
    }

    hr = URLDownloadToCacheFileW( NULL, szUrl, filename, MAX_PATH, 0, NULL );
    if ( FAILED(hr) )
    {
        WARN("failed to download %s to cache file\n", debugstr_w(szUrl));
        return ERROR_FUNCTION_FAILED;
    }

    return ERROR_SUCCESS;
}

UINT msi_create_empty_local_file( LPWSTR path, LPCWSTR suffix )
{
    static const WCHAR szInstaller[] = {
        '\\','I','n','s','t','a','l','l','e','r','\\',0};
    static const WCHAR fmt[] = {'%','x',0};
    DWORD time, len, i, offset;
    HANDLE handle;

    time = GetTickCount();
    GetWindowsDirectoryW( path, MAX_PATH );
    strcatW( path, szInstaller );
    CreateDirectoryW( path, NULL );

    len = strlenW(path);
    for (i = 0; i < 0x10000; i++)
    {
        offset = snprintfW( path + len, MAX_PATH - len, fmt, (time + i) & 0xffff );
        memcpy( path + len + offset, suffix, (strlenW( suffix ) + 1) * sizeof(WCHAR) );
        handle = CreateFileW( path, GENERIC_WRITE, 0, NULL,
                              CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0 );
        if (handle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(handle);
            break;
        }
        if (GetLastError() != ERROR_FILE_EXISTS &&
            GetLastError() != ERROR_SHARING_VIOLATION)
            return ERROR_FUNCTION_FAILED;
    }

    return ERROR_SUCCESS;
}

static enum platform parse_platform( const WCHAR *str )
{
    if (!str[0] || !strcmpW( str, szIntel )) return PLATFORM_INTEL;
    else if (!strcmpW( str, szIntel64 )) return PLATFORM_INTEL64;
    else if (!strcmpW( str, szX64 ) || !strcmpW( str, szAMD64 )) return PLATFORM_X64;
    else if (!strcmpW( str, szARM )) return PLATFORM_ARM;
    return PLATFORM_UNKNOWN;
}

static UINT parse_suminfo( MSISUMMARYINFO *si, MSIPACKAGE *package )
{
    WCHAR *template, *p, *q, *platform;
    DWORD i, count;

    package->version = msi_suminfo_get_int32( si, PID_PAGECOUNT );
    TRACE("version: %d\n", package->version);

    template = msi_suminfo_dup_string( si, PID_TEMPLATE );
    if (!template)
        return ERROR_SUCCESS; /* native accepts missing template property */

    TRACE("template: %s\n", debugstr_w(template));

    p = strchrW( template, ';' );
    if (!p)
    {
        WARN("invalid template string %s\n", debugstr_w(template));
        msi_free( template );
        return ERROR_PATCH_PACKAGE_INVALID;
    }
    *p = 0;
    platform = template;
    if ((q = strchrW( platform, ',' ))) *q = 0;
    package->platform = parse_platform( platform );
    while (package->platform == PLATFORM_UNKNOWN && q)
    {
        platform = q + 1;
        if ((q = strchrW( platform, ',' ))) *q = 0;
        package->platform = parse_platform( platform );
    }
    if (package->platform == PLATFORM_UNKNOWN)
    {
        WARN("unknown platform %s\n", debugstr_w(template));
        msi_free( template );
        return ERROR_INSTALL_PLATFORM_UNSUPPORTED;
    }
    p++;
    if (!*p)
    {
        msi_free( template );
        return ERROR_SUCCESS;
    }
    count = 1;
    for (q = p; (q = strchrW( q, ',' )); q++) count++;

    package->langids = msi_alloc( count * sizeof(LANGID) );
    if (!package->langids)
    {
        msi_free( template );
        return ERROR_OUTOFMEMORY;
    }

    i = 0;
    while (*p)
    {
        q = strchrW( p, ',' );
        if (q) *q = 0;
        package->langids[i] = atoiW( p );
        if (!q) break;
        p = q + 1;
        i++;
    }
    package->num_langids = i + 1;

    msi_free( template );
    return ERROR_SUCCESS;
}

static UINT validate_package( MSIPACKAGE *package )
{
    UINT i;

    if (package->platform == PLATFORM_INTEL64)
        return ERROR_INSTALL_PLATFORM_UNSUPPORTED;
#ifndef __arm__
    if (package->platform == PLATFORM_ARM)
        return ERROR_INSTALL_PLATFORM_UNSUPPORTED;
#endif
    if (package->platform == PLATFORM_X64)
    {
        if (!is_64bit && !is_wow64)
            return ERROR_INSTALL_PLATFORM_UNSUPPORTED;
        if (package->version < 200)
            return ERROR_INSTALL_PACKAGE_INVALID;
    }
    if (!package->num_langids)
    {
        return ERROR_SUCCESS;
    }
    for (i = 0; i < package->num_langids; i++)
    {
        LANGID langid = package->langids[i];

        if (PRIMARYLANGID( langid ) == LANG_NEUTRAL)
        {
            langid = MAKELANGID( PRIMARYLANGID( GetSystemDefaultLangID() ), SUBLANGID( langid ) );
        }
        if (SUBLANGID( langid ) == SUBLANG_NEUTRAL)
        {
            langid = MAKELANGID( PRIMARYLANGID( langid ), SUBLANGID( GetSystemDefaultLangID() ) );
        }
        if (IsValidLocale( langid, LCID_INSTALLED ))
            return ERROR_SUCCESS;
    }
    return ERROR_INSTALL_LANGUAGE_UNSUPPORTED;
}

static WCHAR *get_product_code( MSIDATABASE *db )
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','`','V','a','l','u','e','`',' ',
        'F','R','O','M',' ','`','P','r','o','p','e','r','t','y','`',' ',
        'W','H','E','R','E',' ','`','P','r','o','p','e','r','t','y','`','=',
        '\'','P','r','o','d','u','c','t','C','o','d','e','\'',0};
    MSIQUERY *view;
    MSIRECORD *rec;
    WCHAR *ret = NULL;

    if (MSI_DatabaseOpenViewW( db, query, &view ) != ERROR_SUCCESS)
    {
        return NULL;
    }
    if (MSI_ViewExecute( view, 0 ) != ERROR_SUCCESS)
    {
        MSI_ViewClose( view );
        msiobj_release( &view->hdr );
        return NULL;
    }
    if (MSI_ViewFetch( view, &rec ) == ERROR_SUCCESS)
    {
        ret = strdupW( MSI_RecordGetString( rec, 1 ) );
        msiobj_release( &rec->hdr );
    }
    MSI_ViewClose( view );
    msiobj_release( &view->hdr );
    return ret;
}

static UINT get_registered_local_package( const WCHAR *product, const WCHAR *package, WCHAR *localfile )
{
    MSIINSTALLCONTEXT context;
    HKEY product_key, props_key;
    WCHAR *registered_package = NULL, unsquashed[GUID_SIZE];
    UINT r;

    r = msi_locate_product( product, &context );
    if (r != ERROR_SUCCESS)
        return r;

    r = MSIREG_OpenProductKey( product, NULL, context, &product_key, FALSE );
    if (r != ERROR_SUCCESS)
        return r;

    r = MSIREG_OpenInstallProps( product, context, NULL, &props_key, FALSE );
    if (r != ERROR_SUCCESS)
    {
        RegCloseKey( product_key );
        return r;
    }
    r = ERROR_FUNCTION_FAILED;
    registered_package = msi_reg_get_val_str( product_key, INSTALLPROPERTY_PACKAGECODEW );
    if (!registered_package)
        goto done;

    unsquash_guid( registered_package, unsquashed );
    if (!strcmpiW( package, unsquashed ))
    {
        WCHAR *filename = msi_reg_get_val_str( props_key, INSTALLPROPERTY_LOCALPACKAGEW );
        if (!filename)
            goto done;

        strcpyW( localfile, filename );
        msi_free( filename );
        r = ERROR_SUCCESS;
    }
done:
    msi_free( registered_package );
    RegCloseKey( props_key );
    RegCloseKey( product_key );
    return r;
}

static WCHAR *get_package_code( MSIDATABASE *db )
{
    WCHAR *ret;
    MSISUMMARYINFO *si;
    UINT r;

    r = msi_get_suminfo( db->storage, 0, &si );
    if (r != ERROR_SUCCESS)
    {
        r = msi_get_db_suminfo( db, 0, &si );
        if (r != ERROR_SUCCESS)
        {
            WARN("failed to load summary info %u\n", r);
            return NULL;
        }
    }
    ret = msi_suminfo_dup_string( si, PID_REVNUMBER );
    msiobj_release( &si->hdr );
    return ret;
}

static UINT get_local_package( const WCHAR *filename, WCHAR *localfile )
{
    WCHAR *product_code, *package_code;
    MSIDATABASE *db;
    UINT r;

    if ((r = MSI_OpenDatabaseW( filename, MSIDBOPEN_READONLY, &db )) != ERROR_SUCCESS)
    {
        if (GetFileAttributesW( filename ) == INVALID_FILE_ATTRIBUTES)
            return ERROR_FILE_NOT_FOUND;
        return r;
    }
    if (!(product_code = get_product_code( db )))
    {
        msiobj_release( &db->hdr );
        return ERROR_INSTALL_PACKAGE_INVALID;
    }
    if (!(package_code = get_package_code( db )))
    {
        msi_free( product_code );
        msiobj_release( &db->hdr );
        return ERROR_INSTALL_PACKAGE_INVALID;
    }
    r = get_registered_local_package( product_code, package_code, localfile );
    msi_free( package_code );
    msi_free( product_code );
    msiobj_release( &db->hdr );
    return r;
}

UINT msi_set_original_database_property( MSIDATABASE *db, const WCHAR *package )
{
    UINT r;

    if (UrlIsW( package, URLIS_URL ))
        r = msi_set_property( db, szOriginalDatabase, package, -1 );
    else if (package[0] == '#')
        r = msi_set_property( db, szOriginalDatabase, db->path, -1 );
    else
    {
        DWORD len;
        WCHAR *path;

        if (!(len = GetFullPathNameW( package, 0, NULL, NULL ))) return GetLastError();
        if (!(path = msi_alloc( len * sizeof(WCHAR) ))) return ERROR_OUTOFMEMORY;
        len = GetFullPathNameW( package, len, path, NULL );
        r = msi_set_property( db, szOriginalDatabase, path, len );
        msi_free( path );
    }
    return r;
}

UINT MSI_OpenPackageW(LPCWSTR szPackage, MSIPACKAGE **pPackage)
{
    static const WCHAR dotmsi[] = {'.','m','s','i',0};
    MSIDATABASE *db;
    MSIPACKAGE *package;
    MSIHANDLE handle;
    MSIRECORD *data_row, *info_row;
    LPWSTR ptr, base_url = NULL;
    UINT r;
    WCHAR localfile[MAX_PATH], cachefile[MAX_PATH];
    LPCWSTR file = szPackage;
    DWORD index = 0;
    MSISUMMARYINFO *si;
    BOOL delete_on_close = FALSE;
    LPWSTR productname;
    WCHAR *info_template;

    TRACE("%s %p\n", debugstr_w(szPackage), pPackage);

    MSI_ProcessMessage(NULL, INSTALLMESSAGE_INITIALIZE, 0);

    localfile[0] = 0;
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
            r = msi_download_file( szPackage, cachefile );
            if (r != ERROR_SUCCESS)
                return r;

            file = cachefile;

            base_url = strdupW( szPackage );
            if (!base_url)
                return ERROR_OUTOFMEMORY;

            ptr = strrchrW( base_url, '/' );
            if (ptr) *(ptr + 1) = '\0';
        }
        r = get_local_package( file, localfile );
        if (r != ERROR_SUCCESS || GetFileAttributesW( localfile ) == INVALID_FILE_ATTRIBUTES)
        {
            DWORD localfile_attr;

            r = msi_create_empty_local_file( localfile, dotmsi );
            if (r != ERROR_SUCCESS)
            {
                msi_free ( base_url );
                return r;
            }

            if (!CopyFileW( file, localfile, FALSE ))
            {
                r = GetLastError();
                WARN("unable to copy package %s to %s (%u)\n", debugstr_w(file), debugstr_w(localfile), r);
                DeleteFileW( localfile );
                msi_free ( base_url );
                return r;
            }
            delete_on_close = TRUE;

            /* Remove read-only bit, we are opening it with write access in MSI_OpenDatabaseW below. */
            localfile_attr = GetFileAttributesW( localfile );
            if (localfile_attr & FILE_ATTRIBUTE_READONLY)
                SetFileAttributesW( localfile, localfile_attr & ~FILE_ATTRIBUTE_READONLY);
        }
        TRACE("opening package %s\n", debugstr_w( localfile ));
        r = MSI_OpenDatabaseW( localfile, MSIDBOPEN_TRANSACT, &db );
        if (r != ERROR_SUCCESS)
        {
            msi_free ( base_url );
            return r;
        }
    }
    package = MSI_CreatePackage( db, base_url );
    msi_free( base_url );
    msiobj_release( &db->hdr );
    if (!package) return ERROR_INSTALL_PACKAGE_INVALID;
    package->localfile = strdupW( localfile );
    package->delete_on_close = delete_on_close;

    r = msi_get_suminfo( db->storage, 0, &si );
    if (r != ERROR_SUCCESS)
    {
        r = msi_get_db_suminfo( db, 0, &si );
        if (r != ERROR_SUCCESS)
        {
            WARN("failed to load summary info\n");
            msiobj_release( &package->hdr );
            return ERROR_INSTALL_PACKAGE_INVALID;
        }
    }
    r = parse_suminfo( si, package );
    msiobj_release( &si->hdr );
    if (r != ERROR_SUCCESS)
    {
        WARN("failed to parse summary info %u\n", r);
        msiobj_release( &package->hdr );
        return r;
    }
    r = validate_package( package );
    if (r != ERROR_SUCCESS)
    {
        msiobj_release( &package->hdr );
        return r;
    }
    msi_set_property( package->db, szDatabase, db->path, -1 );
    set_installed_prop( package );
    msi_set_context( package );

    while (1)
    {
        WCHAR patch_code[GUID_SIZE];
        r = MsiEnumPatchesExW( package->ProductCode, NULL, package->Context,
                               MSIPATCHSTATE_APPLIED, index, patch_code, NULL, NULL, NULL, NULL );
        if (r != ERROR_SUCCESS)
            break;

        TRACE("found registered patch %s\n", debugstr_w(patch_code));

        r = msi_apply_registered_patch( package, patch_code );
        if (r != ERROR_SUCCESS)
        {
            ERR("registered patch failed to apply %u\n", r);
            msiobj_release( &package->hdr );
            return r;
        }
        index++;
    }
    if (index) msi_adjust_privilege_properties( package );

    r = msi_set_original_database_property( package->db, szPackage );
    if (r != ERROR_SUCCESS)
    {
        msiobj_release( &package->hdr );
        return r;
    }
    if (gszLogFile)
        package->log_file = CreateFileW( gszLogFile, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
                                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

    /* FIXME: when should these messages be sent? */
    data_row = MSI_CreateRecord(3);
    if (!data_row)
	return ERROR_OUTOFMEMORY;
    MSI_RecordSetStringW(data_row, 0, NULL);
    MSI_RecordSetInteger(data_row, 1, 0);
    MSI_RecordSetInteger(data_row, 2, package->num_langids ? package->langids[0] : 0);
    MSI_RecordSetInteger(data_row, 3, msi_get_string_table_codepage(package->db->strings));
    MSI_ProcessMessageVerbatim(package, INSTALLMESSAGE_COMMONDATA, data_row);

    info_row = MSI_CreateRecord(0);
    if (!info_row)
    {
	msiobj_release(&data_row->hdr);
	return ERROR_OUTOFMEMORY;
    }
    info_template = msi_get_error_message(package->db, MSIERR_INFO_LOGGINGSTART);
    MSI_RecordSetStringW(info_row, 0, info_template);
    msi_free(info_template);
    MSI_ProcessMessage(package, INSTALLMESSAGE_INFO|MB_ICONHAND, info_row);

    MSI_ProcessMessage(package, INSTALLMESSAGE_COMMONDATA, data_row);

    productname = msi_dup_property(package->db, INSTALLPROPERTY_PRODUCTNAMEW);
    MSI_RecordSetInteger(data_row, 1, 1);
    MSI_RecordSetStringW(data_row, 2, productname);
    MSI_RecordSetStringW(data_row, 3, NULL);
    MSI_ProcessMessage(package, INSTALLMESSAGE_COMMONDATA, data_row);

    msi_free(productname);
    msiobj_release(&info_row->hdr);
    msiobj_release(&data_row->hdr);

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
    else
        MSI_ProcessMessage(NULL, INSTALLMESSAGE_TERMINATE, 0);

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
    IUnknown *remote_unk;
    IWineMsiRemotePackage *remote_package;

    TRACE("(%d)\n",hInstall);

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE);
    if( package)
    {
        handle = alloc_msihandle( &package->db->hdr );
        msiobj_release( &package->hdr );
    }
    else if ((remote_unk = msi_get_remote(hInstall)))
    {
        if (IUnknown_QueryInterface(remote_unk, &IID_IWineMsiRemotePackage,
                                        (LPVOID *)&remote_package) == S_OK)
        {
            IWineMsiRemotePackage_GetActiveDatabase(remote_package, &handle);
            IWineMsiRemotePackage_Release(remote_package);
        }
        else
        {
            WARN("remote handle %d is not a package\n", hInstall);
        }
        IUnknown_Release(remote_unk);
    }

    return handle;
}

static INT internal_ui_handler(MSIPACKAGE *package, INSTALLMESSAGE eMessageType, MSIRECORD *record, LPCWSTR message)
{
    static const WCHAR szActionData[] = {'A','c','t','i','o','n','D','a','t','a',0};
    static const WCHAR szActionText[] = {'A','c','t','i','o','n','T','e','x','t',0};
    static const WCHAR szSetProgress[] = {'S','e','t','P','r','o','g','r','e','s','s',0};
    static const WCHAR szWindows_Installer[] =
        {'W','i','n','d','o','w','s',' ','I','n','s','t','a','l','l','e','r',0};

    if (!package || (package->ui_level & INSTALLUILEVEL_MASK) == INSTALLUILEVEL_NONE)
        return 0;

    /* todo: check if message needs additional styles (topmost/foreground/modality?) */

    switch (eMessageType & 0xff000000)
    {
    case INSTALLMESSAGE_FATALEXIT:
    case INSTALLMESSAGE_ERROR:
    case INSTALLMESSAGE_OUTOFDISKSPACE:
        if (package->ui_level & INSTALLUILEVEL_PROGRESSONLY) return 0;
        if (!(eMessageType & MB_ICONMASK))
            eMessageType |= MB_ICONEXCLAMATION;
        return MessageBoxW(gUIhwnd, message, szWindows_Installer, eMessageType & 0x00ffffff);
    case INSTALLMESSAGE_WARNING:
        if (package->ui_level & INSTALLUILEVEL_PROGRESSONLY) return 0;
        if (!(eMessageType & MB_ICONMASK))
            eMessageType |= MB_ICONASTERISK;
        return MessageBoxW(gUIhwnd, message, szWindows_Installer, eMessageType & 0x00ffffff);
    case INSTALLMESSAGE_USER:
        if (package->ui_level & INSTALLUILEVEL_PROGRESSONLY) return 0;
        if (!(eMessageType & MB_ICONMASK))
            eMessageType |= MB_USERICON;
        return MessageBoxW(gUIhwnd, message, szWindows_Installer, eMessageType & 0x00ffffff);
    case INSTALLMESSAGE_INFO:
    case INSTALLMESSAGE_INITIALIZE:
    case INSTALLMESSAGE_TERMINATE:
    case INSTALLMESSAGE_INSTALLSTART:
    case INSTALLMESSAGE_INSTALLEND:
        return 0;
    case INSTALLMESSAGE_SHOWDIALOG:
    {
        LPWSTR dialog = msi_dup_record_field(record, 0);
        INT rc = ACTION_DialogBox(package, dialog);
        msi_free(dialog);
        return rc;
    }
    case INSTALLMESSAGE_ACTIONSTART:
    {
        LPWSTR deformatted;
        MSIRECORD *uirow = MSI_CreateRecord(1);
        if (!uirow) return -1;
        deformat_string(package, MSI_RecordGetString(record, 2), &deformatted);
        MSI_RecordSetStringW(uirow, 1, deformatted);
        msi_event_fire(package, szActionText, uirow);

        msi_free(deformatted);
        msiobj_release(&uirow->hdr);
        return 1;
    }
    case INSTALLMESSAGE_ACTIONDATA:
    {
        MSIRECORD *uirow = MSI_CreateRecord(1);
        if (!uirow) return -1;
        MSI_RecordSetStringW(uirow, 1, message);
        msi_event_fire(package, szActionData, uirow);
        msiobj_release(&uirow->hdr);

        if (package->action_progress_increment)
        {
            uirow = MSI_CreateRecord(2);
            if (!uirow) return -1;
            MSI_RecordSetInteger(uirow, 1, 2);
            MSI_RecordSetInteger(uirow, 2, package->action_progress_increment);
            msi_event_fire(package, szSetProgress, uirow);
            msiobj_release(&uirow->hdr);
        }
        return 1;
    }
    case INSTALLMESSAGE_PROGRESS:
        msi_event_fire(package, szSetProgress, record);
        return 1;
    case INSTALLMESSAGE_COMMONDATA:
        switch (MSI_RecordGetInteger(record, 1))
        {
        case 0:
        case 1:
            /* do nothing */
            return 0;
        default:
            /* fall through */
            ;
        }
    default:
        FIXME("internal UI not implemented for message 0x%08x (UI level = %x)\n", eMessageType, package->ui_level);
        return 0;
    }
}

static const WCHAR szActionNotFound[] = {'D','E','B','U','G',':',' ','E','r','r','o','r',' ','[','1',']',':',' ',' ','A','c','t','i','o','n',' ','n','o','t',' ','f','o','u','n','d',':',' ','[','2',']',0};

static const struct
{
    int id;
    const WCHAR *text;
}
internal_errors[] =
{
    {2726, szActionNotFound},
    {0}
};

static LPCWSTR get_internal_error_message(int error)
{
    int i = 0;

    while (internal_errors[i].id != 0)
    {
        if (internal_errors[i].id == error)
            return internal_errors[i].text;
        i++;
    }

    FIXME("missing error message %d\n", error);
    return NULL;
}

/* Returned string must be freed */
LPWSTR msi_get_error_message(MSIDATABASE *db, int error)
{
    static const WCHAR query[] =
        {'S','E','L','E','C','T',' ','`','M','e','s','s','a','g','e','`',' ',
         'F','R','O','M',' ','`','E','r','r','o','r','`',' ','W','H','E','R','E',' ',
         '`','E','r','r','o','r','`',' ','=',' ','%','i',0};
    MSIRECORD *record;
    LPWSTR ret = NULL;

    if ((record = MSI_QueryGetRecord(db, query, error)))
    {
        ret = msi_dup_record_field(record, 1);
        msiobj_release(&record->hdr);
    }
    else if (error < 2000)
    {
        int len = LoadStringW(msi_hInstance, IDS_ERROR_BASE + error, (LPWSTR) &ret, 0);
        if (len)
        {
            ret = msi_alloc((len + 1) * sizeof(WCHAR));
            LoadStringW(msi_hInstance, IDS_ERROR_BASE + error, ret, len + 1);
        }
        else
            ret = NULL;
    }

    return ret;
}

INT MSI_ProcessMessageVerbatim(MSIPACKAGE *package, INSTALLMESSAGE eMessageType, MSIRECORD *record)
{
    LPWSTR message = {0};
    DWORD len;
    DWORD log_type = 1 << (eMessageType >> 24);
    UINT res;
    INT rc = 0;
    char *msg;

    TRACE("%x\n", eMessageType);
    if (TRACE_ON(msi)) dump_record(record);

    if (!package || !record)
        message = NULL;
    else {
        res = MSI_FormatRecordW(package, record, message, &len);
        if (res != ERROR_SUCCESS && res != ERROR_MORE_DATA)
            return res;
        len++;
        message = msi_alloc(len * sizeof(WCHAR));
        if (!message) return ERROR_OUTOFMEMORY;
        MSI_FormatRecordW(package, record, message, &len);
    }

    /* convert it to ASCII */
    len = WideCharToMultiByte( CP_ACP, 0, message, -1, NULL, 0, NULL, NULL );
    msg = msi_alloc( len );
    WideCharToMultiByte( CP_ACP, 0, message, -1, msg, len, NULL, NULL );

    if (gUIHandlerRecord && (gUIFilterRecord & log_type))
    {
        MSIHANDLE rec = alloc_msihandle(&record->hdr);
        TRACE("Calling UI handler %p(pvContext=%p, iMessageType=%08x, hRecord=%u)\n",
              gUIHandlerRecord, gUIContextRecord, eMessageType, rec);
        rc = gUIHandlerRecord( gUIContextRecord, eMessageType, rec );
        MsiCloseHandle( rec );
    }
    if (!rc && gUIHandlerW && (gUIFilter & log_type))
    {
        TRACE("Calling UI handler %p(pvContext=%p, iMessageType=%08x, szMessage=%s)\n",
              gUIHandlerW, gUIContext, eMessageType, debugstr_w(message));
        rc = gUIHandlerW( gUIContext, eMessageType, message );
    }
    else if (!rc && gUIHandlerA && (gUIFilter & log_type))
    {
        TRACE("Calling UI handler %p(pvContext=%p, iMessageType=%08x, szMessage=%s)\n",
              gUIHandlerA, gUIContext, eMessageType, debugstr_a(msg));
        rc = gUIHandlerA( gUIContext, eMessageType, msg );
    }

    if (!rc)
        rc = internal_ui_handler(package, eMessageType, record, message);

    if (!rc && package && package->log_file != INVALID_HANDLE_VALUE &&
        (eMessageType & 0xff000000) != INSTALLMESSAGE_PROGRESS)
    {
        DWORD written;
        WriteFile( package->log_file, msg, len - 1, &written, NULL );
        WriteFile( package->log_file, "\n", 1, &written, NULL );
    }
    msi_free( msg );
    msi_free( message );

    return rc;
}

INT MSI_ProcessMessage( MSIPACKAGE *package, INSTALLMESSAGE eMessageType, MSIRECORD *record )
{
    switch (eMessageType & 0xff000000)
    {
    case INSTALLMESSAGE_FATALEXIT:
    case INSTALLMESSAGE_ERROR:
    case INSTALLMESSAGE_WARNING:
    case INSTALLMESSAGE_USER:
    case INSTALLMESSAGE_INFO:
    case INSTALLMESSAGE_OUTOFDISKSPACE:
        if (MSI_RecordGetInteger(record, 1) != MSI_NULL_INTEGER)
        {
            /* error message */

            LPWSTR template;
            LPWSTR template_rec = NULL, template_prefix = NULL;
            int error = MSI_RecordGetInteger(record, 1);

            if (MSI_RecordIsNull(record, 0))
            {
                if (error >= 32)
                {
                    template_rec = msi_get_error_message(package->db, error);

                    if (!template_rec && error >= 2000)
                    {
                        /* internal error, not localized */
                        if ((template_rec = (LPWSTR) get_internal_error_message(error)))
                        {
                            MSI_RecordSetStringW(record, 0, template_rec);
                            MSI_ProcessMessageVerbatim(package, INSTALLMESSAGE_INFO, record);
                        }
                        template_rec = msi_get_error_message(package->db, MSIERR_INSTALLERROR);
                        MSI_RecordSetStringW(record, 0, template_rec);
                        MSI_ProcessMessageVerbatim(package, eMessageType, record);
                        msi_free(template_rec);
                        return 0;
                    }
                }
            }
            else
                template_rec = msi_dup_record_field(record, 0);

            template_prefix = msi_get_error_message(package->db, eMessageType >> 24);
            if (!template_prefix) template_prefix = strdupW(szEmpty);

            if (!template_rec)
            {
                /* always returns 0 */
                MSI_RecordSetStringW(record, 0, template_prefix);
                MSI_ProcessMessageVerbatim(package, eMessageType, record);
                msi_free(template_prefix);
                return 0;
            }

            template = msi_alloc((strlenW(template_rec) + strlenW(template_prefix) + 1) * sizeof(WCHAR));
            if (!template) return ERROR_OUTOFMEMORY;

            strcpyW(template, template_prefix);
            strcatW(template, template_rec);
            MSI_RecordSetStringW(record, 0, template);

            msi_free(template_prefix);
            msi_free(template_rec);
            msi_free(template);
        }
        break;
    case INSTALLMESSAGE_ACTIONSTART:
    {
        WCHAR *template = msi_get_error_message(package->db, MSIERR_ACTIONSTART);
        MSI_RecordSetStringW(record, 0, template);
        msi_free(template);

        msi_free(package->LastAction);
        msi_free(package->LastActionTemplate);
        package->LastAction = msi_dup_record_field(record, 1);
        if (!package->LastAction) package->LastAction = strdupW(szEmpty);
        package->LastActionTemplate = msi_dup_record_field(record, 3);
        break;
    }
    case INSTALLMESSAGE_ACTIONDATA:
        if (package->LastAction && package->LastActionTemplate)
        {
            static const WCHAR template_s[] =
                {'{','{','%','s',':',' ','}','}','%','s',0};
            WCHAR *template;

            template = msi_alloc((strlenW(package->LastAction) + strlenW(package->LastActionTemplate) + 7) * sizeof(WCHAR));
            if (!template) return ERROR_OUTOFMEMORY;
            sprintfW(template, template_s, package->LastAction, package->LastActionTemplate);
            MSI_RecordSetStringW(record, 0, template);
            msi_free(template);
        }
        break;
    case INSTALLMESSAGE_COMMONDATA:
    {
        WCHAR *template = msi_get_error_message(package->db, MSIERR_COMMONDATA);
        MSI_RecordSetStringW(record, 0, template);
        msi_free(template);
    }
    break;
    }

    return MSI_ProcessMessageVerbatim(package, eMessageType, record);
}

INT WINAPI MsiProcessMessage( MSIHANDLE hInstall, INSTALLMESSAGE eMessageType,
                              MSIHANDLE hRecord)
{
    UINT ret = ERROR_INVALID_HANDLE;
    MSIPACKAGE *package = NULL;
    MSIRECORD *record = NULL;

    if ((eMessageType & 0xff000000) == INSTALLMESSAGE_INITIALIZE ||
        (eMessageType & 0xff000000) == INSTALLMESSAGE_TERMINATE)
        return -1;

    if ((eMessageType & 0xff000000) == INSTALLMESSAGE_COMMONDATA &&
        MsiRecordGetInteger(hRecord, 1) != 2)
        return -1;

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

void msi_reset_folders( MSIPACKAGE *package, BOOL source )
{
    MSIFOLDER *folder;

    LIST_FOR_EACH_ENTRY( folder, &package->folders, MSIFOLDER, entry )
    {
        if ( source )
        {
            msi_free( folder->ResolvedSource );
            folder->ResolvedSource = NULL;
        }
        else
        {
            msi_free( folder->ResolvedTarget );
            folder->ResolvedTarget = NULL;
        }
    }
}

UINT msi_set_property( MSIDATABASE *db, const WCHAR *name, const WCHAR *value, int len )
{
    static const WCHAR insert_query[] = {
        'I','N','S','E','R','T',' ','I','N','T','O',' ',
        '`','_','P','r','o','p','e','r','t','y','`',' ',
        '(','`','_','P','r','o','p','e','r','t','y','`',',','`','V','a','l','u','e','`',')',' ',
        'V','A','L','U','E','S',' ','(','?',',','?',')',0};
    static const WCHAR update_query[] = {
        'U','P','D','A','T','E',' ','`','_','P','r','o','p','e','r','t','y','`',' ',
        'S','E','T',' ','`','V','a','l','u','e','`',' ','=',' ','?',' ','W','H','E','R','E',' ',
        '`','_','P','r','o','p','e','r','t','y','`',' ','=',' ','\'','%','s','\'',0};
    static const WCHAR delete_query[] = {
        'D','E','L','E','T','E',' ','F','R','O','M',' ',
        '`','_','P','r','o','p','e','r','t','y','`',' ','W','H','E','R','E',' ',
        '`','_','P','r','o','p','e','r','t','y','`',' ','=',' ','\'','%','s','\'',0};
    MSIQUERY *view;
    MSIRECORD *row = NULL;
    DWORD sz = 0;
    WCHAR query[1024];
    UINT rc;

    TRACE("%p %s %s %d\n", db, debugstr_w(name), debugstr_wn(value, len), len);

    if (!name)
        return ERROR_INVALID_PARAMETER;

    /* this one is weird... */
    if (!name[0])
        return value ? ERROR_FUNCTION_FAILED : ERROR_SUCCESS;

    if (value && len < 0) len = strlenW( value );

    rc = msi_get_property( db, name, 0, &sz );
    if (!value || (!*value && !len))
    {
        sprintfW( query, delete_query, name );
    }
    else if (rc == ERROR_MORE_DATA || rc == ERROR_SUCCESS)
    {
        sprintfW( query, update_query, name );
        row = MSI_CreateRecord(1);
        msi_record_set_string( row, 1, value, len );
    }
    else
    {
        strcpyW( query, insert_query );
        row = MSI_CreateRecord(2);
        msi_record_set_string( row, 1, name, -1 );
        msi_record_set_string( row, 2, value, len );
    }

    rc = MSI_DatabaseOpenViewW(db, query, &view);
    if (rc == ERROR_SUCCESS)
    {
        rc = MSI_ViewExecute(view, row);
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
    }
    if (row) msiobj_release(&row->hdr);
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

    ret = msi_set_property( package->db, szName, szValue, -1 );
    if (ret == ERROR_SUCCESS && !strcmpW( szName, szSourceDir ))
        msi_reset_folders( package, TRUE );

    msiobj_release( &package->hdr );
    return ret;
}

static MSIRECORD *msi_get_property_row( MSIDATABASE *db, LPCWSTR name )
{
    static const WCHAR query[]= {
        'S','E','L','E','C','T',' ','`','V','a','l','u','e','`',' ',
        'F','R','O','M',' ' ,'`','_','P','r','o','p','e','r','t','y','`',' ',
        'W','H','E','R','E',' ' ,'`','_','P','r','o','p','e','r','t','y','`','=','?',0};
    MSIRECORD *rec, *row = NULL;
    MSIQUERY *view;
    UINT r;

    static const WCHAR szDate[] = {'D','a','t','e',0};
    static const WCHAR szTime[] = {'T','i','m','e',0};
    WCHAR *buffer;
    int length;

    if (!name || !*name)
        return NULL;

    if (!strcmpW(name, szDate))
    {
        length = GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, NULL, NULL, NULL, 0);
        if (!length)
            return NULL;
        buffer = msi_alloc(length * sizeof(WCHAR));
        GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, NULL, NULL, buffer, length);

        row = MSI_CreateRecord(1);
        if (!row)
        {
            msi_free(buffer);
            return NULL;
        }
        MSI_RecordSetStringW(row, 1, buffer);
        msi_free(buffer);
        return row;
    }
    else if (!strcmpW(name, szTime))
    {
        length = GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOTIMEMARKER, NULL, NULL, NULL, 0);
        if (!length)
            return NULL;
        buffer = msi_alloc(length * sizeof(WCHAR));
        GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOTIMEMARKER, NULL, NULL, buffer, length);

        row = MSI_CreateRecord(1);
        if (!row)
        {
            msi_free(buffer);
            return NULL;
        }
        MSI_RecordSetStringW(row, 1, buffer);
        msi_free(buffer);
        return row;
    }

    rec = MSI_CreateRecord(1);
    if (!rec)
        return NULL;

    MSI_RecordSetStringW(rec, 1, name);

    r = MSI_DatabaseOpenViewW(db, query, &view);
    if (r == ERROR_SUCCESS)
    {
        MSI_ViewExecute(view, rec);
        MSI_ViewFetch(view, &row);
        MSI_ViewClose(view);
        msiobj_release(&view->hdr);
    }
    msiobj_release(&rec->hdr);
    return row;
}

/* internal function, not compatible with MsiGetPropertyW */
UINT msi_get_property( MSIDATABASE *db, LPCWSTR szName,
                       LPWSTR szValueBuf, LPDWORD pchValueBuf )
{
    MSIRECORD *row;
    UINT rc = ERROR_FUNCTION_FAILED;

    TRACE("%p %s %p %p\n", db, debugstr_w(szName), szValueBuf, pchValueBuf);

    row = msi_get_property_row( db, szName );

    if (*pchValueBuf > 0)
        szValueBuf[0] = 0;

    if (row)
    {
        rc = MSI_RecordGetStringW(row, 1, szValueBuf, pchValueBuf);
        msiobj_release(&row->hdr);
    }

    if (rc == ERROR_SUCCESS)
        TRACE("returning %s for property %s\n", debugstr_wn(szValueBuf, *pchValueBuf),
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

LPWSTR msi_dup_property(MSIDATABASE *db, LPCWSTR prop)
{
    DWORD sz = 0;
    LPWSTR str;
    UINT r;

    r = msi_get_property(db, prop, NULL, &sz);
    if (r != ERROR_SUCCESS && r != ERROR_MORE_DATA)
        return NULL;

    sz++;
    str = msi_alloc(sz * sizeof(WCHAR));
    r = msi_get_property(db, prop, str, &sz);
    if (r != ERROR_SUCCESS)
    {
        msi_free(str);
        str = NULL;
    }

    return str;
}

int msi_get_property_int( MSIDATABASE *db, LPCWSTR prop, int def )
{
    LPWSTR str = msi_dup_property( db, prop );
    int val = str ? atoiW(str) : def;
    msi_free(str);
    return val;
}

static UINT MSI_GetProperty( MSIHANDLE handle, LPCWSTR name,
                             awstring *szValueBuf, LPDWORD pchValueBuf )
{
    MSIPACKAGE *package;
    MSIRECORD *row = NULL;
    UINT r = ERROR_FUNCTION_FAILED;
    LPCWSTR val = NULL;
    DWORD len = 0;

    TRACE("%u %s %p %p\n", handle, debugstr_w(name),
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

        remote_package = (IWineMsiRemotePackage *)msi_get_remote( handle );
        if (!remote_package)
            return ERROR_INVALID_HANDLE;

        bname = SysAllocString( name );
        if (!bname)
        {
            IWineMsiRemotePackage_Release( remote_package );
            return ERROR_OUTOFMEMORY;
        }

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

        hr = IWineMsiRemotePackage_GetProperty( remote_package, bname, value, &len );
        if (FAILED(hr))
            goto done;

        r = msi_strcpy_to_awstring( value, len, szValueBuf, pchValueBuf );

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

    row = msi_get_property_row( package->db, name );
    if (row)
        val = msi_record_get_string( row, 1, (int *)&len );

    if (!val)
        val = szEmpty;

    r = msi_strcpy_to_awstring( val, len, szValueBuf, pchValueBuf );

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
    IWineMsiRemotePackage IWineMsiRemotePackage_iface;
    MSIHANDLE package;
    LONG refs;
} msi_remote_package_impl;

static inline msi_remote_package_impl *impl_from_IWineMsiRemotePackage( IWineMsiRemotePackage *iface )
{
    return CONTAINING_RECORD(iface, msi_remote_package_impl, IWineMsiRemotePackage_iface);
}

static HRESULT WINAPI mrp_QueryInterface( IWineMsiRemotePackage *iface,
                REFIID riid,LPVOID *ppobj)
{
    if( IsEqualCLSID( riid, &IID_IUnknown ) ||
        IsEqualCLSID( riid, &IID_IWineMsiRemotePackage ) )
    {
        IWineMsiRemotePackage_AddRef( iface );
        *ppobj = iface;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI mrp_AddRef( IWineMsiRemotePackage *iface )
{
    msi_remote_package_impl* This = impl_from_IWineMsiRemotePackage( iface );

    return InterlockedIncrement( &This->refs );
}

static ULONG WINAPI mrp_Release( IWineMsiRemotePackage *iface )
{
    msi_remote_package_impl* This = impl_from_IWineMsiRemotePackage( iface );
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
    msi_remote_package_impl* This = impl_from_IWineMsiRemotePackage( iface );
    This->package = handle;
    return S_OK;
}

static HRESULT WINAPI mrp_GetActiveDatabase( IWineMsiRemotePackage *iface, MSIHANDLE *handle )
{
    msi_remote_package_impl* This = impl_from_IWineMsiRemotePackage( iface );
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

static HRESULT WINAPI mrp_GetProperty( IWineMsiRemotePackage *iface, BSTR property, BSTR value, DWORD *size )
{
    msi_remote_package_impl* This = impl_from_IWineMsiRemotePackage( iface );
    UINT r = MsiGetPropertyW(This->package, property, value, size);
    if (r != ERROR_SUCCESS) return HRESULT_FROM_WIN32(r);
    return S_OK;
}

static HRESULT WINAPI mrp_SetProperty( IWineMsiRemotePackage *iface, BSTR property, BSTR value )
{
    msi_remote_package_impl* This = impl_from_IWineMsiRemotePackage( iface );
    UINT r = MsiSetPropertyW(This->package, property, value);
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrp_ProcessMessage( IWineMsiRemotePackage *iface, INSTALLMESSAGE message, MSIHANDLE record )
{
    msi_remote_package_impl* This = impl_from_IWineMsiRemotePackage( iface );
    UINT r = MsiProcessMessage(This->package, message, record);
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrp_DoAction( IWineMsiRemotePackage *iface, BSTR action )
{
    msi_remote_package_impl* This = impl_from_IWineMsiRemotePackage( iface );
    UINT r = MsiDoActionW(This->package, action);
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrp_Sequence( IWineMsiRemotePackage *iface, BSTR table, int sequence )
{
    msi_remote_package_impl* This = impl_from_IWineMsiRemotePackage( iface );
    UINT r = MsiSequenceW(This->package, table, sequence);
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrp_GetTargetPath( IWineMsiRemotePackage *iface, BSTR folder, BSTR value, DWORD *size )
{
    msi_remote_package_impl* This = impl_from_IWineMsiRemotePackage( iface );
    UINT r = MsiGetTargetPathW(This->package, folder, value, size);
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrp_SetTargetPath( IWineMsiRemotePackage *iface, BSTR folder, BSTR value)
{
    msi_remote_package_impl* This = impl_from_IWineMsiRemotePackage( iface );
    UINT r = MsiSetTargetPathW(This->package, folder, value);
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrp_GetSourcePath( IWineMsiRemotePackage *iface, BSTR folder, BSTR value, DWORD *size )
{
    msi_remote_package_impl* This = impl_from_IWineMsiRemotePackage( iface );
    UINT r = MsiGetSourcePathW(This->package, folder, value, size);
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrp_GetMode( IWineMsiRemotePackage *iface, MSIRUNMODE mode, BOOL *ret )
{
    msi_remote_package_impl* This = impl_from_IWineMsiRemotePackage( iface );
    *ret = MsiGetMode(This->package, mode);
    return S_OK;
}

static HRESULT WINAPI mrp_SetMode( IWineMsiRemotePackage *iface, MSIRUNMODE mode, BOOL state )
{
    msi_remote_package_impl* This = impl_from_IWineMsiRemotePackage( iface );
    UINT r = MsiSetMode(This->package, mode, state);
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrp_GetFeatureState( IWineMsiRemotePackage *iface, BSTR feature,
                                    INSTALLSTATE *installed, INSTALLSTATE *action )
{
    msi_remote_package_impl* This = impl_from_IWineMsiRemotePackage( iface );
    UINT r = MsiGetFeatureStateW(This->package, feature, installed, action);
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrp_SetFeatureState( IWineMsiRemotePackage *iface, BSTR feature, INSTALLSTATE state )
{
    msi_remote_package_impl* This = impl_from_IWineMsiRemotePackage( iface );
    UINT r = MsiSetFeatureStateW(This->package, feature, state);
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrp_GetComponentState( IWineMsiRemotePackage *iface, BSTR component,
                                      INSTALLSTATE *installed, INSTALLSTATE *action )
{
    msi_remote_package_impl* This = impl_from_IWineMsiRemotePackage( iface );
    UINT r = MsiGetComponentStateW(This->package, component, installed, action);
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrp_SetComponentState( IWineMsiRemotePackage *iface, BSTR component, INSTALLSTATE state )
{
    msi_remote_package_impl* This = impl_from_IWineMsiRemotePackage( iface );
    UINT r = MsiSetComponentStateW(This->package, component, state);
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrp_GetLanguage( IWineMsiRemotePackage *iface, LANGID *language )
{
    msi_remote_package_impl* This = impl_from_IWineMsiRemotePackage( iface );
    *language = MsiGetLanguage(This->package);
    return S_OK;
}

static HRESULT WINAPI mrp_SetInstallLevel( IWineMsiRemotePackage *iface, int level )
{
    msi_remote_package_impl* This = impl_from_IWineMsiRemotePackage( iface );
    UINT r = MsiSetInstallLevel(This->package, level);
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrp_FormatRecord( IWineMsiRemotePackage *iface, MSIHANDLE record,
                                        BSTR *value)
{
    DWORD size = 0;
    msi_remote_package_impl* This = impl_from_IWineMsiRemotePackage( iface );
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
    msi_remote_package_impl* This = impl_from_IWineMsiRemotePackage( iface );
    UINT r = MsiEvaluateConditionW(This->package, condition);
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrp_GetFeatureCost( IWineMsiRemotePackage *iface, BSTR feature,
                                          INT cost_tree, INSTALLSTATE state, INT *cost )
{
    msi_remote_package_impl* This = impl_from_IWineMsiRemotePackage( iface );
    UINT r = MsiGetFeatureCostW(This->package, feature, cost_tree, state, cost);
    return HRESULT_FROM_WIN32(r);
}

static HRESULT WINAPI mrp_EnumComponentCosts( IWineMsiRemotePackage *iface, BSTR component,
                                              DWORD index, INSTALLSTATE state, BSTR drive,
                                              DWORD *buflen, INT *cost, INT *temp )
{
    msi_remote_package_impl* This = impl_from_IWineMsiRemotePackage( iface );
    UINT r = MsiEnumComponentCostsW(This->package, component, index, state, drive, buflen, cost, temp);
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
    mrp_SetMode,
    mrp_GetFeatureState,
    mrp_SetFeatureState,
    mrp_GetComponentState,
    mrp_SetComponentState,
    mrp_GetLanguage,
    mrp_SetInstallLevel,
    mrp_FormatRecord,
    mrp_EvaluateCondition,
    mrp_GetFeatureCost,
    mrp_EnumComponentCosts
};

HRESULT create_msi_remote_package( IUnknown *pOuter, LPVOID *ppObj )
{
    msi_remote_package_impl* This;

    This = msi_alloc( sizeof *This );
    if (!This)
        return E_OUTOFMEMORY;

    This->IWineMsiRemotePackage_iface.lpVtbl = &msi_remote_package_vtbl;
    This->package = 0;
    This->refs = 1;

    *ppObj = &This->IWineMsiRemotePackage_iface;

    return S_OK;
}

UINT msi_package_add_info(MSIPACKAGE *package, DWORD context, DWORD options,
                          LPCWSTR property, LPWSTR value)
{
    MSISOURCELISTINFO *info;

    LIST_FOR_EACH_ENTRY( info, &package->sourcelist_info, MSISOURCELISTINFO, entry )
    {
        if (!strcmpW( info->value, value )) return ERROR_SUCCESS;
    }

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

    LIST_FOR_EACH_ENTRY( disk, &package->sourcelist_media, MSIMEDIADISK, entry )
    {
        if (disk->disk_id == disk_id) return ERROR_SUCCESS;
    }

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
