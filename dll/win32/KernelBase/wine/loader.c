/*
 * Module loader
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 2006 Mike McCormack
 * Copyright 1995, 2003, 2019 Alexandre Julliard
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winternl.h"
#include "ddk/ntddk.h"
#include "kernelbase.h"
#include "wine/list.h"
#include "wine/asm.h"
#include "wine/debug.h"
#include "wine/exception.h"

WINE_DEFAULT_DEBUG_CHANNEL(module);


/* to keep track of LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE file handles */
struct exclusive_datafile
{
    struct list entry;
    HMODULE     module;
    HANDLE      file;
};
static struct list exclusive_datafile_list = LIST_INIT( exclusive_datafile_list );

static CRITICAL_SECTION exclusive_datafile_list_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &exclusive_datafile_list_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": exclusive_datafile_list_section") }
};
static CRITICAL_SECTION exclusive_datafile_list_section = { &critsect_debug, -1, 0, 0, 0, 0 };

/***********************************************************************
 * Modules
 ***********************************************************************/


/******************************************************************
 *      get_proc_address
 */
FARPROC WINAPI get_proc_address( HMODULE module, LPCSTR function )
{
    FARPROC proc;
    ANSI_STRING str;

    if (!module) module = NtCurrentTeb()->Peb->ImageBaseAddress;

    if ((ULONG_PTR)function >> 16)
    {
        RtlInitAnsiString( &str, function );
        if (!set_ntstatus( LdrGetProcedureAddress( module, &str, 0, (void**)&proc ))) return NULL;
    }
    else if (!set_ntstatus( LdrGetProcedureAddress( module, NULL, LOWORD(function), (void**)&proc )))
        return NULL;

    return proc;
}


/******************************************************************
 *      load_library_as_datafile
 */
static BOOL load_library_as_datafile( LPCWSTR load_path, DWORD flags, LPCWSTR name, HMODULE *mod_ret )
{
    WCHAR filenameW[MAX_PATH];
    HANDLE mapping, file = INVALID_HANDLE_VALUE;
    HMODULE module = 0;
    DWORD protect = PAGE_READONLY;

    *mod_ret = 0;

    if (flags & LOAD_LIBRARY_AS_IMAGE_RESOURCE) protect |= SEC_IMAGE;

    if (SearchPathW( NULL, name, L".dll", ARRAY_SIZE( filenameW ), filenameW, NULL ))
    {
        file = CreateFileW( filenameW, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE,
                            NULL, OPEN_EXISTING, 0, 0 );
    }
    if (file == INVALID_HANDLE_VALUE) return FALSE;

    mapping = CreateFileMappingW( file, NULL, protect, 0, 0, NULL );
    if (!mapping) goto failed;

    module = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 );
    CloseHandle( mapping );
    if (!module) goto failed;

    if (!(flags & LOAD_LIBRARY_AS_IMAGE_RESOURCE))
    {
        /* make sure it's a valid PE file */
        if (!RtlImageNtHeader( module ))
        {
            SetLastError( ERROR_BAD_EXE_FORMAT );
            goto failed;
        }
        *mod_ret = (HMODULE)((char *)module + 1); /* set bit 0 for data file module */

        if (flags & LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE)
        {
            struct exclusive_datafile *datafile = HeapAlloc( GetProcessHeap(), 0, sizeof(*datafile) );
            if (!datafile) goto failed;
            datafile->module = *mod_ret;
            datafile->file   = file;
            RtlEnterCriticalSection( &exclusive_datafile_list_section );
            list_add_head( &exclusive_datafile_list, &datafile->entry );
            RtlLeaveCriticalSection( &exclusive_datafile_list_section );
            TRACE( "delaying close %p for module %p\n", datafile->file, datafile->module );
            return TRUE;
        }
    }
    else *mod_ret = (HMODULE)((char *)module + 2); /* set bit 1 for image resource module */

    CloseHandle( file );
    return TRUE;

failed:
    if (module) UnmapViewOfFile( module );
    CloseHandle( file );
    return FALSE;
}


/******************************************************************
 *      load_library
 */
