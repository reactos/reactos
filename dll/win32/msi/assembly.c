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

#include "msipriv.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

static HRESULT (WINAPI *pCreateAssemblyCacheNet10)( IAssemblyCache **, DWORD );
static HRESULT (WINAPI *pCreateAssemblyCacheNet11)( IAssemblyCache **, DWORD );
static HRESULT (WINAPI *pCreateAssemblyCacheNet20)( IAssemblyCache **, DWORD );
static HRESULT (WINAPI *pCreateAssemblyCacheNet40)( IAssemblyCache **, DWORD );
static HRESULT (WINAPI *pCreateAssemblyCacheSxs)( IAssemblyCache **, DWORD );
static HRESULT (WINAPI *pLoadLibraryShim)( LPCWSTR, LPCWSTR, LPVOID, HMODULE * );
static HRESULT (WINAPI *pGetFileVersion)( LPCWSTR, LPWSTR, DWORD, DWORD * );
static HRESULT (WINAPI *pCreateAssemblyNameObject)( IAssemblyName **, LPCWSTR, DWORD, LPVOID );
static HRESULT (WINAPI *pCreateAssemblyEnum)( IAssemblyEnum **, IUnknown *, IAssemblyName *, DWORD, LPVOID );

static HMODULE hfusion10, hfusion11, hfusion20, hfusion40, hmscoree, hsxs;

static BOOL init_function_pointers( void )
{
    static const WCHAR szFusion[]    = {'f','u','s','i','o','n','.','d','l','l',0};
    static const WCHAR szMscoree[]   = {'\\','m','s','c','o','r','e','e','.','d','l','l',0};
    static const WCHAR szSxs[]       = {'s','x','s','.','d','l','l',0};
    static const WCHAR szVersion10[] = {'v','1','.','0','.','3','7','0','5',0};
    static const WCHAR szVersion11[] = {'v','1','.','1','.','4','3','2','2',0};
    static const WCHAR szVersion20[] = {'v','2','.','0','.','5','0','7','2','7',0};
    static const WCHAR szVersion40[] = {'v','4','.','0','.','3','0','3','1','9',0};
    WCHAR path[MAX_PATH];
    DWORD len = GetSystemDirectoryW( path, MAX_PATH );

    if (!hsxs && !(hsxs = LoadLibraryW( szSxs ))) return FALSE;
    if (!(pCreateAssemblyCacheSxs = (void *)GetProcAddress( hsxs, "CreateAssemblyCache" )))
    {
        FreeLibrary( hsxs );
        hsxs = NULL;
        return FALSE;
    }
    strcpyW( path + len, szMscoree );
    if (hmscoree || !(hmscoree = LoadLibraryW( path ))) return TRUE;
    pGetFileVersion = (void *)GetProcAddress( hmscoree, "GetFileVersion" ); /* missing from v1.0.3705 */
    if (!(pLoadLibraryShim = (void *)GetProcAddress( hmscoree, "LoadLibraryShim" )))
    {
        FreeLibrary( hmscoree );
        hmscoree = NULL;
        return TRUE;
    }
    if (!pLoadLibraryShim( szFusion, szVersion10, NULL, &hfusion10 ))
        pCreateAssemblyCacheNet10 = (void *)GetProcAddress( hfusion10, "CreateAssemblyCache" );

    if (!pLoadLibraryShim( szFusion, szVersion11, NULL, &hfusion11 ))
        pCreateAssemblyCacheNet11 = (void *)GetProcAddress( hfusion11, "CreateAssemblyCache" );

    if (!pLoadLibraryShim( szFusion, szVersion20, NULL, &hfusion20 ))
        pCreateAssemblyCacheNet20 = (void *)GetProcAddress( hfusion20, "CreateAssemblyCache" );

    if (!pLoadLibraryShim( szFusion, szVersion40, NULL, &hfusion40 ))
    {
        pCreateAssemblyCacheNet40 = (void *)GetProcAddress( hfusion40, "CreateAssemblyCache" );
        pCreateAssemblyNameObject = (void *)GetProcAddress( hfusion40, "CreateAssemblyNameObject" );
        pCreateAssemblyEnum = (void *)GetProcAddress( hfusion40, "CreateAssemblyEnum" );
    }
    return TRUE;
}

