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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* Msi top level apis directly related to installs */

#include "msipriv.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

/***********************************************************************
 * MsiDoActionA       (MSI.@)
 */
UINT WINAPI MsiDoActionA( MSIHANDLE hInstall, LPCSTR szAction )
{
    LPWSTR szwAction;
    UINT ret;

    TRACE("%s\n", debugstr_a(szAction));

    szwAction = strdupAtoW(szAction);
    if (szAction && !szwAction)
        return ERROR_FUNCTION_FAILED; 

    ret = MsiDoActionW( hInstall, szwAction );
    msi_free( szwAction );
    return ret;
}

/***********************************************************************
 * MsiDoActionW       (MSI.@)
 */
UINT WINAPI MsiDoActionW( MSIHANDLE hInstall, LPCWSTR szAction )
{
    MSIPACKAGE *package;
    UINT ret;

    TRACE("%s\n",debugstr_w(szAction));

    if (!szAction)
        return ERROR_INVALID_PARAMETER;

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE );
    if (!package)
    {
        HRESULT hr;
        BSTR action;
        IWineMsiRemotePackage *remote_package;

        remote_package = (IWineMsiRemotePackage *)msi_get_remote( hInstall );
        if (!remote_package)
            return ERROR_INVALID_HANDLE;

        action = SysAllocString( szAction );
        if (!action)
        {
            IWineMsiRemotePackage_Release( remote_package );
            return ERROR_OUTOFMEMORY;
        }

        hr = IWineMsiRemotePackage_DoAction( remote_package, action );

        SysFreeString( action );
        IWineMsiRemotePackage_Release( remote_package );

        if (FAILED(hr))
        {
            if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
                return HRESULT_CODE(hr);

            return ERROR_FUNCTION_FAILED;
        }

        return ERROR_SUCCESS;
    }
 
    ret = ACTION_PerformUIAction( package, szAction, SCRIPT_NONE );
    msiobj_release( &package->hdr );

    return ret;
}

/***********************************************************************
 * MsiSequenceA       (MSI.@)
 */
UINT WINAPI MsiSequenceA( MSIHANDLE hInstall, LPCSTR szTable, INT iSequenceMode )
{
    LPWSTR szwTable;
    UINT ret;

    TRACE("%s, %d\n", debugstr_a(szTable), iSequenceMode);

    szwTable = strdupAtoW(szTable);
    if (szTable && !szwTable)
        return ERROR_FUNCTION_FAILED; 

    ret = MsiSequenceW( hInstall, szwTable, iSequenceMode );
    msi_free( szwTable );
    return ret;
}

/***********************************************************************
 * MsiSequenceW       (MSI.@)
 */
UINT WINAPI MsiSequenceW( MSIHANDLE hInstall, LPCWSTR szTable, INT iSequenceMode )
{
    MSIPACKAGE *package;
    UINT ret;

    TRACE("%s, %d\n", debugstr_w(szTable), iSequenceMode);

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE );
    if (!package)
    {
        HRESULT hr;
        BSTR table;
        IWineMsiRemotePackage *remote_package;

        remote_package = (IWineMsiRemotePackage *)msi_get_remote( hInstall );
        if (!remote_package)
            return ERROR_INVALID_HANDLE;

        table = SysAllocString( szTable );
        if (!table)
        {
            IWineMsiRemotePackage_Release( remote_package );
            return ERROR_OUTOFMEMORY;
        }

        hr = IWineMsiRemotePackage_Sequence( remote_package, table, iSequenceMode );

        SysFreeString( table );
        IWineMsiRemotePackage_Release( remote_package );

        if (FAILED(hr))
        {
            if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
                return HRESULT_CODE(hr);

            return ERROR_FUNCTION_FAILED;
        }

        return ERROR_SUCCESS;
    }
    ret = MSI_Sequence( package, szTable );
    msiobj_release( &package->hdr );
    return ret;
}

UINT msi_strcpy_to_awstring( const WCHAR *str, int len, awstring *awbuf, DWORD *sz )
{
    UINT r = ERROR_SUCCESS;

    if (awbuf->str.w && !sz)
        return ERROR_INVALID_PARAMETER;
    if (!sz)
        return ERROR_SUCCESS;

    if (len < 0) len = strlenW( str );
 
    if (awbuf->unicode && awbuf->str.w)
        memcpy( awbuf->str.w, str, min(len + 1, *sz) * sizeof(WCHAR) );
    else
    {
        int lenA = WideCharToMultiByte( CP_ACP, 0, str, len + 1, NULL, 0, NULL, NULL );
        if (lenA) lenA--;
        WideCharToMultiByte( CP_ACP, 0, str, len + 1, awbuf->str.a, *sz, NULL, NULL );
        if (awbuf->str.a && *sz && lenA >= *sz)
            awbuf->str.a[*sz - 1] = 0;
        len = lenA;
    }
    if (awbuf->str.w && len >= *sz)
        r = ERROR_MORE_DATA;
    *sz = len;
    return r;
}

const WCHAR *msi_get_target_folder( MSIPACKAGE *package, const WCHAR *name )
{
    MSIFOLDER *folder = msi_get_loaded_folder( package, name );

    if (!folder) return NULL;
    if (!folder->ResolvedTarget)
    {
        MSIFOLDER *parent = folder;
        while (parent->Parent && strcmpW( parent->Parent, parent->Directory ))
        {
            parent = msi_get_loaded_folder( package, parent->Parent );
        }
        msi_resolve_target_folder( package, parent->Directory, TRUE );
    }
    return folder->ResolvedTarget;
}

/***********************************************************************
 * MsiGetTargetPath   (internal)
 */