static HMODULE load_library( const UNICODE_STRING *libname, DWORD flags )
{
    const DWORD unsupported_flags = LOAD_IGNORE_CODE_AUTHZ_LEVEL | LOAD_LIBRARY_REQUIRE_SIGNED_TARGET;
    NTSTATUS status;
    HMODULE module;
    WCHAR *load_path, *dummy;

    if (flags & unsupported_flags) FIXME( "unsupported flag(s) used %#08lx\n", flags );

    if (!set_ntstatus( LdrGetDllPath( libname->Buffer, flags, &load_path, &dummy ))) return 0;

    if (flags & (LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE |
                 LOAD_LIBRARY_AS_IMAGE_RESOURCE))
    {
        if (LdrGetDllHandleEx( 0, load_path, NULL, libname, &module ))
            load_library_as_datafile( load_path, flags, libname->Buffer, &module );
    }
    else
    {
        status = LdrLoadDll( load_path, flags, libname, &module );
        if (!set_ntstatus( status ))
        {
            module = 0;
            if (status == STATUS_DLL_NOT_FOUND && (GetVersion() & 0x80000000))
                SetLastError( ERROR_DLL_NOT_FOUND );
        }
    }

    RtlReleasePath( load_path );
    return module;
}


/****************************************************************************
 *	AddDllDirectory   (kernelbase.@)
 */
DLL_DIRECTORY_COOKIE WINAPI DECLSPEC_HOTPATCH AddDllDirectory( const WCHAR *dir )
{
    UNICODE_STRING str;
    void *cookie;

    RtlInitUnicodeString( &str, dir );
    if (!set_ntstatus( LdrAddDllDirectory( &str, &cookie ))) return NULL;
    return cookie;
}


/***********************************************************************
 *	DelayLoadFailureHook   (kernelbase.@)
 */
FARPROC WINAPI DECLSPEC_HOTPATCH DelayLoadFailureHook( LPCSTR name, LPCSTR function )
{
    ULONG_PTR args[2];

    if ((ULONG_PTR)function >> 16)
        ERR( "failed to delay load %s.%s\n", name, function );
    else
        ERR( "failed to delay load %s.%u\n", name, LOWORD(function) );
    args[0] = (ULONG_PTR)name;
    args[1] = (ULONG_PTR)function;
    RaiseException( EXCEPTION_WINE_STUB, EXCEPTION_NONCONTINUABLE, 2, args );
    return NULL;
}


/****************************************************************************
 *	DisableThreadLibraryCalls   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH DisableThreadLibraryCalls( HMODULE module )
{
    return set_ntstatus( LdrDisableThreadCalloutsForDll( module ));
}


/***********************************************************************
 *	FreeLibrary   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FreeLibrary( HINSTANCE module )
{
    if (!module)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    if ((ULONG_PTR)module & 3) /* this is a datafile module */
    {
        void *ptr = (void *)((ULONG_PTR)module & ~3);
        if (!RtlImageNtHeader( ptr ))
        {
            SetLastError( ERROR_BAD_EXE_FORMAT );
            return FALSE;
        }
        if ((ULONG_PTR)module & 1)
        {
            struct exclusive_datafile *file;

            RtlEnterCriticalSection( &exclusive_datafile_list_section );
            LIST_FOR_EACH_ENTRY( file, &exclusive_datafile_list, struct exclusive_datafile, entry )
            {
                if (file->module != module) continue;
                TRACE( "closing %p for module %p\n", file->file, file->module );
                CloseHandle( file->file );
                list_remove( &file->entry );
                HeapFree( GetProcessHeap(), 0, file );
                break;
            }
            RtlLeaveCriticalSection( &exclusive_datafile_list_section );
        }
        return UnmapViewOfFile( ptr );
    }

    return set_ntstatus( LdrUnloadDll( module ));
}


/***********************************************************************
 *	GetModuleFileNameA   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetModuleFileNameA( HMODULE module, LPSTR filename, DWORD size )
{
    LPWSTR filenameW = HeapAlloc( GetProcessHeap(), 0, size * sizeof(WCHAR) );
    DWORD len;

    if (!filenameW)
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return 0;
    }
    if ((len = GetModuleFileNameW( module, filenameW, size )))
    {
    	len = file_name_WtoA( filenameW, len, filename, size );
        if (len < size)
            filename[len] = 0;
        else
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
    }
    HeapFree( GetProcessHeap(), 0, filenameW );
    return len;
}


/***********************************************************************
 *	GetModuleFileNameW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetModuleFileNameW( HMODULE module, LPWSTR filename, DWORD size )
{
    ULONG len = 0;
    WIN16_SUBSYSTEM_TIB *win16_tib;
    UNICODE_STRING name;
    NTSTATUS status;

    if (!module && ((win16_tib = NtCurrentTeb()->Tib.SubSystemTib)) && win16_tib->exe_name)
    {
        len = min( size, win16_tib->exe_name->Length / sizeof(WCHAR) );
        memcpy( filename, win16_tib->exe_name->Buffer, len * sizeof(WCHAR) );
        if (len < size) filename[len] = 0;
        goto done;
    }

    name.Buffer = filename;
    name.MaximumLength = min( size, UNICODE_STRING_MAX_CHARS ) * sizeof(WCHAR);
    status = LdrGetDllFullName( module, &name );
    if (!status || status == STATUS_BUFFER_TOO_SMALL) len = name.Length / sizeof(WCHAR);
    SetLastError( RtlNtStatusToDosError( status ));
done:
    TRACE( "%s\n", debugstr_wn(filename, len) );
    return len;
}


/***********************************************************************
 *	GetModuleHandleA   (kernelbase.@)
 */
