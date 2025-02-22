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

#define COBJMACROS

#ifdef __REACTOS__
#define WIN32_NO_STATUS
#endif

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winnls.h"
#ifdef __REACTOS__
#include <ndk/rtlfuncs.h>
#else
#include "winternl.h"
#endif
#include "shlwapi.h"
#include "wingdi.h"
#include "msi.h"
#include "msiquery.h"
#include "objidl.h"
#include "wincrypt.h"
#include "winuser.h"
#include "wininet.h"
#include "winver.h"
#include "urlmon.h"
#include "shlobj.h"
#include "objbase.h"
#include "msidefs.h"
#include "sddl.h"

#include "wine/debug.h"
#include "wine/exception.h"

#include "msipriv.h"
#include "winemsi_s.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

static void free_feature( MSIFEATURE *feature )
{
    struct list *item, *cursor;

    LIST_FOR_EACH_SAFE( item, cursor, &feature->Children )
    {
        FeatureList *fl = LIST_ENTRY( item, FeatureList, entry );
        list_remove( &fl->entry );
        free( fl );
    }

    LIST_FOR_EACH_SAFE( item, cursor, &feature->Components )
    {
        ComponentList *cl = LIST_ENTRY( item, ComponentList, entry );
        list_remove( &cl->entry );
        free( cl );
    }
    free( feature->Feature );
    free( feature->Feature_Parent );
    free( feature->Directory );
    free( feature->Description );
    free( feature->Title );
    free( feature );
}

static void free_folder( MSIFOLDER *folder )
{
    struct list *item, *cursor;

    LIST_FOR_EACH_SAFE( item, cursor, &folder->children )
    {
        FolderList *fl = LIST_ENTRY( item, FolderList, entry );
        list_remove( &fl->entry );
        free( fl );
    }
    free( folder->Parent );
    free( folder->Directory );
    free( folder->TargetDefault );
    free( folder->SourceLongPath );
    free( folder->SourceShortPath );
    free( folder->ResolvedTarget );
    free( folder->ResolvedSource );
    free( folder );
}

static void free_extension( MSIEXTENSION *ext )
{
    struct list *item, *cursor;

    LIST_FOR_EACH_SAFE( item, cursor, &ext->verbs )
    {
        MSIVERB *verb = LIST_ENTRY( item, MSIVERB, entry );

        list_remove( &verb->entry );
        free( verb->Verb );
        free( verb->Command );
        free( verb->Argument );
        free( verb );
    }

    free( ext->Extension );
    free( ext->ProgIDText );
    free( ext );
}

static void free_assembly( MSIASSEMBLY *assembly )
{
    free( assembly->feature );
    free( assembly->manifest );
    free( assembly->application );
    free( assembly->display_name );
    if (assembly->tempdir) RemoveDirectoryW( assembly->tempdir );
    free( assembly->tempdir );
    free( assembly );
}

void msi_free_action_script( MSIPACKAGE *package, UINT script )
{
    UINT i;
    for (i = 0; i < package->script_actions_count[script]; i++)
        free( package->script_actions[script][i] );

    free( package->script_actions[script] );
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
        free( file->File );
        free( file->FileName );
        free( file->ShortName );
        free( file->LongName );
        free( file->Version );
        free( file->Language );
        if (msi_is_global_assembly( file->Component )) DeleteFileW( file->TargetPath );
        free( file->TargetPath );
        free( file );
    }

    LIST_FOR_EACH_SAFE( item, cursor, &package->components )
    {
        MSICOMPONENT *comp = LIST_ENTRY( item, MSICOMPONENT, entry );

        list_remove( &comp->entry );
        free( comp->Component );
        free( comp->ComponentId );
        free( comp->Directory );
        free( comp->Condition );
        free( comp->KeyPath );
        free( comp->FullKeypath );
        if (comp->assembly) free_assembly( comp->assembly );
        free( comp );
    }

    LIST_FOR_EACH_SAFE( item, cursor, &package->filepatches )
    {
        MSIFILEPATCH *patch = LIST_ENTRY( item, MSIFILEPATCH, entry );

        list_remove( &patch->entry );
        free( patch->path );
        free( patch );
    }

    /* clean up extension, progid, class and verb structures */
    LIST_FOR_EACH_SAFE( item, cursor, &package->classes )
    {
        MSICLASS *cls = LIST_ENTRY( item, MSICLASS, entry );

        list_remove( &cls->entry );
        free( cls->clsid );
        free( cls->Context );
        free( cls->Description );
        free( cls->FileTypeMask );
        free( cls->IconPath );
        free( cls->DefInprocHandler );
        free( cls->DefInprocHandler32 );
        free( cls->Argument );
        free( cls->ProgIDText );
        free( cls );
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
        free( progid->ProgID );
        free( progid->Description );
        free( progid->IconPath );
        free( progid );
    }

    LIST_FOR_EACH_SAFE( item, cursor, &package->mimes )
    {
        MSIMIME *mt = LIST_ENTRY( item, MSIMIME, entry );

        list_remove( &mt->entry );
        free( mt->suffix );
        free( mt->clsid );
        free( mt->ContentType );
        free( mt );
    }

    LIST_FOR_EACH_SAFE( item, cursor, &package->appids )
    {
        MSIAPPID *appid = LIST_ENTRY( item, MSIAPPID, entry );

        list_remove( &appid->entry );
        free( appid->AppID );
        free( appid->RemoteServerName );
        free( appid->LocalServer );
        free( appid->ServiceParameters );
        free( appid->DllSurrogate );
        free( appid );
    }

    LIST_FOR_EACH_SAFE( item, cursor, &package->sourcelist_info )
    {
        MSISOURCELISTINFO *info = LIST_ENTRY( item, MSISOURCELISTINFO, entry );

        list_remove( &info->entry );
        free( info->value );
        free( info );
    }

    LIST_FOR_EACH_SAFE( item, cursor, &package->sourcelist_media )
    {
        MSIMEDIADISK *info = LIST_ENTRY( item, MSIMEDIADISK, entry );

        list_remove( &info->entry );
        free( info->volume_label );
        free( info->disk_prompt );
        free( info );
    }

    for (i = 0; i < SCRIPT_MAX; i++)
        msi_free_action_script( package, i );

    for (i = 0; i < package->unique_actions_count; i++)
        free( package->unique_actions[i] );
    free( package->unique_actions );

    LIST_FOR_EACH_SAFE( item, cursor, &package->binaries )
    {
        MSIBINARY *binary = LIST_ENTRY( item, MSIBINARY, entry );

        list_remove( &binary->entry );
        if (!DeleteFileW( binary->tmpfile ))
            ERR( "failed to delete %s (%lu)\n", debugstr_w(binary->tmpfile), GetLastError() );
        free( binary->source );
        free( binary->tmpfile );
        free( binary );
    }

    LIST_FOR_EACH_SAFE( item, cursor, &package->cabinet_streams )
    {
        MSICABINETSTREAM *cab = LIST_ENTRY( item, MSICABINETSTREAM, entry );

        list_remove( &cab->entry );
        IStorage_Release( cab->storage );
        free( cab->stream );
        free( cab );
    }

    LIST_FOR_EACH_SAFE( item, cursor, &package->patches )
    {
        MSIPATCHINFO *patch = LIST_ENTRY( item, MSIPATCHINFO, entry );

        list_remove( &patch->entry );
        if (patch->delete_on_close && !DeleteFileW( patch->localfile ))
        {
            ERR( "failed to delete %s (%lu)\n", debugstr_w(patch->localfile), GetLastError() );
        }
        msi_free_patchinfo( patch );
    }

    free( package->PackagePath );
    free( package->ProductCode );
    free( package->ActionFormat );
    free( package->LastAction );
    free( package->LastActionTemplate );
    free( package->langids );

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
    if (package->rpc_server_started)
        RpcServerUnregisterIf(s_IWineMsiRemote_v0_0_s_ifspec, NULL, FALSE);
    if (rpc_handle)
        RpcBindingFree(&rpc_handle);
    if (package->custom_server_32_process)
        custom_stop_server(package->custom_server_32_process, package->custom_server_32_pipe);
    if (package->custom_server_64_process)
        custom_stop_server(package->custom_server_64_process, package->custom_server_64_pipe);

    if (package->delete_on_close) DeleteFileW( package->localfile );
    free( package->localfile );
    MSI_ProcessMessage(NULL, INSTALLMESSAGE_TERMINATE, 0);
}