static UINT MSI_GetTargetPath( MSIHANDLE hInstall, LPCWSTR szFolder,
                               awstring *szPathBuf, LPDWORD pcchPathBuf )
{
    MSIPACKAGE *package;
    const WCHAR *path;
    UINT r = ERROR_FUNCTION_FAILED;

    if (!szFolder)
        return ERROR_INVALID_PARAMETER;

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE );
    if (!package)
    {
        HRESULT hr;
        IWineMsiRemotePackage *remote_package;
        LPWSTR value = NULL;
        BSTR folder;
        DWORD len;

        remote_package = (IWineMsiRemotePackage *)msi_get_remote( hInstall );
        if (!remote_package)
            return ERROR_INVALID_HANDLE;

        folder = SysAllocString( szFolder );
        if (!folder)
        {
            IWineMsiRemotePackage_Release( remote_package );
            return ERROR_OUTOFMEMORY;
        }

        len = 0;
        hr = IWineMsiRemotePackage_GetTargetPath( remote_package, folder, NULL, &len );
        if (FAILED(hr))
            goto done;

        len++;
        value = msi_alloc(len * sizeof(WCHAR));
        if (!value)
        {
            r = ERROR_OUTOFMEMORY;
            goto done;
        }

        hr = IWineMsiRemotePackage_GetTargetPath( remote_package, folder, value, &len );
        if (FAILED(hr))
            goto done;

        r = msi_strcpy_to_awstring( value, len, szPathBuf, pcchPathBuf );

done:
        IWineMsiRemotePackage_Release( remote_package );
        SysFreeString( folder );
        msi_free( value );

        if (FAILED(hr))
        {
            if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
                return HRESULT_CODE(hr);

            return ERROR_FUNCTION_FAILED;
        }

        return r;
    }

    path = msi_get_target_folder( package, szFolder );
    msiobj_release( &package->hdr );

    if (!path)
        return ERROR_DIRECTORY;

    return msi_strcpy_to_awstring( path, -1, szPathBuf, pcchPathBuf );
}

/***********************************************************************
 * MsiGetTargetPathA        (MSI.@)
 */
UINT WINAPI MsiGetTargetPathA( MSIHANDLE hInstall, LPCSTR szFolder, 
                               LPSTR szPathBuf, LPDWORD pcchPathBuf )
{
    LPWSTR szwFolder;
    awstring path;
    UINT r;

    TRACE("%s %p %p\n", debugstr_a(szFolder), szPathBuf, pcchPathBuf);

    szwFolder = strdupAtoW(szFolder);
    if (szFolder && !szwFolder)
        return ERROR_FUNCTION_FAILED; 

    path.unicode = FALSE;
    path.str.a = szPathBuf;

    r = MSI_GetTargetPath( hInstall, szwFolder, &path, pcchPathBuf );

    msi_free( szwFolder );

    return r;
}

/***********************************************************************
 * MsiGetTargetPathW        (MSI.@)
 */
UINT WINAPI MsiGetTargetPathW( MSIHANDLE hInstall, LPCWSTR szFolder,
                               LPWSTR szPathBuf, LPDWORD pcchPathBuf )
{
    awstring path;

    TRACE("%s %p %p\n", debugstr_w(szFolder), szPathBuf, pcchPathBuf);

    path.unicode = TRUE;
    path.str.w = szPathBuf;

    return MSI_GetTargetPath( hInstall, szFolder, &path, pcchPathBuf );
}

static WCHAR *get_source_root( MSIPACKAGE *package )
{
    msi_set_sourcedir_props( package, FALSE );
    return msi_dup_property( package->db, szSourceDir );
}

WCHAR *msi_resolve_source_folder( MSIPACKAGE *package, const WCHAR *name, MSIFOLDER **folder )
{
    MSIFOLDER *f;
    LPWSTR p, path = NULL, parent;

    TRACE("working to resolve %s\n", debugstr_w(name));

    if (!strcmpW( name, szSourceDir )) name = szTargetDir;
    if (!(f = msi_get_loaded_folder( package, name ))) return NULL;

    /* special resolving for root dir */
    if (!strcmpW( name, szTargetDir ) && !f->ResolvedSource)
    {
        f->ResolvedSource = get_source_root( package );
    }
    if (folder) *folder = f;
    if (f->ResolvedSource)
    {
        path = strdupW( f->ResolvedSource );
        TRACE("   already resolved to %s\n", debugstr_w(path));
        return path;
    }
    if (!f->Parent) return path;
    parent = f->Parent;
    TRACE(" ! parent is %s\n", debugstr_w(parent));

    p = msi_resolve_source_folder( package, parent, NULL );

    if (package->WordCount & msidbSumInfoSourceTypeCompressed)
        path = get_source_root( package );
    else if (package->WordCount & msidbSumInfoSourceTypeSFN)
        path = msi_build_directory_name( 3, p, f->SourceShortPath, NULL );
    else
        path = msi_build_directory_name( 3, p, f->SourceLongPath, NULL );

    TRACE("-> %s\n", debugstr_w(path));
    f->ResolvedSource = strdupW( path );
    msi_free( p );

    return path;
}

/***********************************************************************
 * MSI_GetSourcePath   (internal)
 */
static UINT MSI_GetSourcePath( MSIHANDLE hInstall, LPCWSTR szFolder,
                               awstring *szPathBuf, LPDWORD pcchPathBuf )
{
    MSIPACKAGE *package;
    LPWSTR path;
    UINT r = ERROR_FUNCTION_FAILED;

    TRACE("%s %p %p\n", debugstr_w(szFolder), szPathBuf, pcchPathBuf );

    if (!szFolder)
        return ERROR_INVALID_PARAMETER;

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE );
    if (!package)
    {
        HRESULT hr;
        IWineMsiRemotePackage *remote_package;
        LPWSTR value = NULL;
        BSTR folder;
        DWORD len;

        remote_package = (IWineMsiRemotePackage *)msi_get_remote( hInstall );
        if (!remote_package)
            return ERROR_INVALID_HANDLE;

        folder = SysAllocString( szFolder );
        if (!folder)
        {
            IWineMsiRemotePackage_Release( remote_package );
            return ERROR_OUTOFMEMORY;
        }

        len = 0;
        hr = IWineMsiRemotePackage_GetSourcePath( remote_package, folder, NULL, &len );
        if (FAILED(hr))
            goto done;

        len++;
        value = msi_alloc(len * sizeof(WCHAR));
        if (!value)
        {
            r = ERROR_OUTOFMEMORY;
            goto done;
        }

        hr = IWineMsiRemotePackage_GetSourcePath( remote_package, folder, value, &len );
        if (FAILED(hr))
            goto done;

        r = msi_strcpy_to_awstring( value, len, szPathBuf, pcchPathBuf );

done:
        IWineMsiRemotePackage_Release( remote_package );
        SysFreeString( folder );
        msi_free( value );

        if (FAILED(hr))
        {
            if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
                return HRESULT_CODE(hr);

            return ERROR_FUNCTION_FAILED;
        }

        return r;
    }

    if (szPathBuf->str.w && !pcchPathBuf )
    {
        msiobj_release( &package->hdr );
        return ERROR_INVALID_PARAMETER;
    }

    path = msi_resolve_source_folder( package, szFolder, NULL );
    msiobj_release( &package->hdr );

    TRACE("path = %s\n", debugstr_w(path));
    if (!path)
        return ERROR_DIRECTORY;

    r = msi_strcpy_to_awstring( path, -1, szPathBuf, pcchPathBuf );
    msi_free( path );
    return r;
}