BOOL msi_init_assembly_caches( MSIPACKAGE *package )
{
    if (!init_function_pointers()) return FALSE;
    if (pCreateAssemblyCacheSxs( &package->cache_sxs, 0 ) != S_OK) return FALSE;
    if (pCreateAssemblyCacheNet10) pCreateAssemblyCacheNet10( &package->cache_net[CLR_VERSION_V10], 0 );
    if (pCreateAssemblyCacheNet11) pCreateAssemblyCacheNet11( &package->cache_net[CLR_VERSION_V11], 0 );
    if (pCreateAssemblyCacheNet20) pCreateAssemblyCacheNet20( &package->cache_net[CLR_VERSION_V20], 0 );
    if (pCreateAssemblyCacheNet40) pCreateAssemblyCacheNet40( &package->cache_net[CLR_VERSION_V40], 0 );
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
    pCreateAssemblyCacheNet10 = NULL;
    pCreateAssemblyCacheNet11 = NULL;
    pCreateAssemblyCacheNet20 = NULL;
    pCreateAssemblyCacheNet40 = NULL;
    FreeLibrary( hfusion10 );
    FreeLibrary( hfusion11 );
    FreeLibrary( hfusion20 );
    FreeLibrary( hfusion40 );
    FreeLibrary( hmscoree );
    FreeLibrary( hsxs );
    hfusion10 = NULL;
    hfusion11 = NULL;
    hfusion20 = NULL;
    hfusion40 = NULL;
    hmscoree = NULL;
    hsxs = NULL;
}

