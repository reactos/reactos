/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2010 Hans Leidekker for CodeWeavers
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wine/debug.h"
#include "msipriv.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

static void load_fusion_dlls( MSIPACKAGE *package )
{
    HRESULT (WINAPI *pLoadLibraryShim)( const WCHAR *, const WCHAR *, void *, HMODULE * );
    WCHAR path[MAX_PATH];
    DWORD len = GetSystemDirectoryW( path, MAX_PATH );

    lstrcpyW( path + len, L"\\mscoree.dll" );
    if (!package->hmscoree && !(package->hmscoree = LoadLibraryW( path ))) return;
    if (!(pLoadLibraryShim = (void *)GetProcAddress( package->hmscoree, "LoadLibraryShim" )))
    {
        FreeLibrary( package->hmscoree );
        package->hmscoree = NULL;
        return;
    }

    if (!package->hfusion10) pLoadLibraryShim( L"fusion.dll", L"v1.0.3705", NULL, &package->hfusion10 );
    if (!package->hfusion11) pLoadLibraryShim( L"fusion.dll", L"v1.1.4322", NULL, &package->hfusion11 );
    if (!package->hfusion20) pLoadLibraryShim( L"fusion.dll", L"v2.0.50727", NULL, &package->hfusion20 );
    if (!package->hfusion40) pLoadLibraryShim( L"fusion.dll", L"v4.0.30319", NULL, &package->hfusion40 );
}

static BOOL init_assembly_caches( MSIPACKAGE *package )
{
    HRESULT (WINAPI *pCreateAssemblyCache)( IAssemblyCache **, DWORD );

    if (!package->cache_sxs && CreateAssemblyCache( &package->cache_sxs, 0 ) != S_OK) return FALSE;

    load_fusion_dlls( package );
    package->pGetFileVersion = (void *)GetProcAddress( package->hmscoree, "GetFileVersion" ); /* missing from v1.0.3705 */

    if (package->hfusion10 && !package->cache_net[CLR_VERSION_V10])
    {
        pCreateAssemblyCache = (void *)GetProcAddress( package->hfusion10, "CreateAssemblyCache" );
        pCreateAssemblyCache( &package->cache_net[CLR_VERSION_V10], 0 );
    }
    if (package->hfusion11 && !package->cache_net[CLR_VERSION_V11])
    {
        pCreateAssemblyCache = (void *)GetProcAddress( package->hfusion11, "CreateAssemblyCache" );
        pCreateAssemblyCache( &package->cache_net[CLR_VERSION_V11], 0 );
    }
    if (package->hfusion20 && !package->cache_net[CLR_VERSION_V20])
    {
        pCreateAssemblyCache = (void *)GetProcAddress( package->hfusion20, "CreateAssemblyCache" );
        pCreateAssemblyCache( &package->cache_net[CLR_VERSION_V20], 0 );
    }
    if (package->hfusion40 && !package->cache_net[CLR_VERSION_V40])
    {
        pCreateAssemblyCache = (void *)GetProcAddress( package->hfusion40, "CreateAssemblyCache" );
        pCreateAssemblyCache( &package->cache_net[CLR_VERSION_V40], 0 );
        package->pCreateAssemblyNameObject = (void *)GetProcAddress( package->hfusion40, "CreateAssemblyNameObject" );
        package->pCreateAssemblyEnum = (void *)GetProcAddress( package->hfusion40, "CreateAssemblyEnum" );
    }

    return TRUE;
}