static UINT create_temp_property_table(MSIPACKAGE *package)
{
    MSIQUERY *view;
    UINT rc;

    rc = MSI_DatabaseOpenViewW(package->db, L"CREATE TABLE `_Property` ( `_Property` CHAR(56) NOT NULL TEMPORARY, "
                                            L"`Value` CHAR(98) NOT NULL TEMPORARY PRIMARY KEY `_Property`) HOLD", &view);
    if (rc != ERROR_SUCCESS)
        return rc;

    rc = MSI_ViewExecute(view, 0);
    MSI_ViewClose(view);
    msiobj_release(&view->hdr);
    return rc;
}

UINT msi_clone_properties( MSIDATABASE *db )
{
    MSIQUERY *view_select;
    UINT rc;

    rc = MSI_DatabaseOpenViewW( db, L"SELECT * FROM `Property`", &view_select );
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

        rc = MSI_DatabaseOpenViewW( db, L"INSERT INTO `_Property` (`_Property`,`Value`) VALUES (?,?)", &view_insert );
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

            rc = MSI_DatabaseOpenViewW( db, L"UPDATE `_Property` SET `Value` = ? WHERE `_Property` = ?", &view_update );
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
        msi_set_property( package->db, L"Installed", L"1", -1 );
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

    user_name = malloc( (size + 1) * sizeof(WCHAR) );
    if (!user_name)
        return ERROR_OUTOFMEMORY;

    if (!GetUserNameW( user_name, &size ))
        goto done;

    size = 0;
    dom_size = 0;
    LookupAccountNameW( NULL, user_name, NULL, &size, NULL, &dom_size, &use );

    psid = malloc( size );
    dom = malloc( dom_size * sizeof (WCHAR) );
    if (!psid || !dom)
    {
        r = ERROR_OUTOFMEMORY;
        goto done;
    }

    if (!LookupAccountNameW( NULL, user_name, psid, &size, dom, &dom_size, &use ))
        goto done;

    if (!ConvertSidToStringSidW( psid, &sid_str ))
        goto done;

    r = msi_set_property( package->db, L"UserSID", sid_str, -1 );

done:
    LocalFree( sid_str );
    free( dom );
    free( psid );
    free( user_name );

    return r;
}