HMODULE WINAPI DECLSPEC_HOTPATCH GetModuleHandleA( LPCSTR module )
{
    HMODULE ret;

    GetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, module, &ret );
    return ret;
}


/***********************************************************************
 *	GetModuleHandleW   (kernelbase.@)
 */
HMODULE WINAPI DECLSPEC_HOTPATCH GetModuleHandleW( LPCWSTR module )
{
    HMODULE ret;

    GetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, module, &ret );
    return ret;
}


/***********************************************************************
 *	GetModuleHandleExA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetModuleHandleExA( DWORD flags, LPCSTR name, HMODULE *module )
{
    WCHAR *nameW;

    if (!module)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (!name || (flags & GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS))
        return GetModuleHandleExW( flags, (LPCWSTR)name, module );

    if (!(nameW = file_name_AtoW( name, FALSE )))
    {
        *module = NULL;
        SetLastError( ERROR_MOD_NOT_FOUND );
        return FALSE;
    }
    return GetModuleHandleExW( flags, nameW, module );
}


/***********************************************************************
 *	GetModuleHandleExW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetModuleHandleExW( DWORD flags, LPCWSTR name, HMODULE *module )
{
    HMODULE ret = NULL;
    NTSTATUS status;
    void *dummy;

    if (!module)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if ((flags & ~(GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT
                  | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS))
                  || (flags & (GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT))
                  == (GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT))
    {
        *module = NULL;
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (name && !(flags & GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS))
    {
        UNICODE_STRING wstr;
        ULONG ldr_flags = 0;

        if (flags & GET_MODULE_HANDLE_EX_FLAG_PIN)
            ldr_flags |= LDR_GET_DLL_HANDLE_EX_FLAG_PIN;
        if (flags & GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT)
            ldr_flags |= LDR_GET_DLL_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;

        RtlInitUnicodeString( &wstr, name );
        status = LdrGetDllHandleEx( ldr_flags, NULL, NULL, &wstr, &ret );
    }
    else
    {
        ret = name ? RtlPcToFileHeader( (void *)name, &dummy ) : NtCurrentTeb()->Peb->ImageBaseAddress;

        if (ret)
        {
            if (!(flags & GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT))
                status = LdrAddRefDll( flags & GET_MODULE_HANDLE_EX_FLAG_PIN ? LDR_ADDREF_DLL_PIN : 0, ret );
            else
                status = STATUS_SUCCESS;
        } else status = STATUS_DLL_NOT_FOUND;
    }

    *module = ret;
    return set_ntstatus( status );
}


/***********************************************************************
 *	GetProcAddress   (kernelbase.@)
 */

/*
 * Work around a Delphi bug on x86_64.  When delay loading a symbol,
 * Delphi saves rcx, rdx, r8 and r9 to the stack.  It then calls
 * GetProcAddress(), pops the saved registers and calls the function.
 * This works fine if all of the parameters are ints.  However, since
 * it does not save xmm0 - 3, it relies on GetProcAddress() preserving
 * these registers if the function takes floating point parameters.
 * This wrapper saves xmm0 - 3 to the stack.
 */