/***********************************************************************
 * MsiGetSourcePathA     (MSI.@)
 */
UINT WINAPI MsiGetSourcePathA( MSIHANDLE hInstall, LPCSTR szFolder, 
                               LPSTR szPathBuf, LPDWORD pcchPathBuf )
{
    LPWSTR folder;
    awstring str;
    UINT r;

    TRACE("%s %p %p\n", debugstr_a(szFolder), szPathBuf, pcchPathBuf);

    str.unicode = FALSE;
    str.str.a = szPathBuf;

    folder = strdupAtoW( szFolder );
    r = MSI_GetSourcePath( hInstall, folder, &str, pcchPathBuf );
    msi_free( folder );

    return r;
}

/***********************************************************************
 * MsiGetSourcePathW     (MSI.@)
 */
UINT WINAPI MsiGetSourcePathW( MSIHANDLE hInstall, LPCWSTR szFolder,
                               LPWSTR szPathBuf, LPDWORD pcchPathBuf )
{
    awstring str;

    TRACE("%s %p %p\n", debugstr_w(szFolder), szPathBuf, pcchPathBuf );

    str.unicode = TRUE;
    str.str.w = szPathBuf;

    return MSI_GetSourcePath( hInstall, szFolder, &str, pcchPathBuf );
}

/***********************************************************************
 * MsiSetTargetPathA  (MSI.@)
 */
UINT WINAPI MsiSetTargetPathA( MSIHANDLE hInstall, LPCSTR szFolder,
                               LPCSTR szFolderPath )
{
    LPWSTR szwFolder = NULL, szwFolderPath = NULL;
    UINT rc = ERROR_OUTOFMEMORY;

    if ( !szFolder || !szFolderPath )
        return ERROR_INVALID_PARAMETER;

    szwFolder = strdupAtoW(szFolder);
    szwFolderPath = strdupAtoW(szFolderPath);
    if (!szwFolder || !szwFolderPath)
        goto end;

    rc = MsiSetTargetPathW( hInstall, szwFolder, szwFolderPath );

end:
    msi_free(szwFolder);
    msi_free(szwFolderPath);

    return rc;
}

static void set_target_path( MSIPACKAGE *package, MSIFOLDER *folder, const WCHAR *path )
{
    FolderList *fl;
    MSIFOLDER *child;
    WCHAR *target_path;

    if (!(target_path = msi_normalize_path( path ))) return;
    if (strcmpW( target_path, folder->ResolvedTarget ))
    {
        msi_free( folder->ResolvedTarget );
        folder->ResolvedTarget = target_path;
        msi_set_property( package->db, folder->Directory, folder->ResolvedTarget, -1 );

        LIST_FOR_EACH_ENTRY( fl, &folder->children, FolderList, entry )
        {
            child = fl->folder;
            msi_resolve_target_folder( package, child->Directory, FALSE );
        }
    }
    else msi_free( target_path );
}

UINT MSI_SetTargetPathW( MSIPACKAGE *package, LPCWSTR szFolder, LPCWSTR szFolderPath )
{
    DWORD attrib;
    MSIFOLDER *folder;
    MSIFILE *file;

    TRACE("%p %s %s\n", package, debugstr_w(szFolder), debugstr_w(szFolderPath));

    attrib = GetFileAttributesW(szFolderPath);
    /* native MSI tests writeability by making temporary files at each drive */
    if (attrib != INVALID_FILE_ATTRIBUTES &&
        (attrib & FILE_ATTRIBUTE_OFFLINE || attrib & FILE_ATTRIBUTE_READONLY))
    {
        return ERROR_FUNCTION_FAILED;
    }
    if (!(folder = msi_get_loaded_folder( package, szFolder ))) return ERROR_DIRECTORY;

    set_target_path( package, folder, szFolderPath );

    LIST_FOR_EACH_ENTRY( file, &package->files, MSIFILE, entry )
    {
        const WCHAR *dir;
        MSICOMPONENT *comp = file->Component;

        if (!comp->Enabled || msi_is_global_assembly( comp )) continue;

        dir = msi_get_target_folder( package, comp->Directory );
        msi_free( file->TargetPath );
        file->TargetPath = msi_build_directory_name( 2, dir, file->FileName );
    }
    return ERROR_SUCCESS;
}

/***********************************************************************
 * MsiSetTargetPathW  (MSI.@)
 */