static LPWSTR get_fusion_filename(MSIPACKAGE *package)
{
    HKEY netsetup, hkey;
    LONG res;
    DWORD size, len, type;
    WCHAR windir[MAX_PATH], path[MAX_PATH], *filename = NULL;

    res = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\NET Framework Setup\\NDP", 0, KEY_CREATE_SUB_KEY,
                        &netsetup);
    if (res != ERROR_SUCCESS)
        return NULL;

    if (!RegCreateKeyExW(netsetup, L"v4\\Client", 0, NULL, 0, KEY_QUERY_VALUE, NULL, &hkey, NULL))
    {
        size = sizeof(path);
        if (!RegQueryValueExW(hkey, L"InstallPath", NULL, &type, (BYTE *)path, &size))
        {
            len = lstrlenW(path) + lstrlenW(L"fusion.dll") + 2;
            if (!(filename = malloc(len * sizeof(WCHAR)))) return NULL;

            lstrcpyW(filename, path);
            lstrcatW(filename, L"\\");
            lstrcatW(filename, L"fusion.dll");
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

    if (!RegCreateKeyExW(netsetup, L"v2.0.50727", 0, NULL, 0, KEY_QUERY_VALUE, NULL, &hkey, NULL))
    {
        RegCloseKey(hkey);
        GetWindowsDirectoryW(windir, MAX_PATH);
        len = lstrlenW(windir) + lstrlenW(L"Microsoft.NET\\Framework\\") + lstrlenW(L"v2.0.50727") +
              lstrlenW(L"fusion.dll") + 3;
        free(filename);
        if (!(filename = malloc(len * sizeof(WCHAR)))) return NULL;

        lstrcpyW(filename, windir);
        lstrcatW(filename, L"\\");
        lstrcatW(filename, L"Microsoft.NET\\Framework\\");
        lstrcatW(filename, L"v2.0.50727");
        lstrcatW(filename, L"\\");
        lstrcatW(filename, L"fusion.dll");
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

struct lang_codepage
{
  WORD wLanguage;
  WORD wCodePage;
};

static void set_msi_assembly_prop(MSIPACKAGE *package)
{
    UINT val_len;
    DWORD size, handle;
    LPVOID version = NULL;
    WCHAR buf[MAX_PATH];
    LPWSTR fusion, verstr;
    struct lang_codepage *translate;

    fusion = get_fusion_filename(package);
    if (!fusion)
        return;

    size = GetFileVersionInfoSizeW(fusion, &handle);
    if (!size)
        goto done;

    version = malloc(size);
    if (!version)
        goto done;

    if (!GetFileVersionInfoW(fusion, handle, size, version))
        goto done;

    if (!VerQueryValueW(version, L"\\VarFileInfo\\Translation", (LPVOID *)&translate, &val_len))
        goto done;

    swprintf(buf, ARRAY_SIZE(buf), L"\\StringFileInfo\\%04x%04x\\ProductVersion", translate[0].wLanguage,
             translate[0].wCodePage);

    if (!VerQueryValueW(version, buf, (LPVOID *)&verstr, &val_len))
        goto done;

    if (!val_len || !verstr)
        goto done;

    msi_set_property( package->db, L"MsiNetAssemblySupport", verstr, -1 );

done:
    free(fusion);
    free(version);
}

static VOID set_installer_properties(MSIPACKAGE *package)
{
    WCHAR *ptr;
    RTL_OSVERSIONINFOEXW OSVersion;
    MEMORYSTATUSEX msex;
    DWORD verval, len, type;
    WCHAR pth[MAX_PATH], verstr[11], bufstr[22];
    HDC dc;
    HKEY hkey;
    LPWSTR username, companyname;
    SYSTEM_INFO sys_info;
    LANGID langid;

    /*
     * Other things that probably should be set:
     *
     * VirtualMemory ShellAdvSupport DefaultUIFont PackagecodeChanging
     * CaptionHeight BorderTop BorderSide TextHeight RedirectedDllSupport
     */

    SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, 0, pth);
    lstrcatW(pth, L"\\");
    msi_set_property( package->db, L"CommonAppDataFolder", pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_FAVORITES, NULL, 0, pth);
    lstrcatW(pth, L"\\");
    msi_set_property( package->db, L"FavoritesFolder", pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_FONTS, NULL, 0, pth);
    lstrcatW(pth, L"\\");
    msi_set_property( package->db, L"FontsFolder", pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_SENDTO, NULL, 0, pth);
    lstrcatW(pth, L"\\");
    msi_set_property( package->db, L"SendToFolder", pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_STARTMENU, NULL, 0, pth);
    lstrcatW(pth, L"\\");
    msi_set_property( package->db, L"StartMenuFolder", pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_STARTUP, NULL, 0, pth);
    lstrcatW(pth, L"\\");
    msi_set_property( package->db, L"StartupFolder", pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_TEMPLATES, NULL, 0, pth);
    lstrcatW(pth, L"\\");
    msi_set_property( package->db, L"TemplateFolder", pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_DESKTOP, NULL, 0, pth);
    lstrcatW(pth, L"\\");
    msi_set_property( package->db, L"DesktopFolder", pth, -1 );

    /* FIXME: set to AllUsers profile path if ALLUSERS is set */
    SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, 0, pth);
    lstrcatW(pth, L"\\");
    msi_set_property( package->db, L"ProgramMenuFolder", pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_ADMINTOOLS, NULL, 0, pth);
    lstrcatW(pth, L"\\");
    msi_set_property( package->db, L"AdminToolsFolder", pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, pth);
    lstrcatW(pth, L"\\");
    msi_set_property( package->db, L"AppDataFolder", pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_SYSTEM, NULL, 0, pth);
    lstrcatW(pth, L"\\");
    msi_set_property( package->db, L"SystemFolder", pth, -1 );
    msi_set_property( package->db, L"System16Folder", pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, pth);
    lstrcatW(pth, L"\\");
    msi_set_property( package->db, L"LocalAppDataFolder", pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_MYPICTURES, NULL, 0, pth);
    lstrcatW(pth, L"\\");
    msi_set_property( package->db, L"MyPicturesFolder", pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, 0, pth);
    lstrcatW(pth, L"\\");
    msi_set_property( package->db, L"PersonalFolder", pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_WINDOWS, NULL, 0, pth);
    lstrcatW(pth, L"\\");
    msi_set_property( package->db, L"WindowsFolder", pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_PRINTHOOD, NULL, 0, pth);
    lstrcatW(pth, L"\\");
    msi_set_property( package->db, L"PrintHoodFolder", pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_NETHOOD, NULL, 0, pth);
    lstrcatW(pth, L"\\");
    msi_set_property( package->db, L"NetHoodFolder", pth, -1 );

    SHGetFolderPathW(NULL, CSIDL_RECENT, NULL, 0, pth);
    lstrcatW(pth, L"\\");
    msi_set_property( package->db, L"RecentFolder", pth, -1 );

    /* Physical Memory is specified in MB. Using total amount. */
    msex.dwLength = sizeof(msex);
    GlobalMemoryStatusEx( &msex );
    len = swprintf( bufstr, ARRAY_SIZE(bufstr), L"%d", (int)(msex.ullTotalPhys / 1024 / 1024) );
    msi_set_property( package->db, L"PhysicalMemory", bufstr, len );

    SHGetFolderPathW(NULL, CSIDL_WINDOWS, NULL, 0, pth);
    ptr = wcschr(pth,'\\');
    if (ptr) *(ptr + 1) = 0;
    msi_set_property( package->db, L"WindowsVolume", pth, -1 );

    len = GetTempPathW(MAX_PATH, pth);
    msi_set_property( package->db, L"TempFolder", pth, len );

    /* in a wine environment the user is always admin and privileged */
    msi_set_property( package->db, L"AdminUser", L"1", -1 );
    msi_set_property( package->db, L"Privileged", L"1", -1 );
    msi_set_property( package->db, L"MsiRunningElevated", L"1", -1 );

    /* set the os things */
    OSVersion.dwOSVersionInfoSize = sizeof(OSVersion);
    RtlGetVersion((PRTL_OSVERSIONINFOW)&OSVersion);
    verval = OSVersion.dwMinorVersion + OSVersion.dwMajorVersion * 100;
    if (verval > 603)
    {
        verval = 603;
        OSVersion.dwBuildNumber = 9600;
    }
    len = swprintf( verstr, ARRAY_SIZE(verstr), L"%u", verval );
    switch (OSVersion.dwPlatformId)
    {
        case VER_PLATFORM_WIN32_WINDOWS:
            msi_set_property( package->db, L"Version9X", verstr, len );
            break;
        case VER_PLATFORM_WIN32_NT:
            msi_set_property( package->db, L"VersionNT", verstr, len );
            len = swprintf( bufstr, ARRAY_SIZE(bufstr), L"%u", OSVersion.wProductType );
            msi_set_property( package->db, L"MsiNTProductType", bufstr, len );
            break;
    }
    len = swprintf( bufstr, ARRAY_SIZE(bufstr), L"%u", OSVersion.dwBuildNumber );
    msi_set_property( package->db, L"WindowsBuild", bufstr, len );
    len = swprintf( bufstr, ARRAY_SIZE(bufstr), L"%u", OSVersion.wServicePackMajor );
    msi_set_property( package->db, L"ServicePackLevel", bufstr, len );

    len = swprintf( bufstr, ARRAY_SIZE(bufstr), L"%u.%u", MSI_MAJORVERSION, MSI_MINORVERSION );
    msi_set_property( package->db, L"VersionMsi", bufstr, len );
    len = swprintf( bufstr, ARRAY_SIZE(bufstr), L"%u", MSI_MAJORVERSION * 100 );
    msi_set_property( package->db, L"VersionDatabase", bufstr, len );

    RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion", 0,
        KEY_QUERY_VALUE | KEY_WOW64_64KEY, &hkey);

    GetNativeSystemInfo( &sys_info );
    len = swprintf( bufstr, ARRAY_SIZE(bufstr), L"%d", sys_info.wProcessorLevel );
    msi_set_property( package->db, L"Intel", bufstr, len );
    if (sys_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
    {
        GetSystemDirectoryW( pth, MAX_PATH );
        PathAddBackslashW( pth );
        msi_set_property( package->db, L"SystemFolder", pth, -1 );

        len = sizeof(pth);
        RegQueryValueExW(hkey, L"ProgramFilesDir", 0, &type, (BYTE *)pth, &len);
        PathAddBackslashW( pth );
        msi_set_property( package->db, L"ProgramFilesFolder", pth, -1 );

        len = sizeof(pth);
        RegQueryValueExW(hkey, L"CommonFilesDir", 0, &type, (BYTE *)pth, &len);
        PathAddBackslashW( pth );
        msi_set_property( package->db, L"CommonFilesFolder", pth, -1 );
    }
    else if (sys_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
    {
        msi_set_property( package->db, L"MsiAMD64", bufstr, -1 );
        msi_set_property( package->db, L"Msix64", bufstr, -1 );
        msi_set_property( package->db, L"VersionNT64", verstr, -1 );

        GetSystemDirectoryW( pth, MAX_PATH );
        PathAddBackslashW( pth );
        msi_set_property( package->db, L"System64Folder", pth, -1 );

        GetSystemWow64DirectoryW( pth, MAX_PATH );
        PathAddBackslashW( pth );
        msi_set_property( package->db, L"SystemFolder", pth, -1 );

        len = sizeof(pth);
        RegQueryValueExW(hkey, L"ProgramFilesDir", 0, &type, (BYTE *)pth, &len);
        PathAddBackslashW( pth );
        msi_set_property( package->db, L"ProgramFiles64Folder", pth, -1 );

        len = sizeof(pth);
        RegQueryValueExW(hkey, L"ProgramFilesDir (x86)", 0, &type, (BYTE *)pth, &len);
        PathAddBackslashW( pth );
        msi_set_property( package->db, L"ProgramFilesFolder", pth, -1 );

        len = sizeof(pth);
        RegQueryValueExW(hkey, L"CommonFilesDir", 0, &type, (BYTE *)pth, &len);
        PathAddBackslashW( pth );
        msi_set_property( package->db, L"CommonFiles64Folder", pth, -1 );

        len = sizeof(pth);
        RegQueryValueExW(hkey, L"CommonFilesDir (x86)", 0, &type, (BYTE *)pth, &len);
        PathAddBackslashW( pth );
        msi_set_property( package->db, L"CommonFilesFolder", pth, -1 );
    }

    RegCloseKey(hkey);

    /* Screen properties. */
    dc = GetDC(0);
    len = swprintf( bufstr, ARRAY_SIZE(bufstr), L"%d", GetDeviceCaps(dc, HORZRES) );
    msi_set_property( package->db, L"ScreenX", bufstr, len );
    len = swprintf( bufstr, ARRAY_SIZE(bufstr), L"%d", GetDeviceCaps(dc, VERTRES) );
    msi_set_property( package->db, L"ScreenY", bufstr, len );
    len = swprintf( bufstr, ARRAY_SIZE(bufstr), L"%d", GetDeviceCaps(dc, BITSPIXEL) );
    msi_set_property( package->db, L"ColorBits", bufstr, len );
    ReleaseDC(0, dc);

    /* USERNAME and COMPANYNAME */
    username = msi_dup_property( package->db, L"USERNAME" );
    companyname = msi_dup_property( package->db, L"COMPANYNAME" );

    if ((!username || !companyname) &&
        RegOpenKeyW( HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\MS Setup (ACME)\\User Info", &hkey ) == ERROR_SUCCESS)
    {
        if (!username &&
            (username = msi_reg_get_val_str( hkey, L"DefName" )))
            msi_set_property( package->db, L"USERNAME", username, -1 );
        if (!companyname &&
            (companyname = msi_reg_get_val_str( hkey, L"DefCompany" )))
            msi_set_property( package->db, L"COMPANYNAME", companyname, -1 );
        CloseHandle( hkey );
    }
    if ((!username || !companyname) &&
        RegOpenKeyExW( HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0,
                       KEY_QUERY_VALUE|KEY_WOW64_64KEY, &hkey ) == ERROR_SUCCESS)
    {
        if (!username &&
            (username = msi_reg_get_val_str( hkey, L"RegisteredOwner" )))
            msi_set_property( package->db, L"USERNAME", username, -1 );
        if (!companyname &&
            (companyname = msi_reg_get_val_str( hkey, L"RegisteredOrganization" )))
            msi_set_property( package->db, L"COMPANYNAME", companyname, -1 );
        CloseHandle( hkey );
    }
    free( username );
    free( companyname );

    if ( set_user_sid_prop( package ) != ERROR_SUCCESS)
        ERR("Failed to set the UserSID property\n");

    set_msi_assembly_prop( package );

    langid = GetUserDefaultLangID();
    len = swprintf( bufstr, ARRAY_SIZE(bufstr), L"%d", langid );
    msi_set_property( package->db, L"UserLanguageID", bufstr, len );

    langid = GetSystemDefaultLangID();
    len = swprintf( bufstr, ARRAY_SIZE(bufstr), L"%d", langid );
    msi_set_property( package->db, L"SystemLanguageID", bufstr, len );

    len = swprintf( bufstr, ARRAY_SIZE(bufstr), L"%d", MsiQueryProductStateW(package->ProductCode) );
    msi_set_property( package->db, L"ProductState", bufstr, len );

    len = 0;
    if (!GetUserNameW( NULL, &len ) && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        WCHAR *username;
        if ((username = malloc( len * sizeof(WCHAR) )))
        {
            if (GetUserNameW( username, &len ))
                msi_set_property( package->db, L"LogonUser", username, len - 1 );
            free( username );
        }
    }
    len = 0;
    if (!GetComputerNameW( NULL, &len ) && GetLastError() == ERROR_BUFFER_OVERFLOW)
    {
        WCHAR *computername;
        if ((computername = malloc( len * sizeof(WCHAR) )))
        {
            if (GetComputerNameW( computername, &len ))
                msi_set_property( package->db, L"ComputerName", computername, len );
            free( computername );
        }
    }
}

static MSIPACKAGE *alloc_package( void )
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

static UINT load_admin_properties(MSIPACKAGE *package)
{
    BYTE *data;
    UINT r, sz;

    r = read_stream_data(package->db->storage, L"AdminProperties", FALSE, &data, &sz);
    if (r != ERROR_SUCCESS)
        return r;

    r = msi_parse_command_line(package, (WCHAR *)data, TRUE);

    free(data);
    return r;
}

void msi_adjust_privilege_properties( MSIPACKAGE *package )
{
    /* FIXME: this should depend on the user's privileges */
    if (msi_get_property_int( package->db, L"ALLUSERS", 0 ) == 2)
    {
        TRACE("resetting ALLUSERS property from 2 to 1\n");
        msi_set_property( package->db, L"ALLUSERS", L"1", -1 );
    }
    msi_set_property( package->db, L"AdminUser", L"1", -1 );
    msi_set_property( package->db, L"Privileged", L"1", -1 );
    msi_set_property( package->db, L"MsiRunningElevated", L"1", -1 );
}

MSIPACKAGE *MSI_CreatePackage( MSIDATABASE *db )
{
    MSIPACKAGE *package;
    WCHAR uilevel[11];
    int len;
    UINT r;

    TRACE("%p\n", db);

    package = alloc_package();
    if (package)
    {
        msiobj_addref( &db->hdr );
        package->db = db;

        package->LastAction = NULL;
        package->LastActionTemplate = NULL;
        package->LastActionResult = MSI_NULL_INTEGER;
        package->WordCount = 0;
        package->PackagePath = wcsdup( db->path );

        create_temp_property_table( package );
        msi_clone_properties( package->db );
        msi_adjust_privilege_properties( package );

        package->ProductCode = msi_dup_property( package->db, L"ProductCode" );

        set_installer_properties( package );

        package->ui_level = gUILevel;
        len = swprintf( uilevel, ARRAY_SIZE(uilevel), L"%u", gUILevel & INSTALLUILEVEL_MASK );
        msi_set_property( package->db, L"UILevel", uilevel, len );

        r = msi_load_suminfo_properties( package );
        if (r != ERROR_SUCCESS)
        {
            msiobj_release( &package->hdr );
            return NULL;
        }

        if (package->WordCount & msidbSumInfoSourceTypeAdminImage)
            load_admin_properties( package );

        package->log_file = INVALID_HANDLE_VALUE;
        package->script = SCRIPT_NONE;
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
        cache_entry = malloc( size );
        if ( !GetUrlCacheEntryInfoW( szUrl, cache_entry, &size ) )
        {
            UINT error = GetLastError();
            free( cache_entry );
            return error;
        }

        lstrcpyW( filename, cache_entry->lpszLocalFileName );
        free( cache_entry );
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
    DWORD time, len, i, offset;
    HANDLE handle;

    time = GetTickCount();
    GetWindowsDirectoryW( path, MAX_PATH );
    lstrcatW( path, L"\\Installer\\" );
    CreateDirectoryW( path, NULL );

    len = lstrlenW(path);
    for (i = 0; i < 0x10000; i++)
    {
        offset = swprintf( path + len, MAX_PATH - len, L"%x", (time + i) & 0xffff );
        memcpy( path + len + offset, suffix, (lstrlenW( suffix ) + 1) * sizeof(WCHAR) );
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
    if (!str[0] || !wcscmp( str, L"Intel" )) return PLATFORM_INTEL;
    else if (!wcscmp( str, L"Intel64" )) return PLATFORM_INTEL64;
    else if (!wcscmp( str, L"x64" ) || !wcscmp( str, L"AMD64" )) return PLATFORM_X64;
    else if (!wcscmp( str, L"Arm" )) return PLATFORM_ARM;
    else if (!wcscmp( str, L"Arm64" )) return PLATFORM_ARM64;
    return PLATFORM_UNRECOGNIZED;
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

    p = wcschr( template, ';' );
    if (!p)
    {
        WARN("invalid template string %s\n", debugstr_w(template));
        free( template );
        return ERROR_PATCH_PACKAGE_INVALID;
    }
    *p = 0;
    platform = template;
    if ((q = wcschr( platform, ',' ))) *q = 0;
    package->platform = parse_platform( platform );
    while (package->platform == PLATFORM_UNRECOGNIZED && q)
    {
        platform = q + 1;
        if ((q = wcschr( platform, ',' ))) *q = 0;
        package->platform = parse_platform( platform );
    }
    if (package->platform == PLATFORM_UNRECOGNIZED)
    {
        WARN("unknown platform %s\n", debugstr_w(template));
        free( template );
        return ERROR_INSTALL_PLATFORM_UNSUPPORTED;
    }
    p++;
    if (!*p)
    {
        free( template );
        return ERROR_SUCCESS;
    }
    count = 1;
    for (q = p; (q = wcschr( q, ',' )); q++) count++;

    package->langids = malloc( count * sizeof(LANGID) );
    if (!package->langids)
    {
        free( template );
        return ERROR_OUTOFMEMORY;
    }

    i = 0;
    while (*p)
    {
        q = wcschr( p, ',' );
        if (q) *q = 0;
        package->langids[i] = wcstol( p, NULL, 10 );
        if (!q) break;
        p = q + 1;
        i++;
    }
    package->num_langids = i + 1;

    free( template );
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
#ifndef __aarch64__
    if (package->platform == PLATFORM_ARM64)
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

static WCHAR *get_property( MSIDATABASE *db, const WCHAR *prop )
{
    WCHAR query[MAX_PATH];
    MSIQUERY *view;
    MSIRECORD *rec;
    WCHAR *ret = NULL;

    swprintf(query, ARRAY_SIZE(query), L"SELECT `Value` FROM `Property` WHERE `Property`='%s'", prop);
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
        ret = wcsdup( MSI_RecordGetString( rec, 1 ) );
        msiobj_release( &rec->hdr );
    }
    MSI_ViewClose( view );
    msiobj_release( &view->hdr );
    return ret;
}

static WCHAR *get_product_code( MSIDATABASE *db )
{
    return get_property( db, L"ProductCode" );
}

static WCHAR *get_product_version( MSIDATABASE *db )
{
    return get_property( db, L"ProductVersion" );
}

static UINT get_registered_local_package( const WCHAR *product, WCHAR *localfile )
{
    MSIINSTALLCONTEXT context;
    WCHAR *filename;
    HKEY props_key;
    UINT r;

    r = msi_locate_product( product, &context );
    if (r != ERROR_SUCCESS)
        return r;

    r = MSIREG_OpenInstallProps( product, context, NULL, &props_key, FALSE );
    if (r != ERROR_SUCCESS)
        return r;

    filename = msi_reg_get_val_str( props_key, INSTALLPROPERTY_LOCALPACKAGEW );
    RegCloseKey( props_key );
    if (!filename)
        return ERROR_FUNCTION_FAILED;

    lstrcpyW( localfile, filename );
    free( filename );
    return ERROR_SUCCESS;
}

WCHAR *msi_get_package_code( MSIDATABASE *db )
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

static UINT get_local_package( MSIDATABASE *db, WCHAR *localfile )
{
    WCHAR *product_code;
    UINT r;

    if (!(product_code = get_product_code( db )))
        return ERROR_INSTALL_PACKAGE_INVALID;
    r = get_registered_local_package( product_code, localfile );
    free( product_code );
    return r;
}

UINT msi_set_original_database_property( MSIDATABASE *db, const WCHAR *package )
{
    UINT r;

    if (UrlIsW( package, URLIS_URL ))
        r = msi_set_property( db, L"OriginalDatabase", package, -1 );
    else if (package[0] == '#')
        r = msi_set_property( db, L"OriginalDatabase", db->path, -1 );
    else
    {
        DWORD len;
        WCHAR *path;

        if (!(len = GetFullPathNameW( package, 0, NULL, NULL ))) return GetLastError();
        if (!(path = malloc( len * sizeof(WCHAR) ))) return ERROR_OUTOFMEMORY;
        len = GetFullPathNameW( package, len, path, NULL );
        r = msi_set_property( db, L"OriginalDatabase", path, len );
        free( path );
    }
    return r;
}

#ifdef __REACTOS__
BOOL WINAPI ApphelpCheckRunAppEx(HANDLE FileHandle, PVOID Unk1, PVOID Unk2, PCWSTR ApplicationName, PVOID Environment, USHORT ExeType, PULONG Reason, PVOID *SdbQueryAppCompatData, PULONG SdbQueryAppCompatDataSize,
    PVOID *SxsData, PULONG SxsDataSize, PULONG FusionFlags, PULONG64 SomeFlag1, PULONG SomeFlag2);
BOOL WINAPI SE_DynamicShim(LPCWSTR ProcessImage, PVOID hsdb, PVOID pQueryResult, LPCSTR Module, LPDWORD lpdwDynamicToken);
PVOID WINAPI SdbInitDatabase(DWORD flags, LPCWSTR path);
PVOID WINAPI SdbReleaseDatabase(PVOID hsdb);

#define HID_DOS_PATHS 0x1
#define SDB_DATABASE_MAIN_SHIM 0x80030000

#define APPHELP_VALID_RESULT 0x10000
#define APPHELP_RESULT_FOUND 0x40000

static void
AppHelpCheckPackage(LPCWSTR szPackage)
{
    USHORT ExeType = 0;
    ULONG Reason = 0;

    PVOID QueryResult = NULL;
    ULONG QueryResultSize = 0;

    HANDLE Handle = NULL;
    BOOL Continue = ApphelpCheckRunAppEx(
        Handle, NULL, NULL, szPackage, NULL, ExeType, &Reason, &QueryResult, &QueryResultSize, NULL,
        NULL, NULL, NULL, NULL);

    if (Continue)
    {
        if ((Reason & (APPHELP_VALID_RESULT | APPHELP_RESULT_FOUND)) == (APPHELP_VALID_RESULT | APPHELP_RESULT_FOUND))
        {
            DWORD dwToken;
            PVOID hsdb = SdbInitDatabase(HID_DOS_PATHS | SDB_DATABASE_MAIN_SHIM, NULL);
            if (hsdb)
            {
                BOOL bShim = SE_DynamicShim(szPackage, hsdb, QueryResult, "msi.dll", &dwToken);
                ERR("ReactOS HACK(CORE-13283): Used SE_DynamicShim %d!\n", bShim);

                SdbReleaseDatabase(hsdb);
            }
            else
            {
                ERR("Unable to open SDB_DATABASE_MAIN_SHIM\n");
            }
        }
    }

    if (QueryResult)
        RtlFreeHeap(RtlGetProcessHeap(), 0, QueryResult);
}
#endif

UINT MSI_OpenPackageW(LPCWSTR szPackage, DWORD dwOptions, MSIPACKAGE **pPackage)
{
    MSIDATABASE *db;
    MSIPACKAGE *package;
    MSIHANDLE handle;
    MSIRECORD *data_row, *info_row;
    UINT r;
    WCHAR localfile[MAX_PATH], cachefile[MAX_PATH];
    LPCWSTR file = szPackage;
    DWORD index = 0;
    MSISUMMARYINFO *si;
    BOOL delete_on_close = FALSE;
    WCHAR *info_template, *productname, *product_code;
    MSIINSTALLCONTEXT context;

    TRACE("%s %p\n", debugstr_w(szPackage), pPackage);

    MSI_ProcessMessage(NULL, INSTALLMESSAGE_INITIALIZE, 0);

    localfile[0] = 0;
    if( szPackage[0] == '#' )
    {
        handle = wcstol(&szPackage[1], NULL, 10);
        if (!(db = msihandle2msiinfo(handle, MSIHANDLETYPE_DATABASE)))
            return ERROR_INVALID_HANDLE;
    }
    else
    {
        WCHAR *product_version = NULL;

        if ( UrlIsW( szPackage, URLIS_URL ) )
        {
            r = msi_download_file( szPackage, cachefile );
            if (r != ERROR_SUCCESS)
                return r;

            file = cachefile;
        }
#ifdef __REACTOS__
        AppHelpCheckPackage(file);
#endif

        r = MSI_OpenDatabaseW( file, MSIDBOPEN_READONLY, &db );
        if (r != ERROR_SUCCESS)
        {
            if (GetFileAttributesW( file ) == INVALID_FILE_ATTRIBUTES)
                return ERROR_FILE_NOT_FOUND;
            return r;
        }
        r = get_local_package( db, localfile );
        if (r != ERROR_SUCCESS || GetFileAttributesW( localfile ) == INVALID_FILE_ATTRIBUTES)
        {
            DWORD localfile_attr;

            r = msi_create_empty_local_file( localfile, L".msi" );
            if (r != ERROR_SUCCESS)
            {
                msiobj_release( &db->hdr );
                return r;
            }

            if (!CopyFileW( file, localfile, FALSE ))
            {
                r = GetLastError();
                WARN("unable to copy package %s to %s (%u)\n", debugstr_w(file), debugstr_w(localfile), r);
                DeleteFileW( localfile );
                msiobj_release( &db->hdr );
                return r;
            }
            delete_on_close = TRUE;

            /* Remove read-only bit, we are opening it with write access in MSI_OpenDatabaseW below. */
            localfile_attr = GetFileAttributesW( localfile );
            if (localfile_attr & FILE_ATTRIBUTE_READONLY)
                SetFileAttributesW( localfile, localfile_attr & ~FILE_ATTRIBUTE_READONLY);
        }
        else if (dwOptions & WINE_OPENPACKAGEFLAGS_RECACHE)
        {
            if (!CopyFileW( file, localfile, FALSE ))
            {
                r = GetLastError();
                WARN("unable to update cached package (%u)\n", r);
                msiobj_release( &db->hdr );
                return r;
            }
        }
        else
            product_version = get_product_version( db );
        msiobj_release( &db->hdr );
        TRACE("opening package %s\n", debugstr_w( localfile ));
        r = MSI_OpenDatabaseW( localfile, MSIDBOPEN_TRANSACT, &db );
        if (r != ERROR_SUCCESS)
        {
            free( product_version );
            return r;
        }

        if (product_version)
        {
            WCHAR *cache_version = get_product_version( db );
            if (!product_version != !cache_version ||
                    (product_version && wcscmp(product_version, cache_version)))
            {
                msiobj_release( &db->hdr );
                free( product_version );
                free( cache_version );
                return ERROR_PRODUCT_VERSION;
            }
            free( product_version );
            free( cache_version );
        }
    }
    package = MSI_CreatePackage( db );
    msiobj_release( &db->hdr );
    if (!package) return ERROR_INSTALL_PACKAGE_INVALID;
    package->localfile = wcsdup( localfile );
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
    msi_set_property( package->db, L"DATABASE", db->path, -1 );
    set_installed_prop( package );
    msi_set_context( package );

    product_code = get_product_code( db );
    if (msi_locate_product( product_code, &context ) == ERROR_SUCCESS)
    {
        TRACE("product already registered\n");
        msi_set_property( package->db, L"ProductToBeRegistered", L"1", -1 );
    }
    free( product_code );

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
    free(info_template);
    MSI_ProcessMessage(package, INSTALLMESSAGE_INFO|MB_ICONHAND, info_row);

    MSI_ProcessMessage(package, INSTALLMESSAGE_COMMONDATA, data_row);

    productname = msi_dup_property(package->db, INSTALLPROPERTY_PRODUCTNAMEW);
    MSI_RecordSetInteger(data_row, 1, 1);
    MSI_RecordSetStringW(data_row, 2, productname);
    MSI_RecordSetStringW(data_row, 3, NULL);
    MSI_ProcessMessage(package, INSTALLMESSAGE_COMMONDATA, data_row);

    free(productname);
    msiobj_release(&info_row->hdr);
    msiobj_release(&data_row->hdr);

    *pPackage = package;
    return ERROR_SUCCESS;
}

UINT WINAPI MsiOpenPackageExW( const WCHAR *szPackage, DWORD dwOptions, MSIHANDLE *phPackage )
{
    MSIPACKAGE *package = NULL;
    UINT ret;

    TRACE( "%s, %#lx, %p\n", debugstr_w(szPackage), dwOptions, phPackage );

    if( !szPackage || !phPackage )
        return ERROR_INVALID_PARAMETER;

    if ( !*szPackage )
    {
        FIXME("Should create an empty database and package\n");
        return ERROR_FUNCTION_FAILED;
    }

    if( dwOptions )
        FIXME( "dwOptions %#lx not supported\n", dwOptions );

    ret = MSI_OpenPackageW( szPackage, 0, &package );
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

    free( szwPack );

    return ret;
}

UINT WINAPI MsiOpenPackageA(LPCSTR szPackage, MSIHANDLE *phPackage)
{
    return MsiOpenPackageExA( szPackage, 0, phPackage );
}

MSIHANDLE WINAPI MsiGetActiveDatabase( MSIHANDLE hInstall )
{
    MSIPACKAGE *package;
    MSIHANDLE handle = 0;
    MSIHANDLE remote;

    TRACE( "%lu\n", hInstall );

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE);
    if( package)
    {
        handle = alloc_msihandle( &package->db->hdr );
        msiobj_release( &package->hdr );
    }
    else if ((remote = msi_get_remote(hInstall)))
    {
        __TRY
        {
            handle = remote_GetActiveDatabase(remote);
            handle = alloc_msi_remote_handle(handle);
        }
        __EXCEPT(rpc_filter)
        {
            handle = 0;
        }
        __ENDTRY
    }

    return handle;
}

static INT internal_ui_handler(MSIPACKAGE *package, INSTALLMESSAGE eMessageType, MSIRECORD *record, LPCWSTR message)
{
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
        return MessageBoxW(gUIhwnd, message, L"Windows Installer", eMessageType & 0x00ffffff);
    case INSTALLMESSAGE_WARNING:
        if (package->ui_level & INSTALLUILEVEL_PROGRESSONLY) return 0;
        if (!(eMessageType & MB_ICONMASK))
            eMessageType |= MB_ICONASTERISK;
        return MessageBoxW(gUIhwnd, message, L"Windows Installer", eMessageType & 0x00ffffff);
    case INSTALLMESSAGE_USER:
        if (package->ui_level & INSTALLUILEVEL_PROGRESSONLY) return 0;
        if (!(eMessageType & MB_ICONMASK))
            eMessageType |= MB_USERICON;
        return MessageBoxW(gUIhwnd, message, L"Windows Installer", eMessageType & 0x00ffffff);
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
        free(dialog);
        return rc;
    }
    case INSTALLMESSAGE_ACTIONSTART:
    {
        LPWSTR deformatted;
        MSIRECORD *uirow = MSI_CreateRecord(1);
        if (!uirow) return -1;
        deformat_string(package, MSI_RecordGetString(record, 2), &deformatted);
        MSI_RecordSetStringW(uirow, 1, deformatted);
        msi_event_fire(package, L"ActionText", uirow);

        free(deformatted);
        msiobj_release(&uirow->hdr);
        return 1;
    }
    case INSTALLMESSAGE_ACTIONDATA:
    {
        MSIRECORD *uirow = MSI_CreateRecord(1);
        if (!uirow) return -1;
        MSI_RecordSetStringW(uirow, 1, message);
        msi_event_fire(package, L"ActionData", uirow);
        msiobj_release(&uirow->hdr);

        if (package->action_progress_increment)
        {
            uirow = MSI_CreateRecord(2);
            if (!uirow) return -1;
            MSI_RecordSetInteger(uirow, 1, 2);
            MSI_RecordSetInteger(uirow, 2, package->action_progress_increment);
            msi_event_fire(package, L"SetProgress", uirow);
            msiobj_release(&uirow->hdr);
        }
        return 1;
    }
    case INSTALLMESSAGE_PROGRESS:
        msi_event_fire(package, L"SetProgress", record);
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

static const struct
{
    int id;
    const WCHAR *text;
}
internal_errors[] =
{
    {2726, L"DEBUG: Error [1]:  Action not found: [2]"},
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
    MSIRECORD *record;
    LPWSTR ret = NULL;

    if ((record = MSI_QueryGetRecord(db, L"SELECT `Message` FROM `Error` WHERE `Error` = %d", error)))
    {
        ret = msi_dup_record_field(record, 1);
        msiobj_release(&record->hdr);
    }
    else if (error < 2000)
    {
        int len = LoadStringW(msi_hInstance, IDS_ERROR_BASE + error, (LPWSTR) &ret, 0);
        if (len)
        {
            ret = malloc((len + 1) * sizeof(WCHAR));
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
        message = malloc(len * sizeof(WCHAR));
        if (!message) return ERROR_OUTOFMEMORY;
        MSI_FormatRecordW(package, record, message, &len);
    }

    /* convert it to ANSI */
    len = WideCharToMultiByte( CP_ACP, 0, message, -1, NULL, 0, NULL, NULL );
    msg = malloc( len );
    WideCharToMultiByte( CP_ACP, 0, message, -1, msg, len, NULL, NULL );

    if (gUIHandlerRecord && (gUIFilterRecord & log_type))
    {
        MSIHANDLE rec = alloc_msihandle(&record->hdr);
        TRACE( "calling UI handler %p(pvContext = %p, iMessageType = %#x, hRecord = %lu)\n",
               gUIHandlerRecord, gUIContextRecord, eMessageType, rec );
        rc = gUIHandlerRecord( gUIContextRecord, eMessageType, rec );
        MsiCloseHandle( rec );
    }
    if (!rc && gUIHandlerW && (gUIFilter & log_type))
    {
        TRACE( "calling UI handler %p(pvContext = %p, iMessageType = %#x, szMessage = %s)\n",
               gUIHandlerW, gUIContext, eMessageType, debugstr_w(message) );
        rc = gUIHandlerW( gUIContext, eMessageType, message );
    }
    else if (!rc && gUIHandlerA && (gUIFilter & log_type))
    {
        TRACE( "calling UI handler %p(pvContext = %p, iMessageType = %#x, szMessage = %s)\n",
               gUIHandlerA, gUIContext, eMessageType, debugstr_a(msg) );
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
    free( msg );
    free( message );

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
                        free(template_rec);
                        return 0;
                    }
                }
            }
            else
                template_rec = msi_dup_record_field(record, 0);

            template_prefix = msi_get_error_message(package->db, eMessageType >> 24);
            if (!template_prefix) template_prefix = wcsdup(L"");

            if (!template_rec)
            {
                /* always returns 0 */
                MSI_RecordSetStringW(record, 0, template_prefix);
                MSI_ProcessMessageVerbatim(package, eMessageType, record);
                free(template_prefix);
                return 0;
            }

            template = malloc((wcslen(template_rec) + wcslen(template_prefix) + 1) * sizeof(WCHAR));
            if (!template)
            {
                free(template_prefix);
                free(template_rec);
                return ERROR_OUTOFMEMORY;
            }

            lstrcpyW(template, template_prefix);
            lstrcatW(template, template_rec);
            MSI_RecordSetStringW(record, 0, template);

            free(template_prefix);
            free(template_rec);
            free(template);
        }
        break;
    case INSTALLMESSAGE_ACTIONSTART:
    {
        WCHAR *template = msi_get_error_message(package->db, MSIERR_ACTIONSTART);
        MSI_RecordSetStringW(record, 0, template);
        free(template);

        free(package->LastAction);
        free(package->LastActionTemplate);
        package->LastAction = msi_dup_record_field(record, 1);
        if (!package->LastAction) package->LastAction = wcsdup(L"");
        package->LastActionTemplate = msi_dup_record_field(record, 3);
        break;
    }
    case INSTALLMESSAGE_ACTIONDATA:
        if (package->LastAction && package->LastActionTemplate)
        {
            size_t len = lstrlenW(package->LastAction) + lstrlenW(package->LastActionTemplate) + 7;
            WCHAR *template = malloc(len * sizeof(WCHAR));
            if (!template) return ERROR_OUTOFMEMORY;
            swprintf(template, len, L"{{%s: }}%s", package->LastAction, package->LastActionTemplate);
            MSI_RecordSetStringW(record, 0, template);
            free(template);
        }
        break;
    case INSTALLMESSAGE_COMMONDATA:
    {
        WCHAR *template = msi_get_error_message(package->db, MSIERR_COMMONDATA);
        MSI_RecordSetStringW(record, 0, template);
        free(template);
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

    record = msihandle2msiinfo(hRecord, MSIHANDLETYPE_RECORD);
    if (!record)
        return ERROR_INVALID_HANDLE;

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE );
    if( !package )
    {
        MSIHANDLE remote;

        if (!(remote = msi_get_remote(hInstall)))
            return ERROR_INVALID_HANDLE;

        __TRY
        {
            ret = remote_ProcessMessage(remote, eMessageType, (struct wire_record *)&record->count);
        }
        __EXCEPT(rpc_filter)
        {
            ret = GetExceptionCode();
        }
        __ENDTRY

        msiobj_release(&record->hdr);
        return ret;
    }

    ret = MSI_ProcessMessage( package, eMessageType, record );

    msiobj_release( &record->hdr );
    msiobj_release( &package->hdr );
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
    free( szwName );
    free( szwValue );

    return r;
}

void msi_reset_source_folders( MSIPACKAGE *package )
{
    MSIFOLDER *folder;

    LIST_FOR_EACH_ENTRY( folder, &package->folders, MSIFOLDER, entry )
    {
        free( folder->ResolvedSource );
        folder->ResolvedSource = NULL;
    }
}

UINT msi_set_property( MSIDATABASE *db, const WCHAR *name, const WCHAR *value, int len )
{
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

    if (value && len < 0) len = lstrlenW( value );

    rc = msi_get_property( db, name, 0, &sz );
    if (!value || (!*value && !len))
    {
        swprintf( query, ARRAY_SIZE(query), L"DELETE FROM `_Property` WHERE `_Property` = '%s'", name );
    }
    else if (rc == ERROR_MORE_DATA || rc == ERROR_SUCCESS)
    {
        swprintf( query, ARRAY_SIZE(query), L"UPDATE `_Property` SET `Value` = ? WHERE `_Property` = '%s'", name );
        row = MSI_CreateRecord(1);
        msi_record_set_string( row, 1, value, len );
    }
    else
    {
        lstrcpyW( query, L"INSERT INTO `_Property` (`_Property`,`Value`) VALUES (?,?)" );
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
        MSIHANDLE remote;

        if (!(remote = msi_get_remote(hInstall)))
            return ERROR_INVALID_HANDLE;

        __TRY
        {
            ret = remote_SetProperty(remote, szName, szValue);
        }
        __EXCEPT(rpc_filter)
        {
            ret = GetExceptionCode();
        }
        __ENDTRY

        return ret;
    }

    ret = msi_set_property( package->db, szName, szValue, -1 );
    if (ret == ERROR_SUCCESS && !wcscmp( szName, L"SourceDir" ))
        msi_reset_source_folders( package );

    msiobj_release( &package->hdr );
    return ret;
}

static MSIRECORD *get_property_row( MSIDATABASE *db, const WCHAR *name )
{
    MSIRECORD *rec, *row = NULL;
    MSIQUERY *view;
    UINT r;
    WCHAR *buffer;
    int length;

    if (!name || !*name)
        return NULL;

    if (!wcscmp(name, L"Date"))
    {
        length = GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, NULL, NULL, NULL, 0);
        if (!length)
            return NULL;
        buffer = malloc(length * sizeof(WCHAR));
        GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, NULL, NULL, buffer, length);

        row = MSI_CreateRecord(1);
        if (!row)
        {
            free(buffer);
            return NULL;
        }
        MSI_RecordSetStringW(row, 1, buffer);
        free(buffer);
        return row;
    }
    else if (!wcscmp(name, L"Time"))
    {
        length = GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOTIMEMARKER, NULL, NULL, NULL, 0);
        if (!length)
            return NULL;
        buffer = malloc(length * sizeof(WCHAR));
        GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOTIMEMARKER, NULL, NULL, buffer, length);

        row = MSI_CreateRecord(1);
        if (!row)
        {
            free(buffer);
            return NULL;
        }
        MSI_RecordSetStringW(row, 1, buffer);
        free(buffer);
        return row;
    }

    rec = MSI_CreateRecord(1);
    if (!rec)
        return NULL;

    MSI_RecordSetStringW(rec, 1, name);

    r = MSI_DatabaseOpenViewW(db, L"SELECT `Value` FROM `_Property` WHERE `_Property`=?", &view);
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

    row = get_property_row( db, szName );

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
        TRACE( "need %lu sized buffer for %s\n", *pchValueBuf, debugstr_w(szName) );
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
    str = malloc(sz * sizeof(WCHAR));
    r = msi_get_property(db, prop, str, &sz);
    if (r != ERROR_SUCCESS)
    {
        free(str);
        str = NULL;
    }

    return str;
}

