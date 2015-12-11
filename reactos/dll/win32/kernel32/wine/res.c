/*
 * Resources
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995, 2003 Alexandre Julliard
 * Copyright 2006 Mike McCormack
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

#include <k32.h>

#include <wine/list.h>

#define NDEBUG
#include <debug.h>
DEBUG_CHANNEL(resource);

/* retrieve the resource name to pass to the ntdll functions */
static NTSTATUS get_res_nameA( LPCSTR name, UNICODE_STRING *str )
{
    if (IS_INTRESOURCE(name))
    {
        str->Buffer = ULongToPtr(LOWORD(name));
        return STATUS_SUCCESS;
    }
    if (name[0] == '#')
    {
        ULONG value;
        if (RtlCharToInteger( name + 1, 10, &value ) != STATUS_SUCCESS || HIWORD(value))
            return STATUS_INVALID_PARAMETER;
        str->Buffer = ULongToPtr(value);
        return STATUS_SUCCESS;
    }
    RtlCreateUnicodeStringFromAsciiz( str, name );
    RtlUpcaseUnicodeString( str, str, FALSE );
    return STATUS_SUCCESS;
}

/* retrieve the resource name to pass to the ntdll functions */
static NTSTATUS get_res_nameW( LPCWSTR name, UNICODE_STRING *str )
{
    if (IS_INTRESOURCE(name))
    {
        str->Buffer = ULongToPtr(LOWORD(name));
        return STATUS_SUCCESS;
    }
    if (name[0] == '#')
    {
        ULONG value;
        RtlInitUnicodeString( str, name + 1 );
        if (RtlUnicodeStringToInteger( str, 10, &value ) != STATUS_SUCCESS || HIWORD(value))
            return STATUS_INVALID_PARAMETER;
        str->Buffer = ULongToPtr(value);
        return STATUS_SUCCESS;
    }
    RtlCreateUnicodeString( str, name );
    RtlUpcaseUnicodeString( str, str, FALSE );
    return STATUS_SUCCESS;
}

/* implementation of FindResourceExA */
static HRSRC find_resourceA( HMODULE hModule, LPCSTR type, LPCSTR name, WORD lang )
{
    NTSTATUS status;
    UNICODE_STRING nameW, typeW;
    LDR_RESOURCE_INFO info;
    IMAGE_RESOURCE_DATA_ENTRY *entry = NULL;

    nameW.Buffer = NULL;
    typeW.Buffer = NULL;

    __TRY
    {
        if ((status = get_res_nameA( name, &nameW )) != STATUS_SUCCESS) goto done;
        if ((status = get_res_nameA( type, &typeW )) != STATUS_SUCCESS) goto done;
        info.Type = (ULONG_PTR)typeW.Buffer;
        info.Name = (ULONG_PTR)nameW.Buffer;
        info.Language = lang;
        status = LdrFindResource_U( hModule, &info, 3, &entry );
    done:
        if (status != STATUS_SUCCESS) SetLastError( RtlNtStatusToDosError(status) );
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError( ERROR_INVALID_PARAMETER );
    }
    __ENDTRY

    if (!IS_INTRESOURCE(nameW.Buffer)) HeapFree( GetProcessHeap(), 0, nameW.Buffer );
    if (!IS_INTRESOURCE(typeW.Buffer)) HeapFree( GetProcessHeap(), 0, typeW.Buffer );
    return (HRSRC)entry;
}


/* implementation of FindResourceExW */
static HRSRC find_resourceW( HMODULE hModule, LPCWSTR type, LPCWSTR name, WORD lang )
{
    NTSTATUS status;
    UNICODE_STRING nameW, typeW;
    LDR_RESOURCE_INFO info;
    IMAGE_RESOURCE_DATA_ENTRY *entry = NULL;

    nameW.Buffer = typeW.Buffer = NULL;

    __TRY
    {
        if ((status = get_res_nameW( name, &nameW )) != STATUS_SUCCESS) goto done;
        if ((status = get_res_nameW( type, &typeW )) != STATUS_SUCCESS) goto done;
        info.Type = (ULONG_PTR)typeW.Buffer;
        info.Name = (ULONG_PTR)nameW.Buffer;
        info.Language = lang;
        status = LdrFindResource_U( hModule, &info, 3, &entry );
    done:
        if (status != STATUS_SUCCESS) SetLastError( RtlNtStatusToDosError(status) );
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError( ERROR_INVALID_PARAMETER );
    }
    __ENDTRY

    if (!IS_INTRESOURCE(nameW.Buffer)) HeapFree( GetProcessHeap(), 0, nameW.Buffer );
    if (!IS_INTRESOURCE(typeW.Buffer)) HeapFree( GetProcessHeap(), 0, typeW.Buffer );
    return (HRSRC)entry;
}

/**********************************************************************
 *	    FindResourceExA  (KERNEL32.@)
 */
HRSRC WINAPI FindResourceExA( HMODULE hModule, LPCSTR type, LPCSTR name, WORD lang )
{
    TRACE( "%p %s %s %04x\n", hModule, debugstr_a(type), debugstr_a(name), lang );

    if (!hModule) hModule = GetModuleHandleW(0);
    return find_resourceA( hModule, type, name, lang );
}


/**********************************************************************
 *	    FindResourceA    (KERNEL32.@)
 */
HRSRC WINAPI FindResourceA( HMODULE hModule, LPCSTR name, LPCSTR type )
{
    return FindResourceExA( hModule, type, name, MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ) );
}


/**********************************************************************
 *	    FindResourceExW  (KERNEL32.@)
 */
HRSRC WINAPI FindResourceExW( HMODULE hModule, LPCWSTR type, LPCWSTR name, WORD lang )
{
    TRACE( "%p %s %s %04x\n", hModule, debugstr_w(type), debugstr_w(name), lang );

    if (!hModule) hModule = GetModuleHandleW(0);
    return find_resourceW( hModule, type, name, lang );
}


/**********************************************************************
 *	    FindResourceW    (KERNEL32.@)
 */
HRSRC WINAPI FindResourceW( HINSTANCE hModule, LPCWSTR name, LPCWSTR type )
{
    return FindResourceExW( hModule, type, name, MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ) );
}


/**********************************************************************
 *	EnumResourceTypesA	(KERNEL32.@)
 */