void msi_destroy_assembly_caches( MSIPACKAGE *package )
{
    UINT i;

    if (package->cache_sxs)
    {
        IAssemblyCache_Release( package->cache_sxs );
        package->cache_sxs = NULL;
    }
    for (i = 0; i < CLR_VERSION_MAX; i++)
    {
        if (package->cache_net[i])
        {
            IAssemblyCache_Release( package->cache_net[i] );
            package->cache_net[i] = NULL;
        }
    }
    package->pGetFileVersion = NULL;
    package->pCreateAssemblyNameObject = NULL;
    package->pCreateAssemblyEnum = NULL;
    FreeLibrary( package->hfusion10 );
    FreeLibrary( package->hfusion11 );
    FreeLibrary( package->hfusion20 );
    FreeLibrary( package->hfusion40 );
    FreeLibrary( package->hmscoree );
    package->hfusion10 = NULL;
    package->hfusion11 = NULL;
    package->hfusion20 = NULL;
    package->hfusion40 = NULL;
    package->hmscoree = NULL;
}

static MSIRECORD *get_assembly_record( MSIPACKAGE *package, const WCHAR *comp )
{
    MSIQUERY *view;
    MSIRECORD *rec;
    UINT r;

    r = MSI_OpenQuery( package->db, &view, L"SELECT * FROM `MsiAssembly` WHERE `Component_` = '%s'", comp );
    if (r != ERROR_SUCCESS)
        return NULL;

    r = MSI_ViewExecute( view, NULL );
    if (r != ERROR_SUCCESS)
    {
        msiobj_release( &view->hdr );
        return NULL;
    }
    r = MSI_ViewFetch( view, &rec );
    if (r != ERROR_SUCCESS)
    {
        msiobj_release( &view->hdr );
        return NULL;
    }
    if (!MSI_RecordGetString( rec, 4 ))
        TRACE("component is a global assembly\n");

    msiobj_release( &view->hdr );
    return rec;
}

struct assembly_name
{
    DWORD   count;
    UINT    index;
    WCHAR **attrs;
};

static UINT get_assembly_name_attribute( MSIRECORD *rec, LPVOID param )
{
    struct assembly_name *name = param;
    const WCHAR *attr = MSI_RecordGetString( rec, 2 );
    const WCHAR *value = MSI_RecordGetString( rec, 3 );
    int len = lstrlenW( L"%s=\"%s\"" ) + lstrlenW( attr ) + lstrlenW( value );

    if (!(name->attrs[name->index] = malloc( len * sizeof(WCHAR) )))
        return ERROR_OUTOFMEMORY;

    if (!wcsicmp( attr, L"name" )) lstrcpyW( name->attrs[name->index++], value );
    else swprintf( name->attrs[name->index++], len, L"%s=\"%s\"", attr, value );
    return ERROR_SUCCESS;
}

static WCHAR *get_assembly_display_name( MSIDATABASE *db, const WCHAR *comp, MSIASSEMBLY *assembly )
{
    struct assembly_name name;
    WCHAR *display_name = NULL;
    MSIQUERY *view;
    UINT i, r;
    int len;

    r = MSI_OpenQuery( db, &view, L"SELECT * FROM `MsiAssemblyName` WHERE `Component_` = '%s'", comp );
    if (r != ERROR_SUCCESS)
        return NULL;

    name.count = 0;
    name.index = 0;
    name.attrs = NULL;
    MSI_IterateRecords( view, &name.count, NULL, NULL );
    if (!name.count) goto done;

    name.attrs = malloc( name.count * sizeof(WCHAR *) );
    if (!name.attrs) goto done;

    MSI_IterateRecords( view, NULL, get_assembly_name_attribute, &name );

    len = 0;
    for (i = 0; i < name.count; i++) len += lstrlenW( name.attrs[i] ) + 1;

    display_name = malloc( (len + 1) * sizeof(WCHAR) );
    if (display_name)
    {
        display_name[0] = 0;
        for (i = 0; i < name.count; i++)
        {
            lstrcatW( display_name, name.attrs[i] );
            if (i < name.count - 1) lstrcatW( display_name, L"," );
        }
    }

done:
    msiobj_release( &view->hdr );
    if (name.attrs)
    {
        for (i = 0; i < name.count; i++) free( name.attrs[i] );
        free( name.attrs );
    }
    return display_name;
}