UINT WINAPI MsiSetTargetPathW(MSIHANDLE hInstall, LPCWSTR szFolder, 
                             LPCWSTR szFolderPath)
{
    MSIPACKAGE *package;
    UINT ret;

    TRACE("%s %s\n",debugstr_w(szFolder),debugstr_w(szFolderPath));

    if ( !szFolder || !szFolderPath )
        return ERROR_INVALID_PARAMETER;

    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);
    if (!package)
    {
        HRESULT hr;
        BSTR folder, path;
        IWineMsiRemotePackage *remote_package;

        remote_package = (IWineMsiRemotePackage *)msi_get_remote( hInstall );
        if (!remote_package)
            return ERROR_INVALID_HANDLE;

        folder = SysAllocString( szFolder );
        path = SysAllocString( szFolderPath );
        if (!folder || !path)
        {
            SysFreeString(folder);
            SysFreeString(path);
            IWineMsiRemotePackage_Release( remote_package );
            return ERROR_OUTOFMEMORY;
        }

        hr = IWineMsiRemotePackage_SetTargetPath( remote_package, folder, path );

        SysFreeString(folder);
        SysFreeString(path);
        IWineMsiRemotePackage_Release( remote_package );

        if (FAILED(hr))
        {
            if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
                return HRESULT_CODE(hr);

            return ERROR_FUNCTION_FAILED;
        }

        return ERROR_SUCCESS;
    }

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
 *        MSIRUNMODE_SOURCESHORTNAMES  Long names in source files is suppressed
 *        MSIRUNMODE_TARGETSHORTNAMES  Long names in destination files is suppressed
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
    MSIPACKAGE *package;
    BOOL r = FALSE;

    TRACE("%d %d\n", hInstall, iRunMode);

    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);
    if (!package)
    {
        BOOL ret;
        HRESULT hr;
        IWineMsiRemotePackage *remote_package;

        remote_package = (IWineMsiRemotePackage *)msi_get_remote(hInstall);
        if (!remote_package)
            return FALSE;

        hr = IWineMsiRemotePackage_GetMode(remote_package, iRunMode, &ret);
        IWineMsiRemotePackage_Release(remote_package);

        if (hr == S_OK)
            return ret;

        return FALSE;
    }

    switch (iRunMode)
    {
    case MSIRUNMODE_ADMIN:
        FIXME("no support for administrative installs\n");
        break;

    case MSIRUNMODE_ADVERTISE:
        FIXME("no support for advertised installs\n");
        break;

    case MSIRUNMODE_WINDOWS9X:
        if (GetVersion() & 0x80000000)
            r = TRUE;
        break;

    case MSIRUNMODE_OPERATIONS:
    case MSIRUNMODE_RESERVED11:
    case MSIRUNMODE_RESERVED14:
    case MSIRUNMODE_RESERVED15:
        break;

    case MSIRUNMODE_SCHEDULED:
        r = package->scheduled_action_running;
        break;

    case MSIRUNMODE_ROLLBACK:
        r = package->rollback_action_running;
        break;

    case MSIRUNMODE_COMMIT:
        r = package->commit_action_running;
        break;

    case MSIRUNMODE_MAINTENANCE:
        r = msi_get_property_int( package->db, szInstalled, 0 ) != 0;
        break;

    case MSIRUNMODE_ROLLBACKENABLED:
        r = msi_get_property_int( package->db, szRollbackDisabled, 0 ) == 0;
        break;

    case MSIRUNMODE_REBOOTATEND:
        r = package->need_reboot_at_end;
        break;

    case MSIRUNMODE_REBOOTNOW:
        r = package->need_reboot_now;
        break;

    case MSIRUNMODE_LOGENABLED:
        r = (package->log_file != INVALID_HANDLE_VALUE);
        break;

    default:
        FIXME("unimplemented run mode: %d\n", iRunMode);
        r = TRUE;
    }

    msiobj_release( &package->hdr );
    return r;
}

/***********************************************************************
 *           MsiSetMode    (MSI.@)
 */
UINT WINAPI MsiSetMode(MSIHANDLE hInstall, MSIRUNMODE iRunMode, BOOL fState)
{
    MSIPACKAGE *package;
    UINT r;

    TRACE("%d %d %d\n", hInstall, iRunMode, fState);

    package = msihandle2msiinfo( hInstall, MSIHANDLETYPE_PACKAGE );
    if (!package)
    {
        HRESULT hr;
        IWineMsiRemotePackage *remote_package;

        remote_package = (IWineMsiRemotePackage *)msi_get_remote( hInstall );
        if (!remote_package)
            return FALSE;

        hr = IWineMsiRemotePackage_SetMode( remote_package, iRunMode, fState );
        IWineMsiRemotePackage_Release( remote_package );

        if (FAILED(hr))
        {
            if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
                return HRESULT_CODE(hr);

            return ERROR_FUNCTION_FAILED;
        }

        return ERROR_SUCCESS;
    }

    switch (iRunMode)
    {
    case MSIRUNMODE_REBOOTATEND:
        package->need_reboot_at_end = (fState != 0);
        r = ERROR_SUCCESS;
        break;

    case MSIRUNMODE_REBOOTNOW:
        package->need_reboot_now = (fState != 0);
        r = ERROR_SUCCESS;
        break;

    default:
        r = ERROR_ACCESS_DENIED;
    }

    msiobj_release( &package->hdr );
    return r;
}

/***********************************************************************
 * MsiSetFeatureStateA (MSI.@)
 *
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

    msi_free(szwFeature);

    return rc;
}

/* update component state based on a feature change */
void ACTION_UpdateComponentStates( MSIPACKAGE *package, MSIFEATURE *feature )
{
    INSTALLSTATE newstate;
    ComponentList *cl;

    newstate = feature->ActionRequest;
    if (newstate == INSTALLSTATE_ABSENT) newstate = INSTALLSTATE_UNKNOWN;

    LIST_FOR_EACH_ENTRY(cl, &feature->Components, ComponentList, entry)
    {
        MSICOMPONENT *component = cl->component;

        if (!component->Enabled) continue;

        TRACE("Modifying (%d): Component %s (Installed %d, Action %d, Request %d)\n",
            newstate, debugstr_w(component->Component), component->Installed,
            component->Action, component->ActionRequest);

        if (newstate == INSTALLSTATE_LOCAL)
        {
            component->Action = INSTALLSTATE_LOCAL;
            component->ActionRequest = INSTALLSTATE_LOCAL;
        }
        else
        {
            ComponentList *clist;
            MSIFEATURE *f;

            component->hasLocalFeature = FALSE;

            component->Action = newstate;
            component->ActionRequest = newstate;
            /* if any other feature wants it local we need to set it local */
            LIST_FOR_EACH_ENTRY(f, &package->features, MSIFEATURE, entry)
            {
                if ( f->ActionRequest != INSTALLSTATE_LOCAL &&
                     f->ActionRequest != INSTALLSTATE_SOURCE )
                {
                    continue;
                }
                LIST_FOR_EACH_ENTRY(clist, &f->Components, ComponentList, entry)
                {
                    if (clist->component == component &&
                        (f->ActionRequest == INSTALLSTATE_LOCAL ||
                         f->ActionRequest == INSTALLSTATE_SOURCE))
                    {
                        TRACE("Saved by %s\n", debugstr_w(f->Feature));
                        component->hasLocalFeature = TRUE;

                        if (component->Attributes & msidbComponentAttributesOptional)
                        {
                            if (f->Attributes & msidbFeatureAttributesFavorSource)
                            {
                                component->Action = INSTALLSTATE_SOURCE;
                                component->ActionRequest = INSTALLSTATE_SOURCE;
                            }
                            else
                            {
                                component->Action = INSTALLSTATE_LOCAL;
                                component->ActionRequest = INSTALLSTATE_LOCAL;
                            }
                        }
                        else if (component->Attributes & msidbComponentAttributesSourceOnly)
                        {
                            component->Action = INSTALLSTATE_SOURCE;
                            component->ActionRequest = INSTALLSTATE_SOURCE;
                        }
                        else
                        {
                            component->Action = INSTALLSTATE_LOCAL;
                            component->ActionRequest = INSTALLSTATE_LOCAL;
                        }
                    }
                }
            }
        }
        TRACE("Result (%d): Component %s (Installed %d, Action %d, Request %d)\n",
            newstate, debugstr_w(component->Component), component->Installed,
            component->Action, component->ActionRequest);
    }
}