static MSIRECORD *get_assembly_record( MSIPACKAGE *package, const WCHAR *comp )
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','M','s','i','A','s','s','e','m','b','l','y','`',' ',
         'W','H','E','R','E',' ','`','C','o','m','p','o','n','e','n','t','_','`',
         ' ','=',' ','\'','%','s','\'',0};
    MSIQUERY *view;
    MSIRECORD *rec;
    UINT r;

    r = MSI_OpenQuery( package->db, &view, query, comp );
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
    UINT    count;
    UINT    index;
    WCHAR **attrs;
};

static UINT get_assembly_name_attribute( MSIRECORD *rec, LPVOID param )
{
    static const WCHAR fmtW[] = {'%','s','=','"','%','s','"',0};
    static const WCHAR nameW[] = {'n','a','m','e',0};
    struct assembly_name *name = param;
    const WCHAR *attr = MSI_RecordGetString( rec, 2 );
    const WCHAR *value = MSI_RecordGetString( rec, 3 );
    int len = strlenW( fmtW ) + strlenW( attr ) + strlenW( value );

    if (!(name->attrs[name->index] = msi_alloc( len * sizeof(WCHAR) )))
        return ERROR_OUTOFMEMORY;

    if (!strcmpiW( attr, nameW )) strcpyW( name->attrs[name->index++], value );
    else sprintfW( name->attrs[name->index++], fmtW, attr, value );
    return ERROR_SUCCESS;
}

static WCHAR *get_assembly_display_name( MSIDATABASE *db, const WCHAR *comp, MSIASSEMBLY *assembly )
{
    static const WCHAR commaW[] = {',',0};
    static const WCHAR queryW[] = {
        'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
        '`','M','s','i','A','s','s','e','m','b','l','y','N','a','m','e','`',' ',
        'W','H','E','R','E',' ','`','C','o','m','p','o','n','e','n','t','_','`',
        ' ','=',' ','\'','%','s','\'',0};
    struct assembly_name name;
    WCHAR *display_name = NULL;
    MSIQUERY *view;
    UINT i, r;
    int len;

    r = MSI_OpenQuery( db, &view, queryW, comp );
    if (r != ERROR_SUCCESS)
        return NULL;

    name.count = 0;
    name.index = 0;
    name.attrs = NULL;
    MSI_IterateRecords( view, &name.count, NULL, NULL );
    if (!name.count) goto done;

    name.attrs = msi_alloc( name.count * sizeof(WCHAR *) );
    if (!name.attrs) goto done;

    MSI_IterateRecords( view, NULL, get_assembly_name_attribute, &name );

    len = 0;
    for (i = 0; i < name.count; i++) len += strlenW( name.attrs[i] ) + 1;

    display_name = msi_alloc( (len + 1) * sizeof(WCHAR) );
    if (display_name)
    {
        display_name[0] = 0;
        for (i = 0; i < name.count; i++)
        {
            strcatW( display_name, name.attrs[i] );
            if (i < name.count - 1) strcatW( display_name, commaW );
        }
    }

done:
    msiobj_release( &view->hdr );
    if (name.attrs)
    {
        for (i = 0; i < name.count; i++) msi_free( name.attrs[i] );
        msi_free( name.attrs );
    }
    return display_name;
}

static BOOL is_assembly_installed( IAssemblyCache *cache, const WCHAR *display_name )
{
    HRESULT hr;
    ASSEMBLY_INFO info;

    if (!cache) return FALSE;

    memset( &info, 0, sizeof(info) );
    info.cbAssemblyInfo = sizeof(info);
    hr = IAssemblyCache_QueryAssemblyInfo( cache, 0, display_name, &info );
    if (hr == S_OK /* sxs version */ || hr == E_NOT_SUFFICIENT_BUFFER)
    {
        return (info.dwAssemblyFlags == ASSEMBLYINFO_FLAG_INSTALLED);
    }
    TRACE("QueryAssemblyInfo returned 0x%08x\n", hr);
    return FALSE;
}

WCHAR *msi_get_assembly_path( MSIPACKAGE *package, const WCHAR *displayname )
{
    HRESULT hr;
    ASSEMBLY_INFO info;
    IAssemblyCache *cache = package->cache_net[CLR_VERSION_V40];

    if (!cache) return NULL;

    memset( &info, 0, sizeof(info) );
    info.cbAssemblyInfo = sizeof(info);
    hr = IAssemblyCache_QueryAssemblyInfo( cache, 0, displayname, &info );
    if (hr != E_NOT_SUFFICIENT_BUFFER) return NULL;

    if (!(info.pszCurrentAssemblyPathBuf = msi_alloc( info.cchBuf * sizeof(WCHAR) ))) return NULL;

    hr = IAssemblyCache_QueryAssemblyInfo( cache, 0, displayname, &info );
    if (FAILED( hr ))
    {
        msi_free( info.pszCurrentAssemblyPathBuf );
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
    UINT len = 0;

    if (!pCreateAssemblyNameObject || !pCreateAssemblyEnum) return NULL;

    hr = pCreateAssemblyNameObject( &name, displayname, CANOF_PARSE_DISPLAY_NAME, NULL );
    if (FAILED( hr )) return NULL;

    hr = IAssemblyName_GetName( name, &len, NULL );
    if (hr != E_NOT_SUFFICIENT_BUFFER || !(str = msi_alloc( len * sizeof(WCHAR) )))
    {
        IAssemblyName_Release( name );
        return NULL;
    }

    hr = IAssemblyName_GetName( name, &len, str );
    IAssemblyName_Release( name );
    if (FAILED( hr ))
    {
        msi_free( str );
        return NULL;
    }

    hr = pCreateAssemblyNameObject( &name, str, 0, NULL );
    msi_free( str );
    if (FAILED( hr )) return NULL;

    hr = pCreateAssemblyEnum( &ret, NULL, name, ASM_CACHE_GAC, NULL );
    IAssemblyName_Release( name );
    if (FAILED( hr )) return NULL;

    return ret;
}

static const WCHAR clr_version_v10[] = {'v','1','.','0','.','3','7','0','5',0};
static const WCHAR clr_version_v11[] = {'v','1','.','1','.','4','3','2','2',0};
static const WCHAR clr_version_v20[] = {'v','2','.','0','.','5','0','7','2','7',0};
static const WCHAR clr_version_v40[] = {'v','4','.','0','.','3','0','3','1','9',0};
static const WCHAR clr_version_unknown[] = {'u','n','k','n','o','w','n',0};

static const WCHAR *clr_version[] =
{
    clr_version_v10,
    clr_version_v11,
    clr_version_v20,
    clr_version_v40
};

static const WCHAR *get_clr_version_str( enum clr_version version )
{
    if (version >= sizeof(clr_version)/sizeof(clr_version[0])) return clr_version_unknown;
    return clr_version[version];
}

/* assembly caches must be initialized */
MSIASSEMBLY *msi_load_assembly( MSIPACKAGE *package, MSICOMPONENT *comp )
{
    MSIRECORD *rec;
    MSIASSEMBLY *a;

    if (!(rec = get_assembly_record( package, comp->Component ))) return NULL;
    if (!(a = msi_alloc_zero( sizeof(MSIASSEMBLY) )))
    {
        msiobj_release( &rec->hdr );
        return NULL;
    }
    a->feature = strdupW( MSI_RecordGetString( rec, 2 ) );
    TRACE("feature %s\n", debugstr_w(a->feature));

    a->manifest = strdupW( MSI_RecordGetString( rec, 3 ) );
    TRACE("manifest %s\n", debugstr_w(a->manifest));

    a->application = strdupW( MSI_RecordGetString( rec, 4 ) );
    TRACE("application %s\n", debugstr_w(a->application));

    a->attributes = MSI_RecordGetInteger( rec, 5 );
    TRACE("attributes %u\n", a->attributes);

    if (!(a->display_name = get_assembly_display_name( package->db, comp->Component, a )))
    {
        WARN("can't get display name\n");
        msiobj_release( &rec->hdr );
        msi_free( a->feature );
        msi_free( a->manifest );
        msi_free( a->application );
        msi_free( a );
        return NULL;
    }
    TRACE("display name %s\n", debugstr_w(a->display_name));

    if (a->application)
    {
        /* We can't check the manifest here because the target path may still change.
           So we assume that the assembly is not installed and lean on the InstallFiles
           action to determine which files need to be installed.
         */
        a->installed = FALSE;
    }
    else
    {
        if (a->attributes == msidbAssemblyAttributesWin32)
            a->installed = is_assembly_installed( package->cache_sxs, a->display_name );
        else
        {
            UINT i;
            for (i = 0; i < CLR_VERSION_MAX; i++)
            {
                a->clr_version[i] = is_assembly_installed( package->cache_net[i], a->display_name );
                if (a->clr_version[i])
                {
                    TRACE("runtime version %s\n", debugstr_w(get_clr_version_str( i )));
                    a->installed = TRUE;
                    break;
                }
            }
        }
    }
    TRACE("assembly is %s\n", a->installed ? "installed" : "not installed");
    msiobj_release( &rec->hdr );
    return a;
}

static enum clr_version get_clr_version( const WCHAR *filename )
{
    DWORD len;
    HRESULT hr;
    enum clr_version version = CLR_VERSION_V11;
    WCHAR *strW;

    if (!pGetFileVersion) return CLR_VERSION_V10;

    hr = pGetFileVersion( filename, NULL, 0, &len );
    if (hr != E_NOT_SUFFICIENT_BUFFER) return CLR_VERSION_V11;
    if ((strW = msi_alloc( len * sizeof(WCHAR) )))
    {
        hr = pGetFileVersion( filename, strW, len, &len );
        if (hr == S_OK)
        {
            UINT i;
            for (i = 0; i < CLR_VERSION_MAX; i++)
                if (!strcmpW( strW, clr_version[i] )) version = i;
        }
        msi_free( strW );
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
        cache = package->cache_net[get_clr_version( manifest )];
        if (!cache) return ERROR_SUCCESS;
    }
    TRACE("installing assembly %s\n", debugstr_w(manifest));

    hr = IAssemblyCache_InstallAssembly( cache, 0, manifest, NULL );
    if (hr != S_OK)
    {
        ERR("Failed to install assembly %s (0x%08x)\n", debugstr_w(manifest), hr);
        return ERROR_FUNCTION_FAILED;
    }
    if (feature) feature->Action = INSTALLSTATE_LOCAL;
    assembly->installed = TRUE;
    return ERROR_SUCCESS;
}

UINT msi_uninstall_assembly( MSIPACKAGE *package, MSICOMPONENT *comp )
{
    HRESULT hr;
    IAssemblyCache *cache;
    MSIASSEMBLY *assembly = comp->assembly;
    MSIFEATURE *feature = NULL;

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
        if (FAILED( hr )) WARN("failed to uninstall assembly 0x%08x\n", hr);
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
                if (FAILED( hr )) WARN("failed to uninstall assembly 0x%08x\n", hr);
            }
        }
    }
    if (feature) feature->Action = INSTALLSTATE_ABSENT;
    assembly->installed = FALSE;
    return ERROR_SUCCESS;
}

static WCHAR *build_local_assembly_path( const WCHAR *filename )
{
    UINT i;
    WCHAR *ret;

    if (!(ret = msi_alloc( (strlenW( filename ) + 1) * sizeof(WCHAR) )))
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
    static const WCHAR path_win32[] =
        {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
          'I','n','s','t','a','l','l','e','r','\\','W','i','n','3','2','A','s','s','e','m','b','l','i','e','s','\\',0};
    static const WCHAR path_dotnet[] =
        {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
         'I','n','s','t','a','l','l','e','r','\\','A','s','s','e','m','b','l','i','e','s','\\',0};
    static const WCHAR classes_path_win32[] =
        {'I','n','s','t','a','l','l','e','r','\\','W','i','n','3','2','A','s','s','e','m','b','l','i','e','s','\\',0};
    static const WCHAR classes_path_dotnet[] =
        {'I','n','s','t','a','l','l','e','r','\\','A','s','s','e','m','b','l','i','e','s','\\',0};
    HKEY root;
    const WCHAR *path;

    if (context == MSIINSTALLCONTEXT_MACHINE)
    {
        root = HKEY_CLASSES_ROOT;
        if (win32) path = classes_path_win32;
        else path = classes_path_dotnet;
    }
    else
    {
        root = HKEY_CURRENT_USER;
        if (win32) path = path_win32;
        else path = path_dotnet;
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
        msi_free( path );
        return res;
    }
    res = RegCreateKeyW( root, path, hkey );
    RegCloseKey( root );
    msi_free( path );
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
        msi_free( path );
        return res;
    }
    res = RegDeleteKeyW( root, path );
    RegCloseKey( root );
    msi_free( path );
    return res;
}

static LONG open_global_assembly_key( UINT context, BOOL win32, HKEY *hkey )
{
    static const WCHAR path_win32[] =
        {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
         'I','n','s','t','a','l','l','e','r','\\','W','i','n','3','2','A','s','s','e','m','b','l','i','e','s','\\',
         'G','l','o','b','a','l',0};
    static const WCHAR path_dotnet[] =
        {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
         'I','n','s','t','a','l','l','e','r','\\','A','s','s','e','m','b','l','i','e','s','\\',
         'G','l','o','b','a','l',0};
    static const WCHAR classes_path_win32[] =
        {'I','n','s','t','a','l','l','e','r','\\','W','i','n','3','2','A','s','s','e','m','b','l','i','e','s','\\',
         'G','l','o','b','a','l',0};
    static const WCHAR classes_path_dotnet[] =
        {'I','n','s','t','a','l','l','e','r','\\','A','s','s','e','m','b','l','i','e','s','\\','G','l','o','b','a','l',0};
    HKEY root;
    const WCHAR *path;

    if (context == MSIINSTALLCONTEXT_MACHINE)
    {
        root = HKEY_CLASSES_ROOT;
        if (win32) path = classes_path_win32;
        else path = classes_path_dotnet;
    }
    else
    {
        root = HKEY_CURRENT_USER;
        if (win32) path = path_win32;
        else path = path_dotnet;
    }
    return RegCreateKeyW( root, path, hkey );
}

UINT ACTION_MsiPublishAssemblies( MSIPACKAGE *package )
{
    MSICOMPONENT *comp;

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
            if ((res = open_local_assembly_key( package->Context, win32, file->TargetPath, &hkey )))
            {
                WARN("failed to open local assembly key %d\n", res);
                return ERROR_FUNCTION_FAILED;
            }
        }
        else
        {
            if ((res = open_global_assembly_key( package->Context, win32, &hkey )))
            {
                WARN("failed to open global assembly key %d\n", res);
                return ERROR_FUNCTION_FAILED;
            }
        }
        size = sizeof(buffer);
        if ((res = RegSetValueExW( hkey, assembly->display_name, 0, REG_MULTI_SZ, (const BYTE *)buffer, size )))
        {
            WARN("failed to set assembly value %d\n", res);
        }
        RegCloseKey( hkey );

        uirow = MSI_CreateRecord( 2 );
        MSI_RecordSetStringW( uirow, 2, assembly->display_name );
        msi_ui_actiondata( package, szMsiPublishAssemblies, uirow );
        msiobj_release( &uirow->hdr );
    }
    return ERROR_SUCCESS;
}

UINT ACTION_MsiUnpublishAssemblies( MSIPACKAGE *package )
{
    MSICOMPONENT *comp;

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
            if ((res = delete_local_assembly_key( package->Context, win32, file->TargetPath )))
                WARN("failed to delete local assembly key %d\n", res);
        }
        else
        {
            HKEY hkey;
            if ((res = open_global_assembly_key( package->Context, win32, &hkey )))
                WARN("failed to delete global assembly key %d\n", res);
            else
            {
                if ((res = RegDeleteValueW( hkey, assembly->display_name )))
                    WARN("failed to delete global assembly value %d\n", res);
                RegCloseKey( hkey );
            }
        }

        uirow = MSI_CreateRecord( 2 );
        MSI_RecordSetStringW( uirow, 2, assembly->display_name );
        msi_ui_actiondata( package, szMsiPublishAssemblies, uirow );
        msiobj_release( &uirow->hdr );
    }
    return ERROR_SUCCESS;
}