BOOL WINAPI EnumResourceTypesA( HMODULE hmod, ENUMRESTYPEPROCA lpfun, LONG_PTR lparam )
{
    int i;
    BOOL ret = FALSE;
    LPSTR type = NULL;
    DWORD len = 0, newlen;
    NTSTATUS status;
    IMAGE_RESOURCE_DIRECTORY *resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;
    const IMAGE_RESOURCE_DIR_STRING_U *str;

    TRACE( "%p %p %lx\n", hmod, lpfun, lparam );

    if (!hmod) hmod = GetModuleHandleA( NULL );

    if ((status = LdrFindResourceDirectory_U( hmod, NULL, 0, &resdir )) != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    et = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(resdir + 1);
    for (i = 0; i < resdir->NumberOfNamedEntries+resdir->NumberOfIdEntries; i++)
    {
        if (et[i].NameIsString)
        {
            str = (const IMAGE_RESOURCE_DIR_STRING_U *)((const BYTE *)resdir + et[i].NameOffset);
            newlen = WideCharToMultiByte( CP_ACP, 0, str->NameString, str->Length, NULL, 0, NULL, NULL);
            if (newlen + 1 > len)
            {
                len = newlen + 1;
                HeapFree( GetProcessHeap(), 0, type );
                if (!(type = HeapAlloc( GetProcessHeap(), 0, len ))) return FALSE;
            }
            WideCharToMultiByte( CP_ACP, 0, str->NameString, str->Length, type, len, NULL, NULL);
            type[newlen] = 0;
            ret = lpfun(hmod,type,lparam);
        }
        else
        {
            ret = lpfun( hmod, UIntToPtr(et[i].Id), lparam );
        }
        if (!ret) break;
    }
    HeapFree( GetProcessHeap(), 0, type );
    return ret;
}


/**********************************************************************
 *	EnumResourceTypesW	(KERNEL32.@)
 */
BOOL WINAPI EnumResourceTypesW( HMODULE hmod, ENUMRESTYPEPROCW lpfun, LONG_PTR lparam )
{
    int i, len = 0;
    BOOL ret = FALSE;
    LPWSTR type = NULL;
    NTSTATUS status;
    IMAGE_RESOURCE_DIRECTORY *resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;
    const IMAGE_RESOURCE_DIR_STRING_U *str;

    TRACE( "%p %p %lx\n", hmod, lpfun, lparam );

    if (!hmod) hmod = GetModuleHandleW( NULL );

    if ((status = LdrFindResourceDirectory_U( hmod, NULL, 0, &resdir )) != STATUS_SUCCESS)
    {
        SetLastError( RtlNtStatusToDosError(status) );
        return FALSE;
    }
    et = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(resdir + 1);
    for (i = 0; i < resdir->NumberOfNamedEntries + resdir->NumberOfIdEntries; i++)
    {
        if (et[i].NameIsString)
        {
            str = (const IMAGE_RESOURCE_DIR_STRING_U *)((const BYTE *)resdir + et[i].NameOffset);
            if (str->Length + 1 > len)
            {
                len = str->Length + 1;
                HeapFree( GetProcessHeap(), 0, type );
                if (!(type = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) ))) return FALSE;
            }
            memcpy(type, str->NameString, str->Length * sizeof (WCHAR));
            type[str->Length] = 0;
            ret = lpfun(hmod,type,lparam);
        }
        else
        {
            ret = lpfun( hmod, UIntToPtr(et[i].Id), lparam );
        }
        if (!ret) break;
    }
    HeapFree( GetProcessHeap(), 0, type );
    return ret;
}


/**********************************************************************
 *	EnumResourceNamesA	(KERNEL32.@)
 */