UINT MSI_SetFeatureStateW( MSIPACKAGE *package, LPCWSTR szFeature, INSTALLSTATE iState )
{
    UINT rc = ERROR_SUCCESS;
    MSIFEATURE *feature, *child;

    TRACE("%s %i\n", debugstr_w(szFeature), iState);

    feature = msi_get_loaded_feature( package, szFeature );
    if (!feature)
        return ERROR_UNKNOWN_FEATURE;

    if (iState == INSTALLSTATE_ADVERTISED && 
        feature->Attributes & msidbFeatureAttributesDisallowAdvertise)
        return ERROR_FUNCTION_FAILED;

    feature->ActionRequest = iState;

    ACTION_UpdateComponentStates( package, feature );

    /* update all the features that are children of this feature */
    LIST_FOR_EACH_ENTRY( child, &package->features, MSIFEATURE, entry )
    {
        if (child->Feature_Parent && !strcmpW( szFeature, child->Feature_Parent ))
            MSI_SetFeatureStateW(package, child->Feature, iState);
    }
    
    return rc;
}

/***********************************************************************
 * MsiSetFeatureStateW (MSI.@)
 */
UINT WINAPI MsiSetFeatureStateW(MSIHANDLE hInstall, LPCWSTR szFeature,
                                INSTALLSTATE iState)
{
    MSIPACKAGE* package;
    UINT rc = ERROR_SUCCESS;

    TRACE("%s %i\n",debugstr_w(szFeature), iState);

    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);
    if (!package)
    {
        HRESULT hr;
        BSTR feature;
        IWineMsiRemotePackage *remote_package;

        remote_package = (IWineMsiRemotePackage *)msi_get_remote(hInstall);
        if (!remote_package)
            return ERROR_INVALID_HANDLE;

        feature = SysAllocString(szFeature);
        if (!feature)
        {
            IWineMsiRemotePackage_Release(remote_package);
            return ERROR_OUTOFMEMORY;
        }

        hr = IWineMsiRemotePackage_SetFeatureState(remote_package, feature, iState);

        SysFreeString(feature);
        IWineMsiRemotePackage_Release(remote_package);

        if (FAILED(hr))
        {
            if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
                return HRESULT_CODE(hr);

            return ERROR_FUNCTION_FAILED;
        }

        return ERROR_SUCCESS;
    }

    rc = MSI_SetFeatureStateW(package,szFeature,iState);

    msiobj_release( &package->hdr );
    return rc;
}

/***********************************************************************
* MsiSetFeatureAttributesA   (MSI.@)
*/
UINT WINAPI MsiSetFeatureAttributesA( MSIHANDLE handle, LPCSTR feature, DWORD attrs )
{
    UINT r;
    WCHAR *featureW = NULL;

    TRACE("%u, %s, 0x%08x\n", handle, debugstr_a(feature), attrs);

    if (feature && !(featureW = strdupAtoW( feature ))) return ERROR_OUTOFMEMORY;

    r = MsiSetFeatureAttributesW( handle, featureW, attrs );
    msi_free( featureW );
    return r;
}

static DWORD unmap_feature_attributes( DWORD attrs )
{
    DWORD ret = 0;

    if (attrs & INSTALLFEATUREATTRIBUTE_FAVORLOCAL)             ret = msidbFeatureAttributesFavorLocal;
    if (attrs & INSTALLFEATUREATTRIBUTE_FAVORSOURCE)            ret |= msidbFeatureAttributesFavorSource;
    if (attrs & INSTALLFEATUREATTRIBUTE_FOLLOWPARENT)           ret |= msidbFeatureAttributesFollowParent;
    if (attrs & INSTALLFEATUREATTRIBUTE_FAVORADVERTISE)         ret |= msidbFeatureAttributesFavorAdvertise;
    if (attrs & INSTALLFEATUREATTRIBUTE_DISALLOWADVERTISE)      ret |= msidbFeatureAttributesDisallowAdvertise;
    if (attrs & INSTALLFEATUREATTRIBUTE_NOUNSUPPORTEDADVERTISE) ret |= msidbFeatureAttributesNoUnsupportedAdvertise;
    return ret;
}

/***********************************************************************
* MsiSetFeatureAttributesW   (MSI.@)
*/
UINT WINAPI MsiSetFeatureAttributesW( MSIHANDLE handle, LPCWSTR name, DWORD attrs )
{
    MSIPACKAGE *package;
    MSIFEATURE *feature;
    WCHAR *costing;

    TRACE("%u, %s, 0x%08x\n", handle, debugstr_w(name), attrs);

    if (!name || !name[0]) return ERROR_UNKNOWN_FEATURE;

    if (!(package = msihandle2msiinfo( handle, MSIHANDLETYPE_PACKAGE )))
        return ERROR_INVALID_HANDLE;

    costing = msi_dup_property( package->db, szCostingComplete );
    if (!costing || !strcmpW( costing, szOne ))
    {
        msi_free( costing );
        msiobj_release( &package->hdr );
        return ERROR_FUNCTION_FAILED;
    }
    msi_free( costing );
    if (!(feature = msi_get_loaded_feature( package, name )))
    {
        msiobj_release( &package->hdr );
        return ERROR_UNKNOWN_FEATURE;
    }
    feature->Attributes = unmap_feature_attributes( attrs );
    msiobj_release( &package->hdr );
    return ERROR_SUCCESS;
}

/***********************************************************************
* MsiGetFeatureStateA   (MSI.@)
*/
UINT WINAPI MsiGetFeatureStateA(MSIHANDLE hInstall, LPCSTR szFeature,
                  INSTALLSTATE *piInstalled, INSTALLSTATE *piAction)
{
    LPWSTR szwFeature = NULL;
    UINT rc;
    
    if (szFeature && !(szwFeature = strdupAtoW(szFeature))) return ERROR_OUTOFMEMORY;

    rc = MsiGetFeatureStateW(hInstall, szwFeature, piInstalled, piAction);
    msi_free( szwFeature);
    return rc;
}

UINT MSI_GetFeatureStateW(MSIPACKAGE *package, LPCWSTR szFeature,
                  INSTALLSTATE *piInstalled, INSTALLSTATE *piAction)
{
    MSIFEATURE *feature;

    feature = msi_get_loaded_feature(package,szFeature);
    if (!feature)
        return ERROR_UNKNOWN_FEATURE;

    if (piInstalled)
        *piInstalled = feature->Installed;

    if (piAction)
        *piAction = feature->ActionRequest;

    TRACE("returning %i %i\n", feature->Installed, feature->ActionRequest);

    return ERROR_SUCCESS;
}