WCHAR *msi_get_assembly_path( MSIPACKAGE *package, const WCHAR *displayname )
{
    HRESULT hr;
    ASSEMBLY_INFO info;
    IAssemblyCache *cache;

    if (!init_assembly_caches( package ) || !(cache = package->cache_net[CLR_VERSION_V40])) return NULL;

    memset( &info, 0, sizeof(info) );
    info.cbAssemblyInfo = sizeof(info);
    hr = IAssemblyCache_QueryAssemblyInfo( cache, 0, displayname, &info );
    if (hr != E_NOT_SUFFICIENT_BUFFER) return NULL;

    if (!(info.pszCurrentAssemblyPathBuf = malloc( info.cchBuf * sizeof(WCHAR) ))) return NULL;

    hr = IAssemblyCache_QueryAssemblyInfo( cache, 0, displayname, &info );
    if (FAILED( hr ))
    {
        free( info.pszCurrentAssemblyPathBuf );
        return NULL;
    }
    TRACE("returning %s\n", debugstr_w(info.pszCurrentAssemblyPathBuf));
    return info.pszCurrentAssemblyPathBuf;
}

IAssemblyEnum *msi_create_assembly_enum( MSIPACKAGE *package, const WCHAR *displayname )
{
    HRESULT hr;
    IAssemblyName *name;
    IAssemblyEnum *ret;
    WCHAR *str;
    DWORD len = 0;

    if (!init_assembly_caches( package ) || !package->pCreateAssemblyNameObject || !package->pCreateAssemblyEnum)
        return NULL;

    hr = package->pCreateAssemblyNameObject( &name, displayname, CANOF_PARSE_DISPLAY_NAME, NULL );
    if (FAILED( hr )) return NULL;

    hr = IAssemblyName_GetName( name, &len, NULL );
    if (hr != E_NOT_SUFFICIENT_BUFFER || !(str = malloc( len * sizeof(WCHAR) )))
    {
        IAssemblyName_Release( name );
        return NULL;
    }

    hr = IAssemblyName_GetName( name, &len, str );
    IAssemblyName_Release( name );
    if (FAILED( hr ))
    {
        free( str );
        return NULL;
    }

    hr = package->pCreateAssemblyNameObject( &name, str, 0, NULL );
    free( str );
    if (FAILED( hr )) return NULL;

    hr = package->pCreateAssemblyEnum( &ret, NULL, name, ASM_CACHE_GAC, NULL );
    IAssemblyName_Release( name );
    if (FAILED( hr )) return NULL;

    return ret;
}

static const WCHAR *clr_version[] =
{
    L"v1.0.3705",
    L"v1.2.4322",
    L"v2.0.50727",
    L"v4.0.30319"
};

MSIASSEMBLY *msi_load_assembly( MSIPACKAGE *package, MSICOMPONENT *comp )
{
    MSIRECORD *rec;
    MSIASSEMBLY *a;

    if (!(rec = get_assembly_record( package, comp->Component ))) return NULL;
    if (!(a = calloc( 1, sizeof(MSIASSEMBLY) )))
    {
        msiobj_release( &rec->hdr );
        return NULL;
    }
    a->feature = wcsdup( MSI_RecordGetString( rec, 2 ) );
    TRACE("feature %s\n", debugstr_w(a->feature));

    a->manifest = wcsdup( MSI_RecordGetString( rec, 3 ) );
    TRACE("manifest %s\n", debugstr_w(a->manifest));

    a->application = wcsdup( MSI_RecordGetString( rec, 4 ) );
    TRACE("application %s\n", debugstr_w(a->application));

    a->attributes = MSI_RecordGetInteger( rec, 5 );
    TRACE( "attributes %lu\n", a->attributes );

    if (!(a->display_name = get_assembly_display_name( package->db, comp->Component, a )))
    {
        WARN("can't get display name\n");
        msiobj_release( &rec->hdr );
        free( a->feature );
        free( a->manifest );
        free( a->application );
        free( a );
        return NULL;
    }
    TRACE("display name %s\n", debugstr_w(a->display_name));

    msiobj_release( &rec->hdr );
    return a;
}