int msi_get_property_int( MSIDATABASE *db, LPCWSTR prop, int def )
{
    LPWSTR str = msi_dup_property( db, prop );
    int val = str ? wcstol(str, NULL, 10) : def;
    free(str);
    return val;
}

UINT WINAPI MsiGetPropertyA(MSIHANDLE hinst, const char *name, char *buf, DWORD *sz)
{
    const WCHAR *value = L"";
    MSIPACKAGE *package;
    MSIRECORD *row;
    WCHAR *nameW;
    int len = 0;
    UINT r;

    if (!name)
        return ERROR_INVALID_PARAMETER;

    if (!(nameW = strdupAtoW(name)))
        return ERROR_OUTOFMEMORY;

    package = msihandle2msiinfo(hinst, MSIHANDLETYPE_PACKAGE);
    if (!package)
    {
        WCHAR *value = NULL, *tmp;
        MSIHANDLE remote;
        DWORD len;

        if (!(remote = msi_get_remote(hinst)))
        {
            free(nameW);
            return ERROR_INVALID_HANDLE;
        }

        __TRY
        {
            r = remote_GetProperty(remote, nameW, &value, &len);
        }
        __EXCEPT(rpc_filter)
        {
            r = GetExceptionCode();
        }
        __ENDTRY

        free(nameW);

        if (!r)
        {
            /* String might contain embedded nulls.
             * Native returns the correct size but truncates the string. */
            tmp = calloc(1, (len + 1) * sizeof(WCHAR));
            if (!tmp)
            {
                midl_user_free(value);
                return ERROR_OUTOFMEMORY;
            }
            lstrcpyW(tmp, value);

            r = msi_strncpyWtoA(tmp, len, buf, sz, TRUE);

            free(tmp);
        }
        midl_user_free(value);
        return r;
    }

    row = get_property_row(package->db, nameW);
    if (row)
        value = msi_record_get_string(row, 1, &len);

    r = msi_strncpyWtoA(value, len, buf, sz, FALSE);

    free(nameW);
    if (row) msiobj_release(&row->hdr);
    msiobj_release(&package->hdr);
    return r;
}