/***********************************************************************
* MsiGetFeatureStateW   (MSI.@)
*/
UINT WINAPI MsiGetFeatureStateW(MSIHANDLE hInstall, LPCWSTR szFeature,
                  INSTALLSTATE *piInstalled, INSTALLSTATE *piAction)
{
    MSIPACKAGE* package;
    UINT ret;

    TRACE("%d %s %p %p\n", hInstall, debugstr_w(szFeature), piInstalled, piAction);

    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);
    if (!package)
    {
        HRESULT hr;
        BSTR feature;
        IWineMsiRemotePackage *remote_package;

        remote_package = (IWineMsiRemotePackage *)msi_get_remote(hInstall);
        if (!remote_package)
            return ERROR_INVALID_HANDLE;

        feature = SysAllocString(szFeature);
        if (!feature)
        {
            IWineMsiRemotePackage_Release(remote_package);
            return ERROR_OUTOFMEMORY;
        }

        hr = IWineMsiRemotePackage_GetFeatureState(remote_package, feature,
                                                   piInstalled, piAction);

        SysFreeString(feature);
        IWineMsiRemotePackage_Release(remote_package);

        if (FAILED(hr))
        {
            if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
                return HRESULT_CODE(hr);

            return ERROR_FUNCTION_FAILED;
        }

        return ERROR_SUCCESS;
    }

    ret = MSI_GetFeatureStateW(package, szFeature, piInstalled, piAction);
    msiobj_release( &package->hdr );
    return ret;
}

/***********************************************************************
* MsiGetFeatureCostA   (MSI.@)
*/
UINT WINAPI MsiGetFeatureCostA(MSIHANDLE hInstall, LPCSTR szFeature,
                  MSICOSTTREE iCostTree, INSTALLSTATE iState, LPINT piCost)
{
    LPWSTR szwFeature = NULL;
    UINT rc;

    szwFeature = strdupAtoW(szFeature);

    rc = MsiGetFeatureCostW(hInstall, szwFeature, iCostTree, iState, piCost);

    msi_free(szwFeature);

    return rc;
}

static INT feature_cost( MSIFEATURE *feature )
{
    INT cost = 0;
    MSICOMPONENT *comp;

    LIST_FOR_EACH_ENTRY( comp, &feature->Components, MSICOMPONENT, entry )
    {
        cost += comp->Cost;
    }
    return cost;
}

UINT MSI_GetFeatureCost( MSIPACKAGE *package, MSIFEATURE *feature, MSICOSTTREE tree,
                         INSTALLSTATE state, LPINT cost )
{
    TRACE("%s, %u, %d, %p\n", debugstr_w(feature->Feature), tree, state, cost);

    *cost = 0;
    switch (tree)
    {
    case MSICOSTTREE_CHILDREN:
    {
        MSIFEATURE *child;

        LIST_FOR_EACH_ENTRY( child, &feature->Children, MSIFEATURE, entry )
        {
            if (child->ActionRequest == state)
                *cost += feature_cost( child );
        }
        break;
    }
    case MSICOSTTREE_PARENTS:
    {
        const WCHAR *feature_parent = feature->Feature_Parent;
        for (;;)
        {
            MSIFEATURE *parent = msi_get_loaded_feature( package, feature_parent );
            if (!parent)
                break;

            if (parent->ActionRequest == state)
                *cost += feature_cost( parent );

            feature_parent = parent->Feature_Parent;
        }
        break;
    }
    case MSICOSTTREE_SELFONLY:
        if (feature->ActionRequest == state)
            *cost = feature_cost( feature );
        break;

    default:
        WARN("unhandled cost tree %u\n", tree);
        break;
    }

    *cost /= 512;
    return ERROR_SUCCESS;
}

/***********************************************************************
* MsiGetFeatureCostW   (MSI.@)
*/
UINT WINAPI MsiGetFeatureCostW(MSIHANDLE hInstall, LPCWSTR szFeature,
                  MSICOSTTREE iCostTree, INSTALLSTATE iState, LPINT piCost)
{
    MSIPACKAGE *package;
    MSIFEATURE *feature;
    UINT ret;

    TRACE("(%d %s %i %i %p)\n", hInstall, debugstr_w(szFeature),
          iCostTree, iState, piCost);

    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);
    if (!package)
    {
        HRESULT hr;
        BSTR feature;
        IWineMsiRemotePackage *remote_package;

        remote_package = (IWineMsiRemotePackage *)msi_get_remote(hInstall);
        if (!remote_package)
            return ERROR_INVALID_HANDLE;

        feature = SysAllocString(szFeature);
        if (!feature)
        {
            IWineMsiRemotePackage_Release(remote_package);
            return ERROR_OUTOFMEMORY;
        }

        hr = IWineMsiRemotePackage_GetFeatureCost(remote_package, feature,
                                                  iCostTree, iState, piCost);

        SysFreeString(feature);
        IWineMsiRemotePackage_Release(remote_package);

        if (FAILED(hr))
        {
            if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
                return HRESULT_CODE(hr);

            return ERROR_FUNCTION_FAILED;
        }

        return ERROR_SUCCESS;
    }

    feature = msi_get_loaded_feature(package, szFeature);

    if (feature)
        ret = MSI_GetFeatureCost(package, feature, iCostTree, iState, piCost);
    else
        ret = ERROR_UNKNOWN_FEATURE;

    msiobj_release( &package->hdr );
    return ret;
}

/***********************************************************************
* MsiGetFeatureInfoA   (MSI.@)
*/
UINT WINAPI MsiGetFeatureInfoA( MSIHANDLE handle, LPCSTR feature, LPDWORD attrs,
                                LPSTR title, LPDWORD title_len, LPSTR help, LPDWORD help_len )
{
    UINT r;
    WCHAR *titleW = NULL, *helpW = NULL, *featureW = NULL;

    TRACE("%u, %s, %p, %p, %p, %p, %p\n", handle, debugstr_a(feature), attrs, title,
          title_len, help, help_len);

    if (feature && !(featureW = strdupAtoW( feature ))) return ERROR_OUTOFMEMORY;

    if (title && title_len && !(titleW = msi_alloc( *title_len * sizeof(WCHAR) )))
    {
        msi_free( featureW );
        return ERROR_OUTOFMEMORY;
    }
    if (help && help_len && !(helpW = msi_alloc( *help_len * sizeof(WCHAR) )))
    {
        msi_free( featureW );
        msi_free( titleW );
        return ERROR_OUTOFMEMORY;
    }
    r = MsiGetFeatureInfoW( handle, featureW, attrs, titleW, title_len, helpW, help_len );
    if (r == ERROR_SUCCESS)
    {
        if (titleW) WideCharToMultiByte( CP_ACP, 0, titleW, -1, title, *title_len + 1, NULL, NULL );
        if (helpW) WideCharToMultiByte( CP_ACP, 0, helpW, -1, help, *help_len + 1, NULL, NULL );
    }
    msi_free( titleW );
    msi_free( helpW );
    msi_free( featureW );
    return r;
}