#ifdef __arm64ec__
FARPROC WINAPI __attribute__((naked)) GetProcAddress( HMODULE module, LPCSTR function )
{
    asm( ".seh_proc \"#GetProcAddress\"\n\t"
         "stp x29, x30, [sp, #-48]!\n\t"
         ".seh_save_fplr_x 48\n\t"
         ".seh_endprologue\n\t"
         "stp d0, d1, [sp, #16]\n\t"
         "stp d2, d3, [sp, #32]\n\t"
         "bl \"#get_proc_address\"\n\t"
         "ldp d0, d1, [sp, #16]\n\t"
         "ldp d2, d3, [sp, #32]\n\t"
         "ldp x29, x30, [sp], #48\n\t"
         "ret\n\t"
         ".seh_endproc" );
}
#elif defined(__x86_64__)
__ASM_GLOBAL_FUNC( GetProcAddress,
                   ".byte 0x48\n\t"  /* hotpatch prolog */
                   "pushq %rbp\n\t"
                   __ASM_SEH(".seh_pushreg %rbp\n\t")
                   __ASM_CFI(".cfi_adjust_cfa_offset 8\n\t")
                   __ASM_CFI(".cfi_rel_offset %rbp,0\n\t")
                   "movq %rsp,%rbp\n\t"
                   __ASM_SEH(".seh_setframe %rbp,0\n\t")
                   __ASM_CFI(".cfi_def_cfa_register %rbp\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   "subq $0x60,%rsp\n\t"
                   "andq $~15,%rsp\n\t"
                   "movaps %xmm0,0x20(%rsp)\n\t"
                   "movaps %xmm1,0x30(%rsp)\n\t"
                   "movaps %xmm2,0x40(%rsp)\n\t"
                   "movaps %xmm3,0x50(%rsp)\n\t"
                   "call " __ASM_NAME("get_proc_address") "\n\t"
                   "movaps 0x50(%rsp), %xmm3\n\t"
                   "movaps 0x40(%rsp), %xmm2\n\t"
                   "movaps 0x30(%rsp), %xmm1\n\t"
                   "movaps 0x20(%rsp), %xmm0\n\t"
                   "leaq 0(%rbp),%rsp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %rsp\n\t")
                   "popq %rbp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset -8\n\t")
                   __ASM_CFI(".cfi_same_value %rbp\n\t")
                   "ret" )
#else /* __x86_64__ */

FARPROC WINAPI DECLSPEC_HOTPATCH GetProcAddress( HMODULE module, LPCSTR function )
{
    return get_proc_address( module, function );
}

#endif /* __x86_64__ */


/***********************************************************************
 *	IsApiSetImplemented   (kernelbase.@)
 */
BOOL WINAPI IsApiSetImplemented( LPCSTR name )
{
    UNICODE_STRING str;
    NTSTATUS status;
    BOOLEAN in_schema, present;

    if (!RtlCreateUnicodeStringFromAsciiz( &str, name )) return FALSE;
    status = ApiSetQueryApiSetPresenceEx( &str, &in_schema, &present );
    RtlFreeUnicodeString( &str );
    return !status && present;
}


/***********************************************************************
 *	LoadLibraryA   (kernelbase.@)
 */
HMODULE WINAPI DECLSPEC_HOTPATCH LoadLibraryA( LPCSTR name )
{
    return LoadLibraryExA( name, 0, 0 );
}


/***********************************************************************
 *	LoadLibraryW   (kernelbase.@)
 */
HMODULE WINAPI DECLSPEC_HOTPATCH LoadLibraryW( LPCWSTR name )
{
    return LoadLibraryExW( name, 0, 0 );
}


/******************************************************************
 *	LoadLibraryExA   (kernelbase.@)
 */
HMODULE WINAPI DECLSPEC_HOTPATCH LoadLibraryExA( LPCSTR name, HANDLE file, DWORD flags )
{
    WCHAR *nameW;

    if (!(nameW = file_name_AtoW( name, FALSE ))) return 0;
    return LoadLibraryExW( nameW, file, flags );
}


/***********************************************************************
 *	LoadLibraryExW   (kernelbase.@)
 */
HMODULE WINAPI DECLSPEC_HOTPATCH LoadLibraryExW( LPCWSTR name, HANDLE file, DWORD flags )
{
    UNICODE_STRING str;
    HMODULE module;

    if (!name)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    RtlInitUnicodeString( &str, name );
    if (str.Buffer[str.Length/sizeof(WCHAR) - 1] != ' ') return load_library( &str, flags );

    /* library name has trailing spaces */
    RtlCreateUnicodeString( &str, name );
    while (str.Length > sizeof(WCHAR) && str.Buffer[str.Length/sizeof(WCHAR) - 1] == ' ')
        str.Length -= sizeof(WCHAR);

    str.Buffer[str.Length/sizeof(WCHAR)] = 0;
    module = load_library( &str, flags );
    RtlFreeUnicodeString( &str );
    return module;
}


/***********************************************************************
 *      LoadPackagedLibrary    (kernelbase.@)
 */
HMODULE WINAPI /* DECLSPEC_HOTPATCH */ LoadPackagedLibrary( LPCWSTR name, DWORD reserved )
{
    FIXME( "semi-stub, name %s, reserved %#lx.\n", debugstr_w(name), reserved );
    SetLastError( APPMODEL_ERROR_NO_PACKAGE );
    return NULL;
}