static enum clr_version get_clr_version( MSIPACKAGE *package, const WCHAR *filename )
{
    DWORD len;
    HRESULT hr;
    enum clr_version version = CLR_VERSION_V11;
    WCHAR *strW;

    if (!package->pGetFileVersion) return CLR_VERSION_V10;

    hr = package->pGetFileVersion( filename, NULL, 0, &len );
    if (hr != E_NOT_SUFFICIENT_BUFFER) return CLR_VERSION_V11;
    if ((strW = malloc( len * sizeof(WCHAR) )))
    {
        hr = package->pGetFileVersion( filename, strW, len, &len );
        if (hr == S_OK)
        {
            UINT i;
            for (i = 0; i < CLR_VERSION_MAX; i++)
                if (!wcscmp( strW, clr_version[i] )) version = i;
        }
        free( strW );
    }
    return version;
}

UINT msi_install_assembly( MSIPACKAGE *package, MSICOMPONENT *comp )
{
    HRESULT hr;
    const WCHAR *manifest;
    IAssemblyCache *cache;
    MSIASSEMBLY *assembly = comp->assembly;
    MSIFEATURE *feature = NULL;

    if (!init_assembly_caches( package )) return ERROR_FUNCTION_FAILED;

    if (comp->assembly->feature)
        feature = msi_get_loaded_feature( package, comp->assembly->feature );

    if (assembly->application)
    {
        if (feature) feature->Action = INSTALLSTATE_LOCAL;
        return ERROR_SUCCESS;
    }
    if (assembly->attributes == msidbAssemblyAttributesWin32)
    {
        if (!assembly->manifest)
        {
            WARN("no manifest\n");
            return ERROR_FUNCTION_FAILED;
        }
        manifest = msi_get_loaded_file( package, assembly->manifest )->TargetPath;
        cache = package->cache_sxs;
    }
    else
    {
        manifest = msi_get_loaded_file( package, comp->KeyPath )->TargetPath;
        cache = package->cache_net[get_clr_version( package, manifest )];
        if (!cache) return ERROR_SUCCESS;
    }
    TRACE("installing assembly %s\n", debugstr_w(manifest));

    hr = IAssemblyCache_InstallAssembly( cache, 0, manifest, NULL );
    if (hr != S_OK)
    {
        ERR( "failed to install assembly %s (%#lx)\n", debugstr_w(manifest), hr );
        return ERROR_FUNCTION_FAILED;
    }
    if (feature) feature->Action = INSTALLSTATE_LOCAL;
    return ERROR_SUCCESS;
}

UINT msi_uninstall_assembly( MSIPACKAGE *package, MSICOMPONENT *comp )
{
    HRESULT hr;
    IAssemblyCache *cache;
    MSIASSEMBLY *assembly = comp->assembly;
    MSIFEATURE *feature = NULL;

    if (!init_assembly_caches( package )) return ERROR_FUNCTION_FAILED;

    if (comp->assembly->feature)
        feature = msi_get_loaded_feature( package, comp->assembly->feature );

    if (assembly->application)
    {
        if (feature) feature->Action = INSTALLSTATE_ABSENT;
        return ERROR_SUCCESS;
    }
    TRACE("removing %s\n", debugstr_w(assembly->display_name));

    if (assembly->attributes == msidbAssemblyAttributesWin32)
    {
        cache = package->cache_sxs;
        hr = IAssemblyCache_UninstallAssembly( cache, 0, assembly->display_name, NULL, NULL );
        if (FAILED( hr )) WARN( "failed to uninstall assembly %#lx\n", hr );
    }
    else
    {
        unsigned int i;
        for (i = 0; i < CLR_VERSION_MAX; i++)
        {
            if (!assembly->clr_version[i]) continue;
            cache = package->cache_net[i];
            if (cache)
            {
                hr = IAssemblyCache_UninstallAssembly( cache, 0, assembly->display_name, NULL, NULL );
                if (FAILED( hr )) WARN( "failed to uninstall assembly %#lx\n", hr );
            }
        }
    }
    if (feature) feature->Action = INSTALLSTATE_ABSENT;
    return ERROR_SUCCESS;
}