UINT WINAPI MsiGetPropertyW(MSIHANDLE hinst, const WCHAR *name, WCHAR *buf, DWORD *sz)
{
    const WCHAR *value = L"";
    MSIPACKAGE *package;
    MSIRECORD *row;
    int len = 0;
    UINT r;

    if (!name)
        return ERROR_INVALID_PARAMETER;

    package = msihandle2msiinfo(hinst, MSIHANDLETYPE_PACKAGE);
    if (!package)
    {
        WCHAR *value = NULL, *tmp;
        MSIHANDLE remote;
        DWORD len;

        if (!(remote = msi_get_remote(hinst)))
            return ERROR_INVALID_HANDLE;

        __TRY
        {
            r = remote_GetProperty(remote, name, &value, &len);
        }
        __EXCEPT(rpc_filter)
        {
            r = GetExceptionCode();
        }
        __ENDTRY

        if (!r)
        {
            /* String might contain embedded nulls.
             * Native returns the correct size but truncates the string. */
            tmp = calloc(1, (len + 1) * sizeof(WCHAR));
            if (!tmp)
            {
                midl_user_free(value);
                return ERROR_OUTOFMEMORY;
            }
            lstrcpyW(tmp, value);

            r = msi_strncpyW(tmp, len, buf, sz);

            free(tmp);
        }
        midl_user_free(value);
        return r;
    }

    row = get_property_row(package->db, name);
    if (row)
        value = msi_record_get_string(row, 1, &len);

    r = msi_strncpyW(value, len, buf, sz);

    if (row) msiobj_release(&row->hdr);
    msiobj_release(&package->hdr);
    return r;
}