BOOL WINAPI EnumResourceNamesA( HMODULE hmod, LPCSTR type, ENUMRESNAMEPROCA lpfun, LONG_PTR lparam )
{
    int i;
    BOOL ret = FALSE;
    DWORD len = 0, newlen;
    LPSTR name = NULL;
    NTSTATUS status;
    UNICODE_STRING typeW;
    LDR_RESOURCE_INFO info;
    IMAGE_RESOURCE_DIRECTORY *basedir, *resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;
    const IMAGE_RESOURCE_DIR_STRING_U *str;

    TRACE( "%p %s %p %lx\n", hmod, debugstr_a(type), lpfun, lparam );

    if (!hmod) hmod = GetModuleHandleA( NULL );
    typeW.Buffer = NULL;
    if ((status = LdrFindResourceDirectory_U( hmod, NULL, 0, &basedir )) != STATUS_SUCCESS)
        goto done;
    if ((status = get_res_nameA( type, &typeW )) != STATUS_SUCCESS)
        goto done;
    info.Type = (ULONG_PTR)typeW.Buffer;
    if ((status = LdrFindResourceDirectory_U( hmod, &info, 1, &resdir )) != STATUS_SUCCESS)
        goto done;

    et = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(resdir + 1);
    __TRY
    {
        for (i = 0; i < resdir->NumberOfNamedEntries+resdir->NumberOfIdEntries; i++)
        {
            if (et[i].NameIsString)
            {
                str = (const IMAGE_RESOURCE_DIR_STRING_U *)((const BYTE *)basedir + et[i].NameOffset);
                newlen = WideCharToMultiByte(CP_ACP, 0, str->NameString, str->Length, NULL, 0, NULL, NULL);
                if (newlen + 1 > len)
                {
                    len = newlen + 1;
                    HeapFree( GetProcessHeap(), 0, name );
                    if (!(name = HeapAlloc(GetProcessHeap(), 0, len + 1 )))
                    {
                        ret = FALSE;
                        break;
                    }
                }
                WideCharToMultiByte( CP_ACP, 0, str->NameString, str->Length, name, len, NULL, NULL );
                name[newlen] = 0;
                ret = lpfun(hmod,type,name,lparam);
            }
            else
            {
                ret = lpfun( hmod, type, UIntToPtr(et[i].Id), lparam );
            }
            if (!ret) break;
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        ret = FALSE;
        status = STATUS_ACCESS_VIOLATION;
    }
    __ENDTRY;

done:
    HeapFree( GetProcessHeap(), 0, name );
    if (!IS_INTRESOURCE(typeW.Buffer)) HeapFree( GetProcessHeap(), 0, typeW.Buffer );
    if (status != STATUS_SUCCESS) SetLastError( RtlNtStatusToDosError(status) );
    return ret;
}


/**********************************************************************
 *	EnumResourceNamesW	(KERNEL32.@)
 */
BOOL WINAPI EnumResourceNamesW( HMODULE hmod, LPCWSTR type, ENUMRESNAMEPROCW lpfun, LONG_PTR lparam )
{
    int i, len = 0;
    BOOL ret = FALSE;
    LPWSTR name = NULL;
    NTSTATUS status;
    UNICODE_STRING typeW;
    LDR_RESOURCE_INFO info;
    IMAGE_RESOURCE_DIRECTORY *basedir, *resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;
    const IMAGE_RESOURCE_DIR_STRING_U *str;

    TRACE( "%p %s %p %lx\n", hmod, debugstr_w(type), lpfun, lparam );

    if (!hmod) hmod = GetModuleHandleW( NULL );
    typeW.Buffer = NULL;
    if ((status = LdrFindResourceDirectory_U( hmod, NULL, 0, &basedir )) != STATUS_SUCCESS)
        goto done;
    if ((status = get_res_nameW( type, &typeW )) != STATUS_SUCCESS)
        goto done;
    info.Type = (ULONG_PTR)typeW.Buffer;
    if ((status = LdrFindResourceDirectory_U( hmod, &info, 1, &resdir )) != STATUS_SUCCESS)
        goto done;

    et = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(resdir + 1);
    __TRY
    {
        for (i = 0; i < resdir->NumberOfNamedEntries+resdir->NumberOfIdEntries; i++)
        {
            if (et[i].NameIsString)
            {
                str = (const IMAGE_RESOURCE_DIR_STRING_U *)((const BYTE *)basedir + et[i].NameOffset);
                if (str->Length + 1 > len)
                {
                    len = str->Length + 1;
                    HeapFree( GetProcessHeap(), 0, name );
                    if (!(name = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
                    {
                        ret = FALSE;
                        break;
                    }
                }
                memcpy(name, str->NameString, str->Length * sizeof (WCHAR));
                name[str->Length] = 0;
                ret = lpfun(hmod,type,name,lparam);
            }
            else
            {
                ret = lpfun( hmod, type, UIntToPtr(et[i].Id), lparam );
            }
            if (!ret) break;
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        ret = FALSE;
        status = STATUS_ACCESS_VIOLATION;
    }
    __ENDTRY
done:
    HeapFree( GetProcessHeap(), 0, name );
    if (!IS_INTRESOURCE(typeW.Buffer)) HeapFree( GetProcessHeap(), 0, typeW.Buffer );
    if (status != STATUS_SUCCESS) SetLastError( RtlNtStatusToDosError(status) );
    return ret;
}


/**********************************************************************
 *	EnumResourceLanguagesA	(KERNEL32.@)
 */
BOOL WINAPI EnumResourceLanguagesA( HMODULE hmod, LPCSTR type, LPCSTR name,
                                    ENUMRESLANGPROCA lpfun, LONG_PTR lparam )
{
    int i;
    BOOL ret = FALSE;
    NTSTATUS status;
    UNICODE_STRING typeW, nameW;
    LDR_RESOURCE_INFO info;
    IMAGE_RESOURCE_DIRECTORY *basedir, *resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;

    TRACE( "%p %s %s %p %lx\n", hmod, debugstr_a(type), debugstr_a(name), lpfun, lparam );

    if (!hmod) hmod = GetModuleHandleA( NULL );
    typeW.Buffer = nameW.Buffer = NULL;
    if ((status = LdrFindResourceDirectory_U( hmod, NULL, 0, &basedir )) != STATUS_SUCCESS)
        goto done;
    if ((status = get_res_nameA( type, &typeW )) != STATUS_SUCCESS)
        goto done;
    if ((status = get_res_nameA( name, &nameW )) != STATUS_SUCCESS)
        goto done;
    info.Type = (ULONG_PTR)typeW.Buffer;
    info.Name = (ULONG_PTR)nameW.Buffer;
    if ((status = LdrFindResourceDirectory_U( hmod, &info, 2, &resdir )) != STATUS_SUCCESS)
        goto done;

    et = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(resdir + 1);
    __TRY
    {
        for (i = 0; i < resdir->NumberOfNamedEntries + resdir->NumberOfIdEntries; i++)
        {
            ret = lpfun( hmod, type, name, et[i].Id, lparam );
            if (!ret) break;
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        ret = FALSE;
        status = STATUS_ACCESS_VIOLATION;
    }
    __ENDTRY
done:
    if (!IS_INTRESOURCE(typeW.Buffer)) HeapFree( GetProcessHeap(), 0, typeW.Buffer );
    if (!IS_INTRESOURCE(nameW.Buffer)) HeapFree( GetProcessHeap(), 0, nameW.Buffer );
    if (status != STATUS_SUCCESS) SetLastError( RtlNtStatusToDosError(status) );
    return ret;
}


/**********************************************************************
 *	EnumResourceLanguagesW	(KERNEL32.@)
 */
BOOL WINAPI EnumResourceLanguagesW( HMODULE hmod, LPCWSTR type, LPCWSTR name,
                                    ENUMRESLANGPROCW lpfun, LONG_PTR lparam )
{
    int i;
    BOOL ret = FALSE;
    NTSTATUS status;
    UNICODE_STRING typeW, nameW;
    LDR_RESOURCE_INFO info;
    IMAGE_RESOURCE_DIRECTORY *basedir, *resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;

    TRACE( "%p %s %s %p %lx\n", hmod, debugstr_w(type), debugstr_w(name), lpfun, lparam );

    if (!hmod) hmod = GetModuleHandleW( NULL );
    typeW.Buffer = nameW.Buffer = NULL;
    if ((status = LdrFindResourceDirectory_U( hmod, NULL, 0, &basedir )) != STATUS_SUCCESS)
        goto done;
    if ((status = get_res_nameW( type, &typeW )) != STATUS_SUCCESS)
        goto done;
    if ((status = get_res_nameW( name, &nameW )) != STATUS_SUCCESS)
        goto done;
    info.Type = (ULONG_PTR)typeW.Buffer;
    info.Name = (ULONG_PTR)nameW.Buffer;
    if ((status = LdrFindResourceDirectory_U( hmod, &info, 2, &resdir )) != STATUS_SUCCESS)
        goto done;

    et = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(resdir + 1);
    __TRY
    {
        for (i = 0; i < resdir->NumberOfNamedEntries + resdir->NumberOfIdEntries; i++)
        {
            ret = lpfun( hmod, type, name, et[i].Id, lparam );
            if (!ret) break;
        }
    }
    __EXCEPT_PAGE_FAULT
    {
        ret = FALSE;
        status = STATUS_ACCESS_VIOLATION;
    }
    __ENDTRY
done:
    if (!IS_INTRESOURCE(typeW.Buffer)) HeapFree( GetProcessHeap(), 0, typeW.Buffer );
    if (!IS_INTRESOURCE(nameW.Buffer)) HeapFree( GetProcessHeap(), 0, nameW.Buffer );
    if (status != STATUS_SUCCESS) SetLastError( RtlNtStatusToDosError(status) );
    return ret;
}


/**********************************************************************
 *	    LoadResource     (KERNEL32.@)
 */
HGLOBAL WINAPI LoadResource( HINSTANCE hModule, HRSRC hRsrc )
{
    NTSTATUS status;
    void *ret = NULL;

    TRACE( "%p %p\n", hModule, hRsrc );

    if (!hRsrc) return 0;
    if (!hModule) hModule = GetModuleHandleA( NULL );
    status = LdrAccessResource( hModule, (IMAGE_RESOURCE_DATA_ENTRY *)hRsrc, &ret, NULL );
    if (status != STATUS_SUCCESS) SetLastError( RtlNtStatusToDosError(status) );
    return ret;
}


/**********************************************************************
 *	    LockResource     (KERNEL32.@)
 */
LPVOID WINAPI LockResource( HGLOBAL handle )
{
    return handle;
}


/**********************************************************************
 *	    FreeResource     (KERNEL32.@)
 */
BOOL WINAPI FreeResource( HGLOBAL handle )
{
    return FALSE;
}


/**********************************************************************
 *	    SizeofResource   (KERNEL32.@)
 */
DWORD WINAPI SizeofResource( HINSTANCE hModule, HRSRC hRsrc )
{
    if (!hRsrc) return 0;
    return ((PIMAGE_RESOURCE_DATA_ENTRY)hRsrc)->Size;
}

/*
 *  Data structure for updating resources.
 *  Type/Name/Language is a keyset for accessing resource data.
 *
 *  QUEUEDUPDATES (root) ->
 *    list of struct resource_dir_entry    (Type) ->
 *      list of struct resource_dir_entry  (Name)   ->
 *         list of struct resource_data    Language + Data
 */

typedef struct
{
    LPWSTR pFileName;
    BOOL bDeleteExistingResources;
    struct list root;
} QUEUEDUPDATES;

/* this structure is shared for types and names */
struct resource_dir_entry {
    struct list entry;
    LPWSTR id;
    struct list children;
};

/* this structure is the leaf */
struct resource_data {
    struct list entry;
    LANGID lang;
    DWORD codepage;
    DWORD cbData;
    void *lpData;
};

static int resource_strcmp( LPCWSTR a, LPCWSTR b )
{
    if ( a == b )
        return 0;
    if (!IS_INTRESOURCE( a ) && !IS_INTRESOURCE( b ) )
        return lstrcmpW( a, b );
    /* strings come before ids */
    if (!IS_INTRESOURCE( a ) && IS_INTRESOURCE( b ))
        return -1;
    if (!IS_INTRESOURCE( b ) && IS_INTRESOURCE( a ))
        return 1;
    return ( a < b ) ? -1 : 1;
}

static struct resource_dir_entry *find_resource_dir_entry( struct list *dir, LPCWSTR id )
{
    struct resource_dir_entry *ent;

    /* match either IDs or strings */
    LIST_FOR_EACH_ENTRY( ent, dir, struct resource_dir_entry, entry )
        if (!resource_strcmp( id, ent->id ))
            return ent;

    return NULL;
}

static struct resource_data *find_resource_data( struct list *dir, LANGID lang )
{
    struct resource_data *res_data;

    /* match only languages here */
    LIST_FOR_EACH_ENTRY( res_data, dir, struct resource_data, entry )
        if ( lang == res_data->lang )
             return res_data;

    return NULL;
}

static void add_resource_dir_entry( struct list *dir, struct resource_dir_entry *resdir )
{
    struct resource_dir_entry *ent;

    LIST_FOR_EACH_ENTRY( ent, dir, struct resource_dir_entry, entry )
    {
        if (0>resource_strcmp( ent->id, resdir->id ))
            continue;

        list_add_before( &ent->entry, &resdir->entry );
        return;
    }
    list_add_tail( dir, &resdir->entry );
}

static void add_resource_data_entry( struct list *dir, struct resource_data *resdata )
{
    struct resource_data *ent;

    LIST_FOR_EACH_ENTRY( ent, dir, struct resource_data, entry )
    {
        if (ent->lang < resdata->lang)
            continue;

        list_add_before( &ent->entry, &resdata->entry );
        return;
    }
    list_add_tail( dir, &resdata->entry );
}

static LPWSTR res_strdupW( LPCWSTR str )
{
    LPWSTR ret;
    UINT len;

    if (IS_INTRESOURCE(str))
        return (LPWSTR) (UINT_PTR) LOWORD(str);
    len = (lstrlenW( str ) + 1) * sizeof (WCHAR);
    ret = HeapAlloc( GetProcessHeap(), 0, len );
    memcpy( ret, str, len );
    return ret;
}

static void res_free_str( LPWSTR str )
{
    if (!IS_INTRESOURCE(str))
        HeapFree( GetProcessHeap(), 0, str );
}

static BOOL update_add_resource( QUEUEDUPDATES *updates, LPCWSTR Type, LPCWSTR Name,
                                 LANGID Lang, struct resource_data *resdata,
                                 BOOL overwrite_existing )
{
    struct resource_dir_entry *restype, *resname;
    struct resource_data *existing;

    TRACE("%p %s %s %p %d\n", updates,
          debugstr_w(Type), debugstr_w(Name), resdata, overwrite_existing );

    restype = find_resource_dir_entry( &updates->root, Type );
    if (!restype)
    {
        restype = HeapAlloc( GetProcessHeap(), 0, sizeof *restype );
        restype->id = res_strdupW( Type );
        list_init( &restype->children );
        add_resource_dir_entry( &updates->root, restype );
    }

    resname = find_resource_dir_entry( &restype->children, Name );
    if (!resname)
    {
        resname = HeapAlloc( GetProcessHeap(), 0, sizeof *resname );
        resname->id = res_strdupW( Name );
        list_init( &resname->children );
        add_resource_dir_entry( &restype->children, resname );
    }

    /*
     * If there's an existing resource entry with matching (Type,Name,Language)
     *  it needs to be removed before adding the new data.
     */
    existing = find_resource_data( &resname->children, Lang );
    if (existing)
    {
        if (!overwrite_existing)
            return FALSE;
        list_remove( &existing->entry );
        HeapFree( GetProcessHeap(), 0, existing );
    }

    if (resdata)
        add_resource_data_entry( &resname->children, resdata );

    return TRUE;
}

static struct resource_data *allocate_resource_data( WORD Language, DWORD codepage,
                                                     LPVOID lpData, DWORD cbData, BOOL copy_data )
{
    struct resource_data *resdata;

    if (!lpData || !cbData)
        return NULL;

    resdata = HeapAlloc( GetProcessHeap(), 0, sizeof *resdata + (copy_data ? cbData : 0) );
    if (resdata)
    {
        resdata->lang = Language;
        resdata->codepage = codepage;
        resdata->cbData = cbData;
        if (copy_data)
        {
            resdata->lpData = &resdata[1];
            memcpy( resdata->lpData, lpData, cbData );
        }
        else
            resdata->lpData = lpData;
    }

    return resdata;
}

static void free_resource_directory( struct list *head, int level )
{
    struct list *ptr = NULL;

    while ((ptr = list_head( head )))
    {
        list_remove( ptr );
        if (level)
        {
            struct resource_dir_entry *ent;

            ent = LIST_ENTRY( ptr, struct resource_dir_entry, entry );
            res_free_str( ent->id );
            free_resource_directory( &ent->children, level - 1 );
            HeapFree(GetProcessHeap(), 0, ent);
        }
        else
        {
            struct resource_data *data;

            data = LIST_ENTRY( ptr, struct resource_data, entry );
            HeapFree( GetProcessHeap(), 0, data );
        }
    }
}

static IMAGE_NT_HEADERS *get_nt_header( void *base, DWORD mapping_size )
{
    IMAGE_NT_HEADERS *nt;
    IMAGE_DOS_HEADER *dos;

    if (mapping_size<sizeof (*dos))
        return NULL;

    dos = base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return NULL;

    if ((dos->e_lfanew + sizeof (*nt)) > mapping_size)
        return NULL;

    nt = (void*) ((BYTE*)base + dos->e_lfanew);

    if (nt->Signature != IMAGE_NT_SIGNATURE)
        return NULL;

    return nt;
}

static IMAGE_SECTION_HEADER *get_section_header( void *base, DWORD mapping_size, DWORD *num_sections )
{
    IMAGE_NT_HEADERS *nt;
    DWORD section_ofs;

    nt = get_nt_header( base, mapping_size );
    if (!nt)
        return NULL;

    /* check that we don't go over the end of the file accessing the sections */
    section_ofs = FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader) + nt->FileHeader.SizeOfOptionalHeader;
    if ((nt->FileHeader.NumberOfSections * sizeof (IMAGE_SECTION_HEADER) + section_ofs) > mapping_size)
        return NULL;

    if (num_sections)
        *num_sections = nt->FileHeader.NumberOfSections;

    /* from here we have a valid PE exe to update */
    return (void*) ((BYTE*)nt + section_ofs);
}

static BOOL check_pe_exe( HANDLE file, QUEUEDUPDATES *updates )
{
    const IMAGE_NT_HEADERS32 *nt;
    const IMAGE_NT_HEADERS64 *nt64;
    const IMAGE_SECTION_HEADER *sec;
    const IMAGE_DATA_DIRECTORY *dd;
    BOOL ret = FALSE;
    HANDLE mapping;
    DWORD mapping_size, num_sections = 0;
    void *base = NULL;

    mapping_size = GetFileSize( file, NULL );

    mapping = CreateFileMappingW( file, NULL, PAGE_READONLY, 0, 0, NULL );
    if (!mapping)
        goto done;

    base = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, mapping_size );
    if (!base)
        goto done;

    nt = (IMAGE_NT_HEADERS32 *)get_nt_header( base, mapping_size );
    if (!nt)
        goto done;

    nt64 = (IMAGE_NT_HEADERS64*)nt;
    dd = &nt->OptionalHeader.DataDirectory[0];
    if (nt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        dd = &nt64->OptionalHeader.DataDirectory[0];

    TRACE("resources: %08x %08x\n",
          dd[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress,
          dd[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size);

    sec = get_section_header( base, mapping_size, &num_sections );
    if (!sec)
        goto done;

    ret = TRUE;

done:
    if (base)
        UnmapViewOfFile( base );
    if (mapping)
        CloseHandle( mapping );

    return ret;
}

struct resource_size_info {
    DWORD types_ofs;
    DWORD names_ofs;
    DWORD langs_ofs;
    DWORD data_entry_ofs;
    DWORD strings_ofs;
    DWORD data_ofs;
    DWORD total_size;
};

struct mapping_info {
    HANDLE file;
    void *base;
    DWORD size;
    BOOL read_write;
};

static const IMAGE_SECTION_HEADER *section_from_rva( void *base, DWORD mapping_size, DWORD rva )
{
    const IMAGE_SECTION_HEADER *sec;
    DWORD num_sections = 0;
    int i;

    sec = get_section_header( base, mapping_size, &num_sections );
    if (!sec)
        return NULL;

    for (i=num_sections-1; i>=0; i--)
    {
        if (sec[i].VirtualAddress <= rva &&
            rva <= (DWORD)sec[i].VirtualAddress + sec[i].SizeOfRawData)
        {
            return &sec[i];
        }
    }

    return NULL;
}

static void *address_from_rva( void *base, DWORD mapping_size, DWORD rva, DWORD len )
{
    const IMAGE_SECTION_HEADER *sec;

    sec = section_from_rva( base, mapping_size, rva );
    if (!sec)
        return NULL;

    if (rva + len <= (DWORD)sec->VirtualAddress + sec->SizeOfRawData)
        return (void*)((LPBYTE) base + (sec->PointerToRawData + rva - sec->VirtualAddress));

    return NULL;
}

static LPWSTR resource_dup_string( const IMAGE_RESOURCE_DIRECTORY *root, const IMAGE_RESOURCE_DIRECTORY_ENTRY *entry )
{
    const IMAGE_RESOURCE_DIR_STRING_U* string;
    LPWSTR s;

    if (!entry->NameIsString)
        return UIntToPtr(entry->Id);

    string = (const IMAGE_RESOURCE_DIR_STRING_U*) (((const char *)root) + entry->NameOffset);
    s = HeapAlloc(GetProcessHeap(), 0, (string->Length + 1)*sizeof (WCHAR) );
    memcpy( s, string->NameString, (string->Length + 1)*sizeof (WCHAR) );
    s[string->Length] = 0;

    return s;
}

/* this function is based on the code in winedump's pe.c */
static BOOL enumerate_mapped_resources( QUEUEDUPDATES *updates,
                             void *base, DWORD mapping_size,
                             const IMAGE_RESOURCE_DIRECTORY *root )
{
    const IMAGE_RESOURCE_DIRECTORY *namedir, *langdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *e1, *e2, *e3;
    const IMAGE_RESOURCE_DATA_ENTRY *data;
    DWORD i, j, k;

    TRACE("version (%d.%d) %d named %d id entries\n",
          root->MajorVersion, root->MinorVersion, root->NumberOfNamedEntries, root->NumberOfIdEntries);

    for (i = 0; i< root->NumberOfNamedEntries + root->NumberOfIdEntries; i++)
    {
        LPWSTR Type;

        e1 = (const IMAGE_RESOURCE_DIRECTORY_ENTRY*)(root + 1) + i;

        Type = resource_dup_string( root, e1 );

        namedir = (const IMAGE_RESOURCE_DIRECTORY *)((const char *)root + e1->OffsetToDirectory);
        for (j = 0; j < namedir->NumberOfNamedEntries + namedir->NumberOfIdEntries; j++)
        {
            LPWSTR Name;

            e2 = (const IMAGE_RESOURCE_DIRECTORY_ENTRY*)(namedir + 1) + j;

            Name = resource_dup_string( root, e2 );

            langdir = (const IMAGE_RESOURCE_DIRECTORY *)((const char *)root + e2->OffsetToDirectory);
            for (k = 0; k < langdir->NumberOfNamedEntries + langdir->NumberOfIdEntries; k++)
            {
                LANGID Lang;
                void *p;
                struct resource_data *resdata;

                e3 = (const IMAGE_RESOURCE_DIRECTORY_ENTRY*)(langdir + 1) + k;

                Lang = e3->Id;

                data = (const IMAGE_RESOURCE_DATA_ENTRY *)((const char *)root + e3->OffsetToData);

                p = address_from_rva( base, mapping_size, data->OffsetToData, data->Size );

                resdata = allocate_resource_data( Lang, data->CodePage, p, data->Size, FALSE );
                if (resdata)
                {
                    if (!update_add_resource( updates, Type, Name, Lang, resdata, FALSE ))
                        HeapFree( GetProcessHeap(), 0, resdata );
                }
            }
            res_free_str( Name );
        }
        res_free_str( Type );
    }

    return TRUE;
}

static BOOL read_mapped_resources( QUEUEDUPDATES *updates, void *base, DWORD mapping_size )
{
    const IMAGE_RESOURCE_DIRECTORY *root;
    const IMAGE_NT_HEADERS *nt;
    const IMAGE_SECTION_HEADER *sec;
    DWORD num_sections = 0, i;

    nt = get_nt_header( base, mapping_size );
    if (!nt)
        return FALSE;

    sec = get_section_header( base, mapping_size, &num_sections );
    if (!sec)
        return FALSE;

    for (i=0; i<num_sections; i++)
        if (!memcmp(sec[i].Name, ".rsrc", 6))
            break;

    if (i == num_sections)
        return TRUE;

    /* check the resource data is inside the mapping */
    if (sec[i].PointerToRawData > mapping_size ||
        (sec[i].PointerToRawData + sec[i].SizeOfRawData) > mapping_size)
        return TRUE;

    TRACE("found .rsrc at %08x, size %08x\n", sec[i].PointerToRawData, sec[i].SizeOfRawData);

    if (!sec[i].PointerToRawData || sec[i].SizeOfRawData < sizeof(IMAGE_RESOURCE_DIRECTORY))
        return TRUE;

    root = (void*) ((BYTE*)base + sec[i].PointerToRawData);
    enumerate_mapped_resources( updates, base, mapping_size, root );

    return TRUE;
}

static BOOL map_file_into_memory( struct mapping_info *mi )
{
    DWORD page_attr, perm;
    HANDLE mapping;

    if (mi->read_write)
    {
        page_attr = PAGE_READWRITE;
        perm = FILE_MAP_WRITE | FILE_MAP_READ;
    }
    else
    {
        page_attr = PAGE_READONLY;
        perm = FILE_MAP_READ;
    }

    mapping = CreateFileMappingW( mi->file, NULL, page_attr, 0, 0, NULL );
    if (!mapping) return FALSE;

    mi->base = MapViewOfFile( mapping, perm, 0, 0, mi->size );
    CloseHandle( mapping );

    return mi->base != NULL;
}

static BOOL unmap_file_from_memory( struct mapping_info *mi )
{
    if (mi->base)
        UnmapViewOfFile( mi->base );
    mi->base = NULL;
    return TRUE;
}

static void destroy_mapping( struct mapping_info *mi )
{
    if (!mi)
        return;
    unmap_file_from_memory( mi );
    if (mi->file)
        CloseHandle( mi->file );
    HeapFree( GetProcessHeap(), 0, mi );
}

static struct mapping_info *create_mapping( LPCWSTR name, BOOL rw )
{
    struct mapping_info *mi;

    mi = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *mi );
    if (!mi)
        return NULL;

    mi->read_write = rw;

    mi->file = CreateFileW( name, GENERIC_READ | (rw ? GENERIC_WRITE : 0),
                            0, NULL, OPEN_EXISTING, 0, 0 );

    if (mi->file != INVALID_HANDLE_VALUE)
    {
        mi->size = GetFileSize( mi->file, NULL );

        if (map_file_into_memory( mi ))
            return mi;
    }
    destroy_mapping( mi );
    return NULL;
}

static BOOL resize_mapping( struct mapping_info *mi, DWORD new_size )
{
    if (!unmap_file_from_memory( mi ))
        return FALSE;

    /* change the file size */
    SetFilePointer( mi->file, new_size, NULL, FILE_BEGIN );
    if (!SetEndOfFile( mi->file ))
    {
        ERR("failed to set file size to %08x\n", new_size );
        return FALSE;
    }

    mi->size = new_size;

    return map_file_into_memory( mi );
}

static void get_resource_sizes( QUEUEDUPDATES *updates, struct resource_size_info *si )
{
    struct resource_dir_entry *types, *names;
    struct resource_data *data;
    DWORD num_types = 0, num_names = 0, num_langs = 0, strings_size = 0, data_size = 0;

    memset( si, 0, sizeof *si );

    LIST_FOR_EACH_ENTRY( types, &updates->root, struct resource_dir_entry, entry )
    {
        num_types++;
        if (!IS_INTRESOURCE( types->id ))
            strings_size += sizeof (WORD) + lstrlenW( types->id )*sizeof (WCHAR);

        LIST_FOR_EACH_ENTRY( names, &types->children, struct resource_dir_entry, entry )
        {
            num_names++;

            if (!IS_INTRESOURCE( names->id ))
                strings_size += sizeof (WORD) + lstrlenW( names->id )*sizeof (WCHAR);

            LIST_FOR_EACH_ENTRY( data, &names->children, struct resource_data, entry )
            {
                num_langs++;
                data_size += (data->cbData + 3) & ~3;
            }
        }
    }

    /* names are at the end of the types */
    si->names_ofs = sizeof (IMAGE_RESOURCE_DIRECTORY) +
            num_types * sizeof (IMAGE_RESOURCE_DIRECTORY_ENTRY);

    /* language directories are at the end of the names */
    si->langs_ofs = si->names_ofs +
            num_types * sizeof (IMAGE_RESOURCE_DIRECTORY) +
            num_names * sizeof (IMAGE_RESOURCE_DIRECTORY_ENTRY);

    si->data_entry_ofs = si->langs_ofs +
            num_names * sizeof (IMAGE_RESOURCE_DIRECTORY) +
            num_langs * sizeof (IMAGE_RESOURCE_DIRECTORY_ENTRY);

    si->strings_ofs = si->data_entry_ofs +
            num_langs * sizeof (IMAGE_RESOURCE_DATA_ENTRY);

    si->data_ofs = si->strings_ofs + ((strings_size + 3) & ~3);

    si->total_size = si->data_ofs + data_size;

    TRACE("names %08x langs %08x data entries %08x strings %08x data %08x total %08x\n",
          si->names_ofs, si->langs_ofs, si->data_entry_ofs,
          si->strings_ofs, si->data_ofs, si->total_size);
}

static void res_write_padding( BYTE *res_base, DWORD size )
{
    static const BYTE pad[] = {
        'P','A','D','D','I','N','G','X','X','P','A','D','D','I','N','G' };
    DWORD i;

    for ( i = 0; i < size / sizeof pad; i++ )
        memcpy( &res_base[i*sizeof pad], pad, sizeof pad );
    memcpy( &res_base[i*sizeof pad], pad, size%sizeof pad );
}

static BOOL write_resources( QUEUEDUPDATES *updates, LPBYTE base, struct resource_size_info *si, DWORD rva )
{
    struct resource_dir_entry *types, *names;
    struct resource_data *data;
    IMAGE_RESOURCE_DIRECTORY *root;

    TRACE("%p %p %p %08x\n", updates, base, si, rva );

    memset( base, 0, si->total_size );

    /* the root entry always exists */
    root = (IMAGE_RESOURCE_DIRECTORY*) base;
    memset( root, 0, sizeof *root );
    root->MajorVersion = 4;
    si->types_ofs = sizeof *root;
    LIST_FOR_EACH_ENTRY( types, &updates->root, struct resource_dir_entry, entry )
    {
        IMAGE_RESOURCE_DIRECTORY_ENTRY *e1;
        IMAGE_RESOURCE_DIRECTORY *namedir;

        e1 = (IMAGE_RESOURCE_DIRECTORY_ENTRY*) &base[si->types_ofs];
        memset( e1, 0, sizeof *e1 );
        if (!IS_INTRESOURCE( types->id ))
        {
            WCHAR *strings;
            DWORD len;

            root->NumberOfNamedEntries++;
            e1->NameIsString = 1;
            e1->NameOffset = si->strings_ofs;

            strings = (WCHAR*) &base[si->strings_ofs];
            len = lstrlenW( types->id );
            strings[0] = len;
            memcpy( &strings[1], types->id, len * sizeof (WCHAR) );
            si->strings_ofs += (len + 1) * sizeof (WCHAR);
        }
        else
        {
            root->NumberOfIdEntries++;
            e1->Id = LOWORD( types->id );
        }
        e1->OffsetToDirectory = si->names_ofs;
        e1->DataIsDirectory = TRUE;
        si->types_ofs += sizeof (IMAGE_RESOURCE_DIRECTORY_ENTRY);

        namedir = (IMAGE_RESOURCE_DIRECTORY*) &base[si->names_ofs];
        memset( namedir, 0, sizeof *namedir );
        namedir->MajorVersion = 4;
        si->names_ofs += sizeof (IMAGE_RESOURCE_DIRECTORY);

        LIST_FOR_EACH_ENTRY( names, &types->children, struct resource_dir_entry, entry )
        {
            IMAGE_RESOURCE_DIRECTORY_ENTRY *e2;
            IMAGE_RESOURCE_DIRECTORY *langdir;

            e2 = (IMAGE_RESOURCE_DIRECTORY_ENTRY*) &base[si->names_ofs];
            memset( e2, 0, sizeof *e2 );
            if (!IS_INTRESOURCE( names->id ))
            {
                WCHAR *strings;
                DWORD len;

                namedir->NumberOfNamedEntries++;
                e2->NameIsString = 1;
                e2->NameOffset = si->strings_ofs;

                strings = (WCHAR*) &base[si->strings_ofs];
                len = lstrlenW( names->id );
                strings[0] = len;
                memcpy( &strings[1], names->id, len * sizeof (WCHAR) );
                si->strings_ofs += (len + 1) * sizeof (WCHAR);
            }
            else
            {
                namedir->NumberOfIdEntries++;
                e2->Id = LOWORD( names->id );
            }
            e2->OffsetToDirectory = si->langs_ofs;
            e2->DataIsDirectory = TRUE;
            si->names_ofs += sizeof (IMAGE_RESOURCE_DIRECTORY_ENTRY);

            langdir = (IMAGE_RESOURCE_DIRECTORY*) &base[si->langs_ofs];
            memset( langdir, 0, sizeof *langdir );
            langdir->MajorVersion = 4;
            si->langs_ofs += sizeof (IMAGE_RESOURCE_DIRECTORY);

            LIST_FOR_EACH_ENTRY( data, &names->children, struct resource_data, entry )
            {
                IMAGE_RESOURCE_DIRECTORY_ENTRY *e3;
                IMAGE_RESOURCE_DATA_ENTRY *de;
                int pad_size;

                e3 = (IMAGE_RESOURCE_DIRECTORY_ENTRY*) &base[si->langs_ofs];
                memset( e3, 0, sizeof *e3 );
                langdir->NumberOfIdEntries++;
                e3->Id = LOWORD( data->lang );
                e3->OffsetToData = si->data_entry_ofs;

                si->langs_ofs += sizeof (IMAGE_RESOURCE_DIRECTORY_ENTRY);

                /* write out all the data entries */
                de = (IMAGE_RESOURCE_DATA_ENTRY*) &base[si->data_entry_ofs];
                memset( de, 0, sizeof *de );
                de->OffsetToData = si->data_ofs + rva;
                de->Size = data->cbData;
                de->CodePage = data->codepage;
                si->data_entry_ofs += sizeof (IMAGE_RESOURCE_DATA_ENTRY);

                /* write out the resource data */
                memcpy( &base[si->data_ofs], data->lpData, data->cbData );
                si->data_ofs += data->cbData;

                pad_size = (-si->data_ofs)&3;
                res_write_padding( &base[si->data_ofs], pad_size );
                si->data_ofs += pad_size;
            }
        }
    }

    return TRUE;
}

/*
 *  FIXME:
 *  Assumes that the resources are in .rsrc
 *   and .rsrc is the last section in the file.
 *  Not sure whether updating resources will other cases on Windows.
 *  If the resources lie in a section containing other data,
 *   resizing that section could possibly cause trouble.
 *  If the section with the resources isn't last, the remaining
 *   sections need to be moved down in the file, and the section header
 *   would need to be adjusted.
 *  If we needed to add a section, what would we name it?
 *  If we needed to add a section and there wasn't space in the file
 *   header, how would that work?
 *  Seems that at least some of these cases can't be handled properly.
 */
static IMAGE_SECTION_HEADER *get_resource_section( void *base, DWORD mapping_size )
{
    IMAGE_SECTION_HEADER *sec;
    IMAGE_NT_HEADERS *nt;
    DWORD i, num_sections = 0;

    nt = get_nt_header( base, mapping_size );
    if (!nt)
        return NULL;

    sec = get_section_header( base, mapping_size, &num_sections );
    if (!sec)
        return NULL;

    /* find the resources section */
    for (i=0; i<num_sections; i++)
        if (!memcmp(sec[i].Name, ".rsrc", 6))
            break;

    if (i == num_sections)
        return NULL;

    return &sec[i];
}

static DWORD get_init_data_size( void *base, DWORD mapping_size )
{
    DWORD i, sz = 0, num_sections = 0;
    IMAGE_SECTION_HEADER *s;

    s = get_section_header( base, mapping_size, &num_sections );

    for (i=0; i<num_sections; i++)
        if (s[i].Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA)
            sz += s[i].SizeOfRawData;

    TRACE("size = %08x\n", sz);

    return sz;
}

static BOOL write_raw_resources( QUEUEDUPDATES *updates )
{
    static const WCHAR prefix[] = { 'r','e','s','u',0 };
    WCHAR tempdir[MAX_PATH], tempfile[MAX_PATH];
    DWORD section_size;
    BOOL ret = FALSE;
    IMAGE_SECTION_HEADER *sec;
    IMAGE_NT_HEADERS32 *nt;
    IMAGE_NT_HEADERS64 *nt64;
    struct resource_size_info res_size;
    BYTE *res_base;
    struct mapping_info *read_map = NULL, *write_map = NULL;
    DWORD PeSectionAlignment, PeFileAlignment, PeSizeOfImage;

    /* copy the exe to a temp file then update the temp file... */
    tempdir[0] = 0;
    if (!GetTempPathW( MAX_PATH, tempdir ))
        return ret;

    if (!GetTempFileNameW( tempdir, prefix, 0, tempfile ))
        return ret;

    if (!CopyFileW( updates->pFileName, tempfile, FALSE ))
        goto done;

    TRACE("tempfile %s\n", debugstr_w(tempfile));

    if (!updates->bDeleteExistingResources)
    {
        read_map = create_mapping( updates->pFileName, FALSE );
        if (!read_map)
            goto done;

        ret = read_mapped_resources( updates, read_map->base, read_map->size );
        if (!ret)
        {
            ERR("failed to read existing resources\n");
            goto done;
        }
    }

    write_map = create_mapping( tempfile, TRUE );
    if (!write_map)
        goto done;

    nt = (IMAGE_NT_HEADERS32*)get_nt_header( write_map->base, write_map->size );
    if (!nt)
        goto done;

    nt64 = (IMAGE_NT_HEADERS64*)nt;
    if (nt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        PeSectionAlignment = nt64->OptionalHeader.SectionAlignment;
        PeFileAlignment = nt64->OptionalHeader.FileAlignment;
        PeSizeOfImage = nt64->OptionalHeader.SizeOfImage;
    } else {
        PeSectionAlignment = nt->OptionalHeader.SectionAlignment;
        PeFileAlignment = nt->OptionalHeader.FileAlignment;
        PeSizeOfImage = nt->OptionalHeader.SizeOfImage;
    }

    if ((LONG)PeSectionAlignment <= 0)
    {
        ERR("invalid section alignment %08x\n", PeSectionAlignment);
        goto done;
    }

    if ((LONG)PeFileAlignment <= 0)
    {
        ERR("invalid file alignment %08x\n", PeFileAlignment);
        goto done;
    }

    sec = get_resource_section( write_map->base, write_map->size );
    if (!sec) /* no section, add one */
    {
        DWORD num_sections;

        sec = get_section_header( write_map->base, write_map->size, &num_sections );
        if (!sec)
            goto done;

        sec += num_sections;
        nt->FileHeader.NumberOfSections++;

        memset( sec, 0, sizeof *sec );
        memcpy( sec->Name, ".rsrc", 5 );
        sec->Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ;
        sec->VirtualAddress = PeSizeOfImage;
    }

    if (!sec->PointerToRawData)  /* empty section */
    {
        sec->PointerToRawData = write_map->size + (-write_map->size) % PeFileAlignment;
        sec->SizeOfRawData = 0;
    }

    TRACE("before .rsrc at %08x, size %08x\n", sec->PointerToRawData, sec->SizeOfRawData);

    get_resource_sizes( updates, &res_size );

    /* round up the section size */
    section_size = res_size.total_size;
    section_size += (-section_size) % PeFileAlignment;

    TRACE("requires %08x (%08x) bytes\n", res_size.total_size, section_size );

    /* check if the file size needs to be changed */
    if (section_size != sec->SizeOfRawData)
    {
        DWORD old_size = write_map->size;
        DWORD virtual_section_size = res_size.total_size + (-res_size.total_size) % PeSectionAlignment;
        int delta = section_size - (sec->SizeOfRawData + (-sec->SizeOfRawData) % PeFileAlignment);
        int rva_delta = virtual_section_size -
            (sec->Misc.VirtualSize + (-sec->Misc.VirtualSize) % PeSectionAlignment);
        /* when new section is added it could end past current mapping size */
        BOOL rsrc_is_last = sec->PointerToRawData + sec->SizeOfRawData >= old_size;
	/* align .rsrc size when possible */
        DWORD mapping_size = rsrc_is_last ? sec->PointerToRawData + section_size : old_size + delta;

        /* postpone file truncation if there are some data to be moved down from file end */
        BOOL resize_after = mapping_size < old_size && !rsrc_is_last;

        TRACE("file size %08x -> %08x\n", old_size, mapping_size);

        if (!resize_after)
        {
            /* unmap the file before changing the file size */
            ret = resize_mapping( write_map, mapping_size );

            /* get the pointers again - they might be different after remapping */
            nt = (IMAGE_NT_HEADERS32*)get_nt_header( write_map->base, mapping_size );
            if (!nt)
            {
                ERR("couldn't get NT header\n");
                goto done;
            }
            nt64 = (IMAGE_NT_HEADERS64*)nt;

            sec = get_resource_section( write_map->base, mapping_size );
            if (!sec)
                 goto done;
        }

        if (!rsrc_is_last) /* not last section, relocate trailing sections */
        {
            IMAGE_SECTION_HEADER *s;
            DWORD tail_start = sec->PointerToRawData + sec->SizeOfRawData;
            DWORD i, num_sections = 0;

            memmove( (char*)write_map->base + tail_start + delta, (char*)write_map->base + tail_start, old_size - tail_start );

            s = get_section_header( write_map->base, mapping_size, &num_sections );

            for (i=0; i<num_sections; i++)
            {
                if (s[i].PointerToRawData > sec->PointerToRawData)
                {
                    s[i].PointerToRawData += delta;
                    s[i].VirtualAddress += rva_delta;
                }
            }
        }

        if (resize_after)
        {
            ret = resize_mapping( write_map, mapping_size );

            nt = (IMAGE_NT_HEADERS32*)get_nt_header( write_map->base, mapping_size );
            if (!nt)
            {
                ERR("couldn't get NT header\n");
                goto done;
            }
            nt64 = (IMAGE_NT_HEADERS64*)nt;

            sec = get_resource_section( write_map->base, mapping_size );
            if (!sec)
                 goto done;
        }

        /* adjust the PE header information */
        sec->SizeOfRawData = section_size;
        sec->Misc.VirtualSize = virtual_section_size;
        if (nt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
            nt64->OptionalHeader.SizeOfImage += rva_delta;
            nt64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress = sec->VirtualAddress;
            nt64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size = res_size.total_size;
            nt64->OptionalHeader.SizeOfInitializedData = get_init_data_size( write_map->base, mapping_size );
        } else {
            nt->OptionalHeader.SizeOfImage += rva_delta;
            nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress = sec->VirtualAddress;
            nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].Size = res_size.total_size;
            nt->OptionalHeader.SizeOfInitializedData = get_init_data_size( write_map->base, mapping_size );
        }
    }

    res_base = (LPBYTE) write_map->base + sec->PointerToRawData;

    TRACE("base = %p offset = %08x\n", write_map->base, sec->PointerToRawData);

    ret = write_resources( updates, res_base, &res_size, sec->VirtualAddress );

    res_write_padding( res_base + res_size.total_size, section_size - res_size.total_size );

    TRACE("after  .rsrc at %08x, size %08x\n", sec->PointerToRawData, sec->SizeOfRawData);

done:
    destroy_mapping( read_map );
    destroy_mapping( write_map );

    if (ret)
        ret = CopyFileW( tempfile, updates->pFileName, FALSE );

    DeleteFileW( tempfile );

    return ret;
}

/***********************************************************************
 *          BeginUpdateResourceW                 (KERNEL32.@)
 */
HANDLE WINAPI BeginUpdateResourceW( LPCWSTR pFileName, BOOL bDeleteExistingResources )
{
    QUEUEDUPDATES *updates = NULL;
    HANDLE hUpdate, file, ret = NULL;

    TRACE("%s, %d\n", debugstr_w(pFileName), bDeleteExistingResources);

    hUpdate = GlobalAlloc(GHND, sizeof(QUEUEDUPDATES));
    if (!hUpdate)
        return ret;

    updates = GlobalLock(hUpdate);
    if (updates)
    {
        list_init( &updates->root );
        updates->bDeleteExistingResources = bDeleteExistingResources;
        updates->pFileName = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(pFileName)+1)*sizeof(WCHAR));
        if (updates->pFileName)
        {
            lstrcpyW(updates->pFileName, pFileName);

            file = CreateFileW( pFileName, GENERIC_READ | GENERIC_WRITE,
                                0, NULL, OPEN_EXISTING, 0, 0 );

            /* if resources are deleted, only the file's presence is checked */
            if (file != INVALID_HANDLE_VALUE &&
                (bDeleteExistingResources || check_pe_exe( file, updates )))
                ret = hUpdate;
            else
                HeapFree( GetProcessHeap(), 0, updates->pFileName );

            CloseHandle( file );
        }
        GlobalUnlock(hUpdate);
    }

    if (!ret)
        GlobalFree(hUpdate);

    return ret;
}


/***********************************************************************
 *          BeginUpdateResourceA                 (KERNEL32.@)
 */
HANDLE WINAPI BeginUpdateResourceA( LPCSTR pFileName, BOOL bDeleteExistingResources )
{
    UNICODE_STRING FileNameW;
    HANDLE ret;
    RtlCreateUnicodeStringFromAsciiz(&FileNameW, pFileName);
    ret = BeginUpdateResourceW(FileNameW.Buffer, bDeleteExistingResources);
    RtlFreeUnicodeString(&FileNameW);
    return ret;
}


/***********************************************************************
 *          EndUpdateResourceW                 (KERNEL32.@)
 */
BOOL WINAPI EndUpdateResourceW( HANDLE hUpdate, BOOL fDiscard )
{
    QUEUEDUPDATES *updates;
    BOOL ret;

    TRACE("%p %d\n", hUpdate, fDiscard);

    updates = GlobalLock(hUpdate);
    if (!updates)
        return FALSE;

    ret = fDiscard || write_raw_resources( updates );

    free_resource_directory( &updates->root, 2 );

    HeapFree( GetProcessHeap(), 0, updates->pFileName );
    GlobalUnlock( hUpdate );
    GlobalFree( hUpdate );

    return ret;
}


/***********************************************************************
 *          EndUpdateResourceA                 (KERNEL32.@)
 */
BOOL WINAPI EndUpdateResourceA( HANDLE hUpdate, BOOL fDiscard )
{
    return EndUpdateResourceW(hUpdate, fDiscard);
}


/***********************************************************************
 *           UpdateResourceW                 (KERNEL32.@)
 */
BOOL WINAPI UpdateResourceW( HANDLE hUpdate, LPCWSTR lpType, LPCWSTR lpName,
                             WORD wLanguage, LPVOID lpData, DWORD cbData)
{
    QUEUEDUPDATES *updates;
    BOOL ret = FALSE;

    TRACE("%p %s %s %08x %p %d\n", hUpdate,
          debugstr_w(lpType), debugstr_w(lpName), wLanguage, lpData, cbData);

    updates = GlobalLock(hUpdate);
    if (updates)
    {
        if (lpData == NULL && cbData == 0)  /* remove resource */
        {
            ret = update_add_resource( updates, lpType, lpName, wLanguage, NULL, TRUE );
        }
        else
        {
            struct resource_data *data;
            data = allocate_resource_data( wLanguage, 0, lpData, cbData, TRUE );
            if (data)
                ret = update_add_resource( updates, lpType, lpName, wLanguage, data, TRUE );
        }
        GlobalUnlock(hUpdate);
    }
    return ret;
}


/***********************************************************************
 *           UpdateResourceA                 (KERNEL32.@)
 */
BOOL WINAPI UpdateResourceA( HANDLE hUpdate, LPCSTR lpType, LPCSTR lpName,
                             WORD wLanguage, LPVOID lpData, DWORD cbData)
{
    BOOL ret;
    UNICODE_STRING TypeW;
    UNICODE_STRING NameW;
    if(IS_INTRESOURCE(lpType))
        TypeW.Buffer = ULongToPtr(LOWORD(lpType));
    else
        RtlCreateUnicodeStringFromAsciiz(&TypeW, lpType);
    if(IS_INTRESOURCE(lpName))
        NameW.Buffer = ULongToPtr(LOWORD(lpName));
    else
        RtlCreateUnicodeStringFromAsciiz(&NameW, lpName);
    ret = UpdateResourceW(hUpdate, TypeW.Buffer, NameW.Buffer, wLanguage, lpData, cbData);
    if(!IS_INTRESOURCE(lpType)) RtlFreeUnicodeString(&TypeW);
    if(!IS_INTRESOURCE(lpName)) RtlFreeUnicodeString(&NameW);
    return ret;
}