static WCHAR *build_local_assembly_path( const WCHAR *filename )
{
    UINT i;
    WCHAR *ret;

    if (!(ret = malloc( (wcslen( filename ) + 1) * sizeof(WCHAR) )))
        return NULL;

    for (i = 0; filename[i]; i++)
    {
        if (filename[i] == '\\' || filename[i] == '/') ret[i] = '|';
        else ret[i] = filename[i];
    }
    ret[i] = 0;
    return ret;
}

static LONG open_assemblies_key( UINT context, BOOL win32, HKEY *hkey )
{
    HKEY root;
    const WCHAR *path;

    if (context == MSIINSTALLCONTEXT_MACHINE)
    {
        root = HKEY_CLASSES_ROOT;
        if (win32) path = L"Installer\\Win32Assemblies\\";
        else path = L"Installer\\Assemblies\\";
    }
    else
    {
        root = HKEY_CURRENT_USER;
        if (win32) path = L"Software\\Microsoft\\Installer\\Win32Assemblies\\";
        else path = L"Software\\Microsoft\\Installer\\Assemblies\\";
    }
    return RegCreateKeyW( root, path, hkey );
}

static LONG open_local_assembly_key( UINT context, BOOL win32, const WCHAR *filename, HKEY *hkey )
{
    LONG res;
    HKEY root;
    WCHAR *path;

    if (!(path = build_local_assembly_path( filename )))
        return ERROR_OUTOFMEMORY;

    if ((res = open_assemblies_key( context, win32, &root )))
    {
        free( path );
        return res;
    }
    res = RegCreateKeyW( root, path, hkey );
    RegCloseKey( root );
    free( path );
    return res;
}

static LONG delete_local_assembly_key( UINT context, BOOL win32, const WCHAR *filename )
{
    LONG res;
    HKEY root;
    WCHAR *path;

    if (!(path = build_local_assembly_path( filename )))
        return ERROR_OUTOFMEMORY;

    if ((res = open_assemblies_key( context, win32, &root )))
    {
        free( path );
        return res;
    }
    res = RegDeleteKeyW( root, path );
    RegCloseKey( root );
    free( path );
    return res;
}

static LONG open_global_assembly_key( UINT context, BOOL win32, HKEY *hkey )
{
    HKEY root;
    const WCHAR *path;

    if (context == MSIINSTALLCONTEXT_MACHINE)
    {
        root = HKEY_CLASSES_ROOT;
        if (win32) path = L"Installer\\Win32Assemblies\\Global";
        else path = L"Installer\\Assemblies\\Global";
    }
    else
    {
        root = HKEY_CURRENT_USER;
        if (win32) path = L"Software\\Microsoft\\Installer\\Win32Assemblies\\Global";
        else path = L"Software\\Microsoft\\Installer\\Assemblies\\Global";
    }
    return RegCreateKeyW( root, path, hkey );
}