MSIHANDLE __cdecl s_remote_GetActiveDatabase(MSIHANDLE hinst)
{
    return MsiGetActiveDatabase(hinst);
}

UINT __cdecl s_remote_GetProperty(MSIHANDLE hinst, LPCWSTR property, LPWSTR *value, DWORD *size)
{
    WCHAR empty[1];
    UINT r;

    *size = 0;
    r = MsiGetPropertyW(hinst, property, empty, size);
    if (r == ERROR_MORE_DATA)
    {
        ++*size;
        *value = midl_user_allocate(*size * sizeof(WCHAR));
        if (!*value)
            return ERROR_OUTOFMEMORY;
        r = MsiGetPropertyW(hinst, property, *value, size);
    }
    return r;
}

UINT __cdecl s_remote_SetProperty(MSIHANDLE hinst, LPCWSTR property, LPCWSTR value)
{
    return MsiSetPropertyW(hinst, property, value);
}

int __cdecl s_remote_ProcessMessage(MSIHANDLE hinst, INSTALLMESSAGE message, struct wire_record *remote_rec)
{
    MSIHANDLE rec;
    int ret;
    UINT r;

    if ((r = unmarshal_record(remote_rec, &rec)))
        return r;

    ret = MsiProcessMessage(hinst, message, rec);

    MsiCloseHandle(rec);
    return ret;
}