static DWORD map_feature_attributes( DWORD attrs )
{
    DWORD ret = 0;

    if (attrs == msidbFeatureAttributesFavorLocal)            ret |= INSTALLFEATUREATTRIBUTE_FAVORLOCAL;
    if (attrs & msidbFeatureAttributesFavorSource)            ret |= INSTALLFEATUREATTRIBUTE_FAVORSOURCE;
    if (attrs & msidbFeatureAttributesFollowParent)           ret |= INSTALLFEATUREATTRIBUTE_FOLLOWPARENT;
    if (attrs & msidbFeatureAttributesFavorAdvertise)         ret |= INSTALLFEATUREATTRIBUTE_FAVORADVERTISE;
    if (attrs & msidbFeatureAttributesDisallowAdvertise)      ret |= INSTALLFEATUREATTRIBUTE_DISALLOWADVERTISE;
    if (attrs & msidbFeatureAttributesNoUnsupportedAdvertise) ret |= INSTALLFEATUREATTRIBUTE_NOUNSUPPORTEDADVERTISE;
    return ret;
}

static UINT MSI_GetFeatureInfo( MSIPACKAGE *package, LPCWSTR name, LPDWORD attrs,
                                LPWSTR title, LPDWORD title_len, LPWSTR help, LPDWORD help_len )
{
    UINT r = ERROR_SUCCESS;
    MSIFEATURE *feature = msi_get_loaded_feature( package, name );
    int len;

    if (!feature) return ERROR_UNKNOWN_FEATURE;
    if (attrs) *attrs = map_feature_attributes( feature->Attributes );
    if (title_len)
    {
        if (feature->Title) len = strlenW( feature->Title );
        else len = 0;
        if (*title_len <= len)
        {
            *title_len = len;
            if (title) r = ERROR_MORE_DATA;
        }
        else if (title)
        {
            if (feature->Title) strcpyW( title, feature->Title );
            else *title = 0;
            *title_len = len;
        }
    }
    if (help_len)
    {
        if (feature->Description) len = strlenW( feature->Description );
        else len = 0;
        if (*help_len <= len)
        {
            *help_len = len;
            if (help) r = ERROR_MORE_DATA;
        }
        else if (help)
        {
            if (feature->Description) strcpyW( help, feature->Description );
            else *help = 0;
            *help_len = len;
        }
    }
    return r;
}

/***********************************************************************
* MsiGetFeatureInfoW   (MSI.@)
*/
UINT WINAPI MsiGetFeatureInfoW( MSIHANDLE handle, LPCWSTR feature, LPDWORD attrs,
                                LPWSTR title, LPDWORD title_len, LPWSTR help, LPDWORD help_len )
{
    UINT r;
    MSIPACKAGE *package;

    TRACE("%u, %s, %p, %p, %p, %p, %p\n", handle, debugstr_w(feature), attrs, title,
          title_len, help, help_len);

    if (!feature) return ERROR_INVALID_PARAMETER;

    if (!(package = msihandle2msiinfo( handle, MSIHANDLETYPE_PACKAGE )))
        return ERROR_INVALID_HANDLE;

    /* features may not have been loaded yet */
    msi_load_all_components( package );
    msi_load_all_features( package );

    r = MSI_GetFeatureInfo( package, feature, attrs, title, title_len, help, help_len );
    msiobj_release( &package->hdr );
    return r;
}

/***********************************************************************
 * MsiSetComponentStateA (MSI.@)
 */
UINT WINAPI MsiSetComponentStateA(MSIHANDLE hInstall, LPCSTR szComponent,
                                  INSTALLSTATE iState)
{
    UINT rc;
    LPWSTR szwComponent = strdupAtoW(szComponent);

    rc = MsiSetComponentStateW(hInstall, szwComponent, iState);

    msi_free(szwComponent);

    return rc;
}

/***********************************************************************
 * MsiGetComponentStateA (MSI.@)
 */
UINT WINAPI MsiGetComponentStateA(MSIHANDLE hInstall, LPCSTR szComponent,
                  INSTALLSTATE *piInstalled, INSTALLSTATE *piAction)
{
    LPWSTR szwComponent= NULL;
    UINT rc;
    
    szwComponent= strdupAtoW(szComponent);

    rc = MsiGetComponentStateW(hInstall,szwComponent,piInstalled, piAction);

    msi_free( szwComponent);

    return rc;
}

static UINT MSI_SetComponentStateW(MSIPACKAGE *package, LPCWSTR szComponent,
                                   INSTALLSTATE iState)
{
    MSICOMPONENT *comp;

    TRACE("%p %s %d\n", package, debugstr_w(szComponent), iState);

    comp = msi_get_loaded_component(package, szComponent);
    if (!comp)
        return ERROR_UNKNOWN_COMPONENT;

    if (comp->Enabled)
        comp->Action = iState;

    return ERROR_SUCCESS;
}

UINT MSI_GetComponentStateW(MSIPACKAGE *package, LPCWSTR szComponent,
                  INSTALLSTATE *piInstalled, INSTALLSTATE *piAction)
{
    MSICOMPONENT *comp;

    TRACE("%p %s %p %p\n", package, debugstr_w(szComponent),
           piInstalled, piAction);

    comp = msi_get_loaded_component(package,szComponent);
    if (!comp)
        return ERROR_UNKNOWN_COMPONENT;

    if (piInstalled)
    {
        if (comp->Enabled)
            *piInstalled = comp->Installed;
        else
            *piInstalled = INSTALLSTATE_UNKNOWN;
    }

    if (piAction)
    {
        if (comp->Enabled)
            *piAction = comp->Action;
        else
            *piAction = INSTALLSTATE_UNKNOWN;
    }

    TRACE("states (%i, %i)\n", comp->Installed, comp->Action );
    return ERROR_SUCCESS;
}