UINT ACTION_MsiPublishAssemblies( MSIPACKAGE *package )
{
    MSICOMPONENT *comp;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"MsiPublishAssemblies");

    LIST_FOR_EACH_ENTRY(comp, &package->components, MSICOMPONENT, entry)
    {
        LONG res;
        HKEY hkey;
        GUID guid;
        DWORD size;
        WCHAR buffer[43];
        MSIRECORD *uirow;
        MSIASSEMBLY *assembly = comp->assembly;
        BOOL win32;

        if (!assembly || !comp->ComponentId) continue;

        comp->Action = msi_get_component_action( package, comp );
        if (comp->Action != INSTALLSTATE_LOCAL)
        {
            TRACE("component not scheduled for installation %s\n", debugstr_w(comp->Component));
            continue;
        }
        TRACE("publishing %s\n", debugstr_w(comp->Component));

        CLSIDFromString( package->ProductCode, &guid );
        encode_base85_guid( &guid, buffer );
        buffer[20] = '>';
        CLSIDFromString( comp->ComponentId, &guid );
        encode_base85_guid( &guid, buffer + 21 );
        buffer[42] = 0;

        win32 = assembly->attributes & msidbAssemblyAttributesWin32;
        if (assembly->application)
        {
            MSIFILE *file = msi_get_loaded_file( package, assembly->application );
            if (!file)
            {
                WARN("no matching file %s for local assembly\n", debugstr_w(assembly->application));
                continue;
            }
            if ((res = open_local_assembly_key( package->Context, win32, file->TargetPath, &hkey )))
            {
                WARN( "failed to open local assembly key %ld\n", res );
                return ERROR_FUNCTION_FAILED;
            }
        }
        else
        {
            if ((res = open_global_assembly_key( package->Context, win32, &hkey )))
            {
                WARN( "failed to open global assembly key %ld\n", res );
                return ERROR_FUNCTION_FAILED;
            }
        }
        size = sizeof(buffer);
        if ((res = RegSetValueExW( hkey, assembly->display_name, 0, REG_MULTI_SZ, (const BYTE *)buffer, size )))
        {
            WARN( "failed to set assembly value %ld\n", res );
        }
        RegCloseKey( hkey );

        uirow = MSI_CreateRecord( 2 );
        MSI_RecordSetStringW( uirow, 2, assembly->display_name );
        MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
        msiobj_release( &uirow->hdr );
    }
    return ERROR_SUCCESS;
}

UINT ACTION_MsiUnpublishAssemblies( MSIPACKAGE *package )
{
    MSICOMPONENT *comp;

    if (package->script == SCRIPT_NONE)
        return msi_schedule_action(package, SCRIPT_INSTALL, L"MsiUnpublishAssemblies");

    LIST_FOR_EACH_ENTRY(comp, &package->components, MSICOMPONENT, entry)
    {
        LONG res;
        MSIRECORD *uirow;
        MSIASSEMBLY *assembly = comp->assembly;
        BOOL win32;

        if (!assembly || !comp->ComponentId) continue;

        comp->Action = msi_get_component_action( package, comp );
        if (comp->Action != INSTALLSTATE_ABSENT)
        {
            TRACE("component not scheduled for removal %s\n", debugstr_w(comp->Component));
            continue;
        }
        TRACE("unpublishing %s\n", debugstr_w(comp->Component));

        win32 = assembly->attributes & msidbAssemblyAttributesWin32;
        if (assembly->application)
        {
            MSIFILE *file = msi_get_loaded_file( package, assembly->application );
            if (!file)
            {
                WARN("no matching file %s for local assembly\n", debugstr_w(assembly->application));
                continue;
            }
            if ((res = delete_local_assembly_key( package->Context, win32, file->TargetPath )))
                WARN( "failed to delete local assembly key %ld\n", res );
        }
        else
        {
            HKEY hkey;
            if ((res = open_global_assembly_key( package->Context, win32, &hkey )))
                WARN( "failed to delete global assembly key %ld\n", res );
            else
            {
                if ((res = RegDeleteValueW( hkey, assembly->display_name )))
                    WARN( "failed to delete global assembly value %ld\n", res );
                RegCloseKey( hkey );
            }
        }

        uirow = MSI_CreateRecord( 2 );
        MSI_RecordSetStringW( uirow, 2, assembly->display_name );
        MSI_ProcessMessage(package, INSTALLMESSAGE_ACTIONDATA, uirow);
        msiobj_release( &uirow->hdr );
    }
    return ERROR_SUCCESS;
}