UINT __cdecl s_remote_DoAction(MSIHANDLE hinst, LPCWSTR action)
{
    return MsiDoActionW(hinst, action);
}

UINT __cdecl s_remote_Sequence(MSIHANDLE hinst, LPCWSTR table, int sequence)
{
    return MsiSequenceW(hinst, table, sequence);
}

UINT __cdecl s_remote_GetTargetPath(MSIHANDLE hinst, LPCWSTR folder, LPWSTR *value)
{
    WCHAR empty[1];
    DWORD size = 0;
    UINT r;

    r = MsiGetTargetPathW(hinst, folder, empty, &size);
    if (r == ERROR_MORE_DATA)
    {
        *value = midl_user_allocate(++size * sizeof(WCHAR));
        if (!*value)
            return ERROR_OUTOFMEMORY;
        r = MsiGetTargetPathW(hinst, folder, *value, &size);
    }
    return r;
}

UINT __cdecl s_remote_SetTargetPath(MSIHANDLE hinst, LPCWSTR folder, LPCWSTR value)
{
    return MsiSetTargetPathW(hinst, folder, value);
}

UINT __cdecl s_remote_GetSourcePath(MSIHANDLE hinst, LPCWSTR folder, LPWSTR *value)
{
    WCHAR empty[1];
    DWORD size = 1;
    UINT r;

    r = MsiGetSourcePathW(hinst, folder, empty, &size);
    if (r == ERROR_MORE_DATA)
    {
        *value = midl_user_allocate(++size * sizeof(WCHAR));
        if (!*value)
            return ERROR_OUTOFMEMORY;
        r = MsiGetSourcePathW(hinst, folder, *value, &size);
    }
    return r;
}