/***********************************************************************
 *      LoadAppInitDlls    (kernelbase.@)
 */
void WINAPI LoadAppInitDlls(void)
{
    TRACE( "\n" );
}


/****************************************************************************
 *	RemoveDllDirectory   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH RemoveDllDirectory( DLL_DIRECTORY_COOKIE cookie )
{
    return set_ntstatus( LdrRemoveDllDirectory( cookie ));
}


/*************************************************************************
 *	SetDefaultDllDirectories   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetDefaultDllDirectories( DWORD flags )
{
    return set_ntstatus( LdrSetDefaultDllDirectories( flags ));
}


/***********************************************************************
 * Resources
 ***********************************************************************/


#define IS_INTRESOURCE(x)   (((ULONG_PTR)(x) >> 16) == 0)

/* retrieve the resource name to pass to the ntdll functions */
static NTSTATUS get_res_nameA( LPCSTR name, UNICODE_STRING *str )
{
    if (IS_INTRESOURCE(name))
    {
        str->Buffer = ULongToPtr( LOWORD(name) );
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
        str->Buffer = ULongToPtr( LOWORD(name) );
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


/**********************************************************************
 *	EnumResourceLanguagesExA	(kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumResourceLanguagesExA( HMODULE module, LPCSTR type, LPCSTR name,
                                                        ENUMRESLANGPROCA func, LONG_PTR param,
                                                        DWORD flags, LANGID lang )
{
    int i;
    BOOL ret = FALSE;
    NTSTATUS status;
    UNICODE_STRING typeW, nameW;
    LDR_RESOURCE_INFO info;
    const IMAGE_RESOURCE_DIRECTORY *basedir, *resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;

    TRACE( "%p %s %s %p %Ix %lx %d\n", module, debugstr_a(type), debugstr_a(name),
           func, param, flags, lang );

    if (flags & (RESOURCE_ENUM_MUI | RESOURCE_ENUM_MUI_SYSTEM | RESOURCE_ENUM_VALIDATE))
        FIXME( "unimplemented flags: %lx\n", flags );

    if (!flags) flags = RESOURCE_ENUM_LN | RESOURCE_ENUM_MUI;
    if (!(flags & RESOURCE_ENUM_LN)) return ret;

    if (!module) module = GetModuleHandleW( 0 );
    typeW.Buffer = nameW.Buffer = NULL;
    if ((status = LdrFindResourceDirectory_U( module, NULL, 0, &basedir )) != STATUS_SUCCESS)
        goto done;
    if ((status = get_res_nameA( type, &typeW )) != STATUS_SUCCESS)
        goto done;
    if ((status = get_res_nameA( name, &nameW )) != STATUS_SUCCESS)
        goto done;
    info.Type = (ULONG_PTR)typeW.Buffer;
    info.Name = (ULONG_PTR)nameW.Buffer;
    if ((status = LdrFindResourceDirectory_U( module, &info, 2, &resdir )) != STATUS_SUCCESS)
        goto done;

    et = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(resdir + 1);
    __TRY
    {
        for (i = 0; i < resdir->NumberOfNamedEntries + resdir->NumberOfIdEntries; i++)
        {
            ret = func( module, type, name, et[i].Id, param );
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
 *	EnumResourceLanguagesExW	(kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumResourceLanguagesExW( HMODULE module, LPCWSTR type, LPCWSTR name,
                                                        ENUMRESLANGPROCW func, LONG_PTR param,
                                                        DWORD flags, LANGID lang )
{
    int i;
    BOOL ret = FALSE;
    NTSTATUS status;
    UNICODE_STRING typeW, nameW;
    LDR_RESOURCE_INFO info;
    const IMAGE_RESOURCE_DIRECTORY *basedir, *resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;

    TRACE( "%p %s %s %p %Ix %lx %d\n", module, debugstr_w(type), debugstr_w(name),
           func, param, flags, lang );

    if (flags & (RESOURCE_ENUM_MUI | RESOURCE_ENUM_MUI_SYSTEM | RESOURCE_ENUM_VALIDATE))
        FIXME( "unimplemented flags: %lx\n", flags );

    if (!flags) flags = RESOURCE_ENUM_LN | RESOURCE_ENUM_MUI;
    if (!(flags & RESOURCE_ENUM_LN)) return ret;

    if (!module) module = GetModuleHandleW( 0 );
    typeW.Buffer = nameW.Buffer = NULL;
    if ((status = LdrFindResourceDirectory_U( module, NULL, 0, &basedir )) != STATUS_SUCCESS)
        goto done;
    if ((status = get_res_nameW( type, &typeW )) != STATUS_SUCCESS)
        goto done;
    if ((status = get_res_nameW( name, &nameW )) != STATUS_SUCCESS)
        goto done;
    info.Type = (ULONG_PTR)typeW.Buffer;
    info.Name = (ULONG_PTR)nameW.Buffer;
    if ((status = LdrFindResourceDirectory_U( module, &info, 2, &resdir )) != STATUS_SUCCESS)
        goto done;

    et = (const IMAGE_RESOURCE_DIRECTORY_ENTRY *)(resdir + 1);
    __TRY
    {
        for (i = 0; i < resdir->NumberOfNamedEntries + resdir->NumberOfIdEntries; i++)
        {
            ret = func( module, type, name, et[i].Id, param );
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
 *	EnumResourceNamesExA	(kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumResourceNamesExA( HMODULE module, LPCSTR type, ENUMRESNAMEPROCA func,
                                                    LONG_PTR param, DWORD flags, LANGID lang )
{
    int i;
    BOOL ret = FALSE;
    DWORD len = 0, newlen;
    LPSTR name = NULL;
    NTSTATUS status;
    UNICODE_STRING typeW;
    LDR_RESOURCE_INFO info;
    const IMAGE_RESOURCE_DIRECTORY *basedir, *resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;
    const IMAGE_RESOURCE_DIR_STRING_U *str;

    TRACE( "%p %s %p %Ix\n", module, debugstr_a(type), func, param );

    if (flags & (RESOURCE_ENUM_MUI | RESOURCE_ENUM_MUI_SYSTEM | RESOURCE_ENUM_VALIDATE))
        FIXME( "unimplemented flags: %lx\n", flags );

    if (!flags) flags = RESOURCE_ENUM_LN | RESOURCE_ENUM_MUI;
    if (!(flags & RESOURCE_ENUM_LN)) return ret;

    if (!module) module = GetModuleHandleW( 0 );
    typeW.Buffer = NULL;
    if ((status = LdrFindResourceDirectory_U( module, NULL, 0, &basedir )) != STATUS_SUCCESS)
        goto done;
    if ((status = get_res_nameA( type, &typeW )) != STATUS_SUCCESS)
        goto done;
    info.Type = (ULONG_PTR)typeW.Buffer;
    if ((status = LdrFindResourceDirectory_U( module, &info, 1, &resdir )) != STATUS_SUCCESS)
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
                    if (!(name = HeapAlloc( GetProcessHeap(), 0, len + 1 )))
                    {
                        ret = FALSE;
                        break;
                    }
                }
                WideCharToMultiByte( CP_ACP, 0, str->NameString, str->Length, name, len, NULL, NULL );
                name[newlen] = 0;
                ret = func( module, type, name, param );
            }
            else
            {
                ret = func( module, type, UIntToPtr(et[i].Id), param );
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
 *	EnumResourceNamesExW	(kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumResourceNamesExW( HMODULE module, LPCWSTR type, ENUMRESNAMEPROCW func,
                                                    LONG_PTR param, DWORD flags, LANGID lang )
{
    int i, len = 0;
    BOOL ret = FALSE;
    LPWSTR name = NULL;
    NTSTATUS status;
    UNICODE_STRING typeW;
    LDR_RESOURCE_INFO info;
    const IMAGE_RESOURCE_DIRECTORY *basedir, *resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;
    const IMAGE_RESOURCE_DIR_STRING_U *str;

    TRACE( "%p %s %p %Ix\n", module, debugstr_w(type), func, param );

    if (flags & (RESOURCE_ENUM_MUI | RESOURCE_ENUM_MUI_SYSTEM | RESOURCE_ENUM_VALIDATE))
        FIXME( "unimplemented flags: %lx\n", flags );

    if (!flags) flags = RESOURCE_ENUM_LN | RESOURCE_ENUM_MUI;
    if (!(flags & RESOURCE_ENUM_LN)) return ret;

    if (!module) module = GetModuleHandleW( 0 );
    typeW.Buffer = NULL;
    if ((status = LdrFindResourceDirectory_U( module, NULL, 0, &basedir )) != STATUS_SUCCESS)
        goto done;
    if ((status = get_res_nameW( type, &typeW )) != STATUS_SUCCESS)
        goto done;
    info.Type = (ULONG_PTR)typeW.Buffer;
    if ((status = LdrFindResourceDirectory_U( module, &info, 1, &resdir )) != STATUS_SUCCESS)
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
                ret = func( module, type, name, param );
            }
            else
            {
                ret = func( module, type, UIntToPtr(et[i].Id), param );
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
 *	EnumResourceNamesW	(kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumResourceNamesW( HMODULE module, LPCWSTR type,
                                                  ENUMRESNAMEPROCW func, LONG_PTR param )
{
    return EnumResourceNamesExW( module, type, func, param, 0, 0 );
}


/**********************************************************************
 *	EnumResourceTypesExA	(kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumResourceTypesExA( HMODULE module, ENUMRESTYPEPROCA func, LONG_PTR param,
                                                    DWORD flags, LANGID lang )
{
    int i;
    BOOL ret = FALSE;
    LPSTR type = NULL;
    DWORD len = 0, newlen;
    const IMAGE_RESOURCE_DIRECTORY *resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;
    const IMAGE_RESOURCE_DIR_STRING_U *str;

    TRACE( "%p %p %Ix\n", module, func, param );

    if (flags & (RESOURCE_ENUM_MUI | RESOURCE_ENUM_MUI_SYSTEM | RESOURCE_ENUM_VALIDATE))
        FIXME( "unimplemented flags: %lx\n", flags );

    if (!flags) flags = RESOURCE_ENUM_LN | RESOURCE_ENUM_MUI;
    if (!(flags & RESOURCE_ENUM_LN)) return ret;

    if (!module) module = GetModuleHandleW( 0 );

    if (!set_ntstatus( LdrFindResourceDirectory_U( module, NULL, 0, &resdir ))) return FALSE;

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
            ret = func( module, type, param );
        }
        else
        {
            ret = func( module, UIntToPtr(et[i].Id), param );
        }
        if (!ret) break;
    }
    HeapFree( GetProcessHeap(), 0, type );
    return ret;
}


/**********************************************************************
 *	EnumResourceTypesExW	(kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumResourceTypesExW( HMODULE module, ENUMRESTYPEPROCW func, LONG_PTR param,
                                                    DWORD flags, LANGID lang )
{
    int i, len = 0;
    BOOL ret = FALSE;
    LPWSTR type = NULL;
    const IMAGE_RESOURCE_DIRECTORY *resdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *et;
    const IMAGE_RESOURCE_DIR_STRING_U *str;

    TRACE( "%p %p %Ix\n", module, func, param );

    if (!flags) flags = RESOURCE_ENUM_LN | RESOURCE_ENUM_MUI;
    if (!(flags & RESOURCE_ENUM_LN)) return ret;

    if (!module) module = GetModuleHandleW( 0 );

    if (!set_ntstatus( LdrFindResourceDirectory_U( module, NULL, 0, &resdir ))) return FALSE;

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
            ret = func( module, type, param );
        }
        else
        {
            ret = func( module, UIntToPtr(et[i].Id), param );
        }
        if (!ret) break;
    }
    HeapFree( GetProcessHeap(), 0, type );
    return ret;
}


/**********************************************************************
 *	    FindResourceExW  (kernelbase.@)
 */
HRSRC WINAPI DECLSPEC_HOTPATCH FindResourceExW( HMODULE module, LPCWSTR type, LPCWSTR name, WORD lang )
{
    NTSTATUS status;
    UNICODE_STRING nameW, typeW;
    LDR_RESOURCE_INFO info;
    const IMAGE_RESOURCE_DATA_ENTRY *entry = NULL;

    TRACE( "%p %s %s %04x\n", module, debugstr_w(type), debugstr_w(name), lang );

    if (!module) module = GetModuleHandleW( 0 );
    nameW.Buffer = typeW.Buffer = NULL;

    __TRY
    {
        if ((status = get_res_nameW( name, &nameW )) != STATUS_SUCCESS) goto done;
        if ((status = get_res_nameW( type, &typeW )) != STATUS_SUCCESS) goto done;
        info.Type = (ULONG_PTR)typeW.Buffer;
        info.Name = (ULONG_PTR)nameW.Buffer;
        info.Language = lang;
        status = LdrFindResource_U( module, &info, 3, &entry );
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
 *	    FindResourceW    (kernelbase.@)
 */
HRSRC WINAPI DECLSPEC_HOTPATCH FindResourceW( HINSTANCE module, LPCWSTR name, LPCWSTR type )
{
    return FindResourceExW( module, type, name, MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ) );
}


/**********************************************************************
 *	    FreeResource     (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FreeResource( HGLOBAL handle )
{
    return FALSE;
}


/**********************************************************************
 *	    LoadResource     (kernelbase.@)
 */
HGLOBAL WINAPI DECLSPEC_HOTPATCH LoadResource( HINSTANCE module, HRSRC rsrc )
{
    void *ret;

    TRACE( "%p %p\n", module, rsrc );

    if (!rsrc) return 0;
    if (!module) module = GetModuleHandleW( 0 );
    if (!set_ntstatus( LdrAccessResource( module, (IMAGE_RESOURCE_DATA_ENTRY *)rsrc, &ret, NULL )))
        return 0;
    return ret;
}


/**********************************************************************
 *	    LockResource     (kernelbase.@)
 */
LPVOID WINAPI DECLSPEC_HOTPATCH LockResource( HGLOBAL handle )
{
    return handle;
}


/**********************************************************************
 *	    SizeofResource   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH SizeofResource( HINSTANCE module, HRSRC rsrc )
{
    if (!rsrc) return 0;
    return ((IMAGE_RESOURCE_DATA_ENTRY *)rsrc)->Size;
}


/***********************************************************************
 * Activation contexts
 ***********************************************************************/


/***********************************************************************
 *          ActivateActCtx    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ActivateActCtx( HANDLE context, ULONG_PTR *cookie )
{
    return set_ntstatus( RtlActivateActivationContext( 0, context, cookie ));
}


/***********************************************************************
 *          AddRefActCtx    (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH AddRefActCtx( HANDLE context )
{
    RtlAddRefActivationContext( context );
}


/***********************************************************************
 *          CreateActCtxW    (kernelbase.@)
 */
HANDLE WINAPI DECLSPEC_HOTPATCH CreateActCtxW( PCACTCTXW ctx )
{
    HANDLE context;

    TRACE( "%p %08lx\n", ctx, ctx ? ctx->dwFlags : 0 );

    if (!set_ntstatus( RtlCreateActivationContext( &context, ctx ))) return INVALID_HANDLE_VALUE;
    return context;
}


/***********************************************************************
 *          DeactivateActCtx    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH DeactivateActCtx( DWORD flags, ULONG_PTR cookie )
{
    RtlDeactivateActivationContext( flags, cookie );
    return TRUE;
}


/***********************************************************************
 *          FindActCtxSectionGuid    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FindActCtxSectionGuid( DWORD flags, const GUID *ext_guid, ULONG id,
                                                     const GUID *guid, PACTCTX_SECTION_KEYED_DATA info )
{
    return set_ntstatus( RtlFindActivationContextSectionGuid( flags, ext_guid, id, guid, info ));
}


/***********************************************************************
 *          FindActCtxSectionStringW    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH FindActCtxSectionStringW( DWORD flags, const GUID *ext_guid, ULONG id,
                                                        LPCWSTR str, PACTCTX_SECTION_KEYED_DATA info )
{
    UNICODE_STRING us;

    if (!info)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    RtlInitUnicodeString( &us, str );
    return set_ntstatus( RtlFindActivationContextSectionString( flags, ext_guid, id, &us, info ));
}


/***********************************************************************
 *          GetCurrentActCtx    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetCurrentActCtx( HANDLE *pcontext )
{
    return set_ntstatus( RtlGetActiveActivationContext( pcontext ));
}


/***********************************************************************
 *          QueryActCtxSettingsW    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH QueryActCtxSettingsW( DWORD flags, HANDLE ctx, const WCHAR *ns,
                                                    const WCHAR *settings, WCHAR *buffer, SIZE_T size,
                                                    SIZE_T *written )
{
    return set_ntstatus( RtlQueryActivationContextApplicationSettings( flags, ctx, ns, settings,
                                                                       buffer, size, written ));
}


/***********************************************************************
 *          QueryActCtxW    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH QueryActCtxW( DWORD flags, HANDLE context, PVOID inst, ULONG class,
                                            PVOID buffer, SIZE_T size, SIZE_T *written )
{
    return set_ntstatus( RtlQueryInformationActivationContext( flags, context, inst, class,
                                                               buffer, size, written ));
}


/***********************************************************************
 *          ReleaseActCtx    (kernelbase.@)
 */
void WINAPI DECLSPEC_HOTPATCH ReleaseActCtx( HANDLE context )
{
    RtlReleaseActivationContext( context );
}


/***********************************************************************
 *          ZombifyActCtx    (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH ZombifyActCtx( HANDLE context )
{
    return set_ntstatus( RtlZombifyActivationContext( context ));
}