/***********************************************************************
 * MsiSetComponentStateW (MSI.@)
 */
UINT WINAPI MsiSetComponentStateW(MSIHANDLE hInstall, LPCWSTR szComponent,
                                  INSTALLSTATE iState)
{
    MSIPACKAGE* package;
    UINT ret;

    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);
    if (!package)
    {
        HRESULT hr;
        BSTR component;
        IWineMsiRemotePackage *remote_package;

        remote_package = (IWineMsiRemotePackage *)msi_get_remote(hInstall);
        if (!remote_package)
            return ERROR_INVALID_HANDLE;

        component = SysAllocString(szComponent);
        if (!component)
        {
            IWineMsiRemotePackage_Release(remote_package);
            return ERROR_OUTOFMEMORY;
        }

        hr = IWineMsiRemotePackage_SetComponentState(remote_package, component, iState);

        SysFreeString(component);
        IWineMsiRemotePackage_Release(remote_package);

        if (FAILED(hr))
        {
            if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
                return HRESULT_CODE(hr);

            return ERROR_FUNCTION_FAILED;
        }

        return ERROR_SUCCESS;
    }

    ret = MSI_SetComponentStateW(package, szComponent, iState);
    msiobj_release(&package->hdr);
    return ret;
}

/***********************************************************************
 * MsiGetComponentStateW (MSI.@)
 */
UINT WINAPI MsiGetComponentStateW(MSIHANDLE hInstall, LPCWSTR szComponent,
                  INSTALLSTATE *piInstalled, INSTALLSTATE *piAction)
{
    MSIPACKAGE* package;
    UINT ret;

    TRACE("%d %s %p %p\n", hInstall, debugstr_w(szComponent),
           piInstalled, piAction);

    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);
    if (!package)
    {
        HRESULT hr;
        BSTR component;
        IWineMsiRemotePackage *remote_package;

        remote_package = (IWineMsiRemotePackage *)msi_get_remote(hInstall);
        if (!remote_package)
            return ERROR_INVALID_HANDLE;

        component = SysAllocString(szComponent);
        if (!component)
        {
            IWineMsiRemotePackage_Release(remote_package);
            return ERROR_OUTOFMEMORY;
        }

        hr = IWineMsiRemotePackage_GetComponentState(remote_package, component,
                                                     piInstalled, piAction);

        SysFreeString(component);
        IWineMsiRemotePackage_Release(remote_package);

        if (FAILED(hr))
        {
            if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
                return HRESULT_CODE(hr);

            return ERROR_FUNCTION_FAILED;
        }

        return ERROR_SUCCESS;
    }

    ret = MSI_GetComponentStateW( package, szComponent, piInstalled, piAction);
    msiobj_release( &package->hdr );
    return ret;
}

/***********************************************************************
 * MsiGetLanguage (MSI.@)
 */
LANGID WINAPI MsiGetLanguage(MSIHANDLE hInstall)
{
    MSIPACKAGE* package;
    LANGID langid;

    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);
    if (!package)
    {
        HRESULT hr;
        LANGID lang;
        IWineMsiRemotePackage *remote_package;

        remote_package = (IWineMsiRemotePackage *)msi_get_remote(hInstall);
        if (!remote_package)
            return ERROR_INVALID_HANDLE;

        hr = IWineMsiRemotePackage_GetLanguage(remote_package, &lang);

        if (SUCCEEDED(hr))
            return lang;

        return 0;
    }

    langid = msi_get_property_int( package->db, szProductLanguage, 0 );
    msiobj_release( &package->hdr );
    return langid;
}

UINT MSI_SetInstallLevel( MSIPACKAGE *package, int iInstallLevel )
{
    static const WCHAR fmt[] = { '%','d',0 };
    WCHAR level[6];
    int len;
    UINT r;

    TRACE("%p %i\n", package, iInstallLevel);

    if (iInstallLevel > 32767)
        return ERROR_INVALID_PARAMETER;

    if (iInstallLevel < 1)
        return MSI_SetFeatureStates( package );

    len = sprintfW( level, fmt, iInstallLevel );
    r = msi_set_property( package->db, szInstallLevel, level, len );
    if ( r == ERROR_SUCCESS )
        r = MSI_SetFeatureStates( package );

    return r;
}

/***********************************************************************
 * MsiSetInstallLevel (MSI.@)
 */
UINT WINAPI MsiSetInstallLevel(MSIHANDLE hInstall, int iInstallLevel)
{
    MSIPACKAGE* package;
    UINT r;

    TRACE("%d %i\n", hInstall, iInstallLevel);

    package = msihandle2msiinfo(hInstall, MSIHANDLETYPE_PACKAGE);
    if (!package)
    {
        HRESULT hr;
        IWineMsiRemotePackage *remote_package;

        remote_package = (IWineMsiRemotePackage *)msi_get_remote(hInstall);
        if (!remote_package)
            return ERROR_INVALID_HANDLE;

        hr = IWineMsiRemotePackage_SetInstallLevel(remote_package, iInstallLevel);

        IWineMsiRemotePackage_Release(remote_package);

        if (FAILED(hr))
        {
            if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
                return HRESULT_CODE(hr);

            return ERROR_FUNCTION_FAILED;
        }

        return ERROR_SUCCESS;
    }

    r = MSI_SetInstallLevel( package, iInstallLevel );

    msiobj_release( &package->hdr );

    return r;
}

/***********************************************************************
 * MsiGetFeatureValidStatesW (MSI.@)
 */
UINT WINAPI MsiGetFeatureValidStatesW(MSIHANDLE hInstall, LPCWSTR szFeature,
                  LPDWORD pInstallState)
{
    if(pInstallState) *pInstallState = 1<<INSTALLSTATE_LOCAL;
    FIXME("%d %s %p stub returning %d\n",
        hInstall, debugstr_w(szFeature), pInstallState, pInstallState ? *pInstallState : 0);

    return ERROR_SUCCESS;
}

/***********************************************************************
 * MsiGetFeatureValidStatesA (MSI.@)
 */
UINT WINAPI MsiGetFeatureValidStatesA(MSIHANDLE hInstall, LPCSTR szFeature,
                  LPDWORD pInstallState)
{
    UINT ret;
    LPWSTR szwFeature = strdupAtoW(szFeature);

    ret = MsiGetFeatureValidStatesW(hInstall, szwFeature, pInstallState);

    msi_free(szwFeature);

    return ret;
}