BOOL __cdecl s_remote_GetMode(MSIHANDLE hinst, MSIRUNMODE mode)
{
    return MsiGetMode(hinst, mode);
}

UINT __cdecl s_remote_SetMode(MSIHANDLE hinst, MSIRUNMODE mode, BOOL state)
{
    return MsiSetMode(hinst, mode, state);
}

UINT __cdecl s_remote_GetFeatureState(MSIHANDLE hinst, LPCWSTR feature,
                                    INSTALLSTATE *installed, INSTALLSTATE *action)
{
    return MsiGetFeatureStateW(hinst, feature, installed, action);
}

UINT __cdecl s_remote_SetFeatureState(MSIHANDLE hinst, LPCWSTR feature, INSTALLSTATE state)
{
    return MsiSetFeatureStateW(hinst, feature, state);
}

UINT __cdecl s_remote_GetComponentState(MSIHANDLE hinst, LPCWSTR component,
                                      INSTALLSTATE *installed, INSTALLSTATE *action)
{
    return MsiGetComponentStateW(hinst, component, installed, action);
}

UINT __cdecl s_remote_SetComponentState(MSIHANDLE hinst, LPCWSTR component, INSTALLSTATE state)
{
    return MsiSetComponentStateW(hinst, component, state);
}

LANGID __cdecl s_remote_GetLanguage(MSIHANDLE hinst)
{
    return MsiGetLanguage(hinst);
}

UINT __cdecl s_remote_SetInstallLevel(MSIHANDLE hinst, int level)
{
    return MsiSetInstallLevel(hinst, level);
}

UINT __cdecl s_remote_FormatRecord(MSIHANDLE hinst, struct wire_record *remote_rec, LPWSTR *value)
{
    WCHAR empty[1];
    DWORD size = 0;
    MSIHANDLE rec;
    UINT r;

    if ((r = unmarshal_record(remote_rec, &rec)))
        return r;

    r = MsiFormatRecordW(hinst, rec, empty, &size);
    if (r == ERROR_MORE_DATA)
    {
        *value = midl_user_allocate(++size * sizeof(WCHAR));
        if (!*value)
        {
            MsiCloseHandle(rec);
            return ERROR_OUTOFMEMORY;
        }
        r = MsiFormatRecordW(hinst, rec, *value, &size);
    }

    MsiCloseHandle(rec);
    return r;
}

MSICONDITION __cdecl s_remote_EvaluateCondition(MSIHANDLE hinst, LPCWSTR condition)
{
    return MsiEvaluateConditionW(hinst, condition);
}

UINT __cdecl s_remote_GetFeatureCost(MSIHANDLE hinst, LPCWSTR feature,
    MSICOSTTREE cost_tree, INSTALLSTATE state, INT *cost)
{
    return MsiGetFeatureCostW(hinst, feature, cost_tree, state, cost);
}

UINT __cdecl s_remote_EnumComponentCosts(MSIHANDLE hinst, LPCWSTR component,
    DWORD index, INSTALLSTATE state, LPWSTR drive, INT *cost, INT *temp)
{
    DWORD size = 3;
    return MsiEnumComponentCostsW(hinst, component, index, state, drive, &size, cost, temp);
}

UINT msi_package_add_info(MSIPACKAGE *package, DWORD context, DWORD options,
                          LPCWSTR property, LPWSTR value)
{
    MSISOURCELISTINFO *info;

    LIST_FOR_EACH_ENTRY( info, &package->sourcelist_info, MSISOURCELISTINFO, entry )
    {
        if (!wcscmp( info->value, value )) return ERROR_SUCCESS;
    }

    info = malloc(sizeof(MSISOURCELISTINFO));
    if (!info)
        return ERROR_OUTOFMEMORY;

    info->context = context;
    info->options = options;
    info->property = property;
    info->value = wcsdup(value);
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

    disk = malloc(sizeof(MSIMEDIADISK));
    if (!disk)
        return ERROR_OUTOFMEMORY;

    disk->context = context;
    disk->options = options;
    disk->disk_id = disk_id;
    disk->volume_label = wcsdup(volume_label);
    disk->disk_prompt = wcsdup(disk_prompt);
    list_add_head(&package->sourcelist_media, &disk->entry);

    return ERROR_SUCCESS;
}
